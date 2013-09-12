/*******************************************************************************

File name   : testtool.c

Description : Testtool module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
05 oct 1998        Ported to 5510                                    FR
09 mar 1999        testtool.h & debug.h removed                      FQ
                   hardware.h & uart.h & flashdrv.h removed
                   readkey(), io_xxx(), pollkeyxxx() moved to report.c
02 apr 1999        sttbx used instead of report                      FQ
20 jul 1999        STTBX_Init() & Term() moved to startup.c          FQ
06 sep 1999        Remove os20.h & symbol.h, add init. param.        FQ
28 dec 1999        Remove display_byte, modif. display & search      FQ
24 jan 2000        Change to STAPI style & removal of unused code    FQ
31 aou 2000        Create STTST_SetMode();                           FQ
02 jul 2001        Modified delay calculation in wait_time           CL
02 Oct 2001        Decrease stack use.                               HSdLM
17 Oct 2001        Force 32 bits aligned accesses for ST40           HSdLM
28 Nov 2001        Fix pb in display (ascii chars)                   HSdLM
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stcommon.h"
#include "clilib.h"
#ifndef STTBX_INPUT
    #define STTBX_INPUT
#endif


#ifndef STTBX_PRINT
    #define STTBX_PRINT
#endif

#include "sttbx.h"

#if defined(ST_OS21) || defined(ST_OSWINCE)
#define FORCE_32BITS_ALIGN
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

static long SearchBase = 0;   /* used by FindString() & FindNextString() */
static long SearchRange = 0;
static long SearchOccurNb = 0;
static char SearchString[CLI_MAX_LINE_LENGTH];
static S16  SearchLen;
static char PrintBuffer[CLI_MAX_LINE_LENGTH];


/* Global Variables --------------------------------------------------------- */

STTST_InitParams_t sttst_InitParams;
S16 STTST_RunMode;       /* interactive or batch mode */

/* used to maintain testtool prompt visible */
static char line[] = "Testtool> ";
/* Private Macros ----------------------------------------------------------- */

#if defined (ST_OSWINCE)
#define FillMemory	FillMemory
#define CopyMemory	CopyMemory
#endif

#ifdef FORCE_32BITS_ALIGN
#define GETBYTE1(v) (U8)((v)&0xFF)
#define GETBYTE2(v) (U8)(((v)&0xFF00)>>8)
#define GETBYTE3(v) (U8)(((v)&0xFF0000)>>16)
#define GETBYTE4(v) (U8)(((v)&0xFF000000)>>24)
#endif

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*extern BOOL io_zap (STTST_Parse_t *pars_p, char *result_sym_p);*/


/****************************************************************************/
/*********************** COMMANDS FOR MEMORY ACCESS *************************/
/****************************************************************************/

/* ========================================================================
testtool command : display memory in 32bits style
=========================================================================== */
static BOOL DisplayMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL  IsBad;
    long  cnt;
    int   i;
    long  base;
#if defined (ST_OSWINCE)
	long  baseWinCE;
	long  addressCE;
#endif
    long  length;
    long  def_base;
    long  def_range;
    long  address;
    char  filename[FILENAME_MAX];
    FILE* dumpfile=NULL;
    char  options[40];
#ifdef FORCE_32BITS_ALIGN
    volatile long *pt;
#else
    char  *pt;
#endif

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)((void*)&def_base), (S16) 10);
    STTST_EvaluateInteger("RANGE", (S32 *)((void*)&def_range), 10);

    IsBad = STTST_GetInteger(pars_p, (S32) def_base, (S32 *)((void*)&base));
    if (IsBad)
    {
        sprintf( PrintBuffer, "Invalid base address (default is BASEADDRESS=h%08lx)", def_base );
        STTST_TagCurrentLine(pars_p, PrintBuffer);
    }
    else
    {
        IsBad = (STTST_GetInteger(pars_p, (S32)def_range, (S32 *)((void*)&length)) || (length <= 0));
        if (IsBad)
        {
            sprintf( PrintBuffer, "Invalid length (default is RANGE=h%03lx)", def_range );
            STTST_TagCurrentLine(pars_p, PrintBuffer);
        }
        else
        {
            IsBad = STTST_GetString(pars_p, "", filename, FILENAME_MAX);
            if ( IsBad )
            {
                STTST_TagCurrentLine(pars_p, "Illegal file name");
            }
            else
            {
                memset( options, 0, sizeof(options) );
                IsBad = STTST_GetString(pars_p, "w", options, 4);
                if ( IsBad || ( options[0]!='W' && options[0]!='w' && options[0]!='A' && options[0]!='a') )
                {
                    STTST_TagCurrentLine(pars_p, "Illegal option (W=write, A=append)");
                    IsBad = TRUE;
                }
                else
                {
                    if ( options[0]== 'W' )
                    {
                        options[0] = 'w';
                    }
                    if ( options[0]== 'A' )
                    {
                        options[0] = 'a';
                    }
                    if ( strcmp(filename,"")!=0 )
                    {
                        dumpfile = fopen(filename, options);
                        if (dumpfile == NULL)
                        {
                            STTBX_Print(("The dump file [%s] cannot be opened !\n",filename));
                            IsBad = TRUE;
                        }
                    }
                }
            }
        }
    }
#if defined (ST_OSWINCE)
	if ( !IsBad )
	{
		baseWinCE = (long)MapPhysicalToVirtual((LPVOID)base, length);
		if (!baseWinCE)
		{
			WCE_ERROR("MapPhysicalToVirtual(base)");
			IsBad = TRUE;
		}
		else
			addressCE = baseWinCE;
	}
#endif	/* ST_OSWINCE */

    if ( !IsBad )
    {
        /* Do a formatted display, address and four values a line */
        /* Watch for a bus error if we stray out of legal memory  */
        address = base;
        cnt = 0;
        while ((cnt <= length))
        {
#if defined(FORCE_32BITS_ALIGN)
    #if defined (ST_OSWINCE)
            pt = (volatile long *)(addressCE & 0xFFFFFFFC);
	#else
            pt = (volatile long *)(address & 0xFFFFFFFC);
	#endif	/* ST_OSWINCE */
            sprintf( PrintBuffer,
                    "h%08lx: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x",
                    address,
                    GETBYTE1(*pt)    , GETBYTE2(*pt)    , GETBYTE3(*pt)    , GETBYTE4(*pt),
                    GETBYTE1(*(pt+1)), GETBYTE2(*(pt+1)), GETBYTE3(*(pt+1)), GETBYTE4(*(pt+1)),
                    GETBYTE1(*(pt+2)), GETBYTE2(*(pt+2)), GETBYTE3(*(pt+2)), GETBYTE4(*(pt+2)),
                    GETBYTE1(*(pt+3)), GETBYTE2(*(pt+3)), GETBYTE3(*(pt+3)), GETBYTE4(*(pt+3)));
#else
            pt = (char *)address;
            sprintf( PrintBuffer,
                    "h%08lx: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x",
                    address,
                    *pt     , *(pt+1) , *(pt+2) , *(pt+3),
                    *(pt+4) , *(pt+5) , *(pt+6) , *(pt+7),
                    *(pt+8) , *(pt+9) , *(pt+10), *(pt+11),
                    *(pt+12), *(pt+13), *(pt+14), *(pt+15) );

#endif /* #ifdef FORCE_32BITS_ALIGN */
            if ( strcmp(filename,"")!=0 )
            {
                fprintf(dumpfile, "%s", PrintBuffer);
            }
            else
            {
                STTBX_Print(("  %s", PrintBuffer));
            }
#ifdef FORCE_32BITS_ALIGN
            for (i=0;i<4;i++)
            {
                PrintBuffer[4*i]   = GETBYTE1(*(pt+i));
                PrintBuffer[4*i+1] = GETBYTE2(*(pt+i));
                PrintBuffer[4*i+2] = GETBYTE3(*(pt+i));
                PrintBuffer[4*i+3] = GETBYTE4(*(pt+i));
            }
            for (i=0;i<16;i++)
            {
                if (isprint(PrintBuffer[i]) == 0)
                {
                    PrintBuffer[i] = '.';
                }
            }
#else
            for (i=0;i<16;i++)
            {
                PrintBuffer[i] = (U8)*(pt+i);
                if (isprint((int)PrintBuffer[i]) == 0)
                {
                    PrintBuffer[i] = '.';
                }
            }
#endif /* #ifdef FORCE_32BITS_ALIGN */
            PrintBuffer[16] = '\0';
            if ( strcmp(filename,"")!=0 )
            {
                fprintf(dumpfile, "  %s\n", PrintBuffer);
            }
            else
            {
                STTBX_Print(("  %s\n", PrintBuffer));
            }
            /* protect ourselves from address rollover */
            if (address >= 0x7ffffff0)
            {
                address = (long)0x80000000;
#if defined (ST_OSWINCE)
                /* unmap the address range*/
				if (baseWinCE)
					UnmapPhysicalToVirtual((LPVOID)baseWinCE);
				baseWinCE = (long)MapPhysicalToVirtual((LPVOID)address,length-cnt);
				if (!baseWinCE)
				{
					WCE_ERROR("MapPhysicalToVirtual(address)");
					IsBad = TRUE;
					break;
				}
				else
					addressCE = baseWinCE;
#endif /* ST_OSWINCE */
            }
            else
            {
                address += 16;
#if defined (ST_OSWINCE)
				addressCE += 16;
#endif	/* ST_OSWINCE */
            }
            cnt +=16;
        }
        if ( strcmp(filename,"")!=0 )
        {
            STTBX_Print(("Dump stored in [%s] file\n",filename));
            fclose(dumpfile);
        }
#if defined (ST_OSWINCE)
        /* unmap the address range*/
		if (baseWinCE)
			UnmapPhysicalToVirtual((LPVOID)baseWinCE);
#endif /* ST_OSWINCE */
    }

    IsBad |= STTST_AssignInteger(result_sym_p, (S32)base, FALSE) ;

    return(IsBad);
}
#if defined (ST_OSWINCE)
/* IC
 * function added by WINCE team but create pbs with LINUX so defined only for WINCE OS
 * */
static BOOL DisplayRegister(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL  IsBad;
    long  cnt;
    long  base;
#if defined (ST_OSWINCE)
	long  baseWinCE;
	long  addressCE;
#endif
    long  length;
    long  def_base;
    long  def_range;
    long  address;
    char  filename[FILENAME_MAX];
    FILE* dumpfile=NULL;
    char  options[40];
#if defined(FORCE_32BITS_ALIGN)
    volatile long *pt;
#else
    char  *pt;
#endif

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);
    STTST_EvaluateInteger("RANGE", (S32 *)&def_range, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base );
    if (IsBad)
    {
        sprintf( PrintBuffer, "Invalid base address (default is BASEADDRESS=h%08lx)", def_base );
        STTST_TagCurrentLine(pars_p, PrintBuffer);
    }
    else
    {
        IsBad = (STTST_GetInteger(pars_p, (S32)def_range, (S32 *)&length) || (length <= 0));
        if (IsBad)
        {
            sprintf( PrintBuffer, "Invalid length (default is RANGE=h%03lx)", def_range );
            STTST_TagCurrentLine(pars_p, PrintBuffer);
        }
        else
        {
            IsBad = STTST_GetString(pars_p, "", filename, FILENAME_MAX);
            if ( IsBad )
            {
                STTST_TagCurrentLine(pars_p, "Illegal file name");
            }
            else
            {
                memset( options, 0, sizeof(options) );
                IsBad = STTST_GetString(pars_p, "w", options, 4);
                if ( IsBad || ( options[0]!='W' && options[0]!='w' && options[0]!='A' && options[0]!='a') )
                {
                    STTST_TagCurrentLine(pars_p, "Illegal option (W=write, A=append)");
                    IsBad = TRUE;
                }
                else
                {
                    if ( options[0]== 'W' )
                    {
                        options[0] = 'w';
                    }
                    if ( options[0]== 'A' )
                    {
                        options[0] = 'a';
                    }
                    if ( strcmp(filename,"")!=0 )
                    {
                        dumpfile = fopen(filename, options);
                        if (dumpfile == NULL)
                        {
                            STTBX_Print(("The dump file [%s] cannot be opened !\n",filename));
                            IsBad = TRUE;
                        }
                    }
                }
            }
        }
    }
#if defined (ST_OSWINCE)
	if ( !IsBad )
	{
		baseWinCE = (long)MapPhysicalToVirtual((LPVOID)base, length*4);
		if (!baseWinCE)
		{
			WCE_ERROR("MapPhysicalToVirtual(base)");
			IsBad = TRUE;
		}
		else
			addressCE = baseWinCE;
	}
#endif	/* ST_OSWINCE */

    if ( !IsBad )
    {
        /* Do a formatted display, address and four values a line */
        /* Watch for a bus error if we stray out of legal memory  */
        address = base;
        cnt = 0;
        while ((cnt < length))
        {
#if defined(FORCE_32BITS_ALIGN)
    #if defined (ST_OSWINCE)
            pt = (volatile long *)(addressCE & 0xFFFFFFFC);
	#else
            pt = (volatile long *)(address & 0xFFFFFFFC);
	#endif	/* ST_OSWINCE */
            sprintf( PrintBuffer,
                    "h%08lx: %02x %02x %02x %02x\n",
					address,
                    GETBYTE1(*pt)    , GETBYTE2(*pt)    , GETBYTE3(*pt)    , GETBYTE4(*pt)
					);
#else
            pt = (char *)address;
            sprintf( PrintBuffer,
                    "h%08lx: %02x %02x %02x %02x\n",
					address,
                    *pt     , *(pt+1) , *(pt+2) , *(pt+3)
                    );

#endif /* #ifdef FORCE_32BITS_ALIGN */

            STTBX_Print(("  %s", PrintBuffer));

#ifdef FORCE_32BITS_ALIGN
            /* protect ourselves from address rollover */
            if (address >= 0x7ffffff0)
            {
                address = (long)0x80000000;
    #if defined (ST_OSWINCE)
                /* unmap the address range*/
				if (baseWinCE)
					UnmapPhysicalToVirtual((LPVOID)baseWinCE);
				baseWinCE = (long)MapPhysicalToVirtual((LPVOID)address,length-cnt);
				if (!baseWinCE)
				{
					WCE_ERROR("MapPhysicalToVirtual(address)");
					IsBad = TRUE;
					break;
				}
				else
					addressCE = baseWinCE;
	#endif /* ST_OSWINCE */
            }
            else
            {
                address += 4;
    #if defined (ST_OSWINCE)
				addressCE += 4;
	#endif	/* ST_OSWINCE */
            }
            cnt ++;
        }
#endif
#if defined (ST_OSWINCE)
        /* unmap the address range*/
		if (baseWinCE)
			UnmapPhysicalToVirtual((LPVOID)baseWinCE);
#endif /* ST_OSWINCE */
    }

    IsBad |= STTST_AssignInteger(result_sym_p, (S32)base, FALSE) ;

    return(IsBad);
}
#endif /*ST_OSWINCE*/
/* ========================================================================
testtool command : finds string in range of memory (created by FQ, Jan 99)
=========================================================================== */
static BOOL do_find(char *string_p, long *base_p, long range_p, long max_occur_p,
                S16 len_p, char *result_sym_p)
{
    BOOL  IsBad;
    char  *pt;
    long  match =0;
    S16   cpt;
    U32   length;
#if defined (ST_OSWINCE)
	char *  baseWinCE=NULL;
	long offset;
    LPVOID baseFill;
#endif

    length = (U32)len_p;
    IsBad = TRUE; /* not found */
    pt = (char *)(*base_p) ;
    cpt = 0;
#if defined (ST_OSWINCE)
	pt = baseWinCE = (char *)MapPhysicalToVirtual((LPVOID)*base_p, range_p);
	if (!pt)
	{
    while ( ( pt < (char *)(baseWinCE+range_p) ) && ( cpt < max_occur_p ) )
#else
    while ( ( pt < (char *)(*base_p+range_p) ) && ( cpt < max_occur_p ) )
#endif
    {
        if (*pt == string_p[0])
        {
            if ( memcmp(pt, string_p, len_p) == 0 )
            {
                IsBad = FALSE;
                match = (long)pt;
                /*STTBX_Print(("Match found at #%lx\n", match));*/
                STTBX_Print(("Match found at h%lx\n", match));
                if ( cpt==0 )
                {
                    IsBad = STTST_AssignInteger(result_sym_p, (S32)match, FALSE);
                }
                pt = pt + length;
                cpt++;
            }
            else
            {
                pt++;
            }
        }
        else
        {
            pt++;
        }
    } /* end while */
#if defined (ST_OSWINCE)
    /* unmap the address range*/
	if (baseWinCE)
		UnmapPhysicalToVirtual((LPVOID)baseWinCE);
	}
#endif
    if ( cpt > 0 )
    {
        *base_p = match+len_p; /* base address for the next search */
    }
    if ( IsBad || (cpt==0) )
    {
		IsBad = STTST_AssignInteger(result_sym_p, (S32)0, FALSE);
    }
    return(IsBad);
}

/* ========================================================================
testtool command : finds string in range of memory (modif. by FQ, Jan 99)
=========================================================================== */
static BOOL FindString(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long def_base;
    long def_range;
    long def_data;
    char temp[4];
    char *pt;
    long match;
    long lval;
    BOOL not_a_string =FALSE;
    S16  cpt;

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)((void*)&def_base), 10);
    STTST_EvaluateInteger("RANGE", (S32 *)((void*)&def_range), 10);
    STTST_EvaluateInteger("DATAVALUE", (S32 *)((void*)&def_data), 10);
    match = 0;

    /* get the data item (string or integer) */
    IsBad = STTST_GetItem(pars_p, "", SearchString, CLI_MAX_LINE_LENGTH);
    if ( IsBad )
    {
        STTST_TagCurrentLine(pars_p, "Illegal search item");
    }
    else
    {
        STTST_GetTokenCount( pars_p, &cpt );
        if ( cpt == 1 )
        {
            SearchString[0] = (char)def_data;
            not_a_string = TRUE;
            SearchLen = 1;
        }
        else
        {
            /* is it an integer value ? */
            IsBad = STTST_EvaluateInteger(pars_p->token, (S32 *)((void*)&lval), 10);
            if ( IsBad)
            {
                /* is it a string ? */
                IsBad = STTST_EvaluateString(pars_p->token, SearchString, CLI_MAX_LINE_LENGTH);
                if ( IsBad || strlen(SearchString) == 0 )
                {
                    STTST_TagCurrentLine(pars_p, "Illegal search string");
                    IsBad = TRUE;
                    SearchLen = 0;
                }
                else
                {
                    not_a_string = FALSE;
                    SearchLen = strlen(SearchString);
                }
            }
            else
            {
                not_a_string = TRUE;
                SearchLen = 4;
                /* is it an hexadecimal value ? */
                if ( pars_p->token[0]=='#' || pars_p->token[0]=='h' )
                {
                    SearchLen = pars_p->tok_len/2 ;
                    if ( SearchLen > 4 )
                    {
                        IsBad = TRUE;
                        STTST_TagCurrentLine(pars_p, "Illegal search value (max. is 4 bytes for an hexadecimal value)");
                    }
                }
                /* Be carefull with endianess on long value with DCU ST20.                */
                /* ex: sea #12345678 --> lval is #78563412 --> string should be #12345678 */
                /* ex: sea #1234     --> lval is #34120000 --> string should be #12340000 */
                pt = (char *)&lval;
                temp[0] = pt[3];
                temp[1] = pt[2];
                temp[2] = pt[1];
                temp[3] = pt[0];
                pt = temp + ( 4 - SearchLen ) ; /* skip the zeros */
                memcpy( SearchString, pt, SearchLen );
            }
        } /* end if cpt */
    } /* end if item ok */

    if (!IsBad)
    {
        /* get start address */
        IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)((void*)&SearchBase));
        if (IsBad)
        {
            sprintf( PrintBuffer, "Invalid base address (default is BASEADDRESS=h%08lx)", def_base );
            STTST_TagCurrentLine(pars_p, PrintBuffer);
        }
    }
    if (!IsBad)
    {
        /* get range */
        IsBad = STTST_GetInteger(pars_p, (S32)def_range, (S32 *)((void*)&SearchRange));
        if ( IsBad || SearchRange <= 0 )
        {
            sprintf( PrintBuffer, "Invalid range (default is RANGE=h%08lx)", def_range );
            STTST_TagCurrentLine(pars_p, PrintBuffer);
        }
    }
    if (!IsBad)
    {
        /* get nb of occurences to found */
        IsBad = STTST_GetInteger(pars_p, (S32)1, (S32 *)((void*)&SearchOccurNb));
        if ( IsBad || SearchOccurNb <= 0 )
        {
            STTST_TagCurrentLine(pars_p, "Invalid number max. of occurences (default is 1)");
        }
    }

    if (!IsBad)
    {
        /* trace what must be searched */
        pt = (char *)SearchBase;
        if ( not_a_string )
        {
            sprintf( PrintBuffer, "Search h%02x", SearchString[0] ) ;
            cpt = 1;
            while ( cpt < SearchLen )
            {
                if ( SearchLen >= 2 )
                {
                    sprintf( PrintBuffer, "%s%02x", PrintBuffer, SearchString[cpt] );
                }
                cpt++;
            }
            sprintf( PrintBuffer, "%s on %d bytes from h%08lx to h%08lx ...\n",
                PrintBuffer, SearchLen, SearchBase, (SearchBase+SearchRange) );
        }
        else
        {
            sprintf( PrintBuffer, "Search [%s] on %d bytes from h%08lx to h%08lx ...\n",
                SearchString, SearchLen, SearchBase, (SearchBase+SearchRange) );
        }
        STTBX_Print((PrintBuffer));

        /* search in memory */
        IsBad = do_find( SearchString, &SearchBase, SearchRange, SearchOccurNb,
                        SearchLen, result_sym_p);
        /* if success then SearchBase is updated to the match address */
    } /* end not IsBad */

    return(IsBad);
}

/* ========================================================================
testtool command : finds string in range of memory
=========================================================================== */
static BOOL FindNextString(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    UNUSED_PARAMETER(pars_p);

    IsBad = do_find( SearchString, &SearchBase, SearchRange, 1,
                SearchLen, result_sym_p);
    /* if success then SearchBase is updated to the match address */

    return(IsBad);
}

/* ========================================================================
testtool command : writes specific memory contents
=========================================================================== */
static BOOL FillMemory(STTST_Parse_t *pars_p, char *result_sym_p)

{
    BOOL IsBad;
    long base;
    long length;
    long value;
    long def_base;
    long def_range;
    long def_value;
    volatile long* address;

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)((void*)&def_base), 10);
    STTST_EvaluateInteger("RANGE", (S32 *)((void*)&def_range), 10);
    STTST_EvaluateInteger("DATAVALUE", (S32 *)((void*)&def_value), 10);
    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)((void*)&base));
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "Invalid base address");
    }
    else
    {
        IsBad = (STTST_GetInteger(pars_p, (S32)def_range, (S32 *)((void*)&length)) || (length <= 0));
        if (IsBad)
        {
            STTST_TagCurrentLine(pars_p, "Invalid length");
        }
        else
        {
            IsBad = STTST_GetInteger(pars_p, (S32)def_value, (S32 *)((void*)&value));
            if (IsBad)
            {
                STTST_TagCurrentLine(pars_p, "Invalid data value");
            }
            else
            {
				/* do a word oriented global fill operation*/
#if defined (ST_OSWINCE)
				address = baseFill = MapPhysicalToVirtual((LPVOID)base, length);
                while ((address < ((long *)baseFill + length)))
#else
                address = (volatile long*) base;
                while (((long)address < (base + length)))
#endif
                {
                    *address = value;
                    address++;
                }
#if defined (ST_OSWINCE)
                /* unmap the address range*/
				if (baseFill)
					UnmapPhysicalToVirtual((LPVOID)baseFill);
#endif
            }
        }
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)base, FALSE));
}

/* ========================================================================
testtool command : copies memory contents to new address range
=========================================================================== */
static BOOL CopyMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long length;
    long newbase;
    long def_base;
    long def_range;
    long def_newbase;

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)((void*)&def_base), 10);
    STTST_EvaluateInteger("RANGE", (S32 *)((void*)&def_range), 10);
    STTST_EvaluateInteger("ADDRESSVALUE", (S32 *)((void*)&def_newbase), 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)((void*)&base));
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "Invalid base address");
    }
#ifdef FORCE_32BITS_ALIGN
    else if ((base & 0x03) != 0)
    {
        STTST_TagCurrentLine(pars_p, "address must be 32 bits aligned");
        IsBad = TRUE;
    }
#endif
    if (!IsBad)
    {
        IsBad = (STTST_GetInteger(pars_p, (S32)def_range, (S32 *)((void*)&length)) || (length <= 0));
        if (IsBad)
        {
            STTST_TagCurrentLine(pars_p, "Invalid length");
        }
        else
        {
            IsBad = (STTST_GetInteger(pars_p, (S32)def_newbase, (S32 *)((void*)&newbase)) ||
                (newbase == base));
            if (IsBad)
            {
                STTST_TagCurrentLine(pars_p, "Invalid new base address");
            }
#ifdef FORCE_32BITS_ALIGN
            else if ((newbase & 0x03) != 0)
            {
                STTST_TagCurrentLine(pars_p, "address must be 32 bits aligned");
                IsBad = TRUE;
            }
#endif
        }
    }
    if (!IsBad)
    {
        /* do a memory copy operation using routine that works in overlapped case*/
#if defined (ST_OSWINCE)
		base = (long)MapPhysicalToVirtual((LPVOID)base, length);
		newbase = (long)MapPhysicalToVirtual((LPVOID)newbase, length);
#endif
        memmove((void *)newbase, (void *)base, (int) length);
#if defined (ST_OSWINCE)
        /* unmap the address range*/
		if (base)
			UnmapPhysicalToVirtual((LPVOID)base);
		if (newbase)
			UnmapPhysicalToVirtual((LPVOID)newbase);
#endif
    }

    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)base, FALSE));
}

/* ========================================================================
testtool command : peek an 8 bit value
=========================================================================== */
#ifndef ST_OSLINUX
static BOOL PeekByteMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long def_base;
    char value = 0;

    /* get default value from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base);
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "expected base address");
        IsBad = TRUE;
    }

    if (!IsBad)
    {
#if defined(FORCE_32BITS_ALIGN) && !defined(ST_OSWINCE)
        if ((base & 0x03) != 0)
        {
            STTST_TagCurrentLine(pars_p, "address must be 32 bits aligned");
            IsBad = TRUE;
        }
        else
        {
            value = (char)(*((volatile long*)base));
        }
#else
#if defined (ST_OSWINCE)
		base = (long)MapPhysicalToVirtual((LPVOID)base, 1);
		if (!base)
		{
			WCE_ERROR("MapPhysicalToVirtual(base)");
            IsBad = TRUE;
		}
		else
		{
			value = (char)(*(volatile U8*)(base));
			UnmapPhysicalToVirtual((LPVOID)base);
		}
#else
        value = (char)(*((volatile U8*)base));
#endif	/* ST_OSWINCE */
#endif
        STTBX_Print(("read 0x%x (hex) = %d (dec) \n",(U8) value, (char) value ));
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)value, FALSE));
}
#endif
/* ========================================================================
testtool command : poke an 8 bit value
=========================================================================== */
#ifndef ST_OSLINUX
static BOOL PokeByteMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long value;
    long def_base;
    long def_value;

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);
    STTST_EvaluateInteger("DATAVALUE", (S32 *)&def_value, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base);
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "Invalid base address");
    }
#if defined(FORCE_32BITS_ALIGN) && !defined(ST_OSWINCE)
    else if ((base & 0x03) != 0)
    {
        STTST_TagCurrentLine(pars_p, "Address must be 32 bits aligned");
        IsBad = TRUE;
    }
#endif

    if (!IsBad)
    {
        IsBad = STTST_GetInteger(pars_p, (S32)def_value, (S32 *)&value);
        if( (value < 0) || (value > 255) )
        {
            IsBad = TRUE;
        }
        if (IsBad)
        {
            STTST_TagCurrentLine(pars_p, "Invalid data value");
        }
        else
        {
#if defined(FORCE_32BITS_ALIGN) && !defined(ST_OSWINCE)
			*((volatile long *) base) = (long)value;
#else
    #if defined (ST_OSWINCE)
			base = (long)MapPhysicalToVirtual((LPVOID)base, 1);
			if (!base)
			{
				WCE_ERROR("MapPhysicalToVirtual(base)");
				IsBad = TRUE;
			}
			else
			{
				*((volatile U8*) base) = (U8)value;
				UnmapPhysicalToVirtual((LPVOID)base);
			}
	#else
			*((volatile U8*) base) = (U8)value;
	#endif	/* ST_OSWINCE */
#endif
        }
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)value, FALSE));
}
#endif
/* ========================================================================
testtool command : peek an 16 bit value
=========================================================================== */
#ifndef ST_OSLINUX
static BOOL PeekShortMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long def_base;
    short value = 0;

    /* get default value from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base);
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "expected base address");
    }
    else
    {
#ifdef FORCE_32BITS_ALIGN
        if ((base & 0x03) != 0)
        {
            STTST_TagCurrentLine(pars_p, "address must be 32 bits aligned");
            IsBad = TRUE;
        }
        else
        {
            value = (short)(*((volatile long*)base));
        }
#else
        value = (short)(*((volatile U16*)base));
#endif
        STTBX_Print(("read 0x%x (hex) = %d (dec) \n",(U16) value, (short) value ));
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)value, FALSE));
}
#endif
/* ========================================================================
testtool command : poke an 16 bit value
=========================================================================== */
#ifndef ST_OSLINUX
static BOOL PokeShortMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long value;
    long def_base;
    long def_value;

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);
    STTST_EvaluateInteger("DATAVALUE", (S32 *)&def_value, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base);
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "Invalid base address");
    }
#ifdef FORCE_32BITS_ALIGN
    else if ((base & 0x03) != 0)
    {
        STTST_TagCurrentLine(pars_p, "Address must be 32 bits aligned");
        IsBad = TRUE;
    }
#endif
    if (!IsBad)
    {
        IsBad = STTST_GetInteger(pars_p, (S32)def_value, (S32 *)&value);
        if( (value < 0) || (value > 65535) )
        {
            IsBad = TRUE;
        }
        if (IsBad)
        {
            STTST_TagCurrentLine(pars_p, "Invalid data value");
        }
        else
        {
#ifdef FORCE_32BITS_ALIGN
            *((volatile long *) base) = (long)value;
#else
            *((volatile U16*) base) = (U16)value;
#endif
        }
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)value, FALSE));
}
#endif

/* ========================================================================
testtool command : peek a 32 bit value
=========================================================================== */
#ifndef ST_OSLINUX
static BOOL PeekMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long def_base;
    long value = 0;

    /* get default value from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base);
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "expected base address");
    }
#ifdef FORCE_32BITS_ALIGN
    else if ((base & 0x03) != 0)
    {
        STTST_TagCurrentLine(pars_p, "address must be 32 bits aligned");
        IsBad = TRUE;
    }
#endif
    if (!IsBad)
    {
#if defined (ST_OSWINCE)
		base = (long)MapPhysicalToVirtual((LPVOID)base, 4);
		if (!base)
		{
			WCE_ERROR("MapPhysicalToVirtual(base)");
            IsBad = TRUE;
		}
		else
		{
			value = *((volatile long*)base);
			UnmapPhysicalToVirtual((LPVOID)base);
		}
#else
        value = *((volatile long*)base);
#endif /* ST_OSWINCE */
        STTBX_Print(("read 0x%x (hex) = %d (dec) \n",(int) value, (int) value));
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)value, FALSE));
}
#endif
/* ========================================================================
testtool command : poke a 32 bit value
=========================================================================== */
#ifndef ST_OSLINUX
static BOOL PokeMemory(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long base;
    long value;
    long def_base;
    long def_value;

    /* get default values from symbol variables */
    STTST_EvaluateInteger("BASEADDRESS", (S32 *)&def_base, 10);
    STTST_EvaluateInteger("DATAVALUE", (S32 *)&def_value, 10);

    IsBad = STTST_GetInteger(pars_p, (S32)def_base, (S32 *)&base);
    if (IsBad)
    {
        STTST_TagCurrentLine(pars_p, "Invalid base address");
    }
    else
    {
        IsBad = STTST_GetInteger(pars_p, (S32)def_value, (S32 *)&value);
        if (IsBad)
        {
            STTST_TagCurrentLine(pars_p, "Invalid data value");
        }
#ifdef FORCE_32BITS_ALIGN
        else if ((base & 0x03) != 0)
        {
            STTST_TagCurrentLine(pars_p, "Address must be 32 bits aligned");
            IsBad = TRUE;
        }
#endif
    }
    if (!IsBad)
    {
#if defined (ST_OSWINCE)
		base = (long)MapPhysicalToVirtual((LPVOID)base, 4);
		if (!base)
		{
			WCE_ERROR("MapPhysicalToVirtual(base)");
            IsBad = TRUE;
		}
		else
		{
			*((volatile long*) base) = value;
			UnmapPhysicalToVirtual((LPVOID)base);
		}
#else
        *((volatile long*) base) = value;
#endif
    }
    return(IsBad || STTST_AssignInteger(result_sym_p, (S32)value, FALSE));
}
#endif
/****************************************************************************/
/**************************** OTHER COMMANDS ********************************/
/****************************************************************************/
/* ========================================================================
testtool command : pauses and deschedule
=========================================================================== */
static BOOL WaitTime(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long time_wait;
    int  ip_char;
    UNUSED_PARAMETER(result_sym_p);

    IsBad = STTST_GetInteger(pars_p, (S32)1, (S32 *)((void*)&time_wait));
    if (IsBad || (time_wait < 0) || (time_wait > 100000))
    {
        IsBad = TRUE;
        STTST_TagCurrentLine(pars_p, "Expected number of milliseconds to wait");
    }
    else
    {
        /* CL modif July 2nd 2001 */
        /* uses ST_GetClocksPerSecond() to take CPU speed into account */
#if defined(ST_OS21) || defined(ST_OSWINCE)
        task_delay((osclock_t)((time_wait*ST_GetClocksPerSecond())/1000)); /* wait x millisec. */
#else
		/* Take care of ST_GetClocksPerSecond() granularity. If the number of clock per   */
        /* second is huge, then "time*(ST_GetClocksPerSecond" may overflow. So consider   */
        /* the max limit 100000 that may allow a max tempo of 21474 ms.                   */
        if (ST_GetClocksPerSecond() > 1000000)
        {
            task_delay((unsigned int)(((U32)time_wait*(ST_GetClocksPerSecond()/1000))));
        }
        else
        {
            task_delay((unsigned int)((time_wait*ST_GetClocksPerSecond())/1000)); /* wait x millisec. */
        }
#endif
        /* end of CL modif July 2nd 2001*/
        /* task_delay((unsigned int)time*15.625); wait x sec. */
        /* test for user abort during wait */
        /*ip_char = pollkey();FQ*/
#if !defined(ARCHITECTURE_ST200) && !defined(ST_OSWINCE)
        STTBX_InputPollChar((char *) (&ip_char));
        if (ip_char == 0x1b)
        {
            IsBad = TRUE;
        }
#endif
    }
    return(IsBad);
}

#ifndef ST_OSLINUX
/* ========================================================================
testtool command : raise an interrupt
=========================================================================== */
static BOOL RaiseInterrupt(STTST_Parse_t *pars_p, char *result_sym_p)
{
    BOOL IsBad;
    long level;
    UNUSED_PARAMETER(result_sym_p);

    IsBad = STTST_GetInteger(pars_p, (S32)0, (S32 *)&level);
    if (IsBad || (level < 0) || (level >= 8))
    {
        IsBad = TRUE;
        STTST_TagCurrentLine(pars_p, "Expected interrupt level");
    }
    else
    {
        interrupt_raise_number((int)level);
    }
    return(IsBad);
}
#endif

/* ========================================================================
testool command : display processes and Interrupts
=========================================================================== */
#if defined (ST_OSWINCE)
static BOOL MemoryInfo (STTST_Parse_t *pars_p, char *result_sym_p)
{
	DisplayRAMPartitionStatus();
	return 0;
}
#endif

/***static BOOL MemoryInfo (STTST_Parse_t *pars_p, char *result_sym_p)
{
BOOL       IsBad = FALSE;

#if 0
char          *ptr;
int           i;
int           unused;

extern local_interrupt_control_t   interrupt_control[NUM_INTERRUPT_LEVELS];

task_t*  Task;
int*     Stack;
int*     Stack_limit;
int      Unused;
char     PriString[16];
extern task_t*              TaskListHead;


STTBX_Report((STTBX_REPORT_LEVEL_INFO,"****** Interrupts stack *******\n"));
STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Level   Size    Used\n"));
for (i = 0; i < NUM_INTERRUPT_LEVELS; i++)
{
    unused = 0;
    ptr = (char *) interrupt_control[i].stack_base;
    while(*ptr == 0x55)
    {
    unused++;
    ptr++;
    }
    IsBad |= (unused == 0);

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
        "%-8d%-8d%d%s\n",
        i,
        interrupt_control[i].stack_size,
        interrupt_control[i].stack_size - unused,
        unused == 0 ? "*" : ""));
}
*/
/*report (severity_info, "\n****** Processes stack ********\n");
report (severity_info,"%-30s %-6s %-6s %-8s %-4s\n",*/
/*  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n****** Processes stack ********\n"));
STTBX_Report((STTBX_REPORT_LEVEL_INFO,"%-30s %-6s %-6s %-8s %-4s\n",
    "Name",
    "Size",
    "Used",
    "Base",
    "Priority"));
task_lock ();
Task = TaskListHead;
while (Task->task_next != NULL)
{ */
/* This will discard the "root" process */
/*    Unused = 0;
    Stack  = (int*)Task->task_stack_base;
    Stack_limit = Stack + (Task->task_stack_size / sizeof (int)); */
    /* Skip task name first */
/*    while ((*Stack != 0xaaaaaaaa) && (Stack < Stack_limit))
    {
    Stack++;
    Unused++;
    } */
    /* Skip unused */
/*    while ((*Stack == 0xaaaaaaaa) &&  (Stack < Stack_limit))
    {
    Stack++;
    Unused++;
    }

    if (Stack == Stack_limit)
    Unused = 0;
    if (Task->task_tdesc != NULL)
    sprintf (PriString, "%d", Task->task_tdesc->tdesc_priority);
    else
    strcpy (PriString, "High");

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"%-30s %-6d %-6d %p %-4s\n",
        (char*)(Task->task_stack_base),
        Task->task_stack_size,
        Task->task_stack_size - (Unused * 4),
        Task->task_stack_base,
        PriString));
    Task = Task->task_next;
}
task_unlock ();

#endif

return(IsBad);
}***/
/* ========================================================================
cli_setup : register default routines
=========================================================================== */
static BOOL cli_setup(void)
{
    STTST_RegisterCommand("COPY", CopyMemory,
                "<address><range><newaddress> Copies local memory contents");
    STTST_RegisterCommand("DISPLAY", DisplayMemory,
                "<address><range> Displays local memory contents\n\t     default = BASEADDRESS RANGE\n\t     <address><range><filename><option> Dump local memory contents in a file\n\t     option= W|A for write|append (default is w)\n" );
    STTST_RegisterCommand("FILL", FillMemory,
                "<address><range><value> Fill local memory with 32bits value");
#ifndef ST_OSLINUX
    STTST_RegisterCommand("BPEEK", PeekByteMemory,
                "<address> Extracts 8 bit memory value");
    STTST_RegisterCommand("BPOKE", PokeByteMemory,
                "<address><value> Sets 8 bit memory value");
    STTST_RegisterCommand("SPEEK", PeekShortMemory,
                "<address> Extracts 16 bit memory value");
    STTST_RegisterCommand("SPOKE", PokeShortMemory,
                "<address><value> Sets 16 bit memory value");
    /*
	STTST_RegisterCommand("DISPLAYREG", DisplayRegister,
                "<address><range> Displays register contents\n\t     default = BASEADDRESS RANGE\n\t     <address><range><filename><option> Dump local memory contents in a file\n\t     option= W|A for write|append (default is w)\n" );
     * */
    STTST_RegisterCommand("PEEK", PeekMemory,
                "<address> Extracts 32 bit memory value");
    STTST_RegisterCommand("POKE", PokeMemory,
                "<address><value> Sets 32 bit memory value");
    STTST_RegisterCommand("INTERRUPT", RaiseInterrupt,
                "<level> Raise interrupt internally");
#endif

#if defined (ST_OSWINCE)
    STTST_RegisterCommand("MEMORY", MemoryInfo,
                "Dump system memory usage");
#endif

    STTST_RegisterCommand("SEARCH", FindString,
                "<data><address><range><max> Searchs data in local memory\n\t     default: data=DATAVALUE addr=BASEADDRESS range=RANGE max_occurences=1\n\t     the 1st match address is returned (0 if not found)");
    STTST_RegisterCommand("SEARCH_NEXT", FindNextString,
                "Continues the previous search in local memory");
    STTST_RegisterCommand("WAIT", WaitTime,
                "<millisecs> Pause for a time period (available with 1 tick each 64 microsec)");
    /*STTST_RegisterCommand("ZAP", io_zap,
                "Change I/O controls UART/CONSOLE"); FQ */

    STTST_AssignInteger("ZERO", (S32)0, TRUE);
    STTST_AssignInteger("BASEADDRESS", (S32)0x80000000, FALSE);
    STTST_AssignInteger("RANGE", (S32)0x100, FALSE);
    STTST_AssignInteger("DATAVALUE", (S32)0, FALSE);
    STTST_AssignInteger("ADDRESSVALUE", (S32)0x80000000, FALSE);

    return(FALSE);
}

/****************************************************************************/
/**************************  EXTERNAL FUNCTIONS *****************************/
/****************************************************************************/

/***************************************************************
Name : STTST_GetRevision
Description : Retrieve the driver revision
Parameters : none
Return : driver revision
****************************************************************/
ST_Revision_t STTST_GetRevision(void)
{
    static const char Revision[]="STTST-REL_3.3.6";

    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */

    return((ST_Revision_t)Revision);
} /* End of STTST_GetRevision() */


/* ========================================================================
Initialize the command line interpreter
=========================================================================== */
BOOL STTST_Init( const STTST_InitParams_t * const InitParam_p)
{
    memcpy( (char *)&sttst_InitParams,
            (const char *)InitParam_p,
            sizeof(sttst_InitParams) );
    if ( sttst_InitParams.CPUPartition_p == NULL )
    {
        return(TRUE);
    }
    if ( sttst_InitParams.NbMaxOfSymbols==0 )
    {
        sttst_InitParams.NbMaxOfSymbols = 1000;
    }
    sttst_CliInit( cli_setup, sttst_InitParams.NbMaxOfSymbols, 10);
    STTST_RunMode = STTST_INTERACTIVE_MODE;
    return(FALSE);
}

/* ========================================================================
Start the command line interpreter
=========================================================================== */
BOOL STTST_Start(void)
{
    /* This is a temporary solution to remove warnings, it will be reviewed */
    sttst_CliRun((char* const) line, sttst_InitParams.InputFileName);

    return(FALSE);
}

/* ========================================================================
Set Testtool mode
=========================================================================== */
BOOL STTST_SetMode(STTST_RunningMode_t RunMode)
{
    BOOL RetErr;

    if ( ( RunMode | STTST_SUPPORTED_MODES_MASK ) != STTST_SUPPORTED_MODES_MASK )
    {
        RetErr = TRUE; /* unknown mode */
        STTST_RunMode = STTST_INTERACTIVE_MODE; /* default mode */
    }
    else
    {
        STTST_RunMode = RunMode;
        RetErr = FALSE;
    }
    return(RetErr);
}

/* ========================================================================
Terminates the command line interpreter
=========================================================================== */
BOOL STTST_Term(void)
{
    sttst_CliTerm();
    return(FALSE);
}


#ifdef __cplusplus
}
#endif




