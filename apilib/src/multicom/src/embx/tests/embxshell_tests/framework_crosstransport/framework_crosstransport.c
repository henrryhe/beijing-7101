/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_crosstransport.c                                */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX error paths when trying to use message          */
/*      buffers in the wrong transport                             */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

#define BUFFER_SIZE 32

extern EMBXLB_Config_t loopback_config;
extern EMBXLB_Config_t restricted_loopback_config;
extern char *error_strings[];

int run_test(void)
{
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1;
EMBX_TRANSPORT tp1,tp2;
EMBX_PORT      localPort,remotePort;
EMBX_VOID     *buffer1;
EMBX_FACTORY   factory1,factory2;

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&loopback_config,sizeof(EMBXLB_Config_t),&factory1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration 1 failed, res = %s, exiting\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&restricted_loopback_config,sizeof(EMBXLB_Config_t),&factory2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration 2 failed, res = %s, exiting\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    buffer1 = 0;

    /* Test 1 */
    res = EMBX_Init();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 2 */
    res = EMBX_GetFirstTransport(&tpinfo1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 3 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 4 */
    res = EMBX_GetNextTransport(&tpinfo1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 5 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_CreatePort(tp1,"testport",&localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 7 */
    res = EMBX_Connect(tp1,"testport",&remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
        
    /* Test 8 */
    res = EMBX_Alloc(tp2,BUFFER_SIZE,&buffer1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 9 */
    res = EMBX_SendMessage(remotePort,buffer1,BUFFER_SIZE);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 10 */
    res = EMBX_CloseTransport(tp2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 11 */
    /* This tests a slightly different failure mode than test 9
     * as the buffer is now orphaned due to its associated transport
     * being closed.
     */
    res = EMBX_SendMessage(remotePort,buffer1,BUFFER_SIZE);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }


    /* Test 12 */
    res = EMBX_ClosePort(localPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
 
    /* Test 13 */
    res = EMBX_ClosePort(remotePort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 14 */
    res = EMBX_CloseTransport(tp1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 15 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 16 */
    res = EMBX_UnregisterTransport(factory1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 17 */
    res = EMBX_UnregisterTransport(factory2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    return 0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
