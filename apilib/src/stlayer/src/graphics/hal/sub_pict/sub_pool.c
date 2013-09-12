/*******************************************************************************

File name   : sub_pool.c

Description : Functions of the data pool Manager

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
13 Jun 2000        Creation                                   TM

*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#include <stdlib.h>
#include "stddefs.h"
#include "sub_init.h"
#include "sub_pool.h"
#include <string.h>

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : layersub_InitDataPool
Description : initializes a generic datapool
Parameters  :
Assumptions :
Limitations : No error checking is performed
Returns     :
*******************************************************************************/
void layersub_InitDataPool(
    layersub_DataPoolDesc_t*      DataPool_p,
    U32                           NbElem,
    U32                           ElemSize,
    void*                         ElemArray_p,
    void**                        HandleArray_p)
{
  U32 i;

  DataPool_p->NbFreeElem      = NbElem;
  DataPool_p->NbElem          = NbElem;
  DataPool_p->ElemSize        = ElemSize;
  DataPool_p->ElemArray_p     = ElemArray_p;
  DataPool_p->HandleArray_p   = HandleArray_p;

  /* Initialize Handle array content*/
  for( i=0; i < NbElem; i++)
    DataPool_p->HandleArray_p[i] = (void*) ((U32)ElemArray_p + ElemSize*(NbElem - 1 -i));
}

/*******************************************************************************
Name        : layersub_GetElement
Description : gets an element from the data pool returning its handle
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_GetElement(
  layersub_DataPoolDesc_t* DataPool_p,
  void**                 Handle_p)
{

  if(DataPool_p->NbFreeElem > 0)
  {
    *Handle_p = (void*) DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1];
    DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1] = NULL;
    DataPool_p->NbFreeElem--;
    return(ST_NO_ERROR);
  }
  else
  {
    return(ST_ERROR_NO_FREE_HANDLES);
  }
}

/*******************************************************************************
Name        : layersub_ReleaseElement
Description :
Parameters  :
Assumptions :
Limitations : upper level must ensure an handle is released only once
              upper level must also ensure handle validity
Returns     :
*******************************************************************************/
ST_ErrorCode_t layersub_ReleaseElement(
  layersub_DataPoolDesc_t*  DataPool_p,
  void*                   Handle)
{
  DataPool_p->NbFreeElem++;
  DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1] = Handle;
  return(ST_NO_ERROR);
}


/* End of sub_pool.c */


