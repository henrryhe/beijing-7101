/*******************文件说明注释************************************/
/*公司版权说明：版权（2006）归长虹网络科技有限公司所有。           */
/*文件名：ch_pt6964.c                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2006-02-08                                             */
/*模块功能:通过PT6964 完成LED和前面板按键控制                      */
/*主要函数及功能:                                                  */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*  
				  ***1***
                  6      2
                  *      *
                  ***7****
                  5      3
                  *      *
                  ***4****  .8      

  Modifiction: 
  Date:2006-05-03 by 易晓莉 
  目的：和ATMEL控制的LED 兼容，最大程度保持应用程序不变。
  1、修改函数BOOL DisplayTimeLED(void)，增加参数TDTTIME DisTime。
  2、修改函数boolean CH_PUB_PTinit(void)，将按键的初始化独立出来。
  3、增加全局变量IsTunerLigthOn，并修改函数BOOL StandbyLedOn()和BOOL StandbyLedOff(),用以记录当前
		TUNER 灯的状态。

  4、新增加下列函数BOOL LEDDisplay(U8* Content)，BOOL FrontKeybordInit(void)，
	 BOOL DisplayFreqLED(int Freq)，BOOL GetTunerLightStatus(void)


                                                                    */
/*******************************************************************/

/**************** #include 部分*************************************/
#include "stddefs.h"  /* standard definitions           */
#include "stevt.h"
#include "stlite.h"   /* os20 for standard definitions, */

#include "initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "appltype.h"

#include "keymap.h"
#include "ch_time.h"
#include "ch_pt6964.h"

void CH_PUB_ClearPTSTB(void);
void CH_PUB_Senddata(U8 ruc_Data); 
void CH_PUB_PTSetData(U8 ruc_command,U8 *rp_Data,int ri_Len);
void CH_PUB_Scan(void);

/*******************************************************************/
/**************** #define 部分**************************************/
#define PT_REFRESH_MAX         10
#define PT_MODE                0x02
#define PT_DUTY_CYCLE            /*0x8f降低亮度*/0x8A
#define PT_ADDRESS_INC         0x40
#define PT_ADDRESS_FIX         0x44
#define PT_ADDRESS_MASK        0xc0
#define PT_READ_KEY            0x42
#define PT_SW_KEY              0x43
#define PT_READ_SW             0x43

#define TASK_DELAY              ST_GetClocksPerSecond()/1000000

#define PT_STB_BIT            0x02
#define PT_CLK_BIT            0x04
#define PT_DAINOUT_BIT        0x08
#define PT_YLED_BIT           0x10

#define MAX_PT_CHARINDEX      9      /*最大的数字字符索引*/
#define FIRST_ADDR            0x06   /*千位地址*/
#define TWO_ADDR              0x04   /*百位地址*/
#define THREE_ADDR            0x02   /*十位地址*/
#define FOUR_ADDR             0x00   /*个位地址*/
#define GREENLED_ADDR         0x05   /*GREEN LED 地址*/
#define REDLED_ADDR           0x09   /*RED LED 地址*/
#define LED_ADDR              0x05
#define PT_KEYSCAN_LEN        0x02   /*两个扫描线*/
/*******************************************************************/

/***************  全局变量定义部分 *********************************/
STPIO_Handle_t gST_PTHandle;
STPIO_Handle_t gST_PTHandleDataInOut;


U8             *gp_PTtask_stack;
semaphore_t*    pgST_PTSema;
/*对应的0~9字符数据*/
U8             guc_LedData[]=
{
 0x3F,/*0x30*/0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F
};
static      BYTE suc_LastKey=0xFF;            /*上次的键值*/
static      int  si_SamekeyCount=0;        /*相同的键值次数*/


#if 0 /*yxl 2007-03-07 move this variable to file ch_LEDmid.c*/  
static boolean IsTunerLigthOn=false;/*yxl 2006-05-03 add this variable,
	false stand for tuner light has been turned off,
	false stand for tuner light has been turned on*/
#endif

static message_queue_t  *pstUsifMsgQueue;/*yxl 2006-05-05 add this line*/

/*******************************************************************/
/************************函数说明***********************************/
/*函数名:BYTE CH_PUB_PTNumToSeg (int ri_index)              */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:把数字转换为对应的SegMent数据                       */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */

BYTE CH_PUB_PTNumToSeg (int ri_index)
/*int ri_index，需要转换的数字*/
{
	if(ri_index > MAX_PT_CHARINDEX)  
		ri_index = MAX_PT_CHARINDEX;
	return guc_LedData[ri_index];
}


/************************函数说明***********************************/
/*函数名:staic void CH_PUB_PTProcess(void *rp_vParam)              */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964处理进程                                       */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                  */
static void CH_PUB_PTProcess(void *rp_vParam)  
{
#if 0 /*yxl 2006-05-03 cancel below section*/
    int i;
	int j;
	U8  uc_Data1[1]={0x00};
	U8  uc_Data2[1]={0xFF};
	semaphore_wait(pgST_PTSema);
	/*首先设置为显示模式，并且地址固定*/
	CH_PUB_PTSetData(PT_ADDRESS_FIX,NULL,0);
	/*循环清空所有显示数据*/
    for(i=0;i<PT_REFRESH_MAX;i++)
	{
      CH_PUB_PTSetData(PT_ADDRESS_MASK|i,uc_Data1,1);/*设置对应的地址,并且清空该内存数据*/
	}
    /*设置显示模式*/
   	CH_PUB_PTSetData(PT_MODE,NULL,0);
	/*设置显示控制命令,0x88~0x8F,*/
    CH_PUB_PTSetData(PT_DUTY_CYCLE,NULL,0)/*PULS WIDTH=1/16,DISPLAY ON*/;
	semaphore_signal(pgST_PTSema);
    
#endif   /*end yxl 2006-05-03 cancel below section*/

	while(true)
	{
		/*循环扫描KEY*/
         task_delay(ST_GetClocksPerSecond()/20);/*50ms*/
		 /*开始读*/
         semaphore_wait(pgST_PTSema);
         /*设置为读模式*/
		 CH_PUB_ClearPTSTB();
         CH_PUB_Senddata(PT_READ_KEY);
         task_delay(1000);
         CH_PUB_Scan();
		 semaphore_signal(pgST_PTSema);

	}
}

/************************函数说明***********************************/
/*函数名:boolean CH_PUB_PTinit(void)                               */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964初始化                                        */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：false：初始化成功，true：初始化失败                  */
boolean CH_PUB_PTinit(void) 
{
    ST_ErrorCode_t ErrCode;
    boolean b_result=false;
  	STPIO_OpenParams_t    ST_OpenParams;

	ST_OpenParams.ReservedBits      = PIO_BIT_1|PIO_BIT_2|PIO_BIT_4;
	ST_OpenParams.BitConfigure[1]  = STPIO_BIT_OUTPUT;           /*STB*/
	ST_OpenParams.BitConfigure[2]  = STPIO_BIT_OUTPUT;           /*CLK*/
	ST_OpenParams.BitConfigure[4]  = STPIO_BIT_OUTPUT;           /*YELLOW LED*/
	ST_OpenParams.IntHandler       = NULL;         

	ErrCode=STPIO_Open(PIO_DeviceName[5], &ST_OpenParams, &gST_PTHandle );
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\n STPIO_Open()=s",GetErrorText(ErrCode)));
		return TRUE;
	}


	ST_OpenParams.ReservedBits      =PIO_BIT_3;
	ST_OpenParams.BitConfigure[3]  =/* STPIO_BIT_OUTPUT|STPIO_BIT_INPUT*/STPIO_BIT_BIDIRECTIONAL;          /*DIN_OUT*/
	ST_OpenParams.IntHandler       = NULL;     
	ErrCode=STPIO_Open(PIO_DeviceName[5], &ST_OpenParams, &gST_PTHandleDataInOut );
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\n STPIO_Open()=s",GetErrorText(ErrCode)));
		return TRUE;
	}

	#if 1 /*yxl 2006-05-03 add below section*/
	{
		int i;
		int j;
		U8  uc_Data1[1]={0x00};
		U8  uc_Data2[1]={0xFF};
		
		/*首先设置为显示模式，并且地址固定*/
		CH_PUB_PTSetData(PT_ADDRESS_FIX,NULL,0);
		/*循环清空所有显示数据*/
		for(i=0;i<PT_REFRESH_MAX;i++)
		{
			CH_PUB_PTSetData(PT_ADDRESS_MASK|i,uc_Data1,1);/*设置对应的地址,并且清空该内存数据*/
		}
		/*设置显示模式*/
		CH_PUB_PTSetData(PT_MODE,NULL,0);
		/*设置显示控制命令,0x88~0x8F,*/
		CH_PUB_PTSetData(PT_DUTY_CYCLE,NULL,0)/*PULS WIDTH=1/16,DISPLAY ON*/;

	}

#endif   /*end yxl 2006-05-03 add below section*/


     pgST_PTSema=semaphore_create_fifo(1);




	/*******************/
	return b_result;
}



/************************函数说明***********************************/
/*函数名:void CH_PUB_ClearPTSTB(void)                              */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964 STB置低位                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_ClearPTSTB(void) 
{
    ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Clear(gST_PTHandle,PT_STB_BIT);
   	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
	}
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_SetPTSTB(void)                                */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964 STB置高位                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_SetPTSTB(void) 
{
  ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Set(gST_PTHandle,PT_STB_BIT);
  	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
	}
   task_delay(TASK_DELAY);
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_ClearPTCLK(void)                              */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964 CLK置低位                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_ClearPTCLK(void) 
{
     ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Clear(gST_PTHandle,PT_CLK_BIT);
   if(ErrCode!=ST_NO_ERROR)
   {
	   STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
   }
   task_delay(TASK_DELAY);
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_SetPTCLK(void)                                */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964 CLK置高位                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_SetPTCLK(void) 
{
     ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Set(gST_PTHandle,PT_CLK_BIT);
   if(ErrCode!=ST_NO_ERROR)
   {
	   STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
   }
   task_delay(TASK_DELAY);
}
/************************函数说明***********************************/
/*函数名:void CH_PUB_ClearPTDAINOUT(void)                             */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964 DAIN置低位                                   */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_ClearPTDAINOUT(void) 
{
    ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Clear(gST_PTHandleDataInOut,PT_DAINOUT_BIT);
   if(ErrCode!=ST_NO_ERROR)
   {
	   STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
   }
   task_delay(TASK_DELAY);
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_SetPTDAINOUT(void)                               */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:PT6964 DAINOUT置高位                                   */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_SetPTDAINOUT(void) 
{
    ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Set(gST_PTHandleDataInOut,PT_DAINOUT_BIT);
   if(ErrCode!=ST_NO_ERROR)
   {
	   STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
   }
   task_delay(TASK_DELAY);
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_ClearYLED(void)                               */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述: YELLOW LED 置低位                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_ClearYLED(void) 
{
    ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Clear(gST_PTHandle,PT_YLED_BIT);
   if(ErrCode!=ST_NO_ERROR)
   {
	   STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
   }
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_SetYLED(void)                                 */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:YELLOW LED置高位                                    */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_SetYLED(void) 
{
    ST_ErrorCode_t ErrCode;
	ErrCode=STPIO_Set(gST_PTHandle,PT_YLED_BIT);
   if(ErrCode!=ST_NO_ERROR)
   {
	   STTBX_Print(("\n STPIO_Clear()=s",GetErrorText(ErrCode)));
   }
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_Senddata(U8 ruc_Data)                         */
/*开发人和开发时间:zengxianggen 2006-02-09                         */
/*函数功能描述:发送一个数据或命令                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_Senddata(U8 ruc_Data) 
/*U8 ruc_Data,要发送的数据或命令*/
{
	int i; 
	BYTE uc_bTmp = ruc_Data; 
    for (i = 0; i < 8; i++)
	{
		CH_PUB_ClearPTCLK();
		if(uc_bTmp & 0x1)
		{
           CH_PUB_SetPTDAINOUT() ;
		}
		else
		{
           CH_PUB_ClearPTDAINOUT() ;
		}
		uc_bTmp >>= 1;
		CH_PUB_SetPTCLK(); 
	} 
    CH_PUB_SetPTDAINOUT() ; 
}

/************************函数说明***********************************/
/*函数名:void CH_PUB_STBSenddata(U8 ruc_Data)                         */
/*开发人和开发时间:zengxianggen 2006-02-09                         */
/*函数功能描述:发送一个STB命令                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_STBSenddata(U8 ruc_Data) 
/*U8 ruc_Data,要发送的STB命令*/
{
   CH_PUB_ClearPTSTB();
   CH_PUB_Senddata(ruc_Data); 
   CH_PUB_SetPTSTB();
}




/************************函数说明***********************************/
/*函数名:void CH_PUB_PTSetData(U8 ruc_command,U8 *rp_Data,int ri_Len)*/
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:传送相应命令和数据                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_PUB_PTSetData(U8 ruc_command,U8 *rp_Data,int ri_Len)
/*************************参数注释说明*******************************/
/*U8 ruc_command:要传送的命令
  U8 *rp_Data:要传送的数据指针
  int ri_Len: 要传送的数据长度*/
{
	int i;
	int i_tempdata;
	U8  uc_Tmp;

	/*首先传送命令,STB*/
	CH_PUB_ClearPTSTB();
    CH_PUB_Senddata(ruc_command);
	/*传送ri_Len个数据*/
	while(rp_Data!=NULL&&ri_Len>0)
	{        
		uc_Tmp=*rp_Data;
		CH_PUB_Senddata(uc_Tmp);
		ri_Len--;
		rp_Data++;
	}
    CH_PUB_SetPTSTB();
	

}

/************************函数说明***********************************/
/*函数名:BYTE CH_PT_Recv(void) */
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:传送相应命令和数据                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：接收到的扫描值                                       */
BYTE CH_PT_Recv(void) 
{
	BYTE i, uc_KeyData,uc_tempdata;
	
	uc_KeyData = 0;
	
	for(i = 0; i < 8; i++)
	{
		CH_PUB_ClearPTCLK();
		uc_KeyData <<= 1;
		CH_PUB_SetPTCLK();		
		/* delay for data output ready */
		/*task_delay(ST_GetClocksPerSecond)/5000;*/
		STPIO_Read(gST_PTHandleDataInOut, &uc_tempdata);
		if (uc_tempdata&PT_DAINOUT_BIT)
		{
			uc_KeyData |= 1;
		}
	}
	
	return (uc_KeyData);
}
/************************函数说明***********************************/
/*函数名:void CH_PUB_Scan(void)*/
/*开发人和开发时间:zengxianggen 2006-02-08                         */
/*函数功能描述:扫描键                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：接收到的扫描值                                       */

void CH_PUB_Scan(void)
{
	BYTE i, j,  uc_RecvByte,uc_key;
	usif_cmd_t* str_pMsg;
	clock_t     st_timeout;
	
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
	/*	do_report(0,"no Key pressed\n");*/
	}
	else/*有键值*/
	{
		if(suc_LastKey!=uc_key)
		{
			st_timeout = ST_GetClocksPerSecond()*2 ;
			si_SamekeyCount=0;
			if ((str_pMsg = (usif_cmd_t *)message_claim_timeout(pstUsifMsgQueue,/*TIMEOUT_INFINITY*/&st_timeout)) != NULL)
			{
				str_pMsg->from_which_module = KEYBOARD_MODULE;
				str_pMsg->contents.keyboard.scancode = uc_key;
				str_pMsg->contents.keyboard.device = FRONT_PANEL_KEYBOARD;
				str_pMsg->contents.keyboard.repeat_status =FALSE;
				message_send(pstUsifMsgQueue, str_pMsg);
				suc_LastKey=uc_key;
/*				do_report(0,"have Key=%d\n",uc_key);	  */
			}
		}
		else
		{
			if(si_SamekeyCount>3)
			{
				st_timeout = ST_GetClocksPerSecond()*2 ;
				si_SamekeyCount=0;
				if ((str_pMsg = (usif_cmd_t *)message_claim_timeout(pstUsifMsgQueue,/*TIMEOUT_INFINITY*/&st_timeout)) != NULL)
				{
					str_pMsg->from_which_module = KEYBOARD_MODULE;
					str_pMsg->contents.keyboard.scancode = uc_key;
					str_pMsg->contents.keyboard.device = FRONT_PANEL_KEYBOARD;
					str_pMsg->contents.keyboard.repeat_status =FALSE;
					message_send(pstUsifMsgQueue, str_pMsg);
					suc_LastKey=uc_key;
/*					do_report(0,"have Key=%d\n",uc_key);*/	  
				}
			}
			si_SamekeyCount++;
		}
	}
}

/*下列函数的书写格式是为了兼容老的程序*/

void DisplayLEDMode(U32 num,int mode)
{
	U8 TempMode=mode;
	U8 uc_WriteData[4];
	int i;
	semaphore_wait(pgST_PTSema);
	switch( TempMode)
	{
	case 0: /*display ProgNo*/
		{
			uc_WriteData[0]	 = CH_PUB_PTNumToSeg ( num / 1000 );          /*千位*/

			uc_WriteData[1]	 = CH_PUB_PTNumToSeg ( (num % 1000) / 100 );  /*百位*/
			uc_WriteData[2]	 = CH_PUB_PTNumToSeg ( (num % 100) / 10 );    /*十位*/

			uc_WriteData[3]    = CH_PUB_PTNumToSeg ( num % 10 );          /*个位*/
			CH_PUB_PTSetData(PT_MODE,NULL,0);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FIRST_ADDR,&uc_WriteData[0],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|TWO_ADDR,  &uc_WriteData[1],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|THREE_ADDR,&uc_WriteData[2],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FOUR_ADDR, &uc_WriteData[3],1);
		}
		break;
	case 1: /*display SN */
		{
			uc_WriteData[0]	 = CH_PUB_PTNumToSeg ( num / 1000 );        /*千位*/
			uc_WriteData[1]	 = CH_PUB_PTNumToSeg ( (num % 1000) / 100 );/*百位*/
			uc_WriteData[2]	 = CH_PUB_PTNumToSeg ( (num % 100) / 10 );  /*十位*/
			uc_WriteData[3]  = CH_PUB_PTNumToSeg ( num % 10 );          /*个位*/
			CH_PUB_PTSetData(PT_MODE,NULL,0);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FIRST_ADDR,&uc_WriteData[0],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|TWO_ADDR,  &uc_WriteData[1],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|THREE_ADDR,&uc_WriteData[2],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FOUR_ADDR, &uc_WriteData[3],1);
		}
		break;
	case 2: /*display time */
		{
			uc_WriteData[0]	 = CH_PUB_PTNumToSeg ( num / 1000 );          /*千位*/

			uc_WriteData[1]	 = CH_PUB_PTNumToSeg ( (num % 1000) / 100 )|0x80;  /*百位*/
			uc_WriteData[2]	 = CH_PUB_PTNumToSeg ( (num % 100) / 10 )|0x80;    /*十位*/

			uc_WriteData[3]    = CH_PUB_PTNumToSeg ( num % 10 );          /*个位*/
			CH_PUB_PTSetData(PT_MODE,NULL,0);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FIRST_ADDR,&uc_WriteData[0],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|TWO_ADDR,  &uc_WriteData[1],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|THREE_ADDR,&uc_WriteData[2],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FOUR_ADDR, &uc_WriteData[3],1);
			
		}
		break;
	case 3:	/* display search freq */
		{
			uc_WriteData[0]	 = 0x71;/*F字符*/                            /*千位*/
			uc_WriteData[1]	 = CH_PUB_PTNumToSeg ( (num % 1000) / 100 ); /*百位*/
			uc_WriteData[2]	 = CH_PUB_PTNumToSeg ( (num % 100) / 10 );   /*十位*/
			uc_WriteData[3]    = CH_PUB_PTNumToSeg ( num % 10 );         /*个位*/
			CH_PUB_PTSetData(PT_MODE,NULL,0);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FIRST_ADDR,&uc_WriteData[0],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|TWO_ADDR,  &uc_WriteData[1],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|THREE_ADDR,&uc_WriteData[2],1);
			CH_PUB_PTSetData(PT_ADDRESS_MASK|FOUR_ADDR, &uc_WriteData[3],1);
		}
		break;
	}
	semaphore_signal(pgST_PTSema);
}

#if 0 /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  
/*显示节目号*/
void DisplayLED(U32 num)
{
#if 0 /*yxl 2007-02-21 temp cancel for debug*/
	DisplayLEDMode(num,0);
#else
	return;
#endif
}
#endif /*end yxl 2007-03-07 move this function to file ch_LEDmid.c*/  


#if 0 /*yxl 2006-05-03 modify below function*/
/*显示时间*/
BOOL DisplayTimeLED(void)
{
	TDTTIME TempTime;
	U32  hm = 0;
	GetCurrentTime(&TempTime);		
	hm = (TempTime.ucHours* 100) + TempTime.ucMinutes;
	if(hm ==0xffffffff)
		return TRUE;
	
	DisplayLEDMode(hm,2);	
	return FALSE;	
}

#else
#if 0 /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  
boolean DisplayTimeLED(TDTTIME TempTime)
{
#if 0 /*yxl 2007-02-21 temp cancel for debug*/
	U32  hm = 0;

	hm = (TempTime.ucHours* 100) + TempTime.ucMinutes;
	if(hm ==0xffffffff)
		return TRUE;
	
	DisplayLEDMode(hm,2);	
#else
#endif
	return FALSE;	
}
#endif  /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  
#endif /*end yxl 2006-05-03 modify below function,Add para 	TDTTIME DisTime*/


#if 0 /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  
/*待机开*/
BOOL StandbyLedOn()
{  
	U8 uc_WriteData[1]={0x01};
	semaphore_wait(pgST_PTSema);
    CH_PUB_PTSetData(PT_MODE,NULL,0);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|LED_ADDR,&uc_WriteData[0],1);/*绿灯亮*/
	semaphore_signal(pgST_PTSema);
	return true;
	
}
/*待机关*/
BOOL StandbyLedOff()
{
	U8 uc_WriteData[1]={0x02};
	semaphore_wait(pgST_PTSema);
    CH_PUB_PTSetData(PT_MODE,NULL,0);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|LED_ADDR,&uc_WriteData[0],1);/*绿灯灭*/
	semaphore_signal(pgST_PTSema);
	
	return true;/*for test*/		
}

/*电源指示关*/
BOOL PowerLightOff(void)
{
	U8 uc_WriteData[1]={0x01};
	semaphore_wait(pgST_PTSema);
    CH_PUB_PTSetData(PT_MODE,NULL,0);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|LED_ADDR,&uc_WriteData[0],1);/*红灯灭*/
	semaphore_signal(pgST_PTSema);
	return TRUE;
}
/*电源指示开*/
BOOL PowerLightOn()
{
	U8 uc_WriteData[1]={0x02};
	semaphore_wait(pgST_PTSema);
    CH_PUB_PTSetData(PT_MODE,NULL,0);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|LED_ADDR,&uc_WriteData[0],1);/*红灯亮*/
	semaphore_signal(pgST_PTSema);
	return TRUE;
}
/*TUNER指示关*/
BOOL TunerLightOff()
{

	CH_PUB_ClearYLED();
	IsTunerLigthOn=FALSE;/*yxl 2006-05-03 add this line*/
	return TRUE;
}
/*TUNER指示开*/
BOOL TunerLightOn()
{
	CH_PUB_SetYLED();
	IsTunerLigthOn=TRUE;/*yxl 2006-05-03 add this line*/
	return TRUE;
}

BOOL DisplayLEDString(char* pDisString)
{	
	return FALSE;
}


/*yxl 2006-05-03 add below function*/

BOOL LEDDisplay(U8* Content)
{
	U8 uc_WriteData[4];
	int i;

	memcpy((void*)uc_WriteData,(const void*)Content,4);
	semaphore_wait(pgST_PTSema);
	CH_PUB_PTSetData(PT_MODE,NULL,0);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|FIRST_ADDR,&uc_WriteData[0],1);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|TWO_ADDR,  &uc_WriteData[1],1);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|THREE_ADDR,&uc_WriteData[2],1);
	CH_PUB_PTSetData(PT_ADDRESS_MASK|FOUR_ADDR, &uc_WriteData[3],1);
	semaphore_signal(pgST_PTSema);

	return FALSE;
}

#endif  /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  

/*end yxl 2006-05-03 add below function*/

/*yxl 2006-05-01 add below fuctions*/
BOOL FrontKeybordInit(void)
{

	int i_result;
	int i_stacksize=1024;
#if 0
	/*yxl 2006-05-05 add below section*/
	if ( symbol_enquire_value ( "usif_queue", ( void ** ) &pstUsifMsgQueue ) )
	{
   
		do_report ( 0 , "Cant find USIF message queue\n" );

		return   TRUE;
	}
	/*end yxl 2006-05-05 add below section*/
#endif 
	gp_PTtask_stack=memory_allocate(CHSysPartition,i_stacksize);
	if(gp_PTtask_stack==NULL)
	{
		return true;
	}
/*	i_result = task_init( CH_PUB_PTProcess,NULL,gp_PTtask_stack, i_stacksize,&gST_PTtask, &gST_desc_pt,5,"pt_process", 0);	
	
	if(i_result!=0)
	{		
		STTBX_Print(("Failed to create pt process task\n"));
		return true ;
	}
	else
		do_report( 0,"pt process ok\n" );
*/
	return false;
}
/*end yxl 2006-05-01 add below fuctions*/


#if 0 /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  

/*yxl 2006-05-01 add below fuctions*/
BOOL DisplayFreqLED(int Freq)
{
#if 0 /*yxl2007-02-21 temp cancel for debug*/
	DisplayLEDMode(Freq,3);
#endif 
	return FALSE;
}
/*end yxl 2006-05-01 add below fuctions*/

/*yxl 2006-05-01 add below fuctions*/
BOOL GetTunerLightStatus(void)
{

	return IsTunerLigthOn;
}
/*end yxl 2006-05-01 add below fuctions*/

#endif  /*yxl 2007-03-07 move this function to file ch_LEDmid.c*/  

/*********************************************/



