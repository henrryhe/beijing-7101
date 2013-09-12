/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_recblock.c                                      */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX  blocking receive                               */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

#define BUFFER_SIZE 32

extern EMBXLB_Config_t loopback_config;
extern char *error_strings[];

EMBX_TRANSPORT tp = EMBX_INVALID_HANDLE_VALUE;

void send_thread(void *param)
{
EMBX_PORT port = (EMBX_PORT)param;
EMBX_ERROR res;
EMBX_VOID *buffer;

    EMBX_OS_Delay(5000);

    EMBX_Alloc(tp,BUFFER_SIZE,&buffer);

    res = EMBX_SendMessage(port,buffer,BUFFER_SIZE);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("port_thread failed, res = %s\n",error_strings[res]));
        return;
    }
}


int run_test(void)
{
EMBX_UINT    i;
EMBX_ERROR   res;
EMBX_TPINFO  tpinfo1;
EMBX_BOOL    bFailed;
EMBX_PORT    localPort,remotePort;
EMBX_VOID   *buffer1;
EMBX_FACTORY factory;

EMBX_RECEIVE_EVENT ev;

    bFailed = EMBX_FALSE;
    buffer1 = 0;

    /* Test 1 */
    res = EMBX_ReceiveBlock(EMBX_INVALID_HANDLE_VALUE, &ev);
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
    res = EMBX_ReceiveBlock(tp, &ev);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }
        
    /* Test 8 */
    res = EMBX_ReceiveBlock(remotePort, &ev);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_ReceiveBlock(localPort, 0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }


    /* Test 10 */
    res = EMBX_Alloc(tp, BUFFER_SIZE, &buffer1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    for(i=0;i<BUFFER_SIZE;i++)
    {
        ((unsigned char *)buffer1)[i] = (unsigned char)i;
    }

    /* Test 11 */
    res = EMBX_SendMessage(remotePort, buffer1, BUFFER_SIZE);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }


    /* Test 12 */
    res = EMBX_ReceiveBlock(localPort, &ev);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(ev.handle != EMBX_INVALID_HANDLE_VALUE ||
       ev.offset != 0 ||
       ev.type   != EMBX_REC_MESSAGE ||
       ev.size   != BUFFER_SIZE)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, event structure incorrect\n"));
        goto skip_remaining_tests;
    }

    for(i=0;i<ev.size;i++)
    {
        if( ((unsigned char *)ev.data)[i] != (unsigned char)i )
        {
            EMBX_Info(EMBX_TRUE, ("Test13 failed, buffer contents incorrect\n"));
            bFailed = EMBX_TRUE;
            break;
        }
    }

    /* Test 13 */
    res = EMBX_Free(ev.data);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(!EMBX_OS_ThreadCreate(send_thread, (void *)remotePort, EMBX_DEFAULT_THREAD_PRIORITY, "send"))
    {
        EMBX_Info(EMBX_TRUE, ("Unable to create thread\n"));
        goto skip_remaining_tests;
    }

    /* Test 14 */
    res = EMBX_ReceiveBlock(localPort, &ev);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(ev.handle != EMBX_INVALID_HANDLE_VALUE ||
       ev.offset != 0 ||
       ev.type   != EMBX_REC_MESSAGE ||
       ev.size   != BUFFER_SIZE)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, event structure incorrect\n"));
        goto skip_remaining_tests;
    }

    /* Test 15 */
    res = EMBX_Free(ev.data);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 16 */
    res = EMBX_ClosePort(remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 17 */
    res = EMBX_ClosePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 18 */
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 19 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* These test a different code path to the identical tests done before
     * EMBX_Init
     */

    /* Test 20 */
    res = EMBX_ReceiveBlock(localPort, &ev);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 21 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
