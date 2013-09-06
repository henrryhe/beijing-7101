/*****************************************************************************
File Name   : CH_AVSYNBaseline.c

Description : CH_AVSYNBaseline file
Autjoe      : zxg

History     : 2008

Copyright (C) 2008 changhong

Reference   :
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include "stfdma.h" 
#include "stapp_os.h"
#include "stpti.h"
#include "stsys.h"
#include "CH_AVSYNBaseline.h"
#include "stvid.h"

#include "..\main\initterm.h"
#include "channelbase.h"



void CH_PauseIPPlay(void);
void CH_ResmueIPPlay(void);
extern STPTI_Buffer_t BufHandleTemp_swts[2];
extern STPTI_Buffer_t BufHandleTemp_A_swts;

extern STPTI_Slot_t VideoSlot_S,AudioSlot_S,PCRSlot_S;

//#define VID_CALLBACK_DEBUG                    1
int                      PLAY_VideoNbFrames;
boolean              PLAY_VideoOneFrameEvent;
int                      PLAY_PID_Audio;
int                      PLAY_PID_Video;
U32                    PLAY_VideoTimePositionInMs;           /* Video Time code position in the stream     */
U32                    PLAY_VideoTimePTS;                    /* Last video PTS time value latched          */
U32                    PLAY_VideoTimePTS33Bit;               /* Last video PTS time value latched (33bit)  */
U32                    PLAY_AudioTimePositionInMs;           /* Time code position in the stream           */
U32                    PLAY_AudioTimePTS;                    /* Last video PTS time value latched          */
U32                    PLAY_AudioTimePTS33Bit;               /* Last video PTS time value latched (33bit)  */
S32                    AUD_StaticSyncOffset;                 /* Static offset into the audio (ms)          */
U32                    PLAY_VideoFrameRate;                  /* Time of one field in ms                    */
int                    gi_vidindex = 0;
static void CH_VID_AVSYNCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
static void CH_AUD_AVSYNCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p);
boolean b_registervidsynevent =false;
boolean b_registeraudsynevent =false;
boolean b_paused = false;
boolean b_slowvid = false;
boolean b_chspeedAVSyn  = true;
clock_t   g_PuaseTime ;
STCLKRV_Handle_t gst_CLKRVHandle[2];

void CH_SetchspeedAVSyn(boolean rb_handle)
{
 b_chspeedAVSyn = rb_handle;
}

/***************************************************************************
函数名: 		void CH_RegistBaseLINESYNEvent(int ri_VidHandleIndex)
函数作者:	zxg
函数说明: 	注册BLASELIN方式同步处理事件
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
void CH_RegistBaseLINESYNEvent(int ri_VidHandleIndex)
{
	STEVT_DeviceSubscribeParams_t SubscribeParams;
	ST_ErrorCode_t  ErrCode;
	gst_CLKRVHandle[0] = CLKRVHandle;
	
	if(b_registervidsynevent == false)
	{
		memset(&SubscribeParams, 0, sizeof(STEVT_DeviceSubscribeParams_t));	
		SubscribeParams.NotifyCallback = CH_VID_AVSYNCallback;	
		ErrCode = STEVT_SubscribeDeviceEvent ( EVTHandle,  VIDDeviceName[ri_VidHandleIndex], (U32)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT, &SubscribeParams);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STEVT_SubscribeDeviceEvent(STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT) =%s \n", GetErrorText(ErrCode)));
		}

			ErrCode = STEVT_SubscribeDeviceEvent ( EVTHandle,  VIDDeviceName[ri_VidHandleIndex], (U32)STVID_SYNCHRONISATION_CHECK_EVT, &SubscribeParams);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STEVT_SubscribeDeviceEvent(STVID_SYNCHRONISATION_CHECK_EVT) =%s \n", GetErrorText(ErrCode)));
		}

		ErrCode = STEVT_SubscribeDeviceEvent ( EVTHandle,  VIDDeviceName[ri_VidHandleIndex], (U32)STVID_DATA_UNDERFLOW_EVT, &SubscribeParams);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STEVT_SubscribeDeviceEvent(STVID_SYNCHRONISATION_CHECK_EVT) =%s \n", GetErrorText(ErrCode)));
		}


		
		b_registervidsynevent = true;
		gi_vidindex = ri_VidHandleIndex;
	}
	
	
}


/***************************************************************************
函数名: 		void CH_RegistBaseLINEAUDSYNEvent(void)
函数作者:	zxg
函数说明: 	注册BLASELIN方式同步处理事件
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
void CH_RegistBaseLINEAUDSYNEvent(void)
{
	STEVT_DeviceSubscribeParams_t SubscribeParams;
	ST_ErrorCode_t  ErrCode;
	
	
	if(b_registeraudsynevent == false)
	{
		memset(&SubscribeParams, 0, sizeof(STEVT_DeviceSubscribeParams_t));	
		SubscribeParams.NotifyCallback = CH_AUD_AVSYNCallback;	

		ErrCode = STEVT_SubscribeDeviceEvent ( EVTHandle,  AUDDeviceName, (U32)STAUD_AVSYNC_SKIP_EVT, &SubscribeParams);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print(("STEVT_SubscribeDeviceEvent(STVID_SYNCHRONISATION_CHECK_EVT) =%s \n", GetErrorText(ErrCode)));
		}
		
		b_registeraudsynevent = true;
	}
	
	
}

/***************************************************************************
函数名: 		void  CH_UnRegistBaseLINEAUDSYNEvent(void)
函数作者:	zxg
函数说明: 	取消注册注册BLASELIN方式同步处理事件
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
void  CH_UnRegistBaseLINEAUDSYNEvent(void)
{
	ST_ErrorCode_t  ErrCode;
      if(b_registeraudsynevent == true)
      	{
		ErrCode = STEVT_UnsubscribeDeviceEvent(EVTHandle,  AUDDeviceName, (U32)STAUD_AVSYNC_SKIP_EVT);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (( "STEVT_UnsubscribeDeviceEvent(STVID_DISPLAY_NEW_FRAME_EVT) =%s \n", GetErrorText(ErrCode) ));
		}
		
	      b_registeraudsynevent = false;
      	}
}
	
/***************************************************************************
函数名: 		void  CH_UnRegistBaseLINESYNEvent(int ri_VidHandleIndex)
函数作者:	zxg
函数说明: 	取消注册注册BLASELIN方式同步处理事件
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
void  CH_UnRegistBaseLINESYNEvent(int ri_VidHandleIndex)
{
	ST_ErrorCode_t  ErrCode;
      if(b_registervidsynevent == true)
      	{
		ErrCode = STEVT_UnsubscribeDeviceEvent(EVTHandle,  VIDDeviceName[ri_VidHandleIndex], (U32)STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (( "STEVT_UnsubscribeDeviceEvent(STVID_DISPLAY_NEW_FRAME_EVT) =%s \n", GetErrorText(ErrCode) ));
		}
			ErrCode = STEVT_UnsubscribeDeviceEvent(EVTHandle,  VIDDeviceName[ri_VidHandleIndex], (U32)STVID_SYNCHRONISATION_CHECK_EVT);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (( "STEVT_UnsubscribeDeviceEvent(STVID_DISPLAY_NEW_FRAME_EVT) =%s \n", GetErrorText(ErrCode) ));
		}
				ErrCode = STEVT_UnsubscribeDeviceEvent(EVTHandle,  VIDDeviceName[ri_VidHandleIndex], (U32)STVID_DATA_UNDERFLOW_EVT);			
		if (ErrCode != ST_NO_ERROR)
		{
			STTBX_Print (( "STEVT_UnsubscribeDeviceEvent(STVID_DISPLAY_NEW_FRAME_EVT) =%s \n", GetErrorText(ErrCode) ));
		}
	      b_registervidsynevent = false;
      	}
}

/* =======================================================================
   Name:        CH_VID_Callback
   Description: CH_VID_Callback 复位函数，重新换台时候需要调用
   ======================================================================== */

void CH_ResetVID_AYSYNCallback(int ri_vidpid,int i_audpid)
{
   PLAY_PID_Audio = i_audpid;
   PLAY_PID_Video = ri_vidpid;
   PLAY_VideoNbFrames = 0;
   PLAY_VideoOneFrameEvent = false;
   
   PLAY_VideoTimePositionInMs = 0;
   PLAY_VideoTimePTS = 0;
   PLAY_VideoTimePTS33Bit = 0;

  PLAY_AudioTimePositionInMs = 0;
  PLAY_AudioTimePTS = 0;
  PLAY_AudioTimePTS33Bit = 0;
  AUD_StaticSyncOffset = 0/*300*/;
  PLAY_VideoFrameRate=(1000*1000)/(50000); 
   //STAUD_DisableSynchronisation(AUDHandle); 
#if 0
 STAUD_DRSetSyncOffset(AUDHandle,STAUD_OBJECT_DECODER_COMPRESSED0,AUD_StaticSyncOffset);
 STVID_SetSpeed(VIDHandle[gi_vidindex], 100);
 STAUD_DRSetSpeed(AUDHandle,STAUD_OBJECT_INPUT_CD0, 100);

 STAUD_DisableSynchronisation(AUDHandle); 
 STVID_InjectDiscontinuity(VIDHandle[gi_vidindex]);
 #endif

}

/***************************************************************************
函数名: 		static void CH_VID_AVSYNCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)

函数作者:	zxg
函数说明: 	同步处理CH_VID_Callback
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
static void CH_VID_AVSYNCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
 U32                          i;
 ST_ErrorCode_t               ErrCode=ST_NO_ERROR;
 STVID_PictureInfos_t        *VID_PictureInfo;
 STVID_SpeedDriftThreshold_t *VID_SpeedDriftThreshold;
 STVID_DataUnderflow_t       *VID_DataUnderflow;
 
if(EventData == NULL)
	return;

 switch(Event)
  {
   /* New display event */
   /* ----------------- */
   case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT :
    {
     VID_PictureInfo = (STVID_PictureInfos_t *)EventData;
	#ifdef VID_CALLBACK_DEBUG 
     do_report(0,"CH_VID_Callback:PTS=0x%d%08x\n",VID_PictureInfo->VideoParams.PTS.PTS33?1:0,VID_PictureInfo->VideoParams.PTS.PTS);
        #endif
     PLAY_VideoNbFrames++;
     PLAY_VideoOneFrameEvent=TRUE;
#if 0
     if(PLAY_VideoFrameRate == 0)
     	{
     	ST_ErrorCode_t st_error;
     	STVTG_TimingMode_t st_TimingMode;
     	STVTG_ModeParams_t st_ModeParams;
       st_error = STVTG_GetMode(  VTGHandle[VTG_AUX]
                             , &st_TimingMode
                             , &st_ModeParams
                            );
         if(st_error == ST_NO_ERROR)
         	{
                  PLAY_VideoFrameRate = st_ModeParams.FrameRate;
         	}
     	}
    #endif 
	VID_PictureInfo->VideoParams.PTS.PTS = VID_PictureInfo->VideoParams.PTS.PTS /*- 500*90*/;
     /* START - Create a time reference for video */
     if ((PLAY_VideoTimePTS==0)&&(PLAY_VideoTimePTS33Bit==0))
      {
       /* First time, we latch the PTS for the calculation */
       PLAY_VideoTimePTS      = VID_PictureInfo->VideoParams.PTS.PTS;
       PLAY_VideoTimePTS33Bit = VID_PictureInfo->VideoParams.PTS.PTS33;
      }
      {
       /* If new pts < previous pts */
       if ((VID_PictureInfo->VideoParams.PTS.PTS33<PLAY_VideoTimePTS33Bit)||
           ((VID_PictureInfo->VideoParams.PTS.PTS33==PLAY_VideoTimePTS33Bit)&&
            ((VID_PictureInfo->VideoParams.PTS.PTS<PLAY_VideoTimePTS)||((VID_PictureInfo->VideoParams.PTS.PTS-PLAY_VideoTimePTS)>180000))))
        {
      #ifdef VID_CALLBACK_DEBUG  
         do_report(0," New PTS=0x%d%08x<PTS Old=0x%d%08x\n",VID_PictureInfo->VideoParams.PTS.PTS33?1:0,VID_PictureInfo->VideoParams.PTS.PTS,PLAY_VideoTimePTS33Bit,PLAY_VideoTimePTS);
	#endif
         /* We reset the previous PTS with the new one and let's see what happen for next iteration... */
         PLAY_VideoTimePTS      = VID_PictureInfo->VideoParams.PTS.PTS;
         PLAY_VideoTimePTS33Bit = VID_PictureInfo->VideoParams.PTS.PTS33;
        }
       /* When the speed is +, next PTS is always supposed to be higher than previous */
       else
        {
         PLAY_VideoTimePositionInMs+=(VID_PictureInfo->VideoParams.PTS.PTS-PLAY_VideoTimePTS)/90;
        }
      }
      
     /* STOP  - Create a time reference for video */

     /* Synchro is done using PTS  */
      {
       /* Check new valid PTS */
       if ((VID_PictureInfo->VideoParams.PTS.Interpolated==FALSE) && 
           ((PLAY_VideoTimePTS33Bit != VID_PictureInfo->VideoParams.PTS.PTS33) || 
           (PLAY_VideoTimePTS != VID_PictureInfo->VideoParams.PTS.PTS)))
        {
         STCLKRV_ExtendedSTC_t CLKRV_ExtendedSTC;
         CLKRV_ExtendedSTC.BaseBit32=VID_PictureInfo->VideoParams.PTS.PTS33?1:0;
         CLKRV_ExtendedSTC.BaseValue=VID_PictureInfo->VideoParams.PTS.PTS;
         CLKRV_ExtendedSTC.Extension=0;
#ifdef VID_CALLBACK_DEBUG   
       
		 /* Add a delay of -(one field), because the PTS will be available in one field length */
         do_report(0,"Synchro Update with PTS=0x%d%08x\n",VID_PictureInfo->VideoParams.PTS.PTS33?1:0,VID_PictureInfo->VideoParams.PTS.PTS);
#endif
	  ErrCode|=STCLKRV_SetSTCOffset(gst_CLKRVHandle[gi_vidindex],-((PLAY_VideoFrameRate*90000)/1000));
         ErrCode|=STCLKRV_SetSTCBaseline(gst_CLKRVHandle[gi_vidindex],&CLKRV_ExtendedSTC);

         if (ErrCode!=ST_NO_ERROR)
          {
 #ifdef VID_CALLBACK_DEBUG  
                
           do_report(0,"**ERROR** !!! Unable to set the clkrv baseline !!!\n");
#endif
		 }
        }
      }

     /* Latch the new PTS for next iteration */
     PLAY_VideoTimePTS33Bit = VID_PictureInfo->VideoParams.PTS.PTS33;
     PLAY_VideoTimePTS      = VID_PictureInfo->VideoParams.PTS.PTS;


    }
    break;

   /* Synchronisation event */
   /* --------------------- */
   case STVID_SYNCHRONISATION_CHECK_EVT :
    {
     STVID_SynchronisationInfo_t *VID_SynchronisationInfo = (STVID_SynchronisationInfo_t *)EventData;
     /* If the synchro is stable and there is a delay that the video decoder can't recover */
     if ((VID_SynchronisationInfo->IsSynchronisationOk==TRUE) && (VID_SynchronisationInfo->ClocksDifference!=0))
      {
       if (PLAY_PID_Audio!=STPTI_InvalidPid())
        {
 #ifdef VID_CALLBACK_DEBUG  
              
         /* Add delay to the audio decoder in milliseconds */
         do_report(0,"Set audio sync offset to %d ms !\n",AUD_StaticSyncOffset+(VID_SynchronisationInfo->ClocksDifference/90));
#endif
#if defined(ST_7100)||defined(ST_7109)||defined(ST_7200)||defined(ST_5162)
         /* We force it to 0 for now as the time returned by stvid is not stable enough */
         VID_SynchronisationInfo->ClocksDifference=0;
         ErrCode=STAUD_DRSetSyncOffset(AUDHandle,STAUD_OBJECT_DECODER_COMPRESSED0,AUD_StaticSyncOffset+(VID_SynchronisationInfo->ClocksDifference/90));
#else
         ErrCode=STAUD_DRSetSyncOffset(AUDHandle,STAUD_OBJECT_DECODER_COMPRESSED1,AUD_StaticSyncOffset+(VID_SynchronisationInfo->ClocksDifference/90));
#endif
         if (ErrCode!=ST_NO_ERROR)
          {
#ifdef VID_CALLBACK_DEBUG  
                 
           do_report(0,"**ERROR** !!! Unable to set the sync offset in the audio decoder !!!\n");
#endif
		 }
/*		       STCLKRV_SetSTCOffset(gst_CLKRVHandle[gi_vidindex],(-1600 * 90000)/1000);*/

        }
      }
    }
    break;
case STVID_DATA_UNDERFLOW_EVT:

	   break;
   default :
 #ifdef VID_CALLBACK_DEBUG  
         	
    do_report(0,"**ERROR** !!! Event 0x%08x received but not managed !!!\n",Event);
#endif
	break;
  }
}

/***************************************************************************
函数名: 		static void CH_AUD_AVSYNCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)

函数作者:	zxg
函数说明: 	同步处理CH_AUD_AVSYNCallback
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
static void CH_AUD_AVSYNCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
 U32                          i;
 ST_ErrorCode_t               ErrCode=ST_NO_ERROR;

 
if(EventData == NULL)
	return;

 switch(Event)
  {
   
case STAUD_AVSYNC_SKIP_EVT:
	   //CH_PauseIPPlay();
     
	   break;
   default :

	break;
  }
}






/***************************************************************************
函数名: 	void  CH_StartAVSynBaseline(void)
函数作者:	zxg
函数说明: 	开始BLASELIN方式同步处理事件
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
void  CH_StartAVSynBaseline(void)
{

     STCLKRV_SetSTCSource(gst_CLKRVHandle[0],STCLKRV_STC_SOURCE_BASELINE);
	 STCLKRV_InvDecodeClk(gst_CLKRVHandle[0]);
}
/***************************************************************************
函数名: 	void  CH_StartAVSynBaseline(void)
函数作者:	zxg
函数说明: 	开始BLASELIN方式同步处理事件
输入参数:
	无
输出参数:
	无
  ****************************************************************************/
void  CH_StartAVSynPCR(void)
{
     STCLKRV_SetSTCSource(gst_CLKRVHandle[0],STCLKRV_STC_SOURCE_PCR);
	 STCLKRV_InvDecodeClk(gst_CLKRVHandle[0]);
	 CH_UnRegistBaseLINESYNEvent(0);
}


