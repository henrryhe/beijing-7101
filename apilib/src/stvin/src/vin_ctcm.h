/*******************************************************************************

File name   : vin_ctcm.h

Description : Common things commands header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
21 Mar 2000        Created                                           HG
26 Jul 2001        Duplicate/Extract from STVID                      JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __COMMON_THINGS_COMMANDS_H
#define __COMMON_THINGS_COMMANDS_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */

typedef struct {
    U8 * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    U8 * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} CommandsBuffer_t;

typedef struct {
    U32 * Data_p;
    U32  TotalSize;
    U32  UsedSize;
    U32  MaxUsedSize;       /* To monitor max size used in commands buffer */
    U32 * BeginPointer_p;    /* This is a circular buffer, with BeginPointer_p and UsedSize we know what is in */
} CommandsBuffer32_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t vincom_PopCommand(CommandsBuffer_t * const Buffer_p, U8 * const Data_p);
void vincom_PushCommand(CommandsBuffer_t * const Buffer_p, const U8 Data);
ST_ErrorCode_t vincom_PopCommand32(CommandsBuffer32_t * const Buffer_p, U32 * const Data_p);
void vincom_PushCommand32(CommandsBuffer32_t * const Buffer_p, const U32 Data);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __COMMON_THINGS_COMMANDS_H */

/* End of vin_ctcm.h */
