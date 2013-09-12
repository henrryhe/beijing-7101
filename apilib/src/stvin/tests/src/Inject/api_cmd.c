/************************************************************************
COPYRIGHT (C) STMicroelectronics 2001

File name   : api_cmd.c
Description : Miscellaneous commands

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
#include <device.h>

#include "stddefs.h"
#include "stlite.h"
#include "debug.h"
#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants --- */

/* --- Macros ---*/

/* --- Extern functions --- */

/* --- Global variables --- */
BOOL API_EnableError = TRUE;
U32  API_ErrorCount = 0;
S16  API_TestMode = TEST_MODE_MONO_HANDLE ; /* mono or multi handle    */

/* --- Local variables --- */
static BOOL APIInitDone = FALSE;     /* flag for macro initialization */
static char API_Msg[200];            /* text for trace */


/*#######################################################################*/
/*########################### API FUNCTIONS #############################*/
/*#######################################################################*/


/*-------------------------------------------------------------------------
 * Function : API_ConvIntToStr
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
BOOL API_ConvIntToStr( STTST_Parse_t *pars_p, char *result_sym_p )
{
    U32 LVar, Base;
    char IsStr[80], StringTmp[80];
    BOOL RetErr;

    RetErr = STTST_GetInteger(pars_p, 0, (S32*)&LVar );
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected number");
        return(TRUE);
    }

    RetErr = STTST_GetString(pars_p, "", StringTmp, sizeof(StringTmp));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected Symbol/String to add before");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 10, (S32*)&Base );
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected base");
        return(TRUE);
    }

    RetErr = STTST_EvaluateString( StringTmp, IsStr , Base);
    if(RetErr == FALSE)
    {
        STTST_AssignString(StringTmp, IsStr, FALSE);
    }
    sprintf(StringTmp, "%s%02d", StringTmp, LVar);

    STTST_AssignString(result_sym_p, StringTmp, FALSE);
    return (FALSE);
}


/*-------------------------------------------------------------------------
 * Function : API_EvaluateStr
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
BOOL API_EvaluateStr( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char   IsStr[80], StringTmp[80], StrToEval[80], cara, *ptr;
    U32    Base, NoParam, CountEval=0, CntFirst=0, CntLast=0;
    S32    IsInt;
    double IsDouble;
    BOOL   RetErr;

    RetErr = STTST_GetString(pars_p, "", StringTmp, sizeof(StringTmp));
    if ((RetErr == TRUE) || (StringTmp[0]==0))
    {
        STTST_TagCurrentLine(pars_p, "Expected string");
        return(TRUE);
    }

    cara='A';
    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&NoParam);
    while((CountEval!=NoParam) && (cara!='\n'))
    {
        CntFirst=CntLast;
        cara=StringTmp[CntLast++];
        while((cara!=' ') && (cara!='\n') && (cara!=',') && (cara!='/') && (CntLast<80)){
            cara=StringTmp[CntLast++];
        }
        CountEval++;
    }
    memcpy(StrToEval, &(StringTmp[CntFirst]), CntLast-CntFirst-1);
    StrToEval[CntLast-CntFirst-1] = 0;

    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected param number to evaluate (def 1)\n"
                             "\tSeparation caracters are: space, coma, slash");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 10, (S32*)&Base);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected base (default 10)");
        return(TRUE);
    }

    RetErr = STTST_EvaluateInteger( StrToEval, &IsInt, Base);
    if (RetErr == FALSE)
    {
        STTST_AssignInteger(result_sym_p, IsInt, FALSE);
    }
    else
    {
        RetErr = STTST_EvaluateFloat( StrToEval, &IsDouble);
        if (RetErr == FALSE)
        {
            STTST_AssignFloat(result_sym_p, IsDouble, FALSE);
        }
        else
        {
            RetErr = STTST_EvaluateString( StrToEval, IsStr, sizeof(IsStr));
            if(RetErr == FALSE)
            {
                STTST_AssignString(result_sym_p, IsStr, FALSE);
            }
            else
            {
                IsInt = (U32)strtoul(StrToEval, &ptr, Base);
                if(ptr == StrToEval)
                    STTST_AssignString(result_sym_p, StrToEval, FALSE);
                else
                    STTST_AssignInteger(result_sym_p, IsInt, FALSE);
            }
        }
    }
    return(FALSE);
}


/*-------------------------------------------------------------------------
 * Function : API_CreateSymbol
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
BOOL API_CreateSymbol( STTST_Parse_t *pars_p, char *result_sym_p )
{
    S32  IsInt, Value;
    char Name[80], StringTmp[80];
    BOOL RetErr;
    U32  Base;

    RetErr = STTST_GetString(pars_p, "TST", Name, sizeof(Name));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected name of symbol");
        return(TRUE);
    }

    RetErr = STTST_GetItem(pars_p, "0", StringTmp, sizeof(StringTmp));
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected symbol or number");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 0, &Value );
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected symbol or value");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 10, (S32*)&Base);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected base (default 10)");
        return(TRUE);
    }

    RetErr = STTST_EvaluateInteger( StringTmp, &IsInt, Base);
    if (RetErr == FALSE)
    {
        sprintf(Name, "%s%02d", Name, IsInt);
    }
    else
    {
        sprintf(Name, "%s%s", Name, StringTmp);
    }

    STTST_AssignInteger(Name, Value, FALSE);
    return (FALSE);
}


/*-------------------------------------------------------------------------
 * Function : API_CheckErrors
 *            Check number of errors (if a value is given)
 *            Reset error count
 * Input    : *pars_p, *result_sym_p
 * Output   : API_ErrorCount
 * Return   : current state of API_EnableError
 * ----------------------------------------------------------------------*/
BOOL API_CheckErrors( STTST_Parse_t *pars_p, char *result_sym_p )
{
    S32  NbOfExpectedErrors;
    BOOL RetErr;

    /* get nb of expected errors */
    RetErr = STTST_GetInteger(pars_p, -1, &NbOfExpectedErrors );
    if (RetErr == TRUE )
    {
        STTST_TagCurrentLine(pars_p, "expected NbOfExpectedErrors");
        return(TRUE);
    }

    if ( NbOfExpectedErrors >= 0 )
    {
        if ( API_ErrorCount == NbOfExpectedErrors )
        {
            STTST_Print(( "Test sequence SUCCESSFULL\n"));
        }
        else
        {
            sprintf(API_Msg,"Test sequence FAILED (%d errors instead of %d)!\n",
                    API_ErrorCount, NbOfExpectedErrors);
            STTST_Print(( API_Msg));
        }
    }

    STTST_AssignInteger(result_sym_p, (U32)API_ErrorCount, false);
    API_ErrorCount = 0; /* reset count */
    return(FALSE);
} /* end API_CheckErrors */


/*-------------------------------------------------------------------------
 * Function : API_GetErrorCount
 *            Returns number of errors
 * Input    : *pars_p, *result_sym_p
 * Output   : API_ErrorCount
 * Return   : current state of API_EnableError
 * ----------------------------------------------------------------------*/
BOOL API_GetErrorCount( STTST_Parse_t *pars_p, char *result_sym_p )
{
    STTST_AssignInteger(result_sym_p, (U32)API_ErrorCount, false);

    return(FALSE);
} /* end API_GetErrorCount */


/*-------------------------------------------------------------------------
 * Function : API_Error
 *            if arg=true, each command must return true or false
 *                         (macro execution stops if an error occurs)
 *            if arg=false, each command always return false
 *                         (macro execution is never interrupted)
 *            it has no effect on error reporting
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : always FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Error( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    U32 LVar;

    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine(pars_p, "expected 0=Ignore error 1=Stop if error");
        return(TRUE);
    }
    if ( LVar == 0 )
    {
        API_EnableError = FALSE;
        STTST_Print(( "Error detection : disable\n"));
    }
    else
    {
        API_EnableError = TRUE;
        STTST_Print(( "Error detection : enable\n"));
    }
    return (FALSE);
} /* end API_Error */

/*-------------------------------------------------------------------------
 * Function : API_Question
 * Input    : *pars_p, *result_sym_p
 * Output   : *result_sym_p : contains 1 if Y replied, 0 else
 * Return   : Always FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Question( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char Answer[80], Question[80], Buff[120];
    U32 NbBytes, AnswerInt;
    BOOL RetErr;
    U32 RetCode;

    RetCode=0;

#ifdef STTBX_INPUT
    /* get question text */
    memset( Question, 0, sizeof(Question));

    RetErr = STTST_GetString(pars_p, "", Question, sizeof(Question)-2 );
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected string question");
        return(TRUE);
    }

    if (strlen(Question)==0)
    {
        strcpy( Question, "Is it OK (Y/N) ?");
    }

    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&AnswerInt);
    if (RetErr == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected integer answer 0=FALSE=No 1=TRUE=Yes(default)");
        return(TRUE);
    }

    NbBytes = strlen(Question);
    Question[NbBytes] = '\n' ;
    /* repeat question while no answer... */
    STTST_Print(( Question));
    memset( Answer, 0, sizeof(Answer));
    NbBytes = 0;
    while ( NbBytes==0 )
    {
        NbBytes = STTBX_InputStr( Answer, sizeof(Answer) );
    }
    sprintf( Buff, "Answer=[%s] len=%d characters\n", Answer, strlen(Answer));
    STTST_Print(( Buff));

    if ((Answer[0]=='Y') || (Answer[0]=='y'))
    {
        if(AnswerInt == TRUE){
            STTST_AssignInteger("ERRORCODE", 0, FALSE);
        }
        else{
            STTST_AssignInteger("ERRORCODE", 1, FALSE);
            API_ErrorCount++;
            RetErr = TRUE;
        }
    }
    else
    {
        if(AnswerInt == FALSE){
            STTST_AssignInteger("ERRORCODE", 0, FALSE);
        }
        else{
            STTST_AssignInteger("ERRORCODE", 1, FALSE);
            API_ErrorCount++;
            RetErr = TRUE;
        }
    }
#else
    STTST_Print(( "STTBX_INPUT disabled, cannot process user inputs\n"));
#endif
    return ( API_EnableError ? RetErr : FALSE );

} /* end API_Question */


/*-------------------------------------------------------------------------
 * Function : API_Report for trace report
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Report( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char  ReportStr[80], ChapterStr[80], TrvStr[80], *ptr;
    S32   LVar;
    BOOL  RetErr;

    RetErr = STTST_GetString(pars_p, "", ChapterStr, sizeof(ChapterStr));
    if (RetErr==TRUE)
    {
        STTST_TagCurrentLine(pars_p, "Expected string of chapter number & header");
        return(TRUE);
    }

    RetErr = STTST_GetItem(pars_p, "-1", TrvStr, sizeof(TrvStr));

    RetErr = STTST_EvaluateInteger( TrvStr, &LVar, 10);
    if (RetErr == TRUE)
    {
        LVar = (S32)strtoul(TrvStr, &ptr, 10);
        if(ptr == TrvStr)
        {
            STTST_TagCurrentLine(pars_p, "Expected integer (default=-1 header only, 1=Successful, >0 Failed");
            return(TRUE);
        }
    }

    memset(ReportStr, ' ', sizeof(ReportStr));
    memcpy(&ReportStr[2], ChapterStr, strlen(ChapterStr));
    ReportStr[0]='*';
    ReportStr[1]=' ';

    /* Report */
    if(LVar == -1)
        STTST_Print(("\n"));
    STTST_Print(("***********************************************\n"));

    switch(LVar){
        case -1:
            strcpy(&ReportStr[46], "*\n");
            break;

        case 0:
            strcpy(&ReportStr[33], "* SUCCESSFUL *\n");
            break;

        default:
            strcpy(&ReportStr[33], "* FAILED !!  *\n");
            break;
    }

    STTST_Print((ReportStr));
    STTST_Print(("***********************************************\n"));

    if(LVar != -1)
        STTST_Print(("\n"));

    return ( API_EnableError ? RetErr : FALSE );

} /* end API_Report */


/*-------------------------------------------------------------------------
 * Function : API_Trace
 *            Start or stop video trace
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Trace( STTST_Parse_t *pars_p, char *result_sym_p )
{
U32 LVar;
BOOL RetErr;

    /* get flag (0=disable 1=enable) */
    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar );
    if ( RetErr == TRUE )
    {
        STTST_TagCurrentLine(pars_p, "expected Trace (0=disable,1=enable)");
    }
    if ( LVar == 0 )
    {
        /* disable trace */
        /*API_Device = STTBX_OUTPUT_DEVICE_NULL ; */
        STTST_Print(( "Trace disable (device set to null) \n"));  /* report it */
    }
    else
    {
        /* enable trace */
        /*API_Device = STTBX_OUTPUT_DEVICE_NORMAL; */
        STTST_Print(( "Trace enable (device set to normal) \n"));
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end API_Trace */

/*-------------------------------------------------------------------------
 * Function : API_SetTestMode
 *            Set test mode
 *            0=mono-handle (facility: no handle on VID__XXX command lines)
 *            1=multi-handles (handle required on VID_XXX command-lines)
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL API_SetTestMode( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    S32 SMode;

    RetErr = STTST_GetInteger( pars_p, TEST_MODE_MONO_HANDLE, &SMode );
    if ( RetErr == TRUE ||
         ( SMode != TEST_MODE_MONO_HANDLE && SMode != TEST_MODE_MULTI_HANDLES ) )
    {
        sprintf( API_Msg, "expected Mode %d=mono-handle, %d=multi-handles (handle required on command-lines)",
                 TEST_MODE_MONO_HANDLE, TEST_MODE_MULTI_HANDLES );
        STTST_TagCurrentLine( pars_p, API_Msg );
    }
    else
    {
        API_TestMode = SMode;
    }
    if ( API_TestMode == TEST_MODE_MONO_HANDLE )
    {
        STTBX_Print(("Test mode set to mono-handle (handles are not requested on command lines)\n"));
    }
    else
    {
        STTBX_Print(("Test mode set to multi-handles (handles are requested on command lines)\n"));
    }
    return ( FALSE );

} /* end of API_SetTestMode() */

/*-------------------------------------------------------------------------
 * Function : API_Break
 *            Command to stop a macro
 *            it has no effect on error reporting
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : always TRUE
 * ----------------------------------------------------------------------*/
BOOL API_Break( STTST_Parse_t *pars_p, char *result_sym_p )
{
    return (TRUE);
} /* end API_Break */


/*-------------------------------------------------------------------------
 * Function : API_DeviceId
 *            get the device identification (ID) for the current device.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL API_DeviceId( STTST_Parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    device_id_t devid;

    devid = device_id();
    sprintf(API_Msg, "Device 0x%x (%s)\n\n", devid.id, device_name(devid));
    STTBX_Print(( API_Msg ));
    return ( API_EnableError ? RetErr : FALSE );

} /* end of API_DeviceId */


/*-----------------------------------------------------------------------------
 * Function : API_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
BOOL API_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL ErrFound = TRUE;

    /* Error code returned */
    STTST_AssignInteger( "ERRORCODE", Error, FALSE);
    if(Error == ST_NO_ERROR)
    {
        strcat( Text, "Ok\n");
    }
    else
    {
        strcat( Text, "Failed ! ");
        switch ( Error )
        {
            case ST_ERROR_BAD_PARAMETER:
                strcat( Text, "(One parameter is invalid)\n");
                break;
            case ST_ERROR_NO_MEMORY:
                strcat( Text, "(No memory !)\n");
                break;
            case ST_ERROR_UNKNOWN_DEVICE:
                strcat( Text, "(Decoder is not initialized !)\n");
                break;
            case ST_ERROR_ALREADY_INITIALIZED:
                strcat( Text, "(Device already initialized !)\n");
                break;
            case ST_ERROR_NO_FREE_HANDLES:
                strcat( Text, "(No free handles !)\n");
                break;
            case ST_ERROR_OPEN_HANDLE:
                strcat( Text, "(Open handle error !)\n");
                break;
            case ST_ERROR_INVALID_HANDLE:
                strcat( Text, "(Invalid handle !)\n");
                break;
            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                strcat( Text, "(Feature not supported !)\n");
                break;
            case ST_ERROR_INTERRUPT_INSTALL:
                strcat( Text, "(Interrupt install !)\n");
                break;
            case ST_ERROR_INTERRUPT_UNINSTALL:
                strcat( Text, "(Interrupt uninstall !)\n");
                break;
            case ST_ERROR_TIMEOUT:
                strcat( Text, "(Timeout occured !)\n");
                break;
            case ST_ERROR_DEVICE_BUSY:
                strcat( Text, "(Device is busy !))\n");
                break;

            default:
                ErrFound = FALSE;
                break;
        }
    }
    return(ErrFound);
}

/*-------------------------------------------------------------------------
 * Function : API_InitCommand
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL API_InitCommand (void)
{
    BOOL RetErr;

    RetErr = FALSE;
    if ( APIInitDone==FALSE )
    {
        /* API functions */
        RetErr |= STTST_RegisterCommand( "API_INT2STR", API_ConvIntToStr,\
                                         "<num><str|symb> Concate symbol or string with number");
        RetErr |= STTST_RegisterCommand( "API_EVALSTR", API_EvaluateStr, \
                                         "<str><num> Evaluate string or symbol to value after num separation caracter");
        RetErr |= STTST_RegisterCommand( "API_CREATESYMB", API_CreateSymbol, \
                                         "<str><num><value> Create symbol str+num with value (num can be a symbol)");
        RetErr |= STTST_RegisterCommand( "API_CHECKERR", API_CheckErrors, \
                                         "<NbOfExpectedErrors> test & reset nb of errors");
        RetErr |= STTST_RegisterCommand( "API_ERROR", API_Error, \
                                         "<TRUE/FALSE> Enable/disable error detection");
        RetErr |= STTST_RegisterCommand( "API_GETERRORCOUNT", API_GetErrorCount, \
                                         "Returns the internal error counter value");
        RetErr |= STTST_RegisterCommand( "API_QUEST", API_Question, \
                                         "<Quest string><Expect answer> Ask quest, expect answer" \
                                         "default TRUE=Yes, 0=No");
        RetErr |= STTST_RegisterCommand( "API_REPORT", API_Report, \
                                         "<String><Number Success/Failed> Report");
        RetErr |= STTST_RegisterCommand( "API_TRACE", API_Trace, "<1|0> Set or stop trace");
        RetErr |= STTST_RegisterCommand( "API_BREAK", API_Break, "Always return TRUE to stop macros");
        RetErr |= STTST_RegisterCommand( "API_SETTESTMODE", API_SetTestMode, "<0|1> Set mode to mono-handle(0=default) or multi-handles(1)");
        RetErr |= STTST_RegisterCommand( "API_DEVICEID", API_DeviceId, "Read Device Identifier");

        /* API variables */
        RetErr |= STTST_AssignInteger ("ERR_NO_ERROR", ST_NO_ERROR, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_BAD_PARAMETER", ST_ERROR_BAD_PARAMETER, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_NO_MEMORY", ST_ERROR_NO_MEMORY, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_UNKNOWN_DEVICE", ST_ERROR_UNKNOWN_DEVICE, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_ALREADY_INITIALIZED", ST_ERROR_ALREADY_INITIALIZED, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_NO_FREE_HANDLES", ST_ERROR_NO_FREE_HANDLES, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_OPEN_HANDLE", ST_ERROR_OPEN_HANDLE, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_INVALID_HANDLE", ST_ERROR_INVALID_HANDLE, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_FEATURE_NOT_SUPPORTED", ST_ERROR_FEATURE_NOT_SUPPORTED, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_INTERRUPT_INSTALL", ST_ERROR_INTERRUPT_INSTALL, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_INTERRUPT_UNINSTALL", ST_ERROR_INTERRUPT_UNINSTALL, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_TIMEOUT", ST_ERROR_TIMEOUT, TRUE);
        RetErr |= STTST_AssignInteger ("ERR_DEVICE_BUSY", ST_ERROR_DEVICE_BUSY, TRUE);

        RetErr |= STTST_AssignInteger ("ERRORCODE", ST_NO_ERROR, FALSE);

#if defined(ST_5510)
        RetErr |= STTST_AssignString ("API_CHIPID", "5510", TRUE);
#elif defined(ST_5512)
        RetErr |= STTST_AssignString ("API_CHIPID", "5512", TRUE);
#elif defined(ST_5508)
        RetErr |= STTST_AssignString ("API_CHIPID", "5508", TRUE);
#elif defined(ST_5518)
        RetErr |= STTST_AssignString ("API_CHIPID", "5518", TRUE);
#elif defined(ST_5514)
        RetErr |= STTST_AssignString ("API_CHIPID", "5514", TRUE);
#elif defined(ST_7015)
        RetErr |= STTST_AssignString ("API_CHIPID", "7015", TRUE);
#elif defined(ST_7020)
        RetErr |= STTST_AssignString ("API_CHIPID", "7020", TRUE);
#elif defined(ST_GX1)
        RetErr |= STTST_AssignString ("API_CHIPID", "GX1", TRUE);
#else
#error Add defined for chip used
#endif
    }

    return( RetErr ? FALSE : TRUE);
} /* end API_MacroInit */

/*#######################################################################*/
/*########################## GLOBAL FUNCTIONS ###########################*/
/*#######################################################################*/

BOOL API_RegisterCmd()
{
    BOOL RetOk;

    RetOk = API_InitCommand();
    if ( RetOk )
    {
        sprintf( API_Msg, "API_RegisterCmd()  \t: ok\n" );
    }
    else
    {
        sprintf( API_Msg, "API_RegisterCmd()  \t: failed !\n" );
    }
    STTST_Print((API_Msg));

    return(RetOk);
}



