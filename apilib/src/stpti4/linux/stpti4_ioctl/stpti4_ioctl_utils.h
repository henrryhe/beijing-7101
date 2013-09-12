/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.
 *
 *  Module      : stpti4_ioctl_utils
 *  Date        : 01-08-2007
 *  Author      : WAINJ
 *  Description : A few utility routines.
 *
 *****************************************************************************/

#include "stpti.h"
#include "stpti4_ioctl.h"

#if defined( STPTI_FRONTEND_HYBRID )

typedef struct stpti4_SWTSNodeDescriptor_s
{
    U8                       *UserData_p;
    U32                       SizeOfData;
    struct page             **SourcePages;
    unsigned int              SourcePageCount;

    STPTI_FrontendSWTSNode_t *KernelNodeList;

} stpti4_SWTSNodeDescriptor_t;


STPTI_FrontendSWTSNode_t *stpti4_ioctl_GenerateKernelSWTSNodeList ( struct task_struct           *Task,
                                                                    STPTI_FrontendSWTSNode_t     *UserSWTSNodeList_p,
                                                                    U32                           NumberOfSWTSNodes,
                                                                    stpti4_SWTSNodeDescriptor_t **SWTSNodeDescriptorList_pp,
                                                                    U32                          *NumberOfPageNodes );


void stpti4_ioctl_FreeKernelSWTSNodeList ( STPTI_FrontendSWTSNode_t     *SWTSNodeList_p,
                                           U32                           NumberOfSWTSNodes,
                                           stpti4_SWTSNodeDescriptor_t  *SWTSNodeDescriptorList_p );

#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */

