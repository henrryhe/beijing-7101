/******************************************************************************
*   COPYRIGHT (C) STMicroelectronics 2004
*
*   File name   : LAY_test.c
*   Description : Macros used to test STLAYER_xxx functions
*                 of the Layer Driver
*	Note        : All functions return TRUE if error for CLILIB compatibility
*
*	Date          Modification                                    Initials
*	----          ------------                                    --------
*   27 Jul 2000   Creation                                        VV
*   27 Ju2 2002   Change all                                      MB
*
******************************************************************************/

/*###########################################################################*/
/*############################## INCLUDES FILE ##############################*/
/*###########################################################################*/

#include <stdio.h>
#include <string.h>
#ifdef ST_OS20
#include "debug.h"
#endif /* ST_OS20 */
#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include "os21debug.h"
#endif /* ST_OS21 */

#include "stddefs.h"
#include "stdevice.h"
#include "stsys.h"
#include "stcommon.h"

#if !defined (ST_OSLINUX)
	#include "stlite.h"
#endif
#include "sttbx.h"
#include "stos.h"
#include "testtool.h"
#include "api_cmd.h"
#include "startup.h"
#include "stlayer.h"

#if defined (DVD_SECURED_CHIP) && !defined(STLAYER_NO_STMES)
#include "stmes.h"
#endif


#include "../../src/graphics/hal/gamma/back_end.h"
#include "../../src/graphics/hal/gamma/hard_mng.h"
#include "../../src/graphics/hal/gamma/frontend.h"
#include "../../src/graphics/hal/stillpic/still_cm.h"

#include "clevt.h"
#include "lay_cmd.h"

/*###########################################################################*/
/*############################### DEFINITION ################################*/
/*###########################################################################*/

/* 7015, 7020 Variables ----------------------------------------------------- */
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105)\
 || defined(ST_7100) || defined(ST_5301) || defined(ST_7109) || defined(ST_7710) || defined(ST_5188) || defined(ST_5525)\
 || defined(ST_5107) || defined(ST_7200) || defined(ST_5162)

extern STGXOBJ_Bitmap_t BitmapUnknown;
#endif

/* All chips Variable ------------------------------------------------------- */

char                             LAY_Msg[800];        /* text for trace */
static STLAYER_ViewPortHandle_t         VPHandle;
static STGXOBJ_ColorKey_t               ColorKeyGen;
static STLAYER_ViewPortSource_t         Source1,Source2;
static STGXOBJ_Bitmap_t                 Bitmap1,Bitmap2;
static STGXOBJ_Palette_t                Palette;

/* --- Externals --- */
extern STAVMEM_PartitionHandle_t AvmemPartitionHandle;
extern ST_DeviceName_t           AVMEMDeviceName;


#if ((!defined(ST_7109)) && (!defined(ST_7100)))
extern ST_Partition_t *             SystemPartition_p;
#endif


extern ST_ErrorCode_t GUTIL_LoadBitmap ( char * filename,
        STGXOBJ_Bitmap_t *Bitmap_p, STGXOBJ_Palette_t *Palette_p );



/* Private Function prototypes ---------------------------------------------- */

BOOL Test_CmdStart(void);
void os20_main(void *ptr);

#if defined (DVD_SECURED_CHIP) && !defined(STLAYER_NO_STMES)

#define SYS_REGION_BASE_ADDR 0x05580200
#define REGION_SZ 0x800000

extern void LAY_Secure(void);
BOOL LAYER_STMESMode(U32 mode, char *Mode_p);
BOOL LAYER_STMESError(ST_ErrorCode_t Error, char *Error_p);
#endif




/*###########################################################################*/
/*########### STATIC FUNCTIONS : TESTTOOL USER INTERFACE ####################*/
/*###########          (in alphabetic order)             ####################*/
/*###########################################################################*/

/*-----------------------------------------------------------------------------
 * Function : LAYER_DisableBorderAlpha
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_DisableBorderAlpha (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';
    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 0x00000000, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_DisableBorderAlpha(%d): ", VPHandle);
        ErrCode = STLAYER_DisableBorderAlpha( VPHandle ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_DisableColorKey
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_DisableColorKey (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;

    UNUSED_PARAMETER(result_sym_p);
    LAY_Msg[0]='\0';

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 0x00000000, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_DisableColorKey(%d): ", VPHandle);
        ErrCode = STLAYER_DisableColorKey( VPHandle ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_EnableBorderAlpha
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_EnableBorderAlpha (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';


    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_EnableBorderAlpha(%d): ", VPHandle);
        ErrCode = STLAYER_EnableBorderAlpha ( VPHandle ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );
}
/*-----------------------------------------------------------------------------
 * Function : LAYER_EnableColorKey
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_EnableColorKey (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';


    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected handle" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_EnableColorKey(%d): ", VPHandle);
        ErrCode = STLAYER_EnableColorKey( VPHandle ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetCapability
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetCapability (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_Capability_t Capability;
    char DeviceName[80];
    char DefaultName[80];
    LAY_Msg[0]='\0';




	/* get device */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString( pars_p, DefaultName, DeviceName,
            sizeof(DeviceName) );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected DeviceName" );
    }

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_GetCapability(%d): ", VPHandle);
        ErrCode = STLAYER_GetCapability( DeviceName,&Capability ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
        STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
	    switch ( ErrCode )
	    {
	        case ST_NO_ERROR :
	            RetErr = FALSE;
                STTST_AssignInteger ( result_sym_p,
                        (S32)&Capability, FALSE);
                sprintf(LAY_Msg, "MultipleViewPorts:%d\n", Capability.MultipleViewPorts);
                sprintf(LAY_Msg, "%sHorizontalResizing:%d\n", LAY_Msg, Capability.HorizontalResizing);
                sprintf(LAY_Msg, "%sAlphaBorder:%d\n", LAY_Msg, Capability.AlphaBorder);
                sprintf(LAY_Msg, "%sGlobalAlpha:%d\n", LAY_Msg, Capability.GlobalAlpha);
                sprintf(LAY_Msg, "%sColorKeying:%d\n", LAY_Msg, Capability.ColorKeying);
                sprintf(LAY_Msg, "%sMultipleViewPortsOnScanLineCapable:%d\n", LAY_Msg, Capability.MultipleViewPortsOnScanLineCapable);
                sprintf(LAY_Msg, "%sFrameBufferDisplayLateny:%d\n", LAY_Msg, Capability.FrameBufferDisplayLatency);
                sprintf(LAY_Msg, "%sFrameBufferHoldTime:%d\n", LAY_Msg, Capability.FrameBufferHoldTime);
	        break;
        }
	}
    STTBX_Print((LAY_Msg));
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_GetInitAllocParams
 *            Get driver alloc parameters
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetInitAllocParams (parse_t *pars_p, char *result_sym_p)

{
        UNUSED_PARAMETER(result_sym_p);
        UNUSED_PARAMETER(pars_p);
        return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetRevision( parse_t *pars_p, char *result_sym_p )
{
ST_Revision_t RevisionNumber;

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
    RevisionNumber = STLAYER_GetRevision( );
    sprintf( LAY_Msg, "STLAYER_GetRevision() : revision=%s\n", RevisionNumber );
    STTBX_Print((  LAY_Msg ));
    return ( FALSE );

} /* end LAYER_GetRevision */

/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortAlpha
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/

static BOOL LAYER_GetViewPortAlpha (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_GlobalAlpha_t Alpha;
    S32 Lvar;
    char StrParams[80];
    LAY_Msg[0]='\0';

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "LAYER_GetViewPortAlpha(%d): ", VPHandle);
        ErrCode = STLAYER_GetViewPortAlpha ( VPHandle , &Alpha ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
        sprintf(StrParams, "%d %d",Alpha.A0,Alpha.A1);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
	}
    return ( API_EnableError ? RetErr : FALSE );

}

/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortColorKey
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortColorKey (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';
    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;



    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_GetViewPortColorKey(%d): ", VPHandle);
        ErrCode = STLAYER_GetViewPortColorKey ( VPHandle , &ColorKeyGen ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );

}

/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortGain
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortGain (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_GainParams_t Params;
    S32 Lvar;
    LAY_Msg[0]='\0';

    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_GetViewPortGain(%d): ", VPHandle);
        ErrCode = STLAYER_GetViewPortGain ( VPHandle , &Params ) ;
        RetErr = LAYER_TextError( ErrCode, LAY_Msg);
	}
    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortPosition
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortPosition (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar,PositionX,PositionY;
    LAY_Msg[0]='\0';



    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, VPHandle, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    if (!(RetErr))
    {
        RetErr = TRUE;
        sprintf(LAY_Msg, "STLAYER_DisableBorderAlpha(%d): ", VPHandle);
        ErrCode = STLAYER_GetViewPortPosition ( VPHandle , &PositionX , &PositionY) ;
	}
    return ( API_EnableError ? RetErr : FALSE );
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_GetViewPortSource
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_GetViewPortSource (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    /*STGXOBJ_Palette_t Palette;*/
    S32 Lvar;
    LAY_Msg[0]='\0';

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 0x00000000, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        Source1.Data.BitMap_p=&Bitmap1;
        Source1.Palette_p=&Palette;

        ErrCode = STLAYER_GetViewPortSource ( VPHandle , &Source1 ) ;
        STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
        Bitmap1 = *(Source1.Data.BitMap_p);
        Palette = *(Source1.Palette_p);
	    switch ( ErrCode )
	    {
	        case ST_NO_ERROR :
                STTST_AssignInteger ( result_sym_p, (S32)&Source1, FALSE);
	            RetErr = FALSE;
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_GetViewPortSource (), Feature not supported !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_GetViewPortSource (), Handle invalid !\n" );
	            break;
            default:
                API_ErrorCount++;
                sprintf( LAY_Msg, "%sSTLAYER_GetViewPortSource (), unexpected error [%X] !\n",
                        LAY_Msg, ErrCode );
	            break;
    	}
        STTBX_Print((  LAY_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );

}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortColorKey
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortColorKey (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Lvar;
    LAY_Msg[0]='\0';



    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 0x00000000, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;


    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STLAYER_SetViewPortColorKey ( VPHandle , &ColorKeyGen ) ;
        STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
	    switch ( ErrCode )
	    {
	        case ST_NO_ERROR :
	            RetErr = FALSE;
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortColorKey(), Feature not supported !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortColorKey(), Handle invalid !\n" );
	            break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortColorKey(), Bad Parameter !\n" );
	            break;
            default:
                API_ErrorCount++;
                sprintf( LAY_Msg, "%sSTLAYER_SetViewPortColorKey(), unexpected error [%X] !\n",
                        LAY_Msg, ErrCode );
	            break;
    	}
        STTBX_Print((  LAY_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );
}
/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortRec
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortRec(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    BOOL Rec;
    S32 Lvar;
    LAY_Msg[0]='\0';



    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 0x00000000, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    /* get the bool level */
    if (!(RetErr))
	{
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if (RetErr)
	    {
            tag_current_line( pars_p, "expected gain level" );
	    }
        Rec = (U8)Lvar;
    }


    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STLAYER_SetViewPortRecordable ( VPHandle ,Rec  ) ;
        STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
	    switch ( ErrCode )
	    {
	        case ST_NO_ERROR :
	            RetErr = FALSE;
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortRecordable(), Feature not supported !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortRecordable(), Handle invalid !\n" );
	            break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortRecordable(), Bad Parameter !\n" );
	            break;
            default:
                API_ErrorCount++;
                sprintf( LAY_Msg, "%sSTLAYER_SetViewPortRecordable(), unexpected error [%X] !\n",
                        LAY_Msg, ErrCode );
	            break;
    	}
        STTBX_Print((  LAY_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortGain
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortGain (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STLAYER_GainParams_t Params;
    S32 Lvar;
    LAY_Msg[0]='\0';



    UNUSED_PARAMETER(result_sym_p);

    /* get the ViewPort handle */
    RetErr = STTST_GetInteger( pars_p, 0x00000000, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ViewPort" );
    }
    VPHandle = (STLAYER_ViewPortHandle_t)Lvar;

    /* get the Black level */
    if (!(RetErr))
	{
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if (RetErr)
	    {
            tag_current_line( pars_p, "expected Black level" );
	    }
        Params.BlackLevel = (U8)Lvar;
    }
    /* get the gain level */
    if (!(RetErr))
	{
        RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
        if (RetErr)
	    {
            tag_current_line( pars_p, "expected gain level" );
	    }
        Params.GainLevel = (U8)Lvar;
    }


    if (!(RetErr))
    {
        RetErr = TRUE;
        ErrCode = STLAYER_SetViewPortGain ( VPHandle , &Params ) ;
        STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
	    switch ( ErrCode )
	    {
	        case ST_NO_ERROR :
	            RetErr = FALSE;
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortGain(), Feature not supported !\n" );
                break;
            case ST_ERROR_INVALID_HANDLE :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortGain(), Handle invalid !\n" );
	            break;
            case ST_ERROR_BAD_PARAMETER :
                API_ErrorCount++;
                strcat( LAY_Msg, "STLAYER_SetViewPortGain(), Bad Parameter !\n" );
	            break;
            default:
                API_ErrorCount++;
                sprintf( LAY_Msg, "%sSTLAYER_SetViewPortGain(), unexpected error [%X] !\n",
                        LAY_Msg, ErrCode );
	            break;
    	}
        STTBX_Print((  LAY_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );
}


#if defined (DVD_SECURED_CHIP) && !defined(STLAYER_NO_STMES)


/*-----------------------------------------------------------------------------
 * Function : LAYER_STMESError
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL LAYER_STMESError(ST_ErrorCode_t Error, char *Error_p)
{
    BOOL RetErr = FALSE;

    if (Error != ST_NO_ERROR)
    {
        RetErr = TRUE;
        API_ErrorCount++;
    }
    RetErr = TRUE;
    switch ( Error )
    {
    case 0 :
        strcpy(Error_p,"ST_NO_ERROR");
        break;
    case STMES_ERROR_ALREADY_INITIALISED :
        strcpy(Error_p,"STMES_ERROR_ALREADY_INITIALISED");
        break;
    case STMES_ERROR_NOT_INITIALISED :
        strcpy(Error_p,"STMES_ERROR_NOT_INITIALISED");
        break;
    case STMES_ERROR_ADDR_RANGE :
        strcpy(Error_p,"STMES_ERROR_ADDR_RANGE");
        break;
    case STMES_ERROR_ADDR_ALIGN :
        strcpy(Error_p,"STMES_ERROR_ADDR_ALIGN");
        break;
    case STMES_ERROR_MES_INCORRECT :
        strcpy(Error_p,"STMES_ERROR_MES_INCORRECT");
        break;
    case STMES_ERROR_SID_INCORRECT :
        strcpy(Error_p,"STMES_ERROR_SID_INCORRECT");
        break;
    case STMES_ERROR_NO_REGIONS :
        strcpy(Error_p,"STMES_ERROR_NO_REGIONS");
        break;
    case STMES_ERROR_BUSY :
        strcpy(Error_p,"STMES_ERROR_BUSY");
        break;
    default:
        strcpy(Error_p,"Undefied Error Description");
        break;
    }

    return( API_EnableError ? RetErr : FALSE);
}


/*--------------------------------------------------------------
 * Function : LAYER_STMESMode
 *
 * Input    : ST_ErrorCode_t, char *
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL LAYER_STMESMode(U32 mode, char *Mode_p)
{
    BOOL RetErr = FALSE;

    RetErr = TRUE;
    switch ( mode )
        {
    case ACCESS_ALL :
        strcat(Mode_p," ACCESS_ALL ");
        break;
    case ACCESS_NO_STORE_INSECURE :
        strcat(Mode_p," ACCESS_NO_STORE_INSECURE ");
        break;
    case ACCESS_NO_SECURE :
        strcat(Mode_p," ACCESS_NO_SECURE ");
        break;
    case ACCESS_LOAD_SECURE_ONLY :
        strcat(Mode_p,"ACCESS_LOAD_SECURE_ONLY ");
        break;
    default:
        strcat(Mode_p," Undefied Access Mode ");
    break;
        }

    return( API_EnableError ? RetErr : FALSE);
}



/*****************************************************************************/

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetMESMode1
 *             CPU is configured to access to SECURE Memory Region
 *             and  LMI Sys is configured as Secured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int LAYER_SetMESMode1 (parse_t *pars_p, char *result_sym_p)
{

    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    unsigned int uMemoryStatus;
    char       Text[50];


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    LAYER_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");


     RetErr=STMES_SetCPUAccess(SID_SH4_CPU,ACCESS_ALL);

     LAYER_STMESError(RetErr,Text);
     printf(" STMES_SetCPUAccess = %s \n",Text);
     strcpy(Text,"\0");

     uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );
     LAYER_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");


    uMemoryStatus = STMES_SetSecureRange((void *)SYS_REGION_BASE_ADDR,
          (void *)(SYS_REGION_BASE_ADDR + REGION_SZ - 1), 0);
    LAYER_STMESMode(uMemoryStatus,Text);

    if( uMemoryStatus != ST_NO_ERROR )
    {
        printf(" Error for STMES_SetSecureRange(0x%08x,0x%08x,N/A) : %s\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1),Text);
    }
    else
    {
        printf(" Secure Region Created : Start Addr= 0x%08x : End Addr= 0x%08x\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1) );
    }

   strcpy(Text,"\0");

    return 0;
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetMESMode2
 *             CPU is configured to access to SECURE Memory Region
 *             and  LMI Sys is configured as Insecured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int LAYER_SetMESMode2 (parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    char       Text[50];

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    LAYER_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");

    RetErr=STMES_SetCPUAccess(SID_SH4_CPU,ACCESS_ALL);

     LAYER_STMESError(RetErr,Text);
     printf(" STMES_SetCPUAccess = %s \n",Text);
     strcpy(Text,"\0");


    uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

    LAYER_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");

    return (0);
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetMESMode3
 *             CPU is configured to NO access to SECURE Memory Region
 *             and  LMI Sys is configured as Secured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int LAYER_SetMESMode3 (parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    unsigned int uMemoryStatus;
    char       Text[50];

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    LAYER_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");


    uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

    LAYER_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");


    uMemoryStatus = STMES_SetSecureRange((void *)SYS_REGION_BASE_ADDR,
          (void *)(SYS_REGION_BASE_ADDR + REGION_SZ - 1), 0);
    LAYER_STMESMode(uMemoryStatus,Text);

    if( uMemoryStatus != ST_NO_ERROR )
    {
        printf(" Error for STMES_SetSecureRange(0x%08x,0x%08x,N/A) : %s\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1),Text);
    }
    else
    {
        printf(" Secure Region Created : Start Addr= 0x%08x : End Addr= 0x%08x\n",
        SYS_REGION_BASE_ADDR, (SYS_REGION_BASE_ADDR + REGION_SZ - 1) );
    }

    strcpy(Text,"\0");

    return (0);
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_SetMESMode4
 *             CPU is configured to NO access to SECURE Memory Region
 *             and  LMI Sys is configured as Insecured region
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int LAYER_SetMESMode4 (parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t RetErr;
    STMES_InitParams_t Init;
    unsigned int uCPU_Mode;
    char       Text[50];


    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);


    /* get the driver version this can be done before the driver is Initialised. */
    printf("STMES Driver Version= %s\n", STMES_GetVersion());
    /* initialise the driver leaving all interrutps alone*/
    memset(&Init,0,sizeof(STMES_InitParams_t));
    if((RetErr= STMES_Init(&Init)) != ST_NO_ERROR)
    {
        printf(" MES Init Failed  :\n");
        return(FALSE);
    }

    LAYER_STMESError(RetErr,Text);

    printf(" STMES_Init = %s \n",Text);
    strcpy(Text,"\0");


    uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );

    LAYER_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");

 return (FALSE);

}

/*-----------------------------------------------------------------------------
 * Function : LAYER_TermMES
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static int LAYER_TermMES (parse_t *pars_p, char *result_sym_p)
{
     ST_ErrorCode_t RetErr;
     char       Text[50];
     unsigned int uCPU_Mode;



    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

     RetErr=STMES_SetCPUAccess(SID_SH4_CPU,ACCESS_NO_SECURE);

     LAYER_STMESError(RetErr,Text);
     printf(" STMES_SetCPUAccess = %s \n",Text);
     strcpy(Text,"\0");

     uCPU_Mode= STMES_GetSourceMode( SID_SH4_CPU, SYS_MES );
     LAYER_STMESMode(uCPU_Mode,Text);

    printf(" Mesmory Mode is %s \n",Text);
    strcpy(Text,"\0");

    /* finally terminate the driver as finished with it */
    if((RetErr= STMES_Term()) != ST_NO_ERROR)
    {
    printf(" MES Term Failed  :\n");
    return(FALSE);
    }

    LAYER_STMESError(RetErr,Text);

    printf(" STMES_Term = %s \n",Text);
    strcpy(Text,"\0");

    return (FALSE);
}







#endif







/*-----------------------------------------------------------------------------
 * Function : LAYER_SetViewPortPSI
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetViewPortPSI (parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
    return (FALSE);
}
/*-----------------------------------------------------------------------------
 * Function : LAYER_MakeSourceCLUT8
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_MakeSourceCLUT8 (parse_t *pars_p, char *result_sym_p)
{

char      PictureName[80];


#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_5100) || defined (ST_5105)\
    || defined (ST_7710) || defined (ST_7100) || defined (ST_5301) || defined (ST_7109) || defined(ST_5188) || defined(ST_5525)\
    || defined(ST_5107) || defined(ST_5162)
    LAY_Msg[0]='\0';

    strcpy(PictureName,"../../../scripts/face_128x128.gam");

#ifdef ST_OSLINUX
    strcpy(PictureName,"./scripts/layer/face_128x128.gam");
    GUTIL_LoadBitmap(PictureName,&Bitmap2,&Palette);
#else
    GUTIL_LoadBitmap(PictureName,&Bitmap2,&Palette);
#endif
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    Source2.SourceType    = STLAYER_GRAPHIC_BITMAP;
    Source2.Data.BitMap_p = &Bitmap2;
    Source2.Palette_p     = &Palette;
#else
    LAY_Msg[0]='\0';
    strcat(LAY_Msg,"Command not supported in this configuration !!!\n");
#endif
    STTBX_Print((  LAY_Msg ));
    return (FALSE);
}
/*-----------------------------------------------------------------------------
 * Function : LAYER_MakeSourceAlpha1
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_MakeSourceAlpha1 (parse_t *pars_p, char *result_sym_p)
{

    char      PictureName[80];

    LAY_Msg[0]='\0';

     strcpy(PictureName,"../../../scripts/mask1.gam");
    UNUSED_PARAMETER(pars_p);
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528)|| defined(ST_7710) || defined (ST_7100) || defined (ST_7109) \
        || defined(ST_7200)
#ifdef ST_OSLINUX
    strcpy(PictureName,"./files/mask1.gam");
    GUTIL_LoadBitmap(PictureName,&BitmapUnknown,&Palette);
#else
    GUTIL_LoadBitmap(PictureName,&BitmapUnknown,&Palette);
#endif
#else
    strcat(LAY_Msg,"Command not supported in this configuration !!!\n");
#endif
    UNUSED_PARAMETER(result_sym_p);
    STTBX_Print((  LAY_Msg ));
    return (FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : LAYER_MakeSourceRGB565
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_MakeSourceRGB565 (parse_t *pars_p, char *result_sym_p)
{
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528) || defined (ST_5100) || defined (ST_5105)\
 || defined (ST_7710) || defined (ST_7100) || defined (ST_5301) || defined (ST_7109) || defined(ST_5188) || defined(ST_5525)\
 || defined (ST_5107) || defined (ST_7200) || defined (ST_5162)
    S32 Lvar;
    BOOL RetErr = FALSE;


    char      PictureName[80];


    LAY_Msg[0]='\0';

    strcpy(PictureName,"../../../scripts/merou3.gam");


    /* get the flag */
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected flag : (0) no allocation,\
 (1) dump of merou.gam, (2) dump of suzie.gam" );
    }

    if ( Lvar==0 )
    {
        BitmapUnknown.ColorType = STGXOBJ_COLOR_TYPE_RGB565;
        BitmapUnknown.ColorSpaceConversion = 0;
        BitmapUnknown.Width = 500;
        BitmapUnknown.Height = 500;
        BitmapUnknown.Data1_p = (U32*)((U32)(0x000)+0x3000);
        BitmapUnknown.Pitch  = 0x2c0;  /* used in simu */
        BitmapUnknown.BitmapType = STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
        STTST_AssignInteger ( result_sym_p, (S32)&Source1, FALSE);
    }
    else if ( Lvar==1 )
    {
#ifdef ST_OSLINUX
        strcpy(PictureName,"./files/merou3.gam");
        GUTIL_LoadBitmap(PictureName,&BitmapUnknown,
                        &Palette);
#else
                GUTIL_LoadBitmap(PictureName,&BitmapUnknown,
                        &Palette);
#endif
    }
    else
    {
#ifdef ST_OSLINUX
        strcpy(PictureName,"./files/suzie.gam");
        GUTIL_LoadBitmap(PictureName,&BitmapUnknown,
                &Palette );
#else
        strcpy(PictureName,"../../../scripts/suzie.gam");
        GUTIL_LoadBitmap(PictureName,&BitmapUnknown,
                        &Palette);
#endif
    }

#else
    LAY_Msg[0]='\0';
    strcat(LAY_Msg,"Command not supported in this configuration !!!\n");
#endif

    STTBX_Print((  LAY_Msg ));
    return (FALSE);
}

/*-----------------------------------------------------------------------------
 * Function : LAYER_ReadColorKey
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_ReadColorKey (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    char ColorKeyType[80];

    LAY_Msg[0]='\0';
    RetErr = FALSE;
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    switch ( ColorKeyGen.Type )
    {
        case STGXOBJ_COLOR_KEY_TYPE_CLUT8 :
            sprintf (ColorKeyType, "CLUT8");
            sprintf (LAY_Msg,"%sColorKeyType: %s\nPalette Min: %d\tPalette Max: %d\tPalette Out: %d\tPalette Enable: %d\n",
            LAY_Msg,ColorKeyType,
            ColorKeyGen.Value.CLUT8.PaletteEntryMin,
            ColorKeyGen.Value.CLUT8.PaletteEntryMax,
            ColorKeyGen.Value.CLUT8.PaletteEntryOut,
            ColorKeyGen.Value.CLUT8.PaletteEntryEnable);
            break;
        case STGXOBJ_COLOR_KEY_TYPE_RGB888 :
            sprintf (ColorKeyType, "RGB888");
            sprintf (LAY_Msg,"%sColorKeyType: %s\nRMin: %d\tRMax: %d\tROut: %d\tREnable: %d\nGMin: %d\tGMax: %d\tGOut: %d\tGEnable: %d\nBMin: %d\tBMax: %d\tBOut: %d\tBEnable: %d\n",
            LAY_Msg,ColorKeyType,
            ColorKeyGen.Value.RGB888.RMin,
            ColorKeyGen.Value.RGB888.RMax,
            ColorKeyGen.Value.RGB888.ROut,
            ColorKeyGen.Value.RGB888.REnable,
            ColorKeyGen.Value.RGB888.GMin,
            ColorKeyGen.Value.RGB888.GMax,
            ColorKeyGen.Value.RGB888.GOut,
            ColorKeyGen.Value.RGB888.GEnable,
            ColorKeyGen.Value.RGB888.BMin,
            ColorKeyGen.Value.RGB888.BMax,
            ColorKeyGen.Value.RGB888.BOut,
            ColorKeyGen.Value.RGB888.BEnable);
            break;
        case STGXOBJ_COLOR_KEY_TYPE_YCbCr888_SIGNED :
            sprintf (ColorKeyType, "S YCbCr");
            sprintf (LAY_Msg,"%sColorKeyType: %s\nYMin: %d\tYMax: %d\tYOut: %d\tYEnable: %d\nCbMin: %d\tCbMax: %d\tCbOut: %d\tCbEnable: %d\nCrMin: %d\tCrMax: %d\tCrOut: %d\tCrEnable: %d\n",
            LAY_Msg,ColorKeyType,
            ColorKeyGen.Value.SignedYCbCr888.YMin,
            ColorKeyGen.Value.SignedYCbCr888.YMax,
            ColorKeyGen.Value.SignedYCbCr888.YOut,
            ColorKeyGen.Value.SignedYCbCr888.YEnable,
            ColorKeyGen.Value.SignedYCbCr888.CbMin,
            ColorKeyGen.Value.SignedYCbCr888.CbMax,
            ColorKeyGen.Value.SignedYCbCr888.CbOut,
            ColorKeyGen.Value.SignedYCbCr888.CbEnable,
            ColorKeyGen.Value.SignedYCbCr888.CrMin,
            ColorKeyGen.Value.SignedYCbCr888.CrMax,
            ColorKeyGen.Value.SignedYCbCr888.CrOut,
            ColorKeyGen.Value.SignedYCbCr888.CrEnable);
            break;
        case STGXOBJ_COLOR_KEY_TYPE_YCbCr888_UNSIGNED :
            sprintf (ColorKeyType, "US YCbCr");
            sprintf (LAY_Msg,"%sColorKeyType: %s\nYMin: %d\tYMax: %d\tYOut: %d\tYEnable: %d\nCbMin: %d\tCbMax: %d\tCbOut: %d\tCbEnable: %d\nCrMin: %d\tCrMax: %d\tCrOut: %d\tCrEnable: %d\n",
            LAY_Msg,ColorKeyType,
            ColorKeyGen.Value.UnsignedYCbCr888.YMin,
            ColorKeyGen.Value.UnsignedYCbCr888.YMax,
            ColorKeyGen.Value.UnsignedYCbCr888.YOut,
            ColorKeyGen.Value.UnsignedYCbCr888.YEnable,
            ColorKeyGen.Value.UnsignedYCbCr888.CbMin,
            ColorKeyGen.Value.UnsignedYCbCr888.CbMax,
            ColorKeyGen.Value.UnsignedYCbCr888.CbOut,
            ColorKeyGen.Value.UnsignedYCbCr888.CbEnable,
            ColorKeyGen.Value.UnsignedYCbCr888.CrMin,
            ColorKeyGen.Value.UnsignedYCbCr888.CrMax,
            ColorKeyGen.Value.UnsignedYCbCr888.CrOut,
            ColorKeyGen.Value.UnsignedYCbCr888.CrEnable);
            break;
        default:
           break;

    }
    STTBX_Print((  LAY_Msg ));
    return ( FALSE );
}
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528)|| defined(ST_7710) || defined (ST_7100) || defined (ST_7109) \
        || defined(ST_7200)
static BOOL LAYER_LinkedLists (parse_t *pars_p, char *result_sym_p)
{
#if !defined(ST_OSLINUX)
    S32                             Lvar;
    BOOL                            RetErr;
    stlayer_XLayerContext_t         XLayerContext;
    stlayer_ViewPortDescriptor_t *  CurrentVP;
    U32                             i,j,k;
    stlayer_Node_t *                CurrentNode_p;
    RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "Expected num context" );
    }
    UNUSED_PARAMETER(result_sym_p);
    XLayerContext = stlayer_context.XContext[(U32)Lvar];
    printf(" Layer soft context Info ...\n");
    if (!(XLayerContext.Initialised))
    {
        printf(" Context non initialized \n");
        return( FALSE );
    }
    printf("    Layer type is  : %i\n",XLayerContext.LayerType);
    printf("    Layer num open : %i\n",XLayerContext.Open);
    printf("    Layer max vp   : %i\n",XLayerContext.MaxViewPorts);
    printf("    Layer name     : %s\n",XLayerContext.DeviceName);
    printf("    Layer W        : %i\n",XLayerContext.LayerParams.Width);
    printf("    Layer H        : %i\n",XLayerContext.LayerParams.Height);
    printf("    Layer add      : 0x%p\n",XLayerContext.BaseAddress);
    printf("    Layer sync     : %s\n",XLayerContext.VTGName);
    printf("    num ena vp     : %i\n",XLayerContext.NumberLinkedViewPorts);
    printf("\n");

    printf(" Viewports description info ...\n");
    if(XLayerContext.NumberLinkedViewPorts == 0)
    {
        printf(" No linked viewport ...\n");
        printf("\n");
    }
    else
    {
        CurrentVP = XLayerContext.LinkedViewPorts;
        i = 1;
        do
        {
            printf("     |  \n");
            printf("     V  \n");
            printf("    VP %i : add VP       : 0x%p\n", i,
                                                   (void*)CurrentVP);
            printf("    VP %i : add Top Node : 0x%p\n", i,
                                                   (void*)CurrentVP->TopNode_p);
            printf("    VP %i : add Bot Node : 0x%p\n", i,
                                                   (void*)CurrentVP->BotNode_p);
            printf("    VP %i : add next VP  : 0x%p\n", i,
                                                   (void*)CurrentVP->Next_p);
            printf("    VP %i : add prev VP  : 0x%p\n", i,
                                                   (void*)CurrentVP->Prev_p);
            CurrentVP = CurrentVP->Next_p;
            i ++;
        }while((i <= XLayerContext.NumberLinkedViewPorts)
                && (CurrentVP != XLayerContext.LinkedViewPorts));
        printf("\n");
    }
    printf(" Linked list in memory Info ...\n");
    CurrentNode_p = XLayerContext.TmpTopNode_p;
    i = 1;
    do
    {
        printf("     |  \n");
        printf("     V  \n");
        printf("    Node %i : add Node       : 0x%p\n", i,
                                            (void*)CurrentNode_p );
        printf("    Node %i : add Next       : 0x%p\n", i,
                                            (void*)CurrentNode_p->BklNode.NVN );
        CurrentVP = XLayerContext.LinkedViewPorts;
        k = 1;
        do
        {
            if(CurrentVP->TopNode_p == CurrentNode_p)
            {
                printf("    Node matches VP %i -> Top Node\n", k);
            }
            if(CurrentVP->BotNode_p == CurrentNode_p)
            {
                printf("    Node matches VP %i -> Bot Node\n", k);
            }
            CurrentVP = CurrentVP->Next_p;
            k ++;
        }while((k <= XLayerContext.NumberLinkedViewPorts)
                && (CurrentVP != XLayerContext.LinkedViewPorts));

        if(XLayerContext.TmpTopNode_p == CurrentNode_p)
        {
            printf("    Node matches dummy Top Node\n");
        }
        if(XLayerContext.TmpBotNode_p == CurrentNode_p)
        {
            printf("    Node matches dummy Bot Node\n");
        }
        if ((CurrentNode_p->BklNode.CTL & 0x80000000) == 0x80000000)
        {
            printf("    Node %i : last node for this field\n", i);
        }
        i ++;
        j = CurrentNode_p->BklNode.NVN;
        j = j + (U32)XLayerContext.VirtualMapping.PhysicalAddressSeenFromCPU_p;
        j =j -(U32)XLayerContext.VirtualMapping.PhysicalAddressSeenFromDevice_p;
        CurrentNode_p = (stlayer_Node_t*)j;
    }while((i <= (2*XLayerContext.NumberLinkedViewPorts+2))
                && (CurrentNode_p != XLayerContext.TmpTopNode_p));

    printf("\n");


#endif

#if defined(ST_OSLINUX)
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

#endif

    return( FALSE );
}
#endif
/*-----------------------------------------------------------------------------
 * Function : LAYER_SetColorKey
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL LAYER_SetColorKey (parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    int ColorKey;
    S32 Lvar;

    LAY_Msg[0]='\0';
    RetErr = FALSE;
    UNUSED_PARAMETER(result_sym_p);

    ColorKeyGen.Type                           = STGXOBJ_COLOR_KEY_TYPE_RGB888;
    ColorKeyGen.Value.RGB888.RMin             = 0;
    ColorKeyGen.Value.RGB888.RMax             = 0;
    ColorKeyGen.Value.RGB888.ROut             = 0;
    ColorKeyGen.Value.RGB888.REnable          = 0;
    ColorKeyGen.Value.RGB888.RMin             = 0;
    ColorKeyGen.Value.RGB888.RMax             = 0;
    ColorKeyGen.Value.RGB888.ROut             = 0;
    ColorKeyGen.Value.RGB888.REnable          = 0;
    ColorKeyGen.Value.RGB888.RMin             = 0;
    ColorKeyGen.Value.RGB888.RMax             = 0;
    ColorKeyGen.Value.RGB888.ROut             = 0;
    ColorKeyGen.Value.RGB888.REnable          = 0;

    /* get the ColorKey */
    RetErr = STTST_GetInteger( pars_p, 1, &Lvar );
    if (RetErr)
    {
        tag_current_line( pars_p, "expected ColorKeyType CLUT:1, RGB:2,\
                Signed YCbCr:3, Unsigned YCbCr:4" );
    }
    ColorKey = (int)Lvar;

    switch ( ColorKey )
    {
        case 1 :
            ColorKeyGen.Type = STGXOBJ_COLOR_KEY_TYPE_CLUT8;

            /* get the Palette Entry Min */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Palette Entry Min" );
                }
                ColorKeyGen.Value.CLUT8.PaletteEntryMin = (U8)Lvar;
            }


            /* get the PaletteEntryMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected PaletteEntryMax" );
                }
                ColorKeyGen.Value.CLUT8.PaletteEntryMax = (U8)Lvar;
            }


            /* get the PaletteEntryout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected PaletteEntryout" );
                }
                ColorKeyGen.Value.CLUT8.PaletteEntryOut = (BOOL)Lvar;
            }


            /* get the PaletteEntryEnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected PaletteEntryEnable" );
                }
                ColorKeyGen.Value.CLUT8.PaletteEntryEnable = (BOOL)Lvar;
            }

            break;


        case 2 :
            ColorKeyGen.Type = STGXOBJ_COLOR_KEY_TYPE_RGB888;


            /* get the RMin */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected RMin" );
                }
                ColorKeyGen.Value.RGB888.RMin = (U8)Lvar;
            }


           /* get the RMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected RMax" );
                }
                ColorKeyGen.Value.RGB888.RMax = (U8)Lvar;
            }


            /* get the Rout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Rout" );
                }
                ColorKeyGen.Value.RGB888.ROut = (BOOL)Lvar;
            }


            /* get the REnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected REnable" );
                }
                ColorKeyGen.Value.RGB888.REnable = (BOOL)Lvar;
            }


            /* get the GMin */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected GMin" );
                }
                ColorKeyGen.Value.RGB888.GMin = (U8)Lvar;
            }


            /* get the GMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected GMax" );
                }
                ColorKeyGen.Value.RGB888.GMax = (U8)Lvar;
            }


            /* get the Gout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Gout" );
                }
                ColorKeyGen.Value.RGB888.GOut = (BOOL)Lvar;
            }


            /* get the GEnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected GEnable" );
                }
                ColorKeyGen.Value.RGB888.GEnable = (BOOL)Lvar;
            }


            /* get the BMin */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected BMin" );
                }
                ColorKeyGen.Value.RGB888.BMin = (U8)Lvar;
            }


            /* get the BMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected BMax" );
                }
                ColorKeyGen.Value.RGB888.BMax = (U8)Lvar;
            }


            /* get the Bout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Bout" );
                }
                ColorKeyGen.Value.RGB888.BOut = (BOOL)Lvar;
            }


            /* get the BEnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected BEnable" );
                }
                ColorKeyGen.Value.RGB888.BEnable = (BOOL)Lvar;
            }

            break;


        case 3 | 4:
            if ( ColorKey == 3 )
            {
                ColorKeyGen.Type = STGXOBJ_COLOR_KEY_TYPE_YCbCr888_SIGNED;
            }
            else
            {
                ColorKeyGen.Type = STGXOBJ_COLOR_KEY_TYPE_YCbCr888_UNSIGNED;
            }


            /* get the YMin */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected YMin" );
                }
                ColorKeyGen.Value.SignedYCbCr888.YMin = (U8)Lvar;
            }


            /* get the YMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected YMax" );
                }
                ColorKeyGen.Value.SignedYCbCr888.YMax = (U8)Lvar;
            }


            /* get the Yout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Yout" );
                }
                ColorKeyGen.Value.SignedYCbCr888.YOut = (BOOL)Lvar;
            }


            /* get the YEnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected YEnable" );
                }
                ColorKeyGen.Value.SignedYCbCr888.YEnable = (BOOL)Lvar;
            }


            /* get the CbMin */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected CbMin" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CbMin = (U8)Lvar;
            }


            /* get the CbMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected CbMax" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CbMax = (U8)Lvar;
            }


            /* get the Cbout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Cbout" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CbOut = (BOOL)Lvar;
            }


            /* get the CbEnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected CbEnable" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CbEnable = (BOOL)Lvar;
            }


            /* get the CrMin */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected CrMin" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CrMin = (U8)Lvar;
            }


            /* get the CrMax */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected CrMax" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CrMax = (U8)Lvar;
            }


            /* get the Crout */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected Crout" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CrOut = (BOOL)Lvar;
            }


            /* get the CrEnable */
            if (!(RetErr))
            {
                RetErr = STTST_GetInteger( pars_p, 0, &Lvar );
                if (RetErr)
                {
                    tag_current_line( pars_p, "expected CrEnable" );
                }
                ColorKeyGen.Value.SignedYCbCr888.CrEnable = (BOOL)Lvar;
            }

            break;


        default:
            tag_current_line( pars_p, "ColorKeyType not found ! CLUT:1, RGB:2, Signed YCbCr:3, Unsigned YCbCr:4" );
            break;
    }

     if (!(RetErr))
	{
        STTBX_Print((  LAY_Msg ));
	}
    return ( API_EnableError ? RetErr : FALSE );
}

/*###########################################################################*/
/*############################# LAYER COMMANDS ##############################*/
/*###########################################################################*/

/*-----------------------------------------------------------------------------
 * Function : LAYER_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ---------------------------------------------------------------------------*/
static BOOL LAYER_InitCommand (void)
{
BOOL RetErr;

    /*  Command's name, link to C-function, help message     */
    /*  (by alphabetic order of macros = display order)     */
    RetErr = FALSE;

    RetErr |= register_command( "lay_disba",  LAYER_DisableBorderAlpha,
        "<ViewPort Handle>: Disable Border Alpha Function");
    RetErr |= register_command( "lay_disck",  LAYER_DisableColorKey,
        "<ViewPort Handle>: Disable Color Key Function");
    RetErr |= register_command( "lay_enba",  LAYER_EnableBorderAlpha,
        "<ViewPort Handle>: Enable Border Alpha Function");
    RetErr |= register_command( "lay_enck",  LAYER_EnableColorKey,
        "<ViewPort Handle>: Enable Color Key Function");
    RetErr |= register_command( "lay_get_capa",  LAYER_GetCapability,
        "<DeviceName>: Retrieves the driver capabilities");
    RetErr |= register_command( "lay_get_iallocp",  LAYER_GetInitAllocParams,
        "( command not achieved !)");
    RetErr |= register_command( "lay_get_rev",  LAYER_GetRevision,
        "Retrieves the driver revision");
    RetErr |= register_command( "lay_get_vpa",  LAYER_GetViewPortAlpha,
        "<ViewPort Handle>: Display the ViewPort Alpha 0 & ALPHA 1");
    RetErr |= register_command( "lay_get_vpck",  LAYER_GetViewPortColorKey,
        "<ViewPort Handle>: Display the ViewPort Color Key");
    RetErr |= register_command( "lay_get_vpg",  LAYER_GetViewPortGain,
        "<ViewPort Handle>: Display the ViewPort Blacklevel & Gainlevel");
    RetErr |= register_command( "lay_get_vppos",  LAYER_GetViewPortPosition,
        "<ViewPort Handle>: Display the current Position of the ViewPort");
    RetErr |= register_command( "lay_get_vpsrc",  LAYER_GetViewPortSource,
        "<ViewPort Handle>: Get the Source of the ViewPort");
    RetErr |= register_command( "lay_set_vpck",  LAYER_SetViewPortColorKey,
        "<ViewPort Handle><ColorKey>: Set the ViewPort Color Key");
    RetErr |= register_command( "lay_set_vpg",  LAYER_SetViewPortGain,
        "<ViewPort Handle><BlackLevel><GainLevel>: Set the ViewPort Gain");
    RetErr |= register_command( "lay_set_rec",  LAYER_SetViewPortRec,
        "<ViewPort Handle><1|0>: Set the ViewPort Recordable");
    RetErr |= register_command( "lay_set_vppsi",  LAYER_SetViewPortPSI,
        "( command not available ! ): Set the PSI of the ViewPort");
    RetErr |= register_command( "lay_mksrcc8",  LAYER_MakeSourceCLUT8,
        "Create a source ACLUT88");
    RetErr |= register_command( "lay_mksrcrgb565",  LAYER_MakeSourceRGB565,
        "Create a source RGB565");
    RetErr |= register_command( "lay_mksrca1",  LAYER_MakeSourceAlpha1,
        "Create a source Alpha1");
    RetErr |= register_command( "lay_rdck",  LAYER_ReadColorKey,
        "Display the parameters of the current Color Key");
    RetErr |= register_command( "lay_setck",  LAYER_SetColorKey,
        "<Color Key Type><Palette|R|Y min><Palette|R|Y max><Palette|R|Y Out>\
        <Palette|R|Y Enable>...: Set the currnet color key to the specified \
        values");
#if defined(ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_5528)|| defined(ST_7710) || defined (ST_7100) || defined (ST_7109) \
        || defined(ST_7200)
    RetErr |= register_command( "lay_list",  LAYER_LinkedLists,
        "Debug sofware context");
#endif

#ifdef STI7109_CUT2
    RetErr |= STTST_AssignString ("CHIP_CUT", "STI7109_CUT2", TRUE);
#else
    RetErr |= STTST_AssignString ("CHIP_CUT", "", TRUE);
#endif

#if defined (DVD_SECURED_CHIP)&& !defined(STLAYER_NO_STMES)
    RetErr |= STTST_RegisterCommand( "LAYER_SetMESMode1"  ,LAYER_SetMESMode1,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "LAYER_SetMESMode2"  ,LAYER_SetMESMode2,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "LAYER_SetMESMode3"  ,LAYER_SetMESMode3,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "LAYER_SetMESMode4"  ,LAYER_SetMESMode4,"Create partition  ");
    RetErr |= STTST_RegisterCommand( "LAYER_TermMES"  ,LAYER_TermMES,"Create partition  ");

#endif


    if (RetErr)
    {
        STTBX_Print(( "Layer_MacroInit() : macros registrations failure !\n" ));
    }
    else
    {
        STTBX_Print((  "Layer_MacroInit() : macros registrations ok\n" ));
    }
    return( RetErr );

} /* end LAYER_MacroInit */

/*#######################################################################*/
/*#########################  GLOBAL FUNCTIONS  ##########################*/
/*#######################################################################*/

BOOL Test_CmdStart(void)
{
    BOOL RetErr;

    RetErr = LAYER_InitCommand();

    return(RetErr ? FALSE : TRUE);

} /* end LAYER_CmdStart */



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
    STAPIGAT_Init();
    UNUSED_PARAMETER(ptr);
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
#ifdef ST_OSWINCE
	printf ("WINCE May 2005\n");
	SetFopenBasePath("/dvdgr-prj-stlayer/tests/src/objs/ST40");
	printf("Set Path /dvdgr-prj-stlayer/tests/src/objs/ST40\n");
#else
    setbuf(stdout, NULL);
#endif
#endif
    os20_main(NULL);

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);
    return(0);
}
/*###########################################################################*/
