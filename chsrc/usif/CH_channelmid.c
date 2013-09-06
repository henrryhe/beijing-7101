/*
(c) Copyright changhong
  Name:CH_channelmid.c
  Description:实现和视频控制相关的中间层接口函数，既在视频控制底层和应用间建立桥梁。
  Authors:yixiaoli
  Date          Remark
  2007-02-21    Created

Modifiction:
Date:2007-03-05 by yixiaoli
1.Modify function BOOL  CH6_SetVideoWindowSize(BOOL Small,unsigned short PosX,unsigned short PosY,
						 unsigned short Width,unsigned short Height );

Date:2007-04-19 by yixiaoli
为了实现MPEG2,H264同时且自动检测解码,对下列视频控制中间层函数进行结构化改动 
1.Modify function BOOL CH6_AVControl()
2.Modify function 
3.Add new function CH6_TerminateChannel(void),将原底层的CH6_TerminateChannel(void)修改为
  void CHB_TerminateChannel(CH_VIDCELL_t CHVIDCell,STAUD_Handle_t AHandle,
						  STPTI_Slot_t VSlot,STPTI_Slot_t ASlot,STPTI_Slot_t PSlot);

Date:2007-04-29 by yixiaoli
1.Add new interface function BOOL CH_DisplayIFrame(char *PICFileName)

Date:2007-04-30 by yixiaoli
1.Modify function CH6_SetVideoWindowSize(),使之能兼容MPEG2和H.264的小窗口.
2.Modify function void CH66_NewChannel(int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType,BYTE VidType),使之能自动实现全屏和小窗口播放模式。


Date:yxl 2007-07-03 by yixiaoli  
1.Modify function CH_DisplayIFrame(),add new two parameters U32 Loops and U32 Priority,
主要用于由应用控制I帧显示的循环次数和优先级，保证I帧可靠显示

Date:yxl 2007-07-04 by yixiaoli  
1.Add new function CH_DisplayIFrameFromMemory(),
主要用于从内存显示I帧

2007-07-18 增加记录上一个节目的视频流类型
2007-08~2007-09 修改满足歌华需求
2007-11   增加一个AUDIO控制状态，记录上次AUdio的控制状态，每次换台时设置为初始状态*/


#include "stdlib.h"
#include "..\main\initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "channelbase.h"
#include "..\dbase\vdbase.h"
#include "..\usif\CH_channelmid.h"
#include "graphicmid.h"

#define CH_FOR_ONLY_AUDO_OR_VIEDO/*zxg20090628 增加单独控音视频控制接*/

extern semaphore_t *pst_AVControlSema;

#ifdef CH_FOR_ONLY_AUDO_OR_VIEDO
static int  gi_OnlyAudioPid = 8191;
static int  gi_OnlyPcrPid    = 8191;
static int  gi_OnlyVidPid     = 8191;
#endif
void CH_SWitchVidDecoder(void);

/*20071130 add 记录上一次audio控制动作*/
boolean g_LastAudioControl = false;
/*endif 20071130*/



static VID_Device gch_LastViedoDevice=VID_UNKNOW;/*记录上一个节目的视频流类型*/

static  int gi_VidPid = 8191;  /*记录上次播放的视频PID*/
/*函数申明*/
void CH_SHowBackpic(void);/*显示广播背景I贞图片*/
boolean CH_HDVideoType( void );
void CH_NewSubFrame( VID_Device ch_Lastvidtype ,VID_Device ch_Curvidtype);

void CH6_NewChannel(int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType,BYTE VidType);
boolean CH_VideoFramChange( void );
void CH_InitVideofram( void );

int CH_GetBackIFramData(void);
boolean CH_VideoIsBackIFramData();

/*yxl 2007-04-20 add below section*/
static STGXOBJ_Rectangle_t stVPInputRect={0,0,0,0};
/*end yxl 2007-04-20 add below section*/ 

BOOL  CH6_SetVideoWindowSize(BOOL Small,unsigned short PosX,unsigned short PosY,
						 unsigned short Width,unsigned short Height );
BOOL CH6_AVControl(CH6_VideoControl_Type VideoControl,BOOL AudioControl,CH6_AVControl_Type ControlType)
{
	CH_VIDCELL_t VIDCellTemp;
	BOOL   success;
	VID_Device   last_VidCode;
	static CH6_VideoControl_Type ch_LastVideoControlType = VIDEO_UNKNOWN;
	last_VidCode = gch_LastViedoDevice;

 	if(pst_AVControlSema!=NULL)
	{
		semaphore_wait(pst_AVControlSema);/*20070717 add*/ 
		
		if( CONTROL_AUDIO == ControlType)/* wz add 20071121*/
		{
			/*20080612 add*/
			memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));
			VIDCellTemp.VIDDecoderCount=2;

			VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
			VIDCellTemp.VIDCell[0].VPortCount=2;
			VIDCellTemp.VIDCell[0].VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
			VIDCellTemp.VIDCell[0].VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];

			VIDCellTemp.VIDCell[1].VideoHandle=VIDHandle[VID_H];
			VIDCellTemp.VIDCell[1].VPortCount=2;
			VIDCellTemp.VIDCell[1].VPortHandle[0]=VIDVPHandle[VID_H][LAYER_VIDEO1];
			VIDCellTemp.VIDCell[1].VPortHandle[1]=VIDVPHandle[VID_H][LAYER_VIDEO2];
			/**************/  
			success = CHB_AVControl(VIDCellTemp,AUDHandle,VideoControl,AudioControl,ControlType);
		}
		else
		{
			memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));
			VIDCellTemp.VIDDecoderCount=2;

			VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
			VIDCellTemp.VIDCell[0].VPortCount=2;
			VIDCellTemp.VIDCell[0].VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
			VIDCellTemp.VIDCell[0].VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];

			VIDCellTemp.VIDCell[1].VideoHandle=VIDHandle[VID_H];
			VIDCellTemp.VIDCell[1].VPortCount=2;
			VIDCellTemp.VIDCell[1].VPortHandle[0]=VIDVPHandle[VID_H][LAYER_VIDEO1];
			VIDCellTemp.VIDCell[1].VPortHandle[1]=VIDVPHandle[VID_H][LAYER_VIDEO2];

			if(VideoControl ==  VIDEO_DISPLAY && 
			(ch_LastVideoControlType == VIDEO_SHOWPIC || ch_LastVideoControlType == VIDEO_SHOWPIC_AUTORECOVERY ))
			{
			/*如果上次是VIDEO_SHOWPIC或者VIDEO_SHOWPIC_AUTORECOVERY，需要把I贞清除，并且恢复视频*/

				if(gi_OnlyVidPid!= DEMUX_INVALID_PID)
				{
					if(gch_LastViedoDevice == VID_MPEG2)
					{
						CH_VideoDecoderControl(VID_H,FALSE);  
						CH_VideoDecoderControl(VID_MPEG2,TRUE);
					}
					else
					{
						CH_VideoDecoderControl(VID_MPEG2,FALSE);
						CH_VideoDecoderControl(VID_H,TRUE);  	  
					}     
				}
			}
			if(VideoControl == VIDEO_SHOWPIC ||VideoControl ==  VIDEO_SHOWPIC_AUTORECOVERY)
			{

				/*首先显示I FRAM*/
				if( EIS_GetOtherIFramStaus() ||  VideoControl == VIDEO_SHOWPIC_AUTORECOVERY)/*显示过其他I-FRAM需要清楚一下*/
				{
					success = CHB_AVControl(VIDCellTemp,AUDHandle,VIDEO_BLACK/*VIDEO_BLACK*/,AudioControl,ControlType);
					CH_SHowBackpic();/*显示I贞背景图片*/
				}
				else
				{
					if(CH_VideoIsBackIFramData() == false)/*是否上次已经显示过*/
					{
						success = CHB_AVControl(VIDCellTemp,AUDHandle,VIDEO_BLACK/*VIDEO_BLACK*/,AudioControl,ControlType);
				}
				else
			       success = CHB_AVControl(VIDCellTemp,AUDHandle,VIDEO_FREEZE,AudioControl,ControlType);

				CH_SHowBackpic();/*显示I贞背景图片*/
			}

			/*如果是自动恢复，需要触发自动回复*/
			if(VideoControl ==  VIDEO_SHOWPIC_AUTORECOVERY)
			{
				if(last_VidCode == VID_MPEG2 && gch_LastViedoDevice == VID_MPEG2)
				{
					CH_VideoDecoderControl(VID_H,FALSE);  
					CH_VideoDecoderControl(VID_MPEG2,TRUE);
				}
				else if(last_VidCode == VID_H && gch_LastViedoDevice == VID_MPEG2)
				{
					//CH_SWitchVidDecoder();
					CH_VideoDecoderControl(VID_MPEG2,FALSE);
					CH_VideoDecoderControl(VID_H,TRUE);  
				}
				else
				{
					CH_VideoDecoderControl(VID_MPEG2,FALSE);
					CH_VideoDecoderControl(VID_H,TRUE);  
				} 

				success = CHB_AVControl(VIDCellTemp,AUDHandle,VIDEO_DISPLAY,AudioControl,ControlType);

#ifdef CH_FOR_ONLY_AUDO_OR_VIEDO
				if( gi_OnlyVidPid != DEMUX_INVALID_PID )
				{
					STPTI_SlotSetPid( VideoSlot, gi_OnlyVidPid );
				}	
#endif		
			/*************/	  
			}
		} 
		else
		{
			if(VideoControl == VIDEO_BLACK || VideoControl == VIDEO_CLOSE || VideoControl == VIDEO_FREEZE)/*20070913*/
			{
			        //   CH_InitVideofram();
			if(VideoControl == VIDEO_FREEZE)
			    	{
				STVID_Stop_t    StopMode;
				STVID_Freeze_t  Freeze_p;
				ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
				Freeze_p.Mode = STVID_FREEZE_MODE_FORCE;/*STVID_FREEZE_MODE_NO_FLICKER*/
				Freeze_p.Field = STVID_FREEZE_FIELD_TOP;
				StopMode = STVID_STOP_NOW; /*STVID_STOP_WHEN_END_OF_DATA*/
				STVID_Stop(VIDHandle[gch_LastViedoDevice],STVID_STOP_NOW,&Freeze_p);
			    	}

			}
			
			success = CHB_AVControl(VIDCellTemp,AUDHandle,VideoControl,AudioControl,ControlType);
		}



			ch_LastVideoControlType = VideoControl;
		}
		
		if(success == false)
		{
			if(ControlType == CONTROL_VIDEOANDAUDIO || ControlType == CONTROL_AUDIO)
			{
				g_LastAudioControl = AudioControl;
			}
		}

		semaphore_signal(pst_AVControlSema);/*20070717 add*/

		return success;
	}
	else
	{
		return true;
	}	
}

 


#if 1 /*yxl 2007-04-17 add below function*/
void CH6_NewChannel(int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType,BYTE VidType)
{
	BOOL res;
	BYTE VidTypeTemp=VidType;
	BOOL IsSmallWindow=FALSE;/*yxl 2007-04-30 add this line*/
        
	CH_SingleVIDCELL_t   VIDOCellTemp;
	VID_Device ch_LastVid = gch_LastViedoDevice;
	/*return FALSE;yxl 2007-06-06 temp add this line for test*/	
	memset((void*)&VIDOCellTemp,0,sizeof(CH_SingleVIDCELL_t));

	semaphore_wait(pst_AVControlSema);/*20070717 add*/
       if(EnableVideo == true)
       {
		if(VidTypeTemp==MPEG1_VIDEO_STREAM||VidTypeTemp==MPEG2_VIDEO_STREAM)
		{
	           if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
	           {
				VID_CloseViewport(VID_H,LAYER_VIDEO2);
				VID_CloseViewport(VID_H,LAYER_VIDEO1);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
	           	}
				CH_VideoDecoderControl(VID_H,FALSE);		
				CH_VideoDecoderControl(VID_MPEG2,TRUE);

	           if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
	           	{
				res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
			
				res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
				gch_LastViedoDevice=VID_MPEG2;
				CH6_TerminateChannel();
	             }
			


			VIDOCellTemp.VideoHandle=VIDHandle[VID_MPEG2];		
			VIDOCellTemp.VPortCount=2;
			VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
			VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];		
		}
		else if(VidTypeTemp==H264_VIDEO_STREAM)
		{
	           if(gch_LastViedoDevice!=VID_H)	/*20070718 add*/
	           	{
			VID_CloseViewport(VID_H,LAYER_VIDEO2);
			VID_CloseViewport(VID_H,LAYER_VIDEO1);
			VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
			VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);

	           	}
			   
			CH_VideoDecoderControl(VID_MPEG2,FALSE);		
			CH_VideoDecoderControl(VID_H,TRUE);
		       if(gch_LastViedoDevice!=VID_H)	/*20070718 add*/
		       {
				res=VID_OpenViewport(VID_H,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
				res=VID_OpenViewport(VID_H,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
				gch_LastViedoDevice=VID_H;
				CH6_TerminateChannel();
	           	}
		

			VIDOCellTemp.VideoHandle=VIDHandle[VID_H];		
			VIDOCellTemp.VPortCount=2;
			VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_H][LAYER_VIDEO1];
			VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_H][LAYER_VIDEO2];		

		}

		/*yxl 2007-04-30 add below section*/
		if(stVPInputRect.Width==0&&stVPInputRect.Height==0\
			&&stVPInputRect.PositionX==0&&stVPInputRect.PositionY==0)
		{
			IsSmallWindow=FALSE;
		}
		else
		{
			IsSmallWindow=TRUE;
		}
		
		CH6_SetVideoWindowSize(IsSmallWindow, stVPInputRect.PositionX,
			stVPInputRect.PositionY,stVPInputRect.Width,stVPInputRect.Height);/*yxl 2007-04-30 temp add for test*/
       }
	/*end yxl 2007-04-30 add below section*/

	CHB_NewChannel(VIDOCellTemp,AUDHandle,VideoSlot,AudioSlot,PCRSlot,
		VideoPid,AudeoPid,PCRPid,EnableVideo,EnableAudeo,AudType);


	gi_VidPid = VideoPid;	
	semaphore_signal(pst_AVControlSema);/*20070717 add*/
	return; 
}

#endif  /*end yxl 2007-04-17 add below function*/



BOOL  CH6_SetVideoWindowSize(BOOL Small,unsigned short PosX,unsigned short PosY,
						 unsigned short Width,unsigned short Height )
{
	int PosXTemp=PosX;
	int PosYTemp=PosY;
	int WidthTemp=Width;
	int HeightTemp=Height;
	 int  REGION_WIDTH;
	 int  REGION_HEIGHT;

	CH_RESOLUTION_MODE_e enum_Mode;
	enum_Mode = CH_GetReSolutionRate() ;
	 if(enum_Mode == CH_OLD_MOE)
 	{
 		REGION_WIDTH=719;
		REGION_HEIGHT=525;
 	}
	 else
 	{
 		REGION_WIDTH=1279;
		REGION_HEIGHT=719;
 	}
	
	STVID_ViewPortHandle_t VPHandleTemp=0;
	/*return FALSE;yxl 2007-06-06 temp add this line for test*/	
	if(Small==FALSE)/*full screen play*/
	{
		PosXTemp=0;
		PosYTemp=0;
		VPHandleTemp=0;
		if(pstBoxInfo->HDVideoOutputType!=TYPE_INVALID)	
		{
			if(VIDVPHandle[VID_MPEG2][LAYER_VIDEO1]!=0)
			{
				VPHandleTemp=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
			}
			else if(VIDVPHandle[VID_H][LAYER_VIDEO1]!=0&&VPHandleTemp==0)
			{
				VPHandleTemp=VIDVPHandle[VID_H][LAYER_VIDEO1];				
			}
			else
			{ 
				return TRUE;
			}
			
			if(WidthTemp==0)
			{
				WidthTemp=gVTGModeParam.ActiveAreaWidth/2;
				
			}
			if(HeightTemp==0)
			{
				HeightTemp=gVTGModeParam.ActiveAreaHeight/2;
			}
			CHB_SetVideoWindowSize( VPHandleTemp,Small,PosXTemp,PosYTemp,
				WidthTemp,HeightTemp);

			stVPInputRect.PositionX=0;
			stVPInputRect.PositionY=0;
			stVPInputRect.Width=0;
			stVPInputRect.Height=0;
 
		}
		
		if(pstBoxInfo->SDVideoOutputType!=TYPE_INVALID)	
		{
			VPHandleTemp=0;
			if(VIDVPHandle[VID_MPEG2][LAYER_VIDEO2]!=0)
			{
				VPHandleTemp=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];
			}
			else if(VIDVPHandle[VID_H][LAYER_VIDEO2]!=0&&VPHandleTemp==0)
			{
				VPHandleTemp=VIDVPHandle[VID_H][LAYER_VIDEO2];				
			}
			else
			{ 
				return TRUE;
			}
			
			WidthTemp=gVTGAUXModeParam.ActiveAreaWidth/2;
			HeightTemp=gVTGAUXModeParam.ActiveAreaHeight/2;
			
			CHB_SetVideoWindowSize(VPHandleTemp,Small,PosXTemp,PosYTemp,
				WidthTemp,HeightTemp);
		}
		return FALSE;
		
	}
	
	/* Below is small windon play*/
	if(pstBoxInfo->HDVideoOutputType!=TYPE_INVALID)
	{
		VPHandleTemp=0;

		if(VIDVPHandle[VID_MPEG2][LAYER_VIDEO1]!=0)
		{
			VPHandleTemp=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
		}
		else if(VIDVPHandle[VID_H][LAYER_VIDEO1]!=0&&VPHandleTemp==0)
		{
			VPHandleTemp=VIDVPHandle[VID_H][LAYER_VIDEO1];				
		}
		else
		{ 
			return TRUE;
		}
		

		if(WidthTemp==0)
		{
			WidthTemp=gVTGModeParam.ActiveAreaWidth/2;
		}
		if(HeightTemp==0)
		{
			HeightTemp=gVTGModeParam.ActiveAreaHeight/2;
		}
		switch(pstBoxInfo->HDVideoTimingMode)
		{
		case MODE_1080I60:
		case MODE_1080I50:
			PosXTemp=(PosXTemp*1920)/REGION_WIDTH-10-6;/* 20070805 modify*/
			PosYTemp=(PosYTemp*1080)/REGION_HEIGHT-11;
			WidthTemp=WidthTemp*1920/REGION_WIDTH+16;
			HeightTemp=HeightTemp*1080/REGION_HEIGHT-30+18;
			break;
			
		case MODE_720P60:
		case MODE_720P50:
			PosXTemp=PosXTemp*1280/REGION_WIDTH+2+2-8;
			PosYTemp=PosYTemp*720/REGION_HEIGHT+15-9-10+1;
			WidthTemp=WidthTemp*1280/REGION_WIDTH+12;
			HeightTemp=HeightTemp*720/REGION_HEIGHT-33+8;
			break;
		case MODE_720P50_VGA:
		case MODE_720P50_DVI:
			PosXTemp=PosXTemp*1280/REGION_WIDTH+1;
			PosYTemp=PosYTemp*720/REGION_HEIGHT+13;
			WidthTemp=WidthTemp*1280/REGION_WIDTH;
			HeightTemp=HeightTemp*720/REGION_HEIGHT-33;
			break;
		case MODE_576P50:
	
			PosXTemp=PosXTemp*720/REGION_WIDTH+3-10-1;
			PosYTemp=PosYTemp*576/REGION_HEIGHT+4-10+3;
			HeightTemp=HeightTemp*576/REGION_HEIGHT-25;
			WidthTemp=WidthTemp*720/REGION_WIDTH+2;
			break;
		case MODE_576I50:
			PosXTemp=PosXTemp*720/REGION_WIDTH-30;
			PosYTemp=PosYTemp*576/REGION_HEIGHT+22-30;
			HeightTemp=HeightTemp*576/REGION_HEIGHT-24;
			WidthTemp=WidthTemp*720/REGION_WIDTH+10;
			break;
		default:
			break;
		}
		if(PosXTemp<0)
			PosXTemp=0;
		if(PosYTemp<0)
			PosYTemp=0;
		STTBX_Print(("\n YxlInfoA:W=%d %d %d %d",
			PosXTemp,PosYTemp,WidthTemp,HeightTemp));

		
		CHB_SetVideoWindowSize( VPHandleTemp,Small,PosXTemp,PosYTemp,
			WidthTemp,HeightTemp+30);

		stVPInputRect.PositionX=PosX;
		stVPInputRect.PositionY=PosY;
		stVPInputRect.Width=Width;
		stVPInputRect.Height=Height;
	}

	
	if(pstBoxInfo->SDVideoOutputType!=TYPE_INVALID) /*yxl 2007-04-30 temp cancel for test*/
	{
		VPHandleTemp=0;

		if(VIDVPHandle[VID_MPEG2][LAYER_VIDEO2]!=0)
		{
			VPHandleTemp=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];
		}
		else if(VIDVPHandle[VID_H][LAYER_VIDEO2]!=0&&VPHandleTemp==0)
		{
			VPHandleTemp=VIDVPHandle[VID_H][LAYER_VIDEO2];				
		}
		else
		{ 
			return TRUE;
		}

		PosXTemp=PosX;
		PosYTemp=PosY;
		WidthTemp=Width;
		HeightTemp=Height;
		
		if(WidthTemp==0)
		{
			WidthTemp=gVTGAUXModeParam.ActiveAreaWidth/2;
		}
		if(HeightTemp==0)
		{
			HeightTemp=gVTGAUXModeParam.ActiveAreaHeight/2;
		}
		switch(pstBoxInfo->SDVideoTimingMode)
		{
			
		case MODE_576I50:/* cqj 20070801 modify*/
			PosXTemp=(PosXTemp*720)/REGION_WIDTH-12;
			PosYTemp=(PosYTemp*576)/REGION_HEIGHT-2;
			HeightTemp=(HeightTemp*576)/REGION_HEIGHT+12;
			WidthTemp=(WidthTemp*720)/REGION_WIDTH+32;
			break;
		}
		
		if(PosXTemp<0)
			PosXTemp=0;
		if(PosYTemp<0)
			PosYTemp=0;
		CHB_SetVideoWindowSize(VPHandleTemp,Small,PosXTemp,PosYTemp,
			WidthTemp,HeightTemp);
		
		STTBX_Print(("\n YxlInfo:W=%d %d %d %d\n",
			PosXTemp,PosYTemp,WidthTemp,HeightTemp));
		
	}
	
	return FALSE;
	
}






#if 1 /*yxl 2007-04-19 add below function*/
void CH6_TerminateChannel(void)
{
	#if 1
	CH_VIDCELL_t VIDCellTemp;
/*	return FALSE;yxl 2007-06-06 temp add this line for test*/	
	memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));
	VIDCellTemp.VIDDecoderCount=2;
	VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
	VIDCellTemp.VIDCell[1].VideoHandle=VIDHandle[VID_H];
	
	CHB_TerminateChannel(VIDCellTemp,AUDHandle,VideoSlot,AudioSlot,PCRSlot);
	
	return;
	#endif
}
#endif /*end yxl 2007-04-19 add below function*/

#if 1 /*yxl 2007-04-28 add below function,yxl 2007-07-03 add U32 Loops,U32 Priority*/
BOOL CH_DisplayIFrame(char *PICFileName,U32 Loops,U32 Priority)
{
	CH_SingleVIDCELL_t   VIDOCellTemp;
	CH_VIDCELL_t VIDCellTemp;
	BOOL res;
	BOOL IsSmallWindow=FALSE;/*yxl 2007-07-03 add this line*/
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority+3;
       U32 ui_TaskPriority;
       ui_TaskPriority = task_priority(NULL);
	
       Task_Priority_Set(NULL, PriorityTemp);

	memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));
	memset((void*)&VIDOCellTemp,0,sizeof(CH_SingleVIDCELL_t));
	
#if 0 
	
	VIDCellTemp.VIDDecoderCount=1;
	VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
	
	CHB_DisplayIFrame(0,0,PICFileName,VIDCellTemp);
	
#else
	CH6_AVControl(VIDEO_CLOSE/*VIDEO_BLACK*/,false,CONTROL_VIDEO);/*yxl  2007-07-03 add this line*/
  if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
  	{
	VID_CloseViewport(VID_H,LAYER_VIDEO2);
	VID_CloseViewport(VID_H,LAYER_VIDEO1);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
  	}
	CH_VideoDecoderControl(VID_H,FALSE);		
	CH_VideoDecoderControl(VID_MPEG2,TRUE);
	  if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
	 {
		res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
		
		res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
		gch_LastViedoDevice=VID_MPEG2;
  	}
	/*	CH6_TerminateChannel();yxl 2007-04-19 add this line*/
	
	VIDCellTemp.VIDDecoderCount=1;
	VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
	VIDCellTemp.VIDCell[0].VPortCount=2;
	VIDCellTemp.VIDCell[0].VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
	VIDCellTemp.VIDCell[0].VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];
#if 0
	VIDOCellTemp.VideoHandle=VIDHandle[VID_MPEG2];		
	VIDOCellTemp.VPortCount=2;/*2,0*/
	VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
	VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];
#endif

	/*yxl 2007-07-03 add below section*/
	if(stVPInputRect.Width==0&&stVPInputRect.Height==0\
		&&stVPInputRect.PositionX==0&&stVPInputRect.PositionY==0)
	{
		IsSmallWindow=FALSE;
	}
	else
	{
		IsSmallWindow=TRUE;
	}
	/*20070914 add*/
	CH_SetVideoAspectRatioConversion(CH_GetSTVideoARConversionFromCH(CONVERSION_FULLSCREEN));

	
	CH6_SetVideoWindowSize(IsSmallWindow, stVPInputRect.PositionX,
		stVPInputRect.PositionY,stVPInputRect.Width,stVPInputRect.Height);/*yxl 2007-04-30 temp add for test*/
	/*end yxl 2007-07-03 add below section*/
	CHB_DisplayIFrame(0,0,PICFileName,VIDCellTemp,LoopsTemp,PriorityTemp);
#endif
	Task_Priority_Set(NULL, ui_TaskPriority);
}

#endif 

#if 1 
BOOL CH_DisplayIFrameFromMemory(U8* pData,U32 Size,U32 Loops,U32 Priority)
{
	CH_SingleVIDCELL_t   VIDOCellTemp;
	CH_VIDCELL_t VIDCellTemp;
	BOOL res;
	BOOL IsSmallWindow=FALSE;
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority+3;
	U8* pDataTemp=pData;
	U32 SizeTemp=Size;
       U32 ui_TaskPriority;
	if(pDataTemp==NULL||SizeTemp==0)
	{
		return TRUE;
	}
	 
       ui_TaskPriority = task_priority(NULL);
	
	Task_Priority_Set(NULL, PriorityTemp);
	memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));
	memset((void*)&VIDOCellTemp,0,sizeof(CH_SingleVIDCELL_t));

	//CH6_AVControl(VIDEO_BLACK,false,CONTROL_VIDEO);/*yxl  2007-07-03 add this line*/
      if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
  	{
	VID_CloseViewport(VID_H,LAYER_VIDEO2);
	VID_CloseViewport(VID_H,LAYER_VIDEO1);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
  	}
  
	CH_VideoDecoderControl(VID_H,FALSE);		
	CH_VideoDecoderControl(VID_MPEG2,TRUE);
	  if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
	  	{
		res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
		
		res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
	      gch_LastViedoDevice=VID_MPEG2;
        }
	/*	CH6_TerminateChannel();yxl 2007-04-19 add this line*/
	
	VIDCellTemp.VIDDecoderCount=1;
	VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
	VIDCellTemp.VIDCell[0].VPortCount=2;
	VIDCellTemp.VIDCell[0].VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
	VIDCellTemp.VIDCell[0].VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];

	if(stVPInputRect.Width==0&&stVPInputRect.Height==0\
		&&stVPInputRect.PositionX==0&&stVPInputRect.PositionY==0)
	{
		IsSmallWindow=FALSE;
	}
	else
	{
		IsSmallWindow=TRUE;
	}
		/*20070914 add*/
	CH_SetVideoAspectRatioConversion(CH_GetSTVideoARConversionFromCH(CONVERSION_FULLSCREEN));

	CH6_SetVideoWindowSize(IsSmallWindow, stVPInputRect.PositionX,
		stVPInputRect.PositionY,stVPInputRect.Width,stVPInputRect.Height);

/*CHB_DisplayIFrame(0,0,PICFileName,VIDCellTemp,LoopsTemp,PriorityTemp);*/
	CHB_DisplayIFrameFromMemory(pDataTemp,SizeTemp,VIDCellTemp,LoopsTemp,PriorityTemp); 
	task_priority_set(NULL, ui_TaskPriority);

}

#endif /*end yxl  2007-07-04 add below function*/




#if 1 /*yxl 2007-05-16 add below function*/
void CH_SetVideoAspectRatioConversion(STVID_DisplayAspectRatioConversion_t ARConversion)
{
	ST_ErrorCode_t ErrCode=ST_NO_ERROR;
	int i,j;
	STVID_DisplayAspectRatioConversion_t  TempARConversion;
/*		return FALSE;yxl 2007-06-06 temp add this line for test*/
	for(i=0;i<2;i++)
	{
		for(j=0;j<2;j++)
		{
			if(VIDVPHandle[i][j]!=0)
			{
				if( i == 0 &&  j == 1)/*wz 20080129 add  i == 0*/
				{
					if(pstBoxInfo->VideoARConversion == CONVERSION_LETTETBOX)
						{
							TempARConversion = STVID_DISPLAY_AR_CONVERSION_LETTER_BOX;
							ErrCode = STVID_SetDisplayAspectRatioConversion(VIDVPHandle[i][j],TempARConversion);
				
						}
					else
						{
						ErrCode = STVID_SetDisplayAspectRatioConversion(VIDVPHandle[i][j],ARConversion);
				
						}
				}
				else
				{
				ErrCode = STVID_SetDisplayAspectRatioConversion(VIDVPHandle[i][j],ARConversion);

				}
				if (ErrCode != ST_NO_ERROR)
				{
					STTBX_Print(("\nYxlInfo:STVID_SetDisplayAspectRatioConversion()=%s\n", GetErrorText(ErrCode)));
				}
			}
		}
	}
	if(ErrCode==ST_NO_ERROR) 
		return ;
	else 
		return ;
}

#endif  /*end yxl 2007-05-16 add below function*/

/******************************************************************/
/*函数名:BOOL CH_DisplayIFrameForBackPic(char *PICFileName,U32 Loops,U32 Priority)                                                   */
/*开发人和开发时间:zengxianggen 2007-09-02                            */
/*函数功能描述:显示广播背景I贞图,在CH_SHowBackpic中调用*/
/*函数原理说明:                                                                             */
/*输入参数：无                                                                              */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                       */
/*返回值说明：无                                                                       */   
/*调用注意事项:                                                                           */
/*其他说明:                                                                                      */  

BOOL CH_DisplayIFrameForBackPic(char *PICFileName,U32 Loops,U32 Priority)
{
	CH_SingleVIDCELL_t   VIDOCellTemp;
	CH_VIDCELL_t VIDCellTemp;
	BOOL res;
	BOOL IsSmallWindow=FALSE;/*yxl 2007-07-03 add this line*/
	U32 LoopsTemp=Loops;
	U32 PriorityTemp=Priority+3;
       U32 ui_TaskPriority;
       ui_TaskPriority = task_priority(NULL);
	
       Task_Priority_Set(NULL, PriorityTemp);
/*	return FALSE;yxl 2007-06-06 temp add this line for test*/
	memset((void*)&VIDCellTemp,0,sizeof(CH_VIDCELL_t));
	memset((void*)&VIDOCellTemp,0,sizeof(CH_SingleVIDCELL_t));
  if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
  	{
	VID_CloseViewport(VID_H,LAYER_VIDEO2);
	VID_CloseViewport(VID_H,LAYER_VIDEO1);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
  	}
	CH_VideoDecoderControl(VID_H,FALSE);		
	CH_VideoDecoderControl(VID_MPEG2,TRUE);
	  if(gch_LastViedoDevice!=VID_MPEG2)	/*20070718 add*/
	 {
		res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
		
		res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
		gch_LastViedoDevice=VID_MPEG2;
  	}
	/*	CH6_TerminateChannel();yxl 2007-04-19 add this line*/
	
	VIDCellTemp.VIDDecoderCount=1;
	VIDCellTemp.VIDCell[0].VideoHandle=VIDHandle[VID_MPEG2];
	VIDCellTemp.VIDCell[0].VPortCount=2;
	VIDCellTemp.VIDCell[0].VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
	VIDCellTemp.VIDCell[0].VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];

	/*yxl 2007-07-03 add below section*/
	if(stVPInputRect.Width==0&&stVPInputRect.Height==0\
		&&stVPInputRect.PositionX==0&&stVPInputRect.PositionY==0)
	{
		IsSmallWindow=FALSE;
	}
	else
	{
		IsSmallWindow=TRUE;
	}
		/*20070914 add*/
if(CH_GetCurApplMode( ) != APP_MODE_PLAY)
	CH_SetVideoAspectRatioConversion(CH_GetSTVideoARConversionFromCH(CONVERSION_FULLSCREEN));

	CH6_SetVideoWindowSize(IsSmallWindow, stVPInputRect.PositionX,
		stVPInputRect.PositionY,stVPInputRect.Width,stVPInputRect.Height);/*yxl 2007-04-30 temp add for test*/
	/*end yxl 2007-07-03 add below section*/
	CHB_DisplayIFrame(0,0,PICFileName,VIDCellTemp,LoopsTemp,PriorityTemp);
	task_priority_set(NULL, ui_TaskPriority/*4*/);
}
/******************************************************************/
/*函数名:void CH_SHowBackpic(void)                                                     */
/*开发人和开发时间:zengxianggen 2007-09-02                            */
/*函数功能描述:显示广播背景I贞图,在CH6_AVControl中调用*/
/*函数原理说明:                                                                             */
/*输入参数：无                                                                              */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                       */
/*返回值说明：无                                                                       */   
/*调用注意事项:                                                                           */
/*其他说明:                                                                                      */      
void CH_SHowBackpic(void)
{
   	CH6_CloseMepg();	
   
#ifdef USE_IPANEL/*北京IPANEL3.专用*/
       EIS_ShowIFrame();
#else
       CH_DisplayIFrameForBackPic(RADIOBACK,1,7);
#endif

}
VID_Device CH_GetCurrentVIDHandle( void )
{
   VID_Device ch_vidtype;
   semaphore_wait(pst_AVControlSema);
   ch_vidtype = gch_LastViedoDevice;
   semaphore_signal(pst_AVControlSema);
   return ch_vidtype;
}
#if 1
extern ST_DeviceName_t VIDDeviceName[2];
static boolean gb_Subscrtibed=false;
static int gi_NewFramCount = 0;
static boolean gb_HDVideo  = false;
/******************************************************************/
/*函数名:void CH_NewVideoFrameEvent(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p)                                       */
/*开发人和开发时间:zengxianggen 2007-09-11                            */
/*函数功能描述:新的视频贞callback        */
/*函数原理说明: 新的视频贞callback                                                                                   */
/*输入参数：                                                                                   */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                        */
/*返回值说明：无                                                                       */   
/*调用注意事项:                                                                            */
/*其他说明:                                                                                       */  
void CH_NewVideoFrameEvent(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, const void *EventData, const void *SubscriberData_p)
{
	
	STVID_PictureParams_t st_VidParams;
	switch (Event)
	{
		case STVID_DISPLAY_NEW_FRAME_EVT:
			
			st_VidParams = *((STVID_PictureParams_t *)EventData);
			if(gi_NewFramCount<5)
				{
			        gi_NewFramCount++;
			        if(st_VidParams.Height>=720)
				 {
				       gb_HDVideo = true;
				 }
				else
					{
				       gb_HDVideo = false;
					}
				}
			break;

		case STVID_FRAME_RATE_CHANGE_EVT:
			
			break;
			
    }

}
/******************************************************************/
/*函数名:void CH_NewSubFrame( VID_Device ch_Lastvidtype ,VID_Device ch_Curvidtype )                                                 */
/*开发人和开发时间:zengxianggen 2007-09-11                            */
/*函数功能描述:新的视频贞callback                                      */
/*函数原理说明: 新的视频贞callback                                    */
/*输入参数：                                                                                   */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                        */
/*返回值说明：无                                                                       */   
/*调用注意事项:                                                                            */
/*其他说明:                                                                                       */  
void CH_NewSubFrame( VID_Device ch_Lastvidtype ,VID_Device ch_Curvidtype)
{
	STEVT_DeviceSubscribeParams_t st_SubscribeParams;

	if(gb_Subscrtibed)
	{
	     		if (STEVT_UnsubscribeDeviceEvent(EVTHandle,  VIDDeviceName[ch_Lastvidtype], (U32)STVID_DISPLAY_NEW_FRAME_EVT)==ST_NO_ERROR )
				{
					gb_Subscrtibed = false;
				}
				else
				{
				     ;
				} 
	}	
	if (gb_Subscrtibed == FALSE)
	{
		memset(&st_SubscribeParams, 0, sizeof(STEVT_DeviceSubscribeParams_t));	
		st_SubscribeParams.NotifyCallback = CH_NewVideoFrameEvent;	
		if (STEVT_SubscribeDeviceEvent(EVTHandle,  VIDDeviceName[ch_Curvidtype], (U32)STVID_DISPLAY_NEW_FRAME_EVT, &st_SubscribeParams)==ST_NO_ERROR)
		{
			gb_Subscrtibed = TRUE;
			 gb_HDVideo = false;
			gi_NewFramCount = 0;
		}
		else
		{
			;
		}
	}
}

#endif

/******************************************************************/
/*函数名:boolean CH_HDVideoType( void )                                              */
/*开发人和开发时间:zengxianggen 2007-09-11                            */
/*函数功能描述:判断当前的视频是否为高清           */
/*函数原理说明:                                                                             */
/*输入参数：                                                                                   */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                        */
/*返回值说明：TRUE, 是,FALSE,否                                   */   
/*调用注意事项:                                                                            */
/*其他说明:                                                                                       */     
boolean CH_HDVideoType( void )
{
   return gb_HDVideo;
}

/******************************************************************/
/*函数名:boolean CH_VideoFramChange( void )                                       */
/*开发人和开发时间:zengxianggen 2007-09-11                            */
/*函数功能描述:判断是否播放过新的视频           */
/*函数原理说明: 用于控制显示广播背景I FRAM        */
/*输入参数：                                                                                   */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                        */
/*返回值说明：TRUE, 是,FALSE,否                                     */   
/*调用注意事项:                                                                            */
/*其他说明:                                                                                       */  
boolean CH_VideoFramChange( void )
{
   if( gi_NewFramCount > 0  )
   	{
   	    return true;
   	}
       else
       {
           return false;
       }
}
/******************************************************************/
/*函数名:void CH_InitVideofram( void )                                  */
/*开发人和开发时间:zengxianggen 2007-09-11                            */
/*函数功能描述:初始化           */
/*函数原理说明:用于控制显示广播背景I FRAM       */
/*输入参数：                                                                                   */
/*输出参数: 无                                                                                 */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                                        */
/*返回值说明：TRUE, 是,FALSE,否                                     */   
/*调用注意事项:                                                                            */
/*其他说明:                                                                                       */  
void CH_InitVideofram( void )
{
    gi_NewFramCount = 0;
}

#if 1
/*zxg 20061108 add new audio frame handle*/
static boolean CH_HaveSubNewFram=false;/*是否已经订购STAUD_NEW_FRAME_EVT事件*/
semaphore_t  *gp_AudNewFramSeam;
boolean AudNewFramNotify=false;
/*******************************************************************************
 *Function name: CH_AudioNewFrame
 * 
 *
 *Description: AUdio event notifiy function   
 *              
 *
 *
 *Prototype:
 *     static void CH_AudioNewFrame ( STEVT_CallReason_t Reason,
                           STEVT_EventConstant_t Event, const void *EventData)
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:STAUD_NEW_FRAME_EVT
 *           none
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/

static void CH_AudioNewFrame ( STEVT_CallReason_t Reason,
                           STEVT_EventConstant_t Event, const void *EventData)
{
   if(AudNewFramNotify == false)
   	{
             
     semaphore_signal(gp_AudNewFramSeam);
     AudNewFramNotify = true;

    }
}
/************************函数说明***********************************/
/*函数名:void CH_SubaudioNewfram( void )            */
/*开发人和开发时间:zengxianggen 2006-11-08                         */
/*函数功能描述:订购AUDIO           STAUD_NEW_FRAME_EVT事件                           */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：  */
/*输入参数:                                 */
/*返回值说明：无                  */
void CH_SubaudioNewfram( void )
{
ST_ErrorCode_t ErrCode;
STEVT_SubscribeParams_t SubscribeParams = {CH_AudioNewFrame};
if(CH_HaveSubNewFram == false)
{
 ErrCode=STEVT_Subscribe(EVTHandle, (U32)STAUD_NEW_FRAME_EVT, &SubscribeParams);
 if(ErrCode == ST_NO_ERROR)
 	{
          CH_HaveSubNewFram = true;
 	}
}
}
/************************函数说明***********************************/
/*函数名:void CH_UnSubaudioNewfram(void)          */
/*开发人和开发时间:zengxianggen 2006-11-08                         */
/*函数功能描述:取消订购AUDIO           STAUD_NEW_FRAME_EVT事件                           */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：  */
/*输入参数:无*/
/*返回值说明：无                  */
void CH_UnSubaudioNewfram(void)
{
ST_ErrorCode_t ErrCode;
if(CH_HaveSubNewFram == true)
{
 ErrCode=STEVT_Unsubscribe(EVTHandle, (U32)STAUD_NEW_FRAME_EVT);
 if(ErrCode == ST_NO_ERROR)
 	{
          CH_HaveSubNewFram = false;
 	}
}
gp_AudNewFramSeam = semaphore_create_fifo_timeout( 0 );
AudNewFramNotify = false;
}

/************************函数说明***********************************/
/*函数名:void CH_WaitAudioNewFram(void)        */
/*开发人和开发时间:zengxianggen 2006-11-12                        */
/*函数功能描述:等待新的Audio数据                                */
/*函数原理说明:                                                                         */
/*使用的全局变量、表和结构：                                    */
/*调用的主要函数列表：  */
/*输入参数:int ri_waitseconds,等待时间*/
/*返回值说明：无                  */
void CH_WaitAudioNewFram(int ri_waitmiliseconds)
{
   clock_t Timeout ;
   Timeout= time_plus(time_now(), ST_GetClocksPerSecond()*ri_waitmiliseconds/1000);
  if(semaphore_wait_timeout(gp_AudNewFramSeam,&Timeout)==0)
  	{
  	   /*20071130 add delay for audio */
  	   task_delay(ST_GetClocksPerSecond()/10);
	   /*endif 20071130*/
  	}
}
#endif


#ifdef CH_FOR_ONLY_AUDO_OR_VIEDO




void CH_OnlyAudioControl(int ri_vidpid,int ri_AudeoPid,BOOL rb_EnableAudeo,STAUD_StreamContent_t rst_streamtype,char rc_AMode)
{
	ST_ErrorCode_t 			ErrCode = ST_NO_ERROR;
	STVID_StartParams_t 		VIDStartParams;
	STAUD_StartParams_t 		AUDStartParams;
       U32 ui_TaskPriority;
	STAUD_Fade_t            Fade; 
	STAUD_Stereo_t StereoMode;
       semaphore_wait(pst_AVControlSema);/*200701 add*/

	ErrCode = STPTI_SlotClearPid( AudioSlot );

	if( ErrCode != ST_NO_ERROR )
	{
		;	
	}

	if( ri_AudeoPid < DEMUX_INVALID_PID )
	{
		ErrCode = STPTI_SlotSetPid( AudioSlot, ri_AudeoPid );
		if( ErrCode != ST_NO_ERROR )
		{
			;
		}	
		else
		{
			gi_OnlyAudioPid = ri_AudeoPid ;
		}
	}

       if( ri_AudeoPid < DEMUX_INVALID_PID && rb_EnableAudeo==TRUE )
	{
		memset(&AUDStartParams,0,sizeof(STAUD_StartParams_t));

		if(rc_AMode==0)
		{
			StereoMode=STAUD_STEREO_DUAL_MONO;
		}
		else if(rc_AMode==1)
		{
			StereoMode=STAUD_STEREO_DUAL_RIGHT;
		}
		else if(rc_AMode==2)
		{
			StereoMode=STAUD_STEREO_DUAL_LEFT;
		}
		else 
		{
			StereoMode=STAUD_STEREO_STEREO;
			
		}
		AUDStartParams.StreamContent     	= STAUD_STREAM_CONTENT_MPEG1;
		AUDStartParams.SamplingFrequency 	= 44100;
		AUDStartParams.StreamType 		= STAUD_STREAM_TYPE_PES;
		AUDStartParams.StreamID 			= STAUD_IGNORE_ID;
		if(rst_streamtype == STAUD_STREAM_CONTENT_AC3)
		{
			AUDStartParams.SamplingFrequency = 48000; /*yxl 2006-04-20 modify 96000 to 48000,yxl 2006-06-12 add 0*/
			AUDStartParams.StreamContent     	= STAUD_STREAM_CONTENT_AC3;
		}else if(rst_streamtype==STAUD_STREAM_CONTENT_MP3)
		{
			AUDStartParams.StreamType = STAUD_STREAM_TYPE_ES;
		}
		else
		if(rst_streamtype == STAUD_STREAM_CONTENT_MPEG_AAC)
			{
			AUDStartParams.StreamContent=STAUD_STREAM_CONTENT_MPEG_AAC;
			}
		ErrCode=STAUD_Stop( AUDHandle,STAUD_STOP_NOW, &Fade ); 
	       ErrCode=STAUD_SetStereoOutput(AUDHandle,StereoMode);	
		ErrCode = STAUD_Start( AUDHandle, &AUDStartParams );
		if(ErrCode!=ST_NO_ERROR )
		{
			do_report(0,"CH_AudioControl STAUD_Start error\n");
		}

	} 
	if(ri_vidpid < DEMUX_INVALID_PID)
	   ErrCode = STAUD_EnableSynchronisation( AUDHandle );
	else
	   ErrCode = STAUD_DisableSynchronisation( AUDHandle );


       semaphore_signal(pst_AVControlSema);
	   
#if 0
	if( bAudioMuteState == FALSE )
	{
		CH6_UnmuteAudio();
	}
#endif
   

	return;
}

void CH_OnlyVideoControl(int ri_VideoPid,boolean rb_Enablevideo,int ri_PCRPid,int ri_VidType)
{
	ST_ErrorCode_t 			ErrCode = ST_NO_ERROR;
	STVID_StartParams_t 		VIDStartParams;
	STAUD_StartParams_t 		AUDStartParams;
      	CH_SingleVIDCELL_t   VIDOCellTemp;
	CH_VIDCELL_t VIDCellTemp;
	BOOL res;
	BOOL IsSmallWindow=FALSE;

      semaphore_wait(pst_AVControlSema);/*200701 add*/

	ErrCode = STPTI_SlotClearPid( VideoSlot);

	if( ErrCode != ST_NO_ERROR )
	{
		;	
	}
	ErrCode = STPTI_SlotClearPid( PCRSlot );

	if( ErrCode != ST_NO_ERROR )
	{
		;	
	}
	if( ri_VideoPid < DEMUX_INVALID_PID )
	{
		ErrCode = STPTI_SlotSetPid( VideoSlot, ri_VideoPid );
		if( ErrCode != ST_NO_ERROR )
		{
			;
		}	
		else
		{
			gi_OnlyVidPid = ri_VideoPid ;
		}

	}
	
	if( ri_PCRPid < DEMUX_INVALID_PID )
	{
		ErrCode = STPTI_SlotSetPid( PCRSlot, ri_PCRPid );
		if( ErrCode != ST_NO_ERROR )
		{
			;
		}
		else
			gi_OnlyPcrPid = ri_PCRPid;
	}
	ErrCode = STCLKRV_InvDecodeClk( CLKRVHandle );
	if( ErrCode!=ST_NO_ERROR )
		;
	if( ri_VideoPid != DEMUX_INVALID_PID && rb_Enablevideo==TRUE )
	{
		VIDStartParams.RealTime      	= TRUE;
		VIDStartParams.UpdateDisplay 	= TRUE;
		VIDStartParams.StreamID      	= STVID_IGNORE_ID;
		VIDStartParams.DecodeOnce    	= FALSE;
		VIDStartParams.StreamType    	= STVID_STREAM_TYPE_PES;
		VIDStartParams.BrdCstProfile 	= STVID_BROADCAST_DVB;
	{
		if(ri_VidType==MPEG1_VIDEO_STREAM||ri_VidType==MPEG2_VIDEO_STREAM)
		{
	           if(gch_LastViedoDevice!=VID_MPEG2)	
	           {
				VID_CloseViewport(VID_H,LAYER_VIDEO2);
				VID_CloseViewport(VID_H,LAYER_VIDEO1);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
	           	}
				CH_VideoDecoderControl(VID_H,FALSE);		
				CH_VideoDecoderControl(VID_MPEG2,TRUE);

	           if(gch_LastViedoDevice!=VID_MPEG2)	
	           	{
				res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
			
				res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
				gch_LastViedoDevice=VID_MPEG2;
				
	             }
			
			VIDOCellTemp.VideoHandle=VIDHandle[VID_MPEG2];		
			VIDOCellTemp.VPortCount=2;
			VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
			VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];		
		}
		else if(ri_VidType==H264_VIDEO_STREAM)
		{
	           if(gch_LastViedoDevice!=VID_H)	/*20070718 add*/
	           	{
				VID_CloseViewport(VID_H,LAYER_VIDEO2);
				VID_CloseViewport(VID_H,LAYER_VIDEO1);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
	           	}
			   
			CH_VideoDecoderControl(VID_MPEG2,FALSE);		
			CH_VideoDecoderControl(VID_H,TRUE);
		       if(gch_LastViedoDevice!=VID_H)	/*20070718 add*/
		       {
				res=VID_OpenViewport(VID_H,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
				res=VID_OpenViewport(VID_H,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
				gch_LastViedoDevice=VID_H;
				//CH6_TerminateChannel();
	           	}
		

			VIDOCellTemp.VideoHandle=VIDHandle[VID_H];		
			VIDOCellTemp.VPortCount=2;
			VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_H][LAYER_VIDEO1];
			VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_H][LAYER_VIDEO2];		

		}

		/*yxl 2007-04-30 add below section*/
		if(stVPInputRect.Width==0&&stVPInputRect.Height==0\
			&&stVPInputRect.PositionX==0&&stVPInputRect.PositionY==0)
		{
			IsSmallWindow=FALSE;
		}
		else
		{
			IsSmallWindow=TRUE;
		}
		
		CH6_SetVideoWindowSize(IsSmallWindow, stVPInputRect.PositionX,
			stVPInputRect.PositionY,stVPInputRect.Width,stVPInputRect.Height);/*yxl 2007-04-30 temp add for test*/
             }
	      CH_NewSubFrame(gch_LastViedoDevice ,gch_LastViedoDevice);  /*20070913 add*/

		/*视频开始解码*/
		ErrCode = STVID_Start( VIDOCellTemp.VideoHandle, &VIDStartParams );
		if(ErrCode!=ST_NO_ERROR)
		{
			;
		}
		else
			;
			
		ErrCode = STVID_EnableSynchronisation(VIDOCellTemp.VideoHandle);
		if ( ErrCode != ST_NO_ERROR )
		{
		;
		}
		/*开始高清显示*/
		ErrCode = STVID_EnableOutputWindow(VIDOCellTemp.VPortHandle[0]);
		if ( ErrCode != ST_NO_ERROR )
		{
			;
		}
		else
			;
		/*开始标清清显示*/
		ErrCode = STVID_EnableOutputWindow(VIDOCellTemp.VPortHandle[1]);
		if ( ErrCode != ST_NO_ERROR )
		{
			;
		}
		else
			;
		
	}
       semaphore_signal(pst_AVControlSema);

	return;
}

void CH_OnlyVideoControl_xx(int ri_VideoPid,boolean rb_Enablevideo,int ri_PCRPid,int ri_VidType)
{
	ST_ErrorCode_t			ErrCode = ST_NO_ERROR;
			STVID_StartParams_t 		VIDStartParams;
			STAUD_StartParams_t 		AUDStartParams;
				CH_SingleVIDCELL_t	 VIDOCellTemp;
			CH_VIDCELL_t VIDCellTemp;
			BOOL res;
			BOOL IsSmallWindow=FALSE;
			
	if( ri_VideoPid != DEMUX_INVALID_PID && rb_Enablevideo==TRUE )
	{
		VIDStartParams.RealTime      	= TRUE;
		VIDStartParams.UpdateDisplay 	= TRUE;
		VIDStartParams.StreamID      	= STVID_IGNORE_ID;
		VIDStartParams.DecodeOnce    	= FALSE;
		VIDStartParams.StreamType    	= STVID_STREAM_TYPE_PES;
		VIDStartParams.BrdCstProfile 	= STVID_BROADCAST_DVB;
	{
		if(ri_VidType==MPEG1_VIDEO_STREAM||ri_VidType==MPEG2_VIDEO_STREAM)
		{
	           if(gch_LastViedoDevice!=VID_MPEG2)	
	           {
				VID_CloseViewport(VID_H,LAYER_VIDEO2);
				VID_CloseViewport(VID_H,LAYER_VIDEO1);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
	           	}
				CH_VideoDecoderControl(VID_H,FALSE);		
				CH_VideoDecoderControl(VID_MPEG2,TRUE);

	           if(gch_LastViedoDevice!=VID_MPEG2)	
	           	{
				res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
			
				res=VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
				gch_LastViedoDevice=VID_MPEG2;
				
	             }
			
			VIDOCellTemp.VideoHandle=VIDHandle[VID_MPEG2];		
			VIDOCellTemp.VPortCount=2;
			VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];
			VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];		
		}
		else if(ri_VidType==H264_VIDEO_STREAM)
		{
	           if(gch_LastViedoDevice!=VID_H)	/*20070718 add*/
	           	{
				VID_CloseViewport(VID_H,LAYER_VIDEO2);
				VID_CloseViewport(VID_H,LAYER_VIDEO1);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
				VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);
	           	}
			   
			CH_VideoDecoderControl(VID_MPEG2,FALSE);		
			CH_VideoDecoderControl(VID_H,TRUE);
		       if(gch_LastViedoDevice!=VID_H)	/*20070718 add*/
		       {
				res=VID_OpenViewport(VID_H,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
				res=VID_OpenViewport(VID_H,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
				gch_LastViedoDevice=VID_H;
				//CH6_TerminateChannel();
	           	}
		

			VIDOCellTemp.VideoHandle=VIDHandle[VID_H];		
			VIDOCellTemp.VPortCount=2;
			VIDOCellTemp.VPortHandle[0]=VIDVPHandle[VID_H][LAYER_VIDEO1];
			VIDOCellTemp.VPortHandle[1]=VIDVPHandle[VID_H][LAYER_VIDEO2];		

		}


		
		
			  semaphore_wait(pst_AVControlSema);/*200701 add*/
		
			ErrCode = STPTI_SlotClearPid( VideoSlot);
		
			if( ErrCode != ST_NO_ERROR )
			{
				;	
			}
			ErrCode = STPTI_SlotClearPid( PCRSlot );
		
			if( ErrCode != ST_NO_ERROR )
			{
				;	
			}
			if( ri_VideoPid < DEMUX_INVALID_PID )
			{
				ErrCode = STPTI_SlotSetPid( VideoSlot, ri_VideoPid );
				if( ErrCode != ST_NO_ERROR )
				{
					;
				}	
				else
				{
					//CH6_setCurVideoPid(ri_VideoPid);
					gi_OnlyVidPid = ri_VideoPid ;
				}
		
			}
			
			if( ri_PCRPid < DEMUX_INVALID_PID )
			{
				ErrCode = STPTI_SlotSetPid( PCRSlot, ri_PCRPid );
				if( ErrCode != ST_NO_ERROR )
				{
					;
				}
				else
					gi_OnlyPcrPid = ri_PCRPid;
			}
			ErrCode = STCLKRV_InvDecodeClk( CLKRVHandle );
			if( ErrCode!=ST_NO_ERROR )
				;
			

		/*yxl 2007-04-30 add below section*/
		if(stVPInputRect.Width==0&&stVPInputRect.Height==0\
			&&stVPInputRect.PositionX==0&&stVPInputRect.PositionY==0)
		{
			IsSmallWindow=FALSE;
		}
		else
		{
			IsSmallWindow=TRUE;
		}
		
		CH6_SetVideoWindowSize(IsSmallWindow, stVPInputRect.PositionX,
			stVPInputRect.PositionY,stVPInputRect.Width,stVPInputRect.Height);/*yxl 2007-04-30 temp add for test*/
             }
	      CH_NewSubFrame(gch_LastViedoDevice ,gch_LastViedoDevice);  /*20070913 add*/

		//STVID_SetErrorRecoveryMode(VIDOCellTemp.VideoHandle,STVID_ERROR_RECOVERY_NONE);

		/*视频开始解码*/
		ErrCode = STVID_Start( VIDOCellTemp.VideoHandle, &VIDStartParams );
		if(ErrCode!=ST_NO_ERROR)
		{
			;
		}
		else
			;
			
		ErrCode = STVID_EnableSynchronisation(VIDOCellTemp.VideoHandle);
		if ( ErrCode != ST_NO_ERROR )
		{
		;
		}
		/*开始高清显示*/
		ErrCode = STVID_EnableOutputWindow(VIDOCellTemp.VPortHandle[0]);
		if ( ErrCode != ST_NO_ERROR )
		{
			;
		}
		else
			;
		/*开始标清清显示*/
		ErrCode = STVID_EnableOutputWindow(VIDOCellTemp.VPortHandle[1]);
		if ( ErrCode != ST_NO_ERROR )
		{
			;
		}
		else
			;
		
	}
       semaphore_signal(pst_AVControlSema);

	return;
}

void CH_OnlyAudioCut(void)
{

      STAUD_Fade_t AudFade;
      ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	
	ErrCode = STPTI_SlotClearPid(AudioSlot);
	
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotClearPid(AudioSlot)=%s\n", GetErrorText(ErrCode) ));	
	}
	ErrCode = STAUD_Stop( AUDHandle, STAUD_STOP_NOW, &AudFade );		
      return;
}
void CH_OnlyVideoCut(void)
{

      STAUD_Fade_t AudFade;
      ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	
	ErrCode = STPTI_SlotClearPid(AudioSlot);
	
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotClearPid(AudioSlot)=%s\n", GetErrorText(ErrCode) ));	
	}
	ErrCode = STAUD_Stop( AUDHandle, STAUD_STOP_NOW, &AudFade );		
      return;
}


/************************函数说明***********************************/
/*函数名:boolean CH_VideoIsBackIFramData(void)                                            */
/*开发人和开发时间:zengxianggen 2006-02-08                                       */
/*函数功能描述:读取当前显示的视频数据，并且判断是否显示的背景I-FRAM数据                  */
/*函数原理说明:                                                                                     */
/*使用的全局变量、表和结构：                                                 */
/*调用的主要函数列表：                                                                 */
/*返回值说明：无                                                                                 */
static U32 gui_CRCIFramData = 0;

boolean CH_VideoIsBackIFramData(void)
{
    STVID_Freeze_t          st_Freeze;
   STVID_PictureParams_t st_PicParams;
   ST_ErrorCode_t ErrCode;
   U32 uI_CurCRCIFramData;
   ErrCode = STVID_Stop(VIDHandle[gch_LastViedoDevice],STVID_STOP_NOW,&st_Freeze);
   ErrCode = STVID_GetPictureParams(VIDHandle[gch_LastViedoDevice],STVID_PICTURE_DISPLAYED,&st_PicParams);
   if(ErrCode == 0)
   	{
   	     uI_CurCRCIFramData = CRC_Code32(st_PicParams.Data,  st_PicParams.Size/100);
	     if(gui_CRCIFramData ==uI_CurCRCIFramData)
		{
		               do_report(0," IFramData NO change\n");
                             return true;
		 }
		 else
		 {
		             do_report(0," IFramData have change\n");
		             return false;
		 }
   	}
   else
   	{
               return false;
   	}
}

int CH_GetBackIFramData(void)
{
    STVID_Freeze_t          st_Freeze;
   STVID_PictureParams_t st_PicParams;
   ST_ErrorCode_t ErrCode;
  // if(gui_CRCIFramData == 0)
   {
      st_Freeze.Field = STVID_FREEZE_FIELD_TOP;
      st_Freeze.Mode = STVID_FREEZE_MODE_NONE /*FORCE*/;
      ErrCode = STVID_Stop(VIDHandle[gch_LastViedoDevice],STVID_STOP_NOW,&st_Freeze);
      ErrCode = STVID_GetPictureParams(VIDHandle[gch_LastViedoDevice],STVID_PICTURE_DISPLAYED,&st_PicParams);
      if(ErrCode == 0 )
   	{
   	     gui_CRCIFramData = CRC_Code32(st_PicParams.Data,  st_PicParams.Size/100);            
		 return 0;
   	}
	  return -1;
   }
}

void CH_SWitchVidDecoder(void)
{
 
	VID_CloseViewport(VID_H,LAYER_VIDEO2);
	VID_CloseViewport(VID_H,LAYER_VIDEO1);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO2);
	VID_CloseViewport(VID_MPEG2,LAYER_VIDEO1);

       if(gch_LastViedoDevice ==VID_H)	
       	{
	CH_VideoDecoderControl(VID_H,FALSE);		
	CH_VideoDecoderControl(VID_MPEG2,TRUE);
	VID_OpenViewport(VID_MPEG2,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
	VID_OpenViewport(VID_MPEG2,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
	      gch_LastViedoDevice=VID_MPEG2;
       	}
	   else
	   	{
	   	CH_VideoDecoderControl(VID_MPEG2,FALSE);
	       CH_VideoDecoderControl(VID_H,TRUE);		
	      VID_OpenViewport(VID_H,LAYER_VIDEO1,pstBoxInfo->VideoARConversion);
	       VID_OpenViewport(VID_H,LAYER_VIDEO2,pstBoxInfo->VideoARConversion);
	       gch_LastViedoDevice=VID_H;
	}
	  
}
#endif

/*WZ ADD 20090118*/
static boolean b_checkvidErrMode = false;
clock_t  newchannel_starttime;
void CH_VID_ResumeMode(void)
{
	STVID_ErrorRecoveryMode_t viderrmode;
	ST_ErrorCode_t error_temp = ST_NO_ERROR;

	do_report(0,"\n\n mosaic test \n");
	
	//return;
	if(b_checkvidErrMode)
	{
	   if(time_minus(time_now(), newchannel_starttime) > 5*ST_GetClocksPerSecond())/*超过 5秒恢复为正常模式*/
	   {
		do_report(0,"\nCH_VID_ResumeMode\n");	
		
		error_temp = STVID_GetErrorRecoveryMode(VIDHandle[VID_MPEG2], &viderrmode);
			if(error_temp == 0 && viderrmode == STVID_ERROR_RECOVERY_FULL)
			{
				do_report(0," partial mode\n");	
					error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_MPEG2],STVID_ERROR_RECOVERY_HIGH);
					//error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_MPEG2],STVID_ERROR_RECOVERY_FULL);
					 if (error_temp != ST_NO_ERROR)
					 	{
					do_report(0,"  CH_VID_ResumeMode err =%s\n", GetErrorText(error_temp ));
					 	}
		    }
				error_temp = STVID_GetErrorRecoveryMode(VIDHandle[VID_H], &viderrmode);
		if(error_temp == 0 && viderrmode == STVID_ERROR_RECOVERY_FULL)
		{
			do_report(0," partial mode\n");	
			error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_H],STVID_ERROR_RECOVERY_HIGH);
			//error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_H],STVID_ERROR_RECOVERY_FULL);
			 if (error_temp != ST_NO_ERROR)
			 	{
			do_report(0,"  CH_VID_ResumeMode err =%s\n", GetErrorText(error_temp ));
		}
	    }
	    b_checkvidErrMode = false;
	  }
	}

}

void CH_VID_SetMode(void)
{

	STVID_ErrorRecoveryMode_t viderrmode;
	ST_ErrorCode_t error_temp = ST_NO_ERROR;
	//return;
	//if(b_checkvidErrMode==false)
	{
		do_report(0," CH_VID_SetMode\n");
		b_checkvidErrMode = true;
		newchannel_starttime = time_now();

		error_temp = STVID_GetErrorRecoveryMode(VIDHandle[VID_MPEG2], &viderrmode);
		if(error_temp == 0&& viderrmode  != STVID_ERROR_RECOVERY_FULL)
		{
			do_report(0," full  mode\n");
			error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_MPEG2],STVID_ERROR_RECOVERY_FULL);
			//error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_MPEG2],STVID_ERROR_RECOVERY_HIGH);
			if (error_temp != ST_NO_ERROR)
			{
				do_report(0,"  CH_VID_SetMode err =%s\n", GetErrorText(error_temp ));
			}
		}
		error_temp = STVID_GetErrorRecoveryMode(VIDHandle[VID_H], &viderrmode);
		if(error_temp == 0 && viderrmode  != STVID_ERROR_RECOVERY_FULL)
		{
			do_report(0," full  mode\n");
			error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_H],STVID_ERROR_RECOVERY_FULL);
			//error_temp = STVID_SetErrorRecoveryMode(VIDHandle[VID_H],STVID_ERROR_RECOVERY_HIGH);
			if (error_temp != ST_NO_ERROR)
			{
				do_report(0,"  CH_VID_SetMode err =%s\n", GetErrorText(error_temp ));
			}
		}
	}
}

boolean CH_VID_CheckVidMode(void)
{
	return b_checkvidErrMode;
}

clock_t CH_VID_GetNewChTime(void)
{
	return time_minus(time_now(), newchannel_starttime);
}

/*end of file*/


