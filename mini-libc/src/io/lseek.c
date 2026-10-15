// SPDX-License-Identifier: BSD-3-Clause

#include <unistd.h>
#include <internal/syscall.h>
#include <errno.h>

off_t lseek(int fd, off_t offset, int whence)
{
	/* TODO: Implement lseek(). */
	// Syscall-ul (x86-64, 64 biti=long) aferent lseek-ului
	long ret = syscall(__NR_lseek, fd, offset, whence);
	if (ret < 0) {
		errno = -ret;
		return -1;
	} else {
		return ret;
	}

	return -1;
}
