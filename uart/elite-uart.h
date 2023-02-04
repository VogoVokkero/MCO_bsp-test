#ifndef ELITE_UART
#define ELITE_UART
#pragma once

#include <termios.h>
#include <ctype.h>

#ifdef __arm__
#define UART_ELITE "/dev/ttymxc1" /* ELITE uart protocol */
#define UART_TRACE "/dev/ttymxc4" /* TRACEs */
#else
/* idealy we don't give a fork, whether 'dsp' or 'tst'
 * when using tty0tty on native PC:
 *  - tnt0 will be the emulated STM32 uart
 *  - tnt1 will be the looped emulated uart, for injection to tnt0
 *  - tnt2 will be the trace uart, emulates trace from STM32
 */
#define UART_ELITE "/dev/tnt0" /* ELITE uart protocol */
#define UART_TRACE "/dev/tnt2" /* TRACEs */
#endif

#define UART_FRAME_SOF_STR_SZ_BYTES 8U /*"DST<SRC"*/
#define UART_FRAME_RAW_STR_SZ_BYTES 4U /*"500"*/
#define UART_FRAME_PAYLOAD_STR_SZ_BYTES (512U - 7U - 3U)

enum elite_uart_ptcl_fields
{
	UART_PCTL_ELITE_FIELD_SOF = 0,
	UART_PCTL_ELITE_FIELD_RAW_SZ = 1,
	UART_PCTL_ELITE_FIELD_PAYLOAD = 2,
	UART_PCTL_ELITE_FIELD_INVALID = 3 // todo
};

/* Simplified UART protocol version for the sake of this test suite
 * we just need to know if this is a RAW frame, or an ASCII frame.
 */
typedef struct
{
	char sof[UART_FRAME_SOF_STR_SZ_BYTES];
	ssize_t sof_sz;

	char payload[UART_FRAME_PAYLOAD_STR_SZ_BYTES];
	ssize_t payload_sz;

	ssize_t raw_sz;

} elite_uart_frame_t;

extern int uart_elite_fd;

#endif /* ELITE_UART*/
