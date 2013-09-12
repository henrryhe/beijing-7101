/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: stptidbg.h
 Description: Debug Statistics functions.

******************************************************************************/

#ifdef STPTI_DEBUG_SUPPORT
/* #define STTBX_PRINT*/
#else 
    #error Incorrectly included file!
#endif

#ifndef __PTI_STATS_H
 #define __PTI_STATS_H

/* Constants ----------------------------------------------------------- */
#define MAX_NO_OF_DMA_TXFRS_RECORDED 10

/* Exported Types ------------------------------------------------------ */

typedef struct
{
	U32 Top;
	U32 Base;
	U32 Read;
	U32 Write;
} STPTI_DebugDMAInfo_t;

typedef struct
{
	U32 Status;
	U32 SlotHandle;
	U32 BufferHandle;
	U32 PTIBaseAddress;
	U32 InterruptCount;
	U32 DMANumber;
	U32 Time;
} STPTI_DebugInterruptStatus_t;

typedef struct
{
	U32 BufferOverflowEventCount;
	U32 CarouselCycleCompleteEventCount;
	U32 CarouselEntryTimeoutEventCount;
	U32 CarouselTimedEntryEventCount;
	U32 CCErrorEventCount;
	U32 ClearChangeToScrambledEventCount;
	U32 DMACompleteEventCount;
	U32 IndexMatchEventCount;    
	U32 InterruptFailEventCount;
	U32 InvalidDescramblerKeyEventCount;
	U32 InvalidLinkEventCount;
    U32 InvalidParameterEventCount;
	U32 PacketErrorEventCount;
	U32 PCRReceivedEventCount;
	U32 ScrambledChangeToClearEventCount;
	U32 SectionDiscardedOnCRCCheckEventCount;
	U32 TCCodeFaultEventCount;
	U32 PacketSignalCount;
	U32 InterruptBufferOverflowCount;
	U32 InterruptBufferErrorCount;
	U32 InterruptStatusCorruptionCount;
	U32 PESErrorEventCount;	
} STPTI_DebugStatistics_t;


typedef struct STPTI_TCRegisters_s
{
    U16 RegA;
    U16 RegB;
    U16 RegC;
    U16 RegD;
    U16 RegP;
    U16 RegQ;
    U16 RegI;
    U16 RegO;
    U16 IPtr;
    U16 RegE0;
    U16 RegE1;
    U16 RegE2;
    U16 RegE3;
    U16 RegE4;
    U16 RegE5;
    U16 RegE6;
    U16 RegE7;
} STPTI_TCRegisters_t;

typedef struct STPTI_DebugTCStatus_s
{
    U32 TotalPacketCount;
    U32 TotalOutputPacketCount;
    U32 Reserved1;
    U32 Reserved2;
    U32 Reserved3;
    U32 Reserved4;   
    U32 Reserved5;
} STPTI_DebugTCStatus_t;

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCDebug_t)
#endif /* #endif defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1) */

typedef volatile struct
{
    U32 DMAHistoryIndex:15;
    U32 DMAHistoryWrapped:1; 
    U32 DMAHistoryTCWriting:16;
    
    U32 DMAHistoryST20Reading;
    
    STPTI_DebugDMAInfo_t DMAHistory[MAX_NO_OF_DMA_TXFRS_RECORDED];
    STPTI_DebugTCStatus_t DebugTCStatus;        
}TCDebug_t;

/* Exported Variables -------------------------------------------------- */
extern STPTI_DebugStatistics_t      DebugStatistics;
extern STPTI_DebugInterruptStatus_t *DebugInterruptStatus;
extern int  IntInfoCapacity;
extern BOOL IntInfoStart;
extern int  IntInfoIndex;

/* Exported Function Prototypes ---------------------------------------- */
#if defined(ST_OSLINUX)
ST_ErrorCode_t STPTI_DebugInterruptHistoryStart(ST_DeviceName_t DeviceName, STPTI_DebugInterruptStatus_t *History_p, U32 Size);

ST_ErrorCode_t STPTI_DebugGetInterruptHistory(ST_DeviceName_t DeviceName, U32 *NoOfStructsStored, char *buf, int *len);

ST_ErrorCode_t STPTI_DebugStatisticsStart(ST_DeviceName_t DeviceName);
ST_ErrorCode_t STPTI_DebugGetStatistics(ST_DeviceName_t DeviceName, STPTI_DebugStatistics_t *Statistics_p,char *buf,int *le);
#else
ST_ErrorCode_t STPTI_DebugGetDMAHistory(ST_DeviceName_t DeviceName, U32 *NoOfStructsStored_p, STPTI_DebugDMAInfo_t *History_p, U32 Size);
ST_ErrorCode_t STPTI_DebugInterruptHistoryStart(ST_DeviceName_t DeviceName, STPTI_DebugInterruptStatus_t *History_p, U32 Size);
ST_ErrorCode_t STPTI_DebugGetInterruptHistory(ST_DeviceName_t DeviceName, U32 *NoOfStructsStored);
ST_ErrorCode_t STPTI_DebugStatisticsStart(ST_DeviceName_t DeviceName);
ST_ErrorCode_t STPTI_DebugGetStatistics(ST_DeviceName_t DeviceName, STPTI_DebugStatistics_t *Statistics_p);
ST_ErrorCode_t STPTI_DebugGetTCRegisters (ST_DeviceName_t DeviceName, STPTI_TCRegisters_t *TCRegisters_p);
ST_ErrorCode_t STPTI_DebugGetTCStatus(ST_DeviceName_t DeviceName, STPTI_DebugTCStatus_t *TCStatus_p);
ST_ErrorCode_t STPTI_DebugGetTCDRAM(ST_DeviceName_t DeviceName);
#endif /*  #endif..else defined(ST_OSLINUX) */

#endif  /* __PTI_STATS_H */


