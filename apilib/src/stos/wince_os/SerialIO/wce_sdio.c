/*******************************************************************************

File name : wce_sdio.c

Description : WCE API for Serial Debug I/O. 
			Implement fonction to Get and Print string.

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                 Name
----               ------------                                 ----
29 may 2005         Created                                      OLGN

*******************************************************************************/

			
#include "wce_sdio.h"

//------------------------------------------------------------------------------
//
#define SERIAL_ERROR_TIMEOUT    5000

//------------------------------------------------------------------------------
//  Caculate Baud Rate Register Settings
//
#define CEIL(x) ((x > (float)(int)x) ? ((int)x + 1) : ((int)x))
#define BITRATEREGISTERVAL(baud,sysClock) (CEIL(((float)sysClock / (32 * baud)) - 1))

#define OALPAtoUA(pa)       (VOID*)(((UINT32)(pa))|0xA0000000)
#define OEM_DEBUG_READ_NODATA   (-1)

void WCE_OEMInitDebugSerial();
void WCE_OEMWriteDebugByte(BYTE ch);
void WCE_OEMWriteDebugString(unsigned short *string);
int WCE_OEMReadDebugByte();
void WCE_OEMClearDebugCommError();


#define Local_READ_REGISTER_USHORT(reg) \
    (*(volatile unsigned short * const)(reg))

#define Local_WRITE_REGISTER_USHORT(reg, val) \
    (*(volatile unsigned short * const)(reg)) = (val)

#define Local_READ_REGISTER_UCHAR(reg) \
    (*(volatile unsigned char * const)(reg))

#define Local_WRITE_REGISTER_UCHAR(reg, val) \
    (*(volatile unsigned char * const)(reg)) = (val)

#define INREG8(x)           Local_READ_REGISTER_UCHAR(x)
#define OUTREG8(x, y)       Local_WRITE_REGISTER_UCHAR(x, (UCHAR)(y))

#define INREG16(x)          Local_READ_REGISTER_USHORT(x)
#define OUTREG16(x, y)      Local_WRITE_REGISTER_USHORT(x,(USHORT)(y))


// OLGN modification for release BSP phase 3A, compatible with previous release
#ifndef PERIPHERAL_CLOCK_FREQ
#define PERIPHERAL_CLOCK_FREQ SH4_PERIPHERAL_CLOCK_FREQ
#endif

//------------------------------------------------------------------------------
//
//  Function:  OEMInitDebugSerial
//  
//  This function initializes the debug serial port on the target device.
//
void WCE_OEMInitDebugSerial()
{
    UINT8           brrVal;
    SH4_SCIF_REGS   *pSCIFRegs = OALPAtoUA(SH4_REG_PA_SCIF);

    // Disable Clock, Transmit, and Receive
    OUTREG16(&pSCIFRegs->SCSCR2, 0x0000);

    // Reset FIFO Transmit and Receive
    OUTREG16(&pSCIFRegs->SCFCR2, 0x0006);

    // Clear any errors due to FIFO reset
    OUTREG16(&pSCIFRegs->SCSCR2, 0x0000);

    // Set SCIF mode to be 8-Bit Data, Parity Disabled, 1 Stop Bit, No Baud Divisor
    OUTREG16(&pSCIFRegs->SCSMR2, 0x0000);

    brrVal = BITRATEREGISTERVAL(SERIAL_BAUD_RATE, PERIPHERAL_CLOCK_FREQ);
    OUTREG8(&pSCIFRegs->SCBRR2, brrVal);

    // Disable FIFO Error Resets for Transmit and Receive
    OUTREG16(&pSCIFRegs->SCFCR2, 0x0000);	

    // Clear all errors and data status bits
    OUTREG16(&pSCIFRegs->SCFSR2, 0x0000);

    // Enable Transmit and Receive
    OUTREG16(&pSCIFRegs->SCSCR2, 0x0030);
}


//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugByte
//
//  Write byte to debug serial port
//
void WCE_OEMWriteDebugByte(BYTE ch)
{
    UINT16          statusVal;
    UINT32          timeVal    = 0;
    SH4_SCIF_REGS   *pSCIFRegs = OALPAtoUA(SH4_REG_PA_SCIF);

    // Wait for data
    while(!(INREG16(&pSCIFRegs->SCFSR2) & SCIF_SCFSR2_TDFE))
    {
        if(timeVal++ >= SERIAL_ERROR_TIMEOUT)
        {
            WCE_OEMClearDebugCommError();
            return;
        }
    }

    // Send Data
    OUTREG8(&pSCIFRegs->SCFTDR2, ch);

    // Clear transmit buffer not-empty bit
    statusVal =  INREG16(&pSCIFRegs->SCFSR2);
    statusVal &= ~SCIF_SCFSR2_TDFE;
    OUTREG16(&pSCIFRegs->SCFSR2, statusVal);
}


//------------------------------------------------------------------------------
//
//  Function:  OEMWriteDebugString
//
//  Output unicode string to debug serial port
//
void WCE_OEMWriteDebugString(unsigned short *string)
{
    while((*string) != L'\0')
    {
        WCE_OEMWriteDebugByte((BYTE)(*string)); 
        string++;
    }
}


int WCE_OEMIsData()
{
    UINT8           data        = OEM_DEBUG_READ_NODATA;
    UINT16          statusVal;
    UINT16          lineVal;
    SH4_SCIF_REGS   *pSCIFRegs  = OALPAtoUA(SH4_REG_PA_SCIF);

    // Get serial status
    statusVal =  INREG16(&pSCIFRegs->SCFSR2);
    statusVal &= (SCIF_SCFSR2_DR  | SCIF_SCFSR2_ER | SCIF_SCFSR2_BRK |
                  SCIF_SCFSR2_FER | SCIF_SCFSR2_PER);

    // Get line status
    lineVal =  INREG16(&pSCIFRegs->SCLSR2);
    lineVal &= SCIF_SCLSR2_ORER;

    // Check to see if there were any communication errors
    if(!(statusVal || lineVal)) 
    {
        // Check for data in receive buffer
			statusVal = INREG16(&pSCIFRegs->SCFSR2) & SCIF_SCFSR2_RDF;
			if(statusVal) return TRUE;
	}
	return FALSE;
}



//------------------------------------------------------------------------------
//
//  Function:  OEMReadDebugByte
//
//  Input character/byte from debug serial port
//
int WCE_OEMReadDebugByte()
{
    UINT8           data        = OEM_DEBUG_READ_NODATA;
    UINT16          statusVal;
    UINT16          lineVal;
    SH4_SCIF_REGS   *pSCIFRegs  = OALPAtoUA(SH4_REG_PA_SCIF);

    // Get serial status
    statusVal =  INREG16(&pSCIFRegs->SCFSR2);
    statusVal &= (SCIF_SCFSR2_DR  | SCIF_SCFSR2_ER | SCIF_SCFSR2_BRK |
                  SCIF_SCFSR2_FER | SCIF_SCFSR2_PER);

    // Get line status
    lineVal =  INREG16(&pSCIFRegs->SCLSR2);
    lineVal &= SCIF_SCLSR2_ORER;

    // Check to see if there were any communication errors
    if(!(statusVal || lineVal)) 
    {
        // Check for data in receive buffer
        while (!statusVal)
		{
			statusVal = INREG16(&pSCIFRegs->SCFSR2) & SCIF_SCFSR2_RDF;
			Sleep(1);
		}
        if(statusVal)
        {
            // Get Data
			data = INREG8(&pSCIFRegs->SCFRDR2);

            // Clear receive buffer not-empty bit
            statusVal =  INREG16(&pSCIFRegs->SCFSR2); 
            statusVal &= ~SCIF_SCFSR2_RDF;
            OUTREG16(&pSCIFRegs->SCFSR2, statusVal);
        }
    }
    else
    {
        WCE_OEMClearDebugCommError();
    }

    return (int)data;
}


//------------------------------------------------------------------------------
//
//  Function:     OEMClearDebugCommError
//
//  Clear debug serial port error
//
void WCE_OEMClearDebugCommError()
{
    UINT16          statusVal;
    UINT16          lineVal;
    SH4_SCIF_REGS   *pSCIFRegs  = OALPAtoUA(SH4_REG_PA_SCIF);

    // Clear errors on serial port
    statusVal =  INREG16(&pSCIFRegs->SCFSR2);
    statusVal &= ~(SCIF_SCFSR2_ER  | SCIF_SCFSR2_BRK | SCIF_SCFSR2_DR |
                   SCIF_SCFSR2_PER | SCIF_SCFSR2_FER);
    OUTREG16(&pSCIFRegs->SCFSR2, statusVal);

    // Clear overrun error
    lineVal =  INREG16(&pSCIFRegs->SCLSR2);
    lineVal &= ~SCIF_SCLSR2_ORER;
    OUTREG16(&pSCIFRegs->SCLSR2, lineVal);
}


// ---------------------- API FUNCTIONS ------------------------------------

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_WriteSerialByte
//
//  Write Byte on Serial Output
//------------------------------------------------------------------------------
BOOL wce_sdio_putc(VOID* pInpBuffer)
{
    BOOL rVal = FALSE;

    if (pInpBuffer)
    {
        WCE_OEMWriteDebugByte(*(char*)pInpBuffer); 
        rVal = TRUE;
    }
	else
	{
		WCE_OEMClearDebugCommError();
        rVal=FALSE;
	}
 
    return rVal;
}
void _WCE_AddConsoleLogFile(char *pString);

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_puts
//
//  Write string on Serial Output
//------------------------------------------------------------------------------
BOOL wce_sdio_puts(char* pInpBuffer)
{
    BOOL rVal = FALSE; 

  /*  if (pInpBuffer)
    {
        while((*(char*)pInpBuffer) != '\0')
        {
            OEMWriteDebugByte(*(char*)pInpBuffer); 
			((char*)pInpBuffer)++;
        }
        rVal = TRUE;
    }*/
	_WCE_AddConsoleLogFile(pInpBuffer);
	
	if (pInpBuffer)
    {
        while((*(char*)pInpBuffer) != '\0')
        {
            WCE_OEMWriteDebugByte(*(char*)pInpBuffer); 
			if((*(char*)pInpBuffer) == '\n')
				WCE_OEMWriteDebugByte('\r');
			((char*)pInpBuffer)++;
        }
        rVal = TRUE;
    }
	
    return rVal;
}

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_SerialConsoleOut
//
//  Write format string on Serial Output (equivalent printf)
//------------------------------------------------------------------------------
int wce_sdio_printf(const char * format, ...)
{
    static char OutStr[MAX_SERIALBUF_SIZE];
	va_list arglist;

	va_start(arglist, format);

    memset(OutStr, '\0', MAX_SERIALBUF_SIZE);

    // Process the string and dump it to the debug serial port
	_vsnprintf(&OutStr[0], MAX_SERIALBUF_SIZE-1, format, arglist);
    
	wce_sdio_puts(OutStr);

	return(strlen(OutStr));
	
}

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_ReadSerialInputByte
//
//  Read Byte from Serial Input ( prefer  getchar ?)
//------------------------------------------------------------------------------
BOOL wce_sdio_ReadSerialInputByte( VOID *pOutBuffer, UINT32 outSize, UINT32 *pOutSize) 
{

    BOOL rVal = FALSE;
    char readData;

    // Check parameters
    if (pOutBuffer && (outSize == sizeof(readData)))
    {
        readData = (char)WCE_OEMReadDebugByte();
        // Check to see if data was read
        if (readData != (char)OEM_DEBUG_READ_NODATA)
        {
            *(char*)pOutBuffer = readData;
            // Report size of read data, if possible
            if(pOutSize)
            {
                *pOutSize = sizeof(readData);
            }
        }
        else    // No data available
        {
            // Report no data read, if possible
            if (pOutSize)
            {
                *pOutSize = 0;
            }
        }
        rVal = TRUE;
    }
    else    // Invalid parameters
    {
		WCE_OEMClearDebugCommError();
        rVal=FALSE;       
    }

    return rVal;
}

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_getchar
//
//  wait&get input char
//------------------------------------------------------------------------------
int wce_sdio_getchar(BOOL blocking)
{
	int val;
	if(!blocking) 
	{
		if(WCE_OEMIsData() == FALSE) return 0;
	}
	val = (int)WCE_OEMReadDebugByte();
	return val;
}

//------------------------------------------------------------------------------
//
//  Function:  wce_sdio_GetSerialString
//
//  Get String from Serial Input until return
//
// return string length
//------------------------------------------------------------------------------
int wce_sdio_gets(char* str, int size)
{
	char bellChar = '\a';
    char backspace = '\b';
    char space = ' ';
	int  i = 0;
	UINT32 bytesRead;

    memset(str, '\0', size);

    // Read all input until carriage return or buffer full
    while (1)
    {
       // KernelIoControl(IOCTL_HAL_READ_SERIAL_BYTE, NULL, 0, &readChars[i], sizeof(char), &bytesRead);
		wce_sdio_ReadSerialInputByte( &str[i], sizeof(char), &bytesRead);
        if (bytesRead != 0)
        {
            // Stop if we get a carriage return
            if (str[i] == '\r')
            {
				str[i] = '\0';
                break;
            }

            // Handle backspace.
            if (str[i] == '\b')
            {
                // Decrement the index only if it is non-zero.
                i = i ? (i - 1) : i;
            }
            // If we're at the end of the buffer, only accept a carriage return as input.
            else if (i == (size - 1)) 
            {
                // If we get here, we have not received a carriage return.  Simply ignore the last byte received
                // and send a bell character.
              //  KernelIoControl(IOCTL_HAL_WRITE_SERIAL_BYTE, &bellChar, 0, NULL, 0, NULL);
                wce_sdio_puts( &bellChar);
            }
            else
            {
                // Echo read character
              //  KernelIoControl(IOCTL_HAL_WRITE_SERIAL_BYTE, &readChars[i], 0, NULL, 0, NULL);
			//	MyWriteSerialString( &str[i], 0,0, NULL);	
                i++;
            }
        }        
    }

	return i;

}

void wce_InitDebugSerial()
{
	WCE_OEMInitDebugSerial();
}