/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_buffer.c                                        */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX buffer management                               */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"

#include "embxlb.h"

extern EMBXLB_Config_t restricted_loopback_config;
extern char *error_strings[];

#define BUFFER_SIZE 32

int run_test(void)
{
EMBX_ERROR     res;
EMBX_BOOL      bFailed;
EMBX_TPINFO    tpinfo1;
EMBX_TRANSPORT tp1;
EMBX_VOID     *buffer1,*buffer2;
EMBX_UINT      bufsize;
EMBX_FACTORY   factory;

    bFailed = EMBX_FALSE;

    /* Test 1 */
    res = EMBX_Alloc(EMBX_INVALID_HANDLE_VALUE,BUFFER_SIZE,&buffer1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 2 */
    res = EMBX_Free(buffer1);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 3 */
    res = EMBX_GetBufferSize(buffer1,&bufsize);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&restricted_loopback_config,sizeof(EMBXLB_Config_t),&factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration failed, res = %s, exiting\n",error_strings[res]));
        return -1;
    }

    /* Test 4 */
    res = EMBX_Init();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 5 */
    res = EMBX_GetFirstTransport(&tpinfo1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 7 */
    res = EMBX_Alloc(EMBX_INVALID_HANDLE_VALUE,BUFFER_SIZE,&buffer1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 8 */
    res = EMBX_Alloc(tp1,BUFFER_SIZE,0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_Alloc(tp1,BUFFER_SIZE,&buffer1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 10 */
    res = EMBX_Alloc(tp1,BUFFER_SIZE,&buffer2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 11 */
    res = EMBX_GetBufferSize(0,&bufsize);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 12 */
    res = EMBX_GetBufferSize(buffer1,0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 13 */
    res = EMBX_GetBufferSize(buffer1,&bufsize);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    if(bufsize != BUFFER_SIZE)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, bufsize = %u expected %u\n",bufsize,BUFFER_SIZE));
        bFailed = EMBX_TRUE;
    }

    /* Test 14 */
    res = EMBX_Free(0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 15 */
    res = EMBX_Free(buffer1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        EMBX_Info(EMBX_TRUE, ("Skipping test 16\n"));
        bFailed = EMBX_TRUE;
    }
    else
    {
        /* Test 16 */
        res = EMBX_Free(buffer1);
        if(res != EMBX_INVALID_ARGUMENT)
        {
            EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
            bFailed = EMBX_TRUE;
        }

        /* Test 17 */
        res = EMBX_GetBufferSize(buffer1,&bufsize);
        if(res != EMBX_INVALID_ARGUMENT)
        {
            EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
            bFailed = EMBX_TRUE;
        }

    }

    /* Test 18 */
    res = EMBX_CloseTransport(tp1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 19 */
    res = EMBX_Alloc(tp1,BUFFER_SIZE,&buffer1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 20 */
    res = EMBX_GetBufferSize(buffer2,&bufsize);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 21 */
    res = EMBX_Free(buffer2);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 22 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test22 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* These test a different code path to the identical tests done before
     * EMBX_Init
     */

    /* Test 23 */
    res = EMBX_Alloc(EMBX_INVALID_HANDLE_VALUE,BUFFER_SIZE,&buffer1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test23 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 24 */
    res = EMBX_Free(buffer1);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test24 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 25 */
    res = EMBX_GetBufferSize(buffer1,&bufsize);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test25 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 26 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test26 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}   

