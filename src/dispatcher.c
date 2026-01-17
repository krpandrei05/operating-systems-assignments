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

int main(void)
{
	// TODO: Implement dispatcher functionality

	return 0;
}