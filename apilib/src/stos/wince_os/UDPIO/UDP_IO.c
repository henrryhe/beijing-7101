#include <stdlib.h>
#include "stapiconsoleAPI.h"

#define AUTOCLEAR_MESSAGE "Welcome to TestTool UDP Console"


#ifndef WINCE_IPCONSOLEHOST
#define WINCE_IPCONSOLEHOST "127.0.0.1"
#if WINCE_MASTER != 1  && WCE_USE_CONSOLE == 2
	#pragma message("------------------------------------------------------------------------") 
	#pragma message("Warning WINCE_IPCONSOLEHOST  variable is not set, UDP CONSOLE Will fail") 
	#pragma message("------------------------------------------------------------------------") 
#endif
#endif 

BOOL gbUdpIntialized = FALSE;

T_STAPICONSOLEAPI gUDPHandle;

BOOL	Wce_UDPIO_Init(void)
{
	char tBuffer[200];
	sprintf(tBuffer,",%s,,,,%s\r\r\r",WINCE_IPCONSOLEHOST,AUTOCLEAR_MESSAGE);

	SetStapiConsoleDriver(&gUDPHandle,DRVUDP,tBuffer);
	if(InitStapiConsole(&gUDPHandle) != -1)
	{
		gbUdpIntialized = TRUE;
		return TRUE;
	}

	return FALSE;
}
void	Wce_UDPIO_Term(void)
{
	TermStapiConsole(&gUDPHandle);
	gbUdpIntialized = FALSE;

}
int	Wce_UDPIO_Gets(char* str, int size)
{
	if(!gbUdpIntialized) return -1;
	return StapiGetS(&gUDPHandle,str,size);
}
int	Wce_UDPIO_Getchar(BOOL blocking)
{
	if(!gbUdpIntialized) return 0;

	if(!blocking) 
	{
		if(gUDPHandle.HandleGetC.SizeOQP == 0) return  0;

	}
	return StapiGetC(&gUDPHandle);
}
int	Wce_UDPIO_Printf(const char * format, ...)
{

    char tbuffer[1024];
	va_list arglist;
	HRESULT hr = S_OK;
	int size ;
	if(!gbUdpIntialized) return 0;
	va_start(arglist, format);
    // Process the string and dump it to the debug serial port
	size = _vsnprintf(&tbuffer[0], sizeof(tbuffer)-1, format, arglist);
	StapiPutS(&gUDPHandle,tbuffer);
	return size;
}


