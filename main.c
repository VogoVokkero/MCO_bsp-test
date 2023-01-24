/*
 * Copyright (c) 2023 VOGO S.A., All rights reserved
 */

#define DLT_CLIENT_MAIN_MODULE
#include "esg-bsp-test.h"


int main(int argc, char **argv)
{
	int err;

	dlt_client_init("BTST", "ESG BSP Test App", DLT_LOG_INFO);

	DLT_REGISTER_CONTEXT_LL_TS(dlt_ctxt_btst,"BTST","ESG BSP Test Context", DLT_LOG_INFO, DLT_TRACE_STATUS_DEFAULT);

	err = audio_init();
	
	dlt_client_exit();

	return err;
}
