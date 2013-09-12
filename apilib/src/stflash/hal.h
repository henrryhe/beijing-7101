/**********************************************************************

File Name   : hal.h

Description : Function prototypes for the HAL
              Declares the function calls and their formal
              parameters.

Copyright (C) 2005, ST Microelectronics

References  : $ClearCase (VOB: stflash)

***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HAL_H
#define __HAL_H

/* Includes --------------------------------------------------------- */
#include "stddefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------- */
typedef struct
{
    U32                 *BaseAddress;     /* Flash bank base address */
    U32                 MinAccessWidth;   /* Access width of device(s) (bytes) */
    U32                 MaxAccessWidth;   /* devices per bank * MinAccessWidth */
    U32                 MinAccessMask;    /* MinAccessWidth - 1 */
    U32                 MaxAccessMask;    /* MaxAccessWidth - 1 */
    U32                 *VppAddress;      /* Flash bank Vpp enable address */

} HAL_WriteParams_t;

typedef struct
{
    U32                 *BaseAddress;     /* Flash bank base address */
    U32                 MaxAccessWidth;   /* devices per bank * MinAccessWidth */
    U32                 *VppAddress;      /* Flash bank Vpp enable address */

} HAL_EraseParams_t;  /* A HAL_WriteParams_t might be more appropriate in the future... */

typedef struct
{
    U32                 *BaseAddress;     /* Flash bank base address */
    U32                 MaxAccessWidth;   /* devices per bank */
    U32                 *VppAddress;      /* Flash bank Vpp enable address */

} HAL_LockUnlockParams_t; 
	
#ifdef ARCHITECTURE_ST20
#pragma ST_device(DU8)
#pragma ST_device(DU16)
#pragma ST_device(DU32)
#endif

typedef volatile U8   DU8;
typedef volatile U16  DU16;
typedef volatile U32  DU32;

/* Exported Constants ----------------------------------------------- */


/* Exported Variables ----------------------------------------------- */

/* Exported Macros -------------------------------------------------- */

#define HAL_Read(ControlBlock, ReadParams, Address, Buffer_p, NumberToRead, NumberActuallyRead_p) \
    ( ((stflash_Inst_t *)(ControlBlock)) -> \
    ReadFunction( (ReadParams), (Address),(Buffer_p), (NumberToRead), (NumberActuallyRead_p)))

#define HAL_Write(ControlBlock, WriteParams, Address, Buffer_p, NumberToWrite, NumberActuallyWritten_p) \
    ( ((stflash_Inst_t *)(ControlBlock)) -> \
    WriteFunction( (WriteParams), (Address), (Buffer_p), (NumberToWrite), (NumberActuallyWritten_p)))

#define HAL_Erase(ControlBlock, EraseParams, Address, NumberToErase) \
    ( ((stflash_Inst_t *)(ControlBlock)) -> \
    EraseFunction( (EraseParams), (Address), (NumberToErase)) )
    
#define HAL_Lock(ControlBlock, LockParams, Address) \
    ( ((stflash_Inst_t *)(ControlBlock)) -> \
    LockFunction( (LockParams), (Address)) )
    
#define HAL_Unlock(ControlBlock, UnlockParams, Address) \
    ( ((stflash_Inst_t *)(ControlBlock)) -> \
    UnlockFunction( (UnlockParams), (Address)) )

#define HAL_SetReadMemoryArray(ControlBlock, AccessWidth, Addr08, Addr16, Addr32) \
    ( ((stflash_Inst_t *)(ControlBlock)) -> \
    RMAFunction( (AccessWidth), (Addr08), (Addr16), (Addr32)) )


/* Exported Functions ----------------------------------------------- */
void HAL_M28_SetReadMemoryArray( U32 AccessWidth, DU8 *Addr08, DU16 *Addr16, DU32 *Addr32 );
void HAL_M29_SetReadMemoryArray( U32 AccessWidth, DU8 *Addr08, DU16 *Addr16, DU32 *Addr32 );
void HAL_M29_ResetMemoryArray( U32 AccessWidth, DU8 *Addr08, DU16 *Addr16, DU32 *Addr32 );
/***** This may be needed in the future *****
ST_ErrorCode_t HAL_M28_Read(    U32              Address,                  // offset from BaseAddress
                                U8               *Buffer,                  // base of receipt buffer
                                U32              NumberToRead,             // in bytes
                                U32              *NumberActuallyRead );
ST_ErrorCode_t HAL_M29_Read(    U32              Address,                  // offset from BaseAddress
                                U8               *Buffer,                  // base of receipt buffer
                                U32              NumberToRead,             // in bytes
                                U32              *NumberActuallyRead );
*********************************************/
ST_ErrorCode_t HAL_M28_Lock(   HAL_LockUnlockParams_t LockParams,
                               U32              Offset                 /* offset from BaseAddress */
                            );
                             
ST_ErrorCode_t HAL_M28_Unlock(   HAL_LockUnlockParams_t UnlockParams,
                                 U32              Offset                 /* offset from BaseAddress */
                             );

ST_ErrorCode_t HAL_M28_Write(   HAL_WriteParams_t WriteParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U8               *Buffer,                 /* base of source buffer */
                                U32              NumberToWrite,           /* in bytes */
                                U32              *NumberActuallyWritten );                           


ST_ErrorCode_t HAL_M28_WriteToBuffer(   HAL_WriteParams_t WriteParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U8               *Buffer,                 /* base of source buffer */
                                U32              NumberToWrite,           /* in bytes */
                                U32              *NumberActuallyWritten );

ST_ErrorCode_t HAL_M28_FastWrite(   HAL_WriteParams_t WriteParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U8               *Buffer,                 /* base of source buffer */
                                U32              NumberToWrite,           /* in bytes */
                                U32              *NumberActuallyWritten );

ST_ErrorCode_t HAL_M29_Write(   HAL_WriteParams_t WriteParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U8               *Buffer,                 /* base of source buffer */
                                U32              NumberToWrite,           /* in bytes */
                                U32              *NumberActuallyWritten );

ST_ErrorCode_t HAL_M28_Erase(   HAL_EraseParams_t EraseParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U32              NumberToErase );         /* in bytes */
ST_ErrorCode_t HAL_AT49_Erase(   HAL_EraseParams_t EraseParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U32              NumberToErase );         /* in bytes */

ST_ErrorCode_t HAL_M29_Erase(   HAL_EraseParams_t EraseParams,
                                U32              Offset,                 /* offset from BaseAddress */
                                U32              NumberToErase );         /* in bytes */

ST_ErrorCode_t HAL_M29_ChipErase(   HAL_EraseParams_t EraseParams,
                                    U32              Offset,                 /* offset from BaseAddress */
                                    U32              NumberToErase );
#if defined(STFLASH_SPI_SUPPORT)                                    
ST_ErrorCode_t HAL_SPI_Erase( U32              Handle,
                              U32              Offset,                 /* offset from BaseAddress */
                              U32              NumberToErase );        
                                                        
ST_ErrorCode_t HAL_SPI_Write( U32              Handle,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten );
                              
ST_ErrorCode_t HAL_SPI_Read(  U32              Handle,
                              U32              Offset,                 /* offset from BaseAddress */
                              U8               *Buffer,                 /* base of source buffer */
                              U32              NumberToWrite,           /* in bytes */
                              U32              *NumberActuallyWritten );
                              
ST_ErrorCode_t HAL_SPI_Lock(  U32              Handle,
                              U32              Offset );   
                              
ST_ErrorCode_t HAL_SPI_Unlock( U32             Handle,
                               U32             Offset );   
                               
void HAL_SPI_ClkDiv( U8              Divisor );   
                              
void HAL_SPI_Config( U8 PartNum,
                     U32 CSHighTime,
                     U32 CSHoldTime,
                     U32 DataHoldTime );   
                              
void HAL_SPI_ModeSelect(   BOOL IsContigMode,
                           BOOL IsFastRead   );
#endif                              
#ifdef __cplusplus
}
#endif

#endif   /* #ifndef __STFLASH_H */

/* End of stflash.h */

