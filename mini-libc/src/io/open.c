// SPDX-License-Identifier: BSD-3-Clause

#include <fcntl.h>
#include <internal/syscall.h>
#include <stdarg.h>
#include <errno.h>

int open(const char *filename, int flags, ...)
{
	/* TODO: Implement open system call. */
	// Argumentul ramas (daca exista) trebuie pus intr-o variabila
	va_list arg;
	va_start(arg, flags);

	// Folosit (adica != 0) doar daca flags nu contine O_CREAT
	// Flagul O_CREAT == Flag care indica sa creeze un fisier daca nu exista deja
	// Un fisier nou <=> Permisiuni
	mode_t permissions = 0;
	if ((flags & O_CREAT) != 0) {
		permissions = va_arg(arg, mode_t);
	}

	va_end(arg);
	// Syscall-ul (x86-64, 64 biti=long) aferent open-ului
	long fd = syscall(__NR_open, filename, flags, permissions);

	if (fd < 0) {
		errno = -fd;
		return -1;
	} else {
		return fd;
	}
}
