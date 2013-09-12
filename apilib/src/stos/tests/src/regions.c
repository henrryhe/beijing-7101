/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : regions.c
Description : STOS regions specific tests

Note        :

Date          Modification                                    Initials
----          ------------                                    --------
Mar 2007      Creation                                        CD

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include "regions.h"
#include "stdevice.h"
#include "sttbx.h"
#include "stsys.h"

#ifdef ST_OSLINUX
#if !defined MODULE
#include "stostest_ioctl.h"
#endif
#endif

/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Private Constants --- */
#if defined (ST_7200)
#define TEST_MAPPED_MEM_ADDRESS     0x10000000  /* LMI_VID */

#define TEST_MAPPED_REG_ADDRESS     TTXT_BASE_ADDRESS
#define TEST_READ_ONLY_ADDRESS      VIDEO_BASE_ADDRESS
#define TEST_READ_WRITE_OFFSET      0x000
#define TEST_READ_ONLY_OFFSET       0x180

#elif defined (ST_7100) || defined (ST_7109)
#define TEST_MAPPED_MEM_ADDRESS     0x10000000  /* LMI_VID */

#define TEST_MAPPED_REG_ADDRESS     (TTXT_BASE_ADDRESS & ~(0xE0000000))
#define TEST_READ_ONLY_ADDRESS      (VIDEO_BASE_ADDRESS & ~(0xE0000000))
#define TEST_READ_WRITE_OFFSET      0x100
#define TEST_READ_ONLY_OFFSET       0x70

#elif defined (ST_7710)
#define TEST_MAPPED_MEM_ADDRESS     0x10000000  /* LMI_VID */

#define TEST_MAPPED_REG_ADDRESS     TTXT_BASE_ADDRESS
#define TEST_READ_ONLY_ADDRESS      VIDEO_BASE_ADDRESS
#define TEST_READ_WRITE_OFFSET      0x100   /* To be verified */
#define TEST_READ_ONLY_OFFSET       0x70    /* To be verified */

#endif

#define TEST_MAPPED_LENGTH          0x1000
#define MAP_REGISTER_TEST_PATTERN   0xA5A5A5A5


/* --- Extern structure prototypes --- */

/* --- Private structure prototypes --- */

/* --- Global variables --- */

/* --- Extern variables --- */

/* --- Private variables --- */

/* --- Extern functions prototypes --- */

/* --- Private function prototypes --- */

/*******************************************************************************
Name        : Check_PhysicalAddress
Description :
Returns     :
*******************************************************************************/
static STOS_Error_t Check_PhysicalAddress(U32 pPtr)
{
#ifdef ADDRESS_MODE_32BITS
    /* How to check physical address ??? */
    return STOS_SUCCESS;
#else
    return (((pPtr & 0xE0000000) || (pPtr == (U32)NULL)) ? STOS_FAILURE : STOS_SUCCESS);
#endif
}

/*#######################################################################
 *                             Exported functions
 *#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : TestMapRegister_write_read
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestMapRegister_write_read(void)
{
    STOS_Error_t            Error = STOS_SUCCESS;
    U32                   * vMapped_p = NULL;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* First test: Writes a read-only register then read/writes a read write register */
    STTBX_Print(("First test: Writes a read-only register then read/writes a read write register\n"));
    vMapped_p = (U32 *)STOS_MapRegisters((void *)TEST_READ_ONLY_ADDRESS, (U32)TEST_MAPPED_LENGTH, "Map register test");
    if (vMapped_p != NULL)
    {
#if defined ST_OSLINUX && !defined MODULE
        /* Linux user space is failing */
        STTBX_Print(("Not yet implemented for user space !!!\n"));
        Error = STOS_FAILURE;
        goto error;

#else

        U32     read;

        STTBX_Print(("Adress: 0x%.8x successfully mapped at: 0x%.8x\n", (U32)TEST_READ_ONLY_ADDRESS, (U32)vMapped_p));

        STSYS_WriteRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_ONLY_OFFSET), 0x00000000);
        read = STSYS_ReadRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_ONLY_OFFSET));
        if (read != 0x00000000)
        {
            STTBX_Print(("Initial value of RO register is not 0 ([0x%.8x]=0x%.8x)\n", (U32)vMapped_p + TEST_READ_ONLY_OFFSET, read));
            Error = STOS_FAILURE;
            goto error;
        }
        STSYS_WriteRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_ONLY_OFFSET), MAP_REGISTER_TEST_PATTERN);
        read = STSYS_ReadRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_ONLY_OFFSET));
        if (read != 0x00000000)
        {
            /* Pattern should NOT have been written: error ! */
            STTBX_Print(("Pattern should NOT have been written ([0x%.8x]=0x%.8x)\n", (U32)vMapped_p + TEST_READ_ONLY_OFFSET, read));
            Error = STOS_FAILURE;
            goto error;
        }



        STSYS_WriteRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_WRITE_OFFSET), 0x00000000);
        read = STSYS_ReadRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_WRITE_OFFSET));
        if (read != 0x00000000)
        {
            STTBX_Print(("Initial value of R/W register is not 0 ([0x%.8x]=0x%.8x)\n", (U32)vMapped_p + TEST_READ_WRITE_OFFSET, read));
            Error = STOS_FAILURE;
            goto error;
        }
        STSYS_WriteRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_WRITE_OFFSET), MAP_REGISTER_TEST_PATTERN);
        read = STSYS_ReadRegDev32LE((void *) ((U32)vMapped_p + TEST_READ_WRITE_OFFSET));
        if (read != MAP_REGISTER_TEST_PATTERN)
        {
            /* Pattern should have been written: error ! */
            STTBX_Print(("Pattern should have been written ([0x%.8x]=0x%.8x)\n", (U32)vMapped_p + TEST_READ_WRITE_OFFSET, read));
            Error = STOS_FAILURE;
            goto error;
        }
#endif

        STOS_UnmapRegisters(vMapped_p, TEST_MAPPED_LENGTH);
        vMapped_p = NULL;
    }
    else
    {
        STTBX_Print(("Could not map registers !!!\n"));
        Error = STOS_FAILURE;
        goto error;
    }
    STTBX_Print(("\n"));

    return Error;



error:
    if (vMapped_p != NULL)
    {
        STOS_UnmapRegisters(vMapped_p, TEST_MAPPED_LENGTH);
    }

    STTBX_Print(("Error !!!\n"));
    return Error;
}

/*-------------------------------------------------------------------------
 * Function : TestUnmapRegister_aftermap
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestUnmapRegister_aftermap(void)
{
    return STOS_FAILURE;    /* No yet managed */
}

/*-------------------------------------------------------------------------
 * Function : TestUnmapRegister_nomap
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestUnmapRegister_nomap(void)
{
    return STOS_FAILURE;    /* No yet managed */
}

/*-------------------------------------------------------------------------
 * Function : TestVirtToPhys_correct_address
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestVirtToPhys_correct_address(void)
{
#if defined ST_OSLINUX && !defined MODULE
    STOSTEST_MapParams_t    STOSTEST_MapParams;
#endif
    STOS_Error_t            Error = STOS_SUCCESS;
    void                  * pPtr = NULL;
    U32                   * vAllocated_p = NULL;
    U32                   * vMapped_p = NULL;
    U32                   * vMapped2_p = NULL;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    /* First test: Get virt to Phys from memory allocate */
    STTBX_Print(("First test: Get virt to Phys from memory allocate\n"));
    vAllocated_p = (U32 *)STOS_MemoryAllocate(NULL, 8000);   /* Allocates sizes in byte */
    if (vAllocated_p != NULL)
    {
        pPtr = STOS_VirtToPhys(vAllocated_p);
        STTBX_Print(("Virt address:0x%.8x => Phys:0x%.8x\n", (U32)vAllocated_p, (U32)pPtr));
        if (Check_PhysicalAddress((U32)pPtr) != STOS_SUCCESS)
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        STOS_MemoryDeallocate(NULL, vAllocated_p);
        vAllocated_p = NULL;
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }
    STTBX_Print(("\n"));



    /* Second test: Get virt to Phys from register mapping */
    STTBX_Print(("Second test: Get virt to Phys from register mapping\n"));
    vMapped_p = (U32 *)STOS_MapPhysToUncached((void *)TEST_MAPPED_REG_ADDRESS, (U32)TEST_MAPPED_LENGTH);
    if (vMapped_p != NULL)
    {
        pPtr = STOS_VirtToPhys(vMapped_p);
        STTBX_Print(("Virt address:0x%.8x => Phys:0x%.8x (Orig:0x%.8x)\n", (U32)vMapped_p, (U32)pPtr, (U32)TEST_MAPPED_REG_ADDRESS));
        if (Check_PhysicalAddress((U32)pPtr) != STOS_SUCCESS)
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        STOS_UnmapPhysToUncached(vMapped_p, TEST_MAPPED_LENGTH);
        vMapped_p = NULL;
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }
    STTBX_Print(("\n"));



    /* Third test: Get virt to Phys from same register mapping */
    STTBX_Print(("Third test: Get virt to Phys from same register mapping\n"));
    vMapped_p = (U32 *)STOS_MapPhysToUncached((void *)TEST_MAPPED_REG_ADDRESS, (U32)TEST_MAPPED_LENGTH);
    if (vMapped_p != NULL)
    {
        /* Re-mapping same address */
        vMapped2_p = (U32 *)STOS_MapPhysToUncached((void *)TEST_MAPPED_REG_ADDRESS, (U32)TEST_MAPPED_LENGTH);
        if (vMapped2_p != NULL)
        {
            pPtr = STOS_VirtToPhys(vMapped2_p);
            STTBX_Print(("Virt1:0x%.8x, Virt2:0x%.8x => Phys:0x%.8x (Orig:0x%.8x)\n", (U32)vMapped_p, (U32)vMapped2_p, (U32)pPtr, (U32)TEST_MAPPED_REG_ADDRESS));
            if (Check_PhysicalAddress((U32)pPtr) != STOS_SUCCESS)
            {
                STTBX_Print(("Error !!!\n"));
                Error = STOS_FAILURE;
            }

            STOS_UnmapPhysToUncached(vMapped2_p, TEST_MAPPED_LENGTH);
            vMapped2_p = NULL;
        }
        else
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        STOS_UnmapPhysToUncached(vMapped_p, TEST_MAPPED_LENGTH);
        vMapped_p = NULL;
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }
    STTBX_Print(("\n"));



    /* Fourth test: Get virt to Phys from Phys address */
    STTBX_Print(("Fourth test: Get virt to Phys from Phys address\n"));

    pPtr = (void *)STOS_VirtToPhys(TEST_MAPPED_REG_ADDRESS);
    STTBX_Print(("Phys:0x%.8x (Orig:0x%.8x)\n", (U32)pPtr, (U32)TEST_MAPPED_REG_ADDRESS));
    if (Check_PhysicalAddress((U32)pPtr) != STOS_SUCCESS)
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }

    STTBX_Print(("\n"));



    /* Fifth test: Get virt to Phys from NOT mapped/allocated address */
    STTBX_Print(("Fifth test: Get virt to Phys from NOT mapped/allocated address\n"));

#ifdef ADDRESS_MODE_32BITS
    STTBX_Print(("Not done ...\n"));

#else
    pPtr = (void *)STOS_VirtToPhys(TEST_MAPPED_REG_ADDRESS | 0xA0000000);
    STTBX_Print(("Phys:0x%.8x (Orig:0x%.8x)\n", (U32)pPtr, ((U32)TEST_MAPPED_REG_ADDRESS) | 0xA0000000));
    if (Check_PhysicalAddress((U32)pPtr) != STOS_SUCCESS)
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }
#endif  /* !ADDRESS_MODE_32BITS */

    STTBX_Print(("\n"));



#if defined ST_OSLINUX && !defined MODULE
    /* Sixth test: Get virt to Phys from mmapped address (Linux user only) */
    STTBX_Print(("Sixth test: Get virt to Phys from mmapped address (Linux user only)\n"));

    STOSTEST_MapParams.MappedBaseAddress = NULL;
    STOSTEST_MapParams.BaseAddress = (unsigned long) TEST_MAPPED_MEM_ADDRESS;
    STOSTEST_MapParams.AddressWidth = (unsigned long) TEST_MAPPED_LENGTH;
    if (STOSTEST_MapUser(&STOSTEST_MapParams) == STOS_SUCCESS)
    {
        pPtr = STOS_VirtToPhys(STOSTEST_MapParams.MappedBaseAddress);
        STTBX_Print(("Virt address:0x%.8x => Phys:0x%.8x (Orig:0x%.8x)\n",
                            (U32)STOSTEST_MapParams.MappedBaseAddress,
                            (U32)pPtr,
                            (U32)STOSTEST_MapParams.BaseAddress));
        if (Check_PhysicalAddress((U32)pPtr) != STOS_SUCCESS)
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        STOSTEST_UnmapUser(&STOSTEST_MapParams);
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }

    STTBX_Print(("\n"));
#endif  /* LINUX & USER */


    return Error;
}

/*-------------------------------------------------------------------------
 * Function : TestInvalidateRegion_invalidate
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestInvalidateRegion_invalidate(void)
{
    STOS_Error_t    Error = STOS_SUCCESS;
    volatile U32  * CachedPtr, * UncachedPtr;   /* volatile: ptr content will be re-read each time */
    U32             ValueCached, ValueUncached;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    CachedPtr = (U32 *)STOS_MapPhysToCached((void *)TEST_MAPPED_MEM_ADDRESS, TEST_MAPPED_LENGTH);
    UncachedPtr = (U32 *)STOS_MapPhysToUncached((void *)TEST_MAPPED_MEM_ADDRESS, (U32)TEST_MAPPED_LENGTH);
    STTBX_Print(("Cached:0x%.8x, Uncached:0x%.8x\n", (U32)CachedPtr, (U32)UncachedPtr));
    if (   (CachedPtr != NULL)
        && (UncachedPtr != NULL))
    {
        *UncachedPtr = 0;
        ValueUncached = *UncachedPtr;
        ValueCached = 0;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (ValueUncached != 0)
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        *CachedPtr = 0x11111111;
        ValueUncached = *UncachedPtr;
        ValueCached = *CachedPtr;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (   (ValueCached != 0x11111111)
            || (ValueCached == ValueUncached))
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        *UncachedPtr = 0xA5A5A5A5;
        ValueUncached = *UncachedPtr;
        ValueCached = *CachedPtr;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (   (ValueUncached != 0xA5A5A5A5)
            || (ValueCached == ValueUncached))
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        STTBX_Print(("Invalidates region\n"));
        STOS_InvalidateRegion(CachedPtr, 4);

        ValueUncached = *UncachedPtr;
        ValueCached = *CachedPtr;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (   (ValueUncached != 0xA5A5A5A5)
            || (ValueCached != 0xA5A5A5A5))
        {
            STTBX_Print(("Values should be equal to 0xA5A5A5A5: Error !!!\n"));
            Error = STOS_FAILURE;
        }
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }

    if (CachedPtr != NULL)
    {
        STOS_UnmapPhysToCached((void *)CachedPtr, TEST_MAPPED_LENGTH);
    }

    if (UncachedPtr != NULL)
    {
        STOS_UnmapPhysToUncached((U32 *)UncachedPtr, TEST_MAPPED_LENGTH);
    }

    STTBX_Print(("\n"));
    return Error;
}

/*-------------------------------------------------------------------------
 * Function : TestFlushRegion_flush
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
STOS_Error_t TestFlushRegion_flush(void)
{
    STOS_Error_t    Error = STOS_SUCCESS;
    volatile U32  * CachedPtr, * UncachedPtr;   /* volatile: ptr content will be re-read each time */
    U32             ValueCached, ValueUncached;

#if !defined(ST_OS20)
    STTBX_Print(("Starting: %s\n", __FUNCTION__));
#endif

    CachedPtr = (U32 *)STOS_MapPhysToCached((void *)TEST_MAPPED_MEM_ADDRESS, TEST_MAPPED_LENGTH);
    UncachedPtr = (U32 *)STOS_MapPhysToUncached((void *)TEST_MAPPED_MEM_ADDRESS, (U32)TEST_MAPPED_LENGTH);
    STTBX_Print(("Cached:0x%.8x, Uncached:0x%.8x\n", (U32)CachedPtr, (U32)UncachedPtr));
    if (   (CachedPtr != NULL)
        && (UncachedPtr != NULL))
    {
        *UncachedPtr = 0;
        ValueUncached = *UncachedPtr;
        ValueCached = 0;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (ValueUncached != 0)
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        *UncachedPtr = 0xA5A5A5A5;
        ValueUncached = *UncachedPtr;
        ValueCached = *CachedPtr;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (ValueUncached != 0xA5A5A5A5)
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        *CachedPtr = 0x11111111;
        ValueUncached = *UncachedPtr;
        ValueCached = *CachedPtr;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (   (ValueCached != 0x11111111)
            || (ValueCached == ValueUncached))
        {
            STTBX_Print(("Error !!!\n"));
            Error = STOS_FAILURE;
        }

        STTBX_Print(("Flushes region\n"));
        STOS_FlushRegion(CachedPtr, 4);

        ValueUncached = *UncachedPtr;
        ValueCached = *CachedPtr;
        STTBX_Print(("*Uncached=0x%.8x, *Cached=0x%.8x\n", ValueUncached, ValueCached));
        if (   (ValueUncached != 0x11111111)
            || (ValueCached != 0x11111111))
        {
            STTBX_Print(("Values should be equal to 0x11111111: Error !!!\n"));
            Error = STOS_FAILURE;
        }
    }
    else
    {
        STTBX_Print(("Error !!!\n"));
        Error = STOS_FAILURE;
    }

    if (CachedPtr != NULL)
    {
        STOS_UnmapPhysToCached((void *)CachedPtr, TEST_MAPPED_LENGTH);
    }

    if (UncachedPtr != NULL)
    {
        STOS_UnmapPhysToUncached((U32 *)UncachedPtr, TEST_MAPPED_LENGTH);
    }

    STTBX_Print(("\n"));
    return Error;
}


/* end of regions.c */
