/******************************************************************************\
 *
 * File Name   : TComp_5_1.c
 *
 * Description : Test STEVT_Init Interface
 *
 * Author      : Manuela Ielo
 *
 * Copyright STMicroelectronics - February 1999
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "evt_test.h"

ST_ErrorCode_t TCase_5_1_1(STEVT_InitParams_t InitParam);
ST_ErrorCode_t TCase_5_1_2(STEVT_InitParams_t InitParam);
ST_ErrorCode_t TCase_5_1_3(void);
ST_ErrorCode_t TCase_5_1_4(STEVT_InitParams_t InitParam);

ST_ErrorCode_t TComp_5_1()
{
    ST_ErrorCode_t Error, Error1;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum = 4;
    InitParam.ConnectMaxNum = 5;
    InitParam.SubscrMaxNum = 4;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    Error = TCase_5_1_1(InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    Error1 = TCase_5_1_2(InitParam);
    if (Error1 != ST_ERROR_ALREADY_INITIALIZED)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_5_1_3();
    if (Error != ST_ERROR_BAD_PARAMETER)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    Error = TCase_5_1_4(InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed"))
        return Error;
    }
    Error = STEVT_Term("Dev2", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev2 termination failed"))
        return Error;
    }
    Error = STEVT_Term("Devx", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Devx termination failed"))
        return Error;
    }
    Error = STEVT_Term("Dev5", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev5 termination failed"))
        return Error;
    }
    Error = STEVT_Term("Devi", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Devi termination failed"))
        return Error;
    }

    return ST_NO_ERROR;
}

/*  This function checks if Init function works well calling
 *  more than once realloc function  */
ST_ErrorCode_t TCase_5_1_1(STEVT_InitParams_t InitParam)
{
    ST_ErrorCode_t Error, Error1;
    char dev[16];
    int i;
    STEVT_TermParams_t TermParam;

    TermParam.ForceTerminate = FALSE;

    /*  We fill-up the dictionary to check if at the
     *  next initialization the appropriate error code
     *  is returned */
    for (i = 1; i <= 5; i++)
    {
        sprintf(dev, "Dev%d", i);
        Error = STEVT_Init(dev, &InitParam);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "%s initialization failed", dev))
            return Error;
        }

    }

    Error = STEVT_Init("Devx", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                     "%s initialization failed", "Devx"))
        return Error;
    }

    Error1 = STEVT_Term("Dev5", &TermParam);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev5 term failed"))
        return Error1;
    }

    Error1 = STEVT_Term("Dev4", &TermParam);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev4 term failed"))
        return Error1;
    }


    return Error;
}

/*  This test checks if is returned ST_ERROR_ALREADY_INITIALIZED
 *  trying to init an already initialized EH */
ST_ErrorCode_t TCase_5_1_2(STEVT_InitParams_t InitParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Init("Dev1", &InitParam);

    return Error;
}

/*  This test needs to check what happen if InitParams are NULL
 *  It's impossible doing any initialisation */
ST_ErrorCode_t TCase_5_1_3(void)
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam1, InitParam2, InitParam3;

    InitParam1.EventMaxNum = 0;
    InitParam1.ConnectMaxNum = 4;
    InitParam1.SubscrMaxNum = 5;
    InitParam2.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    InitParam2.EventMaxNum = 4;
    InitParam2.ConnectMaxNum = 0;
    InitParam2.SubscrMaxNum = 6;
    InitParam2.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    InitParam3.EventMaxNum = 4;
    InitParam3.ConnectMaxNum = 6;
    InitParam3.SubscrMaxNum = 0;
    InitParam3.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    Error = STEVT_Init("device", NULL);
    if (Error != ST_ERROR_BAD_PARAMETER)
    {
        return Error;
    }

    Error = STEVT_Init("device1", &InitParam1);
    if (Error != ST_ERROR_BAD_PARAMETER)
    {
        return Error;
    }

    Error = STEVT_Init("device2", &InitParam2);
    if (Error != ST_ERROR_BAD_PARAMETER)
    {
        return Error;
    }

    Error = STEVT_Init("device3", &InitParam3);

    return Error;
}

/*  This test need to check if the initialisation works appropriately
 *  after a term */
ST_ErrorCode_t TCase_5_1_4(STEVT_InitParams_t InitParam)
{
    ST_ErrorCode_t Error;
    STEVT_TermParams_t TermParams;

    TermParams.ForceTerminate = TRUE;

    /*  To fill up the dictionary */
    Error = STEVT_Init("Devi", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Dev5 initialization  failed"))
        return Error;
    }

    /*  Free an intermidiate dictionary slot */
    Error = STEVT_Term("Dev3", &TermParams);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4  Dev3 termination failed"))
        return Error;
    }

    Error = STEVT_Init("Dev5", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Dev5 initialization  failed"))
        return Error;
    }

    Error = STEVT_Term("Dev2", &TermParams);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4  Dev2 termination failed"))
        return Error;
    }



    Error = STEVT_Init("Dev2", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Dev2 initialization  failed"))
        return Error;
    }

    return Error;
}
