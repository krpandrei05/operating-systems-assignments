// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <errno.h>

int ftruncate(int fd, off_t length)
{
	/* TODO: Implement ftruncate(). */
	// Syscall-ul (x86-64, 64 biti=long) aferent ftruncate-ului
	long ret = syscall(__NR_ftruncate, fd, length);
	if (ret < 0) {
		errno = -ret;
		return -1;
	} else {
		return 0;
	}
}
