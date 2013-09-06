/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_msg.h
  * 描述: 	定义消息相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-11-06
  * ===================================================================================
  */

#ifndef __EIS_API_MSG__
#define __EIS_API_MSG__

/*
  * Type: eis_msg_t
  * Desc: irdeto 消息类型
  */
typedef struct
{
	U32		mu32_MsgType;
	U32		mu32_Param1;
	U32		mu32_Param2;
}eis_msg_t;

#define EIS_MSG_SIZE	sizeof ( eis_msg_t )		/* 单个消息大小 */
#define EIS_MSG_COUNT	50						/* 最大消息数量 */

/*
  * Func: eis_api_msg_init
  * Desc: eis消息队列初始化
  * In:	n/a
  * Out:	n/a
  * Ret:	IPANEL_OK	-> 成功
  		IPANEL_ERR	-> 不成功
  */
extern int eis_api_msg_init ( void );

/*
  * Func: eis_api_msg_reset
  * Desc: eis消息队列初始化
  * In:	n/a
  * Out:	n/a
  * Ret:	IPANEL_OK	-> 成功
  		IPANEL_ERR	-> 不成功
  */
extern void eis_api_msg_reset ( void );

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
extern int eis_api_msg_send ( U32 ru32_MsgType, U32 ri_Param1, U32 ri_Param2 );

/*
  * Func: eis_api_msg_send
  * Desc: 在主进程中接收消息
  * Out:	rp_MsgType	-> 带出的消息类型
  		rp_Param1	-> 带出的参数1
  		rp_Param2	-> 带出的参数2
  * Ret:	IPANEL_OK	-> 有消息
  		IPANEL_ERR	-> 无消息或出错
  */
extern int eis_api_msg_get ( U32 *rp_MsgType, U32 *rp_Param1, U32 *rp_Param2 );

#endif

/*--eof----------------------------------------------------------------------------------------------------*/

