/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"

enum
{
	RUNNER_AUDIO = 0, // alsa
	RUNNER_ELITE_TDMA = 1, // Elite TDMA (spi)
	RUNNER_ELITE_UDSP = 1, // Elite TDMA (spi)
	RUNNER_INVALID = 2
};

pthread_t test_runner[RUNNER_INVALID];

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	uint32_t nb_loops = 1000U;

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	if (2 <= argc)
	{
		nb_loops = strtoul (argv[1], NULL, 0);
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("Using loops="), DLT_UINT32(nb_loops));
	}

	ret = audio_init(&test_runner[RUNNER_AUDIO], nb_loops);

	if (EXIT_SUCCESS == ret)
	{
		elite_tdma_init(&test_runner[RUNNER_ELITE_TDMA], nb_loops);
	}

	if (EXIT_SUCCESS == ret)
	{
		elite_uart_dsp_init(&test_runner[RUNNER_ELITE_UDSP], nb_loops);
	}

	if (EXIT_SUCCESS == ret)
	{
		pthread_join(test_runner[RUNNER_AUDIO], NULL);
		pthread_join(test_runner[RUNNER_ELITE_TDMA], NULL);
		pthread_join(test_runner[RUNNER_ELITE_UDSP], NULL);
	}
	
	dlt_client_exit();

	return ret;
}
