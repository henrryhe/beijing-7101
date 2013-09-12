/************************************************************************

File name   : vbi_cmd.c

Description : Definition of vbi commands

COPYRIGHT (C) STMicroelectronics 2002.

Date          Modification                                    Initials
----          ------------                                    --------
05 Oct 2001   Reviewed with text error                           BS
14 Jun 2002   Update for use with STAPIGAT                      HSdLM
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"

#include "testcfg.h"

/*#if !defined (ST_OSLINUX)*/
#include "sttbx.h"
/*#endif*/

#include "testtool.h"
#include "stvbi.h"
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
#include "sti2c.h"
#endif

#include "stos.h"
#include "api_cmd.h"
#include "denc_cmd.h"
#include "lay_cmd.h"
#include "vbi_cmd.h"

#if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)
extern STAVMEM_PartitionHandle_t    AvmemPartitionHandle[];
#endif

/*
#if defined (ST_7710)
#include "stlayer.h"
#endif
 */

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

#define VBI_MAX_WRITE_DATA      19  /* Max is for MV */
#define STVBI_MAX_OPEN          3
#define VBI_DEVICE_BASE_ADDRESS 0

#ifdef ST_GX1
#define VBI_DEVICE_TYPE         STVBI_DEVICE_TYPE_GX1
#define PIXCLKCON_BASE_ADDRESS  0xBB450000
#else
#define VBI_DEVICE_TYPE         STVBI_DEVICE_TYPE_DENC
#define PIXCLKCON_BASE_ADDRESS  0  /* only for compilation */
#endif


/* Private Variables (static)------------------------------------------------ */

/* Private Constant ----------------------------------------------------------- */


/* Global Variables --------------------------------------------------------- */

STVBI_Handle_t VBIHndl;              /* Global for closed caption tests */
char VBI_Msg[200];                   /* text for trace for cmd & test file */

#if defined (USE_I2C) && defined (USE_STDENC_I2C)
STI2C_Handle_t VBI_I2CHndl = (U32)-1;
#endif /* USE_I2C USE_STDENC_I2C */

/* Private Macros ----------------------------------------------------------- */

#define VBI_PRINT(x) { \
    /* Check lenght */ \
    if (strlen(x)>sizeof(VBI_Msg)){ \
        sprintf(x, "Message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*-----------------------------------------------------------------------------
 * Function : VBI_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL VBI_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if (API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STVBI_ERROR_VBI_ALREADY_REGISTERED :
                strcat( Text, "(stvbi already registered) !\n");
                break;
            case STVBI_ERROR_VBI_NOT_ENABLE :
                strcat( Text, "(stvbi not enable) !\n");
                break;
            case STVBI_ERROR_VBI_ENABLE :
                strcat( Text, "(stvbi already enable) !\n");
                break;
            case STVBI_ERROR_UNSUPPORTED_VBI :
                strcat( Text, "(stvbi unsupported vbi) !\n");
                break;
            case STVBI_ERROR_DENC_OPEN :
                strcat( Text, "(stvbi denc open error) !\n");
                break;
            case STVBI_ERROR_DENC_ACCESS :
                strcat( Text, "(stvbi denc access error) !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    VBI_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*#######################################################################*/
/*########################### VBI API COMMANDS ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VBI_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_Init(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    ST_ErrorCode_t ErrCode;
    STVBI_InitParams_t VBIInitParams;
    U32 LVar;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    VBIInitParams.MaxOpen = STVBI_MAX_OPEN;  /* MaxOpen */

    /* get device name */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString(pars_p, STVBI_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get device Type */
    RetErr = STTST_GetInteger(pars_p, VBI_DEVICE_TYPE, (S32 *)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected device type 0=DENC 1=GX1 2=VIEWPORT");
        return(TRUE);
    }

    VBIInitParams.DeviceType = (STVBI_DeviceType_t)LVar;

    switch (VBIInitParams.DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC :
            /* Get denc name */
            RetErr = STTST_GetString(pars_p, STDENC_DEVICE_NAME, VBIInitParams.Target.DencName, sizeof(VBIInitParams.Target.DencName));
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected denc name");
                return(TRUE);
            }
            break;
        case STVBI_DEVICE_TYPE_GX1 :
            /* Get denc name */
            RetErr = STTST_GetString(pars_p, STDENC_DEVICE_NAME, VBIInitParams.Target.FullCell.DencName, sizeof(VBIInitParams.Target.FullCell.DencName));
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected denc name");
                return(TRUE);
            }
            /* Get Base Address */
            RetErr = STTST_GetInteger( pars_p, (S32)PIXCLKCON_BASE_ADDRESS, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected base address");
                return(TRUE);
            }
            VBIInitParams.Target.FullCell.BaseAddress_p = (void*)LVar;
            VBIInitParams.Target.FullCell.DeviceBaseAddress_p = (void*)VBI_DEVICE_BASE_ADDRESS;
            break;

#if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

        case STVBI_DEVICE_TYPE_VIEWPORT :

            RetErr = STTST_GetString(pars_p, STLAYER_GFX_DEVICE_NAME, VBIInitParams.Target.Viewport.LayerName, sizeof(VBIInitParams.Target.Viewport.LayerName));
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected denc name");
                return(TRUE);
            }

#if !defined(ST_OSLINUX)
             VBIInitParams.Target.Viewport.AVMemPartitionHandle=AvmemPartitionHandle[0];
#endif


            break;
#endif
        default :
            /* filtered above */
            break;
    }

    switch (VBIInitParams.DeviceType)
    {
        case STVBI_DEVICE_TYPE_DENC :
            sprintf(VBI_Msg, "VBI_Init(%s,Type=DENC,Denc=\"%s\"): ", DeviceName, VBIInitParams.Target.DencName);
            break;
        case STVBI_DEVICE_TYPE_GX1 :
            sprintf(VBI_Msg, "VBI_Init(%s,Type=GX1,Denc=\"%s,BaseAdd=0x%0X\"): ", DeviceName,
                    VBIInitParams.Target.DencName,
                    (U32)(VBIInitParams.Target.FullCell.BaseAddress_p));
            break;
        case STVBI_DEVICE_TYPE_VIEWPORT :
            sprintf(VBI_Msg, "VBI_Init(%s,Type=VIEWPORT,LayerName=%s): ", DeviceName,
                    VBIInitParams.Target.Viewport.LayerName);
            break;

        default :
            sprintf(VBI_Msg, "VBI_Init(%s,Type=????\"): ", DeviceName);
            break;
    }

    /* Init */
    ErrCode = STVBI_Init(DeviceName, &VBIInitParams);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);
    return (API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VBI_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_Open(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80], ParamsStr[80];
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    U32 LVar;


    /* VBI specific */
    STVBI_OpenParams_t VBIOpenParams;

    UNUSED_PARAMETER(result_sym_p);

    /* get device */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString(pars_p, STVBI_DEVICE_NAME , DeviceName, sizeof(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, STVBI_VBI_TYPE_TELETEXT, (S32 *)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected mode (Default 1=TTX, 2=CC, 4=WSS, 8=CGMS, 16=VPS, 32=MV )");
        return(TRUE);
    }
    VBIOpenParams.Configuration.VbiType = (STVBI_VbiType_t)LVar;

    switch (VBIOpenParams.Configuration.VbiType)
    {
        case STVBI_VBI_TYPE_TELETEXT:
#if defined (ST_5510)
            RetErr = STTST_GetInteger(pars_p, 4, (S32 *)&LVar);
#else /* ST_5510 */
            RetErr = STTST_GetInteger(pars_p, 2, (S32 *)&LVar);
#endif /* ST_5510 */
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected TTX latency 0-7");
                return(TRUE);
            }
            VBIOpenParams.Configuration.Type.TTX.Latency = LVar;
            sprintf(ParamsStr, ",Lat=%d", LVar);

            RetErr = STTST_GetInteger(pars_p, 0, (S32 *)&LVar);
            if ((RetErr) || (LVar>1))
            {
                STTST_TagCurrentLine(pars_p, "Expected TTX full page (default 0=Off,1=On)");
                return(TRUE);
            }
            VBIOpenParams.Configuration.Type.TTX.FullPage = (BOOL)LVar;
            sprintf(ParamsStr, "%s,Full=%s", ParamsStr, (LVar == 0)?  "Off" : "On");

            RetErr = STTST_GetInteger(pars_p, STVBI_TELETEXT_AMPLITUDE_70IRE, (S32 *)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected TTX amplitude (default 1=70IRE,2=100IRE)");
                return(TRUE);
            }
            VBIOpenParams.Configuration.Type.TTX.Amplitude = (STVBI_TeletextAmplitude_t)LVar;
            sprintf(ParamsStr, "%s,Amp=%s", ParamsStr, \
                    (LVar == STVBI_TELETEXT_AMPLITUDE_70IRE)?  "70IRE" : \
                    (LVar == STVBI_TELETEXT_AMPLITUDE_100IRE)? "100IRE" : "???" );

            RetErr = STTST_GetInteger(pars_p, STVBI_TELETEXT_STANDARD_B, (S32 *)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected TTX standard (1=A,default 2=B,4=C,8=D)");
                return(TRUE);
            }
            VBIOpenParams.Configuration.Type.TTX.Standard = (STVBI_TeletextStandard_t)LVar;
            sprintf(ParamsStr, "%s,Sdt=%s", ParamsStr, \
                    (LVar == STVBI_TELETEXT_STANDARD_A)?  "A" : \
                    (LVar == STVBI_TELETEXT_STANDARD_B)?  "B" : \
                    (LVar == STVBI_TELETEXT_STANDARD_C)?  "C" : \
                    (LVar == STVBI_TELETEXT_STANDARD_D)?  "D" : "???");
            break;

        case STVBI_VBI_TYPE_CLOSEDCAPTION:
            RetErr = STTST_GetInteger(pars_p, STVBI_CCMODE_FIELD1, (S32 *)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected CC mode (1=None,2=FIELD1 (default),4=FIELD2,8=BOTH1&2)");
                return(TRUE);
            }
            VBIOpenParams.Configuration.Type.CC.Mode = (STVBI_CCMode_t)LVar;
            sprintf(ParamsStr, ",Mode=%s",
                    (LVar == STVBI_CCMODE_NONE)? "None":\
                    (LVar == STVBI_CCMODE_FIELD1)? "F1":\
                    (LVar == STVBI_CCMODE_FIELD2)? "F2":\
                    (LVar == STVBI_CCMODE_BOTH)? "F1&2": "???");
            break;

        case STVBI_VBI_TYPE_WSS:
        case STVBI_VBI_TYPE_CGMS:
        case STVBI_VBI_TYPE_VPS:
            ParamsStr[0]=0;
            break;

        case STVBI_VBI_TYPE_MACROVISION:
            RetErr = STTST_GetInteger(pars_p, STVBI_COPY_PROTECTION_MV7, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected MV mode (1=MV6,default 2=MV7)");
                return(TRUE);
            }
            VBIOpenParams.Configuration.Type.MV.Algorithm = (STVBI_CopyProtectionAlgorithm_t)LVar;
            sprintf(ParamsStr, ",Algo=%s",
                    (LVar == STVBI_COPY_PROTECTION_MV6)? "6":\
                    (LVar == STVBI_COPY_PROTECTION_MV7)? "7": "???");
            break;

  #if defined (ST_7710)||defined (ST_7109)||defined (ST_7100)||defined (ST_7200)

        case STVBI_VBI_TYPE_CGMSFULL:

                        /* Get Cgms Mode */
            RetErr = STTST_GetInteger( pars_p,STVBI_CGMS_HD_1080I60000, (S32 *)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine( pars_p, "Expected base address");
                return(TRUE);
            }
            sprintf(ParamsStr, ",Mode=%s",
                    (LVar == STVBI_CGMS_HD_1080I60000)? "1080I":\
                    (LVar == STVBI_CGMS_SD_480P60000)?  "480P":\
                    (LVar == STVBI_CGMS_HD_720P60000)? "720P":\
                    (LVar == STVBI_CGMS_TYPE_B_HD_1080I60000)?  "1080I_TYPE_B":\
                    (LVar == STVBI_CGMS_TYPE_B_SD_480P60000)? "480P_TYPE_B":\
                    (LVar == STVBI_CGMS_TYPE_B_HD_720P60000)? "720P_TYPE_B": "???");


               VBIOpenParams.Configuration.Type.CGMSFULL.CGMSFULLStandard =(STVBI_CGMSFULLStandard_t) LVar;
               break;
  #endif

        default:
            ParamsStr[0]=0;
            break;
    }

    sprintf(VBI_Msg, "VBI_Open(%s,Type=%s%s): ", DeviceName,
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_TELETEXT) ? "TTX" : \
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_CLOSEDCAPTION) ? "CC" : \
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_WSS) ? "WSS" : \
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_CGMS) ? "CGMS" : \
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_CGMSFULL) ? "CGMSFULL" : \
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_VPS) ? "VPS" : \
            (VBIOpenParams.Configuration.VbiType == STVBI_VBI_TYPE_MACROVISION) ? "MV" : "???", ParamsStr);
    ErrCode = STVBI_Open(DeviceName, &VBIOpenParams, &VBIHndl);
    if ( ErrCode == ST_NO_ERROR)
    {
        sprintf(VBI_Msg, "%sHdl=%d ", VBI_Msg, VBIHndl);
    }
    RetErr = VBI_TextError(ErrCode, VBI_Msg);
    STTST_AssignInteger(result_sym_p, VBIHndl, FALSE);
    return (API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VBI_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_Close(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    /* Close a VBI device connection */

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32*)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(VBI_Msg, "VBI_Close(%d): ", VBIHndl);
    ErrCode = STVBI_Close(VBIHndl);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);
    return (API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VBI_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_Term(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    ST_ErrorCode_t ErrCode;
    STVBI_TermParams_t VBITermParams;
    BOOL RetErr;
    U32  LVar;

    UNUSED_PARAMETER(result_sym_p);

    /* get device name */
    DeviceName[0] = '\0';

    RetErr = STTST_GetString(pars_p, STVBI_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 0, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected force terminate (default 0=FALSE,1=TRUE)");
        return(TRUE);
    }
    VBITermParams.ForceTerminate = (BOOL)LVar;

    sprintf(VBI_Msg, "VBI_Term(%s,ForceTerm=%s): ", DeviceName,
            (VBITermParams.ForceTerminate) ? "TRUE" :
            (!VBITermParams.ForceTerminate) ? "FALSE" : "????");
    ErrCode = STVBI_Term(DeviceName, &VBITermParams);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);
    if ( ErrCode == ST_NO_ERROR)
    {
#if defined (USE_I2C) && defined (USE_STDENC_I2C)
        if (VBI_I2CHndl != (U32)-1)
        {
            ErrCode=STI2C_Close(VBI_I2CHndl);
            if (ErrCode != ST_NO_ERROR)
                strcat(VBI_Msg, "Failed to close I2C for private denc access !!!\n");
            VBI_I2CHndl = (U32)-1;
        }
#endif /* USE_I2C USE_STDENC_I2C */
    }
    return (API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : VBI_getRevision
 *            Get driver revision number
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VBI_GetRevision(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_Revision_t RevisionNumber;
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);
    RevisionNumber = STVBI_GetRevision();
    sprintf(VBI_Msg, "VBI_GetRevision(): Ok\nRevision=%s\n", RevisionNumber);

    VBI_PRINT(VBI_Msg);
    return (FALSE);
} /* end VBI_GetRevision */


/*-------------------------------------------------------------------------
 * Function : VBI_GetCapability
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_GetCapability(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80], StrParams[80];
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVBI_Capability_t Capability;


    UNUSED_PARAMETER(result_sym_p);
    /* get device name */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString(pars_p, STVBI_DEVICE_NAME, DeviceName, sizeof(DeviceName));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf(VBI_Msg, "VBI_GetCapability(%s): ", DeviceName);
    ErrCode = STVBI_GetCapability(DeviceName, &Capability);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    if (ErrCode == ST_NO_ERROR)
    {
        sprintf(VBI_Msg, "Supported Vbi    : ");
        if (Capability.SupportedVbi == 0x0) strcat(VBI_Msg, "0");
        if (Capability.SupportedVbi & STVBI_VBI_TYPE_TELETEXT)      strcat(VBI_Msg, "TELETEXT ");
        if (Capability.SupportedVbi & STVBI_VBI_TYPE_CLOSEDCAPTION) strcat(VBI_Msg, "CC ");
        if (Capability.SupportedVbi & STVBI_VBI_TYPE_WSS)           strcat(VBI_Msg, "WSS ");
        if (Capability.SupportedVbi & STVBI_VBI_TYPE_CGMS)          strcat(VBI_Msg, "CGMS ");
        if (Capability.SupportedVbi & STVBI_VBI_TYPE_VPS)           strcat(VBI_Msg, "VPS ");
        if (Capability.SupportedVbi & STVBI_VBI_TYPE_MACROVISION)   strcat(VBI_Msg, "MV ");
        strcat(VBI_Msg, "\n");
        VBI_PRINT(VBI_Msg);

        if (Capability.SupportedVbi & STVBI_VBI_TYPE_TELETEXT){
            sprintf(VBI_Msg, "Supported TtxStd : ");
            if (Capability.SupportedTtxStd == 0x0) strcat(VBI_Msg, "0");
            if (Capability.SupportedTtxStd & STVBI_TELETEXT_STANDARD_A)   strcat(VBI_Msg, "A ");
            if (Capability.SupportedTtxStd & STVBI_TELETEXT_STANDARD_B)   strcat(VBI_Msg, "B ");
            if (Capability.SupportedTtxStd & STVBI_TELETEXT_STANDARD_C)   strcat(VBI_Msg, "C ");
            if (Capability.SupportedTtxStd & STVBI_TELETEXT_STANDARD_D)   strcat(VBI_Msg, "D ");
            strcat(VBI_Msg, "\n");
            VBI_PRINT(VBI_Msg);
        }

        if (Capability.SupportedVbi & STVBI_VBI_TYPE_MACROVISION){
            sprintf(VBI_Msg, "Supported MvAlgo : ");
            if (Capability.SupportedMvAlgo == 0x0) strcat(VBI_Msg, "0");
            if (Capability.SupportedMvAlgo & STVBI_COPY_PROTECTION_MV6)   strcat(VBI_Msg, "MV6 ");
            if (Capability.SupportedMvAlgo & STVBI_COPY_PROTECTION_MV7)   strcat(VBI_Msg, "MV7 ");
            strcat(VBI_Msg, "\n");
            VBI_PRINT(VBI_Msg);
        }
        sprintf(StrParams, "%d %d %d", Capability.SupportedVbi, Capability.SupportedTtxStd, \
                Capability.SupportedMvAlgo);
        STTST_AssignString(result_sym_p, StrParams, FALSE);
    }
    return (API_EnableError ? RetErr : FALSE);
} /* end STVBI_GetCapability */


/*-------------------------------------------------------------------------
 * Function : VBI_DisableVBI
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_DisableVBI(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32*)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(VBI_Msg, "VBI_Disable(%d): ", VBIHndl);
    ErrCode = STVBI_Disable(VBIHndl);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(VBI_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(VBI_Msg, "Invalid handle !\n");
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(VBI_Msg, "Bad parameters !\n");
            break;
        case STVBI_ERROR_VBI_NOT_ENABLE:
            API_ErrorCount++;
            strcat(VBI_Msg, "Already disable !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(VBI_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    VBI_PRINT(VBI_Msg);
    return (API_EnableError ? RetErr : FALSE);

} /* end VBI_DisableVBI */


/*-------------------------------------------------------------------------
 * Function : VBI_Enable VBI
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_EnableVBI(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32*)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(VBI_Msg, "VBI_Enable(%d): ", VBIHndl);
    ErrCode = STVBI_Enable(VBIHndl);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    return (API_EnableError ? RetErr : FALSE);
} /* end VBI_GetMode */


/*-------------------------------------------------------------------------
 * Function : VBI_GetVBIConfiguration
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_GetVBIConfiguration(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVBI_Configuration_t Config_s;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32 *)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(VBI_Msg, "VBI_GetConfiguration(%d): ", VBIHndl);
    ErrCode = STVBI_GetConfiguration(VBIHndl, &Config_s);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    if (ErrCode == ST_NO_ERROR)
    {
        sprintf(VBI_Msg, "TYPE %s : ",
                (Config_s.VbiType & STVBI_VBI_TYPE_TELETEXT)      ? "TTX":
                (Config_s.VbiType & STVBI_VBI_TYPE_CLOSEDCAPTION) ? "CC":
                (Config_s.VbiType & STVBI_VBI_TYPE_WSS)           ? "WSS":
                (Config_s.VbiType & STVBI_VBI_TYPE_CGMS)          ? "CGMS":
                (Config_s.VbiType & STVBI_VBI_TYPE_VPS)           ? "VPS":
                (Config_s.VbiType & STVBI_VBI_TYPE_MACROVISION)   ? "MV": "????" );
        switch (Config_s.VbiType)
        {
            case STVBI_VBI_TYPE_TELETEXT :
                sprintf(VBI_Msg, "%sLat %d,", VBI_Msg, Config_s.Type.TTX.Latency);
                if (Config_s.Type.TTX.FullPage) {
                    strcat(VBI_Msg, "FullPage Yes,");
                }
                else{
                    strcat(VBI_Msg, "FullPage No,");
                }
                switch (Config_s.Type.TTX.Amplitude){
                    case STVBI_TELETEXT_AMPLITUDE_70IRE : strcat(VBI_Msg, "Amp 70IRE, "); break;
                    case STVBI_TELETEXT_AMPLITUDE_100IRE : strcat(VBI_Msg, "Amp 100IRE, "); break;
                    default:
                        strcat(VBI_Msg, "Amp ????,"); break;
                }
                switch (Config_s.Type.TTX.Standard){
                    case STVBI_TELETEXT_STANDARD_A : strcat(VBI_Msg, "Std A"); break;
                    case STVBI_TELETEXT_STANDARD_B : strcat(VBI_Msg, "Std B"); break;
                    case STVBI_TELETEXT_STANDARD_C : strcat(VBI_Msg, "Std C"); break;
                    case STVBI_TELETEXT_STANDARD_D : strcat(VBI_Msg, "Std D"); break;
                    default:
                        strcat(VBI_Msg, "Std ??"); break;
                }
                break;

            case STVBI_VBI_TYPE_CLOSEDCAPTION :
                switch (Config_s.Type.CC.Mode) {
                    case STVBI_CCMODE_NONE : strcat(VBI_Msg, "Mode None, "); break;
                    case STVBI_CCMODE_FIELD1 : strcat(VBI_Msg, "Mode F1"); break;
                    case STVBI_CCMODE_FIELD2 : strcat(VBI_Msg, "Mode F2"); break;
                    case STVBI_CCMODE_BOTH : strcat(VBI_Msg, "Mode F1&2"); break;
                    default:
                        strcat(VBI_Msg, "Mode ???");
                        break;
                }
                break;

            case STVBI_VBI_TYPE_WSS :
            case STVBI_VBI_TYPE_CGMS :
            case STVBI_VBI_TYPE_VPS :
                strcat(VBI_Msg, "No configuration");
                break;

            case STVBI_VBI_TYPE_MACROVISION :
                switch (Config_s.Type.MV.Algorithm){
                    case STVBI_COPY_PROTECTION_MV6 : strcat(VBI_Msg, "Algo V6"); break;
                    case STVBI_COPY_PROTECTION_MV7 : strcat(VBI_Msg, "Algo V7"); break;
                    default:
                        strcat(VBI_Msg, "Algo ???");
                        break;
                }
                break;

            default:
                strcat(VBI_Msg, "Unknown type ???");
                break;
        }
        strcat( VBI_Msg, "\n");
        VBI_PRINT(VBI_Msg);
    }

    return (API_EnableError ? RetErr : FALSE);
} /* end VBI_GetVBIConfiguration */

/*-------------------------------------------------------------------------
 * Function : VBI_SetVBIDynamicParams
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_SetVBIDynamicParams(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVBI_DynamicParams_t DynamicParams_p;
    U32 LVar;
    char ParamsMsg[80];

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32*)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    /* Default TTX */
    RetErr = STTST_GetInteger(pars_p, STVBI_VBI_TYPE_TELETEXT, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected ouput mode (1=TTX,2=CC,4=WSS,8=CGMS,16=VPS,32=MV)");
        return(TRUE);
    }

    DynamicParams_p.VbiType = (STVBI_VbiType_t)LVar;

    switch (DynamicParams_p.VbiType)
    {
        case STVBI_VBI_TYPE_TELETEXT :
            RetErr = STTST_GetInteger(pars_p, 0, (S32*)&LVar);
            if ((RetErr) || (LVar>1))
            {
                STTST_TagCurrentLine(pars_p, "Expected Mask (default 0=Off,1=On)");
                return(TRUE);
            }
            DynamicParams_p.Type.TTX.Mask = LVar;
            sprintf ( ParamsMsg, ",Mask=%s,", (LVar==0) ? "Off" : "On");
            RetErr = STTST_GetInteger(pars_p, 10, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected LineCount (default 10)");
                return(TRUE);
            }
            DynamicParams_p.Type.TTX.LineCount = LVar;
            sprintf ( ParamsMsg, "%sLineCount=%d,", ParamsMsg, LVar);

            if (DynamicParams_p.Type.TTX.LineCount == 0)
            {
                /* default: line 7, 9 and 13 => 0x2280 */
                RetErr = STTST_GetInteger(pars_p, 0x2280, (S32*)&LVar);
                if (RetErr)
                {
                    STTST_TagCurrentLine(pars_p, "Expected LineMask (default line 7, 9 and 13 => 0x2280)");
                    return(TRUE);
                }
                DynamicParams_p.Type.TTX.LineMask = LVar;
                sprintf ( ParamsMsg, "%sLineMask=0x%0X,", ParamsMsg, LVar);
            }

            RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar);
            if ((RetErr) || (LVar>1))
            {
                STTST_TagCurrentLine(pars_p, "Expected Field (1=Odd,0=Even)");
                return(TRUE);
            }
            DynamicParams_p.Type.TTX.Field = LVar;
            sprintf ( ParamsMsg, "%sField=%s", ParamsMsg, (LVar==1) ? "Odd" : "Even");
            break;

        case STVBI_VBI_TYPE_CLOSEDCAPTION :
            RetErr = STTST_GetInteger(pars_p, STVBI_CCMODE_FIELD1, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected Type F1=2(default),F2=4,BOTH=8");
                return(TRUE);
            }
            DynamicParams_p.Type.CC.Mode = LVar;
            sprintf ( ParamsMsg, ",Write=%s",
                      (LVar == STVBI_CCMODE_NONE)? "None":\
                      (LVar == STVBI_CCMODE_FIELD1)? "F1":\
                      (LVar == STVBI_CCMODE_FIELD2)? "F2":\
                      (LVar == STVBI_CCMODE_BOTH)? "F1&2": "???");

            RetErr = STTST_GetInteger(pars_p, 21, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected LinesField1 (default 21)");
                return(TRUE);
            }
            DynamicParams_p.Type.CC.LinesField1 = LVar;
            sprintf ( ParamsMsg, "%s,F1=%d,", ParamsMsg, LVar);

            RetErr = STTST_GetInteger(pars_p, 284, (S32*)&LVar);
            if (RetErr)
            {
                STTST_TagCurrentLine(pars_p, "Expected LinesField2 (default 284)");
                return(TRUE);
            }
            DynamicParams_p.Type.CC.LinesField2 = LVar;
            sprintf ( ParamsMsg, "%sF2=%d", ParamsMsg, LVar);
            break;

        case STVBI_VBI_TYPE_WSS :
        case STVBI_VBI_TYPE_CGMS :
        case STVBI_VBI_TYPE_VPS :
            ParamsMsg[0]=0;
            break;

        case STVBI_VBI_TYPE_MACROVISION :
            RetErr = STTST_GetInteger(pars_p, STVBI_MV_PREDEFINED_0, (S32*)&LVar);
            if ((RetErr) || (LVar>5))
            {
                STTST_TagCurrentLine(pars_p, "Expected predef mode (0-5)");
                return(TRUE);
            }
            DynamicParams_p.Type.MV.PredefinedMode = LVar;
            sprintf ( ParamsMsg, ",PreDef=%d,", LVar);

            RetErr = STTST_GetInteger(pars_p, STVBI_MV_USER_DEFINED_0, (S32*)&LVar);
            if ((RetErr) || (LVar>255))
            {
                STTST_TagCurrentLine(pars_p, "Expected user mode (1-255)");
                return(TRUE);
            }
            DynamicParams_p.Type.MV.UserMode = LVar;
            sprintf ( ParamsMsg, "%sUserDef=%d", ParamsMsg, LVar);
            break;

        default:
            ParamsMsg[0]=0;
            break;
    }

    sprintf(VBI_Msg, "VBI_SetDynamicParams(%d%s): ", VBIHndl, ParamsMsg);
    ErrCode = STVBI_SetDynamicParams(VBIHndl, &DynamicParams_p);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    return (API_EnableError ? RetErr : FALSE);
} /* end VBI_SetVBIDynamicParams */


/*-------------------------------------------------------------------------
 * Function : VBI_GetTtxSource
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_GetTtxSource(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVBI_TeletextSource_t TtxSource;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32 *)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(VBI_Msg, "VBI_GetTtxSource(%d): ", VBIHndl);
    ErrCode = STVBI_GetTeletextSource(VBIHndl, &TtxSource);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    if (ErrCode == ST_NO_ERROR)
    {
        sprintf(VBI_Msg, "TTX Source: %s\n",
                (TtxSource == STVBI_TELETEXT_SOURCE_DMA) ? "DMA":
                (TtxSource == STVBI_TELETEXT_SOURCE_PIN) ? "PIN": "????" );
        VBI_PRINT(VBI_Msg);
    }
    return (API_EnableError ? RetErr : FALSE);
} /* end VBI_GetTtxSource */

/*-------------------------------------------------------------------------
 * Function : VBI_SetTtxSource
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_SetTtxSource(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    STVBI_TeletextSource_t TtxSource;
    U32 LVar;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32*)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    /* Default DMA */
    RetErr = STTST_GetInteger(pars_p, STVBI_TELETEXT_SOURCE_DMA, (S32*)&LVar);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected teletext source (1=DMA,2=PIN)");
        return(TRUE);
    }

    TtxSource = (STVBI_TeletextSource_t)LVar;

    sprintf(VBI_Msg, "STVBI_SetTeletextSource(%d ,%s): ", VBIHndl,
            (TtxSource==STVBI_TELETEXT_SOURCE_DMA)?"DMA":
            (TtxSource==STVBI_TELETEXT_SOURCE_PIN)?"PIN":"???");
    ErrCode = STVBI_SetTeletextSource(VBIHndl, TtxSource);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    return (API_EnableError ? RetErr : FALSE);
} /* end VBI_SetTtxSource */


/*-------------------------------------------------------------------------
 * Function : VBI_WriteData
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL VBI_WriteData(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL           RetErr;
    U32            LVar;
    ST_ErrorCode_t ErrCode;
    U8             DataToWrite[VBI_MAX_WRITE_DATA], i, NbData;

    UNUSED_PARAMETER(result_sym_p);
    RetErr = STTST_GetInteger(pars_p, VBIHndl, (S32*)&VBIHndl);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar);
    if ((RetErr) || (LVar>VBI_MAX_WRITE_DATA))
    {
        STTST_TagCurrentLine(pars_p, "Expected number of datas");
        return(TRUE);
    }
    NbData= (U8)LVar;

    for (i=0; i<NbData; i++){
        RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected data");
            return(TRUE);
        }
        DataToWrite[i]= (U8)LVar;
    }

    sprintf( VBI_Msg, "VBI_WriteData(%d", VBIHndl);
    for (i=0; i<NbData; i++){
        sprintf( VBI_Msg, "%s,h%02X", VBI_Msg, DataToWrite[i]);
    }
    sprintf( VBI_Msg, "%s): ", VBI_Msg);
    ErrCode = STVBI_WriteData(VBIHndl, DataToWrite, NbData);
    RetErr = VBI_TextError(ErrCode, VBI_Msg);

    return (API_EnableError ? RetErr : FALSE);

} /* end VBI_WriteData */



/*#######################################################################*/
/*########################## REGISTER COMMANDS ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : VBI_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL VBI_InitCommand(void)
{
    BOOL RetErr=FALSE;

    /* API functions */
    RetErr  = STTST_RegisterCommand("VBI_Init", VBI_Init,\
                               "<DeviceName><DeviceType> Init (Type default DENC)");
    RetErr |= STTST_RegisterCommand("VBI_Open", VBI_Open,\
                               "<DeviceName><Mode><...> Open (Mode default 1=TTX, 2=CC, 4=WSS, 8=CGMS, 16=VPS, 32=MV)");
    RetErr |= STTST_RegisterCommand("VBI_Close", VBI_Close, "<Handle> Close");
    RetErr |= STTST_RegisterCommand("VBI_Term", VBI_Term, "<DeviceName><ForceTerm> Terminate");
    RetErr |= STTST_RegisterCommand("VBI_Rev", VBI_GetRevision, "Get revision");
    RetErr |= STTST_RegisterCommand("VBI_Capa", VBI_GetCapability, "<DeviceName> Get capabilities");
    RetErr |= STTST_RegisterCommand("VBI_Disable", VBI_DisableVBI, "<Handle> Disable");
    RetErr |= STTST_RegisterCommand("VBI_Enable", VBI_EnableVBI, "<Handle> Enable");
    RetErr |= STTST_RegisterCommand("VBI_GetConfig", VBI_GetVBIConfiguration, "<Handle> Get configuration");
    RetErr |= STTST_RegisterCommand("VBI_SetParams", VBI_SetVBIDynamicParams,\
                               "<Handle><Mode><...> Set VBI dynamic parameters (arguments lenght depends of mode)");
    RetErr |= STTST_RegisterCommand("VBI_GetTtxSource", VBI_GetTtxSource, "<Handle> Get teletext source: 1=DMA 2=PIN");
    RetErr |= STTST_RegisterCommand("VBI_SetTtxSource", VBI_SetTtxSource, "<Handle> <Source 1=DMA 2=PIN> Set teletext source");
    RetErr |= STTST_RegisterCommand("VBI_WriteData", VBI_WriteData, "<Handle><NbData><Data1><...> Write VBI data");

    /* API variables */
    RetErr |= STTST_AssignInteger ("TTX", STVBI_VBI_TYPE_TELETEXT, TRUE);
    RetErr |= STTST_AssignInteger ("CC", STVBI_VBI_TYPE_CLOSEDCAPTION, TRUE);
    RetErr |= STTST_AssignInteger ("WSS", STVBI_VBI_TYPE_WSS, TRUE);
    RetErr |= STTST_AssignInteger ("CGMS", STVBI_VBI_TYPE_CGMS, TRUE);
    RetErr |= STTST_AssignInteger ("VIEWPORT", STVBI_DEVICE_TYPE_VIEWPORT, TRUE);
    RetErr |= STTST_AssignInteger ("CGMSFULL", STVBI_VBI_TYPE_CGMSFULL, TRUE);
    RetErr |= STTST_AssignInteger ("M1080I", STVBI_CGMS_HD_1080I60000, TRUE);
    RetErr |= STTST_AssignInteger ("M720P", STVBI_CGMS_HD_720P60000, TRUE);
    RetErr |= STTST_AssignInteger ("M480P", STVBI_CGMS_SD_480P60000, TRUE);

    RetErr |= STTST_AssignInteger ("M1080I_B", STVBI_CGMS_TYPE_B_HD_1080I60000, TRUE);
    RetErr |= STTST_AssignInteger ("M720P_B", STVBI_CGMS_TYPE_B_HD_720P60000, TRUE);
    RetErr |= STTST_AssignInteger ("M480P_B", STVBI_CGMS_TYPE_B_SD_480P60000, TRUE);


    RetErr |= STTST_AssignInteger ("VPS", STVBI_VBI_TYPE_VPS, TRUE);
    RetErr |= STTST_AssignInteger ("MV", STVBI_VBI_TYPE_MACROVISION, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VBI_ALREADY_REGISTERED", STVBI_ERROR_VBI_ALREADY_REGISTERED, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VBI_DENC_OPEN", STVBI_ERROR_DENC_OPEN, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VBI_DENC_ACCESS", STVBI_ERROR_DENC_ACCESS, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VBI_ENABLE", STVBI_ERROR_VBI_ENABLE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VBI_NOT_ENABLE", STVBI_ERROR_VBI_NOT_ENABLE, TRUE);
    RetErr |= STTST_AssignInteger ("ERR_VBI_UNSUPPORTED_VBI", STVBI_ERROR_UNSUPPORTED_VBI, TRUE);
    RetErr |= STTST_AssignString  ("VBI_DEVICE_NAME", STVBI_DEVICE_NAME, TRUE);
    #ifdef ST_OSLINUX
    RetErr |= STTST_AssignString ("DRV_PATH_VBI", "vbi/", TRUE);
#else
    RetErr |= STTST_AssignString ("DRV_PATH_VBI", "", TRUE);
#endif

#ifdef ST_7710
#ifdef STI7710_CUT2x
    RetErr |= STTST_AssignString ("CHIP_CUT", "STI7710_CUT2x", TRUE);
#else
    RetErr |= STTST_AssignString ("CHIP_CUT", "", TRUE);
#endif
#endif /*** ST_7710 ***/


    return(!RetErr);
} /* end of VBI_InitCommand() */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL VBI_RegisterCmd()
{
    BOOL RetOk;

    RetOk = VBI_InitCommand();
    if ( RetOk )
    {
        STTBX_Print(( "VBI_RegisterCmd()     \t: ok           %s\n", STVBI_GetRevision()));
    }
    else
    {
        STTBX_Print(( "VBI_RegisterCmd()     \t: failed !\n" ));
    }

    return(RetOk);
} /* end of VBI_RegisterCmd() */

/* end of vbi_cmd.c */

