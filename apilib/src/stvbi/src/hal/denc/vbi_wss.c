/*******************************************************************************
File name   : vbi_wss.c

Description : vbi hal source file for WSS use, on denc

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                           JG
12 Dec 2000        Improve error tracking                            HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
04 Jul 2001        Update DENC register access functions names       HSdLM
 *                 Fix error writing data
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stdenc.h"
#include "vbi_drv.h"
#include "ondenc.h"
#include "denc_reg.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*---------------------------------------------------
 * VBI   Reg    bit  disable enable     remarks
 * -------------------------------------------------
 * WSS   Cfg3     0      0      1   denc cell V6-V11
 * -------------------------------------------------*/
/*----------------------------------------------------------------------------
    WSS status bits transmission scheme (ETS 300 294) :
    b 00 01 02 03 => format (b3=oddparity)
                     (0 0 0 1)=full format 4:3
                     (1 0 0 0)=box 14:9 centre
                     (0 1 0 0)=box 14:9 top
                     (1 1 0 1)=box 16:9 centre
                     (0 0 1 0)=box 16:9 top
                     (1 0 1 1)=box > 16:9 centre
                     (0 1 1 1)=full format 4:3 (shoot and protect 14:9 centre)
                     (1 1 1 0)=full format 16:9 (anamorphic)
    b 04          => 0=camera mode / 1=film mode
    b 05          => 0=standard coding / 1=motion adaptative colour plus
    b 06          => 0=no helper / 1=modulated helper
    b 07          => 0
    b 08          => 0=no subtitles within teletext / 1=subtitles within teletext
    b 09 10       => (0 0)=no open subtitles
                     (1 0)=subtitles in active image area
                     (0 1)=subtitles out of active image area
                     (1 1)=reserved
    b 11          => 0= no surround sound information / 1= surround sound information
    b 12          => 0=no copy right asserted or status unknown / 1= copy right asserted
    b 13          => 0=copying not restricted / 1=copying restricted
    Transmitted words :
        <------byte1----------> <-------byte0--------->
    b : 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
    bits programmed :
    Wss_bit_15_8 (@ DENC_WSS1_MASK) wss15 wss14 wss13 wss12 wss11 wss10 wss09 wss08
                                      0     0     0    b4    b3    b2    b1    b0
    Wss_bit_7_0  (@ DENC_WSS0_MASK) wss07 wss06 wss05 wss04 wss03 wss02 wss01 wss01
                                      0     0     0    b10   b9    b8     0     0
----------------------------------------------------------------------------*/

/*******************************************************************************
Name        : stvbi_HALDisableWssOnDenc
Description : disable WSS feature on Denc
Parameters  : IN : Vbi_p : device info
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALDisableWssOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    if (Vbi_p->Device_p->DencCellId >= 6) /* denc V6 - V11 (not applicable for V3 - V5 */
    {
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_AndReg8(DencHnd, DENC_CFG3, (U8)~DENC_CFG3_ENA_WSS);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALDisableWssOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALEnableWssOnDenc
Description : enable WSS feature on Denc
Parameters  : IN : Vbi_p : device info
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALEnableWssOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    if (Vbi_p->Device_p->DencCellId >= 6) /* denc V6 - V11 (not applicable for V3 - V5 */
    {
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_OrReg8(DencHnd, DENC_CFG3, DENC_CFG3_ENA_WSS);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALEnableWssOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALWriteWssDataOnDenc
Description : write WSS data on Denc
Parameters  : IN : Vbi_p : device info
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALWriteWssDataOnDenc( VBI_t * const Vbi_p, const U8* const Data_p)
{
    U8 Value;
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));

    if (Vbi_p->Device_p->DencCellId >= 6) /* denc V6 - V11 (not applicable for V3 - V5 */
    {
        STDENC_RegAccessLock(DencHnd);
        Value = Data_p[0] & DENC_WSS1_MASK;
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_WSS1, Value);
        Value = Data_p[1] & DENC_WSS0_MASK;
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_WSS0, Value);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALWriteWssDataOnDenc() function */

/* End of vbi_wss.c */
