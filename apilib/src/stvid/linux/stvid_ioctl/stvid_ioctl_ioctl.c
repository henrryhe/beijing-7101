/*****************************************************************************
 *
 *  Module      : stvid_ioctl
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

#include "stvid_ioctl.h"
#include "stvid_ioctl_types.h"

#include "stvid.h"   /* A STAPI ioctl driver. */
#include "stevt.h"
#include "sttbx.h"

#include "stvid_ioctl_fops.h"

#include "api.h"    /* to access private video driver structures */

#ifdef STVID_DIRECT_INTERFACE_PTI
#include "stpti.h"
#endif

/*** PROTOTYPES **************************************************************/

/*** METHODS *****************************************************************/

/****Types********************************************************************/

/****Variables********************************************************************/

#ifdef STVID_USE_CRC
    static STVID_RefCRCEntry_t *KernelRefCRCTable = NULL;
    static STVID_CompCRCEntry_t  *KernelCompCRCTable = NULL;
    static uint CompCRCTableSize = 0;
    static STVID_CompCRCEntry_t  *UserCompCRCTable = NULL;
    static STVID_CRCErrorLogEntry_t    *KernelErrorLogTable = NULL;
    static STVID_CRCErrorLogEntry_t    *UserErrorLogTable = NULL;
    static uint ErrorLogTableSize = 0;
 #endif

/****Functions****************************************************************/

static STVID_Ioctl_CallbackData_t   CallbackData;

static DECLARE_MUTEX_LOCKED(callback_call_sem);
static DECLARE_MUTEX_LOCKED(callback_return_sem);
static DECLARE_MUTEX(callback_mutex);

/*** METHODS *****************************************************************/

/*-------------------------------------------------------------------------
 * Function: VID_GetPTIPESBuffWritePtr
 * Decription: Gets the PES Buffer Write Pointer from PTI
 * Input   : PTI Buffer Handle
 * Output  :
 * Return  : Error code and the Audio Input Buffer Address
 * ----------------------------------------------------------------------*/
#ifdef STVID_DIRECT_INTERFACE_PTI
static ST_ErrorCode_t VID_GetPTIPESBuffWritePtr(void * const Handle_p, void ** const Address_pp)
{
    ST_ErrorCode_t  Err;

#if !defined STVID_PTI_BELOW_6_8_0
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) != ST_NO_ERROR)
    {
        *Address_pp = NULL;
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    Err = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle_p, Address_pp);

#if !defined STVID_PTI_BELOW_6_8_0 && defined STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
    /* CAUTION: 32 bit porting issue !!!! */
    /* Because PES buffer address retrieved by STVID through STVID_GetDataInputBufferParams() */
    /* is mmmaped in the user wrapper in order to manage only user addresses in user space for user injection (HDD, memory, ...) */
    /* PES addresses have been passed to PTI as user addresses with STPTI_BufferAllocateManualUser() */
    /* this function translates the user address into a CACHED kernel virtual address instead of an UNCACHED kernel virtual address */
    /* STPTI_BufferGetWritePointer() returns then CACHED kernel virtual address but STVID has been designed for UNCACHED addresses */
    /* we are twidling bits in order to manage uncached addresses and recover PTI injection in this mode */
    /* THIS HAS TO BE MODIFIED DURING 32 BITS PORTING */
    *Address_pp = (void *)((U32)*Address_pp | 0x20000000);
#endif

#if !defined STVID_PTI_BELOW_6_8_0
    *Address_pp = STAVMEM_VirtualToDevice(*Address_pp, &VirtualMapping);
#endif

    return Err;
}
#endif

/*-------------------------------------------------------------------------
 * Function: VID_SetPTIPESBuffReadPtrFct
 * Decription: Sets the PTI Read Pointer.
 * Input   : PTI Buffer Handle and the Audio Buffer Address
 * Output  :
 * Return  : Error code
 * ----------------------------------------------------------------------*/
#ifdef STVID_DIRECT_INTERFACE_PTI
static void VID_SetPTIPESBuffReadPtrFct(void * const Handle_p, void * const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        /* just cast the handle */
        STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle_p, STAVMEM_DeviceToVirtual(Address_p, &VirtualMapping));
    }
}
#endif

/*-------------------------------------------------------------------------
 * Function: VID_GetUserPESBuffWritePtr
 * Decription: Gets the PES Buffer Write Pointer from PTI
 * Input   : PTI Buffer Handle
 * Output  :
 * Return  : Error code and the Audio Input Buffer Address
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t VID_GetUserPESBuffWritePtr(void * const Handle_p, void ** const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    ST_ErrorCode_t                          Err = ST_NO_ERROR;
    
    down(&callback_mutex);
    
    /* Set parameters for callback */
    CallbackData.ErrorCode = ST_NO_ERROR;
    CallbackData.CBType = STVID_CB_WRITE_PTR;
    CallbackData.Handle = Handle_p;
    CallbackData.Address = NULL;
        
    /* Trigger the callback to run ... */
    up(&callback_call_sem);

    /* ... and wait for the callback to complete */
    down(&callback_return_sem);

    if (CallbackData.Address == NULL)
    {
        if (CallbackData.ErrorCode == ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VID_GetUserPESBuffWritePtr() NULL pointer returned"));
            Err = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
        {
             *Address_p = STAVMEM_VirtualToDevice(CallbackData.Address, &VirtualMapping);
             Err = CallbackData.ErrorCode;
        }
        else
        {
             *Address_p = NULL;
             Err = ST_ERROR_BAD_PARAMETER;
        }
        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_GetUserPESBuffWritePtr()=0x%08x",*Address_p));*/
    }

    up(&callback_mutex);
    
    return Err;
}

/*-------------------------------------------------------------------------
 * Function: VID_SetUserPESBuffReadPtrFct
 * Decription: Sets the PTI Read Pointer.
 * Input   : PTI Buffer Handle and the Audio Buffer Address
 * Output  :
 * Return  : Error code
 * ----------------------------------------------------------------------*/
static void VID_SetUserPESBuffReadPtrFct(void * const Handle_p, void * const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    down(&callback_mutex);
    
    /* Set parameters for callback */
    CallbackData.ErrorCode = ST_NO_ERROR;
    CallbackData.CBType = STVID_CB_READ_PTR;
    CallbackData.Handle = Handle_p;
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        /* just cast the handle */
        CallbackData.Address = STAVMEM_DeviceToVirtual(Address_p, &VirtualMapping);
    }
    else
    {
        CallbackData.Address = NULL;
    }
    /*STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VID_SetUserPESBuffReadPtrFct()=0x%08x",CallbackData.Address));*/
    
    /* Trigger the callback to run  ... */
    up(&callback_call_sem);

    /* ... and wait for the callback to complete */
    down(&callback_return_sem);
   
    up(&callback_mutex);
}


/*=============================================================================

   stvid_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stvid_ioctl.h'.

  ===========================================================================*/
int stvid_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int                 ret = 0;
    void              * Ioctl_Param_p = NULL;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STVID: ioctl command %d [0x%.8x]", _IOC_NR(cmd), cmd));

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
        case STVID_IOC_INIT:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Init_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Init_t *)arg, sizeof(VID_Ioctl_Init_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* Here is the case where the AVMEMPartition handle is know in Kernel space so we have to convert index to handle */
                ((VID_Ioctl_Init_t *)Ioctl_Param_p)->InitParams.AVMEMPartition =
                            STAVMEM_GetPartitionHandle(((VID_Ioctl_Init_t *)Ioctl_Param_p)->InitParams.AVMEMPartition);

                ((VID_Ioctl_Init_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Init(((VID_Ioctl_Init_t *)Ioctl_Param_p)->DeviceName,
                                       &(((VID_Ioctl_Init_t *)Ioctl_Param_p)->InitParams ));

                if ((ret = copy_to_user((VID_Ioctl_Init_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Init_t))) != 0)
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

        case STVID_IOC_OPEN:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Open_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Open_t *)arg, sizeof(VID_Ioctl_Open_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Open_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Open(((VID_Ioctl_Open_t *)Ioctl_Param_p)->DeviceName,
                                            &( ((VID_Ioctl_Open_t *)Ioctl_Param_p)->OpenParams ),
                                            &( ((VID_Ioctl_Open_t *)Ioctl_Param_p)->Handle ));

                if ((ret = copy_to_user((VID_Ioctl_Open_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Open_t))) != 0)
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

        case STVID_IOC_CLOSE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Close_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Close_t *)arg, sizeof(VID_Ioctl_Close_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Close_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Close(((VID_Ioctl_Close_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_Close_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Close_t))) != 0)
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

        case STVID_IOC_TERM:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Term_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Term_t *)arg, sizeof(VID_Ioctl_Term_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Term_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Term(((VID_Ioctl_Term_t *)Ioctl_Param_p)->DeviceName,
                                            &( ((VID_Ioctl_Term_t *)Ioctl_Param_p)->TermParams ));

                if ((ret = copy_to_user((VID_Ioctl_Term_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Term_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

#ifdef STVID_USE_CRC
                /* We need to de-allocate crc check arrays in kernel space if they have been allocated */
                if(KernelRefCRCTable != NULL)
                {
                    STOS_MemoryDeallocate (NULL, KernelRefCRCTable);
                    KernelRefCRCTable = NULL;
                }

                if(KernelCompCRCTable != NULL)
                {
                    STOS_MemoryDeallocate (NULL, KernelCompCRCTable);
                    KernelCompCRCTable = NULL;
                }

                if(KernelErrorLogTable != NULL)
                {
                    STOS_MemoryDeallocate (NULL, KernelErrorLogTable);
                    KernelErrorLogTable = NULL;
                }
#endif
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVID_IOC_GETREVISION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetRevision_t));
            if (Ioctl_Param_p != NULL)
            {
                ((VID_Ioctl_GetRevision_t *)Ioctl_Param_p)->Revision = STVID_GetRevision();

                if ((ret = copy_to_user((VID_Ioctl_GetRevision_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetRevision_t))) != 0)
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

        case STVID_IOC_GETCAPABILITY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetCapability_t));
            if (Ioctl_Param_p != NULL)
            {
                void * HScalingFactors_p;
                void * VScalingFactors_p;
                struct VideoDeviceData_s * VideoDeviceData_p;

                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetCapability_t *)arg, sizeof(VID_Ioctl_GetCapability_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
                HScalingFactors_p = ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability.SupportedHorizontalScaling.ScalingFactors_p;
                VScalingFactors_p = ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability.SupportedVerticalScaling.ScalingFactors_p;

                ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetCapability(((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->DeviceName,
                                            &( ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability ));


                if ((ret = copy_to_user( HScalingFactors_p, ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability.SupportedHorizontalScaling.ScalingFactors_p, sizeof(VideoDeviceData_p->HorizontalFactors))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
                if ((ret = copy_to_user( VScalingFactors_p, ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability.SupportedHorizontalScaling.ScalingFactors_p, sizeof(VideoDeviceData_p->VerticalFactors))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
                ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability.SupportedHorizontalScaling.ScalingFactors_p = HScalingFactors_p;
                ((VID_Ioctl_GetCapability_t *)Ioctl_Param_p)->Capability.SupportedVerticalScaling.ScalingFactors_p = VScalingFactors_p;
                if ((ret = copy_to_user((VID_Ioctl_GetCapability_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetCapability_t))) != 0)
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

#ifdef STVID_DEBUG_GET_STATISTICS
        case STVID_IOC_GETSTATISTICS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetStatistics_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetStatistics_t *)arg, sizeof(VID_Ioctl_GetStatistics_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetStatistics_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetStatistics(((VID_Ioctl_GetStatistics_t *)Ioctl_Param_p)->DeviceName,
                                            &( ((VID_Ioctl_GetStatistics_t *)Ioctl_Param_p)->Statistics ));

                if ((ret = copy_to_user((VID_Ioctl_GetStatistics_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetStatistics_t))) != 0)
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

        case STVID_IOC_RESETSTATISTICS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_ResetStatistics_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_ResetStatistics_t *)arg, sizeof(VID_Ioctl_ResetStatistics_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_ResetStatistics_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_ResetStatistics(((VID_Ioctl_ResetStatistics_t *)Ioctl_Param_p)->DeviceName,
                                            &( ((VID_Ioctl_ResetStatistics_t *)Ioctl_Param_p)->Statistics ));

                if ((ret = copy_to_user((VID_Ioctl_ResetStatistics_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_ResetStatistics_t))) != 0)
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
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
        case STVID_IOC_GETSTATUS:
            Ioctl_Param_p = memory_allocate (NULL, sizeof(VID_Ioctl_GetStatus_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetStatus_t *)arg, sizeof(VID_Ioctl_GetStatus_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetStatus_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetStatus(((VID_Ioctl_GetStatus_t *)Ioctl_Param_p)->DeviceName,
                                            &( ((VID_Ioctl_GetStatus_t *)Ioctl_Param_p)->Status ));

                if ((ret = copy_to_user((VID_Ioctl_GetStatus_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetStatus_t))) != 0)
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

#endif /* STVID_DEBUG_GET_STATUS */


        case STVID_IOC_CLEAR:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Clear_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Clear_t *)arg, sizeof(VID_Ioctl_Clear_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Clear_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Clear(((VID_Ioctl_Clear_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_Clear_t *)Ioctl_Param_p)->Params ));

                if ((ret = copy_to_user((VID_Ioctl_Clear_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Clear_t))) != 0)
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

        case STVID_IOC_CLOSEVIEWPORT:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CloseViewPort_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CloseViewPort_t *)arg, sizeof(VID_Ioctl_CloseViewPort_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CloseViewPort_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_CloseViewPort(((VID_Ioctl_CloseViewPort_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_CloseViewPort_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CloseViewPort_t))) != 0)
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

        case STVID_IOC_CONFIGUREEVENT:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_ConfigureEvent_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_ConfigureEvent_t *)arg, sizeof(VID_Ioctl_ConfigureEvent_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_ConfigureEvent_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_ConfigureEvent(((VID_Ioctl_ConfigureEvent_t *)Ioctl_Param_p)->Handle,
                                             ((VID_Ioctl_ConfigureEvent_t *)Ioctl_Param_p)->Event,
                                          &( ((VID_Ioctl_ConfigureEvent_t *)Ioctl_Param_p)->Params) );

                if ((ret = copy_to_user((VID_Ioctl_ConfigureEvent_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_ConfigureEvent_t))) != 0)
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

        case STVID_IOC_DATAINJECTIONCOMPLETED:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DataInjectionCompleted_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DataInjectionCompleted_t *)arg, sizeof(VID_Ioctl_DataInjectionCompleted_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DataInjectionCompleted_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DataInjectionCompleted(((VID_Ioctl_DataInjectionCompleted_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_DataInjectionCompleted_t *)Ioctl_Param_p)->Params ));

                if ((ret = copy_to_user((VID_Ioctl_DataInjectionCompleted_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DataInjectionCompleted_t))) != 0)
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

        case STVID_IOC_DELETEDATAINPUTINTERFACE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DeleteDataInputInterface_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DeleteDataInputInterface_t *)arg, sizeof(VID_Ioctl_DeleteDataInputInterface_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DeleteDataInputInterface_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DeleteDataInputInterface(((VID_Ioctl_DeleteDataInputInterface_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_DeleteDataInputInterface_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DeleteDataInputInterface_t))) != 0)
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

        case STVID_IOC_DISABLEBORDERALPHA:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableBorderAlpha_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableBorderAlpha_t *)arg, sizeof(VID_Ioctl_DisableBorderAlpha_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableBorderAlpha_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableBorderAlpha(((VID_Ioctl_DisableBorderAlpha_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_DisableBorderAlpha_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableBorderAlpha_t))) != 0)
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

        case STVID_IOC_DISABLECOLORKEY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableColorKey_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableColorKey_t *)arg, sizeof(VID_Ioctl_DisableColorKey_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableColorKey_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableColorKey(((VID_Ioctl_DisableColorKey_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_DisableColorKey_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableColorKey_t))) != 0)
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

        case STVID_IOC_DISABLEDEBLOCKING:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableDeblocking_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableDeblocking_t *)arg, sizeof(VID_Ioctl_DisableDeblocking_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableDeblocking_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableDeblocking(((VID_Ioctl_DisableDeblocking_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_DisableDeblocking_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableDeblocking_t))) != 0)
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

        case STVID_IOC_DISABLEDISPLAY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableDisplay_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableDisplay_t *)arg, sizeof(VID_Ioctl_DisableDisplay_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableDisplay_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableDisplay(((VID_Ioctl_DisableDisplay_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_DisableDisplay_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableDisplay_t))) != 0)
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

        case STVID_IOC_DISABLEFRAMERATECONVERSION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableFrameRateConversion_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableFrameRateConversion_t *)arg, sizeof(VID_Ioctl_DisableFrameRateConversion_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableFrameRateConversion_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableFrameRateConversion(((VID_Ioctl_DisableFrameRateConversion_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_DisableFrameRateConversion_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableFrameRateConversion_t))) != 0)
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

        case STVID_IOC_DISABLEOUTPUTWINDOW:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableOutputWindow_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableOutputWindow_t *)arg, sizeof(VID_Ioctl_DisableOutputWindow_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableOutputWindow_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableOutputWindow(((VID_Ioctl_DisableOutputWindow_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_DisableOutputWindow_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableOutputWindow_t))) != 0)
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

        case STVID_IOC_DISABLESYNCHRONISATION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisableSynchronisation_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisableSynchronisation_t *)arg, sizeof(VID_Ioctl_DisableSynchronisation_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisableSynchronisation_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisableSynchronisation(((VID_Ioctl_DisableSynchronisation_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_DisableSynchronisation_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisableSynchronisation_t))) != 0)
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

        case STVID_IOC_DUPLICATEPICTURE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DuplicatePicture_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DuplicatePicture_t *)arg, sizeof(VID_Ioctl_DuplicatePicture_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DuplicatePicture_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DuplicatePicture(&(((VID_Ioctl_DuplicatePicture_t *)Ioctl_Param_p)->Source),
                                            &( ((VID_Ioctl_DuplicatePicture_t *)Ioctl_Param_p)->Dest ));

                if ((ret = copy_to_user((VID_Ioctl_DuplicatePicture_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DuplicatePicture_t))) != 0)
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

        case STVID_IOC_ENABLEBORDERALPHA:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableBorderAlpha_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableBorderAlpha_t *)arg, sizeof(VID_Ioctl_EnableBorderAlpha_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableBorderAlpha_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableBorderAlpha(((VID_Ioctl_EnableBorderAlpha_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_EnableBorderAlpha_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableBorderAlpha_t))) != 0)
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

        case STVID_IOC_ENABLECOLORKEY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableColorKey_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableColorKey_t *)arg, sizeof(VID_Ioctl_EnableColorKey_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableColorKey_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableColorKey(((VID_Ioctl_EnableColorKey_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_EnableColorKey_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableColorKey_t))) != 0)
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

        case STVID_IOC_ENABLEDEBLOCKING:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableDeblocking_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableDeblocking_t *)arg, sizeof(VID_Ioctl_EnableDeblocking_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableDeblocking_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableDeblocking(((VID_Ioctl_EnableDeblocking_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_EnableDeblocking_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableDeblocking_t))) != 0)
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

        case STVID_IOC_ENABLEDISPLAY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableDisplay_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableDisplay_t *)arg, sizeof(VID_Ioctl_EnableDisplay_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableDisplay_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableDisplay(((VID_Ioctl_EnableDisplay_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_EnableDisplay_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableDisplay_t))) != 0)
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
        case STVID_IOC_ENABLEFRAMERATECONVERSION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableFrameRateConversion_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableFrameRateConversion_t *)arg, sizeof(VID_Ioctl_EnableFrameRateConversion_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableFrameRateConversion_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableFrameRateConversion(((VID_Ioctl_EnableFrameRateConversion_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_EnableFrameRateConversion_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableFrameRateConversion_t))) != 0)
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

        case STVID_IOC_ENABLEOUTPUTWINDOW:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableOutputWindow_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableOutputWindow_t *)arg, sizeof(VID_Ioctl_EnableOutputWindow_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableOutputWindow_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableOutputWindow(((VID_Ioctl_EnableOutputWindow_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_EnableOutputWindow_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableOutputWindow_t))) != 0)
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

        case STVID_IOC_ENABLESYNCHRONISATION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_EnableSynchronisation_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_EnableSynchronisation_t *)arg, sizeof(VID_Ioctl_EnableSynchronisation_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_EnableSynchronisation_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_EnableSynchronisation(((VID_Ioctl_EnableSynchronisation_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_EnableSynchronisation_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_EnableSynchronisation_t))) != 0)
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

        case STVID_IOC_FREEZE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Freeze_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Freeze_t *)arg, sizeof(VID_Ioctl_Freeze_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Freeze_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Freeze(((VID_Ioctl_Freeze_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_Freeze_t *)Ioctl_Param_p)->Freeze ));

                if ((ret = copy_to_user((VID_Ioctl_Freeze_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Freeze_t))) != 0)
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

        case STVID_IOC_FORCEDECIMATIONFACTOR:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_ForceDecimationFactor_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_ForceDecimationFactor_t *)arg, sizeof(VID_Ioctl_ForceDecimationFactor_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_ForceDecimationFactor_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_ForceDecimationFactor(((VID_Ioctl_ForceDecimationFactor_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_ForceDecimationFactor_t *)Ioctl_Param_p)->ForceDecimationFactorParams ));

                if ((ret = copy_to_user((VID_Ioctl_ForceDecimationFactor_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_ForceDecimationFactor_t))) != 0)
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

        case STVID_IOC_GETALIGNIOWINDOWS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetAlignIOWindows_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetAlignIOWindows_t *)arg, sizeof(VID_Ioctl_GetAlignIOWindows_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetAlignIOWindows(((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->InputWinX ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->InputWinY ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->InputWinWidth ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->InputWinHeight ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->OutputWinX ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->OutputWinY ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->OutputWinWidth ),
                                            &( ((VID_Ioctl_GetAlignIOWindows_t *)Ioctl_Param_p)->OutputWinHeight ));

                if ((ret = copy_to_user((VID_Ioctl_GetAlignIOWindows_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetAlignIOWindows_t))) != 0)
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

        case STVID_IOC_GETBITBUFFERFREESIZE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetBitBufferFreeSize_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetBitBufferFreeSize_t *)arg, sizeof(VID_Ioctl_GetBitBufferFreeSize_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetBitBufferFreeSize_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetBitBufferFreeSize(((VID_Ioctl_GetBitBufferFreeSize_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetBitBufferFreeSize_t *)Ioctl_Param_p)->FreeSize ));

                if ((ret = copy_to_user((VID_Ioctl_GetBitBufferFreeSize_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetBitBufferFreeSize_t))) != 0)
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

        case STVID_IOC_GETBITBUFFERPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetBitBufferParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetBitBufferParams_t *)arg, sizeof(VID_Ioctl_GetBitBufferParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetBitBufferParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetBitBufferParams(((VID_Ioctl_GetBitBufferParams_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetBitBufferParams_t *)Ioctl_Param_p)->BaseAddress_p),&( ((VID_Ioctl_GetBitBufferParams_t *)Ioctl_Param_p)->InitSize));

                if ((ret = copy_to_user((VID_Ioctl_GetBitBufferParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetBitBufferParams_t))) != 0)
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


        case STVID_IOC_GETDATAINPUTBUFFERPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetDataInputBufferParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetDataInputBufferParams_t *)arg, sizeof(VID_Ioctl_GetDataInputBufferParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetDataInputBufferParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetDataInputBufferParams(((VID_Ioctl_GetDataInputBufferParams_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetDataInputBufferParams_t *)Ioctl_Param_p)->BaseAddress_p),
                                            &( ((VID_Ioctl_GetDataInputBufferParams_t *)Ioctl_Param_p)->Size ));

                if ((ret = copy_to_user((VID_Ioctl_GetDataInputBufferParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetDataInputBufferParams_t))) != 0)
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

        case STVID_IOC_GETDECIMATIONFACTOR:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetDecimationFactor_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetDecimationFactor_t *)arg, sizeof(VID_Ioctl_GetDecimationFactor_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetDecimationFactor_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetDecimationFactor(((VID_Ioctl_GetDecimationFactor_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetDecimationFactor_t *)Ioctl_Param_p)->DecimationFactor ));

                if ((ret = copy_to_user((VID_Ioctl_GetDecimationFactor_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetDecimationFactor_t))) != 0)
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

        case STVID_IOC_GETDECODEDPICTURES:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetDecodedPictures_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetDecodedPictures_t *)arg, sizeof(VID_Ioctl_GetDecodedPictures_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetDecodedPictures_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetDecodedPictures(((VID_Ioctl_GetDecodedPictures_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetDecodedPictures_t *)Ioctl_Param_p)->DecodedPictures ));

                if ((ret = copy_to_user((VID_Ioctl_GetDecodedPictures_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetDecodedPictures_t))) != 0)
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

        case STVID_IOC_GETDISPLAYASPECTRATIOCONVERSION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetDisplayAspectRatioConversion_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetDisplayAspectRatioConversion_t *)arg, sizeof(VID_Ioctl_GetDisplayAspectRatioConversion_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetDisplayAspectRatioConversion_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetDisplayAspectRatioConversion(((VID_Ioctl_GetDisplayAspectRatioConversion_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetDisplayAspectRatioConversion_t *)Ioctl_Param_p)->Conversion ));

                if ((ret = copy_to_user((VID_Ioctl_GetDisplayAspectRatioConversion_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetDisplayAspectRatioConversion_t))) != 0)
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

        case STVID_IOC_GETERRORRECOVERYMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetErrorRecoveryMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetErrorRecoveryMode_t *)arg, sizeof(VID_Ioctl_GetErrorRecoveryMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetErrorRecoveryMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetErrorRecoveryMode(((VID_Ioctl_GetErrorRecoveryMode_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetErrorRecoveryMode_t *)Ioctl_Param_p)->Mode ));

                if ((ret = copy_to_user((VID_Ioctl_GetErrorRecoveryMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetErrorRecoveryMode_t))) != 0)
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

        case STVID_IOC_VID_GETHDPIPPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetHDPIPParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetHDPIPParams_t *)arg, sizeof(VID_Ioctl_GetHDPIPParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetHDPIPParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetHDPIPParams(((VID_Ioctl_GetHDPIPParams_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetHDPIPParams_t *)Ioctl_Param_p)->HDPIPParams ));

                if ((ret = copy_to_user((VID_Ioctl_GetHDPIPParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetHDPIPParams_t))) != 0)
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

        case STVID_IOC_GETINPUTWINDOWMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetInputWindowMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetInputWindowMode_t *)arg, sizeof(VID_Ioctl_GetInputWindowMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetInputWindowMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetInputWindowMode(((VID_Ioctl_GetInputWindowMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetInputWindowMode_t *)Ioctl_Param_p)->AutoMode ),
                                            &( ((VID_Ioctl_GetInputWindowMode_t *)Ioctl_Param_p)->WinParams ));

                if ((ret = copy_to_user((VID_Ioctl_GetInputWindowMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetInputWindowMode_t))) != 0)
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

        case STVID_IOC_GETIOWINDOWS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetIOWindows_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetIOWindows_t *)arg, sizeof(VID_Ioctl_GetIOWindows_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetIOWindows(((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->InputWinX ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->InputWinY ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->InputWinWidth ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->InputWinHeight ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->OutputWinX ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->OutputWinY ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->OutputWinWidth ),
                                            &( ((VID_Ioctl_GetIOWindows_t *)Ioctl_Param_p)->OutputWinHeight ));

                if ((ret = copy_to_user((VID_Ioctl_GetIOWindows_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetIOWindows_t))) != 0)
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

        case STVID_IOC_GETMEMORYPROFILE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetMemoryProfile_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetMemoryProfile_t *)arg, sizeof(VID_Ioctl_GetMemoryProfile_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetMemoryProfile_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetMemoryProfile(((VID_Ioctl_GetMemoryProfile_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetMemoryProfile_t *)Ioctl_Param_p)->MemoryProfile ));

                if ((ret = copy_to_user((VID_Ioctl_GetMemoryProfile_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetMemoryProfile_t))) != 0)
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

        case STVID_IOC_GETOUTPUTWINDOWMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetOutputWindowMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetOutputWindowMode_t *)arg, sizeof(VID_Ioctl_GetOutputWindowMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetOutputWindowMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetOutputWindowMode(((VID_Ioctl_GetOutputWindowMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetOutputWindowMode_t *)Ioctl_Param_p)->AutoMode ),
                                            &( ((VID_Ioctl_GetOutputWindowMode_t *)Ioctl_Param_p)->WinParams ));

                if ((ret = copy_to_user((VID_Ioctl_GetOutputWindowMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetOutputWindowMode_t))) != 0)
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

        case STVID_IOC_GETPICTUREALLOCPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetPictureAllocParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetPictureAllocParams_t *)arg, sizeof(VID_Ioctl_GetPictureAllocParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetPictureAllocParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetPictureAllocParams(((VID_Ioctl_GetPictureAllocParams_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetPictureAllocParams_t *)Ioctl_Param_p)->Params ),
                                            &( ((VID_Ioctl_GetPictureAllocParams_t *)Ioctl_Param_p)->AllocParams ));

                if ((ret = copy_to_user((VID_Ioctl_GetPictureAllocParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetPictureAllocParams_t))) != 0)
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

        case STVID_IOC_GETPICTUREPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetPictureParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetPictureParams_t *)arg, sizeof(VID_Ioctl_GetPictureParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetPictureParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetPictureParams(((VID_Ioctl_GetPictureParams_t *)Ioctl_Param_p)->Handle,
                                             ((VID_Ioctl_GetPictureParams_t *)Ioctl_Param_p)->PictureType,
                                           &(  ((VID_Ioctl_GetPictureParams_t *)Ioctl_Param_p)->Params) );

                if ((ret = copy_to_user((VID_Ioctl_GetPictureParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetPictureParams_t))) != 0)
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

        case STVID_IOC_GETPICTUREINFOS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetPictureInfos_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetPictureInfos_t *)arg, sizeof(VID_Ioctl_GetPictureInfos_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetPictureInfos_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetPictureInfos(((VID_Ioctl_GetPictureInfos_t *)Ioctl_Param_p)->Handle,
                                             ((VID_Ioctl_GetPictureInfos_t *)Ioctl_Param_p)->PictureType ,
                                            &( ((VID_Ioctl_GetPictureInfos_t *)Ioctl_Param_p)->PictureInfos ));

                if ((ret = copy_to_user((VID_Ioctl_GetPictureInfos_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetPictureInfos_t))) != 0)
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
        case STVID_IOC_GETDISPLAYPICTUREINFO:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetDisplayPictureInfo_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetDisplayPictureInfo_t *)arg, sizeof(VID_Ioctl_GetDisplayPictureInfo_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetDisplayPictureInfo_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetDisplayPictureInfo(((VID_Ioctl_GetDisplayPictureInfo_t *)Ioctl_Param_p)->Handle,
                                             ((VID_Ioctl_GetDisplayPictureInfo_t *)Ioctl_Param_p)->PictureBufferHandle ,
                                            &( ((VID_Ioctl_GetDisplayPictureInfo_t *)Ioctl_Param_p)->DisplayPictureInfos ));

                if ((ret = copy_to_user((VID_Ioctl_GetDisplayPictureInfo_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetDisplayPictureInfo_t))) != 0)
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


        case STVID_IOC_GETSPEED:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetSpeed_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetSpeed_t *)arg, sizeof(VID_Ioctl_GetSpeed_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetSpeed_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetSpeed(((VID_Ioctl_GetSpeed_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetSpeed_t *)Ioctl_Param_p)->Speed ));

                if ((ret = copy_to_user((VID_Ioctl_GetSpeed_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetSpeed_t))) != 0)
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

#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS           
        case STVID_IOC_GETVIDEODISPLAYPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetVideoDisplayParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetVideoDisplayParams_t *)arg, sizeof(VID_Ioctl_GetVideoDisplayParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetVideoDisplayParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetVideoDisplayParams(((VID_Ioctl_GetVideoDisplayParams_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetVideoDisplayParams_t *)Ioctl_Param_p)->Params ));

                if ((ret = copy_to_user((VID_Ioctl_GetVideoDisplayParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetVideoDisplayParams_t))) != 0)
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
#endif

        case STVID_IOC_GETVIEWPORTALPHA:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetViewPortAlpha_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetViewPortAlpha_t *)arg, sizeof(VID_Ioctl_GetViewPortAlpha_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetViewPortAlpha_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetViewPortAlpha(((VID_Ioctl_GetViewPortAlpha_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetViewPortAlpha_t *)Ioctl_Param_p)->GlobalAlpha ));

                if ((ret = copy_to_user((VID_Ioctl_GetViewPortAlpha_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetViewPortAlpha_t))) != 0)
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

        case STVID_IOC_GETVIEWPORTCOLORKEY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetViewPortColorKey_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetViewPortColorKey_t *)arg, sizeof(VID_Ioctl_GetViewPortColorKey_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetViewPortColorKey_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetViewPortColorKey(((VID_Ioctl_GetViewPortColorKey_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetViewPortColorKey_t *)Ioctl_Param_p)->ColorKey ));

                if ((ret = copy_to_user((VID_Ioctl_GetViewPortColorKey_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetViewPortColorKey_t))) != 0)
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

        case STVID_IOC_GETVIEWPORTPSI:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetViewPortPSI_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetViewPortPSI_t *)arg, sizeof(VID_Ioctl_GetViewPortPSI_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetViewPortPSI_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetViewPortPSI(((VID_Ioctl_GetViewPortPSI_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetViewPortPSI_t *)Ioctl_Param_p)->VPPSI ));

                if ((ret = copy_to_user((VID_Ioctl_GetViewPortPSI_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetViewPortPSI_t))) != 0)
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

        case STVID_IOC_GETVIEWPORTSPECIALMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetViewPortSpecialMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetViewPortSpecialMode_t *)arg, sizeof(VID_Ioctl_GetViewPortSpecialMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetViewPortSpecialMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetViewPortSpecialMode(((VID_Ioctl_GetViewPortSpecialMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetViewPortSpecialMode_t *)Ioctl_Param_p)->OuputMode ),
                                            &( ((VID_Ioctl_GetViewPortSpecialMode_t *)Ioctl_Param_p)->Params ));

                if ((ret = copy_to_user((VID_Ioctl_GetViewPortSpecialMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetViewPortSpecialMode_t))) != 0)
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

        case STVID_IOC_HIDEPICTURE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_HidePicture_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_HidePicture_t *)arg, sizeof(VID_Ioctl_HidePicture_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_HidePicture_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_HidePicture(((VID_Ioctl_HidePicture_t *)Ioctl_Param_p)->ViewPortHandle);

                if ((ret = copy_to_user((VID_Ioctl_HidePicture_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_HidePicture_t))) != 0)
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

        case STVID_IOC_INJECTDISCONTINUITY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_InjectDiscontinuity_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_InjectDiscontinuity_t *)arg, sizeof(VID_Ioctl_InjectDiscontinuity_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_InjectDiscontinuity_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_InjectDiscontinuity(((VID_Ioctl_InjectDiscontinuity_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_InjectDiscontinuity_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_InjectDiscontinuity_t))) != 0)
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

        case STVID_IOC_OPENVIEWPORT:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_OpenViewPort_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_OpenViewPort_t *)arg, sizeof(VID_Ioctl_OpenViewPort_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_OpenViewPort_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_OpenViewPort(((VID_Ioctl_OpenViewPort_t *)Ioctl_Param_p)->VideoHandle,
                                            &( ((VID_Ioctl_OpenViewPort_t *)Ioctl_Param_p)->ViewPortParams ),
                                            &( ((VID_Ioctl_OpenViewPort_t *)Ioctl_Param_p)->ViewPortHandle ));

                if ((ret = copy_to_user((VID_Ioctl_OpenViewPort_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_OpenViewPort_t))) != 0)
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


        case STVID_IOC_PAUSE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Pause_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Pause_t *)arg, sizeof(VID_Ioctl_Pause_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Pause_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Pause(((VID_Ioctl_Pause_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_Pause_t *)Ioctl_Param_p)->Freeze ));

                if ((ret = copy_to_user((VID_Ioctl_Pause_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Pause_t))) != 0)
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

        case STVID_IOC_PAUSESYNCHRO:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_PauseSynchro_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_PauseSynchro_t *)arg, sizeof(VID_Ioctl_PauseSynchro_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_PauseSynchro_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_PauseSynchro(((VID_Ioctl_PauseSynchro_t *)Ioctl_Param_p)->Handle,
                                            ((VID_Ioctl_PauseSynchro_t *)Ioctl_Param_p)->Time);

                if ((ret = copy_to_user((VID_Ioctl_PauseSynchro_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_PauseSynchro_t))) != 0)
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

        case STVID_IOC_RESUME:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Resume_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Resume_t *)arg, sizeof(VID_Ioctl_Resume_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Resume_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Resume(((VID_Ioctl_Resume_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_Resume_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Resume_t))) != 0)
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

        case STVID_IOC_SETDATAINPUTINTERFACE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetDataInputInterface_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetDataInputInterface_t *)arg, sizeof(VID_Ioctl_SetDataInputInterface_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

#ifdef STVID_DIRECT_INTERFACE_PTI
                /* Nulls indicate connection to the PTI */
                if (   (((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->GetWriteAddress == NULL)
                    || (((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->InformReadAddress == NULL))
                {
                    ((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetDataInputInterface(((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->Handle,
                                             VID_GetPTIPESBuffWritePtr,
                                             VID_SetPTIPESBuffReadPtrFct,
                                             (void*)((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->FunctionsHandle);
                }
                else
#endif
                {
                    ((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetDataInputInterface(((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->Handle,
                                             VID_GetUserPESBuffWritePtr,
                                             VID_SetUserPESBuffReadPtrFct,
                                             (void*)((VID_Ioctl_SetDataInputInterface_t *)Ioctl_Param_p)->FunctionsHandle);
                }

                if ((ret = copy_to_user((VID_Ioctl_SetDataInputInterface_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetDataInputInterface_t))) != 0)
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

        case STVID_IOC_SETDECODEDPICTURES:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetDecodedPictures_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetDecodedPictures_t *)arg, sizeof(VID_Ioctl_SetDecodedPictures_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetDecodedPictures_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetDecodedPictures(((VID_Ioctl_SetDecodedPictures_t *)Ioctl_Param_p)->Handle,
                                           ((VID_Ioctl_SetDecodedPictures_t *)Ioctl_Param_p)->DecodedPictures );

                if ((ret = copy_to_user((VID_Ioctl_SetDecodedPictures_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetDecodedPictures_t))) != 0)
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

        case STVID_IOC_SETDISPLAYASPECTRATIOCONVERSION:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetDisplayAspectRatioConversion_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetDisplayAspectRatioConversion_t *)arg, sizeof(VID_Ioctl_SetDisplayAspectRatioConversion_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetDisplayAspectRatioConversion_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetDisplayAspectRatioConversion(((VID_Ioctl_SetDisplayAspectRatioConversion_t *)Ioctl_Param_p)->ViewPortHandle,
                                             ((VID_Ioctl_SetDisplayAspectRatioConversion_t *)Ioctl_Param_p)->Mode );

                if ((ret = copy_to_user((VID_Ioctl_SetDisplayAspectRatioConversion_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetDisplayAspectRatioConversion_t))) != 0)
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

        case STVID_IOC_SETERRORRECOVERYMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetErrorRecoveryMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetErrorRecoveryMode_t *)arg, sizeof(VID_Ioctl_SetErrorRecoveryMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetErrorRecoveryMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetErrorRecoveryMode(((VID_Ioctl_SetErrorRecoveryMode_t *)Ioctl_Param_p)->Handle,
                                            ((VID_Ioctl_SetErrorRecoveryMode_t *)Ioctl_Param_p)->Mode );

                if ((ret = copy_to_user((VID_Ioctl_SetErrorRecoveryMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetErrorRecoveryMode_t))) != 0)
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

        case STVID_IOC_SETHDPIPPARAMS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetHDPIPParams_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetHDPIPParams_t *)arg, sizeof(VID_Ioctl_SetHDPIPParams_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetHDPIPParams_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetHDPIPParams(((VID_Ioctl_SetHDPIPParams_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_SetHDPIPParams_t *)Ioctl_Param_p)->HDPIPParams ));

                if ((ret = copy_to_user((VID_Ioctl_SetHDPIPParams_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetHDPIPParams_t))) != 0)
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

        case STVID_IOC_SETINPUTWINDOWMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetInputWindowMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetInputWindowMode_t *)arg, sizeof(VID_Ioctl_SetInputWindowMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetInputWindowMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetInputWindowMode(((VID_Ioctl_SetInputWindowMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            ((VID_Ioctl_SetInputWindowMode_t *)Ioctl_Param_p)->AutoMode ,
                                            &( ((VID_Ioctl_SetInputWindowMode_t *)Ioctl_Param_p)->WinParams ));

                if ((ret = copy_to_user((VID_Ioctl_SetInputWindowMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetInputWindowMode_t))) != 0)
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

        case STVID_IOC_SETIOWINDOWS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetIOWindows_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetIOWindows_t *)arg, sizeof(VID_Ioctl_SetIOWindows_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetIOWindows(((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->ViewPortHandle,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->InputWinX,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->InputWinY,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->InputWinWidth,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->InputWinHeight,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->OutputWinX,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->OutputWinY,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->OutputWinWidth,
                                             ((VID_Ioctl_SetIOWindows_t *)Ioctl_Param_p)->OutputWinHeight );

                if ((ret = copy_to_user((VID_Ioctl_SetIOWindows_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetIOWindows_t))) != 0)
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

        case STVID_IOC_SETMEMORYPROFILE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetMemoryProfile_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetMemoryProfile_t *)arg, sizeof(VID_Ioctl_SetMemoryProfile_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetMemoryProfile_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetMemoryProfile(((VID_Ioctl_SetMemoryProfile_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_SetMemoryProfile_t *)Ioctl_Param_p)->MemoryProfile ));

                if ((ret = copy_to_user((VID_Ioctl_SetMemoryProfile_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetMemoryProfile_t))) != 0)
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

        case STVID_IOC_SETOUTPUTWINDOWMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetOutputWindowMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetOutputWindowMode_t *)arg, sizeof(VID_Ioctl_SetOutputWindowMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetOutputWindowMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetOutputWindowMode(((VID_Ioctl_SetOutputWindowMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            ((VID_Ioctl_SetOutputWindowMode_t *)Ioctl_Param_p)->AutoMode ,
                                            &( ((VID_Ioctl_SetOutputWindowMode_t *)Ioctl_Param_p)->WinParams ));

                if ((ret = copy_to_user((VID_Ioctl_SetOutputWindowMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetOutputWindowMode_t))) != 0)
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

        case STVID_IOC_SETSPEED:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetSpeed_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetSpeed_t *)arg, sizeof(VID_Ioctl_SetSpeed_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetSpeed_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetSpeed(((VID_Ioctl_SetSpeed_t *)Ioctl_Param_p)->Handle,
                                             ((VID_Ioctl_SetSpeed_t *)Ioctl_Param_p)->Speed );

                if ((ret = copy_to_user((VID_Ioctl_SetSpeed_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetSpeed_t))) != 0)
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

        case STVID_IOC_SETUP:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Setup_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Setup_t *)arg, sizeof(VID_Ioctl_Setup_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* Here is the case where the AVMEMPartition handle is know in Kernel space so we have to convert index to handle */
                ((VID_Ioctl_Setup_t *)Ioctl_Param_p)->SetupParams.SetupSettings.AVMEMPartition =
                            STAVMEM_GetPartitionHandle(((VID_Ioctl_Setup_t *)Ioctl_Param_p)->SetupParams.SetupSettings.AVMEMPartition);
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Setup: Partition has handle:%x",
                                                        ((VID_Ioctl_Setup_t *)Ioctl_Param_p)->SetupParams.SetupSettings.AVMEMPartition));

                ((VID_Ioctl_Setup_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Setup(((VID_Ioctl_Setup_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_Setup_t *)Ioctl_Param_p)->SetupParams ));

                if ((ret = copy_to_user((VID_Ioctl_Setup_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Setup_t))) != 0)
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

        case STVID_IOC_SETVIEWPORTALPHA:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetViewPortAlpha_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetViewPortAlpha_t *)arg, sizeof(VID_Ioctl_SetViewPortAlpha_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetViewPortAlpha_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetViewPortAlpha(((VID_Ioctl_SetViewPortAlpha_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_SetViewPortAlpha_t *)Ioctl_Param_p)->GlobalAlpha ));

                if ((ret = copy_to_user((VID_Ioctl_SetViewPortAlpha_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetViewPortAlpha_t))) != 0)
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

        case STVID_IOC_SETVIEWPORTCOLORKEY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetViewPortColorKey_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetViewPortColorKey_t *)arg, sizeof(VID_Ioctl_SetViewPortColorKey_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetViewPortColorKey_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetViewPortColorKey(((VID_Ioctl_SetViewPortColorKey_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_SetViewPortColorKey_t *)Ioctl_Param_p)->ColorKey ));

                if ((ret = copy_to_user((VID_Ioctl_SetViewPortColorKey_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetViewPortColorKey_t))) != 0)
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

        case STVID_IOC_SETVIEWPORTPSI:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetViewPortPSI_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetViewPortPSI_t *)arg, sizeof(VID_Ioctl_SetViewPortPSI_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetViewPortPSI_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetViewPortPSI(((VID_Ioctl_SetViewPortPSI_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_SetViewPortPSI_t *)Ioctl_Param_p)->VPPSI ));

                if ((ret = copy_to_user((VID_Ioctl_SetViewPortPSI_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetViewPortPSI_t))) != 0)
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

        case STVID_IOC_SETVIEWPORTSPECIALMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetViewPortSpecialMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Init_t *)arg, sizeof(VID_Ioctl_Init_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetViewPortSpecialMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetViewPortSpecialMode(((VID_Ioctl_SetViewPortSpecialMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                             ((VID_Ioctl_SetViewPortSpecialMode_t *)Ioctl_Param_p)->OuputMode ,
                                            &( ((VID_Ioctl_SetViewPortSpecialMode_t *)Ioctl_Param_p)->Params ));

                if ((ret = copy_to_user((VID_Ioctl_SetViewPortSpecialMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetViewPortSpecialMode_t))) != 0)
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

        case STVID_IOC_SHOWPICTURE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_ShowPicture_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_ShowPicture_t *)arg, sizeof(VID_Ioctl_ShowPicture_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_ShowPicture_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_ShowPicture(((VID_Ioctl_ShowPicture_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_ShowPicture_t *)Ioctl_Param_p)->Params),
                                            &( ((VID_Ioctl_ShowPicture_t *)Ioctl_Param_p)->Freeze ));


                if ((ret = copy_to_user((VID_Ioctl_ShowPicture_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_ShowPicture_t))) != 0)
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

        case STVID_IOC_SKIPSYNCHRO:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SkipSynchro_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SkipSynchro_t *)arg, sizeof(VID_Ioctl_SkipSynchro_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SkipSynchro_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SkipSynchro(((VID_Ioctl_SkipSynchro_t *)Ioctl_Param_p)->Handle,
                                              ((VID_Ioctl_SkipSynchro_t *)Ioctl_Param_p)->Time );

                if ((ret = copy_to_user((VID_Ioctl_SkipSynchro_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SkipSynchro_t))) != 0)
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

        case STVID_IOC_START:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Start_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Start_t *)arg, sizeof(VID_Ioctl_Start_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Start_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Start(((VID_Ioctl_Start_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_Start_t *)Ioctl_Param_p)->Params ));

                if ((ret = copy_to_user((VID_Ioctl_Start_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Start_t))) != 0)
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

        case STVID_IOC_STARTUPDATINGDISPLAY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_StartUpdatingDisplay_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_StartUpdatingDisplay_t *)arg, sizeof(VID_Ioctl_StartUpdatingDisplay_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_StartUpdatingDisplay_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_StartUpdatingDisplay(((VID_Ioctl_StartUpdatingDisplay_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_StartUpdatingDisplay_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_StartUpdatingDisplay_t))) != 0)
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

        case STVID_IOC_STEP:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Step_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Step_t *)arg, sizeof(VID_Ioctl_Step_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Step_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Step(((VID_Ioctl_Step_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_Step_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Step_t))) != 0)
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

        case STVID_IOC_STOP:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_Stop_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_Stop_t *)arg, sizeof(VID_Ioctl_Stop_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_Stop_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_Stop(((VID_Ioctl_Stop_t *)Ioctl_Param_p)->Handle,
                                               ((VID_Ioctl_Stop_t *)Ioctl_Param_p)->StopMode,
                                            &( ((VID_Ioctl_Stop_t *)Ioctl_Param_p)->Freeze ));

                if ((ret = copy_to_user((VID_Ioctl_Stop_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_Stop_t))) != 0)
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

#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
        case STVID_IOC_GETSYNCHRONIZATIONDELAY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetSynchronizationDelay_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetSynchronizationDelay_t *)arg, sizeof(VID_Ioctl_GetSynchronizationDelay_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetSynchronizationDelay_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetSynchronizationDelay(((VID_Ioctl_GetSynchronizationDelay_t *)Ioctl_Param_p)->VideoHandle,
                                            &( ((VID_Ioctl_GetSynchronizationDelay_t *)Ioctl_Param_p)->SyncDelay ));

                if ((ret = copy_to_user((VID_Ioctl_GetSynchronizationDelay_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetSynchronizationDelay_t))) != 0)
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

        case STVID_IOC_SETSYNCHRONIZATIONDELAY:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetSynchronizationDelay_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetSynchronizationDelay_t *)arg, sizeof(VID_Ioctl_SetSynchronizationDelay_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetSynchronizationDelay_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetSynchronizationDelay(((VID_Ioctl_SetSynchronizationDelay_t *)Ioctl_Param_p)->VideoHandle,
                                                          ((VID_Ioctl_SetSynchronizationDelay_t *)Ioctl_Param_p)->SyncDelay );

                if ((ret = copy_to_user((VID_Ioctl_SetSynchronizationDelay_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetSynchronizationDelay_t))) != 0)
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
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */

        case STVID_IOC_GETPICTUREBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetPictureBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetPictureBuffer_t *)arg, sizeof(VID_Ioctl_GetPictureBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetPictureBuffer_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetPictureBuffer(((VID_Ioctl_GetPictureBuffer_t *)Ioctl_Param_p)->Handle,
                                            &( ((VID_Ioctl_GetPictureBuffer_t *)Ioctl_Param_p)->Params),
                                            &( ((VID_Ioctl_GetPictureBuffer_t *)Ioctl_Param_p)->PictureBufferParams ),
                                            &( ((VID_Ioctl_GetPictureBuffer_t *)Ioctl_Param_p)->PictureBufferHandle ));

                if ((ret = copy_to_user((VID_Ioctl_GetPictureBuffer_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetPictureBuffer_t))) != 0)
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

        case STVID_IOC_RELEASEPICTUREBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_ReleasePictureBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_ReleasePictureBuffer_t *)arg, sizeof(VID_Ioctl_ReleasePictureBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_ReleasePictureBuffer_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_ReleasePictureBuffer(((VID_Ioctl_ReleasePictureBuffer_t *)Ioctl_Param_p)->Handle,
                                            ((VID_Ioctl_ReleasePictureBuffer_t *)Ioctl_Param_p)->PictureBufferHandle );

                if ((ret = copy_to_user((VID_Ioctl_ReleasePictureBuffer_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_ReleasePictureBuffer_t))) != 0)
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

        case STVID_IOC_DISPLAYPICTUREBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_DisplayPictureBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_DisplayPictureBuffer_t *)arg, sizeof(VID_Ioctl_DisplayPictureBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_DisplayPictureBuffer_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_DisplayPictureBuffer(((VID_Ioctl_DisplayPictureBuffer_t *)Ioctl_Param_p)->Handle,
                                            ((VID_Ioctl_DisplayPictureBuffer_t *)Ioctl_Param_p)->PictureBufferHandle ,
                                            &( ((VID_Ioctl_DisplayPictureBuffer_t *)Ioctl_Param_p)->PictureInfos ));

                if ((ret = copy_to_user((VID_Ioctl_DisplayPictureBuffer_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_DisplayPictureBuffer_t))) != 0)
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

        case STVID_IOC_TAKEPICTUREBUFFER:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_TakePictureBuffer_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_TakePictureBuffer_t *)arg, sizeof(VID_Ioctl_TakePictureBuffer_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_TakePictureBuffer_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_TakePictureBuffer(((VID_Ioctl_TakePictureBuffer_t *)Ioctl_Param_p)->Handle,
                                            ((VID_Ioctl_TakePictureBuffer_t *)Ioctl_Param_p)->PictureBufferHandle);

                if ((ret = copy_to_user((VID_Ioctl_TakePictureBuffer_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_TakePictureBuffer_t))) != 0)
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

#ifdef STUB_INJECT
        case STVID_IOC_VID_STREAMSIZE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_StreamSize_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_StreamSize_t *)arg, sizeof(VID_Ioctl_StreamSize_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                StubInject_SetStreamSize( ((VID_Ioctl_StreamSize_t *)Ioctl_Param_p)->Size );
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVID_IOC_STUBINJ_GETBBINFO:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_StubInjectGetBBInfo_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_StubInjectGetBBInfo_t *)arg, sizeof(VID_Ioctl_StubInjectGetBBInfo_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                StubInject_GetBBInfo(&(((VID_Ioctl_StubInjectGetBBInfo_t *)Ioctl_Param_p)->BaseAddress_p),
                                     &(((VID_Ioctl_StubInjectGetBBInfo_t *)Ioctl_Param_p)->Size));

                if ((ret = copy_to_user((VID_Ioctl_StubInjectGetBBInfo_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_StubInjectGetBBInfo_t))) != 0)
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
#endif

#ifdef ST_XVP_ENABLE_FLEXVP
    case STVID_IOC_ACTIXVPPROC:
            Ioctl_Param_p = STOS_MemoryAllocate(NULL, sizeof(VID_Ioctl_XVP_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_XVP_t *)arg, sizeof(VID_Ioctl_XVP_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
                ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ErrorCode =
                    STVID_ActivateXVPProcess(   ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ViewPortHandle,
                                                ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ProcessID);

                if ((ret = copy_to_user(    (VID_Ioctl_XVP_t *)arg,
                                            Ioctl_Param_p,
                                            sizeof(VID_Ioctl_XVP_t))) != 0)
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

    case STVID_IOC_DEACXVPPROC:
            Ioctl_Param_p = STOS_MemoryAllocate(NULL, sizeof(VID_Ioctl_XVP_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_XVP_t *)arg, sizeof(VID_Ioctl_XVP_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
                ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ErrorCode =
                    STVID_DeactivateXVPProcess(     ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ViewPortHandle,
                                                    ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ProcessID);

                if ((ret = copy_to_user(    (VID_Ioctl_XVP_t *)arg,
                                            Ioctl_Param_p,
                                            sizeof(VID_Ioctl_XVP_t))) != 0)
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

    case STVID_IOC_SHOWXVPPROC:
            Ioctl_Param_p = STOS_MemoryAllocate(NULL, sizeof(VID_Ioctl_XVP_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_XVP_t *)arg, sizeof(VID_Ioctl_XVP_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }
                ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ErrorCode =
                    STVID_ShowXVPProcess(   ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ViewPortHandle,
                                            ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ProcessID);

                if ((ret = copy_to_user(    (VID_Ioctl_XVP_t *)arg,
                                            Ioctl_Param_p,
                                            sizeof(VID_Ioctl_XVP_t))) != 0)
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

    case STVID_IOC_HIDEXVPPROC:
            Ioctl_Param_p = STOS_MemoryAllocate(NULL, sizeof(VID_Ioctl_XVP_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_XVP_t *)arg, sizeof(VID_Ioctl_XVP_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ErrorCode =
                    STVID_HideXVPProcess(   ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ViewPortHandle,
                                            ((VID_Ioctl_XVP_t *)Ioctl_Param_p)->ProcessID);

                if ((ret = copy_to_user(    (VID_Ioctl_XVP_t *)arg,
                                            Ioctl_Param_p,
                                            sizeof(VID_Ioctl_XVP_t))) != 0)
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
#endif /* ST_XVP_ENABLE_FLEXVP */

        case STVID_IOC_GETVIEWPORTQUALITYOPTIMIZATIONS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetViewPortQualityOptimizations_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetViewPortQualityOptimizations_t *)arg, sizeof(VID_Ioctl_GetViewPortQualityOptimizations_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetViewPortQualityOptimizations_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetViewPortQualityOptimizations(((VID_Ioctl_GetViewPortQualityOptimizations_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetViewPortQualityOptimizations_t *)Ioctl_Param_p)->Optimizations ));

                if ((ret = copy_to_user((VID_Ioctl_GetViewPortQualityOptimizations_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetViewPortQualityOptimizations_t))) != 0)
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

        case STVID_IOC_SETVIEWPORTQUALITYOPTIMIZATIONS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetViewPortQualityOptimizations_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetViewPortQualityOptimizations_t *)arg, sizeof(VID_Ioctl_SetViewPortQualityOptimizations_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetViewPortQualityOptimizations_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetViewPortQualityOptimizations( ((VID_Ioctl_SetViewPortQualityOptimizations_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &((VID_Ioctl_SetViewPortQualityOptimizations_t *)Ioctl_Param_p)->Optimizations );

                if ((ret = copy_to_user((VID_Ioctl_SetViewPortQualityOptimizations_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetViewPortQualityOptimizations_t))) != 0)
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

#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
        case STVID_IOC_GETREQUESTEDDEINTERLACINGMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_GetRequestedDeinterlacingMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_GetRequestedDeinterlacingMode_t *)arg, sizeof(VID_Ioctl_GetRequestedDeinterlacingMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_GetRequestedDeinterlacingMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_GetRequestedDeinterlacingMode(((VID_Ioctl_GetRequestedDeinterlacingMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            &( ((VID_Ioctl_GetRequestedDeinterlacingMode_t *)Ioctl_Param_p)->RequestedMode ));

                if ((ret = copy_to_user((VID_Ioctl_GetRequestedDeinterlacingMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_GetRequestedDeinterlacingMode_t))) != 0)
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

        case STVID_IOC_SETREQUESTEDDEINTERLACINGMODE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_SetRequestedDeinterlacingMode_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_SetRequestedDeinterlacingMode_t *)arg, sizeof(VID_Ioctl_SetRequestedDeinterlacingMode_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_SetRequestedDeinterlacingMode_t *)Ioctl_Param_p)->ErrorCode =
                            STVID_SetRequestedDeinterlacingMode( ((VID_Ioctl_SetRequestedDeinterlacingMode_t *)Ioctl_Param_p)->ViewPortHandle,
                                            ((VID_Ioctl_SetRequestedDeinterlacingMode_t *)Ioctl_Param_p)->RequestedMode );

                if ((ret = copy_to_user((VID_Ioctl_SetRequestedDeinterlacingMode_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_SetRequestedDeinterlacingMode_t))) != 0)
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
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

#ifdef STVID_USE_CRC
        case STVID_IOC_CRCSTARTQUEUEING:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CRCStartQueueing_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CRCStartQueueing_t *)arg, sizeof(VID_Ioctl_CRCStartQueueing_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CRCStartQueueing_t *)Ioctl_Param_p)->ErrorCode = STVID_CRCStartQueueing(((VID_Ioctl_CRCStartQueueing_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_CRCStartQueueing_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CRCStartQueueing_t))) != 0)
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

        case STVID_IOC_CRCSTOPQUEUEING:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CRCStopQueueing_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CRCStopQueueing_t *)arg, sizeof(VID_Ioctl_CRCStopQueueing_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CRCStopQueueing_t *)Ioctl_Param_p)->ErrorCode = STVID_CRCStopQueueing(((VID_Ioctl_CRCStopQueueing_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_CRCStopQueueing_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CRCStopQueueing_t))) != 0)
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

        case STVID_IOC_CRCGETQUEUE:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CRCGetQueue_t));
            if (Ioctl_Param_p != NULL)
            {
                STVID_CRCDataMessage_t *KernelMessagesArray=NULL;

                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CRCGetQueue_t *)arg, sizeof(VID_Ioctl_CRCGetQueue_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* allocate messages array in kernel space */
                KernelMessagesArray = (STVID_CRCDataMessage_t *)STOS_MemoryAllocate (NULL, sizeof(STVID_CRCDataMessage_t)*((VID_Ioctl_CRCGetQueue_t *)Ioctl_Param_p)->CRCReadMessages.NbToRead);
                if (KernelMessagesArray != NULL)
                {
                    ((VID_Ioctl_CRCGetQueue_t *)Ioctl_Param_p)->CRCReadMessages.Messages_p = KernelMessagesArray;

                     ((VID_Ioctl_CRCGetQueue_t *)Ioctl_Param_p)->ErrorCode = STVID_CRCGetQueue(((VID_Ioctl_CRCGetQueue_t *)Ioctl_Param_p)->Handle,
                                                            &(((VID_Ioctl_CRCGetQueue_t *)Ioctl_Param_p)->CRCReadMessages));

                    /* copy kernel messages array back to user user space array */
                    if ((ret = copy_to_user(((VID_Ioctl_CRCGetQueue_t *)arg)->CRCReadMessages.Messages_p, KernelMessagesArray,
                                                         sizeof(STVID_CRCDataMessage_t)*((VID_Ioctl_CRCGetQueue_t *)Ioctl_Param_p)->CRCReadMessages.NbReturned)) != 0)
                    {
                        /* Invalid user space address */
                        goto fail;
                    }

                    /* free kernel messages array */
                    if(KernelMessagesArray != NULL)
                    {
                        STOS_MemoryDeallocate(NULL, (void *)KernelMessagesArray);
                    }

                    if ((ret = copy_to_user((VID_Ioctl_CRCGetQueue_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CRCGetQueue_t))) != 0)
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
           }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;

        case STVID_IOC_CRCSTARTCHECK:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CRCStartCheck_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CRCStartCheck_t *)arg, sizeof(VID_Ioctl_CRCStartCheck_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                /* Allocate crc check arrays in kernel space and save user space array pointer to copy back data in it */
                /* at next STVID_IOC_CRCGETCURRENTRESULTS */
                /* These arrays will be freed at next STVID_IOC_TERM or re-allocated at next STVID_IOC_CRCSTARTCHECK*/
                if(KernelRefCRCTable != NULL)
                {
                    STOS_MemoryDeallocate(NULL, KernelRefCRCTable);
                    KernelRefCRCTable = NULL;
                }
                KernelRefCRCTable = (STVID_RefCRCEntry_t *)STOS_MemoryAllocate(NULL, sizeof(STVID_RefCRCEntry_t)*((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.NbPictureInRefCRC);
                /* need to copy crc ref table to kernel space */
                if (KernelRefCRCTable == NULL)
                {
                    ret = -ENOMEM;
                    goto fail;
                }

                if ((ret = copy_from_user(KernelRefCRCTable, ((VID_Ioctl_CRCStartCheck_t *)arg)->StartParams.RefCRCTable,
                                                                                       sizeof(STVID_RefCRCEntry_t)*((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.NbPictureInRefCRC)) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.RefCRCTable = KernelRefCRCTable;

                if(KernelCompCRCTable != NULL)
                {
                    STOS_MemoryDeallocate(NULL, KernelCompCRCTable);
                    KernelCompCRCTable = NULL;
                }
                CompCRCTableSize = sizeof(STVID_CompCRCEntry_t)*((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.MaxNbPictureInCompCRC*2;
                KernelCompCRCTable = (STVID_CompCRCEntry_t *)STOS_MemoryAllocate(NULL, CompCRCTableSize);
                UserCompCRCTable = ((VID_Ioctl_CRCStartCheck_t *)arg)->StartParams.CompCRCTable;

                /* Note that we need to copy user data to kernel even if values do not need to be initialized because otherwise we cannot */
                /* free user data with STOS_MemoryDeallocate in user space after kernel data has been copied back in it. */
                /* It seems that some hiddden field might be used in user mode data to check allocation validity before freeing it ... */
                if (KernelCompCRCTable == NULL)
                {
                    ret = -ENOMEM;
                    goto fail;
                }

                if ((ret = copy_from_user(KernelCompCRCTable, UserCompCRCTable,CompCRCTableSize) != 0))
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.CompCRCTable = KernelCompCRCTable;

                if(KernelErrorLogTable != NULL)
                {
                    STOS_MemoryDeallocate(NULL, KernelErrorLogTable);
                    KernelErrorLogTable = NULL;
                }
                ErrorLogTableSize = sizeof(STVID_CRCErrorLogEntry_t)*((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.MaxNbPictureInErrorLog;
                KernelErrorLogTable = (STVID_CRCErrorLogEntry_t *)STOS_MemoryAllocate (NULL, ErrorLogTableSize);
                UserErrorLogTable = ((VID_Ioctl_CRCStartCheck_t *)arg)->StartParams.ErrorLogTable;
                if (KernelErrorLogTable == NULL)
                {
                    ret = -ENOMEM;
                    goto fail;
                }
                if ((ret = copy_from_user(KernelErrorLogTable, UserErrorLogTable,ErrorLogTableSize) != 0))
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams.ErrorLogTable = KernelErrorLogTable;

                ((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->ErrorCode = STVID_CRCStartCheck(((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->Handle,
                                                        &(((VID_Ioctl_CRCStartCheck_t *)Ioctl_Param_p)->StartParams));

                if ((ret = copy_to_user((VID_Ioctl_CRCStartCheck_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CRCStartCheck_t))) != 0)
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

        case STVID_IOC_CRCSTOPCHECK:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CRCStopCheck_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CRCStopCheck_t *)arg, sizeof(VID_Ioctl_CRCStopCheck_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                 ((VID_Ioctl_CRCStopCheck_t *)Ioctl_Param_p)->ErrorCode = STVID_CRCStopCheck(((VID_Ioctl_CRCStopCheck_t *)Ioctl_Param_p)->Handle);

                if ((ret = copy_to_user((VID_Ioctl_CRCStopCheck_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CRCStopCheck_t))) != 0)
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

        case STVID_IOC_CRCGETCURRENTRESULTS:
            Ioctl_Param_p = STOS_MemoryAllocate (NULL, sizeof(VID_Ioctl_CRCGetCurrentResults_t));
            if (Ioctl_Param_p != NULL)
            {
                if ((ret = copy_from_user(Ioctl_Param_p, (VID_Ioctl_CRCGetCurrentResults_t *)arg, sizeof(VID_Ioctl_CRCGetCurrentResults_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

                ((VID_Ioctl_CRCGetCurrentResults_t *)Ioctl_Param_p)->ErrorCode = STVID_CRCGetCurrentResults(((VID_Ioctl_CRCGetCurrentResults_t *)Ioctl_Param_p)->Handle,
                                                        &(((VID_Ioctl_CRCGetCurrentResults_t *)Ioctl_Param_p)->CRCCheckResult));

                if ((ret = copy_to_user((VID_Ioctl_CRCGetCurrentResults_t *)arg, Ioctl_Param_p, sizeof(VID_Ioctl_CRCGetCurrentResults_t))) != 0)
                {
                    /* Invalid user space address */
                    goto fail;
                }

               /* We need to copy back kernel crc check arrays to user mode crc arrays since they are desynchronized due to user/kernel space separation */
               /* Do not return error if the pointers are NULL because it maybe a call to GetCurrentResults before CRC check was started */
               if( UserCompCRCTable != NULL && KernelCompCRCTable != NULL)
               {
                    if ((ret = copy_to_user(UserCompCRCTable, KernelCompCRCTable, CompCRCTableSize)) != 0)
                    {
                        /* Invalid user space address */
                        goto fail;
                    }
               }

               if( UserErrorLogTable != NULL && KernelErrorLogTable != NULL)
               {
                    if ((ret = copy_to_user(UserErrorLogTable, KernelErrorLogTable, ErrorLogTableSize)) != 0)
                    {
                        /* Invalid user space address */
                        goto fail;
                    }
               }
            }
            else
            {
                ret = -ENOMEM;
                goto fail;
            }
            break;
#endif

            case STVID_IOC_AWAIT_CALLBACK:
            {
                if ((ret = copy_from_user(&CallbackData, (STVID_Ioctl_CallbackData_t *)arg, sizeof(STVID_Ioctl_CallbackData_t)) ))
                {
                    /* Invalid user space address */
                    goto fail;                 
                }

                /* Wait for the callback to complete */
                if (down_interruptible(&callback_call_sem)!=0) 
                {
                    return -ERESTARTSYS;
                }

                if((ret = copy_to_user((STVID_Ioctl_CallbackData_t*)arg, &CallbackData, sizeof(STVID_Ioctl_CallbackData_t))))
                {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

            case STVID_IOC_RELEASE_CALLBACK:
            {
                if ((ret = copy_from_user(&CallbackData, (STVID_Ioctl_CallbackData_t *)arg, sizeof(STVID_Ioctl_CallbackData_t)) ))
                {
                    /* Invalid user space address */
                    goto fail; 
                }    
                if (CallbackData.Terminate)
                {
                    /* Release callback in a task termination purpose */
                    /* signal call back semaphore as if callback execution is complete */
                    up(&callback_call_sem);
                }
                else
                {
                    up(&callback_return_sem);
                }
            }
            break;

        default :
            /*** Inappropriate ioctl for the device ***/
            ret = -ENOTTY;
            goto fail;
    }

    if (Ioctl_Param_p != NULL)
    {
        STOS_MemoryDeallocate (NULL, Ioctl_Param_p);
    }
    return (0);

    /**************************************************************************/

    /*** Clean up on error ***/

fail:
    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID: ioctl command %d [0x%.8x] failed (0x%.8x)!!!", _IOC_NR(cmd), cmd, (U32)ret));
    if (Ioctl_Param_p != NULL)
    {
        STOS_MemoryDeallocate (NULL, Ioctl_Param_p);
    }
    return (ret);
}

/* End of stvid_ioctl_ioctl.c */
