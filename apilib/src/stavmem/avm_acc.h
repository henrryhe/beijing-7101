/*******************************************************************************

File name : avm_acc.h

Description : AVMEM memory access header file for memory access features

COPYRIGHT (C) STMicroelectronics 1999.

Date               Modification                                     Name
----               ------------                                     ----
 8 Dec 1999         Created                                          HG
 2 Oct 2000         Correct problem in AbsMinus macro comparison     HG
03 Apr 2001         Added shared memory management with virtual
                    addresses.                                       HSdLM
01 Jun 2001         Add GPDMA copy method variables.                 HSdLM
05 Jul 2001         Remove dependency upon STGPDMA if GPDMA method   HSdLM
 *                  is not available (compilation flag)
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __AVMEM_ACC_MAIN_H
#define __AVMEM_ACC_MAIN_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#ifdef STAVMEM_MEM_ACCESS_GPDMA
#include "stgpdma.h"
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */

/* Parameters required to initialise STAVMEM Mermoy Access */
typedef struct stavmem_MemAccessInitParams_s
{
    STAVMEM_DeviceType_t  DeviceType;
    ST_Partition_t        *CPUPartition_p;           /* Where the module can allocate memory for its internal usage */
    ST_Partition_t        *NCachePartition_p;        /* Non-cached partition, required only for STFDMA nodes */

    void                  *BlockMoveDmaBaseAddr_p;   /* Base address of the block move DMA peripheral registers */
    void                  *CacheBaseAddr_p;          /* Base address of the cache registers */
    void                  *VideoBaseAddr_p;          /* Base address of the video registers */
    STAVMEM_SharedMemoryVirtualMapping_t *SharedMemoryVirtualMapping_p; /* used only if virtual shared memory management */
    void                  *SDRAMBaseAddr_p;          /* Base address of the shared SDRAM in the CPU memory map */
    U32                   SDRAMSize;                 /* Size of the SDRAM available */

    U32                   NumberOfDCachedAreas;      /* Number of ranges to take into account in the following array */
    STAVMEM_MemoryRange_t *DCachedRanges_p;          /* Used to inform STAVMEM of cached memory */

    void                  *OptimisedMemAccessStrategy_p;  /* Array of optimised copy methods for optimised copy strategy */
    ST_DeviceName_t       GpdmaName;                 /* Name of STGPDMA instance initialized with STAVMEM dedicated GPDMA channel */
} stavmem_MemAccessInitParams_t;

typedef struct AvmemMemAccessDevice_s
{
    ST_Partition_t        *CPUPartition_p;           /* Where the module allocated memory for its own usage,
                                                        also where it will have to be freed at termination */
    ST_Partition_t        *NCachePartition_p;        /* Non-cached partition, required only for STFDMA nodes */

    void                  *BlockMoveDmaBaseAddr_p;   /* Base address of the block move DMA peripheral registers */
    void                  *CacheBaseAddr_p;          /* Base address of the cache registers */
    void                  *VideoBaseAddr_p;          /* Base address of the video registers */
    STAVMEM_SharedMemoryVirtualMapping_t SharedMemoryVirtualMapping; /* used only if virtual shared memory management */
    void                  *SDRAMBaseAddr_p;          /* Base address of the SDRAM in the CPU memory map */
    U32                   SDRAMSize;                 /* Size of the SDRAM available */

    U32                   NumberOfDCachedAreas;      /* Number of ranges to take into account in the following array */
    STAVMEM_MemoryRange_t *DCachedRanges_p;          /* Where DCache map is stored */
    BOOL                  TranfertFailed;            /* TRUE if last copy method failed, if so another method must be used */
    BOOL                  LastGpdmaTranfertFailed;   /* TRUE if GPDMA copy method failed, if so another method must be used */
    BOOL                  LastFdmaTranfertFailed;    /* TRUE if FDMA copy method failed, if so another method must be used */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
    STGPDMA_Handle_t      GpdmaHandle;               /* Handle onto open instance on GpdmaName */
#endif /* STAVMEM_MEM_ACCESS_GPDMA */
#ifdef DVD_SECURED_CHIP
    BOOL                  SecuredCopy;               /*  Set to TRUE if copy is done in a secured region*/
#endif
} stavmem_MemAccessDevice_t;

/*
typedef struct
{
    U8 NotUsed;
} stavmem_MemAccessTermParams_t;
*/

typedef void (*stavmem_InitMemAccess_t) (void);
#ifndef STAVMEM_NO_COPY_FILL
typedef void (*stavmem_Copy1D_t) (void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
typedef void (*stavmem_Copy2D_t) (void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                            void * const DestAddr_p, const U32 DestPitch);
typedef void (*stavmem_Fill1D_t) (void * const Pattern_p,  const U32 PatternSize,
                                            void * const DestAddr_p, const U32 DestSize);
typedef void (*stavmem_Fill2D_t) (void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                            void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);
typedef ST_ErrorCode_t (*stavmem_CheckCopy1D_t) (void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
typedef ST_ErrorCode_t (*stavmem_CheckCopy2D_t) (void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                            void * const DestAddr_p, const U32 DestPitch);
typedef ST_ErrorCode_t (*stavmem_CheckFill1D_t) (void * const Pattern_p,  const U32 PatternSize,
                                            void * const DestAddr_p, const U32 DestSize);
typedef ST_ErrorCode_t (*stavmem_CheckFill2D_t) (void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                            void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);

/* Set field to NULL if the operation is not supported by the method */
typedef const struct CopyFill_s {
    stavmem_Copy1D_t Copy1DNoOverlap;
    stavmem_Copy1D_t Copy1DHandleOverlap;
    stavmem_Copy2D_t Copy2DNoOverlap;
    stavmem_Copy2D_t Copy2DHandleOverlap;
    stavmem_Fill1D_t Fill1D;
    stavmem_Fill2D_t Fill2D;
} stavmem_CopyFill_t;

/* Set field to NULL if can always make it, or if the operation is not supported by the method */
typedef const struct CheckCopyFill_s {
    stavmem_CheckCopy1D_t Copy1DNoOverlap;
    stavmem_CheckCopy1D_t Copy1DHandleOverlap;
    stavmem_CheckCopy2D_t Copy2DNoOverlap;
    stavmem_CheckCopy2D_t Copy2DHandleOverlap;
    stavmem_CheckFill1D_t Fill1D;
    stavmem_CheckFill2D_t Fill2D;
} stavmem_CheckCopyFill_t;
#endif
typedef const struct MethodOperations_s {
    stavmem_InitMemAccess_t Init;               /* Should never be NULL ! */
#ifndef STAVMEM_NO_COPY_FILL
    stavmem_CopyFill_t Execute;                 /* Set fields inside to NULL if copy/fill not supported */
    stavmem_CheckCopyFill_t CheckIfCanMakeIt;   /* Set fields inside to NULL if can always make it */
#endif
} stavmem_MethodOperations_t;


/* Exported Variables ------------------------------------------------------- */

extern stavmem_MemAccessDevice_t stavmem_MemAccessDevice; /* Only one init */


/* Exported Macros ---------------------------------------------------------- */

#define AbsMinus(a, b) ((((U32) (a)) > ((U32) (b))) ? (((U32) (a)) - ((U32) (b))) : (((U32) (b)) - ((U32) (a))))
#define RangesOverlap(FirstAddr1, LastAddr1, FirstAddr2, LastAddr2) ((((U32) (FirstAddr1)) <= ((U32) (LastAddr2))) && (((U32) (LastAddr1)) >= ((U32) (FirstAddr2))))
#define RangeInsideRange(FirstAddr1, LastAddr1, FirstAddr2, LastAddr2) ((((U32) (FirstAddr1)) >= ((U32) (FirstAddr2))) && (((U32) (LastAddr1)) <= ((U32) (LastAddr2))))
#define Spatial2DOverlap(FirstAddr1, FirstAddr2, Width12, Height12, Pitch12) ( \
 (RangesOverlap(((U32) FirstAddr1), ((U32) FirstAddr1) + (Width12) - 1 + ((Pitch12) * ((Height12) - 1)), ((U32) FirstAddr2), ((U32) FirstAddr2) + (Width12) - 1 + ((Pitch12) * ((Height12) - 1)))) && \
 ((((AbsMinus((FirstAddr1), (FirstAddr2)) + (Pitch12)) % (Pitch12)) < (Width12)) || \
  (((AbsMinus((FirstAddr1), (FirstAddr2)) + (Pitch12)) % (Pitch12)) > ((Pitch12) - (Width12)))) \
)


/* Exported Functions ------------------------------------------------------- */


ST_ErrorCode_t stavmem_MemAccessInit(const stavmem_MemAccessInitParams_t * const InitParams_p);
/*ST_ErrorCode_t stavmem_MemAccessTerm(const stavmem_MemAccessTermParams_t * const TermParams_p);*/
ST_ErrorCode_t stavmem_MemAccessTerm(void);
ST_ErrorCode_t stavmem_IsAddressInSecureRange(U32* const Addr_p, U32 Size, BOOL IsSecure );

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __AVMEM_ACC_MAIN_H */

/* End of avm_acc.h */
