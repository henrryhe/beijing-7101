/*******************************************************************************
File name   : vbi_ttx.c

Description : vbi hal source file for teletext use, on denc

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                           JG
12 Dec 2000        Improve error tracking                            HSdLM
 *                 Correct mask framing code enabling
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
04 Jul 2001        Update DENC register access functions names       HSdLM
 *                 Fix & change line programming. Restrict to lines
 *                 7->22 and 320->335
14 Nov 2001        Select TTX lines individually (DDTS GNBvd08671)   HSdLM
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stdenc.h"
#include "vbi_drv.h"
#include "ondenc.h"
#include "denc_reg.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* from ETSI standard TR 101 233 V1.1.1 (1998-02) :
 * " Lines 6/319 should be used for quiet lines or test signals, because not all
 * exiting Teletext decoders can accept on these lines. " */

/* Bit position for odd/even teletext line enable. DENC v >= 7 */
#define TTX_ODD_REG             DENC_TTXL1
#define TTX_EVEN_REG            DENC_TTXL4
#define TTX_ODD_FIRST_POS       2       /* Line 7, not line 6 cf comment above */
#define TTX_EVEN_FIRST_POS      7       /* Line 320, not line 318/319 cf comment & doc above  */
#define TTX_FIRST_LINE_OFFSET   7       /* Line 7, not line 6 cf comment above */
#define TTX_LAST_LINE_OFFSET    22      /* Line 0x17 to 0x1F reserved, cf comment above */

/* The DENC v <= 6 handles line enabling differently */
#define TTX_FIELD1_BLOCK1_POS   7
#define TTX_FIELD2_BLOCK2_POS   2

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define UPDATE_REG(hnd, reg, mask, condition) ((condition) ?  STDENC_OrReg8((hnd), (reg), (mask)) \
                                                            : STDENC_AndReg8((hnd), (reg), (U8)~(mask)))

/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t SetTtxOff(      const STDENC_Handle_t DencHnd
                                    , const HALVBI_DencCellId_t DencCellId
                                    , const BOOL Field
                               );
static ST_ErrorCode_t SetTtxLinesOn(  const STDENC_Handle_t DencHnd
                                    , const HALVBI_DencCellId_t DencCellId
                                    , const U16 LineC
                                    , U32 LineMask
                                    , const BOOL Field
                                   );

/* Functions (private) ------------------------------------------------------ */


/*******************************************************************************
Name        : SetTtxOff
Description : set teletext off on denc
Parameters  : IN : DencHnd : Handle on STDENC to reach DENC registers
 *            IN : DencCellId : DENC cell version
 *            IN : Field : TRUE => ODD
Assumptions : Denc Handle has been checked
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
static ST_ErrorCode_t SetTtxOff( const STDENC_Handle_t DencHnd, const HALVBI_DencCellId_t DencCellId, const BOOL Field)
{
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    STDENC_RegAccessLock(DencHnd);
    if (DencCellId <= 6) /* denc V3 - V6 */
    {
        if (Field) /* ODD / Field1 */
        {
            Ec = STDENC_WriteReg8(DencHnd, DENC_TTX1, 0x0);
        }
        else      /* EVEN / Field2 */
        {
            Ec = STDENC_WriteReg8(DencHnd, DENC_TTX2, 0x0);
        }
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_TTX3, 0x0); /* not used */
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_TTX4, 0x0); /* not used */
    }
    else /* denc V7 - V11 */
    {
        if (Field) /* ODD / Field1 */
        {
            Ec = STDENC_AndReg8(DencHnd, DENC_TTXL1, DENC_TTXL1_MASK_CONF);
            if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_TTXL2, 0x0);
            if (OK(Ec)) Ec = STDENC_AndReg8(DencHnd, DENC_TTXL3, (U8)~DENC_TTXL3_MASK_F1);
        }
        else      /* EVEN / Field2 */
        {
            Ec = STDENC_AndReg8(DencHnd, DENC_TTXL3, DENC_TTXL3_MASK_F1);
            if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_TTXL4, 0x0);
            if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_TTXL5, 0x0);
        }
    }
    STDENC_RegAccessUnlock(DencHnd);
    return (VbiEc(Ec));
}  /* end of SetTtxOff() function */

/*******************************************************************************
Name        : SetTtxLinesOn
Description : Set teletext lines On on denc
Parameters  : IN : LineC : line count
 *            IN : LineMask : used only if LineC == 0. Select lines individually
 *            IN : Field : TRUE => ODD,  FALSE => EVEN
Parameters  : DencHnd : Handle on STDENC to reach DENC registers
Assumptions : Denc Handle has been checked
 *            if LineC=0, LineMask has been checked
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
static ST_ErrorCode_t SetTtxLinesOn(  const STDENC_Handle_t DencHnd
                                    , const HALVBI_DencCellId_t DencCellId
                                    , const U16 LineC
                                    , U32 LineMask
                                    , const BOOL Field
                                   )
{
    U8 reg, RegValue, Bitmask, i, first, last;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    if (LineC == 0) /* lines selected with LineMask */
    {
        first = TTX_FIRST_LINE_OFFSET;
        while ( ((LineMask>>first) & 0x01) == 0) first++;
        last = TTX_LAST_LINE_OFFSET;
        while ( ((LineMask>>last) & 0x01) == 0) last--;
    }
    else /* API LineMask not used */
    {
        first = TTX_FIRST_LINE_OFFSET;
        last  = first + LineC-1;
        LineMask = ((1<<LineC) -1) << TTX_FIRST_LINE_OFFSET;
    }

    if (DencCellId <= 6) /* denc V3 - V6 */
    {
        first -= TTX_FIRST_LINE_OFFSET;
        last  -= TTX_FIRST_LINE_OFFSET;

        /* Program block start and end points */
        STDENC_RegAccessLock(DencHnd);
        if (Field) /* Odd Field )*/
        {
            Ec = STDENC_WriteReg8(DencHnd, DENC_TTX1, (first << 4) | last );
        }
        else /* Even Field */
        {
            Ec = STDENC_WriteReg8(DencHnd, DENC_TTX2, (first << 4) | last);
        }
    }
    else /* denc V7 - V11 */
    {
        Ec = SetTtxOff(DencHnd, DencCellId, Field); /* don't lock before calling this function */
        STDENC_RegAccessLock(DencHnd);
        if (Field) /* Odd Field )*/
        {
            reg = TTX_ODD_REG;
            if (first <= 9) /* Reg 34 ends with line 9 */
            {
                Bitmask = (1 << TTX_ODD_FIRST_POS) >> (first - TTX_FIRST_LINE_OFFSET);
            }
            else
            {
                reg++;
                if (first <= 17) /* Reg 35 ends with line 17 */
                {
                    Bitmask = 0x80 >> (first - 10);  /* Reg 35 starts with line 10 */
                }
                else
                {
                    reg++;
                    Bitmask = 0x80 >> (first - 18);  /* Reg 36 starts with line 18 */
                }
            }
        }
        else /* Even Field */
        {
            reg = TTX_EVEN_REG;
            if (first <= 14) /* Reg 37 ends with line 327 */
            {
                Bitmask = 0x80 >> (first - TTX_FIRST_LINE_OFFSET);  /* Reg 37 starts with line 320 */
            }
            else
            {
                reg++;
                Bitmask = 0x80 >> (first - 15);  /* Reg 38 starts with line 328 */
            }
        }

        RegValue = 0;
        for (i = first; (i <= last) && (OK(Ec)); i++)
        {
            if ( ((LineMask >> i) & 0x01) == 1) /* this line must be set on */
            {
                RegValue |= Bitmask;
            }
            Bitmask >>= 1;
            if (i == last) /* last line : write reg value  */
            {
                if (RegValue != 0)
                {
                    Ec = STDENC_OrReg8(DencHnd, reg, RegValue);
                }
            }
            else if (Bitmask == 0) /* write reg value and go to next register */
            {
                Bitmask = 0x80;
                if (RegValue != 0)
                {
                    Ec = STDENC_OrReg8(DencHnd, reg, RegValue);
                    RegValue = 0;
                }
                reg++;
            }
        }
    }
    STDENC_RegAccessUnlock(DencHnd);
    return (VbiEc(Ec));
}  /* end of SetTtxLinesOn() function */


/*
******************************************************************************
Public Functions
******************************************************************************
*/


/*******************************************************************************
Name        : stvbi_HALDisableTeletextOnDenc
Description : Disable teletext feature on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALDisableTeletextOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    STDENC_RegAccessLock(DencHnd);
    if (Vbi_p->Device_p->DencCellId <= 6)
    {
        Ec = STDENC_WriteReg8(DencHnd, DENC_TTXM, 0x0);
    }
    else /* DencCellId > 6 */
    {
        Ec = STDENC_AndReg8(DencHnd, DENC_CFG6, (U8)~DENC_CFG6_TTX_ENA);
    }
    if (OK(Ec))
    {
        Ec = STDENC_AndReg8(DencHnd, DENC_CFG8, (U8)~DENC_CFG8_TTX_NOTMV);
    }
    STDENC_RegAccessUnlock(DencHnd);
    return (VbiEc(Ec));
} /* end of stvbi_HALDisableTeletextOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALEnableTeletextOnDenc
Description : Enable teletext feature on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALEnableTeletextOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    STDENC_RegAccessLock(DencHnd);
    if (Vbi_p->Device_p->DencCellId <= 6)
    {
        /* Enable block for odd field (FIELD1) */
        Ec = STDENC_OrReg8(DencHnd, DENC_TTXM, (1 << TTX_FIELD1_BLOCK1_POS) );
        /* Enable block for even field (FIELD2) */
        if (OK(Ec)) Ec = STDENC_OrReg8(DencHnd, DENC_TTXM, (1 << TTX_FIELD2_BLOCK2_POS) );
    }
    else /* DencCellId > 6 */
    {
        Ec = STDENC_OrReg8(DencHnd, DENC_CFG6, DENC_CFG6_TTX_ENA);
    }
    if (OK(Ec)) Ec = STDENC_OrReg8(DencHnd, DENC_CFG8, DENC_CFG8_TTX_NOTMV);
    STDENC_RegAccessUnlock(DencHnd);

    return (VbiEc(Ec));
} /* end of stvbi_HALEnableTeletextOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALSetTeletextConfigurationOnDenc
Description : Set teletext configuration parameters on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALSetTeletextConfigurationOnDenc( VBI_t * const Vbi_p)
{
    U8 OrMask;
    STDENC_Handle_t DencHnd;
    HALVBI_DencCellId_t DencCellId;
    STVBI_TeletextConfiguration_t * TtxConf_p;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    DencCellId = Vbi_p->Device_p->DencCellId;
    TtxConf_p = &(Vbi_p->Params.Conf.Type.TTX);

    /* set latency */
    STDENC_RegAccessLock(DencHnd);
    if (DencCellId <= 6) /* denc V3 - V6 */
    {
        OrMask  = (U8)(TtxConf_p->Latency - 2);
        Ec = STDENC_MaskReg8(DencHnd, DENC_CFG4, (U8)~DENC_CFG4_MASK_TXD, OrMask);
    }
    else  /* denc V7 - V11 */
    {
        OrMask  = (U8)((TtxConf_p->Latency - 2) << 4);
        Ec = STDENC_MaskReg8(DencHnd, DENC_TTXL1, (U8)~DENC_TTXL1_MASK_DEL, OrMask);
    }
    STDENC_RegAccessUnlock(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    /* set full Page */
    STDENC_RegAccessLock(DencHnd);
    if (DencCellId == 3)
    {
        Ec = UPDATE_REG(DencHnd, DENC_CFG6, DENC_CFG6_FP_TTXT, TtxConf_p->FullPage);
        if (OK(Ec)) Ec = STDENC_AndReg8(DencHnd, DENC_CFG6, (U8)~DENC_CFG6_TTX_ALL);
    }
    else if (DencCellId == 6) /* denc V5 : nothing to do */
    {
        Ec = UPDATE_REG(DencHnd, DENC_CFG8, DENC_CFG8_FP_TTXT, TtxConf_p->FullPage);
        if (OK(Ec)) Ec = STDENC_AndReg8(DencHnd, DENC_CFG8, (U8)~DENC_CFG8_FP_TTXT_ALL);
    }
    else  /* denc V7 - V11 */
    {
        Ec = UPDATE_REG(DencHnd, DENC_TTXL1, DENC_TTXL1_FULL_PAGE, TtxConf_p->FullPage);
    }
    STDENC_RegAccessUnlock(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    /* set amplitude */
    if (DencCellId >= 7) /* denc V7 - V11 (V3-V6 not applicable) */
    {
        STDENC_RegAccessLock(DencHnd);
        /* if amplitude is not 100IRE then it is 70IRE */
        Ec = UPDATE_REG(DencHnd, DENC_TTXCFG, DENC_TTXCFG_100IRE, (TtxConf_p->Amplitude == STVBI_TELETEXT_AMPLITUDE_100IRE));
        STDENC_RegAccessUnlock(DencHnd);
        if (!OK(Ec)) return (VbiEc(Ec));
    }

    /* set standard */
    if (DencCellId >= 7) /* denc V7 - V11 (V3-V6 not applicable) */
    {
        switch (TtxConf_p->Standard)
        {
            case STVBI_TELETEXT_STANDARD_A :
                OrMask = DENC_TTXCFG_STD_A;
                break;
            case STVBI_TELETEXT_STANDARD_B :
                OrMask = DENC_TTXCFG_STD_B;
                break;
            case STVBI_TELETEXT_STANDARD_C :
                OrMask = DENC_TTXCFG_STD_C;
                break;
            case STVBI_TELETEXT_STANDARD_D :
                OrMask = DENC_TTXCFG_STD_D;
                break;
            default :
                /* not handled : see assumptions */
                break;
        }
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_MaskReg8(DencHnd, DENC_TTXCFG, (U8)~DENC_TTXCFG_MASK_STD, OrMask);
        STDENC_RegAccessUnlock(DencHnd);
    }
    if (OK(Ec)) Ec = SetTtxOff( DencHnd, DencCellId, TRUE);  /* Field ODD  / Field1 */
    if (OK(Ec)) Ec = SetTtxOff( DencHnd, DencCellId, FALSE); /* Field EVEN / Field2 */
    if (OK(Ec)) Ec = stvbi_HALDisableTeletextOnDenc(Vbi_p);
    return (VbiEc(Ec));
} /* end of stvbi_HALSetTeletextConfigurationOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALSetTeletextDynamicParamsOnDenc
Description : Set teletext dynamic parameters on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALSetTeletextDynamicParamsOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    /* set Line and Field */
    Ec = SetTtxLinesOn(
            DencHnd,
            Vbi_p->Device_p->DencCellId,
            Vbi_p->Params.Dyn.Type.TTX.LineCount,
            Vbi_p->Params.Dyn.Type.TTX.LineMask,
            Vbi_p->Params.Dyn.Type.TTX.Field
            );
    if (!OK(Ec)) return (VbiEc(Ec));

    /* set mask */
    if (Vbi_p->Device_p->DencCellId >= 7) /* denc V7 - V11 (V3-V6 not applicable) */
    {
        STDENC_RegAccessLock(DencHnd);
        /* mask framing code enable : set bit to 0 */
        Ec = UPDATE_REG(DencHnd, DENC_DAC2, DENC_DAC2_TTX_MASK_OFF, (!(Vbi_p->Params.Dyn.Type.TTX.Mask)));
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALSetTeletextDynamicParamsOnDenc() function */

/* End of vbi_ttx.c */
