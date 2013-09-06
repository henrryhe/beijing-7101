/*
(c) Copyright changhong
  Name:Vfd.c
  Description: 实现与长虹VFD显示板单片机通讯的接口函数,该VFD显示板能实现VFD显示和按键控制功能
  Authors:yixiaoli
  Date          Remark
  2007-03-12    Created

Modifiction:


*/


#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include "stddefs.h" 
#include "stlite.h" 
#include "stdevice.h" 
#include "stpio.h" 
#include "stevt.h" 
#include "sti2c.h"
#include "..\main\initterm.h"
#include "ch_vfd.h"
#include "appltype.h"


#define MAX_VFD_OFFSET 32
#define BASE_ADDRESS

#define MUTE_ADDRESS_OFFSET BASE_ADDRESS+0x00
#define MUTE_BIT_MASK       0x01

#define USB_ADDRESS_OFFSET  BASE_ADDRESS+0x00
#define USB_BIT_MASK        0x02

#define MP3_ADDRESS_OFFSET  BASE_ADDRESS+0x00
#define MP3_BIT_MASK        0x04

#define HD_ADDRESS_OFFSET   BASE_ADDRESS+0x00
#define HD_BIT_MASK         0x08

#define LEFTARROW_ADDRESS_OFFSET  BASE_ADDRESS+0x00
#define LEFTARROW_BIT_MASK        0x20


#define CH_ADDRESS_OFFSET  BASE_ADDRESS+0x00
#define CH_BIT_MASK        0x40

#define POINTA_ADDRESS_OFFSET  BASE_ADDRESS+0x00
#define POINTA_BIT_MASK        0x80

#define POINTB_ADDRESS_OFFSET  BASE_ADDRESS+0x16
#define POINTB_BIT_MASK        0x40

#define T1080I_ADDRESS_OFFSET  BASE_ADDRESS+0x01
#define T1080I_BIT_MASK        0x01

#define T720P_ADDRESS_OFFSET  BASE_ADDRESS+0x04
#define T720P_BIT_MASK        0x80


#define T576P_ADDRESS_OFFSET  BASE_ADDRESS+0x07
#define T576P_BIT_MASK        0x80

#define LINE_ADDRESS_OFFSET  BASE_ADDRESS+0x0A
#define LINE_BIT_MASK        0x40

#define T576I_ADDRESS_OFFSET  BASE_ADDRESS+0x0A
#define T576I_BIT_MASK        0x80

#define HDMI_ADDRESS_OFFSET  BASE_ADDRESS+0x10
#define HDMI_BIT_MASK        0x80

#define VGA_ADDRESS_OFFSET  BASE_ADDRESS+0x13
#define VGA_BIT_MASK        0x80

#define YPBPR_ADDRESS_OFFSET  BASE_ADDRESS+0x16
#define YPBPR_BIT_MASK        0x80


#define VIDEO_ADDRESS_OFFSET  BASE_ADDRESS+0x1C
#define VIDEO_BIT_MASK        0x80

#define TUNER0_ADDRESS_OFFSET  BASE_ADDRESS+0x1E
#define TUNER0_BIT_MASK        0x08

#define RADIO_ADDRESS_OFFSET  BASE_ADDRESS+0x1E
#define RADIO_BIT_MASK        0x80

#define TV_ADDRESS_OFFSET  BASE_ADDRESS+0x1F
#define TV_BIT_MASK        0x01

#define POWER_ADDRESS_OFFSET  BASE_ADDRESS+0x00
#define POWER_BIT_MASK        0x10

static U8 gVFDContentBuf[MAX_VFD_OFFSET+1]; 
static Semaphore_t SemVFDBufLock;   

extern semaphore_t  *gp_semNvmAccessLock;



STI2C_Handle_t 		VFDHandleW;

SHORT2BYTE DigitalToVFDData[10]={0x3123,0x1020,0x30c3,0x30e1,0x11e0,0x21e1,0x21e3,0x3020,0x31e3,0x31e1};/*LED数字格式分别代表LED显示0－9*/


BOOL CH_VFD_Init(ST_DeviceName_t I2CDeviceName)
{	
	ST_ErrorCode_t            ErrCode; 
	STI2C_OpenParams_t 	Open_params; 
	BOOL res;

	Open_params.AddressType=STI2C_ADDRESS_7_BITS;
	Open_params.BaudRate=STI2C_RATE_NORMAL;
	Open_params.BusAccessTimeOut=10000;/*2000;*/
	Open_params.I2cAddress=0xd0;

	ErrCode=STI2C_Open(I2CDeviceName, &Open_params, &VFDHandleW);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\nSTI2C_Open=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	Semaphore_Init_Fifo(SemVFDBufLock,1);
	
	memset((void*)gVFDContentBuf,0,MAX_VFD_OFFSET);
	
	res=CH_VFD_LOginOff();
	
	res=CH_VFD_EnableDisplay();

	return res;  
}



BOOL CH_VFD_GreenLightOn(void)
{
	BOOL res;

	return FALSE;


}

void CH_VFD_EnterCriction(void)
{
	Semaphore_Wait(SemVFDBufLock);
}

void CH_VFD_LeaveCriction(void)
{

	Semaphore_Signal(SemVFDBufLock);
}

BOOL CH_VFD_WriteData(unsigned int PosOff ,U32 NumToWrite,U8* WriteBuf)
{
	ST_ErrorCode_t ErrCode;
	U8 BufTemp[MAX_VFD_OFFSET+20];
	U32 NumTemp=NumToWrite;

	if(PosOff>MAX_VFD_OFFSET) 
	{
		STTBX_Print(("\n YxlInfo:wrong postion offset"));
		return TRUE;
	}

	if(WriteBuf==NULL)
	{
		STTBX_Print(("\n YxlInfo:wrong parameter,WriteBuf is NULL"));
		return TRUE;
	}

	memset((void*)BufTemp,0,MAX_VFD_OFFSET+10);

	if((PosOff+NumTemp)>MAX_VFD_OFFSET)
	{
		NumTemp=MAX_VFD_OFFSET-PosOff;
	}

	BufTemp[0]=(U8)PosOff;
	BufTemp[1]=(U8)NumTemp;

	memcpy((void*)&BufTemp[2],(const void*)WriteBuf,NumTemp);

	CH_VFD_SendCommand(CONTROL_VFD_SET,NumTemp+2,BufTemp);
	
#if 0 /*yxl 2007-03-15*/

	CH_VFD_EnterCriction();
#if 1
	memcpy((void*)&gVFDContentBuf[PosOff],(const void*)WriteBuf,NumTemp);
#else
	memcpy((void*)gVFDContentBuf,(const void*)WriteBuf,NumTemp);
#endif
	CH_VFD_LeaveCriction();

#endif 

	return FALSE;
}


BOOL CH_VFD_SendCommand(CH_VFD_CONTROL_TYPE CtrlCommand,U32 NumToWrite,U8* WriteBuf)
{

	ST_ErrorCode_t ErrCode;
	U8 BufTemp[/*30 20070524 change*/50];
	U32 NumTemp=NumToWrite;
	U32 NumWriten=0;
	U32 TimeOut=5000;
	CH_VFD_CONTROL_TYPE CommandTemp=CtrlCommand;	
	int	i;

	memset((void*)BufTemp,0,50);
	
	switch(CommandTemp)
	{
	case CONTROL_VFD_ON:
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
		break;

	case CONTROL_VFD_OFF:
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
		NumTemp=2;
		break;

	case CONTROL_VFD_SET:
		if(WriteBuf==NULL)
		{
			STTBX_Print(("\n YxlInfo:wrong parametre,WriteBuf is NULL"));
			return TRUE;
		}
		if(NumTemp==0)
		{
			STTBX_Print(("\n YxlInfo:wrong parametre,NumToWrite is NULL"));
			return TRUE;
		}
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
	
		memcpy((void*)&BufTemp[2],(const void*)WriteBuf,NumTemp);
		BufTemp[0]=NumTemp+1;
		NumTemp=2+NumTemp;
		break;

	case CONTROL_VFD_LOGIN_OFF:/*停止开机显示*/
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
		NumTemp=2;
		break;
	case CONTROL_WATCHDOG_REFRESH:/*刷新看门狗定时器*/
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
		NumTemp=2;
		break;
	case CONTROL_WATCHDOG:/*看门狗控制*/
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
		NumTemp=2;
		break;		

	case CONTROL_SET_RTC:/*设置RTC*/
		BufTemp[0]=4;
		BufTemp[1]=(U8)CommandTemp;
		memcpy((void*)&BufTemp[2],(const void*)WriteBuf,3);
		NumTemp=5;
		break;	
	case CONTROL_READ_FRONTKEY:
		BufTemp[0]=1;
		BufTemp[1]=(U8)CommandTemp;
		NumTemp=2;
		break;		
		
		break;	
#if 0
	case CONTROL_SET_AUTOPOWERON:

		break;	


	case CONTROL_SET_AUTOPOWERON_MODE:
		break;	

	case CONTROL_SET_AUTOPOWEROFF:
		break;	

	case CONTROL_SET_AUTOPOWEROFF_MODE:
		break;	

	case CONTROL_SET_WARNNING:
		break;	

	case CONTROL_SET_WARNNING_MODE:
		break;	

	case CONTROL_STANDBY:
		break;	

	case CONTROL_READ_RTC:
		break;	

	case CONTROL_READ_REMOTEKEY:
		break;	


#endif	
	default:
		STTBX_Print(("\nYxlInfo:wrong Command type\n")); 
		return TRUE;

	}
	
	NumTemp=BufTemp[0]+1;

	for(i=0;i<NumTemp;i++)
	{
		BufTemp[NumTemp]=BufTemp[NumTemp]^BufTemp[i];
	}
	semaphore_wait ( gp_semNvmAccessLock );
	ErrCode=STI2C_Write(VFDHandleW,BufTemp,NumTemp+1,TimeOut, &NumWriten);
	if(ErrCode!=ST_NO_ERROR)
	{
		
		STTBX_Print(("\nYxlInfo:STI2C_Write()=%s",GetErrorText(ErrCode)));
		semaphore_signal ( gp_semNvmAccessLock );
		return TRUE;
	}
	semaphore_signal ( gp_semNvmAccessLock );
	task_delay(ST_GetClocksPerSecond() / 50);/*20ms*/

	return FALSE;
}



BOOL CH_VFD_ReadCommand(CH_VFD_CONTROL_TYPE CtrlCommand,U32 NumToRead,U8* ReadBuf)
{

	ST_ErrorCode_t ErrCode;
	U8 BufTemp[30];
	U32 NumTemp=NumToRead;
	U32 NumRead=0;
	U32 TimeOut=5000;
	CH_VFD_CONTROL_TYPE CommandTemp=CtrlCommand;	
	int	i;

	memset((void*)BufTemp,0,30);
	
	switch(CommandTemp)
	{
				
	case CONTROL_SET_AUTOPOWERON:
		
		break;			
	case CONTROL_SET_AUTOPOWERON_MODE:
		break;			
	case CONTROL_SET_AUTOPOWEROFF:
		break;	
		
	case CONTROL_SET_AUTOPOWEROFF_MODE:
		break;	
		
	case CONTROL_SET_WARNNING:
		break;	
		
	case CONTROL_SET_WARNNING_MODE:
		break;		
	case CONTROL_STANDBY:
		break;			
	case CONTROL_READ_RTC:
		break;			
	case CONTROL_READ_REMOTEKEY:
		NumTemp=1;
		break;	
	case CONTROL_READ_FRONTKEY:
		NumTemp=2;
		break;			
	default:
		STTBX_Print(("\nYxlInfo:wrong Command type\n")); 
		return TRUE;

	}
	semaphore_wait ( gp_semNvmAccessLock );

	ErrCode=STI2C_Read(VFDHandleW,BufTemp,NumTemp,TimeOut, &NumRead);
	if(ErrCode!=ST_NO_ERROR)
	{
		
		STTBX_Print(("\nYxlInfo:STI2C_Read()=%s",GetErrorText(ErrCode)));
		semaphore_signal ( gp_semNvmAccessLock );
		return TRUE;
	}
	else
	{
		STTBX_Print(("\nCount=%d Value=%x %x ",NumRead,BufTemp[0],BufTemp[1]));
	}
	semaphore_signal ( gp_semNvmAccessLock );
	task_delay(ST_GetClocksPerSecond() / 50);/*20ms*/

	if(ReadBuf!=NULL)
	{
		memcpy((void*)ReadBuf,(const void*)BufTemp,NumTemp);
	}


	return FALSE;

}

BOOL CH_VFD_EnableDisplay(void)
{
	return CH_VFD_SendCommand(CONTROL_VFD_ON,0,NULL);
}

BOOL CH_VFD_DisableDisplay(void)
{
	return CH_VFD_SendCommand(CONTROL_VFD_OFF,0,NULL);
}

BOOL CH_VFD_ClearAll(void)
{

	BOOL res=FALSE;
	U8 BufTemp[MAX_VFD_OFFSET+1];
	int NumTemp;
	
	memset((void*)BufTemp,0x0,MAX_VFD_OFFSET+1);

	NumTemp=MAX_VFD_OFFSET+1;

	res=CH_VFD_WriteData(0,NumTemp,BufTemp);
	if(res==TRUE) return res;
	
	res=CH_VFD_SendCommand(CONTROL_VFD_SET,NumTemp,BufTemp);
	if(res==TRUE) return res;
	
	return res;

}

BOOL CH_VFD_CellBaseControl(CH_CELL_STATUS Status,U8 CellOffset,U8 CellBitMask)
{

	BOOL res;
	U8 BufTemp[20];
	U8 AdrOffsetTemp=CellOffset;
	U8 BitMaskTemp=CellBitMask;
	U8 ValueTemp;

	if(CellOffset>MAX_VFD_OFFSET)
	{
		STTBX_Print(("\nYxlInfo:wrong parameter---CellOffset"));
		return TRUE;
	}

	CH_VFD_EnterCriction();
	ValueTemp=gVFDContentBuf[AdrOffsetTemp];
	CH_VFD_LeaveCriction();

	if(Status==DISPLAY_ON)
	{
		BufTemp[0]=ValueTemp|BitMaskTemp;
	}
	else
	{
		BufTemp[0]=ValueTemp&(~BitMaskTemp);
	}

	res=CH_VFD_WriteData(AdrOffsetTemp,1,BufTemp);

/*yxl 2007-03-15*/
	CH_VFD_EnterCriction();
	gVFDContentBuf[AdrOffsetTemp]=BufTemp[0];
	CH_VFD_LeaveCriction();
/*end yxl 2007-03-15*/


	return res;

}


BOOL CH_CellControl(CH_CELL_TYPE CellType,CH_CELL_STATUS CellStatus)
{
	BOOL res;
	U8 AdrOffsetTemp;
	U8 BitMaskTemp;

	switch(CellType)
	{
	case CELL_MUTE:
		AdrOffsetTemp=(U8)MUTE_ADDRESS_OFFSET;
		BitMaskTemp=(U8)MUTE_BIT_MASK;
		break;
	case CELL_USB:
		AdrOffsetTemp=(U8)USB_ADDRESS_OFFSET;
		BitMaskTemp=(U8)USB_BIT_MASK;
		break;
	case CELL_MP3:
		AdrOffsetTemp=(U8)MP3_ADDRESS_OFFSET;
		BitMaskTemp=(U8)MP3_BIT_MASK;
		break;		
	case CELL_HD:
		AdrOffsetTemp=(U8)HD_ADDRESS_OFFSET;
		BitMaskTemp=(U8)HD_BIT_MASK;
		break;	
	case CELL_LEFTARROW:
		AdrOffsetTemp=(U8)LEFTARROW_ADDRESS_OFFSET;
		BitMaskTemp=(U8)LEFTARROW_BIT_MASK;
		break;			
	case CELL_CH:
		AdrOffsetTemp=(U8)CH_ADDRESS_OFFSET;
		BitMaskTemp=(U8)CH_BIT_MASK;
		break;	
	case CELL_POINTA:
		AdrOffsetTemp=(U8)POINTA_ADDRESS_OFFSET;
		BitMaskTemp=(U8)POINTA_BIT_MASK;
		break;			
	case CELL_POINTB:
		AdrOffsetTemp=(U8)POINTB_ADDRESS_OFFSET;
		BitMaskTemp=(U8)POINTB_BIT_MASK;
		break;		
	case CELL_1080I:
		AdrOffsetTemp=(U8)T1080I_ADDRESS_OFFSET;
		BitMaskTemp=(U8)T1080I_BIT_MASK;
		break;	
	case CELL_720P:
		AdrOffsetTemp=(U8)T720P_ADDRESS_OFFSET;
		BitMaskTemp=(U8)T720P_BIT_MASK;
		break;			
	case CELL_576P:
		AdrOffsetTemp=(U8)T576P_ADDRESS_OFFSET;
		BitMaskTemp=(U8)T576P_BIT_MASK;
		break;	
	case CELL_576I:
		AdrOffsetTemp=(U8)T576I_ADDRESS_OFFSET;
		BitMaskTemp=(U8)T576I_BIT_MASK;
		break;			
	case CELL_HDMI:
		AdrOffsetTemp=(U8)HDMI_ADDRESS_OFFSET;
		BitMaskTemp=(U8)HDMI_BIT_MASK;
		break;	
	case CELL_VGA:
		AdrOffsetTemp=(U8)VGA_ADDRESS_OFFSET;
		BitMaskTemp=(U8)VGA_BIT_MASK;
		break;	
	case CELL_YPBPR:
		AdrOffsetTemp=(U8)YPBPR_ADDRESS_OFFSET;
		BitMaskTemp=(U8)YPBPR_BIT_MASK;
		break;	
	case CELL_VIDEO:
		AdrOffsetTemp=(U8)VIDEO_ADDRESS_OFFSET;
		BitMaskTemp=(U8)VIDEO_BIT_MASK;
		break;	
	case CELL_TV:
		AdrOffsetTemp=(U8)TV_ADDRESS_OFFSET;
		BitMaskTemp=(U8)TV_BIT_MASK;
		break;	
	case CELL_RADIO:
		AdrOffsetTemp=(U8)RADIO_ADDRESS_OFFSET;
		BitMaskTemp=(U8)RADIO_BIT_MASK;
		break;	
	case CELL_TUNER0:
		AdrOffsetTemp=(U8)TUNER0_ADDRESS_OFFSET;
		BitMaskTemp=(U8)TUNER0_BIT_MASK;
		break;	
	case CELL_LINE:
		AdrOffsetTemp=(U8)LINE_ADDRESS_OFFSET;
		BitMaskTemp=(U8)LINE_BIT_MASK;
		break;
	case CELL_POWER:
		AdrOffsetTemp=(U8)POWER_ADDRESS_OFFSET;
		BitMaskTemp=(U8)POWER_BIT_MASK;
		break;
	default:
		STTBX_Print(("\nYxlInfo:undefined cell type"));
		return TRUE;
	}

	res=CH_VFD_CellBaseControl(CellStatus,AdrOffsetTemp,BitMaskTemp);
	return res;

}


BOOL CH_VFD_WriteNumber(int StartPos,int EndPos,U32 Number)
{
	BOOL res;
	U8 BufTemp[50];
	U8 BufTempOld[50];
	
	int i=0;
	int j=0;
	
	int SpaceTemp=0;
	U32 ValueTemp=Number;

	U8 DiditalTemp[10];
	U8 DigIndex;

	if(EndPos>StartPos)
	{
		SpaceTemp=EndPos-StartPos+1;
	}

	memset((void*)DiditalTemp,0,10);
	while(ValueTemp)
	{

		DiditalTemp[i]=(U8)(ValueTemp%10);
		i++;
		ValueTemp=ValueTemp/10;
	}


#if 1/*yxl 2007-03-15 add */

	if(i<SpaceTemp)
	{
		j=SpaceTemp-i;
		while(j)
		{
			DiditalTemp[i]=0;
			i++;
			j--;
		}
	}

#endif /*end yxl 2007-03-15 add */
	DigIndex=(U8)i;

#if 1 /*yxl 2007-03-15 add */
	CH_VFD_EnterCriction();
	memcpy((void*)BufTempOld,(const void*)&gVFDContentBuf[StartPos*3+3],3*i);
	CH_VFD_LeaveCriction();
#endif

	for(j=0;j<i;j++)
	{
		BufTemp[3*j]=DigitalToVFDData[DiditalTemp[DigIndex-1]].byte.ucByte0;
		BufTemp[3*j+1]=DigitalToVFDData[DiditalTemp[DigIndex-1]].byte.ucByte1;
		BufTempOld[3*j+1]=BufTempOld[3*j+1]&0xC0;
		BufTemp[3*j+1]=BufTemp[3*j+1]&0x3F;
		BufTemp[3*j+1]=BufTemp[3*j+1]|BufTempOld[3*j+1];
		BufTemp[3*j+2]=0x0;
		DigIndex--;
	}
	
	res=CH_VFD_WriteData(StartPos*3+3,3*i,BufTemp);
	
#if 1 /*yxl 2007-03-15 add */
	CH_VFD_EnterCriction();
	memcpy((void*)&gVFDContentBuf[StartPos*3+3],BufTemp,3*i);
	CH_VFD_LeaveCriction();
#endif 

	return res;
}


BOOL CH_VFD_LOginOff(void)
{
	return CH_VFD_SendCommand(CONTROL_VFD_LOGIN_OFF,0,NULL);
}


BOOL CH_VFD_DisplayProgNumber(U32 ProgNum)
{
	BOOL res;
	U32 ProgNumTemp=ProgNum;

	res=CH_CellControl(CELL_CH,DISPLAY_ON);
	res=CH_CellControl(CELL_POINTA,DISPLAY_ON);
	CH_VFD_ShowChar(3,3," ");/*20070810 add*/
	res=CH_VFD_WriteNumber(0,2,ProgNum);

	return res;

}

BOOL CH_VFD_DisplayTime(U32 Time)
{
	BOOL res;
	U32 TimeTemp=Time;

	res=CH_CellControl(CELL_POINTB,DISPLAY_ON);

	res=CH_VFD_WriteNumber(5,8,TimeTemp);

	return res;

}



/*字符和VFD显示数据对照表*/
#define MAX_VFDCHAR_NUM             63      /*最大VFD显示的字符个数*/
typedef struct CH_VFD_CharTable
{
	char       c_VFDChar;                   /*字符*/
	SHORT2BYTE uni_VFDCharToData;           /*字符对应的VFD显示数据*/
}CH_VFD_CharTable_t;
/* 功能：VFD字符对照表                                              */
/* 值域：定义的数值                                                 */
/* 应用环境：1:在函数CH_VFD_GetCharTableIndex和CH_VFD_ShowChar中调用*/
/* 注意事项：                                                       */
CH_VFD_CharTable_t gCH_VFD_CharTable[MAX_VFDCHAR_NUM] =
{
    '0',0x3123,
	'1',0x1020,
	'2',0x30c3,
	'3',0x30e1,
	'4',0x11e0,
	'5',0x21e1,
	'6',0x21e3,
	'7',0x3020,
	'8',0x31e3,
	'9',0x31e1,
	'a',0x30e3,
	'b',0x01e3,
	'c',0x00c3,
	'd',0x10e3,
	'e',0x31c3,
	'f',0x2142,
	'g',0x31e1,
	'h',0x01e2,
	'i',0x0102,
	'j',0x1025,
	'k',0x0c18,
	'l',0x0106,
	'm',0x00ea,
	'n',0x00e2,
	'o',0x00e3,
	'p',0x3488,
	'q',0x31e0,
	'r',0x00c8,
	's',0x2211,
	't',0x04d8,
	'u',0x050b,
	'v',0x0023,
	'w',0x002b,
	'x',0x0a14,
	'y',0x11e1,
	'z',0x2805,
	'A',0x31e2,
	'B',0x31e3,
	'C',0x2103,
	'D',0x3429,
	'E',0x21c3,
	'F',0x21c2,
	'G',0x21a3,
	'H',0x11e2,
	'I',0x2409,
	'J',0x3025,
	'K',0x0952,
	'L',0x0103,
	'M',0x1b2a,
	'N',0x1332,
	'O',0x3123,
	'P',0x31c2,
	'Q',0x3133,
	'R',0x31e2,
	'S',0x21e1,
	'T',0x2408,
	'U',0x1123,
	'V',0x0a08,
	'W',0x1536,
	'X',0x0a14,
	'Y',0x11c8,
	'Z',0x2843,
	' ',0x0000
};


/*函数名:int CH_VFD_GetCharTableIndex(char rc_VFDchar)*/
/*开发人和开发时间:zengxianggen 2007-05-23                        */
/*函数功能描述:在指定位置显示字符串信息                           */
/*函数原理说明:在表中查找                                         */
/*输入参数：      
rc_VFDchar，需要查找的字符                                        */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构： gCH_VFD_CharTable                    */
/*调用的主要函数列表：                                            */
/*返回值说明：返回对应字符在表中的位置,-1为查找失败               */ 
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
int CH_VFD_GetCharTableIndex(char rc_VFDchar)
{
	int i;
	for(i = 0;i < MAX_VFDCHAR_NUM;i++)
	{
		if(gCH_VFD_CharTable[i].c_VFDChar == rc_VFDchar)
		{
			return i;
		}
	}
	return -1;
}



/************************函数说明***********************************/
/*函数名:boolean CH_VFD_ShowChar(U8 ruc_StartPos,U8 ruc_EndPos,char *rpc_String)*/
/*开发人和开发时间:zengxianggen 2007-05-23                        */
/*函数功能描述:在指定位置显示字符串信息                           */
/*函数原理说明:                                                   */
/*输入参数：
U8 ruc_StartPos:在VFD中显示字符串起始位置
U8 ruc_EndPos:在VFD中显示字符串结束位置 
char *rpc_String:需要显示的字符串信息                             */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：CH_VFD_WriteData,CH_VFD_GetCharTableIndex   */
/*返回值说明：TRUE,操作失败,FALSE,操作成功                        */ 
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
boolean CH_VFD_ShowChar(U8 ruc_StartPos,U8 ruc_EndPos,char *rpc_String)
{
	
	int i_Len1;
	int i_Len2;
	int i_Len;
	
	U8 u_BufTemp[50];
	U8 u_BufTempOld[50];
	
	int i = 0;
	
	
	U8 i_VFDCharIndex[12];
	
	/*判断输入参数的正确性*/
	if(rpc_String == NULL)
	{
		return true;
	}
	
	if(ruc_StartPos > 8)
	{
		ruc_StartPos = 8;
	}
	
	if(ruc_EndPos > 8)
	{
		ruc_EndPos = 8;
	}
    
	if(ruc_StartPos > ruc_EndPos)
	{
        return true;
	}
	
	i_Len1 = ruc_EndPos - ruc_StartPos + 1;
	i_Len2 = strlen(rpc_String);
	
	if(i_Len2 < i_Len1)
	{
		i_Len = i_Len2;
	}
	else 
	{
		i_Len = i_Len1;
	}
    /*得到需要显示的字符数据索引*/    	
	for(i = 0;i < i_Len;i++)	
		i_VFDCharIndex[i] = CH_VFD_GetCharTableIndex(rpc_String[i]);
	
	/*得到VFD buffer中原来的数据*/		
	CH_VFD_EnterCriction();
	memcpy((void*)u_BufTempOld,(const void*)&gVFDContentBuf[ruc_StartPos * 3 + 3],3 * i_Len);
	CH_VFD_LeaveCriction();
	/*****************************/
	
	for(i = 0;i < i_Len;i++)
	{
		u_BufTemp[3 * i] = gCH_VFD_CharTable[i_VFDCharIndex[i]].uni_VFDCharToData.byte.ucByte0;
		u_BufTemp[3 * i + 1] = gCH_VFD_CharTable[i_VFDCharIndex[i]].uni_VFDCharToData.byte.ucByte1;
		/*	u_BufTempOld[3 * i + 1] = u_BufTempOld[3 * i + 1]&0xC0;
		u_BufTemp[3 * i + 1] = u_BufTemp[3 * i + 1]&0x3F;
		u_BufTemp[3 * i + 1] = u_BufTemp[3 * i + 1]|u_BufTempOld[3 * i + 1];*/
		u_BufTemp[3 * i + 2] = 0x0;
	}
	/*写新的数据到VFD BUFFER*/
	CH_VFD_WriteData(ruc_StartPos * 3 + 3,3 * i_Len,u_BufTemp);
	
	/*更新数据BUFFER*/
	CH_VFD_EnterCriction();
	memcpy((void*)&gVFDContentBuf[ruc_StartPos * 3 + 3],u_BufTemp,3 * i_Len);
	CH_VFD_LeaveCriction();
	
}

/*End of file*/


















