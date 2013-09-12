/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_recobject.c                                     */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX single transport non blocking                   */
/*      object receive                                             */
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
EMBX_UINT      i;
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1;
EMBX_BOOL      bFailed;
EMBX_TRANSPORT tp;
EMBX_PORT      localPort,remotePort;
EMBX_HANDLE    hObj;
EMBX_FACTORY   factory;

EMBX_RECEIVE_EVENT ev;

    bFailed = EMBX_FALSE;
    tp      = EMBX_INVALID_HANDLE_VALUE;
    hObj    = EMBX_INVALID_HANDLE_VALUE;

    /* Test 1 */
    res = EMBX_Receive(EMBX_INVALID_HANDLE_VALUE, &ev);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory, &loopback_config, sizeof(EMBXLB_Config_t), &factory);
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
    res = EMBX_CreatePort(tp, "testport", &localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 6 */
    res = EMBX_Connect(tp, "testport", &remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 7 */
    res = EMBX_RegisterObject(tp, object, BUFFER_SIZE, &hObj);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    for(i=0;i<BUFFER_SIZE;i++)
    {
        ((unsigned char *)object)[i] = (unsigned char)i;
    }

    /* Test 8 */
    res = EMBX_SendObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_Receive(localPort, &ev);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(ev.handle != hObj || /* This is a specific quirk of the loopback transport */
       ev.offset != 0 ||
       ev.type   != EMBX_REC_OBJECT ||
       ev.size   != BUFFER_SIZE)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, event structure incorrect\n"));
        goto skip_remaining_tests;
    }

    for(i=0;i<ev.size;i++)
    {
        if( ((unsigned char *)ev.data)[i] != (unsigned char)i )
        {
            EMBX_Info(EMBX_TRUE, ("Test9 failed, buffer contents incorrect\n"));
            bFailed = EMBX_TRUE;
            break;
        }
    }

    /* Test 10 */
    res = EMBX_UpdateObject(remotePort, hObj, 0, BUFFER_SIZE);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 11 */
    res = EMBX_Receive(localPort, &ev);
    if(res != EMBX_INVALID_STATUS)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 12 */
    res = EMBX_DeregisterObject(tp, hObj);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 13 */
    res = EMBX_ClosePort(remotePort);
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
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 16 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 17 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
