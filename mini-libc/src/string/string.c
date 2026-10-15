// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>

char *strcpy(char *destination, const char *source)
{
	/* TODO: Implement strcpy(). */
	char *aux = destination;

	while (*source != '\0') {
		*destination = *source;
		destination++;
		source++;
	}

	*destination = '\0';
	destination = aux;

	return destination;
}

char *strncpy(char *destination, const char *source, size_t len)
{
	/* TODO: Implement strncpy(). */
	char *aux = destination;

	while (*source != '\0' && len > 0) {
		*destination = *source;
		destination++;
		source++;
		len--;
	}

	while (len > 0) {
		*destination = '\0';
		len--;
		destination++;
	}

	destination = aux;

	return destination;
}

char *strcat(char *destination, const char *source)
{
	/* TODO: Implement strcat(). */
	char *aux = destination;

	while (*destination != '\0') {
		destination++;
	}

	while (*source != '\0') {
		*destination = *source;
		destination++;
		source++;
	}

	*destination = '\0';
	destination = aux;

	return destination;
}

char *strncat(char *destination, const char *source, size_t len)
{
	/* TODO: Implement strncat(). */
	char *aux = destination;

	while (*destination != '\0') {
		destination++;
	}

	while (*source != '\0' && len > 0) {
		*destination = *source;
		destination++;
		source++;
		len--;
	}

	*destination = '\0';
	destination = aux;

	return destination;
}

int strcmp(const char *str1, const char *str2)
{
	/* TODO: Implement strcmp(). */
	while (*str1 != '\0' && *str2 != '\0') {
		if (*str1 < *str2) {
			return -1;
		} else if (*str1 > *str2) {
			return 1;
		}
		str1++;
		str2++;
	}

	if (*str1 == '\0' && *str2 == '\0') {
		return 0;
	} else if (*str2 == '\0') {
		return 1;
	}

	return -1;
}

int strncmp(const char *str1, const char *str2, size_t len)
{
	/* TODO: Implement strncmp(). */
	while (*str1 != '\0' && *str2 != '\0' && len > 0) {
		if (*str1 < *str2) {
			return -1;
		} else if (*str1 > *str2) {
			return 1;
		}
		str1++;
		str2++;
		len--;
	}

	if (len == 0) {
		return 0;
	} else if (*str1 > *str2) {
		return 1;
	}

	return -1;
}

size_t strlen(const char *str)
{
	size_t i = 0;

	for (; *str != '\0'; str++, i++)
		;

	return i;
}

char *strchr(const char *str, int c)
{
	/* TODO: Implement strchr(). */
	while (*str != '\0') {
		if (*str == (char)c) {
			return (char *)str;
		}
		str++;
	}

	// Daca c == '\0', voi return pointer-ul catre terminatorul de sir a lui str
	if ((char)c == '\0') {
		return (char *)str;
	}

	return NULL;
}

char *strrchr(const char *str, int c)
{
	/* TODO: Implement strrchr(). */
	char *start = (char *)str;
	while (*str != '\0') {
		str++;
	}

	// Sunt la terminatorul de sir al lui str
	while (start != str) {
		if (*str == (char)c) {
			return (char *)str;
		}
		str--;
	}

	// start == str
	if (*str == (char)c) {
		return (char *)str;
	}

	return NULL;
}

char *strstr(const char *haystack, const char *needle)
{
	/* TODO: Implement strstr(). */
	while (*haystack != '\0') {
		char *aux1 = (char *)haystack;
		char *aux2 = (char *)needle;
		while (*needle != '\0') {
			if (*haystack == *needle) {
				needle++;
				haystack++;
			} else {
				break;
			}
		}
		if (*needle == '\0') {
			return aux1;
		} else {
			haystack = aux1;
			needle = aux2;
		}
		haystack++;
	}

	return NULL;
}

char *strrstr(const char *haystack, const char *needle)
{
	/* TODO: Implement strrstr(). */
	size_t len_needle = strlen(needle);

	char *start = (char *)haystack;
	while (*haystack != '\0') {
		haystack++;
	}

	// Am ajuns la terminatorul de sir a lui "haystack"
	while (start != haystack) {
		char *aux1 = (char *)haystack;
		char *temp = (char *)aux1;
		aux1 -= len_needle + 1;
		temp -= len_needle + 1;

		char *aux2 = (char *)needle;
		while (*needle != '\0' && *needle == *aux1) {
			aux1++;
			needle++;
		}

		if (*needle == '\0') {
			return temp;
		}

		haystack--;
	}

  	return NULL;
}

void *memcpy(void *destination, const void *source, size_t num)
{
	/* TODO: Implement memcpy(). */
	// unsigned char (8 biti = 1 byte => 0 - 255 > 0)
	unsigned char *d = (unsigned char *)destination;
	unsigned char *s = (unsigned char *)source;

	while (num > 0) {
		*d = *s;
		d++;
		s++;
		num--;
	}

	return destination;
}

void *memmove(void *destination, const void *source, size_t num)
{
	/* TODO: Implement memmove(). */
	unsigned char *d = destination;
    const unsigned char *s = source;

	// Cazul I: d si s incep in acelasi loc in memorie
    if (d == s || num == 0)
        return destination;

	// Cazul II: d se afla mai in spate in memeorie fata de s
	/* Poate sa existe o suprapunere, dar nu este problema daca s este corupt,
	atata timp cat d iese corect */
    if (d < s) {
        while (num > 0) {
            *d = *s;
			d++;
			s++;
			num--;
        }
	// Cazul II: d se afla mai in fata in memorie fata de s
	/* In acest caz, copierea trebuie facuta de la sfarsit la inceput pentru 
	a ma asigura ca d este cel corect*/
    } else {
        s += num - 1;
        d += num - 1;
        while (num > 0) {
            *d = *s;
			d--;
			s--;
			num--;
        }
    }
	return destination;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
	/* TODO: Implement memcmp(). */
	const unsigned char *p1 = ptr1;
	const unsigned char *p2 = ptr2;

	while (num > 0) {
		if (*p1 < *p2) {
			return -1;
		}
		if (*p1 > *p2) {
			return 1;
		}
		p1++;
		p2++;
		num--;
	}

	return 0;
}

void *memset(void *source, int value, size_t num)
{
	/* TODO: Implement memset(). */
	unsigned char *s = source;

	// Umplu blocul de memorie cu valoarea value
	while (num > 0) {
		*s = (unsigned char)value;
		s++;
		num--;
	}
	return source;
}
