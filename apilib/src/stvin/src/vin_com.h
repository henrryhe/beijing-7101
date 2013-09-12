/*******************************************************************************

File name   : vin_com.h

Description : Video Input common definitions header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
13 Jul 2000        Created                                           HG
26 Jul 2001        Duplicate/Extract from STVID                     JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIN_COMON_DEFINITIONS_H
#define __VIN_COMON_DEFINITIONS_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stevt.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif


/* Exported Constants ------------------------------------------------------- */
#define STVIN_IRQ_NAME          "stvin"
#ifdef ST_OSLINUX
     #define STVIN_MAPPING_WIDTH      0x100    /* Maps DVP registers area */
#endif

#ifndef STVIN_TASK_PRIORITY_INPUT
#define STVIN_TASK_PRIORITY_INPUT  2
#endif /* STVIN_TASK_PRIORITY_INPUT */

#ifdef ST_OS20
    #ifndef STVIN_TASK_STACK_SIZE_INPUT
    #define STVIN_TASK_STACK_SIZE_INPUT            4096
    #endif
#endif /* ST_OS20 */
#if defined ST_OS21 || defined ST_OSLINUX
    #ifndef STVIN_TASK_STACK_SIZE_INPUT
    #define STVIN_TASK_STACK_SIZE_INPUT            16384
    #endif
#endif /* ST_OS21 */


/* Exported Types ----------------------------------------------------------- */

typedef struct {
    U32  Mask;          /* Always contains the value of the interrupt mask, avoids reading it everytime */
    U32  Status;
    S32  EnableCount;
    BOOL InstallHandler; /* FALSE when using interrupt events, TRUE when installing handler inside video */
    U32  Level;     /* Valid if InstallHandler is TRUE */
    U32  Number;    /* Valid if InstallHandler is TRUE */
    STEVT_EventID_t EventID; /* Valid if InstallHandler is TRUE */
} VINCOM_Interrupt_t;

typedef struct {
    tdesc_t TaskDesc;
    void* TaskStack;
    task_t*  Task_p;
    BOOL     IsRunning;          /* TRUE if task is running, FALSE otherwise */
    BOOL     ToBeDeleted;        /* Set TRUE to ask task to end in order to delete it */
} VINCOM_Task_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIN_COMON_DEFINITIONS_H */

/* End of vin_com.h */


