/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_globe.c
  * 描述: 	定义全局变量
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */
#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_msg.h"

clock_t 	geis_reboot_timeout 	= 0;		/* 重启时间 */
VOID *	pEisHandle 				= NULL;	/* ipanel浏览器句柄 */
boolean 	log_out_state 	= false;		/* 退出标志 */

semaphore_t *gp_EisSema = NULL; 

static boolean eis_dhcp_set =false;/*  用于记录IPANLE 模块设置DPCH状态true:set DHCP , false:DHCP end */
void eis_set_dhcp_state( boolean dhcp_state)
{
	eis_dhcp_set = dhcp_state;
}
boolean eis_get_dhcp_state( void )
{
	return eis_dhcp_set;
}

/*  初始化用到 */
void CH_Ipanel_demux_Init ( void )
{
      CH_Eispartition_init_VIDLMI();
       gp_EisSema = semaphore_create_fifo(1);
	ipanel_porting_task_init (); /*初始化TASK相关参数*/
	Eis_Init (); /*启动收数据进程*/
	eis_api_msg_init (); /*初始化IPANEL消息队列*/
	ipanel_porting_init_nvm(); /*启动FLASH操作进程*/
	EIS_time_paly_init();/*启动待机时时间闪烁进程*/
	EIS_Msg_process_init();/*启动消息监控进程*/
}

/* 每次进入用到 */
int eis_api_enter_init ( void )
{
	int i_Return;
#ifdef USE_EIS_2D 
	ipanel_2d_init();
#endif
	eis_api_stop_ca ();		/* 停止CA的接收 */
	/*eis_api_msg_reset ();      刘占芬20101029 去掉*/
	eis_init_region ();
	eis_clear_all ();
	eis_timeinit ();
	CH_ipanel_network_init();
	Eis_DMX_Init();

	return 0;
}

/* 退出时用到 */
void eis_api_out_term ( void )
{
	CH_ipanel_network_exit();
	ipanel_porting_term_nvm ( );
	eis_del_region ();
	eis_clear_all ();
	eis_api_msg_reset ();
	Eis_DMX_term();
}


/*--eof-----------------------------------------------------------------------------------------------------*/

