// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
// Pentru snprintf()
#include <stdio.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

// Implementare min heap

// Swap intre valorile de log_entry ale heap-ului
void swap_entries(log_entry *entry1, log_entry *entry2)
{
	log_entry aux = *entry1;
	*entry1 = *entry2;
	*entry2 = aux;
}

// Initializarea heap-ului
min_heap *init_min_heap(size_t cap)
{
	min_heap *h;

	h = malloc(sizeof(min_heap));
	h->v = malloc(cap * sizeof(log_entry));
	h->cap = cap;
	h->size = 0;

	return h;
}

// Heapify-ul (reordonarea) heap-ului in functie de id pentru a avea timestamp-ul in ordine crescatoare
void heapify_min_heap(min_heap *h, size_t idx)
{
	size_t min = idx;
	size_t l = 2 * idx + 1;
	size_t r = 2 * idx + 2;

	if (l < h->size && h->v[l].id < h->v[min].id)
		min = l;

	if (r < h->size && h->v[r].id < h->v[min].id)
		min = r;

	if (min != idx) {
		swap_entries(&h->v[idx], &h->v[min]);
		heapify_min_heap(h, min);
	}
}

// Inserez un nou log_entry in heap
void insert_min_heap(min_heap *h, log_entry *entry)
{
	if (h->size == h->cap) {
		h->cap *= 2;
		h->v = realloc(h->v, h->cap * sizeof(log_entry));
	}

	h->v[h->size] = *entry;
	size_t i = h->size;

	h->size++;

	while (i > 0) {
		size_t parent = (i - 1) / 2;

		if (h->v[parent].id > h->v[i].id) {
			swap_entries(&h->v[parent], &h->v[i]);
			i = parent;
		} else {
			break;
		}
	}
}

// Extrag din heap un log_entry cu cel mai mic timestamp si il returnez
log_entry extract_min_heap(min_heap *h)
{
	log_entry out = h->v[0];

	h->v[0] = h->v[h->size - 1];
	h->size--;

	heapify_min_heap(h, 0);

	return out;
}

void consumer_thread(so_consumer_ctx_t *ctx)
{
	/* TODO: implement consumer thread */
	(void) ctx;

	// pachetul primit
	so_packet_t pkt;
	// log_entry-ul cu datele din pachet
	log_entry curr_entry;
	// log entry-ul cu datele din heap (pentru afisare)
	log_entry out_entry;
	// Action -> 4 caractere, Hash -> max ulong = 20 caractere
	// Timestamp -> max ulong = 20 caractere, 2 spatii, 1 newline, 1 '\0'
	// 44 caractere -> 64(2^8)
	char buffer[64];
	// Pentru afisare
	int len;
	// Rezultatul dequeue-ului
	ssize_t rez_dequeue;

	while (1) {
		// Extrag pachetul si incrementez id-ul global
		// Blochez mutex-ul pentru a nu corupe alt thread id-ul si ordinea din rb
		pthread_mutex_lock(&ctx->id_mutex);
		rez_dequeue = ring_buffer_dequeue(ctx->producer_rb, &pkt, PKT_SZ);
		if (rez_dequeue != -1)
			curr_entry.id = ctx->global_id_cnt++;
		pthread_mutex_unlock(&ctx->id_mutex);

		// Daca nu mai are pachete de dat producer-ul => break
		if (rez_dequeue == -1)
			break;

		// Extrag decizia, hash-ul si timestamp-ul din pachetul din rb
		curr_entry.action = process_packet(&pkt);
		curr_entry.hash = packet_hash(&pkt);
		curr_entry.timestamp = pkt.hdr.timestamp;

		// Inserez in heap log_entry-ul curent
		// Blochez mutex-ul pentru ca heap-ul sa nu fie corupt
		pthread_mutex_lock(&ctx->heap_mutex);
		insert_min_heap(ctx->h, &curr_entry);

		// Daca id-ul asteptat este acelasi cum id-ul celui mai mic log_entry curent, il afisez
		// While-ul se poate executa mai mult de o data, deoarece pot ramane log_entry-uri in heap care nu au fost afisate
		while (ctx->h->size > 0 && ctx->h->v[0].id == ctx->expected_id) {
			out_entry = extract_min_heap(ctx->h);
			pthread_mutex_unlock(&ctx->heap_mutex);

			len = snprintf(buffer, 64, "%s %016lx %lu\n", RES_TO_STR(out_entry.action), out_entry.hash, out_entry.timestamp);
			write(ctx->log_fd, buffer, len);

			// Dau lock pentru a ma asigura ca alt thread nu ii strica valoarea
			pthread_mutex_lock(&ctx->heap_mutex);
			ctx->expected_id++;
		}

		pthread_mutex_unlock(&ctx->heap_mutex);
	}
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	(void) tids;
	(void) num_consumers;
	(void) rb;
	(void) out_filename;

	// Aloc ctx-ul parinte
	so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

	// Aloc heap-ul, cu o capacitate modica
	ctx->h = init_min_heap(1000);
	ctx->producer_rb = rb;
	ctx->global_id_cnt = 0;
	ctx->expected_id = 0;

	// Initializez mutex-urile
	pthread_mutex_init(&ctx->heap_mutex, NULL);
	pthread_mutex_init(&ctx->id_mutex, NULL);

	// Deschid fisierul de output
	ctx->log_fd = open(out_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	for (int i = 0; i < num_consumers; i++) {
		/*
		 * TODO: Launch consumer threads
		 **/
		pthread_create(&tids[i], NULL, (void *)consumer_thread, (void *)ctx);
	}

	return num_consumers;
}
