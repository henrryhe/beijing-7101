/*******************************************************************************

File name : metrics.c

Description : stack usage metrics for the toolbox driver

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                 Name
----               ------------                                 ----
27 Jun 2001         Created                                      CL
06 Feb 2002         Set some functions definitions to static     HS
14 Sep 2004         Add support for OS21                         MH+FQ
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */
#if !defined ST_OSLINUX
/* Under Linux, STTBX functions are defined in STOS vob */
#if defined(ST_OS21) || defined(ST_OSWINCE)
#define STACK_SIZE          16*1024 /* STACKSIZE must oversize the estimated stack usage of the driver */
#endif
#ifdef ST_OS20
#define STACK_SIZE          4096 /* STACKSIZE must oversize the estimated stack usage of the driver */
#endif

/* Includes ----------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#ifdef ST_OS20
#include <ostime.h>
#endif
#include <string.h>

#include "stlite.h"

#include "startup.h"

/* driver usage related includes --------------------------------------------- */
#include "sttbx.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */
static STTBX_InitParams_t InitTbxParam;
static STTBX_TermParams_t TermTbxParam;
static STTBX_BuffersConfigParams_t BufferConfigParams;
static STTBX_Capability_t Capability;
static char cInput;
static char sInput[512];
static BOOL KeyHit;

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */
#define DRIVER_DEVICE_NAME "STACKUSAGE"

/* Private Function prototypes ---------------------------------------------- */
static void ReportError(int Error, char *FunctionName);

static void test_overhead(void *dummy);
static void test_init(void *dummy);
static void test_typical(void *dummy);

/* Functions ---------------------------------------------------------------- */
static void ReportError(int Error, char *FunctionName)
{
    if ((Error) != ST_NO_ERROR)
    {
        printf( "ERROR: %s returned %d \n", FunctionName, Error );
    }
}


/*******************************************************************************
Name        : test_init
Description : calculates the stack usage made by the driver for an Init Term cycle
Parameters  : None
Assumptions :
Limitations : Make sure to not define local variables within the function
              but use module static gloabls instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
static void test_init(void *dummy)
{
    ST_ErrorCode_t Err;

    Err = ST_NO_ERROR;
    InitTbxParam.SupportedDevices = STTBX_DEVICE_DCU;
    InitTbxParam.DefaultOutputDevice = STTBX_DEVICE_DCU;
    InitTbxParam.DefaultInputDevice = STTBX_DEVICE_DCU;
    strcpy(InitTbxParam.UartDeviceName, "");
    InitTbxParam.CPUPartition_p = SystemPartition_p;
    Err = STTBX_Init( DRIVER_DEVICE_NAME, &InitTbxParam);
    ReportError(Err, "STTBX_Init");
    Err = STTBX_Term( DRIVER_DEVICE_NAME, &TermTbxParam);
    ReportError(Err, "STTBX_Term");
}

/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions :
Limitations : Make sure to not define local variables within the function
              but use module static gloabls instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
static void test_typical(void *dummy)
{
    ST_ErrorCode_t Err;

    Err = ST_NO_ERROR;
    InitTbxParam.SupportedDevices = STTBX_DEVICE_DCU;
    InitTbxParam.DefaultOutputDevice = STTBX_DEVICE_DCU;
    InitTbxParam.DefaultInputDevice = STTBX_DEVICE_DCU;
    strcpy(InitTbxParam.UartDeviceName, "");
    InitTbxParam.CPUPartition_p = SystemPartition_p;
    Err = STTBX_Init( DRIVER_DEVICE_NAME, &InitTbxParam);
    ReportError(Err, "STTBX_Init");

    STTBX_GetRevision();
    STTBX_GetCapability(DRIVER_DEVICE_NAME, &Capability);
    BufferConfigParams.Enable = FALSE;
    Err = STTBX_SetBuffersConfig(&BufferConfigParams);
    ReportError(Err, "STTBX_SetBuffersConfig");
    STTBX_Print(("Wait for ONE second.\n"));
    STTBX_WaitMicroseconds(1000000);
    STTBX_Print(("(Poll char) Please press on a key\n"));
    KeyHit = FALSE;
    while (!KeyHit)
    {
#ifdef ST_OSWINCE
        static int millis = 0;
        Sleep(1); /* be polite, don't hog the processor*/
        ++millis;
        /* let the user check that polling is non-blocking ...*/
        if ((millis % 1000) == 0)
            STTBX_Print(("\t%d second%s...\n", millis  / 1000,
                         millis >= 2000 ? "s" : ""));
#endif
        KeyHit = STTBX_InputPollChar(&cInput);
    }
    STTBX_Print(("You typed '%c' key\n", cInput));
    STTBX_Print(("(blocking inputs) Please press on a key\n"));
    STTBX_InputChar(&cInput);
    STTBX_Print(("You typed '%c' key\n", cInput));
    STTBX_Print(("Please enter a string and [ENTER] : \n"));
    STTBX_InputStr(sInput, sizeof(sInput));
    STTBX_Print(("You typed a %d characters long string\n %s \n", strlen(sInput), sInput));
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Just a call to STTBX_Report."));
    STTBX_Print(("Just a call to STTBX_Print.\n"));
    STTBX_DirectPrint(("Just a call to STTBX_DirectPrint.\n"));
    STTBX_Print(("End of typical use for stack usage calculation.\n"));

    Err = STTBX_Term( DRIVER_DEVICE_NAME, &TermTbxParam);
    ReportError(Err, "STTBX_Term");
}

static void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    Err = ST_NO_ERROR;

   ReportError(Err, "test_overhead");
}


/*******************************************************************************
Name        : metrics_Stack_Test
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void metrics_Stack_Test(void)
{
    task_t * task_p;
    task_status_t status;
    int overhead_stackused;
    char *funcname[]= {
        "test_overhead",
        "test_init",
        "test_typical",
        "NULL"
    };
    void (*func_table[])(void *) = {
        test_overhead,
        test_init,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;

    overhead_stackused = 0;
    printf("*************************************\n");
    printf("* Stack usage calculation beginning *\n");
    printf("*************************************\n");
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];

        /* Start the task */
        task_p = task_create(func, NULL, STACK_SIZE, MAX_USER_PRIORITY, "stack_test", task_flags_no_min_stack_size);

        /* Wait for task to complete */
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, task_status_flags_stack_used);
        /* store overhead value */
        if (i==0)
        {
            printf("*-----------------------------------*\n");
            overhead_stackused = status.task_stack_used;
            printf("%s \t func=0x%08lx stack = %d bytes used\n", funcname[i], (long) func,
                    status.task_stack_used);
            printf("*-----------------------------------*\n");
        }
        else
        {
            printf("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", funcname[i], (long) func,
                    status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused);
        }
        /* Tidy up */
        task_delete(task_p);
    }
    printf("*************************************\n");
    printf("*    Stack usage calculation end    *\n");
    printf("*************************************\n");
}

#endif  /* #ifndef ST_OSLINUX */



