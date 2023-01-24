/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"

enum
{
	RUNNER_AUDIO = 0, // alsa
	RUNNER_ELITE = 1, // Elite TDMA (spi)
	RUNNER_INVALID = 2
};

pthread_t test_runner[RUNNER_INVALID];

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	uint32_t nb_loops = 1000U;

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_audio,"BTST","ESG BSP Audio Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);
	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_tdma,"ELIT","ESG BSP ELITE TDMA Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	if (2 <= argc)
	{
		nb_loops = strtoul (argv[1], NULL, 0);
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("Using loops="), DLT_UINT32(nb_loops));
	}

	ret = audio_init(&test_runner[RUNNER_AUDIO], nb_loops);

	if (EXIT_SUCCESS == ret)
	{
		elite_init(&test_runner[RUNNER_ELITE], nb_loops);
	}

	if (EXIT_SUCCESS == ret)
	{
		pthread_join(test_runner[RUNNER_AUDIO], NULL);
		pthread_join(test_runner[RUNNER_ELITE], NULL);
	}
	
	dlt_client_exit();

	return ret;
}
