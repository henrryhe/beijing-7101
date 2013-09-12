/*****************************************************************************

File name   : Clilib.c

Description : Command line interpreter

COPYRIGHT (C) ST-Microelectronics 1998.

This module contains the core support routines of the c language cli

Date               Modification                                     Name
----               ------------                                     ----
05 oct 1998        Ported to 5510                                    FR
25 jan 1999        Improvment : trace, stat, help x*, del x*         FQ
29 mar 1998        Report.c replaced by ToolBox; print() removed     FQ
06 sep 1999        Remove os20.h, add partitio.h                     FQ
06 sep 1999        change malloc() to memory_allocate()              FQ
04 oct 1999        Change partitio.h to stlite.h                     FQ
05 oct 1999        Change 2 buffers sizes (120 to max_line_len)      FQ
06 oct 1999        Pb: sttbx_print() is limited to 255 charac.       FQ
24 dec 1999        do_log: new modes; print: filter; add do_close    FQ
24 jan 2000        Add read_stdin() for cmd recall with ctrl keys    FQ
06 mar 2000        New banner according to revision                  FQ
02 Oct 2001        Decrease stack use.                               HSdLM
08 Oct 2001        Fix DDTS GNBvd09368, 5960, 6055, allow            HSdLM
 *                 string = string + integer
28 Nov 2001        Remove 'hit a key to enter' as default            HSdLM
 *                 Add option to keep control variables.
 *                 Fix DDTS GNBvd10196 evaluate comparison
15 May 2002        Fix DDTS GNBvd10434 tabulations in scripts        HSdLM
23 Jul 2002        Fix DDTS GNBvd17434 memory leak                   CL
*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */
#include <stdio.h>                /* standard inputs                        */
#include <stdlib.h>               /*                                        */
#include <string.h>               /* for string manipulations               */
#include <stdarg.h>               /* for argv argc va_list                  */
#include <ctype.h>                /*                                        */

#ifndef STTBX_PRINT
    #define STTBX_PRINT
#endif

#include "stddefs.h"
#include "clilib.h"    /* local API                              */
#include "testtool.h"  /* for testtool commands                  */
#ifndef STTBX_INPUT
#define STTBX_INPUT
#endif

#include "sttbx.h"

#if defined ST_OS21 && !defined ST_OSWINCE
/* Include for OS21 Profiler */
#include <os21/profile.h>
#endif /* ST_OS21 */

/* Private Types ------------------------------------------------------------ */

typedef struct
{
    S16 old_cmd;   /* first command to be entered */
    S16 last_cmd;  /* last command to be entered */
    char cmd_name[STTST_MAX_TOK_LEN][CLI_MAX_LINE_LENGTH];
} cmd_history_t; /* circular stack ; old==last==0 if empty ; fisrt item always empty */

typedef struct
{
    char Slot[1]; /* just to say it's a table, memory allocation is done with greater size! */
} SlotTable_t;

/* Private Constants -------------------------------------------------------- */

/* Set following define to enable setting default I/O radix for integer input and output */
/* #define IO_RADIX_CONF */

#define KEY_BACKSP       0x8   /* backspace */

#define KEY_UP           0x9   /* ctrl-i */
#ifndef CONFIG_POSIX
#define KEY_DOWN         0xF   /* ctrl-o */
#define KEY_RIGHT        0x10  /* ctrl-p */
#define KEY_LEFT         0x15  /* ctrl-u */
#else
#define KEY_DOWN         0x17  /* ctrl-o */
#define KEY_RIGHT        0x10 /*0x20*/  /* ctrl-p */
#define KEY_LEFT         0x25  /* ctrl-u */
#endif
#define UNIX_KEY_DOWN    0xA   /* ctrl-o on Unix */
#define UNIX_KEY_BACKSP  0x7F  /* backspace on Unix */

/* OS21 Profiler Default settings */
#define INSTRUCTIONS_PER_BUCKET         (1)
#define SAMPLING_FREQ_IN_HERTZ          (1000)

/* for gcc only ----------------------------------------------------------- */
#ifndef FILENAME_MAX
#define FILENAME_MAX 64
#endif

#define MAX_STTBX_LEN          512
#define DISPLAY_BUFFER_SIZE    (CLI_MAX_LINE_LENGTH*5+1) /* five lines */
#define STRING_TABLE_INIT_SIZE 3  /* if not enough, table is extended with memory allocations  */
#define TOKEN_TABLE_INIT_SIZE  10 /* if not enough, table is extended with memory allocations  */
#define PARSE_TABLE_INIT_SIZE  10 /* if not enough, table is extended with memory allocations  */
#define MAX_NUMBER_LENGTH      34 /* 32 bit word in binary base */
#define CONTROL_NAME_LENGTH    10 /* e.g : WHILE23 */

/* Private Variables (static)------------------------------------------------ */

static symtab_t    **Symtab_p;            /* symbol table pointer                               */
static S32         MaxSymbolCount;        /* maximum number of symbols                          */
static S32         SymbolCount;           /* number of symbols declared                         */
static S16         MacroDepth;            /* macro invocation depth                             */
static U8          IfNameCount;           /* used to define 'If' macro with unique name         */
static U8          ElifNameCount;         /* used to define 'Elif' macro with unique name       */
static U8          ElseNameCount;         /* used to define 'Else' macro with unique name       */
static U8          ForNameCount;          /* used to define 'For' macro with unique name        */
static U8          WhileNameCount;        /* used to define 'While' macro with unique name      */
static S16         NumberBase;            /* default inp/output base for ints                   */
static FILE        *CurrentStream_p;      /* current file ip stream                             */
static macro_t     *CurrentMacro_p;       /* current macro position                             */
static char        *Prompt_p;             /* input prompt                                       */
static BOOL        CurrentEcho;           /* status of input echo                               */
static size_t      TotalFreeSize;
static FILE        *LogFile;
static BOOL        LogOutput;
static BOOL        LogResults;
static char        CommandLineDelimiter[] = "= \\\t";
static char        DelimiterSet[]         = " ,\\\t";
static char        LogicalOps[]           = "><=!";
static char        ComparisonOps[]       = "|&";
static char        ArithmeticOpsInt[]     = "*/+-|&^%";
static char        ArithmeticOpsFloat[]   = "*/+-";
static char        HexaChars[] = "0123456789ABCDEF";
static char        *DisplayBuffer_p;
static SlotTable_t **StringTable_p;
static S16         StringTableIndex;
static U16         MaxSimultaneousStringReached;
static SlotTable_t **TokenTable_p;
static S16         TokenTableIndex;
static U16         MaxSimultaneousTokenReached;
static STTST_Parse_t **ParseTable_p;
static S16         ParseTableIndex;
static U16         MaxSimultaneousParseReached;

static cmd_history_t CommandHistory;

#if defined ST_OS21 && !defined ST_OSWINCE
static BOOL       is_profiler_initialized = FALSE;
static BOOL       is_profiler_started = FALSE;
#endif /* ST_OS21 */

/* Global Variables --------------------------------------------------------- */
extern STTST_InitParams_t sttst_InitParams;
extern S16 STTST_RunMode;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static BOOL do_define(STTST_Parse_t *Parse_p);
static BOOL define_macro(const char *const defline_p, char *name_p);
static BOOL evaluate_integer_expr(STTST_Parse_t*, S32*, S16);
static BOOL evaluate_float_expr(STTST_Parse_t *Parse_p, double *value_p);
static BOOL read_input(char *line_p, const char *const ip_prompt_p);
static BOOL command_loop(macro_t *macro_p, FILE *file_p, char *rtn_exp_p, BOOL echo);
static BOOL uncommand_loop(macro_t *macro_p, FILE *file_p, char *rtn_exp_p, BOOL echo);
static void InitDisplayBuffer(void);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : NewStringTableSlot
Description : give new memory slot in line table
Parameters  : size to allocate
Assumptions :
Limitations :
Returns     : pointer on allocated memory for Line.
*******************************************************************************/
static char * NewStringTableSlot(U16 LineSize)
{
    SlotTable_t **Tmp_p = StringTable_p; /* make it not NULL */

    StringTableIndex++;
    if (StringTableIndex < STRING_TABLE_INIT_SIZE )
    {
        /* Index = 0 => 1 slot took, index=1 => 2 slots ... */
        if (StringTableIndex+1 > MaxSimultaneousStringReached)
        {
            MaxSimultaneousStringReached = StringTableIndex+1;
        }
    }
    else
    {
        if (StringTableIndex+1 > MaxSimultaneousStringReached)
        {
            MaxSimultaneousStringReached = StringTableIndex+1;
            /* re-allocate with greater size print table and copy content to new @ */
            Tmp_p = (SlotTable_t **)memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(SlotTable_t*)*(MaxSimultaneousStringReached));
            if (Tmp_p != NULL)
            {
                /* copy content from StringTable_p to Tmp_p :
                * keep (StringTableIndex+1)-1 old values, -1 because as been incremented yet */
                memcpy(Tmp_p, StringTable_p, sizeof(SlotTable_t*)*(StringTableIndex));
                memory_deallocate(sttst_InitParams.CPUPartition_p, StringTable_p);
                StringTable_p = Tmp_p;
            }
            else
            {
                /* allocation failed : use last slot, crunch it ...*/
                STTBX_Print(("\n ******* NewStringTableSlot : not enough memory ! Last slot crunched ****** \n\n" ));
                StringTableIndex--;
            }
        }
        if (Tmp_p != NULL)
        {
            StringTable_p[StringTableIndex] = (SlotTable_t *)memory_allocate(sttst_InitParams.CPUPartition_p, LineSize);
        }
    }
    return(StringTable_p[StringTableIndex]->Slot);
}

/*******************************************************************************
Name        : FreeStringTableSlot
Description : free memory slot for print
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void FreeStringTableSlot(void)
{
    if (StringTableIndex >= STRING_TABLE_INIT_SIZE )
    {
        memory_deallocate(sttst_InitParams.CPUPartition_p, StringTable_p[StringTableIndex]);
    }
    StringTableIndex--;
}


/*******************************************************************************
Name        : NewTokenTableSlot
Description : give new memory slot in token table
Parameters  : size to allocate
Assumptions :
Limitations :
Returns     : pointer on allocated memory for Token.
*******************************************************************************/
static char * NewTokenTableSlot(U16 TokenSize)
{
    SlotTable_t **Tmp_p = TokenTable_p; /* make it not NULL */

    TokenTableIndex++;
    if (TokenTableIndex < TOKEN_TABLE_INIT_SIZE )
    {
        /* Index = 0 => 1 slot took, index=1 => 2 slots ... */
        if (TokenTableIndex+1 > MaxSimultaneousTokenReached)
        {
            MaxSimultaneousTokenReached = TokenTableIndex+1;
        }
    }
    else
    {
        if (TokenTableIndex+1 > MaxSimultaneousTokenReached)
        {
            MaxSimultaneousTokenReached = TokenTableIndex+1;
            /* re-allocate with greater size print table and copy content to new @ */
            Tmp_p = (SlotTable_t **)memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(SlotTable_t*)*(MaxSimultaneousTokenReached));
            if (Tmp_p != NULL)
            {
                /* copy content from TokenTable_p to Tmp_p :
                * keep (TokenTableIndex+1)-1 old values, -1 because as been incremented yet */
                memcpy(Tmp_p, TokenTable_p, sizeof(SlotTable_t*)*(TokenTableIndex));
                memory_deallocate(sttst_InitParams.CPUPartition_p, TokenTable_p);
                TokenTable_p = Tmp_p;
            }
            else
            {
                /* allocation failed : use last slot, crunch it ...*/
                STTBX_Print(("\n ******* NewTokenTableSlot : not enough memory ! Last slot crunched ****** \n\n" ));
                TokenTableIndex--;
            }
        }
        if (Tmp_p != NULL)
        {
            TokenTable_p[TokenTableIndex] = (SlotTable_t *)memory_allocate(sttst_InitParams.CPUPartition_p, TokenSize);
        }
    }
    return(TokenTable_p[TokenTableIndex]->Slot);
}

/*******************************************************************************
Name        : FreeTokenTableSlot
Description : free memory slot for print
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void FreeTokenTableSlot(void)
{
    if (TokenTableIndex >= TOKEN_TABLE_INIT_SIZE )
    {
        memory_deallocate(sttst_InitParams.CPUPartition_p, TokenTable_p[TokenTableIndex]);
    }
    TokenTableIndex--;
}

/*******************************************************************************
Name        : NewParseTableSlot
Description : give new memory slot in token table
Parameters  :
Assumptions :
Limitations :
Returns     : pointer on allocated memory for Parse
*******************************************************************************/
static STTST_Parse_t * NewParseTableSlot(U16 TokenSize)
{
    STTST_Parse_t **Tmp_p = ParseTable_p; /* make it not NULL */
    U16 AllocSize;

    ParseTableIndex++;
    if (ParseTableIndex < PARSE_TABLE_INIT_SIZE )
    {
        /* Index = 0 => 1 slot took, index=1 => 2 slots ... */
        if (ParseTableIndex+1 > MaxSimultaneousParseReached)
        {
            MaxSimultaneousParseReached = ParseTableIndex+1;
        }
    }
    else
    {
        if (ParseTableIndex+1 > MaxSimultaneousParseReached)
        {
            MaxSimultaneousParseReached = ParseTableIndex+1;
            /* re-allocate with greater size print table and copy content to new @ */
            Tmp_p = (STTST_Parse_t **)memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(STTST_Parse_t*)*(MaxSimultaneousParseReached));
            if (Tmp_p != NULL)
            {
                /* copy content from ParseTable_p to Tmp_p :
                * keep (ParseTableIndex+1)-1 old values, -1 because as been incremented yet */
                memcpy(Tmp_p, ParseTable_p, sizeof(STTST_Parse_t*)*(ParseTableIndex));
                memory_deallocate(sttst_InitParams.CPUPartition_p, ParseTable_p);
                ParseTable_p = Tmp_p;
            }
            else
            {
                /* allocation failed : use last slot, crunch it ...*/
                STTBX_Print(("\n ******* NewParseTableSlot : not enough memory ! Last slot crunched ****** \n\n" ));
                ParseTableIndex--;
            }
        }
        if (Tmp_p != NULL)
        {
            AllocSize = sizeof(STTST_Parse_t) + TokenSize;
            ParseTable_p[ParseTableIndex] = (STTST_Parse_t *)memory_allocate(sttst_InitParams.CPUPartition_p, AllocSize);
        }
    }
    return(ParseTable_p[ParseTableIndex]);
}

/*******************************************************************************
Name        : FreeParseTableSlot
Description : free memory slot for print
Parameters  :
Assumptions :
Limitations :
Returns     : nothing
*******************************************************************************/
static void FreeParseTableSlot(void)
{
    if (ParseTableIndex >= PARSE_TABLE_INIT_SIZE )
    {
        memory_deallocate(sttst_InitParams.CPUPartition_p, ParseTable_p[ParseTableIndex]);
    }
    ParseTableIndex--;
}

/* ========================================================================
   indirect print method using variable arg format to invoke formatting
=========================================================================== */
void sttst_Print(const char* const format, ...)
{
    va_list list;
    char *pt, *StringBuffer;

    StringBuffer = NewStringTableSlot(MAX_STTBX_LEN);

    va_start(list, format);
    vsprintf(StringBuffer, format, list);
    va_end(list);
    /*io_write(StringBuffer); */
    STTBX_Print((StringBuffer));
#ifndef UNHOSTED
    if ((LogFile != NULL) && LogOutput)
    {
        if ( LogResults )
        {
            pt = strstr( StringBuffer, "result");
            if ( pt==NULL )
            {
                pt = strstr( StringBuffer, "RESULT");
            }
            if ( pt==NULL )
            {
                pt = strstr( StringBuffer, "Result");
            }
            if ( pt!=NULL )
            {
                fprintf(LogFile, "%s", StringBuffer);
            }
        }
        else
        {
            fprintf(LogFile, "%s", StringBuffer);
        }
    }
#endif
    FreeStringTableSlot();
}

/* ========================================================================
   display memory free blocks
=========================================================================== */
#ifdef ST_OS20
static void partition_heap_infos(partition_t *l, size_t *MaxBlockSize, BOOL DisplayEnabled)
{

#define partition_free_head   u.partition_heap.partition_heap_free_head
#define __next partition_heap_block_next
#define __blocksize partition_heap_block_blocksize

    partition_heap_block_t *i;
    partition_heap_block_t *m;
    size_t size;

    i =  (partition_heap_block_t *) ((partition_heap_block_t *) l->partition_free_head->__next);
    m =  (partition_heap_block_t *) l->partition_free_head;

    *MaxBlockSize = 0;
    TotalFreeSize = 0;

    while(i != m)
    {
        if (DisplayEnabled)
        {
            sttst_Print("Block %x: %d \tNext block : %x %x\n",(int)i,(int)i->__blocksize,(int)i->__next,(int)m);
        }
        size = (size_t)i->__blocksize;
        TotalFreeSize += size;
        if (size > *MaxBlockSize)
        {
            *MaxBlockSize = size;
        }
        i = i->__next;
    }
    sttst_Print("Free blocks size sum %d\n",(int)TotalFreeSize);

}
#endif /* ST40_OS21 */


/* ========================================================================
   indirect statistics about memory usage (FQ Jan 99)
=========================================================================== */
static void cli_statistics(BOOL DisplayEnabled)
{
/*    void *mem_pt; previous code*/
#ifndef ST_OSLINUX
#ifdef ST_OS21
    partition_status_t status;
#endif
    size_t mem_size=0;
#endif
    symtab_t* sym_pt;
    int  loop_cpt=0;
    int  mac_cpt=0;
    int  com_cpt=0;
    int  str_cpt=0;
    int  nb_cpt=0;
    char *StringBuffer;

    /* Get free memory */
/* previous code */
/*    mem_size=800500;*/
/*    do*/
/*    {*/
/*        mem_size -= 500;*/
/*        mem_pt = memory_allocate((partition_t *)sttst_InitParams.CPUPartition_p, (size_t)mem_size);*/
/*    }*/
/*    while ( mem_pt == NULL );*/
/*    memory_deallocate( sttst_InitParams.CPUPartition_p, mem_pt);*/

#ifndef ST_OSLINUX
    /* new code for free memory calculation CL Aug 2001 */

#ifdef ST_OS21
    partition_status(sttst_InitParams.CPUPartition_p, &status, 0);
    mem_size = status.partition_status_free_largest;
    UNUSED_PARAMETER(DisplayEnabled);
#else
    partition_heap_infos(sttst_InitParams.CPUPartition_p, &mem_size, DisplayEnabled);
#endif
    /* end of new code for free memory calculation CL Aug 2001 */
#endif

    /* Symbols counting */
    while ( loop_cpt < SymbolCount )
    {
        sym_pt = Symtab_p[loop_cpt];
        if ( sym_pt->type == CLI_MAC_SYMBOL ) mac_cpt++;
        if ( sym_pt->type == CLI_COM_SYMBOL ) com_cpt++;
        if ( sym_pt->type == CLI_STR_SYMBOL ) str_cpt++;
        if ( sym_pt->type == CLI_INT_SYMBOL ) nb_cpt++;
        if ( sym_pt->type == CLI_FLT_SYMBOL ) nb_cpt++;
        loop_cpt++;
    }

    StringBuffer = NewStringTableSlot(CLI_MAX_LINE_LENGTH);

    /* Memory informations */
#ifdef ST_OSLINUX
    UNUSED_PARAMETER(DisplayEnabled);
    sprintf(StringBuffer, "max free memory block size  = N/A on Linux\n");
#else
#ifdef ST_OS21
    sprintf(StringBuffer, "partition total memory size = %d bytes\n", status.partition_status_size);
    STTBX_Print((StringBuffer));
    sprintf(StringBuffer, "free memory size            = %d bytes\n", status.partition_status_free);
    STTBX_Print((StringBuffer));
    sprintf(StringBuffer, "max free memory block size  = %d bytes\n", status.partition_status_free_largest);
#else
    sprintf(StringBuffer, "max free memory block size  = %d bytes\n",mem_size);
#endif /* ST_OS21 */
#endif /* ST_OSLINUX */
    STTBX_Print((StringBuffer));
    sprintf( StringBuffer,
        "declared symbols  = %d (%d commands + %d macros + %d strings + %d numbers)\n",
        (com_cpt+mac_cpt+str_cpt+nb_cpt), com_cpt, mac_cpt, str_cpt, nb_cpt);
    STTBX_Print((StringBuffer));
    sprintf(StringBuffer, "allocated symbols = %d / %d \n",SymbolCount, MaxSymbolCount);
    STTBX_Print((StringBuffer));
    sprintf(StringBuffer,
        "Buffer management : max simultaneous reached...\n\t...String = %d\n\t...Token  = %d\n\t...Parse  = %d\n",
        MaxSimultaneousStringReached, MaxSimultaneousTokenReached, MaxSimultaneousParseReached);
    STTBX_Print((StringBuffer));
    FreeStringTableSlot();
}

/* ========================================================================
   statistics about memory usage
=========================================================================== */
static BOOL cli_stat(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL IsBad;
    BOOL DisplayEnabled;
    S32 Enable;

    TotalFreeSize =0;
    IsBad = STTST_GetInteger(Parse_p, FALSE, &Enable);
    if (IsBad)
    {
        STTST_TagCurrentLine(Parse_p, "expected base address");
    }
    else
    {
        DisplayEnabled = Enable;
        cli_statistics(DisplayEnabled);
        IsBad = STTST_AssignInteger(result_sym_p, TotalFreeSize, FALSE);
    }
    return(IsBad);
}

/* ========================================================================
   indirect 'scanf' function from uart or console
=========================================================================== */
/***int scan (const char * format, void* Variable)
{
    int i;
    char ip_char[CLI_MAX_LINE_LENGTH];

    for ( i=0; i<CLI_MAX_LINE_LENGTH; i++)
    {
        ip_char[i] = 0;
    }
    read_input(ip_char, "?> ");
    sscanf(ip_char, format, Variable);
    return (1);
}***/
/* ========================================================================
 tests the character for equality with a set of delimiter characters
=========================================================================== */
static BOOL is_delim(char character, const char *const delim_p)
{
    S16 delim_count = 0;

    while ((delim_p[delim_count] != '\0') &&
          (character != delim_p[delim_count]))
    {
        delim_count++;
    }
    return ((character == delim_p[delim_count]) ||
           (character == CLI_NL_CHAR) ||
           (character == CLI_CR_CHAR) ||
           (character == '\0'));
}
/* ========================================================================
 tests strings for equality, but not in an exact way. Trailing space is OK
 Comparison will succeed if the tested string matches the definition string
 but is shorter (i.e is an abbreviation). It will not match if the tested
 string is longer or less than minlen chars are present in the tested
 string. Comparison is also case insensitive.
=========================================================================== */
static BOOL is_matched(const char *const tested_p, const char *const definition_p, S16 minlen)
{
  BOOL match = TRUE;
  S16 cnt = 0;

    while (((tested_p[cnt] == definition_p[cnt]) ||
        ((tested_p[cnt] & 0xdf) == (definition_p[cnt] & 0xdf)) ) &&
        (tested_p[cnt] != '\0')                                   &&
        (definition_p[cnt] != '\0'))
    {
        cnt++;
    }

    if ((STTST_RunMode & STTST_NO_ABBREVIATION_MODE) != 0)
    {
        if ( tested_p[cnt] != '\0' || definition_p[cnt] != '\0' )
        {
            /* string lengths are different : it does not match */
            match = FALSE;
        }
    }
    else
    {
    /* if we found the end of the tested string before we found a mis-match
     then strings are matched. If the definition string is shorter than
     minumum length requirements, then match can succeed. */
        if ((tested_p[cnt] != '\0') ||
            ((cnt < minlen) && (definition_p[cnt] != '\0')))
        {
            match = FALSE;
        }
    }
    return(match);
}

/* ========================================================================
   tests a token against a set of control redirection primitives
   and returns an identifier for the construct found
=========================================================================== */
static BOOL is_control(const char * const token_p, S16 *construct_p)
{
    BOOL found = TRUE;

    if (is_matched(token_p, "DEFINE", 2))
    {
        *construct_p = CLI_DEFINE_CONST;
    }
    else if (is_matched(token_p, "IF", 2))
    {
        *construct_p = CLI_IF_CONST;
    }
    else if (is_matched(token_p, "ELIF", 2))
    {
        *construct_p = CLI_ELIF_CONST;
    }
    else if (is_matched(token_p, "ELSE", 2))
    {
        *construct_p = CLI_ELSE_CONST;
    }
    else if (is_matched(token_p, "WHILE", 2))
    {
        *construct_p = CLI_WHILE_CONST;
    }
    else if (is_matched(token_p, "FOR", 2))
    {
        *construct_p = CLI_FOR_CONST;
    }
    else
    {
        found = FALSE;
    }
    return(found);
}
/* ========================================================================
   returns, if possible, an integer value. The default base
   is used for conversion in the absence of other information.
   If a '#' character preceeds the number hex base is assumed
   If a '$' is used as a prefix, then a binary representation is assumed
   If an 'o' or 'O' character is used then octal is assumed.
   Any sign makes the number a decimal representation
=========================================================================== */
static S32 conv_int(const char * token_p, S16 default_base, BOOL *IsValid_p)
{
    S32  value;
    S16  base, cnt, i;
    U8   NbDigit = 0;
    BOOL negative;

    negative = FALSE;
    *IsValid_p = TRUE;
    switch (token_p[0])
    {
        case '#' : /* keep old syntax for backward compatibility */
        case 'h' : /* new syntax (Feb. 2000) */
        case 'H' : /* new syntax (Feb. 2000) */
            base = 16;
            token_p++;
            break;
        case '$' : /* keep old syntax for backward compatibility */
        case 'b' : /* new syntax (Feb. 2000) */
        case 'B' : /* new syntax (Feb. 2000) */
            base = 2;
            token_p++;
            break;
        case 'o' :
        case 'O' :
            base = 8;
            token_p++;
            break;
        case '-' :
            negative = TRUE;
            /* no break */
        case '+' :
            base = 10;
            token_p++;
            break;
        default :
            base = default_base;
            break;
    }
    /* convert by comparison to ordered string array of numeric characters */
    value = 0;
    cnt = 0;
    while((token_p[cnt] != '\0') && (*IsValid_p))
    {
        i = 0;
        while((HexaChars[i] != (char)toupper(token_p[cnt])) && (i < base))
        {
            i++;
        }
        if (token_p[cnt]!=' ') /* skip space (test added for DDTS 6642 - 23 March 01) */
        {
            NbDigit++;
            switch (base)
            {
                case 2 :
                    if ( (i >= 2) || ( NbDigit > 32))
                    {
                        *IsValid_p = FALSE;
                    }
                    break;
                case 8 :
                    if ( (i >= 8) || ( NbDigit > 11))
                    {
                        *IsValid_p = FALSE;
                    }
                    break;
                case 10 :
                    if ( (i >= 10) || (((U32)value >= 429496729) && (i > 5)))
                    {
                        *IsValid_p = FALSE;
                    }
                    break;
                case 16 :
                    if ( (i >= 16) || ( NbDigit > 8))
                    {
                        *IsValid_p = FALSE;
                    }
                    break;
                default :
                    *IsValid_p = FALSE;
                    break;
            }
            if (*IsValid_p)
            {
                value = (value * base) + i;
            }
            else
            {
                value = 0;
            }
        }
        cnt++;
    }
    if (negative && (*IsValid_p))
    {
        value = -value;
    }
    return(value);
}
/* ========================================================================
   returns, if possible, an floating point value. Scanf seems not to
   detect any errors in its input format, and so we have to do a check
   on number validity prior to calling it. This is rather complicated
   to try and reject float expressions, but validate pure float numbers
=========================================================================== */
static BOOL conv_flt(const char * const token_p, double *value_p)
{
    S16  cnt = 0;
    BOOL IsBad = FALSE;
    BOOL seen_dp = FALSE;

    while((token_p[cnt] != '\0') && !IsBad)
    {
        /* check for silly characters */
        if (    (token_p[cnt] != '-') && (token_p[cnt] != '+')
             && (token_p[cnt] != '.')
             && (token_p[cnt] != 'E') && (token_p[cnt] != 'e')
             && ((token_p[cnt] > '9') || (token_p[cnt] < '0')))
        {
            IsBad = TRUE;
        }
        else
        {
            /* check for more than one decimal point */
            if (token_p[cnt] == '.')
            {
                if (seen_dp)
                {
                    IsBad = TRUE;
                }
                else
                {
                    seen_dp = TRUE;
                }
            }
        }
        /* check for sign after decimal point not associated with exponent */
        if (    (!IsBad)
             && (((token_p[cnt] == '+') || (token_p[cnt] == '-')) && seen_dp)
             && ((token_p[cnt-1] != 'E') && (token_p[cnt-1] != 'e')))
        {
            IsBad = TRUE;
        }
        /* check for sign before a decimal point but not at start of token */
        if (    (!IsBad)
             && (((token_p[cnt] == '+') || (token_p[cnt] == '-')) && !seen_dp)
             && (cnt > 0))
        {
            IsBad = TRUE;
        }
        cnt++;
    }
    if (!IsBad)
    {
        if (sscanf(token_p, "%lg", value_p) == 0)
        {
            IsBad = TRUE;
            *value_p = 0;
        }
    }
    return(IsBad);
}
/* ========================================================================
   looks for a symbol type/name within the table. We look from
   most recent to oldest symbol to accomodate scope.
   We also scan for the shortest defined symbol that matches
   the input token - this resolves order dependent declaration problems
=========================================================================== */
static symtab_t *look_for(const char * const token_p, S16 type)
{
    S16     cnt;
    BOOL   found = FALSE;
    symtab_t  *symbol_p;
    symtab_t  *shortest_p;
    S16     short_len;

    short_len = STTST_MAX_TOK_LEN;
    shortest_p = (symtab_t*)NULL;

    /* point to last symbol in the table */
    if (SymbolCount != 0)
    {
        cnt = 1;
        while(cnt <= SymbolCount)
        {
            symbol_p = Symtab_p[SymbolCount - cnt];
            /* protect against deleted symbols */
            if (symbol_p->name_p != NULL)
            {
                /* look for a name match of at least two characters and
                shortest matching definition of search type */
                found = ((is_matched(token_p, symbol_p->name_p, 2) &&
                         ((symbol_p->type & type) > 0)));
                if (found && (symbol_p->name_len < short_len))
                {
                    shortest_p = symbol_p;
                    short_len = symbol_p->name_len;
                }
            }
            cnt++;
        }
    }
    return(shortest_p);
}
/* ========================================================================
   displays current tokeniser string and tags the position of last token.
   An optional message is displayed
=========================================================================== */
/*void tag_current_line(STTST_Parse_t *Parse_p, char *message_p)*/
void STTST_TagCurrentLine(STTST_Parse_t *Parse_p, const char *const message_p)
{
    S16 i;
    char *StringBuffer;

    StringBuffer = NewStringTableSlot(CLI_MAX_LINE_LENGTH);

    /* current line */
    sttst_Print("%s\n", Parse_p->line_p);
    /* tag the position (FQ - Oct 99) */
    for (i=0; i<Parse_p->par_pos && i<CLI_MAX_LINE_LENGTH; i++)
    {
        StringBuffer[i] = CLI_SPACE_CHAR;
    }
    if (Parse_p->par_pos < CLI_MAX_LINE_LENGTH)
    {
        StringBuffer[Parse_p->par_pos] = '^';
    }
    if (Parse_p->par_sta < CLI_MAX_LINE_LENGTH)
    {
        StringBuffer[Parse_p->par_sta] = '^';
    }
    if (Parse_p->par_sta+1 < CLI_MAX_LINE_LENGTH)
    {
        StringBuffer[(Parse_p->par_pos) + 1] = '\0';
    }
    else
    {
        StringBuffer[CLI_MAX_LINE_LENGTH-1] = '\0';
    }
    sttst_Print("%s", StringBuffer);
    /* add message */
    sttst_Print("\n%s\n", message_p);
    FreeStringTableSlot();
}
/* ========================================================================
   included for tokeniser testing
=========================================================================== */
/***void pars_debug(STTST_Parse_t *Parse_p)
{
    STTST_TagCurrentLine(Parse_p, "debug");
    sttst_Print("Tok = \"%s\", delim = %x, toklen = %d \n",
        Parse_p->token,
        Parse_p->tok_del,
        Parse_p->tok_len);
}***/
/* ========================================================================
   start up a new environment
=========================================================================== */

static void init_pars(STTST_Parse_t *Parse_p, const char *const new_line_p)
{
    /* This is a temporary solution to remove warnings, it will be reviewed */

    const char*  line_p = (const char*) new_line_p;
    void*        var_p  = (void*) &line_p;

    Parse_p->line_p = (char*) (*(char**)var_p);
    Parse_p->par_pos = 0;
    Parse_p->par_sta = 0;
    Parse_p->tok_del = '\0';
    Parse_p->token[0] = '\0';
    Parse_p->tok_len = 0;
}
/* ========================================================================
   implements a tokeniser with a 'soft' approach to end of line conditions
   repeated calls will eventually leave parse position at the null
   terminating the input string.
=========================================================================== */
static S16 get_tok(STTST_Parse_t *Parse_p, const char *const delim_p)
{
    S16 par_sta = Parse_p->par_pos;
    S16 par_pos = par_sta;
    S16 tok_len = 0;
    S16 quotes = 0;

    /* check that we are not already at the end of a line due to
    a previous call (or a null input) - if so return a null token
    End of line now includes finding comment character! */

    if ((Parse_p->line_p[par_pos] == '\0') ||
    (Parse_p->line_p[par_pos] == CLI_COMMENT_CHAR) )
    {
        Parse_p->token[0] = '\0';
        Parse_p->tok_del = '\0';
    }
    else
    {
        /* attempt to find start of a token, nothing special case of first call,
        incrementing past last delimiter and checking for end of line
        on the way */
        if (par_pos != 0)
        {
            par_pos++;
        }
        while (    (    (Parse_p->line_p[par_pos] == CLI_SPACE_CHAR)
                     || (Parse_p->line_p[par_pos] == CLI_TAB_CHAR))
                && (Parse_p->line_p[par_pos] != '\0')
                && (Parse_p->line_p[par_pos] != CLI_COMMENT_CHAR))
        {
            par_pos++;
        }

        /* if we find a delimiter before anything else, return a null token
        also deal with special case of a comment character ending a line */
        if (is_delim(Parse_p->line_p[par_pos], delim_p) ||
            (Parse_p->line_p[par_pos] == CLI_COMMENT_CHAR))
        {
            Parse_p->token[0] = '\0';
            if (Parse_p->line_p[par_pos] != CLI_COMMENT_CHAR)
            {
                Parse_p->tok_del = Parse_p->line_p[par_pos];
            }
            else
            {
                Parse_p->tok_del = '\0';
            }
        }
        else
        {
          /* copy token from line into token string
             until next delimiter found. Note that delimiters found within
             pairs of double quotes will not be considered significant. Quotes
             can be embedded within strings using '\' however. Note also that
             we have to copy the '\' escape char where it is used, it can
             be taken out when the string is evaluated */
            while((!is_delim(Parse_p->line_p[par_pos], delim_p) || (quotes > 0)) &&
                (tok_len < (S16)strlen(Parse_p->line_p)) && (Parse_p->line_p[par_pos] != '\0'))
            {
                Parse_p->token[tok_len] = Parse_p->line_p[par_pos++];
                if ((Parse_p->token[tok_len] =='"') && (tok_len == 0))
                {
                    quotes++;
                }
                if ((Parse_p->token[tok_len] =='"') && (tok_len > 0))
                {
                    if (Parse_p->token[tok_len-1] != CLI_ESCAPE_CHAR)
                    {
                        if (quotes > 0)
                        {
                            quotes--;
                        }
                        else
                        {
                            quotes++;
                        }
                    }
                }
                tok_len++;
            }
            /* if we ran out of token space before copy ended, move up to delimiter */
            while (!is_delim(Parse_p->line_p[par_pos], delim_p))
            {
                par_pos++;
            }
            /* tidy up the rest of the data */
            Parse_p->tok_del = Parse_p->line_p[par_pos];
            Parse_p->token[tok_len] = '\0';
        }
    }
    Parse_p->par_pos = par_pos;
    Parse_p->par_sta = par_sta;
    Parse_p->tok_len = tok_len;
    return(tok_len);
}
/* ========================================================================
   sets up a simple symbol table as an array of elements, ordered by
   declaration, of different types. SymbolCount will index the next free slot
=========================================================================== */
static void init_sym_table(S16 elements)
{
    partition_t *mem_pt;

    SymbolCount = 0;
    MaxSymbolCount = elements; /* based on mimimum size required */
    mem_pt = memory_allocate( sttst_InitParams.CPUPartition_p,
                (size_t)(sizeof(symtab_t*) * MaxSymbolCount) );
    Symtab_p = (symtab_t**) mem_pt;

    if (Symtab_p == NULL)
    {
        STTBX_Print(("symbol table initialization : not enough memory !\n" ));
        MaxSymbolCount = 0;
    }
}

/* ========================================================================
   allocate memory for a buffer where lines to be displayed can be prepared
=========================================================================== */
static void InitDisplayBuffer(void)
{
    DisplayBuffer_p = memory_allocate( sttst_InitParams.CPUPartition_p, DISPLAY_BUFFER_SIZE);
    if (DisplayBuffer_p == NULL)
    {
        STTBX_Print(("Display buffer initialization : not enough memory !\n" ));
    }
}


/* ========================================================================
   allocate memory for line table
=========================================================================== */
static void InitStringTable(void)
{
    U8 i;

    StringTableIndex = -1;
    MaxSimultaneousStringReached = 0;

    StringTable_p = (SlotTable_t **)memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(SlotTable_t*)*STRING_TABLE_INIT_SIZE);
    if (StringTable_p == NULL)
    {
        STTBX_Print(("Line table initialization : not enough memory !\n" ));
    }
    else
    {
        for (i=0; i<STRING_TABLE_INIT_SIZE; i++)
        {
            StringTable_p[i] = (SlotTable_t *)memory_allocate(sttst_InitParams.CPUPartition_p, MAX_STTBX_LEN);
            if (StringTable_p[i] == NULL)
            {
                STTBX_Print(("Line table initialization : not enough memory !\n" ));
                break;
            }
        }
    }
}


/* ========================================================================
   allocate memory for token table
=========================================================================== */
static void InitTokenTable(void)
{
    U8 i;

    TokenTableIndex = -1;
    MaxSimultaneousTokenReached = 0;

    TokenTable_p = (SlotTable_t **)memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(SlotTable_t*)*TOKEN_TABLE_INIT_SIZE);
    if (TokenTable_p == NULL)
    {
        STTBX_Print(("Token table initialization : not enough memory !\n" ));
    }
    else
    {
        for (i=0; i< TOKEN_TABLE_INIT_SIZE; i++)
        {
            TokenTable_p[i] = (SlotTable_t *)memory_allocate(sttst_InitParams.CPUPartition_p, CLI_MAX_LINE_LENGTH);
            if (TokenTable_p[i] == NULL)
            {
                STTBX_Print(("Token table initialization : not enough memory !\n" ));
                break;
            }
        }
    }
}

/* ========================================================================
   allocate memory for parse table
=========================================================================== */
static void InitParseTable(void)
{
    U8 i;
    U16 AllocSize;

    ParseTableIndex = -1;
    MaxSimultaneousParseReached = 0;


    ParseTable_p = (STTST_Parse_t **)memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(STTST_Parse_t*)*TOKEN_TABLE_INIT_SIZE);
    if (ParseTable_p == NULL)
    {
        STTBX_Print(("Parse table initialization : not enough memory !\n" ));
    }
    else
    {
        AllocSize = sizeof(STTST_Parse_t) + CLI_MAX_LINE_LENGTH;
        for (i=0; i< PARSE_TABLE_INIT_SIZE; i++)
        {
            ParseTable_p[i] = (STTST_Parse_t *)memory_allocate(sttst_InitParams.CPUPartition_p, AllocSize);
            if (ParseTable_p[i] == NULL)
            {
                STTBX_Print(("Parse table initialization : not enough memory !\n" ));
                break;
            }
        }
    }
}

/* ========================================================================
   deletes all symbol entries in table created above a given nest level.
   if the last entry is deleted, the table store is deallocated.
=========================================================================== */
static void purge_symbols(S16 level)
{
    S32      cnt;
    symtab_t  *symbol_p;
    BOOL   ExitLoop = FALSE;

    cnt = 1;
    while(!ExitLoop && (cnt <= SymbolCount))
    {
        symbol_p = Symtab_p[SymbolCount - cnt];
        ExitLoop = (symbol_p->depth <= level);
        if (!(ExitLoop))
        {
            if (symbol_p->name_p != NULL)
            {
                memory_deallocate( sttst_InitParams.CPUPartition_p, symbol_p->name_p );
            }
            if (symbol_p->type == CLI_STR_SYMBOL)
            {
                memory_deallocate( sttst_InitParams.CPUPartition_p, symbol_p->value.str_val);
            }
            if (symbol_p->type == CLI_MAC_SYMBOL)
            {
                macro_t  *l_p,*nl_p;

                l_p = symbol_p->value.mac_val;
                while (l_p != (macro_t*)NULL)
                {
                    nl_p = l_p->line_p;
                    memory_deallocate( sttst_InitParams.CPUPartition_p, (char*)l_p);
                    l_p = nl_p;
                }
            }
            memory_deallocate( sttst_InitParams.CPUPartition_p, (char*)symbol_p);
            cnt++;
        }
    }
    SymbolCount = SymbolCount - (cnt-1);
    if (SymbolCount == 0)
    {
        memory_deallocate( sttst_InitParams.CPUPartition_p, (char*) Symtab_p);
    }
}
/* ========================================================================
   adds symbol structure to table and checks for name clashes, scope
   clashes, invalid identifiers and lack of table space. A valid name
   must contain alphanumerics only, plus '_', with
   the first character alphabetic.
=========================================================================== */
static symtab_t *insert_symbol(const char * const token_p, S16 type)
{
    symtab_t   *symbol_p;
    symtab_t   *oldsym_p;
    S16      cnt;
    BOOL    valid;
    partition_t *mem_pt;

    /* check that symbol table has a spare slot and issue a warning if close */
    symbol_p = (symtab_t*) NULL;
    if (SymbolCount >= (MaxSymbolCount - 1))
    {
        sttst_Print("Cannot insert \"%s\" in symbol table - table is full!\n",token_p);
    }
    else
    {
        valid = TRUE;
        cnt = 0;
        while (token_p[cnt] != '\0')
        {
            if (((token_p[cnt]  < 'A') || (token_p[cnt] > 'Z')) &&
                ((token_p[cnt]  < 'a') || (token_p[cnt] > 'z')) &&
                ((token_p[cnt]  < '0') || (token_p[cnt] > '9') || (cnt == 0)) &&
                ((token_p[cnt] != '_') || (cnt == 0)))
            {
                valid = FALSE;
            }
            cnt++;
        }
        if (!valid)
        {
            sttst_Print("Cannot insert \"%s\" in symbol table - invalid symbol name\n",
                token_p);
        }
        else
        {
            /* carry on with insertion process, checking for scoped name clashes */
            /* look for a symbol of the same type and matching name. This can
               be ok if it was declared in a macro level less than the current one*/
            oldsym_p = look_for(token_p, CLI_ANY_SYMBOL);
            if ((oldsym_p != (symtab_t*) NULL) && (oldsym_p->depth >= MacroDepth))
            {
                sttst_Print("Cannot insert \"%s\" in symbol table - name clash within current scope\n",
                    token_p);
            }
            else
            {
                mem_pt = memory_allocate( sttst_InitParams.CPUPartition_p, sizeof(symtab_t));
                symbol_p = (symtab_t*) mem_pt;
                mem_pt = memory_allocate(sttst_InitParams.CPUPartition_p, strlen(token_p)+1);
                symbol_p->name_p = (char *)mem_pt;
                if ((symbol_p == NULL) || (symbol_p->name_p == NULL))
                {
                    sttst_Print("Cannot insert \"%s\" in symbol table - no memory available\n",
                        token_p);
                    symbol_p = NULL;
                }
                else
                {
                    /* sttst_Print("Inserted symbol \"%s\" using space at %x\n", token_p, symbol_p); */
                    strcpy(symbol_p->name_p, token_p);
                    symbol_p->name_len = strlen(token_p);
                    symbol_p->type = type;
                    symbol_p->depth = MacroDepth;

                    /* insert new structure in table and warn if near full */
                    Symtab_p[SymbolCount] = symbol_p;
                    /* sttst_Print("Inserted symbol \"%s\" at slot %d\n", token_p, SymbolCount); */
                    SymbolCount++;
                    if (SymbolCount >= (MaxSymbolCount - 10))
                    {
                        sttst_Print("Warning: Symbol table nearly full - (%ld of %ld entries)\n",
                            SymbolCount, MaxSymbolCount);
                    }
                }
            }
        }
    }
    return(symbol_p);
}
/* ========================================================================
   allows deletion of a symbol from table, providing its been declared
   at the current MacroDepth
=========================================================================== */
static BOOL delete_symbol(const char * const token_p)
{
    symtab_t *symbol_p;
    BOOL     IsBad = FALSE;
    U16      Index;

    symbol_p = look_for(token_p, CLI_ANY_SYMBOL); /* look for any type */
    if (symbol_p == (symtab_t*)NULL)
    {
        IsBad = TRUE;
    }
    else
    {
        if ((symbol_p->fixed) || (symbol_p->depth != MacroDepth))
        {
            IsBad = TRUE;
        }
        else
        {
            /* free symbol name storage */
            memory_deallocate(sttst_InitParams.CPUPartition_p, symbol_p->name_p);
            symbol_p->name_p = NULL;

            /* delete string storage */
            if (symbol_p->type == CLI_STR_SYMBOL)
            {
                memory_deallocate(sttst_InitParams.CPUPartition_p, symbol_p->value.str_val);
            }

            /* delete any macro line buffers */
            if (symbol_p->type == CLI_MAC_SYMBOL)
            {
                macro_t  *l_p,*nl_p;

                l_p = symbol_p->value.mac_val;
                while (l_p != (macro_t*)NULL)
                {
                    nl_p = l_p->line_p;
                    memory_deallocate(sttst_InitParams.CPUPartition_p, (char*)l_p);
                    l_p = nl_p;
                }
            }
            /* mark symbol as unused, ready for purge when nest level is stripped */
            symbol_p->type = CLI_NO_SYMBOL;

            /* defragment symbol table */
            /* find deleted symbol index in table */
            Index = 0;
            while (Symtab_p[Index]->type != CLI_NO_SYMBOL)
            {
                Index++;
            }
            /* move next symbols backward from 1 slot in table  */
            while(Index < (SymbolCount-1))
            {
                Symtab_p[Index] = Symtab_p[Index+1] ;
                Index++;
            }
            Symtab_p[SymbolCount-1] = NULL;
            SymbolCount--;

            /* Actually de allocate the deleted symbol */
            memory_deallocate(sttst_InitParams.CPUPartition_p, (char*)symbol_p);
       }
    }
    return(IsBad);
}
/* ========================================================================
   creates or updates an integer symbol table entry
=========================================================================== */
/*BOOL assign_integer(char * token_p, long value, BOOL constant)*/
BOOL STTST_AssignInteger(const char *const token_p, S32 value, BOOL constant)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;
    if (strlen(token_p) != 0)
    {
        symbol_p = look_for(token_p, CLI_INT_SYMBOL);
        if ((symbol_p == (symtab_t*) NULL) && (token_p[0] != '\0'))
        {
            symbol_p = insert_symbol(token_p, CLI_INT_SYMBOL);
            if (symbol_p == (symtab_t*) NULL)
            {
                IsBad = TRUE;
            }
            else
            {
                symbol_p->fixed = constant;
                if (symbol_p->fixed)
                {
                    symbol_p->info_p = "integer constant";
                }
                else
                {
                    symbol_p->info_p = "integer variable";
                }
                symbol_p->value.int_val = value;
            }
        }
        else
        {
            if (symbol_p->fixed)
            {
                IsBad = TRUE;
            }
            else
            {
                symbol_p->value.int_val = value;
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   creates an integer symbol table entry without looking for existing symbol
   with a name match
=========================================================================== */
static BOOL create_integer(const char *const token_p, S32 value, BOOL constant)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;

    if (strlen(token_p) != 0)
    {
        symbol_p = insert_symbol(token_p, CLI_INT_SYMBOL);
        if (symbol_p == (symtab_t*) NULL)
        {
            IsBad = TRUE;
        }
        else
        {
            symbol_p->fixed = constant;
            if (symbol_p->fixed)
            {
                symbol_p->info_p = "integer constant";
            }
            else
            {
                symbol_p->info_p = "integer variable";
            }
            symbol_p->value.int_val = value;
        }
    }
    return(IsBad);
}
/* ========================================================================
   creates or updates a floating point symbol table entry
=========================================================================== */
/*BOOL assign_float(char *token_p, double value, BOOL constant)*/
BOOL STTST_AssignFloat(const char *const token_p, double value, BOOL constant)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;
    if (strlen(token_p) != 0)
    {
        symbol_p = look_for(token_p, CLI_FLT_SYMBOL);
        if ((symbol_p == (symtab_t*) NULL) && (token_p[0] != '\0'))
        {
            symbol_p = insert_symbol(token_p, CLI_FLT_SYMBOL);
            if (symbol_p == (symtab_t*) NULL)
            {
                IsBad = TRUE;
            }
            else
            {
                symbol_p->fixed = constant;
                if (symbol_p->fixed)
                {
                    symbol_p->info_p = "floating point constant";
                }
                else
                {
                    symbol_p->info_p = "floating point variable";
                }
            symbol_p->value.flt_val = value;
            }
        }
        else
        {
            if (symbol_p->fixed)
            {
                IsBad = TRUE;
            }
            else
            {
                symbol_p->value.flt_val = value;
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   creates a floating point symbol table entry
=========================================================================== */
static BOOL create_float(const char * const token_p, double value, BOOL constant)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;

    if (strlen(token_p) != 0)
    {
        symbol_p = insert_symbol(token_p, CLI_FLT_SYMBOL);
        if (symbol_p == (symtab_t*) NULL)
        {
            IsBad = TRUE;
        }
        else
        {
            symbol_p->fixed = constant;
            if (symbol_p->fixed)
            {
                symbol_p->info_p = "floating point constant";
            }
            else
            {
                symbol_p->info_p = "floating point variable";
            }
            symbol_p->value.flt_val = value;
        }
    }
    return(IsBad);
}
/* ========================================================================
   creates or updates a string symbol table entry
=========================================================================== */
/*BOOL assign_string(char *token_p, char *value, BOOL constant)*/
BOOL STTST_AssignString(const char *const token_p, const char *const value_p, BOOL constant)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;

    if (strlen(token_p) != 0)
    {
        symbol_p = look_for(token_p, CLI_STR_SYMBOL);
        if (symbol_p == (symtab_t*) NULL)
        {
            symbol_p = insert_symbol(token_p, CLI_STR_SYMBOL);
            if (symbol_p == (symtab_t*) NULL)
            {
                IsBad = TRUE;
            }
            else
            {
                symbol_p->fixed = constant;
                if (symbol_p->fixed)
                {
                    symbol_p->info_p = "string constant";
                }
                else
                {
                    symbol_p->info_p = "string variable";
                }
                symbol_p->value.str_val = memory_allocate(
                    (partition_t *)sttst_InitParams.CPUPartition_p, strlen(value_p)+1);
                if (symbol_p->value.str_val == NULL)
                {
                    IsBad = TRUE;
                }
            }
        }
        else
        {
            if (symbol_p->fixed)
            {
                IsBad = TRUE;
            }
            else
            {
                if (strlen(value_p) > strlen(symbol_p->value.str_val))
                {
                    memory_deallocate(sttst_InitParams.CPUPartition_p, symbol_p->value.str_val);
                    symbol_p->value.str_val = memory_allocate(
                        (partition_t *)sttst_InitParams.CPUPartition_p, strlen(value_p)+1);
                    if (symbol_p->value.str_val == NULL)
                    {
                        IsBad = TRUE;
                    }
                }
            }
        }
        if (!IsBad)
        {
            strcpy(symbol_p->value.str_val, value_p);
        }
    }
    return(IsBad);
}
/* ========================================================================
   creates a string symbol table entry
=========================================================================== */
static BOOL create_string(const char * const token_p, char *value, BOOL constant)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;

    if (strlen(token_p) != 0)
    {
        symbol_p = insert_symbol(token_p, CLI_STR_SYMBOL);
        if (symbol_p == (symtab_t*) NULL)
        {
            IsBad = TRUE;
        }
        else
        {
            symbol_p->fixed = constant;
            if (symbol_p->fixed)
            {
                symbol_p->info_p = "string constant";
            }
            else
            {
                symbol_p->info_p = "string variable";
            }
            symbol_p->value.str_val = memory_allocate(
                (partition_t *)sttst_InitParams.CPUPartition_p, strlen(value)+1);
            if (symbol_p->value.str_val == NULL)
            {
                IsBad = TRUE;
            }
            else
            {
                strcpy(symbol_p->value.str_val, value);
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   delete an existing symbol (macro, variable, but not constant and command)
=========================================================================== */
BOOL STTST_DeleteSymbol(const char *const token_p)
{
    BOOL    IsBad = FALSE;
    symtab_t   *symbol_p;

    symbol_p = look_for(token_p, CLI_ANY_SYMBOL);
    if (symbol_p == (symtab_t*)NULL)
    {
        IsBad = TRUE; /* unknown symbol */
    }
    else if (symbol_p->fixed || (symbol_p->type == CLI_COM_SYMBOL))
    {
        IsBad = TRUE; /* fixed symbol or command */
    }
    else
    {
        IsBad = delete_symbol(token_p);
        /* if IsBad : symbol out of current scope */
    }
    return(IsBad);
}

/* ========================================================================
   establishes the existence of a command procedure
=========================================================================== */
BOOL STTST_RegisterCommand(const char *const token_p,
                              BOOL (*action)(STTST_Parse_t*, char*),
                              const char *const help_p)
{
    symtab_t   *symbol_p;
    BOOL    IsBad = FALSE;

    symbol_p = look_for(token_p, (CLI_COM_SYMBOL | CLI_MAC_SYMBOL));
    if (symbol_p != (symtab_t*) NULL)
    {
        IsBad = TRUE;
        sttst_Print("Name clash when registering command \"%s\"\n",token_p);
    }
    else
    {
        symbol_p = insert_symbol(token_p, CLI_COM_SYMBOL);
        if (symbol_p == (symtab_t*) NULL)
        {
            IsBad = TRUE;
        }
        else
        {
            symbol_p->fixed = TRUE;
            symbol_p->info_p = help_p;
            symbol_p->value.com_val = action;
        }
    }
    return(IsBad);
}

/* ========================================================================
   attempts a series of operations to extract an integer value from a token
=========================================================================== */
BOOL STTST_EvaluateInteger(const char *const token_p, S32 *value_p, S16 default_base)
{
    STTST_Parse_t *Parse_p;
    symtab_t      *symbol_p;
    BOOL          IsValid, IsBad = FALSE;

    if (token_p[0] == '\0')
    {
        IsBad = TRUE;
    }
    else if (token_p[0] == '-')
    { /* unary negative */
        IsBad = STTST_EvaluateInteger(token_p+1, value_p, default_base);
        if(!IsBad)
        {
            *value_p = -(*value_p);
        }
    }
    else if (token_p[0] == '~')
    { /* unary bitflip */
        IsBad = STTST_EvaluateInteger(token_p+1, value_p, default_base);
        if (!IsBad)
        {
            *value_p = ~(*value_p);
        }
    }
    else if (token_p[0] == '!')
    { /* unary NOT */
        IsBad = STTST_EvaluateInteger(token_p+1, value_p, default_base);
        if (!IsBad)
        {
            *value_p = !(*value_p);
        }
    }
    else
    {
        /*   First look for a simple number, then try a symbol reference. If this
         all fails, the value may be due to an expression, so try and evaluate it*/
        *value_p = conv_int(token_p, default_base, &IsValid);
        if (!IsValid)
        {
            symbol_p = look_for(token_p, CLI_INT_SYMBOL);
            if (symbol_p == (symtab_t*) NULL)
            {
                Parse_p = NewParseTableSlot(strlen(token_p)+1);
                init_pars(Parse_p, token_p);
                IsBad = evaluate_integer_expr(Parse_p, value_p, default_base);
                FreeParseTableSlot();
            }
            else
            {
                *value_p = symbol_p->value.int_val;
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   generalised recursive evaluate of a possibly bracketed integer expression.
   No leading spaces are allowed in the passed string, and the parsing
   structure is assumed to be initialised but not yet used
=========================================================================== */
static BOOL evaluate_integer_expr(STTST_Parse_t *Parse_p, S32 *value_p,
                              S16 default_base)
{
    BOOL  IsBad = FALSE;
    S16    brackets = 0;
    S32     value1, value2;
    S16    index;
    char   *sub_expr;
    char   operation;

    /* pair up a leading bracket */
    if (Parse_p->line_p[0] == '(')
    {
        brackets = 1;
        index = 1;
        while((Parse_p->line_p[index] != '\0') && (brackets > 0))
        {
          if (Parse_p->line_p[index] == ')') brackets--;
          if (Parse_p->line_p[index] == '(') brackets++;
          index++;
        }
        if (brackets != 0)
        {
            IsBad = TRUE;
        }
        else
        {
            /* copy substring without enclosing brackets and evaluate it.
             if this is ok, then update parsing start position */
            sub_expr = NewTokenTableSlot(index);
            strncpy(sub_expr,&(Parse_p->line_p[1]), index-2);
            sub_expr[index-2] = '\0';
            IsBad = STTST_EvaluateInteger(sub_expr, &value1, default_base);
            FreeTokenTableSlot(); /* sub_expr */
            if (!IsBad)
            {
                Parse_p->par_pos = index-1;
            }
        }
    }
    if (!IsBad)
    {
        /* look for a token and check for significance */
        get_tok(Parse_p, ArithmeticOpsInt);
        if (Parse_p->tok_del == '\0')
        { /* no operator seen*/
            if (Parse_p->par_sta > 0)       /* bracket removal code used */
            {
                *value_p = value1;
            }
            else
            {
                IsBad = TRUE;                /* not a valid expression! */
            }
        }
        else
        {
            /* have found an operator. If we stripped brackets in the first
             half of this code, then the interesting part of the line is
             now after the operator. If we did not, then evaluate both
             sides of operator */
            operation = Parse_p->tok_del;
            if (Parse_p->par_sta == 0)
            {
                IsBad = STTST_EvaluateInteger(Parse_p->token, &value1, default_base);
            }
            if (!IsBad)
            {
                get_tok(Parse_p, "");   /* all the way to the end of this line */
                IsBad = STTST_EvaluateInteger(Parse_p->token, &value2, default_base);
            }
            if (!IsBad)
            {
                switch(operation)
                {
                    case '+': *value_p = value1 + value2; break;
                    case '-': *value_p = value1 - value2; break;
                    case '*': *value_p = value1 * value2; break;
                    case '/': *value_p = value1 / value2; break;
                    case '&': *value_p = value1 & value2; break;
                    case '|': *value_p = value1 | value2; break;
                    case '^': *value_p = value1 ^ value2; break;
                    case '%': *value_p = value1 % value2; break;
                    default: IsBad = TRUE; break;
                }
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   attempts a series of operations to extract a floating pt value from a tok
=========================================================================== */
/*BOOL evaluate_float(char *token_p, double *value_p)*/
BOOL STTST_EvaluateFloat(const char *const token_p, double *value_p)
{
    STTST_Parse_t  *Parse_p;
    symtab_t *symbol_p;
    BOOL     IsBad = FALSE;

    /* this will only be relevant for non null strings w/o leading white space.
    First look for a simple number, then try a symbol reference. If this
    all fails, the value may be due to an expression, so try and evaluate it*/
    /* sttst_Print("float evaluation of %s \n",token_p); */
    if (token_p[0] == '\0')
    {
        IsBad = TRUE;
    }
    else
    {
        IsBad = conv_flt(token_p, value_p);
        if (IsBad)
        {
            /* a symbol would be legal if it were an integer, but we had to make it float */
            symbol_p = look_for(token_p, CLI_FLT_SYMBOL | CLI_INT_SYMBOL);
            if (symbol_p == (symtab_t*) NULL)
            {
                Parse_p = NewParseTableSlot(strlen(token_p)+1);
                init_pars(Parse_p, token_p);
                IsBad = evaluate_float_expr(Parse_p, value_p);
                FreeParseTableSlot();
            }
            else
            {
                IsBad = FALSE;
                if (symbol_p->type == CLI_FLT_SYMBOL)
                {
                    *value_p = symbol_p->value.flt_val;
                }
                else
                {
                    *value_p = (double) symbol_p->value.int_val;
                }
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   generalised recursive evaluate of a, possibly bracketed, float expression
   No leading spaces are allowed in the passed string, and the parsing
   structure is assumed to be initialised but not yet used
=========================================================================== */
static BOOL evaluate_float_expr(STTST_Parse_t *Parse_p, double *value_p)
{
    BOOL   IsBad = FALSE;
    S16    brackets = 0;
    double value1, value2;
    S16    index;
    char   operation;
    char   *sub_expr;

    if (Parse_p->line_p[0] == '(')
    {
        brackets = 1;
        index = 1;
        while((Parse_p->line_p[index] != '\0') && (brackets > 0))
        {
            if (Parse_p->line_p[index] == ')') brackets--;
            if (Parse_p->line_p[index] == '(') brackets++;
            index++;
        }
        if (brackets != 0)
        {
            IsBad = TRUE;
        }
        else
        {
            /* copy substring without enclosing brackets and evaluate it.
             if this is ok, then update parsing start position */
            sub_expr = NewTokenTableSlot(index);
            strncpy(sub_expr,&(Parse_p->line_p[1]), index-2);
            sub_expr[index-2] = '\0';
            IsBad = STTST_EvaluateFloat(sub_expr, &value1);
            FreeTokenTableSlot(); /* sub_expr */
            if (!IsBad)
            {
                Parse_p->par_pos = index-1;
            }
        }
    }
    if (!IsBad)
    {
        /* look for a token and check for significance */
        get_tok(Parse_p, ArithmeticOpsFloat);
        if (Parse_p->tok_del == '\0')
        { /* no operator seen*/
            if (Parse_p->par_sta > 0)       /* bracket removal code used */
            {
                *value_p = value1;
            }
            else
            {
                IsBad = TRUE;                /* not a valid expression! */
            }
        }
        else
        {
          /* have found an operator. If we stripped brackets in the first
             half of this code, then the interesting part of the line is
             now after the operator. If we did not, then evaluate both
             sides of operator */
            operation = Parse_p->tok_del;
            if (Parse_p->par_sta == 0)
            {
                IsBad = STTST_EvaluateFloat(Parse_p->token, &value1);
            }
            if (!IsBad)
            {
                get_tok(Parse_p, "");   /* all the way to the end of this line */
                IsBad = STTST_EvaluateFloat(Parse_p->token, &value2);
            }
            if (!IsBad)
            {
                switch(operation)
                {
                    case '+': *value_p = value1 + value2; break;
                    case '-': *value_p = value1 - value2; break;
                    case '*': *value_p = value1 * value2; break;
                    case '/': *value_p = value1 / value2; break;
                    default: IsBad = TRUE;
                }
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   specific evaluation of comparison between two numeric expressions.
   No leading spaces are allowed in the passed string.
   A single expression will yield a result of FALSE if equal to zero.
   Although real expressions are allowed, comparison is done in integers.
   A default result of TRUE is returned in the case of an error
=========================================================================== */
/*BOOL evaluate_comparison(char *token_p, BOOL *result_p,
                            short default_base)*/
BOOL STTST_EvaluateComparison(const char *const token_p, BOOL *result_p,
                            S16 default_base)
{
    BOOL    IsBad = FALSE;
    BOOL    RightResult, AndFound, OrFound, LogicalNOTFound, StayInBrackets;
    STTST_Parse_t *Parse_p;
    S32     value1, value2;
    S16     brackets = 0;
    S16     index;
    U8      FirstExprType;
    double  real_val;
    char    op1,op2 = 0;
    char    *str1 = NULL;
    char    *str2, *sub_expr;
    int     cmp;

    Parse_p = NewParseTableSlot(strlen(token_p)+1);
    *result_p = FALSE;

    /* ignore spaces before brackets */
    init_pars(Parse_p, token_p);
    index = 0;
    StayInBrackets = FALSE;
    while ((Parse_p->line_p[index] == ' ') && (Parse_p->line_p[index] != '\0')) index++;

    if ( (Parse_p->line_p[index] == '('))
    {
        index ++;
        brackets = 1;
        StayInBrackets = TRUE;
        while((Parse_p->line_p[index] != '\0') && (brackets > 0))
        {
          if (Parse_p->line_p[index] == ')') brackets--;
          if (Parse_p->line_p[index] == '(') brackets++;
          index ++;
        }
        if (brackets != 0)
        {
            FreeParseTableSlot(); /* Parse_p */
            return(TRUE);
        }
        /* look if thers is any Logical Operator inside.
            * If so, evaluate it, else, this is a expression to be calculated : leave brackets */
        get_tok(Parse_p, LogicalOps);
        if (Parse_p->par_pos >= index) /* outside bracket */
        {
            StayInBrackets = FALSE;
        }
        init_pars(Parse_p, token_p);
    } /* if ( (Parse_p->line_p[index] == '(')) */

    if (StayInBrackets)
    {
        /* copy substring without enclosing brackets and evaluate it.
            if this is ok, then update parsing start position */
        sub_expr = NewTokenTableSlot(index);
        strncpy(sub_expr,&(Parse_p->line_p[1]), index-2);
        sub_expr[index-2] = '\0';
        IsBad = STTST_EvaluateComparison(sub_expr, result_p, default_base);
        FreeTokenTableSlot(); /* sub_expr */
        if (!IsBad)
        {
            Parse_p->par_pos = index-1;
            get_tok(Parse_p, ComparisonOps);
            if (Parse_p->tok_del != '\0')
            {
                sub_expr = &(Parse_p->line_p[Parse_p->par_pos]);
                op1 = *sub_expr;
                sub_expr ++;
                if (is_delim(*sub_expr, ComparisonOps))
                {
                    if (*sub_expr == op1)
                    {
                        sub_expr ++;
                    }
                    else
                    {
                        IsBad = TRUE;
                    }
                }
                if (!IsBad)
                {
                    IsBad = STTST_EvaluateComparison(sub_expr, &RightResult, default_base);
                }
                if (!IsBad)
                {
                    if (op1 == '|')
                    {
                        *result_p |= RightResult;
                    }
                    else if (op1 == '&')
                    {
                        *result_p &= RightResult;
                    }
                }
            }
        }
        FreeParseTableSlot(); /* Parse_p */
        return(IsBad);
    }

    get_tok(Parse_p, LogicalOps);
    LogicalNOTFound = FALSE;
    if ( (Parse_p->line_p[Parse_p->par_pos] == '!') && (Parse_p->line_p[Parse_p->par_pos+1] != '='))
    {
        LogicalNOTFound = TRUE;
        sub_expr = NewTokenTableSlot(strlen(token_p));
        strncpy(sub_expr,&(Parse_p->line_p[Parse_p->par_pos+1]), strlen(token_p));
        sub_expr[strlen(token_p)-1] = '\0';
        IsBad = STTST_EvaluateComparison(sub_expr , result_p, default_base);
        FreeTokenTableSlot(); /* sub_expr */
        if (!IsBad )
        {
            *result_p = !(*result_p);
        }
        FreeParseTableSlot(); /* Parse_p */
        return(IsBad);
    }

    /* look for AND (&&) or OR (||)*/
    AndFound = OrFound = FALSE;
    init_pars(Parse_p, token_p);
    get_tok(Parse_p, ComparisonOps);
    if ( (Parse_p->tok_del == '&') && (Parse_p->line_p[Parse_p->par_pos+1] == '&'))
    {
        AndFound = TRUE;
    }
    else if ( (Parse_p->tok_del == '|') && (Parse_p->line_p[Parse_p->par_pos+1] == '|'))
    {
        OrFound = TRUE;
    }
    if (AndFound || OrFound)
    {
        sub_expr = NewTokenTableSlot(Parse_p->par_pos+1);
        strncpy(sub_expr,Parse_p->line_p, Parse_p->par_pos);
        sub_expr[Parse_p->par_pos] = '\0';
        IsBad = STTST_EvaluateComparison(sub_expr, result_p, default_base);
        FreeTokenTableSlot(); /* sub_expr */
        if (!IsBad)
        {
            sub_expr = &(Parse_p->line_p[Parse_p->par_pos+2]);
            IsBad = STTST_EvaluateComparison(sub_expr, &RightResult, default_base);
        }
        if (!IsBad)
        {
            if (OrFound)
            {
                *result_p |= RightResult;
            }
            else /* AndFound */
            {
                *result_p &= RightResult;
            }
        }
        FreeParseTableSlot(); /* Parse_p */
        return(IsBad);
    }

    /* evaluate first arithmetic expression in line */
    init_pars(Parse_p, token_p);
    get_tok(Parse_p, LogicalOps);
    FirstExprType = CLI_INT_SYMBOL;
    IsBad = STTST_EvaluateInteger(Parse_p->token, &value1, default_base);
    if (IsBad) /* not an integer */
    {
        FirstExprType = CLI_FLT_SYMBOL;
        IsBad = STTST_EvaluateFloat(Parse_p->token, &real_val);
        if (!IsBad)
        {
            value1 = (S32) real_val;
        }
        if (IsBad)
        {
            FirstExprType = CLI_STR_SYMBOL;
            str1 = NewTokenTableSlot(CLI_MAX_LINE_LENGTH);
            IsBad = STTST_EvaluateString(Parse_p->token, str1, CLI_MAX_LINE_LENGTH);
        }
    }
    if (!IsBad)
    {
        /* deal with the case of a single expression value */
        if (Parse_p->tok_del == '\0')
        {
           if (FirstExprType == CLI_INT_SYMBOL)
           {
               *result_p = !(value1 == 0);
           }
           else
           {
               *result_p = FALSE;
           }
        }
        else
        {
            /* look for valid single or pair of operators and move posn accordingly */
            op1 = Parse_p->line_p[Parse_p->par_pos];
            op2 = Parse_p->line_p[Parse_p->par_pos+1];
            if ((op2 != '>') && (op2 != '<') && (op2 != '='))
            {
                op2 = 0;
            }
            else
            {
                Parse_p->par_pos++;
            }
            /* get token for rest of line and extract a value in a similar way */
            get_tok(Parse_p, "");
            IsBad = STTST_EvaluateInteger(Parse_p->token, &value2, default_base);
            if (IsBad)
            {
                IsBad = STTST_EvaluateFloat(Parse_p->token, &real_val);
                if (!IsBad)
                {
                    value2 = (S32) real_val;
                }
                if (IsBad)
                {
                    if (FirstExprType != CLI_STR_SYMBOL) /* str1 is unset */
                    {
                        *result_p = FALSE;
                        FreeParseTableSlot(); /* Parse_p */
                        return(FALSE);
                    }
                    str2 = NewTokenTableSlot(CLI_MAX_LINE_LENGTH);
                    IsBad = STTST_EvaluateString(Parse_p->token, str2, CLI_MAX_LINE_LENGTH);
                    cmp=strcmp(str1,str2);
                    FreeTokenTableSlot(); /* str2 */
                    FreeTokenTableSlot(); /* str1 */
                    *result_p = FALSE;
                    switch(op1)
                    {
                        case '=': switch(op2) {
                                  case '=':  if (cmp==0) *result_p = TRUE; break;
                                  case '!':  if (cmp!=0) *result_p = TRUE; break;
                                  case '<':  if (cmp<=0) *result_p = TRUE; break;
                                  case '>':  if (cmp>=0) *result_p = TRUE; break;
                                  default :  IsBad = TRUE; break;
                                } break;
                        case '!': switch(op2) {
                                  case '=':  if (cmp!=0) *result_p = TRUE; break;
                                  default :  IsBad = TRUE; break;
                                } break;
                        case '>': switch(op2) {
                                  case '=':  if (cmp>=0) *result_p = TRUE; break;
                                  case  0 :  if (cmp>0) *result_p = TRUE;  break;
                                  default :  IsBad = TRUE; break;
                                } break;
                        case '<': switch(op2) {
                                  case '=':  if (cmp<=0) *result_p = TRUE; break;
                                  case '>':  if (cmp!=0) *result_p = TRUE; break;
                                  case  0 :  if (cmp<0) *result_p = TRUE;  break;
                                  default :  IsBad = TRUE; break;
                                } break;
                    }
                    FreeParseTableSlot(); /* Parse_p */
                    return(IsBad);
                }
            }
            if (!IsBad) /* value2 has been set */
            {
                if (FirstExprType == CLI_STR_SYMBOL) /* value1 unset */
                {
                    *result_p = FALSE;
                    FreeParseTableSlot(); /* Parse_p */
                    return(FALSE);
                }
                /* deal with the combination of two expression values */
                switch(op1)
                {
                    case '=': switch(op2) {
                              case '=':  *result_p = (value1 == value2); break;
                              case '!':  *result_p = (value1 != value2); break;
                              case '<':  *result_p = (value1 <= value2); break;
                              case '>':  *result_p = (value1 >= value2); break;
                              default :  IsBad = TRUE; break;
                            } break;
                    case '!': switch(op2) {
                              case '=':  *result_p = (value1 != value2); break;
                              default :  IsBad = TRUE; break;
                            } break;
                    case '>': switch(op2) {
                              case '=':  *result_p = (value1 >= value2); break;
                              case  0 :  *result_p = (value1 > value2);  break;
                              default :  IsBad = TRUE; break;
                            } break;
                    case '<': switch(op2) {
                              case '=':  *result_p = (value1 <= value2); break;
                              case '>':  *result_p = (value1 != value2); break;
                              case  0 :  *result_p = (value1 < value2);  break;
                              default :  IsBad = TRUE; break;
                            } break;
                }
            }
        }
    }
    FreeParseTableSlot(); /* Parse_p */
    return(IsBad);
}
/* ========================================================================
   attempts a series of operations to extract a string value from a token
=========================================================================== */
/*BOOL evaluate_string(char *token_p, char *string_p, short max_len)*/
BOOL STTST_EvaluateString(const char *const token_p, char *string_p, S16 max_len)
{
    symtab_t *symbol_p;
    BOOL     IsBad = FALSE;
    S16      index,i,len;
    S32      int_val;
    double   real_val;
    STTST_Parse_t  *Parse_p;

    string_p[0] = '\0';
    /* sttst_Print("string evaluation of %s \n",token_p); */
    if (token_p[0] == '\0')
    {
        IsBad = TRUE;
    }
    else
    {
        Parse_p = NewParseTableSlot(strlen(token_p)+1);
        /* look for concatenation function */
        init_pars(Parse_p, token_p);
        get_tok(Parse_p,"+");
        if (Parse_p->tok_del == '+')
        {
            /* call routine on halves of passed token */
            IsBad = STTST_EvaluateString(Parse_p->token, string_p, max_len);
            if (!IsBad)
            {
                do
                {
                    get_tok(Parse_p, "+"); /* next '+' or rest of line */
                    IsBad  = STTST_EvaluateInteger(Parse_p->token, &int_val, NumberBase);
                    if (!IsBad)
                    {
                        sprintf(string_p, "%s%d", string_p, int_val);
                    }
                    else
                    {
                        IsBad  = STTST_EvaluateFloat(Parse_p->token, &real_val);
                        if (!IsBad)
                        {
                            sprintf(string_p, "%s%G", string_p, real_val);
                        }
                        else
                        {
                            len = strlen(string_p);
                            IsBad = STTST_EvaluateString(Parse_p->token, &string_p[len], max_len-len);
                        }
                    }
                } while (!IsBad && (Parse_p->tok_del == '+'));
            }
        }
        else
        {
            symbol_p = look_for(token_p, CLI_STR_SYMBOL);
            if (symbol_p != (symtab_t*)NULL)
            {
                if ( ((S16)strlen(symbol_p->value.str_val))<=max_len )
                {
                    strcpy(string_p, symbol_p->value.str_val);
                }
                else
                {
                    sttst_Print("symbol string value too long\n");
                    IsBad = TRUE;
                }
            }
            else
            {
                /* we assume since this is already been passed through the tokeniser that this
                is a valid string - starting with a quote mark and ending with a quote mark
                only containing escaped quote marks within its body. All that is needed
                for evaluation is to strip out the escapes */
                len = strlen(token_p);
                if ( len< max_len && ((token_p[0] == '"') && (token_p[len-1] =='"')) )
                {
                    /* copy substring without enclosing quotes, preserving case */
                    index = 1;
                    i=0;
                    while((token_p[index] != '\0') && (index < (len-1)))
                    {
                        if ((token_p[index+1] == '"') && (token_p[index] == CLI_ESCAPE_CHAR))
                        {
                            string_p[i++] = token_p[index+1];
                            index++;
                        }
                        else
                        {
                            string_p[i++] = token_p[index];
                        }
                        index++;
                    }
                    string_p[i] = '\0';
                    len = i;
                }
                else
                {
                    if ( len >= max_len )
                    {
                        sttst_Print("string value too long\n");
                    }
                    IsBad = TRUE;
                }
            }
        }
        FreeParseTableSlot();
    }
    return(IsBad);
}
/* ========================================================================
   attempts multiple evaluations of a token. If successful, assigns a
   symbol of appropriate type to the resultant value . Two retuns are
   provided. 'error_p' signififies that the token was in itself
   correct, but the assignment to the target symbol failed. 'not_done'
   is used to flag an unsuccessful evaluation. The force parameter can
   be used to force creation of the variable to hold the result without
   looking for possible match in symbol table
=========================================================================== */
static BOOL evaluate_assign(const char *const target_p, const char * const token_p, BOOL* error_p, BOOL force)
{
    BOOL   not_done = TRUE;
    S32    int_val;
    double real_val;
    char   *string_val;

    *error_p = FALSE;
    if (strlen(target_p) != 0)
    {
        not_done = STTST_EvaluateInteger(token_p, &int_val, NumberBase);
        if (!not_done)
        {
            if (force)
            {
                *error_p = create_integer(target_p, int_val, FALSE);
            }
            else
            {
                *error_p = STTST_AssignInteger(target_p, int_val, FALSE);
            }
        }
        else
        {
            not_done = STTST_EvaluateFloat(token_p, &real_val);
            if (!not_done)
            {
                if (force)
                {
                    *error_p = create_float(target_p, real_val, FALSE);
                }
                else
                {
                    *error_p = STTST_AssignFloat(target_p, real_val, FALSE);
                }
            }
            else
            {
                string_val = NewTokenTableSlot(CLI_MAX_LINE_LENGTH);
                not_done = STTST_EvaluateString(token_p, string_val, CLI_MAX_LINE_LENGTH);
                if (!not_done)
                {
                    if (force)
                    {
                            *error_p = create_string(target_p, string_val, FALSE);
                    }
                    else
                    {
                            *error_p = STTST_AssignString(target_p, string_val, FALSE);
                    }
                }
                FreeTokenTableSlot(); /* string_val */
            }
        }
    }
    return(not_done);
}
/* ========================================================================
   a high level parsing routine which forms part of a set. The general
   specification is the incremental reading of an input string via the
   parsing structure, detecting a parameter refernce of the right type and
   passing back a result. If the input line at this point is blank, a default
   value is substituted for a result. If the next token on the input line
   does not have the right characteristics for a parameter of this type, then
   the routine returns TRUE to signify an error. The default parameter is
   passed back in this case as well as an error.
=========================================================================== */
/*BOOL cget_string(STTST_Parse_t *Parse_p, char *default_p, char *result_p,
                                                     short max_len)*/
BOOL STTST_GetString(STTST_Parse_t *Parse_p, const char *const default_p, char *const result_p,
                                                     S16 max_len)
{
    BOOL  IsBad = FALSE;


    get_tok(Parse_p, DelimiterSet);
    if (Parse_p->tok_len == 0)
    {
        result_p[0] = 0;
        if (default_p != NULL)
        {
            strncpy(result_p, default_p, max_len);
        }
    }
    else
    {
        IsBad = STTST_EvaluateString(Parse_p->token, result_p, max_len);
        if (IsBad)
        {
            result_p[0] = 0;
            if (default_p != NULL)
            {
                strncpy(result_p, default_p, max_len);
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   a high level parsing routine which forms part of a set. The general
   specification is the incremental reading of an input string via the
   parsing structure, detecting a parameter refernce of the right type and
   passing back a result. If the input line at this point is blank, a default
   value is substituted for a result. If the next token on the input line
   does not have the right characteristics for a parameter of this type, then
   the routine returns TRUE to signify an error. The default parameter is
   passed back in this case as well.
=========================================================================== */
/*BOOL cget_integer(STTST_Parse_t *Parse_p, long def_int, long *result_p)*/
BOOL STTST_GetInteger(STTST_Parse_t *Parse_p, S32 def_int, S32 *result_p)
{
    BOOL  IsBad = FALSE;

    get_tok(Parse_p, DelimiterSet);
    if (Parse_p->tok_len == 0)
    {
        *result_p = def_int;
    }
    else
    {
        IsBad = STTST_EvaluateInteger(Parse_p->token, result_p, NumberBase);
        if (IsBad)
        {
            *result_p = def_int;
        }
    }
    return(IsBad);
}
/* ========================================================================
   a high level parsing routine which forms part of a set. The general
   specification is the incremental reading of an input string via the
   parsing structure, detecting a parameter refernce of the right type and
   passing back a result. If the input line at this point is blank, a default
   value is substituted for a result. If the next token on the input line
   does not have the right characteristics for a parameter of this type, then
   the routine returns TRUE to signify an error. The default parameter is
   passed back in this case as well.
=========================================================================== */
/*BOOL cget_float(STTST_Parse_t *Parse_p, double def_flt, double *result_p)*/
BOOL STTST_GetFloat(STTST_Parse_t *Parse_p, double def_flt, double *result_p)
{
    BOOL  IsBad = FALSE;

    get_tok(Parse_p, DelimiterSet);
    if (Parse_p->tok_len == 0)
    {
        *result_p = def_flt;
    }
    else
    {
        IsBad = STTST_EvaluateFloat(Parse_p->token, result_p);
        if (IsBad)
        {
            *result_p = def_flt;
        }
    }
    return(IsBad);
}
/* ========================================================================
   a high level parsing routine which forms part of a set. The general
   specification is the incremental reading of an input string via the
   parsing structure, detecting a parameter refernce of the right type and
   passing back a result. If the input line at this point is blank, a default
   value is substituted for a result. If the next token on the input line
   does not have the right characteristics for a parameter of this type, then
   the routine returns TRUE to signify an error. The default parameter is
   passed back in this case as well.
=========================================================================== */
/*BOOL cget_item(STTST_Parse_t *Parse_p, char *default_p, char *result_p, short max_len)*/
BOOL STTST_GetItem(STTST_Parse_t *Parse_p, const char *const default_p, char *const result_p, S16 max_len)
{
    BOOL  IsBad = FALSE;

    /* simply returns next token on line or the default string */
    get_tok(Parse_p, DelimiterSet);
    if (Parse_p->tok_len == 0)
    {
        strncpy(result_p, default_p, max_len);
    }
    else
    {
        strncpy(result_p, Parse_p->token, max_len);
    }
    return(IsBad);
}
/* ========================================================================
   gets the number of token in the line to be parsed (created by FQ)
=========================================================================== */
/*BOOL cget_token_count(STTST_Parse_t *Parse_p, short *result_p)*/
BOOL STTST_GetTokenCount(STTST_Parse_t *Parse_p, S16 *result_p)
{
    char *pt;
    S16 car_cnt, tok_cnt;

    car_cnt = 0;
    tok_cnt = 0;
    pt = Parse_p->line_p;
    do
    {
        if ( *pt!=' ' && *pt!='\0' && *pt!=CLI_NL_CHAR && *pt!=CLI_CR_CHAR )
        {
            pt++;
            car_cnt++;
            if ( *pt==' ' || *pt=='\0' || *pt==CLI_NL_CHAR || *pt==CLI_CR_CHAR )
            {
                /* end of a token (alphanum carac. followed by a separator) */
                tok_cnt++;
            }
        }
        else
        {
            pt++;
            car_cnt++;
        }
    }
    while ( !( *pt=='\0' || car_cnt >= CLI_MAX_LINE_LENGTH ) );

    *result_p = tok_cnt;
    return(FALSE);
}
/* ========================================================================
   obtains input line from stdin
=========================================================================== */
static S16 read_stdin(char *line_p, S16 max_line_size)
{
    BOOL ok_to_store;
    BOOL first_cmd_call;
    char current_char;
    S16 cnt;
    S16 current_line_len;
    S16 previous_line_len;
    S16 cursor_pos;
    S16 current_cmd;
    char *pt, *pt2;

    /* Note : for better performance, do STTBX_Print() with a string instead a single character */

    pt = line_p;
    cursor_pos = 0;
    current_line_len = 0;
    previous_line_len = 0;
    first_cmd_call = TRUE;
    current_cmd = CommandHistory.last_cmd;
    do
    {
        STTBX_InputChar( &current_char );
        /* it is a ctrl key, a backspace, an alphanum. character ? */
        if ( current_char==KEY_UP || current_char==KEY_DOWN )
        {
            /* --- Call the last command --- */

            if ( CommandHistory.last_cmd > 0 )
            {
                if ( !first_cmd_call )
                {
                    previous_line_len = current_line_len ;
                    if ( current_char==KEY_UP && current_cmd>0 )
                    {
                        current_cmd--;
                    }
                    if ( ( current_char==KEY_DOWN || current_char==UNIX_KEY_DOWN )
                          && current_cmd<CommandHistory.last_cmd )
                    {
                        current_cmd++;
                    }
                }
                first_cmd_call = FALSE;
                /* erase all characters at the left of the cursor position */
                /* and move the cursor beside the prompt */
                DisplayBuffer_p[0] = '\0';
                cnt = cursor_pos ;
                while ( cnt>0 )
                {
                    strcat( DisplayBuffer_p, "\b \b");
                    cnt--;
                }
                /* put the required command from history */
                current_line_len = strlen(CommandHistory.cmd_name[current_cmd]);
                cursor_pos = current_line_len;
                strcpy(line_p, CommandHistory.cmd_name[current_cmd] );
                strcat( DisplayBuffer_p, line_p );

                /* is the previous command longer than the current one ? */
                if ( current_line_len < previous_line_len )
                {
                    /* erase characters on the right */
                    pt = DisplayBuffer_p + strlen(DisplayBuffer_p);
                    cnt = previous_line_len - current_line_len;
                    while ( cnt>0 )
                    {
                        *pt= ' ';
                        pt++;
                        cnt--;
                    }
                    /* move cursor to the left */
                    cnt = previous_line_len - current_line_len;
                    while ( cnt>0 )
                    {
                        *pt= '\b';
                        pt++;
                        cnt--;
                    }
                    *pt = '\0';
                }
                /* display the buffer */
                cnt = strlen(DisplayBuffer_p);
                pt2 = DisplayBuffer_p;
                while ( cnt > MAX_STTBX_LEN ) /* cut the string to avoid STTBX limit */
                {
                    STTBX_Print(("%*.*s",MAX_STTBX_LEN,MAX_STTBX_LEN,pt2));
                    pt2 += MAX_STTBX_LEN;
                    cnt -= MAX_STTBX_LEN;
                }
                STTBX_Print(("%s",pt2));

                /* cursor is at line end, set pointer here */
                pt = line_p + current_line_len;
            }
            /* else no command to recall */
        }
        else if ( current_char==KEY_RIGHT )
        {
            /* --- Repeat last character --- */

            if (!first_cmd_call) /* command recall already done */
            {
                if ( cursor_pos < (S16)strlen(CommandHistory.cmd_name[current_cmd]) )
                {
                    current_char = CommandHistory.cmd_name[current_cmd][cursor_pos];
                    STTBX_Print(("%c",current_char));
                    cursor_pos++;
                    *pt = current_char;
                    current_line_len++;
                    pt++;
                }
            }
        }
        else if ( current_char==KEY_LEFT )
        {
            /* --- Move cursor on the left --- */

            if ( cursor_pos > 0 )
            {
                STTBX_Print(("\b"));
                cursor_pos--;
                pt--;
            }
        }
        else if ( current_char=='\b' || current_char==UNIX_KEY_BACKSP )
        {
            /* --- Erase previous character --- */

            if ( current_line_len > 0 )
            {
                if ( cursor_pos < current_line_len && cursor_pos >= 1 )
                {
                    cnt = current_line_len - cursor_pos;
                    /* store end of the line */
                    memset( DisplayBuffer_p, 0, DISPLAY_BUFFER_SIZE);
                    DisplayBuffer_p[0] = '\b';
                    strncpy(DisplayBuffer_p +1, pt, cnt);
                    /* update new input contents (shift end of line to the left */
                    strcpy(pt-1, DisplayBuffer_p+1);
                    cnt = strlen(DisplayBuffer_p);
                    /* erase end of line with a blank */
                    DisplayBuffer_p[cnt] = ' ';
                    /* move cursor on the left */
                    while ( cnt > 0 )
                    {
                        DisplayBuffer_p[strlen(DisplayBuffer_p)] = '\b';
                        cnt--;
                    }
                    /* display the buffer */
                    cnt = strlen(DisplayBuffer_p);
                    pt2 = DisplayBuffer_p;

                    while ( cnt > MAX_STTBX_LEN ) /* cut the string to avoid STTBX limit */
                    {
                        STTBX_Print(("%*.*s",MAX_STTBX_LEN,MAX_STTBX_LEN,pt2));
                        pt2 += MAX_STTBX_LEN;
                        cnt -= MAX_STTBX_LEN;
                    }
                    STTBX_Print(("%s",pt2));

                    current_line_len--;
                    cursor_pos--;
                    pt--;
                }
                if ( cursor_pos == current_line_len )
                {
                    /* erase last character & put cursor on the left */
#ifndef ST_OSWINCE
	#ifndef CONFIG_POSIX
						STTBX_Print(("\b \b"));
	#else
						STTBX_Print((" \b"));
	#endif
#else
						STTBX_Print(("\b"));

#endif
                    *pt = '\0';
                    current_line_len--;
                    cursor_pos--;
                    pt--;
                }
            }
        }
        else
        {
            /* --- Insert new character --- */

            if ( current_line_len+1 >= max_line_size )
            {
                STTBX_Print(("\a")); /* alert character */
            }
            else
            {
                if ( current_char!=CLI_CR_CHAR && current_char!=CLI_NL_CHAR )
                {
                    if ( cursor_pos < current_line_len ) /* insert the new character... */
                    {
                        /* save the end of the string before shifting it to the right */
                        memset( DisplayBuffer_p, 0, DISPLAY_BUFFER_SIZE);
                        cnt = current_line_len - cursor_pos;
                        strncpy(DisplayBuffer_p, pt, cnt);
                        /* update new input line contents */
                        *pt = current_char;
                        strncpy(pt+1, DisplayBuffer_p, cnt);
                        /* prepare the new string to display */
                        cnt++;
                        strncpy(DisplayBuffer_p, pt, cnt);
                        /* move the cursor to the left */
                        pt2 = DisplayBuffer_p +cnt;
                        while ( cnt > 1 )
                        {
                            *pt2 = '\b';
                            pt2++;
                            cnt--;
                        }
                        STTBX_Print(("%s",DisplayBuffer_p));
                    }
                    else  /* append and echo the new character... */
                    {
                        *pt = current_char;
#ifndef CONFIG_POSIX
                        STTBX_Print(("%c",current_char));
#endif
                    }
                    current_line_len++;
                    cursor_pos++;
                    pt++;
                }
            }
        }
    }
    while( current_char!=CLI_CR_CHAR && current_char!=CLI_NL_CHAR );
    /* end on input : \r on Windows, \n on Unix */

    /* close the input string & echo a new line */
    line_p[current_line_len] = '\0';
    STTBX_Print(("\n"));

    /* --- Update the command history --- */

    if ( strlen(line_p) > 1 )
    {
        ok_to_store = TRUE;
        /* is it a new command ? if yes, keep it */
        if ( strcmp( CommandHistory.cmd_name[CommandHistory.last_cmd], line_p ) != 0 )
        {
            /* is it the history command itself ? if yes, ignore it */
            if ( strncmp( line_p, "hi", 2)==0 || strncmp( line_p, "HI", 2)==0 )
            {
                if ( look_for( line_p, CLI_COM_SYMBOL) != (symtab_t *)NULL )
                {
                    ok_to_store = FALSE;
                }
            }
        }
        else
        {
            ok_to_store = FALSE;
        }
        if ( ok_to_store )
        {
            CommandHistory.last_cmd++;
            if (CommandHistory.last_cmd>=STTST_MAX_TOK_LEN)
                CommandHistory.last_cmd = 1;
            if (CommandHistory.last_cmd==CommandHistory.old_cmd)
                CommandHistory.old_cmd++;
            if (CommandHistory.old_cmd>=STTST_MAX_TOK_LEN)
                CommandHistory.old_cmd = 1;
            strcpy(CommandHistory.cmd_name[CommandHistory.last_cmd], line_p);
        }
    }
    return(current_line_len);
}
/* ========================================================================
   obtains input line from current active source, returns FALSE if OK
   Should this routine do echo of input lines?
   Should this routine issue a prompt for interactive input?
=========================================================================== */
static BOOL read_input(char *line_p, const char *const ip_prompt_p)
{
    S16 cnt =0;
    int   temp = EOF;
    /*BOOL ok_to_store;*/

    if (CurrentMacro_p == (macro_t*) NULL)
    {
        if (CurrentStream_p == (FILE*)NULL)
        {
            sttst_Print("Internal error - asked to read from NULL file \n\n\n");
            exit(1);
        }
        else
        {
            if (CurrentStream_p == stdin)
            {
                /* if we are at 'console level' use a user supplied input routine */
                temp = 0;
                /*cnt = io_read(ip_prompt_p, line_p, CLI_MAX_LINE_LENGTH); FQ */

                STTBX_Print((ip_prompt_p));

                /*cnt = STTBX_InputStr(line_p, CLI_MAX_LINE_LENGTH);*/
                cnt = read_stdin(line_p, CLI_MAX_LINE_LENGTH);
            }
#ifndef UNHOSTED
            else
            {
                temp = 0;
                line_p[cnt] = getc(CurrentStream_p);
                while ((cnt < CLI_MAX_LINE_LENGTH) && (line_p[cnt] != '\n') &&
                       ((temp = getc(CurrentStream_p)) != EOF))
                {
                    line_p[++cnt] = temp;
                }
                line_p[cnt] = '\0';
            }
            if (LogFile != NULL && !LogResults )
            {
                fprintf(LogFile, "%s\n", line_p);
            }
#endif
        }
    }
    else
    {
        temp = (CurrentMacro_p->line_p == (macro_t*) NULL);
        strcpy(line_p, CurrentMacro_p->line);
        CurrentMacro_p = CurrentMacro_p->line_p;
    }
    /* echo all non-interactive input if required */
    if (CurrentEcho && ((MacroDepth > 1) || (CurrentStream_p != stdin)))
    {
        sttst_Print("-> %s\n", line_p);
    }
    return(temp == EOF);
}
/* ========================================================================
   accepts a sequence of input lines that form the body of a macro. The
   definition line passed in contains the formal parameter part of the
   command line that triggered the definition and may be null. Macro
   definition is ended when the first token on a line is found to be 'END'
=========================================================================== */
static BOOL define_macro(const char *const defline_p, char *name_p)
{
    symtab_t   *symbol_p;
    macro_t    *cur_line_p, *last_line_p;
    STTST_Parse_t    *Parse_p;
    BOOL    end = FALSE;
    BOOL    IsBad = FALSE;
    BOOL    eof;
    S16      temp;
    S16      nests = 1;
    partition_t *mem_pt;

    symbol_p = look_for(name_p, CLI_ANY_SYMBOL);
    if ((symbol_p == (symtab_t*) NULL) && (name_p[0] != '\0'))
    {
        symbol_p = insert_symbol(name_p, CLI_MAC_SYMBOL);
        if (symbol_p == (symtab_t*) NULL)
        {
            sttst_Print("Cannot create macro \"%s\" \n", name_p);
            IsBad = TRUE;
        }
        else
        {
            symbol_p->fixed = FALSE;
            symbol_p->info_p = "command macro";
            mem_pt = memory_allocate( sttst_InitParams.CPUPartition_p, sizeof(macro_t));
            symbol_p->value.mac_val = (macro_t*) mem_pt;
            if (symbol_p->value.mac_val == NULL)
            {
                sttst_Print("Cant allocate memory for macro value\n");
                IsBad = TRUE;
            }
            else
            {
                strcpy(symbol_p->value.mac_val->line, defline_p);
            }
            last_line_p = symbol_p->value.mac_val;
            while (!end && !IsBad)
            {
                mem_pt = memory_allocate(sttst_InitParams.CPUPartition_p, sizeof(macro_t));
                cur_line_p = (macro_t*) mem_pt;
                if (cur_line_p == NULL)
                {
                    sttst_Print("Cant allocate memory for macro line\n");
                    IsBad = TRUE;
                }
                else
                {
                    last_line_p->line_p = cur_line_p;
                    last_line_p = cur_line_p;
                    eof = read_input(cur_line_p->line, "> ");
                    if (eof)
                    {
                        sttst_Print("Unexpected EOF during macro definition %s\n", name_p);
                        IsBad = TRUE;
                    }
                    else
                    {
                        Parse_p = NewParseTableSlot(strlen(cur_line_p->line)+1);
                        init_pars(Parse_p, cur_line_p->line);
                        get_tok(Parse_p, DelimiterSet);
                        if ((is_matched(Parse_p->token, "END", 2)) ||
                            (is_matched(Parse_p->token, "EXIT", 2)))
                        {
                            nests--;
                            if (nests == 0)
                            {
                                end = TRUE;
                            }
                        }
                        else
                        {
                            if (is_control(Parse_p->token, &temp))
                            {
                                nests++;
                            }
                        }
                        FreeParseTableSlot(); /* Parse_p */
                    }
                }
            } /* end while */
            last_line_p->line_p = (macro_t*)NULL;
            if (IsBad)
            {
                sttst_Print("Cannot complete definition of \"%s\"\n", name_p);
                delete_symbol(name_p);
            }
        }
    }
    else
    {
        sttst_Print("Cannot define macro \"%s\" - name clash\n", name_p);
        IsBad = TRUE;
    }
    return(IsBad);
}
/* ========================================================================
   this routine assumes a string is passed which contains only a
   potential command name followed by a set of parameters. If the first
   token on the line is not a valid command, the routine returns TRUE. If
   it is, then the defined action routine for that command is invoked and the
   result of the execution passed back to the caller
=========================================================================== */
static BOOL execute_command_line(char *line_p, const char *const target_p, BOOL *result_p)
{
    symtab_t *command_p;
    STTST_Parse_t  *Parse_p;
    BOOL     IsBad;

/* This is a temporary solution to remove warnings, it will be reviewed */

    const char*  old_target_p = (const char*) target_p;
    void*        var_p  = (void*) &old_target_p;

    Parse_p = NewParseTableSlot(strlen(line_p)+1);

    IsBad = FALSE;
    init_pars(Parse_p, line_p);
    get_tok(Parse_p, DelimiterSet);
    command_p = look_for(Parse_p->token, CLI_COM_SYMBOL);
    if (command_p != (symtab_t*) NULL)
    {
        *result_p = command_p->value.com_val(Parse_p,(char*) (*(char**)var_p));
    }
    else
    {
        IsBad = TRUE;
    }
    FreeParseTableSlot();
    return(IsBad);
}
/* ========================================================================
   this routine assumes a string is passed which contains only a
   potential macro name followed by a set of actual parameters.
   If the first token on the line is not a current macro, the routine returns
   TRUE. If it is, then the body of the macro is executed and the
   result of the execution passed back to the caller. If the macro attempts
   to pass back a value, the expression is evaluated into the target symbol
   name
=========================================================================== */
static BOOL execute_macro(char *line_p, const char *const target_p, BOOL IsAControl, BOOL *result_p)
{
    symtab_t *macro_p;
    STTST_Parse_t  *FormalParse_p, *ActualParse_p;
    BOOL     IsBad = FALSE;
    BOOL     not_done = FALSE;
    char     *rtn_expression;
    S32      int_val;
    double   real_val;
    char     *string_val = NULL;
    S16      type = CLI_NO_SYMBOL;

    ActualParse_p  = NewParseTableSlot(strlen(line_p)+1);

    init_pars(ActualParse_p, line_p);
    get_tok(ActualParse_p, DelimiterSet);
    macro_p = look_for(ActualParse_p->token, CLI_MAC_SYMBOL);
    if (macro_p != (symtab_t*) NULL)
    {
        /* define the formal parameters with values of actuals. look over
        the length of the invocation line and pick up the actual parameters
        determine their type and create an equivalent symbolic variable
        under the name of the formal parameter at the same position. formal
        parameters with no actual values are given default types and values. */

        if (    ((STTST_RunMode & STTST_KEEP_CONTROL_VARIABLE_MODE) == 0 )
             || (((STTST_RunMode & STTST_KEEP_CONTROL_VARIABLE_MODE) != 0 ) && (!IsAControl)))
        {
            MacroDepth ++;
        }
        if (CurrentEcho)
        {
            sttst_Print("Executing macro \"%s\"\n", macro_p->name_p);
        }
        FormalParse_p = NewParseTableSlot(strlen(macro_p->value.mac_val->line)+1);
        init_pars(FormalParse_p, macro_p->value.mac_val->line);
        get_tok(FormalParse_p, DelimiterSet);
        while(( FormalParse_p->tok_len > 0) && !IsBad)
        {
            get_tok(ActualParse_p, DelimiterSet);
            if (ActualParse_p->tok_len > 0)
            {
                not_done = evaluate_assign(FormalParse_p->token, ActualParse_p->token, &IsBad, TRUE);
            }
            else
            {
                not_done = evaluate_assign(FormalParse_p->token, "0", &IsBad, TRUE);
            }
            get_tok(FormalParse_p, DelimiterSet);
        }
        FreeParseTableSlot(); /* FormalParse_p */
        rtn_expression = NewTokenTableSlot(CLI_MAX_LINE_LENGTH);
        rtn_expression[0] = '\0';
        if (IsBad || not_done)
        {
            STTST_TagCurrentLine(ActualParse_p,
                "Failed to assign actual value to formal parameter");
            FreeParseTableSlot(); /* ActualParse_p */
        }
        else
        {
            FreeParseTableSlot();  /* ActualParse_p */

            /* if all this parameterisation worked out OK, then actually execute the
            macro code and return the result. If an an expression was found at the
            end of the macro we must evaluate it into a temporary variable in the
            scope of the macro and then reassign it to the target variable in the
            enclosing scope */

            *result_p = command_loop(macro_p->value.mac_val->line_p,
                                     (FILE*) NULL, rtn_expression, CurrentEcho);
            if (strlen(rtn_expression) != 0)
            {
                *result_p = STTST_EvaluateInteger(rtn_expression, &int_val, NumberBase);
                if (!*result_p)
                {
                    type = CLI_INT_SYMBOL;
                }
                else
                {
                    *result_p = STTST_EvaluateFloat(rtn_expression, &real_val);
                    if (!*result_p)
                    {
                        type = CLI_FLT_SYMBOL;
                    }
                    else
                    {
                        string_val = NewTokenTableSlot(CLI_MAX_LINE_LENGTH);
                        *result_p = STTST_EvaluateString(rtn_expression, string_val, CLI_MAX_LINE_LENGTH);
                        if (!*result_p)
                        {
                            type = CLI_STR_SYMBOL;
                        }
                        else
                        {
                            type = CLI_NO_SYMBOL;
                        }
                        FreeTokenTableSlot(); /* string_val */
                    }
                }
            }
        }
        if (    ((STTST_RunMode & STTST_KEEP_CONTROL_VARIABLE_MODE) == 0 )
             || ( ((STTST_RunMode & STTST_KEEP_CONTROL_VARIABLE_MODE) != 0 ) && (!IsAControl)))
        {
            MacroDepth --;
            purge_symbols(MacroDepth);
        }
        /* now we are back within original scope, try to evaluate and assign
        return expression, using temporary variable to give value*/
        if ((strlen(rtn_expression) != 0) && (type != CLI_NO_SYMBOL))
        {
            switch(type)
            {
                case CLI_INT_SYMBOL:
                    *result_p = STTST_AssignInteger(target_p, int_val, FALSE);
                    break;
                case CLI_FLT_SYMBOL:
                    *result_p = STTST_AssignFloat(target_p, real_val, FALSE);
                    break;
                case CLI_STR_SYMBOL:
                    *result_p = STTST_AssignString(target_p, string_val, FALSE);
                    break;
            }
        }
        FreeTokenTableSlot(); /* rtn_expression */
    }
    else
    {
        IsBad = TRUE;
        FreeParseTableSlot(); /* ActualParse_p */
    }
    return(IsBad);
}
/* ========================================================================
   performs the actions of displaying constants and symbols. If a
   line of parameters is present, each is interpreted and displayed. Other
   the whole current list of symbols is displayed.
=========================================================================== */
static BOOL do_show(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL     IsBad = FALSE;
    symtab_t *symbol_p;
    S16      cnt = 0;
    macro_t  *line_p;
    UNUSED_PARAMETER(result_sym_p);

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {   /* no items on a line */
        sttst_Print("Currently defined symbols:\n\n");
        /* print symbol table info*/
        while(cnt < SymbolCount)
        {
            symbol_p = Symtab_p[cnt++];
            switch(symbol_p->type)
            {
                case CLI_INT_SYMBOL:
                    sttst_Print( "%2d: %s: ",  symbol_p->depth, symbol_p->name_p);
                    sttst_Print( "%ld - %s\n", symbol_p->value.int_val, symbol_p->info_p);
                    break;
                case CLI_FLT_SYMBOL:
                    sttst_Print("%2d: %s: ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("%G - %s\n", symbol_p->value.flt_val, symbol_p->info_p);
                    break;
                case CLI_STR_SYMBOL:
                    sttst_Print("%2d: %s: ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("\"%s\" - %s\n", symbol_p->value.str_val, symbol_p->info_p);
                    break;
                case CLI_MAC_SYMBOL:
                    sttst_Print("%2d: %s: ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("(%s) - %s\n", symbol_p->value.mac_val->line, symbol_p->info_p);
                    break;
            }
        }
    }
    else
    {
        symbol_p = look_for(Parse_p->token, CLI_ANY_SYMBOL);
        if (symbol_p == NULL)
        {
            STTST_TagCurrentLine(Parse_p, "unrecognised symbol name");
        }
        else
        {
            switch(symbol_p->type)
            {
                case CLI_INT_SYMBOL:
                    sttst_Print( "%d: %s: ", symbol_p->depth, symbol_p->name_p);
                    /*sttst_Print("%ld (#%x) - %s\n", symbol_p->value.int_val, */
                    sttst_Print("%ld (h%x) - %s\n", symbol_p->value.int_val,
                        (int)symbol_p->value.int_val, symbol_p->info_p);
                    break;
                case CLI_FLT_SYMBOL:
                    sttst_Print("%d: %s: ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("%G - %s\n", symbol_p->value.flt_val, symbol_p->info_p);
                    break;
                case CLI_STR_SYMBOL:
                    sttst_Print("%d: %s: ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("\"%s\" - %s\n", symbol_p->value.str_val, symbol_p->info_p);
                    break;
                case CLI_COM_SYMBOL:
                    sttst_Print("%d: %s: ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("is a command\n");
                    break;
                case CLI_MAC_SYMBOL:
                    sttst_Print("%d: %s ", symbol_p->depth, symbol_p->name_p);
                    sttst_Print("(%s) - %s\n", symbol_p->value.mac_val->line, symbol_p->info_p);
                    /* print macro contents */
                    line_p = symbol_p->value.mac_val->line_p;
                    while(line_p != NULL)
                    {
                        /* sttst_Print(">   %s\n", line_p->line); (ddts 03934 - Aug. 2000) */
                        sttst_Print("   %s\n", line_p->line);
                        line_p = line_p->line_p;
                    }
                    break;
                default:
                    sttst_Print("%d: %s has been deleted\n", symbol_p->depth, symbol_p->name_p);
                    break;
            }
        }
    }
    return(IsBad);
}
/* ========================================================================
   perform a fomatting action on a list of variables or values by
   invoking the sprintf routine
=========================================================================== */
static BOOL do_print(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    S16    cnt;
    S32    int_val;
    int    i;
    double real_val;
    char   *str_val, *StringBuffer;

    str_val = NewTokenTableSlot(CLI_MAX_LINE_LENGTH);
    StringBuffer = NewStringTableSlot(CLI_MAX_LINE_LENGTH);

    StringBuffer[0] = '\0';
    cnt = 0;
    while ((Parse_p->tok_del != '\0') && (strlen(StringBuffer) < CLI_MAX_LINE_LENGTH))
    {
        get_tok(Parse_p, DelimiterSet);
        IsBad = STTST_EvaluateInteger(Parse_p->token, &int_val, NumberBase);
        if (!IsBad)
        {
            switch (NumberBase)
            {
                case  2:
                    {
                        str_val[0]='b';
                        str_val[33] = '\0';
                        for (i=31; i>=0; i--)
                        {
                            if ( ((int_val>>i) & 0x1) > 0)
                                str_val[32-i] = '1';
                            else
                                str_val[32-i] = '0';
                        }
                    }
                    break;
                case  8:
                    sprintf(str_val, "O%o", int_val);
                    break;
                case 10:
                    sprintf(str_val, "%d", int_val);
                    break;
                case 16:
                    /*sprintf(str_val, "#%x", int_val); */
                    sprintf(str_val, "h%x", int_val);
                    break;
                default:
                    IsBad=TRUE;
                    sprintf(str_val,"Invalid number base");
                    break;
            }
            strcat(StringBuffer, str_val);
        }
        else
        {
            IsBad = STTST_EvaluateFloat(Parse_p->token, &real_val);
            if (!IsBad)
            {
                sprintf(str_val, "%G", real_val);
                strcat(StringBuffer, str_val);
            }
            else
            {
                IsBad = STTST_EvaluateString(Parse_p->token, str_val, CLI_MAX_LINE_LENGTH);
                if (!IsBad)
                {
                    strcat(StringBuffer, str_val);
                }
                else
                {
                    if (Parse_p->tok_len > 0)
                    {
                        strcat(StringBuffer, "******");
                    }
                    else
                    {
                        IsBad = FALSE;
                    }
                }
            }
        }
        cnt++;
    }
    sttst_Print("%s\n", StringBuffer);
    IsBad = IsBad || STTST_AssignString(result_sym_p, StringBuffer, FALSE);
    FreeStringTableSlot();
    FreeTokenTableSlot();
    return(IsBad);
}
/* ========================================================================
   Allows display of help information attached to each command or macro
   currently defined. If a parameter is present, the display is limited to
   help for that command. Otherwise a list is printed.
=========================================================================== */
static BOOL do_help(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    symtab_t  *symbol_p;
    BOOL   match;
    S16     cnt2;
    S16     cnt = 0;
    char *pt;

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* no items on a line */
        sttst_Print("The following commands and macros are defined:\n\n");
        while(cnt < SymbolCount)
        {
            symbol_p = Symtab_p[cnt++];
            switch(symbol_p->type)
            {
                case CLI_COM_SYMBOL:
                    sttst_Print("%-12s - %s\n",
                        symbol_p->name_p, symbol_p->info_p);
                    break;
                case CLI_MAC_SYMBOL:
                    sttst_Print("%s ", symbol_p->name_p);
                    sttst_Print("(%s) - %s\n",
                        symbol_p->value.mac_val->line, symbol_p->info_p);
                    break;
                default:
                    break;
            }
        }
        sttst_Print("\n");
        sttst_Print("Bases       = binary (bxx), octal (oxx), decimal (xx), hexadecimal (hxx)\n");
        sttst_Print("              print b10+o10+10+h10 is equivalent to print 2+8+10+16 \n");
        sttst_Print("Operators   = + - * / % (arithmetical)\n");
        sttst_Print("              & ^ | ~ ! (logical)\n");
        sttst_Print("Return code = each command returns FALSE if successful or TRUE if failure\n");
        sttst_Print("              some of them also return a data, like s=getstring\n");
        sttst_Print("Commands    = command recall from history is allowed with control keys\n");
        sttst_Print("              ctrl-u=move cursor left ctrl-p=move cursor right\n");
        sttst_Print("              ctrl-i=previous command ctrl-o=next command\n");
        sttst_Print("Variables   = integer, float, string\n\n"); /* FQ Dec 99 */
        sttst_Print("Command and macros can be used as statements or part of expressions,\n");
        sttst_Print("if appropriate, where the assignment is made to a symbol whose value\n");
        sttst_Print("is either defined or modified by this assignment. Generalised assignments\n");
        sttst_Print("can be made to symbols of integer, floating point or string types,\n");
        sttst_Print("including evaluation of arbitrary arithmetic expressions.\n\n");
        sttst_Print("Commands and assignments can be combined into parameterised macros\n");
        sttst_Print("that use FOR, IF, ELSE and WHILE constructs. Using the DEFINE\n");
        sttst_Print("command these macros can be set up for later invocation. All blocks\n");
        sttst_Print("within these macro constructs are terminated using the END statement.\n\n");
    }
    else
    {   /* help X*  (FQ Jan 99) */
        /*{{{  display individual information*/
        pt = strchr( Parse_p->token, '*');
        if ( pt != NULL )
        {
            cnt = 0;
            while ( cnt < SymbolCount )
            {
                symbol_p = Symtab_p[cnt];
                match = TRUE;
                cnt2 = 0;
                while ( ( cnt2<(pt-Parse_p->token) ) && (match) )
                {
                    if ( (symbol_p->name_p[cnt2] & 0xdf) != (Parse_p->token[cnt2] & 0xdf) )
                    {
                        match = FALSE;
                    }
                    cnt2++;
                }
                if (match)
                {
                    /*if ( strncmp( symbol_p->name_p, Parse_p->token, (pt-Parse_p->token) )==0 ) {*/
                    switch(symbol_p->type)
                    {
                        case CLI_COM_SYMBOL:
                            sttst_Print("%-12s - %s\n",
                                symbol_p->name_p, symbol_p->info_p);
                            break;
                        case CLI_MAC_SYMBOL:
                            sttst_Print("%s ", symbol_p->name_p);
                            sttst_Print("(%s) - %s\n",
                                symbol_p->value.mac_val->line, symbol_p->info_p);
                            break;
                        default:
                            break;
                    }
                }
                cnt++;
            } /* end while */
        }
        else
        {   /* help XXX */
            symbol_p = look_for(Parse_p->token, (CLI_MAC_SYMBOL | CLI_COM_SYMBOL));
            if (symbol_p != (symtab_t*)NULL)
            {
                switch(symbol_p->type)
                {
                    case CLI_COM_SYMBOL:
                        sttst_Print("%-12s - %s\n",
                            symbol_p->name_p, symbol_p->info_p);
                        break;
                    case CLI_MAC_SYMBOL:
                        sttst_Print("%s ", symbol_p->name_p);
                        sttst_Print("(%s) - %s\n", symbol_p->value.mac_val->line, symbol_p->info_p);
                        break;
                    default:
                        break;
                } /* end of switch */
            }  /* end of if found one */
            else
            {
                STTST_TagCurrentLine(Parse_p, "unrecognised command or macro");
                IsBad = TRUE;
            }
        } /* end of help XXX */
    } /* end help X* */
    return(IsBad || STTST_AssignString(result_sym_p, "", FALSE));
}
/* ========================================================================
   alternate version of help : lists all the function and macros that
   are matching
=========================================================================== */
/***BOOL do_list(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    symtab_t  *symbol_p;
    S16     cnt = 0;

    if (get_tok(Parse_p, DelimiterSet) != 0)
    {
        sttst_Print("The following commands and macros are defined:\n");
        while(cnt < SymbolCount)
        {
            symbol_p = Symtab_p[cnt++];
            if (symbol_p->name_p != NULL)
            {
                if (  is_matched(Parse_p->token, symbol_p->name_p, 1) &&
                    ( (symbol_p->type | (CLI_MAC_SYMBOL|CLI_COM_SYMBOL) ) !=  0 ) )
                {
                    switch(symbol_p->type)
                    {
                        case CLI_COM_SYMBOL:
                            sttst_Print("%-12s - %s\n", symbol_p->name_p, symbol_p->info_p);
                            break;
                        case CLI_MAC_SYMBOL:
                            sttst_Print("%s ", symbol_p->name_p);
                            sttst_Print("(%s) - %s\n", symbol_p->value.mac_val->line, symbol_p->info_p);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    return(IsBad || STTST_AssignString(result_sym_p, "", FALSE));
}***/

/* ========================================================================
   performs the action of deleting symbols and macros. If
   line of parameters is present, each is interpreted
   and, if possible, deleted.
=========================================================================== */
static BOOL do_delete(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    symtab_t  *symbol_p;
    char *pt;
    S32 cnt, cnt_matches;

    cnt = 0;
    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* no items on a line */
        STTST_TagCurrentLine(Parse_p, "Expected symbol or macro name");
        IsBad = TRUE;
    }
    else
    {   /* DELETE X* (FQ Jan 99) */
        /* fix GNBvd07652 delete X* overrun Symtab_p (CL Aug 2001) */
        pt = strchr( Parse_p->token, '*');
        if ( pt != NULL && pt > Parse_p->token )
        {
            cnt = 1;
            cnt_matches = 0;
            while ( cnt <= SymbolCount )
            {
                symbol_p = Symtab_p[SymbolCount - cnt];
                if (symbol_p->name_p != NULL) /* not yet deleted symbol */
                {
                    if ( strncmp( symbol_p->name_p, Parse_p->token, (pt-Parse_p->token) )==0 )
                    {
                        sttst_Print("[%s] ", symbol_p->name_p);
                        IsBad = delete_symbol(symbol_p->name_p);
                        if (IsBad)
                        {
                            sttst_Print("Cannot delete symbol\n");
                        }
                        else
                        {
                            sttst_Print("deleted\n");
                            /* Symtab_p has been defragmented by delete_symbol, so re-start scanning */
                            cnt = 0;
                        }
                        cnt_matches++;
                    }
                }
                cnt++;
            } /* end while */
            if (cnt_matches == 0)
            {
                sttst_Print("nothing to delete\n");
            }
        } /* end if pt not null */
        else
        {   /* DELETE XXX */
            while ((Parse_p->tok_len > 0) && !IsBad)
            {
                symbol_p = look_for(Parse_p->token, CLI_ANY_SYMBOL);
                if (symbol_p == (symtab_t*)NULL)
                {
                    STTST_TagCurrentLine(Parse_p, "Unrecognised symbol");
                    IsBad = TRUE;
                }
                else if (symbol_p->fixed || (symbol_p->type == CLI_COM_SYMBOL))
                {
                    STTST_TagCurrentLine(Parse_p, "Cannot delete fixed symbol or command");
                    IsBad = TRUE;
                }
                else
                {
                    IsBad = delete_symbol(Parse_p->token);
                    if (IsBad)
                    {
                        STTST_TagCurrentLine(Parse_p, "Cannot delete symbol out of current scope");
                    }
                    get_tok(Parse_p, DelimiterSet);
                }
            }
        }
    } /* end else */
    return(IsBad || STTST_AssignString(result_sym_p, "", FALSE));
}
/* ========================================================================
   toggles the echo of input lines
=========================================================================== */
static BOOL do_verify(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad;
    S32      echo;

    IsBad = STTST_GetInteger(Parse_p, CurrentEcho, &echo);
    if (IsBad)
    {
        STTST_TagCurrentLine(Parse_p, "expected command echo flag");
    }
    else
    {
        CurrentEcho = (BOOL) echo;
        if (CurrentEcho)
        {
            sttst_Print("Command echo is enabled\n");
        }
        else
        {
            sttst_Print("Command echo is disabled\n");
        }
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, echo, FALSE));
}

#ifdef IO_RADIX_CONF
/* ========================================================================
   performs the action of defining the default number base
=========================================================================== */
static BOOL do_base(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad;
    S32      base;
    S32      old_base;

    old_base = NumberBase;
    IsBad = STTST_GetInteger(Parse_p, NumberBase, &base);
    if (IsBad ||
        ((base != 16) && (base != 10) && (base != 8) && (base != 2)))
    {
        STTST_TagCurrentLine(Parse_p, "Illegal number base");
        IsBad = TRUE;
    }
    else
    {
        NumberBase = (S16) base;
        sttst_Print("Number base = %d\n", NumberBase);
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, old_base, FALSE));
}
#endif /* #ifdef IO_RADIX_CONF */

/* ========================================================================
   wait & get one key on the input device
=========================================================================== */
static BOOL do_getkey(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    char answer[8];
    char *val;
    UNUSED_PARAMETER(Parse_p);

    memset( answer, 0, sizeof(answer));
    STTBX_InputChar( answer );
    val = answer;
    /*sprintf(mm,"#%02X ",answer[0]);
    STTBX_Print((mm));
    if (answer[0]==0x00 ||answer[0]==0x1B)
    {
        STTBX_InputChar( answer );
        val = (long *)answer;
        sprintf(mm,"#%02X ",answer[0]);
        STTBX_Print((mm));
    }*/
    STTST_AssignInteger(result_sym_p, *val, FALSE);
    return(FALSE);
}
/* ========================================================================
   wait & get a string on the input device
=========================================================================== */
static BOOL do_getstring(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    char *answer;
    int nbbytes, size;
    UNUSED_PARAMETER(Parse_p);

    size = CLI_MAX_LINE_LENGTH;
    answer = NewTokenTableSlot(size);

    memset( answer, 0, size);

    nbbytes = STTBX_InputStr( answer, size );

/* NS# under linux, local echo is already enabled on stdin, no need to print */
#ifndef ST_OSLINUX
    printf("%s\n",answer);
#endif
    STTST_AssignString(result_sym_p, answer, FALSE);
    FreeTokenTableSlot();
    return(FALSE);
}
/* ========================================================================
   get key hit (without waiting) on the input device
=========================================================================== */
static BOOL do_pollkey(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    char answer[8];
    BOOL hit;
    char *val;
    UNUSED_PARAMETER(Parse_p);

    memset( answer, 0, sizeof(answer));
    hit = STTBX_InputPollChar(answer);
    val = answer;
    STTST_AssignInteger(result_sym_p, *val, FALSE);
    /*if (hit)
    {
        return(TRUE);
    }*/
    return(FALSE);
}
/* ========================================================================
   display the history command of the last commands entered ;
   if a number is given on input line, executes the corresponding command.
=========================================================================== */
static BOOL do_history (STTST_Parse_t *Parse_p, char *result_sym_p)
{
    int cmd_cnt;
    S32 cmd_choice;
    BOOL IsBad;
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(result_sym_p);

    IsBad = STTST_GetInteger(Parse_p, -1, &cmd_choice);
    if (IsBad)
    {
        STTST_TagCurrentLine(Parse_p, "Illegal command number");
        IsBad = TRUE;
    }
    if ( cmd_choice == -1)
    {
        sttst_Print("History of last commands \n");
        cmd_cnt = CommandHistory.old_cmd ;
        while ( cmd_cnt <= CommandHistory.last_cmd )
        {
            sttst_Print("%2d   -- %s\n", cmd_cnt, CommandHistory.cmd_name[cmd_cnt] );
            cmd_cnt++;
        }
    }
    else
    {
        if ( cmd_choice >= 0 && cmd_choice < STTST_MAX_TOK_LEN )
        {
            sttst_Print("%s\n", CommandHistory.cmd_name[cmd_choice]);
            execute_command_line(CommandHistory.cmd_name[cmd_choice], "", &IsBad);
        }
    }
    return ( FALSE );
}

#ifndef UNHOSTED
/* ========================================================================
   stop the logging of input and output to a journal file
=========================================================================== */
static BOOL do_closelog(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    UNUSED_PARAMETER(Parse_p);
    UNUSED_PARAMETER(result_sym_p);

    /* close existing file, if any */
    if (LogFile != NULL)
    {
        LogOutput = FALSE;
        LogResults = FALSE;
        fclose(LogFile);
        sttst_Print("Logging stopped and log file closed\n");
        LogFile = NULL;
    }
    return(FALSE);
}

/* ========================================================================
   controls the logging of input and output to a journal file
=========================================================================== */
static BOOL do_log(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    char      filename[FILENAME_MAX];
    char      def_filename[FILENAME_MAX];
    char      options[40];
    S32      flag;
    char      append_mode[2];

    /* default parameters */
    append_mode[0] = 'w';
    append_mode[1] = '\0';
    LogResults = FALSE;
    LogOutput = FALSE;
    flag = FALSE;
    STTST_EvaluateString("LOG_FILE", def_filename, FILENAME_MAX);
    memset(options, 0, sizeof(options));

    /* get filename */
    IsBad = STTST_GetString(Parse_p, def_filename, filename, FILENAME_MAX);
    if ( IsBad || strlen(filename) == 0 )
    {
        STTST_TagCurrentLine(Parse_p, "Illegal log filename");
    }
    else
    {
        /* get options : access mode & log filtering (FQ) ; quote+2 char.+quote+null=5 bytes */
        IsBad = STTST_GetString(Parse_p, "W", options, 5);
        if ( IsBad )
        {
            /* get flag : 0=input, 1=input+output (for compatibility with previous release) */
            IsBad = STTST_EvaluateInteger(Parse_p->token, &flag, NumberBase);
            if ( !IsBad && flag>0 )
            {
                LogOutput = TRUE;
            }
        }
        else
        {
            if ( strlen(options)>2 )
            {
                STTST_TagCurrentLine(Parse_p, "Illegal log options (select 1 or 2 letters in \"WAIOR\")");
                IsBad = TRUE;
            }
            if ( options[0]=='a' || options[0]=='A' || options[1]=='a' || options[1]=='A' )
            {
                append_mode[0] = 'a';
            }
            if ( options[0]=='r' || options[0]=='R' || options[1]=='r' || options[1]=='R' )
            {
                LogResults = TRUE;
                LogOutput = TRUE;
            }
            if ( options[0]=='o' || options[0]=='O' || options[1]=='o' || options[1]=='O' )
            {
                LogOutput = TRUE;
            }
            if ( options[0]=='1' || options[0]=='1' )
            {
                LogOutput = TRUE;
            }
        }
        /* close existing file, if any */
        do_closelog(NULL, NULL);
        if ( !IsBad )
        {
            LogFile = fopen(filename, append_mode);
            if (LogFile != NULL )
            {
                /* look for log output option */
                /*STTST_GetInteger(Parse_p, 0, &flag);*/
                /*LogOutput = (BOOL) flag;*/
                if (LogOutput)
                {
                    if (!LogResults)
                    {
                        sttst_Print("Logging input and output to file %s (mode=%c)\n", filename, append_mode[0]);
                    }
                    else
                    {
                        sttst_Print("Logging output results to file %s (mode=%c)\n", filename, append_mode[0]);
                    }
                }
                else
                {
                    sttst_Print("Logging input to file %s (mode=%c)\n", filename, append_mode[0]);
                }
            }
            else
            {
                sttst_Print("Cannot open file \"%s\" for output\n", filename);
            }
            STTST_AssignString(result_sym_p, filename, FALSE);
        }
    }
    return(IsBad);
}

/* ========================================================================
   performs the execution of a command input file
=========================================================================== */
static BOOL do_source(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    FILE      *inputfd;
    char      filename[FILENAME_MAX];
    char      def_filename[FILENAME_MAX];
    S32      flag;

    STTST_EvaluateString("COMMAND_FILE", def_filename, FILENAME_MAX);
    if (STTST_GetString(Parse_p, def_filename, filename, FILENAME_MAX) ||
        (strlen(filename) == 0))
    {
        STTST_TagCurrentLine(Parse_p, "Illegal input filename");
        IsBad = TRUE;
    }
    else
    {
        inputfd = fopen(filename, "r");
        if (inputfd != NULL)
        {
            sttst_Print("input from file \"%s\"\n",filename);
            /* look for echo option */
            STTST_GetInteger(Parse_p, 0, &flag);
            command_loop((macro_t*)NULL, inputfd, (char*)NULL,(BOOL) flag);
            fclose(inputfd);
        }
        else
        {
            sttst_Print("Cannot open file \"%s\" for input\n", filename);
        }
    }
    return(IsBad || STTST_AssignString(result_sym_p, filename, FALSE));
}

/* ========================================================================
   Performs the execution of the content of a string
=========================================================================== */
static BOOL do_eval(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad    = FALSE;
    macro_t EvalLine;
    macro_t EndLine;

    if (!STTST_GetString(Parse_p, "", EvalLine.line, CLI_MAX_LINE_LENGTH))
    {
        /* Create a line with END as a string */
        EndLine.line_p = NULL;
        strcpy(EndLine.line, "END");
        /* Link it to the line to evaluate */
        EvalLine.line_p = &EndLine;
        /* Execute the new macro */
        IsBad = command_loop(&EvalLine,
                             (FILE*) NULL, NULL, FALSE);
    }

    if(IsBad)
    {
        STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    }
    else
    {
        STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    }

    return(FALSE);
}

/* ========================================================================
   Removes named commands from named file
=========================================================================== */
static BOOL do_unsource(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    FILE      *inputfd;
    char      filename[FILENAME_MAX];
    char      def_filename[FILENAME_MAX];
    S32      flag;

    STTST_EvaluateString("COMMAND_FILE", def_filename, FILENAME_MAX);
    if (STTST_GetString(Parse_p, def_filename, filename, FILENAME_MAX) ||
        (strlen(filename) == 0))
    {
        STTST_TagCurrentLine(Parse_p, "Illegal input filename");
        IsBad = TRUE;
    }
    else
    {
        inputfd = fopen(filename, "r");
        if (inputfd != NULL)
        {
            sttst_Print("Deleting Symbols from file \"%s\"\n",filename);
            /* look for echo option */
            STTST_GetInteger(Parse_p, 0, &flag);
            uncommand_loop((macro_t*)NULL, inputfd, (char*)NULL,(BOOL) flag);
            fclose(inputfd);
        }
        else
        {
            sttst_Print("Cannot open file \"%s\" for input\n", filename);
        }
    }
    return(IsBad || STTST_AssignString(result_sym_p, filename, FALSE));
}

/* ========================================================================
   Allows user to save state of play, in terms of macros and variables
   by placing definitions in a file such that they can be 'sourced'
   during a future run of the tool
=========================================================================== */
static BOOL do_save(STTST_Parse_t *Parse_p, char *result_sym_p)
{
    BOOL   IsBad = FALSE;
    FILE      *outputfd;
    char      filename[FILENAME_MAX];
    char      def_filename[FILENAME_MAX];
    S32      flag;
    symtab_t  *symbol_p;
    S16     cnt = 0;
    macro_t   *line_p;
    UNUSED_PARAMETER(result_sym_p);

    STTST_EvaluateString("COMMAND_FILE", def_filename, FILENAME_MAX);
    if (STTST_GetString(Parse_p, def_filename, filename, FILENAME_MAX) ||
        (strlen(filename) == 0))
    {
        STTST_TagCurrentLine(Parse_p, "Illegal output filename");
        IsBad = TRUE;
    }
    else
    {
        outputfd = fopen(filename, "w");
        if (outputfd == NULL )
        {
            sttst_Print("Cannot open file \"%s\" for output\n", filename);
            IsBad = TRUE;
        }
        else
        {
            sttst_Print("saving to file \"%s\"\n",filename);
            /* look for constants option */
            STTST_GetInteger(Parse_p, 0, &flag);

            /* look for appropriate symbols to save. flag indicates we should save
            constants as well as variables */
            while(cnt < SymbolCount)
            {
                symbol_p = Symtab_p[cnt++];
                switch(symbol_p->type)
                {
                    case CLI_INT_SYMBOL:
                        if (!(symbol_p->fixed) || flag)
                        {
                            if (symbol_p->value.int_val < 0)
                              sprintf(DisplayBuffer_p,"%s = %d\n",
                                symbol_p->name_p, symbol_p->value.int_val);
                            else
                              sprintf(DisplayBuffer_p,"%s = +%d\n",
                                symbol_p->name_p, symbol_p->value.int_val);
                            fprintf(outputfd, DisplayBuffer_p);
                        }
                        break;
                    case CLI_FLT_SYMBOL:
                        if (!(symbol_p->fixed) || flag)
                        {
                            sprintf(DisplayBuffer_p,"%s = %E\n",
                              symbol_p->name_p,symbol_p->value.flt_val);
                            fprintf(outputfd, DisplayBuffer_p);
                        }
                        break;
                    case CLI_STR_SYMBOL:
                        if (!(symbol_p->fixed) || flag)
                        {
                            sprintf(DisplayBuffer_p,"%s = \"%s\"\n",
                              symbol_p->name_p,symbol_p->value.str_val);
                            fprintf(outputfd, DisplayBuffer_p);
                        }
                        break;
                    case CLI_MAC_SYMBOL:
                        sprintf(DisplayBuffer_p,"DEFINE %s %s\n",
                            symbol_p->name_p, symbol_p->value.mac_val->line);
                        fprintf(outputfd, DisplayBuffer_p);
                        /* print macro contents */
                        line_p = symbol_p->value.mac_val->line_p;
                        while(line_p != NULL)
                        {
                            sprintf(DisplayBuffer_p,"  %s\n", line_p->line);
                            fprintf(outputfd, DisplayBuffer_p);
                            line_p = line_p->line_p;
                        }
                        break;
                    default:
                        break;
                }
            }
            fclose(outputfd);
        }
    }
    return(IsBad);
}
#endif

/* ========================================================================
   allows definition of command macros as a control command in itself
=========================================================================== */
static BOOL do_define(STTST_Parse_t *Parse_p)
{
    BOOL IsBad = FALSE;
    char *name;

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* attempt to find macro name */
        STTST_TagCurrentLine(Parse_p, "macro name expected");
        IsBad = TRUE;
    }
    else
    {
        name = NewTokenTableSlot(STTST_MAX_TOK_LEN);
        strcpy(name, Parse_p->token);
        get_tok(Parse_p, ""); /* get the rest of the line */
        IsBad = define_macro(Parse_p->token, name);
        FreeTokenTableSlot(); /* name */
    }
    return(IsBad);
}
/* ========================================================================
   allows definition of flexible command repetition loop
=========================================================================== */
static BOOL do_for(STTST_Parse_t *Parse_p)
{
    BOOL   IsBad = FALSE;
    S32      first = 0;
    S32      second = 0;
    S32      step;
    S32      i;
    char     *variable;
    char     name[CONTROL_NAME_LENGTH];

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* attempt to find parameters */
        STTST_TagCurrentLine(Parse_p, "for loop variable name expected");
        IsBad = TRUE;
    }
    else
    {
        variable = NewTokenTableSlot(MAX_NUMBER_LENGTH);
        strcpy(variable, Parse_p->token);
        IsBad = STTST_AssignInteger(variable, 0, FALSE);
        if (IsBad)
        {
            STTST_TagCurrentLine(Parse_p, "expected integer loop variable name");
        }
        else
        {
            IsBad = STTST_GetInteger(Parse_p, 1, &first);
            if (IsBad)
            {
                STTST_TagCurrentLine(Parse_p, "invalid loop start value");
            }
            else
            {
                IsBad = STTST_GetInteger(Parse_p, 1, &second);
                if (IsBad)
                    STTST_TagCurrentLine(Parse_p, "invalid loop end value");
                else
                {
                    IsBad = (STTST_GetInteger(Parse_p, 1, &step) ||
                           ((first > second) && (step > 0)) ||
                           ((first < second) && (step < 0)) ||
                           (step == 0));
                    if (IsBad)
                        STTST_TagCurrentLine(Parse_p, "invalid or inconsistent loop step value");
                    else
                    {
                        sprintf(name, "FOR%d", ForNameCount ++);
                        IsBad = define_macro("", name);
                        if (!IsBad)
                        {
                            if (first <= second)
                            {
                                for(i=first; (i<=second) && !IsBad; i=i+step)
                                {
                                    STTST_AssignInteger(variable, i, FALSE);
                                    execute_macro(name,"", TRUE, &IsBad);
                                }
                            }
                            else
                            {
                                for(i=first; (i>=second) && !IsBad; i=i+step) /* step is <0 here */
                                {
                                    STTST_AssignInteger(variable, i, FALSE);
                                    execute_macro(name,"", TRUE, &IsBad);
                                }
                            }
                            delete_symbol(name);
                        }
                        ForNameCount --;
                    }
                }
            }
            delete_symbol(variable);
        }
        FreeTokenTableSlot(); /* variable */
    }
    return(IsBad);
}
/* ========================================================================
   allows definition of conditional command repetition loop
=========================================================================== */
static BOOL do_while(STTST_Parse_t *Parse_p)
{
    BOOL   IsBad = FALSE;
    BOOL   result = FALSE;
    char   name[CONTROL_NAME_LENGTH];
    char   *condition;

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* attempt to find parameters */
        STTST_TagCurrentLine(Parse_p, "comparison expression expected");
        IsBad = TRUE;
    }
    else
    {
        IsBad = STTST_EvaluateComparison(Parse_p->token, &result, NumberBase);
        if (IsBad)
        {
            STTST_TagCurrentLine(Parse_p, "illegal comparison");
        }
        else
        {
            condition = NewTokenTableSlot(MAX_NUMBER_LENGTH);
            sprintf(name, "WHILE%d", WhileNameCount ++);
            strcpy(condition, Parse_p->token);
            IsBad = define_macro("", name);
            if (!IsBad)
            {
                while (result && !IsBad)
                {
                    execute_macro(name,"", TRUE, &IsBad);
                    STTST_EvaluateComparison(condition, &result, NumberBase);
                }
                delete_symbol(name);
            }
            WhileNameCount --;
            FreeTokenTableSlot(); /* condition */
        }
    }
    return(IsBad);
}
/* ========================================================================
   allows definition of conditional command execution, passing the result
   of the condition evaluation back to a caller for use by the 'else' clause
=========================================================================== */
static BOOL do_if(STTST_Parse_t *Parse_p, BOOL *if_taken_p)
{
    BOOL   IsBad = FALSE;
    char   name[CONTROL_NAME_LENGTH];

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* attempt to find parameters */
        STTST_TagCurrentLine(Parse_p, "comparison expression expected");
        IsBad = TRUE;
    }
    else
    {
        IsBad = STTST_EvaluateComparison(Parse_p->token, if_taken_p, NumberBase);
        if (IsBad)
        {
            STTST_TagCurrentLine(Parse_p, "illegal comparison");
        }
        else
        {
            sprintf(name, "IF%d", IfNameCount ++);
            IsBad = define_macro("", name);
            if (!IsBad)
            {
                if (*if_taken_p)
                {
                    execute_macro(name,"", TRUE, &IsBad);
                }
                delete_symbol(name);
            }
            IfNameCount --;
        }
    }
    return(IsBad);
}

/* ========================================================================
   allows definition of conditional command execution, passing the result
   of the condition evaluation back to a caller for use by the 'else' clause
=========================================================================== */
static BOOL do_elif(STTST_Parse_t *Parse_p, BOOL *elif_taken_p, BOOL if_taken)
{
    BOOL   IsBad = FALSE;
    char   name[CONTROL_NAME_LENGTH];

    if (get_tok(Parse_p, DelimiterSet) == 0)
    {  /* attempt to find parameters */
        STTST_TagCurrentLine(Parse_p, "comparison expression expected");
        IsBad = TRUE;
    }
    else
    {
        IsBad = STTST_EvaluateComparison(Parse_p->token, elif_taken_p, NumberBase);
        if (IsBad)
        {
            STTST_TagCurrentLine(Parse_p, "illegal comparison");
        }
        else
        {
            sprintf(name, "ELIF%d", ElifNameCount ++);
            IsBad = define_macro("", name);
            if (!IsBad)
            {
                if ( (*elif_taken_p) && (!if_taken))
                {
                    execute_macro(name,"", TRUE, &IsBad);
                }
                delete_symbol(name);
            }
            ElifNameCount --;
        }
    }
    return(IsBad);
}

/* ========================================================================
   allows definition of conditional command execution after
   evaluation of the 'if condition
=========================================================================== */
static BOOL do_else(BOOL if_taken)
{
    BOOL   IsBad = FALSE;
    char   name[CONTROL_NAME_LENGTH];

    sprintf(name, "ELSE%d", ElseNameCount ++);
    IsBad = define_macro("", name);
    if (!IsBad)
    {
        if (!if_taken)
        {
            execute_macro(name,"", TRUE, &IsBad);
        }
        delete_symbol(name);
    }
    ElseNameCount --;
    return(IsBad);
}

/*-------------------------------------------------------------------------
 * Function : do_profiler_init
 *            Init Profiler
 *            Profiler can be re-initialized by calling this function again.
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE if ok, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL do_profiler_init(STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined ST_OS21 && !defined ST_OSWINCE
    int result;
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    result = profile_init(INSTRUCTIONS_PER_BUCKET, SAMPLING_FREQ_IN_HERTZ);

    if (result != OS21_SUCCESS)
    {
        sttst_Print ("Error during profile_init\n");
        return (TRUE);
    }

    is_profiler_initialized = TRUE;

    sttst_Print ("Profiler initialized:  instructions_per_bucket = %d,  sampling_freq = %d Hz\n", INSTRUCTIONS_PER_BUCKET, SAMPLING_FREQ_IN_HERTZ );

    return (FALSE);

#else /* ST_OS21 */
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    sttst_Print ("Profiler not implemented for this OS");
    return (TRUE);
#endif /* ST_OS21 */
}

/*-------------------------------------------------------------------------
 * Function : do_profiler_start
 *            Start Profiler Data acquisition
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE if ok, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL do_profiler_start(STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined ST_OS21 && !defined ST_OSWINCE   
    int result;
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    /* Check if Profiler is already initialized */
    if (!is_profiler_initialized)
    {
        sttst_Print ("Call PROFILER_INIT first\n");
        return(TRUE);
    }

    if (is_profiler_started)
    {
        sttst_Print ("The Profiler is already started!\n");
        return(TRUE);
    }

    result = profile_start_all();

    if (result != OS21_SUCCESS)
    {
        sttst_Print ("Error during profile_start_all\n");
        return (TRUE);
    }

    sttst_Print ("Profiler started\n");
    is_profiler_started = TRUE;

    return (FALSE);

#else /* ST_OS21 */
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    sttst_Print ("Profiler not implemented for this OS");
    return (TRUE);
#endif /* ST_OS21 */
}

/*-------------------------------------------------------------------------
 * Function : do_profiler_stop
 *            Stop OS21 Profiler acquisition and dump result to a file called "profiler.log".
 *            Result should be parsed by PERL scrypt "os21prof.pl"
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : FALSE if ok, else TRUE
 * ----------------------------------------------------------------------*/
static BOOL do_profiler_stop(STTST_Parse_t *pars_p, char *result_sym_p)
{
#if defined ST_OS21 && !defined ST_OSWINCE 
    int result;
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);

    /* Check if Profiler is already initialized */
    if (!is_profiler_initialized)
    {
        sttst_Print ("Call PROFILER_INIT first\n");
        return(TRUE);
    }

    if (!is_profiler_started)
    {
        sttst_Print ("The Profiler should be started first\n");
        return(TRUE);
    }

   result = profile_stop();

    if (result != OS21_SUCCESS)
    {
        sttst_Print ("Error during profile_stop\n");
        return (TRUE);
    }

   sttst_Print ("Profiler stopped\n");
   is_profiler_started = FALSE;

   /* Write the Data on the Host */
   result = profile_write("profiler.log");

    if (result != OS21_SUCCESS)
    {
        sttst_Print ("Error during profile_write\n");
        return (TRUE);
    }

   sttst_Print ("Results written in profiler.log \n");

    return (FALSE);

#else /* ST_OS21 */
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result_sym_p);
    sttst_Print ("Profiler not implemented for this OS");
    return (TRUE);
#endif /* ST_OS21 */

}


/* ========================================================================
   fundamental command execution loop. Commands come either from
   a supplied string or from an input stream (file or TTY). The loop
   identifies the type of input statement and determines the action to take
   as a result of this. A range of simple execution control primitives have
   been supplied. Block structure is implemented using temporary macro-like
   structures.
=========================================================================== */
static BOOL command_loop(macro_t *macro_p, FILE *file_p, char *rtn_exp_p, BOOL echo)
{
    BOOL     IsBad = FALSE;
    BOOL     ExitLoop = FALSE;
    BOOL     not_done = TRUE;
    BOOL     sav_echo;
    macro_t *sav_macro_p;
    FILE    *sav_stream_p;
    STTST_Parse_t  *Parse_p;
    S16      control_const;
    BOOL     if_flag; /* set if seen in the last line examined */
    BOOL     if_taken, elif_taken;
    int      i;
    char     *target, *StringBuffer;

    Parse_p = NewParseTableSlot(CLI_MAX_LINE_LENGTH);
    StringBuffer = NewStringTableSlot(CLI_MAX_LINE_LENGTH);

    sav_echo = CurrentEcho;
    sav_macro_p = CurrentMacro_p;
    sav_stream_p = CurrentStream_p;
    CurrentEcho = echo;
    CurrentMacro_p = macro_p;
    CurrentStream_p = file_p;
    if_flag = FALSE;

    /* exit from loop on all command error except when interactive */
    while( !ExitLoop && (!IsBad || ((MacroDepth <= 1) && (CurrentStream_p == stdin))))
    {
        /* read an input line and start parsing process. This nesting level
        will terminate if the end of the stream is reached or an error
        is detected. The outermost loop, generally interactive, will not
        terminate on error. */
        ExitLoop = read_input(StringBuffer, Prompt_p);
        init_pars(Parse_p, StringBuffer);
        get_tok(Parse_p, CommandLineDelimiter);

        if (ExitLoop || is_matched(Parse_p->token, "EXIT", 2)
             || is_matched(Parse_p->token, "END", 2))
        {
            ExitLoop = TRUE;
        }
        else if (Parse_p->tok_len > 0)
        {
            /* proceed with examination of an input line. Broadly speaking
             these come in three flavours - assignment, statement and control. The
             key distiguishing features are a leading '=' separator or
             the first token being a member of a small set of reserved
             control words. In each executable
             case, an attempt is made to  execute the line in various
             forms before declaring an error. */
            if (Parse_p->tok_del == ' ')
            {
                /* allow any amount of white space before '=' in assignment */
                i = Parse_p->par_pos;
                while (StringBuffer[i] == ' ')
                {
                    i++;
                }
                if (StringBuffer[i] == '=')
                {
                    Parse_p->tok_del = '=';
                    Parse_p->par_pos = i;
                }
            }
            if (Parse_p->tok_del == '=')
            {
                if_flag = FALSE;
                target = NewTokenTableSlot(strlen(Parse_p->token)+1);

                strcpy(target, Parse_p->token);
                get_tok(Parse_p, "");        /* tokenise rest of line */
                not_done = evaluate_assign(target, Parse_p->token, &IsBad, FALSE);
                if (not_done)
                    /* attempt to execute a command or macro assignment*/
                    not_done = execute_command_line(Parse_p->token, target, &IsBad);
                if (not_done)
                    not_done = execute_macro(Parse_p->token, target, FALSE, &IsBad);
                if (not_done)
                    /* report a warning */
                    STTST_TagCurrentLine(Parse_p, "Unrecognised assignment statement");
                if (IsBad)
                    /* report an error in a separate fashion */
                    sttst_Print("Execution of \"%s\" or assignment of \"%s\" failed\n", Parse_p->token, target);
                FreeTokenTableSlot();
            }
            else if (is_control(Parse_p->token, &control_const))
            {
                switch(control_const)
                {
                    case CLI_DEFINE_CONST:
                        IsBad = do_define(Parse_p);
                        if_flag = FALSE;
                        break;
                    case CLI_IF_CONST:
                        IsBad = do_if(Parse_p, &if_taken);
                        if (!IsBad)
                            if_flag = TRUE;
                        break;
                    case CLI_ELIF_CONST:
                        if (!if_flag)
                        {
                          IsBad = TRUE;
                          STTST_TagCurrentLine(Parse_p, "ELIF not allowed without IF");
                        }
                        else
                        {
                            IsBad = do_elif(Parse_p, &elif_taken, if_taken);
                            if_taken |= elif_taken;
                            if (IsBad)
                            {
                                if_flag = FALSE;
                            }
                        }
                        break;
                    case CLI_ELSE_CONST:
                        if (!if_flag)
                        {
                          IsBad = TRUE;
                          STTST_TagCurrentLine(Parse_p, "ELSE not allowed without IF");
                        }
                        else
                        {
                          IsBad = do_else(if_taken);
                          if_flag = FALSE;
                        }
                        break;
                    case CLI_WHILE_CONST:
                        IsBad = do_while(Parse_p);
                        if_flag = FALSE;
                        break;
                    case CLI_FOR_CONST:
                        IsBad = do_for(Parse_p);
                        if_flag = FALSE;
                        break;
                    default:
                        STTST_TagCurrentLine(Parse_p, "Unable to understand control construct");
                        IsBad = TRUE;
                        break;
                }
            }
            else
            {
                if_flag = FALSE;
                not_done = execute_command_line(StringBuffer, "", &IsBad);
                if (not_done)
                    not_done = execute_macro(StringBuffer, "", FALSE, &IsBad);
                if (not_done)
                {
                    STTST_TagCurrentLine(Parse_p, "Unrecognised command statement");
                    IsBad = TRUE;
                }
            }
        }
    }

    /* if this is a normal termination of the loop via an END statement,
     look for an expression to use as a result. As usual, the type depends
     on the context of the expression, and this is evaluated in the
     scope of the enclosing macro invocation */
    if (!IsBad)
    {
        if ((get_tok(Parse_p, DelimiterSet) != 0) && (rtn_exp_p != (char*)NULL))
        {
            strcpy(rtn_exp_p, Parse_p->token);
        }
    }
    /* restore original input information */
    CurrentEcho = sav_echo;
    CurrentMacro_p = sav_macro_p;
    CurrentStream_p = sav_stream_p;

    FreeStringTableSlot();
    FreeParseTableSlot();
    return(IsBad);
}

/* ========================================================================

=========================================================================== */
static BOOL uncommand_loop(macro_t *macro_p, FILE *file_p, char *rtn_exp_p, BOOL echo)
{
    BOOL     IsBad = FALSE;
    BOOL     ExitLoop = FALSE;
    BOOL     sav_echo;
    macro_t *sav_macro_p;
    FILE    *sav_stream_p;
    STTST_Parse_t  *Parse_p;
    S16      control_const;
    char     *StringBuffer;

    Parse_p = NewParseTableSlot(CLI_MAX_LINE_LENGTH);
    StringBuffer = NewStringTableSlot(CLI_MAX_LINE_LENGTH);

    sav_echo = CurrentEcho;
    sav_macro_p = CurrentMacro_p;
    sav_stream_p = CurrentStream_p;
    CurrentEcho = echo;
    CurrentMacro_p = macro_p;
    CurrentStream_p = file_p;

    /* exit from loop on all command error except when interactive */
    while( !ExitLoop && (!IsBad || ((MacroDepth <= 1) && (CurrentStream_p == stdin))))
    {
        /* read an input line and start parsing process. This nesting level
        will terminate if the end of the stream is reached or an error
        is detected. The outermost loop, generally interactive, will not
        terminate on error. */
        ExitLoop = read_input(StringBuffer, Prompt_p);
        init_pars(Parse_p, StringBuffer);
        get_tok(Parse_p, CommandLineDelimiter);

        if (ExitLoop)
        {
            ExitLoop = TRUE;
        }
        else if (Parse_p->tok_len > 0)
        {
            if (is_control(Parse_p->token, &control_const))
            {
                switch(control_const)
                {
                    case CLI_DEFINE_CONST:
                        get_tok(Parse_p, CommandLineDelimiter);
                        IsBad = delete_symbol(Parse_p->token);
                        if (IsBad)
                        {
                            STTST_TagCurrentLine(Parse_p, "Cannot delete symbol out of current scope");
                        }
                        break;
                    case CLI_IF_CONST:
                    case CLI_ELIF_CONST:
                    case CLI_ELSE_CONST:
                    case CLI_WHILE_CONST:
                    case CLI_FOR_CONST:
                        break;
                    default:
                        STTST_TagCurrentLine(Parse_p, "Unable to understand control construct");
                        IsBad = TRUE;
                        break;
                }
            }
        }
    }

    /* if this is a normal termination of the loop via an END statement,
     look for an expression to use as a result. As usual, the type depends
     on the context of the expression, and this is evaluated in the
     scope of the enclosing macro invocation */
    if (!IsBad)
    {
        if ((get_tok(Parse_p, DelimiterSet) != 0) && (rtn_exp_p != (char*)NULL))
        {
            strcpy(rtn_exp_p, Parse_p->token);
        }
    }
    /* restore original input information */
    CurrentEcho = sav_echo;
    CurrentMacro_p = sav_macro_p;
    CurrentStream_p = sav_stream_p;

    FreeStringTableSlot();
    FreeParseTableSlot();
    return(IsBad);
}

/* ========================================================================
   readkey from keyboard or console, will determin who is the host
=========================================================================== */
static int read_key(void)
{
    /*int key;*/
    char key;
    key=0;
    STTBX_InputChar(&key);
    /*while ( 1 )
    {
        if ( io_mode & IO_UART ) {
        STTBX_InputChar((&key));
        if ( key != -1 ) {
        STTBX_Print(("Using uart I/Os\n"));
        if ( io_mode & IO_CONSOLE )
        STTBX_Print(("I/O are now redirected on UART port\n"));
        io_mode = IO_UART;
        break; }
        }
        if ( io_mode & IO_CONSOLE ) {
        STTBX_InputChar((&key));
        if ( key != -1 ) {
        STTBX_Print(("Using console I/Os\n"));
        io_mode = IO_CONSOLE;
        break; }
        }
    } */
    return ( (int ) key );
}


/* ========================================================================
   top level of the cli code, invokes intialisation and top level shell
=========================================================================== */
void sttst_CliInit(BOOL (*setup_r)(void), S16 max_symbols, S16 default_base)
{
    MacroDepth = 0;
    IfNameCount = 0;
    ElifNameCount = 0;
    ElseNameCount = 0;
    ForNameCount = 0;
    WhileNameCount = 0;
    NumberBase = default_base;
    init_sym_table(max_symbols);
    InitDisplayBuffer();
    InitStringTable();
    InitTokenTable();
    InitParseTable();

    /* Set up of all internal commands */

    STTST_RegisterCommand("DELETE", do_delete,  "<symbolnames> Removes named symbols or macros");
    STTST_RegisterCommand("EVAL",   do_eval,  "<commands> Executes the commands given as argument");
    STTST_RegisterCommand("HELP",   do_help,    "<commandnames> Displays help string for named commands and macros");
#ifdef IO_RADIX_CONF
    STTST_RegisterCommand("IOBASE", do_base,    "<hex/decimal> Sets default I/O radix for integer input and output");
#endif
    STTST_RegisterCommand("GETKEY", do_getkey,  "Waits for one key and returns its value");
    STTST_RegisterCommand("GETSTRING", do_getstring, "Waits for a string and returns it");
    STTST_RegisterCommand("HISTORY",do_history, "<number> Displays the last commands or runs the selected one" );
    /* STTST_RegisterCommand("LIST",   do_list,    "<commandname or partial> Lists all strings of command and macro"); */
    STTST_RegisterCommand("POLLKEY",do_pollkey, "Tests if a key was hitten and returns it if any");
    STTST_RegisterCommand("PRINT",  do_print,   "Formats and prints variables");
    STTST_RegisterCommand("SHOW",   do_show,    "<symbolname> Displays symbol values and macro contents");
    STTST_RegisterCommand("STAT",   cli_stat,   "<FALSE | TRUE> Display memory informations. Returns total free size. TRUE option displays blocks details");
    STTST_RegisterCommand("VERIFY", do_verify,  "<flag> Sets echo of commands for macro execution");
#ifndef UNHOSTED
    STTST_RegisterCommand("CLOSE",  do_closelog, "Stops logging and close the log file");
    STTST_RegisterCommand("LOG",    do_log,     "<filestring><options> Logs all command to named file\n\t     options= W|A for write|append , I|O|R for input|input+output|output results\n\t     default= LOG_FILE \"WI\"\n" );
    STTST_RegisterCommand("SAVE",   do_save,    "<filestring><constflag> Saves macros & variables/constants to named file");
    STTST_RegisterCommand("SOURCE", do_source,  "<filestring><echoflag> Executes commands from named file");
    STTST_RegisterCommand("UNSOURCE", do_unsource,  "<filestring><echoflag> Removes named commands from named file");
#endif

    STTST_RegisterCommand("PROFILER_INIT", do_profiler_init, "Init Profiler doing time analysis");
    STTST_RegisterCommand("PROFILER_START", do_profiler_start, "Start Profiler data acquisition");
    STTST_RegisterCommand("PROFILER_STOP", do_profiler_stop, "Stop Profiler and create Log file");

    /* Set up of all internal constant values */

    STTST_AssignInteger("HEXADECIMAL", 16, TRUE);
    STTST_AssignInteger("DECIMAL", 10, TRUE);
    STTST_AssignInteger("OCTAL", 8, TRUE);
    STTST_AssignInteger("BINARY", 2, TRUE);
    STTST_AssignInteger("TRUE", 1, TRUE);
    STTST_AssignInteger("FALSE", 0, TRUE);
    STTST_AssignFloat("PI", 3.14159, TRUE);
    STTST_AssignString("COMMAND_FILE", "default.com", FALSE);
    STTST_AssignString("LOG_FILE", "default.log", FALSE);

    /* Call user defined setup for commands and symbols */

    (*setup_r)();

    /* Set up internal data */

    MacroDepth ++;       /* increase nesting depth for initial input */
    LogFile = NULL;     /* initialise log file */
    LogOutput = FALSE;  /* no output log */
    LogResults = FALSE; /* no result log */

    CommandHistory.old_cmd = 0;
    CommandHistory.last_cmd = 0;
}
/* ========================================================================
   terminate the line command interpretor
=========================================================================== */
void sttst_CliTerm()
{
    U8 i;

    /* deletion of remaining data (symbols and table are deallocated) */
    while (MacroDepth>=0)
    {
        purge_symbols(MacroDepth);
        MacroDepth --;
    }
    memory_deallocate( sttst_InitParams.CPUPartition_p, DisplayBuffer_p);
    DisplayBuffer_p = NULL;
    for (i=0; i< STRING_TABLE_INIT_SIZE; i++)
    {
        memory_deallocate(sttst_InitParams.CPUPartition_p, StringTable_p[i]);
    }
    memory_deallocate(sttst_InitParams.CPUPartition_p, StringTable_p);
    StringTable_p = NULL;
    for (i=0; i< TOKEN_TABLE_INIT_SIZE; i++)
    {
        memory_deallocate(sttst_InitParams.CPUPartition_p, TokenTable_p[i]);
    }
    memory_deallocate(sttst_InitParams.CPUPartition_p, TokenTable_p);
    TokenTable_p = NULL;
    for (i=0; i< PARSE_TABLE_INIT_SIZE; i++)
    {
        memory_deallocate(sttst_InitParams.CPUPartition_p, ParseTable_p[i]);
    }
    memory_deallocate(sttst_InitParams.CPUPartition_p, ParseTable_p);
    ParseTable_p = NULL;

}
/* ========================================================================
   top level of the cli code, invokes top level shell
=========================================================================== */
void sttst_CliRun(char *const ip_prompt_p, char *file_p)
{
    S16 Key;
    BOOL CliContinue;
    char Msg[80];
#ifndef UNHOSTED
    FILE  *inputfd;
#endif

    CliContinue = TRUE;
    Prompt_p = ip_prompt_p;
    while (CliContinue)
    {
        if (( (STTST_RunMode & STTST_BATCH_MODE) == 0 ) && ((STTST_RunMode & STTST_HIT_KEY_TO_ENTER_MODE) != 0))
        {
            Key = read_key();
            if ( Key==KEY_BACKSP || Key==UNIX_KEY_BACKSP )
            {
                return;
            }
        }
        /* Print banner with the revision number */
        sprintf(Msg, "\nTesttool %s running\n\n", (const char *)STTST_GetRevision() );
        sttst_Print(Msg);

#ifndef UNHOSTED
        /* Process command input & command interpretation from input file */

        if ((file_p != NULL) && (strlen(file_p) != 0)) /* if a file is specified ... */
        {
            inputfd = fopen(file_p, "r");
            if (inputfd != NULL )
            {
                sttst_Print("input from file \"%s\"\n",file_p);
                command_loop((macro_t*)NULL, inputfd, (char*)NULL, FALSE);
                fclose(inputfd);
            }
            else
            {
                sttst_Print("Cannot open file \"%s\" for input\n", file_p);
            }
        }
#endif
        /* Process command input & command interpretation from stdin */
        /* Exit if there is an input file and in batch mode          */

        if ( !( ((STTST_RunMode & STTST_BATCH_MODE) != 0)
             && (file_p != NULL)
             && (strlen(file_p) != 0)))
        {
            command_loop((macro_t*)NULL, stdin, (char*)NULL, FALSE);
        }
        file_p = NULL; /* disable for later loops */

#ifndef UNHOSTED
        if (LogFile != NULL)
        {
            fclose(LogFile);
            LogFile = NULL;
        }
#endif

        if ( (STTST_RunMode & STTST_BATCH_MODE) != 0)
        {
            CliContinue = FALSE;
        }
        else if ((STTST_RunMode & STTST_HIT_KEY_TO_ENTER_MODE) == 0)
        {
            CliContinue = FALSE;
        }
    } /* end while */
}

#ifdef __cplusplus
}
#endif





