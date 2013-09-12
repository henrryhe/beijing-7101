/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_connect.c                                       */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX non blocking connections                        */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

extern EMBXLB_Config_t loopback_config;
extern char *error_strings[];

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
    res = EMBX_Connect(tp,"dummy",&dummyPort);
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
    res = EMBX_Connect(tp,0,&dummyPort);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 7 */
    res = EMBX_Connect(tp,"dummy",0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 8 */
    res = EMBX_Connect(tp,"dummy",&dummyPort);
    if(res != EMBX_PORT_NOT_BIND)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }
    
    /* Test 9 */
    res = EMBX_Connect(EMBX_INVALID_HANDLE_VALUE,"dummy",&dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 10 */
    res = EMBX_Connect(tp,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",&dummyPort);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 11 */
    res = EMBX_Connect(tp,"testport",&remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 12 */
    res = EMBX_Connect(tp,"testport",&dummyPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 13 */
    res = EMBX_ClosePort(dummyPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
        
    /* Test 14 */
    res = EMBX_ClosePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 15 */
    res = EMBX_Connect(tp,"testport",&dummyPort);
    if(res != EMBX_PORT_NOT_BIND)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 16 */
    res = EMBX_CreatePort(tp,"testport",&localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
 
    /* Test 17 */
    res = EMBX_InvalidatePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 18 - regression test for INSbl19113, connect after invalidate succeeded */
    res = EMBX_Connect(tp,"testport",&dummyPort);
    if(res != EMBX_PORT_NOT_BIND)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 19 */
    res = EMBX_ClosePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 20 */
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_PORTS_STILL_OPEN)
    {
        EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 21 */
    res = EMBX_ClosePort(remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 22 */
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test22 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 23 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test23 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* These test a different code path to the identical tests done before
     * EMBX_Init
     */

    /* Test 24 */
    res = EMBX_Connect(tp,"dummy",&dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test24 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 25 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test25 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
