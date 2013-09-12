/*****************************************************************************
File Name   : param.c

Description : STFDMA parameter test source

Copyright (C) 2002 STMicroelectronics

History     :

Reference   :
*****************************************************************************/


/* Includes --------------------------------------------------------------- */
#define STTBX_PRINT 1   /*Always required on */

#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <asm/semaphore.h>
#else
#include <string.h>                     /* C libs */
#include "sttbx.h"
#include "stcommon.h"
#endif

#include "stdevice.h"
#include "stddefs.h"

#include "stfdma.h"                     /* fdma api header file */
#include "fdmatst.h"

#include "param.h"

/* Private Constants ------------------------------------------------------ */

#if defined (ST_7200)
#define STFDMA_TEST STFDMA_2
#define FDMA_TEST_BASE_ADDRESS (U32 *)FDMA_1_BASE_ADDRESS
#define FDMA_TEST_INTERRUPT_NUMBER FDMA_1_INTERRUPT
#else
#define STFDMA_TEST STFDMA_1
#define FDMA_TEST_BASE_ADDRESS fdmatst_GetBaseAddress()
#define FDMA_TEST_INTERRUPT_NUMBER FDMA_INTERRUPT_NUMBER
#endif

/* Private Variables ------------------------------------------------------ */

static void P01(void);
static void P02(void);
static void P03(void);
static void P04(void);
static void P05(void);
static void P06(void);
static void P07(void);
static void P08(void);
static void P09(void);
static void P10(void);
static void P11(void);
static void P12(void);
static void P13(void);
static void P14(void);
static void P15(void);
static void P16(void);
static void P17(void);
static void P18(void);
static void P19(void);
static void P20(void);
static void P21(void);
static void P22(void);
static void P23(void);
static void P24(void);
static void P25(void);
static void P26(void);
static void P27(void);
static void P28(void);
static void P29(void);

static void Init(void);
static void Term(void);

/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : param_RunParamTest
Description  : Entry point for bad parameter tests
Parameters   :
Return Value :
****************************************************************************/
void param_RunParamTest(void)
{
    fdmatst_ResetSuccessState();
    STTBX_Print(("\nParameter Test Cases:\n"));
    STTBX_Print(("--------------------------------------------------------\n"));

    P01();
    P02();
    P03();
    P04();
    P05();
    P06();
    P07();
    P08();
    P09();
    P10();
    P11();
    P12();
    P13();
    P14();
    P15();
    P16();
    P17();
    P18();
    P19();
    P20();
    P21();
    P22();
    P23();
    P24();
    P25();
    P26();
    P27();
    P28();
    P29();
}

static void P01()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("P01 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init with bad name.\n"));
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = FDMA_TEST_BASE_ADDRESS;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_Init("This FDMA device name is far too long", &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P02()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    STTBX_Print(("P02 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init null init params.\n"));
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, NULL);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P03()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("P03 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init with bad devicetype.\n"));
    InitParams.DeviceType = 1000;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = FDMA_TEST_BASE_ADDRESS;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P04()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("P04 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init with NULL driver partition.\n"));
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = NULL;
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = FDMA_TEST_BASE_ADDRESS;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P05()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("P05 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init with NULL ncache partition.\n"));
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = NULL;
    InitParams.InterruptNumber = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = FDMA_TEST_BASE_ADDRESS;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P06()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("P06 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init with NULL base address.\n"));
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = NULL;
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P07()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("P07 executing:\n\n"));
    STTBX_Print(("Testing STFDMA_Init with illegal TicksPerSecond....\n"));
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.ClockTicksPerSecond = 0;
    InitParams.BaseAddress_p = FDMA_TEST_BASE_ADDRESS;
    InitParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();
}




static void P08()
{
}




static void P09()
{
}




static void P10()
{
    ST_ErrorCode_t      ErrorCode   = ST_NO_ERROR;
    STFDMA_TransferId_t TransferId  = 0;

    STTBX_Print(("P10 executing:\n\n"));

    Init();

    VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect BAD_PARAMETER.\n");
    ErrorCode = STFDMA_GetTransferStatus(TransferId, NULL);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);
    fmdatst_TestCaseSummarise();

    Term();
}




static void P11()
{
}




static void P12()
{
}




static void P13()
{
}




static void P14()
{
}




static void P15()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_TransferId_t TransferId;

    STTBX_Print(("P15 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_StartTransfer with NULL transfer params....\n"));
    ErrorCode = STFDMA_StartTransfer(NULL, &TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P16()
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t TransferParams;

    STTBX_Print(("P16 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_StartTransfer with NULL transfer id....\n"));
    ErrorCode = STFDMA_StartTransfer(&TransferParams, NULL);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P17()
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t TransferParams;
    STFDMA_TransferId_t     TransferId;

    STTBX_Print(("P17 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_StartTransfer with NULL node address....\n"));

    TransferParams.BlockingTimeout = 0;
    TransferParams.CallbackFunction = NULL;
    TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
    TransferParams.NodeAddress_p = NULL;
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_TEST;
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P18()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_TransferId_t TransferId;

    STTBX_Print(("P18 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_StartGenericTransfer with NULL transfer params....\n"));
    ErrorCode = STFDMA_StartGenericTransfer(NULL, &TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P19()
{
     /* Covered by P16 */
}




static void P20()
{
     /* Covered by P17 */
}




static void P21()
{
    ST_ErrorCode_t                 ErrorCode = ST_NO_ERROR;
    STFDMA_TransferGenericParams_t TransferParams;
    STFDMA_TransferId_t            TransferId;
    STFDMA_GenericNode_t           Node;

    STTBX_Print(("P21 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_StartGenericTransfer with illegal Pool....\n"));

    TransferParams.BlockingTimeout = 0;
    TransferParams.CallbackFunction = NULL;
    TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
    TransferParams.Pool      = -1;
    TransferParams.NodeAddress_p = &Node;
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_TEST;

    ErrorCode = STFDMA_StartGenericTransfer(&TransferParams, &TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    TransferParams.Pool      = 100;

    ErrorCode = STFDMA_StartGenericTransfer(&TransferParams, &TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P22()
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STTBX_Print(("P22 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_LockChannel with NULL channel id....\n"));
    ErrorCode = STFDMA_LockChannel(NULL, STFDMA_TEST);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P23()
{
     /* Covered by P22 */
}




static void P24()
{
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;
    STFDMA_ChannelId_t ChannelId;

    STTBX_Print(("P24 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_LockChannelInPool with Invalid pool...\n"));

    ErrorCode = STFDMA_LockChannelInPool(-1, &ChannelId, STFDMA_TEST);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    ErrorCode = STFDMA_LockChannelInPool(100, &ChannelId, STFDMA_TEST);
    fdmatst_ErrorReport("", ErrorCode, ST_ERROR_BAD_PARAMETER);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P25()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_ContextId_t  Context1; /* Invalid */
    STFDMA_ContextId_t  Context2; /* Stale */
    STFDMA_ContextId_t  Context3; /* Good */
    STFDMA_SCEntry_t    SCList[64];
    STFDMA_SCEntry_t   *SCListPtr;
    U32                 Size = sizeof(SCList);
    BOOL                Overflow;
    STFDMA_SCState_t    State;
    void               *Ptr;

    STTBX_Print(("P25 executing:\n\n"));

    Init();

    Context1 = 1;

    STTBX_Print(("Create a stale context id...\n"));

    ErrorCode = STFDMA_AllocateContext(&Context2);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

    ErrorCode = STFDMA_DeallocateContext(Context2);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,STFDMA_ERROR_INVALID_CONTEXT_ID));

    ErrorCode = STFDMA_AllocateContext(&Context3);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

    STTBX_Print(("Testing STFDMA_ContextSetSCList with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextSetSCList(Context1, SCList,  Size);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextSetSCList(Context2, SCList,  Size);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextGetSCList with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextGetSCList(Context1, &SCListPtr, &Size, &Overflow);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextGetSCList(Context2, &SCListPtr, &Size, &Overflow);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextGetSCState with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextGetSCState(Context1, &State, STFDMA_DEVICE_PES_RANGE_0);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextGetSCState(Context2, &State, STFDMA_DEVICE_PES_RANGE_0);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextSetSCState with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextSetSCState(Context1, &State, STFDMA_DEVICE_PES_RANGE_0);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextSetSCState(Context2, &State, STFDMA_DEVICE_PES_RANGE_0);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextSetESBuffer with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextSetESBuffer(Context1, (void*)0X1000, 0X1000);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextSetESBuffer(Context2, (void*)0X1000, 0X1000);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextSetESReadPtr with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextSetESReadPtr(Context1, Ptr);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextSetESReadPtr(Context2, Ptr);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextGetESReadPtr with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextGetESReadPtr(Context1, &Ptr);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextGetESReadPtr(Context2, &Ptr);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextSetESWritePtr with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextSetESWritePtr(Context1, Ptr);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextSetESWritePtr(Context2, Ptr);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    STTBX_Print(("Testing STFDMA_ContextGetESWritePtr with Invalid Context Id...\n"));

    ErrorCode = STFDMA_ContextGetESWritePtr(Context1, &Ptr, NULL);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    ErrorCode = STFDMA_ContextGetESWritePtr(Context2, &Ptr, NULL);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_CONTEXT_ID);

    Term();

    fmdatst_TestCaseSummarise();
}




static void P26()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_ContextId_t  Context;

    STTBX_Print(("P26 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_ContextSetESBuffer with Invalid Buffer...\n"));

    ErrorCode = STFDMA_AllocateContext(&Context);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

    ErrorCode = STFDMA_ContextSetESBuffer(Context, NULL, 0X1000);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    ErrorCode = STFDMA_ContextSetESBuffer(Context, (void*)0X1010, 0X1000);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    Term();

    fmdatst_TestCaseSummarise();
}




static void P27()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_ContextId_t  Context;

    STTBX_Print(("P27 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_ContextSetESBuffer with Invalid Size...\n"));

    ErrorCode = STFDMA_AllocateContext(&Context);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

    ErrorCode = STFDMA_ContextSetESBuffer(Context, (void*)0X1000, 0);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    ErrorCode = STFDMA_ContextSetESBuffer(Context, (void*)0X1000, 0X1010);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    Term();

    fmdatst_TestCaseSummarise();
}




static void P28()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_ContextId_t  Context;

    STTBX_Print(("P28 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_ContextSetESBuffer with Invalid Size...\n"));

    ErrorCode = STFDMA_AllocateContext(&Context);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

    ErrorCode = STFDMA_ContextSetSCList(Context, (STFDMA_SCEntry_t*)NULL, 0X1000);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    ErrorCode = STFDMA_ContextSetSCList(Context, (STFDMA_SCEntry_t*)0X1008, 0X1000);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    Term();

    fmdatst_TestCaseSummarise();
}




static void P29()
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STFDMA_ContextId_t  Context;

    STTBX_Print(("P29 executing:\n\n"));

    Init();

    STTBX_Print(("Testing STFDMA_ContextSetESBuffer with Invalid Size...\n"));

    ErrorCode = STFDMA_AllocateContext(&Context);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

    ErrorCode = STFDMA_ContextSetSCList(Context, (STFDMA_SCEntry_t*)0X1000, 0X1008);
    fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_ERROR_BAD_PARAMETER,STFDMA_ERROR_INVALID_CONTEXT_ID));

    Term();

    fmdatst_TestCaseSummarise();
}

#if !defined (ST_OSLINUX)
static partition_status_t MemBefore;
static partition_status_t MemAfter;
#endif

static void Init()
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t InitParams;

    STTBX_Print(("Successful init call to facilitate further testing:\n\n"));

#if !defined (ST_OSLINUX)
    if (partition_status(fdmatst_GetSystemPartition(), &MemBefore, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }
#endif

    InitParams.DeviceType           = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p    = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p    = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber      = FDMA_TEST_INTERRUPT_NUMBER;
    InitParams.InterruptLevel       = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks  = 1;
    InitParams.ClockTicksPerSecond  = fdmatst_GetClocksPerSec();
    InitParams.BaseAddress_p        = FDMA_TEST_BASE_ADDRESS;
    InitParams.FDMABlock            = STFDMA_TEST;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
}




static void Term()
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STTBX_Print(("Successful term call for clean up:\n\n"));
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_TEST);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (ST_OSLINUX)
    if (partition_status(fdmatst_GetSystemPartition(), &MemAfter, 0) == -1)
    {
        VERBOSE_PRINT("*** ERROR: Could not acquire partition data\n");
        fdmatst_SetSuccessState(FALSE);
        return;
    }

    VERBOSE_PRINT("Comparing free space.\n");
    if (MemBefore.partition_status_free != MemAfter.partition_status_free)
    {
        VERBOSE_PRINT("*** ERROR: Memory leak detected!!\n");
        fdmatst_SetSuccessState(FALSE);
    }
#endif
}




/* eof */
