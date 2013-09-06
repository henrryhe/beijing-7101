/******************************************************************************\
 * File Name : testcb.c
 *
 * Description:
 *     This module provides an implementation of presentation callback 
 *     functions which use the TEST display device
 *
 *     See SDD document for more details.
 * 
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Marino Congiu - Jan 1999
 *
\******************************************************************************/
 
#include <stdlib.h>

#include <stddefs.h>
#include <stlite.h>

#include <subtdev.h>
#include <compbuf.h>

#ifdef TEST_DISPLAY_SERVICE

#include "dispserv.h"
#include "testcb.h"
 
/* ------------------------------------------------- */
/* --- Predefined TEST Display Service Structure --- */
/* ------------------------------------------------- */

const STSUBT_DisplayService_t STSUBT_TEST_DisplayServiceStructure = {
  STSUBT_TEST_InitializeService,
  STSUBT_TEST_PrepareDisplay,
  STSUBT_TEST_ShowDisplay,
  STSUBT_TEST_HideDisplay,
  STSUBT_TEST_TerminateService,
} ;


/* service handle for testing */

STSUBT_TEST_ServiceHandle_t *TestServiceHandle;

 
/******************************************************************************\
 * Function: NewServiceHandle
 * Purpose : Create a displaying service handle
 * Return  : A pointer to created Service Handle structure, NULL on error
\******************************************************************************/
 
static __inline
STSUBT_TEST_ServiceHandle_t *NewServiceHandle (ST_Partition_t *MemoryPartition)
{
    semaphore_t *DisplaySemaphore;

    STSUBT_TEST_ServiceHandle_t *ServiceHandle;
 
    /* Alloc memory for the handle and initialize structure  */
 
    ServiceHandle = (STSUBT_TEST_ServiceHandle_t*)
                     memory_allocate(MemoryPartition,
                                     sizeof(STSUBT_TEST_ServiceHandle_t));
    if (ServiceHandle == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** NewServiceHandle: no space **"));
        return (NULL);
    }
 
    /* --- Allocate DisplaySemaphore --- */
 
    DisplaySemaphore = memory_allocate(MemoryPartition, sizeof(semaphore_t));
    if (DisplaySemaphore == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** NewServiceHandle: error creating semaphore **"));
        memory_deallocate(MemoryPartition, ServiceHandle);
        return(NULL);
    }
    semaphore_init_fifo(DisplaySemaphore, 1);

    /* initialize the Service Handle structure */
 
    ServiceHandle->Starting = TRUE;
    ServiceHandle->MemoryPartition = system_partition;
    ServiceHandle->nEpoch = 0;
    ServiceHandle->nDisplay = 0;
    ServiceHandle->DisplaySemaphore = DisplaySemaphore;
    ServiceHandle->ActiveDisplayDescriptor = NULL;
    ServiceHandle->EpochRegionList = NULL;
    ServiceHandle->DisplayList = NULL;
    ServiceHandle->DisplayListTail = NULL;

    return (ServiceHandle);
}


/******************************************************************************\
 * Function: ResetServiceHandle
 * Purpose : Reset the contents of descriptor buffer
\******************************************************************************/
 
static __inline
void ResetServiceHandle (STSUBT_TEST_ServiceHandle_t *ServiceHandle)
{
    ServiceHandle->nEpoch ++;
    ServiceHandle->nDisplay = 0;
    ServiceHandle->EpochRegionList = NULL;
    ServiceHandle->ActiveDisplayDescriptor = NULL;
}
 

/******************************************************************************\
 * Function: NewDisplayDescriptor
 * Purpose : Allocate space for a new Display Descriptor 
 *           Append descriptor to the descriptor list
 * Return  : A pointer to allocated Display Descriptor (always successes)
\******************************************************************************/
 
static __inline STSUBT_TEST_DisplayDescriptor_t * 
NewDisplayDescriptor (ST_Partition_t *MemoryPartition,
                      STSUBT_TEST_ServiceHandle_t *ServiceHandle)
{
    STSUBT_TEST_DisplayDescriptor_t *descriptor_p;
    U32 descriptor_size = sizeof(STSUBT_TEST_DisplayDescriptor_t);
 
    /* Allocate a descriptor */
 
    descriptor_p = (STSUBT_TEST_DisplayDescriptor_t*)
        memory_allocate(MemoryPartition, descriptor_size);
    if (descriptor_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### NewDisplayDescriptor: no space ####\n"));
        return (NULL);
    }

    /* initialize the descriptor */
 
    descriptor_p->nEpoch = ServiceHandle->nEpoch;
    descriptor_p->nDisplay = ServiceHandle->nDisplay;
    descriptor_p->presentation = 0;
    descriptor_p->timeout = 0;
    descriptor_p->VisibleRegionList = NULL;
    descriptor_p->NextDescriptor = NULL;

    /* append descriptor to descriptor list */
 
    if (ServiceHandle->DisplayList == NULL)
    {
        ServiceHandle->DisplayList = descriptor_p;
        ServiceHandle->DisplayListTail = ServiceHandle->DisplayList;
    }
    else
    {
        (ServiceHandle->DisplayListTail)->NextDescriptor = (void *)descriptor_p;
        ServiceHandle->DisplayListTail = descriptor_p;
    }

    return (descriptor_p);
}
 

/******************************************************************************\
 * Function: NewEpochRegion
 * Purpose : Allocate a new TEST Region and a new RegionItem including it in 
 *           the EpochRegionList
 * Parameters:
 * Return  : 
\******************************************************************************/
 
static __inline
ST_ErrorCode_t NewEpochRegion (STTEST_RegionParams_t *RegionParams,
                               STSUBT_TEST_ServiceHandle_t *ServiceHandle)
{
  STSUBT_TEST_RegionItem_t *RegionItem_p;
  U32 region_item_size = sizeof(STSUBT_TEST_RegionItem_t);
  ST_Partition_t *MemoryPartition = ServiceHandle->MemoryPartition;
 
  RegionItem_p = (STSUBT_TEST_RegionItem_t*)memory_allocate(MemoryPartition,
                                                            region_item_size);
  if (RegionItem_p == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### NewEpochRegion: no space ####\n"));
      return (1);
  }
 
  /* --- Fill in RegionItem data structure and --- */
  /* --- insert RegionItem in EpochRegionList  --- */
 
  RegionItem_p->region_id = RegionParams->RegionId;
  RegionItem_p->region_width = RegionParams->Width;
  RegionItem_p->region_height = RegionParams->Height;
  RegionItem_p->horizontal_position = RegionParams->PositionX;
  RegionItem_p->vertical_position = RegionParams->PositionY;
  RegionItem_p->region_depth = RegionParams->NbBitsPerPel;

  RegionItem_p->NextRegion = (void*)ServiceHandle->EpochRegionList;
  ServiceHandle->EpochRegionList = RegionItem_p;

  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: GetRegionItem
 * Purpose : Try to retrieve a RegionItem into the EpochRegionList
 * Return  : A pointer to found RegionItem, NULL if RegionItem was not found
\******************************************************************************/
 
static __inline STSUBT_TEST_RegionItem_t *
GetRegionItem (U8 region_id, STSUBT_TEST_ServiceHandle_t *ServiceHandle)
{
  STSUBT_TEST_RegionItem_t *EpochRegionList;
 
  EpochRegionList = ServiceHandle->EpochRegionList;
  while (EpochRegionList != NULL)
  {
      if (EpochRegionList->region_id == region_id)
          return (EpochRegionList);
      EpochRegionList = EpochRegionList->NextRegion;
  }
  return (EpochRegionList);
}
 
/******************************************************************************\
 * Function: CreateVisibleRegionList
 * Purpose : 
 * Return  : 
\******************************************************************************/
 
static __inline
STSUBT_TEST_RegionItem_t *
CreateVisibleRegionList (PCS_t *PCS, STSUBT_TEST_ServiceHandle_t *ServiceHandle)
{
  ST_Partition_t *MemoryPartition = ServiceHandle->MemoryPartition;
  STSUBT_TEST_RegionItem_t *VisibleRegionList = NULL;
  STSUBT_TEST_RegionItem_t *VisibleRegionItem;
  STSUBT_TEST_RegionItem_t *RegionItem_p;
  U32 vis_region_size;
 
  VisibleRegion_t *PCSVisibleRegion = PCS->VisibleRegion_p;
 
  vis_region_size = sizeof(STSUBT_TEST_RegionItem_t);
 
  while (PCSVisibleRegion != NULL)
  {
      /* For each visible region, find corresponding region item into 
       * EpochRegionList.
       * Create a new visible region item including it into VisibleRegionList
       */
 
      /* --- Allocate a Visible Region Item --- */
 
      VisibleRegionItem = (STSUBT_TEST_RegionItem_t*)
          memory_allocate(MemoryPartition, vis_region_size);
      if (VisibleRegionItem == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### CreateVisibleRegionList: no space ####\n"));
          return (NULL);
      }

      /* --- Retrieve corresponding RegionItem into EpochRegionList --- */
 
      RegionItem_p = GetRegionItem(PCSVisibleRegion->region_id, ServiceHandle);
      if (RegionItem_p == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### CreateVisibleRegionList: Cannot find region\n"));
          PCSVisibleRegion = PCSVisibleRegion->next_VisibleRegion_p;
          continue;
      }

      VisibleRegionItem->region_id = RegionItem_p->region_id;
      VisibleRegionItem->region_width = RegionItem_p->region_width;
      VisibleRegionItem->region_height = RegionItem_p->region_height;
      VisibleRegionItem->region_depth = RegionItem_p->region_depth;

      VisibleRegionItem->horizontal_position = 
                         PCSVisibleRegion->horizontal_position;
      VisibleRegionItem->vertical_position = 
                         PCSVisibleRegion->vertical_position;

      /* Append VisibleRegionItem to VisibleRegionList */
 
      VisibleRegionItem->NextRegion = VisibleRegionList;
      VisibleRegionList = VisibleRegionItem;

      PCSVisibleRegion = PCSVisibleRegion->next_VisibleRegion_p;
  }

  return (VisibleRegionList);
}


/******************************************************************************\
 * Function: STSUBT_TEST_InitializeService
 * Purpose : To be called before any display is achived  
 *           This the implementation of STSUBT_InitializeService callback 
 *           function related to TEST device
 *           Purpose of this function is mainly to create and initialize
 *           the Service Handle
 * Return  : A pointer to allocated Displaying Service Handle
\******************************************************************************/
 
void *STSUBT_TEST_InitializeService (STSUBT_DisplayServiceParams_t *DisplayServiceParams)
{
  TestServiceHandle = NewServiceHandle (DisplayServiceParams->MemoryPartition);
  if (TestServiceHandle == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"#### TEST_Initialize_Display:Error creating Service Handle"));
      return (NULL);
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n#### TEST_Initialize_Display: TEST Device Initialized ####"));

  return ((void*)TestServiceHandle);
}


/******************************************************************************\
 * Function: STSUBT_TEST_TerminateService
 * Purpose : This the implementation of STSUBT_Terminate_Display callback  
 *           function related to TEST device 
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_TEST_TerminateService (void *ServiceHandle)
{  
  STSUBT_TEST_ServiceHandle_t *TEST_ServiceHandle;
  ST_ErrorCode_t               res = ST_NO_ERROR;  
 
  TEST_ServiceHandle = (STSUBT_TEST_ServiceHandle_t*)ServiceHandle;

  semaphore_wait(TEST_ServiceHandle->DisplaySemaphore);

  /* --- Disables current display on the screen (if any) --- */

  if (TEST_ServiceHandle->ActiveDisplayDescriptor != NULL)
  { 
      (TEST_ServiceHandle->ActiveDisplayDescriptor)->timeout = time_now();
      TEST_ServiceHandle->ActiveDisplayDescriptor = NULL;
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n#### TEST_Terminate_Display: TEST Device Terminated ####"));
 
  semaphore_signal(TEST_ServiceHandle->DisplaySemaphore);

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_TEST_PrepareDisplay
 * Purpose : This the implementation of STSUBT_PrepareDisplay callback
 *           function related to TEST device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t STSUBT_TEST_PrepareDisplay (
                      PCS_t *PageComposition_p,
                      void  *ServiceHandle,
                      void **display_descriptor)
{
  STTEST_RegionParams_t     RegionParams;
  ST_ErrorCode_t            res = ST_NO_ERROR;
  STSUBT_TEST_RegionItem_t *VisibleRegionList = NULL;
  ST_Partition_t           *MemoryPartition;
  STSUBT_TEST_ServiceHandle_t     *TEST_ServiceHandle;
  STSUBT_TEST_DisplayDescriptor_t **TEST_display_descriptor;
 
  TEST_ServiceHandle = (STSUBT_TEST_ServiceHandle_t*)ServiceHandle;
  MemoryPartition = TEST_ServiceHandle->MemoryPartition;

  semaphore_wait(TEST_ServiceHandle->DisplaySemaphore);

  *display_descriptor = NewDisplayDescriptor(MemoryPartition,
                                             TEST_ServiceHandle);
  if (*display_descriptor == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"\n#### Prepare_Display: no space ####\n"));
      semaphore_signal(TEST_ServiceHandle->DisplaySemaphore);
      return (1);
  }

  TEST_display_descriptor=(STSUBT_TEST_DisplayDescriptor_t**)display_descriptor;

  /* --- check the acquisition mode of PCS --- */

  if ((PageComposition_p->acquisition_mode == STSUBT_MODE_CHANGE)
  ||  (TEST_ServiceHandle->Starting == TRUE))
  {
      /* A new epoch is starting; current display is disabled,
       * old regions are deleted and new regions created.
       * New regions are linked to create an EpochRegionList.
       *
       * The VisibleRegion_p field in a PCS of STSUBT_MODE_CHANGE
       * contains the list of regions planned for the starting epoch.
       */

      RCS_t *EpochRegionList;

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n#### Prepare_Display: MODE_CHANGE ####\n"));

      /* --- Disables current display on the screen (if any) --- */
 
      if (TEST_ServiceHandle->ActiveDisplayDescriptor != NULL)
      {
         (TEST_ServiceHandle->ActiveDisplayDescriptor)->timeout = time_now();
          TEST_ServiceHandle->ActiveDisplayDescriptor = NULL;
      }

      /* --- reset the Descriptor Buffer --- */

      ResetServiceHandle(TEST_ServiceHandle);

      /* --- Create the new epoch regions --- */

      EpochRegionList = PageComposition_p->EpochRegionList;

      while (EpochRegionList != NULL)
      {
          /* fill in RegionParams */

          RegionParams.RegionId     = EpochRegionList->region_id;
          RegionParams.PositionX    = 0;
          RegionParams.PositionY    = 0;
          RegionParams.Width        = EpochRegionList->region_width;
          RegionParams.Height       = EpochRegionList->region_height;
          RegionParams.NbBitsPerPel = EpochRegionList->region_depth;
          
          /* create the region and include it in the EpochRegionList */

          res = NewEpochRegion(&RegionParams, TEST_ServiceHandle);
          if (res != ST_NO_ERROR)
          {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"#### STSUBT_TEST_PrepareDisplay: error alloc region\n"));
          }
          EpochRegionList = (RCS_t*)EpochRegionList->next_region_p;
      }
  }

  /* Update Descriptor to include provided list of visible regions*/

  TEST_ServiceHandle->nDisplay ++;

  VisibleRegionList = 
      CreateVisibleRegionList (PageComposition_p, TEST_ServiceHandle);

  (*TEST_display_descriptor)->nEpoch = TEST_ServiceHandle->nEpoch;
  (*TEST_display_descriptor)->nDisplay = TEST_ServiceHandle->nDisplay;
  (*TEST_display_descriptor)->page_id = PageComposition_p->page_id;
  (*TEST_display_descriptor)->acquisition_mode = PageComposition_p->acquisition_mode;
  (*TEST_display_descriptor)->VisibleRegionList = VisibleRegionList;

  TEST_ServiceHandle->Starting = FALSE;

  semaphore_signal(TEST_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n#### Prepare_Display: Display Prepared ####"));

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_TEST_ShowDisplay
 * Purpose : This the implementation of STSUBT_ShowDisplay callback
 *           function related to TEST device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_TEST_ShowDisplay (void *ServiceHandle, 
                                        void *display_descriptor)
{
  STSUBT_TEST_DisplayDescriptor_t *TEST_display_descriptor;
  STSUBT_TEST_ServiceHandle_t     *TEST_ServiceHandle;
  ST_ErrorCode_t                  res = ST_NO_ERROR;
 
  TEST_ServiceHandle = (STSUBT_TEST_ServiceHandle_t*)ServiceHandle;

  semaphore_wait(TEST_ServiceHandle->DisplaySemaphore);

  TEST_display_descriptor =(STSUBT_TEST_DisplayDescriptor_t*)display_descriptor;

  if (TEST_display_descriptor->nEpoch != TEST_ServiceHandle->nEpoch)
  {
      /* Current display belong to an old epoch */
      semaphore_signal(TEST_ServiceHandle->DisplaySemaphore);
      return (1); 
  }

  /* --- refresh active display (if any) --- */

  if (TEST_ServiceHandle->ActiveDisplayDescriptor != NULL)
  {
      TEST_ServiceHandle->ActiveDisplayDescriptor->timeout = time_now();
      TEST_ServiceHandle->ActiveDisplayDescriptor = NULL;
  }

  /* --- make display "visible" --- */
 
  TEST_display_descriptor->presentation = time_now();

  /* --- update active display descriptor in ServiceHandle --- */
 
  TEST_ServiceHandle->ActiveDisplayDescriptor = TEST_display_descriptor;

  semaphore_signal(TEST_ServiceHandle->DisplaySemaphore);

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n#### ShowDisplay: Display Shown ####"));

  return (res);
}
 

/******************************************************************************\
 * Function: STSUBT_TEST_HideDisplay
 * Purpose : This the implementation of the STSUBT_HideDisplay callback
 *           function related to TEST display device
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t STSUBT_TEST_HideDisplay (void *ServiceHandle,
                                        void *display_descriptor)
{
  STSUBT_TEST_DisplayDescriptor_t *TEST_display_descriptor;
  STSUBT_TEST_ServiceHandle_t     *TEST_ServiceHandle;
  ST_ErrorCode_t                   res = ST_NO_ERROR;

  TEST_ServiceHandle = (STSUBT_TEST_ServiceHandle_t*)ServiceHandle;

  semaphore_wait(TEST_ServiceHandle->DisplaySemaphore);

  TEST_display_descriptor =(STSUBT_TEST_DisplayDescriptor_t*)display_descriptor;

  if (TEST_display_descriptor->nEpoch == TEST_ServiceHandle->nEpoch)
  {
      /* --- "Hide" display --- */
    
      TEST_display_descriptor->timeout = time_now();
    
      /* --- reset active display descriptor in ServiceHandle --- */
    
      TEST_ServiceHandle->ActiveDisplayDescriptor = NULL;
  }

  semaphore_signal(TEST_ServiceHandle->DisplaySemaphore);
  
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n#### HideDisplay: Display Hidden ####"));

  return (res);
}


/******************************************************************************\
 * Function: STSUBT_TEST_Report
 * Purpose : Report on achieved displays
\******************************************************************************/
 

void STSUBT_TEST_Report (void)
{
  STSUBT_TEST_DisplayDescriptor_t *DisplayList;
  STSUBT_TEST_RegionItem_t *VisibleRegionList;
  U8  region_id;
  U16 region_width;
  U16 region_height;
  U16 horizontal_position;
  U16 vertical_position;
  U16 region_depth;

  DisplayList = TestServiceHandle->DisplayList;

  STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\n\nPrinting Report:"));

  if (DisplayList == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"Report: DisplayList is empty"));
  }

  while (DisplayList != NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"============================================================="));

      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"nEpoch    = %d", DisplayList->nEpoch));
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"nDisplay  = %d", DisplayList->nDisplay));
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"page_id   = %d", DisplayList->page_id));
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"presented = %d", DisplayList->presentation));
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"timed out = %d", DisplayList->timeout));
      if (DisplayList->acquisition_mode == STSUBT_MODE_CHANGE)
      {
           STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"mode      = MODE_CHANGE\n"));
      }
      else 
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"mode      = NORMAL_CASE\n"));
      }

      VisibleRegionList = DisplayList->VisibleRegionList;
      while (VisibleRegionList != NULL)
      {
          region_id = VisibleRegionList->region_id;
          region_width = VisibleRegionList->region_width;
          region_height = VisibleRegionList->region_height;
          horizontal_position = VisibleRegionList->horizontal_position;
          vertical_position = VisibleRegionList->vertical_position;
          region_depth = VisibleRegionList->region_depth;

          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\tregion_id = %d", region_id));
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\thorizontal_position = %d",horizontal_position));
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\tvertical_position = %d",vertical_position));
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\tregion_width = %d",region_width));
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\tregion_height = %d",region_height));
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"\tregion_depth = %d\n",region_depth));

          VisibleRegionList = VisibleRegionList->NextRegion;
      }
      DisplayList = DisplayList->NextDescriptor;
  }
  STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"============================================================="));
}
 
#endif /* end TEST_DISPLAY_SERVICE defined */

