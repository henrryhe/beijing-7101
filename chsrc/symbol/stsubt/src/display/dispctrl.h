/******************************************************************************\
 *
 * File Name   : dispctrl.h
 *  
 * Description : Exported types and function to controlling display
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Feb 1999
 *  
\******************************************************************************/

#ifndef __DISPCTRL_H
#define __DISPCTRL_H

#include <stddefs.h>
#include <subtdev.h>


/* ---------------------------------------- */
/* --- Control functions for Displaying --- */
/* ---------------------------------------- */
 
/* --- initialize/terminate display --- */

ST_ErrorCode_t InitDisplayController (STSUBT_InternalHandle_t *);
ST_ErrorCode_t TermDisplayController (STSUBT_InternalHandle_t *);

/* --- enable/disable presentation of subtitles --- */

ST_ErrorCode_t EnablePresentation  (STSUBT_InternalHandle_t *,
						STSUBT_DisplayServiceParams_t *);
ST_ErrorCode_t DisablePresentation (STSUBT_InternalHandle_t *);

#endif  /* #ifndef __DISPCTRL_H */

