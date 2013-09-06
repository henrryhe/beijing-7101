/* 平台相关的宏定义，平台改变后，重新定义宏 */
#ifndef _PLAT_CTRL_H_
#define _PLAT_CTRL_H_

#include "stdio.h"
#include "stvid.h"
#include "sttbx.h"
#include "graphconfig.h"

/*#define _BUILD_IN_WINDOWS_ 平台定义*/
#define gz_STATIC  /*静态定义*/


#define J_OSP_FreeMemory CH_FreeMem

#ifndef NULL
#undef  NULL
#define NULL 0
#endif

/*无符号整数,根据不同的平台应该有所改变*/
#ifndef DEFINED_U8
#define DEFINED_U8
#define U8 unsigned char
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
#define U16 unsigned short
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
#define U32 unsigned  long
#endif

#ifndef DEFINED_U64
#define DEFINED_U64
#define U64 CH_UNSIGNED_64
#endif

#ifndef UINT
#define UINT unsigned int/* 尽量少用，最好不用,而用U32类型替代，避免产生编译警告 */
#endif

/*符号整数，根据不同的平台应该有所改变*/
#ifndef DEFINED_S8
#define DEFINED_S8
#define S8 signed char
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
#define S16 signed short
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
#define S32 signed  long
#endif

#ifndef DEFINED_S64
#define DEFINED_S64
#define S64 CH_SIGNED_64
#endif

#if 0
#ifndef INT
#define INT int /* 尽量少用，最好不用,而用S32类型替代，避免产生编译警告 */
#endif
#endif/*5107平台已经定义了*/

#ifndef CH_FLOAT 
#define CH_FLOAT float
#endif

#ifndef CH_BOOL
#define CH_BOOL S8
#endif

#define CH_TRUE 1
#define CH_FALSE 0

#ifndef PU64
#define PU64 unsigned long int
#endif
/*定时器状态*/
typedef enum _TimerState_
{
	CH_TIMER_INIT	= 0,
	CH_TIMER_STOP	= 1,
	CH_TIMER_RUN	= 2,
	CH_TIMER_EXPIRE = 3
}CH_TIMER_STATE,*PCH_TIMER_STATE;

/*平台相关OS 初始化函数,成功返回0，失败返回-1
   如果不需要初始化就直接返回0*/
S32 CH_InitPlatOS(void);

S32 CH_ReleasePlatOS(void);

/* 定时器回调函数类型,由不同平台的定时器函数调用该函数实现定时器用户回调 */
typedef void (*CH_TIMERFUNC) (struct _ChTimer_* pTimer);

/*定时器结构*/

typedef enum
{
	CH_TIME_ONESHOT = 0, /*定时器只响应一次*/
	CH_TIME_PERIODIC = 1 /*定时器每隔iDuration响应一次*/
}TIMER_TYPE;

typedef struct _ChTimer_ 
{
	void * pPlatTimer;		/* 指向平台相关的定时器结构,WINDOWS DDK下是PKTIMER结构指针*/
	U32 iDuration;			/* 持续时间,单位：毫秒*/
	U32 iExpiration;		/* 定时器到期时的时间,单位：毫秒 */
	CH_TIMERFUNC TimerFunc;	/* 定时器到期时调用的函数*/
	void* pArg;				/* 定时器回调函数的参数*/
	CH_TIMER_STATE iState;	/* 定时器状态*/
	TIMER_TYPE eTimerType;	/* 定时器类型，0:TIME_ONESHOT*/
	void * pPlatArg;		/* 平台实现可能需要用到的其他参数*/
} CH_TIMER,*PCH_TIMER;

/*平台相关的函数定义:*/

/*定时器和时间函数:*/

/*************************************************************************
 * 初始化定时器结构，某些平台的定时器可能需要先初始化定时器,调用这个函数前
 * pTimer 应该已经分配好内存。
 参数：
	void * pPlatArg;		 平台实现可能需要用到的其他参数
	iDuration:				 持续时间,单位：毫秒
	TIMER_TYPE eTimerType;	 定时器类型，0:TIME_ONESHOT
	CH_TIMERFUNC TimerFunc	 定时器到期时调用的函数
	void* pArg;				 定时器回调函数的参数
返回:
	成功返回 非0,失败返回 0
 *************************************************************************/
S32 CH_InitializeTimer(PCH_TIMER pTimer,void * pPlatArg,U32 iDuration,
					   TIMER_TYPE eTimerType,CH_TIMERFUNC TimerFunc,void* pArg);

void CH_StartTimer(PCH_TIMER pTimer);	/*定时器开始工作*/

void CH_StopTimer(PCH_TIMER pTimer);	/*停止定时器工作*/

void CH_DeleteTimer(PCH_TIMER pTimer);	/*删除定时器,并释放相关资源*/

/* 定时器重新计数.计数周期为pTimer->iDuration*/
#define CH_ResetTimer(pTimer) \
{\
	CH_StopTimer(pTimer);\
	CH_StartTimer(pTimer);\
}

CH_TIMER_STATE CH_GetTimerState(PCH_TIMER pTimer);/*得到定时器状态*/

/*得到系统时间(秒数)*/
U32 CH_GetSysTimeInSec(void);

/*得到系统时间(毫秒数)*/
U32 CH_GetSysTimeInMillisec(void);

/*当前进程休眠 iMillisecond(毫秒) */
void CH_Sleep(U32 iMillisecond);

/*内存函数：*/

void *CH_AllocMem(U32 size);/*分配size个字节的内存*/

void *CH_ReAllocMem(void * ptr,U32 iSize);/*重新分配内存*/

void CH_FreeMemB(void *p);/*释放内存*/


/*设置dest缓冲为指定的c值,设置长度为count,返回dest;(memset)
void *CH_MemSet(void *dest,S8 c,S32 count);*/
  void CH_Printf( char * Format_p, ...);
/*#define CH_Printf sttbx_Print图形层打印输出*/
#define CH_PutError CH_Printf
#define CH_MemSet memset

/*内存拷贝，从src拷贝长度为count的内存数据到dest，返回dest*/
void *CH_MemCopy( void *dest, void *src, S32 count );

/*内存移动，从src拷贝长度为count的内存数据到dest，返回dest
  *如果src 和dest 有重复的地方，函数确保重复的地方的数据
  * 能被正确拷贝*/
void *CH_MemMove( void *dest, void *src, S32 count );

/*Compare characters in two buffers.(功能同 memcmp)
	Return Value    Relationship of First count Bytes of buf1 and buf2 
	< 0             buf1 less than buf2 
	0               buf1 identical to buf2 
	> 0             buf1 greater than buf2 
*/
S32 CH_MemCompare( void *buf1, void *buf2, S32 count );

void *CH_SpaceMemCopy( void *dest, void *src, S32 count ,S32 spaceFlag);

S32 CH_StrLen(S8 *pStr);/*求字符串长度*/

/* 格式化数据到字符串 (sprintf 功能),调用者必须保证pOutBuff缓冲够大,不会越界
   函数原形:
S32 CH_SPrintf(S8 *pOutBuff, const S8* pInFormatStr, ...);  */
#define CH_SPrintf sprintf

/* 格式化输出到标准输出设备(printf)
   函数原形:
S32 CH_Printf( const S8 *pInFormatStr ,... );
#define CH_Printf printf*/

/* 从一个缓冲区中格式化读取数据(sscanf 功能) 
   函数原形:
S32 CH_SScanf(S8 *pInBuff, const S8* pInFormatStr, ...);*/
#define CH_SScanf sscanf

/*随机函数：*/
/*返回在0~n之间的随机整数,若返回负数表示不支持该功能*/
S32 CH_GetRandomInt(S32 n);

/*返回在0~1之间的随机实数，若返回负数表示不支持该功能*/
float CH_GetRandom(void);


/*线程相关数据结构*/

 /* 线程回调函数类型 */
#ifdef _BUILD_IN_WINDOWS_
#define CH_THREADFUNC void*
#else
typedef void (*CH_THREADFUNC)(U32 Param);
#endif
typedef struct  
{
	U32 ThreadHandle;			/* 根据不同的平台,是32位指针或句柄,由CH_CreateThread填充.*/
	void * pPlatThread;			/*指向平台实现相关的数据结构,*/
	CH_THREADFUNC ThreadFunc;	/* 线程函数*/
	void *pArg;					/* 线程函数的参数*/
	S8 Status; /*线程状态，0为挂起，非0为运行中*/
}CH_THREAD,*PCH_THREAD;
#if 0
/* 函数功能：创建线程，如果执行成功，将填充pThreadData的Handle字段。
 * 参数说明：pThreadData，线程数据结构指针。由调用者负责分配内存。
 * 返回值：失败返回NULL，若成功，根据不同的平台返回值是32位指针或句柄,
 * 并用该值填充pThreadData的Handle字段。
 */
U32 CH_CreateThread(PCH_THREAD pThreadData);

/* 函数功能：线程开始运行。
 * 参数说明：pThreadData，线程数据结构指针，由ch_CreateThead()创建。
 * 返回值：	
 */
void CH_StartThread(PCH_THREAD pThreadData);

/* 函数功能：退出,结束当前线程，在线程之外调用,尽量不要用该函数结束线程.
 * 参数说明：pThreadData，线程数据结构指针，pParam,平台实现需要用到的任何参数指针。
 */
S32 CH_TerminateThread(PCH_THREAD pThreadData,void * pParam);

/* 函数功能：继续线程运行。
 * 参数说明：pThreadData，线程数据结构指针。
 * 返回值：  
 */
void CH_ResumeThread(PCH_THREAD pThreadData);

/* 函数功能：挂起线程。
 * 参数说明：pThreadData，线程数据结构指针
 * 返回值：。
 */
void CH_SuspendsThread(PCH_THREAD pThreadData);

/* 函数功能：关闭线程句柄。
 * 参数说明：pThreadData，线程数据结构指针
 * 返回值：。
 */
void CH_CloseThread(PCH_THREAD pThreadData);
#endif

/* 进程同步相关API */

/* 同步事件相关数据结构 */

typedef enum
{
	CH_EVENT_SINAL = 0,
	CH_EVENT_NOSINAL = 1,
	CH_EVENT_TIMEOUT = 2,
	CH_EVENT_TIMEOUT_ERROR  = 3,
	CH_EVENT_ERROR = -1
}CH_EVENT_STATE;

#if 0
typedef struct  
{
	S32 iWaitNumber;
       CH_EVENT_STATE CH_EventSignal;
       S32 CH_TimeOut;
	semaphore_t EventHandle;/* 根据不同的平台,是32位指针或句柄,由CH_InitEvent填充. */
	void * pPlatEvent;/* 指向平台相关实现的事件数据结构, */
} CH_EVENT,*PCH_EVENT;
#else
typedef semaphore_t CH_EVENT;
typedef semaphore_t* PCH_EVENT;
#endif

/* 函数功能：初始化事件,函数成功必须填充pEvent的Handle字段。
 * 参数说明：pEvent，事件数据结构指针,调用者必须负责分配内存;pArg，平台实现可能需要用到的其它参数。
 * 返回值：成功返回pEvent指针，失败返回NULL。
 */
PCH_EVENT CH_InitEvent(PCH_EVENT pEvent,void* pArg);

/* 函数功能：清除事件到无信号态。
 * 参数说明：pEvent，事件数据结构指针。
 * 返回值： 
 */
void CH_ClearEvent(PCH_EVENT pEvent);

/* 函数功能：设置事件到信号态。
 * 参数说明：pEvent，事件数据结构指针;pArg，平台实现可能需要用到的其它参数。
 * 返回值： 
 */
void CH_SetEvent(PCH_EVENT pEvent);

/* 函数功能：设置事件到信号态,然后马上返回到无信号态。
 * 参数说明：pEvent，事件数据结构指针
 * 返回值： 
 */
void CH_PulseEvent(PCH_EVENT pEvent);

/* 函数功能：读事件的信号状态。
 * 参数说明：pEvent，事件数据结构指针。
 * 返回值： 
 */
CH_EVENT_STATE CH_ReadEventStatus(PCH_EVENT pEvent);

/* 函数功能：删除事件，并释放相关资源。
 * 参数说明：pEvent，事件数据结构指针。
 * 返回值： 
 */
void CH_DelEvent(PCH_EVENT pEvent);

/* 函数功能：阻塞进程，等待事件到有信号态。
 * 参数说明：pEvent，事件数据结构指针。
 *          iMilliseconds,等待的毫秒数。-1，无限等待，直到pEvent到信号态。
 *          如果平台不支持时间等待，该参数应是 -1；
 * 返回值：  CH_EVENT_SINAL(0):事件到信号态，CH_EVENT_TIMEOUT(1)：时间到返回，CH_TIMEOUT_ERROR，
 *			设置iMilliseconds > 0,但是平台不支持，返回是因为事件到信号态,CH_EVENT_ERROR(-1):错误。
 */
S32 CH_WaitEvent(PCH_EVENT pEvent,S32 iMilliseconds);

/* 互斥对象的相关数据结构*/
typedef struct  
{
	U32 MutexHandle;/* 根据不同的平台,是32位指针或句柄,由CH_InitMutex填充. */
	void * pPlatMutex;/* 指向平台相关实现的事件数据结构, */
} CH_MUTEX,*PCH_MUTEX;

/* 函数功能：初始化互斥对象,函数成功必须填充pMutex的Handle字段。
 * 参数说明：pMutex，互斥对象数据结构指针,调用者必须负责分配内存;
 *			 pArg，平台实现可能需要用到的其它参数。
 * 返回值：成功返回pMutex指针，失败返回NULL。 */
PCH_MUTEX CH_InitMutex(PCH_MUTEX pMutex, void* pArg);

/* 函数功能：释放互斥对象，互斥对象到信号态。
 * 参数说明：pMutex，互斥对象数据结构指针。
 * 返回值： 0 = 失败；非0 = 成功，pMutex到信号态。 */
S32 CH_MutexUnLock(PCH_MUTEX pMutex);

/* 函数功能：读互斥对象状态。
 * 参数说明：pMutex，互斥对象数据结构指针。
 * 返回值： 0 = 非信号态；1 = 信号态。;-1 = 错误。 */
S32 CH_ReadMutexStatus(PCH_MUTEX pMutex);

/* 函数功能：删除互斥对象，并释放相关资源。
 * 参数说明：pMutex，互斥对象数据结构指针。
 * 返回值：  */
void CH_DelMutex(PCH_MUTEX pMutex);

#define CH_MUTEX_SINAL 0
#define CH_MUTEX_TIMEOUT 1
#define CH_MUTEX_TIMEOUT_ERROR 2
#define CH_MUTEX_ERROR -1

/* 函数功能：阻塞进程，等待互斥对象到有信号态,并且取得互斥对象的拥有者。
 * 参数说明：pMutex，互斥对象数据结构指针。
 *          iMilliseconds,等待的毫秒数。-1，无限等待，直到pMutex到信号态。
 *          如果平台不支持时间等待，该参数应是 -1；
 * 返回值：  CH_MUTEX_SINAL(0):事件到信号态，CH_MUTEX_TIMEOUT(1)：时间到返回，CH_MUTEX_TIMEOUT_ERROR(3)，
 *			设置iMilliseconds > 0,但是平台不支持，返回是因为事件到信号态,CH_MUTEX_ERROR(-1):错误。
 */
U32 CH_MutexLock(PCH_MUTEX pMutex,S32 iMilliseconds/*毫秒*/ );

/* 信号量的相关数据结构 */
typedef struct  
{
	U32 SemaphoreHandle;/* 根据不同的平台,是32位指针或句柄,由CH_InitSemaphores填充.*/
	U32 iMaxCount;		/*设置信号机的最大计数*/
	void * pPlatMutex;	/*指向平台相关实现的事件数据结构,*/
} CH_SEMAPHORE,*PCH_SEMAPHORE ;


/* 函数功能：初始化信号量,函数成功必须填充pSemaphores的Handle字段。
 * 参数说明：pSemaphores，信号量数据结构指针,调用者必须负责分配内存;pArg，平台实现可能需要用到的其它参数。
 * 返回值：成功返回pSemaphores指针，失败返回NULL。 */
PCH_SEMAPHORE CH_InitSemaphores(PCH_SEMAPHORE pSemaphores,void* pArg);

/* 函数功能：读信号量状态。
 * 参数说明：pSemaphores，信号量数据结构指针。
 * 返回值： 0 = 非信号态；非0 = 信号态。 */
S32 CH_ReadSemaphoresStatus(PCH_SEMAPHORE pSemaphores);

/* 函数功能：删除信号量，并释放相关资源。
 * 参数说明：pSemaphores，信号量数据结构指针。
 * 返回值：  */
void CH_DelSemaphores (PCH_SEMAPHORE pSemaphores);

/* 函数功能：阻塞进程，等待信号量到有信号态。
 * 参数说明：pSemaphores，信号量数据结构指针。
 * 返回值：  */
void CH_WaitSemaphores (PCH_SEMAPHORE pSemaphores);

/* 进程间数据传输结构 */
typedef enum
{
	/* 通用数据传输命令 */
	msgCmdGetData,
	msgCmdSetData,
	msgCmdExchangeData
}E_MSG_COMMAND;


typedef enum
{  
	eMsgStatusIgnore,            
	eMsgStatusConsumeNoDefault, 
	eMsgStatusConsumeDoDefault  
}E_MSG_STATUS;

typedef enum
{
	eMsgParser
}E_MSG_TYPE;


typedef struct 
{
	U32 iTagetID;/* 消息发送的目的地（iTagetID 可以是进程(线程)句柄、窗口局柄、
	              * 或其它指向消息发送目标的结构指针） */
	U32 iSourceID;/* 消息发送的发送者标识（iSourceID 可以是进程(线程)句柄、窗口局柄、
	               * 或其它指向消息发送者的结构指针） */
	E_MSG_COMMAND eMsgCommond; /* 消息命令 */
	E_MSG_TYPE eMsgType; /* 消息类型 */
	/* 数据传输结构 */
	void *pInputData; /* 输入数据 */
	S32 iInputDataLength;
	void *pOutputBuff; /* 输出数据,由消息发送者分配内存 */
	S32 iOutputBuffLength;
	U32 iTime; /* 消息发送时的时间(单位：毫秒) */
}CH_MSG,*PCH_MSG;


/* return nonzero if success,return 0 if fail */
S32 CH_CopyFromUser(void * pPlat,S8* pDestBuf,S8* pSourceBuf,U32 iSize);

/* return nonzero if success,return 0 if fail */
S32 CH_CopyToUser(void * pPlat,S8* pDestBuf,S8* pSourceBuf,U32 iSize);

#ifndef CH_KERNEL_APPLICATION_DIVIDED

/*block current task and wait for pEvent ,while pEvent signed the function return.
  both  in_out_arg1 and in_out_arg2 are malloced space by caller.
  retrun nonzero if success,return 0 if fail*/
S32 CH_WaitMsgEvent(PCH_EVENT pEvent,U32 *in_out_arg1,U32 *in_out_arg2);
#endif

#endif
