/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"

#include "options/cmdline.h"

enum
{
	RUNNER_AUDIO = 0,	   // alsa
	RUNNER_ELITE_TDMA = 1, // Elite TDMA (spi)
	RUNNER_ELITE_UDSP = 1, // Elite TDMA (spi)
	RUNNER_INVALID = 2
};

pthread_t test_runner[RUNNER_INVALID] = { 0 };

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	struct gengetopt_args_info args_info;

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	/* let's call our cmdline parser */
	if (cmdline_parser(argc, argv, &args_info) != 0)
		exit(1);

	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("Using loops:"), DLT_UINT32(args_info.loops_arg));
	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("audio enabled:"), DLT_INT32(!args_info.no_audio_flag));
	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("tdma  enabled:"), DLT_INT32(!args_info.no_tdma_flag));
	DLT_LOG(dlt_ctxt_audio, DLT_LOG_INFO, DLT_STRING("uart  enabled:"), DLT_INT32(!args_info.no_uart_flag));

	if ((EXIT_SUCCESS == ret) && (0 == args_info.no_audio_flag))
	{
		ret = audio_init(&test_runner[RUNNER_AUDIO], args_info.loops_arg);
	}

	if ((EXIT_SUCCESS == ret) && (0 == args_info.no_tdma_flag))
	{
		ret = elite_tdma_init(&test_runner[RUNNER_ELITE_TDMA], args_info.loops_arg);
	}

	if ((EXIT_SUCCESS == ret) && (0 == args_info.no_uart_flag))
	{
		ret = elite_uart_dsp_init(&test_runner[RUNNER_ELITE_UDSP], args_info.loops_arg);
	}

	/* Wait for any valid runner to complete */
	if (NULL != test_runner[RUNNER_AUDIO])
	{
		pthread_join(test_runner[RUNNER_AUDIO], NULL);
	}

	if (NULL != test_runner[RUNNER_ELITE_TDMA])
	{
		pthread_join(test_runner[RUNNER_ELITE_TDMA], NULL);
	}

	if (NULL != test_runner[RUNNER_ELITE_UDSP])
	{
		pthread_join(test_runner[RUNNER_ELITE_UDSP], NULL);
	}

	dlt_client_exit();

	return ret;
}
