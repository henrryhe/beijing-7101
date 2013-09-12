/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: test_main.c                                               */
/*                                                                 */
/* Description:                                                    */
/*      Common main program for EMBX tests                         */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embxlb.h"

extern int run_test(void);

EMBXLB_Config_t loopback_config            = { "lo",0,0,0,EMBX_TRUE, EMBX_FALSE };
EMBXLB_Config_t invalid_loopback_config    = { "lo_invalid",5,5,(EMBX_HANDLE_MIN_TABLE_SIZE-1),EMBX_TRUE, EMBX_FALSE };
EMBXLB_Config_t restricted_loopback_config = { "lo_limited",5,5,EMBX_HANDLE_MIN_TABLE_SIZE,EMBX_TRUE, EMBX_FALSE };
EMBXLB_Config_t blocking_loopback_config   = { "lo_blocking",0,0,0,EMBX_TRUE, EMBX_TRUE };

char *error_strings[] = {
    "EMBX_SUCCESS",
    "EMBX_DRIVER_NOT_INITIALIZED",
    "EMBX_ALREADY_INITIALIZED",
    "EMBX_NOMEM",
    "EMBX_INVALID_ARGUMENT",
    "EMBX_INVALID_PORT",
    "EMBX_INVALID_STATUS",
    "EMBX_INVALID_TRANSPORT",
    "EMBX_TRANSPORT_INVALIDATED",
    "EMBX_TRANSPORT_CLOSED",
    "EMBX_PORTS_STILL_OPEN",
    "EMBX_PORT_INVALIDATED",
    "EMBX_PORT_CLOSED",
    "EMBX_PORT_NOT_BIND",
    "EMBX_ALREADY_BIND",
    "EMBX_CONNECTION_REFUSED",
    "EMBX_SYSTEM_INTERRUPT",
    "EMBX_SYSTEM_ERROR"
};

int main(int argc, char **argv)
{

#if defined(__OS21__)
    kernel_initialize(NULL);
    kernel_start();

    kernel_timeslice(OS21_TRUE);
#endif /* __OS21__ */

    return run_test();
}
