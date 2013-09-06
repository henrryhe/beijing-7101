/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_os.c
  * 描述: 	实现OS相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"

#include "eis_include\ipanel_os.h"
#include "eis_api_os.h"
#include "stlite.h"
#include "..\main\initterm.h"
#include "..\dbase\vdbase.h"
#ifdef __EIS_API_DEBUG_ENABLE__
#define __EIS_API_OS_DEBUG__
#endif
extern ST_Partition_t   *gp_appl2_partition;
 extern BOX_INFO_STRUCT	*pstBoxInfo;    /* Box information from e2prom */



ST_Partition_t           *gp_Eis_partition_VID_LMI = NULL;


/*函数名:     void CH_Eispartition_init_VIDLMI(void)
  *开发人员:
  *开发时间:
  *函数功能:初始化应用区64 EIS 内存
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
void CH_Eispartition_init_VIDLMI(void)
{
    U8 *p_Mem;
      p_Mem = ( U8 *)(osplus_cached_address((U8*)(0xB4000000),true));
      gp_Eis_partition_VID_LMI = partition_create_heap((U8*)p_Mem,0x03800000);	
}


static eis_task_t geis_task[EIS_MAX_TASK_NUM];	/* 定义task结构 */
static semaphore_t *gpsema_eis_time	= NULL;	/* 用于保证ipanel_porting_time_ms非重入*/

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
VOID *ipanel_porting_malloc(INT32_T memsize)
{
	void *p_return = NULL;
      
	p_return = memory_allocate (gp_appl2_partition  , memsize );

	if ( NULL == p_return )
	{

	#ifdef __EIS_API_OS_DEBUG__
		eis_report ( "\nEIS no mem<%d>", memsize );
	#endif

             	p_return = memory_allocate ( gp_Eis_partition_VID_LMI, memsize );

		memset(p_return,0,memsize);
		   
		return p_return;

	}
	
	memset(p_return,0,memsize);

	return p_return;
}

/*
功能说明：
	释放ptr 指定的内存空间，并且该内存空间必须是由ipanel_porting_malloc 分配的。
参数说明：
	输入参数：
		ptr ：指向要释放的内存的指针。
	输出参数：无
*/
VOID ipanel_porting_free(VOID *ptr)
{
	if ( NULL == ptr )
	{
		return;
	}

       if((U32)ptr < 0x94000000)
	{
	memory_deallocate ( gp_appl2_partition, ptr );
	}
	else
	{
	     memory_deallocate ( gp_Eis_partition_VID_LMI, ptr );
	}


	ptr = NULL;

	return;
}

#define TICKS_PER_SEC				( ST_GetClocksPerSecond() )
#define TICKS_MS_PER_OVERFLOW		( (0xffffffff  / TICKS_PER_SEC)*1000 )
#define TICKS_PER_MS				( ST_GetClocksPerSecond()  / 1000 )

typedef struct  tagEIS_TIME{
		unsigned long last_ticks;
		unsigned long overflow_times;
}eis_time_t;

static eis_time_t eis_time;
static unsigned long last_ticks;

/*
功能说明：
	浏览器计时器的初始化
注意:
	必须在每次浏览器开始时调用
  */
void eis_timeinit(void)
{
	eis_time.last_ticks 		= 0;
	eis_time.overflow_times 	= 0;
	gpsema_eis_time = semaphore_create_fifo(1);
}

UINT32_T get_time_ms(void)
{
	unsigned long cur_ticks;

	cur_ticks = time_now();

	return cur_ticks / TICKS_PER_MS;	

}
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
UINT32_T ipanel_porting_time_ms(VOID)
{
	unsigned long cur_ticks;
#if 1
	if(NULL != gpsema_eis_time)	
		semaphore_wait(gpsema_eis_time);
	
	cur_ticks 	 = time_now ();
	if((eis_time.last_ticks >cur_ticks)&&(eis_time.last_ticks -cur_ticks>500)&&(eis_time.last_ticks>0xffff0000))
	{
		eis_time.overflow_times+=eis_time.last_ticks/TICKS_PER_MS;
		eis_report("\n time overflow!!");
	}
	eis_time.last_ticks =cur_ticks;
	cur_ticks = cur_ticks / TICKS_PER_MS+eis_time.overflow_times;
	
	if(NULL != gpsema_eis_time)	
		semaphore_signal ( gpsema_eis_time );
	return (cur_ticks);

#else

	cur_ticks = time_now();

	return cur_ticks / TICKS_PER_MS;	
#endif

}

/*
功能说明：
	在主进程中检查是否要重启
参数说明：
	输入参数： 无
	输出参数：无
	返 回： 无
  */
void ipanel_porting_check_reboot ( void )
{
	int i;

	if ( 0 == geis_reboot_timeout )
	{
		return;
	}
	else
	{
		i = time_after ( time_now (), geis_reboot_timeout );

		if ( 1 == i )
		{
			reset_cpu ();	/* 立即重启 */
		}
	}

	return;
}

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
INT32_T ipanel_porting_system_reboot(INT32_T s)
{
	if ( 0 == s )
	{
		reset_cpu ();	/* 立即重启 */
	}
	else
	{
		geis_reboot_timeout = time_plus ( time_now (), ST_GetClocksPerSecondLow() );
	}
}

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
INT32_T ipanel_porting_system_standby(INT32_T s)
{
	eis_report("\n ipanel_porting_system_standby s=%d",s);
	switch(s)
	{
		case -1:
			pstBoxInfo->abiBoxPoweredState=1;
			//CH_NVMUpdate(idNvmBoxInfoId); 
			eis_report("\n 待机pstBoxInfo->abiBoxPoweredState=%d",pstBoxInfo->abiBoxPoweredState);
			return IPANEL_OK;
		case -3:
			pstBoxInfo->abiBoxPoweredState=0;
			//CH_NVMUpdate(idNvmBoxInfoId); 
			eis_report("\n 开机pstBoxInfo->abiBoxPoweredState=%d",pstBoxInfo->abiBoxPoweredState);
	return IPANEL_OK;
}
}

/*
功能说明:
	eis进程创建
注意:
	在系统初始化的时候调用
参数说明:
	无
  */
void ipanel_porting_task_init ( void )
{
	int i;

	for ( i = 0; i < EIS_MAX_TASK_NUM; i ++ )
	{
		geis_task[i].mu32_TaskID 		= 0;
		geis_task[i].mu8_TaskPriority	= 0;
		geis_task[i].mp_TaskStack		= NULL;
	}
}

/*
功能说明：
	用于创建一个线程。
参数说明：
	输入参数：
		name：一个最多四字节长字符串，系统中线程名称应该唯一；
		func：线程主体函数入口地址，函数原型定义如下；
		typedef VOID (*IPANEL_TASK_PROC)(VOID *param);
		param：线程主体函数的参数列表指针(可置为IPANEL_NULL)；
		priority：优先级别(ipanel 优先级从0 到31，0 最低,31 最高)；
		stack_size：栈大小，以字节为单位
	输出参数：无；
	返 回：
		!= IPANEL_NULL：成功，返回线程实例句柄。
		＝＝IPANEL_NULL：失败
*/
UINT32_T ipanel_porting_task_create(CONST CHAR_T *name, IPANEL_TASK_PROC func, VOID *param, INT32_T priority, UINT32_T stack_size)
{
	int 		prio;
	UINT32_T p_Task;
	int		i, i_Error;
	static tdesc_t				g_UsifTaskDesc;


	/* 寻找空闲未用的task */
	for ( i = 0; i < EIS_MAX_TASK_NUM; i ++ )
	{
		if ( 0 == geis_task[i].mu32_TaskID && 0 == geis_task[i].mu8_TaskPriority )
		{
			break;
		}
	}

	if ( i >= EIS_MAX_TASK_NUM )
	{
		eis_report ( "\n++>eis 无可用进程!" );
		return IPANEL_NULL;
	}

	prio = priority / 2 +3;
	if(prio>15)
		prio=12;

	if(memcmp(name,"SCKT",4)==0)
	{
		prio = 12;
	}
	eis_report ( "\n++>eis ipanel_porting_task_create taskname=%s, priority=%d,actpri=%d" ,name,priority,prio);
#if 0
	geis_task[i].mp_TaskStack	= memory_allocate ( SystemPartition, stack_size );

	if ( NULL == geis_task[i].mp_TaskStack )
	{
		eis_report ( "\n++>eis 进程空间分配失败!" );
		return IPANEL_NULL;
	}
#endif	
	geis_task[i].mu8_TaskPriority = prio;

	p_Task = (UINT32_T)Task_Create ( (void(*)(void*))*func, param,stack_size+8*1024, prio, (const char *)name, 0 );

	geis_task[i].mu32_TaskID 		= p_Task;

	eis_report ( "\n++>eis task id=0x%x", p_Task );
	
	return p_Task;
}

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
INT32_T ipanel_porting_task_destroy(UINT32_T handle)
#if 0
{
	int rt;
	task_t *hwtsk = NULL;

	if(handle)
	{
		hwtsk = (task_t*)handle;

		if(hwtsk->__id->task_state == 0)
		{
			eis_report ("HW LOCK(%s %s\n",__FILE__,__LINE__);  
			task_lock();
			hwtsk->__id->task_state = 1;
			hwtsk->__id->task_tdesc = hwtsk->__desc;
			task_unlock();
			eis_report ("HW UNLOCK(%s %s\n",__FILE__,__LINE__);
		}
		rt = task_suspend(hwtsk->__id);
		eis_report ("task_suspend %s\n", rt ? (char*)GetErrorText(rt) : "ok");/**/
		rt = task_kill(hwtsk->__id, 0, 0/*1*/);
		eis_report ("HW kILL (%s %s\n",__FILE__,__LINE__);
		/*等待任务结束*/
		rt = task_wait(&hwtsk->__id, 1, TIMEOUT_INFINITY);
		eis_report ("HW task wait = %d\n", rt);
		/*可以删除堆栈了*/
		rt = task_delete(hwtsk->__id);
		eis_report ("HW task delete = %d\n", rt );

		if(rt == 0)
		{
			CH_FreeMem(hwtsk->__desc);
			CH_FreeMem(hwtsk->__stack);
			CH_FreeMem(hwtsk->__id);
			CH_FreeMem(hwtsk);
		}
		else
			eis_report ("\n\n ### @@@ task_delete FAILED ### @@@ \n");
		//check_memStatus("vod destroy!!");   
		eis_report ("%s: delete task 0x%x return %d-- %s\n",rt ? "ERROR" : "INFO", handle, rt,  (char*)GetErrorText(rt));
	}
}
#else
{
	int i, error;
	task_t *task_list_p[2];

	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_task_destroy 句柄为空" );
		return IPANEL_ERR;
	}

	for ( i = 0; i < EIS_MAX_TASK_NUM; i ++ )
	{
		if ( handle == geis_task[i].mu32_TaskID )
			break;
	}

	if ( i >= EIS_MAX_TASK_NUM )
	{
		eis_report ( "\n++>eis ipanel_porting_task_destroy 没找到对应的句柄" );
		return IPANEL_ERR;
	}

	task_list_p[0]=(task_t*)handle;    
	task_list_p[1]=NULL;

	error = task_wait(task_list_p, 1, TIMEOUT_INFINITY);

	if ( 0 != error )
	{
		eis_report ( "\n++>eis ipanel_porting_task_destroy task_wait 错误" );
		//return IPANEL_ERR;
	}
	else
	{
		eis_report ( "\n++>eis ipanel_porting_task_destroy task_wait 正确" );
	}
	
	error = task_delete((task_t*)handle);
	handle = NULL;

	if(0!=error)
	{
		eis_report ( "\n++>eis ipanel_porting_task_destroy task_delete 错误" );

		//return IPANEL_ERR;
	}
	else
	{
		eis_report ( "\n++>eis ipanel_porting_task_destroy task_delete 正确" );		
	}

	/* 释放空间 */
	if ( NULL != geis_task[i].mp_TaskStack )
	{
		memory_deallocate ( SystemPartition, geis_task[i].mp_TaskStack );
		geis_task[i].mp_TaskStack = NULL;
	}

	geis_task[i].mu32_TaskID 		= 0;
	geis_task[i].mu8_TaskPriority	= 0;
	geis_task[i].mt_TaskDesc		= 0;

	return IPANEL_OK;
}
#endif

/*
功能说明：
	将当前线程挂起指定时间，同时让出CPU 供其他线程使用。
参数说明：
	输入参数：
		ms：挂起线程的时间长度，单位为毫秒。
	输出参数：无；
	返 回：无
*/
VOID ipanel_porting_task_sleep(INT32_T ms)
{
#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n <<<<[ipanel_porting_task_sleep] ms= %d\n",ms);
#endif

	task_delay ( ((unsigned int)(ST_GetClocksPerSecond()) * ms) / 1000 );
}

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
UINT32_T ipanel_porting_sem_create(const CHAR_T*name, INT32_T initial_count, UINT32_T wait_mode)
{
	UINT32_T sema;
#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n time=%d <<<<[ipanel_porting_sem_create] ms= %s",get_time_ms(),name);
#endif

	/* initial_count只能为0或1 */
	initial_count = ( initial_count > 1 || initial_count < 0 ) ? 1 : initial_count;

	sema = (UINT32_T)semaphore_create_fifo_timeout( (U32)initial_count );
	eis_report ( "  handel = %x\n",sema);

	return sema;
}

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
INT32_T ipanel_porting_sem_destroy(UINT32_T handle)
{
	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_sem_destroy 句柄为空" );
		return IPANEL_ERR;
	}
#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n <<<<[ipanel_porting_sem_destroy] ms= %x\n",handle);
#endif

	semaphore_delete ( (semaphore_t *)handle );
	handle = 0;

	return IPANEL_OK;
}

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
INT32_T ipanel_porting_sem_wait(UINT32_T handle,INT32_T wait_time)
#if 1
{
	int ret;
	clock_t  time;
	int val;

#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "<<<<[ipanel_porting_sem_wait] sem_handle = %x,%d\n",handle,wait_time);
#endif

	if ( NULL == handle )
	{
#ifdef __EIS_API_OS_DEBUG__
		eis_report ( "<<<<[ipanel_porting_sem_wait] failed  this sem didn't exist\n");
#endif
		return IPANEL_ERR;
	}		

	if(wait_time == 0)
	{
    		val = semaphore_wait_timeout((semaphore_t*)handle,TIMEOUT_IMMEDIATE);
		
	}
	else if(wait_time == -1)
	{
		
		val =  semaphore_wait_timeout((semaphore_t*)handle,TIMEOUT_INFINITY);	

	}
	else if(wait_time >0)
	{
		
		time = time_plus(time_now(), ST_GetClocksPerSecondLow()/1000*wait_time);
		val =  semaphore_wait_timeout((semaphore_t*)handle,&time);	
	}
	else
	{
#ifdef __EIS_API_OS_DEBUG__
		eis_report ( "<<<<[ipanel_porting_sem_wait] failed  wait_time ERR\n");
#endif
		return IPANEL_ERR;

	}
	
	if(val == 0)
	{
#ifdef __EIS_API_OS_DEBUG__
		eis_report ( "<<<<[ipanel_porting_sem_wait] sem_handle = %x SUCESSFUL \n",handle);
#endif
		ret = IPANEL_OK;
	}
	else
	{
#ifdef __EIS_API_OS_DEBUG__
		eis_report ( "<<<<[ipanel_porting_sem_wait] sem_handle = %x FAILED! \n",handle);
#endif
		ret = IPANEL_ERR;
	}
	
        return ret;
}
#else
{
	clock_t 	time_out;
	int		error;

	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_sem_wait 句柄为空" );
		return IPANEL_ERR;
	}

	eis_report ( "\n++>eis signal wait=<%s, 0x%x, %d>",  task_name((task_t*)task_id()), handle, wait_time );

	if ( 0 == wait_time )
	{
		error = semaphore_wait_timeout ( (semaphore_t *)handle, TIMEOUT_IMMEDIATE );
		if ( 0 == error )
		{
			eis_report ( "\n++>eis wait end1" );
			return IPANEL_OK;
		}
		else
		{
			eis_report ( "\n++>eis wait end2" );
			return IPANEL_ERR;
		}
		return IPANEL_OK;
	}
	else if ( -1 == wait_time )
	{
		semaphore_wait_timeout ( (semaphore_t *)handle, TIMEOUT_INFINITY );
		return IPANEL_OK;
	}
	else
	{
		time_out = time_plus ( time_now(), ST_GetClocksPerSecondLow() * wait_time / 1000 );
		error = semaphore_wait_timeout ( (semaphore_t *)handle, &time_out );
		if ( 0 == error )
		{
			eis_report ( "\n++>eis wait end3" );
			return IPANEL_OK;
		}
		else
		{
			eis_report ( "\n++>eis wait end4" );
			return IPANEL_ERR;
		}
	}

	return IPANEL_ERR;
}
#endif

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
INT32_T ipanel_porting_sem_release(UINT32_T handle)
{
	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_sem_release 句柄为空" );
		return IPANEL_ERR;
	}

#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n++>eis signal release=<%s, 0x%x>", task_name((task_t*)task_id()), handle );
#endif
	semaphore_signal ( (semaphore_t *)handle );
#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n++>eis release end" );
#endif
	return IPANEL_OK;
}

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
UINT32_T ipanel_porting_queue_create(const CHAR_T *name, UINT32_T len, UINT32_T wait_mode)
{
	UINT32_T queue;

	eis_report ( "\n++>eis ipanel_porting_queue_create" );

	queue = (UINT32_T)message_create_queue_timeout ( sizeof ( IPANEL_QUEUE_MESSAGE ), 50 );
	if ( 0 == queue )
   	{
		eis_report ( "\n++>eis ipanel_porting_queue_create fail<%s>", (char*)name );
		return IPANEL_NULL;
   	}
   	else
   	{
		symbol_register ( name, ( void * ) queue );
   	}

	return queue;
}

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
INT32_T ipanel_porting_queue_destroy ( UINT32_T handle )
{
	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_queue_destroy 无效句柄" );
		return IPANEL_ERR;
	}

	message_delete_queue ( (message_queue_t*)handle );
	handle = 0;

	return IPANEL_OK;
}

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
INT32_T ipanel_porting_queue_send(UINT32_T handle,IPANEL_QUEUE_MESSAGE *msg)
{
	IPANEL_QUEUE_MESSAGE *pMsg;
	clock_t	time_out;
#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n time=%d <<<<[ipanel_porting_queue_send] handle= %x",get_time_ms(),handle);
#endif

	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_queue_send 无效句柄" );
		return IPANEL_ERR;
	}

	time_out = time_plus ( time_now(), ST_GetClocksPerSecondLow() );

	pMsg = (IPANEL_QUEUE_MESSAGE *)message_claim_timeout ( (message_queue_t*)handle, &time_out );

	if ( NULL == pMsg )
	{
		eis_report ( "\n++>eis ipanel_porting_queue_send 消息申请失败" );
		return IPANEL_ERR;
	}

	pMsg->q1stWordOfMsg = msg->q1stWordOfMsg;
	pMsg->q2ndWordOfMsg = msg->q2ndWordOfMsg;
	pMsg->q3rdWordOfMsg = msg->q3rdWordOfMsg;
	pMsg->q4thWordOfMsg = msg->q4thWordOfMsg;

	message_send ( (message_queue_t*)handle, pMsg );

	return IPANEL_OK;
}

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
INT32_T ipanel_porting_queue_recv(UINT32_T handle,IPANEL_QUEUE_MESSAGE *msg, INT32_T wait_time)
{
	clock_t time_out;
	IPANEL_QUEUE_MESSAGE *p_msg = NULL;
#ifdef __EIS_API_OS_DEBUG__
	eis_report ( "\n time=%d <<<<[ipanel_porting_queue_send] handle= %x",get_time_ms(),handle);
#endif

	if ( 0 == handle )
	{
		eis_report ( "\n++>eis ipanel_porting_queue_send 无效句柄" );
		return IPANEL_ERR;
	}

	if ( 0 == wait_time )
	{
		p_msg = (IPANEL_QUEUE_MESSAGE *)message_receive_timeout ( (message_queue_t*)handle, TIMEOUT_IMMEDIATE );

		if ( NULL == p_msg )
		{
			return IPANEL_ERR;
		}
	}
	else if ( -1 == wait_time )
	{
		p_msg = (IPANEL_QUEUE_MESSAGE *)message_receive_timeout ( (message_queue_t*)handle, TIMEOUT_INFINITY );

		if ( NULL == p_msg )
		{
			return IPANEL_ERR;
		}
	}
	else
	{
		time_out = time_plus ( time_now(), ST_GetClocksPerSecondLow() / 1000 * wait_time );
		p_msg = (IPANEL_QUEUE_MESSAGE *)message_receive_timeout ( (message_queue_t*)handle, &time_out );

		if ( NULL == p_msg )
		{
			return IPANEL_ERR;
		}
	}

	eis_report ( "\ncome here!" );
	msg->q1stWordOfMsg 	= p_msg->q1stWordOfMsg;
	msg->q2ndWordOfMsg = p_msg->q2ndWordOfMsg;
	msg->q3rdWordOfMsg 	= p_msg->q3rdWordOfMsg;
	msg->q4thWordOfMsg 	= p_msg->q4thWordOfMsg;
	message_release((message_queue_t*)handle, p_msg);

	return IPANEL_OK;
}

/*--eof-----------------------------------------------------------------------------------------------------*/

