/**********************************************************************

File Name   : ste2p.h

Description : External function prototypes for STE2P API
              Declares the function calls and their formal
              parameters.  Requires prior inclusion of
              stddefs.h file to define parameter sizes.

Copyright (C) 1999 STMicroelectronics

References  :

$ClearCase (VOB: ste2p)

STE2P.PDF "EEPROM API" Reference DVD-API-024 Version 1.2

***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STE2P_H
#define __STE2P_H

/* Includes --------------------------------------------------------- */

#include "stddefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------- */

typedef U32 STE2P_Handle_t;             /* Identifier to EEPROM device */

typedef struct STE2P_InitParams_s
{
    U32                 *BaseAddress;   /* base address of memory-mapped device */
    ST_DeviceName_t     I2CDeviceName;  /* name of an initialized I2C device */
    size_t              MemoryWidth;    /* width in bytes of memory access */
    U32                 SizeInBytes;    /* EEPROM device size in bytes */
    U32                 PagingBytes;    /* page size for write/read operations */
    U32                 DeviceCode;     /* MFG Device Code and Chip Select */
    U32                 MaxSimOpen;     /* Max. no. of simultaneous Opens req. */
    ST_Partition_t      *DriverPartition;
} STE2P_InitParams_t;

typedef struct STE2P_OpenParams_s
{
    U32                 Offset;         /* access offset into EEPROM device */
    U32                 Length;         /* Access length from offset location */
} STE2P_OpenParams_t;

typedef struct STE2P_Params_s
{
    STE2P_InitParams_t InitParams;      /* parameters describing device */
    STE2P_OpenParams_t OpenParams;      /* parameters describing segment */
} STE2P_Params_t;

typedef struct STE2P_TermParams_s
{
    BOOL               ForceTerminate;
} STE2P_TermParams_t;

/* Exported Constants ----------------------------------------------- */

#define STE2P_DRIVER_ID         24      /* from DVD-API-024 */

/* Error constants */
#define STE2P_DRIVER_BASE       ( STE2P_DRIVER_ID << 16 )

enum
{
    STE2P_ERROR_READ=STE2P_DRIVER_BASE,
    STE2P_ERROR_WRITE,
    STE2P_ERROR_I2C ,
    STE2P_ERROR_I2C_READ,
    STE2P_ERROR_I2C_WRITE
};

#define STE2P_MIN_OPEN_CHANS    1       /* minimum value for MaxSimOpen */
#define STE2P_MAX_OPEN_CHANS    256     /* maximum value for MaxSimOpen */

/* The following constants are device dependent, currently
   suitable for ST 24E32 / M24C32 type devices.  These are
   likely to require change if other devices are used instead. */
#define ONE_K                   1024
#define STE2P_SIZEOF_DEVICE     ( ( ONE_K * 4 ) - 4 ) /* top 4 bytes for MFG id */
#define STE2P_SIZEOF_PAGE       32      /* maximum number of accessible bytes */
                                        /*  without address register reload */


/* Exported Variables ----------------------------------------------- */

/* Exported Macros -------------------------------------------------- */

/* Exported Functions ----------------------------------------------- */

ST_ErrorCode_t STE2P_Init( const ST_DeviceName_t    Name,           /* name for EEPROM device */
                           const STE2P_InitParams_t *InitParams );  /* pointer to Init parameters */

ST_ErrorCode_t STE2P_Term( const ST_DeviceName_t Name,              /* name for EEPROM device */
                           const STE2P_TermParams_t *TermParams );     /* ->ForceTerminate */

ST_ErrorCode_t STE2P_Open( const ST_DeviceName_t    Name,           /* name for EEPROM device */
                           const STE2P_OpenParams_t *OpenParams,    /* pointer to segment details */
                           STE2P_Handle_t           *Handle );      /* returned handle */

ST_ErrorCode_t STE2P_Close( STE2P_Handle_t Handle );            /* device handle to be closed */

ST_ErrorCode_t STE2P_Read( STE2P_Handle_t Handle,               /* device identifier */
                           U32            SegOffset,            /* offset into segment */
                           U8             *Buffer,              /* Destination buffer head */
                           U32            NumberToRead,         /* number of bytes to read */
                           U32            *NumberReadOK );      /* no. successfully read */

ST_ErrorCode_t STE2P_Write( STE2P_Handle_t Handle,              /* unique segment handle */
                            U32            SegOffset,           /* offset into segment */
                            U8             *Buffer,             /* Source buffer head */
                            U32            NumberToWrite,       /* number of bytes to write */
                            U32            *NumberWrittenOK );  /* no. successfully written */

ST_ErrorCode_t STE2P_GetParams( STE2P_Handle_t Handle,          /* device identifier */
                                STE2P_Params_t *Params );       /* Init & Open parameters */

ST_Revision_t STE2P_GetRevision( void );                        /* version string */

#ifdef __cplusplus
}
#endif

#endif   /* #ifndef __STE2P_H */

/* End of ste2p.h */




