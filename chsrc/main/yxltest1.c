#include "..\main\initterm.h"
#include "sttbx.h"
#include "appltype.h"
#include "..\dbase\vdbase.h"

/*#define YXL_DECODE yxl 2007-01-26 add this macro & connected*/
/*#define YXL_NO_CLKRV  yxl 2007-01-28 add this macro*/


#if 0
typedef enum
{
	CH_DVBC=0,/*DVBC platform*/
	CH_DVBT   /*DVBT platform*/
}CH_Platform;

typedef struct  transponder_info_table 
{
/*
* header_info
	*/
	char  status;                        /*  0= FREE: 1= OCCUPIED */
	SHORT  cPrevTransponderIndex;  /* -1= head record: x= offset in xpdr list which is prev to this */
	SHORT  cNextTransponderIndex;  /* -1= tail record: x= offset in xpdr list which is next to this */
	
								   /*
								   * body
	*/
	
	int abiTransponderNum;         /* 频点号 */
    WORD2SHORT  stTransponderNo;
	/*
	* stTransponderNo . sLo16 - TransportStream Id (16)  - NIT
	* stTransponderNo . sHi16 - Network Id (16)          - NIT
	*/
	char  acNetworkName[2][ 12 + 1 ];  /* name of the network */
	U8 TransType;/*频点类型,1->NVOD channel*/
	CH_Platform DatabaseType;/*数据库类型,CH_DVBC代表DVBC数据库,CH_DVBT代表DVBT数据库*/
	
	
    int   iTransponderFreq;        /* transponder freq in KHz */
	int	  iBandWidth;             /* band width in Mhz for DVBT ,no use for DVBC*/
	int   iSymbolRate;             /* symbol rate,no use for DVBt */
	BYTE  ucPwmVal;                /* PWM value to acheive the symbol rate ,no use for DVBT*/
	
	
	
   BYTE ucQAMMode;	/*no use for DVBT,0x00= not defined 
					0x01=16 QAM
													0x02=32 QAM
													0x03=64 QAM
													0x04=128 QAM
								   0x05=256 QAM*/
								   
	boolean abiIQInversion;/*no use for DVBT*/
	BYTE  SignalStrength;      /*yxl 2006-02-20 add this variable*/

} TRANSPONDER_INFO_STRUCT;

#endif 


void Yxl_PtiCount(void);

#ifdef YXL_DECODE  /*yxl 2007-01-26 */
ST_ErrorCode_t VideoGetWritePtrFct(void * const Handle_p, void ** const Address_p)
{
    ST_ErrorCode_t ErrCode;
	void* pWriteBufTemp;

    ErrCode = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle_p,&pWriteBufTemp);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferGetWritePointer()Failed %s\n", GetErrorText(ErrCode)));
		return ErrCode;
    }

	pWriteBufTemp=(void *)((U32)pWriteBufTemp&0x1FFFFFFF);
	*Address_p=pWriteBufTemp;

    return ST_NO_ERROR;
}


void VideoSetReadPtrFct(void * const Handle_p, void * const Address_p)
{
    ST_ErrorCode_t ErrCode;
	void* pReadBufTemp;

	pReadBufTemp=Address_p;
	pReadBufTemp=(void *)((U32)pReadBufTemp|0xA0000000);
	
	ErrCode = STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle_p,pReadBufTemp);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferSetReadPointer()Failed %s\n", GetErrorText(ErrCode)));
    }
	
    return;
}



static ST_ErrorCode_t AudioGetWritePtrFct(void *const Handle_p,void **const Address_p)
{                
	ST_ErrorCode_t ErrCode;
	void* pWriteBufTemp;
	
    ErrCode = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle_p,&pWriteBufTemp);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferGetWritePointer()Failed %s\n", GetErrorText(ErrCode)));
		return ErrCode;
    }
	
	*Address_p=(void *)((U32)pWriteBufTemp|0xA0000000);
	
    return ST_NO_ERROR;
}

static ST_ErrorCode_t AudioSetReadPtrFct(void *const Handle_p,void *const Address_p)
{
    ST_ErrorCode_t ErrCode;
	
	ErrCode = STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle_p,Address_p);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(( "STPTI_BufferSetReadPointer()Failed %s\n", GetErrorText(ErrCode)));
    }
	return ST_NO_ERROR;
}

#endif 

ST_DeviceName_t PTIDeviceName;
STPTI_Handle_t PTIHandle;


STPTI_Slot_t VideoSlot,AudioSlot,PCRSlot;

STAUD_Handle_t AUDHandle;
STVID_Handle_t VIDHandle[2];
STCLKRV_Handle_t CLKRVHandle;

STVID_ViewPortHandle_t VIDVPHandle[2][2];


STVID_ViewPortHandle_t VIDAUXVPHandle;

#if 0
/*分配和配置LAYER_VIDEO1、AUDEO AND PCR slot*/
BOOL CH_AVSlotInit(void) /*7100*/
{
    ST_ErrorCode_t ErrCode;
	STPTI_SlotData_t SlotData;
	STPTI_DMAParams_t      DMAParams;
    STCLKRV_SourceParams_t CLKRVSourceParams;
	U32 DMAUsed;
	void* Video_PESBufferAdd_p;
    U32   Video_PESBufferSize;
	STPTI_Buffer_t BufHandleTemp;

#if 0 /*yxl 2007-01-25 add*/


	PTIHandle=PTI_Handle[0]; 
	VIDHandle[0]=VID_Handle[0];
	AUDHandle=AUD_Handle[0];
	CLKRVHandle=CLKRV_Handle[0];

/*	strcpy(,VID_DeviceName[0]);
	strcpy(YYM.AUD_DeviceName,AUD_DeviceName[0]);
	strcpy(YYM.EVT_DeviceName,EVT_DeviceName[0]);*/
	strcpy(PTIDeviceName,PTI_DeviceName[0]);
		

#endif /*end yxl 2007-01-25 add*/

	memset((void *)&SlotData, 0, sizeof(STPTI_SlotData_t));
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
#if 1 /*2007-01-09*/
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
#else
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = TRUE;
#endif
	SlotData.SlotFlags.SoftwareCDFifo = TRUE; 

	/*Video slot*/
    ErrCode = STPTI_SlotAllocate(PTIHandle,&VideoSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(Video)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	ErrCode = STPTI_SlotClearPid(VideoSlot);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotClearPid(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

	
/*yxl 2007-01-04 add below*/
#if 1
	ErrCode = STPTI_SlotSetCCControl(VideoSlot,FALSE/*TRUE*/);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotSetCCControl(Video)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
#endif
/*end yxl 2007-01-04 add below*/

#ifdef YXL_DECODE 

	ErrCode = STVID_GetDataInputBufferParams(VIDHandle[0],(void **)&Video_PESBufferAdd_p,&Video_PESBufferSize);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_GetDataInputBufferParams Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STPTI_BufferAllocateManual(PTIHandle,
                                           (U8*)Video_PESBufferAdd_p, Video_PESBufferSize,
                                           1, &BufHandleTemp);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_BufferAllocateManual Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }


    ErrCode = STPTI_SlotLinkToBuffer(VideoSlot,BufHandleTemp);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTPTI_SlotLinkToBuffer Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

#if 0 /*yxl 2006-12-31 modify */
    ErrCode = STPTI_BufferSetOverflowControl(BufHandleTemp, TRUE);/*FALSE*/
#else
    ErrCode = STPTI_BufferSetOverflowControl(BufHandleTemp, FALSE);/*FALSE*/

#endif/*end yxl 2006-12-31 modify */
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

    ErrCode = STVID_SetDataInputInterface( VIDHandle[0],
                                           VideoGetWritePtrFct, VideoSetReadPtrFct,
                                           (void * const)BufHandleTemp);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\nSTVID_SetDataInputInterface Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }


	ErrCode=STPTI_BufferSetReadPointer(BufHandleTemp,Video_PESBufferAdd_p);
	if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print (("\n STPTI_BufferSetOverflowControl Failed!!! Error = %s \n",GetErrorText(ErrCode)));
        return TRUE;
    }

/*	ErrCode=STVID_GetBitBufferFreeSize(VIDHandle[0],&PRM->PTI_VideoBitBufferSize);*/

#endif

	/*Audio slot*/
    ErrCode = STPTI_SlotAllocate(PTIHandle,&AudioSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(Audio)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	ErrCode = STPTI_SlotClearPid(AudioSlot);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_SlotClearPid(Audio)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

	ErrCode = STPTI_SlotSetCCControl(AudioSlot,FALSE/*TRUE*/);
    if (ErrCode != ST_NO_ERROR)

    {
        STTBX_Print(("STPTI_SlotSetCCControl(AudioSlot)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }
#ifdef YXL_DECODE 

	{
		STAUD_BufferParams_t AUDBufferParams;
		STPTI_Buffer_t AUDBufHandleTemp;
		
		ErrCode=STAUD_IPGetInputBufferParams(AUDHandle,STAUD_OBJECT_INPUT_CD0,&AUDBufferParams);
		
	/*	PRM->PTI_AudioBufferBase = (U32)AUDBufferParams.BufferBaseAddr_p;
		PRM->PTI_AudioBufferSize = AUDBufferParams.BufferSize;
	*/
		ErrCode=STPTI_BufferAllocateManual(PTIHandle,
			(U8 *)AUDBufferParams.BufferBaseAddr_p,
			AUDBufferParams.BufferSize,
			1,&AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferAllocateManual(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		ErrCode=STPTI_SlotLinkToBuffer(AudioSlot,AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotLinkToBuffer(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		
		ErrCode=STPTI_BufferSetOverflowControl(AUDBufHandleTemp,FALSE);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferSetOverflowControl(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
		
		ErrCode=STAUD_IPSetDataInputInterface(AUDHandle,STAUD_OBJECT_INPUT_CD0,
			AudioGetWritePtrFct,AudioSetReadPtrFct,(void *)AUDBufHandleTemp);
		if(ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STAUD_IPSetDataInputInterface(Audio)=%s\n", GetErrorText(ErrCode)));
			return TRUE;
		}   
	}

#endif

#ifndef YXL_NO_CLKRV
	/*PCR Slot*/
    SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
	SlotData.SlotFlags.SoftwareCDFifo = FALSE; 
    ErrCode = STPTI_SlotAllocate(PTIHandle,&PCRSlot, &SlotData);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print((" STPTI_SlotAllocate(PCR)=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

	ErrCode = STPTI_SlotClearPid(PCRSlot);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STPTI_SlotClearPid(PCR)=%s\n",GetErrorText(ErrCode)));
        return TRUE;
     }

#ifdef YXL_DECODE 
#if 1 /*yxl 2007-01-02 add*/
	   ErrCode=STPTI_SlotSetCCControl(PCRSlot,TRUE/*FALSE*/);
	   if (ErrCode != ST_NO_ERROR)
	   {
		   STTBX_Print(("STPTI_SlotSetCCControl(PCR)=%s\n",GetErrorText(ErrCode)));
		   return TRUE;
	   }
	   
	   
	   
	   ErrCode=STCLKRV_InvDecodeClk(CLKRVHandle);
	   if (ErrCode != ST_NO_ERROR)
	   {
		   STTBX_Print(("STCLKRV_InvDecodeClk()=%s\n",GetErrorText(ErrCode)));
		   return TRUE;
	   }
	   
	   

	
#endif 
    CLKRVSourceParams.Source= STCLKRV_PCR_SOURCE_PTI;
    CLKRVSourceParams.Source_u.STPTI_s.Slot=PCRSlot;


    ErrCode = STCLKRV_SetPCRSource(CLKRVHandle, &CLKRVSourceParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetPCRSource()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#if 0 /*yxl 2007-01-02 cancel below*/
	ErrCode = STCLKRV_SetSTCSource(CLKRVHandle, STCLKRV_STC_SOURCE_PCR);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetSTCSource()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#endif 
	ErrCode=STCLKRV_SetSTCOffset(CLKRVHandle,(((-160)*90000)/1000));
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_SetSTCOffset()=%s", GetErrorText(ErrCode) ));
        return TRUE;
    }
#endif

#endif /*YXL_NO_CLKRV*/

#if 0 /*yxl 2007-01-02 add below*/
	{   
		STEVT_OpenParams_t            EVT_OpenParams;
		STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
		STVID_ConfigureEventParams_t  VID_ConfigureEventParams;
		/* Subscribe to video event to manage the display */
		/* ---------------------------------------------- */
		memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
		memset(&EVT_OpenParams    ,0,sizeof(STEVT_OpenParams_t));
		EVT_SubcribeParams.NotifyCallback=STPRMi_VID_Callback;
#if 1 /*yxl 2007-01-04 temp cancel*/
	/*	ErrCode|=STEVT_Open(PRM->EVT_DeviceName,&EVT_OpenParams,&(EVTHandle));*/
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[0],STVID_SYNCHRONISATION_CHECK_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[0],STVID_DATA_ERROR_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[0],STVID_DATA_OVERFLOW_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[0],STVID_DATA_UNDERFLOW_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[0],STVID_PICTURE_DECODING_ERROR_EVT,&EVT_SubcribeParams);
		ErrCode|=STEVT_SubscribeDeviceEvent(EVTHandle,VIDDeviceName[0],STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,&EVT_SubcribeParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPRM_PlayStart():**ERROR** !!! Unable to subscrive to video events !!!\n"));
		
			return TRUE;
		}
#endif 		
		/* Configure the video events */
		/* -------------------------- */
		memset(&VID_ConfigureEventParams,0,sizeof(STVID_ConfigureEventParams_t));
		VID_ConfigureEventParams.Enable              = TRUE;
		VID_ConfigureEventParams.NotificationsToSkip = 0;
		ErrCode|=STVID_ConfigureEvent(VIDHandle[0],STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,&VID_ConfigureEventParams);
		ErrCode|=STVID_ConfigureEvent(VIDHandle[0],STVID_SYNCHRONISATION_CHECK_EVT,&VID_ConfigureEventParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STVID_ConfigureEvent():**ERROR** !!! Unable to subscrive to video events !!!\n"));
		
			return TRUE;
		}
	}
#endif 



	return FALSE;
}



BOOL Yxl_A22(int VPid,int APid,int PCRPid) /*yxl 2007-01-25*/
{


	U32                           i,j;
	ST_ErrorCode_t                ErrCode=ST_NO_ERROR;
	STVID_StartParams_t           VID_StartParams;
	STVID_ConfigureEventParams_t  VID_ConfigureEventParams;
	STAUD_StartParams_t           AUD_StartParams;
	STAUD_StreamContent_t         AUD_StreamContent;
	STPTI_SlotData_t              PTI_SlotData;
	STCLKRV_SourceParams_t        CLKRV_SourceParams;
	STEVT_OpenParams_t            EVT_OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams; 
	STVTG_TimingMode_t            VTG_TimingMode;
	STVTG_ModeParams_t            VTG_ModeParams;


	AUD_StreamContent = STAUD_STREAM_CONTENT_MPEG1; 
#if 0	
	ErrCode|=VID_OpenViewPort(0,0,&VID_ViewPortHandle[0][0]);
	ErrCode|=VID_OpenViewPort(0,1,&VID_ViewPortHandle[0][1]);
#endif
/*	CH_AVSlotInit();*/



	/* Start video decoder */
	/* =================== */
	if (VPid)
	{   

		
		/* Start the video decoder */
		/* ----------------------- */
		VID_StartParams.RealTime      = TRUE;
		VID_StartParams.UpdateDisplay = TRUE;
		VID_StartParams.StreamID      = STVID_IGNORE_ID;
		VID_StartParams.DecodeOnce    = FALSE;
		VID_StartParams.StreamType    = STVID_STREAM_TYPE_PES;
		VID_StartParams.BrdCstProfile = STVID_BROADCAST_DVB;
		if (PCRPid)
		{
			ErrCode=STVID_EnableSynchronisation(VIDHandle[0]);
		}
		else
		{
			ErrCode=STVID_DisableSynchronisation(VIDHandle[0]);
		}
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		ErrCode=STVID_SetErrorRecoveryMode(VIDHandle[0],STVID_ERROR_RECOVERY_FULL);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		ErrCode=STVID_Start(VIDHandle[0],&VID_StartParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		

		ErrCode=STVID_EnableOutputWindow(VID_ViewPortHandle[0][0]);
		ErrCode=STVID_EnableOutputWindow(VID_ViewPortHandle[0][1]);
 
		
	}

	/* Start audio decoder */
	/* =================== */
	if (APid)
	{
		
		ErrCode=STAUD_DRSetSyncOffset(AUDHandle,STAUD_OBJECT_DECODER_COMPRESSED0,0);
		
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		
		/* Start the audio decoder */
		/* ----------------------- */
		AUD_StartParams.StreamContent     = AUD_StreamContent;
		AUD_StartParams.SamplingFrequency = 44100;
		AUD_StartParams.StreamID          = STAUD_IGNORE_ID;
		AUD_StartParams.StreamType        = STAUD_STREAM_TYPE_PES;
		if (PCRPid)
		{
			ErrCode=STAUD_EnableSynchronisation(AUDHandle);
		}
		else
		{
			ErrCode=STAUD_DisableSynchronisation(AUDHandle);
		}
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		ErrCode=STAUD_Mute(AUDHandle,FALSE,FALSE);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		ErrCode=STAUD_Start(AUDHandle,&AUD_StartParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		
		ErrCode=STAUD_Mute(AUDHandle,TRUE,TRUE);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
		
	}
	

	if (PCRPid)
	{
		ErrCode=STPTI_SlotSetPid(PCRSlot,PCRPid);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
	}


	if (VPid)
	{
		ErrCode=STPTI_SlotSetPid(VideoSlot,VPid);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
	}

	if (APid)
	{
		ErrCode=STPTI_SlotSetPid(AudioSlot,APid);
		if (ErrCode!=ST_NO_ERROR)
		{
			return TRUE;
		}
	}



	while(1) 
	{
		Yxl_PtiCount();
		task_delay(10000); /*yxl 2007-01-25 add*/
	}
	/* Return no errors */
	/* ================ */
	return FALSE;


}


#endif

void Yxl_PtiCount(void)
{
	ST_ErrorCode_t ErrCode;
	STPTI_ScrambleState_t StateA;
	STPTI_Pid_t Pid;
	BOOL Temp1,Temp2,Temp3;
	U16 CountTemp=0;
	U32 Value1,Value2,Value3,Value4,Value5;
				
	
	
	CountTemp=0;
				
	ErrCode=STPTI_GetInputPacketCount(PTIDeviceName,&CountTemp);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_GetInputPacketCount()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n\n\n YxlIno:CountTemp_Total=%d",CountTemp));
					printf("\n YxlIno:CountTemp_Total=%d\n",CountTemp);
				}
				
				CountTemp=0;
				
				ErrCode=STPTI_SlotPacketCount(VideoSlot,&CountTemp);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_SlotPacketCount()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n YxlIno:CountTemp_Video=%d",CountTemp));
					printf("\n YxlIno:CountTemp_Video=%d",CountTemp);
				}
				

				ErrCode=STPTI_SlotQuery(VideoSlot, &Temp1, &Temp2,&Temp3,&Pid);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_SlotQuery()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n YxlIno:CountTemp_VIDEO pid=%d State=%x %x %x",
						Pid,Temp1,Temp2,Temp3));
					printf("\n YxlIno:CountTemp_VIDEO pid=%d State=%x %x %x",
						Pid,Temp1,Temp2,Temp3);
				}

		

#ifndef YXL_NO_CLKRV				
				CountTemp=0;
				ErrCode=STPTI_SlotPacketCount(PCRSlot,&CountTemp);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_SlotPacketCount()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n YxlIno:CountTemp_PCR=%d",CountTemp));
					printf("\n  YxlIno:CountTemp_PCR=%d",CountTemp);
				}
				ErrCode=STPTI_SlotQuery(PCRSlot, &Temp1, &Temp2,&Temp3,&Pid);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_SlotQuery()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n YxlIno:CountTemp_PCR pid=%d State=%x %x %x",
						Pid,Temp1,Temp2,Temp3));
					printf("\n YxlIno:CountTemp_PCR pid=%d State=%x %x %x",
						Pid,Temp1,Temp2,Temp3);
				}
#endif 	/*YXL_NO_CLKRV		*/			
				
				CountTemp=0;
				ErrCode=STPTI_SlotPacketCount(AudioSlot,&CountTemp);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_SlotPacketCount()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n YxlIno:CountTemp_Audeo=%d",CountTemp));
					printf("\n  YxlIno:CountTemp_Audeo=%d",CountTemp);
				}
				ErrCode=STPTI_SlotQuery(AudioSlot, &Temp1, &Temp2,&Temp3,&Pid);
				if(ErrCode!=ST_NO_ERROR) 
				{
					STTBX_Print(("STPTI_SlotQuery()=%s", GetErrorText(ErrCode)));	
				}
				else 
				{
					STTBX_Print(("\n YxlIno:CountTemp_Audeo pid=%d State=%x %x %x",
						Pid,Temp1,Temp2,Temp3));
				/*	printf("\n YxlIno:CountTemp_Audeo pid=%d State=%x %x %x",
						Pid,Temp1,Temp2,Temp3);*/
					
				}
				
				
				
				
}


void Yxl_PTISetValue(int VPid,int APid,int PCRPid)
{

	ST_ErrorCode_t ErrCode;
	STPTI_ScrambleState_t StateA;
	STPTI_Pid_t Pid;
#ifndef YXL_NO_CLKRV
	ErrCode=STPTI_SlotSetPid(PCRSlot,PCRPid);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STPTI_SlotSetPid()=%s", GetErrorText(ErrCode)));	
	}
	else 
	{
		STTBX_Print(("\n\n STPTI_SlotSetPid()=ok"));
	}
#endif				
				
	ErrCode=STPTI_SlotSetPid(VideoSlot,VPid);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STPTI_SlotSetPid(VideoSlot)=%s", GetErrorText(ErrCode)));	
	}
	else 
	{
		STTBX_Print(("\n STPTI_SlotSetPid(VideoSlot)=ok"));
	}
		
	ErrCode=STPTI_SlotSetPid(AudioSlot,APid);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STPTI_SlotSetPid(AudioSlot)=%s", GetErrorText(ErrCode)));	
	}
	else 
	{
		STTBX_Print(("\n STPTI_SlotSetPid(AudioSlot)=ok"));
	}
				
	
}


void Yxl_PTITest(int VPid,int APid,int PCRPid)
{

	int Count=23;
#ifdef ST_DEMO_PLATFORM
	TUNER_SetFrequency(0,546000,0,
		STTUNER_MOD_ALL,STTUNER_FEC_ALL,STTUNER_PLR_ALL);
		Yxl_PTISetValue(VPid,APid,PCRPid);
#else
				{
					
					STTBX_Print(("\n YxlInfo:received GREEN_KEY and begin tuner\n"));
					TRANSPONDER_INFO_STRUCT InfoTemp;
					STAUD_StreamContent_t AudTypeTemp=STAUD_STREAM_CONTENT_MPEG1;

					Count=23;
					
					memset((void*)&InfoTemp,0,sizeof(TRANSPONDER_INFO_STRUCT));
					InfoTemp.abiIQInversion=1;
					
					InfoTemp.DatabaseType=CH_DVBC;				
					
					InfoTemp.iSymbolRate=6875;
					InfoTemp.ucPwmVal=0;
					InfoTemp.ucQAMMode=3;
#if 1
					InfoTemp.iTransponderFreq=413000;/*235000,*/
					CH_TuneNewFrequency(&InfoTemp);
				
					task_delay(100000);
					
					Yxl_PTISetValue(512,650,8190);/*413M,CCTV1*/
					
					
#else
					
					InfoTemp.iTransponderFreq=235000;/*235000,*/
					CH_TuneNewFrequency(&InfoTemp);	
					task_delay(100000);
					
					Yxl_PTISetValue(516,776,516);

				
					
					
#endif
					
					
				}
#endif
			
				
				while(Count)
				{
					
					task_delay(100000);
					Yxl_PtiCount();
					Count--;
				}
				
			/*	while(1)
				{
					
					task_delay(100000);
					
				}*/
				

}


