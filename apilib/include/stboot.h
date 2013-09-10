/*****************************************************************************
File Name   : stboot.h

Description : API References for the STBOOT Device

Copyright (C) 2000 STMicroelectronics

Revision History :

    18/05/00  Modified values of enum type STBOOT_DramMemorySize_t
              to allow SDRAM_SIZE constant (defined by include)
              to map directly on to typedef'd values.
              
Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STBOOT_H
#define __STBOOT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"           /* STAPI includes */

/* Exported Constants ----------------------------------------------------- */

/* Driver constants, required by the driver model */
#define STBOOT_DRIVER_ID                119
#define STBOOT_ERROR_BASE               (STBOOT_DRIVER_ID << 16)

/* Error constants */
#define STBOOT_ERROR_KERNEL_INIT        (STBOOT_ERROR_BASE)
#define STBOOT_ERROR_KERNEL_START       (STBOOT_ERROR_BASE + 1)
#define STBOOT_ERROR_INTERRUPT_INIT     (STBOOT_ERROR_BASE + 2)
#define STBOOT_ERROR_INTERRUPT_ENABLE   (STBOOT_ERROR_BASE + 3)
#define STBOOT_ERROR_UNKNOWN_BACKEND    (STBOOT_ERROR_BASE + 4)
#define STBOOT_ERROR_BACKEND_MISMATCH   (STBOOT_ERROR_BASE + 5)
#define STBOOT_ERROR_INVALID_DCACHE     (STBOOT_ERROR_BASE + 6)
#define STBOOT_ERROR_SDRAM_INIT         (STBOOT_ERROR_BASE + 7)
#define STBOOT_ERROR_STI2C              (STBOOT_ERROR_BASE + 8)
#define STBOOT_ERROR_BSP_FAILURE        (STBOOT_ERROR_BASE + 9)

/* Exported API constants */
#define STBOOT_REVISION_UNKNOWN         ((U8)-1)

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Enumeration of device types available:
  st.device_code is bits 21-12 of DEVICEID */
typedef enum STBOOT_Device_e
{
    STBOOT_DEVICE_UNKNOWN,
    STBOOT_DEVICE_5500 = 0x0C9,
    STBOOT_DEVICE_5505 = 0x0CB,
    STBOOT_DEVICE_5508 = 0x002,
    STBOOT_DEVICE_5510 = 0x0CD,
    STBOOT_DEVICE_5512 = 0x001,
    STBOOT_DEVICE_5514 = 0x00A,
    STBOOT_DEVICE_5518 = 0x005,
    STBOOT_DEVICE_5580 = 0x00B,
    STBOOT_DEVICE_TP3  = 0x194,
    STBOOT_DEVICE_5516 = 0x01D,
    STBOOT_DEVICE_5517 = 0x01F,
    STBOOT_DEVICE_5528 = 0x01A,
    STBOOT_DEVICE_5100 = 0x023,
    STBOOT_DEVICE_5101 = 0x026,
    STBOOT_DEVICE_7710 = 0x021,
    STBOOT_DEVICE_5105 = 0x027,
    STBOOT_DEVICE_5700 = 0x3FF,
    STBOOT_DEVICE_5188 = 0x031,
    STBOOT_DEVICE_5107 = 0x032,
    
    /* the following symbols do not represent production devices,
      and are retained for backwards compatibility only */
    STBOOT_DEVICE_5520 = 0x1000,
    STBOOT_DEVICE_TT1
} STBOOT_Device_t;

/* Enum to specify the memory size of the embedded DRAM on the board */
typedef enum STBOOT_DramMemorySize_e
{
    STBOOT_DRAM_MEMORY_SIZE_0        = 0x00000000,
    STBOOT_DRAM_MEMORY_SIZE_16_MBIT  = 0x00200000,
    STBOOT_DRAM_MEMORY_SIZE_32_MBIT  = 0x00400000,
    STBOOT_DRAM_MEMORY_SIZE_64_MBIT  = 0x00800000
} STBOOT_DramMemorySize_t;

/* Backend information exported */
typedef struct STBOOT_Backend_s
{
    STBOOT_Device_t         DeviceType;
    U8                      MajorRevision;
    U8                      MinorRevision;
} STBOOT_Backend_t;

/* DCACHE memory area definition */
typedef struct STBOOT_DCache_Area_s
{
    U32                     *StartAddress;
    U32                     *EndAddress;
} STBOOT_DCache_Area_t;

/* Initialization parameters */
typedef struct STBOOT_InitParams_s
{
    U32                         *CacheBaseAddress;
    U32                         SDRAMFrequency;
    BOOL                        ICacheEnabled;
    STBOOT_DCache_Area_t        *DCacheMap;
    STBOOT_Backend_t            BackendType;
    STBOOT_DramMemorySize_t     MemorySize;
    U32                         IntTriggerMask;
#ifdef ST_OS20
    interrupt_trigger_mode_t    *IntTriggerTable_p;
#endif
#ifdef ST_OS21
    BOOL                        TimeslicingEnabled;
    BOOL                        DCacheEnabled;
    STBOOT_DCache_Area_t        *HostMemoryMap;
#endif
} STBOOT_InitParams_t;

/* Termination parameters */
typedef struct STBOOT_TermParams_s
{
    U32                     ReservedForFutureUse;
} STBOOT_TermParams_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_ErrorCode_t  STBOOT_Init( const ST_DeviceName_t DeviceName,
                             const STBOOT_InitParams_t *InitParams );
ST_ErrorCode_t  STBOOT_GetBackendInfo( STBOOT_Backend_t *BackendInfo );
ST_Revision_t   STBOOT_GetRevision( void );
ST_ErrorCode_t  STBOOT_Term( const ST_DeviceName_t DeviceName,
                             const STBOOT_TermParams_t *TermParams );

#if defined (ST_7020) && !defined(mb290) && !defined(mb295)
ST_ErrorCode_t  STBOOT_Init7020STEM( const ST_DeviceName_t I2CDeviceName,
                                     U32 SDRAMFrequency );
#endif /* db573 support */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __STBOOT_H */

/* End of stboot.h */
