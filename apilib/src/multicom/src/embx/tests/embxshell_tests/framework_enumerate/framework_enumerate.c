/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_enumerate.c                                     */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX driver transport enumeration                    */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"

#include "embxlb.h"

extern EMBXLB_Config_t loopback_config;
extern EMBXLB_Config_t restricted_loopback_config;
extern EMBXLB_Config_t invalid_loopback_config;

extern char *error_strings[];

int run_test(void)
{
EMBX_ERROR   res;
EMBX_TPINFO  tpinfo;
EMBX_TPINFO *dummy;
EMBX_BOOL    bFailed;
EMBX_FACTORY factory1,factory2,factory3;

    dummy = 0;
    bFailed = EMBX_FALSE;

    /* Test 1 */
    res = EMBX_GetFirstTransport(&tpinfo);
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 2 */
    res = EMBX_GetNextTransport(&tpinfo);
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
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

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&invalid_loopback_config,sizeof(EMBXLB_Config_t),&factory3);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration 3 failed, res = %s, exiting\n",error_strings[res]));
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
    res = EMBX_GetFirstTransport(dummy);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 5 */
    res = EMBX_GetFirstTransport(&tpinfo);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_GetNextTransport(dummy);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 7 */
    res = EMBX_GetNextTransport(&tpinfo);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 8 */
    res = EMBX_GetNextTransport(&tpinfo);
    if(res != EMBX_INVALID_STATUS)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_Deinit();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* The following tests exercise a different code path to those before EMBX_Init */

    /* Test 10 */
    res = EMBX_GetFirstTransport(&tpinfo);
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 11 */
    res = EMBX_GetNextTransport(&tpinfo);
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        bFailed = EMBX_TRUE;
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
    }

    /* Test 12 */
    res = EMBX_UnregisterTransport(factory1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 13 */
    res = EMBX_UnregisterTransport(factory2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 14 */
    res = EMBX_UnregisterTransport(factory3);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}   
