/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2007.  All rights reserved.
 *
 *  Module      : stpti4_ioctl_utils
 *  Date        : 01-08-2007
 *  Author      : WAINJ
 *  Description : A few utility routines.
 *
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#if defined( STPTI_DEBUG_IOCTL_UTILS )
#define STTBX_PRINT
#endif

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */
#include <linux/errno.h>         /* Defines standard error codes */
#include <asm/uaccess.h>         /* User space access routines */
#include <linux/sched.h>         /* User privilages (capabilities) */
#include <linux/random.h>

#include <linux/mm.h>
#include <asm/page.h>
#include <linux/page-flags.h>
#include <linux/pagemap.h>


#include "stpti4_ioctl_types.h"    /* Module specific types */

#include "stpti4_ioctl.h"          /* Defines ioctl numbers */


#include "stpti.h"   /* A STAPI ioctl driver. */

#include "stpti4_ioctl_utils.h"

#if defined( STPTI_FRONTEND_HYBRID )

/******************************************************************************
Function Name: stpti4_ioctl_GetUserPages()
  Description: Retrieves the list of pages corresponding to a userspace buffer
   Parameters: Task - The context for the data. Usually 'current'.
               Buffer - a pointer to the userspace buffer.
               Count - the size of the userspace buffer, in bytes.
               Pages - pointer to be populated with an array of pointers to page descriptors.
               PageCount - Number of pages returned in the 'Pages' array.
 Return Value: > 0 - Succeeded (value is number of pages mapped).
               < 0 - Failed
******************************************************************************/
static int stpti4_ioctl_GetUserPages( struct task_struct *Task,
                                      const char         *Buffer,
                                      size_t              Count,
                                      struct page      ***Pages,
                                      unsigned int       *PageCount )
{
    int Result = 0;
    int Index  = 0;

    U32 FirstPage = ( (U32)Buffer               & PAGE_MASK) >> PAGE_SHIFT;
    U32 LastPage  = (((U32)Buffer + Count - 1 ) & PAGE_MASK) >> PAGE_SHIFT;

    STTBX_Print(( KERN_ALERT "%s( Task = 0x%08x, Buffer = 0x%08x, Count = %d, Pages = 0x%08x, PageCount = 0x%08x )\n",
                             __FUNCTION__, Task, Buffer, Count, Pages, PageCount ));

    *PageCount = LastPage - FirstPage + 1;

    #if defined( STPTI_DEBUG_IOCTL_UTILS )

        STTBX_Print(( KERN_ALERT"Count     %10d\n",   (U32)Count ));
        STTBX_Print(( KERN_ALERT"Buffer    0x%08x\n", (U32)Buffer ));
        STTBX_Print(( KERN_ALERT"FirstPage 0x%08x\n", (U32)FirstPage ));
        STTBX_Print(( KERN_ALERT"LastPage  0x%08x\n", (U32)LastPage ));
        STTBX_Print(( KERN_ALERT"PageCount %10d\n",   (U32)*PageCount ));

    #endif /* #if defined( STPTI_DEBUG_IOCTL_UTILS ) ... #endif */

    *Pages = kmalloc(*PageCount * sizeof(struct page *), GFP_KERNEL);

    if( Pages == NULL )
    {
        Result = -ENOMEM;
    }
    else
    {
        down_read( &Task->mm->mmap_sem );

        #if defined( STPTI_DEBUG_IOCTL_UTILS )
        {
            STTBX_Print(( KERN_ALERT"Task               0x%08x\n", (U32)Task ));
            STTBX_Print(( KERN_ALERT"Task->mm           0x%08x\n", (U32)Task->mm ));
            STTBX_Print(( KERN_ALERT"Buffer & PAGE_MASK 0x%08x\n", (U32)Buffer & PAGE_MASK ));
            STTBX_Print(( KERN_ALERT"READ               %10d\n",   (U32)READ ));
            STTBX_Print(( KERN_ALERT"Pages              0x%08x\n", (U32)Pages ));
        }
        #endif /* #if defined( STPTI_DEBUG_IOCTL_UTILS ) ... #endif */

        Result = get_user_pages( Task,
                                 Task->mm,
                                 (unsigned long)Buffer & PAGE_MASK,
                                *PageCount,
                                 READ,
                                 0,
                                *Pages,
                                 NULL );

        up_read( &Task->mm->mmap_sem );

        if( Result < *PageCount)
        {
            printk( KERN_ALERT "%s: Failed to get user pages\n", __FUNCTION__ );
            for( Index = 0; Index < Result; Index++ )
            {
                page_cache_release((*Pages)[Index]);
            }

            kfree(*Pages);
            *Pages = NULL;
            Result = -EINVAL;
        }
    }

    return Result;
}


/******************************************************************************
Function Name: stpti4_ioctl_FreeUserPages()
  Description: Releases pages cached by stpti4_ioctl_GetUserPages.
   Parameters: Pages - pointer to an array of pointers to page descriptors.
               PageCount - Number of page descriptors in the 'Pages' array.
               Dirty - If the mapped pages are likely to have changed, tag them as 'Dirty'.
 Return Value: N/A
******************************************************************************/
static void stpti4_ioctl_FreeUserPages( struct page **Pages,
                                        unsigned int  PageCount,
                                        int           Dirty )
{
    int Index = 0;

    STTBX_Print(( KERN_ALERT "%s( Pages = 0x%08x, PageCount = %d, Dirty = %d )\n", __FUNCTION__, (U32)Pages, PageCount, Dirty ));

    if( Pages != NULL )
    {
        for( Index = 0; Index < PageCount; Index++ )
        {
            if( Pages[Index] != NULL )
            {
                if( Dirty )
                {
                    if(! PageReserved( Pages[Index] ))
                    {
                        SetPageDirty( Pages[Index] );
                    }
                }

                page_cache_release( Pages[Index] );
            }
        }

        kfree(Pages);
    }
}


/******************************************************************************
Function Name: stpti4_ioctl_GetPageNodes()
  Description: Copies the attributes of each page node into the Node descriptor
   Parameters: SWTSNodeDescriptor_p - pointer to the node descriptor to receive the page data
Return Values: Positive value indicates number of pages processed - this should match the
               SourcePageCount member of the passed NodeDescriptor.  A negative value indicates
               an error occurred.
******************************************************************************/
static S32 stpti4_ioctl_GetPageNodes( stpti4_SWTSNodeDescriptor_t *SWTSNodeDescriptor_p )
{
    S32 Result          = 0;
    U32 StartAddress    = (U32) SWTSNodeDescriptor_p->UserData_p;
    U32 EndAddress      = (U32) StartAddress + SWTSNodeDescriptor_p->SizeOfData;
    U32 BeginOffset     = 0;
    U32 Length          = 0;
    U32 PageIndex       = 0;
    void* VirtAddress_p = NULL;

    STTBX_Print(( KERN_ALERT "%s( SWTSNodeDescriptor_p = 0x%08x )\n", __FUNCTION__, (U32)SWTSNodeDescriptor_p ));

    SWTSNodeDescriptor_p->KernelNodeList = (STPTI_FrontendSWTSNode_t *) kmalloc( sizeof( STPTI_FrontendSWTSNode_t ) * SWTSNodeDescriptor_p->SourcePageCount, GFP_KERNEL );

    if( SWTSNodeDescriptor_p->KernelNodeList == NULL )
    {
        Result = -ENOMEM;
    }
    else
    {
        while( StartAddress < EndAddress )
        {
            BeginOffset = StartAddress & ~PAGE_MASK;

            if( EndAddress - StartAddress >= ( PAGE_SIZE - BeginOffset ))
            {
                Length = PAGE_SIZE - BeginOffset;
            }
            else
            {
                Length = EndAddress - StartAddress;
            }

            #if defined( STPTI_DEBUG_IOCTL_UTILS )
            {
                STTBX_Print(( KERN_ALERT"%s: StartAddress = 0x%08x\n", __FUNCTION__, StartAddress));
                STTBX_Print(( KERN_ALERT"%s: EndAddress   = 0x%08x\n", __FUNCTION__, EndAddress  ));
                STTBX_Print(( KERN_ALERT"%s: BeginOffset  = 0x%08x\n", __FUNCTION__, BeginOffset ));
                STTBX_Print(( KERN_ALERT"%s: Length       = 0x%08x\n", __FUNCTION__, Length      ));

                STTBX_Print(( KERN_ALERT"%s: KMapping     = 0x%08x\n", __FUNCTION__, SWTSNodeDescriptor_p->SourcePages[PageIndex] ));
            }
            #endif /* #if defined( STPTI_DEBUG_IOCTL_UTILS ) ... #endif */

            VirtAddress_p = (void *) ((U32)kmap( SWTSNodeDescriptor_p->SourcePages[PageIndex] ) + BeginOffset );

            /* Note: we use the kernel virtual address below, as the stpti4_core function will call STOS_VirtToPhys */
            SWTSNodeDescriptor_p->KernelNodeList[PageIndex].Data_p     = (void *)VirtAddress_p;
            SWTSNodeDescriptor_p->KernelNodeList[PageIndex].SizeOfData = Length;

            #if defined( STPTI_DEBUG_IOCTL_UTILS )
            {
                STTBX_Print(( KERN_ALERT"%s: SWTSNodeDescriptor_p->NodeList[PageIndex].Data_p     = 0x%08x\n", __FUNCTION__, SWTSNodeDescriptor_p->KernelNodeList[PageIndex].Data_p     ));
                STTBX_Print(( KERN_ALERT"%s: SWTSNodeDescriptor_p->NodeList[PageIndex].SizeOfData = 0x%08x\n", __FUNCTION__, SWTSNodeDescriptor_p->KernelNodeList[PageIndex].SizeOfData ));
            }
            #endif /* #if defined( STPTI_DEBUG_IOCTL_UTILS ) ... #endif */

            /* Should this be unmapped so soon? */
            kunmap( SWTSNodeDescriptor_p->SourcePages[PageIndex] );

            StartAddress += Length;

            PageIndex++; /* Next Page */
        }

        Result = PageIndex;
    }

    return Result;
}


/******************************************************************************
Function Name: stpti4_ioctl_GenerateKernelSWTSNodeList()
  Description: Generates a list of kernel memory nodes for TSGDMA from the list
               of user nodes supplied.
   Parameters: Task - The context for the data. Usually 'current'.
               UserSWTSNodeList_p - The list of SWTS nodes from userspace.
               NumberOfSWTSNodes - The number of SWTS nodes from userspace.
               SWTSNodeDescriptorList_pp - Pointer to a pointer that will
                       contain the node descriptor list upon completion.  The
                       data contained with is used to free the memory later.
               NumberOfPageNodes - output value: the number of pages described
                       by SWTSNodeDescriptorList_pp.
       Return: Valid Pointer - Succeeded
               NULL          - Failed
******************************************************************************/
STPTI_FrontendSWTSNode_t *stpti4_ioctl_GenerateKernelSWTSNodeList ( struct task_struct           *Task,
                                                                    STPTI_FrontendSWTSNode_t     *UserSWTSNodeList_p,
                                                                    U32                           NumberOfSWTSNodes,
                                                                    stpti4_SWTSNodeDescriptor_t **SWTSNodeDescriptorList_pp,
                                                                    U32                          *NumberOfPageNodes )
{
    /* We will be passed a list of SWTS nodes via UserSWTSNodeList_p, each of which refers to a userspace buffer.
       Each userspace buffer will consist of a list of pages - in the kernel, each of these pages will become a node.

           User             Kernel (A)          Kernel (B)
       -------------      --------------      --------------

         List:              List:               List:
           Node 0             Node 0:
                                Page A              Node 0
                                Page B              Node 1
                                Page C              Node 2
                                Page D              Node 3
           Node 1             Node 1
                                Page A              Node 4
                                Page B              Node 5
           Node 2             Node 2
                                Page A              Node 6
                                Page B              Node 7
                                Page C              Node 8


       As the final number of nodes is unknown initially, a list of lists will be created to start with (A);
       Once this is complete, the nodes will be copied to a single list (B) to be returned.
    */

    STPTI_FrontendSWTSNode_t    *Result                   = NULL;
    U32                          TotalPageCount           = 0;
    ST_ErrorCode_t               ErrorCode                = ST_NO_ERROR;

    stpti4_SWTSNodeDescriptor_t *SWTSNodeDescriptorList_p = NULL;

    STTBX_Print(( KERN_ALERT "%s( Task = 0x%08x, UserSWTSNodeList_p = 0x%08x, NumberOfSWTSNodes = %d, SWTSNodeDescriptorList_pp = 0x%08x, NumberOfPageNodes = %d )\n",
                             __FUNCTION__, Task, UserSWTSNodeList_p, NumberOfSWTSNodes, SWTSNodeDescriptorList_pp, NumberOfPageNodes ));

    if( UserSWTSNodeList_p != NULL )
    {
        SWTSNodeDescriptorList_p = (stpti4_SWTSNodeDescriptor_t *)kmalloc( sizeof(stpti4_SWTSNodeDescriptor_t *) * NumberOfSWTSNodes, GFP_KERNEL );

        if( SWTSNodeDescriptorList_p != NULL )
        {
            U32 NodeIndex = 0;

            for( NodeIndex = 0; NodeIndex < NumberOfSWTSNodes; NodeIndex++ )
            {
                /* For each user node, */
                SWTSNodeDescriptorList_p[NodeIndex].UserData_p = UserSWTSNodeList_p[NodeIndex].Data_p;
                SWTSNodeDescriptorList_p[NodeIndex].SizeOfData = UserSWTSNodeList_p[NodeIndex].SizeOfData;

                if( stpti4_ioctl_GetUserPages( Task,
                                               SWTSNodeDescriptorList_p[NodeIndex].UserData_p,
                                               SWTSNodeDescriptorList_p[NodeIndex].SizeOfData,
                                              &SWTSNodeDescriptorList_p[NodeIndex].SourcePages,
                                              &SWTSNodeDescriptorList_p[NodeIndex].SourcePageCount ) > 0 )
                {
                    if( stpti4_ioctl_GetPageNodes( &SWTSNodeDescriptorList_p[NodeIndex] ) > 0 )
                    {
                        TotalPageCount += SWTSNodeDescriptorList_p[NodeIndex].SourcePageCount;
                    }
                    else
                    {
                        printk( KERN_ALERT "%s: stpti4_ioctl_GetPageNodes failed\n", __FUNCTION__ );
                        ErrorCode = ST_ERROR_NO_MEMORY;
                        break;
                    }
                }
                else
                {
                    printk( KERN_ALERT "%s: stpti4_ioctl_GetUserPages failed\n", __FUNCTION__ );
                    ErrorCode = ST_ERROR_NO_MEMORY;
                    break;
                }
            }

            if( ErrorCode == ST_NO_ERROR )
            {
                /* Create a single list of STPTI_FrontendSWTSNode_t objects from the list of lists... */
                Result = (STPTI_FrontendSWTSNode_t *) kmalloc( sizeof( STPTI_FrontendSWTSNode_t ) * TotalPageCount, GFP_KERNEL );

                if( Result != NULL )
                {
                    U32 PageIndex       = 0;
                    U32 NodeIndex       = 0;
                    U32 NodeListIndex   = 0;

                    STTBX_Print(( KERN_ALERT "%s: Generating new node list from page list...\n", __FUNCTION__ ));

                    for( NodeIndex = 0; NodeIndex < NumberOfSWTSNodes; NodeIndex++ )
                    {
                        /* For each Node descriptor, iterate through the node list... */
                        stpti4_SWTSNodeDescriptor_t *SWTSNodeDescriptor_p = &SWTSNodeDescriptorList_p[NodeIndex];

                        for( NodeListIndex = 0; NodeListIndex < SWTSNodeDescriptor_p->SourcePageCount; NodeListIndex++ )
                        {
                            Result[PageIndex].Data_p     = SWTSNodeDescriptor_p->KernelNodeList[NodeListIndex].Data_p;
                            Result[PageIndex].SizeOfData = SWTSNodeDescriptor_p->KernelNodeList[NodeListIndex].SizeOfData;

                            #if defined( STPTI_DEBUG_IOCTL_UTILS )
                            {
                                STTBX_Print(( KERN_ALERT "%s: Result[%d].Data_p     = 0x%08x\n", __FUNCTION__, PageIndex, Result[PageIndex].Data_p ));
                                STTBX_Print(( KERN_ALERT "%s: Result[%d].SizeOfData = 0x%08x\n", __FUNCTION__, PageIndex, Result[PageIndex].SizeOfData ));
                            }
                            #endif /* #if defined( STPTI_DEBUG_IOCTL_UTILS ) ... #endif */

                            PageIndex++;
                        }
                    }

                    if( PageIndex != TotalPageCount )
                    {
                        STTBX_Print(( KERN_ALERT "%s: Something is very wrong.  PageIndex = %d   TotalPageCount = %d.\n", __FUNCTION__, PageIndex, TotalPageCount ));
                    }
                }
            }

            /* If an error has occurred, tidy up here. */
            if( ErrorCode != ST_NO_ERROR )
            {
                stpti4_ioctl_FreeKernelSWTSNodeList( Result, NumberOfSWTSNodes, SWTSNodeDescriptorList_p );

                SWTSNodeDescriptorList_p = NULL;
                TotalPageCount = 0;
                Result = NULL;
            }
        }
    }

    *SWTSNodeDescriptorList_pp = SWTSNodeDescriptorList_p;
    *NumberOfPageNodes = TotalPageCount;

    return Result;
}


/******************************************************************************
Function Name: stpti4_ioctl_FreeKernelSWTSNodeList()
  Description: Frees and unmaps memory acquired by stpti4_ioctl_GenerateKernelSWTSNodeList.
   Parameters: Task - The context for the data. Usually 'current'.
               SWTSNodeList_p - The list of SWTS nodes to free.
               NumberOfSWTSNodes - The number of SWTS nodes to free.
               SWTSNodeDescriptorList_p - Pointer to data describing the pages
                       previously locked into memory.
       Return: N/A
******************************************************************************/
void stpti4_ioctl_FreeKernelSWTSNodeList ( STPTI_FrontendSWTSNode_t     *SWTSNodeList_p,
                                           U32                           NumberOfSWTSNodes,
                                           stpti4_SWTSNodeDescriptor_t  *SWTSNodeDescriptorList_p )
{
    U32 NodeIndex       = 0;

    STTBX_Print(( KERN_ALERT "%s( SWTSNodeList_p = 0x%08x, NumberOfSWTSNodes = %d, SWTSNodeDescriptorList_p = 0x%08x )\n", __FUNCTION__, SWTSNodeList_p, NumberOfSWTSNodes, SWTSNodeDescriptorList_p ));

    for( NodeIndex = 0; NodeIndex < NumberOfSWTSNodes; NodeIndex++ )
    {
        /* For each Node descriptor, free the node list... */
        stpti4_SWTSNodeDescriptor_t *SWTSNodeDescriptor_p = &SWTSNodeDescriptorList_p[NodeIndex];

        STTBX_Print(( KERN_ALERT "%s: SWTSNodeDescriptor_p = &SWTSNodeDescriptorList_p[%d] = 0x%08x\n", __FUNCTION__, NodeIndex, SWTSNodeDescriptor_p ));

        if( SWTSNodeDescriptor_p->KernelNodeList != NULL )
        {
            /* We don't need to flag the data as 'Dirty' as we're not expecting the mapped buffers to change. */
            stpti4_ioctl_FreeUserPages( SWTSNodeDescriptor_p->SourcePages, SWTSNodeDescriptor_p->SourcePageCount, 0 );

            kfree( SWTSNodeDescriptor_p->KernelNodeList );
        }
    }

    if( SWTSNodeDescriptorList_p != NULL )
    {
        kfree( SWTSNodeDescriptorList_p );
    }

    if( SWTSNodeList_p != NULL )
    {
        kfree( SWTSNodeList_p );
    }
}

#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */



