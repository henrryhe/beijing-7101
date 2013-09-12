/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_transport.c                                     */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX transport initialization                        */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"

#include "embxlb.h"

#include "embx_osheaders.h"

extern EMBXLB_Config_t loopback_config;
extern EMBXLB_Config_t restricted_loopback_config;
extern EMBXLB_Config_t invalid_loopback_config;

extern char *error_strings[];

int run_test(void)
{
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1,tpinfo2;
EMBX_BOOL      bFailed;
EMBX_TRANSPORT tp1,tp2;
EMBX_FACTORY   factory1,factory2,factory3;

    bFailed = EMBX_FALSE;

    /* Test 1 */
    res = EMBX_OpenTransport("dummy", &tp1);
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 2 */
    res = EMBX_GetTransportInfo(tp1, &tpinfo2);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 3 */
    res = EMBX_CloseTransport(tp1);
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

    res = EMBX_RegisterTransport(EMBXLB_loopback_factory,&invalid_loopback_config,sizeof(EMBXLB_Config_t),&factory3);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Transport Registration 3 failed, res = %s, exiting\n",error_strings[res]));
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
    res = EMBX_OpenTransport(0, &tp1);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 7 */
    res = EMBX_OpenTransport(tpinfo1.name, 0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 8 */
    res = EMBX_OpenTransport("UNKNOWN_TRANSPORT", &tp1);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 9 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 10 */
    res = EMBX_GetTransportInfo(tp1, 0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 11 */
    res = EMBX_GetTransportInfo(tp1, &tpinfo2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        EMBX_Info(EMBX_TRUE, ("Skipping Test 12\n"));
        bFailed = EMBX_TRUE;
    }
    else
    {
        /* Test 12 */
        if( strcmp(tpinfo1.name, tpinfo2.name) != 0 )
        {
            EMBX_Info(EMBX_TRUE, ("Test12 failed, open transport = %s  transport info name = %s\n",tpinfo1.name, tpinfo2.name));
            bFailed = EMBX_TRUE;
        }
    }

    /* Test 13 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp2);
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
        EMBX_Info(EMBX_TRUE, ("Skipping Test15\n"));
        bFailed = EMBX_TRUE;
    }
    else
    {
        /* Test 15 */
        res = EMBX_CloseTransport(tp1);
        if(res != EMBX_INVALID_TRANSPORT)
        {
            EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
            bFailed = EMBX_TRUE;
        }
    }

    /* Test 16 */
    res = EMBX_CloseTransport(tp2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
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
    res = EMBX_OpenTransport("dummy", &tp1);
    if(res != EMBX_DRIVER_NOT_INITIALIZED)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 19 */
    res = EMBX_GetTransportInfo(tp1, &tpinfo2);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 20 */
    res = EMBX_CloseTransport(tp1);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 21 */
    res = EMBX_UnregisterTransport(factory1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 22 */
    res = EMBX_UnregisterTransport(factory2);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test22 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 23 */
    res = EMBX_UnregisterTransport(factory3);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test23 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
