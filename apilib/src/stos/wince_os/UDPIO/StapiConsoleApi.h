#ifndef __STAPICONSOLEAPI__
#define __STAPICONSOLEAPI__
#ifdef __cplusplus
extern "C"
{
#endif
#include "stdarg.h"
#include "stdio.h"

#if 0
#ifndef TRACE
#define TRACE _Trace
#endif

__inline void _Trace(char* lpszFormat, ...)
{
	int nBuf;
	char szBuffer[1024];
	va_list args;
	va_start(args, lpszFormat);

	
	// Warning to the size of the buffer
	nBuf = vsprintf(szBuffer,  lpszFormat, args);

	OutputDebugString(szBuffer);
	va_end(args);

	
}

#endif

typedef  void  ( *STAPICONSOLECB)(void *pLparam,unsigned long addr );

#define SIZE_BUFFER_IN  512
#define MAX_LBUF_SIZE	1024

typedef struct  
{
	void *			ghCharAvalaible;
	void *			ghBufferFull;
	unsigned int    SizeOQP;
	unsigned char  *pBufferWrite;
	unsigned char  *pBufferRead;
	unsigned char  *pBufferEnd;
	unsigned char tBufferIn[SIZE_BUFFER_IN];
	CRITICAL_SECTION sCR;

}T_BUFFER_LIST;


typedef int  (*PROCTERM)(struct t_stapihandle *pHandle);
typedef int  (*PROCINIT)(struct t_stapihandle *pHandle,char *pConfig);


typedef struct t_stapihandle 
{
	T_BUFFER_LIST	HandleGetC;
	T_BUFFER_LIST	HandlePutC;
	STAPICONSOLECB  gpCallBack;
	void *			gpLparamCB;
	int				gServer;
	PROCTERM		m_ProcStapiTerm;
	PROCINIT		m_ProcStapiInit;
	char			*pConfig;
	void			*pDrvParam;

}T_STAPICONSOLEAPI;


enum {DRVFILE,DRVUDP};;

int   StapiGetC(T_STAPICONSOLEAPI *pHandle);
int   StapiPutC(T_STAPICONSOLEAPI *pHandle,int theChar);
int	  StapiPutS(T_STAPICONSOLEAPI *pHandle,const char *pString);
int	  StapiGetS(T_STAPICONSOLEAPI *pHandle,char* str, int size);
int	  StapiPrintf(T_STAPICONSOLEAPI *pHandle,const char * format, ...);

void  SetStapiCallBackPutC(T_STAPICONSOLEAPI *pHandle,STAPICONSOLECB pCB,void *pLparam);

void  TermStapiConsole(T_STAPICONSOLEAPI *pHandle);
int   InitStapiConsole(T_STAPICONSOLEAPI *pHandle);

void AddToBuffer(T_BUFFER_LIST *pHandle,int theChar);
int  ReadFromBuffer(T_BUFFER_LIST *pHandle);
void InitBuffer(T_BUFFER_LIST *pHandle);
void TermBuffer(T_BUFFER_LIST *pHandle);
int  SetStapiConsoleDriver(T_STAPICONSOLEAPI *pHandle,int index,char *pConfig);


#ifdef __cplusplus

}
#endif
#endif