/*******************************************************************************

File name   : vtg_cmd.c

Description : STVTG driver call layer source file.

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
21 Feb 2002        Created                                          HSdLM
08 Jul 2002        Add STVTG_DEBUG_STATISTICS                       HSdLM
11 Oct 2002        Add support for STi5517                          HSdLM
22 Nov 2002        Fix endianness issue with STTST_GetInteger()     HSdLM
 *                 use
12 May 2003        Add support of STi5528 (VFE2) and stem7020       HSdLM
05 Jul 2004        Add 7710/mb391 support                            AT
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"
#include "stdevice.h"

#include "stcommon.h"
/* Application settings */
#include "testcfg.h"

/* STAPI libraries */
#include "stsys.h"
#include "stvtg.h"

#ifdef ST_OSLINUX
#include "iocstapigat.h"
#endif

#include "sttbx.h"

#include "testtool.h"

/* Tests dependencies  */
#include "api_cmd.h"
#include "denc_cmd.h"
#include "vtg_cmd.h"
#ifdef USE_INTMR
#include "clintmr.h"
#include "stintmr.h"
#endif
#include "clevt.h"
#ifdef STVTG_USE_CLKRV
#include "clclkrv.h"
#endif

/* Private Types ------------------------------------------------------------ */

typedef struct vtg_ModeOption_s
{
    const char Mode[18];
    const STVTG_Option_t Value;
} vtg_ModeOption_t;

typedef struct VTG_StringToEnum_s
{
    const char String[16];
    const U32 Value;
} VTG_StringToEnum_t;

/* Private Constants -------------------------------------------------------- */

#define VTG_MAX_MODE  68
#define VTG_MAX_OPTION_MODE 5
#define STRING_DEVICE_LENGTH   80
#define RETURN_PARAMS_LENGTH   256

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5510) || defined (ST_5512) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))
#define VTG_DEVICE_TYPE          "TYPE_DENC"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      DENC_BASE_ADDRESS
#elif defined (ST_7015)
#define VTG_DEVICE_TYPE          "TYPE_7015"
#define VTG_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VTGCMD_BASE_ADDRESS      ST7015_VTG1_OFFSET
#elif defined (ST_5528) && !defined (ST_7020)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5528_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5528_VTG_VSYNC_INTERRUPT
#elif defined (ST_7020)
#define VTG_DEVICE_TYPE          "TYPE_7020"
#define VTG_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VTGCMD_BASE_ADDRESS      ST7020_VTG1_OFFSET
#if defined (ST_5528)
    #define VTG_5528_DEVICE_BASE_ADDRESS  0
    #define VTGCMD_5528_BASE_ADDRESS      ST5528_VTG_BASE_ADDRESS
    #define VTG_5528_INTERRUPT_NUMBER     ST5528_VTG_VSYNC_INTERRUPT
#endif /* defined (ST_5528) */
#elif defined (ST_5100)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5100_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5100_VTG_VSYNC_INTERRUPT
#elif defined (ST_5105)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5105_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5105_VTG_VSYNC_INTERRUPT

#elif defined (ST_5107)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5107_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5107_VTG_VSYNC_INTERRUPT

#elif defined (ST_5162)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5162_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5162_VTG_VSYNC_INTERRUPT

#elif defined (ST_5188)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5188_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5188_VTG_VSYNC_INTERRUPT
#elif defined (ST_5301)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5301_VTG_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5301_VTG_VSYNC_INTERRUPT
#elif defined (ST_5525)
#define VTG_DEVICE_TYPE          "TYPE_VFE2"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      ST5525_VTG_0_BASE_ADDRESS
#define VTG_INTERRUPT_NUMBER     ST5525_VTG_1_VSYNC_INTERRUPT
#define VTG1_INTERRUPT_NUMBER     ST5525_VTG_0_VSYNC_INTERRUPT
#define VTG2_INTERRUPT_NUMBER     ST5525_VTG_1_VSYNC_INTERRUPT

#elif defined (ST_GX1)
#define VTG_DEVICE_TYPE          "TYPE_VFE"
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTGCMD_BASE_ADDRESS      (S32)STGX1_VTG_BASE_ADDRESS
#elif defined (ST_7710)
#define VTG_DEVICE_TYPE           "TYPE_7710"
#define VTG1_BASE_ADDRESS         ST7710_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS         ST7710_VTG2_BASE_ADDRESS
#define VTG1_INTERRUPT_NUMBER     ST7710_VOS_0_INTERRUPT
#define VTG2_INTERRUPT_NUMBER     ST7710_VOS_1_INTERRUPT
#define VTG_DEVICE_BASE_ADDRESS   0
#define VTGCMD_BASE_ADDRESS       ST7710_VTG1_BASE_ADDRESS

#elif defined (ST_7100)
#define VTG_DEVICE_TYPE           "TYPE_7100"
#define VTG1_BASE_ADDRESS         ST7100_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS         ST7100_VTG2_BASE_ADDRESS
#define VTG1_INTERRUPT_NUMBER     ST7100_VTG_0_INTERRUPT
#define VTG2_INTERRUPT_NUMBER     ST7100_VTG_1_INTERRUPT
#define VTG_DEVICE_BASE_ADDRESS   0
#define VTGCMD_BASE_ADDRESS       ST7100_VTG1_BASE_ADDRESS

#elif defined (ST_7109)
#define VTG_DEVICE_TYPE           "TYPE_7100"
#define VTG1_BASE_ADDRESS         ST7109_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS         ST7109_VTG2_BASE_ADDRESS
#define VTG1_INTERRUPT_NUMBER     ST7109_VTG_0_INTERRUPT
#define VTG2_INTERRUPT_NUMBER     ST7109_VTG_1_INTERRUPT
#define VTG_DEVICE_BASE_ADDRESS   0
#define VTGCMD_BASE_ADDRESS       ST7109_VTG1_BASE_ADDRESS

#elif defined (ST_7200)
#define VTG_DEVICE_TYPE               "TYPE_7200"
#define VTG1_BASE_ADDRESS             ST7200_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS             ST7200_VTG2_BASE_ADDRESS
#define VTG3_BASE_ADDRESS             ST7200_VTG3_BASE_ADDRESS
#define VTG1_INTERRUPT_NUMBER         ST7200_VTG_MAIN0_INTERRUPT
#define VTG1_LINE_INTERRUPT_NUMBER    ST7200_VTG_MAIN1_INTERRUPT
#define VTG2_INTERRUPT_NUMBER         ST7200_VTG_AUX0_INTERRUPT
#define VTG2_LINE_INTERRUPT_NUMBER    ST7200_VTG_AUX1_INTERRUPT
#define VTG3_INTERRUPT_NUMBER         ST7200_VTG_SD0_INTERRUPT
#define VTG3_LINE_INTERRUPT_NUMBER    ST7200_VTG_SD1_INTERRUPT
#define VTG_DEVICE_BASE_ADDRESS       0
#define VTGCMD_BASE_ADDRESS           ST7200_VTG1_BASE_ADDRESS
#elif defined (ST_7111)
#define VTG_DEVICE_TYPE               "TYPE_7111"
#define VTG1_BASE_ADDRESS             ST7111_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS             ST7111_VTG2_BASE_ADDRESS
#define VTG3_BASE_ADDRESS             ST7111_VTG3_BASE_ADDRESS
#define VTG1_INTERRUPT_NUMBER         ST7111_VTG_MAIN0_INTERRUPT
#define VTG1_LINE_INTERRUPT_NUMBER    ST7111_VTG_MAIN1_INTERRUPT
#define VTG2_INTERRUPT_NUMBER         ST7111_VTG_AUX0_INTERRUPT
#define VTG2_LINE_INTERRUPT_NUMBER    ST7111_VTG_AUX1_INTERRUPT
#define VTG3_INTERRUPT_NUMBER         ST7111_VTG_SD0_INTERRUPT
#define VTG3_LINE_INTERRUPT_NUMBER    ST7111_VTG_SD1_INTERRUPT
#define VTG_DEVICE_BASE_ADDRESS       0
#define VTGCMD_BASE_ADDRESS           ST7111_VTG1_BASE_ADDRESS
#elif defined (ST_7105)
#define VTG_DEVICE_TYPE               "TYPE_7105"
#define VTG1_BASE_ADDRESS             ST7105_VTG1_BASE_ADDRESS
#define VTG2_BASE_ADDRESS             ST7105_VTG2_BASE_ADDRESS
#define VTG3_BASE_ADDRESS             ST7105_VTG3_BASE_ADDRESS
#define VTG1_INTERRUPT_NUMBER         ST7105_VTG_MAIN0_INTERRUPT
#define VTG1_LINE_INTERRUPT_NUMBER    ST7105_VTG_MAIN1_INTERRUPT
#define VTG2_INTERRUPT_NUMBER         ST7105_VTG_AUX0_INTERRUPT
#define VTG2_LINE_INTERRUPT_NUMBER    ST7105_VTG_AUX1_INTERRUPT
#define VTG3_INTERRUPT_NUMBER         ST7105_VTG_SD0_INTERRUPT
#define VTG3_LINE_INTERRUPT_NUMBER    ST7105_VTG_SD1_INTERRUPT
#define VTG_DEVICE_BASE_ADDRESS       0
#define VTGCMD_BASE_ADDRESS           ST7105_VTG1_BASE_ADDRESS
#ifdef ST_OSLINUX
/*extern MapParams_t   OutputStageMap;
extern MapParams_t   DencMap;
extern MapParams_t   SysCfgMap; */

#define BASE_ADDRESS_DENC        DencMap.MappedBaseAddress
#define VOS_BASE_ADDRESS_1         OutputStageMap.MappedBaseAddress
#define SYS_CFG_BASE_ADDRESS    SysCfgMap.MappedBaseAddress
#else
#define VOS_BASE_ADDRESS_1        VOS_BASE_ADDRESS
#define SYS_CFG_BASE_ADDRESS      CFG_BASE_ADDRESS
#endif
#else
#error Chip not defined !
#endif

#if defined (ST_7100)|| defined (ST_7109)
#ifdef ST_OSLINUX
extern MapParams_t   OutputStageMap;
extern MapParams_t   CompoMap;
extern MapParams_t   DencMap;
extern MapParams_t   SysCfgMap;

#define BASE_ADDRESS_DENC        DencMap.MappedBaseAddress
#define VOS_BASE_ADDRESS_1         OutputStageMap.MappedBaseAddress
#define VMIX1_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xC00
#define VMIX2_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xD00
#define VIDEO2_LAYER_BASE_ADDRESS  CompoMap.MappedBaseAddress + 0x800
#define SYS_CFG_BASE_ADDRESS    SysCfgMap.MappedBaseAddress
#else
#define VOS_BASE_ADDRESS_1        VOS_BASE_ADDRESS
#define SYS_CFG_BASE_ADDRESS      CFG_BASE_ADDRESS
#define VMIX1_BASE_ADDRESS        VMIX1_LAYER_BASE_ADDRESS
#define VMIX2_BASE_ADDRESS        VMIX2_LAYER_BASE_ADDRESS
#define VIDEO2_LAYER_BASE_ADDRESS    VID2_LAYER_BASE_ADDRESS
#endif
#endif

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7710)
#define   DHDO_ZEROL    0xa0
#define   DHDO_MIDL     0xa4
#define   DHDO_HIGHL    0xa8
#define   DHDO_MIDLOWL  0xac
#define   DHDO_LOWL     0xb0
#define   DHDO_COLOR    0xb4
#define   DHDO_YOFF     0xc4
#define   DHDO_YMLT     0xb8
#define   DHDO_CMLT     0xbc
#define   DHDO_COFF     0xc0
#endif


/* Private Variables (static)------------------------------------------------ */
#define MAX_VTG_TYPE 8
static const VTG_StringToEnum_t VtgType[MAX_VTG_TYPE] = {
   {"TYPE_DENC", STVTG_DEVICE_TYPE_DENC},
   {"TYPE_VFE",  STVTG_DEVICE_TYPE_VFE},
   {"TYPE_VFE2",  STVTG_DEVICE_TYPE_VFE2},
   {"TYPE_7015", STVTG_DEVICE_TYPE_VTG_CELL_7015},
   {"TYPE_7020", STVTG_DEVICE_TYPE_VTG_CELL_7020},
   {"TYPE_7710", STVTG_DEVICE_TYPE_VTG_CELL_7710},
   {"TYPE_7100", STVTG_DEVICE_TYPE_VTG_CELL_7100},
   {"TYPE_7200", STVTG_DEVICE_TYPE_VTG_CELL_7200}
};

 static const char ModeName[VTG_MAX_MODE][24] = {
    "MODE_SLAVE"            ,

    /* SD modes */
    "MODE_480I60000_13514"  , /* NTSC 60Hz */
    "MODE_480P60000_27027"  , /* ATSC P60 *1.001 */
    "MODE_480P30000_13514"  , /* ATSC P30 *1.001 */
    "MODE_480P24000_10811"  , /* ATSC P24 *1.001 */
    "MODE_480I59940_13500"  , /* NTSC, PAL M */
    "MODE_480P59940_27000"  , /* ATSC P60 */
    "MODE_480P29970_13500"  , /* ATSC P30 */
    "MODE_480P23976_10800"  , /* ATSC P24 */
    "MODE_480I60000_12285"  , /* NTSC 60Hz square */
    "MODE_480P60000_24570"  , /* ATSC P60 *1.1001square*/
    "MODE_480P30000_12285"  , /* ATSC P30 *1.1001square*/
    "MODE_480P24000_9828"   , /* ATSC P24 *1.1001square*/
    "MODE_480I59940_12273"  , /* NTSC square, PAL M square */
    "MODE_480P59940_24545"  , /* ATSC P60 square*/
    "MODE_480P29970_12273"  , /* ATSC P30 square*/
    "MODE_480P23976_9818"   , /* ATSC P24 square*/
    "MODE_576I50000_13500"  , /* PAL B,D,G,H,I,N, SECAM */
    "MODE_576I50000_14750"  , /* PAL B,D,G,H,I,N, SECAM square */

    /* HD modes */
    "MODE_1080P60000_148500", /* SMPTE 274M #1  P60 */
    "MODE_1080P59940_148352", /* SMPTE 274M #2  P60 /1.001 */
    "MODE_1080P50000_148500", /* SMPTE 274M #3  P50 */
    "MODE_1080I60000_74250" , /* EIA770.3 #3 I60 = SMPTE274M #4 I60 */
    "MODE_1080I59940_74176" , /* EIA770.3 #4 I60 /1.001 = SMPTE274M #5 I60 /1.001 */
    "MODE_1080I50000_74250" , /* SMPTE 295M #6  I50 */
    "MODE_1080I50000_74250_1", /* SMPTE 274M   P50   */
    "MODE_1080P30000_74250" , /* SMPTE 274M #7  P30 */
    "MODE_1080P29970_74176" , /* SMPTE 274M #8  P30 /1.001 */
    "MODE_1080P25000_74250" , /* SMPTE 274M #9  P25 */
    "MODE_1080P24000_74250" , /* SMPTE 274M #10 P24 */
    "MODE_1080P23976_74176" , /* SMPTE 274M #11 P24 /1.001 */
    "MODE_1035I60000_74250" , /* SMPTE 240M #1  I60 */
    "MODE_1035I59940_74176" , /* SMPTE 240M #2  I60 /1.001 */
    "MODE_720P60000_74250"  , /* EIA770.3 #1 P60 = SMPTE 296M #1 P60 */
    "MODE_720P59940_74176"  , /* EIA770.3 #2 P60 /1.001= SMPTE 296M #2 P60 /1.001 */
    "MODE_720P30000_37125"  , /* ATSC 720x1280 30P */
    "MODE_720P29970_37088"  , /* ATSC 720x1280 30P/1.001 */
    "MODE_720P24000_29700"  , /* ATSC 720x1280 24P */
    "MODE_720P23976_29670"  , /* ATSC 720x1280 24P/1.001 */
    "MODE_1080I50000_72000" , /* AS 4933-1 200x 1080i(1250) */
    "MODE_720P50000_74250"  , /* non standard mode */
    "MODE_576P50000_27000"  , /*Australian mode*/
    "MODE_1152I50000_48000" , /* non standard mode */
    "MODE_480P60000_25200"  ,  /*VGA mode*/
    "MODE_480P59940_25175"  ,
    "MODE_768P75000_78750"  ,  /*XGA mode*/
    "MODE_768P60000_65000"  ,  /*XGA mode ` 60Hz*/
    "MODE_768P85000_94500"  ,  /*XGA mode @ 85Hz*/
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", /* 47 -> 49 */
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", /* 50 -> 54 */
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", /* 55 -> 59 */
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED"  /* 60 -> 64 */
    };


static const vtg_ModeOption_t VTGOptionMode[VTG_MAX_OPTION_MODE+1] = {
    { "EMBEDDED_SYNCHRO", STVTG_OPTION_EMBEDDED_SYNCHRO},
    { "NO_OUTPUT_SIGNAL", STVTG_OPTION_NO_OUTPUT_SIGNAL},
    { "HSYNC_POLARITY", STVTG_OPTION_HSYNC_POLARITY},
    { "VSYNC_POLARITY", STVTG_OPTION_VSYNC_POLARITY},
    { "OUT_EDGE_POSITION", STVTG_OPTION_OUT_EDGE_POSITION},
    { "????", 0}
};

static STVTG_Handle_t VTGHndl;
static char VTG_Msg[1024];            /* text for trace */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */
#define SIZEOF_STRING(x) sizeof(x)/sizeof(char)
#define VTG_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(VTG_Msg)){ \
        sprintf(x, "VTG message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*-----------------------------------------------------------------------------
 * Function : VTG_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL VTG_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVTG_ERROR_INVALID_MODE:
                strcat( Text, "(stvtg error invalid mode) !\n");
                break;
            case STVTG_ERROR_EVT_REGISTER:
                strcat( Text, "(stvtg error evt register) !\n");
                break;
            case STVTG_ERROR_EVT_UNREGISTER:
                strcat( Text, "(stvtg error evt unregister) !\n");
                break;
            case STVTG_ERROR_EVT_SUBSCRIBE:
                strcat( Text, "(stvtg error evt subscribe) !\n");
                break;
            case STVTG_ERROR_DENC_OPEN:
                strcat( Text, "(stvtg error denc open) !\n");
                break;
            case STVTG_ERROR_CLKRV_OPEN:
                strcat( Text, "(stvtg error clkrv open) !\n");
                break;
            case STVTG_ERROR_CLKRV_CLOSE:
                strcat( Text, "(stvtg error clkrv close) !\n");
                break;

            case STVTG_ERROR_DENC_ACCESS:
                strcat( Text, "(stvtg error denc access) !\n");
                break;
#ifdef ST_OSLINUX
            case STVTG_ERROR_MAP_VTG :
                strcat( Text, "(stvtg error mapping VTG Address) !\n");
                break;
            case STVTG_ERROR_MAP_CKG:
                strcat( Text, "(stvtg error mapping CKG Address) !\n");
                break;
            case STVTG_ERROR_MAP_SYS:
                strcat( Text, "(stvtg error mapping SYS CFG Address) !\n");
                break;
#endif
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VTG_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : API_TagExpected
 * Input    :
 * Output   :
 * Return   :
 * --------------------------------------------------------------------------*/
static void API_TagExpected(const VTG_StringToEnum_t * UnionType, const U32 MaxValue, const char * DefaultValue)
{
    U32  i;

    sprintf(VTG_Msg, "Expected enum (");
    for(i=0; i<MaxValue; i++)
    {
        if(!strcmp(UnionType[i].String, DefaultValue))
        {
            strcat(VTG_Msg,"def ");
        }
        sprintf(VTG_Msg, "%s%d=%s,", VTG_Msg, UnionType[i].Value, UnionType[i].String);
    }
    /* assumes that one component is listed at least */
    strcat(VTG_Msg,"\b)");
    if(strlen(VTG_Msg) >= SIZEOF_STRING(VTG_Msg))
    {
        sprintf(VTG_Msg, "String is too short\n");
    }
} /* API_TagExpected */

/*#######################################################################*/
/*############################# VTG COMMANDS ############################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : VTG_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_InitParams_t VTGInitParams;
    char DeviceName[STRING_DEVICE_LENGTH], TrvStr[80], IsStr[80], *ptr;
    ST_DeviceName_t                ClkrvDeviceName;
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    S32 Var, DevNb;

    UNUSED_PARAMETER(result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }

    /* Get device number */
    STTST_GetItem( pars_p, VTG_DEVICE_TYPE, TrvStr, SIZEOF_STRING(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, SIZEOF_STRING(IsStr));
    if (!RetErr)
    {
        strncpy(TrvStr, IsStr, SIZEOF_STRING(TrvStr));
    }
    for(DevNb=0; DevNb<MAX_VTG_TYPE; DevNb++)
    {
        if((strcmp(VtgType[DevNb].String, TrvStr) == 0 ) ||
           (strncmp(VtgType[DevNb].String, TrvStr+1, strlen(VtgType[DevNb].String)) == 0 ))
            break;
    }
    if(DevNb==MAX_VTG_TYPE)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, &DevNb, 10);
        if(RetErr)
        {
            DevNb = (U32)strtoul(TrvStr, &ptr, 10);
            if (TrvStr==ptr)
            {
                API_TagExpected(VtgType, MAX_VTG_TYPE, VTG_DEVICE_TYPE);
                STTST_TagCurrentLine(pars_p, VTG_Msg);
                return(TRUE);
            }
        }
        VTGInitParams.DeviceType = DevNb;

        for (DevNb=0; DevNb<MAX_VTG_TYPE; DevNb++)
        {
            if(VtgType[DevNb].Value == VTGInitParams.DeviceType)
                break;
        }
    }
    else
    {
        VTGInitParams.DeviceType = VtgType[DevNb].Value;
    }

    if(VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        /* Get denc name */
        RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VTGInitParams.Target.DencName, \
                                  sizeof(VTGInitParams.Target.DencName) );
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected DENC name");
            return(TRUE);
        }
    }
    else
    {
        /* Get Base Address */
        RetErr = STTST_GetInteger( pars_p, VTGCMD_BASE_ADDRESS, &Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected base address");
            return(TRUE);
        }
#if defined (ST_7015)
        VTGInitParams.Target.VtgCell.BaseAddress_p = (void*)Var;
        VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG1_OFFSET)
        {
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7015_VTG1_INT;
        }
        else if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG2_OFFSET)
        {
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7015_VTG2_INT;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_7020)
#if defined (mb376)
         if (VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_VFE2)
        {
            VTGInitParams.Target.VtgCell2.BaseAddress_p       = (void*)VTGCMD_5528_BASE_ADDRESS;
            VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void*)VTG_5528_DEVICE_BASE_ADDRESS;
            VTGInitParams.Target.VtgCell2.InterruptNumber     = VTG_5528_INTERRUPT_NUMBER;
            VTGInitParams.Target.VtgCell2.InterruptLevel      = 7;
        }
        else
#endif /*defined (mb376)*/
        {
            VTGInitParams.Target.VtgCell.BaseAddress_p = (void*)Var;
            VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
            if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG1_OFFSET)
            {
                VTGInitParams.Target.VtgCell.InterruptEvent = ST7020_VTG1_INT;
            }
            else if (VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG2_OFFSET)
            {
                VTGInitParams.Target.VtgCell.InterruptEvent = ST7020_VTG2_INT;
            }
            else
            {
                STTST_Print(("No interrupt found!!\n"));
            }
            strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
        }
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)||defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        VTGInitParams.Target.VtgCell3.BaseAddress_p = (void*)Var;
        VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG1_BASE_ADDRESS)
        {
            VTGInitParams.Target.VtgCell3.InterruptNumber = VTG1_INTERRUPT_NUMBER;
        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG2_BASE_ADDRESS)
        {
           VTGInitParams.Target.VtgCell3.InterruptNumber = VTG2_INTERRUPT_NUMBER;
        }
      #if defined (ST_7200)|| defined (ST_7111)||defined (ST_7105)
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG3_BASE_ADDRESS)
        {
           VTGInitParams.Target.VtgCell3.InterruptNumber = VTG3_INTERRUPT_NUMBER;
        }
      #endif
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        VTGInitParams.Target.VtgCell3.InterruptLevel      = 4;

#elif defined(ST_5528)|| defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5107)|| defined(ST_5162)
        VTGInitParams.Target.VtgCell2.BaseAddress_p       = (void*)Var;
        VTGInitParams.Target.VtgCell2.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell2.InterruptNumber     = VTG_INTERRUPT_NUMBER;
#if defined(ST_5100)|| defined(ST_5105)|| defined(ST_5301)|| defined(ST_5188)|| defined(ST_5107)|| defined(ST_5162)
        VTGInitParams.Target.VtgCell2.InterruptLevel      = 0;
#else
        VTGInitParams.Target.VtgCell2.InterruptLevel      = 7;
#endif
#elif defined (ST_5525)
        VTGInitParams.Target.VtgCell3.BaseAddress_p = (void*)Var;
        VTGInitParams.Target.VtgCell3.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG_0_BASE_ADDRESS)
        {
            VTGInitParams.Target.VtgCell3.InterruptNumber = VTG1_INTERRUPT_NUMBER;
        }
        else if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG_1_BASE_ADDRESS)
        {
           VTGInitParams.Target.VtgCell3.InterruptNumber = VTG2_INTERRUPT_NUMBER;
        }
        else
        {
            STTST_Print(("No interrupt found!!\n"));
        }
        VTGInitParams.Target.VtgCell3.InterruptLevel      = 0;

#elif defined (ST_GX1)
        VTGInitParams.Target.VtgCell.BaseAddress_p        = (void*)Var;
        VTGInitParams.Target.VtgCell.DeviceBaseAddress_p  = (void*)VTG_DEVICE_BASE_ADDRESS;
        VTGInitParams.Target.VtgCell.InterruptEvent       = 0;
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#endif
    }

    /* Get Max Open */
    RetErr = STTST_GetInteger( pars_p, 2, (S32*)&Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected max open init parameter (default=1)");
        return(TRUE);
    }
    VTGInitParams.MaxOpen = (U32)Var;

    strcpy(VTGInitParams.EvtHandlerName, STEVT_DEVICE_NAME);

    if(VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        sprintf( VTG_Msg, "VTG_Init(%s, Type=%s, Name=%s, MaxOpen=%d): ", \
                 DeviceName, "55XX" , VTGInitParams.Target.DencName, VTGInitParams.MaxOpen);
    }
    else
    {
        sprintf( VTG_Msg, "VTG_Init(%s, Type=%s, BaseAdd=0x%0X, MaxOpen=%d): ", \
                 DeviceName,
                 ((DevNb >= 0) && (DevNb < MAX_VTG_TYPE)) ? VtgType[DevNb].String : "?????",
                 (U32)VTGInitParams.Target.VtgCell.BaseAddress_p, VTGInitParams.MaxOpen);
    }
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    VTGInitParams.VideoBaseAddress_p       = (void*) VIDEO_BASE_ADDRESS;
    VTGInitParams.VideoDeviceBaseAddress_p = (void*) 0;
    VTGInitParams.VideoInterruptNumber     = VIDEO_INTERRUPT;
    VTGInitParams.VideoInterruptLevel      = VIDEO_INTERRUPT_LEVEL;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */

#ifdef STVTG_USE_CLKRV
       /* Get Clkrv name */
    #if defined (ST_7200)|| defined (ST_7111)||defined (ST_7105) /* connect VTG to the right STCLKRV instance */
        if (VTGInitParams.Target.VtgCell3.BaseAddress_p == (void*)VTG3_BASE_ADDRESS)
        {
            strcpy(ClkrvDeviceName, STCLKRV1_DEVICE_NAME);
        }
        else
        {
            strcpy(ClkrvDeviceName, STCLKRV_DEVICE_NAME);
        }
    #else
            strcpy(ClkrvDeviceName, STCLKRV_DEVICE_NAME);
    #endif
        RetErr = STTST_GetString( pars_p, ClkrvDeviceName, VTGInitParams.Target.VtgCell3.ClkrvName, \
                                      sizeof(VTGInitParams.Target.VtgCell3.ClkrvName) );

     if (RetErr)
     {
        STTST_TagCurrentLine( pars_p, "expected CLKRV name");
        return(TRUE);
     }
#endif


    ErrorCode = STVTG_Init(DeviceName, &VTGInitParams);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);
    return(API_EnableError ? RetErr: FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VTG_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_OpenParams_t VTGOpenParams;
    BOOL RetErr;
    char DeviceName[STRING_DEVICE_LENGTH];
    ST_ErrorCode_t     ErrorCode;

    /* get device */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VTG_Msg, "VTG_Open(%s): ", DeviceName);
    ErrorCode = STVTG_Open(DeviceName, &VTGOpenParams, &VTGHndl);
    if (ErrorCode == ST_NO_ERROR)
    {
        sprintf(VTG_Msg, "%sHdl=%d ", VTG_Msg, VTGHndl);
    }
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);
    STTST_AssignInteger(result_sym_p, VTGHndl, FALSE);
    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VTG_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Open a VTG device connection */
    sprintf( VTG_Msg, "VTG_Close(%d): ", VTGHndl);

    ErrorCode = STVTG_Close(VTGHndl);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VTG_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_TermParams_t VTGTermParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    S32 lVar;

    UNUSED_PARAMETER(result_sym_p);
    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
    STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&lVar);
    if (RetErr)
    {
    STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }

    VTGTermParams.ForceTerminate = (U32)lVar;

    sprintf( VTG_Msg, "VTG_Term(%s): ", DeviceName );
    ErrorCode = STVTG_Term(DeviceName, &VTGTermParams);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    return(API_EnableError ? RetErr: FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VTG_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;


    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    RevisionNumber = STVTG_GetRevision();
    sprintf( VTG_Msg, "VTG_GetRevision(): Ok\nRevision=%s\n", RevisionNumber );

    VTG_PRINT(VTG_Msg);
    return ( FALSE );
} /* end VTG_GetRevision */


/*-------------------------------------------------------------------------
 * Function : VTG_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode;
    STVTG_Capability_t Capability;
    char StrParams[RETURN_PARAMS_LENGTH];
    char DeviceName[STRING_DEVICE_LENGTH];
    U32  Mode;

    /* get device */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VTG_Msg, "VTG_GetCapability(%s): ", DeviceName);
    ErrorCode = STVTG_GetCapability(DeviceName, &Capability);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    if ( ErrorCode == ST_NO_ERROR)
    {
        strcat( VTG_Msg, "Timing Mode available: ");
        for (Mode = 0; Mode < 32; Mode++)
        {
            if (Capability.TimingMode &  (1<<Mode))
            {
                sprintf( VTG_Msg, "\t%s = %d", ModeName[Mode], Mode);

                if (Mode == STVTG_TIMING_MODE_480I59940_13500)
                    strcat( VTG_Msg, " usual NTSC\n");
                else if (Mode == STVTG_TIMING_MODE_576I50000_13500)
                    strcat( VTG_Msg, " usual PAL\n");
                else
                    strcat( VTG_Msg, "\n");
                VTG_PRINT(VTG_Msg);
            }
        }
        /* Second part of allowed mode */
        for (Mode = 0; Mode < 32; Mode++)
        {
            if (Capability.TimingModesAllowed2 &  (1<<Mode))
            {
                sprintf( VTG_Msg, "\t%s = %d", ModeName[Mode+32], Mode+32);
                strcat( VTG_Msg, "\n");
                VTG_PRINT(VTG_Msg);
            }
        }
        sprintf( StrParams, "%d %d", \
                 Capability.TimingMode, Capability.TimingModesAllowed2);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end STVTG_GetCapability */


/*-------------------------------------------------------------------------
 * Function : VTG_SetMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_SetMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_TimingMode_t ModeVtg;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    char *ptr=NULL, TrvStr[80], IsStr[80];
    U32 Var;

    UNUSED_PARAMETER(result_sym_p);
    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output mode  (default:  PAL Mode 480x576 at 50Hz ) */
    STTST_GetItem(pars_p, "PAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }

    for(Var=0; Var<STVTG_TIMING_MODE_COUNT; Var++){
        if((strcmp(ModeName[Var], TrvStr)==0)|| \
           strncmp(ModeName[Var], TrvStr+1, strlen(ModeName[Var]))==0)
            break;
    }
    if((strcmp("\"PAL\"",TrvStr)==0) || (strcmp("PAL",TrvStr)==0)){
        Var=STVTG_TIMING_MODE_576I50000_13500;
    }
    if((strcmp("\"NTSC\"",TrvStr)==0) || (strcmp("NTSC",TrvStr)==0)){
        Var=STVTG_TIMING_MODE_480I59940_13500;
    }
    if(Var==STVTG_TIMING_MODE_COUNT){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if (RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
        }
    }

    if ((Var>VTG_MAX_MODE) || (TrvStr==ptr))
    {
        STTST_TagCurrentLine( pars_p, "Expected VTG mode (see capabilities)");
        return(TRUE);
    }
    ModeVtg=(STVTG_TimingMode_t)Var;

    sprintf( VTG_Msg,  "VTG_SetMode(%d, %s=%d): ", VTGHndl, ModeName[ModeVtg], ModeVtg);
    ErrorCode = STVTG_SetMode(VTGHndl, ModeVtg);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    return (API_EnableError ? RetErr : FALSE);

} /* end VTG_SetMode */


/*-------------------------------------------------------------------------
 * Function : VTG_GetMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_ModeParams_t ModeParams;
    STVTG_TimingMode_t TimingMode;
    ST_ErrorCode_t ErrorCode;
    char StrParams[RETURN_PARAMS_LENGTH];
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VTG_Msg, "VTG_GetMode(%d) : ", VTGHndl);
    ErrorCode = STVTG_GetMode(VTGHndl, &TimingMode, &ModeParams);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    if ( ErrorCode == ST_NO_ERROR)
    {
       sprintf( VTG_Msg, "\tMode: %s = %d\n", ModeName[TimingMode], TimingMode);
       sprintf( VTG_Msg, "%s\tFrameRate:%d  ScanType:%s\n"\
                 ,VTG_Msg , ModeParams.FrameRate,\
                 (ModeParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" : \
                 (ModeParams.ScanType == STGXOBJ_INTERLACED_SCAN) ? "INTER" : "?????");

       sprintf( VTG_Msg, "%s\tWidth:%d  Height:%d  XStart:%d  YStart:%d  DXStart:%d  DYStart:%d\n",
                 VTG_Msg,
                 ModeParams.ActiveAreaWidth,
                 ModeParams.ActiveAreaHeight,
                 ModeParams.ActiveAreaXStart,
                 ModeParams.ActiveAreaYStart,
                 ModeParams.DigitalActiveAreaXStart,
                 ModeParams.DigitalActiveAreaYStart);
        sprintf( StrParams, "%d %d %d %d %d %d %d %d %d",
                 TimingMode,
                 ModeParams.FrameRate,
                 ModeParams.ScanType,
                 ModeParams.ActiveAreaWidth,
                 ModeParams.ActiveAreaHeight,
                 ModeParams.ActiveAreaXStart,
                 ModeParams.ActiveAreaYStart,
                 ModeParams.DigitalActiveAreaXStart,
                 ModeParams.DigitalActiveAreaYStart
            );
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VTG_PRINT(VTG_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VTG_GetMode */
/*-------------------------------------------------------------------------
 * Function : VTG_GetModeSyncParams_EnableHVSync
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetModeSyncParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
	BOOL RetErr;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
	STVTG_SyncParams_t SyncParams;
	RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
	sprintf(VTG_Msg, "VTG_GetModeSyncParams(%d): ", VTGHndl);
	ErrorCode = STVTG_GetModeSyncParams(VTGHndl, &SyncParams);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);
	if ( ErrorCode == ST_NO_ERROR)
	{
	sprintf( VTG_Msg, "\nPixels Per Line  :%d \n",SyncParams.PixelsPerLine);
	VTG_PRINT(VTG_Msg);
	sprintf( VTG_Msg, "Lines By Frame   :%d \n",SyncParams.LinesByFrame);
	VTG_PRINT(VTG_Msg);
	sprintf( VTG_Msg, "Pixel Clock      :%d \n",SyncParams.PixelClock);
	VTG_PRINT(VTG_Msg);
	sprintf( VTG_Msg, "HSync Polarity   :%d \n",SyncParams.HSyncPolarity);
	VTG_PRINT(VTG_Msg);
	sprintf( VTG_Msg, "HSync Pulse Width:%d \n",SyncParams.HSyncPulseWidth);
	VTG_PRINT(VTG_Msg);
    sprintf( VTG_Msg, "VSync Polarity   :%d \n",SyncParams.VSyncPolarity);
	VTG_PRINT(VTG_Msg);
	sprintf( VTG_Msg, "VSync Pulse Width:%d \n",SyncParams.VSyncPulseWidth);
	VTG_PRINT(VTG_Msg);
	}
	return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VTG_SetOptionalConfiguration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_SetOptionalConfiguration( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_OptionalConfiguration_t OptionalParams_s;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    S32 Var, i;
    char TrvStr[80], IsStr[80], *ptr;

    UNUSED_PARAMETER(result_sym_p);

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "EMBEDDED_SYNCHRO", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VTG_MAX_OPTION_MODE; Var++){
        if((strcmp(VTGOptionMode[Var].Mode, TrvStr)==0) || \
           (strncmp(VTGOptionMode[Var].Mode, TrvStr+1, strlen(VTGOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==VTG_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if (RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Option (default EMBEDDED_SYNCHRO)");
                return(TRUE);
            }
        }
    }
    OptionalParams_s.Option = (STVTG_Option_t)Var;

    /* Find in array */
    for(i=0;i<VTG_MAX_OPTION_MODE;i++)
        if(VTGOptionMode[i].Value == OptionalParams_s.Option)
            break;

    /* Get Values */
    if(OptionalParams_s.Option == STVTG_OPTION_OUT_EDGE_POSITION)
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected HSyncRising");
            return(TRUE);
        }
        OptionalParams_s.Value.OutEdges.HSyncRising = (U32)Var;

        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected HSyncFalling");
            return(TRUE);
        }
        OptionalParams_s.Value.OutEdges.HSyncFalling = (U32)Var;

        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected VSyncRising");
            return(TRUE);
        }
        OptionalParams_s.Value.OutEdges.VSyncRising = (U32)Var;

        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected VSyncFalling");
            return(TRUE);
        }
        OptionalParams_s.Value.OutEdges.VSyncFalling = (U32)Var;

        sprintf( VTG_Msg, "VTG_SetOptionalConfiguration(%d,Opt=%s,HRis=%d,HFal=%d,VRis=%d,VFal=%d) : ",
                 VTGHndl,
                 VTGOptionMode[i].Mode,
                 OptionalParams_s.Value.OutEdges.HSyncRising, OptionalParams_s.Value.OutEdges.HSyncFalling,
                 OptionalParams_s.Value.OutEdges.VSyncRising, OptionalParams_s.Value.OutEdges.VSyncFalling
                 );
    }
    else
    {
        /* Same type for the all the rest */
        RetErr = STTST_GetInteger( pars_p, FALSE, &Var);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected Boolean (default FALSE)");
            return(TRUE);
        }
        OptionalParams_s.Value.EmbeddedSynchro = (BOOL)Var;
        sprintf( VTG_Msg, "VTG_SetOptionalConfiguration(%d,Opt=%s,Val=%s) : ", VTGHndl,
                 VTGOptionMode[i].Mode,
                 (OptionalParams_s.Value.EmbeddedSynchro) ? "TRUE" :
                 (!OptionalParams_s.Value.EmbeddedSynchro) ? "FALSE" : "????"
                 );
    }

    ErrorCode = STVTG_SetOptionalConfiguration(VTGHndl, &OptionalParams_s);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : VTG_SetSMParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_SetSMParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_SlaveModeParams_t SlaveModeParams_s;
    S32 LVar;
    S32 Type, DevNb;
    ST_ErrorCode_t ErrorCode;
    char TrvStr[80], IsStr[80], *ptr;
    BOOL RetErr;

    #if defined (ST_7710) || defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)||defined (ST_7105)
       U32 SlvSrc = 1 ;   /** default slave src with ST_7710/7100/7109 must be VTG1 because there is no external sync for VTG */
    #else
       U32 SlvSrc = 0 ;
    #endif

    UNUSED_PARAMETER(result_sym_p);

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get device number */
    STTST_GetItem( pars_p, VTG_DEVICE_TYPE, TrvStr, SIZEOF_STRING(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, SIZEOF_STRING(IsStr));
    if (!RetErr)
    {
        strncpy(TrvStr, IsStr, SIZEOF_STRING(TrvStr));
    }
    for(DevNb=0; DevNb<MAX_VTG_TYPE; DevNb++)
    {
        if((strcmp(VtgType[DevNb].String, TrvStr) == 0 ) ||
           (strncmp(VtgType[DevNb].String, TrvStr+1, strlen(VtgType[DevNb].String)) == 0 ))
            break;
    }
    if(DevNb==MAX_VTG_TYPE)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, &DevNb, 10);
        if(RetErr)
        {
            DevNb = (U32)strtoul(TrvStr, &ptr, 10);
            if (TrvStr==ptr)
            {
                API_TagExpected(VtgType, MAX_VTG_TYPE, VTG_DEVICE_TYPE);
                STTST_TagCurrentLine(pars_p, VTG_Msg);
                return(TRUE);
            }
        }
        Type = DevNb;

        for (DevNb=0; DevNb<MAX_VTG_TYPE; DevNb++)
        {
            if(VtgType[DevNb].Value == (U32)Type)
                break;
        }
    }
    else
    {
        Type = VtgType[DevNb].Value;
    }

    /* Get output mode */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected slave mode : (default 0,1,2) <-> (H_AND_V, V_ONLY, H_ONLY)");
        return(TRUE);
    }
    SlaveModeParams_s.SlaveMode = (STVTG_SlaveMode_t)LVar;


    if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_DENC){
        /* Get DENC slave of */
        RetErr = STTST_GetInteger( pars_p, STVTG_DENC_SLAVE_OF_ODDEVEN, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected DENC slave of : (default 0,1,2) <-> (ODDEVEN,VSYNC,SYNC_IN_DATA)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.DencSlaveOf = (STVTG_DencSlaveOf_t)LVar;

        /* Get FreeRun */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected FreeRun : (default 0,1) <-> (FALSE, TRUE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.FreeRun = (BOOL)LVar;

        /* Get OutSyncAvailable */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected OutSyncAvailable : (default 0,1) <-> (FALSE, TRUE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.OutSyncAvailable = (BOOL)LVar;

        /* Get SyncInAdjust */
        RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected SyncInAdjust : (0,default 1,2,3) <-> (Nomi,+1,+2,+3)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.SyncInAdjust = (U8)LVar;
    }
    else if ( ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7015)
            ||((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7020)
            ||((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7710)
            ||((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7100)
            ||((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7200))
    {
        /* Get H slave of */
        RetErr = STTST_GetInteger( pars_p, SlvSrc, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, \
                                  "Expected H slave of : (Default 0,1,2,3,4) <-> (SYNC_PIN, VTG1, SDIN1, SDIN2, HDIN)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.HSlaveOf = (STVTG_SlaveOf_t)LVar;

        /* Get V slave of */
        RetErr = STTST_GetInteger( pars_p, SlvSrc, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected V slave of : (0,1,2,3,4) <-> (SYNC_PIN, VTG1, SDIN1, SDIN2, HDIN)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.VSlaveOf = (STVTG_SlaveOf_t)LVar;

        /* Set default value for input */
        SlaveModeParams_s.Target.VtgCell.HRefVtgIn = STVTG_VTG_ID_1;
        SlaveModeParams_s.Target.VtgCell.VRefVtgIn = STVTG_VTG_ID_1;

        /* Get BottomFieldDetectMin (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected BottomFieldDetectMin");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMin = (U32)LVar;

        /* Get BottomFieldDetectMax (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected BottomFieldDetectMax >=0");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMax = (U32)LVar;

        /* Get ExtVSyncShape (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected ExtVSyncShape : (0,1) <-> (BNOTT, PULSE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.ExtVSyncShape = (STVTG_ExtVSyncShape_t)LVar;

        /* Get IsExtVSyncRisingEdge (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected IsExtVSyncRisingEdge : (0,1) <-> (FALSE, TRUE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.IsExtVSyncRisingEdge = (LVar == 1);

        /* Get IsExtHSyncRisingEdge (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected IsExtHSyncRisingEdge : (0,1) <-> (FALSE, TRUE))");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.IsExtHSyncRisingEdge = (LVar == 1);
    }
    else if(((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE) || (STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE2)
    {
        /* Get slave of */
        RetErr = STTST_GetInteger( pars_p, 5, (S32*)&LVar );
        if(RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected slave of : (Default 5,6,7,8) <-> (DVP0, DVP1, EXT0, EXT1)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VfeSlaveOf = (STVTG_SlaveOf_t)LVar;
    }

    sprintf( VTG_Msg, "VTG_SetSMParams(%d): ", VTGHndl);
    ErrorCode = STVTG_SetSlaveModeParams(VTGHndl, &SlaveModeParams_s);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);
    if(ErrorCode == ST_NO_ERROR)
    {
        sprintf( VTG_Msg, "\tSlaveMode = %s\n", \
                 (SlaveModeParams_s.SlaveMode == STVTG_SLAVE_MODE_H_AND_V) ? "H_AND_V":
                 (SlaveModeParams_s.SlaveMode == STVTG_SLAVE_MODE_V_ONLY) ? "V_ONLY":
                 (SlaveModeParams_s.SlaveMode == STVTG_SLAVE_MODE_H_ONLY) ? "H_ONLY" : "?????");

        if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_DENC)
        {
            sprintf( VTG_Msg, "%s\tDencSlaveOf= %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.Denc.DencSlaveOf == STVTG_DENC_SLAVE_OF_ODDEVEN) ? "ODDEVEN": \
                     (SlaveModeParams_s.Target.Denc.DencSlaveOf == STVTG_DENC_SLAVE_OF_VSYNC) ? "VSYNC": \
                     (SlaveModeParams_s.Target.Denc.DencSlaveOf == STVTG_DENC_SLAVE_OF_SYNC_IN_DATA) ? "IN_DATA": \
                     "????");
            sprintf( VTG_Msg, "%s\tFreeRun= %d\n" , VTG_Msg, SlaveModeParams_s.Target.Denc.FreeRun);
            sprintf( VTG_Msg, "%s\tOutSyncAvailable= %d\n" , VTG_Msg, SlaveModeParams_s.Target.Denc.OutSyncAvailable);
            sprintf( VTG_Msg, "%s\tSyncInAdjust= %d\n" , VTG_Msg, SlaveModeParams_s.Target.Denc.SyncInAdjust);
        }
        else if ( ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7015) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7020) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7710) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7100) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7200))
        {
            sprintf( VTG_Msg, "%s\tHSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_PIN) ? "PIN": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_REF) ? "REF": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_SDIN1) ? "SDIN1": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_SDIN2) ? "SDIN2": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_HDIN) ? "HDIN": "?????");

            sprintf( VTG_Msg, "%s\tVSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_PIN) ? "PIN": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_REF) ? "REF": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_SDIN1) ? "SDIN1": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_SDIN2) ? "SDIN2": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_HDIN) ? "HDIN": "?????");

            sprintf( VTG_Msg, "%s\tBottomFieldDetectMin = %u\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMin);
            sprintf( VTG_Msg, "%s\tBottomFieldDetectMax = %u\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMax);

            sprintf( VTG_Msg, "%s\tExtVSyncShape = %s\n", VTG_Msg,
                     (SlaveModeParams_s.Target.VtgCell.ExtVSyncShape == STVTG_EXT_VSYNC_SHAPE_BNOTT) ? "BNOTT" : \
                     (SlaveModeParams_s.Target.VtgCell.ExtVSyncShape == STVTG_EXT_VSYNC_SHAPE_PULSE) ? "PULSE" : \
                     "?????" );
            sprintf( VTG_Msg, "%s\tIsExtVSyncRisingEdge = %d\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.IsExtVSyncRisingEdge);
            sprintf( VTG_Msg, "%s\tIsExtHSyncRisingEdge = %d\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.IsExtHSyncRisingEdge);
        }
        else if(((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE) || ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE2))
        {
            sprintf( VTG_Msg, "%s\tSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_DVP0) ? "DVP0": \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_DVP1) ? "DVP1": \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_EXT0) ? "EXT0": \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_EXT1) ? "EXT1": "?????");
        }
        VTG_PRINT(VTG_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VTG_SetSMParams */


/*-------------------------------------------------------------------------
 * Function : VTG_GetOptionalConfiguration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetOptionalConfiguration( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_OptionalConfiguration_t OptionalParams_s;
    char StrParams[RETURN_PARAMS_LENGTH];
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr;
    char TrvStr[80], IsStr[80], *ptr;
    S32 Var, i;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "EMBEDDED_SYNCHRO", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VTG_MAX_OPTION_MODE; Var++)
    {
        if((strcmp(VTGOptionMode[Var].Mode, TrvStr)==0) ||
           (strncmp(VTGOptionMode[Var].Mode, TrvStr+1, strlen(VTGOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==VTG_MAX_OPTION_MODE)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if (RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Option (default EMBEDDED_SYNCHRO)");
                return(TRUE);
            }
        }
    }
    OptionalParams_s.Option = (STVTG_Option_t)Var;

    /* Find in array */
    for(i=0;i<VTG_MAX_OPTION_MODE;i++)
        if(VTGOptionMode[i].Value == OptionalParams_s.Option)
            break;

    sprintf( VTG_Msg, "VTG_GetOptionalConfiguration(%d,Opt=%s) : ", VTGHndl, VTGOptionMode[i].Mode);
    ErrorCode = STVTG_GetOptionalConfiguration(VTGHndl, &OptionalParams_s);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);
    if(ErrorCode == ST_NO_ERROR)
    {
        VTG_Msg[0] = '\0';
        if(OptionalParams_s.Option == STVTG_OPTION_OUT_EDGE_POSITION)
        {
            sprintf ( VTG_Msg, "%s\tHSyncRising=%d HSyncFalling=%d\tVSyncRising=%d VSyncFalling=%d\n",
                      VTG_Msg, OptionalParams_s.Value.OutEdges.HSyncRising, OptionalParams_s.Value.OutEdges.HSyncFalling,
                      OptionalParams_s.Value.OutEdges.VSyncRising, OptionalParams_s.Value.OutEdges.VSyncFalling
                );
            sprintf(StrParams, "%d %d %d %d",
                    OptionalParams_s.Value.OutEdges.HSyncRising, OptionalParams_s.Value.OutEdges.HSyncFalling,
                    OptionalParams_s.Value.OutEdges.VSyncRising, OptionalParams_s.Value.OutEdges.VSyncFalling
                );
        }
        else
        {
            /* Same type for the all the rest */
            sprintf ( VTG_Msg, "%s\tValue=%s\n", VTG_Msg,
                      (OptionalParams_s.Value.EmbeddedSynchro) ? "TRUE" :
                      (!OptionalParams_s.Value.EmbeddedSynchro) ? "FALSE" : "????"
                );
            sprintf(StrParams, "%d", OptionalParams_s.Value.EmbeddedSynchro);
        }

        STTST_AssignString(result_sym_p, StrParams, FALSE);

        VTG_PRINT(VTG_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : VTG_GetSMParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetSMParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_SlaveModeParams_t SlaveModeParams_s;
    ST_ErrorCode_t ErrorCode;
    S32 Type, DevNb;
    char StrParams[RETURN_PARAMS_LENGTH], TrvStr[80], IsStr[80], *ptr;
    BOOL RetErr;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get device number */
    STTST_GetItem( pars_p, VTG_DEVICE_TYPE, TrvStr, SIZEOF_STRING(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, SIZEOF_STRING(IsStr));
    if (!RetErr)
    {
        strncpy(TrvStr, IsStr, SIZEOF_STRING(TrvStr));
    }
    for(DevNb=0; DevNb<MAX_VTG_TYPE; DevNb++)
    {
        if((strcmp(VtgType[DevNb].String, TrvStr) == 0 ) ||
           (strncmp(VtgType[DevNb].String, TrvStr+1, strlen(VtgType[DevNb].String)) == 0 ))
            break;
    }
    if(DevNb==MAX_VTG_TYPE)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, &DevNb, 10);
        if(RetErr)
        {
            DevNb = (U32)strtoul(TrvStr, &ptr, 10);
            if (TrvStr==ptr)
            {
                API_TagExpected(VtgType, MAX_VTG_TYPE, VTG_DEVICE_TYPE);
                STTST_TagCurrentLine(pars_p, VTG_Msg);
                return(TRUE);
            }
        }
        Type = DevNb;

        for (DevNb=0; DevNb<MAX_VTG_TYPE; DevNb++)
        {
            if(VtgType[DevNb].Value ==(U32)Type)
                break;
        }
    }
    else
    {
        Type = VtgType[DevNb].Value;
    }

    sprintf( VTG_Msg, "STVTG_GetSMParams(%d): ", VTGHndl);
    ErrorCode = STVTG_GetSlaveModeParams(VTGHndl, &SlaveModeParams_s);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    if ( ErrorCode == ST_NO_ERROR)
    {
        sprintf( VTG_Msg, "Parameters: \n");
        sprintf( VTG_Msg, "%s\tSlaveMode = %s\n", VTG_Msg,
                 (SlaveModeParams_s.SlaveMode == STVTG_SLAVE_MODE_H_AND_V) ? "H_AND_V":
                 (SlaveModeParams_s.SlaveMode == STVTG_SLAVE_MODE_V_ONLY) ? "V_ONLY":
                 (SlaveModeParams_s.SlaveMode == STVTG_SLAVE_MODE_H_ONLY) ? "H_ONLY" : "?????");

        if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_DENC)
        {
            sprintf( VTG_Msg, "%s\tDencSlaveOf= %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.Denc.DencSlaveOf == STVTG_DENC_SLAVE_OF_ODDEVEN) ? "ODDEVEN": \
                     (SlaveModeParams_s.Target.Denc.DencSlaveOf == STVTG_DENC_SLAVE_OF_VSYNC) ? "VSYNC": \
                     (SlaveModeParams_s.Target.Denc.DencSlaveOf == STVTG_DENC_SLAVE_OF_SYNC_IN_DATA) ? "IN_DATA": \
                     "????");
            sprintf( VTG_Msg, "%s\tFreeRun= %d\n" , VTG_Msg, SlaveModeParams_s.Target.Denc.FreeRun);
            sprintf( VTG_Msg, "%s\tOutSyncAvailable= %d\n" , VTG_Msg, SlaveModeParams_s.Target.Denc.OutSyncAvailable);
            sprintf( VTG_Msg, "%s\tSyncInAdjust= %d\n" , VTG_Msg, SlaveModeParams_s.Target.Denc.SyncInAdjust);
        }
        else if ( ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7015) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7020) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7710) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7100) ||
                  ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7200))
        {
            sprintf( VTG_Msg, "%s\tHSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_PIN) ? "SYNC_PIN": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_REF) ? "REF": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_SDIN1) ? "SDIN1": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_SDIN2) ? "SDIN2": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_HDIN) ? "HDIN": "?????");

            sprintf( VTG_Msg, "%s\tVSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_PIN) ? "SYNC_PIN": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_REF) ? "REF": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_SDIN1) ? "SDIN1": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_SDIN2) ? "SDIN2": \
                     (SlaveModeParams_s.Target.VtgCell.VSlaveOf == STVTG_SLAVE_OF_HDIN) ? "HDIN": "?????");

            sprintf( VTG_Msg, "%s\tBottomFieldDetectMin = %u\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMin);
            sprintf( VTG_Msg, "%s\tBottomFieldDetectMax = %u\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMax);

            sprintf( VTG_Msg, "%s\tExtVSyncShape = %s\n", VTG_Msg,
                     (SlaveModeParams_s.Target.VtgCell.ExtVSyncShape == STVTG_EXT_VSYNC_SHAPE_BNOTT) ? "BNOTT" : \
                     (SlaveModeParams_s.Target.VtgCell.ExtVSyncShape == STVTG_EXT_VSYNC_SHAPE_PULSE) ? "PULSE" : \
                     "?????" );
            sprintf( VTG_Msg, "%s\tIsExtVSyncRisingEdge = %d\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.IsExtVSyncRisingEdge);
            sprintf( VTG_Msg, "%s\tIsExtHSyncRisingEdge = %d\n", VTG_Msg, \
                     SlaveModeParams_s.Target.VtgCell.IsExtHSyncRisingEdge);
            sprintf(StrParams, "%d %d %d %d %d %d %d", \
                    SlaveModeParams_s.Target.VtgCell.HSlaveOf, \
                    SlaveModeParams_s.Target.VtgCell.VSlaveOf, \
                    SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMin, \
                    SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMax, \
                    SlaveModeParams_s.Target.VtgCell.ExtVSyncShape, \
                    SlaveModeParams_s.Target.VtgCell.IsExtVSyncRisingEdge, \
                    SlaveModeParams_s.Target.VtgCell.IsExtHSyncRisingEdge );
        }
        else if(((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE) || ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE2))
        {
            sprintf( VTG_Msg, "%s\tSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_DVP0) ? "DVP0": \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_DVP1) ? "DVP1": \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_EXT0) ? "EXT0": \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_EXT1) ? "EXT1": "?????");
        }
        VTG_PRINT(VTG_Msg);

        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end VTG_GetSMParams */


#ifdef STVTG_DEBUG_STATISTICS
/*-------------------------------------------------------------------------
 * Function : VTG_GetStat
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetStat( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_Statistics_t Statistics_s;
    ST_ErrorCode_t ErrorCode;
    BOOL RetErr, ResetAll;
    S32 Var;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, FALSE, &Var);
    if ( (RetErr) || ((Var != 0) && (Var !=1)))
    {
        STTST_TagCurrentLine( pars_p, "Expected Boolean (default FALSE)");
        return(TRUE);
    }
    ResetAll = (BOOL)Var;

    sprintf( VTG_Msg, "STVTG_GetStatistics(%d): ", VTGHndl);
    ErrorCode = STVTG_GetStatistics(VTGHndl, &Statistics_s, ResetAll);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    if ( ErrorCode == ST_NO_ERROR)
    {
        sprintf( VTG_Msg, "Statistics: \n");
        sprintf( VTG_Msg, "%s\tCountSimultaneousTopAndBottom = %d\n", VTG_Msg, \
                     Statistics_s.CountSimultaneousTopAndBottom);
        sprintf( VTG_Msg, "%s\tCountVsyncItDuringNotify = %d\n", VTG_Msg, \
                     Statistics_s.CountVsyncItDuringNotify);
        VTG_PRINT(VTG_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VTG_GetStat */
#endif /* #ifdef STVTG_DEBUG_STATISTICS */

#if defined (ST_7109)
/*-------------------------------------------------------------------------
 * Function : VTG_EnableHVSync
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_EnableHVSync( STTST_Parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Value;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    /*printf("Enable VSync and Hsync\n");*/

    STOS_InterruptLock();
    Value = STSYS_ReadRegDev32LE(SYS_CFG_BASE_ADDRESS + 0x11c);
    Value &= 0xFFFCFFFF;
    Value |= 0x20000;
    STSYS_WriteRegDev32LE(SYS_CFG_BASE_ADDRESS + 0x11c, Value);
    STOS_InterruptUnlock();

    return ( API_EnableError ? ErrorCode : FALSE );

} /* end VTG_EnableHVSync */

#endif  /* #ifdef ST_7109 */

#if defined (ST_7100) || defined (ST_7109)
/*-------------------------------------------------------------------------
 * Function : VTG_GamSet
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GamSet( STTST_Parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    S32 Mix_Sel;
    U32 CutId;

    UNUSED_PARAMETER(result_sym_p);

    CutId = ST_GetCutRevision();

    ErrorCode = STTST_GetInteger(pars_p, 2, &Mix_Sel);
    if (ErrorCode)
    {
        STTST_TagCurrentLine( pars_p, "Mix_Sel(1/2)" );
        return(TRUE);
    }

    if(Mix_Sel == 2)
    {
        STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x0, 0x4);
        STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x28, 0x29007f);
        STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x2c, 0x208034e);
        #if defined (ST_7109)
            if(CutId >= 0xC0 )
            {
                STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x34, 0x2);/*for cut 3*/
            }
        #endif

        STSYS_WriteRegDev32LE( VOS_BASE_ADDRESS_1 + 0x70, 0x0);
        STSYS_WriteRegDev32LE( VIDEO2_LAYER_BASE_ADDRESS + 0x04, 0x80);
        STSYS_WriteRegDev32LE( VIDEO2_LAYER_BASE_ADDRESS + 0x0c, 0xc800f0);
        STSYS_WriteRegDev32LE( VIDEO2_LAYER_BASE_ADDRESS + 0x10, 0x1df0257);

    }
    else
    {
        STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x0, 0x3);
        STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x28, 0x300089);
        STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x2c, 0x26f0358);
        STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x34, 0x40);
        STSYS_WriteRegDev32LE( VOS_BASE_ADDRESS_1 + 0x70, 0x1);
    }

    return ( API_EnableError ? ErrorCode : FALSE );

} /* end VTG_GamSet */
#endif /* #ifdef ST_7100 */

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7710)
static void Dump_DHDO( void )
{
    U32 Value = 0;

    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_ZEROL);
    sprintf( VTG_Msg, "%sDHDO_ZEROL: Addr = 0x%x  Val = 0x%x\n", VTG_Msg, (VTG1_BASE_ADDRESS + DHDO_ZEROL), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_MIDL);
    sprintf( VTG_Msg, "%sDHDO_MIDL: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_MIDL), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_HIGHL);
    sprintf( VTG_Msg, "%sDHDO_HIGHL: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_HIGHL), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_MIDLOWL);
    sprintf( VTG_Msg, "%sDHDO_MIDLOWL: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_MIDLOWL), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_LOWL);
    sprintf( VTG_Msg, "%sDHDO_LOWL: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_LOWL), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_COLOR);
    sprintf( VTG_Msg, "%sDHDO_COLOR: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_COLOR), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_YOFF);
    sprintf( VTG_Msg, "%sDHDO_YOFF: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_YOFF), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_YMLT);
    sprintf( VTG_Msg, "%sDHDO_YMLT: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_YMLT), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_COFF);
    sprintf( VTG_Msg, "%sDHDO_COFF: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_COFF), Value);
    Value = STSYS_ReadRegDev32LE(VTG1_BASE_ADDRESS + DHDO_CMLT);
    sprintf( VTG_Msg, "%sDHDO_CMLT: Addr = 0x%x Val = 0x%x\n", VTG_Msg,(VTG1_BASE_ADDRESS + DHDO_CMLT), Value);
 }
/*-------------------------------------------------------------------------
 * Function : VTG_DumpDHDORegisters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/

static BOOL VTG_DumpDHDORegisters( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    sprintf( VTG_Msg, "DUMP DHDO registers : \n");
    Dump_DHDO();

    VTG_PRINT(VTG_Msg);

    return ( API_EnableError ? ErrorCode : FALSE );

} /*VTG_DumpDHDORegisters*/


/*-------------------------------------------------------------------------
 * Function : VTG_CalibrateLevel1080I
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * Description : example done for 1080I mode with Echostar values
 * ----------------------------------------------------------------------*/
static BOOL VTG_CalibrateLevel1080I( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_SyncLevel_t   SyncLevel1080I = { STVTG_TIMING_MODE_1080I60000_74250, {0xf9, 0x16e, 0x1e3, 0x82, 0xb, 0xd2, 0xca, 0x27c, 0x279}};

    UNUSED_PARAMETER(result_sym_p);
    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VTG_Msg, "STVTG_CalibrateSyncLevel(%d): ", VTGHndl);
    ErrorCode = STVTG_CalibrateSyncLevel(VTGHndl , &SyncLevel1080I);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    if ( ErrorCode == ST_NO_ERROR)
    {
        sprintf( VTG_Msg, "DHDODUMPING: \n");
        Dump_DHDO();
    }
     VTG_PRINT(VTG_Msg);
     return ( API_EnableError ? RetErr : FALSE );
} /*VTG_CalibrateLevel1080I*/
/*-------------------------------------------------------------------------
 * Function : VTG_CalibrateLevel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_CalibrateLevel( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;

    char *ptr=NULL, TrvStr[80], IsStr[80];
    U32 Var, LVar;

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_ModeParams_t ModeParams;
    STVTG_TimingMode_t TimingMode,ModeVtg;

    STVTG_SyncLevel_t   SyncLevel;

    UNUSED_PARAMETER(result_sym_p);

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output mode  (default: mode currently defined ) */
    ErrorCode = STVTG_GetMode(VTGHndl, &TimingMode, &ModeParams);

    STTST_GetItem(pars_p, ModeName[TimingMode], TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if (!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }

    for(Var=0; Var<STVTG_TIMING_MODE_COUNT; Var++)
    {
        if((strcmp(ModeName[Var], TrvStr)==0)|| strncmp(ModeName[Var], TrvStr+1, strlen(ModeName[Var]))==0)
            break;
    }
    if((strcmp("\"PAL\"",TrvStr)==0) || (strcmp("PAL",TrvStr)==0))
    {
        Var=STVTG_TIMING_MODE_576I50000_13500;
    }
    if((strcmp("\"NTSC\"",TrvStr)==0) || (strcmp("NTSC",TrvStr)==0))
    {
        Var=STVTG_TIMING_MODE_480I59940_13500;
    }
    if(Var==STVTG_TIMING_MODE_COUNT)
    {
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if (RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
        }
    }

    if ((Var>VTG_MAX_MODE) || (TrvStr==ptr))
    {
        STTST_TagCurrentLine( pars_p, "Expected VTG mode (see capabilities)");
        return(TRUE);
    }
    ModeVtg =(STVTG_TimingMode_t)Var;

    SyncLevel.TimingMode = ModeVtg;
    /*get sync Level parameters*/
    /* Get Sync_ZeroLevel  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync ZeroLevel ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_ZeroLevel = (U16)LVar;

    /* Get Sync_MidLevel  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync MidLevel ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_MidLevel = (U16)LVar;

    /* Get Sync_HighLevel  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync HighLevel ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_HighLevel = (U16)LVar;

    /* Get Sync_MidLowLevel  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync MidLowLevel ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_MidLowLevel = (U16)LVar;

    /* Get Sync_LowLevel  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync LowLevel ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_LowLevel = (U16)LVar;

    /* Get Sync_YOff  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync YOff  ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_YOff = (U16)LVar;

    /* Get Sync_COff  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync COff");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_COff = (U16)LVar;

    /* Get Sync_YMult  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync YMult ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_YMult = (U16)LVar;

    /* Get Sync_CMult  */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Sync CMult ");
        return(TRUE);
    }
    SyncLevel.Sync.Sync_CMult = (U16)LVar;

    sprintf( VTG_Msg, "STVTG_CalibrateSyncLevel(%d): ", VTGHndl);

    ErrorCode = STVTG_CalibrateSyncLevel(VTGHndl , &SyncLevel);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    sprintf( VTG_Msg, "\nCalibrate Mode %s\n", ModeName[SyncLevel.TimingMode]);
    if ( ErrorCode == ST_NO_ERROR)
    {
        sprintf( VTG_Msg, "%sDHDO DUMPING: \n",VTG_Msg);
        Dump_DHDO();
    }

    VTG_PRINT(VTG_Msg);
    return ( API_EnableError ? RetErr : FALSE );
}  /*VTG_CalibrateLevel*/

/*-------------------------------------------------------------------------
 * Function : VTG_GetSyncLevel
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_GetSyncLevel( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STVTG_SyncLevel_t   SyncLevel;

    UNUSED_PARAMETER(result_sym_p);

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VTG_Msg, "STVTG_GetLevelSettings(%d): ", VTGHndl);
    ErrorCode = STVTG_GetLevelSettings(VTGHndl , &SyncLevel);
    RetErr = VTG_TextError(ErrorCode, VTG_Msg);

    sprintf( VTG_Msg, "Sync Level: \n");
    sprintf( VTG_Msg, "%s         Timing Mode is %s: \n",VTG_Msg,ModeName[SyncLevel.TimingMode]);
    sprintf( VTG_Msg, "%s         Sync Level params are: \n",VTG_Msg);
    sprintf( VTG_Msg, "%s                   Sync ZeroLevel  = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_ZeroLevel  );
    sprintf( VTG_Msg, "%s                   Sync MidLevel   = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_MidLevel   );
    sprintf( VTG_Msg, "%s                   Sync HighLevel  = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_HighLevel  );
    sprintf( VTG_Msg, "%s                   Sync MidLowLevel= 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_MidLowLevel);
    sprintf( VTG_Msg, "%s                   Sync LowLevel   = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_LowLevel   );
    sprintf( VTG_Msg, "%s                   Sync YOff       = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_YOff       );
    sprintf( VTG_Msg, "%s                   Sync COff       = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_COff       );
    sprintf( VTG_Msg, "%s                   Sync YMult      = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_YMult      );
    sprintf( VTG_Msg, "%s                   Sync CMult      = 0x%x\n" ,VTG_Msg, SyncLevel.Sync.Sync_CMult      );

    VTG_PRINT(VTG_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /*VTG_GetSyncLevel*/

#endif /*defined (ST_7100) || defined (ST_7109) || defined (ST_7710)*/
/*-------------------------------------------------------------------------
 * Function : VTG_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VTG_InitCommand(void)
{
    BOOL RetErr=FALSE;

    RetErr |= STTST_RegisterCommand( "VTG_Init", VTG_Init,
                                     "<DeviceName><DeviceType TYPE_(DENC,VFE,7015,7020,7710,7100)><DENC Name/BaseAddress>VTG Init") ;
    RetErr |= STTST_RegisterCommand( "VTG_Open", VTG_Open, "<Device Name> VTG Open");
    RetErr |= STTST_RegisterCommand( "VTG_Close", VTG_Close, "<Handle> VTG Close");
    RetErr |= STTST_RegisterCommand( "VTG_GetMode", VTG_GetMode, "<Handle> VTG GetMode");
    RetErr |= STTST_RegisterCommand( "VTG_GOptional", VTG_GetOptionalConfiguration,
                                     "<Handle> Get Optional parameters");
    RetErr |= STTST_RegisterCommand( "VTG_GSMParams", VTG_GetSMParams, "<Handle><Type> Get slave mode params");
    RetErr |= STTST_RegisterCommand( "VTG_SetMode", VTG_SetMode,
                                     "<Handle><TimingMode> Set timing mode (See capabilities for more info)");
    RetErr |= STTST_RegisterCommand( "VTG_SOptional", VTG_SetOptionalConfiguration,
                                     "<Handle><EmbeddedSynchro><NoOutputSignal> Set Optional parameters");
    RetErr |= STTST_RegisterCommand( "VTG_SSMParams", VTG_SetSMParams,
                                     "<Handle><Type><Slave mode><...> Set slave mode params");
    RetErr |= STTST_RegisterCommand( "VTG_Term", VTG_Term, "<DeviceName><ForceTeminate> Terminate");
    RetErr |= STTST_RegisterCommand( "VTG_Rev", VTG_GetRevision, "VTG Get Revision");
    RetErr |= STTST_RegisterCommand( "VTG_Capa", VTG_GetCapability, "<Device Name> VTG Get Capability");
	RetErr |= STTST_RegisterCommand( "VTG_GetModeSyncParams", VTG_GetModeSyncParams, "<Handle>");

#ifdef STVTG_DEBUG_STATISTICS
    RetErr |= STTST_RegisterCommand( "VTG_Stat", VTG_GetStat, "<Device Name> VTG Get Statistics");
#endif
#ifdef ST_7109
    RetErr |= STTST_RegisterCommand( "VTG_EnableHVSync", VTG_EnableHVSync, "Enable Hsync and Vsync") ;
#endif
#if defined (ST_7100) || defined (ST_7109)
    RetErr |= STTST_RegisterCommand( "VTG_GamSet", VTG_GamSet, "VTG_GamSet");
#endif
#if defined (ST_7100) || defined (ST_7109) ||  defined (ST_7710)
    RetErr |= STTST_RegisterCommand( "VTG_Calib", VTG_CalibrateLevel, "<Handle><TimingMode><Sync parameters>:calibrate sync level");
    RetErr |= STTST_RegisterCommand( "VTG_Calib1080I", VTG_CalibrateLevel1080I, "example done for 1080I mode with Echostar values");
    RetErr |= STTST_RegisterCommand( "VTG_GetS", VTG_GetSyncLevel, "VTG_GetSyncLevel");
    RetErr |= STTST_RegisterCommand( "VTG_DumpDHDO", VTG_DumpDHDORegisters, "dump DHDO registers");
#endif

    /* Constants */
    RetErr |= STTST_AssignInteger ("ERR_VTG_INVALID_MODE", STVTG_ERROR_INVALID_MODE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VTG_DENC_OPEN", STVTG_ERROR_DENC_OPEN, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VTG_CLKRV_OPEN", STVTG_ERROR_CLKRV_OPEN, TRUE);
#ifdef ST_OSLINUX
    RetErr |= STTST_AssignInteger ("ERR_MAP_VTG", STVTG_ERROR_MAP_VTG, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_MAP_CKG", STVTG_ERROR_MAP_CKG, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_MAP_SYS",  STVTG_ERROR_MAP_SYS, TRUE);
    RetErr |= STTST_AssignString ("DRV_PATH_VTG", "vtg/", TRUE);
#else
  RetErr |= STTST_AssignString ("DRV_PATH_VTG", "", TRUE);
#endif

    RetErr |= STTST_AssignString  ("VTG_TYPE", VTG_DEVICE_TYPE, FALSE);
    RetErr |= STTST_AssignString  ("VTG_NAME", STVTG_DEVICE_NAME, FALSE);

    return(!RetErr);


} /* end VTG_InitCommand */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL VTG_RegisterCmd(void)
{
    BOOL RetOk;
    RetOk = VTG_InitCommand();
    if ( RetOk )
    {
        sprintf( VTG_Msg, "VTG_RegisterCmd() \t: ok           %s\n", STVTG_GetRevision() );
    }
    else
    {
        sprintf( VTG_Msg, "VTG_RegisterCmd() \t: failed !\n");
    }
    VTG_PRINT(VTG_Msg);

    return(RetOk);
}
/* end of file */

