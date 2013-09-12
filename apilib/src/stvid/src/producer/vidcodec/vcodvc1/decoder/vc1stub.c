#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#endif

#include "stlite.h"
#include "stddefs.h"
#include "stavmem.h"

#include "mme.h"
void TaskStub(void);
void * CallBackUserData_fmw_stub;
MME_GenericCallback_t CallbackFunction;
static unsigned int Mme_CommandID = 123;
MME_CommandStatus_t CmdStatus;
MME_Command_t       CmdInfo;
semaphore_t*      		WaitForFmwCompleted;

#if defined(TRACE_UART) && defined(VALID_TOOLS)

#define MMESTUB_TRACE_BLOCKNAME "MME"

/* !!!! WARNING !!!! : Any modification of this table should be reported in emun in mmedebugprint.h */
static TRACE_Item_t mmestub_TraceItemList[] =
{
    {"GLOBAL",          TRACE_MMESTUB_COLOR,                            TRUE, FALSE},
    {"FMW_SET",         TRACE_MMESTUB_COLOR,                            TRUE, FALSE},
    {"FMW_START",       TRACE_MMESTUB_COLOR,                            TRUE, FALSE},
    {"FMW_CALLBACK",    TRACE_MMESTUB_COLOR,                            TRUE, FALSE}
};
#endif /* defined(TRACE_UART) && defined(VALID_TOOLS) */

#define RAND_RANGE_FMW 20

/*******************************************************************************
Name        :  InitStub
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t InitStub (MME_GenericCallback_t Callback)
{
	ST_ErrorCode_t  Err = ST_NO_ERROR;
    task_t        * Task_p;
    tdesc_t         TaskDesc;
    void          * TaskStack;

	CallbackFunction = Callback ;
    if (STOS_TaskCreate((void (*) (void*)) TaskStub,
                              (void *) NULL,
                              NULL,
                              20000,
                              &(TaskStack),
                              NULL,
                              &(Task.Task_p),
                              &(TaskDesc),
                              15,
                              "FmwCtrlTask",
                              (task_flags_t)0) != ST_NO_ERROR)
  {
		return ST_ERROR_NO_MEMORY;
  }

	return Err;
}

/*******************************************************************************
Name        : MME_InitTransformer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MMESTUB_InitTransformer(ST_DeviceName_t DeviceName,
									MME_TransformerInitParams_t *Params_p,
									MME_TransformerHandle_t	*Handle_p)
{
	task_t * receiver_task;

    CallBackUserData_fmw_stub = Params_p->CallbackUserData;
    WaitForFmwCompleted = semaphore_create_fifo(0);
    InitStub(Params_p->Callback);
    *Handle_p = 1;/* TODO (LD) : not sure whether it requires a specific value ! */

	return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : TaskStub
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
void TaskStub()
{
    osclock_t fmw_duration;
		float GetClocksPerMillisecond;
		float RandTime;
/*
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    MME_CommandStatus_t                    *CPU_CmdStatus_p;

    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping);
*/
    while (1)
    {
	     semaphore_wait(WaitForFmwCompleted);

			 GetClocksPerMillisecond = ST_GetClocksPerSecond();
			 GetClocksPerMillisecond /= 1000;
			 RandTime = rand();
			 RandTime /= RAND_MAX;
			 fmw_duration = (osclock_t)(GetClocksPerMillisecond * RandTime * RAND_RANGE_FMW);
       /* TODO (LD) : fill CmdInfo structure */
       (*(CallbackFunction)) (MME_COMMAND_COMPLETED_EVT, &CmdInfo,
                                        CallBackUserData_fmw_stub);
		}
}


/*******************************************************************************
Name        : MME_SendCommand
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
MME_ERROR MMESTUB_SendCommand(MME_TransformerHandle_t Handle, MME_Command_t *CmdInfo_p)
{
/*
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    MME_CommandStatus_t                    *CPU_CmdStatus_p;
*/
  CmdStatus.CmdId = Mme_CommandID++;
#if defined(TRACE_UART) && defined(VALID_TOOLS)
  TraceBufferColorConditional((MMESTUB_TRACE_BLOCKID, MMESTUB_TRACE_FMW_START, "mFMWStart%d ", CmdStatus.CmdId));
#endif
 	semaphore_signal(WaitForFmwCompleted);

  /*HDM_DumpCommand_stub(CmdCode, CmdInfo_p);*/
  /*FmwessFrame_H264SingleFrameFirmware_stub(&SingleFrameFirmware, CmdInfo_p);*/
/*	task_reschedule();	   /* previously : task_yield(); */

  return (MME_SUCCESS);
}

/*******************************************************************************
Name        :  MME_Init
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MMESTUB_Init(ST_DeviceName_t Name)
{
	return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : MME_Term
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MMESTUB_Term(ST_DeviceName_t Name)
{
	return(ST_NO_ERROR);
}

/*******************************************************************************
Name        : MME_TermTransformer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t MMESTUB_TermTransformer(MME_TransformerHandle_t Handle)
{
}

#ifdef VALID_TOOLS
ST_ErrorCode_t MMESTUB_RegisterTrace(void)
{
#if defined(TRACE_UART)
    TRACE_ConditionalRegister(MMESTUB_TRACE_BLOCKID, MMESTUB_TRACE_BLOCKNAME, mmestub_TraceItemList, sizeof(mmestub_TraceItemList)/sizeof(TRACE_Item_t));
#endif /* defined(TRACE_UART) */
}
#endif /* VALID_TOOLS */

