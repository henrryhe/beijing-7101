/*******************************************************************************

File name   : bdisp_init.c

Description : Init module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
01 Jun 2004        Created                                          HE
25 May 2006        Modified for WinCE platfor.                      WinCE Noida):-Abhishek
                   All changes are under ST_OSWINCE flag
05 Sep 2006        Modified for ZEUS platfor.                      AT
                   All changes are under PTV and ST_ZEUS flag (to be resumed next release )
*******************************************************************************/
/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(STBLIT_LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#include <string.h>
#endif

#ifdef STBLIT_LINUX_FULL_USER_VERSION
#ifndef PTV
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif  /* PTV */
#endif

#include "stddefs.h"
#include "blit_rev.h"
#ifdef ST_OSLINUX
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stblit_ioctl.h"
#ifndef ST_ZEUS_PTV_MEM
#include "stlayer.h"
#endif
#endif
#else
#include "sttbx.h"
#endif

#include "stblit.h"
#include "stevt.h"
#ifdef STBLIT_LINUX_FULL_USER_VERSION
#include "stdevice.h"
#ifdef ST_ZEUS_PTV_MEM
#include "stblit_ioctl_ptv.h"
#endif
#else
#include "stavmem.h"
#endif
#include "bdisp_job.h"
#include "bdisp_init.h"
#include "bdisp_pool.h"
#include "bdisp_be.h"
#include "bdisp_flt.h"
#include "bdisp_mem.h"

#ifdef STBLIT_EMULATOR
/*#include "blt_emu.h"*/
#endif

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
void * SecuredBlitMappedBaseAddress_p;

int DevBlitFile = -1 ;


STLAYER_Handle_t LayerHndl = 0x123;  /* Handle for allocating in AVMEM */
#endif


/* Private Types ------------------------------------------------------------ */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
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
static semaphore_t *InstancesAccessControl;   /* Init/Open/Close/Term protection semaphore */
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

/* Private Macros ----------------------------------------------------------- */

/* Passing a (STBLIT_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stblit_Unit_t *) (Handle)) >= UnitArray) &&                    \
                               (((stblit_Unit_t *) (Handle)) < (UnitArray + STBLIT_MAX_UNIT)) &&  \
                               (((stblit_Unit_t *) (Handle))->UnitValidity == STBLIT_VALID_UNIT))

/* Private Function prototypes ---------------------------------------------- */

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

#ifdef PTV
/* Queue into which event will be posted on thread exit */
Pk_Queue * ThreadExitNotifyQ;

ST_ErrorCode_t STBLIT_UserTaskCreate (void * UserTask, void *TaskID, const char *name, U32 priority, void *param);
ST_ErrorCode_t STBLIT_UserTaskTerm (task_t TaskID);
#endif /* !PTV */

STBLITMod_StatusBuffer_t *gpStatusBuffer;

static void* stblit_InterruptEvent_Task(void* data_p);
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

#if !defined(ST_OSLINUX)
    STOS_TaskLock();
#endif
#if defined(STBLIT_LINUX_FULL_USER_VERSION)
    STOS_InterruptLock();
#endif
    if (!InstancesAccessControlInitialized)
    {
        InstancesAccessControlInitialized = TRUE;
        /* Initialise the Init/Open/Close/Term protection semaphore
          Caution: this semaphore is never deleted. (Not an issue) */
        InstancesAccessControl = STOS_SemaphoreCreateFifo(NULL, 1 );
    }

#if !defined(ST_OSLINUX)
    STOS_TaskUnlock();
#endif
#if defined(STBLIT_LINUX_FULL_USER_VERSION)
    STOS_InterruptUnlock();
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
Description : Open  event handler, register and subscribe events.
Parameters  :
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
static ST_ErrorCode_t SetupEventsAndIT(stblit_Device_t* Device_p,const ST_DeviceName_t DeviceName)
{
    STEVT_OpenParams_t              OpenParams;
    ST_ErrorCode_t                  Err;
    char TmpStr[10];

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    STLAYER_AllocDataParams_t StatusAllocParams;
	U32                       TempAddress;
#endif

    /* initialize the DeviceName */
    strcpy(TmpStr,"BLITTER");

    /* Open Enent handler */
    Err = STEVT_Open(Device_p->EventHandlerName, &OpenParams, &(Device_p->EvtHandle));


    if ( Err == ST_NO_ERROR )
    {
        STOS_InterruptLock();

    #ifdef STBLIT_LINUX_FULL_USER_VERSION
        StatusAllocParams.Size = sizeof(STBLITMod_StatusBuffer_t);
        StatusAllocParams.Alignment = 16;

		/* Added to rmove compilation warnning (dereferencing type-punned pointer will break strict-aliasing rules) */
		TempAddress = (U32)&gpStatusBuffer;

		STLAYER_AllocData( LayerHndl, &StatusAllocParams, (void**) TempAddress );
        gpStatusBuffer->NextRead = gpStatusBuffer->NextWrite = 1;


        STTBX_Report ((STTBX_REPORT_LEVEL_INFO, "SetupEventsAndIT(): cpu %x device %x size %d\n",
                       gpStatusBuffer, stblit_CpuToDevice((void *)gpStatusBuffer), StatusAllocParams.Size));

/* install interrupt */
#ifdef PTV
        if ( Err = ioctl( Device_p->BLIT_DevNum, STBLIT_IOCTL_INSTALL_INT, stblit_CpuToDevice((void*)gpStatusBuffer)) != ST_NO_ERROR )
#else
        Err = ioctl( fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_INSTALL_INT, stblit_CpuToDevice((void*)gpStatusBuffer));
        if( Err != ST_NO_ERROR )
#endif
		{
			return (ST_ERROR_INTERRUPT_INSTALL);
		}


     #ifndef PTV
  	     /* task used to signal interrupt process  */

         /* Create InterruptEvent task used to notify user when interrupt from blitter engine is generated*/

        if (pthread_create( &(Device_p->InterruptEventTask), NULL, stblit_InterruptEvent_Task, (void *)Device_p))
        {
            Err = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            STTBX_Report (( STTBX_REPORT_LEVEL_INFO,"pthread_crate(): task created\n"));
        }


        #ifdef STBLIT_HW_BLITTER_RESET_DEBUG
        if (pthread_crate( &(Device_p->ErrorCheckProcessTask), NULL, stblit_BlitterErrorCheckProcess, (void *)Device_p))
        {
            Err = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            STTBX_Report (( STTBX_REPORT_LEVEL_INFO,"pthread_crate(): task created\n"));
        }

        #endif

     #else /* !PTV */
         /* Starting thread with priority 4 */
         Err = STBLIT_UserTaskCreate( (stblit_InterruptEvent_Task,
                                      (void *)(&(Device_p->InterruptEventTask)),
                                      "STBLIT IntEvntTst", 4, (void*) Device_p);

        #ifdef STBLIT_HW_BLITTER_RESET_DEBUG
            Err = STBLIT_UserTaskCreate( (void*) stblit_BlitterErrorCheckProcess,
                                        (void *)&(Device_p->ErrorCheckProcessTask),
                                        "STBLIT CrashcheckTst", 8, (void*) Device_p);
        #endif


     #endif /* !PTV */
         if (Err != ST_NO_ERROR)
		 {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error create task :stblit_InterruptEvent_Task\n" ));
		    return(ST_ERROR_BAD_PARAMETER);
		 }

    #endif

#ifndef STBLIT_LINUX_FULL_USER_VERSION
        /* install interrupt */
        Err = STOS_InterruptInstall( Device_p->BlitInterruptNumber,
                    Device_p->BlitInterruptLevel,
                    stblit_BlitterInterruptHandler,
                    TmpStr,
                    (void *) Device_p);
        if ( Err )
        {
            Err = ST_ERROR_INTERRUPT_INSTALL;
        }


#ifdef BLIT_DEBUG
        STTBX_Print(("Blit_init : Err: %d,  nb interrupt : %x, \n",Err, Device_p->BlitInterruptNumber ));
#endif


#endif

        STOS_InterruptUnlock();

        if (Err == ST_NO_ERROR)
        {
#ifdef ST_OSLINUX
           if (FALSE)   /* May be enable_irq with probe_irq_on/probe_irq_off ??? To be checked ... */
#else
           if (STOS_InterruptEnable(Device_p->BlitInterruptNumber, Device_p->BlitInterruptLevel) != 0)
#endif
           {
               Err = ST_ERROR_INTERRUPT_INSTALL;
           }
       }
    }

    /* Register different API events */
     if (Err == ST_NO_ERROR)
    {

        /* Declare this event as to be exported */
        Err = STEVT_RegisterDeviceEvent(Device_p->EvtHandle, DeviceName,
                                        STBLIT_BLIT_COMPLETED_EVT, &(Device_p->EventID[STBLIT_BLIT_COMPLETED_ID]));
    }

    if (Err == ST_NO_ERROR)
    {
        /* Declare this event as to be exported */
        Err = STEVT_RegisterDeviceEvent(Device_p->EvtHandle,DeviceName,
                                        STBLIT_JOB_COMPLETED_EVT, &(Device_p->EventID[STBLIT_JOB_COMPLETED_ID]));
    }

    if (Err != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error %X while Opening EventHandler %s or registering events",
                 Err, Device_p->EventHandlerName));
        Err = ST_ERROR_BAD_PARAMETER;
    }


    return(Err);

}


#ifdef STBLIT_LINUX_FULL_USER_VERSION

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

    UNUSED_PARAMETER(Address);
    /* Determinating length of mapping */
    /* Should be mapped according to LinuxPageSize (=0x1000)*/
    BlitModuleParam.BlitBaseAddress = MappingArea_p->StartAddress;
    BlitModuleParam.BlitAddressWidth = 0x1000;    /* CD: to be modified ... */
    BlitModuleParam.BlitMappingIndex = MappingArea_p->MappingIndex;

    /* Giving parameters to the module. Open is done in API. To be checked */
    ioctl(FileNo, STBLIT_IOCTL_ADDRESS_PARAM, &BlitModuleParam);

#ifdef DEBUG
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO, "shared virtual address=%x\n", *(MappingArea_p->MappedAddress_pp) ));
#endif
    /* mapping */
#ifndef PTV
    if ((ErrorCode = ioctl(FileNo, STBLIT_IOCTL_MAP)) == ST_NO_ERROR)
#else /* !PTV */
    if ((ErrorCode = ioctl(FileNo, STBLIT_IOCTL_MAP, NULL)) == ST_NO_ERROR)
#endif /* !PTV */
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
        }
        else
        {
            *(MappingArea_p->MappedAddress_pp) = *VirtualAdress_pp;
            (*(MappingArea_p->MappingNumber_p))++;
#ifdef BLIT_DEBUG
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLIT: Mapping successfull: %x from %x to %x\n",
                                            *VirtualAdress_pp,
                                            BlitModuleParam.BlitBaseAddress,
                                            BlitModuleParam.BlitBaseAddress + BlitModuleParam.BlitAddressWidth));
#endif
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
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO, "BLITTER: Unmapping: BaseAddress=%x\n", *(MappingArea_p->MappedAddress_pp)));
        munmap(*(MappingArea_p->MappedAddress_pp), BlitModuleParam.BlitAddressWidth);

        /* clearing share allocation pointer */
        *(MappingArea_p->MappingNumber_p) = 0;
        *(MappingArea_p->MappedAddress_pp) = NULL;
    }
    else
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_INFO, "BLITTER: Unmapping: Shared base region still mapped for other drivers\n"));
    }

    return (ErrorCode);
}

/* task that read from kernel */
void* stblit_InterruptEvent_Task(void* data_p)
{
    stblit_Device_t*    Device_p;

    int     EventCount ;
    char    DataArrived[4];
    BOOL    TaskTerminate = FALSE;


#if !defined(MODULE) && defined(PTV)
    Device_p = (stblit_Device_t*) (((PK_ThreadData *)data_p)->param);
#else
    Device_p =  (stblit_Device_t*)data_p;
#endif /* !MODULE && PTV */

#ifdef BLIT_DEBUG
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "stblit_interruptevent_Task():Starting event interrupt task...\n"));
#endif

	while(Device_p->TaskTerminate == FALSE)
	{
#if !defined(MODULE) && defined(PTV)
        EventCount = read(Device_p->BLIT_DevNum, DataArrived, 1);

        if ( EventCount  > 0)
        {
            //stblit_InterruptHandlerTask(data_p);
           stblit_InterruptHandlerTask(data_p);
        }  /* EventCount > 0 */
#else
        EventCount = read(fileno(Device_p->BLIT_DevFile), DataArrived,  1);

        if ( EventCount  > 0)
        {
            stblit_InterruptHandlerTask(data_p);
        }                               /* EventCount > 0 */
#endif /* !MODULE && PTV */

        /* To remove compilation warnning (function might be possible candidate for attribute bnoreturnb)*/
		if (TaskTerminate)
		{
			break;
		}
    }
                                      /* while 1 */
 return(NULL);
}

#ifndef PTV
/* Function to create InterruptEvent task used to notify user when interrupt from blitter engine is generated*/
ST_ErrorCode_t STBLIT_UserTaskTerm( pthread_t TaskID, pthread_attr_t *Task_Attribute )
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;

    UNUSED_PARAMETER(Task_Attribute);
    if ( TaskID )
    {
        if (pthread_cancel(TaskID))
        {
            STTBX_Report (( STTBX_REPORT_LEVEL_INFO,"STBLIT_UserTaskTerm(): unable to kill task\n"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
    }
    return(ErrorCode);
}
#else /* !PTV */

ST_ErrorCode_t STBLIT_UserTaskCreate (void * UserTask, void *TaskID, const char *name, U32 priority, void *param)
{
  ST_ErrorCode_t   ErrorCode = ST_NO_ERROR;
  Pk_Thread      * thread = NULL;
  PK_ThreadData  * tData_p;
  size_t           stkSize = 6 * 1024;  /* XXX PK: Set stack size to some arbitrary value */


  if (ThreadExitNotifyQ == NULL)
  {
    /* Create a queue where event will be posted on thread exist */
    ThreadExitNotifyQ = pk_NewQueue("ST Thread Queue");
    if (ThreadExitNotifyQ == NULL)
    {
      STTBX_Report ((STTBX_REPORT_LEVEL_INFO,
                     "STBLIT_UserTaskCreate(): unable to create thread exit notify task \n"));
      ErrorCode = ST_ERROR_NO_MEMORY;
    }
  }

  /* Allocate memory for thread data */
  tData_p = (PK_ThreadData *)malloc(sizeof(PK_ThreadData));
  if (tData_p == NULL)
  {
    STTBX_Report ((STTBX_REPORT_LEVEL_INFO,
                   "STBLIT_UserTaskCreate(): unable to alloc memory for thread data \n"));
    ErrorCode = ST_ERROR_NO_MEMORY;
  }
  else
  {
    tData_p->threadName = NULL;
    tData_p->threadName = (char *)malloc((strlen(name) +1)*sizeof(char));
    if (tData_p->threadName == NULL)
    {
      STTBX_Report ((STTBX_REPORT_LEVEL_INFO,
                   "STBLIT_UserTaskCreate(): unable to alloc memory for thread name \n"));
      ErrorCode = ST_ERROR_NO_MEMORY;
    }
    else
    {
      strncpy (tData_p->threadName, name, strlen(name));
      tData_p->param = param;

      thread = pk_LaunchNotify (UserTask, stkSize, (void *)tData_p, priority, ThreadExitNotifyQ);
      if (thread)
      {
        *((Pk_Thread **)TaskID) = thread;
      }
      else
      {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
      }
    }
  }

  return ErrorCode;
}


ST_ErrorCode_t STBLIT_UserTaskTerm (task_t TaskID)
{
  U8            taskName[32];
  U32           taskNameLen;

/*
  pk_GetThreadName(TaskID, taskName, 32, &taskNameLen);
  printf ("%s: terminting task %s\n", __FUNCTION__, taskName);
*/
  return ST_NO_ERROR;
}

ST_ErrorCode_t STBLIT_ThreadJoin(task_t TaskID)
{
  Pk_Event *evt;
  ST_ErrorCode_t   ErrorCode = ST_NO_ERROR;


  if (ThreadExitNotifyQ != NULL)
  {
    /* Wait on ThreadExitNotifyQ till thread exits */
    evt = pk_NextEvent(ThreadExitNotifyQ, kPtv_Forever);
    if (evt)
      pk_FreeEvent(evt);
    else
      ErrorCode = ST_ERROR_BAD_PARAMETER;
  }

  return ErrorCode;
}
#endif /* !PTV */

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
   STAVMEM_MemoryRange_t      RangeArea[2];
   STAVMEM_AllocBlockParams_t AllocBlockParams;
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
    U32 TmpAddress;

    /* common stuff initialization*/
    Device_p->DeviceType                = InitParams_p->DeviceType;
    Device_p->CPUPartition_p            = InitParams_p->CPUPartition_p;
#ifndef STBLIT_LINUX_FULL_USER_VERSION
#ifdef ST_OSLINUX
#if defined (DVD_SECURED_CHIP)
    PartitionHandle = STAVMEM_GetPartitionHandle( BLIT_PARTITION_AVMEM );
#else
    PartitionHandle = STAVMEM_GetPartitionHandle( BLIT_PARTITION_AVMEM );
#endif
#endif

#ifdef ST_OSLINUX
    Device_p->AVMEMPartition            = PartitionHandle;
#else
    Device_p->AVMEMPartition            = InitParams_p->AVMEMPartition;
#endif

    /* STAVMEM mapping */
#ifdef ST_OSLINUX
    STAVMEM_GetSharedMemoryVirtualMapping2( BLIT_PARTITION_AVMEM, &Device_p->VirtualMapping );
#else
    STAVMEM_GetSharedMemoryVirtualMapping(&Device_p->VirtualMapping);
#endif

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
#ifndef PTV
        if (!(Device_p->BLIT_DevFile = fopen(filename,"w+")))
#else
        if ((Device_p->BLIT_DevNum = open_linux(filename, O_RDWR, 0)) == -1)
#endif
        {
            Err = ST_ERROR_UNKNOWN_DEVICE;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "BLIT MODULE Init Failed ! Cannot Open: %s\n", filename));
        }
        DevBlitFile = fileno(Device_p->BLIT_DevFile) ;
    }

    if (Err == ST_NO_ERROR)
    {
        MappingArea_t  MappingArea;

        MappingArea.StartAddress = BLITTER_BASE_ADDRESS;
        MappingArea.MappingIndex = 0;
        MappingArea.MappingNumber_p = &BlitterMappingNumber;
        MappingArea.MappedAddress_pp = &BlitterVirtualAddress;

#ifndef PTV
        if ( (Err = ModuleMapRegisters( fileno(Device_p->BLIT_DevFile),
#else
        if ( (Err = ModuleMapRegisters( Device_p->BLIT_DevNum,
#endif /* !PTV */
                                        (unsigned long)InitParams_p->BaseAddress_p,
                                        &MappingArea,
                                        (void **)(&Device_p->BlitMappedBaseAddress_p))) == ST_NO_ERROR)
        {
#ifdef BLIT_DEBUG
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLITTER: Init Base@:0x%x\n", Device_p->BaseAddress_p));
#endif
            Device_p->BaseAddress_p = (void *)((U32)(Device_p->BlitMappedBaseAddress_p) + (U32)(InitParams_p->BaseAddress_p) - MappingArea.StartAddress);

#ifdef BLIT_DEBUG
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "BLITTER: Mapped Base@:0x%x\n", Device_p->BaseAddress_p));
#endif
        }
        else
        {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "BLITTER: Register Mapping FAILURE!!!!!\n"));
           return Err;
        }
    }

#endif

    Device_p->AQ_IP_ADDRESS[0] = ((U32)Device_p->BaseAddress_p) + BLT_AQ1_IP;
    Device_p->AQ_IP_ADDRESS[1] = ((U32)Device_p->BaseAddress_p) + BLT_AQ2_IP;
    Device_p->AQ_IP_ADDRESS[2] = ((U32)Device_p->BaseAddress_p) + BLT_AQ3_IP;
    Device_p->AQ_IP_ADDRESS[3] = ((U32)Device_p->BaseAddress_p) + BLT_AQ4_IP;

    Device_p->AQ_LNA_ADDRESS[0] = ((U32)Device_p->BaseAddress_p) + BLT_AQ1_LNA;
    Device_p->AQ_LNA_ADDRESS[1] = ((U32)Device_p->BaseAddress_p) + BLT_AQ2_LNA;
    Device_p->AQ_LNA_ADDRESS[2] = ((U32)Device_p->BaseAddress_p) + BLT_AQ3_LNA;
    Device_p->AQ_LNA_ADDRESS[3] = ((U32)Device_p->BaseAddress_p) + BLT_AQ4_LNA;

    Device_p->AQ_CTL_ADDRESS[0] = ((U32)Device_p->BaseAddress_p) + BLT_AQ1_CTL;
    Device_p->AQ_CTL_ADDRESS[1] = ((U32)Device_p->BaseAddress_p) + BLT_AQ2_CTL;
    Device_p->AQ_CTL_ADDRESS[2] = ((U32)Device_p->BaseAddress_p) + BLT_AQ3_CTL;
    Device_p->AQ_CTL_ADDRESS[3] = ((U32)Device_p->BaseAddress_p) + BLT_AQ4_CTL;

    Device_p->ContinuationNodeTab[0] = NULL;
    Device_p->ContinuationNodeTab[1] = NULL;
    Device_p->ContinuationNodeTab[2] = NULL;
    Device_p->ContinuationNodeTab[3] = NULL;

    Device_p->AQFirstInsertion[0]   = TRUE;
    Device_p->AQFirstInsertion[1]   = TRUE;
    Device_p->AQFirstInsertion[2]   = TRUE;
    Device_p->AQFirstInsertion[3]   = TRUE;
    Device_p->AntiFlutterOn         = FALSE; /*by default the AntiFlutter on target bitmap is disabled */
    Device_p->AntiAliasingOn        = FALSE; /*by default the AntiFlutter on target bitmap is disabled */

#ifdef STBLIT_LINUX_FULL_USER_VERSION
    PhysicSingleBlitNodeBuffer_p = 0 ;
    PhysicJobBlitNodeBuffer_p = 0 ;
#endif


    /* Single Blit node management initialization */
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
            AllocDataParams.Alignment                = 8;  /* 8 bytes = 64 bit */

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
#ifndef STBLIT_SINGLE_BLIT_DCACHE_NODES_ALLOCATION
            AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
            AllocBlockParams.Size                     = Device_p->SBlitNodeNumber * sizeof(stblit_Node_t);
            AllocBlockParams.Alignment                = 8;  /* 8 bytes = 64 bit */
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
    #ifdef ST_OSWINCE
        Elembuffer_p = STAVMEM_VirtualToCPU(Elembuffer_p,&(Device_p->VirtualMapping));
    #else
        Elembuffer_p = STAVMEM_VirtualToCPU((void*)Elembuffer_p,&(Device_p->VirtualMapping));
    #endif
#else
            {
                U32 Alignment = 8;
                Elembuffer_p = (void**) STOS_MemoryAllocate(InitParams_p->CPUPartition_p,
                                                             Device_p->SBlitNodeNumber * sizeof(stblit_Node_t));
                Elembuffer_p = (void *)(((U32)Elembuffer_p + (Alignment)) & (~(Alignment - 1)));

                if (Elembuffer_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Elembuffer_p allocation failed"));
                    return(ST_ERROR_NO_MEMORY);

                }
            }
    }
#endif
#endif /* STBLIT_LINUX_FULL_USER_VERSION */
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
        if (InitParams_p->JobBlitNodeBufferUserAllocated == TRUE)  /* buffer which is user allocated */
        {
            Elembuffer_p  = (void*) InitParams_p->JobBlitNodeBuffer_p;
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
#ifndef STBLIT_JOB_DCACHE_NODES_ALLOCATION
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
    #ifndef ST_OSWINCE
        Elembuffer_p = STAVMEM_VirtualToCPU((void*)Elembuffer_p,&(Device_p->VirtualMapping));
    #else
        Elembuffer_p = STAVMEM_VirtualToCPU(Elembuffer_p,&(Device_p->VirtualMapping));
    #endif /* ST_OSWINCE*/
#else
            {
                U32 Alignment = 8;
                Elembuffer_p = (void**) STOS_MemoryAllocate(InitParams_p->CPUPartition_p,
                                                             Device_p->JBlitNodeNumber * sizeof(stblit_Node_t));

                Elembuffer_p = (void *)(((U32)Elembuffer_p + (Alignment)) & (~(Alignment - 1)));

                if (Elembuffer_p == NULL)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Elembuffer_p allocation failed"));
                    return(ST_ERROR_NO_MEMORY);

                }
            }
    }
#endif
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
     *   + Used as a 8-TAP filter coefficients pool
     *   + Used to store the palette for Alpha4 to Alpha8 conversion */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
    if (InitParams_p->WorkBufferUserAllocated == TRUE)  /* buffer which is user allocated */
    {
        Device_p->WorkBuffer_p     = (void*) InitParams_p->WorkBuffer_p;
	}
	else
	{
        AllocDataParams.Size                     = STBLIT_DEFAULT_FILTER_BUFFER_SIZE + STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE;
#if defined(ST_7109)
        AllocDataParams.Alignment                = 256;  /* 256 bytes = 2048 bit */
#else
        AllocDataParams.Alignment                = 16;  /* 16 bytes = 128 bit */
#endif
        STLAYER_AllocData( LayerHndl, &AllocDataParams, (void**)&(Device_p->WorkBuffer_p));
        PhysicWorkBuffer_p = (U32)stblit_CpuToDevice((void *)Device_p->WorkBuffer_p);
    }
#else
    Device_p->WorkBufferSize  = InitParams_p->WorkBufferSize;
    if (InitParams_p->WorkBufferUserAllocated == TRUE)  /* buffer which is user allocated */
    {
        Device_p->WorkBuffer_p     = (void*) InitParams_p->WorkBuffer_p;
        Device_p->WorkBufferHandle = STAVMEM_INVALID_BLOCK_HANDLE;
    }
    else                                /* Driver allocation in shared memory */
    {
        AllocBlockParams.PartitionHandle          = Device_p->AVMEMPartition;
        AllocBlockParams.Size                     = STBLIT_DEFAULT_FILTER_BUFFER_SIZE + STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE/*Device_p->WorkBufferSize*/;
#if defined(ST_7109)
        AllocBlockParams.Alignment                = 256;  /* 256 bytes = 2048 bit */
#elif defined(ST_7200)
        AllocBlockParams.Alignment                = 512;
#else
        AllocBlockParams.Alignment                = 16;  /* 16 bytes = 128 bit */
#endif
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

    /* Setup Filter buffer We assumed it is well aligned! */
    Device_p->HFilterBuffer_p = (U8*)Device_p->WorkBuffer_p;
    Device_p->VFilterBuffer_p = (U8*)((U32)Device_p->WorkBuffer_p + STBLIT_DEFAULT_HFILTER_BUFFER_SIZE);

    /* Copy coeffcients into filter buffer */

#ifndef PTV
    TmpAddress = (U32)HFilterBuffer;
    memcpy(Device_p->HFilterBuffer_p,(void *)TmpAddress,STBLIT_DEFAULT_HFILTER_BUFFER_SIZE);

    TmpAddress = (U32)VFilterBuffer;
    memcpy(Device_p->VFilterBuffer_p,(void *)TmpAddress,STBLIT_DEFAULT_VFILTER_BUFFER_SIZE);
#else
    {
        /* memcpy replaced to make filter coeffcients Endian safe*/
        U32 Tmp1;
        for ( Tmp1 = 0; Tmp1 <  STBLIT_DEFAULT_HFILTER_BUFFER_SIZE / 4; Tmp1++ )
        {
            STSYS_WriteRegMem32LE((void*)(((U32 *)Device_p->HFilterBuffer_p) + Tmp1),
                                                ((U32)  (((U8)HFilterBuffer[(Tmp1 * 4) +0] << 0)|
                                                        ((U8)HFilterBuffer[(Tmp1 * 4) +1] << 8)|
                                                        ((U8)HFilterBuffer[(Tmp1 * 4) +2] << 16)|
                                                        ((U8)HFilterBuffer[(Tmp1 * 4) +3] << 24))));
        }
        for ( Tmp1 = 0; Tmp1 <  STBLIT_DEFAULT_VFILTER_BUFFER_SIZE / 4; Tmp1++ )
        {
            STSYS_WriteRegMem32LE((void*)(((U32*)Device_p->VFilterBuffer_p) + Tmp1),
                                                ((U32) (((U8)VFilterBuffer[(Tmp1 * 4) +0] << 0)|
                                                        ((U8)VFilterBuffer[(Tmp1 * 4) +1] << 8)|
                                                        ((U8)VFilterBuffer[(Tmp1 * 4) +2] << 16)|
                                                        ((U8)VFilterBuffer[(Tmp1 * 4) +3] << 24))));
        }
    }
#endif

    /* Set Palette pointer to address right after the last filter coefficient in work buffer */
    Device_p->Alpha4To8Palette_p = Device_p->HFilterBuffer_p +  STBLIT_DEFAULT_FILTER_BUFFER_SIZE;

#ifdef ST_OSLINUX
#ifndef PTV
    {
        U32 Tmp;
        for ( Tmp = 0; Tmp <  STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE ; Tmp++ )
        {
            *((U8 *) (((U8 *) Device_p->Alpha4To8Palette_p) + Tmp)) = Alpha4To8Palette[Tmp];
        }
    }
#else
    {
        U32 Tmp;
        for ( Tmp = 0; Tmp <  STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE / 4 ; Tmp++ )
        {
                    STSYS_WriteRegMem32LE((void*)(((U32*)Device_p->Alpha4To8Palette_p) + Tmp),
                                                ((U32) (((U8)Alpha4To8Palette[(Tmp * 4) +0] << 0)|
                                                        ((U8)Alpha4To8Palette[(Tmp * 4) +1] << 8)|
                                                        ((U8)Alpha4To8Palette[(Tmp * 4) +2] << 16)|
                                                        ((U8)Alpha4To8Palette[(Tmp * 4) +3] << 24))));

        }
    }
#endif
#else /* !ST_OSLINUX */
    /* Copy 16 entries Palette for Alpha4 To Alpha8 conversion */
#ifndef ST_OSWINCE
    TmpAddress = (U32)Alpha4To8Palette;
    STAVMEM_CopyBlock1D((void *)TmpAddress,(void*)(STAVMEM_VirtualToCPU(Device_p->Alpha4To8Palette_p,&(Device_p->VirtualMapping))),
                                                  STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE);
#else /* !ST_OSWINCE */
    STAVMEM_CopyBlock1D((void*)Alpha4To8Palette,(void*)(STAVMEM_VirtualToCPU(Device_p->Alpha4To8Palette_p,&(Device_p->VirtualMapping))),
                                                 STBLIT_ALPHA4TO8_CONVERSION_PALETTE_SIZE);
#endif /* ST_OSWINCE */
#endif /* ST_OSLINUX */

    /* Back-end initializations */
    Device_p->HWUnLocked                   = TRUE;
    Device_p->InsertionRequest             = FALSE;
    for(i = 0; i < STBLIT_NUM_AQS; i++)
    {
        Device_p->FirstNodeTab[i]        = NULL;
    }

    Device_p->LastNode_p                   = NULL;
    /*Device_p->ContinuationNode_p                  = NULL;*/

    Device_p->LastNodeInBlitList_p         = NULL;
    Device_p->InsertAtFront                = FALSE;
    Device_p->BlitterIdle                  = TRUE;
    Device_p->LockRequest                  = FALSE;
    Device_p->UnlockRequest                = FALSE;
    Device_p->LockBeforeFirstNode          = FALSE;
#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    Device_p->BlitterErrorCheck_WaitingForInterrupt = FALSE;
    Device_p->BlitterErrorCheck_GotOneInterrupt     = FALSE;
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */
#ifdef STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION
    Device_p->NbSubmission                 = 0;
#endif /* STBLIT_ENABLE_HW_BLITTER_RESET_SIMULATION */


    /* Initialise interrupts queue */
    Device_p->InterruptsBuffer.Data_p        = Device_p->InterruptsData;
    Device_p->InterruptsBuffer.TotalSize     = sizeof(Device_p->InterruptsData) / sizeof(stblit_ItStatus);
    Device_p->InterruptsBuffer.BeginPointer_p = Device_p->InterruptsBuffer.Data_p;
    Device_p->InterruptsBuffer.UsedSize      = 0;
    Device_p->InterruptsBuffer.MaxUsedSize   = 0;

    /* ... TB updated ..*/

    /* Register initialization */

#ifdef ST_7109
    /* configuration of the STBus plugs*/
    {
        unsigned long chz=0x3;
        unsigned long msz=0;
        unsigned long pgz=0x2;

        STSYS_WriteRegMem32LE((void*)0xb920bb00,0x3);
        STSYS_WriteRegMem32LE((void*)0xb920bb04,0x5);
        STSYS_WriteRegMem32LE((void*)0xb920bb08,chz);
        STSYS_WriteRegMem32LE((void*)0xb920bb0c,msz);
        STSYS_WriteRegMem32LE((void*)0xb920bb10,pgz);

        STSYS_WriteRegMem32LE((void*)0xb920bb20,0x3);
        STSYS_WriteRegMem32LE((void*)0xb920bb24,0x5);
        STSYS_WriteRegMem32LE((void*)0xb920bb28,chz);
        STSYS_WriteRegMem32LE((void*)0xb920bb2c,msz);
        STSYS_WriteRegMem32LE((void*)0xb920bb30,pgz);

        STSYS_WriteRegMem32LE((void*)0xb920bb40,0x3);
        STSYS_WriteRegMem32LE((void*)0xb920bb44,0x5);
        STSYS_WriteRegMem32LE((void*)0xb920bb48,chz);
        STSYS_WriteRegMem32LE((void*)0xb920bb4c,msz);
        STSYS_WriteRegMem32LE((void*)0xb920bb50,pgz);

        STSYS_WriteRegMem32LE((void*)0xb920bb60,0x3);
        STSYS_WriteRegMem32LE((void*)0xb920bb64,0x5);
        STSYS_WriteRegMem32LE((void*)0xb920bb68,chz);
        STSYS_WriteRegMem32LE((void*)0xb920bb6c,msz);
        STSYS_WriteRegMem32LE((void*)0xb920bb70,pgz);

        STSYS_WriteRegMem32LE((void*)0xb920bb80,0x3);
        STSYS_WriteRegMem32LE((void*)0xb920bb84,0x5);
        STSYS_WriteRegMem32LE((void*)0xb920bb88,chz);
        STSYS_WriteRegMem32LE((void*)0xb920bb8c,msz);
        STSYS_WriteRegMem32LE((void*)0xb920bb90,pgz);
    }
#endif

    /* semaphore initialization */
    Device_p->HWUnlockedSemaphore       = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->InsertionDoneSemaphore    = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->UnlockInstalledSemaphore  = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->LockInstalledSemaphore    = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->IPSemaphore               = STOS_SemaphoreCreateFifo(NULL,1);
    Device_p->InterruptProcessSemaphore = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->AccessControl             = STOS_SemaphoreCreateFifo(NULL,1);
#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    Device_p->BlitterErrorCheck_AccessControlSemaphore = STOS_SemaphoreCreateFifo(NULL,1);
    Device_p->BlitterErrorCheck_InterruptSemaphore     = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    Device_p->BlitterErrorCheck_SubmissionSemaphore    = STOS_SemaphoreCreateFifo(NULL,0);
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */
    Device_p->AQInsertionSemaphore[0]   = STOS_SemaphoreCreateFifo(NULL,1);
    Device_p->AQInsertionSemaphore[1]   = STOS_SemaphoreCreateFifo(NULL,1);
    Device_p->AQInsertionSemaphore[2]   = STOS_SemaphoreCreateFifo(NULL,1);
    Device_p->AQInsertionSemaphore[3]   = STOS_SemaphoreCreateFifo(NULL,1);
#ifdef STBLIT_DEBUG_GET_STATISTICS
    Device_p->StatisticsAccessControl_p = STOS_SemaphoreCreateFifo(NULL,1);
#endif /* STBLIT_DEBUG_GET_STATISTICS */

    Device_p->AllBlitsCompleted_p            = STOS_SemaphoreCreateFifo(NULL,0);
    Device_p->StartWaitBlitComplete          = FALSE ;

    strcpy(Device_p->EventHandlerName,InitParams_p->EventHandlerName);
#ifndef STBLIT_LINUX_FULL_USER_VERSION
    /* Events open /subscribe / registrater  */

    strcpy(Device_p->BlitInterruptEventName,InitParams_p->BlitInterruptEventName);
    Device_p->BlitInterruptEvent    = InitParams_p->BlitInterruptEvent;
    Device_p->BlitInterruptNumber   = InitParams_p->BlitInterruptNumber;
    Device_p->BlitInterruptLevel    = InitParams_p->BlitInterruptLevel;
#endif
    if (SetupEventsAndIT(Device_p,DeviceName) != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Init task stblit_InterruptProcess */

     /* Task initialization */
    Device_p->TaskTerminate = FALSE;

#ifndef STBLIT_LINUX_FULL_USER_VERSION
            /* Create the InterruptProcess task */
            if (STOS_TaskCreate ((void (*) (void*))stblit_InterruptProcess,
                                (void*)Device_p,
                                Device_p->CPUPartition_p,
                                STBLIT_INTERRUPT_PROCESS_TASK_STACK_SIZE,
                                &(Device_p->InterruptProcessTaskStack),
                                Device_p->CPUPartition_p,
                                &(Device_p->InterruptProcessTask_p),
                                &(Device_p->InterruptProcessTaskDesc),
                                STBLIT_INTERRUPT_PROCESS_TASK_STACK_PRIORITY,
                                BLITINTERRUPT_NAME,
                                0 /*task_flags_no_min_stack_size*/ ) != ST_NO_ERROR)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }

    #ifdef STBLIT_HW_BLITTER_RESET_DEBUG
            /* Create the BlitterErrorCheckProcess task */
            if (STOS_TaskCreate ((void (*) (void*))stblit_BlitterErrorCheckProcess,
                                (void*)Device_p,
                                Device_p->CPUPartition_p,
                                STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_SIZE,
                                &(Device_p->BlitterErrorCheckProcessTaskStack),
                                Device_p->CPUPartition_p,
                                &(Device_p->BlitterErrorCheckProcessTask_p),
                                &(Device_p->BlitterErrorCheckProcessTaskDesc),
                                STBLIT_BLITTER_CRASH_CHECK_PROCESS_TASK_STACK_PRIORITY,
                                BLITTER_CRASH_CHECK_NAME,
                                0 /*task_flags_no_min_stack_size*/ ) != ST_NO_ERROR)
            {
                return(ST_ERROR_BAD_PARAMETER);
            }
    #endif /* STBLIT_HW_BLITTER_RESET_DEBUG */
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

#ifdef STBLIT_ENABLE_BASIC_TRACE
    Device_p->NbBltSub = 0;
    Device_p->NbBltInt = 0;
#endif

#ifdef STBLIT_DEBUG_GET_STATISTICS
    Device_p->TotalGenerationTime = 0;
    Device_p->TotalExecutionTime = 0;
    Device_p->TotalProcessingTime = 0;
    Device_p->TotalExecutionRate = 0;
#ifdef STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY
    Device_p->ExecutionRateCalculationStarted = FALSE;
#endif /* STBLIT_DEBUG_STATISTICS_USE_FIRST_AND_LAST_TIME_VALUE_ONLY */
    Device_p->stblit_Statistics.Submissions = 0;
    Device_p->stblit_Statistics.BlitCompletionInterruptions = 0;
    Device_p->stblit_Statistics.CorrectTimeStatisticsValues = 0;
    Device_p->stblit_Statistics.MinGenerationTime = 1000000;
    Device_p->stblit_Statistics.AverageGenerationTime = 0;
    Device_p->stblit_Statistics.MaxGenerationTime = 0;
    Device_p->stblit_Statistics.LatestBlitGenerationTime = 0;
    Device_p->stblit_Statistics.MinExecutionTime = 1000000;
    Device_p->stblit_Statistics.AverageExecutionTime = 0;
    Device_p->stblit_Statistics.MaxExecutionTime = 0;
    Device_p->stblit_Statistics.LatestBlitExecutionTime = 0;
    Device_p->stblit_Statistics.MinProcessingTime = 1000000;
    Device_p->stblit_Statistics.AverageProcessingTime = 0;
    Device_p->stblit_Statistics.MaxProcessingTime = 0;
    Device_p->stblit_Statistics.LatestBlitProcessingTime = 0;
    Device_p->stblit_Statistics.MinExecutionRate = 1000000;
    Device_p->stblit_Statistics.AverageExecutionRate = 0;
    Device_p->stblit_Statistics.MaxExecutionRate = 0;
    Device_p->stblit_Statistics.LatestBlitExecutionRate = 0;
#endif /* STBLIT_DEBUG_GET_STATISTICS */

#ifdef STBLIT_USE_MEMORY_TRACE
    MemoryTrace_Init( Device_p->CPUPartition_p );
#endif

return(Err);
}

#ifdef ST_OSWINCE
// Accessory function for debug purpose
void stblit_DebugPools(STBLIT_Handle_t Handle)
{
    stblit_Unit_t* Unit_p   = (stblit_Unit_t*) Handle;
    stblit_Device_t*    Device_p = Unit_p->Device_p;

    printf("stblit_DebugPools: showing status of pools ...\n");
    printf("Single-blit nodes: ");
    stblit_DebugDataPool(&Device_p->SBlitNodeDataPool);
    printf("Job-blit nodes: ");
    stblit_DebugDataPool(&Device_p->JBlitNodeDataPool);
    printf("Jobs: ");
    stblit_DebugDataPool(&Device_p->JobDataPool);
    printf("Job blits: ");
    stblit_DebugDataPool(&Device_p->JBlitDataPool);
}

#endif /* ST_OSWINCE */

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

#else
    STAVMEM_FreeBlockParams_t   FreeParams;
    task_t*                     InterruptTask_p = Device_p->InterruptProcessTask_p;
#endif

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    task_t*                     BlitterErrorCheckTask_p = Device_p->BlitterErrorCheckProcessTask_p;
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */



    if (STEVT_Close(Device_p->EvtHandle) != ST_NO_ERROR)
    {
        STTBX_Report ((STTBX_REPORT_LEVEL_ERROR,"Evt Close failed"));
    }


    /* Task deletion process */
    Device_p->TaskTerminate = TRUE;

#ifndef STBLIT_LINUX_FULL_USER_VERSION
    /* Params for shared memory deallocation */
    FreeParams.PartitionHandle = Device_p->AVMEMPartition;
#endif

    /* Release remaining continuation node */
    if (!Device_p->AQFirstInsertion[0])
    {
        stblit_ReleaseNode(Device_p, (stblit_NodeHandle_t)Device_p->ContinuationNodeTab[0]);
    }

    /* Single Blit Node Data Pool deallocation */
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
#elif !defined(STBLIT_SINGLE_BLIT_DCACHE_NODES_ALLOCATION)
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
#elif !defined(STBLIT_JOB_DCACHE_NODES_ALLOCATION)
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

    /* Work buffer deallocation */
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

    /* Signal any semaphore pending to go out of the waits (TBD) */
   STOS_SemaphoreSignal(Device_p->InterruptProcessSemaphore);

#ifndef ST_OSLINUX
#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_InterruptSemaphore);
    STOS_SemaphoreSignal(Device_p->BlitterErrorCheck_SubmissionSemaphore);
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */
#endif

    /* delete tasks */

    #if !defined(STBLIT_LINUX_FULL_USER_VERSION)
        #ifndef ST_GX1 /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
        STOS_TaskWait(  &InterruptTask_p, TIMEOUT_INFINITY );
        #endif
        Err = STOS_TaskDelete ( InterruptTask_p,
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

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    /* delete tasks */
    #if !defined(STBLIT_LINUX_FULL_USER_VERSION)
        #ifndef ST_GX1 /* TO BE REMOVED WHEN task_wait ON OS20EMU FIXED */
        STOS_TaskWait(  &BlitterErrorCheckTask_p, TIMEOUT_INFINITY );
        #endif
        Err = STOS_TaskDelete ( BlitterErrorCheckTask_p,
            Device_p->CPUPartition_p,
            Device_p->BlitterErrorCheckProcessTaskStack,
            Device_p->CPUPartition_p );
        if(Err != ST_NO_ERROR)
        {
#if defined(DVD_STAPI_DEBUG)
            STTBX_Print(("Error in call to STOS_TaskDelete(): %s %i\n\r",__FILE__, __LINE__));
#endif/*DVD_STAPI_DEBUG*/
            return(Err);
        }
    #endif
#endif /* STBLIT_HW_BLITTER_RESET_DEBUG */




#if defined(ST_OSLINUX)  && !defined(STBLIT_LINUX_FULL_USER_VERSION)
    /** free BLIT region **/
    STOS_UnmapRegisters( (void*)Device_p->BlitMappedBaseAddress_p, (U32)Device_p->BlitMappedWidth);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */

#ifdef STBLIT_LINUX_FULL_USER_VERSION
#ifndef PTV

#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    Err = STBLIT_UserTaskTerm( Device_p->ErrorCheckProcessTask, NULL );
#endif
    if (Err != ST_NO_ERROR)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :ErrorCheckProcessTask\n" ));
	}

    Err = STBLIT_UserTaskTerm( Device_p->InterruptEventTask, NULL );

    if (Err != ST_NO_ERROR)
	{
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error terminating task :stblit_InterruptEvent_Task 1\n" ));
    }




#else /* !PTV */


    Err = STBLIT_UserTaskTerm( Device_p->InterruptEventTask);


#ifdef STBLIT_HW_BLITTER_RESET_DEBUG
    Err = STBLIT_UserTaskTerm( Device_p->ErrorCheckProcessTask);
#endif

#endif /* !PTV */


    MappingArea.StartAddress = (unsigned long)Device_p->BlitMappedBaseAddress_p;
    MappingArea.MappingIndex = 0;
    MappingArea.MappingNumber_p = &BlitterMappingNumber;
    MappingArea.MappedAddress_pp = &BlitterVirtualAddress;

#ifndef PTV
    Err = ModuleUnmapRegisters(fileno(Device_p->BLIT_DevFile), &MappingArea);
#else
    Err = ModuleUnmapRegisters(Device_p->BLIT_DevNum, &MappingArea);
#endif /* !PTV */
    /* unnistall blitter irq in kernel */
   ioctl( fileno(Device_p->BLIT_DevFile), STBLIT_IOCTL_UNINSTALL_INT);
#else
    Err = STOS_InterruptUninstall( Device_p->BlitInterruptNumber, Device_p->BlitInterruptLevel, (void *)Device_p);
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

    if ( (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_5100) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_5105) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_5301) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_7109) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_5525) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_5188) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_7200) &&
         (InitParams_p->DeviceType != STBLIT_DEVICE_TYPE_BDISP_5107))
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

                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated", DeviceName));
                }
            } /* End terminate OK */
        } /* End valid device */
    } /* End FirstInitDone */

    LeaveCriticalSection();

    return(Err);
}

#ifdef ST_ZEUS_PTV_MEM

#if 0 /* temp stub*/
unsigned long gfx_UserToBus( void *pUser)
{
	unsigned long i=0;
	return (i);
}

void * gfx_BusToUser(unsigned long BusAddr)
{
	void * i=NULL;
	return (i);
}


void * gfx_Alloc( unsigned long size, unsigned long byteAlignment )
{
	printf("stblit_ioctl_lib: gfx_Alloc STUB size=0x%x, byteAlignment=0x%x\n",size,byteAlignment);
	return NULL;
}


void gfx_Free( void *pMem )
{
	printf("stblit_ioctl_lib: gfx_Free STUB\n");
	return;
}
#endif/* temp stub*/

/*
--------------------------------------------------------------------------------
PTV version of STLAYER_AllocData.
uses PTV 16M graphics partition instead of AVMEM.
AVMEM is not used at all
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STLAYER_AllocData( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p )
{
	ST_ErrorCode_t		ErrorCode = ST_NO_ERROR;

    printf("%s:: calling gfx_Alloc with size %d and align = %d \n", __FUNCTION__, (unsigned long) Params_p->Size, (unsigned long)Params_p->Alignment);
	*Address_p = gfx_Alloc((unsigned long) Params_p->Size, (unsigned long) Params_p->Alignment);
    printf("%s:: Address = 0x%x\n", __FUNCTION__, Address_p);
	if (*Address_p == NULL)
	{
		printf("stblit_ioctl_lib: STLAYER_AllocData: gfx_Alloc FAILED\n");
		ErrorCode = ST_ERROR_BAD_PARAMETER;
	}
	return(ErrorCode);

}
/*
--------------------------------------------------------------------------------
PTV version of STLAYER_FreeData
uses PTV 16M graphics partition instead of AVMEM.
AVMEM is not used at all
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STLAYER_FreeData( STLAYER_Handle_t  LayerHandle, void *Address_p )
{
	ST_ErrorCode_t		ErrorCode = ST_NO_ERROR;
	gfx_Free(Address_p);
	return(ErrorCode);
}

#endif /*ST_ZEUS_PTV_MEM*/

/* End of bdisp_init.c */
