/*
Name:channelbase.c
Description:all function about tuner section
created date :2004-06-30 by yxl for changhong QAM5516 DBVC project

date 2004-11-17 by Yixiaoli
  1.Modify function  CH6_NewChannel(int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo),
  add two paramter BOOL EnableVideo,BOOL EnableAudeo)
date 2004-11-19 by Yixiaoli
  1.Modify function CH6_AVControl(),对video控制类型加以分类并做相应处理，并增加相应函数CH6_ShowBlack
  
date:2004-12-05记（2004-12-03 to 2004-12-05) by Yixiaoli
1.以另一种方式实现黑屏，并优化新黑屏函数的操作顺序
date:2004-12-06记 by Yixiaoli
1.修改CH6_AVControl(),以VIDEO窗口的大小来决定是否进行STILL层的处理

date:2005-02-25 by Yixiaoli
作下述增改,make CH6_AVControl() can control both MPEG1 and MP3 audio stream(needed by HTML) 
	  
1. add enum CH6_Audio_Type;
2.add inner variable CurAudioType;
3.add two new functions,which are void CH6_MarkAudioStreamType(CH6_Audio_Type Type) and
	CH6_Audio_Type Type CH6_GetAudioStreamType(void); 
4.Modify function CH6_AVControl(),make it can  control both MPEG1 and MP3 audio stream 
		
date:2005-08-11  by Yixiaoli
1.Modify function CH6_NewChannel(),add parameter STAUD_StreamContent_t AudType
		  
date:2005-09-21  by Yixiaoli
			
1.Rename CH6_NewChannel() to CHB_NewChannel(),and add paratemeter STVID_ViewPortHandle_t 
VPHandle and modify inner
2.Rename CH6_SetVideoWindowSize() to CHB_SetVideoWindowSize(),
and add parameter STVID_ViewPortHandle_t VPHandle and modify inner 
3.Rename CH6_AVControl() to CHB_AVControl(),and add paratemeter 
STVID_ViewPortHandle_t VPHandle and modify inner
			  
Date:2005-11-27  by Yixiaoli
1.Modify function CH6_TerminateChannel(),add	CurrentVideoPID=DEMUX_INVALID_PID;
				
Date:2005-11-29 by Yixiaoli
				  
Date:2005-12-09 by Yixiaoli
1. Modify function CHB_NewChannel(),
					
Date:2006-07-26 by Yixiaoli
1. Modify function CHB_AVControl()，增加相应部分解决在某些状态下（CVBS所有状态，576IYUV 的4：3 LETTER BOX状态）HD节目黑屏无效的问题。
					  
						
Date:2006-12-31 by Yixiaoli
1.将原来修改的无用部分delete掉
2.为了兼容7100 chip,进行下列修改
1）Modify function void CHB_NewChannel(STVID_Handle_t VHandle,STVID_ViewPortHandle_t VPHandle,int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
STAUD_StreamContent_t AudType),add a new paramter STVID_Handle_t VHandle,and modify inner section
2）Modify function BOOL CHB_AVControl(STVID_Handle_t VHandle,STVID_ViewPortHandle_t VPHandle,CH6_VideoControl_Type VideoControl,BOOL AudioControl,CH6_AVControl_Type ControlType)
add a new paramter STVID_Handle_t VHandle,and modify inner section


Date:2007-04-19 by Yixiaoli


date:2008-07-16 by Yixiaoli
1.修改函数CHB_SetVideoWindowSize()的原型，将输入参数由unsigned short PosX,unsigned short PosY改为
int PosX,int PosY,because ST surport 负数,mainly for DTG test

Date:2008-10-15 by yixiaoli
1.Modify function CH_SetDigitalAudioOut(),对其进行优化.
2.Add new function char* CHDRV_CHANNELBASE_GetVision(void),用于查询CHANNLE底层的版本，
 目前版本为：CHDRV_CHB_7101R3.1.1.

*/ 


#include "..\main\initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "..\dbase\vdbase.h"
#include "channelbase.h"
#include "..\main\errors.h"
#include "..\usif\graphicmid.h"


static int CurrentVideoPID=DEMUX_INVALID_PID;/*yxl 2004-11-22 add this line*/


#if 0 /*yxl 2005-11-29 modify this line*/
static CH6_Audio_Type CurAudioType=AUDIO_MPEG1;/*yxl 2005-02-25 add this variable CurAudioType*/ 
#else
static STAUD_StreamContent_t CurAudioType=AUDIO_MPEG1;

#endif/*end yxl 2005-11-29 modify this line*/

void CH6_MarkAudioStreamType(STAUD_StreamContent_t Type);
STAUD_StreamContent_t CH6_GetAudioStreamType(void);



/*yxl 2008-10-15 add below function*/
char* CHDRV_CHANNELBASE_GetVision(void)
{
	return "CHDRV_CHB_7101R3.1.1";/*yxl 2008-10-15 add this line*/

}
/*end yxl 2008-10-15 add below function*/




/*yxl 2007-04-19 modify below section for 兼容7100 h264,mpeg2*/
void CHB_NewChannel(CH_SingleVIDCELL_t CHOVIDCell,STAUD_Handle_t AHandle,
					STPTI_Slot_t VSlot,STPTI_Slot_t ASlot,STPTI_Slot_t PSlot,
					int VideoPid,int AudeoPid,int PCRPid,
					BOOL EnableVideo,BOOL EnableAudeo,STAUD_StreamContent_t AudType)
					
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STVID_StartParams_t VIDStartParams;
    STAUD_StartParams_t AUDStartParams;

	STVID_Handle_t VHandleTemp;
	STVID_ViewPortHandle_t  VPHandleTemp;
	CH_SingleVIDCELL_t   VIDOCellTemp;

	STPTI_Slot_t VSlotTemp=VSlot;
	STPTI_Slot_t ASlotTemp=ASlot;
	STPTI_Slot_t PSlotTemp=PSlot;

	int i;


	STAUD_Handle_t AHandleTemp=AHandle;

	memcpy((void*)&VIDOCellTemp,(const void*)&CHOVIDCell,sizeof(CH_SingleVIDCELL_t));
	
/*	STTBX_Print(("\n YxlInfo():VPid=%d AVid=%d PVid=%d",VideoPid,AudeoPid,PCRPid));*/
	
/*	CH6_TerminateChannel();*/

	
	if(VideoPid!=DEMUX_INVALID_PID&&EnableVideo)
	{
		VHandleTemp=VIDOCellTemp.VideoHandle;
		if(VHandleTemp!=0) 
		{
#if 1 /*this is should to do*/ 
			if(PCRPid!=DEMUX_INVALID_PID/*PCRPid!=DEMUX_INVALID_PID,yxl 2007-06-27 temp */)
#else
			if(PCRPid==DEMUX_INVALID_PID/*PCRPid!=DEMUX_INVALID_PID,yxl 2007-06-27 temp */)
#endif
			{
				ErrCode = STVID_EnableSynchronisation(VHandleTemp);
				if ( ErrCode != ST_NO_ERROR )
				{
					STTBX_Print(("STVID_EnableSyncronisation()=%s\n", GetErrorText(ErrCode) ));
				}
			}
			else
			{
				
				ErrCode = STVID_DisableSynchronisation(VHandleTemp);
				if ( ErrCode != ST_NO_ERROR )
				{
					STTBX_Print(("STVID_EnableSyncronisation()=%s\n", GetErrorText(ErrCode) ));
				}
			}
			
#if 0 /*yxl 2007-07-11 move this section to other place initterm.c*/			
#if 1 
			ErrCode=STVID_SetErrorRecoveryMode(VHandleTemp,STVID_ERROR_RECOVERY_FULL);
#else
			ErrCode=STVID_SetErrorRecoveryMode(VHandleTemp,STVID_ERROR_RECOVERY_NONE);

#endif
#endif/*end yxl 2007-07-11 move this section to other place initterm.c*/

			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STVID_SetErrorRecoveryMode()=%s\n", GetErrorText(ErrCode) ));
			}
			VIDStartParams.RealTime      = TRUE;
			VIDStartParams.UpdateDisplay = TRUE;
			VIDStartParams.StreamID      = STVID_IGNORE_ID;
			VIDStartParams.DecodeOnce    = FALSE;
			VIDStartParams.StreamType    = STVID_STREAM_TYPE_PES;
			VIDStartParams.BrdCstProfile = STVID_BROADCAST_DVB;
			ErrCode = STVID_Start(VHandleTemp, &VIDStartParams);/*yxl 2007-06-11 temp cancle*/
			if(ErrCode !=ST_NO_ERROR)
			{
				STTBX_Print(("STVID_Start()=%s\n", GetErrorText(ErrCode) ));
			}
		}
	}
	else
	{
		
	}
	
	/*audio process*/

	if(AudeoPid!=DEMUX_INVALID_PID&&EnableAudeo)
	{

#if 1	/*yxl 2007-07-06 temp modify below */	
		ErrCode=STAUD_DRSetSyncOffset(AHandleTemp,STAUD_OBJECT_DECODER_COMPRESSED0,0);
		if ( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STAUD_DRSetSyncOffset()=%s\n", GetErrorText(ErrCode) ));
		}
#else
        ErrCode=STAUD_DRSetSyncOffset(AHandleTemp, STAUD_OBJECT_OUTPUT_PCMP1,0);

#endif
		
		
		ErrCode = STAUD_Mute(AHandleTemp, TRUE, TRUE);
		if ( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STAUD_Mute(FALSE)=%s\n", GetErrorText(ErrCode) ));
		}

		AUDStartParams.StreamType = STAUD_STREAM_TYPE_PES;
		AUDStartParams.StreamID = STAUD_IGNORE_ID;
#if 0 /*yxl 2007-07-02 temp modify below*/
		AUDStartParams.SamplingFrequency = 32000;/*44100;*/		
#else
		AUDStartParams.SamplingFrequency = 48000;
#endif
		AUDStartParams.StreamContent =AudType;
#if 1		
		if(AudType==STAUD_STREAM_CONTENT_AC3)
		{
			AUDStartParams.SamplingFrequency = 48000; /*yxl 2006-04-20 modify 96000 to 48000,,yxl 2006-06-12 add 0*/
		}else if(AudType==STAUD_STREAM_CONTENT_MP3)
		{
			AUDStartParams.StreamType = STAUD_STREAM_TYPE_ES;
		}
		
		CH6_MarkAudioStreamType(AudType);	/*yxl 2005-11-29 add this line*/
#endif 
#if 1	/*yxl 2007-07-06 temp modify for test,this is org*/	
		ErrCode = STAUD_Start(AHandleTemp, &AUDStartParams);
#else
		ErrCode = STAUD_DRStart(AHandleTemp,STAUD_OBJECT_DECODER_COMPRESSED0, 
			&AUDStartParams);
#endif
		if(ErrCode!=ST_NO_ERROR )
		{
			STTBX_Print(("STAUD_Start()=%s\n", GetErrorText(ErrCode) ));
		}
	/*	else STTBX_Print(("STAUD_Start()=%s\n", GetErrorText(ErrCode) ));*/
#if 1 /*this is should to do*/ 		
		if(PCRPid!=DEMUX_INVALID_PID/*=*/)
#else
		if(PCRPid==DEMUX_INVALID_PID/*==*/)
#endif
		{
			
#if 1 /*yxl 2007-07-06 temp modify below section*/
			ErrCode = STAUD_EnableSynchronisation(AHandleTemp);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STAUD_EnableSyncronisation()=%s\n", GetErrorText(ErrCode) ));
			}
#else
			ErrCode = STAUD_OPEnableSynchronization(AHandleTemp, 
				STAUD_OBJECT_OUTPUT_PCMP1);

			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STAUD_OPEnableSynchronization()=%s\n", GetErrorText(ErrCode) ));
			}
#endif


		}
		else
		{
			
#if 1 /*yxl 2007-07-06 temp modify below section*/			
			ErrCode = STAUD_DisableSynchronisation(AHandleTemp);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STAUD_EnableSyncronisation()=%s\n", GetErrorText(ErrCode) ));
			}
#else
            ErrCode = STAUD_OPDisableSynchronization(AHandleTemp, STAUD_OBJECT_OUTPUT_PCMP1);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STAUD_OPDisableSynchronization()=%s\n", GetErrorText(ErrCode) ));
			}
#endif

			
		}
		ErrCode = STAUD_Mute(AHandleTemp, FALSE, FALSE);
		if ( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STAUD_Mute(FALSE)=%s\n", GetErrorText(ErrCode) ));
		}
    }

	if(PCRPid!=DEMUX_INVALID_PID)
	{
		
		ErrCode = STCLKRV_InvDecodeClk(CLKRVHandle);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STCLKRV_InvDecodeClk()=%s\n", GetErrorText(ErrCode) ));
		}

		ErrCode = STPTI_SlotSetPid(PSlotTemp, PCRPid); 
		if ( ErrCode != ST_NO_ERROR )
		{
			/*STTBX_Print(("STPTI_SlotSetPid(PCRSlot)=%s\n", GetErrorText(ErrCode) ));*/
			
			
		}
	}

	if(VideoPid!=DEMUX_INVALID_PID)
	{
		ErrCode = STPTI_SlotSetPid(VSlotTemp, VideoPid);
		if ( ErrCode != ST_NO_ERROR )
		{
			/*STTBX_Print(("STPTI_SlotSetPid(VideoSlot)=%s\n", GetErrorText(ErrCode) ));*/
		}
		else CurrentVideoPID=VideoPid;/*yxl 2004-11-22 add this line*/
	}

	if(AudeoPid!=DEMUX_INVALID_PID)
	{
		ErrCode = STPTI_SlotSetPid(ASlotTemp, AudeoPid);
		if ( ErrCode != ST_NO_ERROR )
		{
		/*	STTBX_Print(("STPTI_SlotSetPid(AudioSlot)=%s\n", GetErrorText(ErrCode) ));*/
		}
	}
	
	if(VideoPid!=DEMUX_INVALID_PID&&EnableVideo)
	{
		if(VHandleTemp!=0)
		{ 
			for(i=0;i<VIDOCellTemp.VPortCount;i++)
			{
				VPHandleTemp=VIDOCellTemp.VPortHandle[i];
				if(VPHandleTemp==0) continue;
				ErrCode = STVID_EnableOutputWindow(VPHandleTemp);
				if ( ErrCode != ST_NO_ERROR )
				{
					STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
				}
			}
		}
	}

    return;
}



void CHB_TerminateChannel(CH_VIDCELL_t CHVIDCell,STAUD_Handle_t AHandle,
						  STPTI_Slot_t VSlot,STPTI_Slot_t ASlot,STPTI_Slot_t PSlot)
{
	
	ST_ErrorCode_t ErrCode;
    STVID_Freeze_t Freeze;
    STAUD_Fade_t   Fade;
	int i;

	CH_VIDCELL_t   VIDCellTemp;

	STVID_Handle_t VHandleTemp;

	memcpy((void*)&VIDCellTemp,(const void*)&CHVIDCell,sizeof(CH_VIDCELL_t));

	if(VIDCellTemp.VIDDecoderCount==0) return ;

	Freeze.Field = STVID_FREEZE_FIELD_TOP;
    Freeze.Mode = STVID_FREEZE_MODE_FORCE;
	for(i=0;i<VIDCellTemp.VIDDecoderCount;i++)
	{
		VHandleTemp=VIDCellTemp.VIDCell[i].VideoHandle;	
		if(VHandleTemp==0) continue;
		ErrCode=STVID_Stop(VHandleTemp,STVID_STOP_NOW,&Freeze);
		if(ErrCode!=ST_NO_ERROR)
		{
		/*	STTBX_Print(("STVID_Stop()=%s",GetErrorText(ErrCode)));yxl 2007-12-09 cancel*/
		}
	}

#if 1 /*yxl 2007-02-23 temp cancel for 7100debug*/
	Fade.FadeType=STAUD_FADE_SOFT_MUTE;
	ErrCode=STAUD_Stop(AHandle,STAUD_STOP_NOW,&Fade);
	if(ErrCode!=ST_NO_ERROR/*&&ErrCode!=STAUD_ERROR_DECODER_STOPPED*/)
	{
	/*	STTBX_Print(("STAUD_Stop()=%s",GetErrorText(ErrCode))); yxl 2007-12-09 cancel*/
	}

#endif 

#if 1
	ErrCode = STPTI_SlotClearPid(VSlot);
	if(ErrCode!=ST_NO_ERROR)
	{
	    STTBX_Print(("STPTI_SlotClearPid(VSlot)=%s\n", GetErrorText(ErrCode) ));	
	}
	CurrentVideoPID=DEMUX_INVALID_PID;/*yxl 2005-11-27 add this line*/


	ErrCode = STPTI_SlotClearPid(ASlot);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotClearPid(ASlot)=%s\n", GetErrorText(ErrCode) ));	
	}

	ErrCode = STPTI_SlotClearPid(PSlot);
	if(ErrCode!=ST_NO_ERROR)
	{
	    STTBX_Print(("STPTI_SlotClearPid(PSlot)=%s\n", GetErrorText(ErrCode) ));	
	}
#endif

}




BOOL  CHB_SetVideoWindowSize(STVID_ViewPortHandle_t VPHandle,BOOL Small,int PosX,int PosY,
						 unsigned short Width,unsigned short Height)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STVID_WindowParams_t WinParams = {STVID_WIN_ALIGN_TOP_HCENTRE,STVID_WIN_SIZE_DONT_CARE};
    STGXOBJ_Rectangle_t OutputRectangle;
    BOOL AutoMode;

    /* Scale video window */
    if ( Small )
    {
        AutoMode = FALSE;
        OutputRectangle.PositionX = PosX;
        OutputRectangle.PositionY = PosY;
        OutputRectangle.Width     = Width;
        OutputRectangle.Height    = Height;
 
    }
    else
    {
        AutoMode =TRUE ;
        OutputRectangle.PositionX = 0;
        OutputRectangle.PositionY = 0;
        OutputRectangle.Width     = 0;
        OutputRectangle.Height    = 0;
 
    }

    ErrCode = STVID_SetOutputWindowMode(VPHandle, AutoMode, &WinParams);
    if ( ErrCode != ST_NO_ERROR )
	{
        STTBX_Print(("STVID_SetOutputWindowMode()=%s\n", GetErrorText(ErrCode)));
		return TRUE;
	}
    ErrCode = STVID_SetIOWindows(VPHandle, 0, 0, 0, 0,
                                 OutputRectangle.PositionX,
                                 OutputRectangle.PositionY,
                                 OutputRectangle.Width,
                                 OutputRectangle.Height);

    if ( ErrCode != ST_NO_ERROR )
	{
        STTBX_Print(("STVID_SetIOWindows()=%s\n", GetErrorText(ErrCode)));
		return TRUE;	
	}

    return FALSE;
	
}


BOOL CHB_AVControl(CH_VIDCELL_t CHVIDCell,STAUD_Handle_t AHandle,
				   CH6_VideoControl_Type VideoControl,BOOL AudioControl,
				   CH6_AVControl_Type ControlType)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	
	BOOL VideoTemp=FALSE;
	BOOL AudioTemp=FALSE;
	
	CH_VIDCELL_t VIDCellTemp;
	int i;
	int j;

	memcpy((void*)&VIDCellTemp,(const void*)&CHVIDCell,sizeof(CH_VIDCELL_t));

	if(VIDCellTemp.VIDDecoderCount==0)
	{
		STTBX_Print(("\nYxlInfo:Video Decoder is zero"));
		return TRUE;
	}

	switch(ControlType)
	{
	case CONTROL_VIDEO:
		VideoTemp=TRUE;
		break;
	case CONTROL_AUDIO:
		AudioTemp=TRUE;
		break;
	case CONTROL_VIDEOANDAUDIO:
		VideoTemp=TRUE;
		AudioTemp=TRUE;
		break;
	default:
		return TRUE;
	}
	if(VideoTemp)
	{
		STVID_Handle_t VHandleTemp;
		STVID_ViewPortHandle_t  VPHandleTemp;

		switch(VideoControl)
		{
		case VIDEO_DISPLAY:
			{
				STVID_StartParams_t VIDStartParams;
				
				if(CurrentVideoPID!=DEMUX_INVALID_PID)
				{
					ErrCode=STPTI_SlotSetPid(VideoSlot,CurrentVideoPID);
					if(ErrCode!=ST_NO_ERROR&&ErrCode!=STPTI_ERROR_PID_ALREADY_COLLECTED)
					{
						STTBX_Print(("STPTI_SlotSetPid()=%s",GetErrorText(ErrCode)));
					}				
				}	
			
				VIDStartParams.RealTime      = TRUE;
				VIDStartParams.UpdateDisplay = TRUE;
				VIDStartParams.StreamID      = STVID_IGNORE_ID;
				VIDStartParams.DecodeOnce    = FALSE;
				VIDStartParams.StreamType    = STVID_STREAM_TYPE_PES;
				VIDStartParams.BrdCstProfile = STVID_BROADCAST_DVB;
				
				for(i=0;i<VIDCellTemp.VIDDecoderCount;i++)
				{
					VHandleTemp=VIDCellTemp.VIDCell[i].VideoHandle;
					if(VHandleTemp==0) continue;
					ErrCode = STVID_Start(VHandleTemp, &VIDStartParams);
					if ( ErrCode != ST_NO_ERROR )
					{
					/*	STTBX_Print(("STVID_Start()=%s\n", GetErrorText(ErrCode) ));*/
					}
					for(j=0;j<VIDCellTemp.VIDCell[i].VPortCount;j++)
					{
						VPHandleTemp=VIDCellTemp.VIDCell[i].VPortHandle[j];
						if(VPHandleTemp==0) continue;
						ErrCode = STVID_EnableOutputWindow(VPHandleTemp);
						if ( ErrCode != ST_NO_ERROR )
						{
							STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
						}
					}
				}
			}
			
			break;
		case VIDEO_BLACK:
		case VIDEO_CLOSE:
			{
				
				STVID_Freeze_t          Freeze;
				STVID_PictureParams_t PicParams;
				
				ST_ErrorCode_t ErrCode;
				
				
				BOOL WrongSign=FALSE;/*yxl 2004-12-04 add this line*/
				
				
				Freeze.Field = STVID_FREEZE_FIELD_TOP;
				Freeze.Mode = STVID_FREEZE_MODE_NONE;

				for(i=0;i<VIDCellTemp.VIDDecoderCount;i++)
				{
					VHandleTemp=VIDCellTemp.VIDCell[i].VideoHandle;
					if(VHandleTemp==0) continue;

					for(j=0;j<VIDCellTemp.VIDCell[i].VPortCount;j++)
					{
						VPHandleTemp=VIDCellTemp.VIDCell[i].VPortHandle[j];
						if(VPHandleTemp==0) continue;
						
						ErrCode = STVID_DisableOutputWindow(VPHandleTemp);
						if ( ErrCode != ST_NO_ERROR )
						{
							STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
						}
					}
					
					
					ErrCode=STVID_Stop(VHandleTemp,STVID_STOP_NOW,&Freeze);
					if(ErrCode!=ST_NO_ERROR&&ErrCode!=STVID_ERROR_DECODER_STOPPED)
					{
						STTBX_Print(("STVID_Stop()=%s",GetErrorText(ErrCode)));
					}
					
					
					ErrCode=STVID_GetPictureParams(VHandleTemp,STVID_PICTURE_DISPLAYED,&PicParams);
					if(ErrCode!=ST_NO_ERROR)
					{
						
						WrongSign=TRUE;
						
					}
					if(WrongSign==FALSE)/*yxl 2004-12-04 add this line*/
					{/*yxl 2004-12-04 add this line*/
						/*	task_lock(); yxl 2007-01-16 cancel this line*/
						memset((void*)((U8*)PicParams.Data+PicParams.pChromaOffset),0x80,
							PicParams.Size-PicParams.pChromaOffset);
						memset((void*)PicParams.Data,0,PicParams.pChromaOffset);	
						
						/*	task_unlock(); yxl 2007-01-16 cancel this line*/
					}/*yxl 2004-12-04 add this line*/
					
#if 1 /*yxl 2006-07-26 add below section for解决在某些状态下（CVBS所有状态，576IYUV 的4：3 LETTER BOX状态）HD节目黑屏无效的问题*/
					
					{
						
						STVID_ClearParams_t VidClearParams;
						VidClearParams.ClearMode = STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER;
						
						ErrCode=STVID_Clear( VHandleTemp, &VidClearParams );
						if ( ErrCode != ST_NO_ERROR )
						{
							STTBX_Print(("STVID_Clear()=%s\n", GetErrorText(ErrCode) ));
						}
						
					}
#endif /*end yxl 2006-07-26 add below section for解决在某些状态下（CVBS所有状态，576IYUV 的4：3 LETTER BOX状态）HD节目黑屏无效的问题*/
					
					if(VideoControl==VIDEO_BLACK) /*yxl 2004-12-04 add this line*/
					{	
						for(j=0;j<VIDCellTemp.VIDCell[i].VPortCount;j++)
						{
							VPHandleTemp=VIDCellTemp.VIDCell[i].VPortHandle[j];
							if(VPHandleTemp==0) continue;			
							ErrCode = STVID_EnableOutputWindow(VPHandleTemp);
							if ( ErrCode != ST_NO_ERROR )
							{
								STTBX_Print(("STVID_EnableOutputindow(AAA)=%s\n", GetErrorText(ErrCode) ));
							}
						}
						
					}/*yxl 2004-12-04 add this line*/
				}
			}
			break;
#if 0 /*yxl 2007-04-19 cancel below section*/
		case VIDEO_OPEN:
			ErrCode = STVID_EnableOutputWindow(VPHandle);
			if ( ErrCode != ST_NO_ERROR )
			{
				STTBX_Print(("STVID_EnableOutputindow()=%s\n", GetErrorText(ErrCode) ));
			}
			break;
#endif /*end yxl 2007-04-19 cancel below section*/
			/*yxl 2004-11-22 ADD this section*/
		case VIDEO_FREEZE:
			{
				STVID_Freeze_t          Freeze;
				Freeze.Field = STVID_FREEZE_FIELD_TOP;
				Freeze.Mode = STVID_FREEZE_MODE_NONE;
				for(i=0;i<VIDCellTemp.VIDDecoderCount;i++)
				{
					VHandleTemp=VIDCellTemp.VIDCell[i].VideoHandle;
					if(VHandleTemp==0) continue;
					ErrCode=STVID_Stop(VHandleTemp,STVID_STOP_NOW,&Freeze);
					if(ErrCode!=ST_NO_ERROR&&ErrCode!=STVID_ERROR_DECODER_STOPPED)
					{
						STTBX_Print(("STVID_Stop()=%s",GetErrorText(ErrCode)));
					}
				}
				
				
			}
			break;
			
			/*end yxl 2004-11-22 ADD this section*/
		default:
			break;
		}
	}
	if(AudioTemp)
	{
		STAUD_Handle_t AHandleTemp=AHandle;

		if(AudioControl)
		{
			STAUD_StartParams_t AUDStartParams;

#if 0 /*yxl 2005-11-29 modfiy below section*/
			CH6_Audio_Type ATypeTemp;
			AUDStartParams.SamplingFrequency = 44100;
			AUDStartParams.StreamID = STAUD_IGNORE_ID;
			ATypeTemp=CH6_GetAudioStreamType();
			if(ATypeTemp==AUDIO_MPEG1)
			{
				AUDStartParams.StreamContent     = STAUD_STREAM_CONTENT_MPEG1;
				AUDStartParams.StreamType = STAUD_STREAM_TYPE_PES;				
			}
			if(ATypeTemp==AUDIO_MP3)
			{
				AUDStartParams.StreamContent     = STAUD_STREAM_CONTENT_MP3;
				AUDStartParams.StreamType = STAUD_STREAM_TYPE_ES;				
			}

#else
			 STAUD_StreamContent_t streamtype;
			 streamtype=CH6_GetAudioStreamType();
			 AUDStartParams.StreamContent        = streamtype;
			 AUDStartParams.StreamType           = STAUD_STREAM_TYPE_PES;
			 AUDStartParams.SamplingFrequency    = 44100;
			 AUDStartParams.StreamID             = STAUD_IGNORE_ID;
			 if(streamtype==STAUD_STREAM_CONTENT_AC3)
			 {
				 AUDStartParams.SamplingFrequency = 48000; /*yxl 2006-04-20 modify 96000 to 48000,yxl 2006-06-12 add 0*/
			 }else if(streamtype==STAUD_STREAM_CONTENT_MP3)
			 {
				 AUDStartParams.StreamType = STAUD_STREAM_TYPE_ES;
			 }

#endif/*end yxl 2005-11-29 modfiy below section*/
			
			ErrCode = STAUD_Start(AHandleTemp, &AUDStartParams);
			if(ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("STAUD_Start()=%s",GetErrorText(ErrCode)));
			}
		}
		else
		{
			STAUD_Fade_t            Fade;
			Fade.FadeType=STAUD_FADE_SOFT_MUTE;/*yxl 2008-04-25 add this line*/
			ErrCode=STAUD_Stop(AHandleTemp,STAUD_STOP_NOW,&Fade);
			if(ErrCode!=ST_NO_ERROR&&ErrCode!=STAUD_ERROR_DECODER_STOPPED)
			{
			/*	STTBX_Print(("STAUD_Stop()=%s",GetErrorText(ErrCode)));yxl 2008-04-24 cancel*/
			}
		}
		
	}

	return FALSE;

}



void CH6_MarkAudioStreamType(STAUD_StreamContent_t Type)
{
	CurAudioType=Type;
}


STAUD_StreamContent_t CH6_GetAudioStreamType(void)
{
	return CurAudioType;
}

/*yxl 2005-02-25 add below function*/
BOOL CH6_EnableSPDIFOutput(void)
{
	ST_ErrorCode_t ErrCode;
	STAUD_DigitalOutputConfiguration_t DigitalOutput;
	DigitalOutput.DigitalMode=STAUD_DIGITAL_MODE_NONCOMPRESSED;/*STAUD_DIGITAL_MODE_COMPRESSED;*/
	DigitalOutput.Copyright=STAUD_COPYRIGHT_MODE_NO_RESTRICTION;
	DigitalOutput.Latency=0;
	
	ErrCode=STAUD_SetDigitalOutput(AUDHandle,DigitalOutput);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAUD_SetDigitalOutput()=%s\n", GetErrorText(ErrCode) ));
		return TRUE;
	}    
	return FALSE;

}
/*end yxl 2005-02-25 add below function*/


BOOL CH_SetDigitalAudioOut(STAUD_StreamContent_t AudStreamType)
{
	ST_ErrorCode_t ErrCode;
/*	STAUD_StreamContent_t AudTypeTemp=STAUD_STREAM_CONTENT_MPEG1;*/
	/*	STAUD_Object_t AUDInputSource; yxl 2008-10-15 cancel this line*/
	STAUD_DigitalOutputConfiguration_t DOutputConfig;
	
	/*yxl 2008-10-15 add below section*/
	BOOL SignTemp=FALSE;
	
	ErrCode=STAUD_GetDigitalOutput(AUDHandle,&DOutputConfig);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAUD_GetDigitalOutput()=%s\n", GetErrorText(ErrCode) ));
		SignTemp=TRUE;
	}    
	/*end yxl 2008-10-15 add below section*/
	
	if(AudStreamType==STAUD_STREAM_CONTENT_AC3)
	{
		
		/*yxl 2008-10-15 add below section*/
		if(DOutputConfig.DigitalMode==STAUD_DIGITAL_MODE_COMPRESSED&&SignTemp==FALSE) 
		{
			return FALSE;
		}
		/*end yxl 2008-10-15 add below section*/
		
		DOutputConfig.DigitalMode=STAUD_DIGITAL_MODE_COMPRESSED;
	}
	else 
	{
		/*yxl 2008-10-15 add below section*/
		if(DOutputConfig.DigitalMode==STAUD_DIGITAL_MODE_NONCOMPRESSED&&SignTemp==FALSE) 
		{
			return FALSE;
		}
		/*end yxl 2008-10-15 add below section*/
		
		DOutputConfig.DigitalMode=STAUD_DIGITAL_MODE_NONCOMPRESSED;		
	}
	
	
	
	DOutputConfig.Copyright=STAUD_COPYRIGHT_MODE_NO_RESTRICTION;
	DOutputConfig.Latency=0;
	
	
	ErrCode=STAUD_SetDigitalOutput(AUDHandle,DOutputConfig);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STAUD_SetDigitalOutput()=%s\n", GetErrorText(ErrCode) ));
		return TRUE;
	}    
	
	return FALSE; 

}




/*END of file*/



