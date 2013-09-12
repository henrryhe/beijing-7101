/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embx_osthread.c                                           */
/*                                                                 */
/* Description:                                                    */
/*         Operating system abstraction of threads/tasks           */
/*                                                                 */
/*******************************************************************/

#include "embxP.h"
#include "embx_osinterface.h"
#include "debug_ctrl.h"

static char *_default_name = "embx";

static char *get_default_name()
{
    /* Trivial for now, think about adding an incrementing
     * number to the name, but how do we deal with mutual
     * exclusion, or don't we care for this?
     */
    return _default_name;
}


#if defined(__LINUX__) && defined(__KERNEL__)

struct ThreadInfo
{
    task_t *kthread;
    void (*entry)(void *);
    void *param;
    EMBX_INT priority;
};

static int ThreadHelper(void *param)
{
    struct ThreadInfo *t = param;

    /* TODO: should priorities beyond -20 provoke a different scheduling policy */
    set_user_nice(t->kthread, t->priority);

    t->entry(t->param);

    /* wait for the user to call kthread_stop() */
    set_user_nice(t->kthread, 19);
    while (!kthread_should_stop()) {
	    set_current_state(TASK_INTERRUPTIBLE);
	    schedule();
    }

    return 0;
}

EMBX_THREAD EMBX_OS_ThreadCreate(void (*entry)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
    struct ThreadInfo *t;

    EMBX_Info(EMBX_INFO_OS, (">>>>EMBX_OS_ThreadCreate\n"));

    t = (struct ThreadInfo *) EMBX_OS_MemAlloc(sizeof(struct ThreadInfo));
    if (!t) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadCreate = EMBX_INVALID_THREAD (#1)\n"));
	return EMBX_INVALID_THREAD;
    }

    t->entry = entry;
    t->param = param;
    t->priority = priority;

    if (name == EMBX_DEFAULT_THREAD_NAME) {
	name = get_default_name();
    }

    t->kthread = kthread_create(ThreadHelper, t, "%s", name);

    if (IS_ERR(t->kthread)) {
	EMBX_OS_MemFree(t);
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadCreate = EMBX_INVALID_THREAD (#2)\n"));
	return EMBX_INVALID_THREAD;
    }

    wake_up_process(t->kthread);

    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadCreate = 0x%p\n", t));
    return t;
}

#endif /* __LINUX__ */


#if defined (__WINCE__) || defined(WIN32)

EMBX_THREAD EMBX_OS_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
DWORD  ThreadId;
HANDLE hThread;

    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    hThread = CreateThread( NULL,
                           (DWORD)0,
                           (LPTHREAD_START_ROUTINE)thread,
                           (LPVOID)param,
                           (DWORD)0,
                           &ThreadId );

    if( hThread != NULL )
    {
        SetThreadPriority(hThread, priority);
    }
    else
    {
        EMBX_DebugMessage(("ThreadCreate: CreateThread failed\n"));
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return hThread;
}

#endif /* __WINCE__ */


#if defined(__OS20__) || defined(__OS21__)

EMBX_THREAD EMBX_OS_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
task_t *t;

    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    if(name == EMBX_DEFAULT_THREAD_NAME)
    {
        name = get_default_name();
    }

    t = task_create(thread, param, EMBX_DEFAULT_THREAD_STACK_SIZE, priority, name, 0);
    if(t == EMBX_INVALID_THREAD)
    {
        EMBX_DebugMessage(("ThreadCreate: task_create failed.\n"));
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return t;
}

#endif /* __OS20__ || __OS21__ */


#if defined(__SOLARIS__) || (defined(__LINUX__) && !defined(__KERNEL__))

EMBX_THREAD EMBX_OS_ThreadCreate(void (*thread)(void *), void *param, EMBX_INT priority, const EMBX_CHAR *name)
{
pthread_t *tid;

    EMBX_Info(EMBX_INFO_OS, (">>>>ThreadCreate\n"));

    tid = (pthread_t *)EMBX_OS_MemAlloc(sizeof(pthread_t));

    if(tid != EMBX_INVALID_THREAD)
    {
        if(pthread_create(tid, NULL, (void*(*)(void*))thread, param))
        {
            EMBX_DebugMessage(("ThreadCreate: task_create failed.\n"));

            EMBX_OS_MemFree(tid);
            tid = EMBX_INVALID_THREAD;
        }
    }

    EMBX_Info(EMBX_INFO_OS, ("<<<<ThreadCreate\n"));

    return tid;
}

#endif /* __SOLARIS__ */



EMBX_ERROR EMBX_OS_ThreadDelete(EMBX_THREAD thread)
{
    EMBX_Info(EMBX_INFO_OS, (">>>>EMBX_OS_ThreadDelete\n"));

    if(thread == EMBX_INVALID_THREAD) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS (invalid task)\n"));
        return EMBX_SUCCESS;
    }


#if defined(__OS20__) || defined(__OS21__)

    if(task_wait(&thread, 1, TIMEOUT_INFINITY) != 0)
    {
        EMBX_DebugMessage(("EMBX_OS_ThreadDelete: task_wait failed.\n"));
        return EMBX_SYSTEM_INTERRUPT;        
    }

    task_delete(thread);

#elif defined(__WINCE__) || defined(WIN32)

    if(WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0) {
	EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SYSTEM_INTERRUPT\n"));
        return EMBX_SYSTEM_INTERRUPT;
    }


    CloseHandle(thread);

#elif defined(__SOLARIS__) || (defined(__LINUX__) && !defined(__KERNEL__))

    pthread_join(thread, NULL);
    EMBX_OS_MemFree(thread);

#elif defined(__LINUX__) && defined(__KERNEL__)

    {
	struct ThreadInfo *t = (struct ThreadInfo *) thread;
	int res;

	res = kthread_stop(t->kthread);
	if (0 != res) {
	    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SYSTEM_INTERRUPT\n"));
	    return EMBX_SYSTEM_INTERRUPT;
	}

	EMBX_OS_MemFree(t);
    }

#else
#error EMBX_OS_ThreadDelete, Unsupported OS
#endif /* __OS21__ */

    EMBX_Info(EMBX_INFO_OS, ("<<<<EMBX_OS_ThreadDelete = EMBX_SUCCESS\n"));
    return EMBX_SUCCESS;
}
