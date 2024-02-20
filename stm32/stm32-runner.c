/*
 */

/*
 * See README
 */
#include "esg-bsp-test.h"
#include "stm32.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_stm32);

static void *stm32_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	ebt_settings_t *settings = (ebt_settings_t *)p_data;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("START failed, ebt_settings_t null"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		uint32_t nb_loops = settings->nb_loops;

		DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("START"), DLT_UINT32(nb_loops));

		sleep(3); // TODO

	}

	DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("EXIT"), DLT_UINT32(ret));

	stm32_fini();

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

		/* Initialize dependencies for this, e.g. the slave-ready GPIO */
		ret = stm32_init();
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = pthread_create(runner, NULL, stm32_runner, (void*)settings);
		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_stm32, DLT_LOG_ERROR, DLT_STRING("stm32_runner_init: failed to creating running"));
		}
	}

	return ret;
}
