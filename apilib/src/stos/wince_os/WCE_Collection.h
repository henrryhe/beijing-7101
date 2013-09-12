/*! Time-stamp: <@(#)WCE_Collection.h   05/04/2005 - 22:43:37   William Hennebois>
 *********************************************************************
 *  @file   : WCE_Collection.h
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William Hennebois            Date: 05/04/2005
 *
 *  Purpose : Implementation of some function of collection for debug
 *            and ToolBox
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  05/04/2005 WH : First Revision
 *
 *********************************************************************
 */
#ifndef __WINCE_COLLECTION__
#define  __WINCE_COLLECTION__
// used only in debug mode

#define WINCE_DEFINE_TRACE
 
#ifdef WINCE_DEFINE_TRACE
void TraceState(const char *VariableName_p, const char *format,...);
void TraceMessage(const char *VariableName_p, const char *format,...);
#endif

char *_WCE_GetFName(char *pFullPath);
void _WCE_Trace(char * lpszFormat, ...);
void _WCE_Printf(char * lpszFormat, ...);

/* ST-CD: added STTBX_Report traces */
void _WCE_Report_Level(int level, char * lpszFormat, ...);

void _WCE_Msg(unsigned int flg,char * lpszFormat, ...);
void _WCE_SetMsgLevel(unsigned int flg);  // select a debug zone
void _WCE_SetTraceMode(unsigned int flg); // enable or disable all console's trace 
void _WCE_SetMessageLogFile(const char *pFileName);
void _WCE_SetConsoleLogFile(const char *pFileName);

// string on console debugs

#define		 STAPI_LOG_FILE "/Release/Stapi.log"
void		 WCE_OutputDebugString( char *pString);
void		 WCE_SetOutputDebugStringMode(BOOL f);

// timing fonction for debug
void _WCE_Start_Measure();
void _WCE_End_Measure();
void _WCE_Print_Measure();
void _WCE_PrintSystemError(int dwStatus);
__int64 _WCE_Tick2Millisecond(__int64 ticks);
void _WCE_DumpMemory(unsigned char *pData,int size,int sizeline);

// Break the debugger ( only on x86) to be change for sh4 
#define	__gbreak() DebugBreak()

typedef enum {MDL_CALL=1,MDL_MSG=2,MDL_OSCALL=4,MDL_WARNING=8,MDL_ASSERT=16,MDL_FORDEBUG=32,MDL_MEMORY=64,MDL_PRIORITY=128} DebugLevel;

#if DEBUG == 1
#define WCE_MSG				  _WCE_Msg
#define WCE_TRACE             _WCE_Trace



// Will be removed after Porting 
#define WCE_NOT_IMPLEMENTED() {\
		_WCE_Msg(MDL_WARNING,"NOT IMPLEMENTED In File %s(%d)\n",_WCE_GetFName(__FILE__),__LINE__);\
		__gbreak();\
		}



#define WCE_NOT_TESTED(){\
		_WCE_Msg(MDL_WARNING,"NOT TESTED In File %s(%d)\n",_WCE_GetFName(__FILE__),__LINE__);\
		}


#define WCE_WARNING_IMPLEMENTATION(p) {\
		_WCE_Msg(MDL_WARNING,"WARNING %s In File %s(%d)\n",(p),_WCE_GetFName(__FILE__),__LINE__);\
		}

#define WCE_ERROR(p) {\
		_WCE_Msg(MDL_ASSERT,"%s%s(%d)\n",(p),_WCE_GetFName(__FILE__),__LINE__);\
		__gbreak();\
		}


#define WCE_VERIFY(condition) \
		{\
			if((condition) == 0)\
			{\
				int dwStatus = GetLastError(); \
				_WCE_Msg(MDL_ASSERT,"Function Fails   In File %s(%d)\n",_WCE_GetFName(__FILE__),__LINE__);\
				_WCE_PrintSystemError(dwStatus); \
				__gbreak();\
			}\
		}

#define WCE_ASSERT(condition) \
		{\
			if((condition) == 0)\
			{\
				_WCE_Msg(MDL_ASSERT,"Condition Fails   In File %s(%d)\n",_WCE_GetFName(__FILE__),__LINE__);\
				__gbreak();\
			}\
		}


#else
// remove heavy code in the release mode

#define WCE_TRACE        1 ? (void)0 : _WCE_Trace
#define WCE_MSG			 1 ? (void)0 : _WCE_Msg
#define WCE_NOT_IMPLEMENTED()
#define WCE_NOT_TESTED()
#define WCE_WARNING_IMPLEMENTATION(a)
#define WCE_VERIFY(a) (a)
#define WCE_ASSERT(a)
#define WCE_ERROR(a)

#endif

enum{T_EBUSY=1,T_ETIMEDOUT,T_SIGNALED,T_ABANDONED};

//Could be define in stddefs.h for STOS compatibility.
#ifndef WINCE_SEMAPHORE_T
#define WINCE_SEMAPHORE_T
//#ifndef semaphore_t
typedef enum 
{
	SEMAPHORE,
	CRIT_SECT,
	WCE_EVENT
} WINCE_SYNCH_OBJECT_TYPE;

//! It's the handle on a semaphore OS21 we map on a semaphore WINCE
//! the handle OS21 semaphore_t becomes a struct with all information 
//! used for a windows CE sem, off course we use an opaque types
//! to avoid the include of windows.h in all STAPI module
//! See semaphore_init_fifo,etc...

typedef struct	_semaphore_t 
{
	void							*hHandle_opaque;				   // the Semaphore handle (HANDLE)
	WINCE_SYNCH_OBJECT_TYPE			eSynchObjType;
	int								dbgcount;
	long							   _SemCount;
	
}semaphore_t;
#endif

#ifndef WINCE_MUTEX_T
#define WINCE_MUTEX_T
typedef struct	_mutex_t 
{
	CRITICAL_SECTION hHandle_opaque;							// the mutex handle (HANDLE)
}mutex_t;
#endif

void	WCE_Yield();
int		WCE_Semaphore_Destroy(semaphore_t *psem);
int		WCE_Semaphore_Lock(semaphore_t *psem);
int		WCE_Semaphore_TimedLock(semaphore_t *psem, DWORD timeout);
int		WCE_Semaphore_Unlock(semaphore_t *psem);
int		WCE_SemaphoreSignal(semaphore_t *psem);
int		WCE_Semaphore_Count(semaphore_t *psem);
int		WCE_Mutex_Create(mutex_t *psem);
int		WCE_Mutex_Destroy(mutex_t *psem);
int		WCE_Mutex_Lock(mutex_t *psem);
int		WCE_Mutex_Release(mutex_t *psem);


// uncomment def BENCHMARK_WINCESTAPI to allow MONTE_CARLO and specific WINCE emulator profiling 
//#define BENCHMARK_WINCESTAPI
#include "Wce_Stapi_Bench.h"

#endif
