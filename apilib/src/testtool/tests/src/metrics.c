/*******************************************************************************

File name : metrics.c

Description : stack usage metrics for the toolbox driver

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                 Name
----               ------------                                 ----
27 Jun 2001         Created                                      CL
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */
#define STACK_SIZE          16384 /* STACKSIZE must oversize the estimated stack usage of the driver */

/* Includes ----------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <ostime.h>
#include <string.h>

#include "stddefs.h"

/* driver usage related includes --------------------------------------------- */
#include "testtool.h"
#include "cltst.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

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
        printf( "ERROR: %s returned %d\n", FunctionName, Error );
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
void test_init(void *dummy)
{
    TST_Init(NULL);
    TST_Term();
}

/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions :
Limitations : Make sure to not define local variables within the function
              but use module static globals instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{
    ST_ErrorCode_t Err;

    Err = ST_NO_ERROR;

    TST_Init(NULL);
    Err = STTST_SetMode(STTST_BATCH_MODE);
    ReportError(Err, "STTST_SetMode");
    STTST_Start();
    TST_Term();
}

void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    Err = ST_NO_ERROR;

   ReportError(Err, "test_overhead");
}


/*******************************************************************************
Name        : MetricsStackTest
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
void MetricsStackTest(void)
{
    task_t *task_p;
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
        task_status(task_p, &status, 1);
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

