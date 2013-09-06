/*******************文件说明注释************************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：ch_produce.c                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-01-16                                             */
/*模块功能:设置生产状态菜单                                        */
/*主要函数及功能:                                                  */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/* 20070720    zxg       从STi5107移植到STi7101平台                */
/*******************************************************************/

/**************** #include 部分*************************************/
#include "appltype.h"
#include "..\main\initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "string.h"
#include "stsys.h"	
#include "..\report\report.h"
#include "..\key\keymap.h"
#include "..\dbase\vdbase.h"
/*******************************************************************/
/**************** #define 部分**************************************/
typedef enum
{
  WRITE_PAIRING,
  WRITE_STBID,
  READ_STB_INFO,
  ENABLE_BOOT_ROM,
  ENABLE_JTAG_LOCK,
  ENABLE_OTP,
  AUTO_SETTING
#ifdef STBID_FROM_E2PROM
  ,MENUAL_SET_STBID
#endif  
}CH_ControlCMD_t;

#ifdef STBID_FROM_E2PROM
#define MENU_MAX_ITEMS	  8
#else
#define MENU_MAX_ITEMS	7
#endif
#if 1
#define USE_MAC  /*fj 20080228 add to use mac*/
#endif
void CH_Draw_produce_menu( S8 sCurrKey, S8 sPrevKey, 	U8 uMaxCount, char ItemName[MENU_MAX_ITEMS][30]  );
void CH_DisplaySTBInfo( void );
void CH_ManualSetSTBID(void);


/*******************************************************************/

/***************  全局变量定义部分 *********************************/

/*******************************************************************/
#ifdef ADD_SMART_CONTROL

/*20061022 add 控制智能卡的操作状态，位置为E2PROM中CA数据的前一个字节*/
/************************函数说明***********************************/
/*函数名:void CH_GetSmartStatus(void)                                              */
/*开发人和开发时间:zengxianggen 2006-10-22                         */
/*函数功能描述:初始化控制SMART的状态                          */
/*函数原理说明:                                                                          */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                                     */
/*返回值说明：无               */
static boolean sb_NeedCard=true;

/************************函数说明***********************************/
/*函数名:void CH_GetSmartStatus(void)                                              */
/*开发人和开发时间:zengxianggen 2006-10-22                         */
/*函数功能描述:得到控制SMART的状态                          */
/*函数原理说明:                                                                          */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                                     */
/*返回值说明： 返回卡的控制状态             */
boolean CH_GetSmartControlStatus(void)
{
      return sb_NeedCard;
}

/************************函数说明***********************************/
/*函数名:void CH_GetSmartStatus(void)                                              */
/*开发人和开发时间:zengxianggen 2006-10-22                         */
/*函数功能描述:设置控制SMART的状态                          */
/*函数原理说明:                                                                          */
/*使用的全局变量、表和结构：                                     */
/*调用的主要函数列表：                                                     */
/*返回值说明： 返回卡的控制状态             */
void CH_SetSmartControlStatus(boolean NeedCard)
{
   		U8 SmartValue;
     		 if(NeedCard==true)
	      	{
			  sb_NeedCard=true;
	      	}
	  	else
	  	{
		  sb_NeedCard=false;
	  	}
}
#endif
/************************函数说明***********************************/
/*函数名:void CH_ProduceMenu(void)            */
/*开发人和开发时间:zengxianggen 2007-01-15                         */
/*函数功能描述:显示生产设置主菜单                                      */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */	
/*返回值说明：无                                                   */
void CH_ProduceMenu(void)
{
	U8				ItemCount=MENU_MAX_ITEMS;
	char 	        ItemNameList[MENU_MAX_ITEMS][30]=
	{
		     /*"状态查询",*/
			"Serial mode",
			"Read ChipID",//reset the stb
			"change to no card Mode",
			"Auto Enable security",
			"Enable BOOT ROM",
			"Enable JTAG LOCK",
			"Enable OTP"
#ifdef STBID_FROM_E2PROM
            ,"write the stbid"
#endif		
	};
	S8				sCurrKey, sPrevKey;
	int				iKeyRead;
	boolean 			bRedraw,b_result;
	
	
	
	/* 初始化键值 */
	sCurrKey	= 0;
	sPrevKey	= -1;
	iKeyRead	= -1;
	bRedraw	= true;
#ifdef CH_MUTI_RESOLUTION_RATE
{
       CH_DeleteOSDViewPort_HighSolution();
       CH_SetReSolutionRate(CH_OLD_MOE);
	CH_CreateOSDViewPort_HighSolution();
	
}
#endif

	CH_Secure_Init();

	/* 进入消息循环 */
	while( 1 )
	{
		/* 处理重画 */
		if( bRedraw == true )
		{
		#if 1
			bRedraw	= false;
			CH_Draw_produce_menu( sCurrKey, sPrevKey, ItemCount, ItemNameList );
			CH_UpdateAllOSDView();  
			CH_DisplaySTBInfo();
		#endif	
		}
		
		/* 等待键值 */
		iKeyRead	= GetKeyFromKeyQueue( -1 );
		
		/* 根据键值处理 */
		switch( iKeyRead )
		{
		case EXIT_KEY:			/* 退出 */
		{
#if 0				
			CH_Draw_produce_menu( -1, -1, ItemCount, ItemNameList );
			CH_UpdateAllOSDView( );
#endif				
#ifdef CH_MUTI_RESOLUTION_RATE
			{
				CH_DeleteOSDViewPort_HighSolution();
				CH_SetReSolutionRate(CH_1280X720_MODE);
				CH_CreateOSDViewPort_HighSolution();	
			}
#endif
		CH_Secure_Term();
		return;
		}
			break;
		case UP_ARROW_KEY:		/* 向上 */
			if( sCurrKey == 0 )
			{
				/* 判断翻页 */
				if( ItemCount > 1 )
				{
					sPrevKey	= sCurrKey;
					sCurrKey	= ItemCount - 1;
					bRedraw	= true;
				}
			}
			else
			{
				sPrevKey	= sCurrKey;
				sCurrKey --;
				bRedraw	= true;
			}
			break;
			
		case DOWN_ARROW_KEY:	/* 向下 */
			
			if( sCurrKey == ItemCount - 1 )
			{
				/* 判断翻页 */
				if( ItemCount > 1 )
				{
					sPrevKey		= sCurrKey;
					sCurrKey 	= 0;
					bRedraw		= true;
				}
			}
			else
			{
				sPrevKey	= sCurrKey;
				sCurrKey ++;
				bRedraw = true;
			}
			break;
			
		case PAGE_UP_KEY:		/* 向前翻页 */
			{
				if( sCurrKey < MENU_MAX_ITEMS )
				{
					sPrevKey		= sCurrKey;
					sCurrKey		= 0;
					bRedraw		= true;
				}
				else
				{
					sPrevKey		= sCurrKey;
					sCurrKey		-= MENU_MAX_ITEMS;
					bRedraw		= true;
				}
			}
			break;
			
		case PAGE_DOWN_KEY:	/* 向后翻页 */
			{
				if( ItemCount - sCurrKey > MENU_MAX_ITEMS )
				{
					sPrevKey 	= sCurrKey;
					sCurrKey		+= MENU_MAX_ITEMS;
					bRedraw		= true;
				}
				else
				{
					sPrevKey		= sCurrKey;
					sCurrKey		= ItemCount - 1;
					bRedraw		= true;
				}
			}
			break;
		case OK_KEY:			/* 进入 */
			{
				switch(sCurrKey)
				{

				case 0:/*进入串口设置模式*/
					CH_UrtComm2Host();
                                  	CH_SetReport(1);
					while(true)
					{
					 	iKeyRead	= GetKeyFromKeyQueue( -1 );
						if(iKeyRead!=-1)
						{
							break;
						}
					}

					/* 初始化键值 */
					sCurrKey	= 0;
					sPrevKey	= -1;
					iKeyRead	= -1;
					bRedraw	= true;
					CH_DisplaySTBInfo();
					CH_UpdateAllOSDView( );
					break;
				case 1:/*读出NUID Report号*/

					CH_UrtReadChipID();
                                  	         CH_SetReport(1);
		
					while(true)
					{
					 	iKeyRead	= GetKeyFromKeyQueue( -1 );
						if(iKeyRead!=-1)
						{
							break;
						}
					}

					/* 初始化键值 */
					sCurrKey	= 1;
					sPrevKey	= -1;
					iKeyRead	= -1;
					bRedraw	= true;
					CH_DisplaySTBInfo();
					CH_UpdateAllOSDView( );
                   
					break;
				case 2:/*切换卡模式*/
          
					if(CH_GetSmartControlStatus()==true)
					{
						CH_SetSmartControlStatus(false);
						
					}
					bRedraw	= true;
					
					break;
				case 3:/*"Enable BOOT ROM,JTAG LOCK,OTP"*/
					/**/
					if(CH6_InputLcokPinCode(true) == 0)
						{
							CH_EnableBootAuthenticaion();
							CH_EnableJTAGLOCK();
							CH_EnableOTPBootCode();
							CH_DisplaySTBInfo();
							CH_UpdateAllOSDView( );
						}
					 break;
				case 4:/*	"Enable BOOT ROM"*/
		   		    if(CH6_InputLcokPinCode(true) == 0)
					{
					/**/
                                    CH_EnableBootAuthenticaion();					
					CH_DisplaySTBInfo();
					CH_UpdateAllOSDView( );
					}
					 break;
				case 5:/* "Enable JTAG LOCK"*/
		   		    if(CH6_InputLcokPinCode(true) == 0)
		   		    	{
					/**/
                                   CH_EnableJTAGLOCK();				
					CH_DisplaySTBInfo();
					CH_UpdateAllOSDView( );
		   		    	}
					 break;
				case 6:/*"Enable OTP"*/
		   		    if(CH6_InputLcokPinCode(true) == 0)
		   		    	{
		                            CH_EnableOTPBootCode();
						CH_DisplaySTBInfo();
						CH_UpdateAllOSDView( );
		   		    	}
					break;
#ifdef STBID_FROM_E2PROM					 
			    case 7:
					CH_ManualSetSTBID();
					break;
#endif							
				}
			}
			break;
			
	}
  }
}

#if 1 	/*<!-- ZQ 2009/9/5 19:18:29 --!>*/
void CH_CheckForEnterProMenu( void )
{
	int				iKeyRead;
    
        /* 等待键值 */
        iKeyRead	=GetMilliKeyFromKeyQueue(200) ;

        /* 根据键值处理 */
        switch( iKeyRead )
        {
        case KEY_E:
            ipanel_DisableDebugControl();/*关茁壮打印*/
            CH_ProduceMenu();
        break;

        default:
            
        break;
        }
  
        return ;
}
#endif 	/*<!-- ZQ 2009/9/5 19:18:29 --!>*/

/************************函数说明***********************************/
/*函数名:void CH_Draw_produce_menu( S8 sCurrKey, S8 sPrevKey,
 	U8 uMaxCount, char ItemName[MENU_MAX_ITEMS][30]  )
            */
/*开发人和开发时间:zengxianggen 2007-01-15                         */
/*函数功能描述:画生产设置菜单                                     */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */	
/*返回值说明：无                                                   */

void CH_Draw_produce_menu( S8 sCurrKey, S8 sPrevKey, 	U8 uMaxCount, char ItemName[MENU_MAX_ITEMS][30]  )
{
	U16	uBannerStartX, uBannerStartY;
	U16	uBannerWidth, uBannerHeight, uBannerBorderWidth;
	U32	uBannerColor, uBannerBorderColor, uTextColor;
	U16	uItemStartX, uItemStartY;
	U16 uItemWidth, uItemHeight;
	U8	uTextGap, uIndex;
	boolean	bDrawAllMenu;
	U16	sPx, sPy;
	U8	uScrStart, uScrEnd;
#ifdef OSD_COLOR_TYPE_RGB16
	uBannerBorderColor	= 0xffff;
	uBannerColor			= 0xa992;
	uTextColor			= 0xc2dc;
#else
       uBannerBorderColor	= 0xfff8f8f8;
	uBannerColor			= 0xff506090;
	uTextColor			= 0xff80b0e0;
#endif
	uBannerStartX		= 150;
	uBannerStartY			= 260;

	uBannerWidth			= 300;
	uBannerHeight		= 230;

	uBannerBorderWidth	= 2;
	
	/* 处理退出画 */
	if( sCurrKey == -1 && sPrevKey == -1 )
	{
		CH6_ClearFullArea(GFXHandle,CH6VPOSD.ViewPortHandle);
		return;
	}

	uItemStartX	= uBannerStartX + uBannerBorderWidth;
	uItemStartY 	= uBannerStartY + uBannerBorderWidth + 5;

	uItemWidth	= uBannerWidth - 2 * uBannerBorderWidth;
	uItemHeight	= 30;

	uTextGap		= 4;

	/* 判断是否要重绘所有屏幕 */
	if( ( sPrevKey == -1 && sCurrKey == 0 ) ||
              ( sPrevKey == -1 && sCurrKey == 1 ) ||
	    ( sCurrKey / MENU_MAX_ITEMS != sPrevKey / MENU_MAX_ITEMS )
	  )
	{
		bDrawAllMenu = true;
	}
	else
		bDrawAllMenu = false;

	if( bDrawAllMenu == false )
	{
		/* 只画移动 */

		/* 先画失去焦点项 */
		/* 画item */
		sPy = uItemStartY + uItemHeight * ( sPrevKey - ( sPrevKey / MENU_MAX_ITEMS ) * MENU_MAX_ITEMS );
		CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   uItemStartX, sPy, 
							   uItemWidth, uItemHeight, uBannerColor, uBannerColor, 0 );
		CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,  uItemStartX + 20, sPy + uTextGap, uTextColor,ItemName[sPrevKey]);
		/* 画得到焦点项 */
		sPy = uItemStartY + uItemHeight * ( sCurrKey - ( sCurrKey / MENU_MAX_ITEMS ) * MENU_MAX_ITEMS );
		CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   uItemStartX, sPy, 
							   uItemWidth, uItemHeight, uTextColor, uTextColor, 0 );
		CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,  uItemStartX + 20, sPy + uTextGap, uBannerColor, ItemName[sCurrKey] );
             			CH_UpdateAllOSDView();  
	
	}
	else
	{
		/* 重绘整屏幕 */

		/* 画字banner */
		CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
									uBannerStartX, uBannerStartY, 
									uBannerWidth, uBannerHeight + 20, 
									uBannerColor, uBannerBorderColor, 2 );

		/* 画Items */
		uScrStart	= ( sCurrKey / MENU_MAX_ITEMS ) * MENU_MAX_ITEMS;
		uScrEnd	= uScrStart + MENU_MAX_ITEMS ;
		uScrEnd	= ( uMaxCount > uScrEnd ) ? uScrEnd : uMaxCount;
		for( uIndex = uScrStart; uIndex <  uScrEnd; uIndex ++ )
		{
			if( uIndex == sCurrKey )
			{
				/* 画得到焦点项 */
				sPy = uItemStartY + uItemHeight * ( uIndex - ( uIndex / MENU_MAX_ITEMS ) * MENU_MAX_ITEMS );
				CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
									   uItemStartX, sPy, 
									   uItemWidth, uItemHeight, uTextColor, uTextColor, 0 );
				CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,  uItemStartX + 20, sPy + uTextGap, uBannerColor, ItemName[uIndex]);
			}
			else
			{
				/* 画item */
				sPy = uItemStartY + uItemHeight * ( uIndex - ( uIndex / MENU_MAX_ITEMS ) * MENU_MAX_ITEMS );

				CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
									   uItemStartX, sPy, 
									   uItemWidth, uItemHeight, uBannerColor, uBannerColor, 0 );
				CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,uItemStartX + 20, sPy + uTextGap, uTextColor,ItemName[uIndex] );
			}
		}
			CH_UpdateAllOSDView();  

	}

}

/************************函数说明***********************************/
/*函数名:void CH_DisplaySTBInfo( void )                            */
/*开发人和开发时间:zengxianggen 2007-01-15                         */
/*函数功能描述:显示机顶盒状态信息                                  */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */	
/*返回值说明：无                                                   */
void CH_DisplaySTBInfo( void ) 
{
	char STBInfoTxt[400];
	char STBTempInfo[100];
#ifdef USE_MAC
	U8 STB_SN[14];/*8-->14*/
	U8 STB_ID[14];/*8-->14*/
	U8 Mac[6];  /*fj 20080228 add*/
#else
	U8 STB_SN[8];
	U8 STB_ID[8];
#endif
	boolean GetSTB=true;
	U8 ManuID;
	U8 ProductModel;
	int ProYear;
	U8 ProMonth;
	U8 ProDay;
	U32 DevPubID;
	long int STB_SNnum;
	long int CA_Serial_Num;
	U8 CA_Serial[4];
	int i;
    int 	iKeyRead;
	int x=50;
	int y=30;
	int width=600;
	int height=230;
	U8 NuIDKey[16];
	U32 NuICRC;
	U8 pCRC[4];
	U32 CRC;
	U8  pCheckNum[4];
	U8 CRCData[16];
    
	memset(STBInfoTxt,0,400);
    /*得到机号信息*/
	CH_ReadSTBAndMACID((U8 *)STB_ID);
#ifdef USE_MAC
	for (i=0;i<14;i++)
	{
		STB_SN[i] = STB_ID[13-i];
	}
	Mac[0]=STB_SN[8];/*fj 20080228 add*/
	Mac[1]=STB_SN[9];
	Mac[2]=STB_SN[10];
	Mac[3]=STB_SN[11];
	Mac[4]=STB_SN[12];
	Mac[5]=STB_SN[13];/*fj 20080228 end*/
#else
	for (i=0;i<8;i++)
	{
		STB_SN[i] = STB_ID[7-i];
	}
#endif
	STB_SNnum=STB_SN[5]|(STB_SN[4]<<8)|(STB_SN[3]&0xF)<<16;
	ManuID=STB_SN[7]=0x01;
	ProductModel=STB_SN[6];
	ProYear=((STB_SN[1]&0xf)<<3)|((STB_SN[2]&0xe0)>>5);
	ProYear=ProYear;
	ProMonth=((STB_SN[2]&0x1e)>>1)&0xF;
	ProDay=((STB_SN[2]&0x1)<<4)|((STB_SN[3]>>4)&0xf);
	if(STB_SNnum==0xFFFFF)
	{	
		strcat(STBInfoTxt,"invalid stbid");
	}
	else
	{
		sprintf(STBTempInfo,"stbid:%02d%02d%02d%02d%02d%07d",ManuID,ProductModel,ProYear,
			ProMonth,ProDay,STB_SNnum);
		strcat(STBInfoTxt,STBTempInfo);

	}
	strcat(STBInfoTxt,"\n");
#ifdef USE_MAC 
	/*MAC地址*/                             /*fj 20080228 add*/
	sprintf(STBTempInfo,"MAC Adress %02x%02x%02x%02x%02x%02x",Mac[0],Mac[1],Mac[2],Mac[3],Mac[4],Mac[5]);
		strcat(STBInfoTxt,STBTempInfo);
	    strcat(STBInfoTxt,"\n");
		                                   /*fj 20080228 end*/
#endif									   
	/*得到CA_serrila Number*/
	CH_GetPairCASerial(CA_Serial);
	sprintf(STBTempInfo,"CA_serial Number:0x%02x%02x%02x%02x",CA_Serial[0],CA_Serial[1],CA_Serial[2],CA_Serial[3]);
	strcat(STBInfoTxt,STBTempInfo);
	strcat(STBInfoTxt,"\n");

	/*得到NUID*/
	
	/*CH_GetNUID(NuIDKey,&NuICRC);*/

	{
	
		  


				/*得到NUID CHECK NUM,NUID*/
				CH_GetNUID((U8 *)NuIDKey,(U32 *)&NuICRC);/*显示不需要处理NUID读取错误情况*/

				pCheckNum[0]=(NuICRC>>24)&0xFF;
				pCheckNum[1]=(NuICRC>>16)&0xFF;
				pCheckNum[2]=(NuICRC>>8)&0xFF;
				pCheckNum[3]=NuICRC&0xFF;




				/*计算CRC*/
				memset(CRCData,0,16);

				memcpy(CRCData,CA_Serial,4);
				memcpy(&CRCData[4],&NuIDKey[12],4);
				memcpy(&CRCData[8],pCheckNum,4);
				
				CRC=crc_ISO_3309(CRCData,16);

				pCRC[0]=(CRC>>24)&0xFF;
				pCRC[1]=(CRC>>16)&0xFF;
				pCRC[2]=(CRC>>8)&0xFF;
				pCRC[3]=CRC&0xFF;


			}

	strcat(STBInfoTxt,"NUID:");
#if 1
	DevPubID=(U32)(NuIDKey[12] << 24) | \
				(NuIDKey[13] << 16) | \
				(NuIDKey[14] << 8)  |  \
				(NuIDKey[15]);
#else
   /* 
    DevPubID=CH_GetNUID();
    NuIDKey[12]=DevPubID&0xFF;
    NuIDKey[13]=(DevPubID>>8)&0xFF;

	NuIDKey[14]=(DevPubID>>16)&0xFF;
    NuIDKey[15]=(DevPubID>>24)&0xFF;
	
	*/

#endif
	sprintf(STBTempInfo,"0x%02x%02x%02x%02x CheckNum:%02x%02x%02x%02x CRC:%02x%02x%02x%02x",
		NuIDKey[12] ,NuIDKey[13] ,NuIDKey[14],NuIDKey[15],
		pCheckNum[0] ,pCheckNum[1] ,pCheckNum[2],pCheckNum[3],
		pCRC[0] ,pCRC[1] ,pCRC[2],pCRC[3]);
	
	strcat(STBInfoTxt,STBTempInfo);
	strcat(STBInfoTxt,"\n");
	/*得到 BOOT ROM状态*/
	strcat(STBInfoTxt,"BOOT ROM status:");
	if(CH_BootAuthenticaionStatus()==1)
	{
		strcat(STBInfoTxt,"Enable");
	}
	else
	{
		strcat(STBInfoTxt,"Disable");
	}
		strcat(STBInfoTxt,"\n");
	/*得到 JTAG LOCK状态*/
	strcat(STBInfoTxt,"JTAG LOCK status:");
	if(CH_JTAGLOCKStatus()==1)
	{
		strcat(STBInfoTxt,"Enable");
	}
	else
	{
		strcat(STBInfoTxt,"Disable");
	}
		strcat(STBInfoTxt,"\n");
	/*得到 OTP状态*/
	strcat(STBInfoTxt,"OTP status:");
	if(CH_OTPStatus()==1)
	{
		strcat(STBInfoTxt,"Enable");
	}
	else
	{
		strcat(STBInfoTxt,"Disable");
	}
		strcat(STBInfoTxt,"\n");
	/*得到卡操作模式*/
	strcat(STBInfoTxt,"CARD operation status:");
#ifdef ADD_SMART_CONTROL
if(CH_GetSmartControlStatus()==1)
	{
		strcat(STBInfoTxt,"Need card");
	}
	else
	{
		strcat(STBInfoTxt,"No need card");
	}
#else
         strcat(STBInfoTxt,"Need card");
#endif
		strcat(STBInfoTxt,"\n");
    /*得到软件发布日期*/
    sprintf(STBTempInfo,"software release time: %s %s",__DATE__,__TIME__);
	strcat(STBInfoTxt,STBTempInfo);
	strcat(STBInfoTxt,"\n");
	/*显示最终的结果*/
#ifdef OSD_COLOR_TYPE_RGB16
	
	CH6m_DrawRectangle(
		GFXHandle,
		CH6VPOSD.ViewPortHandle,&gGC,
		x,
		y,
		width,
		height,
		0XE35C,
		0XE35C,
		1);
		CH6_MulLine_ChangeLine(GFXHandle,CH6VPOSD.ViewPortHandle,
		x, y,
		width, height,
		(char* )STBInfoTxt,0x910e,2,2,false);	
#else
CH6m_DrawRectangle(
		GFXHandle,
		CH6VPOSD.ViewPortHandle,&gGC,
		x,
		y,
		width,
		height,
		0xffc0d0e0,
		0xffc0d0e0,
		1);
		CH6_MulLine_ChangeLine(GFXHandle,CH6VPOSD.ViewPortHandle,
		x, y,
		width, height,
		(char* )STBInfoTxt,0xff204070,2,2,false);	
#endif

		CH_UpdateAllOSDView();  
#if 0
	while( 1 )
	{
	
		/* 等待键值 */
		iKeyRead	= GetKeyFromKeyQueue( -1 );
		if(iKeyRead==EXIT_KEY)
			break;
	}
	CH6_ClearFullArea(GFXHandle,CH6VPOSD.ViewPortHandle);
	return;
#endif
}

#ifdef STBID_FROM_E2PROM
/*fj 20080311 add for MANUAL_SET_STBID*/
void CH_ManualSetSTBID(void)
{
  int i;
  boolean Redraw=true;
  int CurSlect=0;
  int iKeyRead;
  int keyRead;
  int Curpos=0;
  char STBIDInfo[17];
  		char InputSeriNo[8];
		char InputYear[3];
		char InputMonth[3];
		char InputDate[3];
		char InputManuID[3];
		char InputModel[3];
		char *End;
		long int SeriNo=0;
		long int Year;
		long int Month;
		long int Date;
		long int ManuID;
		long int Model;
		BYTE STBID_output[8];
	U8 ManuID1;
	U8 ProductModel;
	int ProYear;
	U8 ProMonth;
	U8 ProDay;
	U8 STB_SN[8];
	long int STB_SNnum;

	int ErrorCode;
	   U32 uBannerBorderColor	= 0xfff8f8f8;
   U32 uBannerColor		= 0xff506090;
	U32 uTextColor			= 0xff80b0e0;
  char SelectList[2][20]={
  	                 "Enable",
					 "Disable"
  	               };
     CH_ReadSTBIDFromE2P(STBID_output);
	for (i=0;i<8;i++)
	{
		STB_SN[i] = STBID_output[i];
	}
	STB_SNnum=STB_SN[5]|(STB_SN[4]<<8)|(STB_SN[3]&0xF)<<16;
	ManuID1=STB_SN[7]=0x01;
	ProductModel=STB_SN[6];
	ProYear=((STB_SN[1]&0xf)<<3)|((STB_SN[2]&0xe0)>>5);
	ProYear=ProYear;
	ProMonth=((STB_SN[2]&0x1e)>>1)&0xF;
	ProDay=((STB_SN[2]&0x1)<<4)|((STB_SN[3]>>4)&0xf);	 
  sprintf(STBIDInfo, "%02d%02d%02d%02d%02d%07d",ManuID1,ProductModel,ProYear,
			ProMonth,ProDay,STB_SNnum);
  while(1)
  	{
  	if(Redraw==true)
  		{
      for(i=0;i<2;i++)
  	   {
  	     if(i == CurSlect)
  	  	  {
  	        CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   450,470+i*35, 
							   100, 35, uTextColor, uTextColor, 0 );
		    CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC, 450, 470+i*35, uBannerColor,SelectList[i]);  	  	 
  	  	  }
	    else
	  	  {
  	        CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   450,470+i*35, 
							   100, 35, uBannerColor, uBannerColor, 0 );
		    CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 450, 470+i*35, uTextColor,SelectList[i]);
	       }
  	   }
	   CH_UpdateAllOSDView();
	  }

	  iKeyRead=GetKeyFromKeyQueue(-1);
	  switch(iKeyRead)
	  	{
	  	   case OK_KEY:
		   	    if(CurSlect == 0)
		   	    	{
		   	    	  ErrorCode=CH6_InputPinCode_stbid(false);
					  if(ErrorCode != 0)
					  	{
					  	  return;
					  	}
		   	    	  CH_EanbleSTBIDFromE2P();
			          CH6_ClearArea( GFXHandle, CH6VPOSD.ViewPortHandle,450,470,220,100 );
					  CH_UpdateAllOSDView();					  
					  while(1)
					  {
                      if(Redraw == true)
                      	{
                      	
  	                  CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   450,470, 
							   220, 35, uTextColor, uTextColor, 0 );
		              CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,  450, 470, uBannerColor,STBIDInfo);
					  if(Curpos<17)
					  	{
					       CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   450+Curpos*12,470, 
							   12, 35, 0xfff08020, 0xfff08020, 0 );
					    }
			          CH_UpdateAllOSDView();
					    }
					  keyRead=GetKeyFromKeyQueue(1);
					  switch(keyRead)
					  	{
					  	  case EXIT_KEY:
						  	
			                 CH6_ClearArea( GFXHandle, CH6VPOSD.ViewPortHandle,450,470,220,100 );			 
			                 CH_UpdateAllOSDView();	
							 Redraw=true;
                             return;
						  case LEFT_ARROW_KEY:
						  case RIGHT_ARROW_KEY:
						  	 Curpos +=(keyRead == RIGHT_ARROW_KEY ? 1 : -1 ) ;
							 if(Curpos>16)
							 	{
							 	  Curpos=0;
							 	}
							 else if(Curpos<0)
							 	{
							 	  Curpos=16;
							 	}
				             Redraw=true;
							 break;
						  case OK_KEY:
						  	 
						  	  	{
	       for(i=0;i<7;i++)
			{
				InputSeriNo[i]=STBIDInfo[i+10];
			}
			InputSeriNo[7]='\0';
			SeriNo=strtol(InputSeriNo,&End,10);

			for(i=0;i<2;i++)
			{
				InputManuID[i]=STBIDInfo[i];
			}
			InputManuID[2]=0;
			ManuID=strtol(InputManuID,&End,10);

			for(i=0;i<2;i++)
			{
				InputModel[i]=STBIDInfo[i+2];
			}
			InputModel[2]=0;
			Model=strtol(InputModel,&End,10);

			for(i=0;i<2;i++)
			{
				InputYear[i]=STBIDInfo[i+4];
			}
			InputYear[2]=0;
			Year=strtol(InputYear,&End,10);

			for(i=0;i<2;i++)
			{
				InputMonth[i]=STBIDInfo[i+6];
			}
			InputMonth[2]=0;
			Month=strtol(InputMonth,&End,10);

			for(i=0;i<2;i++)
			{
				InputDate[i]=STBIDInfo[i+8];
			}
			InputDate[2]=0;
			Date=strtol(InputDate,&End,10);
			if(ManuID==1)//changhong
			{
			
					//第三版本机号&&机型&&
					STBID_output[0]=0;
					STBID_output[1]=0x10|((Year&0x7f)>>3);
					STBID_output[2]=((Year&0x07)<<5)|((Month&0x0f)<<1)|((Date&0x1f)>>4);
					STBID_output[3]=((Date&0x0f)<<4)|((SeriNo&0xf0000)>>16);
					STBID_output[4]=(SeriNo&0xff00)>>8;
					STBID_output[5]=SeriNo&0xff;
					STBID_output[6]=Model&0xff;
					STBID_output[7]=0x12;

			}			
							       CH_WriteSTBIDToE2P(STBID_output);	
			                       CH6_ClearArea( GFXHandle, CH6VPOSD.ViewPortHandle,450,470,220,100 );			 
			                       CH_UpdateAllOSDView();	
								   
						  	  	}


						  	 return;
						  default:
						  	if((keyRead>=KEY_DIGIT0)&&(keyRead<=KEY_DIGIT9)&&Curpos<17)
						  		{
						  		  
						  		  	  STBIDInfo[Curpos]=Convert2Char(keyRead);
									 // STBID[Curpos]=atoi(&STBIDInfo[i]);
									  Curpos++;
						  		  	
						  		}
							if(Curpos == 17 &&(keyRead>=KEY_DIGIT0)&&(keyRead<=KEY_DIGIT9))
								{
								   Curpos=0;
								}
							Redraw=true;
							break;
					  	}
					  }

		   	    	}
		   	    else if(CurSlect == 1)
		   	    	{
		   	    	  CH_DisableSTBIDFromE2P();
			          CH6_ClearArea( GFXHandle, CH6VPOSD.ViewPortHandle,450,470,100,100 );			 
			          CH_UpdateAllOSDView();	
					  return;
		   	    	}
		
		   	    break;
		   case UP_ARROW_KEY:
		   case DOWN_ARROW_KEY:  
		   	    CurSlect += (iKeyRead == DOWN_ARROW_KEY ? 1 : -1 ) ;
				if(CurSlect>1)
					{
					  CurSlect=0;
					}
				else if(CurSlect<0)
					{
					  CurSlect=1;
					}
				Redraw=true;
		   	    break;
		   case EXIT_KEY:
			    CH6_ClearArea( GFXHandle, CH6VPOSD.ViewPortHandle,450,470,100,100 );			 
			    CH_UpdateAllOSDView();		   	  
		   	    return;
					
	  	}
  }
}
#endif
/***************************函数体定义*****************************/ 
/******************************************************************/
/*函数名:void CH_BootDisableBackgroundCheck(void)                          */
/*开发人和开发时间:zengxianggen 2008-06-12                        */
/*函数功能描述:如果打开Bootrom 功能需要关闭background check
功能，否则会5分钟左右自动重新启动*/
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：       */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */ 
void CH_BootDisableBackgroundCheck(void)
{	
        int i_error;
	 CH_Antifuse_Init();
	 do_report(0,"Start Check0\n ");

	 if(CH_BootAuthenticaionStatus() == true)
	 {


	       int i_region_id;
		do_report(0,"Start Check1\n ");
		for(i_region_id =0 ;i_region_id < 8 ; i_region_id++)
		{
		    i_error = cryptRemoveRegionNumber(i_region_id);	      /***********************************/	
		    do_report(0,"i_region_id =%d,i_error = %d\n",i_region_id,i_error);		
		}
		do_report(0,"Start Check2\n ");

		
	 }
	 CH_Antifuse_Term();
}


void CH_EnPopupDialog(char* pc_text)
{
		CH6m_DrawRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
							   450, 300, 
							   200, 60, 0xFF000000, 0xFF000000, 0 );
		CH6_EnTextOut( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,  450 + 20, 300 + 20, 0xFFFFFFFF, pc_text );
             	CH_UpdateAllOSDView();  

}
