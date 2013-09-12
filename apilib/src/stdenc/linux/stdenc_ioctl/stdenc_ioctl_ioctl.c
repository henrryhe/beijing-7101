/*****************************************************************************
 *
 *  Module      : stdenc_ioctl
 *  Date        : 21-10-2005
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

#include "stdenc_ioctl_types.h"    /* Module specific types */

#include "stdenc_ioctl.h"          /* Defines ioctl numbers */


#include "stdenc.h"	 /* A STAPI ioctl driver. */

#include "stos.h"

/*** PROTOTYPES **************************************************************/


#include "stdenc_ioctl_fops.h"


/*** METHODS *****************************************************************/

/*=============================================================================

   stdenc_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stdenc_ioctl.h'.

  ===========================================================================*/
int stdenc_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;


    STTBX_Print((KERN_ALERT "ioctl command [0X%08X]\n", cmd));

    /*** Check the user privalage ***/

    /* if (!capable (CAP_SYS_ADMIN)) */ /* See 'sys/sched.h' */
    /* {                             */
    /*     return (-EPERM);          */
    /* }                             */

    /*** Check the command is for this module ***/





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

      switch (cmd)
            {

        case STDENC_IOC_INIT:
          {
                STDENC_Ioctl_Init_t UserParams;
               STTBX_Print(("STDENC_IOC_INIT \n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_Init_t *)arg, sizeof(STDENC_Ioctl_Init_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_Init(UserParams.DeviceName, &( UserParams.InitParams ));

                if((err = copy_to_user((STDENC_Ioctl_Init_t*)arg, &UserParams, sizeof(STDENC_Ioctl_Init_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

           }

            break;

        case STDENC_IOC_TERM:

            {

                STDENC_Ioctl_Term_t UserParams;

                STTBX_Print(("STDENC_IOC_TERM\n"));
                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_Term_t *)arg, sizeof(STDENC_Ioctl_Term_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_Term(UserParams.DeviceName, &( UserParams.TermParams ));

                if((err = copy_to_user((STDENC_Ioctl_Term_t*)arg, &UserParams, sizeof(STDENC_Ioctl_Term_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

             }
            break;
        case STDENC_IOC_OPEN:
            {
                STDENC_Ioctl_Open_t UserParams;
                 STTBX_Print(("STDENC_IOC_OPEN \n"));
                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_Open_t *)arg, sizeof(STDENC_Ioctl_Open_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_Open(UserParams.DeviceName, &( UserParams.OpenParams ), &(UserParams.Handle));

                if((err = copy_to_user((STDENC_Ioctl_Open_t*)arg, &UserParams, sizeof(STDENC_Ioctl_Open_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

             }
            break;
        case STDENC_IOC_CLOSE:
            {
                STDENC_Ioctl_Close_t UserParams;
                 STTBX_Print(("STDENC_IOC_CLOSE \n"));
                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_Close_t *)arg, sizeof(STDENC_Ioctl_Close_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_Close(UserParams.Handle);

                if((err = copy_to_user((STDENC_Ioctl_Close_t*)arg, &UserParams, sizeof(STDENC_Ioctl_Close_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

             }
            break;

        case STDENC_IOC_SETENCODINGMODE:
         {
           STDENC_Ioctl_SetEncodingMode_t UserParams;

           STTBX_Print(("STDENC_IOC_SETENCODINGMODE\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_SetEncodingMode_t *)arg, sizeof(STDENC_Ioctl_SetEncodingMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_SetEncodingMode(UserParams.Handle, &UserParams.EncodingMode );

                if((err = copy_to_user((STDENC_Ioctl_SetEncodingMode_t*)arg, &UserParams, sizeof(STDENC_Ioctl_SetEncodingMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
           break;
        case STDENC_IOC_GETENCODINGMODE:
           {
                STDENC_Ioctl_GetEncodingMode_t UserParams;

               STTBX_Print(("STDENC_IOC_GETENCODINGMODE\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_GetEncodingMode_t *)arg, sizeof(STDENC_Ioctl_GetEncodingMode_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_GetEncodingMode(UserParams.Handle, &UserParams.EncodingMode );

                if((err = copy_to_user((STDENC_Ioctl_GetEncodingMode_t*)arg, &UserParams, sizeof(STDENC_Ioctl_GetEncodingMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STDENC_IOC_GETCAPABILITY:
           {
              STDENC_Ioctl_GetCapability_t UserParams;

              STTBX_Print(("STDENC_IOC_GETCAPABILITY:\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_GetCapability_t *)arg, sizeof(STDENC_Ioctl_GetCapability_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_GetCapability(UserParams.DeviceName, &UserParams.Capability );

                if((err = copy_to_user((STDENC_Ioctl_GetCapability_t*)arg, &UserParams, sizeof(STDENC_Ioctl_GetCapability_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STDENC_IOC_SETMODEOPTION:
          {
            STDENC_Ioctl_SetModeOption_t UserParams;

            STTBX_Print(("STDENC_IOC_SETMODEOPTION\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_SetModeOption_t *)arg, sizeof(STDENC_Ioctl_SetModeOption_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_SetModeOption(UserParams.Handle, &UserParams.ModeOption );

                if((err = copy_to_user((STDENC_Ioctl_SetModeOption_t*)arg, &UserParams, sizeof(STDENC_Ioctl_SetModeOption_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }
            break;


        case STDENC_IOC_GETMODEOPTION:
          {
            STDENC_Ioctl_GetModeOption_t UserParams;

            STTBX_Print(("STDENC_IOC_GETMODEOPTION\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_GetModeOption_t *)arg, sizeof(STDENC_Ioctl_GetModeOption_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_GetModeOption(UserParams.Handle, &UserParams.ModeOption );

                if((err = copy_to_user((STDENC_Ioctl_GetModeOption_t*)arg, &UserParams, sizeof(STDENC_Ioctl_GetModeOption_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }
            break;

        case STDENC_IOC_SETCOLORBARPATTERN:
          {
            STDENC_Ioctl_SetColorBarPattern_t UserParams;

            STTBX_Print(("STDENC_IOC_SETCOLORBARPATTERN\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_SetColorBarPattern_t *)arg, sizeof(STDENC_Ioctl_SetColorBarPattern_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_SetColorBarPattern(UserParams.Handle, UserParams.ColorBarPattern );

                if((err = copy_to_user((STDENC_Ioctl_SetColorBarPattern_t*)arg, &UserParams, sizeof(STDENC_Ioctl_SetColorBarPattern_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }
            break;

        case STDENC_IOC_READREG8:
          {
            STDENC_Ioctl_ReadReg8_t UserParams;

            STTBX_Print(("STDENC_IOC_READREG8\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_ReadReg8_t *)arg, sizeof(STDENC_Ioctl_ReadReg8_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_ReadReg8(UserParams.Handle,UserParams.Addr,&UserParams.Value );

                if((err = copy_to_user((STDENC_Ioctl_ReadReg8_t*)arg, &UserParams, sizeof(STDENC_Ioctl_ReadReg8_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }
            break;

        case STDENC_IOC_WRITEREG8:
          {
            STDENC_Ioctl_WriteReg8_t UserParams;

            STTBX_Print(("STDENC_IOC_WRITEREG8\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_WriteReg8_t *)arg, sizeof(STDENC_Ioctl_WriteReg8_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_WriteReg8(UserParams.Handle,UserParams.Addr,UserParams.Value );

                if((err = copy_to_user((STDENC_Ioctl_WriteReg8_t*)arg, &UserParams, sizeof(STDENC_Ioctl_WriteReg8_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }
            break;

        case STDENC_IOC_ANDREG8:
         {
            STDENC_Ioctl_AndReg8_t UserParams;

            STTBX_Print(("STDENC_IOC_ANDREG8 \n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_ReadReg8_t *)arg, sizeof(STDENC_Ioctl_ReadReg8_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_AndReg8(UserParams.Handle,UserParams.Addr,UserParams.AndMask );

                if((err = copy_to_user((STDENC_Ioctl_ReadReg8_t*)arg, &UserParams, sizeof(STDENC_Ioctl_ReadReg8_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
           }
            break;

        case STDENC_IOC_ORREG8:
          {
            STDENC_Ioctl_OrReg8_t UserParams;

            STTBX_Print(("STDENC_IOC_ORREG8 \n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_OrReg8_t *)arg, sizeof(STDENC_Ioctl_OrReg8_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_OrReg8(UserParams.Handle,UserParams.Addr,UserParams.OrMask );

                if((err = copy_to_user((STDENC_Ioctl_OrReg8_t*)arg, &UserParams, sizeof(STDENC_Ioctl_OrReg8_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

            }
            break;

        case STDENC_IOC_MASKREG8:
          {
            STDENC_Ioctl_MaskReg8_t UserParams;

            STTBX_Print(("STDENC_IOC_MASKREG8 \n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_MaskReg8_t *)arg, sizeof(STDENC_Ioctl_MaskReg8_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_MaskReg8(UserParams.Handle,UserParams.Addr,UserParams.AndMask,UserParams.OrMask );

                if((err = copy_to_user((STDENC_Ioctl_MaskReg8_t*)arg, &UserParams, sizeof(STDENC_Ioctl_MaskReg8_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

           }
            break;

        case STDENC_IOC_CHECKHANDLE:
          {
            STDENC_Ioctl_CheckHandle_t UserParams;

            STTBX_Print(("STDENC_IOC_CHECKHANDLE\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_CheckHandle_t *)arg, sizeof(STDENC_Ioctl_CheckHandle_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STDENC_CheckHandle(UserParams.Handle);

                if((err = copy_to_user((STDENC_Ioctl_CheckHandle_t*)arg, &UserParams, sizeof(STDENC_Ioctl_CheckHandle_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
          }
            break;

        case STDENC_IOC_REGACCESSLOCK:
          {
            STDENC_Ioctl_RegAccessLock_t UserParams;

            STTBX_Print(("STDENC_IOC_REGACCESSLOCK\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_RegAccessLock_t *)arg, sizeof(STDENC_Ioctl_RegAccessLock_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                STDENC_RegAccessLock(UserParams.Handle);

                if((err = copy_to_user((STDENC_Ioctl_RegAccessLock_t*)arg, &UserParams, sizeof(STDENC_Ioctl_RegAccessLock_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
          }
            break;

        case STDENC_IOC_REGACCESSUNLOCK:
           {
             STDENC_Ioctl_RegAccessUnlock_t UserParams;

             STTBX_Print(("STDENC_IOC_REGACCESSUNLOCK\n"));

                if ((err = copy_from_user(&UserParams, (STDENC_Ioctl_RegAccessUnlock_t *)arg, sizeof(STDENC_Ioctl_RegAccessUnlock_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }
                STDENC_RegAccessUnlock(UserParams.Handle);

                if((err = copy_to_user((STDENC_Ioctl_RegAccessUnlock_t*)arg, &UserParams, sizeof(STDENC_Ioctl_RegAccessUnlock_t)))) {
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


