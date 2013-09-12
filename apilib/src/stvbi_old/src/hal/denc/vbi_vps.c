/*******************************************************************************
File name   : vbi_vps.c

Description : vbi hal source file for VPS use, on denc

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                           JG
12 Dec 2000        Improve error tracking                            HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
 *                 Fix error writing data
04 Jul 2001        Update DENC register access functions names       HSdLM
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


/*
    ETS 300 231
    data format of the programm delivery data in the dedicated TV line :
*/
/*---------------------------------------------------
 * VBI   Reg    bit  disable enable     remarks
 * -------------------------------------------------
 * VPS   Cfg7     1      0      1   denc cell V6-V11
 * -------------------------------------------------*/

/*******************************************************************************
Name        : stvbi_HALDisableVpsOnDenc
Description : disable VPS feature on Denc
Parameters  : IN : Vbi_p : device info
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALDisableVpsOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    if (Vbi_p->Device_p->DencCellId >= 6) /* denc V6 - V11 (not applicable for V3 - V5 */
    {
        Ec = STDENC_CheckHandle(DencHnd);
        if (OK(Ec))
        {
            STDENC_RegAccessLock(DencHnd);
            Ec = STDENC_AndReg8(DencHnd, DENC_CFG7, (U8)~DENC_CFG7_ENA_VPS);
            STDENC_RegAccessUnlock(DencHnd);
        }
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALDisableVpsOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALEnableVpsOnDenc
Description : enable VPS feature on Denc
Parameters  : IN : Vbi_p : device info
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALEnableVpsOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    if (Vbi_p->Device_p->DencCellId >= 6) /* denc V6 - V11 (not applicable for V3 - V5 */
    {
        Ec = STDENC_CheckHandle(DencHnd);
        if (OK(Ec))
        {
            STDENC_RegAccessLock(DencHnd);
            Ec = STDENC_OrReg8(DencHnd, DENC_CFG7, DENC_CFG7_ENA_VPS);
            STDENC_RegAccessUnlock(DencHnd);
        }
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALEnableVpsOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALWriteVpsDataOnDenc
Description : write VPS data on Denc
Parameters  : IN : Vbi_p : device info
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALWriteVpsDataOnDenc( VBI_t * const Vbi_p, const U8* const Data_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    if (Vbi_p->Device_p->DencCellId >= 6) /* denc V6 - V11 (not applicable for V3 - V5 */
    {
        Ec = STDENC_CheckHandle(DencHnd);
        if (!OK(Ec)) return (VbiEc(Ec));
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_MaskReg8(DencHnd, DENC_VPS0, (U8)~DENC_VPS0_MASK, Data_p[0] & DENC_VPS0_MASK);
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_VPS1, Data_p[1]);
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_VPS2, Data_p[2]);
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_VPS3, Data_p[3]);
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_VPS4, Data_p[4]);
        if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_VPS5, Data_p[5]);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALWriteVpsDataOnDenc() function */

/* End of vbi_vps.c */
