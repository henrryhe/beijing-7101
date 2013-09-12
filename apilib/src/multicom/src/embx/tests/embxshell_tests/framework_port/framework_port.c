/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: framework_port.c                                          */
/*                                                                 */
/* Description:                                                    */
/*      Tests EMBX port creation                                   */
/*                                                                 */
/*******************************************************************/

#include "embx.h"
#include "embx_debug.h"
#include "embx_osinterface.h"

#include "embxlb.h"

extern EMBXLB_Config_t restricted_loopback_config;
extern char *error_strings[];

int run_test(void)
{
EMBX_ERROR     res;
EMBX_TPINFO    tpinfo1;
EMBX_PORT      dummyPort;
EMBX_UINT      i;
EMBX_CHAR      longName[EMBX_MAX_PORT_NAME+2];
EMBX_FACTORY   factory;
EMBX_BOOL      bFailed;
EMBX_TRANSPORT tp;
EMBX_PORT     *localPorts;

    bFailed    = EMBX_FALSE;
    tp         = EMBX_INVALID_HANDLE_VALUE;
    localPorts = 0;

    /* Test 1 */
    res = EMBX_CreatePort(tp, "dummy", &dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test1 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 2 */
    res = EMBX_ClosePort(dummyPort);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test2 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 3 */
    res = EMBX_InvalidatePort(dummyPort);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test3 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 4 */
    res = EMBX_RegisterTransport(EMBXLB_loopback_factory, &restricted_loopback_config, sizeof(EMBXLB_Config_t), &factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test4 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 5 */
    res = EMBX_Init();
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test5 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 6 */
    res = EMBX_GetFirstTransport(&tpinfo1);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test6 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }


    /* Test 7 */
    res = EMBX_OpenTransport(tpinfo1.name, &tp);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test7 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }


    localPorts = (EMBX_PORT *)EMBX_OS_MemAlloc(tpinfo1.maxPorts * sizeof(EMBX_PORT));
    if(localPorts == 0)
    {
        EMBX_Info(EMBX_TRUE, ("Test Harness failed, out of memory\n"));
        goto skip_remaining_tests;
    }

    /* Test 8 */
    res = EMBX_CreatePort(EMBX_INVALID_HANDLE_VALUE, "dummy", &dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test8 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }
  
    /* Test 9 */
    res = EMBX_CreatePort(tp, 0, &dummyPort);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test9 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }
    
    /* Test 10 */
    res = EMBX_CreatePort(tp, "dummy", 0);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test10 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 11 */
    memset(longName, 'a', EMBX_MAX_PORT_NAME+1);
    longName[EMBX_MAX_PORT_NAME+1] = '\0';

    res = EMBX_CreatePort(tp, longName, &dummyPort);
    if(res != EMBX_INVALID_ARGUMENT)
    {
        EMBX_Info(EMBX_TRUE, ("Test11 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
        if(res == EMBX_SUCCESS)
        {
            if( EMBX_ClosePort(dummyPort) != EMBX_SUCCESS )
            {
                EMBX_Info(EMBX_TRUE, ("Couldn't close unexpectedly created port\n"));
                goto skip_remaining_tests;
            }
        }       
    }

    /* Test 12 */
    for(i=0;i<tpinfo1.maxPorts;i++)
    {
    char name[EMBX_MAX_PORT_NAME+1];

        sprintf(name, "port%d", i);
        res = EMBX_CreatePort(tp, name, &localPorts[i]);
        if(res != EMBX_SUCCESS)
        {
            EMBX_Info(EMBX_TRUE, ("Test12 failed, res = %s\n",error_strings[res]));
            goto skip_remaining_tests;
        }
    }

    /* Test 13 */
    res = EMBX_CreatePort(tp, "dummy", &dummyPort);
    if(res != EMBX_NOMEM)
    {
        EMBX_Info(EMBX_TRUE, ("Test13 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    
    /* Test 14 */
    res = EMBX_ClosePort(tp);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test14 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 15 */
    res = EMBX_InvalidatePort(tp);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test15 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 16 */
    res = EMBX_ClosePort(localPorts[0]);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test16 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }
    else
    {
        /* Test 17 */
        res = EMBX_ClosePort(localPorts[0]);
        if(res != EMBX_INVALID_PORT)
        {
            EMBX_Info(EMBX_TRUE, ("Test17 failed, res = %s\n",error_strings[res]));
            bFailed = EMBX_TRUE;
        }
    }

    /* Test 18 */
    res = EMBX_CreatePort(tp, "port1", &dummyPort);
    if(res != EMBX_ALREADY_BIND)
    {
        EMBX_Info(EMBX_TRUE, ("Test18 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 19 */
    res = EMBX_CloseTransport(tp);
    if(res != EMBX_PORTS_STILL_OPEN)
    {
        EMBX_Info(EMBX_TRUE, ("Test19 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 20 */
    res = EMBX_InvalidatePort(localPorts[1]);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test20 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 21 - regression test for INSbl19113, invalidate did not unbind name */
    res = EMBX_CreatePort(tp, "port1", &dummyPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test21 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 22 */
    res = EMBX_ClosePort(dummyPort);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test22 failed, res = %s\n",error_strings[res]));
        goto skip_remaining_tests;
    }

    /* Test 23 */
    for(i=1;i<tpinfo1.maxPorts;i++) /* port0 already been closed */
    {
        res = EMBX_ClosePort(localPorts[i]);
        if(res != EMBX_SUCCESS)
        {
            EMBX_Info(EMBX_TRUE, ("Test23 failed, res = %s\n",error_strings[res]));
            goto skip_remaining_tests;
        }
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
    res = EMBX_CreatePort(tp, "dummy", &dummyPort);
    if(res != EMBX_INVALID_TRANSPORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test26 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 27 */
    res = EMBX_ClosePort(dummyPort);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test27 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 28 */
    res = EMBX_InvalidatePort(dummyPort);
    if(res != EMBX_INVALID_PORT)
    {
        EMBX_Info(EMBX_TRUE, ("Test28 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    /* Test 29 */
    res = EMBX_UnregisterTransport(factory);
    if(res != EMBX_SUCCESS)
    {
        EMBX_Info(EMBX_TRUE, ("Test29 failed, res = %s\n",error_strings[res]));
        bFailed = EMBX_TRUE;
    }

    return bFailed?-1:0;

skip_remaining_tests:
    EMBX_Info(EMBX_TRUE, ("Skipping Remaining Tests\n"));
    return -1;
}
