/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"
#include "elite-slave-ready-gpio.h"
#include "rackAuvitran.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_btst)

#include "options/cmdline.h"

enum
{
	RUNNER_AUDIO = 0,	   // alsa
	RUNNER_ELITE_GPIOD = 1, 
	RUNNER_ELITE_UDSP = 2, 
	RUNNER_RACK = 3,	   // auvitran rack
	RUNNER_STM32 = 4,
	//
	RUNNER_INVALID = 5
};

pthread_t test_runner[RUNNER_INVALID] = {0};

ebt_settings_t g_settings =
	{
		.nb_loops = 1000U,
		.verbosity = DLT_LOG_INFO,
		.pauses = 0U,
		.rack_freq = 0U,
		.sched_rt = 0U
	};

int main(int argc, char **argv)
{
	int ret = EXIT_SUCCESS;
	struct gengetopt_args_info args_info;

	/* let's call our cmdline parser */
	if ((cmdline_parser(argc, argv, &args_info) != 0) || (1 == argc))
	{
		cmdline_parser_print_help();
		exit(1);
	}

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	if (0 != args_info.verbose_given)
	{
		g_settings.verbosity = DLT_LOG_VERBOSE;
	}
	else
	{
		g_settings.verbosity = (args_info.loops_arg < DLT_VERBOSITY_LOOP_THRESHOLD) ? DLT_LOG_VERBOSE : DLT_LOG_INFO;
	}

	g_settings.nb_loops = args_info.loops_arg;
	g_settings.pauses = args_info.pauses_arg;
	g_settings.rack_freq = args_info.rack_arg;
	g_settings.sched_rt = args_info.sched_rt_arg;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_btst, "BTST", "BSP Test suite", g_settings.verbosity, DLT_TRACE_STATUS_DEFAULT);

	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("audio enabled:"), DLT_INT32(args_info.audio_flag));

	if (0 != args_info.audio_flag)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("audio : loops:"), DLT_UINT32(g_settings.nb_loops));
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("audio : pauses:"), DLT_INT32(args_info.pauses_arg));
	}

	if (0 != args_info.rack_given)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("rack read freq: "), DLT_INT32(g_settings.rack_freq));
	}

	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("stm32 enabled:"), DLT_INT32(args_info.stm32_flag));
	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("uart  enabled:"), DLT_INT32(args_info.uart_flag));
	DLT_LOG(dlt_ctxt_btst, DLT_LOG_INFO, DLT_STRING("gpio poll test only:"), DLT_INT32(args_info.gpio_test_only_flag));

	/* quick ctr+c test for the slave-ready GPIO */
	if ((EXIT_SUCCESS == ret) && (1 == args_info.gpio_test_only_flag))
	{
		elite_gpio_t blah = {0};

		elite_slave_ready_gpio_init(&blah, &g_settings);

		while (1)
		{
			elite_slave_ready_wait(&blah);
		}
	}

	/* check if auvitran interface test is wanted, set the default gains and audio matrix */
	if ((EXIT_SUCCESS == ret) && (0 != args_info.rack_given))
	{
		ret = rack_runner_init(&test_runner[RUNNER_RACK], (void *)&g_settings);
	}

	if ((EXIT_SUCCESS == ret) && (0 != args_info.audio_flag))
	{
		ret = audio_runner_init_poll(&test_runner[RUNNER_AUDIO], (void *)&g_settings);
	}

	if ((EXIT_SUCCESS == ret) && (0 != args_info.gpiod_flag))
	{
		ret = elite_gpiod_init(&test_runner[RUNNER_ELITE_GPIOD], (void *)&g_settings);
	}

	if ((EXIT_SUCCESS == ret) && (0 != args_info.uart_flag))
	{
		ret = elite_uart_dsp_runner_init(&test_runner[RUNNER_ELITE_UDSP], (void *)&g_settings);
	}

	if ((EXIT_SUCCESS == ret) && (0 != args_info.stm32_flag))
	{
		ret = stm32_runner_init(&test_runner[RUNNER_STM32], (void *)&g_settings);
	}


	/* Wait for any valid runner to complete */
	if (0 != test_runner[RUNNER_AUDIO])
	{
		pthread_join(test_runner[RUNNER_AUDIO], NULL);
	}

	if (0 != test_runner[RUNNER_ELITE_GPIOD])
	{
		pthread_join(test_runner[RUNNER_ELITE_GPIOD], NULL);
	}

	if (0 != test_runner[RUNNER_RACK])
	{
		pthread_join(test_runner[RUNNER_RACK], NULL);
	}

	if (0 != test_runner[RUNNER_STM32])
	{
		pthread_join(test_runner[RUNNER_STM32], NULL);
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
