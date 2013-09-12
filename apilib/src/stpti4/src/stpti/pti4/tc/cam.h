/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: cam.h
 Description: PTI cam data structures needed by ../pti4.h to complete
             its memory map of the TC.

******************************************************************************/

#ifndef __CAM_H
 #define __CAM_H


/* Includes ------------------------------------------------------------ */

#include "stpti.h"

#if defined( STPTI_ARCHITECTURE_PTI4 )
 #include "stdcam.h"
#elif defined( STPTI_ARCHITECTURE_PTI4_LITE ) || defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
 #include "ramcam.h"
#else
 #error Please supply architecture information!
#endif

#include "pti4.h"


/* ------------------------------------------------------------------------- */


#endif /* __CAM_H */


/* EOF --------------------------------------------------------------------- */
