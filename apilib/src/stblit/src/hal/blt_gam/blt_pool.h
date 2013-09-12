/*******************************************************************************

File name   : blt_pool.h

Description : pool manager interface

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
13 Jun 2000        Creation                                   TM

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_POOL_H
#define __BLT_POOL_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "blt_init.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void stblit_InitDataPool(
  stblit_DataPoolDesc_t*    DataPool_p,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     ElemArray_p,
  void**                    HandleArray_p
);

ST_ErrorCode_t stblit_GetElement(
  stblit_DataPoolDesc_t* DataPool_p,
  void**                 Handle_p
);

ST_ErrorCode_t stblit_ReleaseElement(
  stblit_DataPoolDesc_t*  DataPool_p,
  void*                   Handle_p
);

void stblit_ReleaseNode(stblit_Device_t* Device_p, stblit_NodeHandle_t NodeHandle);
void stblit_ReleaseListNode(stblit_Device_t* Device_p, stblit_NodeHandle_t NodeHandle);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_POOL_H */

/* End of blt_pool.h */
