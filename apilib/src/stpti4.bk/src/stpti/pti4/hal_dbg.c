/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: hal_dbg.c
 Description: debugging functions. 

******************************************************************************/

#include <string.h>
#include <assert.h>

#include "stddefs.h"
#include "stdevice.h"
#define STTBX_PRINT
#include "sttbx.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "pti4.h"
#include "tchal.h"
#include "memget.h"

#ifndef STPTI_DEBUG_SUPPORT
 #error Incorrect build options!
#endif


/* Private Variables ------------------------------------------------------- */
#ifdef STPTI_DEBUG_SUPPORT
STPTI_DebugDMAInfo_t         *DebugDMAInfo = NULL;
STPTI_DebugInterruptStatus_t *DebugInterruptStatus = NULL;
STPTI_DebugStatistics_t      DebugStatistics = {0};

int  IntInfoCapacity = 0;
BOOL IntInfoStart = FALSE;
int  IntInfoIndex =0;
#endif

 /******************************************************************************
Function Name : stptiHAL_DebugGetTCRegisters
  Description : Peeks the values of all of the TC registers and the 
                Instruction pointer.
   Parameters : FullHandle_t DeviceHandle = A handle to the stpti device.
                TCRegisters = the address where the value of the TC registers
                are returned.
******************************************************************************/
void  stptiHAL_DebugGetTCRegisters(FullHandle_t DeviceHandle, STPTI_TCRegisters_t *TCRegisters)
{
    TCDevice_t *TC_Device = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

    TCRegisters->RegA = TC_Device->TCRegA & 0xffff; 
    TCRegisters->RegB = TC_Device->TCRegB & 0xffff;     
    TCRegisters->RegC = TC_Device->TCRegC & 0xffff;     
    TCRegisters->RegD = TC_Device->TCRegD & 0xffff;     
    TCRegisters->RegP = TC_Device->TCRegP & 0xffff;     
    TCRegisters->RegQ = TC_Device->TCRegQ & 0xffff;     
    TCRegisters->RegI = TC_Device->TCRegI & 0xffff;
    TCRegisters->RegO = TC_Device->TCRegO & 0xffff;
    TCRegisters->IPtr = TC_Device->TCIPtr & 0xffff;     
    TCRegisters->RegE0 = TC_Device->TCRegE0 & 0xffff;     
    TCRegisters->RegE1 = TC_Device->TCRegE1 & 0xffff;     
    TCRegisters->RegE2 = TC_Device->TCRegE2 & 0xffff;     
    TCRegisters->RegE3 = TC_Device->TCRegE3 & 0xffff;
    TCRegisters->RegE4 = TC_Device->TCRegE4 & 0xffff;
    TCRegisters->RegE5 = TC_Device->TCRegE5 & 0xffff;
    TCRegisters->RegE6 = TC_Device->TCRegE6 & 0xffff;
    TCRegisters->RegE7 = TC_Device->TCRegE7 & 0xffff;
    
    STTBX_Print(("\nTC Registers:\n=============\n"));
    STTBX_Print(("Reg:   A    B    C    D    P    Q    I    O    E0   E1   E2   E3   E4   E5   E6   E7   Iptr\n"));
    STTBX_Print(("       %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x %04x\n", 
                TCRegisters->RegA, TCRegisters->RegB, TCRegisters->RegC, TCRegisters->RegD,
                TCRegisters->RegP, TCRegisters->RegQ, TCRegisters->RegI, TCRegisters->RegO,
                TCRegisters->RegE0, TCRegisters->RegE1, TCRegisters->RegE2, TCRegisters->RegE3,
                TCRegisters->RegE4, TCRegisters->RegE5, TCRegisters->RegE6, TCRegisters->RegE7,
                TCRegisters->IPtr));
    
}


/******************************************************************************
Function Name : stptiHAL_DebugDumpTCDRam
  Description : Dumps the contents of the TC Data Ram.  
   Parameters : FullHandle_t DeviceHandle = A handle to the stpti device.
******************************************************************************/
void  stptiHAL_DebugDumpTCDRam(FullHandle_t DeviceHandle)
{
    TCDevice_t *TC_Device = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;
    int i=0;
    
    STTBX_Print(("\nTCRam Dump: Base Address %x\n", TC_Device->TC_Data ));
    
    for(i=0; i< TC_DATA_RAM_SIZE; i+=8)
    {
    STTBX_Print(("%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n", 
                &(TC_Device->TC_Data[i]),
                TC_Device->TC_Data[i], TC_Device->TC_Data[i+1], TC_Device->TC_Data[i+2], TC_Device->TC_Data[i+3],
                TC_Device->TC_Data[i+4], TC_Device->TC_Data[i+5], TC_Device->TC_Data[i+6], TC_Device->TC_Data[i+7]
                /*
                TC_Device->TC_Data[i], TC_Device->TC_Data[i+1], TC_Device->TC_Data[i+2], TC_Device->TC_Data[i+3],
                                TC_Device->TC_Data[i], TC_Device->TC_Data[i+1], TC_Device->TC_Data[i+2], TC_Device->TC_Data[i+3]*/
                ));    
    }
}

/******************************************************************************
Function Name : stptiHAL_DebugGetDMAHistory
  Description : 
   Parameters : FullHandle_t DeviceHandle = A handle to the stpti device.
******************************************************************************/
void  stptiHAL_DebugGetDMAHistory(FullHandle_t DeviceHandle, U32 *NoOfStructsStored, STPTI_DebugDMAInfo_t *History_p, U32 Size)
{

    TCPrivateData_t      *PrivateData_p = NULL;
    STPTI_TCParameters_t *TC_Params_p   = NULL;    
    TCDebug_t            *TCDebug_p     = NULL;
    U32                  DMAInfoIndex   = 0;
    U32                  UsersArrayCapacity = Size / (sizeof(STPTI_DebugDMAInfo_t)) ;
    int                  i;
    
    PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);    
    TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;     
    
    TCDebug_p = (TCDebug_t*) TC_Params_p->TC_DebugStart;

    /* set the st20 readig flag */                    
    TCDebug_p->DMAHistoryST20Reading = 0xffff;
        
    __optasm{ ldc 0; ldc 0; ldc 0; ldc 0; }
    __optasm{ ldc 0; ldc 0; ldc 0; ldc 0; } 
    __optasm{ ldc 0; ldc 0; ldc 0; ldc 0; } 

    /* wait until the TC is not writing debug */  
    do
    {
        /* Delay a few no-ops to give TC time to process */
        __optasm{ ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; ldc 0; }       
        
    } while (TCDebug_p->DMAHistoryTCWriting && 0xffff);

    /* finished ensuring mutual exclusion now read to get dma info from TC ram */
    
    DMAInfoIndex = TCDebug_p->DMAHistoryIndex;
    
    STTBX_Print(("TCDebug_p->DMAHistoryWrapped %x\n", TCDebug_p->DMAHistoryWrapped));
    STTBX_Print(("TCDebug_p->DMAHistoryIndex %x\n",     TCDebug_p->DMAHistoryIndex));
    STTBX_Print(("TCDebug_p->DMAHistoryTCWriting %x\n",     TCDebug_p->DMAHistoryTCWriting));
    if ( DMAInfoIndex == 0 && (TCDebug_p->DMAHistoryWrapped == 0 ))
    {        
        /* No packets have been received */
        *NoOfStructsStored = 0;
        STTBX_Print(("\n No DMA info collected\n"));
    }
    else
    {        
        STTBX_Print(("\nDMAHistory\n==========\nTop      Base     Read     Write    \n"));

        /* if we have wrapped */
        if ( TCDebug_p->DMAHistoryWrapped )
        {      
            int j=0;

            /* the buffer is full */
            *NoOfStructsStored = MAX_NO_OF_DMA_TXFRS_RECORDED;

            for (i=DMAInfoIndex, j=0; j < MAX_NO_OF_DMA_TXFRS_RECORDED; i=((i+1)%MAX_NO_OF_DMA_TXFRS_RECORDED), j++)
            {
                if (j < UsersArrayCapacity) /* only copy those we have space for as user specifies the size of his array up to MAX_NO_OF_DMA_TXFRS_RECORDED */
                {

                    History_p[j].Top   = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Top;
                    History_p[j].Base  = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Base;
                    History_p[j].Read  = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Read;
                    History_p[j].Write = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Write;
                }
                /* Print all MAX_NO_OF_DMA_TXFRS_RECORDED results */
                STTBX_Print(("%08x %08x %08x %08x\n", TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Top,
                                                      TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Base,
                                                      TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Read,
                                                      TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Write));                    
            }
        }
        else
        {
            /* the buffer has not wrapped */
            *NoOfStructsStored = DMAInfoIndex;

            for (i=0; i < DMAInfoIndex; i++) 
            {              
                History_p[i].Top   = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Top;
                History_p[i].Base  = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Base;
                History_p[i].Read  = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Read;
                History_p[i].Write = TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Write;
                STTBX_Print(("%08x %08x %08x %08x\n", TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Top,
                                                      TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Base,
                                                      TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Read,
                                                      TCDebug_p->DMAHistory[i%MAX_NO_OF_DMA_TXFRS_RECORDED].Write));

            }                
        }
    }
    /* clear st20 reading flag */
    TCDebug_p->DMAHistoryST20Reading = 0;
}

/******************************************************************************
Function Name : stptiHAL_DebugGetTCStatus
  Description : 
   Parameters : FullHandle_t DeviceHandle = A handle to the stpti device.
******************************************************************************/
void stptiHAL_DebugGetTCStatus(FullHandle_t DeviceHandle, STPTI_DebugTCStatus_t *TCStatus_p)
{
    TCPrivateData_t      *PrivateData_p = NULL;
    STPTI_TCParameters_t *TC_Params_p   = NULL;    
    TCDebug_t            *TCDebug_p     = NULL;
    
    PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);    
    TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;     
    TCDebug_p = (TCDebug_t*) TC_Params_p->TC_DebugStart;
    
    TCStatus_p->TotalPacketCount = TCDebug_p->DebugTCStatus.TotalPacketCount;
    TCStatus_p->TotalOutputPacketCount = TCDebug_p->DebugTCStatus.TotalOutputPacketCount;
}

/* EOF */
