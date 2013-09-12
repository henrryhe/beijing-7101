/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_sendobject.c                                    */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX single transport object send and update         */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

#define BUFFER_SIZE 32

extern EMBXLB_Config_t loopback_config;
extern char *error_strings[];
EMBX_CHAR object[BUFFER_SIZE];

int run_test(void)
{
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1;
EMBX_BOOL      bFailed;
EMBX_TRANSPORT tp;
EMBX_PORT      localPort,remotePort;
EMBX_HANDLE    hObj;
EMBX_FACTORY   factory;

    bFailed = EMBX_FALSE;
    tp      = EMBX_INVALID_HANDLE_VALUE;
    hObj    = EMBX_INVALID_HANDLE_VALUE;
         
    /* Test 1 */
    res = EMBX_SendObject(EMBX_INVALID_HANDLE_VALUE, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 2 */
    res = EMBX_UpdateObject(EMBX_INVALID_HANDLE_VALUE, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&loopback_config,sizeof(EMBXLB_Config_t),&factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration failed, res = %s, exiting\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 3 */
    res = EMBX_Init();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 4 */
    res = EMBX_GetFirstTransport(&tpinfo1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 5 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_CreatePort(tp, "testport", &localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 7 */
    res = EMBX_Connect(tp, "testport", &remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 8 */
    res = EMBX_SendObject(tp, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_SendObject(localPort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 10 */
    res = EMBX_SendObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 11 */
    res = EMBX_UpdateObject(tp, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 12 */
    res = EMBX_UpdateObject(localPort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 13 */
    res = EMBX_UpdateObject(remotePort, EMBX_INVALID_HANDLE_VALUE, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 14 */
    res = EMBX_RegisterObject(tp, object, BUFFER_SIZE, &hObj);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 15 */
    res = EMBX_SendObject(remotePort, hObj, 0, BUFFER_SIZE+1);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 16 */
    res = EMBX_UpdateObject(remotePort, hObj, 0, BUFFER_SIZE+1);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 17 */
    res = EMBX_SendObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 18 */
    res = EMBX_UpdateObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 19 */
    res = EMBX_ClosePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 20 */
    res = EMBX_SendObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 21 */
    res = EMBX_UpdateObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 22 */
    res = EMBX_DeregisterObject(tp, hObj);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test22 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 23 */
    res = EMBX_ClosePort(remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test23 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 24 */
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test24 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 25 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test25 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* These test a different code path to the identical tests done before
     * EMBX_Init
     */

    /* Test 26 */
    res = EMBX_SendObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test26 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 27 */
    res = EMBX_UpdateObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test27 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 28 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test28 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
