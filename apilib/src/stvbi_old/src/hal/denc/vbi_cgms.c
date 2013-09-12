/*******************************************************************************
File name   : vbi_cgms.c

Description : vbi hal source file for CGMS use, on Denc

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Jul 2000        Created                                           JG
12 Dec 2000        Improve error tracking                            HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
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


/*---------------------------------------------------
 * VBI   Reg    bit  disable enable     remarks
 * -------------------------------------------------
 * CGMS  Cfg3     5      0      1   denc cell V3-V11
 * -------------------------------------------------*/

/*******************************************************************************
Name        : stvbi_HALDisableCgmsOnDenc
Description : Disable CGMS feature on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALDisableCgmsOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (OK(Ec))
    {
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_AndReg8(DencHnd, DENC_CFG3, (U8)~DENC_CFG3_ENA_CGMS);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALDisableCgmsOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALEnableCgmsOnDenc
Description : Enable CGMS feature on Denc
Parameters  : IN : Vbi_p : device access
Assumptions : Vbi_p is not NULL and ok
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALEnableCgmsOnDenc( VBI_t * const Vbi_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (OK(Ec))
    {
        STDENC_RegAccessLock(DencHnd);
        Ec = STDENC_OrReg8(DencHnd, DENC_CFG3, DENC_CFG3_ENA_CGMS);
        STDENC_RegAccessUnlock(DencHnd);
    }
    return (VbiEc(Ec));
} /* end of stvbi_HALEnableCgmsOnDenc() function */


/*******************************************************************************
Name        : stvbi_HALWriteCgmsDataOnDenc
Description : Write CGMS data on Denc
Parameters  : IN : Vbi_p : device access
 *            IN : Data_p : data to write
Assumptions : Vbi_p is not NULL and ok, Data_p not NULL and point on a buffer
 *            which has good length
Limitations :
Returns     : ST_NO_ERROR, STVBI_ERROR_DENC_ACCESS
*******************************************************************************/
ST_ErrorCode_t stvbi_HALWriteCgmsDataOnDenc( VBI_t * const Vbi_p, const U8* const Data_p)
{
    STDENC_Handle_t DencHnd;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    DencHnd = Vbi_p->Device_p->DencHandle;

    Ec = STDENC_CheckHandle(DencHnd);
    if (!OK(Ec)) return (VbiEc(Ec));
    STDENC_RegAccessLock(DencHnd);
    Ec = STDENC_WriteReg8(DencHnd, DENC_CGMS2, Data_p[0]);
    if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_CGMS1, Data_p[1]);
    if (OK(Ec)) Ec = STDENC_WriteReg8(DencHnd, DENC_CGMS0, Data_p[2]);
    STDENC_RegAccessUnlock(DencHnd);
    return (VbiEc(Ec));
} /* end of stvbi_HALWriteCgmsDataOnDenc() function */

/* End of vbi_cgms.c */
