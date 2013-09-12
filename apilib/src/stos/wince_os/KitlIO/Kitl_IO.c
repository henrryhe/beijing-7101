/***********************************************************************
 MODULE:   Kitl_IO.c

 PURPOSE: Definition of KITL_IO Library implementing simple I/O cmd using ITL communication stream 
			
 COMMENTS: This lib use 2 ITL streams (1 in, 1 out) to allow communication with developpement station
			connecting using Plateform Builder KITL Transport 

 HISTORY :
 OLGN		18/05/05		Creation
***********************************************************************/

#define INITGUID

#include "Kitl_IO.h"
//#include <winbase.h>

// cannot trace here !
#define SILENT_ASSERT(cond) if (!(cond)) DebugBreak();

class CConnectionStream
{
public:
	CConnectionStream();
	virtual ~CConnectionStream();
	virtual BOOL Close();
	virtual LRESULT WriteBytes( LPVOID pData, DWORD  dwSize);
	virtual LRESULT ReadBytes(LPVOID pData, DWORD dwSize, DWORD *pdwSizeRet);
};

extern "C" CConnectionStream *CreateStream(GUID HostId, DWORD dwPort); 
typedef CConnectionStream *(*CREATESTREAMFUNC)(GUID HostId, DWORD dwPort);

// ReceiveStreamID - {5344F94F-E122-430c-A553-62EB86D7671C}
static const GUID  PcToDeviceStreamID = {0x5344f94f, 0xe122, 0x430c, {0xa5, 0x53, 0x62, 0xeb, 0x86, 0xd7, 0x67, 0x1c}};

// SendStreamID - {6EA3CD6A-F31F-403b-92BD-73CFB77B8C31}
static const GUID DeviceToPcStreamID = { 0x6ea3cd6a, 0xf31f, 0x403b, { 0x92, 0xbd, 0x73, 0xcf, 0xb7, 0x7b, 0x8c, 0x31 } };

typedef enum _CMD_VALUES
{
    CMD_NONE,
    CMD_GET_DATA,
	CMD_GET_STRING,
	CMD_GET_STRING_LOOP,
	CMD_SCANF,
    CMD_FINISHED
} CMD_VALUES;


HMODULE hModule = NULL;
CConnectionStream *pPcToDeviceStream = NULL;
CConnectionStream *pDeviceToPcStream = NULL;

char String[MAX_KITLBUF_SIZE];

// Asynchronous input
static HANDLE receivingThread = NULL;
static HANDLE receivedCharsW = NULL; // message queue, writer side
static HANDLE receivedCharsR = NULL; // message queue, reader side

//**********************************************************************
BOOL InitializeITL(HMODULE *phModule,
                   CConnectionStream **ppStream1, CConnectionStream **ppStream2)
//**********************************************************************
{
    BOOL brc = FALSE;
    CREATESTREAMFUNC pfnCreateStream = NULL;

	// Load the Transport Library
	*phModule = LoadLibrary( L"CETLSTUB.DLL");

	if (*phModule)
    {
		pfnCreateStream =(CREATESTREAMFUNC)GetProcAddress(*phModule ,
                                                           L"CreateStream");
		/*--------------------------------------------------------------*/
		// Create the stream that matches up to the desktop (GUID)
		/*--------------------------------------------------------------*/
		*ppStream1 = NULL;
 		*ppStream1 = pfnCreateStream(PcToDeviceStreamID, 0);
	//	*ppStream1 = CreateStream(PcToDeviceStreamID, 0);

     	/*--------------------------------------------------------------*/
		// Create a 2nd stream that matches up to the desktop (GUID)
		/*--------------------------------------------------------------*/
		*ppStream2 = NULL;
 		*ppStream2 = pfnCreateStream(DeviceToPcStreamID, 0);
	//	*ppStream1 = CreateStream(PcToDeviceStreamID, 0);

		if (*ppStream1 && *ppStream2)
        {
            brc = TRUE;
        }
		FreeLibrary(*phModule);
    }

    return brc;
}


//**********************************************************************
void Cleanup(HMODULE hModule,
             CConnectionStream *pStream1, CConnectionStream *pStream2)
//**********************************************************************
{
	if (pStream1)
	{
		pStream1->Close();
		delete pStream1;
		pStream1 = NULL;
	}
	if (pStream2)
	{
		pStream2->Close();
		delete pStream2;
		pStream2 = NULL;
	}

	//FreeLibrary(hModule);

	return;
}

// Waiting char Thread
DWORD WaitForChar (void* param)
{
	DWORD dwSizeRec = 0;
    char c;

    while (   pPcToDeviceStream != NULL
		   && pPcToDeviceStream->ReadBytes((BYTE*)&c, sizeof(char), &dwSizeRec) == S_OK
           && dwSizeRec == sizeof(char)
           && WriteMsgQueue(receivedCharsW, &c, sizeof(char), INFINITE, 0)
          )
        ;
	return 1;
}

//------------------------------------------------------------------------------
//
//  Function:  wce_Kitl_ReadSerialInputByte
//
//  Read Byte from Kitl Input ( prefer  getchar ?) using multitasking
//  Return TRUE if a char was read
//------------------------------------------------------------------------------
BOOL wce_Kitl_ReadSerialInputByte(char* pOutBuffer, BOOL blocking) 
{
    DWORD read = 0, flags;

    return ReadMsgQueue(receivedCharsR, pOutBuffer, sizeof(*pOutBuffer), &read, 
                     blocking ? INFINITE : 0, &flags)
           && read == sizeof(*pOutBuffer);
}


/*****************************************************************/
//					EXPORTED FUNCTIONS
/*****************************************************************/

//-----------------------------------------------------------------------------
//
//  Function:  Wce_KitlIO_Init
//
//  Init ITL Communication Stream - This fonction is waiting for Init on PC side but
//	with a TimeOut.
//------------------------------------------------------------------------------
BOOL Wce_KitlIO_Init(void)
{
    MSGQUEUEOPTIONS mSGQOptions = { sizeof(mSGQOptions), 
	                                MSGQUEUE_NOPRECOMMIT,
                                    100, // max messages = 100
	                                1, // size of each message in queue
                                    FALSE // writer side
                                  };

    BOOL ret;
    
    ret = InitializeITL(&hModule, &pPcToDeviceStream, &pDeviceToPcStream);
    if (!ret)
        return ret;

    // create thread for waiting on input char
    receivedCharsW = CreateMsgQueue(NULL, &mSGQOptions);
    SILENT_ASSERT(receivedCharsW != NULL);
     
	receivingThread = CreateThread(NULL, // no security options
	                               0,    // No Stack Size
	                               WaitForChar,     // Entry point
	                               NULL, // Entry point parameters
                                   0,    // no flags (so threads runs immediately)
                                   NULL  // don't need Thread Id
                                  );
    SILENT_ASSERT(receivingThread != NULL);

//	CeSetThreadPriority(receivingThread, 255);

    mSGQOptions.bReadAccess = TRUE;
    receivedCharsR = OpenMsgQueue(GetCurrentProcess(), // queue owner process
                                  receivedCharsW, // queue handle
                                  &mSGQOptions);
    SILENT_ASSERT(receivedCharsR != NULL);
    return ret;
}

//-----------------------------------------------------------------------------
//
//  Function:  Wce_KitlIO_Init
//
//  Init ITL Communication Stream - This fonction is waiting for Init on PC side but
//	with a TimeOut.
//------------------------------------------------------------------------------
void Wce_KitlIO_Term(void)
{
    CloseHandle(receivedCharsR);
    CloseHandle(receivedCharsW);
    CloseHandle(receivingThread);
	Cleanup(hModule, pPcToDeviceStream, pDeviceToPcStream);
}




//-----------------------------------------------------------------------------
//
//  Function:  wce_kitl_printf
//
//  Write format string on Kitl Output (equivalent printf)
//------------------------------------------------------------------------------
int Wce_KitlIO_Printf(const char * format, ...)
{
    char OutStr[MAX_KITLBUF_SIZE];
	va_list arglist;
	HRESULT hr = S_OK;

	va_start(arglist, format);

    memset(OutStr, '\0', MAX_KITLBUF_SIZE);

    // Process the string and dump it to the debug serial port
	_vsnprintf(&OutStr[0], MAX_KITLBUF_SIZE-1, format, arglist);

	int size = sizeof(OutStr);
    
	if (pDeviceToPcStream)
		pDeviceToPcStream->WriteBytes((BYTE *)&OutStr, size);

	return size;
  
}

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_getchar
//
//  wait&get input char
//------------------------------------------------------------------------------
int Wce_KitlIO_Getchar(BOOL blocking)
{
	int val;
	if (wce_Kitl_ReadSerialInputByte((char*) &val, blocking))
	    return val;
    else
        return 0; // no char in blocking mode
}

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_GetSerialString
//
//  Get String from Serial Input until return
//
// return string length
//------------------------------------------------------------------------------
int Wce_KitlIO_Gets(char* str, int size)
{
	char bellChar = '\a';
    char backspace = '\b';
    char space = ' ';
	int  i = 0;

    memset(str, '\0', size);

    // Read all input until carriage return or buffer full
    while (i < size)
    {
		wce_Kitl_ReadSerialInputByte( &str[i], TRUE); // waiting for character
        
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
		/*if (pDeviceToPcStream)
			pDeviceToPcStream->WriteBytes((BYTE *)&backspace, sizeof(char));*/
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

