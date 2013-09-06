/******************************************************************************\
 * 
 * File Name   : subtitle.h
 * 
 * Description : Definition for pes and segment parsing. 
 * 
 * (c) Copyright STMicroelectronics - Nov 1999
 * 
\******************************************************************************/

#ifndef __SUBTITLE_H
#define __SUBTITLE_H

#include <stddefs.h>

/* ------------------------------------------------ */
/* --- Some defines for PES and segment parsing --- */
/* ------------------------------------------------ */

#define PACKET_START_CODE_PREFIX        0x000001
#define PRIVATE_STREAM_ID_1             0xbd
#define SEGMENT_SYNC_BYTE               0x0f
 
#define PTS_SEGMENT_TYPE                0x0f    /* our code for pts segments  */
#define PCS_SEGMENT_TYPE                0x10
#define RCS_SEGMENT_TYPE                0x11
#define CLUT_SEGMENT_TYPE               0x12
#define EDS_SEGMENT_TYPE                0x80    /* for DTG standard - not DVB */
#define OBJECT_SEGMENT_TYPE             0x13
 
/* define length of pts segments */
 
#define PTS_SEGMENT_LENGHT 14
 
/* pts mark is 33 bits value, so we need two 32-bit words to store it */
 
typedef struct pts_s {
  S32 high;
  U32 low;
} pts_t;
 
/* A segment is simply a sequence of characters */
 
typedef U8 segment_t;

#endif

