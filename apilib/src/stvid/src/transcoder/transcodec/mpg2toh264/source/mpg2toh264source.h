/*!
 ************************************************************************
 * \file mpg2toh264source.h
 *
 * \brief MPEG2 to H264 source decoder data structures and functions prototypes
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __MPG2TOH264SOURCE_H
#define __MPG2TOH264SOURCE_H

/* Includes ----------------------------------------------------------------- */
#include "mpg2toh264.h"

#include "stfdma.h"
#include "inject.h"

#ifdef ST_OSLINUX
#include "compat.h"
#else

#ifndef STTBX_REPORT
    #define STTBX_REPORT
#endif
#ifndef STTBX_PRINT    
    #define STTBX_PRINT
#endif    
#include "sttbx.h"
#include "stsys.h"
#endif

#include "vid_ctcm.h"

/* Constants ------------------------------------------------------- */
typedef enum SkipBMethod_e
{
  SKIP_0_B,
  SKIP_1_B,
  SKIP_2_B,
  SKIP_3_B
} SkipBMethod_t;

/* Exported types ----------------------------------------------------------- */
/*! Source decoder top level data structure */
typedef struct mpg2toh264source_PrivateData_s
{
  BOOL IsFirstPicture;
  U32 PreviousPictureDecodingOrderFrameID;
  U32 BPictureCounter;
  U32 BPictureBetweenP;
  BOOL ToggleBSkip;
  BOOL IsLastPictureSkipped;
} mpg2toh264source_PrivateData_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

#endif /* #ifdef __MPG2TOH264SOURCE_H */

