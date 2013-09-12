/*******************************************************************************

File name   : h264preproc_dump.h

Description : Prototype functions of "h264preproc_dump.c"

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
16/12/2003         Creation                                         Didier SIRON

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __H264_PREPROC_DUMP_H
#define __H264_PREPROC_DUMP_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "h264preproc.h"

/* Exported Defines --------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

extern ST_ErrorCode_t PPDMP_Init(void);
extern ST_ErrorCode_t PPDMP_Term(void);
extern ST_ErrorCode_t PPDMP_DumpCommand(H264Preproc_Context_t *Preproc_Context_p);

extern ST_ErrorCode_t BBDMP_Init(void);
extern ST_ErrorCode_t BBDMP_Term(void);
extern ST_ErrorCode_t BBDMP_DumpBitBuffer(H264Preproc_Context_t *Preproc_Context_p);

#endif /* #ifndef __H264_PREPROC_DUMP_H */

/* End of h264preproc_dump.h */
