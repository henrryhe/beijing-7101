/*******************************************************************************

File name   : lc_pool.h

Description : pool manager interface

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
Sept 2003        Creation                                   TM

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __LC_POOL_H
#define __LC_POOL_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "lc_init.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

void laycompo_InitDataPool(
  laycompo_DataPoolDesc_t*    DataPool_p,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     ElemArray_p,
  void**                    HandleArray_p
);

ST_ErrorCode_t laycompo_GetElement(
  laycompo_DataPoolDesc_t* DataPool_p,
  void**                 Handle_p
);

ST_ErrorCode_t laycompo_ReleaseElement(
  laycompo_DataPoolDesc_t*  DataPool_p,
  void*                   Handle_p
);



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LC_POOL_H */

/* End of lc_pool.h */
