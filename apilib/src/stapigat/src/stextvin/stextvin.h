/*******************************************************************************

File name   : stextvin.h

Description : EXTVIN module header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
28 Nov 2000        Created                                          BD
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STEXTVIN_H
#define __STEXTVIN_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stlite.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STEXTVIN_DRIVER_ID    172
#define STEXTVIN_DRIVER_BASE  (STEXTVIN_DRIVER_ID << 16)

enum
{
    STEXTVIN_ERROR_FUNCTION_NOT_IMPLEMENTED = STEXTVIN_DRIVER_BASE,
    STEXTVIN_ERROR_INVALID_STANDARD_TYPE,
    STEXTVIN_HW_FAILURE,
    STEXTVIN_ERROR_INVALID_INPUT_TYPE,
    STEXTVIN_ERROR_INVALID_OUTPUT_TYPE,
    STEXTVIN_ERROR_I2C
};


/* Exported Types ----------------------------------------------------------- */



/* STEXTVIN handle could be any type of handle */
typedef void * STEXTVIN_Handle_t;

typedef enum
{
    STEXTVIN_NTSC_640_480,      /* NTSC  sqr  , 640x480  , 15.734kHz, 12.273 MHz  */
    STEXTVIN_VGA_640_480,       /* VGA        , 640x480  , 31.50 kHz, 25.200 MHz. */
    STEXTVIN_NTSC_720_480,      /* NTSC       , 720x480  , 15.734kHz, 13.500 MHz  */
    STEXTVIN_SVGA_800_600,      /* SVGA  72 Hz, 800x600  , 48.08 kHz, 50.000 MHz  */
    STEXTVIN_XGA_1024_768,      /* XVGA  75 Hz, 1024x768 , 60.02 kHz, 78.800 MHz  */
    STEXTVIN_SUN_1152_900,      /* SUN        , 1152x900 , 66.67 kHz, 100.00 MHz  */
    STEXTVIN_HD_1280_720,       /* EIA770.3   , 1280x720 , 45.00 kHz, 74.250 MHz  */
    STEXTVIN_SXGA_1280_1024,    /* SXVGA 60 Hz, 1280x1024, 64.00 kHz, 108.00 MHz  */
    STEXTVIN_HD_1920_1080       /* SMPTE274   , 1920x1080, 67.50 kHz, 74.250 MHz  */
}   STEXTVIN_AdcInputFormat_t;


typedef enum
{
	STEXTVIN_SAA7114,
    STEXTVIN_DB331,
    STEXTVIN_TDA8752,
    STEXTVIN_STV2310
}   STEXTVIN_DeviceType_t;


typedef enum
{
	STEXTVIN_VIPSTANDARD_UNDEFINED,
	STEXTVIN_VIPSTANDARD_PAL_BGDHI,
	STEXTVIN_VIPSTANDARD_NTSC_M,
	STEXTVIN_VIPSTANDARD_PAL4,
	STEXTVIN_VIPSTANDARD__NTSC4_50,
	STEXTVIN_VIPSTANDARD_PALN,
	STEXTVIN_VIPSTANDARD_NTSC4_60,
	STEXTVIN_VIPSTANDARD_NTSC_N,
	STEXTVIN_VIPSTANDARD_PAL_M,
	STEXTVIN_VIPSTANDARD_NTSC_JAPAN,
	STEXTVIN_VIPSTANDARD_SECAM
}   STEXTVIN_VipStandard_t;

typedef enum
{
	STEXTVIN_VideoComp,
	STEXTVIN_VideoYc,
	STEXTVIN_CCIR656
}   STEXTVIN_VipInputSelection_t;

typedef enum
{
	STEXTVIN_ITU656,
	STEXTVIN_NoITU656
}   STEXTVIN_VipOutputSelection_t;

typedef struct
{
    STEXTVIN_VipStandard_t     SupportedVipStandard;
}   STEXTVIN_Capability_t;





typedef struct
{
    ST_Partition_t*                     CPUPartition_p;
	U32									MaxOpen;
    STEXTVIN_DeviceType_t               DeviceType;
	ST_DeviceName_t						I2CDeviceName;
    U8                                  I2CAddress;
}   STEXTVIN_InitParams_t;


typedef struct
{
    U32 NotUsed;
} STEXTVIN_UnitPublicParams_t;



typedef struct
{
    U32 NotUsed;
    STEXTVIN_UnitPublicParams_t UnitPublicParams;
} STEXTVIN_OpenParams_t;


/*
typedef struct
{
	U8 NotUsed;
} STEXTVIN_OpenParams_t;
*/

typedef struct
{
    BOOL   ForceTerminate;
} STEXTVIN_TermParams_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_Revision_t STEXTVIN_GetRevision(void);
ST_ErrorCode_t STEXTVIN_GetCapability(const ST_DeviceName_t DeviceName, STEXTVIN_Capability_t * const Capability_p);
ST_ErrorCode_t STEXTVIN_Init(const ST_DeviceName_t DeviceName, const STEXTVIN_InitParams_t * const InitParams_p);
ST_ErrorCode_t STEXTVIN_Open(const ST_DeviceName_t DeviceName, const STEXTVIN_OpenParams_t * const OpenParams_p, STEXTVIN_Handle_t * const Handle_p);
ST_ErrorCode_t STEXTVIN_Close(const STEXTVIN_Handle_t Handle);
ST_ErrorCode_t STEXTVIN_Term(const ST_DeviceName_t DeviceName, const STEXTVIN_TermParams_t * const TermParams_p);
ST_ErrorCode_t STEXTVIN_GetVipStatus(const STEXTVIN_Handle_t Handle,U8* Status);
ST_ErrorCode_t STEXTVIN_SelectVipInput(const STEXTVIN_Handle_t Handle,const STEXTVIN_VipInputSelection_t Selection);
ST_ErrorCode_t STEXTVIN_SelectVipOutput(const STEXTVIN_Handle_t Handle,const STEXTVIN_VipOutputSelection_t Selection);
ST_ErrorCode_t STEXTVIN_SetVipStandard(const STEXTVIN_Handle_t Handle,const STEXTVIN_VipStandard_t Standard);
ST_ErrorCode_t STEXTVIN_SetAdcInputFormat(const STEXTVIN_Handle_t Handle,const STEXTVIN_AdcInputFormat_t Format);

ST_ErrorCode_t STEXTVIN_WriteI2C(const STEXTVIN_Handle_t Handle, const U8 Adress, const U8 Value);
ST_ErrorCode_t STEXTVIN_ReadI2C(const STEXTVIN_Handle_t Handle, const U8 Adress, const U8* Value_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STEXTVIN_H */

/* End of stextvin.h */
