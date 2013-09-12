/*******************************************************************************

File name   : extvi_pr.h

Description : Extvi_pri header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
04 december 00        Created                                        BD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __EXTVI_PRI_H
#define __EXTVI_PRI_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sti2c.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define EXTVIN_SAA7114REG_MAX    0xF0
#define EXTVIN_SAA7111REG_MAX    0xF0
#define EXTVIN_TDA8752REG_MAX    0xF0
#define EXTVIN_STV2310REG_MAX    0x86


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t EXTVIN_SAA7114Init(STI2C_Handle_t I2cHandle);
ST_ErrorCode_t EXTVIN_SAA7111Init(STI2C_Handle_t I2cHandle);
ST_ErrorCode_t EXTVIN_TDA8752Init(STI2C_Handle_t I2cHandle);
ST_ErrorCode_t EXTVIN_SelectSAA7114Input(STI2C_Handle_t I2cHandle,STEXTVIN_VipInputSelection_t Selection);
ST_ErrorCode_t EXTVIN_SelectSAA7111Input(STI2C_Handle_t I2cHandle,STEXTVIN_VipInputSelection_t Selection);
ST_ErrorCode_t EXTVIN_SelectVipOutput(STI2C_Handle_t I2cHandle,STEXTVIN_VipOutputSelection_t Selection);
ST_ErrorCode_t EXTVIN_SetSAA7114Standard(STI2C_Handle_t I2cHandle,STEXTVIN_VipStandard_t Standard);
ST_ErrorCode_t EXTVIN_SetSAA7111Standard(STI2C_Handle_t I2cHandle,STEXTVIN_VipStandard_t Standard);
ST_ErrorCode_t EXTVIN_GetSAA7114Status(STI2C_Handle_t I2cHandle,U8* Status);
ST_ErrorCode_t EXTVIN_GetSAA7111Status(STI2C_Handle_t I2cHandle,U8* Status);
ST_ErrorCode_t EXTVIN_SAA7114SoftReset (STI2C_Handle_t  I2cHandle);

ST_ErrorCode_t EXTVIN_SetAdcInputFormat(STI2C_Handle_t I2cHandle,STEXTVIN_AdcInputFormat_t Format);
ST_ErrorCode_t EXTVIN_WriteI2C(STI2C_Handle_t I2cHandle, U8 Address, U8 Value);
ST_ErrorCode_t EXTVIN_ReadI2C (STI2C_Handle_t  I2cHandle, U8 SubAddress, U8 *Value_p);

ST_ErrorCode_t EXTVIN_STV2310Init(STI2C_Handle_t I2cHandle);
ST_ErrorCode_t EXTVIN_SelectSTV2310Input(STI2C_Handle_t I2cHandle,STEXTVIN_VipInputSelection_t Selection);
ST_ErrorCode_t EXTVIN_SetSTV2310Standard(STI2C_Handle_t I2cHandle,STEXTVIN_VipStandard_t Standard);
ST_ErrorCode_t EXTVIN_GetSTV2310Status(STI2C_Handle_t I2cHandle,U8* Status);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __EXTVI_PRI_H */

/* End of extvi_pr.h */
