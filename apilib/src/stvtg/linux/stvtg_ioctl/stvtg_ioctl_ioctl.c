/*****************************************************************************
 *
 *  Module      : stvtg_ioctl
 *  Date        : 15-10-2005
 *  Author      : ATROUSWA
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

#include "stvtg_ioctl.h"
#include "stvtg_ioctl_types.h"

#include "stvtg.h"   /* A STAPI ioctl driver. */
#include "stevt.h"

/*** PROTOTYPES **************************************************************/


#include "stvtg_ioctl_fops.h"



/*** METHODS *****************************************************************/

/****Types*******************************************************************/

/*=============================================================================

   stvtg_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stvtg_ioctl.h'.

  ===========================================================================*/
int stvtg_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;



    printk(KERN_ALERT "ioctl command [0X%08X]\n", cmd);


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
        case STVTG_IOC_INIT:
            {
                STVTG_Ioctl_Init_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_Init_t *)arg, sizeof(STVTG_Ioctl_Init_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_Init(UserParams.DeviceName, &( UserParams.InitParams ));

                if((err = copy_to_user((STVTG_Ioctl_Init_t*)arg, &UserParams, sizeof(STVTG_Ioctl_Init_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_TERM:
            {
                STVTG_Ioctl_Term_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_Term_t *)arg, sizeof(STVTG_Ioctl_Term_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_Term(UserParams.DeviceName, &( UserParams.TermParams ));

                if((err = copy_to_user((STVTG_Ioctl_Term_t*)arg, &UserParams, sizeof(STVTG_Ioctl_Term_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_OPEN:
            {
                STVTG_Ioctl_Open_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_Open_t *)arg, sizeof(STVTG_Ioctl_Open_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_Open(UserParams.DeviceName, &( UserParams.OpenParams ), &(UserParams.Handle));

                if((err = copy_to_user((STVTG_Ioctl_Open_t*)arg, &UserParams, sizeof(STVTG_Ioctl_Open_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_CLOSE:
            {
                STVTG_Ioctl_Close_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_Close_t *)arg, sizeof(STVTG_Ioctl_Close_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_Close(UserParams.Handle);

                if((err = copy_to_user((STVTG_Ioctl_Close_t*)arg, &UserParams, sizeof(STVTG_Ioctl_Close_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVTG_IOC_SETMODE:
            {
                STVTG_Ioctl_SetMode_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_SetMode_t *)arg, sizeof(STVTG_Ioctl_SetMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_SetMode(UserParams.Handle, UserParams.Mode );

                if((err = copy_to_user((STVTG_Ioctl_SetMode_t*)arg, &UserParams, sizeof(STVTG_Ioctl_SetMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVTG_IOC_SETOPTIONALCONFIGURATION:
            {
                STVTG_Ioctl_SetOptionalConfiguration_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_SetOptionalConfiguration_t *)arg, sizeof(STVTG_Ioctl_SetOptionalConfiguration_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_SetOptionalConfiguration(UserParams.Handle, &(UserParams.OptionalConfiguration));

                if((err = copy_to_user((STVTG_Ioctl_SetOptionalConfiguration_t*)arg, &UserParams, sizeof(STVTG_Ioctl_SetOptionalConfiguration_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_SETSLAVEMODEPARAMS:
            {
                STVTG_Ioctl_SetSlaveModeParams_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_SetSlaveModeParams_t *)arg, sizeof(STVTG_Ioctl_SetSlaveModeParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVTG_SetSlaveModeParams(UserParams.Handle, &(UserParams.SlaveModeParams));

                if((err = copy_to_user((STVTG_Ioctl_SetSlaveModeParams_t*)arg, &UserParams, sizeof(STVTG_Ioctl_SetSlaveModeParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_GETMODESYNCPARAMS:
			{
                STVTG_Ioctl_GetModeSyncParams_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetModeSyncParams_t *)arg, sizeof(STVTG_Ioctl_GetModeSyncParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_GetModeSyncParams(UserParams.Handle, &(UserParams.SyncParams));

                if((err = copy_to_user((STVTG_Ioctl_GetModeSyncParams_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetModeSyncParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
			break;
        case STVTG_IOC_GETMODE:
            {
                STVTG_Ioctl_GetMode_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetMode_t *)arg, sizeof(STVTG_Ioctl_GetMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_GetMode(UserParams.Handle, &(UserParams.TimingMode),&(UserParams.ModeParams));

                if((err = copy_to_user((STVTG_Ioctl_GetMode_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVTG_IOC_GETVTGID:
            {
                STVTG_Ioctl_GetVtgId_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetVtgId_t *)arg, sizeof(STVTG_Ioctl_GetVtgId_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_GetVtgId(UserParams.DeviceName, &(UserParams.VtgId));

                if((err = copy_to_user((STVTG_Ioctl_GetVtgId_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetVtgId_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVTG_IOC_GETCAPABILITY:
            {
                STVTG_Ioctl_GetCapability_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetCapability_t *)arg, sizeof(STVTG_Ioctl_GetCapability_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_GetCapability(UserParams.DeviceName, &(UserParams.Capability));

                if((err = copy_to_user((STVTG_Ioctl_GetCapability_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetCapability_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVTG_IOC_GETOPTIONALCONFIGURATION:
            {
                STVTG_Ioctl_GetOptionalConfiguration_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetOptionalConfiguration_t *)arg, sizeof(STVTG_Ioctl_GetOptionalConfiguration_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVTG_GetOptionalConfiguration(UserParams.Handle, &(UserParams.OptionalConfiguration));

                if((err = copy_to_user((STVTG_Ioctl_GetOptionalConfiguration_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetOptionalConfiguration_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVTG_IOC_GETSLAVEMODEPARAMS:
            {
                STVTG_Ioctl_GetSlaveModeParams_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetSlaveModeParams_t *)arg, sizeof(STVTG_Ioctl_GetSlaveModeParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVTG_GetSlaveModeParams(UserParams.Handle, &(UserParams.SlaveModeParams));

                if((err = copy_to_user((STVTG_Ioctl_GetSlaveModeParams_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetSlaveModeParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_CALIBRATESYNCLEVEL:
            {
                STVTG_Ioctl_CalibrateSyncLevel_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_CalibrateSyncLevel_t *)arg, sizeof(STVTG_Ioctl_CalibrateSyncLevel_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_CalibrateSyncLevel(UserParams.Handle, &(UserParams.SyncLevel));

                if((err = copy_to_user((STVTG_Ioctl_CalibrateSyncLevel_t*)arg, &UserParams, sizeof(STVTG_Ioctl_CalibrateSyncLevel_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVTG_IOC_GETLEVELSETTINGS:
            {
                STVTG_Ioctl_GetLevelSettings_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVTG_Ioctl_GetLevelSettings_t *)arg, sizeof(STVTG_Ioctl_GetLevelSettings_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVTG_GetLevelSettings(UserParams.Handle, &(UserParams.SyncLevel));

                if((err = copy_to_user((STVTG_Ioctl_GetLevelSettings_t*)arg, &UserParams, sizeof(STVTG_Ioctl_GetLevelSettings_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

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


