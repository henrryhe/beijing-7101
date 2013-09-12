/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: debug.c
 Description: Debug Statistics functions.

******************************************************************************/

#ifndef STPTI_DEBUG_SUPPORT
    #error Incorrectly included file!
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#include <stdio.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"
#include "memget.h"


/* Functions --------------------------------------------------------------- */


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
ST_ErrorCode_t STPTI_DebugGetInterruptHistory (ST_DeviceName_t DeviceName, U32 *NoOfStructsStored,char *buf,int *len)
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
            *len += sprintf( buf+*len,"\n No Interrupts processed\n");
        }
        else
        {        
            *len += sprintf( buf+*len,"\nInterrupt History\n=================\nStatus    SlotHandle  BufferHandle  PTIBaseAddress  InterruptCount  DMANumber  Time      \n");
            *len += sprintf( buf+*len,"(hex)     (hex)       (hex)         (hex)           (dec)           (hex)      (hex)\n");

            if ( DebugInterruptStatus[IntInfoIndex].SlotHandle != 0 && DebugInterruptStatus[IntInfoIndex].InterruptCount != 0 ) /* if the next array element is non zero we have wrapped */
            {      
                int j=0;
                STPTI_DebugInterruptStatus_t *TempBuffer_p;

                /* the buffer is full */
                *NoOfStructsStored = IntInfoCapacity;


                for (i=IntInfoIndex, j=0; j < IntInfoCapacity; i=((i+1)%IntInfoCapacity), j++) 
                {               
                    *len += sprintf( buf+*len,"%08x  %08x    %08x      %08x        %010d      %08x   %08x\n", 
   	                                                    DebugInterruptStatus[i].Status,
	                                                    DebugInterruptStatus[i].SlotHandle,
	                                                    DebugInterruptStatus[i].BufferHandle,
	                                                    DebugInterruptStatus[i].PTIBaseAddress,
	                                                    DebugInterruptStatus[i].InterruptCount,
	                                                    DebugInterruptStatus[i].DMANumber,
	                                                    DebugInterruptStatus[i].Time); 
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
                    *len += sprintf( buf+*len,"%08x  %08x    %08x      %08x        %010d      %08x   %08x\n", 
   	                                                    DebugInterruptStatus[i].Status,
	                                                    DebugInterruptStatus[i].SlotHandle,
	                                                    DebugInterruptStatus[i].BufferHandle,
	                                                    DebugInterruptStatus[i].PTIBaseAddress,
	                                                    DebugInterruptStatus[i].InterruptCount,
	                                                    DebugInterruptStatus[i].DMANumber,
	                                                    DebugInterruptStatus[i].Time); 
                }                
            }            
        }
    }
    return Error;
}


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
ST_ErrorCode_t STPTI_DebugGetStatistics(ST_DeviceName_t DeviceName, STPTI_DebugStatistics_t *Statistics_p,char *buf,int *len)
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
        *len += sprintf( buf+*len, "\nSTPTI Debug Statistics:\n=======================\n");
        *len += sprintf( buf+*len, "BufferOverflowEventCount             %d\n", DebugStatistics.BufferOverflowEventCount);
        *len += sprintf( buf+*len, "CarouselCycleCompleteEventCount      %d\n", DebugStatistics.CarouselCycleCompleteEventCount);
        *len += sprintf( buf+*len, "CarouselEntryTimeoutEventCount       %d\n", DebugStatistics.CarouselEntryTimeoutEventCount);
        *len += sprintf( buf+*len, "CarouselTimedEntryEventCount         %d\n", DebugStatistics.CarouselTimedEntryEventCount);                        
        *len += sprintf( buf+*len, "CCErrorEventCount                    %d\n", DebugStatistics.CCErrorEventCount);
        *len += sprintf( buf+*len, "ClearChangeToScrambledEventCount     %d\n", DebugStatistics.ClearChangeToScrambledEventCount);
        *len += sprintf( buf+*len, "DMACompleteEventCount                %d\n", DebugStatistics.DMACompleteEventCount);
        *len += sprintf( buf+*len, "IndexMatchEventCount                 %d\n", DebugStatistics.IndexMatchEventCount);
        *len += sprintf( buf+*len, "InterruptFailEventCount              %d\n", DebugStatistics.InterruptFailEventCount);
        *len += sprintf( buf+*len, "InvalidDescramblerKeyEventCount      %d\n", DebugStatistics.InvalidDescramblerKeyEventCount);
        *len += sprintf( buf+*len, "InvalidLinkEventCount                %d\n", DebugStatistics.InvalidLinkEventCount);
        *len += sprintf( buf+*len, "InvalidParameterEventCount           %d\n", DebugStatistics.InvalidParameterEventCount);
        *len += sprintf( buf+*len, "PacketErrorEventCount                %d\n", DebugStatistics.PacketErrorEventCount);
        *len += sprintf( buf+*len, "PCRReceivedEventCount                %d\n", DebugStatistics.PCRReceivedEventCount);
        *len += sprintf( buf+*len, "ScrambledChangeToClearEventCount     %d\n", DebugStatistics.ScrambledChangeToClearEventCount);
        *len += sprintf( buf+*len, "SectionDiscardedOnCRCCheckEventCount %d\n", DebugStatistics.SectionDiscardedOnCRCCheckEventCount);
        *len += sprintf( buf+*len, "TCCodeFaultEventCount                %d\n", DebugStatistics.TCCodeFaultEventCount);
        *len += sprintf( buf+*len, "PacketSignalCount                    %d\n", DebugStatistics.PacketSignalCount);
        *len += sprintf( buf+*len, "InterruptBufferOverflowCount         %d\n", DebugStatistics.InterruptBufferOverflowCount);
        *len += sprintf( buf+*len, "InterruptBufferErrorCount            %d\n", DebugStatistics.InterruptBufferErrorCount);
        *len += sprintf( buf+*len, "InterruptStatusCorruptionCount       %d\n", DebugStatistics.InterruptStatusCorruptionCount);

                
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

/* ------------------------------------------------------------------------- */


/* --- EOF --- */
