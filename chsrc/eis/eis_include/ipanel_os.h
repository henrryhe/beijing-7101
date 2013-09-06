/*******************************************************************************
Copyright (c) 2007 by iPanel Technologies, Ltd.
All rights reserved. You are not allowed to copy or distribute
the code without permission.
*******************************************************************************/

#ifndef __IPANEL_PORTING_TASK_H______
#define __IPANEL_PORTING_TASK_H______

#include "ipanel_typedef.h"

/*
** 强烈建议不要使用"EIS_"打头的这些definitions!  --McKing
*/

#ifdef __cplusplus
extern "C" {
#endif

/* ipanel_porting_sem_wait()/ipanel_porting_queue_recv()的等待方式: 立即返回 和 永久等待 */
#define EIS_QUEUE_SEM_WAIT_RET_NOW  (0)
#define EIS_QUEUE_SEM_WAIT_FOREVER  (-1)

#define IPANEL_NO_WAIT              (0)
#define IPANEL_WAIT_FOREVER         (-1)

/*******************************************************************************
** Queue Message Structure used by the OSP.
** Note that it was decided to use a structure for the message type so as to
** provide bounds checking on the message being placed in the queue. An array
** was the obvious alternative to such a structure, but it was felt that the
** array would not clearly indicate to the user the the limitations on the
** message size to use.
*******************************************************************************/
typedef struct
{
    UINT32_T q1stWordOfMsg;                /* First word of a queue message.  */
    UINT32_T q2ndWordOfMsg;                /* Second word of a queue message. */
    UINT32_T q3rdWordOfMsg;                /* Third word of a queue message.  */
    UINT32_T q4thWordOfMsg;                /* Fourth word of a queue message. */
} IPANEL_QUEUE_MESSAGE;

/*
** constants for returning status of waiting semaphore(P operation)
*/
enum
{
	EIS_OS_QUEUE_SEM_STATUS_AVAILABLE=IPANEL_OK,    // queue_wait/semaphore_wait is available(not locked, etc)
    EIS_OS_QUEUE_SEM_STATUS_UNAVAILABLE=IPANEL_ERR  // queue_wait/semaphore_wait is unavailable(locked, etc)
};

enum
{
    IPANEL_TASK_WAIT_FIFO, //推荐使用的模式
    IPANEL_TASK_WAIT_PRIO
};
#define EIS_TASK_WAIT_FIFO	(0)
#define EIS_TASK_WAIT_PRIO	(1)

typedef  VOID (*IPANEL_TASK_PROC)(VOID *param);

/*!!!!!!!注意!!!!!!!!*/
/*PSOS::::如果func在程序运行的过程中退出后，系统自动销毁了该task handle; 如果func一直在运行，那么需要销毁该task handle. */
/*VxWorks::::func在运行的过程中退出，该task handle系统没有销毁. */
/*******************************************************************************
参数:
name：          任务的名字
Func:       函数入口地址:typedef VOID(*IPANEL_TASK_PROC)(VOID *);
Param:      参数列表(一般置为NULL)
stack_size: 栈大小(>0有效)
priority:   优先级别...(ipanel优先级从0到31,31最高,0最低)
返回值:
handle( 0 失败)
*******************************************************************************/
UINT32_T ipanel_porting_task_create(CONST CHAR_T *name, IPANEL_TASK_PROC func, VOID *param,
                                    INT32_T priority, UINT32_T stack_size);

/*******************************************************************************
参数:
handle: task handle(非0,且存在,有效)
返回值:
0:success
-1:failed
*******************************************************************************/
INT32_T ipanel_porting_task_destroy(UINT32_T handle);

/*******************************************************************************
参数:
       　休眠时间，单位毫秒
返回值:　无
*******************************************************************************/
VOID ipanel_porting_task_sleep(INT32_T ms);

/*******************************************************************************
参数:
name: semaphore name(好像只有pSOS的sm_create()需要的name, 原型是: char name[4])
initial_count:最大初始化计数(大于等于0有效)this is the initial token count
        for the semaphore being created. This value will determine the maximum
        number of simultaneous accesses allowed to whatever resource is
        being protected by the semaphore.
wait_mode: this parameter determines how tasks will wait on a
        token from an 'empty' semaphore. There are two options for this
        parameter;
        EIS_TASK_WAIT_FIFO - the first task to start pending on the token, will
                    receive the token when is made available.先进入等待队列的任务先
                    获得信号量
        EIS_TASK_WAIT_PRIO - the highest priority task pending on the token,
                    will receive the token when it is made available.优先级高的任务
                    先获得信号量
        并非所有的RTOS都支持这两种等待模式。win32、OS20、Ecos、UCOS、Linux上
        不能设置，PSOS、VxWorks可以设置。
返回值:
Handle(0 失败)
*******************************************************************************/
UINT32_T ipanel_porting_sem_create(CONST CHAR_T *name, INT32_T initial_count,
                                   UINT32_T wait_mode);

/*******************************************************************************
参数:
handle  semaphore handle(非0,且存在,有效)
返回值:
0:success
-1:failed
*******************************************************************************/
INT32_T ipanel_porting_sem_destroy(UINT32_T handle);

/*******************************************************************************
参数:
handle  semaphore handle(非0,且存在,有效)
wait_time  等待semaphore的时间,  0 表示立即返回, -1 表示一直等待。
返回值:
EIS_OS_QUEUE_SEM_STATUS 信号的当前状态
当EIS_OS_QUEUE_SEM_STATUS_AVAILABLE时,表示等待到信号.
EIS_OS_QUEUE_SEM_STATUS_UNAVAILABLE:failed
*******************************************************************************/
INT32_T ipanel_porting_sem_wait(UINT32_T handle, INT32_T wait_time);

/*******************************************************************************
参数:
handle  semaphore handle(非0,且存在,有效)
将handle释放一个信号
返回值:
0:success
-1:failed
*******************************************************************************/
INT32_T ipanel_porting_sem_release(UINT32_T handle);

/*******************************************************************************
参数:
name:   queue name
len, this specifies the maximum number of messages that
    can reside in the Queue at any given moment. When the number of
    un-read messages in the queue reaches this value, then any
    subsequent attempts to place a message in this queue will fail.
wait_mode, this parameter determines how tasks waiting on a
    message from an empty queue will wait. There are two options
    for this parameter;
    EIS_TASK_WAIT_FIFO - the first task to start pending on the queue, will
                receive the message when it arrives.
                先进入队列的任务先得到消息
    EIS_TASK_WAIT_PRIO - the highest priority waiting task will receive the
                message when it arrives.
                优先级高的任务先得到消息。
    并非所有RTOS都支持设置这两种模式，win32、OS20、ECOS、UCOS、Linux不能设置，
    PSOS、VxWorks可以设置。
返回值:
handle   0 失败
*******************************************************************************/
UINT32_T ipanel_porting_queue_create(CONST CHAR_T *name, UINT32_T len,
                                     UINT32_T wait_mode);

/*******************************************************************************
参数:
queue handle (非0,且存在,有效)
返回值:
0:success
-1:failed
*******************************************************************************/
INT32_T ipanel_porting_queue_destroy(UINT32_T handle);

/*******************************************************************************
参数:
queue handle (非0,且存在,有效)
msg:Pointer to a IPANEL_QUEUE_MESSAGE structure containing the
    message to put in the queue.
返回值:
0:success
-1:failed
*******************************************************************************/
INT32_T ipanel_porting_queue_send(UINT32_T handle, IPANEL_QUEUE_MESSAGE *msg);

/*******************************************************************************
** Function: ipanel_porting_queue_recv(): This function is called to attempt to
** retrieve a message from the specified queue. If a message is present, then
** the RTOS will copy the message into the place holder specified in the
** input parameter list. If no message is present, then depending on the input
** parameters, the calling task will either wait for a timeout, or return
** immediately. In either case, the return code will indicate whether or not
** a message was received. If no message is present, and the task has chosen
** to wait forever, then the calling task will block until a message is posted
** to this queue.
** NOTE: The millSecsToWait period is limited by the interrupt frequency of the
** hardware timer in the system. For that reason, if it is greater than zero,
** it is rounded to the lower period, for which OSP_TIMER_TICK_INT_PERIOD is
** an integral.
**
** Inputs:  handle: the id of the queue from which the message is to be read.
**          msg:     a pointer to a message structure - 16 bytes in length.
**          wait_time: 等待时间，单位为毫秒。为0时表示立即返回，为-1表示永久等待。
**
** Outputs: NONE.
**
** Returns:
**  EIS_OS_QUEUE_SEM_STATUS function status
**  EIS_OS_QUEUE_SEM_STATUS_AVAILABLE:success
**  EIS_OS_QUEUE_SEM_STATUS_UNAVAILABLE:failed
*******************************************************************************/
INT32_T ipanel_porting_queue_recv(UINT32_T handle, IPANEL_QUEUE_MESSAGE *msg,
                                  INT32_T wait_time);

#ifdef __cplusplus
}
#endif

#endif // __IPANEL_PORTING_TASK_H______
