/*****************************************************************************
 *
 *  Module      : tsmerger_ioctl
 *  Date        : 24-07-2005
 *  Author      : STIEGLITZP
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
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

#include "tsmerger_ioctl_types.h"    /* Module specific types */
#include "tsmerger_ioctl.h"          /* Defines ioctl numbers */

#include "stpti.h"            /* Defines architecture information */

#include "tsmerger.h"	 /* A STAPI ioctl driver. */

/*** PROTOTYPES **************************************************************/


#include "tsmerger_ioctl_fops.h"


/*** METHODS *****************************************************************/

/*=============================================================================

   tsmerger_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'tsmerger_ioctl.h'.

  ===========================================================================*/
int tsmerger_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    /*** Check the user privalage ***/

    /*** Check the command is for this module ***/

    if (_IOC_TYPE(cmd) != TSMERGER_DRIVER_ID) {

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

    #if defined( STPTI_FRONTEND_TSMERGER )

    case TSMERGER_IOC_SETSWTS0BYPASSMODE:
    {
        TSMERGER_SetSWTS0BypassMode_t UserParams;

        UserParams.ErrorCode = TSMERGER_SetSWTS0BypassMode();

        if((err = copy_to_user((TSMERGER_SetSWTS0BypassMode_t*)arg, &UserParams, sizeof(TSMERGER_SetSWTS0BypassMode_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_SETTSIN0BYPASSMODE:
    {
        TSMERGER_SetTSIn0BypassMode_t UserParams;

        UserParams.ErrorCode = TSMERGER_SetTSIn0BypassMode();

        if((err = copy_to_user((TSMERGER_SetTSIn0BypassMode_t*)arg, &UserParams, sizeof(TSMERGER_SetTSIn0BypassMode_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_SETINPUT:
    {
        TSMERGER_SetInput_t UserParams;

        if ((err = copy_from_user(&UserParams, (TSMERGER_SetInput_t *)arg, sizeof(TSMERGER_SetInput_t)) )){
            /* Invalid user space address */
            goto fail;
        }

        UserParams.ErrorCode = TSMERGER_SetInput(UserParams.Source,
                                                 UserParams.Destination,
                                                 UserParams.SyncLock,
                                                 UserParams.SyncDrop,
                                                 UserParams.SyncPeriod,
                                                 UserParams.SerialNotParallel,
                                                 UserParams.SyncNotASync,
                                                 UserParams.Tagging);

        if((err = copy_to_user((TSMERGER_SetInput_t*)arg, &UserParams, sizeof(TSMERGER_SetInput_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_CLEARINPUT:
    {
        TSMERGER_ClearInput_t UserParams;

        if ((err = copy_from_user(&UserParams, (TSMERGER_ClearInput_t *)arg, sizeof(TSMERGER_ClearInput_t)) )){
            /* Invalid user space address */
            goto fail;
        }

        UserParams.ErrorCode = TSMERGER_ClearInput(UserParams.Source,
                                         UserParams.Destination);

        if((err = copy_to_user((TSMERGER_ClearInput_t*)arg, &UserParams, sizeof(TSMERGER_ClearInput_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_CLEARALL:
    {
        TSMERGER_ClearAll_t UserParams;

        UserParams.ErrorCode = TSMERGER_ClearAll();

        if((err = copy_to_user((TSMERGER_ClearAll_t*)arg, &UserParams, sizeof(TSMERGER_ClearAll_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_SETPRIORITY:
    {
        TSMERGER_SetPriority_t UserParams;

        if ((err = copy_from_user(&UserParams, (TSMERGER_SetPriority_t *)arg, sizeof(TSMERGER_SetPriority_t)) )){
            /* Invalid user space address */
            goto fail;
        }

        UserParams.ErrorCode = TSMERGER_SetPriority(UserParams.Source,
                                         UserParams.Priority);

        if((err = copy_to_user((TSMERGER_SetPriority_t*)arg, &UserParams, sizeof(TSMERGER_SetPriority_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_SETSYNC:
    {
        TSMERGER_SetSync_t UserParams;

        if ((err = copy_from_user(&UserParams, (TSMERGER_SetSync_t *)arg, sizeof(TSMERGER_SetSync_t)) )){
            /* Invalid user space address */
            goto fail;
        }

        UserParams.ErrorCode = TSMERGER_SetSync(UserParams.Source,
                                                UserParams.SyncPeriod,
                                                UserParams.SyncLock,
                                                UserParams.SyncDrop);

        if((err = copy_to_user((TSMERGER_SetSync_t*)arg, &UserParams, sizeof(TSMERGER_SetSync_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_SWTSINJECTWORD:
    {
        TSMERGER_SWTSInjectWordArray_t UserParams;
        U32 *Data_p;

        if ((err = copy_from_user(&UserParams, (TSMERGER_SWTSInjectWordArray_t *)arg, sizeof(TSMERGER_SWTSInjectWordArray_t)) )){
            /* Invalid user space address */
            goto fail;
        }

        Data_p = (U32 *)kmalloc(sizeof(U32)*UserParams.size, GFP_KERNEL);
        if ((err = copy_from_user(Data_p, UserParams.Data_p, sizeof(U32)*UserParams.size) )){
            /* Invalid user space address */
            goto fail;
        }

        UserParams.ErrorCode = TSMERGER_SWTSInjectWordArray(Data_p, UserParams.size, UserParams.ts, &UserParams.words_output);

        kfree( Data_p );

        if((err = copy_to_user((TSMERGER_SWTSInjectWordArray_t*)arg, &UserParams, sizeof(TSMERGER_SWTSInjectWordArray_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;
    case TSMERGER_IOC_IIFSETSYNCPERIOD:
    {
        TSMERGER_IIFSetSyncPeriod_t UserParams;

        if ((err = copy_from_user(&UserParams, (U32 *)arg, sizeof(U32)) )){
            /* Invalid user space address */
            goto fail;
        }

        UserParams.ErrorCode = TSMERGER_IIFSetSyncPeriod(UserParams.OutputInjectionSize);

        if((err = copy_to_user((TSMERGER_IIFSetSyncPeriod_t*)arg, &UserParams, sizeof(TSMERGER_IIFSetSyncPeriod_t)))) {
            /* Invalid user space address */
            goto fail;
        }
    }
    break;

    #endif /* #if defined( STPTI_FRONTEND_TSMERGER ) ... #endif */

    default :
        /*** Inappropriate ioctl for the device ***/
        err = -ENOTTY;
        goto fail;
    }


    return (0);

    /**************************************************************************/

    /*** Clean up on error ***/

fail:
    return (err);
}


