/**********************************************************************************

File Name   : denc_reg.c

Description : DENC function for register accesses

COPYRIGHT (C) STMicroelectronics 2000

Date               Modification                                             Name
----               ------------                                             ----
22 Feb 2001        Created                                                  HSdLM
22 Jun 2001        Rename functions so that they are compliant with         HSdLM
 *                 exported symbols rules : API level => STDENC_xxx
***********************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stdenc.h"
#include "denc_drv.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : STDENC_RegAccessLock
Description : lock register access for current device
Parameters  : Handle  : IN  : Handle on DENC driver connection
Assumptions : Handle is valid.
Limitations :
Returns     : none
*******************************************************************************/
void STDENC_RegAccessLock(STDENC_Handle_t Handle)
{
    HALDENC_RegAccessLock(((DENC_t *)Handle)->Device_p);
} /* End of STDENC_RegAccessLock() function */

/*******************************************************************************
Name        : STDENC_RegAccessUnlock
Description : unlock register access for current device
Parameters  : Handle  : IN  : Handle on DENC driver connection
Assumptions : Handle is valid.
Limitations :
Returns     : none
*******************************************************************************/
void STDENC_RegAccessUnlock(STDENC_Handle_t Handle)
{
    HALDENC_RegAccessUnlock(((DENC_t *)Handle)->Device_p);
} /* End of STDENC_RegAccessUnlock() function */

/*******************************************************************************
Name        : STDENC_ReadReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            Value_p : OUT : value read
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_ReadReg8(STDENC_Handle_t Handle, const U32 Addr, U8 * const Value_p)
{
    return(HALDENC_ReadReg8(((DENC_t *)Handle)->Device_p, Addr, Value_p));
} /* End of STDENC_ReadReg8() function */

/*******************************************************************************
Name        : STDENC_WriteReg8
Description : register access function
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            Value   : IN  : value to write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_WriteReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 Value)
{
    ST_ErrorCode_t Ec = ST_NO_ERROR;
    Ec = HALDENC_WriteReg8(((DENC_t *)Handle)->Device_p, Addr, Value);
    return(Ec);
} /* End of STDENC_WriteReg8() function */

/*******************************************************************************
Name        : STDENC_AndReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            AndMask : IN  : mask to apply to read value before write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_AndReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 AndMask)
{
    U8 Value;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    Ec = HALDENC_ReadReg8(((DENC_t *)Handle)->Device_p, Addr, &Value);
    if (Ec == ST_NO_ERROR)
    {
        Value &= AndMask;
        Ec = HALDENC_WriteReg8(((DENC_t *)Handle)->Device_p, Addr, Value);
    }
    return(Ec);
} /* End of STDENC_AndReg8() function */

/*******************************************************************************
Name        : STDENC_OrReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            OrMask : IN  : mask to apply to read value before write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_OrReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 OrMask)
{
    U8 Value;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    Ec = HALDENC_ReadReg8(((DENC_t *)Handle)->Device_p, Addr, &Value);
    if (Ec == ST_NO_ERROR)
    {
        Value |= OrMask;
        Ec = HALDENC_WriteReg8(((DENC_t *)Handle)->Device_p, Addr, Value);
    }
    return(Ec);
} /* End of STDENC_OrReg8() function */

/*******************************************************************************
Name        : STDENC_MaskReg8
Description : register access functions
Parameters  : Handle  : IN  : Handle on DENC driver connection
 *            Addr    : IN  : Register offset
 *            AndMask : IN  : mask to apply to read value before write
 *            OrMask : IN  : mask to apply to read value before write
Assumptions : Handle is valid.
Limitations :
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
ST_ErrorCode_t STDENC_MaskReg8(STDENC_Handle_t Handle, const U32 Addr, const U8 AndMask, const U8 OrMask)
{
    U8 Value;
    ST_ErrorCode_t Ec = ST_NO_ERROR;

    Ec = HALDENC_ReadReg8(((DENC_t *)Handle)->Device_p, Addr, &Value);
    if (Ec == ST_NO_ERROR)
    {
        Value &= AndMask;
        Value |= OrMask;
        Ec = HALDENC_WriteReg8(((DENC_t *)Handle)->Device_p, Addr, Value);
    }
    return(Ec);
} /* End of STDENC_MaskReg8() function */


/* ----------------------------- End of file ------------------------------ */

