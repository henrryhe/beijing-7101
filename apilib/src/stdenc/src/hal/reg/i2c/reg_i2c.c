/************************************************************************

File Name   : reg_i2c.c

Description : DENC register access functions by I2C

COPYRIGHT (C) STMicroelectronics 2001

Date               Modification                                     Name
----               ------------                                     ----
06 Feb 2001        Created                                          HSdLM
07 Mar 2001        Add DENCRA_ before function table names and      HSdLM
 *                 routines to return function table addresses
 *                 Use STI2C_Write() instead of STI2C_WriteNoStop
 *                 Test nb of bytes actually read / wrote.
22 Jun 2001        Add parenthesis around macro argument            HSdLM
 *                 Exported symbols => stdenc_...                   HSdLM
 *                 To prevent issues with endianness, 16, 24 and
 *                 32 bits access routines have been removed.
*******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#include <stdio.h>
#include "stlite.h"
#include "sttbx.h"
#include "denc_drv.h"
#include "denc_ra.h"
#include "reg_i2c.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants ------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Private Macros ---------------------------------------------------------- */

#define GetI2CHandle(Device_p) (((const DENCDevice_t * const)(Device_p))->I2CHandle)

/* Private Function prototypes ---------------------------------------------- */
static ST_ErrorCode_t I2C_ReadReg8(const void * const Device_p, const U32 Addr_p, U8 * const Value_p);
static ST_ErrorCode_t I2C_WriteReg8(const void * const Device_p, const U32 Addr, const U8 Value);

/* Global Variables --------------------------------------------------------- */

/* Function pointer tables.*/

const stdenc_RegFunctionTable_t stdenc_I2CFunctionTable = {
    I2C_ReadReg8,
    I2C_WriteReg8
};


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : ReadI2C
Description : read data through I2C
Parameters  : IN  : I2CHandle : reach I2C driver
              IN  : Address : where read
              OUT : Data_p : single byte read
Assumptions : all parameters are ok
Limitations : 8 bits addressing register offsets, 8 bits value read (only a byte)
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
static ST_ErrorCode_t ReadI2C (const STI2C_Handle_t I2CHandle, const U8 Address, U8 * const Data_p)
{
    ST_ErrorCode_t  ErrorCode;
    U8              SubAddress[1];  /* parameter 'Address' is U8 */
    U32             AddressLen = 1; /* parameter 'Address' is U8 */
    U32             MaxLen = 1;     /* read a byte */
    U32             ReadTimeout = 10;
    U32             WriteTimeout = 10;
    U32             ActLen;

    /* Write sub-address to bus and lock bus to this handle */
    SubAddress[0] = Address;
    /* See DDTS GNBvd06615 : "Error returned by STDENC_Init() on external DENC"
     *  STI2C_Read() returns an error if STI2C_WriteNoStop is used
    */
    ErrorCode = STI2C_Write( I2CHandle, SubAddress, AddressLen, WriteTimeout, &ActLen );
    if ( (ErrorCode != ST_NO_ERROR) || (ActLen != AddressLen))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "ERROR: STI2C_Write returned code %d and wrote %d bytes\n", ErrorCode, ActLen));
        ErrorCode = STDENC_ERROR_I2C;
    }
    else
    {
        /* Read data from device and release bus lock */
        ErrorCode = STI2C_Read (I2CHandle, Data_p, MaxLen, ReadTimeout, &ActLen);
        if ( (ErrorCode != ST_NO_ERROR) || (ActLen != MaxLen))
        {
            STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "ERROR: STI2C_read returned code %d and read %d bytes\n", ErrorCode, MaxLen));
            ErrorCode = STDENC_ERROR_I2C;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STI2C_Read read %d bytes\n", ActLen ));
        }
    }
    return(ErrorCode);
} /* end of ReadI2C */


/*******************************************************************************
Name        : WriteI2C
Description : write a byte through I2C
Parameters  : IN  : I2CHandle : reach I2C driver
              IN  : Address : where write
              IN  : Data : byte to write
Assumptions : all parameters are ok
Limitations : 8 bits addressing register offsets, 8 bits value written (only a byte)
Returns     : ST_NO_ERROR, STDENC_ERROR_I2C
*******************************************************************************/
static ST_ErrorCode_t WriteI2C (const STI2C_Handle_t I2CHandle, const U8 Address, const U8 Data)
{
    ST_ErrorCode_t  ErrorCode;
    U8              LocalBuff[2];
    U32             NumberToWrite = 2;
    U32             WriteTimeout = 10;
    U32             ActLen;

    LocalBuff[0] = Address;
    LocalBuff[1] = Data;

    ErrorCode = STI2C_Write( I2CHandle, LocalBuff, NumberToWrite, WriteTimeout, &ActLen );
    if (( ErrorCode != ST_NO_ERROR) || (ActLen != NumberToWrite))
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL, "ERROR: STI2C_Write returned code %d and wrote %d bytes\n", ErrorCode, ActLen));
        ErrorCode = STDENC_ERROR_I2C;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STI2C_Write wrote %d bytes\n", ActLen ));
    }
    return(ErrorCode);
} /* end of WriteI2C */


static ST_ErrorCode_t I2C_ReadReg8(const void * const Device_p, const U32 Addr, U8 * const Value_p)
{
    return(ReadI2C(GetI2CHandle(Device_p), (const U8)Addr, Value_p));
}

static ST_ErrorCode_t I2C_WriteReg8(const void * const Device_p, const U32 Addr, const U8 Value)
{
    return(WriteI2C(GetI2CHandle(Device_p), (const U8)Addr, Value));
}

/* Public functions                                                         */

/*******************************************************************************
Name        : Dencra_I2CGetFunctionTableAddress
Description : return address of stdenc_I2CFunctionTable
Parameters  : none
Assumptions :
Limitations :
Returns     : address of stdenc_I2CFunctionTable
*******************************************************************************/
const stdenc_RegFunctionTable_t * stdenc_GetI2CFunctionTableAddress(void)
{
    return(&stdenc_I2CFunctionTable);
}

/* ----------------------------- End of file ------------------------------ */

