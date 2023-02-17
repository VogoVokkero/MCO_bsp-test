/*
 */

/*
 * See README
 */
#include "esg-bsp-test.h"
#include "rackAuvitran.h"

static void *rack_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	ebt_settings_t *settings = (ebt_settings_t *)p_data;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_audio, DLT_LOG_ERROR, DLT_STRING("START failed, ebt_settings_t null"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		uint32_t nb_loops = settings->nb_loops;

		DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("START"), DLT_UINT32(nb_loops));

		while ((0 < nb_loops--) && (EXIT_SUCCESS == ret))
		{
			DLT_LOG(dlt_ctxt_rack, DLT_LOG_DEBUG, DLT_STRING("rack_runner"), DLT_UINT32(nb_loops));

		}
	}

	DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("EXIT"), DLT_UINT32(ret));

	rack_release();

	return (void *)ret;
}

int rack_init(pthread_t *runner, ebt_settings_t *settings)
{
	int ret = EXIT_SUCCESS;

	if (NULL == settings)
	{
		DLT_LOG(dlt_ctxt_btst, DLT_LOG_ERROR, DLT_STRING("rack_init: invalid settings"));
		ret = -EINVAL;
	}

	if (EXIT_SUCCESS == ret)
	{
		DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_rack, "RACK", "AUVITRAN Rack Context", settings->verbosity, DLT_TRACE_STATUS_DEFAULT);

		ret = rack_initialize();
	}

	if (EXIT_SUCCESS == ret)
	{
		ret = pthread_create(runner, NULL, rack_runner, (void*)settings);
		if (EXIT_SUCCESS != ret)
		{
			DLT_LOG(dlt_ctxt_rack, DLT_LOG_ERROR, DLT_STRING("rack_init: failed to creating running"));
		}
	}

	return ret;
}
