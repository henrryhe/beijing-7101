/************************************************************************
File name   : api_cmd.c

Description : Miscellaneous commands

COPYRIGHT (C) STMicroelectronics 2004

Date          Modification                                    Initials
----          ------------                                    --------
11 Jun 2002   Add Path variables (C + Testtool)                HSdLM
28 Jun 2002   Improve API_CheckErrors and API_Question, add
 *            API_IncrErrors and API_Pollkey for STVID tests
 *            harness needs.                                   CL/HSdLM
11 Oct 2002   Add support for 5517                             HSdLM
13 May 2003   Add support for stem7020. Add API_BOARDID.       CL/HSdLM
16 May 2003   Add support for 5528.                            HSdLM
27 Aug 2003   Add support for 5100.                            HSdLM
06 Nov 2003   Add support for espresso                         CL
16 Jun 2004   Add support for 7710.                            GGi
16 Sep 2004   Add support for ST40/OS21                        MH
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef ST_OS20
#include <device.h>
#endif /* ST_OS20 */
#ifdef ST_OS21
#include "stlite.h"
#endif /* ST_OS21 */
#include "testcfg.h"


#ifdef USE_API_TOOL

#include "stddefs.h"

#include "stdevice.h"
#include "stsys.h"
#include "stcommon.h"

#ifdef ST_OSLINUX
#include "compat.h"
#else
#ifdef ST_OS20
#include "debug.h"
#endif /* ST_OS20 */
#ifdef ST_OS21
#include "os21debug.h"
#endif /* ST_OS21 */
#endif

#include "sttbx.h"
#include "testtool.h"
#include "api_cmd.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#if defined(ST_7100)|| defined(ST_7109)
#define DEVICE_ID 0x0
#ifndef SYS_CFG_BASE_ADDRESS
#ifdef ST_OSLINUX
#define SYS_CFG_BASE_ADDRESS  SysCfgMap.MappedBaseAddress
#else
#define SYS_CFG_BASE_ADDRESS  CFG_BASE_ADDRESS
#endif /* ST_OSLINUX */
#endif
#endif /*7100 || 7109*/
/* Private Variables (static)------------------------------------------------ */

static BOOL APIInitDone = FALSE;     /* flag for macro initialization */
static char API_Msg[200];            /* text for trace */

/* Global Variables --------------------------------------------------------- */

BOOL API_EnableError = TRUE;
U32  API_ErrorCount = 0;
S16  API_TestMode = TEST_MODE_MONO_HANDLE ; /* mono or multi handle    */
char RootPath[STTST_MAX_TOK_LEN];
char StapigatDataPath[105];

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
static BOOL API_InitCommand (void);


/* Functions ---------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : API_ConvIntToStr
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
BOOL API_ConvIntToStr( STTST_Parse_t *pars_p, char *result_sym_p )
{
    U32 LVar, Base;
    char IsStr[STTST_MAX_TOK_LEN], StringTmp[STTST_MAX_TOK_LEN];
    BOOL RetErr;

    RetErr = STTST_GetInteger(pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected number");
        return(TRUE);
    }

    RetErr = STTST_GetString(pars_p, "", StringTmp, sizeof(StringTmp));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected Symbol/String to add before");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 10, (S32*)&Base );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected base");
        return(TRUE);
    }

    RetErr = STTST_EvaluateString( StringTmp, IsStr , Base);
    if(!RetErr)
    {
        STTST_AssignString(StringTmp, IsStr, FALSE);
    }
    sprintf(StringTmp, "%s%02d", StringTmp, LVar);

    STTST_AssignString(result_sym_p, StringTmp, FALSE);
    return (FALSE);
}


/*-------------------------------------------------------------------------
 * Function : API_EvaluateStr
 *            Testtool command for evaluating one or more parameters of one
 *            string. Exemples :
 *              testtool> x=api_eval "one two three four" 2
 *              testtool> print x
 *              two
 *              testtool> x=api_eval "one two three four" 2*
 *              testtool> print x
 *              two three four
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
BOOL API_EvaluateStr( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char   IsStr[STTST_MAX_TOK_LEN], StringTmp[STTST_MAX_TOK_LEN], StrToEval[STTST_MAX_TOK_LEN], ParamsToEval[STTST_MAX_TOK_LEN], Carac, *ptr;
    U32    Base, ParamIndex, CountEval=0, CntFirst=0, CntLast=0;
    S32    IsInt;
    double IsDouble;
    BOOL   RetErr, EvaluteMultipleParams = FALSE;

    RetErr = STTST_GetString(pars_p, "", StringTmp, sizeof(StringTmp));
    if ((RetErr) || (StringTmp[0]==0))
    {
        STTST_TagCurrentLine(pars_p, "Expected string");
        return(TRUE);
    }

    Carac='A';
    RetErr = STTST_GetItem(pars_p, "1", ParamsToEval, sizeof(ParamsToEval));
    /* Find the star */
    if(ParamsToEval[strlen(ParamsToEval) - 1] == '*')
    {
        EvaluteMultipleParams = TRUE;
        ParamsToEval[strlen(ParamsToEval) - 1] = '\0';
    }
    RetErr = STTST_EvaluateInteger( ParamsToEval, (S32*)&ParamIndex, 10);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected param number to evaluate (def 1)\n"
                             "\tSeparation characters are: space, coma & slash");
        return(TRUE);
    }

    /* Until ParamIndex not found or end of string */
    while((CountEval!=ParamIndex) && (Carac!='\0') && (CntLast<STTST_MAX_TOK_LEN))
    {
        CntFirst=CntLast;
        Carac=StringTmp[CntLast++];
        while((Carac!=' ') && (Carac!='\n') && (Carac!=',') && (Carac!='/') && (Carac!='\0') && (CntLast<STTST_MAX_TOK_LEN))
        {
            Carac=StringTmp[CntLast++];
        }
        CountEval++;
    }
    /* ParamIndex found ? */
    if(CountEval==ParamIndex)
    {
        if(EvaluteMultipleParams)
        {
            strcpy(StrToEval, &(StringTmp[CntFirst]));
        }
        else
        {
            memcpy(StrToEval, &(StringTmp[CntFirst]), CntLast-CntFirst-1);
            StrToEval[CntLast-CntFirst-1] = 0;
        }
    }
    else
    {
        StrToEval[0] = '\0';
    }

    /* When evaluating many parameters, no evaluation is done. We just return
       the string that holds the params as they were entered */
    if(EvaluteMultipleParams)
    {
        STTST_AssignString(result_sym_p, StrToEval, FALSE);
    }
    else
    {
        RetErr = STTST_GetInteger(pars_p, 10, (S32*)&Base);
        if (RetErr)
        {
            STTST_TagCurrentLine(pars_p, "Expected base (default 10)");
            return(TRUE);
        }

        RetErr = STTST_EvaluateInteger( StrToEval, &IsInt, Base);
        if (!RetErr)
        {
            STTST_AssignInteger(result_sym_p, IsInt, FALSE);
        }
        else
        {
            RetErr = STTST_EvaluateFloat( StrToEval, &IsDouble);
            if (!RetErr)
            {
                STTST_AssignFloat(result_sym_p, IsDouble, FALSE);
            }
            else
            {
                RetErr = STTST_EvaluateString( StrToEval, IsStr, sizeof(IsStr));
                if(!RetErr)
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
    char Name[STTST_MAX_TOK_LEN], StringTmp[STTST_MAX_TOK_LEN];
    BOOL RetErr;
    U32  Base;
    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetString(pars_p, "TST", Name, sizeof(Name));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected name of symbol");
        return(TRUE);
    }

    RetErr = STTST_GetItem(pars_p, "0", StringTmp, sizeof(StringTmp));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected symbol or number");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 0, &Value );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected symbol or value");
        return(TRUE);
    }

    RetErr = STTST_GetInteger(pars_p, 10, (S32*)&Base);
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected base (default 10)");
        return(TRUE);
    }

    RetErr = STTST_EvaluateInteger( StringTmp, &IsInt, Base);
    if (!RetErr)
    {
        sprintf(Name, "%s%d", Name, IsInt);
    }
    else
    {
        sprintf(Name, "%s%s", Name, StringTmp);
    }

    STTST_AssignInteger(Name, Value, FALSE);
    return (FALSE);
} /* end of API_CreateSymbol() */


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
    char Msg[41];
    BOOL RetErr;

    /* get nb of expected errors */
    RetErr = STTST_GetInteger(pars_p, -1, &NbOfExpectedErrors );
    if (RetErr )
    {
        STTST_TagCurrentLine(pars_p, "expected NbOfExpectedErrors");
        return(TRUE);
    }

    if ( NbOfExpectedErrors >= 0 )
    {
        /* get test or macro name */
        RetErr = cget_string( pars_p, "", Msg, sizeof(Msg) );
        if (RetErr)
        {
            STTST_TagCurrentLine( pars_p, "expected macro name (40 car. max)" );
            return(TRUE);
        }
        if ( Msg[0]=='\0' )
        {
            sprintf( API_Msg, "Test sequence");
        }
        else
        {
            /* use the keyword 'result' for trace with command LOG ,"R" option */
            sprintf( API_Msg, "### %s result :", Msg);
        }
        if ( API_ErrorCount == ((U32)NbOfExpectedErrors) )
        {
            sprintf(API_Msg, "%s SUCCESSFUL\n", API_Msg);
        }
        else
        {
            sprintf(API_Msg, "%s FAILED (%d errors instead of %d) !\n",
                   API_Msg, API_ErrorCount, NbOfExpectedErrors);
        }
        STTST_Print(( API_Msg));
    }

    STTST_AssignInteger(result_sym_p, (U32)API_ErrorCount, FALSE);
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
    UNUSED_PARAMETER(pars_p);

    STTST_AssignInteger(result_sym_p, (U32)API_ErrorCount, FALSE);

    return(FALSE);
} /* end API_GetErrorCount */


/*-------------------------------------------------------------------------
 * Function : API_Error
 *            if arg=true, each command must return true or FALSE
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
    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar );
    if ( RetErr )
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
 * Function : API_IncrErrors
 *            Get the number of errors to add to API_ErrorCount
 * Input    : *pars_p, *result_sym_p
 * Output   : new API_ErrorCount
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL API_IncrErrors( STTST_Parse_t *pars_p, char *result_sym_p )
{
    S32 LVar;
    BOOL RetErr;

    /* get number of errors to add to API_ErrorCount */
    RetErr = STTST_GetInteger( pars_p, 0, &LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected number of errors (0, 1, ...) to add to API_ErrorCount" );
    }
    else
    {
        API_ErrorCount+=(int)LVar;
    }
    STTST_AssignInteger(result_sym_p, API_ErrorCount, FALSE);
	return (FALSE);

} /* end of API_IncrErrors() */

/*-------------------------------------------------------------------------
 * Function : API_Pollkey
 *            press a key to exit of a macro
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL API_Pollkey(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL Hit;
#ifdef STTBX_INPUT
    char Answer[10];
#endif /* STTBX_INPUT */
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    Hit = 0;
#ifdef STTBX_INPUT
    Hit = STTBX_InputPollChar(Answer);
#endif /* STTBX_INPUT */
    return(Hit);
} /* end of API_Pollkey() */

/*-------------------------------------------------------------------------
 * Function : API_Question
 * Input    : *pars_p, *result_sym_p
 * Output   : *result_sym_p : contains 1 if Y replied, 0 else
 * Return   : Always FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Question( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char Answer[STTST_MAX_TOK_LEN], Question[STTST_MAX_TOK_LEN], Buff[120];
    U32 NbBytes, AnswerInt;
    BOOL RetErr;
    U32 RetCode;

    RetCode=0;

#ifdef STTBX_INPUT
    /* get question text */
    memset( Question, 0, sizeof(Question));

    RetErr = STTST_GetString(pars_p, "", Question, sizeof(Question)-2 );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected string question");
        return(TRUE);
    }

    if (strlen(Question)==0)
    {
        strcpy( Question, "Is it OK (Y/N) ?");
    }

    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&AnswerInt);
    if (RetErr)
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
        if(AnswerInt != 0)
        {
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
        if(AnswerInt == 0)
        {
            STTST_AssignInteger("ERRORCODE", 0, FALSE);
        }
        else
        {
            STTST_AssignInteger("ERRORCODE", 1, FALSE);
            API_ErrorCount++;
            RetErr = TRUE;
        }
    }
#else
    STTST_Print(( "STTBX_INPUT disabled, cannot process user inputs\n"));
#endif
    if ( Answer[0]=='Y' || Answer[0]=='y' )
    {
        STTST_AssignInteger(result_sym_p, 'Y', FALSE); /* Yes in Uppercase */
    }
    else
    {
        STTST_AssignInteger(result_sym_p, Answer[0], FALSE);
    }
    return ( API_EnableError ? RetErr : FALSE );

} /* end API_Question */

/*-------------------------------------------------------------------------
 * Function : API_Prompt
 * Input    : *pars_p, *result_sym_p
 * Output   : *result_sym_p : contains the string we got from the input device
 * Return   : Always FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Prompt( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char Answer[STTST_MAX_TOK_LEN], Prompt[STTST_MAX_TOK_LEN];
    BOOL RetErr;

    memset( Answer, 0, sizeof(Answer));
#ifdef STTBX_INPUT
    /* get question text */
    memset( Prompt, 0, sizeof(Prompt));

    RetErr = STTST_GetString(pars_p, "", Prompt, sizeof(Prompt)-2 );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected string prompt");
        return(TRUE);
    }

    if (strlen(Prompt)==0)
    {
        strcpy( Prompt, "> ");
    }

    STTST_Print((Prompt));
    STTBX_InputStr( Answer, sizeof(Answer) );
#else
    STTST_Print(( "STTBX_INPUT disabled, cannot process user inputs\n"));
    RetErr = TRUE;
#endif
    STTST_AssignString(result_sym_p, Answer, FALSE);

    return ( API_EnableError ? RetErr : FALSE );

} /* end API_Prompt */


/*-------------------------------------------------------------------------
 * Function : API_Report for trace report
 *
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
BOOL API_Report( STTST_Parse_t *pars_p, char *result_sym_p )
{
    char  ReportStr[STTST_MAX_TOK_LEN], ChapterStr[STTST_MAX_TOK_LEN], TrvStr[STTST_MAX_TOK_LEN], *ptr;
    S32   LVar;
    BOOL  RetErr;
    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetString(pars_p, "", ChapterStr, sizeof(ChapterStr));
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected string of chapter number & header");
        return(TRUE);
    }

    RetErr = STTST_GetItem(pars_p, "-1", TrvStr, sizeof(TrvStr));

    RetErr = STTST_EvaluateInteger( TrvStr, &LVar, 10);
    if (RetErr)
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
    UNUSED_PARAMETER(result_sym_p);

    /* get flag (0=disable 1=enable) */
    RetErr = STTST_GetInteger(pars_p, 1, (S32*)&LVar );
    if ( RetErr )
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
    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger( pars_p, TEST_MODE_MONO_HANDLE, &SMode );
    if ( RetErr ||
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
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

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
#ifdef ST_OS20
    device_id_t devid;
#endif /* ST_OS20 */
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

#ifdef ST_OSLINUX
    sprintf(API_Msg, "Device Id not available\n\n");
#else
#ifdef ST_OS20
    devid = device_id();
    sprintf(API_Msg, "Device 0x%x (%s)\n\n", devid.id, device_name(devid));
#endif /* ST_OS20 */

#ifdef ST_OS21
    sprintf(API_Msg, "Device (%s)\n\n", kernel_chip());
#endif /* ST_OS21 */

#endif
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
static BOOL API_InitCommand (void)
{
    BOOL RetErr;
#ifdef ST_OS20
    char * Env_p;
#endif

     U32 CutId;



    RetErr = FALSE;
    if (!APIInitDone)
    {
        /* --- API functions --- */

        RetErr |= STTST_RegisterCommand( "API_INT2STR", API_ConvIntToStr,\
                                         "<num><str|symb> Concate symbol or string with number");
        RetErr |= STTST_RegisterCommand( "API_EVALSTR", API_EvaluateStr, \
                                         "<str><num|num*> Evaluate string(s) or symbol(s) to value after num(decimal) separation caracter");
        RetErr |= STTST_RegisterCommand( "API_CREATESYMB", API_CreateSymbol, \
                                         "<str><num><value> Create symbol str+num with value (num can be a symbol)");
        RetErr |= STTST_RegisterCommand( "API_CHECKERR", API_CheckErrors, \
                                         "<NbOfExpectedErrors> test & reset nb of errors");
        RetErr |= STTST_RegisterCommand( "API_ERROR", API_Error, \
                                         "<TRUE/FALSE> Enable/disable error detection");
        RetErr |= STTST_RegisterCommand( "API_GETERRORCOUNT", API_GetErrorCount, \
                                         "Returns the internal error counter value");
        RetErr |= STTST_RegisterCommand( "API_INCRERR",  API_IncrErrors, \
                                         "<NbOfErrors> Add errors to API_ErrorCount");
        RetErr |= STTST_RegisterCommand( "API_POLLKEY",  API_Pollkey, \
                                         "Return True if a key was hit, else False");
        RetErr |= STTST_RegisterCommand( "API_PROMPT", API_Prompt, \
                                         "<Prompt> Display Prompt, expect answer" \
                                         "default TRUE=Yes, 0=No");
        RetErr |= STTST_RegisterCommand( "API_QUEST", API_Question, \
                                         "<Quest string><Expect answer> Ask quest, expect answer" \
                                         "default TRUE=Yes, 0=No");
        RetErr |= STTST_RegisterCommand( "API_REPORT", API_Report, \
                                         "<String><Number Success/Failed> Report");
        RetErr |= STTST_RegisterCommand( "API_TRACE", API_Trace, "<1|0> Set or stop trace");
        RetErr |= STTST_RegisterCommand( "API_BREAK", API_Break, "Always return TRUE to stop macros");
        RetErr |= STTST_RegisterCommand( "API_SETTESTMODE", API_SetTestMode, "<0|1> Set mode to mono-handle(0=default) or multi-handles(1)");
        RetErr |= STTST_RegisterCommand( "API_DEVICEID", API_DeviceId, "Read Device Identifier");

        /* --- API variables --- */

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

        /* OS40 only:
         * OS40 does not support getenv(), that returns NULL. So DVD_ROOT and STAPIGATDATA testtool variables
         * must be set by user in testtool script */
#ifdef ST_OSLINUX
            sprintf(RootPath, ".");
            sprintf(StapigatDataPath, "%s/files/", RootPath);
#endif

#ifdef ST_OS20
        Env_p = getenv("DVD_ROOT");
        if(Env_p != NULL)
        {
            strncpy(RootPath, Env_p, sizeof(RootPath)/sizeof(char));
            RetErr |= STTST_AssignString ("DVD_ROOT", RootPath, TRUE);
            sprintf(StapigatDataPath, "%s/dvdgr-prj-stapigat/data/", RootPath);
            RetErr |= STTST_AssignString ("STAPIGATDATA", StapigatDataPath, TRUE);
        }
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
#if defined (ARCHITECTURE_ST200)
        sprintf(StapigatDataPath, "\\dvdgr-prj-stapigat\\data\\");
#else
        sprintf(StapigatDataPath, "/dvdgr-prj-stapigat/data/");
#endif
        RetErr |= STTST_AssignString ("STAPIGATDATA", StapigatDataPath, TRUE);
#endif

#if defined (ST_7020) /* must be before 5514/5517 because of mb290 and stem7020 */
        RetErr |= STTST_AssignString ("API_CHIPID", "7020", TRUE);
#elif defined(ST_5510)
        RetErr |= STTST_AssignString ("API_CHIPID", "5510", TRUE);
#elif defined(ST_5512)
        RetErr |= STTST_AssignString ("API_CHIPID", "5512", TRUE);
#elif defined(ST_5508)
        RetErr |= STTST_AssignString ("API_CHIPID", "5508", TRUE);
#elif defined(ST_5518)
        RetErr |= STTST_AssignString ("API_CHIPID", "5518", TRUE);
#elif defined(ST_5514)
        RetErr |= STTST_AssignString ("API_CHIPID", "5514", TRUE);
#elif defined(ST_5516)
        RetErr |= STTST_AssignString ("API_CHIPID", "5516", TRUE);
#elif defined(ST_5517)
        RetErr |= STTST_AssignString ("API_CHIPID", "5517", TRUE);
#elif defined(ST_5100)
        RetErr |= STTST_AssignString ("API_CHIPID", "5100", TRUE);
#elif defined(ST_5528)
        RetErr |= STTST_AssignString ("API_CHIPID", "5528", TRUE);
#elif defined(ST_7015)
        RetErr |= STTST_AssignString ("API_CHIPID", "7015", TRUE);
#elif defined(ST_7100)
        RetErr |= STTST_AssignString ("API_CHIPID", "7100", TRUE);
#elif defined(ST_7710)
        RetErr |= STTST_AssignString ("API_CHIPID", "7710", TRUE);
#elif defined(ST_GX1)
        RetErr |= STTST_AssignString ("API_CHIPID", "GX1", TRUE);
#elif defined(ST_5105)
        RetErr |= STTST_AssignString ("API_CHIPID", "5105", TRUE);
#elif defined(ST_5525)
        RetErr |= STTST_AssignString ("API_CHIPID", "5525", TRUE);
#elif defined(ST_5301)
        RetErr |= STTST_AssignString ("API_CHIPID", "5301", TRUE);
#elif defined(ST_8010)
        RetErr |= STTST_AssignString ("API_CHIPID", "8010", TRUE);
#elif defined(ST_5557)
        RetErr |= STTST_AssignString ("API_CHIPID", "5557", TRUE);
#elif defined(ST_7109)
        RetErr |= STTST_AssignString ("API_CHIPID", "7109", TRUE);
#elif defined(ST_5188)
        RetErr |= STTST_AssignString ("API_CHIPID", "5188", TRUE);
#elif defined(ST_5107)
        RetErr |= STTST_AssignString ("API_CHIPID", "5107", TRUE);
#elif defined(ST_7200)
        RetErr |= STTST_AssignString ("API_CHIPID", "7200", TRUE);
#else
#error Add defined for chip used
#endif

#if defined(mb275)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb275", TRUE);
#elif defined(mb275_64)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb275_64", TRUE);
#elif defined(mb282)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb282", TRUE);
#elif defined(mb282b)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb282b", TRUE);
#elif defined(mb290)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb290", TRUE);
#elif defined(mb295)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb295", TRUE);
#elif defined(mb314)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb314", TRUE);
#elif defined(mb317)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb317", TRUE);
#elif defined(mb361)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb361", TRUE);
#elif defined(mb376)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb376", TRUE);
#elif defined(espresso)
        RetErr |= STTST_AssignString ("API_BOARDID", "espresso", TRUE);
#elif defined(mb382)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb382", TRUE);
#elif defined(mb390)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb390", TRUE);
#elif defined(mb391)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb391", TRUE);
#elif defined(mb400)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb400", TRUE);
#elif defined(maly3s)
        RetErr |= STTST_AssignString ("API_BOARDID", "maly3s", TRUE);
#elif defined(SAT5107)
        RetErr |= STTST_AssignString ("API_BOARDID", "SAT5107", TRUE);
#elif defined(mb411)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb411", TRUE);
#elif defined(mb5518)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb5518", TRUE);
#elif defined(mb421)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb421", TRUE);
#elif defined(mb424)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb424", TRUE);
#elif defined(mb428)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb428", TRUE);
#elif defined(mb457)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb457", TRUE);
#elif defined(mb436)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb436", TRUE);
#elif defined(DTT5107)
        RetErr |= STTST_AssignString ("API_BOARDID", "DTT5107", TRUE);
#elif defined(mb519)
        RetErr |= STTST_AssignString ("API_BOARDID", "mb519", TRUE);
#else
#error Add defined for board used
#endif

#if defined(STVID_DIRECTV)
        RetErr |= STTST_AssignString ("API_SERVICE", "DIRECTV", TRUE);
#else
        RetErr |= STTST_AssignString ("API_SERVICE", "DVB", TRUE);
#endif

#if defined(ST_OS20)
        RetErr |= STTST_AssignString ("API_OS", "OS20", TRUE);
        RetErr |= STTST_AssignString ("ST_OS", "OS20", TRUE);       /* Redundant with API_OS, should be removed ... */
        RetErr |= STTST_AssignString ("OSPATH", "../../../scripts/", TRUE);
        RetErr |= STTST_AssignString ("HDMIDATA", "Z:/bistrim/mpeg2/es/", TRUE);
        RetErr |= STTST_AssignString ("HDMIAUDIODATA", "C:/", TRUE);
#elif defined(ST_OSWINCE)
        RetErr |= STTST_AssignString ("API_OS", "WINCE", TRUE);
        RetErr |= STTST_AssignString ("ST_OS", "WINCE", TRUE);       /* Redundant with API_OS, should be removed ... */
        RetErr |= STTST_AssignString ("OSPATH", "../../../scripts/", TRUE);
        RetErr |= STTST_AssignString ("HDMIDATA", "//gnx168/gnx1685/bistrim/video/es/", TRUE);
        RetErr |= STTST_AssignString ("HDMIAUDIODATA", "//gnx168/gnx1685/bistrim/audio/", TRUE);
#elif defined(ST_OS21)
        RetErr |= STTST_AssignString ("API_OS", "OS21", TRUE);
        RetErr |= STTST_AssignString ("ST_OS", "OS21", TRUE);       /* Redundant with API_OS, should be removed ... */
        RetErr |= STTST_AssignString ("OSPATH", "../../../scripts/", TRUE);
        RetErr |= STTST_AssignString ("HDMIDATA", "Z:/bistrim/mpeg2/es/", TRUE);
        RetErr |= STTST_AssignString ("HDMIAUDIODATA", "C:/", TRUE);
#elif defined(ST_OSLINUX)
        RetErr |= STTST_AssignString ("API_OS", "LINUX", TRUE);
        RetErr |= STTST_AssignString ("ST_OS", "LINUX", TRUE);       /* Redundant with API_OS, should be removed ... */
        RetErr |= STTST_AssignString ("OSPATH", "./scripts/", TRUE);
        RetErr |= STTST_AssignString ("STAPIGATDATA", "./files/", TRUE);
        RetErr |= STTST_AssignString ("HDMIDATA", "./bistrim/video/es/", TRUE);
        RetErr |= STTST_AssignString ("HDMIAUDIODATA", "./audio/", TRUE);
#else
        RetErr |= STTST_AssignString ("API_OS", "unknown", TRUE);
#endif


#ifdef ARCHITECTURE_ST40
        RetErr |= STTST_AssignString ("API_PROCESSORID", "ST40", TRUE);
#elif defined (ARCHITECTURE_ST20)
        RetErr |= STTST_AssignString ("API_PROCESSORID", "ST20", TRUE);
#elif defined (ARCHITECTURE_ST200)
        RetErr |= STTST_AssignString ("API_PROCESSORID", "ST200", TRUE);
#elif defined (ARCHITECTURE_SPARC)
        RetErr |= STTST_AssignString ("API_PROCESSORID", "SPARC", TRUE);
#elif defined (ARCHITECTURE_LINUX)
        RetErr |= STTST_AssignString ("API_PROCESSORID", "ST40", TRUE);
#else
#error No Architecture defined
#endif

#if defined(DVD_SECURED_CHIP) && defined(ST_7109)
        RetErr |= STTST_AssignString ("API_SECURITY", "MES", TRUE);
#else
        RetErr |= STTST_AssignString ("API_SECURITY", "NONE", TRUE);
#endif /* Secure 7109 */

        CutId = ST_GetCutRevision();

        RetErr |= STTST_AssignInteger ("API_CUTID", CutId, TRUE);


    }
    return(RetErr ? FALSE : TRUE);
} /* end API_InitCommand() */

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
#endif /* USE_API_TOOL */

/* End of api_cmd.c */
