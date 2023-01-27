/*
 */

/*
 * See README
 */
#include "esg-bsp-test.h"
#include <termios.h>

DLT_DECLARE_CONTEXT(dlt_ctxt_udsp);

#define UART_STM "/dev/ttymxc1"

int uart_fd;

static void *elite_uart_dsp_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	ebt_settings_t *settings = (ebt_settings_t *)p_data;
	char line_buffer[4096];

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("START"));

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("elite_uart_dsp_runner: invalid settings"));
		ret = -EINVAL;
	}

	/* not counting loops, but kill by parent */
	while (EXIT_SUCCESS == ret)
	{
		char c;
		uint16_t cur_pos = 0;

		while (('\n' != c) && (512 > cur_pos))
		{
			ssize_t bytes = read(uart_fd, &c, sizeof(c));
			if (sizeof(char) == bytes)
			{
				line_buffer[cur_pos++] = c;
			}
		}

		DLT_LOG(dlt_ctxt_udsp, settings->verbosity, DLT_STRING("udsp_runner got :"), DLT_STRING(line_buffer));
	}

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("EXIT"), DLT_UINT32(ret));

	return (void *)ret;
}

int elite_uart_dsp_init(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = EXIT_SUCCESS;
	struct termios tty;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_udsp, "UDSP", "ESG BSP ELITE UART DSP Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("elite_uart_dsp_init: invalid settings"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_INFO, DLT_STRING("udsp_init"));

		uart_fd = open(UART_STM, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
		if (uart_fd < 0)
		{
			DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: open failed " UART_STM));
			ret = uart_fd;
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = tcgetattr(uart_fd, &tty);
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

		ret = tcsetattr(uart_fd, TCSANOW, &tty);

		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: tcgetattr failed"));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		char buff[200];
		while (read(uart_fd, &buff, 200) > 0)
		{
			// printf("flushing...\n");
		}

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
