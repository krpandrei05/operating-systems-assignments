// SPDX-License-Identifier: BSD-3-Clause

#include <sys/stat.h>
#include <internal/syscall.h>
#include <fcntl.h>
#include <errno.h>

int stat(const char *restrict path, struct stat *restrict buf)
{
	/* TODO: Implement stat(). */
	// Syscall-ul (x86-64, 64 biti=long) aferent stat-ului
	long ret = syscall(__NR_stat, path, buf);

	if (ret < 0) {
		errno = -ret;
		return -1;
	} else {
		return 0;
	}
}
