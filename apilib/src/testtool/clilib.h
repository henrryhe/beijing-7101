/*****************************************************************************

File name   : clilib.h

Description : command line interpreter

COPYRIGHT (C) ST-Microelectronics 1998.

Date               Modification                                     Name
----               ------------                                     ----
05 oct 1998        Ported to 5510                                    FR
09 sep_1999        Update include files list                         FQ
04 oct 1999        Change os20.h to stlite.h                         FQ
23 feb 2000        Change to STAPI style                             FQ
02 Oct 2001        Decrease stack use.                               HSdLM
18 Oct 2001        Remove CLI_BAD_INTEGER_VALUE                      HSdLM
*****************************************************************************/

#ifndef __CLILIB_H
#define __CLILIB_H

#include "testtool.h"

#ifdef __cplusplus
extern "C" {
#endif

/* constants -------------------------------------------------------------- */

#define CLI_SPACE_CHAR  0x20
#define CLI_TAB_CHAR    0x09
#define CLI_ESCAPE_CHAR '\\'
#define CLI_NL_CHAR     '\n'
#define CLI_CR_CHAR     '\r'

#define CLI_COMMENT_CHAR ';'  /* semi-colon */

enum
{
    CLI_NO_CONST,
    CLI_DEFINE_CONST,
    CLI_IF_CONST,
    CLI_ELIF_CONST,
    CLI_ELSE_CONST,
    CLI_WHILE_CONST,
    CLI_FOR_CONST
};

#define CLI_MAX_LINE_LENGTH 255

/* macro store structure -------------------------------------------------- */

struct macro_s
{
    struct macro_s *line_p;
    char           line[CLI_MAX_LINE_LENGTH];
};
typedef struct macro_s macro_t;

/* symbol table data structure and types ---------------------------------- */

#define CLI_NO_SYMBOL  0
#define CLI_INT_SYMBOL 1             /* integer symbol                          */
#define CLI_FLT_SYMBOL 2             /* floating point symbol                   */
#define CLI_STR_SYMBOL 4             /* string symbol                           */
#define CLI_COM_SYMBOL 8             /* command symbol                          */
#define CLI_MAC_SYMBOL 16            /* macro symbol                            */
#define CLI_ANY_SYMBOL 0xFF          /* matches all symbol types                */

struct symtab_s
{
    char *name_p;                    /* symbol id                               */
    S16  type;                       /* type of symbol                          */
    S16  name_len;                   /* length of symbol name                   */
    union
    {
        S32     int_val;
        double  flt_val;
        char    *str_val;
        BOOL    (*com_val)(STTST_Parse_t*, char*);
        macro_t *mac_val;
    } value;                         /* value of symbol                         */
    BOOL fixed;                      /* flag for symbol                         */
    S16  depth;                      /* nesting depth at which declaration made */
    const char *info_p;              /* informational string                    */
};
typedef struct symtab_s symtab_t;

/* prototypes ------------------------------------------------------------- */

void sttst_CliInit(BOOL (*setup_r)(void), S16 max_symbols, S16 default_base );
void sttst_CliRun (char *const ip_prompt_p, char *file_p );
void sttst_CliTerm(void);

#ifdef __cplusplus
}
#endif

#endif
