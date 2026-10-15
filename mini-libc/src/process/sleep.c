#include <unistd.h>
#include <errno.h>
#include <time.h>

unsigned int sleep(unsigned int seconds)
{
    struct timespec request, remaining;
    request.tv_sec = seconds;
    request.tv_nsec = 0;

    // Executa efectiv "dormitul"
    if (nanosleep(&request, &remaining) < 0) {
        // EINTR = Error Interrupted => Perturbat de un semnal
        if (errno == EINTR) {
            // Cat mai trebuia sa doarma
            return remaining.tv_sec;
        }
    }

    return 0;
}
