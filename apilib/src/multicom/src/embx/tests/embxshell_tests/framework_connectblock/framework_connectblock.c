/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_connectblock.c                                  */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX blocking connections                            */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

extern EMBXLB_Config_t loopback_config;
extern char *error_strings[];

EMBX_PORT gTestPort;

static void port_thread(void *param)
{
EMBX_ERROR     res;
EMBX_TRANSPORT tp = (EMBX_TRANSPORT)param;

    EMBX_OS_Delay(5000);

    res = EMBX_CreatePort(tp,"testport",&gTestPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("port_thread failed, res = %s\n",error_strings[res]));
    }
}



static void close_thread(void *param)
{
EMBX_ERROR     res;
EMBX_TRANSPORT tp = (EMBX_TRANSPORT)param;

    EMBX_OS_Delay(5000);

    res = EMBX_CloseTransport(tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("close_thread failed, res = %s\n",error_strings[res]));
    }
}



int run_test(void)
{
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1;
EMBX_PORT      localPort,remotePort;
EMBX_PORT      dummyPort;
EMBX_FACTORY   factory;
EMBX_BOOL      bFailed;
EMBX_TRANSPORT tp;


    bFailed = EMBX_FALSE;
    tp      = EMBX_INVALID_HANDLE_VALUE;

    /* Test 1 */
    res = EMBX_ConnectBlock(tp,"dummy",&dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&loopback_config,sizeof(EMBXLB_Config_t),&factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration failed, res = %s, exiting\n",error_strings[res]));
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
    res = EMBX_GetFirstTransport(&tpinfo1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 4 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 5 */
    res = EMBX_CreatePort(tp,"testport",&localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_ConnectBlock(tp,0,&dummyPort);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 7 */
    res = EMBX_ConnectBlock(tp,"dummy",0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }
    
    /* Test 8 */
    res = EMBX_ConnectBlock(EMBX_INVALID_HANDLE_VALUE,"dummy",&dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_ConnectBlock(tp,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",&dummyPort);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 10 */
    res = EMBX_ConnectBlock(tp,"testport",&remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 11 */
    res = EMBX_ClosePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 12 */
    res = EMBX_ClosePort(remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(!EMBX_OS_ThreadCreate(port_thread, (void *)tp, EMBX_DEFAULT_THREAD_PRIORITY, "port"))
    {
        EMBX_Info(EMBX_TRUE, ("Unable to create port thread\n"));
        goto skip_remaining_tests;
    }

    /* Test 13 */
    res = EMBX_ConnectBlock(tp,"testport",&remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 14 */
    res = EMBX_ClosePort(remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 15 */
    res = EMBX_ClosePort(gTestPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(!EMBX_OS_ThreadCreate(close_thread, (void *)tp, EMBX_DEFAULT_THREAD_PRIORITY, "close"))
    {
        EMBX_Info(EMBX_TRUE, ("Unable to create port thread\n"));
        goto skip_remaining_tests;
    }

    /* Test 16 */
    res = EMBX_ConnectBlock(tp,"testport",&remotePort);
    if(res != EMBX_TRANSPORT_CLOSED)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 17 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* These test a different code path to the identical tests done before
     * EMBX_Init
     */

    /* Test 18 */
    res = EMBX_ConnectBlock(tp,"dummy",&dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 19 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
