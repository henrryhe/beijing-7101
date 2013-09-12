/*******************************************************************************

File name   : sub_pool.h

Description : pool manager interface

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
18 Dec 2000        Creation                                   TM

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUB_POOL_H
#define __SUB_POOL_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "sub_init.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void layersub_InitDataPool(
  layersub_DataPoolDesc_t*    DataPool_p,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     ElemArray_p,
  void**                    HandleArray_p
);

ST_ErrorCode_t layersub_GetElement(
  layersub_DataPoolDesc_t* DataPool_p,
  void**                 Handle_p
);

ST_ErrorCode_t layersub_ReleaseElement(
  layersub_DataPoolDesc_t*  DataPool_p,
  void*                   Handle_p
);



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUB_POOL_H */

/* End of sub_pool.h */
