#if !defined (ST_OSLINUX)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#endif

#include "stlite.h"
#include "stddefs.h"
#include "stavmem.h"
#include "vid_com.h"

#include "mme.h"

ST_ErrorCode_t MPEG2STUB_TermTransformer(MME_TransformerHandle_t Handle, ST_Partition_t * CPUPartition_p);
ST_ErrorCode_t MPEG2STUB_Term(ST_DeviceName_t Name);
ST_ErrorCode_t MPEG2STUB_Init(ST_DeviceName_t Name);
MME_ERROR MPEG2STUB_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t *CmdInfo_p);
ST_ErrorCode_t MPEG2STUB_InitTransformer(ST_DeviceName_t DeviceName,
									MME_TransformerInitParams_t *Params_p,
                                    MME_TransformerHandle_t *Handle_p,
                                    ST_Partition_t * CPUPartition_p);
ST_ErrorCode_t MPEG2InitStub (MME_GenericCallback_t Callback, ST_Partition_t * CPUPartition_p);
void Mpeg2TaskStub(void);

void * Mpeg2CallBackUserData_fmw_stub;
MME_GenericCallback_t Mpeg2CallbackFunction;
static unsigned int Mpeg2Mme_CommandID = 123;
MME_CommandStatus_t Mpeg2CmdStatus;
MME_Command_t       Mpeg2CmdInfo;
semaphore_t*      	Mpeg2WaitForFmwCompleted;
VIDCOM_Task_t * const Task_p ;

#if defined(TRACE_UART) && defined(VALID_TOOLS)

#define MPEG2STUB_TRACE_BLOCKNAME "MME"

/* !!!! WARNING !!!! : Any modification of this table should be reported in emun in mmedebugprint.h */
static TRACE_Item_t mpeg2stub_TraceItemList[] =
{
    {"GLOBAL",          TRACE_MMESTUB_COLOR,                            TRUE, FALSE},
    {"FMW_SET",         TRACE_MMESTUB_COLOR,                            TRUE, FALSE},
    {"FMW_START",       TRACE_MMESTUB_COLOR,                            TRUE, FALSE},
    {"FMW_CALLBACK",    TRACE_MMESTUB_COLOR,                            TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

#define MPEG2RAND_RANGE_FMW 20

/*******************************************************************************
Name        :  InitStub
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MPEG2InitStub (MME_GenericCallback_t Callback, ST_Partition_t * CPUPartition_p)
{
	ST_ErrorCode_t Err = ST_NO_ERROR;

	Mpeg2CallbackFunction = Callback ;
    if (STOS_TaskCreate((void (*) (void*)) Mpeg2TaskStub,
                              NULL,
                              CPUPartition_p,
                              20000,
                              &(Task_p->TaskStack),
                              CPUPartition_p,
                              &(Task_p->Task_p),
                              &(Task_p->TaskDesc),
                              15,
                              "FmwCtrlTask",
                              (task_flags_t)0) != ST_NO_ERROR)
    {
        return ST_ERROR_NO_MEMORY;
    }
    Task_p->IsRunning = TRUE;

	return Err;
}

/*******************************************************************************
Name        : MPEG2STUB_InitTransformer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MPEG2STUB_InitTransformer(ST_DeviceName_t DeviceName,
									MME_TransformerInitParams_t *Params_p,
                                    MME_TransformerHandle_t *Handle_p,
                                    ST_Partition_t * CPUPartition_p)
{
    UNUSED_PARAMETER(DeviceName);
    Mpeg2CallBackUserData_fmw_stub = Params_p->CallbackUserData;
    Mpeg2WaitForFmwCompleted = STOS_SemaphoreCreateFifo(CPUPartition_p,0);
    MPEG2InitStub(Params_p->Callback,CPUPartition_p);
    *Handle_p = 1;/* TODO (LD) : not sure whether it requires a specific value ! */

	return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : Mpeg2TaskStub
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void Mpeg2TaskStub()
{
    osclock_t fmw_duration;
    float GetClocksPerMillisecond;
    float RandTime;
    BOOL InputTaskFinished = FALSE;

    while (!InputTaskFinished)
    {
        STOS_SemaphoreWait(Mpeg2WaitForFmwCompleted);

        GetClocksPerMillisecond = ST_GetClocksPerSecond();
        GetClocksPerMillisecond /= 1000;
        RandTime = rand();
        RandTime /= RAND_MAX;
        fmw_duration = (osclock_t)(GetClocksPerMillisecond * RandTime * MPEG2RAND_RANGE_FMW);
       /* TODO (LD) : fill CmdInfo structure */
        (*(Mpeg2CallbackFunction)) (MME_COMMAND_COMPLETED_EVT, &Mpeg2CmdInfo,
                                            Mpeg2CallBackUserData_fmw_stub);
        if (Task_p->IsRunning == FALSE)
        {
            InputTaskFinished = TRUE;
        }
    }
}


/*******************************************************************************
Name        : MPEG2STUB_SendCommand
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
MME_ERROR MPEG2STUB_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t *CmdInfo_p)
{

    UNUSED_PARAMETER(Handle);
  Mpeg2CmdStatus.CmdId = Mpeg2Mme_CommandID++;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
  TraceBufferColorConditional((MPEG2STUB_TRACE_BLOCKID, MMESTUB_TRACE_FMW_START, "mFMWStart%d ", Mpeg2CmdStatus.CmdId));
#endif

    if (CmdInfo_p->CmdEnd != MME_COMMAND_END_RETURN_NO_INFO)
    {
        STOS_SemaphoreSignal(Mpeg2WaitForFmwCompleted);
    }
    else
    {
        /* Global Param: no action */
    }

  return (MME_SUCCESS);
}

/*******************************************************************************
Name        :  MPEG2STUB_Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MPEG2STUB_Init(ST_DeviceName_t Name)
{
    UNUSED_PARAMETER(Name);
	return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : MPEG2STUB_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MPEG2STUB_Term(ST_DeviceName_t Name)
{
    UNUSED_PARAMETER(Name);
	return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : MPEG2STUB_TermTransformer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MPEG2STUB_TermTransformer(MME_TransformerHandle_t Handle, ST_Partition_t * CPUPartition_p)
{
    UNUSED_PARAMETER(Handle);
    Task_p->IsRunning = FALSE;
    STOS_TaskDelete ( Task_p->Task_p,
                      CPUPartition_p,
                      &Task_p->TaskStack,
                      CPUPartition_p);
	return(ST_NO_ERROR);
}

#ifdef VALID_TOOLS
ST_ErrorCode_t MPEG2STUB_RegisterTrace(void)
{
#if defined(TRACE_UART)
    TRACE_ConditionalRegister(MPEG2STUB_TRACE_BLOCKID, MPEG2STUB_TRACE_BLOCKNAME, mpeg2stub_TraceItemList, sizeof(mpeg2stub_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* defined(TRACE_UART) */
}
#endif /* VALID_TOOLS */

