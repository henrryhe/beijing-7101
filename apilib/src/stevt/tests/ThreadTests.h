/******************************************************************************

File Name : ThreadTests.h

Description: Multithreading safety and performance tests for STEVT

******************************************************************************/

#ifndef  THREAD_TEST_H
#define  THREAD_TEST_H

    /* Includes ------------------------------------------------------------ */

#include "stevt.h"

    /* Exported Types ------------------------------------------------------ */

    /* Exported Constants -------------------------------------------------- */

    /* Exported Variables -------------------------------------------------- */

    /* Exported Macros ----------------------------------------------------- */

    /* Exported Function Prototypes ---------------------------------------- */

/* ListLock.c */
extern ST_ErrorCode_t TESTEVT_ListLock( void );

/* ConcNotify.c */
extern ST_ErrorCode_t TESTEVT_ConcNotify( void );

#endif
