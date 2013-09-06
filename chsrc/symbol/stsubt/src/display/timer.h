/******************************************************************************\
 *
 * File Name   : timer.h
 *
 * Description : Exported types and function of Display Timer
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Feb 1999
 *
\******************************************************************************/
 
#ifndef __TIMER_H
#define __TIMER_H
 
#include <stddefs.h>

#include <subtdev.h>

/* --------------------------------------- */
/* --- Control functions for Timer --- */
/* --------------------------------------- */

/* --- Create/Delete a Timer --- */
 
ST_ErrorCode_t Timer_Create (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Timer_Delete (STSUBT_InternalHandle_t *);
 
/* --- Start/Stop Timer --- */
 
ST_ErrorCode_t Timer_Start (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Timer_Stop  (STSUBT_InternalHandle_t *);
 

#endif  /* #ifndef __TIMER_H */

