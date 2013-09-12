/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : tst_test.c

Date          Modification                                    Initials
----          ------------                                    --------
01 Mar 2000   Creation                                        FQ
12 Apr 2002   Big update                                      HSdLM
************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stddefs.h"

#include "startup.h"

#include "sttbx.h"

#include "testtool.h"
#include "cltst.h"

#ifdef ST_OS20
#include "metrics.h"
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

static BOOL TST_InitDone = FALSE;     /* flag for macro initialization */
static char TST_Msg[200];             /* text for trace */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static BOOL TST_CreSymbol(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL TST_DelSymbol(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL TST_GetRevision(STTST_Parse_t *pars_p, char *result_sym_p);
static BOOL TST_SetMode(STTST_Parse_t *pars_p, char *result_sym_p);
static void TST_CompileTest(void);

/* Functions ---------------------------------------------------------------- */


/*-------------------------------------------------------------------------
 * Function : TST_CreSymbol
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TST_CreSymbol(STTST_Parse_t *pars_p, char *result_sym_p)
{

    BOOL RetErr;
    char SymbolName[STTST_MAX_TOK_LEN];
    char SymbolType[STTST_MAX_TOK_LEN];
    S32  SymbolCste;
    BOOL IsConstant;
    UNUSED_PARAMETER(result_sym_p);

    IsConstant = FALSE;
    RetErr = STTST_GetString(pars_p, "", SymbolName, STTST_MAX_TOK_LEN);
    if ( RetErr )
    {
        STTST_TagCurrentLine(pars_p,"invalid symbol name");
    }
    else
    {
        RetErr = STTST_GetString(pars_p, "I", SymbolType, STTST_MAX_TOK_LEN);
        if ( RetErr || ( SymbolType[0]!='I' && SymbolType[0]!='F' && SymbolType[0]!='S' ) )
        {
            STTST_TagCurrentLine(pars_p,"invalid symbol type (I=integer F=float S=string)");
        }
        else
        {
            RetErr = STTST_GetInteger(pars_p, FALSE, &SymbolCste);
            if ( RetErr )
            {
                STTST_TagCurrentLine(pars_p,"invalid type (TRUE=constant FALSE=variable)");
            }
            else
            {
                if ( SymbolCste!=0 )
                {
                    IsConstant = TRUE;
                }
            }
        }
    }
    if ( !RetErr )
    {
        switch(SymbolType[0])
        {
            case 'I' :
                RetErr = STTST_AssignInteger(SymbolName, 0, SymbolCste);
                strcpy(TST_Msg, "STTST_AssignInteger");
                break;
            case 'F' :
                RetErr = STTST_AssignFloat(SymbolName, 0, SymbolCste);
                strcpy(TST_Msg, "STTST_AssignFloat");
                break;
            case 'S' :
                RetErr = STTST_AssignString(SymbolName, "", SymbolCste);
                strcpy(TST_Msg, "STTST_AssignString");
                break;
            default:
                RetErr = TRUE;
                break;
        }
        if ( !RetErr )
        {
            STTST_Print(("%s(%s): ok \n", TST_Msg, SymbolName));
        }
        else
        {
            STTST_Print(("%s(%s): error \n", TST_Msg, SymbolName));
        }
    }
    return(RetErr);
} /* end of TST_CreSymbol() */

/*-------------------------------------------------------------------------
 * Function : TST_DelSymbol
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TST_DelSymbol(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    char SymbolName[STTST_MAX_TOK_LEN];
    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetString(pars_p, "", SymbolName, STTST_MAX_TOK_LEN);
    if ( RetErr || strlen(SymbolName)==0 )
    {
        STTST_TagCurrentLine(pars_p,"invalid symbol name");
    }
    else
    {
        RetErr = STTST_DeleteSymbol(SymbolName);
        if ( !RetErr )
        {
            STTST_Print(("STTST_DeleteSymbol(%s): ok \n", SymbolName));
        }
        else
        {
            STTST_Print(("STTST_DeleteSymbol(%s): error \n", SymbolName));
        }
    }
    return(RetErr);
} /* end of TST_DelSymbol() */

/*-------------------------------------------------------------------------
 * Function : TST_GetRevision
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TST_GetRevision(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_Revision_t CurrentRevision;
    UNUSED_PARAMETER(result_sym_p);
    UNUSED_PARAMETER(pars_p);

    CurrentRevision = STTST_GetRevision();
    STTST_Print(("TST_GetRevision(): %s \n", CurrentRevision));

    return(false);
} /* end of TST_GetRevision() */

/*-------------------------------------------------------------------------
 * Function : TST_SetMode
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL TST_SetMode(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    S32 NewMode;
    char StrMode[150];
    UNUSED_PARAMETER(result_sym_p);

    RetErr = STTST_GetInteger(pars_p, STTST_INTERACTIVE_MODE, &NewMode);
    if ( RetErr )
    {
        sprintf( TST_Msg, "invalid mode (%d=interactive %d=batch %d=no abbreviation %d=hit key to enter %d=keep control variables)",
                    STTST_INTERACTIVE_MODE, STTST_BATCH_MODE, STTST_NO_ABBREVIATION_MODE, STTST_HIT_KEY_TO_ENTER_MODE, STTST_KEEP_CONTROL_VARIABLE_MODE );
        STTST_TagCurrentLine(pars_p, TST_Msg);
    }
    else
    {
        RetErr = STTST_SetMode((STTST_RunningMode_t)NewMode);

        if ((NewMode & STTST_BATCH_MODE) != 0)
        {
            sprintf( StrMode, "STTST_BATCH_MODE");
        }
        else
        {
            sprintf( StrMode, "STTST_INTERACTIVE_MODE");
        }
        if ((NewMode & STTST_NO_ABBREVIATION_MODE) != 0)
        {
            strcat( StrMode, " | STTST_NO_ABBREVIATION_MODE");
        }
        if ((NewMode & STTST_HIT_KEY_TO_ENTER_MODE) != 0)
        {
            strcat( StrMode, " | STTST_HIT_KEY_TO_ENTER_MODE");
        }
        if ((NewMode & STTST_KEEP_CONTROL_VARIABLE_MODE) != 0)
        {
            strcat( StrMode, " | STTST_KEEP_CONTROL_VARIABLE_MODE");
        }

        sprintf( TST_Msg, "STTST_SetMode(%s):", StrMode);
        if ( !RetErr )
        {
            STTST_Print(("%s ok \n", TST_Msg));
        }
        else
        {
            STTST_Print(("%s error \n", TST_Msg));
        }
    }
    return(RetErr);
} /* end of TST_SetMode() */

/*#######################################################################*/
/*########################### MISCELLANEOUS #############################*/
/*#######################################################################*/

static void TST_CompileTest(void)
{
    STTST_InitParams_t InitPar;
    STTST_Parse_t Pars_p;
    char *Result_sym_p, *Token, *Text;
    double DValue;
    /* old types : */
    long LValue;
    short SValue;
    BOOL Flag;

    /* do not launch this functions; it is a compilation test  */
    /* in order to verify compatibility with previous releases */
    /* (check of the #define Xxx() y_yy() in the include file) */

    testtool_init(&InitPar);
    testtool_run();
    cget_string(&Pars_p,Text,Text,SValue);
    cget_integer(&Pars_p,LValue,&LValue);
    cget_float(&Pars_p,DValue,&DValue);
    cget_item(&Pars_p,Text,Result_sym_p,SValue);
    cget_token_count(&Pars_p,&SValue);
    assign_integer(Token,LValue,Flag);
    assign_float(Token,DValue,Flag);
    assign_string(Token,Text,Flag);
    evaluate_integer(Token,&SValue,SValue);
    evaluate_float(Token,&DValue);
    evaluate_comparison(Token,&Flag,SValue);
    evaluate_string(Token,Text,SValue);
    register_command(Token,TST_CreSymbol,Text);
    tag_current_line(&Pars_p,Text);
} /* end of TST_Compile_Test() */

/*-------------------------------------------------------------------------
 * Function : Test_CmdStart
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL Test_CmdStart(void)
{
  BOOL RetErr;

    RetErr = FALSE;
    if ( RetErr ) /* keep this 'false' statement */
    {
        TST_CompileTest(); /* to avoid compilation message */
    }
    if (!TST_InitDone)
    {
        RetErr = STTST_RegisterCommand(  "TST_CRESYM",   TST_CreSymbol,   "<SymbolName><I|F|S><0|1> Creates a symbol\n\t     I=integer F=float S=string 0=variable 1=constant\n\t     default=<symbolname> \"I\" 0 ");
        RetErr |= STTST_RegisterCommand( "TST_DELSYM",   TST_DelSymbol,   "<SymbolName> Deletes a symbol");
        RetErr |= STTST_RegisterCommand( "TST_REVISION", TST_GetRevision, "Get revision");
        RetErr |= STTST_RegisterCommand( "TST_SETMODE",  TST_SetMode,     "Set mode. OR of: 0=interactive 1=batch 2=no abbrev. 4=hit key to enter 8=keep control variables");
        if (RetErr)
        {
            STTST_Print(("TST_TestCommand() : macros registrations failure !\n"));
        }
        else
        {
            STTST_Print(("TST_TestCommand() : Ok\n"));
            TST_InitDone = TRUE;
        }

        STTST_AssignInteger("STTST_INTERACTIVE_MODE", STTST_INTERACTIVE_MODE, TRUE);
        STTST_AssignInteger("STTST_BATCH_MODE", STTST_BATCH_MODE, TRUE);
        STTST_AssignInteger("STTST_NO_ABBREVIATION_MODE", STTST_NO_ABBREVIATION_MODE, TRUE);
        STTST_AssignInteger("STTST_HIT_KEY_TO_ENTER_MODE", STTST_HIT_KEY_TO_ENTER_MODE, TRUE);
        STTST_AssignInteger("STTST_KEEP_CONTROL_VARIABLE_MODE", STTST_KEEP_CONTROL_VARIABLE_MODE, TRUE);
    }
    return( !RetErr );
} /* end of Test_CmdStart() */

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
    BOOL RetOk, RetErr;
    char InputFileAnswer;
    char BatchModeAnswer;
    STTST_InitParams_t InitParams;

    TST_InitDone = TRUE; /* we don't want Test_CmdStart() to be executed right now */
    STAPIGAT_Init();
    TST_InitDone = FALSE;
    RetOk = TRUE;
    UNUSED_PARAMETER(ptr);

#ifdef ST_OS20
    MetricsStackTest();
#endif

    /* --- Test menu --- */
    STTBX_Print(("\nDo you want to launch the tests stored in default.mac    (Y/N) ? " ));
    STTBX_InputChar(&InputFileAnswer);
#ifndef CONFIG_POSIX
	STTBX_Print(("%c",InputFileAnswer));

#endif
    STTBX_Print(("\nDo you want to start in batch mode (default=interactive) (Y/N) ? " ));
    STTBX_InputChar(&BatchModeAnswer);
#ifndef CONFIG_POSIX
    STTBX_Print(("%c\n\n",BatchModeAnswer));
#else
    STTBX_Print(("\n\n",BatchModeAnswer));
#endif

    /* --- Initialise Testtool --- */

    InitParams.CPUPartition_p = DriverPartition_p;
    InitParams.NbMaxOfSymbols = 1000;
    if ( InputFileAnswer=='y' || InputFileAnswer=='Y' )
    {
#if !defined(ST_OSLINUX)
       strcpy( InitParams.InputFileName, "../../../scripts/default.mac" );
#else
       strcpy( InitParams.InputFileName, "./scripts/testtool/default.mac" );
#endif
    }
    else
    {
        strcpy( InitParams.InputFileName, "" );
    }
    RetErr = STTST_Init(&InitParams);
    if (!RetErr)
    {
        printf("\nSTTST initialized, \trevision=%s\n",STTST_GetRevision());
    }
    else
    {
        printf("\nSTTST_Init() failed !\n");
        RetOk = FALSE;
    }
    if (RetOk)
    {
        RetOk = Test_CmdStart();
        STTST_AssignInteger("STTST_MODE", STTST_INTERACTIVE_MODE, FALSE);
        if ( BatchModeAnswer=='y' || BatchModeAnswer=='Y' )
        {
            RetErr = STTST_SetMode(STTST_BATCH_MODE);
            if ( RetErr )
            {
                STTBX_Print(("STTST_SetMode(STTST_BATCH_MODE) : failure !\n"));
            }
            else
            {
                STTBX_Print(("STTST_SetMode(STTST_BATCH_MODE) : ok\n"));
                STTST_AssignInteger("STTST_MODE", STTST_BATCH_MODE, FALSE);
            }
        }
        /* --- Run Testtool --- */

        STTST_Start();
        TST_Term();
    }
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
#if defined(ST_OS21) && !defined(ST_OSWINCE)
    printf ("\nBOOT ...\n");
    setbuf(stdout, NULL);
    #if defined(ST_OS21)
        UNUSED_PARAMETER(argc);
        UNUSED_PARAMETER(argv);
        os20_main(NULL);
    #else
        OS20_main(argc, argv, os20_main);
    #endif  /*End of ST_OS21*/
#else
    #ifdef ST_OSWINCE
        SetFopenBasePath("/dvdgr-prj-testtool/tests/src/objs/ST40");
    #endif

    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);
    os20_main(NULL);
#endif

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}

/* End of avm_test.c */


