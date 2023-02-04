/*
 * See README
 * This module shall:
 * - listen to ELITE-uart protocol frames
 * - scan byte byte until RAW or Line frames can be discriminated
 */
#include "esg-bsp-test.h"
#include "elite-uart.h"
#include "dlt/dlt_user.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_udsp);
DLT_DECLARE_CONTEXT(dlt_ctxt_term); // terminal traces, through trace uart, TODO

int uart_elite_fd;

static void *elite_uart_dsp_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	ebt_settings_t *settings = (ebt_settings_t *)p_data;

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("START"));

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("elite_uart_dsp_runner: invalid settings"));
		ret = -EINVAL;
	}

	/* not counting loops, but kill by parent */
	while (EXIT_SUCCESS == ret)
	{
		char c = 0;

		elite_uart_frame_t frame = {0};
		ssize_t max_size = UART_FRAME_PAYLOAD_STR_SZ_BYTES;

		uint32_t pctl_field = UART_PCTL_ELITE_FIELD_SOF;

		while (('\n' != c) && ('\r' != c) && (0 < max_size--))
		{
			ssize_t bytes = read(uart_elite_fd, &c, sizeof(c));
			uint8_t update_payload_sz = 1U;

			if (sizeof(char) == bytes)
			{
				if (':' == c)
				{
					pctl_field++;
				}
				else if ((0 == isalnum(c)) && (0 == ispunct(c)) && (0 == frame.raw_sz))
				{
					/* we hit a \n, \t, \r, \0, while this is not a RAW frame, this is not expected, skip this char*/
					DLT_LOG(dlt_ctxt_udsp, DLT_LOG_FATAL, DLT_STRING("skipping non alnum char in non-RAW mode"), DLT_HEX8(c));
				}
				else
				{
					/* decode per field */
					switch (pctl_field)
					{
					case UART_PCTL_ELITE_FIELD_SOF:
						frame.sof[frame.sof_sz] = c;
						frame.sof_sz++;
						break;

					case UART_PCTL_ELITE_FIELD_RAW_SZ:
						if (0 != isdigit(c))
						{
							frame.raw_sz = frame.raw_sz * 10 + (c - '0');
						}
						else
						{
							DLT_LOG(dlt_ctxt_udsp, DLT_LOG_FATAL, DLT_STRING("invalid char in RAW size field"), DLT_HEX8(c));
						}
						break;

					case UART_PCTL_ELITE_FIELD_PAYLOAD:

						frame.payload[frame.payload_sz] = c;
						frame.payload_sz++;

						if ((1U == frame.payload_sz) && (0 < frame.raw_sz))
						{
							/** we just left RAW field;, update expected payload size */
							max_size = frame.raw_sz - 1U;
						}
						break;

					default:
						break;
					};
				}
			}
			else
			{
				DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("failed reading char"), DLT_INT32(bytes), DLT_UINT8(c));
			}
		}

		frame.payload[frame.payload_sz] = '\0';
		frame.sof[frame.sof_sz] = '\0';

		if (0 < (frame.sof_sz + frame.payload_sz))
		{
			if (0 < frame.raw_sz)
			{
				if (dlt_user_is_logLevel_enabled(&dlt_ctxt_udsp, DLT_LOG_INFO) == DLT_RETURN_TRUE)
				{
					DltContextData log_local;
					if (0 < dlt_user_log_write_start(&dlt_ctxt_udsp, &log_local, DLT_LOG_INFO))
					{
						u_int16_t len = 0;
						(void)dlt_user_log_write_string(&log_local, "udsp_runner got RAW frame");

						while (frame.raw_sz > len)
						{
							(void)dlt_user_log_write_uint8_formatted(&log_local, frame.payload[len++], DLT_FORMAT_HEX8);
						};
					}
					(void)dlt_user_log_write_finish(&log_local);
				}
			}
			else
			{
				DLT_LOG(dlt_ctxt_udsp, DLT_LOG_INFO, DLT_STRING("udsp_runner got frame"), DLT_STRING(frame.sof), DLT_STRING(frame.payload)); // todo
			}
		}
	}

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("EXIT"), DLT_UINT32(ret));

	return (void *)ret;
}

int elite_uart_dsp_init(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = EXIT_SUCCESS;
	struct termios tty;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("elite_uart_dsp_init: invalid settings"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_udsp, "UDSP", "ESG BSP ELITE UART DSP/TST Context", settings->verbosity, DLT_TRACE_STATUS_DEFAULT);
		DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_term, "TERM", "ESG BSP ELITE UART Traces Context", settings->verbosity, DLT_TRACE_STATUS_DEFAULT);

		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_INFO, DLT_STRING("udsp_init"));

		uart_elite_fd = open(UART_ELITE, O_RDWR | O_NOCTTY | O_SYNC /*| O_NONBLOCK*/);
		if (uart_elite_fd < 0)
		{
			DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: open failed " UART_ELITE));
			ret = uart_elite_fd;
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = tcgetattr(uart_elite_fd, &tty);
		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: tcgetattr failed"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		cfsetospeed(&tty, (speed_t)B115200);
		cfsetispeed(&tty, (speed_t)B115200);

		tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
		tty.c_cflag &= ~CSIZE;
		tty.c_cflag |= CS8;		 /* 8-bit characters */
		tty.c_cflag &= ~PARENB;	 /* no parity bit */
		tty.c_cflag &= ~CSTOPB;	 /* only need 1 stop bit */
		tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

		/* setup for non-canonical mode */
		tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
		tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		tty.c_oflag &= ~OPOST;

		/* fetch bytes as they become available */
		tty.c_cc[VMIN] = 1;
		tty.c_cc[VTIME] = 1;

		ret = tcsetattr(uart_elite_fd, TCSANOW, &tty);

		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: tcgetattr failed"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = pthread_create(runner, NULL, elite_uart_dsp_runner, (void *)settings);

		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: failed to creating runner"));
		}
	}

	if (EXIT_SUCCESS != ret)
	{
		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init failed"));
	}

	return ret;
}
