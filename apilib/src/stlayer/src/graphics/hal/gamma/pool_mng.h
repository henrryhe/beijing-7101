/*******************************************************************************

File name   : pool_mng.h

Description : pool manager interface

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __POOL_MANAGER_H
#define __POOL_MANAGER_H


/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */
typedef U32 Handle_t;

typedef struct
{
  U32         NbFreeElem;
  void*       Pool;
  Handle_t*   Handle_p;
} stlayer_DataPoolDesc_t;

/* Exported Functions ------------------------------------------------------- */
void stlayer_InitDataPool(
  stlayer_DataPoolDesc_t*   dataPool,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     Pool,
  Handle_t*                 Handle
);
                                   
ST_ErrorCode_t stlayer_GetElement(
  stlayer_DataPoolDesc_t* Pool,
  Handle_t*               Handle
);

ST_ErrorCode_t stlayer_ReleaseElement(
  stlayer_DataPoolDesc_t*  Pool,
  Handle_t                 Handle
);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __POOL_MANAGER_H */

/* End of pool_mng.h */
