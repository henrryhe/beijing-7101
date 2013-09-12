/************************************************************************

File name   : vout_test.c
Description : VOUT macros
Note        : All functions return TRUE if error for CLILIB compatibility

COPYRIGHT (C) STMicroelectronics 2003.

Date          Modification                                    Initials
----          ------------                                    --------
31 Aug 2000   Creation                                        JG
24 Apr 2002   Add 5516/7020 support                           HSdLM
16 Dec 2002   Update for 5517                                 BS
13 May 2003   Add 7020stem support                            HSdLM
04 Jun 2003   Add 5528 support                                HSdLM
15 Jun 2004   Add 7710 support                                AC
13 Sep 2004   Add ST40/OS21 support                           MH
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"
#include "stdevice.h"

#include "stsys.h"

#include "sttbx.h"

#include "testtool.h"
#include "stdenc.h"
#include "stvtg.h"
#include "stvout.h"
#ifdef __FULL_DEBUG__
#include "../../src/vout_drv.h"
#endif

#include "startup.h"
#include "api_cmd.h"
#include "denc_cmd.h"
#include "vtg_cmd.h"
#include "vout_cmd.h"

/* Private Types ------------------------------------------------------------ */

typedef U8 halvout_DencVersion_t;

typedef struct TaskParams_s
{
    STVOUT_InitParams_t VOUTInitParams;
    STVOUT_Capability_t VOUTCapability;
    STVOUT_OpenParams_t VOUTOpenParams;
    STVOUT_Handle_t VOUTHndl;
    STVOUT_OutputParams_t OutParam;
    STVOUT_TermParams_t TermVOUTParam;
} TaskParams_t;

/* Private Constants -------------------------------------------------------- */

/* check STVOUT_MAX_DEVICE in ../../src/vout_drv.h */

#define VOUT_NO_MEMORY_TEST_MAX_INIT 11  /* STVOUT_MAX_DEVICE + 1 */

#if defined(ST_5510) || defined(ST_5512) || defined(ST_5508) || defined(ST_5518) || defined(ST_5516) || \
    ((defined(ST_5514) || defined(ST_5517)) && !defined(ST_7020))

#define VOUT_BASE_ADDRESS         0
#define VOUT_DEVICE_BASE_ADDRESS  0
#if defined (STVOUT_DACS_ENHANCED)
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_DENC_ENHANCED
#else
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_DENC
#endif
#define DIGOUT_BASE_ADDRESS       VIDEO_BASE_ADDRESS

#if defined(ST_5514)||defined (ST_5516)|| defined(ST_5517)
#define DVO_TESTS
#endif

#elif defined(ST_5528)
/* VOUT_BASE_ADDRESS defined in chip */
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_5528
#define DIGOUT_BASE_ADDRESS       DVO_BASE_ADDRESS
#define DVO_TESTS

#elif defined(ST_5100)
/* VOUT_BASE_ADDRESS defined in chip */
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13
#define DIGOUT_BASE_ADDRESS       DVO_BASE_ADDRESS
#define DVO_TESTS

#elif defined(ST_5105)|| defined(ST_5107)|| defined(ST_5162)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13

#elif defined(ST_5188)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_5525

#elif defined(ST_5525)
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_5525

#elif defined(ST_5301)
/* VOUT_BASE_ADDRESS defined in chip */
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_V13
#define DIGOUT_BASE_ADDRESS       DVO_BASE_ADDRESS
#define DVO_TESTS

#elif defined(ST_7015)
#define VOUT_BASE_ADDRESS         ST7015_DSPCFG_OFFSET
#define VOUT_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7015

#elif defined(ST_7020)
#define VOUT_BASE_ADDRESS         ST7020_DSPCFG_OFFSET
#define VOUT_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7020

#elif defined (ST_7710)
/*VOUT_BASE_ADDRESS defined in chip*/
#define VOUT_BASE_ADDRESS         ST7710_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7710
#define VOUT_BASE_ADDRESS1        0x20103400
#define VOUT_BASE_ADDRESS2        0x20103200

#elif defined (ST_7100)
/*VOUT_BASE_ADDRESS defined in chip*/
#define VOUT_BASE_ADDRESS         ST7100_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7100
#define VOUT_BASE_ADDRESS1        0xB9005800
#define VOUT_BASE_ADDRESS2        0xB9005400

#elif defined (ST_7109)
/*VOUT_BASE_ADDRESS defined in chip*/
#define VOUT_BASE_ADDRESS         ST7109_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7100
#define VOUT_BASE_ADDRESS1        0xB9005800
#define VOUT_BASE_ADDRESS2        0xB9005400

#elif defined (ST_7111)
#define VOUT_BASE_ADDRESS         ST7111_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7111
#define VOUT_BASE_ADDRESS1        0xFd106000
#define VOUT_BASE_ADDRESS2        0xFd106800
#define HD_TVOUT_HDF_BASE_ADDRESS ST7111_HD_TVOUT_HDF_BASE_ADDRESS

#elif defined (ST_7105)
#define VOUT_BASE_ADDRESS         ST7105_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7105
#define VOUT_BASE_ADDRESS1        0xFd106000
#define VOUT_BASE_ADDRESS2        0xFd106800
#define HD_TVOUT_HDF_BASE_ADDRESS ST7105_HD_TVOUT_HDF_BASE_ADDRESS

#elif defined (ST_7200)
#define VOUT_BASE_ADDRESS         ST7200_VOUT_BASE_ADDRESS
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_7200
#define VOUT_BASE_ADDRESS1        0xFd106000
#define VOUT_BASE_ADDRESS2        0xFd106800
#define HD_TVOUT_HDF_BASE_ADDRESS ST7200_HD_TVOUT_HDF_BASE_ADDRESS

#elif defined(ST_GX1)
#define VOUT_BASE_ADDRESS_DENCIF1 0xBB440010
#define VOUT_BASE_ADDRESS_DENCIF2 0xBB440020
#define VOUT_BASE_ADDRESS         VOUT_BASE_ADDRESS_DENCIF2
#define VOUT_DEVICE_BASE_ADDRESS  0
#define VOUT_DEVICE_TYPE          STVOUT_DEVICE_TYPE_GX1

#else
#error ERROR!!! target not supported !!!
#endif

#if defined (mb376) || defined (espresso)
    #ifdef ST_OS21
    #define ST4629_BASE_ADDRESS STI4629_ST40_BASE_ADDRESS
    #endif /* ST_OS21 */
    #ifdef ST_OS20
    #define ST4629_BASE_ADDRESS  STI4629_BASE_ADDRESS
    #endif /* ST_OS20 */
    #ifdef ST_OSLINUX
    #define ST4629_BASE_ADDRESS  0
    #endif /* ST_OS20 */

    #include "sti4629.h"
    #define STI4629_DNC_BASE_ADDRESS ST4629_DENC_OFFSET
#endif /* mb376-espresso */

/* Private Variables (static)------------------------------------------------ */

static char VOUT_Msg[255];            /* text for trace */
static STVOUT_OutputParams_t Str_OutParam;

/* Global Variables --------------------------------------------------------- */

#ifdef __FULL_DEBUG__
stvout_Unit_t *stvout_DbgPtr;
stvout_Device_t *stvout_DbgDev;
#endif
extern BOOL VoutTest_PrintDotsIfOk;
/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif/*TRACE_DYNAMIC_DATASIZE*/
/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
static BOOL VOUT_NullPointersTest(   STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_BadParameterTest(   STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_InitDencError(     STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_TestLimitInit(      STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VOUT_IncompatibleOut(    STTST_Parse_t *pars_p, char *result_sym_p );

#ifdef ST_OS20
static void ReportError(int Error, char *FunctionName);

static void test_overhead(void *dummy);
static void test_typical(void *dummy);
#endif /*ST_OS20*/

int puiss2(int n);
int shift( int n);
BOOL VOUT2_CmdStart(void);
void os20_main(void *ptr);
BOOL Test_CmdStart(void);

/* Functions ---------------------------------------------------------------- */
#ifdef ST_OS20
static void ReportError(int Error, char *FunctionName)
{
    if ((Error) != ST_NO_ERROR)
    {
        printf( "ERROR: %s returned 0x%0x\n", FunctionName, Error );
    }
}
/*******************************************************************************
Name        : test_typical
Description : calculates the stack usage made by the driver for its typical
              conditions of use
Parameters  : None
Assumptions : !!!! STDENC INITIALIZATION MUST BE DONE BEFORE !!!
Limitations : Make sure to not define local variables within the function
              but use module static gloabls instead in order to not pollute the
              stack usage calculation.
Returns     : None
*******************************************************************************/
void test_typical(void *dummy)
{

    ST_ErrorCode_t Err;
    TaskParams_t *Params_p;
    char string[50] = "";

    Params_p = (TaskParams_t *) dummy;
    Err = ST_NO_ERROR;
    Params_p->VOUTInitParams.DeviceType      = VOUT_DEVICE_TYPE;
    Params_p->VOUTInitParams.CPUPartition_p  = DriverPartition_p;
    Params_p->VOUTInitParams.MaxOpen         = 3;
    Params_p->VOUTInitParams.OutputType      = STVOUT_OUTPUT_ANALOG_CVBS;

#if defined (ST_7020) || defined (ST_GX1)
    Params_p->VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
    Params_p->VOUTInitParams.Target.GenericCell.BaseAddress_p       = (void*)VOUT_BASE_ADDRESS;
    strcpy( Params_p->VOUTInitParams.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
#elif defined (ST_7710)|| defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5188)|| defined(ST_5107)|| defined(ST_5162)
    Params_p->VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
    Params_p->VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void*)VOUT_BASE_ADDRESS;
    #if defined (ST_7710)|| defined(ST_5188)|| defined(ST_5525)|| defined(ST_5105)|| defined(ST_5107)|| defined(ST_5162)
    Params_p->VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
    #else
    Params_p->VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_6;
    #endif
    strcpy( Params_p->VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
#elif defined (STVOUT_DACS_ENHANCED)
    strcpy( Params_p->VOUTInitParams.Target.EnhancedCell.DencName, STDENC_DEVICE_NAME);
    Params_p->VOUTInitParams.Target.EnhancedCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
#else /* other 55xx omega1, 7015. */
    strcpy( Params_p->VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
#endif

    Err = STVOUT_Init( STVOUT_DEVICE_NAME, &(Params_p->VOUTInitParams));
    sprintf(string ,"STVOUT_Init");
    ReportError(Err, string);


    Err = STVOUT_GetCapability(STVOUT_DEVICE_NAME, &(Params_p->VOUTCapability));
    sprintf(string ,"STVOUT_GetCapability");
    ReportError(Err, string);

    Err = STVOUT_Open(STVOUT_DEVICE_NAME, &(Params_p->VOUTOpenParams), &(Params_p->VOUTHndl));
    sprintf(string ,"STVOUT_Open");
    ReportError(Err, string);

    Err = STVOUT_Enable( Params_p->VOUTHndl);
    sprintf(string , "STVOUT_Enable");
    ReportError(Err, string);

    Params_p->OutParam.Analog.StateAnalogLevel  = STVOUT_PARAMS_DEFAULT;
    Params_p->OutParam.Analog.StateBCSLevel     = STVOUT_PARAMS_DEFAULT;
    Params_p->OutParam.Analog.StateChrLumFilter = STVOUT_PARAMS_DEFAULT;
    Params_p->OutParam.Analog.EmbeddedType      = FALSE;
    Params_p->OutParam.Analog.InvertedOutput    = FALSE;
    Params_p->OutParam.Analog.SyncroInChroma    = FALSE;
    Params_p->OutParam.Analog.ColorSpace        = STVOUT_ITU_R_601;
    Err = STVOUT_SetOutputParams(Params_p->VOUTHndl, &(Params_p->OutParam));

    sprintf(string , "STVOUT_SetOutputParams");
    ReportError(Err, string);

    Err = STVOUT_GetOutputParams(Params_p->VOUTHndl, &(Params_p->OutParam));
    sprintf(string , "STVOUT_GetOutputParams");
    ReportError(Err, string);

    Err = STVOUT_Disable( Params_p->VOUTHndl);
    sprintf(string , "STVOUT_Disable");
    ReportError(Err, string);

    Err = STVOUT_Close(Params_p->VOUTHndl);
    sprintf(string , "STVOUT_Close");
    ReportError(Err, string);

    Params_p->TermVOUTParam.ForceTerminate = FALSE;
    Err = STVOUT_Term( STVOUT_DEVICE_NAME, &(Params_p->TermVOUTParam));
    sprintf(string , "STVOUT_Term");
    ReportError(Err, string);

}

void test_overhead(void *dummy)
{
    ST_ErrorCode_t Err;
    TaskParams_t *Params_p;
    char string[50] = "";
    Err = ST_NO_ERROR;

    UNUSED_PARAMETER(dummy);
    Params_p = NULL;

    sprintf(string ,"test_overhead" );
    ReportError(Err, string);
}
#endif /*ST_OS20*/

/*-------------------------------------------------------------------------
 * Function : VOUT_DotsOutIfOk
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_DotsOutIfOk( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    int LVar;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 1) )
    {
        STTST_TagCurrentLine( pars_p, "Expecting a boolean !");
        RetErr = TRUE;
    }
    else      /* RetErr == FALSE  */
    {
        VoutTest_PrintDotsIfOk = LVar;
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end of VOUT_DotsOutIfOk() */


/*-------------------------------------------------------------------------
 * Function : VOUT_InitDencError
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * VOUT_InitDencError is used to test STVOUT_ERROR_DENC_ACCESS error codes
 * ----------------------------------------------------------------------*/
static BOOL VOUT_InitDencError( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVOUT_InitParams_t VOUTInitParams;
    char DeviceName[10];
    BOOL RetErr = FALSE;
    STVOUT_OutputType_t Out = STVOUT_OUTPUT_ANALOG_CVBS;
    STVOUT_DeviceType_t Device = STVOUT_DEVICE_TYPE_DENC;
    ST_ErrorCode_t ErrorCode;

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    DeviceName[0] = '\0';
    sprintf(DeviceName, STVOUT_DEVICE_NAME);
    VOUTInitParams.DeviceType      = Device;
    VOUTInitParams.CPUPartition_p  = DriverPartition_p;
    VOUTInitParams.MaxOpen         = 3; /* MaxOpen */
    strcpy( VOUTInitParams.Target.DencName, "XXX");
    VOUTInitParams.OutputType = Out;
/* init */

    sprintf(VOUT_Msg, "STVOUT Init (%s, dev %d, out %d, maxopen 3) :", DeviceName, Device, Out);
    ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);

    RetErr = VOUT_TextError(ErrorCode, VOUT_Msg);
    return ( API_EnableError ? RetErr : FALSE );
} /* end VOUT_InitDencError */


/*-------------------------------------------------------------------------
 * Function : VOUT_TestLimitInit
 * Description : test limit ST_ERROR_NO_MEMORY :
 *  (C code program because macro are not convenient for this test)
 *  this code is not return for outputs on denc, because denc open limitations
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_TestLimitInit(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STVOUT_InitParams_t VOUTInitParams;
    STVOUT_TermParams_t VOUTTermParams;
    BOOL RetErr;
    U16 NbErr,i;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    ST_ErrorCode_t ErrorCodeTerm = ST_NO_ERROR;
    int LVar;
    BOOL DencTest = FALSE;
    char VoutName[10];

    STTBX_Print(( "Call Init function  ...\n" ));
    NbErr = 0;

    /* get device type (default: 1=55xx) */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr || (LVar < 1 || LVar > 13) )
    {
        STTST_TagCurrentLine( pars_p, "Expected device type 1=DENC 2=7015 3=GX1 4=DigOut 5=5528 6=4629 7=DigOut 8=DencEnhanced 9=5100 10=7710 11=7100 12=5525 13=7200");
        RetErr = TRUE;
    }
    else      /* RetErr == FALSE  */
    {
        VOUTInitParams.DeviceType      = (STVOUT_DeviceType_t)(LVar-1);
        VOUTInitParams.CPUPartition_p  = DriverPartition_p;
        VOUTInitParams.MaxOpen         = 1;
        switch ( VOUTInitParams.DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /*no break*/
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /*no break*/
            case STVOUT_DEVICE_TYPE_GX1 :  /*no break*/
            case STVOUT_DEVICE_TYPE_5528 : /*no break*/
            case STVOUT_DEVICE_TYPE_5525 : /*no break*/
            case STVOUT_DEVICE_TYPE_V13 :  /*no break*/
            case STVOUT_DEVICE_TYPE_4629 :
                DencTest = TRUE;
                STTBX_Print(( " test not available \n" ));
                STTST_AssignInteger( result_sym_p, NbErr, FALSE);
                RetErr = FALSE;
                break;
            case STVOUT_DEVICE_TYPE_7015 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.HdsvmCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                break;
            case STVOUT_DEVICE_TYPE_7020 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.GenericCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                break;
           case STVOUT_DEVICE_TYPE_7710 : /* no break */
           case STVOUT_DEVICE_TYPE_7100 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                break;
           case STVOUT_DEVICE_TYPE_7200 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                break;
            #ifdef DVO_TESTS
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED;
                VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = NULL;
                VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void *)DIGOUT_BASE_ADDRESS;
                break;
            #endif /* DVO_TESTS */

            default :
                break;
        }
        if ( !DencTest )
        {
            /* artificial method to get VOUT_NO_MEMORY_TEST_MAX_INIT Init :
            use different DeviceBaseAddress_p */
            /* the last init must generate a ST_ERROR_NO_MEMORY */
            for (i=0; (i < VOUT_NO_MEMORY_TEST_MAX_INIT) && (ErrorCode == ST_NO_ERROR); i++ )
            {
                if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015)
                {
                    VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)(0x40 + i*16);
                }
                sprintf(VoutName, "Vout%d", i);
                ErrorCode = STVOUT_Init( VoutName, &VOUTInitParams);

            }
            if ( ErrorCode != ST_ERROR_NO_MEMORY )
            {
                STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
                NbErr++;
            }
            for (i=0; (i < (VOUT_NO_MEMORY_TEST_MAX_INIT-1)) && (ErrorCodeTerm == ST_NO_ERROR); i++ )
            {
                sprintf(VoutName, "Vout%d", i);
                VOUTTermParams.ForceTerminate = 0;
                ErrorCodeTerm = STVOUT_Term( VoutName, &VOUTTermParams);
            }
            if ( ErrorCodeTerm != ST_NO_ERROR)
            {
                STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCodeTerm ));
                NbErr++;
            }

            /* test result */
            if ( NbErr == 0 )
            {
                STTBX_Print(( "### VOUT_TestLimitInit() result : ok, ST_ERROR_NO_MEMORY returned as expected\n" ));
                RetErr = FALSE;
            }
            else
            {
                STTBX_Print(( "### VOUT_TestLimitInit() result : failed ! \n"));
                RetErr = TRUE;
            }
            STTST_AssignInteger( result_sym_p, NbErr, FALSE);
        }
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VOUT_TestLimitInit() */

/*-------------------------------------------------------------------------
 * Function : vout_testGx1IncompatibleOut
 * Description : test limit STVOUT_ERROR_VOUT_INCOMPATIBLE :
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/

static int vout_testGx1IncompatibleOut( BOOL p, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = STVOUT_DEVICE_TYPE_GX1;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    strcpy( InitParams.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
    InitParams.Target.GenericCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
    InitParams.Target.GenericCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;


    ErrorCode = STVOUT_Init( "voutI", &InitParams);
    if (Test)
    {
        if (ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE)
        {
            Err++;
            if (p)
            {
                STTBX_Print((" nI%d",out));
            }
        }
    }
    else
    {
        if (ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE)
        {
            Err++;
            if (p)
            {
                STTBX_Print((" I%d",out));
            }
        }
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}

static int vout_test55xxIncompatibleOut( BOOL p, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;
#if defined (STVOUT_DACS_ENHANCED)
     InitParams.DeviceType      = STVOUT_DEVICE_TYPE_DENC_ENHANCED;
     strcpy( InitParams.Target.EnhancedCell.DencName, STDENC_DEVICE_NAME);
#else
     InitParams.DeviceType      = STVOUT_DEVICE_TYPE_DENC;
     strcpy( InitParams.Target.DencName, STDENC_DEVICE_NAME);
#endif
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;

    ErrorCode = STVOUT_Init( "voutI", &InitParams);
    if (Test)
    {
        if (ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE)
        {
            Err++;
            if (p)
            {
                STTBX_Print((" nI%d",out));
            }
        }
    }
    else
    {
        if (ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE)
        {
            Err++;
            if (p)
            {
                STTBX_Print((" I%d",out));
            }
        }
    }
    if (ErrorCode == ST_NO_ERROR) STVOUT_Term( "voutI", &TermParams);

    return(Err);
}
static int vout_test7015IncompatibleOut( BOOL DoPrint, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = VOUT_DEVICE_TYPE;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    if (( out == STVOUT_OUTPUT_ANALOG_RGB ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YUV ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YC  ) ||
        ( out == STVOUT_OUTPUT_ANALOG_CVBS) )
    {
        strcpy( InitParams.Target.DencName, STDENC_DEVICE_NAME);
    }
    else
    {
        InitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
        InitParams.Target.HdsvmCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
    }
    ErrorCode = STVOUT_Init( "voutI", &InitParams);
    if (Test)
    {
        if ( ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" nI%d",out));
        }
    }
    else
    {
        if ( ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" I%d",out));
        }
    }
    if ( ErrorCode == ST_NO_ERROR )
    {
         STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}

#if defined(mb376) || defined (espresso) ||defined (mb390) ||defined (mb400)||defined (mb436)||defined (mb634)||defined (DTT5107)|| defined(ST_5162)
static int vout_test5528IncompatibleOut( BOOL DoPrint, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = VOUT_DEVICE_TYPE;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    if (( out == STVOUT_OUTPUT_ANALOG_RGB ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YUV ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YC  ) ||
        ( out == STVOUT_OUTPUT_ANALOG_CVBS) )
    {
        strcpy( InitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
    }
    else
    {
        InitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
        InitParams.Target.HdsvmCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
    }


    ErrorCode = STVOUT_Init( "voutI", &InitParams);

    if (Test)
    {
        if ( ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" nI%d",out));
        }
    }
    else
    {
        if ( ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" I%d",out));
        }
    }
    if ( ErrorCode == ST_NO_ERROR )
    {
         STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}
#endif /* mb376 - espresso -mb390*/
#if defined(mb376)|| defined(espresso)
static int vout_test4629IncompatibleOut( BOOL DoPrint, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = STVOUT_DEVICE_TYPE_4629;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    if (( out == STVOUT_OUTPUT_ANALOG_RGB ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YUV ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YC  ) ||
        ( out == STVOUT_OUTPUT_ANALOG_CVBS) )
    {
        strcpy( InitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
    }
    else
    {
        InitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
        InitParams.Target.HdsvmCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
    }

    ErrorCode = STVOUT_Init( "voutI", &InitParams);

    if (Test)
    {
        if ( ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" nI%d",out));
        }
    }
    else
    {
        if ( ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" I%d",out));
        }
    }
    if ( ErrorCode == ST_NO_ERROR )
    {
         STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}
#endif /* mb376 || espresso */
static int vout_test5525IncompatibleOut( BOOL DoPrint, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = STVOUT_DEVICE_TYPE_5525;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    if (( out == STVOUT_OUTPUT_ANALOG_RGB ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YUV ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YC  ) ||
        ( out == STVOUT_OUTPUT_ANALOG_CVBS) )
    {
        strcpy( InitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
    }
    else
    {
        InitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
        InitParams.Target.HdsvmCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
    }


    ErrorCode = STVOUT_Init( "voutI", &InitParams);

    if (Test)
    {
        if ( ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" nI%d",out));
        }
    }
    else
    {
        if ( ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" I%d",out));
        }
    }
    if ( ErrorCode == ST_NO_ERROR )
    {
         STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}
static int vout_test7710IncompatibleOut( BOOL DoPrint, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = VOUT_DEVICE_TYPE;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    if (( out == STVOUT_OUTPUT_ANALOG_RGB ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YUV ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YC  ) ||
        ( out == STVOUT_OUTPUT_ANALOG_CVBS))
    {
        strcpy( InitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
        InitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
        InitParams.Target.DualTriDacCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
    }
    else
    {
          if ( (out == STVOUT_OUTPUT_HD_ANALOG_RGB)||
               (out == STVOUT_OUTPUT_HD_ANALOG_YUV)||
               (out == STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED )||
               (out == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED ))
          {
            InitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
            InitParams.Target.DualTriDacCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
          }
          /*else
          {

            InitParams.Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
            InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS1;
            InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)VOUT_BASE_ADDRESS2;
          }   */

    }
    InitParams.Target.DualTriDacCell.HD_Dacs   = 0;
    ErrorCode = STVOUT_Init( "voutI", &InitParams);
    if (Test)
    {
        if ( ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" nI%d",out));
        }
    }
    else
    {
        if ( ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" I%d",out));
        }
    }
    if ( ErrorCode == ST_NO_ERROR )
    {
         STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}

static int vout_test7200IncompatibleOut( BOOL DoPrint, STVOUT_OutputType_t out, BOOL Test)
{
    STVOUT_InitParams_t InitParams;
    STVOUT_TermParams_t TermParams;
    U16 Err=0;
    ST_ErrorCode_t ErrorCode;

    TermParams.ForceTerminate = TRUE;

    InitParams.DeviceType      = VOUT_DEVICE_TYPE;
    InitParams.CPUPartition_p  = DriverPartition_p;
    InitParams.MaxOpen         = 1;
    InitParams.OutputType = out;
    if (( out == STVOUT_OUTPUT_ANALOG_RGB ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YUV ) ||
        ( out == STVOUT_OUTPUT_ANALOG_YC  ) ||
        ( out == STVOUT_OUTPUT_ANALOG_CVBS))
    {
        strcpy( InitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
        strcpy( InitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
        InitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
        InitParams.Target.EnhancedDacCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
    }
    else
    {
          if ( (out == STVOUT_OUTPUT_HD_ANALOG_RGB)||
               (out == STVOUT_OUTPUT_HD_ANALOG_YUV)||
               (out == STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED )||
               (out == STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED ))
          {
            strcpy( InitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
            InitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
            InitParams.Target.EnhancedDacCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS;
          }
          /*else
          {

            InitParams.Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
            InitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p = (void *)VOUT_BASE_ADDRESS1;
            InitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)VOUT_BASE_ADDRESS2;
          }   */

    }
    InitParams.Target.EnhancedDacCell.HD_Dacs   = 0;
    ErrorCode = STVOUT_Init( "voutI", &InitParams);
    if (Test)
    {
        if ( ErrorCode != STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" nI%d",out));
        }
    }
    else
    {
        if ( ErrorCode == STVOUT_ERROR_VOUT_INCOMPATIBLE )
        {
            Err++;
            if (DoPrint) STTBX_Print((" I%d",out));
        }
    }
    if ( ErrorCode == ST_NO_ERROR )
    {
         STVOUT_Term( "voutI", &TermParams);
    }
    return(Err);
}


static BOOL VOUT_IncompatibleOut(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STVOUT_InitParams_t VOUTInitParams;
    STVOUT_TermParams_t VOUTTermParams;
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    int LVar;
    BOOL printoption = FALSE;

    STTBX_Print(( "Call Init function  ...\n" ));
    NbErr = 0;

    /* get device type (default: 1=55xx) */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr || (LVar < 1 || LVar > 13) )
    {
        STTST_TagCurrentLine( pars_p, "expected device type (1-13)\n" );
        RetErr = TRUE;
    }
    else      /* RetErr == FALSE  */
    {
        VOUTInitParams.DeviceType = (STVOUT_DeviceType_t)(LVar-1);

        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr || LVar < 0 )
        {
            STTST_TagCurrentLine( pars_p, "expected print option (>0) \n" );
            RetErr = TRUE;
        }
        printoption = (BOOL)LVar;
    }

    if ( !RetErr ) /* RetErr == FALSE  */
    {
        VOUTInitParams.CPUPartition_p  = DriverPartition_p;
        VOUTInitParams.MaxOpen         = 1;

        VOUTTermParams.ForceTerminate = TRUE;
        /* test all incompatible output */
        switch ( VOUTInitParams.DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC :
    /*      RGB  YUV  Y/C  CVBS*/
    /* RGB   pb   pb   ok   ok*/
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* YUV   pb   pb   ok   ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* Y/C   ok   ok   pb   ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* CVBS  ok   ok   ok   pb*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE );
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));
                break;
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
     /*      RGB  YUV  Y/C  CVBS*/
    /* RGB   pb   pb   ok   ok*/
                strcpy( VOUTInitParams.Target.EnhancedCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                VOUTInitParams.Target.EnhancedCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_4|STVOUT_DAC_5|STVOUT_DAC_6);
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* YUV   pb   pb   ok   ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* Y/C   ok   ok   pb   ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                VOUTInitParams.Target.EnhancedCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_1|STVOUT_DAC_2);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* CVBS  ok   ok   ok   pb*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                VOUTInitParams.Target.EnhancedCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_3);
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                NbErr += vout_test55xxIncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE );
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));
                break;

            case STVOUT_DEVICE_TYPE_7015 : /* no break */
/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*    RGB  pb   pb    pb   pb     ok     ok     ok    ok    ok    ok    ok*/
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.HdsvmCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*    YUV  pb   pb    pb   pb     ok     ok     ok    ok    ok    ok    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*    Y/C  pb   pb    pb   ok     ok     ok     ok    ok    ok    ok    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*   CVBS  pb   pb    ok   pb     ok     ok     ok    ok    ok    ok    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/* HD_RGB  ok   ok    ok   ok     pb     pb     ok    ok    ok    ok    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                ErrorCode = STVOUT_Init( "5hdrgb1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "5hdrgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :5hdrgb1\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/* HD_YUV  ok   ok    ok   ok     pb     pb     ok    ok    ok    ok    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_YUV;
                ErrorCode = STVOUT_Init( "6hdyuv1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "6hdyuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :6hdyuv1\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*   DIG1  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS;
                ErrorCode = STVOUT_Init( "7dig11", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "7dig11", &VOUTTermParams);
                if (printoption) STTBX_Print((" :7dig11\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*   DIG2  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED;
                ErrorCode = STVOUT_Init( "8dig21", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "8dig21", &VOUTTermParams);
                if (printoption) STTBX_Print((" :8dig21\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*   DIG3  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED;
                ErrorCode = STVOUT_Init( "9dig31", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "9dig31", &VOUTTermParams);
                if (printoption) STTBX_Print((" :9dig31\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*   DIG4  ok   ok    ok   ok     ok     ok     pb    pb    pb    pb    ok*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS;
                ErrorCode = STVOUT_Init( "10dig41", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, TRUE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, FALSE);
                STVOUT_Term( "10dig41", &VOUTTermParams);
                if (printoption) STTBX_Print((" :10dig41\n"));

/*        RGB  YUV   Y/C  CVBS  HD_RGB HD_YUV  DIG1  DIG2  DIG3  DIG4  SVM*/
/*    SVM  ok   ok    ok   ok     ok     ok     ok    ok    ok    ok    pb*/
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_SVM;
                ErrorCode = STVOUT_Init( "11svm1", &VOUTInitParams);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS, FALSE);
                NbErr += vout_test7015IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_SVM, TRUE);
                STVOUT_Term( "11svm1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :11svm1\n"));
                break;

            case STVOUT_DEVICE_TYPE_GX1 :
    /*      RGB  YUV  Y/C  CVBS*/
    /* RGB   ok   pb   ok   ok*/
                    strcpy( VOUTInitParams.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.GenericCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                    VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                    ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                    STVOUT_Term( "1rgb1", &VOUTTermParams);
                    if (printoption) STTBX_Print((" :1rgb1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* YUV   pb   ok   ok   ok*/
                    VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                    ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                    VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                    STVOUT_Term( "2yuv1", &VOUTTermParams);
                    if (printoption) STTBX_Print((" :2yuv1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* Y/C   ok   ok   ok   ok*/
                    VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                    ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                    STVOUT_Term( "3yc1", &VOUTTermParams);
                    if (printoption) STTBX_Print((" :3yc1\n"));
    /*      RGB  YUV  Y/C  CVBS*/
    /* CVBS  ok   ok   ok   ok*/
                    VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                    ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE );
                    NbErr += vout_testGx1IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE );
                    STVOUT_Term( "4cvbs1", &VOUTTermParams);
                    if (printoption) STTBX_Print((" :4cvbs1\n"));
                break;

            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT : /* test not relevant */
                break;
            case STVOUT_DEVICE_TYPE_5528 :
            case STVOUT_DEVICE_TYPE_V13  :/* to be written */
#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105) ||defined(ST_5301)|| defined(ST_5107)|| defined(ST_5162)
/*        RGB  YUV   Y/C  CVBS  */
/*    RGB  pb   pb    ok   ok */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*) VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS ;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3);
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*    YUV  pb   pb    ok   ok */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*    Y/C  OK   OK    pb   ok   */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_4|STVOUT_DAC_5);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*   CVBS  OK   OK    OK   pb   */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_6);
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test5528IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));

                break;
#endif /* ST_5528 */
            case STVOUT_DEVICE_TYPE_4629 :
#if defined(mb376) || defined (espresso)
/*        RGB  YUV   Y/C  CVBS  */
/*    RGB  pb   pb    ok   ok */

                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*) STI4629_DNC_BASE_ADDRESS ;
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)ST4629_BASE_ADDRESS ;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3);
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*    YUV  pb   pb    ok   ok */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*    Y/C  OK   OK    pb   ok   */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_1|STVOUT_DAC_2);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*   CVBS  OK   OK    OK   pb   */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_3);
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test4629IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption)
                    STTBX_Print((" :4cvbs1\n"));
#endif /*  mb376 || espresso*/
                break;
            case STVOUT_DEVICE_TYPE_7020 :  /* to be written */
                 break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 :
 /*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*    RGB    pb   pb  pb   pb     pb     pb     */
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));
/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*    YUV    pb   pb  pb   pb     pb     pb     */
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));

/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*    Y/C    pb  pb  pb   pb     pb     pb     */
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));

/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*   CVBS    pb  pb  pb   pb     pb     pb     */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));
/*          RGB YUV Y/C  CVBS  HD_RGB HD_YUV  */
/* HD_RGB   pb  pb  pb   pb     pb     pb    */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "5hdrgb1", &VOUTInitParams);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "5hdrgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :5hdrgb1\n"));

 /*        RGB YUV Y/C  CVBS  HD_RGB HD_YUV  */
/* HD_YUV  pb  pb  pb   pb     pb     pb     */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_YUV;
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "6hdyuv1", &VOUTInitParams);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7710IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "6hdyuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :6hdyuv1\n"));
                break;

            case STVOUT_DEVICE_TYPE_7200 :
/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*    RGB    pb   pb  pb   pb     ok     ok     */
                strcpy( VOUTInitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));
/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*    YUV    pb   pb  pb   pb     ok     ok     */
                strcpy( VOUTInitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));
/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*    Y/C    pb  pb  pb   ok     ok     ok     */
                strcpy( VOUTInitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));

/*           RGB YUV Y/C  CVBS  HD_RGB HD_YUV   */
/*   CVBS    pb  pb  ok   ok     ok     ok     */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, FALSE);
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));
/*          RGB YUV  Y/C  CVBS  HD_RGB HD_YUV  */
/* HD_RGB   ok   ok  ok    ok     pb     pb    */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "5hdrgb1", &VOUTInitParams);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, TRUE);
                STVOUT_Term( "5hdrgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :5hdrgb1\n"));

 /*        RGB YUV Y/C  CVBS  HD_RGB HD_YUV  */
/* HD_YUV  ok  ok  ok   ok     pb     pb     */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_HD_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                ErrorCode = STVOUT_Init( "6hdyuv1", &VOUTInitParams);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_RGB, TRUE);
                NbErr += vout_test7200IncompatibleOut( printoption, STVOUT_OUTPUT_HD_ANALOG_YUV, TRUE);
                STVOUT_Term( "6hdyuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :6hdyuv1\n"));
                break;
            case STVOUT_DEVICE_TYPE_5525 :
 /*        RGB  YUV   Y/C  CVBS  */
/*    RGB  pb   pb    ok   ok */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_RGB;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p = (void*) VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS ;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_1|STVOUT_DAC_2|STVOUT_DAC_3);
                ErrorCode = STVOUT_Init( "1rgb1", &VOUTInitParams);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "1rgb1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :1rgb1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*    YUV  pb   pb    ok   ok */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YUV;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                ErrorCode = STVOUT_Init( "2yuv1", &VOUTInitParams);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, TRUE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, TRUE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "2yuv1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :2yuv1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*    Y/C  OK   OK    pb   ok   */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_YC;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_1|STVOUT_DAC_2);
                ErrorCode = STVOUT_Init( "3yc1", &VOUTInitParams);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, TRUE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, FALSE);
                STVOUT_Term( "3yc1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :3yc1\n"));

/*        RGB  YUV   Y/C  CVBS  */
/*   CVBS  OK   OK    OK   pb   */
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DacSelect           =(STVOUT_DAC_t)(STVOUT_DAC_3);
                ErrorCode = STVOUT_Init( "4cvbs1", &VOUTInitParams);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_RGB, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YUV, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_YC, FALSE);
                NbErr += vout_test5525IncompatibleOut( printoption, STVOUT_OUTPUT_ANALOG_CVBS, TRUE);
                STVOUT_Term( "4cvbs1", &VOUTTermParams);
                if (printoption) STTBX_Print((" :4cvbs1\n"));

                break;
            default:
                break;
        }

        /* test result */
        if ( NbErr == 0 )
        {
            STTBX_Print(( "### VOUT_IncompatibleOut() result : ok, STVOUT_ERROR_VOUT_INCOMPATIBLE returned as expected\n" ));
            RetErr = FALSE;
        }
        else
        {
            STTBX_Print(( "### VOUT_IncompatibleOut() result : failed ! \n"));
            RetErr = TRUE;
        }
        STTST_AssignInteger( result_sym_p, NbErr, FALSE);
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VOUT_IncompatibleOut() */


/*-------------------------------------------------------------------------
 * Function : VOUT_NullPointersTest
 * Description : Bad parameter test : function call with null pointer
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_NullPointersTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    STVOUT_InitParams_t VOUTInitParams;
    STVOUT_OpenParams_t  VOUTOpenParams;
    STVOUT_Handle_t VOUTHndl;
    STVOUT_TermParams_t VOUTTermParams;
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrorCode;
    int LVar;

    STTBX_Print(( "Call API functions with null pointers...\n" ));
    NbErr = 0;

    /* get device type (default: 1=55xx) */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 13) )
    {
        STTST_TagCurrentLine( pars_p, "expected device type (1-13)\n" );
        RetErr = TRUE;
    }
    else      /* RetErr == FALSE  */
    {
        VOUTInitParams.DeviceType      = (STVOUT_DeviceType_t)(LVar-1);
        VOUTInitParams.CPUPartition_p  = DriverPartition_p;
        VOUTInitParams.MaxOpen         = 3;
        switch ( VOUTInitParams.DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC :
            case STVOUT_DEVICE_TYPE_7015 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                break;
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.EnhancedCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                break;
            case STVOUT_DEVICE_TYPE_V13 :
            case STVOUT_DEVICE_TYPE_5528 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_6;
                break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_6;
                VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                break;
            case STVOUT_DEVICE_TYPE_7200 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.EnhancedDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                break;
            case STVOUT_DEVICE_TYPE_7020 :
            case STVOUT_DEVICE_TYPE_GX1 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.GenericCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;

                break;
            case STVOUT_DEVICE_TYPE_4629 :
#if defined(mb376) || defined (espresso)
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)ST4629_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)STI4629_DNC_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
#endif /* mb376 || espresso*/
                break;
#ifdef DVO_TESTS
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
#if defined (ST_5528)|| defined(ST_5100) ||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED;
                VOUTInitParams.Target.DVOCell.DeviceBaseAddress_p = (void *)0;
                VOUTInitParams.Target.DVOCell.BaseAddress_p = (void *)DIGOUT_BASE_ADDRESS;
#else
                VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED;
                VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)0;
                VOUTInitParams.Target.HdsvmCell.BaseAddress_p = (void *)DIGOUT_BASE_ADDRESS;
#endif /* ST_5528 - ST_5100 - ST_5301*/
                break;
#endif /* DVO_TESTS */
           case STVOUT_DEVICE_TYPE_5525 :
                VOUTInitParams.OutputType = STVOUT_OUTPUT_ANALOG_CVBS;
                strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                break;
           default :
                break;
        }

        /* Test STVOUT_Init with a NULL pointer parameter */
        DeviceName[0] = '\0';

        ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVOUT_Init(NULL,...) : unexpected return code=0x%0x !\n", ErrorCode ));
            NbErr++;
        }

        /* Test STVOUT_Init with a NULL pointer parameter */
        sprintf(DeviceName , STVOUT_DEVICE_NAME);
        ErrorCode = STVOUT_Init(DeviceName, NULL);

        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVOUT_Init(%s, NULL) : unexpected return code=0x%0x \n", STVOUT_DEVICE_NAME, ErrorCode ));
            NbErr++;
        }

        /* Init a device to for further tests needing an init device */
        ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
        if ( ErrorCode != ST_NO_ERROR )
        {
            STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
            NbErr++;
        }
        else
        {
            STTBX_Print(( " STVOUT_Init('%s') : ok\n", DeviceName));
            /* Test STVOUT_GetCapability with a NULL pointer parameter */
            ErrorCode = STVOUT_GetCapability("", NULL);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_GetCapability(NULL,...) : unexpected return code=0x%0x \n", ErrorCode ));
                NbErr++;
            }

            /* Test STVOUT_GetCapability with a NULL pointer parameter */
            ErrorCode = STVOUT_GetCapability(STVOUT_DEVICE_NAME, NULL);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_GetCapability(%s, NULL,...) : unexpected return code=0x%0x \n", STVOUT_DEVICE_NAME, ErrorCode ));
                NbErr++;
            }

            /* Test STVOUT_Open with a NULL pointer parameter */
            ErrorCode = STVOUT_Open("", &VOUTOpenParams, &VOUTHndl);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_Open(NULL,...) : unexpected return code=0x%0x \n", ErrorCode ));
                NbErr++;
            }

            /* Test STVOUT_Open with a NULL pointer parameter */
            ErrorCode = STVOUT_Open(STVOUT_DEVICE_NAME, NULL, &VOUTHndl);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_Open(%s, NULL,...) : unexpected return code=0x%0x \n", STVOUT_DEVICE_NAME, ErrorCode ));
                NbErr++;
            }

            /* Test STVOUT_Open with a NULL pointer parameter */
            ErrorCode = STVOUT_Open(STVOUT_DEVICE_NAME, &VOUTOpenParams, NULL);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_Open(%s, ...,NULL) : unexpected return code=0x%0x \n", STVOUT_DEVICE_NAME, ErrorCode ));
                NbErr++;
            }

            /* Open device to get a good handle for further tests */
            ErrorCode = STVOUT_Open(STVOUT_DEVICE_NAME, &VOUTOpenParams, &VOUTHndl);
            if ( ErrorCode != ST_NO_ERROR )
            {
                STTBX_Print(( "  STVOUT_Open() : unexpected return code=0x%0x \n", ErrorCode ));
                NbErr++;
            }
            else
            {
                STTBX_Print(( " STVOUT_Open('%s') : ok\n", DeviceName));
                /* Test STVOUT_SetOutputParams with a NULL pointer parameter */
                ErrorCode = STVOUT_SetOutputParams(VOUTHndl, NULL);
                if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                {
                    STTBX_Print(( "  STVOUT_SetOutputParams(.., NULL) : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }

                /* Test STVOUT_GetOutputParams with a NULL pointer parameter */
                ErrorCode = STVOUT_GetOutputParams(VOUTHndl, NULL);
                if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                {
                    STTBX_Print(( "  STVOUT_GetOutputParams(.., NULL) : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }

                /* Close connection */
                ErrorCode = STVOUT_Close(VOUTHndl);
                if ( ErrorCode != ST_NO_ERROR )
                {
                    STTBX_Print(( "  STVOUT_Close() : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }
            }
            /* Test STVOUT_Term with a NULL pointer parameter */
            ErrorCode = STVOUT_Term("", &VOUTTermParams);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_Term(NULL, ...) : unexpected return code=0x%0x \n", ErrorCode ));
                NbErr++;
            }

            /* Test STVOUT_Term with a NULL pointer parameter */
            ErrorCode = STVOUT_Term(STVOUT_DEVICE_NAME, NULL);
            if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
            {
                STTBX_Print(( "  STVOUT_Term(%s, NULL) : unexpected return code=0x%0x \n", STVOUT_DEVICE_NAME, ErrorCode ));
                NbErr++;
            }
            /* Terminate device use */
            VOUTTermParams.ForceTerminate = 0;
            ErrorCode = STVOUT_Term(STVOUT_DEVICE_NAME, &VOUTTermParams);
            if ( ErrorCode != ST_NO_ERROR )
            {
                STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCode ));
            }
        }

        /* cases of structures containing a null pointer */



        /*  */

        VOUTInitParams.CPUPartition_p  = NULL;
        ErrorCode = STVOUT_Init(STVOUT_DEVICE_NAME, &VOUTInitParams);

        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
        {
            STTBX_Print(( "  STVOUT_Init(..,(CPUPartition_p = NULL)) : unexpected return code=0x%0x \n", ErrorCode ));
            NbErr++;
        }

        /* test result */
        if ( NbErr == 0 )
        {
            STTBX_Print(( "### VOUT_NullPointersTest() result : ok, ST_ERROR_BAD_PARAMETER returned each time as expected\n" ));
            RetErr = FALSE;
        }
        else
        {
            STTBX_Print(( "### VOUT_NullPointersTest() result : failed ! ST_ERROR_BAD_PARAMETER not returned %d times\n", NbErr ));
            RetErr = TRUE;
        }
        STTST_AssignInteger( result_sym_p, NbErr, FALSE);
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VOUT_NullPointersTest() */


/*-------------------------------------------------------------------------
 * Function : VOUT_BadParameterTest
 * Description : Bad parameter test : function call with bad parameter
 *              (C code program because macro are not convenient for this test)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_BadParameterTest(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    STVOUT_InitParams_t VOUTInitParams;
    STVOUT_OpenParams_t VOUTOpenParams;
    STVOUT_Handle_t VOUTHndl;
    STVOUT_TermParams_t VOUTTermParams;
    BOOL RetErr;
    U16 NbErr;
    ST_ErrorCode_t ErrorCode;
    int LVar;
    STVOUT_OutputParams_t *OutParam  = &Str_OutParam;


    STTBX_Print(( "Call API function STVOUT_SetOutputParams() with bad parameter...\n" ));
    NbErr = 0;

    /* get device type (default: 1=55xx) */
    RetErr = STTST_GetInteger( pars_p, 1, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 13) )
    {
        STTST_TagCurrentLine( pars_p, "expected device type (1-13)\n" );
        RetErr = TRUE;
    }
    else      /* RetErr == FALSE  */
    {
        VOUTInitParams.DeviceType      = (STVOUT_DeviceType_t)(LVar-1);
        VOUTInitParams.CPUPartition_p  = DriverPartition_p;
        VOUTInitParams.MaxOpen         = 3;

        sprintf(DeviceName , STVOUT_DEVICE_NAME);

        switch ( VOUTInitParams.DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /* no break */
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /*no break*/
            case STVOUT_DEVICE_TYPE_7015 : /* no break */
            case STVOUT_DEVICE_TYPE_7020 : /* no break */
            case STVOUT_DEVICE_TYPE_5528 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 : /* no break */
            case STVOUT_DEVICE_TYPE_V13  : /* no break */
            case STVOUT_DEVICE_TYPE_4629 : /* no break */
            case STVOUT_DEVICE_TYPE_GX1  : /* no break */
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 : /* no break */
            case STVOUT_DEVICE_TYPE_7200 :
                if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DENC)
                {
                    strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                }
                else if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)
                {
                    strcpy( VOUTInitParams.Target.EnhancedCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.EnhancedCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                }
                else if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7015)
                {
                    strcpy( VOUTInitParams.Target.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.HdsvmCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.HdsvmCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                }
                else if ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7020)
                          ||(VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_GX1))

                {
                    strcpy( VOUTInitParams.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.GenericCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.GenericCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                }
                else if ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_5528)
                         ||(VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_V13))
                {
                    strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_6;
                }
                else if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_5525)
                {
                    strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                }
                else if ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7710) || (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7100))
                {
                    strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_6;
                    VOUTInitParams.Target.DualTriDacCell.HD_Dacs             = 0;
                }
                else if (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7200)
                {
                    strcpy( VOUTInitParams.Target.EnhancedDacCell.DencName, STDENC_DEVICE_NAME);
                    strcpy( VOUTInitParams.Target.EnhancedDacCell.VTGName, STVTG_DEVICE_NAME);
                    VOUTInitParams.Target.EnhancedDacCell.DeviceBaseAddress_p = (void *)VOUT_DEVICE_BASE_ADDRESS;
                    VOUTInitParams.Target.EnhancedDacCell.BaseAddress_p       = (void *)VOUT_BASE_ADDRESS;
                    VOUTInitParams.Target.EnhancedDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
                    VOUTInitParams.Target.EnhancedDacCell.HD_Dacs             = 0;
                }
                else if ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_4629))
                {
#if defined(mb376) || defined (espresso)
                    strcpy( VOUTInitParams.Target.DualTriDacCell.DencName, STDENC_DEVICE_NAME);
                    VOUTInitParams.Target.DualTriDacCell.DeviceBaseAddress_p = (void *)ST4629_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.BaseAddress_p       = (void *)STI4629_DNC_BASE_ADDRESS;
                    VOUTInitParams.Target.DualTriDacCell.DacSelect           = (STVOUT_DAC_t)STVOUT_DAC_3;
#endif /* mb376 || espresso*/
                }
                VOUTInitParams.OutputType      = STVOUT_OUTPUT_ANALOG_CVBS;

                /* Init a device to for further tests needing an init device */
                ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
                if ( ErrorCode != ST_NO_ERROR )
                {
                    STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }
                else
                {
                    STTBX_Print(( " STVOUT_Init('%s') : ok\n", DeviceName));
                    /* Open device to get a good handle for further tests */
                    ErrorCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
                    if ( ErrorCode != ST_NO_ERROR )
                    {
                        STTBX_Print(( "  STVOUT_Open() : unexpected return code=0x%0x \n", ErrorCode ));
                        NbErr++;
                    }
                    else
                    {
                        STTBX_Print(( " STVOUT_Open('%s') : ok\n", DeviceName));
                        OutParam->Analog.StateAnalogLevel  = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.StateBCSLevel     = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.ColorSpace        = STVOUT_SMPTE240M;

                        OutParam->Analog.StateAnalogLevel  = STVOUT_PARAMS_NOT_CHANGED-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 1 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.StateAnalogLevel = STVOUT_PARAMS_AFFECTED+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 2 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.StateAnalogLevel = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 3 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_AFFECTED+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 4 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_NOT_CHANGED-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 5 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_AFFECTED+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 6 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_NOT_CHANGED;

                        /* Close connection */
                        ErrorCode = STVOUT_Close(VOUTHndl);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Close() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        /* Term */
                        ErrorCode = STVOUT_Term(DeviceName, &VOUTTermParams);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                    }
                }
                if (     ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DENC)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_5528)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_4629)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_GX1)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_DENC_ENHANCED)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_V13)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_5525)
                     ||  ( VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7200))
                {
                    break;
                }
                /* next : test digital and svm output type */
                VOUTInitParams.OutputType      = STVOUT_OUTPUT_HD_ANALOG_RGB;

                /* Init a device to for further tests needing an init device */
                ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
                if ( ErrorCode != ST_NO_ERROR )
                {
                    STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }
                else
                {
                    STTBX_Print(( " STVOUT_Init('%s') : ok\n", DeviceName));
                    /* Open device to get a good handle for further tests */
                    ErrorCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
                    if ( ErrorCode != ST_NO_ERROR )
                    {
                        STTBX_Print(( "  STVOUT_Open() : unexpected return code=0x%0x \n", ErrorCode ));
                        NbErr++;
                    }
                    else
                    {
                        STTBX_Print(( " STVOUT_Open('%s') : ok\n", DeviceName));
                        OutParam->Analog.StateAnalogLevel = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.StateBCSLevel = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->Analog.StateChrLumFilter = STVOUT_PARAMS_NOT_CHANGED;

                        OutParam->Analog.ColorSpace = STVOUT_SMPTE240M-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 7 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.ColorSpace = STVOUT_ITU_R_709+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 8 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Analog.ColorSpace = STVOUT_SMPTE240M;

                        /* Close connection */

                        ErrorCode = STVOUT_Close(VOUTHndl);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Close() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        /* Term */
                        ErrorCode = STVOUT_Term(DeviceName, &VOUTTermParams);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                    }
                }
                if ((VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7710) ||
                    (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7100) ||
                    (VOUTInitParams.DeviceType == STVOUT_DEVICE_TYPE_7200))
                {
                    break;
                }

                VOUTInitParams.OutputType      = STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS;

                /* Init a device to for further tests needing an init device */
                ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
                if ( ErrorCode != ST_NO_ERROR )
                {
                    STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }
                else
                {
                    STTBX_Print(( " STVOUT_Init('%s') : ok\n", DeviceName));
                    /* Open device to get a good handle for further tests */
                    ErrorCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
                    if ( ErrorCode != ST_NO_ERROR )
                    {
                        STTBX_Print(( "  STVOUT_Open() : unexpected return code=0x%0x \n", ErrorCode ));
                        NbErr++;
                    }
                    else
                    {
                        STTBX_Print(( " STVOUT_Open('%s') : ok\n", DeviceName));
                        OutParam->Digital.ColorSpace = STVOUT_SMPTE240M-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 9 : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->Digital.ColorSpace = STVOUT_ITU_R_709+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 10: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }

                        /* Close connection */
                        ErrorCode = STVOUT_Close(VOUTHndl);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Close() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        /* Term */
                        ErrorCode = STVOUT_Term(DeviceName, &VOUTTermParams);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                    }
                }

                VOUTInitParams.OutputType      = STVOUT_OUTPUT_ANALOG_SVM;

                /* Init a device to for further tests needing an init device */
                ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
                if ( ErrorCode != ST_NO_ERROR )
                {
                    STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
                    NbErr++;
                }
                else
                {
                    STTBX_Print(( " STVOUT_Init('%s') : ok\n", DeviceName));
                    /* Open device to get a good handle for further tests */
                    ErrorCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
                    if ( ErrorCode != ST_NO_ERROR )
                    {
                        STTBX_Print(( "  STVOUT_Open() : unexpected return code=0x%0x \n", ErrorCode ));
                        NbErr++;
                    }
                    else
                    {
                        STTBX_Print(( " STVOUT_Open('%s') : ok\n", DeviceName));
                        OutParam->SVM.StateAnalogSVM = STVOUT_PARAMS_NOT_CHANGED;
                        OutParam->SVM.DelayCompensation = 0;
                        OutParam->SVM.Shape = STVOUT_SVM_SHAPE_OFF;
                        OutParam->SVM.Gain = 0;
                        OutParam->SVM.OSDFactor = STVOUT_SVM_FACTOR_0;
                        OutParam->SVM.VideoFilter = STVOUT_SVM_FILTER_1;
                        OutParam->SVM.OSDFilter = STVOUT_SVM_FILTER_1;

                        OutParam->SVM.StateAnalogSVM = STVOUT_PARAMS_NOT_CHANGED-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 11: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.StateAnalogSVM = STVOUT_PARAMS_AFFECTED+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 12: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.StateAnalogSVM = STVOUT_PARAMS_AFFECTED;
                        OutParam->SVM.Shape = STVOUT_SVM_SHAPE_OFF-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 13: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.Shape = STVOUT_SVM_SHAPE_3+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 14: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.OSDFactor = STVOUT_SVM_FACTOR_0-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 15: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.OSDFactor = STVOUT_SVM_FACTOR_1+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 16: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.OSDFactor = STVOUT_SVM_FACTOR_0;
                        OutParam->SVM.VideoFilter = STVOUT_SVM_FILTER_1-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 17: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.VideoFilter = STVOUT_SVM_FILTER_2+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 18: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.VideoFilter = STVOUT_SVM_FILTER_1;
                        OutParam->SVM.OSDFilter = STVOUT_SVM_FILTER_1-1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 19: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        OutParam->SVM.OSDFilter = STVOUT_SVM_FILTER_2+1;
                        ErrorCode = STVOUT_SetOutputParams(VOUTHndl, OutParam);
                        if ( ErrorCode != ST_ERROR_BAD_PARAMETER )
                        {
                            STTBX_Print(( "  STVOUT_SetOutputParams() 20: unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }

                        /* Close connection */
                        ErrorCode = STVOUT_Close(VOUTHndl);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Close() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                        /* Term */
                        ErrorCode = STVOUT_Term(DeviceName, &VOUTTermParams);
                        if ( ErrorCode != ST_NO_ERROR )
                        {
                            STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCode ));
                            NbErr++;
                        }
                    }
                }
                break;
            case STVOUT_DEVICE_TYPE_DIGITAL_OUTPUT :
                 break;
        }

        /* test result */
        if ( NbErr == 0 )
        {
            STTBX_Print(( "### VOUT_BadParameterTest() result : ok, ST_ERROR_BAD_PARAMETER returned each time as expected\n" ));
            RetErr = FALSE;
        }
        else
        {
            STTBX_Print(( "### VOUT_BadParameterTest() result : failed ! ST_ERROR_BAD_PARAMETER not returned %d times\n", NbErr ));
            RetErr = TRUE;
        }
        STTST_AssignInteger( result_sym_p, NbErr, FALSE);
    }

    return(API_EnableError ? RetErr : FALSE );

} /* end of VOUT_BadParameterTest() */

#if defined (ST_7710)|| defined (ST_7100) || defined(ST_7109)
/*-------------------------------------------------------------------------
 * Function : VOUT_TestMachine
 * Description : Tests the transition between the different states in the state
 *              machine.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VOUT_TestStateMachine(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    STVOUT_InitParams_t VOUTInitParams;
    STVOUT_OpenParams_t VOUTOpenParams;
    STVOUT_Handle_t VOUTHndl;
    STVOUT_TermParams_t VOUTTermParams;
    STVOUT_State_t State;
    STVOUT_TargetInformation_t TargetInfo;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    U16 NbErr;
    int LVar;


    STTBX_Print(( "Call API function to Test The State Machine ...\n" ));
    NbErr = 0;

    /* get device type (default: 10=7710) */
    RetErr = STTST_GetInteger( pars_p, 11, &LVar );
    if ( RetErr || (LVar < 0 || LVar > 13) )
    {
        STTST_TagCurrentLine( pars_p, "expected device type (1-13)\n" );
        RetErr = TRUE;
    }
    else
    {
        VOUTInitParams.DeviceType      = (STVOUT_DeviceType_t)(LVar-1);
        VOUTInitParams.CPUPartition_p  = DriverPartition_p;
        VOUTInitParams.MaxOpen         = 3;

        sprintf(DeviceName , STVOUT_DEVICE_NAME);

        switch ( VOUTInitParams.DeviceType)
        {
            case STVOUT_DEVICE_TYPE_DENC : /* no break */
            case STVOUT_DEVICE_TYPE_DENC_ENHANCED : /* no break */
            case STVOUT_DEVICE_TYPE_7015 : /* no break */
            case STVOUT_DEVICE_TYPE_7020 : /* no break */
            case STVOUT_DEVICE_TYPE_5528 : /* no break */
            case STVOUT_DEVICE_TYPE_5525 : /* no break */
            case STVOUT_DEVICE_TYPE_V13  : /* no break */
            case STVOUT_DEVICE_TYPE_4629 : /* no break */
            case STVOUT_DEVICE_TYPE_GX1  :
                 break;
            case STVOUT_DEVICE_TYPE_7710 : /* no break */
            case STVOUT_DEVICE_TYPE_7100 :
                     VOUTInitParams.OutputType = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.DeviceBaseAddress_p= (void *)VOUT_DEVICE_BASE_ADDRESS;
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.BaseAddress_p= (void *)VOUT_BASE_ADDRESS1;
                     VOUTInitParams.Target.HDMICell.Target.OnChipHdmiCell.SecondBaseAddress_p = (void *)VOUT_BASE_ADDRESS2;
                     /* Init a device to for further tests needing an init device */
                      ErrorCode = STVOUT_Init(DeviceName, &VOUTInitParams);
                      if ( ErrorCode != ST_NO_ERROR )
                      {
                         STTBX_Print(( "  STVOUT_Init() : unexpected return code=0x%0x \n", ErrorCode ));
                         NbErr++;
                      }
                      else
                      {
                         STTBX_Print(( " STVOUT_Init('%s') : ok\n", DeviceName));
                         /* Open device to get a good handle for further tests */
                         ErrorCode = STVOUT_Open(DeviceName, &VOUTOpenParams, &VOUTHndl);
                         if ( ErrorCode != ST_NO_ERROR )
                         {
                             STTBX_Print(( "  STVOUT_Open() : unexpected return code=0x%0x \n", ErrorCode ));
                             NbErr++;
                         }
                         else
                         {
                             STTBX_Print(( " STVOUT_Open('%s') : ok\n", DeviceName));
                             ErrorCode = STVOUT_GetState(VOUTHndl, &State);
                             if ( ErrorCode != ST_NO_ERROR )
                             {
                                STTBX_Print(( "  STVOUT_GetState() : unexpected return code=0x%0x \n", ErrorCode ));
                                NbErr++;
                             }
                             ErrorCode =  STVOUT_GetTargetInformation(VOUTHndl, &TargetInfo);
                             if ( ErrorCode != ST_NO_ERROR )
                             {
                                STTBX_Print(( "  STVOUT_GetTargetInformation() : unexpected return code=0x%0x \n", ErrorCode ));
                                NbErr++;
                             }

                             /* Close connection */
                             ErrorCode = STVOUT_Close(VOUTHndl);
                             if ( ErrorCode != ST_NO_ERROR )
                             {
                               STTBX_Print(( "  STVOUT_Close() : unexpected return code=0x%0x \n", ErrorCode ));
                               NbErr++;
                             }
                             /* Term */
                             ErrorCode = STVOUT_Term(DeviceName, &VOUTTermParams);
                             if ( ErrorCode != ST_NO_ERROR )
                             {
                                 STTBX_Print(( "  STVOUT_Term() : unexpected return code=0x%0x \n", ErrorCode ));
                                 NbErr++;
                             }
                         }
                      }
                      break;
                default :
                     break;
                }

    /* test result */
   if ( NbErr == 0 )
   {
       STTBX_Print(( "### VOUT_TestStateMachine() result : ok, ST_ERROR_BAD_PARAMETER returned each time as expected\n" ));
       RetErr = FALSE;
   }
   else
   {
       STTBX_Print(( "### VOUT_TestStateMachine() result : failed ! ST_ERROR_BAD_PARAMETER not returned %d times\n", NbErr ));
       RetErr = TRUE;
   }
   STTST_AssignInteger( result_sym_p, NbErr, FALSE);
   }

    return(API_EnableError ? RetErr : FALSE );

 }
#endif

#ifdef ST_OS20
/*******************************************************************************
Name        : VOUT_StackUsage
Description : launch tasks to calculate the stack usage made by the driver
              for an Init Term cycle and in its typical conditions of use
Parameters  : None
Assumptions :
Limitations :
Returns     : None
*******************************************************************************/
static BOOL VOUT_StackUsage(STTST_Parse_t *pars_p, char *result_sym_p)
{
    #define STACK_SIZE          4096 /* STACKSIZE must oversize the estimated stack usage of the driver */
    task_t task;
    /*tdesc_t tdesc;*/
    task_t *task_p;
    task_status_t status;
    int overhead_stackused;
    TaskParams_t TaskParams_p;

    /*char stack[STACK_SIZE];*/
    char *funcname[]= {
        "test_overhead",
        "test_typical",
        "NULL"
    };
    void (*func_table[])(void *) = {
        test_overhead,
        test_typical,
        NULL
    };
    void (*func)(void *);
    int i;


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    task_p = &task;
    overhead_stackused = 0;
    printf("*************************************\n");
    printf("* Stack usage calculation beginning *\n");
    printf("*************************************\n");
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];

        /* Start the task */
        task_p = task_create(func, (void *)&TaskParams_p, STACK_SIZE, MAX_USER_PRIORITY, "stack_test", task_flags_no_min_stack_size);

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


    return(FALSE );
}
#endif /* ST_OS20 */
#ifdef TRACE_DYNAMIC_DATASIZE
/*******************************************************************************
Name        : VOUT_GetDataSize
Description : Get DynamicDatasize allocated for the driver
Parameters  : None
Returns     : None
*******************************************************************************/
static BOOL VOUT_GetDynamicDataSize(STTST_Parse_t *pars_p, char *result_sym_p)
{
    STVOUT_InitParams_t InitParam;
    STVOUT_TermParams_t TermParams;

    InitParam.DeviceType      = VOUT_DEVICE_TYPE;
    InitParam.CPUPartition_p  = DriverPartition_p;
    InitParam.MaxOpen         = 3;
    InitParam.OutputType      = STVOUT_OUTPUT_ANALOG_CVBS;

    #if defined (ST_7020) || defined (ST_GX1)
        InitParam.Target.GenericCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
        InitParam.Target.GenericCell.BaseAddress_p       = (void*)VOUT_BASE_ADDRESS;
        strcpy( InitParam.Target.GenericCell.DencName, STDENC_DEVICE_NAME);
    #elif defined (ST_5528)
        InitParam.Target.DualTridacCell.DeviceBaseAddress_p = (void*)VOUT_DEVICE_BASE_ADDRESS;
        InitParam.Target.DualTridacCell.BaseAddress_p       = (void*)VOUT_BASE_ADDRESS;
        strcpy( InitParam.Target.DualTridacCell.DencName, STDENC_DEVICE_NAME);
    #else /* 55xx omega1, 7015. */
        strcpy( InitParam.Target.DencName, STDENC_DEVICE_NAME);
    #endif

    printf("*******************************************\n");
    printf("* Dynamic data size calculation beginning *\n");
    printf("*******************************************\n");

    DynamicDataSize=0;
    STVOUT_Init( STVOUT_DEVICE_NAME, &InitParam);
    printf("** Size of Dynamic Data= %d bytes **\n", DynamicDataSize);
    TermParams.ForceTerminate = FALSE;
    STVOUT_Term( STVOUT_DEVICE_NAME, &TermParams);

    printf("*******************************************\n");
    printf("*   Dynamic data size calculation end     *\n");
    printf("*******************************************\n");

    return(FALSE );
}
#endif

#ifdef ST_OSLINUX
static BOOL VOUT_Read( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
	S32 Addr;
    UNUSED_PARAMETER(result_sym_p );
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }
	printf("Read@%x:\n",Addr);
	printf("%x\n",STSYS_ReadRegDev32LE(Addr));

return RetErr;
}

static BOOL VOUT_Write( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr=FALSE;
	S32 Addr, Val;

    UNUSED_PARAMETER(result_sym_p );
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Addr );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Addr" );
        }
    }

    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &Val );
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected Value" );
        }
    }

	STSYS_WriteRegDev32LE((Addr),(Val));

	printf("Write@%x:\n",Addr);
	printf("%x\n",Val);

return RetErr;
}
#endif

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of test register command
 * Input    :
 * Output   :
 * Return   : TRUE if ok, FALSE if error
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart(void)
{
    BOOL RetErr = FALSE;

#ifdef ST_OSLINUX
    RetErr |= STTST_RegisterCommand( "VOUT_Read", VOUT_Read, "<Address> Read at @Address");
    RetErr |= STTST_RegisterCommand( "VOUT_Write", VOUT_Write, "<Address> <Value> Write at @Address");
#endif
    RetErr |= STTST_RegisterCommand( "VOUT_NullPt",    VOUT_NullPointersTest,"Call API functions with null pointers (bad param. test)");
    RetErr |= STTST_RegisterCommand( "VOUT_DencError", VOUT_InitDencError,"Call Init API function to test STVOUT_ERROR_DENC_x error");
    RetErr |= STTST_RegisterCommand( "VOUT_TestLimit", VOUT_TestLimitInit,"Call Init API function to test ST_ERROR_NO_MEMORY error");
    RetErr |= STTST_RegisterCommand( "VOUT_TestIncOut",VOUT_IncompatibleOut,"Call Init API function to test STVOUT_ERROR_VOUT_INCOMPATIBLE error");
    RetErr |= STTST_RegisterCommand( "VOUT_BadParam",  VOUT_BadParameterTest,"Call API functions with null pointers (bad param. test)");

#if defined (ST_7710) ||defined (ST_7100) || defined(ST_7109)
    RetErr |= STTST_RegisterCommand( "VOUT_StateMachine",  VOUT_TestStateMachine,"Call API functions to test the State Machine");
#endif
#ifdef ST_OS20
    RetErr |= STTST_RegisterCommand( "VOUT_StackUsage",VOUT_StackUsage,"Call stack usage task");
#endif /* ST_OS20 */

#ifdef STI7710_CUT2x
    RetErr |= STTST_AssignString ("CHIP_CUT", "STI7710_CUT2x", TRUE);
#else
    RetErr |= STTST_AssignString ("CHIP_CUT", "", TRUE);
#endif

    RetErr |= STTST_RegisterCommand( "VOUT_DotsOutIfOk",  VOUT_DotsOutIfOk,"Display dots in ok instead of full text");
    VoutTest_PrintDotsIfOk = FALSE;

    #ifdef TRACE_DYNAMIC_DATASIZE
        RetErr |= STTST_RegisterCommand( "VOUT_GetDynamicDataSize",VOUT_GetDynamicDataSize,"Get Dynamic Data Size allocated for the driver");
    #endif

    if (RetErr)
    {
        sprintf( VOUT_Msg, "VOUT_TestCommand()  \t: failed !\n" );
    }
    else
    {
        sprintf( VOUT_Msg, "VOUT_TestCommand()  \t: ok\n" );
    }
    STTBX_Print(( VOUT_Msg ));

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
#if defined(ST_OS21) || defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");

    #if defined (ST_OSWINCE)
		SetFopenBasePath("/dvdgr-prj-stvout/tests/src/objs/ST40");
	#endif

    setbuf(stdout, NULL);
#endif
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    os20_main(NULL);

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}
/* end of vout_test.c */



