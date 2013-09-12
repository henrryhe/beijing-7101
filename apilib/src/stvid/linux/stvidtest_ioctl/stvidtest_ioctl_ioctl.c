/*****************************************************************************
 *
 *  Module      : stvidtest_ioctl
 *  Date        : 09-01-2006
 *  Author      : C. DAILLY
 *  Description :
 *  Copyright   : STMicroelectronics (c)2006
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

#include <linux/sched.h>         /* User privileges (capabilities) */

#include "stvidtest_ioctl.h"
#include "stvidtest_ioctl_types.h"

#include "stvidtest.h"   /* A STAPI ioctl driver. */

#include "sttbx.h"

#include "vid_util.h"
#include "stpti.h"

#include "stvidtest_ioctl_fops.h"

/*** PROTOTYPES **************************************************************/

/*** METHODS *****************************************************************/

/****Types********************************************************************/

/****Functions****************************************************************/

/* *************************************************** */
/* GetWritePTIPtrFct                                      */
/* *************************************************** */

ST_ErrorCode_t GetWritePTIPtrFct(void * const Handle, void ** const Address_p)
{

    ST_ErrorCode_t Err;

    /* STPTI_BufferGetWritePointer returns a device address, no need to convert */
#if !defined(STPTI_UNAVAILABLE)
    Err = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle,Address_p);
#endif /* #ifdef STPTI_UNAVAILABLE */

    return(Err);

}

/* *************************************************** */
/* SetReadPTIPtrFct                                       */
/* *************************************************** */

void SetReadPTIPtrFct(void * const Handle, void * const Address_p)
{
    /* Registered handle = index in cd-fifo array */
    /* Just cast the handle */
#if !defined(STPTI_UNAVAILABLE)
    STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle, Address_p);
#endif /* #ifdef STPTI_UNAVAILABLE */
}


/* ================================================================= >
   Common functions to all parts of the module
   ================================================================= */

/* GetKernelInjectionFunctionPointers */

ST_ErrorCode_t STVIDTEST_GetKernelInjectionFunctionPointers(KernelInjectionType_t Type, void **GetWriteAdd, void **InformReadAdd)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Type:%d", Type));

    switch (Type)
    {
/* disabling pti if not used */
#ifndef STPTI_UNAVAILABLE
        case STVIDTEST_INJECT_PTI:
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Setting pointers for PTI injection..."));
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "GetWriteAdd K&:%lx\nInformReadAdd K&:%lx",
                                                        (unsigned long)(GetWritePTIPtrFct),
                                                        (unsigned long)(SetReadPTIPtrFct)));
            *GetWriteAdd = (void*) GetWritePTIPtrFct;
            *InformReadAdd = (void*) SetReadPTIPtrFct;
            break;
#endif

#ifndef HDD_UNAVAILABLE
        case STVIDTEST_INJECT_HDD:
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Setting pointers for HDD injection..."));
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "GetWriteAdd K&:%lx\nInformReadAdd K&:%lx",
                                                        (unsigned long)GetWriteAddress,
                                                        (unsigned long)InformReadAddress));
            *GetWriteAdd = (void*) GetWriteAddress;
            *InformReadAdd = (void*) InformReadAddress;
            break;
#endif

        case STVIDTEST_INJECT_MEMORY:
            /* Memory injection is no more managed with this function */
            /* since the injection is now directly in kernel ... */
            /* No break */
        default :
            /* Error case. This event can't be configured. */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Injection pointers error"));
            ErrCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    /* Return the configuration's result. */
    return(ErrCode);
}


/*=============================================================================

   stvidtest_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stvidtest_ioctl.h'.

  ===========================================================================*/
int stvidtest_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int                 ret = 0;
    void              * Ioctl_Param_p = NULL;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVIDTEST: ioctl command %d [0x%.8x]", _IOC_NR(cmd), cmd));

    /*** Check the access permittions ***/

    if ((_IOC_DIR(cmd) & _IOC_READ) && !(filp->f_mode & FMODE_READ))
    {

        /* No Read permittions */
        ret = -ENOTTY;
        goto fail;
    }


    if ((_IOC_DIR(cmd) & _IOC_WRITE) && !(filp->f_mode & FMODE_WRITE))
    {

        /* No Write permittions */
        ret = -ENOTTY;
        goto fail;
    }

    /*** Execute the command ***/
    switch (cmd)
    {
        case STVIDTEST_IOC_INIT:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_Init_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_Init_t *)arg, sizeof(VIDTEST_Ioctl_Init_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* Init of the driver */
                ((VIDTEST_Ioctl_Init_t *)Ioctl_Param_p)->ErrorCode = ST_NO_ERROR;

                if ((ret = copy_to_user((VIDTEST_Ioctl_Init_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_Init_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;


        case STVIDTEST_IOC_TERM:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_Term_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_Term_t *)arg, sizeof(VIDTEST_Ioctl_Term_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* Term of the driver */
                ((VIDTEST_Ioctl_Term_t *)Ioctl_Param_p)->ErrorCode = ST_NO_ERROR;

                if ((ret = copy_to_user((VIDTEST_Ioctl_Term_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_Term_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVIDTEST_IOC_GETCAPABILITY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_GetCapability_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_GetCapability_t *)arg, sizeof(VIDTEST_Ioctl_GetCapability_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* Fill the capability of the driver. Should list the functions implemented here */
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Not implemented yed..."));

                if ((ret = copy_to_user((VIDTEST_Ioctl_GetCapability_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_GetCapability_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVIDTEST_IOC_GETKERNELINJECTIONFUNCTIONPOINTERS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t *)arg, sizeof(VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* GetKernelInjectionFunctionPointers of the driver */
                ((VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t *)Ioctl_Param_p)->ErrorCode =
                            STVIDTEST_GetKernelInjectionFunctionPointers(((VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t *)Ioctl_Param_p)->Type,
                                            &( ((VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t *)Ioctl_Param_p)->GetWriteAdd ),
                                            &( ((VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t *)Ioctl_Param_p)->InformReadAdd ));

                if ((ret = copy_to_user((VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_GetKernelInjectionFunctionPointers_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVIDTEST_IOC_INJECTFROMSTREAMBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_InjectFromStreamBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_InjectFromStreamBuffer_t *)arg, sizeof(VIDTEST_Ioctl_InjectFromStreamBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                if (VID_MemInject(((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->Handle,
                                          ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->NbInjections,
                                          ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->FifoNb,
                                          ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->BufferNb,
                                          ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->Buffer_p,
                                          ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->NbBytes))
                {
                    ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    ((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)Ioctl_Param_p)->ErrorCode = ST_NO_ERROR;
                }

                if ((ret = copy_to_user((VIDTEST_Ioctl_InjectFromStreamBuffer_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_InjectFromStreamBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVIDTEST_IOC_KILLINJECTFROMSTREAMBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_KillInjectFromStreamBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_KillInjectFromStreamBuffer_t *)arg, sizeof(VIDTEST_Ioctl_KillInjectFromStreamBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                if (VID_KillTask(((VIDTEST_Ioctl_KillInjectFromStreamBuffer_t *)Ioctl_Param_p)->FifoNb,
                                 ((VIDTEST_Ioctl_KillInjectFromStreamBuffer_t *)Ioctl_Param_p)->WaitEnfOfOnGoingInjection))
                {
                    ((VIDTEST_Ioctl_KillInjectFromStreamBuffer_t *)Ioctl_Param_p)->ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    ((VIDTEST_Ioctl_KillInjectFromStreamBuffer_t *)Ioctl_Param_p)->ErrorCode = ST_NO_ERROR;
                }

                if ((ret = copy_to_user((VIDTEST_Ioctl_KillInjectFromStreamBuffer_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_KillInjectFromStreamBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVIDTEST_IOC_DECODEFROMSTREAMBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VIDTEST_Ioctl_DecodeFromStreamBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)arg, sizeof(VIDTEST_Ioctl_DecodeFromStreamBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                if (VIDDecodeFromMemory(((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->Handle,
                                          ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->FifoNb,
                                          ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->BuffNb,
                                          ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->NbLoops,
                                          ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->Buffer_p,
                                          ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->NbBytes))
                {
                    ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    ((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)Ioctl_Param_p)->ErrorCode = ST_NO_ERROR;
                }

                if ((ret = copy_to_user((VIDTEST_Ioctl_DecodeFromStreamBuffer_t *)arg, Ioctl_Param_p, sizeof(VIDTEST_Ioctl_DecodeFromStreamBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        default :
            /*** Inappropriate ioctl for the device ***/
            ret = -ENOTTY;
            goto fail;
            break;
    }

    if (Ioctl_Param_p != NULL)
    {
        memory_deallocate (NULL, Ioctl_Param_p);
    }
    return (0);

    /**************************************************************************/

    /*** Clean up on error ***/

fail:
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVIDTEST: ioctl command %d [0x%.8x] failed (0x%.8x)!!!", _IOC_NR(cmd), cmd, (U32)ret));
    if (Ioctl_Param_p != NULL)
    {
        memory_deallocate (NULL, Ioctl_Param_p);
    }
    return (ret);
}

/* End of stvidtest_ioctl_ioctl.c */
