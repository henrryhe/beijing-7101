#ifndef __WINCE_PROFILER__
#define  __WINCE_PROFILER__

#ifdef __cplusplus
extern "C"
{
#endif

// structure exported in anycase
typedef enum
{
    P_SEMAPHORE_L = 0,
	P_SEMAPHORE_T,
	P_SEMAPHORE_S,
	P_ENTERCRIT,
	P_LEAVECRIT,
    P_TASK_WAIT,
    P_TASK_DELAY,
    P_WAIT_OBJECT,
    P_READ_QUEUE,
    P_WRITE_QUEUE,
    P_MUTEX_LOCK,
	P_IT_LOCK,
	P_IT_UNLOCK,
	P_IT_LOCK_RE,
	P_IT_UNLOCK_RE,
    P_NUM_CALLS
} P_SystemCall;

// default
#define P_ADDSYSTEMCALL
#define P_ADDSEMAPHORE
#define P_ADDTHREAD

// uncomment to avoid heavy system call profile, keeping only task benchmark (no performance loss)
//#define SYSTEMCALL_NOPROFILE	

#ifdef BENCHMARK_WINCESTAPI 
#include "profiler.h"		// for MONTE_CARLO PROFILING

typedef struct
{
    DWORD threadId;
    DWORD parentId;
    char name[100];
    int systemCalls[P_NUM_CALLS];
	int UserTime;
} P_ThreadInfo;

#define MAX_THREADS 100

void P_Init(void);  // create an empty list of threads
void P_Start(void); // start profiling
void P_Stop();      // stop profiling
void P_Print();     // display stats

void P_addThread(DWORD threadId, DWORD parentId, const char* name);
void P_AddSystemCall(P_SystemCall call, int psem); // adds a system call for current thread
void P_AddSemaphore(DWORD SemId, const char* name);

#undef P_ADDTHREAD
#define P_ADDTHREAD(a,b,c)	 P_addThread(a,b,c)

typedef struct
{
    DWORD SemId;
	DWORD threadId;
    char name[100];
    int systemCalls[P_NUM_CALLS];
} P_SemInfo;

#define MAX_SEMAPHORE 50
int pertinentcall;
int EvaluateTimeInSystemCall;
int EvaluateTimeInTaskScheduling;

#ifndef SYSTEMCALL_NOPROFILE	// else keep default value
#undef P_ADDSYSTEMCALL
#undef P_ADDSEMAPHORE
#define P_ADDSYSTEMCALL(a,b) P_AddSystemCall(a,b)
#define P_ADDSEMAPHORE(a, b) P_AddSemaphore (a, b)
#endif	// SYSTEMCALL_NOPROFILE

#endif //BENCHMARK_WINCESTAPI 

#ifdef __cplusplus
}
#endif


#endif