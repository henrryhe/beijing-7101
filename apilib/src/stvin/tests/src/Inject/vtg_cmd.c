/************************************************************************
COPYRIGHT (C) STMicroelectronics 2000

File name   : vtg_cmd.c
Description : VTG API commands

Note        :
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stsys.h"
#include "stlite.h"
#include "stvtg.h"
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/


/* --- Constants --- */
#define VTG_MAX_MODE  64
#define VTG_MAX_OPTION_MODE 5


typedef struct
{
    const char Mode[18];
    const STVTG_Option_t Value;
}vtg_ModeOption_t;


static const char ModeName[VTG_MAX_MODE][23] = {
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

    /* non standard modes */
    "MODE_480I60000_27027"  ,
    "MODE_480I59940_27000"  ,
    "MODE_480P59940_36000"  ,
    "MODE_576I50000_27000"  ,
    "MODE_720P50000_74250"  ,
    "MODE_1152I50000_74250" ,
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED",   /* 47 -> 49 */
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED",  "MODE_UNDEFINED", "MODE_UNDEFINED", /* 50 -> 54 */
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED",  "MODE_UNDEFINED", "MODE_UNDEFINED", /* 55 -> 59 */
    "MODE_UNDEFINED", "MODE_UNDEFINED", "MODE_UNDEFINED",  "MODE_UNDEFINED", "MODE_UNDEFINED"  /* 60 -> 64 */
    };

static const vtg_ModeOption_t VTGOptionMode[VTG_MAX_OPTION_MODE+1] = {
    { "EMBEDDED_SYNCHRO", STVTG_OPTION_EMBEDDED_SYNCHRO},
    { "NO_OUTPUT_SIGNAL", STVTG_OPTION_NO_OUTPUT_SIGNAL},
    { "HSYNC_POLARITY", STVTG_OPTION_HSYNC_POLARITY},
    { "VSYNC_POLARITY", STVTG_OPTION_VSYNC_POLARITY},
    { "OUT_EDGE_POSITION", STVTG_OPTION_OUT_EDGE_POSITION},
    { "????", 0}
};

/* --- Defined ---*/

#if defined (ST_5508) || defined (ST_5510) || defined (ST_5512) || defined (ST_5518) || defined (ST_5514)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_DENC
#define VTG_DEVICE_BASE_ADDRESS  0
#define VTG_BASE_ADDRESS         DENC_BASE_ADDRESS
#elif defined (ST_7015)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7015
#define VTG_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define VTG_BASE_ADDRESS         ST7015_VTG1_OFFSET
#elif defined (ST_7020)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VTG_CELL_7020
#define VTG_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define VTG_BASE_ADDRESS         ST7020_VTG1_OFFSET
#elif defined (ST_GX1)
#define VTG_DEVICE_TYPE          STVTG_DEVICE_TYPE_VFE
#define VTG_DEVICE_BASE_ADDRESS  0                      /* waiting to be defined in vob chip */
#define VTG_BASE_ADDRESS         (S32)(0xBB420000 + 0x200 )  /* waiting to be defined in vob chip */
#else
#error Not defined processor
#endif

/* --- Global variables --- */
/* --- Extern functions --- */

/* --- Extern variables --- */
extern ST_Partition_t *SystemPartition;

/* --- Private variables --- */
static STVTG_Handle_t VTGHndl;
static char   VTG_Msg[250];            /* text for trace */

extern ST_Partition_t *SystemPartition;

/* --- Macros ---*/
#define VTG_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(VTG_Msg)){ \
        sprintf(x, "VTG message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* prototype */

/* Local prototypes ------------------------------------------------------- */


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
            case STVTG_ERROR_DENC_ACCESS:
                strcat( Text, "(stvtg error denc access) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VTG_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

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
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t      ErrCode;
    long                lVar;

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
  	STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }

    /* Get device number */
    RetErr = STTST_GetInteger( pars_p, VTG_DEVICE_TYPE, (S32*)&lVar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Device DENC=0, GX1=1, 7015=2, 7020=3");
        return(TRUE);
    }
    VTGInitParams.DeviceType = (STVTG_DeviceType_t)lVar;

    if(VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_DENC)
    {
        /* Get denc name */
        RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, VTGInitParams.Target.DencName, \
                                  sizeof(VTGInitParams.Target.DencName) );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected DENC name");
            return(TRUE);
        }
    }
    else
    {
        /* Get Base Address */
        RetErr = STTST_GetInteger( pars_p, VTG_BASE_ADDRESS, (S32*)&lVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "Expected base address");
            return(TRUE);
        }
        VTGInitParams.Target.VtgCell.BaseAddress_p = (void*)lVar;
        VTGInitParams.Target.VtgCell.DeviceBaseAddress_p = (void*)VTG_DEVICE_BASE_ADDRESS;
#if defined (ST_5508) || defined (ST_5510) || defined (ST_5512) || defined (ST_5518) || defined (ST_5514)
#elif defined (ST_7015)
        if(VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG1_OFFSET)
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7015_VTG1_INT;
        else if(VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7015_VTG2_OFFSET)
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7015_VTG2_INT;
        else
            STTST_Print(("No interrupt found!!\n"));
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_7020)
        if(VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG1_OFFSET)
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7020_VTG1_INT;
        else if(VTGInitParams.Target.VtgCell.BaseAddress_p == (void*)ST7020_VTG2_OFFSET)
            VTGInitParams.Target.VtgCell.InterruptEvent = ST7020_VTG2_INT;
        else
            STTST_Print(("No interrupt found!!\n"));
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#elif defined (ST_GX1)
        VTGInitParams.Target.VtgCell.InterruptEvent = 0; /* VTG= Num 0 */
        strcpy(VTGInitParams.Target.VtgCell.InterruptEventName, STINTMR_DEVICE_NAME);
#else
#error Not yet defined
#endif
    }

    /* Get Max Open */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&VTGInitParams.MaxOpen);
    if ( RetErr == TRUE )
    {
  	STTST_TagCurrentLine( pars_p, "expected max open init parameter (default=1)");
        return(TRUE);
    }

    strcpy(VTGInitParams.EvtHandlerName, STEVT_DEVICE_NAME);

    if(VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_DENC){
        sprintf( VTG_Msg, "VTG_Init(%s, Type=%s, Name=%s, MaxOpen=%d): ", \
                 DeviceName, "55XX" , VTGInitParams.Target.DencName, VTGInitParams.MaxOpen);
    }
    else
    {
        sprintf( VTG_Msg, "VTG_Init(%s, Type=%s, BaseAdd=0x%0X, MaxOpen=%d): ", \
                 DeviceName,
                 (VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7015) ? "7015" :
                 (VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_VTG_CELL_7020) ? "7020" :
                 (VTGInitParams.DeviceType == STVTG_DEVICE_TYPE_VFE) ? "GX1" : "?????",
                 (U32)VTGInitParams.Target.VtgCell.BaseAddress_p, VTGInitParams.MaxOpen);
    }
    ErrCode = STVTG_Init(DeviceName, &VTGInitParams);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);
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
    ST_ErrorCode_t     ErrCode;

    /* get device */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VTG_Msg, "VTG_Open(%s): ", DeviceName);
    ErrCode = STVTG_Open(DeviceName, &VTGOpenParams, &VTGHndl);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

    STTST_AssignInteger(result_sym_p, VTGHndl, false);
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
    ST_ErrorCode_t ErrCode;

    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Open a VTG device connection */
    sprintf( VTG_Msg, "VTG_Close(%d): ", VTGHndl);

    ErrCode = STVTG_Close(VTGHndl);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : VTG_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL VTG_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_TermParams_t VTGTermParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    long lVar;

    /* Get device name */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
  	STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&lVar);
    if ( RetErr == TRUE )
    {
  	STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }

    VTGTermParams.ForceTerminate = (U32)lVar;

    sprintf( VTG_Msg, "VTG_Term(%s): ", DeviceName );
    ErrCode = STVTG_Term(DeviceName, &VTGTermParams);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

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
    ST_ErrorCode_t ErrCode;
    STVTG_Capability_t Capability;
    char StrParams[RETURN_PARAMS_LENGTH];
    char DeviceName[STRING_DEVICE_LENGTH];
    U32  Mode;

    /* get device */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf( VTG_Msg, "VTG_GetCapability(%s): ", DeviceName);
    ErrCode = STVTG_GetCapability(DeviceName, &Capability);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

    if ( ErrCode == ST_NO_ERROR)
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
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    char *ptr=NULL, TrvStr[80], IsStr[80];
    U32 Var;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get output mode  (default:  PAL Mode 480x576 at 50Hz ) */
    STTST_GetItem(pars_p, "PAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
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
        if(RetErr==TRUE){
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
    ErrCode = STVTG_SetMode(VTGHndl, ModeVtg);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

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
    ST_ErrorCode_t ErrCode;
    char StrParams[RETURN_PARAMS_LENGTH];
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    sprintf( VTG_Msg, "VTG_GetMode(%d) : ", VTGHndl);
    ErrCode = STVTG_GetMode(VTGHndl, &TimingMode, &ModeParams);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf( VTG_Msg, "\tMode: %s = %d\n", ModeName[TimingMode], TimingMode);
        sprintf( VTG_Msg, "%s\tFrameRate:%d  ScanType:%s\n"\
                 ,VTG_Msg , ModeParams.FrameRate,\
                 (ModeParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" : \
                 (ModeParams.ScanType == STGXOBJ_INTERLACED_SCAN) ? "INTER" : "?????");
        sprintf( VTG_Msg, "%s\tWidth:%d  Height:%d  XStart:%d  YStart:%d  DXStart:%d  DYStart:%d\n", \
                 VTG_Msg,
                 ModeParams.ActiveAreaWidth,
                 ModeParams.ActiveAreaHeight,
                 ModeParams.ActiveAreaXStart,
                 ModeParams.ActiveAreaYStart,
                 ModeParams.DigitalActiveAreaXStart,
                 ModeParams.DigitalActiveAreaYStart);
        sprintf( StrParams, "%d %d %d %d %d %d %d", \
                 TimingMode,
                 ModeParams.FrameRate,
                 ModeParams.ScanType,
                 ModeParams.ActiveAreaWidth,
                 ModeParams.ActiveAreaHeight,
#if defined (ST_5508) | defined (ST_5510) | defined (ST_5512) | defined (ST_5518) | defined (ST_5514)
                 ModeParams.ActiveAreaXStart,
                 ModeParams.ActiveAreaYStart
#elif defined (ST_7015) || defined (ST_7020) || defined (ST_GX1)
                 ModeParams.DigitalActiveAreaXStart,
                 ModeParams.DigitalActiveAreaYStart
#else
#error Not defined
#endif
            );
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VTG_PRINT(VTG_Msg);
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end VTG_GetMode */


/*-------------------------------------------------------------------------
 * Function : VTG_SetOptionalConfiguration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VTG_SetOptionalConfiguration( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STVTG_OptionalConfiguration_t OptionalParams_s;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 Var, i;
    char TrvStr[80], IsStr[80], *ptr;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "EMBEDDED_SYNCHRO", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VTG_MAX_OPTION_MODE; Var++){
        if((strcmp(VTGOptionMode[Var].Mode, TrvStr)==0) || \
           (strncmp(VTGOptionMode[Var].Mode, TrvStr+1, strlen(VTGOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==VTG_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr==TRUE){
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
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&OptionalParams_s.Value.OutEdges.HSyncRising);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected HSyncRising");
            return(TRUE);
        }
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&OptionalParams_s.Value.OutEdges.HSyncFalling);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected HSyncFalling");
            return(TRUE);
        }
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&OptionalParams_s.Value.OutEdges.VSyncRising);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected VSyncRising");
            return(TRUE);
        }
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&OptionalParams_s.Value.OutEdges.VSyncFalling);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected VSyncFalling");
            return(TRUE);
        }
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
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Boolean (default FALSE)");
            return(TRUE);
        }
        OptionalParams_s.Value.EmbeddedSynchro = (BOOL)Var;
        sprintf( VTG_Msg, "VTG_SetOptionalConfiguration(%d,Opt=%s,Val=%s) : ", VTGHndl,
                 VTGOptionMode[i].Mode,
                 (OptionalParams_s.Value.EmbeddedSynchro == TRUE) ? "TRUE" :
                 (OptionalParams_s.Value.EmbeddedSynchro == FALSE) ? "FALSE" : "????"
                 );
    }

    ErrCode = STVTG_SetOptionalConfiguration(VTGHndl, &OptionalParams_s);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

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
    long LVar;
    S32 Type;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get type for slave mode */
    RetErr = STTST_GetInteger( pars_p, VTG_DEVICE_TYPE, &Type);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Device DENC=0, GX1=1, 7015=2, 7020=3");
        return(TRUE);
    }

    /* Get output mode */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if(RetErr == TRUE)
    {
        STTST_TagCurrentLine( pars_p, "Expected slave mode : (default 0,1,2) <-> (H_AND_V, V_ONLY, H_ONLY)");
        return(TRUE);
    }
    SlaveModeParams_s.SlaveMode = (STVTG_SlaveMode_t)LVar;


    if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_DENC){
        /* Get DENC slave of */
        RetErr = STTST_GetInteger( pars_p, STVTG_DENC_SLAVE_OF_ODDEVEN, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected DENC slave of : (default 0,1,2) <-> (ODDEVEN,VSYNC,SYNC_IN_DATA)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.DencSlaveOf = (STVTG_DencSlaveOf_t)LVar;

        /* Get FreeRun */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected FreeRun : (default 0,1) <-> (FALSE, TRUE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.FreeRun = (BOOL)LVar;

        /* Get OutSyncAvailable */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected OutSyncAvailable : (default 0,1) <-> (FALSE, TRUE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.OutSyncAvailable = (BOOL)LVar;

        /* Get SyncInAdjust */
        RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected SyncInAdjust : (0,default 1,2,3) <-> (Nomi,+1,+2,+3)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.Denc.SyncInAdjust = (U8)LVar;
    }
    else if( ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7015) ||
             ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7020) )
    {
        /* Get H slave of */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, \
                                  "Expected H slave of : (Default 0,1,2,3,4) <-> (SYNC_PIN, VTG1, SDIN1, SDIN2, HDIN)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.HSlaveOf = (STVTG_SlaveOf_t)LVar;

        /* Get V slave of */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
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
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected BottomFieldDetectMin");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMin = (U32)LVar;

        /* Get BottomFieldDetectMax (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected BottomFieldDetectMax >=0");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.BottomFieldDetectMax = (U32)LVar;

        /* Get ExtVSyncShape (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected ExtVSyncShape : (0,1) <-> (BNOTT, PULSE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.ExtVSyncShape = (STVTG_ExtVSyncShape_t)LVar;

        /* Get IsExtVSyncRisingEdge (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected IsExtVSyncRisingEdge : (0,1) <-> (FALSE, TRUE)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.IsExtVSyncRisingEdge = (LVar == 1);

        /* Get IsExtHSyncRisingEdge (default: 0) */
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected IsExtHSyncRisingEdge : (0,1) <-> (FALSE, TRUE))");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VtgCell.IsExtHSyncRisingEdge = (LVar == 1);
    }
    else if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE)
    {
        /* Get slave of */
        RetErr = STTST_GetInteger( pars_p, 5, (S32*)&LVar );
        if(RetErr == TRUE)
        {
            STTST_TagCurrentLine( pars_p, "Expected slave of : (Default 5,6) <-> (DVP0, DVP1)");
            return(TRUE);
        }
        SlaveModeParams_s.Target.VfeSlaveOf = (STVTG_SlaveOf_t)LVar;
    }

    sprintf( VTG_Msg, "VTG_SetSMParams(%d): ", VTGHndl);
    ErrCode = STVTG_SetSlaveModeParams(VTGHndl, &SlaveModeParams_s);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);
    if(ErrCode == ST_NO_ERROR)
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
        else if( ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7015) ||
                 ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7020) )
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
        else if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE)
        {
            sprintf( VTG_Msg, "%s\tSlaveOf = %s\n" , VTG_Msg, \
                     (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_DVP0) ? "DVP0": \
                     (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_DVP1) ? "DVP1": "?????");
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
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    char TrvStr[80], IsStr[80], *ptr;
    S32 Var, i;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "EMBEDDED_SYNCHRO", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<VTG_MAX_OPTION_MODE; Var++){
        if((strcmp(VTGOptionMode[Var].Mode, TrvStr)==0) || \
           (strncmp(VTGOptionMode[Var].Mode, TrvStr+1, strlen(VTGOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==VTG_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr==TRUE){
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
    ErrCode = STVTG_GetOptionalConfiguration(VTGHndl, &OptionalParams_s);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);
    if(ErrCode == ST_NO_ERROR)
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
                      (OptionalParams_s.Value.EmbeddedSynchro == TRUE) ? "TRUE" :
                      (OptionalParams_s.Value.EmbeddedSynchro == FALSE) ? "FALSE" : "????"
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
    ST_ErrorCode_t ErrCode;
    S32 Type;
    char StrParams[RETURN_PARAMS_LENGTH];
    BOOL RetErr;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, VTGHndl, (S32*)&VTGHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* Get type for slave mode */
    RetErr = STTST_GetInteger( pars_p, VTG_DEVICE_TYPE, &Type);
    if ( RetErr == TRUE )
    {
    STTST_TagCurrentLine( pars_p, "expected Device DENC=0, 7015=2, 7020=3");
        return(TRUE);
    }

    sprintf( VTG_Msg, "STVTG_GetSMParams(%d): ", VTGHndl);
    ErrCode = STVTG_GetSlaveModeParams(VTGHndl, &SlaveModeParams_s);
    RetErr = VTG_TextError(ErrCode, VTG_Msg);

    if ( ErrCode == ST_NO_ERROR)
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
        else if( ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7015) ||
                 ((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VTG_CELL_7020) )
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
        else if((STVTG_DeviceType_t)Type == STVTG_DEVICE_TYPE_VFE)
        {
            sprintf( VTG_Msg, "%s\tSlaveOf = %s\n" , VTG_Msg, \
                        (SlaveModeParams_s.Target.VfeSlaveOf == STVTG_SLAVE_OF_DVP0) ? "DVP0": \
                        (SlaveModeParams_s.Target.VtgCell.HSlaveOf == STVTG_SLAVE_OF_DVP1) ? "DVP1": "?????");
        }
        VTG_PRINT(VTG_Msg);

        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );
} /* end VTG_GetSMParams */


/*-------------------------------------------------------------------------
 * Function : VTG_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VTG_InitCommand(void)
{
    BOOL RetErr=FALSE;
    RetErr |= STTST_RegisterCommand( "VTGInit", VTG_Init, \
                                     "<DeviceName><DeviceType(0-5)><MaxOpen><BaseAddress> VTG Init");
    RetErr |= STTST_RegisterCommand( "VTGOpen", VTG_Open, "<Device Name> VTG Open");
    RetErr |= STTST_RegisterCommand( "VTGClose", VTG_Close, "<Handle> VTG Close");
    RetErr |= STTST_RegisterCommand( "VTGGetMode", VTG_GetMode, "<Handle> VTG GetMode");
    RetErr |= STTST_RegisterCommand( "VTGGOptional", VTG_GetOptionalConfiguration, \
                                     "<Handle> Get Optional parameters");
    RetErr |= STTST_RegisterCommand( "VTGGSMParams", VTG_GetSMParams, "<Handle><Type> Get slave mode params");
    RetErr |= STTST_RegisterCommand( "VTGSetMode", VTG_SetMode, \
                                     "<Handle><TimingMode> Set timing mode (See capabilities for more info)");
    RetErr |= STTST_RegisterCommand( "VTGSOptional", VTG_SetOptionalConfiguration, \
                                     "<Handle><EmbeddedSynchro><NoOutputSignal> Set Optional parameters");
    RetErr |= STTST_RegisterCommand( "VTGSSMParams", VTG_SetSMParams,\
                                     "<Handle><Type><Slave mode><...> Set slave mode params");
    RetErr |= STTST_RegisterCommand( "VTGTerm", VTG_Term, "<DeviceName><ForceTeminate> Terminate");
    RetErr |= STTST_RegisterCommand( "VTGRev", VTG_GetRevision, "VTG Get Revision");
    RetErr |= STTST_RegisterCommand( "VTGCapa", VTG_GetCapability, "<Device Name> VTG Get Capability");

    /* Constants */
    RetErr |= STTST_AssignInteger ("ERR_VTG_INVALID_MODE", STVTG_ERROR_INVALID_MODE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VTG_DENC_OPEN", STVTG_ERROR_DENC_OPEN, TRUE);
    RetErr |= STTST_AssignInteger ("VTG_TYPE", VTG_DEVICE_TYPE, FALSE);

    RetErr |= STTST_AssignString  ("VTG_NAME", STVTG_DEVICE_NAME, FALSE);

    return( RetErr ? FALSE : TRUE);
} /* end VTG_MacroInit */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL VTG_RegisterCmd()
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

