/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_os.h
  * 描述: 	定义OS相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#ifndef __EIS_API_OS__
#define __EIS_API_OS__

#define EIS_MAX_TASK_NUM	50		/* 最多可申请50个进程 */
/*
  * struct: eis_task_t
  * desc:  定义eis的进程结构
  */
typedef struct eis_task_s
{
	U32		mu32_TaskID;			/* 进程指针地址 */
	U32		mu8_TaskPriority;		/* 进程优先级 */
	U8		*mp_TaskStack;			/* 进程堆栈指针 */
	tdesc_t	mt_TaskDesc;			/* 进程描述 */
}eis_task_t;

/*
功能说明：
	请求分配指定大小的内存块。内存分配函数， iPanel MiddleWare会若干次调用该
	函数。iPanel MiddleWare 要求每次调用该函数系统分配的内存空间必须是连续的整块
	内存。
参数说明：
	输入参数：
		memsize：请求的内存大小，以字节为单位。大于0有效，小于等于0无效。
	输出参数：无
	返 回：
		!=IPANEL_NULL: 成功，返回有效内存块的起始地址；
		==IPANEL_NULL，失败，返回的是空指针。
*/
extern VOID *ipanel_porting_malloc(INT32_T memsize);

/*
功能说明：
	释放ptr 指定的内存空间，并且该内存空间必须是由ipanel_porting_malloc 分配的。
参数说明：
	输入参数：
		ptr ：指向要释放的内存的指针。
	输出参数：无
*/
extern VOID ipanel_porting_free(VOID *ptr);

/*
功能说明：
	浏览器计时器的初始化
注意:
	必须在每次浏览器开始时调用
  */
extern void eis_timeinit(void);

/*
功能说明：
	iPanel Middleware 调用此函数获取当前时间和STB 启动时间之间经过的毫秒数。
	ipanel现在还没有处理返回时间的溢出问题，如果出现了数据溢出，会导致意想不到的
	后果，这样在理论上iPanel Middleware的正常工作时间最大值是49天。如果此函数所
	获得的时间是由其他元素计算得到，同样要注意溢出问题。另外，实现者必须保证通过
	ipanel_porting_time_ms()获得的时间要有连续性，精确性，在实际的移植过程中，很
	多问题都出现在这个函数的实现上。
注意：该函数的使用频率很高，请确保其执行效率尽可能高效，返回值准确，实现
	者的时钟精度最好是小于1 毫秒。
参数说明：
  */
extern UINT32_T ipanel_porting_time_ms(VOID);

/*
功能说明：
	命令系统在多少秒钟后重新启动。
参数说明：
	输入参数：
		s：多少秒后重新启动，s 单位为秒。s＝0，表示立即重启系统。
		输出参数：无
	返 回：
		IPANEL_OK: 成功，
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_system_reboot(INT32_T s);

/*
功能说明：
	在主进程中检查是否要重启
参数说明：
	输入参数： 无
	输出参数：无
	返 回： 无
  */
extern void ipanel_porting_check_reboot ( void );

/*
功能说明：
	告诉系统进入待机模式，并在指定时间后（以秒为单位）恢复正常工作模式，ipanel
	Middleware 对系统是否使用真待机没有要求，客户根据自己产品的情况决定如何处
	理。
参数说明：
	输入参数：
		s：待机s 秒后恢复正常模式，IPANEL_WAIT_FOREVER(-1)为永久待机
	输出参数：无
	返 回：
		IPANEL_OK : 成功，
		IPANEL_ERR : 失败。
*/
extern INT32_T ipanel_porting_system_standby(INT32_T s);


/*
功能说明：
	用于销毁一个线程。
参数说明：
	输入参数：
		handle：线程句柄(非0 且存在，有效)。
	输出参数：无；
	返 回：
		IPANEL_OK：成功，
		IPANEL_ERR：失败
*/
extern INT32_T ipanel_porting_task_destroy(UINT32_T handle);

/*
功能说明：
	将当前线程挂起指定时间，同时让出CPU 供其他线程使用。
参数说明：
	输入参数：
		ms：挂起线程的时间长度，单位为毫秒。
	输出参数：无；
	返 回：无
*/
extern VOID ipanel_porting_task_sleep(INT32_T ms);

/*
功能说明：
	创建一个信号量，iPanel MiddleWare 只使用互斥信号量，不使用计数信号量。
参数说明：
	输入参数：
		name：一个最多四字符长字符串，系统中信号量的名字应该唯一；
		initial_count：最大初始化计数(只有0 和1 有效)
		wait_mode：这个参数决定当信号量有效时，等待此信号量的线程获得满足
		的次序，有两个选项：
		– IPANEL_TASK_WAIT_FIFO – 按先入先出的方式在等待线程中分发
		消息
		– IPANEL_TASK_WAIT_PRIO – 优先满足高优先级的线程。
	输出参数：无
	返 回：
		！＝IPANEL_NULL： 成功，信号量句柄。
		＝＝IPANEL_NULL ：失败。
*/
extern UINT32_T ipanel_porting_sem_create(const CHAR_T*name, INT32_T initial_count, UINT32_T wait_mode);

/*
功能说明：
	销毁一个信号量。
参数说明：
	输入参数：
		handle：信号量句柄，由ipanel_porting_sem_create 获得。
	输出参数：无
	返 回：
		IPANEL_OK：成功；
		IPANEL_ERR：失败。
*/
extern INT32_T ipanel_porting_sem_destroy(UINT32_T handle);


/*
功能说明：
	信号量等待。
参数说明：
	输入参数：
		handle：信号量句柄，由ipanel_porting_sem_create 获得。
		wait_time：等待时间，单位为毫秒。为IPANEL_NO_WAIT(0)时表示不等待立
		即返回，为IPANEL_WAIT_FOREVER(-1)表示永久等待。
		输出参数：无
	返 回：
		IPANEL_OK：表示等待到信号；
		IPANEL_ERR：表示没有等到信号,或异常中止。
*/
extern INT32_T ipanel_porting_sem_wait(UINT32_T handle,INT32_T wait_time);

/*
功能说明：
	释放信号量。
参数说明：
	输入参数：
		handle：信号量句柄，由ipanel_porting_sem_create 获得。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_sem_release(UINT32_T handle);

/*
功能说明：
	创建一个消息队列。每个消息结构定义为IPANEL_QUEUE_MESSAGE，长度为16 字
	节。
参数说明：
	输入参数：
		name：一个最多四字符长字符串，系统中队列的名字应该唯一；
		len：本参数指定队列中最多能保存的消息个数。当队列中消息达到这个值时，
		后续的在这个队列上发消息的操作会失败
		wait_mode：这个参数决定当队列中有消息时，等待在此队列上的线程获得
		满足的次序，有两个选项：
		– IPANEL_TASK_WAIT_FIFO 按先入先出的方式在等待线程中分发
		消息
		– IPANEL_TASK_WAIT_PRIO 优先满足高优先级的线程。
	输出参数：无
		返 回：
		！＝IPANEL_NULL: 成功，消息队列句柄;
		＝＝IPANEL_NULL: 失败。
*/
extern UINT32_T ipanel_porting_queue_create(const CHAR_T *name, UINT32_T len, UINT32_T wait_mode);

/*
功能说明：
	销毁一个消息队列。
参数说明：
	输入参数：
		handle：消息队列句柄，由ipanel_porting_queue_create 获得。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_queue_destroy ( UINT32_T handle );

/*
功能说明：
	发送消息。注意：函数返回后msg 指向的结构中的内容就无效了，函数实现者要
	注意自己保存相关信息。
参数说明：
	输入参数：
		handle：消息队列句柄，由ipanel_porting_queue_create 获得。
		msg：指向一个IPANEL_QUEUE_MESSAGE 结构的一个指针，结构中包含
		要放到消息队列中的消息
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_queue_send(UINT32_T handle,IPANEL_QUEUE_MESSAGE *msg);

/*
功能说明：
	从指定队列接收消息，消息队列需要支持指定时间的超时等待模式。
参数说明：
	输入参数：
		handle：消息队列句柄，由ipanel_porting_queue_create 获得。
		msg：指向一个IPANEL_QUEUE_MESSAGE 结构的一个指针，保存从队列
		中收到的消息，不能为空。
		wait_time：等待时间，单位为毫秒。为IPANEL_NO_WAIT(0)时表示立即返回，
		为IPANEL_WAIT_FOREVER(-1)表示永久等待。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
extern INT32_T ipanel_porting_queue_recv(UINT32_T handle,IPANEL_QUEUE_MESSAGE *msg, INT32_T wait_time);

#endif

/*--eof-----------------------------------------------------------------------------------------------------*/

