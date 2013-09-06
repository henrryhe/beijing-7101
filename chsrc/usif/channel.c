/*
	(c) Copyright changhong
  Name:channel.c
  Description:program control for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-06-30    Created


 * =====================
 * MODIFICATION	HISTORY:
 * =====================
 *
 * Date	       Modification	    remarks									
 * ----	       ------------		--------

 * 2005-02-     changed by yxl  形成一个NORMAL Release的版本         
						        定义宏BEIJING_DBASE表示GeHua Release 否则，为NORMAL Release
				          
*/
#include "stdlib.h"
#include "..\main\initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "string.h"
#include "..\report\report.h"
#include "..\dbase\vdbase.h"
#include "channelbase.h"

#include "putin_pin.h"
#include "..\key\key.h"
#include "Audio.h"  
#include "graphicmid.h"


void CH_AudioSet(int Audio_pid,STAUD_StreamContent_t AudTypeTemp);


#ifdef   NAFR_KERNEL /*add this on 040928*/
extern BOOL CHCA_StopBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType);
extern BOOL CHCA_StartBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType);
extern BOOL CHCA_StopBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected);
extern BOOL CHCA_StartBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected);
extern BOOL  ChSendMessage2PMT ( S16 sProgramID,S16  sTransponderID,PMTStaus  iModuleType );/*add this on 050424*/
extern BOOL  ChCaReSearchProgram;/*add this on 050303*/
/*extern void ClearDescrambler(void); add this on 050425*/
#endif

void CH_UpdateLast(void);
void CH6_NewChannel(int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType,BYTE VidType);

/*static*/ WORD2SHORT LastWatchPro;/*上一次观看的节目,yxl 2006-01-15 cancel static*/
static SHORT  LastLockPro=-1;     /*观看上一次锁定节目*/
static BOOL gIsNeedPasswordCheck=true;/*true :need ,false no need,yxl 2005-01-17 add this variable*/






/*yxl 2005-01-17 add these function
parameter status :
true:stand for need password check
flase:stand for no need password check
*/
void SetPasswordCheckStatus(BOOL Status)
{
	gIsNeedPasswordCheck=Status;
}

/*
return value:
true:stand for need password check
flase:stand for no need password check
*/
BOOL GetPasswordCheckStatus(void)
{
	return gIsNeedPasswordCheck;
}
/*end yxl 2005-01-17 add these function*/

void CH_SetLastLock(SHORT LockPro)
{
	LastLockPro=LockPro;
}


void CH6_SendNewTuneReq(SHORT sNewTransSlot, TRANSPONDER_INFO_STRUCT *pstTransInfoPassed)
{

	if(sNewTransSlot!=INVALID_LINK)
	{
		;
	}
       else
	{
		pstTransInfoPassed ->abiIQInversion= (IQPWM_INIT&0x80)?1:0;
	}	
	CH_UpdatesCurTransponderId(sNewTransSlot);   /*20070718 add*/
	CHDVBNewTuneReq(pstTransInfoPassed);
}





clock_t  g_NewChannelTime = 0/* wz add 20071126*/;

BOOL CH_NewChannel(BOOL IsOpenVideo,BOOL IsOpenAudio)
{

}


void CH_ReturnLastPro(void)
{
	
}

void CH_UpdateLast(void)
{
	

}



/*设置伴音PID*/


void CH_AudioCut(void)
{

    STAUD_Fade_t AudFade;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	
	ErrCode = STPTI_SlotClearPid(AudioSlot);
	
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotClearPid(AudioSlot)=%s\n", GetErrorText(ErrCode) ));	
	}

#ifdef USE_INNER_DA	  
	AVC_AudioStop();
#else
	ErrCode = STAUD_Stop( AUDHandle, STAUD_STOP_NOW, &AudFade );	
#endif
		
    return;
}

void CH_AudioSet(int Audio_pid,STAUD_StreamContent_t AudTypeTemp)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    STAUD_StartParams_t AUDStartParams;




 #if 0/* 20080403 del for HDMI暴音*/
    CH_OpenMute();
 #endif
    CH_AudioCut();
	task_delay(ST_GetClocksPerSecond()/10);
	/*audio process*/
    ErrCode = STPTI_SlotSetPid(AudioSlot, Audio_pid);

    AUDStartParams.StreamContent     = AudTypeTemp;
    AUDStartParams.SamplingFrequency = 44100;

	AUDStartParams.StreamType = STAUD_STREAM_TYPE_PES;
	AUDStartParams.StreamID = STAUD_IGNORE_ID;

	 CH_SetSPIDFtOut(AudTypeTemp);/* wz 20071130 add */
	ErrCode = STAUD_Start(AUDHandle, &AUDStartParams);
    
	ErrCode = STAUD_EnableSynchronisation(AUDHandle);

    if ( ErrCode != ST_NO_ERROR )
    {
		STTBX_Print(("STAUD_EnableSyncronisation()=%s\n", GetErrorText(ErrCode) ));
    }

/*	ErrCode = STAUD_Mute(AUDHandle, FALSE, FALSE); wz del 20080403

    if ( ErrCode != ST_NO_ERROR )
    {
		STTBX_Print(("STAUD_Mute(FALSE)=%s\n", GetErrorText(ErrCode) ));
    }    */   
#if 0/* 20080403 del */
    CH_CloseMute();
#endif
    return;
}



boolean bFrameOut 	= false;
boolean bFrameLost 	= false;

/*
  * Func: CH6_IfVideoRun
  * Desc: 判断视频是否在运行
  */
boolean CH6_IfVideoRun()
{
	static int iCheckCount	= 0;
	
	if( bFrameOut == false )
	{
		iCheckCount ++;

		if( iCheckCount == 5 )
		{
			bFrameLost = true;
			return false;
		}
	}
	else
	{
		/*do_report( 0, "->Get!\n" );*/
		bFrameLost	= false;
		bFrameOut 	= false;
		iCheckCount 	= 0;
		return true;
	}
}
/* End jqz050415 add */






