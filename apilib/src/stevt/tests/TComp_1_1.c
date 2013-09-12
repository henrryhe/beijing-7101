/******************************************************************************\
 *
 * File Name   : TComp_1_1.c
 *
 * Description : Test STEVT_Open Interface
 *
 * Author      : Manuela Ielo
 *
 * Copyright STMicroelectronics - February 1999
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "evt_test.h"

ST_ErrorCode_t TCase_1_1_1(void);
ST_ErrorCode_t TCase_1_1_2(void);
ST_ErrorCode_t TCase_1_1_3(void);
ST_ErrorCode_t TCase_1_1_4(void);
ST_ErrorCode_t TCase_1_1_5(STEVT_InitParams_t InitParams);

#define EVENT3 3

U32 TComp_1_1()
{
    ST_ErrorCode_t Error;
    U32 ErrorNum=0;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum = 4;
    InitParam.ConnectMaxNum = 4;
    InitParam.SubscrMaxNum = 4;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = (ST_Partition_t *)SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
         STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "Dev1 initialisation failed"))
         ErrorNum++;
         return ErrorNum;
    }

    Error = STEVT_Init("Dev2", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev2 initialisation failed"))
        ErrorNum++;
        return ErrorNum;
    }

    Error = TCase_1_1_1();
    if (Error != ST_ERROR_UNKNOWN_DEVICE)
    {

        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

/*     Error = TCase_1_1_2(); */
/*     if (Error != ST_ERROR_NO_FREE_HANDLES) */
/*     { */
/*         STEVT_Print(( "^^^Failed Test Case 2\n")); */
/*         ErrorNum++; */
/*     } */
/*     else  */
/*     { */
/*         STEVT_Print(( "^^^Passed Test Case 2\n")); */
/*     } */
    STEVT_Print(("^^^Removed Test Case 2\n"));

    Error = TCase_1_1_3();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }
    Error = TCase_1_1_4();
    if (Error != ST_ERROR_UNKNOWN_DEVICE)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

    Error = TCase_1_1_5(InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 5\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 5\n"));
    }

    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
         STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "Dev1 termination failed"))
         ErrorNum++;
         return ErrorNum;
    }

    Error = STEVT_Term("Dev2", &TermParam);
    if (Error != ST_NO_ERROR)
    {
         STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "Dev2 termination failed"))
         ErrorNum++;
         return ErrorNum;
    }

    return ErrorNum;
}

/*  This test check that ST_ERROR_UNKNOWN_DEVICE is returned
 *  doing an open for a not initialised device */
ST_ErrorCode_t TCase_1_1_1()
{
    STEVT_Handle_t Handle ;

    return STEVT_Open("Dev3", NULL, &Handle);
}

/*  This test check that ST_ERROR_NO_FREE_HANDLES is
 *  returned filling up the Connection List. We initialised the
 *  list with 4 elements */
ST_ErrorCode_t TCase_1_1_2()
{
    STEVT_Handle_t Handle1, Handle2, Handle3, Handle4, Handle5;
    ST_ErrorCode_t Error, Error1;

    Error1 = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Opening Handle1 failed\n"));
    }

    Error1 = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Opening Handle2 failed\n"));
    }

    Error1 = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Opening Handle3 failed\n"));
    }

    Error1 = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Opening Handle4 failed\n"));
    }

    Error = STEVT_Open("Dev1", NULL, &Handle5);

    return Error;
}

/*  It checks if all works well */
ST_ErrorCode_t TCase_1_1_3()
{
    STEVT_Handle_t Handle1;
    ST_ErrorCode_t Error;

    Error = STEVT_Open("Dev2", NULL, &Handle1);

    return Error;
}

ST_ErrorCode_t TCase_1_1_4()
{
    STEVT_Handle_t Handle1;
    ST_ErrorCode_t Error;
    ST_DeviceName_t DeviceName;

    Error = STEVT_Open(DeviceName, NULL, &Handle1);

    return Error;
}

ST_ErrorCode_t TCase_1_1_5(STEVT_InitParams_t InitParams)
{
    STEVT_Handle_t Handle1;
    STEVT_TermParams_t TermParams;
    ST_ErrorCode_t Error;

    Error = STEVT_Init("", &InitParams);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test case 5 Initialization failed"));
    }

    Error |= STEVT_Open("", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test case 5 open failed"));
    }

    Error |= STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test case 5 close failed"));
    }

    TermParams.ForceTerminate = FALSE;
    Error |= STEVT_Term("", &TermParams);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test case 5 term failed"));
    }

    return Error;
}




