/************************************************************************
COPYRIGHT (C) STMicroelectronics 2001

File name   : denc_cmd.c
Description : Definition of denc commands

Note        :

Date          Modification                                    Initials
----          ------------                                    --------

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stsys.h"

#include "startup.h"
#include "stdenc.h"
#include "stvtg.h"
#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#include "sti2c.h"
#endif  /* #ifdef USE_I2C */

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants --- */
#define DENC_MAX_MODE           9
#define DENC_MAX_OPTION_MODE    7
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#define I2C_STV119_ADDRESS   0x40
#define I2C_TIMEOUT          1000
#endif  /* USE_I2C USE_STDENC_I2C */

/* Private type */
enum{
    DENC_ACCESS_8BITS,
    DENC_ACCESS_32BITS,
    DENC_ACCESS_I2C
};

/* --- Global variables --- */

/* --- Macros ---*/
#if defined (ST_5508) || defined (ST_5510) || defined (ST_5512) || defined (ST_5518) || defined (ST_5514)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined (STV0119)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_I2C
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         0
#define DENC_REG_SHIFT           0
#elif defined (ST_7015)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7015
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DNC_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7015_DENC_OFFSET
#define DENC_REG_SHIFT           0
#elif defined (ST_7020)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7020
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DNC_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7020_DENC_OFFSET
#define DENC_REG_SHIFT           0
#elif defined (ST_GX1)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_GX1
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         0xBB430000 /* different from value given in vob chip DENC_BASE_ADDRESS */
#define DENC_REG_SHIFT           1
#else
#error Not defined yet
#endif

#define DENC_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(DNC_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

typedef struct
{
    const char Mode[15];
    const STDENC_Option_t Value;
}denc_ModeOption_t;

typedef struct
{
    const char Mode[10];
    const STDENC_Mode_t Value;
}denc_Mode_t;

/* --- Extern functions --- */

/* --- Extern variables --- */

/* --- Private variables --- */
static char DNC_Msg[600];            /* text for trace */
static STDENC_Handle_t DENCHndl;

static U8 Denc_Register[100];        /* To read and memorize denc values */
static U8 DencAccessType=DENC_ACCESS_8BITS;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)

static STI2C_Handle_t I2CHndl=-1;
#endif  /* #ifdef USE_I2C */

static const denc_Mode_t DencModeName[DENC_MAX_MODE+1] = {
    {"NONE", STDENC_MODE_NONE },
    {"NTSCM", STDENC_MODE_NTSCM },
    {"NTSCM_J", STDENC_MODE_NTSCM_J },
    {"NTSCM_443", STDENC_MODE_NTSCM_443 },
    {"PAL", STDENC_MODE_PALBDGHI },
    {"PALM", STDENC_MODE_PALM },
    {"PALN", STDENC_MODE_PALN },
    {"PALN_C", STDENC_MODE_PALN_C },
    {"SECAM", STDENC_MODE_SECAM },
    { "????", 0}
};

static const denc_ModeOption_t DencOptionMode[DENC_MAX_OPTION_MODE+1] = {
    { "LEVEL_PEDESTAL", STDENC_OPTION_BLACK_LEVEL_PEDESTAL},
    { "CHROMA_DELAY", STDENC_OPTION_CHROMA_DELAY},
    { "TRAP_FILTER", STDENC_OPTION_LUMA_TRAP_FILTER},
    { "RATE_60HZ", STDENC_OPTION_FIELD_RATE_60HZ},
    { "NON_INTERLACED", STDENC_OPTION_NON_INTERLACED},
    { "SQUARE_PIXEL", STDENC_OPTION_SQUARE_PIXEL},
    { "YCBCR444_INPUT", STDENC_OPTION_YCBCR444_INPUT},
    { "????", 0}
};

/* Prototype */

/* Local prototypes ------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : denc_read
 * Input    : Address, length, buffer
 * Output   : None
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL denc_read(U8 add, U8* value, U8 lenght)
{
    U8 i;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    ST_ErrorCode_t      ErrCode;
    U32                 ActLen;
#endif  /* #ifdef USE_I2C */

    switch(DencAccessType)
    {
        case DENC_ACCESS_32BITS:
            for(i=0; i<lenght; i++, value++)
                *value=STSYS_ReadRegDev32LE((void*)((U32*)(DNC_BASE_ADDRESS+DNC_DEVICE_BASE_ADDRESS)+((i+add)<<DENC_REG_SHIFT)));
            break;

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        case DENC_ACCESS_I2C:
            if(I2CHndl == -1){
                STI2C_OpenParams_t  OpenParams;

                OpenParams.I2cAddress        = I2C_STV119_ADDRESS;
                OpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
                OpenParams.BusAccessTimeOut  = 100000;
                ErrCode=STI2C_Open(STI2C_BACK_DEVICE_NAME, &OpenParams, &I2CHndl);
                if(ErrCode != ST_NO_ERROR){
                    STTST_Print(("Failed to open back I2C driver\n"));
                    return(TRUE);
                }
            }

            ErrCode=STI2C_Write(I2CHndl, &add, 1, I2C_TIMEOUT, &ActLen);
            if(ErrCode != ST_NO_ERROR)
                return(TRUE);

            ErrCode=STI2C_Read(I2CHndl, value, lenght, I2C_TIMEOUT, &ActLen);
            if(ErrCode != ST_NO_ERROR)
                return(TRUE);
            break;
#endif  /* #ifdef USE_I2C */

        case DENC_ACCESS_8BITS:
            for(i=0; i<lenght; i++, value++)
                *value=STSYS_ReadRegDev8((void*)((U8*)(DNC_BASE_ADDRESS+DNC_DEVICE_BASE_ADDRESS)+add+i));
            break;

        default:
            STTST_Print(("Denc access not provided !!!\n"));
            return(TRUE);
            break;
    }
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : denc_write
 * Input    : Address, length, buffer
 * Output   : None
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL denc_write(U8 add, U8* value, U8 lenght)
{
    U8 i;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    ST_ErrorCode_t      ErrCode;
    U32                 ActLen;
    U8                  SavValue;
#endif  /* #ifdef USE_I2C */

    switch(DencAccessType)
    {
        case DENC_ACCESS_32BITS:
            for(i=0; i<lenght; i++, value++)
                STSYS_WriteRegDev32LE((void*)((U32*)(DNC_BASE_ADDRESS+DNC_DEVICE_BASE_ADDRESS)+((i+add)<<DENC_REG_SHIFT)),*value);
            break;

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        case DENC_ACCESS_I2C:
            if(I2CHndl == -1){
                STI2C_OpenParams_t  OpenParams;

                OpenParams.I2cAddress        = I2C_STV119_ADDRESS;
                OpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
                OpenParams.BusAccessTimeOut  = 100000;
                ErrCode=STI2C_Open(STI2C_BACK_DEVICE_NAME, &OpenParams, &I2CHndl);
                if(ErrCode != ST_NO_ERROR){
                    STTST_Print(("Failed to open back I2C driver\n"));
                    return(TRUE);
                }
            }
            value--;
            SavValue = *value;
            *value=add;

            ErrCode=STI2C_Write(I2CHndl, value, lenght+1, I2C_TIMEOUT, &ActLen);
            *value=SavValue;
            value++;
            if(ErrCode != ST_NO_ERROR)
                return(TRUE);
            break;
#endif  /* #ifdef USE_I2C */

        case DENC_ACCESS_8BITS:
            for(i=0; i<lenght; i++, value++)
                STSYS_WriteRegDev8((void*)((U8*)(DNC_BASE_ADDRESS+DNC_DEVICE_BASE_ADDRESS)+add+i),*value);
            break;

        default:
            STTST_Print(("Denc access not provided !!!\n"));
            return(TRUE);
            break;
    }
    return(FALSE);
}


/*-----------------------------------------------------------------------------
 * Function : DENC_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL DENC_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STDENC_ERROR_I2C:
                strcat( Text, "(stdenc error i2c) !\n");
                break;
            case STDENC_ERROR_INVALID_ENCODING_MODE:
                strcat( Text, "(stdenc error invalid encoding mode) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    DENC_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*#######################################################################*/
/*############################# DENC COMMANDS ###########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : DENC_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_InitParams_t DENCInitParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    U32 Var;

    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
    STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }

    /* Get Max Open */
    RetErr = STTST_GetInteger(pars_p, 5, (S32*)&Var);
    if (RetErr == TRUE)
    {
        tag_current_line(pars_p, "Expected max open (default 5)");
        return(TRUE);
    }
    DENCInitParams.MaxOpen = Var;

    /* Get Device Type */
    RetErr = STTST_GetInteger(pars_p, DENC_DEVICE_TYPE, (S32*)&Var);
    if (RetErr == TRUE)
    {
        tag_current_line(pars_p, "Expected device type (0=DENC,1=7015,2=7020,3=GX1GX1)");
        return(TRUE);
    }
    DENCInitParams.DeviceType = (STDENC_DeviceType_t)Var;

    /* Get Device Type */
    RetErr = STTST_GetInteger(pars_p, DENC_DEVICE_ACCESS, (S32*)&Var);
    if (RetErr == TRUE)
    {
        tag_current_line(pars_p, "Expected Access type (0=EMI,1=I2C)");
        return(TRUE);
    }
    DENCInitParams.AccessType = (STDENC_DeviceType_t)Var;
    if (DENCInitParams.AccessType == STDENC_ACCESS_TYPE_EMI)
    {
        DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_DEVICE_BASE_ADDRESS;
        DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)DNC_BASE_ADDRESS;
#if defined (ST_7015) || defined(ST_7020) || defined(ST_GX1) || defined(ST_NGX1)
        DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_32_BITS;
        DencAccessType=DENC_ACCESS_32BITS;
#else
        DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_8_BITS;
        DencAccessType=DENC_ACCESS_8BITS;
#endif
    }
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    else /* I2C driver for STV0119 */
    {
        strcpy(DENCInitParams.STDENC_Access.I2C.DeviceName, STI2C_BACK_DEVICE_NAME);
        DENCInitParams.STDENC_Access.I2C.OpenParams.I2cAddress = I2C_STV119_ADDRESS;
        DENCInitParams.STDENC_Access.I2C.OpenParams.AddressType = STI2C_ADDRESS_7_BITS;
        DENCInitParams.STDENC_Access.I2C.OpenParams.BusAccessTimeOut = STI2C_TIMEOUT_INFINITY;
        DencAccessType=DENC_ACCESS_I2C;
    }
#endif  /* #ifdef USE_I2C */
    sprintf( DNC_Msg, "DENC_Init(%s, Type=%s, Access=%s, MaxOpen=%d): ", DeviceName,
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_DENC) ? "DENC" :
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_7015) ? "7015" :
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_7020) ? "7020" :
#if defined(ST_GX1) || defined(ST_NGX1)
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_GX1) ? "GX1" :
#endif
             "????",
             (DENCInitParams.AccessType == STDENC_ACCESS_TYPE_EMI) ? "EMI" :
             (DENCInitParams.AccessType == STDENC_ACCESS_TYPE_I2C) ? "I2C" \
             : "????",
             DENCInitParams.MaxOpen
        );
    ErrCode = STDENC_Init(DeviceName, &DENCInitParams);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    return(API_EnableError ? RetErr: FALSE);
}


/*-------------------------------------------------------------------------
 * Function : DENC_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_OpenParams_t DENCOpenParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t      ErrCode;

    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }

    sprintf( DNC_Msg, "DENC_Open(%s): ", DeviceName);
    ErrCode = STDENC_Open(DeviceName, &DENCOpenParams, &DENCHndl);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);
    if ( RetErr == ST_NO_ERROR)
    {
        STTST_AssignInteger( result_sym_p, DENCHndl, FALSE);
        sprintf( DNC_Msg, "\thandle %d\n", DENCHndl);
        DENC_PRINT(DNC_Msg);
    }

    return(API_EnableError ? RetErr: FALSE);
}


/*-------------------------------------------------------------------------
 * Function : DENC_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t      ErrCode;

    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }

    sprintf( DNC_Msg, "DENC_Close(%d): ", DENCHndl);
    ErrCode = STDENC_Close(DENCHndl);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    return(RetErr);
}


/*-------------------------------------------------------------------------
 * Function : DENC_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_TermParams_t DENCTermParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Var;

    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, FALSE, &Var);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }
    DENCTermParams.ForceTerminate = (BOOL)Var;

    sprintf( DNC_Msg, "DENC_Term(%s, Force=%s): ",
             DeviceName,
             (DENCTermParams.ForceTerminate == FALSE) ? "FALSE" :
             (DENCTermParams.ForceTerminate == TRUE) ? "TRUE" : "???" );
    ErrCode = STDENC_Term(DeviceName, &DENCTermParams);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    if(RetErr == ST_NO_ERROR)
    {
        /* Close access to I2C before leaving */
        if(I2CHndl != -1)
        {
            ErrCode=STI2C_Close(I2CHndl);
            if(ErrCode != ST_NO_ERROR)
            {
                sprintf(DNC_Msg, "Failed to close I2C for private denc access !!!\n");
                DENC_PRINT(DNC_Msg);
            }
            I2CHndl = -1;
        }
    }
#endif  /* USE_I2C USE_STDENC_I2C */

    return(API_EnableError ? RetErr: FALSE);
}


/*-------------------------------------------------------------------------
 * Function : DENC_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_GetRevision( STTST_Parse_t *pars_p, char *result_sym_p )
{
    ST_Revision_t RevisionNumber;

    RevisionNumber = STDENC_GetRevision();
    sprintf( DNC_Msg, "DENC_GetRevision(): Ok\nrevision=%s\n", RevisionNumber );
    DENC_PRINT(DNC_Msg);
    return ( FALSE );
} /* end DNC_GetRevision */


/*-------------------------------------------------------------------------
 * Function : DENC_SetEncodingMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_SetEncodingMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_EncodingMode_t ModeDenc;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    U32 Var;
    char *ptr=NULL, TrvStr[80], IsStr[80];

    /* Default init */
    memset(&ModeDenc, 0xff, sizeof(STDENC_EncodingMode_t));

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    /* get output mode  (default: PAL) */
    STTST_GetItem( pars_p, "PAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<DENC_MAX_MODE; Var++){
        if((strcmp(DencModeName[Var].Mode, TrvStr)==0) || \
           (strncmp(DencModeName[Var].Mode, TrvStr+1, strlen(DencModeName[Var].Mode))==0))
            break;
    }
    if(Var==DENC_MAX_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr==TRUE){
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected encoding mode (See capabilities)");
                return(TRUE);
            }
        }
    }
    ModeDenc.Mode = (STDENC_Mode_t)Var;

    if((ModeDenc.Mode != STDENC_MODE_SECAM) && (ModeDenc.Mode != STDENC_MODE_NONE)){
        /* Get square pixel */
        RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&Var);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "Expected SquarePixel (default=FALSE)");
            return(TRUE);
        }
        switch(ModeDenc.Mode){
            case STDENC_MODE_PALBDGHI:
            case STDENC_MODE_PALM:
            case STDENC_MODE_PALN:
            case STDENC_MODE_PALN_C:
                ModeDenc.Option.Pal.SquarePixel = (BOOL)Var;
                break;

            case STDENC_MODE_NTSCM:
            case STDENC_MODE_NTSCM_J:
            case STDENC_MODE_NTSCM_443:
                ModeDenc.Option.Ntsc.SquarePixel = (BOOL)Var;
                break;
        }

        /* Get interlaced */
        RetErr = STTST_GetInteger( pars_p, TRUE, (S32*)&Var);
        if ( RetErr == TRUE )
        {
            STTST_TagCurrentLine( pars_p, "Expected Interlaced (default=TRUE)");
            return(TRUE);
        }
        switch(ModeDenc.Mode){
            case STDENC_MODE_PALBDGHI:
            case STDENC_MODE_PALM:
            case STDENC_MODE_PALN:
            case STDENC_MODE_PALN_C:
                ModeDenc.Option.Pal.Interlaced = (BOOL)Var;
                break;

            case STDENC_MODE_NTSCM:
            case STDENC_MODE_NTSCM_J:
            case STDENC_MODE_NTSCM_443:
                ModeDenc.Option.Ntsc.Interlaced = (BOOL)Var;
                break;
        }

        switch(ModeDenc.Mode){

            case STDENC_MODE_NTSCM:
            case STDENC_MODE_NTSCM_J:
            case STDENC_MODE_NTSCM_443:
                /* Get FR60Hz */
                RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&Var);
                if ( RetErr == TRUE )
                {
                    STTST_TagCurrentLine( pars_p, "Expected Field rate 60Hz (default=FALSE)");
                    return(TRUE);
                }
                ModeDenc.Option.Ntsc.FieldRate60Hz = (BOOL)Var;
                break;
        }
    }

    switch(ModeDenc.Mode){
        case STDENC_MODE_NTSCM:
        case STDENC_MODE_NTSCM_J:
        case STDENC_MODE_NTSCM_443:
            sprintf( DNC_Msg, "DENC_SetEncodingMode(%d,Mode=%s,SQ=%s,Inter=%s,FR60Hz=%s): ", DENCHndl,
                     (ModeDenc.Mode < DENC_MAX_MODE) ? DencModeName[ModeDenc.Mode].Mode : "????",
                     (ModeDenc.Option.Ntsc.SquarePixel == FALSE) ? "FALSE" :
                     (ModeDenc.Option.Ntsc.SquarePixel == TRUE) ? "TRUE" : "????",
                     (ModeDenc.Option.Ntsc.Interlaced == FALSE) ? "FALSE" :
                     (ModeDenc.Option.Ntsc.Interlaced == TRUE) ? "TRUE" : "????",
                     (ModeDenc.Option.Ntsc.FieldRate60Hz == FALSE) ? "FALSE" :
                     (ModeDenc.Option.Ntsc.FieldRate60Hz == TRUE) ? "TRUE" : "????"
                );
            break;

        case STDENC_MODE_PALBDGHI:
        case STDENC_MODE_PALM:
        case STDENC_MODE_PALN:
        case STDENC_MODE_PALN_C:
            sprintf( DNC_Msg, "DENC_SetEncodingMode(%d,Mode=%s,SQ=%s,Inter=%s): ", DENCHndl,
                     (ModeDenc.Mode < DENC_MAX_MODE) ? DencModeName[ModeDenc.Mode].Mode : "????",
                     (ModeDenc.Option.Pal.SquarePixel == FALSE) ? "FALSE" :
                     (ModeDenc.Option.Pal.SquarePixel == TRUE) ? "TRUE" : "????",
                     (ModeDenc.Option.Pal.Interlaced == FALSE) ? "FALSE" :
                     (ModeDenc.Option.Pal.Interlaced == TRUE) ? "TRUE" : "????"
                );
            break;

        case STDENC_MODE_SECAM:
        default:
            sprintf( DNC_Msg, "DENC_SetEncodingMode(%d,Mode=%s): ", DENCHndl,
                     (ModeDenc.Mode < DENC_MAX_MODE) ? DencModeName[ModeDenc.Mode].Mode : "????"
                );
            break;
    }

    ErrCode = STDENC_SetEncodingMode(DENCHndl, &ModeDenc);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    return(API_EnableError ? RetErr: FALSE);
} /* end DENC_SetEncodingMode */


/*-------------------------------------------------------------------------
 * Function : DENC_GetEncodingMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL DENC_GetEncodingMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_EncodingMode_t Mode;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    char StrParams[RETURN_PARAMS_LENGTH];

    /* Default init */
    memset(&Mode, 0xff, sizeof(STDENC_EncodingMode_t));

    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }
    sprintf( DNC_Msg, "DENC_GetEncodingMode(%d): " , DENCHndl);
    ErrCode = STDENC_GetEncodingMode(DENCHndl, &Mode);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        switch(Mode.Mode)
        {
            case STDENC_MODE_NTSCM:
            case STDENC_MODE_NTSCM_J:
            case STDENC_MODE_NTSCM_443:
                sprintf( DNC_Msg, "\tMode=%s\tSquare=%s Interlaced=%s FR60Hz=%s\n", \
                            DencModeName[Mode.Mode].Mode,
                            (Mode.Option.Ntsc.SquarePixel == FALSE) ? "FALSE" :
                            (Mode.Option.Ntsc.SquarePixel == TRUE) ? "TRUE" : "????",
                            (Mode.Option.Ntsc.Interlaced == FALSE) ? "FALSE" :
                            (Mode.Option.Ntsc.Interlaced == TRUE) ? "TRUE" : "????"  ,
                            (Mode.Option.Ntsc.FieldRate60Hz == FALSE) ? "FALSE" :
                            (Mode.Option.Ntsc.FieldRate60Hz == TRUE) ? "TRUE" : "????"
                    );
                sprintf( StrParams, "%d %d %d %d", \
                            Mode.Mode, Mode.Option.Ntsc.SquarePixel, Mode.Option.Ntsc.Interlaced,
                            Mode.Option.Ntsc.FieldRate60Hz);
                break;

            case STDENC_MODE_PALBDGHI:
            case STDENC_MODE_PALM:
            case STDENC_MODE_PALN:
            case STDENC_MODE_PALN_C:
                sprintf( DNC_Msg, "\tMode=%s\tSquare=%s Interlaced=%s\n", \
                            DencModeName[Mode.Mode].Mode,
                            (Mode.Option.Pal.SquarePixel == FALSE) ? "FALSE" :
                            (Mode.Option.Pal.SquarePixel == TRUE) ? "TRUE" : "????",
                            (Mode.Option.Pal.Interlaced == FALSE) ? "FALSE" :
                            (Mode.Option.Pal.Interlaced == TRUE) ? "TRUE" : "????"
                    );
                sprintf( StrParams, "%d %d %d", \
                            Mode.Mode, Mode.Option.Pal.SquarePixel, Mode.Option.Pal.Interlaced);
                break;

            case STDENC_MODE_SECAM:
            default:
                sprintf( DNC_Msg, "\tMode=%s\n", \
                            DencModeName[Mode.Mode].Mode
                    );
                sprintf( StrParams, "%d", \
                            Mode.Mode);
                break;
        }
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        DENC_PRINT(DNC_Msg);
    }
    return(API_EnableError ? RetErr: FALSE);
} /* end DENC_GetEncodingMode */


/*-------------------------------------------------------------------------
 * Function : DENC_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_GetCapability( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STDENC_Capability_t Capability;
    char DeviceName[STRING_DEVICE_LENGTH];
    char StrParams[RETURN_PARAMS_LENGTH];
    U32  Var;

    /* Default init */
    memset(&Capability, 0xff, sizeof(STDENC_Capability_t));

    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }

    sprintf( DNC_Msg, "DENC_GetCapability(%s): ", DeviceName);
    ErrCode = STDENC_GetCapability(DeviceName, &Capability);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf( DNC_Msg, "\tMode\t\tValue\tSquare\tInter\tFR60Hz\n");
        strcat(  DNC_Msg, "\t------------------------------------------------\n");
        for(Var=0; Var<DENC_MAX_MODE; Var++){
            if(Capability.Modes & (1<<Var))
                sprintf(DNC_Msg, "%s\t%-11.11s\t %3d\t%s\t%s\t%s\n", DNC_Msg, DencModeName[Var].Mode, Var, \
                        (Capability.SquarePixel & (1<<Var)) ? "TRUE" : "FALSE",
                        (Capability.Interlaced & (1<<Var)) ? "TRUE" : "FALSE",
                        (Capability.FieldRate60Hz & (1<<Var)) ? "TRUE" : "FALSE");
        }
        strcat(DNC_Msg, "\n");
        sprintf(DNC_Msg, "%s\tCellID=%d\n\tChroma: min=%d max=%d step=%d\n", DNC_Msg,
                Capability.CellId, Capability.MinChromaDelay, Capability.MaxChromaDelay, Capability.StepChromaDelay);

        sprintf( StrParams, "%d %d %d %d %d %d %d %d", \
                 Capability.Modes, Capability.SquarePixel, Capability.Interlaced, Capability.FieldRate60Hz,
                  Capability.CellId, Capability.MinChromaDelay, Capability.MaxChromaDelay, Capability.StepChromaDelay);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        DENC_PRINT(DNC_Msg);
    }
    return(API_EnableError ? RetErr: FALSE);
} /* end STDENC_GetCapability */


/*-------------------------------------------------------------------------
 * Function : DENC_SetModeOption
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_SetModeOption( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STDENC_ModeOption_t ModeOption;
    S32 Var,i;
    char TrvStr[80], IsStr[80], *ptr;

    /* Default init */
    memset(&ModeOption, 0xff, sizeof(STDENC_ModeOption_t));

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "LEVEL_PEDESTAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<DENC_MAX_OPTION_MODE; Var++){
        if((strcmp(DencOptionMode[Var].Mode, TrvStr)==0) || \
           (strncmp(DencOptionMode[Var].Mode, TrvStr+1, strlen(DencOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==DENC_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr==TRUE){
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Option (default LEVEL_PEDESTAL)");
                sprintf(DNC_Msg, "Other options: CHROMA_DELAY, TRAP_FILTER, RATE_60HZ, "
                        "NON_INTERLACED, SQUARE_PIXEL, YCBCR444_INPUT \n");
                DENC_PRINT(DNC_Msg);
                return(TRUE);
            }
        }
    }
    ModeOption.Option = (STDENC_Option_t)Var;

    /* Get value */
    RetErr = STTST_GetInteger( pars_p, FALSE, &Var);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected boolean (default FALSE)\n");
        return(TRUE);
    }

    for(i=0;i<DENC_MAX_OPTION_MODE;i++)
        if(DencOptionMode[i].Value == ModeOption.Option)
            break;

    if(ModeOption.Option == STDENC_OPTION_CHROMA_DELAY){
        ModeOption.Value.ChromaDelay = (S8) Var;
        sprintf( DNC_Msg, "DENC_SetModeOption(%d,Opt=%s,Val=%d): ", DENCHndl,
                 DencOptionMode[i].Mode, ModeOption.Value.ChromaDelay);
    }
    else{
        /* Permited for all other option of the same type !!!*/
        ModeOption.Value.YCbCr444Input = (BOOL) Var;
        sprintf( DNC_Msg, "DENC_SetModeOption(%d,Opt=%s,Val=%d): ", DENCHndl,
                 DencOptionMode[i].Mode, ModeOption.Value.YCbCr444Input);
    }

    ErrCode = STDENC_SetModeOption(DENCHndl, &ModeOption);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    return(API_EnableError ? RetErr: FALSE);
} /* end STDENC_SetModeOption */


/*-------------------------------------------------------------------------
 * Function : DENC_GetModeOption
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_GetModeOption( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STDENC_ModeOption_t ModeOption;
    S32 Var, i;
    char StrParams[RETURN_PARAMS_LENGTH];
    char TrvStr[80], IsStr[80], *ptr;

    /* Default init */
    memset(&ModeOption, 0xff, sizeof(STDENC_ModeOption_t));

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "LEVEL_PEDESTAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(RetErr==FALSE){
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<DENC_MAX_OPTION_MODE; Var++){
        if((strcmp(DencOptionMode[Var].Mode, TrvStr)==0) || \
           (strncmp(DencOptionMode[Var].Mode, TrvStr+1, strlen(DencOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==DENC_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr==TRUE){
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Option (default LEVEL_PEDESTAL)");
                sprintf(DNC_Msg, "Other options: CHROMA_DELAY, TRAP_FILTER, RATE_60HZ, "
                        "NON_INTERLACED, SQUARE_PIXEL, YCBCR444_INPUT \n");
                DENC_PRINT(DNC_Msg);
                return(TRUE);
            }
        }
    }
    ModeOption.Option = (STDENC_Option_t)Var;

    for(i=0;i<DENC_MAX_OPTION_MODE;i++)
        if(DencOptionMode[i].Value == ModeOption.Option)
            break;
    RetErr = FALSE;

    sprintf( DNC_Msg, "DENC_GetModeOption(%d,Opt=%s): ", DENCHndl, DencOptionMode[i].Mode);
    ErrCode = STDENC_GetModeOption(DENCHndl, &ModeOption);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    if(ErrCode == ST_NO_ERROR)
    {
        if(ModeOption.Option == STDENC_OPTION_CHROMA_DELAY)
        {
            sprintf( DNC_Msg, "\tValue = %d\n", ModeOption.Value.ChromaDelay);
            sprintf( StrParams, "%d", ModeOption.Value.ChromaDelay);
        }
        else
        {
            /* Permited for all other option of the same type !!!*/
            sprintf( DNC_Msg, "\tValue = %s\n",
                     (ModeOption.Value.YCbCr444Input == TRUE) ? "TRUE" :
                     (ModeOption.Value.YCbCr444Input == FALSE) ? "FALSE" :
                     "????"
                );
            sprintf( StrParams, "%d", ModeOption.Value.YCbCr444Input);
        }
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        DENC_PRINT(DNC_Msg);
    }
    return(API_EnableError ? RetErr: FALSE);
} /* end STDENC_GetModeOption */


/*-------------------------------------------------------------------------
 * Function : DENC_ColorBarPattern
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_ColorBarPattern( STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    U32 LVar;
    BOOL RetErr;

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }

    /* get test mode  (default: 1= Color bar on, 0= Off) */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine(pars_p, "expected  ColorBar mode (On=1, Off=0)\n");
        return(TRUE);
    }

    sprintf( DNC_Msg, "DENC_SetColorBarPattern(%d): ", DENCHndl);
    ErrCode = STDENC_SetColorBarPattern(DENCHndl, (BOOL)LVar);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    if ( ErrCode == ST_NO_ERROR )
    {
        sprintf( DNC_Msg, "\tColor bar pattern : %s \n", (LVar == 0) ? "Off": "On");
        DENC_PRINT(DNC_Msg);
    }
    return(API_EnableError ? RetErr: FALSE);
}  /* end DENC_ColorBarPattern */


/*-------------------------------------------------------------------------
 * Function : DENC_ReadConfigurationRegister
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_ReadConfigurationRegister( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U8   Index;

    RetErr = denc_read(0, Denc_Register ,9);
    if(RetErr == FALSE){

        sprintf( DNC_Msg, "DENC Configuration Register in Hexa :\nAddress");
        sprintf( DNC_Msg, "%s  00  01  02  03  04  05  06  07  08\nValues ", DNC_Msg);
        for (Index=0; Index<=8; Index++)
        {
            sprintf( DNC_Msg, "%s  %02X", DNC_Msg, Denc_Register[Index]);
        }
        strcat( DNC_Msg, "\n");
        DENC_PRINT(DNC_Msg);
    }
    else
        STTST_Print(("DENC_RegCFG(): Error occured!!!\n"));

    return(FALSE);
} /* end DENC_ReadConfigurationRegister */


/*-------------------------------------------------------------------------
 * Function : DENC_CompareRegisterr
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_CompareRegister( STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 Index, LineFeed=0;
    U8 Denc_RegTmp[100];
    BOOL RetErr;

    RetErr = denc_read(0, Denc_RegTmp, 91);
    if(RetErr == FALSE){
        sprintf( DNC_Msg, "DENC register comparaison in Hexa Add:Value\n");
        for (Index=0; Index<=91; Index++)
        {
            if(Denc_RegTmp[Index] != Denc_Register[Index]){
                sprintf( DNC_Msg, "%s %02d:h%02X", DNC_Msg, Index,  Denc_RegTmp[Index]);
                Denc_Register[Index] = Denc_RegTmp[Index];
                LineFeed++;
                if(LineFeed > 10){
                    strcat( DNC_Msg, "\n");
                    DENC_PRINT(DNC_Msg);
                    LineFeed=0; DNC_Msg[0] = 0;
                }
            }
        }
        strcat( DNC_Msg, "\n");
        DENC_PRINT(DNC_Msg);
    }
    else
        STTST_Print(("DENC_CmpReg(): Error occured!!!\n"));

    return (FALSE);

} /* end DENC_CompareRegister */


/*-------------------------------------------------------------------------
 * Function : Read_55XX_dnc_regs
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL Read_dnc_regs(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 RegAdr;
    U8  RegVal;
    BOOL Error = FALSE;
    char StrParams[RETURN_PARAMS_LENGTH];


    Error = STTST_GetInteger(pars_p, 1, (S32*)&RegAdr);
    if(Error == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected address of denc register\n");
        return(TRUE);
    }
    Error = denc_read(RegAdr, &RegVal, 1);
    if(Error == TRUE)
        sprintf( DNC_Msg, "An error occured !!!\n");
    else
    {
        sprintf( DNC_Msg, "Add(%02d=h%02X)= h%02X\n", RegAdr, RegAdr, RegVal);
        sprintf(StrParams, "%d", RegVal);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }

    DENC_PRINT(DNC_Msg);
    return(FALSE);
}

/*-------------------------------------------------------------------------
 * Function : Write_55XX_dnc_regs




 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL Write_dnc_regs(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U8  RegVal[2];
    BOOL Error = FALSE;
    S32 Var;

    Error = STTST_GetInteger (pars_p, 0, &Var);
    if(Error == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected address of denc register\n");
        return(TRUE);
    }
    RegVal[0] = (U8)Var;

    Error = STTST_GetInteger (pars_p, 0, &Var);
    if(Error == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected value of denc register\n");
        return(TRUE);
    }
    RegVal[1] = (U8)Var;

    Error = denc_write(RegVal[0], &RegVal[1], 1);
    if(Error == TRUE)
        sprintf( DNC_Msg, "An error occured !!!\n");
    else
        sprintf( DNC_Msg, "Add(%02d=h%02X)= h%02X\n", RegVal[0], RegVal[0], RegVal[1]);
    DENC_PRINT(DNC_Msg);
    return(FALSE);
}


/*-------------------------------------------------------------------------
 * Function : DENC_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_InitCommand(void)
{
    BOOL RetErr = FALSE;
    /* API functions : */
    RetErr  = STTST_RegisterCommand( "DENCInit", DENC_Init, "<DeviceName><DeviceType> Init denc");
    RetErr |= STTST_RegisterCommand( "DENCOpen", DENC_Open, "<Handle> Open denc");
    RetErr |= STTST_RegisterCommand( "DENCClose", DENC_Close, "<Handle> Close denc");
    RetErr |= STTST_RegisterCommand( "DENCTerm", DENC_Term, "<DeviceName> Term denc");
    RetErr |= STTST_RegisterCommand( "DENCRev", DENC_GetRevision, "Get denc revision");
    RetErr |= STTST_RegisterCommand( "DENCSetMode", DENC_SetEncodingMode, "<Handle><Mode> (See capabilities for more info)");
    RetErr |= STTST_RegisterCommand( "DENCGetMode", DENC_GetEncodingMode, "<Handle> Get encoding mode ");
    RetErr |= STTST_RegisterCommand( "DENCCBP", DENC_ColorBarPattern, "<Handle><0|1> Display a color bar pattern");
    RetErr |= STTST_RegisterCommand( "DENCCapa", DENC_GetCapability, "<DeviceName> DENC Get Capability");
    RetErr |= STTST_RegisterCommand( "DENCSOption", DENC_SetModeOption, "<Handle><Option><Value>Option parameter");
    RetErr |= STTST_RegisterCommand( "DENCGOption", DENC_GetModeOption, "<Handle><Option> Get optiond parameter");
    /* Utilities : */
    RetErr |= STTST_RegisterCommand( "DENCCFG", DENC_ReadConfigurationRegister, "Read Configuration Register");
    RetErr |= STTST_RegisterCommand( "DENCCMP", DENC_CompareRegister, "Compare Configuration Register");
    RetErr |= STTST_RegisterCommand( "DENCRead", Read_dnc_regs, "<Add> Read a Sti5510 denc register");
    RetErr |= STTST_RegisterCommand( "DENCWrite", Write_dnc_regs, "<Add><Value> Write to a Sti5510 denc register");

    /* Constants */
    RetErr |= STTST_AssignInteger ("ERR_DENC_INVALID_ENCODING_MODE", STDENC_ERROR_INVALID_ENCODING_MODE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_DENC_I2C", STDENC_ERROR_I2C, TRUE);

    /* Variable constant */
    RetErr |= STTST_AssignInteger ("DENC_TYPE", DENC_DEVICE_TYPE, FALSE);
    RetErr |= STTST_AssignInteger ("DENC_ACCESS", DENC_DEVICE_ACCESS, FALSE);
    RetErr |= STTST_AssignString  ("DENC_NAME", STDENC_DEVICE_NAME, FALSE);

    return( RetErr ? FALSE : TRUE);
} /* end DENC_MacroInit */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL DENC_RegisterCmd()
{
    BOOL RetOk;

    RetOk = DENC_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "DENC_Command()     \t: ok           %s\n", STDENC_GetRevision()  ));
    }
    else
    {
        STTBX_Print(( "DENC_Command()     \t: failed !\n" ));
    }

    return( RetOk );
}
/* end of file */

