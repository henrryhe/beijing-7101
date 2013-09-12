/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_object.c                                        */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX object registration and query                   */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"

#include "embxlb.h"

extern EMBXLB_Config_t loopback_config;
extern EMBXLB_Config_t restricted_loopback_config;
extern char *error_strings[];

#define OBJECT_SIZE 32

EMBX_CHAR *object[OBJECT_SIZE];

int run_test(void)
{
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1;
EMBX_TRANSPORT tp1;
EMBX_HANDLE    hObj1,hObj2;
EMBX_VOID     *objptr;
EMBX_UINT      objsize;
EMBX_FACTORY   factory1,factory2;
EMBX_BOOL      bFailed;

    bFailed = EMBX_FALSE;

    /* Test 1 */
    res = EMBX_RegisterObject(EMBX_INVALID_HANDLE_VALUE,0,0,&hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 2 */
    res = EMBX_DeregisterObject(EMBX_INVALID_HANDLE_VALUE,hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 3 */
    res = EMBX_GetObject(EMBX_INVALID_HANDLE_VALUE,hObj1,&objptr,&objsize);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

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
    res = EMBX_RegisterObject(EMBX_INVALID_HANDLE_VALUE,object,OBJECT_SIZE,&hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 8 */
    res = EMBX_RegisterObject(tp1,object,OBJECT_SIZE,0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_RegisterObject(tp1,object,OBJECT_SIZE,&hObj1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 10 */
    res = EMBX_RegisterObject(tp1,object,OBJECT_SIZE/2,&hObj2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 11 */
    res = EMBX_GetObject(EMBX_INVALID_HANDLE_VALUE,hObj1,&objptr,&objsize);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 12 */
    res = EMBX_GetObject(tp1,EMBX_INVALID_HANDLE_VALUE,&objptr,&objsize);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 13 */
    res = EMBX_GetObject(tp1,hObj1,0,&objsize);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 14 */
    res = EMBX_GetObject(tp1,hObj1,&objptr,0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 15 */
    res = EMBX_GetObject(tp1,hObj1,&objptr,&objsize);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    if(objptr != (EMBX_VOID *)object ||
       objsize != OBJECT_SIZE)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, returned data incorrect\n"));
        bFailed = EMBX_TRUE;
    }

    /* Test 16 */
    res = EMBX_DeregisterObject(EMBX_INVALID_HANDLE_VALUE,hObj2);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 17 */
    res = EMBX_DeregisterObject(tp1,EMBX_INVALID_HANDLE_VALUE);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 18 */
    res = EMBX_DeregisterObject(tp1,hObj2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 19 */
    res = EMBX_DeregisterObject(tp1,hObj1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        EMBX_Info(EMBX_TRUE, ("Skipping test 20\n"));
        bFailed = EMBX_TRUE;
    }
    else
    {
        /* Test 20 */
        res = EMBX_DeregisterObject(tp1,hObj1);
        if(res != EMBX_INVALID_ARGUMENT)
        {
            EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
            bFailed = EMBX_TRUE;
        }
    }

    /* Test 21 */
    res = EMBX_CloseTransport(tp1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 22 */
    res = EMBX_RegisterObject(tp1,object,OBJECT_SIZE,&hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test22 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 23 */
    res = EMBX_GetObject(tp1,hObj1,&objptr,&objsize);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test23 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 24 */
    res = EMBX_DeregisterObject(tp1,hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test24 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
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
    res = EMBX_RegisterObject(tp1,0,0,&hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test26 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 27 */
    res = EMBX_DeregisterObject(tp1,hObj1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test27 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 28 */
    res = EMBX_GetObject(tp1,hObj1,&objptr,&objsize);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test28 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 29 */
    res = EMBX_UnregisterTransport(factory1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test29 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 30 */
    res = EMBX_UnregisterTransport(factory2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test30 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
