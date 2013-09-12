/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description :
 *
 *****************************************************************************/

/* This macro is used to printout all STPTI calls to the kernel log */
#define STPTI_PRINTK_IOCTL(x)


/* Requires   MODULE   defined on the command line */
/* Requires __KERNEL__ defined on the command line */
/* Requires __SMP__    defined on the command line for SMP systems */

#define _NO_VERSION_
#include <linux/module.h>  /* Module support */
#include <linux/version.h> /* Kernel version */
#include <linux/fs.h>            /* File operations (fops) defines */
#include <linux/ioport.h>        /* Memory/device locking macros   */
#include <linux/mm.h>

#if defined( CONFIG_BPA2)
#include <linux/bpa2.h>
#else
#if defined( CONFIG_BIGPHYS_AREA)
#include <linux/bigphysarea.h>
#else
#error Update your Kernel config to include BigPhys Area.
#endif
#endif

#include <linux/errno.h>         /* Defines standard error codes */

#include <asm/uaccess.h>         /* User space access routines */

#include <linux/sched.h>         /* User privilages (capabilities) */

#include "stpti4_ioctl_types.h"    /* Module specific types */

#include "stpti4_ioctl.h"          /* Defines ioctl numbers */


#include "stpti.h"	 /* A STAPI ioctl driver. */
#define STPTI_NOINLINE_MEMGET
#include "memget.h"

#include "stpti4_ioctl_cfg.h"  /* PTI specific configurations. */

#include "pti_linux.h" /* Internal Linux event fuctions */


/*** PROTOTYPES **************************************************************/


#include "stpti4_ioctl_fops.h"
#include "stpti4_ioctl_utils.h"


static void *uvirt_to_kvirt(struct mm_struct *mm, unsigned long va);

/* Imports */
extern PTI_Device_Info PTI_Devices[STPTI_CORE_COUNT];

/*** METHODS *****************************************************************/

/*=============================================================================

   stpti4_ioctl_ioctl

   Handle any device specific calls.

   The commands constants are defined in 'stpti4_ioctl.h'.

  ===========================================================================*/

/* Returns the index of the address passed.
   If not found returns -1

   This function allows the Address to be either an index or an address.
   If the value is less than the core count then it is an index.
   If it is >= core count then it is an adddress.

   If the address does not match any know address then it returns -1.
*/
int stpti4_ioctl_findindex(U32 addr)
{
    int Index = -1;
    int i;

    if( addr < STPTI_CORE_COUNT )
         Index = (int)addr;
    else{
        for(i = 0;i<STPTI_CORE_COUNT;i++){
            if( PTI_Devices[i].Address == addr ){
                Index = i;
                break;
            }
        }
    }

    return Index;
}

int stpti4_ioctl_ioctl  (struct inode *node, struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err = 0;

    /* This union is used for Parameter passing between User Space and Kernel Space */
    /* As there are a large number of these structures, by making it a union we are saving stack
       space when optimisation is turned off (when doing debug). */
    union UserParams_u {
        STPTI_DescAllocate_t DescAllocate;
        STPTI_DescAssociate_t DescAssociate;
        STPTI_DescDeAllocate_t DescDeAllocate;
        STPTI_DescDisAssociate_t DescDisAssociate;
        STPTI_DescSetKeys_t DescSetKeys;
        STPTI_GetArrivalTime_t GetArrivalTime;
        STPTI_Ioctl_AlternateOutputSetDefaultAction_t AlternateOutputSetDefaultAction;
        STPTI_Ioctl_BufferAllocateManual_t BufferAllocateManual;
        STPTI_Ioctl_BufferAllocate_t BufferAllocate;
        STPTI_Ioctl_BufferDeallocate_t BufferDeallocate;
        STPTI_Ioctl_BufferExtractData_t BufferExtractData;
        STPTI_Ioctl_BufferExtractPartialPesPacketData_t BufferExtractPartialPesPacketData;
        STPTI_Ioctl_BufferExtractPesPacketData_t BufferExtractPesPacketData;
        STPTI_Ioctl_BufferExtractSectionData_t BufferExtractSectionData;
        STPTI_Ioctl_BufferExtractTSHeaderData_t BufferExtractTSHeaderData;
        STPTI_Ioctl_BufferFlush_t BufferFlush;
        STPTI_Ioctl_BufferGetFreeLength_t BufferGetFreeLength;
        STPTI_Ioctl_BufferGetWritePointer_t BufferGetWritePointer;
        STPTI_Ioctl_BufferPacketCount_t BufferPacketCount;
        STPTI_Ioctl_BufferLevelSignalDisable_t BufferLevelSignalDisable;
        STPTI_Ioctl_BufferLevelSignalEnable_t BufferLevelSignalEnable;
        STPTI_Ioctl_BufferLinkToCdFifo_t BufferLinkToCdFifo;
        STPTI_Ioctl_BufferReadNBytes_t BufferReadNBytes;
        STPTI_Ioctl_BufferReadPartialPesPacket_t BufferReadPartialPesPacket;
        STPTI_Ioctl_BufferReadPes_t BufferReadPes;
        STPTI_Ioctl_BufferReadSection_t BufferReadSection;
        STPTI_Ioctl_BufferRead_t BufferRead;
        STPTI_Ioctl_BufferSetMultiPacket_t BufferSetMultiPacket;
        STPTI_Ioctl_BufferSetOverflowControl_t BufferSetOverflowControl;
        STPTI_Ioctl_BufferSetReadPointer_t BufferSetReadPointer;
        STPTI_Ioctl_BufferTestForData_t BufferTestForData;
        STPTI_Ioctl_BufferUnLink_t BufferUnLink;
#ifdef STPTI_CAROUSEL_SUPPORT
        STPTI_Ioctl_CarouselAllocate_t CarouselAllocate;
        STPTI_Ioctl_CarouselDeallocate_t CarouselDeallocate;
        STPTI_Ioctl_CarouselEntryAllocate_t CarouselEntryAllocate;
        STPTI_Ioctl_CarouselEntryDeallocate_t CarouselEntryDeallocate;
        STPTI_Ioctl_CarouselEntryLoad_t CarouselEntryLoad;
        STPTI_Ioctl_CarouselGetCurrentEntry_t CarouselGetCurrentEntry;
        STPTI_Ioctl_CarouselInsertEntry_t CarouselInsertEntry;
        STPTI_Ioctl_CarouselInsertTimedEntry_t CarouselInsertTimedEntry;
        STPTI_Ioctl_CarouselLinkToBuffer_t CarouselLinkToBuffer;
        STPTI_Ioctl_CarouselLock_t CarouselLock;
        STPTI_Ioctl_CarouselRemoveEntry_t CarouselRemoveEntry;
        STPTI_Ioctl_CarouselSetAllowOutput_t CarouselSetAllowOutput;
        STPTI_Ioctl_CarouselSetBurstMode_t CarouselSetBurstMode;
        STPTI_Ioctl_CarouselUnlinkBuffer_t CarouselUnLinkBuffer;
        STPTI_Ioctl_CarouselUnLock_t CarouselUnlock;
#endif /* #ifdef STPTI_CAROUSEL_SUPPORT */
        STPTI_Ioctl_Close_t Close;
        STPTI_Ioctl_Debug_t Debug;
        STPTI_Ioctl_DisableScramblingEvents_t DisableScramblingEvents;
        STPTI_Ioctl_DmaFree_t DmaFree;
        STPTI_Ioctl_DumpInputTS_t DumpInputTS;
        STPTI_Ioctl_EnableScramblingEvents_t EnableScramblingEvents;
        STPTI_Ioctl_ErrorEvent_t ErrorEvent;
        STPTI_Ioctl_FilterAllocate_t FilterAllocate;
        STPTI_Ioctl_FilterAssociate_t FilterAssociate;
        STPTI_Ioctl_FilterDeallocate_t FilterDeallocate;
        STPTI_Ioctl_FilterDisassociate_t FilterDisassociate;
        STPTI_Ioctl_FilterSetMatchAction_t FilterSetMatchAction;
        STPTI_Ioctl_FilterSet_t FilterSet;
        STPTI_Ioctl_FiltersFlush_t FiltersFlush;
#if defined (STPTI_FRONTEND_HYBRID)
        STPTI_Ioctl_FrontendAllocate_t FrontendAllocate;
        STPTI_Ioctl_FrontendDeallocate_t FrontendDeallocate;
        STPTI_Ioctl_FrontendGetParams_t FrontendGetParams;
        STPTI_Ioctl_FrontendGetStatus_t FrontendGetStatus;
        STPTI_Ioctl_FrontendInjectData_t FrontendInjectData;
        STPTI_Ioctl_FrontendLinkToPTI_t FrontendLinkToPTI;
        STPTI_Ioctl_FrontendReset_t FrontendReset;
        STPTI_Ioctl_FrontendSetParams_t FrontendSetParams;
        STPTI_Ioctl_FrontendUnLink_t FrontendUnLink;
#endif /* #if defined (STPTI_FRONTEND_HYBRID) ... #endif */
        STPTI_Ioctl_GetBuffersFromSlot_t GetBuffersFromSlot;
        STPTI_Ioctl_GetCapability_t GetCapability;
        STPTI_Ioctl_GetCurrentPTITimer_t GetCurrentPTITimer;
        STPTI_Ioctl_GetGlobalPacketCount_t GetGlobalPacketCount;
        STPTI_Ioctl_GetInputPacketCount_t GetInputPacketCount;
        STPTI_Ioctl_GetPacketErrorCount_t GetPacketErrorCount;
        STPTI_Ioctl_GetPresentationSTC_t GetPresentationSTC;
        STPTI_Ioctl_GetSWCDFifoCfg_t GetSWCDFifoCfg;
        STPTI_Ioctl_HardwareData_t HardwareData;
        STPTI_Ioctl_HardwareFIFOLevels_t HardwareFIFOLevels;
#ifndef STPTI_NO_INDEX_SUPPORT
        STPTI_Ioctl_IndexAllocate_t IndexAllocate;
        STPTI_Ioctl_IndexAssociate_t IndexAssociate;
        STPTI_Ioctl_IndexDeallocate_t IndexDeallocate;
        STPTI_Ioctl_IndexDisassociate_t IndexDisassociate;
        STPTI_Ioctl_IndexReset_t IndexReset;
        STPTI_Ioctl_IndexSet_t IndexSet;
        STPTI_Ioctl_IndexChain_t IndexChain;
        STPTI_Ioctl_IndexStart_t IndexStart;
        STPTI_Ioctl_IndexStop_t IndexStop;
#endif /* STPTI_NO_INDEX_SUPPORT */
        STPTI_Ioctl_Init_t Init;
        STPTI_Ioctl_ModifyGlobalFilterState_t ModifyGlobalFilterState;
        STPTI_Ioctl_ModifySyncLockAndDrop_t ModifySyncLockAndDrop;
        STPTI_Ioctl_Open_t Open;
        STPTI_Ioctl_ObjectDisableFeature_t DisableFeature;
        STPTI_Ioctl_ObjectEnableFeature_t EnableFeature;
        STPTI_Ioctl_PidQuery_t PidQuery;
        STPTI_Ioctl_PrintDebug_t PrintDebug;
        STPTI_Ioctl_SendEvent_t SendEvent;
        STPTI_Ioctl_SetDiscardParams_t SetDiscardParams;
        STPTI_Ioctl_SetStreamID_t SetStreamID;
        STPTI_Ioctl_SetSystemKey_t SetSystemKey;
        STPTI_Ioctl_SignalAbort_t SignalAbort;
        STPTI_Ioctl_SignalAllocate_t SignalAllocate;
        STPTI_Ioctl_SignalAssociateBuffer_t SignalAssociateBuffer;
        STPTI_Ioctl_SignalDeallocate_t SignalDeallocate;
        STPTI_Ioctl_SignalDisassociateBuffer_t SignalDisassociateBuffer;
        STPTI_Ioctl_SignalWaitBuffer_t SignalWaitBuffer;
        STPTI_Ioctl_SlotAllocate_t SlotAllocate;
        STPTI_Ioctl_SlotClearPid_t SlotClearPid;
        STPTI_Ioctl_SlotDeallocate_t SlotDeallocate;
        STPTI_Ioctl_SlotDescramblingControl_t SlotDescramblingControl;
        STPTI_Ioctl_SlotLinkToBuffer_t SlotLinkToBuffer;
        STPTI_Ioctl_SlotLinkToRecordBuffer_t SlotLinkToRecordBuffer;
        STPTI_Ioctl_SlotPacketCount_t SlotPacketCount;
        STPTI_Ioctl_SlotQuery_t SlotQuery;
        STPTI_Ioctl_SlotSetAlternateOutputAction_t SlotSetAlternateOutputAction;
        STPTI_Ioctl_SlotSetCCControl_t SlotSetCCControl;
        STPTI_Ioctl_SlotSetCorruptionParams_t SlotSetCorruptionParams;
        STPTI_Ioctl_SlotSetPid_t SlotSetPid;
        STPTI_Ioctl_SlotState_t SlotState;
        STPTI_Ioctl_SlotUnLinkRecordBuffer_t SlotUnLinkRecordBuffer;
        STPTI_Ioctl_SlotUnLink_t SlotUnLink;
        STPTI_Ioctl_SlotUpdatePid_t SlotUpdatePid;
        STPTI_Ioctl_SubscribeEvent_t SubscribeEvent;
        STPTI_Ioctl_Term_t Term;
        STPTI_Ioctl_UnsubscribeEvent_t UnsubscribeEvent;
        STPTI_Ioctl_WaitEventList_t WaitEventList;
        STPTI_Ioctl_WaitForEvent_t WaitForEvent;
    } UserParams;


    if (_IOC_TYPE(cmd) != STPTI_IOCTL_MAGIC_NUMBER) {

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
        case STPTI_IOC_INIT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INIT\n"));
            {
                unsigned int pti_index;

                if ((err = copy_from_user(&UserParams.Init, (STPTI_Ioctl_Init_t *)arg, sizeof(STPTI_Ioctl_Init_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                /* Determine the index */
                pti_index = stpti4_ioctl_findindex( (unsigned int)UserParams.Init.InitParams.TCDeviceAddress_p );
                if( pti_index != -1 ){

                    UserParams.Init.ErrorCode = ST_NO_ERROR;
                    
                    /* The pain of backward compatibility.   The LINUX PTI was developed to support only */
                    /* one loader.  However in our regression tests it is useful to change loaders on    */
                    /* the fly. */
                    if(UserParams.Init.ForceLoader >= 0)
                    {
                        UserParams.Init.ErrorCode = STPTI_TEST_ForceLoader(UserParams.Init.ForceLoader);
                    }

                    if( UserParams.Init.ErrorCode == ST_NO_ERROR )
                    {
                        /* Setup what the driver knows */
                        UserParams.Init.InitParams.Device = PTI_Devices[pti_index].Architecture;
                        UserParams.Init.InitParams.Partition_p = (void*)NULL;
                        UserParams.Init.InitParams.TCDeviceAddress_p = (STPTI_DevicePtr_t) PTI_Devices[pti_index].Address;      /* Physical address of PTI */
                        UserParams.Init.InitParams.TCLoader_p = PTI_Devices[pti_index].TCLoader_p;
                        UserParams.Init.InitParams.NCachePartition_p	=  (void*)NULL;
                        UserParams.Init.InitParams.InterruptNumber = PTI_Devices[pti_index].Interrupt;

                        UserParams.Init.ErrorCode = STPTI_Init(UserParams.Init.DeviceName, &( UserParams.Init.InitParams ));
                    }
                }
                else
                    UserParams.Init.ErrorCode = ST_ERROR_BAD_PARAMETER;

                if((err = copy_to_user((STPTI_Ioctl_Init_t*)arg, &UserParams.Init, sizeof(STPTI_Ioctl_Init_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_TERM:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_TERM\n"));
            {
                if ((err = copy_from_user(&UserParams.Term, (STPTI_Ioctl_Term_t *)arg, sizeof(STPTI_Ioctl_Term_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.Term.ErrorCode = STPTI_Term(UserParams.Term.DeviceName, &( UserParams.Term.TermParams ));

                if((err = copy_to_user((STPTI_Ioctl_Term_t*)arg, &UserParams.Term, sizeof(STPTI_Ioctl_Term_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_OPEN:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_OPEN\n"));
            {
                if ((err = copy_from_user(&UserParams.Open, (STPTI_Ioctl_Open_t *)arg, sizeof(STPTI_Ioctl_Open_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                /* Not used but setup dummy values. */
                UserParams.Open.OpenParams.DriverPartition_p = (void*)NULL;
                UserParams.Open.OpenParams.NonCachedPartition_p = (void*)NULL;

                UserParams.Open.ErrorCode = STPTI_Open(UserParams.Open.DeviceName, &( UserParams.Open.OpenParams ), &(UserParams.Open.Handle));

                if((err = copy_to_user((STPTI_Ioctl_Open_t*)arg, &UserParams.Open, sizeof(STPTI_Ioctl_Open_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CLOSE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CLOSE\n"));
            {
                if ((err = copy_from_user(&UserParams.Close, (STPTI_Ioctl_Close_t *)arg, sizeof(STPTI_Ioctl_Close_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.Close.ErrorCode = STPTI_Close(UserParams.Close.Handle);

                if((err = copy_to_user((STPTI_Ioctl_Close_t*)arg, &UserParams.Close, sizeof(STPTI_Ioctl_Close_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotAllocate, (STPTI_Ioctl_SlotAllocate_t*)arg, sizeof(STPTI_Ioctl_SlotAllocate_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotAllocate.Error = STPTI_SlotAllocate( UserParams.SlotAllocate.SessionHandle, &UserParams.SlotAllocate.SlotHandle, &UserParams.SlotAllocate.SlotData );

                if((err = copy_to_user( (STPTI_Ioctl_SlotAllocate_t*)arg, &UserParams.SlotAllocate, sizeof(STPTI_Ioctl_SlotAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotDeallocate, (STPTI_Ioctl_SlotDeallocate_t*)arg, sizeof(STPTI_Ioctl_SlotDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotDeallocate.Error = STPTI_SlotDeallocate( UserParams.SlotDeallocate.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SlotDeallocate_t*)arg, &UserParams.SlotDeallocate, sizeof(STPTI_Ioctl_SlotDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTSETPID:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTSETPID\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotSetPid, (STPTI_Ioctl_SlotSetPid_t*)arg, sizeof(STPTI_Ioctl_SlotSetPid_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotSetPid.Error = STPTI_SlotSetPid( UserParams.SlotSetPid.SlotHandle, UserParams.SlotSetPid.Pid );

                if((err = copy_to_user( (STPTI_Ioctl_SlotSetPid_t*)arg, &UserParams.SlotSetPid, sizeof(STPTI_Ioctl_SlotSetPid_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTCLEARPID:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTCLEARPID\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotClearPid, (STPTI_Ioctl_SlotClearPid_t*)arg, sizeof(STPTI_Ioctl_SlotClearPid_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotClearPid.Error = STPTI_SlotClearPid( UserParams.SlotClearPid.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SlotClearPid_t*)arg, &UserParams.SlotClearPid, sizeof(STPTI_Ioctl_SlotClearPid_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferAllocate, (STPTI_Ioctl_BufferAllocate_t*)arg, sizeof(STPTI_Ioctl_BufferAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferAllocate.Error = STPTI_BufferAllocate( UserParams.BufferAllocate.SessionHandle,
                                                      UserParams.BufferAllocate.RequiredSize,
                                                      UserParams.BufferAllocate.NumberOfPacketsInMultiPacket,
                                                      &UserParams.BufferAllocate.BufferHandle );

               if((err = copy_to_user( (STPTI_Ioctl_BufferAllocate_t*)arg, &UserParams.BufferAllocate, sizeof(STPTI_Ioctl_BufferAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_BUFFERDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferDeallocate, (STPTI_Ioctl_BufferDeallocate_t*)arg, sizeof(STPTI_Ioctl_BufferDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferDeallocate.Error = STPTI_BufferDeallocate( UserParams.BufferDeallocate.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_BufferDeallocate_t*)arg, &UserParams.BufferDeallocate, sizeof(STPTI_Ioctl_BufferDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTLINKTOBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTLINKTOBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotLinkToBuffer, (STPTI_Ioctl_SlotLinkToBuffer_t*)arg, sizeof(STPTI_Ioctl_SlotLinkToBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotLinkToBuffer.Error = STPTI_SlotLinkToBuffer( UserParams.SlotLinkToBuffer.Slot, UserParams.SlotLinkToBuffer.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SlotLinkToBuffer_t*)arg, &UserParams.SlotLinkToBuffer, sizeof(STPTI_Ioctl_SlotLinkToBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTUNLINK:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTUNLINK\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotUnLink, (STPTI_Ioctl_SlotUnLink_t*)arg, sizeof(STPTI_Ioctl_SlotUnLink_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotUnLink.Error = STPTI_SlotUnLink( UserParams.SlotUnLink.Slot );

                if((err = copy_to_user( (STPTI_Ioctl_SlotUnLink_t*)arg, &UserParams.SlotUnLink, sizeof(STPTI_Ioctl_SlotUnLink_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERLINKTOCDFIFO:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERLINKTOCDFIFO\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferLinkToCdFifo, (STPTI_Ioctl_BufferLinkToCdFifo_t*)arg, sizeof(STPTI_Ioctl_BufferLinkToCdFifo_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferLinkToCdFifo.Error = STPTI_BufferLinkToCdFifo( UserParams.BufferLinkToCdFifo.BufferHandle, &UserParams.BufferLinkToCdFifo.CdFifoParams );

                if((err = copy_to_user( (STPTI_Ioctl_BufferLinkToCdFifo_t*)arg, &UserParams.BufferLinkToCdFifo, sizeof(STPTI_Ioctl_BufferLinkToCdFifo_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERUNLINK:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERUNLINK\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferUnLink, (STPTI_Ioctl_BufferUnLink_t*)arg, sizeof(STPTI_Ioctl_BufferUnLink_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferUnLink.Error = STPTI_BufferUnLink( UserParams.BufferUnLink.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_BufferUnLink_t*)arg, &UserParams.BufferUnLink, sizeof(STPTI_Ioctl_BufferUnLink_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_ENABLEERROREVENT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_ENABLEERROREVENT\n"));
            {
                if ((err = copy_from_user(&UserParams.ErrorEvent, (STPTI_Ioctl_ErrorEvent_t *)arg, sizeof(STPTI_Ioctl_ErrorEvent_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorEvent.Error = STPTI_EnableErrorEvent(UserParams.ErrorEvent.DeviceName, UserParams.ErrorEvent.EventName);
                if((err = copy_to_user((STPTI_Ioctl_ErrorEvent_t*)arg, &UserParams.ErrorEvent, sizeof(STPTI_Ioctl_ErrorEvent_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_DISABLEERROREVENT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DISABLEERROREVENT\n"));
            {
                if ((err = copy_from_user(&UserParams.ErrorEvent, (STPTI_Ioctl_ErrorEvent_t *)arg, sizeof(STPTI_Ioctl_ErrorEvent_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.ErrorEvent.Error = STPTI_DisableErrorEvent(UserParams.ErrorEvent.DeviceName, UserParams.ErrorEvent.EventName);
                if((err = copy_to_user((STPTI_Ioctl_ErrorEvent_t*)arg, &UserParams.ErrorEvent, sizeof(STPTI_Ioctl_ErrorEvent_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_GETSWCDFIFOCFG:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETSWCDFIFOCFG\n"));
            {
                if ((err = copy_from_user(&UserParams.GetSWCDFifoCfg, (STPTI_Ioctl_GetSWCDFifoCfg_t *)arg, sizeof(STPTI_Ioctl_GetSWCDFifoCfg_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                if ((err = put_user( (void*)STPTI_BufferGetWritePointer, UserParams.GetSWCDFifoCfg.GetWritePtrFn ) )){
                    /* Invalid user space address */
                    goto fail;
                }

                if ((err = put_user( (void*)STPTI_BufferSetReadPointer, UserParams.GetSWCDFifoCfg.SetReadPtrFn ) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetSWCDFifoCfg.Error = ST_NO_ERROR;

                if((err = copy_to_user((STPTI_Ioctl_GetSWCDFifoCfg_t*)arg, &UserParams.GetSWCDFifoCfg, sizeof(STPTI_Ioctl_GetSWCDFifoCfg_t)) )) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERALLOCATEMANUAL:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERALLOCATEMANUAL\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferAllocateManual, (STPTI_Ioctl_BufferAllocateManual_t*)arg, sizeof(STPTI_Ioctl_BufferAllocateManual_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferAllocateManual.Error = STPTI_BufferAllocateManual( UserParams.BufferAllocateManual.SessionHandle,
                                                               UserParams.BufferAllocateManual.Base_p,
                                                               UserParams.BufferAllocateManual.RequiredSize,
                                                               UserParams.BufferAllocateManual.NumberOfPacketsInMultiPacket,
                                                              &UserParams.BufferAllocateManual.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_BufferAllocateManual_t*)arg, &UserParams.BufferAllocateManual, sizeof(STPTI_Ioctl_BufferAllocateManual_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_BUFFERALLOCATEMANUALUSER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERALLOCATEMANUALUSER\n"));
            {
                struct vm_area_struct *vma;

                if ((err = copy_from_user( &UserParams.BufferAllocateManual, (STPTI_Ioctl_BufferAllocateManual_t*)arg, sizeof(STPTI_Ioctl_BufferAllocateManual_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                down_read(&current->mm->mmap_sem);
                vma = find_vma(current->mm, (U32)UserParams.BufferAllocateManual.Base_p);

                /* Allocate if the buffer is mapped as a contigouous buffer */
                if (vma && ((U32)UserParams.BufferAllocateManual.Base_p + UserParams.BufferAllocateManual.RequiredSize <= vma->vm_end))
                {
                    void *kaddr = uvirt_to_kvirt(vma->vm_mm, vma->vm_start);

                    if (kaddr != (void*)-1)
                    {
#if defined(STPTI_LEGACY_LINUX_SWCDFIFO)
                        /* We are using Physical Addresses */
                        U8* PhysAddr = (U8*)STOS_VirtToPhys(kaddr) + ((U32)UserParams.BufferAllocateManual.Base_p - vma->vm_start);

                        UserParams.BufferAllocateManual.Error = STPTI_BufferAllocateManual( UserParams.BufferAllocateManual.SessionHandle,
                                                                       PhysAddr,
                                                                       UserParams.BufferAllocateManual.RequiredSize,
                                                                       UserParams.BufferAllocateManual.NumberOfPacketsInMultiPacket,
                                                                      &UserParams.BufferAllocateManual.BufferHandle );
                        if (UserParams.BufferAllocateManual.Error == ST_NO_ERROR)
                        {
                            U32 AddrDiff = (U32)UserParams.BufferAllocateManual.Base_p - (U32)PhysAddr;
                            UserParams.BufferAllocateManual.Error = stpti_SetUserSpaceMap(UserParams.BufferAllocateManual.BufferHandle, AddrDiff);
                        }
#else
                        /* We are using Virtual Addresses */
                        U8* KVirtAddr = (U8*)kaddr + ((U32)UserParams.BufferAllocateManual.Base_p - vma->vm_start);

                        UserParams.BufferAllocateManual.Error = STPTI_BufferAllocateManual( UserParams.BufferAllocateManual.SessionHandle,
                                                                       KVirtAddr,
                                                                       UserParams.BufferAllocateManual.RequiredSize,
                                                                       UserParams.BufferAllocateManual.NumberOfPacketsInMultiPacket,
                                                                      &UserParams.BufferAllocateManual.BufferHandle );
                        if (UserParams.BufferAllocateManual.Error == ST_NO_ERROR)
                        {
                            U32 AddrDiff = (U32)UserParams.BufferAllocateManual.Base_p - (U32)KVirtAddr;
                            UserParams.BufferAllocateManual.Error = stpti_SetUserSpaceMap(UserParams.BufferAllocateManual.BufferHandle, AddrDiff);
                        }
#endif

                    }
                    else
                    {
                        UserParams.BufferAllocateManual.Error = ST_ERROR_NO_MEMORY;
                    }
                }
                else
                {
                    UserParams.BufferAllocateManual.Error = ST_ERROR_NO_MEMORY;
                }

                up_read(&current->mm->mmap_sem);

                if((err = copy_to_user( (STPTI_Ioctl_BufferAllocateManual_t*)arg, &UserParams.BufferAllocateManual, sizeof(STPTI_Ioctl_BufferAllocateManual_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_DMAFREE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DMAFREE\n"));
            {
                struct vm_area_struct *vma;

                if ((err = copy_from_user( &UserParams.DmaFree, (STPTI_Ioctl_DmaFree_t*)arg, sizeof(STPTI_Ioctl_DmaFree_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.DmaFree.Error = ST_NO_ERROR;
                down_write(&current->mm->mmap_sem);
                vma = find_vma(current->mm, (U32)UserParams.DmaFree.Buffer_p);

                if (vma)
                {
                    struct mm_struct *mm = vma->vm_mm;
                    void *kaddr = uvirt_to_kvirt(mm, vma->vm_start);

                    if (kaddr != (void*)-1)
                    {
                    	do_munmap(mm, vma->vm_start, vma->vm_end - vma->vm_start);
#ifdef CONFIG_BIGPHYS_AREA
                    	if(vma->vm_pgoff) /* used as a flag to indicate bigphysarea */
                            bigphysarea_free_pages( kaddr );
                        else
#endif
                            kfree( kaddr );
                    }
                    else
                        UserParams.DmaFree.Error = ST_ERROR_NO_MEMORY;
                }
                else
                    UserParams.DmaFree.Error = ST_ERROR_NO_MEMORY;

                up_write(&current->mm->mmap_sem);

                if((err = copy_to_user( (STPTI_Ioctl_DmaFree_t*)arg, &UserParams.DmaFree, sizeof(STPTI_Ioctl_DmaFree_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERSETOVERFLOWCONTROL:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERSETOVERFLOWCONTROL\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferSetOverflowControl, (STPTI_Ioctl_BufferSetOverflowControl_t*)arg, sizeof(STPTI_Ioctl_BufferSetOverflowControl_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferSetOverflowControl.Error = STPTI_BufferSetOverflowControl( UserParams.BufferSetOverflowControl.BufferHandle,UserParams.BufferSetOverflowControl.IgnoreOverflow );

               if((err = copy_to_user( (STPTI_Ioctl_BufferSetOverflowControl_t*)arg, &UserParams.BufferSetOverflowControl, sizeof(STPTI_Ioctl_BufferSetOverflowControl_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_GETINPUTPACKETCOUNT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETINPUTPACKETCOUNT\n"));
            {
                if ((err = copy_from_user( &UserParams.GetInputPacketCount, (STPTI_Ioctl_GetInputPacketCount_t*)arg, sizeof(STPTI_Ioctl_GetInputPacketCount_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetInputPacketCount.Error = STPTI_GetInputPacketCount( UserParams.GetInputPacketCount.DeviceName,&UserParams.GetInputPacketCount.Count );

               if((err = copy_to_user( (STPTI_Ioctl_GetInputPacketCount_t*)arg, &UserParams.GetInputPacketCount, sizeof(STPTI_Ioctl_GetInputPacketCount_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_SLOTPACKETCOUNT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTPACKETCOUNT\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotPacketCount, (STPTI_Ioctl_SlotPacketCount_t*)arg, sizeof(STPTI_Ioctl_SlotPacketCount_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotPacketCount.Error = STPTI_SlotPacketCount( UserParams.SlotPacketCount.SlotHandle,&UserParams.SlotPacketCount.Count );

               if((err = copy_to_user( (STPTI_Ioctl_SlotPacketCount_t*)arg, &UserParams.SlotPacketCount, sizeof(STPTI_Ioctl_SlotPacketCount_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;

        case STPTI_IOC_GETREVISION:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETREVISION\n"));
            {
                if((err = copy_to_user( (char*)arg, STPTI_GetRevision(), 31 ))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_SUBSCRIBEEVENT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SUBSCRIBEEVENT\n"));
            {
                if ((err = copy_from_user( &UserParams.SubscribeEvent, (STPTI_Ioctl_SubscribeEvent_t*)arg, sizeof(STPTI_Ioctl_SubscribeEvent_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SubscribeEvent.Error = STPTI_OSLINUX_SubscribeEvent( UserParams.SubscribeEvent.SessionHandle,UserParams.SubscribeEvent.Event_ID );

               if((err = copy_to_user( (STPTI_Ioctl_SubscribeEvent_t*)arg, &UserParams.SubscribeEvent, sizeof(STPTI_Ioctl_SubscribeEvent_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;

        case STPTI_IOC_UNSUBSCRIBEEVENT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_UNSUBSCRIBEEVENT\n"));
            {
                if ((err = copy_from_user( &UserParams.UnsubscribeEvent, (STPTI_Ioctl_UnsubscribeEvent_t*)arg, sizeof(STPTI_Ioctl_UnsubscribeEvent_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.UnsubscribeEvent.Error = STPTI_OSLINUX_UnsubscribeEvent( UserParams.UnsubscribeEvent.SessionHandle,UserParams.UnsubscribeEvent.Event_ID );

                if((err = copy_to_user( (STPTI_Ioctl_UnsubscribeEvent_t*)arg, &UserParams.UnsubscribeEvent, sizeof(STPTI_Ioctl_UnsubscribeEvent_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SENDEVENT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SENDEVENT\n"));
            {
                if ((err = copy_from_user( &UserParams.SendEvent, (STPTI_Ioctl_SendEvent_t*)arg, sizeof(STPTI_Ioctl_SendEvent_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SendEvent.Error = STPTI_OSLINUX_QueueSessionEvent( UserParams.SendEvent.SessionHandle,UserParams.SendEvent.Event_ID, &UserParams.SendEvent.EventData );

                if((err = copy_to_user( (STPTI_Ioctl_SendEvent_t*)arg, &UserParams.SendEvent, sizeof(STPTI_Ioctl_SendEvent_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_WAITFOREVENT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_WAITFOREVENT\n"));
            {
                if ((err = copy_from_user( &UserParams.WaitForEvent, (STPTI_Ioctl_WaitForEvent_t*)arg, sizeof(STPTI_Ioctl_WaitForEvent_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.WaitForEvent.Error = STPTI_OSLINUX_WaitForEvent( UserParams.WaitForEvent.SessionHandle,&UserParams.WaitForEvent.Event_ID, &UserParams.WaitForEvent.EventData );
                if(  UserParams.WaitForEvent.Error == STPTI_ERROR_SIGNAL_ABORTED ){
                    err = -ERESTARTSYS;
                    goto fail;
                }
                if((err = copy_to_user( (STPTI_Ioctl_WaitForEvent_t*)arg, &UserParams.WaitForEvent, sizeof(STPTI_Ioctl_WaitForEvent_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_TEST_RESETALLPTI:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_TEST_RESETALLPTI\n"));
            STPTI_TEST_ResetAllPti();/* Used by test harness */
            break;
        case STPTI_IOC_SIGNALALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SIGNALALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.SignalAllocate, (STPTI_Ioctl_SignalAllocate_t*)arg, sizeof(STPTI_Ioctl_SignalAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SignalAllocate.Error = STPTI_SignalAllocate(UserParams.SignalAllocate.SessionHandle, &UserParams.SignalAllocate.SignalHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SignalAllocate_t*)arg, &UserParams.SignalAllocate, sizeof(STPTI_Ioctl_SignalAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SIGNALASSOCIATEBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SIGNALASSOCIATEBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.SignalAssociateBuffer, (STPTI_Ioctl_SignalAssociateBuffer_t*)arg, sizeof(STPTI_Ioctl_SignalAssociateBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SignalAssociateBuffer.Error = STPTI_SignalAssociateBuffer( UserParams.SignalAssociateBuffer.SignalHandle,
                                                                UserParams.SignalAssociateBuffer.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SignalAssociateBuffer_t*)arg, &UserParams.SignalAssociateBuffer, sizeof(STPTI_Ioctl_SignalAssociateBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SIGNALDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SIGNALDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.SignalDeallocate, (STPTI_Ioctl_SignalDeallocate_t*)arg, sizeof(STPTI_Ioctl_SignalDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SignalDeallocate.Error = STPTI_SignalDeallocate(UserParams.SignalDeallocate.SignalHandle);

                if((err = copy_to_user( (STPTI_Ioctl_SignalDeallocate_t*)arg, &UserParams.SignalDeallocate, sizeof(STPTI_Ioctl_SignalDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SIGNALDISASSOCIATEBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SIGNALDISASSOCIATEBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.SignalDisassociateBuffer, (STPTI_Ioctl_SignalDisassociateBuffer_t*)arg, sizeof(STPTI_Ioctl_SignalDisassociateBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SignalDisassociateBuffer.Error = STPTI_SignalDisassociateBuffer( UserParams.SignalDisassociateBuffer.SignalHandle,
                                                                UserParams.SignalDisassociateBuffer.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SignalDisassociateBuffer_t*)arg, &UserParams.SignalDisassociateBuffer, sizeof(STPTI_Ioctl_SignalDisassociateBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SIGNALWAITBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SIGNALWAITBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.SignalWaitBuffer, (STPTI_Ioctl_SignalWaitBuffer_t*)arg, sizeof(STPTI_Ioctl_SignalWaitBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SignalWaitBuffer.Error = STPTI_SignalWaitBuffer( UserParams.SignalWaitBuffer.SignalHandle,
                                                           &UserParams.SignalWaitBuffer.BufferHandle,/* Returns with handle of buffer in this location. */
                                                           UserParams.SignalWaitBuffer.TimeoutMS );

                if((err = copy_to_user( (STPTI_Ioctl_SignalWaitBuffer_t*)arg, &UserParams.SignalWaitBuffer, sizeof(STPTI_Ioctl_SignalWaitBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SIGNALABORT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SIGNALABORT\n"));
            {
                if ((err = copy_from_user( &UserParams.SignalAbort, (STPTI_Ioctl_SignalAbort_t*)arg, sizeof(STPTI_Ioctl_SignalAbort_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SignalAbort.Error = STPTI_SignalAbort( UserParams.SignalAbort.SignalHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SignalAbort_t*)arg, &UserParams.SignalAbort, sizeof(STPTI_Ioctl_SignalAbort_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTQUERY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTQUERY\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotQuery, (STPTI_Ioctl_SlotQuery_t*)arg, sizeof(STPTI_Ioctl_SlotQuery_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotQuery.Error = STPTI_SlotQuery( UserParams.SlotQuery.SlotHandle,
                                                    &UserParams.SlotQuery.PacketsSeen,
                                                    &UserParams.SlotQuery.TransportScrambledPacketsSeen,
                                                    &UserParams.SlotQuery.PESScrambledPacketsSeen,
                                                    &UserParams.SlotQuery.Pid);

                if((err = copy_to_user( (STPTI_Ioctl_SlotQuery_t*)arg, &UserParams.SlotQuery, sizeof(STPTI_Ioctl_SlotQuery_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERFLUSH:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERFLUSH\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferFlush, (STPTI_Ioctl_BufferFlush_t*)arg, sizeof(STPTI_Ioctl_BufferFlush_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferFlush.Error = STPTI_BufferFlush( UserParams.BufferFlush.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_BufferFlush_t*)arg, &UserParams.BufferFlush, sizeof(STPTI_Ioctl_BufferFlush_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERGETFREELENGTH:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERGETFREELENGTH\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferGetFreeLength, (STPTI_Ioctl_BufferGetFreeLength_t*)arg, sizeof(STPTI_Ioctl_BufferGetFreeLength_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferGetFreeLength.Error = STPTI_BufferGetFreeLength( UserParams.BufferGetFreeLength.BufferHandle,
                                                              &UserParams.BufferGetFreeLength.FreeLength);

                if((err = copy_to_user( (STPTI_Ioctl_BufferGetFreeLength_t*)arg, &UserParams.BufferGetFreeLength, sizeof(STPTI_Ioctl_BufferGetFreeLength_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERREADPES:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERREADPES\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferReadPes, (STPTI_Ioctl_BufferReadPes_t*)arg, sizeof(STPTI_Ioctl_BufferReadPes_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferReadPes.Error = STPTI_BufferReadPes( UserParams.BufferReadPes.BufferHandle,
                                                        UserParams.BufferReadPes.Destination0_p,
                                                        UserParams.BufferReadPes.DestinationSize0,
                                                        UserParams.BufferReadPes.Destination1_p,
                                                        UserParams.BufferReadPes.DestinationSize1,
                                                        &UserParams.BufferReadPes.DataSize,
                                                        UserParams.BufferReadPes.DmaOrMemcpy);

                if((err = copy_to_user( (STPTI_Ioctl_BufferReadPes_t*)arg, &UserParams.BufferReadPes, sizeof(STPTI_Ioctl_BufferReadPes_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERLEVELSIGNALDISABLE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERLEVELSIGNALDISABLE\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferLevelSignalDisable, (STPTI_Ioctl_BufferLevelSignalDisable_t *)arg, sizeof(STPTI_Ioctl_BufferLevelSignalDisable_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferLevelSignalDisable.Error = STPTI_BufferLevelSignalDisable(UserParams.BufferLevelSignalDisable.BufferHandle);

                if((err = copy_to_user( (STPTI_Ioctl_BufferLevelSignalDisable_t *)arg, &UserParams.BufferLevelSignalDisable, sizeof(STPTI_Ioctl_BufferLevelSignalDisable_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERLEVELSIGNALENABLE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERLEVELSIGNALENABLE\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferLevelSignalEnable, (STPTI_Ioctl_BufferLevelSignalEnable_t *)arg, sizeof(STPTI_Ioctl_BufferLevelSignalEnable_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferLevelSignalEnable.Error = STPTI_BufferLevelSignalEnable(UserParams.BufferLevelSignalEnable.BufferHandle, UserParams.BufferLevelSignalEnable.BufferLevelThreshold);

                if((err = copy_to_user( (STPTI_Ioctl_BufferLevelSignalEnable_t *)arg, &UserParams.BufferLevelSignalEnable, sizeof(STPTI_Ioctl_BufferLevelSignalEnable_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERTESTFORDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERTESTFORDATA\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferTestForData, (STPTI_Ioctl_BufferTestForData_t*)arg, sizeof(STPTI_Ioctl_BufferTestForData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferTestForData.Error = STPTI_BufferTestForData( UserParams.BufferTestForData.BufferHandle,
                                                            &UserParams.BufferTestForData.BytesInBuffer );

                if((err = copy_to_user( (STPTI_Ioctl_BufferTestForData_t*)arg, &UserParams.BufferTestForData, sizeof(STPTI_Ioctl_BufferTestForData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_GETCAPABILITY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETCAPABILITY\n"));
            {
                if ((err = copy_from_user( &UserParams.GetCapability, (STPTI_Ioctl_GetCapability_t*)arg, sizeof(STPTI_Ioctl_GetCapability_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetCapability.Error = STPTI_GetCapability( UserParams.GetCapability.DeviceName, &UserParams.GetCapability.DeviceCapability );

                if((err = copy_to_user( (STPTI_Ioctl_GetCapability_t*)arg, &UserParams.GetCapability, sizeof(STPTI_Ioctl_GetCapability_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.FilterAllocate, (STPTI_Ioctl_FilterAllocate_t*)arg, sizeof(STPTI_Ioctl_FilterAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FilterAllocate.Error = STPTI_FilterAllocate( UserParams.FilterAllocate.SessionHandle, UserParams.FilterAllocate.FilterType, &UserParams.FilterAllocate.FilterObject );

                if((err = copy_to_user( (STPTI_Ioctl_FilterAllocate_t*)arg, &UserParams.FilterAllocate, sizeof(STPTI_Ioctl_FilterAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERASSOCIATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERASSOCIATE\n"));
            {
                if ((err = copy_from_user( &UserParams.FilterAssociate, (STPTI_Ioctl_FilterAssociate_t*)arg, sizeof(STPTI_Ioctl_FilterAssociate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FilterAssociate.Error = STPTI_FilterAssociate( UserParams.FilterAssociate.FilterHandle, UserParams.FilterAssociate.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FilterAssociate_t*)arg, &UserParams.FilterAssociate, sizeof(STPTI_Ioctl_FilterAssociate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.FilterDeallocate, (STPTI_Ioctl_FilterDeallocate_t*)arg, sizeof(STPTI_Ioctl_FilterDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FilterDeallocate.Error = STPTI_FilterDeallocate( UserParams.FilterDeallocate.FilterHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FilterDeallocate_t*)arg, &UserParams.FilterDeallocate, sizeof(STPTI_Ioctl_FilterDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERDISASSOCIATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERDISASSOCIATE\n"));
            {
                if ((err = copy_from_user( &UserParams.FilterDisassociate, (STPTI_Ioctl_FilterDisassociate_t*)arg, sizeof(STPTI_Ioctl_FilterDisassociate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FilterDisassociate.Error = STPTI_FilterDisassociate( UserParams.FilterDisassociate.FilterHandle, UserParams.FilterDisassociate.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FilterDisassociate_t*)arg, &UserParams.FilterDisassociate, sizeof(STPTI_Ioctl_FilterDisassociate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERSET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERSET\n"));
            {

                U8 Data[16];
                U8 Mask[16];
                U8 Mode[16];
                STPTI_TimeStamp_t PTSValueMax;
                STPTI_TimeStamp_t DTSValueMax;
                STPTI_TimeStamp_t PTSValueMin;
                STPTI_TimeStamp_t DTSValueMin;

                /* Clear the mask by default */
                memset(Mask, 0, sizeof(Mask));

                if ((err = copy_from_user( &UserParams.FilterSet, (STPTI_Ioctl_FilterSet_t*)arg, sizeof(STPTI_Ioctl_FilterSet_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                /* Do any deep copies */
                switch (UserParams.FilterSet.FilterData.FilterType)
                {

                    case STPTI_FILTER_TYPE_EMM_FILTER :                                 /* ECM Filters are Section Filters */
                    case STPTI_FILTER_TYPE_ECM_FILTER :                                 /* EMM Filters are Section Filters */
                    case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE :
                        /* 8 byte filters (Only relevent for NEG_MATCH_MODE) */
                        if ((UserParams.FilterSet.FilterData.u.SectionFilter.ModePattern_p != NULL) &&
                            (err = copy_from_user( Mode, (U8*)UserParams.FilterSet.FilterData.u.SectionFilter.ModePattern_p, 8) )){
                        /* Invalid user space address */
                            goto fail;
                        }
                        if (UserParams.FilterSet.FilterData.u.SectionFilter.ModePattern_p != NULL) {
                            UserParams.FilterSet.FilterData.u.SectionFilter.ModePattern_p = Mode;
                        }
                        /* Drop into STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE case below */

                    case STPTI_FILTER_TYPE_SECTION_FILTER :
                    case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE :
                        if ((UserParams.FilterSet.FilterData.FilterBytes_p != NULL) &&
                            (err = copy_from_user( Data, (U8*)UserParams.FilterSet.FilterData.FilterBytes_p, 8) )){
                        /* Invalid user space address */
                            goto fail;
                        }
                        if ((UserParams.FilterSet.FilterData.FilterMasks_p != NULL) &&
                            (err = copy_from_user( Mask, (U8*)UserParams.FilterSet.FilterData.FilterMasks_p, 8) )){
                        /* Invalid user space address */
                            goto fail;
                        }

                        break;


                    case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE :
                        /* 16 byte filters */
                        if ((UserParams.FilterSet.FilterData.FilterBytes_p != NULL) &&
                            (err = copy_from_user( Data, (U8*)UserParams.FilterSet.FilterData.FilterBytes_p, 16) )){
                        /* Invalid user space address */
                            goto fail;
                        }
                        if ((UserParams.FilterSet.FilterData.FilterMasks_p != NULL) &&
                            (err = copy_from_user( Mask, (U8*)UserParams.FilterSet.FilterData.FilterMasks_p, 16) )){
                        /* Invalid user space address */
                            goto fail;
                        }
                        break;

                    case STPTI_FILTER_TYPE_PES_STREAMID_FILTER :
                        if (UserParams.FilterSet.FilterData.u.PESFilter.PTSValueMax_p != NULL) {
                            if ((err = copy_from_user( &PTSValueMax, (U8*)UserParams.FilterSet.FilterData.u.PESFilter.PTSValueMax_p, sizeof(PTSValueMax))) ){
                            /* Invalid user space address */
                                goto fail;
                            }

                            UserParams.FilterSet.FilterData.u.PESFilter.PTSValueMax_p = &PTSValueMax;
                        }
                        break;
                        if (UserParams.FilterSet.FilterData.u.PESFilter.DTSValueMax_p != NULL) {
                            if ((err = copy_from_user( &DTSValueMax, (U8*)UserParams.FilterSet.FilterData.u.PESFilter.DTSValueMax_p, sizeof(DTSValueMax))) ){
                            /* Invalid user space address */
                                goto fail;
                            }

                            UserParams.FilterSet.FilterData.u.PESFilter.DTSValueMax_p = &DTSValueMax;
                        }
                        if (UserParams.FilterSet.FilterData.u.PESFilter.PTSValueMin_p != NULL) {
                            if ((err = copy_from_user( &PTSValueMin, (U8*)UserParams.FilterSet.FilterData.u.PESFilter.PTSValueMin_p, sizeof(PTSValueMin))) ){
                            /* Invalid user space address */
                                goto fail;
                            }

                            UserParams.FilterSet.FilterData.u.PESFilter.PTSValueMin_p = &PTSValueMin;
                        }
                        break;
                        if (UserParams.FilterSet.FilterData.u.PESFilter.DTSValueMin_p != NULL) {
                            if ((err = copy_from_user( &DTSValueMin, (U8*)UserParams.FilterSet.FilterData.u.PESFilter.DTSValueMin_p, sizeof(DTSValueMin))) ){
                            /* Invalid user space address */
                                goto fail;
                            }

                            UserParams.FilterSet.FilterData.u.PESFilter.DTSValueMin_p = &DTSValueMin;
                        }

                    case STPTI_FILTER_TYPE_TSHEADER_FILTER :
                    case STPTI_FILTER_TYPE_PES_FILTER :
                        /* Not currently supported */
                        break;

#ifdef STPTI_DC2_SUPPORT
                    case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER :
                    case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER :
                    case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER :
                        /* No deep copy needed */
                        break;
#endif
                }

                if (UserParams.FilterSet.FilterData.FilterBytes_p != NULL) UserParams.FilterSet.FilterData.FilterBytes_p = Data;
                if (UserParams.FilterSet.FilterData.FilterMasks_p != NULL) UserParams.FilterSet.FilterData.FilterMasks_p = Mask;

                UserParams.FilterSet.Error = STPTI_FilterSet( UserParams.FilterSet.FilterHandle, &UserParams.FilterSet.FilterData );

                if((err = copy_to_user( (STPTI_Ioctl_FilterSet_t*)arg, &UserParams.FilterSet, sizeof(STPTI_Ioctl_FilterSet_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERREADNBYTES:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERREADNBYTES\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferReadNBytes, (STPTI_Ioctl_BufferReadNBytes_t*)arg, sizeof(STPTI_Ioctl_BufferReadNBytes_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferReadNBytes.Error = STPTI_BufferReadNBytes( UserParams.BufferReadNBytes.BufferHandle,
                                                        UserParams.BufferReadNBytes.Destination0_p,
                                                        UserParams.BufferReadNBytes.DestinationSize0,
                                                        UserParams.BufferReadNBytes.Destination1_p,
                                                        UserParams.BufferReadNBytes.DestinationSize1,
                                                        &UserParams.BufferReadNBytes.DataSize,
                                                        UserParams.BufferReadNBytes.DmaOrMemcpy,
                                                        UserParams.BufferReadNBytes.BytesToCopy );

                if((err = copy_to_user( (STPTI_Ioctl_BufferReadNBytes_t*)arg, &UserParams.BufferReadNBytes, sizeof(STPTI_Ioctl_BufferReadNBytes_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTSETCCCONTROL:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTSETCCCONTROL\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotSetCCControl, (STPTI_Ioctl_SlotSetCCControl_t*)arg, sizeof(STPTI_Ioctl_SlotSetCCControl_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotSetCCControl.Error = STPTI_SlotSetCCControl( UserParams.SlotSetCCControl.SlotHandle, UserParams.SlotSetCCControl.SetOnOff );

                if((err = copy_to_user( (STPTI_Ioctl_SlotSetCCControl_t*)arg, &UserParams.SlotSetCCControl, sizeof(STPTI_Ioctl_SlotSetCCControl_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERREADSECTION:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERREADSECTION\n"));
            {
                STPTI_Filter_t *MatchedFilterList_p=NULL;

                if ((err = copy_from_user( &UserParams.BufferReadSection, (STPTI_Ioctl_BufferReadSection_t*)arg, sizeof(STPTI_Ioctl_BufferReadSection_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                if (UserParams.BufferReadSection.MaxLengthofFilterList != 0){
                    MatchedFilterList_p = (STPTI_Filter_t *)kmalloc(sizeof(STPTI_Filter_t)*UserParams.BufferReadSection.MaxLengthofFilterList, GFP_KERNEL);
                }

                UserParams.BufferReadSection.Error = STPTI_BufferReadSection( UserParams.BufferReadSection.BufferHandle,
                                                            MatchedFilterList_p,
                                                            UserParams.BufferReadSection.MaxLengthofFilterList,
                                                            &UserParams.BufferReadSection.NumOfFilterMatches,
                                                            &UserParams.BufferReadSection.CRCValid,
                                                            UserParams.BufferReadSection.Destination0_p,
                                                            UserParams.BufferReadSection.DestinationSize0,
                                                            UserParams.BufferReadSection.Destination1_p,
                                                            UserParams.BufferReadSection.DestinationSize1,
                                                            &UserParams.BufferReadSection.DataSize,
                                                            UserParams.BufferReadSection.DmaOrMemcpy);

                 if (UserParams.BufferReadSection.MaxLengthofFilterList != 0){
	            if ((err = copy_to_user(((STPTI_Ioctl_BufferReadSection_t*)arg)->MatchedFilterList, MatchedFilterList_p, sizeof(STPTI_Filter_t)*UserParams.BufferReadSection.MaxLengthofFilterList) )){
	            /* Invalid user space address */
		        kfree( MatchedFilterList_p );
	                goto fail;
	            }
		    kfree( MatchedFilterList_p );
		}

                if((err = copy_to_user( (STPTI_Ioctl_BufferReadSection_t*)arg, &UserParams.BufferReadSection, sizeof(STPTI_Ioctl_BufferReadSection_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_WAITEVENTLIST:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_WAITEVENTLIST\n"));
            {
                U32 *Events_p;
                void *EventData_p;

                if ((err = copy_from_user( &UserParams.WaitEventList, (STPTI_Ioctl_WaitEventList_t*)arg, sizeof(STPTI_Ioctl_WaitEventList_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                Events_p = (U32*)kmalloc(sizeof(U32)*UserParams.WaitEventList.NbEvents,GFP_KERNEL);
                assert( Events_p != NULL );

                EventData_p = kmalloc(UserParams.WaitEventList.EventDataSize,GFP_KERNEL);
                assert( Events_p != NULL );

                if ((err = copy_from_user(Events_p, ((STPTI_Ioctl_WaitEventList_t*)arg)->FuncData, sizeof(U32)*UserParams.WaitEventList.NbEvents) )){
                /* Invalid user space address */
                    kfree( Events_p );
                    kfree( EventData_p );
                    goto fail;
                }

                UserParams.WaitEventList.Error = STPTI_WaitEventList( UserParams.WaitEventList.SessionHandle,
                                                        UserParams.WaitEventList.NbEvents,
                                                        Events_p,
                                                        &UserParams.WaitEventList.Event_That_Has_Occurred,
                                                        UserParams.WaitEventList.EventDataSize,
                                                        EventData_p );

                if((err = copy_to_user( ((STPTI_Ioctl_WaitEventList_t*)arg)->FuncData+sizeof(U32)*UserParams.WaitEventList.NbEvents, EventData_p, UserParams.WaitEventList.EventDataSize ))) {
                    /* Invalid user space address */
                    kfree( Events_p );
                    kfree( EventData_p );
                    goto fail;
                }

                kfree( Events_p );
                kfree( EventData_p );

                if((err = copy_to_user( (STPTI_Ioctl_WaitEventList_t*)arg, &UserParams.WaitEventList, sizeof(STPTI_Ioctl_WaitEventList_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_DESCALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DESCALLOCATE\n"));
            {
              if ((err = copy_from_user( &UserParams.DescAllocate, (STPTI_DescAllocate_t*)arg, sizeof(STPTI_DescAllocate_t)) )){
                 /* Invalid user space address */
                 goto fail;
              }
              UserParams.DescAllocate.Error = STPTI_DescramblerAllocate(UserParams.DescAllocate.PtiHandle, &(UserParams.DescAllocate.DescramblerObject), UserParams.DescAllocate.DescramblerType);
              if ((err = copy_to_user( (STPTI_DescAllocate_t*)arg, &UserParams.DescAllocate, sizeof(STPTI_DescAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }

            }
            break;
            case STPTI_IOC_DESCASSOCIATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DESCASSOCIATE\n"));
            {
              if ((err = copy_from_user( &UserParams.DescAssociate, (STPTI_DescAssociate_t*)arg, sizeof(STPTI_DescAssociate_t)) )){
                 /* Invalid user space address */
                 goto fail;
              }
              UserParams.DescAssociate.Error = STPTI_DescramblerAssociate(UserParams.DescAssociate.DescramblerHandle, UserParams.DescAssociate.SlotOrPid);
              if ((err = copy_to_user( (STPTI_DescAssociate_t*)arg, &UserParams.DescAssociate, sizeof(STPTI_DescAssociate_t)))) {
                 /* Invalid user space address */
                  goto fail;
                }
            }
            break;
        case STPTI_IOC_DESCDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DESCDEALLOCATE\n"));
            {
              if ((err = copy_from_user( &UserParams.DescDeAllocate, (STPTI_DescDeAllocate_t*)arg, sizeof(STPTI_DescDeAllocate_t)) )){
                 /* Invalid user space address */
                 goto fail;
              }
              UserParams.DescDeAllocate.Error = STPTI_DescramblerDeallocate(UserParams.DescDeAllocate.DescramblerHandle);
              if((err = copy_to_user( (STPTI_DescDeAllocate_t*)arg, &UserParams.DescDeAllocate, sizeof(STPTI_DescDeAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

            case STPTI_IOC_DESCDISASSOCIATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DESCDISASSOCIATE\n"));
            {
              if ((err = copy_from_user( &UserParams.DescDisAssociate, (STPTI_DescDisAssociate_t*)arg, sizeof(STPTI_DescDisAssociate_t)) )){
              /* Invalid user space address */
                 goto fail;
              }
              UserParams.DescDisAssociate.Error = STPTI_DescramblerDisassociate(UserParams.DescDisAssociate.DescramblerHandle, UserParams.DescDisAssociate.SlotOrPid);
              if((err = copy_to_user( (STPTI_DescDisAssociate_t*)arg, &UserParams.DescDisAssociate, sizeof(STPTI_DescDisAssociate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_DESCSETKEYS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DESCSETKEYS\n"));
            {

              FullHandle_t FullDescramblerHandle;

              if ((err = copy_from_user( &UserParams.DescSetKeys, (STPTI_DescSetKeys_t*)arg, sizeof(STPTI_DescSetKeys_t)) )){
              /* Invalid user space address */
                 goto fail;
              }

              FullDescramblerHandle.word = UserParams.DescSetKeys.DescramblerHandle;

              UserParams.DescSetKeys.Error = stpti_DescramblerHandleCheck(FullDescramblerHandle);
              if(UserParams.DescSetKeys.Error == ST_NO_ERROR)
              {
                  switch( stptiMemGet_Descrambler(FullDescramblerHandle)->Type )
                  {
                       case STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER:
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                       case STPTI_DESCRAMBLER_TYPE_AES_ECB_LR_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_AES_CBC_LR_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER:
#endif
                       {
                           if((err = copy_from_user( &UserParams.DescSetKeys.Data, UserParams.DescSetKeys.UserKeyPtr, 32) ))
                           {
                              goto fail;
                           }
                           break;
                       }
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                       case STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER:
                       case STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER:
                       {
                           if((err = copy_from_user( &UserParams.DescSetKeys.Data, UserParams.DescSetKeys.UserKeyPtr, 16) ))
                           {
                              goto fail;
                           }
                           break;
                       }
#endif
                       default:
                       {
                           if((err = copy_from_user( &UserParams.DescSetKeys.Data, UserParams.DescSetKeys.UserKeyPtr, 8) ))
                           {
                               goto fail;
                           }
                       }
                  }
                  UserParams.DescSetKeys.Error = STPTI_DescramblerSet(UserParams.DescSetKeys.DescramblerHandle, UserParams.DescSetKeys.Parity, UserParams.DescSetKeys.Usage, UserParams.DescSetKeys.Data);
              }
              if((err = copy_to_user( (STPTI_DescSetKeys_t*)arg, &UserParams.DescSetKeys, sizeof(STPTI_DescSetKeys_t)))) 
              {
                   /* Invalid user space address */
                   goto fail;
              }
            }
            break;
        case STPTI_IOC_GETARRIVALTIME:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETARRIVALTIME\n"));
            {
              if ((err = copy_from_user( &UserParams.GetArrivalTime, (STPTI_GetArrivalTime_t*)arg, sizeof(STPTI_GetArrivalTime_t)) )){
              /* Invalid user space address */
                 goto fail;
              }
              UserParams.GetArrivalTime.Error = STPTI_GetPacketArrivalTime(UserParams.GetArrivalTime.Handle, &(UserParams.GetArrivalTime.ArrivalTime), &(UserParams.GetArrivalTime.ArrivalTimeExtension));
              if((err = copy_to_user( (STPTI_GetArrivalTime_t*)arg, &UserParams.GetArrivalTime, sizeof(STPTI_GetArrivalTime_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTDESCRAMBLINGCONTROL:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTDESCRAMBLINGCONTROL\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotDescramblingControl, (STPTI_Ioctl_SlotDescramblingControl_t*)arg, sizeof(STPTI_Ioctl_SlotDescramblingControl_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotDescramblingControl.Error = STPTI_SlotDescramblingControl( UserParams.SlotDescramblingControl.SlotHandle, UserParams.SlotDescramblingControl.EnableDescramblingControl );

                if((err = copy_to_user( (STPTI_Ioctl_SlotDescramblingControl_t*)arg, &UserParams.SlotDescramblingControl, sizeof(STPTI_Ioctl_SlotDescramblingControl_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTSETALTERNATEOUTPUTACTION:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTSETALTERNATEOUTPUTACTION\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotSetAlternateOutputAction, (STPTI_Ioctl_SlotSetAlternateOutputAction_t*)arg, sizeof(STPTI_Ioctl_SlotSetAlternateOutputAction_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotSetAlternateOutputAction.Error = STPTI_SlotSetAlternateOutputAction( UserParams.SlotSetAlternateOutputAction.SlotHandle, UserParams.SlotSetAlternateOutputAction.Method, UserParams.SlotSetAlternateOutputAction.Tag );

                if((err = copy_to_user( (STPTI_Ioctl_SlotSetAlternateOutputAction_t*)arg, &UserParams.SlotSetAlternateOutputAction, sizeof(STPTI_Ioctl_SlotSetAlternateOutputAction_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTSTATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTSTATE\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotState, (STPTI_Ioctl_SlotState_t*)arg, sizeof(STPTI_Ioctl_SlotState_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotState.Error = STPTI_SlotState( UserParams.SlotState.SlotHandle, &UserParams.SlotState.SlotCount, &UserParams.SlotState.ScrambleState, &UserParams.SlotState.Pid );

                if((err = copy_to_user( (STPTI_Ioctl_SlotState_t*)arg, &UserParams.SlotState, sizeof(STPTI_Ioctl_SlotState_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_ALTERNATEOUTPUTSETDEFAULTACTION:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_ALTERNATEOUTPUTSETDEFAULTACTION\n"));
            {
                if ((err = copy_from_user( &UserParams.AlternateOutputSetDefaultAction, (STPTI_Ioctl_AlternateOutputSetDefaultAction_t*)arg, sizeof(STPTI_Ioctl_AlternateOutputSetDefaultAction_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.AlternateOutputSetDefaultAction.Error = STPTI_AlternateOutputSetDefaultAction( UserParams.AlternateOutputSetDefaultAction.SlotHandle, UserParams.AlternateOutputSetDefaultAction.Method, UserParams.AlternateOutputSetDefaultAction.Tag );

                if((err = copy_to_user( (STPTI_Ioctl_AlternateOutputSetDefaultAction_t*)arg, &UserParams.AlternateOutputSetDefaultAction, sizeof(STPTI_Ioctl_AlternateOutputSetDefaultAction_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_HARWAREPAUSE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_HARWAREPAUSE\n"));
            {
                if ((err = copy_from_user(&UserParams.HardwareData, (STPTI_Ioctl_HardwareData_t *)arg, sizeof(STPTI_Ioctl_HardwareData_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.HardwareData.Error = STPTI_HardwarePause(UserParams.HardwareData.DeviceName);

                if((err = copy_to_user((STPTI_Ioctl_HardwareData_t*)arg, &UserParams.HardwareData, sizeof(STPTI_Ioctl_HardwareData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_HARWARERESET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_HARWARERESET\n"));
            {
                if ((err = copy_from_user(&UserParams.HardwareData, (STPTI_Ioctl_HardwareData_t *)arg, sizeof(STPTI_Ioctl_HardwareData_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.HardwareData.Error = STPTI_HardwareReset(UserParams.HardwareData.DeviceName);

                if((err = copy_to_user((STPTI_Ioctl_HardwareData_t*)arg, &UserParams.HardwareData, sizeof(STPTI_Ioctl_HardwareData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_HARWARERESUME:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_HARWARERESUME\n"));
            {
                if ((err = copy_from_user(&UserParams.HardwareData, (STPTI_Ioctl_HardwareData_t *)arg, sizeof(STPTI_Ioctl_HardwareData_t)) )){
                    /* Invalid user space address */
                    goto fail;
                }

                UserParams.HardwareData.Error = STPTI_HardwareResume(UserParams.HardwareData.DeviceName);

                if((err = copy_to_user((STPTI_Ioctl_HardwareData_t*)arg, &UserParams.HardwareData, sizeof(STPTI_Ioctl_HardwareData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_GETPRESENTATIONSTC:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETPRESENTATIONSTC\n"));
            {
                if ((err = copy_from_user( &UserParams.GetPresentationSTC, (STPTI_Ioctl_GetPresentationSTC_t*)arg, sizeof(STPTI_Ioctl_GetPresentationSTC_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetPresentationSTC.Error = STPTI_GetPresentationSTC(UserParams.GetPresentationSTC.DeviceName , UserParams.GetPresentationSTC.Timer, &UserParams.GetPresentationSTC.TimeStamp );

                if((err = copy_to_user( (STPTI_Ioctl_GetPresentationSTC_t*)arg, &UserParams.GetPresentationSTC, sizeof(STPTI_Ioctl_GetPresentationSTC_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERSETMULTIPACKET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERSETMULTIPACKET\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferSetMultiPacket, (STPTI_Ioctl_BufferSetMultiPacket_t*)arg, sizeof(STPTI_Ioctl_BufferSetMultiPacket_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferSetMultiPacket.Error = STPTI_BufferSetMultiPacket( UserParams.BufferSetMultiPacket.BufferHandle, UserParams.BufferSetMultiPacket.NumberOfPacketsInMultiPacket );

                if((err = copy_to_user( (STPTI_Ioctl_BufferSetMultiPacket_t*)arg, &UserParams.BufferSetMultiPacket, sizeof(STPTI_Ioctl_BufferSetMultiPacket_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERREAD:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERREAD\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferRead, (STPTI_Ioctl_BufferRead_t*)arg, sizeof(STPTI_Ioctl_BufferRead_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferRead.Error = STPTI_BufferRead( UserParams.BufferRead.BufferHandle,
                                                        UserParams.BufferRead.Destination0_p,
                                                        UserParams.BufferRead.DestinationSize0,
                                                        UserParams.BufferRead.Destination1_p,
                                                        UserParams.BufferRead.DestinationSize1,
                                                        &UserParams.BufferRead.DataSize,
                                                        UserParams.BufferRead.DmaOrMemcpy );

                if((err = copy_to_user( (STPTI_Ioctl_BufferRead_t*)arg, &UserParams.BufferRead, sizeof(STPTI_Ioctl_BufferRead_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFEREXTRACTDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFEREXTRACTDATA\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferExtractData, (STPTI_Ioctl_BufferExtractData_t*)arg, sizeof(STPTI_Ioctl_BufferExtractData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferExtractData.Error = STPTI_BufferExtractData( UserParams.BufferExtractData.BufferHandle,
                                                            UserParams.BufferExtractData.Offset,
                                                            UserParams.BufferExtractData.NumBytesToExtract,
                                                            UserParams.BufferExtractData.Destination0_p,
                                                            UserParams.BufferExtractData.DestinationSize0,
                                                            UserParams.BufferExtractData.Destination1_p,
                                                            UserParams.BufferExtractData.DestinationSize1,
                                                            &UserParams.BufferExtractData.DataSize,
                                                            UserParams.BufferExtractData.DmaOrMemcpy );

                if((err = copy_to_user( (STPTI_Ioctl_BufferExtractData_t*)arg, &UserParams.BufferExtractData, sizeof(STPTI_Ioctl_BufferExtractData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFEREXTRACTPARTIALPESPACKETDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFEREXTRACTPARTIALPESPACKETDATA\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferExtractPartialPesPacketData, (STPTI_Ioctl_BufferExtractPartialPesPacketData_t*)arg, sizeof(STPTI_Ioctl_BufferExtractPartialPesPacketData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferExtractPartialPesPacketData.Error = STPTI_BufferExtractPartialPesPacketData( UserParams.BufferExtractPartialPesPacketData.BufferHandle,
                                                            &UserParams.BufferExtractPartialPesPacketData.PayloadUnitStart,
                                                            &UserParams.BufferExtractPartialPesPacketData.CCDiscontinuity,
                                                            &UserParams.BufferExtractPartialPesPacketData.ContinuityCount,
                                                            &UserParams.BufferExtractPartialPesPacketData.DataLength );

                if((err = copy_to_user( (STPTI_Ioctl_BufferExtractPartialPesPacketData_t*)arg, &UserParams.BufferExtractPartialPesPacketData, sizeof(STPTI_Ioctl_BufferExtractPartialPesPacketData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFEREXTRACTPESPACKETDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFEREXTRACTPESPACKETDATA\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferExtractPesPacketData, (STPTI_Ioctl_BufferExtractPesPacketData_t*)arg, sizeof(STPTI_Ioctl_BufferExtractPesPacketData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferExtractPesPacketData.Error = STPTI_BufferExtractPesPacketData( UserParams.BufferExtractPesPacketData.BufferHandle,
                                                                     &UserParams.BufferExtractPesPacketData.PesFlags,
                                                                     &UserParams.BufferExtractPesPacketData.TrickModeFlags,
                                                                     &UserParams.BufferExtractPesPacketData.PESPacketLength,
                                                                     &UserParams.BufferExtractPesPacketData.PTSValue,
                                                                     &UserParams.BufferExtractPesPacketData.DTSValue);

                if((err = copy_to_user( (STPTI_Ioctl_BufferExtractPesPacketData_t*)arg, &UserParams.BufferExtractPesPacketData, sizeof(STPTI_Ioctl_BufferExtractPesPacketData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFEREXTRACTSECTIONDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFEREXTRACTSECTIONDATA\n"));
            {
                STPTI_Filter_t *MatchedFilterList_p;

                if ((err = copy_from_user( &UserParams.BufferExtractSectionData, (STPTI_Ioctl_BufferExtractSectionData_t*)arg, sizeof(STPTI_Ioctl_BufferExtractSectionData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                MatchedFilterList_p = (STPTI_Filter_t *)kmalloc(sizeof(STPTI_Filter_t)*UserParams.BufferExtractSectionData.MaxLengthofFilterList,GFP_KERNEL);

                UserParams.BufferExtractSectionData.Error = STPTI_BufferExtractSectionData( UserParams.BufferExtractSectionData.BufferHandle,
                                                            MatchedFilterList_p,
                                                            UserParams.BufferExtractSectionData.MaxLengthofFilterList,
                                                            &UserParams.BufferExtractSectionData.NumOfFilterMatches,
                                                            &UserParams.BufferExtractSectionData.CRCValid,
                                                            &UserParams.BufferExtractSectionData.SectionHeader);

                if ((err = copy_to_user( ((STPTI_Ioctl_BufferExtractSectionData_t*)arg)->MatchedFilterList, MatchedFilterList_p, sizeof(STPTI_Filter_t)*UserParams.BufferExtractSectionData.MaxLengthofFilterList) )){
                /* Invalid user space address */
                    kfree( MatchedFilterList_p );
                    goto fail;
                }

                kfree( MatchedFilterList_p );

                if((err = copy_to_user( (STPTI_Ioctl_BufferExtractSectionData_t*)arg, &UserParams.BufferExtractSectionData, sizeof(STPTI_Ioctl_BufferExtractSectionData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFEREXTRACTTSHEADERDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFEREXTRACTTSHEADERDATA\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferExtractTSHeaderData, (STPTI_Ioctl_BufferExtractTSHeaderData_t*)arg, sizeof(STPTI_Ioctl_BufferExtractTSHeaderData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferExtractTSHeaderData.Error = STPTI_BufferExtractTSHeaderData( UserParams.BufferExtractTSHeaderData.BufferHandle,
                                                            &UserParams.BufferExtractTSHeaderData.TSHeader );

                if((err = copy_to_user( (STPTI_Ioctl_BufferExtractTSHeaderData_t*)arg, &UserParams.BufferExtractTSHeaderData, sizeof(STPTI_Ioctl_BufferExtractTSHeaderData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERREADPARTIALPESPACKET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERREADPARTIALPESPACKET\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferReadPartialPesPacket, (STPTI_Ioctl_BufferReadPartialPesPacket_t*)arg, sizeof(STPTI_Ioctl_BufferReadPartialPesPacket_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferReadPartialPesPacket.Error = STPTI_BufferReadPartialPesPacket( UserParams.BufferReadPartialPesPacket.BufferHandle,
                                                                     UserParams.BufferReadPartialPesPacket.Destination0_p,
                                                                     UserParams.BufferReadPartialPesPacket.DestinationSize0,
                                                                     UserParams.BufferReadPartialPesPacket.Destination1_p,
                                                                     UserParams.BufferReadPartialPesPacket.DestinationSize1,
                                                                     &UserParams.BufferReadPartialPesPacket.PayloadUnitStart,
                                                                     &UserParams.BufferReadPartialPesPacket.CCDiscontinuity,
                                                                     &UserParams.BufferReadPartialPesPacket.ContinuityCount,
                                                                     &UserParams.BufferReadPartialPesPacket.DataSize,
                                                                     UserParams.BufferReadPartialPesPacket.DmaOrMemcpy);

                if((err = copy_to_user( (STPTI_Ioctl_BufferReadPartialPesPacket_t*)arg, &UserParams.BufferReadPartialPesPacket, sizeof(STPTI_Ioctl_BufferReadPartialPesPacket_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERREADTSPACKET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERREADTSPACKET\n"));
            {
                /* We share STPTI_Ioctl_BufferRead_t sructure */

                if ((err = copy_from_user( &UserParams.BufferRead, (STPTI_Ioctl_BufferRead_t*)arg, sizeof(STPTI_Ioctl_BufferRead_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferRead.Error = STPTI_BufferReadTSPacket( UserParams.BufferRead.BufferHandle,
                                                        UserParams.BufferRead.Destination0_p,
                                                        UserParams.BufferRead.DestinationSize0,
                                                        UserParams.BufferRead.Destination1_p,
                                                        UserParams.BufferRead.DestinationSize1,
                                                        &UserParams.BufferRead.DataSize,
                                                        UserParams.BufferRead.DmaOrMemcpy );

                if((err = copy_to_user( (STPTI_Ioctl_BufferRead_t*)arg, &UserParams.BufferRead, sizeof(STPTI_Ioctl_BufferRead_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERSETMATCHACTION:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERSETMATCHACTION\n"));
            {
                STPTI_Filter_t *FiltersToEnable_p;

                if ((err = copy_from_user( &UserParams.FilterSetMatchAction, (STPTI_Ioctl_FilterSetMatchAction_t*)arg, sizeof(STPTI_Ioctl_FilterSetMatchAction_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                FiltersToEnable_p = (STPTI_Filter_t *)kmalloc(sizeof(STPTI_Filter_t)*UserParams.FilterSetMatchAction.NumOfFiltersToEnable,GFP_KERNEL);
                if ((err = copy_from_user(FiltersToEnable_p, ((STPTI_Ioctl_FilterSetMatchAction_t*)arg)->FiltersToEnable, sizeof(STPTI_Filter_t)*UserParams.FilterSetMatchAction.NumOfFiltersToEnable) )){
                /* Invalid user space address */
                    kfree( FiltersToEnable_p );
                    goto fail;
                }

                UserParams.FilterSetMatchAction.Error = STPTI_FilterSetMatchAction( UserParams.FilterSetMatchAction.FilterHandle,
                                                            FiltersToEnable_p,
                                                            UserParams.FilterSetMatchAction.NumOfFiltersToEnable);
                kfree( FiltersToEnable_p );

                if((err = copy_to_user( (STPTI_Ioctl_FilterSetMatchAction_t*)arg, &UserParams.FilterSetMatchAction, sizeof(STPTI_Ioctl_FilterSetMatchAction_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_FILTERSFLUSH:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FILTERSFLUSH\n"));
            {
                STPTI_Filter_t *Filters_p;

                if ((err = copy_from_user( &UserParams.FiltersFlush, (STPTI_Ioctl_FiltersFlush_t*)arg, sizeof(STPTI_Ioctl_FiltersFlush_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                Filters_p = (STPTI_Filter_t *)kmalloc(sizeof(STPTI_Filter_t)*UserParams.FiltersFlush.NumOfFiltersToFlush,GFP_KERNEL);
                if ((err = copy_from_user(Filters_p, ((STPTI_Ioctl_FiltersFlush_t*)arg)->Filters, sizeof(STPTI_Filter_t)*UserParams.FiltersFlush.NumOfFiltersToFlush) )){
                /* Invalid user space address */
                    kfree( Filters_p );
                    goto fail;
                }

                UserParams.FiltersFlush.Error = STPTI_FiltersFlush( UserParams.FiltersFlush.BufferHandle,
                                                            Filters_p,
                                                            UserParams.FiltersFlush.NumOfFiltersToFlush);
                kfree( Filters_p );

                if((err = copy_to_user( (STPTI_Ioctl_FiltersFlush_t*)arg, &UserParams.FiltersFlush, sizeof(STPTI_Ioctl_FiltersFlush_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_MODIFYGLOBALFILTERSTATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_MODIFYGLOBALFILTERSTATE\n"));
            {
                STPTI_Filter_t *Filters_p;
                U16 TotalFilters;

                if ((err = copy_from_user( &UserParams.ModifyGlobalFilterState, (STPTI_Ioctl_ModifyGlobalFilterState_t*)arg, sizeof(STPTI_Ioctl_ModifyGlobalFilterState_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                TotalFilters = UserParams.ModifyGlobalFilterState.NumOfFiltersToDisable+UserParams.ModifyGlobalFilterState.NumOfFiltersToEnable;
                Filters_p = (STPTI_Filter_t *)kmalloc(sizeof(STPTI_Filter_t)*TotalFilters,GFP_KERNEL);
                if ((err = copy_from_user(Filters_p, ((STPTI_Ioctl_ModifyGlobalFilterState_t*)arg)->Filters, sizeof(STPTI_Filter_t)*TotalFilters) )){
                /* Invalid user space address */
                    kfree( Filters_p );
                    goto fail;
                }

                UserParams.ModifyGlobalFilterState.Error = STPTI_ModifyGlobalFilterState( Filters_p ,
                                                                  UserParams.ModifyGlobalFilterState.NumOfFiltersToDisable,
                                                                  Filters_p+UserParams.ModifyGlobalFilterState.NumOfFiltersToDisable,
                                                                  UserParams.ModifyGlobalFilterState.NumOfFiltersToEnable);
                kfree( Filters_p );

                if((err = copy_to_user( (STPTI_Ioctl_ModifyGlobalFilterState_t*)arg, &UserParams.ModifyGlobalFilterState, sizeof(STPTI_Ioctl_ModifyGlobalFilterState_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_GETPACKETERRORCOUNT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETPACKETERRORCOUNT\n"));
            {
                if ((err = copy_from_user( &UserParams.GetPacketErrorCount, (STPTI_Ioctl_GetPacketErrorCount_t*)arg, sizeof(STPTI_Ioctl_GetPacketErrorCount_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetPacketErrorCount.Error = STPTI_GetPacketErrorCount( UserParams.GetPacketErrorCount.DeviceName,&UserParams.GetPacketErrorCount.Count );

               if((err = copy_to_user( (STPTI_Ioctl_GetPacketErrorCount_t*)arg, &UserParams.GetPacketErrorCount, sizeof(STPTI_Ioctl_GetPacketErrorCount_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_PIDQUERY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_PIDQUERY\n"));
            {
                if ((err = copy_from_user( &UserParams.PidQuery, (STPTI_Ioctl_PidQuery_t*)arg, sizeof(STPTI_Ioctl_PidQuery_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.PidQuery.Error = STPTI_PidQuery( UserParams.PidQuery.DeviceName,UserParams.PidQuery.Pid, &UserParams.PidQuery.Slot );

               if((err = copy_to_user( (STPTI_Ioctl_PidQuery_t*)arg, &UserParams.PidQuery, sizeof(STPTI_Ioctl_PidQuery_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_GETCURRENTPTITIMER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETCURRENTPTITIMER\n"));
            {
                if ((err = copy_from_user( &UserParams.GetCurrentPTITimer, (STPTI_Ioctl_GetCurrentPTITimer_t*)arg, sizeof(STPTI_Ioctl_GetCurrentPTITimer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetCurrentPTITimer.Error = STPTI_GetCurrentPTITimer( UserParams.GetCurrentPTITimer.DeviceName, &UserParams.GetCurrentPTITimer.TimeStamp );

               if((err = copy_to_user( (STPTI_Ioctl_GetCurrentPTITimer_t*)arg, &UserParams.GetCurrentPTITimer, sizeof(STPTI_Ioctl_GetCurrentPTITimer_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_GETGLOBALPACKETCOUNT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETGLOBALPACKETCOUNT\n"));
            {
                if ((err = copy_from_user( &UserParams.GetGlobalPacketCount, (STPTI_Ioctl_GetGlobalPacketCount_t*)arg, sizeof(STPTI_Ioctl_GetGlobalPacketCount_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetGlobalPacketCount.Error = STPTI_GetGlobalPacketCount( UserParams.GetGlobalPacketCount.DeviceName,&UserParams.GetGlobalPacketCount.Count );

               if((err = copy_to_user( (STPTI_Ioctl_GetGlobalPacketCount_t*)arg, &UserParams.GetGlobalPacketCount, sizeof(STPTI_Ioctl_GetGlobalPacketCount_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_GETBUFFERSFROMSLOT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_GETBUFFERSFROMSLOT\n"));
            {
                if ((err = copy_from_user( &UserParams.GetBuffersFromSlot, (STPTI_Ioctl_GetBuffersFromSlot_t*)arg, sizeof(STPTI_Ioctl_GetBuffersFromSlot_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.GetBuffersFromSlot.Error = STPTI_GetBuffersFromSlot( UserParams.GetBuffersFromSlot.SlotHandle,&UserParams.GetBuffersFromSlot.BufferHandle, &UserParams.GetBuffersFromSlot.RecordBufferHandle );

               if((err = copy_to_user( (STPTI_Ioctl_GetBuffersFromSlot_t*)arg, &UserParams.GetBuffersFromSlot, sizeof(STPTI_Ioctl_GetBuffersFromSlot_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_SETDISCARDPARAMS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SETDISCARDPARAMS\n"));
            {
                if ((err = copy_from_user( &UserParams.SetDiscardParams, (STPTI_Ioctl_SetDiscardParams_t*)arg, sizeof(STPTI_Ioctl_SetDiscardParams_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SetDiscardParams.Error = STPTI_SetDiscardParams( UserParams.SetDiscardParams.DeviceName, UserParams.SetDiscardParams.NumberOfDiscardBytes );

               if((err = copy_to_user( (STPTI_Ioctl_SetDiscardParams_t*)arg, &UserParams.SetDiscardParams, sizeof(STPTI_Ioctl_SetDiscardParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_SETSTREAMID:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SETSTREAMID\n"));
            {
                if ((err = copy_from_user( &UserParams.SetStreamID, (STPTI_Ioctl_SetStreamID_t*)arg, sizeof(STPTI_Ioctl_SetStreamID_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SetStreamID.Error = STPTI_SetStreamID( UserParams.SetStreamID.Device, UserParams.SetStreamID.StreamID );

               if((err = copy_to_user( (STPTI_Ioctl_SetStreamID_t*)arg, &UserParams.SetStreamID, sizeof(STPTI_Ioctl_SetStreamID_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_HARDWAREFIFOLEVELS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_HARDWAREFIFOLEVELS\n"));
            {
                if ((err = copy_from_user( &UserParams.HardwareFIFOLevels, (STPTI_Ioctl_HardwareFIFOLevels_t*)arg, sizeof(STPTI_Ioctl_HardwareFIFOLevels_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.HardwareFIFOLevels.Error = STPTI_HardwareFIFOLevels( UserParams.HardwareFIFOLevels.DeviceName, &UserParams.HardwareFIFOLevels.Overflow, &UserParams.HardwareFIFOLevels.InputLevel, &UserParams.HardwareFIFOLevels.AltLevel, &UserParams.HardwareFIFOLevels.HeaderLevel );

               if((err = copy_to_user( (STPTI_Ioctl_HardwareFIFOLevels_t*)arg, &UserParams.HardwareFIFOLevels, sizeof(STPTI_Ioctl_HardwareFIFOLevels_t)))) {
                    /* Invalid user space address */
                    goto fail;
               }
            }
            break;
        case STPTI_IOC_ENABLESCRAMBLINGEVENTS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_ENABLESCRAMBLINGEVENTS\n"));
            {
                if ((err = copy_from_user( &UserParams.EnableScramblingEvents, (STPTI_Ioctl_EnableScramblingEvents_t*)arg, sizeof(STPTI_Ioctl_EnableScramblingEvents_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.EnableScramblingEvents.Error = STPTI_EnableScramblingEvents( UserParams.EnableScramblingEvents.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_EnableScramblingEvents_t*)arg, &UserParams.EnableScramblingEvents, sizeof(STPTI_Ioctl_EnableScramblingEvents_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_SLOTLINKTORECORDBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTLINKTORECORDBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotLinkToRecordBuffer, (STPTI_Ioctl_SlotLinkToRecordBuffer_t*)arg, sizeof(STPTI_Ioctl_SlotLinkToRecordBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotLinkToRecordBuffer.Error = STPTI_SlotLinkToRecordBuffer( UserParams.SlotLinkToRecordBuffer.Slot, UserParams.SlotLinkToRecordBuffer.Buffer, UserParams.SlotLinkToRecordBuffer.DescrambleTS );

                if((err = copy_to_user( (STPTI_Ioctl_SlotLinkToRecordBuffer_t*)arg, &UserParams.SlotLinkToRecordBuffer, sizeof(STPTI_Ioctl_SlotLinkToRecordBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_SLOTUNLINKRECORDBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTUNLINKRECORDBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotUnLinkRecordBuffer, (STPTI_Ioctl_SlotUnLinkRecordBuffer_t*)arg, sizeof(STPTI_Ioctl_SlotUnLinkRecordBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotUnLinkRecordBuffer.Error = STPTI_SlotUnLinkRecordBuffer( UserParams.SlotUnLinkRecordBuffer.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_SlotUnLinkRecordBuffer_t*)arg, &UserParams.SlotUnLinkRecordBuffer, sizeof(STPTI_Ioctl_SlotUnLinkRecordBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;




/* Added Indexing Under Linux */
#ifndef STPTI_NO_INDEX_SUPPORT
        case STPTI_IOC_INDEXALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexAllocate, (STPTI_Ioctl_IndexAllocate_t*)arg, sizeof(STPTI_Ioctl_IndexAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexAllocate.Error    = STPTI_IndexAllocate( UserParams.IndexAllocate.SessionHandle, &UserParams.IndexAllocate.IndexHandle );

                if((err = copy_to_user( (STPTI_Ioctl_IndexAllocate_t*)arg, &UserParams.IndexAllocate, sizeof(STPTI_Ioctl_IndexAllocate_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXASSOCIATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXASSOCIATE\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexAssociate, (STPTI_Ioctl_IndexAssociate_t*)arg, sizeof(STPTI_Ioctl_IndexAssociate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexAssociate.Error    = STPTI_IndexAssociate( UserParams.IndexAssociate.IndexHandle, UserParams.IndexAssociate.SlotOrPid );

                if((err = copy_to_user( (STPTI_Ioctl_IndexAssociate_t*)arg, &UserParams.IndexAssociate, sizeof(STPTI_Ioctl_IndexAssociate_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexDeallocate, (STPTI_Ioctl_IndexDeallocate_t*)arg, sizeof(STPTI_Ioctl_IndexDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexDeallocate.Error    = STPTI_IndexDeallocate( UserParams.IndexDeallocate.IndexHandle );

                if((err = copy_to_user( (STPTI_Ioctl_IndexDeallocate_t*)arg, &UserParams.IndexDeallocate, sizeof(STPTI_Ioctl_IndexDeallocate_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXDISASSOCIATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXDISASSOCIATE\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexDisassociate, (STPTI_Ioctl_IndexDisassociate_t*)arg, sizeof(STPTI_Ioctl_IndexDisassociate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexDisassociate.Error    = STPTI_IndexDisassociate( UserParams.IndexDisassociate.IndexHandle, UserParams.IndexDisassociate.SlotOrPid );

                if((err = copy_to_user( (STPTI_Ioctl_IndexDisassociate_t*)arg, &UserParams.IndexDisassociate, sizeof(STPTI_Ioctl_IndexDisassociate_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXRESET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXRESET\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexReset, (STPTI_Ioctl_IndexReset_t*)arg, sizeof(STPTI_Ioctl_IndexReset_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexReset.Error    = STPTI_IndexReset( UserParams.IndexReset.IndexHandle );

                if((err = copy_to_user( (STPTI_Ioctl_IndexReset_t*)arg, &UserParams.IndexReset, sizeof(STPTI_Ioctl_IndexReset_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXSET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXSET\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexSet, (STPTI_Ioctl_IndexSet_t*)arg, sizeof(STPTI_Ioctl_IndexSet_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexSet.Error    = STPTI_IndexSet( UserParams.IndexSet.IndexHandle, UserParams.IndexSet.IndexMask, UserParams.IndexSet.MPEGStartCode, UserParams.IndexSet.MPEGStartCodeMode );

                if((err = copy_to_user( (STPTI_Ioctl_IndexSet_t*)arg, &UserParams.IndexSet, sizeof(STPTI_Ioctl_IndexSet_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXCHAIN:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXCHAIN\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexChain, (STPTI_Ioctl_IndexChain_t*)arg, sizeof(STPTI_Ioctl_IndexChain_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexChain.Error    = STPTI_IndexChain( UserParams.IndexChain.IndexHandles, UserParams.IndexChain.NumberOfHandles );

                if((err = copy_to_user( (STPTI_Ioctl_IndexChain_t*)arg, &UserParams.IndexChain, sizeof(STPTI_Ioctl_IndexChain_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXSTART:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXSTART\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexStart, (STPTI_Ioctl_IndexStart_t*)arg, sizeof(STPTI_Ioctl_IndexStart_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexStart.Error    = STPTI_IndexStart( UserParams.IndexStart.DeviceName );

                if((err = copy_to_user( (STPTI_Ioctl_IndexStart_t*)arg, &UserParams.IndexStart, sizeof(STPTI_Ioctl_IndexStart_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

        case STPTI_IOC_INDEXSTOP:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_INDEXSTOP\n"));
            {
                if ((err = copy_from_user( &UserParams.IndexStop, (STPTI_Ioctl_IndexStop_t*)arg, sizeof(STPTI_Ioctl_IndexStop_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.IndexStop.Error    = STPTI_IndexStop( UserParams.IndexStop.DeviceName );

                if((err = copy_to_user( (STPTI_Ioctl_IndexStop_t*)arg, &UserParams.IndexStop, sizeof(STPTI_Ioctl_IndexStop_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;

#endif /* STPTI_NO_INDEX_SUPPORT */

        case STPTI_IOC_SLOTSETCORRUPTIONPARAMS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTSETCORRUPTIONPARAMS\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotSetCorruptionParams, (STPTI_Ioctl_SlotSetCorruptionParams_t*)arg, sizeof(STPTI_Ioctl_SlotSetCorruptionParams_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotSetCorruptionParams.Error = STPTI_SlotSetCorruptionParams( UserParams.SlotSetCorruptionParams.SlotHandle, UserParams.SlotSetCorruptionParams.Offset, UserParams.SlotSetCorruptionParams.Value );

                if((err = copy_to_user( (STPTI_Ioctl_SlotSetCorruptionParams_t*)arg, &UserParams.SlotSetCorruptionParams, sizeof(STPTI_Ioctl_SlotSetCorruptionParams_t)))) {
                     /* Invalid user space address */
                     goto fail;
                }
            }
            break;


        case STPTI_IOC_SLOTUPDATEPID:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SLOTUPDATEPID\n"));
            {
                if ((err = copy_from_user( &UserParams.SlotUpdatePid, (STPTI_Ioctl_SlotUpdatePid_t*)arg, sizeof(STPTI_Ioctl_SlotUpdatePid_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SlotUpdatePid.Error = STPTI_SlotUpdatePid( UserParams.SlotUpdatePid.SlotHandle, UserParams.SlotUpdatePid.Pid );

                if((err = copy_to_user( (STPTI_Ioctl_SlotUpdatePid_t*)arg, &UserParams.SlotUpdatePid, sizeof(STPTI_Ioctl_SlotUpdatePid_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_BUFFERSETREADPOINTER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERSETREADPOINTER\n"));
            {
                U32 AddressDiff;

                if ((err = copy_from_user( &UserParams.BufferSetReadPointer, (STPTI_Ioctl_BufferSetReadPointer_t*)arg, sizeof(STPTI_Ioctl_BufferSetReadPointer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                stpti_GetUserSpaceMap( UserParams.BufferSetReadPointer.BufferHandle, &AddressDiff );
                UserParams.BufferSetReadPointer.Error = STPTI_BufferSetReadPointer( UserParams.BufferSetReadPointer.BufferHandle, UserParams.BufferSetReadPointer.Read_p -  AddressDiff);

                if((err = copy_to_user( (STPTI_Ioctl_BufferSetReadPointer_t*)arg, &UserParams.BufferSetReadPointer, sizeof(STPTI_Ioctl_BufferSetReadPointer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_BUFFERGETWRITEPOINTER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERGETWRITEPOINTER\n"));
            {
                U32 AddressDiff;
                ST_ErrorCode_t Error;

                if ((err = copy_from_user( &UserParams.BufferGetWritePointer, (STPTI_Ioctl_BufferGetWritePointer_t*)arg, sizeof(STPTI_Ioctl_BufferGetWritePointer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                Error = stpti_GetUserSpaceMap( UserParams.BufferGetWritePointer.BufferHandle, &AddressDiff );
                UserParams.BufferGetWritePointer.Error = STPTI_BufferGetWritePointer( UserParams.BufferGetWritePointer.BufferHandle, &UserParams.BufferGetWritePointer.Write_p );

                if (Error == ST_NO_ERROR && UserParams.BufferGetWritePointer.Error == ST_NO_ERROR)
                {
                    stpti_BufferCacheInvalidate( UserParams.BufferGetWritePointer.BufferHandle );
                    UserParams.BufferGetWritePointer.Write_p += AddressDiff;
                }

                if((err = copy_to_user( (STPTI_Ioctl_BufferGetWritePointer_t*)arg, &UserParams.BufferGetWritePointer, sizeof(STPTI_Ioctl_BufferGetWritePointer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_BUFFERPACKETCOUNT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_BUFFERPACKETCOUNT\n"));
            {
                if ((err = copy_from_user( &UserParams.BufferPacketCount, (STPTI_Ioctl_BufferPacketCount_t*)arg, sizeof(STPTI_Ioctl_BufferPacketCount_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.BufferPacketCount.Error = STPTI_BufferPacketCount( UserParams.BufferPacketCount.BufferHandle, &UserParams.BufferPacketCount.Count );

                if((err = copy_to_user( (STPTI_Ioctl_BufferPacketCount_t*)arg, &UserParams.BufferPacketCount, sizeof(STPTI_Ioctl_BufferPacketCount_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_DISABLESCRAMBLINGEVENTS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DISABLESCRAMBLINGEVENTS\n"));
            {
                if ((err = copy_from_user( &UserParams.DisableScramblingEvents, (STPTI_Ioctl_DisableScramblingEvents_t*)arg, sizeof(STPTI_Ioctl_DisableScramblingEvents_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.DisableScramblingEvents.Error = STPTI_DisableScramblingEvents( UserParams.DisableScramblingEvents.SlotHandle );

                if((err = copy_to_user( (STPTI_Ioctl_DisableScramblingEvents_t*)arg, &UserParams.DisableScramblingEvents, sizeof(STPTI_Ioctl_DisableScramblingEvents_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_MODIFYSYNCLOCKANDDROP:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_MODIFYSYNCLOCKANDDROP\n"));
            {
                if ((err = copy_from_user( &UserParams.ModifySyncLockAndDrop, (STPTI_Ioctl_ModifySyncLockAndDrop_t*)arg, sizeof(STPTI_Ioctl_ModifySyncLockAndDrop_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.ModifySyncLockAndDrop.Error = STPTI_ModifySyncLockAndDrop( UserParams.ModifySyncLockAndDrop.DeviceName, UserParams.ModifySyncLockAndDrop.SyncLock, UserParams.ModifySyncLockAndDrop.SyncDrop);

                if((err = copy_to_user( (STPTI_Ioctl_ModifySyncLockAndDrop_t*)arg, &UserParams.ModifySyncLockAndDrop, sizeof(STPTI_Ioctl_ModifySyncLockAndDrop_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_SETSYSTEMKEY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_SETSYSTEMKEY\n"));
            {
                U8 SetSystemKeyData[32];

                if ((err = copy_from_user( &UserParams.SetSystemKey, (STPTI_Ioctl_SetSystemKey_t*)arg, sizeof(STPTI_Ioctl_SetSystemKey_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                if ((err = copy_from_user( SetSystemKeyData, UserParams.SetSystemKey.Data, 32) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.SetSystemKey.Error = STPTI_SetSystemKey( UserParams.SetSystemKey.DeviceName, UserParams.SetSystemKey.Data );

                if((err = copy_to_user( (STPTI_Ioctl_SetSystemKey_t*)arg, &UserParams.SetSystemKey, sizeof(STPTI_Ioctl_SetSystemKey_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_DUMPINPUTTS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DUMPINPUTTS\n"));
            {
                if ((err = copy_from_user( &UserParams.DumpInputTS, (STPTI_Ioctl_DumpInputTS_t*)arg, sizeof(STPTI_Ioctl_DumpInputTS_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.DumpInputTS.Error = STPTI_DumpInputTS( UserParams.DumpInputTS.BufferHandle, UserParams.DumpInputTS.bytes_to_capture_per_packet );

                if((err = copy_to_user( (STPTI_Ioctl_DumpInputTS_t*)arg, &UserParams.DumpInputTS, sizeof(STPTI_Ioctl_DumpInputTS_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

#if defined (STPTI_FRONTEND_HYBRID)

        case STPTI_IOC_FRONTENDALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendAllocate, (STPTI_Ioctl_FrontendAllocate_t*)arg, sizeof(STPTI_Ioctl_FrontendAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendAllocate.Error = STPTI_FrontendAllocate( UserParams.FrontendAllocate.PTIHandle, &UserParams.FrontendAllocate.FrontendHandle, UserParams.FrontendAllocate.FrontendType );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendAllocate_t*)arg, &UserParams.FrontendAllocate, sizeof(STPTI_Ioctl_FrontendAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendDeallocate, (STPTI_Ioctl_FrontendDeallocate_t*)arg, sizeof(STPTI_Ioctl_FrontendDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendDeallocate.Error = STPTI_FrontendDeallocate( UserParams.FrontendDeallocate.FrontendHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendDeallocate_t*)arg, &UserParams.FrontendDeallocate, sizeof(STPTI_Ioctl_FrontendDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDGETPARAMS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDGETPARAMS\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendGetParams, (STPTI_Ioctl_FrontendGetParams_t*)arg, sizeof(STPTI_Ioctl_FrontendGetParams_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendGetParams.Error = STPTI_FrontendGetParams( UserParams.FrontendGetParams.FrontendHandle, &UserParams.FrontendGetParams.FrontendType, &UserParams.FrontendGetParams.FrontendParams );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendGetParams_t*)arg, &UserParams.FrontendGetParams, sizeof(STPTI_Ioctl_FrontendGetParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDGETSTATUS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDGETSTATUS\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendGetStatus, (STPTI_Ioctl_FrontendGetStatus_t*)arg, sizeof(STPTI_Ioctl_FrontendGetStatus_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendGetStatus.Error = STPTI_FrontendGetStatus( UserParams.FrontendGetStatus.FrontendHandle, &UserParams.FrontendGetStatus.FrontendStatus );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendGetStatus_t*)arg, &UserParams.FrontendGetStatus, sizeof(STPTI_Ioctl_FrontendGetStatus_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDINJECTDATA:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDINJECTDATA\n"));
            {
                STPTI_FrontendSWTSNode_t* SWTSUserNodeList_p = NULL;
                STPTI_FrontendSWTSNode_t* SWTSKernelNodeList_p = NULL;

                if ((err = copy_from_user( &UserParams.FrontendInjectData, (STPTI_Ioctl_FrontendInjectData_t*)arg, sizeof(STPTI_Ioctl_FrontendInjectData_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                // Check the pointer to the NodeList...
                if( UserParams.FrontendInjectData.FrontendSWTSNode_p != NULL )
                {
                    U32 NodeIndex = 0;

                    // Determine the size of the NodeList data to copy...
                    size_t NodeListSize = sizeof(STPTI_FrontendSWTSNode_t) * UserParams.FrontendInjectData.NumberOfSWTSNodes;

                    // Allocate the memory and attempt a copy...
                    SWTSUserNodeList_p = (STPTI_FrontendSWTSNode_t *) kmalloc( NodeListSize, GFP_KERNEL );

                    if ((err = copy_from_user( SWTSUserNodeList_p, UserParams.FrontendInjectData.FrontendSWTSNode_p, NodeListSize ) )){
                        kfree( SWTSUserNodeList_p );
                        /* Invalid user space address */
                        goto fail;
                    }

                    // Allocate the memory and check the return pointer...
                    SWTSKernelNodeList_p = (STPTI_FrontendSWTSNode_t *) kmalloc( NodeListSize, GFP_KERNEL );

                    if( SWTSKernelNodeList_p == NULL ) {
                        /* Tidy up any pre-allocated memory */
                        kfree( SWTSUserNodeList_p );
                        goto fail;
                    }

                    // For each node, allocate a kernel space buffer and copy ...
                    for( NodeIndex = 0; NodeIndex < UserParams.FrontendInjectData.NumberOfSWTSNodes; NodeIndex++ )
                    {
                        // Set up CURRENT NODE pointers to make the rest shorter...
                        STPTI_FrontendSWTSNode_t* SWTSUserNode_p   = & SWTSUserNodeList_p[ NodeIndex ];
                        STPTI_FrontendSWTSNode_t* SWTSKernelNode_p = & SWTSKernelNodeList_p[ NodeIndex ];

                        if( SWTSUserNode_p->SizeOfData > 131072 )
                        {
                            // Print a warning if we try to malloc more than 128k
                            printk( KERN_ALERT "STPTI_IOC_FRONTENDINJECTDATA :: Trying to allocate %d bytes with kmalloc.  Exceeds 128k. \n", SWTSUserNode_p->SizeOfData );
                        }

                        SWTSKernelNode_p->SizeOfData = SWTSUserNode_p->SizeOfData;
                        SWTSKernelNode_p->Data_p     = kmalloc( SWTSKernelNode_p->SizeOfData, GFP_KERNEL );

                        if ((err = copy_from_user( SWTSKernelNode_p->Data_p, SWTSUserNode_p->Data_p, SWTSKernelNode_p->SizeOfData ) )){

                            // Free all allocated memory and goto fail - ugly, but necessary...

                            S32 FreeIndex = NodeIndex;  // Note SIGNED to allow the loop completion test below to work correctly.

                            for( FreeIndex = NodeIndex; FreeIndex >= 0; FreeIndex-- )
                            {
                                kfree( SWTSKernelNodeList_p[ FreeIndex ].Data_p );
                            }

                            kfree( SWTSKernelNodeList_p );
                            kfree( SWTSUserNodeList_p );
                            /* Invalid user space address */
                            goto fail;
                        }
                    }

                    // Call the core function and free the NodeList memory...
                    UserParams.FrontendInjectData.Error = STPTI_FrontendInjectData ( UserParams.FrontendInjectData.FrontendHandle, SWTSKernelNodeList_p, UserParams.FrontendInjectData.NumberOfSWTSNodes );

                    // For each node, deallocate the kernel space buffer...
                    for( NodeIndex = 0; NodeIndex < UserParams.FrontendInjectData.NumberOfSWTSNodes; NodeIndex++ )
                    {
                        STPTI_FrontendSWTSNode_t* SWTSKernelNode_p = & SWTSKernelNodeList_p[ NodeIndex ];

                        kfree( SWTSKernelNode_p->Data_p );
                    }

                    kfree( SWTSKernelNodeList_p );
                    kfree( SWTSUserNodeList_p );
                }
                else
                {
                    UserParams.FrontendInjectData.Error = ST_ERROR_NO_MEMORY;
                }

                if((err = copy_to_user( (STPTI_Ioctl_FrontendInjectData_t*)arg, &UserParams.FrontendInjectData, sizeof(STPTI_Ioctl_FrontendInjectData_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDLINKTOPTI:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDLINKTOPTI\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendLinkToPTI, (STPTI_Ioctl_FrontendLinkToPTI_t*)arg, sizeof(STPTI_Ioctl_FrontendLinkToPTI_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendLinkToPTI.Error = STPTI_FrontendLinkToPTI( UserParams.FrontendLinkToPTI.FrontendHandle, UserParams.FrontendLinkToPTI.PTIHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendLinkToPTI_t*)arg, &UserParams.FrontendLinkToPTI, sizeof(STPTI_Ioctl_FrontendLinkToPTI_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDRESET:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDRESET\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendReset, (STPTI_Ioctl_FrontendReset_t*)arg, sizeof(STPTI_Ioctl_FrontendReset_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendReset.Error = STPTI_FrontendReset( UserParams.FrontendReset.FrontendHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendReset_t*)arg, &UserParams.FrontendReset, sizeof(STPTI_Ioctl_FrontendReset_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDSETPARAMS:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDSETPARAMS\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendSetParams, (STPTI_Ioctl_FrontendSetParams_t*)arg, sizeof(STPTI_Ioctl_FrontendSetParams_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendSetParams.Error = STPTI_FrontendSetParams( UserParams.FrontendSetParams.FrontendHandle, &UserParams.FrontendSetParams.FrontendParams );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendSetParams_t*)arg, &UserParams.FrontendSetParams, sizeof(STPTI_Ioctl_FrontendSetParams_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_FRONTENDUNLINK:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_FRONTENDUNLINK\n"));
            {
                if ((err = copy_from_user( &UserParams.FrontendUnLink, (STPTI_Ioctl_FrontendUnLink_t*)arg, sizeof(STPTI_Ioctl_FrontendUnLink_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.FrontendUnLink.Error = STPTI_FrontendUnLink( UserParams.FrontendUnLink.FrontendHandle );

                if((err = copy_to_user( (STPTI_Ioctl_FrontendUnLink_t*)arg, &UserParams.FrontendUnLink, sizeof(STPTI_Ioctl_FrontendUnLink_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */
#ifdef STPTI_CAROUSEL_SUPPORT
        case STPTI_IOC_CAROUSELALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselAllocate, (STPTI_Ioctl_CarouselAllocate_t*)arg, sizeof(STPTI_Ioctl_CarouselAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselAllocate.Error = STPTI_CarouselAllocate( UserParams.CarouselAllocate.SessionHandle, &(UserParams.CarouselAllocate.CarouselHandle) );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselAllocate_t*)arg, &UserParams.CarouselAllocate, sizeof(STPTI_Ioctl_CarouselAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselDeallocate, (STPTI_Ioctl_CarouselDeallocate_t*)arg, sizeof(STPTI_Ioctl_CarouselDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselDeallocate.Error = STPTI_CarouselDeallocate( UserParams.CarouselDeallocate.CarouselHandle );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselDeallocate_t*)arg, &UserParams.CarouselDeallocate, sizeof(STPTI_Ioctl_CarouselDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELENTRYALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELENTRYALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselEntryAllocate, (STPTI_Ioctl_CarouselEntryAllocate_t*)arg, sizeof(STPTI_Ioctl_CarouselEntryAllocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselEntryAllocate.Error = STPTI_CarouselEntryAllocate( UserParams.CarouselEntryAllocate.SessionHandle, &(UserParams.CarouselEntryAllocate.CarouselEntryHandle) );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselEntryAllocate_t*)arg, &UserParams.CarouselEntryAllocate, sizeof(STPTI_Ioctl_CarouselEntryAllocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELENTRYDEALLOCATE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELENTRYDEALLOCATE\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselEntryDeallocate, (STPTI_Ioctl_CarouselEntryDeallocate_t*)arg, sizeof(STPTI_Ioctl_CarouselEntryDeallocate_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselEntryDeallocate.Error = STPTI_CarouselEntryDeallocate( UserParams.CarouselEntryDeallocate.CarouselEntryHandle );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselEntryDeallocate_t*)arg, &UserParams.CarouselEntryDeallocate, sizeof(STPTI_Ioctl_CarouselEntryDeallocate_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELENTRYLOAD:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELENTRYLOAD\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselEntryLoad, (STPTI_Ioctl_CarouselEntryLoad_t*)arg, sizeof(STPTI_Ioctl_CarouselEntryLoad_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselEntryLoad.Error = STPTI_CarouselEntryLoad( UserParams.CarouselEntryLoad.CarouselEntryHandle, UserParams.CarouselEntryLoad.Packet, UserParams.CarouselEntryLoad.FromByte  );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselEntryLoad_t*)arg, &UserParams.CarouselEntryLoad, sizeof(STPTI_Ioctl_CarouselEntryLoad_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;        
        case STPTI_IOC_CAROUSELGETCURRENTENTRY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELGETCURRENTENTRY\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselGetCurrentEntry, (STPTI_Ioctl_CarouselGetCurrentEntry_t*)arg, sizeof(STPTI_Ioctl_CarouselGetCurrentEntry_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselGetCurrentEntry.Error = STPTI_CarouselGetCurrentEntry( UserParams.CarouselGetCurrentEntry.CarouselHandle, &(UserParams.CarouselGetCurrentEntry.Entry), &(UserParams.CarouselGetCurrentEntry.PrivateData) );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselGetCurrentEntry_t*)arg, &UserParams.CarouselGetCurrentEntry, sizeof(STPTI_Ioctl_CarouselGetCurrentEntry_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELINSERTENTRY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELINSERTENTRY\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselInsertEntry, (STPTI_Ioctl_CarouselInsertEntry_t*)arg, sizeof(STPTI_Ioctl_CarouselInsertEntry_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselInsertEntry.Error = STPTI_CarouselInsertEntry( UserParams.CarouselInsertEntry.CarouselHandle,
                                                                                  UserParams.CarouselInsertEntry.AlternateOutputTag,
                                                                                  UserParams.CarouselInsertEntry.InjectionType,
                                                                                  UserParams.CarouselInsertEntry.Entry );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselInsertEntry_t*)arg, &UserParams.CarouselInsertEntry, sizeof(STPTI_Ioctl_CarouselInsertEntry_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELINSERTTIMEDENTRY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELINSERTTIMEDENTRY\n"));
            {
                STPTI_TSPacket_t TSPacket;

                if ((err = copy_from_user( &UserParams.CarouselInsertTimedEntry, (STPTI_Ioctl_CarouselInsertTimedEntry_t*)arg, sizeof(STPTI_Ioctl_CarouselInsertTimedEntry_t)) )){
                /* Invalid user space address */
                    goto fail;
                }
                /* Copy the TSPacket from user space */
                else if ((err = copy_from_user( &TSPacket, UserParams.CarouselInsertTimedEntry.TSPacket_p, sizeof(STPTI_TSPacket_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselInsertTimedEntry.Error = STPTI_CarouselInsertTimedEntry( UserParams.CarouselInsertTimedEntry.CarouselHandle,
                                                                                            (U8*)(&TSPacket),
                                                                                            UserParams.CarouselInsertTimedEntry.CCValue,
                                                                                            UserParams.CarouselInsertTimedEntry.InjectionType,
                                                                                            UserParams.CarouselInsertTimedEntry.MinOutputTime,
                                                                                            UserParams.CarouselInsertTimedEntry.MaxOutputTime,
                                                                                            UserParams.CarouselInsertTimedEntry.EventOnOutput,
                                                                                            UserParams.CarouselInsertTimedEntry.EventOnTimeout,
                                                                                            UserParams.CarouselInsertTimedEntry.PrivateData,
                                                                                            UserParams.CarouselInsertTimedEntry.Entry,
                                                                                            UserParams.CarouselInsertTimedEntry.FromByte );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselInsertTimedEntry_t*)arg, &UserParams.CarouselInsertTimedEntry, sizeof(STPTI_Ioctl_CarouselInsertTimedEntry_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELLINKTOBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELLINKTOBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselLinkToBuffer, (STPTI_Ioctl_CarouselLinkToBuffer_t*)arg, sizeof(STPTI_Ioctl_CarouselLinkToBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselLinkToBuffer.Error = STPTI_CarouselLinkToBuffer( UserParams.CarouselLinkToBuffer.CarouselHandle,
                                                                                  UserParams.CarouselLinkToBuffer.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselLinkToBuffer_t*)arg, &UserParams.CarouselLinkToBuffer, sizeof(STPTI_Ioctl_CarouselLinkToBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELLOCK:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELLOCK\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselLock, (STPTI_Ioctl_CarouselLock_t*)arg, sizeof(STPTI_Ioctl_CarouselLock_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselLock.Error = STPTI_CarouselLock( UserParams.CarouselLock.CarouselHandle );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselLock_t*)arg, &UserParams.CarouselLock, sizeof(STPTI_Ioctl_CarouselLock_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELREMOVEENTRY:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELREMOVEENTRY\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselRemoveEntry, (STPTI_Ioctl_CarouselRemoveEntry_t*)arg, sizeof(STPTI_Ioctl_CarouselRemoveEntry_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselRemoveEntry.Error = STPTI_CarouselRemoveEntry( UserParams.CarouselRemoveEntry.CarouselHandle,
                                                                                  UserParams.CarouselRemoveEntry.Entry );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselRemoveEntry_t*)arg, &UserParams.CarouselRemoveEntry, sizeof(STPTI_Ioctl_CarouselRemoveEntry_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELSETALLOWOUTPUT:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELSETALLOWOUTPUT\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselSetAllowOutput, (STPTI_Ioctl_CarouselSetAllowOutput_t*)arg, sizeof(STPTI_Ioctl_CarouselSetAllowOutput_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselSetAllowOutput.Error = STPTI_CarouselSetAllowOutput( UserParams.CarouselSetAllowOutput.CarouselHandle,
                                                                                        UserParams.CarouselSetAllowOutput.OutputAllowed );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselSetAllowOutput_t*)arg, &UserParams.CarouselSetAllowOutput, sizeof(STPTI_Ioctl_CarouselSetAllowOutput_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELSETBURSTMODE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELSETBURSTMODE\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselSetBurstMode, (STPTI_Ioctl_CarouselSetBurstMode_t*)arg, sizeof(STPTI_Ioctl_CarouselSetBurstMode_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselSetBurstMode.Error = STPTI_CarouselSetBurstMode( UserParams.CarouselSetBurstMode.CarouselHandle,
                                                                                    UserParams.CarouselSetBurstMode.PacketTimeMs,
                                                                                    UserParams.CarouselSetBurstMode.CycleTimeMs );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselSetBurstMode_t*)arg, &UserParams.CarouselSetBurstMode, sizeof(STPTI_Ioctl_CarouselSetBurstMode_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELUNLINKBUFFER:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELUNLINKBUFFER\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselUnLinkBuffer, (STPTI_Ioctl_CarouselUnlinkBuffer_t*)arg, sizeof(STPTI_Ioctl_CarouselUnlinkBuffer_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselUnLinkBuffer.Error = STPTI_CarouselUnlinkBuffer( UserParams.CarouselUnLinkBuffer.CarouselHandle,
                                                                                    UserParams.CarouselUnLinkBuffer.BufferHandle );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselUnlinkBuffer_t*)arg, &UserParams.CarouselUnLinkBuffer, sizeof(STPTI_Ioctl_CarouselUnlinkBuffer_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
        case STPTI_IOC_CAROUSELUNLOCK:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_CAROUSELUNLOCK\n"));
            {
                if ((err = copy_from_user( &UserParams.CarouselUnlock, (STPTI_Ioctl_CarouselUnLock_t*)arg, sizeof(STPTI_Ioctl_CarouselUnLock_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.CarouselUnlock.Error = STPTI_CarouselUnLock( UserParams.CarouselUnlock.CarouselHandle );

                if((err = copy_to_user( (STPTI_Ioctl_CarouselUnLock_t*)arg, &UserParams.CarouselUnlock, sizeof(STPTI_Ioctl_CarouselUnLock_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;
#endif /* #ifdef STPTI_CAROUSEL_SUPPORT */

        case STPTI_IOC_OBJECTENABLEFEATURE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_OBJECTENABLEFEATURE\n"));
            {
                if ((err = copy_from_user( &UserParams.EnableFeature, (STPTI_Ioctl_ObjectEnableFeature_t*)arg, sizeof(STPTI_Ioctl_ObjectEnableFeature_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.EnableFeature.Error = STPTI_ObjectEnableFeature( UserParams.EnableFeature.ObjectHandle, UserParams.EnableFeature.FeatureInfo );

                if((err = copy_to_user( (STPTI_Ioctl_ObjectEnableFeature_t*)arg, &UserParams.EnableFeature, sizeof(STPTI_Ioctl_ObjectEnableFeature_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_OBJECTDISABLEFEATURE:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_OBJECTDISABLEFEATURE\n"));
            {
                if ((err = copy_from_user( &UserParams.DisableFeature, (STPTI_Ioctl_ObjectDisableFeature_t*)arg, sizeof(STPTI_Ioctl_ObjectDisableFeature_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.DisableFeature.Error = STPTI_ObjectDisableFeature( UserParams.DisableFeature.ObjectHandle, UserParams.DisableFeature.Feature );

                if((err = copy_to_user( (STPTI_Ioctl_ObjectDisableFeature_t*)arg, &UserParams.DisableFeature, sizeof(STPTI_Ioctl_ObjectDisableFeature_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;            

        case STPTI_IOC_PRINTDEBUG:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_PRINTDEBUG\n"));
            {
                if ((err = copy_from_user( &UserParams.PrintDebug, (STPTI_Ioctl_PrintDebug_t*)arg, sizeof(STPTI_Ioctl_PrintDebug_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                UserParams.PrintDebug.Error = STPTI_PrintDebug( UserParams.PrintDebug.DeviceName );

                if((err = copy_to_user( (STPTI_Ioctl_PrintDebug_t*)arg, &UserParams.PrintDebug, sizeof(STPTI_Ioctl_PrintDebug_t)))) {
                    /* Invalid user space address */
                    goto fail;
                }
            }
            break;

        case STPTI_IOC_DEBUG:
            STPTI_PRINTK_IOCTL(("stpti_ioctl: STPTI_IOC_DEBUG\n"));
            {
                char *string;
                if ((err = copy_from_user( &UserParams.Debug, (STPTI_Ioctl_PrintDebug_t*)arg, sizeof(STPTI_Ioctl_Debug_t)) )){
                /* Invalid user space address */
                    goto fail;
                }

                string = (char *)kmalloc(sizeof(char)*UserParams.Debug.string_size,GFP_KERNEL);
                assert( string != NULL );

                UserParams.Debug.Error = STPTI_Debug( UserParams.Debug.DeviceName, UserParams.Debug.dbg_class, string, UserParams.Debug.string_size );
                
                if((err = copy_to_user( ((STPTI_Ioctl_Debug_t*)arg)->string, string, UserParams.Debug.string_size ))){
                    /* Invalid user space address */
                    kfree( string );
                    goto fail;
                }

                kfree( string );

                if((err = copy_to_user( (STPTI_Ioctl_Debug_t*)arg, &UserParams.Debug, sizeof(STPTI_Ioctl_Debug_t)))) {
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

static void *uvirt_to_kvirt(struct mm_struct *mm, unsigned long va)
{
    void* kaddr = (void*)-1;
    pgd_t *pgd = pgd_offset(mm, va);          /* page directory */
    pud_t *pud;
    pmd_t *pmd;                               /* page middle directory */
    pte_t pte;                                /* page table entry */

    /* Find the physical page address */
    if (!pgd_none(*pgd))
    {
        pud = pud_offset(pgd, va);
        if (!pud_none(*pud))
        {
            pmd = pmd_offset(pud, va);
            if (!pmd_none(*pmd))
            {
                pte = *pte_offset_kernel(pmd, va);
                if (pte_present(pte))
                {
                    kaddr = page_address(pte_page(pte));
                }
            }
        }
    }
    return kaddr;
}


