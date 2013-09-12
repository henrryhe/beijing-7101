/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

File name   : evin_cmd.c
Description : EXTVIN macros

Note        : All functions return TRUE if error for CLILIB compatibility

Date          Modification                                    Initials
----          ------------                                    --------
16 Mar 2001   Creation                                        JG
05 May 2003   Add support of stv2310 on 7020 stem db577       CL
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>

/*#include "os20.h"*/
#include "stsys.h"
#include "stlite.h"
#include "sti2c.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stcommon.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#include "cli2c.h"
#include "stextvin.h"
#include "evin_cmd.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants --- */

/*********** Dauther board DB331 **********/
/* SAA7111 Base address */
#define SAA7111_I2C_BASE                0x48
/* SAA7114 Slave address */
#define SAA7114_I2C_BASE                0x42
#define TDA8752_BASE                    0x98

/*********** Dauther board DB577 **********/
/* STV2310 Slave address */
#define STV2310_I2C_BASE                0x86

#define DEFAULT_EXTVIN_NAME   "EXTVIN"   /* device name */

static const char TabAdcInput[4][13] = {
    "VGA_640_480 " ,
    "SVGA_800_600" ,
    "XGA_1024_768" ,
    "SUN_1152_900"
};
static const char TabVipStd[11][14] = {
    "ST_UNDEFINED " ,
    "ST_PAL_BGDHI " ,
    "ST_NTSC_M    " ,
    "ST_PAL4      " ,
    "ST_NTSC4_50  " ,
    "ST_PALN      " ,
    "ST_NTSC4_60  " ,
    "ST_NTSC_N    " ,
    "ST_PAL_M     " ,
    "ST_NTSC_JAPAN" ,
    "ST_SECAM     "
};

static const char TabVipInput[3][10] = {
    "VideoComp" ,
    "VideoYc  " ,
    "CCIR656  "
};

static const char TabVipOutput[2][9] = {
    "ITU656  " ,
    "NoITU656"
};






/* --- Global variables --- */

char    EXTVIN_Msg[200];            /* text for trace */


/* --- Extern variables --- */

static STEXTVIN_Handle_t EXTVINHndl;
static task_t *ExtVinTid = NULL;
static BOOL gSpoilI2c;
static U32 SPOIL_DELAY=1000;
static U32 SpoilErrors=0;

extern ST_Partition_t *SystemPartition_p;

/* Prototype */


/* Private Function prototypes ---------------------------------------------- */
static BOOL EXTVIN_GetRevision(        parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_GetCapability(      parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_Init(               parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_Open(               parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_Close(              parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_Term(               parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_SelectVipInput(     parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_SelectVipOutput(    parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_SetVipStandard(     parse_t *pars_p, char *result_sym_p );
static BOOL EXTVIN_SetAdcInputFormat(  parse_t *pars_p, char *result_sym_p );


/* Local prototypes ------------------------------------------------------- */
/*-----------------------------------------------------------------------------
 * Function : EXTVIN_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL EXTVIN_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STEXTVIN_ERROR_INVALID_STANDARD_TYPE:
                strcat( Text, "(STEXTVIN Invalid Standard Type) !\n");
                break;
            case STEXTVIN_HW_FAILURE:
                strcat( Text, "(STEXTVIN Hardware Failure) !\n");
                break;
            case STEXTVIN_ERROR_INVALID_INPUT_TYPE:
                strcat( Text, "(STEXTVIN Invalid Input Type) !\n");
                break;
            case STEXTVIN_ERROR_INVALID_OUTPUT_TYPE:
                strcat( Text, "(STEXTVIN Invalid Output Type) !\n");
                break;
            case STEXTVIN_ERROR_I2C:
                strcat( Text, "(STEXTVIN Error I2C) !\n");
                break;
            case STEXTVIN_ERROR_FUNCTION_NOT_IMPLEMENTED:
                strcat( Text, "(STEXTVIN Error function not implemented) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    STTBX_Print(( "%s\n", Text ));
    return( API_EnableError ? RetErr : FALSE);
}

/*####################################################################*/
/*####################### EXTVIN COMMANDS ############################*/
/*####################################################################*/

/*-------------------------------------------------------------------------
 * Function : EXTVIN_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_GetRevision( parse_t *pars_p, char *result_sym_p )
{
    UNUSED_PARAMETER( pars_p);
    UNUSED_PARAMETER( result_sym_p);

    STTBX_Print(( "STEXTVIN_GetRevision() %s \n", STEXTVIN_GetRevision( ) ));
    return ( FALSE );
} /* end EXTVIN_GetRevision */

/*-------------------------------------------------------------------------
 * Function : EXTVIN_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_GetCapability( parse_t *pars_p, char *result_sym_p)
{
    BOOL                RetErr = TRUE;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    STEXTVIN_Capability_t EXTVINCapability;
    char EXTVINName[20];

    UNUSED_PARAMETER( result_sym_p);

    /* get device names : EXTVIN */
    RetErr = STTST_GetString( pars_p, DEFAULT_EXTVIN_NAME, EXTVINName, sizeof(EXTVINName) );
    if ( RetErr == TRUE )
    {
  	STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }

    /* Get capabilities */
    ErrCode = STEXTVIN_GetCapability(EXTVINName, &EXTVINCapability);

    /* */
    sprintf ( EXTVIN_Msg, "STEXTVIN_GetCapability ('%s')", EXTVINName);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg );
	if ( ErrCode == ST_NO_ERROR)
	{
        sprintf( EXTVIN_Msg, "\tCapability : VipSupported= %x ", EXTVINCapability.SupportedVipStandard);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
	}

    return ( API_EnableError ? RetErr : FALSE );
} /* end EXTVIN_GetCapability */

/*-------------------------------------------------------------------------
 * Function : EXTVIN_GetVipStatus
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_GetVipStatus( parse_t *pars_p, char *result_sym_p)
{
    BOOL                RetErr;
    ST_ErrorCode_t      ErrCode;
    U8                  Status;

    UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* close EXTVIN */
    ErrCode = STEXTVIN_GetVipStatus(EXTVINHndl, &Status);
    sprintf( EXTVIN_Msg, "STEXTVIN_GetVipStatus(%d): status= 0x%X ", (int)EXTVINHndl, Status);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);

    return ( API_EnableError ? RetErr : FALSE );

} /* end EXTVIN_GetVipStatus */


/*-------------------------------------------------------------------------
 * Function : EXTVIN_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Init( parse_t *pars_p, char *result_sym_p )
{
    STEXTVIN_InitParams_t EXTVINInitParams;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    BOOL                RetErr = FALSE;
    STEXTVIN_DeviceType_t Device;
    char EXTVINName[20], I2CName[20];
    U8 I2CAddress = 0;
    S32 TempVal = 0;

    UNUSED_PARAMETER( result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, DEFAULT_EXTVIN_NAME, EXTVINName, sizeof(EXTVINName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }
    /* Get device number */
    RetErr = STTST_GetInteger( pars_p, STEXTVIN_SAA7114, (S32*)&Device);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Device SAA7114=0(def), DB331=1, TDA8752=2, STV2310=3");
        return(TRUE);
    }

    strcpy( EXTVINInitParams.I2CDeviceName, STI2C_BACK_DEVICE_NAME);
    switch ( Device)
    {
        case 0 :
            EXTVINInitParams.DeviceType = STEXTVIN_SAA7114;
            EXTVINInitParams.I2CAddress = SAA7114_I2C_BASE;
            break;
        case 1 :
            EXTVINInitParams.DeviceType = STEXTVIN_DB331;
            EXTVINInitParams.I2CAddress = SAA7111_I2C_BASE;
            break;
        case 2 :
            EXTVINInitParams.DeviceType = STEXTVIN_TDA8752;
            EXTVINInitParams.I2CAddress = TDA8752_BASE;
            break;
        case 3 :
            EXTVINInitParams.DeviceType = STEXTVIN_STV2310;
            EXTVINInitParams.I2CAddress = STV2310_I2C_BASE;
            #if defined(mb391)   /* ST_7710 */
                strcpy( EXTVINInitParams.I2CDeviceName, STI2C_FRONT_DEVICE_NAME);
            #endif /* mb391 */
            break;
        default :
            STTST_TagCurrentLine( pars_p, "expected Device SAA7114=0(def), DB331=1, TDA8752=2, STV2310=3");
            return(TRUE);
            break;
    }

    /* Get i2c device name */
    RetErr = STTST_GetString( pars_p, EXTVINInitParams.I2CDeviceName, I2CName, sizeof(I2CName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected I2C DeviceName");
        return(TRUE);
    }

    /* Get i2c address */
    RetErr = STTST_GetInteger( pars_p, EXTVINInitParams.I2CAddress, (S32*)&TempVal);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected I2C Address");
        return(TRUE);
    }
    I2CAddress = (U8)TempVal;
    EXTVINInitParams.I2CAddress = I2CAddress;

    /* EXTVIN InitParams */
    EXTVINInitParams.CPUPartition_p = SystemPartition_p;
    EXTVINInitParams.MaxOpen = 1;
    strcpy( EXTVINInitParams.I2CDeviceName, I2CName);

    /* Init EXTVIN */
    ErrCode = STEXTVIN_Init( EXTVINName, &EXTVINInitParams);

    /* error ? */
    sprintf ( EXTVIN_Msg, "STEXTVIN_Init('%s'): ", EXTVINName);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);

    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf ( EXTVIN_Msg, "\tTyp=" );
        switch ( EXTVINInitParams.DeviceType)
        {
            case 0 :    strcat( EXTVIN_Msg, "SAA7114 ,");   break;
            case 1 :    strcat( EXTVIN_Msg, "DB331 ,");     break;
            case 2 :    strcat( EXTVIN_Msg, "TDA8752 ,");   break;
            case 3 :    strcat( EXTVIN_Msg, "STV2310 ,");   break;
            default:    strcat( EXTVIN_Msg, "??? ,");       break;
        }
        sprintf ( EXTVIN_Msg, "%s I2C='%s'/0x%2x", EXTVIN_Msg, I2CName, I2CAddress);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Open( parse_t *pars_p, char *result_sym_p )
{
    STEXTVIN_OpenParams_t EXTVINOpenParams;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    BOOL                RetErr = FALSE;
    char EXTVINName[20];

    /* Get device name */
    RetErr = STTST_GetString( pars_p, DEFAULT_EXTVIN_NAME, EXTVINName, sizeof(EXTVINName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }

    /* open EXTVIN */
    ErrCode = STEXTVIN_Open( EXTVINName, &EXTVINOpenParams, &EXTVINHndl);

    /* error ? */
    sprintf( EXTVIN_Msg, "STEXTVIN Open('%s'): ", EXTVINName);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        STTST_AssignInteger( result_sym_p, (int)EXTVINHndl, FALSE);
        sprintf(EXTVIN_Msg, "\tHdl %d", (int)EXTVINHndl);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
    }

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Close( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    BOOL            RetErr = FALSE;

    UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* close EXTVIN */
    ErrCode = STEXTVIN_Close(EXTVINHndl);

    /* */
    sprintf( EXTVIN_Msg, "STEXTVIN_Close(%d): ", (int)EXTVINHndl);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Term( parse_t *pars_p, char *result_sym_p )
{
    STEXTVIN_TermParams_t EXTVINTermParams;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    BOOL                RetErr = FALSE;
    char EXTVINName[20];
    long LVar;

    UNUSED_PARAMETER( result_sym_p);

    /* Get device name */
    RetErr = STTST_GetString( pars_p, DEFAULT_EXTVIN_NAME, EXTVINName, sizeof(EXTVINName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName");
        return(TRUE);
    }

    /* get ForceTerminate (default: 1 = TRUE) */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "! expected ForceTerminate : 0:FALSE or 1:TRUE");
        return(TRUE);
    }
    EXTVINTermParams.ForceTerminate = ( LVar == 1);

    /* Term EXTVIN */
    ErrCode = STEXTVIN_Term(EXTVINName, &EXTVINTermParams);

    /* */
    sprintf( EXTVIN_Msg, "STEXTVIN_Term('%s'): ", EXTVINName);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_SelectVipInput
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_SelectVipInput( parse_t *pars_p, char *result_sym_p )
{
  ST_ErrorCode_t ErrCode = ST_NO_ERROR;
  BOOL RetErr = FALSE;
  STEXTVIN_VipInputSelection_t VipInput ;

  UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get VipInput Seclection  (default: 1=VideoYc) */
    RetErr = STTST_GetInteger( pars_p, STEXTVIN_VideoComp, (S32*)&VipInput);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Vip Input (0=VideoComp(def), 1=VideoYc, 2=CCIR656)\n" );
        return(TRUE);
    }

    ErrCode = STEXTVIN_SelectVipInput(EXTVINHndl, VipInput);
    sprintf( EXTVIN_Msg, "STEXTVIN_SelectVipInput(%d): ", (int)EXTVINHndl);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf( EXTVIN_Msg, "\tVipIn=%s", TabVipInput[VipInput]);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end EXTVIN_SelectVipInput */

/*-------------------------------------------------------------------------
 * Function : EXTVIN_SelectVipOutput
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_SelectVipOutput( parse_t *pars_p, char *result_sym_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr = FALSE;
    STEXTVIN_VipOutputSelection_t VipOutput ;

    UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get VipOutput Seclection  (default: 1=ITU656) */
    RetErr = STTST_GetInteger( pars_p, STEXTVIN_ITU656, (S32*)&VipOutput);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected Vip Output (1=ITU656, 0=NoITU656)\n" );
        return(TRUE);
    }

    /* Select Vip Output */
    sprintf ( EXTVIN_Msg, "STEXTVIN_SelectVipOutput(%d): ", (int)EXTVINHndl);
    ErrCode = STEXTVIN_SelectVipOutput(EXTVINHndl, VipOutput);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf ( EXTVIN_Msg, "\tVipOut=%s", TabVipOutput[VipOutput]);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end EXTVIN_SelectVipOutput */

/*-------------------------------------------------------------------------
 * Function : EXTVIN_SetVipStandard
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_SetVipStandard( parse_t *pars_p, char *result_sym_p )
{
  ST_ErrorCode_t ErrCode = ST_NO_ERROR;
  BOOL RetErr;
  STEXTVIN_VipStandard_t VipStandard ;

  UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get VipStandard (default: 2=ntsc m) */
    RetErr = STTST_GetInteger( pars_p, STEXTVIN_VIPSTANDARD_NTSC_M, (S32*)&VipStandard);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected standard (0=undef, 1=PAL_BGDHI, 2=NTSC_M, 3=PAL4, 4=_NTSC4_50, 5=PALN, 6=NTSC4_60, 7=NTSC_N, 8=PAL_M, 9=NTSC_JAPAN 10=SECAM)\n" );
        return(TRUE);
    }

    /* Set Vip Standard */
    sprintf ( EXTVIN_Msg, "STEXTVIN_SetVipStandard(%d): ", (int)EXTVINHndl );
    ErrCode = STEXTVIN_SetVipStandard(EXTVINHndl, VipStandard);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf ( EXTVIN_Msg, "\tVipStandard=%s", TabVipStd[VipStandard]);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end EXTVIN_SetVipStandard */

/*-------------------------------------------------------------------------
 * Function : EXTVIN_SetAdcInputFormat
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_SetAdcInputFormat( parse_t *pars_p, char *result_sym_p )
{
  ST_ErrorCode_t ErrCode = ST_NO_ERROR;
  BOOL RetErr;
  STEXTVIN_AdcInputFormat_t ADCFormat;

  UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get ADCFormat (default: 1=) */
    RetErr = STTST_GetInteger( pars_p, STEXTVIN_SVGA_800_600, (S32*)&ADCFormat);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected format (0=VGA_640_480, 1=SVGA_800_600, 2=XGA_1024_768, 3=SUN_1152_900)\n" );
        return(TRUE);
    }

    /* Set ADC Input Format */
    sprintf ( EXTVIN_Msg, "STEXTVIN_SetAdcInputFormat(%d): ", (int)EXTVINHndl);
    ErrCode = STEXTVIN_SetAdcInputFormat(EXTVINHndl, ADCFormat);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf ( EXTVIN_Msg, "\tADCFormat=%s", TabAdcInput[ADCFormat]);
        STTBX_Print(( "%s\n", EXTVIN_Msg ));
    }
    return ( API_EnableError ? RetErr : FALSE );
} /* end EXTVIN_SetAdcInputFormat */

/*-------------------------------------------------------------------------
 * Function : EXTVIN_Write
 *            Write one I2C register.
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Write(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr;
    U16 I2C_Adress, I2C_Value;
    S32 TempVal = 0;

    UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get I2C adress (default: 0x00) */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&TempVal);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected I2C Adress \n" );
        return(TRUE);
    }
    I2C_Adress = (U16) TempVal;
    /* get I2C register Value (default: 0x00) */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&TempVal);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected I2C register value \n" );
        return(TRUE);
    }
    I2C_Value = (U16) TempVal;
    sprintf ( EXTVIN_Msg, "EXTVIN_WriteI2C (Addr :0x%02x, Value :0x%02x ): ", I2C_Adress, I2C_Value);
    ErrCode = STEXTVIN_WriteI2C(EXTVINHndl, (U8)I2C_Adress, (U8)I2C_Value);
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_Read
 *            Read one I2C register.
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Read(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    BOOL RetErr;
    U16 I2C_Adress, I2C_Value;
    S32 TempVal = 0;

    UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    /* get I2C adress (default: 0x00) */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&TempVal);
    if ( RetErr == TRUE )
    {
        tag_current_line( pars_p, "expected I2C Adress \n" );
        return(TRUE);
    }
    I2C_Adress = (U16) TempVal;

    if ( STEXTVIN_ReadI2C(EXTVINHndl, (U8)I2C_Adress, (U8*)&I2C_Value) == ST_NO_ERROR)
    {
        sprintf ( EXTVIN_Msg, "EXTVIN_ReadI2C (Addr :0x%02x, Value :0x%02x ): ", I2C_Adress, I2C_Value);
    }
    else
    {
        sprintf ( EXTVIN_Msg, "EXTVIN_ReadI2C (Addr :0x%02x, Value : ?? ): ", I2C_Adress);
    }
    RetErr = EXTVIN_TextError( ErrCode, EXTVIN_Msg);

    return ( API_EnableError ? RetErr : FALSE );
}


/*-------------------------------------------------------------------------
 * Function : DoSpoil_I2C
 *            Function is called by subtask
 * Input    :
 * Parameter:
 * Output   : TRUE if ok
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static void DoSpoil_I2C( void )
{
    U8 Dummy;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    while (gSpoilI2c)
    {
        ErrCode=STEXTVIN_ReadI2C(EXTVINHndl, 0x00, &Dummy);
        if (ErrCode!=ST_NO_ERROR)
        {
            SpoilErrors++;
        }
        STOS_TaskDelay(SPOIL_DELAY);
    }
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_Spoil
 *            Read one I2C register.
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_Spoil(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U32 Delay;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    UNUSED_PARAMETER( result_sym_p);

    /* get handle */
    RetErr = STTST_GetInteger( pars_p, (S32)EXTVINHndl, (S32*)&EXTVINHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, SPOIL_DELAY, (S32*)&Delay);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Delay in milliseconds");
        return(TRUE);
    }

    SPOIL_DELAY = (ST_GetClocksPerSecond()*Delay)/(U32)1000;

    if (ExtVinTid==NULL)
    {
        gSpoilI2c=TRUE;
        SpoilErrors=0;

        Error = STOS_TaskCreate((void (*) (void*)) DoSpoil_I2C, (void *) NULL,
                            0 ,
                            1024,
                            0,
                            NULL ,
                            &(ExtVinTid),
                            0,
                            2,
                            "Extvin_Spoil_I2C",
                            0);

    }
    if ( Error != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to create Extvin_Spoil_I2C task: exiting ...\n"));
        return(TRUE);
    }

    return ( FALSE );
}

/*-------------------------------------------------------------------------
 * Function : EXTVIN_KillSpoil
 *            Read one I2C register.
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_KillSpoil(parse_t *pars_p, char *result_sym_p)
{
    task_t *TaskList[1];

    UNUSED_PARAMETER( pars_p);
    UNUSED_PARAMETER( result_sym_p);

    TaskList[0] = ExtVinTid;
    gSpoilI2c=FALSE;
    if ( STOS_TaskWait( TaskList, TIMEOUT_INFINITY ) != ST_NO_ERROR )
    {
        STTBX_Print(("Error: Timeout task_wait\n"));
        return ( TRUE );
    }
    STTBX_Print(("Extvin_Spoil_I2C Task finished\n"));
    if ( STOS_TaskDelete( ExtVinTid, 0, 0, 0 ) != ST_NO_ERROR )
    {
        STTBX_Print(("Error: unable to delete Extvin_Spoil_I2C Task !\n"));
        return ( TRUE );
    }
    STTBX_Print(("Extvin_Spoil_I2C Task deleted\n"));
    ExtVinTid = NULL;

    return ( FALSE );
}



/*-------------------------------------------------------------------------
 * Function : EXTVIN_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL EXTVIN_InitCommand(void)
{
  BOOL RetErr = FALSE;
/* API functions : */
    RetErr  = STTST_RegisterCommand( "EXTVINInit"        ,EXTVIN_Init, "EXTVIN Init 'dev' 'type' 'i2cname' 'i2caddress'");
    RetErr |= STTST_RegisterCommand( "EXTVINOpen"        ,EXTVIN_Open, "EXTVIN Open");
    RetErr |= STTST_RegisterCommand( "EXTVINClose"       ,EXTVIN_Close, "EXTVIN Close");
    RetErr |= STTST_RegisterCommand( "EXTVINTerm"        ,EXTVIN_Term, "EXTVIN Term");
    RetErr |= STTST_RegisterCommand( "EXTVINCapa"        ,EXTVIN_GetCapability, "EXTVIN Get Capability");
    RetErr |= STTST_RegisterCommand( "EXTVINGetVipStatus",EXTVIN_GetVipStatus, "EXTVIN Get Vip status");
    RetErr |= STTST_RegisterCommand( "EXTVINRev"         ,EXTVIN_GetRevision, "EXTVIN Get Revision");
    RetErr |= STTST_RegisterCommand( "EXTVINVipInput"    ,EXTVIN_SelectVipInput, "Select EXTVIN Vip Input");
    RetErr |= STTST_RegisterCommand( "EXTVINVipOutput"   ,EXTVIN_SelectVipOutput, "Select EXTVIN Vip Output");
    RetErr |= STTST_RegisterCommand( "EXTVINVipStandard" ,EXTVIN_SetVipStandard, "Set EXTVIN Vip Standard");
    RetErr |= STTST_RegisterCommand( "EXTVINAdcInputFt"  ,EXTVIN_SetAdcInputFormat, "Set EXTVIN Adc Input Format");

    RetErr |= STTST_RegisterCommand( "EXTVINWriteI2c"    ,EXTVIN_Write, "<I2C Sub-Address> <Value> Write one I2C register");
    RetErr |= STTST_RegisterCommand( "EXTVINReadI2c"     ,EXTVIN_Read, "<I2C Sub-Address> <Value> Read one I2C register");
    RetErr |= STTST_RegisterCommand( "EXTVINSpoilI2c"    ,EXTVIN_Spoil, "<DelayBetween2Reads> starts an I2c polling read task");
    RetErr |= STTST_RegisterCommand( "EXTVINKillSpoilI2c",EXTVIN_KillSpoil, "");

/* assign */
    RetErr |= STTST_AssignInteger ("NTSC_640_480"  ,STEXTVIN_NTSC_640_480  , TRUE);
    RetErr |= STTST_AssignInteger ("VGA_640_480"   ,STEXTVIN_VGA_640_480   , TRUE);
    RetErr |= STTST_AssignInteger ("NTSC_720_480"  ,STEXTVIN_NTSC_720_480  , TRUE);
    RetErr |= STTST_AssignInteger ("SVGA_800_600"  ,STEXTVIN_SVGA_800_600  , TRUE);
    RetErr |= STTST_AssignInteger ("XGA_1024_768"  ,STEXTVIN_XGA_1024_768  , TRUE);
    RetErr |= STTST_AssignInteger ("SUN_1152_900"  ,STEXTVIN_SUN_1152_900  , TRUE);
    RetErr |= STTST_AssignInteger ("HD_1280_720"   ,STEXTVIN_HD_1280_720   , TRUE);
    RetErr |= STTST_AssignInteger ("SXGA_1280_1024",STEXTVIN_SXGA_1280_1024, TRUE);
    RetErr |= STTST_AssignInteger ("HD_1920_1080"  ,STEXTVIN_HD_1920_1080  , TRUE);

    RetErr |= STTST_AssignInteger ("SAA7114"       ,STEXTVIN_SAA7114, TRUE);
    RetErr |= STTST_AssignInteger ("DB331"         ,STEXTVIN_DB331, TRUE);
    RetErr |= STTST_AssignInteger ("TDA8752"       ,STEXTVIN_TDA8752, TRUE);

    RetErr |= STTST_AssignInteger ("ST_UNDEFINED"  ,STEXTVIN_VIPSTANDARD_UNDEFINED, TRUE);
    RetErr |= STTST_AssignInteger ("ST_PAL_BGDHI"  ,STEXTVIN_VIPSTANDARD_PAL_BGDHI, TRUE);
    RetErr |= STTST_AssignInteger ("ST_NTSC_M"     ,STEXTVIN_VIPSTANDARD_NTSC_M, TRUE);
    RetErr |= STTST_AssignInteger ("ST_PAL4"       ,STEXTVIN_VIPSTANDARD_PAL4, TRUE);
    RetErr |= STTST_AssignInteger ("ST_NTSC4_50"   ,STEXTVIN_VIPSTANDARD__NTSC4_50, TRUE);
    RetErr |= STTST_AssignInteger ("ST_PALN"       ,STEXTVIN_VIPSTANDARD_PALN, TRUE);
    RetErr |= STTST_AssignInteger ("ST_NTSC4_60"   ,STEXTVIN_VIPSTANDARD_NTSC4_60, TRUE);
    RetErr |= STTST_AssignInteger ("ST_NTSC_N"     ,STEXTVIN_VIPSTANDARD_NTSC_N, TRUE);
    RetErr |= STTST_AssignInteger ("ST_PAL_M"      ,STEXTVIN_VIPSTANDARD_PAL_M, TRUE);
    RetErr |= STTST_AssignInteger ("ST_NTSC_JAPAN" ,STEXTVIN_VIPSTANDARD_NTSC_JAPAN, TRUE);
    RetErr |= STTST_AssignInteger ("ST_SECAM"      ,STEXTVIN_VIPSTANDARD_SECAM, TRUE);

    RetErr |= STTST_AssignInteger ("VideoComp"     ,STEXTVIN_VideoComp, TRUE);
    RetErr |= STTST_AssignInteger ("VideoYc"       ,STEXTVIN_VideoYc, TRUE);
    RetErr |= STTST_AssignInteger ("CCIR656"       ,STEXTVIN_CCIR656, TRUE);

    RetErr |= STTST_AssignInteger ("ITU656"        ,STEXTVIN_ITU656, TRUE);
    RetErr |= STTST_AssignInteger ("NoITU656"      ,STEXTVIN_NoITU656, TRUE);

    return( RetErr ? FALSE : TRUE);

} /* end DNC_MacroInit */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL EXTVIN_RegisterCmd()
{
    BOOL RetOk;

    RetOk = EXTVIN_InitCommand();
    if ( RetOk == FALSE )
    {
        sprintf( EXTVIN_Msg, "EXTVIN_RegisterCmd()  : failed !\n" );
    }
    else
    {
        sprintf( EXTVIN_Msg, "EXTVIN_RegisterCmd()  : ok, \trevision=%s\n", STEXTVIN_GetRevision() );
    }
    STTST_Print((EXTVIN_Msg));
    return(RetOk);
}
/* end of file */
