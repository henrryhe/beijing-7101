
/*
	(c) Copyright changhong
  Name:vusif.c
  Description:人机交互部分  for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark   
  2004-5-27    Created
*/



#include "stddefs.h"
#include "stcommon.h"
#include "math.h"
#include "..\report\report.h"
#include "..\symbol\symbol.h"
#include "string.h"
#include "appltype.h"
#include "..\main\initterm.h"
#include "graphicmid.h"
#include "..\key\keymap.h"
#include "..\key\key.h"
#include "setjmp.h"
#include "sectionbase.h"
#include "..\dbase\section.h"
#include "audio.h"
#include "Stvout.h"
#include "video.h"
#include "..\util\ch_time.h"
#include "channelbase.h"
#include "..\dbase\vdbase.h"
#define RESET_STB

extern ST_Partition_t          *appl_partition;
void CH_TestSystemPartion(void);


semaphore_t *pApplSema=NULL; /*应用类型访问信号量*/
semaphore_t *pst_AVControlSema=NULL;/*AV控制访问信号量*/

#ifdef SUMA_SECURITY
semaphore_t *pst_GifSema=NULL;/*gif显示信号量*/
semaphore_t *pst_OsdSema = NULL;/* 用于IPNAEL的显示控制*/
#endif
semaphore_t *pst_CircleSema = NULL;
task_t *ptidTunerTask;		


task_t* ptidUsifTask;

/*20060118 add for init usif task*/
tdesc_t Stktdesc_usif;
/*task_t task_usif;*/
U8 *task_stack_usif;
/*********************************/
/*static*/ message_queue_t *pstUsifMsgQueue;

static int UsifResetMark=-1;

jmp_buf UsifMainMark;

extern  BOX_INFO_STRUCT	*pstBoxInfo;
static void DVBUsifProcess ( void *pvParam );
void MakeVirtualkeypress(int ucKeyCode);


boolean CheckSetMark(void)
{
  if(UsifResetMark==-1)
	  return false;
  else
	  return true;
}
/*20050124 change to main init*/
extern  semaphore_t  *gp_semNvmAccessLock;
extern semaphore_t    *gp_semFlashAccess;
void CH_InitSema(void)
{
	
	pApplSema=semaphore_create_fifo(1);
	pst_AVControlSema=semaphore_create_fifo(1);	
#ifdef SUMA_SECURITY
         pst_GifSema = semaphore_create_fifo_timeout(0);
	pst_OsdSema = semaphore_create_fifo(1);
#endif
	pst_CircleSema = semaphore_create_fifo(1);
	gp_semNvmAccessLock = semaphore_create_fifo(1);
	gp_semFlashAccess = semaphore_create_fifo(1);
	
}
void CH6_STBInit(void)
{
	STVOUT_OutputParams_t OutPutParam;
	SHORT	 tempsCurProgram=CH_GetsCurProgramId();


	InitKeyQueue();
	CH_UpdatesCurTransponderId(-1);
	CH_UpdatesCurProgramId(-1);;
	M_CHECK(pstBoxInfo->abiLanguageInUse,0,(MAX_LANGUAGE_ID-1));
	M_CHECK(pstBoxInfo->abiTVSaturation,1,100);
	M_CHECK(pstBoxInfo->abiTVBright,1,100);
	M_CHECK(pstBoxInfo->abiTVContrast,1,100);
        M_CHECK(pstBoxInfo->abiTVColor,0,2);

	
     do_report(0,"%d:%d:%d\n",pstBoxInfo->abiTVSaturation,pstBoxInfo->abiTVBright,pstBoxInfo->abiTVContrast);

    
      memset(&OutPutParam,0,sizeof(STVOUT_OutputParams_t));

}

/*20061113 add*/
/*函数原型:void CH_SetVedioOut(void)
功能:设置视频输出参数
输入:无
输出:无
返回:无
*/
void CH_SetVedioOut(void)
{
	STVOUT_OutputParams_t OutPutParam; 

	memset(&OutPutParam,0,sizeof(STVOUT_OutputParams_t));


	STVOUT_GetOutputParams(VOUTHandle[VOUT_AUX], &OutPutParam);
		

	M_CHECK(pstBoxInfo->abiTVSaturation,1,100);
	M_CHECK(pstBoxInfo->abiTVBright,1,100);
	M_CHECK(pstBoxInfo->abiTVContrast,1,100);
#if 0
	OutPutParam.Analog.BCSLevel.Brightness=Lum_Convert(pstBoxInfo->abiTVBright);
	OutPutParam.Analog.BCSLevel.Contrast=Con_Convert(pstBoxInfo->abiTVContrast);
	OutPutParam.Analog.BCSLevel.Saturation=Chr_Convert(pstBoxInfo->abiTVSaturation);
	OutPutParam.Analog.StateBCSLevel=STVOUT_PARAMS_AFFECTED;
	STVOUT_SetOutputParams(VOUTHandle[VOUT_MAIN],&OutPutParam);
#endif

	OutPutParam.Analog.BCSLevel.Brightness=Lum_Convert(pstBoxInfo->abiTVBright);
	OutPutParam.Analog.BCSLevel.Contrast=Con_Convert(pstBoxInfo->abiTVContrast);
	OutPutParam.Analog.BCSLevel.Saturation=Chr_Convert(pstBoxInfo->abiTVSaturation);
	OutPutParam.Analog.StateBCSLevel=STVOUT_PARAMS_AFFECTED;
	STVOUT_SetOutputParams(VOUTHandle[VOUT_AUX],&OutPutParam);
}
BOOLEAN	DVBUsifInit ( void )
{

	BOOLEAN  bError = FALSE;
       /*20060705 add Powe LED*/
        PowerLightOn();
        StandbyLedOff();
	  /*********************/
      
	/* choose the message pools for mailbox communications */
	if ( symbol_enquire_value ( "usif_queue", ( void ** ) &pstUsifMsgQueue ) )
	{
		STTBX_Print(("Cant find USIF message queue\n" ));
		return   TRUE;
	}


#ifndef CH_MUTI_RESOLUTION_RATE

	CH6_ShowViewPort(&CH6VPOSD);
#endif
	/*20070508 add显示当前输出制式*/
      CH_LED_DisplayHDMode();
	/******************************/


	if ( ( ptidUsifTask = Task_Create (DVBUsifProcess,NULL,
		2048*165,9/*8*/,"usif_task",0 )) == NULL )
 	{
		STTBX_Print(("Failed to start USIF process\n"));
		bError = TRUE;
	}
	else
	{
	/*	STTBX_Print(("Successfully started USIF process\n"));*/
	}


	return	bError;
}

void ch_usifTaskstatus(void)
{
        task_status_t Status;
        task_status_flags_t Flag = task_status_flags_stack_used; 
        int i;


		
            if(!task_status(ptidUsifTask,&Status,Flag ))
            {
                    do_report(0,"Task_Usif[%s]: task_stack_size = %d,task_stack_used = %d \n",
                                                                                task_name(ptidUsifTask),
                                                                                Status.task_stack_size,
                                                                                Status.task_stack_used);
            }


}
		


static int TestValue=0x20;

static boolean StandbyEpgResearch=true;/*待机状态是否重新搜索EPG*/
static void DVBUsifProcess ( void *pvParam )
{
	
	static  int mailindex_test=0;
	int iKeyScanCode;
	int InputNum;
	short ProgNumTemp;	
	
	SHORT MulAudioIndex=0;
	CHAR NvodPlayState=0;
    U8   TimeRefresh=0;
	int  ApplIndex;/*yxl 2005-03-03 add this line*/
#ifdef RESET_STB
	
	int TestKeyIndex=0;
	extern boolean PrintOff;
#endif	


	
#if 1
	CH_MuteInit();
	CH_CloseMute();
#endif	

#ifdef USE_IPANEL
	CH_ExigencyBroadInit();
	CH_CloseExigencyBroad();
#endif	

	

	UsifResetMark=setjmp(UsifMainMark);          /*USIF恢复位置*/			
      
      if(UsifResetMark==0)	/*第一次运行*/
	 CH6_STBInit();  
	 CH_LDR_ReadSTBConfig();
        CH_VFD_ClearAll();
	 eis_entry();

       
}
/*挂起USIF处理进程*/

void CH_SuspendUsif(void)
{
	task_suspend(ptidUsifTask);
	CH6_TerminateChannel();
	STVID_DisableOutputWindow(VIDVPHandle);             /*关闭MPEG显示*/													
	CH6_ClearFullArea(GFXHandle,CH6VPOSD.ViewPortHandle);
/*	CH6_MixLayer(FALSE,FALSE,FALSE); */
}


/*20050202 add*/
void MakeVirtualkeypress(int ucKeyCode)
{
	usif_cmd_t  *msg_p;
	clock_t timeout = ST_GetClocksPerSecondLow();
	if ( ( msg_p = ( usif_cmd_t * ) message_claim_timeout ( pstUsifMsgQueue, &timeout ) ) != NULL )
	{
		msg_p -> from_which_module   = KEYBOARD_MODULE;
		msg_p -> contents . keyboard . scancode = ucKeyCode;
		msg_p -> contents . keyboard . device   = REMOTE_KEYBOARD;
		msg_p -> contents . keyboard . repeat_status = FALSE;
		message_send ( pstUsifMsgQueue, msg_p );
	}
}



boolean gb_ShowNovidDataPic = true;
boolean CH_Check_VIDData(void)
{
    U32 BitBufferSize;
	int LoopI;
	int Judge = 0;
	VID_Device ch_CurrentVId = CH_GetCurrentVIDHandle();
	
	for (LoopI= 0;LoopI<50;LoopI++)
		{
			if(EIS_GetVIdeoType()==H264_VIDEO_STREAM)
			{
				ch_CurrentVId=VID_H;
			}
		    STVID_GetBitBufferFreeSize(VIDHandle[ch_CurrentVId], &BitBufferSize);
			
			if(ch_CurrentVId==VID_MPEG2)
			{
			    if(BitBufferSize >= 2105344)
			    	{
					Judge++;
				    	}
				}
			else
			{
			    if(BitBufferSize >= 5124096)
			    	{
					Judge++;
			    	}
		    	}
		
		}
	if(Judge == 50)
	{
		if(ch_CurrentVId== VID_H)
		{
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}

}



static ApplMode  CurApplMode=APP_MODE_PLAY; /*当前应用状态*/



/*更新当前应用*/
void CH_UpdateApplMode(ApplMode Mode)
{
	semaphore_wait(pApplSema);
	CurApplMode=Mode;
	semaphore_signal(pApplSema);
}
/*得到当前应用状态*/
ApplMode CH_GetCurApplMode(void)
{
	ApplMode tempmode;
	semaphore_wait(pApplSema);
	tempmode=CurApplMode;
	semaphore_signal(pApplSema);
	return tempmode;
}

S8 Convert2Char(int iKeyScanCode)
{
	return iKeyScanCode+0x30;
}




