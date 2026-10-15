// SPDX-License-Identifier: BSD-3-Clause

#include <sys/stat.h>
#include <internal/syscall.h>
#include <errno.h>

int fstat(int fd, struct stat *st)
{
	/* TODO: Implement fstat(). */
	// Syscall-ul (x86-64, 64 biti=long) aferent fstat-ului
	long ret = syscall(__NR_fstat, fd, st);
	if (ret < 0) {
		errno = -ret;
		return -1;
	} else {
		return 0;
	}
}
