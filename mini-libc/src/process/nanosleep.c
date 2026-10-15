#include <internal/syscall.h>
#include <errno.h>
#include <time.h>

int nanosleep(const struct timespec *request, struct timespec *remaining) {
    long ret = syscall(__NR_nanosleep, request, remaining);

    if (ret < 0) {
        errno = -ret;
        return -1;
    } else {
        return 0;
    }
}
