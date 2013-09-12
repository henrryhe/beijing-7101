/*****************************************************************************
 *
 * Module : staudlx_ioctl
 * Date : 11-07-2005
 * Author : TAYLORD
 * Description :
 * Copyright : STMicroelectronics (c)2005
 *****************************************************************************/

/* Requires MODULE defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__ defined on the command line for SMP systems */
//#define STTBX_Print

#define _NO_VERSION_
#include <linux/module.h>			/* Module support */
#include <linux/version.h>			/* Kernel version */
#include <linux/fs.h>				/* File operations (fops) defines */
#include <linux/ioport.h>			/* Memory/device locking macros */
#include <linux/errno.h>			/* Defines standard error codes */
#include <asm/uaccess.h>			/* User space access routines */
#include <linux/sched.h>			/* User privilages (capabilities) */
#include "staudlx_ioctl_types.h"	/* Module specific types */
#include "stos.h"
#include "staudlx_ioctl.h"			/* Defines ioctl numbers */
#include "staudlx.h"				/* A STAPI ioctl driver. */
#include "stpti.h"

/* Conditional compilation (#if's) etc taken from aud_wrap.c */

#ifndef STB
#define STB
#endif


/*** PROTOTYPES **************************************************************/


#include "staudlx_ioctl_fops.h"

static STPTI_Buffer_t PTI_AudioBufferHandle;
static STAUD_Ioctl_FPCallbackData_t tempData;
static DECLARE_MUTEX_LOCKED(sema_return);
static DECLARE_MUTEX_LOCKED(sema_call);
static STAUD_Ioctl_CallbackData_t CallbackData;
static DECLARE_MUTEX_LOCKED(callback_call_sem);
static DECLARE_MUTEX_LOCKED(callback_return_sem);
static DECLARE_MUTEX(callback_mutex);
static struct	task_struct *callback_thread	=	NULL;
static int		callback_thread_count			=	0;
static int		fpcallback_thread_count			=	0;
typedef struct
{
    BOOL Used;
    U32 AudioHandle;
    U32 BufferHandle;
	U32 *BaseUnCachedVirtualAddress;
	U32 *BasePhyAddress;
	U32 *BaseCachedVirtualAddress;
} STAud_IOclt_Memory_Address_t;


static  STAud_IOclt_Memory_Address_t  PESBuferInstanceCount[MAX_AUDIO_INSTANCE];


/*** METHODS *****************************************************************/
static ST_ErrorCode_t AUD_GetFreePESBuffer(U8 *FreeBuffer)
{
    U8 Count =0;
    for(Count=0;Count< MAX_AUDIO_INSTANCE;Count++)
    {
        if(PESBuferInstanceCount [Count].Used == FALSE)
        {
            *FreeBuffer = Count;
            STTBX_Print((" AUD_GetFreePESBuffer Count =%d ",Count));
            return ST_NO_ERROR;
        }
    }
    STTBX_Print(( "AUD_GetFreePESBuffer Error "));
    return ST_ERROR_NO_FREE_HANDLES;

}

static ST_ErrorCode_t AUD_GetPESBufferFromAudioHandle(U8 *PESBuffer, U32 AudioHandle)
{
    U8 Count =0;
    for(Count=0;Count< MAX_AUDIO_INSTANCE;Count++)
    {
        if(PESBuferInstanceCount[Count].AudioHandle == AudioHandle)
        {
            *PESBuffer = Count;
            STTBX_Print(( "AUD_GetPESBufferFromAudioHandle Count =%d ",Count));
            return ST_NO_ERROR;
        }
    }
      STTBX_Print(( "AUD_GetPESBufferFromAudioHandle Error "));
    return ST_ERROR_INVALID_HANDLE;
}

static ST_ErrorCode_t AUD_GetPESBufferFromBufferHandle(U8 *PESBuffer, U32 BufferHandle)
{
    U8 Count =0;
    for(Count=0;Count< MAX_AUDIO_INSTANCE;Count++)
    {
        if(PESBuferInstanceCount[Count].BufferHandle == BufferHandle)
        {
            *PESBuffer = Count;
           STTBX_Print(( " AUD_GetPESBufferFromBufferHandle Count =%d ",Count));
            return ST_NO_ERROR;
        }
    }
    STTBX_Print(("AUD_GetPESBufferFromBufferHandle Error "));
    return ST_ERROR_INVALID_HANDLE;
}

/*-------------------------------------------------------------------------
 * Function: AUD_GetPTIPESBuffWritePtr
 * Decription: Gets the PES Buffer Write Pointer from PTI
 * Input : PTI Buffer Handle
 * Output :
 * Return : Error code and the Audio Input Buffer Address
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t AUD_GetPTIPESBuffWritePtr(
		void * const Handle_p,
		void ** const Address_p)
{
	ST_ErrorCode_t Err=ST_NO_ERROR;
	void *PES_BufferAddress;
	U8 PESBuffer;

	#ifndef STAUD_REMOVE_PTI_SUPPORT
		/* just cast the handle */
		Err = STPTI_BufferGetWritePointer(/*PTI_AudioBufferHandle*/(STPTI_Buffer_t)Handle_p,&PES_BufferAddress);
		Err =  AUD_GetPESBufferFromBufferHandle(&PESBuffer, Handle_p);
		if ( Err != ST_NO_ERROR )
		{
			printk(KERN_ALERT"STPTI_BufferGetWritePointer() Failed, Error=%d\n", Err);
 	    }
 	else
		{
			//*Address_p= (void*)((U32)(phys_to_virt((U32)PES_BufferAddress)) | 0xA0000000);
			*Address_p=STOS_AddressTranslate(PESBuferInstanceCount[PESBuffer].BaseUnCachedVirtualAddress,PESBuferInstanceCount[PESBuffer].BasePhyAddress,
			                            PES_BufferAddress);
			//printk(KERN_INFO" WriteAddress(0x%x)\n", PES_BufferAddress);
		}
	#endif
	return(Err);

}

/*-------------------------------------------------------------------------
 * Function: AUD_SetPTIPESBuffReadPtrFct
 * Decription: Sets the PTI Read Pointer.
 * Input : PTI Buffer Handle and the Audio Buffer Address
 * Output :
 * Return : Error code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t
AUD_SetPTIPESBuffReadPtrFct(
		void * const Handle_p,
		void * const Address_p)
{
	ST_ErrorCode_t Err = ST_NO_ERROR;
	void * PhyAddress;
	U8 PESBuffer;
	Err = AUD_GetPESBufferFromBufferHandle(&PESBuffer, (U32)Handle_p);
    if(!Err)
    	PhyAddress = (void*)STOS_AddressTranslate(PESBuferInstanceCount[PESBuffer].BasePhyAddress,
	                                             PESBuferInstanceCount[PESBuffer].BaseUnCachedVirtualAddress,
	                                             Address_p);

	#ifndef STAUD_REMOVE_PTI_SUPPORT
 	/* just cast the handle */
		Err = STPTI_BufferSetReadPointer(/*PTI_AudioBufferHandle*/(STPTI_Buffer_t)Handle_p,PhyAddress);
		if ( Err != ST_NO_ERROR )
		{
			printk(KERN_ALERT"PTI_BufferSetReadPointer() Failed, Error=%d\n", Err);
		}
		else
		{
			//printk(KERN_INFO"ReadAddress(0x%x)\n", Address_p);
		}
	#endif
		return Err;
}

/*-------------------------------------------------------------------------
 * Function: AUD_GetUserPESBuffWritePtr
 * Decription: Gets the PES Buffer Write Pointer from PTI
 * Input : PTI Buffer Handle
 * Output :
 * Return : Error code and the Audio Input Buffer Address
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t AUD_GetUserPESBuffWritePtr(
		void * const Handle_p,
		void ** const Address_p)
{
	ST_ErrorCode_t Err = ST_NO_ERROR;
	//int callback_policy = SCHED_NORMAL;
	//int callback_priority = 0;

	down(&callback_mutex);

	/* Set parameters for callback */
	CallbackData.ErrorCode = ST_NO_ERROR;
	CallbackData.CBType = STAUD_CB_WRITE_PTR;
	CallbackData.Handle = Handle_p;
	CallbackData.Address = NULL;


	#if 0
		/* Set callback thread priority to match this so as to immediately switch to it */
		if (callback_thread)
		{
			struct sched_param sparam = {current->rt_priority};
			callback_policy = callback_thread->policy;
			callback_priority = callback_thread->prio;
			STLINUX_sched_setscheduler(callback_thread->pid, current->policy, &sparam);
		}
	#endif
	/* Trigger the callback to run */
	up(&callback_call_sem);

	/* Wait for the callback to complete */
	if (!down_interruptible(&callback_return_sem))
	//if (wait_event_interruptible_timeout(callback_return_sem.wait, !down_trylock(&callback_return_sem), HZ/10))
	{
		if (CallbackData.Address == NULL)
		{
			if (CallbackData.ErrorCode == ST_NO_ERROR)
			{
				printk(KERN_ALERT"AUD_GetUserPESBuffWritePtr() NULL pointer returned\n");
				Err = ST_ERROR_BAD_PARAMETER;
			}
		}
		else
		{
//			*Address_p= (void*)((U32)(phys_to_virt((U32)CallbackData.Address)) | 0xA0000000);
            U8 PESBuffer;
            Err =  AUD_GetPESBufferFromBufferHandle(&PESBuffer, (U32)Handle_p);
            if(!Err)
    			*Address_p=STOS_AddressTranslate(PESBuferInstanceCount[PESBuffer].BaseUnCachedVirtualAddress,PESBuferInstanceCount[PESBuffer].BasePhyAddress,
			                            CallbackData.Address);
			Err = CallbackData.ErrorCode;
		}
	}
	else
	{
		Err = ST_ERROR_TIMEOUT; /* Userland callback not called or never returned */
	}
	#if 0
		/* Restore callback thread priority */
		if (callback_thread)
		{
			struct sched_param sparam = {callback_priority};
			STLINUX_sched_setscheduler(callback_thread->pid, callback_policy, &sparam);
		}
	#endif
	up(&callback_mutex);

	return Err;
}

/*-------------------------------------------------------------------------
 * Function: AUD_SetUserPESBuffReadPtrFct
 * Decription: Sets the PTI Read Pointer.
 * Input : PTI Buffer Handle and the Audio Buffer Address
 * Output :
 * Return : Error code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t AUD_SetUserPESBuffReadPtrFct(
		void * const Handle_p,
		void * const Address_p)
{
	ST_ErrorCode_t Err		=	ST_NO_ERROR;
//	int callback_policy 	=	SCHED_NORMAL;
//	int callback_priority	=	0;
	U8 PESBuffer;

	down(&callback_mutex);

	/* Set parameters for callback */
	CallbackData.ErrorCode	=	ST_NO_ERROR;
	CallbackData.CBType		=	STAUD_CB_READ_PTR;
	CallbackData.Handle		=	Handle_p;
	//CallbackData.Address	=	(void*)(virt_to_phys(Address_p));
	Err = AUD_GetPESBufferFromBufferHandle(&PESBuffer,(U32) Handle_p);
    if(!Err)
	    CallbackData.Address = (void*)STOS_AddressTranslate(PESBuferInstanceCount[PESBuffer].BasePhyAddress,
	                                             PESBuferInstanceCount[PESBuffer].BaseUnCachedVirtualAddress,
	                                             Address_p);
	#if 0
		/* Set callback thread priority to match this so as to immediately switch to it */
		if (callback_thread)
		{
			struct sched_param sparam = {current->prio};
			callback_policy = callback_thread->policy;
			callback_priority = callback_thread->prio;
			STLINUX_sched_setscheduler(callback_thread->pid, current->policy, &sparam);
		}
	#endif

	/* Trigger the callback to run */
	up(&callback_call_sem);

 /* Wait for the callback to complete */
	if (!down_interruptible(&callback_return_sem))
	//if (wait_event_interruptible_timeout(callback_return_sem.wait, !down_trylock(&callback_return_sem), HZ/10))
	{
		Err = CallbackData.ErrorCode;
	}
	else
	{
		Err = ST_ERROR_TIMEOUT; /* Userland callback not called or never returned */
	}
	#if 0
		/* Restore callback thread priority */
		if (callback_thread)
		{
			struct sched_param sparam = {callback_priority};
			STLINUX_sched_setscheduler(callback_thread->pid, callback_policy, &sparam);
		}
	#endif
	up(&callback_mutex);

	return Err;
}
/*=============================================================================

 STAUD_FPParamCopy

 This is the callback function which is registered on call to the
 STAUD_IOC_DPSETCALLBACK ioctl. The call to this callback is made by the
 DataProcessorTask.

 =============================================================================
 */

ST_ErrorCode_t	STAUD_FPParamCopy (
		U8 *MemoryStart,
		U32 Size,
		BufferMetaData_t bufferMetaData,
		void * clientInfo)
{
	STTBX_Print(("STAUD_FPParamCopy: release semaphore sema_return \n"));
	tempData.MemoryStart = MemoryStart;
	tempData.Size = Size;
	tempData.bufferMetaData = bufferMetaData;
	tempData.clientInfo = clientInfo;

	STTBX_Print(("STAUD_FPParamCopy: wait on semaphore sema_call \n"));

	up(&sema_return);

	/* Wait for call to STAUD_IOC_AWAIT_FPCALLBACK to copy this data from tempData */

	if(down_interruptible(&sema_call))
	{
 return (-EINTR);

 }

	return ST_NO_ERROR;
}

/*=============================================================================

 staudlx_ioctl_ioctl

 Handle any device specific calls.

 The commands constants are defined in 'staudlx_ioctl.h'.

 ===========================================================================*/
int staudlx_ioctl_ioctl (
		struct inode *node,
		struct file *filp,
		unsigned int cmd,
		unsigned long arg)
{
	int err = 0;
	//printk(KERN_ALERT "ioctl command [0X%08X]\n", cmd);
	/*** Check the user privalage ***/

	/* if (!capable (CAP_SYS_ADMIN)) */ /* See 'sys/sched.h' */
	/* { */
	/* return (-EPERM); */
	/* } */

	/*** Check the command is for this module ***/

	if (_IOC_TYPE(cmd) != STAUDLX_IOCTL_MAGIC_NUMBER)
	{
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

	case STAUD_IOC_INIT:
	{
		STAUD_Ioctl_Init_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Init_t *)arg, sizeof(STAUD_Ioctl_Init_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		/* Few of aud init params settings like AVMem handle etc, will be handled in core driver's STAUD_Init(0 Function */
		UserParams.ErrorCode = STAUD_Init(UserParams.DeviceName, &( UserParams.InitParams ));

		if((err = copy_to_user((STAUD_Ioctl_Init_t*)arg, &UserParams, sizeof(STAUD_Ioctl_Init_t)))) {
			/* Invalid user space address */
			goto fail;
		}

		/* Wait for the callback thread to start */
		if (!callback_thread_count)
		{
			if (down_interruptible(&callback_return_sem))
			//if (!wait_event_interruptible_timeout(callback_return_sem.wait, !down_trylock(&callback_return_sem), HZ/10))
			{
				err = ETIME; /* Userland callback thread not running */
				goto fail;
			}
		}

		if( UserParams.ErrorCode == ST_NO_ERROR)
		{
			callback_thread_count++;
		}
	}
		break;
	case STAUD_IOC_TERM:
	{
		STAUD_Ioctl_Term_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Term_t *)arg, sizeof(STAUD_Ioctl_Term_t)) )){
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_Term(UserParams.DeviceName, &( UserParams.TermParams ));

		if((err = copy_to_user((STAUD_Ioctl_Term_t*)arg, &UserParams, sizeof(STAUD_Ioctl_Term_t)))) {
			/* Invalid user space address */
			goto fail;
		}

		if(UserParams.ErrorCode == ST_NO_ERROR)
		{
			/* Release callback thread */
			if (--callback_thread_count == 0)
			{
				down(&callback_mutex);
				up(&callback_call_sem); /* cause callback thread to return */
				up(&callback_mutex);
			}
			if (fpcallback_thread_count)
			{
				up(&sema_return); /* cause fpthread to return*/
				fpcallback_thread_count--;
			}
		}
	}
		break;
	case STAUD_IOC_OPEN:
	{
		STAUD_Ioctl_Open_t UserParams;
		U8 FreeBuffer;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Open_t *)arg, sizeof(STAUD_Ioctl_Open_t)) )){
				/* Invalid user space address */
				goto fail;
		}
		UserParams.ErrorCode = STAUD_Open(UserParams.DeviceName, &( UserParams.OpenParams ), &(UserParams.Handle));


		  err |=AUD_GetFreePESBuffer(&FreeBuffer);
		  if(!err)
		  {
		    PESBuferInstanceCount[FreeBuffer].AudioHandle = UserParams.Handle;
		    PESBuferInstanceCount[FreeBuffer].Used = TRUE;

		  }

		if((err |= copy_to_user((STAUD_Ioctl_Open_t*)arg, &UserParams, sizeof(STAUD_Ioctl_Open_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_CLOSE:
	{
		STAUD_Ioctl_Close_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Close_t *)arg, sizeof(STAUD_Ioctl_Close_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_Close(UserParams.Handle);
		if(UserParams.ErrorCode == ST_NO_ERROR)
		{
		    U8 PESBuffer;
            err = AUD_GetPESBufferFromAudioHandle(&PESBuffer, UserParams.Handle);
            if(err==0)
            {
                PESBuferInstanceCount[PESBuffer].AudioHandle = 0 ;
                PESBuferInstanceCount[PESBuffer].BufferHandle = 0;
                PESBuferInstanceCount[PESBuffer].Used = FALSE;
            }


		}
		if((err = copy_to_user((STAUD_Ioctl_Close_t*)arg, &UserParams, sizeof(STAUD_Ioctl_Close_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

#ifndef STAUD_NO_WRAPPER_LAYER
#if defined (DVD) || defined (ST_7100)

	case STAUD_IOC_PLAY:
	{
		STAUD_Ioctl_Play_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Play_t *)arg, sizeof(STAUD_Ioctl_Play_t)) )){
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_Play(UserParams.Handle);
		if((err = copy_to_user((STAUD_Ioctl_Play_t*)arg, &UserParams, sizeof(STAUD_Ioctl_Play_t)))) {
			/* Invalid user space address */
			goto fail;
 }
	}
	break;
#endif
#endif
	case STAUD_IOC_DRCONNECTSOURCE:
	{
		STAUD_Ioctl_DRConnectSource_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRConnectSource_t *)arg, sizeof(STAUD_Ioctl_DRConnectSource_t)) )){
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_DRConnectSource(UserParams.Handle,UserParams.DecoderObject, UserParams.InputSource);
		if((err = copy_to_user((STAUD_Ioctl_DRConnectSource_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRConnectSource_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_PPDISABLEDOWNSAMPLING:
	{
		STAUD_Ioctl_PPDisableDownsampling_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPDisableDownsampling_t *)arg, sizeof(STAUD_Ioctl_PPDisableDownsampling_t)) )){
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_PPDisableDownsampling(UserParams.Handle,UserParams.OutputObject);
		if((err = copy_to_user((STAUD_Ioctl_PPDisableDownsampling_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PPDisableDownsampling_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_PPENABLEDOWNSAMPLING:
	{
		STAUD_Ioctl_PPEnableDownsampling_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPEnableDownsampling_t *)arg, sizeof(STAUD_Ioctl_PPEnableDownsampling_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_PPEnableDownsampling(UserParams.Handle,UserParams.OutputObject,UserParams.Value);

		if((err = copy_to_user((STAUD_Ioctl_PPEnableDownsampling_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PPEnableDownsampling_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_OPDISABLESYNCHRONIZATION:
	{
		STAUD_Ioctl_OPDisableSynchronization_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPDisableSynchronization_t *)arg, sizeof(STAUD_Ioctl_OPDisableSynchronization_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_OPDisableSynchronization(UserParams.Handle,UserParams.OutputObject);

		if((err = copy_to_user((STAUD_Ioctl_OPDisableSynchronization_t*)arg, &UserParams, sizeof(STAUD_Ioctl_OPDisableSynchronization_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_OPENABLESYNCHRONIZATION:
	{
		STAUD_Ioctl_OPEnableSynchronization_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPEnableSynchronization_t *)arg, sizeof(STAUD_Ioctl_OPEnableSynchronization_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_OPEnableSynchronization(UserParams.Handle,UserParams.OutputObject);

		if((err = copy_to_user((STAUD_Ioctl_OPEnableSynchronization_t*)arg, &UserParams, sizeof(STAUD_Ioctl_OPEnableSynchronization_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPGETBROADCASTPROFILE:
	{
		STAUD_Ioctl_IPGetBroadcastProfile_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetBroadcastProfile_t *)arg, sizeof(STAUD_Ioctl_IPGetBroadcastProfile_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetBroadcastProfile(UserParams.Handle,UserParams.InputObject,&(UserParams.BroadcastProfile));


		if((err = copy_to_user((STAUD_Ioctl_IPGetBroadcastProfile_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetBroadcastProfile_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRGETCAPABILITY:
	{
		STAUD_Ioctl_DRGetCapability_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetCapability_t *)arg, sizeof(STAUD_Ioctl_DRGetCapability_t)) )){
		/* Invalid user space address */
		goto fail;
		}

		UserParams.ErrorCode = STAUD_DRGetCapability(UserParams.DeviceName,UserParams.DecoderObject,&(UserParams.Capability));


		if((err = copy_to_user((STAUD_Ioctl_DRGetCapability_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetCapability_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRGETDYNAMICRANGECONTROL:
	{
		STAUD_Ioctl_DRGetDynamicRangeControl_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetDynamicRangeControl_t *)arg, sizeof(STAUD_Ioctl_DRGetDynamicRangeControl_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRGetDynamicRangeControl(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DynamicRange));


		if((err = copy_to_user((STAUD_Ioctl_DRGetDynamicRangeControl_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetDynamicRangeControl_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;
	case STAUD_IOC_IPGETSAMPLINGFREQUENCY:
	{
		STAUD_Ioctl_IPGetSamplingFrequency_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetSamplingFrequency_t *)arg, sizeof(STAUD_Ioctl_IPGetSamplingFrequency_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetSamplingFrequency(UserParams.Handle,UserParams.DecoderObject,&(UserParams.SamplingFrequency));


		if((err = copy_to_user((STAUD_Ioctl_IPGetSamplingFrequency_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetSamplingFrequency_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRGETSPEED:
	{
		STAUD_Ioctl_DRGetSpeed_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetSpeed_t *)arg, sizeof(STAUD_Ioctl_DRGetSpeed_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRGetSpeed(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Speed));


		if((err = copy_to_user((STAUD_Ioctl_DRGetSpeed_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetSpeed_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPGETSTREAMINFO:
	{
		STAUD_Ioctl_IPGetStreamInfo_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetStreamInfo_t *)arg, sizeof(STAUD_Ioctl_IPGetStreamInfo_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetStreamInfo(UserParams.Handle,UserParams.DecoderObject,&(UserParams.StreamInfo));


		if((err = copy_to_user((STAUD_Ioctl_IPGetStreamInfo_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetStreamInfo_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRGETSYNCOFFSET:
	{
		STAUD_Ioctl_DRGetSyncOffset_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetSyncOffset_t *)arg, sizeof(STAUD_Ioctl_DRGetSyncOffset_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRGetSyncOffset(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Offset));

		if((err = copy_to_user((STAUD_Ioctl_DRGetSyncOffset_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetSyncOffset_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRPAUSE:
	{
		STAUD_Ioctl_DRPause_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRPause_t *)arg, sizeof(STAUD_Ioctl_DRPause_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRPause(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Fade));


		if((err = copy_to_user((STAUD_Ioctl_DRPause_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRPause_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPPAUSESYNCHRO:
	{
		STAUD_Ioctl_IPPauseSynchro_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPPauseSynchro_t *)arg, sizeof(STAUD_Ioctl_IPPauseSynchro_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPPauseSynchro(UserParams.Handle,UserParams.InputObject,UserParams.Delay);


		if((err = copy_to_user((STAUD_Ioctl_IPPauseSynchro_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPPauseSynchro_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRPREPARE:
	{
		STAUD_Ioctl_DRPrepare_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRPrepare_t *)arg, sizeof(STAUD_Ioctl_DRPrepare_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRPrepare(UserParams.Handle,UserParams.DecoderObject,&(UserParams.StreamParams));


		if((err = copy_to_user((STAUD_Ioctl_DRPrepare_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRPrepare_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRRESUME:
	{
		STAUD_Ioctl_DRResume_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRResume_t *)arg, sizeof(STAUD_Ioctl_DRResume_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRResume(UserParams.Handle,UserParams.OutputObject);

		if((err = copy_to_user((STAUD_Ioctl_DRResume_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRResume_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

#ifndef STAUD_REMOVE_CLKRV_SUPPORT

	case STAUD_IOC_DRSETCLOCKRECOVERYSOURCE:
	{
		STAUD_Ioctl_DRSetClockRecoverySource_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetClockRecoverySource_t *)arg, sizeof(STAUD_Ioctl_DRSetClockRecoverySource_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRSetClockRecoverySource(UserParams.Handle,UserParams.Object,UserParams.ClkSource);


		if((err = copy_to_user((STAUD_Ioctl_DRSetClockRecoverySource_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetClockRecoverySource_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

#endif

	case STAUD_IOC_DRSETDYNAMICRANGECONTROL:
	{
		STAUD_Ioctl_DRSetDynamicRangeControl_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetDynamicRangeControl_t *)arg, sizeof(STAUD_Ioctl_DRSetDynamicRangeControl_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRSetDynamicRangeControl(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DynamicRange));

		if((err = copy_to_user((STAUD_Ioctl_DRSetDynamicRangeControl_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetDynamicRangeControl_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRSETSPEED:
	{
		STAUD_Ioctl_DRSetSpeed_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetSpeed_t *)arg, sizeof(STAUD_Ioctl_DRSetSpeed_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRSetSpeed(UserParams.Handle,UserParams.DecoderObject,UserParams.Speed);


		if((err = copy_to_user((STAUD_Ioctl_DRSetSpeed_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetSpeed_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;


	case STAUD_IOC_IPSETSTREAMID:
	{
		STAUD_Ioctl_IPSetStreamID_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetStreamID_t *)arg, sizeof(STAUD_Ioctl_IPSetStreamID_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPSetStreamID(UserParams.Handle,UserParams.DecoderObject,UserParams.StreamID);

		if((err = copy_to_user((STAUD_Ioctl_IPSetStreamID_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetStreamID_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPSKIPSYNCHRO:
	{
		STAUD_Ioctl_IPSkipSynchro_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSkipSynchro_t *)arg, sizeof(STAUD_Ioctl_IPSkipSynchro_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPSkipSynchro(UserParams.Handle,UserParams.InputObject,UserParams.Delay);

		if((err = copy_to_user((STAUD_Ioctl_IPSkipSynchro_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPSkipSynchro_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRSETSYNCOFFSET:
	{
		STAUD_Ioctl_DRSetSyncOffset_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetSyncOffset_t *)arg, sizeof(STAUD_Ioctl_DRSetSyncOffset_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRSetSyncOffset(UserParams.Handle,UserParams.DecoderObject,UserParams.Offset);

		if((err = copy_to_user((STAUD_Ioctl_DRSetSyncOffset_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetSyncOffset_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRSTART:
	{
		STAUD_Ioctl_DRStart_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRStart_t *)arg, sizeof(STAUD_Ioctl_DRStart_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRStart(UserParams.Handle,UserParams.DecoderObject,&(UserParams.StreamParams));

		if((err = copy_to_user((STAUD_Ioctl_DRStart_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRStart_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRSTOP:
	{
		STAUD_Ioctl_DRStop_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRStop_t *)arg, sizeof(STAUD_Ioctl_DRStop_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRStop(UserParams.Handle,UserParams.DecoderObject,UserParams.StopMode,&(UserParams.Fade));

		if((err = copy_to_user((STAUD_Ioctl_DRStop_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRStop_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_GETCAPABILITY:
	{
		STAUD_Ioctl_GetCapability_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetCapability_t *)arg, sizeof(STAUD_Ioctl_GetCapability_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_GetCapability(UserParams.DeviceName,&(UserParams.Capability));


		if((err = copy_to_user((STAUD_Ioctl_GetCapability_t*)arg, &UserParams, sizeof(STAUD_Ioctl_GetCapability_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_GETREVISION:
	{
		const char* Revision = STAUD_GetRevision();
		if((err = copy_to_user((char*)arg, Revision, strlen(Revision)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_OPSETDIGITALMODE:
	{
		STAUD_Ioctl_OPSetDigitalMode_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPSetDigitalMode_t *)arg, sizeof(STAUD_Ioctl_OPSetDigitalMode_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_OPSetDigitalMode(UserParams.Handle,UserParams.OutputObject,UserParams.OutputMode);


		if((err = copy_to_user((STAUD_Ioctl_OPSetDigitalMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPSetDigitalMode_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_OPGETDIGITALMODE:
	{
		STAUD_Ioctl_OPGetDigitalMode_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPGetDigitalMode_t *)arg, sizeof(STAUD_Ioctl_OPGetDigitalMode_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_OPGetDigitalMode (UserParams.Handle,UserParams.OutputObject,&UserParams.OutputMode);

		if((err = copy_to_user((STAUD_Ioctl_OPGetDigitalMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPGetDigitalMode_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPGETCAPABILITY:
	{
		STAUD_Ioctl_IPGetCapability_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetCapability_t *)arg, sizeof(STAUD_Ioctl_IPGetCapability_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetCapability(UserParams.DeviceName,UserParams.InputObject,&(UserParams.IPCapability));


		if((err = copy_to_user((STAUD_Ioctl_IPGetCapability_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetCapability_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPGETDATAINTERFACEPARAMS:
	{
		STAUD_Ioctl_IPGetDataInterfaceParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetDataInterfaceParams_t *)arg, sizeof(STAUD_Ioctl_IPGetDataInterfaceParams_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetDataInterfaceParams(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DMAParams));


		if((err = copy_to_user((STAUD_Ioctl_IPGetDataInterfaceParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetDataInterfaceParams_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPGETPARAMS:
	{
		STAUD_Ioctl_IPGetParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetParams_t *)arg, sizeof(STAUD_Ioctl_IPGetParams_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetParams(UserParams.Handle,UserParams.InputObject,&(UserParams.InputParams));

		if((err = copy_to_user((STAUD_Ioctl_IPGetParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetParams_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPGETINPUTBUFFERPARAMS:
	{
		STAUD_Ioctl_IPGetInputBufferParams_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetInputBufferParams_t *)arg, sizeof(STAUD_Ioctl_IPGetInputBufferParams_t)) )){
		/* Invalid user space address */
		goto fail;
		}

		UserParams.ErrorCode = STAUD_IPGetInputBufferParams(UserParams.Handle,UserParams.InputObject,&(UserParams.DataParams));

        // Change the Address to Physical Here for Application //
        if(UserParams.ErrorCode == ST_NO_ERROR)
        {
          U8 PESBuffer;
          err = AUD_GetPESBufferFromAudioHandle(&PESBuffer, UserParams.Handle);
          if(err==0)
          {
              PESBuferInstanceCount[PESBuffer].BaseUnCachedVirtualAddress =UserParams.DataParams.BufferBaseAddr_p;
              PESBuferInstanceCount[PESBuffer].BaseCachedVirtualAddress = 0;

              UserParams.DataParams.BufferBaseAddr_p = (void*)STOS_VirtToPhys (UserParams.DataParams.BufferBaseAddr_p);
              PESBuferInstanceCount[PESBuffer].BasePhyAddress =UserParams.DataParams.BufferBaseAddr_p ;
           }
         }

		if((err = copy_to_user((STAUD_Ioctl_IPGetInputBufferParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetInputBufferParams_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_IPQUEUEPCMBUFFER:
	{
		STAUD_Ioctl_IPQueuePCMBuffer_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPQueuePCMBuffer_t *)arg, sizeof(STAUD_Ioctl_IPQueuePCMBuffer_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPQueuePCMBuffer(UserParams.Handle,UserParams.InputObject,&(UserParams.PCMBuffer),UserParams.NumBuffers,&(UserParams.NumQueued));


		if((err = copy_to_user((STAUD_Ioctl_IPQueuePCMBuffer_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPQueuePCMBuffer_t)))) {
		/* Invalid user space address */
		goto fail;
		}
	}
	break;

	case STAUD_IOC_IPSETLOWDATALEVELEVENT:
	{
		STAUD_Ioctl_IPSetLowDataLevelEvent_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetLowDataLevelEvent_t *)arg, sizeof(STAUD_Ioctl_IPSetLowDataLevelEvent_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPSetLowDataLevelEvent(UserParams.Handle,UserParams.InputObject,UserParams.Level);

		if((err = copy_to_user((STAUD_Ioctl_IPSetLowDataLevelEvent_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetLowDataLevelEvent_t)))) {
			/* Invalid user space address */
			goto fail;
		}
 	}
	break;

	case STAUD_IOC_IPSETPARAMS:
	{
		STAUD_Ioctl_IPSetParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetParams_t *)arg, sizeof(STAUD_Ioctl_IPSetParams_t)) )){
			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPSetParams(UserParams.Handle,UserParams.InputObject,&(UserParams.InputParams));

		if((err = copy_to_user((STAUD_Ioctl_IPSetParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_MXCONNECTSOURCE:
	{
		STAUD_Ioctl_MXConnectSource_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_MXConnectSource_t *)arg, sizeof(STAUD_Ioctl_MXConnectSource_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_MXConnectSource(UserParams.Handle,UserParams.MixerObject,(STAUD_Object_t *)&(UserParams.InputSources),(STAUD_MixerInputParams_t*)&(UserParams.InputParams),UserParams.NumInputs);

		if((err = copy_to_user((STAUD_Ioctl_MXConnectSource_t *)arg, &UserParams, sizeof(STAUD_Ioctl_MXConnectSource_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_MXGETCAPABILITY:
	{
		STAUD_Ioctl_MXGetCapability_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_MXGetCapability_t *)arg, sizeof(STAUD_Ioctl_MXGetCapability_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_MXGetCapability(UserParams.DeviceName,UserParams.MixerObject,&(UserParams.MXCapability));


		if((err = copy_to_user((STAUD_Ioctl_MXGetCapability_t *)arg, &UserParams, sizeof(STAUD_Ioctl_MXGetCapability_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_MXSETINPUTPARAMS:
	{
		STAUD_Ioctl_MXSetInputParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_MXSetInputParams_t *)arg, sizeof(STAUD_Ioctl_MXSetInputParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_MXSetInputParams(UserParams.Handle,UserParams.MixerObject,UserParams.InputSource,&(UserParams.MXMixerInputParams));


 		if((err = copy_to_user((STAUD_Ioctl_MXSetInputParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_MXSetInputParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPGETATTENUATION:
 	{
 		STAUD_Ioctl_PPGetAttenuation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetAttenuation_t *)arg, sizeof(STAUD_Ioctl_PPGetAttenuation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetAttenuation(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Attenuation));


 		if((err = copy_to_user((STAUD_Ioctl_PPGetAttenuation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetAttenuation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_OPGETCAPABILITY:
 	{
 		STAUD_Ioctl_OPGetCapability_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPGetCapability_t *)arg, sizeof(STAUD_Ioctl_OPGetCapability_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_OPGetCapability(UserParams.DeviceName,UserParams.OutputObject,&(UserParams.OPCapability));

 		if((err = copy_to_user((STAUD_Ioctl_OPGetCapability_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPGetCapability_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPSETSPEAKERENABLE:
 	{
 		STAUD_Ioctl_PPSetSpeakerEnable_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetSpeakerEnable_t *)arg, sizeof(STAUD_Ioctl_PPSetSpeakerEnable_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetSpeakerEnable(UserParams.Handle,UserParams.PostProcObject,&(UserParams.SpeakerEnable));


 		if((err = copy_to_user((STAUD_Ioctl_PPSetSpeakerEnable_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetSpeakerEnable_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPGETSPEAKERENABLE:
 	{
 		STAUD_Ioctl_PPGetSpeakerEnable_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetSpeakerEnable_t *)arg, sizeof(STAUD_Ioctl_PPGetSpeakerEnable_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetSpeakerEnable(UserParams.Handle,UserParams.PostProcObject,&(UserParams.SpeakerEnable));


 		if((err = copy_to_user((STAUD_Ioctl_PPGetSpeakerEnable_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetSpeakerEnable_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if 0
 	case STAUD_IOC_PPSETSTEREOMODENOW:
 	{
 		STAUD_Ioctl_PPSetStereoModeNow_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetStereoModeNow_t *)arg, sizeof(STAUD_Ioctl_PPSetStereoModeNow_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetStereoModeNow(UserParams.Handle,UserParams.DecoderObject,UserParams.StereoMode);

 		if((err = copy_to_user((STAUD_Ioctl_PPSetStereoModeNow_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetStereoModeNow_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

	case STAUD_IOC_PPSETATTENUATION:
 	{
 		STAUD_Ioctl_PPSetAttenuation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetAttenuation_t *)arg, sizeof(STAUD_Ioctl_PPSetAttenuation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetAttenuation(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Attenuation));


 		if((err = copy_to_user((STAUD_Ioctl_PPSetAttenuation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetAttenuation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_OPSETPARAMS:
 	{
 		STAUD_Ioctl_OPSetParams_t UserParams;
 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPSetParams_t *)arg, sizeof(STAUD_Ioctl_OPSetParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_OPSetParams(UserParams.Handle,UserParams.OutputObject,&(UserParams.Params));

 		if((err = copy_to_user((STAUD_Ioctl_OPSetParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPSetParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_OPGETPARAMS:
 	{
 		STAUD_Ioctl_OPGetParams_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPGetParams_t *)arg, sizeof(STAUD_Ioctl_OPGetParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_OPGetParams(UserParams.Handle,UserParams.OutputObject,&(UserParams.Params));


 		if((err = copy_to_user((STAUD_Ioctl_OPGetParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPGetParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPSETDELAY:
 	{
 		STAUD_Ioctl_PPSetDelay_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetDelay_t *)arg, sizeof(STAUD_Ioctl_PPSetDelay_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetDelay(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Delay));

 		if((err = copy_to_user((STAUD_Ioctl_PPSetDelay_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetDelay_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
	break;

 	case STAUD_IOC_PPGETDELAY:
 	{
 		STAUD_Ioctl_PPGetDelay_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetDelay_t *)arg, sizeof(STAUD_Ioctl_PPGetDelay_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetDelay(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Delay));

 		if((err = copy_to_user((STAUD_Ioctl_PPGetDelay_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetDelay_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if 0
 	case STAUD_IOC_PPGETEFFECT:
 	{
 		STAUD_Ioctl_PPGetEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetEffect_t *)arg, sizeof(STAUD_Ioctl_PPGetEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetEffect(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Effect));


 		if((err = copy_to_user((STAUD_Ioctl_PPGetEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetEffect_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPSETEFFECT:
 	{
	 	STAUD_Ioctl_PPSetEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetEffect_t *)arg, sizeof(STAUD_Ioctl_PPSetEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetEffect(UserParams.Handle,UserParams.PostProcObject,UserParams.Effect);


 		if((err = copy_to_user((STAUD_Ioctl_PPSetEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetEffect_t)))) {
 		/* Invalid user space address */
 		goto fail;
 		}
 	}
 	break;
#endif

	case STAUD_IOC_PPSETKARAOKE:
 	{
	 	STAUD_Ioctl_PPSetKaraoke_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetKaraoke_t *)arg, sizeof(STAUD_Ioctl_PPSetKaraoke_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetKaraoke(UserParams.Handle,UserParams.PostProcObject,UserParams.Karaoke);

 		if((err = copy_to_user((STAUD_Ioctl_PPSetKaraoke_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetKaraoke_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPGETKARAOKE:
 	{
 		STAUD_Ioctl_PPGetKaraoke_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetKaraoke_t *)arg, sizeof(STAUD_Ioctl_PPGetKaraoke_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetKaraoke(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Karaoke));

 		if((err = copy_to_user((STAUD_Ioctl_PPGetKaraoke_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetKaraoke_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPSETSPEAKERCONFIG:
 	{
 		STAUD_Ioctl_PPSetSpeakerConfig_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetSpeakerConfig_t *)arg, sizeof(STAUD_Ioctl_PPSetSpeakerConfig_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetSpeakerConfig(UserParams.Handle,UserParams.PostProcObject,&(UserParams.SpeakerConfig),UserParams.BassType);

 		if((err = copy_to_user((STAUD_Ioctl_PPSetSpeakerConfig_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetSpeakerConfig_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPGETSPEAKERCONFIG:
 	{
 		STAUD_Ioctl_PPGetSpeakerConfig_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetSpeakerConfig_t *)arg, sizeof(STAUD_Ioctl_PPGetSpeakerConfig_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetSpeakerConfig(UserParams.Handle,UserParams.PostProcObject,&(UserParams.SpeakerConfig));

 		if((err = copy_to_user((STAUD_Ioctl_PPGetSpeakerConfig_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetSpeakerConfig_t)))) {
 		/* Invalid user space address */
 		goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PPCONNECTSOURCE:
 	{
 		STAUD_Ioctl_PPConnectSource_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPConnectSource_t *)arg, sizeof(STAUD_Ioctl_PPConnectSource_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPConnectSource(UserParams.Handle,UserParams.PostProcObject,UserParams.InputSource);

 		if((err = copy_to_user((STAUD_Ioctl_PPConnectSource_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPConnectSource_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_OPMUTE:
 	{
 		STAUD_Ioctl_OPMute_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPMute_t *)arg, sizeof(STAUD_Ioctl_OPMute_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_OPMute(UserParams.Handle,UserParams.OutputObject);


 		if((err = copy_to_user((STAUD_Ioctl_OPMute_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPMute_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_OPUNMUTE:
 	{
 		STAUD_Ioctl_OPUnMute_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPUnMute_t *)arg, sizeof(STAUD_Ioctl_OPUnMute_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_OPUnMute(UserParams.Handle,UserParams.OutputObject);

		if((err = copy_to_user((STAUD_Ioctl_OPUnMute_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPUnMute_t)))) {
 			/* Invalid user space address */
 			goto fail;
		}
 	}
 	break;

 	case STAUD_IOC_PPGETCAPABILITY:
 	{
 		STAUD_Ioctl_PPGetCapability_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetCapability_t *)arg, sizeof(STAUD_Ioctl_PPGetCapability_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetCapability(UserParams.DeviceName,UserParams.PostProcObject,&(UserParams.PPCapability));

 		if((err = copy_to_user((STAUD_Ioctl_PPGetCapability_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetCapability_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_IPSETDATAINPUTINTERFACE:
 	{
 		STAUD_Ioctl_IPSetDataInputInterface_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetDataInputInterface_t *)arg, sizeof(STAUD_Ioctl_IPSetDataInputInterface_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		/* Nulls indicate connection to the PTI */
 		if (UserParams.GetWriteAddress == NULL || UserParams.InformReadAddress == NULL)
 		{
 			PTI_AudioBufferHandle = (STPTI_Buffer_t)(UserParams.BufferHandle);
 			UserParams.ErrorCode = STAUD_IPSetDataInputInterface(UserParams.Handle,UserParams.InputObject, AUD_GetPTIPESBuffWritePtr, AUD_SetPTIPESBuffReadPtrFct, (void*)PTI_AudioBufferHandle);
 		}
 		else
 		{
 			UserParams.ErrorCode = STAUD_IPSetDataInputInterface(UserParams.Handle,UserParams.InputObject,AUD_GetUserPESBuffWritePtr, AUD_SetUserPESBuffReadPtrFct, UserParams.BufferHandle);
 		}

 		if(UserParams.ErrorCode==ST_NO_ERROR)
 		{
 		    U8 PESBuffer;
            err = AUD_GetPESBufferFromAudioHandle(&PESBuffer, UserParams.Handle);
            if(err==0)
            {
                PESBuferInstanceCount[PESBuffer].BufferHandle = (U32) UserParams.BufferHandle;

            }
 		}

 		if((err = copy_to_user((STAUD_Ioctl_IPSetDataInputInterface_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetDataInputInterface_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETMODULEATTENUATION:
 	{
 		STAUD_Ioctl_SetModuleAttenuation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetModuleAttenuation_t *)arg, sizeof(STAUD_Ioctl_SetModuleAttenuation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetModuleAttenuation(UserParams.Handle,UserParams.Object,&(UserParams.Attenuation));

 		if((err = copy_to_user((STAUD_Ioctl_SetModuleAttenuation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetModuleAttenuation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETMODULEATTENUATION:
 	{
 		STAUD_Ioctl_GetModuleAttenuation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetModuleAttenuation_t *)arg, sizeof(STAUD_Ioctl_GetModuleAttenuation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetModuleAttenuation(UserParams.Handle,UserParams.Object,&(UserParams.Attenuation));


 		if((err = copy_to_user((STAUD_Ioctl_GetModuleAttenuation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetModuleAttenuation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#ifndef STAUD_NO_WRAPPER_LAYER

 	case STAUD_IOC_DISABLEDEEMPHASIS:
 	{
 		STAUD_Ioctl_DisableDeEmphasis_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DisableDeEmphasis_t *)arg, sizeof(STAUD_Ioctl_DisableDeEmphasis_t)) )){
 			/* Invalid user space address */
			goto fail;
 		}

	 	UserParams.ErrorCode = STAUD_DisableDeEmphasis(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_DisableDeEmphasis_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DisableDeEmphasis_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if defined (STB)
	 case STAUD_IOC_DISABLESYNCHRONISATION:
 	{
 		STAUD_Ioctl_DisableSynchronisation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DisableSynchronisation_t *)arg, sizeof(STAUD_Ioctl_DisableSynchronisation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DisableSynchronisation(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_DisableSynchronisation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DisableSynchronisation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

 	case STAUD_IOC_ENABLEDEEMPHASIS:
 	{
 		STAUD_Ioctl_EnableDeEmphasis_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_EnableDeEmphasis_t *)arg, sizeof(STAUD_Ioctl_EnableDeEmphasis_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_EnableDeEmphasis(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_EnableDeEmphasis_t *)arg, &UserParams, sizeof(STAUD_Ioctl_EnableDeEmphasis_t)))) {
 		/* Invalid user space address */
 		goto fail;
 		}
 	}
 	break;

#if defined (STB)
 	case STAUD_IOC_ENABLESYNCHRONISATION:
 	{
 		STAUD_Ioctl_EnableSynchronisation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_EnableSynchronisation_t *)arg, sizeof(STAUD_Ioctl_EnableSynchronisation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_EnableSynchronisation(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_EnableSynchronisation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_EnableSynchronisation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

	case STAUD_IOC_RESUME:
 	{
 		STAUD_Ioctl_Resume_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Resume_t *)arg, sizeof(STAUD_Ioctl_Resume_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_Resume(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_Resume_t *)arg, &UserParams, sizeof(STAUD_Ioctl_Resume_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETPROLOGIC:
 	{
 		STAUD_Ioctl_SetPrologic_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetPrologic_t *)arg, sizeof(STAUD_Ioctl_SetPrologic_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetPrologic(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_SetPrologic_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetPrologic_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETATTENUATION:
 	{
 		STAUD_Ioctl_GetAttenuation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetAttenuation_t *)arg, sizeof(STAUD_Ioctl_GetAttenuation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetAttenuation(UserParams.Handle,&(UserParams.Attenuation));

 		if((err = copy_to_user((STAUD_Ioctl_GetAttenuation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetAttenuation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETCHANNELDELAY:
 	{
 		STAUD_Ioctl_GetChannelDelay_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetChannelDelay_t *)arg, sizeof(STAUD_Ioctl_GetChannelDelay_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetChannelDelay(UserParams.Handle,&(UserParams.Delay));

 		if((err = copy_to_user((STAUD_Ioctl_GetChannelDelay_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetChannelDelay_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETDIGITALOUTPUT:
 	{
 		STAUD_Ioctl_GetDigitalOutput_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetDigitalOutput_t *)arg, sizeof(STAUD_Ioctl_GetDigitalOutput_t)) )){
			/* Invalid user space address */
			goto fail;
 		}

		UserParams.ErrorCode = STAUD_GetDigitalOutput(UserParams.Handle,&(UserParams.Mode));

		if((err = copy_to_user((STAUD_Ioctl_GetDigitalOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetDigitalOutput_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETDYNAMICRANGECONTROL:
 	{
 		STAUD_Ioctl_GetDynamicRangeControl_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetDynamicRangeControl_t *)arg, sizeof(STAUD_Ioctl_GetDynamicRangeControl_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetDynamicRangeControl(UserParams.Handle,&(UserParams.Control));

 		if((err = copy_to_user((STAUD_Ioctl_GetDynamicRangeControl_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetDynamicRangeControl_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if defined (STB)
 	case STAUD_IOC_START:
 	{
 		STAUD_Ioctl_Start_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Start_t *)arg, sizeof(STAUD_Ioctl_Start_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_Start(UserParams.Handle,&(UserParams.Params));

 		if((err = copy_to_user((STAUD_Ioctl_Start_t *)arg, &UserParams, sizeof(STAUD_Ioctl_Start_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

	case STAUD_IOC_MUTE:
 	{
 		STAUD_Ioctl_Mute_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Mute_t *)arg, sizeof(STAUD_Ioctl_Mute_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_Mute(UserParams.Handle,UserParams.AnalogueOutput,UserParams.DigitalOutput);

 		if((err = copy_to_user((STAUD_Ioctl_Mute_t *)arg, &UserParams, sizeof(STAUD_Ioctl_Mute_t)))) {
 			/* Invalid user space address */
			 goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETEFFECT:
 	{
 		STAUD_Ioctl_GetEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetEffect_t *)arg, sizeof(STAUD_Ioctl_GetEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetEffect(UserParams.Handle,&(UserParams.EffectMode));

 		if((err = copy_to_user((STAUD_Ioctl_GetEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetEffect_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETKARAOKEOUTPUT:
 	{
 		STAUD_Ioctl_GetKaraokeOutput_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetKaraokeOutput_t *)arg, sizeof(STAUD_Ioctl_GetKaraokeOutput_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetKaraokeOutput(UserParams.Handle,&(UserParams.KaraokeMode));

		if((err = copy_to_user((STAUD_Ioctl_GetKaraokeOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetKaraokeOutput_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETPROLOGIC:
 	{
 		STAUD_Ioctl_GetPrologic_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetPrologic_t *)arg, sizeof(STAUD_Ioctl_GetPrologic_t)) )){
			/* Invalid user space address */
			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetPrologic(UserParams.Handle,&(UserParams.PrologicState));

 		if((err = copy_to_user((STAUD_Ioctl_GetPrologic_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetPrologic_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETSPEAKER:
 	{
 		STAUD_Ioctl_GetSpeaker_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetSpeaker_t *)arg, sizeof(STAUD_Ioctl_GetSpeaker_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetSpeaker(UserParams.Handle,&(UserParams.Speaker));

 		if((err = copy_to_user((STAUD_Ioctl_GetSpeaker_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetSpeaker_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETSPEAKERCONFIGURATION:
 	{
 		STAUD_Ioctl_GetSpeakerConfiguration_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetSpeakerConfiguration_t *)arg, sizeof(STAUD_Ioctl_GetSpeakerConfiguration_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetSpeakerConfiguration(UserParams.Handle,&(UserParams.Speaker));

 		if((err = copy_to_user((STAUD_Ioctl_GetSpeakerConfiguration_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetSpeakerConfiguration_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_GETSTEREOOUTPUT:
 	{
 		STAUD_Ioctl_GetStereoOutput_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetStereoOutput_t *)arg, sizeof(STAUD_Ioctl_GetStereoOutput_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetStereoOutput(UserParams.Handle,&(UserParams.Mode));

 		if((err = copy_to_user((STAUD_Ioctl_GetStereoOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetStereoOutput_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if defined (DVD) || defined (ST_7100)
 	case STAUD_IOC_GETSYNCHROUNIT:
 	{
		STAUD_Ioctl_GetSynchroUnit_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetSynchroUnit_t *)arg, sizeof(STAUD_Ioctl_GetSynchroUnit_t)) )){
 			/* Invalid user space address */
 			goto fail;
		}

 		UserParams.ErrorCode = STAUD_GetSynchroUnit(UserParams.Handle,&(UserParams.SynchroUnit));

 		if((err = copy_to_user((STAUD_Ioctl_GetSynchroUnit_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetSynchroUnit_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

	case STAUD_IOC_PAUSE:
 	{
 		STAUD_Ioctl_Pause_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Pause_t *)arg, sizeof(STAUD_Ioctl_Pause_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_Pause(UserParams.Handle,&(UserParams.Fade));

 		if((err = copy_to_user((STAUD_Ioctl_Pause_t *)arg, &UserParams, sizeof(STAUD_Ioctl_Pause_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if defined (DVD)
 	case STAUD_IOC_PAUSESYNCHRO:
 	{
 		STAUD_Ioctl_PauseSynchro_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PauseSynchro_t *)arg, sizeof(STAUD_Ioctl_PauseSynchro_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PauseSynchro(UserParams.Handle,UserParams.Unit);

 		if((err = copy_to_user((STAUD_Ioctl_PauseSynchro_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PauseSynchro_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PREPARE:
 	{
 		STAUD_Ioctl_Prepare_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Prepare_t *)arg, sizeof(STAUD_Ioctl_Prepare_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_Prepare(UserParams.Handle,&(UserParams.Params));

 		if((err = copy_to_user((STAUD_Ioctl_Prepare_t *)arg, &UserParams, sizeof(STAUD_Ioctl_Prepare_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

	case STAUD_IOC_SETATTENUATION:
 	{
 		STAUD_Ioctl_SetAttenuation_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetAttenuation_t *)arg, sizeof(STAUD_Ioctl_SetAttenuation_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetAttenuation(UserParams.Handle,(UserParams.Attenuation));

 		if((err = copy_to_user((STAUD_Ioctl_SetAttenuation_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetAttenuation_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETCHANNELDELAY:
 	{
 		STAUD_Ioctl_SetChannelDelay_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetChannelDelay_t *)arg, sizeof(STAUD_Ioctl_SetChannelDelay_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetChannelDelay(UserParams.Handle,(UserParams.Delay));

 		if((err = copy_to_user((STAUD_Ioctl_SetChannelDelay_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetChannelDelay_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
	break;

 	case STAUD_IOC_SETDIGITALOUTPUT:
 	{
 		STAUD_Ioctl_SetDigitalOutput_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetDigitalOutput_t *)arg, sizeof(STAUD_Ioctl_SetDigitalOutput_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetDigitalOutput(UserParams.Handle,(UserParams.Mode));

 		if((err = copy_to_user((STAUD_Ioctl_SetDigitalOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetDigitalOutput_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETDYNAMICRANGECONTROL:
 	{
 		STAUD_Ioctl_SetDynamicRangeControl_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetDynamicRangeControl_t *)arg, sizeof(STAUD_Ioctl_SetDynamicRangeControl_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetDynamicRangeControl(UserParams.Handle,(UserParams.Control));

 		if((err = copy_to_user((STAUD_Ioctl_SetDynamicRangeControl_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetDynamicRangeControl_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETEFFECT:
 	{
 		STAUD_Ioctl_SetEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetEffect_t *)arg, sizeof(STAUD_Ioctl_SetEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetEffect(UserParams.Handle,(UserParams.EffectMode));

 		if((err = copy_to_user((STAUD_Ioctl_SetEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetEffect_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	 case STAUD_IOC_SETKARAOKEOUTPUT:
	 {
	 	STAUD_Ioctl_SetKaraokeOutput_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetKaraokeOutput_t *)arg, sizeof(STAUD_Ioctl_SetKaraokeOutput_t)) )){
			 /* Invalid user space address */
			 goto fail;
	 	}

		 UserParams.ErrorCode = STAUD_SetKaraokeOutput(UserParams.Handle,(UserParams.KaraokeMode));

	 	if((err = copy_to_user((STAUD_Ioctl_SetKaraokeOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetKaraokeOutput_t)))) {
			 /* Invalid user space address */
			 goto fail;
	 	}
	 }
	 break;

 	case STAUD_IOC_SETSPEAKER:
 	{
 		STAUD_Ioctl_SetSpeaker_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetSpeaker_t *)arg, sizeof(STAUD_Ioctl_SetSpeaker_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetSpeaker(UserParams.Handle,(UserParams.Speaker));

 		if((err = copy_to_user((STAUD_Ioctl_SetSpeaker_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetSpeaker_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETSPEAKERCONFIGURATION:
 	{
 		STAUD_Ioctl_SetSpeakerConfiguration_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetSpeakerConfiguration_t *)arg, sizeof(STAUD_Ioctl_SetSpeakerConfiguration_t)) )){
 			/* Invalid user space address */
 			goto fail;
 }

 		UserParams.ErrorCode = STAUD_SetSpeakerConfiguration(UserParams.Handle,(UserParams.Speaker),UserParams.BassType);

 		if((err = copy_to_user((STAUD_Ioctl_SetSpeakerConfiguration_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetSpeakerConfiguration_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_SETSTEREOOUTPUT:
 	{
 		STAUD_Ioctl_SetStereoOutput_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetStereoOutput_t *)arg, sizeof(STAUD_Ioctl_SetStereoOutput_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

		UserParams.ErrorCode = STAUD_SetStereoOutput(UserParams.Handle,(UserParams.Mode));

 		if((err = copy_to_user((STAUD_Ioctl_SetStereoOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetStereoOutput_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_STOP:
 	{
 		STAUD_Ioctl_Stop_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_Stop_t *)arg, sizeof(STAUD_Ioctl_Stop_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_Stop(UserParams.Handle,UserParams.StopMode,&(UserParams.Fade));

 		if((err = copy_to_user((STAUD_Ioctl_Stop_t *)arg, &UserParams, sizeof(STAUD_Ioctl_Stop_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#if 0
 	case STAUD_IOC_GETINPUTBUFFERPARAMS:
 	{
 		STAUD_Ioctl_GetInputBufferParams_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetInputBufferParams_t *)arg, sizeof(STAUD_Ioctl_GetInputBufferParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GetInputBufferParams(UserParams.Handle,UserParams.PlayerID,&(UserParams.DataParams));

 		if((err = copy_to_user((STAUD_Ioctl_GetInputBufferParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetInputBufferParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_INTHANDLERINSTALL:
 	{
		STAUD_Ioctl_IntHandlerInstall_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IntHandlerInstall_t *)arg, sizeof(STAUD_Ioctl_IntHandlerInstall_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = AUD_IntHandlerInstall(UserParams.InterruptNumber,UserParams.InterruptLevel);

 		if((err = copy_to_user((STAUD_Ioctl_IntHandlerInstall_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IntHandlerInstall_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;
#endif

#if defined (DVD)
 	case STAUD_IOC_SKIPSYNCHRO:
 	{
 		STAUD_Ioctl_SkipSynchro_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SkipSynchro_t *)arg, sizeof(STAUD_Ioctl_SkipSynchro_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SkipSynchro(UserParams.Handle,UserParams.PlayerID,UserParams.Delay);

 		if((err = copy_to_user((STAUD_Ioctl_SkipSynchro_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SkipSynchro_t)))) {
		/* Invalid user space address */
 		goto fail;
 		}
 	}
 	break;
#endif
#endif /* STAUD_NO_WRAPPER_LAYER */

 	case STAUD_IOC_DRSETDEEMPHASISFILTER:
 	{
 		STAUD_Ioctl_DRSetDeEmphasisFilter_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetDeEmphasisFilter_t *)arg, sizeof(STAUD_Ioctl_DRSetDeEmphasisFilter_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetDeEmphasisFilter(UserParams.Handle,UserParams.DecoderObject,UserParams.Emphasis);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetDeEmphasisFilter_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetDeEmphasisFilter_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRGETDEEMPHASISFILTER:
 	{
 		STAUD_Ioctl_DRGetDeEmphasisFilter_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetDeEmphasisFilter_t *)arg, sizeof(STAUD_Ioctl_DRGetDeEmphasisFilter_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetDeEmphasisFilter(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Emphasis));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetDeEmphasisFilter_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetDeEmphasisFilter_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRSETEFFECT:
 	{
 		STAUD_Ioctl_DRSetEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetEffect_t *)arg, sizeof(STAUD_Ioctl_DRSetEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetEffect(UserParams.Handle,UserParams.DecoderObject,UserParams.Effect);


 		if((err = copy_to_user((STAUD_Ioctl_DRSetEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetEffect_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRGETEFFECT:
 	{
 		STAUD_Ioctl_DRGetEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetEffect_t *)arg, sizeof(STAUD_Ioctl_DRGetEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetEffect(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Effect));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetEffect_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRSETPROLOGIC:
 	{
 		STAUD_Ioctl_DRSetPrologic_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetPrologic_t *)arg, sizeof(STAUD_Ioctl_DRSetPrologic_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetPrologic(UserParams.Handle,UserParams.DecoderObject,UserParams.Prologic);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetPrologic_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetPrologic_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRGETPROLOGIC:
 	{
 		STAUD_Ioctl_DRGetPrologic_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetPrologic_t *)arg, sizeof(STAUD_Ioctl_DRGetPrologic_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetPrologic(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Prologic));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetPrologic_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetPrologic_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRSETSTEREOMODE:
 	{
 		STAUD_Ioctl_DRSetStereoMode_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetStereoMode_t *)arg, sizeof(STAUD_Ioctl_DRSetStereoMode_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetStereoMode(UserParams.Handle,UserParams.DecoderObject,UserParams.StereoMode);


 		if((err = copy_to_user((STAUD_Ioctl_DRSetStereoMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetStereoMode_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRGETSTEREOMODE:
 	{
 		STAUD_Ioctl_DRGetStereoMode_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetStereoMode_t *)arg, sizeof(STAUD_Ioctl_DRGetStereoMode_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetStereoMode(UserParams.Handle,UserParams.DecoderObject,&(UserParams.StereoMode));


 		if((err = copy_to_user((STAUD_Ioctl_DRGetStereoMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetStereoMode_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRMUTE:
 	{
 		STAUD_Ioctl_DRMute_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRMute_t *)arg, sizeof(STAUD_Ioctl_DRMute_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRMute(UserParams.Handle,UserParams.OutputObject);


 		if((err = copy_to_user((STAUD_Ioctl_DRMute_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRMute_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRUNMUTE:
 	{
 		STAUD_Ioctl_DRUnMute_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRUnMute_t *)arg, sizeof(STAUD_Ioctl_DRUnMute_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRUnMute(UserParams.Handle,UserParams.OutputObject);


 		if((err = copy_to_user((STAUD_Ioctl_DRUnMute_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRUnMute_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#ifndef STAUD_NO_WRAPPER_LAYER

 	case STAUD_IOC_PLAYERSTART:
 	{
 		STAUD_Ioctl_PlayerStart_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PlayerStart_t *)arg, sizeof(STAUD_Ioctl_PlayerStart_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PlayerStart(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_PlayerStart_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PlayerStart_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_PLAYERSTOP:
 	{
 		STAUD_Ioctl_PlayerStop_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PlayerStop_t *)arg, sizeof(STAUD_Ioctl_PlayerStop_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PlayerStop(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_PlayerStop_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PlayerStop_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

#endif /* STAUD_NO_WRAPPER_LAYER */

 	case STAUD_IOC_PPDCREMOVE:
 	{
 		STAUD_Ioctl_PPDcRemove_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPDcRemove_t *)arg, sizeof(STAUD_Ioctl_PPDcRemove_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPDcRemove(UserParams.Handle,UserParams.PPObject,&UserParams.Params);

 		if((err = copy_to_user((STAUD_Ioctl_PPDcRemove_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PPDcRemove_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRSETCOMPRESSIONMODE:
 	{
 		STAUD_Ioctl_DRSetCompressionMode_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetCompressionMode_t *)arg, sizeof(STAUD_Ioctl_DRSetCompressionMode_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetCompressionMode(UserParams.Handle,UserParams.DecoderObject,UserParams.CompressionMode);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetCompressionMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetCompressionMode_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRSETAUDIOCODINGMODE:
 	{
 		STAUD_Ioctl_DRSetAudioCodingMode_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetAudioCodingMode_t *)arg, sizeof(STAUD_Ioctl_DRSetAudioCodingMode_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetAudioCodingMode(UserParams.Handle,UserParams.DecoderObject,UserParams.AudioCodingMode);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetAudioCodingMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetAudioCodingMode_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_IPSETPCMPARAMS:
 	{
 		STAUD_Ioctl_IPSetPcmParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetPcmParams_t *)arg, sizeof(STAUD_Ioctl_IPSetPcmParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPSetPCMParams(UserParams.Handle,UserParams.InputObject,&(UserParams.InputParams));

 		if((err = copy_to_user((STAUD_Ioctl_IPSetPcmParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetPcmParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_IPGETPCMBUFFER:
 	{
 		STAUD_Ioctl_IPGetPCMBuffer_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetPCMBuffer_t *)arg, sizeof(STAUD_Ioctl_IPGetPCMBuffer_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

		UserParams.ErrorCode = STAUD_IPGetPCMBuffer(UserParams.Handle,UserParams.InputObject,&(UserParams.PCMBuffer_p));
		//printk(KERN_INFO"ioctl_ioctl ErrorCode (%d)\n", UserParams.ErrorCode);

 		if((err = copy_to_user((STAUD_Ioctl_IPGetPCMBuffer_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetPCMBuffer_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_IPGETPCMBUFFERSIZE:
 	{
 		STAUD_Ioctl_IPGetPCMBufferSize_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetPCMBufferSize_t *)arg, sizeof(STAUD_Ioctl_IPGetPCMBufferSize_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPGetPCMBufferSize(UserParams.Handle,UserParams.InputObject,&(UserParams.BufferSize));

 		if((err = copy_to_user((STAUD_Ioctl_IPGetPCMBuffer_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetPCMBufferSize_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRGETSRSEFFECTPARAM:
 	{
 		STAUD_Ioctl_DRGetSrsEffectParam_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetSrsEffectParam_t *)arg, sizeof(STAUD_Ioctl_DRGetSrsEffectParam_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetSRSEffectParams(UserParams.Handle,UserParams.DecoderObject,UserParams.ParamType,&(UserParams.Value_p));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetSrsEffectParam_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetSrsEffectParam_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRSETDOWNMIX:
 	{
 		STAUD_Ioctl_DRSetDownMix_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetDownMix_t *)arg, sizeof(STAUD_Ioctl_DRSetDownMix_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetDownMix(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DownMixParam));

 		if((err = copy_to_user((STAUD_Ioctl_DRSetDownMix_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetDownMix_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRSETSRSEFFECT:
 	{
		STAUD_Ioctl_DRSetSrsEffect_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetSrsEffect_t *)arg, sizeof(STAUD_Ioctl_DRSetSrsEffect_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetSRSEffectParams(UserParams.Handle,UserParams.DecoderObject,UserParams.ParamsType,UserParams.Value);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetSrsEffect_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetSrsEffect_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_MXUPDATEPTSSTATUS:
 	{
 		STAUD_IOCTL_MXUpdatePTSStatus_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_IOCTL_MXUpdatePTSStatus_t *)arg, sizeof(STAUD_IOCTL_MXUpdatePTSStatus_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_MXUpdatePTSStatus(UserParams.Handle,UserParams.MixerObject,UserParams.InputID,UserParams.PTSStatus);

 		if((err = copy_to_user((STAUD_IOCTL_MXUpdatePTSStatus_t *)arg, &UserParams, sizeof(STAUD_IOCTL_MXUpdatePTSStatus_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;


	case STAUD_IOC_SETCLOCKRECOVERYSOURCE:
	{
		STAUD_Ioctl_SetClockRecoverySource_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetClockRecoverySource_t *)arg, sizeof(STAUD_Ioctl_SetClockRecoverySource_t)) ))
		{
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_SetClockRecoverySource(UserParams.Handle,UserParams.ClkSource);
		if((err = copy_to_user((STAUD_Ioctl_SetClockRecoverySource_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetClockRecoverySource_t))))
		{
			/* Invalid user space address */
			goto fail;
		}
	}
	break;


	case STAUD_IOC_OPENABLEHDMIOUTPUT:
	{
		STAUD_Ioctl_OPEnableHDMIOutput_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPEnableHDMIOutput_t *)arg, sizeof(STAUD_Ioctl_OPEnableHDMIOutput_t)) ))
		{
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_OPEnableHDMIOutput(UserParams.Handle,UserParams.OutputObject);
		if((err = copy_to_user((STAUD_Ioctl_OPEnableHDMIOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPEnableHDMIOutput_t))))
		{
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_OPDISABLEHDMIOUTPUT:
	{
		STAUD_Ioctl_OPDisableHDMIOutput_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPDisableHDMIOutput_t *)arg, sizeof(STAUD_Ioctl_OPDisableHDMIOutput_t)) ))
		{
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_OPDisableHDMIOutput(UserParams.Handle,UserParams.OutputObject);

		if((err = copy_to_user((STAUD_Ioctl_OPDisableHDMIOutput_t *)arg, &UserParams, sizeof(STAUD_Ioctl_OPDisableHDMIOutput_t))))
		{
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRGETBEEPTONEFREQ:
 	{
 		STAUD_Ioctl_DRGetBeepToneFreq_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetBeepToneFreq_t *)arg, sizeof(STAUD_Ioctl_DRGetBeepToneFreq_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetBeepToneFrequency(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Freq));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetBeepToneFreq_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetBeepToneFreq_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRSETBEEPTONEFREQ:
 	{
 		STAUD_Ioctl_DRSetBeepToneFreq_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetBeepToneFreq_t *)arg, sizeof(STAUD_Ioctl_DRSetBeepToneFreq_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetBeepToneFrequency(UserParams.Handle,UserParams.DecoderObject,UserParams.BeepToneFrequency);
		if((err = copy_to_user((STAUD_Ioctl_DRSetBeepToneFreq_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetBeepToneFreq_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_MXSETMIXERLEVEL:
 	{
 		STAUD_Ioctl_MXSetMixerLevel_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_MXSetMixerLevel_t *)arg, sizeof(STAUD_Ioctl_MXSetMixerLevel_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_MXSetMixLevel(UserParams.Handle,UserParams.MixerObject,UserParams.InputID,UserParams.MixLevel);

 		if((err = copy_to_user((STAUD_Ioctl_MXSetMixerLevel_t*)arg, &UserParams, sizeof(STAUD_Ioctl_MXSetMixerLevel_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_IPSETBROADCASTPROFILE:
 	{
 		STAUD_Ioctl_IPSetBroadcastProfile_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetBroadcastProfile_t *)arg, sizeof(STAUD_Ioctl_IPSetBroadcastProfile_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPSetBroadcastProfile(UserParams.Handle,UserParams.InputObject,UserParams.BroadcastProfile);

 		if((err = copy_to_user((STAUD_Ioctl_IPSetBroadcastProfile_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetBroadcastProfile_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_AWAIT_CALLBACK:
 	{
 		if ((err = copy_from_user(&CallbackData, (STAUD_Ioctl_CallbackData_t *)arg, sizeof(STAUD_Ioctl_CallbackData_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		callback_thread = current; /* global linux current task pointer */
 		up(&callback_return_sem);

 		if (down_interruptible(&callback_call_sem))
 			return -ERESTARTSYS;

 		callback_thread = NULL;

 		if((err = copy_to_user((STAUD_Ioctl_CallbackData_t*)arg, &CallbackData, sizeof(STAUD_Ioctl_CallbackData_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_IPGETBITBUFFERFREESIZE:
	{
		STAUD_Ioctl_IPGetBitBufferFreeSize_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetBitBufferFreeSize_t *)arg, sizeof(STAUD_Ioctl_IPGetBitBufferFreeSize_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPGetBitBufferFreeSize(UserParams.Handle,UserParams.InputObject,&(UserParams.Size));

 		if((err = copy_to_user((STAUD_Ioctl_IPGetBitBufferFreeSize_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetBitBufferFreeSize_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_IPSETPCMREADERPARAMS:
	{
		STAUD_Ioctl_IPSetPCMReaderParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPSetPCMReaderParams_t *)arg, sizeof(STAUD_Ioctl_IPSetPCMReaderParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPSetPCMReaderParams(UserParams.Handle,UserParams.InputObject,&(UserParams.ReaderParams));

 		if((err = copy_to_user((STAUD_Ioctl_IPSetPCMReaderParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPSetPCMReaderParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_DRSETDOLBYDIGITALEX:
	{
		STAUD_Ioctl_DRSetDolbyDigitalEx_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetDolbyDigitalEx_t *)arg, sizeof(STAUD_Ioctl_DRSetDolbyDigitalEx_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetDolbyDigitalEx(UserParams.Handle,UserParams.DecoderObject,UserParams.DolbyDigitalEx);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetDolbyDigitalEx_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetDolbyDigitalEx_t)))) {
 		/* Invalid user space address */
 		goto fail;
 		}
	}
	break;

	case STAUD_IOC_DRGETDOLBYDIGITALEX:
	{
		STAUD_Ioctl_DRGetDolbyDigitalEx_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetDolbyDigitalEx_t *)arg, sizeof(STAUD_Ioctl_DRGetDolbyDigitalEx_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetDolbyDigitalEx(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DolbyDigitalEx_p));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetDolbyDigitalEx_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetDolbyDigitalEx_t)))) {
 		/* Invalid user space address */
 		goto fail;
 		}
	}
	break;

	case STAUD_IOC_PPGETEQUALIZATIONPARAMS:
	{
		STAUD_Ioctl_PPGetEqualizationParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetEqualizationParams_t *)arg, sizeof(STAUD_Ioctl_PPGetEqualizationParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPGetEqualizationParams(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Equalization_p));

 		if((err = copy_to_user((STAUD_Ioctl_PPGetEqualizationParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetEqualizationParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_PPSETEQUALIZATIONPARAMS:
	{
		STAUD_Ioctl_PPSetEqualizationParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetEqualizationParams_t *)arg, sizeof(STAUD_Ioctl_PPSetEqualizationParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_PPSetEqualizationParams(UserParams.Handle,UserParams.PostProcObject,&(UserParams.Equalization_p));

 		if((err = copy_to_user((STAUD_Ioctl_PPSetEqualizationParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetEqualizationParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_DRSETOMNIPARAMS:
	{
		STAUD_Ioctl_DRSetOmniParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetOmniParams_t *)arg, sizeof(STAUD_Ioctl_DRSetOmniParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetOmniParams(UserParams.Handle,UserParams.DecoderObject,UserParams.Omni);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetOmniParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetOmniParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_DRGETOMNIPARAMS:
	{
		STAUD_Ioctl_DRGetOmniParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetOmniParams_t *)arg, sizeof(STAUD_Ioctl_DRGetOmniParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetOmniParams(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Omni_p));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetOmniParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetOmniParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_MODULESTART:
	{
		STAUD_Ioctl_ModuleStart_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_ModuleStart_t *)arg, sizeof(STAUD_Ioctl_ModuleStart_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_ModuleStart(UserParams.Handle,UserParams.ModuleObject,&(UserParams.StreamParams_p));

 		if((err = copy_to_user((STAUD_Ioctl_ModuleStart_t*)arg, &UserParams, sizeof(STAUD_Ioctl_ModuleStart_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_GENERICSTART:
	{
		STAUD_Ioctl_GenericStart_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GenericStart_t *)arg, sizeof(STAUD_Ioctl_GenericStart_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GenericStart(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_GenericStart_t*)arg, &UserParams, sizeof(STAUD_Ioctl_GenericStart_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_IPGETPCMREADERPARAMS:
	{
		STAUD_Ioctl_IPGetPCMReaderParams_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetPCMReaderParams_t *)arg, sizeof(STAUD_Ioctl_IPGetPCMReaderParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPGetPCMReaderParams(UserParams.Handle,UserParams.InputObject,&(UserParams.PCMReaderParams));

 		if((err = copy_to_user((STAUD_Ioctl_IPGetPCMReaderParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetPCMReaderParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_MODULESTOP:
	{
		STAUD_Ioctl_ModuleStop_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_ModuleStop_t *)arg, sizeof(STAUD_Ioctl_ModuleStop_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_ModuleStop(UserParams.Handle,UserParams.ModuleObject);

 		if((err = copy_to_user((STAUD_Ioctl_ModuleStop_t *)arg, &UserParams, sizeof(STAUD_Ioctl_ModuleStop_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_GENERICSTOP:
	{
		STAUD_Ioctl_GenericStop_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GenericStop_t *)arg, sizeof(STAUD_Ioctl_GenericStop_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_GenericStop(UserParams.Handle);

 		if((err = copy_to_user((STAUD_Ioctl_GenericStop_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GenericStop_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_MXUPDATEMASTER:
	{
		STAUD_Ioctl_MXUpdateMaster_t UserParams;

	 	if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_MXUpdateMaster_t *)arg, sizeof(STAUD_Ioctl_MXUpdateMaster_t)) )){
	 		/* Invalid user space address */
	 		goto fail;
	 	}

	 	UserParams.ErrorCode = STAUD_MXUpdateMaster(UserParams.Handle,UserParams.MixerObject,UserParams.InputID,UserParams.FreeRun);

	 	if((err = copy_to_user((STAUD_Ioctl_MXUpdateMaster_t*)arg, &UserParams, sizeof(STAUD_Ioctl_MXUpdateMaster_t)))) {
	 		/* Invalid user space address */
	 		goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_OPSETLATENCY:
	{
	 	STAUD_Ioctl_OPSetLatency_t UserParams;

	 	if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPSetLatency_t *)arg, sizeof(STAUD_Ioctl_OPSetLatency_t)) )){
	 		/* Invalid user space address */
	 		goto fail;
	 	}

	 	UserParams.ErrorCode = STAUD_OPSetLatency(UserParams.Handle,UserParams.OutputObject,UserParams.Latency);

	 	if((err = copy_to_user((STAUD_Ioctl_OPSetLatency_t*)arg, &UserParams, sizeof(STAUD_Ioctl_OPSetLatency_t)))) {
	 		/* Invalid user space address */
	 		goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRSETADVPLII:
 	{
 		STAUD_Ioctl_DRSetAdvancePLII_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetAdvancePLII_t *)arg, sizeof(STAUD_Ioctl_DRSetAdvancePLII_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetPrologicAdvance(UserParams.Handle,UserParams.DecoderObject,UserParams.PLIIParams);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetAdvancePLII_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetAdvancePLII_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

 	case STAUD_IOC_DRGETADVPLII:
 	{
 		STAUD_Ioctl_DRGetAdvancePLII_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetAdvancePLII_t *)arg, sizeof(STAUD_Ioctl_DRGetAdvancePLII_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetPrologicAdvance(UserParams.Handle,UserParams.DecoderObject,&(UserParams.PLIIParams));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetAdvancePLII_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetAdvancePLII_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_IPGETSYNCHROUNIT:
 	{
 		STAUD_Ioctl_IPGetSynchroUnit_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPGetSynchroUnit_t *)arg, sizeof(STAUD_Ioctl_IPGetSynchroUnit_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_IPGetSynchroUnit(UserParams.Handle,UserParams.inputObject,&(UserParams.SynchroUnit));

 		if((err = copy_to_user((STAUD_Ioctl_IPGetSynchroUnit_t*)arg, &UserParams, sizeof(STAUD_Ioctl_IPGetSynchroUnit_t)))) {
 			/* Invalid user space address */
			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_OPENCSETDIGITAL:
 	{
 		STAUD_Ioctl_OPEncSetDigital_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_OPEncSetDigital_t *)arg, sizeof(STAUD_Ioctl_OPEncSetDigital_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_OPSetEncDigitalOutput(UserParams.Handle,UserParams.OutputObject,UserParams.EnableDisableEncOutput);

 		if((err = copy_to_user((STAUD_Ioctl_OPEncSetDigital_t*)arg, &UserParams, sizeof(STAUD_Ioctl_OPEncSetDigital_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRSETCIRCLE:
 	{
 		STAUD_Ioctl_DRSetCircleSrround_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetCircleSrround_t *)arg, sizeof(STAUD_Ioctl_DRSetCircleSrround_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetCircleSurroundParams(UserParams.Handle,UserParams.DecoderObject,UserParams.Csii);

 		if((err = copy_to_user((STAUD_Ioctl_DRSetCircleSrround_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetCircleSrround_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRGETCIRCLE:
 	{
 		STAUD_Ioctl_DRGetCircleSrround_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetCircleSrround_t *)arg, sizeof(STAUD_Ioctl_DRGetCircleSrround_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetCircleSurroundParams(UserParams.Handle,UserParams.DecoderObject,&(UserParams.Csii));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetCircleSrround_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetCircleSrround_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_IPHANDLEEOF:
	{
		STAUD_Ioctl_IPHandleEOF_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_IPHandleEOF_t *)arg, sizeof(STAUD_Ioctl_IPHandleEOF_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_IPHandleEOF(UserParams.Handle,UserParams.InputObject);

		if((err = copy_to_user((STAUD_Ioctl_IPHandleEOF_t *)arg, &UserParams, sizeof(STAUD_Ioctl_IPHandleEOF_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRSETDDPOP:
	{
		STAUD_Ioctl_DRSetDDPOP_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetDDPOP_t *)arg, sizeof(STAUD_Ioctl_DRSetDDPOP_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRSetDDPOP(UserParams.Handle,UserParams.DecoderObject,UserParams.DDPOPSetting);

		if((err = copy_to_user((STAUD_Ioctl_DRSetDDPOP_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetDDPOP_t)))) {
			/* Invalid user space address */
			goto fail;
		}

	}
	break;

	case STAUD_IOC_ENCGETCAPABILITY:
 	{
 		STAUD_Ioctl_ENCGetCapability_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_ENCGetCapability_t *)arg, sizeof(STAUD_Ioctl_ENCGetCapability_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_ENCGetCapability(UserParams.DeviceName,UserParams.EncoderObject,&(UserParams.Capability));

 		if((err = copy_to_user((STAUD_Ioctl_ENCGetCapability_t*)arg, &UserParams, sizeof(STAUD_Ioctl_ENCGetCapability_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_ENCSETOUTPUTPARAMS:
 	{
 		STAUD_Ioctl_ENCSetOutputParams_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_ENCSetOutputParams_t *)arg, sizeof(STAUD_Ioctl_ENCSetOutputParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_ENCSetOutputParams(UserParams.Handle,UserParams.EncoderObject,UserParams.EncoderOPParams);

 		if((err = copy_to_user((STAUD_Ioctl_ENCSetOutputParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_ENCSetOutputParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_ENCGETOUTPUTPARAMS:
 	{
 		STAUD_Ioctl_ENCGetOutputParams_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_ENCGetOutputParams_t *)arg, sizeof(STAUD_Ioctl_ENCGetOutputParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_ENCGetOutputParams(UserParams.Handle,UserParams.EncoderObject,&(UserParams.EncoderOPParams));

 		if((err = copy_to_user((STAUD_Ioctl_ENCGetOutputParams_t*)arg, &UserParams, sizeof(STAUD_Ioctl_ENCGetOutputParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_CONNECTSRCDST:
 	{
 		STAUD_Ioctl_ConnectSrcDst_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_ConnectSrcDst_t *)arg, sizeof(STAUD_Ioctl_ConnectSrcDst_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_ConnectSrcDst(UserParams.Handle,UserParams.SrcObject,UserParams.SrcId,UserParams.DstObject,UserParams.DstId);

 		if((err = copy_to_user((STAUD_Ioctl_ConnectSrcDst_t*)arg, &UserParams, sizeof(STAUD_Ioctl_ConnectSrcDst_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DISCONNECTINPUT:
 	{
 		STAUD_Ioctl_DisconnectInput_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DisconnectInput_t *)arg, sizeof(STAUD_Ioctl_DisconnectInput_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DisconnectInput(UserParams.Handle,UserParams.Object,UserParams.InputId);

 		if((err = copy_to_user((STAUD_Ioctl_DisconnectInput_t*)arg, &UserParams, sizeof(STAUD_Ioctl_DisconnectInput_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DPSETCALLBACK:
 	{
		STAUD_Ioctl_DPSetCallback UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DPSetCallback *)arg, sizeof(STAUD_Ioctl_DPSetCallback)) )){
			/* Invalid user space address */
			goto fail;
		}

		/* Call DPSetCallback and pass pointer to STAUD_FPParamCopy to register the callback */
	 	/* But before doing that wait for the AUD_FPCallback thread to start in user space*/
 		if (!fpcallback_thread_count)
 		{
 			if(down_interruptible(&sema_call))
 			{
 				err=-EINTR;
 				goto fail;
 			}
 			STTBX_Print((" STAUD_IOC_DPSETCALLBACK: Invoking DPSetCallback \n"));
 			UserParams.ErrorCode=STAUD_DPSetCallback(UserParams.Handle,UserParams.inputObject,(FrameDeliverFunc_t )STAUD_FPParamCopy,UserParams.clientInfo);
 		}

 		if( UserParams.ErrorCode == ST_NO_ERROR)
 		{
 			fpcallback_thread_count++;
 		}

		if((err = copy_to_user((STAUD_Ioctl_DPSetCallback *)arg, &UserParams, sizeof(STAUD_Ioctl_DPSetCallback)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_GETBUFFERPARAM:
	{
		STAUD_Ioctl_GetBufferParam_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_GetBufferParam_t *)arg, sizeof(STAUD_Ioctl_GetBufferParam_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_GetBufferParam(UserParams.Handle,UserParams.InputObject,&(UserParams.BufferParam));

		if((err = copy_to_user((STAUD_Ioctl_GetBufferParam_t *)arg, &UserParams, sizeof(STAUD_Ioctl_GetBufferParam_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_AWAIT_FPCALLBACK:
	{
		STAUD_Ioctl_FPCallbackData_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_FPCallbackData_t *)arg, sizeof(STAUD_Ioctl_FPCallbackData_t)) )){
			/* Invalid user space address */
			goto fail;
		}
		//printk(KERN_ALERT " STAUD_IOC_AWAIT_FPCALLBACK: wait on semaphore sema_return \n");
		//printk(KERN_ALERT " STAUD_IOC_AWAIT_FPCALLBACK: release semaphore sema_call \n");

		/* Release the sema_call semaphore for the waiting callback to continue */
		up(&sema_call);

		/* Wait for the callback function to copy the desired info to tempData before passing it to user space */
		if(down_interruptible(&sema_return))
 		{
 			err=-EINTR;
 			goto fail;
 		}
		UserParams.MemoryStart= tempData.MemoryStart;
		UserParams.Size= tempData.Size;
		UserParams.bufferMetaData= tempData.bufferMetaData;
		UserParams.clientInfo= tempData.clientInfo;
		UserParams.ErrorCode= ST_NO_ERROR;

		if((err = copy_to_user((STAUD_Ioctl_FPCallbackData_t *)arg, &UserParams, sizeof(STAUD_Ioctl_FPCallbackData_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;

	case STAUD_IOC_DRSETDTSNEO:
	{
	 	STAUD_Ioctl_DRSetDTSNeo_t UserParams;
	 	if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetDTSNeo_t *)arg, sizeof(STAUD_Ioctl_DRSetDTSNeo_t)) )){
	 		/* Invalid user space address */
	 		goto fail;
	 	}

	 	UserParams.ErrorCode = STAUD_DRSetDTSNeoParams(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DTSNeo));

	 	if((err = copy_to_user((STAUD_Ioctl_DRSetDTSNeo_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetDTSNeo_t)))) {
	 		/* Invalid user space address */
	 		goto fail;
	 	}
	}
	break;

	case STAUD_IOC_DRGETDTSNEO:
 	{
 		STAUD_Ioctl_DRGetDTSNeo_t UserParams;

 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetDTSNeo_t *)arg, sizeof(STAUD_Ioctl_DRGetDTSNeo_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRGetDTSNeoParams(UserParams.Handle,UserParams.DecoderObject,&(UserParams.DTSNeo));

 		if((err = copy_to_user((STAUD_Ioctl_DRGetDTSNeo_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetDTSNeo_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
 	}
 	break;

	case STAUD_IOC_DRSETBTSC:
	{
		STAUD_Ioctl_PPSetBTSC_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPSetBTSC_t *)arg, sizeof(STAUD_Ioctl_PPSetBTSC_t)) )){
			/* Invalid user space address */
			goto fail;
	 	}

		UserParams.ErrorCode = STAUD_PPSetBTSCParams(UserParams.Handle,UserParams.PPObject,&(UserParams.BTSC));

	 	if((err = copy_to_user((STAUD_Ioctl_PPSetBTSC_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPSetBTSC_t)))) {
	 		/* Invalid user space address */
	 		goto fail;
	 	}
	}
	break;

	case STAUD_IOC_DRGETBTSC:
	{
		STAUD_Ioctl_PPGetBTSC_t UserParams;

		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_PPGetBTSC_t *)arg, sizeof(STAUD_Ioctl_PPGetBTSC_t)) )){
			/* Invalid user space address */
	 		goto fail;
	 	}

		UserParams.ErrorCode = STAUD_PPGetBTSCParams(UserParams.Handle,UserParams.PPObject,&(UserParams.BTSC));

	 	if((err = copy_to_user((STAUD_Ioctl_PPGetBTSC_t *)arg, &UserParams, sizeof(STAUD_Ioctl_PPGetBTSC_t)))) {
	 		/* Invalid user space address */
	 		goto fail;
	 	}
	}
	break;

	case STAUD_IOC_SETSCENARIO:
	{
 		STAUD_Ioctl_SetScenario_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_SetScenario_t *)arg, sizeof(STAUD_Ioctl_SetScenario_t)) )){
			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_SetScenario(UserParams.Handle,UserParams.Scenario);

 		if((err = copy_to_user((STAUD_Ioctl_SetScenario_t *)arg, &UserParams, sizeof(STAUD_Ioctl_SetScenario_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_DRSETINITPARAMS:
	{
 		STAUD_Ioctl_DRSetInitParams_t UserParams;
 		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRSetInitParams_t *)arg, sizeof(STAUD_Ioctl_DRSetInitParams_t)) )){
 			/* Invalid user space address */
 			goto fail;
 		}

 		UserParams.ErrorCode = STAUD_DRSetInitParams(UserParams.Handle,UserParams.DecoderObject, &(UserParams.DRInitParams));

 		if((err = copy_to_user((STAUD_Ioctl_DRSetInitParams_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRSetInitParams_t)))) {
 			/* Invalid user space address */
 			goto fail;
 		}
	}
	break;

	case STAUD_IOC_DRGETSTREAMINFO:
	{
		STAUD_Ioctl_DRGetStreamInfo_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetStreamInfo_t *)arg, sizeof(STAUD_Ioctl_DRGetStreamInfo_t)) )){
			/* Invalid user space address */
			goto fail;
		}

		UserParams.ErrorCode = STAUD_DRGetStreamInfo(UserParams.Handle,UserParams.DecoderObject, &(UserParams.StreamInfo));

		if((err = copy_to_user((STAUD_Ioctl_DRGetStreamInfo_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetStreamInfo_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;
	case STAUD_IOC_DRGETINSTEREOMODE:
	{
		STAUD_Ioctl_DRGetInStereoMode_t UserParams;
		if ((err = copy_from_user(&UserParams, (STAUD_Ioctl_DRGetInStereoMode_t *)arg, sizeof(STAUD_Ioctl_DRGetInStereoMode_t)) )){
			/* Invalid user space address */
			goto fail;
		}
		UserParams.ErrorCode = STAUD_DRGetInStereoMode(UserParams.Handle,UserParams.DecoderObject,&(UserParams.StereoMode));

		if((err = copy_to_user((STAUD_Ioctl_DRGetInStereoMode_t *)arg, &UserParams, sizeof(STAUD_Ioctl_DRGetInStereoMode_t)))) {
			/* Invalid user space address */
			goto fail;
		}
	}
	break;
	default :
 		/*** Inappropriate ioctl for the device ***/
 		printk(KERN_ALERT "ERROR CASE NOT FOUND\n");
 		err = -ENOTTY;
 		goto fail;
}


 /**************************************************************************/

 /*** Clean up on error ***/

fail:
 return (err);
}


