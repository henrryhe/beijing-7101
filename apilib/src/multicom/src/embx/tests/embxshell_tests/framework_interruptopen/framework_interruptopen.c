/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_connectblock.c                                  */
/*                                                                 */
/* Description:                                                    */
/*    Tests interrupting blocking transport initialization         */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

extern EMBXLB_Config_t blocking_loopback_config;
extern char *error_strings[];

void deinit_thread(void *param)
{
EMBX_TRANSPORT tp = (EMBX_TRANSPORT)param;
EMBX_ERROR res;

    EMBX_OS_Delay((unsigned long)param);

    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("deinit_thread(%ld) failed, res = %s\n", (unsigned long)param, error_strings[res]));
        return;
    }
}


int run_test(void)
{
EMBX_ERROR     res;
EMBX_TRANSPORT tp;
EMBX_FACTORY   factory;

    tp = EMBX_INVALID_HANDLE_VALUE;

    /* Test 1 */
    res = EMBX_RegisterTransport(EMBXLB_loopback_factory, &blocking_loopback_config, sizeof(EMBXLB_Config_t), &factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s, exiting\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 2 */
    res = EMBX_Init();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 3 */
    res = EMBX_OpenTransport("lo_blocking", &tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 4 */
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 5 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&blocking_loopback_config,sizeof(EMBXLB_Config_t),&factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s, exiting\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(!EMBX_OS_ThreadCreate(deinit_thread, (void *)2000, EMBX_DEFAULT_THREAD_PRIORITY, "deinit"))
    {
        EMBX_Info(EMBX_TRUE, ("Unable to create thread\n"));
        goto skip_remaining_tests;
    }

    /* Test 7 */
    res = EMBX_OpenTransport("lo_blocking", &tp);
    if(res != EMBX_TRANSPORT_CLOSED)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 8 */
    res = EMBX_Deinit();
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    return 0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
