
#include <stdio.h>
#include <string.h>

//#define FROM_STARSCOPE
#define WINDOWS_DBG
//#define WINDOWS_DBG
#include "sysuser.h"

#define DEFAULT_PROC    // if you want to use default event processing


// WHEN YOU MUST DEFINE FROM_STARSCOPE AND WINDOWS_DBG ?

// define FROM_STARSCOPE means you start an application by clicking on the "Start Log" Button
// in this case the application must be embedded (using the development tool MICROTEC)
// and WINDOWS_DBG must not be defined.
// By clicking on "Start Log", STARSCOPE sends the following commands to the STAR 1150
// with the parameters you have chosen before running the embedded application you have chosen
// MPS_SetMode
// MPS_OpenLog
// MPS_SetTrigger
// By clicking on "Stop Log", STARSCOPE will send "APST" to the STAR 1150 (which will generate
// the event EVT_STOP_REQUEST), will stop the acquisition and will get the trace automatically.
// if you want STARSCOPE to display the number of events dynamically, you have to call
// the API MPS_SendCurrentState(1);



// If the application runs on PC, you must not press the "Start Log" Button because
// the application is not embedded.
// in this case FROM_STARSCOPE must not be defined and WINDOWS_DBG must be defined.
// You have to call the API : MPS_SetMode, MPS_SetConvention , MPS_OpenLog and MPS_CloseLog
// When the application ends, you can use STARSCOPE to connect the STAR 1150 and get the
// trace (if any) by clicking the button "Get Trace" but not "Stop Log"
// 

BYTE GRemotePort;       // Save remote port
BYTE CardMemory[4096];  // Reserve 4K for memory of simulated card

// Application initialization
void MyInit(void)
{
#ifdef FROM_STARSCOPE
    // Get th remote port
    GRemotePort = MPS_GetRemotePort();
    // Send response to StarScope (mandatory: response to APGO sent by STARSCOPE)
    MPS_PortSend( GRemotePort, (BYTE *)"APGO 0000\r\n", 11);
#else
    // Select Asynchronous mode
    MPS_SetMode(MODE_ASYNC);
#endif

    // For handling parity error
    MPS_IOSetErrorMode( 0);
    MPS_SetNbRetransmission(3);	// for example

#ifndef FROM_STARSCOPE
    // call to MPS_Openlog to get the trace
    MPS_OpenLog( EN_C1|EN_C2|EN_C6|EN_C7|EN_PRECLK|
                 EN_RX|EN_TX|EN_ETUCHG|EN_CLKDIV);
#endif
}

// Cold ATR processing in embedded application
void ColdATRRequest( void)
{    
WORD ret=0;

    // ATR in direct convention
    ret = MPS_SendATR( (BYTE *)"\x3B\x35\x11\x00\x24\xc2\x01\x90\x00", 9L);
    
    if( ret)
    {
        // Error handling
    }
}

// Warm ATR processing in embeded mode
void WarmATRRequest( void)
{
WORD ret=0;

    // ATR with direct convention
    ret = MPS_SendATR( (BYTE *)"\x3B\x35\x11\x00\x24\xc2\x01\x90\x00", 9L);
    
    if( ret)
    {
        // Error handling
    }
}

// PPS processing
void PPSRequest(void)
{
WORD ret=0;
PPS_STRUCT bufpps;

    // Get PPS request
    ret = MPS_GetPPSRequest( &bufpps);

    // Sending default PPS response
    if( !ret)
        ret = MPS_SendPPSResponse( 0L);
    
    if( ret)
    {
        // Error handling
    }
}

// Ack request processing
void AckRequest( void)
{
WORD        ret=0;
APDU_HEADER hapdu;

    // Get APDU header
    if( !ret)
        ret = MPS_GetAPDUHeader( &hapdu);
    
    if( !ret)
        // Ack = Ins        
        MPS_TransmitAck( hapdu.Ins);

    if( ret)
    {
        // Error handling
    }
}

// APDU header processing
void APDUHeaderReceived( void)
{
WORD        ret=0;
APDU_HEADER hapdu;

    // Get APDU header
    ret = MPS_GetAPDUHeader( &hapdu);
    
    if( !ret)
    {
        switch( hapdu.Ins)
        {
            case 0x0e:  // ERASE
                MPS_SetNbBytesToReceive(2);
                break;
                
            case 0xb0:  // READ 
                MPS_SetNbBytesToReceive(0);
                break;

            case 0xb8:  // read directory
                MPS_SetNbBytesToReceive(0);
                break;
            
            case 0xa4:  // select
            case 0xd0:  // WRITE
            case 0xd6:
                MPS_SetNbBytesToReceive(hapdu.P3);
                break;
        }
    }

    if( ret)
    {
        // Error handling
    }
}

// APDU processing
void APDUReceived( void)
{
WORD        ret=0;
APDU_HEADER hapdu;
BYTE        buf[256];
DWORD       len;      
WORD        i,curidx,endidx;

    // Get APDU header
    ret = MPS_GetAPDU( &hapdu, &buf[0], &len);
    
    if( ret)
    {
        // Error handling
    }
    else
    {
        // Get start card memory location
        curidx   = hapdu.P1;
        curidx <<= 8;
        curidx  |= hapdu.P2;
                
        switch( hapdu.Ins)
        {
            case 0xb8: // read directory
                // Fill up the buffer (with junk), so that the test harness doesn't time out. It doesn't
                // actually matter either way, it just looks better to get STSMART_UNKNOWN_ERROR
                // than ST_ERROR_TIMEOUT.
                for( i=0; curidx < hapdu.P3; curidx++, i++)
                    buf[i] = CardMemory[curidx];
                    
                MPS_SendRAPDU( &buf[0], (DWORD)(hapdu.P3==0?256:hapdu.P3), 0x6B00);
                break;

            case 0xa4:  // select
                // Get APDU header
                ret = MPS_GetAPDU( &hapdu, &buf[0], &len);

                MPS_SendRAPDU( 0L, 0L, 0x9000);
                break;

            case 0x0e:  // ERASE
                endidx   = buf[0];
                endidx <<= 8;
                endidx  |= buf[1];
                
                for( ; curidx <= endidx; curidx++)
                    CardMemory[ curidx] = (BYTE)0xff; 
                
                // For erase simulation start waiting mode ACK = 0x60
                MPS_SetWaitingMode();
                
                // Waiting 3 seconds for simulation
                //MPS_Wait( WD_ETU, 40322L);  // in ETU
                MPS_Wait( WD_MS, 3000L);	// in ms
                
                // Reset waiting mode
                MPS_ResetWaitingMode();
                    
                MPS_SendRAPDU( 0L, 0L, 0x9000);
                break;
                
            case 0xb0:  // READ
                endidx = curidx + (hapdu.P3==0?256:hapdu.P3);
                
                for( i=0; curidx<endidx; curidx++,i++)
                    buf[i] = CardMemory[curidx];
                    
                MPS_SendRAPDU( &buf[0], (DWORD)(hapdu.P3==0?256:hapdu.P3), 0x9000);
                break;
                
            case 0xd6:
            case 0xd0:  // WRITE
                endidx = curidx + (hapdu.P3==0?256:hapdu.P3);
                
                for( i=0; curidx<=endidx; curidx++,i++)
                    CardMemory[curidx] = buf[i];
                    
                MPS_SendRAPDU( 0L, 0L, 0x9000);
                break;
        }
    }
}

// Error processing
void ErrorHandling(void)
{
WORD error;

    error = MPS_GetLastError();
}

// Entry point of application
void entry( void)
{
DWORD eventID; 
WORD  ret=0;
    
    MyInit();
    
    // Init context
    MPS_OpenStarSim( SIMULATION_MODE);

#ifdef FROM_STARSCOPE    
    // this embedded application has been launched by STARSCOPE (Start Log Button)
	// call to MPS_SendCurrentState to send perodically the number of events to STARSCOPE
	// that will display them dynamically
    MPS_SendCurrentState(1);
#endif
	// example to generate perturbations 
	//ret = MPS_GenErrTXParity( 2, 4, 2);
	//ret = MPS_GenErrETUShift( 1, 1, 1, 2);
	//ret = MPS_GenErrRXParity( 3, 6, 1);

#ifdef WINDOWS_DBG
    // Set cold and warm ATR in case of application running on PC
    MPS_SetColdATR( (BYTE *)"\x3B\x35\x11\x00\x24\xc2\x01\x90\x00", 9L);
    MPS_SetWarmATR( (BYTE *)"\x3B\x35\x11\x00\x24\xc2\x01\x90\x00", 9L);
#endif

	// Waiting for next event
    while( eventID = MPS_WaitNextEvent())
    {
        switch( eventID)
        {
            case EVT_COLD_ATR_REQUEST:
                ColdATRRequest();
                break;
                
            case EVT_WARM_ATR_REQUEST:
                WarmATRRequest();
                break; 
                
#ifndef DEFAULT_PROC
            case EVT_PPS_REQUEST:
                PPSRequest();
                break;

            case EVT_ATR_SENT:
                break;
                
            case EVT_ACK_REQUEST:
                AckRequest();
                break;   
#endif

            case EVT_APDU_HEADER_RECEIVED:
                APDUHeaderReceived();
                break;
                
            case EVT_APDU_RECEIVED:
                APDUReceived();
                break;

#ifdef FROM_STARSCOPE
            case EVT_STOP_REQUEST:		// stop the application if STAR 1150 receive APST from STARSCOPE
#endif
#ifndef FROM_STARSCOPE
			case EVT_POWER_OFF:			// example to stop the application
#endif
                goto f_entry;
                break;
                
            case EVT_ERROR:
                ErrorHandling();
                break;
                
            default:
                // If you want to use default event processing
                ret = MPS_DefaultProc( eventID);
                break;
        }
    } 

f_entry:
	MPS_CloseStarSim();
#ifndef FROM_STARSCOPE    
    // we have to stop acquisition trace
	// STARSCOPE does it automatically when we click on the Stop Log button
    MPS_CloseLog();
#endif
return;
}

#ifdef WINDOWS_DBG
// Entry point of application running on PC
int main()
{
HINSTANCE hinst;
int       r;
    
    // Load DLL (fonction WINDOWS)
    hinst = LoadLibrary("stcp1150.dll");       
                           
    // if <= 32 then error
    if( (long)hinst>32)
    {                                                
        // init with TCP/IP
        r= StartDebug("10779:blackcap"); 
        
        // init with serial port
        //r= StartDebug("COM1:19200,N,8,2");
    
        if(!r)
        {                  
            // call application
            entry();               
            
            // Free DLL resources
            EndDebug();
        }                                                    
        
        // Unload DLL
        FreeLibrary(hinst);
    }
return 0;
}
#else                                               

// Entry point of application (embedded)
void main( void)
{                                                                              
    // call application
    entry();
}

#endif
