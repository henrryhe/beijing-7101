/*******************文件说明注释************************************/
/*公司版权说明：版权（2006）归长虹网络科技有限公司所有。           */
/*文件名：CH_watchdog.c                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2006-08-08                                             */
/*模块功能:软件看门狗复位                      */
/*主要函数及功能:                                                  */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */

/*******************************************************************/

/**************** #include 部分*************************************/
#include "appltype.h"
#include "..\main\initterm.h"
#include "stddefs.h"
#include "stcommon.h"
#include "string.h"
#include "stsys.h"	
#include "..\report\report.h"

void CH_EnableWatchDog(void);
/*******************************************************************/
/**************** #define 部分**************************************/

#define SYstemBaseAddr                 0x20f00000
#define WATCHDOG_CFG0_OFFSET  0x130
#define WATCHDOG_CFG1_OFFSET  0x134
#define WATCHDOG_RESET_OFFSET 0x140
#define REGISTER_LOCK_OFFSET      0x300
 /*[31:5]reserved,[4]WD_COUNT_EN:watchdog counter enable [3:0] the value and WATCHDOG_CFG0 value combine to maximum 1048576 seconds*/


typedef volatile unsigned int system_device;




/*******************************************************************/

/***************  全局变量定义部分 *********************************/
semaphore_t *pWatchAlamSema;



typedef void (*APP_SetEntry)(void);
/******************************************************************/
/*函数名:void CH_LongJmp(unsigned long int rAdress)               */
/*开发人和开发时间:zengxianggen 2007-05-15                        */
/*函数功能描述:跳转到指定程序位置                                 */
/*函数原理说明:                                                   */
/*输入参数：无                                                    */
/*输出参数: 无                                                    */
/*使用的全局变量、表和结构：                                      */
/*调用的主要函数列表：                                            */
/*返回值说明：无                               */   
/*调用注意事项:                                                   */
/*其他说明:                                                       */    
void CH_LongJmp(unsigned long int rAdress)
{
#if 0
APP_SetEntry AppJmp;
AppJmp=rAdress;
AppJmp();
#else/*通过非法操作实现看门狗重新启动*/
int *pi_Error=NULL;
*pi_Error=0xFFFF;
#endif
}

void CH_WatchSemaInit(void)
{
  	pWatchAlamSema=semaphore_create_fifo( 1);	
}
/*******************************************************************/
/************************函数说明***********************************/
/*函数名:void CH_SetWatchDog(void)            */
/*开发人和开发时间:zengxianggen 2006-08-08                         */
/*函数功能描述:设置看门狗参数                                      */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_SetWatchDog(void)
{
	semaphore_wait(pWatchAlamSema);


	*(volatile unsigned long*) (SYstemBaseAddr+REGISTER_LOCK_OFFSET)=0x0F;
	*(volatile unsigned long*) (SYstemBaseAddr+REGISTER_LOCK_OFFSET)=0xF0;
	  
	*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG0_OFFSET) = 0x0f/*15秒*/;
	*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG1_OFFSET) = 0x00;
	
	CH_EnableWatchDog();/*打开看门狗功能*/
	semaphore_signal(pWatchAlamSema);
	return ;

}

/*函数名:void CH_EnableWatchDog(void)            */
/*开发人和开发时间:zengxianggen 2006-08-08                         */
/*函数功能描述:打开看门狗功能                                        */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
void CH_EnableWatchDog(void)
{
      volatile unsigned long ResetSetting;
	*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG1_OFFSET)= 0x10;
#if 0	
	/*读取当前的RESET设置*/
	ResetSetting=*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_RESET_OFFSET);
	/*设置看门狗功能*/
	ResetSetting=ResetSetting|0x01;
#endif	
	*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_RESET_OFFSET)= 0x181;
	return ;

}


void CH_ReadWatchDog(void)
{
	#if 1
	volatile unsigned long data;
	data = *(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG0_OFFSET);
	do_report(0,"data0=%x \n",data);
	data = *(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG1_OFFSET);
	do_report(0,"data1=%x \n",data);
	data = *(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_RESET_OFFSET);
	do_report(0,"data2=%x \n",data);
	
	return;
	#endif
}


/*函数名:static void WatchDogProcess(void *pvParam)        */
/*开发人和开发时间:zengxianggen 2006-08-08                         */
/*函数功能描述:看门狗监控进程                                   */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
static void WatchDogProcess(void *pvParam)
{
	
	
	while(true)
	{
		CH_SetWatchDog();
		task_delay(ST_GetClocksPerSecond());
		CH_ReadWatchDog();  
	}
}
/*函数名:BOOL CH_WatchDogInit(void)                               */
/*开发人和开发时间:zengxianggen 2006-08-08                         */
/*函数功能描述:看门狗监控进程                                      */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */
BOOL CH_WatchDogInit(void)
{
	task_t *ptidWatchDogTask;
	if ((ptidWatchDogTask = Task_Create(WatchDogProcess,NULL,512 ,5,
		"WatchDogTask",0))==NULL)
    {
		
		STTBX_Print(("Failed to start WatchDog task\n"));
		return TRUE;
    }
	else 
	{
		STTBX_Print(("creat WatchDog task successfully \n"));
		return FALSE;
	}

}


/*函数名:void CH_Restart(void)                               */
/*开发人和开发时间:zengxianggen 2006-08-08                         */
/*函数功能描述:重新启动                                      */
/*函数原理说明:                                                    */
/*使用的全局变量、表和结构：                                       */
/*调用的主要函数列表：                                             */
/*返回值说明：无                                                   */

void CH_Restart(void)   
{   
#if 0

     task_lock();
	interrupt_lock();
	semaphore_wait(pWatchAlamSema);

     
	*(volatile unsigned long*) (SYstemBaseAddr+REGISTER_LOCK_OFFSET)=0x0F;
	*(volatile unsigned long*) (SYstemBaseAddr+REGISTER_LOCK_OFFSET)=0xF0;

	*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG0_OFFSET) = 0x02/*2秒*/;
	*(volatile unsigned long*) (SYstemBaseAddr+WATCHDOG_CFG1_OFFSET) = 0x00;
	CH_EnableWatchDog();/*打开看门狗功能*/
	
	semaphore_signal(pWatchAlamSema);

	task_delay(10000);
	
	interrupt_unlock();
	task_unlock();
#else
   
   /* kernel_timeslice(OS21_FALSE);
    task_priority_set(NULL, MAX_USER_PRIORITY);
    task_lock();
    interrupt_lock();*/
    while(true)
       CH_LongJmp(0xA0000000);
  /*  interrupt_unlock();
    task_unlock();*/
#endif

}
/*++++++++++++++++++++++++++++++++++++++*/





