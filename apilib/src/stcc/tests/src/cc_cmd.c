/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : cc_cmd.c
Description : Closed Caption macros

Note        :

Date          Modification                                    Initials
----          ------------                                    --------
28 aug 2001   Creation                                        Michel Bruant
09 Jan 2002   Updates.                                          HSdLM
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif

#include "stos.h"

#include "stddefs.h"
#include "stdevice.h"
#include "stvbi.h"
#include "testtool.h"
#include "startup.h"
#include "stsys.h"
#include "sti2c.h"
#include "vbi_cmd.h"
#include "vid_cmd.h"
#include "vtg_cmd.h"
#include "stcc.h"
#include "api_cmd.h"
#include "clevt.h"
#ifndef STPTI_UNAVAILABLE
#include "stpti.h"
#endif
#include "vid_util.h"



/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Extern functions prototypes --- */
/*extern BOOL VID_RegisterCmd2 (void);*/
 BOOL VID_Inj_RegisterCmd(void);
/* --- Global variables --- */
STCC_Handle_t CCHndl;

/* --- Private variables --- */
static char CC_Msg[200];            /* text for trace */

/* --- Extern variables --- */
ST_Partition_t *SystemPartition_p;

/* DEBUG PART */
#include "../../src/stccinit.h"

BOOL CC_ResetDebugCounters(parse_t *pars_p, char *result_sym_p);
BOOL CC_PrintDebugCounters(parse_t *pars_p, char *result_sym_p);


BOOL CC_Init(parse_t *pars_p, char *result_sym_p);
void os20_main(void *ptr);
BOOL Test_CmdStart(void);

#if !defined(ST_OSLINUX)
BOOL CC_ResetDebugCounters(parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    STCC_DebugStats.NbUserDataReceived      = 0;
    STCC_DebugStats.NbUserDataParsed        = 0;
    STCC_DebugStats.NBCaptionDataFound      = 0;
    STCC_DebugStats.NBCaptionDataPresented  = 0;
    return (FALSE);
}
/*-------------------------------------------------------------------------
 * Function : CC_PrintDebugCounters
 * Description : assign integer values to the the parameters of the function "CC_PrintDebugCounters"
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE  if no errors, else TRUE.
 * ----------------------------------------------------------------------*/

BOOL CC_PrintDebugCounters(parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    STTST_Print(("Nb User Data Received : %i \r\n",
                STCC_DebugStats.NbUserDataReceived));
    STTST_AssignInteger("CaptionNbUserDataReceived", STCC_DebugStats.NbUserDataReceived, FALSE);
    STTST_Print(("Nb User Data Parsed : %i \r\n",
                STCC_DebugStats.NbUserDataParsed));
    STTST_AssignInteger("CaptionNbUserDataParsed", STCC_DebugStats.NbUserDataParsed, FALSE);
    STTST_Print(("Nb Caption found : %i \r\n",
                STCC_DebugStats.NBCaptionDataFound));
    STTST_AssignInteger("CaptionNbCaptionDataFound", STCC_DebugStats.NBCaptionDataFound, FALSE);
    STTST_Print(("Nb Caption Presented : %i \r\n",
                STCC_DebugStats.NBCaptionDataPresented));
    STTST_AssignInteger("CaptionNbCaptionDataPresented", STCC_DebugStats.NBCaptionDataPresented, FALSE);

    return (FALSE);
}
#else
BOOL CC_ResetDebugCounters(parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
     return (FALSE);
}
BOOL CC_PrintDebugCounters(parse_t *pars_p, char *result_sym_p)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    return (FALSE);
}
#endif
/*-------------------------------------------------------------------------
 * Function : CC_Init
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL CC_Init(parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    ST_ErrorCode_t ErrCode;
    STCC_InitParams_t CCInitParams;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    /* get device name */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString(pars_p, "CLOSEDCAPTION", DeviceName,
                                                sizeof(DeviceName));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    /* Get vbi name */
    RetErr = STTST_GetString(pars_p, STVBI_DEVICE_NAME, CCInitParams.VBIName,
                                    sizeof(CCInitParams.VBIName));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected vbi name");
        return(TRUE);
    }

    /* Get vtg name */
    RetErr = STTST_GetString(pars_p, STVTG_DEVICE_NAME, CCInitParams.VTGName,
                        sizeof(CCInitParams.VTGName));

    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected vtg name");
        return(TRUE);
    }

    /* Get video name */
    RetErr = STTST_GetString(pars_p, STVID_DEVICE_NAME1, CCInitParams.VIDName,
                                 sizeof(CCInitParams.VIDName));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected video name");
        return(TRUE);
    }

    CCInitParams.MaxOpen = 1;  /* MaxOpen */
    CCInitParams.CPUPartition_p = SystemPartition_p;
    strcpy(CCInitParams.EvtHandlerName , STEVT_DEVICE_NAME);

    sprintf(CC_Msg,"CC_Init(%s,Vbi=\"%s\",Vtg=\"%s\",Vid=\"%s\"): ",DeviceName,
            CCInitParams.VBIName, CCInitParams.VTGName, CCInitParams.VIDName);

    /* Init */
    ErrCode = STCC_Init(DeviceName, &CCInitParams);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_NO_MEMORY:
            API_ErrorCount++;
            strcat(CC_Msg, "No more memory available !\n");
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(CC_Msg, "One parameter is invalid !\n");
            break;
        case ST_ERROR_ALREADY_INITIALIZED:
            API_ErrorCount++;
            strcat(CC_Msg, "Device already initialized !\n");
            break;
        case STCC_ERROR_VBI_UNKNOWN:
            API_ErrorCount++;
            strcat(CC_Msg, "Vbi unknown!\n");
            break;
        case STCC_ERROR_VBI_ACCESS:
            API_ErrorCount++;
            strcat(CC_Msg, "Vbi access!\n");
            break;
        case STCC_ERROR_EVT_REGISTER:
            API_ErrorCount++;
            strcat(CC_Msg, "Evt register!\n");
            break;
        case STCC_ERROR_EVT_SUBSCRIBE:
            API_ErrorCount++;
            strcat(CC_Msg, "Evt subscribe!\n");
            break;
        case STCC_ERROR_TASK_CALL:
            API_ErrorCount++;
            strcat(CC_Msg, "Task call!\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n", ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}
/*-------------------------------------------------------------------------
 * Function :
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_SetService(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 Var;

    UNUSED_PARAMETER(result_sym_p);
    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p,
            STCC_OUTPUT_MODE_DENC|STCC_OUTPUT_MODE_UART_DECODED, &Var);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p,"Expected service");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_SetService(%d): %d : ", CCHndl,Var);
/*    ErrCode = STCC_SetDTVService(CCHndl,Var);*/ErrCode=0;
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : CC_SetOutpuMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_SetOutputMode(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 Var;

    UNUSED_PARAMETER(result_sym_p);

    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p,
            STCC_OUTPUT_MODE_DENC|STCC_OUTPUT_MODE_UART_DECODED, &Var);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p,"Expected Ouput mode (default DENC+debug)");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_SetOutputMode(%d): %d : ", CCHndl,Var);
    ErrCode = STCC_SetOutputMode(CCHndl,Var);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : CC_SetFormatMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_SetFormatMode(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    S32 Var;

    UNUSED_PARAMETER(result_sym_p);
    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p,STCC_FORMAT_MODE_DETECT ,
                         &Var);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p,"Expected Format mode ");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_SetFormatMode(%d): %d : ", CCHndl,Var);
    ErrCode = STCC_SetFormatMode(CCHndl,Var);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}
/*-------------------------------------------------------------------------
 * Function : CC_GetFormatMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_GetFormatMode(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STCC_FormatMode_t Format;

    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_GetFormatMode(%d): ", CCHndl);
    ErrCode = STCC_GetFormatMode(CCHndl,&Format);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            if (Format == 0)
            {
                strcat(CC_Msg, "   No format set or detected.\n");
            }
            else
            {
                if((Format & STCC_FORMAT_MODE_DTVVID21) ==
                            STCC_FORMAT_MODE_DTVVID21)
                {
                    strcat(CC_Msg, "   Format DTV-Vid21 DirecTV is set.\n");
                }
                if((Format & STCC_FORMAT_MODE_DVB) ==
                            STCC_FORMAT_MODE_DVB)
                {
                    strcat(CC_Msg, "   Format DVB is set.\n");
                }
                if((Format & STCC_FORMAT_MODE_EIA708) ==
                            STCC_FORMAT_MODE_EIA708)
                {
                    strcat(CC_Msg, "   Format EIA-708   is set.\n");
                }
                if((Format & STCC_FORMAT_MODE_DVS157) ==
                            STCC_FORMAT_MODE_DVS157)
                {
                    strcat(CC_Msg, "   Format DVS-157   is set.\n");
                }
                if((Format & STCC_FORMAT_MODE_UDATA130) ==
                            STCC_FORMAT_MODE_UDATA130)
                {
                    strcat(CC_Msg, "   Format Udata-130 is set.\n");
                }
                if((Format & STCC_FORMAT_MODE_EIA608) ==
                            STCC_FORMAT_MODE_EIA608)
                {
                    strcat(CC_Msg, "   Format  EIA-608 is set.\n");
                }
            }
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}


/*-------------------------------------------------------------------------
 * Function : CC_GetOutputMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_GetOutputMode(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;
    STCC_OutputMode_t Output;

    UNUSED_PARAMETER(result_sym_p);

    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }


    sprintf(CC_Msg, "CC_GetOutputMode(%d): ", CCHndl);
    ErrCode = STCC_GetOutputMode(CCHndl,&Output);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            if (Output == 0)
            {
                strcat(CC_Msg, "   No Output is set.\n");
            }
            else
            {
                if((Output & STCC_OUTPUT_MODE_DENC) ==
                            STCC_OUTPUT_MODE_DENC)
                {
                    strcat(CC_Msg, "   Output to denc  is set.\n");
                }
                if((Output &  STCC_OUTPUT_MODE_EVENT)==
                            STCC_OUTPUT_MODE_EVENT)
                {
                    strcat(CC_Msg, "   Output to event is set.\n");
                }
                if((Output & STCC_OUTPUT_MODE_UART_DECODED) ==
                            STCC_OUTPUT_MODE_UART_DECODED)
                {
                    strcat(CC_Msg, "   Output to uart  is set.\n");
                }
#if 0
                if((Output & STCC_OUTPUT_MODE_DTV_BMP) ==
                            STCC_OUTPUT_MODE_DTV_BMP)
                {
                    strcat(CC_Msg, "   Output to bitmap is set.\n");
                }
#endif
            }
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}





/*-------------------------------------------------------------------------
 * Function : CC_Term
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_Term(parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    ST_ErrorCode_t ErrCode;
    STCC_TermParams_t CCTermParams;
    BOOL RetErr;
    U32  LVar;

    UNUSED_PARAMETER(result_sym_p);
    /* get device name */
    DeviceName[0] = '\0';

    RetErr = STTST_GetString(pars_p, "CLOSEDCAPTION", DeviceName,
                                             sizeof(DeviceName));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 0, (S32*)&LVar);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p,
                "Expected force terminate (default 0=FALSE, 1=TRUE)");
        return(TRUE);
    }
    CCTermParams.ForceTerminate = (BOOL)LVar;

    sprintf(CC_Msg, "CC_Term(%s): ", DeviceName);
    ErrCode = STCC_Term(DeviceName, &CCTermParams);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(CC_Msg, "One parameter is invalid !\n");
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(CC_Msg, "Decoder is not initialized !\n");
            break;
        case ST_ERROR_OPEN_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Still open handle !\n");
            break;
        case STCC_ERROR_VBI_ACCESS:
            API_ErrorCount++;
            strcat(CC_Msg, "Vbi access!\n");
            break;
        case STCC_ERROR_EVT_REGISTER:
            API_ErrorCount++;
            strcat(CC_Msg, "Evt register!\n");
            break;
        case STCC_ERROR_EVT_SUBSCRIBE:
            API_ErrorCount++;
            strcat(CC_Msg, "Evt subscribe!\n");
            break;
        case STCC_ERROR_TASK_CALL:
            API_ErrorCount++;
            strcat(CC_Msg, "Task call!\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n", ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : CC_Open
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_Open(parse_t *pars_p, char *result_sym_p)
{
    char DeviceName[80];
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    STCC_OpenParams_t CCOpenParams;


    /* get device */
    DeviceName[0] = '\0';
    RetErr = STTST_GetString(pars_p, "CLOSEDCAPTION", DeviceName,
                                                     sizeof(DeviceName));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected DeviceName");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_Open(%s): ", DeviceName);
    ErrCode = STCC_Open(DeviceName, &CCOpenParams, &CCHndl);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
     switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            STTST_AssignInteger(result_sym_p, CCHndl, FALSE);
            sprintf(CC_Msg, "%sOk\nHdl %d\n", CC_Msg, CCHndl);
            break;
        case ST_ERROR_BAD_PARAMETER:
            API_ErrorCount++;
            strcat(CC_Msg, "One parameter is invalid !\n");
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            API_ErrorCount++;
            strcat(CC_Msg, "Decoder is not initialized !\n");
            break;
        case ST_ERROR_NO_FREE_HANDLES:
            API_ErrorCount++;
            strcat(CC_Msg, "No free handles !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n", ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : CC_Close
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_Close(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_Close(%d): ", CCHndl);
    ErrCode = STCC_Close(CCHndl);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : CC_Start
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_Start(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);

    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_Start(%d): ", CCHndl);
    ErrCode = STCC_Start(CCHndl);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        case STCC_ERROR_DECODER_RUNNING:
            API_ErrorCount++;
            strcat(CC_Msg, "Decoder running !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : CC_Stop
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_Stop(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode;
    BOOL RetErr;

    UNUSED_PARAMETER(result_sym_p);
    /* Close a VBI device connection */

    RetErr = STTST_GetInteger(pars_p, CCHndl, (S32*)&CCHndl);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected handle");
        return(TRUE);
    }

    sprintf(CC_Msg, "CC_Stop(%d): ", CCHndl);
    ErrCode = STCC_Stop(CCHndl);
    STTST_AssignInteger("ERRORCODE", ErrCode, FALSE);
    RetErr = TRUE;
    switch (ErrCode)
    {
        case ST_NO_ERROR :
            RetErr = FALSE;
            strcat(CC_Msg, "Ok\n");
            break;
        case ST_ERROR_INVALID_HANDLE:
            API_ErrorCount++;
            strcat(CC_Msg, "Invalid handle !\n");
            break;
        case STCC_ERROR_DECODER_STOPPED:
            API_ErrorCount++;
            strcat(CC_Msg, "Decoder stopped !\n");
            break;
        default:
            API_ErrorCount++;
            sprintf(CC_Msg, "Unexpected error [%X] !\n",  ErrCode);
            break;
    }
    STTST_Print((CC_Msg));
    return (API_EnableError ? RetErr : FALSE);
}

#ifdef TRACE_UART
#if 0
/*-------------------------------------------------------------------------
 * Function : CC_TraceInfo
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if Ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL CC_TraceInfo(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr = FALSE;

    UNUSED_PARAMETER(result_sym_p);

    STTST_Print(("Closed caption : NbMaxSlotsReached = %d    NbFreeSlotDone = %d  NbUserDataEvtReceived = %d \n",
                     NbMaxSlotsReached, NbFreeSlotDone , NbUserDataEvtReceived ));
    NbMaxSlotsReached = 0;
    NbFreeSlotDone = 0;
    NbVSyncTopReceived = 0;
    NbUserDataEvtReceived = 0;
    STTST_Print(("Closed caption : Reset NbMaxSlotsReached = %d    NbFreeSlotDone = %d  NbUserDataEvtReceived = %d \n",
                     NbMaxSlotsReached, NbFreeSlotDone , NbUserDataEvtReceived ));

    return (API_EnableError ? RetErr : FALSE);
}
#endif /* #ifdef TRACE_UART */
#endif /* #ifdef TRACE_UART */


/*#######################################################################*/
/*########################## REGISTER COMMANDS ##########################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : CC_MacroInit
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL CC_InitCommand(void)
{
    BOOL RetErr=FALSE;

    /* API functions */

    RetErr  = STTST_RegisterCommand("CaptionInit", CC_Init,
     "<DeviceName><OuputMode><VTGName><VideoName><VBIName> Init closed caption");
    RetErr |= STTST_RegisterCommand("CaptionTerm", CC_Term,
            "<DeviceName><ForceTerm> Terminate closed caption");
    RetErr |= STTST_RegisterCommand("CaptionOpen", CC_Open,
            "<DeviceName> Open closed caption" );
    RetErr |= STTST_RegisterCommand("CaptionClose", CC_Close,
            "<Handle> Close closed caption" );
    RetErr |= STTST_RegisterCommand("CaptionStart", CC_Start,
            "<Handle> Start closed caption" );
    RetErr |= STTST_RegisterCommand("CaptionStop", CC_Stop,
            "<Handle> Stop closed caption" );
    RetErr |= STTST_RegisterCommand("CaptionSetFormat",CC_SetFormatMode ,
            "<Handle> <format(s)>" );
    RetErr |= STTST_RegisterCommand("CaptionSetOutput", CC_SetOutputMode,
            "<Handle> <output(s)>" );
    RetErr |= STTST_RegisterCommand("CaptionSetService", CC_SetService,
            "<Handle> <num service>" );
    RetErr |= STTST_RegisterCommand("CaptionGetOutput", CC_GetOutputMode,
            "<Handle>" );
    RetErr |= STTST_RegisterCommand("CaptionGetFormat",CC_GetFormatMode ,
            "<Handle>" );
#ifdef ST_OSLINUX
    RetErr |= STTST_AssignString ("DRV_PATH_CC", "cc/", TRUE);
#else
    RetErr |= STTST_AssignString ("DRV_PATH_CC", "", TRUE);
#endif

    STTST_RegisterCommand("CaptionResetCounters",CC_ResetDebugCounters, "< >" );
    STTST_RegisterCommand("CaptionPrintCounters",CC_PrintDebugCounters, "< >" );


#ifdef TRACE_UART
#if 0
    RetErr |= STTST_RegisterCommand("CC_TraceInfo",CC_TraceInfo ,
            "Gives statistics" );
#endif
#endif /* #ifdef TRACE_UART */

    if (RetErr == TRUE)
    {
        sprintf(CC_Msg,  "CC_Command()    : macros registrations failure !\n");
    }
    else
    {
        sprintf(CC_Msg,  "CC_Command() Ok,\trevision=%s\n",STCC_GetRevision());
    }
    STTBX_Print((CC_Msg));
    return(RetErr ? FALSE : TRUE);
} /* end VBI_MacroInit */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL Test_CmdStart(void)
{
    BOOL RetOk;

    RetOk = CC_InitCommand();
    RetOk = VID_RegisterCmd2();

    if ( RetOk )
    {
        RetOk = VID_Inj_RegisterCmd();
    }
#if defined (ST_OSLINUX)
    VIDTEST_Init();
#endif

#if 0
    BlitInit();

    /* demo rendering init */
    {
        STCC_DTVInitParams_t STCC_DTVInitParams;

        strcpy(STCC_DTVInitParams.BlitterName , "BLITTER");
        strcpy(STCC_DTVInitParams.VtgName     , STVTG_DEVICE_NAME);
        strcpy(STCC_DTVInitParams.EvtName     , STEVT_DEVICE_NAME);
        strcpy(STCC_DTVInitParams.CCName     , "CLOSEDCAPTION");
        STCC_DTVInitParams.RenderBitmap_p     = &BitmapUnknown;
        STCC_DTVInitParams.CPUPartition_p     = SystemPartition_p;
        STCC_DTVInitParams.AVMEM_Partition    = AvmemPartitionHandle;
        stcc_DTVInit(&STCC_DTVInitParams);
    }

#endif
    return(RetOk);
}




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
    UNUSED_PARAMETER(ptr);

    STAPIGAT_Init();
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
#ifdef ST_OS21
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
#endif

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

    os20_main(NULL);

    exit (0);

}
/*###########################################################################*/


