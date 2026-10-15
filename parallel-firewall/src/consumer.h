/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include "ring_buffer.h"
#include "packet.h"

// Sturctura pentru un entry in log_file
typedef struct {
	so_action_t action;
	unsigned long hash;
	unsigned long timestamp;
	unsigned long id;
} log_entry;

// Structura pentru min_heap -> gestioneaza ordinea log_entry-urilor
typedef struct {
	log_entry *v;
	size_t size;
	size_t cap;
} min_heap;

typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;

    /* TODO: add synchronization primitives for timestamp ordering */

	// Heap-ul log_entry-urilor
	min_heap *h;
	// File Descriptor-ul fisierului output
	int log_fd;
	// Contor pentru id-ul global al pachetului (al catelea pachet din rb este)
	unsigned long global_id_cnt;
	// Contor pentru urmatorul pachet(id) care trebuie scris in log_file
	unsigned long expected_id;
	// Mutex pentru a ma asigura ca id-ul global nu este modificat de alt thread
	pthread_mutex_t id_mutex;
	// Mutex pentru a ma asigura ca elementele heap-ului nu sunt "calcate" in picioare
	pthread_mutex_t heap_mutex;
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
					int num_consumers,
					so_ring_buffer_t *rb,
					const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
