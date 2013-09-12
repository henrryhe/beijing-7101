/*******************************************************************************

File name   : blt_pool.c

Description : Functions of the data pool Manager

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                               Name
----               ------------                               ----
13 Jun 2000        Creation                                   TM

*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <stdlib.h>
#endif
#include "stddefs.h"
#include "blt_init.h"
#include "blt_com.h"
#include "blt_pool.h"
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <string.h>
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables -------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */
/*******************************************************************************
Name        : stblit_InitDataPool
Description : initializes a generic datapool
Parameters  :
Assumptions :
Limitations : No error checking is performed
Returns     :
*******************************************************************************/
void stblit_InitDataPool(
  stblit_DataPoolDesc_t*    DataPool_p,
  U32                       NbElem,
  U32                       ElemSize,
  void*                     ElemArray_p,
  void**                    HandleArray_p)
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
Name        : stblit_GetElement
Description : gets an element from the data pool returning its handle
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_GetElement(
  stblit_DataPoolDesc_t* DataPool_p,
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
Name        : stblit_ReleaseElement
Description :
Parameters  :
Assumptions :
Limitations : upper level must ensure an handle is released only once
              upper level must also ensure handle validity
Returns     :
*******************************************************************************/
ST_ErrorCode_t stblit_ReleaseElement(
  stblit_DataPoolDesc_t*  DataPool_p,
  void*                   Handle)
{
  DataPool_p->NbFreeElem++;
  DataPool_p->HandleArray_p[DataPool_p->NbFreeElem - 1] = Handle;
  return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : stblit_ReleaseNode
Description :
Parameters  :
Assumptions : Access control protection done at upper level
              Valid NodeHandle
Limitations :
Returns     :
*******************************************************************************/
void stblit_ReleaseNode(stblit_Device_t* Device_p, stblit_NodeHandle_t NodeHandle)
{
    U32 i;

    if (((stblit_Node_t*)NodeHandle)->JobHandle == STBLIT_NO_JOB_HANDLE)  /* Single blit node */
    {
        /* Reset Node to  0*/
/*        memset((void*)NodeHandle, 0, sizeof(stblit_Node_t));*/
        for (i=0;i < sizeof(stblit_Node_t) /4 ;i++ )
        {
/*            *((U32*)(NodeHandle + 4*i)) = 0;*/
            STSYS_WriteRegMem32LE((void*)((U32)NodeHandle + 4 * i),0);
        }

        stblit_ReleaseElement(&(Device_p->SBlitNodeDataPool), (void*) NodeHandle);
    }
    else  /* Node from job */
    {
        /* Reset Node to  0*/
/*        memset((void*)NodeHandle, 0, sizeof(stblit_Node_t));*/
        for (i=0;i < sizeof(stblit_Node_t) /4 ;i++ )
        {
/*            *((U32*)(NodeHandle + 4*i)) = 0;*/
            STSYS_WriteRegMem32LE((void*)((U32)NodeHandle + 4 * i),0);
        }

        stblit_ReleaseElement(&(Device_p->JBlitNodeDataPool), (void*) NodeHandle);
    }
}


/*******************************************************************************
Name        : stblit_ReleaseListNode
Description : Being given the last node of the list, it releases and reset to 0 all nodes.
Parameters  :
Assumptions : The PreviousNode field of the first node in the list is STBLIT_NO_JOB_HANDLE
              Access control protection done at upper level. It is due to the fact that this function must
              not deschedule at all (used in backend module!).
Limitations :
Returns     : Nothing
*******************************************************************************/
void stblit_ReleaseListNode(stblit_Device_t* Device_p, stblit_NodeHandle_t NodeHandle)
{
    stblit_NodeHandle_t  PreviousHandle;

    while (NodeHandle != STBLIT_NO_NODE_HANDLE)
    {
        PreviousHandle = ((stblit_Node_t*)NodeHandle)->PreviousNode;
        stblit_ReleaseNode(Device_p,NodeHandle);
        NodeHandle = PreviousHandle;
    }
}



/* End of blt_pool.c */


