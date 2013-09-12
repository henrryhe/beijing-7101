/*******************************************************************************
File name   : vbi_cc.c

Description : vbi closed caption on denc source file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                           JG
12 Dec 2000        write data in BOTH mode : use the 4 bytes         HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
04 Jul 2001        Update DENC register access functions names       HSdLM
 *                 Import dynamic parameters checking. Use constants.
14 Jun 2002        Improve 'Wait5ms' use                             HSdLM
02 Jan 2005        Support of OS21/ST40                              AC
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stdenc.h"
#include "stcommon.h"
#include "vbi_drv.h"
#include "ondenc.h"
#include "denc_reg.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#define MAX_FRAME_RATE_PERIOD 8 /* 1/25Hz = 40 ms = 8 x 5ms */

#define LINE_FIELD1_MIN                       7
#define LINE_FIELD1_MAX                       37
#define LINE_FIELD1_REG_OFFSET                6

#define LINE_FIELD2_525_60_MIN                273
#define LINE_FIELD2_525_60_MAX                284
#define LINE_FIELD2_525_60_SINGLE             289
#define LINE_FIELD2_525_60_REG_OFFSET         269
#define LINE_FIELD2_525_60_SINGLE_REG_OFFSET  31

#define LINE_FIELD2_625_50_MIN                319
#define LINE_FIELD2_625_50_MAX                336
#define LINE_FIELD2_625_50_SINGLE             349
#define LINE_FIELD2_625_50_REG_OFFSET         318
#define LINE_FIELD2_625_50_SINGLE_REG_OFFSET  31

/* Private Variables (static)------------------------------------------------ */
#if defined (ST_OS20) || defined (ST_OSLINUX)
static U32 Wait5ms = 0;
#endif
#ifdef ST_OS21
static osclock_t Wait5ms = 0;
#endif

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

#define IS_LINE_F2_525_60_VALID(line) (   (    ((line) >= LINE_FIELD2_525_60_MIN)     \
                                            && ((line) <= LINE_FIELD2_525_60_MAX))    \
                                       || ((line) == LINE_FIELD2_525_60_SINGLE))

#define IS_LINE_F2_625_50_VALID(line) (   (    ((line) >= LINE_FIELD2_625_50_MIN)     \
                                            && ((line) <= LINE_FIELD2_625_50_MAX))    \
                                       || ((line) == LINE_FIELD2_625_50_SINGLE))


/* Private Function prototypes ---------------------------------------------- */

static ST_ErrorCode_t WaitCCBufferFreed(const STDENC_Handle_t DencHnd, const U8 Mask);

/* Private Functions -------------------------------------------------------- */

/*******************************************************************************
Name        : WaitCCBufferFreed
Description : Delay CC write until CC buffer is freed.
Parameters  : IN : DencHnd : Handle of DENC to program
 *            IN : Mask : Mask to read wanted buffer status in DENC status byte.
Assumptions : DencHnd is OK, Mask is OK.
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
static ST_ErrorCode_t WaitCCBufferFreed(const STDENC_Handle_t DencHnd, const U8 Mask)
{
    U8 Value, TimeOutIndex = 0;
    ST_ErrorCode_t Ec = ST_NO_ERROR;
#ifdef ST_OSLINUX
    BOOL DencStat = FALSE;
#endif
    STDENC_RegAccessLock(DencHnd);
    Ec = STDENC_ReadReg8(DencHnd, DENC_STA, &Value);
    STDENC_RegAccessUnlock(DencHnd);

#if defined(ST_OSLINUX)
    if (OK(Ec))
    {
       while ( !DencStat )
       {
            TimeOutIndex = MAX_FRAME_RATE_PERIOD;
            while ( ((Value & Mask) == 0) && (TimeOutIndex != 0) && (OK(Ec)))
            {
                STDENC_RegAccessLock(DencHnd);
                Ec = STDENC_ReadReg8(DencHnd, DENC_STA, &Value);
                STDENC_RegAccessUnlock(DencHnd);
                TimeOutIndex--;
            }
            DencStat = TRUE;
            if (((Value & Mask) == 0) && (TimeOutIndex == 0) && (OK(Ec)))
            {
                DencStat = FALSE ;
            }
      }

    }
#else
    if (OK(Ec))
    {
        TimeOutIndex = MAX_FRAME_RATE_PERIOD;
        while ( ((Value & Mask) == 0) && (TimeOutIndex != 0) && (OK(Ec)))
        {
            STOS_TaskDelay(Wait5ms);
            STDENC_RegAccessLock(DencHnd);
            Ec = STDENC_ReadReg8(DencHnd, DENC_STA, &Value);
            STDENC_RegAccessUnlock(DencHnd);
            TimeOutIndex--;
        }

        if (((Value & Mask) == 0) && (TimeOutIndex == 0) && (OK(Ec)))
        {
            Ec = STVBI_ERROR_DENC_ACCESS;
        }

    }

#endif
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Timeout : %d    Status : %0x", TimeOutIndex, Value));
    return (VbiEc(Ec));
}

/* Public Functions --------------------------------------------------------- */

/*******************************************************************************
Name        : stvbi_HALCheckClosedCaptionDynamicParamsOnDenc
Description : check if parameters given are valid.
Parameters  : IN : Vbi_p : device data access
 *            IN : DynamicParams_p : parameters to be checked
Assumptions : Vbi_p and DynamicParams_p are not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvbi_HALCheckClosedCaptionDynamicParamsOnDenc( const VBI_t * const Vbi_p, const STVBI_DynamicParams_t * const DynamicParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U16 LinesF2;

    switch ( DynamicParams_p->Type.CC.Mode)
    {
        case STVBI_CCMODE_NONE :
            break;
        case STVBI_CCMODE_BOTH :
        case STVBI_CCMODE_FIELD1 :
            if (   (    (Vbi_p->Params.Conf.Type.CC.Mode != STVBI_CCMODE_BOTH)
                     && (Vbi_p->Params.Conf.Type.CC.Mode != STVBI_CCMODE_FIELD1))
                || (DynamicParams_p->Type.CC.LinesField1 < LINE_FIELD1_MIN)
                || (DynamicParams_p->Type.CC.LinesField1 > LINE_FIELD1_MAX) )
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            if (DynamicParams_p->Type.CC.Mode == STVBI_CCMODE_FIELD1)
            {
                break;
            }
        case STVBI_CCMODE_FIELD2 :
            LinesF2 = DynamicParams_p->Type.CC.LinesField2;
            if (    (    (Vbi_p->Params.Conf.Type.CC.Mode != STVBI_CCMODE_BOTH)
                      && (Vbi_p->Params.Conf.Type.CC.Mode != STVBI_CCMODE_FIELD2))
                || !(IS_LINE_F2_525_60_VALID(LinesF2) || IS_LINE_F2_625_50_VALID(LinesF2)))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* end switch ( DynamicParams_p->Type.CC.Mode) */
    return (ErrorCode);
} /* End of stvbi_HALCheckClosedCaptionDynamicParamsOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALDisableClosedCaptionOnDenc
Description : Disable closed caption on Denc
Parameters  : IN : Vbi_p : device data access
Assumptions : Vbi_p is not NULL and ok.
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALDisableClosedCaptionOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (OK(Ec))
    {
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_AndReg8(DencHnd, DENC_CFG1, DENC_CFG1_MASK_CC);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* End of stvbi_HALDisableClosedCaptionOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALEnableClosedCaptionOnDenc
Description : Enable closed caption feature on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALEnableClosedCaptionOnDenc( VBI_t * const Vbi_p)
{
    U8 OrMask;
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    if (Wait5ms == 0) /* not yet initialized */
    {
        Wait5ms = ST_GetClocksPerSecond()/200 + 1; /* round by excess */
    }
    switch (Vbi_p->Params.Conf.Type.CC.Mode)
    {
        case STVBI_CCMODE_FIELD1 :
            OrMask = DENC_CFG1_CC_ENA_F1;
            break;
        case STVBI_CCMODE_FIELD2 :
            OrMask = DENC_CFG1_CC_ENA_F2;
            break;
        case STVBI_CCMODE_BOTH :
            OrMask = DENC_CFG1_CC_ENA_BOTH;
            break;
        case STVBI_CCMODE_NONE : /* no break */
        default :
            /* not handled : see assumptions */
            return (ST_NO_ERROR);
            break;
    }

    DencHnd = Vbi_p->Device_p->DencHandle;
    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    STDENC_RegAccessLock(DencHnd);
    Ec = STDENC_MaskReg8(DencHnd, DENC_CFG1, DENC_CFG1_MASK_CC, OrMask);
    STDENC_RegAccessUnlock(DencHnd);
    return (VbiEc(Ec));
} /* End of stvbi_HALEnableClosedCaptionOnDenc() function */

/*******************************************************************************
Name        : stvbi_HALSetClosedCaptionDynamicParamsOnDenc
Description : Set closed caption dynamic parameters on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALSetClosedCaptionDynamicParamsOnDenc( VBI_t * const Vbi_p)
{
    U8 Value =0;
    U16 LineF2;
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    STDENC_RegAccessLock(DencHnd);
    switch (Vbi_p->Params.Dyn.Type.CC.Mode)
    {
        case STVBI_CCMODE_BOTH :
        case STVBI_CCMODE_FIELD1 :
            Value = (U8)Vbi_p->Params.Dyn.Type.CC.LinesField1 - LINE_FIELD1_REG_OFFSET;
            Ec = STDENC_WriteReg8(DencHnd, DENC_CFL1, Value);
            if (( Vbi_p->Params.Dyn.Type.CC.Mode == STVBI_CCMODE_FIELD1) || (!OK(Ec)))
            {
                break;
            }
        case STVBI_CCMODE_FIELD2 :
            LineF2 = Vbi_p->Params.Dyn.Type.CC.LinesField2;
            if ( LineF2 >= LINE_FIELD2_525_60_MIN && LineF2 <= LINE_FIELD2_525_60_MAX )
            {
                Value = (U8)(LineF2 - LINE_FIELD2_525_60_REG_OFFSET);
            }
            if ( LineF2 == LINE_FIELD2_525_60_SINGLE )
            {
                Value = (U8)LINE_FIELD2_525_60_SINGLE_REG_OFFSET;
            }
            if ( LineF2 >= LINE_FIELD2_625_50_MIN && LineF2 <= LINE_FIELD2_625_50_MAX )
            {
                Value = (U8)(LineF2 - LINE_FIELD2_625_50_REG_OFFSET);
            }
            if ( LineF2 == LINE_FIELD2_625_50_SINGLE )
            {
                Value = (U8)LINE_FIELD2_625_50_SINGLE_REG_OFFSET;
            }
            Ec = STDENC_WriteReg8(DencHnd, DENC_CFL2, Value);
            break;
        case STVBI_CCMODE_NONE : /* no break */
        default : /* not handled, see assumptions */
            break;
    }
    STDENC_RegAccessUnlock(DencHnd);
    return (VbiEc(Ec));
} /* End of stvbi_HALSetClosedCaptionDynamicParamsOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALWriteClosedCaptionDataOnDenc
Description : Write closed caption data on Denc
Parameters  : IN : Vbi_p : device to write into
 *            IN : Data_p : buffer of values to write
Assumptions : Vbi_p is not NULL and OK
 *            Data_p is not NULL and point a buffer of good length
 *            Vbi_p->Params.Dyn.Type.CC.Mode is OK
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALWriteClosedCaptionDataOnDenc(VBI_t * const Vbi_p, const U8 * const Data_p)
{
    U8 cfg1, byte1, byte2;
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    byte1 = Data_p[0];
    byte2 = Data_p[1];

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    STDENC_RegAccessLock(DencHnd);
    Ec = STDENC_ReadReg8(DencHnd, DENC_CFG1, &cfg1);
    STDENC_RegAccessUnlock(DencHnd);
    if (!OK(Ec))
    {
        return (VbiEc(Ec));
    }

    switch (Vbi_p->Params.Dyn.Type.CC.Mode)
    {
        case STVBI_CCMODE_BOTH :
        case STVBI_CCMODE_FIELD1 :
            if ( (cfg1 & DENC_CFG1_CC_ENA_F1) != 0) /* Closed caption enabled in field 1 */
            {
                Ec = WaitCCBufferFreed(DencHnd, DENC_STA_BUF1_FREE);
            }
            if (Ec == ST_NO_ERROR)
            {
                STDENC_RegAccessLock(DencHnd);
                Ec = STDENC_WriteReg8(DencHnd, DENC_CC11, byte1);
                if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_CC12, byte2);
                STDENC_RegAccessUnlock(DencHnd);
            }
            if (( Vbi_p->Params.Dyn.Type.CC.Mode == STVBI_CCMODE_FIELD1) || (!OK(Ec)))
            {
                break;
            }
        case STVBI_CCMODE_FIELD2 :
            if ( Vbi_p->Params.Dyn.Type.CC.Mode == STVBI_CCMODE_BOTH)
            {
                byte1 = Data_p[2];
                byte2 = Data_p[3];
            }
            if ( (cfg1 & DENC_CFG1_CC_ENA_F2) != 0) /* Closed caption enabled in field 2 */
            {
                Ec = WaitCCBufferFreed(DencHnd, DENC_STA_BUF2_FREE);
            }
            if (Ec == ST_NO_ERROR)
            {
                STDENC_RegAccessLock(DencHnd);
                Ec = STDENC_WriteReg8(DencHnd, DENC_CC21, byte1);
                if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_CC22, byte2);
                STDENC_RegAccessUnlock(DencHnd);
            }
            break;
        case STVBI_CCMODE_NONE : /* no break */
        default : /* not handled, see assumptions */
            break;
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALWriteClosedCaptionDataOnDenc() function */

/* End of vbi_cc.c */
