/*******************************************************************************

File name   : blt_init.c

Description : Init module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
13 Jun 2000        Created                                          TM
29 May 2006        Modified for WinCE platform						WinCE Team-ST Noida
*****************************************************************************/

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#include <string.h>
#endif

#if defined(ST_OS21) && !defined(ST_OSWINCE)
#include <os21/interrupt.h>
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

#include "stddefs.h"
#include "blit_rev.h"
#ifdef ST_OSLINUX
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stblit_ioctl.h"
#endif
#else
#include "sttbx.h"
#include "stdevice.h" /* chip-specific constants*/
#endif
#include "stblit.h"
#include "blt_init.h"
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stlayer.h"
#include "stdevice.h"
#include "blt_mem.h"
#else
#include "stavmem.h"
#endif
#include "blt_pool.h"
#include "blt_be.h"
#include "stevt.h"
#include "bltswcfg.h"
#include "blt_job.h"
#include "blt_flt.h"

#ifdef STBLIT_EMULATOR
#include "blt_emu.h"
#endif

/* Private Types ------------------------------------------------------------ */
#ifdef STBLIT_LINUX_FULL_USER_VERSION


int DevBlitFile = -1 ;


typedef struct
{
    U32             StartAddress;       /* Physical area start address */
    U32             MappingIndex;
    int           * MappingNumber_p;
    void         ** MappedAddress_pp;
} MappingArea_t;
#endif

/* Private Constants -------------------------------------------------------- */

#define STBLIT_MAX_DEVICE  1      /* Max number of Init() allowed */
#define STBLIT_MAX_OPEN    10     /* Max number of Open() allowed per Init() */
#define STBLIT_MAX_UNIT    (STBLIT_MAX_OPEN * STBLIT_MAX_DEVICE)

#define INVALID_DEVICE_INDEX (-1)

/* Alpha4 to Alpha8 conversion palette related : 16 entries of 32 bits = 64 byte */
#define STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE  64

/* Private Variables (static)------------------------------------------------ */

static stblit_Device_t DeviceArray[STBLIT_MAX_DEVICE];
static stblit_Unit_t UnitArray[STBLIT_MAX_UNIT];

static semaphore_t* InstancesAccessControl;   /* Init/Open/Close/Term protection semaphore */

static BOOL FirstInitDone = FALSE;

/* 16 ARGB8888 entries with 0:128 alpha range */
static const U8 Alpha4To8Palette [STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE] =
{
 /* B ,G, R, A */      /* ALPHA4 */

    0, 0, 0, 0,        /*    0   */
    0, 0, 0, 12,       /*    1   */
    0, 0, 0, 20,       /*    2   */
    0, 0, 0, 28,       /*    3   */
    0, 0, 0, 36,       /*    4   */
    0, 0, 0, 44,       /*    5   */
    0, 0, 0, 52,       /*    6   */
    0, 0, 0, 60,       /*    7   */
    0, 0, 0, 68,       /*    8   */
    0, 0, 0, 76,       /*    9   */
    0, 0, 0, 84,       /*    10  */
    0, 0, 0, 92,       /*    11  */
    0, 0, 0, 100,      /*    12  */
    0, 0, 0, 108,      /*    13  */
    0, 0, 0, 116,      /*    14  */
    0, 0, 0, 128       /*    15  */
};


/* Global Variables --------------------------------------------------------- */

#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)

#define STBLIT_MAPPING_WIDTH        0x1000

#define BLIT_PARTITION_AVMEM 0  /* Which partition to use for allocating in AVMEM memory */
#define BLIT_SECURED_PARTITION_AVMEM 2  /* Which partition to use for allocating in SECURED AVMEM memory */

typedef struct STBLITMod_Param_s
{
    int           BlitMappingIndex;
    int           BlitBaseMappingNumber;  /* the number the base adress mapping is needed by other drivers */
    unsigned long BlitBaseAddress;   /* Blit Register base address to map */
    unsigned long BlitAddressWidth;  /* Blit address range */
} STBLITMod_Param_t;


extern STAVMEM_PartitionHandle_t STAVMEM_GetPartitionHandle( U32 PartitionID );
STAVMEM_PartitionHandle_t PartitionHandle;
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
STLAYER_Handle_t LayerHndl = 0x123;  /* Handle for allocating in AVMEM */
#endif


/* Private Macros ----------------------------------------------------------- */

/* Passing a (STBLIT_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stblit_Unit_t *) (Handle)) >= UnitArray) &&                    \
                               (((stblit_Unit_t *) (Handle)) < (UnitArray + STBLIT_MAX_UNIT)) &&  \
                               (((stblit_Unit_t *) (Handle))->UnitValidity == STBLIT_VALID_UNIT))

/*#if defined (ST_5528) || (ST_7710) */
/*#pragma ST_translate(interrupt_trigger_mode_level, "interrupt_trigger_mode_level%os")*/
/*interrupt_trigger_mode_t interrupt_trigger_mode_level(int Level, interrupt_trigger_mode_t trigger_mode);*/
/*#endif*/                              /* work arround */

/* Private Function prototypes ---------------------------------------------- */

#ifdef ST_OSWINCE
// EnterCriticalSection & LeaveCriticalSection are WinCE system calls
#define EnterCriticalSection myEnterCriticalSection
#define LeaveCriticalSection myLeaveCriticalSection
#endif

static void EnterCriticalSection(void);
static void LeaveCriticalSection(void);
static ST_ErrorCode_t SetupEventsAndIT(stblit_Device_t* Device_p,const ST_DeviceName_t DeviceName);
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);
static ST_ErrorCode_t Init(stblit_Device_t * const Device_p, const STBLIT_InitParams_t * const InitParams_p, const ST_DeviceName_t DeviceName);
static ST_ErrorCode_t Open(stblit_Unit_t * const Unit_p);
static ST_ErrorCode_t Close(stblit_Unit_t * const Unit_p);
static ST_ErrorCode_t Term(stblit_Device_t * const Device_p);

#ifdef STBLIT_LINUX_FULL_USER_VERSION
static ST_ErrorCode_t ModuleMapRegisters(int FileNo, unsigned long Address, MappingArea_t  * MappingArea_p, void ** VirtualAdress_pp);
static ST_ErrorCode_t ModuleUnmapRegisters(int FileNo, MappingArea_t * MappingArea_p);
/*static ST_ErrorCode_t InitInterruptEventTask( stblit_Device_t * const Device_p );*/
static void stblit_InterruptEvent_Task(void* data_p);
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
extern int fileno(FILE*);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

/* Functions ---------------------------------------------------------------- */
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

#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
    task_lock();
#endif
    if (!InstancesAccessControlInitialized)
    {
        InstancesAccessControlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue)
         * */
        InstancesAccessControl = STOS_SemaphoreCreateFifo( NULL, 1 );
    }

#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
    task_unlock();
#endif
    /* Wait for the Init/Open/Close/Term protection semaphore */
    STOS_SemaphoreWait(InstancesAccessControl);

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
    STOS_SemaphoreSignal(InstancesAccessControl);

} /* End of LeaveCriticalSection() function */


/*******************************************************************************
Name        : SetupEventsAndIT
Description : Open  event handler, register and subscribe events. Also install IT in case
              of no intmr module support (5528,7710)
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t SetupEventsAndIT(stblit_Device_t* Device_p,const ST_DeviceName_t DeviceName)
{
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    STEVT_OpenParams_t              OpenParams;
    STEVT_DeviceSubscribeParams_t   SubscribeParams;
#endif
    ST_ErrorCode_t                  Err = ST_NO_ERROR;
    char                            TmpStr[10];


    strcpy(TmpStr,"BLITTER");

#ifdef STBLIT_LINUX_FULL_USER_VERSION
	/* To remove comipation warnning */
	UNUSED_PARAMETER(DeviceName);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */


#ifdef STBLIT_LINUX_FULL_USER_VERSION
    Err = ST_NO_ERROR;
#else
    /* Open Event handler */
    Err = STEVT_Open( Device_p->EventHandlerName, &OpenParams, &(Device_p->EvtHandle));
#endif
    if ((Err == ST_NO_ERROR) && ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7015) ||
                                 (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
                                 (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_ST40GX1)))
    {
#ifndef STBLIT_LINUX_FULL_USER_VERSION
        /* Subscribe to the HW blitter interruption */
#if defined(ST_OS20) ||defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)
        SubscribeParams.NotifyCallback      = (STEVT_DeviceCallbackProc_t)stblit_BlitterInterruptEventHandler;
#endif
        SubscribeParams.SubscriberData_p    = (void*)Device_p;
        Err = STEVT_SubscribeDeviceEvent(Device_p->EvtHandle, /*(char*)*/Device_p->BlitInterruptEventName,
                                         Device_p->BlitInterruptEvent,&SubscribeParams);
#endif  /* STBLIT_LINUX_FULL_USER_VERSION */
    }
    else if ((Err == ST_NO_ERROR) && ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528)
                || (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710)
                || (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100)) )
    {
        STOS_InterruptLock();

        #ifdef STBLIT_LINUX_FULL_USER_VERSION
			/* task used to signal interrupt process  */
        #ifdef USER_TASK_CREATE
           Err = STBLIT_UserTaskCreate( (void*) stblit_InterruptHandlerTask,
					&(Device_p->InterruptHandlerTask),
                    NULL,
                    (void*) Device_p );
		#else /* WA to fix compilation warnning
                (ISO C forbids passing argument 3 of bpthread_createb between function pointer and bvoid *b) */
			  /* NOTE : (void*(*)(void*)) is a temporary solution to remove warnings, it will be reviewed */
			if (pthread_create( &(Device_p->InterruptHandlerTask), NULL, (void*(*)(void*))stblit_InterruptHandlerTask , (void*)Device_p))
			{
    		    Err = ST_ERROR_BAD_PARAMETER;
			}
			else
			{
				Err = ST_NO_ERROR;
			}
		#endif
            if (Err != ST_NO_ERROR)
			{
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :stblit_InterruptHandlerTask\n" ));
				return(ST_ERROR_BAD_PARAMETER);
		    }

			/* Create InterruptEvent task used to notify user when interrupt from blitter engine is generated*/
		#if 0
			Err = STBLIT_UserTaskCreate( (void*) stblit_InterruptEvent_Task,
					&(Device_p->InterruptEventTask),
                    NULL,
                    (void*) Device_p );
		#else /* WA to fix compilation warnning
                (ISO C forbids passing argument 3 of bpthread_createb between function pointer and bvoid *b) */
              /* NOTE : (void*(*)(void*)) is a temporary solution to remove warnings, it will be reviewed */
			if (pthread_create( &(Device_p->InterruptEventTask), NULL, (void*(*)(void*))stblit_InterruptEvent_Task, (void*)Device_p))
			{
    		    Err = ST_ERROR_BAD_PARAMETER;
			}
			else
			{
				Err = ST_NO_ERROR;
			}
		#endif
            if (Err != ST_NO_ERROR)
			{
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :stblit_InterruptEvent_Task\n" ));
				return(ST_ERROR_BAD_PARAMETER);
		    }

            /* install interrupt */
			Err = ioctl(fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_INSTALL_INT);
        #endif

            Err = STOS_InterruptInstall( Device_p->BlitInterruptNumber,
                    Device_p->BlitInterruptLevel,
                    stblit_BlitterInterruptHandler,
                    TmpStr,
                    (void *) Device_p);

            if ( Err )
            {
                Err = ST_ERROR_INTERRUPT_INSTALL;
            }

/*#ifdef ST_5528*/
        /* Work around 5528 */
/*        interrupt_trigger_mode_level(Device_p->BlitInterruptLevel, interrupt_trigger_mode_high_level);*/
/*#endif*/

        STOS_InterruptUnlock();

#ifndef STBLIT_LINUX_FULL_USER_VERSION
        if (Err == ST_NO_ERROR)
        {
            if (STOS_InterruptEnable(Device_p->BlitInterruptNumber,Device_p->BlitInterruptLevel)!=0)
            {
               Err = ST_ERROR_INTERRUPT_INSTALL;
            }
        }
#endif

    }

#ifndef STBLIT_LINUX_FULL_USER_VERSION
    /* Register different API events */
     if (Err == ST_NO_ERROR)
    {
        Err = STEVT_RegisterDeviceEvent(Device_p->EvtHandle,DeviceName,
                                        STBLIT_BLIT_COMPLETED_EVT, &(Device_p->EventID[STBLIT_BLIT_COMPLETED_ID]));
    }
    if (Err == ST_NO_ERROR)
    {
        Err = STEVT_RegisterDeviceEvent(Device_p->EvtHandle,DeviceName,
                                        STBLIT_JOB_COMPLETED_EVT, &(Device_p->EventID[STBLIT_JOB_COMPLETED_ID]));
    }

    if (Err != ST_NO_ERROR) /* Registering failed but STEVT_Open succeeded : close */
    {
        if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error in STEVT_Close(%lu)", Device_p->EvtHandle));
        }
    }

#endif /* !STBLIT_LINUX_FULL_USER_VERSION */

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error %X while Opening EventHandler %s or registering events",
                 Err, Device_p->EventHandlerName));
        Err = ST_ERROR_BAD_PARAMETER;
    }


    return(Err);

}

#ifdef STBLIT_LINUX_FULL_USER_VERSION

/*-----------------------------------------------------------------------------
 * Function : BlitCompletion_Task
 * Input    : *Context_p
 * Output   :
 * Return   : void
 * --------------------------------------------------------------------------*/
ST_ErrorCode_t STBLIT_NotifyCompletion(BOOL SimpleBlit)
{
    while (1)
    {
        if ( SimpleBlit )
        {
            STOS_SemaphoreWait( BlitCompletionSemaphore );
        }
        else
        {
            STOS_SemaphoreWait( JobCompletionSemaphore );
        }
        return ST_NO_ERROR ;
    }
}

/*******************************************************************************
Name        : ModuleMapRegisters
Description : LINUX specific initialisations
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the LINUX mapping function
*******************************************************************************/
static ST_ErrorCode_t ModuleMapRegisters(int FileNo,
                                         unsigned long Address,
                                         MappingArea_t  * MappingArea_p,
                                         void ** VirtualAdress_pp)
{
    STBLITMod_Param_t  BlitModuleParam;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
	/* To remove comipation warnning */
	UNUSED_PARAMETER(Address);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

    /* Determinating length of mapping */
    /* Should be mapped according to LinuxPageSize (=0x1000)*/
    BlitModuleParam.BlitBaseAddress = MappingArea_p->StartAddress;
    BlitModuleParam.BlitAddressWidth = 0x1000;    /* CD: to be modified ... */
    BlitModuleParam.BlitMappingIndex = MappingArea_p->MappingIndex;

    /* Giving parameters to the module. Open is done in API. To be checked */
    ioctl(FileNo, STBLIT_IOCTL_ADDRESS_PARAM, &BlitModuleParam);

#ifdef DEBUG
    printf("shared virtual address=%x\n", *(MappingArea_p->MappedAddress_pp));
#endif
    /* mapping */
    if ((ErrorCode = ioctl(FileNo, STBLIT_IOCTL_MAP)) == ST_NO_ERROR)
    {
        *VirtualAdress_pp = mmap(0,
                                 BlitModuleParam.BlitAddressWidth,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 FileNo,
                                 (unsigned long) (BlitModuleParam.BlitBaseAddress) | REGION2);

        if (*VirtualAdress_pp == (void *)-1)
        {
            /* unmapping */
            ioctl(FileNo, STBLIT_IOCTL_UNMAP, &BlitModuleParam);

            ErrorCode = ST_ERROR_BAD_PARAMETER;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "BLIT: Mapping problem: Mapping Failed !\n"));
            printf("BLIT: Mapping problem: Mapping Failed !\n");
        }
        else
        {
            *(MappingArea_p->MappedAddress_pp) = *VirtualAdress_pp;
            (*(MappingArea_p->MappingNumber_p))++;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLIT: Mapping successfull: %x from %x to %x\n",
                                            *VirtualAdress_pp,
                                            BlitModuleParam.BlitBaseAddress,
                                            BlitModuleParam.BlitBaseAddress + BlitModuleParam.BlitAddressWidth));
        }
    }
    else
    {
        /* There was an error */
        if (ErrorCode == STBLIT_ALREADY_MAPPED)
        {
            /* Address is probably mapped by another module. Getting the virtual address.. */
            if (*(MappingArea_p->MappedAddress_pp))
            {
                *VirtualAdress_pp = *(MappingArea_p->MappedAddress_pp);
                (*(MappingArea_p->MappingNumber_p))++;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLIT: Already mapped: at %x\n",
                                                                    *VirtualAdress_pp));
                ErrorCode = ST_NO_ERROR;
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLIT: Mapping problem: multi-mapping not supported !\n"));
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLIT: Mapping problem: mapping problem !\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    return (ErrorCode);
}


static ST_ErrorCode_t ModuleUnmapRegisters(int FileNo, MappingArea_t * MappingArea_p)
{
    STBLITMod_Param_t   BlitModuleParam;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    (*(MappingArea_p->MappingNumber_p))--;
    if (*(MappingArea_p->MappingNumber_p) <= 0)
    {
        /* We have the responsability to unmap the shared base region */
        BlitModuleParam.BlitBaseAddress = MappingArea_p->StartAddress;
        BlitModuleParam.BlitAddressWidth = 0x1000;    /* CD: to be modified ... */
        BlitModuleParam.BlitMappingIndex = MappingArea_p->MappingIndex;

        /* unmapping */
        ioctl(FileNo, STBLIT_IOCTL_UNMAP, &BlitModuleParam);

        /* Allow proper file close. No call to driver. */
        /* Retrieving the MappedBaseAddress */
        printf("BLITTER: Unmapping: BaseAddress=%x\n", (U32)(*(MappingArea_p->MappedAddress_pp)));
        munmap(*(MappingArea_p->MappedAddress_pp), BlitModuleParam.BlitAddressWidth);

        /* clearing share allocation pointer */
        *(MappingArea_p->MappingNumber_p) = 0;
        *(MappingArea_p->MappedAddress_pp) = NULL;
    }
    else
    {
        printf("BLITTER: Unmapping: Shared base region still mapped for other drivers\n");
    }

    return (ErrorCode);
}

/* task that read from kernel */
void stblit_InterruptEvent_Task(void* data_p)
{
    stblit_Device_t*    Device_p;
    int  EventCount ;
    char DataArrived[4];

    Device_p = (stblit_Device_t*) data_p;

    printf("stblit_interruptevent_Task():Starting event interrupt task...\n");

	while(Device_p->TaskTerminate == FALSE)
	{
        EventCount = read(fileno(Device_p->BLIT_DevFile), DataArrived,  1);
        if ( EventCount  > 0)
        {
            STOS_SemaphoreSignal( Device_p->BlitInterruptHandlerSemaphore );
        }                               /* EventCount > 0 */
	}                                   /* while 1 */

}

/* Function to create InterruptEvent task used to notify user when interrupt from blitter engine is generated*/
#if 0 /* WA to fix compilation warnning
        (ISO C forbids passing argument 3 of bpthread_createb between function pointer and bvoid *b) */
ST_ErrorCode_t STBLIT_UserTaskCreate( void* UserTask, pthread_t *TaskID, pthread_attr_t *Task_Attribute, void * const Device_p )
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

	if (pthread_create( TaskID, NULL, (void*)UserTask, (void*)Device_p))
	{
        ErrorCode = ST_ERROR_BAD_PARAMETER;
	}

    return ErrorCode;
}
#endif

ST_ErrorCode_t STBLIT_UserTaskTerm( pthread_t TaskID, pthread_attr_t *Task_Attribute )
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

	/* To remove comipation warnning */
	UNUSED_PARAMETER(Task_Attribute);


    if ( TaskID )
    {
        if (pthread_cancel(TaskID))
        {
            STTBX_Report (( STTBX_REPORT_LEVEL_ERROR,"STBLIT_UserTaskTerm(): unable to kill task\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    return(ErrorCode);
}

#endif

/*******************************************************************************
Name        : Init
Description : API specific initialisations
Parameters  : pointer on device and initialisation parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t Init(stblit_Device_t * const Device_p,
                           const STBLIT_InitParams_t * const InitParams_p,
                           const ST_DeviceName_t DeviceName)
{
   ST_ErrorCode_t             Err = ST_NO_ERROR;

#ifdef STBLIT_LINUX_FULL_USER_VERSION
   STLAYER_AllocDataParams_t  AllocDataParams;
#else
   STAVMEM_AllocBlockParams_t AllocBlockParams;
   STAVMEM_MemoryRange_t      RangeArea[2];
   U8                         NbForbiddenRange;
#endif
   void*                      Elembuffer_p;    /* Virtual memory in case of shared memory */
   void**                     Handlebuffer_p;
   U32                        i,j;
   stblit_Job_t*              Job_p;
   stblit_JobBlit_t*          JBlit_p;
   stblit_Node_t*             JBlitNode_p;
   stblit_Node_t*             SBlitNode_p;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    char                      filename[32];
#endif
#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
   STBLITMod_Param_t          BlitModuleParam; /* blit modules parameters */
#endif
   U32                        TempAddress;

#ifdef ST_OSLINUX
	/* To remove comipation warnning */
	UNUSED_PARAMETER(TempAddress);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */


    /* common stuff initialization*/
    Device_p->DeviceType                = InitParams_p->DeviceType;
    Device_p->CPUPartition_p            = InitParams_p->CPUPartition_p;

#ifndef STBLIT_LINUX_FULL_USER_VERSION
#ifdef ST_OSLINUX
    PartitionHandle = STAVMEM_GetPartitionHandle( BLIT_PARTITION_AVMEM );
#endif

#ifdef ST_OSLINUX
    Device_p->AVMEMPartition            = PartitionHandle;
#else
    Device_p->AVMEMPartition            = InitParams_p->AVMEMPartition;
#endif
    /* STAVMEM mapping */


#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2( BLIT_PARTITION_AVMEM, &Device_p->VirtualMapping);
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&Device_p->VirtualMapping);
#endif

      /* FilterFlicker Mode initialization*/

         Device_p->FlickerFilterMode  = STBLIT_FLICKER_FILTER_MODE_ADAPTIVE ;

    Device_p->SharedMemoryBaseAddress_p = InitParams_p->SharedMemoryBaseAddress_p;

    /* Forbidden range : All Virtual memory range but Virtual window
    *  VirtualWindowOffset may be 0
    *  Virtual Size is always > 0
    *  Assumption : Physical base Address from device = 0 !!!! */
    NbForbiddenRange = 1;
    if (Device_p->VirtualMapping.VirtualWindowOffset > 0)
    {
        RangeArea[0].StartAddr_p = (void *) Device_p->VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                         (U32)(Device_p->VirtualMapping.VirtualWindowOffset) - 1);
    }
    else /*  VirtualWindowOffset = 0 */
    {
        RangeArea[0].StartAddr_p = (void *) Device_p->VirtualMapping.VirtualBaseAddress_p;
        RangeArea[0].StopAddr_p  = (void *) RangeArea[0].StartAddr_p;
    }

    if ((Device_p->VirtualMapping.VirtualWindowOffset + Device_p->VirtualMapping.VirtualWindowSize) !=
        Device_p->VirtualMapping.VirtualSize)
    {
        RangeArea[1].StartAddr_p = (void *) ((U32)(RangeArea[0].StartAddr_p) +
                                            (U32)(Device_p->VirtualMapping.VirtualWindowOffset) +
                                            (U32)(Device_p->VirtualMapping.VirtualWindowSize));
        RangeArea[1].StopAddr_p  = (void *) ((U32)(Device_p->VirtualMapping.VirtualBaseAddress_p) +
                                                (U32)(Device_p->VirtualMapping.VirtualSize) - 1);

        NbForbiddenRange= 2;
    }
#endif

    /* Register base address related : In emulation/sofware mode, registers are allocated by driver in order to ensure they are not
       in a cached area : so in AV Memory*/
#ifdef STBLIT_EMULATOR

    AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
    AllocBlockParams.Size                     = 168;
    AllocBlockParams.Alignment                = 0;
    AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
    AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
    AllocBlockParams.NumberOfForbiddenBorders = 0;
    AllocBlockParams.ForbiddenBorderArray_p   = NULL;

    Err = STAVMEM_AllocBlock(&AllocBlockParams,&(Device_p->EmulatorRegisterHandle));
    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
        return(STBLIT_ERROR_NO_AV_MEMORY);
    }

    STAVMEM_GetBlockAddress(Device_p->EmulatorRegisterHandle,(void**)&Elembuffer_p);
    /* Convert Virtual buffer to CPU buffer */
    Device_p->BaseAddress_p  = STAVMEM_VirtualToCPU((void*)Elembuffer_p,&(Device_p->VirtualMapping));

#else

#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
		/* Determinating length of mapping */
        Device_p->BlitMappedWidth = (U32)(STBLIT_MAPPING_WIDTH);


        /* base address of BLIT mapping */

        BlitModuleParam.BlitBaseAddress = (unsigned long)InitParams_p->BaseAddress_p;
        BlitModuleParam.BlitAddressWidth = (unsigned long)Device_p->BlitMappedWidth;

        Device_p->BlitMappedBaseAddress_p = STOS_MapRegisters( (void*)BlitModuleParam.BlitBaseAddress, BlitModuleParam.BlitAddressWidth, "BLIT");
        if ( Device_p->BlitMappedBaseAddress_p == NULL )
        {
            Err =  STBLIT_ERROR_MAP_BLIT;
        }
#ifdef BLIT_DEBUG
        STTBX_Print(("BLIT virtual io kernel address of phys %lX = %lX\n", (unsigned long)(BlitModuleParam.BlitBaseAddress | REGION2), (unsigned long)(Device_p->BlitMappedBaseAddress_p)));
#endif

        if ( Err == ST_NO_ERROR)
	    {
            /** affecting BLIT Base Adress with mapped Base Adress **/
            Device_p->BaseAddress_p = Device_p->BlitMappedBaseAddress_p;
	    }
#else
         Device_p->BaseAddress_p  = InitParams_p->BaseAddress_p;
#endif
#ifdef STBLIT_LINUX_FULL_USER_VERSION

    if (Err == ST_NO_ERROR)
    {
        /* opening the module */
        sprintf( filename,"%s%s",  STAPI_DEVICE_DIRECTORY, STBLIT_IOCTL_LINUX_DEVICE_NAME );
        if (!(Device_p->BLIT_DevFile = fopen(filename,"w+")))
        {
            Err = ST_ERROR_UNKNOWN_DEVICE;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "BLIT MODULE Init Failed ! Cannot Open: %s\n", filename));
            printf("BLIT MODULE Init Failed ! Cannot Open: %s\n", filename);
        }

    }

    if (Err == ST_NO_ERROR)
    {
        MappingArea_t  MappingArea;
		U32 TempAddress1, TempAddress2, TempAddress3;

		MappingArea.StartAddress = BLITTER_BASE_ADDRESS;
        MappingArea.MappingIndex = 0;
        MappingArea.MappingNumber_p = &BlitterMappingNumber;
        MappingArea.MappedAddress_pp = &BlitterVirtualAddress;

		/* Added to fix compilation warrning (pointer of type bvoid *b used in arithmetic) */
		TempAddress1 = (U32)Device_p->BlitMappedBaseAddress_p;
		TempAddress2 = (U32)InitParams_p->BaseAddress_p;
		TempAddress3 = (U32)MappingArea.StartAddress;

        if ( (Err = ModuleMapRegisters( fileno(Device_p->BLIT_DevFile),
                                        (unsigned long)InitParams_p->BaseAddress_p,
                                        &MappingArea,
                                        &Device_p->BlitMappedBaseAddress_p)) == ST_NO_ERROR)
        {
            printf("BLITTER: Init Base@:0x%x\n", (U32)Device_p->BaseAddress_p);
            Device_p->BaseAddress_p = (void*)(TempAddress1 + TempAddress2 - TempAddress3);
            printf("BLITTER: Mapped Base@:0x%x\n", (U32)Device_p->BaseAddress_p);
        }
    }
#endif



#endif
     /* Single Blit node management initialization */
#ifdef ST_OSWINCE
	// translate BaseAddress_p to virtual memory
    Device_p->BaseAddress_p = MapPhysicalToVirtual(Device_p->BaseAddress_p,
                                                   ST7100_BLITTER_ADDRESS_WIDTH);
	if (Device_p->BaseAddress_p == NULL)
	{
		WCE_ERROR("MapPhysicalToVirtual(Device_p->BaseAddress_p)");
        return STBLIT_ERROR_FUNCTION_NOT_IMPLEMENTED; // what else ?
	}
#endif
    Device_p->SBlitNodeNumber = InitParams_p->SingleBlitNodeMaxNumber;
    if (Device_p->SBlitNodeNumber != 0)
    {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if (InitParams_p->SingleBlitNodeBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p  = (void*) InitParams_p->SingleBlitNodeBuffer_p;
		}
		else
        {
			AllocDataParams.Size                     = Device_p->SBlitNodeNumber * sizeof(stblit_Node_t);
            AllocDataParams.Alignment                = 16;  /* 16 bytes = 128 bit */
            STLAYER_AllocData( LayerHndl, &AllocDataParams, (void**)&Elembuffer_p );
            Device_p->SingleBlitNodeBuffer_p = Elembuffer_p;
            PhysicSingleBlitNodeBuffer_p = (U32)stblit_CpuToDevice((void*)Device_p->SingleBlitNodeBuffer_p) ;
		}
#else
        if (InitParams_p->SingleBlitNodeBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p  = (void*) InitParams_p->SingleBlitNodeBuffer_p;
            Device_p->SBlitNodeBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
        }
        else                                /* Driver allocation in shared memory */
        {
            AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
            AllocBlockParams.Size                     = Device_p->SBlitNodeNumber * sizeof(stblit_Node_t);
            AllocBlockParams.Alignment                = 16;  /* 16 bytes = 128 bit */
            AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
            AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
            AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;
            Err = STAVMEM_AllocBlock(&AllocBlockParams,&(Device_p->SBlitNodeBufferHandle));
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
                return(STBLIT_ERROR_NO_AV_MEMORY);
            }
            STAVMEM_GetBlockAddress(Device_p->SBlitNodeBufferHandle,(void**)&Elembuffer_p);
        }
        /* Convert Virtual buffer to CPU buffer */
        Elembuffer_p = STAVMEM_VirtualToCPU((void*)Elembuffer_p,&(Device_p->VirtualMapping));
#endif  /* STBLIT_LINUX_FULL_USER_VERSION */
        Handlebuffer_p = (void**) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                             (Device_p->SBlitNodeNumber * sizeof(stblit_Node_t*)));
        if (Handlebuffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
            return(ST_ERROR_NO_MEMORY);
        }

        stblit_InitDataPool(&(Device_p->SBlitNodeDataPool),
                            Device_p->SBlitNodeNumber,
                            sizeof(stblit_Node_t),
                            Elembuffer_p,
                            Handlebuffer_p);
        /* Initialize All Single Blit Nodes to 0 */
        SBlitNode_p = (stblit_Node_t*)Elembuffer_p;
        for (i=0; i < Device_p->SBlitNodeNumber; i++)
        {
/*            memset(SBlitNode_p, 0, sizeof(stblit_Node_t));*/
            for (j=0;j < sizeof(stblit_Node_t) /4 ;j++ )
            {
                STSYS_WriteRegMem32LE((void*)(((U32)SBlitNode_p) + 4 * j),0);
            }
            SBlitNode_p++;
        }
    }
    else /* Device_p->SBlitNodeNumber == 0 */
    {
        Device_p->SBlitNodeDataPool.NbFreeElem = 0;
    }

   /* Job Blit node management initialization */
    Device_p->JBlitNodeNumber = InitParams_p->JobBlitNodeMaxNumber;
    if (Device_p->JBlitNodeNumber != 0)
    {
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if (InitParams_p->SingleBlitNodeBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p  = (void*) InitParams_p->SingleBlitNodeBuffer_p;
		}
		else
        {
            AllocDataParams.Size                     = Device_p->JBlitNodeNumber * sizeof(stblit_Node_t);
			AllocDataParams.Alignment                = 16;  /* 16 bytes = 128 bit */
            STLAYER_AllocData( LayerHndl, &AllocDataParams, (void**)&Elembuffer_p );
            Device_p->JobBlitNodeBuffer_p = Elembuffer_p;
		}

        /* Variable used to convert Node addresses */
        PhysicJobBlitNodeBuffer_p = 0 ;
#else
        if (InitParams_p->JobBlitNodeBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p  = (void*) InitParams_p->JobBlitNodeBuffer_p;
            Device_p->JBlitNodeBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
        }
        else                                /* Driver allocation in shared memory */
        {
            AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
            AllocBlockParams.Size                     = Device_p->JBlitNodeNumber * sizeof(stblit_Node_t);
            AllocBlockParams.Alignment                = 16;  /* 16 bytes = 128 bit */
            AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
            AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
            AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
            AllocBlockParams.NumberOfForbiddenBorders = 0;
            AllocBlockParams.ForbiddenBorderArray_p   = NULL;

            Err = STAVMEM_AllocBlock(&AllocBlockParams,&(Device_p->JBlitNodeBufferHandle));
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
                return(STBLIT_ERROR_NO_AV_MEMORY);
            }
            STAVMEM_GetBlockAddress(Device_p->JBlitNodeBufferHandle,(void**)&Elembuffer_p);
        }
        /* Convert Virtual buffer to CPU buffer */
        Elembuffer_p = STAVMEM_VirtualToCPU((void*)Elembuffer_p,&(Device_p->VirtualMapping));
#endif  /* STBLIT_LINUX_FULL_USER_VERSION */
        Handlebuffer_p = (void**) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                                 (Device_p->JBlitNodeNumber * sizeof(stblit_Node_t*)));
        if (Handlebuffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
            return(ST_ERROR_NO_MEMORY);
        }

        stblit_InitDataPool(&(Device_p->JBlitNodeDataPool),
                            Device_p->JBlitNodeNumber,
                            sizeof(stblit_Node_t),
                            Elembuffer_p,
                            Handlebuffer_p);

        /* Initialize All Job Nodes to 0 */
        JBlitNode_p = (stblit_Node_t*)Elembuffer_p;
        for (i=0; i < Device_p->JBlitNodeNumber; i++)
        {
/*            memset(JBlitNode_p, 0, sizeof(stblit_Node_t));*/
            for (j=0;j < sizeof(stblit_Node_t) /4 ;j++ )
            {
                STSYS_WriteRegMem32LE((void*)(((U32)JBlitNode_p) + 4 * j),0);
            }
            JBlitNode_p++;
        }
    }
    else /* Device_p->JBlitNodeNumber == 0 */
    {
        Device_p->JBlitNodeDataPool.NbFreeElem = 0;
    }

    /* Job descriptor management initialization */
    Device_p->JobNumber = InitParams_p->JobMaxNumber ;
    if (Device_p->JobNumber != 0)
    {
        if (InitParams_p->JobBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p = (void*) InitParams_p->JobBuffer_p;
            Device_p->JobBufferUserAllocated = TRUE ;
        }
        else
        {
            Elembuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                              (Device_p->JobNumber * sizeof(stblit_Job_t)));
            if (Elembuffer_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
                return(ST_ERROR_NO_MEMORY);
            }
            Device_p->JobBufferUserAllocated = FALSE ;
        }
        Handlebuffer_p = (void**) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                             (Device_p->JobNumber * sizeof(stblit_Job_t*)));
        if (Handlebuffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
            return(ST_ERROR_NO_MEMORY);
        }

        stblit_InitDataPool(&(Device_p->JobDataPool),
                            Device_p->JobNumber,
                            sizeof(stblit_Job_t),
                            Elembuffer_p,
                            Handlebuffer_p);

        /* Initialize Job Validity field in the JobDataPool */
        Job_p = (stblit_Job_t*)Elembuffer_p;
        for (i=0; i < Device_p->JobNumber; i++)
        {
            Job_p->JobValidity = (U32)(~STBLIT_VALID_JOB);
            Job_p++;
        }
    }
    else /* Device_p->JobNumber == 0 */
    {
        Device_p->JobDataPool.NbFreeElem = 0;
    }

    /* Job Blit descriptor management initialization */
    Device_p->JBlitNumber = InitParams_p->JobBlitMaxNumber ;
    if (Device_p->JBlitNumber != 0)
    {
        if (InitParams_p->JobBlitBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p = (void*) InitParams_p->JobBlitBuffer_p;
            Device_p->JBlitBufferUserAllocated = TRUE ;
        }
        else
        {
            Elembuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                                  (Device_p->JBlitNumber * sizeof(stblit_JobBlit_t)));
            if (Elembuffer_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
                return(ST_ERROR_NO_MEMORY);
            }
            Device_p->JBlitBufferUserAllocated = FALSE ;
        }
        Handlebuffer_p = (void**) STOS_MemoryAllocate(Device_p->CPUPartition_p,
                                                 (Device_p->JBlitNumber * sizeof(stblit_JobBlit_t*)));
        if (Handlebuffer_p == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "CPU memory allocation failed"));
            return(ST_ERROR_NO_MEMORY);
        }
        stblit_InitDataPool(&(Device_p->JBlitDataPool),
                            Device_p->JBlitNumber,
                            sizeof(stblit_JobBlit_t),
                            Elembuffer_p,
                            Handlebuffer_p);

        /* Initialize Job Blit Validity field in the JBlitDataPool */
        JBlit_p = (stblit_JobBlit_t*)Elembuffer_p;
        for (i=0; i < Device_p->JBlitNumber; i++)
        {
            memset(JBlit_p, 0, sizeof(stblit_JobBlit_t));
            JBlit_p->JBlitValidity = (U32)(~STBLIT_VALID_JOB_BLIT);
            JBlit_p++;
        }
    }
    else /* Device_p->JBlitNumber == 0 */
    {
        Device_p->JBlitDataPool.NbFreeElem = 0;
    }


    /* Work buffer initialization :
     *   + Used as a 5-TAP filter coefficients pool
     *   + Used to store the palette for Alpha4 to Alpha8 conversion */
    Device_p->WorkBufferSize  = InitParams_p->WorkBufferSize;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if (InitParams_p->WorkBufferUserAllocated == TRUE)  /* buffer which is user allocated */
    {
        Device_p->WorkBuffer_p     = (void*) InitParams_p->WorkBuffer_p;
	}
	else
	{
        AllocDataParams.Size                     = Device_p->WorkBufferSize;
		AllocDataParams.Alignment                = 16;  /* 16 bytes = 128 bit */
        STLAYER_AllocData( LayerHndl, &AllocDataParams, (void**)&(Device_p->WorkBuffer_p));
    }
#else
    if (InitParams_p->WorkBufferUserAllocated == TRUE)  /* buffer which is user allocated */
    {
        Device_p->WorkBuffer_p     = (void*) InitParams_p->WorkBuffer_p;
        Device_p->WorkBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    }
    else                                /* Driver allocation in shared memory */
    {
        AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
        AllocBlockParams.Size                     = Device_p->WorkBufferSize;
        AllocBlockParams.Alignment                = 16;  /* 16 bytes = 128 bit */
        AllocBlockParams.AllocMode                = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
        AllocBlockParams.NumberOfForbiddenRanges  = NbForbiddenRange;
        AllocBlockParams.ForbiddenRangeArray_p    = &RangeArea[0];
        AllocBlockParams.NumberOfForbiddenBorders = 0;
        AllocBlockParams.ForbiddenBorderArray_p   = NULL;

        Err = STAVMEM_AllocBlock(&AllocBlockParams,&(Device_p->WorkBufferHandle));
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Shared memory allocation failed"));
            return(STBLIT_ERROR_NO_AV_MEMORY);
        }
        STAVMEM_GetBlockAddress(Device_p->WorkBufferHandle,(void**)&(Device_p->WorkBuffer_p));
    }
#endif   /* STBLIT_LINUX_FULL_USER_VERSION */

    /* Set the filter pointer to the first coeff in filter buffer : In this implementatin the filter buffer is the WorkBuffer !*/
    Device_p->FilterBuffer_p = (U8*) Device_p->WorkBuffer_p;

#ifdef ST_OSLINUX
    {
        U32 Tmp;
        for ( Tmp = 0; Tmp <  STBLIT_DEFAULT_FILTER_BUFFER_SIZE ; Tmp++ )
        {
            *((U8 *) (((U8 *) Device_p->FilterBuffer_p) + Tmp)) = FilterBuffer[Tmp];
        }
    }
#else
	/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
	TempAddress = (U32)FilterBuffer;

	/* Copy filter buffer coeffs */
    STAVMEM_CopyBlock1D((void*)TempAddress,(void*)(STAVMEM_VirtualToCPU((void*)Device_p->FilterBuffer_p,&(Device_p->VirtualMapping))),
                                                    STBLIT_DEFAULT_FILTER_BUFFER_SIZE);
#endif

    /* Set Palette pointer to address right after the last filter coefficient in work buffer */
    Device_p->Alpha4To8Palette_p = Device_p->FilterBuffer_p +  STBLIT_DEFAULT_FILTER_BUFFER_SIZE;


#ifdef ST_OSLINUX
    {
        U32 Tmp;
        for ( Tmp = 0; Tmp <  STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE ; Tmp++ )
        {
            *((U8 *) (((U8 *) Device_p->Alpha4To8Palette_p) + Tmp)) = Alpha4To8Palette[Tmp];
        }
    }
#else

	/* Get pointer address (This is a temporary solution to remove warnings, it will be reviewed) */
	TempAddress = (U32)Alpha4To8Palette;

	/* Copy 16 entries Palette for Alpha4 To Alpha8 conversion */
    STAVMEM_CopyBlock1D((void*)TempAddress,(void*)(STAVMEM_VirtualToCPU((void*)Device_p->Alpha4To8Palette_p,&(Device_p->VirtualMapping))),
                                                 STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE);
#endif

    /* Back-end initializations */
    Device_p->HWUnLocked                    = TRUE;
    Device_p->InsertionRequest              = FALSE;
    Device_p->FirstNode_p                   = NULL;
    Device_p->LastNode_p                    = NULL;
    Device_p->LastNodeInBlitList_p          = NULL;
    Device_p->InsertAtFront                 = FALSE;
    Device_p->LowPriorityLockSent           = FALSE;
    Device_p->BlitterIdle                   = TRUE;
    Device_p->LockRequest                   = FALSE;
    Device_p->UnlockRequest                 = FALSE;
    Device_p->LockBeforeFirstNode           = FALSE;
    Device_p->AntiFlutterOn                 = FALSE; /*by default the AntiFlutter on target bitmap is disabled */
    Device_p->AntiAliasingOn                = FALSE; /*by default the AntiAliasing on target bitmap is disabled */

    /* ... TB updated ..*/

    /* Register initialization */


    /* semaphore initialization */
    Device_p->BackEndQueueControl       =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->HWUnlockedSemaphore       =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->InsertionDoneSemaphore    =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->UnlockInstalledSemaphore  =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->LockInstalledSemaphore    =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->SlaveTerminatedSemaphore  =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->InterruptProcessSemaphore =STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->AccessControl             =STOS_SemaphoreCreateFifo(NULL,1);
    #ifdef ST_OSLINUX
        #ifdef STBLIT_LINUX_FULL_USER_VERSION
            Device_p->BlitInterruptHandlerSemaphore     =STOS_SemaphoreCreateFifo(NULL,0);
            BlitCompletionSemaphore                     =STOS_SemaphoreCreateFifo(NULL,0);
            JobCompletionSemaphore                      =STOS_SemaphoreCreateFifo(NULL,0);
        #else
            Device_p->MasterTerminatedSemaphore  =STOS_SemaphoreCreateFifo(NULL,0);
        #endif
    #endif

    Device_p->AllBlitsCompleted_p             = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->StartWaitBlitComplete          = FALSE;



    #if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)
		/* Low Priority Message queue allocation and initialization */
/*        Device_p->LowPriorityQueueBuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,*/
/*                                                     sizeof(stblit_Msg_t) * STBLIT_MAX_NUMBER_MESSAGE);*/

            Device_p->LowPriorityQueue= STOS_MessageQueueCreate(sizeof(stblit_Msg_t),STBLIT_MAX_NUMBER_MESSAGE);

            if (Device_p->LowPriorityQueue == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Low Priority Queue allocation in CPU memory failed"));
                return(ST_ERROR_NO_MEMORY);
            }
			/* High Priority Message queue allocation and initialization */
/*            Device_p->HighPriorityQueueBuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,*/
/*                                                          sizeof(stblit_Msg_t)*STBLIT_MAX_NUMBER_MESSAGE);*/

            Device_p->HighPriorityQueue= STOS_MessageQueueCreate(sizeof(stblit_Msg_t),STBLIT_MAX_NUMBER_MESSAGE);

            if (Device_p->HighPriorityQueue == NULL)
			{
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "High Priority Queue allocation in CPU memory failed 1"));
				return(ST_ERROR_NO_MEMORY);
			}
    #endif

    #ifdef ST_OS20 /* ST_OS20*/
		/* Low Priority Message queue allocation and initialization */
        Device_p->LowPriorityQueueBuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,
													 MESSAGE_MEMSIZE_QUEUE(sizeof(stblit_Msg_t),
																		   STBLIT_MAX_NUMBER_MESSAGE));
		if (Device_p->LowPriorityQueueBuffer_p == NULL)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Low Priority Queue allocation in CPU memory failed"));
			return(ST_ERROR_NO_MEMORY);
		}
        message_init_queue(Device_p->LowPriorityQueue,
						   (void*)Device_p->LowPriorityQueueBuffer_p,
						   sizeof(stblit_Msg_t),
						   STBLIT_MAX_NUMBER_MESSAGE);

		/* High Priority Message queue allocation and initialization */
        Device_p->HighPriorityQueueBuffer_p = (void*) STOS_MemoryAllocate(Device_p->CPUPartition_p,
													  MESSAGE_MEMSIZE_QUEUE(sizeof(stblit_Msg_t),
																			STBLIT_MAX_NUMBER_MESSAGE));
		if (Device_p->HighPriorityQueueBuffer_p == NULL)
		{
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "High Priority Queue allocation in CPU memory failed"));
			return(ST_ERROR_NO_MEMORY);
		}

        message_init_queue_timeout(Device_p->HighPriorityQueue,
						   (void*)Device_p->HighPriorityQueueBuffer_p,
						   sizeof(stblit_Msg_t),
						   STBLIT_MAX_NUMBER_MESSAGE);

    #endif
    /* Task initialization */
    Device_p->TaskTerminate = FALSE;

    #ifdef STBLIT_LINUX_FULL_USER_VERSION
			/* NOTE : (void*(*)(void*)) is a temporary solution to remove warnings, it will be reviewed */
			if (pthread_create(&(Device_p->MasterTask),
                            NULL,
                            (void*(*)(void*))stblit_MasterProcess,
                            (void *)Device_p) == 0)
            {
                printf("BLITTER: created task: %s.\n", "STBLIT_ProcessHighPriorityQueue");
            }

			/* NOTE : (void*(*)(void*)) is a temporary solution to remove warnings, it will be reviewed */
			if (pthread_create(&(Device_p->SlaveTask),
                            NULL,
                            (void*(*)(void*))stblit_SlaveProcess,
                            (void *)Device_p) == 0)
            {
                printf("BLITTER: created task: %s.\n", "STBLIT_ProcessLowPriorityQueue");
            }

			/* NOTE : (void*(*)(void*)) is a temporary solution to remove warnings, it will be reviewed */
			if (pthread_create(&(Device_p->InterruptProcessTask),
                            NULL,
                            (void*(*)(void*))stblit_InterruptProcess,
                            (void *)Device_p) == 0)
            {
                printf("BLITTER: created task: %s.\n", "STBLIT_InterruptProcess");
            }

    #else

        if (STOS_TaskCreate ((void(*)(void*))stblit_MasterProcess,
                            (void*)Device_p,
                            Device_p->CPUPartition_p,
                            STBLIT_MASTER_TASK_STACK_SIZE,
                            &(Device_p->MasterTaskStack),
                            Device_p->CPUPartition_p,
                            &(Device_p->MasterTask_p),
                            &(Device_p->MasterTaskDesc),
                            STBLIT_MASTER_TASK_STACK_PRIORITY,
                            BLITMASTER_NAME,
                            0 ) != ST_NO_ERROR)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Create the slave task */
        if (STOS_TaskCreate ((void(*)(void*))stblit_SlaveProcess,
                            (void*)Device_p,
                            Device_p->CPUPartition_p,
                            STBLIT_SLAVE_TASK_STACK_SIZE,
                            &(Device_p->SlaveTaskStack),
                            Device_p->CPUPartition_p,
                            &(Device_p->SlaveTask_p),
                            &(Device_p->SlaveTaskDesc),
                            STBLIT_SLAVE_TASK_STACK_PRIORITY,
                            BLITSLAVE_NAME,
                            0 ) != ST_NO_ERROR)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Create the InterruptProcess task */
        if (STOS_TaskCreate ((void(*)(void*))stblit_InterruptProcess,
                            (void*)Device_p,
                            Device_p->CPUPartition_p,
                            STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE,
                            &(Device_p->InterruptProcessTaskStack),
                            Device_p->CPUPartition_p,
                            &(Device_p->InterruptProcessTask_p),
                            &(Device_p->InterruptProcessTaskDesc),
                            STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY,
                            BLITINTERRUPT_NAME,
                            0 ) != ST_NO_ERROR)
        {
            return(ST_ERROR_BAD_PARAMETER);
        }
    #endif
    /* Events open /subscribe / registrater  */
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    strcpy(Device_p->EventHandlerName, InitParams_p->EventHandlerName);
    strcpy(Device_p->BlitInterruptEventName,InitParams_p->BlitInterruptEventName);

    Device_p->BlitInterruptEvent    = InitParams_p->BlitInterruptEvent;
    Device_p->BlitInterruptNumber   = InitParams_p->BlitInterruptNumber;
    Device_p->BlitInterruptLevel    = InitParams_p->BlitInterruptLevel;
#endif

    if (SetupEventsAndIT(Device_p,DeviceName) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef STBLIT_EMULATOR
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE))
    {
        stblit_EmulatorInit(Device_p);
    }
#endif

    /* Initialize Packet size control register :
     * By default PKT_SIZE = 0 and Little Endian
     * */
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7020) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_5528) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7710) ||
       (Device_p->DeviceType == STBLIT_DEVICE_TYPE_GAMMA_7100))
    {
        if (InitParams_p->BigNotLittle == TRUE)
        {
            STSYS_WriteRegDev32LE((void*)((U32)Device_p->BaseAddress_p + BLT_PKZ_OFFSET), 1 << BLT_PKZ_ENDIANNESS_SHIFT);
        }
    }

    return(Err);
}

/*******************************************************************************
Name        : Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t Open(stblit_Unit_t * const Unit_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    Unit_p->FirstCreatedJob   = STBLIT_NO_JOB_HANDLE;
    Unit_p->LastCreatedJob    = STBLIT_NO_JOB_HANDLE;
    Unit_p->NbCreatedJobs     = 0;

    return(Err);
}

/*******************************************************************************
Name        : Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t Close(stblit_Unit_t * const Unit_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Delete created jobs and its associated nodes */
    if (Unit_p->NbCreatedJobs != 0)
    {
        /*  Unit_p->FirstCreatedJob != STBLIT_NO_JOB_HANDLE */
        stblit_DeleteAllJobFromList(Unit_p,Unit_p->FirstCreatedJob);
    }

    Unit_p->FirstCreatedJob   = STBLIT_NO_JOB_HANDLE;
    Unit_p->LastCreatedJob    = STBLIT_NO_JOB_HANDLE;
    Unit_p->NbCreatedJobs     = 0;

    return(Err);
}

/*******************************************************************************
Name        : Term
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
static ST_ErrorCode_t Term(stblit_Device_t * const Device_p)
{
    ST_ErrorCode_t              Err = ST_NO_ERROR;
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    MappingArea_t               MappingArea;
	int                         ret;
#else
    task_t*                     MasterTask_p = Device_p->MasterTask_p;
    task_t*                     SlaveTask_p = Device_p->SlaveTask_p;
    task_t*                     InterruptProcessTask_p = Device_p->InterruptProcessTask_p;

    STAVMEM_FreeBlockParams_t   FreeParams;
#endif
    stblit_Msg_t*               Msg_p;

#ifndef STBLIT_LINUX_FULL_USER_VERSION
    /* Event Close */
    if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Evt Close failed"));
    }
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION

    Err = STBLIT_UserTaskTerm( Device_p->InterruptEventTask,
                    NULL );
    if (Err != ST_NO_ERROR)
	{
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :stblit_InterruptEvent_Task\n" ));
		return(ST_ERROR_BAD_PARAMETER);
	}

     Err =   STBLIT_UserTaskTerm( Device_p->InterruptHandlerTask,
                    NULL );
    if (Err != ST_NO_ERROR)
	{
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :stblit_InterruptHandlerTask\n" ));
		return(ST_ERROR_BAD_PARAMETER);
	}


#else
    STOS_InterruptUninstall(Device_p->BlitInterruptNumber, Device_p->BlitInterruptLevel, (void *)Device_p);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

    /* Task deletion process
     * TaskTerminate has to be set to FALSE before the TERMINATE Message to avoid the system to hang! */
    Device_p->TaskTerminate = TRUE;
    /* Send message to go out from Slave message stuff */
    Msg_p = (stblit_Msg_t*) STOS_MessageQueueClaim(Device_p->LowPriorityQueue);
    Msg_p->MessageID = TERMINATE;
    STOS_MessageQueueSend(Device_p->LowPriorityQueue, (void*)Msg_p);

    /* Signal any semaphore pending to go out of the wait.
     * (waits for InsertionDoneSemaphore, LockInstalledSemaphore and UnlockInstalledSemaphore). Only to do so
     * if emulation or HW is in place*/

    /* Wait for Slave to be terminated before terminating the Master :
     * If not it could happen that
     * the Slave need a BackEndQueueControlSemaphore token (its queue is not empty)
     * and there isn't any more Master to give one to it ! */
    STOS_SemaphoreWait(Device_p->SlaveTerminatedSemaphore);

    /* Send message to go out from Master message stuff */
    Msg_p = (stblit_Msg_t*) STOS_MessageQueueClaimTimeout(Device_p->HighPriorityQueue,TIMEOUT_INFINITY);
    Msg_p->MessageID = TERMINATE;
    STOS_MessageQueueSend(Device_p->HighPriorityQueue, (void*)Msg_p);

    #if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
        STOS_SemaphoreWait(Device_p->MasterTerminatedSemaphore);
    #endif
    /* Signal any semaphore pending to go out of the waits (TBD) */
    STOS_SemaphoreSignal(Device_p->InterruptProcessSemaphore);

    /* delete tasks */
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if !defined(ST_GX1) && !defined(ST_OSLINUX)  /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
    STOS_TaskWait( &MasterTask_p, TIMEOUT_INFINITY );
#endif
    Err = STOS_TaskDelete ( MasterTask_p,
        Device_p->CPUPartition_p,
        Device_p->MasterTaskStack,
        Device_p->CPUPartition_p );
    if(Err != ST_NO_ERROR)
    {
#if defined(DVD_STAPI_DEBUG)
        STTBX_Print(("Error in call to STOS_TaskDelete(): %s %i\n\r",__FILE__, __LINE__));
#endif /*DVD_STAPI_DEBUG*/
        return(Err);
    }
#endif
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if !defined(ST_GX1) && !defined(ST_OSLINUX)  /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
    STOS_TaskWait( &SlaveTask_p, TIMEOUT_INFINITY );
#endif
    Err = STOS_TaskDelete ( SlaveTask_p,
        Device_p->CPUPartition_p,
        Device_p->SlaveTaskStack,
        Device_p->CPUPartition_p );
    if(Err != ST_NO_ERROR)
    {
#if defined(DVD_STAPI_DEBUG)
        STTBX_Print(("Error in call to STOS_TaskDelete(): %s %i\n\r",__FILE__, __LINE__));
#endif /*DVD_STAPI_DEBUG*/
        return(Err);
    }
#endif
#if !defined(STBLIT_LINUX_FULL_USER_VERSION)
#if !defined(ST_GX1) && !defined(ST_OSLINUX) /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
    STOS_TaskWait( &InterruptProcessTask_p, TIMEOUT_INFINITY );
#endif
    Err = STOS_TaskDelete ( InterruptProcessTask_p,
        Device_p->CPUPartition_p,
        Device_p->InterruptProcessTaskStack,
        Device_p->CPUPartition_p );
    if(Err != ST_NO_ERROR)
    {
#if defined(DVD_STAPI_DEBUG)
        STTBX_Print(("Error in call to STOS_TaskDelete(): %s %i\n\r",__FILE__, __LINE__));
#endif /*DVD_STAPI_DEBUG*/
        return(Err);
    }
#endif
#ifdef STBLIT_EMULATOR
    if ((Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION) ||
        (Device_p->DeviceType == STBLIT_DEVICE_TYPE_SOFTWARE))
    {
        stblit_EmulatorTerm(Device_p);
    }
#endif

    /* Message queues deletion and deallocation (Need to make sure no task is waiting for a message (claim/receive) !!)*/
    STOS_MessageQueueDelete(Device_p->LowPriorityQueue);
    STOS_MessageQueueDelete(Device_p->HighPriorityQueue);

    #ifdef ST_OS20
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->LowPriorityQueueBuffer_p);
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->HighPriorityQueueBuffer_p);
    #endif

    /* Params for shared memory deallocation */

#ifndef STBLIT_LINUX_FULL_USER_VERSION
    /* Params for shared memory deallocation */
    FreeParams.PartitionHandle = Device_p->AVMEMPartition;
#endif

    /* Single Blit Node Data Pool deallocation  */
    if (Device_p->SBlitNodeNumber != 0)
    {
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->SBlitNodeDataPool.HandleArray_p);
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( Device_p->SingleBlitNodeBuffer_p != NULL)  /* buffer has been allocated by driver */
        {
            Err = STLAYER_FreeData( LayerHndl, Device_p->SingleBlitNodeBuffer_p );
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
            }
        }
#else
        if ( Device_p->SBlitNodeBufferHandle != STAVMEM_INVALID_BLOCK_HANDLE)  /* buffer has been allocated by driver */
        {
            Err = STAVMEM_FreeBlock(&FreeParams,&(Device_p->SBlitNodeBufferHandle));
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
                /* return (); */
            }
        }
#endif
    }


    /* Job Blit Node Data Pool deallocation  */
    if (Device_p->JBlitNodeNumber != 0)
    {
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->JBlitNodeDataPool.HandleArray_p);
#ifdef STBLIT_LINUX_FULL_USER_VERSION
        if ( Device_p->JobBlitNodeBuffer_p != NULL)  /* buffer has been allocated by driver */
        {
            Err = STLAYER_FreeData( LayerHndl, Device_p->JobBlitNodeBuffer_p );
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
            }
        }
#else
        if ( Device_p->JBlitNodeBufferHandle != STAVMEM_INVALID_BLOCK_HANDLE)  /* buffer has been allocated by driver */
        {
            Err = STAVMEM_FreeBlock(&FreeParams,&(Device_p->JBlitNodeBufferHandle));
            if (Err != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
                /* return (); */
            }
        }
#endif
    }

    /* Job Descriptor Data Pool deallocation  */
    if (Device_p->JobNumber != 0)
    {
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->JobDataPool.HandleArray_p);
        if (Device_p->JobBufferUserAllocated != TRUE)   /* buffer has been allocated by driver */
        {
            STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->JobDataPool.ElemArray_p);
        }
    }


    /* Job Blit Data Pool deallocation  */
    if (Device_p->JBlitNumber != 0)
    {
        STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->JBlitDataPool.HandleArray_p);
        if (Device_p->JBlitBufferUserAllocated != TRUE)   /* buffer has been allocated by driver */
        {
            STOS_MemoryDeallocate(Device_p->CPUPartition_p, Device_p->JBlitDataPool.ElemArray_p);
        }
    }

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    /* Work buffer deallocation */
	if ( Device_p->WorkBuffer_p != NULL)  /* buffer has been allocated by driver */
	{
        Err = STLAYER_FreeData( LayerHndl, Device_p->WorkBuffer_p );
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
		}
	}
#else
#ifdef ST_OSWINCE
    // unmap the address range
    if (Device_p->BaseAddress_p != NULL)
        UnmapPhysicalToVirtual(Device_p->BaseAddress_p);
#endif /* ST_OSWINCE*/

    /* Work buffer deallocation */
    if ( Device_p->WorkBufferHandle != STAVMEM_INVALID_BLOCK_HANDLE)  /* buffer has been allocated by driver */
    {
        Err = STAVMEM_FreeBlock(&FreeParams,&(Device_p->WorkBufferHandle));
        if (Err != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error Free Shared memory : %d\n",Err));
            /* return (); */
        }
    }
#endif

#if defined(ST_OSLINUX) && !defined(STBLIT_LINUX_FULL_USER_VERSION)
    STOS_UnmapRegisters( (void*)Device_p->BlitMappedBaseAddress_p, (U32)Device_p->BlitMappedWidth);
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if ((ret=pthread_join(Device_p->MasterTask, NULL)) != 0)
    {
        printf("BLITTER: %s task join problem (%d) !!!!\n", "MasterTask", ret);
    }
    else
    {
        printf("BLITTER: %s task killed.\n", "MasterTask" );
    }


    if ((ret=pthread_join(Device_p->SlaveTask, NULL)) != 0)
    {
        printf("BLITTER: %s task join problem (%d) !!!!\n", "SlaveTask", ret);
    }
    else
    {
        printf("BLITTER: %s task killed.\n", "SlaveTask");
    }


    if ((ret=pthread_join(Device_p->InterruptProcessTask, NULL)) != 0)
    {
        printf("BLITTER: %s task join problem (%d) !!!!\n", "InterruptProcessTask", ret);
    }
    else
    {
        printf("BLITTER: %s task killed.\n", "InterruptProcessTask");
    }

    MappingArea.StartAddress = (unsigned long)Device_p->BlitMappedBaseAddress_p;
    MappingArea.MappingIndex = 0;
    MappingArea.MappingNumber_p = &BlitterMappingNumber;
    MappingArea.MappedAddress_pp = &BlitterVirtualAddress;

    Err = ModuleUnmapRegisters(fileno(Device_p->BLIT_DevFile), &MappingArea);

    /* unnistall blitter irq in kernel */
   ioctl( fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_UNINSTALL_INT);
#endif

    return(Err);
}

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
    } while ((WantedIndex == INVALID_DEVICE_INDEX) && (Index < STBLIT_MAX_DEVICE));

    return(WantedIndex);
} /* End of GetIndexOfDeviceNamed() function */

 /*
--------------------------------------------------------------------------------
Get Init Allocation parameters of blit API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetInitAllocParams( STBLIT_AllocParams_t* Params_p)
{
    ST_ErrorCode_t Err = ST_NO_ERROR;

    if (Params_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }


    Params_p->SingleBlitNodeSize            = sizeof(stblit_Node_t);
    Params_p->SingleBlitNodeInSharedMemory  = TRUE;
    Params_p->SingleBlitNodeAlignment       = 128;                   /* 128 bit word */

    Params_p->JobBlitNodeSize               = sizeof(stblit_Node_t);
    Params_p->JobBlitNodeInSharedMemory     = TRUE;
    Params_p->JobBlitNodeAlignment          = 128;                   /* 128 bit word */

    Params_p->JobBlitSize                   = sizeof(stblit_JobBlit_t);
    Params_p->JobBlitInSharedMemory         = FALSE;
    Params_p->JobBlitAlignment              = 8;                     /* 8 bit */

    Params_p->JobSize                       = sizeof(stblit_Job_t);
    Params_p->JobInSharedMemory             = FALSE;
    Params_p->JobAlignment                  = 8;                     /* 8 bit */

    Params_p->MinWorkBufferSize             = STBLIT_DEFAULT_FILTER_BUFFER_SIZE + STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE;
                                              /* size to store the 5-TAP filter coefficients + Alpha4 to Alpha8 conversion palette */
    Params_p->WorkBufferInSharedMemory      = TRUE;
    Params_p->WorkBufferAlignment           = 128;                     /* 128 bit */

    return(Err);
}

/*
--------------------------------------------------------------------------------
Get revision of STBLIT API
--------------------------------------------------------------------------------
*/
ST_Revision_t STBLIT_GetRevision(void)
{
    /* Revision string format: STXXX-REL_x.x.x
                                 /       \ \ \__patch release number  }
                            API name      \ \__minor release number  } revision number
                                           \__major release number  }
       Example: STAVMEM-REL_1.0.0 */

    return(BLIT_Revision);
}


/*
--------------------------------------------------------------------------------
Get capabilities of STBLIT API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_GetCapability(const ST_DeviceName_t DeviceName, STBLIT_Capability_t * const Capability_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((Capability_p == NULL) ||                       /* Pointer to capability structure should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((DeviceName[0]) == '\0')
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    EnterCriticalSection();

    /* Check if device already initialised and return error if not so */
    DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
    if (DeviceIndex == INVALID_DEVICE_INDEX)
    {
        /* Device name not found */
        Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        /* Fill capability structure */

        /* Eventually not supported */
        Err = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    LeaveCriticalSection();

    return(Err);
}



/*
--------------------------------------------------------------------------------
Initialise blit API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Init(const ST_DeviceName_t DeviceName, const STBLIT_InitParams_t * const InitParams_p)
{
    S32 Index = 0;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if (InitParams_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    if ((InitParams_p->MaxHandles > STBLIT_MAX_OPEN)  ||     /* No more than MAX_OPEN open handles supported */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) ||     /* Device name length should be respected */
        ((DeviceName[0]) == '\0')                 /* Device name should not be empty */
       )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (InitParams_p->CPUPartition_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ((InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_SOFTWARE_EMULATION) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_SOFTWARE) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_GAMMA_7015) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_GAMMA_7020) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_GAMMA_5528) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_GAMMA_7710) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_GAMMA_7100) &&
        (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_GAMMA_ST40GX1))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Bad parameter if neither job operations nor single operations are possible  */
    if ((InitParams_p->SingleBlitNodeMaxNumber == 0 ) && ((InitParams_p->JobMaxNumber == 0) ||
                                                          (InitParams_p->JobBlitMaxNumber == 0) ||
                                                          (InitParams_p->JobBlitNodeMaxNumber == 0)))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* When not all descriptors associated with jobs are present, job can not be used. Either they are all 0 and Ok it is clear that
     * there is no job, or they all are different from 0 and job can be used !*/
    if (((InitParams_p->JobBlitNodeMaxNumber != 0) && ((InitParams_p->JobBlitMaxNumber == 0) || (InitParams_p->JobMaxNumber == 0))) ||
        ((InitParams_p->JobMaxNumber != 0) && ((InitParams_p->JobBlitMaxNumber == 0) || (InitParams_p->JobBlitNodeMaxNumber == 0))) ||
        ((InitParams_p->JobBlitMaxNumber != 0) && ((InitParams_p->JobBlitNodeMaxNumber == 0) || (InitParams_p->JobMaxNumber == 0))))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Make sure the work buffer is large enough for filter coefficients buffer! */
    if (InitParams_p->WorkBufferSize < STBLIT_DEFAULT_FILTER_BUFFER_SIZE)
    {
        return(STBLIT_ERROR_WORK_BUFFER_SIZE);
    }

    EnterCriticalSection();

    /* First time: initialise devices and units as empty */
    if (!FirstInitDone)
    {
        for (Index = 0; Index < STBLIT_MAX_DEVICE; Index++)
        {
            DeviceArray[Index].DeviceName[0] = '\0';
        }

        for (Index = 0; Index < STBLIT_MAX_UNIT; Index++)
        {
            UnitArray[Index].UnitValidity = 0;
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
        /* Look for a non-initialised device and return error if none is available */
        Index = 0;
        while ((DeviceArray[Index].DeviceName[0] != '\0') && (Index < STBLIT_MAX_DEVICE))
        {
            Index++;
        }
        if (Index >= STBLIT_MAX_DEVICE)
        {
            /* All devices initialised */
            Err = ST_ERROR_NO_MEMORY;
        }
        else
        {
            /* API specific initialisations */
            Err = Init(&DeviceArray[Index], InitParams_p,DeviceName);

            if (Err == ST_NO_ERROR)
            {
                /* Device available and successfully initialised: register device name */
                strcpy(DeviceArray[Index].DeviceName, DeviceName);
                DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';
                DeviceArray[Index].MaxHandles = InitParams_p->MaxHandles;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised", DeviceArray[Index].DeviceName));
            }
        } /* End exists device not initialised */
    } /* End device not already initialised */

    LeaveCriticalSection();

    return(Err);
}


/*
--------------------------------------------------------------------------------
open
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Open(const ST_DeviceName_t DeviceName, const STBLIT_OpenParams_t * const OpenParams_p, STBLIT_Handle_t *Handle_p)
{
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex, Index;
    U32 OpenedUnitForThisInit;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((OpenParams_p == NULL) ||                       /* There must be parameters ! */
        (Handle_p == NULL) ||                           /* Pointer to handle should be valid ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((DeviceName[0]) == '\0')
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
        /* Check if device already initialised and return error if not so */
        DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
        if (DeviceIndex == INVALID_DEVICE_INDEX)
        {
            /* Device name not found */
            Err = ST_ERROR_UNKNOWN_DEVICE;
        }
        else
        {
            /* Look for a free unit and check all opened units to check if MaxHandles is not reached */
            UnitIndex = STBLIT_MAX_UNIT;
            OpenedUnitForThisInit = 0;
            for (Index = 0; Index < STBLIT_MAX_UNIT; Index++)
            {
                if ((UnitArray[Index].UnitValidity == STBLIT_VALID_UNIT) && (UnitArray[Index].Device_p == &DeviceArray[DeviceIndex]))
                {
                    OpenedUnitForThisInit ++;
                }
                if (UnitArray[Index].UnitValidity != STBLIT_VALID_UNIT)
                {
                    /* Found a free handle structure */
                    UnitIndex = Index;
                }
            }
            if ((OpenedUnitForThisInit >= DeviceArray[DeviceIndex].MaxHandles) || (UnitIndex >= STBLIT_MAX_UNIT))
            {
                /* None of the units is free or MaxHandles reached */
                Err = ST_ERROR_NO_FREE_HANDLES;
            }
            else
            {
                *Handle_p = (STBLIT_Handle_t) &UnitArray[UnitIndex];
                UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];

                /* API specific actions after opening */
                Err = Open(&UnitArray[UnitIndex]);

                if (Err == ST_NO_ERROR)
                {
                    /* Register opened handle */
                    UnitArray[UnitIndex].UnitValidity = STBLIT_VALID_UNIT;

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle opened on device '%s'", DeviceName));
                }
            } /* End found unit unused */
        } /* End device valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);


}


/*
--------------------------------------------------------------------------------
Close
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Close(STBLIT_Handle_t Handle)
{
    stblit_Unit_t *Unit_p;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    EnterCriticalSection();

    if (!(FirstInitDone))
    {
        Err = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
        if (!IsValidHandle(Handle))
        {
            Err = ST_ERROR_INVALID_HANDLE;
        }
        else
        {
            Unit_p = (stblit_Unit_t *) Handle;

            /* API specific actions before closing */
            Err = Close(Unit_p);

            /* Close only if no errors */
            if (Err == ST_NO_ERROR)
            {
                /* Un-register opened handle */
                Unit_p->UnitValidity = 0;

                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
            }
        } /* End handle valid */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);



 }


/*
--------------------------------------------------------------------------------
Terminate blitter API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STBLIT_Term(const ST_DeviceName_t DeviceName, const STBLIT_TermParams_t *const TermParams_p)
{
    stblit_Unit_t *Unit_p;
    S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex;
    BOOL Found = FALSE;
    ST_ErrorCode_t Err = ST_NO_ERROR;

    /* Exit now if parameters are bad */
    if ((TermParams_p == NULL) ||                       /* There must be parameters ! */
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME - 1) || /* Device name length should be respected */
        ((DeviceName[0]) == '\0')            /* Device name should not be empty */
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
            /* Check if there is still 'open' on this device */
/*            Found = FALSE;*/
            UnitIndex = 0;
            Unit_p = UnitArray;
            while ((UnitIndex < STBLIT_MAX_UNIT) && (!Found))
            {
                Found = ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STBLIT_VALID_UNIT));
                Unit_p++;
                UnitIndex++;
            }

            if (Found)
            {
                if (TermParams_p->ForceTerminate)
                {
                    UnitIndex = 0;
                    Unit_p = UnitArray;
                    while (UnitIndex < STBLIT_MAX_UNIT)
                    {
                        if ((Unit_p->Device_p == &DeviceArray[DeviceIndex]) && (Unit_p->UnitValidity == STBLIT_VALID_UNIT))
                        {
                            /* Found an open instance: close it ! */
                            Err = Close(Unit_p);
                            if (Err == ST_NO_ERROR)
                            {
                                /* Un-register opened handle */
                                Unit_p->UnitValidity = 0;

                                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'", Unit_p->Device_p->DeviceName));
                            }
                            else
                            {
                                /* Problem: this should not happen
                                Fail to terminate ? No because of force */
                                Err = ST_NO_ERROR;
                            }
                        }
                        Unit_p++;
                        UnitIndex++;
                    }
                } /* End ForceTerminate: closed all opened handles */
                else
                {
                    /* Can't term if there are handles still opened, and ForceTerminate not set */
                    Err = ST_ERROR_OPEN_HANDLE;
                }
            } /* End found handle not closed */

            /* Terminate if OK */
            if (Err == ST_NO_ERROR)
            {
                /* API specific terminations */
                Err = Term(&DeviceArray[DeviceIndex]);
                if (Err == ST_NO_ERROR)
                {
                    /* Device found: desallocate memory, free device */
                    DeviceArray[DeviceIndex].DeviceName[0] = '\0';

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated\n", DeviceName));
                }
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */
    LeaveCriticalSection();

    return(Err);
}


/* End of blt_init.c */
