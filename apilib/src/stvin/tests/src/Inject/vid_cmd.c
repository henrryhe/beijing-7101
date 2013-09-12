/************************************************************************
COPYRIGHT (C) STMicroelectronics 2001

File name   : vid_cmd.c
Description : Definition of video commands

Note        : All functions return TRUE if error for Testtool compatibility

Date          Modification                                      Initials
----          ------------                                      --------


************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>

#include "stvid.h"
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

#define DEFAULT_SCREEN_WIDTH     720         /* nb of horiz. pixels */
#define DEFAULT_SCREEN_HEIGHT    576         /* nb of vert. pixels */
#define DEFAULT_SCREEN_XOFFSET   132         /* time of horiz. pixels */
#define DEFAULT_SCREEN_YOFFSET    22         /* time of vert. pixels */
#define NB_MAX_OF_COPIED_PICTURES  2         /* pictures in memory for show/hide tests */


/* --- Global variables ------------------------------------------------ */

STVID_Handle_t          VidDecHndl0;         /* handle for video */
STVID_ViewPortHandle_t  ViewPortHndl0;       /* handle for viewport */

static char VID_Msg[800];                    /* text for trace */

static STVID_PictureParams_t VID_PictParams; /* picture parameters */
static STVID_PictureParams_t PictParamSrc;
static STVID_PictureParams_t PictParamDest;

STVID_PictureParams_t CopiedPictureParams[NB_MAX_OF_COPIED_PICTURES];

S16  NbOfCopiedPictures;

#ifdef STVID_WRAPPER_OLDARCH
static STVID_ScreenParams_t ScreenParams;    /* screen parameters */
#endif

/* --- Macros ---*/
#define VID_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(VID_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* --- Externals ------------------------------------------------------- */

extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;

extern BOOL VID_InitCommand2(void);

/*-----------------------------------------------------------------------------
 * Function : VID_TextError
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL VID_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVID_ERROR_DECODER_RUNNING :
                strcat( Text, "(stvid error can't retrieve info. while decoding) !\n");
                break;
            case STVID_ERROR_DECODER_RUNNING_IN_RT_MODE:
                strcat( Text, "(stvid error decoder is running in real time mode) !\n");
                break;
            case STVID_ERROR_DECODER_FREEZING:
                strcat( Text, "(stvid error decoder is freezing) !\n");
                break;
            case STVID_ERROR_DECODER_PAUSING:
                strcat( Text, "(stvid error decoder is pausing) !\n");
                break;
            case STVID_ERROR_DECODER_STOPPED:
                strcat( Text, "(stvid error decoder is stopped) !\n");
                break;
            case STVID_ERROR_DECODER_NOT_PAUSING:
                strcat( Text, "(stvid error decoder is not pausing) !\n");
                break;
            case STVID_ERROR_NOT_AVAILABLE:
                strcat( Text, "(stvid error no picture available) !\n");
                break;
            case STVID_ERROR_EVENT_REGISTRATION:
                strcat( Text, "(stvid error events registration error) !\n");
                break;
            case STVID_ERROR_SYSTEM_CLOCK:
                strcat( Text, "(stvid error connection error to the system clock) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VID_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*#######################################################################*/
/*###################### VIDEO REGISTER COMMANDS ########################*/
/*######################  (in alphabetic order)  ########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_Close
 *            Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Close( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;
    RetErr = FALSE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf ( VID_Msg, "STVID_Close(handle=%d) : ", CurrentHandle );
        ErrCode = STVID_Close( CurrentHandle );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_Close() */

/*-------------------------------------------------------------------------
 * Function : VID_CloseViewPort
 *            Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_CloseViewPort( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_CloseViewPort(viewportHndl=%d) : ", (S32)CurrentViewportHndl );
        ErrCode = STVID_CloseViewPort( CurrentViewportHndl );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_CloseViewPort() */

/*-------------------------------------------------------------------------
 * Function : VID_DisableOutputWindow
 *            Disable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableOutputWindow( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_DisableOutputWindow(viewportHndl=%d) : ", (S32)CurrentViewportHndl );
        ErrCode = STVID_DisableOutputWindow( CurrentViewportHndl );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_DisableOutputWindow() */

/*-------------------------------------------------------------------------
 * Function : VID_DisableSynchronisation
 *            Disable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DisableSynchronisation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_DisableSynchronisation(handle=%d) : ", CurrentHandle );
        ErrCode = STVID_DisableSynchronisation( CurrentHandle );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_DisableSynchronisation() */

/*-------------------------------------------------------------------------
 * Function : VID_DuplicatePicture
 *            Copy a picture to a user defined area
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_DuplicatePicture( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    RetErr = TRUE;
    sprintf( VID_Msg, "STVID_DuplicatePicture(src=%d,dest=%d) : ", (S32)&PictParamSrc, (S32)&PictParamDest );
    ErrCode = STVID_DuplicatePicture( &PictParamSrc, &PictParamDest );
    RetErr  = VID_TextError( ErrCode, VID_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_DuplicatePicture() */

/*-------------------------------------------------------------------------
 * Function : VID_EnableOutputWindow
 *            Enable the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableOutputWindow( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_EnableOutputWindow(viewportHndl=%d) : ",
                 (S32)CurrentViewportHndl );
        ErrCode = STVID_EnableOutputWindow( CurrentViewportHndl );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_EnableOutputWindow() */

/*-------------------------------------------------------------------------
 * Function : VID_EnableSynchronisation
 *            Enable synchronisation with PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_EnableSynchronisation( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_EnableSynchronisation(handle=%d) : ", CurrentHandle );
        ErrCode = STVID_EnableSynchronisation( CurrentHandle );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_EnableSynchronisation() */

/*-------------------------------------------------------------------------
 * Function : VID_Freeze
 *            Freeze the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Freeze( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_Freeze_t Freeze;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get freeze mode */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_MODE_FORCE, (S32 *)&Freeze.Mode );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Mode (%d=none %d=force %d=no_flicker)" ,
                        STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
            STTST_TagCurrentLine( pars_p, VID_Msg);
        }
    }
    /* get freeze field */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_FIELD_TOP, (S32 *)&Freeze.Field );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Field (%d=top %d=bot %d=current %d=next)" ,
                    STVID_FREEZE_FIELD_TOP,	STVID_FREEZE_FIELD_BOTTOM,
                    STVID_FREEZE_FIELD_CURRENT,	STVID_FREEZE_FIELD_NEXT );
            STTST_TagCurrentLine( pars_p, VID_Msg);
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STVID_Freeze( CurrentHandle, &Freeze );
        sprintf( VID_Msg, "STVID_Freeze(handle=%d,(freezemode=%d,freezefield=%d)) : ",
                 CurrentHandle, Freeze.Mode, Freeze.Field );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_Freeze() */

/*------------------------------------------------------------------------
 * Function : VID_GetAlignIOWindows
 *            Get the closiest window from the specified one
 *            matching hardware constraints
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetAlignIOWindows( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinPositionX;
    S32 OutputWinPositionY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
	    RetErr = TRUE;
        ErrCode = STVID_GetAlignIOWindows( CurrentViewportHndl,
                                           &InputWinPositionX, &InputWinPositionY,
                                           &InputWinWidth, &InputWinHeight,
                                           &OutputWinPositionX, &OutputWinPositionY,
                                           &OutputWinWidth, &OutputWinHeight );

        sprintf( VID_Msg, "STVID_GetAlignIOWindows(viewportHndl=%d,ix=%d,iy=%d,iw=%d,ih=%d,ox=%d,oy=%d,ow=%d,oh=%d) : ",
                 (U32)CurrentViewportHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
                STTST_AssignInteger("RET_VAL1", InputWinPositionX, FALSE);
                STTST_AssignInteger("RET_VAL2", InputWinPositionY, FALSE);
                STTST_AssignInteger("RET_VAL3", InputWinWidth, FALSE);
                STTST_AssignInteger("RET_VAL4", InputWinHeight, FALSE);
                STTST_AssignInteger("RET_VAL5", OutputWinPositionX, FALSE);
                STTST_AssignInteger("RET_VAL6", OutputWinPositionY, FALSE);
                STTST_AssignInteger("RET_VAL7", OutputWinWidth, FALSE);
                STTST_AssignInteger("RET_VAL8", OutputWinHeight, FALSE);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetAlignIOWindows() */


/*------------------------------------------------------------------------
 * Function : VID_GetBitBufferFreeSize
 *            Get the free memory size of bit buffer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetBitBufferFreeSize( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;
    U32 BBFreeSize;

    RetErr = FALSE;
    BBFreeSize = 0;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
	    RetErr = TRUE;
        ErrCode = STVID_GetBitBufferFreeSize( CurrentHandle, &BBFreeSize );
        sprintf( VID_Msg, "VID_GetBitBufferFreeSize(hndl=%d,freesize=%d) : ",
                 (U32)CurrentHandle, BBFreeSize );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetBitBufferFreeSize() */

/*-------------------------------------------------------------------------
 * Function : VID_GetCapability
 *            Retreive driver's capabilities
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetCapability( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_Capability_t Capability;
    BOOL RetErr;
    U16 Count;
    char Name[2*ST_MAX_DEVICE_NAME]; /* allow big name for error case test */
    ST_DeviceName_t *DeviceName;

	/* get DeviceName */
    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME, Name, sizeof(Name) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName" );
    /*}
    else
    {*/
        STTST_AssignInteger(result_sym_p, ST_ERROR_UNKNOWN_DEVICE, FALSE);
        /*return(TRUE);*/
    }
    DeviceName = (ST_DeviceName_t *)Name ;

    RetErr = TRUE;
    sprintf( VID_Msg, "STVID_GetCapability(device=%s,&capa=%ld) : ",
             *DeviceName, (long)&Capability );
    ErrCode = STVID_GetCapability(*DeviceName, &Capability);
    RetErr  = VID_TextError( ErrCode, VID_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf( VID_Msg, "   codingmode=%d colortype=%d ARconv=%d AR=%d freezemode=%d freezefield=%d\n" ,
                    Capability.SupportedCodingMode, Capability.SupportedColorType,
                    Capability.SupportedDisplayARConversion, Capability.SupportedDisplayAspectRatio,
                    Capability.SupportedFreezeMode, Capability.SupportedFreezeField );
        sprintf( VID_Msg, "%s   picture=%d scantype=%d stop=%d streamtype=%d pictcapa=%d\n",
                    VID_Msg,
                    Capability.SupportedPicture, Capability.SupportedScreenScanType,
                    Capability.SupportedStop, Capability.SupportedStreamType,
                    Capability.StillPictureCapable );
        sprintf( VID_Msg, "%s   inputwindowcapa=%d outputwindowcapa=%d align=%d size=%d\n",
                    VID_Msg,
                    Capability.ManualInputWindowCapable, Capability.ManualOutputWindowCapable,
                    Capability.SupportedWinAlign, Capability.SupportedWinSize );
        sprintf( VID_Msg, "%s   INPUT heightmin=%d widthmin=%d xprec=%d yprec=%d widthprec=%d heightprec=%d\n",
                    VID_Msg,
                    Capability.InputWindowHeightMin, Capability.InputWindowWidthMin,
                    Capability.InputWindowPositionXPrecision, Capability.InputWindowPositionYPrecision,
                    Capability.InputWindowWidthPrecision, Capability.InputWindowHeightPrecision );
        sprintf( VID_Msg, "%s   OUTPUT heightmin=%d widthmin=%d xprec=%d yprec=%d widthprec=%d heightprec=%d\n",
                    VID_Msg,
                    Capability.OutputWindowHeightMin, Capability.OutputWindowWidthMin,
                    Capability.OutputWindowPositionXPrecision, Capability.OutputWindowPositionYPrecision,
                    Capability.OutputWindowWidthPrecision, Capability.OutputWindowHeightPrecision );
        /*sprintf( VID_Msg, "%s   maxmixweight=%d profilecapa=%d horizmin=%d horizmax=%d vertmin=%d vertmax=%d\n",
                    VID_Msg,
                    Capability.MaxMixWeight, Capability.ProfileCapable,
                    Capability.ScreenHorizontalOffsetMin, Capability.ScreenHorizontalOffsetMax,
                    Capability.ScreenVerticalOffsetMin, Capability.ScreenVerticalOffsetMax );*/
        sprintf( VID_Msg, "%s   profilecapa=%d \n",
                    VID_Msg, Capability.ProfileCapable );
        VID_PRINT(( VID_Msg ));

        sprintf( VID_Msg, "   ContinuousHorizontalScaling=%s",
                    Capability.SupportedHorizontalScaling.Continuous ? "TRUE " : "FALSE");
        STTST_AssignInteger("MIN_SCALE_X", (Capability.SupportedHorizontalScaling.ScalingFactors_p)->N, TRUE);
        for (Count = 0; Count < Capability.SupportedHorizontalScaling.NbScalingFactors && Count < 15 ; Count++)
        {
            sprintf(VID_Msg, "%s [%d/%d]", VID_Msg,
                (Capability.SupportedHorizontalScaling.ScalingFactors_p + Count)->N,
                (Capability.SupportedHorizontalScaling.ScalingFactors_p + Count)->M);
            STTST_AssignInteger("MAX_SCALE_X", (Capability.SupportedHorizontalScaling.ScalingFactors_p + Count)->N, TRUE);
        }
        VID_PRINT(VID_Msg);
        sprintf(VID_Msg, "\n   ContinuousVerticalScaling=%s  ",
                    Capability.SupportedVerticalScaling.Continuous ? "TRUE " : "FALSE");
        STTST_AssignInteger("MIN_SCALE_Y", (Capability.SupportedVerticalScaling.ScalingFactors_p)->N, TRUE);
        for (Count = 0; Count < Capability.SupportedVerticalScaling.NbScalingFactors && Count < 15; Count++)
        {
            sprintf(VID_Msg, "%s [%d/%d]", VID_Msg,
                (Capability.SupportedVerticalScaling.ScalingFactors_p + Count)->N,
                (Capability.SupportedVerticalScaling.ScalingFactors_p + Count)->M);
            STTST_AssignInteger("MAX_SCALE_Y", (Capability.SupportedVerticalScaling.ScalingFactors_p + Count)->N, TRUE);
        }
        strcat( VID_Msg, "\n");
        STTST_AssignInteger("RET_VAL1", Capability.SupportedCodingMode, FALSE);
        STTST_AssignInteger("RET_VAL2", Capability.SupportedColorType, FALSE);
        STTST_AssignInteger("RET_VAL3", Capability.SupportedDisplayARConversion, FALSE);
        STTST_AssignInteger("RET_VAL4", Capability.SupportedDisplayAspectRatio, FALSE);
        STTST_AssignInteger("RET_VAL5", Capability.SupportedFreezeMode, FALSE);
        STTST_AssignInteger("RET_VAL6", Capability.SupportedFreezeField, FALSE);
        STTST_AssignInteger("RET_VAL7", Capability.SupportedPicture, FALSE);
        STTST_AssignInteger("RET_VAL8", Capability.SupportedScreenScanType, FALSE);
        STTST_AssignInteger("RET_VAL9", Capability.SupportedStop, FALSE);
        VID_PRINT(VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetCapability() */

/*-------------------------------------------------------------------------
 * Function : VID_GetDataInterfaceParams
 *            Get pel ratio conversion
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDataInterfaceParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_DataInterfaceParams_t InterfParam;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_GetDataInterfaceParams(handle=%d,&intparam=%d) : ",
                          CurrentHandle, (S32)&InterfParam );
        ErrCode = STVID_GetDataInterfaceParams( CurrentHandle, &InterfParam );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf(VID_Msg, "   destination=%d holdoff=%d writelength=%d\n",
                        (S32)InterfParam.Destination,
                        InterfParam.Holdoff, InterfParam.WriteLength);
            VID_PRINT(VID_Msg);
            STTST_AssignInteger("RET_VAL1", (S32)InterfParam.Destination, FALSE);
            STTST_AssignInteger("RET_VAL2", InterfParam.Holdoff, FALSE);
            STTST_AssignInteger("RET_VAL3", InterfParam.WriteLength, FALSE);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetDataInterfaceParams() */

/*-------------------------------------------------------------------------
 * Function : VID_GetDecodedPictures
 *            Get decoded pictures type
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDecodedPictures( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_DecodedPictures_t DecodedPictures;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;

    RetErr = FALSE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "VID_GetDecodedPictures(handle=%d,&decodedpict=%d) : ",
                          CurrentHandle, (S32)&DecodedPictures );
        ErrCode = STVID_GetDecodedPictures( CurrentHandle, &DecodedPictures );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf(VID_Msg, "   decodedpictures=%d ",
                        (S32)DecodedPictures ) ;
            switch(DecodedPictures)
            {
                case STVID_DECODED_PICTURES_ALL :
                    strcat(VID_Msg, "(ALL)\n");
                    break;
                case STVID_DECODED_PICTURES_IP :
                    strcat(VID_Msg, "(IP)\n");
                    break;
                case STVID_DECODED_PICTURES_I :
                    strcat(VID_Msg, "(I)\n");
                    break;
            }
            STTST_AssignInteger("RET_VAL1", (S32)DecodedPictures, FALSE);
            VID_PRINT(VID_Msg);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetDecodedPictures() */


/*-------------------------------------------------------------------------
 * Function : VID_GetDisplayARConversion
 *            Get pel ratio conversion
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetDisplayARConversion( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_DisplayAspectRatioConversion_t RatioCv;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetDisplayAspectRatioConversion( CurrentViewportHndl, &RatioCv );
        sprintf( VID_Msg, "STVID_GetDisplayAspectRatioConversion(viewporthndl=%d,ratio=%d) : ",
                          (U32)CurrentViewportHndl, RatioCv );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetDisplayARConversion() */

/*-------------------------------------------------------------------------
 * Function : VID_GetErrorRecoveryMode
 *            Get current error recovery mode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetErrorRecoveryMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_ErrorRecoveryMode_t Mode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetErrorRecoveryMode( CurrentHandle, &Mode );
        sprintf( VID_Msg, "STVID_GetErrorRecoveryMode(handle=%d,mode=%d) : ",
                          CurrentHandle, Mode );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", Mode, FALSE);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetErrorRecoveryMode() */

/*-------------------------------------------------------------------------
 * Function : VID_GetInputWindowMode
 *            Get input window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetInputWindowMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_WindowParams_t WinParams;
    BOOL AutoMode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetInputWindowMode( CurrentViewportHndl, &AutoMode, &WinParams );
        sprintf( VID_Msg, "STVID_GetInputWindowMode(viewporthndl=%d,automode=%d,align=%d,size=%d) : ",
                 (U32)CurrentViewportHndl, AutoMode, WinParams.Align, WinParams.Size );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            if ( WinParams.Align!=STVID_WIN_ALIGN_TOP_LEFT
                && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_LEFT
                && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_LEFT
                && WinParams.Align!=STVID_WIN_ALIGN_TOP_RIGHT
                && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_RIGHT
                && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_RIGHT
                && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_HCENTRE
                && WinParams.Align!=STVID_WIN_ALIGN_TOP_HCENTRE
                && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_HCENTRE )
            {
                API_ErrorCount++;
                sprintf( VID_Msg, "   wrong Align value = %d !\n", WinParams.Align );
                VID_PRINT(VID_Msg);
            }
            else
            {
                RetErr = FALSE;
                STTST_AssignInteger("RET_VAL1", AutoMode, FALSE);
                STTST_AssignInteger("RET_VAL2", WinParams.Align, FALSE);
                STTST_AssignInteger("RET_VAL3", WinParams.Size, FALSE);
            }
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetInputWindowMode() */

/*-------------------------------------------------------------------------
 * Function : VID_GetIOWindows
 *            Retrieve the input and output window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetIOWindows( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    S32 InputWinPositionX;
    S32 InputWinPositionY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    S32 OutputWinPositionX;
    S32 OutputWinPositionY;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetIOWindows( CurrentViewportHndl,
                                      &InputWinPositionX, &InputWinPositionY,
                                      &InputWinWidth, &InputWinHeight,
                                      &OutputWinPositionX, &OutputWinPositionY,
                                      &OutputWinWidth, &OutputWinHeight );
        sprintf( VID_Msg, "STVID_GetIOWindows(viewporthndl=%d,ix=%d,iy=%d,iw=%d,ih=%d,ox=%d,oy=%d,ow=%d,oh=%d) : ",
                 (U32)CurrentViewportHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", InputWinPositionX, FALSE);
            STTST_AssignInteger("RET_VAL2", InputWinPositionY, FALSE);
            STTST_AssignInteger("RET_VAL3", InputWinWidth, FALSE);
            STTST_AssignInteger("RET_VAL4", InputWinHeight, FALSE);
            STTST_AssignInteger("RET_VAL5", OutputWinPositionX, FALSE);
            STTST_AssignInteger("RET_VAL6", OutputWinPositionY, FALSE);
            STTST_AssignInteger("RET_VAL7", OutputWinWidth, FALSE);
            STTST_AssignInteger("RET_VAL8", OutputWinHeight, FALSE);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetIOWindows() */

/*-------------------------------------------------------------------------
 * Function : VID_GetMemoryProfile
 *            Retrieve the input and output window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetMemoryProfile( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_MemoryProfile_t MemProfile;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetMemoryProfile( CurrentHandle, &MemProfile );
        sprintf( VID_Msg, "STVID_GetMemoryProfile(handle=%d,&memprof=%d) : ",
                 (U32)CurrentHandle, (int)&MemProfile );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf( VID_Msg, "   maxwidth=%d,maxheight=%d,nbframestore=%d,compress=%d,decim=%d \n",
                        MemProfile.MaxWidth, MemProfile.MaxHeight,
                        MemProfile.NbFrameStore, MemProfile.CompressionLevel,
                        MemProfile.DecimationFactor );
            STTST_AssignInteger("RET_VAL1", MemProfile.MaxWidth, FALSE);
            STTST_AssignInteger("RET_VAL2", MemProfile.MaxHeight, FALSE);
            STTST_AssignInteger("RET_VAL3", MemProfile.NbFrameStore, FALSE);
            STTST_AssignInteger("RET_VAL4", MemProfile.CompressionLevel, FALSE);
            STTST_AssignInteger("RET_VAL5", MemProfile.DecimationFactor, FALSE);
            VID_PRINT(VID_Msg);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetMemoryProfile() */

#ifdef STVID_WRAPPER_OLDARCH

/*-------------------------------------------------------------------------
 * Function : VID_GetMixWeight
 *            Get blending factor video plane / below plane
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetMixWeight( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    U8 MixWeight;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetMixWeight( VidDecHndl0, &MixWeight );
        sprintf( VID_Msg, "STVID_GetMixWeight(handle=%d,mix=%d) : ", CurrentHandle, MixWeight );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
        if ( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", MixWeight, false);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetMixWeight */

#endif

  /*-------------------------------------------------------------------------
 * Function : VID_GetOutputWindowMode
 *            Get output window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetOutputWindowMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_WindowParams_t WinParams;
    BOOL AutoMode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_GetOutputWindowMode( CurrentViewportHndl, &AutoMode, &WinParams );
        sprintf( VID_Msg, "STVID_GetOutputWindowMode(viewporthndl=%d,automode=%d,align=%d,size=%d) : ",
                 (int)CurrentViewportHndl, AutoMode, WinParams.Align, WinParams.Size );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            if ( WinParams.Align!=STVID_WIN_ALIGN_TOP_LEFT
                && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_LEFT
                && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_LEFT
                && WinParams.Align!=STVID_WIN_ALIGN_TOP_RIGHT
                && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_RIGHT
                && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_RIGHT
                && WinParams.Align!=STVID_WIN_ALIGN_BOTTOM_HCENTRE
                && WinParams.Align!=STVID_WIN_ALIGN_TOP_HCENTRE
                && WinParams.Align!=STVID_WIN_ALIGN_VCENTRE_HCENTRE )
            {
                sprintf( VID_Msg, "  wrong Align value = %d !\n", WinParams.Align );
                API_ErrorCount++;
                VID_PRINT(VID_Msg);
            }
            else
            {
                STTST_AssignInteger("RET_VAL1", AutoMode, FALSE);
                STTST_AssignInteger("RET_VAL2", WinParams.Align, FALSE);
                STTST_AssignInteger("RET_VAL3", WinParams.Size, FALSE);
            }
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetOutputWindowMode() */

/*-------------------------------------------------------------------------
 * Function : VID_GetPictureAllocParams
 *            Get parameters to allocate space for the picture
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetPictureAllocParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STAVMEM_AllocBlockParams_t AllocParams;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetPictureAllocParams( CurrentHandle, &VID_PictParams, &AllocParams );
        sprintf( VID_Msg, "STVID_GetPictureAllocParams(handle=%d,&pictparam=%d,&allocparams=%d) : ",
                          CurrentHandle, (U32)&VID_PictParams, (U32)&AllocParams );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf( VID_Msg, "   codingmode=%d colortype=%d framerate=%d data=%d size=%d\n",
                        VID_PictParams.CodingMode,
                        VID_PictParams.ColorType, VID_PictParams.FrameRate,
                        (U32)VID_PictParams.Data, VID_PictParams.Size ) ;
            sprintf( VID_Msg, "%s   height=%d width=%d\n",
                        VID_Msg, VID_PictParams.Height, VID_PictParams.Width ) ;
            sprintf( VID_Msg, "%s   aspect=%d scantype=%d mpegframe=%d topfieldfirst=%d\n",
                        VID_Msg, VID_PictParams.Aspect,
                        VID_PictParams.ScanType, VID_PictParams.MPEGFrame,
                        VID_PictParams.TopFieldFirst );
            sprintf( VID_Msg, "%s   hours=%d minutes=%d seconds=%d frames=%d interpolated=%d\n",
                        VID_Msg, VID_PictParams.TimeCode.Hours,
                        VID_PictParams.TimeCode.Minutes, VID_PictParams.TimeCode.Seconds,
                        VID_PictParams.TimeCode.Frames, VID_PictParams.TimeCode.Interpolated );
            sprintf( VID_Msg, "%s   pts=%d pts33=%d ptsinterpolated=%d\n",
                        VID_Msg, VID_PictParams.PTS.PTS,
                        VID_PictParams.PTS.PTS33, VID_PictParams.PTS.Interpolated );
            sprintf( VID_Msg, "%s   partitionhndl=%d size=%d align=%d\n",
                        VID_Msg, AllocParams.PartitionHandle,
                        AllocParams.Size, AllocParams.Alignment );
            sprintf( VID_Msg, "%s   mode=%d nbfranges=%d &range=%d\n",
                        VID_Msg, AllocParams.AllocMode,
                        AllocParams.NumberOfForbiddenRanges, (U32)AllocParams.ForbiddenRangeArray_p );
            sprintf( VID_Msg, "%s   nbfborders=%d &borders=%d\n",
                        VID_Msg, AllocParams.NumberOfForbiddenBorders,
                        (U32)AllocParams.ForbiddenBorderArray_p );
            STTST_AssignInteger("RET_VAL1", VID_PictParams.CodingMode, FALSE);
            STTST_AssignInteger("RET_VAL2", VID_PictParams.ColorType, FALSE);
            STTST_AssignInteger("RET_VAL3", VID_PictParams.FrameRate, FALSE);
            STTST_AssignInteger("RET_VAL4", (U32)VID_PictParams.Data, FALSE);
            STTST_AssignInteger("RET_VAL5", VID_PictParams.Size, FALSE);
            STTST_AssignInteger("RET_VAL6", VID_PictParams.Height, FALSE);
            STTST_AssignInteger("RET_VAL7", VID_PictParams.Width, FALSE);
            STTST_AssignInteger("RET_VAL8", VID_PictParams.Aspect, FALSE);
            STTST_AssignInteger("RET_VAL9", VID_PictParams.MPEGFrame, FALSE);
            VID_PRINT(VID_Msg);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetPictureAllocParams() */


/*-------------------------------------------------------------------------
 * Function : VID_GetPictureParams
 *            Get info. about a picture
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetPictureParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_Picture_t PictureType;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get type */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_PICTURE_DISPLAYED, (S32 *)&PictureType );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Name(%d=last decoded, %d=displayed) ",
                              STVID_PICTURE_LAST_DECODED, STVID_PICTURE_DISPLAYED );
            STTST_TagCurrentLine( pars_p, VID_Msg );
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetPictureParams( CurrentHandle, PictureType, &VID_PictParams );
        sprintf( VID_Msg, "STVID_GetPictureParams(handle=%d,picttype=%d,&params=%ld) : ",
                 CurrentHandle, PictureType, (long)&VID_PictParams );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf( VID_Msg, "   codingmode=%d colortype=%d framerate=%d data=%d size=%d\n",
                        VID_PictParams.CodingMode,
                        VID_PictParams.ColorType, VID_PictParams.FrameRate,
                        (U32)VID_PictParams.Data, VID_PictParams.Size ) ;
            sprintf( VID_Msg, "%s   height=%d width=%d\n",
                        VID_Msg, VID_PictParams.Height, VID_PictParams.Width ) ;
            sprintf( VID_Msg, "%s   aspect=%d scantype=%d mpegframe=%d topfieldfirst=%d\n",
                        VID_Msg, VID_PictParams.Aspect,
                        VID_PictParams.ScanType, VID_PictParams.MPEGFrame,
                        VID_PictParams.TopFieldFirst );
            sprintf( VID_Msg, "%s   hours=%d minutes=%d seconds=%d frames=%d interpolated=%d\n",
                        VID_Msg, VID_PictParams.TimeCode.Hours,
                        VID_PictParams.TimeCode.Minutes, VID_PictParams.TimeCode.Seconds,
                        VID_PictParams.TimeCode.Frames, VID_PictParams.TimeCode.Interpolated );
            sprintf( VID_Msg, "%s   pts=%d pts33=%d ptsinterpolated=%d\n",
                        VID_Msg, VID_PictParams.PTS.PTS,
                        VID_PictParams.PTS.PTS33, VID_PictParams.PTS.Interpolated );
            STTST_AssignInteger("RET_VAL1", PictureType, FALSE);
            STTST_AssignInteger("RET_VAL2", VID_PictParams.CodingMode, FALSE);
            STTST_AssignInteger("RET_VAL3", VID_PictParams.ColorType, FALSE);
            STTST_AssignInteger("RET_VAL4", VID_PictParams.FrameRate, FALSE);
            STTST_AssignInteger("RET_VAL5", (U32)VID_PictParams.Data, FALSE);
            STTST_AssignInteger("RET_VAL6", VID_PictParams.Size, FALSE);
            STTST_AssignInteger("RET_VAL7", VID_PictParams.Height, FALSE);
            STTST_AssignInteger("RET_VAL8", VID_PictParams.Width, FALSE);
            STTST_AssignInteger("RET_VAL9", VID_PictParams.Aspect, FALSE);
            STTST_AssignInteger("RET_VAL10", VID_PictParams.ScanType, FALSE);
            STTST_AssignInteger("RET_VAL11", VID_PictParams.MPEGFrame, FALSE);
            STTST_AssignInteger("RET_VAL12", VID_PictParams.TopFieldFirst, FALSE);
            STTST_AssignInteger("RET_VAL13", VID_PictParams.TimeCode.Hours, FALSE);
            STTST_AssignInteger("RET_VAL14", VID_PictParams.TimeCode.Minutes, FALSE);
            STTST_AssignInteger("RET_VAL15", VID_PictParams.TimeCode.Seconds, FALSE);
            STTST_AssignInteger("RET_VAL16", VID_PictParams.TimeCode.Frames, FALSE);
            STTST_AssignInteger("RET_VAL17", VID_PictParams.TimeCode.Interpolated, FALSE);
            STTST_AssignInteger("RET_VAL18", VID_PictParams.PTS.PTS, FALSE);
            STTST_AssignInteger("RET_VAL19", VID_PictParams.PTS.PTS33, FALSE);
            STTST_AssignInteger("RET_VAL20", VID_PictParams.PTS.Interpolated, FALSE);
            VID_PRINT(VID_Msg);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetPictureParams() */

/*-------------------------------------------------------------------------
 * Function : VID_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    RevisionNumber = STVID_GetRevision();

    sprintf( VID_Msg, "STVID_GetRevision() : revision=%s\n", RevisionNumber );
    VID_PRINT(VID_Msg);
    STTST_AssignInteger(result_sym_p, ST_NO_ERROR, FALSE);

    return ( FALSE );

} /* end VID_GetRevision() */


#ifdef STVID_WRAPPER_OLDARCH

/*-------------------------------------------------------------------------
 * Function : VID_GetScreenOffset
 *            Set screen position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetScreenOffset( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    S16 Horizontal;
    S16 Vertical;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetScreenOffset( CurrentHandle, &Horizontal, &Vertical );
        sprintf( VID_Msg, "STVID_GetScreenOffset(handle=%d,horiz=%d,vert=%d) : ",
                 CurrentHandle, Horizontal, Vertical );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", Horizontal, false);
            STTST_AssignInteger("RET_VAL2", Vertical, false);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetScreenOffset */

/*-------------------------------------------------------------------------
 * Function : VID_GetScreenParams
 *            Get screen parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetScreenParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetScreenParams( CurrentHandle, &ScreenParams );
        sprintf( VID_Msg, "STVID_GetScreenParams(handle=%d,&param=%ld) : ",
                 CurrentHandle, (long)&ScreenParams );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf( VID_Msg, "  aspect=%d width=%d height=%d xoffset=%d yoffset=%d scantype=%d framerate=%d\n",
                    ScreenParams.Aspect,
                    ScreenParams.Width, ScreenParams.Height,
                    ScreenParams.XOffset, ScreenParams.YOffset,
                    ScreenParams.ScanType, ScreenParams.FrameRate );
            STTST_AssignInteger("RET_VAL1", ScreenParams.Aspect, false);
            STTST_AssignInteger("RET_VAL2", ScreenParams.Width, false);
            STTST_AssignInteger("RET_VAL3", ScreenParams.Height, false);
            STTST_AssignInteger("RET_VAL4", ScreenParams.XOffset, false);
            STTST_AssignInteger("RET_VAL5", ScreenParams.YOffset, false);
            STTST_AssignInteger("RET_VAL6", ScreenParams.ScanType, false);
            STTST_AssignInteger("RET_VAL7", ScreenParams.FrameRate, false);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_GetScreenParams */

#endif

/*-------------------------------------------------------------------------
 * Function : VID_GetSpeed
 *            Get play speed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_GetSpeed( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    S32 Speed;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_GetSpeed( CurrentHandle, &Speed );
        sprintf( VID_Msg, "STVID_GetSpeed(handle=%d,speed=%d) : ", CurrentHandle, Speed );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
        if ( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", Speed, FALSE);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_GetSpeed() */

/*-------------------------------------------------------------------------
 * Function : VID_HidePicture
 *            Restore the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_HidePicture( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
	RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	if ( RetErr == TRUE )
	{
	    STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	}
    else
    {
        RetErr = TRUE;
        ErrCode = STVID_HidePicture( CurrentViewportHndl );
        sprintf( VID_Msg, "STVID_HidePicture(viewporthndl=%d) : ", (U32)CurrentViewportHndl );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_HidePicture() */

/*-------------------------------------------------------------------------
 * Function : VID_Init
 *            Initialisation
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Init( parse_t *pars_p, char *result_sym_p )
{
    extern ST_Partition_t *DriverPartition;
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
#if defined ST_7015 || defined (ST_7020)
    int LVar;
    ST_DeviceName_t VINDeviceName;
    char InputType[4];
#endif
    STVID_InitParams_t InitParams;
    BOOL RetErr;
    STVID_DeviceType_t Device;

    ErrCode = ST_ERROR_BAD_PARAMETER;
    memset( &InitParams, 0, sizeof(InitParams)); /* set all params to null */

	/* get DeviceName */
    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName" );
    }

    /* Get device number */
#if defined ST_5510
    InitParams.DeviceType = STVID_DEVICE_TYPE_5510_MPEG;
#elif defined ST_5512
    InitParams.DeviceType = STVID_DEVICE_TYPE_5512_MPEG;
#elif defined ST_5508
    InitParams.DeviceType = STVID_DEVICE_TYPE_5508_MPEG;
#elif defined ST_5514
    InitParams.DeviceType = STVID_DEVICE_TYPE_5514_MPEG;
#elif defined ST_5518
    InitParams.DeviceType = STVID_DEVICE_TYPE_5518_MPEG;
#endif
#if defined ST_5510 || defined ST_5512 || defined ST_5508 || defined ST_5514 || defined ST_5518
    InitParams.BaseAddress_p = (void*) VIDEO_BASE_ADDRESS;
    InitParams.DeviceBaseAddress_p = (void*) 0;
    InitParams.SharedMemoryBaseAddress_p = (void*) SDRAM_BASE_ADDRESS;
    InitParams.InstallVideoInterruptHandler = TRUE;
    InitParams.InterruptNumber = VIDEO_INTERRUPT;
    InitParams.InterruptLevel = VIDEO_INTERRUPT_LEVEL;
#elif defined ST_7015 || defined (ST_7020)
    /* get input type  */
	if ( RetErr == FALSE )
	{
        RetErr = STTST_GetString( pars_p, "V", InputType, sizeof(InputType) );
	    if ( RetErr == TRUE || ( InputType[0]!='V' && InputType[0]!='v'
             && InputType[0]!='D' && InputType[0]!='d' ) )
	    {
	        STTST_TagCurrentLine( pars_p, "expected input type (\"V\"=video \"D\"=digital)" );
	    }
    }

    if ( InputType[0]=='v' || InputType[0]=='V' )
    {
#if defined (ST_7015)
        InitParams.DeviceType = STVID_DEVICE_TYPE_7015_MPEG;
#else
        InitParams.DeviceType = STVID_DEVICE_TYPE_7020_MPEG;
#endif
        RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar);
        if ( RetErr || (LVar < 1 || LVar > 5) )
        {
            STTST_TagCurrentLine( pars_p, "expected 7015/7020 video , VID1=1, VID2=2, VID3=3, VID4=4, VID5=5");
            RetErr = TRUE;
        }
        switch ( LVar )
        {
            case 1 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID1_OFFSET;
                InitParams.InterruptEvent = ST7015_VID1_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID1_OFFSET;
                InitParams.InterruptEvent = ST7020_VID1_INT;
#endif
                break;
            case 2 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID2_OFFSET;
                InitParams.InterruptEvent = ST7015_VID2_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID2_OFFSET;
                InitParams.InterruptEvent = ST7020_VID2_INT;
#endif
                break;
            case 3 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID3_OFFSET;
                InitParams.InterruptEvent = ST7015_VID3_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID3_OFFSET;
                InitParams.InterruptEvent = ST7020_VID3_INT;
#endif
                break;
            case 4 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID4_OFFSET;
                InitParams.InterruptEvent = ST7015_VID4_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID4_OFFSET;
                InitParams.InterruptEvent = ST7020_VID4_INT;
#endif
                break;
            case 5 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_VID5_OFFSET;
                InitParams.InterruptEvent = ST7015_VID5_INT;
#else
                InitParams.BaseAddress_p = (void*) ST7020_VID5_OFFSET;
                InitParams.InterruptEvent = ST7020_VID5_INT;
#endif
                break;
            default:
                InitParams.BaseAddress_p = (void*) 0;
                break;
        }
#if defined (ST_7015)
        InitParams.DeviceBaseAddress_p = (void*) STI7015_BASE_ADDRESS;
#else
        InitParams.DeviceBaseAddress_p = (void*) STI7020_BASE_ADDRESS;
#endif
        InitParams.SharedMemoryBaseAddress_p = (void*) (SDRAM_WINDOW_BASE_ADDRESS - SDRAM_WINDOW_BASE_ADDRESS);
        InitParams.InstallVideoInterruptHandler = FALSE;
        strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);
    }
    else
    {
#if defined (ST_7015)
        InitParams.DeviceType = STVID_DEVICE_TYPE_7015_DIGITAL_INPUT;
#else
        InitParams.DeviceType = STVID_DEVICE_TYPE_7020_DIGITAL_INPUT;
#endif
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar);
        if ( RetErr || (LVar < 1 || LVar > 3) )
        {
            STTST_TagCurrentLine( pars_p, "expected 7015/7020 input , SD1=1, SD2=2, HD=3");
            RetErr = TRUE;
        }
        switch ( LVar )
        {
            case 1 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_SDIN1_OFFSET; /* for digital SD1 input */
                InitParams.InterruptEvent = ST7015_SDIN1_INT; /* for digital input */
#else
                InitParams.BaseAddress_p = (void*) ST7020_SDIN1_OFFSET; /* for digital SD1 input */
                InitParams.InterruptEvent = ST7020_SDIN1_INT; /* for digital input */
#endif
                break;
            case 2 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_SDIN2_OFFSET; /* for digital SD2 input */
                InitParams.InterruptEvent = ST7015_SDIN2_INT; /* for digital input */
#else
                InitParams.BaseAddress_p = (void*) ST7020_SDIN2_OFFSET; /* for digital SD2 input */
                InitParams.InterruptEvent = ST7020_SDIN2_INT; /* for digital input */
#endif
                break;
            case 3 :
#if defined (ST_7015)
                InitParams.BaseAddress_p = (void*) ST7015_HDIN_OFFSET; /* for digital HD input */
                InitParams.InterruptEvent = ST7015_HDIN_INT; /* for digital input */
#else
                InitParams.BaseAddress_p = (void*) ST7020_HDIN_OFFSET; /* for digital HD input */
                InitParams.InterruptEvent = ST7020_HDIN_INT; /* for digital input */
#endif
                break;
            default:
                InitParams.BaseAddress_p = (void*) 0;
                break;
        }
#if defined (ST_7015)
        InitParams.DeviceBaseAddress_p = (void*) STI7015_BASE_ADDRESS;
#else
        InitParams.DeviceBaseAddress_p = (void*) STI7020_BASE_ADDRESS;
#endif
        InitParams.SharedMemoryBaseAddress_p = (void*) (SDRAM_WINDOW_BASE_ADDRESS - SDRAM_WINDOW_BASE_ADDRESS);
        InitParams.InstallVideoInterruptHandler = FALSE;
        strcpy(InitParams.InterruptEventName, STINTMR_DEVICE_NAME);

        if ( !RetErr)
        {
            /* get VIN DeviceName */
            RetErr = STTST_GetString( pars_p, "", VINDeviceName, sizeof(VINDeviceName) );
            if ( RetErr )
            {
                STTST_TagCurrentLine( pars_p, "expected VIN DeviceName" );
            }
            strcpy(InitParams.VINName, VINDeviceName);
        }
    }
#endif
    /* get Evt Name */
    if ( !RetErr )
	{
        RetErr = STTST_GetString( pars_p, STEVT_DEVICE_NAME, InitParams.EvtHandlerName, \
                                  sizeof(InitParams.EvtHandlerName) );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected EVT name");
        }
    }
    /* get Clkrv Name */
    if ( !RetErr )
	{
        RetErr = STTST_GetString( pars_p, STCLKRV_DEVICE_NAME, InitParams.ClockRecoveryName, \
                                  sizeof(InitParams.ClockRecoveryName) );
        if ( RetErr )
        {
            STTST_TagCurrentLine( pars_p, "expected CLKRV name");
        }
    }

	/* get MaxOpen */
    if ( !RetErr )
	{
        if ( Device <= 4)
        {
            RetErr = STTST_GetInteger( pars_p, 1, (S32 *)&InitParams.MaxOpen );
        }
        else
        {
            RetErr = STTST_GetInteger( pars_p, 2, (S32 *)&InitParams.MaxOpen );
        }
        if ( RetErr )
	    {
	        STTST_TagCurrentLine( pars_p, "expected MaxOpen" );
	    }
	}

    if ( !RetErr )
	{
        /* set other init. parameters */
        InitParams.BitBufferAllocated = FALSE;
        InitParams.BitBufferAddress_p = (void *) 0; /* don't care since BitBufferAllocated is FALSE */
        InitParams.BitBufferSize = 0;               /* don't care since BitBufferAllocated is FALSE */
        InitParams.UserDataSize = 0xFF; /* new for release 1.7.0 */
        InitParams.AVMEMPartition = AvmemPartitionHandle;
        InitParams.CPUPartition_p = DriverPartition;
        InitParams.AVSYNCDriftThreshold = 2 * (90000 / 50); /* 2 * VSync max time */

    	ErrCode = STVID_Init( DeviceName, &InitParams);
        sprintf( VID_Msg, "STVID_Init(device=%s): ", DeviceName);
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        sprintf( VID_Msg, "   type=%d baseaddr=%x devicebaseaddr=%x bballoc=%x bitbuff=%x size=%x \n",
                InitParams.DeviceType,
                (U32)InitParams.BaseAddress_p,
                (U32)InitParams.DeviceBaseAddress_p,
                (U32) InitParams.BitBufferAllocated,
                (U32)InitParams.BitBufferAddress_p,
                (U32)InitParams.BitBufferSize );
        sprintf( VID_Msg, "%s   intevt=%d evt=%s instint=%d intnb=%d intlev=%d \n",
                 VID_Msg,
                (U32)InitParams.InterruptEvent,
                InitParams.InterruptEventName,
                (U32)InitParams.InstallVideoInterruptHandler,
                (U32)InitParams.InterruptNumber,
                (U32)InitParams.InterruptLevel );

        sprintf( VID_Msg, "%s   partition=%d memaddr=%d mempart=%d maxopen=%d \n",
                 VID_Msg,
                (U32)InitParams.CPUPartition_p,
                (U32)InitParams.SharedMemoryBaseAddress_p,
                InitParams.AVMEMPartition,
                (U32)InitParams.MaxOpen );
        sprintf( VID_Msg, "%s   userdatasize=%d vin=%s evthdl=%s clkrv=%s avsyncthreshold=%d \n",
                VID_Msg, (U32)InitParams.UserDataSize,
                InitParams.VINName,
                InitParams.EvtHandlerName,
                InitParams.ClockRecoveryName,
                (U32)InitParams.AVSYNCDriftThreshold );
    	VID_PRINT(VID_Msg);
	}

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_Init() */


/*-------------------------------------------------------------------------
 * Function : VID_Open
 *            Share physical decoder usage
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Open( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_OpenParams_t OpenParams;
    ST_DeviceName_t DeviceName;
    STVID_Handle_t CurrentHandle=0;
    BOOL RetErr;

    ErrCode = ST_ERROR_UNKNOWN_DEVICE;

    /* get DeviceName */
    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName" );
    }
	/* get SyncDelay */
	if ( RetErr == FALSE )
	{
	    RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&OpenParams.SyncDelay );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected SyncDelay" );
	    }
	}
	if ( RetErr == FALSE )
	{
        RetErr = TRUE;
        ErrCode = STVID_Open( DeviceName, &OpenParams, &CurrentHandle );
        sprintf( VID_Msg, "STVID_Open(device=%s,syncdelay=%d,handle=%d) : ",
	             DeviceName, OpenParams.SyncDelay, CurrentHandle );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
        if ( ErrCode == ST_NO_ERROR )
        {
            VidDecHndl0 = CurrentHandle; /* default handle */
	    }
	}
    STTST_AssignInteger( result_sym_p, (int)CurrentHandle, FALSE);
    STTST_AssignInteger("RET_VAL1", CurrentHandle, FALSE);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_Open() */

/*-------------------------------------------------------------------------
 * Function : VID_OpenViewPort
 *            Open a viewport for a decoder on a layer
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_OpenViewPort( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_ViewPortParams_t ViewPortParams;
    STVID_ViewPortHandle_t CurrentViewportHndl;
    STVID_Handle_t CurrentHandle;
    BOOL RetErr = FALSE;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get LayerName */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetString( pars_p, STLAYER_VID_DEVICE_NAME, ViewPortParams.LayerName, \
                        sizeof(ViewPortParams.LayerName) );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected LayerName");
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STVID_OpenViewPort( CurrentHandle, &ViewPortParams, &CurrentViewportHndl );
        sprintf( VID_Msg, "STVID_OpenViewPort(handle=%d,layer=%s,viewporthndl=%d) : ",
                 CurrentHandle, ViewPortParams.LayerName, (U32)CurrentViewportHndl );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            ViewPortHndl0 = CurrentViewportHndl; /* default viewport */
        }
    }
    STTST_AssignInteger( result_sym_p, (int)CurrentViewportHndl, FALSE);
    STTST_AssignInteger("RET_VAL1", (S32)CurrentViewportHndl, FALSE);

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_OpenViewPort() */

/*-------------------------------------------------------------------------
 * Function : VID_Pause
 *            Stop the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Pause( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_Freeze_t Freeze;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get freeze mode */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_MODE_FORCE, (S32 *)&Freeze.Mode );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Mode (%d=none %d=force %d=no_flicker)" ,
                        STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
            STTST_TagCurrentLine( pars_p, VID_Msg);
        }
    }
    /* get freeze field */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_FIELD_TOP, (S32 *)&Freeze.Field );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Field (%d=top %d=bot %d=current %d=next)" ,
                    STVID_FREEZE_FIELD_TOP,	STVID_FREEZE_FIELD_BOTTOM,
                    STVID_FREEZE_FIELD_CURRENT,	STVID_FREEZE_FIELD_NEXT );
            STTST_TagCurrentLine( pars_p, VID_Msg);
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STVID_Pause( CurrentHandle, &Freeze );
        sprintf( VID_Msg, "STVID_Pause(handle=%d,(freezemode=%d,freezefield=%d)) : ",
                 CurrentHandle, Freeze.Mode, Freeze.Field );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_Pause() */

/*-------------------------------------------------------------------------
 * Function : VID_PauseSynchro
 *            Resume the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_PauseSynchro( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STTS_t Time;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get duration */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&Time );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected duration of the pause");
        }
    }

    if ( RetErr == FALSE )
    {
        ErrCode = STVID_PauseSynchro( CurrentHandle, Time );
        sprintf( VID_Msg, "STVID_PauseSynchro(handle=%d) : ", CurrentHandle );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_PauseSynchro() */

/*-------------------------------------------------------------------------
 * Function : VID_Resume
 *            Resume the decoding
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_Resume( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_Resume( CurrentHandle );
        sprintf ( VID_Msg, "STVID_Resume(handle=%d) : ", CurrentHandle );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_Resume() */

/*-------------------------------------------------------------------------
 * Function : VID_SetDecodedPictures
 *            Set decoded pictures type
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetDecodedPictures( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_DecodedPictures_t DecodedPictures;
    BOOL RetErr;
    STVID_Handle_t CurrentHandle;

    RetErr = FALSE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get type of pictures */
    if ( RetErr == FALSE )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)STVID_DECODED_PICTURES_ALL, (S32 *)&DecodedPictures );
	    if ( RetErr == TRUE )
	    {
            sprintf(VID_Msg,"expected Decoded Picture (%d=ALL %d=IP %d=I)",
                    STVID_DECODED_PICTURES_ALL, STVID_DECODED_PICTURES_IP, STVID_DECODED_PICTURES_I );
	        STTST_TagCurrentLine( pars_p, VID_Msg );
	    }
    }

    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "VID_SetDecodedPictures(handle=%d,decodedpict=%d) : ",
                          CurrentHandle, (S32)DecodedPictures );
        ErrCode = STVID_SetDecodedPictures( CurrentHandle, DecodedPictures );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_SetDecodedPictures() */


/*-------------------------------------------------------------------------
 * Function : VID_SetDisplayARConversion
 *            Set pel ratio conversion
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetDisplayARConversion( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_DisplayAspectRatioConversion_t RatioCv;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	    }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get RatioCv */
	if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_DISPLAY_AR_CONVERSION_PAN_SCAN, (S32 *)&RatioCv);
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected RatioCv (%x=pan&scan,%x=letter box,"
                    "%x=combined,%x=ignore)",
                     STVID_DISPLAY_AR_CONVERSION_PAN_SCAN,
                     STVID_DISPLAY_AR_CONVERSION_LETTER_BOX,
                     STVID_DISPLAY_AR_CONVERSION_COMBINED,
                     STVID_DISPLAY_AR_CONVERSION_IGNORE);
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
	if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        /* set conversion ratio */
        sprintf( VID_Msg, "STVID_SetDisplayAspectRatioConversion(viewporthndl=%d,ratio=%d) : ",
                 (U32)CurrentViewportHndl, RatioCv );
        ErrCode = STVID_SetDisplayAspectRatioConversion( CurrentViewportHndl, RatioCv );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetDisplayARConversion() */

/*-------------------------------------------------------------------------
 * Function : VID_SetErrorRecoveryMode
 *            Set driver behavior regarding error recovery
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetErrorRecoveryMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_ErrorRecoveryMode_t Mode;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get Mode */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_ERROR_RECOVERY_FULL, (S32 *)&Mode);
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Mode (%x=full,%x=partial,%x=none)",
                     STVID_ERROR_RECOVERY_FULL,
                     STVID_ERROR_RECOVERY_PARTIAL,
                     STVID_ERROR_RECOVERY_NONE);
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_SetErrorRecoveryMode(hanle=%d,mode=%d) : ",
                 CurrentHandle, Mode );
        ErrCode = STVID_SetErrorRecoveryMode( CurrentHandle, Mode);
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetErrorRecoveryMode() */

/*-------------------------------------------------------------------------
 * Function : VID_SetInputWindowMode
 *            Set input window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetInputWindowMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_WindowParams_t WinParams;
    BOOL Auto;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	    }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get Auto */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, FALSE, (S32 *)&Auto );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected auto (%d=TRUE,%d=FALSE)", TRUE, FALSE );
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    /* get Align */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&WinParams.Align );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected align (%d,%d,%d,%d,%d,%d,%d,%d,%d)",
				STVID_WIN_ALIGN_TOP_LEFT,
				STVID_WIN_ALIGN_VCENTRE_LEFT,
				STVID_WIN_ALIGN_BOTTOM_LEFT,
				STVID_WIN_ALIGN_TOP_RIGHT,
				STVID_WIN_ALIGN_VCENTRE_RIGHT,
				STVID_WIN_ALIGN_BOTTOM_RIGHT,
				STVID_WIN_ALIGN_BOTTOM_HCENTRE,
				STVID_WIN_ALIGN_TOP_HCENTRE,
                STVID_WIN_ALIGN_VCENTRE_HCENTRE );
	        STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    /* get Size */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_WIN_SIZE_DONT_CARE, (S32 *)&WinParams.Size );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected size (fix=%d,dont_care=%d,incr=%d,decr=%d)",
                     STVID_WIN_SIZE_FIXED,
                     STVID_WIN_SIZE_DONT_CARE,
                     STVID_WIN_SIZE_INCREASE,
                     STVID_WIN_SIZE_DECREASE );
	        STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( VID_Msg, "STVID_SetInputWindowMode(viewporthndl=%d,auto=%d,(align=%d,size=%d)) : ",
                 (U32)CurrentViewportHndl, Auto, WinParams.Align, WinParams.Size );
        RetErr = TRUE;
        ErrCode = STVID_SetInputWindowMode( CurrentViewportHndl, Auto, &WinParams);
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetInputWindowMode() */


/*-------------------------------------------------------------------------
 * Function : VID_SetIOWindows
 *            Adjust the window size and position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetIOWindows( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    S16 InputWinPositionX;
    S16 InputWinPositionY;
    S16 InputWinWidth;
    S16 InputWinHeight;
    S16 OutputWinPositionX;
    S16 OutputWinPositionY;
    S16 OutputWinWidth;
    S16 OutputWinHeight;
	BOOL Trace;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	    }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get InputWinPositionX */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&InputWinPositionX );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected InputWinPositionX" );
        }
    }
    /* get InputWinPositionY */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&InputWinPositionY );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected InputWinPositionY" );
        }
    }
    /* get InputWinWidth */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, DEFAULT_SCREEN_WIDTH, (S32 *)&InputWinWidth );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinWidth" );
        }
    }
    /* get InputWinHeight */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, DEFAULT_SCREEN_HEIGHT, (S32 *)&InputWinHeight );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected InputWinHeight" );
        }
    }
    /* get OutputWinPositionX */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&OutputWinPositionX );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected OutputWinPositionX" );
        }
    }
    /* get OutputWinPositionY */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&OutputWinPositionY );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected OutputWinPositionY" );
        }
    }
    /* get OutputWinWidth */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, DEFAULT_SCREEN_WIDTH, (S32 *)&OutputWinWidth );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinWidth" );
        }
    }
    /* get OutputWinHeight */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, DEFAULT_SCREEN_HEIGHT, (S32 *)&OutputWinHeight );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected OutputWinHeight" );
        }
    }
    /* get Trace (Sept 2000 : argument added because VID_PRINT can't be always disable) */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, TRUE, (S32 *)&Trace );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected Trace (TRUE/FALSE)" );
        }
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( VID_Msg, "STVID_SetIOWindows(viewporthndl=%d,ix=%d,iy=%d,iw=%d,ih=%d,ox=%d,oy=%d,ow=%d,oh=%d) : ",
                 (U32)CurrentViewportHndl,
                 InputWinPositionX, InputWinPositionY, InputWinWidth, InputWinHeight,
                 OutputWinPositionX, OutputWinPositionY, OutputWinWidth, OutputWinHeight );
        ErrCode = STVID_SetIOWindows( CurrentViewportHndl,
                                      InputWinPositionX, InputWinPositionY,
                                      InputWinWidth, InputWinHeight,
                                      OutputWinPositionX, OutputWinPositionY,
                                      OutputWinWidth, OutputWinHeight );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    } /* end if */

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetIOWindows() */

/*-------------------------------------------------------------------------
 * Function : VID_SetMemoryProfile
 *            Set amount of memory
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetMemoryProfile( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_MemoryProfile_t MemoryProfile;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get MaxWidth */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MemoryProfile.MaxWidth );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected MaxWidth" );
        }
    }
    /* get MaxHeight */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MemoryProfile.MaxHeight );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected MaxHeight" );
        }
    }
    /* get NbFrameStore */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MemoryProfile.NbFrameStore );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected NbFrameStore" );
        }
    }
    /* get CompressionLevel */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MemoryProfile.CompressionLevel );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected CompressionLevel" );
        }
    }
    /* get DecimationFactor */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MemoryProfile.DecimationFactor );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected DecimationFactor" );
        }
    }

    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_SetMemoryProfile( CurrentHandle, &MemoryProfile );
        sprintf( VID_Msg, "STVID_SetMemoryProfile(handle=%d,(width=%d,height=%d,nbfr=%d,compress=%d,decim=%d)) : ",
                 CurrentHandle, MemoryProfile.MaxWidth, MemoryProfile.MaxHeight,
                 MemoryProfile.NbFrameStore, MemoryProfile.CompressionLevel,
                 MemoryProfile.DecimationFactor );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    } /* end if */

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetMemoryProfile() */

#ifdef STVID_WRAPPER_OLDARCH

/*-------------------------------------------------------------------------
 * Function : VID_SetMixWeight
 *            Set blending factor video plane / below plane
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetMixWeight( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    U8 MixWeight;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get mix weight */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&MixWeight );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected MixWeight" );
        }
    }
    if ( RetErr == FALSE )
    {
        RetErr = TRUE;
        ErrCode = STVID_SetMixWeight( CurrentHandle, MixWeight );
        sprintf( VID_Msg, "STVID_SetMixWeight(handle=%d,mix=%d) : ", CurrentHandle, MixWeight );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_SetMixWeight */

#endif

/*-------------------------------------------------------------------------
 * Function : VID_SetOutputWindowMode
 *            Set output window parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_SetOutputWindowMode( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_WindowParams_t WinParams;
    BOOL Auto;
    BOOL RetErr = FALSE;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	    }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get auto mode*/
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, FALSE, (S32 *)&Auto );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected auto mode (%d=TRUE,%d=FALSE)", TRUE, FALSE );
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    /* get align */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_WIN_ALIGN_TOP_LEFT, (S32 *)&WinParams.Align );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected align (%d,%d,%d,%d,%d,%d,%d,%d,%d)",
				STVID_WIN_ALIGN_TOP_LEFT,
				STVID_WIN_ALIGN_VCENTRE_LEFT,
				STVID_WIN_ALIGN_BOTTOM_LEFT,
				STVID_WIN_ALIGN_TOP_RIGHT,
				STVID_WIN_ALIGN_VCENTRE_RIGHT,
				STVID_WIN_ALIGN_BOTTOM_RIGHT,
				STVID_WIN_ALIGN_BOTTOM_HCENTRE,
				STVID_WIN_ALIGN_TOP_HCENTRE,
                STVID_WIN_ALIGN_VCENTRE_HCENTRE );
	        STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    /* get size */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_WIN_SIZE_DONT_CARE, (S32 *)&WinParams.Size );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected size (fix=%d,dont_care=%d,incr=%d,decr=%d)",
                     STVID_WIN_SIZE_FIXED,
                     STVID_WIN_SIZE_DONT_CARE,
                     STVID_WIN_SIZE_INCREASE,
                     STVID_WIN_SIZE_DECREASE );
	        STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }

    if ( RetErr == FALSE )
	{
        RetErr = TRUE;
    	ErrCode = STVID_SetOutputWindowMode( CurrentViewportHndl, Auto, &WinParams );
    	sprintf( VID_Msg, "STVID_SetOutputWindowMode(viewporthndl=%d,auto=%d,(align=%d,size=%d)) : ",
	             (U32)CurrentViewportHndl, Auto, WinParams.Align, WinParams.Size );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
	}

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetOutputWindowMode() */

#ifdef STVID_WRAPPER_OLDARCH

/*-------------------------------------------------------------------------
 * Function : VID_SetScreenOffset
 *            Set screen position
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetScreenOffset( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    S16 Horizontal;
    S16 Vertical;
    S32 LVar;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get Horizontal (default : no shift) */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Horizontal" );
        }
    }
	Horizontal = (S16)LVar;
    /* get Vertical (default : no shift) */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected Vertical" );
        }
		Vertical = (S16)LVar;
    }
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        /* set screen position */
        sprintf( VID_Msg, "STVID_SetScreenOffset(handle=%d,horiz=%d,vert=%d) : ",
                 CurrentHandle, Horizontal, Vertical );
        ErrCode = STVID_SetScreenOffset( CurrentHandle, Horizontal, Vertical );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    } /* end if */

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_SetScreenOffset */


/*-------------------------------------------------------------------------
 * Function : VID_SetScreenParams
 *            Set screen parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetScreenParams( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr = FALSE;
    S32 LVar;
    STVID_Handle_t CurrentHandle;

    /* init. default values into globale variable ScreenParams */
    memset(&ScreenParams, 0, sizeof(ScreenParams));
    RetErr = VID_GetScreenParams( (parse_t *)NULL, '\0' );

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get Aspect */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.Aspect, &LVar ) ;
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Aspect (%x=16:9,%x=4:3,%x=221,%x=square)",
                     STVID_DISPLAY_ASPECT_RATIO_16TO9, STVID_DISPLAY_ASPECT_RATIO_4TO3,
				     STVID_DISPLAY_ASPECT_RATIO_221TO1,	STVID_DISPLAY_ASPECT_RATIO_SQUARE );
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
	    ScreenParams.Aspect = (STVID_DisplayAspectRatio_t)LVar ;
    }
    /* get Width */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.Width, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected Width" );
        }
		ScreenParams.Width = (U16)LVar ;
    }
    /* get Height */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.Height, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine(pars_p, "expected Height" );
        }
		ScreenParams.Height = (U16)LVar ;
    }
    /* get XOffset (default : 132 pixels)*/
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.XOffset, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected XOffset" );
        }
		ScreenParams.XOffset = (U8)LVar ;
    }
    /* get YOffset (default : 22 pixels)*/
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.YOffset, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected YOffset");
        }
		ScreenParams.YOffset = (U8)LVar ;
    }
    /* get ScanType */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.ScanType, &LVar);
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected ScanType (%d=Progressive %d=Interlaced)",
                     STVID_SCAN_TYPE_PROGRESSIVE, STVID_SCAN_TYPE_INTERLACED);
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
		ScreenParams.ScanType = (STVID_ScanType_t)LVar ;
    }
    /* get frame rate */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, ScreenParams.FrameRate, &LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected FrameRate" );
        }
		ScreenParams.FrameRate = (U8)LVar ;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        sprintf( VID_Msg, "STVID_SetScreenParams(handle=%d,&param=%ld) : ",
                 CurrentHandle, (long)&ScreenParams );
        ErrCode = STVID_SetScreenParams( CurrentHandle, &ScreenParams );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf( VID_Msg, "   aspect=%d width=%d height=%d xoffset=%d yoffset=%d scantype=%d framerate=%d \n",
                        ScreenParams.Aspect,
                        ScreenParams.Width, ScreenParams.Height,
                        ScreenParams.XOffset, ScreenParams.YOffset,
                        ScreenParams.ScanType, ScreenParams.FrameRate );
            VID_PRINT(VID_Msg);
        }
    } /* end if */

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_SetScreenParams */

#endif

/*-------------------------------------------------------------------------
 * Function : VID_SetSpeed
 *            Set play speed
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SetSpeed( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    S32 Speed;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get Speed */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&Speed );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected Speed" );
        }
    }
    if ( RetErr == FALSE)
    {
        ErrCode = STVID_SetSpeed( CurrentHandle, Speed );
        sprintf( VID_Msg, "STVID_SetSpeed(handle=%d,speed=%d) : ", CurrentHandle, Speed );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            STTST_AssignInteger("RET_VAL1", Speed, FALSE);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SetSpeed() */

/*-------------------------------------------------------------------------
 * Function : VID_ShowPicture
 *            Restore the display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_ShowPicture( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_PictureParams_t PictParams;
    STVID_Freeze_t Freeze;
    BOOL RetErr = FALSE;
    short PictureNo;
    STVID_ViewPortHandle_t CurrentViewportHndl;

    if ( NbOfCopiedPictures == 0 )
    {
        STTST_TagCurrentLine( pars_p, "VID_ShowPicture() : no picture to show in memory !" );
        return(API_EnableError ? TRUE : FALSE );
    }

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get ViewPortHandle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)ViewPortHndl0, (S32 *)&CurrentViewportHndl );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected ViewPortHandle" );
	    }
    }
    else
    {
        CurrentViewportHndl = ViewPortHndl0;
    }
    /* get picture number */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, NbOfCopiedPictures, (S32 *)&PictureNo );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Picture No. (max is %d)", NbOfCopiedPictures );
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    if ( PictureNo > NbOfCopiedPictures )
    {
        PictureNo = NbOfCopiedPictures ;
    }
    if ( PictureNo <= 0 )
    {
        PictureNo = 1 ;
    }

    Freeze.Field = STVID_FREEZE_FIELD_TOP;
    Freeze.Mode = STVID_FREEZE_MODE_FORCE;
    PictParams = CopiedPictureParams[PictureNo-1];

    if (pars_p->line_p[pars_p->par_pos] == '\0')  /* no param on line */
    {
        RetErr = FALSE;
    }
    else
    {
        /* get coding mode */
        /*RetErr = STTST_GetInteger( pars_p, STVID_CODING_MODE_MB, (S32 *)&PictParams.CodingMode );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected CodingMode (%d=STVID_CODING_MODE_MB)", STVID_CODING_MODE_MB);
            STTST_TagCurrentLine( pars_p, VID_Msg);
        }*/
        /* get color space */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, STVID_COLOR_SPACE_YUV420, (S32 *)&PictParams.ColorType );
            if ( RetErr == TRUE )
            {
                sprintf( VID_Msg, "expected ColorType (%d=STVID_COLOR_SPACE_YUV420)", STVID_COLOR_SPACE_YUV420 );
                STTST_TagCurrentLine( pars_p, VID_Msg);
            }
        }*/
        /* get mpeg frame */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, STVID_MPEG_FRAME_I , (S32 *)&PictParams.MPEGFrame );
            if ( RetErr == TRUE )
            {
                sprintf( VID_Msg, "expected MPEGFrame (%d=I, %d=B, %d=P)",
                                    STVID_MPEG_FRAME_I, STVID_MPEG_FRAME_B, STVID_MPEG_FRAME_P );
                STTST_TagCurrentLine( pars_p, VID_Msg);
            }
        }*/
        /* get memory handle */
        if ( RetErr == FALSE )
        {
          /*RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.MemoryHandle );
          if ( RetErr == TRUE )
          {
              STTST_TagCurrentLine( pars_p, "expected memory handle" );
          }*/
        }
	    /* PictParams.MemoryHandle = VID_DestBufferHnd ; */

        /* get height */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, DEFAULT_SCREEN_HEIGHT, (S32 *)&PictParams.Height );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Height" );
            }
        }*/
        /* get width */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, DEFAULT_SCREEN_WIDTH, (S32 *)&PictParams.Width );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Width" );
            }
        }*/
        /* get aspect */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, STVID_DISPLAY_ASPECT_RATIO_4TO3, (S32 *)&PictParams.Aspect );
            if ( RetErr == TRUE )
            {
                sprintf( VID_Msg, "expected Aspect (%d=4:3 %d=16:9 %d=2211 %d=square)",
                    STVID_DISPLAY_ASPECT_RATIO_4TO3, STVID_DISPLAY_ASPECT_RATIO_16TO9,
                    STVID_DISPLAY_ASPECT_RATIO_221TO1, STVID_DISPLAY_ASPECT_RATIO_SQUARE );
                STTST_TagCurrentLine( pars_p, VID_Msg);
            }
        }*/
        /* get scan type */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, STVID_SCAN_TYPE_PROGRESSIVE, (S32 *)&PictParams.ScanType );
            if ( RetErr == TRUE )
            {
                sprintf( VID_Msg, "expected ScanType (%d=progressive %d=interlaced)",
                    STVID_SCAN_TYPE_PROGRESSIVE, STVID_SCAN_TYPE_INTERLACED );
                STTST_TagCurrentLine( pars_p, VID_Msg);
            }
        }*/
        /* get top field first */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.TopFieldFirst );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected TopFieldFirst" );
            }
        }*/
        /* get frame rate */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.FrameRate );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected FrameRate" );
            }
        }*/
        /* get hours */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.TimeCode.Hours );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Hours" );
            }
        }*/
        /* get minutes */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.TimeCode.Minutes );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Minutes" );
            }
        }*/
        /* get seconds */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.TimeCode.Seconds );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Seconds" );
            }
        }*/
        /* get frames */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.TimeCode.Frames );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Frames" );
            }
        }*/
        /* get interpolated */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.TimeCode.Interpolated );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Interpolated" );
            }
        }*/
        /* get PTS */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.PTS.PTS );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected PTS" );
            }
        }*/
        /* get PTS33 */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.PTS.PTS33 );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected PTS33" );
            }
        }*/
        /* get interpolated */
        /*if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, 0, (S32 *)&PictParams.PTS.Interpolated );
            if ( RetErr == TRUE )
            {
                STTST_TagCurrentLine( pars_p, "expected Interpolated" );
            }
        }*/
        /* get freeze mode */
        if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_MODE_FORCE, (S32 *)&Freeze.Mode );
            if ( RetErr == TRUE )
            {
                sprintf( VID_Msg, "expected Mode (%d=none %d=force %d=no_flicker)" ,
                            STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
                STTST_TagCurrentLine( pars_p, VID_Msg);
            }
        }
        /* get freeze field */
        if ( RetErr == FALSE )
        {
            RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_FIELD_TOP, (S32 *)&Freeze.Field );
            if ( RetErr == TRUE )
            {
                sprintf( VID_Msg, "expected Field (%d=top %d=bot %d=current %d=next)" ,
                        STVID_FREEZE_FIELD_TOP,	STVID_FREEZE_FIELD_BOTTOM,
                        STVID_FREEZE_FIELD_CURRENT,	STVID_FREEZE_FIELD_NEXT );
                STTST_TagCurrentLine( pars_p, VID_Msg);
            }
        }
    } /* end if input param. */

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        ErrCode = STVID_ShowPicture( CurrentViewportHndl, &PictParams, &Freeze );
        sprintf( VID_Msg, "STVID_ShowPicture(viewporthndl=%d,&param=%d,&freeze=%d) : ",
                 (S32)CurrentViewportHndl, (S32)&PictParams, (S32)&Freeze );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        if ( ErrCode == ST_NO_ERROR )
        {
            sprintf( VID_Msg, "   codingmode=%d colortype=%d mpegframe=%d height=%d width=%d aspect=%d \n",
                        PictParams.CodingMode,
                        PictParams.ColorType, PictParams.MPEGFrame,
                        PictParams.Height,
                        PictParams.Width, PictParams.Aspect );
            sprintf( VID_Msg, "%s   scantype=%d topfieldfirst=%d framerate=%d hours=%d minutes=%d seconds=%d \n",
                        VID_Msg, PictParams.ScanType,
                        PictParams.TopFieldFirst, PictParams.FrameRate,
                        PictParams.TimeCode.Hours, PictParams.TimeCode.Minutes,
                        PictParams.TimeCode.Seconds );
            sprintf( VID_Msg, "%s   frames=%d interpolated=%d pts=%d pts33=%d ptsinterpolated=%d freezemode=%d freezefield=%d \n",
                        VID_Msg, PictParams.TimeCode.Frames,
                        PictParams.TimeCode.Interpolated, PictParams.PTS.PTS,
                        PictParams.PTS.PTS33, PictParams.PTS.Interpolated,
                        Freeze.Mode, Freeze.Field );
            VID_PRINT(VID_Msg);
        }
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_ShowPicture */

/*-------------------------------------------------------------------------
 * Function : VID_SkipSynchro
 *            Skip some pictures
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_SkipSynchro( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STTS_t Time;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get time to skip */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, TRUE, (S32 *)&Time );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected time to skip" );
        }
    }
    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_SkipSynchro(handle=%d,time=%d) : ",
                 CurrentHandle, Time );
        ErrCode = STVID_SkipSynchro(CurrentHandle, Time );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

    } /* end if */

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_SkipSynchro() */

/*-------------------------------------------------------------------------
 * Function : VID_Start
 *            Start decoding of input stream
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Start( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_StartParams_t StartParams;
    S32 LVar;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get RealTime */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, TRUE, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected RealTime decoding (TRUE=yes FALSE=no)" );
        }
    }
    /* get UpdateDisplay  */
    if ( RetErr == FALSE )
    {
        StartParams.RealTime = (BOOL)LVar;
        RetErr = STTST_GetInteger( pars_p, TRUE, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected UpdateDisplay (TRUE=yes FALSE=no)" );
        }
    }
    /* get stream type */
    if ( RetErr == FALSE )
    {
        StartParams.UpdateDisplay = (BOOL)LVar;
#ifdef STVID_DIRECTV
        RetErr = STTST_GetInteger( pars_p, STVID_STREAM_TYPE_ES, &LVar);
#else
        RetErr = STTST_GetInteger( pars_p, STVID_STREAM_TYPE_PES, &LVar);
#endif
        if ( RetErr == TRUE )
        {
          sprintf( VID_Msg, "expected StreamType (%x=ES,%x=PES,%x=Mpeg1 pack.)",
                   STVID_STREAM_TYPE_ES, STVID_STREAM_TYPE_PES,
                   STVID_STREAM_TYPE_MPEG1_PACKET );
          STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    /* get stream id (default : accept all packets in decoder) */
    if ( RetErr == FALSE )
    {
        StartParams.StreamType = LVar;
        RetErr = STTST_GetInteger( pars_p, STVID_IGNORE_ID, &LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected id (from 0 to 15)");
        }
    }
    /* get broadcast type */
    if ( RetErr == FALSE )
    {
        StartParams.StreamID = (U8)LVar;
#ifdef STVID_DIRECTV
        RetErr = STTST_GetInteger( pars_p, STVID_BROADCAST_DIRECTV, &LVar);
#else
        RetErr = STTST_GetInteger( pars_p, STVID_BROADCAST_DVB, &LVar);
#endif
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected broadcast type (%d=DVB,%d=DSS,%d=ATSC,%d=DVD)",
                    STVID_BROADCAST_DVB, STVID_BROADCAST_DIRECTV,
                    STVID_BROADCAST_ATSC, STVID_BROADCAST_DVD );
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
    /* get DecodeOnce */
    if ( RetErr == FALSE )
    {
        StartParams.BrdCstProfile = (U8)LVar;
        RetErr = STTST_GetInteger( pars_p, FALSE, &LVar);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected DecodeOnce (FALSE=all picture, TRUE=1 picture)" );
        }
        else
        {
            StartParams.DecodeOnce = (U8)LVar;
        }
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_Start(handle=%d,&param=%d) : ",
                 CurrentHandle, (S32)&StartParams );
        ErrCode = STVID_Start(CurrentHandle, &StartParams );
        RetErr  = VID_TextError( ErrCode, VID_Msg);

        /* Trace */
        sprintf( VID_Msg, "   realtime=%d updatedisplay=%d streamtype=%d ",
                 StartParams.RealTime,
                 StartParams.UpdateDisplay, StartParams.StreamType );
        switch(StartParams.StreamType)
        {
            case STVID_STREAM_TYPE_ES:
                strcat(VID_Msg, "(ES) \n");
                break;
            case STVID_STREAM_TYPE_PES:
                strcat(VID_Msg, "(PES) \n");
                break;
            case STVID_STREAM_TYPE_MPEG1_PACKET:
                strcat(VID_Msg, "(Mpeg1 Packet) \n");
                break;
        }
        sprintf( VID_Msg, "%s   streamid=%d broadcast=%d ",
                 VID_Msg,
                 StartParams.StreamID, StartParams.BrdCstProfile);
        switch(StartParams.BrdCstProfile)
        {
            case STVID_BROADCAST_DVB:
                strcat(VID_Msg, "(DVB)");
                break;
            case STVID_BROADCAST_DIRECTV:
                strcat(VID_Msg, "(DIRECTV)");
                break;
            case STVID_BROADCAST_ATSC:
                strcat(VID_Msg, "(ATSC)");
                break;
            case STVID_BROADCAST_DVD:
                strcat(VID_Msg, "(DVD)");
                break;
        }
        sprintf( VID_Msg, "%s decodeonce=%d\n",
                 VID_Msg, StartParams.DecodeOnce );
        VID_PRINT(VID_Msg);

    } /* end if */

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_Start */

/*-------------------------------------------------------------------------
 * Function : VID_Stop
 *            Stop decoding and display
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Stop( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    STVID_Stop_t StopMode;
    STVID_Freeze_t Freeze;
    S32 LVar;
    BOOL RetErr = FALSE;
    STVID_Handle_t CurrentHandle;

    ErrCode = ST_ERROR_INVALID_HANDLE;

    /* get Handle */
    if ( API_TestMode == TEST_MODE_MULTI_HANDLES )
    {
	    RetErr = STTST_GetInteger( pars_p, (int)VidDecHndl0, (S32 *)&CurrentHandle );
	    if ( RetErr == TRUE )
	    {
	        STTST_TagCurrentLine( pars_p, "expected Handle" );
	    }
    }
    else
    {
        CurrentHandle = VidDecHndl0;
    }
    /* get stop mode */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_STOP_NOW, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected StopMode (%d=next ref,%d=end of data,%d=now)",
                STVID_STOP_WHEN_NEXT_REFERENCE, STVID_STOP_WHEN_END_OF_DATA,
                STVID_STOP_NOW);
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
    }
	StopMode = (STVID_Stop_t)LVar;
    /* get freeze field */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_FIELD_TOP, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Field (%x=STVID_FREEZE_FIELD_TOP %x=STVID_FREEZE_FIELD_BOTTOM %x=STVID_FREEZE_FIELD_CURRENT %x=STVID_FREEZE_FIELD_NEXT)",
                    STVID_FREEZE_FIELD_TOP, STVID_FREEZE_FIELD_BOTTOM,
                    STVID_FREEZE_FIELD_CURRENT, STVID_FREEZE_FIELD_NEXT);
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
		Freeze.Field = (STVID_FreezeField_t)LVar;
    }
    /* get freeze mode */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, STVID_FREEZE_MODE_FORCE, &LVar );
        if ( RetErr == TRUE )
        {
            sprintf( VID_Msg, "expected Mode (%x=STVID_FREEZE_MODE_NONE, %x=FREEZE_MODE_FORCE, %x=FREEZE_MODE_NO_FLICKER,)",
 				 		  	  STVID_FREEZE_MODE_NONE, STVID_FREEZE_MODE_FORCE, STVID_FREEZE_MODE_NO_FLICKER );
            STTST_TagCurrentLine( pars_p, VID_Msg );
        }
		Freeze.Mode = (STVID_FreezeMode_t)LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf( VID_Msg, "STVID_Stop(handle=%d,stopmode=%d,(freezefield=%d,freezemode=%d)) :",
                 CurrentHandle, StopMode, Freeze.Field, Freeze.Mode );
        ErrCode = STVID_Stop( CurrentHandle, StopMode, &Freeze );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end of VID_Stop() */

/*-------------------------------------------------------------------------
 * Function : VID_Term
 *            Terminate the video decoder
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VID_Term( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode;
    ST_DeviceName_t DeviceName;
    STVID_TermParams_t TermParams;
    S32 LVar;
    BOOL RetErr;

    ErrCode = ST_ERROR_BAD_PARAMETER;

    /* get DeviceName */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, STVID_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName" );
    }
    /* get term param */
    if ( RetErr == FALSE )
    {
        RetErr = STTST_GetInteger( pars_p, 0, &LVar );
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "expected ForceTerminate (TRUE, FALSE)" );
        }
		TermParams.ForceTerminate = (BOOL)LVar;
    }

    if ( RetErr == FALSE ) /* if parameters are ok */
    {
        RetErr = TRUE;
        sprintf ( VID_Msg, "STVID_Term(device=%s,forceterminate=%d) : ", DeviceName, TermParams.ForceTerminate );
        ErrCode = STVID_Term( DeviceName, &TermParams );
        RetErr  = VID_TextError( ErrCode, VID_Msg);
    }

    return ( API_EnableError ? RetErr : FALSE );

} /* end VID_Term */



/*#######################################################################*/
/*##########################  VIDEO COMMANDS  ###########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VID_MacroInit
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VID_InitCommand (void)
{
    BOOL RetErr;

    /* Macro's name, link to C-function, help message  */
    /* (by alphabetic order of macros = display order) */

    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "VID_CLOSE",     VID_Close,                 "<Handle> Close");
    RetErr |= STTST_RegisterCommand( "VID_CLOSEVP",   VID_CloseViewPort,         "<ViewportHndl> Close viewport");
    RetErr |= STTST_RegisterCommand( "VID_DISP",      VID_EnableOutputWindow,    "<ViewportHndl> Enable the display of the video layer");
    RetErr |= STTST_RegisterCommand( "VID_DUPLICATE", VID_DuplicatePicture,      "Copy a picture");
    RetErr |= STTST_RegisterCommand( "VID_FREEZE",    VID_Freeze,                "<Handle> Freeze the decoding");
    RetErr |= STTST_RegisterCommand( "VID_GETALIGN",  VID_GetAlignIOWindows,     "<ViewportHndl> Get the closiest window");
    RetErr |= STTST_RegisterCommand( "VID_GETBBFSIZE", VID_GetBitBufferFreeSize,      "<DeviceName> Get bit buffer free size");
    RetErr |= STTST_RegisterCommand( "VID_GETCAPA",   VID_GetCapability,         "<DeviceName> Get driver's capability");
    RetErr |= STTST_RegisterCommand( "VID_GETDATAINT",VID_GetDataInterfaceParams,"<Handle> Get parameters used to send data");
    RetErr |= STTST_RegisterCommand( "VID_GETDECP",   VID_GetDecodedPictures,    "<Handle> Get decoded pictures type");
    RetErr |= STTST_RegisterCommand( "VID_GETERR",    VID_GetErrorRecoveryMode,  "<Handle> Get error recovery mode");
    RetErr |= STTST_RegisterCommand( "VID_GETIO",     VID_GetIOWindows,          "<ViewportHndl> Get IO windows size & position");
    RetErr |= STTST_RegisterCommand( "VID_GETIN",     VID_GetInputWindowMode,    "<ViewportHndl> Get input window mode");
    RetErr |= STTST_RegisterCommand( "VID_GETOUT",    VID_GetOutputWindowMode,   "<ViewportHndl> Get output window parameters");
    RetErr |= STTST_RegisterCommand( "VID_GETPALLOCP",VID_GetPictureAllocParams, "<Handle> Get params to allocate space to picture");
    RetErr |= STTST_RegisterCommand( "VID_GETPPARAM", VID_GetPictureParams,      "<Handle> Get picture parameters");
    RetErr |= STTST_RegisterCommand( "VID_GETMEMPROF",VID_GetMemoryProfile,      "<Handle> Get memory profile");
#ifdef STVID_WRAPPER_OLDARCH
    RetErr |= STTST_RegisterCommand( "VID_GETMIX",    VID_GetMixWeight,    "<Handle> Get the blending factor");
    RetErr |= STTST_RegisterCommand( "VID_GETOFFSET", VID_GetScreenOffset, "<Handle> Get screen offset");
#endif
    RetErr |= STTST_RegisterCommand( "VID_GETRATIO",  VID_GetDisplayARConversion,"<ViewportHndl> Get pel aspect ratio conversion");
    RetErr |= STTST_RegisterCommand( "VID_GETSPEED",  VID_GetSpeed,              "<Handle> Get the speed");
#ifdef STVID_WRAPPER_OLDARCH
    RetErr |= STTST_RegisterCommand( "VID_GETSPARAM", VID_GetScreenParams,       "<Handle> Get screen parameters");
#endif
    RetErr |= STTST_RegisterCommand( "VID_HIDE",      VID_HidePicture,           "<ViewportHndl> Restore the display");
    RetErr |= STTST_RegisterCommand( "VID_INIT",      VID_Init,                  "<DeviceName><Type>[<vid/inp>][<vin>]<evt><clkrv><MaxHandle> Initialization");
    RetErr |= STTST_RegisterCommand( "VID_NODISP",    VID_DisableOutputWindow,   "<ViewportHndl> Disable the display of the video layer");
    RetErr |= STTST_RegisterCommand( "VID_NOSYNC",    VID_DisableSynchronisation,"<Handle> Disable the video synchronisation with PCR");
    RetErr |= STTST_RegisterCommand( "VID_OPEN",      VID_Open,                  "<DeviceName><SyncDelay> Share physical decoder usage");
    RetErr |= STTST_RegisterCommand( "VID_OPENVP",    VID_OpenViewPort,          "[<Handle>]<LayerName> Open a viewport");
    RetErr |= STTST_RegisterCommand( "VID_PAUSE",     VID_Pause,                 "<Handle> Pause the decoding");
    RetErr |= STTST_RegisterCommand( "VID_PAUSESYNC", VID_PauseSynchro,          "[<Handle>]<Time> Pause the decoding for synchro. purpose");
    RetErr |= STTST_RegisterCommand( "VID_RESUME",    VID_Resume,                "<Handle> Resume a previously paused decoding");
    RetErr |= STTST_RegisterCommand( "VID_REV",       VID_GetRevision,           "Get driver revision number");
    RetErr |= STTST_RegisterCommand( "VID_SETDECP",   VID_SetDecodedPictures,    "<Handle><Type> Set decoded pictures type");
    RetErr |= STTST_RegisterCommand( "VID_SETERR",    VID_SetErrorRecoveryMode,  "[<Handle>]<Mode> Set behavior regarding errors");
    RetErr |= STTST_RegisterCommand( "VID_SETIN",     VID_SetInputWindowMode,    "[<ViewportHndl>]<AutoMode><Align> Set input window mode");
    RetErr |= STTST_RegisterCommand( "VID_SETIO",     VID_SetIOWindows,
			  "[<ViewportHndl>]<InX><InY><InWidth><InHeight><OutX><OutY>\n\t\t<OutWidth><OutHeight> Set IO window size & position" );
    RetErr |= STTST_RegisterCommand( "VID_SETMEMPROF", VID_SetMemoryProfile,
              "[<Handle>]<MawWidth><MaxHeight><NbFrameStore><Compression>\n\t\t<Decimation> Set memory profile");
#ifdef STVID_WRAPPER_OLDARCH
    RetErr |= STTST_RegisterCommand( "VID_SETMIX",    VID_SetMixWeight,          "[<Handle>]<MixWeight> Set the blending factor");
#endif
    RetErr |= STTST_RegisterCommand( "VID_SETOUT",    VID_SetOutputWindowMode,   "[<ViewportHndl>]<Auto><Align> Set output window parameters");
    RetErr |= STTST_RegisterCommand( "VID_SETRATIO",  VID_SetDisplayARConversion,
			  "[<ViewportHdnl>]<RatioCv> Set ratio conv. between stream and TV");
#ifdef STVID_WRAPPER_OLDARCH
    RetErr |= STTST_RegisterCommand( "VID_SETSPARAM",VID_SetScreenParams,
	    	  "[<Handle>]<Aspect><Width><Height><XOffset><YOffset><ScanType><FrameRate>\n\t\tSet screen parameters");
#endif
    RetErr |= STTST_RegisterCommand( "VID_SETSPEED",  VID_SetSpeed,              "[<Handle>]<Speed> Set the speed");
#ifdef STVID_WRAPPER_OLDARCH
    RetErr |= STTST_RegisterCommand( "VID_SHIFT",     VID_SetScreenOffset,       "[<Handle>]<Horizontal><Vertical> Shift the screen position");
#endif
    RetErr |= STTST_RegisterCommand( "VID_SHOW",      VID_ShowPicture,
			  "[<ViewportHndl>]<CodingMode><ColorType><MPEGFrame><Size>\n\t\t<Height><Width><Aspect><ScanType><TopFieldFirst><FrameRate>\n\t\t<Hours><Min><Sec><Frames><Interpolated><PTS><PTS33>\n\t\t<Interpolated> Display the specific picture");
    RetErr |= STTST_RegisterCommand( "VID_SKIPSYNC",  VID_SkipSynchro,           "[<Handle>]<Time> Skip pictures");
    RetErr |= STTST_RegisterCommand( "VID_START",     VID_Start,
			  "[<Handle>]<RealTime><UpdateDisplay><StreamType><StreamID>\n\t\t Start the decoding of input stream");
    RetErr |= STTST_RegisterCommand( "VID_STOP",      VID_Stop,
			  "[<Handle>]<StopMode><Field><Mode> Stop decoding the input stream");
    RetErr |= STTST_RegisterCommand( "VID_SYNC",      VID_EnableSynchronisation, "<Handle> Enable video synchro with PCR");
    RetErr |= STTST_RegisterCommand( "VID_TERM",      VID_Term,                  "<DeviceName><ForceTerminate> Terminate");

    /* constants */

    RetErr |= STTST_AssignInteger("PAL",  0, TRUE);
    RetErr |= STTST_AssignInteger("NTSC", 1, TRUE);
    RetErr |= STTST_AssignInteger("STTES",   STVID_STREAM_TYPE_ES, TRUE);
    RetErr |= STTST_AssignInteger("STTPES",  STVID_STREAM_TYPE_PES, TRUE);
    RetErr |= STTST_AssignInteger("STTMP1P", STVID_STREAM_TYPE_MPEG1_PACKET, TRUE);

    RetErr |= STTST_AssignInteger("AR169",   STVID_DISPLAY_ASPECT_RATIO_16TO9, TRUE);
    RetErr |= STTST_AssignInteger("AR43",    STVID_DISPLAY_ASPECT_RATIO_4TO3, TRUE);
    RetErr |= STTST_AssignInteger("AR2211",  STVID_DISPLAY_ASPECT_RATIO_221TO1, TRUE);
    RetErr |= STTST_AssignInteger("ARSQ",    STVID_DISPLAY_ASPECT_RATIO_SQUARE, TRUE);

    RetErr |= STTST_AssignInteger("ARCONV_PS", STVID_DISPLAY_AR_CONVERSION_PAN_SCAN, TRUE);
    RetErr |= STTST_AssignInteger("ARCONV_LB", STVID_DISPLAY_AR_CONVERSION_LETTER_BOX, TRUE);
    RetErr |= STTST_AssignInteger("ARCONV_CO", STVID_DISPLAY_AR_CONVERSION_COMBINED, TRUE);
    RetErr |= STTST_AssignInteger("ARCONV_IG", STVID_DISPLAY_AR_CONVERSION_IGNORE, TRUE);

    /* variables (returned values) */

    /*RetErr |= STTST_AssignInteger("MIN_SCALE_X", 0.5, TRUE);
    RetErr |= STTST_AssignInteger("MIN_SCALE_Y", 0.5, TRUE);
    RetErr |= STTST_AssignInteger("MAX_SCALE_X", 2, TRUE);
    RetErr |= STTST_AssignInteger("MAX_SCALE_Y", 2, TRUE);*/

    RetErr |= STTST_AssignInteger("RET_VAL1",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL2",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL3",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL4",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL5",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL6",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL7",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL8",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL9",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL10",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL11",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL12",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL13",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL14",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL15",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL16",  0, FALSE);
    RetErr |= STTST_AssignInteger("RET_VAL17",  0, FALSE);

    RetErr |= STTST_AssignInteger("HNDVID1",  0, FALSE); /* video handle */
    RetErr |= STTST_AssignInteger("HNDVID2",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVID3",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVID4",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVID5",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVPVID1",  0, FALSE); /* viewport handle */
    RetErr |= STTST_AssignInteger("HNDVPVID2",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVPVID3",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVPVID4",  0, FALSE);
    RetErr |= STTST_AssignInteger("HNDVPVID5",  0, FALSE);

    API_TestMode = TEST_MODE_MULTI_HANDLES ;

    return(RetErr ? FALSE : TRUE);

} /* end VID_InitCommand */


/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL VID_RegisterCmd()
{
    BOOL RetOk;

    RetOk = VID_InitCommand();
    if ( RetOk )
    {
        sprintf( VID_Msg, "VID_InitCommand()  \t: ok           %s\n", STVID_GetRevision() );
    }
    else
    {
        sprintf( VID_Msg, "VID_InitCommand()  \t: failed !\n");
    }
    STTST_Print((VID_Msg));

    return(RetOk);
} /* end VID_CmdStart */


/*#######################################################################*/

