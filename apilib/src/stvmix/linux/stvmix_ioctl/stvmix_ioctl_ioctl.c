/*****************************************************************************
 *
 *  Module      : stvmix_ioctl
 *  Date        : 30-10-2005
 *  Author      : AYARITAR
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

#include "stvmix_ioctl_types.h"    /* Module specific types */

#include "stvmix_ioctl.h"          /* Defines ioctl numbers */

#include "stvmix.h"	 /* A STAPI ioctl driver. */

#include "stos.h"

/*** PROTOTYPES **************************************************************/


#include "stvmix_ioctl_fops.h"


/*** METHODS *****************************************************************/

/*=============================================================================

   stvmix_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stvmix_ioctl.h'.

  ===========================================================================*/
int stvmix_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    STTBX_Print((KERN_ALERT "ioctl command [0X%08X]\n", cmd));

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

        case STVMIX_IOC_INIT:
            {
                STVMIX_Ioctl_Init_t UserParams;
                int                 i = 0;
                ST_DeviceName_t VoutName[MAX_VOUT];

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_Init_t *)arg, sizeof(STVMIX_Ioctl_Init_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                if ((err = copy_from_user(VoutName, ((STVMIX_Ioctl_Init_t *)arg)->VoutName, sizeof(ST_DeviceName_t) * MAX_VOUT))){
                    /* Invalid user space address */
                    goto fail;
                }

                if ( UserParams.InitParams.OutputArray_p!=NULL ) {
                    UserParams.InitParams.OutputArray_p = VoutName;
                }

                UserParams.ErrorCode = STVMIX_Init(UserParams.DeviceName, &( UserParams.InitParams ));

                if((err = copy_to_user((STVMIX_Ioctl_Init_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_Init_t)))){
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVMIX_IOC_TERM:
            {
                STVMIX_Ioctl_Term_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_Term_t *)arg, sizeof(STVMIX_Ioctl_Term_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_Term(UserParams.DeviceName, &( UserParams.TermParams ));

                if((err = copy_to_user((STVMIX_Ioctl_Term_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_Term_t)))){
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVMIX_IOC_OPEN:
            {
                STVMIX_Ioctl_Open_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_Open_t *)arg, sizeof(STVMIX_Ioctl_Open_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_Open(UserParams.DeviceName, &( UserParams.OpenParams ), &(UserParams.Handle));

                if((err = copy_to_user((STVMIX_Ioctl_Open_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_Open_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVMIX_IOC_CLOSE:
            {
                STVMIX_Ioctl_Close_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_Close_t *)arg, sizeof(STVMIX_Ioctl_Close_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_Close(UserParams.Handle);

                if((err = copy_to_user((STVMIX_Ioctl_Close_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_Close_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVMIX_IOC_DISABLE:
            {
                STVMIX_Ioctl_Disable_t UserParams;
                STTBX_Print(("STVMIX_IOC_ENABLE \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_Disable_t *)arg, sizeof(STVMIX_Ioctl_Disable_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_Disable(UserParams.Handle);

                if((err = copy_to_user((STVMIX_Ioctl_Disable_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_Disable_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;
        case STVMIX_IOC_ENABLE:

            {
                STVMIX_Ioctl_Enable_t UserParams;

                STTBX_Print(("STVMIX_IOC_ENABLE \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_Enable_t *)arg, sizeof(STVMIX_Ioctl_Enable_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_Enable(UserParams.Handle);

                if((err = copy_to_user((STVMIX_Ioctl_Enable_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_Enable_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_GETCAPABILITY:

            {
                STVMIX_Ioctl_GetCapability_t UserParams;
                STTBX_Print(("STVMIX_IOC_GETCAPABILITY:\n"));
                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_GetCapability_t *)arg, sizeof(STVMIX_Ioctl_GetCapability_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_GetCapability(UserParams.DeviceName, &(UserParams.Capability));

                if((err = copy_to_user((STVMIX_Ioctl_GetCapability_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_GetCapability_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_DISCONNECTLAYERS:

           {
                STVMIX_Ioctl_DisconnectLayers_t UserParams;
                STTBX_Print(("STVMIX_IOC_DISCONNECTLAYERS \n"));
                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_DisconnectLayers_t *)arg, sizeof(STVMIX_Ioctl_DisconnectLayers_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_DisconnectLayers(UserParams.Handle);

                if((err = copy_to_user((STVMIX_Ioctl_DisconnectLayers_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_DisconnectLayers_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }

            break;

        case STVMIX_IOC_SETBACKGROUNDCOLOR:


            {
                STVMIX_Ioctl_SetBackgroundColor_t UserParams;
                STTBX_Print(("STVMIX_IOC_SETBACKGROUNDCOLOR \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_SetBackgroundColor_t *)arg, sizeof(STVMIX_Ioctl_SetBackgroundColor_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_SetBackgroundColor(UserParams.Handle, UserParams.RGB888 ,UserParams.Enable );

                if((err = copy_to_user((STVMIX_Ioctl_SetBackgroundColor_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_SetBackgroundColor_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;
        case STVMIX_IOC_SETSCREENOFFSET:
            {
                STVMIX_Ioctl_SetScreenOffset_t UserParams;
                STTBX_Print(("STVMIX_IOC_SETSCREENOFFSET \n"));
                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_SetScreenOffset_t *)arg, sizeof(STVMIX_Ioctl_SetScreenOffset_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_SetScreenOffset(UserParams.Handle,  UserParams.Horizontal , UserParams.Vertical);

                if((err = copy_to_user((STVMIX_Ioctl_SetScreenOffset_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_SetScreenOffset_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_SETSCREENPARAMS:

            {
                STVMIX_Ioctl_SetScreenParams_t UserParams;
                STTBX_Print(("STVMIX_IOC_SETSCREENPARAMS \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_SetScreenParams_t *)arg, sizeof(STVMIX_Ioctl_SetScreenParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_SetScreenParams(UserParams.Handle, &( UserParams.ScreenParams ));

                if((err = copy_to_user((STVMIX_Ioctl_SetScreenParams_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_SetScreenParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


            break;
        case STVMIX_IOC_SETTIMEBASE:
             {
                STVMIX_Ioctl_SetTimeBase_t UserParams;
                STTBX_Print(("STVMIX_IOC_SETTIMEBASE \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_SetTimeBase_t *)arg, sizeof(STVMIX_Ioctl_SetTimeBase_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_SetTimeBase(UserParams.Handle, UserParams.VTGDriver );

                if((err = copy_to_user((STVMIX_Ioctl_SetTimeBase_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_SetTimeBase_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_GETTIMEBASE:


            {
                STVMIX_Ioctl_GetTimeBase_t UserParams;
                STTBX_Print(("STVMIX_IOC_GETTIMEBASE \n"));
                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_GetTimeBase_t *)arg, sizeof(STVMIX_Ioctl_GetTimeBase_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_GetTimeBase(UserParams.Handle, &( UserParams.VTGDriver ));

                if((err = copy_to_user((STVMIX_Ioctl_GetTimeBase_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_GetTimeBase_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }


            break;

        case STVMIX_IOC_GETSCREENPARAMS:

            {
                STVMIX_Ioctl_GetScreenParams_t UserParams;
                STTBX_Print(("STVMIX_IOC_GETSCREENPARAMS \n"));
                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_GetScreenParams_t *)arg, sizeof(STVMIX_Ioctl_GetScreenParams_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_GetScreenParams(UserParams.Handle, &( UserParams.ScreenParams ));

                if((err = copy_to_user((STVMIX_Ioctl_GetScreenParams_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_GetScreenParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;
        case STVMIX_IOC_GETSCREENOFFSET:

            {
                STVMIX_Ioctl_GetScreenOffset_t UserParams;
                STTBX_Print(("STVMIX_IOC_GETSCREENOFFSET \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_GetScreenOffset_t *)arg, sizeof(STVMIX_Ioctl_GetScreenOffset_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_GetScreenOffset(UserParams.Handle, &( UserParams.Horizontal ), &( UserParams.Vertical ));

                if((err = copy_to_user((STVMIX_Ioctl_GetScreenOffset_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_GetScreenOffset_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_GETCONNECTEDLAYERS:

            {
                STVMIX_Ioctl_GetConnectedLayers_t UserParams;
                STTBX_Print(("STVMIX_IOC_GETCONNECTEDLAYERS \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_GetConnectedLayers_t *)arg, sizeof(STVMIX_Ioctl_GetConnectedLayers_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_GetConnectedLayers(UserParams.Handle,  UserParams.LayerPosition , &(UserParams.LayerParams) );

                if((err = copy_to_user((STVMIX_Ioctl_GetConnectedLayers_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_GetConnectedLayers_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_GETBACKGROUNDCOLOR:

            {
                STVMIX_Ioctl_GetBackgroundColor_t UserParams;
                STTBX_Print(("STVMIX_IOC_GETBACKGROUNDCOLOR \n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_GetBackgroundColor_t *)arg, sizeof(STVMIX_Ioctl_GetBackgroundColor_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVMIX_GetBackgroundColor(UserParams.Handle, &( UserParams.RGB888 ), &( UserParams.Enable ));

                if((err = copy_to_user((STVMIX_Ioctl_GetBackgroundColor_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_GetBackgroundColor_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }

            break;

        case STVMIX_IOC_CONNECTLAYERS:

            {
                STVMIX_Ioctl_ConnectLayers_t UserParams;
                int                             i;
                STVMIX_LayerDisplayParams_t     *LayerArray[MAX_LAYER];

                STTBX_Print(("STVMIX_IOC_CONNECTLAYERS\n"));

                if ((err = copy_from_user(&UserParams, (STVMIX_Ioctl_ConnectLayers_t *)arg, sizeof(STVMIX_Ioctl_ConnectLayers_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }
                for ( i=0 ; i < (UserParams.NbLayerParams) ; i++ )
                {
                    LayerArray[i] = &(UserParams.LayerDisplayParams[i]);
                }

                UserParams.ErrorCode = STVMIX_ConnectLayers(UserParams.Handle, (const STVMIX_LayerDisplayParams_t**)LayerArray , UserParams.NbLayerParams );

                if((err = copy_to_user((STVMIX_Ioctl_ConnectLayers_t*)arg, &UserParams, sizeof(STVMIX_Ioctl_ConnectLayers_t)))) {
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


