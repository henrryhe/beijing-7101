/*******************文件说明注释************************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：ch_serialNum.c                                           */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-07-20                                             */
/*模块功能:
  STi7109/710 串口写机号和配对数据                                 */
/*主要函数及功能:                                                  */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*          20070720    zxg       从STi5107移植到STi7101平台,
            删除备用机号处理,只需要主机号                          */
/*******************************************************************/

/**************** #include 部分*************************************/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>   
#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"


#include "Stuart.h"
#include "graphicbase.h" 
#include "..\main\initterm.h"
/******************************************************************/
/**************** #define 部分*************************************/
#define     PAIRING_ADDR         0xA003FC00
#define     STBID_ADDR           (PAIRING_ADDR+512)    /*机号在FLASH中的存贮位置*/

#define     TBX_UART_DEVICE      2
#define     DATA_UART_RXD_DEV    4
#define     DATA_UART_TXD_DEV    4
#define     DATA_UART_RXD_BIT    PIO_BIT_2
#define     DATA_UART_TXD_BIT    PIO_BIT_3
#if 1
#define     USE_MAC             /*fj 20080228 add*/
#endif
/***************  全局变量定义部分 ********************************/
STUART_Handle_t GlobalUartHandle=0;
extern          STGFX_Handle_t	GFXHandle;
/***************************函数体定义*****************************/ 
/******************************************************************/
/*函数名:BOOL CH_OpenMyUART( void )                               */
/*开发人和开发时间:zengxianggen 2007-07-21                        */
/*函数功能描述:打开串口UART                                       */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：TRUE, 失败,FALSE,成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
BOOL CH_OpenMyUART( void )
{
    ST_ErrorCode_t ErrCode;
    STUART_InitParams_t InitParams;
    STUART_Params_t UartDefaultParams;
    STUART_OpenParams_t OpenParameters;	
    STUART_TermParams_t TermParams;
	if(GlobalUartHandle==0)
	{
		TermParams.ForceTerminate = TRUE;
		ErrCode = STUART_Term(UART_DeviceName[TBX_UART_DEVICE],&TermParams); /* */
		CH_SetReport(0);
		
		UartDefaultParams.RxMode.DataBits = STUART_8BITS_NO_PARITY;
		UartDefaultParams.RxMode.StopBits = STUART_STOP_1_0;
		UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL;
		UartDefaultParams.RxMode.BaudRate = 115200;
		UartDefaultParams.RxMode.TermString = NULL;
		UartDefaultParams.RxMode.TimeOut = 1;
		UartDefaultParams.RxMode.NotifyFunction = NULL;
		
		UartDefaultParams.TxMode.DataBits = STUART_8BITS_NO_PARITY;
		UartDefaultParams.TxMode.StopBits = STUART_STOP_1_0;
		UartDefaultParams.TxMode.FlowControl = STUART_NO_FLOW_CONTROL;
		UartDefaultParams.TxMode.BaudRate = 115200;
		UartDefaultParams.TxMode.TermString = NULL;
		UartDefaultParams.TxMode.TimeOut = 0;
		UartDefaultParams.TxMode.NotifyFunction = NULL;
		UartDefaultParams.SmartCardModeEnabled = FALSE;
		UartDefaultParams.GuardTime = 0;
		
		InitParams.UARTType        = STUART_16_BYTE_FIFO;
		InitParams.DriverPartition = SystemPartition;
		InitParams.BaseAddress     = (U32 *) ASC_2_BASE_ADDRESS;
		InitParams.InterruptNumber = ASC_2_INTERRUPT;
		InitParams.InterruptLevel  =  ASC_2_INTERRUPT_LEVEL;
		InitParams.ClockFrequency  = gClockInfo.CommsBlock;
		InitParams.SwFIFOEnable    = TRUE;
		InitParams.FIFOLength      = 1024;
		
		strcpy(InitParams.RXD.PortName, PIO_DeviceName[DATA_UART_RXD_DEV]);
		InitParams.RXD.BitMask     = DATA_UART_RXD_BIT;
		
		strcpy(InitParams.TXD.PortName, PIO_DeviceName[DATA_UART_TXD_DEV]);
		InitParams.TXD.BitMask     = DATA_UART_TXD_BIT;
		InitParams.DefaultParams = &UartDefaultParams;
		
		ErrCode = STUART_Init( "MyUart", &InitParams );
		
		if(ErrCode==0)
		{
			OpenParameters.LoopBackEnabled = FALSE;
			OpenParameters.FlushIOBuffers = TRUE;
			OpenParameters.DefaultParams = NULL;    /* Don't overwrite default parameters set at init */
			
			ErrCode = STUART_Open( "MyUart", &OpenParameters, &GlobalUartHandle);
			
			
		}	
		
	}
	else
	{
		CH_SetReport(0);
	}
    return ( ErrCode == ST_NO_ERROR ? FALSE:TRUE );
	
}



/******************************************************************/
/*函数名:void CH_Loader_VidPrintfColor(int x,int y,
         int font_color,const char *format, ...)                                                                  */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:显示状态信息                                       */
/*函数原理说明:                                                   */
/*输入参数：int x,显示X坐标位置
            int y,显示Y坐标位置
            int font_color,显示颜色
            const char *format, 显示字符信息                      */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：无                                                  */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */  
void CH_Loader_VidPrintfColor(int x,int y,int font_color,const char *format, ...)
{
{

	char s1[256]="";
#if 0
	va_list v1;
	va_start (v1, format);
	vsprintf (s1, format, v1);
#endif	
#if 1	
	CH6_DisRectTxtInfo(GFXHandle, CH6VPOSD.ViewPortHandle,x,y,400,100,format,0x9950,font_color,1,1);
#else
	CH6_PopupDialog (GFXHandle,CH6VPOSD.ViewPortHandle,format,0,0,0);
#endif
}
}
#ifdef USE_MAC
void CH_ReadSTBAndMACID(U8* puc_STBID)
{
 	memcpy(puc_STBID,(U8 *)(STBID_ADDR),14);/*fj 20080228 mod 8-->14*/
}

void CH_ReadSTBID(U8* puc_STBID)
{
 	memcpy(puc_STBID,(U8 *)(STBID_ADDR+6),8);/*fj 20080228 mod 8-->14*/
}
void CH_ReadSTBMac(U8* MacADD)
{
	U8 tempmac[6];
	int loopi;
	memcpy(tempmac,(U8 *)STBID_ADDR,6);/*20080229 add*/
	for(loopi = 0;loopi < 6;loopi++)
		MacADD[loopi] = tempmac[5-loopi];
}
#else
/******************************************************************/
/*函数名:void CH_ReadSTBID(U8* puc_STBID)                         */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:得到机号信息                                       */
/*函数原理说明:                                                   */
/*输入参数：                                                      */
/*输出参数: U8* puc_STBID,STBID数据信息                           */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：无                                                  */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
void CH_ReadSTBID(U8* puc_STBID)
{
 	memcpy(puc_STBID,(U8 *)STBID_ADDR,8);/*fj 20080228 mod 8-->14*/
}

#endif
/*得到MAC地址信息
void CH_ReadMACAddr(U8*  r_MainMACAddr)
{		
    memcpy(r_MainMACAddr,(U8 *)MAC_ADDR,6);
}*/


/******************************************************************/
/*函数名:U32 CH_LDR_ReadSTBID( void )                             */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:得到机号显示信息                                   */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：返回机号显示信息                                   */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
U32 CH_LDR_ReadSTBID( void )
{
    U32   ST_STBID=0; 
#ifdef USE_MAC
	U8    STBID[14];/*fj 20080228 mod 8-->14*/
#else
	U8    STBID[8];/*fj 20080228 mod 8-->14*/
#endif
	CH_ReadSTBID(STBID);
	ST_STBID=STBID[5]|(STBID[4]<<8)|(STBID[3]&0xF)<<16;
	
	if(0xFFFFF != (ST_STBID & 0xFFFFF ))
	{
		return ST_STBID;
	}
	else
	{
	   return 0;
	}
}

int CH_UrtReadChipID(void)
{
	/*U8 tmp1[] = "Uart begin...\n";	*/
	U8              ReadData[1000],tmp;
	U8              SendCommand[1000];
	int             i,k,WithMainSerialNum=0; 
	boolean         errorcode;
	int             err;
	U32             NumberRead;
	U32             NumberWritten;
#ifdef USE_MAC
        U8              STB_Id[14],STB_Idbak[14]; /*fj 20080227 mod 8--->14*/
#else
        U8              STB_Id[8],STB_Idbak[8]; /*fj 20080227 mod 8--->14*/
#endif
	int             wr_errcode;
	int             timer_count = 0;
	int             UrtConnect=0;
	
        CH_OpenMyUART();

        CH_ReadSTBAndMACID(STB_Idbak);
	for (i=0;i<14;i++)
	{
		STB_Id[i] = STB_Idbak[13-i];
	}
    
        CH6_ClearFullArea(GFXHandle,CH6VPOSD.ViewPortHandle);
        CH_Loader_VidPrintfColor(150,110,0xFFFF,  "connecting.......");
        CH_UpdateAllOSDView();

        for(i=5;i<14;i++)
        {
                if(STB_Id[i]!=0xff)
                    break;
        }

        if( i >= 14 ) 
        {
                CH_Loader_VidPrintfColor(150,110,0xFFFF,  "no STBID In the box,Failed.");
                CH_UpdateAllOSDView();
                return -1;
        }  
	   
        STUART_Flush(GlobalUartHandle);
        
	/*send the ack info to the serial port*/
	SendCommand[0] = 0xaa;
	SendCommand[1] = 0x46;
	SendCommand[2] = 0x08;
			
			
        {
            U8 NUIDKey[16];
            int i;
            U8 pCRC[4];
            U32 CRC;
            U32 CheckNum;
            U8  pCheckNum[4];
            U8 CRCData[16];
            U8 CA_SN[4];
            
            /*得到NUID CHECK NUM,NUID*/
            {
                    int i_times = 5;
                    int i_status;
                    
                        do
                        {
                            i_status = CH_GetNUID((U8 *)NUIDKey,(U32 *)&CheckNum);
                            i_times --;
                        }while( i_status == false && i_times > 0);

                        if(i_status == false)/*读取NUID 失败*/
                        {
                            CH_Loader_VidPrintfColor(150,110,0xFAAA,  "Read NUID failed,Please try again");
                            CH_UpdateAllOSDView();
                            return -1;
                        }
            }

            pCheckNum[0]=(CheckNum>>24)&0xFF;
            pCheckNum[1]=(CheckNum>>16)&0xFF;
            pCheckNum[2]=(CheckNum>>8)&0xFF;
            pCheckNum[3]=CheckNum&0xFF;
#if 0
            NUIDKey[12]=0x26;
            NUIDKey[13]=0xA5;
            NUIDKey[14]=0xC6;
            NUIDKey[15]=0xB7;

            pCheckNum[0]=0xA2;
            pCheckNum[1]=0xD3;
            pCheckNum[2]=0x56;
            pCheckNum[3]=0x4C;
#endif

            /*得到CA_SN*/
            CH_GetPairCASerial(CA_SN);
#if 0
            CA_SN[0]=0x42;
            CA_SN[1]=0x9A;
            CA_SN[2]=0x7C;
            CA_SN[3]=0x95;
#endif


            /*计算CRC*/
            memset(CRCData,0,16);
#if 0
            CRCData[12]=0x00;
            CRCData[13]=0x15;
            CRCData[14]=0x73;
            CRCData[15]=0x78;
#endif
            memcpy(CRCData,CA_SN,4);
            memcpy(&CRCData[4],&NUIDKey[12],4);
            memcpy(&CRCData[8],pCheckNum,4);

            CRC=crc_ISO_3309(CRCData,16);

            pCRC[0]=(CRC>>24)&0xFF;
            pCRC[1]=(CRC>>16)&0xFF;
            pCRC[2]=(CRC>>8)&0xFF;
            pCRC[3]=CRC&0xFF;
            
            /*传送CRC*/
            memcpy(&SendCommand[3],pCRC,4);
            /*pCRC 20070831 change*/SendCommand[7]=0;
            /*传送check number*/
            memcpy(&SendCommand[8],pCheckNum,4);
            /*pCRC*/SendCommand[12]=0;
            /*传送NUID*/
            memcpy(&SendCommand[13],&NUIDKey[12],4);
            /*pCRC 20070831 change*/SendCommand[17]=0;
            /*传送STB_SN*/
            memcpy(&SendCommand[18],STB_Id,8);
            /*pCRC 20070831 change*/SendCommand[26]=0;
            /*传送CA_SN*/
            memcpy(&SendCommand[27],CA_SN,4);
            SendCommand[31]=0;
            /*传送效验和*/
            SendCommand[32]=0;
            
            for(i=0;i<32;i++)
            {
                SendCommand[32]+=SendCommand[i];
            }
        }
        
        /********************/
#if 0/*20090823 change*/
        STUART_Flush(GlobalUartHandle);
        STUART_Write(GlobalUartHandle,SendCommand,33,&NumberWritten,0);

        CH_Loader_VidPrintfColor(150,110,0xFAAA,  "操作成功");			 
#else
        {
                char   End_String[8] = {"OK_END"};

                for(i=0;i<10;i++)
                {
                        STUART_Flush(GlobalUartHandle);
                        STUART_Write(GlobalUartHandle,SendCommand,33,&NumberWritten,0);

                        errorcode = STUART_Read(GlobalUartHandle,ReadData,/*11*/8,&NumberRead,500);
                        if((NumberRead == 8) && (memcmp(ReadData,End_String,6 ) == 0) )
                        {
                                CH_Loader_VidPrintfColor(150,110,0xFAAA,  "SUCCESS !");
                                CH_UpdateAllOSDView();
                                return 1;
                        }
                }
        }
#endif

	CH_Loader_VidPrintfColor(150,110,0xFAAA,  "ERROR!");
	CH_UpdateAllOSDView();
	return -1;
}


/*函数名:int CH_UrtComm2Host (void)                                  */
/*开发人和开发时间:zengxianggen 2007-07-20                        */
/*函数功能描述:串口和计算机通讯处理                               */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：-1,操作失败,1操作成功                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
int CH_UrtComm2Host (void)
{
	/*U8 tmp1[] = "Uart begin...\n";	*/
	U8              ReadData[1000],tmp;
	U8              SendCommand[1000];
	int             i,k,WithMainSerialNum=0; 
	boolean         errorcode;
	int             err;
	U32             NumberRead;
	U32             NumberWritten;
#ifdef USE_MAC
    U8              STB_Id[14],STB_Idbak[14]; /*fj 20080227 mod 8--->14*/
#else
	U8              STB_Id[8],STB_Idbak[8]; /*fj 20080227 mod 8--->14*/
#endif
	int             wr_errcode;
	int             timer_count = 0;
	int             UrtConnect=0;
	
	  CH_OpenMyUART();

	   CH_ReadSTBAndMACID(STB_Id);

       CH6_ClearFullArea(GFXHandle,CH6VPOSD.ViewPortHandle);

	   CH_Loader_VidPrintfColor(150,110,0xFFFF,  "connecting.......");
	   CH_UpdateAllOSDView();
	   
	   for(i=6;i<14;i++)
	   {
		   if(STB_Id[i]!=0xff)
			   break;
	   }
	   
	   WithMainSerialNum=0;
	   if( i< 14 ) 
	   {
		   err = 4;
		   printf("the stbid has already been written into flash\n");
		   
		   /*STBID_Display(STB_Id,0,err);
		   return -1;*/
		   WithMainSerialNum=100;/*已经写了主机号*/
	   }  
	   
	   STUART_Flush(GlobalUartHandle);
	   /*STUART_Write(GlobalUartHandle,tmp1,12,&NumberWritten,0);*/
	   /*require data command*/
	   SendCommand[0] = 0xaa;
	   SendCommand[1] = 0x44;
	   SendCommand[2] = 0x01;
	   SendCommand[3] = 0x08;
	   SendCommand[4] = SendCommand[0]+SendCommand[1]+SendCommand[2]+SendCommand[3];
	   
	   /*建立握手循环*/ 
	   for(i=0;i<30;i++)
	   {
		   STUART_Write(GlobalUartHandle,SendCommand,5,&NumberWritten,0);
		   
		   errorcode = STUART_Read(GlobalUartHandle,ReadData,1,&NumberRead,500);	
		   if( ( ReadData[0] == 0xaa ) && ( errorcode == 0 ))
		   {
			   UrtConnect = 100;
			   break;
		   }
	   }
	   
	   if(UrtConnect == 0 )
	   {
		   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "connect failed,please try again");
		   CH_UpdateAllOSDView();
		   return -1;
	   }
	   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "waiting for the order");
	   CH_UpdateAllOSDView();
	   while ( TRUE )
	   {
		   /*read the stb id data from the serial port*/
		   /*张强修改循环握手模式*/		
		   
		   tmp = ReadData[0];
#ifdef USE_MAC
		   errorcode = STUART_Read(GlobalUartHandle,ReadData,/*11*/525+6,&NumberRead,0);
#else
		   errorcode = STUART_Read(GlobalUartHandle,ReadData,/*11*/525,&NumberRead,0);
#endif
		   
#ifdef USE_MAC
		   for(i=0;i</*10*/524+6;i++)
#else
		   for(i=0;i</*10*/524;i++)
#endif
			   tmp = tmp + ReadData[i]; 
		   
		   /*check sum*/
#ifdef USE_MAC
		   if((tmp == ReadData[524+6/*10*/])&&(ReadData[0]==0x57)&&(ReadData[1]==0x8)) 
#else
		   if((tmp == ReadData[524/*10*/])&&(ReadData[0]==0x57)&&(ReadData[1]==0x8)) 
#endif
		   {
			   /*写PK DATA 512 BYTE*/
			   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "Waiting for  PK DATA...");
			   CH_UpdateAllOSDView();
			   if(FLSH_DRV_Write(&ReadData[11],PAIRING_ADDR,512)==true)
			   {
				   /*写PK DATA 出错*/
				   CH_Loader_VidPrintfColor(150,110,0xFAAA,  " PK DATA is failed");
				   CH_UpdateAllOSDView();
				   return -1;
				   
			   }
			   
			   if(memcmp((U8 *)PAIRING_ADDR,&ReadData[11],512)==0)
			   {
				   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "PK DATA is Successful!");
                               CH_UpdateAllOSDView();
			   }
			   else
			   {
				   /*写PK DATA 出错*/
				   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "Write  PK DATA  failed !");
				   CH_UpdateAllOSDView();
				   return -1;
			   }
			   /********************/
			   task_delay(ST_GetClocksPerSecond()/2);
			   /* When receive the stb id data from the serial port, 
			   write the stb id to the flash*/
			   /*写机号*/
			   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "Writing STBID and MAC");
			   CH_UpdateAllOSDView();
			   task_delay(ST_GetClocksPerSecond()/2);
			   
			   /*if(WithMainSerialNum == 100)*/
			   {	
				#ifdef USE_MAC
				   for(i=0;i<14;i++)
				   {
				       if(i<8)
				       	{
					       STB_Idbak[13-i] = ReadData[2+i];
				       	}
					   else if(i>=8)
					   	{
					   	   STB_Idbak[13-i] = ReadData[524-8+i];
					   	}
					   			   
					 //  STB_Idbak[7-i] = ReadData[2+i];
				   }
				   FLSH_DRV_Write(STB_Idbak,STBID_ADDR,14);/*写机号,写MAC地址*/
#else
				   for(i=0;i<8;i++)
				   {
					   STB_Idbak[7-i] = ReadData[2+i];
				   }
				   FLSH_DRV_Write(STB_Idbak,STBID_ADDR,8);/*写机号,写MAC地址*/
#endif

			   }
			   
			   
			   /*if(WithMainSerialNum == 100)*/
			   {
#ifdef USE_MAC
				   memcpy(STB_Idbak,(U8 *)STBID_ADDR,14);
				   for (i=0;i<14;i++)
				   {
					   STB_Id[i] = STB_Idbak[13-i];
				   }
#else
				   memcpy(STB_Idbak,(U8 *)STBID_ADDR,8);
				   for (i=0;i<8;i++)
				   {
					   STB_Id[i] = STB_Idbak[7-i];
				   }
#endif
			   
			   }
			   
			   
#ifdef USE_MAC 
			   if(!memcmp(STB_Id,&ReadData[2],8) && !memcmp(&STB_Id[8],&ReadData[524],6))
#else
			   if(!memcmp(STB_Id,&ReadData[2],8))
#endif
			   {
				   /*the stb id is good*/
				   err = 0; 
			   }
			   else
			   {
				   int    j;
				   
				   
				   /*  if(WithMainSerialNum == 100)*/
				   {
                      				CH_Loader_VidPrintfColor(150,110,0xFAAA,  "Writing STBID and MAC failed");
						CH_UpdateAllOSDView();
						task_delay(ST_GetClocksPerSecond());
						return -1;
				   }
				   
			   }
		   }
		   else
		   {
			   /*no write any data to flash*/
#ifdef USE_MAC
			   memset(STB_Id,0xff,14);
#else
			   memset(STB_Id,0xff,8);
#endif
			   err = 3;
			   
			   CH_Loader_VidPrintfColor(150,110,0xFAAA,  "The operation is failed,Please try again");
			   CH_UpdateAllOSDView();
			   return -1;
			   
		   }
				
			
			/*send the ack info to the serial port*/
			
			SendCommand[0] = 0xaa;
			SendCommand[1] = 0x46;
			SendCommand[2] = 0x08;
			
			
			{
				U8 NUIDKey[16];
				int i;
				U8 pCRC[4];
				U32 CRC;
				U32 CheckNum;
				U8  pCheckNum[4];
				U8 CRCData[16];
				U8 CA_SN[4];
				/*得到NUID CHECK NUM,NUID*/
				{
					   int i_times = 5;
					   int i_status;
	                              do
	                              {
					      i_status = CH_GetNUID((U8 *)NUIDKey,(U32 *)&CheckNum);
					      i_times --;
	                              }while( i_status == false && i_times > 0);
					 			  
					 if(i_status == false)/*读取NUID 失败*/
				 	 {
                                        	CH_Loader_VidPrintfColor(150,110,0xFAAA,  "Read NUID failed,Please try again");
						   CH_UpdateAllOSDView();
						 return -1;
				 	 }
				}
				
				pCheckNum[0]=(CheckNum>>24)&0xFF;
				pCheckNum[1]=(CheckNum>>16)&0xFF;
				pCheckNum[2]=(CheckNum>>8)&0xFF;
				pCheckNum[3]=CheckNum&0xFF;
#if 0
				NUIDKey[12]=0x26;
				NUIDKey[13]=0xA5;
				NUIDKey[14]=0xC6;
				NUIDKey[15]=0xB7;

				pCheckNum[0]=0xA2;
				pCheckNum[1]=0xD3;
				pCheckNum[2]=0x56;
				pCheckNum[3]=0x4C;
#endif

				/*得到CA_SN*/
				CH_GetPairCASerial(CA_SN);
#if 0
				CA_SN[0]=0x42;
				CA_SN[1]=0x9A;
				CA_SN[2]=0x7C;
				CA_SN[3]=0x95;
#endif


				/*计算CRC*/
				memset(CRCData,0,16);
#if 0
				CRCData[12]=0x00;
				CRCData[13]=0x15;
				CRCData[14]=0x73;
				CRCData[15]=0x78;
#endif
				memcpy(CRCData,CA_SN,4);
				memcpy(&CRCData[4],&NUIDKey[12],4);
				memcpy(&CRCData[8],pCheckNum,4);
				
				CRC=crc_ISO_3309(CRCData,16);

				pCRC[0]=(CRC>>24)&0xFF;
				pCRC[1]=(CRC>>16)&0xFF;
				pCRC[2]=(CRC>>8)&0xFF;
				pCRC[3]=CRC&0xFF;
				/*传送CRC*/
				memcpy(&SendCommand[3],pCRC,4);
				/*pCRC 20070831 change*/SendCommand[7]=0;
				/*传送check number*/
				memcpy(&SendCommand[8],pCheckNum,4);
				/*pCRC*/SendCommand[12]=0;
				/*传送NUID*/
				memcpy(&SendCommand[13],&NUIDKey[12],4);
				/*pCRC 20070831 change*/SendCommand[17]=0;
				/*传送STB_SN*/
				memcpy(&SendCommand[18],STB_Id,8);
				/*pCRC 20070831 change*/SendCommand[26]=0;
				/*传送CA_SN*/
                		memcpy(&SendCommand[27],CA_SN,4);
				SendCommand[31]=0;
				/*传送效验和*/
				SendCommand[32]=0;
				for(i=0;i<32;i++)
				{
					SendCommand[32]+=SendCommand[i];
				}
			}
			/********************/
#if 0/*20090823 change*/
			STUART_Flush(GlobalUartHandle);
                     STUART_Write(GlobalUartHandle,SendCommand,33,&NumberWritten,0);
					 
		       CH_Loader_VidPrintfColor(150,110,0xFAAA,  "操作成功");			 
#else
                            {
                            char   End_String[8] = {"OK_END"};
                            
                                for(i=0;i<10;i++)
                                {
                                        STUART_Flush(GlobalUartHandle);
                                        STUART_Write(GlobalUartHandle,SendCommand,33,&NumberWritten,0);
	
				errorcode = STUART_Read(GlobalUartHandle,ReadData,/*11*/8,&NumberRead,0);
                                        if((NumberRead == 8) && (memcmp(ReadData,End_String,6 ) == 0) )
                                        {
                                    		 CH_Loader_VidPrintfColor(150,110,0xFAAA,  "SUCCESS !");
	                                        CH_UpdateAllOSDView();
                                    		return 1;
                                        }
                                }
                            }
#endif
			break;
		}
		CH_Loader_VidPrintfColor(150,110,0xFAAA,  "ERROR!");
		CH_UpdateAllOSDView();
		return -1;
}
