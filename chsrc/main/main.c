/*
	(c) Copyright changhong
  Name:main.c
  Description:main entry  section for changhong sti7100 platform
  Authors:yxl
  Date          Remark
  2006-09-04    Created

 Modifiction:

*/
#include <stdio.h> 
#include <string.h>
#include <stdarg.h>

#include "stddefs.h"  
#include "sttbx.h" 
#include "..\symbol\symbol.h"
#include "initterm.h"
#include "..\usif\graphicmid.h"
#include "..\dbase\vdbase.h"
#include "chmid_tcpip_api.h"
int   DVB_ApplicationInit ( void );


/*{{{ queue size definition*/
#define  USIF_MSG_QUEUE_SIZE                            20
#define  USIF_MSG_QUEUE_NO_OF_MESSAGES      50


/*
Name :       main
Description: Main entry point
Parameters:  command line arguments
 */
/*#define CH6_MODULE_TEST*/
#define INTERCONNECT_BASE                (U32)0xB9001000
#define SYS_CONFIG_7                           0x11C
#define SYSTEM_CONFIG_7                    (INTERCONNECT_BASE + SYS_CONFIG_7)

void CH_CVBS_Setting(void);
void CH_YPbPr_Setting(void);


extern  boolean CH_tkd_init(void);

/*zxg20090623 增加LMI VID支持128 MBYE*/
void CH_SettingLMIVID128M(void)
{
  	U32 ui_oldconfig11;	
	ui_oldconfig11 = STSYS_ReadRegDev32LE(INTERCONNECT_BASE + 0x012c);
	ui_oldconfig11 = ui_oldconfig11|0x80000000;
	STSYS_WriteRegDev32LE(INTERCONNECT_BASE + 0x012c,ui_oldconfig11  );
}

int main(int argc, char *argv[])
{

	BOOL res=FALSE;
	 STSYS_WriteRegDev32LE(SYSTEM_CONFIG_7,0x003001B0  );/*zxg 20070417 change*/
        CH_SettingLMIVID128M();

		{
		 extern unsigned int bsp_xtal_frequency_hz;
	          bsp_xtal_frequency_hz=30*1000*1000;
		}
	res=GlobalInit();
	CH_CVBS_Setting();

	STTBX_Print(("\n==============================================\n"));

	CH_YPbPr_Setting();
  
#if  1
	DVB_ApplicationInit();
#endif

	task_priority_set(NULL, 0);

	if (res==FALSE )
	{
		STTBX_Print(("\n==========CHANGHONG===================\n"));
		STTBX_Print(("      Built on %s at %s\n", __DATE__, __TIME__  ));
		STTBX_Print(("\tChip STi7101\n"));
		STTBX_Print(("         CPU Clock Speed = %d\n", ST_GetClockSpeed() ));
		STTBX_Print(("==============================================\n\n"));

	}
	#if 1
	while(true)
	{
	   task_delay( ST_GetClocksPerSecond()*10);/*20070523 add*/
	}
      #endif
	return 0;
}




/*{{{ queue size definition*/
#define  USIF_MSG_QUEUE_SIZE                20
#define  USIF_MSG_QUEUE_NO_OF_MESSAGES      50

#define  DBASE_MSG_QUEUE_SIZE               20
#define  DBASE_MSG_QUEUE_NO_OF_MESSAGES     50


#define  MULAUDIO_MSG_QUEUE_SIZE            20
#define  MULAUDIO_MSG_QUEUE_NO_OF_MESSAGES  20   /*  50 */

#define  EITINFO_MSG_QUEUE_SIZE             20
#define  EITINFO_MSG_QUEUE_NO_OF_MESSAGES   20   /* 50 */


#define NVOD_MSG_QUEUE_SIZE                 20
#define NVOD_MSG_QUEUE_NO_OF_MESSAGES       20



/*yxl 2005-11-10 add this function*/
/*Discription:
完成应用程序使用的各消息队列的创建*/
BOOL CH_MessageInit(void)
{
	message_queue_t  *pstMessageQueue = NULL;
	symbol_init();
	
	STTBX_Print ( ( "Creating Application Message queues...\n" ) );
	if ( ( pstMessageQueue = message_create_queue_timeout ( USIF_MSG_QUEUE_SIZE,
		USIF_MSG_QUEUE_NO_OF_MESSAGES ) ) == NULL )
	{
		STTBX_Print(("Unable to create USIF message queue\n"));
		return TRUE;
	}
	else
	{
		symbol_register ( "usif_queue", ( void * ) pstMessageQueue );
		
	}
	
	if ( ( pstMessageQueue = message_create_queue_timeout ( DBASE_MSG_QUEUE_SIZE,
		DBASE_MSG_QUEUE_NO_OF_MESSAGES ) ) == NULL )
	{
		STTBX_Print(("Unable to create DBASE message queue\n"));
		return TRUE;
	}
	else
	{
		symbol_register ( "dbase_queue", ( void * ) pstMessageQueue );
	}
	
	if ( ( pstMessageQueue  = message_create_queue_timeout ( EITINFO_MSG_QUEUE_SIZE,
		EITINFO_MSG_QUEUE_NO_OF_MESSAGES ) ) == NULL )
	{
		STTBX_Print(("%s  %d Unable to create EITINFO message queue\n"));
		return TRUE;
	}
	else
	{
		symbol_register ( "eitinfo_queue", (void*) pstMessageQueue );
	}

	return FALSE;
}
/*调整系统优先级统一到0~15*/
void CH_AdjustTaskPri(void)
{
    int           pri, count;
    task_t*       task;
   
    for (task = /*task_list_head()*/task_list_next(NULL), count = 0;
         task != NULL;
         task = task_list_next(task), count++ )
    {
#if 0
		 pri = task_priority(task);
		 if(pri > 15)
		{
			pri = pri +15 -255;
			task_priority_set(task,pri);
		}
#endif
		do_report( 0,"\n %s %d \n",task_name(task),task_priority(task));
    } 
}                                                                              

/*end yxl 2005-11-10 add this function*/
int   DVB_ApplicationInit ( void )
{
	message_queue_t  *pstMessageQueue = NULL;
     int res;
#if 0
{
			FILE *fp;
			U8*  p_TempBuf = NULL;

			fp=fopen("F:\\DVB-C8000BI_(CH-01-05)_(CS-01-08)_EEPROM.bin","rb");
			if(fp==NULL)
			{
				return;
			}

			p_TempBuf = memory_allocate(CHSysPartition,8*1024);

			if(p_TempBuf != NULL)
			{
				memset(p_TempBuf,0,8*1024);
				fread(p_TempBuf,1,7*1024,fp);
				WriteNvmData(0,7*1024,p_TempBuf);
			}
			
			fclose(fp);

			return;

		}
#endif



	if (	( pstMessageQueue = message_create_queue_timeout ( DBASE_MSG_QUEUE_SIZE,
				 DBASE_MSG_QUEUE_NO_OF_MESSAGES	) ) == NULL )
   	{
      		STTBX_Report( ( STTBX_REPORT_LEVEL_ERROR,	"Unable to create DBASE message queue\n" ) );
   	}
   	else
   	{
		symbol_register ("av_queue", ( void * ) pstMessageQueue );
   	}
	

	if(CH6_SectionInit())
	{
		STTBX_Print(("Failed to initialise SFILTER ..." ));
		return   TRUE;	
	}

	if (DVBDbaseInit())
	{
		STTBX_Print(( "Failed to initialise DBASE ..." ));
		return   TRUE;
	}

	if( DVBKeyboardInit())
	{
		STTBX_Print(("Failed to initialise KEYBOARD PROCESS ..." ) );
		return   TRUE;
	}
 /*20070523 add*/
	if(AFrontKeybordInit())
	{
		STTBX_Print(("Failed to initialise FRONT KEYBOARD PROCESS ..." ) );
		return   TRUE;
	}
#if 1
        CH_USB_Init();
#endif

         CH_CheckForEnterProMenu();/* zq add 开机控制进入生产模式*/
  #ifdef SUMA_SECURITY
  	SumaSTBSecure_GetChipIDPrevate();
  #endif

#ifdef  NAFR_KERNEL
	{
       
		extern BOOL  CHCA_SCardHwInit(void);      

		CH_MGCA_IPanel_Init();
	        CH_tkd_init();
		res = CHCA_SCardHwInit();
		if(res==TRUE)
			return res;
	}	  
#endif


#if 1
        CHMID_TCPIP_Init();//
        CHMID_CM_Init();
#endif     



#ifdef USE_IPANEL
	CH_Ipanel_demux_Init();
#endif

#ifdef SUMA_SECURITY
	CH_SumaSecurity_init();
#endif

	if ( DVBUsifInit())
	{
		STTBX_Print(("Failed to initialise USIF ..." ) );
		return TRUE;
	}
#if 0    //
        CH_AdjustTaskPri();
#endif  //<!-- ZQiang 2010/12/23 14:25:50 --!>	

	return   FALSE;
}




void CH_CVBS_Setting(void)
{
#define BASE_ADDRESS  0xB920C000
#define CFG3_ADDRESS  0x0C
#define CFG9_ADDRESS  0x144
#define CM_ADDRESS    0x104
#define DAC6_ADDRESS  0x1AC

#define DAC13_ADDRESS  0x44
#define DAC34_ADDRESS  0x48
#define DAC45_ADDRESS  0x4C
#define DAC2_ADDRESS   0x1A8
#define CFG13_ADDRESS  0x17C 

U32 ui_CFG3 = 0x48;
U32 ui_CFG9 = 0xD5;
U32 ui_DC6 =  0x13;
U32 ui_CM =    0x31;

STSYS_WriteRegDev8((void*)(BASE_ADDRESS+CFG3_ADDRESS),ui_CFG3);
STSYS_WriteRegDev8((void*)(BASE_ADDRESS+CFG9_ADDRESS),ui_CFG9);
STSYS_WriteRegDev8((void*)(BASE_ADDRESS+DAC6_ADDRESS),ui_DC6);
STSYS_WriteRegDev8((void*)(BASE_ADDRESS+CM_ADDRESS),ui_CM);


}
void CH_YPbPr_Setting(void)
{

  	STSYS_WriteRegDev32BE(0xb90050B4, 0x00);/*709/601设置*/
	
	STSYS_WriteRegMem32LE(0xb90050A0, 0x00e000e0);/*o level*/
	STSYS_WriteRegMem32LE(0xb90050A8, 0x01c001c0);/*High level*/

	STSYS_WriteRegMem32LE(0xb90050B8, 0x252);      /*Y luma multi factor*/
	STSYS_WriteRegMem32LE(0xb90050C4, 0xbd);        /*Y luma offset*/


       STSYS_WriteRegMem32LE(0xb90050BC, 0x235);       /*Pr luma multi factor*/
	STSYS_WriteRegMem32LE(0xb90050C0, 0x1F0);        /*Pr luma offset*/
}



