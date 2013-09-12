/*******************************************************************************

File name   : Sub_copy.h

Description : Sub Picture viewport header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
15 Dec 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUB_COPY_H
#define __SUB_COPY_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t layersub_MaskedCopyBlock2D(
        void *SrcAddr_p,
        U32 SrcWidth,
        U32 SrcHeight,
        U32 SrcPitch,
        void *DstAddr_p,
        U32 DstPitch,
        U8 BitsSLeft,
        U8 BitsSRight,
        U8 BitsDLeft,
        U8 BitsDRight,
        BOOL PlusHalfSrcPitch,
        BOOL PlusHalfDstPitch
);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUB_COPY_H */


