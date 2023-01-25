/*
 */

/*
 * See README
 */
#include "esg-bsp-test.h"

DLT_DECLARE_CONTEXT(dlt_ctxt_udsp);

static void *elite_uart_dsp_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	uint32_t nb_loops = (uint32_t)p_data;

	int verbosity = (nb_loops < DLT_VERBOSITY_LOOP_THRESHOLD) ? DLT_LOG_VERBOSE : DLT_LOG_INFO ;

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("START"));

	while (0 < nb_loops--)
	{
		int avail;

		DLT_LOG(dlt_ctxt_udsp, verbosity, DLT_STRING("udsp_runner"), DLT_UINT32(nb_loops));
	}

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("EXIT"));

	return (void *)ret;
}

int elite_uart_dsp_init(pthread_t *runner, uint32_t nb_loops)
{
	int ret = EXIT_SUCCESS;

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_udsp,"UDSP","ESG BSP ELITE UART DSP Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	DLT_LOG(dlt_ctxt_udsp, DLT_LOG_INFO, DLT_STRING("udsp_init: Using "), DLT_STRING((SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved"));

	ret = pthread_create(runner, NULL, elite_uart_dsp_runner, nb_loops);

	if (EXIT_SUCCESS != ret)
	{
		DLT_LOG(dlt_ctxt_udsp, DLT_LOG_ERROR, DLT_STRING("udsp_init: failed to creating running"));
	}

	/**/

	return ret;
}
