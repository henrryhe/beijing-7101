/*******************************************************************************

File name : avm_acc.c

Description : AVMEM module memory access features

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
17 Jun 1999         Created                                          HG
19 Aug 1999         Add STAVMEM_CopyMethod_t, structure
                    copy with optimised method and add
                    'MustHandleOverlaps' instead of U8*              HG
 6 Oct 1999         Suppressed handles in mem access func            HG
13 Oct 1999         Added base addresses as init params              HG
 8 Dec 1999         Split copy methods in separate files             HG
11 Jan 2000         In copy2D, added test on spatial 2D overlap      HG
03 Apr 2001         Added shared memory management with virtual
                    addresses. OS40 support.                         HSdLM
29 May 2001         Add GPDMA copy method                            HSdLM
                    Initialize virtual mapping in GENERIC mode
                    Add stavmem_ before non API exported symbols
05 Jul 2001         Remove dependency upon STGPDMA if GPDMA method   HSdLM
 *                  is not available (compilation flag)
02 Jul 2002         Fix DDTS GNBvd13397 "STAVMEM_CopyBlock2D fails   HSdLM
 *                  at end of memory"
09 May 2003         Multi-partition handling. Fix erroneous error    HSdLM
 *                  returned on 1D copy calling 2D function.
*******************************************************************************/

/* Private Definitions (internal use only) ---------------------------------- */

/* Outcomment definition below to use OSD private functions */
/*#define OSD_CODE*/

/* Outcomment definition below to define read/write U8, U16, U24 and U32 access functions */
/*#define POINTER_ACCESS_FUNCTIONS*/


/* Includes ----------------------------------------------------------------- */
#ifndef ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#include "stddefs.h"
/*#ifndef ST_OSLINUX */
#include "sttbx.h"
#include "stsys.h"
/*#endif   */
#include "stos.h"
#include "stavmem.h"
#include "avm_init.h"
#include "avm_acc.h"
#include "acc_best.h"
#if defined (DVD_SECURED_CHIP)
#include "stmes.h"
#endif


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Flag to ensure init is not done without term (mainly because of memory allocation) */
static BOOL MemAccessInitialised = FALSE;


/* Global Variables --------------------------------------------------------- */

stavmem_MemAccessDevice_t stavmem_MemAccessDevice; /* Only one init */
STAVMEM_SharedMemoryVirtualMapping_t * STAVMEM_VIRTUAL_MAPPING_AUTO_P;

/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */

#ifdef POINTER_ACCESS_FUNCTIONS
/*
--------------------------------------------------------------------------------
 function to read an 8 bit word from the specified address via CPU memory access
--------------------------------------------------------------------------------
*/
U8 STAVMEM_ReadMem8(void *Addr_p)
{
    return (*(U8 *)Addr_p);
}

/*
--------------------------------------------------------------------------------
 function to write an 8 bit word to the specified address via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem8(void *Addr_p, U8 Value)
{
    *(U8 *)Addr_p = Value;
}


/*
--------------------------------------------------------------------------------
 function to read a 16 bit word from the specified address with big endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
U16 STAVMEM_ReadMem16BE(void *Addr_p)
{
    U16 Result;

    Result = (U16)           (*(((U8 *)Addr_p)    ));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 1));

    return (Result);
}

/*
--------------------------------------------------------------------------------
 function to read a 16 bit word from the specified address with little endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
U16 STAVMEM_ReadMem16LE(void *Addr_p)
{
    U16 Result;

    Result = (U16)           (*(((U8 *)Addr_p) + 1));
    Result = (Result << 8) | (*(((U8 *)Addr_p)    ));

    return (Result);
}

/*
--------------------------------------------------------------------------------
 function to write a 16 bit word to the specified address with big endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem16BE(void *Addr_p, U16 Value)
{
    *(((U8 *)Addr_p)    ) = (U8) (Value >> 8);
    *(((U8 *)Addr_p) + 1) = (U8) (Value     );
}


/*
--------------------------------------------------------------------------------
 function to write a 16 bit word to the specified address with little endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem16LE(void *Addr_p, U16 Value)
{
    *(((U8 *)Addr_p) + 1) = (U8)(Value >> 8);
    *(((U8 *)Addr_p)    ) = (U8)(Value     );
}


/*
--------------------------------------------------------------------------------
 function to read a 24 bit word from the specified address with big endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
U32 STAVMEM_ReadMem24BE(void *Addr_p)
{
    U32 Result;

    Result = (U32)           (*(((U8 *)Addr_p)    ));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 1));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 2));

    return (Result);
}

/*
--------------------------------------------------------------------------------
 function to read a 24 bit word from the specified address with little endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
U32 STAVMEM_ReadMem24LE(void *Addr_p)
{
    U32 Result;

    Result = (U32)           (*(((U8 *)Addr_p) + 2));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 1));
    Result = (Result << 8) | (*(((U8 *)Addr_p)    ));

    return (Result);
}


/*
--------------------------------------------------------------------------------
 function to write a 24 bit word to the specified address with big endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem24BE(void *Addr_p, U32 Value)
{
    *(((U8 *)Addr_p)    ) = (U8)(Value >> 16);
    *(((U8 *)Addr_p) + 1) = (U8)(Value >> 8 );
    *(((U8 *)Addr_p) + 2) = (U8)(Value      );
}


/*
--------------------------------------------------------------------------------
 function to write a 24 bit word to the specified address with little endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem24LE(void *Addr_p, U32 Value)
{
    *(((U8 *)Addr_p) + 2) = (U8)(Value >> 16);
    *(((U8 *)Addr_p) + 1) = (U8)(Value >> 8 );
    *(((U8 *)Addr_p)    ) = (U8)(Value      );
}


/*
--------------------------------------------------------------------------------
 function to read a 32 bit word from the specified address with big endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
U32 STAVMEM_ReadMem32BE(void *Addr_p)
{
    U32 Result;

    Result = (U32)           (*(((U8 *)Addr_p)    ));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 1));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 2));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 3));

    return (Result);
}


/*
--------------------------------------------------------------------------------
 function to read a 32 bit word from the specified address with little endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
/* U32 STAVMEM_ReadMem32LE(void *Addr_p) */
U32 STAVMEM_ReadMem32(void *Addr_p)
{
    U32 Result;

    Result = (U32 )          (*(((U8 *)Addr_p) + 3));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 2));
    Result = (Result << 8) | (*(((U8 *)Addr_p) + 1));
    Result = (Result << 8) | (*(((U8 *)Addr_p)    ));

    return (Result);
}

/*
--------------------------------------------------------------------------------
 function to write a 32 bit word to the specified address with big endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem32BE(void *Addr_p, U32 Value)
{
    *(((U8 *)Addr_p)    ) = (U8)(Value >> 24);
    *(((U8 *)Addr_p) + 1) = (U8)(Value >> 16);
    *(((U8 *)Addr_p) + 2) = (U8)(Value >> 8 );
    *(((U8 *)Addr_p) + 3) = (U8)(Value      );
}

/*
--------------------------------------------------------------------------------
 function to write a 32 bit word to the specified address with little endianness
 via CPU memory access
--------------------------------------------------------------------------------
*/
void STAVMEM_WriteMem32LE(void *Addr_p, U32 Value)
{
    *(((U8 *)Addr_p) + 3) = (U8)(Value >> 24);
    *(((U8 *)Addr_p) + 2) = (U8)(Value >> 16);
    *(((U8 *)Addr_p) + 1) = (U8)(Value >> 8 );
    *(((U8 *)Addr_p)    ) = (U8)(Value      );
}
#endif /* #ifdef POINTER_ACCESS_FUNCTIONS */




/*******************************************************************************
Name        : stavmem_MemAccessInit
Description : Initialise memory accesses
Parameters  : Init parameters
Assumptions : InitParams_p is good and InitParams_p->DeviceType already checked.
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_NO_MEMORY if alloc failed,
*             STAVMEM_ERROR_GPDMA_OPEN if STGPDMA Open fails.
*******************************************************************************/
ST_ErrorCode_t stavmem_MemAccessInit(const stavmem_MemAccessInitParams_t * const InitParams_p)
{
    U32 i;
#ifdef STAVMEM_MEM_ACCESS_GPDMA
    const STGPDMA_OpenParams_t GPDMAOpenParams;
#endif

    if (!MemAccessInitialised)
    {
        /* Allocate memory for internal use: to store DCache map */
        if (InitParams_p->NumberOfDCachedAreas == 0)
        {
            /* Used in Term not to free when nothing is allocated */
            stavmem_MemAccessDevice.DCachedRanges_p = NULL;
        }
        else
        {
            stavmem_MemAccessDevice.DCachedRanges_p = STOS_MemoryAllocate(InitParams_p->CPUPartition_p,
                                                                    InitParams_p->NumberOfDCachedAreas * sizeof(STAVMEM_MemoryRange_t));
            AddDynamicDataSize(InitParams_p->NumberOfDCachedAreas * sizeof(STAVMEM_MemoryRange_t));
            if (stavmem_MemAccessDevice.DCachedRanges_p == NULL)
            {
                /* If allocation failed, quit with failure */
                return(ST_ERROR_NO_MEMORY);
            }
            /* Store DCache map */
            for (i = 0; i < InitParams_p->NumberOfDCachedAreas; i++)
            {
                stavmem_MemAccessDevice.DCachedRanges_p[i].StartAddr_p = InitParams_p->DCachedRanges_p[i].StartAddr_p;
                stavmem_MemAccessDevice.DCachedRanges_p[i].StopAddr_p  = InitParams_p->DCachedRanges_p[i].StopAddr_p;
            }
        }

        /* Save constants */
        stavmem_MemAccessDevice.CPUPartition_p         = InitParams_p->CPUPartition_p;
        stavmem_MemAccessDevice.NCachePartition_p      = InitParams_p->NCachePartition_p;
        stavmem_MemAccessDevice.BlockMoveDmaBaseAddr_p = InitParams_p->BlockMoveDmaBaseAddr_p;
        stavmem_MemAccessDevice.CacheBaseAddr_p        = InitParams_p->CacheBaseAddr_p;
        stavmem_MemAccessDevice.VideoBaseAddr_p        = InitParams_p->VideoBaseAddr_p;
        stavmem_MemAccessDevice.SDRAMBaseAddr_p        = InitParams_p->SDRAMBaseAddr_p;
        stavmem_MemAccessDevice.SDRAMSize              = InitParams_p->SDRAMSize;
        switch (InitParams_p->DeviceType)
        {
            case STAVMEM_DEVICE_TYPE_GENERIC :
                /* Init virtual shared memory for targets which do not need virtual addressing and for backward compatibility.
                   Indeed, for these targets, some drivers were (before virtual addressing development) accessing device memory
                   with the operation : 'address - SDRAMBaseAddr_p'.
                   Now, as these drivers must handle targets needing virtual addressing, they use virtual addressing access macros.
                   But these drivers must continue handling targets not needing virtual addressing, and do not know STAVMEM DeviceType.
                   So the macros must continue performing the operation 'address - SDRAMBaseAddr_p' for device access.
                   This is provided for backward compatibility, cannot be used on multi-chip platform.
                */
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.PhysicalAddressSeenFromCPU_p    = InitParams_p->SDRAMBaseAddr_p;
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.PhysicalAddressSeenFromDevice_p = NULL;
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.PhysicalAddressSeenFromDevice2_p= NULL;
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.VirtualBaseAddress_p            = InitParams_p->SDRAMBaseAddr_p;
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.VirtualSize                     = InitParams_p->SDRAMSize;
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.VirtualWindowOffset             = 0;
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping.VirtualWindowSize               = InitParams_p->SDRAMSize;
                break;
            case STAVMEM_DEVICE_TYPE_VIRTUAL :
                stavmem_MemAccessDevice.SharedMemoryVirtualMapping = *(InitParams_p->SharedMemoryVirtualMapping_p);
                break;
            default : /* not possible, already filtered */
                break;
        }
        /* Store DCache map size */
        stavmem_MemAccessDevice.NumberOfDCachedAreas = InitParams_p->NumberOfDCachedAreas;
        STAVMEM_VIRTUAL_MAPPING_AUTO_P = &(stavmem_MemAccessDevice.SharedMemoryVirtualMapping);
        #if defined (DVD_SECURED_CHIP)
        stavmem_MemAccessDevice.SecuredCopy = FALSE; /*Non Secure copy by default*/
#endif
        /* Initialise best copy method (others are initialised there) */
        stavmem_BestMethod.Init();

#ifdef STAVMEM_MEM_ACCESS_GPDMA
        /* Open STGPDMA connection */
        if (STGPDMA_Open(InitParams_p->GpdmaName, &GPDMAOpenParams, &(stavmem_MemAccessDevice.GpdmaHandle)) != ST_NO_ERROR)
        {
            return(STAVMEM_ERROR_GPDMA_OPEN);
        }
#endif /* STAVMEM_MEM_ACCESS_GPDMA */

        MemAccessInitialised = TRUE;
    }
    return(ST_NO_ERROR);

} /* End of stavmem_MemAccessInit() */

/*******************************************************************************
Name        : stavmem_TermMemoryAccess
Description : Terminate memory accesses
Parameters  : Term parameters
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, STAVMEM_ERROR_GPDMA_CLOSE
*******************************************************************************/
ST_ErrorCode_t stavmem_MemAccessTerm(void)
{
    /* De-allocate memory used internally to store DCache map, allocated at init */
    if (MemAccessInitialised)
    {
        if (stavmem_MemAccessDevice.DCachedRanges_p != NULL)
        {
            STOS_MemoryDeallocate(stavmem_MemAccessDevice.CPUPartition_p, stavmem_MemAccessDevice.DCachedRanges_p);
        }
#ifdef STAVMEM_MEM_ACCESS_GPDMA
        /* Close STGPDMA connection */
        if (STGPDMA_Close(stavmem_MemAccessDevice.GpdmaHandle) != ST_NO_ERROR)
        {
            return(STAVMEM_ERROR_GPDMA_CLOSE);
        }
#endif
        MemAccessInitialised = FALSE;
    }
    return(ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
 Get shared memory virtual mapping information needed to access object
 having a virtual address
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetSharedMemoryVirtualMapping(STAVMEM_SharedMemoryVirtualMapping_t * const SharedMemoryVirtualMapping_p)
{
    /* Exit if bad parameter */
    if (SharedMemoryVirtualMapping_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    *(SharedMemoryVirtualMapping_p) = stavmem_MemAccessDevice.SharedMemoryVirtualMapping;
    return(ST_NO_ERROR);
} /* End of  STAVMEM_GetSharedMemoryVirtualMapping() function */


#if defined (DVD_SECURED_CHIP)
/*--------------------------------------------------------------------------------
 * stavmem_IsAddressInSecureRange
 * Description : check if given address is in secure range
--------------------------------------------------------------------------------*/
ST_ErrorCode_t stavmem_IsAddressInSecureRange(U32* const Addr_p, U32 Size, BOOL IsSecure )
{
     unsigned int           uMemoryStatus;

    /* The check is  focus on one address and not a Range so the Length is null */
   /* The Source ID passed to the API is the sh4 cpu :SID_SH4_CPU */

      uMemoryStatus = STMES_IsMemorySecure((void *)Addr_p, Size , 0);
      if(uMemoryStatus >= STMES_ERROR_ALREADY_INITIALISED)
      {
            STTBX_Print(("ERROR : STMES_IsMemorySecure failed !! \n"));
            IsSecure = FALSE;
            return (ST_ERROR_BAD_PARAMETER);
      }
      else if (uMemoryStatus == INSECURE_REGION)
      {
            STTBX_Print(("OK : Memory is INSECURE : \n"));
            IsSecure = FALSE;
            return (ST_NO_ERROR);
      }
      else
      {
            STTBX_Print(("ERROR : Memory is SECURE : \n"));
            IsSecure = TRUE;
            return (ST_NO_ERROR);
      }
}
#endif /*(DVD_SECURED_CHIP)*/

#ifndef STAVMEM_NO_COPY_FILL
/*--------------------------------------------------------------------------------
Copy memory 1 dimension block
--------------------------------------------------------------------------------*/
ST_ErrorCode_t STAVMEM_CopyBlock1D(void * const SrcAddr_p, void * const DestAddr_p, const U32 Size)
{
    void *PhySrcAddr_p, *PhyDestAddr_p;  /* Physical address */
    STAVMEM_SharedMemoryVirtualMapping_t * VirtToPhy_p; /* information to turn virtual addresses to physical */
#if defined (DVD_SECURED_CHIP)
    BOOL IsSrcSecured= FALSE;
    BOOL IsDestSecured = FALSE;
#endif /* DVD_SECURED_CHIP */

    /* Return now if there's nothing to do */
    if (((U8 *) SrcAddr_p) == ((U8 *) DestAddr_p) ||
        (Size == 0))
    {
        return(ST_NO_ERROR);
    }

    VirtToPhy_p = &stavmem_MemAccessDevice.SharedMemoryVirtualMapping;

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if (   ((STAVMEM_IsAddressVirtual(SrcAddr_p,  VirtToPhy_p)) && !(STAVMEM_IsDataInVirtualWindow(SrcAddr_p,  Size, VirtToPhy_p)))
        || ((STAVMEM_IsAddressVirtual(DestAddr_p, VirtToPhy_p)) && !(STAVMEM_IsDataInVirtualWindow(DestAddr_p, Size, VirtToPhy_p))))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    PhySrcAddr_p =  STAVMEM_IfVirtualThenToCPU(SrcAddr_p,  VirtToPhy_p);
    PhyDestAddr_p = STAVMEM_IfVirtualThenToCPU(DestAddr_p, VirtToPhy_p);

#if defined (DVD_SECURED_CHIP)
    if ((stavmem_IsAddressInSecureRange((U32*)PhySrcAddr_p, Size, IsSrcSecured) == ST_NO_ERROR) && (stavmem_IsAddressInSecureRange((U32*)PhyDestAddr_p, Size, IsDestSecured) ==ST_NO_ERROR))
    {
        if((IsSrcSecured)&&(IsDestSecured))
        {
            stavmem_MemAccessDevice.SecuredCopy = TRUE; /* Secure copy */
        }
        else if((!IsSrcSecured)&&(!IsDestSecured))
        {
            stavmem_MemAccessDevice.SecuredCopy = FALSE; /*Non Secure copy */
        }
        else
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
    }
    else
    {
         return (ST_ERROR_BAD_PARAMETER);
    }

#endif
    if (!RangesOverlap(PhySrcAddr_p, ((U8 *) PhySrcAddr_p) + Size - 1, PhyDestAddr_p, ((U8 *) PhyDestAddr_p) + Size - 1))
    {
        /* No overlap */
        stavmem_BestMethod.Execute.Copy1DNoOverlap(PhySrcAddr_p, PhyDestAddr_p, Size);
    }
    else
    {
        /* Must care of overlaps */
        if (((U8 *) PhySrcAddr_p) < ((U8 *) PhyDestAddr_p))
        {
            /* Copy not overlaping area with no overlap functions */
            stavmem_BestMethod.Execute.Copy1DNoOverlap((void *) (((U8 *) PhySrcAddr_p) + Size - ((U8 *) PhyDestAddr_p) + ((U8 *) PhySrcAddr_p)),
                                               (void *) (((U8 *) PhySrcAddr_p) + Size),
                                               ((U8 *) PhyDestAddr_p) - ((U8 *) PhySrcAddr_p));
            /* Copy overlaping area */
            stavmem_BestMethod.Execute.Copy1DHandleOverlap(PhySrcAddr_p,
                                                   PhyDestAddr_p,
                                                   Size - (((U8 *) PhyDestAddr_p) - ((U8 *) PhySrcAddr_p)));
        }
        else /* PhySrcAddr_p > PhyDestAddr_p  (as PhySrcAddr_p != PhyDestAddr_p) */
        {
            /* Copy not overlaping area with no overlap functions */
            stavmem_BestMethod.Execute.Copy1DNoOverlap(PhySrcAddr_p,
                                               PhyDestAddr_p,
                                               ((U8 *) PhySrcAddr_p) - ((U8 *) PhyDestAddr_p));
            /* Copy overlaping area */
            stavmem_BestMethod.Execute.Copy1DHandleOverlap((void *) (((U32) PhySrcAddr_p) + ((U8 *) PhySrcAddr_p) - ((U8 *) PhyDestAddr_p)),
                                                   PhySrcAddr_p,
                                                   Size - (((U8 *) PhySrcAddr_p) - ((U8 *) PhyDestAddr_p)));
        }
    }

    return(ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
Copy memory 2 dimensions block
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_CopyBlock2D(void * const SrcAddr_p,  const U32 SrcWidth, const U32 SrcHeight, const U32 SrcPitch,
                                   void * const DestAddr_p, const U32 DestPitch)
{
    void *PhySrcAddr_p, *PhyDestAddr_p;  /* Physical address */
    U32 SrcSize, DestSize;
    STAVMEM_SharedMemoryVirtualMapping_t * VirtToPhy_p; /* information to turn virtual addresses to physical */
#if defined (DVD_SECURED_CHIP)
    BOOL IsSrcSecured= FALSE;
    BOOL IsDestSecured = FALSE;
#endif /* DVD_SECURED_CHIP */
    /* Return now if there's nothing to do */
    if ((SrcWidth == 0) || (SrcHeight == 0) ||
        ((((U8 *) SrcAddr_p) == ((U8 *) DestAddr_p)) && (SrcPitch == DestPitch)))
    {
        return(ST_NO_ERROR);
    }

    /* If width of the copy greater than the source or than destination pitch (only if height exists), error */
    if (((SrcWidth > SrcPitch) || (SrcWidth > DestPitch)) && (SrcHeight > 1))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    VirtToPhy_p = &stavmem_MemAccessDevice.SharedMemoryVirtualMapping;
    SrcSize  = SrcWidth + (SrcHeight - 1)* SrcPitch;
    DestSize = SrcWidth + (SrcHeight - 1)* DestPitch;

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if (   ((STAVMEM_IsAddressVirtual(SrcAddr_p,  VirtToPhy_p)) && !(STAVMEM_IsDataInVirtualWindow(SrcAddr_p,  SrcSize,  VirtToPhy_p)))
        || ((STAVMEM_IsAddressVirtual(DestAddr_p, VirtToPhy_p)) && !(STAVMEM_IsDataInVirtualWindow(DestAddr_p, DestSize, VirtToPhy_p))))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    PhySrcAddr_p =  STAVMEM_IfVirtualThenToCPU(SrcAddr_p,  &stavmem_MemAccessDevice.SharedMemoryVirtualMapping);
    PhyDestAddr_p = STAVMEM_IfVirtualThenToCPU(DestAddr_p, &stavmem_MemAccessDevice.SharedMemoryVirtualMapping);

#if defined (DVD_SECURED_CHIP)
    if ((stavmem_IsAddressInSecureRange((U32*)PhySrcAddr_p, SrcSize, IsSrcSecured)== ST_NO_ERROR)&& (stavmem_IsAddressInSecureRange((U32*)PhyDestAddr_p, DestSize, IsDestSecured)==ST_NO_ERROR))
    {
        if((IsSrcSecured)&&(IsDestSecured))
        {
            stavmem_MemAccessDevice.SecuredCopy = TRUE; /* Secure copy */
        }
        else if((!IsSrcSecured)&&(!IsDestSecured))
        {
            stavmem_MemAccessDevice.SecuredCopy = FALSE; /*Non Secure copy */
        }
        else
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
    }
    else
    {
         return (ST_ERROR_BAD_PARAMETER);
    }
#endif /*DVD_SECURED_CHIP*/

    /* Execute Copy 2D */
    if ((!RangesOverlap(PhySrcAddr_p,  ((U8 *) PhySrcAddr_p ) + (SrcPitch  * (SrcHeight - 1)) + SrcWidth - 1,
                        PhyDestAddr_p, ((U8 *) PhyDestAddr_p) + (DestPitch * (SrcHeight - 1)) + SrcWidth - 1)) ||
        /* Check that although there is memory overlap, there may not be spatial 2D overlap: this will spped up
           copy in same region when areas don't spatially overlap (but memory overlap).
           (test when pitches are equal because it's simple. Otherwise: suppose there is spatial overlap...) */
        ((SrcPitch == DestPitch) && (!Spatial2DOverlap(PhySrcAddr_p, PhyDestAddr_p, SrcWidth, SrcHeight, SrcPitch)))
    )
    {
        /* No overlap */
        stavmem_BestMethod.Execute.Copy2DNoOverlap(PhySrcAddr_p, SrcWidth, SrcHeight, SrcPitch, PhyDestAddr_p, DestPitch);
    }
    else
    {
        /* Must care of overlaps */
        stavmem_BestMethod.Execute.Copy2DHandleOverlap(PhySrcAddr_p, SrcWidth, SrcHeight, SrcPitch, PhyDestAddr_p, DestPitch);
    }

    return(ST_NO_ERROR);
}


/*
--------------------------------------------------------------------------------
Fill memory 1 dimension block with 1 dimension pattern
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_FillBlock1D(void * const Pattern_p,  const U32 PatternSize,
                                   void * const DestAddr_p, const U32 DestSize)
{
    void * PhyDestAddr_p; /* Physical address */
    STAVMEM_SharedMemoryVirtualMapping_t * VirtToPhy_p; /* information to turn virtual addresses to physical */

    /* Return now if there's nothing to do */
    if ((PatternSize == 0) ||
        (DestSize == 0))
    {
        return(ST_NO_ERROR);
    }

    VirtToPhy_p = &stavmem_MemAccessDevice.SharedMemoryVirtualMapping;

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if ((STAVMEM_IsAddressVirtual(DestAddr_p, VirtToPhy_p)) && !(STAVMEM_IsDataInVirtualWindow(DestAddr_p, DestSize, VirtToPhy_p)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    PhyDestAddr_p = STAVMEM_IfVirtualThenToCPU(DestAddr_p, &stavmem_MemAccessDevice.SharedMemoryVirtualMapping);


    /* Ensure pattern is not inside the copy area */
    if (RangesOverlap(Pattern_p, ((U8 *) Pattern_p) + PatternSize - 1, PhyDestAddr_p, ((U8 *) PhyDestAddr_p) + DestSize - 1))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Execute fill */
    stavmem_BestMethod.Execute.Fill1D(Pattern_p, PatternSize, PhyDestAddr_p, DestSize);

    return(ST_NO_ERROR);
}



/*
--------------------------------------------------------------------------------
Fill memory 2 dimension block with 2 dimension pattern
Note: patttern heigth is now limited to 1, so it is 1 dimension in fact
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_FillBlock2D(void * const Pattern_p,  const U32 PatternWidth, const U32 PatternHeight, const U32 PatternPitch,
                                   void * const DestAddr_p, const U32 DestWidth,    const U32 DestHeight,    const U32 DestPitch)
{
    void * PhyDestAddr_p; /* Physical address */
    U32 DestSize;
    STAVMEM_SharedMemoryVirtualMapping_t * VirtToPhy_p; /* information to turn virtual addresses to physical */

    /* Return now if there's nothing to do */
    if ((DestWidth == 0) ||
        (PatternWidth == 0))
    {
        return(ST_NO_ERROR);
    }

    /* Ensure current limitation is respected */
    if (PatternHeight != 1)
    {
        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* If width of the copy greater than the destination pitch or
          width of the pattern greater than the pattern pitch (only if height exists), error */
    if ((DestWidth > DestPitch) ||
        ((PatternHeight > 1) && (PatternWidth > PatternPitch)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    VirtToPhy_p = &stavmem_MemAccessDevice.SharedMemoryVirtualMapping;
    DestSize = DestWidth + (DestHeight - 1)* DestPitch;

    /* shared memory is CPU accessed, so if address is in virtual range, check it can be accessed from CPU */
    if ((STAVMEM_IsAddressVirtual(DestAddr_p, VirtToPhy_p)) && !(STAVMEM_IsDataInVirtualWindow(DestAddr_p, DestSize, VirtToPhy_p)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    PhyDestAddr_p = STAVMEM_IfVirtualThenToCPU(DestAddr_p, &stavmem_MemAccessDevice.SharedMemoryVirtualMapping);

    /* Ensure pattern is not inside the copy area */
    if (RangesOverlap(((U8 *) Pattern_p), ((U8 *) Pattern_p) + PatternWidth - 1, PhyDestAddr_p, ((U8 *) PhyDestAddr_p) + ((DestHeight - 1) * DestPitch) + (DestWidth -1)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Execute fill */
    stavmem_BestMethod.Execute.Fill2D(Pattern_p, PatternWidth, PatternHeight, PatternPitch, PhyDestAddr_p, DestWidth, DestHeight, DestPitch);

    return(ST_NO_ERROR);
}
#endif /*STAVMEM_NO_COPY_FILL*/







#ifdef OSD_CODE
/* The functions below are for the OSD use only. */
ST_ErrorCode_t STAVMEM_CopyBlock2DAndMirror(void *SrcAddress_p, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch,
                                            void *DestAddress_p, U32 DestPitch, void *MirrorSrcAddress_p, void *MirrorDestAddress_p, U32 MirrorSrcHeight);
ST_ErrorCode_t STAVMEM_MaskedCopyBlock2D(void *SrcAddr_p, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch,
                                   void *DestAddr_p, U32 DestPitch, U8 BitsSLeft, U8 BitsSRight, U8 BitsDLeft, U8 BitsDRight);
ST_ErrorCode_t STAVMEM_MaskedCopyBlock2DAndMirror(void *SrcAddr_p, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch,
                                   void *DestAddr_p, U32 DestPitch, U8 BitsSLeft, U8 BitsSRight, U8 BitsDLeft, U8 BitsDRight,
                                   void *MirrorSrcAddr_p, void *MirrorDestAddr_p, U32 MirrorSrcHeight);
ST_ErrorCode_t STAVMEM_MaskedFillBlock1D(void *Pattern_p, U32 PatternSize, void *DestAddr_p, U32 DestSize, U8 BitsDLeft, U8 BitsDRight);
/* The functions above are for the OSD use only. */

static __inline void MaskedCopyLineUp(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch,
                                      U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft,
                                      U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft);
static __inline void MaskedCopyLineDown(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch,
                                        U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft,
                                        U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft);
static __inline void MaskedCopyLineUpAndMirror(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch,
                                               U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft,
                                               U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft, U32 MirrorSrcAddr, U32 MirrorDestAddr);
static __inline void MaskedCopyLineDownAndMirror(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch,
                                                 U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft,
                                                 U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft, U32 MirrorSrcAddr, U32 MirrorDestAddr);


/*******************************************************************************
Name        : STAVMEM_CopyBlock2DAndMirror
Description : Copy 2D + copy 2D from/in mirror areas as well
Parameters  : id copy 2D + mirror source and destination
Assumptions :
Limitations : Mirror not implemented as block handle for now !!!
Returns     :
*******************************************************************************/
ST_ErrorCode_t STAVMEM_CopyBlock2DAndMirror(void *SrcAddr_p, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch,
                                            void *DestAddr_p, U32 DestPitch, void *MirrorSrcAddr_p, void *MirrorDestAddr_p, U32 MirrorSrcHeight)
{
    U32 x = 0, y = 0;
    U32 ChangeDirectionPoint = 1;
    U32 Width = SrcWidth, Height;
    U32 FirstAddrSrc = ((U32) SrcAddr_p), FirstAddrDest = ((U32) DestAddr_p);
    S32 Diff1;
    S32 Diff2;
    S32 MaxDiff;

    /* If width of the copy greater than the source pitch, limit it */
    if (Width > SrcPitch)
    {
/*        Width = SrcPitch;*/
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* If width of the copy greater than the destination pitch, limit it */
    if (Width > DestPitch)
    {
/*        Width = DestPitch;*/
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Consider smallest heigth to process it */
    if (MirrorSrcHeight >= SrcHeight)
    {
        Height = SrcHeight;
    }
    else
    {
        Height = MirrorSrcHeight;
    }

    /* Overlap of regions: must care of directions of copy in order not to loose data */
    /* Depending on the location of the overlap, there will be 2 directions of copy with
       a particular point where the direction change. */

    /* Start searching from first addresses */
/*    ChangeDirectionPoint = 1;*/
    Diff1 = (((U32) MirrorSrcAddr_p) - FirstAddrDest);
    Diff2 = (FirstAddrSrc - ((U32) MirrorDestAddr_p));
    if (Diff1 < 0)
    {
        Diff1 = -Diff1;
    }
    if (Diff2 < 0)
    {
        Diff2 = -Diff2;
    }

    if (Diff2 > Diff1)
    {
        MaxDiff = (S32)(FirstAddrSrc - ((U32) MirrorDestAddr_p));
/*        FirstAddrSrc = ((U32) SrcAddr_p);
        FirstAddrDest = ((U32) MirrorDestAddr_p));*/
    }
    else
    {
        MaxDiff = (S32)(((U32) MirrorSrcAddr_p) - FirstAddrDest);
/*        FirstAddrSrc = ((U32) MirrorSrcAddr_p);
        FirstAddrDest = ((U32) DestAddr_p));*/
    }

    /*if (FirstAddrDest < FirstAddrSrc)*/
    if (MaxDiff > 0)
    {
        /* Case where destination region comes before: start copy in forward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (Height + 1))
            {
                break;
            }
        } while ((FirstAddrDest + ((ChangeDirectionPoint - 1) * DestPitch)) < (FirstAddrSrc + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from height down to ChangeDirectionPoint and from 1 up to ChangeDirectionPoint */
        /* From 1 to ChangeDirectionPoint-1, copy forward direction */
        for (y = 1; y < ChangeDirectionPoint; y++)
        {
            for (x = 1; x <= Width; x++)
            {
                *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 1));
                *(((U8 *) MirrorDestAddr_p) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + ((y - 1) * SrcPitch) + (x - 1));
            }
        }
        /* Last line eventually... */
        if (MirrorSrcHeight > Height)
        {
            for (x = Width; x > 0; x--)
            {
                *(((U8 *) MirrorDestAddr_p) + (Height * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + (Height * SrcPitch) + (x - 1));
            }
        }
        if (SrcHeight > Height)
        {
            for (x = Width; x > 0; x--)
            {
                *(((U8 *) FirstAddrDest) + (Height * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + (Height * SrcPitch) + (x - 1));
            }
        }
        /* From ChangeDirectionPoint to SrcHeight, copy backward direction */
        for (y = Height; y >= ChangeDirectionPoint; y--)
        {
            for (x = Width; x > 0; x--)
            {
                *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 1));
                *(((U8 *) MirrorDestAddr_p) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + ((y - 1) * SrcPitch) + (x - 1));
            }
        }
    }
    else {
        /* Case where source region comes before: start copy in backward direction */

        /* Look for the point where the direction of copy must change */
        /* Test addresses of begining of rows for source and destination */
        do
        {
            ChangeDirectionPoint++;
            if (ChangeDirectionPoint == (Height + 1))
            {
                break;
            }
        } while ((FirstAddrDest + ((ChangeDirectionPoint - 1) * DestPitch)) > (FirstAddrSrc + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from ChangeDirectionPoint up to height and down to 1 */
        /* From 1 to ChangeDirectionPoint-1, copy backward direction */
        for (y = ChangeDirectionPoint - 1; y > 0; y--)
        {
            for (x = Width; x > 0; x--)
            {
                *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 1));
                *(((U8 *) MirrorDestAddr_p) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + ((y - 1) * SrcPitch) + (x - 1));
            }
        }
        /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
        for (y = ChangeDirectionPoint; y <= Height; y++)
        {
            for (x = 1; x <= Width; x++)
            {
                *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 1));
                *(((U8 *) MirrorDestAddr_p) + ((y - 1) * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + ((y - 1) * SrcPitch) + (x - 1));
            }
        }
        /* Last line eventually... */
        if (MirrorSrcHeight > Height)
        {
            for (x = 1; x <= Width; x++)
            {
                *(((U8 *) MirrorDestAddr_p) + (Height * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + (Height * SrcPitch) + (x - 1));
            }
        }
        if (SrcHeight > Height)
        {
            for (x = 1; x <= Width; x++)
            {
                *(((U8 *) FirstAddrDest) + (Height * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + (Height * SrcPitch) + (x - 1));
            }
        }
    }

    return(ST_NO_ERROR);
}



/*******************************************************************************
Name        : MaskedCopyLineUp
Description : Copy a ligne up with masks (used in STAVMEM_MaskedCopyBlock2D)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static __inline void MaskedCopyLineUp(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch, U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft, U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft)
{
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U32 x;

    /* Particular case 1 byte to 1 byte */
    if (Width == 1)
    {
        TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch)) & FirstSMask;
        WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
        WriteVal |= TempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;
        return;
    }

    /* Read source */
    RightTempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + 1);
    TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch)) & FirstSMask;
    LeftTempVal = 0;
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & FirstDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & FirstDMask;
    }
    /* Write destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;
    /* Process middle bytes */
    for (x = 2; x <= Width - 1; x++)
    {
        LeftTempVal = TempVal;
        TempVal = RightTempVal;
        RightTempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 0));
        /* x going up */
        if (ShiftLeft)
        {
            WriteVal = (TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft));
        }
        else
        {
            WriteVal = (TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft));
        }
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = WriteVal;
    }
    /* Read and mask source */
    LeftTempVal = TempVal;
    TempVal = RightTempVal & LastSMask;
    RightTempVal = 0;
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1))) & ~LastDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & LastDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & LastDMask;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1)) = WriteVal;
}

/*******************************************************************************
Name        : MaskedCopyLineDown
Description : Copy a ligne down with masks (used in STAVMEM_MaskedCopyBlock2D)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static __inline void MaskedCopyLineDown(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch, U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft, U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft)
{
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U32 x;

    /* Particular case 1 byte to 1 byte */
    if (Width == 1)
    {
        TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch)) & FirstSMask;
        WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
        WriteVal |= TempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;
        return;
    }

    /* Read and mask source */
    TempVal = (*(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (Width - 1))) & LastSMask;
    RightTempVal = 0;
    LeftTempVal = (*(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (Width - 2)));
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1))) & ~LastDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & LastDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & LastDMask;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1)) = WriteVal;
    /* Process middle bytes */
    for (x = Width - 1; x > 1; x--)
    {
        /* Read source */
/*        TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 1));*/
        RightTempVal = TempVal;
        TempVal = LeftTempVal;
        LeftTempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 2));
        /* x going down */
        if (ShiftLeft)
        {
            WriteVal = (TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft));
        }
        else
        {
            WriteVal = (TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft));
        }
        /* Write destination */
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = WriteVal;
    }
    /* Read source */
    RightTempVal = TempVal;
    TempVal = LeftTempVal & FirstSMask;
    LeftTempVal = 0;
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & FirstDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & FirstDMask;
    }
    /* Write destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;
}

/*******************************************************************************
Name        : STAVMEM_MaskedCopyBlock2D
Description : Copy 2D with masks for first and last bytes
Parameters  : id copy 2D + mirror source and destination + bits to consider to the left and right
Assumptions :
Limitations : Mirror not implemented as block handle for now !!!
Returns     :
*******************************************************************************/
ST_ErrorCode_t STAVMEM_MaskedCopyBlock2D(void *SrcAddr_p, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch,
                                   void *DestAddr_p, U32 DestPitch, U8 BitsSLeft, U8 BitsSRight, U8 BitsDLeft, U8 BitsDRight)
{
    U32 y = 0;
    U32 DestWidth;
    U32 ChangeDirectionPoint = 1;
    U32 Width = SrcWidth;
    U32 FirstAddrSrc = (U32) SrcAddr_p, FirstAddrDest = (U32) DestAddr_p;
    BOOL ShiftLeft;
    U8 FirstSMask = ((1 << (    BitsSLeft )) - 1); /* 1 gives 00000001 */
    U8 FirstDMask = ((1 << (    BitsDLeft )) - 1); /* 3 gives 00000111 */
    U8 LastSMask = ~((1 << (8 - BitsSRight)) - 1); /* 3 gives 11100000 */
    U8 LastDMask = ~((1 << (8 - BitsDRight)) - 1); /* 1 gives 10000000 */

    /* If width of the copy greater than the source pitch, limit it */
    if (Width > SrcPitch)
    {
/*        Width = SrcPitch;*/
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* If width of the copy greater than the destination pitch, limit it */
    if (Width > DestPitch)
    {
/*        Width = DestPitch;*/
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Calculate DestWidth */
    if ((BitsSLeft + BitsSRight) > (BitsDLeft + BitsDRight))
    {
        DestWidth = Width + 1;
        Width = DestWidth;
        /* One more byte: don't consider additional last source byte */
        LastSMask = 0;
    }
    else if ((BitsSLeft + BitsSRight) < (BitsDLeft + BitsDRight))
    {
        DestWidth = Width - 1;
        /* One less byte: don't consider additional last destination byte */
        LastDMask = 0;
    }
    else
    {
        DestWidth = Width;
    }

    /* Overlap of regions: must care of directions of copy in order not to loose data */
    /* Depending on the location of the overlap, there will be 2 directions of copy with
       a particular point where the direction change. */

    if (BitsSLeft > BitsDLeft)
    {
        ShiftLeft = FALSE;
    }
    else
    {
        ShiftLeft = TRUE;
    }

    /* Particular case 1 byte to 1 byte */
    if (Width == 1)
    {
        FirstDMask = (FirstDMask & LastDMask);
        FirstSMask = (FirstSMask & LastSMask);
    }

    /* Start searching from first addresses */
/*    ChangeDirectionPoint = 1;*/
    if (FirstAddrDest < FirstAddrSrc)
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
        } while ((FirstAddrDest + ((ChangeDirectionPoint - 1) * DestPitch)) < (FirstAddrSrc + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from height down to ChangeDirectionPoint and from 1 up to ChangeDirectionPoint */
        /* From 1 to ChangeDirectionPoint-1, copy forward direction */
        for (y = 1; y < ChangeDirectionPoint; y++)
        {
            MaskedCopyLineUp(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy backward direction */
        for (y = SrcHeight; y >= ChangeDirectionPoint; y--)
        {
            MaskedCopyLineDown(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
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
        } while ((FirstAddrDest + ((ChangeDirectionPoint - 1) * DestPitch)) > (FirstAddrSrc + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from ChangeDirectionPoint up to height and down to 1 */
        /* From 1 to ChangeDirectionPoint-1, copy backward direction */
        for (y = ChangeDirectionPoint - 1; y > 0; y--)
        {
            MaskedCopyLineDown(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
        }
        /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
        for (y = ChangeDirectionPoint; y <= SrcHeight; y++)
        {
            MaskedCopyLineUp(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
        }
    }

    return(ST_NO_ERROR);
}




/*******************************************************************************
Name        : MaskedCopyLineUpAndMirror
Description : Copy a ligne up with masks and mirror (used in STAVMEM_MaskedCopyBlock2DAndMirror)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static __inline void MaskedCopyLineUpAndMirror(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch, U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft, U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft, U32 MirrorSrcAddr, U32 MirrorDestAddr)
{
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U8 MirrorTempVal, MirrorLeftTempVal, MirrorRightTempVal, MirrorWriteVal;
    U32 x;

    /* Particular case 1 byte to 1 byte */
    if (Width == 1)
    {
        TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch)) & FirstSMask;
        WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
        WriteVal |= TempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;

        MirrorTempVal = *(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch)) & FirstSMask;
        MirrorWriteVal = (*(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch))) & ~FirstDMask;
        MirrorWriteVal |= MirrorTempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch)) = MirrorWriteVal;
        return;
    }

    /* Read source */
    RightTempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + 1);
    TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch)) & FirstSMask;
    LeftTempVal = 0;
    MirrorRightTempVal = *(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch) + 1);
    MirrorTempVal = *(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch)) & FirstSMask;
    MirrorLeftTempVal = 0;
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
    MirrorWriteVal = (*(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch))) & ~FirstDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & FirstDMask;
        MirrorWriteVal |= ((MirrorTempVal << (BitsDLeft - BitsSLeft)) | (MirrorRightTempVal >> (8 - BitsDLeft + BitsSLeft))) & FirstDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & FirstDMask;
        MirrorWriteVal |= ((MirrorTempVal >> (BitsSLeft - BitsDLeft)) | (MirrorLeftTempVal << (8 - BitsSLeft + BitsDLeft))) & FirstDMask;
    }
    /* Write destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;
    *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch)) = MirrorWriteVal;
    /* Process middle bytes */
    for (x = 2; x <= Width - 1; x++)
    {
        LeftTempVal = TempVal;
        TempVal = RightTempVal;
        RightTempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 0));
        MirrorLeftTempVal = MirrorTempVal;
        MirrorTempVal = MirrorRightTempVal;
        MirrorRightTempVal = *(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch) + (x - 0));
        /* x going up */
        if (ShiftLeft)
        {
            WriteVal = (TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft));
            MirrorWriteVal = (MirrorTempVal << (BitsDLeft - BitsSLeft)) | (MirrorRightTempVal >> (8 - BitsDLeft + BitsSLeft));
        }
        else
        {
            WriteVal = (TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft));
            MirrorWriteVal = (MirrorTempVal >> (BitsSLeft - BitsDLeft)) | (MirrorLeftTempVal << (8 - BitsSLeft + BitsDLeft));
        }
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = WriteVal;
        *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch) + (x - 1)) = MirrorWriteVal;
    }
    /* Read and mask source */
    LeftTempVal = TempVal;
    TempVal = RightTempVal & LastSMask;
    RightTempVal = 0;
    MirrorLeftTempVal = MirrorTempVal;
    MirrorTempVal = MirrorRightTempVal & LastSMask;
    MirrorRightTempVal = 0;
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1))) & ~LastDMask;
    MirrorWriteVal = (*(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch) + (Width - 1))) & ~LastDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & LastDMask;
        MirrorWriteVal |= ((MirrorTempVal << (BitsDLeft - BitsSLeft)) | (MirrorRightTempVal >> (8 - BitsDLeft + BitsSLeft))) & LastDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & LastDMask;
        MirrorWriteVal |= ((MirrorTempVal >> (BitsSLeft - BitsDLeft)) | (MirrorLeftTempVal << (8 - BitsSLeft + BitsDLeft))) & LastDMask;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1)) = WriteVal;
    *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch) + (Width - 1)) = MirrorWriteVal;
}

/*******************************************************************************
Name        : MaskedCopyLineDownAndMirror
Description : Copy a ligne down with masks and mirror (used in STAVMEM_MaskedCopyBlock2DAndMirror)
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static __inline void MaskedCopyLineDownAndMirror(U32 y, U32 FirstAddrSrc, U32 Width, U32 SrcPitch, U32 FirstAddrDest, U32 DestPitch, U8 BitsSLeft, U8 BitsDLeft, U8 FirstSMask, U8 LastSMask, U8 FirstDMask, U8 LastDMask, BOOL ShiftLeft, U32 MirrorSrcAddr, U32 MirrorDestAddr)
{
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U8 MirrorTempVal, MirrorLeftTempVal, MirrorRightTempVal, MirrorWriteVal;
    U32 x;

    /* Particular case 1 byte to 1 byte */
    if (Width == 1)
    {
        TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch)) & FirstSMask;
        WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
        WriteVal |= TempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;

        MirrorTempVal = *(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch)) & FirstSMask;
        MirrorWriteVal = (*(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch))) & ~FirstDMask;
        MirrorWriteVal |= MirrorTempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch)) = MirrorWriteVal;
        return;
    }

    /* Read and mask source */
    TempVal = (*(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (Width - 1))) & LastSMask;
    RightTempVal = 0;
    LeftTempVal = (*(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (Width - 2)));
    MirrorTempVal = (*(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch) + (Width - 1))) & LastSMask;
    MirrorRightTempVal = 0;
    MirrorLeftTempVal = (*(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch) + (Width - 2)));

    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1))) & ~LastDMask;
    MirrorWriteVal = (*(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch) + (Width - 1))) & ~LastDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & LastDMask;
        MirrorWriteVal |= ((MirrorTempVal << (BitsDLeft - BitsSLeft)) | (MirrorRightTempVal >> (8 - BitsDLeft + BitsSLeft))) & LastDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & LastDMask;
        MirrorWriteVal |= ((MirrorTempVal >> (BitsSLeft - BitsDLeft)) | (MirrorLeftTempVal << (8 - BitsSLeft + BitsDLeft))) & LastDMask;
    }
    /* Write back destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (Width - 1)) = WriteVal;
    *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch) + (Width - 1)) = MirrorWriteVal;
    /* Process middle bytes */
    for (x = Width - 1; x > 1; x--)
    {
        /* Read source */
/*        TempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 1));*/
        RightTempVal = TempVal;
        TempVal = LeftTempVal;
        LeftTempVal = *(((U8 *) FirstAddrSrc) + ((y - 1) * SrcPitch) + (x - 2));
        MirrorRightTempVal = MirrorTempVal;
        MirrorTempVal = MirrorLeftTempVal;
        MirrorLeftTempVal = *(((U8 *) MirrorSrcAddr) + ((y - 1) * SrcPitch) + (x - 2));
        /* x going down */
        if (ShiftLeft)
        {
            WriteVal = (TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft));
            MirrorWriteVal = (MirrorTempVal << (BitsDLeft - BitsSLeft)) | (MirrorRightTempVal >> (8 - BitsDLeft + BitsSLeft));
        }
        else
        {
            WriteVal = (TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft));
            MirrorWriteVal = (MirrorTempVal >> (BitsSLeft - BitsDLeft)) | (MirrorLeftTempVal << (8 - BitsSLeft + BitsDLeft));
        }
        /* Write destination */
        *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch) + (x - 1)) = WriteVal;
        *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch) + (x - 1)) = MirrorWriteVal;
    }
    /* Read source */
    RightTempVal = TempVal;
    TempVal = LeftTempVal & FirstSMask;
    LeftTempVal = 0;
    MirrorRightTempVal = MirrorTempVal;
    MirrorTempVal = MirrorLeftTempVal & FirstSMask;
    MirrorLeftTempVal = 0;
    /* Read destination because it has to be masked */
    WriteVal = (*(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch))) & ~FirstDMask;
    MirrorWriteVal = (*(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch))) & ~FirstDMask;
    if (ShiftLeft)
    {
        WriteVal |= ((TempVal << (BitsDLeft - BitsSLeft)) | (RightTempVal >> (8 - BitsDLeft + BitsSLeft))) & FirstDMask;
        MirrorWriteVal |= ((MirrorTempVal << (BitsDLeft - BitsSLeft)) | (MirrorRightTempVal >> (8 - BitsDLeft + BitsSLeft))) & FirstDMask;
    }
    else
    {
        WriteVal |= ((TempVal >> (BitsSLeft - BitsDLeft)) | (LeftTempVal << (8 - BitsSLeft + BitsDLeft))) & FirstDMask;
        MirrorWriteVal |= ((MirrorTempVal >> (BitsSLeft - BitsDLeft)) | (MirrorLeftTempVal << (8 - BitsSLeft + BitsDLeft))) & FirstDMask;
    }
    /* Write destination */
    *(((U8 *) FirstAddrDest) + ((y - 1) * DestPitch)) = WriteVal;
    *(((U8 *) MirrorDestAddr) + ((y - 1) * DestPitch)) = MirrorWriteVal;
}



/*******************************************************************************
Name        : STAVMEM_MaskedCopyBlock2DAndMirror
Description : Copy 2D with masks for first and last bytes
Parameters  : id copy 2D + mirror source and destination + bits to consider to the left and right
Assumptions :
Limitations : Mirror not implemented as block handle for now !!!
Returns     :
*******************************************************************************/
ST_ErrorCode_t STAVMEM_MaskedCopyBlock2DAndMirror(void *SrcAddr_p, U32 SrcWidth, U32 SrcHeight, U32 SrcPitch,
                                   void *DestAddr_p, U32 DestPitch, U8 BitsSLeft, U8 BitsSRight, U8 BitsDLeft, U8 BitsDRight,
                                   void *MirrorSrcAddr_p, void *MirrorDestAddr_p, U32 MirrorSrcHeight)
{
    U32 y = 0;
    U32 DestWidth;
    U32 ChangeDirectionPoint = 1;
    U32 Width = SrcWidth, Height;
    S32 Diff1;
    S32 Diff2;
    S32 MaxDiff;
    U32 FirstAddrSrc = (U32) SrcAddr_p, FirstAddrDest = (U32) DestAddr_p;
    BOOL ShiftLeft;
    U8 FirstSMask = ((1 << (    BitsSLeft )) - 1); /* 1 gives 00000001 */
    U8 FirstDMask = ((1 << (    BitsDLeft )) - 1); /* 3 gives 00000111 */
    U8 LastSMask = ~((1 << (8 - BitsSRight)) - 1); /* 3 gives 11100000 */
    U8 LastDMask = ~((1 << (8 - BitsDRight)) - 1); /* 1 gives 10000000 */

    /* If width of the copy greater than the source pitch, limit it */
    if (Width > SrcPitch)
    {
/*        Width = SrcPitch;*/
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* If width of the copy greater than the destination pitch, limit it */
    if (Width > DestPitch)
    {
/*        Width = DestPitch;*/
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Calculate DestWidth */
    if ((BitsSLeft + BitsSRight) > (BitsDLeft + BitsDRight))
    {
        DestWidth = Width + 1;
        Width = DestWidth;
        /* One more byte: don't consider additional last source byte */
        LastSMask = 0;
    }
    else if ((BitsSLeft + BitsSRight) < (BitsDLeft + BitsDRight))
    {
        DestWidth = Width - 1;
        /* One less byte: don't consider additional last destination byte */
        LastDMask = 0;
    }
    else
    {
        DestWidth = Width;
    }

    /* Consider smallest heigth to process it */
    if (MirrorSrcHeight >= SrcHeight)
    {
        Height = SrcHeight;
    }
    else
    {
        Height = MirrorSrcHeight;
    }

    /* Overlap of regions: must care of directions of copy in order not to loose data */
    /* Depending on the location of the overlap, there will be 2 directions of copy with
       a particular point where the direction change. */

    if (BitsSLeft > BitsDLeft)
    {
        ShiftLeft = FALSE;
    }
    else
    {
        ShiftLeft = TRUE;
    }

    /* Particular case 1 byte to 1 byte */
    if (Width == 1)
    {
        FirstDMask = (FirstDMask & LastDMask);
        FirstSMask = (FirstSMask & LastSMask);
    }

    /* Start searching from first addresses */
/*    ChangeDirectionPoint = 1;*/
    Diff1 = (((U32) MirrorSrcAddr_p) - FirstAddrDest);
    Diff2 = (FirstAddrSrc - ((U32) MirrorDestAddr_p));
    if (Diff1 < 0)
    {
        Diff1 = -Diff1;
    }
    if (Diff2 < 0)
    {
        Diff2 = -Diff2;
    }

    if (Diff2 > Diff1)
    {
        MaxDiff = (S32)(FirstAddrSrc - ((U32) MirrorDestAddr_p));
/*        FirstAddrSrc = ((U32) SrcAddr_p);
        FirstAddrDest = ((U32) MirrorDestAddr_p));*/
    }
    else
    {
        MaxDiff = (S32)(((U32) MirrorSrcAddr_p) - FirstAddrDest);
/*        FirstAddrSrc = ((U32) MirrorSrcAddr_p);
        FirstAddrDest = ((U32) DestAddr_p));*/
    }

    /*if (FirstAddrDest < FirstAddrSrc)*/
    if (MaxDiff > 0)
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
        } while ((FirstAddrDest + ((ChangeDirectionPoint - 1) * DestPitch)) < (FirstAddrSrc + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from height down to ChangeDirectionPoint and from 1 up to ChangeDirectionPoint */
        /* From 1 to ChangeDirectionPoint-1, copy forward direction */
        for (y = 1; y < ChangeDirectionPoint; y++)
        {
            MaskedCopyLineUpAndMirror(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft, ((U32) MirrorSrcAddr_p), ((U32) MirrorDestAddr_p));
        }

        /* Last line eventually... */
        if (MirrorSrcHeight > Height)
        {
            MaskedCopyLineDown(Height + 1, ((U32) MirrorSrcAddr_p), Width, SrcPitch, ((U32) MirrorDestAddr_p), DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
/*            for (x = Width; x > 0; x--)
            {
                *(((U8 *) MirrorDestAddr_p) + (Height * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + (Height * SrcPitch) + (x - 1));
            }*/
        }
        if (SrcHeight > Height)
        {
            MaskedCopyLineDown(Height + 1, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
/*            for (x = Width; x > 0; x--)
            {
                *(((U8 *) FirstAddrDest) + (Height * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + (Height * SrcPitch) + (x - 1));
            }*/
        }

        /* From ChangeDirectionPoint to SrcHeight, copy backward direction */
        for (y = SrcHeight; y >= ChangeDirectionPoint; y--)
        {
            MaskedCopyLineDownAndMirror(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft, ((U32) MirrorSrcAddr_p), ((U32) MirrorDestAddr_p));
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
        } while ((FirstAddrDest + ((ChangeDirectionPoint - 1) * DestPitch)) > (FirstAddrSrc + ((ChangeDirectionPoint - 1) * SrcPitch)));

        /* 1 <= ChangeDirectionPoint <= Height+1 */
        /* Now, go in 2 directions from ChangeDirectionPoint up to height and down to 1 */
        /* From 1 to ChangeDirectionPoint-1, copy backward direction */
        for (y = ChangeDirectionPoint - 1; y > 0; y--)
        {
            MaskedCopyLineDownAndMirror(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft, ((U32) MirrorSrcAddr_p), ((U32) MirrorDestAddr_p));
        }
        /* From ChangeDirectionPoint to SrcHeight, copy forward direction */
        for (y = ChangeDirectionPoint; y <= SrcHeight; y++)
        {
            MaskedCopyLineUpAndMirror(y, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft, ((U32) MirrorSrcAddr_p), ((U32) MirrorDestAddr_p));
        }


        /* Last line eventually... */
        if (MirrorSrcHeight > Height)
        {
            MaskedCopyLineUp(Height + 1, ((U32) MirrorSrcAddr_p), Width, SrcPitch, ((U32) MirrorDestAddr_p), DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
/*            for (x = 1; x <= Width; x++)
            {
                *(((U8 *) MirrorDestAddr_p) + (Height * DestPitch) + (x - 1)) = *(((U8 *) MirrorSrcAddr_p) + (Height * SrcPitch) + (x - 1));
            }*/
        }
        if (SrcHeight > Height)
        {
            MaskedCopyLineUp(Height + 1, FirstAddrSrc, Width, SrcPitch, FirstAddrDest, DestPitch, BitsSLeft, BitsDLeft, FirstSMask, LastSMask, FirstDMask, LastDMask, ShiftLeft);
/*            for (x = 1; x <= Width; x++)
            {
                *(((U8 *) FirstAddrDest) + (Height * DestPitch) + (x - 1)) = *(((U8 *) FirstAddrSrc) + (Height * SrcPitch) + (x - 1));
            }*/
        }

    }

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : STAVMEM_MaskedFillBlock1D
Description : Fill 1D with masks for first and last bytes
Parameters  : id fill 1D + mirror source and destination + bits to consider to the left and right
Assumptions : BitsDRight and BitsDLeft are 0 to 8.
Limitations : Mirror not implemented as block handle for now !!!
Returns     :
*******************************************************************************/
ST_ErrorCode_t STAVMEM_MaskedFillBlock1D(void *Pattern_p, U32 PatternSize, void *DestAddr_p, U32 DestSize, U8 BitsDLeft, U8 BitsDRight)
{
    U32 IndexDest, IndexPat;
    U32 FirstAddrPattern = ((U32) Pattern_p), FirstAddrDest = ((U32) DestAddr_p);
    U8 TempVal, LeftTempVal, RightTempVal, WriteVal;
    U8 FirstDMask = ((1 << (    BitsDLeft )) - 1); /* 1 gives 00000001 */
    U8 LastDMask = ~((1 << (8 - BitsDRight)) - 1); /* 3 gives 11100000 */

    /* Ensure pattern is not inside the copy area */
    if (RangesOverlap(FirstAddrPattern, FirstAddrPattern + PatternSize - 1, FirstAddrDest, FirstAddrDest + DestSize - 1))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Be careful: works only if the pattern is not inside the copy area !!! */

    /* Particular case size is 1 */
    if (DestSize == 1)
    {
        FirstDMask = (FirstDMask & LastDMask);
        TempVal = *(((U8 *) FirstAddrPattern));
        WriteVal = (*(((U8 *) FirstAddrDest))) & ~FirstDMask;
        WriteVal |= TempVal & FirstDMask;
        /* Write destination */
        *(((U8 *) FirstAddrDest)) = WriteVal;
    }
    else
    {
        /* No particular pattern size: this is the general case */
        /* First byte alone */

        /* Read destination because it has to be masked */
        RightTempVal = *(((U8 *) FirstAddrPattern) + (1 % PatternSize));
        TempVal = *(((U8 *) FirstAddrPattern));
        LeftTempVal = 0;
        WriteVal = (*(((U8 *) FirstAddrDest))) & ~FirstDMask;
        WriteVal |= (TempVal >> (8 - BitsDLeft)) & FirstDMask;
        /* Write destination */
        *(((U8 *) FirstAddrDest)) = WriteVal;

        IndexDest = 1;
        IndexPat = 0;
        while (IndexDest < DestSize - 1)
        {
            IndexPat++;
            if (IndexPat >= PatternSize)
            {
                IndexPat = 0;
            }
            IndexDest++;
            LeftTempVal = TempVal;
            TempVal = RightTempVal;
            RightTempVal = *(((U8 *) FirstAddrPattern) + IndexPat);

            /* x going up */
            WriteVal = (TempVal >> (8 - BitsDLeft)) | (LeftTempVal << (BitsDLeft));
            *(((U8 *) FirstAddrDest) + (IndexDest - 1)) = WriteVal;
        }
        IndexDest++;

        /* Last byte alone */
        /* Read and mask source */
        LeftTempVal = TempVal;
        TempVal = RightTempVal;
        RightTempVal = 0;
        /* Read destination because it has to be masked */
        WriteVal = (*(((U8 *) FirstAddrDest) + (DestSize - 1))) & ~LastDMask;
        WriteVal |= ((TempVal >> (8 - BitsDLeft)) | (LeftTempVal << (BitsDLeft))) & LastDMask;
        /* Write back destination */
        *(((U8 *) FirstAddrDest) + (DestSize - 1)) = WriteVal;
    }

    return(ST_NO_ERROR);
}

#endif /* OSD_CODE */


/* End of avm_acc.c */
