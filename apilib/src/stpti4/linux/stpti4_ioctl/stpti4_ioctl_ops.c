/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description :
 *
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>      /* File operations (fops) defines */
#include <linux/ioport.h>  /* Memory/device locking macros   */
#include <linux/poll.h>    /* Poll and select support        */
#include <linux/errno.h>   /* Defines standard error codes */

#if defined( CONFIG_BPA2)
#include <linux/bpa2.h>
#elif defined( CONFIG_BIGPHYS_AREA)
#include <linux/bigphysarea.h>
#else
#error Update your Kernel config to include BigPhys Area.
#endif

#include "linuxcommon.h"
#include "pti_linux.h"

#include "stpti4_ioctl_types.h"       /* Modules specific defines */

#include "stpti4_ioctl_cfg.h"  /* PTI specific configurations. */

/*** PROTOTYPES **************************************************************/

#include "stpti4_ioctl_fops.h"

/*** GLOBAL VARIABLES *********************************************************/

/*** LOCAL TYPES *********************************************************/

/*** LOCAL CONSTANTS ********************************************************/

#define KMALLOC_LIMIT 131072

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

/*=============================================================================

   stpti4_ioctl_mmap

   Map a buffer into user space.

  ===========================================================================*/
int stpti4_ioctl_mmap (struct file *filp, struct vm_area_struct *vma)
{
    U32 size = vma->vm_end - vma->vm_start;
    void *buffer;
    
#ifdef CONFIG_BIGPHYS_AREA
    if (vma->vm_pgoff)
    {
        U32 pages = size >> PAGE_SHIFT;
        if (size % PAGE_SIZE)
            pages++;
    
        buffer = bigphysarea_alloc_pages(pages, 0, GFP_KERNEL | __GFP_DMA);
    }
    else
#endif
    {
        if( size > KMALLOC_LIMIT )
            printk(KERN_WARNING"stpti4_ioctl_mmap: attempted to kmalloc an area in excess of %d bytes - required: %d\n", KMALLOC_LIMIT, size);
    
        buffer = kmalloc( size, GFP_KERNEL | __GFP_DMA );
    }

    if( buffer == NULL )
    {
        printk(KERN_ALERT"stpti4_ioctl_mmap: Failed to allocate memory for buffer. Size %d\n", size);
        return -EAGAIN;
    }

    if (remap_pfn_range(vma, vma->vm_start, 
                        virt_to_phys(buffer) >> PAGE_SHIFT, 
                        size,
                        vma->vm_page_prot))
    {
        printk(KERN_ALERT"stpti4_ioctl_mmap: Failed to remap memory for buffer. Size %d\n", size);
        return -EAGAIN;
    }

    return 0;
}
