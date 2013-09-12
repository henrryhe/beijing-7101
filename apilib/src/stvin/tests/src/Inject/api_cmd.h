/*******************************************************************************

File name   : api_cmd.h

Description : Constants and exported functions

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                                 Name
----               ------------                                 ----
01 Jan 99          Created                                      FQ
*******************************************************************************/


/* Define to prevent recursive inclusion */
#ifndef __API_CMD_H
#define __API_CMD_H

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Macros and Defines -------------------------------------------- */
#define TEST_MODE_MONO_HANDLE      0
#define TEST_MODE_MULTI_HANDLES    1

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */
extern U32   API_ErrorCount ;   /* number of drivers errors detected */
extern BOOL  API_EnableError ;  /* if false, commands always return false */
								 /* to avoid macro interruption */
extern S16  API_TestMode;         /* mono/multi handle    */

/* Exported Functions ----------------------------------------------------- */
BOOL API_MacroInit (void);

BOOL API_ConvIntToStr(  STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_EvaluateStr(   STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_CreateSymbol(  STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_CheckErrors(   STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_GetErrorCount( STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_Error(         STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_Question(      STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_Report(        STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_Trace(         STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_Break(         STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_SetTestMode(   STTST_Parse_t *pars_p, char *result_sym_p );
BOOL API_TextError(     ST_ErrorCode_t Error, char *Text);    

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __API_CMD_H */
