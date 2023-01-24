/*
 */

/*
 * See README
 */
#include "esg-bsp-test.h"

static void *elite_runner(void *p_data)
{
	int ret = EXIT_SUCCESS;
	uint32_t nb_loops = (uint32_t)p_data;

	int verbosity = (nb_loops < DLT_VERBOSITY_LOOP_THRESHOLD) ? DLT_LOG_VERBOSE : DLT_LOG_INFO ;

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("START elite_runner"), DLT_INT32(nb_loops));

	while (0 < nb_loops--)
	{
		int avail;

		DLT_LOG(dlt_ctxt_tdma, verbosity, DLT_STRING("elite_runner"), DLT_UINT32(nb_loops));
	}

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("EXIT elite_runner"));

	return (void *)ret;
}

int elite_init(pthread_t *runner, uint32_t nb_loops)
{
	int ret = EXIT_SUCCESS;

	DLT_LOG(dlt_ctxt_tdma, DLT_LOG_INFO, DLT_STRING("elite_init: Using "), DLT_STRING((SAMPLE_ACCESS == SND_PCM_ACCESS_RW_NONINTERLEAVED) ? "non-interleaved" : "interleaved"));

	ret = pthread_create(runner, NULL, elite_runner, nb_loops);

	if (EXIT_SUCCESS != ret)
	{
		DLT_LOG(dlt_ctxt_tdma, DLT_LOG_ERROR, DLT_STRING("elite_init: failed to creating running"));
	}

	return ret;
}
