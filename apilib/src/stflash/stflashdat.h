/**********************************************************************

File Name   : stflashdat.h

Description : Function prototypes for the HAL
              Declares the function calls and their formal
              parameters.

Copyright (C) 2005, ST Microelectronics

References  : $ClearCase (VOB: stflash)

***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STFLASHDAT_H
#define __STFLASHDAT_H

/* Includes --------------------------------------------------------- */
#include "stddefs.h"
#include "stflash.h"
#include "hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------- */
/* linked list construct to support any number of Flash banks */
#if defined(STFLASH_MULTIBANK_SUPPORT)
typedef struct STFLASH_OpenInfo_s
{
    U32                     BankStartOffset;
    U32                     BankEndOffset;
    BOOL                    FlashBankOpen;
    STFLASH_Handle_t 		Handle;
} STFLASH_OpenInfo_t;
#endif

struct inst_t
{
    struct inst_t       *Next;                      /* ptr. to next (else NULL) */
    ST_DeviceName_t     BankName;                   /* Initialization bank name */
    U32                 MagicNumber;                /* Handle validity check value */
    BOOL                BankOpen;                   /* Flash bank Opened iff TRUE */
    U32                 LastOffsP1;                 /* last offset plus one */
    U32                 DeviceCode;                 /* stored for GetParams */
    U32                 ManufactCode;               /* stored for GetParams */
    STFLASH_DeviceType_t DeviceType;                /* Flash device type */
    U32                 *BaseAddress;               /* Flash bank base address */
    U32                 *VppAddress;                /* Flash bank Vpp enable address */
    U32                 MinAccessWidth;             /* Access width of device(s) (bytes) */
    U32                 MaxAccessWidth;             /* devices per bank * MinAccessWidth */
    U32                 MinAccessMask;              /* MinAccessWidth - 1 */
    U32                 MaxAccessMask;              /* MaxAccessWidth - 1 */
    ST_Partition_t      *DriverPartition;           /* base address of partition */
#if defined(STFLASH_SPI_SUPPORT)
    BOOL                IsSerialFlash;
    BOOL                IsFastRead;
    U32                 SPIHandle;
    U32                 CSHandle;
    STPIO_BitMask_t     SPICSBit;
#endif
/* This may be needed in future*************************
    ST_ErrorCode_t     (*ReadFunction) (U32 Address, U8 *Buffer,
                            U32 NumberToRead, U32 *NumberActuallyRead );
********************************************************/

    ST_ErrorCode_t      (*WriteFunction) (HAL_WriteParams_t WriteParams, U32 Address, U8 *Buffer,
                         U32 NumberToWrite, U32 *NumberActuallyWritten );
    ST_ErrorCode_t      (*EraseFunction) (HAL_EraseParams_t EraseParams, U32 Address, U32 NumberToErase);
    ST_ErrorCode_t      (*ReadFunction) ( HAL_WriteParams_t ReadParams,U32 Address, U8 *Buffer, U32 NumberToRead, U32 *NumberActuallyRead);
    void                (*RMAFunction) (U32 AccessWidth, DU8 *Addr08, DU16 *Addr16, DU32 *Addr32);
    ST_ErrorCode_t      (*LockFunction) (HAL_LockUnlockParams_t LockParams, U32 Address );
    ST_ErrorCode_t      (*UnlockFunction) (HAL_LockUnlockParams_t UnlockParams, U32 Address );
    U32                 NumberOfBlocks;             /* number of regions in Blocks */

#if defined(STFLASH_MULTIBANK_SUPPORT)
    U32                  NumberOfBanks;
    STFLASH_OpenInfo_t   *OpenInfo;
    STFLASH_BankParams_t *BankInfo; /* Information about Banks */
#endif/*STFLASH_MULTIBANK_SUPPORT*/

    STFLASH_Block_t      Blocks[1];                  /* extendable size */
};
typedef struct inst_t   stflash_Inst_t;
/* Private Function prototypes -------------------------------------------- */

#if defined(STFLASH_MULTIBANK_SUPPORT)
BOOL HandleIsValid( STFLASH_Handle_t Handle,stflash_Inst_t **ThisElem_p);
#else
BOOL HandleIsValid( STFLASH_Handle_t Handle );
#endif

#ifdef __cplusplus
}
#endif

#endif   /* #ifndef __STFLASHDAT_H */

/* End of stflash.h */

