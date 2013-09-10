/************************************************************************
File Name   : stlite.h

Description : Includes all stlite header files, so that drivers & applications
              need only include this file.

Copyright (C) 2005 STMicroelectronics

Revision History:

    18/05/00    Redefined legacy boolean type to bool when compiling with
                C++.

    18/04/01    Added ST20/ST40 compatibility definitions

Reference   :

************************************************************************/


#ifndef __STLITE_H
#define __STLITE_H

#ifdef ST_OSLINUX
#include <stddefs.h>
#endif


#ifndef ST_OSLINUX /*corresponding endif at end of file*/
#ifdef ST_OS21
#include <os21.h>
#include <time.h>
#ifdef ARCHITECTURE_ST40
#include <os21/st40.h>
#endif /*#ifdef ARCHITECTURE_ST40*/
#ifdef ARCHITECTURE_ST200
#include <os21/st200.h>
#endif /*#ifdef ARCHITECTURE_ST200*/
#endif

#ifdef ST_OS20
#include <interrup.h>
#include <kernel.h>
#include <message.h>
#include <mutex.h>
#include <partitio.h>
#include <semaphor.h>
#include <task.h>
#include <ostime.h>
#include <cache.h>
#endif

/*
The macros in os_20to21_map.h are included by default.
The following DVD_EXCLUDE_OS20TO21_MACROS needs to be passed as a Cflag
from that driver's makefile, which intends to disable the macros provided
by os_20to21_map.h

As an example, the following 3 lines can be added to the makefile of that
driver/testapp, which wishes to disable these macros:

ifdef DVD_EXCLUDE_OS20TO21_MACROS
 CFLAGS  +=-DDVD_EXCLUDE_OS20TO21_MACROS
endif

*/
#ifndef DVD_EXCLUDE_OS20TO21_MACROS
#include "os_20to21_map.h" /*OS20-OS21 wrapper*/
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ARCHITECTURE_ST20
/* fix for DCU 1.8.1 - different number of parameters */
#if OS20_VERSION_MAJOR >= 2 && OS20_VERSION_MINOR >= 7
  #define interrupt_status(a,b) interrupt_status(a,b,0)
#endif
#endif

/* The following is required for (legacy) PTI and TestTool compatibility,
  which require the identifiers boolean, true, and false */

#ifdef __cplusplus
/* true and false are already defined. Define the type the same size
  as under C compilation below (DDTS 18417) */
#ifndef ST_OSWINCE
typedef int boolean;
#endif

#else
#if !defined(true) && !defined(false)


#if 0 /*yxl 2007-06-27 修改类型boolean的定义,以便V1+ CA和ST 平台的统一 */ 

typedef enum { false = 0, true = 1 } boolean;

#else

#ifndef DEFINE_BOOLEAN
typedef unsigned char	boolean;
#endif

enum { false = 0, true = 1 };

#endif/*end /*yxl 2007-06-27 修改类型boolean的定义,以便V1+ CA和ST 平台的统一 */  

#endif
#endif

/****************************************************************************
 Toolset compatibility issues 
****************************************************************************/

/* ST20/ST40 __inline compatibility */
#ifndef __inline
#if defined(ARCHITECTURE_ST40) && !defined(ST_OSWINCE)
#define __inline __inline__
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif /*#ifndef ST_OSLINUX*/

#endif /* __STLITE_H */
