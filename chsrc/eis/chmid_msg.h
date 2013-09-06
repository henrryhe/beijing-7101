/*
  * ==========================================================================================
  * Copyright By CHANGHONG network
  * file: chmid_msg.h
  * des: 定义消息的结构及相关函数
  * -------DATE---------WORK--------NAME---------
  *             081110              创建		蒋庆洲
  * ==========================================================================================
  */

#ifndef __CHMID_MSG_H__
#define  __CHMID_MSG_H__

/* 定义消息最大优先级个数 */
#define CHMID_MSG_PRIORITY_NUMBER	16

/* 定义消息中间层的错误类型 */
typedef  enum
{
	CHMID_MSG_OK			= 0,		 	/* 正确 */
	CHMID_MSG_NO_MEM,				/* 无内存 */
	CHMID_MSG_PARAM,					/* 参数不正确 */
	CHMID_MSG_REPEAT,					/* 重复消息 */
	CHMID_MSG_UNKNOW_ERROR			/* 其他错误 */
}chmid_msg_error_e;

/* 定义消息的类型 */
typedef enum
{
}chmid_msg_type_e;

/* 定义一个消息的结构 */
typedef struct chmid_msg_node_s
{
	chmid_msg_type_e			me_MsgType;		/* 消息类型 				*/
	void						*mp_MsgParam;		/* 消息参数指针 		*/
	struct chmid_msg_node_s	*mp_NextMsg;		/* 下一个消息的指针 */
}chmid_msg_node_t;

/*  定义一个消息队列的结构 */
typedef struct
{
	int					mi_MsgCount;		/* 该队列消息结构 		*/
	chmid_msg_node_t		*mp_QueueHead;	/* 队列指针 					*/
	chmid_msg_node_t		*mp_QueueTail;		/* 队列尾指针 				*/
}chmid_msg_queue_t;

/* 定义跨平台的bool函数 */
typedef enum
{
	CHMID_MSG_FALSE = 0,
	CHMID_MSG_TRUE
}chmid_msg_bool_e;

/* 定义msg中间层消息队列全局数组 */
static chmid_msg_queue_t 	gt_msg_mid_queue[CHMID_MSG_PRIORITY_NUMBER];
static ST_Partition_t		*gp_MsgPartition 	= NULL;
static semaphore_t			*gp_MsgSemaphore 	= NULL;

/*
  * Func: CHMID_MSG_Init
  * Desc: 初始化消息中间层
  * In:	所用分区指针
  * Out:	n/a
  * Ret:	CHMID_MSG_PARAM	-> 传入的内存分区指针不对
  		CHMID_MSG_NO_MEM	-> 创建互斥信号量失败
  		CHMID_MSG_OK		-> 初始化成功
  */
chmid_msg_error_e CHMID_MSG_Init ( void* rp_Partition )
{
	int i;

	/* 判断参数是否正确 */
	if ( NULL == rp_Partition )
	{
		return CHMID_MSG_PARAM;
	}

	/* 创建互斥用信号量 */
	gp_MsgSemaphore = semaphore_create_fifo ( 1 );

	if ( NULL == gp_MsgSemaphore )
	{
		return CHMID_MSG_NO_MEM;
	}

	gp_MsgPartition = ( ST_Partition_t* )rp_Partition;

	/* 循环初始化消息队列数组 */
	for ( i = 0; i < CHMID_MSG_PRIORITY_NUMBER; i ++ )
	{
		gt_msg_mid_queue[i].mi_MsgCount 	= 0;
		gt_msg_mid_queue[i].mp_QueueHead	= NULL;
		gt_msg_mid_queue[i].mp_QueueTail	= NULL;
	}

	/* 正确返回 */
	return CHMID_MSG_OK;
}

/*
  * Func: CHMID_MSG_Put
  * Desc: 用消息中间层发送消息
  * In:	re_MsgType			-> 消息类型
  		rc_MsgPriority			-> 消息优先级
  							     0~15
  							     0 级最低
  							     15级最高
  		rp_MsgParam			-> 消息参数指针
  		re_CheckRepeat		-> 是否检查重复消息
  							     带参数的消息最好不要检测重复
  * Out:	n/a
  * Ret:	CHMID_MSG_REPEAT	-> 传入的消息已经在消息队列中了
  		CHMID_MSG_NO_MEM	-> 创建互斥信号量失败
  		CHMID_MSG_OK		-> 初始化成功
  */
chmid_msg_error_e CHMID_MSG_Send ( chmid_msg_type_e re_MsgType, char rc_MsgPriority, void* rp_MsgParam, chmid_msg_bool_e re_CheckRepeat )
{
	int 					i_MsgQueueIndex;	/* 消息队列序号 */
	int					i_Index;
	chmid_msg_node_t		*pt_MsgNode;

	semaphore_wait ( gp_MsgSemaphore );

	/* 检查优先级 */
	if ( rc_MsgPriority > CHMID_MSG_PRIORITY_NUMBER - 1 )
	{
		rc_MsgPriority = CHMID_MSG_PRIORITY_NUMBER - 1;
	}

	if (  rc_MsgPriority < 0 )
	{
		rc_MsgPriority = 0;
	}

	/* 得到消息队列的索引 */
	i_MsgQueueIndex = CHMID_MSG_PRIORITY_NUMBER - 1 - rc_MsgPriority;

	if ( CHMID_MSG_TRUE == re_CheckRepeat )
	{
		pt_MsgNode 	= gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueHead;
		i_Index		= gt_msg_mid_queue[i_MsgQueueIndex].mi_MsgCount;

		/* 判断是否重复 */
		while ( i_Index > 0 && NULL != pt_MsgNode )
		{
			if ( NULL != pt_MsgNode && pt_MsgNode->me_MsgType == re_MsgType )
			{
				semaphore_signal ( gp_MsgSemaphore );
				return CHMID_MSG_REPEAT;
			}
		}
	}
	
	/* 往里面添加该信息 */
	pt_MsgNode = memory_allocate ( gp_MsgPartition, sizeof ( chmid_msg_node_t ) );

	if ( NULL == pt_MsgNode )
	{
		semaphore_signal ( gp_MsgSemaphore );
		return CHMID_MSG_NO_MEM;		
	}

	pt_MsgNode->me_MsgType 		= re_MsgType;
	pt_MsgNode->mp_MsgParam	= rp_MsgParam;
	pt_MsgNode->mp_NextMsg		= NULL;
	if ( NULL == gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueHead )
	{
		gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueHead = pt_MsgNode;
	}

	if ( NULL == gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueTail )
	{
		gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueTail = pt_MsgNode;
	}
	else
	{
		gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueTail->mp_NextMsg = pt_MsgNode;
		gt_msg_mid_queue[i_MsgQueueIndex].mp_QueueTail = pt_MsgNode;
	}

	semaphore_signal ( gp_MsgSemaphore );
	return CHMID_MSG_OK;
}

#endif

/*--eof--------------------------------------------------------------------------------------------------------------*/

