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

/* Frame fields positions */
typedef enum
{
	ELITE_UART_FIELD_SOF = 0U,
	ELITE_UART_FIELD_RAW_SZ = 1U,
	ELITE_UART_FIELD_PAYLOAD = 2U,
	ELITE_UART_FIELD_LAST = ELITE_UART_FIELD_PAYLOAD,

	ELITE_UART_FIELD_SEPARATOR = ':', // 58
} elite_uart_fields_t;


/* Possible ECU Ids (Sof)
 * if we keep an UART protocol, it will be close to json/mqtt topics
 * this set of short IDs would have been a good idea back then, but we
 * will most likely no longuer need this optiom with future Terminals/Gateways.
 */
typedef enum
{
	ELITE_UART_ECU_INVALID = 0,
	ELITE_UART_ECU_FROM = '<',

	ELITE_UART_ECU_R = 'R', /* radio */
	ELITE_UART_ECU_TST = 'R',

	ELITE_UART_ECU_C = 'C', /* configurator */
	ELITE_UART_ECU_CFG = 'C',

	ELITE_UART_ECU_A = 'D', /* audio */
	ELITE_UART_ECU_DSP = 'A',
	ELITE_UART_ECU_AVX = 'A',

	ELITE_UART_ECU_H = 'H', /* HMI */
	ELITE_UART_ECU_BCK = 'H',
	ELITE_UART_ECU_TBT = 'H',

	ELITE_UART_ECU_ALL = '*'
} elite_uart_ecu_ids_t;

/* unfortunately, the uart protocol uses long ASCII ids for uart routing */
#define ELITE_UART_ECU_STRING_SZ 3U
#define ELITE_UART_ECU_TST_STRING "TST"
#define ELITE_UART_ECU_CFG_STRING "CFG"
#define ELITE_UART_ECU_DSP_STRING "DSP"
#define ELITE_UART_ECU_BCK_STRING "BCK"
#define ELITE_UART_ECU_TBT_STRING "TBT"

#define ELITE_UART_FRAME_SOF_STR_SZ_BYTES (ELITE_UART_ECU_STRING_SZ*2+1) /*"DST<SRC"*/
#define ELITE_UART_FRAME_RAW_STR_SZ_BYTES 4U /*"500"*/
#define ELITE_UART_FRAME_PAYLOAD_STR_SZ_BYTES (512U - 7U - 3U)

/* Simplified UART protocol version for the sake of this test suite
 * we just need to know if this is a RAW frame, or an ASCII frame.
 */
typedef struct
{
	char sof[ELITE_UART_FRAME_SOF_STR_SZ_BYTES+1]; // for debug only
	ssize_t sof_pos;

	elite_uart_ecu_ids_t dst;
	elite_uart_ecu_ids_t src;

	char payload[ELITE_UART_FRAME_PAYLOAD_STR_SZ_BYTES+1];
	ssize_t payload_sz;

	ssize_t raw_sz;

} elite_uart_frame_t;

extern int uart_elite_fd;

#endif /* ELITE_UART*/
