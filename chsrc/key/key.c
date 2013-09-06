/*
	(c) Copyright changhong
  Name:key.c
  Description:keyboard control section,include all info about remote control and front panel control  
				for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-05-17    Created

 =====================
 MODIFICATION HISTORY:
 =====================

Modification:
1.Date:2005-4-,by Yxl 
  Content:
  1)modify task DVBKeyboardProcess(),使其能接受RC6和NEC遥控码，
  由宏USE_RC6_REMOTE和USE_NEC_REMOTE控制(详见config.txt)。

  2006-12-28增加NEC和RC6解码兼容处理由NEC_RC6_KEY控制  zengxianggen
  2007-04-24移植到STi7100平台                          zengxianggen
*/

#include <string.h>

#include "..\main\initterm.h"
#include "key.h"
#include "stddefs.h"
#include "stcommon.h"
#include "appltype.h"
#include "keymap.h"
#include "setjmp.h"
#include "..\dbase\vdbase.h"

#ifdef USE_RC6_REMOTE /*yxl 2005-04-01 add this line*/
extern int RC6_Decode(U32* piSymbolCount,U32* pKeyDataNum,U32* pRC6Mode,void* pSymbolValue,char* KeyData);
#endif /*yxl 2005-04-01 add this line*/

extern int JL_ParseKey ( U32* piSymbolCount, void* pSymbolValue, U32* pKeyCodeNum, U8* pKeyData );

#ifdef USE_NEC_REMOTE /*yxl 2005-04-01 add below section*/
extern int NEC_Decode(U32* piSymbolCount,void* pSymbolValue,U32* pKeyCodeNum,U8* pKeyData);
#endif /*end yxl 2005-04-01 add below section*/

extern BOOL FrontKeybordInit(void);
#define KEY_DEBUG /**//*yxl 2004-08-19 add this macro*/


#define MAX_KEYQUEUE_ITEMS 10      /*键值队列缓冲个数*/
static int KeyQueue[MAX_KEYQUEUE_ITEMS];  
static S8 KeyQueueTail, KeyQueueHead;

static task_t *ptidKeyboardTask;

static message_queue_t *pstUsifMsgQueue;


#define WAIT_FOR_1_SEC          ST_GetClocksPerSecondLow()

boolean  CH_HandleContinusKey(unsigned short Key);

/*是否为空*/
S8 IsKeyQueueEmpty(void)
{
	return  ( KeyQueueHead == KeyQueueTail ) ;
}

/*20050306 add 清空消息队列中键值*/
void CH_FreeAllKey(void)
{
	
	usif_cmd_t *msg_p;
	while (TRUE)
	{
		
		msg_p = message_receive_timeout(pstUsifMsgQueue, TIMEOUT_IMMEDIATE);
		if(msg_p!=NULL)
		{
			switch (msg_p->from_which_module)
			{
			case KEYBOARD_MODULE:				
				break;
			case STBSTATUS_MODULE:/*STB Current Status*/	
				 break;
			default:
				break;
			}
			/*
			* free     the sent message
			*/
			message_release(pstUsifMsgQueue, msg_p);
		}
		else
		{
             break;
		}
	}
}

/*初始化键值队列*/
int InitKeyQueue(void)
{
	memset(KeyQueue, 0 , sizeof(KeyQueue));
	KeyQueueTail =  KeyQueueHead = 0 ;
	return true;
}
/*把键值压入队列中*/
int PutKeyIntoKeyQueue(int iKeyScanCode)
{
	if (  ( ( KeyQueueTail + 1 ) % MAX_KEYQUEUE_ITEMS ) == KeyQueueHead  )
	{
		return FALSE ;
	}
	KeyQueue[KeyQueueTail] = iKeyScanCode;
	
	KeyQueueTail = ( KeyQueueTail+1 ) % MAX_KEYQUEUE_ITEMS ;
	
	return TRUE;	
}


/*yxl 2005-01-11 add this section
前面板按键和遥控按键的对应函数
*/
int CH6_FrontKeyMapRemoteKey(int FrontKey)
{
	int MappedRemoteKey=NO_KEY_PRESS;
	switch(FrontKey)
	{
		#if 0			
		case MENU_CUM_EXIT_KEY: 
			if(CH_GetCurApplMode()==APP_MODE_PLAY)
				MappedRemoteKey = MENU_KEY;
			else
				MappedRemoteKey = EXIT_KEY;
			break;
		#else
		//case ALT_CUM_KEY:
			//MappedRemoteKey = EXIT_KEY;
			//break;
		case MENU_CUM_EXIT_KEY:
			MappedRemoteKey = MENU_FRONTKEY;
			break;	
		#endif
		case OK_CUM_SELECT_KEY:
			MappedRemoteKey=OK_FRONTKEY;
			break;
		case ALT_CUM_KEY:
			MappedRemoteKey=AUDIO_KEY/*TV_RADIO_KEY 20051013 change*/;
			break;
		case VOL_DOWN_CUM_LEFT_ARROW_KEY:/* 左映射成为上*/
#if 1
			MappedRemoteKey = LEFT_ARROW_FRONTKEY;
#else
			if(CH_GetCurApplMode()==APP_MODE_PLAY)
				MappedRemoteKey =P_DOWN_KEY;
			else 
			    MappedRemoteKey =UP_ARROW_KEY;
#endif
			break;
		case VOL_UP_CUM_RIGHT_ARROW_KEY:/*右映射成为下*/
#if 1
			MappedRemoteKey = RIGHT_ARROW_FRONTKEY;	
#else
		if(CH_GetCurApplMode()==APP_MODE_PLAY)			
				MappedRemoteKey =P_UP_KEY ;
			else
			    MappedRemoteKey =DOWN_ARROW_KEY;
#endif
			break;
						
		case PROG_PREV_CUM_UP_ARROW_KEY:/* 上映射为左*/
#if 0
			if(CH_GetCurApplMode()==APP_MODE_PLAY)
				MappedRemoteKey =P_DOWN_KEY;
			else 
			    MappedRemoteKey =UP_ARROW_KEY;
#else
			MappedRemoteKey = UP_ARROW_FRONTKEY;
#endif
			break;
						
		case PROG_NEXT_CUM_DOWN_ARROW_KEY:		/*下映射为右*/		
#if 0
			if(CH_GetCurApplMode()==APP_MODE_PLAY)			
				MappedRemoteKey =P_UP_KEY ;
			else
			    MappedRemoteKey =DOWN_ARROW_KEY;
#else
				MappedRemoteKey = DOWN_ARROW_FRONTKEY;	
#endif
			break;
						
		case POWER_CUM_SELECT_KEY:
			MappedRemoteKey = POWER_KEY ;
			break;				
		/*20050222 add*/
		case FONRT_MENU_OK_KEY:
			 MappedRemoteKey=MENU_FRONTKEY;
			 break;
		/*************/			
		
		}
	return MappedRemoteKey;
}
/*end yxl 2005-01-11 add this function*/


int ReadKey(cursor_state_t cursorStatus, clock_t tclkWaitTime)
{
	clock_t timeout;
	usif_cmd_t 	*pMsg;
	usif_cmd_t 	TempMsg;
	usif_cmd_t 	*pTempMsg;
	BOOLEAN 	ErrCode;
	int 			iKeyPassed;
	BOOLEAN 	bExitFlag;
	
	bExitFlag = FALSE;
	pMsg	= &TempMsg;

	while( TRUE )
	{
		/*
		* indefinitely wait for the keyboard input via the usif mailbox
		*/

		if (cursorStatus == CURSOR_OFF)
		{
			if ( tclkWaitTime )
			{
				ErrCode 	= TRUE;
				timeout = time_plus(time_now(), tclkWaitTime);
				pTempMsg = (usif_cmd_t *)message_receive_timeout(pstUsifMsgQueue,&timeout );
				if ( NULL != pTempMsg )
				{
				
					ErrCode = FALSE;
				}
			}
			else
			{
				if ( ( pTempMsg = (usif_cmd_t *)message_receive_timeout( pstUsifMsgQueue,TIMEOUT_INFINITY ) ) != NULL )
				{
					ErrCode = FALSE;
				}
			}
		}


		if( ErrCode == FALSE )
		{
			memcpy ( pMsg, pTempMsg, sizeof( usif_cmd_t ) );

			message_release ( pstUsifMsgQueue, pTempMsg );

			switch( pMsg->from_which_module )
			{
				case KEYBOARD_MODULE:
					/*iKeyPassed = pMsg->contents.keyboard.scancode & ~REMOTE_KEY_REPEATED;*/

					bExitFlag 		= TRUE;
					iKeyPassed 	= pMsg->contents.keyboard.scancode ;
					break;

				#if 0	/* jqz070127 */
				/*event handle*/
				case TIMER_MODULE:
					EventInfo = pMsg->contents.Timer.NotifyInfo;
					/* 得到虚拟键值 */
					iKeyPassed = UsifEventHandle( EventInfo );
				/*****************/
					break;

				case STBSTATUS_MODULE:/*STB Current Status*/
					CH_UpdateSTBStaus( pMsg->contents.STBInfo.CurSTBStaus );
					break;
				#endif
					
				default:
					break;
			}

			/*
			* free the sent message
			*/
			/*message_release(pstUsifMsgQueue, pMsg);*/

			/****Add for front key define **********/
			if( bExitFlag )
			{
				if( pMsg->contents.keyboard.device == FRONT_PANEL_KEYBOARD )
				{
					iKeyPassed = CH6_FrontKeyMapRemoteKey( iKeyPassed );
				}

				return iKeyPassed;
			}
		}
		else
		{
			/* failed to receive the key */
			if ( tclkWaitTime && cursorStatus == CURSOR_OFF )
			{
				/* no key pressed	*/
				return NO_KEY_PRESS;
			}
		}
	}
}
/*把键值从队列中取出*/

int GetKeyFromKeyQueue(S8 WaitingSeconds)
{
	int iKey ;

	if ( KeyQueueHead == KeyQueueTail ) 
	{
		if ( WaitingSeconds == -1 )
		{
			while ( ( iKey = ReadKey(CURSOR_OFF,WAIT_FOR_1_SEC  ) )  == NO_KEY_PRESS);
			return iKey ;
		}
		else
		{
			iKey =ReadKey(CURSOR_OFF,WAIT_FOR_1_SEC  *  WaitingSeconds ) ;
			if(iKey==NO_KEY_PRESS) iKey=-1;/*yxl 2004-11-29 add this line for 兼容股票应用*/	
			return iKey ;		
		}
	}
	iKey = KeyQueue[KeyQueueHead] ;
	
	KeyQueue[KeyQueueHead] = 0 ;

	KeyQueueHead = ( KeyQueueHead + 1 ) % MAX_KEYQUEUE_ITEMS ;

	return iKey;

}





int GetMilliKeyFromKeyQueue(int WaitingMilliSeconds)
{
	int iKey ;
    
	if ( KeyQueueHead == KeyQueueTail ) 
	{
		if ( WaitingMilliSeconds == -1 )
		{
			while ( ( iKey = ReadKey(CURSOR_OFF,WAIT_FOR_1_SEC*WaitingMilliSeconds/1000  ) )  == NO_KEY_PRESS );
			return iKey ;
		}
		else
		{

			iKey =ReadKey(CURSOR_OFF,WAIT_FOR_1_SEC  *  WaitingMilliSeconds/1000  ) ;
			if(iKey==NO_KEY_PRESS) iKey=-1;/*yxl 2004-11-29 add this line for 兼容股票应用*/				
			return iKey ;
		}
	}
	iKey = KeyQueue[KeyQueueHead] ;
	
	KeyQueue[KeyQueueHead] = 0 ;

	KeyQueueHead = ( KeyQueueHead + 1 ) % MAX_KEYQUEUE_ITEMS ;

	return iKey;
}

/************************函数说明*******************/
/*函数名:int CH_PUB_ConvertKeyToRC6(int ri_Key)               */
/*开发人和开发时间:zengxianggen 2006-10-30          */
/*函数功能描述:转换NEC键值到RC6键值对应表*/
/*函数原理说明:                                                           */
/*使用的全局变量、表和结构：                      */
/*调用的主要函数列表：                                        */
/*输入参数:int ri_Key,输入NEC码值?/
/*返回值说明：转换后的RC6键值                  */
int CH_PUB_ConvertKeyToRC6(int ri_Key)
{
int i;
int KeympaTbale[][2]=
{
    2805,KEY_A,
    35445,KEY_B,
    19125,KEY_C,
    51765,KEY_D,
    41565,KEY_E,
    39525,KEY_F,
    25245,RED_KEY,
    17085,GREEN_KEY,
    27285,YELLOW_KEY,
    23205,BLUE_KEY,
    49725,STOCK_KEY,
    53295,EPG_KEY,
    8925,NVOD_KEY,
    37485,TV_RADIO_KEY,
    34935,EXIT_KEY,
    4845,LIST_KEY,
    43095,MENU_KEY,
    59415,MOSAIC_PAGE_KEY,
    20655,OK_KEY,
    14535,LEFT_ARROW_KEY,
    6375,RIGHT_ARROW_KEY,
    39015,UP_ARROW_KEY,
    47175,DOWN_ARROW_KEY,
    33405,HELP_KEY,
    765,RETURN_KEY,
    63495,PAGE_DOWN_KEY,
    55335,PAGE_UP_KEY,
    30855,VOLUME_DOWN_KEY,
    22695,VOLUME_UP_KEY,
    18615,POWER_KEY,
    2295,MUTE_KEY,
    255,KEY_DIGIT0,
    32895,KEY_DIGIT1,
    16575,KEY_DIGIT2,
    49215,KEY_DIGIT3,
    8415,KEY_DIGIT4,
    41055,KEY_DIGIT5,
    24735,KEY_DIGIT6,
    57375,KEY_DIGIT7,
    4335,KEY_DIGIT8,
    36975,KEY_DIGIT9    
};
/*do_report(0,"ri_Key=%d\n",ri_Key);*/
for(i=0;i<42;i++)
{
   if(KeympaTbale[i][0]==ri_Key)
   	return KeympaTbale[i][1];
}
return -1;
}

#ifdef NEC_RC6_KEY
static void DVBKeyboardProcess(void *pvParam)
{
  
	unsigned int  			PacketRead		= 0;
	int 					DecodeResult	= -1;
    
	STBLAST_Symbol_t 	SymBufTemp[50];
	STBLAST_Symbol_t BackUpSymBufTemp[50];
	 unsigned int  BackupPacketRead=0;
	ST_ErrorCode_t 		ErrCode;
	int 					SemResult;
	int 					CountTemp		= 0;			/* 2004-07-12 add */
	int 					RepeatKeyCount	= 0;			/* yxl 2005-04-13 add this line */
	 
	BOOLEAN 			SendSign		= FALSE;		/* 2004-07-12 add,when true,send out the message of remote control */
	int LastKeyValue=-1;/*2004-07-12 add, the valid key pressed last time,initlia value is -1*/
	int CurKeyValue=-1;/*2004-07-12 add, the valid key pressed last time,initlia value is -1*/
	clock_t time;
	
	unsigned short KeyValueTemp;/*U8 KeyValueTemp; yxl 2005-04-04 modify this line*/
       usif_cmd_t *pMsg;

	clock_t timeout;
   
       boolean ContinusKey=false;/*20060823 add 处理连续键*/

	static	U32 Mode;
	static	U32 KeyCount;
	static	unsigned /*2007048 add unsigned*/char KeyData[8];
	
       U32 Rc6UserKey;
       U32  Rc6KeyCount;
       U32  Rc6KeySymbolCount;
	boolean RC6DecodeSucces=true;
   
      RepeatKeyCount=2;
      
	memset((void*)KeyData,0,sizeof(char)*8);

	while(true)
       {
             memset((void*)KeyData,0,sizeof(char)*8);
	       
		ErrCode=STBLAST_ReadRaw(BLASTRXHandle,SymBufTemp,50,&PacketRead,13500/*13500 20061231 change*/,0);

		if (ErrCode != ST_NO_ERROR) 
		{
                     do_report(0,"Error Key code\n");
			continue;			
		}
		while(true)
		{
                      time=time_plus(time_now(),ST_GetClocksPerSecond()/4);

		       SemResult=semaphore_wait_timeout(gpSemBLAST,/*TIMEOUT_INFINITY*/&time);

			
			if(SemResult==0) 
			{
				CountTemp++;
				/* do_report(0,"Get key CountTemp=%d\n",CountTemp);*/
				break;
			}
			else 
				{
			             ContinusKey=false;
				}

		}
         memcpy(BackUpSymBufTemp,SymBufTemp,PacketRead*sizeof(STBLAST_Symbol_t));
	  BackupPacketRead=PacketRead;
            /*首先按照RC6协议解码*/
	  DecodeResult=CH_BLAST_RC6ADecode(
				  &Rc6UserKey,
                               1,
                               SymBufTemp,
                               PacketRead,
                                &Rc6KeyCount,
                               &Rc6KeySymbolCount);

        if(DecodeResult==0&&Rc6KeyCount==1)
       {
         RC6DecodeSucces=true;	
       /*  do_report(0,"Rc6KeyCount=%d,Rc6UserKey=%x\n", Rc6KeyCount,Rc6UserKey&0xFF);*//**/        
	}
	else/*如果RC6协议不对，再按照NEC方式解码*/
		{
		  RC6DecodeSucces=false;
                DecodeResult=NEC_Decode(&BackupPacketRead,(void*)BackUpSymBufTemp,&KeyCount,(U8*)KeyData);
	        if((DecodeResult==0) && ((KeyData[0]*256 + KeyData[1]) != NEC_SYSTEM_CODE))
	        {
	        	U32 SystemKey=KeyData[0]*256 + KeyData[1];
		       DecodeResult = 1; /**/
			 do_report(0,"NEC system Key=%d\n",SystemKey);
	         }
	        else
	        	{
	        	   if(BackupPacketRead!=3)/*表示非连续按键*/
	        	   	{
                              /*  ContinusKey=false*/;
	        	   	}
	        	}
		}
		if(DecodeResult==0) /*解码成功*/
			{
			
                         if(RC6DecodeSucces==true)/*RC6*/
                         	{
                                   CurKeyValue=Rc6UserKey&0xFF;
					KeyValueTemp=CurKeyValue;  
					RepeatKeyCount=2;
                         	}
				else/*NEC*/
				{
				  KeyValueTemp=KeyData[3];
				  CurKeyValue=KeyData[2]*256+KeyValueTemp;
				/*  do_report(0,"NEC Key=%d\n",CurKeyValue);*/
				  CurKeyValue=CH_PUB_ConvertKeyToRC6(CurKeyValue);
				  KeyValueTemp=CurKeyValue;
		                RepeatKeyCount=1;
      
				}
			
				if(LastKeyValue!=CurKeyValue) 
				{

					CountTemp=0;	
					LastKeyValue=CurKeyValue;
					SendSign=TRUE;		
                                 ContinusKey=true; /*20060823 add 设置连续键标志*/
				}
				else
				{
				    

					if(CountTemp>RepeatKeyCount) 
					{
				
						if((ContinusKey==true&&(CH_HandleContinusKey(KeyValueTemp)==true))||ContinusKey==false)
						{
						   if(RC6DecodeSucces==true)/*RC6*/
					             CountTemp=0;			
						   else
						   	 CountTemp=0;		
						LastKeyValue=CurKeyValue;
						SendSign=TRUE;		
                                       	 ContinusKey=true; /*20060823 add 设置连续键标志*/
                                       	}
					}
				    }
				}
                   else
                   	{

		           do_report(0,"BackupPacketRead=%d\n",BackupPacketRead);
			
			}
                
	       PacketRead=0;		
		if(SendSign) 
		{	
			SendSign=FALSE;	
			/*send out remote key pressed message*/
			timeout=time_plus(time_now(), WAIT_FOR_1_SEC*10);
	
			if ((pMsg = (usif_cmd_t *)message_claim_timeout(pstUsifMsgQueue,&timeout)) != NULL)
			{
				
				pMsg->from_which_module = KEYBOARD_MODULE;
				pMsg->contents.keyboard.scancode = KeyValueTemp;
				pMsg->contents.keyboard.device = REMOTE_KEYBOARD;
				pMsg->contents.keyboard.repeat_status =FALSE;
				/**/
				message_send(pstUsifMsgQueue, pMsg);
			}
			else
			{
                            do_report(0,"error kecode\n");
			}
		}
	}
}


#else
int CH_GetKeyType(void)
{
 return 0;
}
static void DVBKeyboardProcess(void *pvParam)
{
  
    unsigned int  PacketRead=0;
    int DecodeResult=-1;
    
	STBLAST_Symbol_t SymBufTemp[50];
    ST_ErrorCode_t ErrCode;
	int SemResult;
	int CountTemp=0;
	int RepeatKeyCount=0;
	 
	BOOLEAN SendSign=FALSE;
	int 					LastKeyValue	= -1;		
	int 					CurKeyValue		= -1;		 
	clock_t 				time;
	
	unsigned short KeyValueTemp;
	usif_cmd_t 			*pMsg;

	clock_t 				timeout;

	static	U32 Mode;
	static	U32 KeyCount;
	static	char KeyData[8];

  
       boolean ContinusKey=false;/*20060823 add 处理连续键*/

	memset((void*)KeyData,0,sizeof(char)*8);/*yxl 2005-04-01 add this line*/

	if(CH_GetKeyType()==0)/*RC6 */
		RepeatKeyCount=2;
	else
		RepeatKeyCount=1;

	while ( true )
	{


		ErrCode=STBLAST_ReadRaw(BLASTRXHandle,SymBufTemp,50,&PacketRead,/*4000*//*6000*/13500,0);

		if ( ErrCode != ST_NO_ERROR )
		{
			STTBX_Print ( ( "STBLAST_RcReceive()=%s ", GetErrorText ( ErrCode ) ) );
			continue;
		}
		while ( true )
		{

			SemResult=semaphore_wait(gpSemBLAST);
			CountTemp++;
			if(SemResult==0) 
			{
			
				break;
		}
	
			}

			{

			 
			/* proceed RC6 Decode*/


      if(CH_GetKeyType()==0)/*RC6 */
	      DecodeResult=RC6_Decode(&PacketRead,&KeyCount,&Mode,(void*)SymBufTemp,KeyData);
      else
	  {
          DecodeResult=NEC_Decode(&PacketRead,(void*)SymBufTemp,&KeyCount,(U8*)KeyData);
		  if((DecodeResult==0) && ((KeyData[0]*256 + KeyData[1]) != NEC_SYSTEM_CODE))
		  {
			  DecodeResult = 1;
		  }	
		  /*20070117 add*/
		  if(PacketRead!=3)/*取消连续按键标志*/
		  {
			  ContinusKey=false;
			  LastKeyValue=-1;
		  }
		  /***************/
	  }
	      /*  do_report(0,"DecodeResult=%d\n",DecodeResult);*/
			if(DecodeResult==0) 
			{
				KeyValueTemp=KeyData[3]&0xFF/*20070428 add*/;
				CurKeyValue=KeyData[2]*256+KeyValueTemp;

			     if(CH_GetKeyType()==1)/*NEC */
				{
				   CurKeyValue=CH_PUB_ConvertKeyToRC6(CurKeyValue);
				   KeyValueTemp=CurKeyValue;
				}

				if(LastKeyValue!=CurKeyValue) 
				{
		
					CountTemp=0;	
					LastKeyValue=CurKeyValue;
					SendSign=TRUE;		
                                        ContinusKey=true;/*20060823 add 设置连续键标志*/

			}
			else
			{
					if(CountTemp>RepeatKeyCount) /*2,yxl 2005-04-13 modify 2 to RepeatKeyCount */
					{

						if((ContinusKey==true&&CH_HandleContinusKey(KeyValueTemp)==true)||ContinusKey==false)
                                       
                                       						{
						CountTemp=0;		
						LastKeyValue=CurKeyValue;
						SendSign=TRUE;		

                                       	  ContinusKey=true;/*20060823 add 设置连续键标志*/

			}

					}

					}
				}
                   else
		{			
                    /*20060823 add 取消连续键标志*/
			if(PacketRead==0)
			  ContinusKey=false;
			/**************/  
			
			}
                
			PacketRead=0;

		}

		if(SendSign) 
		{	
			SendSign=FALSE;	
			/*send out remote key pressed message*/
			timeout=time_plus(time_now(), WAIT_FOR_1_SEC*10);

			if ( ( pMsg = (usif_cmd_t *)message_claim_timeout ( pstUsifMsgQueue, &timeout ) ) != NULL )
			{
				
				pMsg->from_which_module 				= KEYBOARD_MODULE;
				pMsg->contents.keyboard.scancode 		= KeyValueTemp;
				pMsg->contents.keyboard.device 		= REMOTE_KEYBOARD;
				pMsg->contents.keyboard.repeat_status 	= FALSE;
				/**/
				message_send ( pstUsifMsgQueue, pMsg );

				//do_report(0,"key time = %d,key =%d\n",ipanel_porting_time_ms(),KeyValueTemp);
			
			}
			else
			{
				do_report ( 0, "error kecode\n" );
			}
		

		}
   	}
}


#endif

BOOL DVBKeyboardInit(void)
{
    BOOL  bError=ST_NO_ERROR;

#if 0	/*delete it on 050306*/
	InitKeyQueue();
#endif
/*200701111 move to here from initterm.c*/
    /*   bError = BLASTER_Setup();*/
    if (bError != ST_NO_ERROR) 
		return(bError);


    bError = FALSE;

 
    /* choose a COMMON pool for mailbox communications */
    if (symbol_enquire_value("usif_queue", (void **)&pstUsifMsgQueue))
    {
		STTBX_Print(("\n\nCan't find USIF message queue\n"));
		bError = TRUE;
		return TRUE;
    }

    if ((ptidKeyboardTask = Task_Create(DVBKeyboardProcess,	NULL,2048 ,7+3,
		"KeyboardTask",0))==NULL)
    {
		
		STTBX_Print(("Failed to start keyboard process\n"));
		return TRUE;
    }
	else 
	{
	/*	STTBX_Print(("creat keyboard task successfully \n"));*/
	}
#if 0	
#ifndef PT6964/*20060208 add*/
	/*20051220 add front key control*/
	FrontKeybordInit();
#endif
#endif
    return FALSE;

}


void FrontKeybordCheckTask( void *pParam )
{
	STPIO_OpenParams_t OpenParams;
	STPIO_Handle_t FrontKBPIOHandle,FrontKBScanHandle;	
	ST_ErrorCode_t ErrCode;		
    unsigned char  StatusScanH = 0;	/* the status of PIO input when scan line is High */
	unsigned char  StatusScanL = 0;	/* the status of PIO input when scan line is low */

	U8 KeyPressedLast=0xff;/* the valid key pressed last time,initlia value is 0xff*/

	U8 KeyPressed;/* the valid key pressed current*/
	int CountTemp=0;
	BOOLEAN SendSign=FALSE;/* when true,send out the message of front panel button pressed */
    usif_cmd_t* pMsg;
	U8 InputMaskTemp;
	clock_t timeout;


    /*20051220 change for 5105 key sense */
	OpenParams.ReservedBits = ( PIO_BIT_2 | PIO_BIT_3 | PIO_BIT_5 | PIO_BIT_6 );
	OpenParams.BitConfigure[2] = STPIO_BIT_INPUT;
	OpenParams.BitConfigure[3] = STPIO_BIT_INPUT;
	OpenParams.BitConfigure[5] = STPIO_BIT_INPUT;
	OpenParams.BitConfigure[6] = STPIO_BIT_INPUT;
	OpenParams.IntHandler =  NULL; 

	ErrCode = STPIO_Open(PIO_DeviceName[2], &OpenParams, &FrontKBPIOHandle );
	
	if( ErrCode != ST_NO_ERROR )
	{
		
		STTBX_Print(("\nSTPIO_Open()=%s ",GetErrorText(ErrCode)));
		return ;
	}

	InputMaskTemp=OpenParams.ReservedBits;
#ifdef	KEY_DEBUG
	STTBX_Print(("\n InputMaskTemp=%x",InputMaskTemp));
#endif
   /*20051220 change for 5105 key scan */
	OpenParams.ReservedBits    = ( PIO_BIT_7 );
	OpenParams.BitConfigure[7] = STPIO_BIT_OUTPUT;
	OpenParams.IntHandler      = NULL; 
	ErrCode = STPIO_Open(PIO_DeviceName[2], &OpenParams, &FrontKBScanHandle );

	if( ErrCode != ST_NO_ERROR )
	{
		STTBX_Print(("\nSTPIO_Open()=%s ",GetErrorText(ErrCode)));
		return ;
	}
/*	InputMaskTemp=0xb8;*/

	STTBX_Print(("ST_GetClocksPerSecond()=%d\n",ST_GetClocksPerSecond()));

	while(true)
	{

        task_delay(ST_GetClocksPerSecond()/15);

		CountTemp++;
		if(CountTemp>=200) CountTemp=200;


		/*ErrCode= STPIO_Write( FrontKBScanHandle, 0xff);*/
        ErrCode= STPIO_Set( FrontKBScanHandle, 0x80);/*设置扫描位为高*/

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
		
		/*ErrCode= STPIO_Write( FrontKBScanHandle, 0xfe);*/
		ErrCode= STPIO_Clear( FrontKBScanHandle, 0x80);/*设置扫描位为低*/
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

		/*do_report(0,"StatusScanL=%d,StatusScanH=%d\n",StatusScanL,StatusScanH);*/

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
			if(CountTemp>3) 
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
		
	
			timeout = ST_GetClocksPerSecondLow()*2 ;
		
			if ((pMsg = (usif_cmd_t *)message_claim_timeout(pstUsifMsgQueue,/*TIMEOUT_INFINITY*/&timeout)) != NULL)
			{
				pMsg->from_which_module = KEYBOARD_MODULE;
				pMsg->contents.keyboard.scancode = KeyPressed;
				pMsg->contents.keyboard.device = FRONT_PANEL_KEYBOARD;
				pMsg->contents.keyboard.repeat_status =FALSE;
				message_send(pstUsifMsgQueue, pMsg);
				do_report(0,"key=%d\n",KeyPressed);
				#ifdef	KEY_DEBUG
				STTBX_Print(("\n front panel key Message Sendout  keyvalue=%d\n",KeyPressed));
				#endif

			}
		
		}
	}
}

BOOL FrontKeybordInit(void)
{

	task_t* ptidFrontKBCheckTask;

	ptidFrontKBCheckTask = Task_Create(FrontKeybordCheckTask, NULL, 1024, 7+3/*12*/, "FrontKeybordChecktask", 0 );
	if(ptidFrontKBCheckTask==NULL)
	{
		STTBX_Print(("can't create FrontKeybordChecktask successfully"));
		return TRUE;
	}
	else STTBX_Print(("create FrontKeybordChecktask successfully"));
	return FALSE;
}
/*20060823 add对待机键和静音键的连续键不处理*/
/*函数原型:boolean  CH_HandleContinusKey(unsigned short Key)
功能:判断是否需要处理连续键
输入:int Key,处理的键值
输出:无
返回:TRUE,需要处理，FALSE,不需要处理
*/
boolean  CH_HandleContinusKey(unsigned short Key)
{
#if 0/*20060830 change*/
    if(Key==MUTE_KEY||Key==POWER_KEY)
    	{
            return false;
    	}
	else
	{
           return true;
	}
#else
if(Key==UP_ARROW_KEY||
   Key==DOWN_ARROW_KEY||
   Key==LEFT_ARROW_KEY||
   Key==RIGHT_ARROW_KEY||
   Key==VOLUME_PREV_KEY||
   Key==VOLUME_NEXT_KEY||
   Key==P_UP_KEY||
   Key==P_DOWN_KEY)
{
  return true;
}
else
{
   return false;
}
#endif
}

void CH_SetInputActive(boolean InputActive)
{
#if 0
   #include "..\src\stblast\blastint.h"
   extern BLAST_ControlBlock_t *BLAST_GetControlBlockFromHandle(STBLAST_Handle_t Handle);
    BLAST_ControlBlock_t *Blaster_p;
    Blaster_p =(BLAST_ControlBlock_t *) BLAST_GetControlBlockFromHandle(BLASTRXHandle);
    Blaster_p->InputActiveLow=InputActive;
#endif
}




