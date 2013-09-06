/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_msg.c
  * 描述: 	实现消息相关的接口
  * 作者:	刘战芬，蒋庆洲
  * 时间:	2008-11-06
  * ===================================================================================
  */
#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "eis_api_msg.h"

static message_queue_t	*gp_eis_msg_queue = NULL;

/*
  * Func: eis_api_msg_init
  * Desc: eis消息队列初始化
  * In:	n/a
  * Out:	n/a
  * Ret:	IPANEL_OK	-> 成功
  		IPANEL_ERR	-> 不成功
  */
int eis_api_msg_init ( void )
{
	gp_eis_msg_queue = message_create_queue_timeout ( EIS_MSG_SIZE, EIS_MSG_COUNT );
	if ( NULL == gp_eis_msg_queue )
	{
		eis_report ( "\n++>eis fatal msg_queue create fail!!!!" );
		return IPANEL_ERR;
	}

	return IPANEL_OK;
}

/*
  * Func: eis_api_msg_reset
  * Desc: eis消息队列初始化
  * In:	n/a
  * Out:	n/a
  * Ret:	IPANEL_OK	-> 成功
  		IPANEL_ERR	-> 不成功
  */
void eis_api_msg_reset ( void )
{
	eis_msg_t *pt_msg;

	if ( NULL == gp_eis_msg_queue )
		return;

	do
	{
		pt_msg = message_receive_timeout ( gp_eis_msg_queue, TIMEOUT_IMMEDIATE );

		if ( NULL != pt_msg )
		{
			message_release ( gp_eis_msg_queue, pt_msg );
		}
	}while ( pt_msg );

	return;
}

/*
  * Func: eis_api_msg_send
  * Desc: 向主进程发送消息
  * In:	ru32_MsgType	-> 消息类型
  		ri_Param1	-> 消息参数1
  		ri_Param2	-> 消息参数2
  * Out:	n/a
  * Ret:	IPANEL_OK	-> 成功
  		IPANEL_ERR	-> 不成功
  */
int eis_api_msg_send ( U32 ru32_MsgType, U32 ri_Param1, U32 ri_Param2 )
{
	clock_t 		time_out;
	eis_msg_t 	*p_Msg = NULL;

	if ( NULL == gp_eis_msg_queue )
	{
		return IPANEL_ERR;
	}

	time_out 	= time_plus ( time_now(), ST_GetClocksPerSecondLow() * 10 );

	p_Msg	= (eis_msg_t*)message_claim_timeout ( gp_eis_msg_queue, &time_out );

	if ( NULL == p_Msg )
	{
		eis_report ( "\n++>eis msg allocate fail!!!!" );
		return IPANEL_ERR;
	}

	p_Msg->mu32_MsgType 	= ru32_MsgType;
	p_Msg->mu32_Param1		= ri_Param1;
	p_Msg->mu32_Param2		= ri_Param2;

	message_send ( gp_eis_msg_queue, p_Msg );

	return IPANEL_OK;
}

/*
  * Func: eis_api_msg_send
  * Desc: 在主进程中接收消息
  * Out:	rp_MsgType	-> 带出的消息类型
  		rp_Param1	-> 带出的参数1
  		rp_Param2	-> 带出的参数2
  * Ret:	IPANEL_OK	-> 有消息
  		IPANEL_ERR	-> 无消息或出错
  */
int eis_api_msg_get ( U32 *rp_MsgType, U32 *rp_Param1, U32 *rp_Param2 )
{
	eis_msg_t *pt_msg;

	if ( NULL == gp_eis_msg_queue )
	{
		return IPANEL_ERR;
	}

	pt_msg = message_receive_timeout ( gp_eis_msg_queue, TIMEOUT_IMMEDIATE );

	if ( NULL == pt_msg )
	{
		return IPANEL_ERR;
	}

	*rp_MsgType 	= pt_msg->mu32_MsgType;
	*rp_Param1		= pt_msg->mu32_Param1;
	*rp_Param2		= pt_msg->mu32_Param2;
	message_release ( gp_eis_msg_queue, pt_msg );
	return IPANEL_OK;
}

/*--eof-----------------------------------------------------------------------------------------------------*/

