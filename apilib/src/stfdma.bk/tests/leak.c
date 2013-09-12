/*****************************************************************************
File Name   : leak.c

Description : STFDMA memory leak test source

Copyright (C) 2002 STMicroelectronics

History     :

Reference   :
*****************************************************************************/


/* Includes --------------------------------------------------------------- */

#include <string.h>                     /* C libs */
#include <stdio.h>

#include "sttbx.h"
#include "stdevice.h"
#include "stlite.h"                     /* Standard includes */
#include "stddefs.h"
#include "stcommon.h"

#include "stfdma.h"                     /* fdma api header file */

#include "fdmatst.h"

#include "leak.h"

/* Private Variables ------------------------------------------------------ */

/* Declarations for memory partitions */

/* Sizes of partitions */


/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : CaseL01
Description  : Tests for occurance of memory leak during normal usage
Parameters   :
Return Value :
****************************************************************************/
static void CaseL01(void)
{
    STFDMA_InitParams_t         InitParams;
    partition_status_t          MemBefore;
    partition_status_t          MemAfter;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    STTBX_Print(("Checking for memory leak during normal usage...\n\n"));

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemBefore, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
    VERBOSE_PRINT("System partition data before test:\n");
    fdmatst_DisplayPartitionStatus(&MemBefore);

    /* Initialise the FDMA */
    VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.NumberCallbackTasks = 1;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_1;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemAfter, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
    }
    else
    {
        VERBOSE_PRINT("System partition data after initialisation:\n");
        fdmatst_DisplayPartitionStatus(&MemAfter);
    }


    VERBOSE_PRINT("Calling STFDMA_Term, unforced, expect NO_ERROR.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemAfter, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
    VERBOSE_PRINT("System partition data after termination:\n");
    fdmatst_DisplayPartitionStatus(&MemAfter);

    VERBOSE_PRINT("Comparing free space.\n");
    if (MemBefore.partition_status_free != MemAfter.partition_status_free)
    {
        VERBOSE_PRINT("*** ERROR: Memory leak detected!!\n");
        fdmatst_SetSuccessState(FALSE);
    }
}

/****************************************************************************
Name         : CaseL02
Description  : Tests for occurance of memory leak during failed calls to init
Parameters   :
Return Value :
****************************************************************************/
static void CaseL02(void)
{
    STFDMA_InitParams_t         InitParams;
    partition_status_t          MemBefore;
    partition_status_t          MemAfter;
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;

    STTBX_Print(("Checking for memory leak during failed initialisation calls...\n\n"));

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemBefore, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
    VERBOSE_PRINT("System partition data before test:\n");
    fdmatst_DisplayPartitionStatus(&MemBefore);

    /* Parameters that do no result in memory allocation */
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();

    /* Under OS21 the task statcks are allocated by the OS and the
       interrupt level is not used so the first two tests will not fail */

#if !defined (ST_OS21)
    /* Initialise the FDMA  with series of bad parameters, checking
     * memory is freed correctly after exit
     */
    VERBOSE_PRINT("Use bad number of callback tasks to invoke error.\n");
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 100000;              /* error value */
    InitParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT("Calling STFDMA_Init, expect NO_MEMORY.\n");
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_NO_MEMORY);

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemAfter, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
    VERBOSE_PRINT("System partition data after failed intialisation:\n");
    fdmatst_DisplayPartitionStatus(&MemAfter);

    VERBOSE_PRINT("Comparing free space.\n");
    if (MemBefore.partition_status_free != MemAfter.partition_status_free)
    {
        VERBOSE_PRINT("*** ERROR: Memory leak detected!!\n");
        fdmatst_SetSuccessState(FALSE);
    }



    VERBOSE_PRINT("Use bad number of interrupt level to invoke error.\n");
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = -1;       /* error value */
    InitParams.NumberCallbackTasks = 1;
    InitParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT("Calling STFDMA_Init, expect ERROR_INTERRUPT_INSTALL.\n");
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_INTERRUPT_INSTALL);

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemAfter, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
    VERBOSE_PRINT("System partition data after failed initialisation:\n");
    fdmatst_DisplayPartitionStatus(&MemAfter);

    VERBOSE_PRINT("Comparing free space.\n");
    if (MemBefore.partition_status_free != MemAfter.partition_status_free)
    {
        VERBOSE_PRINT("*** ERROR: Memory leak detected!!\n");
        fdmatst_SetSuccessState(FALSE);
    }
#endif

    VERBOSE_PRINT("Use bad number of interrupt level to invoke error.\n");
    InitParams.InterruptNumber = -1;      /* error value */
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT("Calling STFDMA_Init, expect ERROR_INTERRUPT_INSTALL.\n");
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
#if defined(ST_OSWINCE)
	fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

	VERBOSE_PRINT("Calling STFDMA_Term, unforced, expect NO_ERROR.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
#else
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_INTERRUPT_INSTALL);
#endif

    VERBOSE_PRINT("Acquiring partition data.\n");
    if (partition_status(fdmatst_GetSystemPartition(), &MemAfter, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
    VERBOSE_PRINT("System partition data after failed initialisation:\n");
    fdmatst_DisplayPartitionStatus(&MemAfter);

    VERBOSE_PRINT("Comparing free space.\n");
    if (MemBefore.partition_status_free != MemAfter.partition_status_free)
    {
        VERBOSE_PRINT("*** ERROR: Memory leak detected!!\n");
        fdmatst_SetSuccessState(FALSE);
    }
}

/****************************************************************************
Name         : leak_RunLeakTest
Description  : Entry point for leak test
Parameters   :
Return Value :
****************************************************************************/
void leak_RunLeakTest(void)
{

    STTBX_Print(("\nLeak Test Cases:\n"));
    STTBX_Print(("--------------------------------------------------------\n"));

    fdmatst_ResetSuccessState();
    STTBX_Print(("L01 executing:\n\n"));

    CaseL01();
    fmdatst_TestCaseSummarise();

    STTBX_Print(("L02 executing:\n\n"));
    CaseL02();
    fmdatst_TestCaseSummarise();
}



/* eof */
