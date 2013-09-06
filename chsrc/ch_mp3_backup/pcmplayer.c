/****************************************************************************

File Name   : testapp4.c
Description : Test application for testing the audio features.

Copyright (C) ST Microelectronics 2007

References  :
****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "staudlx.h"
#include "stcommon.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stsys.h"
#include "sttbx.h"
#include "stvtg.h"
#include "stapp_os.h"
#include "pcmplayer.h"
#include "..\main\errors.h"
#include "ch_Mp3.h"

#define AUD_MAX_NUMBER 1

extern ST_Partition_t*   SystemPartition;
extern STAVMEM_PartitionHandle_t AVMEMHandle[2];
extern STAUD_Handle_t  AUDHandle;
extern ST_DeviceName_t AUDDeviceName;
extern U8              gs_PCMInjectProcessPriority;

AUD_InjectContext_t* AUDi_InjectContext[AUD_MAX_NUMBER];

/* Exported Variables -------------------------------------------------- 
semaphore_t   *Play_Next_File;*/

/* Functions ---------------------------------------------------------- */

/*------------------------------------------------------------------------------
 * Function    : AUDi_InjectCopy_CPU
 * Description : Copy datas in the audio decoder
 * Input       : *AUD_InjectContext, SizeToCopy
 * Output      :
 * Return      : void
 * --------------------------------------------------------------------------- */
static void AUDi_InjectCopy_CPU(AUD_InjectContext_t *AUD_InjectContext,U32 SizeToCopy)
{
    U32 SizeToCopy1, SizeToCopy2;
    /* !!! First SizeToCopy has to be less than the pes audio buffer !!! */

    /* Get size to inject */
    /* ------------------ */
    SizeToCopy1 = SizeToCopy;
    SizeToCopy2 = 0;

    /* Check if we have to split the copy into pieces into the destination buffer */
    /* -------------------------------------------------------------------------- */
    if ((AUD_InjectContext->BufferAudioProducer + SizeToCopy) > (AUD_InjectContext->BufferAudio + AUD_InjectContext->BufferAudioSize))
    {
        SizeToCopy1 = (AUD_InjectContext->BufferAudio + AUD_InjectContext->BufferAudioSize) - AUD_InjectContext->BufferAudioProducer;
        SizeToCopy2 = SizeToCopy - SizeToCopy1;
    }

    /* First piece */
    /* ----------- */
    memcpy((U8 *)(AUD_InjectContext->BufferAudioProducer|0xA0000000), (U8 *)AUD_InjectContext->BufferConsumer, SizeToCopy1);
    semaphore_wait(AUD_InjectContext->SemLock);
    AUD_InjectContext->BufferAudioProducer += SizeToCopy1;
    AUD_InjectContext->BufferConsumer      += SizeToCopy1;
    if ((AUD_InjectContext->BufferAudioProducer) == (AUD_InjectContext->BufferAudio + AUD_InjectContext->BufferAudioSize))
    {
        AUD_InjectContext->BufferAudioProducer = AUD_InjectContext->BufferAudio;
    }
    semaphore_signal(AUD_InjectContext->SemLock);

    /* Second piece */
    /* ------------ */
    if (SizeToCopy2 != 0)
    {
        AUD_InjectContext->BufferAudioProducer = AUD_InjectContext->BufferAudio;
        memcpy((U8 *)(AUD_InjectContext->BufferAudioProducer|0xA0000000), (U8 *)AUD_InjectContext->BufferConsumer, SizeToCopy2);
        semaphore_wait(AUD_InjectContext->SemLock);
        AUD_InjectContext->BufferAudioProducer += SizeToCopy2;
        AUD_InjectContext->BufferConsumer      += SizeToCopy2;
        if ((AUD_InjectContext->BufferAudioProducer) == (AUD_InjectContext->BufferAudio + AUD_InjectContext->BufferAudioSize))
        {
            AUD_InjectContext->BufferAudioProducer = AUD_InjectContext->BufferAudio;
        }
        semaphore_signal(AUD_InjectContext->SemLock);
    }

    /* Trace for debug */
    /* --------------- */
    /* STTBX_Print(("AUDi_InjectCopy(%d): Size1 = 0x%08x - Size2 = 0x%08x - SrcPtr = 0x%08x - DstPtr = 0x%08x\n",AUD_InjectContext->InjectorId,SizeToCopy1,SizeToCopy2,AUD_InjectContext->BufferConsumer,AUD_InjectContext->BufferAudioProducer));*/
} /* AUDi_InjectCopy_CPU */

/*------------------------------------------------------------------------------
 * Function    : AUDi_InjectCopy
 * Description : Copy datas in the audio decoder
 * Input       : *AUD_InjectContext, SizeToCopy
 * Output      :
 * Return      : void
 * --------------------------------------------------------------------------- */
static void AUDi_InjectCopy(AUD_InjectContext_t *AUD_InjectContext, U32 SizeToCopy)
{
    AUDi_InjectCopy_CPU(AUD_InjectContext, SizeToCopy);
} /* AUDi_InjectCopy */

/*------------------------------------------------------------------------------
 * Function    : AUD_PcmConfig
 * Description : Configuration
 * Input       : DeviceId, PCMInputParams, InputObject
 *
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_PcmConfig(AUD_DeviceId_t DeviceId, STAUD_PCMInputParams_t PCMInputParams, STAUD_Object_t InputObject)
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    ErrCode = STAUD_IPSetPCMParams (AUDHandle, InputObject, &PCMInputParams);
    if(ErrCode != ST_NO_ERROR)
    {
           STTBX_Print(("[MP3 Player]STAUD_IPSetPCMParams() -> FAILED(%d)", ErrCode));
           return ErrCode;
    }
    else
    {
        STTBX_Print(("[MP3 Player]STAUD_IPSetPCMParams() -> OK"));
    }

    return ErrCode;
}

void PCM_SetAttenuation(U32 CurVolume)
{
	STAUD_Attenuation_t Attenuation;

	if(CurVolume < 10)
	{
		CurVolume = 10;
	}
	else if(CurVolume > 53)
	{
		CurVolume = 53;
	}
	
	Attenuation.Left  = CurVolume;
	Attenuation.Right = CurVolume;
	STAUD_SetAttenuation(AUDHandle, Attenuation);
}

void PCM_MuteAudio(void)
{
	ST_ErrorCode_t error;
	
	PCM_SetAttenuation(0); 
	error =STAUD_Mute(AUDHandle,TRUE,TRUE);
	if ( error != ST_NO_ERROR )
		STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Mute=%s", GetErrorText(error) ));
	
}

void PCM_UnmuteAudio(U32 CurVolume)
{

	int error;
	
#if 0/*yxl 2008-04-15 temp modify*/

	error = STAUD_Mute(AUDHandle,FALSE, FALSE);
	if ( error != ST_NO_ERROR )
	STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Mute=%s", GetErrorText(error)));

    PCM_SetAttenuation(CurVolume);
#else
    PCM_SetAttenuation(CurVolume);
	task_delay(25000);
	error = STAUD_Mute(AUDHandle,FALSE, FALSE);
	if ( error != ST_NO_ERROR )
	STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_Mute=%s", GetErrorText(error)));


#endif /*end yxl 2008-04-15 temp modify*/
}

/*------------------------------------------------------------------------------
 * Function    : AUD_Start
 * Description : Start audio driver
 * Input       : DeviceId, AUD_StartParams
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Start(AUD_DeviceId_t  DeviceId, STAUD_StreamContent_t StreamContent , U32 freq, STAUD_StreamType_t StreamType)
{
    STAUD_StreamParams_t    AUD_StreamParams;
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;

    AUD_StreamParams.SamplingFrequency = freq;
    AUD_StreamParams.StreamID          = STAUD_IGNORE_ID;
    AUD_StreamParams.StreamType        = StreamType;
    AUD_StreamParams.StreamContent     = StreamContent;
    U32 		SamplingFrequency;
    U32            CutId = ST_GetCutRevision();



    ErrCode = STAUD_DRStart(AUDHandle, STAUD_OBJECT_DECODER_COMPRESSED0, &AUD_StreamParams);

#if 1 /*yxl 2008-04-14 temp modify*/
    STAUD_IPGetSamplingFrequency(AUDHandle, STAUD_OBJECT_INPUT_CD0, &SamplingFrequency );
	
#else
	STAUD_IPGetSamplingFrequency(AUDHandle, STAUD_OBJECT_INPUT_CD1, &SamplingFrequency );
#endif /*end yxl 2008-04-14 temp modify*/
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print((
                        "[MP3 Player]STAUD_DRStart(%s, 0x%x, %d, %d, F:%dHz, Id: %02.2x) -> FAILED ",
                        AUDDeviceName,
                        AUDHandle,
                        AUD_StreamParams.StreamContent,
                        AUD_StreamParams.StreamType,
                        SamplingFrequency,
                        AUD_StreamParams.StreamID));
        return ErrCode;
    }
    else
    {
        STTBX_Print((
                      "[MP3 Player]STAUD_DRStart(%s, 0x%x, %d, %d, F:%dHz, Id: %02.2x) -> OK ",
                      AUDDeviceName,
                      AUDHandle,
                      AUD_StreamParams.StreamContent,
                      AUD_StreamParams.StreamType,
                      AUD_StreamParams.SamplingFrequency,/*??????不知道为什么读不出来SamplingFrequency ???*/
                      AUD_StreamParams.StreamID));
    }
#if 0
	ErrCode = STAUD_OPEnableSynchronization(AUDHandle, STAUD_OBJECT_OUTPUT_PCMP1);
    if (ErrCode == ST_NO_ERROR)
    {
        STTBX_Print(( "STAUD_DREnableSynchronization on  PCMP (%s)  -> OK",AUDDeviceName));
    }
    else
    {
        STTBX_Print(( "STAUD_DREnableSynchronization on  PCMP(%s)  -> FAILED(%d)",AUDDeviceName, ErrCode));

    }
#endif
    
    return(ErrCode);
} /* AUD_Start */

static ST_ErrorCode_t AUDi_GetWritePtr(void *const Handle,void **const Address)
{
    AUD_InjectContext_t *AUD_InjectContext;

    /* NULL address are ignored ! */
    /* -------------------------- */
    if (Address == NULL)
    {
        STTBX_Print(("[MP3 Player]AUDi_SetReadPtr(?):**ERROR** !!! Address is NULL pointer !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve context */
    /* ---------------- */
    AUD_InjectContext = (AUD_InjectContext_t *)Handle;
    if (AUD_InjectContext == NULL)
    {
        STTBX_Print(("[MP3 Player]AUDi_GetWritePtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Set the value of the write pointer */
    /* ---------------------------------- */
    semaphore_wait(AUD_InjectContext->SemLock);
    *Address = (void *)((U32)(AUD_InjectContext->BufferAudioProducer)|0xA0000000);
    semaphore_signal(AUD_InjectContext->SemLock);

    /* Return no errors */
    /* ---------------- */
    /*STTBX_Print(("AUDi_GetWritePtr(%d): Address=0x%08x\n", AUD_InjectContext->InjectorId, *Address));*/
    return (ST_NO_ERROR);
}

/*------------------------------------------------------------------------------
 * Function    : AUDi_SetReadPtr
 * Description : Set the read pointer of the back buffer
 * Input       : Handle, Address
 * Output      :
 * Return      : Error code
 * --------------------------------------------------------------------------- */
static ST_ErrorCode_t AUDi_SetReadPtr(void *const Handle,void *const Address)
{
    AUD_InjectContext_t *AUD_InjectContext;

    /* NULL address are ignored ! */
    /* -------------------------- */
    if (Address    ==    NULL)
    {
        STTBX_Print(("[MP3 Player]AUDi_SetReadPtr(?):**ERROR** !!! Address is NULL pointer !!!\n"));
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Retrieve context */
    /* ---------------- */
    AUD_InjectContext = (AUD_InjectContext_t *)Handle;
    if (AUD_InjectContext == NULL)
    {
        STTBX_Print(("[MP3 Player]AUDi_SetReadPtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Set the value of the read pointer */
    /* --------------------------------- */
    semaphore_wait(AUD_InjectContext->SemLock);
    AUD_InjectContext->BufferAudioConsumer = (U32)Address;
    semaphore_signal(AUD_InjectContext->SemLock);

    /* Return no errors */
    /* ---------------- */
    /*STTBX_Print(("AUDi_SetReadPtr(%d): Address=0x%08x\n", AUD_InjectContext->InjectorId, Address));*/
    return (ST_NO_ERROR);
} /* AUDi_SetReadPtr */

/*------------------------------------------------------------------------------
 * Function    : AUDi_InjectGetFreeSize
 * Description : Get free size able to be injected
 * Input       : *AUD_InjectContext
 * Output      :
 * Return      : U32
 * --------------------------------------------------------------------------- */
static U32 AUDi_InjectGetFreeSize(AUD_InjectContext_t *AUD_InjectContext)
{
    U32 FreeSize;

    /* Get free size into the PES buffer */
    /* --------------------------------- */
    semaphore_wait(AUD_InjectContext->SemLock);
    if (AUD_InjectContext->BufferAudioProducer>=AUD_InjectContext->BufferAudioConsumer)
    {
        FreeSize = (AUD_InjectContext->BufferAudioSize -(AUD_InjectContext->BufferAudioProducer - AUD_InjectContext->BufferAudioConsumer));
    }
    else
    {
        FreeSize = (AUD_InjectContext->BufferAudioConsumer - AUD_InjectContext->BufferAudioProducer);
    }
    semaphore_signal(AUD_InjectContext->SemLock);

    /* Align it on 256 bytes to use external DMA */
    /* ----------------------------------------- */
    /* Here we assume the producer will be never equal to base+size           */
    /* If it's the case, this is managed into the fill producer procedure     */
    /* The audio decoder is working like this, if ptr=base+size, the ptr=base */
    if (FreeSize <= 16)
        FreeSize = 0;
    else
        FreeSize = FreeSize - 16;
    FreeSize = (FreeSize/256) * 256;
    return FreeSize;
} /* AUDi_InjectGetFreeSize */


void AUDi_InjectTask(void *InjectContext)
{
    AUD_InjectContext_t *AUD_InjectContext;
    U32              SizeToInject;
    U32              SizeAvailable;
    U32              SizeToRead, iError;
	char 		Delay = 0, DataOver = 0;
    /* Retrieve context */
    /* ================ */
    AUD_InjectContext = (AUD_InjectContext_t *)InjectContext;

    /* Wait start of play */
    /* ================== */
    semaphore_wait(AUD_InjectContext->SemStart);
    SizeAvailable = SizeToInject = 0;
	AUD_InjectContext->BufferSize = 0;
	
    /* Infinite loop */
    /* ============= */
    while(AUD_InjectContext->State==AUD_INJECT_START)
    {
		 /*有效注入buffer长度*/
         SizeAvailable = AUD_InjectContext->BufferSize;
	Delay = 0;
		/*当注入的缓存播出完毕*/
        if ( ((AUD_InjectContext->BufferConsumer) == (AUD_InjectContext->BufferBase + AUD_InjectContext->BufferSize))
			 || CH_MP3_CheckBreakForNew() )
        {
			#if 0
			CH_GetAACData((U16 *)AUD_InjectContext->BufferBase,&SizeAvailable);
			#else
#ifdef IPANEL_AAC_FILE_PLAY
			if(CH_get_AAC_play_state())
			{
				CH_GetDecodedAACData((U16 *)AUD_InjectContext->BufferBase,&SizeAvailable);
			}
			else
#endif /* IPANEL_AAC_FILE_PLAY */
			{
				CH_GetDecodedPCMData((U16 *)AUD_InjectContext->BufferBase,&SizeAvailable);
			}
			#endif
			if(SizeAvailable)
			{
				eis_report("[%d]", SizeAvailable);
				Delay = 1;
				DataOver ++;
			}
			AUD_InjectContext->BufferSize = SizeAvailable;
			SizeToInject =  0;
			AUD_InjectContext->BufferConsumer = AUD_InjectContext->BufferBase;
			AUD_InjectContext->BufferProducer = AUD_InjectContext->BufferBase;
			//do_report(0,"\n111111111");
			
        }
	
	if(DataOver)
	{
		if(DataOver == 2)
		{
			CH_IPANEL_MP3CallBack();
			DataOver = 0;
			Delay = 1;
		}
		else
		{
			DataOver ++;
		}
	}
	if(Delay == 0)
	{
		 /* Sleep a while */
        	/* ------------- */
        	Task_Delay(ST_GetClocksPerSecond() / 100);  /*Sleep 10ms */
	}
        /* Do we have to datas to inject */
        /* ----------------------------- */
        if ((SizeToInject = AUDi_InjectGetFreeSize(AUD_InjectContext)) != 0)
        {
            /* Check if we have to split the copy into pieces into the source buffer */
            if ((AUD_InjectContext->BufferConsumer + SizeToInject) >= (AUD_InjectContext->BufferBase + AUD_InjectContext->BufferSize))
            {
                SizeToInject = (AUD_InjectContext->BufferBase + AUD_InjectContext->BufferSize)-(AUD_InjectContext->BufferConsumer);
            }
            if (SizeToInject >= SizeAvailable)    SizeToInject = SizeAvailable;
            /* Inject the piece of datas */
            if (SizeToInject != 0)   
	   {
		AUDi_InjectCopy(AUD_InjectContext,SizeToInject);
            }
            SizeAvailable = SizeAvailable - SizeToInject;

        }
    }

    /* Finish the task */
    /* =============== */
    semaphore_signal(AUD_InjectContext->SemStop);
    /*semaphore_signal(Play_Next_File);*/
} /* AUDi_InjectTask */


#if 1 /*yxl 2008-08-25 cancel below function*/
/*------------------------------------------------------------------------------
 * Function    : Audio_Allocate
 * Description : Allocate Avmem block
 * Input       : PartitionHandle,Size,Alignment,**Address_p,*Handle_p
 * Output      :
 * Return      : Error code
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t  Audio_Allocate(STAVMEM_PartitionHandle_t PartitionHandle, U32 Size, U32 Alignment, void **Address_p, STAVMEM_BlockHandle_t *Handle_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    STAVMEM_AllocBlockParams_t AvmemAllocParams;
    void * VirtualAddress;

    *Handle_p = STAVMEM_INVALID_BLOCK_HANDLE;
    AvmemAllocParams.PartitionHandle = PartitionHandle;
    AvmemAllocParams.Size = Size;
    AvmemAllocParams.Alignment = Alignment;
    AvmemAllocParams.AllocMode = STAVMEM_ALLOC_MODE_BOTTOM_TOP;
    AvmemAllocParams.NumberOfForbiddenRanges = 0;
    AvmemAllocParams.ForbiddenRangeArray_p = NULL;
    AvmemAllocParams.NumberOfForbiddenBorders = 0;
    AvmemAllocParams.ForbiddenBorderArray_p = NULL;
    ErrCode = STAVMEM_AllocBlock(&AvmemAllocParams, Handle_p);
    if (ErrCode == ST_NO_ERROR)
    {
        ErrCode = STAVMEM_GetBlockAddress(*Handle_p, &VirtualAddress);
        *Address_p = VirtualAddress;
    }
    return (ErrCode);
}

/*------------------------------------------------------------------------------
 * Function    : Audio_Deallocate
 * Description : Deallocate Avmem block
 * Input       : PartitionHandle,*Handle_p
 * Output      :
 * Return      : None
 * --------------------------------------------------------------------------- */
void  Audio_Deallocate(STAVMEM_PartitionHandle_t PartitionHandle, STAVMEM_BlockHandle_t *Handle_p)
{
    STAVMEM_FreeBlockParams_t    FreeParams;

    FreeParams.PartitionHandle = PartitionHandle;
    STAVMEM_FreeBlock(&FreeParams, Handle_p);
}

#endif /*end yxl 2008-08-25 cancel below function*/


/*------------------------------------------------------------------------------
 * Function    : AUD_InjectStart
 * Description : Start an audio injection
 * Input       : DeviceId,, *AUD_StartParams
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_InjectStart(U32 DeviceId, U32 ContextId, STAUD_Object_t InputObject, BOOL FileIsByteReversed)
{
    U8								*FILE_Buffer;
    U8								*FILE_Buffer_Aligned;
    U16								*FILE_Buffer_Aligned1;
    U32								FILE_BufferSize;
    ST_ErrorCode_t					ErrCode = ST_NO_ERROR;
    STAUD_BufferParams_t			AUD_BufferParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;


	/*yxl 2008-04-09 add below section*/
	{
		extern STAUD_BufferParams_t AUDBufferParams;
		memcpy((void*)&AUD_BufferParams,(const void*)&AUDBufferParams,sizeof(STAUD_BufferParams_t));
	}

	/*end yxl 2008-04-09 add below section*/

    /* If the injecter is already in place, go out */
    /* ------------------------------------------- */
    if (AUDi_InjectContext[ContextId]!=NULL)
    {
        STTBX_Print(("[MP3 Player]AUD_InjectStart(%d):**ERROR** !!! Injector %d  is already running !!!\n", ContextId));
        return (ST_ERROR_ALREADY_INITIALIZED);
    }

	/* Allocate a context */
	AUDi_InjectContext[ContextId]=memory_allocate(SystemPartition,sizeof(AUD_InjectContext_t));
	if (AUDi_InjectContext[ContextId]==NULL)
	{
		STTBX_Print(("[MP3 Player]AUD_InjectStart(%d):**ERROR** !!! Unable to allocate memory !!!\n",ContextId));
			return(ST_ERROR_NO_MEMORY);
	}
	memset(AUDi_InjectContext[ContextId],0,sizeof(AUD_InjectContext_t));


    /* -------------------------- */
    /* This file is coming from an external file system, try to load all the file into memory */
    FILE_BufferSize     = PCM_INJECT_BUFFER_SIZE;
	if(Audio_Allocate(AVMEMHandle[1], FILE_BufferSize+256, 256, (void **)&FILE_Buffer,
                                                                  &(AUDi_InjectContext[ContextId]->Block_Handle_p))!=ST_NO_ERROR)
    {
        STTBX_Print(("[MP3 Player]Error Audio Allocate\n"));
        return(ST_ERROR_NO_MEMORY);
    }
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
    FILE_Buffer_Aligned  = STAVMEM_VirtualToCPU(FILE_Buffer, &VirtualMapping);
    FILE_Buffer_Aligned1 = STAVMEM_VirtualToCPU(FILE_Buffer, &VirtualMapping);
    if(!FileIsByteReversed)
    {
        FILE_Buffer_Aligned = (U8 *)((((U32)FILE_Buffer)+255)&0xFFFFFF00);
    }
    else
    {
        FILE_Buffer_Aligned1 = (U16 *)((((U32)FILE_Buffer)+255)&0xFFFFFF00);
    }

    if (FILE_Buffer_Aligned==NULL)
    {
        STTBX_Print(("[MP3 Player]AUD_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n", ContextId));
        return (ST_ERROR_NO_MEMORY);
    }
	
    /* Fill up the context */
    /* ------------------- */
    AUDi_InjectContext[ContextId]->AUDIO_Handler       = AUDHandle;
    AUDi_InjectContext[ContextId]->InjectorId          = DeviceId;
    AUDi_InjectContext[ContextId]->BufferPtr           = (U32)FILE_Buffer;
    AUDi_InjectContext[ContextId]->BufferBase          = (U32)FILE_Buffer_Aligned;
    AUDi_InjectContext[ContextId]->BufferConsumer      = (U32)FILE_Buffer_Aligned;
    AUDi_InjectContext[ContextId]->BufferProducer      = (U32)FILE_Buffer_Aligned;
    AUDi_InjectContext[ContextId]->BufferSize          = FILE_BufferSize;
    AUDi_InjectContext[ContextId]->State               = AUD_INJECT_SLEEP;

    /* Create the semaphores */
    /* --------------------- */
    AUDi_InjectContext[ContextId]->SemStart = semaphore_create_fifo(0);
    AUDi_InjectContext[ContextId]->SemStop = semaphore_create_fifo(0);
    AUDi_InjectContext[ContextId]->SemLock = semaphore_create_fifo(1);
    /*Play_Next_File = semaphore_create_fifo(0);*/


	ErrCode |= STAUD_IPGetInputBufferParams(AUDHandle, InputObject, &AUD_BufferParams);/*yxl 2008-04-09 cancel this line,yxl 2008-04-14 reenable this line*/

    AUDi_InjectContext[ContextId]->BitBufferAudioSize  = 0;
    AUDi_InjectContext[ContextId]->BufferAudioProducer = AUDi_InjectContext[ContextId]->BufferAudioConsumer = AUDi_InjectContext[ContextId]->BufferAudio = (U32)AUD_BufferParams.BufferBaseAddr_p;
    AUDi_InjectContext[ContextId]->BufferAudioSize     = AUD_BufferParams.BufferSize;


    ErrCode |=STAUD_IPSetDataInputInterface(AUDHandle, InputObject, AUDi_GetWritePtr, AUDi_SetReadPtr, AUDi_InjectContext[ContextId]);
	if (ErrCode != ST_NO_ERROR)
	{
        STTBX_Print(("[MP3 Player]AUD_InjectStart(%d):**ERROR** !!! Unable to setup the audio buffer !!!\n",ContextId));
			memory_deallocate(SystemPartition, AUDi_InjectContext[ContextId]);
        AUDi_InjectContext[ContextId] = NULL;
		return ErrCode;
	}






    AUDi_InjectContext[ContextId]->Task = Task_Create((void (*)(void*))AUDi_InjectTask, AUDi_InjectContext[ContextId], 64*1024,gs_PCMInjectProcessPriority, "AUD_InjectorTask", 0);
    if (AUDi_InjectContext[ContextId]->Task == NULL)
    {
        STAUD_Fade_t AUD_Fade;
		
        AUD_Fade.FadeType = STAUD_FADE_SOFT_MUTE;
        STAUD_Stop(AUDHandle, STAUD_STOP_NOW, &AUD_Fade);
		memory_deallocate(SystemPartition, AUDi_InjectContext[ContextId]);
        AUDi_InjectContext[ContextId] = NULL;
        STTBX_Print(("[MP3 Player]AUD_InjectStart(%d):**ERROR** !!! Unable to create the injector task !!!\n", ContextId));
        return ErrCode;
    }
	CH_MP3_SetPlayTaskID(AUDi_InjectContext[ContextId]->Task);
    STTBX_Print(("\n[MP3 Player]Task_Create[AUDi_InjectTask]!  \n"));
    /* Start the injector */
    /* ------------------ */
    AUDi_InjectContext[ContextId]->State               = AUD_INJECT_START;
    semaphore_signal(AUDi_InjectContext[ContextId]->SemStart);

    /* Return no errors */
    /* ---------------- */
    return (ST_NO_ERROR);
} /* AUD_InjectStart */

#if 1 /*yxl 2008-08-25 cancel below function*/
/*------------------------------------------------------------------------------
 * Function    : AUD_InjectStop
 * Description : Stop an audio injection
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_InjectStop_A(U32 DeviceId, U32 ContextId)
{
    STAUD_Fade_t   AUD_Fading;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    /* If the injecter is not running, go out */
    /* -------------------------------------- */
    if (AUDi_InjectContext[ContextId] == NULL)
    {
        STTBX_Print(("[MP3 Player]AUD_InjectStop(%d):**ERROR** !!! Injector is not running !!!\n", ContextId));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Stop the task */
    /* ------------- 
    AUDi_InjectContext[ContextId]->State = AUD_INJECT_STOP;
    semaphore_wait(AUDi_InjectContext[ContextId]->SemStop);*/

    /* Stop the audio decoder */
    /* ---------------------- */
    AUD_Fading.FadeType = STAUD_FADE_SOFT_MUTE;
	ErrCode=STAUD_Stop(AUDi_InjectContext[ContextId]->AUDIO_Handler,STAUD_STOP_NOW,&AUD_Fading);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != STAUD_ERROR_DECODER_STOPPED))
    {
        STTBX_Print((
                  "[MP3 Player]STAUD_Stop(%s, %d, %d) -> FAILED",
                  AUDDeviceName,
                  AUDHandle,
                  STAUD_STOP_NOW));
        return ErrCode;
    }
    else
    {
        STTBX_Print((
                      "[MP3 Player]STAUD_Stop(%s, %d, %d) -> OK \n",
                      AUDDeviceName,
                      AUDHandle,
                      STAUD_STOP_NOW));
    }

    /* Delete the inject task */
    /* ---------------------- */
#if 0	
	if(AUDi_InjectContext[ContextId]->Task)
	{
		task_suspend(AUDi_InjectContext[ContextId]->Task);	
		task_kill(AUDi_InjectContext[ContextId]->Task,0,0);
		task_delete(AUDi_InjectContext[ContextId]->Task);	
	}
#else
	if(AUDi_InjectContext[ContextId]->Task)
	{
		AUDi_InjectContext[ContextId]->State  = AUD_INJECT_STOP;
		semaphore_wait(AUDi_InjectContext[ContextId]->SemStop);
		Task_Wait(AUDi_InjectContext[ContextId]->Task);
		task_kill(AUDi_InjectContext[ContextId]->Task,0,0);	
	        Task_Delete(AUDi_InjectContext[ContextId]->Task);
	}
#endif	

    /*STTBX_Print(("[MP3 Player]AUD_InjectStop(%d): Task Deleted! \n", ContextId)); sqzow20100610*/
  
    /* Delete the semaphores */
    /* --------------------- */
    semaphore_delete(AUDi_InjectContext[ContextId]->SemStart);
    semaphore_delete(AUDi_InjectContext[ContextId]->SemStop);
    semaphore_delete(AUDi_InjectContext[ContextId]->SemLock);

    /* Do we have to close the file ? */
    /* ------------------------------ */

    /* Free the injector context */
    /* ------------------------- */
	Audio_Deallocate(AVMEMHandle[1], &(AUDi_InjectContext[ContextId]->Block_Handle_p));
    memory_deallocate(SystemPartition, AUDi_InjectContext[ContextId]);
    AUDi_InjectContext[ContextId] = NULL;

    /* Return no errors */
    /* ---------------- */
    return (ST_NO_ERROR);
} /* AUD_InjectStop */

#endif  /*yxl 2008-08-25 cancel below function*/


ST_ErrorCode_t PCM_SetSamplingFreq(U32 SamplingFrequency)
{
    STAUD_PCMInputParams_t 	PCMInputParams0;
    ST_ErrorCode_t   ErrCode = ST_NO_ERROR;
	
	PCMInputParams0.Frequency = SamplingFrequency;
	PCMInputParams0.DataPrecision = 32;
	PCMInputParams0.NumChannels = 2;
#if 1 /*yxl 2008-04-14 temp modify*/
	ErrCode |= AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD0);
#else
	ErrCode |= AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD1);

#endif /*end yxl 2008-04-14 temp modify*/
	return ErrCode;
}
#ifdef USE_IPANEL

unsigned int PCM_SampFreq=44100;
unsigned int PCM_DataPrecision=16;
unsigned int PCM_NumChannels=1;

boolean CH_SetPCMPara(unsigned int SampFreq, unsigned int DataPrecision,unsigned int NumChannels)
{
    STAUD_PCMInputParams_t 	PCMInputParams0;
    ST_ErrorCode_t   ErrCode = ST_NO_ERROR;
	if((PCM_SampFreq==SampFreq) &&
	(PCM_DataPrecision==DataPrecision) &&
	(PCM_NumChannels==NumChannels))
	return false;
	
	PCM_SampFreq=SampFreq;
	PCM_DataPrecision=DataPrecision;
	PCM_NumChannels=NumChannels;
	do_report(0,"\n CH_SetPCMPara freq=%d,DataPrecision=%d,NumChannels=%d",SampFreq,DataPrecision,NumChannels);
	PCMInputParams0.Frequency = SampFreq;
	PCMInputParams0.DataPrecision = DataPrecision;
	PCMInputParams0.NumChannels = NumChannels;
	ErrCode |= AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD0);
	return true;

}
void CH_CleanAudioBuf( void )
{
	STAUD_Fade_t   AUD_Fading;
	STAUD_StreamParams_t AUD_StartParams0;
	STAUD_PCMInputParams_t 	PCMInputParams0;

	AUD_StartParams0.StreamType        = STAUD_STREAM_TYPE_ES;
	AUD_StartParams0.StreamContent     = STAUD_STREAM_CONTENT_PCM;
	AUD_StartParams0.StreamID          = STAUD_IGNORE_ID;
	AUD_StartParams0.SamplingFrequency = PCM_SampFreq;
	
	PCMInputParams0.Frequency = PCM_SampFreq;
	PCMInputParams0.DataPrecision = PCM_DataPrecision;
	PCMInputParams0.NumChannels = PCM_NumChannels;

	AUD_Fading.FadeType = STAUD_FADE_SOFT_MUTE;
	STAUD_Stop(AUDi_InjectContext[0]->AUDIO_Handler,STAUD_STOP_NOW,&AUD_Fading);
	Task_Delay(ST_GetClocksPerSecond() / 2);  
	AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD0);
	AUD_Start(AUD_INT, AUD_StartParams0.StreamContent, AUD_StartParams0.SamplingFrequency, AUD_StartParams0.StreamType);
	
}
ST_ErrorCode_t AAC_Comp_Start(int StreamContent , int sample_rate)
{
    STAUD_StreamParams_t AUD_StartParams0;
    STAUD_PCMInputParams_t 	PCMInputParams0;
    ST_ErrorCode_t   ErrCode = ST_NO_ERROR;

	//获得mp3采样频率
	
    AUD_StartParams0.StreamType        = STAUD_STREAM_TYPE_ES;
    AUD_StartParams0.StreamContent     = StreamContent;
    AUD_StartParams0.StreamID          = STAUD_IGNORE_ID;
    AUD_StartParams0.SamplingFrequency =sample_rate;

	PCMInputParams0.Frequency = AUD_StartParams0.SamplingFrequency;
	PCMInputParams0.DataPrecision = 16;
	PCMInputParams0.NumChannels = 2;
	
	
	//ErrCode |= AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD0);

	ErrCode |= AUD_InjectStart(AUD_INT, 0, STAUD_OBJECT_INPUT_CD0,TRUE);
    if (ErrCode != ST_NO_ERROR)
    {
         return (ErrCode);
    }           
    ErrCode |= AUD_Start(AUD_INT, AUD_StartParams0.StreamContent, AUD_StartParams0.SamplingFrequency, AUD_StartParams0.StreamType);
    if (ErrCode != ST_NO_ERROR)
    {
       	 STTBX_Print(("\n[MP3 Player]AUD_Start Err \n")); 
         return (ErrCode);
    }    			
    return (ErrCode);
} /* Comp_Start() */

#endif
/*-------------------------------------------------------------------------
 * Function     : Comp_Start
 * Description : Fill audio params. and then start audio decoding,
                       dual audio decoding, if the related environment
                       settings are set then activate pcm mixing, 
                       mono/stereo output, prologic or trusurround
 * Input          : Stream Content, Loop Number, Attenuation
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t Comp_Start(StreamContent_t p_COMP_stream_content, char *StreamType)
{
    STAUD_StreamParams_t AUD_StartParams0;
    STAUD_PCMInputParams_t 	PCMInputParams0;
    ST_ErrorCode_t   ErrCode = ST_NO_ERROR;

	//获得mp3采样频率
	
    strcpy(StreamType, "PCM ES");
    AUD_StartParams0.StreamType        = STAUD_STREAM_TYPE_ES;
    AUD_StartParams0.StreamContent     = STAUD_STREAM_CONTENT_PCM;
    AUD_StartParams0.StreamID          = STAUD_IGNORE_ID;
    AUD_StartParams0.SamplingFrequency = PCM_SampFreq;/*16000;*/

	PCMInputParams0.Frequency = AUD_StartParams0.SamplingFrequency;
	PCMInputParams0.DataPrecision = PCM_DataPrecision;
	PCMInputParams0.NumChannels = PCM_NumChannels;
	
	
	/*TRUE: stream is ?????byte reversed????*/
#if 1 /*yxl 2008-04-14 temp modify*/
	ErrCode |= AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD0);

	ErrCode |= AUD_InjectStart(AUD_INT, 0, STAUD_OBJECT_INPUT_CD0,TRUE);
#else
	ErrCode |= AUD_PcmConfig(AUD_INT, PCMInputParams0, STAUD_OBJECT_INPUT_CD1);
	
	ErrCode |= AUD_InjectStart(AUD_INT, 0, STAUD_OBJECT_INPUT_CD1,TRUE);
	
#endif
    if (ErrCode != ST_NO_ERROR)
    {
         return (ErrCode);
    }           
    ErrCode |= AUD_Start(AUD_INT, AUD_StartParams0.StreamContent, AUD_StartParams0.SamplingFrequency, AUD_StartParams0.StreamType);
    if (ErrCode != ST_NO_ERROR)
    {
       	 STTBX_Print(("\n[MP3 Player]AUD_Start Err \n")); 
         return (ErrCode);
    }    			
    return (ErrCode);
} /* Comp_Start() */


/*-------------------------------------------------------------------------
 * Function     : Comp_Stop
 * Description : Stop audio decoding and remove attenuation
 * Input          : None
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t Comp_Stop(void)
{
    ST_ErrorCode_t ErrCode;

    /*semaphore_wait(Play_Next_File);*/
    task_delay(100);

#if 1 /*yxl 2008-08-25 modify below section*/
    ErrCode = AUD_InjectStop_A(AUD_INT, 0);
#else
    ErrCode = AUD_InjectStop(AUD_INT);

#endif /*end yxl 2008-08-25 modify below section*/
    if (ErrCode != ST_NO_ERROR)
       return (ErrCode);


    return (ErrCode);
}

/*-------------------------------------------------------------------------
 * Function     : CH_MP3_PCMDecodeInit
 * Description : Start pcm audio decoding
 * Input          : None
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t CH_MP3_PCMDecodeInit(void)
{
    ST_ErrorCode_t ErrCode;
    char  StreamType[20];

	PCM_SampFreq = 0;/*重新设置参数 sqzow20100604*/
    ErrCode = Comp_Start(PCM, StreamType);
    if(ErrCode != ST_NO_ERROR)
       return (ErrCode);
	
    STTBX_Print(("\n[MP3 Player]PCM Decode Initlized[%s]!\n",StreamType));
    return (ErrCode);
} /* CH_MP3_PCMDecodeInit() */
