/*******************************************************************************

File name : avm_init.c

Description : Audio and video memory standard API functions source file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
17 jun 1999         Created                                          HG
 6 Oct 1999         Suppressed Open/Close and introduce
                    partitions functions                             HG
13 Oct 1999         Added base addresses as init params              HG
10 Jan 2000         Added video, SDRAM base addresses and size,
                    and cached memory map in init params.
                    Moved away init related to mem access module.    HG
 9 Feb 2000         Corrected problem with too long device names     HG
02 Apr 2001         Added DeviceType and vitual mapping for shared
                    memory.                                          HSdLM
07 Jun 2001         Add stavmem_ before non API exported symbols     HSdLM
 *                  terminate as much as possible. Fix issue in
 *                  GetCapability.
05 Jul 2001         Remove dependency upon STGPDMA if GPDMA method   HSdLM
 *                  is not available (compilation flag). Terminate
 *                  as much as possible, even if errors. Make
 *                  GetCapability more robust.
20 Nov 2002         Add support of FDMA copy method.                 HSdLM
17 Apr 2003         Multi-partitions support                         HSdLM
31 Jan 2005         Port to Linux                                    ATa
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#ifndef ST_OSLINUX
#include <stdlib.h>
#include <string.h>
#endif

#ifdef ST_OSLINUX
#include <linux/version.h>

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,0))
#include <linux/page-flags.h>
#else
#include <linux/wrapper.h>
#endif

#include "stavmem_core_types.h"
/*#else */
#endif  /* ST_OSLINUX */

#include "sttbx.h"

#if defined(ST200_OS21)
#include <os21/st200/mmap.h>
#endif /* ST200_OS21 */


#include "stddefs.h"
#include "avmem_rev.h"
#include "stavmem.h"
#include "avm_init.h"
#include "avm_allo.h"
#include "avm_acc.h"
#include "stos.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

#define INVALID_DEVICE_INDEX (-1)


/* Private Variables (static)------------------------------------------------ */

static stavmem_Device_t DeviceArray[STAVMEM_MAX_DEVICE];
static semaphore_t *InstancesAccessControl_p;   /* Init/Open/Close/Term protection semaphore */
static BOOL FirstInitDone = FALSE;


/* Global Variables --------------------------------------------------------- */
#if defined(ST200_OS21)
/* Variable needed by the STSYS driver to remember the offset to add to a physical address
 * to have a CPU uncached address to the memory. This is more efficient
 * than calling mmap_translate_uncached every time we make an uncached
 * access */
S32 STSYS_ST200UncachedOffset;
#endif /* defined(ST200_OS21) */


/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    extern  U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif
/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(stavmem_Device_t * const Device_p, const STAVMEM_InitParams_t * const InitParams_p);
static ST_ErrorCode_t Term(stavmem_Device_t * const Device_p);


/* Functions ---------------------------------------------------------------- */
#if defined (ST_OSWINCE) && defined (DEBUG)
/*******************************************************************************
Name        : STAVMEM_CPUToVirtual
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_CPUToVirtual(void* Address,
						   STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{
	U32 VirtAddr;

	VirtAddr = (U32)(Address)
                - (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromCPU_p)
                + (U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p)
                + (U32)((ShMemVirtualMap_p)->VirtualWindowOffset);

	return ((void*) VirtAddr);
}
/*******************************************************************************
Name        : STAVMEM_VirtualToCPU
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_VirtualToCPU (void* Address,
						   STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{
	U32 VirtAddr;


/*  printf (" Virtual To Cpu %x\n", Address);*/


	VirtAddr = (U32)(Address)
                - (U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p)
                + (U32)((ShMemVirtualMap_p)->VirtualWindowOffset)
                + (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromCPU_p);

	return ((void*) VirtAddr);
}
/*******************************************************************************
Name        : STAVMEM_VirtualToDevice
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_VirtualToDevice(void* Address,
							  STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{
    U32 DevAddr;
	DevAddr = 	(U32)(Address)
				- (U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p)
				+ (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromDevice_p);


/* printf (" Virt To Dev %x %x \n", Address, DevAddr);*/
	return ((void *)DevAddr);
}
/*******************************************************************************
Name        : STAVMEM_CPUToDevice
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_CPUToDevice(void* Address,
							  STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{
    U32 DevAddr;
	DevAddr = 	(U32)(Address)
				- (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromCPU_p)
				+ (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromDevice_p);


/*  printf (" Cpu To Dev %x %x \n", Address, DevAddr);*/

	return ((void *)DevAddr);
}
/*******************************************************************************
Name        : STAVMEM_DeviceToCPU
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_DeviceToCPU(void* Address,
							  STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{
    U32 CpuAddr;
	CpuAddr = 	(U32)(Address)
				- (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromDevice_p)
				+ (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromCPU_p);


/*  printf (" Dev To Cpu %x %x \n", Address, CpuAddr);*/

	return ((void *)CpuAddr);
}
/*******************************************************************************
Name        : STAVMEM_DeviceToVirtual
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_DeviceToVirtual(void* Address,
							  STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{

	U32 VirtAddr;
	VirtAddr = 	(U32)(Address)
				- (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromDevice_p)
				+ (U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p);                                           \


/*  printf (" DevToVirt %x %x \n", Address, VirtAddr);*/

	return ((void *)VirtAddr);

}
/*******************************************************************************
Name        : STAVMEM_VirtualToDevice2
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_VirtualToDevice2(void* Address,
							   STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{

	U32 DevAddr;
	DevAddr = 	(U32)(Address)
				- (U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p)
				+ (U32)((ShMemVirtualMap_p)->PhysicalAddressSeenFromDevice2_p);

  /*printf (" VirtToDev2 %x %x \n", Address, DevAddr);*/

	return ((void *)DevAddr);
}
/*******************************************************************************
Name        : STAVMEM_IsAddressVirtual
Description :
Parameters  :
*******************************************************************************/
char STAVMEM_IsAddressVirtual (void* Address,
							   STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{

  /*printf ("IsAddressVirtual 0x%x\n", Address);*/

	return (
				((U32)(Address) >= (U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p))
             && ((U32)(Address) < ((U32)( (ShMemVirtualMap_p)->VirtualBaseAddress_p)
                                            + (ShMemVirtualMap_p)->VirtualSize))
                                                );
}
/*******************************************************************************
Name        : STAVMEM_IsDataInVirtualWindow
Description :
Parameters  :
*******************************************************************************/
char STAVMEM_IsDataInVirtualWindow (void* Address, int Size,
							   STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{
   /*printf ("IsDataInVirtualWdw\n");*/

	return (
			( (U32)(Address) >= ((U32)((ShMemVirtualMap_p)->VirtualBaseAddress_p)
									+ (ShMemVirtualMap_p)->VirtualWindowOffset ) ) )
				 && ( (U32)(Address) + (Size) <= (  (U32) ((ShMemVirtualMap_p)->VirtualBaseAddress_p)
                                                + (ShMemVirtualMap_p)->VirtualWindowOffset
                                                + (ShMemVirtualMap_p)->VirtualWindowSize )
												);

}
/*******************************************************************************
Name        : STAVMEM_IfVirtualThenToCPU
Description :
Parameters  :
*******************************************************************************/
void* STAVMEM_IfVirtualThenToCPU (void* Address,
							   STAVMEM_SharedMemoryVirtualMapping_t *ShMemVirtualMap_p)
{

  /*printf ("IfVirtualToCPu 0x%x\n", Address);*/

	if ( STAVMEM_IsAddressVirtual(Address, ShMemVirtualMap_p) )
		return (STAVMEM_VirtualToCPU(Address, ShMemVirtualMap_p));
	else
		return Address;

}
#endif /*ST_OSWINCE*/

/*******************************************************************************
Name        : EnterCriticalSection
Description : Used to protect critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void EnterCriticalSection(void)
{
    static BOOL InstancesAccessControlInitialized = FALSE;
    STOS_TaskLock();

    if (!InstancesAccessControlInitialized)
    {
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);
        InstancesAccessControlInitialized = TRUE;
    }

    STOS_TaskUnlock();

    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl_p);
} /* End of EnterCriticalSection() function */

/*******************************************************************************
Name        : LeaveCriticalSection
Description : Used to release protection of critical sections of Init/Open/Close/Term
Parameters  : None
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void LeaveCriticalSection(void)
{
    /* Release the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreSignal(InstancesAccessControl_p);
} /* End of LeaveCriticalSection() function */

/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 WantedIndex = INVALID_DEVICE_INDEX, Index = 0;

    do
    {
        /* Device initialised: check if name is matching */
        if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
        {
            /* Name found in the initialised devices */
            WantedIndex = Index;
        }
        Index++;
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STAVMEM_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */


/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_NO_MEMORY, STAVMEM_ERROR_GPDMA_OPEN
*******************************************************************************/
static ST_ErrorCode_t Init(stavmem_Device_t * const Device_p, const STAVMEM_InitParams_t * const InitParams_p )
{
    U32 TotalPartitionsBlocks, TotalBlocks;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    stavmem_MemAccessInitParams_t MemAccessParams;

#if defined(ST200_OS21)
    STSYS_ST200UncachedOffset = (S32)mmap_translate_uncached(&STSYS_ST200UncachedOffset) - ((S32)(&STSYS_ST200UncachedOffset));
#endif /* ST200_OS21 */

    /* Save constants */
    Device_p->MaxPartitions = InitParams_p->MaxPartitions;
    Device_p->MaxUsedBlocks = InitParams_p->MaxBlocks;
    Device_p->CurrentlyUsedBlocks = 0;
    Device_p->MaxForbiddenRanges = InitParams_p->MaxForbiddenRanges;
    Device_p->MaxForbiddenBorders = InitParams_p->MaxForbiddenBorders;

    /* Total number of blocks per partition can be potentially:
        + (2*Init->MaxBlocks)+1 blocks alterning used/unused blocks,
        + (2*MaxNumberOfMemoryMapRanges)+1 blocks because of reserved/sub-space blocks,
        + 2*MaxForbiddenRanges blocks when marking forbidden blocks for allocation,
        + MaxForbiddenBorders blocks when cutting unused blocks in 2 for allocation */
    TotalPartitionsBlocks = 2 * (InitParams_p->MaxBlocks +
                                 InitParams_p->MaxNumberOfMemoryMapRanges +
                                 Device_p->MaxForbiddenRanges + 1)
                            + Device_p->MaxForbiddenBorders;
    TotalBlocks = TotalPartitionsBlocks * Device_p->MaxPartitions;
    Device_p->TotalAllocatedBlocks = TotalBlocks;

    /* Allocate memory for internal use: for as much block structures as required */

#ifdef ST_OSLINUX
    Device_p->MemoryAllocated_p = STOS_MemoryAllocate(NULL, TotalBlocks * sizeof(MemBlockDesc_t));
#else
    Device_p->MemoryAllocated_p = STOS_MemoryAllocate(InitParams_p->CPUPartition_p, TotalBlocks * sizeof(MemBlockDesc_t));
#endif

    AddDynamicDataSize(TotalBlocks * sizeof(MemBlockDesc_t));
    if (Device_p->MemoryAllocated_p == NULL)
    {
        /* If allocation failed, quit with failure */
        return(ST_ERROR_NO_MEMORY);
    }


    /* Save the partition where the memory allocation for internal use was done */
#ifdef ST_OSLINUX
    Device_p->CPUPartition_p = NULL;
#else
    Device_p->CPUPartition_p = InitParams_p->CPUPartition_p;
#endif
    /* Remember first block address to test block handle validity */
    Device_p->FirstBlock_p = Device_p->MemoryAllocated_p;

    /* Initialise allocations locking semaphore */
    Device_p->LockAllocSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);    /* Create allocations locking semaphore */

    /* Initialise memory accesses */
    MemAccessParams.DeviceType                   = InitParams_p->DeviceType;
#ifdef ST_OSLINUX
    MemAccessParams.CPUPartition_p               = NULL;
    MemAccessParams.NCachePartition_p            = NULL;
#else
    MemAccessParams.CPUPartition_p               = InitParams_p->CPUPartition_p;
    MemAccessParams.NCachePartition_p            = InitParams_p->NCachePartition_p;
#endif


    /* no Block Move DMA on 5516/17/28/GX1/5100 */
    MemAccessParams.BlockMoveDmaBaseAddr_p       = InitParams_p->BlockMoveDmaBaseAddr_p;

#ifdef ST_OSLINUX
    MemAccessParams.CacheBaseAddr_p              = MapBaseAddress( InitParams_p->CacheBaseAddr_p, 0x1000 );
    MemAccessParams.VideoBaseAddr_p              = MapBaseAddress( InitParams_p->VideoBaseAddr_p, 0x1000 );
#else
    MemAccessParams.VideoBaseAddr_p              = InitParams_p->VideoBaseAddr_p;
#endif

    MemAccessParams.SDRAMBaseAddr_p              = InitParams_p->SDRAMBaseAddr_p;

    MemAccessParams.SDRAMSize                    = InitParams_p->SDRAMSize;

    MemAccessParams.SharedMemoryVirtualMapping_p = InitParams_p->SharedMemoryVirtualMapping_p;

    MemAccessParams.NumberOfDCachedAreas         = InitParams_p->NumberOfDCachedRanges;
    MemAccessParams.DCachedRanges_p              = InitParams_p->DCachedRanges_p;
    MemAccessParams.OptimisedMemAccessStrategy_p = InitParams_p->OptimisedMemAccessStrategy_p;
#ifdef STAVMEM_MEM_ACCESS_GPDMA
    strcpy(MemAccessParams.GpdmaName, InitParams_p->GpdmaName);
    MemAccessParams.GpdmaName[ST_MAX_DEVICE_NAME - 1] = '\0';
#endif /* STAVMEM_MEM_ACCESS_GPDMA */
    Err = stavmem_MemAccessInit(&MemAccessParams);

    return(Err);
} /* end of Init() */

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_NO_ERROR, STAVMEM_ERROR_GPDMA_CLOSE
*******************************************************************************/
static ST_ErrorCode_t Term(stavmem_Device_t * const Device_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;
/*    stavmem_MemAccessTermParams_t MemAccessTermParams;*/

    /* Terminate memory accesses */
    Err = stavmem_MemAccessTerm(); /* Err = stavmem_MemAccessTerm(&MemAccessTermParams); */

    /* De-allocate memory allocated for internal use at init() */
    STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->MemoryAllocated_p);

    /* Delete allocations locking semaphore */
    STOS_SemaphoreDelete(NULL, Device_p->LockAllocSemaphore_p);

    return(Err);
} /* end of Term() */

/*******************************************************************************
Name        : stavmem_GetPointerOnDeviceNamed
Description : Get the pointer in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions :
Limitations :
Returns     : the pointer if the name was found, NULL otherwise
*******************************************************************************/
stavmem_Device_t *stavmem_GetPointerOnDeviceNamed(const ST_DeviceName_t WantedName)
{
    S32 DeviceIndex;

    /* Check if device already initialised and return error if not so */
    DeviceIndex = GetIndexOfDeviceNamed(WantedName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        return(NULL);
    }
    else
    {
        /* Device name found */
        return(&DeviceArray[DeviceIndex]);
    }
} /* end of stavmem_GetPointerOnDeviceNamed() */


/*
--------------------------------------------------------------------------------
Get revision of the STAVMEM API
--------------------------------------------------------------------------------
*/
ST_Revision_t STAVMEM_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
    */
    return(AVMEM_Revision);
}


/*
--------------------------------------------------------------------------------
Get capabilities of the STAVMEM API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_GetCapability(const ST_DeviceName_t DeviceName, STAVMEM_Capability_t * const Capability_p)
{
    stavmem_Device_t *Device_p;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                        /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        Device_p = stavmem_GetPointerOnDeviceNamed(DeviceName);
        if (Device_p == NULL)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Fill capability structure */
            Capability_p->MaxPartition = Device_p->MaxPartitions;
            #ifdef STAVMEM_NO_COPY_FILL
                Capability_p->IsCopyFillCapable = FALSE;
            #else
                Capability_p->IsCopyFillCapable = TRUE;
            #endif
            Capability_p->FillBlock1DPatternSize = NULL;
            Capability_p->FillBlock2DPatternHeight[0] = 1;
            Capability_p->FillBlock2DPatternHeight[1] = 0;
            Capability_p->FillBlock2DPatternWidth = NULL;


        }
    }
    LeaveCriticalSection();

    return(Err);
} /* End of STAVMEM_GetCapability() function */


/*
--------------------------------------------------------------------------------
Initialise the STAVMEM API
(Standard instances management. Driver specific implementations should be put in Init() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_Init(const ST_DeviceName_t DeviceName, const STAVMEM_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;
    /* Exit now if parameters are invalid */
    if ((InitParams_p == NULL) ||                           /* Some init parameters must be specified */
#ifndef ST_OSLINUX
        (InitParams_p->CPUPartition_p == NULL) ||           /* A CPU partition must be specified */
#endif
#ifdef STAVMEM_MEM_ACCESS_FDMA
        (InitParams_p->NCachePartition_p == NULL) ||        /* A NCache partition must be specified */
#endif
        (!(InitParams_p->MaxNumberOfMemoryMapRanges > 0)) ||   /* At least one range must be specified */
        (!(InitParams_p->MaxPartitions > 0)) ||             /* At least one partition must be allowed */
        (!(InitParams_p->MaxPartitions <= STAVMEM_MAX_MAX_PARTITION)) || /* MaxPartitions is limited */
        (!(InitParams_p->MaxBlocks > 0)) ||                 /* At least one block must be allowed */
#ifdef STAVMEM_MEM_ACCESS_GPDMA
        (strlen(InitParams_p->GpdmaName) > ST_MAX_DEVICE_NAME - 1) ||
        ((((const char *) InitParams_p->GpdmaName)[0]) == '\0') ||
#endif
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) ||    /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')          /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    switch (InitParams_p->DeviceType)
    {
        case STAVMEM_DEVICE_TYPE_GENERIC :
            if (!(InitParams_p->SDRAMSize >= 16))           /* SDRAM size must not be 0 nor too small */
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;
        case STAVMEM_DEVICE_TYPE_VIRTUAL :
            if (InitParams_p->SharedMemoryVirtualMapping_p == NULL)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
            break;
        default :
            return(ST_ERROR_BAD_PARAMETER);
            break;
    }

    EnterCriticalSection();

    /* First init: initialise devices and partitions as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < STAVMEM_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < STAVMEM_MAX_MAX_PARTITION; Index++)
        {
            stavmem_PartitionArray[Index].Validity = ~AVMEM_VALID_PARTITION;
            stavmem_PartitionArray[Index].VeryTopBlock_p = NULL;       /* Not to address any block */
            stavmem_PartitionArray[Index].VeryBottomBlock_p = NULL;    /* Not to address any block */
        }
        /* Process this only once */
        FirstInitDone = TRUE;
    }

    /* Check if device already initialised and return error if so */
    if (GetIndexOfDeviceNamed(DeviceName) != INVALID_DEVICE_INDEX)
    {
        /* Device name found */
        Err = ST_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
        /* Look for a non-initialised device and return error if none is available*/
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STAVMEM_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STAVMEM_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            Err = Init(&DeviceArray[Index], InitParams_p);

            if (Err == ST_NO_ERROR)
            {
                /* Device available and not already initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    return(Err);
} /* End of STAVMEM_Init() function */


/*
--------------------------------------------------------------------------------
Terminate the STAVMEM API
(Standard instances management. Driver specific implementations should be put in Term() function.)
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STAVMEM_Term(const ST_DeviceName_t DeviceName, const STAVMEM_TermParams_t * const TermParams_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    S32 PartitionIndex = 0;
    BOOL FoundPartitions = FALSE;
    STAVMEM_DeletePartitionParams_t DeleteParams;
    STAVMEM_PartitionHandle_t ToBeDeletedPartitionHandle;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                        /* There shoulb be term params */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((((const char *) DeviceName)[0]) == '\0')       /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    if (!FirstInitDone)
    {
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Check if device already initialised and return error if NOT so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Check if there is still created partitions on this device */
            while ((PartitionIndex < STAVMEM_MAX_MAX_PARTITION) && (!FoundPartitions))
            {
                FoundPartitions = ((stavmem_PartitionArray[PartitionIndex].Device_p == &DeviceArray[DeviceIndex]) && (stavmem_PartitionArray[PartitionIndex].Validity == AVMEM_VALID_PARTITION));
                PartitionIndex++;
            }

            if (FoundPartitions)
            {
                /* There's still created partition(s): terminate only if required. */
                if (TermParams_p->ForceTerminate)
                {
                    /* Force terminate required: close all existing partition before terminating */
                    PartitionIndex = 0;
                    DeleteParams.ForceDelete = TRUE;
                    while (PartitionIndex < STAVMEM_MAX_MAX_PARTITION)
                    {
                        if ((stavmem_PartitionArray[PartitionIndex].Device_p == &DeviceArray[DeviceIndex]) && (stavmem_PartitionArray[PartitionIndex].Validity == AVMEM_VALID_PARTITION))
                        {
                            /* Partition is created and belongs to the device: delete it */
                            ToBeDeletedPartitionHandle = (STAVMEM_PartitionHandle_t) &stavmem_PartitionArray[PartitionIndex];
                            Err = STAVMEM_DeletePartition(DeviceName, &DeleteParams, &ToBeDeletedPartitionHandle);
                            if (Err != ST_NO_ERROR)
                            {
                                /* Problem: this should not happen
                                Fail to terminate ? No because of force */
                                Err = ST_NO_ERROR;
                            }
                        }
                        PartitionIndex++;
                    }
                }
                else
                {
                    /* Not required to delete partitions: return error, cannot terminate */
                    Err = STAVMEM_ERROR_CREATED_PARTITION;
                }
            }

            /* Terminate if OK */
            if (Err == ST_NO_ERROR)
            {
                /* API specific terminations */
                Err = Term(&DeviceArray[DeviceIndex]);
                /* Don't leave instance semi-terminated: terminate as much as possible */
                /* free device */
                DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
} /* End of STAVMEM_Term() function */


/* End of avm_init.c */


