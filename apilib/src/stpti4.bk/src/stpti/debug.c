/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: debug.c
 Description: Debug Statistics functions.

******************************************************************************/

#ifdef STPTI_DEBUG_SUPPORT
#if !defined(ST_OSLINUX)
 #define STTBX_PRINT
#endif /* #endif !defined(ST_OSLINUX) */
#else 
    #error Incorrectly included file!
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"
#include "memget.h"

#if defined(ST_OSLINUX)
#undef STTBX_Print
#define PROC_Print(fmt,rest...) sprintf( buf+*len,fmt,##rest)
#define STTBX_Print(args) *len += PROC_Print args;
#endif /* #endif defined(ST_OSLINUX) */

/* Functions --------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
/******************************************************************************
Function Name : STPTI_DebugGetDMAHistory
  Description : This halts a debug session for collating DMA transaction 
                history.  
                If STTBX_Print is enabled the data will be printed.
   Parameters : 
******************************************************************************/
ST_ErrorCode_t STPTI_DebugGetDMAHistory (ST_DeviceName_t DeviceName, U32 *NoOfStructsStored_p, STPTI_DebugDMAInfo_t *History_p, U32 Size)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    

    if (! stpti_DeviceNameExists(DeviceName) )
    {
        Error = ST_ERROR_UNKNOWN_DEVICE;    
    }
    else if (NoOfStructsStored_p == NULL)
    {
        Error = ST_ERROR_BAD_PARAMETER;        
    }
    else if (History_p == NULL)
    {  
        Error = STPTI_ERROR_NOT_INITIALISED;
    }

    /* 0 < Size <= MAX_NO_OF_DMA_TXFRS_RECORDED  Only space for ten */
    else if (Size < 1 || (Size/sizeof (STPTI_DebugDMAInfo_t)) > MAX_NO_OF_DMA_TXFRS_RECORDED)
    {    
        Error = ST_ERROR_BAD_PARAMETER;
    }

    else /* finished checking parameters */
    {
        FullHandle_t        DeviceHandle;          

      	STOS_SemaphoreWait(stpti_MemoryLock);    	   
        
        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);        
        stptiHAL_DebugGetDMAHistory(DeviceHandle, NoOfStructsStored_p, History_p, Size);   
    	
        STOS_SemaphoreSignal(stpti_MemoryLock);
        
    }
    return Error;
}
#endif /* #endif !defined(ST_OSLINUX */

/******************************************************************************
Function Name : STPTI_DebugInterruptHistoryStart
  Description :
   Parameters :
******************************************************************************/                        
ST_ErrorCode_t STPTI_DebugInterruptHistoryStart (ST_DeviceName_t DeviceName, STPTI_DebugInterruptStatus_t *History_p, U32 Size)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (History_p == NULL) /* check parameters */
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else if (Size < sizeof (STPTI_DebugInterruptStatus_t))
    {
        Error = ST_ERROR_BAD_PARAMETER;        
    }
    else if (! stpti_DeviceNameExists(DeviceName) )
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else      /* finished checking parameters */
    {
        DebugInterruptStatus = History_p;
        IntInfoCapacity = (Size/sizeof (STPTI_DebugInterruptStatus_t)); /* make sure size is a whole number of STPTI_DebugInterruptStatus_t */
        memset (History_p, 0, Size );       /* clear the buffer */
        IntInfoStart = TRUE;                /* start collecting debug */
        IntInfoIndex = 0;
    }            
    return Error;
}


/******************************************************************************
Function Name : STPTI_DebugGetInterruptHistory
  Description :
   Parameters :
******************************************************************************/
#if defined(ST_OSLINUX)
ST_ErrorCode_t STPTI_DebugGetInterruptHistory (ST_DeviceName_t DeviceName, U32 *NoOfStructsStored,char *buf,int *len)
#else
ST_ErrorCode_t STPTI_DebugGetInterruptHistory (ST_DeviceName_t DeviceName, U32 *NoOfStructsStored)
#endif /* #if..else defined(ST_OSLINUX */
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    if (DebugInterruptStatus == NULL)
    {  
        return STPTI_ERROR_NOT_INITIALISED;
    }
    else if (NoOfStructsStored == NULL)
    {
        Error = ST_ERROR_BAD_PARAMETER;        
    }
    else if (! stpti_DeviceNameExists(DeviceName) )
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else /* finished checking parameters */
    {
        int i;

        IntInfoStart = FALSE; /* stop debug collection */        

        if (IntInfoIndex == 0 && DebugInterruptStatus[IntInfoIndex].SlotHandle == 0)
        {        
            /* No packets have been received */
            *NoOfStructsStored = 0;
            STTBX_Print(("\n No Interrupts processed\n"));
        }
        else
        {        
            STTBX_Print(("\nInterrupt History\n=================\nStatus    SlotHandle  BufferHandle  PTIBaseAddress  InterruptCount  DMANumber  Time      \n"));
            STTBX_Print(("(hex)     (hex)       (hex)         (hex)           (dec)           (hex)      (hex)\n"));            

            if ( DebugInterruptStatus[IntInfoIndex].SlotHandle != 0 && DebugInterruptStatus[IntInfoIndex].InterruptCount != 0 ) /* if the next array element is non zero we have wrapped */
            {      
                int j=0;
                STPTI_DebugInterruptStatus_t *TempBuffer_p;

                /* the buffer is full */
                *NoOfStructsStored = IntInfoCapacity;


                for (i=IntInfoIndex, j=0; j < IntInfoCapacity; i=((i+1)%IntInfoCapacity), j++) 
                {               
                    STTBX_Print(("%08x  %08x    %08x      %08x        %010d      %08x   %08x\n", 
   	                                                    DebugInterruptStatus[i].Status,
	                                                    DebugInterruptStatus[i].SlotHandle,
	                                                    DebugInterruptStatus[i].BufferHandle,
	                                                    DebugInterruptStatus[i].PTIBaseAddress,
	                                                    DebugInterruptStatus[i].InterruptCount,
	                                                    DebugInterruptStatus[i].DMANumber,
	                                                    DebugInterruptStatus[i].Time)); 
                }

                /* rearrange the data so the oldest entry is in DebugInterruptStatus[0] */
#if defined(ST_OSLINUX)
                TempBuffer_p = kmalloc ( IntInfoIndex * sizeof (STPTI_DebugInterruptStatus_t), GFP_KERNEL );
#else
                TempBuffer_p = malloc ( IntInfoIndex * sizeof (STPTI_DebugInterruptStatus_t) );
#endif /* #endif defined(ST_OSLINUX) */
                if (TempBuffer_p == NULL) 
                {
                    return ST_ERROR_NO_MEMORY;
                }
                /* copy data prior to index into temp buffer */
                memcpy(TempBuffer_p, DebugInterruptStatus, (IntInfoIndex * sizeof (STPTI_DebugInterruptStatus_t)) ); 
                /* copy data between index and top of buffer into the bottom of the buffer */                
                memcpy(DebugInterruptStatus, &(DebugInterruptStatus[IntInfoIndex]), ((IntInfoCapacity - IntInfoIndex) * sizeof (STPTI_DebugInterruptStatus_t)) ); 
                /* copy data from TempBuffer_p into top of the debug buffer */                
                memcpy(&(DebugInterruptStatus[IntInfoCapacity - IntInfoIndex]), TempBuffer_p, (IntInfoIndex * sizeof (STPTI_DebugInterruptStatus_t)) ); 
            }
            else
            {
                /* the buffer has not wrapped */
                *NoOfStructsStored = IntInfoIndex;;

                for (i=0; i < IntInfoIndex; i++) 
                {
                    STTBX_Print(("%08x  %08x    %08x      %08x        %010d      %08x   %08x\n", 
   	                                                    DebugInterruptStatus[i].Status,
	                                                    DebugInterruptStatus[i].SlotHandle,
	                                                    DebugInterruptStatus[i].BufferHandle,
	                                                    DebugInterruptStatus[i].PTIBaseAddress,
	                                                    DebugInterruptStatus[i].InterruptCount,
	                                                    DebugInterruptStatus[i].DMANumber,
	                                                    DebugInterruptStatus[i].Time)); 
                }                
            }            
        }
    }
    return Error;
}

#if !defined(ST_OSLINUX)
/******************************************************************************
Function Name : STPTI_DebugGetTCRegisters
  Description : Peeks the contents of the TC Registers.  
                If STTBX_PRINT is defined it also prints the contents
   Parameters : DeviceName - The PTI device to interrogate.
                TCRegisters_p - The structure that is populated with the TC's
                register contents.   
******************************************************************************/
ST_ErrorCode_t STPTI_DebugGetTCRegisters (ST_DeviceName_t DeviceName, STPTI_TCRegisters_t *TCRegisters_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;
    
    if (TCRegisters_p == NULL) /* check parameters */
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else if (! stpti_DeviceNameExists(DeviceName) )
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else /* finished checking parameters */
    {
      	STOS_SemaphoreWait(stpti_MemoryLock);    	   
        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            stptiHAL_DebugGetTCRegisters(DeviceHandle, TCRegisters_p);
        }        
    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }         
    return Error;
}


/******************************************************************************
Function Name : STPTI_DebugGetTCDRAM
  Description : Dump the contents of the TC Data Ram.
   Parameters : DeviceName - The PTI device to interrogate.
******************************************************************************/
ST_ErrorCode_t STPTI_DebugGetTCDRAM (ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;
    
    if (! stpti_DeviceNameExists(DeviceName) ) /* check parameters */
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else /* finished checking parameters */
    {
        STOS_SemaphoreWait(stpti_MemoryLock);    
	   
        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            stptiHAL_DebugDumpTCDRam(DeviceHandle);
        }
        
    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }         
    return Error;
}
#endif /* #endif !defined(ST_OSLINUX) */

/******************************************************************************
Function Name : STPTI_DebugStatisticsStart
  Description :
   Parameters : DeviceName - The PTI device to interrogate.
******************************************************************************/
ST_ErrorCode_t STPTI_DebugStatisticsStart (ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if (! stpti_DeviceNameExists(DeviceName) )
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else /* finished checking parameters */
    {
        /* reset counters */
        DebugStatistics.BufferOverflowEventCount = 0;
        DebugStatistics.CarouselCycleCompleteEventCount = 0;
        DebugStatistics.CarouselEntryTimeoutEventCount = 0;
        DebugStatistics.CarouselTimedEntryEventCount = 0;
        DebugStatistics.CCErrorEventCount = 0;
        DebugStatistics.ClearChangeToScrambledEventCount = 0;
        DebugStatistics.DMACompleteEventCount = 0;
        DebugStatistics.InterruptFailEventCount = 0;
        DebugStatistics.InvalidDescramblerKeyEventCount = 0;
        DebugStatistics.InvalidLinkEventCount = 0;
        DebugStatistics.InvalidParameterEventCount = 0;
        DebugStatistics.PacketErrorEventCount = 0;
        DebugStatistics.PCRReceivedEventCount = 0;
        DebugStatistics.ScrambledChangeToClearEventCount = 0;
        DebugStatistics.SectionDiscardedOnCRCCheckEventCount = 0;
        DebugStatistics.TCCodeFaultEventCount = 0;
        DebugStatistics.PacketSignalCount = 0;
        DebugStatistics.InterruptBufferOverflowCount = 0;
        DebugStatistics.InterruptBufferErrorCount = 0;
        DebugStatistics.InterruptStatusCorruptionCount = 0;
        DebugStatistics.IndexMatchEventCount = 0;        
    }            
    return Error;
}
  
/******************************************************************************
Function Name : STPTI_DebugGetStatistics
  Description :
   Parameters : DeviceName - The PTI device to interrogate.
******************************************************************************/ 
#if defined(ST_OSLINUX)
ST_ErrorCode_t STPTI_DebugGetStatistics(ST_DeviceName_t DeviceName, STPTI_DebugStatistics_t *Statistics_p,char *buf,int *len)
#else
ST_ErrorCode_t STPTI_DebugGetStatistics(ST_DeviceName_t DeviceName, STPTI_DebugStatistics_t *Statistics_p)
#endif /* #endif..else defined(ST_OSLINUX) */
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (Statistics_p == NULL) /* check parameters */
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else if (! stpti_DeviceNameExists(DeviceName) ) /* check parameters */
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else /* finished checking parameters */
    {
        STTBX_Print(("\nSTPTI Debug Statistics:\n=======================\n"));
        STTBX_Print(("BufferOverflowEventCount             %d\n", DebugStatistics.BufferOverflowEventCount));
        STTBX_Print(("CarouselCycleCompleteEventCount      %d\n", DebugStatistics.CarouselCycleCompleteEventCount));
        STTBX_Print(("CarouselEntryTimeoutEventCount       %d\n", DebugStatistics.CarouselEntryTimeoutEventCount));
        STTBX_Print(("CarouselTimedEntryEventCount         %d\n", DebugStatistics.CarouselTimedEntryEventCount));                        
        STTBX_Print(("CCErrorEventCount                    %d\n", DebugStatistics.CCErrorEventCount));
        STTBX_Print(("ClearChangeToScrambledEventCount     %d\n", DebugStatistics.ClearChangeToScrambledEventCount));
        STTBX_Print(("DMACompleteEventCount                %d\n", DebugStatistics.DMACompleteEventCount));
        STTBX_Print(("IndexMatchEventCount                 %d\n", DebugStatistics.IndexMatchEventCount));
        STTBX_Print(("InterruptFailEventCount              %d\n", DebugStatistics.InterruptFailEventCount));
        STTBX_Print(("InvalidDescramblerKeyEventCount      %d\n", DebugStatistics.InvalidDescramblerKeyEventCount));
        STTBX_Print(("InvalidLinkEventCount                %d\n", DebugStatistics.InvalidLinkEventCount));
        STTBX_Print(("InvalidParameterEventCount           %d\n", DebugStatistics.InvalidParameterEventCount));
        STTBX_Print(("PacketErrorEventCount                %d\n", DebugStatistics.PacketErrorEventCount));
        STTBX_Print(("PCRReceivedEventCount                %d\n", DebugStatistics.PCRReceivedEventCount));
        STTBX_Print(("ScrambledChangeToClearEventCount     %d\n", DebugStatistics.ScrambledChangeToClearEventCount));
        STTBX_Print(("SectionDiscardedOnCRCCheckEventCount %d\n", DebugStatistics.SectionDiscardedOnCRCCheckEventCount));
        STTBX_Print(("TCCodeFaultEventCount                %d\n", DebugStatistics.TCCodeFaultEventCount));
        STTBX_Print(("PacketSignalCount                    %d\n", DebugStatistics.PacketSignalCount));
        STTBX_Print(("InterruptBufferOverflowCount         %d\n", DebugStatistics.InterruptBufferOverflowCount));
        STTBX_Print(("InterruptBufferErrorCount            %d\n", DebugStatistics.InterruptBufferErrorCount));
        STTBX_Print(("InterruptStatusCorruptionCount       %d\n", DebugStatistics.InterruptStatusCorruptionCount));

                
        Statistics_p->BufferOverflowEventCount             = DebugStatistics.BufferOverflowEventCount;
        Statistics_p->CarouselCycleCompleteEventCount      = DebugStatistics.CarouselCycleCompleteEventCount;
        Statistics_p->CarouselEntryTimeoutEventCount       = DebugStatistics.CarouselEntryTimeoutEventCount;
        Statistics_p->CarouselTimedEntryEventCount         = DebugStatistics.CarouselTimedEntryEventCount;        
        Statistics_p->CCErrorEventCount                    = DebugStatistics.CCErrorEventCount;
        Statistics_p->ClearChangeToScrambledEventCount     = DebugStatistics.ClearChangeToScrambledEventCount;
        Statistics_p->DMACompleteEventCount                = DebugStatistics.DMACompleteEventCount;
        Statistics_p->IndexMatchEventCount                 = DebugStatistics.IndexMatchEventCount;        
        Statistics_p->InterruptFailEventCount              = DebugStatistics.InterruptFailEventCount;
        Statistics_p->InvalidDescramblerKeyEventCount      = DebugStatistics.InvalidDescramblerKeyEventCount;
        Statistics_p->InvalidLinkEventCount                = DebugStatistics.InvalidLinkEventCount;
        Statistics_p->InvalidParameterEventCount           = DebugStatistics.InvalidParameterEventCount;
        Statistics_p->PacketErrorEventCount                = DebugStatistics.PacketErrorEventCount;
        Statistics_p->PCRReceivedEventCount                = DebugStatistics.PCRReceivedEventCount;
        Statistics_p->ScrambledChangeToClearEventCount     = DebugStatistics.ScrambledChangeToClearEventCount;
        Statistics_p->SectionDiscardedOnCRCCheckEventCount = DebugStatistics.SectionDiscardedOnCRCCheckEventCount;
        Statistics_p->TCCodeFaultEventCount                = DebugStatistics.TCCodeFaultEventCount;
        Statistics_p->PacketSignalCount                    = DebugStatistics.PacketSignalCount;
        Statistics_p->InterruptBufferOverflowCount         = DebugStatistics.InterruptBufferOverflowCount;
        Statistics_p->InterruptBufferErrorCount            = DebugStatistics.InterruptBufferErrorCount;
        Statistics_p->InterruptStatusCorruptionCount       = DebugStatistics.InterruptStatusCorruptionCount;        

    }
    return Error;
}
#if !defined(ST_OSLINUX)
/******************************************************************************
Function Name : STPTI_DebugGetTCStatus
  Description :
   Parameters : DeviceName - The PTI device to interrogate.
******************************************************************************/
ST_ErrorCode_t STPTI_DebugGetTCStatus(ST_DeviceName_t DeviceName, STPTI_DebugTCStatus_t *TCStatus_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if (TCStatus_p == NULL) /* check parameters */
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else if (! stpti_DeviceNameExists(DeviceName) ) /* check parameters */
    {
        Error = ST_ERROR_UNKNOWN_DEVICE ;    
    }
    else /* finished checking parameters */
    {
        FullHandle_t        DeviceHandle;
      	STOS_SemaphoreWait(stpti_MemoryLock);    	      
        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);        
        stptiHAL_DebugGetTCStatus(DeviceHandle, TCStatus_p);       	
        STOS_SemaphoreSignal(stpti_MemoryLock);    
        STTBX_Print(("\nTCStatus:\n========\n"));
        STTBX_Print(("TotalPacketCount = %d\n", TCStatus_p->TotalPacketCount));
    }
    return ST_NO_ERROR;
}
#endif /* #endif !defined(ST_OSLINUX) */
/* ------------------------------------------------------------------------- */


/* --- EOF --- */
