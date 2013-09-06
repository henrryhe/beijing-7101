/******************************************************************************\
 *
 * File Name   : engine.h
 *
 * Description : Exported types and function of Display Engine
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Feb 1999
 *
\******************************************************************************/
 
#ifndef __ENGINE_H
#define __ENGINE_H
 
#include <stddefs.h>

#include <subtdev.h>


/* --------------------------------------- */
/* --- Control functions for Engine --- */
/* --------------------------------------- */

/* --- Create/Delete a Engine --- */
 
ST_ErrorCode_t Engine_Create (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Engine_Delete (STSUBT_InternalHandle_t *);

/* --- Start/Stop Engine --- */
 
ST_ErrorCode_t Engine_Start (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Engine_Stop  (STSUBT_InternalHandle_t *);

#endif  /* #ifndef __ENGINE_H */

