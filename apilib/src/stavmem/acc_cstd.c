/*******************************************************************************

File name : acc_cstd.c

Description : AVMEM memory access file for C standard functions memory access

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
 8 Dec 1999         Created                                          HG
 3 Jan 2000         Added use of memset in fill                      HG
01 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#ifndef ST_OSLINUX
    #include <string.h>
#endif

#include "stos.h"
#include "stddefs.h"
#include "stavmem.h"
#include "avm_acc.h"
#include "acc_cstd.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

static void InitCStandardFunctions(void);
static void Copy1DNoOverlapCStandardFunctions(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy1DHandleOverlapCStandardFunctions(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size);
static void Copy2DNoOverlapCStandardFunctions(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                              void * const DestAddr_p, const U32 DestPitch);
static void Copy2DHandleOverlapCStandardFunctions(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                  void * const DestAddr_p, const U32 DestPitch);
static void Fill1DCStandardFunctions(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize);
static void Fill2DCStandardFunctions(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                     void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch);


/* Private Variables (static)------------------------------------------------ */

/* C memory access functions locking semaphore (memcpy, memmove, memset...) */
static semaphore_t *stavmem_LockCStandardFunctions_p;

static BOOL CStandardFunctionsInitialised = FALSE;

stavmem_MethodOperations_t stavmem_CStandardFunctions = {
    InitCStandardFunctions,                         /* Private function */
    /* All functions supported by C standard functions */
    {
        Copy1DNoOverlapCStandardFunctions,          /* Private function */
        Copy1DHandleOverlapCStandardFunctions,      /* Private function */
        Copy2DNoOverlapCStandardFunctions,          /* Private function */
        Copy2DHandleOverlapCStandardFunctions,      /* Private function */
        Fill1DCStandardFunctions,                   /* Private function */
        Fill2DCStandardFunctions                    /* Private function */
    },
    /* No need to check: C standard functions is always successful */
    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};

/* Private Macros ----------------------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : InitCStandardFunctions
Description : Initialise C standard functions copy/fill method
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void InitCStandardFunctions(void)
{
    static BOOL CStandardFunctionsSemaphoreInitialised = FALSE;

    /* Protect semaphores initialisation */
#ifdef ST_OSLINUX
    STOS_InterruptLock();
#else
    STOS_TaskLock();
#endif
    if (!CStandardFunctionsSemaphoreInitialised)
    {
        /* Init C memory access functions locking semaphore */
        stavmem_LockCStandardFunctions_p = STOS_SemaphoreCreateFifo(NULL,1);
        CStandardFunctionsSemaphoreInitialised = TRUE;
    }
    /* Un-protect semaphores initialisation */
#ifdef ST_OSLINUX
    STOS_InterruptUnlock();
#else
    STOS_TaskUnlock();
#endif
    /* Caution, deletion will never occur ! */
    /* Delete C memory access functions locking semaphore */
/*    semaphore_delete(stavmem_LockCStandardFunctions_p);*/

    CStandardFunctionsInitialised = TRUE;
}


/*******************************************************************************
Name        : Copy1DNoOverlapCStandardFunctions
Description : Perform 1D copy with no care of overlaps, with C standard functions method
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DNoOverlapCStandardFunctions(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    /* Ensure semaphore is initialised */
/*    if (!CStandardFunctionsInitialised)
    {
        InitCStandardFunctions();
    }*/

    /* Wait for the C memory access functions locking semaphore to be free */
    STOS_SemaphoreWait(stavmem_LockCStandardFunctions_p);

    memcpy(DestAddr_p, SrcAddr_p, Size);

    /* Free the C memory access functions locking semaphore */
    STOS_SemaphoreSignal(stavmem_LockCStandardFunctions_p);
}


/*******************************************************************************
Name        : Copy1DHandleOverlapCStandardFunctions
Description : Perform 1D copy with care of overlaps, with C standard functions method
Parameters  : source, destination, size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy1DHandleOverlapCStandardFunctions(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    /* Ensure semaphore is initialised */
/*    if (!CStandardFunctionsInitialised)
    {
        InitCStandardFunctions();
    }*/

    /* Wait for the C memory access functions locking semaphore to be free */
    STOS_SemaphoreWait(stavmem_LockCStandardFunctions_p);

    memmove(DestAddr_p, SrcAddr_p, Size);

    /* Free the C memory access functions locking semaphore */
    STOS_SemaphoreSignal(stavmem_LockCStandardFunctions_p);
}


/*******************************************************************************
Name        : Copy2DNoOverlapCStandardFunctions
Description : Perform 2D copy with no care of overlaps, with C standard functions method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DNoOverlapCStandardFunctions(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                              void * const DestAddr_p, const U32 DestPitch)
{
    U32 y;

    /* Ensure semaphore is initialised */
/*    if (!CStandardFunctionsInitialised)
    {
        InitCStandardFunctions();
    }*/

    /* Wait for the C memory access functions locking semaphore to be free */
    STOS_SemaphoreWait(stavmem_LockCStandardFunctions_p);

    for (y = 0; y < SrcHeight; y++)
    {
        memcpy((void *) (((U8 *) DestAddr_p) + (DestPitch * y)), (void *) (((U8 *) SrcAddr_p) + (SrcPitch * y)), SrcWidth);
    }

    /* Free the C memory access functions locking semaphore */
    STOS_SemaphoreSignal(stavmem_LockCStandardFunctions_p);
}


/*******************************************************************************
Name        : Copy2DHandleOverlapCStandardFunctions
Description : Perform 2D copy with care of overlaps, with C standard functions method
Parameters  : source, dimensions, source pitch, destination, destination pitch
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Copy2DHandleOverlapCStandardFunctions(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                                  void * const DestAddr_p, const U32 DestPitch)
{
    U32 y;
    U32 ChangeDirectionPoint;

    /* Ensure semaphore is initialised */
/*    if (!CStandardFunctionsInitialised)
    {
        InitCStandardFunctions();
    }*/

    /* Wait for the C memory access functions locking semaphore to be free */
    STOS_SemaphoreWait(stavmem_LockCStandardFunctions_p);

    /* To handle overlap of regions: must care of directions of copy in order not to loose data
       Depending on the location of the overlap, there will be 2 directions of copy with
       a particular point where the direction change. */

    /* Start searching from first addresses */
    ChangeDirectionPoint = 1;
    if (((U8 *) DestAddr_p) < ((U8 *) SrcAddr_p))
    {
        /* Case where destination region comes before: start copy in forward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (SrcHeight + 1))
            {
                break;
            }
        } while ((((U8 *) DestAddr_p) + ((ChangeDirectionPoint - 1) * DestPitch)) < (((U8 *) SrcAddr_p) + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from height down to ChangeDirectionPoint and from 1 up to ChangeDirectionPoint */
        /* From 1 to ChangeDirectionPoint-1, copy forward direction */
        for (y = 1; y < ChangeDirectionPoint; y++)
        {
            /* Care about overlaps */
            memmove((void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), (void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), SrcWidth);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy backward direction */
        for (y = SrcHeight; y >= ChangeDirectionPoint; y--)
        {
            /* Care about overlaps */
            memmove((void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), (void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), SrcWidth);
        }
    }
    else {
        /* Case where source region comes before: start copy in backward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (SrcHeight + 1))
            {
                break;
            }
        } while ((((U8 *) DestAddr_p) + ((ChangeDirectionPoint - 1) * DestPitch)) > (((U8 *) SrcAddr_p) + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from ChangeDirectionPoint up to height and down to 1 */
        /* From 1 to ChangeDirectionPoint-1, copy backward direction */
        for (y = ChangeDirectionPoint - 1; y > 0; y--)
        {
            /* Care about overlaps */
            memmove((void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), (void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), SrcWidth);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
        for (y = ChangeDirectionPoint; y <= SrcHeight; y++)
        {
            /* Care about overlaps */
            memmove((void *) (((U8 *) DestAddr_p) + ((y - 1) * DestPitch)), (void *) (((U8 *) SrcAddr_p) + ((y - 1) * SrcPitch)), SrcWidth);
        }
    }

    /* Free the C memory access functions locking semaphore */
    STOS_SemaphoreSignal(stavmem_LockCStandardFunctions_p);
}


/*******************************************************************************
Name        : Fill1DCStandardFunctions
Description : Perform 1D fill with C standard functions method
Parameters  : pattern, pattern size, destination, destination size
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Fill1DCStandardFunctions(void * const Pattern_p,  const U32 PatternSize,  void * const DestAddr_p, const U32 DestSize)
{
    U32 IndexDest = 0;

    /* Wait for the C memory access functions locking semaphore to be free */
    STOS_SemaphoreWait(stavmem_LockCStandardFunctions_p);

    if (PatternSize == 1)
    {
        memset(DestAddr_p, *((U8 *) Pattern_p), DestSize);
    }
    else
    {
        while ((DestSize - IndexDest) > PatternSize)
        {
            memcpy((void *) (((U8 *) DestAddr_p) + IndexDest), Pattern_p, PatternSize);
            IndexDest += PatternSize;
        }
        memcpy((void *) (((U8 *) DestAddr_p) + IndexDest), Pattern_p, DestSize - IndexDest);
    }

    /* Free the C memory access functions locking semaphore */
    STOS_SemaphoreSignal(stavmem_LockCStandardFunctions_p);
}


/*******************************************************************************
Name        : Fill2DCStandardFunctions
Description : Perform 2D fill with C standard functions method
Parameters  : pattern, pattern characteristics, destination, destination  characteristics
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void Fill2DCStandardFunctions(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                     void * const DestAddr_p, const U32 DestWidth, const U32 DestHeight, const U32 DestPitch)
{
    U32 Line, PatLine, IndexDest;

    /* Wait for the C memory access functions locking semaphore to be free */
    STOS_SemaphoreWait(stavmem_LockCStandardFunctions_p);

    if ((PatternWidth == 1) && (PatternHeight == 1))
    {
        for (Line = 0; Line < DestHeight; Line++)
        {
            memset((void *) (((U8 *) DestAddr_p) + (Line * DestPitch)), *((U8 *) Pattern_p), DestWidth);
        }
    }
    else
    {
        PatLine = 0;
        for (Line = 0; Line < DestHeight; Line++)
        {
            IndexDest = 0;
            while ((DestWidth - IndexDest) > PatternWidth)
            {
                memcpy((void *) (((U8 *) DestAddr_p) + (Line * DestPitch) + IndexDest), (void *) (((U8 *) Pattern_p) + (PatLine * PatternPitch)), PatternWidth);
                IndexDest += PatternWidth;
            }
            memcpy((void *) (((U8 *) DestAddr_p) + (Line * DestPitch) + IndexDest), (void *) (((U8 *) Pattern_p) + (PatLine * PatternPitch)), DestWidth - IndexDest);

            PatLine++;
            if (PatLine == PatternHeight)
            {
                PatLine = 0;
            }
        }
    }

    /* Free the C memory access functions locking semaphore */
    STOS_SemaphoreSignal(stavmem_LockCStandardFunctions_p);
}



/* End of acc_cstd.c */


