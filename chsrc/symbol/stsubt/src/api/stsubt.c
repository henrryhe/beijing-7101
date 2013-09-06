/******************************************************************************\ 
 *
 * File Name   : stsubt.c
 *  
 * Description : API functions implementation for subtitling decoder
 *  
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Dec 1998 - Dec 1999
 *  
\******************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <stlite.h>
#include <stddefs.h>
#include <stcommon.h>

#include <sttbx.h>

#ifdef DVD_TRANSPORT_STPTI
#include <stpti.h>
#elif defined DVD_TRANSPORT_PTI 
#include <pti.h>
#elif defined DVD_TRANSPORT_LINK 
#include <pti.h>
#include <ptilink.h>
#else  
#error "Only PTI, LINK or STPTI driver is supported !!"
#endif

#include <stevt.h> 

#include <common.h>
#include <dispserv.h>
#include <filter.h>
#include <decoder.h>
#include <dispctrl.h>
#include <compbuf.h>
#include <pixbuf.h>

#include "stsubt.h"
#include "subtdev.h"


/* --- Driver revision number --- */

const U8 STSUBT_RevisionLevel[] = "STSUBT_OLDARCH-REL_3.1.5";


/* --- Define structure to register all STSUBT instances --- */

static STSUBT_DecoderInstance_t STSUBT_Dictionary[STSUBT_MAX_NUM_INSTANCES];

/* --- Number of instances created --- */

static U32 STSUBT_NumInstances = 0;

/**********************************************************************
 Macros
**********************************************************************/

#define STSUBTMAGICNUMBER           0xAA7879BB

#define IS_VALID_STSUBT_HANDLE(d)    ((d)?((d)->MagicNumber == STSUBTMAGICNUMBER):0)




/******************************************************************************\
 * Function: STSUBT_GetRecovery
 * Purpose : Get del level of recovery.
 *
 * Parameters:
 *     SubtHandle:
 *             	Handle of SubTitle
 *
 * Return  : 
 *              Recovery Level.
 *
\******************************************************************************/

ST_ErrorCode_t STSUBT_GetRecovery (STSUBT_Handle_t SubtHandle,
				STSUBT_RecoveryLevel_t *RecoveryByte)
{
  STSUBT_InternalHandle_t *InternalHandle;

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  /* --- Check the handle --- */

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_GetRecovery: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }


  *RecoveryByte=InternalHandle->RecoveryByte;

  return(ST_NO_ERROR);
}



/******************************************************************************\
 * Function: STSUBT_SetRecovery
 * Purpose : Set Recovery Level, 0=no recovery, 1=recovery 
 *
 * Parameters:
 *     SubtHandle:
 *             	Handle of SubTitle
 *     RecoveryByte:
 *		Recovery Level
 * Return  : 
\******************************************************************************/

ST_ErrorCode_t STSUBT_SetRecovery (STSUBT_Handle_t SubtHandle,
				  STSUBT_RecoveryLevel_t RecoveryByte)
{
  STSUBT_InternalHandle_t *InternalHandle;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** STSUBT_SetRecovery: Setting recovery value **\n"));

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  /* --- Check the handle --- */

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_SetRecovery: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  if(RecoveryByte >= STSUBT_RecoveryLevel_NUMBER_OF)
        return (STSUBT_ERROR_BAD_PARAMETER);

  InternalHandle->RecoveryByte=RecoveryByte;

  return(ST_NO_ERROR);
}

/****************************************************************************\
 * Function: STSUBT_GetRevision
 *
 * Return  : Revision level of the subtitling decoder
\****************************************************************************/

ST_Revision_t STSUBT_GetRevision (void)
{
    return ((ST_Revision_t)STSUBT_RevisionLevel);
}


/******************************************************************************\
 * Function: STSUBT_Init
 *
 * Purpose : Creates an instances of the subtitling decoder; 
 *           allocates all decoder resources 
 *
 * Parameters:
 *     DeviceName:
 *           Device name 
 *     InitParams:
 *           Input parameters for STSUBT_Init
 *
 * Return  : ST_NO_ERROR on success
 *           STSUBT_ERROR_TOO_MANY_INSTANCES
 *           STSUBT_ERROR_ALREADY_INITIALIZED
 *           STSUBT_ERROR_ALLOCATING_MEMORY
 *           STSUBT_ERROR_CREATING_FILTER
 *           STSUBT_ERROR_CREATING_PROCESSOR
 *           STSUBT_ERROR_INITIALIZING_DISPLAY
 *
\******************************************************************************/

ST_ErrorCode_t STSUBT_Init (ST_DeviceName_t     DeviceName,
                            STSUBT_InitParams_t *InitParams)
{
  STSUBT_InternalHandle_t  *InternalHandle;
  ST_Partition_t           *MemoryPartition;
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  semaphore_t              *pti_semaphore; 
#endif
  U32			    CompositionBufferSize;
  U32			    PixelBufferSize;
  U32			    TempBufferSize;
  U32			    i;

  MemoryPartition = InitParams->MemoryPartition;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"STSUBT_Init: Creating instance of decoder!\n"));


  /* --- check if an instance to the specified DeviceName exists --- */

  for (i = 0; i < STSUBT_NumInstances; i++)
  {
      if (strcmp(STSUBT_Dictionary[i].DeviceName, DeviceName) == 0)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Init: already initialized\n"));
          return (STSUBT_ERROR_ALREADY_INITIALIZED);
      }
  }

    /* --- Find place into the dictionary --- */

  if (STSUBT_NumInstances >= STSUBT_MAX_NUM_INSTANCES)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: too many instances\n"));
      return (STSUBT_ERROR_TOO_MANY_INSTANCES);
  }

  
  /* --- Alloc memory for a new (internal) handle --- */
 
  InternalHandle = (STSUBT_InternalHandle_t*) 
      memory_allocate(MemoryPartition, sizeof(STSUBT_InternalHandle_t));

  if (InternalHandle == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot allocate handle\n"));
      return (STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* register partition name */

  InternalHandle->MemoryPartition = MemoryPartition;

    /* --- Get and register number of clocks per second --- */
  InternalHandle->clocks_per_sec = ST_GetClocksPerSecond(); /* akem moved from below */


#ifdef DVD_TRANSPORT_STPTI
  if (InitParams->NoCacheMemoryPartition == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: InitParams->NoCacheMemoryPartition is NULL\n"));
      return (ST_ERROR_BAD_PARAMETER);
  }
#endif


  InternalHandle->NoCacheMemoryPartition=InitParams->NoCacheMemoryPartition;

  /* --- create Coded Data Buffer --- */

  if ((InternalHandle->CodedDataBuffer = cBuffer_Create(
                                                 MemoryPartition,
                                                 CODED_DATA_BUFFER_SIZE,
                                                 &TempBufferSize)) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create CodedDataBuffer !\n"));
      /* release allocated resources */
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* --- create Composition Buffer --- */
 
  if (InitParams->FullScreenDisplayEnabled == FALSE)
       CompositionBufferSize = COMPOSITION_BUFFER_SIZE;
  else CompositionBufferSize = COMPOSITION_BUFFER_SIZE_FS;
          
  InternalHandle->CompositionBuffer1 = CompBuffer_Create(
                                      MemoryPartition,
                                      CompositionBufferSize,
                                      COMPOSITION_BUFFER_MAX_VIS_REGIONS);
  if (InternalHandle->CompositionBuffer1 == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create Composition Buffer !\n"));
      /* release allocated resources */
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
 
  /* --- create Composition Buffer 2 --- */
 
          
  InternalHandle->CompositionBuffer2 = CompBuffer_Create(
                                      MemoryPartition,
                                      CompositionBufferSize,
                                      COMPOSITION_BUFFER_MAX_VIS_REGIONS);
  if (InternalHandle->CompositionBuffer2 == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create Composition Buffer !\n"));
      /* release allocated resources */
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  
  /* --- create the Pixel Buffer --- */
 
  if (InitParams->FullScreenDisplayEnabled == FALSE)
       PixelBufferSize = PIXEL_BUFFER_SIZE;
  else PixelBufferSize = PIXEL_BUFFER_SIZE_FS;

  InternalHandle->PixelBuffer1 = 
      PixelBuffer_Create(MemoryPartition, PixelBufferSize);
  if (InternalHandle->PixelBuffer1 == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create Pixel Buffer !\n"));
      /* release allocated resources */
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  InternalHandle->PixelBuffer2 = 
      PixelBuffer_Create(MemoryPartition, PixelBufferSize);
  if (InternalHandle->PixelBuffer2 == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create Pixel Buffer !\n"));
      /* release allocated resources */
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }


  
  /* --- create the PCS Buffer --- */
 
  if ((InternalHandle->PCSBuffer = fcBuffer_Create(
                                            MemoryPartition,
                                            PCS_BUFFER_NUM_ITEMS,
                                            PCS_BUFFER_ITEM_SIZE)) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create PCS Buffer !\n"));
      /* release allocated resources */
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      PixelBuffer_Delete(InternalHandle->PixelBuffer2);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* --- create the Filter --- */

  if (Filter_Create(InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create Filter !\n"));
      /* release allocated resources */
      fcBuffer_Delete(InternalHandle->PCSBuffer);
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      PixelBuffer_Delete(InternalHandle->PixelBuffer2);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_CREATING_FILTER);
  }

  /* --- create the Processor --- */

  if (Processor_Create(InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: cannot create Processor !\n"));
      /* release allocated resources */
      Filter_Delete(InternalHandle);
      fcBuffer_Delete(InternalHandle->PCSBuffer);
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      PixelBuffer_Delete(InternalHandle->PixelBuffer2);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_CREATING_PROCESSOR);
  }

  /* --- initialize Display Controller --- */
 
  if (InitDisplayController (InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: error initializing display controller!\n"));
      /* release allocated resources */
      Processor_Delete(InternalHandle);
      Filter_Delete(InternalHandle);
      fcBuffer_Delete(InternalHandle->PCSBuffer);
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      PixelBuffer_Delete(InternalHandle->PixelBuffer2);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_INITIALIZING_DISPLAY);
  }

#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  /* --- Allocate pti semaphore --- */

  if ((pti_semaphore = 
      memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: Error creating pti semaphore\n"));
      /* release allocated resources */
      TermDisplayController (InternalHandle);
      Processor_Delete(InternalHandle);
      Filter_Delete(InternalHandle);
      fcBuffer_Delete(InternalHandle->PCSBuffer);
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      PixelBuffer_Delete(InternalHandle->PixelBuffer2);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(pti_semaphore,0);

  InternalHandle->pti_semaphore = pti_semaphore;
#endif

  /* Allocate display mutx semaphore */
  
  if ((InternalHandle->DisplayMutex= 
      memory_allocate(MemoryPartition, sizeof(semaphore_t))) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Init: Error creating DisplayMutex\n"));
      /* release allocated resources */
      TermDisplayController (InternalHandle);
      Processor_Delete(InternalHandle);
      Filter_Delete(InternalHandle);
      fcBuffer_Delete(InternalHandle->PCSBuffer);
      PixelBuffer_Delete(InternalHandle->PixelBuffer1);
      PixelBuffer_Delete(InternalHandle->PixelBuffer2);
      CompBuffer_Delete(InternalHandle->CompositionBuffer1);
      CompBuffer_Delete(InternalHandle->CompositionBuffer2);
      cBuffer_Delete(InternalHandle->CodedDataBuffer);
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
      semaphore_delete (InternalHandle->pti_semaphore);
      memory_deallocate (MemoryPartition, InternalHandle->pti_semaphore);
#endif            
      memory_deallocate (MemoryPartition, InternalHandle);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }
  semaphore_init_fifo_timeout(InternalHandle->DisplayMutex,1);

  

  /* --- Enable Common Interface, if required, and alloc resources --- */

  if (InitParams->CommonInterfaceEnabled != TRUE)
  {
      InternalHandle->CommonInterfaceEnabled = FALSE;
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"STSUBT_Init: Common Interface Enabled!\n"));

      InternalHandle->CommonInterfaceEnabled = TRUE;

      /* --- create Common Interface Reply buffer --- */

      if ((InternalHandle->CI_ReplyBuffer = cBuffer_Create(
                                                    MemoryPartition,
                                                    CI_REPLY_BUFFER_SIZE,
                                                    &TempBufferSize)) == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Init: cannot create CI Reply Buffer!\n"));
          /* release allocated resources */
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
          semaphore_delete (InternalHandle->pti_semaphore);
          memory_deallocate (MemoryPartition, InternalHandle->pti_semaphore);
#endif
          TermDisplayController (InternalHandle);
          Processor_Delete(InternalHandle);
          Filter_Delete(InternalHandle);
          fcBuffer_Delete(InternalHandle->PCSBuffer);
          PixelBuffer_Delete(InternalHandle->PixelBuffer1);
          PixelBuffer_Delete(InternalHandle->PixelBuffer2);
          CompBuffer_Delete(InternalHandle->CompositionBuffer1);
          CompBuffer_Delete(InternalHandle->CompositionBuffer2);
          cBuffer_Delete(InternalHandle->CodedDataBuffer);
          memory_deallocate (MemoryPartition, InternalHandle);
          return(STSUBT_ERROR_ALLOCATING_MEMORY);
      }

      /* --- create Common Interface message buffer --- */

      if ((InternalHandle->CI_MessageBuffer = cBuffer_Create(
                                                      MemoryPartition,
                                                      CI_MESSAGE_BUFFER_SIZE,
                                                      &TempBufferSize)) == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Init: cannot create CI Message Buffer!\n"));
          /* release allocated resources */
          cBuffer_Delete(InternalHandle->CI_ReplyBuffer);
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
          semaphore_delete (InternalHandle->pti_semaphore);          
          memory_deallocate (MemoryPartition, InternalHandle->pti_semaphore);
#endif
          TermDisplayController (InternalHandle);
          Processor_Delete(InternalHandle);
          Filter_Delete(InternalHandle);
          fcBuffer_Delete(InternalHandle->PCSBuffer);
          PixelBuffer_Delete(InternalHandle->PixelBuffer1);
          PixelBuffer_Delete(InternalHandle->PixelBuffer2);
          CompBuffer_Delete(InternalHandle->CompositionBuffer1);
          CompBuffer_Delete(InternalHandle->CompositionBuffer2);
          cBuffer_Delete(InternalHandle->CodedDataBuffer);
          memory_deallocate (MemoryPartition, InternalHandle);
          return(STSUBT_ERROR_ALLOCATING_MEMORY);
      }

      /* --- Allocate Common Interface Working buffer --- */

      if ((InternalHandle->CI_WorkingBuffer =
          (U8*)memory_allocate(MemoryPartition,CI_WORKING_BUFFER_SIZE)) == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Init: cannot create CI Working Buffer!\n"));
          /* release allocated resources */
          cBuffer_Delete(InternalHandle->CI_MessageBuffer);
          cBuffer_Delete(InternalHandle->CI_ReplyBuffer);
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
          semaphore_delete (InternalHandle->pti_semaphore);          
          memory_deallocate (MemoryPartition, InternalHandle->pti_semaphore);
#endif
          TermDisplayController (InternalHandle);
          Processor_Delete(InternalHandle);
          Filter_Delete(InternalHandle);
          fcBuffer_Delete(InternalHandle->PCSBuffer);
          PixelBuffer_Delete(InternalHandle->PixelBuffer1);
          PixelBuffer_Delete(InternalHandle->PixelBuffer2);
          CompBuffer_Delete(InternalHandle->CompositionBuffer1);
          CompBuffer_Delete(InternalHandle->CompositionBuffer2);
          cBuffer_Delete(InternalHandle->CodedDataBuffer);
          memory_deallocate (MemoryPartition, InternalHandle);
          return(STSUBT_ERROR_ALLOCATING_MEMORY);
      }

      /* --- Allocate array of 16 PCS Descriptors --- */

      if ((InternalHandle->ArrayPCS = 
          (PCS_t*)memory_allocate(MemoryPartition, 16*sizeof(PCS_t))) == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Init: cannot create Object Cache!\n"));
          /* release allocated resources */
          memory_deallocate (MemoryPartition, InternalHandle->CI_WorkingBuffer);
          cBuffer_Delete(InternalHandle->CI_MessageBuffer);
          cBuffer_Delete(InternalHandle->CI_ReplyBuffer);
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
          semaphore_delete (InternalHandle->pti_semaphore);          
          memory_deallocate (MemoryPartition, InternalHandle->pti_semaphore);
#endif
          TermDisplayController (InternalHandle);
          Processor_Delete(InternalHandle);
          Filter_Delete(InternalHandle);
          fcBuffer_Delete(InternalHandle->PCSBuffer);
          PixelBuffer_Delete(InternalHandle->PixelBuffer1);
          PixelBuffer_Delete(InternalHandle->PixelBuffer2);
          CompBuffer_Delete(InternalHandle->CompositionBuffer1);
          CompBuffer_Delete(InternalHandle->CompositionBuffer2);
          cBuffer_Delete(InternalHandle->CodedDataBuffer);
          memory_deallocate (MemoryPartition, InternalHandle);
          return(STSUBT_ERROR_ALLOCATING_MEMORY);
      }

      /* --- Allocate the object cache --- */

      if (InitParams->ObjectCacheSize == 0)
      {
          InternalHandle->ObjectCache = NULL;
          InternalHandle->ObjectCacheEnabled = FALSE;
      }
      else
      {
        if ((InternalHandle->ObjectCache =
                ObjectCache_Create (MemoryPartition,
                       InitParams->ObjectCacheSize)) == NULL)
        {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Init: cannot create Object Cache!\n"));
          /* release allocated resources */
          memory_deallocate (MemoryPartition, InternalHandle->ArrayPCS);
          memory_deallocate (MemoryPartition, InternalHandle->CI_WorkingBuffer);
          cBuffer_Delete(InternalHandle->CI_MessageBuffer);
          cBuffer_Delete(InternalHandle->CI_ReplyBuffer);
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
          semaphore_delete (InternalHandle->pti_semaphore);          
          memory_deallocate (MemoryPartition, InternalHandle->pti_semaphore);
#endif
          TermDisplayController (InternalHandle);
          Processor_Delete(InternalHandle);
          Filter_Delete(InternalHandle);
          fcBuffer_Delete(InternalHandle->PCSBuffer);
          PixelBuffer_Delete(InternalHandle->PixelBuffer1);
          PixelBuffer_Delete(InternalHandle->PixelBuffer2);
          CompBuffer_Delete(InternalHandle->CompositionBuffer1);
          CompBuffer_Delete(InternalHandle->CompositionBuffer2);
          cBuffer_Delete(InternalHandle->CodedDataBuffer);
          memory_deallocate (MemoryPartition, InternalHandle);
          return(STSUBT_ERROR_ALLOCATING_MEMORY);
        }
        InternalHandle->ObjectCacheEnabled = TRUE;
      }
  }

  /* --- Register instance, device name and id into the handle --- */
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"STSUBT_Init: Current Instance is %d!", STSUBT_NumInstances));

  InternalHandle->DeviceInstance = STSUBT_NumInstances;

  InternalHandle->MagicNumber = 0x00000000;

  /* --- Register new instance into the dictionary --- */
 
  strcpy(STSUBT_Dictionary[STSUBT_NumInstances].DeviceName, DeviceName);
  STSUBT_Dictionary[STSUBT_NumInstances].DecoderHandle = (U32)InternalHandle;
  STSUBT_Dictionary[STSUBT_NumInstances].HandleIsOpen = FALSE;
 
  /* --- increment instances counter --- */

  STSUBT_NumInstances ++;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"STSUBT_Init: Instance successfully created!\n"));

  return(ST_NO_ERROR);
}


/******************************************************************************\
 * Function: NotifyDisplayMessage
 * Purpose : Generate a display_message() APDU to be inserted in 
 *           the CI Message Buffer. 
 * !! Only in Overlay Mode !!
 *
\******************************************************************************/

static void NotifyDisplayMessage (STEVT_CallReason_t Reason,
								  const ST_DeviceName_t RegistrantName,
                                  STEVT_EventConstant_t Event,
                                  const void *EventData,
								  const void *SubscriberData_p)
{
  STSUBT_InternalHandle_t *InternalHandle;
  U8 display_message_id = 0;

  InternalHandle = (STSUBT_InternalHandle_t*)EventData;

  if(strcmp(RegistrantName,STSUBT_Dictionary[InternalHandle->DeviceInstance].DeviceName)!=0)
  {
	STTBX_Report((STTBX_REPORT_LEVEL_INFO,
        "NotifyDisplayMessage: Registrant Name not match with Istance Device Name\n"));
  }

  switch (Event) 
  {
    case STSUBT_EVENT_DISPLAY_ERROR: 
         display_message_id = 1;
         break;
    case STSUBT_EVENT_COMP_BUFFER_NO_SPACE: 
    case STSUBT_EVENT_PIXEL_BUFFER_NO_SPACE: 
    case STSUBT_EVENT_OBJECT_CACHE_NO_SPACE: 
         display_message_id = 2;
         break;
    case STSUBT_EVENT_BAD_PES: 
    case STSUBT_EVENT_BAD_SEGMENT: 
         display_message_id = 3;
         break;
    case STSUBT_EVENT_UNKNOWN_REGION: 
         display_message_id = 4;
         break;
    case STSUBT_EVENT_UNKNOWN_CLUT: 
         display_message_id = 5;
         break;
    case STSUBT_EVENT_UNKNOWN_OBJECT: 
         display_message_id = 6;
         break;
    case STSUBT_EVENT_INCOMPATIBLE_DEPTH: 
         display_message_id = 7;
         break;
    case STSUBT_EVENT_NOT_SUPPORTED_OBJECT_TYPE: 
    case STSUBT_EVENT_NOT_SUPPORTED_OBJECT_CODING: 
         display_message_id = 8;
         break;
  }

  /* insert display_message() APDU in the CI Message buffer */

  cBuffer_InsertAPDU_ow(InternalHandle->CI_MessageBuffer,
                        STSUBT_DISPLAY_MESSAGE_TAG,
                        1, &display_message_id);
}


/******************************************************************************\
 * Function: STSUBT_Open
 *
 * Purpose : Opens the subtitling decoder; 
 *
 * Parameters:
 *     DeviceName:
 *           Device name 
 *     OpenParams:
 *           Parameters for STSUBT_Open
 *     SubtHandle:
 *           Returned pointer to subtitling decoder handle
 *
 * Return  : ST_NO_ERROR on success
 *           ST_ERROR_UNKNOWN_DEVICE
 *           STSUBT_ERROR_OPEN_HANDLE
 *           STSUBT_ERROR_OVERLAY_MODE_NOT_ENABLED
 *           STSUBT_ERROR_INVALID_WORKING_MODE
 *           STSUBT_ERROR_OPENING_STEVT
 *           STSUBT_ERROR_OPENING_STCLKRV
 *           STSUBT_ERROR_EVENT_REGISTER
 *
 * Note    : In according to the "General Principles" about ST-API device
 *           driver common "feel", the STSUBT_Handle_t structure is provided
 *           by the caller.
\******************************************************************************/

ST_ErrorCode_t STSUBT_Open (ST_DeviceName_t     DeviceName,
                            STSUBT_OpenParams_t *OpenParams,
                            STSUBT_Handle_t    *SubtHandle)
{
  STSUBT_InternalHandle_t *InternalHandle;
  STEVT_Handle_t           stevt_handle;
  STCLKRV_OpenParams_t     stclkrv_openparams;
  STCLKRV_Handle_t         stclkrv_handle;
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
  ST_ErrorCode_t ErrorCode;
  STPTI_OpenParams_t     stpti_openparams;
  STPTI_SlotData_t       SlotData;
#endif
#endif
  ST_ErrorCode_t           res;
  U32			   i;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"STSUBT_Open: Opening the Driver !\n"));

  /* --- Retrieve handle associated to the specified DeviceName --- */

  InternalHandle = NULL;

  for (i = 0; i < STSUBT_NumInstances; i++)
  {
      if (strcmp(STSUBT_Dictionary[i].DeviceName, DeviceName) == 0)
      {
          InternalHandle = 
              (STSUBT_InternalHandle_t*)STSUBT_Dictionary[i].DecoderHandle;
          break;
      } 
  }

  if (InternalHandle == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Open: Device name not found\n"));
      *SubtHandle = 0;
      return (ST_ERROR_UNKNOWN_DEVICE);
  }

  /* --- check handle --- */

  if (STSUBT_Dictionary[InternalHandle->DeviceInstance].HandleIsOpen
													 == TRUE)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_Open: handle already open\n"));
      return (STSUBT_ERROR_OPEN_HANDLE);
  }

  /* --- Check and register the working mode --- */

  switch (OpenParams->WorkingMode) 
  {
      case STSUBT_NormalMode:
          InternalHandle->WorkingMode = STSUBT_NormalMode;
          InternalHandle->StreamInitialized  = FALSE;
          break;
      case STSUBT_OverlayMode:
          if (InternalHandle->CommonInterfaceEnabled == FALSE)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_Open: Working Mode not enabled\n"));
              return(STSUBT_ERROR_OVERLAY_MODE_NOT_ENABLED);
          }
          /* No stream needs to be set in overlay mode. 
           * Default standard for the CI is DVB.
           */
          InternalHandle->WorkingMode = STSUBT_OverlayMode;
          InternalHandle->SubtitleStandard = STSUBT_DVBStandard;
          InternalHandle->AutoDetection = FALSE;
          InternalHandle->StreamInitialized = TRUE;
          break;
      default:
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_Open: Working Mode not supported\n"));
          return(STSUBT_ERROR_INVALID_WORKING_MODE);
  }

  /* --- set handle pointer to null --- */

  *SubtHandle = 0;

  /* --- open a handle to the stevt driver --- */
  

  if ((res=STEVT_Open(OpenParams->stevt_name, NULL, &stevt_handle)) 
					!= ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                    "STSUBT_Open: Failed Opening Event Handler\n"));
      return(STSUBT_ERROR_OPENING_STEVT);
  }

  /* --- register Event Handler handle --- */

  InternalHandle->stevt_handle = stevt_handle;

  /* --- Register all SubT Events --- */

  for (i = 0 ; i < STSUBT_NUM_EVENTS; i++)
  {
      if ((res=STEVT_RegisterDeviceEvent(
                stevt_handle,
                DeviceName,
                STSUBT_DRIVER_BASE+i,
                InternalHandle->stevt_event_table + i)) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                        "STSUBT_Open: Error registering event 0x%x\n",res));
          return(STSUBT_ERROR_EVENT_REGISTER);
      }
  }

  /* --- open and register handle to stclkrv driver (normal mode only) --- */
 
  if (InternalHandle->WorkingMode == STSUBT_NormalMode)
  {
      if(STCLKRV_Open (OpenParams->stclkrv_name, 
                       &stclkrv_openparams, 
                       &stclkrv_handle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                        "STSUBT_Open: Failed Opening Clock Recovery Handle\n"));
          return(STSUBT_ERROR_OPENING_STCLKRV);
      }

      /* register Clock Recovery Driver Handle */

      InternalHandle->stclkrv_handle = stclkrv_handle;
  }

  /* --- Initialize pes demultiplexing (normal mode only) --- */

  if (InternalHandle->WorkingMode == STSUBT_NormalMode)
  {
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI

#if 0/*zxg 20050818 change*/
	  SlotData.SlotType=STPTI_SLOT_TYPE_PES;		
	  SlotData.SlotFlags.SignalOnEveryTransportPacket=false;
	  SlotData.SlotFlags.CollectForAlternateOutputOnly=FALSE;
	  SlotData.SlotFlags.StoreLastTSHeader=FALSE;
	  SlotData.SlotFlags.InsertSequenceError=FALSE;

#else
	  memset((void *)&SlotData, 0, sizeof(STPTI_SlotData_t));
      SlotData.SlotType = STPTI_SLOT_TYPE_PES;
      SlotData.SlotFlags.SignalOnEveryTransportPacket = false;
      SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
      SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
      SlotData.SlotFlags.StoreLastTSHeader = FALSE;
      SlotData.SlotFlags.InsertSequenceError = FALSE;
      SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
      SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    /*SlotData.SlotFlags.AppendSyncBytePrefixToRawData = TRUE;*/
#endif
	  stpti_openparams.DriverPartition_p
          = InternalHandle->MemoryPartition;

      stpti_openparams.NonCachedPartition_p
          = InternalHandle->NoCacheMemoryPartition;
      
      if((ErrorCode=STPTI_Open(OpenParams->stpti_name,
			&stpti_openparams,&InternalHandle->stpti_handle))!=ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Open: Error STPTI_Open() 0x%x\n",ErrorCode));
          return STSUBT_ERROR_OPENING_STPTI;
      }

	  if((ErrorCode=STPTI_SlotAllocate(InternalHandle->stpti_handle,
				&InternalHandle->stpti_slot, &SlotData))!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_SlotAllocate() 0x%x\n",ErrorCode));
          return STSUBT_ERROR_OPENING_STPTI;
	  }

	  /*zxg 20050819 add*/
	  ErrorCode = STPTI_SlotClearPid(InternalHandle->stpti_slot);
      /******************/

	  if((ErrorCode=STPTI_BufferAllocate(InternalHandle->stpti_handle,
                                             OpenParams->pti_buffer_size ,1,
                                             &InternalHandle->stpti_buffer)) != ST_NO_ERROR)
	  {
              STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
                            "STPTI_BufferAllocate() 0x%x\n",ErrorCode));
              return STSUBT_ERROR_OPENING_STPTI;
	  }

	  if((ErrorCode=STPTI_SignalAllocate(InternalHandle->stpti_handle,
				&InternalHandle->stpti_signal))!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_STPTI_SignalAllocate()  0x%x\n",ErrorCode));
          return STSUBT_ERROR_OPENING_STPTI;
	  }

/*zxg 20050819 move to here*/
#if 0
	  if((ErrorCode=STPTI_SlotLinkToBuffer(InternalHandle->stpti_slot,
				InternalHandle->stpti_buffer))!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: STPTI_SlotLinkToBuffer() Error.%d\n",
						ErrorCode));

          return (STSUBT_ERROR_STARTING_FILTER);
	  }

	  if(STPTI_SignalAssociateBuffer(InternalHandle->stpti_signal,
				InternalHandle->stpti_buffer)!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: STPTI_SignalAssociateBuffer() Error.\n"));

          return (STSUBT_ERROR_STARTING_FILTER);
	  }
#endif
/**************************************/
#endif
#if defined (DVD_TRANSPORT_PTI) || defined (DVD_TRANSPORT_LINK)

      /* --- Complete decoder handle information --- */

      InternalHandle->pti_slot = OpenParams->pti_slot;
      InternalHandle->pti_buffer = OpenParams->pti_buffer;
      InternalHandle->pti_buffer_size = OpenParams->pti_buffer_size;

      /* --- Set pti buffer --- */

      if (pti_set_buffer (InternalHandle->pti_slot,
                          stream_type_pes,
                          InternalHandle->pti_buffer,
                          InternalHandle->pti_buffer_size,
                          &InternalHandle->pti_producer,
                          &InternalHandle->pti_consumer,
                          InternalHandle->pti_semaphore,
#if defined (DVD_TRANSPORT_LINK)
                          no_flags)) 
#else
                          signal_on_every_transport_packet))
#endif
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_Open: Error setting pti buffer.\n"));
          return(STSUBT_ERROR_SETTING_SLOT);
      }

#endif

#else
      InternalHandle->pti_buffer = (cBuffer_t*)OpenParams->pti_buffer;
#endif
  }

  /* --- Subscribe events for display_message() APDU (overlay mode only) --- */

  if (OpenParams->WorkingMode == STSUBT_OverlayMode)
  {
      STEVT_DeviceSubscribeParams_t SubscrParam;

      SubscrParam.NotifyCallback = (STEVT_DeviceCallbackProc_t)NotifyDisplayMessage;
	  /*
      SubscrParam.RegisterCallback = NULL;
      SubscrParam.UnregisterCallback = NULL;
     */
      for (i = 0 ; i < STSUBT_NUM_EVENTS; i++)
      {
          switch (STSUBT_DRIVER_BASE+i) 
          {
            case STSUBT_EVENT_DISPLAY_ERROR:
            case STSUBT_EVENT_COMP_BUFFER_NO_SPACE:
            case STSUBT_EVENT_PIXEL_BUFFER_NO_SPACE:
            case STSUBT_EVENT_OBJECT_CACHE_NO_SPACE:
            case STSUBT_EVENT_BAD_PES:
            case STSUBT_EVENT_BAD_SEGMENT:
            case STSUBT_EVENT_UNKNOWN_REGION:
            case STSUBT_EVENT_UNKNOWN_CLUT:
            case STSUBT_EVENT_UNKNOWN_OBJECT:
            case STSUBT_EVENT_INCOMPATIBLE_DEPTH:
            case STSUBT_EVENT_NOT_SUPPORTED_OBJECT_TYPE:
            case STSUBT_EVENT_NOT_SUPPORTED_OBJECT_CODING:
            res = STEVT_SubscribeDeviceEvent(stevt_handle,DeviceName,
						 STSUBT_DRIVER_BASE+i, &SubscrParam);
             if (res != ST_NO_ERROR)
             {
                 STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
					"STSUBT_Open: Error subscribing Event (%d)\n",res));
                 return(STSUBT_ERROR_EVENT_SUBSCRIBE);
             }
             break;
          }
      }
  }

  /* --- Set  default level graphical object recovery --- */

  InternalHandle->RecoveryByte = STSUBT_NoRecovery;

  /* --- Set decoder status to Stopped --- */
 
  InternalHandle->DecoderStatus = STSUBT_Driver_Stopped;

  /* --- make the handle open and valid --- */
 
  STSUBT_Dictionary[InternalHandle->DeviceInstance].HandleIsOpen = TRUE;
 
  InternalHandle->MagicNumber = STSUBTMAGICNUMBER;

  /* --- Update number of clocks per second --- */

  InternalHandle->clocks_per_sec = ST_GetClocksPerSecond();

  /* --- External handle is just a pointer to the real handle --- */

  *SubtHandle = (U32)InternalHandle;

  
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"STSUBT_Open: Driver successfully opened !\n"));

  return(ST_NO_ERROR);
}


/******************************************************************************\
 * Function: STSUBT_SetStream
 *
 * Purpose : Set-up the subtitling decoder to extract a particular subtitle
 *           stream among the ones available in the incoming 
 *           transport packet stream (i.e. on a single PID).
 *
 * Parameters:
 *     SubtStream:
 *           A structure including the subtitle_pid, the composition_page_id and
 *           ancillary_page_id values which identify a subtitle stream     
 *     SubtStandard:
 *           Standard of subtitles stream
 *     SubtHandle:
 *           Handle of subtitling decoder     
 *     
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_INVALID_STANDARD
 *           STSUBT_ERROR_INVALID_WORKING_MODE
 *           STSUBT_ERROR_DRIVER_RUNNING
 *           STSUBT_ERROR_NO_STREAM_SET 
 *
 * Note    : The decoder must be in the Stopped status when setting the stream
\******************************************************************************/

ST_ErrorCode_t STSUBT_SetStream (STSUBT_Handle_t   SubtHandle,
                                 STSUBT_Stream_t   SubtStream,
                                 STSUBT_Standard_t SubtStandard)
{
  STSUBT_InternalHandle_t *InternalHandle;

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  /* --- check handle --- */

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_SetStream: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- check the working mode --- */

  if (InternalHandle->WorkingMode != STSUBT_NormalMode)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_SetStream: Error: Invalid working mode!\n"));
      return (STSUBT_ERROR_INVALID_WORKING_MODE);
  }

  /* --- check subtitles stream standard --- */

  if ((SubtStandard != STSUBT_DVBStandard)
  &&  (SubtStandard != STSUBT_DTGStandard)
  &&  (SubtStandard != STSUBT_StandardNotSpecified))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_SetStream: Error: Invalid standard!\n"));
      return (STSUBT_ERROR_INVALID_STANDARD);
  }

  /* --- register standard --- */

  if (SubtStandard == STSUBT_StandardNotSpecified)
  {
      /* when standard is not specified, it is set to DVB by default
       * and changed when a DGT standard is detected.
       */
      InternalHandle->SubtitleStandard = STSUBT_DVBStandard;
      InternalHandle->AutoDetection = TRUE;
  }
  else
  {
      InternalHandle->SubtitleStandard = SubtStandard;
      InternalHandle->AutoDetection = FALSE;
  }

  /* --- check status --- */

  if (InternalHandle->DecoderStatus != STSUBT_Driver_Stopped)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_SetStream: Error: Driver is Running !\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }

#ifndef PTI_EMULATION_MODE

  /* --- associated pid with assigned slot --- */

#ifdef DVD_TRANSPORT_STPTI
  if (STPTI_SlotSetPid(InternalHandle->stpti_slot,
			 (STPTI_Pid_t)SubtStream.subtitle_pid)!= ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STPTI_SlotSetPid() Error setting pid !\n"));
      InternalHandle->StreamInitialized  = FALSE;
      return (STSUBT_ERROR_NO_STREAM_SET);
  }

#endif
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK

  if (pti_set_pid(InternalHandle->pti_slot,
		 SubtStream.subtitle_pid)!=FALSE)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_SetStream: Error setting pid !\n"));
      InternalHandle->StreamInitialized  = FALSE;
      return (STSUBT_ERROR_NO_STREAM_SET);
  }

#endif
  
#endif

  /* --- enable filtering on given composition and ancillary page ids --- */

  InternalHandle->composition_page_id = SubtStream.composition_page_id;
  InternalHandle->ancillary_page_id   = SubtStream.ancillary_page_id;

  InternalHandle->StreamInitialized = TRUE;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STSUBT_SetStream: Stream set!\n"));

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: STSUBT_Start
 *
 * Purpose : Starts the subtitling decoder
 *
 * Parameters:
 *     SubtHandle:
 *           Handle of subtitling decoder
 *
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE 
 *           STSUBT_ERROR_DRIVER_RUNNING
 *           STSUBT_ERROR_NO_STREAM_SET 
 *           STSUBT_ERROR_STARTING_FILTER
 *           STSUBT_ERROR_STARTING_PROCESSOR
 *
 * Note    : The decoder must be stopped and a subtitling stream set 
 *           before starting the decoder
\******************************************************************************/

ST_ErrorCode_t STSUBT_Start (STSUBT_Handle_t SubtHandle)
{
  STSUBT_InternalHandle_t *InternalHandle;
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
  ST_ErrorCode_t ErrorCode;
#endif
#endif


  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_Start: Starting the Driver **\n"));

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  /* --- Check the handle --- */

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Start: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- Check the decoder status --- */

  if (InternalHandle->DecoderStatus != STSUBT_Driver_Stopped)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Start: Error: Driver is Running !\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }

  /* --- Check the stream --- */

  if (InternalHandle->StreamInitialized == FALSE)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Start: Stream not initialized\n"))
      return (STSUBT_ERROR_NO_STREAM_SET);
  }
 
  /* --- Reset the epoch counter --- */

  InternalHandle->EpochNumber = 0;

    /* Mediabright GK - Race condition fix, InternalHandle->CodedDataBuffer
must be reset before starting the Processor / Filter Tasks as the data flow semaphores get reinitialised!.  Any task waiting on a re-initialised semaphore will not then function correctly afterwards.  Cover Filter / Processor task error conditions in pathological situations. */
 
  /* Danger, short circuited conditional. */
  if ((InternalHandle->WorkingMode == STSUBT_NormalMode) &&
      ((InternalHandle->FilterStatus == STSUBT_TaskRunning) ||
       (InternalHandle->StreamInitialized == FALSE)))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
             "** STSUBT_Start: Error: Filter_Task is currently running **\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }
 
  if (InternalHandle->ProcessorStatus == STSUBT_TaskRunning)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
             "** STSUBT_Start: Error: Processor_Task is currently running **\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }


  /* --- Reset buffers --- */

  CompBuffer_EpochReset (InternalHandle->CompositionBuffer1);
  CompBuffer_EpochReset (InternalHandle->CompositionBuffer2);
  InternalHandle->CompositionBuffer=InternalHandle->CompositionBuffer1;
  PixelBuffer_Reset (InternalHandle->PixelBuffer2);
  PixelBuffer_Reset (InternalHandle->PixelBuffer2);
  InternalHandle->PixelBuffer=InternalHandle->PixelBuffer1;
  cBuffer_Reset(InternalHandle->CodedDataBuffer);
  fcBuffer_Reset (InternalHandle->PCSBuffer);

  /* --- Start the Filter (normal mode only) --- */
 
  if (InternalHandle->WorkingMode == STSUBT_NormalMode)
  {
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI

#if 1/*zxg move to*/
	  if((ErrorCode=STPTI_SlotLinkToBuffer(InternalHandle->stpti_slot,
				InternalHandle->stpti_buffer))!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: STPTI_SlotLinkToBuffer() Error.%d\n",
						ErrorCode));

          return (STSUBT_ERROR_STARTING_FILTER);
	  }

	  if(STPTI_SignalAssociateBuffer(InternalHandle->stpti_signal,
				InternalHandle->stpti_buffer)!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: STPTI_SignalAssociateBuffer() Error.\n"));

          return (STSUBT_ERROR_STARTING_FILTER);
	  }
#endif
	  if(STPTI_BufferFlush(InternalHandle->stpti_buffer)!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: STPTI_BufferFlush() Error.\n"));

          return (STSUBT_ERROR_STARTING_FILTER);
	  }
#endif
#endif
          
      if (Filter_Start (InternalHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: Filter not started\n"));
          return (STSUBT_ERROR_STARTING_FILTER);
      }
#ifdef DVD_TRANSPORT_LINK
    /* Start the link sending data (DDTS 11975)*/
    pti_start_slot(InternalHandle->pti_slot);
#endif
      

  }

  /* --- Provide Processor with the Display_Depth value --- */

  InternalHandle->STSUBT_Display_Depth = 8;


  /* --- Start the Processor --- */
  if (Processor_Start (InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Start: Processor not started\n"));
      return (STSUBT_ERROR_STARTING_PROCESSOR);
  }

  if (InternalHandle->CommonInterfaceEnabled == TRUE)
  {
      if (InternalHandle->ObjectCacheEnabled == TRUE)
          ObjectCache_Flush(InternalHandle->ObjectCache);
      cBuffer_Reset(InternalHandle->CI_ReplyBuffer);
      cBuffer_Reset(InternalHandle->CI_MessageBuffer);
  }

  /* --- Set new decoder status --- */
 
  InternalHandle->DecoderStatus = STSUBT_Driver_Running_But_Not_Displaying;
  InternalHandle->SubtitleDisplayEnabled = FALSE;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_Start: Driver successfully started **\n"));
 
  return (ST_NO_ERROR);
}

/******************************************************************************\
 * Function: STSUBT_Show
 * Purpose : Enables Display of subtitles on the screen
 * Parameters:
 *     SubtHandle:
 *           Handle of subtitling decoder
 *     DisplayService:
 *           Predefined Display Service
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
 *           STSUBT_ERROR_DRIVER_DISPLAYING
 *           STSUBT_ERROR_UNKNOWN_DISPLAY_SERVICE
 *           STSUBT_ERROR_STARTING_ENGINE	(by EnablePresentation)
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_Show (STSUBT_Handle_t SubtHandle,
                            STSUBT_ShowParams_t ShowParams)
{
	return(STSUBT_OutStart(SubtHandle,ShowParams));
}

/******************************************************************************\
 * Function: STSUBT_OutStart
 * Purpose : Enables Output of subtitles on Disply Service
 * Parameters:
 *     SubtHandle:
 *           Handle of subtitling decoder
 *     DisplayService:
 *           Predefined Display Service
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
 *           STSUBT_ERROR_DRIVER_DISPLAYING
 *           STSUBT_ERROR_UNKNOWN_DISPLAY_SERVICE
 *           STSUBT_ERROR_STARTING_ENGINE	(by EnablePresentation)
 *           STSUBT_ERROR_STARTING_TIMER	(by EnablePresentation)
 *           STSUBT_ERROR_INIT_DISPLAY_SERVICE	(by EnablePresentation)
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_OutStart (STSUBT_Handle_t SubtHandle,
                            STSUBT_ShowParams_t ShowParams)
{
  ST_ErrorCode_t res;
  STSUBT_InternalHandle_t *InternalHandle;
  STSUBT_DisplayServiceParams_t DisplayServiceParams;
  STSUBT_DisplayServiceName_t DisplayServiceName = 
                              ShowParams.DisplayServiceName;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_OutStart: Enabling Display **\n"));
 
  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_OutStart: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- Driver must be Running before displaying --- */
 
  if (InternalHandle->DecoderStatus == STSUBT_Driver_Running_And_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_OutStart: Driver already displaying !\n"));
      return (STSUBT_ERROR_DRIVER_DISPLAYING);
  }

  if (InternalHandle->DecoderStatus == STSUBT_Driver_Stopped)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_OutStart: Driver not Running !\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  /* --- Match display service to built-in structure --- */

  switch (DisplayServiceName) {
    case STSUBT_NULL_DisplayService :
         InternalHandle->DisplayService = (STSUBT_DisplayService_t *)
                                            &STSUBT_NULL_DisplayServiceStructure; 
         break;
#ifndef DISABLE_OSD_DISPLAY_SERVICE
   /* case STSUBT_OSD_DisplayService :
         InternalHandle->DisplayService = (STSUBT_DisplayService_t *)
                                            &STSUBT_OSD_DisplayServiceStructure; */
         break;
#endif
#ifdef TEST_DISPLAY_SERVICE
    case STSUBT_TEST_DisplayService :
         InternalHandle->DisplayService = (STSUBT_DisplayService_t *)
                                            &STSUBT_TEST_DisplayServiceStructure;
         break;
#endif
#ifdef USER_DISPLAY_SERVICE
    case STSUBT_UserDefined_DisplayService :
         InternalHandle->DisplayService = (STSUBT_DisplayService_t *)
                                            &STSUBT_UserDefined_DisplayServiceStructure;
         break;
#endif
#ifdef RDE_DISPLAY_SERVICE
    case STSUBT_RDE_DisplayService :
         InternalHandle->DisplayService = (STSUBT_DisplayService_t *)
                                            &STSUBT_RDE_DisplayServiceStructure;
         break;
#endif
    default:
         STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_OutStart: Unknown Display Service!\n"));
         return (STSUBT_ERROR_UNKNOWN_DISPLAY_SERVICE);
  }

  /* --- Reset PCS buffer --- */
 
  fcBuffer_Reset (InternalHandle->PCSBuffer);

  /* --- Enable Display --- */

  DisplayServiceParams.RDEBuffer_p=ShowParams.RDEBuffer_p;
  DisplayServiceParams.RDEBufferSize=ShowParams.RDEBufferSize;
 
  res = EnablePresentation (InternalHandle,&DisplayServiceParams); 

  if (res != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_OutStart: error enabling display\n"));
      return (res);
  }
 
  /* --- Set new decoder status --- */
 
  InternalHandle->DecoderStatus = STSUBT_Driver_Running_And_Displaying;
 
  /* --- Enable display of subtitles --- */

  InternalHandle->SubtitleDisplayEnabled = TRUE;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_OutStart: Display successfully Enabled **\n"));
 
  return (ST_NO_ERROR);
} 


/******************************************************************************\
 * Function: STSUBT_Hide
 * Purpose : Disables Display of subtitles
 * Parameters:
 *     SubtHandle:
 *           Handle of subtitling decoder
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_DRIVER_NOT_DISPLAYING
 *           STSUBT_ERROR_STOPPING_ENGINE	(by DisablePresentation)
 *           STSUBT_ERROR_STOPPING_TIMER	(by DisablePresentation)
 *           STSUBT_ERROR_TERM_DISPLAY_SERVICE	(by DisablePresentation)
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_Hide ( STSUBT_Handle_t SubtHandle )
{
	return(STSUBT_OutStop (SubtHandle));
}

/******************************************************************************\
 * Function: STSUBT_OutStop
 * Purpose : Disables Output of subtitles on Display Service
 * Parameters:
 *     SubtHandle:
 *           Handle of subtitling decoder
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_DRIVER_NOT_DISPLAYING
 *           STSUBT_ERROR_STOPPING_ENGINE	(by DisablePresentation)
 *           STSUBT_ERROR_STOPPING_TIMER	(by DisablePresentation)
 *           STSUBT_ERROR_TERM_DISPLAY_SERVICE	(by DisablePresentation)
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_OutStop ( STSUBT_Handle_t SubtHandle )
{
  ST_ErrorCode_t res;
  STSUBT_InternalHandle_t *InternalHandle;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_OutStop: disabling Display **\n"));
 
  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_OutStop: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- Driver must be displaying when disabling --- */
 
  if (InternalHandle->DecoderStatus != STSUBT_Driver_Running_And_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_OutStop: Driver not displaying !\n"));
      return (STSUBT_ERROR_DRIVER_NOT_DISPLAYING);
  }
 
  /* --- disable Presentation of subtitles --- */
 
  InternalHandle->SubtitleDisplayEnabled = FALSE;

  res = DisablePresentation (InternalHandle);
  if (res != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_OutStop: error disabling Presentation\n"));
      return (res);
  }
 
  /* --- Set new decoder status --- */
 
  InternalHandle->DecoderStatus = STSUBT_Driver_Running_But_Not_Displaying;

  InternalHandle->SubtitleDisplayEnabled = FALSE;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
 			"** STSUBT_OutStop: Display successfully Disablied **\n"));
 
  return (ST_NO_ERROR);
}
 

/******************************************************************************\
 * Function: STSUBT_Stop
 * Purpose : Stops the subtitling decoder
 * Parameters:
 *     SubtHandle:
 *           Handle of the subtitling decoder
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
 *           STSUBT_ERROR_DRIVER_DISPLAYING
 *           STSUBT_ERROR_STOPPING_FILTER
 *           STSUBT_ERROR_STOPPING_PROCESSOR
 * Note    : The decoder must be in Running_But_Not_Displaying status  
\******************************************************************************/

ST_ErrorCode_t STSUBT_Stop (STSUBT_Handle_t SubtHandle )
{
  STSUBT_InternalHandle_t *InternalHandle;
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
  ST_ErrorCode_t  ErrorCode;
#endif
#endif
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_Stop: Stopping the Driver **\n"));
 
  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Stop: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- Driver must be Running_But_Not_Displaying when stopping it --- */
 
  if (InternalHandle->DecoderStatus == STSUBT_Driver_Stopped)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Stop: Driver already Stopped !\n"));
      return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
  }

  if (InternalHandle->DecoderStatus == STSUBT_Driver_Running_And_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Stop: Error: Driver is displaying!\n"));
      return (STSUBT_ERROR_DRIVER_DISPLAYING);
  }

  /* --- Stop the Filter (normal mode only) --- */

  if (InternalHandle->WorkingMode == STSUBT_NormalMode)
  {
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
	  if(STPTI_SlotClearPid(InternalHandle->stpti_slot)!= ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_SlotClearPid() Failed Clear Pid\n"));
	  } 
	  if(STPTI_SignalDisassociateBuffer(InternalHandle->stpti_signal,
				InternalHandle->stpti_buffer)!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Start: STPTI_SignalDisassociateBuffer() Error 0x%x.\n",
							ErrorCode));
          return (STSUBT_ERROR_STARTING_FILTER);
      }	
	  if((ErrorCode=STPTI_SlotUnLink(InternalHandle->stpti_slot))!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Start: STPTI_SlotUnLink() Error 0x%x.\n",ErrorCode));

          return (STSUBT_ERROR_STARTING_FILTER);
	  }

#endif
#endif
#ifdef DVD_TRANSPORT_LINK
      /* Stop slot. DDTS 11975 */
      pti_stop_slot(InternalHandle->pti_slot);
#endif          
      if (Filter_Stop (InternalHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_Stop: Filter not stopped\n"));
          return (STSUBT_ERROR_STOPPING_FILTER);
      }

     
  }
 
  /* --- Stop the Processor --- */
 
  if (Processor_Stop (InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Stop: Processor not stopped\n"));
      return (STSUBT_ERROR_STOPPING_PROCESSOR);
  }
 
  /* --- Set new decoder status --- */
 
  InternalHandle->DecoderStatus = STSUBT_Driver_Stopped;
 
#ifdef PTI_EMULATION_MODE
  /* decoder may be restarted after stopped, 
   * so, when testing, we have to reset the pti-emulation buffer
   */
 
  if (InternalHandle->WorkingMode == STSUBT_NormalMode)
  {
      cBuffer_Reset(InternalHandle->pti_buffer);
  }
#endif

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"** STSUBT_Stop: Driver successfully stopped **\n"));
 
  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: STSUBT_Close
 * Purpose : Closes a subtitling handle
 * Parameters:
 *     SubtHandle:
 *           Handle of subtitling decoder
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_DRIVER_RUNNING
 *           STSUBT_ERROR_EVENT_UNREGISTER
 *           STSUBT_ERROR_CLOSING_STEVT
 *           STSUBT_ERROR_CLOSING_STCLKRV
 * Note    : The decoder must be in Stopped status before closing
\******************************************************************************/

ST_ErrorCode_t STSUBT_Close (STSUBT_Handle_t SubtHandle)
{
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
  ST_ErrorCode_t ErrorCode;
#endif
#endif
  STSUBT_InternalHandle_t *InternalHandle;
  U32 i;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"STSUBT_Close: Closing the Handle !\n"));

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Close: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- Driver must be Stopped when removing it --- */

  if (InternalHandle->DecoderStatus != STSUBT_Driver_Stopped)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Close: Decoder still running !\n"));
      return (STSUBT_ERROR_DRIVER_RUNNING);
  }

  /* --- Unregister Events --- */
 
  for (i = 0 ; i < STSUBT_NUM_EVENTS; i++)
  {
      if (STEVT_UnregisterDeviceEvent(InternalHandle->stevt_handle,
		  STSUBT_Dictionary[InternalHandle->DeviceInstance].DeviceName,
				 STSUBT_DRIVER_BASE+i) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Close: Error unregistering event\n"));
          return(STSUBT_ERROR_EVENT_UNREGISTER);
      }
  }


  /* --- close stclkrv handle and clear pid (normal mode only) --- */

  if (InternalHandle->WorkingMode == STSUBT_NormalMode)
  {
      /* --- stop pti capture on the slot -- */
#ifndef PTI_EMULATION_MODE
#ifdef DVD_TRANSPORT_STPTI
	  if(STPTI_SlotClearPid(InternalHandle->stpti_slot)!= ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_SlotClearPid() Failed Clear Pid\n"));
	  } 

	  if((ErrorCode=STPTI_SignalDeallocate(InternalHandle->stpti_signal))
				!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_SignalDeallocate() 0x%x\n",ErrorCode));
          return STSUBT_ERROR_CLOSING_STPTI;
	  }

	  if((ErrorCode=STPTI_BufferDeallocate(InternalHandle->stpti_buffer))
				 != ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_BufferDeallocate() 0x%x\n",ErrorCode));
          return STSUBT_ERROR_CLOSING_STPTI;
	  }

	  if((ErrorCode=STPTI_SlotDeallocate(InternalHandle->stpti_slot))
				!=ST_NO_ERROR)
	  {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STPTI_SlotDeallocate() 0x%x\n",ErrorCode));
          return STSUBT_ERROR_CLOSING_STPTI;
	  }

      if((ErrorCode=STPTI_Close(InternalHandle->stpti_handle))
				!=ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Open: Error STPTI_Close() 0x%x\n",ErrorCode));
          return STSUBT_ERROR_CLOSING_STPTI;
      }
#endif
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
      pti_clear_pid(InternalHandle->pti_slot);
#endif
#endif
      if (STCLKRV_Close(InternalHandle->stclkrv_handle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Close: Failed closing Clock Recovery\n"));
          return(STSUBT_ERROR_CLOSING_STCLKRV);
      }
  }

  /* --- close stevt driver handle --- */
 
  if (STEVT_Close(InternalHandle->stevt_handle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Close: Failed closing Event Handler\n"));
      return(STSUBT_ERROR_CLOSING_STEVT);
  }

  /* --- invalidate the handle --- */

  InternalHandle->MagicNumber = 0x00000000;
  
  STSUBT_Dictionary[InternalHandle->DeviceInstance].HandleIsOpen = FALSE;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
					"STSUBT_Close: Handle Closed !\n"));

  return(ST_NO_ERROR);
}



/******************************************************************************\
 * Function: STSUBT_Term
 * Purpose : Removes instance of subtitling decoder;
 *           All allocated resources are released.
 * Parameters:
 *     DeviceName:
 *           Device name
 * Return  : ST_NO_ERROR on success, else ...
 *           ST_ERROR_UNKNOWN_DEVICE
 *           STSUBT_ERROR_OPEN_HANDLE
 *           STSUBT_ERROR_DELETING_FILTER
 *           STSUBT_ERROR_DELETING_PROCESSOR
 *           STSUBT_ERROR_DELETING_ENGINE
 *           STSUBT_ERROR_DELETING_TIMER
 * Note    : The decoder must be in Stopped status before closing
\******************************************************************************/

ST_ErrorCode_t STSUBT_Term (ST_DeviceName_t DeviceName, STSUBT_TermParams_t *TermParams)
{

  ST_ErrorCode_t res;
  U32            DeviceInstance;
  U32            i;

  STSUBT_InternalHandle_t *InternalHandle;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"STSUBT_Term: Removing instance of subtitling Decoder !\n"));

  /* --- Retrieve handle associated to the specified DeviceName --- */
 
  InternalHandle = NULL;
 
  for (i = 0; i < STSUBT_NumInstances; i++)
  {
      if (strcmp(STSUBT_Dictionary[i].DeviceName, DeviceName) == 0)
      {
          InternalHandle =
              (STSUBT_InternalHandle_t*)STSUBT_Dictionary[i].DecoderHandle;
          break;
      }
  }
 
  if (InternalHandle == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Term: Invalid Device Name\n"));
      return (ST_ERROR_UNKNOWN_DEVICE);
  }

  /* --- get current instance number --- */
  
  DeviceInstance = InternalHandle->DeviceInstance;

  /* --- check handle --- */

  if (STSUBT_Dictionary[DeviceInstance].HandleIsOpen == TRUE)
  {
		if (TermParams->ForceTerminate == TRUE)
		{
			switch (InternalHandle->DecoderStatus)
			{
  				case STSUBT_Driver_Suspended:
  				/* Falling down case */
               res=Processor_Resume(InternalHandle);
               if (res != ST_NO_ERROR)
               {
                  STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_Term: Processor_Resume 0x%08X!\n",res));
                  return (res);
               }
               InternalHandle->DecoderStatus = 
                       STSUBT_Driver_Running_And_Displaying;

  				case STSUBT_Driver_Running_And_Displaying:
				/* Falling down case */
					res=STSUBT_OutStop((STSUBT_Handle_t)(InternalHandle));
					if (res != ST_NO_ERROR)
         		{
      				STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_Term: hide handle 0x%08X!\n",res));
            		return (res);
         		}

  				case STSUBT_Driver_Running_But_Not_Displaying:
				/* Falling down case */
					res=STSUBT_Stop((STSUBT_Handle_t)(InternalHandle));
					if (res != ST_NO_ERROR)
         		{
      				STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_Term: stop handle 0x%08X!\n",res));
            		return (res);
         		}

  				case STSUBT_Driver_Stopped:
					res=STSUBT_Close((STSUBT_Handle_t )(InternalHandle));
					if (res != ST_NO_ERROR)
         		{
      				STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_Term: close handle 0x%08X!\n",res));
            		return (res);
         		}
					break;

			}
		}
		else
		{
      	STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_Term: Handle is still open!\n"));
      	return (STSUBT_ERROR_OPEN_HANDLE);
		}
  }

  /* --- delete the Filter --- */
 
  if (Filter_Delete (InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Term: cannot delete Filter !\n"));
      return (STSUBT_ERROR_DELETING_FILTER);
  }
  
  /* --- delete the Processor --- */
 
  if (Processor_Delete (InternalHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Term: cannot delete Processor !\n"));
      return (STSUBT_ERROR_DELETING_PROCESSOR);
  }
  
  /* --- terminate Display Controller --- */
 
  res = TermDisplayController(InternalHandle);
  if (res != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_Term: cannot terminate Display Controller!\n"));
      return (res);
  }
  

#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  /* --- delete pti semaphore --- */

  semaphore_delete (InternalHandle->pti_semaphore);  
  memory_deallocate (InternalHandle->MemoryPartition, 
                     InternalHandle->pti_semaphore);
#endif

  /* --- delete buffers --- */
 
  cBuffer_Delete (InternalHandle->CodedDataBuffer);
  CompBuffer_Delete (InternalHandle->CompositionBuffer1);
  CompBuffer_Delete (InternalHandle->CompositionBuffer2);
  PixelBuffer_Delete (InternalHandle->PixelBuffer1);
  PixelBuffer_Delete (InternalHandle->PixelBuffer2);
  fcBuffer_Delete(InternalHandle->PCSBuffer);
 
  /* --- remove CI resources --- */

  if (InternalHandle->CommonInterfaceEnabled == TRUE)
  {
      cBuffer_Delete (InternalHandle->CI_ReplyBuffer);
      cBuffer_Delete (InternalHandle->CI_MessageBuffer);
      memory_deallocate (InternalHandle->MemoryPartition,
                         InternalHandle->CI_WorkingBuffer);
      memory_deallocate (InternalHandle->MemoryPartition, 
                         InternalHandle->ArrayPCS);

      if (InternalHandle->ObjectCacheEnabled == TRUE)
          ObjectCache_Delete(InternalHandle->ObjectCache);
  }

  /* --- delete decoder handle --- */

  memory_deallocate (InternalHandle->MemoryPartition, InternalHandle);

  /* --- reset instance entry --- */

  strcpy(STSUBT_Dictionary[DeviceInstance].DeviceName, "");
  STSUBT_Dictionary[DeviceInstance].DecoderHandle = 0;
  STSUBT_Dictionary[DeviceInstance].HandleIsOpen = FALSE;

  /* --- decrement instances counter --- */
  
  STSUBT_NumInstances --;

  /* --- make room in the dictionary --- */

  if (DeviceInstance < STSUBT_NumInstances)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"STSUBT_Term: making room in the dictionary!\n"));

      STSUBT_Dictionary[DeviceInstance] = 
          STSUBT_Dictionary[STSUBT_NumInstances];
      
      /* --- update DeviceInstance in the handle of moved instance --- */
    
      InternalHandle = (STSUBT_InternalHandle_t*)
			STSUBT_Dictionary[DeviceInstance].DecoderHandle;
     
      InternalHandle->DeviceInstance = DeviceInstance;
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"STSUBT_Term: Instance removed !\n"));

  return(ST_NO_ERROR);
}


/* -------------------------------------------------------------------------- */
/* --- Extra functions for the Enhanced Common Interface -------------------- */
/* -------------------------------------------------------------------------- */


/******************************************************************************\
 * Function: STSUBT_InsertPacket
 * Purpose : Notify APDU packets to the decoder
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_INVALID_WORKING_MODE
 *           STSUBT_ERROR_DRIVER_NOT_DISPLAYING
 *           STSUBT_ERROR_DRIVER_NOT_RUNNING
 *           STSUBT_ERROR_INVALID_APDU
 *           STSUBT_ERROR_UNKNOWN_APDU
 *           STSUBT_ERROR_UNEXPECTED_APDU
 *           STSUBT_ERROR_LOST_APDU_PACKET
\******************************************************************************/

ST_ErrorCode_t STSUBT_InsertPacket (
                      STSUBT_Handle_t SubtHandle,
                      U32             PacketTag,
                      U32             PacketLength,
                      void           *PacketData)
{
  STSUBT_InternalHandle_t *InternalHandle;
  ST_ErrorCode_t ans, res = ST_NO_ERROR;
  U8 packet_status, reset_flag;

#ifdef  STTBX_REPORT
	switch(PacketTag)
	{
		case STSUBT_SUBTITLE_SEGMENT_LAST_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SUBTITLE_SEGMENT_LAST_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_SUBTITLE_SEGMENT_MORE_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SUBTITLE_SEGMENT_MORE_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_DISPLAY_MESSAGE_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_DISPLAY_MESSAGE_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_SCENE_END_MARK_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SCENE_END_MARK_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_SCENE_DONE_MESSAGE_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SCENE_DONE_MESSAGE_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_SCENE_CONTROL_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SCENE_CONTROL_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_SUBTITLE_DOWNLOAD_LAST_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SUBTITLE_DOWNLOAD_LAST_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_SUBTITLE_DOWNLOAD_MORE_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_SUBTITLE_DOWNLOAD_MORE_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_FLUSH_DOWNLOAD_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_FLUSH_DOWNLOAD_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_DOWNLOAD_REPLY_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STSUBT_DOWNLOAD_REPLY_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_DECODER_PAUSE_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STSUBT_DECODER_PAUSE_TAG, Len=%d\n",PacketLength));
			break;
		case STSUBT_DECODER_RESUME_TAG:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STSUBT_DECODER_RESUME_TAG, Len=%d\n",PacketLength));
			break;

		default:
			STTBX_Report((STTBX_REPORT_LEVEL_INFO,"Unknow Tag, Len=%d\n",PacketLength));
	}
#endif


  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  /* --- check handle --- */

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_InsertPacket: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- check the working mode --- */

  if (InternalHandle->WorkingMode != STSUBT_OverlayMode)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_InsertPacket: Error: Invalid working mode!\n"));
      return (STSUBT_ERROR_INVALID_WORKING_MODE);
  }

  /* --- Check status: decoder must be running and displaying --- */

  if (InternalHandle->DecoderStatus != STSUBT_Driver_Running_And_Displaying)
  {
      if (InternalHandle->DecoderStatus != STSUBT_Driver_Suspended)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Decoder not displaying !\n"));
          return (STSUBT_ERROR_DRIVER_NOT_DISPLAYING);
      }
  }

  /* --- Check APDU tag --- */

  switch (PacketTag) 
  {
      /* these are output APDUs and cannot be sent to the decoder */
      case STSUBT_SCENE_DONE_MESSAGE_TAG:
      case STSUBT_DOWNLOAD_REPLY_TAG:
      case STSUBT_DISPLAY_MESSAGE_TAG:
           STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_InsertPacket: invalid APDU received!\n"));
           return (STSUBT_ERROR_INVALID_APDU);
   
      case STSUBT_FLUSH_DOWNLOAD_TAG:
           if (InternalHandle->ObjectCacheEnabled == FALSE) 
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Object Cache not enabled!\n"));
               return (STSUBT_ERROR_OBJECT_CACHE_NOT_ENABLED);
           }

      case STSUBT_SCENE_END_MARK_TAG:
           reset_flag = CBUFFER_FALSE;
           if (InternalHandle->ExpectingStatus != STSUBT_expecting_any)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: unexpected APDU received!\n"));
               reset_flag = CBUFFER_TRUE;
               res = STSUBT_ERROR_UNEXPECTED_APDU;
           }
           packet_status = CBUFFER_SINGLE;
           break;

      /* check if decoder is suspended and decoder_continue_flag = 1 */
      case STSUBT_SCENE_CONTROL_TAG:
           reset_flag = CBUFFER_FALSE;
           if (InternalHandle->ExpectingStatus != STSUBT_expecting_any)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: unexpected APDU received!\n"));
               reset_flag = CBUFFER_TRUE;
               res = STSUBT_ERROR_UNEXPECTED_APDU;
           }
           /* check decoder_continue_flag (only if decoder is suspended) */
           if (InternalHandle->DecoderStatus == STSUBT_Driver_Suspended)
           {
               U8 decoder_continue_flag = (*(U8*)PacketData & 0x80) >> 7;
               if (decoder_continue_flag == 1)
               {
                   /* resume decoding process */
                   if (Processor_Resume(InternalHandle) != ST_NO_ERROR)
                   {
                       STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
							"STSUBT_InsertPacket: Decoder Not Resumed!"));
                       return (STSUBT_ERROR_DRIVER_NOT_RESUMED);
                   }
                   /* update decoder status */
                   InternalHandle->DecoderStatus = 
                       STSUBT_Driver_Running_And_Displaying;
               }
           }
           packet_status = CBUFFER_SINGLE;
           break;

      /* suspend decoding process */
      case STSUBT_DECODER_PAUSE_TAG:
           if (InternalHandle->DecoderStatus == STSUBT_Driver_Suspended)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Decoder is already Suspended"));
               return (STSUBT_ERROR_DRIVER_NOT_RUNNING);
           }
           /* The APDU is inserted in the Coded Data Buffer since 
            * the Processor task may be suspended in semaphore queue
            * waiting for packets to be inserted.
            */
           reset_flag = CBUFFER_FALSE;
           packet_status = CBUFFER_SINGLE;
           PacketLength = 0;
           PacketData = NULL;
           break;

      /* resume the decoding process */
      case STSUBT_DECODER_RESUME_TAG:
           if (InternalHandle->DecoderStatus != STSUBT_Driver_Suspended)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Decoder is Not Suspended!\n"));
               return (STSUBT_ERROR_DRIVER_NOT_SUSPENDED);
           }
           if (Processor_Resume(InternalHandle) != ST_NO_ERROR)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Decoder Not Resumed!\n"));
               return (STSUBT_ERROR_DRIVER_NOT_RESUMED);
           }
           /* update decoder status */
           InternalHandle->DecoderStatus =
								 STSUBT_Driver_Running_And_Displaying;
           /* No need to insert this APDU in the Coded Data Buffer */ 
           return (ST_NO_ERROR);

      case STSUBT_SUBTITLE_SEGMENT_MORE_TAG:
           reset_flag = CBUFFER_FALSE;
           if (InternalHandle->ExpectingStatus == 
                       STSUBT_expecting_subtitle_download)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: unexpected APDU received!\n"));
               reset_flag = CBUFFER_TRUE;
               res = STSUBT_ERROR_UNEXPECTED_APDU;
           }
           InternalHandle->ExpectingStatus =
								 STSUBT_expecting_subtitle_segment;
           packet_status = CBUFFER_MORE;
           break;

      case STSUBT_SUBTITLE_SEGMENT_LAST_TAG:
           reset_flag = CBUFFER_FALSE;
           if (InternalHandle->ExpectingStatus == 
                       STSUBT_expecting_subtitle_download)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: unexpected APDU received!\n"));
               reset_flag = CBUFFER_TRUE;
               res = STSUBT_ERROR_UNEXPECTED_APDU;
           }
           else if (InternalHandle->ExpectingStatus == STSUBT_expecting_any)
                packet_status = CBUFFER_SINGLE;
           else packet_status = CBUFFER_LAST;
           InternalHandle->ExpectingStatus = STSUBT_expecting_any;
           break;

      case STSUBT_SUBTITLE_DOWNLOAD_MORE_TAG:
           if (InternalHandle->ObjectCacheEnabled == FALSE) 
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Object Cache not enabled!\n"));
               return (STSUBT_ERROR_OBJECT_CACHE_NOT_ENABLED);
           }
           reset_flag = CBUFFER_FALSE;
           if (InternalHandle->ExpectingStatus == 
                       STSUBT_expecting_subtitle_segment)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: unexpected APDU received!\n"));
               reset_flag = CBUFFER_TRUE;
               res = STSUBT_ERROR_UNEXPECTED_APDU;
           }
           InternalHandle->ExpectingStatus =
						 STSUBT_expecting_subtitle_download;
           packet_status = CBUFFER_MORE;
           break;

      case STSUBT_SUBTITLE_DOWNLOAD_LAST_TAG:
           if (InternalHandle->ObjectCacheEnabled == FALSE) 
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Object Cache not enabled!\n"));
               return (STSUBT_ERROR_OBJECT_CACHE_NOT_ENABLED);
           }
           reset_flag = CBUFFER_FALSE;
           if (InternalHandle->ExpectingStatus ==
                       STSUBT_expecting_subtitle_segment)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: unexpected APDU received!\n"));
               reset_flag = CBUFFER_TRUE;
               res = STSUBT_ERROR_UNEXPECTED_APDU;
           }
           else if (InternalHandle->ExpectingStatus == STSUBT_expecting_any)
                packet_status = CBUFFER_SINGLE;
           else packet_status = CBUFFER_LAST;
           InternalHandle->ExpectingStatus = STSUBT_expecting_any;
           break;

      default:
           STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_InsertPacket: unknown APDU received!\n"));
           return (STSUBT_ERROR_UNKNOWN_APDU);
  }

  /* --- insert APDU in the Coded Data Buffer --- */

  ans = cBuffer_InsertAPDU (InternalHandle->CodedDataBuffer,
                            PacketTag, PacketLength, PacketData, 
                            packet_status, reset_flag);

  if (ans != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"STSUBT_InsertPacket: error inserting APDU packet\n"));
      return (STSUBT_ERROR_LOST_APDU_PACKET);
  }

  /* --- suspend the decoder if required --- */

  if (PacketTag == STSUBT_DECODER_PAUSE_TAG)
  {
      if (Processor_Pause(InternalHandle) != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_InsertPacket: Decoder Not Suspended!\n"));
          return (STSUBT_ERROR_DRIVER_NOT_SUSPENDED);
      }
      /* set decoder status to suspend */
      InternalHandle->DecoderStatus = STSUBT_Driver_Suspended;
  }

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_GetReplyPacket
 * Purpose : Get APDU reply packets from the decoder
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
\******************************************************************************/

ST_ErrorCode_t STSUBT_GetReplyPacket (
                      STSUBT_Handle_t  SubtHandle,
                      U32             *PacketTag,
                      U32             *PacketLength,
                      U32             *PacketData,
                      STSUBT_CallingMode_t Mode)
{
  STSUBT_InternalHandle_t *InternalHandle;
  ST_ErrorCode_t res;

  *PacketTag = 0;
  *PacketLength = 0;
  *PacketData = 0;

  /* --- check handle --- */

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
						"STSUBT_GetReplyPacket: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- check the working mode --- */

  if (InternalHandle->WorkingMode != STSUBT_OverlayMode)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_GetReplyPacket: Error: Invalid working mode!\n"));
      return (STSUBT_ERROR_INVALID_WORKING_MODE);
  }

  /* --- Check status: decoder must be running and displaying --- */

  if (InternalHandle->DecoderStatus != STSUBT_Driver_Running_And_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
					"STSUBT_GetReplyPacket: Decoder not displaying !\n"));
      return (STSUBT_ERROR_DRIVER_NOT_DISPLAYING);
  }

  /* --- check the calling mode, and get packet --- */

  switch (Mode)
  {
      case STSUBT_NonBlocking:
           res = cBuffer_GetReplyAPDU (InternalHandle->CI_ReplyBuffer,
                                       PacketTag, PacketLength, PacketData);
           if (res == CBUFFER_ERROR_NO_PACKET_AVAILABLE)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"STSUBT_GetReplyPacket: No APDU available!\n"));
               return (STSUBT_ERROR_NO_PACKET_AVAILABLE);
           }
           if (res == CBUFFER_ERROR_PACKET_OVERWRITTEN)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"STSUBT_GetReplyPacket: APDU overwritten!\n"));
               return (STSUBT_ERROR_LOST_APDU_PACKET);
           }
           break;

      case STSUBT_Blocking:
           res = cBuffer_CopyAPDU (InternalHandle->CI_ReplyBuffer,
                                   PacketTag, PacketLength, PacketData, 4);
           if (res == CBUFFER_ERROR_PACKET_OVERWRITTEN)
           {
               STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
						"STSUBT_GetReplyPacket: APDU overwritten!\n"));
               return (STSUBT_ERROR_LOST_APDU_PACKET);
           }
           break;

      default:
           STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_GetReplyPacket: Invalid blocking mode!\n"));
           return (STSUBT_ERROR_INVALID_CALL_MODE);
  }

  return(ST_NO_ERROR);
}


/******************************************************************************\
 * Function: STSUBT_GetMessagePacket
 * Purpose : Get message APDU packets from the decoder
 * Parameters:
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_INVALID_HANDLE
 *           STSUBT_ERROR_INVALID_WORKING_MODE
 *           STSUBT_ERROR_DRIVER_NOT_DISPLAYING
 *           STSUBT_ERROR_NO_PACKET_AVAILABLE
 *           STSUBT_ERROR_LOST_APDU_PACKET
\******************************************************************************/

ST_ErrorCode_t STSUBT_GetMessagePacket (
                      STSUBT_Handle_t  SubtHandle,
                      U32             *PacketTag,
                      U32             *PacketLength,
                      U32             *PacketData)
{
  ST_ErrorCode_t res;
  STSUBT_InternalHandle_t *InternalHandle;

  *PacketTag = 0;
  *PacketLength = 0;
  *PacketData = 0;

  /* --- check handle --- */

  InternalHandle = (STSUBT_InternalHandle_t*)SubtHandle;

  if (!IS_VALID_STSUBT_HANDLE(InternalHandle))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_GetMessagePacket: Invalid Handle!\n"));
      return (STSUBT_ERROR_INVALID_HANDLE);
  }

  /* --- check the working mode --- */

  if (InternalHandle->WorkingMode != STSUBT_OverlayMode)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"STSUBT_GetMessagePacket: Error: Invalid working mode!\n"));
      return (STSUBT_ERROR_INVALID_WORKING_MODE);
  }

  /* --- Check status: decoder must be running and displaying --- */

  if (InternalHandle->DecoderStatus != STSUBT_Driver_Running_And_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"STSUBT_GetMessagePacket: Decoder not displaying !\n"));
      return (STSUBT_ERROR_DRIVER_NOT_DISPLAYING);
  }

  /* --- get APDU from the ci message buffer --- */

  res = cBuffer_GetReplyAPDU (InternalHandle->CI_MessageBuffer,
                              PacketTag, PacketLength, PacketData);
  
  if (res == CBUFFER_ERROR_NO_PACKET_AVAILABLE)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,
				"STSUBT_GetMessagePacket: No APDU available!\n"));
      return (STSUBT_ERROR_NO_PACKET_AVAILABLE);
  }

  if (res == CBUFFER_ERROR_PACKET_OVERWRITTEN)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
				"STSUBT_GetMessagePacket: APDU overwritten!\n"));
      return (STSUBT_ERROR_LOST_APDU_PACKET);
  }

  return(ST_NO_ERROR);
}

