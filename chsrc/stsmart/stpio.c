/************************************************************************
COPYRIGHT (C) STMicroelectronics 2006

Source file name : stpio.c

PIO driver.

NOTE: This driver is not fully MT-safe. Init/Term/Open/Close should be
mutually MT-safe; exclusion is achieved by means of a semaphore.
Open/Close/Set/Clear/Read/Write etc should also be mutually MT-safe;
exclusion is achieved by disabling interrupts when accessing critical data
structures. But Init/Term are NOT MT-safe w.r.t. Set/Clear/Read/Write etc,
& vice-versa.

The proper solution to this problem, which will also make the functions
a lot more efficient, is to redesign the method of allocating & using handles.
A handle should effectively be a direct reference to the port's data structure.
This would mean that the handle cache would no longer be needed, thus getting
rid of much of the access contention.

************************************************************************/

/* Includes --------------------------------------------------------------*/
#if defined(ST_OS20)||defined(ST_OS21)
#include <string.h>
#endif

#ifdef ST_OSLINUX
#include "compat.h"
#else
#include "stlite.h"
#endif

#include "stddefs.h"
#include "stpio.h"


/* Offsets (8 bit registers, byte aligned) */
#define PIO_OUT                         0
#define PIO_OUT_SET                     1
#define PIO_OUT_CLR                     2
#define PIO_IN                          4
#define PIO_C0                          8
#define PIO_C0_SET                      9
#define PIO_C0_CLR                      10
#define PIO_C1                          12
#define PIO_C1_SET                      13
#define PIO_C1_CLR                      14
#define PIO_C2                          16
#define PIO_C2_SET                      17
#define PIO_C2_CLR                      18
#define PIO_CMP                         20
#define PIO_CMP_SET                     21
#define PIO_CMP_CLR                     22
#define PIO_MASK                        24
#define PIO_MASK_SET                    25
#define PIO_MASK_CLR                    26

#ifdef ST_OSLINUX
#define STPIO_MAPPING_WIDTH           4096
#endif

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1) && !defined(ST_OSLINUX)
#pragma ST_device (stpio_Reg_t)
#endif
typedef volatile U32 stpio_Reg_t;

/************************************************
STPIO_OpenInfo_s - Structure which is contained
within STPIO_Init_s and contains current open
port information.
************************************************/
struct STPIO_OpenInfo_s
{
    STPIO_Handle_t          Handle;
    STPIO_BitMask_t         ReservedBits;
    STPIO_CompareClear_t    CompareClear;
    struct STPIO_Init_s     *InitStructure;
    void (*IntHandler)(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
};

/************************************************
STPIO_Init_t - Init structure which contains all
the open parameters and init parameters.
************************************************/

struct STPIO_Init_s
{
    ST_DeviceName_t     DeviceName;
    U32                 *BaseAddress;
#if defined(ST_OSLINUX)  && defined(LINUX_FULL_KERNEL_VERSION)
    /* Linux ioremap address */
    unsigned long       *PioMappingBaseAddress;
    unsigned long        PioMappingWidth;
#endif
#ifdef ST_OSLINUX
    U32                 InterruptNumber;
#else
#ifdef ST_OS21
    interrupt_name_t    InterruptNumber;
#else
    U32                 InterruptNumber;
#endif
#endif
    U32                 InterruptLevel;
    STPIO_BitConfig_t   BitConfigure[8];
    STPIO_BitMask_t     AllocatedBits; /* these bits are being used already */
    U32                 PIOID;
    ST_Partition_t      *Partition;
    struct STPIO_OpenInfo_s  OpenInfo[8];
    struct STPIO_Init_s      *NextSTPIO;
};

typedef struct STPIO_Init_s STPIO_Init_t;

/************************************************
STPIO_IOCache_t - IOCache which holds all
the users data for sequntial writes.
************************************************/
typedef struct
{
    STPIO_Handle_t         Handle;
    STPIO_BitMask_t        ReservedBits;
    struct STPIO_Init_s    *PIO;
} STPIO_IOCache_t;

static STPIO_IOCache_t IOCache;

/* STATIC POINTER TO THE FIRST PIO STRUCTURE */
static STPIO_Init_t *PIOHead = NULL;

/* DRIVER VERSION NUMBER */
static const ST_Revision_t Revision = "STPIO-REL_1.3.13";

/* UTILITY FUNCTIONS */
static STPIO_Init_t *GetStructFromName(const ST_DeviceName_t DeviceName);
static BOOL IsPIOInit(const ST_DeviceName_t DeviceName);
static void GetRBitsFromHandle(U32 Handle, STPIO_Init_t **PIOIndexOut,
                                STPIO_BitMask_t *ReservedBitsOut);
static ST_ErrorCode_t TermLinkedList(STPIO_Init_t **PIO);
ST_ErrorCode_t pio_init (const ST_DeviceName_t DeviceName,
                         STPIO_InitParams_t *InitParameters,
                         BOOL ResetConfig);

/* ISR */

#if defined(ST_OSLINUX) && defined(LINUX_FULL_KERNEL_VERSION)
irqreturn_t STPIO_Handler(int irq, void* dev_id, struct pt_regs* regs);
#else
#ifdef ST_OS21
static int STPIO_Handler (STPIO_Init_t *PIO);
#else
static void STPIO_Handler (STPIO_Init_t *PIO);
#endif
#endif

#define CHAR_BITS                8
#define BYTE_BIT_MASK            255
#define DISABLE_PIO_INTERRUPTS   255
#define ENABLE_PIO_INTERRUPTS    255

/* STATIC INDEX OF PIOs Initialized TO 0 */
static U32 PIOID = 0;

/* For mutual exclusion on driver data. Initialised on 1st init call. */
#ifdef ST_OSLINUX
static semaphore_t *Sema_p;
#else
#ifdef ST_OS21
static semaphore_t *Sema_p;
#else
static semaphore_t Sema;
static semaphore_t *Sema_p = NULL;
#endif
#endif

static BOOL   SemaphoreInitialized = FALSE;

/******************************************************************************
*        PUBLIC DRIVER FUNCTIONS
******************************************************************************/
/******************************************************************************
* Function Name: STPIO_Init
* Returns:       ST_ErrorCode_t
* Parameters:    ST_DeviceName_t DeviceName, STPIO_InitParams_t *InitParameters
* Use:           A wrapper for pio_init fuction which initialises the port &
*                configure every pin of the port in it's default settings.
*
******************************************************************************/

ST_ErrorCode_t STPIO_Init (const ST_DeviceName_t DeviceName,
                           STPIO_InitParams_t *InitParameters)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;

#if defined(ST_OS20) || defined(ST_OS21)
    task_lock();
#elif defined(ST_OSLINUX)
    preempt_enable();
#endif

    if (!SemaphoreInitialized)
    {
        SemaphoreInitialized = TRUE;

#if defined(ST_OS21)||defined(ST_OSLINUX)
        Sema_p = semaphore_create_fifo(1);
#else
        Sema_p = &Sema;
        semaphore_init_fifo(Sema_p, 1);
#endif
    }
#if defined(ST_OS20) || defined(ST_OS21)
    task_unlock();
#elif defined(ST_OSLINUX)
    preempt_disable();
#endif

    /* This function needs to be atomic */
    semaphore_wait (Sema_p);

    Error = pio_init( DeviceName,InitParameters,TRUE);

    /* Release the resource */
    semaphore_signal(Sema_p);

    return(Error);
}

/******************************************************************************
* Function Name: STPIO_InitNoReset
* Returns:       ST_ErrorCode_t
* Parameters:    ST_DeviceName_t DeviceName, STPIO_InitParams_t *InitParameters
* Use:           A wrapper for pio_init fuction which initialises the port but
*                does not configure pins of the port in it's default settings.
*
******************************************************************************/
ST_ErrorCode_t STPIO_InitNoReset (const ST_DeviceName_t DeviceName,
                                  STPIO_InitParams_t *InitParameters)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;

#if defined(ST_OS20) || defined(ST_OS21)
    task_lock();
#elif defined(ST_OSLINUX)
    preempt_enable();
#endif

    if (!SemaphoreInitialized)
    {
        SemaphoreInitialized = TRUE;
#if defined(ST_OS21)||defined(ST_OSLINUX)
        Sema_p = semaphore_create_fifo(1);
#else
        Sema_p = &Sema;
        semaphore_init_fifo(Sema_p, 1);
#endif
    }

#if defined(ST_OS20) || defined(ST_OS21)
    task_unlock();
#elif defined(ST_OSLINUX)
    preempt_disable();
#endif


    /* This function needs to be atomic */
    semaphore_wait(Sema_p);

    Error = pio_init( DeviceName,InitParameters,FALSE);

    /* Release the resource */
    semaphore_signal (Sema_p);

    return(Error);
}

/******************************************************************************
* Function Name: pio_init
* Returns:       ST_ErrorCode_t
* Parameters:    ST_DeviceName_t DeviceName, STPIO_InitParams_t *InitParameters
*                BOOL ResetConfig.
* Use:           To initialize the each PIO port separately, and store the PIOs
*                in a linked list. Allocates enough memory for PIO init structs
*                and for open info structs, when the PIOs are opened. Also
*                installs and enables interrupts and handler.
******************************************************************************/
ST_ErrorCode_t pio_init (const ST_DeviceName_t DeviceName,
                         STPIO_InitParams_t *InitParameters,
                         BOOL ResetConfig)
{
    STPIO_Init_t    *PIO = NULL;
    STPIO_Init_t    *PIOIndex = NULL;
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    stpio_Reg_t     *PioReg;
    U32             Index = 0;
    U32             Bit = 0x01;
    U32             BitConfig = 0;
    U8              C0=0, C1=0, C2=0;

#ifdef ST_OSLINUX
    struct resource* pio_requested_resource;
#endif

    /* First check partition has been specified, since we cannot do much
       without it. */
    if (InitParameters->DriverPartition == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check for no head pointer, if so, initialize a semaphore
       to enable IO caching and atomic operations.  */
    if (IsPIOInit(DeviceName) == FALSE)
    {
        /* create & initialise instance of PIO driver.
           Allocate memory to the size of STPIO_Init_t struct
           and return a pointer to the start address in memory. Note that each
           device name (i.e. port) allocates from the partition specified in
           the init params. */
        PIO = (STPIO_Init_t *)memory_allocate(
                          (partition_t *) InitParameters->DriverPartition,
                          sizeof(STPIO_Init_t));

        /* Check the pointer is valid before proceeding */
        if (PIO != NULL)
        {
            /* Populate the struct with the ip params */
            strncpy(PIO->DeviceName, DeviceName, ST_MAX_DEVICE_NAME);
            PIO->BaseAddress     = InitParameters->BaseAddress;
            PIO->InterruptNumber = InitParameters->InterruptNumber;
            PIO->InterruptLevel  = InitParameters->InterruptLevel;
            PIO->AllocatedBits   = 0;
            PIO->PIOID           = PIOID;
            PIO->Partition       = InitParameters->DriverPartition;
            PIO->NextSTPIO       = NULL;

#ifdef ST_OSLINUX
            /* Remap base address under Linux */
            PIO->PioMappingWidth = STPIO_MAPPING_WIDTH;
            if (check_mem_region((unsigned long)PIO->BaseAddress/*| REGION2*/,
                                                PIO->PioMappingWidth) < 0 )
            {
                printk("stpio.c=>pio_init(): checking memory busy \n");
                printk("stpio.c=>pio_init:   assumed memory has been mapped\n");
            }
            pio_requested_resource = request_mem_region((unsigned long)PIO->BaseAddress /*| REGION2*/,
                                                                       PIO->PioMappingWidth, "STPIO");

            PIO->PioMappingBaseAddress = (unsigned long *)ioremap((unsigned long)PIO->BaseAddress/*| REGION2*/, PIO->PioMappingWidth);

            if (PIO->PioMappingBaseAddress == NULL)
            {
                release_mem_region((unsigned long)PIO->BaseAddress /*| REGION2*/,
                                                  PIO->PioMappingWidth);
                return ST_ERROR_NO_MEMORY;
            }
            /* set the base address to the device pointer */
            PioReg = (stpio_Reg_t*)PIO->PioMappingBaseAddress;
            /*
             *   clear the mask reg will have the effect of disabling
             *   interrupts
             */
            STSYS_WriteRegDev8((&(PioReg[PIO_MASK_CLR])),DISABLE_PIO_INTERRUPTS);
#else
            /* set the base address to the device pointer */
            PioReg = (stpio_Reg_t*)PIO->BaseAddress;
            /*
             *   clear the mask reg will have the effect of disabling
             *   interrupts
             */
            PioReg[PIO_MASK_CLR] = DISABLE_PIO_INTERRUPTS;
#endif

            /* init all the open info struct and hardware */
            for(Index = 0; Index < 8; Index++)
            {
                PIO->OpenInfo[Index].Handle       = 0;
                PIO->OpenInfo[Index].ReservedBits = 0;
                PIO->OpenInfo[Index].IntHandler   = NULL;
                PIO->BitConfigure[Index] = STPIO_BIT_NOT_SPECIFIED;

                /* Derive config register values */
                BitConfig = (U32)PIO->BitConfigure[Index];

                if ((BitConfig & 0x01) != 0)
                {
                    C0 |= Bit;
                }
                if ((BitConfig & 0x02) != 0)
                {
                    C1 |= Bit;
                }
                if ((BitConfig & 0x04) != 0)
                {
                    C2 |= Bit;
                }

                /* now shift the bit along */
                Bit <<= 1;
            }

            if (ResetConfig)
            {
                /* Now set the config regs to the required values */
#ifdef ST_OSLINUX
                STSYS_WriteRegDev8((&(PioReg[PIO_C0_CLR])),0xFF);
                STSYS_WriteRegDev8((&(PioReg[PIO_C1_CLR])),0xFF);
                STSYS_WriteRegDev8((&(PioReg[PIO_C2_CLR])),0xFF);
                STSYS_WriteRegDev8((&(PioReg[PIO_C0_SET])),C0);
                STSYS_WriteRegDev8((&(PioReg[PIO_C1_SET])),C1);
                STSYS_WriteRegDev8((&(PioReg[PIO_C2_SET])),C2);
#else
                PioReg[PIO_C0_CLR] = 0xFF;
                PioReg[PIO_C1_CLR] = 0xFF;
                PioReg[PIO_C2_CLR] = 0xFF;
                PioReg[PIO_C0_SET] = C0;
                PioReg[PIO_C1_SET] = C1;
                PioReg[PIO_C2_SET] = C2;
#endif

            }
            /* install interrupt handler with 2 calls to STLite */
#if defined(ST_OSLINUX)&& defined(LINUX_FULL_KERNEL_VERSION)
            /*if (request_irq((unsigned int)InitParameters->InterruptNumber,
                                    STPIO_Handler,
                                    SA_INTERRUPT,
                                    DeviceName,
                                    (void *)PIO) == 0 )*/
            if(1)
            {
                printk("SUCCESSFULL\n");
                if(1)
#else

            if (interrupt_install(InitParameters->InterruptNumber,
                                  InitParameters->InterruptLevel,
                                  (void(*)(void*))STPIO_Handler,
                                  (STPIO_Init_t *) PIO) == 0/* success */)
            {
#ifdef ST_OS21

                if (interrupt_enable_number(InitParameters->InterruptNumber) == 0)
#else /* !defined ST_OS21 */
                /* interrupt_enable call will be redundant after change will be done in STBOOT*/
#ifdef STAPI_INTERRUPT_BY_NUMBER
                if((interrupt_enable(InitParameters->InterruptLevel) == 0) &&
                   (interrupt_enable_number(InitParameters->InterruptNumber)==0))
#else
                if (interrupt_enable(InitParameters->InterruptLevel) == 0)
#endif /* STAPI_INTERRUPT_BY_NUMBER */
#endif /* ST_OS21 */
#endif /* ST_OSLINUX */
                {
                    /* success */
                    if (PIOHead != NULL)
                    {

                        /* Assign the PIOHead for indexing purposes */
                        PIOIndex = PIOHead;

                        /* Iterate the linked list till Next pointer in NULL */
                        while (PIOIndex->NextSTPIO != NULL)
                        {
                            /* Point at the next defined PIO */
                            PIOIndex = PIOIndex->NextSTPIO;
                        }

                        /* now do the assignment to put the current
                        pointer in the previous struct variable */
                        PIOIndex->NextSTPIO = PIO;
                    }
                    /* Give the PIOHead pointer a value */
                    else
                    {
                        PIOHead = PIO;
                    }
                    /* increment the PIO counter to tag the PIOs with */
                    PIOID++;

                    /* success */
                }
                else
                {
                    /* error on interrupt enable */
                    Error = ST_ERROR_BAD_PARAMETER;

                    /* clean up */
                    memory_deallocate ((partition_t *) PIO->Partition, PIO);
                }
            }
            else
            {
                /* error on interrupt install */
                Error = ST_ERROR_INTERRUPT_INSTALL;

                /* clean up */
                memory_deallocate ((partition_t *) PIO->Partition, PIO);
            }
        }
        else
        {
            /* failed to allocate memory */
            Error = ST_ERROR_NO_MEMORY;
        }
    }
    else
    {
        /* PIO already inited */
        Error = ST_ERROR_ALREADY_INITIALIZED;
    }
    return Error;
}

/******************************************************************************
* Function Name: STPIO_Term
* Returns:       ST_ErrorCode_t
* Parameters:    ST_DeviceName_t DeviceName, STPIO_TermParams_t *TermParams
* Use:           Unistalls relevant PIO interrupts the handler and deallocates
*                PIOInit structures
******************************************************************************/

ST_ErrorCode_t STPIO_Term (const ST_DeviceName_t DeviceName,
                           STPIO_TermParams_t *TermParams)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_Init_t    *PIO  = NULL;
    U32             Index = 0;

    if (PIOHead == NULL)                /* has init been done ? */
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* This function needs to be atomic */
    semaphore_wait (Sema_p);

    PIO = GetStructFromName(DeviceName);
    if (PIO != NULL)
    {
        /* Check to ensure the state of the force terminate flag
        if its true, term no matter whether there are handles open */
        if(TermParams->ForceTerminate != TRUE)
        {
            /* Check to ensure there are no open handles on the PIO */
            for (Index = 0; Index < 8; Index++)
            {
                /* Make sure all the handles are == 0 or pass back an error */
                if(PIO->OpenInfo[Index].Handle != 0)
                {
                    Error = ST_ERROR_OPEN_HANDLE;
                }
            }
            if (Error == ST_NO_ERROR)
            {
                /* Now terminate the linked list and
                kill off the registered interrupts */
                Error = TermLinkedList(&PIO);
            }
        }
        else
        {
            /* Don't bother checking for open handles, just terminate
            the linked list and kill off the registered interrupts
            otherwise they will keep coming back to haunt you =:o */
            Error = TermLinkedList(&PIO);
        }
    }
    else
    {
        /* PIO not inited */
        Error = ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Release the semaphore - cannot delete the semaphore! */
    semaphore_signal (Sema_p);

    return Error;
}

/******************************************************************************
* Function Name: STPIO_Open
* Returns:       ST_ErrorCode_t
* Parameters:    ST_DeviceName_t DeviceName, STPIO_OpenParams_t *OpenParams,
*                STPIO_Handle_t *Handle
* Use:           Opens an initialized PIO and configures its port bits.
*                Also creates a handle to pass back and installs an IRQ handler
******************************************************************************/
ST_ErrorCode_t STPIO_Open(const ST_DeviceName_t DeviceName, \
                          STPIO_OpenParams_t *OpenParams, \
                          STPIO_Handle_t *Handle)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_Handle_t  OpenHandle = 0;
    STPIO_Init_t    *PIOIndex = NULL;
    U32             Index = 0;
    U32             Bit = 0x01;
    stpio_Reg_t     *PioReg;
    U32             BitConfig = 0;
    U8              C0=0, C1=0, C2=0;


    if ( PIOHead == NULL )
    {
        return ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Note - assumes openparams is valid. :/ */
    if (OpenParams->ReservedBits == 0)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* This function needs to be atomic */
    semaphore_wait (Sema_p);

    /* ensure the PIO has been initialized first and get
    the base address from the device name  */
    PIOIndex = GetStructFromName (DeviceName);
    if (PIOIndex != NULL)
    {
        /* check the required bits are not in use */
        if((PIOIndex->AllocatedBits & OpenParams->ReservedBits) == 0)
        {
            /* Clear the handle (see loop below) */
            OpenHandle = 0;

            /*
             * For each requested bit:
             *   - store  the handle
             *   - store reserved bits
             *   - store the IRQ handler
             *   - store and config each bit
             *   - set the hardware config bits
             */

            /* Update the AllocateBits */
            PIOIndex->AllocatedBits |= OpenParams->ReservedBits;

            /* set the base address to the device pointer */
#ifdef ST_OSLINUX
            PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
#else
            PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
#endif

            for (Index = 0; Index < 8; Index++, Bit <<= 1)
            {
                if (OpenParams->ReservedBits & Bit)
                {
                    if (OpenHandle == 0)
                    {
                        OpenHandle = (U32)&PIOIndex->OpenInfo[Index];
                    }

                    PIOIndex->OpenInfo[Index].Handle       = OpenHandle;
                    /* Default to auto clear */
                    PIOIndex->OpenInfo[Index].CompareClear = \
                        STPIO_COMPARE_CLEAR_AUTO;
                    PIOIndex->OpenInfo[Index].ReservedBits = \
                        OpenParams->ReservedBits;
                    PIOIndex->OpenInfo[Index].IntHandler   = \
                        OpenParams->IntHandler;
                    PIOIndex->OpenInfo[Index].InitStructure = PIOIndex;

                    /* store the bit configure */
                    PIOIndex->BitConfigure[Index] = \
                        OpenParams->BitConfigure[Index];

                    /*
                     *  Check that the config bit has been specified
                     *  else do nothing
                     */

                    if (PIOIndex->BitConfigure[Index] != \
                        STPIO_BIT_NOT_SPECIFIED)
                    {
                        /* Build up values to write to h/w */
                        BitConfig = (U32)PIOIndex->BitConfigure[Index];

                        if ((BitConfig & 0x01) != 0)
                        {
                            C0 |= Bit;
                        }
                        if ((BitConfig & 0x02) != 0)
                        {
                            C1 |= Bit;
                        }
                        if ((BitConfig & 0x04) != 0)
                        {
                            C2 |= Bit;
                        }
                    }

                    /* Set initial line state while opening */
                    if((BitConfig == (U32)STPIO_BIT_BIDIRECTIONAL_HIGH) ||
                       (BitConfig == (U32)STPIO_BIT_OUTPUT_HIGH))
                    {
#ifdef ST_OSLINUX
                        STSYS_WriteRegDev8((&(PioReg[PIO_OUT_SET])),Bit);
#else
                        PioReg[PIO_OUT_SET] = Bit;
#endif
                    }
                }
            }  /* end of for loop */

            /* Now set the config regs to the required values */
#ifdef ST_OSLINUX
            STSYS_WriteRegDev8((&(PioReg[PIO_C0_CLR])) , OpenParams->ReservedBits);
            STSYS_WriteRegDev8((&(PioReg[PIO_C1_CLR])) , OpenParams->ReservedBits);
            STSYS_WriteRegDev8((&(PioReg[PIO_C2_CLR])) , OpenParams->ReservedBits);
            STSYS_WriteRegDev8((&(PioReg[PIO_C0_SET])) , C0);
            STSYS_WriteRegDev8((&(PioReg[PIO_C1_SET])) , C1);
            STSYS_WriteRegDev8((&(PioReg[PIO_C2_SET])) , C2);
#else
            PioReg[PIO_C0_CLR] = OpenParams->ReservedBits;
            PioReg[PIO_C1_CLR] = OpenParams->ReservedBits;
            PioReg[PIO_C2_CLR] = OpenParams->ReservedBits;
            PioReg[PIO_C0_SET] = C0;
            PioReg[PIO_C1_SET] = C1;
            PioReg[PIO_C2_SET] = C2;
#endif

            /* Pass back an open handle and any error */
            *Handle = OpenHandle;

            /* success */
        }
        else
        {
            *Handle = 0;
            Error = ST_ERROR_DEVICE_BUSY;
        }
    }
    else
    {
        *Handle = 0;
        Error = ST_ERROR_UNKNOWN_DEVICE;
    }

    /* Release the resource */
    semaphore_signal (Sema_p);

    return Error;
}

/******************************************************************************
* Function Name: STPIO_Close
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle
* Use:           Closes a PIO handle if it is already open. Resets the PIO
*                configuration and destroys the handle along with resetting
*                the open info structure.
******************************************************************************/
ST_ErrorCode_t STPIO_Close (STPIO_Handle_t Handle)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_Init_t    *PIOIndex = NULL;
    STPIO_BitMask_t ReservedBits;
    U32             Index = 0;
    U32             Bit = 0x01;
    unsigned int    flags;

    if (PIOHead == NULL)                /* has init been done ? */
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    /* This function needs to be atomic */
    semaphore_wait (Sema_p);

    /* Get active bits and PIOIndex pointer from the handle */
    GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    if (ReservedBits != 0)
    {
        stpio_Reg_t *PioReg;

        /* Disable compare mask */
#if defined (ST_OSLINUX)
        spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        U32 oldPriority;
        oldPriority = interrupt_mask_all();
#else
        interrupt_lock();
#endif
#endif

#ifdef ST_OSLINUX
        PioReg = (stpio_Reg_t *)PIOIndex->PioMappingBaseAddress;
#else
        PioReg = (stpio_Reg_t *)PIOIndex->BaseAddress;
#endif

#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_MASK_CLR])), ReservedBits);
#else
        PioReg[PIO_MASK_CLR] = ReservedBits;
#endif


#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        /*
         *  For each requested bit:
         *    - erase and unconfig each bit
         *    - erase  the handle
         *    - erase reserved bits
         */

        /* Update the AllocateBits to remove the reserved bits */
        PIOIndex->AllocatedBits &= (~ReservedBits);

        for (Index = 0; Index < 8; Index++)
        {
            if (ReservedBits & Bit)
            {
                PIOIndex->OpenInfo[Index].Handle       = 0;
                PIOIndex->OpenInfo[Index].ReservedBits = 0;
                PIOIndex->OpenInfo[Index].IntHandler   = NULL;
            }
            Bit <<= 1;
        }

        /* Also update IOCACHE structure -- DDTS 28536 */
        IOCache.PIO          = NULL;
        IOCache.ReservedBits = 0;
        IOCache.Handle       = 0;
        /* success */
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    /* Release the resource */
    semaphore_signal (Sema_p);

    return Error;
}

/******************************************************************************
* Function Name:  STPIO_Read
* Returns:     ST_ErrorCode_t
* Parameters:  STPIO_Handle_t Handle, U8 *Buffer
* Use:         Simply read the PIO_OUT register and pass the contents back in
*              a buffer.
******************************************************************************/
ST_ErrorCode_t STPIO_Read(STPIO_Handle_t Handle, U8 *Buffer)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_BitMask_t ReservedBits;
    STPIO_Init_t    *PIOIndex = NULL;
    stpio_Reg_t     *PioReg;
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined (ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif

    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
#ifdef ST_OSLINUX
        /* set the base address to the device pointer */
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
        /* get the bits */
        *Buffer = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_IN]))) & ReservedBits;
#else
        /* set the base address to the device pointer */
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
        /* get the bits */
        *Buffer = PioReg[PIO_IN] & ReservedBits;
#endif
        /* success */
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/******************************************************************************
* Function Name: STPIO_Write
* Returns:     ST_ErrorCode_t
* Parameters:  STPIO_Handle_t Handle, U8 Buffer
* Use:         Simply takes a U8 buffer and writes it to the PIO_OUT register
******************************************************************************/
ST_ErrorCode_t STPIO_Write (STPIO_Handle_t Handle, U8 Buffer)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_BitMask_t ReservedBits;
    STPIO_Init_t   *PIOIndex = NULL;
    stpio_Reg_t    *PioReg;
    U32             Set;
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined (ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
        /* set the base address to the device pointer */
#ifdef ST_OSLINUX
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
#else
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
#endif

        /* set the high bits */
        Set = Buffer & ReservedBits;

#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_OUT_SET])), Set);
#else
        PioReg[PIO_OUT_SET] = Set;
#endif

        /* now set the low bits */
        Set = (Set ^ 0xFF) & ReservedBits;
#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_OUT_CLR])) , Set);
#else
        PioReg[PIO_OUT_CLR] = Set;
#endif
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/******************************************************************************
* Function Name:   STPIO_Set
* Returns:         ST_ErrorCode_t
* Parameters:      STPIO_Handle_t Handle, STPIO_BitMask_t BitMask
* Use:             Takes a BitMask and writes it to the PIO_OUT_SET register to
*                  write to the PIO_OUT register.
******************************************************************************/
ST_ErrorCode_t STPIO_Set (STPIO_Handle_t Handle, STPIO_BitMask_t BitMask)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_BitMask_t ReservedBits;
    STPIO_Init_t   *PIOIndex = NULL;
    stpio_Reg_t    *PioReg;
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined (ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif

    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {

#ifdef ST_OSLINUX
        /* set the base address to the device pointer */
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
        /* write the reg */
        STSYS_WriteRegDev8((&(PioReg[PIO_OUT_SET])),BitMask & ReservedBits);
#else
        /* set the base address to the device pointer */
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
        /* write the reg */
        PioReg[PIO_OUT_SET] = BitMask & ReservedBits;
#endif
        /* success */
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/******************************************************************************
* Function Name: STPIO_Clear
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, STPIO_BitMask_t BitMask
* Use:           Takes a BitMask and writes it to the PIO_OUT_CLR register to
*                clear the PIO_OUT register.
******************************************************************************/
ST_ErrorCode_t STPIO_Clear(STPIO_Handle_t Handle, STPIO_BitMask_t BitMask)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_BitMask_t ReservedBits;
    STPIO_Init_t    *PIOIndex = NULL;
    stpio_Reg_t     *PioReg;
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined (ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
#ifdef ST_OSLINUX
        /* set the base address to the device pointer */
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
        /* write the reg */
        STSYS_WriteRegDev8((&(PioReg[PIO_OUT_CLR])), BitMask & ReservedBits);
#else
        /* set the base address to the device pointer */
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
        /* write the reg */
        PioReg[PIO_OUT_CLR] = BitMask & ReservedBits;
#endif
        /* success */
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}

/******************************************************************************
* Function Name: STPIO_GetCompare
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, ST_Compare_t *CompareStatus
* Use:           Gets the compare and enable pattern of the PIO_MASK and
*                PIO_CMP registers.
******************************************************************************/
ST_ErrorCode_t STPIO_GetCompare (STPIO_Handle_t Handle,
                                 STPIO_Compare_t *CompareStatus)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_BitMask_t ReservedBits;
    STPIO_Init_t    *PIOIndex = NULL;
    stpio_Reg_t     *PioReg;
    unsigned int flags;
    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined(ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
        /* set the base address to the device pointer */
#ifdef ST_OSNLINUX
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
#else
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
#endif

        /* disable the interrupts by name */
#ifdef ST_OS21
        interrupt_disable_number(PIOIndex->InterruptNumber);
#else
        /* disable the interrupts for the particular level */
        /*interrupt_disable will become redundant after change in STBOOT*/
#ifdef  STAPI_INTERRUPT_BY_NUMBER
        interrupt_disable(PIOIndex->InterruptLevel);
        interrupt_disable_number(PIOIndex->InterruptNumber);
#else
#ifndef ST_OSLINUX
        interrupt_disable(PIOIndex->InterruptLevel);
#endif
#endif  /*DDTS 21751--Added support for Interrupt by number*/
#endif  /* ST_OS21 */

        /* get the regs */
#ifdef ST_OSLINUX
        CompareStatus->CompareEnable  = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_MASK]))) & ReservedBits;
        CompareStatus->ComparePattern = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_CMP])))  & ReservedBits;
#else
        CompareStatus->CompareEnable  = PioReg[PIO_MASK] & ReservedBits;
        CompareStatus->ComparePattern = PioReg[PIO_CMP]  & ReservedBits;
#endif

#ifndef ST_OSLINUX
        /* now reenable the interrupts */
#ifdef ST_OS21
        interrupt_enable_number(PIOIndex->InterruptNumber);
#else
        /*Interrupt_enable call will be redundant after change will be done in STBOOT*/
#ifdef STAPI_INTERRUPT_BY_NUMBER
        interrupt_enable(PIOIndex->InterruptLevel);
        interrupt_enable_number(PIOIndex->InterruptNumber);
#else
#ifndef ST_OSLINUX
        interrupt_enable(PIOIndex->InterruptLevel);
#endif
#endif /* STAPI_INTERRUPT_BY_NUMBER */
#endif /* ST_OS21 */
#endif

    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

/******************************************************************************
* Function Name: STPIO_GetCompareClear
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, STPIO_CompareClear_t *CompareClear_p
* Use:           Gets the current CompareClear behaviour of the driver for
                 the given handle.
******************************************************************************/
ST_ErrorCode_t STPIO_GetCompareClear (STPIO_Handle_t Handle,
                                      STPIO_CompareClear_t *CompareClear_p)
{
    ST_ErrorCode_t  error = ST_NO_ERROR;
    STPIO_Init_t    *PIOIndex = NULL;
    STPIO_BitMask_t ReservedBits;
    U32             Bit = 0x01, Index;
    unsigned int flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined(ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Don't actually want the bits, this is just the easiest way to
         * retrieve the structure.
         */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
        Bit = 0x01;

        for (Index = 0; Index < 8; Index++, Bit <<= 1)
        {
            if (PIOIndex->OpenInfo[Index].Handle == Handle)
            {
                *CompareClear_p = PIOIndex->OpenInfo[Index].CompareClear;
            }
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/******************************************************************************
* Function Name: STPIO_SetCompare
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, ST_Compare_t *CompareStatus
* Use:           Sets the compare and enable pattern of the PIO_MASK and
*                PIO_CMP registers.
******************************************************************************/
ST_ErrorCode_t STPIO_SetCompare(STPIO_Handle_t Handle, \
                                STPIO_Compare_t *CompareStatus)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_BitMask_t ReservedBits;
    STPIO_Init_t    *PIOIndex = NULL;
    stpio_Reg_t     *PioReg;
    U32             Set;
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined(ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX) && defined(LINUX_FULL_KERNEL_VERSION)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX) && defined(LINUX_FULL_KERNEL_VERSION)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {

#ifdef ST_OS21
        interrupt_disable_number(PIOIndex->InterruptNumber);
#else
        /* disable the interrupts for the particular level */
#ifdef STAPI_INTERRUPT_BY_NUMBER
        interrupt_disable(PIOIndex->InterruptLevel);
        interrupt_disable_number(PIOIndex->InterruptNumber);
#else
#ifndef ST_OSLINUX
        interrupt_disable(PIOIndex->InterruptLevel);
#endif
#endif  /*DDTS 21751*/
#endif  /* ST_OS21 */

        /* set the base address to the device pointer */

#ifdef ST_OSLINUX
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
#else
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
#endif

        /* set for the cmp reg */
        /* set the high bits */
        Set = CompareStatus->ComparePattern & ReservedBits;

#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_CMP_SET])), Set);
#else
        PioReg[PIO_CMP_SET] = Set;
#endif

        /* now set the low bits */
        Set = (Set ^ 0xFF) & ReservedBits;
#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_CMP_CLR])), Set);
#else
        PioReg[PIO_CMP_CLR] = Set;
#endif

        Set = CompareStatus->CompareEnable & ReservedBits;
#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_MASK_SET])), Set);
#else
        PioReg[PIO_MASK_SET] = Set;
#endif

        /* now set the low bits */
        Set = (Set ^ 0xFF) & ReservedBits;
#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_MASK_CLR])), Set);
#else
        PioReg[PIO_MASK_CLR] = Set;
#endif

#ifndef ST_OSLINUX
        /* now reenable the interrupts */
#ifdef ST_OS21
        interrupt_enable_number(PIOIndex->InterruptNumber);
#else
        /*interrupt_enable call will be redundant after change will be done in STBOOT*/
#ifdef STAPI_INTERRUPT_BY_NUMBER
        interrupt_enable(PIOIndex->InterruptLevel);
        interrupt_enable_number(PIOIndex->InterruptNumber);
#else
#ifndef ST_OSLINUX
        interrupt_enable(PIOIndex->InterruptLevel);
#endif
#endif /*DDTS 21751*/
#endif /* ST_OS21 */
#endif
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}


/******************************************************************************
* Function Name: STPIO_SetCompareClear
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, STPIO_CompareClear_t CompareClear
* Use:           Sets the current CompareClear behaviour of the driver for
                 the given handle.
******************************************************************************/
ST_ErrorCode_t STPIO_SetCompareClear(STPIO_Handle_t Handle,
                                     STPIO_CompareClear_t CompareClear)
{
    ST_ErrorCode_t  error = ST_NO_ERROR;
    STPIO_Init_t    *PIOIndex = NULL;
    STPIO_BitMask_t ReservedBits;
    U32             Bit = 0x01, Index;
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined(ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Don't actually want the bits, this is just the easiest way to
         * retrieve the structure.
         */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
        Bit = 0x01;

        for (Index = 0; Index < 8; Index++, Bit <<= 1)
        {
            if (PIOIndex->OpenInfo[Index].Handle == Handle)
            {
                PIOIndex->OpenInfo[Index].CompareClear = CompareClear;
            }
        }
    }
    else
    {
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
}

/******************************************************************************
* Function Name: STPIO_SetConfig
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, const STPIO_BitConfig_t BitConfigure[8]
* Use:           Takes a handle and to an open PIO port and a set of
*                configuration bits and reconfigures the PIO port to the users
*                preference.
******************************************************************************/
ST_ErrorCode_t STPIO_SetConfig (STPIO_Handle_t Handle,
                                const STPIO_BitConfig_t BitConfigure[8])
{
    stpio_Reg_t     *PioReg;
    U32             Index = 0;
    U32             Bit = 0x01;
    U32             BitConfig = 0;
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPIO_Init_t    *PIOIndex = NULL;
    STPIO_BitMask_t ReservedBits;
    U8              C0=0, C1=0, C2=0;
    unsigned int flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined(ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;

#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (ReservedBits != 0)
    {
        /* set the base address to the device pointer */
#ifdef ST_OSLINUX
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
#else
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
#endif

        for (Index = 0; Index < 8; Index++, Bit <<= 1)
        {
            if (ReservedBits & Bit)
            {
                /* store the bit configure */
                PIOIndex->BitConfigure[Index] = BitConfigure[Index];

                /*
                 *  Check that the config bit has been
                 *  specified else do nothing
                 */

                if (PIOIndex->BitConfigure[Index] != STPIO_BIT_NOT_SPECIFIED)
                {
                    /* Derive config register values */
                    BitConfig = (U32)PIOIndex->BitConfigure[Index];
                    if ((BitConfig & 0x01) != 0)
                    {
                        C0 |= Bit;
                    }
                    if ((BitConfig & 0x02) != 0)
                    {
                        C1 |= Bit;
                    }
                    if ((BitConfig & 0x04) != 0)
                    {
                        C2 |= Bit;
                    }
                }
            }
        }

        /* Now set the config regs to the required values */
#if defined(ST_OSLINUX)
        spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        oldPriority = interrupt_mask_all();
#else
        interrupt_lock();
#endif
#endif

#ifdef ST_OSLINUX
        STSYS_WriteRegDev8((&(PioReg[PIO_C0_CLR])), (ReservedBits^C0) & ReservedBits);
        STSYS_WriteRegDev8((&(PioReg[PIO_C1_CLR])), (ReservedBits^C1) & ReservedBits);
        STSYS_WriteRegDev8((&(PioReg[PIO_C2_CLR])), (ReservedBits^C2) & ReservedBits);
        STSYS_WriteRegDev8((&(PioReg[PIO_C0_SET])), C0);
        STSYS_WriteRegDev8((&(PioReg[PIO_C1_SET])), C1);
        STSYS_WriteRegDev8((&(PioReg[PIO_C2_SET])), C2);
#else
        PioReg[PIO_C0_CLR] = (ReservedBits^C0) & ReservedBits;
        PioReg[PIO_C1_CLR] = (ReservedBits^C1) & ReservedBits;
        PioReg[PIO_C2_CLR] = (ReservedBits^C2) & ReservedBits;
        PioReg[PIO_C0_SET] = C0;
        PioReg[PIO_C1_SET] = C1;
        PioReg[PIO_C2_SET] = C2;
#endif

#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

/******************************************************************************
* Function Name: STPIO_GetStatus
* Returns:       ST_ErrorCode_t
* Parameters:    STPIO_Handle_t Handle, ST_Status_t *Status
* Use:           Takes a handle and returns the status of the PIO port bits and
*                how each one is configured.
******************************************************************************/
ST_ErrorCode_t STPIO_GetStatus (STPIO_Handle_t Handle, STPIO_Status_t *Status)
{
    stpio_Reg_t     *PioReg;
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    U32             Index = 0;
    U32             Bit = 0x01;
    U8              C0 = 0;
    U8              C1 = 0;
    U8              C2 = 0;
    U8              Conf;
    STPIO_Init_t   *PIOIndex = NULL;
    STPIO_BitMask_t ReservedBits; /* do not need in this routine */
    unsigned int    flags;

    /* Check the IO Cache for the handle. Use of the cache must be atomic. */
#if defined(ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
    U32 oldPriority;
    oldPriority = interrupt_mask_all();
#else
    interrupt_lock();
#endif
#endif
    if (IOCache.Handle == Handle)
    {
        PIOIndex = IOCache.PIO;
        ReservedBits = IOCache.ReservedBits;
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
    else
    {
        /* Get active bits and PIOIndex pointer from the handle */
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
        GetRBitsFromHandle (Handle, &PIOIndex, &ReservedBits);
    }

    if (PIOIndex != NULL)
    {
        /* set the base address to the device pointer */
#ifdef ST_OSLINUX
        PioReg = (stpio_Reg_t*)PIOIndex->PioMappingBaseAddress;
#else
        PioReg = (stpio_Reg_t*)PIOIndex->BaseAddress;
#endif

        /* pass back the allocated pins */
        Status->BitMask = PIOIndex->AllocatedBits;

        /* Get the values of the C0,C1,C2 regs */
#if defined(ST_OSLINUX)
        spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        oldPriority = interrupt_mask_all();
#else
        interrupt_lock();
#endif
#endif

#ifdef ST_OSLINUX
        C0 = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_C0])));
        C1 = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_C1])));
        C2 = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_C2])));
#else
        C0 = PioReg[PIO_C0];
        C1 = PioReg[PIO_C1];
        C2 = PioReg[PIO_C2];
#endif

#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif

        /* pass back the bit config for each allocated pin */
        for (Index = 0; Index < 8; Index++)
        {
            Conf = 0;
            if ((C0 & Bit) != 0)
            {
                Conf = 0x01;
            }

            if ((C1 & Bit) != 0)
            {
                Conf |= 0x02;
            }

            if ((C2 & Bit) != 0)
            {
                Conf |= 0x04;
            }

            /* Now pass back the number */
            Status->BitConfigure[Index] = (STPIO_BitConfig_t)Conf;

            Bit <<= 1;
        }
    }
    else
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return Error;
}

/******************************************************************************
* Function Name:  STPIO_GetRevision
* Returns:        ST_Revision_t
* Parameters:     None
* Use:            Simply returns the current driver revision.
******************************************************************************/
ST_Revision_t STPIO_GetRevision(void)
{
    return Revision;
}

/******************************************************************************
* Function Name:  STPIO_Handler
* Returns:        Void
* Parameters:     STPIO_Init_t *PIO
* Use:            The interrupt handler for the PIO driver.
******************************************************************************/
#if defined (ST_OSLINUX)
irqreturn_t STPIO_Handler(int irq, void* dev_id, struct pt_regs* regs)
#else
#ifdef ST_OS21
static int STPIO_Handler (STPIO_Init_t  *PIO)
#else
static void STPIO_Handler (STPIO_Init_t  *PIO)
#endif
#endif
{
    U32             InputPins;
    U32             InputMask;
    U32             InputCompare;
    STPIO_BitMask_t Active;
    stpio_Reg_t     *PioReg;
#ifdef ST_OSLINUX
    STPIO_Init_t    *PIO=NULL;
#endif
    U32             Bit = 0x01;
    U32             Index = 0;

    /* set the base address to the device pointer */
#ifdef ST_OSNLINUX
    PIO = (STPIO_Init_t *)dev_id;
#endif

#ifdef ST_OSLINUX
    PioReg = (stpio_Reg_t*)PIO->PioMappingBaseAddress;
#else
    PioReg = (stpio_Reg_t*)PIO->BaseAddress;
#endif

    /* read current status of input pins */
#ifdef ST_OSLINUX
    InputPins =STSYS_ReadRegDev8((void*)(&(PioReg[PIO_IN])));
#else
    InputPins = PioReg[PIO_IN];
#endif

    /* which pins are interrupt enabled */
#ifdef ST_OSLINUX
    InputMask = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_MASK])));
#else
    InputMask = PioReg[PIO_MASK];
#endif

    /* account for level */
#ifdef ST_OSLINUX
    InputCompare = STSYS_ReadRegDev8((void*)(&(PioReg[PIO_CMP])));
#else
    InputCompare = PioReg[PIO_CMP];
#endif

    /* get valid interrupts */
    Active = (InputPins ^ InputCompare) & InputMask;

    /* iterate through the open info struct to work out which func to call */
    for (Index = 0; Index < 8 ; Index++)
    {
        if ((Active & Bit) != 0)
        {
            /* If auto-clearing is desired */
            if (PIO->OpenInfo[Index].CompareClear == STPIO_COMPARE_CLEAR_AUTO)
            {
                /* Clear the condition */
                if (Bit & InputPins)
                {
#ifdef ST_OSLINUX
                    STSYS_WriteRegDev8((&(PioReg[PIO_CMP_SET])), Bit);
#else
                    PioReg[PIO_CMP_SET] = Bit;
#endif
                }
                else
                {
#ifdef ST_OSLINUX
                    STSYS_WriteRegDev8((&(PioReg[PIO_CMP_CLR])), Bit);
#else
                    PioReg[PIO_CMP_CLR] = Bit;
#endif
                }
            }

            if(PIO->OpenInfo[Index].IntHandler != NULL)
            {
                (*PIO->OpenInfo[Index].IntHandler) \
                    (PIO->OpenInfo[Index].Handle, Active);
            }
        }
        Bit <<= 1;
    }
#ifdef ST_OS21
    return(OS21_SUCCESS);
#elif defined ST_OSLINUX
    return 0;
#else
    return;
#endif
}

/******************************************************************************
* Function Name:  STPIO_GetBaseAddress
* Returns:        ST_ErrorCode_t
* Parameters:     ST_DeviceName_t DeviceName, U32 **BaseAddress
* Use:            Determines physical base address of the PIO port DeviceName.
*                 Address is returned in BaseAddress.
*
* NOTE: This routine is not part of the formal API. It is provided as a
* backdoor way of gaining accesss to PIO ports.
******************************************************************************/
ST_ErrorCode_t STPIO_GetBaseAddress (ST_DeviceName_t DeviceName,
                                     U32             **BaseAddress)
{
    ST_ErrorCode_t Error    = ST_NO_ERROR;
    STPIO_Init_t   *PIOIndex = GetStructFromName (DeviceName);

    if (PIOIndex == NULL)
    {
        Error = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        *BaseAddress = PIOIndex->BaseAddress;
    }
    return Error;
}

/******************************************************************************
*        PRIVATE UTILITY FUNCTIONS
******************************************************************************/

/******************************************************************************
* Function Name: IsPIOInit
* Returns:       BOOL
* Parameters:    ST_DeviceName_t DeviceName
* Use:           Utility function which checks if a PIO is already initialized,
*                given the DeviceName. Returns false if it is not initialized
*                and true if it is.
******************************************************************************/
static BOOL IsPIOInit(const ST_DeviceName_t DeviceName)
{
    BOOL         RetVal = FALSE;
    STPIO_Init_t *PIOIndex = NULL;

    if (PIOHead != NULL)
    {
        /* Assign the PIO pointer to an index pointer */
        PIOIndex = PIOHead;

        /* do the check for device name in each struct
           while the next pointer isn't NULL */
        do
        {
            if(strcmp(PIOIndex->DeviceName, DeviceName) != 0)
            {
                /* Point at the next defined PIO */
                PIOIndex = PIOIndex->NextSTPIO;
            }
            else
            {
                /* return PIO is already Inited */
                RetVal = TRUE;

                /* Make PIOIndex = NULL to get out of the loop */
                PIOIndex = NULL;
            }
        }
        while (PIOIndex != NULL);
    }

    /* return PIO not inited as a PIOHead pointer does not exist */

    return RetVal;
}

/******************************************************************************
* Function Name: GetRBitsFromHandle
* Returns:
* Parameters:    STPIO_Handle_t Handle, STPIO_Init_t *PIOIndexOut,
*                STPIO_BitMask_t *ReservedBitsOut
* Use:           Utility function which takes an open handle and passes back a
*                pointer to its PIOInit structure and which PIO bits are
*                reserved.
******************************************************************************/
static void GetRBitsFromHandle(STPIO_Handle_t  Handle,
                               STPIO_Init_t    **PIOIndexOut, \
                               STPIO_BitMask_t *ReservedBitsOut)
{
    struct STPIO_OpenInfo_s *OpenStruct;
    unsigned int     flags ;

    *PIOIndexOut = NULL;
    *ReservedBitsOut = 0;

    if (Handle)
    {
        /* Required? */
#if defined(ST_OSLINUX)
        spin_lock_irqsave(&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        U32 oldPriority;
        oldPriority = interrupt_mask_all();
#else
        interrupt_lock();
#endif
#endif
        if (PIOHead != NULL)
        {
            OpenStruct = (struct STPIO_OpenInfo_s *)Handle;

            /* Basic integrity check */
            if (OpenStruct->Handle == Handle)
            {
                *PIOIndexOut = OpenStruct->InitStructure;
                *ReservedBitsOut = OpenStruct->ReservedBits;

                /* Pass back values to the IO cache */
                IOCache.PIO = *PIOIndexOut;
                IOCache.ReservedBits = OpenStruct->ReservedBits;
                IOCache.Handle = Handle;
            }
        }
#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();
#endif
#endif
    }
}

/******************************************************************************
* Function Name: GetStructFromName
* Returns:       STPIO_Init_t *
* Parameters:    ST_DeviceName_t DeviceName
* Use:           Utility function which takes a device name and passes back a
*                pointer to its PIOInit structure.
******************************************************************************/
static STPIO_Init_t *GetStructFromName(const ST_DeviceName_t DeviceName)
{
    STPIO_Init_t *PIOIndex = NULL;
    STPIO_Init_t *PIOStructAddress = NULL;

    if (PIOHead != NULL)
    {
        /* Assign the PIO pointer to an index pointer */
        PIOIndex = PIOHead;

        /*
         *  Do the check for device name in each struct
         *  while the next pointer isn't NULL
         */
        do
        {
            if(strcmp(PIOIndex->DeviceName, DeviceName) != 0)
            {
                /* Point at the next defined PIO */
                PIOIndex = PIOIndex->NextSTPIO;
            }
            else
            {
                /* Pass back the struct address */
                PIOStructAddress = PIOIndex;

                /* Make PIOIndex = NULL to get out of the loop */
                PIOIndex = NULL;
            }
        }
        while (PIOIndex != NULL);
    }

    return PIOStructAddress;
}

/******************************************************************************
* Function Name: TermLinkedList
* Returns:       void
* Parameters:    STPIO_Init_t *PIO
* Use:           Utility function which takes a pointer to an STPIO_Init_t
*                struct and takes the struct out of the linked list and
*                deallocates the memory assigned to it.
******************************************************************************/
static ST_ErrorCode_t TermLinkedList(STPIO_Init_t **PIO)
{
    stpio_Reg_t         *PioReg;
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    STPIO_Init_t        *PIOIndex;

#if defined(ST_OSLINUX)
    unsigned int flags;
#endif

#ifdef ST_OS21
    U32 oldPriority;
#elif defined ST_OS20
    interrupt_status_t  IntStatus;
#else
    /* OSLINUX stuff here?*/
#endif

    /* To delete the interrupts the code must be
    executed in this order to bin the interrupts properly */

    /* set the base address to the device pointer */
#ifdef ST_OSLINUX
    PioReg = (stpio_Reg_t*)(*PIO)->PioMappingBaseAddress;
#else
    PioReg = (stpio_Reg_t*)(*PIO)->BaseAddress;
#endif

    /* clear the mask reg to disable interrupts first*/
#ifdef ST_OSLINUX
    STSYS_WriteRegDev8((&(PioReg[PIO_MASK_CLR])), DISABLE_PIO_INTERRUPTS);
#else
    PioReg[PIO_MASK_CLR] = DISABLE_PIO_INTERRUPTS;
#endif

    /*
     * Uninstall the interrupt handler for this port. Only disable interrupt
     * if no other handlers connected on the same level. Note this needs to
     * be MT-safe.
     */
#ifdef ST_OS21
    oldPriority = interrupt_mask_all();
#else

#if defined (ST_OSLINUX)
    spin_lock_irqsave(&current->sighand->siglock, flags);
#else
    interrupt_lock();
#endif

#endif

#ifdef ST_OS21
    /* Disable interrupt with this name */
    interrupt_disable_number( (*PIO)->InterruptNumber );
#endif

#if defined (ST_OSLINUX) && defined (LINUX_FULL_KERNEL_VERSION)
    /*free_irq((*PIO)->InterruptNumber,(void *)&PIO );*/
#else

    if (interrupt_uninstall((*PIO)->InterruptNumber,
                            (*PIO)->InterruptLevel) != 0)
    {
        Error = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif

#if defined(ST_OSLINUX)  && defined(LINUX_FULL_KERNEL_VERSION)
    /* disable_irq((*PIO)->InitParams.InterruptNumber); */
#else

#ifndef ST_OS21
    /* Obtain number of active handlers still installed */
    if (interrupt_status ((*PIO)->InterruptLevel, &IntStatus) == 0)
    {
        /* Disable interrupts at this level if no handlers are installed */
        if ( IntStatus.interrupt_numbers == 0 )
        {
            /* Disable interrupts at this level */
#ifdef STAPI_INTERRUPT_BY_NUMBER
            if((interrupt_disable( (*PIO)->InterruptLevel ) != 0) &&
               (interrupt_disable_number((*PIO)->InterruptNumber ) != 0))
#else
            if (interrupt_disable( (*PIO)->InterruptLevel ) != 0)
#endif  /*DDTS 21751*/
            {
                Error = ST_ERROR_INTERRUPT_UNINSTALL;
            }

        }
    }
    else
    {
        Error = ST_ERROR_INTERRUPT_UNINSTALL;
    }
#endif /* if not defined ST_OS21 */
#endif

#if defined(ST_OSLINUX)
        spin_unlock_irqrestore (&current->sighand->siglock, flags);
#else
#ifdef ST_OS21
        interrupt_unmask(oldPriority);
#else
        interrupt_unlock();/* end of interrupt uninstallation */
#endif
#endif

    /* Update the linked list and deallocate memory. Attempt to do this even
     * if there has been an error, as we still need to try to free the
     * resources used.
     *
     * First test if PIO struct is at head of list - this is a special case.
     * Otherwise, chain through the list until we find it, then remove it
     * from the list.
     */

    if ((*PIO) == PIOHead)        /* Head of list? */
    {
        PIOHead = (*PIO)->NextSTPIO;
    }
    else
    {
        /* Chain through list until we find it or until end of list */

        PIOIndex = PIOHead;
        while (PIOIndex != NULL)
        {
            if(PIOIndex->NextSTPIO == (*PIO))
            {
                /* Found it - dechain it from list */
                PIOIndex->NextSTPIO = (*PIO)->NextSTPIO;
                break;
            }
            else   /* Not yet found, try next in list */
                PIOIndex = PIOIndex->NextSTPIO;
        }
    }

    /*
     * Now that the interrupts have been turned off,
     * deallocate the PIOInit memory (returns void)
     */
#if defined(ST_OSLINUX)
    iounmap((unsigned long*)(*PIO)->PioMappingBaseAddress);
    release_mem_region ((unsigned long)(stpio_Reg_t*)(*PIO)->BaseAddress,
                                    (unsigned long)(stpio_Reg_t*)(*PIO)->PioMappingWidth);
#endif
    memory_deallocate ((partition_t *) (*PIO)->Partition, (*PIO));
    return Error;
}

/* End of module */
