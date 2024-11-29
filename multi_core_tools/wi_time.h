/*
 ============================================================================
 Name        : time.h
 Author      : tmu
 Version     :
 Copyright   : Closed
 Description : time use tools
 ============================================================================
 */
#ifndef __TIME_H__
#define __TIME_H__

#include <sys/time.h>
#include <stdint.h>

void time_setTimeout( struct timeval *a, uint32_t timeout );
int time_isTimeout( struct timeval *a);
long time_getClkockResolution_ns(void);
long time_getClock_us(void);
long time_getElapse_us(long start_time);

#endif //__TIME_H__