// SPDX-License-Identifier: BSD-3-Clause

#include <internal/mm/mem_list.h>
#include <internal/types.h>
#include <internal/essentials.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

// Pentru setarea erorii "errno"
#include <errno.h>

void *malloc(size_t size)
{
	/* TODO: Implement malloc(). */
	if (size == 0) {
		return NULL;
	}

	// PROT == protection flags
	// MAP_PRIVATE e obligatoriu (SAU MAP_SHARED)
	void *memory = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	// Daca nu am memorie
	if (memory == MAP_FAILED) {
		return NULL;
	}

	// Adaug blocul de memorie creat in structura interna
	if (mem_list_add(memory, size) == 0) {
		return memory;
	} else {
		// Daca structura interna e corupta, trebuie sa eliberez si sa sterg blocul mapat
		munmap(memory, size);
		return NULL;
	}
}

void *calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement calloc(). */
	if (nmemb == 0 || size == 0) {
		return NULL;
	}

	// Rezerv memorie
	void *memory = mmap(NULL, nmemb * size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (memory == MAP_FAILED) {
		return NULL;
	}

	// Setez memoria rezervata la zero
	memset(memory, 0, nmemb * size);

	if (mem_list_add(memory, nmemb * size) == 0) {
		return memory;
	} else {
		munmap(memory, nmemb * size);
		return NULL;
	}
}

void free(void *ptr)
{
	/* TODO: Implement free(). */
	if (ptr == NULL) {
		return;
	}

	// Caut in lista nodul in care se afla evidenta ptr-ul
	// Am nevoie de lungime zonei de memorie
    struct mem_list *head = mem_list_find(ptr);
	if (head == NULL) {
		return;
	}

    size_t length = head->len;
	// Eliberez memoria pt ptr
    munmap(ptr, length);
	// Eliberez memoria pt nodul din lista asociat ptr-ului
    mem_list_del(ptr);
}

void *realloc(void *ptr, size_t size)
{
	/* TODO: Implement realloc(). */
	// Cazul I: size = 0
	if (size == 0) {
		return NULL;
	}

	// Cazul II: pointer == NULL si size > 0
	if (ptr == NULL) {
		return malloc(size);
	}

	// Blocul de memorie curent
	struct mem_list *head = mem_list_find(ptr);
	if (head == NULL) {
		return NULL;
	}

	// Cazul III: Micsorarea sau acelasi bloc de memorie
	if (size <= head->len) {
		head->len = size;
		return ptr;
	}

	// Cazul IV: Marirea blocului de memorie (trebuie mutat)
	void *new_ptr = malloc(size);
	// Copiez datele vechi in noul bloc
	memcpy(new_ptr, ptr, head->len);
	// Eliberez vechiul bloc
	free(ptr);
	return new_ptr;
}

void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
	/* TODO: Implement reallocarray(). */
	// Cazul I: size = 0
	if (nmemb == 0 || size == 0) {
		free(ptr);
		return NULL;
	}

	// Rezolva cazul de overflow intalnit la realloc
	// Cast explicit: fortez ca -1 sa fie considerat size_t, nu int
	if (size > 0 && nmemb > (((size_t) - 1) / size)) {
		return NULL;
	}

	// Cazul II: pointer == NULL si size > 0
	if (ptr == NULL) {
		return malloc(nmemb * size);
	}

	// Blocul de memorie curent
	struct mem_list *head = mem_list_find(ptr);
	if (head == NULL) {
		return NULL;
	}

	// Cazul III: Micsorarea sau acelasi bloc de memorie
	if (head->len <= size) {
		head->len = size;
		return ptr;
	}

	// Cazul IV: Marirea blocului de memorie (trebuie mutat)
	void *new_ptr = malloc(nmemb * size);
	// Copiez datele vechi in noul bloc
	memcpy(new_ptr, ptr, nmemb * size);
	// Eliberez vechiul bloc
	free(ptr);
	return new_ptr;
}
