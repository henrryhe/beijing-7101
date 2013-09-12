/*******************************************************************************

File name   : pool_mng.c

Description : Functions of the data pool Manager

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
2000-06-07         generalized, to be shared between          Adriano Melis
                   different drivers

Pool Manager functions
----------------------
stlayer_InitDataPool
stlayer_GetElement
stlayer_ReleaseElement

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "pool_mng.h"
#include "layergfx.h"


/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : stlayer_InitDataPool
Description : initializes a generic datapool
Parameters  :
Assumptions :
Limitations : No error checking is performed
Returns     :
*******************************************************************************/
void stlayer_InitDataPool(
  stlayer_DataPoolDesc_t*   dataPool,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     Pool,
  Handle_t*                 Handle)
{
  U32 i;

  dataPool->NbFreeElem = NbElem;
  dataPool->Pool = Pool;
  dataPool->Handle_p = Handle;
  for(i=0; i<NbElem; i++)
  {
    dataPool->Handle_p[i] = (Handle_t)Pool + ElemSize*(NbElem - 1 -i);
  }
}

/*******************************************************************************
Name        : stlayer_GetElement
Description : gets an element from the data pool, returning its handle
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_GetElement(
  stlayer_DataPoolDesc_t* Pool,
  Handle_t*               Handle)
{

  if(Pool->NbFreeElem > 0)
  {
    *Handle = Pool->Handle_p[Pool->NbFreeElem - 1];
    Pool->Handle_p[Pool->NbFreeElem - 1] = (U32)0;
    Pool->NbFreeElem--;
    return(ST_NO_ERROR);
  }
  else
  {
    layergfx_Defense(ST_ERROR_NO_FREE_HANDLES);
    return(ST_ERROR_NO_FREE_HANDLES);
  }
}

/*******************************************************************************
Name        : stlayer_ReleaseElement
Description : 
Parameters  :
Assumptions :
Limitations : upper level must ensure an handle is released only once
              upper level must also ensure handle validity
Returns     :
*******************************************************************************/
ST_ErrorCode_t stlayer_ReleaseElement(
  stlayer_DataPoolDesc_t*  Pool,
  Handle_t                 Handle)
{
  Pool->NbFreeElem++;
  Pool->Handle_p[Pool->NbFreeElem - 1] = Handle;
  return(ST_NO_ERROR);
}

/******************************************************************************/
/* End of pool_mng.c */


