/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_media.c
  * 描述: 	实现媒体控制相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-24
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"

#include "..\dbase\vdbase.h"
#include "channelbase.h"

#include "eis_include\ipanel_adec.h"
#include "eis_include\ipanel_aout.h"
#include "eis_include\ipanel_avm.h"
#include "eis_include\ipanel_media.h"
#include "eis_include\ipanel_mic.h"
#include "eis_include\ipanel_sound.h"
#include "eis_api_media.h"
#include "eis_api_osd.h"
#include "..\main\initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "string.h"
#include "..\report\report.h"
#include "..\dbase\vdbase.h"
extern U16 vPid ,aPid,pcrPid;

typedef enum
{
	BRIGHTNESSGAIN, 
	CONTRASTGAIN,
	TINTANGLE,
	SATURATIONGAIN    
} PSI_ColorProcessing_t;

/*定义spdif输出模式 sqzow20100730*/
enum
{
	SPDIF_OUT_AC3,
	SPDIF_OUT_PCM
};
static int gi_SpdifOutMode = SPDIF_OUT_PCM;
message_queue_t  *gp_EisIFramePlay_Message = NULL;    
typedef struct eis_IFrame_s
{
	BOOL		IFrame_play_only;	
	CH6_VideoControl_Type VideoControl;
	BOOL AudioControl;
	CH6_AVControl_Type ControlType;	
}eis_IFrame_t;

#ifdef MP3_DATA	
/*MP3播放状态定义*/
typedef enum
{
	MP3_STATUS_UNKOWN_E = 0,/*播放状态未知*/
	MP3_STATUS_PLAY_E,/*正在播放*/
	MP3_STATUS_PAUSE_E,/*暂停*/
	MP3_STATUS_STOP_E,/*停止*/
	MP3_STATUS_OVER_E,/*播放刚刚结束*/
	MP3_STATUS_FREE_E/*空闲*/
}ch_MP3status_e;

typedef void (*IPANEL_PLAYER_EVENT_NOTIFY)(UINT32_T player, INT32_T event, void *param);

#endif

typedef void (*player_event_notify)(UINT32_T player, INT32_T event, void *param);

void EIS_ShowIFrame(void);

int MP3_Open =  0; /* MP3设备是否已经打开1 为已经打开*/

#ifdef MP3_DATA
ch_MP3status_e  MP3_play_state=MP3_STATUS_UNKOWN_E;
#endif

#ifdef IPANEL_AAC_FILE_PLAY

typedef enum
{
	AAC_STATUS_UNKOWN_E = 0,/*播放状态未知*/
	AAC_STATUS_PLAY_E,/*正在播放*/
	AAC_STATUS_PAUSE_E,/*暂停*/
	AAC_STATUS_STOP_E,/*停止*/
	AAC_STATUS_OVER_E,/*播放刚刚结束*/
	AAC_STATUS_FREE_E/*空闲*/
}ch_AACstatus_e;
ch_AACstatus_e  AAC_play_state=AAC_STATUS_UNKOWN_E;
int AAC_Open =  0; /* AAC设备是否已经打开1 为已经打开*/
int AAC_handel=1001;

typedef void (*IPANEL_PLAYER_EVENT_NOTIFY)(UINT32_T player, INT32_T event, void *param);
IPANEL_AUDIO_MIXER_NOTIFY AAC_CallBackFunc;
#endif
//#define AV_SYN_NOPCR

STAUD_StreamContent_t gst_AudioType = STAUD_STREAM_CONTENT_MPEG1;
U8                                  gc_AudioMode   = 1;/*0=STAUD_STEREO_DUAL_MONO,1=STAUD_STEREO_DUAL_LEFT,2=STAUD_STEREO_DUAL_RIGHT,3=STAUD_STEREO_STEREO*/
U8                                  gc_VideoType     = MPEG2_VIDEO_STREAM;

static U8 * eis_IFrame_buf = NULL;/* 用于保存 I_FRAME数据*/
static int 	  eis_IFrame_len;           /* 用于保存 I_FRAME数据 长度*/
static clock_t eis_IFrame_time = 0;
static unsigned int  eis_VDECstart_time = 0;
boolean    gb_DisplayOtherIFram = true;

static       clock_t eis_AUDIOstart_time = 0;

static        int   gi_FirstAudioControl = true;
static        IPANEL_VDEC_IOCTL_e enum_LastVideoControl=IPANEL_VDEC_UNDEFINED;
static        IPANEL_ADEC_IOCTL_e enum_LastAudioControl=IPANEL_ADEC_UNDEFINED;
static        IPANEL_VDEC_IOCTL_e eis_actVideoControl;
static        IPANEL_ADEC_CHANNEL_OUT_MODE_e last_audio_mode=IPANEL_AUDIO_MODE_UNDEFINED;
static 	char gi_IFrameShow = false;/*i帧显示状态 sqzow20100723*/

//#define __EIS_API_MEDIA_DEBUG__


boolean EIS_ResumeAudioPlay(void);
U8 EIS_GetVIdeoType( void );

extern BOOLEAN		bAudioMuteState ; 
IPANEL_AUDIO_MIXER_NOTIFY MP3_CallBackFunc;
int MP3_handel=1000;
pIPANEL_XMEMBLK ipanel_pcmblk;

void CH_StopVidSlot(void)
{
	ST_ErrorCode_t  error;
	STVID_ClearParams_t clearParams;
	STPTI_SlotClearPid( VideoSlot );
	STPTI_SlotClearPid( PCRSlot );

	clearParams.ClearMode        = STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER;
	clearParams.PatternAddress1_p = (void *)NULL;
	clearParams.PatternSize1      = 0;
	clearParams.PatternAddress2_p = (void *)NULL;
	clearParams.PatternSize2      = 0;

	STVID_Clear(VIDHandle[0], &clearParams);
	STVID_Clear(VIDHandle[1], &clearParams);
	task_delay((clock_t)(ST_GetClocksPerSecond()/10));

          
}
void CH_ClearVidBuf(void)
{
	ST_ErrorCode_t  error;
	STVID_ClearParams_t clearParams;
	clearParams.ClearMode        = STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER;

	clearParams.PatternAddress1_p = (void *)NULL;
	clearParams.PatternSize1      = 0;
	clearParams.PatternAddress2_p = (void *)NULL;
	clearParams.PatternSize2      = 0;

	STVID_Clear(VIDHandle[0], &clearParams);
	task_delay((clock_t)(ST_GetClocksPerSecond()/10));

          
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_AudioDelayControl
    Purpose: 换台后延迟数ms打开声音，解决换台或状态切换时的爆音问题
    Reference: 
    in:
        bMute >> -1常规检查 1静音 0关闭静音
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/07/30 10:50:10 
---------------------------------函数体定义----------------------------------------*/
void CH_AudioDelayControl(int Mute)
{
#if 1 	/*sqzow20100730*/
	static U32 uiMuteTime = 0;
	static boolean bMuteStatus = false, bMuteControl = false;
	U32 uiTimeTemp;
	
	if(Mute == 1)
	{
		if(bMuteStatus == false)
		{
			CH6_MuteAudio();
			eis_report("\n我静音了");
			bMuteStatus = true;
		}
		bMuteControl = false;
	}
	else if(Mute == 0)
	{
		if(bMuteControl == false)
		{
			uiMuteTime = get_time_ms();
			bMuteControl = true;
		}
	}
	else
	{
		if(bMuteStatus == true
			&& bMuteControl == true)
		{
			uiTimeTemp = get_time_ms(); 
			if(uiTimeTemp < uiMuteTime
				|| uiTimeTemp - uiMuteTime > 1500)
			{
				if (bAudioMuteState == FALSE ) /*换台后延迟恢复静音，防止爆音 sqzow20100706*/ 
				{
					CH6_UnmuteAudio();
					eis_report("\n我恢复静音了");
				}
				bMuteStatus = false;
			}
		}
	}
#endif
}

void CH_StopAudSlot(void)
{
	STPTI_SlotClearPid( AudioSlot );
	STPTI_SlotClearPid( PCRSlot );
}
/*
功能说明：
	打开一个解码单元。
参数说明：
	输入参数：无
	输出参数：无
返 回：
	！＝IPANEL_NULL：成功，解码器句柄；
	＝＝IPANEL_NULL：失败
*/
UINT32_T ipanel_porting_adec_open(VOID)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_adec_open " );
	#endif

	return IPANEL_OK;
}
/*
功能说明：
	关闭指定的解码单元。
参数说明：
	输入参数：
		decoder: 要关闭的解码单元句柄。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_adec_close(UINT32_T decoder)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_adec_close " );
	#endif
	return IPANEL_OK;	
}

/*
功能说明：
	对decoder 进行一个操作，或者用于设置和获取decoder 设备的参数和属性。
参数说明：
	输入参数：
		decoder: 解码单元句柄。
		op － 操作命令
		typedef enum
		{
			IPANEL_ADEC_SET_SOURCE =1,
			IPANEL_ADEC_START,
			IPANEL_ADEC_STOP,
			IPANEL_ADEC_PAUSE,
			IPANEL_ADEC_RESUME,
			IPANEL_ADEC_CLEAR,
			IPANEL_ADEC_SYNCHRONIZE,
			IPANEL_ADEC_SET_CHANNEL_MODE,
			IPANEL_ADEC_SET_MUTE,
			IPANEL_ADEC_SET_PASS_THROUGH,
			IPANEL_ADEC_SET_VOLUME,
			IPANEL_ADEC_GET_BUFFER_RATE
		} IPANEL_ADEC_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
		op, arg 取值见规范中表：
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败
*/
INT32_T ipanel_porting_adec_ioctl(UINT32_T decoder, IPANEL_ADEC_IOCTL_e op, VOID *arg)
{
	INT32_T ret = IPANEL_OK ; 
	UINT32_T oparg = (UINT32_T)arg;
	boolean AudioControl =false;

	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\ntime =%d, ++>eis ipanel_porting_adec_ioctl op = %d, arg = %d\n" ,get_time_ms(),op, ((int*)arg));
	#endif
	switch(op)
	{
		case IPANEL_ADEC_SET_SOURCE:  /* 指定Audio decoder输入数据来源 */
			break;
			
		case IPANEL_ADEC_START:  /* 启动指定的decoder，并指出输入的数据格式*/
			eis_report("\n>>>>>>>>>IPANEL_ADEC_START  vPid=%d, aPid=%d, pcrPid=%d,gst_AudioType=%d,gc_AudioMode=%d",vPid, aPid, pcrPid,arg,gc_AudioMode);
			#if 0
			CH6_AVControl(VIDEO_DISPLAY, true, CONTROL_AUDIO);
			  #endif
		      if((IPANEL_ADEC_AUDIO_FORMAT_e)(arg) == IPANEL_AUDIO_CODEC_AC3)
			{
                       gst_AudioType = STAUD_STREAM_CONTENT_AC3;
			}
		      else 
			  if((IPANEL_ADEC_AUDIO_FORMAT_e)(arg) == IPANEL_AUDIO_CODEC_AAC)
		  	{
		  	gst_AudioType = STAUD_STREAM_CONTENT_MPEG_AAC;
		  	}
			  else
		      	{
                        gst_AudioType = STAUD_STREAM_CONTENT_MPEG1;
		      	}
#if 1 	/*sqzow20100730*/
			if(gi_SpdifOutMode == SPDIF_OUT_AC3)
			{
				if(gst_AudioType != STAUD_STREAM_CONTENT_AC3)
				{
					if(CH6_GetSPDIFDigtalOutput() != STAUD_DIGITAL_MODE_NONCOMPRESSED)
					{
						CH6_DisableSPDIFDigtalOutput();
					        CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED();
					}
				}
				else
				{
					if(CH6_GetSPDIFDigtalOutput() != STAUD_DIGITAL_MODE_COMPRESSED)
					{
						CH6_DisableSPDIFDigtalOutput();
					        CH6_EnableSPDIFDigtalOutput_COMPRESSED();
					}
				}
			}
#endif
			  
			if(enum_LastVideoControl == IPANEL_VDEC_STOP)
			{
			 	CH_OnlyAudioControl(8191,aPid,true,gst_AudioType,gc_AudioMode);
			}
			else
			{
				CH_OnlyAudioControl(vPid,aPid,true,gst_AudioType,gc_AudioMode);
			}
			
			//CH_OnlyAudioControl(8191,aPid,true,gst_AudioType,gc_AudioMode);						
			eis_AUDIOstart_time=time_now();
			  gi_FirstAudioControl = true;
			  gi_IFrameShow = false;

			  enum_LastAudioControl=IPANEL_ADEC_START;
#if 1 	/*延迟恢复静音 sqzow20100722*/	
			CH_AudioDelayControl(0);
#else
			if (bAudioMuteState == FALSE ) /*换台后恢复静音，防止爆音 sqzow20100706*/ 
			{
				CH6_UnmuteAudio();
				eis_report("\n我恢复静音了");
			}
#endif
#ifdef AV_SYN_NOPCR			
		CH_ResetVID_AYSYNCallback(vPid, aPid);
	        CH_StartAVSynBaseline();
		CH_RegistBaseLINESYNEvent(0);
#endif			
			  
			break; 
			
		case IPANEL_ADEC_STOP:   /* 停止指定的decoder*/
			eis_report("\n>>>>>>>>>>>>>>>>>>>IPANEL_ADEC_STOP  ");
			CH_AudioDelayControl(1);
			CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);
			gi_FirstAudioControl = false;
			enum_LastAudioControl=IPANEL_ADEC_STOP;
			CH_StopAudSlot();
			break;

		case IPANEL_ADEC_PAUSE: /* 暂定已启动的decoder解码单元的解码。*/
			eis_report("\n>>>>>>>>>>>>>>>>>>>IPANEL_ADEC_PAUSE  ");
			CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);
			  gi_FirstAudioControl = false;
			   enum_LastAudioControl=IPANEL_ADEC_PAUSE;
			break;

		case IPANEL_ADEC_RESUME:  /* 恢复已暂定的decoder解码单元的解码。*/
			eis_report("\n>>>>>>>>>>>>>>>>>>>IPANEL_ADEC_RESUME  ");
			CH6_AVControl(VIDEO_DISPLAY, true, CONTROL_AUDIO);
			  gi_FirstAudioControl = true;
			  gi_IFrameShow = false;
			  enum_LastAudioControl=IPANEL_ADEC_RESUME;
			break;

		case IPANEL_ADEC_CLEAR:  /* 清空Decoder解码单元内部缓冲区的内容*/
			eis_report("\n>>>>>>>>>>>>>>>>>>>IPANEL_ADEC_CLEAR  ");
			enum_LastAudioControl=IPANEL_ADEC_CLEAR;
			break;

		case IPANEL_ADEC_SYNCHRONIZE:  /* 禁止和允许视音频同步功能*/
			break;
			
		case IPANEL_ADEC_SET_CHANNEL_MODE:  /* 设置双声道输出模式。*/
			{
			IPANEL_ADEC_CHANNEL_OUT_MODE_e p_Parm;

			p_Parm = ( IPANEL_ADEC_CHANNEL_OUT_MODE_e)(arg);
			eis_report(" \n IPANEL_AUDIO_MODE=%d\n ",p_Parm);

				if(last_audio_mode!=p_Parm)
				{
					last_audio_mode=p_Parm;
			switch ( p_Parm )
			{
			case IPANEL_AUDIO_MODE_STEREO:				/* 立体声*/
						eis_report(" IPANEL_AUDIO_MODE_STEREO ");
				gc_AudioMode = 3;
				break;
			case IPANEL_AUDIO_MODE_LEFT_MONO:			/* 左声道*/
				gc_AudioMode = 2;
						eis_report(" IPANEL_AUDIO_MODE_LEFT_MONO ");
				break;
			case IPANEL_AUDIO_MODE_RIGHT_MONO:		/* 右声道*/
						eis_report(" IPANEL_AUDIO_MODE_RIGHT_MONO ");
				gc_AudioMode = 1;
				break;
			case IPANEL_AUDIO_MODE_MIX_MONO:			/* 左右声道混合*/
				gc_AudioMode = 0;
						eis_report(" IPANEL_AUDIO_MODE_MIX_MONO ");
				break;
			case IPANEL_AUDIO_MODE_STEREO_REVERSE:	/* 立体声，左右声道反转*/
				gc_AudioMode = 3;
						eis_report(" IPANEL_AUDIO_MODE_STEREO_REVERSE ");
				break;
			}

				#if 1
					if(enum_LastAudioControl==IPANEL_ADEC_START)
					{
						eis_report("\n  set PANEL_ADEC_CHANNEL_OUT_MODE--gc_AudioMode=%d-----------",gc_AudioMode);
						if(enum_LastVideoControl == IPANEL_VDEC_STOP)
						{
						 	CH_OnlyAudioControl(8191,aPid,true,gst_AudioType,gc_AudioMode);
						}
						else
						{
							CH_OnlyAudioControl(vPid,aPid,true,gst_AudioType,gc_AudioMode);
						}
						gi_FirstAudioControl = true;
						eis_AUDIOstart_time=time_now();
					}
				#endif
			}
		}
			break;
			
		case IPANEL_ADEC_SET_MUTE:  /* 禁止和允许静音功能。*/
		eis_report("\n>>>>>>>>>>>>>>>>>>>IPANEL_ADEC_SET_MUTE  ");
		  {
				if( time_minus(time_now(),eis_AUDIOstart_time) < (ST_GetClocksPerSecond()))
				{
					CH_UnSubaudioNewfram();
					CH_SubaudioNewfram();
					CH_WaitAudioNewFram(500);
				}
				{
					if (bAudioMuteState == FALSE ) /*MUTE*/
					{
						CH6_MuteAudio();
						bAudioMuteState = TRUE ;
					}
					else
					{  
						//if( time_minus(time_now(),eis_AUDIOstart_time) > (ST_GetClocksPerSecond()))
						CH6_UnmuteAudio();
						bAudioMuteState = FALSE ;			
					}
				}
			}
			
			break;

		case IPANEL_ADEC_SET_PASS_THROUGH:
			if((int)(arg) == 1)/*AC3*/
				{
					gi_SpdifOutMode = SPDIF_OUT_AC3;
			          	CH6_DisableSPDIFDigtalOutput();
			              CH6_EnableSPDIFDigtalOutput_COMPRESSED();

				}
			else            /*PCM*/
				{
					gi_SpdifOutMode = SPDIF_OUT_PCM;
				       CH6_DisableSPDIFDigtalOutput();
				       CH6_EnableSPDIFDigtalOutput_UNCOMPRESSED();
				}
			break;		
			
		case IPANEL_ADEC_SET_VOLUME:
#ifdef __EIS_API_MEDIA_DEBUG__
		eis_report("\n ipanel_porting_adec_ioctl  IPANEL_ADEC_SET_VOLUME %d",(int)arg);
#endif

			break;		

		case IPANEL_ADEC_GET_BUFFER_RATE:
			break;
			
		default:
			break;
	}
	#ifdef __EIS_API_MEDIA_DEBUG__
   	//eis_report ( "\n++>eis ipanel_porting_adec_ioctl end  time=%d ",get_time_ms());
	#endif

	return IPANEL_OK ; 
}

/*
功能说明：
	打开音频输出管理单元。
参数说明：
	输入参数：无
	输出参数：无
返 回：
	！＝IPANEL_NULL：成功，音频输出管理单元句柄；
	＝＝IPANEL_NULL：失败
*/
UINT32_T ipanel_porting_audio_output_open(VOID)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_output_open " );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	关闭指定的音频输出单元。
参数说明：
	输入参数：
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
INT32_T ipanel_porting_audio_output_close(UINT32_T handle)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_output_close " );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	对声音输出设置进行一个操作，或者用于设置和获取声音输出设备的参数和属性。
参数说明：
	输入参数：
		audio： 输出设备句柄
		op － 操作命令
		typedef enum
		{
			IPANEL_AOUT_SET_OUTPUT =1,
			IPANEL_AOUT_SET_VOLUME,
			IPANEL_AOUT_SET_BALANCE,
		} IPANEL_AOUT_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
		op, arg 取值见移植文档中的表：
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_audio_output_ioctl(UINT32_T handle, IPANEL_AOUT_IOCTL_e op, VOID *arg)
{
	INT32_T ret = IPANEL_OK ; 
	UINT32_T oparg = (UINT32_T)arg;
	int 		temp_volume  ;
	static int last_volume;

	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_output_ioctl op=%d" ,op);
	#endif

	switch(op)
	{
		case IPANEL_AOUT_SET_OUTPUT:  /* 指定声音输出设备*/
			break;
			
		case IPANEL_AOUT_SET_VOLUME:  /* 设置音频输出音量*/
			{
				boolean AudioControl =false;
				if((bAudioMuteState == FALSE)&&(last_volume == ((int)arg)))
					break;
				if(gi_FirstAudioControl && time_minus(time_now(),eis_AUDIOstart_time)< (ST_GetClocksPerSecond()))
				{
					//break;
					task_delay(ST_GetClocksPerSecond()/3);
				}
				last_volume = ((int)arg);
				temp_volume = ((int)arg);
				temp_volume=((temp_volume)+3);
				if(temp_volume>63)
					temp_volume=63;
				if(temp_volume<0)
					temp_volume=0;
				
				eis_report("\n ipanel_porting_audio_output_ioctl IPANEL_AOUT_SET_VOLUME ipanel volume %d,real volume %d",((int)arg),temp_volume);
				pstBoxInfo->GlobalVolume=temp_volume;
				
				if(gi_FirstAudioControl == true)
				{
					#if 0
					CH_UnSubaudioNewfram();
					CH_SubaudioNewfram();
					CH_WaitAudioNewFram(100);
					#endif
					AudioControl=true;
					gi_FirstAudioControl = false;
				}
				if (bAudioMuteState == TRUE ) /*MUTE*/
				{  
					STAUD_Mute(AUDHandle,FALSE, FALSE);
					mpeg_set_audio_volume_LR(temp_volume,temp_volume);
					CH_CloseMute();
					bAudioMuteState = FALSE ;			
				}
				else
				{
				      mpeg_set_audio_volume_LR(temp_volume,temp_volume);
				}
				if(AudioControl)
				{
					//CH_OnlyAudioControl(8191,aPid,true,gst_AudioType,gc_AudioMode);
				}
			}
			break;
			
		case IPANEL_AOUT_SET_BALANCE: /* 设置音频输出左右声道均衡参数*/
			break;

		default:
			break;
	}	

	return IPANEL_OK ; 
}

/*
功能说明：
	打开一个视频解码单元。
参数说明：
	输入参数：无
	输出参数：无
	返 回：
		！＝IPANEL_NULL：成功，视频解码单元句柄；
		＝＝IPANEL_NULL：失败
*/
UINT32_T ipanel_porting_vdec_open(VOID)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_vdec_open " );
	#endif
	CH6_AVControl(VIDEO_DISPLAY, true, CONTROL_VIDEO);
	return IPANEL_OK;
}

/*
功能说明：
	关闭指定的解码单元。
参数说明：
	输入参数：
	decoder：要关闭的解码单元句柄
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_vdec_close(UINT32_T decoder)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_vdec_close" );
	#endif
	CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEO);
	return IPANEL_OK;	
}

/*
功能说明：
	对decoder 进行一个操作，或者用于设置和获取decoder 设备的参数和属性。
参数说明：
	输入参数：
		decoder： 解码单元句柄
		op － 操作命令
		typedef enum
		{
			IPANEL_VDEC_SET_SOURCE =1,
			IPANEL_VDEC_START,
			IPANEL_VDEC_STOP,
			IPANEL_VDEC_PAUSE,
			IPANEL_VDEC_RESUME,
			IPANEL_VDEC_CLEAR,
			IPANEL_VDEC_SYNCHRONIZE,
			IPANEL_VDEC_GET_BUFFER_RATE,
			IPANEL_VDEC_PLAY_I_FRAME,
			IPANEL_VDEC_STOP_I_FRAME，
			IPANEL_VDEC_SET_STREAM_TYPE,
		} IPANEL_VDEC_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
		op, arg 取值见移植文档中表：
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
boolean gb_h264play = true;

INT32_T ipanel_porting_vdec_ioctl(UINT32_T decoder,IPANEL_VDEC_IOCTL_e op, VOID *arg)
{
	INT32_T ret = IPANEL_OK ; 
	UINT32_T oparg = (UINT32_T)arg;
#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report("\n ipanel_porting_vdec_ioctl been called op=%d,time=%d, arg = %d\n",op,get_time_ms(), ((int*)arg));
#endif
  
	eis_actVideoControl = op;

	switch(op)
	{
		case IPANEL_VDEC_SET_SOURCE:
			break;
			
		case IPANEL_VDEC_START:
			eis_report("\n VideoType=%d ",arg);
			switch((int)arg)
			{
				case IPANEL_VIDEO_STREAM_MPEG1:
					gc_VideoType=MPEG1_VIDEO_STREAM;
					break;
				case IPANEL_VIDEO_STREAM_MPEG2:
					gc_VideoType=MPEG2_VIDEO_STREAM;
					break;
				case IPANEL_VIDEO_STREAM_H264:
					gc_VideoType=H264_VIDEO_STREAM;
					break;
					
				default:
					gc_VideoType=MPEG1_VIDEO_STREAM;
					break;
			}
			eis_VDECstart_time=get_time_ms();
			CH_SetVideoAspectRatioConversion(CH_GetSTVideoARConversionFromCH(pstBoxInfo->VideoARConversion));/*20070813 add*/

			//aPid = 1211;
			
			eis_report("\n IPANEL_VDEC_START  vPid=%d, aPid=%d, pcrPid=%d,gst_AudioType=%d,gc_AudioMode=%d",vPid, aPid, pcrPid,arg,gc_AudioMode);

			 if((enum_LastVideoControl==IPANEL_VDEC_CLEAR)||(enum_LastVideoControl==IPANEL_VDEC_PLAY_I_FRAME))
		 	{
		 		CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEO);
		 	}
#if 0
			if(gc_VideoType == H264_VIDEO_STREAM && true == gb_h264play)
			{
				gb_h264play = false;
				
				do_report(0,"\n 第一次调用播放H264\n");
				
				//ipanel_porting_graphics_set_alpha(40);

				CH_OnlyVideoControl_xx(vPid,true,pcrPid,H264_VIDEO_STREAM);
				CH6_AVControl(VIDEO_DISPLAY, true, CONTROL_VIDEO);
			
			}

#else
			if(gc_VideoType == H264_VIDEO_STREAM && true == gb_h264play)
			{
				gb_h264play = false;

				do_report(0,"\n 第一次调用播放H264\n");

				//ipanel_porting_graphics_set_alpha(40);
				CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEO);
				CH_OnlyVideoControl(vPid,true,pcrPid,gc_VideoType);
			}
#endif	
			else
			{
				do_report(0,"\n 调用之前播放的接口\n");
			
				CH_OnlyVideoControl(vPid,true,pcrPid,gc_VideoType);
			}

			enum_LastVideoControl = IPANEL_VDEC_START;

#ifdef AV_SYN_NOPCR			
			CH_ResetVID_AYSYNCallback(vPid, aPid);
			CH_StartAVSynBaseline();
			CH_RegistBaseLINESYNEvent(0);
#endif			
				break; 

		case IPANEL_VDEC_STOP: 
			eis_VDECstart_time=get_time_ms();
			#if 1
			{
				 CH_StopVidSlot();
				if((gc_VideoType==MPEG1_VIDEO_STREAM)||(gc_VideoType==MPEG2_VIDEO_STREAM)||(gc_VideoType==H264_VIDEO_STREAM))
				{
				 	CH_VID_SetMode();
				}
				if(arg == 0)
				{
					CH6_AVControl(VIDEO_FREEZE, false, CONTROL_VIDEO);
					eis_report("\n test print   IPANEL_VDEC_STOP VIDEO_FREEZE");
				}
				else
				{
					eis_report("\n test print    IPANEL_VDEC_STOP VIDEO_BLACK");
					CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEO);
				}
			}
			
			#endif
			enum_LastVideoControl = IPANEL_VDEC_STOP;

			break; 

		case IPANEL_VDEC_PAUSE:
			CH6_AVControl(VIDEO_FREEZE, false, CONTROL_VIDEO);
			enum_LastVideoControl = IPANEL_VDEC_PAUSE;
			break;

		case IPANEL_VDEC_RESUME:
			CH6_AVControl(VIDEO_DISPLAY, true, CONTROL_VIDEO);
			enum_LastVideoControl = IPANEL_VDEC_RESUME;
			break;

		case IPANEL_VDEC_CLEAR:
			#if 1		
			if((NULL!=eis_IFrame_buf)&&(eis_IFrame_len>0))
			{
			       if((enum_LastVideoControl == IPANEL_VDEC_RESUME || enum_LastVideoControl == IPANEL_VDEC_START))
				{
#ifdef __EIS_API_MEDIA_DEBUG__
					eis_report("\n test print    IPANEL_VDEC_CLEAR VIDEO_SHOWPIC_AUTORECOVERY");
#endif
					if(EIS_GetVIdeoType() != H264_VIDEO_STREAM ||false == CH_TUNER_IsLocked() )
					{
						CH6_AVControl(VIDEO_SHOWPIC_AUTORECOVERY,false,CONTROL_VIDEO);
					}
				
				}
				else
				{
#ifdef __EIS_API_MEDIA_DEBUG__
					eis_report("\n test print    IPANEL_VDEC_CLEAR VIDEO_SHOWPIC");
#endif
					CH6_AVControl(VIDEO_SHOWPIC,false,CONTROL_VIDEO);
				}
				   gb_DisplayOtherIFram = false;
			}			
			#endif
			break;

		case IPANEL_VDEC_SYNCHRONIZE:
			break;
		
		case IPANEL_VDEC_GET_BUFFER_RATE:
			break;
		case IPANEL_VDEC_PLAY_I_FRAME:
			
			{
				IPANEL_IOCTL_DATA *p_Param;
				  boolean b_IframChange  = true;
                            if(arg != NULL)
                            {
						p_Param = (IPANEL_IOCTL_DATA *)arg;
						if(p_Param->data == NULL || p_Param->len == 0)
						{
                                             break;
						}
	                                  semaphore_wait(gp_EisSema);		
						/*先和上次备份IFrame数据比较如果相同不显示*/			  
						if((NULL != eis_IFrame_buf))
						{
							b_IframChange=memcmp(eis_IFrame_buf,p_Param->data,p_Param->len) ;
							if(b_IframChange== 0 && (time_minus(time_now(),eis_IFrame_time) < (ST_GetClocksPerSecond()/10)))
							{
								semaphore_signal(gp_EisSema);
								#ifdef __EIS_API_MEDIA_DEBUG__
								eis_report("\n test print    IPANEL_VDEC_PLAY_I_FRAME 与上次I帧相同不显示");
								#endif
								break;
							}
						}
						
						/*备份IFrame数据*/
						if(NULL == eis_IFrame_buf)
						{
							eis_IFrame_buf = (U8*)ipanel_porting_malloc(0x80000);
						}
						
						if((NULL != eis_IFrame_buf)&&(b_IframChange!=0)&&(p_Param->len<0x80000))
						{
							memcpy(eis_IFrame_buf,p_Param->data,p_Param->len);
							eis_IFrame_len=p_Param->len;
							gb_DisplayOtherIFram = true;
						}
						
						semaphore_signal(gp_EisSema);	
						 if(enum_LastVideoControl == IPANEL_VDEC_RESUME || enum_LastVideoControl == IPANEL_VDEC_START)
							{
							#ifdef __EIS_API_MEDIA_DEBUG__
							eis_report("\n test print    IPANEL_VDEC_PLAY_I_FRAME 正在播放不用显示");
							#endif
						}
						 else
							{
							CH6_AVControl(VIDEO_SHOWPIC,false,CONTROL_VIDEO);
							 gb_DisplayOtherIFram = false;
							eis_IFrame_time = time_now();
							#ifdef __EIS_API_MEDIA_DEBUG__
							eis_report("\n test print    IPANEL_VDEC_PLAY_I_FRAME 显示I帧IFRAME len=%d",eis_IFrame_len);
							#endif
						 }
					
						
                            }
				        
			}	
			 BootSETHDVideo();
			break;

		case IPANEL_VDEC_STOP_I_FRAME:
			if(enum_LastVideoControl == IPANEL_VDEC_RESUME || enum_LastVideoControl == IPANEL_VDEC_START)
			{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report("\n test print    IPANEL_VDEC_STOP_I_FRAME 正在播放不用显示只清理内存");
			#endif
			}
			else
			{
			CH6_AVControl(VIDEO_BLACK, true, CONTROL_VIDEO);
			 gb_DisplayOtherIFram = true;
			 #ifdef __EIS_API_MEDIA_DEBUG__
			 eis_report("\n test print    IPANEL_VDEC_STOP_I_FRAME 清理I帧黑屏");
			 #endif
			}
			 semaphore_wait(gp_EisSema);
			memset(eis_IFrame_buf,0,eis_IFrame_len);
			semaphore_signal(gp_EisSema);	
			
			break;

		default:
			break;
	}
	#ifdef __EIS_API_MEDIA_DEBUG__
   	eis_report ( "\n++>eis ipanel_porting_vdec_ioctl end  time=%d ",get_time_ms());
	#endif
	return ret ; 
}

/*
功能说明：
	打开一个显示单元。
参数说明：
	输入参数：无
	输出参数：无
	返 回：
		！＝IPANEL_NULL：成功，显示单元句柄；
		＝＝IPANEL_NULL：失败
*/
UINT32_T ipanel_porting_display_open (VOID)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_display_open " );
	#endif

	return IPANEL_OK;	
}

/*
功能说明：
	关闭一个显示管理单元。
参数说明：
	输入参数：无
	输出参数：无
	返 回：
		！＝IPANEL_NULL：成功，显示管理单元句柄；
		＝＝IPANEL_NULL：失败
*/
INT32_T ipanel_porting_display_close(UINT32_T display)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_display_close " );
	#endif

	return IPANEL_OK;	
}

/*
功能说明：
	在指定的显示管理单元上的创建一个窗口。
参数说明：
	输入参数：
		display： 一个显示单元管理句柄
		type： 窗口的类型，只支持0，表示视频窗口。
	输出参数：无
	返 回：
		！＝IPANEL_NULL：成功，窗口句柄；
		＝＝IPANEL_NULL：失败
*/
UINT32_T ipanel_porting_display_open_window(UINT32_T display, INT32_T type)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_display_open_window " );
	#endif

	  return 1;	
}

/*
功能说明：
	关闭指定的窗口。
参数说明：
	输入参数：
		display: 显示单元管理句柄。
		window： 要关闭的窗口句柄。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_display_close_window(UINT32_T display,UINT32_T window)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_display_close_window " );
	#endif
	CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEOANDAUDIO);
	return IPANEL_OK;	
}

/*
功能说明：
	对显示设备进行一个操作，或者用于设置和获取显示设备的参数和属性。
参数说明：
	输入参数：
		display：显示管理单元句柄
		op － 操作命令
		typedef enum
		{
			IPANEL_DIS_SELECT_DEV =1,
			IPANEL_DIS_ENABLE_DEV,
			IPANEL_DIS_SET_MODE,
			IPANEL_DIS_SET_VISABLE,
			IPANEL_DIS_SET_ASPECT_RATIO,
			IPANEL_DIS_SET_WIN_LOCATION,
			IPANEL_DIS_SET_WIN_TRANSPARENT,
			IPANEL_DIS_SET_CONTRAST,
			IPANEL_DIS_SET_HUE,
			IPANEL_DIS_SET_BRIGHTNESS,
			IPANEL_DIS_SET_SATURATION,
			IPANEL_DIS_SET_SHARPNESS,
		} IPANEL_DIS_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_display_ioctl(UINT32_T display, IPANEL_DIS_IOCTL_e op, VOID *arg)
{
	INT32_T ret = IPANEL_OK ; 
	UINT32_T  oparg = (UINT32_T)arg;
	int temp;

	eis_report("\n ipanel_porting_display_ioctl been called,op is %d",op);
	#ifdef __EIS_API_MEDIA_DEBUG__
	#endif
		
	switch(op)
	{		
		case IPANEL_DIS_SELECT_DEV:   /* 设置视频输出设备*/
			break ; 
			
		case IPANEL_DIS_ENABLE_DEV :  /* 控制是否在相应的接口设备上输出信号*/
			break ; 
			
	case IPANEL_DIS_SET_MODE:    /* 设置视频输出转换模式*/
		
		  
			break ; 
			
		case IPANEL_DIS_SET_VISABLE:  /* 设置视频窗口的可见性*/
			break ; 
			
		case IPANEL_DIS_SET_ASPECT_RATIO: /* 设置视频的纵横比和显示设备纵横比 */
			  if((int)(arg) == IPANEL_DIS_AR_FULL_SCREEN)
			  	{
				  	if(CONVERSION_FULLSCREEN != pstBoxInfo->VideoARConversion)
				  	{
					   pstBoxInfo->VideoARConversion = CONVERSION_FULLSCREEN;
					   
                       		   CH_NVMUpdateByAddress(idNvmBoxInfoId,&pstBoxInfo->VideoOutputType,40);
				  	}
			     }
			  else                            
			  	{
                                  if(CONVERSION_LETTETBOX != pstBoxInfo->VideoARConversion)
				  	{
					   pstBoxInfo->VideoARConversion = CONVERSION_LETTETBOX;
					  
                       		   CH_NVMUpdateByAddress(idNvmBoxInfoId,&pstBoxInfo->VideoOutputType,40);
				  	}
			  	}	
			break ; 
			
		case IPANEL_DIS_SET_WIN_LOCATION:  /* 设置视频窗口的 */
			#if 1
			{
				IPANEL_RECT *rect  = 	(IPANEL_RECT *)oparg;
				int x,y,w,h;

				x = (rect->x+EIS_REGION_STARTX+2)/2*2;
				y = (rect->y+EIS_REGION_STARTY)/2*2;
				w = rect->w/2*2;
				h = rect->h/2*2;
				eis_report("\n ipanel_porting_display_ioctl %d  %d %d %d",x,y,w,h);

				if( x < 0 )
					x = 0;
				if( y < 0 )
					y = 0;
				
				if(w ==0||h==0)
					CH6_SetVideoWindowSize(false, x, y, w, h);
				else
					CH6_SetVideoWindowSize(true, x, y, w, h);
	
			}
			#endif
			break ; 
			
		case IPANEL_DIS_SET_WIN_TRANSPARENT :  /* 设置透明度*/
			break ; 
			
		case IPANEL_DIS_SET_CONTRAST:  /* 设置图像对比度*/
			if(pstBoxInfo->abiTVContrast != (int)(arg))
			{
			eis_report ( "\n set CONTRASTGAIN>>>>>>>>>>>>>>>arg=%d ",(int)(arg) );
			 pstBoxInfo->abiTVContrast=(int)(arg);	
			temp=Con_ConvertHD(pstBoxInfo->abiTVContrast);
			ColorProcessingTest(CONTRASTGAIN, temp);
			CH_NVMUpdate( idNvmBoxInfoId);
			}
			break ; 

			
		case IPANEL_DIS_SET_HUE:   /* 设置图像色调*/
			if(pstBoxInfo->abiTVSaturation != (int)(arg))
			{
			eis_report ( "\n set SATURATIONGAIN>>>>>>>>>>>>>>>arg=%d ",(int)(arg) );
			 pstBoxInfo->abiTVSaturation=(int)(arg);
			temp=Chr_ConvertHD(pstBoxInfo->abiTVSaturation);
			 ColorProcessingTest(SATURATIONGAIN,temp);
			 CH_NVMUpdate( idNvmBoxInfoId);
			}
			break ; 
			
		case IPANEL_DIS_SET_BRIGHTNESS:  /* 设置图像亮度 */
			if(pstBoxInfo->abiTVBright!=(int)(arg))
			{
			eis_report ( "\n set BRIGHTNESSGAIN>>>>>>>>>>>>>>>arg=%d ",(int)(arg) );
			pstBoxInfo->abiTVBright=(int)(arg);
			temp=Lum_ConvertHD(pstBoxInfo->abiTVBright);
			ColorProcessingTest(BRIGHTNESSGAIN,temp);
			CH_NVMUpdate( idNvmBoxInfoId);
			}
			break ; 
			
		case IPANEL_DIS_SET_SATURATION:  /* 设置图像饱和度*/
			break ; 
			
		case IPANEL_DIS_SET_SHARPNESS:  /* 设置图像锐度 */
			break ; 
              case IPANEL_DIS_SET_HD_RES:/*设置输出分辨率*/
			 {
			 	int VideoMode=(int)arg;
				switch(VideoMode)
				{
					case IPANEL_DIS_HD_RES_1080I:
					  	{
						  	if(MODE_1080I50 != pstBoxInfo->HDVideoTimingMode)
						  	{
							   pstBoxInfo->HDVideoTimingMode = MODE_1080I50;
							   if(CH_SetVideoOut(pstBoxInfo->HDVideoOutputType,pstBoxInfo->HDVideoTimingMode,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->VideoARConversion))
							   {
								   STTBX_Print(("\nYxlInfo:CH_SetVideoOut() isn't successful"));						
							   }
							   CH_RstoreALLOSD();
		                       		   CH_NVMUpdateByAddress(idNvmBoxInfoId,&pstBoxInfo->VideoOutputType,40);
						  	}
					     }
						break;
					case IPANEL_DIS_HD_RES_720P:
					  	{
		                                  if(MODE_720P50 != pstBoxInfo->HDVideoTimingMode)
						  	{
							   pstBoxInfo->HDVideoTimingMode = MODE_720P50;
							   if(CH_SetVideoOut(pstBoxInfo->HDVideoOutputType,pstBoxInfo->HDVideoTimingMode,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->VideoARConversion))
							   {
								   STTBX_Print(("\nYxlInfo:CH_SetVideoOut() isn't successful"));						
							   }
							    CH_RstoreALLOSD();
		                       		   CH_NVMUpdateByAddress(idNvmBoxInfoId,&pstBoxInfo->VideoOutputType,40);
						  	}
					  	}
						break;
					case IPANEL_DIS_HD_RES_576P:
					  	{
		                                  if(MODE_576P50 != pstBoxInfo->HDVideoTimingMode)
						  	{
							   pstBoxInfo->HDVideoTimingMode = MODE_576P50;
							   if(CH_SetVideoOut(pstBoxInfo->HDVideoOutputType,pstBoxInfo->HDVideoTimingMode,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->VideoARConversion))
							   {
								   STTBX_Print(("\nYxlInfo:CH_SetVideoOut() isn't successful"));						
							   }
							    CH_RstoreALLOSD();
		                       		   CH_NVMUpdateByAddress(idNvmBoxInfoId,&pstBoxInfo->VideoOutputType,40);
						  	}
					  	}
						break;
					default:
						break;
				}
              	}
			break;
		default:
			break;
	}

	return ret ; 
}

/*
功能说明：
	打开一个PCM 播放设备实例，为了解决资源冲突问题，特约定（假定只存在一个硬件
	设备的情况）：
	当已存在实例是以独占方式（IPANEL_DEV_USE_EXCUSIVE）打开时，后续的所有打开操
	作都不成功
	当以前的实例都是以共享方式（IPANEL_DEV_USE_SHARED）打开时，可以再打开共享实
	例和独占实例。如果所有都是共享实例则按先入先出方式播放，如果新实例是独占方式打开，
	则以前所有共享实例的输出将被丢弃或阻塞。
参数说明：
	输入参数：
		mode： 使用设备的方式。
		typedef enum
		{
		IPANEL_DEV_USE_SHARED, /和其他用户共享使用设备/
		IPANEL_DEV_USE_EXCUSIVE, /独占使用设备/
		}IPANEL_DEV_USE_MODE;
		func： 用户实现的事件通知函数指针，用户也可以不传入指针，这时func 必须
		是IPANEL_NULL（0）。
		VOID (*IPANEL_AUDIO_MIXER_NOTIFY)(UINT32_T hmixer,
		IPANEL_AUDIO_MIXER_ENENT event, UINT32_T *param)
		AUDIO_DATA_CONSUMED：param 指向处理完的数据块描叙符，如果
		数据块描叙符，在底层释放掉，则param 为空指针。
		AUDIO_DATA_LACK：param 为空指针。
	输出参数: 无
	返回值:无
	输出参数：无
	返 回：
		！＝IPANEL_NULL：成功，PCM 播放设备单元句柄；
		＝＝IPANEL_NULL：失败
*/

UINT32_T ipanel_porting_audio_mixer_open(IPANEL_DEV_USE_MODE mode, IPANEL_AUDIO_MIXER_NOTIFY func)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_mixer_open " );
	#endif
	
#ifdef MP3_DATA	
	return IPANEL_NULL;
#else
	if(MP3_Open ==  0)	
	{
		extern ST_Partition_t *SystemPartition;
		MP3_Open =1;
		CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);/*关闭音频*/
		CH_AudioUnLink();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
		CH_MP3_PCMDecodeInit();
        //如果静音状态保持静音
        if (bAudioMuteState == FALSE)
        {
		    CH6_UnmuteAudio();/*wz add MP3声音控制*/
        }
	}
	MP3_CallBackFunc = func;
	return MP3_handel;
#endif
}

/*
功能说明：
	关闭通过ipanel_porting_audio_mixer_open 打开的PCM 播放设备实例，同时要释放可能
	存在的待处理数据块。
参数说明：
	输入参数：
		handle： PCM 播放设备实例句柄。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_audio_mixer_close(UINT32_T handle)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_mixer_close " );
	#endif
#ifdef MP3_DATA	
	return IPANEL_ERR;
#else
	if(MP3_Open)
	{
		CH_MP3_ExitPlayer();
		CH_AudioUnLink();
		CH_AudioLink();
		EIS_ResumeAudioPlay();
		CH6_AVControl(VIDEO_DISPLAY,TRUE,CONTROL_AUDIO); 
		MP3_Open =0;
		MP3_CallBackFunc=NULL;
	}
	return IPANEL_OK;
#endif
}

/*
功能说明：
	从PCM 播放设备获取内存块
参数说明：
	输入参数：
		handle：PCM 播放设备实例句柄
		size： 需要的内存块大小。
	输出参数：无
	返 回：
		!=IPANEL_NULL:返回IPANEL_XMEMBLK 结构，底层分配的内存指针放在这个结
		构中返回；
		==IPANEL_NULL:失败
*/
pIPANEL_XMEMBLK ipanel_porting_audio_mixer_memblk_get(UINT32_T handle,UINT32_T size)
{
	static IPANEL_XMEMBLK  temp;
	extern void CH_MP3_MemoryDeallocate(void* pvMemory);
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_mixer_memblk_get " );
	#endif
	
#ifdef MP3_DATA	
	return IPANEL_NULL;
#else
	
	temp.pbuf=(UINT32_T  *)CH_MP3_MemoryAllocate(size);
	if(temp.pbuf!=IPANEL_NULL)
	{
		temp.pfree=CH_MP3_MemoryDeallocate;
		temp.len=size;
		return &temp;
	}
	else
	{
		return IPANEL_NULL;
	}
#endif
}

/*
功能说明：
	向PCM 播放设备实例句柄发送音频数据，该函数必须立即返回。IPANEL_XMEMBLK
	包含有实际的数据。而由实际播放的进程/线程来处理该数据。
参数说明：
	输入参数:
		handle –PCM 播放设备实例句柄
		pcmblk - IPANEL_XMEMBLK 结构体见前页：
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_audio_mixer_memblk_send(UINT32_T handle,pIPANEL_XMEMBLK pcmblk)
{
	pIPANEL_PCMDES pcm_des = NULL;
	#ifdef __EIS_API_MEDIA_DEBUG__
	//eis_report ( "\n++>eis ipanel_porting_audio_mixer_memblk_send " );
	#endif
#ifdef MP3_DATA	
	return IPANEL_ERR;
#else
	
	if(NULL != pcmblk)
	{
		if(pcmblk->pdes)
		{
			pcm_des = (pIPANEL_PCMDES)pcmblk->pdes;
			CH_SetPCMPara(pcm_des->samplerate,pcm_des->bitspersample,pcm_des->channelnum);
		}
		ipanel_pcmblk=pcmblk;
		if((pcmblk->pbuf) && (pcmblk->len>0))
		{
			if(CH_IPANEL_MP3Start(pcmblk->pbuf, pcmblk->len))
			{
				return IPANEL_OK;
			}
			else
			{
				return IPANEL_ERR;
			}
		}
	}
	return IPANEL_OK;
#endif
}

/*
功能说明：
	对PCM 播放设备实例句柄进行一个操作，或者用于设置和获取PCM 播放设备实例句
	柄的参数和属性。
参数说明：
	输入参数：
		handle –PCM 播放设备实例句柄
		op － 操作命令
		typedef enum
		{
			IPANEL_MIXER_SET_VOLUME =1,
			IPANEL_MIXER_CLEAR_BUFFER
		} IPANEL_MIXER_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换成对
		应数据类型。
		op 和arg 的取值关系见下表：
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败
*/
INT32_T ipanel_porting_audio_mixer_ioctl(UINT32_T handle, IPANEL_MIXER_IOCTL_e op, VOID *arg)
{
	int temp_volume;
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_audio_mixer_ioctl  " );
	#endif
	switch(op)
	{
		case IPANEL_MIXER_SET_VOLUME:
			eis_report ( " IPANEL_MIXER_SET_VOLUME  " );
				temp_volume = ((int)arg);
				temp_volume=((temp_volume)+3);
				if(temp_volume>63)
					temp_volume=63;
				if(temp_volume<0)
					temp_volume=0;
				 mpeg_set_audio_volume_LR(temp_volume,temp_volume);
			break;
		case IPANEL_MIXER_CLEAR_BUFFER:
			eis_report ( " IPANEL_MIXER_CLEAR_BUFFER  " );
			
			ipanel_mediaplayer_close(0); 
			
			
			//ipanel_mediaplayer_stop(0);
			//CH_CleanBuf();
			break;
		case IPANEL_MIXER_PAUSE:
			eis_report ( " IPANEL_MIXER_PAUSE  " );
			CH_MP3_Pause();
			break;
		case IPANEL_MIXER_RESUME:
			eis_report ( " IPANEL_MIXER_RESUME  " );
			CH_MP3_Resume();
			break;
		default:
			break;
	
	}
	return IPANEL_OK;
}

void CH_IPANEL_MP3CallBack(void)
{
#ifdef IPANEL_AAC_FILE_PLAY
		if(AAC_CallBackFunc)
		{
			AAC_CallBackFunc(AAC_handel,IPANEL_AUDIO_DATA_CONSUMED,(UINT32_T*)(ipanel_pcmblk));
			do_report(0,"\n   播放下一段");
		}
	else
#endif
	if(MP3_CallBackFunc)
	{
		MP3_CallBackFunc(MP3_handel,IPANEL_AUDIO_DATA_CONSUMED,(UINT32_T*)(ipanel_pcmblk));
	}
}

/*
功能说明：
	打开一个声音采集设备实例。
参数说明：
输入参数：
	Func － 用户实现的事件通知函数指针，用户也可以不传入指针，这时func 必须
	是IPANEL_NULL（0）。
	typedef void (*IPANEL_MIC_NOTIFY)( UINT32_T handle, INT32_T event, VOID *param)
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
UINT32_T ipanel_porting_mic_open(IPANEL_MIC_NOTIFY func)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_mic_open 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	关闭通过ipanel_porting_mic_open 打开的声音采集设备实例，同时要释放可能存在的
	待处理数据块。
参数说明：
	输入参数：
		handle –声音采集设备实例句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_mic_close(UINT32_T handle)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_mic_close 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	对声音采集设备实例进行一个操作，或者用于设置和获取声音采集设备实例的参数和属
	性。
参数说明：
	输入参数：
		handle –声音采集设备实例句柄
		op － 操作命令
		typedef enum
		{
			IPANEL_MIC_START=1,
			IPANEL_MIC_STOP,
			IPANEL_MIC_CLEAR_BUFFER,
			IPANEL_MIC_SET_PARAM
		} IPANEL_MIC_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换成对
		应数据类型。
		op 和arg 的取值关系见下表：
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_mic_ioctl(UINT32_T handle, IPANEL_MIC_IOCTL_e op, VOID *arg)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_mic_ioctl 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	读取声音采样数据。
参数说明：
	输入参数：
		handle –声音采集设备实例句柄
		buf － 指向数据块的指针
		len － buf 的长度
	输出参数：
		buf：读取的采样数据
	返 回：
		>=0: 函数执行成功,返回读取的数据长度;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_mic_read(UINT32_T handle,BYTE_T *buf, UINT32_T len)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_mic_read 未完成" );
	#endif

	return IPANEL_OK;
}

/*
* 功能说明：
	播放一个音视频service，srv 中传递的是一个service 的相关信息，如果service
	是CA 加扰的，实现者应该将CA 相关的信息传递给CA 模块。
参数说明：
	输入参数：
		srv：节目信息结构体指针；
		typedef struct
		{
			UINT16_T service_id;
			BYTE_T serviename[128];
			UINT16_T video_pid;
			UINT16_T audio_pid;
			UINT16_T pcr_pid;
			// 同密情况下, PMT common loop 会有多个CA_descriptor 
			UINT16_T pmt_pid;
			/ PMT PID(一些CA 要从PMT 的PID 开始, ECM/EMM 的PID 无用!) /
			UINT16_T ecmPids[IPANEL_CA_MAX_NUM];
			UINT16_T ecmCaSysIDs[IPANEL_CA_MAX_NUM];
			UINT16_T audioEcmPids[IPANEL_CA_MAX_NUM];
			UINT16_T audioCaSysIDs[IPANEL_CA_MAX_NUM];
			UINT16_T videoEcmPids[IPANEL_CA_MAX_NUM];
			UINT16_T videoCaSysIDs[IPANEL_CA_MAX_NUM];
		} IPANEL_SERVICE_INFO;
	#define IPANEL_CA_MAX_NUM 8
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
  */
INT32_T ipanel_avm_play_srv(IPANEL_SERVICE_INFO *srv)
{
#if 0
	TRANSPONDER_INFO_STRUCT 	t_XpdrInfo;
eis_report("\n ipanel_avm_play_srv freq=%d, symbolrate=%d,video_pid=%d, audio_pid=%d, pcr_pid=%d\n ",
	srv->stansport.transport.dvbc.tuningfreqhz,srv->stansport.transport.dvbc.symbolrate,
	srv->video_pid,srv->audio_pid,srv->pcr_pid);
#if 1
	eis_report ( "\n++>eis ipanel_avm_play_srv,video_pid=%x,audio_pid=%x,pcr_pid=%x" ,srv->video_pid,srv->audio_pid,srv->pcr_pid);
	t_XpdrInfo.iTransponderFreq 	= srv->stansport.transport.dvbc.tuningfreqhz / 10;
	t_XpdrInfo.iSymbolRate			= srv->stansport.transport.dvbc.symbolrate / 10;
	t_XpdrInfo.ucQAMMode			= 3/*smodulation*/;
	//CHDVBNewTune0Req(&t_XpdrInfo);

	CH_UpdatesCurTransponderId ( -1 );
#endif
	CH6_NewChannel(srv->video_pid,srv->audio_pid,srv->pcr_pid,true,true,STAUD_STREAM_CONTENT_MPEG1,MPEG1_VIDEO_STREAM);
	//CH6_NewChannel(0x204,0x308,srv->pcr_pid,true,true,STAUD_STREAM_CONTENT_MPEG1,MPEG2_VIDEO_STREAM);

	CH6_AVControl ( VIDEO_DISPLAY, true, CONTROL_VIDEOANDAUDIO );

	/*  注意此处需添加CA */
#endif
	return IPANEL_OK;
}

/*
功能说明：
	停止当前service 的播放。
参数说明：
	输入参数：
		无
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
  */
INT32_T ipanel_avm_stop_srv(VOID)
{
	CH6_NewChannel ( 0x1fff, 0x1ffe, 0x1ffd, true, false, STAUD_STREAM_CONTENT_MPEG1, MPEG1_VIDEO_STREAM );
	CH6_AVControl ( VIDEO_BLACK, false, CONTROL_VIDEOANDAUDIO );

	return IPANEL_OK;
}

/*
功能说明：
	对AV manager 模块和相关的设备进行操作控制。
参数说明：
	输入参数：
		op：控制命令
		typedef enum
		{
			IPANEL_AVM_SET_CHANNEL_MODE,
			IPANEL_AVM_SET_MUTE,
			IPANEL_AVM_SET_PASS_THROUGH,
			IPANEL_AVM_SET_VOLUME,
			IPANEL_AVM_SELECT_DEV,
			IPANEL_AVM_ENABLE_DEV,
			IPANEL_AVM_SET_TVMODE,
			IPANEL_AVM_SET_VISABLE,
			IPANEL_AVM_SET_ASPECT_RATIO,
			IPANEL_AVM_SET_WIN_LOCATION,
			IPANEL_AVM_SET_WIN_TRANSPARENT,
			IPANEL_AVM_SET_CONTRAST,
			IPANEL_AVM_SET_HUE,
			IPANEL_AVM_SET_BRIGHTNESS,
			IPANEL_AVM_SET_SATURATION,
			IPANEL_AVM_SET_SHARPNESS,
			IPANEL_AVM_PLAY_I_FRAME，
			IPANEL_AVM_STOP_I_FRAME，
			IPANEL_AVM_SET_STREAM_TYPE，
			IPANEL_AVM_START_AUDIO,
			IPANEL_AVM_STOP_AUDIO,
		} IPANEL_AVM_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
		op, arg 取值见下表：
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_avm_ioctl(IPANEL_AVM_IOCTL_e op,VOID *arg)
{
	eis_report("\n ipanel_avm_ioctl op=%d",op);
	switch ( op )
	{
	case IPANEL_AVM_SET_CHANNEL_MODE:
		{
			IPANEL_ADEC_CHANNEL_OUT_MODE_e p_Parm;

			p_Parm = ( IPANEL_ADEC_CHANNEL_OUT_MODE_e)(arg);
			switch ( p_Parm )
			{
			case IPANEL_AUDIO_MODE_STEREO:				/* 立体声*/
				eis_report("\n IPANEL_AUDIO_MODE_STEREO ");
				gc_AudioMode = 3;
				CH_SetAudioModeOnly(gc_AudioMode);
				break;
			case IPANEL_AUDIO_MODE_LEFT_MONO:			/* 左声道*/
				gc_AudioMode = 2;
				eis_report("\n IPANEL_AUDIO_MODE_LEFT_MONO ");
				CH_SetAudioModeOnly(gc_AudioMode);
				break;
			case IPANEL_AUDIO_MODE_RIGHT_MONO:		/* 右声道*/
				eis_report("\n IPANEL_AUDIO_MODE_RIGHT_MONO ");
					gc_AudioMode = 1;
				CH_SetAudioModeOnly(gc_AudioMode);
				break;
			case IPANEL_AUDIO_MODE_MIX_MONO:			/* 左右声道混合*/
					gc_AudioMode = 0;
					eis_report("\n IPANEL_AUDIO_MODE_MIX_MONO ");
			      CH_SetAudioModeOnly(gc_AudioMode);
				break;
			case IPANEL_AUDIO_MODE_STEREO_REVERSE:	/* 立体声，左右声道反转*/
				gc_AudioMode = 3;
				eis_report("\n IPANEL_AUDIO_MODE_STEREO_REVERSE ");
				CH_SetAudioModeOnly(gc_AudioMode);
				break;
			}
		}
		break;
	case IPANEL_AVM_SET_MUTE:
		{
			IPANEL_SWITCH_e p_Param;

			p_Param = (int)(( int  *)(arg));
			switch ( p_Param )
			{
				case IPANEL_DISABLE:
					CH6_UnmuteAudio ();
					break;
				case IPANEL_ENABLE:
					CH6_MuteAudio ();
					break;
			}
		}
		break;
	case IPANEL_AVM_SET_PASS_THROUGH:
		{
		#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_PASS_THROUGH 未实现" );
		#endif
			break;
		}
	case IPANEL_AVM_SET_VOLUME:
		{
			int 	p_Param;

			p_Param = (int)(arg);
				//CH6_MuteAudio();
			mpeg_set_audio_volume_LR( (p_Param)  / 3+ 31, (p_Param)  / 3 + 31 );
				//CH6_UnmuteAudio();

			//main_set_audio_volume_LR ( (*p_Param)  / 3+ 31, (*p_Param)  / 3 + 31 );
		}
		break;
	case IPANEL_AVM_SELECT_DEV:
		{
			IPANEL_VDIS_VIDEO_OUTPUT_e p_Param;

			p_Param =(IPANEL_VDIS_VIDEO_OUTPUT_e) arg;
			switch ( p_Param )
			{
			case IPANEL_VIDEO_OUTPUT_CVBS:
			case IPANEL_VIDEO_OUTPUT_SVIDEO:
			case IPANEL_VIDEO_OUTPUT_RGB:
			case IPANEL_VIDEO_OUTPUT_YPBPR:
			case IPANEL_VIDEO_OUTPUT_HDMI:
				#ifdef __EIS_API_MEDIA_DEBUG__
				eis_report ( "\n++>eis IPANEL_AVM_SELECT_DEV 未实现" );
				#endif
				break;
			}
		}
		break;
	case IPANEL_AVM_ENABLE_DEV:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_ENABLE_DEV 未实现" );
			#endif
		}
		break;
	case IPANEL_AVM_SET_TVMODE:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_TVMODE 只在标清上有用" );
			#endif
		}
		break;
	case IPANEL_AVM_SET_VISABLE:
		{
			IPANEL_SWITCH_e p_Param;

			p_Param = ( IPANEL_SWITCH_e  )arg;

			switch ( p_Param )
			{
			case IPANEL_ENABLE:
				CH6_AVControl ( VIDEO_BLACK, FALSE, CONTROL_VIDEOANDAUDIO );
				break;
			case IPANEL_DISABLE:
				CH6_AVControl ( VIDEO_DISPLAY, true, CONTROL_VIDEOANDAUDIO );
				break;
			}
		}
		break;
	case IPANEL_AVM_SET_ASPECT_RATIO:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_ASPECT_RATIO 未实现" );
			#endif
		}
		break;
	case IPANEL_AVM_SET_WIN_LOCATION:
		{
			IPANEL_RECT *p_Param;

			p_Param = (IPANEL_RECT *)(arg);

			eis_report( "\n++>eis video_location,x=%d,y=%d,w=%d,h=%d",p_Param->x, p_Param->y, p_Param->w, p_Param->h );
			#ifdef eis_if_debug
			#endif

			if ( ( p_Param->x==0 && p_Param->y==0 && p_Param->w==0 && p_Param->h==0 )
				||(p_Param->x==0 && p_Param->y==0 && p_Param->w==720 && p_Param->h==576 ) )
			{
				CH6_SetVideoWindowSize( false, 0, 0, 720, 576 );
			 }
			else
			{
				//eis_set_video_window(x,y,w,h);
				if(((p_Param->x+EIS_SCREEN_STARTX)>=10)&&((p_Param->y+EIS_SCREEN_STARTY)>=30))
					CH6_SetVideoWindowSize( TRUE, p_Param->x+30-4, p_Param->y+10, p_Param->w, p_Param->h );
				else
					CH6_SetVideoWindowSize( TRUE, p_Param->x+40-4, p_Param->y+40, p_Param->w, p_Param->h );					
			}
		}
		break;
	case IPANEL_AVM_SET_WIN_TRANSPARENT:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_WIN_TRANSPARENT 未实现" );
			#endif
		}
		break;
	case IPANEL_AVM_SET_CONTRAST:
		{
		}
		break;
	case IPANEL_AVM_SET_BRIGHTNESS:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_BRIGHTNESS 未实现" );
			#endif			
		}
		break;
	case IPANEL_AVM_SET_SATURATION:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_SATURATION 未实现" );
			#endif			
		}
		break;
	case IPANEL_AVM_SET_SHARPNESS:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_SHARPNESS 未实现" );
			#endif
		}
		break;
	case IPANEL_AVM_PLAY_I_FRAME:
		break;
	case IPANEL_AVM_STOP_I_FRAME:
		CH6_CloseMepg ();
		CH6_AVControl ( VIDEO_BLACK, false, CONTROL_VIDEOANDAUDIO );
		break;
	case IPANEL_AVM_SET_STREAM_TYPE:
		{
			#ifdef __EIS_API_MEDIA_DEBUG__
			eis_report ( "\n++>eis IPANEL_AVM_SET_STREAM_TYPE 未实现" );
			#endif
		}
		break;
	case IPANEL_AVM_START_AUDIO:
		{
			CH6_UnmuteAudio ();
		}
		break;
	case IPANEL_AVM_STOP_AUDIO:
		{
			CH6_MuteAudio ();
		}
		break;
	}
}

/*
功能说明：
	打开一个播放器实例。
参数说明：
	输入参数：
		des： 播放器初始化参数，具体含义待定义。
		cbk： 打开播放器时注册的一个回调函数。
		typedef void (*player_event_notify)(UINT32_T player, INT32_T event, void
		*param)
		player: 播放器实例句柄
		event： 待定义
		param： event 关联数据指针。待定义
	输出参数：无
	返 回：
		！＝ IPANEL_NULL：返回打开的具体的媒体handle；
		＝＝ IPANEL_NULL：失败。
*/
#ifdef IPANEL_AAC_FILE_PLAY
static boolean AAC_play=false;
void CH_set_AAC_play_state(boolean AAC_play_state)
{
	AAC_play=AAC_play_state;
}
boolean CH_get_AAC_play_state(void )
{
	return AAC_play;
}

UINT32_T ipanel_mediaplayer_open(const CHAR_T *des, player_event_notify cbk)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_open " );
	#endif
	
	if(/*des==NULL ||  sqzow20100602*/cbk==NULL)
		return IPANEL_NULL;
	CH_set_AAC_play_state(true);
	if(AAC_Open ==  0)	
	{
		extern ST_Partition_t *SystemPartition;
		AAC_Open =1;
		CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);/*关闭音频*/
		CH_AudioUnLink();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
		CH_CleanBuf();
		AAC_play_state=AAC_STATUS_FREE_E ;
	}
	else
	{
		CH_CleanBuf();
		CH_MP3_ExitPlayer();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
	}
	AAC_CallBackFunc = cbk;
	return AAC_handel;
}

/*
功能说明：
	停止正在播放的、暂停的、停止的、快进的、后退的媒体源，并且释放该媒体源
	信息及其相关信息。该函数是非阻塞的。
参数说明：
	输入参数：
		player: 一个播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败
*/
INT32_T ipanel_mediaplayer_close(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_close " );
	#endif
	if(AAC_Open)
	{
		CH_MP3_ExitPlayer();
		CH_AudioUnLink();
		CH_AudioLink();
		CH6_AVControl(VIDEO_DISPLAY,TRUE,CONTROL_AUDIO); 
		AAC_Open =0;
		AAC_CallBackFunc=NULL;
		AAC_play_state=AAC_STATUS_UNKOWN_E ;
		CH_set_AAC_play_state(false);

	}
	return IPANEL_OK;
}

/*
功能说明：
	从起始位置播放由mrl 指定的视音频流。
参数说明：
	输入参数：
		player：播放器句柄
		mrl：指定媒体位置和获取方式，如：
		组播:igmp://238.22.22.22:1000;
		点播：rtsp://host/path/;
		广播：dvb://f=384M,s=6875,M=qam64，serviceid = 8；
		des：播放初始化参数，具体含义待定。
		说明：因为我们没有区分setup 和play 过程，可考虑使用play 参数扩展支
		持。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
/*int gs_test_content = STAUD_STREAM_CONTENT_AIFF; sqzow20100604*/
INT32_T ipanel_mediaplayer_play(UINT32_T player,const BYTE_T *mrl,const BYTE_T *des)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_play " );
	#endif
#if 0	
	if(((AAC_play_state == AAC_STATUS_STOP_E)||(AAC_play_state == AAC_STATUS_PAUSE_E) )&&(AAC_Open))
	{
		CH_CleanBuf();
		CH_MP3_Resume();
	}
	AAC_play_state = AAC_STATUS_PLAY_E;
#else
	CH_set_AAC_play_state(true);
	if(AAC_Open ==  0)	
	{
		extern ST_Partition_t *SystemPartition;
		AAC_Open =1;
		CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);/*关闭音频*/
		CH_AudioUnLink();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
		CH_CleanBuf();
		AAC_play_state=AAC_STATUS_FREE_E ;
	}
	else
	{
		CH_CleanBuf();
		CH_MP3_ExitPlayer();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
	}

	//初始化播出部分
	#ifdef IPANEL_AAC_FILE_PLAY
	if(CH_get_AAC_play_state())
	{
		int steam_content, sample_rate, i;
		unsigned char buf[24], *pstr;
		const char *pRateString = "sample_rate=", *pTypeString = "mediatype=";
		
#if 0
		steam_content = gs_test_content;
		gs_test_content = gs_test_content >> 1;
		if(gs_test_content == 1)
		{
			gs_test_content = STAUD_STREAM_CONTENT_AIFF;
			sttbx_Print("\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");
		}
#else
		/*设置默认值*/
		steam_content = STAUD_STREAM_CONTENT_MPEG_AAC;
		sample_rate = 48000;
		eis_report("\nipanel_mediaplayer_play des =[%s]", des);
		/*获取播放参数*/
		if(des)
		{
			/*获取采样率*/
			pstr = strstr(des, pRateString);
			if(pstr)
			{
				pstr += strlen(pRateString);
				sscanf((char*)pstr, "%d", &sample_rate);
				if(sample_rate == 0)
				{
					sample_rate = 48000;
				}
			}
			/*获取类型*/
			pstr = strstr(des, pTypeString);
			if(pstr)
			{
				pstr += strlen(pTypeString);
				for(i = 0; pstr[i] != '&' && pstr[i]; i++)
				{
					buf[i] = pstr[i];
				}
				buf[i] = 0;
			}
			else
			{
				buf[0] = 0;
			}
			/*解析类型*/
			if(strcmp(buf, "aac+LC") == 0)
			{
				steam_content = STAUD_STREAM_CONTENT_MPEG_AAC;/*ok*/
			}
			else if(strcmp(buf, "aac_raw") == 0)
			{
				steam_content = STAUD_STREAM_CONTENT_RAW_AAC;/*ok*/
			}
			else if(strcmp(buf, "aac_HE") == 0)
			{
				steam_content = STAUD_STREAM_CONTENT_MP4_FILE;/*ok*/
			}
			/*else if(strcmp(des, ".aac_LTP") == 0)
			{
				steam_content = STAUD_STREAM_CONTENT_RAW_AAC;
			} sqzow20100604*/
		}
#endif		
		AAC_Comp_Start(steam_content, sample_rate);
	}
	else
	#endif
    	CH_MP3_PCMDecodeInit();

	AAC_play_state=AAC_STATUS_PLAY_E;
	return AAC_handel;

#endif
	return IPANEL_OK;
}

/*
功能说明：
	将正在播放的、暂停的、后退的、前进的视频停止播放，进入待命状态。
参数说明：
	输入参数：
		player：播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_stop(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_stop " );
	#endif
	if((AAC_Open))
	{
		CH_AAC_Pause();
		AAC_play_state = AAC_STATUS_STOP_E;
	}
	return IPANEL_OK;
}

/*
功能说明：
	暂时停止正在播放的视频，需要记忆住视频的状态(播放时间、位置和模式等)。
参数说明:
	输入参数：
		player: 播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_pause(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_pause " );
	#endif
	if((AAC_play_state== AAC_STATUS_PLAY_E)&&(AAC_Open))
	{
		CH_AAC_Pause();
		AAC_play_state = AAC_STATUS_PAUSE_E;
	}
	return IPANEL_OK;
}

/*
功能说明：
	恢复暂时停止播放的视频。该函数是非阻塞的。
参数说明：
	输入参数：
		player：播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_resume(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_resume " );
	#endif
	if((AAC_play_state== AAC_STATUS_PAUSE_E)&&(AAC_Open))
	{
		CH_MP3_Resume();
		AAC_play_state = AAC_STATUS_PLAY_E;
	}
	return IPANEL_OK;
}

/*
功能说明：
	以不同的速度快速播放的视频， 2 表示2 倍速，5 表示5 倍速，其他值视为无效，
	直接返回-1。该函数是非阻塞的。
参数说明：
输入参数：
	player：播放器句柄
	rate： 快速播放的倍数如2，5 等。
输出参数：无
返 回：
	IPANEL_OK: 函数执行成功;
	IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_forward(UINT32_T player, INT32_T rate)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_forward " );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	以不同的速度慢速播放的视频， 2 表示1/2 倍速，5 表示1/5 倍速，值为正整数
	有效，其他值视为无效，直接返回-1。该函数是非阻塞的。
参数说明：
	输入参数：
		player：播放器句柄
		rate：慢速播放的倍数如2，5 等。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_slow(UINT32_T player, INT32_T rate)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_slow " );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	以不同的速度回退播放的视频， 2 表示2 倍速，5 表示5 倍速，其他值视为无效，
	直接返回-1。该函数是非阻塞的。
参数说明：
输入参数：
	player：播放器句柄
	rate：回退视频的倍数如2，5 等。
输出参数：无
返 回：
	IPANEL_OK: 函数执行成功;
	IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_rewind(UINT32_T player,INT32_T rate)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_slow 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	跳转到一个指定的时间点开始正常播放。
参数说明：
	输入参数：
		player：播放器句柄
		pos：时间，可以是秒为单位的offset 或绝对时间（如：2007 年10 月18 日
		11：00：00）。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败
*/
INT32_T ipanel_mediaplayer_seek(UINT32_T player,CHAR_T *pos)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_slow 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	设置和获取播放器的各种参数。
	duration(Get)、position（Set/Get）、repeat、change service(MTS),伴音选择，
	声道选择, status(play,pause,ff,fr),rate、adec、vdec 的控制、display 的控制等。
	具体见下面的枚举类型：
	typedef enum
	{
		IPANEL_MEDIA_GET_DURATION,
		IPANEL_MEDIA_GET_STATUS,
		IPANEL_MEDIA_GET_RATE,
		IPANEL_MEDIA_GET_POSITION,
		IPANEL_MEDIA_SET_POSITION,
		IPANEL_MEDIA_SET_REPEAT,
		IPANEL_MEDIA_SET_SERVICE,
		IPANEL_MEDIA_SET_AUDIO,
		IPANEL_MEDIA_SET_SOUND_CHANNEL,
		// adec 的控制
		// vdec 的控制
		// display 的控制等。
	} IPANEL_MEDIA_PLAYER_IOCTL_e;
	这部分接口使用到的类型还有：
	typedef enum _MEDIA_STATU_TYPE
	{
		running ＝ ０,
		stop,
		pause,
		rewind,
		reverse,
		forward,
		slow
	} MEDIA_STATU_TYPE
	typedef struct _MEDIA_STATUS
	{
		INT32_T statu;
		INT32_T param;
	} MEDIA_STATUS;
参数说明：
	输入参数：
		player：播放器句柄
		op： 操作类型
		param：不同操作类型所带的参数
		op 和对应的param 取值见下表：
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_ioctl(UINT32_T player,INT32_T op,UINT32_T *param)
{
	pIPANEL_XMEMBLK pcmblk;
	pIPANEL_PCMDES pcm_des = NULL;
	boolean push_data_sussce;
	#ifdef __EIS_API_MEDIA_DEBUG__
	/*eis_report ( "\n++>eis ipanel_mediaplayer_ioctl  op=%d",op ); sqzow20100603*/
	#endif
	switch(op)
	{
		case 7:
			pcmblk=(pIPANEL_XMEMBLK)param;
			if(NULL != pcmblk)
			{
				if((pcmblk->pbuf) && (pcmblk->len>0))
				{
					/*eis_report ( "\n  ipanel_mediaplayer_ioctl pcmblk->pbuf=%x,pcmblk->len=%d", pcmblk->pbuf,pcmblk->len); sqzow20100603*/
					push_data_sussce=CH_IPANEL_AACStart(pcmblk->pbuf, pcmblk->len);
				}
				
				if(push_data_sussce)
				{
					return IPANEL_OK;
				}
				else
				{
					/*eis_report("\n ipanel_mediaplayer_ioctl failed "); sqzow20100611*/
					return IPANEL_ERR;
				}
			}
			else
			{
				return IPANEL_ERR;
			}
			
			break;
		default:
			break;
	}
	return IPANEL_OK;
}

/*
功能说明：
	开始记录节目到一个设备。
参数说明：
	输入参数：
		player：播放器句柄
		mrl：指定媒体位置和获取方式，如：
		组播:igmp://238.22.22.22:1000;
		点播：rtsp://host/path/;
		广播：dvb://f=384M,s=6875,M=qam64，serviceid = 8；
		如果录制当前player 上正在播放的节目，mrl 可以为空。
		device：录制设备初始化参数，具体含义待定。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_start_record(UINT32_T player, const BYTE_T *mrl, const BYTE_T *device)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_start_record 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	停止录制节目。
参数说明：
	输入参数：
		player：播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_stop_record(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_start_record 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	将当前播放位置置为断点，并记录此时的状态，先去做其他事情，然后再跳转回
	来继续正常播放（该操作只有在正常播放的状态下才可以生效）；该函数是非阻塞的。
注意：（tujz）
	书签功能实现需要研究是客户端实现的还是服务器端实现的
	如果设置书签需要相应接口返回到书签位置，接口待定。
参数说明：
	输入参数：无
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_break(VOID)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_break 未完成" );
	#endif

	return IPANEL_OK;
}

#else
#ifdef MP3_DATA	
UINT32_T ipanel_mediaplayer_open(CONST CHAR_T *des, IPANEL_PLAYER_EVENT_NOTIFY cbk)
#else
UINT32_T ipanel_mediaplayer_open(const CHAR_T *des, player_event_notify cbk)
#endif
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_open " );
	#endif
	
#ifdef MP3_DATA	
	if(des==NULL || cbk==NULL)
		return IPANEL_NULL;
	if(MP3_Open ==  0)	
	{
		extern ST_Partition_t *SystemPartition;
		MP3_Open =1;
		CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);/*关闭音频*/
		CH_AudioUnLink();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
		MP3_play_state=MP3_STATUS_FREE_E ;
	}
	else
	{
		CH_CleanBuf();
		CH_MP3_ExitPlayer();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);
	}
	
	MP3_CallBackFunc = cbk;
	return MP3_handel;
#endif

	return IPANEL_OK;
}

/*
功能说明：
	停止正在播放的、暂停的、停止的、快进的、后退的媒体源，并且释放该媒体源
	信息及其相关信息。该函数是非阻塞的。
参数说明：
	输入参数：
		player: 一个播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败
*/
INT32_T ipanel_mediaplayer_close(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_close " );
	#endif
#ifdef MP3_DATA	
	if(MP3_Open)
	{
		if(MP3_play_state ==  MP3_STATUS_PLAY_E)
		{
			CH_DecodesentlastData();
		}
	
		CH_MP3_ExitPlayer();
		CH_AudioUnLink();
		CH_AudioLink();
		CH6_AVControl(VIDEO_DISPLAY,TRUE,CONTROL_AUDIO); 
		MP3_Open =0;
		MP3_CallBackFunc=NULL;
		MP3_play_state=MP3_STATUS_UNKOWN_E ;
	}
	return IPANEL_OK;
#endif

	return IPANEL_OK;
}

/*
功能说明：
	从起始位置播放由mrl 指定的视音频流。
参数说明：
	输入参数：
		player：播放器句柄
		mrl：指定媒体位置和获取方式，如：
		组播:igmp://238.22.22.22:1000;
		点播：rtsp://host/path/;
		广播：dvb://f=384M,s=6875,M=qam64，serviceid = 8；
		des：播放初始化参数，具体含义待定。
		说明：因为我们没有区分setup 和play 过程，可考虑使用play 参数扩展支
		持。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_play(UINT32_T player,const BYTE_T *mrl,const BYTE_T *des)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_play " );
	#endif
	#ifdef MP3_DATA
	if(((MP3_play_state == MP3_STATUS_STOP_E)||(MP3_play_state == MP3_STATUS_PAUSE_E) )&&(MP3_Open))
	{
		CH_MP3_Resume();
	}
	MP3_play_state = MP3_STATUS_PLAY_E;
	#endif
	return IPANEL_OK;
}

/*
功能说明：
	将正在播放的、暂停的、后退的、前进的视频停止播放，进入待命状态。
参数说明：
	输入参数：
		player：播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_stop(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_stop " );
	#endif
	#ifdef MP3_DATA
	if((MP3_Open))
	{
		CH_MP3_Pause();
		MP3_play_state = MP3_STATUS_STOP_E;
	}
	#endif
	return IPANEL_OK;
}

/*
功能说明：
	暂时停止正在播放的视频，需要记忆住视频的状态(播放时间、位置和模式等)。
参数说明:
	输入参数：
		player: 播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_pause(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_pause " );
	#endif
	#ifdef MP3_DATA
	if((MP3_play_state== MP3_STATUS_PLAY_E)&&(MP3_Open))
	{
		CH_MP3_Pause();
		MP3_play_state = MP3_STATUS_PAUSE_E;
	}
	#endif
	return IPANEL_OK;
}

/*
功能说明：
	恢复暂时停止播放的视频。该函数是非阻塞的。
参数说明：
	输入参数：
		player：播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_resume(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_resume " );
	#endif
	#ifdef MP3_DATA
	if((MP3_play_state== MP3_STATUS_PAUSE_E)&&(MP3_Open))
	{
		CH_MP3_Resume();
		MP3_play_state = MP3_STATUS_PLAY_E;
	}
	#endif
	return IPANEL_OK;
}

/*
功能说明：
	以不同的速度快速播放的视频， 2 表示2 倍速，5 表示5 倍速，其他值视为无效，
	直接返回-1。该函数是非阻塞的。
参数说明：
输入参数：
	player：播放器句柄
	rate： 快速播放的倍数如2，5 等。
输出参数：无
返 回：
	IPANEL_OK: 函数执行成功;
	IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_forward(UINT32_T player, INT32_T rate)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_forward " );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	以不同的速度慢速播放的视频， 2 表示1/2 倍速，5 表示1/5 倍速，值为正整数
	有效，其他值视为无效，直接返回-1。该函数是非阻塞的。
参数说明：
	输入参数：
		player：播放器句柄
		rate：慢速播放的倍数如2，5 等。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_slow(UINT32_T player, INT32_T rate)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_slow " );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	以不同的速度回退播放的视频， 2 表示2 倍速，5 表示5 倍速，其他值视为无效，
	直接返回-1。该函数是非阻塞的。
参数说明：
输入参数：
	player：播放器句柄
	rate：回退视频的倍数如2，5 等。
输出参数：无
返 回：
	IPANEL_OK: 函数执行成功;
	IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_rewind(UINT32_T player,INT32_T rate)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_slow 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	跳转到一个指定的时间点开始正常播放。
参数说明：
	输入参数：
		player：播放器句柄
		pos：时间，可以是秒为单位的offset 或绝对时间（如：2007 年10 月18 日
		11：00：00）。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败
*/
INT32_T ipanel_mediaplayer_seek(UINT32_T player,CHAR_T *pos)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_slow 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	设置和获取播放器的各种参数。
	duration(Get)、position（Set/Get）、repeat、change service(MTS),伴音选择，
	声道选择, status(play,pause,ff,fr),rate、adec、vdec 的控制、display 的控制等。
	具体见下面的枚举类型：
	typedef enum
	{
		IPANEL_MEDIA_GET_DURATION,
		IPANEL_MEDIA_GET_STATUS,
		IPANEL_MEDIA_GET_RATE,
		IPANEL_MEDIA_GET_POSITION,
		IPANEL_MEDIA_SET_POSITION,
		IPANEL_MEDIA_SET_REPEAT,
		IPANEL_MEDIA_SET_SERVICE,
		IPANEL_MEDIA_SET_AUDIO,
		IPANEL_MEDIA_SET_SOUND_CHANNEL,
		// adec 的控制
		// vdec 的控制
		// display 的控制等。
	} IPANEL_MEDIA_PLAYER_IOCTL_e;
	这部分接口使用到的类型还有：
	typedef enum _MEDIA_STATU_TYPE
	{
		running ＝ ０,
		stop,
		pause,
		rewind,
		reverse,
		forward,
		slow
	} MEDIA_STATU_TYPE
	typedef struct _MEDIA_STATUS
	{
		INT32_T statu;
		INT32_T param;
	} MEDIA_STATUS;
参数说明：
	输入参数：
		player：播放器句柄
		op： 操作类型
		param：不同操作类型所带的参数
		op 和对应的param 取值见下表：
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_ioctl(UINT32_T player,INT32_T op,UINT32_T *param)
{
	pIPANEL_XMEMBLK pcmblk;
	pIPANEL_PCMDES pcm_des = NULL;
	
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_ioctl  op=%d",op );
	#endif
	switch(op)
	{
		case 7:
			#ifdef MP3_DATA	

			pcmblk=(pIPANEL_XMEMBLK)param;
			if(NULL != pcmblk)
			{
				#if 0
				if(pcmblk->pdes)
				{
					pcm_des = (pIPANEL_PCMDES)pcmblk->pdes;
					CH_SetPCMPara(pcm_des->samplerate,pcm_des->bitspersample,pcm_des->channelnum);
				}
				#endif
				if((pcmblk->pbuf) && (pcmblk->len>0))
				{
					eis_report ( "\n  ipanel_mediaplayer_ioctl pcmblk->pbuf=%x,pcmblk->len=%d", pcmblk->pbuf,pcmblk->len);
					CH_IPANEL_MP3Start(pcmblk->pbuf, pcmblk->len);
				}
				ipanel_pcmblk=pcmblk;
			}
			return IPANEL_OK;
			
			#endif
			break;
		default:
			break;
	}
	return IPANEL_OK;
}

/*
功能说明：
	开始记录节目到一个设备。
参数说明：
	输入参数：
		player：播放器句柄
		mrl：指定媒体位置和获取方式，如：
		组播:igmp://238.22.22.22:1000;
		点播：rtsp://host/path/;
		广播：dvb://f=384M,s=6875,M=qam64，serviceid = 8；
		如果录制当前player 上正在播放的节目，mrl 可以为空。
		device：录制设备初始化参数，具体含义待定。
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_start_record(UINT32_T player, const BYTE_T *mrl, const BYTE_T *device)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_start_record 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	停止录制节目。
参数说明：
	输入参数：
		player：播放器句柄
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_stop_record(UINT32_T player)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_start_record 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	将当前播放位置置为断点，并记录此时的状态，先去做其他事情，然后再跳转回
	来继续正常播放（该操作只有在正常播放的状态下才可以生效）；该函数是非阻塞的。
注意：（tujz）
	书签功能实现需要研究是客户端实现的还是服务器端实现的
	如果设置书签需要相应接口返回到书签位置，接口待定。
参数说明：
	输入参数：无
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_mediaplayer_break(VOID)
{
	#ifdef __EIS_API_MEDIA_DEBUG__
	eis_report ( "\n++>eis ipanel_mediaplayer_break 未完成" );
	#endif

	return IPANEL_OK;
}
#endif
/*
功能说明：
显示I FAME背景	
注意：

参数说明：
	输入参数：无
	输出参数：无
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/

void EIS_ShowIFrame(void)
{
	boolean errno=true; 
	int loop=8;
	
#if 1 	/*重复多播放几次 解决有时显示失败的问题 sqzow20100708*/

	if(eis_IFrame_buf == NULL)
	{
		return;	
	}
	semaphore_wait(gp_EisSema);

	while(loop -- > 0)
	{
		eis_report("\n ShowIFrame %d\n", 8 - loop);
		
		CH_DisplayIFrameFromMemory ( (U8 *)eis_IFrame_buf,eis_IFrame_len, 8, 4);
		errno = CH_GetBackIFramData();
		if(errno == 0)
		{
			gi_IFrameShow = true;
			break;
		}
	}
	semaphore_signal(gp_EisSema);	
	
#else
	semaphore_wait(gp_EisSema);
	eis_report("\n ShowIFrame");
	if(eis_IFrame_buf != NULL)
	{
	   CH_DisplayIFrameFromMemory ( (U8 *)eis_IFrame_buf,eis_IFrame_len, 8, 4);
	 }
	semaphore_signal(gp_EisSema);	
	CH_GetBackIFramData();
#endif

}

boolean EIS_GetOtherIFramStaus(void)
{
    boolean b_status;
    semaphore_wait(gp_EisSema);	
    b_status = gb_DisplayOtherIFram;
    semaphore_signal(gp_EisSema);
    return b_status;
}
 
boolean EIS_GetLastVideoControlStaus(void)
{
	unsigned int time_now=get_time_ms();
	if((enum_LastVideoControl == IPANEL_VDEC_RESUME || enum_LastVideoControl == IPANEL_VDEC_START)&&((time_now-eis_VDECstart_time) > 3000))
	{
		//eis_report("\n time_now=%d eis_VDECstart_time=%d",time_now,eis_VDECstart_time);
		return true;
	}
	else
		return false;
}

U8 EIS_GetVIdeoType( void )
{
	return gc_VideoType;
}

void EIS_ResumeVideoPlay( void )
{
	task_delay(ST_GetClocksPerSecond()/10);
	CH6_AVControl(VIDEO_BLACK, false, CONTROL_VIDEO);
	CH_OnlyVideoControl(vPid,true,pcrPid,gc_VideoType);
}
boolean EIS_ResumeAudioPlay(void)
{
	if(enum_LastAudioControl == IPANEL_ADEC_RESUME|| enum_LastAudioControl == IPANEL_ADEC_START)
	{
		CH_OnlyAudioControl(8191,aPid,true,gst_AudioType,gc_AudioMode);
		return true;
	}
}
/*-------------------------------函数定义注释头--------------------------------------
    FuncName:EIS_GetIFrameStatus
    Purpose: 获得I帧的最终显示状态
    Reference: 
    in:
        void >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/07/23 20:05:53 
---------------------------------函数体定义----------------------------------------*/
char EIS_GetIFrameStatus(void)
{
	return gi_IFrameShow;
}
INT32_T ipanel_porting_media_close(VOID)
{
	return 0;
}

#if 0
void TestVid(void)
{
					      CH_OnlyVideoControl(vPid,true,pcrPid,gc_VideoType);

}

boolean play_over=false;
void  test_callbak(UINT32_T hmixer, IPANEL_AUDIO_MIXER_EVENT event, UINT32_T *param)
{
	play_over=true;
}
void test_pcm_player(void)
{
#if 1
	FILE *fp = NULL;
	int handel;
	unsigned char * pcm_buf=NULL;
	int size,act_size;
	int block_size=128*1024;
	IPANEL_XMEMBLK player_block;
	int file_size=600*1024/*730112*/;
			CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);/*关闭音频*/
		CH_AudioUnLink();
		CH_MP3_PlayerInitlize(SystemPartition,4,6);

	CH_MP3_NewMp3("G://mp3_err/8000-16-mono-8.mp3",8000);
	Task_Delay(ST_GetClocksPerSecond() * 20);  
			CH_MP3_ExitPlayer();
		CH_AudioUnLink();
		CH_AudioLink();
		CH6_AVControl(VIDEO_DISPLAY,TRUE,CONTROL_AUDIO); 

		return ;
		
       //fp = fopen("D://billie_jean.pcm","rb");
       fp = fopen("G://mp3_err/44100-16-mono-32.mp3","rb");
	   
	if(fp == NULL)
		return ;
	pcm_buf=ipanel_porting_malloc(file_size+1024);
	if(NULL==pcm_buf)
	{
		do_report(0,"\n  内存不够");
		return ;
	}
	size = fread((void *)pcm_buf, 1,file_size, fp);
	if(size>0)
	{
		act_size=0;
		player_block.pbuf=pcm_buf;
		player_block.len=block_size;
		handel=ipanel_porting_audio_mixer_open(0,test_callbak);
		ipanel_porting_audio_mixer_memblk_send(handel,&player_block);
		act_size+=block_size;
		play_over=false;
		do_report(0,"\n   开始播放");
		while((act_size+block_size)<size)
		{
			if(play_over)
			{
				player_block.pbuf=(UINT32_T *)((int)pcm_buf+act_size);
				player_block.len=block_size;
				ipanel_porting_audio_mixer_memblk_send(handel,&player_block);
				act_size+=block_size;
				play_over=false;
				do_report(0,"\n   播放下一段");
			}
			Task_Delay(ST_GetClocksPerSecond() / 20);  
		}
	}
	else
	{
		do_report(0,"\n   读文件失败");
	}
	
	fclose(fp);
	ipanel_porting_free(pcm_buf);
	#endif
}
#ifdef  IPANEL_AAC_FILE_PLAY
static CHAR_T * AAC_buff=NULL;
static unsigned int  ACT_len=0;
unsigned int file_size=(3901440);
void test_aac_player(void)
{
	FILE *fp = NULL;
	unsigned int size;
	do_report(0,"\n 播放开始");
	CH6_AVControl(VIDEO_DISPLAY, false, CONTROL_AUDIO);/*关闭音频*/
	CH_AudioUnLink();
	if(AAC_buff==NULL)
	{
		AAC_buff=(CHAR_T *)memory_allocate ( CHSysPartition,file_size);
		if(AAC_buff==NULL)
			return ; 
		#if 0
		memcpy((U8 *)AAC_buff,0xa1000000,0x400000);
		#else
		   fp = fopen("D:\\TDdownload\\Music\\i'm gonna getcha good.mp3","rb");
			if(fp == NULL)
				return ;
		size = fread((void *)AAC_buff, 1,file_size, fp);
		
		if(size<=0)
		{
			fclose(fp);
			return ;
		}

		fclose(fp);
		#endif
	}
	do_report(0, "\n读入数据OK");
	ACT_len=0;
	AAC_Comp_Start(STAUD_STREAM_CONTENT_MP3, 44100);
	while(ACT_len<size)
	{
	Task_Delay(ST_GetClocksPerSecond() );  
	}
	Task_Delay(ST_GetClocksPerSecond() *5);  
	Comp_Stop();
	CH_AudioUnLink();
	CH_AudioLink();
	CH6_AVControl(VIDEO_DISPLAY,TRUE,CONTROL_AUDIO); 
	do_report(0,"\n 播放结束");
	
	return  ; 
}


void CH_GetAACData(U16* uST_GetBuffer,
										   U32* uST_Length )
{
	int	Buf_Length = 128*1024, i_Loop;
	if(ACT_len>=file_size)
		return;
	
	if(ACT_len+Buf_Length<file_size)
	{
		memcpy(uST_GetBuffer,AAC_buff+ACT_len,Buf_Length);
		*uST_Length = Buf_Length;	
		ACT_len+=Buf_Length;
		do_report(0,"\n 已经播放长度ACT_len=%d",ACT_len);
	}
	else
	{
		memcpy(uST_GetBuffer,
		AAC_buff+ACT_len,
		file_size-ACT_len);
		*uST_Length = file_size-ACT_len;
		ACT_len=file_size;
	}
#if 0/*放到注入时变换 sqzow20100607*/
	for ( i_Loop = 0 ; i_Loop < (*uST_Length >> 1); i_Loop++ )
	{
		__asm__("swap.b %1, %0" : "=r" (uST_GetBuffer[i_Loop]): 
				"r" (uST_GetBuffer[i_Loop]));
	}
#endif

}
#endif
#endif
/*--eof---------------------------------------------------------------------------------------------------*/

