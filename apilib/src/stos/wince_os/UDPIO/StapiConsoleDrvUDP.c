#include "Windows.h"
#include "StapiConsoleDrvUDP.h"
#include "stdio.h"

#define DEFAULT_PORT	2046
#define STAPI_CE_PRORITY 128

#ifdef UNDER_CE
#include "WinSock2.h"
#pragma comment(lib,"ws2.lib")
#else
#pragma comment(lib,"ws2_32.lib")

#endif




static char *GetString(char *pString, char *pBuffer,int len)
{
	while(*pString && isspace(*pString))pString++;
	while(*pString && len-- )
	{
		if(*pString == ',') 
		{
			pString++;
			break;
		}
		*pBuffer++ = *pString++;

	}
	*pBuffer = 0;
	return pString;

}

// Syntax
// Adress Source , AdressTarget, Port , Multicast Mask , echo
static void ParseConfig(T_STAPICONSOLEAPI *pHandle,char *pConfig)
{
	char buffer[300];
	T_UDPPARAM *pParam = (T_UDPPARAM *)pHandle->pDrvParam;
	pConfig = GetString(pConfig,buffer,sizeof(buffer));
	// Source
	if(strlen(buffer))
	{
		pParam->iAdressSource = inet_addr(buffer);
	}

	pConfig = GetString(pConfig,buffer,sizeof(buffer));
	if(strlen(buffer))
	{
		pParam->iAdressTarget = inet_addr(buffer);
	}

	pConfig = GetString(pConfig,buffer,sizeof(buffer));
	if(strlen(buffer))
	{
		pParam->iPort = atoi(buffer);
	}

	pConfig = GetString(pConfig,buffer,sizeof(buffer));
	if(strlen(buffer))
	{
		pParam->iMulticastMask = inet_addr(buffer);
	}

	pConfig = GetString(pConfig,buffer,sizeof(buffer));
	if(strlen(buffer))
	{
		pParam->bEchoState = atoi(buffer);
	}

	pConfig = GetString(pConfig,pParam->pWelcomeMessage,sizeof(pParam->pWelcomeMessage));


}
//____________________________________________________________
static int sok_setup (int sok)
{
    int ret; struct timeval t;

    t.tv_sec = 0;
    t.tv_usec= 50000;
    ret = setsockopt (sok, SOL_SOCKET, SO_RCVTIMEO, (const char FAR *)&t, sizeof(t));
    if (ret == -1) return -1;

    ret = 1;
    ret = setsockopt (sok, SOL_SOCKET, SO_DONTROUTE, (const char FAR *)&ret, sizeof(int));
    if (ret == -1) return -1;

    ret = 1;
    ret = setsockopt (sok, SOL_SOCKET, SO_BROADCAST, (const char FAR *)&ret, sizeof(int));
    if (ret == -1) return -1;
/*
    ret = 1;
    ret = setsockopt (sok, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&ret, sizeof(int));
    if (ret == -1) return -1;
*/

	return TRUE;
}

//____________________________________________________________
static int sok_addrsetup (struct sockaddr_in * dst,int kPortNum,int kBroacastMask )
{
    char host[256];
    struct hostent * info;

    dst->sin_family = AF_INET;
    dst->sin_port = htons(kPortNum);

    if (gethostname(host, 255) == -1) {
    	
    	return 0;
    }

    if (( dst->sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
        info = gethostbyname(host);
        if (!info) {
		
			return 0;
		}
        dst->sin_addr.s_addr = *(long *)info->h_addr;
    }
    dst->sin_addr.s_addr |= kBroacastMask;
	return 1;
}

//____________________________________________________________

static int sok_send (int sok, struct sockaddr_in * dst,unsigned char *pData,int size)
{
	
	int n;
    n = sendto(sok,(const char FAR *) pData,size, 0, (struct sockaddr *)dst, sizeof(*dst));
    if (n == -1) return -1;
	return n;
	
}
//____________________________________________________________

static int sok_init(struct in_addr adr,int iPort,int flg)
{
	 int sok = -1;
	 int err ;

    struct sockaddr_in in = { 0 };
	WORD wVersionRequested;
	WSADATA wsaData;
    CRITICAL_SECTION gCR;
	InitializeCriticalSection(&gCR);
	EnterCriticalSection(&gCR);


	wVersionRequested = MAKEWORD( 2, 2 );
	 err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return -1;
	}

    sok = socket (AF_INET, SOCK_DGRAM, 0);
    if (sok != -1) 
	{
   		
		memset(&in.sin_zero,0, sizeof(in.sin_zero)); 			/* zero the rest of the struct */
		in.sin_family = AF_INET; 			/* host byte order */
		in.sin_port = htons((unsigned short)iPort); 		/* short, network byte order */
		in.sin_addr = adr; 	/* auto-fill with my IP */
		if(flg)
		{

			if ( bind (sok, (struct sockaddr *)&in, sizeof(in)) == -1) 
			{
				int ii =WSAGetLastError();
				closesocket(sok);
				
				sok = -1;
			}
		}
	if(sok != -1) sok_setup (sok);
	}
	LeaveCriticalSection(&gCR);
	DeleteCriticalSection(&gCR);

	return sok;
}

//____________________________________________________________
static int sok_term(int sok)
{
	if(sok != -1) closesocket(sok);
	WSACleanup ();
	return 1;
}
//____________________________________________________________
static int sok_read(int sok,unsigned char *pData,int size)
{
	return  recv(sok,(char *)pData,size,0);

}
//____________________________________________________________

static int sok_listen(int sok)
{
	fd_set read;
	struct timeval t = { 0, 5000 };
	int ret;
	
	FD_ZERO(&read);
	
	FD_SET(sok, &read);
	ret = select (sok+1, &read, 0, 0, &t);
	if (ret == -1) return 0;
	if (ret == 0) return 0;
	return 1;
}

//____________________________________________________________



// Thread qui envoie les caracteres du print
int cpt=0;

static DWORD WINAPI StdWriteFnRemote(LPVOID lpThreadParameter)
{
	T_STAPICONSOLEAPI *pHandle;
	int			thechar;
	T_UDPPARAM *pParam;
	struct sockaddr_in dst;
	pHandle = (T_STAPICONSOLEAPI*) lpThreadParameter;
	pParam = (T_UDPPARAM *)pHandle->pDrvParam;
	memset(&dst,0,sizeof(dst));
	dst.sin_family = AF_INET;
	dst.sin_port = htons(pParam->iPort);
	dst.sin_addr.s_addr = pParam->iAdressTarget | pParam->iMulticastMask; 
	
	SetEvent(pParam->hThreadInitialized);
	while(pParam->gExitThread && pParam->hConnectIn != -1)
	{

			
		thechar = ReadFromBuffer(&pHandle->HandlePutC);
		sendto(pParam->hConnectIn,(unsigned char *)&thechar,1, 0, ( struct sockaddr*)&dst, sizeof(dst));


	}

	return 0;

}
//____________________________________________________________


static DWORD WINAPI StdReadFnRemote(LPVOID lpThreadParameter)
{
	unsigned char thechar;
	int ret;
	struct sockaddr_in from;
	T_UDPPARAM *pParam;
	int nAddrLen = sizeof(struct sockaddr_in);

	T_STAPICONSOLEAPI *pHandle;
	pHandle = (T_STAPICONSOLEAPI*) lpThreadParameter;
	pParam = (T_UDPPARAM *)pHandle->pDrvParam;
	SetEvent(pParam->hThreadInitialized);

	while(pParam->gExitThread && pParam->hConnectIn!=-1)
	{
		 ret = recvfrom(pParam->hConnectIn,&thechar,1,0 ,  (struct sockaddr *)&from,&nAddrLen);
	     if(ret != -1)
		 {
			AddToBuffer(&pHandle->HandleGetC,thechar);
			if(pParam->bEchoState)
			{
				from.sin_port = htons(pParam->iPort);
				sok_send(pParam->hConnectIn,&from,(unsigned char *)&thechar,1);

			}
			if(pHandle->gpCallBack) pHandle->gpCallBack(pHandle->gpLparamCB,from.sin_addr.S_un.S_addr);

		 }

	}

	return 0;

}
//____________________________________________________________


int InitStapiConsoleUDP(T_STAPICONSOLEAPI *pHandle,char *pConfig)
{
	struct in_addr in;

	T_UDPPARAM *pParam = malloc(sizeof(T_UDPPARAM));
	memset(pParam,0,sizeof(T_UDPPARAM));
	pHandle->pDrvParam = (void *)pParam;
	pParam->iPort	   = DEFAULT_PORT;
	pParam->iMulticastMask   = 0;
	pParam->bEchoState = FALSE;
	pParam->gExitThread = TRUE;
	InitBuffer(&pHandle->HandleGetC);
	InitBuffer(&pHandle->HandlePutC);

	ParseConfig(pHandle,pConfig);

	in.s_addr = pParam->iAdressSource;
	pParam->hConnectIn = sok_init(in,pParam->iPort,TRUE);
	if(pParam->hConnectIn == -1) return 0;

	pParam->hThreadInitialized = CreateEvent(0,0,0,0);
	if(pParam->hThreadInitialized==0) return -1;

	pParam->TblHandle[0] = CreateThread(NULL,0,StdWriteFnRemote,pHandle,0,&pParam->TblId[1]);
	WaitForSingleObject(pParam->hThreadInitialized,INFINITE);

	pParam->TblHandle[1] = CreateThread(NULL,0,StdReadFnRemote,pHandle,0,&pParam->TblId[3]);
	WaitForSingleObject(pParam->hThreadInitialized,INFINITE);
#ifdef UNDER_CE
	CeSetThreadPriority(pParam->TblHandle[0],STAPI_CE_PRORITY);
	CeSetThreadPriority(pParam->TblHandle[1],STAPI_CE_PRORITY);
#endif
	StapiPutS(pHandle,pParam->pWelcomeMessage);
	return 1;

}
//____________________________________________________________


int TermStapiConsoleUDP(T_STAPICONSOLEAPI *pHandle)
{
	T_UDPPARAM *pParam = (T_UDPPARAM *)pHandle->pDrvParam;
	if(pParam->gExitThread && pParam)
	{
		pParam->gExitThread = FALSE;
//		SetEvent(pHandle->HandleGetC.ghCharAvalaible);
//		SetEvent(pHandle->HandleGetC.ghBufferFull);
//		SetEvent(pHandle->HandlePutC.ghCharAvalaible);
//		SetEvent(pHandle->HandlePutC.ghBufferFull);
		TerminateThread(pParam->TblHandle[0],-1);
		TerminateThread(pParam->TblHandle[1],-1);
		if(pParam->hConnectIn != -1) sok_term(pParam->hConnectIn);
		WaitForMultipleObjects(2,pParam->TblHandle,TRUE,INFINITE);
		CloseHandle(pParam->hThreadInitialized);
		CloseHandle(pParam->TblHandle[0]);
		CloseHandle(pParam->TblHandle[1]);
		free(pParam);
	}

	return 1;
}
