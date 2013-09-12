/*******************************************************************************

File name   : denc_cmd.c

Description : STDENC commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                           HSdLM
30 Oct 2002        Add 5517 support                                  HSdLM
06 May 2003        Add 5528 and 7020 STEM support                    HSdLM
21 Aug 2003        Add 5100/mb390 support                            HSdLM
27 Feb 2004        Add new STDENC_DEVICE_TYPE_V13 for STi5100        MH
05 Jul 2004        Add 7710/mb391 support                            TA
22 Sep 2004        Add ST40/OS21 support                             MH
01 Dec 2004        Add 5105/mb400 support                            TA
*******************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stsys.h"

#include "startup.h"
#include "stdenc.h"
#include "stddefs.h"
#include "stdevice.h"

#if defined(ST_OSLINUX)
#include "iocstapigat.h"
/*#else*/
#endif
#include "sttbx.h"

#include "testtool.h"
#include "api_cmd.h"
#include "denc_cmd.h"

#include "testcfg.h"

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#include "cli2c.h"
#include "sti2c.h"
#endif  /* #ifdef USE_I2C */

#ifdef USE_VTGSIM
#include "simvtg.h"
#endif /*#ifdef USE_VTGSIM*/

#ifdef USE_VOUTSIM
#include "simvout.h"
#endif /*#ifdef USE_VOUTSIM */
#include "../../dvdgr-prj-stdenc/src/hal/reg/denc_reg.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants --- */
#define STRING_DEVICE_LENGTH   80
#define RETURN_PARAMS_LENGTH   50

#define DENC_MAX_MODE           10
#define DENC_MAX_OPTION_MODE    10
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#define I2C_STV119_ADDRESS   0x40
#define I2C_TIMEOUT          1000
#endif  /* USE_I2C USE_STDENC_I2C */

/* Private type */
enum{
    DENC_ACCESS_8BITS  = STDENC_EMI_ACCESS_WIDTH_8_BITS,
    DENC_ACCESS_32BITS = STDENC_EMI_ACCESS_WIDTH_32_BITS,
    DENC_ACCESS_I2C
};

#if defined (ST_5508) || defined (ST_5518) || defined (ST_5510) || defined (ST_5512) || \
    defined (ST_5516) || ((defined (ST_5514) || defined (ST_5517)) && !defined(ST_7020))
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_8_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif (defined (ST_5528) && !defined(ST_7020))
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_5188) || defined(ST_5107)||defined(ST_5162)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_V13
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined (ST_5525)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_V13
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC1_BASE_ADDRESS        DENC_1_BASE_ADDRESS
#define DNC_BASE_ADDRESS         DENC_0_BASE_ADDRESS
#define DENC_REG_SHIFT           0

#elif defined (ST_7015)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7015
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  STI7015_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7015_DENC_OFFSET
#define DENC_REG_SHIFT           0
#elif defined (ST_7020)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_7020
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  STI7020_BASE_ADDRESS
#define DNC_BASE_ADDRESS         ST7020_DENC_OFFSET
#define DENC_REG_SHIFT           0
#if defined (ST_5528)
    #define DNC_5528_DEVICE_BASE_ADDRESS  0
    #define DNC_5528_BASE_ADDRESS         ST5528_DENC_BASE_ADDRESS
#endif /* defined (ST_5528) */
#elif defined (ST_GX1)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_GX1
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         STGX1_DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           1
#elif defined (ST_7710)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7710_DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined (ST_7100)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7100_DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined (ST_7109)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_DENC
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7109_DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0
#elif defined (ST_7200)
#define DENC_DEVICE_TYPE         STDENC_DEVICE_TYPE_V13
#define DENC_DEVICE_ACCESS       STDENC_ACCESS_TYPE_EMI
#define DENC_DEVICE_ACCESS_WIDTH STDENC_EMI_ACCESS_WIDTH_32_BITS
#define DNC_DEVICE_BASE_ADDRESS  0
#define DNC_BASE_ADDRESS         ST7200_DENC_BASE_ADDRESS
#define DENC_REG_SHIFT           0

#else
#error Not defined yet
#endif

#if defined (mb376) || defined (espresso)

    #ifdef ST_OSLINUX
    #define ST4629_BASE_ADDRESS STI4629_ST40_BASE_ADDRESS
    #endif /* ST_OSLINUX */

    #ifdef ST_OS21
    #define ST4629_BASE_ADDRESS STI4629_ST40_BASE_ADDRESS
    #endif /* ST_OS21 */

    #ifdef ST_OS20
    #define ST4629_BASE_ADDRESS  STI4629_BASE_ADDRESS
    #endif /* ST_OS20 */

    #include "sti4629.h"
    #define STI4629_DNC_BASE_ADDRESS ST4629_DENC_OFFSET
#endif /* mb376 || espresso*/

#if defined (ST_7100)|| defined (ST_7109)
#ifdef ST_OSLINUX
extern MapParams_t   OutputStageMap;
extern MapParams_t   CompoMap;
extern MapParams_t   ClockMap;
#define VOS_BASE_ADDRESS_1        OutputStageMap.MappedBaseAddress
#define VMIX1_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xC00
#define VMIX2_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xD00
#define CKG_BASE_ADDRESS_1         ClockMap.MappedBaseAddress
#else  /** ST_OSLINUX **/
#define VOS_BASE_ADDRESS_1       VOS_BASE_ADDRESS
#define VMIX1_BASE_ADDRESS       VMIX1_LAYER_BASE_ADDRESS
#define VMIX2_BASE_ADDRESS       VMIX2_LAYER_BASE_ADDRESS
#define CKG_BASE_ADDRESS_1       CKG_BASE_ADDRESS
#endif
#elif defined (ST_7200)
#ifdef ST_OSLINUX
extern MapParams_t   OutputStageMap;
extern MapParams_t   CompoMap;
extern MapParams_t   ClockMap;
#define VOS_BASE_ADDRESS_1        OutputStageMap.MappedBaseAddress
#define VMIX1_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xC00
#define VMIX2_BASE_ADDRESS       CompoMap.MappedBaseAddress + 0xD00
#define CKG_BASE_ADDRESS_1         ClockMap.MappedBaseAddress
#else
#define VOS_BASE_ADDRESS_1       VOS_BASE_ADDRESS
#define VMIX1_BASE_ADDRESS       VMIX1_LAYER_BASE_ADDRESS
#define VMIX2_BASE_ADDRESS       VMIX2_LAYER_BASE_ADDRESS
#define CKG_BASE_ADDRESS_1       CKG_BASE_ADDRESS
#endif
#endif

/* ---------------- Global variables ------------ */

/* --------------- Macros ----------------------*/

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
    const char Mode[20];
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
static U8 DencAccessType = DENC_DEVICE_ACCESS_WIDTH;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)

static STI2C_Handle_t I2CHndl=-1;
#endif  /* #ifdef USE_I2C */

static const denc_Mode_t DencModeName[DENC_MAX_MODE + 1] = {
    {"NONE", STDENC_MODE_NONE },
    {"NTSCM", STDENC_MODE_NTSCM },
    {"NTSCM_J", STDENC_MODE_NTSCM_J },
    {"NTSCM_443", STDENC_MODE_NTSCM_443 },
    {"PAL", STDENC_MODE_PALBDGHI },
    {"PALM", STDENC_MODE_PALM },
    {"PALN", STDENC_MODE_PALN },
    {"PALN_C", STDENC_MODE_PALN_C },
    {"SECAM", STDENC_MODE_SECAM },
    {"SECAM_AUX", STDENC_MODE_SECAM_AUX },
    { "????", 0}
};

static const denc_ModeOption_t DencOptionMode[DENC_MAX_OPTION_MODE + 1] = {
    { "LEVEL_PEDESTAL", STDENC_OPTION_BLACK_LEVEL_PEDESTAL},
    { "LEVEL_PEDESTAL_AUX", STDENC_OPTION_BLACK_LEVEL_PEDESTAL_AUX},
    { "CHROMA_DELAY", STDENC_OPTION_CHROMA_DELAY},
    { "CHROMA_DELAY_AUX", STDENC_OPTION_CHROMA_DELAY_AUX},
    { "TRAP_FILTER", STDENC_OPTION_LUMA_TRAP_FILTER},
    { "TRAP_FILTER_AUX", STDENC_OPTION_LUMA_TRAP_FILTER_AUX},
    { "RATE_60HZ", STDENC_OPTION_FIELD_RATE_60HZ},
    { "NON_INTERLACED", STDENC_OPTION_NON_INTERLACED},
    { "SQUARE_PIXEL", STDENC_OPTION_SQUARE_PIXEL},
    { "YCBCR444_INPUT", STDENC_OPTION_YCBCR444_INPUT},
    { "????", 0}
};

/* Prototype */
    BOOL DENC_GetEncodingMode( STTST_Parse_t *pars_p, char *result_sym_p );


/* Local prototypes ------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : denc_read
 * Input    : Address, length, buffer
 * Output   : None
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL denc_read(U8 add, U8* value, U8 lenght, STDENC_Handle_t DENC_Hndl)
{
    U8 i;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    U32                 ActLen;
#endif  /* #ifdef USE_I2C */                           /* ((DENC_t *)DENC_Hndl)->Device_p->BaseAddress_p*/

    switch(DencAccessType)
    {
        case DENC_ACCESS_32BITS:
            for(i=0; i<lenght; i++, value++)
            {
                ErrCode |= STDENC_ReadReg8(DENC_Hndl, (i+add)<<DENC_REG_SHIFT, value );
            }
            if(ErrCode != ST_NO_ERROR)
            {
                STTST_Print(("Failed to read registers\n"));
                return(TRUE);
            }
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
                ErrCode |= STDENC_ReadReg8(DENC_Hndl, add+i, value);
           if(ErrCode != ST_NO_ERROR)
           {
                STTST_Print(("Failed to read registers\n"));
                return(TRUE);
           }
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
static BOOL denc_write(U8 add, U8* value, U8 lenght, STDENC_Handle_t DENC_Hndl)
{
    U8 i;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
#if defined (USE_I2C) && defined (USE_STDENC_I2C)

    U32                 ActLen;
    U8                  SavValue;
#endif  /* #ifdef USE_I2C */

    switch(DencAccessType)
    {
        case DENC_ACCESS_32BITS:
            for(i=0; i<lenght; i++, value++)
            {
                ErrCode |= STDENC_WriteReg8(DENC_Hndl, (i+add)<<DENC_REG_SHIFT, *value);
            }
            if(ErrCode != ST_NO_ERROR){
                STTST_Print(("Failed to write in registers\n"));
                return(TRUE);
            }
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
            {
                ErrCode |= STDENC_WriteReg8(DENC_Hndl, add+i, *value);
            }
            if(ErrCode != ST_NO_ERROR){
                STTST_Print(("Failed to write in registers\n"));
                return(TRUE);
            }
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
 * Input    : ST_ErrorCode_t, char *
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
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_Init( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_InitParams_t DENCInitParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Var;

    UNUSED_PARAMETER(result_sym_p);
    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }

    /* Get Max Open */
    RetErr = STTST_GetInteger(pars_p, 5, (S32*)&Var);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected max open (default 5)");
        return(TRUE);
    }
    DENCInitParams.MaxOpen = Var;

    /* Get Device Type */
    RetErr = STTST_GetInteger(pars_p, DENC_DEVICE_TYPE, (S32*)&Var);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected device type (0=DENC,1=7015,2=7020,3=GX1,4=4629,5=5100)");
        return(TRUE);
    }
    DENCInitParams.DeviceType = (STDENC_DeviceType_t)Var;

    /* Get Access Type */
    RetErr = STTST_GetInteger(pars_p, DENC_DEVICE_ACCESS, (S32*)&Var);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected Access type (0=RBUS,1=I2C)");
        return(TRUE);
    }
    #if defined (ST_7200)
        DENCInitParams.AccessType = STDENC_ACCESS_TYPE_EMI;
    #else
        DENCInitParams.AccessType = (STDENC_AccessType_t)Var;
    #endif
    /* Get Base Address */
    RetErr = STTST_GetInteger( pars_p, DNC_BASE_ADDRESS, &Var);
    if (RetErr)
    {
         STTST_TagCurrentLine( pars_p, "Expected base address");
         return(TRUE);
    }
    if (DENCInitParams.AccessType == STDENC_ACCESS_TYPE_EMI)
    {
#if ((defined (mb376) && !defined(ST_7020)) || defined (espresso))
        if ( DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_4629 )
        {
            DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)ST4629_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.BaseAddress_p = (void *)STI4629_DNC_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.Width = STDENC_EMI_ACCESS_WIDTH_8_BITS;
            DencAccessType=DENC_ACCESS_8BITS;
        }
        else
        {
            DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_DEVICE_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.BaseAddress_p       = (void *)DNC_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.Width               = DENC_DEVICE_ACCESS_WIDTH;
            DencAccessType                                       = DENC_DEVICE_ACCESS_WIDTH;
        }
#elif (defined (ST_5528) && defined(ST_7020))
        if ( DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_4629 )
        {
            DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)STI4629_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.BaseAddress_p       = (void *)STI4629_DNC_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.Width               = STDENC_EMI_ACCESS_WIDTH_8_BITS;
            DencAccessType                                       = DENC_ACCESS_8BITS;
        }
        else
        {
            if ( DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_DENC )
            {
                DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_5528_DEVICE_BASE_ADDRESS;
                DENCInitParams.STDENC_Access.EMI.BaseAddress_p       = (void *)DNC_5528_BASE_ADDRESS;
            }
            else
            {
                DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_DEVICE_BASE_ADDRESS;
                DENCInitParams.STDENC_Access.EMI.BaseAddress_p       = (void *)DNC_BASE_ADDRESS;
            }
        }
        DENCInitParams.STDENC_Access.EMI.Width   = DENC_DEVICE_ACCESS_WIDTH;
        DencAccessType                           = DENC_DEVICE_ACCESS_WIDTH;
#else
            DENCInitParams.STDENC_Access.EMI.DeviceBaseAddress_p = (void *)DNC_DEVICE_BASE_ADDRESS;
            DENCInitParams.STDENC_Access.EMI.BaseAddress_p       = (void *)Var;
            DENCInitParams.STDENC_Access.EMI.Width               = DENC_DEVICE_ACCESS_WIDTH;

            DencAccessType                                       = DENC_DEVICE_ACCESS_WIDTH;
#endif /* mb376 || espresso*/
    }
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
    else /* I2C driver for STV0119 on mb295 */
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
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_4629) ? "4629" :
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_7015) ? "7015" :
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_7020) ? "7020" :
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_V13) ? "DENCV13" :
#if defined(ST_GX1) || defined(ST_NGX1)
             (DENCInitParams.DeviceType == STDENC_DEVICE_TYPE_GX1) ? "GX1" :
#endif
             "????",
             (DENCInitParams.AccessType == STDENC_ACCESS_TYPE_EMI) ? "RBUS" :
             (DENCInitParams.AccessType == STDENC_ACCESS_TYPE_I2C) ? "I2C"
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
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_Open( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_OpenParams_t DENCOpenParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t      ErrCode;
#ifdef USE_VOUTSIM
    BOOL IsOutputAux = FALSE;
#endif /*#ifdef USE_VOUTSIM */
#if defined(USE_VTGSIM) || defined(USE_VOUTSIM)
    BOOL IsDencExternal = FALSE;
#endif /*#ifdef USE_VTGSIM */

    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }
#ifdef USE_VOUTSIM
    RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&IsOutputAux);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected number\n");
        return(TRUE);
    }
#endif
#if defined(USE_VTGSIM) || defined(USE_VOUTSIM)

    RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&IsDencExternal);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected number\n");
        return(TRUE);
    }
#endif
    sprintf( DNC_Msg, "DENC_Open(%s): ", DeviceName);
    ErrCode = STDENC_Open(DeviceName, &DENCOpenParams, &DENCHndl);
    RetErr |= DENC_TextError(ErrCode, DNC_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
#ifdef USE_VTGSIM
        /* Master for STi55xx and Slave if external Denc :*/
        VtgSimInit(DENCHndl, IsDencExternal ? VTGSIM_DEVICE_TYPE_0 : VTGSIM_DEVICE_TYPE);
#if defined(ST_7200)
        RetErr = VtgSimSetConfiguration(DENCHndl,
                                        IsDencExternal ? VTGSIM_DEVICE_TYPE_0 : VTGSIM_DEVICE_TYPE ,
                                        IsDencExternal ? VTGSIM_TIMING_MODE_SLAVE : VTGSIM_TIMING_MODE_480I59940_13500 ,
                                        2);
#else
        RetErr = VtgSimSetConfiguration(DENCHndl,
                                        IsDencExternal ? VTGSIM_DEVICE_TYPE_0 : VTGSIM_DEVICE_TYPE ,
                                        IsDencExternal ? VTGSIM_TIMING_MODE_SLAVE : VTGSIM_TIMING_MODE_480I59940_13500 ,
                                        1);
#endif
#endif /*#ifdef USE_VTGSIM */
#ifdef USE_VOUTSIM
#if defined (mb376) || defined (espresso)
        VoutSimDencOut(DENCHndl, IsDencExternal ? STDENC_DEVICE_TYPE_4629 : DENC_DEVICE_TYPE, VOUTSIM_OUTPUT_CVBS,
                       IsOutputAux ? VOUTSIM_SELECT_OUT_AUX : VOUTSIM_SELECT_OUT_MAIN);

        VoutSimSetViewport(TRUE,IsOutputAux ? VOUTSIM_SELECT_OUT_AUX : VOUTSIM_SELECT_OUT_MAIN);   /* Set viewport params to NTSC by default */
#else
        VoutSimDencOut(DENCHndl, DENC_DEVICE_TYPE, VOUTSIM_OUTPUT_CVBS,
                       IsOutputAux ? VOUTSIM_SELECT_OUT_AUX : VOUTSIM_SELECT_OUT_MAIN);
#endif /* mb376 || espresso*/
#endif /*#ifdef USE_VOUTSIM */

        sprintf( DNC_Msg, "\tHandle=%d\n", DENCHndl);
        DENC_PRINT(DNC_Msg);
        STTST_AssignInteger( result_sym_p, DENCHndl, FALSE);
    }

    return(API_EnableError ? RetErr: FALSE);
}


/*-------------------------------------------------------------------------
 * Function : DENC_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_Close( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    ST_ErrorCode_t      ErrCode;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (RetErr)
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
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_Term( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_TermParams_t DENCTermParams;
    char DeviceName[STRING_DEVICE_LENGTH];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    S32 Var;

    UNUSED_PARAMETER(result_sym_p);
    /* get device */
    RetErr = STTST_GetString( pars_p, STDENC_DEVICE_NAME, DeviceName, sizeof(DeviceName) );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected DeviceName\n");
        return(TRUE);
    }

    /* Get force terminate */
    RetErr = STTST_GetInteger( pars_p, FALSE, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected force terminate (default FALSE)");
        return(TRUE);
    }
    DENCTermParams.ForceTerminate = (BOOL)Var;

    sprintf( DNC_Msg, "DENC_Term(%s, Force=%s): ",
             DeviceName,
             (DENCTermParams.ForceTerminate) ? "TRUE" : "FALSE");
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

    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
    RevisionNumber = STDENC_GetRevision();
    sprintf( DNC_Msg, "DENC_GetRevision(): Ok\nrevision=%s\n", RevisionNumber );
    DENC_PRINT(DNC_Msg);
    return ( FALSE );
} /* end DNC_GetRevision */


/*-------------------------------------------------------------------------
 * Function : DENC_SetEncodingMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_SetEncodingMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STDENC_EncodingMode_t ModeDenc;
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    U32 Var;
    char *ptr=NULL, TrvStr[80], IsStr[80];
#ifdef USE_VOUTSIM
    BOOL IsNTSC = FALSE;
    U32  IsOutputAux;
#endif /* USE_VOUTSIM */

    UNUSED_PARAMETER(result_sym_p);
    /* Default init */
    memset(&ModeDenc, 0xff, sizeof(STDENC_EncodingMode_t));

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    /* get output mode  (default: PAL) */
    STTST_GetItem( pars_p, "PAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<DENC_MAX_MODE; Var++){
        if((strcmp(DencModeName[Var].Mode, TrvStr)==0) ||
           (strncmp(DencModeName[Var].Mode, TrvStr+1, strlen(DencModeName[Var].Mode))==0))
            break;
    }

    if(Var==DENC_MAX_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected encoding mode (See capabilities)");
                return(TRUE);
            }
        }
    }
    ModeDenc.Mode = (STDENC_Mode_t)Var;

    if((ModeDenc.Mode != STDENC_MODE_SECAM) && (ModeDenc.Mode != STDENC_MODE_SECAM_AUX) && (ModeDenc.Mode != STDENC_MODE_NONE)){
        /* Get square pixel */
        RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&Var);
        if (RetErr)
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

            default:
                break;
         }

        /* Get interlaced */
        RetErr = STTST_GetInteger( pars_p, TRUE, (S32*)&Var);
        if (RetErr)
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
            default:
                break;
        }

         switch(ModeDenc.Mode){

            case STDENC_MODE_NTSCM:
            case STDENC_MODE_NTSCM_J:
            case STDENC_MODE_NTSCM_443:
                /* Get FR60Hz */
                RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&Var);
                if (RetErr)
                {
                    STTST_TagCurrentLine( pars_p, "Expected Field rate 60Hz (default=FALSE)");
                    return(TRUE);
                }
                ModeDenc.Option.Ntsc.FieldRate60Hz = (BOOL)Var;
                break;
            default :
                break;
        }
    }

#if defined (USE_VOUTSIM)
        /* Get AUX or MAIN output */
        RetErr = STTST_GetInteger( pars_p, FALSE, (S32*)&IsOutputAux);
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "Expected AUX/MAIN (default=MAIN)");
            return(TRUE);
        }
#endif

  switch(ModeDenc.Mode){
        case STDENC_MODE_NTSCM:
        case STDENC_MODE_NTSCM_J:
        case STDENC_MODE_NTSCM_443:
#ifdef USE_VOUTSIM
            IsNTSC = TRUE;
#endif /* USE_VOUTSIM */
            sprintf( DNC_Msg, "DENC_SetEncodingMode(%d,Mode=%s,SQ=%s,Inter=%s,FR60Hz=%s): ", DENCHndl,
                     (ModeDenc.Mode < DENC_MAX_MODE) ? DencModeName[ModeDenc.Mode].Mode : "????",
                     (ModeDenc.Option.Ntsc.SquarePixel) ? "TRUE" : "FALSE" ,
                     (ModeDenc.Option.Ntsc.Interlaced) ? "TRUE" : "FALSE" ,
                     (ModeDenc.Option.Ntsc.FieldRate60Hz) ? "TRUE" : "FALSE"
                );
            break;

        case STDENC_MODE_PALBDGHI:
        case STDENC_MODE_PALM:
        case STDENC_MODE_PALN:
        case STDENC_MODE_PALN_C:
            sprintf( DNC_Msg, "DENC_SetEncodingMode(%d,Mode=%s,SQ=%s,Inter=%s): ", DENCHndl,
                     (ModeDenc.Mode < DENC_MAX_MODE) ? DencModeName[ModeDenc.Mode].Mode : "????",
                     (ModeDenc.Option.Pal.SquarePixel) ? "TRUE" : "FALSE" ,
                     (ModeDenc.Option.Pal.Interlaced) ? "TRUE" : "FALSE"
                );
            break;

        case STDENC_MODE_SECAM:
        case STDENC_MODE_SECAM_AUX:
        default:
            sprintf( DNC_Msg, "DENC_SetEncodingMode(%d,Mode=%s): ", DENCHndl,
                     (ModeDenc.Mode < DENC_MAX_MODE) ? DencModeName[ModeDenc.Mode].Mode : "????"
                );
            break;
    }

#if (defined(USE_VOUTSIM) && defined(mb376)) ||(defined(USE_VOUTSIM)&& defined(espresso))
     VoutSimSetViewport(IsNTSC,IsOutputAux ? VOUTSIM_SELECT_OUT_AUX : VOUTSIM_SELECT_OUT_MAIN);
#endif /* USE_VOUTSIM && mb376 && espresso*/

    ErrCode = STDENC_SetEncodingMode(DENCHndl, &ModeDenc);
    RetErr = DENC_TextError(ErrCode, DNC_Msg);

    return(API_EnableError ? RetErr: FALSE);
} /* end DENC_SetEncodingMode */


/*-------------------------------------------------------------------------
 * Function : DENC_GetEncodingMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
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
    if (RetErr)
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
                sprintf( DNC_Msg, "\tMode=%s\tSquare=%s Interlaced=%s FR60Hz=%s\n",
                         DencModeName[Mode.Mode].Mode,
                         (Mode.Option.Ntsc.SquarePixel) ? "TRUE" : "FALSE" ,
                         (Mode.Option.Ntsc.Interlaced) ? "TRUE" : "FALSE" ,
                         (Mode.Option.Ntsc.FieldRate60Hz) ? "TRUE" : "FALSE"
                    );
                sprintf( StrParams, "%d %d %d %d",
                         Mode.Mode, Mode.Option.Ntsc.SquarePixel, Mode.Option.Ntsc.Interlaced,
                         Mode.Option.Ntsc.FieldRate60Hz);
                break;

            case STDENC_MODE_PALBDGHI:
            case STDENC_MODE_PALM:
            case STDENC_MODE_PALN:
            case STDENC_MODE_PALN_C:
                sprintf( DNC_Msg, "\tMode=%s\tSquare=%s Interlaced=%s\n",
                         DencModeName[Mode.Mode].Mode,
                         (Mode.Option.Pal.SquarePixel) ? "TRUE" : "FALSE" ,
                         (Mode.Option.Pal.Interlaced) ? "TRUE" : "FALSE"
                    );
                sprintf( StrParams, "%d %d %d",
                         Mode.Mode, Mode.Option.Pal.SquarePixel, Mode.Option.Pal.Interlaced);
                break;

            case STDENC_MODE_SECAM:
            case STDENC_MODE_SECAM_AUX:
            default:
                sprintf( DNC_Msg, "\tMode=%s\n",
                         DencModeName[Mode.Mode].Mode
                    );
                sprintf( StrParams, "%d",
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
 * Return   : TRUE if error, FALSE if success
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
    if (RetErr)
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
                sprintf(DNC_Msg, "%s\t%-11.11s\t %3d\t%s\t%s\t%s\n", DNC_Msg, DencModeName[Var].Mode, Var,
                        (Capability.SquarePixel & (1<<Var)) ? "TRUE" : "FALSE",
                        (Capability.Interlaced & (1<<Var)) ? "TRUE" : "FALSE",
                        (Capability.FieldRate60Hz & (1<<Var)) ? "TRUE" : "FALSE");
        }
        DENC_PRINT(DNC_Msg);
        sprintf(DNC_Msg, "\n\tCellID=%d\n\tChroma: min=%d max=%d step=%d\n",
                Capability.CellId, Capability.MinChromaDelay, Capability.MaxChromaDelay, Capability.StepChromaDelay);
        DENC_PRINT(DNC_Msg);
        sprintf(DNC_Msg, "\tBlackLevelPedestalAux=\t%s\n\tChromaDelayAux=\t\t%s\n\tLumaTrapFilterAux=\t%s\n\tYCbCr444Input=\t\t%s\n",
                Capability.BlackLevelPedestalAux ? "TRUE" : "FALSE",
                Capability.ChromaDelayAux ? "TRUE" : "FALSE",
                Capability.LumaTrapFilterAux ? "TRUE" : "FALSE",
                Capability.YCbCr444Input ? "TRUE" : "FALSE" );

        sprintf( StrParams, "%d %d %d %d %d %d %d %d %d %d %d %d",
                 Capability.Modes, Capability.SquarePixel, Capability.Interlaced, Capability.FieldRate60Hz,
                 Capability.CellId, Capability.MinChromaDelay, Capability.MaxChromaDelay, Capability.StepChromaDelay,
                 Capability.YCbCr444Input, Capability.BlackLevelPedestalAux, Capability.ChromaDelayAux, Capability.LumaTrapFilterAux);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
        DENC_PRINT(DNC_Msg);
    }
    return(API_EnableError ? RetErr: FALSE);
} /* end STDENC_GetCapability */


/*-------------------------------------------------------------------------
 * Function : DENC_SetModeOption
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_SetModeOption( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STDENC_ModeOption_t ModeOption;
    S32 Var,i;
    char TrvStr[80], IsStr[80], *ptr;

    UNUSED_PARAMETER(result_sym_p);
    /* Default init */
    memset(&ModeOption, 0xff, sizeof(STDENC_ModeOption_t));

    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "LEVEL_PEDESTAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<DENC_MAX_OPTION_MODE; Var++){
        if((strcmp(DencOptionMode[Var].Mode, TrvStr)==0) ||
           (strncmp(DencOptionMode[Var].Mode, TrvStr+1, strlen(DencOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==DENC_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Option (default LEVEL_PEDESTAL)");
                sprintf(DNC_Msg, "Other options: LEVEL_PEDESTAL_AUX, CHROMA_DELAY, CHROMA_DELAY_AUX, "
                        "TRAP_FILTER, TRAP_FILTER_AUX, RATE_60HZ, NON_INTERLACED, SQUARE_PIXEL, YCBCR444_INPUT \n");
                DENC_PRINT(DNC_Msg);
                return(TRUE);
            }
        }
    }
    ModeOption.Option = (STDENC_Option_t)Var;

    /* Get value */
    RetErr = STTST_GetInteger( pars_p, FALSE, &Var);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected boolean (default FALSE)\n");
        return(TRUE);
    }

    for(i=0;i<DENC_MAX_OPTION_MODE;i++)
        if(DencOptionMode[i].Value == ModeOption.Option)
            break;

    if (ModeOption.Option == STDENC_OPTION_CHROMA_DELAY)
    {
        ModeOption.Value.ChromaDelay = (S8) Var;
        sprintf( DNC_Msg, "DENC_SetModeOption(%d,Opt=%s,Val=%d): ", DENCHndl,
                 DencOptionMode[i].Mode, ModeOption.Value.ChromaDelay);
    }
    else if (ModeOption.Option == STDENC_OPTION_CHROMA_DELAY_AUX)
    {
        ModeOption.Value.ChromaDelayAux = (S8) Var;
        sprintf( DNC_Msg, "DENC_SetModeOption(%d,Opt=%s,Val=%d): ", DENCHndl,
                 DencOptionMode[i].Mode, ModeOption.Value.ChromaDelayAux);
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
 * Return   : TRUE if error, FALSE if success
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
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    /* Get option */
    STTST_GetItem( pars_p, "LEVEL_PEDESTAL", TrvStr, sizeof(TrvStr));
    RetErr = STTST_EvaluateString( TrvStr, IsStr, sizeof(IsStr));
    if(!RetErr)
    {
        strcpy(TrvStr, IsStr);
    }
    for(Var=0; Var<DENC_MAX_OPTION_MODE; Var++){
        if((strcmp(DencOptionMode[Var].Mode, TrvStr)==0) ||
           (strncmp(DencOptionMode[Var].Mode, TrvStr+1, strlen(DencOptionMode[Var].Mode))==0))
            break;
    }
    if(Var==DENC_MAX_OPTION_MODE){
        RetErr = STTST_EvaluateInteger( TrvStr, (S32*)&Var, 10);
        if(RetErr)
        {
            Var = (U32)strtoul(TrvStr, &ptr, 10);
            if(TrvStr==ptr)
            {
                STTST_TagCurrentLine( pars_p, "Expected Option (default LEVEL_PEDESTAL)");
                sprintf(DNC_Msg, "Other options: LEVEL_PEDESTAL_AUX, CHROMA_DELAY, CHROMA_DELAY_AUX, "
                        "TRAP_FILTER, TRAP_FILTER_AUX, RATE_60HZ, NON_INTERLACED, SQUARE_PIXEL, YCBCR444_INPUT \n");
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
        else if(ModeOption.Option == STDENC_OPTION_CHROMA_DELAY_AUX)
        {
            sprintf( DNC_Msg, "\tValue = %d\n", ModeOption.Value.ChromaDelayAux);
            sprintf( StrParams, "%d", ModeOption.Value.ChromaDelayAux);
        }
        else
        {
            /* Permited for all other option of the same type !!!*/
            sprintf( DNC_Msg, "\tValue = %s\n",
                     (ModeOption.Value.YCbCr444Input) ? "TRUE" : "FALSE"
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
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL DENC_ColorBarPattern( STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    U32 LVar;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    /* Get Handle */
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected Handle\n");
        return(TRUE);
    }

    /* get test mode  (default: 1= Color bar on, 0= Off) */
    RetErr = STTST_GetInteger( pars_p, 1, (S32*)&LVar );
    if (RetErr)
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
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_ReadConfigurationRegister( STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U8   Index;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    RetErr = denc_read(0, Denc_Register, 9, DENCHndl);
    if(!RetErr)
    {

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
 * Return   : FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_CompareRegister( STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 Index, LineFeed=0;
    U8 Denc_RegTmp[100];
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    RetErr = denc_read(0, Denc_RegTmp, 91, DENCHndl );
    if(!RetErr)
    {
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
    {
        STTST_Print(("DENC_CmpReg(): Error occured!!!\n"));
    }

    return (FALSE);

} /* end DENC_CompareRegister */


/*-------------------------------------------------------------------------
 * Function : Read_55XX_dnc_regs
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Read_dnc_regs(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U32 RegAdr;
    U8  RegVal;
    BOOL Error = FALSE;
    char StrParams[RETURN_PARAMS_LENGTH];

    Error = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (Error)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }
    Error = STTST_GetInteger(pars_p, 1, (S32*)&RegAdr);
    if(Error)
    {
        STTST_TagCurrentLine(pars_p, "Expected address of denc register\n");
        return(TRUE);
    }
    Error = denc_read(RegAdr, &RegVal, 1, DENCHndl);
    if(Error)
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
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL Write_dnc_regs(STTST_Parse_t *pars_p, char *result_sym_p)
{
    U8  RegVal[2];
    BOOL Error = FALSE;
    S32 Var;

    UNUSED_PARAMETER(result_sym_p);
    Error = STTST_GetInteger( pars_p, DENCHndl, (S32*)&DENCHndl);
    if (Error)
    {
        STTST_TagCurrentLine( pars_p, "Expected Handle\n");
        return(TRUE);
    }

    Error = STTST_GetInteger (pars_p, 0, &Var);
    if(Error)
    {
        STTST_TagCurrentLine(pars_p, "Expected address of denc register\n");
        return(TRUE);
    }
    RegVal[0] = (U8)Var;

    Error = STTST_GetInteger (pars_p, 0, &Var);
    if(Error)
    {
        STTST_TagCurrentLine(pars_p, "Expected value of denc register\n");
        return(TRUE);
    }
    RegVal[1] = (U8)Var;

    Error = denc_write(RegVal[0], &RegVal[1], 1, DENCHndl);
    if(Error)
        sprintf( DNC_Msg, "An error occured !!!\n");
    else
        sprintf( DNC_Msg, "Add(%02d=h%02X)= h%02X\n", RegVal[0], RegVal[0], RegVal[1]);
    DENC_PRINT(DNC_Msg);
    return(FALSE);
}

#if defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)
/*-------------------------------------------------------------------------
 * Function : DENC_GamSet
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL DENC_GamSet( STTST_Parse_t *pars_p, char *result_sym_p )
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    S32 Mix_Sel;

    UNUSED_PARAMETER(result_sym_p);
    ErrorCode = STTST_GetInteger(pars_p, 1, &Mix_Sel);
    if (ErrorCode)
    {
        STTST_TagCurrentLine( pars_p, "Mix_Sel(1/2)" );
        return(TRUE);
    }
   if(Mix_Sel == 1)
   {
    STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x0, 0x3);
    STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x28, 0x300089);
    STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x2c, 0x26f0358);
    STSYS_WriteRegDev32LE( VMIX1_BASE_ADDRESS + 0x34, 0x40);
    STSYS_WriteRegDev32LE( VOS_BASE_ADDRESS_1 + 0x70, 0x1);

   }
   else
   {
    STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x0, 0x4);
    STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x28, 0x29007f);
    STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x2c, 0x208034e);
    STSYS_WriteRegDev32LE( VMIX2_BASE_ADDRESS + 0x34, 0x0);
    STSYS_WriteRegDev32LE( VOS_BASE_ADDRESS_1 + 0x70, 0x0);
   }

#if defined (ST_7100)
#if 0/*for cut 2.0*/
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x010,0xc0de);

    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x0b8,0x1);
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x014,0x18);
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x018 ,0xf);
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x01c ,0x0);
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x020 ,0x0);
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x024 ,0x1);

    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x0a8 ,0x0);
    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x0a4 ,0x420);

    STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x010,0xc1a0);
#endif
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 +0x0b8,0x1);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x10, 0xc0de);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x1c, 0x1cfe);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x20, 0x1);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0xa8, 0x0);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0xa4 ,0x420);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x10, 0xc1a0);
#elif defined (ST_7109)
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x10, 0xc0de);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x1c, 0x1cfe);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x20, 0x1);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0xa8, 0x0);
   STSYS_WriteRegDev32LE(CKG_BASE_ADDRESS_1 + 0x10, 0xc1a0);

#endif

     return ( API_EnableError ? ErrorCode : FALSE );

   } /* end DENC_GamSet */
#endif /* #ifdef ST_7100 */


#if defined (ST_7200)

/*-----------------01/02/2007 14:14PM----------------
 * Function to dump the DENC1 registers
 * --------------------------------------------------*/
static BOOL DENC1_Dump_Regs(STTST_Parse_t *pars_p, char *result_sym_p )
{
    U32 Value = 0x0;
    BOOL RetErr=FALSE;

    sprintf( DNC_Msg, "---------------------------------------\n");
    sprintf( DNC_Msg, "           DENC1 Registers Dump        \n");
    sprintf( DNC_Msg, "---------------------------------------\n");
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG0));
    sprintf( DNC_Msg, "DENC_CFG0:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG1));
    sprintf( DNC_Msg, "DENC_CFG1:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG2));
    sprintf( DNC_Msg, "DENC_CFG2:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG3));
    sprintf( DNC_Msg, "DENC_CFG3:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG4));
    sprintf( DNC_Msg, "DENC_CFG4:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG5));
    sprintf( DNC_Msg, "DENC_CFG5:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG6));
    sprintf( DNC_Msg, "DENC_CFG6:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG7));
    sprintf( DNC_Msg, "DENC_CFG7:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG8));
    sprintf( DNC_Msg, "DENC_CFG8:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG9));
    sprintf( DNC_Msg, "DENC_CFG9:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG10));
    sprintf( DNC_Msg, "DENC_CFG10:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG11));
    sprintf( DNC_Msg, "DENC_CFG11:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG12));
    sprintf( DNC_Msg, "DENC_CFG12:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CFG13));
    sprintf( DNC_Msg, "DENC_CFG13:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + 0x24/*DENC_STA*/));
    sprintf( DNC_Msg, "DENC_STA:                        0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_IDFS0));
    sprintf( DNC_Msg, "DENC_INDFS0:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_IDFS1));
    sprintf( DNC_Msg, "DENC_INDFS1:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_IDFS2));
    sprintf( DNC_Msg, "DENC_INDFS2:                     0x%x\n", Value);
#if 0
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_PHDFS0));
    sprintf( DNC_Msg, "DENC_PHDFS0:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_PHDFS1));
    sprintf( DNC_Msg, "DENC_PHDFS1:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_WSSMSB));
    sprintf( DNC_Msg, "DENC_WSSMSB:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_WSSLSB));
    sprintf( DNC_Msg, "DENC_WSSLSB:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_DAC13));
    sprintf( DNC_Msg, "DENC_DAC13:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_DAC45));
    sprintf( DNC_Msg, "DENC_DAC45:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_DAC6));
    sprintf( DNC_Msg, "DENC_DAC6:                       0x%x\n", Value);
/*    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + RESERVED20));
    printf("RESERVED20:                       0x%x\n", Value);              */
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LINE0));
    sprintf( DNC_Msg, "DENC_LINE0:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LINE1));
    sprintf( DNC_Msg, "DENC_LINE1:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LINE2));
    sprintf( DNC_Msg, "DENC_LINE2:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_ID));
    sprintf( DNC_Msg, "DENC_ID:                         0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_VPS0));
    sprintf( DNC_Msg, "DENC_VPS0:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_VPS1));
    sprintf( DNC_Msg, "DENC_VPS1:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_VPS2));
    sprintf( DNC_Msg, "DENC_VPS2:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_VPS3));
    sprintf( DNC_Msg, "DENC_VPS3:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_VPS4));
    sprintf( DNC_Msg, "DENC_VPS4:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_VPS5));
    sprintf( DNC_Msg, "DENC_VPS5:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CGMS0));
    sprintf( DNC_Msg, "DENC_CGMS0:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CGMS1));
    sprintf( DNC_Msg, "DENC_CGMS1:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CGMS2));
    sprintf( DNC_Msg, "DENC_CGMS2:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_TTXT1));
    sprintf( DNC_Msg, "DENC_TTXT1:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_TTXT2));
    sprintf( DNC_Msg, "DENC_TTXT2:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_TTXT3));
    sprintf( DNC_Msg, "DENC_TTXT3:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_TTXT4));
    sprintf( DNC_Msg, "DENC_TTXT4:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_TTXT5));
    sprintf( DNC_Msg, "DENC_TTXT5:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CCCF1L));
    sprintf( DNC_Msg, "DENC_CCCF1L:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CCCF1M));
    sprintf( DNC_Msg, "DENC_CCCF1M:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CCCF2L));
    sprintf( DNC_Msg, "DENC_CCCF2L:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CCCF2M));
    sprintf( DNC_Msg, "DENC_CCCF2M:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CCLIF1));
    sprintf( DNC_Msg, "DENC_CCLIF1:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CCLIF2));
    sprintf( DNC_Msg, "DENC_CCLIF2:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_TTXCON));
    sprintf( DNC_Msg, "DENC_TTXCON:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_DAC2));
    sprintf( DNC_Msg, "DENC_DAC2:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_BRIGHT));
    sprintf( DNC_Msg, "DENC_BRIGHT:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CONTR));
    sprintf( DNC_Msg, "DENC_CONTR:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_SATUR));
    sprintf( DNC_Msg, "DENC_SATUR:                      0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR0));
    sprintf( DNC_Msg, "DENC_CHR0:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR1));
    sprintf( DNC_Msg, "DENC_CHR1:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR2));
    sprintf( DNC_Msg, "DENC_CHR2:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR3));
    sprintf( DNC_Msg, "DENC_CHR3:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR4));
    sprintf( DNC_Msg, "DENC_CHR4:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR5));
    sprintf( DNC_Msg, "DENC_CHR5:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR6));
    sprintf( DNC_Msg, "DENC_CHR6:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR7));
    sprintf( DNC_Msg, "DENC_CHR7:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_CHR8));
    sprintf( DNC_Msg, "DENC_CHR8:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM0));
    sprintf( DNC_Msg, "DENC_LUM0:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM1));
    sprintf( DNC_Msg, "DENC_LUM1:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM2));
    sprintf( DNC_Msg, "DENC_LUM2:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM3));
    sprintf( DNC_Msg, "DENC_LUM3:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM4));
    sprintf( DNC_Msg, "DENC_LUM4:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM5));
    sprintf( DNC_Msg, "DENC_LUM5:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM6));
    sprintf( DNC_Msg, "DENC_LUM6:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM7));
    sprintf( DNC_Msg, "DENC_LUM7:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM8));
    sprintf( DNC_Msg, "DENC_LUM8:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_LUM9));
    sprintf( DNC_Msg, "DENC_LUM9:                       0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH0));
    sprintf( DNC_Msg, "DENC_AUXCH0:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH1));
    sprintf( DNC_Msg, "DENC_AUXCH1:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH2));
    sprintf( DNC_Msg, "DENC_AUXCH2:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH3));
    sprintf( DNC_Msg, "DENC_AUXCH3:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH4));
    sprintf( DNC_Msg, "DENC_AUXCH4:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH5));
    sprintf( DNC_Msg, "DENC_AUXCH5:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH6));
    sprintf( DNC_Msg, "DENC_AUXCH6:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH7));
    sprintf( DNC_Msg, "DENC_AUXCH7:                     0x%x\n", Value);
    Value = STSYS_ReadRegDev32LE((void *)(DNC_BASE_ADDRESS + DENC_AUXCH8));
    sprintf( DNC_Msg, "DENC_AUXCH8:                     0x%x\n", Value);
#endif
    sprintf( DNC_Msg, "---------------------------------------\n");

    return(RetErr);

}
#endif /*ST_7200*/



/*-------------------------------------------------------------------------
 * Function : DENC_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   :  TRUE if success, FALSE if error
 * ----------------------------------------------------------------------*/
static BOOL DENC_InitCommand(void)
{
    BOOL RetErr = FALSE;
    /* API functions : */
    RetErr  = STTST_RegisterCommand( "DENC_Init", DENC_Init, "<DeviceName><MaxOpen><DeviceType> Init denc");
    RetErr |= STTST_RegisterCommand( "DENC_Open", DENC_Open, "<Handle> <AUX> <DENC_EXTERN> Open denc AUX=1/0 DENC_EXTERN=1/0");
    RetErr |= STTST_RegisterCommand( "DENC_Close", DENC_Close, "<Handle> Close denc");
    RetErr |= STTST_RegisterCommand( "DENC_Term", DENC_Term, "<DeviceName> Term denc");
    RetErr |= STTST_RegisterCommand( "DENC_Rev", DENC_GetRevision, "Get denc revision");
    RetErr |= STTST_RegisterCommand( "DENC_SetMode", DENC_SetEncodingMode, "<Handle><Mode> (See capabilities for more info)");
    RetErr |= STTST_RegisterCommand( "DENC_GetMode", DENC_GetEncodingMode, "<Handle> Get encoding mode ");
    RetErr |= STTST_RegisterCommand( "DENC_CBP", DENC_ColorBarPattern, "<Handle><0|1> Display a color bar pattern");
    RetErr |= STTST_RegisterCommand( "DENC_Capa", DENC_GetCapability, "<DeviceName> DENC Get Capability");
    RetErr |= STTST_RegisterCommand( "DENC_SOption", DENC_SetModeOption, "<Handle><Option><Value>Option parameter");
    RetErr |= STTST_RegisterCommand( "DENC_GOption", DENC_GetModeOption, "<Handle><Option> Get optiond parameter");
    /* Utilities : */
    RetErr |= STTST_RegisterCommand( "DENC_CFG", DENC_ReadConfigurationRegister, "<Handle> Read Configuration Register");
    RetErr |= STTST_RegisterCommand( "DENC_CMP", DENC_CompareRegister, "<Handle>Compare Configuration Register");
    RetErr |= STTST_RegisterCommand( "DENC_Read", Read_dnc_regs, "<Handle><Add> Read a Sti5510 denc register");
    RetErr |= STTST_RegisterCommand( "DENC_Write", Write_dnc_regs, "<Handle><Add><Value> Write to a Sti5510 denc register");

#ifdef ST_OSLINUX
#ifdef USE_INITSIM
    RetErr |= STTST_RegisterCommand( "DENC_SimInit", DENC_SimInit, "Pokes to VTG and VOUT for Linux init");
#endif
    RetErr |= STTST_AssignString ("DRV_PATH_DENC", "denc/", TRUE);
#else
  RetErr |= STTST_AssignString ("DRV_PATH_DENC", "", TRUE);
#endif

#if defined (ST_7100) ||defined (ST_7109)|| defined (ST_7200)
    RetErr |= STTST_RegisterCommand( "DENC_GamSet", DENC_GamSet, "DENC_GamSet");
#endif

    /* Constants */
    RetErr |= STTST_AssignInteger ("ERR_DENC_INVALID_ENCODING_MODE", STDENC_ERROR_INVALID_ENCODING_MODE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_DENC_I2C", STDENC_ERROR_I2C, TRUE);

    /* Variable constant */
    RetErr |= STTST_AssignInteger ("DENC_TYPE", DENC_DEVICE_TYPE, FALSE);
#ifdef USE_VTGSIM
    RetErr |= STTST_AssignInteger ("VTGSIM_TYPE", VTGSIM_DEVICE_TYPE, FALSE);
#endif /* #ifdef USE_VTGSIM */
    RetErr |= STTST_AssignInteger ("DENC_ACCESS", DENC_DEVICE_ACCESS, FALSE);
    RetErr |= STTST_AssignString  ("DENC_NAME", STDENC_DEVICE_NAME, FALSE);


#if defined (ST_7200)
    RetErr |= STTST_RegisterCommand( "DENC1_DUMP", DENC1_Dump_Regs, "Dump DENC1 registers");
#endif  /* defined (ST_7200) */


    return(!RetErr);
} /* end DENC_MacroInit */


/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL DENC_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = DENC_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "DENC_RegisterCmd()     \t: ok           %s\n", STDENC_GetRevision()  ));
    }
    else
    {
        STTBX_Print(( "DENC_RegisterCmd()     \t: failed !\n" ));
    }

    return( RetOk );
}
/* end of file */


