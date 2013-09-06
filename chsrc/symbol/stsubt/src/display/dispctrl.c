/******************************************************************************\
 *
 * File Name   : dispctrl.c
 *  
 * Description : API functions for controlling Display
 *     This module drives the available display hardware to show the decoded 
 *     subtitle displays (whose pixels are found in the Pixel Buffer) at the 
 *     time indicated by the associated presentation time stamp (PTS). 
 *     Since displays have no natural refresh rate, a dedicated timing 
 *     controller has been introduced (Display Timer).
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *  
 * Author : Marino Congiu - Feb 1999
 *
\******************************************************************************/

#include <stdlib.h>

#include <stddefs.h>
#include <stlite.h>

#include <stsubt.h>
#include <subtdev.h>
#include <common.h>


#include <sttbx.h>

#include "dispctrl.h"
#include "engine.h"
#include "timer.h"


/******************************************************************************\
 * Function: InitDisplayController
 *
 * Purpose : Initialises the presentation module and creates instances 
 *           of Display Engine, Display Buffer and Display Timer
 *
 * Parameters:
 *     OpenParams:
 *           Input parameters for Displaying
 *
 * Return  : ST_NO_ERROR on success
 * Note    : This function is called into STSUBT_Init API function
\******************************************************************************/

ST_ErrorCode_t InitDisplayController (STSUBT_InternalHandle_t *DecoderHandle)
{
  fcBuffer_t              *DisplayBuffer;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"## InitDisplayController: Initializing Display!\n"));

  /* --- create the Display Buffer --- */
 
  if ((DisplayBuffer = fcBuffer_Create(DecoderHandle->MemoryPartition,
                                       DISPLAY_BUFFER_NUM_ITEMS,
                                       DISPLAY_ITEM_SIZE)) == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"## InitDisplayController: cannot create Display Buffer !\n"));
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  if (Engine_Create (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"## InitDisplayController: cannot create Display Engine !\n"));
      fcBuffer_Delete(DisplayBuffer);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* --- create Display Timer --- */
 
  if (Timer_Create (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
		"## InitDisplayController: cannot create Display Timer !\n"));
      Engine_Delete(DecoderHandle);
      fcBuffer_Delete(DisplayBuffer);
      return(STSUBT_ERROR_ALLOCATING_MEMORY);
  }

  /* --- fill in display handle --- */
 
  DecoderHandle->DisplayStatus = STSUBT_Display_Ready;
  DecoderHandle->DisplayBuffer = DisplayBuffer;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"## InitDisplayController: Display Initialized !\n"));

  return(ST_NO_ERROR);
}


/******************************************************************************\
 * Function: EnablePresentation
 * Parameters
 *     SubtHandle:
 *           Handle of Display Controller
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_DISPLAYING
 *           STSUBT_ERROR_STARTING_ENGINE
 *           STSUBT_ERROR_STARTING_TIMER
 *           STSUBT_ERROR_INIT_DISPLAY_SERVICE
 * Note    : This function is called into STSUBT_Show API function
\******************************************************************************/
 
ST_ErrorCode_t EnablePresentation (STSUBT_InternalHandle_t *DecoderHandle,
			STSUBT_DisplayServiceParams_t *DisplayServiceParams)
{

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
			"## EnablePresentation: Enabling Subtitles Display !\n"));
 
  /* --- Driver should be not displaying --- */
 
  if (DecoderHandle->DisplayStatus == STSUBT_Display_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
				"## EnablePresentation: Still Displaying !\n"));
      return (STSUBT_ERROR_DRIVER_DISPLAYING);
  }
 
  /* --- Reset Display Buffer --- */

  fcBuffer_Reset(DecoderHandle->DisplayBuffer);
  

  /* --- Initialize the Display Service --- */

  if  (DecoderHandle->DisplayService->STSUBT_InitializeService == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"## EnablePresentation: Init Display callback NULL **\n"));
      DecoderHandle->DisplayService->STSUBT_ServiceHandle = NULL;
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"## EnablePresentation: Initializing Display Device **\n"));

       DisplayServiceParams->MemoryPartition=DecoderHandle->MemoryPartition;
       DisplayServiceParams->Handle_p=DecoderHandle;
 
      DecoderHandle->DisplayService->STSUBT_ServiceHandle = 
            DecoderHandle->DisplayService->STSUBT_InitializeService 
				        (DisplayServiceParams);

      if (DecoderHandle->DisplayService->STSUBT_ServiceHandle == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"## EnablePresentation: Error: Init Display Device **\n"));
          return (STSUBT_ERROR_INIT_DISPLAY_SERVICE);
      }
		STTBX_Report((STTBX_REPORT_LEVEL_USER1,
			"## EnablePresentation: Display Device Initialized **\n"));
  }

  /* --- start the Display Engine --- */
 
  if (Engine_Start (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"## EnablePresentation: cannot start Engine !\n"));
      return (STSUBT_ERROR_STARTING_ENGINE);
  }
 
  /* --- start the Display Timer --- */
 
  if (Timer_Start (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,
			"## EnablePresentation: cannot start Timer!\n"));
      return (STSUBT_ERROR_STARTING_TIMER);
  }
 
  /* --- set display status --- */

  DecoderHandle->DisplayStatus = STSUBT_Display_Displaying;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,
		"## EnablePresentation: Display successfully Enabled !\n"));
 
  return(ST_NO_ERROR);
}


/******************************************************************************\
 * Function: DisablePresentation
 * Parameters
 *     SubtHandle:
 *           Handle of Display Controller
 * Return  : ST_NO_ERROR on success, else ...
 *           STSUBT_ERROR_DRIVER_NOT_DISPLAYING
 *           STSUBT_ERROR_STOPPING_ENGINE
 *           STSUBT_ERROR_STOPPING_TIMER
 *           STSUBT_ERROR_TERM_DISPLAY_SERVICE
 * Note    : This function is called into STSUBT_Hide API function
\******************************************************************************/
 
ST_ErrorCode_t DisablePresentation (STSUBT_InternalHandle_t *DecoderHandle)
{
  ST_ErrorCode_t res = ST_NO_ERROR;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## DisablePresentation: Disabling Display !\n"));
 
  /* --- Driver should be displaying when disabling display --- */
 
  if (DecoderHandle->DisplayStatus != STSUBT_Display_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## DisablePresentation: Not Displaying !\n"));
      return (STSUBT_ERROR_DRIVER_NOT_DISPLAYING);
  }
 
  /* --- stop the Display Engine --- */
 
  if (Engine_Stop (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## DisablePresentation: cannot stop Engine !\n"));
      return (STSUBT_ERROR_STOPPING_ENGINE);
  }
 
  /* --- stop the Display Timer --- */
 
  if (Timer_Stop (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## DisablePresentation: cannot stop Timer!\n"));
      return (STSUBT_ERROR_STOPPING_TIMER);
  }

  /* --- Terminate Display Service --- */
 
  if (DecoderHandle->DisplayService->STSUBT_TerminateService == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** DisablePresentation: Term Service callback NULL **\n"));
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** DisablePresentation: Terminating Display Service **\n"));
 
      res = DecoderHandle->DisplayService->STSUBT_TerminateService (
                 DecoderHandle->DisplayService->STSUBT_ServiceHandle);
      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** DisablePresentation: Error: Term Display Service \n"));
          return (STSUBT_ERROR_TERM_DISPLAY_SERVICE);
      }
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** DisablePresentation: Display Service Terminated **\n"));
  }
 
  /* --- Set new display status --- */
 
  DecoderHandle->DisplayStatus = STSUBT_Display_Ready;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## DisablePresentation: Display Disabled !\n"));
 
  return(res);
}
 

/******************************************************************************\
 * Function: TermDisplayController
 * Purpose : Terminates the Presentation Module and removes instances of
 *           Display Engine, Display Buffer and Display Timer.
 * Parameters
 *     SubtHandle:
 *           Handle of Display Controller
 * Return  : ST_NO_ERROR on success
 *           STSUBT_ERROR_DRIVER_DISPLAYING
 *           STSUBT_ERROR_DELETING_TIMER
 *           STSUBT_ERROR_DELETING_ENGINE
 * Note    : This function is called into STSUBT_Close API function
\******************************************************************************/
 
ST_ErrorCode_t TermDisplayController (STSUBT_InternalHandle_t *DecoderHandle)
{
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## TermDisplayController: Terminating Display !\n"));
 
  /* --- Driver must be not displaying when Terminating --- */
 
  if (DecoderHandle->DisplayStatus == STSUBT_Display_Displaying)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## TermDisplayController: Still Displaying !\n"));
      return (STSUBT_ERROR_DRIVER_DISPLAYING);
  }
 
  /* --- delete the Display Timer --- */
 
  if (Timer_Delete (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## TermDisplayController: cannot delete Display Timer !\n"));
      return (STSUBT_ERROR_DELETING_TIMER);
  }
 
  /* --- delete the Display Engine --- */
 
  if (Engine_Delete (DecoderHandle) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"## TermDisplayController: cannot delete Display Engine !\n"));
      return (STSUBT_ERROR_DELETING_ENGINE);
  }
 
  /* --- delete Display buffer --- */
 
  fcBuffer_Delete(DecoderHandle->DisplayBuffer);
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"## TermDisplayController: Display Terminated !\n"));
 
  return(ST_NO_ERROR);
}

