/******************************************************************************\
 *
 * File Name   : filter.h
 *
 * Description : Exported types and function of Filter Module
 *
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Dec 1998
 *
\******************************************************************************/
 
#ifndef __FILTER_H
#define __FILTER_H
 
#include <stddefs.h>
#include <subtdev.h>

#define WAIT_NO_TS	1
#define READ_ERROR	2
#define WAIT_ERROR	3

/* ----------------------------------------- */
/* --- Control functions for Filter task --- */
/* ----------------------------------------- */

/* --- Create/Delete a filter task --- */
 
ST_ErrorCode_t Filter_Create (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Filter_Delete (STSUBT_InternalHandle_t *);
 
/* --- Set a subtitle stream --- */
 
ST_ErrorCode_t Filter_SetStream (U16 composition_page_id, 
                                 U16 ancillary_page_id,
                                 STSUBT_InternalHandle_t *);

/* --- Start/Stop filter task --- */
 
ST_ErrorCode_t Filter_Start (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Filter_Stop  (STSUBT_InternalHandle_t *);
 
#endif  /* #ifndef __FILTER_H */

