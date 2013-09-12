/************************************************************************
File name   : denc_test.c
Description : DENC various tests commands

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
                   Created
30 Oct 2002        Add 5517 support                                 HSdLM
21 Aug 2003        Add 5100 support                                 HSdLM
27 Feb 2004        Add new STDENC_DEVICE_TYPE_V13 for STi5100        MH
05 Jul 2004        Add 7710/mb391 support                            TA
13 Sep 2004        Add support for ST40/OS21                         MH
01 Dec 2004        Add 5105/mb400 support                            TA
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef ST_OS20
#include "task.h"
#endif
#ifdef ST_OS21
#include "os21/task.h"
#endif

#ifdef ST_OS20
#include "time.h"
#endif /* ST_OS20 */

#include "stddefs.h"

/*#if !defined(ST_OSLINUX)*/
#include "sttbx.h"
/*#endif */

#include "stdevice.h"

#include "stsys.h"
#include "stcommon.h"
#include "testtool.h"
#include "stdenc.h"
#include "denc_cmd.h"
#include "startup.h"
#include "api_cmd.h"
#include "testcfg.h"

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#include "cli2c.h"
#include "sti2c.h"
#endif  /* #ifdef USE_I2C */

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5510) || defined (ST_5512) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))

#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_8_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#elif defined (ST_5528)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#elif defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188) || defined(ST_5525)|| defined(ST_5107)||defined(ST_5162)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_V13
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#elif defined(ST_7015)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7015
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7015_DENC_OFFSET
#elif defined (ST_7020)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7020
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7020_DENC_OFFSET
#elif defined (ST_GX1)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_GX1
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         0xBB430000 /* different from value given in vob chip DENC_BASE_ADDRESS */
#define DENC_REG_SHIFT           1
#elif defined (ST_7710)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7710_DENC_BASE_ADDRESS
#elif defined (ST_7100)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7100_DENC_BASE_ADDRESS
#elif defined (ST_7109)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7109_DENC_BASE_ADDRESS
#elif defined (ST_7200)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC1_BASE_ADDRESS        DENC_1_BASE_ADDRESS
#define DNC_BASE_ADDRESS         DENC_0_BASE_ADDRESS

#else
#error Not defined yet
#endif

/* Private Variables (static)------------------------------------------------ */
static char Test_Msg[200];

#define I2C_STV119_ADDRESS   0x40

#define TEST_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(x)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

static STDENC_Handle_t DENCHandle;
void os20_main(void *ptr);
BOOL Test_CmdStart(void);

#ifdef ST_OS20
static STDENC_InitParams_t InitDENCParam;
static STDENC_OpenParams_t OpenDENCParam;
static STDENC_TermParams_t TermDENCParam;
static STDENC_Capability_t DENCCapability;
static STDENC_EncodingMode_t ModeDenc;
static STDENC_ModeOption_t ModeOption;
static ST_ErrorCode_t Err;

/* --- Macros ---*/
#define STACK_SIZE          4096 /* STACKSIZE must oversize the estimated stack usage of the driver */
#define DRIVER_DEVICE_NAME "STACKUSAGE"

/* Private Function prototypes ---------------------------------------------- */

static void test_overhead(void *dummy);
static void test_init(void *dummy);
static void test_typical(void *dummy);


/*#######################################################################*/
/*######################### DENC TEST FUNCTIONS #########################*/
/*#######################################################################*/

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
    UNUSED_PARAMETER(dummy);

    Err = ST_NO_ERROR;

    InitDENCParam.DeviceType = DENC_DEVICE_TYPE;
    InitDENCParam.MaxOpen = 2;
    InitDENCParam.AccessType = DENC_DEVICE_ACCESS;

    if (InitDENCParam.AccessType == STDENC_ACCESS_TYPE_EMI)
    {
        InitDENCParam.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_DEVICE_BASE_ADDRESS;
        InitDENCParam.STDENC_Access.EMI.BaseAddress_p       = (void *)DNC_BASE_ADDRESS;
        InitDENCParam.STDENC_Access.EMI.Width               = DENC_DEVICE_ACCESS_WIDTH;
    }
    else /* I2C driver for STV0119 on mb295 */
    {
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        strcpy(InitDENCParam.STDENC_Access.I2C.DeviceName, STI2C_BACK_DEVICE_NAME);
        InitDENCParam.STDENC_Access.I2C.OpenParams.I2cAddress = I2C_STV119_ADDRESS;
        InitDENCParam.STDENC_Access.I2C.OpenParams.AddressType = STI2C_ADDRESS_7_BITS;
        InitDENCParam.STDENC_Access.I2C.OpenParams.BusAccessTimeOut = STI2C_TIMEOUT_INFINITY;
#endif  /* USE_I2C USE_STDENC_I2C */
    }

    Err = STDENC_Init( DRIVER_DEVICE_NAME, &InitDENCParam);


    TermDENCParam.ForceTerminate = FALSE;
    Err = STDENC_Term( DRIVER_DEVICE_NAME, &TermDENCParam);

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
    UNUSED_PARAMETER(dummy);
    Err = ST_NO_ERROR;


    InitDENCParam.DeviceType = DENC_DEVICE_TYPE;
    InitDENCParam.MaxOpen = 2;
    InitDENCParam.AccessType = DENC_DEVICE_ACCESS;

    if (InitDENCParam.AccessType == STDENC_ACCESS_TYPE_EMI)
    {
        InitDENCParam.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_DEVICE_BASE_ADDRESS;
        InitDENCParam.STDENC_Access.EMI.BaseAddress_p       = (void *)DNC_BASE_ADDRESS;
        InitDENCParam.STDENC_Access.EMI.Width               = DENC_DEVICE_ACCESS_WIDTH;
    }
    else /* I2C driver for STV0119 on mb295 */
    {
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        strcpy(InitDENCParam.STDENC_Access.I2C.DeviceName, STI2C_BACK_DEVICE_NAME);
        InitDENCParam.STDENC_Access.I2C.OpenParams.I2cAddress = I2C_STV119_ADDRESS;
        InitDENCParam.STDENC_Access.I2C.OpenParams.AddressType = STI2C_ADDRESS_7_BITS;
        InitDENCParam.STDENC_Access.I2C.OpenParams.BusAccessTimeOut = STI2C_TIMEOUT_INFINITY;
#endif /* USE_I2C USE_STDENC_I2C  */
    }

    Err |= STDENC_Init( DRIVER_DEVICE_NAME, &InitDENCParam);

    Err |= STDENC_Open( DRIVER_DEVICE_NAME, &OpenDENCParam, &DENCHandle);

    ModeDenc.Mode = STDENC_MODE_PALBDGHI;
    ModeDenc.Option.Pal.SquarePixel = FALSE;
    ModeDenc.Option.Pal.Interlaced = TRUE;
    Err = STDENC_SetEncodingMode(DENCHandle, &ModeDenc);

    ModeDenc.Mode = STDENC_MODE_NTSCM;
    ModeDenc.Option.Ntsc.SquarePixel = FALSE;
    ModeDenc.Option.Ntsc.Interlaced = TRUE;
    ModeDenc.Option.Ntsc.FieldRate60Hz = FALSE;

    Err |= STDENC_GetCapability(DRIVER_DEVICE_NAME, &DENCCapability);

    Err |= STDENC_SetEncodingMode(DENCHandle, &ModeDenc);
    ModeOption.Option = STDENC_OPTION_CHROMA_DELAY;
    ModeOption.Value.ChromaDelay = 0;
    Err |= STDENC_SetModeOption(DENCHandle, &ModeOption);

    Err |= STDENC_GetModeOption(DENCHandle, &ModeOption);

    Err |= STDENC_SetColorBarPattern(DENCHandle,TRUE);

    Err |= STDENC_Close(DENCHandle);

    TermDENCParam.ForceTerminate = FALSE;
    Err |= STDENC_Term( DRIVER_DEVICE_NAME, &TermDENCParam);

}


void test_overhead(void *dummy)
{
    UNUSED_PARAMETER(dummy);
    Err = ST_NO_ERROR;
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
#if !defined(ST_OSLINUX) /* due to task status not yet implemented. To be done. */
static BOOL metrics_Stack_Test(STTST_Parse_t *pars_p, char *result_sym_p)
{
    task_t *task_p;
    task_status_t status;
    int overhead_stackused;
    char funcname[4][20]= {
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

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
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

        /* Report Error */
        if(Err)
            printf("An error occured during the process !!!\n");

        /* store overhead value */
        if (i==0)
        {
            printf("*-----------------------------------*\n");
            overhead_stackused = status.task_stack_used;
            printf("%s \t func=0x%08lx stack = %d bytes used\n", &funcname[i][0], (long) func,
                    status.task_stack_used);
            printf("*-----------------------------------*\n");
        }
        else
        {
            printf("%s \t func=0x%08lx stack = %d bytes used (%d - %d overhead)\n", &funcname[i][0], (long) func,
                    status.task_stack_used-overhead_stackused,status.task_stack_used,overhead_stackused);
        }
        /* Tidy up */
        task_delete(task_p);
    }
    printf("*************************************\n");
    printf("*    Stack usage calculation end    *\n");
    printf("*************************************\n");
    return(FALSE);
}

#endif /* Linux porting */
#endif /* ST_OS20 */

/*******************************************************************************
Name        : DENC_NullPointersTest
Description : Bad parameter test : function call with null pointer
             (C code program because macro are not convenient for this test)
Parameters  : *pars_p, *result_sym_p
Assumptions :
Limitations :
Returns     : TRUE if error, FALSE if success
*******************************************************************************/

static BOOL DENC_NullPointersTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 Error = 0;
    ST_ErrorCode_t ErrorCode;
    STDENC_InitParams_t InitParams;
    STDENC_OpenParams_t OpenParams;
    STDENC_TermParams_t TermParams;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    /* Init params NULL */
    ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, NULL);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tInit params NULL : Failed !!!!\n"));
    }

    /* Init device type unknown */
    InitParams.DeviceType = -1;
    ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, &InitParams);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tInit device type unknown : Failed !!!!\n"));
    }

    /* Init access type unknown */
    InitParams.DeviceType = DENC_DEVICE_TYPE;
    InitParams.AccessType = -1;
    ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, &InitParams);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tInit access type unknown : Failed !!!!\n"));
    }

    /* Init access Address null for RBUS only */
    InitParams.AccessType = DENC_DEVICE_ACCESS;
    if(InitParams.AccessType == STDENC_ACCESS_TYPE_EMI)
    {
        InitParams.STDENC_Access.EMI.BaseAddress_p = (void*)NULL;
        ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, &InitParams);
        if(ErrorCode != ST_ERROR_BAD_PARAMETER){
            Error += 1;
            STTST_Print(("\tInit base address null: Failed !!!!\n"));
        }
        InitParams.STDENC_Access.EMI.BaseAddress_p       = (void*)DNC_BASE_ADDRESS;
        InitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void*)DNC_DEVICE_BASE_ADDRESS;
        InitParams.STDENC_Access.EMI.Width               = -1;
        ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, &InitParams);
        if(ErrorCode != ST_ERROR_BAD_PARAMETER){
            Error += 1;
            STTST_Print(("\tInit width unknown : Failed !!!!\n"));
        }
        InitParams.STDENC_Access.EMI.Width = DENC_DEVICE_ACCESS_WIDTH;
    }
    else
    {
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        sprintf(InitParams.STDENC_Access.I2C.DeviceName, STI2C_BACK_DEVICE_NAME);
        InitParams.STDENC_Access.I2C.OpenParams.I2cAddress = I2C_STV119_ADDRESS;
        InitParams.STDENC_Access.I2C.OpenParams.AddressType = STI2C_ADDRESS_7_BITS;
        InitParams.STDENC_Access.I2C.OpenParams.BusAccessTimeOut = STI2C_TIMEOUT_INFINITY;
#endif /* USE_I2C USE_STDENC_I2C */
    }

    /* Init Normaly */
    InitParams.MaxOpen = 1;
    ErrorCode = STDENC_Init(STDENC_DEVICE_NAME, &InitParams);
    if(ErrorCode != ST_NO_ERROR){
        Error += 1;
        STTST_Print(("\tInit Normaly : Failed !!!!\n"));
    }

    /* Open open params null */
    ErrorCode = STDENC_Open(STDENC_DEVICE_NAME, NULL, &DENCHandle);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tOpen open params null : Failed !!!!\n"));
    }

    /* Open handle address null */
    ErrorCode = STDENC_Open(STDENC_DEVICE_NAME, &OpenParams, NULL);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tOpen handle address null : Failed !!!!\n"));
    }

    /* Open normaly */
    ErrorCode = STDENC_Open(STDENC_DEVICE_NAME, &OpenParams, &DENCHandle);
    if(ErrorCode != ST_NO_ERROR){
        Error += 1;
        STTST_Print(("\tOpen normaly : Failed !!!!\n"));
    }

    /* Set mode params null */
    ErrorCode = STDENC_SetEncodingMode(DENCHandle, NULL);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tSet mode params null : Failed !!!!\n"));
    }

    /* Get mode params null */
    ErrorCode = STDENC_GetEncodingMode(DENCHandle, NULL);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tGet mode params null : Failed !!!!\n"));
    }

    /* Get Capability params null */
    ErrorCode = STDENC_GetCapability(STDENC_DEVICE_NAME, NULL);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tGet Capability params null : Failed !!!!\n"));
    }

    /* Close normaly */
    ErrorCode = STDENC_Close(DENCHandle);
    if(ErrorCode != ST_NO_ERROR){
        Error += 1;
        STTST_Print(("\tClose normaly : Failed !!!!\n"));
    }

    /* Term params null */
    ErrorCode = STDENC_Term(STDENC_DEVICE_NAME, NULL);
    if(ErrorCode != ST_ERROR_BAD_PARAMETER){
        Error += 1;
        STTST_Print(("\tTerm params null : Failed !!!!\n"));
    }

    /* Term normally */
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STDENC_Term(STDENC_DEVICE_NAME, &TermParams);
    if(ErrorCode != ST_NO_ERROR){
        Error += 1;
        STTST_Print(("\tTerm normally : Failed !!!!\n"));
    }

    /* Report */
    if(Error == 0)
        STTST_Print(("DENC_NullPointersTest() : Ok\n"));
    else
        STTST_Print(("DENC_NullPointersTest() : Failed !!!!\n"));


    STTST_AssignInteger("ERRORCODE", Error, FALSE);

    return(API_EnableError ? Error : FALSE);
} /* end of DENC_NullPointersTest() */

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of test register command
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart(void)
{

    BOOL RetErr = FALSE;

    RetErr |= STTST_RegisterCommand( "DENC_NullPt", DENC_NullPointersTest, \
                                     "Call API functions with null pointers (bad param. test)");
#ifdef ST_OS20
    RetErr |= STTST_RegisterCommand( "DENC_StackUse" , metrics_Stack_Test, "");
#endif /* ST_OS20 */

    if (RetErr)
    {
        sprintf( Test_Msg,  "DENC_TestCommand() \t: failed !\n");
    }
    else
    {
        sprintf( Test_Msg,  "DENC_TestCommand() \t: ok\n");
    }
    TEST_PRINT(Test_Msg);

    return(!RetErr);
}


/*#########################################################################
 *                                 MAIN
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : os20_main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void os20_main(void *ptr)
{
    UNUSED_PARAMETER(ptr);
    STAPIGAT_Init();
    STAPIGAT_Term();

} /* end os20_main */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{

UNUSED_PARAMETER(argc);
UNUSED_PARAMETER(argv);

#ifdef ST_OS21
	#ifdef ST_OSWINCE
		SetFopenBasePath("/dvdgr-prj-stdenc/tests/src/objs/ST40");
	#endif

    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
#endif

    os20_main(NULL);

    printf ("\n --- End of main 1--- \n");
    fflush (stdout);

    exit (0);
}
/* end of denc_test.c */




