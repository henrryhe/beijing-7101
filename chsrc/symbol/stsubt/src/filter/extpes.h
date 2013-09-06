/******************************************************************************\
 *
 * File Name   : extpes.h
 *  
 * Description : For link only, extracts pes data from pti_buffer into
 *               local buffer for initial parsing, then deposits packet
 *               either whole or paritally into the linear buffer.
 *  
 * Copyright 2002 STMicroelectronics. All Rights Reserved.
 *
 * Author : Anthony Kemble 2002
 *  
\******************************************************************************/
 
#ifndef __EXTPES_H
#define __EXTPES_H
 
#include <stddefs.h>
#include <subtdev.h>


/* ----------------------------------------- */
/* --- Control functions for Extpes task --- */
/* ----------------------------------------- */

/* --- Create/Delete a extpes task --- */
 
ST_ErrorCode_t ExtractPes_Create (STSUBT_InternalHandle_t *);
ST_ErrorCode_t ExtractPes_Delete (STSUBT_InternalHandle_t *);
 

/* --- Start/Stop ExtractPes task --- */
 
ST_ErrorCode_t ExtractPes_Start (STSUBT_InternalHandle_t *);
ST_ErrorCode_t ExtractPes_Stop  (STSUBT_InternalHandle_t *);
 
#endif  /* #ifndef __ExtractPes_H */

