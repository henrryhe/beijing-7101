/************************************************************************
COPYRIGHT (C) STMicroelectronics 2000

File name   : vin_test.c
Description : VIN macros

Note        : All functions return TRUE if error for CLILIB compatibility

Date          Modification                                    Initials
----          ------------                                    --------
12 Feb 2001   Creation                                        JG
08 Jul 2004   STi7710 support modifications.                  GG
17 Jul 2006   STi7100 and STi7109 support modifications.      CS

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "testcfg.h"
#include "stsys.h"
#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"

#if defined(ST_OSLINUX)
#include "iocstapigat.h"
#endif  /*ST_OSLINUX*/

#include "stvin.h"
#include "stvid.h"
#include "testtool.h"
#include "stgxobj.h"

#include "api_cmd.h"
#include "startup.h"
#include "clevt.h"
#ifdef USE_CLKRV
#include "clclkrv.h"
#endif
#include "clintmr.h"
#include "vid_cmd.h"
#include "vin_cmd.h"
#if !defined ST_OSLINUX
#include "clavmem.h" /* for AvmemPartitionHandle[MAX_PARTITIONS] */
#endif /*!ST_OSLINUX*/

#if defined(ST_7015)
# include "sti7015.h"
# include "mb295.h"
#endif /* ST_7015 */

#if defined(ST_7020)
# include "sti7020.h"
  #if defined(mb295)
  # include "mb295.h"
  #endif
  #if defined(mb290)
  # include "mb290.h"
  #endif
#endif /* ST_7020 */

#if defined (ST_7710)
# include "sti7710.h"
# include "mb391.h"
#endif /* ST_7710 */

#if defined (ST_7100)
# include "sti7100.h"
# include "mb411.h"
#endif /* ST_7100 */

#if defined (ST_7109)
# include "sti7109.h"
# include "mb411.h"
#endif /* ST_7109 */

#if defined(ST_GX1)
#include "st40gx1.h"
#include "mb317.h"
#define GX1_DVP0_OFFSET 0x000
#define GX1_DVP1_OFFSET 0x100
/* TODO: Update this ! */
#define GX1_DVP0_INT 1
#define GX1_DVP1_INT 2
#endif

#if defined (ST_7020) && (defined(mb382) || defined(mb314))
#define db573 /* clearer #define for below */
#endif

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/
/* --- Private Constants --- */
static STVIN_Handle_t VINHndl;

static const char TabDeviceTyp[7][22] = {
    "7015_DIGITAL_INPUT"   ,
    "7020_DIGITAL_INPUT"   ,
    "ST40GX1_DVP_INPUT"    ,
    "5528_DVP_INPUT"       ,
    "7710_DVP_INPUT"       ,
    "7100_DVP_INPUT"       ,
    "7109_DVP_INPUT"
};

static const char TabInputMod[10][26] = {
    "SD_NTSC_720_480_I_CCIR"   ,
    "SD_NTSC_640_480_I_CCIR"   ,
    "SD_PAL_720_576_I_CCIR"    ,
    "SD_PAL_768_576_I_CCIR"    ,
    "HD_YCbCr_1920_1080_I_CCIR",
    "HD_YCbCr_1280_720_P_CCIR" ,
    "HD_RGB_1024_768_P_EXT"    ,
    "HD_RGB_800_600_P_EXT",
    "HD_RGB_640_480_P_EXT",
    "HD_YCbCr_720_480_P_CCIR"
};

static const char TabInputTyp[7][35] = {
    "DIGITAL_YCbCr422_8_MULT"           ,
    "DIGITAL_YCbCr422_16_CHROMA_MULT"   ,
    "DIGITAL_YCbCr444_24_COMP"          ,
    "DIGITAL_RGB888_24_COMP_TO_YCbCr422",
    "DIGITAL_RGB888_24_COMP"            ,
    "DIGITAL_CD_SERIAL_MULT"            ,
    "DIGITAL_CD_MULT"
};

static const char TabSync[2][14] = {
    "TYPE_EXTERNAL",
    "TYPE_CCIR"
};

static const char TabPath[2][9] = {
    "Path_PAD",
    "Path_DVO"
};

static const char TabScan[2][25] = {
    "STGXOBJ_PROGRESSIVE_SCAN",
    "STGXOBJ_INTERLACED_SCAN"
};

static const char TabEdge[2][8] = {
    "RISING " ,
    "FALLING"
};

static const char TabMethod[2][52] = {
    "STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING"         ,
    "STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES"
};

static const char TabPolarity[2][46] = {
    "STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH",
    "STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_LOW"
};

static const char TabAnc[3][37] = {
    "STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED",
    "STVIN_ANC_DATA_PACKET_DIRECT"        ,
    "STVIN_ANC_DATA_RAW_CAPTURE"
};

static const char TabStandard[4][23] = {
    "STVIN_STANDARD_PAL"    ,
    "STVIN_STANDARD_PAL_SQ" ,
    "STVIN_STANDARD_NTSC"   ,
    "STVIN_STANDARD_NTSC_SQ"
};

#if defined (ST_GX1)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  (S32)(0xBB420000 + 0x0 ) /* waiting to be defined in vob chip */
#define VIN_BASE_ADDRESS         0 /* waiting to be defined in vob chip */
#define VIN_INPUT_MODE           STVIN_SD_PAL_720_576_I_CCIR
#elif defined (ST_7015)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT
#define VIN_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VIN_BASE_ADDRESS         ST7015_SDIN1_OFFSET
#define VIN_INPUT_MODE           STVIN_SD_NTSC_720_480_I_CCIR
#elif defined (ST_7020)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT
#define VIN_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VIN_BASE_ADDRESS         ST7020_SDIN1_OFFSET
#define VIN_INPUT_MODE           STVIN_SD_NTSC_720_480_I_CCIR
#elif defined (ST_5528)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST5528_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0 /*STI5528_BASE_ADDRESS Not defined*/
#define VIN_BASE_ADDRESS         ST5528_DVP_BASE_ADDRESS
#define VIN_INPUT_MODE           STVIN_SD_NTSC_720_480_I_CCIR
#elif defined (ST_7710)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST7710_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0 /*STI7710_BASE_ADDRESS Not yet defined !!!*/
#define VIN_BASE_ADDRESS         ST7710_DVP_BASE_ADDRESS
#define VIN_INPUT_MODE           STVIN_SD_NTSC_720_480_I_CCIR
#elif defined (ST_7100)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST7100_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0 /*STI7100_BASE_ADDRESS Not yet defined !!!*/
#define VIN_BASE_ADDRESS         ST7100_DVP_BASE_ADDRESS
#define VIN_INPUT_MODE           STVIN_SD_NTSC_720_480_I_CCIR
#elif defined (ST_7109)
#define VIN_DEVICE_TYPE          STVIN_DEVICE_TYPE_ST7109_DVP_INPUT
#define VIN_DEVICE_BASE_ADDRESS  0 /*STI7109_BASE_ADDRESS Not yet defined !!!*/
#define VIN_BASE_ADDRESS         ST7109_DVP_BASE_ADDRESS
#define VIN_INPUT_MODE           STVIN_SD_NTSC_720_480_I_CCIR
#else
#error Not defined processor
#endif

/* --- Global variables --- */
char    VIN_Msg[255];            /* text for trace */

#define DEFAULT_SCREEN_WIDTH     720         /* nb of horiz. pixels */
#define DEFAULT_SCREEN_HEIGHT    480         /* nb of vert. pixels */


/* --- Macros ---*/
#define VIN_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(VIN_Msg)){ \
        sprintf(x, "VIN Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \


/* --- Extern variables --- */

/* Private Function prototypes ---------------------------------------------- */
static BOOL VIN_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_Close(         STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_Init(          STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_Open(          STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_GetRevision(   STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_Term(          STTST_Parse_t *pars_p, char *result_sym_p );

static BOOL VIN_SetSyncType(   STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_SetScanType(   STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_SetAncillaryParameters( STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_SetInputType(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_SetFrameRate(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_GetFrameRate(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_SetInputWindowMode(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_GetInputWindowMode(  STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL VIN_SetOutputWindowMode(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_GetOutputWindowMode(  STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL VIN_SetIOWindows(  STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_GetIOWindows(  STTST_Parse_t *pars_p, char *result_sym_p );

static BOOL VIN_SetParams(     STTST_Parse_t *pars_p, char *result_sym_p );
static BOOL VIN_GetParams(     STTST_Parse_t *pars_p, char *result_sym_p );

static BOOL VIN_InitCommand(void);

/*#######################################################################*/
/*############################# VIN COMMANDS ###########################*/
/*#######################################################################*/
static int puiss2(int n)   {   int p;  for( p=1; n>0; --n, p*=2);  return p;   }

static int shift( int n)   {   int p;  for ( p=0; n!=0; p++, n>>=1);   return(p);  }

/*-----------------------------------------------------------------------------
 * Function : VIN_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL VIN_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVIN_ERROR_FUNCTION_NOT_IMPLEMENTED:
                strcat( Text, "(stvin error function not implemented) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VIN_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VIN_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
ST_Revision_t RevisionNumber;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    RevisionNumber = STVIN_GetRevision( );
    sprintf( VIN_Msg, "STVIN_GetRevision() : revision=%s\n", RevisionNumber );
    VIN_PRINT( VIN_Msg );
    return ( FALSE );
} /* end VIN_GetRevision */



/*-------------------------------------------------------------------------
 * Function : VIN_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    STVIN_Capability_t Capability;
    char DeviceName[80];

  	/* get device */
    RetErr = STTST_GetString( pars_p, STVIN_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
    }
    else
    {
        ErrCode = STVIN_GetCapability(DeviceName, &Capability);
        sprintf( VIN_Msg, "STVIN_GetCapability('%s'): ", DeviceName );
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if ( ErrCode == ST_NO_ERROR)
        {
            STTST_AssignInteger("RET_VAL1", (int)Capability.SupportedAncillaryData, FALSE);
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );
} /* end STVIN_GetCapability */



/*-------------------------------------------------------------------------
 * Function : VIN_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVIN_InitParams_t VINInitParams;
    char DeviceName[40], EvtName[40];
#ifdef ST_MasterDigInput
    char VidName[40], ClockRecoveryName[40];
#endif
    long LVar;
    BOOL RetErr;
    STVIN_DeviceType_t  Device;
    STVIN_InputMode_t   Input;
    U32 MaxOpen;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;

#ifdef ST_OSLINUX
    /* Defined for compatibility reason */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle[1] = {0};
#endif /*ST_OSLINUX*/

    /* Initialise the VIN to the default initialisation mode */

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVIN_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }

    /* Get device type */
    RetErr = STTST_GetInteger( pars_p, (S32)VIN_DEVICE_TYPE, (S32*)&LVar);
    if ( ( RetErr == TRUE ) || ( (STVIN_DeviceType_t)(LVar) > STVIN_DEVICE_TYPE_ST7109_DVP_INPUT) )
    {
        STTST_TagCurrentLine( pars_p, "expected Device type 7015=0, 7020=1, ST40GX1=2, ST5528=3, ST7710=4, ST7100=5, ST7109=6");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    Device = (STVIN_DeviceType_t)(LVar);
    VINInitParams.DeviceType = Device;

    /* Get input mode */
    RetErr = STTST_GetInteger( pars_p, (S32)VIN_INPUT_MODE, (S32*)&LVar);
    if ( ( RetErr == TRUE ) || ((STVIN_InputMode_t)(LVar) > STVIN_HD_YCbCr_720_480_P_CCIR) )
    {
        STTST_TagCurrentLine( pars_p, "expected Input mode :\n"
                              "\tSD: ntsc720_480=0(def), ntsc640_480=1,   pal720_576=2,  pal768_576=3\n"
                              "\tHD: Ycbcr1920_1080=4  , Ycbcr1280_720=5, Rgb1024_768=6, Rgb800_600=7, Rgb640_480=8, Ycbcr720_480=9\n");

        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    Input = (STVIN_InputMode_t)(LVar);
    VINInitParams.InputMode  = Input;

    RetErr = STTST_GetInteger( pars_p, (S32)VIN_DEVICE_BASE_ADDRESS, (S32*)&LVar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected base address");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }

    VINInitParams.DeviceBaseAddress_p = (void *)LVar;
    RetErr = STTST_GetInteger( pars_p, (S32)VIN_BASE_ADDRESS, (S32*)&LVar);
    if ( RetErr == TRUE )
    {
#if defined(ST_7015) || defined (ST_7020)
        STTST_TagCurrentLine( pars_p, "Expected SD0(0x7000)/SD1(0x7100)/HD(0x7200) base address");
#elif defined(ST_GX1)
        STTST_TagCurrentLine( pars_p, "Expected DVP0(0x000)/DVP1(0x100) base address");
#endif
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    VINInitParams.RegistersBaseAddress_p = (void *)LVar;

#ifdef ST_MasterDigInput
    /* */
   strcpy(VINInitParams.InterruptEventName, "");
#if defined(ST_7710)
            VINInitParams.InterruptEvent = ST7710_DVP_INTERRUPT;
#elif defined(ST_7100)
            VINInitParams.InterruptEvent = ST7100_DVP_INTERRUPT;
#elif defined(ST_7109)
            VINInitParams.InterruptEvent = ST7109_DVP_INTERRUPT;
#elif defined(ST_5528)
            VINInitParams.InterruptEvent = ST5528_DVP_INTERRUPT;
#else

    switch (LVar)
    {
#if defined(ST_7015)
        case ST7015_SDIN1_OFFSET:
            VINInitParams.InterruptEvent = ST7015_SDIN1_INT;
            break;
        case ST7015_SDIN2_OFFSET:
            VINInitParams.InterruptEvent = ST7015_SDIN2_INT;
            break;
        case ST7015_HDIN_OFFSET:
            VINInitParams.InterruptEvent = ST7015_HDIN_INT;
            break;
#elif defined(ST_7020)
        case ST7020_SDIN1_OFFSET:
            VINInitParams.InterruptEvent = ST7020_SDIN1_INT;
            break;
        case ST7020_SDIN2_OFFSET:
            VINInitParams.InterruptEvent = ST7020_SDIN2_INT;
            break;
        case ST7020_HDIN_OFFSET:
            VINInitParams.InterruptEvent = ST7020_HDIN_INT;
            break;
#elif defined(ST_GX1)
        case GX1_DVP0_OFFSET:
            VINInitParams.InterruptEvent = GX1_DVP0_INT;
            break;
        case GX1_DVP1_OFFSET:
            VINInitParams.InterruptEvent = GX1_DVP1_INT;
            break;
#endif
        default:
            STTST_TagCurrentLine( pars_p, "Expected base address");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return( API_EnableError );
    }
    sprintf( VINInitParams.InterruptEventName, STINTMR_DEVICE_NAME);
#endif /* ST_7710 */
#endif /* ST_MasterDigInput */

    /* Get Max open parameter */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&MaxOpen);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected max open init parameter (default=1)");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    VINInitParams.MaxOpen = MaxOpen; /* MaxOpen */

    /* Get Event handler name */
    RetErr = STTST_GetString( pars_p, STEVT_DEVICE_NAME, EvtName, sizeof(EvtName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected EVENT handler name");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    sprintf( VINInitParams.EvtHandlerName, EvtName);

#ifdef ST_MasterDigInput
    /* Get Video handler name */
    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME1, VidName, sizeof(VidName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected VIDEO handler name");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    sprintf( VINInitParams.VidHandlerName, VidName);

    /* Get Clock Recovery handler name */
#ifdef USE_CLKRV
    RetErr = STTST_GetString( pars_p, STCLKRV_DEVICE_NAME, ClockRecoveryName, sizeof(ClockRecoveryName) );
#else
    RetErr = STTST_GetString( pars_p, "", ClockRecoveryName, sizeof(ClockRecoveryName) );
#endif
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected CLOCK RECOVERY handler name");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    sprintf( VINInitParams.ClockRecoveryName, ClockRecoveryName);

#endif /* ST_MasterDigInput */

    /* Get user data buffer number. */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected UserData buffer number (default=2)");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    VINInitParams.UserDataBufferNumber = (U32)LVar;

    /* Get user data buffer size. */
    RetErr = STTST_GetInteger( pars_p, (19*1024), (S32*)&LVar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected UserData buffer size (default=19Ko)");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return( API_EnableError );
    }
    VINInitParams.UserDataBufferSize = (U32)LVar;

    /* init parameters : */
    VINInitParams.CPUPartition_p  = SystemPartition_p; /* CPUPartition */
#ifdef ST_OSLINUX
        /* use LMI_SYS by default */
    VINInitParams.AVMEMPartition = (STAVMEM_PartitionHandle_t) 0; /* Partition 0: Only the index is passed, conversion done in kstvin */
#else
    VINInitParams.AVMEMPartition = AvmemPartitionHandle[0];
#endif /* ST_OSLINUX */

    /* init */
   /* all input param. have been checked */
   if (!(RetErr))
   {
       RetErr = TRUE;
       sprintf(VIN_Msg, "STVIN_Init('%s'): ", DeviceName );
       ErrCode = STVIN_Init(DeviceName, &VINInitParams);
       RetErr = VIN_TextError(ErrCode, VIN_Msg);

       if ( ErrCode == ST_NO_ERROR)
       {
            sprintf(VIN_Msg, "\tdevice type=%s, input mode=%s, maxopen %d\n",
                TabDeviceTyp[ VINInitParams.DeviceType],
                TabInputMod[VINInitParams.InputMode],
                VINInitParams.MaxOpen);
            sprintf(VIN_Msg, "%s\tUserDataBufferNumber %d UserDataBufferSize %d\n",
                VIN_Msg,
                VINInitParams.UserDataBufferNumber,
                VINInitParams.UserDataBufferSize);
            sprintf(VIN_Msg, "%s\tBaseaddress: device %x, InAd %x\n",
                VIN_Msg,
                (int)VINInitParams.DeviceBaseAddress_p,
                (int)VINInitParams.RegistersBaseAddress_p);
            sprintf(VIN_Msg, "%s\tevt='%s', vid='%s', clkrv='%s', intmr='%s'\n",
                VIN_Msg,
                VINInitParams.EvtHandlerName,
                VINInitParams.VidHandlerName,
                VINInitParams.ClockRecoveryName,
                VINInitParams.InterruptEventName);
            VIN_PRINT( VIN_Msg );
       }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_Init */



/*-------------------------------------------------------------------------
 * Function : VIN_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVIN_OpenParams_t VINOpenParams;
    char DeviceName[40];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;

    /* Open a VIN device connection */

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVIN_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
    }

    if (!(RetErr))
    {
        sprintf( VIN_Msg, "STVIN_Open('%s'): ", DeviceName );
        ErrCode = STVIN_Open(DeviceName, &VINOpenParams, &VINHndl);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if ( ErrCode == ST_NO_ERROR)
        {
            STTST_AssignInteger( result_sym_p, (int)VINHndl, FALSE);
            STTST_AssignInteger("RET_VAL1", (int)VINHndl, FALSE);
            sprintf(VIN_Msg, "\thnd=%d\n", (int)VINHndl );
            VIN_PRINT( VIN_Msg );
        }
    }
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);

    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_Open */


#ifdef ST_MasterDigInput
/*-------------------------------------------------------------------------
 * Function : VIN_Start
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_Start( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    STVIN_StartParams_t StartParams;

    /* Close a VIN device connection */
    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        /* Parameter is Ok */
        sprintf( VIN_Msg, "STVIN_Start( hndl: %d): ", (int)VINHndl );
        ErrCode = STVIN_Start(VINHndl, &StartParams);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    } /* end if */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_Start */

/*-------------------------------------------------------------------------
 * Function : VIN_Stop
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_Stop( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;

    /* Close a VIN device connection */
    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        sprintf( VIN_Msg, "STVIN_Stop(%d, STVIN_STOP_NOW): ", (int)VINHndl);
        ErrCode = STVIN_Stop(VINHndl, STVIN_STOP_NOW);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    } /* end if */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_Stop */
#endif /* ST_MasterDigInput */

/*-------------------------------------------------------------------------
 * Function : VIN_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;

    /* Close a VIN device connection */
    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        sprintf( VIN_Msg, "STVIN_Close(%d): ", (int)VINHndl);
        ErrCode = STVIN_Close(VINHndl);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    } /* end if */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_Close */



/*-------------------------------------------------------------------------
 * Function : VIN_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
  	STVIN_TermParams_t VINTermParams;
    long LVar;
  	char DeviceName[80];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVIN_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
    }

    if (!RetErr)
    {
        /* Get force terminate */
        RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "Expected force terminate (default=FALSE)");
        }
        else
        {
            VINTermParams.ForceTerminate = (U32)LVar;
        }
    }

    if (!RetErr)
    {
        sprintf( VIN_Msg, "STVIN_Term ('%s'): ", DeviceName);
        ErrCode = STVIN_Term(DeviceName, &VINTermParams);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );
} /* end VIN_Term */


/*-------------------------------------------------------------------------
 * Function : VIN_SetSyncType
 *            (set the active line lenght of the input video)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetSyncType( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
  	long LVar;
	STVIN_SyncType_t Sync;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    if(!RetErr)
    {
        /* get stream synchronisation type (default: 0=STVIN_SYNC_TYPE_EXTERNAL) */
        RetErr = STTST_GetInteger( pars_p, STVIN_SYNC_TYPE_EXTERNAL, (S32*)&LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Sync Type external=0(def), ccir mode=1\n");
        }
        else
        {
            Sync = (STVIN_SyncType_t)LVar;
        }
    }

    if(!RetErr)
    {
        sprintf( VIN_Msg, "STVIN_SetSyncType( hndl: %d, Sync: %s ): ", (int)VINHndl, TabSync[LVar] );
        ErrCode = STVIN_SetSyncType(VINHndl, Sync);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if ( ErrCode == ST_NO_ERROR)
        {
            sprintf(VIN_Msg, "\tsync=%s\n", TabSync[LVar] );
            VIN_PRINT( VIN_Msg );
        }
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetSyncType */


/*-------------------------------------------------------------------------
 * Function : VIN_SetFrameRate
 *            (set the input frame rate)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetFrameRate( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode  = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
  	long LVar;
    U32 FrameRt;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get input frame rate (default: 30000=60I) */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger( pars_p, 30000, (S32*)&LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Frame Rate: 60I:30000(default), 50I:25000, 60P:60000, 50P:50000\n");
        }
        FrameRt = (U32)LVar;
    }
    /* Parameters are Ok */
    if (!(RetErr))
    {
        sprintf( VIN_Msg, "STVIN_SetFrameRate( hndl: %d, FrameRate: %d ): ", (int)VINHndl, (int)FrameRt );
        ErrCode = STVIN_SetFrameRate(VINHndl, FrameRt);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    } /* end if */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetFrameRate */

/*-------------------------------------------------------------------------
 * Function : VIN_GetFrameRate
 *            (Get the input frame rate)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetFrameRate( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode  = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    U32 FrameRt;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        sprintf(VIN_Msg, "STVIN_GetFrameRate( hndl=%d ) : ",(int)VINHndl );
        ErrCode = STVIN_GetFrameRate( VINHndl, &FrameRt );
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if( ErrCode == ST_NO_ERROR )
        {
            sprintf(VIN_Msg, "\tFrameRate = %d\n",(int)FrameRt );
            VIN_PRINT((VIN_Msg ));
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return (API_EnableError ? RetErr : FALSE );

} /* End of VIN_GetFrameRate*/

/*-------------------------------------------------------------------------
 * Function : VIN_SetInputWindowMode
 *            (set the input window mode )
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetInputWindowMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    BOOL Auto;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get AutoMode */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, FALSE, (S32 *)&Auto );
        if (RetErr)
        {
            sprintf(VIN_Msg, "expected auto (%d=TRUE,%d=FALSE)", TRUE, FALSE );
            STTST_TagCurrentLine(pars_p, VIN_Msg );
        }
    }
    /* if parameter is ok */
    if (!(RetErr))
    {
        sprintf( VIN_Msg, "STVIN_SetInputWindowMode( hndl: %d, AutoMode:%d ): ", (int)VINHndl, Auto );
        ErrCode = STVIN_SetInputWindowMode(VINHndl, Auto);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetInputWindowMode */


/*-------------------------------------------------------------------------
 * Function : VIN_GetInputWindowMode
 *            Get input window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetInputWindowMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL AutoMode;
    BOOL RetErr;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        sprintf(VIN_Msg, "STVIN_GetInputWindowMode( hndl=%d ) : ", (int)VINHndl );
        ErrCode = STVIN_GetInputWindowMode( VINHndl, &AutoMode );
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if( ErrCode == ST_NO_ERROR )
        {
            sprintf(VIN_Msg, "\tAutoMode = %d\n",AutoMode );
            VIN_PRINT((VIN_Msg ));
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return (API_EnableError ? RetErr : FALSE );

} /* end VIN_GetInputWindowMode() */

/*-------------------------------------------------------------------------
 * Function : VIN_SetOutputWindowMode
 *            (set the Output window mode )
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetOutputWindowMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    BOOL Auto;

   /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get AutoMode */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, FALSE, (S32 *)&Auto );
        if (RetErr)
        {
            sprintf(VIN_Msg, "expected auto (%d=TRUE,%d=FALSE)", TRUE, FALSE );
            STTST_TagCurrentLine(pars_p, VIN_Msg );
        }
    }
    /* if parameter is ok */
    if (!(RetErr))
    {
        sprintf( VIN_Msg, "STVIN_SetOutputWindowMode( hndl: %d, AutoMode: %d): ", (int)VINHndl, Auto );
        ErrCode = STVIN_SetOutputWindowMode(VINHndl, Auto);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetOutputWindowMode */

/*-------------------------------------------------------------------------
 * Function : VIN_GetOutputWindowMode
 *            Get output window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetOutputWindowMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL AutoMode;
    BOOL RetErr;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        sprintf(VIN_Msg, "STVIN_GetOutputWindowMode( hndl=%d ) : ", (int)VINHndl );
        ErrCode = STVIN_GetOutputWindowMode( VINHndl, &AutoMode );
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if( ErrCode == ST_NO_ERROR )
        {
            sprintf(VIN_Msg, "\tAutoMode = %d\n",AutoMode );
            VIN_PRINT((VIN_Msg ));
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return (API_EnableError ? RetErr : FALSE );

} /* end VIN_GetOutputWindowMode() */


/*-------------------------------------------------------------------------
 * Function : VIN_SetIOWindows
 *            Adjust the input & output windows sizes and positions
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetIOWindows(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;

    S16 InputWinPositionX;
    S16 InputWinPositionY;
    S16 InputWinWidth;
    S16 InputWinHeight;
    S16 OutputWinWidth;
    S16 OutputWinHeight;

    BOOL RetErr;
    S32 LVar;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get InputWinPositionX */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        InputWinPositionX = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinPositionX" );
        }
    }
    /* get InputWinPositionY */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, 0, &LVar );
        InputWinPositionY = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinPositionY" );
        }
    }
    /* get InputWinWidth */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_WIDTH, &LVar );
        InputWinWidth = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinWidth" );
        }
    }
    /* get InputWinHeight */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_HEIGHT, &LVar );
        InputWinHeight = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinHeight" );
        }
    }

    /* get OutputWinWidth */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_WIDTH, &LVar );
        OutputWinWidth = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinWidth" );
        }
    }
    /* get OutputWinHeight */
    if (!(RetErr))
    {
        RetErr = STTST_GetInteger(pars_p, DEFAULT_SCREEN_HEIGHT, &LVar );
        OutputWinHeight = LVar;
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinHeight" );
        }
    }

    if (!(RetErr)) /* if parameters are ok */
    {
        sprintf(VIN_Msg, "STVIN_SetIOWindows( hndl:%d, ix=%d, iy=%d, iw=%d, ih=%d, ow=%d, oh=%d): ",
                (int)VINHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinWidth, OutputWinHeight );
        ErrCode = STVIN_SetIOWindows( VINHndl,
                                      InputWinPositionX, InputWinPositionY,
                                      InputWinWidth, InputWinHeight,
                                      OutputWinWidth, OutputWinHeight );

        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return (API_EnableError ? RetErr : FALSE );

} /* end of VIN_SetIOWindows() */


/*-------------------------------------------------------------------------
 * Function : VIN_GetIOWindows
 *            Retrieve the input and output window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetIOWindows(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    BOOL RetErr;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }
    else
    {
        sprintf(VIN_Msg, "STVIN_GetIOWindows( hndl=%d ): ", (int)VINHndl);
        ErrCode = STVIN_GetIOWindows( VINHndl,
                                      &InputWinPositionX, &InputWinPositionY,
                                      &InputWinWidth, &InputWinHeight,
                                      &OutputWinWidth, &OutputWinHeight );

        RetErr = VIN_TextError(ErrCode, VIN_Msg);
        if( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", InputWinPositionX, FALSE);
            STTST_AssignInteger("RET_VAL2", InputWinPositionY, FALSE);
            STTST_AssignInteger("RET_VAL3", InputWinWidth, FALSE);
            STTST_AssignInteger("RET_VAL4", InputWinHeight, FALSE);
            STTST_AssignInteger("RET_VAL5", OutputWinWidth, FALSE);
            STTST_AssignInteger("RET_VAL6", OutputWinHeight, FALSE);
            sprintf(VIN_Msg, "\tix=%d, iy=%d, iw=%d, ih=%d, ow=%d, oh=%d\n",
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinWidth, OutputWinHeight );
            VIN_PRINT((VIN_Msg ));
        }
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return (API_EnableError ? RetErr : FALSE );

} /* end VIN_GetIOWindows() */

/*-------------------------------------------------------------------------
 * Function : VIN_SetScanType
 *            (set the active line lenght of the input video)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetScanType( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
  	long LVar;
	STGXOBJ_ScanType_t Scan;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get the possible television display and stream scan type (default: 0=STGXOBJ_PROGRESSIVE_SCAN) */
    if(!RetErr)
    {
        RetErr = STTST_GetInteger( pars_p, STGXOBJ_PROGRESSIVE_SCAN, (S32*)&LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Scan Type progressive=0(def), interlaced=1\n");
        }
        else
        {
            Scan = (STGXOBJ_ScanType_t) LVar;
        }
    }

    if(!RetErr)
    {
        sprintf( VIN_Msg, "STVIN_SetScanType( hndl: %d, Scan: %s ): ", (int)VINHndl, TabScan[LVar] );
        ErrCode = STVIN_SetScanType(VINHndl, Scan);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetScanType */


/*-------------------------------------------------------------------------
 * Function : VIN_SetAncillaryParameters
 *            (set the active line lenght of the input video)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetAncillaryParameters( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    long LVar, AncInd;
    STVIN_AncillaryDataType_t Data = STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED;
    U16 Length = 0;

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get format of the incomming ancillary data (default: 0=STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED) */
    if(!RetErr)
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&AncInd);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Ancillary Data Type >0 (0/1/2)\n");
        }
        else
        {
            AncInd = puiss2((int)(AncInd));
            Data = (STVIN_AncillaryDataType_t)(AncInd);
        }
    }

    /* get size of the incomming ancillary data (default: 0) */
    if(!RetErr)
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Ancillary Data size >0\n");
        }
        else
        {
            Length = (U16) LVar;
        }
    }

    if(!RetErr)
    {
        RetErr = TRUE;
        sprintf( VIN_Msg, "STVIN_SetAncillaryParameters( hndl:%d, data=%s length=%d, d:%d ): ", (int)VINHndl,
             TabAnc[shift(AncInd)-1], Length, Data );
        ErrCode = STVIN_SetAncillaryParameters(VINHndl, Data, Length);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetAncillaryParameters */


/*-------------------------------------------------------------------------
 * Function : VIN_SetInputType
 *            (set the active line lenght of the input video)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetInputType( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    long LVar;
    STVIN_InputType_t Input;

    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
    }

    /* get the operational mode of the interface (default: 0=STVIN_INPUT_DIGITAL_YCbCr422_8_MULT) */
    if(!RetErr)
    {
        RetErr = STTST_GetInteger( pars_p, STVIN_INPUT_DIGITAL_YCbCr422_8_MULT, (S32*)&LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Input Type 0(def)-5\n");
        }
        else
        {
            Input = (STVIN_InputType_t)LVar;
        }
    }

    if(!RetErr)
    {
        sprintf( VIN_Msg, "STVIN_SetInputType( hndl:%d, Input=%s ): ", (int)VINHndl, TabInputTyp[LVar] );
        ErrCode = STVIN_SetInputType(VINHndl, Input);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetInputType */

/*-------------------------------------------------------------------------
 * Function : VIN_SetParams
 *            (set parameters)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_SetParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    S32 Result;
    STVIN_VideoParams_t Params;
    STVIN_InputType_t                   Input,      DefInput      = STVIN_INPUT_DIGITAL_YCbCr422_8_MULT;
    STGXOBJ_ScanType_t                  Scan,       DefScan       = STGXOBJ_INTERLACED_SCAN;
    STVIN_SyncType_t                    Sync,       DefSync       = STVIN_SYNC_TYPE_CCIR;
    STVIN_InputPath_t                   InputPath,  DefInputPath  = STVIN_InputPath_PAD;
    U16                                 FrameW,     DefFrameW     = 720;
    U16                                 FrameH,     DefFrameH     = 480;
    U16                                 HBlankOff,  DefHBlankOff  = 0;
    U16                                 VBlankOff,  DefVBlankOff  = 0;
    U16                                 HLowLim,    DefHLowLim    = 0;
    U16                                 HUpLim,     DefHUpLim     = 0x210;
    STVIN_ActiveEdge_t                  HSync,      DefHSync      = STVIN_RISING_EDGE;
    STVIN_ActiveEdge_t                  VSync,      DefVSync      = STVIN_RISING_EDGE;
    U16                                 HSyncOutH,  DefHSyncOutH  = 0;
    U16                                 HVSyncOutL, DefHVSyncOutL = 0;
    U16                                 VVSyncOutL, DefVVSyncOutL = 0;
    STVIN_Standard_t                    Standard,   DefStandard   = STVIN_STANDARD_NTSC;
    STVIN_AncillaryDataType_t           AncData,    DefAncData    = 2 /*shift(STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED)-1*/;

    U16                                 DataLength, DefDataLength = 0;
    STVIN_FirstPixelSignification_t     FPixSig,    DefFPixSig    = STVIN_FIRST_PIXEL_IS_COMPLETE;
    STVIN_ActiveEdge_t                  Clock,      DefClock      = STVIN_RISING_EDGE;
    STVIN_FieldDetectionMethod_t        Method,     DefMethod     = STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING;
    STVIN_PolarityOfUpperFieldOutput_t  Polarity,   DefPolarity   = STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH;
    char Str[8];

    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }

    /* get parameters : */
/*
  typedef struct
  {
  STVIN_InputType_t           InputType;
  STGXOBJ_ScanType_t          ScanType;
  STVIN_SyncType_t            SyncType;
  STVIN_InputPath_t           InputPath;
  U16                         FrameWidth;
  U16                         FrameHeight;
  U16                         HorizontalBlankingOffset;
  U16                         VerticalBlankingOffset;
  U16                         HorizontalLowerLimit;
  U16                         HorizontalUpperLimit;
  STVIN_ActiveEdge_t          HorizontalSyncActiveEdge;
  STVIN_ActiveEdge_t          VerticalSyncActiveEdge;
  U16                         HSyncOutHorizontalOffset;
  U16                         HorizontalVSyncOutLineOffset;
  U16                         VerticalVSyncOutLineOffset;
  STVIN_ExtraParams_t         ExtraParams;
  } STVIN_VideoParams_t;
*/

    /* get the operational mode of the interface (default: 1=STVIN_INPUT_DIGITAL_YCbCr422_8_MULT) */
    RetErr = STTST_GetInteger( pars_p, DefInput, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Input Type 0-5 (default 0:Ycbcr422_8_mult)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    Input = (STVIN_InputType_t) Result;
    /* get the possible television display and stream scan type (default: 1= STGXOBJ_INTERLACED_SCAN) */
    RetErr = STTST_GetInteger( pars_p, DefScan, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Scan Type 0-1 (default 1:interlaced)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    Scan = (STGXOBJ_ScanType_t) Result;
    /* get stream synchronisation type (default: 0=STVIN_SYNC_TYPE_EXTERNAL) */
    RetErr = STTST_GetInteger( pars_p, DefSync, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Sync Type 0-1 (default 0:external)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    Sync = (STVIN_SyncType_t) Result;
    /* get data input path (default: 0=PAD) */
    RetErr = STTST_GetInteger( pars_p, DefInputPath, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Input Path 0-1 (default 0:PAD)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    InputPath = (STVIN_InputPath_t) Result;
    /* get Frame Width and Height */
    RetErr = STTST_GetInteger( pars_p, DefFrameW, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Frame Width >0 (default 720)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    FrameW = (U16) Result;
    RetErr = STTST_GetInteger( pars_p, DefFrameH, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Frame Height >0 (default 480)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    FrameH = (U16) Result;
    /* get HorizontalBlankingOffset and VerticalBlankingOffset */
    RetErr = STTST_GetInteger( pars_p, DefHBlankOff, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Horizontal Blanking Offset >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    HBlankOff = (U16) Result;
    RetErr = STTST_GetInteger( pars_p, DefVBlankOff, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Vertical Blanking Offset >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    VBlankOff = (U16) Result;
    /* get HorizontalLowerLimit and HorizontalUpperLimit */
    RetErr = STTST_GetInteger( pars_p, DefHLowLim, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Horizontal Lower Limit >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    HLowLim = (U16) Result;
    RetErr = STTST_GetInteger( pars_p, DefHUpLim, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Horizontal Upper Limit >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    HUpLim = (U16) Result;
    /* get HorizontalSyncActiveEdge and VerticalSyncActiveEdge */
    RetErr = STTST_GetInteger( pars_p, DefHSync, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Horizontal Sync Active Edge >0 (default 0:rising edge)");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    HSync = (STVIN_ActiveEdge_t) Result;
    RetErr = STTST_GetInteger( pars_p, DefVSync, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Vertical Sync Active Edge >0 (default 0:rising edge)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    VSync = (STVIN_ActiveEdge_t) Result;
    /* get HSyncOutHorizontalOffset, HorizontalVSyncOutLineOffset and VerticalVSyncOutLineOffset */
    RetErr = STTST_GetInteger( pars_p, DefHSyncOutH, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected HSync Out Horizontal Offset >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    HSyncOutH = (U16) Result;
    RetErr = STTST_GetInteger( pars_p, DefHVSyncOutL, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Horizontal VSync Out Line Offset >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    HVSyncOutL = (U16) Result;
    RetErr = STTST_GetInteger( pars_p, DefVVSyncOutL, &Result);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Vertical VSync Out Line Offset >0 (default 0)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    VVSyncOutL = (U16) Result;

    Params.InputType                    = Input;
    Params.ScanType                     = Scan;
    Params.SyncType                     = Sync;
    Params.FrameWidth                   = FrameW;
    Params.FrameHeight                  = FrameH;
    Params.InputPath                    = InputPath;
    Params.HorizontalBlankingOffset     = HBlankOff;
    Params.VerticalBlankingOffset       = VBlankOff;
    Params.HorizontalLowerLimit         = HLowLim;
    Params.HorizontalUpperLimit         = HUpLim;
    Params.HorizontalSyncActiveEdge     = HSync;
    Params.VerticalSyncActiveEdge       = VSync;
    Params.HSyncOutHorizontalOffset     = HSyncOutH;
    Params.HorizontalVSyncOutLineOffset = HVSyncOutL;
    Params.VerticalVSyncOutLineOffset   = VVSyncOutL;

    /* SD or HD */
    /* Standard/High Definition Parameters  - Used by STVIN_ExtraParams_t */
    STTST_GetItem(pars_p, "SD", Str, sizeof(Str));
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected SD or HD (default SD)\n");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }
    if ( strcmp( Str, "HD") == 0)
    {
        RetErr = STTST_GetInteger( pars_p, DefClock, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Clock Active Edge >0  (default 0: rising edge)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        Clock = (STVIN_ActiveEdge_t) Result;
        RetErr = STTST_GetInteger( pars_p, DefMethod, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Detection Method 0-1 (default 0:detect line counting)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        Method = (STVIN_FieldDetectionMethod_t) Result;
        RetErr = STTST_GetInteger( pars_p, DefPolarity, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Polarity 0-1  (default 0:active high)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        Polarity = (STVIN_PolarityOfUpperFieldOutput_t) Result;

        Params.ExtraParams.HighDefinitionParams.ClockActiveEdge = Clock;
        Params.ExtraParams.HighDefinitionParams.DetectionMethod = Method;
        Params.ExtraParams.HighDefinitionParams.Polarity        = Polarity;
    }
    else /* SD */
    {
        RetErr = STTST_GetInteger( pars_p, DefStandard, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Standard Type 0-4 (default 2:ntsc)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        Standard = (STVIN_Standard_t) Result;
        RetErr = STTST_GetInteger( pars_p, DefAncData, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Ancillary Data Type >0 (0/1/2)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        Result = puiss2((int)(Result));
        AncData = (STVIN_AncillaryDataType_t)(Result);
        RetErr = STTST_GetInteger( pars_p, DefDataLength, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Ancillary Data Capture Length >0 (default 0:data capature disabled)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        DataLength = (U16) Result;

        RetErr = STTST_GetInteger( pars_p, DefFPixSig, &Result);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected First Pixel Signification 0-1 (default 0: STVIN_FIRST_PIXEL_IS_COMPLETE)\n");
            STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
            return ( API_EnableError );
        }
        FPixSig = (STVIN_FirstPixelSignification_t) Result;

        Params.ExtraParams.StandardDefinitionParams.StandardType               = Standard;
        Params.ExtraParams.StandardDefinitionParams.AncillaryDataType          = AncData;
        Params.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength = DataLength;
        Params.ExtraParams.StandardDefinitionParams.FirstPixelSignification    = FPixSig;
    }

    /* Parameters are Ok */
    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf( VIN_Msg, "STVIN_SetParams( hndl: %d ): ", (int)VINHndl );
        ErrCode = STVIN_SetParams(VINHndl, &Params);
        RetErr = VIN_TextError(ErrCode, VIN_Msg);
    } /* end if */

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_SetParams */


/*-------------------------------------------------------------------------
 * Function : VIN_GetParams
 *            (get parameters)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VIN_GetParams( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_ERROR_BAD_PARAMETER;
    BOOL RetErr;
    STVIN_VideoParams_t Params;
    STVIN_DefInputMode_t DefInputMode;
    BOOL Prnt;
    int LVar;

    RetErr = STTST_GetInteger( pars_p, (S32)VINHndl, (S32*)&VINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
        return ( API_EnableError );
    }

    /* Print ? */
    RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&LVar);
    if ( RetErr == TRUE )
    {
         STTST_TagCurrentLine( pars_p, "Expected Print FALSE(default)/TRUE");
         STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
         return ( API_EnableError );
    }
    Prnt = (BOOL)LVar;

    sprintf( VIN_Msg, "STVIN_GetParams(%d): ", (int)VINHndl );
    ErrCode = STVIN_GetParams(VINHndl, &Params, &DefInputMode);
    RetErr = VIN_TextError(ErrCode, VIN_Msg);

    STTST_AssignInteger( "DEFMOD", DefInputMode, FALSE);
    switch ( DefInputMode) {
        case STVIN_INPUT_INVALID :
            sprintf ( VIN_Msg, "\t!!! Warning : invalid input mode  ");
            if ( Prnt) {
                sprintf ( VIN_Msg, "%s (InputType %s, SyncType %s)",

                                       VIN_Msg,
                                       TabInputTyp[Params.InputType],
                                       TabSync[Params.SyncType]        );
            }
            STTBX_Print(( "%s  !!!\n", VIN_Msg ));
            RetErr = TRUE;
            break;

        case STVIN_INPUT_MODE_SD :
        case STVIN_INPUT_MODE_HD :
            if ( Prnt) {
                STTBX_Print(( "\tVideo Parameters :\n"));
                STTBX_Print(( "\t InputType                : %s\n",
                                    TabInputTyp[Params.InputType] ));
                STTBX_Print(( "\t ScanType                 : %s\n",
                                    TabScan[Params.ScanType] ));
                STTBX_Print(( "\t SyncType                 : %s\n",
                                    TabSync[Params.SyncType] ));
                STTBX_Print(( "\t InputPath                 : %s\n",
                                    TabPath[Params.InputPath] ));
                STTBX_Print(( "\t Frame                    : Width      %8.8d,\t Height   %8.8d\n",
                                    Params.FrameWidth,
                                    Params.FrameHeight ));
                STTBX_Print(( "\t BlankingOffset           : Horizontal %8.8d,\t Vertical %8.8d\n",
                                    Params.HorizontalBlankingOffset,
                                    Params.VerticalBlankingOffset ));
                STTBX_Print(( "\t Horizontal Limit         : Lower      %8.8d,\t Upper    %8.8d\n",
                                    Params.HorizontalLowerLimit,
                                    Params.HorizontalUpperLimit ));
                STTBX_Print(( "\t Sync ACtive Edge         : Horizontal %s,\t Vertical %s\n",
                                    TabEdge[Params.HorizontalSyncActiveEdge],
                                    TabEdge[Params.VerticalSyncActiveEdge]   ));
                STTBX_Print(( "\t HSyncOutHorizontalOffset : %8.8d\n",
                                    Params.HSyncOutHorizontalOffset ));
                STTBX_Print(( "\t VSyncOutLineOffset       : Horizontal %8.8d,\t Vertical %8.8d\n",
                                    Params.HorizontalVSyncOutLineOffset,
                                    Params.VerticalVSyncOutLineOffset));
            }
            STTST_AssignInteger( "INPTYP", Params.InputType, FALSE);
            STTST_AssignInteger( "SCANTY", Params.ScanType, FALSE);
            STTST_AssignInteger( "SYNCTY", Params.SyncType, FALSE);
            STTST_AssignInteger( "INPUTP", Params.InputPath, FALSE);
            STTST_AssignInteger( "FR_WID", Params.FrameWidth, FALSE);
            STTST_AssignInteger( "FR_HGH", Params.FrameHeight, FALSE);
            STTST_AssignInteger( "BL_HOR", Params.HorizontalBlankingOffset, FALSE);
            STTST_AssignInteger( "BL_VER", Params.VerticalBlankingOffset, FALSE);
            STTST_AssignInteger( "LI_HOR", Params.HorizontalLowerLimit, FALSE);
            STTST_AssignInteger( "LI_VER", Params.HorizontalUpperLimit, FALSE);
            STTST_AssignInteger( "SY_HOR", Params.HorizontalSyncActiveEdge, FALSE);
            STTST_AssignInteger( "SY_VER", Params.VerticalSyncActiveEdge, FALSE);
            STTST_AssignInteger( "HSYOHO", Params.HSyncOutHorizontalOffset, FALSE);
            STTST_AssignInteger( "VS_HOR", Params.HorizontalVSyncOutLineOffset, FALSE);
            STTST_AssignInteger( "VS_VER", Params.VerticalVSyncOutLineOffset, FALSE);
            if ( DefInputMode == STVIN_INPUT_MODE_SD)
            {
                if ( Prnt) {
                    STTBX_Print(( "\t Standard Def. Parameters :\n"));
                    STTBX_Print(( "\t  Standard Type                 %s\n",
                                        TabStandard[Params.ExtraParams.StandardDefinitionParams.StandardType] ));
                    STTBX_Print(( "\t  Ancillary Data Type           %s\n",
                                        TabAnc[shift(Params.ExtraParams.StandardDefinitionParams.AncillaryDataType)-1] ));
                    STTBX_Print(( "\t  Ancillary Data Capture Length %8.8d\n",
                                        Params.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength ));
                    STTBX_Print(( "\t  First Pixel Signification     %1.1d\n",
                                        Params.ExtraParams.StandardDefinitionParams.FirstPixelSignification ));
                }
                STTST_AssignInteger( "SD_STD", Params.ExtraParams.StandardDefinitionParams.StandardType, FALSE);
                STTST_AssignInteger( "SD_ANC", shift(Params.ExtraParams.StandardDefinitionParams.AncillaryDataType)-1, FALSE);
                STTST_AssignInteger( "SD_LGH", Params.ExtraParams.StandardDefinitionParams.AncillaryDataCaptureLength, FALSE);
                STTST_AssignInteger( "SD_FPX", Params.ExtraParams.StandardDefinitionParams.FirstPixelSignification, FALSE);
                STTST_AssignInteger( "HD_CLK", 0, FALSE);
                STTST_AssignInteger( "HD_MET", 0, FALSE);
                STTST_AssignInteger( "HD_POL", 0, FALSE);
            }
            else
            {
                if ( Prnt) {
                    STTBX_Print(( "\t High Def. Parameters     :\n"));
                    STTBX_Print(( "\t  Clock Active Edge             %s\n",
                                    TabEdge[Params.ExtraParams.HighDefinitionParams.ClockActiveEdge] ));
                    STTBX_Print(( "\t  Detection Method              %s\n",
                                    TabMethod[Params.ExtraParams.HighDefinitionParams.DetectionMethod] ));
                    STTBX_Print(( "\t  Polarity                      %s\n",
                                    TabPolarity[Params.ExtraParams.HighDefinitionParams.Polarity] ));
                }
                STTST_AssignInteger( "HD_CLK", Params.ExtraParams.HighDefinitionParams.ClockActiveEdge, FALSE);
                STTST_AssignInteger( "HD_MET", Params.ExtraParams.HighDefinitionParams.DetectionMethod, FALSE);
                STTST_AssignInteger( "HD_POL", Params.ExtraParams.HighDefinitionParams.Polarity, FALSE);
                STTST_AssignInteger( "SD_STD", 0, FALSE);
                STTST_AssignInteger( "SD_ANC", 0, FALSE);
                STTST_AssignInteger( "SD_LGH", 0, FALSE);
                STTST_AssignInteger( "SD_FPX", 0, FALSE);
            }
            break;
    }

    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
	return ( API_EnableError ? RetErr : FALSE );

} /* end VIN_GetParams */



/*-------------------------------------------------------------------------
 * Function : VIN_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VIN_InitCommand(void)
{
    BOOL RetErr = FALSE;

/* API functions : */
#ifdef ST_MasterDigInput
    RetErr |= STTST_RegisterCommand( "VIN_START"              , VIN_Start                 , "STVIN_Start('Handle') ");
    RetErr |= STTST_RegisterCommand( "VIN_STOP"               , VIN_Stop                  , "STVIN_Stop('Handle' ");
#endif /* ST_MasterDigInput */
    RetErr |= STTST_RegisterCommand( "VIN_CAPA"               , VIN_GetCapability         , "STVIN_GetCapability('DevName') ");
    RetErr |= STTST_RegisterCommand( "VIN_CLOSE"              , VIN_Close                 , "STVIN_Close('Handle' ");
#ifdef ST_MasterDigInput
    RetErr |= STTST_RegisterCommand( "VIN_INIT"               , VIN_Init                  , "STVIN_Init('DeviceName' 'DeviceType' 'InputMode' DevAd InAd 'MaxOpen' 'EvtName' 'VidName' 'ClkRecName') ");
#else
    RetErr |= STTST_RegisterCommand( "VIN_INIT"               , VIN_Init                  , "STVIN_Init('DeviceName' 'DeviceType' 'InputMode' DevAd InAd 'MaxOpen' 'EvtName') ");
#endif /* ST_MasterDigInput */
    RetErr |= STTST_RegisterCommand( "VIN_OPEN"               , VIN_Open                  , "STVIN_Open('DevName' ");
    RetErr |= STTST_RegisterCommand( "VIN_REVISION"           , VIN_GetRevision           , "STVIN_GetRevision() ");
    RetErr |= STTST_RegisterCommand( "VIN_TERM"               , VIN_Term                  , "STVIN_Term('DevName' 'ForceTerminate') ");
    RetErr |= STTST_RegisterCommand( "VIN_ANCILLARYPARAMETERS", VIN_SetAncillaryParameters, "STVIN_SetAncillaryParameters('Handle' 'AncillaryDataType' 'Lenght' ");
    RetErr |= STTST_RegisterCommand( "VIN_INPUTTYPE"          , VIN_SetInputType          , "STVIN_SetInputType('Handle' 'InputType') ");
    RetErr |= STTST_RegisterCommand( "VIN_SCANTYPE"           , VIN_SetScanType           , "STVIN_SetScanType('Handle' 'ScanType') ");
    RetErr |= STTST_RegisterCommand( "VIN_SYNCTYPE"           , VIN_SetSyncType           , "STVIN_SetSyncType('Handle' 'SyncType') ");
    RetErr |= STTST_RegisterCommand( "VIN_SETPARAMS"          , VIN_SetParams             , "STVIN_SetParams('Handle' 'InputType' 'ScanType' 'SyncType' 'InputPath' 'FrameWidth' 'FrameHeight' 'HBOff' 'VBOff' 'HlowLim' 'HUpLim' 'HSAE' 'VSAE' 'HSOutHOff' 'HVSOutLOff' 'VVSOutLOff' 'Std' 'StdTyp/CLkActEd' 'AncDataTyp/FDetMeth' 'ADCLght/Pol' 'FirstPixSig') ");
    RetErr |= STTST_RegisterCommand( "VIN_GETPARAMS"          , VIN_GetParams             , "STVIN_GetParams('Handle' 'Print') ");
    RetErr |= STTST_RegisterCommand( "VIN_FRAMERATE"          , VIN_SetFrameRate          , "STVIN_SetFrameRate('Handle' 'FrameRate') ");
    RetErr |= STTST_RegisterCommand( "VIN_GETFRAMER"          , VIN_GetFrameRate          , "STVIN_GetFrameRate('Handle')  Get the frame rate ");
    RetErr |= STTST_RegisterCommand( "VIN_SETIN"              , VIN_SetInputWindowMode    , "STVIN_SetInputWindowMode('Handle' 'AutoMode') ");
    RetErr |= STTST_RegisterCommand( "VIN_GETIN"              , VIN_GetInputWindowMode    , "STVIN_GetInputWindowMode('Handle) Get input window mode.");
    RetErr |= STTST_RegisterCommand( "VIN_SETOUT"             , VIN_SetOutputWindowMode   , "STVIN_SetOutputWindowMode('Handle' 'AutoMode') ");
    RetErr |= STTST_RegisterCommand( "VIN_GETOUT"             , VIN_GetOutputWindowMode   , "STVIN_GetOutputWindowMode('Handle') Get output window mode.");
    RetErr |= STTST_RegisterCommand( "VIN_SETIO"              , VIN_SetIOWindows          , "STVIN_SetIOWindows([<Handle>]<InX><InY><InWidth><InHeight><OutWidth><OutHeight>)" );
    RetErr |= STTST_RegisterCommand( "VIN_GETIO"              , VIN_GetIOWindows,          " STVIN_GetIOWindows('Handle') Get IO windows size & position.");


/* assign */
    RetErr |= STTST_AssignInteger ("TYPE_7015_DIGITAL_INPUT" , STVIN_DEVICE_TYPE_7015_DIGITAL_INPUT,TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_7020_DIGITAL_INPUT" , STVIN_DEVICE_TYPE_7020_DIGITAL_INPUT,TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_ST40GX1_DVP_INPUT"  , STVIN_DEVICE_TYPE_ST40GX1_DVP_INPUT, TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_5528_DVP_INPUT"     , STVIN_DEVICE_TYPE_ST5528_DVP_INPUT,  TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_7710_DVP_INPUT"     , STVIN_DEVICE_TYPE_ST7710_DVP_INPUT,  TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_7100_DVP_INPUT"     , STVIN_DEVICE_TYPE_ST7100_DVP_INPUT,  TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_7109_DVP_INPUT"     , STVIN_DEVICE_TYPE_ST7109_DVP_INPUT,  TRUE);

    RetErr |= STTST_AssignInteger ("MODE_SD"     , STVIN_INPUT_MODE_SD, TRUE);
    RetErr |= STTST_AssignInteger ("MODE_HD"     , STVIN_INPUT_MODE_HD, TRUE);
    RetErr |= STTST_AssignInteger ("INVALID_MODE", STVIN_INPUT_INVALID, TRUE);

    RetErr |= STTST_AssignInteger ("PATH_PAD", STVIN_InputPath_PAD, TRUE);
    RetErr |= STTST_AssignInteger ("PATH_DVO", STVIN_InputPath_DVO, TRUE);

    RetErr |= STTST_AssignInteger ("RISING_EDGE" , STVIN_RISING_EDGE, TRUE);
    RetErr |= STTST_AssignInteger ("FALLING_EDGE", STVIN_FALLING_EDGE, TRUE);

    RetErr |= STTST_AssignInteger ("TYPE_EXTERNAL", STVIN_SYNC_TYPE_EXTERNAL, TRUE);
    RetErr |= STTST_AssignInteger ("TYPE_CCIR"    , STVIN_SYNC_TYPE_CCIR, TRUE);

    RetErr |= STTST_AssignInteger ("PACKET_NIBBLE_ENCODED", (shift(STVIN_ANC_DATA_PACKET_NIBBLE_ENCODED)-1), TRUE);
    RetErr |= STTST_AssignInteger ("PACKET_DIRECT"        , (shift(STVIN_ANC_DATA_PACKET_DIRECT)-1), TRUE);
    RetErr |= STTST_AssignInteger ("CAPTURE"              , (shift(STVIN_ANC_DATA_RAW_CAPTURE)-1), TRUE);

    RetErr |= STTST_AssignInteger ("YCbCr422_8_MULT"        , STVIN_INPUT_DIGITAL_YCbCr422_8_MULT, TRUE);
    RetErr |= STTST_AssignInteger ("YCbCr422_16_CHROMA_MULT", STVIN_INPUT_DIGITAL_YCbCr422_16_CHROMA_MULT, TRUE);
    RetErr |= STTST_AssignInteger ("YCbCr444_24_COMP"       , STVIN_INPUT_DIGITAL_YCbCr444_24_COMP, TRUE);
    RetErr |= STTST_AssignInteger ("RGB888_24_TO_YCbCr422"  , STVIN_INPUT_DIGITAL_RGB888_24_COMP_TO_YCbCr422, TRUE);
    RetErr |= STTST_AssignInteger ("RGB888_24_COMP"         , STVIN_INPUT_DIGITAL_RGB888_24_COMP, TRUE);
    RetErr |= STTST_AssignInteger ("CD_SERIAL_MULT"         , STVIN_INPUT_DIGITAL_CD_SERIAL_MULT, TRUE);

    RetErr |= STTST_AssignInteger ("STD_PAL"    , STVIN_STANDARD_PAL, TRUE);
    RetErr |= STTST_AssignInteger ("STD_PAL_SQ" , STVIN_STANDARD_PAL_SQ, TRUE);
    RetErr |= STTST_AssignInteger ("STD_NTSC"   , STVIN_STANDARD_NTSC, TRUE);
    RetErr |= STTST_AssignInteger ("STD_NTSC_SQ", STVIN_STANDARD_NTSC_SQ, TRUE);

    RetErr |= STTST_AssignInteger ("LINE_COUNTING"         , STVIN_FIELD_DETECTION_METHOD_LINE_COUNTING, TRUE);
    RetErr |= STTST_AssignInteger ("RELATIVE_ARRIVAL_TIMES", STVIN_FIELD_DETECTION_METHOD_RELATIVE_ARRIVAL_TIMES, TRUE);

    RetErr |= STTST_AssignInteger ("POLARITY_ACTIVE_HIGH", STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_HIGH, TRUE);
    RetErr |= STTST_AssignInteger ("POLARITY_ACTIVE_LOW" , STVIN_UPPER_FIELD_OUTPUT_POLARITY_ACTIVE_LOW, TRUE);

    RetErr |= STTST_AssignInteger ("SD_NTSC_720_480_I_CCIR"   , STVIN_SD_NTSC_720_480_I_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("SD_NTSC_640_480_I_CCIR"   , STVIN_SD_NTSC_640_480_I_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("SD_PAL_720_576_I_CCIR"    , STVIN_SD_PAL_720_576_I_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("SD_PAL_768_576_I_CCIR"    , STVIN_SD_PAL_768_576_I_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("HD_YCbCr_1920_1080_I_CCIR", STVIN_HD_YCbCr_1920_1080_I_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("HD_YCbCr_1280_720_P_CCIR" , STVIN_HD_YCbCr_1280_720_P_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("HD_YCbCr_720_480_P_CCIR" , STVIN_HD_YCbCr_720_480_P_CCIR, TRUE);
    RetErr |= STTST_AssignInteger ("HD_RGB_1024_768_P_EXT"    , STVIN_HD_RGB_1024_768_P_EXT, TRUE);
    RetErr |= STTST_AssignInteger ("HD_RGB_800_600_P_EXT"     , STVIN_HD_RGB_800_600_P_EXT, TRUE);
    RetErr |= STTST_AssignInteger ("HD_RGB_640_480_P_EXT"     , STVIN_HD_RGB_640_480_P_EXT, TRUE);

    RetErr |= STTST_AssignInteger ("PROGRESSIVE_SCAN", STGXOBJ_PROGRESSIVE_SCAN, TRUE);
    RetErr |= STTST_AssignInteger ("INTERLACED_SCAN",  STGXOBJ_INTERLACED_SCAN, TRUE);

    RetErr |= STTST_AssignInteger ("FIRST_PIXEL_IS_COMPLETE", STVIN_FIRST_PIXEL_IS_COMPLETE, TRUE);
    RetErr |= STTST_AssignInteger ("FIRST_PIXEL_IS_NOT_COMPLETE", STVIN_FIRST_PIXEL_IS_NOT_COMPLETE, TRUE);

    /* Drive mapping depends on the Operating System ; use the exact lower/uppercase on Linux */
#ifdef ST_OSLINUX
    RetErr |= STTST_AssignString ("BISTRIMROOT",   "./streams/", FALSE);
    RetErr |= STTST_AssignString ("PATH_SCRIPTS",  "./scripts/vin/", FALSE);
    RetErr |= STTST_AssignString ("PATH_FIRMWARES","./scripts/vin/", FALSE);
    RetErr |= STTST_AssignString ("PATH_TESTS",    "./scripts/vin/", FALSE);
#else /*ST_OSLINUX*/
    RetErr |= STTST_AssignString ("BISTRIMROOT",   "//filer15/stb/dstapi/bistrim/", FALSE);
    RetErr |= STTST_AssignString ("PATH_SCRIPTS",  "../../../scripts/", FALSE);
    RetErr |= STTST_AssignString ("PATH_FIRMWARES","../../../../", FALSE);
    RetErr |= STTST_AssignString ("PATH_TESTS",    "../../", FALSE);
#endif /*ST_OSLINUX*/

    return( RetErr ? FALSE : TRUE);

} /* end VIN_MacroInit */



/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL VIN_RegisterCmd (void)
{
    BOOL RetOk;

    RetOk = VIN_InitCommand();
    if ( RetOk )
    {
        sprintf(VIN_Msg, "VIN_RegisterCmd()\t: ok           %s\n", STVIN_GetRevision() );
    }
    else
    {
        sprintf(VIN_Msg, "VIN_RegisterCmd()\t: failed !\n" );
    }
    STTST_Print((VIN_Msg));

    return(RetOk);
}
/* end of file */
