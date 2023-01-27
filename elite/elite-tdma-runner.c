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
	ebt_settings_t *settings = (ebt_settings_t *)p_data;

	if ((NULL == settings) || (0 >= slave_ready_gpio.fd))
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("START failed, ebt_settings_t null, or gpio fd not valid"), DLT_UINT32(slave_ready_gpio.fd));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		uint32_t nb_loops = settings->nb_loops;

		DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("START"), DLT_UINT32(nb_loops));

		while ((0 < nb_loops--) && (EXIT_SUCCESS == ret))
		{
			DLT_LOG(dlt_ctxt_tdma, DLT_LOG_DEBUG, DLT_STRING("elite_tdma_runner"), DLT_UINT32(nb_loops));

			/*wait for slave-ready GPIO to be asserted */
			elite_slave_ready_wait(&slave_ready_gpio);
		}
	}

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("EXIT"), DLT_UINT32(ret));

	return (void *)ret;
}

int elite_tdma_init(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = EXIT_SUCCESS;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_tdma, "ELIT", "ESG BSP ELITE TDMA Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("elite_tdma_init: invalide settings"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		DLT_LOG(dlt_ctxt_tdma, DLT_LOG_INFO, DLT_STRING("elite_tdma_init: Using "), DLT_STRING((SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved"));

		/* Initialize dependencies for this, e.g. the slave-ready GPIO */
		ret = elite_slave_ready_gpio(&slave_ready_gpio);
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = pthread_create(runner, NULL, elite_tdma_runner, (void*)settings);
		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("elite_tdma_init: failed to creating running"));
		}
	}

	return ret;
}
