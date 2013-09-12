/************************************************************************
COPYRIGHT (C) STMicroelectronics 2000

File name   : mix_test.c
Description : Definition of mixer commands

Note        : All functions return TRUE if error for Testtool compatibility

Date          Modification
----          ------------
Aug 2000      Creation
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"

#include "stvmix.h"
#include "stevt.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */
#define MAX_LAYER 10
#define MAX_VOUT  6

#if defined (ST_5510) || defined (ST_5512) || defined (ST_5508) || defined (ST_5518) || defined (ST_5514)
#define MIXER_BASE_ADDRESS (VIDEO_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  0
#define MIXER_DEVICE_TYPE  (STVMIX_OMEGA1_SD)
#elif defined (ST_7015)
#define MIXER_BASE_ADDRESS (STI7015_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  (ST7015_GAMMA_OFFSET+ST7015_GAMMA_MIX1_OFFSET)
#define MIXER_DEVICE_TYPE  (STVMIX_GAMMA_TYPE_7015)
#elif defined (ST_7020)
#define MIXER_BASE_ADDRESS (STI7020_BASE_ADDRESS)
#define MIXER_BASE_OFFSET  (ST7020_GAMMA_OFFSET+ST7020_GAMMA_MIX1_OFFSET)
#define MIXER_DEVICE_TYPE  (STVMIX_GAMMA_TYPE_7020)
#elif defined (ST_GX1)
#define MIXER_BASE_ADDRESS (0xBB410000)
#define MIXER_DEVICE_TYPE  (STVMIX_GAMMA_TYPE_GX1)
#define MIXER_BASE_OFFSET  0x500
#else
#error Not defined address & type for mixer
#endif


/* --- Global variables ------------------------------------------------ */

static STVMIX_Handle_t  MixHndl;                          /* Handle for mixer */
static STVMIX_LayerDisplayParams_t LayerParam[MAX_LAYER]; /* Layer parameters */
static char VMIX_Msg[400];                          /* Text for trace */

/* --- Macros ---*/
#define VMIX_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(VMIX_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \


/*-----------------------------------------------------------------------------
 * Function : VMIX_TextError
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL VMIX_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVMIX_ERROR_LAYER_UNKNOWN:
                strcat( Text, "(Unknown layer to connect) !\n");
                break;
            case STVMIX_ERROR_LAYER_UPDATE_PARAMETERS:
                strcat( Text, "(Layer cannot support parameters) !\n");
                break;
            case STVMIX_ERROR_LAYER_ALREADY_CONNECTED:
                strcat( Text, "(Layer already connected & VTG are different) !\n");
                break;
            case STVMIX_ERROR_LAYER_ACCESS:
                strcat( Text, "(Mixer can't acces to connected layer) !\n");
                break;
            case STVMIX_ERROR_VOUT_UNKNOWN:
                strcat( Text, "(Vout doesn't exist) !\n");
                break;
            case STVMIX_ERROR_VOUT_ALREADY_CONNECTED:
                strcat( Text, "(Vout already connected to another mixer) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VMIX_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*#######################################################################*/
/*###################### MIXER REGISTER COMMANDS ########################*/
/*######################  (in alphabetic order)  ########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VMIX_Close
 *            Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Close( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_Close(%d): ", MixHndl );
    ErrCode = STVMIX_Close( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Close() */


/*-------------------------------------------------------------------------
 * Function : VMIX_ConnectLayers
 *            Connect layers to the outputs
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_ConnectLayers( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t                ErrCode;
    STVMIX_LayerDisplayParams_t*  LayerArray[MAX_LAYER];
    char               DeviceName[STRING_DEVICE_LENGTH];
    BOOL                          RetErr;
    U16                           i = 0, j;

    DeviceName[0] = '\0';

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    do{
        RetErr = STTST_GetString( pars_p, "", DeviceName, sizeof(DeviceName) );
        strcpy(LayerParam[i].DeviceName, DeviceName);
        if(LayerParam[i].DeviceName[0] == 0x00)
            RetErr=TRUE;
        else{
            LayerArray[i] = &LayerParam[i];
            i++;
            if(i == MAX_LAYER)
                RetErr=TRUE;
        }
    }while(RetErr==FALSE);

    sprintf( VMIX_Msg, "STVMIX_ConnectLayers (%d", MixHndl);
    for(j=0; j<i; j++){
        sprintf(VMIX_Msg, "%s, %s", VMIX_Msg, LayerParam[j].DeviceName);
        LayerParam[j].ActiveSignal = FALSE;
    }
    sprintf( VMIX_Msg, "%s):", VMIX_Msg);
    ErrCode = STVMIX_ConnectLayers(MixHndl, (const STVMIX_LayerDisplayParams_t**)LayerArray, i);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_ConnectLayers() */


/*-------------------------------------------------------------------------
 * Function : VMIX_DisconnectLayers
 *            Disable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_DisconnectLayers( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_DisconnectLayers(%d): ", MixHndl);
    ErrCode = STVMIX_DisconnectLayers( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Disable() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Disable
 *            Disable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Disable( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_Disable(%d): ", MixHndl);
    ErrCode = STVMIX_Disable( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Disable() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Enable
 *            Enable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Enable( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_Enable(%d): ", MixHndl);
    ErrCode = STVMIX_Enable( MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_Enable() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetBackgroundColor
 *            Get background color
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetBackgroundColor( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    STGXOBJ_ColorRGB_t    Color;
    BOOL                  Enable;
    BOOL                  RetErr;
    char                  StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetBackgroundColor(%d): ", MixHndl);
    ErrCode = STVMIX_GetBackgroundColor( MixHndl, &Color, &Enable);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tEnable=%s  Red=%d Green=%d Blue=%d\n",
                (Enable == TRUE) ? "TRUE" :
                (Enable == FALSE) ?  "FALSE" : "?????",
                Color.R, Color.G, Color.B );
        sprintf(StrParams, "%d %d %d %d",
                Enable, Color.R, Color.G, Color.B);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_GetBackgroundColor */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetCapability
 *            Retreive driver's capabilities
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetCapability( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVMIX_Capability_t Capability;
    char DeviceName[STRING_DEVICE_LENGTH];
    char StrParams[RETURN_PARAMS_LENGTH];
    BOOL RetErr;

    /* get DeviceName */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetCapability(%s): ",  DeviceName);
    ErrCode = STVMIX_GetCapability(DeviceName, &Capability);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if(ErrCode == ST_NO_ERROR){
        sprintf(VMIX_Msg, "\tOffsetHorizontalMin = %d  OffsetHorizontalMax = %d\n"
                " \tOffsetVerticalMin = %d    OffsetVerticalMax = %d\n",
                Capability.ScreenOffsetHorizontalMin,
                Capability.ScreenOffsetHorizontalMax,
                Capability.ScreenOffsetVerticalMin ,
                Capability.ScreenOffsetVerticalMax);
        VMIX_PRINT(( VMIX_Msg ));
        sprintf(StrParams, "%d %d %d %d",
                Capability.ScreenOffsetHorizontalMin,
                Capability.ScreenOffsetHorizontalMax,
                Capability.ScreenOffsetVerticalMin ,
                Capability.ScreenOffsetVerticalMax);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VMIX_GetCapability() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetScreenOffset
 *            Retrieve the screen Offset parameter information
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetScreenOffset( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t  ErrCode;
    S8              Horizontal, Vertical;
    BOOL            RetErr;
    char            StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetScreenOffset(%d): ", MixHndl);
    ErrCode = STVMIX_GetScreenOffset( MixHndl, &Horizontal, &Vertical );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tHorizontal=%d Vertical=%d\n", Horizontal, Vertical );
        sprintf(StrParams, "%d %d", Horizontal, Vertical );
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_GetScreenOffset() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetScreenParams
 *            Retrieve the screen parameter information
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetScreenParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    STVMIX_ScreenParams_t ScreenParams;
    BOOL                  RetErr;
    char                  StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetScreenParams(%d): ", MixHndl);
    ErrCode = STVMIX_GetScreenParams( MixHndl, &ScreenParams );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tScanType=%s AspectRatio=%s FRate=%d\n",
                (ScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROGRESSIVE" :
                (ScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTERLACED" : "?????",
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
                (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
                ScreenParams.FrameRate);
        sprintf(VMIX_Msg, "%s\tWidth=%d Height=%d XStart=%d YStart=%d\n",
                VMIX_Msg, ScreenParams.Width, ScreenParams.Height, ScreenParams.XStart, ScreenParams.YStart);
        sprintf(StrParams, "%d %d %d %d %d %d %d",
                ScreenParams.ScanType, ScreenParams.AspectRatio, ScreenParams.FrameRate,\
                ScreenParams.Width, ScreenParams.Height, ScreenParams.XStart, ScreenParams.YStart);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_GetScreenParams() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetTimeBase
 *            Retrieve the VTG name
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetTimeBase( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    BOOL                  RetErr;
    char                  DeviceName[STRING_DEVICE_LENGTH];
    char                  StrParams[RETURN_PARAMS_LENGTH];

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_GetTimeBase(%d): ", MixHndl);
    ErrCode = STVMIX_GetTimeBase( MixHndl, (ST_DeviceName_t*)&DeviceName );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf(VMIX_Msg, "\tVTG name=\"%s\"\n", DeviceName );
        sprintf(StrParams, "%s", DeviceName);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        VMIX_PRINT(( VMIX_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end  VMIX_GetTimeBase() */


/*-------------------------------------------------------------------------
 * Function : VMIX_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    RevisionNumber = STVMIX_GetRevision();
    sprintf( VMIX_Msg, "STVMIX_GetRevision(): Ok Revision=%s\n", RevisionNumber );
    VMIX_PRINT(( VMIX_Msg ));
    return ( FALSE );

} /* end VMIX_GetRevision() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Init
 *            Initialisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Init( parse_t *pars_p, char *result_sym_p )
{
    extern ST_Partition_t *DriverPartition;
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    STVMIX_InitParams_t InitParams;
    ST_DeviceName_t VoutArray [MAX_VOUT];
    S32 Var;
    BOOL RetErr;
    U16  i=0,j ;

    ErrCode = ST_ERROR_BAD_PARAMETER;

    /* get DeviceName */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    /* get DeviceType */
    RetErr = STTST_GetInteger( pars_p, MIXER_DEVICE_TYPE, (S32 *)&InitParams.DeviceType );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Device Type" );
        return(TRUE);
    }

    /* get BaseAddress */
    RetErr = STTST_GetInteger( pars_p, MIXER_BASE_OFFSET, (S32 *)&InitParams.BaseAddress_p );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Base Address" );
        return(TRUE);
    }

    /* get VTGName */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, InitParams.VTGName, sizeof(char)*STRING_DEVICE_LENGTH);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected VTGName" );
        return(TRUE);
    }

    /* get MaxOpen */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&InitParams.MaxOpen );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected MaxOpen" );
        return(TRUE);
    }

    /* get MaxLayer */
    RetErr = STTST_GetInteger( pars_p, 5, &Var );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected MaxLayer" );
        return(TRUE);
    }
    InitParams.MaxLayer=(S16)Var;

    /* Get Vout Name */
    do{
        RetErr = STTST_GetString( pars_p, "", VoutArray[i], sizeof(ST_DeviceName_t));
        if((VoutArray[i][0] == '\0')){
            if (VoutArray[0][0] == '\0')
                InitParams.OutputArray_p = NULL;
            else{
                InitParams.OutputArray_p = VoutArray;
            }
            break;
        }
        i++;
    } while((RetErr == FALSE)&&(i<MAX_VOUT));

    if(i == MAX_VOUT){
        STTST_TagCurrentLine( pars_p, "Max output reach !!!" );
        return(TRUE);
    }


    InitParams.CPUPartition_p = DriverPartition;
    InitParams.DeviceBaseAddress_p = (void*)MIXER_BASE_ADDRESS;
    strcpy(InitParams.EvtHandlerName, STEVT_DEVICE_NAME);

    sprintf( VMIX_Msg, "STVMIX_Init(%s,%s,0x%X,VTG=\"%s\",Open=%d,Lay=%d,Vout=", \
             DeviceName,
             (InitParams.DeviceType == STVMIX_OMEGA1_SD) ? "55XX" :
             (InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015) ? "7015" :
             (InitParams.DeviceType == STVMIX_GAMMA_TYPE_7020) ? "7020" :
             (InitParams.DeviceType == STVMIX_GAMMA_TYPE_GX1) ? "GX1" : "????",
             (U32)InitParams.BaseAddress_p, (char*) InitParams.VTGName, InitParams.MaxOpen, InitParams.MaxLayer);
    for(j=0; j<i; j++){
        if(j!=0)
            strcat(VMIX_Msg, ",");
        sprintf( VMIX_Msg, "%s%s", VMIX_Msg, VoutArray[j]);
    }
    strcat(VMIX_Msg, "): ");

    ErrCode = STVMIX_Init( DeviceName, &InitParams);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_Init() */



/*-------------------------------------------------------------------------
 * Function : VMIX_Open
 *            Share physical decoder usage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Open( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVMIX_OpenParams_t OpenParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;

    /* get device */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
        return(TRUE);
    }

    RetErr = TRUE;
    sprintf( VMIX_Msg, "STVMIX_Open(%s): ", DeviceName );
    ErrCode = STVMIX_Open( DeviceName, &OpenParams, &MixHndl );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);
    sprintf(VMIX_Msg, "    ( hnd=%d )\n", (int)MixHndl );
    VMIX_PRINT(( VMIX_Msg ));
    if ( ErrCode == ST_NO_ERROR)
    {
        STTST_AssignInteger( result_sym_p, (int)MixHndl, FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_Open() */


/*-------------------------------------------------------------------------
 * Function : VMIX_SetBackgroundColor
 *            Set background color
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetBackgroundColor( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t        ErrCode;
    STGXOBJ_ColorRGB_t    Color;
    BOOL                  Enable;
    BOOL                  RetErr;
    S32                   Var;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    /* Get Enable */
    RetErr = STTST_GetInteger( pars_p, TRUE, &Var );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Enable (default TRUE)" );
        return(TRUE);
    }
    Enable = (BOOL)Var;

    /* Get Red */
    RetErr = STTST_GetInteger( pars_p, 127, &Var);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Red (default 127)" );
        return(TRUE);
    }
    Color.R = (U8)Var;

    /* Get Green */
    RetErr = STTST_GetInteger( pars_p, 127, &Var);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Green (default 127)" );
        return(TRUE);
    }
    Color.G = (U8)Var;

    /* Get Blue */
    RetErr = STTST_GetInteger( pars_p, 127, &Var);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Blue (default 127)" );
        return(TRUE);
    }
    Color.B = (U8)Var;

    sprintf( VMIX_Msg, "STVMIX_SetBackgroundColor(%d,Enable=%s,R=%d,G=%d,B=%d): ", MixHndl,
             (Enable == TRUE) ? "TRUE" :
             (Enable == FALSE) ?  "FALSE" : "?????",
             Color.R, Color.G, Color.B );

    ErrCode = STVMIX_SetBackgroundColor( MixHndl, Color, Enable);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_SetBackgroundColor() */


/*-------------------------------------------------------------------------
 * Function : VMIX_SetScreenOffset
 *            Change screen offsets
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetScreenOffset( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t     ErrCode;
    BOOL               RetErr;
    S32                XOffset, YOffset;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &XOffset );
    if ((RetErr == TRUE) || (XOffset > 127) || (XOffset < -128))
    {
        STTST_TagCurrentLine( pars_p, "Expected XOffset -128->127(default 0)" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, &YOffset );
    if ((RetErr == TRUE)  || (YOffset > 127) || (YOffset < -128))
    {
        STTST_TagCurrentLine( pars_p, "Expected YOffset -128->127(default 0)" );
        return(TRUE);
    }

    ErrCode = STVMIX_SetScreenOffset( MixHndl, (S8)XOffset, (S8)YOffset);
    sprintf( VMIX_Msg, "STVMIX_SetScreenOffset(%d,XOff=%d,YOff=%d): ", MixHndl, XOffset, YOffset);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );
} /* end of VMIX_SetScreenOffset() */


/*-------------------------------------------------------------------------
 * Function : VMIX_SetScreenParams
 *            Change screen paramaters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetScreenParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t         ErrCode;
    STVMIX_ScreenParams_t  ScreenParams;
    BOOL                   RetErr;

    ErrCode = ST_ERROR_BAD_PARAMETER;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_INTERLACED_SCAN , (S32 *)&ScreenParams.ScanType );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected ScanType" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, STGXOBJ_ASPECT_RATIO_4TO3, (S32 *)&ScreenParams.AspectRatio );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected AspectRatio" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 50000, (S32 *)&ScreenParams.FrameRate );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Frame Rate" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 720, (S32 *)&ScreenParams.Width );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Width" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 576, (S32 *)&ScreenParams.Height );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Height" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 132, (S32 *)&ScreenParams.XStart );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Xstart" );
        return(TRUE);
    }
    RetErr = STTST_GetInteger( pars_p, 23, (S32 *)&ScreenParams.YStart );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Ystart" );
        return(TRUE);
    }

    sprintf( VMIX_Msg, "STVMIX_SetScreenParams(%d,%s,%s,FR=%d,W=%d,H=%d,XS=%d,YS=%d): ", \
             MixHndl,
             (ScreenParams.ScanType == STGXOBJ_PROGRESSIVE_SCAN) ? "PROG" :
             (ScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN) ?  "INTER" : "?????",
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_4TO3) ? "4:3" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_221TO1) ? "221:1" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_SQUARE) ? "SQUARE" :
             (ScreenParams.AspectRatio == STGXOBJ_ASPECT_RATIO_16TO9) ? "16:9" : "?????",
             ScreenParams.FrameRate,
             ScreenParams.Width, ScreenParams.Height, ScreenParams.XStart, ScreenParams.YStart);

    ErrCode = STVMIX_SetScreenParams( MixHndl, &ScreenParams);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_SetScreenParams() */


/*-------------------------------------------------------------------------
 * Function : VMixSetTimeBase
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_SetTimeBase( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;

    ErrCode = ST_ERROR_BAD_PARAMETER;

    RetErr = STTST_GetInteger( pars_p, MixHndl, (S32*)&MixHndl );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle" );
        return(TRUE);
    }

    /* get device */
    RetErr = STTST_GetString( pars_p, STVTG_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected VTG DeviceName" );
    }

    ErrCode = STVMIX_SetTimeBase( MixHndl, DeviceName);
    sprintf( VMIX_Msg, "STVMIX_SetTimeBase(%d, VTGName=\"%s\"): ", MixHndl, DeviceName);
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMixSetTimeBase() */


/*-------------------------------------------------------------------------
 * Function : VMIX_Term
 *            Terminate the mixer decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VMIX_Term( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    char DeviceName[STRING_DEVICE_LENGTH];
    STVMIX_TermParams_t TermParams;
    S32 LVar;
    BOOL RetErr;

    ErrCode = ST_ERROR_BAD_PARAMETER;

    /* get device name */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVMIX_DEVICE_NAME,  DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected DeviceName" );
       return(TRUE);
    }

    /* get term param */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected ForceTerminate (TRUE, FALSE)" );
        return(TRUE);
    }
    TermParams.ForceTerminate = (BOOL)LVar;

    sprintf( VMIX_Msg, "STVMIX_Term(%s,forceterminate=%d): ", DeviceName, TermParams.ForceTerminate );
    ErrCode = STVMIX_Term( DeviceName, &TermParams );
    RetErr = VMIX_TextError(ErrCode, VMIX_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VMIX_Term */


/*#######################################################################*/
/*##########################  MIXER COMMANDS  ###########################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : VMIX_InitCommand
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VMIX_InitCommand (void)
{
    BOOL RetErr;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "VMIXClose",     VMIX_Close, "<Handle>, Close specified handle");
    RetErr |= STTST_RegisterCommand( "VMIXConnect",   VMIX_ConnectLayers, \
                                    "<Handle><LayerName1><LayerName2>..., Connect Layers");
    RetErr |= STTST_RegisterCommand( "VMIXDconnect",  VMIX_DisconnectLayers, "<Handle>, Disconnect Layers");
    RetErr |= STTST_RegisterCommand( "VMIXDisable",   VMIX_Disable, "<Handle>, Disable outputs of the mixer ");
    RetErr |= STTST_RegisterCommand( "VMIXEnable",    VMIX_Enable, "<Handle>, Enable outputs of the mixer");
    RetErr |= STTST_RegisterCommand( "VMIXCapa",      VMIX_GetCapability, "<DeviceName>, Get driver's capability");
    RetErr |= STTST_RegisterCommand( "VMIXGBack",     VMIX_GetBackgroundColor, "<Handle>, Returns string \"Enable R G B\" ");
    RetErr |= STTST_RegisterCommand( "VMIXGOffset",   VMIX_GetScreenOffset, "<Handle> Get screen offset");
    RetErr |= STTST_RegisterCommand( "VMIXGScreen",   VMIX_GetScreenParams, "<Handle>, Returns string containing screen parameters");
    RetErr |= STTST_RegisterCommand( "VMIXGTime",     VMIX_GetTimeBase, "<Handle>, Get time base");
    RetErr |= STTST_RegisterCommand( "VMIXInit",      VMIX_Init,\
                                    "<DevName><DevType><BaseAddr><VTGName><MaxOpen><MaxLayer><Out1>...<Out?>, Initialization");
    RetErr |= STTST_RegisterCommand( "VMIXOpen",      VMIX_Open, "<DeviceName>, Get handle from device");
    RetErr |= STTST_RegisterCommand( "VMIXRevision",  VMIX_GetRevision, "Get revision of mixer driver");
    RetErr |= STTST_RegisterCommand( "VMIXSBack",     VMIX_SetBackgroundColor, \
                                     "<Handle><Enable><Red><Green><Blue>, Set background color");
    RetErr |= STTST_RegisterCommand( "VMIXSOffset",   VMIX_SetScreenOffset, \
                                     "<Handle><XOffset><YOffset> Set screen offset");
    RetErr |= STTST_RegisterCommand( "VMIXSScreen",   VMIX_SetScreenParams, \
                                     "<Handle><Scan><Ratio><FRate><Width><Height><XStart><YStart>, Set screen parameters");
    RetErr |= STTST_RegisterCommand( "VMIXSTime",     VMIX_SetTimeBase, "<Handle><VTG Name>, Set time base with VTG name");
    RetErr |= STTST_RegisterCommand( "VMIXTerm",      VMIX_Term, "<DeviceName><ForceTerminate>, Terminate");

    /* constants */
    RetErr |= STTST_AssignInteger ("ERR_VMIX_LAYER_UNKNOWN"         , STVMIX_ERROR_LAYER_UNKNOWN          , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_UPDATE_PARAMETERS"     , STVMIX_ERROR_LAYER_UPDATE_PARAMETERS, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_ALREADY_CONNECTED"     , STVMIX_ERROR_LAYER_ALREADY_CONNECTED, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_LAYER_ACCESS"          , STVMIX_ERROR_LAYER_ACCESS           , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_VOUT_UNKNOWN"          , STVMIX_ERROR_VOUT_UNKNOWN           , TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VMIX_VOUT_ALREADY_CONNECTED", STVMIX_ERROR_VOUT_ALREADY_CONNECTED , TRUE);

    RetErr |= STTST_AssignString  ("VMIX_NAME", STVMIX_DEVICE_NAME, TRUE);
    RetErr |= STTST_AssignInteger ("VMIX_BASE_OFFSET", MIXER_BASE_OFFSET, FALSE);
    RetErr |= STTST_AssignInteger ("VMIX_TYPE", MIXER_DEVICE_TYPE, FALSE);

    return(RetErr ? FALSE : TRUE);
} /* end VMIX_InitCommand */


/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL VMIX_RegisterCmd()
{
    BOOL RetOk;

    RetOk = VMIX_InitCommand();
    if ( RetOk )
    {
        sprintf(VMIX_Msg, "VMIX_InitCommand() \t: ok           %s\n", STVMIX_GetRevision());
    }
    else
    {
        sprintf(VMIX_Msg, "VMIX_InitCommand() \t: failed !\n");
    }
    STTST_Print((VMIX_Msg));

    return( RetOk );

} /* end VMIX_CmdStart */


/*############################### END OF FILE ###########################*/

