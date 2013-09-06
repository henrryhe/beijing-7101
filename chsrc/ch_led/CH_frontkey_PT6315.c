/*
(c) Copyright changhong
  Name:CH_frontkey_PT6315.c
  Description: 实现由PT6315 控制的前面板按键相关操作
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

#if 1
#define FRONT_KEY_DEBUG
#endif


BOOL CH_VFD_ReadKey(U8* pKeyRead)
{
	BOOL res;
	U8 BufTemp[10];
	
	
	res=CH_VFD_SendCommand(CONTROL_READ_FRONTKEY,0,NULL);
	
	res=CH_VFD_ReadCommand(CONTROL_READ_FRONTKEY,2,BufTemp);
	
	
	if(BufTemp[0]!=CONTROL_READ_FRONTKEY) 
	{
		
		return TRUE;
	}
	
	
	if(BufTemp[1]!=0)
	{
		*pKeyRead=BufTemp[1];
		return FALSE;
	}
	else return TRUE;
	
	
	return res;

}





static message_queue_t *pstUsifMsgQueue;

void AFrontKeybordCheckTask( void *pParam )
{
	STPIO_OpenParams_t OpenParams;
	STPIO_Handle_t FrontKBPIOHandle,FrontKBScanHandle;	
	ST_ErrorCode_t ErrCode;		
    unsigned char  StatusScanH = 0;	/* the status of PIO input when scan line is High */
	unsigned char  StatusScanL = 0;	/* the status of PIO input when scan line is low */
	
	U8 KeyPressedLast=0xff;/* the valid key pressed last time,initlia value is 0xff*/
	
	U8 KeyPressed;/* the valid key pressed current*/
	int CountTemp=0;
	BOOL SendSign=FALSE;/* when true,send out the message of front panel button pressed */
    usif_cmd_t* pMsg;
	U8 InputMaskTemp;
	clock_t timeout;
	BOOL res;
	
	OpenParams.ReservedBits = ( PIO_BIT_7 );
	OpenParams.BitConfigure[7] = STPIO_BIT_INPUT;
	OpenParams.IntHandler =  NULL; 
	ErrCode = STPIO_Open(PIO_DeviceName[4], &OpenParams, &FrontKBPIOHandle );
	
	if( ErrCode != ST_NO_ERROR )
	{
		STTBX_Print(("PIO3 Open false!\nTest end abnormally\n"));
		return ;
	}
	
	InputMaskTemp=OpenParams.ReservedBits;	
	
	
	STTBX_Print(("\nYxlInfo:Here,ST_GetClocksPerSecond()=%d\n",ST_GetClocksPerSecond()));
	
	while(true)
	{
		task_delay(ST_GetClocksPerSecond()/100);/*15625 ticks per second,ST_GetClocksPerSecond()/60*/
	/*	STTBX_Print(("\nYxlInfo:Here,AAA"));*/
		
		ErrCode = STPIO_Read(FrontKBPIOHandle, &StatusScanL);
		if( ErrCode != ST_NO_ERROR )
		{
			
			STTBX_Print(("STPIO_Read()=%s",GetErrorText(ErrCode)));
		
		}
		else
		{
			StatusScanL=StatusScanL&InputMaskTemp;
		/*	STTBX_Print(("\nYxlInfo--B:StatusScanL=%x %x",StatusScanL,InputMaskTemp));	/*		*/

		}
		
		if(StatusScanL==0x80/*TRUE*/)
		{
			STTBX_Print(("\nYxlInfo--C:StatusScanL=%x %x",StatusScanL,InputMaskTemp));		/*	*/

			KeyPressed=0;
			res=CH_VFD_ReadKey(&KeyPressed);
			if(res==TRUE) continue;
			SendSign=TRUE;
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
#ifdef FRONT_KEY_DEBUG				
				STTBX_Print(("\n front panel key Message Sendout  keyvalue=%d\n",KeyPressed));

#endif
			
			}
		}
	}
}


BOOL AFrontKeybordInit(void)
{

	task_t* ptidFrontKBCheckTask;

	    if (symbol_enquire_value("usif_queue", (void **)&pstUsifMsgQueue))
    {
		STTBX_Print(("\n\nCan't find USIF message queue\n"));

		return TRUE;
    }

	ptidFrontKBCheckTask = Task_Create(AFrontKeybordCheckTask, NULL, 1024, 6+3/*yxl 2006-04-14 modify 7 ro 6*/, "FrontKeybordChecktask", 0 );
	if(ptidFrontKBCheckTask==NULL)
	{
		STTBX_Print(("can't create FrontKeybordChecktask successfully"));
		return TRUE;
	}
	else STTBX_Print(("create FrontKeybordChecktask successfully"));
	return FALSE;
}




/*End of file*/


















