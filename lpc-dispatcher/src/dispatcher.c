// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <endian.h>
#include "protocol/components.h"

#define DISPATCHER_PIPE_DIR ".dispatcher"
#define PIPES_DIR ".pipes"
#define INSTALL_PIPE_NAME "install_req_pipe"
#define CONNECTION_PIPE_NAME "connection_req_pipe"

typedef struct {
    char access_path[ACCESS_PATH_LENGTH];
    char call_pipe_name[COMM_PIPE_NAME_LENGTH];
    char return_pipe_name[COMM_PIPE_NAME_LENGTH];
    uint8_t version_len;
    char version[256];
    int is_active;
    int call_fd;   
    int return_fd; 
} ServiceInfo;

typedef struct {
    char connect_pipe_name[256];
    char access_path[256];
} ClientArgs;

ServiceInfo g_service;
pthread_mutex_t g_service_lock = PTHREAD_MUTEX_INITIALIZER;

long read_full(int fd, void *buf, size_t count)
{
    long total = 0;

    // Citim pana umplem tot bufferul
    while (total < count) {
        ssize_t n = read(fd, (char*)buf + total, count - total);
        if (n <= 0) 
            return n;

        total = total + n;
    }

    return total;
}

long write_full(int fd, const void *buf, size_t count)
{
    long total = 0;

    // Scriem pana umplem tot bufferul
    while (total < count) {
        ssize_t n = write(fd, (const char*)buf + total, count - total);
        if (n <= 0)
            return n;

        total = total + n;
    }

    return total;
}

void get_disp_pipe_path(char *buffer, const char *pipe_name)
{
    snprintf(buffer, 256, "%s/%s", DISPATCHER_PIPE_DIR, pipe_name);
}

void get_comm_pipe_path(char *buffer, const char *pipe_name)
{
    if (strncmp(pipe_name, PIPES_DIR, strlen(PIPES_DIR)) == 0) {
        snprintf(buffer, 256, "%s", pipe_name);
    } else {
        snprintf(buffer, 256, "%s/%s", PIPES_DIR, pipe_name);
    }
}

void create_pipe(const char *path)
{
    // Daca pipe ul nu exista il cream
    if (mkfifo(path, 0666) != 0) {
        if (errno != EEXIST) {
             fprintf(stderr, "Eroare la crearea pipe ului %s: %s\n", path, strerror(errno));
        }
    } else {
        fprintf(stderr, "Pipe creat: %s\n", path);
    }
}

void *client_handler(void *arg)
{
    ClientArgs *args = (ClientArgs *)arg;
    
    char p_call_name[64], p_ret_name[64];
    unsigned long id = (unsigned long)pthread_self();
    
    // Numele pipe urilor de comunicare cu clientul
    snprintf(p_call_name, 64, ".pipes/pc_%lu", id);
    snprintf(p_ret_name, 64, ".pipes/pr_%lu", id);

    char p_call_path[256], p_ret_path[256];
    get_comm_pipe_path(p_call_path, p_call_name);
    get_comm_pipe_path(p_ret_path, p_ret_name);

    // Creeaza pipe urile daca nu exista
    create_pipe(p_call_path);
    create_pipe(p_ret_path);

    // Trimite numeel pipe urilor inapoi la client
    char client_conn_path[256];
    get_comm_pipe_path(client_conn_path, args->connect_pipe_name);
    create_pipe(client_conn_path);

    // Deschidem pipe ul de conexiune catre client
    int conn_fd = open(client_conn_path, O_WRONLY);
    if (conn_fd >= 0) {
        struct ConnectHeader ch;
        ch.m_VersionLen = g_service.version_len;

        // Header ul de conexiune in big endian
        ch.m_CpnLen = htobe32(strlen(p_call_name));
        ch.m_RpnLen = htobe32(strlen(p_ret_name));
        
        // Trimite raspunsul inapoi la client
        write_full(conn_fd, &ch, sizeof(ch));
        write_full(conn_fd, g_service.version, g_service.version_len);
        write_full(conn_fd, p_call_name, strlen(p_call_name));
        write_full(conn_fd, p_ret_name, strlen(p_ret_name));

        // Inchidem pipe ul de conexiumne
        close(conn_fd);

        // Deschidem pipe urile de comunicare cu clientul
        int pc_fd = open(p_call_path, O_RDWR); 
        int pr_fd = open(p_ret_path, O_RDWR);

        // Daca s au deschis corect citim si scriem datele
        if (pc_fd >= 0 && pr_fd >= 0) {
            struct CallingHeader call;

            // Citeste request de la client
            if (read_full(pc_fd, &call, sizeof(call)) == sizeof(call)) {
                uint32_t args_len = be32toh(call.m_ArgumentsLen);

                // Calculam lungimea payload ului
                size_t size = call.m_FnLen + 4 * call.m_ArgsCnt + args_len;

                char *payload = malloc(size);

                // Daca avem payload il citim
                if (payload) {
                    read_full(pc_fd, payload, size);

                    // Asiguram ca nu scrie altcnv in service
                    pthread_mutex_lock(&g_service_lock);
                    
                    // Trimitem la Service
                    write_full(g_service.call_fd, &call, sizeof(call));
                    write_full(g_service.call_fd, payload, size);

                    // Primim de la Service
                    struct CallingHeader resp;
                    read_full(g_service.return_fd, &resp, sizeof(resp));
                    
                    // Citim payload ul de raspuns
                    uint32_t r_args_len = be32toh(resp.m_ArgumentsLen);

                    // Calculam lungimea payload
                    size_t r_size = resp.m_FnLen + 4 * resp.m_ArgsCnt + r_args_len;
                    char *r_payload = malloc(r_size);
                    read_full(g_service.return_fd, r_payload, r_size);
                    
                    // Dam unlock pe service
                    pthread_mutex_unlock(&g_service_lock);

                    // Trimitem inapoi la Client
                    write_full(pr_fd, &resp, sizeof(resp));
                    write_full(pr_fd, r_payload, r_size);

                    free(payload);
                    free(r_payload);
                }
            }

            // Inchidem pipe urile de comunicare
            close(pc_fd);
            close(pr_fd);
        }
        
    }
    
    free(args);
    return NULL;
}

void *handle_client_req(void *arg)
{
    char path[256];

    // Verificam daca pipe ul de conexiuni exista daca nu il cream
    get_disp_pipe_path(path, CONNECTION_PIPE_NAME);
    create_pipe(path);

    // Deschidem pipe ul de conexiuni
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    while (1) {
        struct ConnectionRequestHeader req;

        if (read_full(fd, &req, sizeof(req)) == sizeof(req)) {
            ClientArgs *args = malloc(sizeof(ClientArgs));
            uint32_t len = be32toh(req.m_RpnLen);
            if(len > 255)
                len = 255;

            // Citim numele pipe ului de la client
            read_full(fd, args->connect_pipe_name, len);
            args->connect_pipe_name[len] = '\0';

            len = be32toh(req.m_ApLen);
            if(len > 255)
                len = 255;

            // Citim calea de acces
            read_full(fd, args->access_path, len);
            args->access_path[len] = '\0';

            // Daca serviciul este activ pornim un thread pt client
            if (g_service.is_active) {
                pthread_t t;
                pthread_create(&t, NULL, client_handler, args);
                pthread_detach(t);
            } else {
                free(args);
            }
        }
    }
    return NULL;
}

void *handle_service_req(void *arg)
{
    // Verificam daca pipe ul de instalare exista daca nu il cream
    char path[256];
    get_disp_pipe_path(path, INSTALL_PIPE_NAME);
    create_pipe(path);

    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return NULL;

    while (1) {
        struct InstallRequestHeader req;

        if (read_full(fd, &req, sizeof(req)) == sizeof(req)) {
            // Citim lungimea numelui pipe ului de instalare
            uint16_t len = be16toh(req.m_IpnLen);

            // Cream un buffer pt numele pipe ului de instalare
            char name[128];
            read_full(fd, name, len);
            name[len] = '\0';

            // Cream un buffer pt numele pipe ului de comunicare
            char comm_path[256];
            get_comm_pipe_path(comm_path, name);
            create_pipe(comm_path);

            // Deschidem pipe ul pt comunicare cu serviciul
            int ifd = open(comm_path, O_RDONLY);
            if (ifd >= 0) {
                // Citim headerul de instalare
                struct InstallHeader h;
                read_full(ifd, &h, sizeof(h));

                // Asiguram ca nu scrie altcnv service
                pthread_mutex_lock(&g_service_lock);

                // Citim versiunea serviciului
                g_service.version_len = h.m_VersionLen;
                read_full(ifd, g_service.version, h.m_VersionLen);

                // Citim pipe u de apel
                uint16_t clen = be16toh(h.m_CpnLen);
                read_full(ifd, g_service.call_pipe_name, clen);
                g_service.call_pipe_name[clen] = '\0';

                // Citim pipe ul de return
                uint16_t rlen = be16toh(h.m_RpnLen);
                read_full(ifd, g_service.return_pipe_name, rlen);
                g_service.return_pipe_name[rlen] = '\0';

                // Citim calea de acces
                uint16_t alen = be16toh(h.m_ApLen);
                read_full(ifd, g_service.access_path, alen);
                g_service.access_path[alen] = '\0';

                // Verificam daca pipe urile de comunicare exista daca nu le cream
                char scall[256], sret[256];
                get_comm_pipe_path(scall, g_service.call_pipe_name);
                get_comm_pipe_path(sret, g_service.return_pipe_name);
                create_pipe(scall);
                create_pipe(sret);

                // Deschidem pipe urile de comunicare
                g_service.call_fd = open(scall, O_RDWR);
                g_service.return_fd = open(sret, O_RDWR);
                g_service.is_active = 1;

                // Dam unlock la mutex pe service
                pthread_mutex_unlock(&g_service_lock);

                // Inchidem pipe ul de instalare pt serviciul curent
                close(ifd);
            }
        }
    }
    return NULL;
}

int main(void)
{
    // Cream directorul de pipe uri dispatcher
    mkdir(DISPATCHER_PIPE_DIR, 0777);

    // Cream directorul de pipe uri de comunicare
    mkdir(PIPES_DIR, 0777);

    // Cream thread urile pt gestionarea cererilor de instalare si conexiuni
    pthread_t st, ct;
    pthread_create(&st, NULL, handle_service_req, NULL);
    pthread_create(&ct, NULL, handle_client_req, NULL);

    // Asteptam ca thread urile sa se termine
    pthread_join(st, NULL);
    pthread_join(ct, NULL);

    return 0;
}