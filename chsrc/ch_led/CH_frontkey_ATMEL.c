/*
(c) Copyright changhong
  Name:CH_frontkey_ATMEL.c
  Description: 实现由ATMEL控制的前面板按键相关操作
  Authors:yixiaoli
  Date          Remark
  2007-05-21    Created 

Modifiction:
Date:2007-05-21 by Yixiaoli
1.将原KEY.C中由ATMEL控制的前面板按键相关操作的相关部分移植到该文件中,目的是优化软件系统结构



*/

#include "initterm.h"
#include "key.h"
#include "stddefs.h"
#include "stcommon.h"
#include "appltype.h"

#include "keymap.h"
#include "errors.h"
#include "..\symbol\symbol.h"
#include <string.h>
#include "setjmp.h"

#include "..\usif\applIF.h"
#include "..\dbase\vdbase.h"
#if 0
#define FRONT_KEY_DEBUG
#endif



#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL /*yxl 2006-05-03 add this line*/
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

	STTBX_Print(("ST_GetClocksPerSecond()=%d\n",ST_GetClocksPerSecond()));

	while(true)
	{
		task_delay(/*5000 20050918 change to */ST_GetClocksPerSecond()/60/*1000*/);/*15625 ticks per second*/
		CountTemp++;
		if(CountTemp>=200) CountTemp=200;


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
			if(CountTemp>15/*3,6*/) 
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
		if(SendSign==TRUE/*&&KeyPressed!=16*/)
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

				STTBX_Print(("\n front panel key Message Sendout  keyvalue=%d\n",KeyPressed));
				#ifdef	KEY_DEBUG
				#endif

			}
		}
	}
}
#endif /*yxl 2006-05-03 add this line*/


#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL /*yxl 2006-05-03 add this line*/
BOOL FrontKeybordInit(void)
{

	task_t* ptidFrontKBCheckTask;

	ptidFrontKBCheckTask = Task_Create(FrontKeybordCheckTask, NULL, 1024, 6+3/*yxl 2006-04-14 modify 7 ro 6*/, "FrontKeybordChecktask", 0 );
	if(ptidFrontKBCheckTask==NULL)
	{
		STTBX_Print(("can't create FrontKeybordChecktask successfully"));
		return TRUE;
	}
	else STTBX_Print(("create FrontKeybordChecktask successfully"));
	return FALSE;
}

#endif /*yxl 2006-05-03 add this line*/


/*End of file*/


















