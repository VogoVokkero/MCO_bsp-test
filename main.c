/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"

enum
{
	RUNNER_AUDIO = 0,
	RUNNER_SPI = 1,
	RUNNER_INVALID = 2
};

pthread_t test_runner[RUNNER_INVALID];

uint32_t nb_loops = 100U;

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_btst,"BTST","ESG BSP Test Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	ret = audio_init(&test_runner[RUNNER_AUDIO], nb_loops);

	if (EXIT_SUCCESS == ret)
	{
		pthread_join(test_runner[RUNNER_AUDIO], NULL);
	}
	
	dlt_client_exit();

	return ret;
}
