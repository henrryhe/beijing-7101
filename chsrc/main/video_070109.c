/******************************************************************************/
/*                                                                            */
/* File name   : VIDEO.C                                                      */
/*                                                                            */
/* Description : Initialization of the video driver                           */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/*                                                                            */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 02/03/05           Created                      M.CHAPPELLIER              */
/*                                                                            */
/******************************************************************************/

/* Includes */
/* -------- */
#include "initterm.h"
#include "CH_sys.h"

#define VID_MAX_NUMBER 2
#define LAYER_MAX_NUMBER 2
/* Flag to inject using DMAs */
/* ------------------------- */
#if defined(ST_5100)||defined(ST_5105)/*||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)/**/
#define INJECTOR_USE_DMA
#endif


/* Macro definitions */
/* ----------------- */

#define VIDi_TraceDBG(x) 

ST_ErrorCode_t VID_OpenViewPort(U32 VID_Index,U32 LAYER_Index,STVID_ViewPortHandle_t *VIDVPHandle);
ST_ErrorCode_t VID_CloseViewPort(U32 VID_Index,STVID_ViewPortHandle_t *VIDVPHandle);


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
	STVID_Handle_t       VIDHandle;            /* Video handle                         */
	STEVT_Handle_t       EVT_Handle;            /* Event handle                         */
	STVID_StartParams_t  VID_StartParams;       /* Video start parameters               */
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


extern STVID_Handle_t VIDHandle[2];
extern STVID_ViewPortHandle_t VIDVPHandle[2][2];



/* Local Prototypes */
/* ---------------- */
static void           VIDi_InjectCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);




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


#if 0 /*yxl 2007-01-9 modify below section*/

/* ========================================================================
   Name:        VID_InjectStart
   Description: Start an video injection
   ======================================================================== */
ST_ErrorCode_t VID_InjectStart(U32 DeviceId,U8 *FileName,STVID_StartParams_t *VID_StartParams,U32 NbLoops)
{
	void                         *FILE_Handle;
	U32                           FILE_Size;
	U8                           *FILE_Buffer;
	U8                           *FILE_Buffer_Aligned;
	U32                           FILE_BufferSize;
	STEVT_OpenParams_t            EVT_OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
	ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
	STVID_Freeze_t                VID_Freeze;
	
	/* If the injecter is already in place, go out */
	/* ------------------------------------------- */
	if (VIDi_InjectContext[DeviceId]!=NULL)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Injector %d  is already running !!!\n",DeviceId));
		return(ST_ERROR_ALREADY_INITIALIZED);
	}
	
	/* Check presence of the file */
	/* -------------------------- */
	FILE_Handle=SYS_FOpen((char *)FileName,"rb");
	if (FILE_Handle==NULL) 
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! File doesn't exists !!!\n",DeviceId));
		return(ST_ERROR_BAD_PARAMETER);
	}
	SYS_FSeek(FILE_Handle,0,SEEK_END);
	FILE_Size=SYS_FTell(FILE_Handle);
	SYS_FClose(FILE_Handle);
	
	/* Select method of injection, read from disk or read from memory */
	/* -------------------------------------------------------------- */
	if ((FileName[0]!='/') || ((FileName[0]=='/') && (FILE_Size<(1*1024*1024))))
	{
		/* This file is coming from an external file system, try to load all the file into memory */
		FILE_BufferSize     = FILE_Size;
		FILE_Buffer         = Memory_Allocate(NcachePartition,FILE_BufferSize+256);
		FILE_Buffer_Aligned = (U8 *)((((U32)FILE_Buffer)+255)&0xFFFFFF00);
		if (FILE_Buffer==NULL)
		{
			STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n",DeviceId));
			return(ST_ERROR_NO_MEMORY);
		} 
		/* Load the file into memory */
		FILE_Handle=SYS_FOpen((char *)FileName,"rb");
		if (FILE_Handle==NULL) 
		{
			STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));
			Memory_Deallocate(NcachePartition,FILE_Buffer);
			return(ST_ERROR_NO_MEMORY);
		}
		SYS_FRead(FILE_Buffer_Aligned,FILE_Size,1,FILE_Handle);
		SYS_FClose(FILE_Handle);
		/* No more access to the file */
		FILE_Handle=0;
	}
	/* Else, this is a read file on the fly */
	/* ------------------------------------ */
	else
	{
		FILE_BufferSize     = 1*1024*1024;
		FILE_Buffer         = Memory_Allocate(NcachePartition,FILE_BufferSize+256);
		FILE_Buffer_Aligned = (U8 *)((((U32)FILE_Buffer)+255)&0xFFFFFF00);
		if (FILE_Buffer==NULL)
		{
			STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n",DeviceId));
			return(ST_ERROR_NO_MEMORY);/*/**/
			
		} 
		/* Open the file */
		FILE_Handle=SYS_FOpen((char *)FileName,"rb");
		if (FILE_Handle==NULL) 
		{
			/*   STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));*/
			Memory_Deallocate(NcachePartition,FILE_Buffer);
			return(ST_ERROR_NO_MEMORY);/**/
			
		}
	}
	
	
	/* Allocate a context */
	/* ------------------ */
	VIDi_InjectContext[DeviceId]=Memory_Allocate(SystemPartition,sizeof(VID_InjectContext_t));
	if (VIDi_InjectContext[DeviceId]==NULL)
	{ 
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate memory !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		return(ST_ERROR_NO_MEMORY);/**/
	}
	memset(VIDi_InjectContext[DeviceId],0,sizeof(VID_InjectContext_t));
	
	/* Fill up the context */
	/* ------------------- */
	VIDi_InjectContext[DeviceId]->VIDHandle          = VIDHandle[DeviceId];
	VIDi_InjectContext[DeviceId]->InjectorId          = DeviceId;
	VIDi_InjectContext[DeviceId]->VID_StartParams     = *VID_StartParams;
	VIDi_InjectContext[DeviceId]->BufferPtr           = (U32)FILE_Buffer;
	VIDi_InjectContext[DeviceId]->BufferBase          = (U32)FILE_Buffer_Aligned;
	VIDi_InjectContext[DeviceId]->BufferConsumer      = (U32)FILE_Buffer_Aligned;
	VIDi_InjectContext[DeviceId]->BufferProducer      = (U32)FILE_Buffer_Aligned;
	VIDi_InjectContext[DeviceId]->BufferSize          = FILE_BufferSize;
	VIDi_InjectContext[DeviceId]->FileSize            = FILE_Size;
	VIDi_InjectContext[DeviceId]->FileHandle          = FILE_Handle;
	VIDi_InjectContext[DeviceId]->FilePosition        = 0;
	VIDi_InjectContext[DeviceId]->NbLoops             = NbLoops;
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
	ErrCode|=STVID_GetBitBufferFreeSize(VIDHandle[DeviceId],&VIDi_InjectContext[DeviceId]->BitBufferVideoSize); 
	ErrCode|=STVID_GetDataInputBufferParams(VIDHandle[DeviceId],(void **)&(VIDi_InjectContext[DeviceId]->BufferVideo),&(VIDi_InjectContext[DeviceId]->BufferVideoSize));
#if defined(ST_7100)||defined(ST_7109)
	/*ErrCode|=VID_OpenViewPort(DeviceId,0,&VIDVPHandle[DeviceId][0]);
	ErrCode|=VID_OpenViewPort(DeviceId,1,&VIDVPHandle[DeviceId][1]);
	*/
	VIDi_InjectContext[DeviceId]->BufferVideo = (U32)VIDi_InjectContext[DeviceId]->BufferVideo&0x1FFFFFFF; 
#endif
	VIDi_InjectContext[DeviceId]->BufferVideoProducer = VIDi_InjectContext[DeviceId]->BufferVideoConsumer = VIDi_InjectContext[DeviceId]->BufferVideo;
	ErrCode|=STVID_SetDataInputInterface(VIDHandle[DeviceId],VIDi_GetWritePtr,VIDi_SetReadPtr,VIDi_InjectContext[DeviceId]);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to setup the video buffer !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
#endif

	/* Setup for FDMA transfer */
	/* ----------------------- */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
	VIDi_InjectContext[DeviceId]->FdmaNode=Memory_Allocate(NcachePartition,sizeof(STFDMA_Node_t)+256);
	if (VIDi_InjectContext[DeviceId]->FdmaNode==NULL)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate a fdma node!!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ST_ERROR_NO_MEMORY);
	}
	VIDi_InjectContext[DeviceId]->FdmaNodeAligned=(STFDMA_Node_t *)(((U32)VIDi_InjectContext[DeviceId]->FdmaNode+255)&0xFFFFFF00); 
	ErrCode=STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL/*STFDMA_HIGH_BANDWIDTH_POOL*/,&VIDi_InjectContext[DeviceId]->FdmaChannelId);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to lock a channel !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
#endif
	
	/* Subscribe to events */
	/* ------------------- */
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
	memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
	EVT_SubcribeParams.NotifyCallback=VIDi_InjectCallback;
	ErrCode|=STEVT_Open(EVTDeviceName,&EVT_OpenParams,&(VIDi_InjectContext[DeviceId]->EVT_Handle));
	ErrCode|=STEVT_SubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT,&EVT_SubcribeParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to subscribe to video events !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
	
	/* Start the video decoder */
	/* ----------------------- */
	ErrCode|=STVID_DisableSynchronisation(VIDHandle[DeviceId]); 
	ErrCode|=STVID_SetSpeed(VIDHandle[DeviceId],100); 
	ErrCode|=STVID_Start(VIDHandle[DeviceId],VID_StartParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to start the video decoder !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
		STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
		STVID_DeleteDataInputInterface(VIDHandle[DeviceId]); 
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
	
	/* Create the inject task */
	/* ---------------------- */
	VIDi_InjectContext[DeviceId]->Task=Task_Create(VIDi_InjectTask,VIDi_InjectContext[DeviceId],2048,MAX_USER_PRIORITY/2,"VID_InjectorTask",0);
	if (VIDi_InjectContext[DeviceId]->Task==NULL)
	{
		VID_Freeze.Field = STVID_FREEZE_FIELD_TOP;
		VID_Freeze.Mode  = STVID_FREEZE_MODE_NONE;
		STVID_Stop(VIDHandle[DeviceId],STVID_STOP_NOW,&VID_Freeze);
		STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
		STVID_DeleteDataInputInterface(VIDHandle[DeviceId]); 
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		/*  STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to create the injector task !!!\n",DeviceId));*/
		return(ST_ERROR_NO_MEMORY);
	}
	
	/* Start the injector */
	/* ------------------ */
	Semaphore_Signal(VIDi_InjectContext[DeviceId]->SemStart);
	
	/* Return no errors */
	/* ---------------- */
	return(ST_NO_ERROR);
}

#else

/* ========================================================================
   Name:        VID_InjectStart
   Description: Start an video injection
   ======================================================================== */
ST_ErrorCode_t VID_InjectStart(U32 DeviceId,U8 *FileName,STVID_StartParams_t *VID_StartParams,U32 NbLoops)
{
	void                         *FILE_Handle;
	U32                           FILE_Size;
	U8                           *FILE_Buffer;
	U8                           *FILE_Buffer_Aligned;
	U32                           FILE_BufferSize;
	STEVT_OpenParams_t            EVT_OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
	ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
	STVID_Freeze_t                VID_Freeze;
	
	/* If the injecter is already in place, go out */
	/* ------------------------------------------- */
	if (VIDi_InjectContext[DeviceId]!=NULL)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Injector %d  is already running !!!\n",DeviceId));
		return(ST_ERROR_ALREADY_INITIALIZED);
	}
	
	/* Check presence of the file */
	/* -------------------------- */
	FILE_Handle=SYS_FOpen((char *)FileName,"rb");
	if (FILE_Handle==NULL) 
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! File doesn't exists !!!\n",DeviceId));
		return(ST_ERROR_BAD_PARAMETER);
	}
	SYS_FSeek(FILE_Handle,0,SEEK_END);
	FILE_Size=SYS_FTell(FILE_Handle);
	SYS_FClose(FILE_Handle);
	
	/* Select method of injection, read from disk or read from memory */
	/* -------------------------------------------------------------- */
	if ((FileName[0]!='/') || ((FileName[0]=='/') && (FILE_Size<(1*1024*1024))))
	{
		/* This file is coming from an external file system, try to load all the file into memory */
		FILE_BufferSize     = FILE_Size;
		FILE_Buffer         = Memory_Allocate(NcachePartition,FILE_BufferSize+256);
		FILE_Buffer_Aligned = (U8 *)((((U32)FILE_Buffer)+255)&0xFFFFFF00);
		if (FILE_Buffer==NULL)
		{
			STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n",DeviceId));
			return(ST_ERROR_NO_MEMORY);
		} 
		/* Load the file into memory */
		FILE_Handle=SYS_FOpen((char *)FileName,"rb");
		if (FILE_Handle==NULL) 
		{
			STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));
			Memory_Deallocate(NcachePartition,FILE_Buffer);
			return(ST_ERROR_NO_MEMORY);
		}
		SYS_FRead(FILE_Buffer_Aligned,FILE_Size,1,FILE_Handle);
		SYS_FClose(FILE_Handle);
		/* No more access to the file */
		FILE_Handle=0;
	}
	/* Else, this is a read file on the fly */
	/* ------------------------------------ */
	else
	{
		FILE_BufferSize     = 1*1024*1024;
		FILE_Buffer         = Memory_Allocate(NcachePartition,FILE_BufferSize+256);
		FILE_Buffer_Aligned = (U8 *)((((U32)FILE_Buffer)+255)&0xFFFFFF00);
		if (FILE_Buffer==NULL)
		{
			STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate memory to load the file !!!\n",DeviceId));
			return(ST_ERROR_NO_MEMORY);/*/**/
			
		} 
		/* Open the file */
		FILE_Handle=SYS_FOpen((char *)FileName,"rb");
		if (FILE_Handle==NULL) 
		{
			/*   STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to open the file !!!\n",DeviceId));*/
			Memory_Deallocate(NcachePartition,FILE_Buffer);
			return(ST_ERROR_NO_MEMORY);/**/
			
		}
	}
	
	
	/* Allocate a context */
	/* ------------------ */
	VIDi_InjectContext[DeviceId]=Memory_Allocate(SystemPartition,sizeof(VID_InjectContext_t));
	if (VIDi_InjectContext[DeviceId]==NULL)
	{ 
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate memory !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		return(ST_ERROR_NO_MEMORY);/**/
	}
	memset(VIDi_InjectContext[DeviceId],0,sizeof(VID_InjectContext_t));
	
	/* Fill up the context */
	/* ------------------- */
	VIDi_InjectContext[DeviceId]->VIDHandle          = VIDHandle[DeviceId];
	VIDi_InjectContext[DeviceId]->InjectorId          = DeviceId;
	VIDi_InjectContext[DeviceId]->VID_StartParams     = *VID_StartParams;
	VIDi_InjectContext[DeviceId]->BufferPtr           = (U32)FILE_Buffer;
	VIDi_InjectContext[DeviceId]->BufferBase          = (U32)FILE_Buffer_Aligned;
	VIDi_InjectContext[DeviceId]->BufferConsumer      = (U32)FILE_Buffer_Aligned;
	VIDi_InjectContext[DeviceId]->BufferProducer      = (U32)FILE_Buffer_Aligned;
	VIDi_InjectContext[DeviceId]->BufferSize          = FILE_BufferSize;
	VIDi_InjectContext[DeviceId]->FileSize            = FILE_Size;
	VIDi_InjectContext[DeviceId]->FileHandle          = FILE_Handle;
	VIDi_InjectContext[DeviceId]->FilePosition        = 0;
	VIDi_InjectContext[DeviceId]->NbLoops             = NbLoops;
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
	ErrCode|=STVID_GetBitBufferFreeSize(VIDHandle[DeviceId],&VIDi_InjectContext[DeviceId]->BitBufferVideoSize); 
	ErrCode|=STVID_GetDataInputBufferParams(VIDHandle[DeviceId],(void **)&(VIDi_InjectContext[DeviceId]->BufferVideo),&(VIDi_InjectContext[DeviceId]->BufferVideoSize));
#if defined(ST_7100)||defined(ST_7109)
	/*ErrCode|=VID_OpenViewPort(DeviceId,0,&VIDVPHandle[DeviceId][0]);
	ErrCode|=VID_OpenViewPort(DeviceId,1,&VIDVPHandle[DeviceId][1]);
	*/
	VIDi_InjectContext[DeviceId]->BufferVideo = (U32)VIDi_InjectContext[DeviceId]->BufferVideo&0x1FFFFFFF; 
#endif
	VIDi_InjectContext[DeviceId]->BufferVideoProducer = VIDi_InjectContext[DeviceId]->BufferVideoConsumer = VIDi_InjectContext[DeviceId]->BufferVideo;
	ErrCode|=STVID_SetDataInputInterface(VIDHandle[DeviceId],VIDi_GetWritePtr,VIDi_SetReadPtr,VIDi_InjectContext[DeviceId]);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to setup the video buffer !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
#endif

	/* Setup for FDMA transfer */
	/* ----------------------- */
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
	VIDi_InjectContext[DeviceId]->FdmaNode=Memory_Allocate(NcachePartition,sizeof(STFDMA_Node_t)+256);
	if (VIDi_InjectContext[DeviceId]->FdmaNode==NULL)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to allocate a fdma node!!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ST_ERROR_NO_MEMORY);
	}
	VIDi_InjectContext[DeviceId]->FdmaNodeAligned=(STFDMA_Node_t *)(((U32)VIDi_InjectContext[DeviceId]->FdmaNode+255)&0xFFFFFF00); 
	ErrCode=STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL/*STFDMA_HIGH_BANDWIDTH_POOL*/,&VIDi_InjectContext[DeviceId]->FdmaChannelId);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to lock a channel !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
#endif
	
	/* Subscribe to events */
	/* ------------------- */
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
	memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
	EVT_SubcribeParams.NotifyCallback=VIDi_InjectCallback;
	ErrCode|=STEVT_Open(EVTDeviceName,&EVT_OpenParams,&(VIDi_InjectContext[DeviceId]->EVT_Handle));
	ErrCode|=STEVT_SubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT,&EVT_SubcribeParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to subscribe to video events !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
	
	/* Start the video decoder */
	/* ----------------------- */
	ErrCode|=STVID_DisableSynchronisation(VIDHandle[DeviceId]); 
	ErrCode|=STVID_SetSpeed(VIDHandle[DeviceId],100); 
	ErrCode|=STVID_Start(VIDHandle[DeviceId],VID_StartParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to start the video decoder !!!\n",DeviceId));
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
		STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
		STVID_DeleteDataInputInterface(VIDHandle[DeviceId]); 
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		return(ErrCode);
	}
	
	/* Create the inject task */
	/* ---------------------- */
	VIDi_InjectContext[DeviceId]->Task=Task_Create(VIDi_InjectTask,VIDi_InjectContext[DeviceId],2048,MAX_USER_PRIORITY/2+3,"VID_InjectorTask",0);
	if (VIDi_InjectContext[DeviceId]->Task==NULL)
	{
		VID_Freeze.Field = STVID_FREEZE_FIELD_TOP;
		VID_Freeze.Mode  = STVID_FREEZE_MODE_NONE;
		STVID_Stop(VIDHandle[DeviceId],STVID_STOP_NOW,&VID_Freeze);
		STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
		if (FILE_Handle!=NULL) SYS_FClose(FILE_Handle);
#if defined(ST_7100)||defined(ST_7109)
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
		VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
#endif
#if defined(INJECTOR_USE_DMA)&&(defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710))
		Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
		STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
#endif
#if defined(ST_5100)||defined(ST_5105)||defined(ST_5301)||defined(ST_7100)||defined(ST_7109)||defined(ST_7710)
		STVID_DeleteDataInputInterface(VIDHandle[DeviceId]); 
#endif
		Memory_Deallocate(NcachePartition,FILE_Buffer);
		Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
		VIDi_InjectContext[DeviceId]=NULL;
		/*  STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to create the injector task !!!\n",DeviceId));*/
		return(ST_ERROR_NO_MEMORY);
	}
	
	/* Start the injector */
	/* ------------------ */
	Semaphore_Signal(VIDi_InjectContext[DeviceId]->SemStart);
	
	/* Return no errors */
	/* ---------------- */
	return(ST_NO_ERROR);
}

#endif

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
#if 0
 {
  Clock_t TimeOut=Time_Plus(Time_Now(),ST_GetClocksPerSecond());
  Semaphore_Wait_Timeout(VIDi_InjectContext[DeviceId]->SemVideoStopped,&TimeOut);
 }
#endif
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
 ErrCode|=VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][0]);
 ErrCode|=VID_CloseViewPort(DeviceId,&VIDVPHandle[DeviceId][1]);
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
 Memory_Deallocate(NcachePartition,VIDi_InjectContext[DeviceId]->FdmaNode);
 ErrCode=STFDMA_UnlockChannel(VIDi_InjectContext[DeviceId]->FdmaChannelId);
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("VID_InjectStart(%d):**ERROR** !!! Unable to unlock the fdma channel !!!\n",DeviceId));
   return(ErrCode);
  }
#endif

 /* Unsubscrive to events */
 /* --------------------- */
 ErrCode|=STEVT_UnsubscribeDeviceEvent(VIDi_InjectContext[DeviceId]->EVT_Handle,VOUTDeviceName[DeviceId],STVID_STOPPED_EVT);
 ErrCode|=STEVT_Close(VIDi_InjectContext[DeviceId]->EVT_Handle);
 if (ErrCode!=ST_NO_ERROR)
  {
   STTBX_Print(("VID_InjectStop(%d):**ERROR** !!! Unable to unsubscribe to video events !!!\n",DeviceId));
   return(ErrCode);
  } 

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
 Memory_Deallocate(NcachePartition,(U8 *)VIDi_InjectContext[DeviceId]->BufferPtr);
 Memory_Deallocate(SystemPartition,VIDi_InjectContext[DeviceId]);
 VIDi_InjectContext[DeviceId]=NULL;

 /* Return no errors */
 /* ---------------- */
 return(ST_NO_ERROR);
}

/* ========================================================================
   Name:        VID_OpenViewPort
   Description: Open the video view port 
   ======================================================================== */

ST_ErrorCode_t VID_OpenViewPort(U32 VID_Index,U32 LAYER_Index,STVID_ViewPortHandle_t *VIDVPHandle)
{
 ST_ErrorCode_t         ErrCode=ST_NO_ERROR;
 STVID_ViewPortParams_t VID_ViewPortParams;
 STVTG_TimingMode_t     VTG_TimingMode;
 STVTG_ModeParams_t     VTG_ModeParams;

 /* Check if index is invalid */
 /* ------------------------- */
 if (VID_Index>=VID_MAX_NUMBER)
  {
   return(ST_ERROR_INVALID_HANDLE);
  }

 /* Check if index is invalid */
 /* ------------------------- */
 if (LAYER_Index>=LAYER_MAX_NUMBER)
  {
   return(ST_ERROR_INVALID_HANDLE);
  }

 /* If invalid pointer, go out */
 /* -------------------------- */
 if (VIDVPHandle==NULL)
  { 
   return(ST_ERROR_INVALID_HANDLE);
  }
  
 /* If we are in SD mode, don't create extra view port */
 /* -------------------------------------------------- */
 if (LAYER_Index==1)
  {
   ErrCode=STVTG_GetMode(VTGHandle[VTG_MAIN],&VTG_TimingMode,&VTG_ModeParams);
   if (ErrCode!=ST_NO_ERROR)
    {
     return(ST_ERROR_INVALID_HANDLE);
    }
   if (VTG_TimingMode==STVTG_TIMING_MODE_576I50000_13500) 
    {
     *VIDVPHandle=NULL;
     return(ST_NO_ERROR);
    }
  }

 /* Open the view port */
 /* ------------------ */
 memset(&VID_ViewPortParams,0,sizeof(STVID_ViewPortParams_t));
 strcpy(VID_ViewPortParams.LayerName,LAYER_DeviceName[LAYER_Index]); 
 ErrCode=STVID_OpenViewPort(VIDHandle[VID_Index],&VID_ViewPortParams,VIDVPHandle);
 if (ErrCode!=ST_NO_ERROR)
  {
	 		STTBX_Print(("STVID_OpenViewPort()=%s", GetErrorText(ErrCode)));	
	
   return(ErrCode);
  }
 ErrCode=STVID_SetDisplayAspectRatioConversion(*VIDVPHandle,STVID_DISPLAY_AR_CONVERSION_IGNORE/*STVID_DISPLAY_AR_CONVERSION_LETTER_BOX*//*STVID_DISPLAY_AR_CONVERSION_PAN_SCAN*/);
 if (ErrCode!=ST_NO_ERROR)
  {
   return(ErrCode);
  }

 /* Return no errors */
 /* ---------------- */
 return(ST_NO_ERROR);
}
 
/* ========================================================================
   Name:        VID_CloseViewPort
   Description: Close the video view port 
   ======================================================================== */

ST_ErrorCode_t VID_CloseViewPort(U32 VID_Index,STVID_ViewPortHandle_t *VIDVPHandle)
{
 ST_ErrorCode_t      ErrCode=ST_NO_ERROR;
 STVID_ClearParams_t VID_ClearParams;

 /* Check if index is invalid */
 /* ------------------------- */
 if (VID_Index>=VID_MAX_NUMBER)
  {
   return(ST_ERROR_INVALID_HANDLE);
  }

 /* If invalid pointer, go out */
 /* -------------------------- */
 if (VIDVPHandle==NULL)
  { 
   return(ST_ERROR_INVALID_HANDLE);
  }

 /* If already closed, go out */
 /* ------------------------- */
 if (*VIDVPHandle==NULL)
  { 
   return(ST_NO_ERROR);
  }

 /* Clear/Deallocate all the frame buffers as we close the view port */
 /* ---------------------------------------------------------------- */
 memset(&VID_ClearParams,0,sizeof(STVID_ClearParams_t));
 VID_ClearParams.ClearMode=STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER;
 ErrCode=STVID_Clear(VIDHandle[VID_Index],&VID_ClearParams);
 if (ErrCode!=ST_NO_ERROR)
  {
   return(ErrCode);
  }
 
 /* Close the view port */
 /* ------------------- */
 ErrCode=STVID_CloseViewPort(*VIDVPHandle);
 if (ErrCode!=ST_NO_ERROR)
  {
   return(ErrCode);
  }
 
 /* Clear view port handle */
 /* ---------------------- */
 *VIDVPHandle=0;

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
		Task_Delay(ST_GetClocksPerSecond()/10); /* Sleep 100ms */
  }
  
  /* Finish the task */
  /* =============== */
  Semaphore_Signal(VID_InjectContext->SemStop);
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
 /* For CD video fifos */
 /* ================== */
#if defined(ST_5528)
 {
 U32 i,FIFO,FIFO_FULL;
 
 /* Write in the CD fifo */
 /* -------------------- */
 if (VID_InjectContext->InjectorId==0) 
  {
   FIFO      = ST5528_VIDEO_CD_FIFO1_BASE_ADDRESS; 
   FIFO_FULL = ST5528_VID1_BASE_ADDRESS+0x3F8;
  }
 else
  {
   FIFO      = ST5528_VIDEO_CD_FIFO2_BASE_ADDRESS; 
   FIFO_FULL = ST5528_VID2_BASE_ADDRESS+0x3F8;
  }
 for (i=0;i<(SizeToCopy/4);i++)
  {   
   /* Wait 1ms if the 256byte fifo is full */
   while((*(DU32 *)FIFO_FULL)&0x10) Task_Delay(ST_GetClocksPerSecond()/1000);   
   *((DU32 *)FIFO)=((U32 *)VID_InjectContext->BufferConsumer)[i];
  }
 VID_InjectContext->BufferConsumer+=SizeToCopy;

 /* Trace for debug */
 /* --------------- */ 
 VIDi_TraceDBG(("VIDi_InjectCopy(%d): Size = 0x%08x - SrcPtr = 0x%08x\n",VID_InjectContext->InjectorId,SizeToCopy1,SizeToCopy2,VID_InjectContext->BufferConsumer));
 }
#endif

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

 /* Return no error */
 /* --------------- */
 /*VIDi_TraceDBG(("VIDi_SetReadPtr(%d): Address=0x%08x\n",VID_InjectContext->InjectorId,Address));*/
}
#endif






static void VIDi_Callback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
static STEVT_Handle_t EVT_VID_Handle;



static void VIDi_Callback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
static U32 diffVal = 0;
STVID_PictureParams_t*	pPictureInfo;
	
	switch(Event)
	{
		case STVID_FRAME_RATE_CHANGE_EVT:
			if(EventData)
			{
				pPictureInfo = (STVID_PictureParams_t*) EventData;
			/*	STTBX_Print("======== STVID_FRAME_RATE_CHANGE_EVT ==========\n");*/
				switch(pPictureInfo->Aspect)
				{
					case STVID_DISPLAY_ASPECT_RATIO_16TO9: STTBX_Print(("STVID_DISPLAY_ASPECT_RATIO_16TO9\n")); break;
					case STVID_DISPLAY_ASPECT_RATIO_4TO3: STTBX_Print(("STVID_DISPLAY_ASPECT_RATIO_4TO3\n")); break;
					case STVID_DISPLAY_ASPECT_RATIO_221TO1: STTBX_Print(("STVID_DISPLAY_ASPECT_RATIO_221TO1\n")); break;
					case STVID_DISPLAY_ASPECT_RATIO_SQUARE: STTBX_Print(("STVID_DISPLAY_ASPECT_RATIO_SQUARE\n")); break;
				}
				STTBX_Print(("Frame Rate %d\n", pPictureInfo->FrameRate));
				STTBX_Print(("Resolution %d X %d : %s\n", pPictureInfo->Width, pPictureInfo->Height, (pPictureInfo->ScanType == STVID_SCAN_TYPE_INTERLACED ? "I" : "P")));
				STTBX_Print(("======== STVID_FRAME_RATE_CHANGE_EVT ==========\n"));
			}
			break;
		case STVID_BACK_TO_SYNC_EVT:
			STTBX_Print(("STVID_BACK_TO_SYNC_EVT\n"));
			break;
		case STVID_SYNCHRONISATION_CHECK_EVT:
			{
		     	STVID_SynchronisationInfo_t *VID_SynchronisationInfo = (STVID_SynchronisationInfo_t *)EventData;
	     /* If the synchro is stable and there is a delay that the video decoder can't recover */
		     	if ((VID_SynchronisationInfo->IsSynchronisationOk==TRUE) && (VID_SynchronisationInfo->ClocksDifference!=0))
      			{
      				if(diffVal != (VID_SynchronisationInfo->ClocksDifference/90)) 
      				{
					//STTBX_Print("STVID_SYNCHRONISATION_CHECK_EVT\n");
					//STTBX_Print("AUD DR OFFSET NEEDED! %d\n", VID_SynchronisationInfo->ClocksDifference/90);
					diffVal = VID_SynchronisationInfo->ClocksDifference/90;
      				}
		     	}
			}			
			break;
		case STVID_SYNCHRO_EVT:
			STTBX_Print(("STVID_SYNCHRO_EVT\n"));
			break;
		case STVID_OUT_OF_SYNC_EVT:		
			STTBX_Print(("STVID_OUT_OF_SYNC_EVT\n"));
			break;
	}
}


void Yxl_Channeltest(void)
{       
	STVID_StartParams_t VID_StartParams;
	ST_ErrorCode_t	ErrCode;
	static U32 ID = 1;
	U16 count;
  
	VID_InjectStop(ID);
	
	if(ID) ID = 0;
	else ID = 1;
	
	/* Start the injector */
	/* ------------------ */
	memset(&VID_StartParams,0,sizeof(VID_StartParams));
	VID_StartParams.RealTime      = FALSE;
	VID_StartParams.UpdateDisplay = TRUE;
	VID_StartParams.BrdCstProfile = STVID_BROADCAST_DVB;
	if(ID)
		VID_StartParams.StreamType    = (STVID_StreamType_t)STVID_STREAM_TYPE_PES;
	else
		VID_StartParams.StreamType    = (STVID_StreamType_t)STVID_STREAM_TYPE_PES;
	VID_StartParams.StreamID      = STVID_IGNORE_ID;
	VID_StartParams.DecodeOnce    = FALSE;
#if 0
	if(ID)
		
		VID_InjectStart(ID ,(U8 *)".\\files\\teksoccer_pes.mp4",&VID_StartParams, 10);
	
	else
		VID_InjectStart(ID ,(U8 *)".\\files\\mpeg2_hd_pes.mp2",&VID_StartParams, 10);
#else
	ID=0;
		VID_StartParams.StreamType    = (STVID_StreamType_t)STVID_STREAM_TYPE_ES;
/*	VID_InjectStart(ID ,(U8 *)"trumpet.pes",&VID_StartParams, 10);*/
/*	VID_InjectStart(ID ,(U8 *)"susie.m1v",&VID_StartParams, 10);ok*/
	VID_InjectStart(ID ,(U8 *)"BK06.MPG",&VID_StartParams, 10);/*ok*/
#endif

	STVID_EnableOutputWindow(VIDVPHandle[ID][0]);
	STVID_EnableOutputWindow(VIDVPHandle[ID][1]);
	
	/* Return no errors */
	/* ---------------- */
	STTBX_Print(("--> Video injection started !\n"));
	
}


