#ifndef __TIME_H__
#define __TIME_H__ 1

typedef long time_t;

// Time Specification
struct timespec {
    // Time Value Seconds => Secunde
    time_t tv_sec;
    // Time Value Nanoseconds => Nanosecunde
    long tv_nsec;
};

int nanosleep(const struct timespec *request, struct timespec *remaining);

#endif
