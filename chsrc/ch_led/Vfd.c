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
#include "Vfd.h"
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


static U8 gVFDContentBuf[MAX_VFD_OFFSET+1]; 
static Semaphore_t SemVFDBufLock;   




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

	ErrCode=STI2C_Write(VFDHandleW,BufTemp,NumTemp+1,TimeOut, &NumWriten);
	if(ErrCode!=ST_NO_ERROR)
	{
		
		STTBX_Print(("\nYxlInfo:STI2C_Write()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

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

	ErrCode=STI2C_Read(VFDHandleW,BufTemp,NumTemp,TimeOut, &NumRead);
	if(ErrCode!=ST_NO_ERROR)
	{
		
		STTBX_Print(("\nYxlInfo:STI2C_Read()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
	else
	{
		STTBX_Print(("\nCount=%d Value=%x %x ",NumRead,BufTemp[0],BufTemp[1]));
	}

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


	
	/*更新数据BUFFER*/
	CH_VFD_EnterCriction();
	memcpy((void*)&gVFDContentBuf[ruc_StartPos * 3 + 3],u_BufTemp,3 * i_Len);
	CH_VFD_LeaveCriction();
	
}

/*End of file*/


















