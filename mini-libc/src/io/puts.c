#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int puts(const char* str) {
    size_t length = strlen(str);

    /* (Documentatie) Otherwise, it shall return EOF, shall set an error indicator for the stream, 
    and errno shall be set to indicate the error.*/
    ssize_t ret1 = write(1, str, length);
    if (ret1 < 0) {
        errno = -ret1;
        // EOF == -1
        return -1;
    }

    ssize_t ret2 = write(1, "\n", 1);
    if (ret2 < 0) {
        errno = -ret2;
        // EOF == -1
        return -1;
    }

    // (Documentatie) Upon successful completion, puts() shall return a non-negative number.
    return EXIT_SUCCESS;
}
