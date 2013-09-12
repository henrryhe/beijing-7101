#include "Windows.h"
#include "StapiConsoleApi.h"
#include "stdio.h"
#include "StapiConsoleDrvUDP.h"



void InitBuffer(T_BUFFER_LIST *pHandle)
{
	pHandle->SizeOQP		= 0;
	pHandle->pBufferWrite = &pHandle->tBufferIn[0];
	pHandle->pBufferRead  = &pHandle->tBufferIn[0];
	pHandle->pBufferEnd   = &pHandle->tBufferIn[SIZE_BUFFER_IN] ;
	pHandle->ghCharAvalaible = CreateEvent(0,0,0,0);
	pHandle->ghBufferFull	 = CreateEvent(0,0,0,0);
	ResetEvent(pHandle->ghCharAvalaible);
	ResetEvent(pHandle->ghBufferFull);


}
void TermBuffer(T_BUFFER_LIST *pHandle)
{
	CloseHandle(pHandle->ghCharAvalaible);
	CloseHandle(pHandle->ghBufferFull);
}



 void AddToBuffer(T_BUFFER_LIST *pHandle,int theChar)
{
	// si le buffer est presque plein on attend qu'il se vide
	if(pHandle->SizeOQP  >= SIZE_BUFFER_IN)
	{
		ResetEvent(pHandle->ghBufferFull);
		WaitForSingleObject(pHandle->ghBufferFull,INFINITE);
	}
	EnterCriticalSection(&pHandle->sCR);

	if(pHandle->pBufferWrite >= pHandle->pBufferEnd)
	{
		pHandle->pBufferWrite =  &pHandle->tBufferIn[0];
	}
	

	*pHandle->pBufferWrite++ = theChar;
	pHandle->SizeOQP ++;
	SetEvent(pHandle->ghCharAvalaible);
	LeaveCriticalSection(&pHandle->sCR);
}


int ReadFromBuffer(T_BUFFER_LIST *pHandle)
{
	int theChar = -1;

	if(pHandle->SizeOQP==0) 
	{
		// Wait for data
		ResetEvent(pHandle->ghCharAvalaible);
		WaitForSingleObject(pHandle->ghCharAvalaible,INFINITE);

	}
		
	EnterCriticalSection(&pHandle->sCR);
	
	if(pHandle->SizeOQP) 
	{
		if(pHandle->pBufferRead  >= pHandle->pBufferEnd)
		{
			pHandle->pBufferRead =  &pHandle->tBufferIn[0];
		}
		theChar =(unsigned char) *pHandle->pBufferRead++;

		pHandle->SizeOQP--;
	}
	SetEvent(pHandle->ghBufferFull);
	LeaveCriticalSection(&pHandle->sCR);
	return theChar;
	
}


int StapiGetC(T_STAPICONSOLEAPI *pHandle)
{
	unsigned char theChar;
	theChar = ReadFromBuffer(&pHandle->HandleGetC);
	if(pHandle->gServer) 
	{
		AddToBuffer(&pHandle->HandlePutC,theChar);
	}
	return  theChar;
}

int StapiPutC(T_STAPICONSOLEAPI *pHandle,int theChar)
{
	AddToBuffer(&pHandle->HandlePutC,theChar);
	return TRUE;
}

int StapiPutS(T_STAPICONSOLEAPI *pHandle,const char *pString)
{
	while(*pString)
	{

		AddToBuffer(&pHandle->HandlePutC,*pString++);
		Sleep(1);
	}
	return TRUE;
}

int StapiGetS(T_STAPICONSOLEAPI *pHandle,char* str, int size)
{
	char bellChar = '\a';
    char backspace = '\b';
    char space = ' ';
	int  i = 0;

    memset(str, '\0', size);

    // Read all input until carriage return or buffer full
    while (i < size)
    {
		str[i] = StapiGetC(pHandle ); // waiting for character
        
		// Stop if we get a carriage return
        if (str[i] == '\r')
        {
			//str[i] = '\0';
            break;
        }
        // Handle backspace.
        else if (str[i] == '\b')
        {
          // Decrement the index only if it is non-zero.
          i = i ? (i - 1) : i;
        }
        else
        {
          // Not Echo read character
         i++;
        }
    }
	return i;

}

int StapiPrintf(T_STAPICONSOLEAPI *pHandle,const char * format, ...)
{
    char tbuffer[MAX_LBUF_SIZE];
	va_list arglist;
	HRESULT hr = S_OK;
	int size ;

	va_start(arglist, format);


    // Process the string and dump it to the debug serial port
	size = _vsnprintf(&tbuffer[0], MAX_LBUF_SIZE-1, format, arglist);
	StapiPutS(pHandle,tbuffer);
	return size;
  
}


void SetStapiCallBackPutC(T_STAPICONSOLEAPI *pHandle,STAPICONSOLECB pCB,void *pLparam)
{
	pHandle->gpCallBack = pCB;
	pHandle->gpLparamCB =pLparam;
}

int GetStapiServer(T_STAPICONSOLEAPI *pHandle)
{
	return pHandle->gServer ;
}

int SetStapiConsoleDriver(T_STAPICONSOLEAPI *pHandle,int index,char *pConfig)
{
	memset(pHandle,0,sizeof(T_STAPICONSOLEAPI));
	pHandle->pConfig = pConfig;
	switch(index)
	{
	case DRVUDP:
			pHandle->m_ProcStapiInit = InitStapiConsoleUDP;
			pHandle->m_ProcStapiTerm = TermStapiConsoleUDP;
			break;

	}
	return 1;
}




int InitStapiConsole(T_STAPICONSOLEAPI *pHandle)
{
	InitializeCriticalSection(&pHandle->HandleGetC.sCR);
	InitializeCriticalSection(&pHandle->HandlePutC.sCR);

	if(pHandle->m_ProcStapiInit)	return pHandle->m_ProcStapiInit(pHandle,pHandle->pConfig);
	return -1;

}
void TermStapiConsole(T_STAPICONSOLEAPI *pHandle)
{
	if(pHandle->m_ProcStapiTerm) pHandle->m_ProcStapiTerm(pHandle);
	DeleteCriticalSection(&pHandle->HandleGetC.sCR);
	DeleteCriticalSection(&pHandle->HandlePutC.sCR);

}

