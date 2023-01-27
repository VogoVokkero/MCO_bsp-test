/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"
#include "elite/elite-slave-ready-gpio.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_btst)

#include "options/cmdline.h"

enum
{
	RUNNER_AUDIO = 0,	   // alsa
	RUNNER_ELITE_TDMA = 1, // Elite TDMA (spi)
	RUNNER_ELITE_UDSP = 1, // Elite TDMA (spi)
	RUNNER_INVALID = 2
};

pthread_t test_runner[RUNNER_INVALID] = {0};

ebt_settings_t g_settings = { 1000U, DLT_LOG_INFO };

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	struct gengetopt_args_info args_info;

	/* let's call our cmdline parser */
	if (cmdline_parser(argc, argv, &args_info) != 0)
		exit(1);

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	g_settings.verbosity = (args_info.loops_arg < DLT_VERBOSITY_LOOP_THRESHOLD) ? DLT_LOG_VERBOSE : DLT_LOG_INFO;
	g_settings.nb_loops = args_info.loops_arg;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_btst, "BTST", "BSP Test suite", g_settings.verbosity, DLT_TRACE_STATUS_DEFAULT);

	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("Using loops:"), DLT_UINT32(g_settings.nb_loops));
	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("audio enabled:"), DLT_INT32(!args_info.no_audio_flag));
	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("tdma  enabled:"), DLT_INT32(!args_info.no_tdma_flag));
	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("uart  enabled:"), DLT_INT32(!args_info.no_uart_flag));
	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("gpio poll test only:"), DLT_INT32(args_info.gpio_test_only_flag));

	/* quick ctr+c test for the slave-ready GPIO */
	if ((EXIT_SUCCESS == ret) && (1 == args_info.gpio_test_only_flag))
	{
		elite_gpio_t blah = { 0 };

		elite_slave_ready_gpio(&blah);

		while (1)
		{
			elite_slave_ready_wait(&blah);
		}
	}

	if ((EXIT_SUCCESS == ret) && (0 == args_info.no_audio_flag))
	{
		ret = audio_init(&test_runner[RUNNER_AUDIO], (void*)&g_settings);
	}

	if ((EXIT_SUCCESS == ret) && (0 == args_info.no_tdma_flag))
	{
		ret = elite_tdma_init(&test_runner[RUNNER_ELITE_TDMA], (void*)&g_settings);
	}

	if ((EXIT_SUCCESS == ret) && (0 == args_info.no_uart_flag))
	{
		ret = elite_uart_dsp_init(&test_runner[RUNNER_ELITE_UDSP], (void*)&g_settings);
	}

	/* Wait for any valid runner to complete */
	if (0 != test_runner[RUNNER_AUDIO])
	{
		pthread_join(test_runner[RUNNER_AUDIO], NULL);
	}

	if (0 != test_runner[RUNNER_ELITE_TDMA])
	{
		pthread_join(test_runner[RUNNER_ELITE_TDMA], NULL);
	}

/* we don't join the uard runner, we just kill it when comm
 * is no longuer needed
 
	if (0 != test_runner[RUNNER_ELITE_UDSP])
	{
		pthread_join(test_runner[RUNNER_ELITE_UDSP], NULL);
	}
*/

	dlt_client_exit();

	return ret;
}
