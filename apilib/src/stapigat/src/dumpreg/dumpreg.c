/*****************************************************************************

File name   : Dumpreg.c

Description :  Test of registers

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                 Name
----               ------------                 ----
23-March 2001      Valid. software modified      BD from FC
27-April 2001      Remove unnecessary exports    XD
                   and functions

*****************************************************************************/


/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "testtool.h"

#ifdef ST_OSLINUX
#include "compat.h"
#endif

#include "sttbx.h"

#include "stdevice.h"
#include "testcfg.h"
#ifdef DUMP_REGISTERS
#include "dumpreg.h"

#if defined ST_7015 || defined ST_7020
#include "audio_reg.h"
#include "cfg_reg.h"
#include "cfg_treg.h"
#include "denc_reg.h"
#include "denc_treg.h"
#include "display_reg.h"
#include "display_treg.h"
#include "gamma_reg.h"
#include "gamma_treg.h"
#include "hdin_reg.h"
#include "hdin_treg.h"
#include "sdin_reg.h"
#include "sdin_treg.h"

/* Choose chip                                                                */
#if defined (ST_7015)
#define VIDEO_7015
#elif defined (ST_7020)
#define VIDEO_7020
#endif
#include "video_reg.h"
#include "video_treg.h"
#include "vos_reg.h"
#include "vos_treg.h"
#include "vtg_reg.h"
#include "vtg_treg.h"
#include "audio_treg.h"

#endif /* ST_7015 || ST_7020 */

#if defined (ST_5528)
#define VIDEO_5528
#include "gamma_reg.h"
#include "gamma_treg.h"
#include "video_reg.h"
#include "video_treg.h"
#endif /* ST_5528 */

#if defined (ST_7710)
#define VIDEO_7710
#include "gamma_reg.h"
#include "gamma_treg.h"
#include "video_reg.h"
#include "video_treg.h"
#endif /* ST_7710 */

#if defined (ST_7100)
#define VIDEO_7100
#include "gamma_reg.h"
#include "gamma_treg.h"
#include "video_reg.h"
#include "video_treg.h"
#endif /* ST_7100 */

#if defined (ST_7109)
#define VIDEO_7109
#include "gamma_reg.h"
#include "gamma_treg.h"
#include "video_reg.h"
#include "video_treg.h"
#endif /* ST_7109 */

/* Private Types ------------------------------------------------------------ */
typedef struct RegArray_s
{
    struct RegArray_s *Next;
    const Register_t *RegList;
    S32 Size;
} RegArray_t;

/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */
static RegArray_t *RegArrays = NULL;

/* Private Macros ----------------------------------------------------------- */
/* Register access */
#ifdef ST_OSLINUX
#define ReadReg8(a)  STAPIGAT_Phys_BRead(a)
#define ReadReg16(a) STAPIGAT_Phys_SRead(a)
#define ReadReg24(a) (STAPIGAT_Phys_Read(a) & 0xFFFFFF)
#define ReadReg32(a) STAPIGAT_Phys_Read(a)

#define WriteReg8(a, v) STAPIGAT_Phys_Write(a, (STAPIGAT_Phys_Read(a) & 0xFFFFFF00) | v)
#define WriteReg16(a, v) STSYS_WriteRegDev16LE(a, (STAPIGAT_Phys_Read(a) & 0xFFFF0000) | v)
#define WriteReg24(a, v) STSYS_WriteRegDev24LE(a, (STAPIGAT_Phys_Read(a) & 0xFF000000) | v)
#define WriteReg32(a, v) STSYS_WriteRegDev32LE(a, v)

#else

#define ReadReg8(a)  STSYS_ReadRegDev8(a)
#define ReadReg16(a) STSYS_ReadRegDev16LE(a)
#define ReadReg24(a) STSYS_ReadRegDev24LE(a)
#define ReadReg32(a) STSYS_ReadRegDev32LE(a)

#define WriteReg8(a, v) STSYS_WriteRegDev8(a, v)
#define WriteReg16(a, v) STSYS_WriteRegDev16LE(a, v)
#define WriteReg24(a, v) STSYS_WriteRegDev24LE(a, v)
#define WriteReg32(a, v) STSYS_WriteRegDev32LE(a, v)
#endif

/* testtool conversion bases */
#define HEXADECIMAL_BASE 16
#define DECIMAL_BASE 10

/* Private Function prototypes ---------------------------------------------- */
static BOOL RegCmp(const char *Name, char *RegExp);
static S32 getIndex(char *RegExp);

static void writeReg(Register_t Reg, U32 Val, S32 index);
static U32 printReg(FILE *Fp, Register_t Reg, S32 index);

static U32 readAddr(void *Addr, U32 Mask, U32 Type);
static void writeAddr(void *Addr, U32 Val, U32 Mask, U32 Type);

/* Functions ---------------------------------------------------------------- */


#if defined ST_7015 || defined ST_7020 || defined ST_5528 || defined ST_7710 || defined ST_7100 || defined ST_7109
/*------------------------------------------------------------------------------
 Finds one Array of registers in the list of register Arrays
 Param:     Reg[]   the array of registers to find
 Return:            the found array, or NULL
------------------------------------------------------------------------------*/
static const Register_t* FindRegArray(const Register_t Reg[])
{
    RegArray_t *p = RegArrays;

    /* Look for the Array in the list */
    if (Reg != NULL)
    {
        while(p != NULL) {
            if (p->RegList == Reg)
            {
                return p->RegList;
            }
            else
            {
                p = p->Next;
            }
       	}
    }

    return NULL;
}

/*------------------------------------------------------------------------------
 Adds one Array of registers in the list of register Arrays
 Param:     Reg[]   pointer on array of registers to add
            Size    size of the given array
------------------------------------------------------------------------------*/
static void DUMPREG_AddRegArray(const Register_t Reg[], S32 Size)
{
    RegArray_t *new;

    /* Look if it's already registered */
    if(FindRegArray(Reg) == NULL)
    {
        /* allocate new elt */
        new = (RegArray_t *)malloc(sizeof(RegArray_t));
        /* add it to the head of the list */
        if(new != NULL)
        {
            new->RegList = Reg;
            new->Size = Size;
            new->Next = RegArrays;
            RegArrays = new;
        }
    }
}
#endif /* ST_7015 || ST_7020 || ST_5528 || ST_7710 || ST_7100 || ST_7109 */

/*-------------------------------------------------------------------------
 * Function : DUMPREG_Init
 *          : Initialise the DUMP driver
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t DUMPREG_Init()
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

#if defined ST_7015 || defined ST_7020
    DUMPREG_AddRegArray(CFGReg,    CFG_REGISTERS);
    DUMPREG_AddRegArray(VideoReg,  VIDEO_REGISTERS);
    DUMPREG_AddRegArray(SdinReg,   SDIN_REGISTERS);
    DUMPREG_AddRegArray(HdinReg,   HDIN_REGISTERS);
    DUMPREG_AddRegArray(VtgReg,    VTG_REGISTERS);
    DUMPREG_AddRegArray(VosReg,    VOS_REGISTERS);
    DUMPREG_AddRegArray(DencReg,   DENC_REGISTERS);
    DUMPREG_AddRegArray(DisplayReg,DISPLAY_REGISTERS);
    DUMPREG_AddRegArray(GammaReg,  GAMMA_REGISTERS);
    DUMPREG_AddRegArray(AudioReg,  AUDIO_REGISTERS);
#endif /* ST_7015 || ST_7020 */

#if defined ST_5528
    DUMPREG_AddRegArray(GammaReg,  GAMMA_REGISTERS);
    DUMPREG_AddRegArray(VideoReg,  VIDEO_REGISTERS);
#endif /* ST_7015 || ST_7020 */

#if defined ST_7710 || defined ST_7100 || defined ST_7109
    DUMPREG_AddRegArray(GammaReg,  GAMMA_REGISTERS);
    DUMPREG_AddRegArray(VideoReg,  VIDEO_REGISTERS);
#endif /* ST_7015 || ST_7020 */


#ifdef STTBX_PRINT
    STTBX_Print(("DUMPREG_Init()=Ok\n"));
#endif

    return ( ErrCode );

} /* end DUMPREG_Init() */

/*------------------------------------------------------------------------------
 Compares one name with a regular expression
 Param:     Name    the name to compare
            RegExp  the regular expression with which perform comparison
 Return:            TRUE if register matches the given name
------------------------------------------------------------------------------*/
static BOOL RegCmp(const char *Name, char *RegExp)
{
    char *r = RegExp;
    const char *s = Name;
    char *p = NULL;
    char Tok[64];
    BOOL Star = FALSE;

    while ( *r!='\0' && *s!='\0')
    {
        switch (*r)
        {
            case '?' :                  /* jump over one character */
                r++;
                s++;
                break;
            case '*' :                  /* inc. regexp pointer, memorize star */
                r++;
                Star = TRUE;
                break;
            default:
                strcpy(Tok, r);    /* get next token */
                strtok(Tok, "*?");
                if (Star)
                {
                    p = (char *)strstr(s, Tok);     /* p points on the found token */
                    if ( p==NULL )
                        return FALSE;       /* doesn't match token */
                    else
                        s = p+strlen(Tok);  /* s points after the found token */
                }
                else
                {
                    if (strncmp(s, Tok, strlen(Tok)) != 0)    /* compare to next characters */
                        return FALSE;
                    else
                        s += strlen(Tok);       /* prepare next step */
                }
                r+= strlen(Tok);
                Star = FALSE;           /* we processed the '*' */
                break;
        }
    }

    /* nothing left in either string, or regexp finishes by a star */
    if (*r == '\0' && (*s == '\0' || Star==TRUE))
        return TRUE;
    else
        return FALSE;
}

/*------------------------------------------------------------------------------
 Extracts a possible index from the end of a regular expression
 Param:     RegExp  the regular expression with which perform comparison
 Return:            the value of the index, -1 if no index found
------------------------------------------------------------------------------*/
static S32 getIndex(char *RegExp)
{
    char *s, *e;
    S32 index = -1;

    if ((s = strchr(RegExp, (S32) '[')) != NULL)
        if ((e = strchr(s+1, (S32) ']')) != NULL)
        {
            *e = '\0';		/* replace ']' by end of string */
            STTST_EvaluateInteger(s+1, &index, DECIMAL_BASE);
            *s = '\0';		/* remove index from the RegExp */
        }

    return (S32)index;
}


/*------------------------------------------------------------------------------
 Prints register contents
 Param:     Fp		the output log file
   		   	Reg  	the register to read
   		   	Index 	index for register array, -1 if not valid
------------------------------------------------------------------------------*/
static U32 printReg(FILE *Fp, Register_t Reg, S32 Index)
{
    U32 Start = 0, Step = 4, nbr = 0, i;
    U32 RegValue = 0;

    if (Reg.Type & REG_ARRAY) 						/* Index is meaningful */
    {
        if (Index >= 0)
        {
            if (Index > (S32)Reg.Size)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Index out of bounds !\n"));
                return(0);
            }
            Start = Index;                          /* start position */
            nbr = 1;								/* read only one value */
        }
        else
        {
            Start = 0;
            nbr = Reg.Size;							/* read everything */
        }
    }
    else if (Reg.Type & REG_CIRCULAR)
    {
        Start = 0;
        nbr = Reg.Size;								/* read everything */
    }
    else 											/* only one register */
    {
        Start = 0;
        nbr = 1;
    }

    if (Reg.Type & REG_CIRCULAR)					/* no increment needed */
        Step = 0;
    else if (Reg.Type & REG_32B)
        Step = 4;
    else if(Reg.Type & REG_24B)
        Step = 3;
    else if(Reg.Type & REG_16B)
        Step = 2;
    else if(Reg.Type & REG_8B)
        Step = 1;

    for (i=Start; i<Start+nbr; i++)
    {
        if (Reg.RdMask != 0)
        {
            if (Index >= 0 && ((Reg.Type & (REG_ARRAY|REG_CIRCULAR)) != 0))
            {
                if (Fp == stdout)
                {
                    STTBX_Print(("%-18s[%3d]", Reg.Name, i));
                }
                else
                {
                    fprintf(Fp, "%-18s[%3d]", Reg.Name, i);
                }
            }
            else
            {
                if (Fp == stdout)
                {
                    STTBX_Print(("%-21s", Reg.Name));
                }
                else
                {
                    fprintf(Fp, "%-21s", Reg.Name);
                }
            }
            RegValue = readAddr((void *)(Reg.Base + Reg.Offset + i*Step), Reg.RdMask, Reg.Type);
                if (Fp == stdout)
                {
                    STTBX_Print((", Addr: %08x, Mask: %08x, Val: %08x\n",
                                 (Reg.Base + Reg.Offset + i*Step),
                                 Reg.RdMask,
                                 RegValue));
                }
                else
                {
                    fprintf(Fp, ", Addr: %08x, Mask: %08x, Val: %08x\n",
                            (Reg.Base + Reg.Offset + i*Step),
                            Reg.RdMask,
                            RegValue);
                }
        }
    }
    return(RegValue);
}

/*------------------------------------------------------------------------------
 Writes value in register
 Param:          	Reg  	the register to write
			Val		the value to write
   		   	Index 	index for register array, -1 if not valid
------------------------------------------------------------------------------*/
static void writeReg(Register_t Reg, U32 Val, S32 Index)
{
    U32 Step = 4;

    if (Reg.Type & REG_ARRAY) 		/* Index is meaningful */
    {
        if (Index >= 0)
        {
            if (Index > (S32)Reg.Size)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Index out of bounds !\n"));
                return;
            }
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error: Please give an index !\n"));
            return;
        }
    }
    else
    {
        Index = 0;
    }

    if (Reg.Type & REG_32B)
        Step = 4;
    else if(Reg.Type & REG_24B)
        Step = 3;
    else if(Reg.Type & REG_16B)
        Step = 2;
    else if(Reg.Type & REG_8B)
        Step = 1;

    writeAddr((void *)(Reg.Base + Reg.Offset + Index*Step), Val, Reg.WrMask, Reg.Type);
}


/*------------------------------------------------------------------------------
 Reads address contents
 Param:     Addr 	the address to read
            Mask 	the mask to use
            Type 	register type (size)
------------------------------------------------------------------------------*/
static U32 readAddr(void *Addr, U32 Mask, U32 Type)
{
    if (Mask == 0)
        return 0;

    if (Type & REG_8B)
       	return (ReadReg8((U8*)Addr) & Mask);
    else if (Type & REG_16B)
    	return (ReadReg16((U16*)Addr) & Mask);
    else if (Type & REG_24B)
    	return (ReadReg24((U32*)Addr) & Mask);
    else	/* 32 bits */
    	return (ReadReg32((U32*)Addr) & Mask);
}


/*------------------------------------------------------------------------------
 Writes value at given address
 Param:     Addr 	the write address
            Val 	the value to write
            Mask 	the mask to use
            Type 	register type (size)
------------------------------------------------------------------------------*/
static void writeAddr(void *Addr, U32 Val, U32 Mask, U32 Type)
{
    if (Type & REG_8B)
    {
    	WriteReg8(Addr, (Val & Mask));
    }
    else if (Type & REG_16B)
    {
    	WriteReg16(Addr, (Val & Mask));
    }
    else if (Type & REG_24B)
    {
    	WriteReg24(Addr, (Val & Mask));
    }
    else	/* 32 bits */
    {
    	WriteReg32(Addr, (Val & Mask));
    }
}

/*------------------------------------------------------------------------------
 Prints contents of all registers matching given name
 Param:     pars_p  		the parsed arguments
 			result_sym_p	the command result
 Return:					FALSE if no error occured
------------------------------------------------------------------------------*/
static BOOL DUMPREG_Read(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char        regexp[64];
    S32 		addr;
    S32 		index;
    RegArray_t 	*p = RegArrays;
    S32 		r;
    BOOL 		found = FALSE;
    BOOL        isName = FALSE;
    U32         RegValue = 0;

    /* 1st argument: register name, with possible wildcards */
    if (STTST_GetString(pars_p, "*", regexp, sizeof(regexp)) == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "expected register name");
    	STTST_AssignInteger(result_sym_p, 0, FALSE);
    	return FALSE;
    }
    if ( pars_p->token[0]=='"' ) /* if 1st argument start by quote */
    {
        isName = TRUE;
    }
    /* get a possible index */
    index = getIndex(regexp);

    /* print all matching registers */
    while (p != NULL)
    {
        for(r = 0; r < p->Size; r++)
        {
            if(RegCmp(((p->RegList)[r]).Name, regexp))
            {
            	found = TRUE;
                if(!strpbrk(regexp,"?*") ||
                   (((p->RegList)[r]).Type & REG_SIDE_READ) == 0)
                {
                    RegValue = printReg(stdout, (p->RegList)[r], (S32)index);
                }
            }
        }
        p = p->Next;
    }

    if (!found)
    {
        if (isName)
        {
            STTBX_Print(("Register name %s not found !\n", regexp));
        }
        else
        {
          if (STTST_EvaluateInteger(regexp, &addr, HEXADECIMAL_BASE))
          {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,  "Couldn't find register or address %s\n", regexp));
            STTBX_Print(("Register name or address %s not found !\n", regexp));
            STTST_AssignInteger(result_sym_p, 0, FALSE);
            return FALSE;
    	}
    	else
    	{
            RegValue = readAddr((void *)addr, DEFAULT_MASK, DEFAULT_TYPE);
            STTBX_Print(("Addr: %08x, Mask: %08x, Val: %08x\n",
                         (S32)addr, DEFAULT_MASK, RegValue));
    	}
    }
    }

    STTST_AssignInteger(result_sym_p, RegValue, FALSE);
    return FALSE;
}

/*------------------------------------------------------------------------------
 Dumps contents of all registers matching given name
 Param:     pars_p  		the parsed arguments
 			result_sym_p	the command result
 Return:					FALSE if no error occured
------------------------------------------------------------------------------*/
static BOOL DUMPREG_Dump(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char        regexp[64];
    char 		filename[64];
    S32 		index;
    static 		FILE *fp;
    RegArray_t 	*p = RegArrays;
    S32 		r;
    BOOL 		found = FALSE;

    /* 1st argument: register name, with possible wildcards */
    if (STTST_GetString(pars_p, "*", regexp, sizeof(regexp)) == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "expected register name");
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }

    /* 2nd argument: dump file name */
    if (STTST_GetString(pars_p, "*", filename, sizeof(regexp)) == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "expected dump file name");
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }

    if ((fp = fopen(filename, "w")) == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error opening file %s\n", filename));
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }

    /* get a possible index */
    index = getIndex(regexp);

    /* dump all matching registers */
    while (p != NULL)
    {
        for(r = 0; r < p->Size; r++)
        {
            if(RegCmp(((p->RegList)[r]).Name, regexp))
            {
                found = TRUE;
                if(!strpbrk(regexp,"?*") ||
                   (((p->RegList)[r]).Type & REG_SIDE_READ) == 0)
                {
    	            printReg(fp, (p->RegList)[r], index);
                }
            }
        }
        p = p->Next;
    }
    fclose(fp);

    STTST_AssignInteger(result_sym_p, found ? FALSE : TRUE, FALSE);
    return (FALSE);
}

/*------------------------------------------------------------------------------
 Writes value in all registers matching given name
 Param:     pars_p  		the parsed arguments
 			result_sym_p	the command result
 Return:					FALSE if no error occured
------------------------------------------------------------------------------*/
static BOOL DUMPREG_Write(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char        regexp[64];
    S32 		val;
    S32 		addr;
    S32 		index;
    RegArray_t 	*p = RegArrays;
    S32 		r;
    BOOL	 	found = FALSE;

    /* 1st argument: register name, with possible wildcards */
    if (STTST_GetString(pars_p, "*", regexp, sizeof(regexp)) == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "expected register name");
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }

    /* 2nd argument: value to write */
    if (STTST_GetInteger(pars_p, 0, &val) == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "expected integer value");
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }

    /* get a possible index */
    index = getIndex(regexp);

    /* write in all matching registers */
    while (p != NULL)
    {
        for(r = 0; r < p->Size; r++)
        {
            if(RegCmp(((p->RegList)[r]).Name, regexp))
            {
            	found = TRUE;
                writeReg((p->RegList)[r], (U32)val, index);
            }
        }
        p = p->Next;
    }

    if (!found)
    {
    	if (STTST_EvaluateInteger(regexp, &addr, HEXADECIMAL_BASE))
    	{
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Couldn't find register or address %s\n", regexp));
            STTST_AssignInteger(result_sym_p, TRUE, FALSE);
            return FALSE;
    	}
    	else
    	{
            writeAddr((void *)addr, (U32)val, DEFAULT_MASK, DEFAULT_TYPE);
    	}
    }
    STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    return FALSE;
}

/*------------------------------------------------------------------------------
 Lists all registers matching given name
 Param:     regexp 		    register name (with/without wildcards)
 Return:					FALSE if no error occured
------------------------------------------------------------------------------*/
static BOOL Dumpreg_ListReg(char *regexp)
{
    RegArray_t 	*p;
    S32  		index;
    S32 		r;
    BOOL 		found = FALSE;

    /* get a possible index */
    index = getIndex(regexp);

    /* list all registers */
    p = RegArrays;
    while (p != NULL)
    {
        for(r = 0; r < p->Size; r++)
        {
            if(RegCmp(((p->RegList)[r]).Name, regexp))
            {
                found = TRUE;
                STTST_Print(("%-21s", ((p->RegList)[r]).Name));
                if (index >= 0)
                {
                    STTST_Print(("[%1d]", index));
                }
                STTST_Print((", Addr: %08x, RdMask: %08x, WrMask: %08x",
                             ((p->RegList)[r]).Base + ((p->RegList)[r]).Offset,
                             ((p->RegList)[r]).RdMask,
                             ((p->RegList)[r]).WrMask));
                if((((p->RegList)[r]).RdMask) == 0)
                {
                    STTST_Print((", WO"));
                }
                if((((p->RegList)[r]).WrMask) == 0)
                {
                    STTST_Print((", RO"));
                }
                if((((p->RegList)[r]).Type) & REG_SPECIAL)
                {
                    STTST_Print((", Special"));
                }
                if((((p->RegList)[r]).Type) & REG_SIDE_READ)
                {
                    STTST_Print((", Read Side Effect"));
                }
                if((((p->RegList)[r]).Type) & REG_SIDE_WRITE)
                {
                    STTST_Print((", Write Side Effect"));
                }
                if((((p->RegList)[r]).Type) & REG_NEED_CLK)
                {
                    STTST_Print((", Needs CLK"));
                }
                if((((p->RegList)[r]).Type) & REG_CIRCULAR)
                {
                    STTST_Print((", Circular[%1d]",((p->RegList)[r]).Size));
                }
                if((((p->RegList)[r]).Type) & REG_ARRAY)
                {
                    STTST_Print((", Array[%4d]",((p->RegList)[r]).Size));
                }
                if((((p->RegList)[r]).Type) & REG_EXCL_FROM_TEST)
                {
                    STTST_Print((", EXCLUDED"));
                }
                STTST_Print(("\n"));
            }
        }
        p = p->Next;
    }
    return found;
}

/*------------------------------------------------------------------------------
 Lists all registers matching given name
 Param:     pars_p  		the parsed arguments
 			result_sym_p	the command result
 Return:					FALSE if no error occured
------------------------------------------------------------------------------*/
static BOOL DUMPREG_ListReg(STTST_Parse_t *pars_p, char *result_sym_p)
{
    char  Regexp[64];
    BOOL        Found = FALSE;

    /* 1st argument: register name, with possible wildcards */

    if (STTST_GetString(pars_p, "*", Regexp, sizeof(Regexp)) == TRUE)
    {
        STTST_TagCurrentLine(pars_p, "expected register name");
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }

    /* list all registers */
    STTST_Print(("Listing registers:\n"));

    Found = Dumpreg_ListReg(Regexp);

    if (!Found)
    {
        STTST_Print(("\tSorry, couldn't find register: %s \n", Regexp));
    	STTST_AssignInteger(result_sym_p, TRUE, FALSE);
    	return FALSE;
    }
    else
    {
    	STTST_AssignInteger(result_sym_p, FALSE, FALSE);
    	return FALSE;
    }
}


/*-------------------------------------------------------------------------
* Function : DUMPREG_RegisterCmd
*            Init Testtool commands for DUMP registers * Input    : N/A
* Output   : N/A
* Return   : BOOL (FALSE if OK)
*
----------------------------------------------------------------------*/
BOOL DUMPREG_RegisterCmd(void) {
    BOOL RetErr;
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand("REG_LIST",DUMPREG_ListReg,"<RegisterName> List register(s)");
    RetErr |= STTST_RegisterCommand("REG_READ",DUMPREG_Read,"<RegisterName | RegisterAddress> Read register(s) content");
    RetErr |= STTST_RegisterCommand("REG_WRITE",DUMPREG_Write,"<RegisterName> <Value> Write value in register(s)");
    RetErr |= STTST_RegisterCommand("REG_DUMP",DUMPREG_Dump,"<RegisterName> <FileName> Dump register(s) contents in file");
    if ( RetErr == TRUE )
    {
        STTBX_Print(("DUMPREG_RegisterCmd() : failed !\n" ));
    }
    else
    {
        STTBX_Print(("DUMPREG_RegisterCmd() : ok\n"));
    }

    return(!(RetErr));
} /* DUMPREG_RegisterCmd () */

#endif /* DUMP_REGISTERS */

/* End of dumpreg.c */

