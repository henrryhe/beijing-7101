/*****************************************************************************
 *
 *  Module      : stvout_ioctl
 *  Date        : 23-10-2005
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

#include "stdevice.h"

#include "stvout_ioctl.h"          /* Defines ioctl numbers */
#include "stvout_ioctl_types.h"    /* Module specific types */
#include "stvout.h"  /* A STAPI ioctl driver. */
#include "stvout_ioctl_fops.h"


/*** PROTOTYPES **************************************************************/


/*** METHODS *****************************************************************/

/*=============================================================================

   stvout_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stvout_ioctl.h'.

  ===========================================================================*/
int stvout_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;
#if defined(STVOUT_HDMI_DEBUG) /* for debug purpose */
    printk(KERN_ALERT "ioctl command [0X%08X]\n", cmd);
#endif

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
        case STVOUT_IOC_INIT:
            {
                STVOUT_Ioctl_Init_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Init_t *)arg, sizeof(STVOUT_Ioctl_Init_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVOUT_Init(UserParams.DeviceName, &( UserParams.InitParams ));

                if((err = copy_to_user((STVOUT_Ioctl_Init_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Init_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_TERM:
            {
                STVOUT_Ioctl_Term_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Term_t *)arg, sizeof(STVOUT_Ioctl_Term_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVOUT_Term(UserParams.DeviceName, &( UserParams.TermParams ));

                if((err = copy_to_user((STVOUT_Ioctl_Term_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Term_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_OPEN:
            {
                STVOUT_Ioctl_Open_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Open_t *)arg, sizeof(STVOUT_Ioctl_Open_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVOUT_Open(UserParams.DeviceName, &( UserParams.OpenParams ), &(UserParams.Handle));

                if((err = copy_to_user((STVOUT_Ioctl_Open_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Open_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_CLOSE:
            {
                STVOUT_Ioctl_Close_t UserParams;

                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Close_t *)arg, sizeof(STVOUT_Ioctl_Close_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorCode = STVOUT_Close(UserParams.Handle);

                if((err = copy_to_user((STVOUT_Ioctl_Close_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Close_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;


        case STVOUT_IOC_DISABLE:
            {
                STVOUT_Ioctl_Disable_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Disable_t *)arg, sizeof(STVOUT_Ioctl_Disable_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_Disable( UserParams.Handle );
                if((err = copy_to_user((STVOUT_Ioctl_Disable_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Disable_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }

            }
            break;
        case STVOUT_IOC_ENABLE:
            {
                STVOUT_Ioctl_Enable_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Enable_t *)arg, sizeof(STVOUT_Ioctl_Enable_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_Enable( UserParams.Handle );
                if((err = copy_to_user((STVOUT_Ioctl_Enable_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Enable_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }

            }
            break;

        case STVOUT_IOC_GETCAPABILITY:
            {
                STVOUT_Ioctl_GetCapability_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetCapability_t *)arg, sizeof(STVOUT_Ioctl_GetCapability_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetCapability(UserParams.DeviceName , &(UserParams.Capability));
                if((err = copy_to_user((STVOUT_Ioctl_GetCapability_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetCapability_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_SETOUTPUTPARAMS:
            {
                STVOUT_Ioctl_SetOutputParams_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SetOutputParams_t *)arg, sizeof(STVOUT_Ioctl_SetOutputParams_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SetOutputParams( UserParams.Handle , &(UserParams.OutputParams)  );
                if((err = copy_to_user((STVOUT_Ioctl_SetOutputParams_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SetOutputParams_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_GETOUTPUTPARAMS:
            {
                STVOUT_Ioctl_GetOutputParams_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetOutputParams_t *)arg, sizeof(STVOUT_Ioctl_GetOutputParams_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetOutputParams( UserParams.Handle , &(UserParams.OutputParams)  );
                if((err = copy_to_user((STVOUT_Ioctl_GetOutputParams_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetOutputParams_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_SETOPTION:
            {
                STVOUT_Ioctl_SetOption_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SetOption_t *)arg, sizeof(STVOUT_Ioctl_SetOption_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SetOption( UserParams.Handle , &(UserParams.OptionParams)  );
                if((err = copy_to_user((STVOUT_Ioctl_SetOption_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SetOption_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_GETOPTION:
            {
                STVOUT_Ioctl_GetOption_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetOption_t *)arg, sizeof(STVOUT_Ioctl_GetOption_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetOption( UserParams.Handle , &(UserParams.OptionParams)  );
                if((err = copy_to_user((STVOUT_Ioctl_GetOption_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetOption_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_SETINPUTSOURCE:
            {
                STVOUT_Ioctl_SetInputSource_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SetInputSource_t *)arg, sizeof(STVOUT_Ioctl_SetInputSource_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SetInputSource( UserParams.Handle , (UserParams.Source)  );
                if((err = copy_to_user((STVOUT_Ioctl_SetInputSource_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SetInputSource_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_GETDACSELECT:
            {
                STVOUT_Ioctl_GetDacSelect_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetDacSelect_t *)arg, sizeof(STVOUT_Ioctl_GetDacSelect_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetDacSelect( UserParams.Handle , &(UserParams.DacSelect)  );
                if((err = copy_to_user((STVOUT_Ioctl_GetDacSelect_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetDacSelect_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_GETSTATE:
            {
                STVOUT_Ioctl_GetState_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetState_t *)arg, sizeof(STVOUT_Ioctl_GetState_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetState( UserParams.Handle , &(UserParams.State) );
                if((err = copy_to_user((STVOUT_Ioctl_GetState_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetState_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_GETTARGETINFORMATION:
            {
                STVOUT_Ioctl_GetTargetInformation_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetTargetInformation_t *)arg, sizeof(STVOUT_Ioctl_GetTargetInformation_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetTargetInformation( UserParams.Handle , &(UserParams.TargetInformation) );
                if((err = copy_to_user((STVOUT_Ioctl_GetTargetInformation_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetTargetInformation_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_SENDDATA:
            {
                STVOUT_Ioctl_SendData_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SendData_t *)arg, sizeof(STVOUT_Ioctl_SendData_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SendData( UserParams.Handle , UserParams.InfoFrameType, &(UserParams.Buffer), UserParams.Size );

                if((err = copy_to_user((STVOUT_Ioctl_SendData_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SendData_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
#if defined (STVOUT_HDCP_PROTECTION)

        case STVOUT_IOC_SETHDCPPARAMS:
            {
                STVOUT_Ioctl_SetHDCPParams_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SetHDCPParams_t *)arg, sizeof(STVOUT_Ioctl_SetHDCPParams_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SetHDCPParams( UserParams.Handle , &(UserParams.HDCPParams));

                if((err = copy_to_user((STVOUT_Ioctl_SetHDCPParams_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SetHDCPParams_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_GETHDCPSINKPARAMS:
            {
                STVOUT_Ioctl_GetHDCPSinkParams_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetHDCPSinkParams_t *)arg, sizeof(STVOUT_Ioctl_GetHDCPSinkParams_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetHDCPSinkParams( UserParams.Handle , &(UserParams.HDCPSinkParams) );
                if((err = copy_to_user((STVOUT_Ioctl_GetHDCPSinkParams_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetHDCPSinkParams_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_UPDATEFORBIDDENKSVS:
            {
                STVOUT_Ioctl_UpdateForbiddenKSVs_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_UpdateForbiddenKSVs_t *)arg, sizeof(STVOUT_Ioctl_UpdateForbiddenKSVs_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_UpdateForbiddenKSVs( UserParams.Handle , &(UserParams.KSVList), UserParams.KSVNumber  );
                if((err = copy_to_user((STVOUT_Ioctl_UpdateForbiddenKSVs_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_UpdateForbiddenKSVs_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_ENABLEDEFAULTOUTPUT:
            {
                STVOUT_Ioctl_EnableDefaultOutput_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_EnableDefaultOutput_t *)arg, sizeof(STVOUT_Ioctl_EnableDefaultOutput_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_EnableDefaultOutput( UserParams.Handle , &(UserParams.DefaultOutput) );
                if((err = copy_to_user((STVOUT_Ioctl_EnableDefaultOutput_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_EnableDefaultOutput_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_DISABLEDEFAULTOUTPUT:
            {
                STVOUT_Ioctl_DisableDefaultOutput_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_DisableDefaultOutput_t *)arg, sizeof(STVOUT_Ioctl_DisableDefaultOutput_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_DisableDefaultOutput( UserParams.Handle );
                if((err = copy_to_user((STVOUT_Ioctl_DisableDefaultOutput_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_DisableDefaultOutput_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

#endif  /* STVOUT_HDCP_PROTECTION */

        case STVOUT_IOC_START:
            {
                STVOUT_Ioctl_Start_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_Start_t *)arg, sizeof(STVOUT_Ioctl_Start_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_Start( UserParams.Handle );

                if((err = copy_to_user((STVOUT_Ioctl_Start_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_Start_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_GETSTATISTICS:
            {
                STVOUT_Ioctl_GetStatistics_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_GetStatistics_t *)arg, sizeof(STVOUT_Ioctl_GetStatistics_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_GetStatistics( UserParams.Handle , &(UserParams.Statistics) );

                if((err = copy_to_user((STVOUT_Ioctl_GetStatistics_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_GetStatistics_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_SETHDMIOUTPUTTYPE:
            {
                STVOUT_Ioctl_SetHDMIOutputType_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SetHDMIOutputType_t *)arg, sizeof(STVOUT_Ioctl_SetHDMIOutputType_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SetHDMIOutputType( UserParams.Handle , UserParams.OutputType );

                if((err = copy_to_user((STVOUT_Ioctl_SetHDMIOutputType_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SetHDMIOutputType_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_ADJUSTCHROMALUMADELAY:
            {
                STVOUT_Ioctl_AdjustChromaLumaDelay_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_AdjustChromaLumaDelay_t *)arg, sizeof(STVOUT_Ioctl_AdjustChromaLumaDelay_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_AdjustChromaLumaDelay( UserParams.Handle , (UserParams.CLDelay)  );
                if((err = copy_to_user((STVOUT_Ioctl_AdjustChromaLumaDelay_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_AdjustChromaLumaDelay_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

#ifdef STVOUT_CEC_PIO_COMPARE
        case STVOUT_IOC_SENDCECMESSAGE:
            {
                STVOUT_Ioctl_SendCECMessage_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_SendCECMessage_t *)arg, sizeof(STVOUT_Ioctl_SendCECMessage_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_SendCECMessage( UserParams.Handle , &(UserParams.Message) );
                if((err = copy_to_user((STVOUT_Ioctl_SendCECMessage_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_SendCECMessage_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STVOUT_IOC_CECPHYSICALADDRAVAILABLE:
            {
                STVOUT_Ioctl_CECPhysicalAddressAvailable_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_CECPhysicalAddressAvailable_t *)arg, sizeof(STVOUT_Ioctl_CECPhysicalAddressAvailable_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_CECPhysicalAddressAvailable( UserParams.Handle );
                if((err = copy_to_user((STVOUT_Ioctl_CECPhysicalAddressAvailable_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_CECPhysicalAddressAvailable_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STVOUT_IOC_CECSETADDITIONALADDRESS:
            {
                STVOUT_Ioctl_CECSetAdditionalAddress_t UserParams;
                if ((err = copy_from_user(&UserParams, (STVOUT_Ioctl_CECSetAdditionalAddress_t *)arg, sizeof(STVOUT_Ioctl_CECSetAdditionalAddress_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;
                }
                UserParams.ErrorCode = STVOUT_CECSetAdditionalAddress( UserParams.Handle, UserParams.Role );
                if((err = copy_to_user((STVOUT_Ioctl_CECSetAdditionalAddress_t*)arg, &UserParams, sizeof(STVOUT_Ioctl_CECSetAdditionalAddress_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

#endif

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

