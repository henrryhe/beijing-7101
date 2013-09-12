/****************************************************************************

File Name   : clkrvsim.c

Description : Clock Recovery API Routines

Copyright (C) 2006, STMicroelectronics

Revision History    :

References  :

$ClearCase (VOB: stclkrv)S

stclkrv.fm "Clock Recovery API"

****************************************************************************/


/* Includes ----------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "stclkrv.h"
#include "stdevice.h"
#include "stos.h"
#include "stevt.h"
#include "sttbx.h"

#include "clkrvsim.h"
#include "clkrvreg.h"

/* Private Constants -------------------------------------------------- */

#define K                           1000
#define M                           (K * K)
#define TICKS_IN_SECOND             ST_GetClocksPerSecond  /* for task delay of 1 sec */

#ifdef ST_OS21
#define TASK_STACK_SIZE             8*2048
#else
#define TASK_STACK_SIZE             2048
#endif
#define SD_VIDEO_FREQ               27000000
#define CLKRVSIM_ENCODER_MAX_FREQ   27006000
#define CLKRVSIM_ENCODER_MIN_FREQ   26994000

#define CLKRVSIM_JITTER_MIN         0
#define CLKRVSIM_JITTER_MAX         100000

/* Private Types ------------------------------------------------------ */

/* Store EVT Context */
typedef struct
{
    ST_DeviceName_t      EVTDevNamPcr;
    STEVT_Handle_t       EVTHandlePcr;

}CLKRVSIM_EVT_t;

/* Task States */
typedef enum
{
    NOT_INITIALIZED = 0,
    INITIALIZED,
    RUNNING

}CLKRVSIM_TaskState_t;

/* Task Control Block */
typedef struct
{
    FILE*                  Logfile_p;               /* Pointer to Log File */
    tdesc_t                simulator_desc;
    void                   *SimulatorTaskStack_p;
    semaphore_t            *TaskSemaphore_p;
    task_t                 *simulator_task_p;
    CLKRVSIM_TaskState_t   State;                   /* Simulator Task State */
    ST_Partition_t         *SysPartition_p;         /* Memory partition to use (mi) */
    ST_Partition_t         *InternalPartition_p;    /* Memory partition to use (mi) */

}CLKRVSIM_TCB_t;

/* Simulator Context */
typedef struct
{
    U32                     EncoderFrequency;     /* Encoder Frequency From Test App */
    U32                     Jitter;               /* Jitter value */
    U8                      StreamStatus;         /* Real Time disturbance to be introduced */
    U32                     PCR_Interval;         /* In ms */
    U32                     PCR_DriftThreshold;
    U8                      DriftCorrection;      /* Flag to check if DriftCorrection is ON */
#ifdef ST_OS21                                    /* clock recovery module HW interrupt number */
    interrupt_name_t        InterruptNumber;
#else
    U32                     InterruptNumber;
#endif
    CLKRVSIM_TCB_t          TaskContext;
    CLKRVSIM_EVT_t          Evt;
    FILE                    *TestFile_p;          /* File Pointer for a File Read Test */
    BOOL                    FileTest;             /* Flag to check for File Read Test */
    BOOL                    SimReset;             /* Flag to check if Simulator needs ot be reset */

}CLKRVSIM_Context_t;

/* Private Variables -------------------------------------------------- */

U32                         CLKRVSIM_RegArray[CLKRVSIM_REG_ARRAY_SIZE];
STEVT_EventID_t             CLKRVEventIDSIM;
#ifdef ST_5188
static STDEMUX_EventData_t  PcrDataSim;
#else
static STPTI_EventData_t    PcrDataSim;
#endif
static CLKRVSIM_Context_t   *ControlObject_p;      /* Instance of the Simulator */

/* Globals ------------------------------------------------------------- */




/* Private Macros ----------------------------------------------------- */

#define GET_SDIV(a) ((1)<<((a) + 1))

/* Private Function prototypes ---------------------------------------- */

/* EVT Related */
static ST_ErrorCode_t CLKRVSIM_EVTInit(void);
static ST_ErrorCode_t CLKRVSIM_EVTTerm(void);

static S32 NetworkJitter(S32 MaxJitter);

/* Main Simulator Task */
void SimulatorTask(void *voidptr);
void exit_simulator_task(task_t* task, int param);

/* Register Read/Write */
__inline void ClkrvSim_RegWrite(U32 *BaseAddress, U32 Register, U32 Value);
__inline void ClkrvSim_RegRead(U32 *BaseAddress, U32 Register, U32* ReadVal );

/* externs ------------------------------------------------------------ */
#define CLKRV_MAX_SAMPLES  5000
extern U32 ClkrvDbg[13][CLKRV_MAX_SAMPLES];
extern U32 Clkrv_Slave_Dbg[5][CLKRV_MAX_SAMPLES];
extern U32 SampleCount;

/* Functions ---------------------------------------------------------- */

/****************************************************************************
Name         : CLKRVSIM_Init()

Description  : Initializes the Simulator

Parameters   : Pointer to STCLKRVSIM_InitParams_t
               Double Pointer to CLKRVBaseAddress

Return Value : ST_ErrorCode_t
               ST_NO_ERROR               Successful completion
               ST_ERROR_NO_MEMORY        Memory Allocation Faliure
See Also     :
 ****************************************************************************/

ST_ErrorCode_t STCLKRVSIM_Init(STCLKRVSIM_InitParams_t *SimInitParams_p,CLKRVBaseAddress **BaseAddr_p)
{
    ST_ErrorCode_t  Error  = ST_NO_ERROR;

    /* Return Base Address of Reg Array */
    memset(CLKRVSIM_RegArray,0,(CLKRVSIM_REG_ARRAY_SIZE * 4));
    *BaseAddr_p =  CLKRVSIM_RegArray ;

    ControlObject_p = (CLKRVSIM_Context_t*) memory_allocate_clear(SimInitParams_p->SysPartition_p,
                                                                    1,sizeof(CLKRVSIM_Context_t));
    if (ControlObject_p == NULL)
    {
         return ST_ERROR_NO_MEMORY;
    }

    ControlObject_p->TaskContext.SysPartition_p       = SimInitParams_p->SysPartition_p;
    ControlObject_p->TaskContext.InternalPartition_p  = SimInitParams_p->InternalPartition_p;
    ControlObject_p->InterruptNumber                  = SimInitParams_p->InterruptNumber;
    strcpy(ControlObject_p->Evt.EVTDevNamPcr,SimInitParams_p->EVTDevNamPcr);

    /* create semaphore */
    ControlObject_p->TaskContext.TaskSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL, 1);

    ControlObject_p->TaskContext.State = INITIALIZED;

    Error = STOS_TaskCreate((void(*)(void *))SimulatorTask,
                                        NULL,
                                        ControlObject_p->TaskContext.SysPartition_p ,
                                        TASK_STACK_SIZE,
                                        &ControlObject_p->TaskContext.SimulatorTaskStack_p,
                                        ControlObject_p->TaskContext.SysPartition_p ,
                                        &ControlObject_p->TaskContext.simulator_task_p,
                                        &ControlObject_p->TaskContext.simulator_desc,
                                        (int)MAX_USER_PRIORITY,
                                        "Simulator_Task",
                                        (task_flags_t)0);
    if(Error != ST_NO_ERROR)
    {
        STTBX_Print(("STC_PCR Task Init failed\n"));
        Error = ST_ERROR_NO_MEMORY;     /* Error for OS Call faliure */
    }

    if(Error == ST_NO_ERROR)
    {
        Error = CLKRVSIM_EVTInit();
    }

    if(Error == ST_NO_ERROR)
    {
        task_onexit_set(exit_simulator_task);
    }

    return Error;
}

/****************************************************************************
Name         : CLKRVSIM_Term()

Description  : Terminate Simulator

Parameters   : void

Return Value : ST_ErrorCode_t
               ST_NO_ERROR               Successful completion

See Also     :
 ****************************************************************************/

ST_ErrorCode_t STCLKRVSIM_Term(void)
{
    ST_ErrorCode_t Error =  ST_NO_ERROR;
    task_t        *task_p;

    if(ControlObject_p->TaskContext.State == INITIALIZED)
    {
        CLKRVSIM_EVTTerm();
        task_p = ControlObject_p->TaskContext.simulator_task_p;

        ControlObject_p->TaskContext.State = NOT_INITIALIZED;

        task_kill(ControlObject_p->TaskContext.simulator_task_p, 5, 0);
        STOS_TaskWait(&task_p, TIMEOUT_INFINITY);
        STOS_TaskDelete ( ControlObject_p->TaskContext.simulator_task_p,
                          ControlObject_p->TaskContext.SysPartition_p ,
                          ControlObject_p->TaskContext.SimulatorTaskStack_p,
                          ControlObject_p->TaskContext.SysPartition_p);
        STOS_SemaphoreDelete(NULL, ControlObject_p->TaskContext.TaskSemaphore_p);
        memory_deallocate(ControlObject_p->TaskContext.SysPartition_p,ControlObject_p);
    }
    else
    {
        STTBX_Print(("STCLKRVSIM_Term called in Invalid State\n"));
        Error = ST_ERROR_BAD_PARAMETER;
    }

    return  Error;

}


/****************************************************************************
Name         : CLKRVSIM_EVTInit

Description  : Initialize EVT and Register STPTI_EVENT_PCR_RECEIVED_EVT Event

Parameters   : void

Return Value : ST_ErrorCode_t

See Also     :
****************************************************************************/

static ST_ErrorCode_t CLKRVSIM_EVTInit(void)
{
    STEVT_InitParams_t   EVTInitPars;
    STEVT_OpenParams_t   EVTOpenParsPCR;
    ST_ErrorCode_t       EvtError = ST_NO_ERROR;

    memset(&EVTInitPars,'\0',sizeof(EVTInitPars));

    /*  Setup Event registration for notification of PCR_RECEIVED
     *  event and callback to ActionPCR
     */
    EVTInitPars.EventMaxNum       = 3;
    EVTInitPars.ConnectMaxNum     = 2;
    EVTInitPars.SubscrMaxNum      = 9;
    EVTInitPars.MemoryPartition   = ControlObject_p->TaskContext.SysPartition_p;
    EVTInitPars.MemorySizeFlag    = STEVT_UNKNOWN_SIZE;

    EvtError = STEVT_Init(ControlObject_p->Evt.EVTDevNamPcr,&EVTInitPars);
    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("ERROR:STEVT_Init in CLKRVSIM_EVTInit\n"));
    }

    /* Call STEVT_Open() so that we can subscribe to events */

    EvtError = STEVT_Open(ControlObject_p->Evt.EVTDevNamPcr,
                          &EVTOpenParsPCR,
                          &(ControlObject_p->Evt.EVTHandlePcr));
    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("ERROR:STEVT_Open in CLKRVSIM_EVTInit\n"));
    }

#ifdef ST_5188
    EvtError = STEVT_Register(ControlObject_p->Evt.EVTHandlePcr,
                             (U32)STDEMUX_EVENT_PCR_RECEIVED_EVT,
                             &CLKRVEventIDSIM );
#else
    EvtError = STEVT_Register(ControlObject_p->Evt.EVTHandlePcr,
                             (U32)STPTI_EVENT_PCR_RECEIVED_EVT,
                             &CLKRVEventIDSIM );
#endif
    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("ERROR:STEVT_Register in CLKRVSIM_EVTInit\n"));
    }

    return EvtError;
}

/****************************************************************************
Name         : CLKRVSIM_EVTTerm

Description  : Terminate EVT

Parameters   : void

Return Value : ST_ErrorCode_t

See Also     :
****************************************************************************/

static ST_ErrorCode_t CLKRVSIM_EVTTerm(void)
{
    STEVT_TermParams_t  EVTTermPars_s;
    ST_ErrorCode_t      EvtError = ST_NO_ERROR;

#ifdef ST_5188
    EvtError = STEVT_Unregister(ControlObject_p->Evt.EVTHandlePcr,
                                (U32)STDEMUX_EVENT_PCR_RECEIVED_EVT);
#else
    EvtError = STEVT_Unregister(ControlObject_p->Evt.EVTHandlePcr,
                                (U32)STPTI_EVENT_PCR_RECEIVED_EVT);
#endif
    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("ERROR:STEVT_Unregister\n"));
    }
    else
    {
        EvtError = STEVT_Close(ControlObject_p->Evt.EVTHandlePcr);
        if(EvtError != ST_NO_ERROR)
        {
            STTBX_Print(("ERROR:STEVT_Close\n"));
        }
    }

    EVTTermPars_s.ForceTerminate = FALSE;

    EvtError = STEVT_Term(ControlObject_p->Evt.EVTDevNamPcr,&EVTTermPars_s);
    if (EvtError != ST_NO_ERROR )
    {
        STTBX_Print(("Call to STEVT_Term() failed"));
    }
    return EvtError;
}


/****************************************************************************
Name         : CLKRVSIM_Start()

Description  : Start generating STC, PCR values.

Parameters   : Pointer to STCLKRVSIM_StartParams_t structure

Return Value : ST_ErrorCode_t

See Also     :
 ****************************************************************************/

ST_ErrorCode_t STCLKRVSIM_Start(STCLKRVSIM_StartParams_t* StartParams)
{

    /* Validate Parameters */
    if((StartParams->File_p == NULL) || (StartParams->PCR_Interval == 0) ||
                                   (StartParams->PCRDriftThreshold == 0))
    {
        STTBX_Print(("Invalid Parameter to CLKRVSIM_Init:"));
        return ST_ERROR_BAD_PARAMETER;
    }

    ControlObject_p->TaskContext.Logfile_p = StartParams->File_p;
    ControlObject_p->PCR_Interval          = StartParams->PCR_Interval;
    ControlObject_p->PCR_DriftThreshold    = StartParams->PCRDriftThreshold;

    if(StartParams->FileRead)
    {
        if(StartParams->TestFile != NULL)
        {
            ControlObject_p->TestFile_p = fopen(StartParams->TestFile, "r+");
            if(ControlObject_p->TestFile_p == NULL)
            {
                STTBX_Print(( "Could Not Open File To read PCR,STC values\n" ));
                return ST_ERROR_BAD_PARAMETER;
            }
            ControlObject_p->FileTest = 1;
        }
        else
        {
            return ST_ERROR_BAD_PARAMETER;
        }
    }
    /* Change the Task State to RUNNING */
    ControlObject_p->TaskContext.State = RUNNING;

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STCLKRVSIM_Stop()

Description  : Stop generating STC, PCR values.

Parameters   : Pvoid

Return Value : ST_ErrorCode_t

See Also     :
 ****************************************************************************/
void STCLKRVSIM_Stop(void)
{
    ControlObject_p->TaskContext.State     = INITIALIZED;
    /*ControlObject_p->TaskContext.Logfile_p = NULL;*/

    STOS_SemaphoreWait(ControlObject_p->TaskContext.TaskSemaphore_p);

    /* Reset Elements to remove side effects from previous Run */
    memset( CLKRVSIM_RegArray,0,CLKRVSIM_REG_ARRAY_SIZE );
    /* RS memset( &ClkrvData,0,sizeof(STCLKRV_Log_Data_t ));*/
    ControlObject_p->Jitter = 0;
    ControlObject_p->StreamStatus = 0;
    ControlObject_p->PCR_Interval = 0;           /* In ms */
    ControlObject_p->PCR_DriftThreshold = 0;
    ControlObject_p->FileTest = 0;
    ControlObject_p->SimReset = 1;
    STOS_SemaphoreSignal(ControlObject_p->TaskContext.TaskSemaphore_p);

    if(ControlObject_p->TestFile_p)
    {
        fclose(ControlObject_p->TestFile_p);
        ControlObject_p->TestFile_p = NULL;
    }

}


/****************************************************************************
Name         : STCLKRVSIM_SetTestParameters()

Description  : Sets the test Parameters which would be used to generate
               (PCR,STC) values
Parameters   : Freq         - Encoder Frequency
               Jitter       - Max Jitter Value
               StreamStatus - Type of Real Time Disturbance to induce

Return Value : ST_ErrorCode_t
See Also     :
*****************************************************************************/

ST_ErrorCode_t STCLKRVSIM_SetTestParameters(U32 Freq ,U32 Jitter ,U8 StreamStatus)
{

    if((Freq > CLKRVSIM_ENCODER_MAX_FREQ) || (Freq < CLKRVSIM_ENCODER_MIN_FREQ))
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    STOS_SemaphoreWait(ControlObject_p->TaskContext.TaskSemaphore_p);
    ControlObject_p->EncoderFrequency = Freq;
    ControlObject_p->StreamStatus     = StreamStatus;
    ControlObject_p->Jitter           = Jitter;

    STOS_SemaphoreSignal(ControlObject_p->TaskContext.TaskSemaphore_p);

    return ST_NO_ERROR;

}

/****************************************************************************
Name         : SimulatorTask()

Description  : Provides PCR and STC values to clock recovery driver depending
               taking the various Real Time Disturbances like Jitter, Glitch,
               Packet loss, Channel Change into consideration.

Parameters   : NONE

Return Value : NONE
 ****************************************************************************/

void SimulatorTask(void *voidptr)
{
    U32 SDFreq                   = DEFAULT_SD_FREQUENCY;
    U32 PCMFreq                  = PCM_CLOCK_FREQ1;
    U32 SPDIFFreq                = PCM_CLOCK_FREQ2;
    U32 EncoderFrequency         = 0;
    U32 DriftThreshold           = 0;
    U32 PCR_Interval             = 0;
    U32 PacketDrop               = 0;
    U32 StreamStatus             = 0;
    U32 Jitter                   = 0;
    U32 MaxJitter                = 0;
    U32 PCR_Ticks_To_Increment   = 0;
    U32 STC_Ticks_To_Increment   = 0;
    U32 Glitch_Ticks_To_Increment= 0;
    U32 STC_Ticks_Elapsed        = 0;
    U32 PCR_Ticks_Elapsed        = 0;
    S32 RetVal                   = 0;
    U32 FilePrevSTC = 0, FilePrevPCR = 0;

    U32 RefCounter               = 0;
    U32 PCRCounter               = 0;
    U32 STCCounter               = 0;
    U32 RefCntMaxVal             = 0;
    U32 TempRefCntMaxVal         = 0;
    U32 CmdReg                   = 0;
    U32 PCMCounter               = 0;
    U32 SPDIFCounter             = 0;

    U32 PCMCorrection            = 0;
    U32 SPDIFCorrection          = 0;
    U32 STCCorrection            = 0;
    U32 PCRCorrection            = 0;

    U32 InterruptAudioticks      = 0;
    U32 InterruptSPDIFticks      = 0;
    U32 InterruptPCRticks        = 0;
    U32 InterruptSTCticks        = 0;

    double SPDIF_Ratio = 0;     /* double type required for precision */
    double PCM_Ratio   = 0;

    while (ControlObject_p->TaskContext.State != NOT_INITIALIZED)
    {
        /* If Task State RUNNING start generating PCR, STC values */
        if(ControlObject_p->TaskContext.State == RUNNING)
        {
            /* Store values in internal variables */
            PCR_Interval     = ControlObject_p->PCR_Interval;
            DriftThreshold   = ControlObject_p->PCR_DriftThreshold;
            Glitch_Ticks_To_Increment = 0;

            STOS_SemaphoreWait(ControlObject_p->TaskContext.TaskSemaphore_p);
            MaxJitter        = ControlObject_p->Jitter;
            StreamStatus     = ControlObject_p->StreamStatus;
            EncoderFrequency = ControlObject_p->EncoderFrequency;
            ControlObject_p->StreamStatus = 0;
            STOS_SemaphoreSignal(ControlObject_p->TaskContext.TaskSemaphore_p);

            ClkrvSim_RegRead(CLKRVSIM_RegArray, REF_MAX_CNT,&TempRefCntMaxVal);

            /* Check if MaxRefCount Val has changed, if yes reload the value */
            if(TempRefCntMaxVal != RefCntMaxVal)
            {
                RefCounter   = RefCntMaxVal = TempRefCntMaxVal;
                /* Reset the PCM and SPDIF counters and update the Registers for the same */
                SPDIFCounter = 0;
                PCMCounter   = 0;
                /* ClkrvSim_RegWrite(CLKRVSIM_RegArray,OFFSET_DCO_REF_CNT,RefCounter);*/
                ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_PCM, PCMCounter);
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)
                ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_SPDIF, SPDIFCounter);
#elif defined (ST_7710) || defined (ST_7100)
                ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_HD, SPDIFCounter);
#endif

                /* Set the Drift Correction bit ON or OFF */
                if(RefCntMaxVal != 0)
                {
                    ControlObject_p->DriftCorrection = 1;
                }
                else
                {
                    STTBX_Print(( "Ref CntMax Val = 0, Turning Off Drift Correction\n" ));
                    ControlObject_p->DriftCorrection = 0;
                }
            }

            /* ALthough this will never happen, just a safety check */
            if((SPDIFFreq == 0) || (PCMFreq == 0))
            {
                STTBX_Print(("\n\nERROR IN SLAVE FREQ- DIV BY 0\n\n"));
            }
            else
            {
                /* Store the ratios */
                SPDIF_Ratio = (double)SDFreq / (double)SPDIFFreq;
                PCM_Ratio   = (double)SDFreq / (double)PCMFreq;
            }

            /* Check if STC, PCr values have to be generated or read from a file */
            if(!ControlObject_p->FileTest)
            {
                /* Calculate STC and PCR Ticks elapsed */
                STC_Ticks_Elapsed = (U32)(((double)SDFreq * (double)PCR_Interval + STCCorrection)/ K);
                PCR_Ticks_Elapsed = (U32)(((double)EncoderFrequency * (double)PCR_Interval +
                                                                        PCRCorrection) / K );
                STCCorrection     += (((U32)((double)SDFreq * (double)PCR_Interval)) % K );
                PCRCorrection     += (((U32)((double)EncoderFrequency * (double)PCR_Interval)) % K);

                if(STCCorrection >= K)
                {
                   STCCorrection = STCCorrection - K;
                }

                if(PCRCorrection >= K)
                {
                   PCRCorrection = PCRCorrection - K;
                }

                /* Introduce Real Time disturbances in the STC, PCR ticks elapsed */
                if(StreamStatus & MASK_BIT_PACKET_DROP)
                {
                    PacketDrop = 2;
                }
                else
                {
                    PacketDrop = 1;
                }

                Jitter =  NetworkJitter(MaxJitter);     /* Get the Random value of Jitter */

                if(StreamStatus & MASK_BIT_GENERATE_GLITCH)
                {
                    STC_Ticks_To_Increment    = STC_Ticks_Elapsed * PacketDrop + Jitter ;
                    PCR_Ticks_To_Increment    = PCR_Ticks_Elapsed * PacketDrop ;

                    Glitch_Ticks_To_Increment = (ControlObject_p->PCR_DriftThreshold * 2);
                }
                else if(StreamStatus & MASK_BIT_BASE_CHANGE)
                {
                    STC_Ticks_To_Increment  = (STC_Ticks_Elapsed * PacketDrop) + Jitter;
                    PCR_Ticks_To_Increment  = (PCR_Ticks_Elapsed * PacketDrop) + DriftThreshold + 1;
                }
                else    /* i.e No Real time disturbance */
                {
                    STC_Ticks_To_Increment  = (STC_Ticks_Elapsed * PacketDrop) + Jitter;
                    PCR_Ticks_To_Increment  = (PCR_Ticks_Elapsed * PacketDrop);
                }
            }
            else
            {
                if(ControlObject_p->TestFile_p !=NULL)
                {
                    RetVal = fscanf(ControlObject_p->TestFile_p,"%u\t%u",&PCR_Ticks_Elapsed,
                                                                         &STC_Ticks_Elapsed);
                    if(RetVal == EOF)
                    {
                        ControlObject_p->TaskContext.State = INITIALIZED;
                        fclose(ControlObject_p->TestFile_p);
                        ControlObject_p->TestFile_p = NULL;
                        continue ;
                    }
                    else
                    {
                        STC_Ticks_To_Increment = STC_Ticks_Elapsed - FilePrevSTC;
                        PCR_Ticks_To_Increment = PCR_Ticks_Elapsed - FilePrevPCR;
                        FilePrevSTC = STC_Ticks_Elapsed;
                        FilePrevPCR = PCR_Ticks_Elapsed;
                    }
                }
            }

            /* Go inside if only if Interrupt is not to be generated in this iteration */
            if(((ControlObject_p->DriftCorrection) && (RefCounter > ((U32)STC_Ticks_To_Increment +
                Glitch_Ticks_To_Increment))) || (!ControlObject_p->DriftCorrection))
            {
                /* Increment the Counters with the ticks calculated as above */
                STCCounter   += ((U32)STC_Ticks_To_Increment - InterruptSTCticks);
                PCRCounter   += ((U32)PCR_Ticks_To_Increment - InterruptPCRticks);
#ifdef ST_5188
                PcrDataSim.member.PCREventData.DiscontinuityFlag   = 0;
                /* Loss of accuracy here */
                PcrDataSim.member.PCREventData.PCRBase.LSW          = (PCRCounter / DIV_90KHZ_27MHZ);
                PcrDataSim.member.PCREventData.PCRExtension         = (PCRCounter % DIV_90KHZ_27MHZ);
                PcrDataSim.member.PCREventData.PCRArrivalTime.Bit32 = 0;
                PcrDataSim.member.PCREventData.PCRBase.Bit32        = 0;
                PcrDataSim.ObjectHandle                          = CLKRV_SLOT_1;   /* As set in Test
                                                                                    * App SetPCRSource()*/

                PcrDataSim.member.PCREventData.PCRArrivalTime.LSW = (U32)((STCCounter
                                                                + Glitch_Ticks_To_Increment)
                                                                / DIV_90KHZ_27MHZ);

                PcrDataSim.member.PCREventData.PCRArrivalTimeExtension = (U32)((STCCounter
                                                                     + Glitch_Ticks_To_Increment)
                                                                     % DIV_90KHZ_27MHZ);
#else
                PcrDataSim.u.PCREventData.DiscontinuityFlag   = 0;
                /* Loss of accuracy here */
                PcrDataSim.u.PCREventData.PCRBase.LSW          = (PCRCounter / DIV_90KHZ_27MHZ);
                PcrDataSim.u.PCREventData.PCRExtension         = (PCRCounter % DIV_90KHZ_27MHZ);
                PcrDataSim.u.PCREventData.PCRArrivalTime.Bit32 = 0;
                PcrDataSim.u.PCREventData.PCRBase.Bit32        = 0;
                PcrDataSim.SlotHandle                          = CLKRV_SLOT_1;   /* As set in Test
                                                                                    * App SetPCRSource()*/

                PcrDataSim.u.PCREventData.PCRArrivalTime.LSW = (U32)((STCCounter
                                                                + Glitch_Ticks_To_Increment)
                                                                / DIV_90KHZ_27MHZ);

                PcrDataSim.u.PCREventData.PCRArrivalTimeExtension = (U32)((STCCounter
                                                                     + Glitch_Ticks_To_Increment)
                                                                     % DIV_90KHZ_27MHZ);
#endif
                /* Notify Driver */
                STEVT_Notify(ControlObject_p->Evt.EVTHandlePcr, CLKRVEventIDSIM, &PcrDataSim);

                /* Filter will take 100 micro-seconds */
                STOS_TaskDelay(ST_GetClocksPerSecond()/1000);


                /* If Drift Correction is ON Increment PCM and SPDIf counters too */
                if(ControlObject_p->DriftCorrection)
                {
                    RefCounter = RefCounter - ((U32)STC_Ticks_To_Increment * PacketDrop +
                                                    Glitch_Ticks_To_Increment)+ InterruptSTCticks;

                    /* Debug message - should never happen though */
                    if((S32)RefCounter < 0)
                    {
                        STTBX_Print(( "\n***Reference Counter became Negative ***\n" ));
                    }

                    if(!ControlObject_p->FileTest)
                    {

                        PCMCounter   += (U32) ((U32)(((double)PCR_Interval * (double)PCMFreq * PacketDrop +
                                       PCMCorrection ) / K ) + (Glitch_Ticks_To_Increment / PCM_Ratio)
                                                                            - InterruptAudioticks);
                        SPDIFCounter += (U32) ((U32)(((double)PCR_Interval  * (double)SPDIFFreq * PacketDrop +
                                       SPDIFCorrection) / K ) + (Glitch_Ticks_To_Increment / SPDIF_Ratio)
                                                                            - InterruptSPDIFticks);
                        PCMCorrection     += (((U32)((double) PCMFreq   * (double)PCR_Interval )) % K );
                        SPDIFCorrection   += (((U32)((double) SPDIFFreq * (double)PCR_Interval )) % K );
                    }
                    else
                    {
                        PCMCounter += (U32)((STC_Ticks_To_Increment / PCM_Ratio)- InterruptAudioticks);
                        SPDIFCounter += (U32)((STC_Ticks_To_Increment / SPDIF_Ratio)- InterruptSPDIFticks);

                        PCMCorrection += ((U32)(STC_Ticks_To_Increment / PCM_Ratio) % K );
                        SPDIFCorrection += ((U32)(STC_Ticks_To_Increment / SPDIF_Ratio) % K );
                    }

                    if(PCMCorrection >= K)
                    {
                       PCMCorrection = PCMCorrection - K;
                    }

                    if(SPDIFCorrection >= K)
                    {
                       SPDIFCorrection = SPDIFCorrection - K;
                    }
                    /* Update the respective counters */
                    ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_PCM, PCMCounter);
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)
                    ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_SPDIF, SPDIFCounter);
#elif defined (ST_7710) || defined (ST_7100)
                    ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_HD, SPDIFCounter);
#endif
                }

                /* Print Data to Excel File */
                fprintf((ControlObject_p->TaskContext.Logfile_p),"%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%u,"
                "%u,%u\n",ClkrvDbg[0][0],ClkrvDbg[5][0],ClkrvDbg[1][0],ClkrvDbg[3][0],ClkrvDbg[2][0],
                ClkrvDbg[8][0],ClkrvDbg[4][0],ClkrvDbg[7][0],ClkrvDbg[6][0],ClkrvDbg[9][0],ClkrvDbg[10][0],ClkrvDbg[11][0],
                Clkrv_Slave_Dbg[0][0],Clkrv_Slave_Dbg[1][0]);

                InterruptAudioticks = InterruptSPDIFticks = 0;
                InterruptSTCticks   = InterruptPCRticks = 0;

                /* Update the Frequencies */
                if(ClkrvDbg[1][0] != 0)
                {
#ifdef WA7710_35956
                    SDFreq      = (ClkrvDbg[1][0] * 10) /55;
#else
                    SDFreq      = ClkrvDbg[1][0];
#endif

                    SPDIFFreq   = Clkrv_Slave_Dbg[0][0];
                    PCMFreq     = Clkrv_Slave_Dbg[1][0];
                }

                SampleCount = 0;
            }
            else    /* Generate Interrupt */
            {
                InterruptSTCticks  =  RefCounter;

                /* Debug Message - would never happen though */
                if((RefCounter > (U32)STC_Ticks_Elapsed )&&(RefCounter < (U32)STC_Ticks_To_Increment))
                {
                    STTBX_Print(( "\n***Interrupt invoked when Stream Status = %d***\n",StreamStatus));
                }

                /* Reload Actual Reference Counter value */
                RefCounter = RefCntMaxVal ;
                ClkrvSim_RegWrite(CLKRVSIM_RegArray, REF_MAX_CNT, RefCounter);

                if(PCMFreq) /* Just a double check */
                {
                    /* Now get the audio counter value at the instance interrupt is generated */
                    /* check accuracy here */
                    InterruptAudioticks  =  (U32)(((double)InterruptSTCticks * (double)PCMFreq) /
                                                                                (double)SDFreq);
                    PCMCounter += InterruptAudioticks;

                    ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_PCM, PCMCounter);
                }
                else
                {
                    STTBX_Print(( "BY-PASSSING PCM FREQUENCY Since PCM not Set:\n" ));
                }
                /* Now get the SPDIF counter value at the instance interrupt is generated */
                if(SPDIFFreq)
                {
                    InterruptSPDIFticks    =  (U32)(((double)InterruptSTCticks * (double)SPDIFFreq) /
                                                                                    (double)SDFreq);
                    SPDIFCounter += InterruptSPDIFticks;

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)
                    ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_SPDIF, SPDIFCounter);
#elif defined (ST_7710) || defined (ST_7100)
                    ClkrvSim_RegWrite(CLKRVSIM_RegArray, CAPTURE_COUNTER_HD, SPDIFCounter);
#endif
                }
                else
                {
                    STTBX_Print(( "BY-PASSSING SPDIF:\n" ));
                }
                /* Increment STC and PCR counters Too */
                InterruptPCRticks = (U32)(((double)InterruptSTCticks * (double)EncoderFrequency) /
                                                                                 (double)SDFreq);
                PCRCounter    += InterruptPCRticks ;
                STCCounter    += InterruptSTCticks ;

                /* Generate Interupt Now */
                ClkrvSim_RegRead(CLKRVSIM_RegArray, CLK_REC_CMD, &CmdReg);
                ClkrvSim_RegWrite(CLKRVSIM_RegArray, CLK_REC_CMD,(CmdReg |0x02));
                interrupt_raise_number(ControlObject_p->InterruptNumber);

            }
        }
        /*  Reset all local variables before changing state from RUNNING to INITIALIZED
         *  and prevent any side effects next time
         */
        else
        {

            STOS_TaskDelay(ST_GetClocksPerSecond()/1000); /* Bad simulator design TBCorrected at some point later */
            if(ControlObject_p->SimReset == 1)
            {
                SDFreq                   = DEFAULT_SD_FREQUENCY;
                PCMFreq                  = PCM_CLOCK_FREQ1;
                SPDIFFreq                = PCM_CLOCK_FREQ2;
                PCRCounter               = 0;
                STCCounter               = 0;
                RefCounter               = 0;
                PCMCounter               = 0;
                SPDIFCounter             = 0;
                EncoderFrequency         = 0;
                DriftThreshold           = 0;
                PCR_Interval             = 0;
                PacketDrop               = 0;
                StreamStatus             = 0;
                Jitter                   = 0;
                MaxJitter                = 0;
                PCR_Ticks_To_Increment   = 0;
                STC_Ticks_To_Increment   = 0;
                Glitch_Ticks_To_Increment= 0;
                STC_Ticks_Elapsed        = 0;
                PCR_Ticks_Elapsed        = 0;
                RetVal                   = 0;
                RefCounter               = 0;
                RefCntMaxVal             = 0;
                CmdReg                   = 0;
                STCCorrection            = 0;
                PCRCorrection            = 0;
                PCMCorrection            = 0;
                SPDIFCorrection          = 0;
                InterruptAudioticks      = 0;
                InterruptSPDIFticks      = 0;
                InterruptPCRticks        = 0,
                InterruptSTCticks        = 0;
                FilePrevSTC              = 0;
                FilePrevPCR              = 0;

                ControlObject_p->SimReset= 0;
            }
            /* Do Anything else - ??? */
        }
    }
}


/****************************************************************************
Name         : NetworkJitter()

Description  : Returns a signed random number which is limited by
                value set by CLKRVSIM_SetJitterMax call.

Parameters   : NONE

Return Value : Returns a signed random number
 ****************************************************************************/

static S32 NetworkJitter(S32 MaxJitter)
{
    U32 Sign =0;
    S32 Jitter = 0;

    if(0 == MaxJitter)
    {
        return 0;
    }
    else
    {
        Jitter = rand() % MaxJitter;
        Sign   = rand() % MaxJitter;
        if ( Sign > (MaxJitter/2))
        {
            return Jitter;
        }
        else
        {
            return -Jitter;
        }
    }
}


/****************************************************************************
Name         : exit_simulator_task

Description  : On Task Exit Handler

Parameters   : NONE

Return Value : NONE
 ****************************************************************************/

void exit_simulator_task(task_t* task, S32 param)
{
}

/****************************************************************************
Name         : ClkrvSim_RegWrite()

Description  : Write to Register Array

Parameters   : Pointer to BaseAddress of the Array
               Register Offset
               Value to write to Register

Return Value : void

 ****************************************************************************/

__inline void ClkrvSim_RegWrite(U32 *BaseAddress, U32 Register, U32 Value)
{
    *((U32 *)((U32)BaseAddress + Register)) = Value;

}

/****************************************************************************
Name         : ClkrvSim_RegRead()

Description  : Read from Register Array

Parameters   : Pointer to BaseAddress of the Array
               Register Offset
               Pointer to variable in which to store the Read Value

Return Value : void

****************************************************************************/

__inline void ClkrvSim_RegRead(U32 *BaseAddress, U32 Register, U32* ReadVal )
{
    *ReadVal = *((U32 *)((U32)BaseAddress + Register));
}


/* End of clkrvsim.c*/
