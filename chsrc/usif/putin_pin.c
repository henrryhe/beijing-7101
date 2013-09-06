
/*************/
/*   myx added   */  
/*   2004/9 changed  */
/**********************/


//#include "..\util\errors.h"

#include "..\main\initterm.h"
#include "graphicmid.h"
#include "..\key\key.h"
#include "..\key\keymap.h"
#include "putin_pin.h"
#include "..\dbase\vdbase.h"

#define DIALOG_X      246 
#define DIALOG_Y      170 
#define DIALOG_WIDTH  235
#define DIALOG_HEIGHT 196

#define MAX_DIA_DEEP   10
static U8 * Dialog_Save_infomation[MAX_DIA_DEEP]=
{
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL
};
static signed char   DisDeepIndex=0;


boolean	CH6_AutoPopupDialog(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *DialogContent,char Buttons,int WaitSeconds)
{ 
	   
	   int Dia_x;
	   int Dia_y;
	   ApplMode CurAppMode=CH_GetCurApplMode();
	   if(CurAppMode==APP_MODE_EPG)
	   {
		   Dia_x=DIALOG_X+180;
		   Dia_y=DIALOG_Y-135;
	   }
#ifdef INTEGRATE_NVOD/*2005.12.22 shb add*/	
	   else  if(CurAppMode==APP_MODE_NVOD&&Nvod_GetMenuStatus()==true)
	   {
		   Dia_x=DIALOG_X-130;
		   Dia_y=DIALOG_Y;
		   
	   }
#endif
	   else if(CurAppMode==APP_MODE_STOCK)
	   {
           Dia_x=DIALOG_X-10;
		   Dia_y=DIALOG_Y-10;
	   }
	   else if(CurAppMode==APP_MODE_HTML)
	   	{
	 
	       Dia_x=DIALOG_X-11;
		   Dia_y=DIALOG_Y+18;
		 }
	   else
	   {
		   Dia_x=DIALOG_X;
		   Dia_y=DIALOG_Y;
	   }
	   CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_x,Dia_y,DIALOG1_PIC,true);	  	
	   

	   /*display dialog text*/	   
	   CH6_MulLine_ChangeLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_x/*+10*/,Dia_y,DIALOG_WIDTH/*-20*/,DIALOG_HEIGHT,DialogContent,DIALOG_TEXT_COLOR,2,2,true);

	   /***************/
	   CH_UpdateAllOSDView();/*zxg 2005-11-09 add */

}



/*CLearDialog*/
void CH6_ClearDialog(void)
{

     int Dia_x;
	   int Dia_y;
	   ApplMode CurAppMode=CH_GetCurApplMode();
	   if(CurAppMode==APP_MODE_EPG)
	   {
		   Dia_x=DIALOG_X+180;
		   Dia_y=DIALOG_Y-135;
	   }
#ifdef INTEGRATE_NVOD/*2005.12.22 shb add*/	
	   else  if(CurAppMode==APP_MODE_NVOD&&Nvod_GetMenuStatus()==true)
	   {
		   Dia_x=DIALOG_X-130;
		   Dia_y=DIALOG_Y;
		   
	   }
#endif
	   else if(CurAppMode==APP_MODE_STOCK)
	   {
           Dia_x=DIALOG_X-10;
		   Dia_y=DIALOG_Y-10;
	   }
	   else
	   {
		   Dia_x=DIALOG_X;
		   Dia_y=DIALOG_Y;
	   }
  CH6_ClearArea(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_x,Dia_y,DIALOG_WIDTH,DIALOG_HEIGHT);	
 CH_UpdateAllOSDView();/*20051215 add*/
}
/*得到对话框显示之前对话框显示区域显示内存数据*/
U8* CH6_GetDialogMemory(int *MemmorySize)
{   
   U8 *Save_infomation;
     int Dia_x;
	   int Dia_y;
	   ApplMode CurAppMode=CH_GetCurApplMode();
	   if(CurAppMode==APP_MODE_EPG)
	   {
		   Dia_x=DIALOG_X+180;
		   Dia_y=DIALOG_Y-135;
	   }
#ifdef INTEGRATE_NVOD/*2005.12.22 shb add*/	
	   else  if(CurAppMode==APP_MODE_NVOD&&Nvod_GetMenuStatus()==true)
	   {
		   Dia_x=DIALOG_X-130;
		   Dia_y=DIALOG_Y;
		   
	   }
#endif
	   else if(CurAppMode==APP_MODE_STOCK)
	   {
           Dia_x=DIALOG_X-10;
		   Dia_y=DIALOG_Y-10;
	   }
	    else if(CurAppMode==APP_MODE_HTML)
	   	{
	 
	       Dia_x=DIALOG_X-11;
		   Dia_y=DIALOG_Y+18;
		 }
	   else
	   {
		   Dia_x=DIALOG_X;
		   Dia_y=DIALOG_Y;
	   }

   Save_infomation=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_x,Dia_y,DIALOG_WIDTH,DIALOG_HEIGHT);
#ifdef OSD_COLOR_TYPE_RGB16
   *MemmorySize=DIALOG_WIDTH*DIALOG_HEIGHT*2;
#else
   *MemmorySize=DIALOG_WIDTH*DIALOG_HEIGHT*4;

#endif
   return Save_infomation;
}
/*恢复对话框显示之前对话框显示区域显示内存数据*/
void CH6_SetDialogMemory(U8 *MemmoryData)
{   
	   int Dia_x;
	   int Dia_y;

	

	   ApplMode CurAppMode=CH_GetCurApplMode();

	      if(MemmoryData==NULL)
	   {
		   return;
	   }

	   if(CurAppMode==APP_MODE_EPG)
	   {
		   Dia_x=DIALOG_X+180;
		   Dia_y=DIALOG_Y-135;
	   }
#ifdef INTEGRATE_NVOD/*2005.12.22 shb add*/	
	   else  if(CurAppMode==APP_MODE_NVOD&&Nvod_GetMenuStatus()==true)
	   {
		   
		   /*CH_NvodMenuRestore();*//*20050107 add*/

		   Dia_x=DIALOG_X-130;
		   Dia_y=DIALOG_Y;
		   /*return;*/
   
	   }
#endif
	   else if(CurAppMode==APP_MODE_STOCK)
	   {
           Dia_x=DIALOG_X-10;
		   Dia_y=DIALOG_Y-10;
	   }
	    else if(CurAppMode==APP_MODE_HTML)
	   	{
	 
	       Dia_x=DIALOG_X-11;
		   Dia_y=DIALOG_Y+18;
		 }
	   else
	   {
		   Dia_x=DIALOG_X;
		   Dia_y=DIALOG_Y;
	   }
	   if(MemmoryData!=NULL)
	   {
		   CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_x,Dia_y,DIALOG_WIDTH,DIALOG_HEIGHT,MemmoryData); 
	   }
	   else
	   {
		   CH6_ClearDialog();
	   }
		CH_UpdateAllOSDView();
}


/*对话框*/
/*yxl 2004-12-26 add  paramter U8 DefReturnValue: 0,standfor return OK button when no key pressed,
1,standfor return exit button when no key pressed,
other value,standfor return OK button when no key pressed*/
boolean	CH6_PopupDialog(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *DialogContent,char Buttons,int WaitSeconds,U8 DefReturnValue)
{ 
	   int iKeyScanCode;   

	   
 	   clock_t StartTime;/*zxg20050113 add*/

	   boolean NOEXIT_TAG=true;
	   int YesNO=1;/*defaut yes*/
	   
	   /*20050305 add*/
	   int Dia_X;
	   int Dia_Y;
       ApplMode iAppMode;

	   iAppMode=CH_GetCurApplMode();
	   switch(iAppMode)
	   {
	   case APP_MODE_STOCK:
		   Dia_X=DIALOG_X-10;
		   Dia_Y=DIALOG_Y-10;
		   break;

	   case APP_MODE_HTML:
         	
				Dia_X=DIALOG_X-13;
				Dia_Y=DIALOG_Y+12;
         	   break;
		case APP_MODE_NVOD:
				
         	   break;
	   default:
		   Dia_X=DIALOG_X;
		   Dia_Y=DIALOG_Y;
		   break;
	   }
	   /***************/
	  if(DefReturnValue==1) YesNO=0; /*yxl 2004-12-26 add this line*/

	   StartTime=time_now();/*zxg20050113 add*/

       if(WaitSeconds!=0)/*得到显示之前显示内存*/
	   {
#if 1/*20050301 move to here*/
		   if(DisDeepIndex>=MAX_DIA_DEEP)
		   {
               return false;
		   }
#endif
		   Dialog_Save_infomation[DisDeepIndex]=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT);
		   if(Dialog_Save_infomation[DisDeepIndex]==NULL)
		   {
			   return false;
		   }
		   DisDeepIndex++;
#if 0/*20050301 move to up*/
		   if(DisDeepIndex>MAX_DIA_DEEP)
		   {
               return false;
		   }
#endif
	   }

	CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC, Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT, SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);

	CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle, Dia_X+(DIALOG_WIDTH-50)/2, Dia_Y+DIALOG_HEIGHT-50, 60, 30, "OK",SYSTEM_FONT_NORMA_COLORL ,whitecolor, 1, 1);
	CH_UpdateAllOSDView();
	
	   if(Buttons>=1)
	   {
		   
		   int LeftSeconds=1;
		   

		   CH6_MulLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+10,Dia_Y,DIALOG_WIDTH-20,
			   DIALOG_HEIGHT-50,DialogContent,DIALOG_TEXT_COLOR,2,2);
		   

		   if(WaitSeconds!=-1)
			   LeftSeconds=WaitSeconds;
	   
		   CH_UpdateAllOSDView();/*20051215 add*/
		   while(NOEXIT_TAG)
		   {
			   
			   if(WaitSeconds==-1)
			   {
				   iKeyScanCode=GetKeyFromKeyQueue(WaitSeconds);
			   }
			   else 
			   {
				   iKeyScanCode=GetKeyFromKeyQueue ( 1 );
#if 0				   
				   LeftSeconds--;
#else
                    LeftSeconds=CH_GetLeaveTime(StartTime,WaitSeconds);
#endif
			   }
			   
			   switch(iKeyScanCode)
			   {
			   case OK_KEY:
				   if(Buttons >= 2)
				   {
					   YesNO=1;
					   NOEXIT_TAG=false;
				   }
				   break;
			   case EXIT_KEY:
			   /*case MENU_KEY:*/
				   NOEXIT_TAG=false;
				   YesNO=0;
				   break;
			   default:                                   
				   break;
			   }
			   if(LeftSeconds<=0)
			   {
				   NOEXIT_TAG=false;	
				   /*YesNO=1;*/
			   }
		   }
		   
		   if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
		   {
			   CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex]); 
                          CH_UpdateAllOSDView();/*20051215 add*/
			   memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);
			   Dialog_Save_infomation[DisDeepIndex]=NULL;
		   }
	   }
	   else
	   {

      /*20060109 add*/
#ifdef SINO_FONT
	   CH_SetPicBack();
#endif
       /**************/
		   /*display dialog text*/	   
		   CH6_MulLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+10,Dia_Y,DIALOG_WIDTH-20,DIALOG_HEIGHT,DialogContent,DIALOG_TEXT_COLOR,2,2);
		/*20060109 add*/
#ifdef SINO_FONT
	   CH_ClearPicBack();
#endif
       /**************/
		   CH_UpdateAllOSDView();/*zxg 2005-11-09 add */

		   if(WaitSeconds>0)
		   {		   	 
			   iKeyScanCode=GetKeyFromKeyQueue(WaitSeconds );
			   if(DisDeepIndex>0&&Dialog_Save_infomation[DisDeepIndex-1]!=NULL)
                             {
				   CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex-1]); 				   
                                   CH_UpdateAllOSDView();/*20051215 add*/
                             }
		   }
		   if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
		   {
			   memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);
			   Dialog_Save_infomation[DisDeepIndex]=NULL;
		   }
	   }		   		 	   
	   return  YesNO ;
}



/*显示输入密码框*/
/*0，密码成功，1，表示密码失败，2，表示取消,3,表示其它*/
int	CH6_PIN_PopupDialog (char *DialogTile ,int WaitSeconds,boolean ResponsePKEY)
{
	int iKeyScanCode;       
	int i,x,y,TextX,TextY;	   
	boolean ExitLoop=false;	   
	int errorflag=0;
       char Tempstr[8];
	char PinCode[8];
	clock_t StartTime;/*zxg20050113 add*/
	char TempCharactor[2]={'\0','\0'};/*cqj 20070726 modify*/
	int CurIputPos=0;	
	int PinRectWidth=30;
	#ifdef SINO_FONT/*20060420 change*/
	int PinRectHeight=36;
	#else
	int PinRectHeight=30;
	#endif
   	 int LeftSeconds=1;



	   int Dia_X;
	   int Dia_Y;
       ApplMode iAppMode;
	   
	   iAppMode=CH_GetCurApplMode();
	   switch(iAppMode)
	   {
	   case APP_MODE_STOCK:
		   Dia_X=DIALOG_X-10;
		   Dia_Y=DIALOG_Y-10;
		   break;
	   default:
		   Dia_X=DIALOG_X;
		   Dia_Y=DIALOG_Y;
		   break;
	   }
	   /***************/

	 StartTime=time_now();/*zxg20050113 add*/

	x=Dia_X+55;
	y=Dia_Y+90;
	TextX=x+(PinRectWidth-CHARACTER_WIDTH)/2;
#ifdef SINO_FONT/*20060420 change*/
       TextY=y+(PinRectHeight-CHARACTER_WIDTH*2)/2+5;
#else
	TextY=y+(PinRectHeight-CHARACTER_WIDTH*2)/2;
#endif
	/*初始化密码为空,把要显示的密码字符串初始化为"???????\0"*/
	for(i=0;i<8;i++)
	{
		PinCode[i]='\0';
		Tempstr[i] = 0;
		if(i==7)
			Tempstr[i]='\0';
	}	
	if(WaitSeconds!=0)/*得到显示之前显示内存*/
	{
#if 1/*20050301 move to here*/
		   if(DisDeepIndex>=MAX_DIA_DEEP)
		   {
               return false;
		   }
#endif
		Dialog_Save_infomation[DisDeepIndex]=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT);
		if(Dialog_Save_infomation[DisDeepIndex]==NULL)
		{
			return 1;
		}
		DisDeepIndex++;
#if 0/*20050301 move to up*/
			if(DisDeepIndex>MAX_DIA_DEEP)
		   {
               return false;
		   }
#endif
	}   	   

	switch(iAppMode)
	   {
	   case APP_MODE_PLAY:
	   case APP_MODE_NVOD:
	   case APP_MODE_LIST:
		CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC, Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT, SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);
		   break;
	   default:
		  CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC, Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT, SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);

		   break;

   }

       /*20060109 add*/
#ifdef SINO_FONT
	   CH_SetPicBack();
#endif
       /**************/
	CH6_MulLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+20,Dia_Y+10,DIALOG_WIDTH-40,/*DIALOG_HEIGHT-40*/72,DialogTile,DIALOG_TEXT_COLOR,2,2);
	  /*20060109 add*/
#ifdef SINO_FONT
	   CH_ClearPicBack();
#endif
       /**************/
	for(i=0;i<4;i++)
	{
		TempCharactor[0]=Tempstr[i];/*cqj 20070726 modify*/
		CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x+33*i,y,PinRectWidth,PinRectHeight,4,DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1);
		CH6_EnTextOut(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,TextX+33*i,TextY,DIALOG_PIN_BORDER_COLOR,&TempCharactor);			
	}
	CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle, Dia_X+(DIALOG_WIDTH-50)/2, Dia_Y+DIALOG_HEIGHT-50, 60, 30, "OK",SYSTEM_FONT_NORMA_COLORL ,whitecolor, 1, 1);
       CH_UpdateAllOSDView();/*20051215 add*/
   if(WaitSeconds!=-1)
			LeftSeconds=WaitSeconds;
	while(!ExitLoop)
	{	 
		if(WaitSeconds==-1)
		{
			iKeyScanCode=GetKeyFromKeyQueue(WaitSeconds);
		}
		else 
		{
			iKeyScanCode=GetKeyFromKeyQueue ( 1 );
			
#if 0				   
			LeftSeconds--;
#else/*zxg20050113 change*/
			LeftSeconds=CH_GetLeaveTime(StartTime,WaitSeconds);
#endif
		}	
		switch(iKeyScanCode)
		{
		/* Jqz add on 041116(add_main) */
		case LEFT_ARROW_KEY:

			if( CurIputPos == 0 )
				break;

			CurIputPos --;

			TempCharactor[0] = Tempstr[CurIputPos] = 0;/*cqj 20070726 modify*/
			PinCode[CurIputPos] = 0;

			CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle,TextX+33*CurIputPos,TextY,CHARACTER_WIDTH,CHARACTER_WIDTH*2,(char *)(&TempCharactor),DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1,1);			
                       CH_UpdateAllOSDView();/*20051215 add*/
			break;
		/* End Jqz add on 041116(add_main) */
		case OK_KEY:
			if(PasswordVerify(PinCode)!=0)/*如果密码错误，不能进入*/
				errorflag=1;
			else 
				errorflag=0;
			ExitLoop=true;
			break;				 
		case EXIT_KEY:
		/*case MENU_KEY:*/
			errorflag=2;	
			ExitLoop=true;
			break;	
		case UP_ARROW_KEY:
		case DOWN_ARROW_KEY:
		case P_UP_KEY:
		case P_DOWN_KEY:
			if(ResponsePKEY==true)/*响应节目切换键*/
			{
				ExitLoop=true;
                errorflag=iKeyScanCode;
			}
			break;
		default:
			if((iKeyScanCode>=KEY_DIGIT0)&&(iKeyScanCode<=KEY_DIGIT9))
			{
				if(CurIputPos>3)
				{
					CurIputPos=0;
					/*恢复密码为空,把要显示的密码字符串恢复为"???????\0"*/
					for(i=0;i<8;i++)
					{
						PinCode[i]='\0';
						Tempstr[i]= 0;
						if(i==7)
							Tempstr[i]='\0';
					}
					/*重新显示*/
					for(i=0;i<4;i++)
					{
						TempCharactor[0]=Tempstr[i];/*cqj 20070726 modify*/
						CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle,TextX+33*i,TextY,CHARACTER_WIDTH,CHARACTER_WIDTH*2,(char *)(&TempCharactor),DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1,1);							
					}
				}
				PinCode[CurIputPos]=Convert2Char(iKeyScanCode);
				TempCharactor[0]=Tempstr[CurIputPos]='*';	/*cqj 20070726 modify*/				
				CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle,TextX+33*CurIputPos,TextY,CHARACTER_WIDTH,CHARACTER_WIDTH*2,(char *)(&TempCharactor),DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1,1);			
				CurIputPos++;
                                CH_UpdateAllOSDView();/*20051215 add*/
				if( CurIputPos > 3 )
				{
					InitKeyQueue();/*yxl 2005-01-20 add this line*/
					PutKeyIntoKeyQueue( OK_KEY );
				}
			}
			break;
		}	 	 	  
		if(LeftSeconds<=0)
		{
			errorflag=2;	
			ExitLoop=true;
		}
	}
	
	if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
	{
		CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex]); 
                CH_UpdateAllOSDView();/*20051215 add*/
		memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);	   
		Dialog_Save_infomation[DisDeepIndex]=NULL;
	}
	return errorflag;	   
}
#if 1/*20050203 add */
int  CH6_InputLcokPinCode(boolean ReponseProKey)
{
	int errorflag;
	int times=3;
	while(times>0)
	{
		if(times==3)
			errorflag=CH6_PIN_PopupDialog  ((pstBoxInfo->abiLanguageInUse==0)?"Locked channel,\ninput the PIN. ":"Locked channel,\ninput the PIN. ",20,ReponseProKey);
		else
			errorflag=CH6_PIN_PopupDialog  ((pstBoxInfo->abiLanguageInUse==0)?"Wrong PIN!\nTry again.?:":"Wrong PIN!\nTry again.",20,ReponseProKey);
		
		if(errorflag==2)
		{
			return 1;
		}	
		else if(errorflag==0)
		{
			return 0;
		}
		else if(errorflag==1)
		{
			
		}
		else
		{
			return errorflag;
		}
		times--;
	}
	return 1;
}
#endif
/*0，密码成功，1，表示密码失败，2，表示取消*/
/*8.23 added*/
int  CH6_InputPinCode(boolean ReponseProKey)
{
	
	int errorflag;
	int times=3;
	while(times>0)
	{
		if(times==3)
			errorflag=CH6_PIN_PopupDialog  ((pstBoxInfo->abiLanguageInUse==0)?"Please input\nthe PIN":"Please input\nthe PIN",20,ReponseProKey);
		else
			errorflag=CH6_PIN_PopupDialog  ((pstBoxInfo->abiLanguageInUse==0)?"Wrong PIN!\nTry again.":"Wrong PIN!\nTry again.",20,ReponseProKey);
		
		if(errorflag==2)
		{
			return 1;
		}	
		else if(errorflag==0)
		{
			return 0;
		}
		else if(errorflag==1)
		{
			
		}
		else
		{
			return errorflag;
		}
		times--;
	}
	return 1;
}



/*进入待机时对话框内存释放*/
void CH_StandbyDiaFreeMemory(void)
{
	int i;
	for(i=0;i<MAX_DIA_DEEP;i++)
	{
 	   if(Dialog_Save_infomation[i]!=NULL)
	   {
		   memory_deallocate(CHSysPartition,Dialog_Save_infomation[i]);
		   Dialog_Save_infomation[i]=NULL;
	   }
	}
	DisDeepIndex=0;/*20050218 add*/
	
#ifdef INTEGRATE_NVOD/*2005.12.22 shb add*/
	CH_NvodFreeDiaMemory();
#endif
}

/*专用于定时器提示信息显示*/
void CH6_MulLine_ChangeTimerLine(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,
							int x_top,int y_top,int Width,int Heigth,char *TxtStr,
							int txtColor,U8 H_Align,U8 V_Align,boolean MulLine_ALine)
{
	int LineNum;
	char AllIine[20][100];
	int iTextXPos,iTextYPos;
	int TextLen;
	int TextPos;
	boolean IsDisplayAll;
	int i;
	int TextNum;
	STGFX_Handle_t GFXHandleTemp=GHandle; 
	STLAYER_ViewPortHandle_t VPHandleTemp=VPHandle; 
	TextLen=CH6_GetTextWidth(TxtStr);
	
	x_top=x_top+6;
	Width=Width-12;
	
	if(TxtStr==NULL)
		return;
	TextNum=strlen(TxtStr);


	TextPos=CH_GetCharLine(Width,Heigth,TxtStr,(char *)&AllIine[0][0],100,20,&LineNum,false);

    if(TextPos>=TextNum)
	{
       IsDisplayAll=true;
	}
	else
	{
       IsDisplayAll=false;
	}
	
	iTextXPos=x_top;
	iTextYPos=y_top;
#if 1
	if(LineNum<=1&&IsDisplayAll==true)/*只有一行显示*/
	{
		switch(H_Align)
		{
		case 1:/*居左*/
			break;
		case 2:/*居中*/
			iTextXPos=x_top+(Width-TextLen)/2;				
			break;
		case 3:/*居右*/
			iTextXPos=x_top+(Width-TextLen);				
			break;
		default:
			break;	
		}	
	}
#endif
	switch(V_Align)
	{
	case 1:
		
		break;
	case 2:	
		iTextYPos=iTextYPos+(Heigth-CHARACTER_WIDTH*2*LineNum)/2;
		break;
	case 3:
		iTextYPos=iTextYPos+(Heigth-CHARACTER_WIDTH*2*LineNum);
		break;
	default:
		break;
		
	}	
	
	for(i=0;i<LineNum;i++)
	{
		if(MulLine_ALine==true)
		{
			TextLen=CH6_GetTextWidth(AllIine[i]);
			switch(H_Align)
			{
			case 1:/*居左*/
				break;
			case 2:/*居中*/
				iTextXPos=x_top+(Width-TextLen)/2;				
				break;
			case 3:/*居右*/
				iTextXPos=x_top+(Width-TextLen);				
				break;
			default:
				break;	
			}
		}
		if(IsDisplayAll==false&&i==(LineNum-1))/*需要显三个点完毕表示没有显示,*/
		{
            int TempLen=strlen(AllIine[i]);
			int TempTxtLen=CH6_GetTextWidth(AllIine[i]);

			if(iTextXPos+TempTxtLen+CHARACTER_WIDTH*2>=x_top+Width)
			{
				AllIine[i][TempLen-1]=AllIine[i][TempLen-2]='\0';
				TextPos-=2;
                
				CH6_DrawPoint(iTextXPos+TempTxtLen-CHARACTER_WIDTH*2,iTextYPos,txtColor);
				/*显示 事件名的引号*/
				CH6_TextOut(GFXHandleTemp,VPHandleTemp,iTextXPos+TempTxtLen,iTextYPos,txtColor,"\"");
			}
			else
			{
				CH6_DrawPoint(iTextXPos+TempTxtLen,iTextYPos,txtColor);
				/*显示 事件名的引号*/
				CH6_TextOut(GFXHandleTemp,VPHandleTemp,iTextXPos+CHARACTER_WIDTH*2,iTextYPos,txtColor,"\"");
			}
		
		}

		CH6_TextOut(GFXHandleTemp,VPHandleTemp,iTextXPos,iTextYPos,txtColor,AllIine[i]);

		iTextYPos+=CHARACTER_WIDTH*2;
	}
                    CH_UpdateAllOSDView();/*20051215 add*/
}


boolean	CH6_PopupTimerDialog(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *TimerName,char *InfoName,char Buttons,int WaitSeconds,U8 DefReturnValue)
{ 
	   int iKeyScanCode;   
	   
 	   clock_t StartTime;/*zxg20050113 add*/

	   boolean NOEXIT_TAG=true;
	   int YesNO=1;/*defaut yes*/
	   
	   /*20050305 add*/
	   int Dia_X;
	   int Dia_Y;
       ApplMode iAppMode;
	   
	   iAppMode=CH_GetCurApplMode();
	   switch(iAppMode)
	   {
	   case APP_MODE_STOCK:
		   Dia_X=DIALOG_X-10;
		   Dia_Y=DIALOG_Y-10;
		   break;
		case APP_MODE_HTML:
		   Dia_X=DIALOG_X-13;
		   Dia_Y=DIALOG_Y+12;
			break;
	   default:
		   Dia_X=DIALOG_X;
		   Dia_Y=DIALOG_Y;
		   break;
	   }
	   /***************/

	  if(DefReturnValue==1) 
		  YesNO=0; /*yxl 2004-12-26 add this line*/

	   StartTime=time_now();/*zxg20050113 add*/

       if(WaitSeconds!=0)/*得到显示之前显示内存*/
	   {
		   Dialog_Save_infomation[DisDeepIndex]=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT);
		   if(Dialog_Save_infomation[DisDeepIndex]==NULL)
		   {
			   return false;
		   }
		   DisDeepIndex++;

		   if(DisDeepIndex>MAX_DIA_DEEP)
		   {
               return false;
		   }

	   }

	   switch(iAppMode)
	   {
	   case APP_MODE_PLAY:
	   case APP_MODE_NVOD:
	   case APP_MODE_LIST:
		   CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X,Dia_Y,DIALOG2_PIC,true);
		   break;
	   default:
		   CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X,Dia_Y,DIALOG1_PIC,true);
	  	
		   break;

	   }
	  
             CH_UpdateAllOSDView();/*20051215 add*/
  	   
	   /*display button*/
	   if(Buttons>=1)
	   {
		   
		   int LeftSeconds=1;
		   
	    /*20060109 add*/
#ifdef SINO_FONT
	   CH_SetPicBack();
#endif
       /**************/
		   /*display dialog text*/	   
		   CH6_MulLine_ChangeTimerLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+15,Dia_Y+27,DIALOG_WIDTH-36,50,TimerName,DIALOG_TEXT_COLOR,2,2,true);
		   
		   CH6_MulLine_ChangeLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+/*15*/9,Dia_Y+DIALOG_HEIGHT-120,DIALOG_WIDTH/*-35*/,50,InfoName,DIALOG_TEXT_COLOR,2,2,false);/*20070819 modify no check*/
	   /*20060109 add*/
#ifdef SINO_FONT
	   CH_ClearPicBack();
#endif
       /**************/	   
		   if(WaitSeconds!=-1)
			   LeftSeconds=WaitSeconds;
#if 0		   
		   if(Buttons==1)/*Display one button*/
			   CH6_DisplayKeyButton(Dia_X+(DIALOG_WIDTH-50)/2, Dia_Y+DIALOG_HEIGHT-50,BUTTON_EXIT);
		   if(Buttons >= 2)/*Display two button*/
		   {
			   CH6_DisplayKeyButton(Dia_X+45, Dia_Y+DIALOG_HEIGHT-50,BUTTON_OK);
			   CH6_DisplayKeyButton(Dia_X+DIALOG_WIDTH-95, Dia_Y+DIALOG_HEIGHT-50,BUTTON_EXIT);
			   
		   }
#endif		   
		   CH_UpdateAllOSDView();/*20051215 add*/
		   while(NOEXIT_TAG)
		   {
			   
			   if(WaitSeconds==-1)
			   {
				   iKeyScanCode=GetKeyFromKeyQueue(WaitSeconds);
			   }
			   else 
			   {
				   iKeyScanCode=GetKeyFromKeyQueue ( 1 );
#if 0				   
				   LeftSeconds--;
#else/*zxg20050113 change*/
				   LeftSeconds=CH_GetLeaveTime(StartTime,WaitSeconds);
#endif
			   }
			   
			   switch(iKeyScanCode)
			   {
			   case OK_KEY:
				   if(Buttons >= 2)
				   {
					   YesNO=1;
					   NOEXIT_TAG=false;
				   }
				   break;
			   case EXIT_KEY:
				   /*case MENU_KEY:*/
				   NOEXIT_TAG=false;
				   YesNO=0;
				   break;
			   default:                                   
				   break;
			   }
			   if(LeftSeconds<=0)
			   {
				   NOEXIT_TAG=false;	
				   /*YesNO=1;*/
			   }
		   }
		   
		   if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
		   {
			   CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex]); 
			   CH_UpdateAllOSDView();/*20051215 add*/
			   memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);
			   Dialog_Save_infomation[DisDeepIndex]=NULL;
		   }
	   }
	   else
	   {
	     /*20060109 add*/
#ifdef SINO_FONT
	   CH_SetPicBack();
#endif
       /**************/
		   /*display dialog text*/	   
		   CH6_MulLine_ChangeTimerLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+18,Dia_Y+27,DIALOG_WIDTH-36,50,TimerName,DIALOG_TEXT_COLOR,2,2,true);
		   
		   CH6_MulLine_ChangeLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+/*18*/12,Dia_Y+DIALOG_HEIGHT-120,DIALOG_WIDTH/*-35*/,50,InfoName,DIALOG_TEXT_COLOR,2,2,false);
   /*20060109 add*/
#ifdef SINO_FONT
	   CH_ClearPicBack();
#endif
       /**************/
		   if(WaitSeconds>0)
		   {		   	 
			   iKeyScanCode=GetKeyFromKeyQueue(WaitSeconds );
			   if(DisDeepIndex>0&&Dialog_Save_infomation[DisDeepIndex-1]!=NULL)
                             {
			      CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex-1]); 				   
                              CH_UpdateAllOSDView();/*20051215 add*/
                              }
		   }
		   if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
		   {
			   memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);
			   Dialog_Save_infomation[DisDeepIndex]=NULL;
		   }
	   }		   		 	   
	   return  YesNO ;
}


/*lzf 20060615 add aitchange dialog function*/
boolean	eis_ait_change_dialog(void)
{ 
	boolean exitdlg;
	int iKeyScanCode;   

	int Dia_X;
	int Dia_Y;
	   
	Dia_X=DIALOG_X-11;
	Dia_Y=DIALOG_Y+18;
	   
	if(DisDeepIndex>=MAX_DIA_DEEP)
	{
		return false;
	}

	Dialog_Save_infomation[DisDeepIndex]=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT);
	if(Dialog_Save_infomation[DisDeepIndex]==NULL)
	{
		return false;
	}
		   
	DisDeepIndex++;
	   
	CH6_DisplayPIC(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X,Dia_Y,DIALOG1_PIC,true);

#ifdef SINO_FONT
	CH_SetPicBack();
#endif

	CH6_MulLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+10,Dia_Y,DIALOG_WIDTH-20,
		DIALOG_HEIGHT-50,
		(pstBoxInfo->abiLanguageInUse ==0)? "Website updated,\nplease exit and \nreenter         ":"Website updated,\nplease exit and \nreenter         ",
		DIALOG_TEXT_COLOR,2,2);
		   
#ifdef SINO_FONT
	CH_ClearPicBack();
#endif
	//CH6_DisplayKeyButton(Dia_X+(DIALOG_WIDTH-50)/2, Dia_Y+DIALOG_HEIGHT-50,BUTTON_OK);
	CH_UpdateAllOSDView();
	exitdlg=true;		   
	while(exitdlg)
	{
		iKeyScanCode=GetKeyFromKeyQueue(-1);
	   
		switch(iKeyScanCode)
		{
			case OK_KEY:
			/*case EXIT_KEY:*/
				exitdlg=false;
				break;
		}
	}
		   
	if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
	{
		CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex]); 
		CH_UpdateAllOSDView();
		memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);
		Dialog_Save_infomation[DisDeepIndex]=NULL;
	}
	   		   		 	   
	return true;
}


#ifdef STBID_FROM_E2PROM
/*fj 20080311 add pincode to write stbid into e2p*/

int	CH6_PIN_PopupDialog_stbid (char *DialogTile ,int WaitSeconds,boolean ResponsePKEY)
{
	int iKeyScanCode;       
	int i,x,y,TextX,TextY;	   
	boolean ExitLoop=false;	   
	int errorflag=0;
       char Tempstr[8];
	char PinCode[8];
	clock_t StartTime;/*zxg20050113 add*/
	char TempCharactor;
	int CurIputPos=0;	
	int PinRectWidth=30;
	#ifdef SINO_FONT/*20060420 change*/
	int PinRectHeight=36;
	#else
	int PinRectHeight=30;
	#endif
    int LeftSeconds=1;
/*	if(Dialog_Save_infomation!=NULL)
	{
		memory_deallocate(chsys_partition,Dialog_Save_infomation);
		Dialog_Save_infomation=NULL;
	}
*/

		/*20050305 add*/
	   int Dia_X;
	   int Dia_Y;
       ApplMode iAppMode;
	   
	   iAppMode=CH_GetCurApplMode();
	   switch(iAppMode)
	   {
	   case APP_MODE_STOCK:
		   Dia_X=DIALOG_X-10;
		   Dia_Y=DIALOG_Y-10;
		   break;
	   default:
		   Dia_X=DIALOG_X;
		   Dia_Y=DIALOG_Y;
		   break;
	   }
	   /***************/

	 StartTime=time_now();/*zxg20050113 add*/

	x=Dia_X+55;
	y=Dia_Y+90;
	TextX=x+(PinRectWidth-CHARACTER_WIDTH)/2;
#ifdef SINO_FONT/*20060420 change*/
       TextY=y+(PinRectHeight-CHARACTER_WIDTH*2)/2+5;
#else
	TextY=y+(PinRectHeight-CHARACTER_WIDTH*2)/2;
#endif
	/*初始化密码为空,把要显示的密码字符串初始化为"???????\0"*/
	for(i=0;i<8;i++)
	{
		PinCode[i]='\0';
		Tempstr[i] = 0;
		if(i==7)
			Tempstr[i]='\0';
	}	
	if(WaitSeconds!=0)/*得到显示之前显示内存*/
	{
#if 1/*20050301 move to here*/
		   if(DisDeepIndex>=MAX_DIA_DEEP)
		   {
               return false;
		   }
#endif
		Dialog_Save_infomation[DisDeepIndex]=CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT);
		if(Dialog_Save_infomation[DisDeepIndex]==NULL)
		{
			return 1;
		}
		DisDeepIndex++;
#if 0/*20050301 move to up*/
			if(DisDeepIndex>MAX_DIA_DEEP)
		   {
               return false;
		   }
#endif
	}   	   

	switch(iAppMode)
	   {
	   case APP_MODE_PLAY:
	   case APP_MODE_NVOD:
	   case APP_MODE_LIST:
		    CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC, Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT, SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);

		   break;
	   default:
		    CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC, Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT, SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);

	  
		   break;

   }

       /*20060109 add*/
#ifdef SINO_FONT
	   CH_SetPicBack();
#endif
       /**************/
	CH6_MulLine(GFXHandle,CH6VPOSD.ViewPortHandle,Dia_X+20,Dia_Y+10,DIALOG_WIDTH-40,/*DIALOG_HEIGHT-40*/72,DialogTile,DIALOG_TEXT_COLOR,2,2);
	  /*20060109 add*/
#ifdef SINO_FONT
	   CH_ClearPicBack();
#endif
       /**************/
	for(i=0;i<4;i++)
	{
		TempCharactor=Tempstr[i];
		CH6m_DrawRoundRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,x+33*i,y,PinRectWidth,PinRectHeight,4,DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1);
		CH6_EnTextOut(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,TextX+33*i,TextY,DIALOG_PIN_BORDER_COLOR,&TempCharactor);			
	}
		CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle, Dia_X+(DIALOG_WIDTH-50)/2, Dia_Y+DIALOG_HEIGHT-50, 60, 30, "OK",SYSTEM_FONT_NORMA_COLORL ,whitecolor, 1, 1);

	       CH_UpdateAllOSDView();
   if(WaitSeconds!=-1)
			LeftSeconds=WaitSeconds;
	while(!ExitLoop)
	{	 
		if(WaitSeconds==-1)
		{
			iKeyScanCode=GetKeyFromKeyQueue(WaitSeconds);
		}
		else 
		{
			iKeyScanCode=GetKeyFromKeyQueue ( 1 );
			
#if 0				   
			LeftSeconds--;
#else/*zxg20050113 change*/
			LeftSeconds=CH_GetLeaveTime(StartTime,WaitSeconds);
#endif
		}	
		switch(iKeyScanCode)
		{
		/* Jqz add on 041116(add_main) */
		case LEFT_ARROW_KEY:

			if( CurIputPos == 0 )
				break;

			CurIputPos --;

			TempCharactor = Tempstr[CurIputPos] = 0;
			PinCode[CurIputPos] = 0;

			CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle,TextX+33*CurIputPos,TextY,CHARACTER_WIDTH,CHARACTER_WIDTH*2,&TempCharactor,DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1,1);			
                       CH_UpdateAllOSDView();/*20051215 add*/
			break;
		/* End Jqz add on 041116(add_main) */
		case OK_KEY:
			if(memcmp("0304",(char*)PinCode,4) != 0)/*如果密码错误，不能进入*/
				errorflag=1;
			else 
				errorflag=0;
			ExitLoop=true;
			break;				 
		case EXIT_KEY:
		/*case MENU_KEY:*/
			errorflag=2;	
			ExitLoop=true;
			break;	
		case UP_ARROW_KEY:
		case DOWN_ARROW_KEY:
		case P_UP_KEY:
		case P_DOWN_KEY:
			if(ResponsePKEY==true)/*响应节目切换键*/
			{
				ExitLoop=true;
                errorflag=iKeyScanCode;
			}
			break;
		default:
			if((iKeyScanCode>=KEY_DIGIT0)&&(iKeyScanCode<=KEY_DIGIT9))
			{
				if(CurIputPos>3)
				{
					CurIputPos=0;
					/*恢复密码为空,把要显示的密码字符串恢复为"???????\0"*/
					for(i=0;i<8;i++)
					{
						PinCode[i]='\0';
						Tempstr[i]= 0;
						if(i==7)
							Tempstr[i]='\0';
					}
					/*重新显示*/
					for(i=0;i<4;i++)
					{
						TempCharactor=Tempstr[i];
						CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle,TextX+33*i,TextY,CHARACTER_WIDTH,CHARACTER_WIDTH*2,&TempCharactor,DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1,1);							
					}
				}
				PinCode[CurIputPos]=Convert2Char(iKeyScanCode);
				TempCharactor=Tempstr[CurIputPos]='*';					
				CH6_DisRectTxtInfo(GFXHandle,CH6VPOSD.ViewPortHandle,TextX+33*CurIputPos,TextY,CHARACTER_WIDTH,CHARACTER_WIDTH*2,&TempCharactor,DIALOG_PIN_COLOR,DIALOG_PIN_BORDER_COLOR,1,1);			
				CurIputPos++;
                                CH_UpdateAllOSDView();/*20051215 add*/
				if( CurIputPos > 3 )
				{
					InitKeyQueue();/*yxl 2005-01-20 add this line*/
					PutKeyIntoKeyQueue( OK_KEY );
				}
			}
			break;
		}	 	 	  
		if(LeftSeconds<=0)
		{
			errorflag=2;	
			ExitLoop=true;
		}
	}

	if(DisDeepIndex>0&&Dialog_Save_infomation[--DisDeepIndex]!=NULL)
	{
		CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Dia_X,Dia_Y,DIALOG_WIDTH,DIALOG_HEIGHT,Dialog_Save_infomation[DisDeepIndex]); 
                CH_UpdateAllOSDView();/*20051215 add*/
		memory_deallocate(CHSysPartition,Dialog_Save_infomation[DisDeepIndex]);	   
		Dialog_Save_infomation[DisDeepIndex]=NULL;
	}
	return errorflag;	   
}

/*fj 20080304 add pincode to write stbid into e2p*/
int  CH6_InputPinCode_stbid(boolean ReponseProKey)
{
	
	int errorflag;
	int times=3;
	while(times>0)
	{
		if(times==3)
			errorflag=CH6_PIN_PopupDialog_stbid  ((pstBoxInfo->abiLanguageInUse==0)?"Please input\nthe password":"Please input\nthe password",20,ReponseProKey);
		else
			errorflag=CH6_PIN_PopupDialog_stbid  ((pstBoxInfo->abiLanguageInUse==0)?"Wrong password,please input   \nagain          ":"Wrong password,please input   \nagain          ",20,ReponseProKey);
		
		if(errorflag==2)
		{
			return 1;
		}	
		else if(errorflag==0)
		{
			return 0;
		}
		else if(errorflag==1)
		{
			
		}
		else
		{
			return errorflag;
		}
		times--;
	}
	return 1;
}
#endif



