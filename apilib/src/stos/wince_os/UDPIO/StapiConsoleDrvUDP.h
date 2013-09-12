#ifndef __STAPICONSOLEDRVUDP__
#define __STAPICONSOLEDRVUDP__
#ifdef __cplusplus
extern "C"
{
#endif
#include "StapiConsoleApi.h"
typedef struct _UDPParam
{
	int				hConnectIn;
	unsigned int    iAdressSource;
	unsigned int    iAdressTarget;
	unsigned int	iMulticastMask;
	int				bEchoState;
	unsigned short	iPort;
	void*			hThreadInitialized;
	void *			TblHandle[5];
	unsigned long   TblId[5];
	int				gExitThread ;
	char 			pWelcomeMessage[256];
	
}T_UDPPARAM;

int InitStapiConsoleUDP(T_STAPICONSOLEAPI *pHandle,char *pConfig);
int TermStapiConsoleUDP(T_STAPICONSOLEAPI *pHandle);



#ifdef __cplusplus

}
#endif
#endif