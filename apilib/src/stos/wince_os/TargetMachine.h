/*! Time-stamp: <@(#)TargetMachine.h   07/04/2005 - 13:38:08   William Hennebois>
 *********************************************************************
 *  @file   : TargetMachine.h
 *
 *  Project : stdevice_user.h
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William Hennebois            Date: 07/04/2005
 *
 *  Purpose : All definition for the Target Machine, this file 
 *		      is forced for the compilation of all source code
 *			   by the commande /FITargetMachine.h
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  07/04/2005 WH : First Revision
 *
 *********************************************************************
 */
#ifndef __TargetMachine__
#define __TargetMachine__

#include "WinBase.h"
#include "stdlib.h"
#include "string.h"
#include "types.h"
#include "limits.h"

#undef WINCE_USE_INLINE

#define WINCE_USE_INLINE 

#pragma warning(disable:4068) /* disable pragma */
#pragma warning(disable:4018) /* disable <' : signed/unsigned mismatch */
#pragma warning(disable:4028) /* disable formal parameter 1 different from declaration */
#pragma warning(disable:4761) /* disable integral size mismatch in argument; conversion supplied */
#pragma warning(disable:4005) /* disable macro redefinition */
#pragma warning(disable:4700) /* local variable 'ErrorCode' used without hav..)*/
#pragma warning(disable:4146) /* unary minus operator applied to unsigned ... */
#pragma warning(disable:4101) /* unreferenced local variable */
#pragma warning(disable:4244) /* conversion from 'double ' to 'unsigned int ', possible loss of data*/
#pragma check_stack(off)



//! Include collection toolbox for unicode conversion
#include <stdarg.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C"
{
#endif

/// Device that should be defined before everything

// JLX: the two following values are taken verbatim from OS21U.PDF
#define OS21_SUCCESS		0
#define OS21_FAILURE		(-1)

#define true				TRUE
#define false				FALSE
#define LONG_LONG_MAX		_I64_MAX
#define _LONG_LONG			__int64	
typedef unsigned char		boolean;	

#ifndef ST_OSWINCE
#define ST_OSWINCE
#endif

#undef CONFIG_POSIX // 'cause it's not true (JLX)



//! Include collection toolbox
#include "WCE_Collection.h"

// include OS21 to WINCE wrapper for general externs defines etc... 
#include "OS21WrapperMisc.h"

// include OS21 to WINCE wrapper for general externs defines etc... 
#include "OS21WrapperConsole.h"


// include OS21 to WINCE wrapper for Memory management
#include "OS21WrapperMemory.h"

// include OS21 to WINCE wrapper for thread
#include "OS21WrapperTask.h"

// include OS21 to WINCE Wrapper for Clock System Time and Ticks
#include "OS21WrapperClock.h"

// include OS21 to WINCE wrapper for semaphore and FIFO
#include "OS21WrapperSemaphore.h"

// include OS21 to WINCE wrapper for semaphore and FIFO
#include "OS21WrapperMutex.h"

// include OS21 to WINCE Wrapper for message Queue
#include "OS21WrapperMessageQueue.h"

// include 0S21 to WINCE Wrapper for debug functions
#include "OS21DebugWrapper.h"

//#include "stddefs.h"

// include OS21 to WINCE Wrapper for Clock System Time and Ticks
#include "OS21WrapperInterrupt.h"

// include OS21 to WINCE Wrapper for Files operations
#include "OS21WrapperFile.h"

#ifdef WINCE_USE_INLINE
#include "OS21WrapperInliner.h"
#endif


#define OS21_INTERRUPT_MB_LX_DPHI 1 //???? tempo MME
#ifdef __cplusplus
}
#endif




#endif //__TargetMachine__
