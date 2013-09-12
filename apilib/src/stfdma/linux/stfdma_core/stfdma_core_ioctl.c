/*****************************************************************************
 *
 *  Module      : fdma
 *  Date        : 29-02-2005
 *  Author      : WALKERM
 *  Description :
 *
 *****************************************************************************/

/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */

#include <linux/errno.h>         /* Defines standard error codes */

#include <asm/uaccess.h>         /* User space access routines */

#include <linux/sched.h>         /* User privilages (capabilities) */

#include "stfdma_core_types.h"    /* Module specific types */

#include "stfdma_core.h"          /* Defines ioctl numbers */

/*** PROTOTYPES **************************************************************/


#include "stfdma_core_fops.h"

/*** METHODS *****************************************************************/

static  U8*                 pKernSCList     = NULL;
static  STFDMA_SCEntry_t*   pUserSCList     = NULL;
static  U32                 SCListSize      = 0;
static  U8*                 pKernESBuffer   = NULL;
static  void*               pUserESBuffer   = NULL;
static  U32                 ESBufferSize    = 0;

/*=============================================================================

   fdma_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'fdma.h'.

  ===========================================================================*/
int fdma_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    printk(KERN_ALERT "ioctl command [0X%08X]\n", cmd);

    /*** Check the user privalage ***/

    /* if (!capable (CAP_SYS_ADMIN)) */ /* See 'sys/sched.h' */
    /* {                             */
    /*     return (-EPERM);          */
    /* }                             */

    /*** Check the command is for this module ***/

    if (_IOC_TYPE(cmd) != FDMA_TYPE) {

        /* Not an ioctl for this module */
        err = -ENOTTY;
        goto fail;
    }

    /*** Check the access permittions ***/

    if ((_IOC_DIR(cmd) & _IOC_READ) && !(filp->f_mode & FMODE_READ)) {

        /* No Read permittions */
        err = -ENOTTY;
        goto fail;
    }

    if ((_IOC_DIR(cmd) & _IOC_WRITE) && !(filp->f_mode & FMODE_WRITE)) {

        /* No Write permittions */
        err = -ENOTTY;
        goto fail;
    }

    /*** Execute the command ***/

    switch (cmd) {

        case FDMA_CMD0 :
            break;

        case FDMA_CMD1 :
            {
                int data = 1;

                if ((err = put_user(data, (int*)arg))) {

                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case FDMA_CMD2 :
            {
                long data;

                if ((err = get_user(data, (long*)arg))) {

                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;


        case FDMA_CMD3 :
            {
                double data;

                if (copy_from_user(&data, (double*)arg, sizeof(data))) {

                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                data = 10.0;

                if (copy_to_user((double*)arg, &data, sizeof(data))) {

                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }

            break;

        case  IO_STFDMA_StartGenericTransfer:
            {
                StartGenericTransferParams Params;

                if (copy_from_user(&Params, (StartGenericTransferParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_StartGenericTransfer(&(Params.TransferParams),
                    &(Params.TransferId)))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((StartGenericTransferParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_StartTransfer:
            {
                StartTransferParams Params;

                if (copy_from_user(&Params, (StartTransferParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_StartTransfer(&(Params.TransferParams),
                    &(Params.TransferId)))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((StartTransferParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ResumeTransfer:
            {
                STFDMA_TransferId_t Param;

                if (copy_from_user(&Param, (STFDMA_TransferId_t*)arg, sizeof(Param)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_ResumeTransfer(Param))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_FlushTransfer:
            {
                STFDMA_TransferId_t Param;

                if (copy_from_user(&Param, (STFDMA_TransferId_t*)arg, sizeof(Param)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_FlushTransfer(Param))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_AbortTransfer:
            {
                STFDMA_TransferId_t Param;

                if (copy_from_user(&Param, (STFDMA_TransferId_t*)arg, sizeof(Param)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_AbortTransfer(Param))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_AllocateContext:
            {
                STFDMA_ContextId_t Param;

                if (ST_NO_ERROR != STFDMA_AllocateContext(&Param))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_ContextId_t*)arg, &Param, sizeof(Param)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_DeallocateContext:
            {
                STFDMA_ContextId_t Param;

                if (copy_from_user(&Param, (STFDMA_ContextId_t*)arg, sizeof(Param)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_DeallocateContext(Param))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextGetSCList:
            {
                STFDMA_ContextGetSCListParams   Params;
                STFDMA_SCEntry_t*               pSCList;

                if ((NULL == pKernSCList) || (NULL == pUserSCList))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_from_user(&Params, (STFDMA_ContextGetSCListParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_ContextGetSCList(Params.ContextId,
                    &pSCList,
                    &(Params.Size),
                    &(Params.Overflow)))
                {
                    err = -EFAULT;
                    goto fail;
                }

                Params.SCList = pUserSCList + ((U32)pKernSCList - (U32)pSCList);
                if (copy_to_user(pUserSCList, pKernSCList, SCListSize))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_ContextGetSCListParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextSetSCList:
            {
                STFDMA_ContextSetSCListParams   Params;

                if (copy_from_user(&Params, (STFDMA_ContextSetSCListParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                pKernSCList = kmalloc(Params.Size, GFP_KERNEL);
                if (NULL == pKernSCList)
                {
                    err = -EFAULT;
                    goto fail;
                }

                pUserSCList = Params.SCList;
                SCListSize = Params.Size;

                if (ST_NO_ERROR != STFDMA_ContextSetSCList(Params.ContextId,
                    (STFDMA_SCEntry_t*)pKernSCList,
                    Params.Size))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_ContextSetSCListParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextGetSCState:
            {
                STFDMA_ContextGetSCStateParams Params;

                if (copy_from_user(&Params, (STFDMA_ContextGetSCStateParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_ContextGetSCState(Params.ContextId,
                    &(Params.State),
                    Params.Range))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_ContextGetSCStateParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextSetSCState:
            {
                STFDMA_ContextSetSCStateParams Params;

                if (copy_from_user(&Params, (STFDMA_ContextSetSCStateParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_ContextGetSCState(Params.ContextId,
                    &(Params.State),
                    Params.Range))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextSetESBuffer:
            {
                STFDMA_ContextSetESBufferParams   Params;

                if (copy_from_user(&Params, (STFDMA_ContextSetESBufferParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                pKernESBuffer = kmalloc(Params.Size, GFP_KERNEL);
                if (NULL == pKernESBuffer)
                {
                    err = -EFAULT;
                    goto fail;
                }

                pUserESBuffer = Params.Buffer;
                ESBufferSize = Params.Size;

                if (ST_NO_ERROR != STFDMA_ContextSetESBuffer(Params.ContextId,
                    (void*)pKernESBuffer,
                    Params.Size))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextSetESReadPtr:
            {
                STFDMA_ContextSetESReadPtrParams    Params;
                void*                               KerRead;

                if (copy_from_user(&Params, (STFDMA_ContextSetESReadPtrParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                KerRead = pKernESBuffer + ((U32) pUserESBuffer - (U32)Params.Read);

                if (ST_NO_ERROR != STFDMA_ContextSetESReadPtr(Params.ContextId,
                    KerRead))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextSetESWritePtr:
            {
                STFDMA_ContextSetESWritePtrParams   Params;
                void*                               KerWrite;

                if (copy_from_user(&Params, (STFDMA_ContextSetESWritePtrParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                KerWrite = pKernESBuffer + ((U32) pUserESBuffer - (U32)Params.Write);

                if (ST_NO_ERROR != STFDMA_ContextSetESWritePtr(Params.ContextId,
                    KerWrite))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextGetESReadPtr:
            {
                STFDMA_ContextGetESReadPtrParams    Params;
                void*                               KerRead = NULL;

                if (ST_NO_ERROR != STFDMA_ContextSetESReadPtr(Params.ContextId,
                    KerRead))
                {
                    err = -EFAULT;
                    goto fail;
                }

                Params.Read = (void*)((U8*)pUserESBuffer + ((U32)pKernESBuffer - (U32)KerRead));
                if (copy_to_user((STFDMA_ContextGetESReadPtrParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_ContextGetESWritePtr:
            {
                STFDMA_ContextGetESWritePtrParams   Params;
                void*                               KerWrite = NULL;

                if (ST_NO_ERROR != STFDMA_ContextGetESWritePtr(Params.ContextId,
                    KerWrite,
                    &(Params.Overflow)))
                {
                    err = -EFAULT;
                    goto fail;
                }

                Params.Write = (void*)((U8*)pUserESBuffer + ((U32)pKernESBuffer - (U32)KerWrite));
                if (copy_to_user((STFDMA_ContextGetESWritePtrParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_GetTransferStatus:
            {
                STFDMA_GetTransferStatusParams Params;

                if (copy_from_user(&Params, (STFDMA_GetTransferStatusParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_GetTransferStatus(Params.TransferId,
                    &(Params.TransferStatus)))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_GetTransferStatusParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_LockChannelInPool:
            {
                STFDMA_LockChannelInPoolParams Params;

                if (copy_from_user(&Params, (STFDMA_LockChannelInPoolParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_LockChannelInPool(Params.Pool, &(Params.ChannelId), Params.DeviceNo))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_LockChannelInPoolParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_LockChannel:
            {
                STFDMA_LockChannelParams Params;

                if (copy_from_user(&Params, (STFDMA_LockChannelParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_LockChannel(&(Params.ChannelId), Params.DeviceNo))
                {
                    err = -EFAULT;
                    goto fail;
                }

                if (copy_to_user((STFDMA_LockChannelParams*)arg, &Params, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_UnlockChannel:
            {
                STFDMA_UnlockChannelParams Params;

                if (copy_from_user(&Params, (STFDMA_UnlockChannelParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_UnlockChannel(Params.ChannelId, Params.DeviceNo))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_GetRevision:
            {
                ST_Revision_t Param = STFDMA_GetRevision();

                if (copy_to_user((ST_Revision_t*)arg, &Param, sizeof(Param)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        case  IO_STFDMA_SetAddDataRegionParameter:
            {
                STFDMA_SetAddDataRegionParameterParams Params;

                if (copy_from_user(&Params, (STFDMA_SetAddDataRegionParameterParams*)arg, sizeof(Params)))
                {
                    /* Data remaining to transfer */
                    /* Invalid user space address */
                    err = -EFAULT;
                    goto fail;
                }

                if (ST_NO_ERROR != STFDMA_SetAddDataRegionParameter(Params.DeviceNo, Params.RegionNo, Params.ADRParameter, Params.Value))
                {
                    err = -EFAULT;
                    goto fail;
                }
            }
            break;

        default :
            /*** Inappropriate ioctl for the device ***/
            err = -ENOTTY;
            goto fail;
    }


    printk(KERN_ALERT "fdma - ioctl complete\n");
    return (0);

    /**************************************************************************/

    /*** Clean up on error ***/

fail:
    return (err);
}


