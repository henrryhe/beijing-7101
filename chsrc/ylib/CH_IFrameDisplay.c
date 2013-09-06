/*
Name:CH_IFrameDisplay.c
Description:I帧显示 for changhong 7100 platform

Authors:yixiaoli
Date          Remark
2007-01-09    Created

Modifiction:

Date:2007-02-21 by yixiaoli
1. 将所有内存(NcachePartition,CHGDPPartition)分区统一为CHGDPPartition。 
2.Modify function CH7_FrameDisplay(),在相关地方add 	
	ErrCode = STVID_SetDataInputInterface( VIDHandle[VID_MPEG2],
		VideoGetWritePtrFct, VideoSetReadPtrFct,
		(void * const)VIDEOBufHandleTemp);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print (("\nSTVID_SetDataInputInterface Failed!!! Error = %s \n",GetErrorText(ErrCode)));
					
	}

  解决在I帧显示完后，视频图像出不来的问题。

Date:2007-04-28 by yixiaoli  ,Mark 2007-04-29
1.修改相应函数，解决H.264和MPEG2自动切换情况下的I 帧显示问题
2.修改相应部分，将所有与应用相关的全局变量改为参数传递，
  
*/


/* Includes */
/* -------- */
#include "..\main\initterm.h"
#include "..\main\CH_sys.h"
#include "channelbase.h"


#define VID_MAX_NUMBER 2
#define LAYER_MAX_NUMBER 2
/* Flag to inject using DMAs */
/* ------------------------- */
#if defined(ST_5100)||defined(ST_5105)/*||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)/**/
#define INJECTOR_USE_DMA
#endif

extern ST_Partition_t* CHGDPPartition;/*yxl 2007-02-21 add this line*/

/*#define USE_EVENT*/

U8* VID_LoadFileToMemory(U8 *FileName,U32* pFileSize,Partition_t UsePartition);

/* Macro definitions */
/* ----------------- */

#define VIDi_TraceDBG(x) 

/* Injector State */
/* -------------- */
typedef enum 
{
	VID_INJECT_SLEEP,
	VID_INJECT_START,
	VID_INJECT_STOP
} VID_InjectState_t;

/* Inject node structure */
/* --------------------- */
typedef struct VID_InjectContext_s
{
	U32                  InjectorId;            /* Injector identifier                  */
	STVID_Handle_t       VIDHandle;             /* Video handle                         */
	STEVT_Handle_t       EVT_Handle;            /* Event handle                         */
	STVID_StartParams_t  VIDStartParams;       /* Video start parameters               */
	void                *FileHandle;            /* File handle descriptor               */
	U32                  FileSize;              /* File size to play                    */
	U32                  FilePosition;          /* Current file position                */
	U32                  BufferPtr;             /* Buffer to read the file              */
	U32                  BufferBase;            /* Buffer base to read the file         */
	U32                  BufferProducer;        /* Producer pointer in the file buffer  */
	U32                  BufferConsumer;        /* Consumer pointer in the file buffer  */
	U32                  BufferSize;            /* Buffer size to read the file         */
	U32                  BufferVideo;           /* Video pes buffer                     */
	U32                  BufferVideoProducer;   /* Producer pointer in the video buffer */
	U32                  BufferVideoConsumer;   /* Consumer pointer in the video buffer */
	U32                  BufferVideoSize;       /* Video pes buffer size                */
	U32                  BitBufferVideoSize;    /* Video bit buffer empty size          */
	VID_InjectState_t    State;                 /* State of the injector                */
	U32                  NbLoops;               /* Nb loops to do (0=Infinity)          */
	U32                  NbLoopsAlreadyDone;    /* Nb loops already done                */
	Semaphore_t          SemStart;              /* Start semaphore to injector task     */
	Semaphore_t          SemStop;               /* Stop  semaphore to injector task     */
	Semaphore_t          SemLock;               /* Lock  semaphore for ptr protection   */
	Semaphore_t          SemVideoStopped;       /* Stop  semaphore for video stopped    */
	Task_t              *Task;                  /* Task descriptor of the injector      */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
	U8                  *FdmaNode;              /* Node pointer allocated               */
	STFDMA_Node_t       *FdmaNodeAligned;       /* Node aligned address                 */
	STFDMA_ChannelId_t   FdmaChannelId;         /* Channel id                           */
#endif
	
} VID_InjectContext_t;


#ifdef USE_EVENT 
/* Local Prototypes */
/* ---------------- */
static void           VIDi_InjectCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
#endif


#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
static ST_ErrorCode_t VIDi_GetWritePtr(void *const Handle,void **const Address);
static void           VIDi_SetReadPtr(void *const Handle,void *const Address);
#endif
static void           VIDi_InjectTask(void *VID_InjectContext);
static void           VIDi_InjectCopy(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy);
static U32            VIDi_InjectGetFreeSize(VID_InjectContext_t *VID_InjectContext);
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
static void           VIDi_InjectCopy_FDMA(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy);
#endif

#if !defined(INJECTOR_USE_DMA)
	static void           VIDi_InjectCopy_CPU(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy);
#endif

/* Local variables */
/* --------------- */
VID_InjectContext_t *VIDi_InjectContext[VID_MAX_NUMBER];






/*yxl 2007-07-03 add U32 Loops,U32 Priority*/
BOOL CH7_FrameDisplay(U8 *pMembuf,U32 Size,Partition_t UsePartition,
					  CH_VIDCELL_t CHVIDCell,U32 Loops,U32 Priority)  
{
	ST_ErrorCode_t ErrCode;
	BOOL result=TRUE;
	U32 FSizeTemp=0;
	U8* pFBufTemp=NULL;
	U8* pFBufTemp_Aligned=NULL;
	STVID_StartParams_t VIDStartParams;
	CH_VIDCELL_t VIDCellTemp;
	STVID_Handle_t VIDHandleTemp;
	STVID_ViewPortHandle_t VPHanTemp;
	int IndexTemp;
	int i;
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority;

	Partition_t PartitionTemp=UsePartition;

	memset(&VIDStartParams,0,sizeof(VIDStartParams));
	
	memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));

	memcpy((void*)&VIDCellTemp,(const void*)&CHVIDCell,sizeof(CH_VIDCELL_t));

	for(i=0;i<VIDCellTemp.VIDDecoderCount;i++)
	{
		VIDHandleTemp=VIDCellTemp.VIDCell[i].VideoHandle;
		if(VIDHandleTemp!=0) 
		{
			IndexTemp=i;
			break;
		}
	}

	VIDStartParams.RealTime      = FALSE;
	VIDStartParams.UpdateDisplay = TRUE;
	VIDStartParams.BrdCstProfile = STVID_BROADCAST_DVB;

	VIDStartParams.StreamID      = STVID_IGNORE_ID;
	VIDStartParams.DecodeOnce    = FALSE;

	VIDStartParams.StreamType    = (STVID_StreamType_t)STVID_STREAM_TYPE_ES;


	pFBufTemp = Memory_Allocate(PartitionTemp/*CHGDPPartition*/,Size+256);
	if (pFBufTemp==NULL)
	{
		STTBX_Print(("YxlInfo:Can't allocate memory to load the file from flash!!!\n"));
		
		return TRUE;
	} 
	pFBufTemp_Aligned = (U8 *)((((U32)pFBufTemp)+255)&0xFFFFFF00);
	memcpy((void*)pFBufTemp_Aligned,(const void*)pMembuf,Size);
	FSizeTemp=Size;

#if 0 /*yxl 2007-04-29 modify below section*/
	result=VID_InjectParamInit(FSizeTemp,pFBufTemp,VIDStartParams);
#else
	result=VID_InjectParamInit(FSizeTemp,pFBufTemp,VIDStartParams,VIDHandleTemp,
		LoopsTemp,PriorityTemp);
#endif/*end yxl 2007-04-29 modify below section*/
	if(result==TRUE)
	{
		VID_InjectParamRelease();
		STTBX_Print(("\nYxlInfo:VID_InjectParamInit() isn't successful"));
		goto Exit1;
		return TRUE;
	}

	result=VID_InjectStartA();
	if(result==TRUE)
	{
		VID_InjectParamRelease();
		STTBX_Print(("\nYxlInfo:VID_InjectStartA() isn't successful"));
		goto Exit1;
		return TRUE;
	}

#if 0
	STVID_EnableOutputWindow(VIDVPHandle[0][0]);
	STVID_EnableOutputWindow(VIDVPHandle[0][1]);
#else
	for(i=0;i<VIDCellTemp.VIDCell[IndexTemp].VPortCount;i++)
	{
		VPHanTemp=VIDCellTemp.VIDCell[IndexTemp].VPortHandle[i];
		if(VPHanTemp==0) continue;
		STVID_EnableOutputWindow(VPHanTemp);
	}
#endif

	VID_InjectWaitFinish();

	result=VID_InjectTerm();
	if(result==TRUE)
	{
		STTBX_Print(("\nYxlInfo:VID_InjectTerm() isn't successful"));
		goto Exit1;
		return TRUE;
	}
	
	VID_InjectParamRelease();

	if(pFBufTemp!=NULL)
	{
		Memory_Deallocate(PartitionTemp,pFBufTemp);
	}


	return FALSE;
Exit1:
	if(pFBufTemp!=NULL)
	{
		Memory_Deallocate(CHGDPPartition,pFBufTemp);
	}

	return TRUE;

}




BOOL VID_InjectWaitFinish(void)
{
	Semaphore_Wait(VIDi_InjectContext[0]->SemStop);
	return FALSE;
}


BOOL VID_InjectTerm(void)
{
	STVID_Freeze_t VID_Freeze;
	ST_ErrorCode_t ErrCode=ST_NO_ERROR;
	int DeviceId=0;
	
	if (VIDi_InjectContext[DeviceId]==NULL)
	{
		STTBX_Print(("\nYxlInfo:VID_InjectTerm() isn't successful"));
		return TRUE;
	}


	/* Stop the video decoder */
	/* ----------------------- */
	VID_Freeze.Field = STVID_FREEZE_FIELD_TOP;
	VID_Freeze.Mode  = STVID_FREEZE_MODE_NONE;
	ErrCode=STVID_InjectDiscontinuity(VIDi_InjectContext[DeviceId]->VIDHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_InjectDiscontinuity()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
/*	ErrCode=STVID_InjectDiscontinuity(VIDi_InjectContext[DeviceId]->VIDHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_InjectDiscontinuity()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}*/
	ErrCode=STVID_Stop(VIDi_InjectContext[DeviceId]->VIDHandle,STVID_STOP_NOW,&VID_Freeze);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_Stop()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}


	/* Delete buffer interface */
	/* ----------------------- */
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
	ErrCode=STVID_DeleteDataInputInterface(VIDi_InjectContext[DeviceId]->VIDHandle); 
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to delete the video interface !!!\n",DeviceId));
		return TRUE;
	}
#endif
	
	/* Destroy FDMA transfer */
	/* --------------------- */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
	ErrCode=STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STFDMA_UnlockChannel(%d):**ERROR** !!! Unable to unlock the fdma channel !!!\n",DeviceId));
		return TRUE;
	}
#endif

#ifdef USE_EVENT	
	/* Unsubscrive to events */
	/* --------------------- */
	ErrCode=STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
	ErrCode=STEVT_Close(VIDi_InjectContext[DeviceId]->EVT_Handle);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to unsubscribe to video events !!!\n",DeviceId));
		return TRUE;
	} 
#endif
	
	/* Delete the inject task */
	/* ---------------------- */
	Task_Wait(VIDi_InjectContext[DeviceId]->Task);

	if(Task_Delete(VIDi_InjectContext[DeviceId]->Task)==OS_SUCCESS)
	{
		VIDi_InjectContext[DeviceId]->Task=NULL;

	}
	else
	{
		STTBX_Print(("\nYxlInfo:Delete Task isn't succesful"));
		return TRUE;
	}

	
	return FALSE;
}



BOOL VID_InjectParamRelease(void)
{
	int DeviceId=0;
	if(VIDi_InjectContext[DeviceId]==NULL) return FALSE;

	if (VIDi_InjectContext[DeviceId]->Task!=NULL)
	{
		if(Task_Delete(VIDi_InjectContext[DeviceId]->Task)==OS_SUCCESS)
		{
			VIDi_InjectContext[DeviceId]->Task=NULL;
			
		}
		else
		{
			STTBX_Print(("\nYxlInfo:Delete Task isn't succesful"));
			return TRUE;
		}
	}
 
	Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemVideoStopped);
	Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemStart);
	Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemStop);
	Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemLock);

 #if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
	if(VIDi_InjectContext[DeviceId]->FdmaNode!=NULL)
	{
		Memory_Deallocate(CHGDPPartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		VIDi_InjectContext[DeviceId]->FdmaNode=NULL;
	}
#endif
	if(VIDi_InjectContext[DeviceId]!=NULL)
	{
		Memory_Deallocate(CHGDPPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
	}
	return FALSE;
}



/*yxl 2007-07-03 add U32 Loops,U32 Priority*/
BOOL VID_InjectParamInit(U32 FileSize,U8* pFileBuffer,
						 STVID_StartParams_t VIDStartParams,
						 STVID_Handle_t VHandle,U32 Loops,U32 Priority)
{

	U8* pFILE_Buffer;
	U8* pFILE_Buffer_Aligned;
	int DeviceId=0;
#ifdef USE_EVENT 
	STEVT_OpenParams_t            EVT_OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
#endif
	ST_ErrorCode_t                ErrCode=ST_NO_ERROR;

	STVID_Handle_t                VHandleTemp=VHandle;
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority;


	if(pFileBuffer==NULL||FileSize==0) 
	{
		STTBX_Print(("\nYxlInfo:Params is wrong"));
		return TRUE;
	}
	pFILE_Buffer=pFileBuffer;
	pFILE_Buffer_Aligned=(U8 *)((((U32)pFILE_Buffer)+255)&0xFFFFFF00);

	if(VIDi_InjectContext[DeviceId]!=NULL) 
	{
		STTBX_Print(("\nYxlInfo:Params has been Initerinted"));
		return TRUE;
	}
	/* Allocate a context */
	/* ------------------ */
	VIDi_InjectContext[DeviceId]=Memory_Allocate(CHGDPPartition,sizeof(VID_InjectContext_t));
	if (VIDi_InjectContext[DeviceId]==NULL)
	{ 
		STTBX_Print(("(%d):**ERROR** !!! Unable to allocate memory !!!\n",DeviceId));
		return TRUE;
	}
	memset(VIDi_InjectContext[DeviceId],0,sizeof(VID_InjectContext_t));
	
	/* Fill up the context */
	/* ------------------- */
	VIDi_InjectContext[DeviceId]->VIDHandle          = VHandleTemp;/*VIDHandle[DeviceId];*/
	VIDi_InjectContext[DeviceId]->InjectorId          = DeviceId;

	memcpy((void*)&VIDi_InjectContext[DeviceId]->VIDStartParams,
		(const void*)&VIDStartParams,sizeof(STVID_StartParams_t));


	VIDi_InjectContext[DeviceId]->BufferPtr           = (U32)pFILE_Buffer;
	VIDi_InjectContext[DeviceId]->BufferBase          = \
	VIDi_InjectContext[DeviceId]->BufferConsumer      = \
	VIDi_InjectContext[DeviceId]->BufferProducer      = (U32)pFILE_Buffer_Aligned;

	VIDi_InjectContext[DeviceId]->BufferSize          = \
	VIDi_InjectContext[DeviceId]->FileSize            = FileSize;

	VIDi_InjectContext[DeviceId]->FileHandle          = 0;
	VIDi_InjectContext[DeviceId]->FilePosition        = 0;

	VIDi_InjectContext[DeviceId]->NbLoops             = LoopsTemp;

	VIDi_InjectContext[DeviceId]->NbLoopsAlreadyDone  = 0;
	VIDi_InjectContext[DeviceId]->State               = VID_INJECT_START;
	
	/* Create the semaphores */
	/* --------------------- */
	Semaphore_Init_Fifo(VIDi_InjectContext[DeviceId]->SemStart               ,0);
	Semaphore_Init_Fifo(VIDi_InjectContext[DeviceId]->SemStop                ,0);
	Semaphore_Init_Fifo(VIDi_InjectContext[DeviceId]->SemLock                ,1);
	Semaphore_Init_Fifo_TimeOut(VIDi_InjectContext[DeviceId]->SemVideoStopped,0);
	
	/* Get buffer parameters for the video */
	/* ----------------------------------- */
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
	ErrCode=STVID_GetBitBufferFreeSize(VHandleTemp,&VIDi_InjectContext[DeviceId]->BitBufferVideoSize); 
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_GetBitBufferFreeSize()=%s",GetErrorText(ErrCode)));
		goto Exit1;
	}

	ErrCode=STVID_GetDataInputBufferParams(VHandleTemp,(void **)&(VIDi_InjectContext[DeviceId]->BufferVideo),&(VIDi_InjectContext[DeviceId]->BufferVideoSize));
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_GetDataInputBufferParams()=%s",GetErrorText(ErrCode)));
		goto Exit1;
	}
#if defined(ST_7100)||defined(ST_7109)
	/*ErrCode|=VID_OpenViewPort(DeviceId,0,&VIDVPHandle[DeviceId][0]);
	ErrCode|=VID_OpenViewPort(DeviceId,1,&VIDVPHandle[DeviceId][1]);
	*/
	VIDi_InjectContext[DeviceId]->BufferVideo = (U32)VIDi_InjectContext[DeviceId]->BufferVideo&0x1FFFFFFF; 
#endif
	VIDi_InjectContext[DeviceId]->BufferVideoProducer = VIDi_InjectContext[DeviceId]->BufferVideoConsumer = VIDi_InjectContext[DeviceId]->BufferVideo;

	ErrCode=STVID_SetDataInputInterface(VHandleTemp,VIDi_GetWritePtr,VIDi_SetReadPtr,VIDi_InjectContext[DeviceId]);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_SetDataInputInterface()=%s",GetErrorText(ErrCode)));
		goto Exit1;
	}


#endif

	/* Setup for FDMA transfer */
	/* ----------------------- */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
	VIDi_InjectContext[DeviceId]->FdmaNode=Memory_Allocate(CHGDPPartition,sizeof(STFDMA_Node_t)+256);
	if (VIDi_InjectContext[DeviceId]->FdmaNode==NULL)
	{
		STTBX_Print(("\nYxlInfo:Unable allocate memory"));
		goto Exit1;
	}
	VIDi_InjectContext[DeviceId]->FdmaNodeAligned=(STFDMA_Node_t *)(((U32)VIDi_InjectContext[DeviceId]->FdmaNode+255)&0xFFFFFF00); 
	ErrCode=STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL/*STFDMA_HIGH_BANDWIDTH_POOL*/,&VIDi_InjectContext[DeviceId]->FdmaChannelId);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STFDMA_LockChannelInPool()=%s",GetErrorText(ErrCode)));

		goto Exit1;

	}
#endif

#ifdef USE_EVENT 	
	/* Subscribe to events */
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
	memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
	EVT_SubcribeParams.NotifyCallback=VIDi_InjectCallback;
	ErrCode=STEVT_Open(EVTDeviceName,&EVT_OpenParams,&(VIDi_InjectContext[DeviceId]->EVT_Handle));
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STEVT_Open()=%s",GetErrorText(ErrCode)));
	}
	ErrCode=STEVT_SubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT,&EVT_SubcribeParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STEVT_SubscribeDeviceEvent()=%s",GetErrorText(ErrCode)));
	
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
	goto Exit1;
	}
#endif 

#if 0 /*yxl 2007-07-03 modify below section*/
	/* Create the inject task */
	VIDi_InjectContext[DeviceId]->Task=Task_Create(VIDi_InjectTask,
		VIDi_InjectContext[DeviceId],2048,MAX_USER_PRIORITY/2,"VID_InjectorTask",
		/*0*/task_flags_suspended);
#else
	VIDi_InjectContext[DeviceId]->Task=Task_Create(VIDi_InjectTask,
		VIDi_InjectContext[DeviceId],2048,PriorityTemp,"VID_InjectorTask",
		/*0*/task_flags_suspended);
#endif /*end yxl 2007-07-03 modify below section*/

	if (VIDi_InjectContext[DeviceId]->Task==NULL)
	{
			
#ifdef USE_EVENT 
		STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
		STVID_DeleteDataInputInterface(VHandleTemp); 
#endif
		goto Exit1;
	}	

	return FALSE;
	
Exit1:
	VID_InjectParamRelease();
	return TRUE;
}





BOOL VID_InjectStartA()
{
	ST_ErrorCode_t ErrCode=ST_NO_ERROR;
	STVID_Handle_t VIDHandleTemp;
	STVID_StartParams_t* pStartParamsTemp;
	int DeviceId=0;

	if(VIDi_InjectContext[DeviceId]==NULL) 
	{	
		STTBX_Print(("\nYxlInfo:Inject Params haven't init,"));
		return TRUE;
	}

	if(VIDi_InjectContext[DeviceId]->State!=VID_INJECT_START) 
	{	
		STTBX_Print(("\nYxlInfo:Inject Params haven't initAAA,"));
		return TRUE;
	}


	VIDHandleTemp=VIDi_InjectContext[DeviceId]->VIDHandle;
	pStartParamsTemp=&VIDi_InjectContext[DeviceId]->VIDStartParams;

	/* Start the video decoder */
	/* ----------------------- */
	ErrCode=STVID_DisableSynchronisation(VIDHandleTemp); 
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_DisableSynchronisation()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	ErrCode=STVID_SetSpeed(VIDHandleTemp,100); 
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_SetSpeed()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	ErrCode=STVID_Start(VIDHandleTemp,pStartParamsTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nYxlInfo:STVID_Start()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	Task_resume(VIDi_InjectContext[DeviceId]->Task);/*yxl 2007-06-10 temp cancel*/


	Semaphore_Signal(VIDi_InjectContext[DeviceId]->SemStart);

	return FALSE;
}




/* ========================================================================
   Name:        VID_InjectStop
   Description: Stop an video injection
   ======================================================================== */

ST_ErrorCode_t VID_InjectStop(U32 DeviceId)
{
 STVID_Freeze_t VID_Freeze;
 ST_ErrorCode_t ErrCode=ST_NO_ERROR;

 /* If the injecter is not running, go out */
 /* -------------------------------------- */
 if (VIDi_InjectContext[DeviceId]==NULL)
  {
   STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Injector is not running !!!\n",DeviceId));
   return(ST_ERROR_BAD_PARAMETER);
  }

 /* Stop the task */
 /* ------------- */
 VIDi_InjectContext[DeviceId]->State = VID_INJECT_STOP;
 Semaphore_Wait(VIDi_InjectContext[DeviceId]->SemStop);

 /* Stop the video decoder */
 /* ----------------------- */
 VID_Freeze.Field = STVID_FREEZE_FIELD_TOP;
 VID_Freeze.Mode  = STVID_FREEZE_MODE_NONE;
 ErrCode|=STVID_InjectDiscontinuity(VIDi_InjectContext[DeviceId]->VIDHandle);
 ErrCode|=STVID_InjectDiscontinuity(VIDi_InjectContext[DeviceId]->VIDHandle);
 ErrCode|=STVID_Stop(VIDi_InjectContext[DeviceId]->VIDHandle,STVID_STOP_NOW,&VID_Freeze);
 if ((ErrCode!=ST_NO_ERROR) && (ErrCode!=STVID_ERROR_DECODER_STOPPED))
  {
   STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to stop the video decoder !!!\n",DeviceId));
   return(ErrCode);
  }
 /* Wait end of decoder when calling STVID_Stop() with STVID_STOP_WHEN_END_OF_DATA*/

  Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemVideoStopped);

 /* Delete buffer interface */
 /* ----------------------- */
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
 ErrCode=STVID_DeleteDataInputInterface(VIDi_InjectContext[DeviceId]->VIDHandle); 
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to delete the video interface !!!\n",DeviceId));
   return(ErrCode);
  }
#if defined(ST_7100)||defined(ST_7109)
 /*
 ErrCode|=VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
 ErrCode|=VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
 */
#endif
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to close the viewport !!!\n",DeviceId));
   return(ErrCode);
  }
#endif

 /* Destroy FDMA transfer */
 /* --------------------- */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
 Memory_Deallocate(CHGDPPartition,VIDi_InjectContext[DeviceId]->FdmaNode);
 ErrCode=STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("STFDMA_UnlockChannel(%d):**ERROR** !!! Unable to unlock the fdma channel !!!\n",DeviceId));
   return(ErrCode);
  }
#endif

#ifdef USE_EVENT  /*yxl 2009-07-22 add this line*/
 /* Unsubscrive to events */
 /* --------------------- */
 ErrCode|=STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
 ErrCode|=STEVT_Close(VIDi_InjectContext[DeviceId]->EVT_Handle);
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to unsubscribe to video events !!!\n",DeviceId));
   return(ErrCode);
  } 
#endif /*yxl 2009-07-22 add this line*/

 /* Delete the inject task */
 /* ---------------------- */
 Task_Wait(VIDi_InjectContext[DeviceId]->Task);
 Task_Delete(VIDi_InjectContext[DeviceId]->Task);

 /* Delete the semaphores */
 /* --------------------- */                               
 Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemStart);
 Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemStop);
 Semaphore_Delete(VIDi_InjectContext[DeviceId]->SemLock);
 
 /* Do we have to close the file ? */
 /* ------------------------------ */
 if (VIDi_InjectContext[DeviceId]->FileHandle!=NULL) 
  {
   SYS_FClose(VIDi_InjectContext[DeviceId]->FileHandle);
  }

 /* Free the injector context */
 /* ------------------------- */
 Memory_Deallocate(CHGDPPartition,(U8 *)VIDi_InjectContext[DeviceId]->BufferPtr);
 Memory_Deallocate(CHGDPPartition,VIDi_InjectContext[DeviceId]);
 VIDi_InjectContext[DeviceId]=NULL;

 /* Return no errors */
 /* ---------------- */
 return(ST_NO_ERROR);
}


/* ========================================================================
   Name:        VIDi_InjectTask
   Description: Inject task 
   ======================================================================== */

static void VIDi_InjectTask(void *InjectContext)
{
	VID_InjectContext_t *VID_InjectContext;
	U32                  SizeToInject;
	U32                  SizeAvailable;
	
	/* Retrieve context */
	/* ================ */
	VID_InjectContext=(VID_InjectContext_t *)InjectContext;
	
	/* Wait start of play */
	/* ================== */
	Semaphore_Wait(VID_InjectContext->SemStart);
	SizeAvailable = SizeToInject = 0;
	
	/* Infinite loop */
	/* ============= */
	while(VID_InjectContext->State==VID_INJECT_START)
	{
		/* Do we have to read datas from a file */
		/* ------------------------------------ */
		if (VID_InjectContext->FileHandle!=NULL)
		{
			/* If we reach the end of the file, check number of loops */
			if (VID_InjectContext->FilePosition==VID_InjectContext->FileSize)
			{
				/* If infinite play, restart from the beginning */
				if (VID_InjectContext->NbLoops==0)
				{
					VID_InjectContext->NbLoopsAlreadyDone++;
					/* Seek to the beginning of the file */
					SYS_FSeek(VID_InjectContext->FileHandle,0,SEEK_SET);
					/* Update file position */
					VID_InjectContext->FilePosition=0;
				}
				/* Else check number of loops */
				else
				{
					/* If nb loops reached, switch in stop state */
					if (VID_InjectContext->NbLoops==(VID_InjectContext->NbLoopsAlreadyDone+1))
					{
						/* Wait to inject remaining datas */
						if (VID_InjectContext->BufferConsumer==VID_InjectContext->BufferProducer)
						{
							VID_InjectContext->State = VID_INJECT_STOP;
							break;
						}
					}
					/* Else increment number of loops and seek to the beginning of the file */
					else
					{
						VID_InjectContext->NbLoopsAlreadyDone++;
						/* Seek to the beginning of the file */
						SYS_FSeek(VID_InjectContext->FileHandle,0,SEEK_SET);
						/* Update file position */
						VID_InjectContext->FilePosition=0;
					}
				}
			}
			/* If we have to read something from the file */
			if (VID_InjectContext->FilePosition!=VID_InjectContext->FileSize)
			{
				U32 SizeToRead,SizeToStuff;       
				/* Compute number of bytes which we have to read from the file */
				SizeToRead  = (VID_InjectContext->BufferSize-SizeAvailable)-256; /* Always keep 256 bytes for stuff */
				SizeToStuff = 0;
				/* Get free size into the circular buffer */
				if ((VID_InjectContext->BufferProducer+SizeToRead)>(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize))
				{
					SizeToRead=(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize)-VID_InjectContext->BufferProducer;
				}
				/* If we reach the end of the file, need probably to inject stuff */
				if ((VID_InjectContext->FileSize-VID_InjectContext->FilePosition)<=SizeToRead)
				{
					SizeToRead  = (VID_InjectContext->FileSize-VID_InjectContext->FilePosition);
					SizeToStuff = (256-(SizeToRead&0xFF))&0xFF;
				}
				/* Read the file */
				if (SizeToRead!=0)
				{
					SYS_FRead((U8 *)VID_InjectContext->BufferProducer,SizeToRead,1,VID_InjectContext->FileHandle);
				}
				/* Increase size available to be injected */
				SizeAvailable = SizeAvailable+SizeToRead;
				/* Update file position & producer */
				VID_InjectContext->FilePosition   = VID_InjectContext->FilePosition+SizeToRead;
				VID_InjectContext->BufferProducer = VID_InjectContext->BufferProducer+SizeToRead;
				if (VID_InjectContext->BufferProducer==(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize)) 
				{
					VID_InjectContext->BufferProducer=VID_InjectContext->BufferBase; 
				}
				/* Inject stuff */
				if (SizeToStuff!=0) 
				{
					U32 SizeToStuff1,SizeToStuff2;
					SizeToStuff1 = SizeToStuff;
					SizeToStuff2 = 0;
					/* Check if we have to split the stuff in two parts */
					if ((VID_InjectContext->BufferProducer+SizeToStuff)>(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize))
					{
						SizeToStuff1=(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize)-VID_InjectContext->BufferProducer;
						SizeToStuff2=SizeToStuff-SizeToStuff1;
					}
					/* Do the stuff */
					if (SizeToStuff1!=0) memset((U8 *)VID_InjectContext->BufferProducer,0,SizeToStuff1);
					if (SizeToStuff2!=0) memset((U8 *)VID_InjectContext->BufferBase    ,0,SizeToStuff2);         
					/* Wrap the producer */
					if (SizeToStuff2==0) VID_InjectContext->BufferProducer=VID_InjectContext->BufferProducer+SizeToStuff;
					else VID_InjectContext->BufferProducer=VID_InjectContext->BufferBase+SizeToStuff2; 
					if (VID_InjectContext->BufferProducer==(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize)) 
					{
						VID_InjectContext->BufferProducer=VID_InjectContext->BufferBase; 
					}
					/* Increase size available */
					SizeAvailable = SizeAvailable+SizeToStuff;
				}
			}
		}
		
		/* If there is no file and all the stream is in memory */
		/* --------------------------------------------------- */
		if (VID_InjectContext->FileHandle==NULL)
		{
			SizeAvailable = VID_InjectContext->BufferSize;
		}
		
		/* Do we have to datas to inject */
		/* ----------------------------- */
		if ((SizeToInject=VIDi_InjectGetFreeSize(VID_InjectContext))!=0)
		{
			/* Check if we have to split the copy into pieces into the source buffer */
			if ((VID_InjectContext->BufferConsumer+SizeToInject)>=(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize))
			{
				SizeToInject=(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize)-(VID_InjectContext->BufferConsumer);
			}
			if (SizeToInject>=SizeAvailable) SizeToInject=SizeAvailable;
			
			/* Inject the piece of datas */
			if (SizeToInject!=0) VIDi_InjectCopy(VID_InjectContext,SizeToInject);
			SizeAvailable = SizeAvailable-SizeToInject;
			
			/* Wrapping of the buffer */
			if ((VID_InjectContext->BufferConsumer)==(VID_InjectContext->BufferBase+VID_InjectContext->BufferSize)) 
			{
				VID_InjectContext->BufferConsumer=VID_InjectContext->BufferBase;
				/* If all the stream is in memory */
				if (VID_InjectContext->FileHandle==0)
				{
					VID_InjectContext->NbLoopsAlreadyDone++;
					/* If nb loop is not infinite */
					if (VID_InjectContext->NbLoops!=0)
					{
						/* If nb loops reached, switch in stop state */
						if (VID_InjectContext->NbLoops==VID_InjectContext->NbLoopsAlreadyDone)
						{
							VID_InjectContext->State = VID_INJECT_STOP;
							break;
						}
					}
				}
			}
		}
		
		/* Sleep a while */
		/* ------------- */
	 /*	Task_Delay(ST_GetClocksPerSecond()/10); Sleep 100ms */
  }
  
  /* Finish the task */
  /* =============== */
  Semaphore_Signal(VID_InjectContext->SemStop);
  STTBX_Print(("\n\nYxlInfo:Out of task -------------------\n"));/**/

}

/* ========================================================================
   Name:        VIDi_InjectGetFreeSize
   Description: Get free size able to be injected
   ======================================================================== */

static U32 VIDi_InjectGetFreeSize(VID_InjectContext_t *VID_InjectContext)
{
 U32            FreeSize;
 U32            BitBufferFreeSize;
 ST_ErrorCode_t ErrCode=ST_NO_ERROR;

 /* Get bit buffer size */
 /* ------------------- */
 ErrCode=STVID_GetBitBufferFreeSize(VID_InjectContext->VIDHandle,&BitBufferFreeSize);
 BitBufferFreeSize=(BitBufferFreeSize*3)/4;
 if (BitBufferFreeSize<(VID_InjectContext->BitBufferVideoSize/2)) return(0);
 FreeSize=BitBufferFreeSize;

 /* Get free size into the video decoder buffer */
 /* ------------------------------------------- */
#if defined(ST_5528)
#else
 Semaphore_Wait(VID_InjectContext->SemLock); 
 if (VID_InjectContext->BufferVideoProducer>=VID_InjectContext->BufferVideoConsumer)
  {
   FreeSize=(VID_InjectContext->BufferVideoSize-(VID_InjectContext->BufferVideoProducer-VID_InjectContext->BufferVideoConsumer));
  }
 else
  {
   FreeSize=(VID_InjectContext->BufferVideoConsumer-VID_InjectContext->BufferVideoProducer);
  }
 Semaphore_Signal(VID_InjectContext->SemLock); 
#endif

 /* Align it on 256 bytes to use external DMA */
 /* ----------------------------------------- */
 /* Here we assume the producer will be never equal to base+size           */
 /* If it's the case, this is managed into the fill producer procedure     */
 /* The video decoder is working like this, if ptr=base+size, the ptr=base */ 
 if (FreeSize<=16) FreeSize=0; else FreeSize=FreeSize-16;
 FreeSize=(FreeSize/256)*256;
 return(FreeSize);
}

/* ========================================================================
   Name:        VIDi_InjectCopy
   Description: Copy datas in the video decoder
   ======================================================================== */

static void VIDi_InjectCopy(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy)
{
#if !defined(INJECTOR_USE_DMA)
 VIDi_InjectCopy_CPU(VID_InjectContext,SizeToCopy);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
 VIDi_InjectCopy_FDMA(VID_InjectContext,SizeToCopy);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5528))
 VIDi_InjectCopy_GPDMA(VID_InjectContext,SizeToCopy);
#endif
}

/* ========================================================================
   Name:        VIDi_InjectCopy_CPU
   Description: Copy datas in the video decoder
   ======================================================================== */

#if !defined(INJECTOR_USE_DMA)

static void VIDi_InjectCopy_CPU(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy)
{
 /* For no CD video fifos */
 /* ===================== */
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)
 {
 U32 SizeToCopy1,SizeToCopy2;
 /* !!! First SizeToCopy has to be less than the pes video buffer !!! */

 /* Get size to inject */
 /* ------------------ */
 SizeToCopy1 = SizeToCopy;
 SizeToCopy2 = 0;

 /* Check if we have to split the copy into pieces into the destination buffer */
 /* -------------------------------------------------------------------------- */
 if ((VID_InjectContext->BufferVideoProducer+SizeToCopy)>(VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize))
  {
   SizeToCopy1 = (VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize)-VID_InjectContext->BufferVideoProducer;
   SizeToCopy2 = SizeToCopy-SizeToCopy1;
  }

 /* First piece */
 /* ----------- */
#if defined(ST_7100)||defined(ST_7109)
 memcpy((U8 *)(VID_InjectContext->BufferVideoProducer|0xA0000000),(U8 *)VID_InjectContext->BufferConsumer,SizeToCopy1);
#else
 memcpy((U8 *)(VID_InjectContext->BufferVideoProducer),(U8 *)VID_InjectContext->BufferConsumer,SizeToCopy1);
 SYS_DCacheFlush();
#endif
 Semaphore_Wait(VID_InjectContext->SemLock);
 VID_InjectContext->BufferVideoProducer += SizeToCopy1;
 VID_InjectContext->BufferConsumer      += SizeToCopy1;
 if ((VID_InjectContext->BufferVideoProducer)==(VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize)) VID_InjectContext->BufferVideoProducer = VID_InjectContext->BufferVideo;
 Semaphore_Signal(VID_InjectContext->SemLock);

 /* Second piece */
 /* ------------ */
 if (SizeToCopy2!=0)
  {
   VID_InjectContext->BufferVideoProducer = VID_InjectContext->BufferVideo;
#if defined(ST_7100)||defined(ST_7109)
   memcpy((U8 *)(VID_InjectContext->BufferVideoProducer|0xA0000000),(U8 *)VID_InjectContext->BufferConsumer,SizeToCopy2);
#else
   memcpy((U8 *)(VID_InjectContext->BufferVideoProducer),(U8 *)VID_InjectContext->BufferConsumer,SizeToCopy2);
   SYS_DCacheFlush();
#endif
   Semaphore_Wait(VID_InjectContext->SemLock);
   VID_InjectContext->BufferVideoProducer += SizeToCopy2;
   VID_InjectContext->BufferConsumer      += SizeToCopy2;
   if ((VID_InjectContext->BufferVideoProducer)==(VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize)) VID_InjectContext->BufferVideoProducer = VID_InjectContext->BufferVideo;
   Semaphore_Signal(VID_InjectContext->SemLock);
  }
 
 /* Trace for debug */
 /* --------------- */ 
 VIDi_TraceDBG(("VIDi_InjectCopy(%d): Size1 = 0x%08x - Size2 = 0x%08x - SrcPtr = 0x%08x - DstPtr = 0x%08x\n",VID_InjectContext->InjectorId,SizeToCopy1,SizeToCopy2,VID_InjectContext->BufferConsumer,VID_InjectContext->BufferVideoProducer));
 }
#endif
}
#endif

/* ========================================================================
   Name:        VIDi_InjectCopy_FDMA
   Description: Copy datas in the video decoder
   ======================================================================== */

#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
static void VIDi_InjectCopy_FDMA(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy)
{
 /* !!! First SizeToCopy has to be less than the pes video buffer !!! */
 ST_ErrorCode_t          ErrCode=ST_NO_ERROR;
 STFDMA_TransferParams_t FDMA_TransferParams;
 STFDMA_TransferId_t     FDMA_TransferId;
 U32                     SizeToCopy1,SizeToCopy2;

 /* Get size to inject */
 /* ------------------ */
 SizeToCopy1 = SizeToCopy;
 SizeToCopy2 = 0;

 /* Check if we have to split the copy into pieces into the destination buffer */
 /* -------------------------------------------------------------------------- */
 if ((VID_InjectContext->BufferVideoProducer+SizeToCopy)>(VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize))
  {
   SizeToCopy1 = (VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize)-VID_InjectContext->BufferVideoProducer;
   SizeToCopy2 = SizeToCopy-SizeToCopy1;
  }

 /* First piece */
 /* ----------- */

 /* Fill the node */
#if defined(ST_7100)||defined(ST_7109)
 VID_InjectContext->FdmaNodeAligned->DestinationAddress_p             = (void *)(VID_InjectContext->BufferVideoProducer&0x1FFFFFFF);
#else
 VID_InjectContext->FdmaNodeAligned->DestinationAddress_p             = (void *)(VID_InjectContext->BufferVideoProducer);
#endif
 VID_InjectContext->FdmaNodeAligned->DestinationStride                = 0;
 VID_InjectContext->FdmaNodeAligned->Length                           = SizeToCopy1;
 VID_InjectContext->FdmaNodeAligned->Next_p                           = NULL;
 VID_InjectContext->FdmaNodeAligned->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
 VID_InjectContext->FdmaNodeAligned->NodeControl.NodeCompleteNotify   = TRUE;
 VID_InjectContext->FdmaNodeAligned->NodeControl.NodeCompletePause    = FALSE;
 VID_InjectContext->FdmaNodeAligned->NodeControl.PaceSignal           = 0;
 VID_InjectContext->FdmaNodeAligned->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
 VID_InjectContext->FdmaNodeAligned->NumberBytes                      = SizeToCopy1;
#if defined(ST_7100)||defined(ST_7109)
 VID_InjectContext->FdmaNodeAligned->SourceAddress_p                  = (void *)(VID_InjectContext->BufferConsumer&0x1FFFFFFF); 
#else
 VID_InjectContext->FdmaNodeAligned->SourceAddress_p                  = (void *)(VID_InjectContext->BufferConsumer); 
#endif
 VID_InjectContext->FdmaNodeAligned->SourceStride                     = 0; 
 FDMA_TransferParams.ApplicationData_p                                = NULL;
 FDMA_TransferParams.BlockingTimeout                                  = 0;
 FDMA_TransferParams.CallbackFunction                                 = NULL;
 FDMA_TransferParams.ChannelId                                        = VID_InjectContext->FdmaChannelId;
#if defined(ST_7100)||defined(ST_7109)                                                  
 FDMA_TransferParams.NodeAddress_p                                    = (void *)((U32)VID_InjectContext->FdmaNodeAligned&0x1FFFFFFF);
#else                                                                 
 FDMA_TransferParams.NodeAddress_p                                    = VID_InjectContext->FdmaNodeAligned;
#endif 

 /* Do the transfer now */
 VIDi_TraceDBG(("VIDi_InjectCopy(%d): Size = 0x%08x - SrcPtr = 0x%08x\n",VID_InjectContext->InjectorId,SizeToCopy1,VID_InjectContext->BufferConsumer));
 ErrCode=STFDMA_StartTransfer(&FDMA_TransferParams,&FDMA_TransferId);
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("VIDi_InjectCopy(%d):**ERROR** !!! FDMA transfer has failed !!!\n",VID_InjectContext->InjectorId));
  }

 /* Move the consumer */
 Semaphore_Wait(VID_InjectContext->SemLock);
 VID_InjectContext->BufferVideoProducer += SizeToCopy1;
 VID_InjectContext->BufferConsumer      += SizeToCopy1;
 if ((VID_InjectContext->BufferVideoProducer)==(VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize)) VID_InjectContext->BufferVideoProducer = VID_InjectContext->BufferVideo;
 Semaphore_Signal(VID_InjectContext->SemLock);

 /* Second piece */
 /* ------------ */
 if (SizeToCopy2!=0)
  {
   /* Fill the node */
#if defined(ST_7100)||defined(ST_7109)
   VID_InjectContext->FdmaNodeAligned->DestinationAddress_p             = (void *)(VID_InjectContext->BufferVideoProducer&0x1FFFFFFF);
#else
   VID_InjectContext->FdmaNodeAligned->DestinationAddress_p             = (void *)(VID_InjectContext->BufferVideoProducer);
#endif
   VID_InjectContext->FdmaNodeAligned->DestinationStride                = 0;
   VID_InjectContext->FdmaNodeAligned->Length                           = SizeToCopy2;
   VID_InjectContext->FdmaNodeAligned->Next_p                           = NULL;
   VID_InjectContext->FdmaNodeAligned->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
   VID_InjectContext->FdmaNodeAligned->NodeControl.NodeCompleteNotify   = TRUE;
   VID_InjectContext->FdmaNodeAligned->NodeControl.NodeCompletePause    = FALSE;
   VID_InjectContext->FdmaNodeAligned->NodeControl.PaceSignal           = 0;
   VID_InjectContext->FdmaNodeAligned->NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
   VID_InjectContext->FdmaNodeAligned->NumberBytes                      = SizeToCopy2;
#if defined(ST_7100)||defined(ST_7109)
   VID_InjectContext->FdmaNodeAligned->SourceAddress_p                  = (void *)(VID_InjectContext->BufferConsumer&0x1FFFFFFF); 
#else
   VID_InjectContext->FdmaNodeAligned->SourceAddress_p                  = (void *)(VID_InjectContext->BufferConsumer); 
#endif
   VID_InjectContext->FdmaNodeAligned->SourceStride                     = 0; 
   FDMA_TransferParams.ApplicationData_p                                = NULL;
   FDMA_TransferParams.BlockingTimeout                                  = 0;
   FDMA_TransferParams.CallbackFunction                                 = NULL;
   FDMA_TransferParams.ChannelId                                        = VID_InjectContext->FdmaChannelId;
#if defined(ST_7100)||defined(ST_7109)                                                  
   FDMA_TransferParams.NodeAddress_p                                    = (void *)((U32)VID_InjectContext->FdmaNodeAligned&0x1FFFFFFF);
#else                                                                 
   FDMA_TransferParams.NodeAddress_p                                    = VID_InjectContext->FdmaNodeAligned;
#endif 

   /* Do the transfer now */
   VIDi_TraceDBG(("VIDi_InjectCopy(%d): Size = 0x%08x - SrcPtr = 0x%08x\n",VID_InjectContext->InjectorId,SizeToCopy2,VID_InjectContext->BufferConsumer));
   ErrCode=STFDMA_StartTransfer(&FDMA_TransferParams,&FDMA_TransferId);
   if (ErrCode!=ST_NO_ERROR)
    {
     STTBX_Print(("VIDi_InjectCopy(%d):**ERROR** !!! FDMA transfer has failed !!!\n",VID_InjectContext->InjectorId));
    }

   /* Move the consumer */
   Semaphore_Wait(VID_InjectContext->SemLock);
   VID_InjectContext->BufferVideoProducer += SizeToCopy2;
   VID_InjectContext->BufferConsumer      += SizeToCopy2;
   if ((VID_InjectContext->BufferVideoProducer)==(VID_InjectContext->BufferVideo+VID_InjectContext->BufferVideoSize)) VID_InjectContext->BufferVideoProducer = VID_InjectContext->BufferVideo;
   Semaphore_Signal(VID_InjectContext->SemLock);
  }
}
#endif

/* ========================================================================
   Name:        VIDi_InjectCopy_GPDMA
   Description: Copy datas in the video decoder
   ======================================================================== */

#if defined(INJECTOR_USE_DMA)&&(defined(ST_5528))
static void VIDi_InjectCopy_GPDMA(VID_InjectContext_t *VID_InjectContext,U32 SizeToCopy)
{
}
#endif

#ifdef USE_EVENT 
/* =======================================================================
   Name:        VIDi_InjectCallback
   Description: Video event callback
   ======================================================================== */

static void VIDi_InjectCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
 U32 i;

 /* Identify video decoder */
 /* ====================== */
 for (i=0;i<VID_MAX_NUMBER;i++)
  {
   if (strcmp(RegistrantName,VOUTDeviceName[i])==0) break;
  }
 if (i==VID_MAX_NUMBER)
  {
   STTBX_Print(("VIDi_InjectCallback(?):**ERROR** !!! Unable to retrieve the video decoder !!!\n"));
   return;
  }
 if (VIDi_InjectContext[i]==NULL)
  {
   STTBX_Print(("VIDi_InjectCallback(?):**ERROR** !!! Injector descriptor is a NULL pointer !!!\n"));
   return;
  }
 if (VIDi_InjectContext[i]->EVT_Handle==0)
  {
   STTBX_Print(("VIDi_InjectCallback(?):**ERROR** !!! Invalid event handle !!!\n"));
   return;
  }

 /* Switch according to the event received */
 /* ====================================== */
 switch(Event)
  {
   /* Video stopped event */
   /* ------------------- */
   case STVID_STOPPED_EVT : VIDi_TraceDBG(("VIDi_InjectCopy(%d):STVID_STOPPED_EVT\n",VIDi_InjectContext[i]->InjectorId));
                            Semaphore_Signal(VIDi_InjectContext[i]->SemVideoStopped); 
                            break;

   /* Strange event */
   /* ------------- */
   default : 
    STTBX_Print(("VIDi_InjectCallback(%d):**ERROR** !!! Event 0x%08x received but not managed !!!\n",VIDi_InjectContext[i]->InjectorId,Event));
    break;
  }
}

#endif 

/* ========================================================================
   Name:        VIDi_GetWritePtr
   Description: Return the current write pointer of the buffer
   ======================================================================== */

#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
static ST_ErrorCode_t VIDi_GetWritePtr(void *const Handle,void **const Address)
{
 VID_InjectContext_t *VID_InjectContext;

 /* NULL address are ignored ! */
 /* -------------------------- */
 if (Address==NULL) 
  {
   STTBX_Print(("VIDi_GetWritePtr(?):**ERROR** !!! Address is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }

 /* Retrieve context */
 /* ---------------- */
 VID_InjectContext=(VID_InjectContext_t *)Handle;
 if (VID_InjectContext==NULL) 
  {
   STTBX_Print(("VIDi_GetWritePtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
   return(ST_ERROR_BAD_PARAMETER);
  }      

 /* Set the value of the read pointer */
 /* --------------------------------- */
 Semaphore_Wait(VID_InjectContext->SemLock);
 *Address=(void *)VID_InjectContext->BufferVideoProducer;
 Semaphore_Signal(VID_InjectContext->SemLock);

 /* Return no error */
 /* --------------- */
 /*VIDi_TraceDBG(("VIDi_GetWritePtr(%d): Address=0x%08x\n",VID_InjectContext->InjectorId,*Address));*/
 return(ST_NO_ERROR);
}
#endif

/* ========================================================================
   Name:        VIDi_SetReadPtr
   Description: Set the read pointer of the back buffer
   ======================================================================== */

#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
static void VIDi_SetReadPtr(void *const Handle,void *const Address)
{
 VID_InjectContext_t *VID_InjectContext;

 /* NULL address are ignored ! */
 /* -------------------------- */
 if (Address==NULL) 
  {
   STTBX_Print(("VIDi_SetReadPtr(?):**ERROR** !!! Address is NULL pointer !!!\n"));
   return;
  }

 /* Retrieve context */
 /* ---------------- */
 VID_InjectContext=(VID_InjectContext_t *)Handle;
 if (VID_InjectContext==NULL) 
  {
   STTBX_Print(("VID_SetReadPtr(?):**ERROR** !!! Context is NULL pointer !!!\n"));
   return;
  }

 /* Set the value of the read pointer */
 /* --------------------------------- */
 Semaphore_Wait(VID_InjectContext->SemLock);
 VID_InjectContext->BufferVideoConsumer=(U32)Address;
 Semaphore_Signal(VID_InjectContext->SemLock);

}
#endif




/*End of file*/


