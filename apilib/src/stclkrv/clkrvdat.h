/****************************************************************************

File Name   : clkrvdat.h

Description : Clock Recovery data structures

Copyright (C) 2007, STMicroelectronics

Revision History    :

    [...]

References  :

$ClearCase (VOB: stclkrv)S

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 5.0

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */


/* Define to prevent recursive inclusion */
#ifndef __CLKRVDAT_H
#define __CLKRVDAT_H

/* Includes --------------------------------------------------------------- */

#include "stcommon.h"
#include "stos.h"

#if defined(ST_OSLINUX)
#include "stevt.h"
#include "stdevice.h"
typedef volatile U32 STSYS_DU32;
#endif

#ifndef STCLKRV_NO_PTI
#ifdef ST_5188
#include "stdemux.h"
#else
#include "stpti.h"
#endif
#endif /* STCLKRV_NO_PTI */

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include "stsys.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* HW related Offset and values */

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)\
 || defined (ST_5188) || defined (ST_5525)
#define SD_BIT_OFFSET                 6
#elif defined (ST_7200)
#define SD_BIT_OFFSET                 22
#define MD_BIT_OFFSET                 16
#endif

#define COMPLEMENT2S_FOR_5BIT_VALUE   32

#define STCLKRV_PCR_VALID_ID          0         /* Offsets for events array  */
#define STCLKRV_PCR_INVALID_ID        1
#define STCLKRV_PCR_GLITCH_ID         2

#define STC_REBASELINE_TIME           10000000u /* 10secs in usecs units */
#define ENCODER_FREQUENCY             27000000

#define PCRTIMEFACTOR                 40
#define ACCURACY_FACTOR               100
#define CONV90K_TO_27M                300       /* 90kHz to 27MHz conversion */
#define PCR_STALE_SHIFT               24        /* stale shift limit on PCR  */

#define MAX_NO_IN_FREQ_TABLE          55        /* Number of Enteris In Frequency Table */

#if defined(ST_7100) || defined (ST_7109)
#define STCLKRV_NO_OF_CLOCKS          5         /* SD/PCM0/HD0/SPDIF/PCM1 */
#elif defined ST_7710
#define STCLKRV_NO_OF_CLOCKS          (3 + 1)   /* SD/PCM0/HD0 + extra clock not used */
#elif defined (ST_5525)
#define STCLKRV_NO_OF_CLOCKS          (1 + 5)   /* SD/PCM0/1/2/3/SPDIF0 */
#elif defined (ST_7200)
#define STCLKRV_NO_OF_CLOCKS          (1 + 7)   /* SD/PCM0/1/2/3/SPDIF0/HDMI/HD */
#else /* 5100/5105/5301/5188 */
#define STCLKRV_NO_OF_CLOCKS          3         /* SD/PCM0/SPDIF */
#endif

#ifndef STCLKRV_INTERRUPT_TIME_SPAN
#define STCLKRV_INTERRUPT_TIME_SPAN    25        /* Time Span between 2 interrupts in secs */
#endif

/* For GNBvd44290 */
#define STCLKRV_INTERRUPT_TIME_FIRST    5

#ifndef GRADUAL_CORRECTION_FACTOR
#define GRADUAL_CORRECTION_FACTOR     2         /* To apply Correction to PWM gradually */
#endif

#define PRECISION_FACTOR              100       /* To handle precision loss in case of division */

#ifndef STCLKRV_FILTER_STACK_SIZE
#if defined(ARCHITECTURE_ST40)
#define STCLKRV_FILTER_STACK_SIZE     16*1024
#elif defined(ARCHITECTURE_ST200)
#define STCLKRV_FILTER_STACK_SIZE     4*1024
#else
#define STCLKRV_FILTER_STACK_SIZE     1*1024
#endif
#endif

#ifndef STCLKRV_FILTER_TASK_PRIORITY
#ifdef ST_OSLINUX
#define STCLKRV_FILTER_TASK_PRIORITY  99
#else
#define STCLKRV_FILTER_TASK_PRIORITY  MAX_USER_PRIORITY
#endif
#endif


/* MPEG doesn't species any minimum time limit between PCR,
   But PCR Burst trouble us see DDTS - 28442 */
#define MINIMUM_TIME_BETWEEN_PCR      27000 * 20   /* For 1 millisecond = ~27000000/1000 */
                                                   /* For 2 millisecond = 54000 */
/* Internal state machine    */
enum PCREvent_State_e {
STCLKRV_STATE_PCR_UNKNOWN,
STCLKRV_STATE_PCR_OK,
STCLKRV_STATE_PCR_INVALID,
STCLKRV_STATE_PCR_GLITCH
};

/* Specifies the validity of PCRs when pass through Low Pass Filter */
enum PCRState_e {
PCR_NO_UPDATE,
PCR_UPDATE_OK,
PCR_UPDATE_THIS_LAST_GLITCH,
PCR_INVALID,
PCR_BURST
};


/* Private Types ---------------------------------------------------------- */
#ifndef STCLKRV_NO_PTI
typedef struct
{
    STCLKRV_ExtendedSTC_t       STC;                 /* Given data      */
    STCLKRV_STCSource_t         STCReferenceControl; /* Control switch  */
} stclkrv_STCBaseline_t;
#endif

typedef struct stclkrv_Context_s
{
    BOOL Reset;
    S32  PrevDiff;
    U32  PrevPcr;
    S32  OutStandingErr;
    S32  ControlValue;
    S32  *ErrorStore_p;
    S32  ErrorStoreCount;
    U32  Head;
    S32  ErrorSum;
    U32  MaxFrequencyError;
} stclkrv_Context_t;

typedef struct stclkrv_ReferencePCR_s
{
    BOOL AwaitingPCR;
    BOOL Valid;
    BOOL PCRHandlingActive;
    U32  PcrBaseBit32;
    U32  PcrBaseValue;
    U32  PcrExtension;
    U32  PcrArrivalBaseBit32;
    U32  PcrArrivalBaseValue;
    U32  PcrArrivalExtension;
    U32  TotalValue;               /* PCRBase * CONV90K_TO_27M + PCRExtension (27MHz) */
    U32  Time;
    U32  GlitchCount;
    BOOL Glitch;
    U32  MachineState;
} stclkrv_ReferencePCR_t;

typedef struct
{
    BOOL        Active;            /* Control */
    clock_t     Timer;

} stclkrv_FreerunSTC_t;

typedef struct FilterDataSet
{
    U32 TotalSTCTime;              /* STC total value */
    U32 TotalPCRTime;              /* PCR total value */
    S32 Difference;                /* STC, PCR relative difference */
    U32 PCRTickDiff;               /* Difference in two succesive PCRs */
    S32 CurrentTicksErrorValue;    /* Tick error = (S1-S0)-(P1-P0)*/
    S32 CurrentFreqErrorValue;     /* tick error diviveded by time elapsed */
    S32 AverageError;              /* Moving Average error */
    S32 AverageErrorPerSample;     /* Gradual error */
    U32 ExecutionTime;             /* Weighing Filter execution time */
    U32 SD_STC_Frequency;          /* SD or STC frequency */
}FilterData;


/* Nominal Freq Look Up Table Structure */
typedef struct stclkrv_Freqtable_s
{
    U32         Frequency;
    U16         SDIV;
    S8          MD;
    U16         IPE;
    U16         Step;
    U32         Ratio;
}stclkrv_Freqtable_t;

/* Clock Context One Master(STC/SD) two slaves(PCM,SPDIF/HD) */
typedef struct stclkrv_FS_s
{
    STCLKRV_ClockSource_t ClockType; /* Type of the clock */
    U32         Frequency;           /* Frequency of the clock */
    U16         SDIV;                /* FS Block Param - see Freq. Table */
    S8          MD;                  /* FS Block Param - see Freq. Table */
    U16         IPE;                 /* FS Block Param  Fine Selector- see Freq. Table */
    U32         Step;                /* Frequency change per IPE change */
    BOOL        Valid;               /* Flag for valid frequency(Slave), FS0 - STC/SD always valid */
    U32         PrevCounter;         /* Slave clock counter value */
    U32         Ratio;               /* Ratio of Slave's clock nominal frequency with Master ~27 Mhz*/
    S32         DriftError;          /* Slaves clock's Drift error */
    U32         TicksToElapse;       /* Expected Clock ticks of Slave */
    BOOL        SlaveWithCounter;             /* Flag for master/slave  */
} stclkrv_FS_t;

/* 5100 Hardware Context */
typedef struct stclkrv_HW_context_s
{
    stclkrv_FS_t    Clocks[STCLKRV_NO_OF_CLOCKS];
#if defined (ST_7710)||defined(ST_7100) || defined (ST_7109)
    STCLKRV_ApplicationMode_t ApplicationMode;
#endif
    U32 ClockIndex;

} stclkrv_HW_context_t;

/* Multi-instance link list control block */
typedef struct STCLKRV_ControlBlock_s
{
    struct STCLKRV_ControlBlock_s *Next;          /* Next  in link list */
#if defined(ST_OSLINUX)
    /* Linux ioremap addresses */
    unsigned long                 *FSMappedAddress_p;
    unsigned long                 *AUDFSMappedAddress_p;
    unsigned long                 *CRUMappedAddress_p;
#endif
    ST_DeviceName_t               DeviceName;
    char					RecoveryName[20];
    U32                           OpenCount;      /* OpenCount per instance */
    STCLKRV_InitParams_t          InitPars;       /* Copy of Initialisation params */
    stclkrv_ReferencePCR_t        ActvRefPcr;     /* Active pcr reference data */
    stclkrv_Context_t             ActvContext;    /* Active context data */
    stclkrv_HW_context_t          HWContext;      /* CLKRV HW module data */
    STEVT_Handle_t                ClkrvEvtHandle; /* Instance Event handle */
    STEVT_EventID_t               RegisteredEvents[STCLKRV_NB_REGISTERED_EVENTS];
    semaphore_t                   *InstanceSemaphore_p;
#ifndef STCLKRV_NO_PTI
    STEVT_Handle_t                PcrEvtHandle;   /* PCR event for callback from pti */
    stclkrv_STCBaseline_t         STCBaseline;    /* STC baseline value and control switch */
    stclkrv_FreerunSTC_t          FreerunSTC;     /* Free running STC control */
#ifdef ST_5188
    STDEMUX_Slot_t                Slot;           /* STPTI slot handle */
    STDEMUX_EventData_t           InstancePcrData;
#else
    STPTI_Slot_t                  Slot;           /* STPTI slot handle */
    STPTI_EventData_t             InstancePcrData;
#endif
#endif /* STCLKRV_NO_PTI */
    BOOL                          SlotEnabled;    /* STPTI Slot use enabled */

    /* Filter Task */
    tdesc_t                       FilterTaskDescriptor;
    task_t                        *FilterTask_p;
    void                          *FilterTaskStack_p;
    semaphore_t                   *FilterSemaphore_p;
    BOOL                          FilterTaskActive;    /* STPTI Slot use enabled */
    BOOL                          SlaveCorrectionBeingApplied;
    S32                           OffsetInSTC;

}STCLKRV_ControlBlock_t;

#if defined(ST_5100)
#pragma ST_device (DU32)
#endif

typedef volatile U32 DU32;

/* Private Variables ------------------------------------------------------*/


/* Private Macros --------------------------------------------------------- */

/* High priority timer access */
#if defined (ST_5100)
#define GetHPTimer(Value) __optasm{ldc 0; ldclock; st Value;}
#else
#define GetHPTimer(Value) {Value = STOS_time_now();}
#endif

#define CLKRV_DELAY(x) { int temp; for(temp=0;temp<x;temp++); }


/* Enter shared memory protection */
#if defined(ST_OS2O)||defined(ST_OS21)
#define EnterCriticalSection() if (task_context(NULL,NULL)==task_context_task) task_lock();   else interrupt_lock();
#define LeaveCriticalSection() if (task_context(NULL,NULL)==task_context_task) task_unlock(); else interrupt_unlock();
#else
#define EnterCriticalSection() interrupt_lock();
#define LeaveCriticalSection() interrupt_unlock();
#endif

/* Private Function prototypes -------------------------------------------- */

/* Program FS registers*/
ST_ErrorCode_t clkrv_ProgramFSRegs( STCLKRV_ControlBlock_t *TmpClkrvList_p,
                                                  STCLKRV_ClockSource_t Clock);

BOOL SlaveHaveCounter(STCLKRV_ClockSource_t ClockSource );
U32  FindClockIndex(STCLKRV_ControlBlock_t *CB_p, STCLKRV_ClockSource_t Clock);
U32  GetRegIndex(STCLKRV_ControlBlock_t *CB_p);

ST_ErrorCode_t clkrv_setClockFreq( STCLKRV_ControlBlock_t *CB_p,
                                   STCLKRV_ClockSource_t Clock,
                                   U32 Index );

/* Reset 5100 H/W Block */
ST_ErrorCode_t clkrv_InitHW( STCLKRV_ControlBlock_t *TmpClkrvList_p );

/* Register Read Write Function */
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
__inline void clkrv_writeReg(STSYS_DU32 *Base, U32 Reg, U32 Value);
__inline void clkrv_readReg (STSYS_DU32 *Base, U32 Reg, U32 *Value);
#elif defined(ST_OSLINUX)
static __inline void clkrv_writeReg(STSYS_DU32 *Base, U32 Reg, U32 Value)
{
#if defined(ST_7100) || defined(ST_7109)
    STSYS_WriteRegDev32LE((CKG_BASE_ADDRESS + 0x10), 0xC0DE);
#endif
    STSYS_WriteRegDev32LE((void *)((U32)Base + Reg), Value);
}
static __inline void clkrv_readReg (STSYS_DU32 *Base, U32 Reg, U32 *Value)
{
    *Value = STSYS_ReadRegDev32LE((void *)((U32)Base + Reg));
}
#endif

/* Filter Functions */
U32 ClockRecoveryFilter(U32 ExtdPCRTime, U32 ExtdSTCTime, STCLKRV_ControlBlock_t *Instance_p);


#ifdef CLKRV_FILTERDEBUG
void Dump_Debug_Data(U32 Code, FilterData *FilterData_p,
                        STCLKRV_ControlBlock_t *Instance_p);
#endif


/* Utility Functions */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLKRVDAT_H */


/* End of clkrvdat.h */

