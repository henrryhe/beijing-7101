/*****************************************************************************

File name   : testtool.h

Description : main development API

COPYRIGHT (C) ST-Microelectronics 1998.

Date               Modification                                     Name
----               ------------                                     ----
05 oct 1998        ported to 5510                                    FR
06 sep 1999        Add os20.h + init. struct.                        FQ
04 oct 1999        Change os20.h to stlite.h                         FQ
11 oct 1999        Change partition type                             FQ
24 jan 2000        Change to STAPI style                             FQ
31 aou 2000        Create STTST_SetMode();                           FQ
02 oct 2001        Decrease stack use.                               HSdLM
28 nov 2001        Add option to keep control variables.             HSdLM
 *                 Fix DDTS GNBvd10194 for wrapper around STAPI
*****************************************************************************/

#ifndef __TESTTOOL_H
#define __TESTTOOL_H

#define TESTTOOL_PRESENT

/* Linux porting */

#include "stos.h"

#include "stddefs.h"   /* needed for partition and types */

#ifdef __cplusplus
extern "C" {
#endif

/* Constants -------------------------------------------------------------- */

#define STTST_MAX_TOK_LEN          80 /* nb max of bytes for one token      */

#define STTST_INTERACTIVE_MODE              0x00  /* testtool prompt available (default mode) */
#define STTST_BATCH_MODE                    0x01  /* batch mode : no testtool prompt available, auto leave */
#define STTST_NO_ABBREVIATION_MODE          0x02  /* no symbol name abbreviation allowed */
#define STTST_HIT_KEY_TO_ENTER_MODE         0x04  /* if STTST_INTERACTIVE_MODE, hit key to enter testtool
                                                     Was behaviour of Testtool <= 3.0.7 */
#define STTST_KEEP_CONTROL_VARIABLE_MODE    0x08  /* if/then/else, for, while controls: do not delete variables */
#define STTST_SUPPORTED_MODES_MASK          0x0F  /* OR of supported modes */

/* Initialization parameters ---------------------------------------------- */

typedef struct STTST_InitParams_s
{
    ST_Partition_t *CPUPartition_p;  /* partition for internal memory allocation */
    S16  NbMaxOfSymbols;             /* size of the internal symbols table  */
    char InputFileName[STTST_MAX_TOK_LEN]; /* filename for input commands (not mandatory) */
} STTST_InitParams_t;

/* Tokeniser data structure ----------------------------------------------- */

struct parse
{
  char    *line_p;   /* string under examination                            */
  S16     par_pos;   /* index of current position, at delimiter or EOL      */
  S16     par_sta;   /* index of start position for last operation          */
  S16     tok_len;   /* length of identified token                          */
  char    tok_del;   /* delimiter of current token (not part of token)      */
  char    token[STTST_MAX_TOK_LEN]; /* token found. Space allocated on need, constant kept for backward compatibility */
};
typedef struct parse STTST_Parse_t;
typedef struct parse parse_t; /* old prototype (before year 2000) */

typedef S16 STTST_RunningMode_t; /* bit 1=abbreviation; bit 0=interactive   */

/* Functions and variables NOT TO BE USED by users of the API --------------- */
void sttst_Print(const char *, ...); /* allow output on stdout+log file in applications */

/* Function prototypes for user (since February 2000) -----------------------*/

#define STTST_Print(msg) sttst_Print msg

BOOL STTST_Init(const STTST_InitParams_t *);
BOOL STTST_Start(void);
BOOL STTST_Term(void);
BOOL STTST_SetMode(STTST_RunningMode_t);

ST_Revision_t STTST_GetRevision(void);

BOOL STTST_GetString (STTST_Parse_t *pars_p, const char *const default_p, char *const result_p, S16 max_len);
BOOL STTST_GetItem   (STTST_Parse_t *pars_p, const char *const default_p, char *const result_p, S16 max_len);
BOOL STTST_GetInteger(STTST_Parse_t *pars_p, S32    def_int, S32    *result_p);
BOOL STTST_GetFloat  (STTST_Parse_t *pars_p, double def_flt, double *result_p);
BOOL STTST_GetTokenCount(STTST_Parse_t *pars_p, S16 *result_p);

void STTST_TagCurrentLine (STTST_Parse_t *pars_p, const char *const message_p);

BOOL STTST_AssignInteger(const char *const token_p, S32        value         , BOOL constant);
BOOL STTST_AssignFloat  (const char *const token_p, double     value         , BOOL constant);
BOOL STTST_AssignString (const char *const token_p, const char *const value_p, BOOL constant);

BOOL STTST_DeleteSymbol (const char *const token_p);

BOOL STTST_EvaluateInteger(   const char *const token_p, S32    *value_p,  S16 default_base);
BOOL STTST_EvaluateFloat  (   const char *const token_p, double *value_p);
BOOL STTST_EvaluateString (   const char *const token_p, char *  string_p, S16 max_len);
BOOL STTST_EvaluateComparison(const char *const token_p, BOOL *  result_p, S16 default_base);

BOOL STTST_RegisterCommand(   const char *const token_p, BOOL (*action)(STTST_Parse_t*, char*),
                              const char *const help_p);

/* Old function prototypes for user ----------------------------------------*/
                   /* used to keep compatibility with old test applications */

#define max_tok_len STTST_MAX_TOK_LEN

/*boolean STTST_RegisterCommand(char *token_p, boolean (*action)(parse_t*, char*),
                         char *help_p);*/

#define testtool_init(i)        STTST_Init(i)
#define testtool_run()          STTST_Start()
#define cget_string(p,d,r,m)    STTST_GetString(p,d,r,m)
#define cget_integer(p,d,r)     STTST_GetInteger((parse_t*)p,(S32)d,(S32*)r)
#define cget_float(p,d,r)       STTST_GetFloat(p,d,r)
#define cget_item(p,d,r,m)      STTST_GetItem(p,d,r,(S16)m)
#define cget_token_count(p,r)   STTST_GetTokenCount(p,(S16 *)r)
#define assign_integer(t,v,c)   STTST_AssignInteger(t,(S32)v,c)
#define assign_float(t,v,c)     STTST_AssignFloat(t,v,c)
#define assign_string(t,v,c)    STTST_AssignString(t,v,(S16)c)
#define evaluate_integer(t,v,d) STTST_EvaluateInteger(t,(S32*)v,(S16)d)
#define evaluate_float(t,v)     STTST_EvaluateFloat(t,v)
#define evaluate_comparison(t,r,d) STTST_EvaluateComparison(t,(BOOL *)r,(S16)d)
#define evaluate_string(t,s,m)  STTST_EvaluateString(t,s,(S16)m)
#define register_command(t,a,h) STTST_RegisterCommand(t,(BOOL (*)(STTST_Parse_t*, char*)) a,h)
#define tag_current_line(p,m)   STTST_TagCurrentLine(p,m)
#define print(m)                sttst_Print m

#define TESTTOOL_InitParams_t   STTST_InitParams_t

#ifdef __cplusplus
}
#endif

#endif /* ifndef __TESTTOOL_H */
