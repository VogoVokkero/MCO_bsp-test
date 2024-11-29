#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h> //4 uint...
#include <time.h>

#include "wi_time.h"

#define TIME_CLOCK CLOCK_MONOTONIC

// Set a timeout duration in ms
void time_setTimeout( struct timeval *a, uint32_t timeout )
{
    struct timeval b = {0};
    b.tv_sec = timeout / 1000;
    b.tv_usec = (timeout % 1000) * 1000;
    gettimeofday( a, NULL);
    timeradd(a, &b, a);
}

// See if a timeout is expired
int time_isTimeout( struct timeval *a)
{
    struct timeval b;
    gettimeofday(&b, NULL);
    return timercmp(&b, a, >);
}

long time_getClkockResolution_ns(void)
{
    struct timespec res;
    if(clock_getres(TIME_CLOCK, &res)<0)
    {
    }
    return (res.tv_sec*1000000000 + res.tv_nsec);
}

long time_getClock_us(void)
{
    struct timespec res;
    if(clock_gettime(TIME_CLOCK, &res)<0)
    {
    }
    return (res.tv_sec*1000000 + res.tv_nsec/1000);
}

long time_getElapse_us(long start_time)
{
    struct timespec res;
    if(clock_gettime(TIME_CLOCK, &res)<0)
    {
    }
    return ((res.tv_sec*1000000 + res.tv_nsec/1000) - start_time);
}