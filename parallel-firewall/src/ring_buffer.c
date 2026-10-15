// SPDX-License-Identifier: BSD-3-Clause

#include "ring_buffer.h"

// malloc, free
#include <stdlib.h>

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	/* TODO: implement ring_buffer_init */
	(void) ring;
	(void) cap;

	if (ring == NULL || cap == 0)
		return -1;

	// Aloc spatiu pe heap pentru buffer
	ring->data = malloc(cap);
	ring->cap = cap;
	ring->len = 0;
	// Prod va scrie de la inceputul buffer-ului
	ring->write_pos = 0;
	// Cons va scrie de la inceputul buffer-ului
	ring->read_pos = 0;

	// Buffer-ul este activ, deci prod lucreaza.
	ring->stopped = 0;

	// Initializez mutex-ul si conditiile
	pthread_mutex_init(&ring->rb_mutex, NULL);
	pthread_cond_init(&ring->cond_cons_not_empty, NULL);
	pthread_cond_init(&ring->cond_prod_not_full, NULL);

	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: implement ring_buffer_enqueue */
	(void) ring;
	(void) data;
	(void) size;

	// Niciun alt thread nu poate modifica buffer-ul
	pthread_mutex_lock(&ring->rb_mutex);

	// Daca nu am loc de inserare, astept
	while (ring->len + size > ring->cap)
		// Buffer-ul e plin => Producatorul elibereaza mutex-ul si asteapta
		// => Va fi trezit de semnalul unui consumator care elibereaza buffer-ul
		pthread_cond_wait(&ring->cond_prod_not_full, &ring->rb_mutex);

	// Copiez continutul data in buffer la offset-ul write_pos
	memcpy(ring->data + ring->write_pos, data, size);

	// Actualizez lungimea buffer-ului
	ring->len += size;
	// Actulizez offset-ul write_pos la finalul zonei data scrise anterior
	ring->write_pos = (ring->write_pos + size) % ring->cap;

	// Semnalez consumerii ca in buffer a mai fost adaugat un pachet (in cazul in care era gol buffer-ul)
	pthread_cond_signal(&ring->cond_cons_not_empty);
	// Deblochez mutex-ul pentru a permite altor thread-uri sa acceseze buffer-ul
	pthread_mutex_unlock(&ring->rb_mutex);

	// Returnez dimnesiunea pachetului data inserat in buffer
	return size;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: Implement ring_buffer_dequeue */
	(void) ring;
	(void) data;
	(void) size;

	// Niciun alt thread nu poate modifica buffer-ul
	pthread_mutex_lock(&ring->rb_mutex);

	// Daca dimensiunea datelor pe care vreau sa le consum este mai mare decat
	//dimensiunea buffer-ului si producer-ul nu a terminat de adaugat pachete,
	//=> consumerii asteapta.
	while (ring->len < size && ring->stopped == 0)
		pthread_cond_wait(&ring->cond_cons_not_empty, &ring->rb_mutex);

	// Daca dupa trezire, buffer-ul este gol sau nu are sufiecti bytes pentru un pachet valid
	//si flag-ul stopped (1) indica ca producer-ul nu mai are de adaugat pachete
	//=> consumer-ul isi incheie executia
	if (ring->len < size && ring->stopped == 1) {
		pthread_mutex_unlock(&ring->rb_mutex);
		return -1;
	}

	// Copiaza pachetul din buffer (ring->data) in spatiul consumerului (data)
	memcpy(data, ring->data + ring->read_pos, size);

	// Actualizez offset-ul de citire
	ring->read_pos = (ring->read_pos + size) % ring->cap;
	// Actualizez lungimea buffer-ului
	ring->len -= size;

	// Semnalez producer-ul ca s-a elibarat din spatiu si poate incearca sa insereze din nou
	pthread_cond_signal(&ring->cond_prod_not_full);
	// Deblochez mutex-ul
	pthread_mutex_unlock(&ring->rb_mutex);

	// Returnez dimensiunae pachetului extras
	return size;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	/* TODO: Implement ring_buffer_destroy */
	(void) ring;

	// Eliberez resursele pentru mutex si conditiile producer-ului si a consumer-iilor
	pthread_mutex_destroy(&ring->rb_mutex);
	pthread_cond_destroy(&ring->cond_prod_not_full);
	pthread_cond_destroy(&ring->cond_cons_not_empty);
	// Eliberez buffer-ul
	free(ring->data);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	/* TODO: Implement ring_buffer_stop */
	(void) ring;

	// Blochez modificare buffer-ului
	pthread_mutex_lock(&ring->rb_mutex);

	// Nu vor mai veni pachete
	ring->stopped = 1;

	// Semnalez toti consumerii sa se trezeasca sa vada ca nu vor mai veni pachete
	pthread_cond_broadcast(&ring->cond_cons_not_empty);
	// Deblochez mutex-ul
	pthread_mutex_unlock(&ring->rb_mutex);
}
