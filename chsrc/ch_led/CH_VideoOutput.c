/*
  (C) Copyright changhong
  Name:CH_VideoOutput.c
  Description:实现长虹7100 platform视频输出类型、定时模式等转换的应用控制和接口函数
				(与客户需求相关）
  Authors:Yixiaoli
  Date          Remark
  2007-05-14   Created (将原7710 相关部分移植过来)

  Modifiction : 

*/

#include "appltype.h"
#include "initterm.h"
#include "stddefs.h"
#include "stcommon.h"

#include "errors.h"
#include <string.h>
#include <channelbase.h>

#include "..\dbase\vdbase.h"
#include "keymap.h"
#include "..\main\CH_VideoOutput.h"
#include "..\main\ch_hdmi.h"
#include "..\main\ch_pt6964.h"


/*yxl 2005-11-12 add below function*/
/*用四位LED显示当前视频输出类型

	LED显示 H--X 表示系统处于显示视频输出状态
	LED显示 H--0 代表视频输出类型unknown	
	LED显示 H--1 代表视频输出为YPBPR
	LED显示 H--2 代表视频输出VGA
	LED显示 H--3 代表视频输出DVI
	LED显示 H--4 代表视频输出CVBS
	
	LED显示 E--X 表示系统处于设置视频输出的状态

*/
/*****************************************************************************
Name:
BOOL CH_LEDDisVOutType(CH_VideoOutputType Type,U8 Mode)
Description:
	Diaplay  current video output type in the LED.
Parameters:

Return Value:
	TRUE:not success
	FALSE:success 
See Also:
*****************************************************************************/
BOOL CH_LEDDisVOutType(CH_VideoOutputType Type,U8 Mode )
{
	U8  BufTemp [8];
	
	BufTemp[0]=0x40;

	if(Mode==0)
		BufTemp[FIRSTBIT]=LED_H;/*display "H" in the first position*/
	else BufTemp[FIRSTBIT]=LED_E;/*display "E" in the first position*/

#if 0 /*yxl 2006-05-23 modify below section*/
	BufTemp[SECONDBIT]=LED_MLINE;/*display "-" in the sectond position*/
	BufTemp[THIRDBIT]=LED_MLINE;/*display "-" in the third position*/

	switch(Type)
	{
	case TYPE_YUV:
		BufTemp[FOURTHBIT]=LED_1;/*display "1" in the fourth position,which stand for YPBPR output*/ 
		break;
	case TYPE_RGB:
		BufTemp[FOURTHBIT]=LED_2;/*display "2" in the fourth position,which stand for VGA output*/ 
		break;
#ifdef DVI_FUNCTION  /*yxl 2006-04-25 add this line*/
	case TYPE_DVI:
		BufTemp[FOURTHBIT]=LED_3;/*display "3" in the fourth position,which stand for DVI output*/ 
		break;
#endif /*yxl 2006-04-25 add this line*/
#ifdef HDMI_FUNCTION  /*yxl 2006-05-18 add this line*/
	case TYPE_HDMI:
		BufTemp[FOURTHBIT]=LED_5;/*display "5" in the fourth position,which stand for HDMI output*/ 
		break;
#endif /*yxl 2006-04-25 add this line*/
	case TYPE_CVBS:
		BufTemp[FOURTHBIT]=LED_4;/*display "4" in the fourth position,which stand for CVBS output*/ 
		break;
	default:
		BufTemp[FOURTHBIT]=LED_0;/*display "0" in the fourth position,which stand for output type unknown*/ 
	}

#else

	switch(Type)
	{
	case TYPE_CVBS:
		BufTemp[SECONDBIT]=LED_C;/*display "c " in the sectond position*/
		BufTemp[THIRDBIT]=LED_V;/*display "v" in the third position*/
		BufTemp[FOURTHBIT]=LED_B;/*display "b" in the fourth position,which stand for CVBS output*/ 
		break;
	case TYPE_YUV:
		BufTemp[SECONDBIT]=LED_Y;/*display "y " in the sectond position*/
		BufTemp[THIRDBIT]=LED_P;/*display "P" in the third position*/
		BufTemp[FOURTHBIT]=LED_B;/*display "b" in the fourth position,which stand for YPBPR output*/ 
		break;
	case TYPE_RGB:
		BufTemp[SECONDBIT]=LED_V;/*display "V" in the sectond position*/
		BufTemp[THIRDBIT]=LED_G;/*display "G" in the third position*/
		BufTemp[FOURTHBIT]=LED_A;/*display "A" in the fourth position,which stand for VGA output*/ 
		break;
#ifdef DVI_FUNCTION  
	case TYPE_DVI:
		BufTemp[SECONDBIT]=LED_D;/*display "d" in the sectond position*/
		BufTemp[THIRDBIT]=LED_V;/*display "V" in the third position*/
		BufTemp[FOURTHBIT]=LED_I;/*display "I" in the fourth position,which stand for DVI output*/ 
		break;
#endif 
#ifdef HDMI_FUNCTION  
case TYPE_HDMI:
		BufTemp[SECONDBIT]=LED_h;/*display "h" in the sectond position*/
		BufTemp[THIRDBIT]=LED_D;/*display "d" in the third position*/
		BufTemp[FOURTHBIT]=LED_I;/*display "I" in the fourth position,which stand for HDMI output*/ 
		break;
#endif 
	default:
		BufTemp[SECONDBIT]=LED_NOP;/*display " " in the sectond position*/
		BufTemp[THIRDBIT]=LED_NOP;/*display " " in the third position*/
		BufTemp[FOURTHBIT]=LED_0;/*display "0" in the fourth position,which stand for output type unknown*/ 
	}
#endif /*end yxl 2006-05-23 modify below section*/

	
#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL
	return LEDDisplay(BufTemp);
#endif

#ifdef LEDANDBUTTON_CONTROL_MODE_BY_PT6964
	return LEDDisplay(&BufTemp[1]);
#endif
}
/*end 2005-11-12 add below function*/


/*yxl 2005-11-12 add below function*/
/*
用LED显示视频输出类型和通过前边面板设置视频输出类型
*/
/*****************************************************************************
Name:
BOOL CH_VOutTypeDisplayAndControl()
Description:
	Diaplay  video output type in the LED and set video output type by front panel key. 
Parameters:

Return Value:
	TRUE:not success
	FALSE:success 
See Also:
*****************************************************************************/
/*llr add 060516*/
#ifdef LEDANDBUTTON_CONTROL_MODE_BY_PT6964
#define FRONT_UP_KEY 12
#define FRONT_DOWN_KEY 4
#define FRONT_OK_KEY 6
#define FRONT_MENU_KEY 11
#define PT_KEYSCAN_LEN        0x02   /*两个扫描线*/

BOOL CH_VOutTypeDisplayAndControl(CH_VideoOutputType*  pType)
{
	ST_ErrorCode_t ErrCode;	
	BYTE i, j,  uc_RecvByte,uc_key;
	usif_cmd_t* str_pMsg;
	clock_t     st_timeout;
	int si_SamekeyCount;
	BYTE suc_LastKey;
			
	U8 KeyPressedLast=0xff;/* the valid key pressed last time,initlia value is 0xff*/

	BOOLEAN Temp=FALSE;
	int timeout=10;
	BOOLEAN FirstPressedKey=FALSE;
	CH_VideoOutputType TypeTemp=*pType;



	CH_LEDDisVOutType(TypeTemp,0);
	
	while(true)
	{
         	task_delay(ST_GetClocksPerSecond()/10*3/2);/*150ms*/
         	/*设置为读模式*/
		 CH_PUB_ClearPTSTB();
        	 CH_PUB_Senddata(PT_READ_KEY);
         	task_delay(1000);
		if(timeout<0) return;

		{
			uc_key = 0xFF;

			for (j=PT_KEYSCAN_LEN;j>0;j--)
			{
				uc_RecvByte = CH_PT_Recv();
				if (uc_RecvByte != 0)
				{
					for(i = 0; i < 8; i++)
					{
						if (uc_RecvByte & 0x1)
						{
							uc_key = i + ((PT_KEYSCAN_LEN - j) * 8);
							break;
						}
						uc_RecvByte >>= 1;
					}
				}
			}
	
			CH_PUB_ClearPTCLK(); /* ? */
			CH_PUB_SetPTSTB();
			
			if (uc_key == 0xFF)
			{
				si_SamekeyCount=0;
				suc_LastKey=0xFF;
				if(FirstPressedKey==FALSE)
					timeout--;
			}
			else/*有键值*/
			{
				if(suc_LastKey!=uc_key)
				{
					si_SamekeyCount=0;
					Temp=TRUE;
					suc_LastKey=uc_key;
/*					do_report(0,"have Key=%d\n",uc_key);	 */ 
				}
				else
				{
					if(si_SamekeyCount>3)
					{
						si_SamekeyCount=0;
						Temp=TRUE;
						suc_LastKey=uc_key;
/*						do_report(0,"have Key=%d\n",uc_key);	 */ 
					}
					si_SamekeyCount++;
				}
				if(Temp=TRUE)
				{
					Temp = false;
					if(uc_key!=FRONT_MENU_KEY&&FirstPressedKey==FALSE)
					{
						STTBX_Print(("\n YxlInfo  keyvalue=%d\n",uc_key));
						timeout--;
					}
					if(uc_key==FRONT_MENU_KEY&&FirstPressedKey==FALSE)
					{
						CH_LEDDisVOutType(TypeTemp,1);
						FirstPressedKey=true;
					}
					if(FirstPressedKey=true)
					{
						switch(uc_key)
						{
						case FRONT_UP_KEY:
						case FRONT_DOWN_KEY:
							
							
							TypeTemp+=(uc_key==FRONT_UP_KEY)?1:-1;
							if(TypeTemp>=TYPE_UNCHANGED) /*yxl 2006-05-23 modify 4 to TYPE_UNCHANGED*/
							{
								TypeTemp=0;
							}
							if(TypeTemp<0)
							{
								
								TypeTemp=TYPE_UNCHANGED-1; /*yxl 2006-05-23 modify 3 to TYPE_UNCHANGED-1*/
							}
							CH_LEDDisVOutType(TypeTemp,1);
							break;
						case FRONT_OK_KEY:
							CH_LEDDisVOutType(TypeTemp,0);
							*pType=TypeTemp;
							task_delay(30000);
							return;
						default:
							break;

						}
					}
				}
			}
		}
	}
}
#endif

#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL  /*yxl 2006-05-14 add this line*/

#define FRONT_UP_KEY 15
#define FRONT_DOWN_KEY 27
#define FRONT_OK_KEY 28
#define FRONT_MENU_KEY 14

BOOL CH_VOutTypeDisplayAndControl(CH_VideoOutputType*  pType)
{
	STPIO_OpenParams_t OpenParams;
	STPIO_Handle_t FrontKBPIOHandle,FrontKBScanHandle;	
	ST_ErrorCode_t ErrCode;		
    unsigned char  StatusScanH = 0;	/* the status of PIO input when scan line is High */
	unsigned char  StatusScanL = 0;	/* the status of PIO input when scan line is low */

	U8 KeyPressedLast=0xff;/* the valid key pressed last time,initlia value is 0xff*/

	U8 KeyPressed;/* the valid key pressed current*/
	int CountTemp=0;
	int TicksTemp;
	BOOLEAN SendSign=FALSE;/* when true,send out the message of front panel button pressed */
  /*  usif_cmd_t* pMsg;*/
	U8 InputMaskTemp;
	clock_t timeout;
	BOOLEAN FirstPressedKey=FALSE;
	CH_VideoOutputType TypeTemp=*pType;

	CH_LEDDisVOutType(TypeTemp,0);
	OpenParams.ReservedBits = ( PIO_BIT_1 | PIO_BIT_2 | PIO_BIT_3 | PIO_BIT_4 );
	OpenParams.BitConfigure[1] = STPIO_BIT_INPUT;
	OpenParams.BitConfigure[2] = STPIO_BIT_INPUT;
	OpenParams.BitConfigure[3] = STPIO_BIT_INPUT;
	OpenParams.BitConfigure[4] = STPIO_BIT_INPUT;
	OpenParams.IntHandler =  NULL; 
	ErrCode = STPIO_Open(PIO_DeviceName[5], &OpenParams, &FrontKBPIOHandle );

	if( ErrCode != ST_NO_ERROR )
	{
		debugmessage("PIO3 Open false!\nTest end abnormally\n");
		return ;
	}

	InputMaskTemp=OpenParams.ReservedBits;	

	OpenParams.ReservedBits    = ( PIO_BIT_0 );
	OpenParams.BitConfigure[0] = STPIO_BIT_OUTPUT;
	OpenParams.IntHandler      = NULL; 
	ErrCode = STPIO_Open(PIO_DeviceName[5], &OpenParams, &FrontKBScanHandle );

	if( ErrCode != ST_NO_ERROR )
	{
		debugmessage("PIO4 Open false!\nTest end abnormally\n");
		return ;
	}

/*	STTBX_Print(("ST_GetClocksPerSecond()=%d\n",ST_GetClocksPerSecond()));*/
	TicksTemp=ST_GetClocksPerSecond()/60;
	while(true)
	{
		task_delay(TicksTemp);
		CountTemp++;
		if(CountTemp>=70/*100,200*/&&KeyPressedLast==0xff) 
		{
			break;
		}
		else if(CountTemp>=200)
		{
			CountTemp=200;
		}


		ErrCode= STPIO_Write( FrontKBScanHandle, 0xff);
		if( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STPIO_Write()=%s",GetErrorText(ErrCode)));
			return ;
		}		
		
		ErrCode= STPIO_Read( FrontKBPIOHandle, &StatusScanH );
		if( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STPIO_Read()=%s",GetErrorText(ErrCode)));			
			return ;
		}	
		else StatusScanH=StatusScanH&InputMaskTemp;
		
		ErrCode= STPIO_Write( FrontKBScanHandle, 0xfe);
		if( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print(("STPIO_Write()=%s",GetErrorText(ErrCode)));
			return ;
		}		

		ErrCode = STPIO_Read(FrontKBPIOHandle, &StatusScanL);
		if( ErrCode != ST_NO_ERROR )
		{

			STTBX_Print(("STPIO_Read()=%s",GetErrorText(ErrCode)));
			return ;
		}
		else StatusScanL=StatusScanL&InputMaskTemp;

		if((StatusScanL==StatusScanH)&&StatusScanH==InputMaskTemp) /* not any key pressed*/
		{
			continue;
		}

		KeyPressed=StatusScanL;
		if(StatusScanL!=StatusScanH)
		{
			KeyPressed=KeyPressed|0x01;
		}

		if(KeyPressedLast!=KeyPressed) 
		{
			CountTemp=0;		
			#ifdef	KEY_DEBUG
			STTBX_Print(("Front key pressed=%x Last=%x Count=%d \n", 
			KeyPressed,KeyPressedLast,CountTemp));
			#endif

			KeyPressedLast=KeyPressed;
			SendSign=TRUE;

		}
		else if(KeyPressedLast==KeyPressed)
		{
			if(CountTemp>30/*15*/) 
			{
				#ifdef	KEY_DEBUG
				STTBX_Print(("AA:Front key pressed=%x Last=%x Count=%d \n", 
				KeyPressed,KeyPressedLast,CountTemp));
				#endif
				KeyPressedLast=KeyPressed;
				CountTemp=0;
				SendSign=TRUE;
			}

		}
		if(SendSign==TRUE)
		{
			SendSign=FALSE;
				STTBX_Print(("\n YxlInfo  keyvalue=%d\n",KeyPressed));
			if(KeyPressed!=FRONT_MENU_KEY&&FirstPressedKey==FALSE)
			{
				STTBX_Print(("\n YxlInfo  keyvalue=%d\n",KeyPressed));
				break;
			}
			if(KeyPressed==FRONT_MENU_KEY&&FirstPressedKey==FALSE)
				CH_LEDDisVOutType(TypeTemp,1);
			FirstPressedKey=TRUE;
			switch(KeyPressed)
			{
			case FRONT_UP_KEY:
			case FRONT_DOWN_KEY:
				
				
				TypeTemp+=(KeyPressed==FRONT_UP_KEY)?1:-1;
				if(TypeTemp>=TYPE_UNCHANGED) /*yxl 2006-05-23 modify 4 to TYPE_UNCHANGED*/
				{
					TypeTemp=0;
				}
				if(TypeTemp<0)
				{
					
					TypeTemp=TYPE_UNCHANGED-1; /*yxl 2006-05-23 modify 3 to TYPE_UNCHANGED-1*/
				}
				CH_LEDDisVOutType(TypeTemp,1);
				break;
			case FRONT_OK_KEY:
				CH_LEDDisVOutType(TypeTemp,0);
				*pType=TypeTemp;
				task_delay(30000);
				goto exit1;
			default:
				break;

			}
		}
	
	}
exit1:
	ErrCode=STPIO_Close(FrontKBScanHandle);
	if( ErrCode != ST_NO_ERROR )
	{
		STTBX_Print(("STPIO_Close()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	ErrCode=STPIO_Close(FrontKBPIOHandle);
	if( ErrCode != ST_NO_ERROR )
	{
		STTBX_Print(("STPIO_Close()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}	
	return FALSE;
}
/*end 2005-11-12 add below function*/

#endif /*yxl 2006-05-14 add this line*/


/*yxl 2005-11-14 add below section*/
/*视频输出类型和定时模式的一致性检查
*/
/*****************************************************************************
Name:
BOOL CH_VOutTypeAndTimingMatchCheck()
Description:
	视频输出类型和定时模式的一致性检查,检查时以视频输出类型为主，若两者有冲突时则定时模式顺从于
	视频输出类型。
Parameters:

Return Value:
	TRUE:not success
	FALSE:success 
See Also:
*****************************************************************************/
/*yxl 2006-07-28 add paramter BOOL IsFindBestMode,when True,the timeing mode is the best for all output type*/
BOOL CH_VOutTypeAndTimingMatchCheck(CH_VideoOutputType* pType,CH_VideoOutputMode* pTimingMode,BOOL IsFindBestMode )
{
	int i;
	CH_VideoOutputType TypeTemp;
	CH_VideoOutputMode ModeTemp;

	static CH_VideoOutputMode TvoutMode[]=
	{	
		MODE_576I50
	};
	static CH_VideoOutputMode YPbPrMode[]=
	{
		MODE_1080I50,
		MODE_720P50,
		MODE_576P50
#ifdef MODEY_576I /*yxl 2006-06-26 add below section*/
		,MODE_576I50
#endif/* end yxl 2006-06-26 add below section*/
#ifdef TIMEMODE_ALL_HZ
		,MODE_1080I60,
		MODE_720P60
#endif
	};
	static CH_VideoOutputMode VGAMode[]=
	{
#ifndef VGA_ONLY_ONE /*yxl 2005-11-17 add this line*/
		MODE_1080I50,
		MODE_720P50_VGA,/*yxl 2005-12-23 add_VGA _VGA*/
#endif /*yxl 2005-11-17 add this line*/
		MODE_576P50
#ifdef TIMEMODE_ALL_HZ
		,MODE_1080I60,
		MODE_720P60
#endif
	};

	#ifdef DVI_FUNCTION  /*yxl 2006-04-25 add this line*/
	static CH_VideoOutputMode DVIMode[]=
	{	
#ifndef DVI_ONLY_ONE /*yxl 2005-11-21 add this line*/
		MODE_1080I50,
#endif /*yxl 2005-11-21 add this line*/
			MODE_720P50_DVI /*YXL 2006-03-27 ADD _DVI*/
#ifdef TIMEMODE_ALL_HZ
			,MODE_1080I60,
			MODE_720P60
#endif
	};
#endif /*yxl 2006-04-25 add this line*/

#ifdef HDMI_FUNCTION /*yxl 2006-04-25 add below section*/
	static CH_VideoOutputMode HDMIMode[]=
	{	
		MODE_1080I50,
		MODE_720P50
#ifdef TIMEMODE_ALL_HZ
		,MODE_576P50/*yxlxl*/
/*#ifdef TIMEMODE_ALL_HZ*/
	,MODE_1080I60,
		MODE_720P60
#endif

	};
#endif /*end yxl 2006-04-25 add below section*/

#if 0 /*yxl 2006-04-25 modify below section*/
	
#ifdef TIMEMODE_ALL_HZ
	int ModeNum[4]={1,5,5,4};
#else
	int ModeNum[4]={1,3,3,2};
#endif

#else /*yxl 2006-04-25 modify below section*/

#ifdef TIMEMODE_ALL_HZ
	int ModeNum[]=
	{1,5,5
	#ifdef DVI_FUNCTION 
	,4
	#endif
	#ifdef HDMI_FUNCTION 
	,5
	#endif
	};

#else
	int ModeNum[]=
	{1
#ifdef MODEY_576I /*yxl 2006-06-26 add below section*/
	,4
#else
	,3
#endif/* end yxl 2006-06-26 add below section*/
	,3

#ifdef DVI_FUNCTION 
	,2
	#endif
	#ifdef HDMI_FUNCTION 
	,2
#endif
	};

#endif

#endif /*end yxl 2006-04-25 modify below section*/


	CH_VideoOutputMode *ModeList[]=
	{
		TvoutMode,
		YPbPrMode,
		VGAMode
		#ifdef DVI_FUNCTION 
		,DVIMode
		#endif
		#ifdef HDMI_FUNCTION 
		,HDMIMode
		#endif	
	};

	/*yxl 2005-11-21 add below section*/
#ifdef DVI_ONLY_ONE
	ModeNum[3]=1;
#endif
#ifdef VGA_ONLY_ONE
	ModeNum[2]=1;
#endif
	/*end yxl 2005-11-21 add below section*/


	TypeTemp=*pType;

	/*yxl 2005-12-23 add below section*/
	if(TypeTemp==TYPE_RGB)
	{
		if(*pTimingMode==MODE_720P50) *pTimingMode=MODE_720P50_VGA;
	}
	/*end yxl 2005-12-23 add below section*/
#ifdef DVI_FUNCTION  /*yxl 2006-04-25 add this line*/
	/*yxl 2006-03-27 add below section*/
	if(TypeTemp==TYPE_DVI)
	{
		if(*pTimingMode==MODE_720P50) *pTimingMode=MODE_720P50_DVI;
	}
	/*end yxl 2006-03-27 add below section*/
#endif /*yxl 2006-04-25 add this line*/
	ModeTemp=*pTimingMode;




	if(TypeTemp>=TYPE_UNCHANGED) 
	{
		TypeTemp=TYPE_YUV;
		*pType=TypeTemp;
	}

	for(i=0;i<ModeNum[TypeTemp];i++)
	{
		if(ModeTemp==ModeList[TypeTemp][i]) break;
	}
	if(i<ModeNum[TypeTemp]&&IsFindBestMode==FALSE)  /*yxl 2006-07-28 add &&IsFindBestMode==FALSE*/
	{
		return FALSE;
	}
	switch(TypeTemp)
	{
	case TYPE_YUV:
		ModeTemp=ModeList[TypeTemp][0];/*ModeList[TypeTemp][1],yxl 2005-11-22 modfiy to ModeTemp=ModeList[TypeTemp][0],that's MODE_1080I50*/
		break;
	case TYPE_RGB:
		#ifndef VGA_ONLY_ONE/*yxl 2005-11-21 add this line*/		
		ModeTemp=ModeList[TypeTemp][0]; /*yxl 2006-08-02 modify 2 to 0*/
		#else /*yxl 2005-11-21 add below section*/
		ModeTemp=ModeList[TypeTemp][0];
		#endif /*end yxl 2005-11-21 add below section*/
		break;
#ifdef DVI_FUNCTION  /*yxl 2006-04-25 add this line*/
	case TYPE_DVI:
		#ifndef DVI_ONLY_ONE /*yxl 2005-11-21 add this line*/
		ModeTemp=ModeList[TypeTemp][0];/*yxl 2006-03-28 modify 1->0*/
		#else /*yxl 2005-11-21 add below section*/
		ModeTemp=ModeList[TypeTemp][0];
		#endif /*end yxl 2005-11-21 add below section*/
		break;
#endif  /*yxl 2006-04-25 add this line*/
	case TYPE_CVBS:
		ModeTemp=ModeList[TypeTemp][0];
		break;
#ifdef HDMI_FUNCTION  /*yxl 2006-04-25 add this line*/
	case TYPE_HDMI:
		ModeTemp=ModeList[TypeTemp][0]; /*yxl 2006-05-19 modify from 1->0*/
		break;
#endif  /*yxl 2006-04-25 add this line*/
	default:
				/*yxl 2006-03-29 add below section*/
		TypeTemp=TYPE_YUV;
		ModeTemp=MODE_1080I50;
		/*end yxl 2006-03-29 add below section*/
		break;
	}
	*pType=TypeTemp;
	*pTimingMode=ModeTemp;
	return FALSE;
}
/*end yxl 2005-11-14 add below section*/


/*yxl 2006-03-13 add below function*/

void CH_VideoFormatChange(int ChangeKey)
{
	int KeyRead;
	SHORT	 temp;
	CH_VideoOutputType TypeTemp;
	CH_VideoOutputMode ModeTemp;

	int TimeOut=5;


/*yxl 2006-03-20 add below section*/
	TypeTemp=pstBoxInfo->VideoOutputType;
	CH_LEDDisVOutType(TypeTemp,0);
	KeyRead=GetKeyFromKeyQueue(TimeOut); /*yxl 2006-03-31 modify 2 to TimeOut*/
	if(KeyRead!=ChangeKey) goto exit1; 
/*end yxl 2006-03-20 add below section*/

#ifndef FRONTKEY_TO_DGTEC /*yxl 2006-07-11 add this line*/
	/*yxl 2006-03-24 add below section*/
	if(CH6_InputFormatChangePinCode(false,true)!=0) 
	{
		goto exit1;
	}
	/*end yxl 2006-03-24 add below section*/
#endif /*end yxl 2006-07-11 add this line*/

/*	CH6_AVControl(VIDEO_BLACK,false,CONTROL_VIDEOANDAUDIO);yxl 2006-03-27 add,yxl 2006-03-31 cancel this line*/
	while(true)
	{
		CH6_AVControl(VIDEO_BLACK,false,CONTROL_VIDEO);/*yxl 2006-03-31 add this line*/
		TypeTemp=pstBoxInfo->VideoOutputType;
		TypeTemp++;
		if(TypeTemp==TYPE_UNCHANGED) TypeTemp=0;
		ModeTemp=pstBoxInfo->VideoOutputMode;
		CH_LEDDisVOutType(TypeTemp,0);
		
		CH_VOutTypeAndTimingMatchCheck(&TypeTemp,&ModeTemp,TRUE); /*yxl 2006-07-28 add ,TRUE*/
		if(TypeTemp!=pstBoxInfo->VideoOutputType||ModeTemp!=pstBoxInfo->VideoOutputMode)
		{
#ifdef MODEY_576I  /*yxl 2006-07-07 add below section*/
			if(pstBoxInfo->VideoOutputType==TYPE_YUV&&pstBoxInfo->VideoOutputMode==MODE_576I50&&TypeTemp!=TYPE_YUV)
			{
				if(CH_SetVideoOut(pstBoxInfo->VideoOutputType,
					MODE_1080I50,pstBoxInfo->VideoOutputAspectRatio,
					pstBoxInfo->VideoARConversion))
				{
					STTBX_Print(("\nYxlInfo:CH_SetVideoOut() isn't successful"));						
				}
			}
#endif  /*yxl 2006-07-07 add below section*/	
			
			pstBoxInfo->VideoOutputType=TypeTemp;
			pstBoxInfo->VideoOutputMode=ModeTemp;			
			CH_NVMUpdate ( idNvmBoxInfoId );
		}
		if(CH_SetVideoOut(TypeTemp,ModeTemp,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->VideoARConversion))
		{
			STTBX_Print(("\nYxlInfo:CH_SetVideoOut() isn't successful"));
		}
		task_delay(50000);/*yxl 2006-03-26 add this line,yxl 2006-03-31 modify 100000 to */
		CH6_AVControl(VIDEO_DISPLAY,true,CONTROL_VIDEO);/*yxl 2006-03-31 add this line*/
		
		KeyRead=GetKeyFromKeyQueue(TimeOut);/*yxl 2006-03-31 modify 2 to TimeOut*/
		if(KeyRead!=ChangeKey) break; 
		else InitKeyQueue();/*yxl 2006-03-28 add this line*/
	
	}
exit1:
#if 0 /*yxl 2006-03-31 below this line*/
		CH6_AVControl(VIDEO_DISPLAY,true/*false*/,CONTROL_VIDEOANDAUDIO);/*yxl 2006-03-27 add,yxl 2006-03-31 modify false to true*/
#endif

		/*yxl 2006-06-01 add below section*/
		if(CH_PUB_GetBGPicDisplayStatus()==true)
		{
			CH6_DrawLog();
			CH_PUB_SetBGPicDisplayStatus(true);
		}
		/*end yxl 2006-06-01 add below section*/

	temp=CH_GetsCurProgramId();
	if(temp == INVALID_LINK) 
	{
#if 0 /*yxl 2006-03-31 modify below section*/
		DisplayLED(0);
#else
		CH_PUB_ReturnCurrentChannel(false);
#endif/*end yxl 2006-03-31 modify below section*/
		CH_PUB_ReturnCurrentChannel();
	}
	else
	{
#ifdef AU_LOGICAL
	DisplayLED(pastProgramFlashInfoTable [ temp ] -> Logi_number);
#else
	DisplayLED(pastProgramFlashInfoTable [ temp ] -> abiProgramNum);
#endif

	}
	return ;
}

/*end yxl 2006-03-13 add below function*/

/*yxl 2006-03-24 add below function*/
/*用四位LED显示当前视频输出类型


*/

/*****************************************************************************
Name:
BOOL CH_LEDDisPWStatus()
Description:
	Diaplay  input password status in the LED.
Parameters:

Return Value:
	TRUE:not success
	FALSE:success 
See Also:
*****************************************************************************/
BOOL CH_LEDDisPWStatus(U8 PWPos )
{

#if defined(LEDANDBUTTON_CONTROL_MODE_BY_ATMEL) || defined(LEDANDBUTTON_CONTROL_MODE_BY_PT6964)
	U8  BufTemp [8];
	
	BufTemp[0]=0x40;

	BufTemp[FIRSTBIT]=LED_P;/*display "P" in the first position*/

	BufTemp[THIRDBIT]=LED_NOP;/*display " " in the third position*/
	BufTemp[FOURTHBIT]=LED_MLINE;/*display "-" in the fourth position*/
	switch(PWPos)
	{
	case 1:
		BufTemp[SECONDBIT]=LED_1;/*display "1" in the fourth position,which stand for 第一位 PW*/ 
		break;
	case 2:
		BufTemp[SECONDBIT]=LED_2;/*display "2" in the fourth position,which stand for 第2位 PW*/ 
		break;
	case 3:
		BufTemp[SECONDBIT]=LED_3;/*display "3" in the fourth position,which stand for 第3位 PW*/ 		
		break;
	case 4:
		BufTemp[SECONDBIT]=LED_4;/*display "4" in the fourth position,which stand for 第4位 PW*/ 
		break;
	case -1:/*display "PErr" in the LED*/ 
		BufTemp[FIRSTBIT]=LED_P;/*display "P" in the first position*/
		BufTemp[SECONDBIT]=LED_E;/*display "E" in the second position*/ 
		BufTemp[THIRDBIT]=LED_R;/*display "r " in the third position*/
		BufTemp[FOURTHBIT]=LED_R;/*display " r" in the fourth position*/
		break;
	default:
		BufTemp[SECONDBIT]=LED_NOP;/*display "" in the fourth position,which stand for */ 
	}
	if(BufTemp[SECONDBIT]==LED_NOP)
		BufTemp[FOURTHBIT]=LED_NOP;/*display "" in the fourth position*/

	/*yxl 2006-06-17 add below section*/
#ifdef STARTUP_VIDEOFORMAT_SET_BY_DIGICRYSTAL
	if(BufTemp[SECONDBIT]!=LED_NOP)
	{
		BufTemp[SECONDBIT]=LED_0;
	}
#endif

	/*end yxl 2006-06-17 add below section*/

#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL
	return LEDDisplay(BufTemp);
#endif

#ifdef LEDANDBUTTON_CONTROL_MODE_BY_PT6964
        return LEDDisplay(&BufTemp[1]);
#endif

#endif
}
/*end 2006-03-24 add below function*/

/* 2006-03-26 add below function*/
void CH_StartupVideoFormatSet(void)
{
	return ;
#ifdef STARTUP_VIDEOFORMAT_SET_BY_CHANGHONG
	CH_VideoOutputType TypeTemp;
	CH_VideoOutputMode ModeTemp;
	CH_VideoOutputType TypeTemp1;/*yxl 2006-07-28 add this line*/
	TypeTemp=pstBoxInfo->VideoOutputType;
	ModeTemp=pstBoxInfo->VideoOutputMode;
	TypeTemp1=TypeTemp;/*yxl 2006-07-28 add this line*/
	CH_VOutTypeDisplayAndControl(&TypeTemp);

#if 0 /*yxl 2006-07-28 modify below section*/
	CH_VOutTypeAndTimingMatchCheck(&TypeTemp,&ModeTemp);
#else

	if(TypeTemp1!=TypeTemp)
	{
		CH_VOutTypeAndTimingMatchCheck(&TypeTemp,&ModeTemp,TRUE);
	}
	else
	{
		CH_VOutTypeAndTimingMatchCheck(&TypeTemp,&ModeTemp,FALSE);
	}
#endif /*end yxl 2006-07-28 modify below section*/


	if(TypeTemp!=pstBoxInfo->VideoOutputType||ModeTemp!=pstBoxInfo->VideoOutputMode)
	{
		pstBoxInfo->VideoOutputType=TypeTemp;
		pstBoxInfo->VideoOutputMode=ModeTemp;			
		CH_NVMUpdate ( idNvmBoxInfoId );
	}
#endif 
#ifdef STARTUP_VIDEOFORMAT_SET_BY_DIGICRYSTAL
	int Lefttimeout;
	clock_t StartTime;
	clock_t Total_time=3;
	int KeyRead;
	CH_VideoOutputType TypeTemp;
	CH_VideoOutputMode ModeTemp;
	TypeTemp=pstBoxInfo->VideoOutputType;
	ModeTemp=pstBoxInfo->VideoOutputMode;
	CH_LEDDisVOutType(TypeTemp,0);
	InitKeyQueue();	
	StartTime=time_now();
	while(true)
	{
		KeyRead=GetKeyFromKeyQueue(1);
		if(KeyRead==MENU_KEY||KeyRead==YELLOW_KEY) break;
		Lefttimeout=CH_GetLeaveTime(StartTime,Total_time);
		if(Lefttimeout<=0) return;
	}

	CH_VideoFormatChangeA(MENU_KEY,FALSE);

#endif

}
/*end 2006-03-26 add below function*/


