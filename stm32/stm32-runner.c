#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "dlt-client.h"
#include "esg-spidev.h"
#include "esg-bsp-test.h"

#include <sys/time.h>	  //4 time
#include <sys/types.h>	  //4 signals
#include <signal.h>		  //4 signals
#include <sys/signalfd.h> //4 signalfd

#include "Common/commParam.h"
#include "Common/protocoleDSP.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_stm32);

#define SPI_BUFF_NB_TX 2

static protdspSpiFrame_t SpiTxFrame = {0};
static protdspSpiFrame_t SpiRxFrame = {0};

#define TSK_TIME_LOOP 1000
#define STM_SPIDEV "/dev/spidev3.0"

static spi_dev_t spi_dev = {0};

static void *stm32_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	struct timeval t_in, timeout = {0};
	ebt_settings_t *settings = (ebt_settings_t *)p_data;
	fd_set read_fds, write_fds, except_fds;
	int max_fd;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("START failed, ebt_settings_t null"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		uint32_t nb_loops = settings->nb_loops;
		ssize_t byte_rx = 0;

		DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("START"), DLT_UINT32(nb_loops));

		while ((nb_loops--) && (0 <= byte_rx))
		{
			// TODO : block on sready gpio somehow.
			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_ZERO(&except_fds);
			// Avec le descripteur de fichier de la file de messages
			FD_SET(spi_dev.fd, &except_fds);
			FD_SET(spi_dev.fd, &read_fds);
			FD_SET(spi_dev.fd, &write_fds);

			max_fd = spi_dev.fd + 1;

			timeout.tv_sec = 0;
			timeout.tv_usec = 100;

			int ret_select = select(max_fd + 1, &read_fds, &write_fds, &except_fds, &timeout);
			if (ret_select == -1)
			{
				fprintf(stderr, "stm32_runner: timeout!\n");
				fflush(stderr);
				DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("timeout"), DLT_UINT(nb_loops));
			}
			else if (ret_select)
			{
				if (FD_ISSET(spi_dev.fd, &read_fds))
				{
					DLT_LOG(dlt_ctxt_stm32, DLT_LOG_INFO, DLT_STRING("spi_transfer"), DLT_UINT(nb_loops));

					// vanilla SPI_IOC_MESSAGE
					byte_rx = spi_transfer(&spi_dev,
										   (const uint8_t *)&SpiTxFrame,
										   (const uint8_t *)&SpiRxFrame,
										   sizeof(protdspSpiFrame_t));
				}
			}
		};
	}

	DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("EXIT"), DLT_UINT32(ret));

	spi_close(&spi_dev);

	return (void *)ret;
}

int stm32_runner_init(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = EXIT_SUCCESS;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("stm32_runner_init: invalid settings"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_stm32, "ELIT", "ESG BSP STM32 Context", settings->verbosity, DLT_TRACE_STATUS_DEFAULT);

		ret = spi_init(&spi_dev, STM_SPIDEV, SPI_STM_SPEED, SPI_MODE_3);
		if (0 > ret)
		{
			DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("Failed to initialize SPI device"), DLT_STRING(STM_SPIDEV));
		}
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = pthread_create(runner, NULL, stm32_runner, (void *)settings);
		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("stm32_runner_init: failed to creating running"));
		}
	}

	return ret;
}
