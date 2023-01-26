/*
 */

/*
 * See README
 */
#include "esg-bsp-test.h"
#include "elite-slave-ready-gpio.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_tdma);

elite_gpio_t slave_ready_gpio = {0};

static void *elite_tdma_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	uint32_t nb_loops = (uint32_t)p_data;

	int verbosity = (nb_loops < DLT_VERBOSITY_LOOP_THRESHOLD) ? DLT_LOG_VERBOSE : DLT_LOG_INFO ;

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("START"));

	while (0 < nb_loops--)
	{
		int avail;
		DLT_LOG(dlt_ctxt_tdma, verbosity, DLT_STRING("elite_tdma_runner"), DLT_UINT32(nb_loops));

		/*wait for slave-ready GPIO to be asserted */



	}

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("EXIT"));

	return (void *)ret;
}

int elite_tdma_init(pthread_t *runner, uint32_t nb_loops)
{
	int ret = EXIT_SUCCESS;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_tdma,"ELIT","ESG BSP ELITE TDMA Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_INFO, DLT_STRING("elite_tdma_init: Using "), DLT_STRING((SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved"));

	/* Initialize dependencies for this, e.g. the slave-ready GPIO */
	ret = elite_slave_ready_gpio(&slave_ready_gpio);

	if (EXIT_SUCCESS == ret)
	{
		ret = pthread_create(runner, NULL, elite_tdma_runner, nb_loops);
		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("elite_tdma_init: failed to creating running"));
		}
	}

	return ret;
}
