/*******************************************************************************

File name   : stgfx_init.c

Description : STGFX module standard API functions source file

COPYRIGHT (C) STMicroelectronics 2000/2001.
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sttbx.h"

#include "stgfx_init.h"


/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */
#define STGFX_MAX_DEVICE       1 
#define STGFX_MAX_UNIT         5 
#define INVALID_DEVICE_INDEX (-1)
#define DEVICE_HEAP_SIZE       65536  /* 64 kB */


/* Private Variables (static)------------------------------------------------ */

static stgfx_Device_t DeviceArray[STGFX_MAX_DEVICE]; /* devices descriptors */
static stgfx_Unit_t UnitArray[STGFX_MAX_UNIT];       /* units (connections)
                                                        descriptors */

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
static semaphore_t FctAccessCtrl; /* Init/Term protection semaphore */
#else /*yxl 2006-11-30 add below line*/
static semaphore_t* pFctAccessCtrl; /* Init/Term protection semaphore */
#endif/*end  yxl 2006-11-30 add below line*/
static FirstInitDone = FALSE;     /* flag for first initialization done */

/* THESE NEED TO BE BETTER HANDLED IF STGFX_MAX_DEVICE > 1 */
static STAVMEM_BlockHandle_t AVMEM_SyncBitmapBlockHandle;
static U8*                   SyncBitmapData_p;


/* Global Variables --------------------------------------------------------- */

#ifdef STGFX_COUNT_AVMEM
U32 STGFX_AvmemAllocCount = 0;
U32 STGFX_AvmemFreeCount = 0;
U32 STGFX_AvmemFreeErrorCount = 0;
 /* errors occur when we try to free the same block freed twice, for example */
#endif

/* Private Macros ----------------------------------------------------------- */

/* encapsulate stgfx_Unit_t* -> STGFX_Handle_t conversion, for possible later
  change to handles that are indexes into UnitArray */
#define UnitToHandle(Unit_p) ((STGFX_Handle_t)(Unit_p))

/* Private Function prototypes ---------------------------------------------- */

static void EnterCriticalSection(void);

static void LeaveCriticalSection(void);

static ST_ErrorCode_t CheckDeviceName(const ST_DeviceName_t Name);

static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName);

static ST_ErrorCode_t stgfx_Init(
  stgfx_Device_t * const Device_p,
  const STGFX_InitParams_t * const InitParams_p
);

static ST_ErrorCode_t stgfx_Open(stgfx_Unit_t * const Unit_p);

static ST_ErrorCode_t stgfx_Close(stgfx_Unit_t * const Unit_p);

static ST_ErrorCode_t stgfx_Term(
  stgfx_Device_t * const Device_p,
  const STGFX_TermParams_t * const TermParams_p
);


/* Callback Functions prototypes -------------------------------------------- */

/* callback to be executed at registration time by the blitter */
void stgfx_BlitRegisterCallback(
  STEVT_CallReason_t      Reason,
  const ST_DeviceName_t   RegistrantName,
  STEVT_EventConstant_t   Event,
  const void*             EventData_p,
  const void*             SubscriberData_p
);

/* callback to be executed at blit completion time by the blitter */
void stgfx_BlitNotifyCallback(
  STEVT_CallReason_t      Reason,
  const ST_DeviceName_t   RegistrantName,
  STEVT_EventConstant_t   Event,
  const void*             EventData_p,
  const void*             SubscriberData_p
);

/* callback to be executed at un-registration time by the blitter */
void stgfx_BlitUnregisterCallback(
  STEVT_CallReason_t      Reason,
  const ST_DeviceName_t   RegistrantName,
  STEVT_EventConstant_t   Event,
  const void*             EventData_p,
  const void*             SubscriberData_p
);


/* Functions ---------------------------------------------------------------- */


/* STATIC FUNCTIONS --------------------------------------------------------- */

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
  static  BOOL FctAccessCtrlInitialized = FALSE;

  interrupt_lock();
  if (!FctAccessCtrlInitialized)
  {
    /* Initialise the Init/Term protection semaphore
      Caution: this semaphore is never deleted. (Not an issue) */

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
    semaphore_init_fifo(&FctAccessCtrl, 1);
#else /*yxl 2006-11-30 add below line*/
    pFctAccessCtrl=semaphore_create_fifo(1);
#endif/*end  yxl 2006-11-30 add below line*/
    FctAccessCtrlInitialized = TRUE;
  }
  interrupt_unlock();

  /* Wait for the Init/Open/Close/Term protection semaphore */

  #ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
  semaphore_wait(&FctAccessCtrl);
#else /*yxl 2006-11-30 add below line*/
  semaphore_wait(pFctAccessCtrl);
#endif/*end  yxl 2006-11-30 add below line*/
}

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
  /* Release the Init/Term protection semaphore */
	
#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
	semaphore_signal(&FctAccessCtrl);
#else /*yxl 2006-11-30 add below line*/
	semaphore_signal(pFctAccessCtrl);
#endif/*end  yxl 2006-11-30 add below line*/
}

/*******************************************************************************
Name        : stgfx_Init()
Description : STGFX API specific initializations
Parameters  : pointer on device and initialization parameters, neither NULL
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Init function
*******************************************************************************/
ST_ErrorCode_t stgfx_Init(
  stgfx_Device_t* const             Device_p,
  const STGFX_InitParams_t* const   InitParams_p
)
{
  ST_ErrorCode_t                        Err = ST_NO_ERROR;
  U32                                   RequiredMemorySize;
  STBLIT_OpenParams_t                   BlitOpenParams;
  STAVMEM_AllocBlockParams_t            AVMEMAllocBlockParams;
  void *                                VAddr;
  
  
  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "Entering stgfx_Init()"));
  
  /* Test initialisation parameters and exit if some are invalid. */
  if(InitParams_p->CPUPartition_p == NULL)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "stgfx_Init(): bad pointer given"));
    return(ST_ERROR_BAD_PARAMETER);
  }
  
  if(InitParams_p->NumBitsAccuracy > 8)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "stgfx_Init(): bad NumBitsAccuracy"));
    return(ST_ERROR_BAD_PARAMETER);
  }
  
  /* Set up subpartition, in which we will allocate/release blocks */
  Device_p->CPUPartition_p = InitParams_p->CPUPartition_p;
  RequiredMemorySize = sizeof(STGXOBJ_Bitmap_t) + DEVICE_HEAP_SIZE;
  Device_p->MemoryAllocated_p = memory_allocate(InitParams_p->CPUPartition_p,
                                                RequiredMemorySize);
  if (Device_p->MemoryAllocated_p == NULL)
  {
    return(ST_ERROR_NO_MEMORY);
  }
  
  Device_p->DevicePartition_p = partition_create_heap(
                                             (void*)Device_p->MemoryAllocated_p,
                                             RequiredMemorySize);

  /* write user precision in appropriate field */
  Device_p->NumBitsAccuracy = InitParams_p->NumBitsAccuracy;
  
  /* register shared memory partition handle */
  Device_p->AVMemPartitionHandle = InitParams_p->AVMemPartitionHandle;

  Err = STAVMEM_GetSharedMemoryVirtualMapping(&Device_p->SharedMemMap);
  if ( Err != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error retriving shared memory mapping"));
      return (STGFX_ERROR_STAVMEM);
  }

  /* allocate microbitmap for sync (has to be allocated for alignment) */
  AVMEMAllocBlockParams.PartitionHandle = InitParams_p->AVMemPartitionHandle;
  AVMEMAllocBlockParams.Size            = 4;
  AVMEMAllocBlockParams.Alignment       = 4;
  AVMEMAllocBlockParams.AllocMode       = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
  AVMEMAllocBlockParams.NumberOfForbiddenRanges  = 0;
  AVMEMAllocBlockParams.ForbiddenRangeArray_p    = NULL;
  AVMEMAllocBlockParams.NumberOfForbiddenBorders = 0;
  AVMEMAllocBlockParams.ForbiddenBorderArray_p   = NULL;

  Err = STAVMEM_AllocBlock(&AVMEMAllocBlockParams,
                           &AVMEM_SyncBitmapBlockHandle);
  if(Err != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "stgfx_Init(): error during call to STAVMEM_AllocBlock()"));
    memory_deallocate(Device_p->CPUPartition_p, Device_p->MemoryAllocated_p);
    return(STGFX_ERROR_STAVMEM);
  }
  Err = STAVMEM_GetBlockAddress(AVMEM_SyncBitmapBlockHandle,
                                (void**)&VAddr);
  if(Err != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error during avmem ALLOCATION"));
    return(STGFX_ERROR_STAVMEM);
  }
  
  SyncBitmapData_p = (void *)STAVMEM_IfVirtualThenToCPU(VAddr, &Device_p->SharedMemMap);
  
  /* write event handler name - used in Open() to open a connection to it */ 
  strcpy(Device_p->EventHandlerName, InitParams_p->EventHandlerName);

  /* blitter Open() */
  strcpy(Device_p->STBLITDeviceName,InitParams_p->BlitName);
  BlitOpenParams.Dummy = 0;
  Err = STBLIT_Open(Device_p->STBLITDeviceName, &BlitOpenParams,
                    &(Device_p->BlitHandle));
  if(Err != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "stgfx_Init(): error during call to STBLIT_Open()"));
    memory_deallocate(Device_p->CPUPartition_p, Device_p->MemoryAllocated_p);
    return(STGFX_ERROR_STBLIT_DRV);
  }
  
  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "Leaving stgfx_Init()"));
  return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : stgfx_Term()
Description : API specific terminations
Parameters  : pointer on device and termination parameters
Assumptions : pointer on device is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Term function
*******************************************************************************/
ST_ErrorCode_t stgfx_Term(
  stgfx_Device_t* const            Device_p,
  const STGFX_TermParams_t* const  TermParams_p
)
{
  ST_ErrorCode_t Err, LastErr = ST_NO_ERROR;
  STAVMEM_FreeBlockParams_t  FreeBlockParams;
  
  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "Entering gstgfx_Term()"));

  /* CPU memory deallocation */
  memory_deallocate(Device_p->CPUPartition_p, Device_p->MemoryAllocated_p);
  
  /* free synchronization databuffer bitmap */
  FreeBlockParams.PartitionHandle = Device_p->AVMemPartitionHandle; 
  Err = STAVMEM_FreeBlock(&FreeBlockParams, &AVMEM_SyncBitmapBlockHandle);
  if(Err != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "stgfx_Term(): error during call to STAVMEM_FreeBlock()"));
    LastErr = STGFX_ERROR_STAVMEM;
  }
  
  /* Blitter Close() */
  Err = STBLIT_Close(Device_p->BlitHandle);
  if(Err != ST_NO_ERROR)
  {
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "stgfx_Term(): error during call to STBLIT_Close()"));
    LastErr = STGFX_ERROR_STBLIT_DRV;
  }
  
  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "Leaving stgfx_Term()"));
  return (LastErr);
}

/*******************************************************************************
Name        : CheckDeviceName
Description : Check the given device name is neither too long nor too short
Assumptions : none
Limitations :
Returns     : ST_NO_ERROR / ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t CheckDeviceName(const ST_DeviceName_t Name)
{
  int i;
  
  if(Name[0] == '\0')
  {
    return ST_ERROR_BAD_PARAMETER;
  }
  
  /* require '\0' within MAX_DEVICE_NAME chars
   (strlen(Name) < ST_MAX_DEVICE_NAME-1, but safer) */
   
  for(i = 1; i < ST_MAX_DEVICE_NAME; ++i)
  {
    if(Name[i] == '\0') return ST_NO_ERROR;
  }
  
  return ST_ERROR_BAD_PARAMETER; /* doesn't end in time */
}

/*******************************************************************************
Name        : GetIndexOfDeviceNamed
Description : Get the index in DeviceArray where a device has been
              initialised with the wanted name, if it exists
Parameters  : the name to look for
Assumptions : FirstInitDone, strlen(WantedName) acceptable
Limitations :
Returns     : the index if the name was found, INVALID_DEVICE_INDEX otherwise
*******************************************************************************/
static S32 GetIndexOfDeviceNamed(const ST_DeviceName_t WantedName)
{
  S32 Index, WantedIndex = INVALID_DEVICE_INDEX;

  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "STGFX: entering GetIndexOfDeviceNamed()"));
                  
  for(Index = 0; (Index < STGFX_MAX_DEVICE); ++Index)
  {
    /* a name with length > 0 marks an initialised device, so just compare */
    if (strcmp(DeviceArray[Index].DeviceName, WantedName) == 0)
    {
      /* Name found in the initialised devices */
      WantedIndex = Index;
      break;
    }
  }
  
  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "STGFX: leaving GetIndexOfDeviceNamed()"));
  return(WantedIndex);
}


/*******************************************************************************
Name        : stgfx_Open
Description : API specific actions just before opening
Parameters  : pointer on unit and open parameters
Assumptions : Unit_p and Unit_p->Device_p not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Open function
*******************************************************************************/
static ST_ErrorCode_t stgfx_Open(
  stgfx_Unit_t * const Unit_p
)
{
  ST_ErrorCode_t                        Err = ST_NO_ERROR;
  STEVT_OpenParams_t                    EventHandlerOpenParams;
  
  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "STGFX: entering stgfx_Open()"));

  /*interrupt_lock();*/
  /* Initialise the sync semaphore (do we really need interrupt_lock()? */

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
   semaphore_init_fifo(&(Unit_p->SyncSemaphore), 0);
#else /*yxl 2006-11-30 add below line*/
   Unit_p->pSyncSemaphore=semaphore_create_fifo_timeout(0);/*20070724 change to timeout*/
#endif/*end  yxl 2006-11-30 add below line*/

  /*interrupt_unlock();*/
  
  /* open a connection to the Event Handler */
  Err = STEVT_Open(Unit_p->Device_p->EventHandlerName,
                   &EventHandlerOpenParams,
                   &(Unit_p->EventHandlerHandle));
  if(Err != ST_NO_ERROR)
  {

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
    semaphore_delete(&(Unit_p->SyncSemaphore));
#else /*yxl 2006-11-30 add below line*/
    semaphore_delete(Unit_p->pSyncSemaphore);
#endif/*end  yxl 2006-11-30 add below line*/

    return(STGFX_ERROR_STEVT_DRV);
  }
  
  /* this flag changes state to TRUE when the blitter
     registers to the EvHandler */
  Unit_p->BlitterRegistered = FALSE;
 /* Unit_p->GfxSubscribeParams.RegisterCallback   = stgfx_BlitRegisterCallback; yxl 2005-07-12 cancel this line*/
  Unit_p->GfxSubscribeParams.NotifyCallback     = stgfx_BlitNotifyCallback;
/*  Unit_p->GfxSubscribeParams.UnregisterCallback = stgfx_BlitUnregisterCallback;yxl 2005-07-12 cancel this line*/
  Unit_p->GfxSubscribeParams.SubscriberData_p   = (void*)Unit_p;
  
  /* subscribe to blit completion events */
  Err = STEVT_SubscribeDeviceEvent(Unit_p->EventHandlerHandle,
                                   Unit_p->Device_p->STBLITDeviceName,
                                   STBLIT_BLIT_COMPLETED_EVT,
                                   &(Unit_p->GfxSubscribeParams));
  if(Err != ST_NO_ERROR)
  {
    Err = STEVT_Close(Unit_p->EventHandlerHandle);

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
    semaphore_delete(&(Unit_p->SyncSemaphore));
#else /*yxl 2006-11-30 add below line*/
    semaphore_delete(Unit_p->pSyncSemaphore);
#endif/*end  yxl 2006-11-30 add below line*/
    return(STGFX_ERROR_STEVT_DRV); 
  }  
  
  /* ask for a device Id to the EventHandler */
  Err = STEVT_GetSubscriberID(Unit_p->EventHandlerHandle,
                              &(Unit_p->GfxSubscriberID));
  if(Err != ST_NO_ERROR)
  {
    Err = STEVT_Close(Unit_p->EventHandlerHandle);

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
    semaphore_delete(&(Unit_p->SyncSemaphore));
#else /*yxl 2006-11-30 add below line*/
    semaphore_delete(Unit_p->pSyncSemaphore);
#endif/*end  yxl 2006-11-30 add below line*/
    return(STGFX_ERROR_STEVT_DRV);
  }

  Unit_p->PfrData_p = NULL;

  STTBX_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,
                  "STGFX: leaving stgfx_Open()"));
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stgfx_Close
Description : API specific actions just before closing
Parameters  : pointer on unit
Assumptions : pointer on unit is not NULL
Limitations :
Returns     : ST_ErrorCode_t for the API Close function
              If not ST_NO_ERROR, the Handle would not be closed afterwards
*******************************************************************************/
static ST_ErrorCode_t stgfx_Close(stgfx_Unit_t * const Unit_p)
{
  ST_ErrorCode_t Err, LastErr = ST_NO_ERROR;

  /* sync semaphore can't be in use on entry because of single-thread guarantee.
    But blit operations might be pending, with memory to free at the end, so
    we need to wait for them */
#if 0  /*如果打开，会有信号量死锁，递归调用WZ 20101210*/
  Err = STGFX_Sync(UnitToHandle(Unit_p));
  if(Err != ST_NO_ERROR)
  {
    LastErr = Err;
  }
#endif  
  /* delete sync semaphore */
  /*interrupt_lock();*/

#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
  semaphore_delete(&(Unit_p->SyncSemaphore));
#else /*yxl 2006-11-30 add below line*/
  semaphore_delete(Unit_p->pSyncSemaphore);
#endif/*end  yxl 2006-11-30 add below line*/
  /*interrupt_unlock();*/

  /* close the connection to the Event Handler */
  Unit_p->BlitterRegistered = FALSE; /* ? */
  Err = STEVT_Close(Unit_p->EventHandlerHandle);
  if(Err != ST_NO_ERROR)
  {
    LastErr = STGFX_ERROR_STEVT_DRV;
  }

  /* free memory that olf may have allocated */
  if (Unit_p->PfrData_p)
  {
    memory_deallocate(Unit_p->Device_p->DevicePartition_p, Unit_p->PfrData_p);
  }

  return(LastErr);
}

/*******************************************************************************
Name        : stgfx_BlitNotifyCallback()
Description : This function is passed to the event handler as callback
              to be executed whenever the STBLIT_BLIT_COMPLETED_EVENT
              is notified to the appropriate STGFX connection
Parameters  :
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
void stgfx_BlitNotifyCallback(
  STEVT_CallReason_t      Reason,
  const ST_DeviceName_t   RegistrantName,
  STEVT_EventConstant_t   Event,
  const void*             EventData_p,
  const void*             SubscriberData_p
)
{
  stgfx_Unit_t*               GfxUnit_p = (stgfx_Unit_t*)SubscriberData_p;
  ST_ErrorCode_t              Err;
  STAVMEM_BlockHandle_t       BH;
  STAVMEM_FreeBlockParams_t   FreeParams;
  
  TIME_HERE("STBLIT Notify Callback in");

#if 0
  /* we only register for this one event from this one Registrant */  
  if((strcmp(RegistrantName,GfxUnit_p->Device_p->STBLITDeviceName) == 0) &&
     (Event == STBLIT_BLIT_COMPLETED_EVT))
#endif
  {
#if 1 /*yxl 2007-06-14 temp modify below section*/
    if(EventData_p == &AVMEM_SyncBitmapBlockHandle)
#else
    if(EventData_p != &AVMEM_SyncBitmapBlockHandle)
#endif/*end yxl 2007-06-14 temp modify below section*/
    {
#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
      semaphore_signal(&(GfxUnit_p->SyncSemaphore));
#else /*yxl 2006-11-30 add below line*/
      semaphore_signal(GfxUnit_p->pSyncSemaphore);
#endif/*end  yxl 2006-11-30 add below line*/
    }
    else if(EventData_p != NULL) /* free AVMEM memory */
    {
      FreeParams.PartitionHandle = GfxUnit_p->Device_p->AVMemPartitionHandle;
      BH = (STAVMEM_BlockHandle_t)EventData_p;
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                    "AVMEM blockHandle received : %x\n", BH));

      Err = STAVMEM_FreeBlock(&FreeParams, &BH);
      if(Err != ST_NO_ERROR)
      {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error 0x%08X freeing AVMEM blockHandle 0x%08X\n",
                      Err, BH));
#ifdef STGFX_COUNT_AVMEM
        ++STGFX_AvmemFreeErrorCount;
      }
      else
      {
        ++STGFX_AvmemFreeCount;
#endif
      }
    }
  }

  TIME_HERE("STBLIT Notify Callback out");
}

/*******************************************************************************
Name        : stgfx_BlitRegisterCallback()
Description : This function is passed to the event handler as callback
              to be executed when the Blitter registers to the EventHandler
Parameters  :
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
void stgfx_BlitRegisterCallback(
  STEVT_CallReason_t      Reason,
  const ST_DeviceName_t   RegistrantName,
  STEVT_EventConstant_t   Event,
  const void*             EventData_p,
  const void*             SubscriberData_p
)
{
  stgfx_Unit_t* GfxUnit_p = (stgfx_Unit_t*)SubscriberData_p;
  
  if((strcmp(RegistrantName,GfxUnit_p->Device_p->STBLITDeviceName) == 0)/* &&
     Reason == CALL_REASON_REGISTER_CALL yxl 2005-07-12 cancel this line*/ )
  {
    GfxUnit_p->BlitterRegistered = TRUE;
  }
}

/*******************************************************************************
Name        : stgfx_BlitUnregisterCallback()
Description : This function is passed to the event handler as callback
              to be executed when the Blitter un-registers to the EventHandler
Parameters  : void
Assumptions : 
Limitations :
Returns     : 
*******************************************************************************/
void stgfx_BlitUnregisterCallback(
  STEVT_CallReason_t      Reason,
  const ST_DeviceName_t   RegistrantName,
  STEVT_EventConstant_t   Event,
  const void*             EventData_p,
  const void*             SubscriberData_p
)
{
  stgfx_Unit_t* GfxUnit_p = (stgfx_Unit_t*)SubscriberData_p;
  
  if((strcmp(RegistrantName,GfxUnit_p->Device_p->STBLITDeviceName) == 0) /* &&
     Reason == CALL_REASON_UNREGISTER_CALL yxl 2005-07-12 cancel this line*/)
  {
    GfxUnit_p->BlitterRegistered = FALSE;
  }
}


/* EXPORTED (API) FUNCTIONS ------------------------------------------------- */

/*
--------------------------------------------------------------------------------
Get revision of stgfx API
--------------------------------------------------------------------------------
*/
ST_Revision_t STGFX_GetRevision(void)
{
  static char Revision[] = "STGFX-REL_1.5.3";
  return((ST_Revision_t) Revision);
}

/*
--------------------------------------------------------------------------------
Initialise stgfx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGFX_Init(
  const ST_DeviceName_t DeviceName,
  const STGFX_InitParams_t* const InitParams_p
)
{
  S32 Index = 0;
  ST_ErrorCode_t Err = ST_NO_ERROR;

  /* Initial parameter checking */
  if(InitParams_p == NULL)
  {
    Err = ST_ERROR_BAD_PARAMETER;
  }
  else
  {
    Err = CheckDeviceName(DeviceName);
  }
  
  if(Err != ST_NO_ERROR)
  {
    return Err;
  }

  EnterCriticalSection();

  /* First time: initialise devices and units as empty */
  if(!FirstInitDone)
  {
     for (Index= 0; Index < STGFX_MAX_DEVICE; Index++)
     {
        DeviceArray[Index].DeviceName[0] = '\0';
     }

     for (Index= 0; Index < STGFX_MAX_UNIT; Index++)
     {
       UnitArray[Index].Device_p = NULL;
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
    while ((DeviceArray[Index].DeviceName[0] != '\0') &&
           (Index < STGFX_MAX_DEVICE))
    {
      Index++;
    }
    if(Index >= STGFX_MAX_DEVICE)
    {
      /* All devices initialised */
      Err = ST_ERROR_NO_MEMORY;
    }
    else
    {
      /* API specific initialisations */
      Err = stgfx_Init(&DeviceArray[Index], InitParams_p);

      if (Err == ST_NO_ERROR)
      {
        /* Device available and successfully initialised:register device name */
        strcpy(DeviceArray[Index].DeviceName, DeviceName);
        /*DeviceArray[Index].DeviceName[ST_MAX_DEVICE_NAME - 1] = '\0';*/
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' initialised",
                      DeviceArray[Index].DeviceName));
      }
    }
  }

  LeaveCriticalSection();

  return(Err);
}


/*
--------------------------------------------------------------------------------
Terminate stgfx API driver
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGFX_Term(
  const ST_DeviceName_t DeviceName,
  const STGFX_TermParams_t *const TermParams_p
)
{
  ST_ErrorCode_t  Err = ST_NO_ERROR;
  S32             DeviceIndex = INVALID_DEVICE_INDEX;
  stgfx_Unit_t*   Unit_p;

  /* Initial parameter checking */
  if(TermParams_p == NULL)
  {
    Err = ST_ERROR_BAD_PARAMETER;
  }
  else
  {
    Err = CheckDeviceName(DeviceName);
  }
  
  if(Err != ST_NO_ERROR)
  {
    return Err;
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
    if(DeviceIndex == INVALID_DEVICE_INDEX)
    {
      /* Device name not found */
      Err = ST_ERROR_UNKNOWN_DEVICE;
    }
    else /* device index is valid */
    {
      /* Check if there is still 'open' on this device */
      for(Unit_p = UnitArray; Unit_p <= &UnitArray[STGFX_MAX_UNIT]; ++Unit_p)
      {
        if(Unit_p->Device_p == &(DeviceArray[DeviceIndex]))
        {
          if (TermParams_p->ForceTerminate)
          {
            /* forced termination: the driver is terminated even if
               there are open connections */
               
            /* api-specific actions before closing */
            Err = stgfx_Close(Unit_p);

            /* Even in error cases unit should be counted as closed now */
            Unit_p->Device_p = NULL;
          }
          else
          {
            /* alternative: LeaveCriticalSection and return */
            Err = ST_ERROR_OPEN_HANDLE;
            break;
          }
        }
      }
      
      /* for valid device, terminate except for non-forced with open handles */
      if(Err != ST_ERROR_OPEN_HANDLE)
      {
        /* API specific terminations */
        Err = stgfx_Term(&DeviceArray[DeviceIndex], TermParams_p);

        DeviceArray[DeviceIndex].DeviceName[0] = '\0';
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Device '%s' terminated",
                      DeviceName));
      }
    }
  }
    
  LeaveCriticalSection();

  return(Err);
}

/*
--------------------------------------------------------------------------------
 STGFX API STGFX_SetGCDefaultValues() function
--------------------------------------------------------------------------------
*/

ST_ErrorCode_t STGFX_SetGCDefaultValues(
  STGXOBJ_Color_t           DrawColor,
  STGXOBJ_Color_t           FillColor,
  STGFX_GC_t*               GC_p
)
{
  if(GC_p == NULL)
  {
    return(ST_ERROR_BAD_PARAMETER);
  }

  GC_p->AluMode             = STBLIT_ALU_COPY;
  GC_p->CapStyle            = STGFX_RECTANGLE_CAP;
  GC_p->ClipMode            = STGFX_NO_CLIPPING;
  /* GC_p->Rectangle          undefined */
  /* GC_p->ColorKey.Type      undefined */
  /* GC_p->ColorKey.Value     undefined */
  GC_p->ColorKeyCopyMode    = STBLIT_COLOR_KEY_MODE_NONE; 
  GC_p->DrawColor           = DrawColor; 
  GC_p->EnableFilling       = FALSE;
  GC_p->FillColor           = FillColor;
  GC_p->FillTexture_p       = NULL;
  GC_p->DrawTexture_p       = NULL;
  GC_p->Font_p              = NULL;
  GC_p->FontSize            = 32;
  GC_p->GlobalAlpha         = 255;
  GC_p->JoinStyle           = STGFX_RECTANGLE_JOIN;
  GC_p->LineStyle_p         = NULL;
  GC_p->LineWidth           = 1;
  GC_p->PolygonFillMode     = STGFX_EVENODD_FILL;
  GC_p->Priority            = 0;
  GC_p->TextForegroundColor = DrawColor;
  GC_p->TextBackgroundColor = FillColor;
  GC_p->TxtFontAttributes_p = NULL;
  GC_p->UseTextBackground   = FALSE;
  GC_p->XAspectRatio        = 1;
  GC_p->YAspectRatio        = 1;
  GC_p->Palette_p           = NULL;
  
  return(ST_NO_ERROR);
}

/*
--------------------------------------------------------------------------------
open ...
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGFX_Open(
  const ST_DeviceName_t     DeviceName,
  STGFX_Handle_t*           Handle_p
)
{
  S32 DeviceIndex = INVALID_DEVICE_INDEX, UnitIndex = 0;
  ST_ErrorCode_t Err = ST_NO_ERROR;

  /* Initial parameter checking */
  if(Handle_p == NULL)
  {
    Err = ST_ERROR_BAD_PARAMETER;
  }
  else
  {
    Err = CheckDeviceName(DeviceName);
  }
  
  if(Err != ST_NO_ERROR)
  {
    return Err;
  }

  /* locking logic: STGFX_DrawXXX functions cannot act on this unit until we have
    finished, because a unit may only be used from one thread. Another init/open/
    close/term cannot do anything nasty to the device/unit arrays because of the
    critical section */
    
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
      /* Look for a free unit and return error if none is free */
      UnitIndex = 0;
      while ((UnitArray[UnitIndex].Device_p != NULL) &&
             (UnitIndex < STGFX_MAX_UNIT))
      {
        UnitIndex++;
      }
      if (UnitIndex >= STGFX_MAX_UNIT)
      {
        /* None of the units is free */
        Err = ST_ERROR_NO_FREE_HANDLES;
      }
      else
      {
        /* stgfx_Open needs Device_p, but it also marks unit validity. This
          is the only member of stgfx_Unit_t managed by STGFX_Open/Close -
          stgfx_Open/Close deal with the rest, eg to help STGFX_Term */
          
        UnitArray[UnitIndex].Device_p = &DeviceArray[DeviceIndex];
        
        /* API specific actions after opening */
        Err = stgfx_Open(&UnitArray[UnitIndex]);
        if(Err == ST_NO_ERROR)
        {
          *Handle_p = UnitToHandle(&UnitArray[UnitIndex]);
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "Handle opened on device '%s'", DeviceName));
        }
        else
        {
          /* This unit shall not infact be valid */
          UnitArray[UnitIndex].Device_p = NULL;
        }
      }
    }
  }
  
  LeaveCriticalSection();
  return(Err);
}


/*
--------------------------------------------------------------------------------
Close
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGFX_Close(STGFX_Handle_t Handle)
{
  stgfx_Unit_t*   Unit_p;
  ST_ErrorCode_t  Err = ST_NO_ERROR;

  EnterCriticalSection();

  if(!FirstInitDone)
  {
    Err = ST_ERROR_INVALID_HANDLE;
  }
  else
  {
    Unit_p = stgfx_HandleToUnit(Handle);
    if (Unit_p == NULL)
    {
      Err = ST_ERROR_INVALID_HANDLE;
    }
    else
    {
      /* API specific actions before closing */
      Err = stgfx_Close(Unit_p);
      
      /* unit ends up closed even if an error occurs - Un-register opened handle */
      Unit_p->Device_p = NULL;
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Handle closed on device '%s'",
                    Unit_p->Device_p->DeviceName));
    }
  }
  
  LeaveCriticalSection();
  return(Err);
}




/*
--------------------------------------------------------------------------------
Sync
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGFX_Sync(STGFX_Handle_t Handle)
{

#if 1 /*yxl 2005-07-22 temp cancel this section for test,yxl 2007-06-06 ,yxl 2007-06-19 cancel below section*/
  ST_ErrorCode_t Err = ST_NO_ERROR;
  static STGXOBJ_Rectangle_t Rect = { 0, 0, 1, 1 };
  static STGXOBJ_Color_t Color = {STGXOBJ_COLOR_TYPE_CLUT8, 1};
  static STBLIT_BlitContext_t BC;
  stgfx_Unit_t *       Unit_p;
  static STGXOBJ_Bitmap_t Bitmap = {STGXOBJ_COLOR_TYPE_CLUT8,
                                    STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE,
                                    FALSE,
                                    STGXOBJ_ITU_R_BT601,
                                    STGXOBJ_ASPECT_RATIO_SQUARE,
                                    1, /* width */
                                    1, /* height */
                                    1, /* size in bytes: 1 because CLUT8 */
                                    0, /* offset */
                                    NULL, /* set in 1st instruction */
                                    1,    /* pitch */
                                    NULL, /* irrelevant */
                                    0,    /* irrelevant */
                                    0,    /* irrelevant */
                                    FALSE};   /* irrelevant */
  
  Bitmap.Data1_p = SyncBitmapData_p;

    
  /* check handle validity */
  Unit_p = stgfx_HandleToUnit(Handle);
  if(NULL == Unit_p)
  {
    return(ST_ERROR_INVALID_HANDLE);
  }
  
  BC.ColorKeyCopyMode       = STBLIT_COLOR_KEY_MODE_NONE;
  BC.AluMode                = STBLIT_ALU_COPY;
  BC.EnableMaskWord         = FALSE;
  BC.EnableMaskBitmap       = FALSE;
  BC.EnableColorCorrection  = FALSE;
  BC.Trigger.EnableTrigger  = FALSE;
  BC.GlobalAlpha            = 63;
  BC.EnableClipRectangle    = FALSE;
  BC.EnableFlickerFilter    = TRUE/*FALSE 打开抗闪烁功能sqzow*/;
  BC.JobHandle              = STBLIT_NO_JOB_HANDLE;
#if 1 /*yxl 2007-06-14 modify below section*/
  BC.UserTag_p              = &AVMEM_SyncBitmapBlockHandle;
  BC.NotifyBlitCompletion   = TRUE;
#else
  BC.UserTag_p              = NULL;
  BC.NotifyBlitCompletion   = FALSE;
#endif /*end yxl 2007-06-14 modify below section*/

  BC.EventSubscriberID      = ((stgfx_Unit_t*)Handle)->GfxSubscriberID;

#if 1 /*yxl 2007-06-05 temp modify below section for test*/

#if 0/* STBLIT_SetPixel goes to STBLIT_Blit and is not yet optimised,
        whereas STBLIT_FillRectangle has direct code and is optimised */
        
  Err = STBLIT_SetPixel((((stgfx_Unit_t*)Handle)->Device_p)->BlitHandle,
                        &Bitmap, 0, 0, &Color, &BC);
#else
  Err = STBLIT_FillRectangle((((stgfx_Unit_t*)Handle)->Device_p)->BlitHandle,
                             &Bitmap, &Rect, &Color, &BC);
#endif
 if(Err != ST_NO_ERROR)
  {
    return(STGFX_ERROR_STBLIT_BLIT);
  }
 
/* else return(ST_NO_ERROR);yxl 2007-06-07 temp add this line for test*/

#endif /*end yxl 2007-06-05 temp modify below section for test*/

 
#ifndef ST_OS21  /*yxl 2006-11-30 add this line*/
  semaphore_wait(&((stgfx_Unit_t*)Handle)->SyncSemaphore);
#else /*yxl 2006-11-30 add below line*/
  /*STTBX_Print(("\nYxlOk"));*/
#if 0 /*yxl 2007-06-19 temp modify below section for test,this is org*/
  semaphore_wait(((stgfx_Unit_t*)Handle)->pSyncSemaphore);
#else
  {
	  osclock_t                 time;
	 /* osclock_t                 time1;
	  osclock_t                 time2;*/

	  /*time = time_plus(time_now(), 15625*200);time is too long*/
	  time = time_plus(time_now(), ST_GetClocksPerSecond()/20);
	 /* time1 =time_now();*/
	  if(semaphore_wait_timeout(((stgfx_Unit_t*)Handle)->pSyncSemaphore,&time)!=0)
	  {
	  /* STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "TIMEOUT"));
		  ResetEngine();*/
          CH_ResetBlitEngine();/*zxg 20070724 add*/
          
	      do_report(0,("GFX error\n"));
	  }
	
	 	
	  /*time2 =time_now();
	  STTBX_Print(("\nYxlInfo:t=%d",time2-time1));*/

  }

#endif /*end yxl 2007-06-19 temp modify below section for test,this is org*/


 /* STTBX_Print((" YxlOk\n"));*/
#endif/*end  yxl 2006-11-30 add below line*/
#endif /*end yxl 2005-07-22 temp cancel this section for test*/
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stgfx_HandleToUnit
Description : Get the unit structure for a given handle, or NULL if invalid
              (caller should return ST_ERROR_INVALID_HANDLE)
Parameters  : handle to connection
Assumptions : none
Limitations : 
Returns     : if the handle is valid, it returns the corresponding unit
              if the handle is not valid, it returns NULL
*******************************************************************************/
stgfx_Unit_t * stgfx_HandleToUnit(STGFX_Handle_t Handle)
{
  stgfx_Unit_t * Unit_p = (stgfx_Unit_t *) Handle;
  
  /* Require that Unit_p point to an entry in UnitArray, that is active
    (attached to a device). Casts to U32 in second line are important
    or you get 1, 2, etc with unknown rounding */
    
  if((Unit_p < UnitArray) || (Unit_p > &UnitArray[STGFX_MAX_UNIT]) ||
     (((U32)Unit_p - (U32)UnitArray) % sizeof(UnitArray[0]) != 0) ||
     !FirstInitDone || (Unit_p->Device_p == NULL))
  {
    Unit_p = NULL;
  }
  
  return Unit_p;
}

/*******************************************************************************
Name        : stgfx_DEVMEM_malloc
Description : memory allocation in CPU memory
Parameters  : handle to connection,
              size of desired buffer
Assumptions : handle is valid!
Limitations : the initial buffer size
Returns     : on success, the pointer to the allocated memory
              on failure, NULL
*******************************************************************************/
/*void* stgfx_DEVMEM_malloc(STGFX_Handle_t Handle, size_t size)
{
  return(memory_allocate((((stgfx_Unit_t*)Handle)->Device_p)->DevicePartition_p,
                         size));
} - macro in stgfx_init.h */

/*******************************************************************************
Name        : stgfx_DEVMEM_free
Description : memory deallocation in CPU memory
Parameters  : handle to connection,
              pointer to allocated buffer
Assumptions : handle and pointer are valid!
Limitations : none
Returns     : void
*******************************************************************************/
/*void stgfx_DEVMEM_free(STGFX_Handle_t Handle, void* p)
{
  memory_deallocate((((stgfx_Unit_t*)Handle)->Device_p)->DevicePartition_p, p);
} - macro in stgfx_init.h */

/*******************************************************************************
Name        : stgfx_AVMEM_malloc
Description : memory allocation in shared memory
Parameters  : Input:
                GFX handle to connection,
                size of desired buffer
              Output:
                AVMEM block handle
                Physcal address of the allocated block
Assumptions : handle is valid!
Returns     : on success: 
                the pointer to the allocated memory
                the AVMEM block handle
                and ST_NO_ERROR
              on failure, NULL for both, and STGFX_ERROR_STAVMEM
*******************************************************************************/
ST_ErrorCode_t stgfx_AVMEM_malloc(
  STGFX_Handle_t          Handle,
  U32                     Size,
  STAVMEM_BlockHandle_t*  AVMEM_BlockHandle_p,
  void **                 PAddr_p
)
{
  ST_ErrorCode_t              Err;
  STAVMEM_AllocBlockParams_t  AVMEMAllocBlockParams;
  void *                      VAddr;

  AVMEMAllocBlockParams.PartitionHandle =
                      (((stgfx_Unit_t*)Handle)->Device_p)->AVMemPartitionHandle;
  AVMEMAllocBlockParams.Size            = Size;
#if defined(ST_GX1) || defined(ST_7020)
  /* hardware actually needs 16-byte alignment */
  AVMEMAllocBlockParams.Alignment       = sizeof(STBLIT_XYL_t);
#else
  AVMEMAllocBlockParams.Alignment       = 4;
#endif
  AVMEMAllocBlockParams.AllocMode       = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
  AVMEMAllocBlockParams.NumberOfForbiddenRanges  = 0;
  AVMEMAllocBlockParams.ForbiddenRangeArray_p    = NULL;
  AVMEMAllocBlockParams.NumberOfForbiddenBorders = 0;
  AVMEMAllocBlockParams.ForbiddenBorderArray_p   = NULL;

  Err = STAVMEM_AllocBlock(&AVMEMAllocBlockParams,
                           AVMEM_BlockHandle_p);
  if ( Err != ST_NO_ERROR )
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error allocating AVMEM memory : 0x%08X",Err));
      *AVMEM_BlockHandle_p = STAVMEM_INVALID_BLOCK_HANDLE;
      *PAddr_p = NULL;
      return (STGFX_ERROR_STAVMEM);
  }
  
  Err = STAVMEM_GetBlockAddress( *AVMEM_BlockHandle_p, (void **)&(VAddr));
  if ( Err != ST_NO_ERROR )
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error obtaining block address : 0x%08X",Err));
      stgfx_AVMEM_free(Handle, AVMEM_BlockHandle_p);
      *AVMEM_BlockHandle_p = STAVMEM_INVALID_BLOCK_HANDLE;
      *PAddr_p = NULL;
      return (STGFX_ERROR_STAVMEM);
  }

  *PAddr_p = (void *)STAVMEM_IfVirtualThenToCPU(VAddr,
                        &(((stgfx_Unit_t*)Handle)->Device_p)->SharedMemMap);
  
#ifdef STGFX_COUNT_AVMEM
  ++STGFX_AvmemAllocCount;
#endif
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stgfx_AVMEM_GetPhysAddr
Description : Retrieve the physical address of an allocated block of
             memory in shared memory
Parameters  : Input:
                GFX handle to connection,
                size of desired buffer
              Output:
                AVMEM block handle
                Physcal address of the allocated block
Assumptions : handle is valid!
Returns     : on success: 
                the pointer to the allocated memory
                and ST_NO_ERROR
              on failure, NULL and STGFX_ERROR_STAVMEM
*******************************************************************************/
ST_ErrorCode_t stgfx_AVMEM_GetPhysAddr(
  STGFX_Handle_t          Handle,
  STAVMEM_BlockHandle_t   AVMEM_BlockHandle,
  void **                 PAddr_p
)
{
  ST_ErrorCode_t  Err;
  void *          VAddr;

  Err = STAVMEM_GetBlockAddress(AVMEM_BlockHandle, (void **)&(VAddr));
  if ( Err != ST_NO_ERROR )
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"Error obtaining block address : 0x%08X",Err));
      *PAddr_p = NULL;
      return (STGFX_ERROR_STAVMEM);
  }

  *PAddr_p = (void *)STAVMEM_IfVirtualThenToCPU(VAddr,
                        &(((stgfx_Unit_t*)Handle)->Device_p)->SharedMemMap);
                        
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stgfx_AVMEM_free
Description : memory deallocation in shared memory
Parameters  : handle to connection,
              pointer to allocated buffer
Assumptions : handle is valid!
Limitations : none
Returns     : ST_NO_ERROR/STGFX_ERROR_STAVMEM (though the block should still be
              considered released)
*******************************************************************************/
ST_ErrorCode_t stgfx_AVMEM_free(
  STGFX_Handle_t           Handle,
  STAVMEM_BlockHandle_t*   AVMEM_BlockHandle_p
)
{
  STAVMEM_FreeBlockParams_t   FreeParams;
  ST_ErrorCode_t              Err;

  FreeParams.PartitionHandle = 
                      (((stgfx_Unit_t*)Handle)->Device_p)->AVMemPartitionHandle;

  Err = STAVMEM_FreeBlock(&FreeParams, AVMEM_BlockHandle_p);
  if(Err != ST_NO_ERROR)
  {
    Err = STGFX_ERROR_STAVMEM;
#ifdef STGFX_COUNT_AVMEM
    ++STGFX_AvmemFreeErrorCount;
  }
  else
  {
    ++STGFX_AvmemFreeCount;
#endif
  }
  
  return(Err);
}

/*******************************************************************************
Name        : stgfx_ScheduleAVMEM_free
Description : Schedule the BLITTER to free the AVMEM memory used when it 
              ends all the previus operations.
Parameters  : handle to connection,
              handle of allocated memory
Assumptions : handles are valid!
Limitations : none
Returns     : ST_NO_ERROR/STGFX_ERROR_STBLIT_BLIT (the memory will not be freed
              in this case, though you must still wait until BLIT completes)
*******************************************************************************/
ST_ErrorCode_t stgfx_ScheduleAVMEM_free(
  STGFX_Handle_t           Handle,
  STAVMEM_BlockHandle_t*   AVMEM_BlockHandle_p
)
{
  ST_ErrorCode_t Err = ST_NO_ERROR;
  static STGXOBJ_Rectangle_t Rect = { 0, 0, 1, 1 };
  static STGXOBJ_Color_t Color = {STGXOBJ_COLOR_TYPE_CLUT8, 1};
  static STBLIT_BlitContext_t BC;
  static STGXOBJ_Bitmap_t Bitmap = {STGXOBJ_COLOR_TYPE_CLUT8,
                                    STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE,
                                    FALSE,
                                    STGXOBJ_ITU_R_BT601,
                                    STGXOBJ_ASPECT_RATIO_SQUARE,
                                    1, /* width */
                                    1, /* height */
                                    1, /* size in bytes: 1 because CLUT8 */
                                    0, /* offset */
                                    NULL, /* set in 1st instruction */
                                    1,    /* pitch */
                                    NULL, /* irrelevant */
                                    0,    /* irrelevant */
                                    0,    /* irrelevant */
                                    FALSE};   /* irrelevant */
  
  Bitmap.Data1_p = SyncBitmapData_p;
  
  BC.ColorKeyCopyMode       = STBLIT_COLOR_KEY_MODE_NONE;
  BC.AluMode                = STBLIT_ALU_COPY;
  BC.EnableMaskWord         = FALSE;
  BC.EnableMaskBitmap       = FALSE;
  BC.EnableColorCorrection  = FALSE;
  BC.Trigger.EnableTrigger  = FALSE;
  BC.GlobalAlpha            = 63;
  BC.EnableClipRectangle    = FALSE;
  BC.EnableFlickerFilter    = FALSE;
  BC.JobHandle              = STBLIT_NO_JOB_HANDLE;
  BC.UserTag_p              = (void *)(*AVMEM_BlockHandle_p);
  BC.NotifyBlitCompletion   = TRUE;
  BC.EventSubscriberID      = ((stgfx_Unit_t*)Handle)->GfxSubscriberID;
  
#if 0 /* STBLIT_SetPixel goes to STBLIT_Blit and is not yet optimised,
        whereas STBLIT_FillRectangle has direct code and is optimised */
        
  Err = STBLIT_SetPixel((((stgfx_Unit_t*)Handle)->Device_p)->BlitHandle,
                        &Bitmap, 0, 0, &Color, &BC);
#else
  Err = STBLIT_FillRectangle((((stgfx_Unit_t*)Handle)->Device_p)->BlitHandle,
                             &Bitmap, &Rect, &Color, &BC);
#endif
  if(Err != ST_NO_ERROR)
  {
    Err = STGFX_ERROR_STBLIT_BLIT;
    /* memory will be left hanging ... */
    
#ifdef STGFX_COUNT_AVMEM
    ++STGFX_AvmemFreeErrorCount;
#endif
  }

  return(Err);
}

/*
--------------------------------------------------------------------------------
Get capabilities of stgfx API
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STGFX_GetCapability(
  const ST_DeviceName_t     DeviceName,
  STGFX_Capability_t*       Capability_p
)
{
  S32 DeviceIndex = INVALID_DEVICE_INDEX;
  ST_ErrorCode_t Err = ST_NO_ERROR;

  /* Initial parameter checking */
  if(Capability_p == NULL)
  {
    Err = ST_ERROR_BAD_PARAMETER;
  }
  else
  {
    Err = CheckDeviceName(DeviceName);
  }
  
  if(Err != ST_NO_ERROR)
  {
    return Err;
  }

  /* Check if device already initialised and return error if not so */
  DeviceIndex = GetIndexOfDeviceNamed(DeviceName);
  if (DeviceIndex == INVALID_DEVICE_INDEX)
  {
    /* Device name not found */
    return(ST_ERROR_UNKNOWN_DEVICE);
  }

  /* Fill capability structure */
  Capability_p->MaxNumPriorities = 1;
  Capability_p->Blocking = FALSE;
  Capability_p->ColorModeConversion = FALSE;
  Capability_p->MaxNumHandles = STGFX_MAX_UNIT;
  Capability_p->MaxLineWidth = STGFX_MAX_LINE_WIDTH;
  Capability_p->MaxNumPolyPoints = STGFX_MAX_NUM_POLY_POINTS;

  return(ST_NO_ERROR);
}

/* End of stgfx_init.c */
