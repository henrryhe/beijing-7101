/*! Time-stamp: <@(#)WCE_Collection.c   05/04/2005 - 22:41:57   William hennebois>
 *********************************************************************
 *  @file   : WCE_Collection.c
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William hennebois            Date: 05/04/2005
 *
 *  Purpose : Implementation of some function of collection for debug
 *            and ToolBox. This code sould be called only 
 *			  throw MACRO ( disable in release) or in stcommon module
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  05/04/2005 WH : First Revision
 *         06/06/2005 WH : remplacement of _Trace for a call _Msg 
 *
 *********************************************************************
 */
#include "WCE_Collection.h"
#include <stdio.h>
#include <kfuncs.h>

#undef fopen	// use of the real fopen ( path on fly of 

// -------------------------------------------------------------------------------------
//
//
//  Collection of functions mainly used for debug 
//
//
//
//
//

// Enable trace
static int			gTraceEnabled = FALSE;
static unsigned int gDebugLevel = 0;
static char			gMessageLogFileName[MAX_PATH];
static char			gConsoleLogFileName[MAX_PATH];
static int			gbOutputLog;
// Collection 

static LARGE_INTEGER gfreqTimer,gTimerStart,gTimerEnd;


#define INIT_VALUE_SEMAPHORE 1000000000

#ifdef WINCE_PERF_OPTIMIZE
#define WINCE_USE_CRITICAL 1
//#define  WINCE_USE_EVENT

static unsigned int nCptSem = 0;
static unsigned int nCptCrit = 0;
#endif


/******************************************************************
***************** BENCHMARK TOOLS ****************************
******************************************************************/

#ifdef BENCHMARK_WINCESTAPI

// globals
P_ThreadInfo	PG_threads[MAX_THREADS];
P_SemInfo		PG_Sem[MAX_SEMAPHORE];
int				PG_numThreads;
BOOL			PG_profiling;
int				PG_misses;

int P_NumSem;

// initialize profiler. To call before any other profile func
void P_Init(void)
{
    PG_numThreads = 0;
    PG_profiling = FALSE;
    P_ADDTHREAD(GetCurrentThreadId(), 0, "USER");
	P_NumSem=0;
}

// reset and start system call profiling
void P_Start(void)
{
    int i, j;
	FILETIME starttime, lpCreationTime, lpExitTime, lpKernelTime;

    PG_misses = 0;

	for (i = 0; i < MAX_SEMAPHORE; i++)
        for (j = 0; j < P_NUM_CALLS; j++)
            PG_Sem[i].systemCalls[j] = 0;

	for (i = 0; i < PG_numThreads; i++)
	{
        for (j = 0; j < P_NUM_CALLS; j++)
            PG_threads[i].systemCalls[j] = 0;
		// record start usertime
		GetThreadTimes(PG_threads[i].threadId, &lpCreationTime, &lpExitTime, &lpKernelTime, &starttime);
		PG_threads[i].UserTime = starttime.dwLowDateTime;
	}
	
	pertinentcall=0;
	EvaluateTimeInSystemCall=0;
	EvaluateTimeInTaskScheduling=0;
    PG_profiling = TRUE;
}

// stop profiling
void P_Stop()
{
	int i;
	FILETIME StopUserTime, lpCreationTime, lpExitTime, lpKernelTime;
    PG_profiling = FALSE;

	for (i = 0; i < PG_numThreads; i++)
	{
       // record stop usertime
		GetThreadTimes(PG_threads[i].threadId, &lpCreationTime, &lpExitTime, &lpKernelTime, &StopUserTime);
		 PG_threads[i].UserTime = StopUserTime.dwLowDateTime - PG_threads[i].UserTime;
	}
/*	printf ("Blocked Critical %d \nEvaluateTimeInSystemCall %d tic %dms \nEvaluateTimeInTaskScheduling %d tic %dms \n", 
		pertinentcall, EvaluateTimeInSystemCall, EvaluateTimeInSystemCall*1000/time_ticks_per_sec(), EvaluateTimeInTaskScheduling, EvaluateTimeInTaskScheduling*1000/time_ticks_per_sec() );*/
}


// printf results of profiling
void P_Print()
{
    int i;

    printf("\n\nJLX profiler\n");
    printf(    "============\n");

    printf("Total number of monitored threads: %d\n", PG_numThreads);
    printf("Total number of system calls in unmonitored threads: %d\n\n", PG_misses);

    for (i = 0; i < PG_numThreads; i++)
    {
        P_ThreadInfo* info = PG_threads + i;
        printf("%s\t%x\t%x\t%d\t%d \n", info->name, info->threadId, info->parentId, CeGetThreadPriority(info->threadId), info->UserTime/10000 );
    }

#ifndef SYSTEMCALL_NOPROFILE	
	printf("\nThreadName\tid\tparent\tSemL\tSemW\tSemS\tTskW\tTskD\tWaitObj\tECrit\tLCrit\tMtxLck\tITLck\tITunLck\tITLck_RE\tITunLck_RE\n");
    for (i = 0; i < PG_numThreads; i++)
    {
        P_ThreadInfo* info = PG_threads + i;

		printf("%s\t0x%08x\t0x%08x\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", 
			info->name, info->threadId, info->parentId,
			info->systemCalls[P_SEMAPHORE_L], 
			info->systemCalls[P_SEMAPHORE_T], 
			info->systemCalls[P_SEMAPHORE_S], 
			info->systemCalls[P_TASK_WAIT],
			info->systemCalls[P_TASK_DELAY],
			info->systemCalls[P_WAIT_OBJECT],
			info->systemCalls[P_ENTERCRIT],
			info->systemCalls[P_LEAVECRIT],
			info->systemCalls[P_MUTEX_LOCK],
			info->systemCalls[P_IT_LOCK],
			info->systemCalls[P_IT_UNLOCK],
			info->systemCalls[P_IT_LOCK_RE],
			info->systemCalls[P_IT_UNLOCK_RE]);
    }

	printf ("\nSEMAPHORE PROFILE\n");
	printf("Name\tThreadId\tSemL\tSemT\tSemS\tECrit\tLCrit\tLocked\n");
	for (i = 0; i < P_NumSem; i++)
    {
		printf ("%s\t0x%08x\t%d\t%d\t%d\t%d\t%d\t%d\n", PG_Sem[i].name, PG_Sem[i].SemId,
					PG_Sem[i].systemCalls[P_SEMAPHORE_L],
						PG_Sem[i].systemCalls[P_SEMAPHORE_T],
						PG_Sem[i].systemCalls[P_SEMAPHORE_S],
						PG_Sem[i].systemCalls[P_ENTERCRIT],
						PG_Sem[i].systemCalls[P_LEAVECRIT],
						PG_Sem[i].systemCalls[P_MUTEX_LOCK] ); // number of locked Section
	}
#endif
}

// record thread ID to identify call on it
void P_addThread(DWORD threadId, DWORD parentId, const char* name)
{
    int j;

    if (PG_numThreads >= MAX_THREADS)
    {
        printf("P_addThread: max number of threads %d reached !\n", MAX_THREADS);
        DebugBreak();
        exit(-1);
    }
    if (PG_profiling)
    {
        printf("P_addThread: cannot add thread while profiling\n");
        DebugBreak();
        exit(-1);
    }
    PG_threads[PG_numThreads].threadId = threadId;
    PG_threads[PG_numThreads].parentId = parentId;
    strcpy(PG_threads[PG_numThreads].name, name);
    ++PG_numThreads;
    for (j = 0; j < P_NUM_CALLS; j++)
        PG_threads[PG_numThreads].systemCalls[j] = 0;
}

// record Sem id in a tab in order to identify call on it
void P_AddSemaphore(DWORD SemId, const char* name)
{
    int j;

    if (P_NumSem >= MAX_SEMAPHORE)
    {
        printf("P_addSem: max number of threads %d reached !\n", MAX_THREADS);
        DebugBreak();
        exit(-1);
    }

	PG_Sem[P_NumSem].SemId = SemId;
    PG_Sem[P_NumSem].threadId = GetCurrentThreadId();
    strcpy(PG_Sem[P_NumSem].name, name);
    ++P_NumSem;
    for (j = 0; j < P_NUM_CALLS; j++)
        PG_Sem[P_NumSem].systemCalls[j] = 0;
}

// call counter for each recorded sem
void P_SemProfile(P_SystemCall call, int psem)
{
	int idx;
	// find psem index in the record tab
	for (idx=0; idx<P_NumSem+1;idx++)
	{
		if (PG_Sem[idx].SemId==psem)
			break;
	}
	// increment call counter
	PG_Sem[idx].systemCalls[call]++;

	// process critical section
	if (call== P_ENTERCRIT)
	{
		// la section critique est-t-elle dij` prise ?
		if (((CRITICAL_SECTION*)((semaphore_t*)psem)->hHandle_opaque)->OwnerThread !=0)
		//	if (TryEnterCriticalSection((CRITICAL_SECTION*)((semaphore_t*)psem)->hHandle_opaque) !=0)
			PG_Sem[idx].systemCalls[P_MUTEX_LOCK]++;
	}

	// All non-recorded sem in PG_Sem[P_NumSem]
}

// count sytem call
void P_AddSystemCall(P_SystemCall call, int psem)
{
    int i;
    DWORD threadId = GetCurrentThreadId();

    if (!PG_profiling)
        return;

    for (i = 0; i < PG_numThreads; i++)
        if (PG_threads[i].threadId == threadId)
            break;
    if (i < PG_numThreads)
        PG_threads[i].systemCalls[call] ++;
    else
        PG_misses ++;

	if (call<P_TASK_WAIT)
		P_SemProfile(call, psem);
}

#endif  // BENCHMARK_WINCESTAPI
  
/******************************************************************
***************** END PROFILING TOOLS ****************************
******************************************************************/

////////////////////// START DEBUG MODE ////////////////////////////////////////////
///    FUNCTIONS ARE ONLY PRESENT IN  DEBUG MODE


#if DEBUG ==1
/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param none
 *
 * @return void  : 
 */
void _WCE_Start_Measure()
{
	QueryPerformanceFrequency(&gfreqTimer);
	QueryPerformanceCounter(&gTimerStart);
}


/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param none
 *
 * @return void  : 
 */
void _WCE_End_Measure()
{

	QueryPerformanceCounter(&gTimerEnd);

}


/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param none
 *
 * @return void  : 
 */
void _WCE_Print_Measure()
{
	double fsec;
	fsec = ((double)gTimerEnd.QuadPart - gTimerStart.QuadPart)/gfreqTimer.QuadPart;
	WCE_TRACE("delay of timer %f  Seconds\n",fsec);
}


/*!-------------------------------------------------------------------------------------
 * print system error string from a code 
 *
 * @param dwStatus : 
 *
 * @return void  : 
 */
void _WCE_PrintSystemError(int dwStatus)
{ 
	WCHAR buffer[256]; 
	char  bufferA[256];

	if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwStatus, 0, buffer, sizeof(buffer), NULL) == 0) {
		wcscpy(buffer,L"<FormatMessage() failed>");
	}
	wcstombs(bufferA,buffer,   512);
	_WCE_Trace("--- SysError GetLastError =(%07X) %s \n",dwStatus,bufferA);
}

void _WCE_DumpMemory(unsigned char *pData,int size,int sizeline)
{
	int cptLine = 0;
	wchar_t tbuffer[1024];
	if(sizeline==0) sizeline=16;

	while(size)
	{

		if(cptLine==0) 
		{
			swprintf(tbuffer,TEXT("%010X : %02X"),pData,*pData);
			OutputDebugString(tbuffer);

		}
		else
		{
			swprintf(tbuffer,TEXT(",%02X"),*pData);
			OutputDebugString(tbuffer);

		}
		cptLine++;
		if(cptLine > sizeline)
		{

			swprintf(tbuffer,TEXT("\r\n"));
			OutputDebugString(tbuffer);

			cptLine = 0;
		}
		size--;
		pData++;
		
	}
	OutputDebugString(TEXT("\r\n"));

}





/*!-------------------------------------------------------------------------------------
 * Extract the file name from a full qualified path 
 *
 * @param *pFullPath : the full qualified path 
 *
 * @return char  : on  pointer on a the file name
 */
char *_WCE_GetFName(char *pFullPath)
{
	char *pfname = strchr(pFullPath,0);
	if(pfname == 0) return pFullPath;

	while(pfname != pFullPath)
	{
		if(*(pfname-1) == '\\' || *(pfname-1) == '/') break;
		pfname--;
	}
	return pfname;

}



/*!-------------------------------------------------------------------------------------
 * Send a Trace on the Outout debug
 *
 * @param lpszFormat : 
 * @param ... : 
 *
 * @return void  : 
 */

void _WCE_Trace(char* lpszFormat, ...)
{
	int nBuf;
	char szBuffer[1024];
	va_list args;
	va_start(args, lpszFormat);
	// Stop Console here, vsprintf spend to much CPU time
	if (!gTraceEnabled)	return;

	// Warning to the size of the buffer
	nBuf = vsprintf(szBuffer,  lpszFormat, args);
	// was there an error? was the expanded string too long?
	WCE_ASSERT(nBuf >= 0);
	WCE_OutputDebugString(szBuffer);
	va_end(args);

	
}




/*!-------------------------------------------------------------------------------------
 * Print a message on the debug console depending a flag DebugLevel
 *
 * @param flag : 
 * @param lpszFormat : 
 * @param ... : 
 *
 * @return void  : 
 */
void _WCE_Msg(unsigned int flag, char* lpszFormat, ...)
{
	int nBuf;
	char szBuffer[1024];
	va_list args;
	if(!(gDebugLevel & flag)) return;
	va_start(args, lpszFormat);
	// Warning to the size of the buffer
	nBuf = vsprintf(szBuffer,  lpszFormat, args);
	// was there an error? was the expanded string too long?
	WCE_ASSERT(nBuf >= 0);
	strcat(szBuffer,"\r\n");
	WCE_OutputDebugString(szBuffer);
	va_end(args);
}


/*!-------------------------------------------------------------------------------------
 * Set the path of the message debug log file 
 *
 * @param *pFileName : 
 *
 * @return void  : 
 */
void _WCE_SetMessageLogFile(const char *pFileName)
{
	if(pFileName == 0) 
	{
		gMessageLogFileName[0] = 0;
	}
	else
	{
		FILE *pFile = fopen(pFileName,"w");
		if(pFile) 
		{
			fclose(pFile);
		}
		strcpy(gMessageLogFileName,pFileName);
	}
}



/*!-------------------------------------------------------------------------------------
 * enable or disable traces on debug console
 *
 * @param flg : 
 *
 * @return void  : 
 */
void _WCE_SetTraceMode(unsigned int flg)
{
	gTraceEnabled = flg;
	if(flg ) WCE_OutputDebugString("Trace enable\n");
	else	 WCE_OutputDebugString("Trace desable\n");
}


/*!-------------------------------------------------------------------------------------
 * Add a string to a file 
 *
 * @param *pLogName : 
 * @param *pString : 
 *
 * @return void  : 
 */
void _WCE_AddLogFile(const char *pLogName,const char *pString)
{
	FILE *stream = NULL;
	stream = fopen(pLogName,"a+");
	if(stream)
	{
		fprintf(stream,pString);
		fclose(stream);
	}

}



/*!-------------------------------------------------------------------------------------
 * Add a string to the console log file
 *
 * @param *pString : 
 *
 * @return void  : 
 */
void _WCE_AddConsoleLogFile(const char *pString)
{
	if(gConsoleLogFileName[0]) _WCE_AddLogFile(gConsoleLogFileName,pString);

}

/*!-------------------------------------------------------------------------------------
 * Set the path of the console log file 
 *
 * @param *pFileName : 
 *
 * @return void  : 
 */
void _WCE_SetConsoleLogFile(const char *pFileName)
{
	if(pFileName == 0) 
	{
		gConsoleLogFileName[0] = 0;
	}
	else
	{
		FILE *pFile = fopen(pFileName,"w");
		if(pFile) 
		{
			fclose(pFile);
		}
		strcpy(gConsoleLogFileName,pFileName);
	}

}


/*!-------------------------------------------------------------------------------------
 * Output a string on the console or in a log file depanding the mode
 *
 * @param *pString : 
 *
 * @return  : 
 */
void	WCE_OutputDebugString( char *pString)
{
	static WCHAR str[512];
	if(gMessageLogFileName[0]) _WCE_AddLogFile(gMessageLogFileName,pString);
	if (!gTraceEnabled)	return;
	WCE_ASSERT(strlen(pString) < 512);
	mbstowcs(str,pString,   512);
	OutputDebugString(str);

}



/*!-------------------------------------------------------------------------------------
 * sets the outputstring mode 
 *
 * @param f : 0 or &
 *
 * @return void  : 
 */
void WCE_SetOutputDebugStringMode(BOOL f)
{
	gbOutputLog =f;
}

#endif
////////////////////// END DEBUG MODE ////////////////////////////////////////////


/*!-------------------------------------------------------------------------------------
 *  
 *
 *
 * @param tick : value in Ticks 
 *
 * @return int  : value in milliseconds
 */
__int64 _WCE_Tick2Millisecond(__int64 ticks)
{
	__int64 millisec ;
	millisec = (__int64)(((double)ticks * 1000) / time_ticks_per_sec());
	return millisec;
}


/*!-------------------------------------------------------------------------------------
Give back our time slot to other thread of same priority
*/

void WCE_Yield(void )
{
	Sleep(0);
}


/*!-------------------------------------------------------------------------------------
 * Create a mutex
 *
 * @param *psem : 
 * @param *pattr : 
 *
 * @return int  : 1 if ok
 */

int WCE_Semaphore_Create(semaphore_t *psem,int count, BOOL bTimeOut)
{
	WCE_ASSERT(psem);
	
#ifdef WINCE_USE_CRITICAL
	if ((count==1) && !bTimeOut) // critical section
	{
		nCptCrit++;
		psem->hHandle_opaque= memory_allocate(NULL,sizeof(CRITICAL_SECTION));
		WCE_ASSERT(psem->hHandle_opaque);
		psem->eSynchObjType=CRIT_SECT;
		InitializeCriticalSection((CRITICAL_SECTION*)psem->hHandle_opaque);
	}
	else
	{
#endif
#ifdef WINCE_USE_EVENT
		if(count == 0)
		{
			psem->hHandle_opaque =  CreateEvent(0,FALSE,0,0);
			psem->eSynchObjType=WCE_EVENT;
			WCE_ASSERT(psem->hHandle_opaque);
		}
		else
		{
			// count can be very high (e.g. unused semaphore for VSync)		
			psem->hHandle_opaque = CreateSemaphore(NULL,count,INIT_VALUE_SEMAPHORE,NULL);		
			psem->eSynchObjType=SEMAPHORE;
			WCE_ASSERT(psem->hHandle_opaque);
		}
	
#else
		// count can be very high (e.g. unused semaphore for VSync)		
		psem->hHandle_opaque = CreateSemaphore(NULL,count,INIT_VALUE_SEMAPHORE,NULL);		
		WCE_ASSERT(psem->hHandle_opaque);
#endif
#ifdef WINCE_USE_CRITICAL
	}
#endif	


	return psem->hHandle_opaque != NULL;
}


/*!-------------------------------------------------------------------------------------
 * Destroy a mutex
 *
 * @param *psem : 
 *
 * @return int  : 1 if ok
 */
int WCE_Semaphore_Destroy(semaphore_t *psem)
{
	int iRet;
	WCE_ASSERT(psem);
	WCE_ASSERT(psem->hHandle_opaque);

	if (psem->eSynchObjType==CRIT_SECT)
	{
		DeleteCriticalSection(psem->hHandle_opaque);
		iRet=1; // success
	}
	else
	{
		iRet= CloseHandle(psem->hHandle_opaque);
		WCE_ASSERT(iRet != 0);
	}
	return iRet;
}


/*!-------------------------------------------------------------------------------------
 * Returns the count of the sema
 *
 * @param *psem : 
 *
 * @return int  : 
 */
int WCE_Semaphore_Count(semaphore_t *psem)
{
	ULONG count = 0;
	DWORD ret;
	WCE_ASSERT(psem);
	WCE_ASSERT(psem->hHandle_opaque);

	if (psem->eSynchObjType!= SEMAPHORE)
		return 0;

	while(1)
	{
		if(WaitForSingleObject(psem->hHandle_opaque,0) == WAIT_TIMEOUT) break;
		count++;
	}
	ReleaseSemaphore(psem->hHandle_opaque,count,0);
	return (int)count;
}







/*!-------------------------------------------------------------------------------------
 * Lock the mutex ( wait until the mutex is released)
 *
 * @param *psem : 
 *
 * @return int  : T_ETIMEDOUT
 */
int WCE_Semaphore_Lock(semaphore_t *psem)
{
	int iRet;
	WCE_ASSERT(psem > 0);
	WCE_ASSERT(psem->hHandle_opaque);

	if (psem->eSynchObjType==CRIT_SECT)
	{
		P_ADDSYSTEMCALL(P_ENTERCRIT, psem);
		EnterCriticalSection((CRITICAL_SECTION*)psem->hHandle_opaque);
	//	return T_SIGNALED;
	}
	else
	{
		P_ADDSYSTEMCALL(P_SEMAPHORE_L, psem);		
		iRet = WaitForSingleObject(psem->hHandle_opaque,INFINITE);
		if(iRet == WAIT_ABANDONED) return T_ABANDONED;
	}
	return T_SIGNALED;
}




/*!-------------------------------------------------------------------------------------
 * Lock semaphore with time out
 *
 * @param *psem : 
 *
 * @return int  : 
 */
int WCE_Semaphore_TimedLock(semaphore_t *psem, DWORD timeout )
{
	int iRet=1;
	int sem;
	WCE_ASSERT(psem);
	WCE_ASSERT(psem->hHandle_opaque);
	WCE_ASSERT(psem->eSynchObjType!=CRIT_SECT);
	P_ADDSYSTEMCALL(P_SEMAPHORE_T, psem);
	iRet = WaitForSingleObject(psem->hHandle_opaque,timeout);
	if(iRet == WAIT_TIMEOUT) 	return T_ETIMEDOUT;
	if(iRet == WAIT_ABANDONED) return T_ABANDONED;
	return T_SIGNALED;
}


 


/*!-------------------------------------------------------------------------------------
 * Unlock sur mutex ( Signaled)
 *
 * @param *psem : 
 *
 * @return int  : 1 if ok
 */
int WCE_Semaphore_Unlock(semaphore_t *psem)
{
	LONG oldVal;
	int sem;
	int iRet=1;
	WCE_ASSERT(psem);
	WCE_ASSERT(psem->hHandle_opaque);

	if (psem->eSynchObjType==CRIT_SECT)
	{
		P_ADDSYSTEMCALL(P_LEAVECRIT, psem);	
		LeaveCriticalSection((CRITICAL_SECTION*)psem->hHandle_opaque);
		iRet=1; // success
	}

	if (psem->eSynchObjType==SEMAPHORE)
	{
		P_ADDSYSTEMCALL(P_SEMAPHORE_S, psem);				
		iRet = ReleaseSemaphore(psem->hHandle_opaque,1,&oldVal);		
		WCE_ASSERT(iRet != 0);	
	}
	if (psem->eSynchObjType==WCE_EVENT)
	{
		P_ADDSYSTEMCALL(P_SEMAPHORE_S, psem);		
		iRet = SetEvent(psem->hHandle_opaque);
		WCE_ASSERT(iRet != 0);
	}
	return iRet;
}

/*!-------------------------------------------------------------------------------------
 * Signal the semaphore
 *
 * @param *psem : 
 *
 * @return  : 
 */
int	WCE_SemaphoreSignal(semaphore_t *psem)
{
	WCE_ASSERT(psem);
	WCE_ASSERT(psem->hHandle_opaque);

	// TODO: Pas clair  a verifier
	SetEvent(psem->hHandle_opaque);
	return 1;
}


void MyLeaveCriticalSection(CRITICAL_SECTION *p)
{
	LeaveCriticalSection(p);

}
void MyEnterCriticalSection(CRITICAL_SECTION *p)
{
	EnterCriticalSection(p);
}



/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param *psem : 
 * @param count : 
 *
 * @return  : 
 */
int		WCE_Mutex_Create(mutex_t *pmutex)
{
	WCE_ASSERT(pmutex);
	InitializeCriticalSection(&pmutex->hHandle_opaque);
	return 1;
}


/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param *psem : 
 *
 * @return  : 
 */
int		WCE_Mutex_Destroy(mutex_t *pmutex)
{
	WCE_ASSERT(pmutex);
	DeleteCriticalSection(&pmutex->hHandle_opaque);
	return 1;

}


/*!-------------------------------------------------------------------------------------
 * <Detailed description of the method>
 *
 * @param *psem : 
 *
 * @return  : 
 */
int		WCE_Mutex_Lock(mutex_t *pMutex)
{
	WCE_ASSERT(pMutex);
    P_ADDSYSTEMCALL(P_MUTEX_LOCK, 0);
	EnterCriticalSection(&pMutex->hHandle_opaque);
	return 1;
}

/*!-------------------------------------------------------------------------------------
 * 
 *
 * @param *psem : 
 *
 * @return  : 
 */
int		WCE_Mutex_Release(mutex_t *pmutex)
{
	WCE_ASSERT(pmutex);
	LeaveCriticalSection(&pmutex->hHandle_opaque);
	return 1;

}




void _WCE_Printf(char * lpszFormat, ...)
{
	int nBuf;
	char szBuffer[1024];
	static WCHAR str[512];
	va_list args;
	va_start(args, lpszFormat);
	// Warning to the size of the buffer
	nBuf = vsprintf(szBuffer,  lpszFormat, args);
	// was there an error? was the expanded string too long?
	WCE_ASSERT(nBuf >= 0);
	WCE_ASSERT(strlen(szBuffer) < 512);
	mbstowcs(str,szBuffer,   512);
	OutputDebugString(str);
	va_end(args);
}

/* ST-CD: added STTBX_Report traces */
void _WCE_Report_Level(int level, char * lpszFormat, ...)
{
	int nBuf;
	char szBuffer[1024];
	static WCHAR str[512];
	va_list args;
	va_start(args, lpszFormat);
	// Warning to the size of the buffer
	nBuf = vsprintf(szBuffer,  lpszFormat, args);
	// was there an error? was the expanded string too long?
	WCE_ASSERT(nBuf >= 0);
	WCE_ASSERT(strlen(szBuffer) < 512);
	mbstowcs(str,szBuffer,   512);
	OutputDebugString(str);
	va_end(args);
}

/*!-------------------------------------------------------------------------------------
 * Set the level of information of debugs see enum DebugLevel flags
 *
 * @param flags : 
 *
 * @return void  : 
 */
void _WCE_SetMsgLevel(unsigned int flags)
{
	gDebugLevel = flags;
}

#ifdef WINCE_DEFINE_TRACE
/* =======================================================================
   Name:        TraceState
   Description: Function used to send on the UART the value of a multiple
                variable, formatted for the Visual Debugger utility.
   Usage        TraceState ("<variable_name>","<state_name>");
   Examples :   TraceState ("VSync","Top");
                TraceState ("VSync","Bot");
   ======================================================================== */
void TraceState(const char *VariableName_p, const char *format,...) 
{
    char Message[256];

    va_list         list;
    va_start (list, format);
    vsprintf ( Message , format, list);
    va_end (list);
    
    printf("Stt:%s:%s\r\n", VariableName_p, Message);
}

/* =======================================================================
   Name:        TraceMessage
   Description: Function used to send on the UART a characters string, 
                formatted for the Visual Debugger utility.
   Usage        TraceMessage("<variable_name>","<message>");
   Examples     TraceMessage ("Injection","Start");
                TraceMessage ("Injection","Stop");
   ======================================================================== */
void TraceMessage(const char *VariableName_p, const char *format,...)
{   
    char Message[256];

    va_list         list;
    va_start (list, format);
    vsprintf ( Message , format, list);
    va_end (list);

    printf("Msg:%s:%s\r\n", VariableName_p, Message);
}
#endif
