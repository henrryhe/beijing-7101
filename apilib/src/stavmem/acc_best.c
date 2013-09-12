/*******************************************************************************

File name : acc_best.c

Description : AVMEM memory access file for best method strategy

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
 8 Dec 1999         Created                                          HG
29 May 2001         Add GPDMA method                                 HSdLM
 *                  Add stavmem_ before non API exported symbols
 *                  Remove dead code on cache control.
02 Jul 2002         Fix DDTS GNBvd13397 "STAVMEM_CopyBlock2D fails   HSdLM
 *                  at end of memory"
20 Nov 2002         Add FDMA method                                 HSdLM
 ********************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

/* Outcomment definition below to disable cache coherence preservation (defined for release) */
#define PRESERVE_CACHE_COHERENCE


/* Includes ----------------------------------------------------------------- */
#ifndef ST_OSLINUX
    #include <string.h>
#endif

#include "stddefs.h"
/*#ifndef ST_OSLINUX */
#include "sttbx.h"
#include "stsys.h"
/*#endif */

#include "stavmem.h"
#include "avm_acc.h"
#include "acc_best.h"
#include "stos.h"
/* Always include byte pointers method */
#include "acc_bptr.h"

/* Choose below to include C standard functions or not by including .h file: */
#ifdef STAVMEM_MEM_ACCESS_C_STANDARD_FUNCTIONS
#include "acc_cstd.h"
#undef STAVMEM_MEM_ACCESS_C_STANDARD_FUNCTIONS
#endif

/* Choose below to include block move DMA peripheral or not by including .h file: */
#ifdef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
#include "acc_bmp.h"
#undef STAVMEM_MEM_ACCESS_BLOCK_MOVE_PERIPHERAL
#endif

/* Choose below to include MPEG 2D block move or not by including .h file: */
#ifdef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
#include "acc_2dbm.h"
#undef STAVMEM_MEM_ACCESS_MPEG_2D_BLOCK_MOVE
#endif

/* Choose below to include MPEG 1D block move or not by including .h file: */
#ifdef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
#include "acc_1dbm.h"
#undef STAVMEM_MEM_ACCESS_MPEG_1D_BLOCK_MOVE
#endif

/* Choose below to include GPDMA or not by including .h file: */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
#include "acc_gpd.h"
#undef STAVMEM_MEM_ACCESS_GPDMA
#endif

/* Choose below to include FDMA or not by including .h file: */
#ifdef STAVMEM_MEM_ACCESS_FDMA
#include "acc_fdma.h"
#undef STAVMEM_MEM_ACCESS_FDMA
#endif


/* Private Types ------------------------------------------------------------ */

typedef enum
{
    STAVMEM_COPY_METHOD_BYTE_POINTERS,          /* Copy with U8 pointers */
    STAVMEM_COPY_METHOD_C_STANDARD_FUNC,        /* Copy with memcpy and memmove ANSI C standard functions */
    STAVMEM_COPY_METHOD_BLOCK_MOVE_PERIPHERAL,  /* Copy with block move DMA peripheral */
    STAVMEM_COPY_METHOD_MPEG_2D_BLOCK_MOVE,     /* Copy with MPEG 2D block move peripheral */
    STAVMEM_COPY_METHOD_MPEG_1D_BLOCK_MOVE,     /* Copy with MPEG 1D block move peripheral */
    STAVMEM_COPY_METHOD_GPDMA,                  /* Copy with GPDMA peripheral */
    STAVMEM_COPY_METHOD_FDMA,                   /* Copy with FDMA peripheral */
    STAVMEM_SIZE_OF_METHOD_ID                   /* Keep this is element as the last one: it determines the number of methods ! */
} stavmem_MethodID_t;

typedef struct MemArea_s
{
    void *FirstAddr_p;              /* First physical address of the block */
    void *LastAddr_p;               /* Last physical address of the block */
} MemArea_t;


#if 0
/* Ready for the future */
typedef struct
{
    STAVMEM_MemoryRange_t SrcRange;     /* Concerns source in there */
    STAVMEM_MemoryRange_t DestRange;    /* Concerns destination in there */
    stavmem_MethodID_t  Method;         /* Then this is the best copy method */

} stavmem_BestCopyMethod_t;
#endif

/* Private Constants -------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

static void InitBestMethod(void);

#ifndef STAVMEM_NO_COPY_FILL
static BOOL IsInCache(void * const FirstAddr_p, void * const LastAddr_p);

static stavmem_MethodID_t DetermineBestMethodCopy(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size, const U32 HeightFor2D, const U32 SrcPitchFor2D, const U32 DestPitchFor2D, const BOOL MustHandleOverlaps);
static stavmem_MethodID_t DetermineBestMethodFill(void * const Pattern_p, void * const DestAddr_p, const U32 PatternWidth, const U32 DestWidth, const U32 PatternHeightFor2D, const U32 PatternPitchFor2D, const U32 DestHeightFor2D, const U32 DestPitchFor2D);

static void Copy1DNoOverlapBestMethod(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy1DHandleOverlapBestMethod(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapBestMethod(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                void * const DestAddr_p, const U32 DestPitch);
static void Copy2DHandleOverlapBestMethod(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                          void * const DestAddr_p, const U32 DestPitch);
static void Fill1DBestMethod(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DBestMethod(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                             void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);
#endif

/* Private Variables (static)------------------------------------------------ */

/* This table should contain pointers to all the available copy methods structures */
static stavmem_MethodOperations_t * const stavmem_RegisteredMethods[STAVMEM_SIZE_OF_METHOD_ID] = {
    /* Respect the order of enum stavmem_MethodID_t, and insure there is STAVMEM_SIZE_OF_METHOD_ID elements ! */
    &stavmem_BytePointers, /* Always having this one ... */
#ifdef __AVMEM_ACC_CSTD_H
    &stavmem_CStandardFunctions,        /* Use name from acc_cstd.h */
#else /* not def __AVMEM_ACC_CSTD_H */
    NULL,
#endif
#ifdef __AVMEM_ACC_BMPERIPH_H
    &stavmem_BlockMovePeripheral,       /* Use name from acc_bmp.h */
#else /* not def __AVMEM_ACC_BMPERIPH_H */
    NULL,
#endif
#ifdef __AVMEM_ACC_MPEG2DBM_H
    &stavmem_MPEG2DBlockMove,           /* Use name from acc_2dbm.h */
#else /* not def __AVMEM_ACC_MPEG2DBM_H */
    NULL,
#endif
#ifdef __AVMEM_ACC_MPEG1DBM_H
    &stavmem_MPEG1DBlockMove,           /* Use name from acc_1dbm.h */
#else /* not def __AVMEM_ACC_MPEG1DBM_H */
    NULL,
#endif
#ifdef __AVMEM_ACC_GPDMA_H
    &stavmem_GPDMA,                     /* Use name from acc_gpd.h */
#else /* not def __AVMEM_ACC_GPDMA_H */
    NULL,
#endif
#ifdef __AVMEM_ACC_FDMA_H
    &stavmem_FDMA,                     /* Use name from acc_fdma.h */
#else /* not def __AVMEM_ACC_FDMA_H */
    NULL,
#endif
};


/* Private Macros ----------------------------------------------------------- */


/* Exported variables ------------------------------------------------------- */

stavmem_MethodOperations_t stavmem_BestMethod = {
    InitBestMethod,                         /* Private function */
    /* All functions must be supported by best method */
#ifndef STAVMEM_NO_COPY_FILL
    {
        Copy1DNoOverlapBestMethod,          /* Private function */
        Copy1DHandleOverlapBestMethod,      /* Private function */
        Copy2DNoOverlapBestMethod,          /* Private function */
        Copy2DHandleOverlapBestMethod,      /* Private function */
        Fill1DBestMethod,                   /* Private function */
        Fill2DBestMethod                    /* Private function */
    },
    /* No need to check: best method is always successful */
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
#endif /*#ifndef STAVMEM_NO_COPY_FILL*/
};


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : InitBestMethod
Description : Initialise memory access strategy engine
Parameters  : None
Assumptions : Table stavmem_RegisteredMethods is already initialised with all the
              methods available, there's a NULL in the table where there's no method
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitBestMethod(void)
{
    stavmem_MethodID_t Id;

    /* Initialise all registered methods */
    for (Id = 0; Id < STAVMEM_SIZE_OF_METHOD_ID; Id++)
    {
        if (stavmem_RegisteredMethods[Id] != NULL)
        {
            (stavmem_RegisteredMethods[Id])->Init();
        }
    }
} /* end of InitBestMethod() */
#ifndef STAVMEM_NO_COPY_FILL

/*******************************************************************************
Name        : IsInCache
Description : Check if memory area is in cache
Parameters  : first and last address of the memory area to check
Assumptions :
Limitations :
Returns     : TRUE if is in cache, FALSE otherwise
*******************************************************************************/
static BOOL IsInCache(void * const FirstAddr_p, void * const LastAddr_p)
{
    U32 AreaIndex;

    for (AreaIndex = 0; AreaIndex < stavmem_MemAccessDevice.NumberOfDCachedAreas; AreaIndex++)
    {
        if (RangesOverlap(FirstAddr_p, LastAddr_p, stavmem_MemAccessDevice.DCachedRanges_p[AreaIndex].StartAddr_p,
                                                   stavmem_MemAccessDevice.DCachedRanges_p[AreaIndex].StopAddr_p))
        {
            return(TRUE);
        }
    }

    return(FALSE);
} /* end of IsInCache() */

/*******************************************************************************
Name        : DetermineBestMethodCopy
Description : Choose the best copy method depending on different parameters
Parameters  : All copy parameters + flag: TRUE if must care overlap, FALSE otherwise
              (set Height to 1 and pitch to 0 if 1D)
Assumptions :
Limitations :
Returns     : Method ID
*******************************************************************************/
static stavmem_MethodID_t DetermineBestMethodCopy(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size, const U32 HeightFor2D, const U32 SrcPitchFor2D, const U32 DestPitchFor2D, const BOOL MustHandleOverlaps)
{
#ifndef STAVMEM_NOT_OPTIMIZED_MEM_ACCESS
    /* Variable used to avoid methods bypassing cache when cache coherence must be preserved */
    BOOL MustCareAboutCache = FALSE;

    /* Check if there need to care about cache coherence */
#ifdef PRESERVE_CACHE_COHERENCE
    if (IsInCache(SrcAddr_p,  (void *) ((U32) SrcAddr_p  + Size - 1 + (SrcPitchFor2D  * (HeightFor2D - 1)))) ||
        IsInCache(DestAddr_p, (void *) ((U32) DestAddr_p + Size - 1 + (DestPitchFor2D * (HeightFor2D - 1)))))
    {
        MustCareAboutCache = TRUE;
    }
#endif

    /* Look for the very best method of copy. */
    /* Of course, it depends on the chip. The choice below is done for STi5510, and relys on:
    -the memory address
    -the size
    -the cacheability
    -the CPU use (not considered)
    -the alignement (not considered) */

#ifdef __AVMEM_ACC_CSTD_H
    /* For very small sizes, don't loose time cheking methods, use C standard functions */
    if ((Size * HeightFor2D) < 10)
    {
        /* Best choice in that case: memcpy and memmove */
        return(STAVMEM_COPY_METHOD_C_STANDARD_FUNC);
    }
#endif /* not def __AVMEM_ACC_CSTD_H */


#ifdef __AVMEM_ACC_MPEG2DBM_H
    /* MPEG 2D is bypassing cache: it cannot be used if cache coherence has to be maintained */
    if (!MustCareAboutCache)
    {
        /* Check best cases for MPEG 2D block move */
        if (MustHandleOverlaps)
        {
            if ((HeightFor2D == 1) && (SrcPitchFor2D == 0))
            {
                /* 1D */
                if (stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy1DHandleOverlap(SrcAddr_p, DestAddr_p, Size ) == ST_NO_ERROR)
                {
                    return(STAVMEM_COPY_METHOD_MPEG_2D_BLOCK_MOVE);
                }
            }
            else
            {
                /* 2D */
                if (stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy2DHandleOverlap(SrcAddr_p, Size, HeightFor2D, SrcPitchFor2D, DestAddr_p, DestPitchFor2D) == ST_NO_ERROR)
                {
                    return(STAVMEM_COPY_METHOD_MPEG_2D_BLOCK_MOVE);
                }
            }
        }
        else
        {
            if ((HeightFor2D == 1) && (SrcPitchFor2D == 0))
            {
                /* 1D */
                if (stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy1DNoOverlap(SrcAddr_p, DestAddr_p, Size ) == ST_NO_ERROR)
                {
                    return(STAVMEM_COPY_METHOD_MPEG_2D_BLOCK_MOVE);
                }
            }
            else
            {
                /* 2D */
                if (stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Copy2DNoOverlap(SrcAddr_p, Size, HeightFor2D, SrcPitchFor2D, DestAddr_p, DestPitchFor2D) == ST_NO_ERROR)
                {
                    return(STAVMEM_COPY_METHOD_MPEG_2D_BLOCK_MOVE);
                }
            }
        }
    }
#endif /* not def __AVMEM_ACC_MPEG2DBM_H */

#ifdef __AVMEM_ACC_MPEG1DBM_H
    /* MPEG 1D is bypassing cache: it cannot be used if cache coherence has to be maintained */
    if (!MustCareAboutCache)
    {
        /* Check best cases for MPEG 2D block move */
        if (!MustHandleOverlaps)
        {
            if ((HeightFor2D == 1) && (SrcPitchFor2D == 0))
            {
                /* 1D */
                if (stavmem_MPEG1DBlockMove.CheckIfCanMakeIt.Copy1DNoOverlap(SrcAddr_p, DestAddr_p, Size ) == ST_NO_ERROR)
                {
                    return(STAVMEM_COPY_METHOD_MPEG_1D_BLOCK_MOVE);
                }
            }
            else
            {
                /* 2D */
                if (stavmem_MPEG1DBlockMove.CheckIfCanMakeIt.Copy2DNoOverlap(SrcAddr_p, Size, HeightFor2D, SrcPitchFor2D, DestAddr_p, DestPitchFor2D) == ST_NO_ERROR)
                {
                    return(STAVMEM_COPY_METHOD_MPEG_1D_BLOCK_MOVE);
                }
            }
        }
    }
#endif /* not def __AVMEM_ACC_MPEG1DBM_H */

#ifdef __AVMEM_ACC_CSTD_H
    /* Check best cases for C standard functions */
    if ((Size < 512) || /* Choose memcpy for smaller sizes */
/*        ((((U32) SrcAddr_p) >= 0x40000000) && (((U32) SrcAddr_p) <= 0x4fffffff) &&*/
/*         (!IsInCache(SrcAddr_p, (void *) ((U32) SrcAddr_p  + Size - 1 + (SrcPitchFor2D  * (HeightFor2D - 1))))))*/
        /* Choose memcpy when source is DRAM and there's no cache */ (FALSE) /* No more true */
    )
    {
        /* Best choice in that case: memcpy and memmove */
        return(STAVMEM_COPY_METHOD_C_STANDARD_FUNC);
    }
#endif /* not def __AVMEM_ACC_CSTD_H */

    /* No very best method was found: choose a method by default.
       Caution: this method should always work ! (Check functions NULL in stavmem_MethodOperations_t structure) */

#ifdef __AVMEM_ACC_GPDMA_H
    /* If GPDMA fails, DetermineBestMethodCopy is called again to choose another method */
    if (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed)
    {
        /* GPDMA is bypassing cache: it cannot be used if cache coherence has to be maintained */
        if (!MustCareAboutCache)
        {
            if (!MustHandleOverlaps)
            {
                return(STAVMEM_COPY_METHOD_GPDMA);
            }
        }
    }
#endif /* not def __AVMEM_ACC_GPDMA_H */

#ifdef __AVMEM_ACC_FDMA_H
    /* If FDMA fails, DetermineBestMethodCopy is called again to choose another method */
    if (!stavmem_MemAccessDevice.LastFdmaTranfertFailed)
    {
        /* FDMA is bypassing cache: it cannot be used if cache coherence has to be maintained */
        if (!MustCareAboutCache)
        {
            if (!MustHandleOverlaps)
            {
                return(STAVMEM_COPY_METHOD_FDMA);
            }
        }
    }
#endif /* not def __AVMEM_ACC_FDMA_H */

#ifdef __AVMEM_ACC_BMPERIPH_H
    /* Block move peripheral is bypassing cache: it cannot be used if cache coherence has to be maintained */
    if (!MustCareAboutCache)
    {
        if (!MustHandleOverlaps)
        {
            return(STAVMEM_COPY_METHOD_BLOCK_MOVE_PERIPHERAL);
        }
    }
#endif /* not def __AVMEM_ACC_BMPERIPH_H */

#ifdef __AVMEM_ACC_CSTD_H
    return(STAVMEM_COPY_METHOD_C_STANDARD_FUNC);
#endif /* not def __AVMEM_ACC_CSTD_H */

#endif /* def STAVMEM_NOT_OPTIMIZED_MEM_ACCESS */

    /* No choice of best method: copy with byte pointers */
    return(STAVMEM_COPY_METHOD_BYTE_POINTERS);
} /* end of DetermineBestMethodCopy() */

/*******************************************************************************
Name        : DetermineBestMethodFill
Description : Choose the best fill method depending on different parameters
Parameters  : All fill parameters (set Height to 1 and pitch to 0 if 1D)
Assumptions :
Limitations :
Returns     : Method ID
*******************************************************************************/
static stavmem_MethodID_t DetermineBestMethodFill(void * const Pattern_p, void * const DestAddr_p,
                                                  const U32 PatternWidth, const U32 DestWidth,
                                                  const U32 PatternHeightFor2D, const U32 PatternPitchFor2D,
                                                  const U32 DestHeightFor2D, const U32 DestPitchFor2D)
{
#ifndef STAVMEM_NOT_OPTIMIZED_MEM_ACCESS
    /* Variable used to avoid methods bypassing cache when cache coherence must be preserved */
    BOOL MustCareAboutCache = FALSE;

    /* Check if there need to care about cache coherence */
#ifdef PRESERVE_CACHE_COHERENCE
    if (IsInCache(Pattern_p,  (void *) ((U32) Pattern_p  + PatternWidth - 1 + (PatternPitchFor2D  * (PatternHeightFor2D - 1)))) ||
        IsInCache(DestAddr_p, (void *) ((U32) DestAddr_p + DestWidth    - 1 + (DestPitchFor2D     * (DestHeightFor2D    - 1)))))
    {
        MustCareAboutCache = TRUE;
    }
#endif

    /* Now look for the very best method of copy. */
    /* Of course, it depends on the chip. The choice below is done for STi5510, and relys on:
    -the memory address
    -the size
    -the cacheability
    -the CPU use (not considered)
    -the alignement (not considered) */

#ifdef __AVMEM_ACC_CSTD_H
    /* For very small sizes, don't loose time cheking methods, use C standard functions */
    if (DestWidth < 8)
    {
        /* Best choice in that case: memcpy and memmove */
        return(STAVMEM_COPY_METHOD_C_STANDARD_FUNC);
    }
#endif /* not def __AVMEM_ACC_CSTD_H */


#ifdef __AVMEM_ACC_MPEG2DBM_H
    /* MPEG 2D is bypassing cache: it cannot be used if cache coherence has to be maintained */
    if (!MustCareAboutCache)
    {
        /* Check best cases for MPEG 2D block move */
        if (stavmem_MPEG2DBlockMove.CheckIfCanMakeIt.Fill2D(Pattern_p, PatternWidth, PatternHeightFor2D, PatternPitchFor2D, DestAddr_p, DestWidth, DestHeightFor2D, DestPitchFor2D) == ST_NO_ERROR)
        {
            return(STAVMEM_COPY_METHOD_MPEG_2D_BLOCK_MOVE);
        }
    }
#endif /* not def __AVMEM_ACC_MPEG2DBM_H */

#ifdef __AVMEM_ACC_CSTD_H
    /* Check best cases for C standard functions */
    if ((PatternWidth < 512) || /* Choose memcpy for smaller sizes */
/*        ((((U32) Pattern_p) >= 0x40000000) && (((U32) Pattern_p) <= 0x4fffffff) &&*/
/*         (!IsInCache(Pattern_p, (void *) ((U32) Pattern_p  + PatternWidth - 1 + (PatternPitchFor2D  * (PatternHeightFor2D - 1))))))*/
        /* Choose memcpy when source is DRAM and there's no cache */ (FALSE) /* No more true */
    )
    {
        /* Best choice in that case: memcpy and memmove */
        return(STAVMEM_COPY_METHOD_C_STANDARD_FUNC);
    }
#endif /* not def __AVMEM_ACC_CSTD_H */

    /* No very best method was found: choose a method by default.
       Caution: this method should always work ! (Check functions NULL in stavmem_MethodOperations_t structure) */

#ifdef __AVMEM_ACC_GPDMA_H
    /* If GPDMA fails, DetermineBestMethodCopy is called again to choose another method */
    if (!stavmem_MemAccessDevice.LastGpdmaTranfertFailed)
    {
        /* GPDMA is bypassing cache: it cannot be used if cache coherence has to be maintained */
        if (!MustCareAboutCache)
        {
            return(STAVMEM_COPY_METHOD_GPDMA);
        }
    }
#endif /* not def __AVMEM_ACC_GPDMA_H */

#ifdef __AVMEM_ACC_FDMA_H
    /* If FDMA fails, DetermineBestMethodCopy is called again to choose another method */
    if (!stavmem_MemAccessDevice.LastFdmaTranfertFailed)
    {
        /* FDMA is bypassing cache: it cannot be used if cache coherence has to be maintained */
        if (!MustCareAboutCache)
        {
            return(STAVMEM_COPY_METHOD_FDMA);
        }
    }
#endif /* not def __AVMEM_ACC_FDMA_H */

#ifdef __AVMEM_ACC_BMPERIPH_H
    /* Block move peripheral is bypassing cache: it cannot be used if cache coherence has to be maintained */
    if (!MustCareAboutCache)
    {
        return(STAVMEM_COPY_METHOD_BLOCK_MOVE_PERIPHERAL);
    }
#endif /* not def __AVMEM_ACC_BMPERIPH_H */

#ifdef __AVMEM_ACC_CSTD_H
    return(STAVMEM_COPY_METHOD_C_STANDARD_FUNC);
#endif /* not def __AVMEM_ACC_CSTD_H */

#endif /* def STAVMEM_NOT_OPTIMIZED_MEM_ACCESS */
    /* No choice of best method: copy with byte pointers */
    return(STAVMEM_COPY_METHOD_BYTE_POINTERS);
} /* end of DetermineBestMethodFill() */



/*******************************************************************************
Name        : Copy1DNoOverlapBestMethod
Description : Perform 1D copy with no care of overlaps, with best method
Parameters  : source, destination, and size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapBestMethod(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    stavmem_MethodID_t BestID;

    /* transfers expected to succeed */
    stavmem_MemAccessDevice.LastGpdmaTranfertFailed = FALSE;
    stavmem_MemAccessDevice.LastFdmaTranfertFailed = FALSE;
    do
    {
        /* new transfer expected to succeed */
        stavmem_MemAccessDevice.TranfertFailed = FALSE;
        BestID = DetermineBestMethodCopy(SrcAddr_p, DestAddr_p, Size, 1, 0, 0, FALSE);

        /* Perform copy depending on best copy method */
        if (((stavmem_RegisteredMethods[BestID])->Execute.Copy1DNoOverlap) != NULL)
        {
            (stavmem_RegisteredMethods[BestID])->Execute.Copy1DNoOverlap(SrcAddr_p, DestAddr_p, Size);
        }
        else
        {
            stavmem_BytePointers.Execute.Copy1DNoOverlap(SrcAddr_p, DestAddr_p, Size);
        }
    } while (stavmem_MemAccessDevice.TranfertFailed);
} /* end of Copy1DNoOverlapBestMethod() */

/*******************************************************************************
Name        : Copy1DHandleOverlapBestMethod
Description : Perform 1D copy with care of overlaps, with best method
Parameters  : source, destination, and size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DHandleOverlapBestMethod(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    stavmem_MethodID_t BestID;

    /* transfers expected to succeed */
    stavmem_MemAccessDevice.LastGpdmaTranfertFailed = FALSE;
    stavmem_MemAccessDevice.LastFdmaTranfertFailed = FALSE;

    do
    {
        /* new transfer expected to succeed */
        stavmem_MemAccessDevice.TranfertFailed = FALSE;
        BestID = DetermineBestMethodCopy(SrcAddr_p, DestAddr_p, Size, 1, 0, 0, TRUE);

        /* Perform copy depending on best copy method */
        if (((stavmem_RegisteredMethods[BestID])->Execute.Copy1DHandleOverlap) != NULL)
        {
            (stavmem_RegisteredMethods[BestID])->Execute.Copy1DHandleOverlap(SrcAddr_p, DestAddr_p, Size);
        }
        else
        {
            stavmem_BytePointers.Execute.Copy1DHandleOverlap(SrcAddr_p, DestAddr_p, Size);
        }
    } while (stavmem_MemAccessDevice.TranfertFailed);
} /* end of Copy1DHandleOverlapBestMethod() */


/*******************************************************************************
Name        : Copy2DNoOverlapBestMethod
Description : Perform 2D copy with no care of overlaps, with best method
Parameters  : source, source characteristics, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapBestMethod(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                      void * const DestAddr_p, const U32 DestPitch)
{
    stavmem_MethodID_t BestID;

    /* transfers expected to succeed */
    stavmem_MemAccessDevice.LastGpdmaTranfertFailed = FALSE;
    stavmem_MemAccessDevice.LastFdmaTranfertFailed = FALSE;

    do
    {
        /* new transfer expected to succeed */
        stavmem_MemAccessDevice.TranfertFailed = FALSE;
        BestID = DetermineBestMethodCopy(SrcAddr_p, DestAddr_p, SrcWidth, SrcHeight, SrcPitch, DestPitch, FALSE);

        /* Perform copy depending on best copy method */
        if (((stavmem_RegisteredMethods[BestID])->Execute.Copy2DNoOverlap) != NULL)
        {
            (stavmem_RegisteredMethods[BestID])->Execute.Copy2DNoOverlap(SrcAddr_p, SrcWidth, SrcHeight, SrcPitch, DestAddr_p, DestPitch);
        }
        else
        {
            stavmem_BytePointers.Execute.Copy2DNoOverlap(SrcAddr_p, SrcWidth, SrcHeight, SrcPitch, DestAddr_p, DestPitch);
        }
    } while (stavmem_MemAccessDevice.TranfertFailed);
} /* end of Copy2DNoOverlapBestMethod() */



/*******************************************************************************
Name        : Copy2DHandleOverlapBestMethod
Description : Perform 2D copy with no care of overlaps, with best method
Parameters  : source, source characteristics, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DHandleOverlapBestMethod(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                          void * const DestAddr_p, const U32 DestPitch)
{
    stavmem_MethodID_t BestID;

    /* transfers expected to succeed */
    stavmem_MemAccessDevice.LastGpdmaTranfertFailed = FALSE;
    stavmem_MemAccessDevice.LastFdmaTranfertFailed = FALSE;

    do
    {
        /* new transfer expected to succeed */
        stavmem_MemAccessDevice.TranfertFailed = FALSE;
        BestID = DetermineBestMethodCopy(SrcAddr_p, DestAddr_p, SrcWidth, SrcHeight, SrcPitch, DestPitch, TRUE);

        /* Perform copy depending on best copy method */
        if (((stavmem_RegisteredMethods[BestID])->Execute.Copy2DHandleOverlap) != NULL)
        {
            (stavmem_RegisteredMethods[BestID])->Execute.Copy2DHandleOverlap(SrcAddr_p, SrcWidth, SrcHeight, SrcPitch, DestAddr_p, DestPitch);
        }
        else
        {
            stavmem_BytePointers.Execute.Copy2DHandleOverlap(SrcAddr_p, SrcWidth, SrcHeight, SrcPitch, DestAddr_p, DestPitch);
        }
    } while (stavmem_MemAccessDevice.TranfertFailed);
} /* end of Copy2DHandleOverlapBestMethod() */


/*******************************************************************************
Name        : Fill1DBestMethod
Description : Perform 1D fill with best method
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Fill1DBestMethod(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    stavmem_MethodID_t BestID;

    /* transfers expected to succeed */
    stavmem_MemAccessDevice.LastGpdmaTranfertFailed = FALSE;
    stavmem_MemAccessDevice.LastFdmaTranfertFailed = FALSE;

    do
    {
        /* new transfer expected to succeed */
        stavmem_MemAccessDevice.TranfertFailed = FALSE;
        BestID = DetermineBestMethodFill(Pattern_p, DestAddr_p, PatternSize, DestSize, 1, 0, 1, 0);

        /* Perform fill depending on best fill method */
        if (((stavmem_RegisteredMethods[BestID])->Execute.Fill1D) != NULL)
        {
            (stavmem_RegisteredMethods[BestID])->Execute.Fill1D(Pattern_p, PatternSize,DestAddr_p, DestSize);
        }
        else
        {
            stavmem_BytePointers.Execute.Fill1D(Pattern_p, PatternSize,DestAddr_p, DestSize);
        }
    } while (stavmem_MemAccessDevice.TranfertFailed);
} /* end of Fill1DBestMethod() */



/*******************************************************************************
Name        : Fill1DBestMethod
Description : Perform 2D fill with best method
Parameters  : pattern, pattern characteristics, destination, destination characteristics
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Fill2DBestMethod(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                             void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    stavmem_MethodID_t BestID;

    /* transfers expected to succeed */
    stavmem_MemAccessDevice.LastGpdmaTranfertFailed = FALSE;
    stavmem_MemAccessDevice.LastFdmaTranfertFailed = FALSE;

    do
    {
        /* new transfer expected to succeed */
        stavmem_MemAccessDevice.TranfertFailed = FALSE;
        BestID = DetermineBestMethodFill(Pattern_p, DestAddr_p, PatternWidth, DestWidth, PatternHeight, PatternPitch, DestHeight, DestPitch);

        /* Perform fill depending on best fill method */
        if (((stavmem_RegisteredMethods[BestID])->Execute.Fill2D) != NULL)
        {
            (stavmem_RegisteredMethods[BestID])->Execute.Fill2D(Pattern_p, PatternWidth, PatternHeight, PatternPitch, DestAddr_p, DestWidth, DestHeight, DestPitch);
        }
        else
        {
            stavmem_BytePointers.Execute.Fill2D(Pattern_p, PatternWidth, PatternHeight, PatternPitch, DestAddr_p, DestWidth, DestHeight, DestPitch);
        }
    } while (stavmem_MemAccessDevice.TranfertFailed);
} /* end of Fill2DBestMethod() */
#endif /*STAVMEM_NO_COPY_FILL*/
/* End of acc_best.c */
