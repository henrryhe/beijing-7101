/******************************************************************************/
/*                                                                            */
/* File name   : pcminput.c                                                   */
/*                                                                            */
/* Description : pcm input receiver file                                      */
/*                                                                            */
/* COPYRIGHT (C) ST-Microelectronics 2005                                     */
/* History     :                                                              */
/* Date               Modification                 Name                       */
/* ----               ------------                 ----                       */
/* 30/11/05           Created                      udit kumar                 */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

//#define STTBX_PRINT
#include "stos.h"
#ifndef ST_OSLINUX
#include "sttbx.h"
#endif
#include "pcminput.h"
#ifndef ST_51XX
extern U32 MAX_SAMPLES_PER_FRAME;
#endif
/*****************************************************************************
STAud_InitPCMInput()
Description:
    This routine initializes the PCM input controlblock.

Parameters:
    InitParams, the initialization params.
    Handle,  pointer the handle of this unit.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_NO_MEMORY,             unable to allocate memory.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_InitPCMInput(STAudPCMInputInit_t *InitParams_p, STPCMInput_Handle_t * Handle_p)
{
    /* Local variables */
    ST_ErrorCode_t            error = ST_NO_ERROR;
    STAudPCMInputControl_t  * Control_p = NULL;
    ST_Partition_t          * DriverPartition_p = NULL;
    U32                       i = 0;
    STAUD_BufferParams_t      MemoryBuffers;

    DriverPartition_p = (ST_Partition_t *)InitParams_p->CPUPartition_p;

    Control_p = (STAudPCMInputControl_t *)STOS_MemoryAllocate(
                                                DriverPartition_p,
                                                sizeof(STAudPCMInputControl_t));

    if(Control_p == NULL)
    {
        /* Error No memory available */
        return ST_ERROR_NO_MEMORY;
    }

    memset(Control_p,0,sizeof(STAudPCMInputControl_t));
    /* Fill in the details of our new control block */
    *Handle_p = Control_p->Handle    = (U32 )Control_p;
    Control_p->InitParams = *InitParams_p;
    Control_p->PCMBlockHandle=InitParams_p->PCMBlockHandle;

    Control_p->PCMBufferSem_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Update the size of PCM input buffer */
    Control_p->PCM_Buff_Size = NUM_NODES_PCMINPUT * InitParams_p->NumChannels * 4 * MAX_SAMPLES_PER_FRAME;

    /* As Producer of outgoing data */
    Control_p->ProducerParam_s.Handle = (U32)Control_p;
    Control_p->ProducerParam_s.sem    = Control_p->PCMBufferSem_p;

    Control_p->EOFFlag = FALSE;

    Control_p->UpdateParams         = FALSE;
    for(i = 0;i < NUM_NODES_PCMINPUT ; i++)
    {
        Control_p->AppData_p[i] = STOS_MemoryAllocate(DriverPartition_p,sizeof(STAud_LpcmStreamParams_t));
        memset(Control_p->AppData_p[i],0,sizeof(STAud_LpcmStreamParams_t));
    }
    Control_p->OPBlkSent = 0;
    /*Change the PCM state  */
    Control_p->PCMCurrentState = PCM_INPUT_STATE_INIT;

    /* Register to memory block manager as a Producer */
    error |= MemPool_RegProducer(Control_p->InitParams.PCMBlockHandle, Control_p->ProducerParam_s);

    error |=MemPool_GetBufferParams(Control_p->InitParams.PCMBlockHandle, &MemoryBuffers);
    if(!error)
    {
        Control_p->MemoryAddress.BaseUnCachedVirtualAddress=(U32*)MemoryBuffers.BufferBaseAddr_p;
        Control_p->MemoryAddress.BasePhyAddress=(U32*) STOS_VirtToPhys(MemoryBuffers.BufferBaseAddr_p);
        Control_p->MemoryAddress.BaseCachedVirtualAddress = NULL ;

    }


    return error;
}

/*****************************************************************************
STAud_QueuePCMBuffer()
Description:
    This routine queues the valid PCM buffer.

Parameters:
    Handle, the handle to this block.
    PCMBuffer_p,  the PCM buffer params.
    NumBuffer, the number of valid buffer.
    NumQueue, the queue size
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_QueuePCMBuffer(STPCMInput_Handle_t Handle, STAUD_PCMBuffer_t* PCMBuffer_p, U32 NumBuffer, U32* NumQueue_p)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    MemoryBlock_t  * CurrentBlock_P;
    U32 NumberDone=0;

    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Control_p->PCMCurrentState != PCM_INPUT_STATE_START)
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    for (NumberDone=0;NumberDone<NumBuffer;NumberDone++)
    {
        CurrentBlock_P = (MemoryBlock_t *)(PCMBuffer_p->Block);
        CurrentBlock_P->Size = PCMBuffer_p->Length;

        if(Control_p->UpdateParams == TRUE)
        {
            U32 Index = Control_p->OPBlkSent % NUM_NODES_PCMINPUT;
            Control_p->UpdateParams = FALSE;
            Control_p->AppData_p[Index]->sampleRate   = Control_p->Frequency;
            Control_p->AppData_p[Index]->sampleBits   = Control_p->SampleSize;
            Control_p->AppData_p[Index]->channels     = Control_p->Nch;
            Control_p->AppData_p[Index]->ChangeSet    = TRUE;
            CurrentBlock_P->AppData_p                 = Control_p->AppData_p[Index];
            CurrentBlock_P->Data[FREQ_OFFSET]         = Control_p->Frequency;
            CurrentBlock_P->Data[NCH_OFFSET]          = Control_p->Nch;
            CurrentBlock_P->Data[SAMPLESIZE_OFFSET]   = Control_p->SampleSize;
            CurrentBlock_P->Flags |= FREQ_VALID | NCH_VALID | SAMPLESIZE_VALID;


        }
        else
        {
            CurrentBlock_P->AppData_p = 0;
        }

        if(Control_p->EOFFlag)
        {
            CurrentBlock_P->Flags |= EOF_VALID;
            Control_p->EOFFlag = FALSE;
        }

        Error = MemPool_PutOpBlk(Control_p->PCMBlockHandle , (U32 *)&(CurrentBlock_P));
        if (Error!=ST_NO_ERROR)
        {
            STTBX_Print(("couldn't send PCM\n"));
            return Error;
        }
        Control_p->OPBlkSent++;
        PCMBuffer_p->Block = 0;
        PCMBuffer_p++;
        if(Error!=ST_NO_ERROR)
        {
            STTBX_Print(("PutOpBlk Err\n"));
            break;
        }
    }

    * NumQueue_p=NumberDone;
    return Error;


}

/*****************************************************************************
STAud_GetPCMBuffer()
Description:
    This routine requests for an empty PCM buffer.

Parameters:
    Handle, the handle to this block.
    PCMBuffer_p,  the PCM buffer params.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
    ST_ERROR_NO_MEMORY,            No Memory.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_GetPCMBuffer(STPCMInput_Handle_t Handle, STAUD_PCMBuffer_t *PCMBuffer_p)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;
    ST_ErrorCode_t           Error = ST_NO_ERROR;
    MemoryBlock_t          * CurrentBlock_p;

    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Control_p->PCMCurrentState != PCM_INPUT_STATE_START)
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    STTBX_Print(("STAud_GetPCMBuffer Called \n"));
    // get the free blocks
    Error = MemPool_GetOpBlk(Control_p->PCMBlockHandle, (U32 *)&(CurrentBlock_p));
    if (Error!=ST_NO_ERROR)
    {
        /*wait for the semaphore*/
        STTBX_Print(("Couldn't get PCM\n"));
        //STOS_SemaphoreWait(Control->PCMBufferSem_p);
        return Error;
    }

    PCMBuffer_p->Block       = (U32)CurrentBlock_p;
    PCMBuffer_p->StartOffset = CurrentBlock_p->MemoryStart;
    PCMBuffer_p->Length      = CurrentBlock_p->MaxSize;

#ifdef ST_OSLINUX
    PCMBuffer_p->StartOffset = (U32)STOS_AddressTranslate(Control_p->MemoryAddress.BasePhyAddress,Control_p->MemoryAddress.BaseUnCachedVirtualAddress,PCMBuffer_p->StartOffset);
#endif


    STTBX_Print(("PCMBuffer_p->StartOffset = %x, PCMBuffer_p->Block = %x,PCMBuffer_p->Length =%d \n",
        PCMBuffer_p->StartOffset,PCMBuffer_p->Block,PCMBuffer_p->Length));
    return Error;
}

/*****************************************************************************
STAud_TermPCMInput()
Description:
    This routine terminates this unit.

Parameters:
    Handle, the handle to this block.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_TermPCMInput(STPCMInput_Handle_t Handle)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;
    ST_Partition_t         * DriverPartition_p = NULL;
    U32                      i = 0;
    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if ((Control_p->PCMCurrentState == PCM_INPUT_STATE_STOP) ||
        (Control_p->PCMCurrentState == PCM_INPUT_STATE_INIT))
    {
        Control_p->PCMCurrentState = PCM_INPUT_STATE_TERMINATE;
    }
    else
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    DriverPartition_p = Control_p->InitParams.CPUPartition_p;

    for(i = 0;i < NUM_NODES_PCMINPUT ; i++)
    {
        STOS_MemoryDeallocate(DriverPartition_p,Control_p->AppData_p[i]);
        memset(Control_p->AppData_p[i],0,sizeof(STAud_LpcmStreamParams_t));
    }

    STOS_SemaphoreDelete(NULL,Control_p->PCMBufferSem_p);
    Control_p->PCMBufferSem_p=NULL;
    memory_deallocate(DriverPartition_p, Control_p);
    Control_p = NULL;

    return ST_NO_ERROR;

}

/*****************************************************************************
STAud_TermPCMInput()
Description:
    This routine terminates this unit.

Parameters:
    Handle, the handle to this block.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_SetPCMParams(STPCMInput_Handle_t Handle, U32 Frequency, U32 SampleSize, U32 NCh)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;

    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Control_p->PCMCurrentState != PCM_INPUT_STATE_INIT)
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    Control_p->Frequency    = Frequency;
    Control_p->Nch          = (U8)NCh;
    Control_p->SampleSize   = SampleSize;

    Control_p->UpdateParams = TRUE;
    return ST_NO_ERROR;

}

/*****************************************************************************
STAud_GetPCMBufferSize()
Description:
    This routine return the total buffers size for PCM input.

Parameters:
    Handle, the handle to this block.
    U32 * BufferSize.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_GetPCMBufferSize(STPCMInput_Handle_t Handle, U32 *BufferSize)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;

    ST_ErrorCode_t Error = ST_NO_ERROR;

    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if((Control_p->PCMCurrentState != PCM_INPUT_STATE_INIT)&&
        (Control_p->PCMCurrentState != PCM_INPUT_STATE_START))
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    STTBX_Print(("STAud_GetPCMBufferSize() Called \n"));

    /* Return Buffer Size */
    *BufferSize = Control_p->PCM_Buff_Size;

    return Error;
}

/*****************************************************************************
STAud_StartPCMInput()
Description:
    This routine starts this unit.

Parameters:
    Handle, the handle to this block.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_StartPCMInput(STPCMInput_Handle_t Handle)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;

    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if ((Control_p->PCMCurrentState == PCM_INPUT_STATE_STOP) ||
        (Control_p->PCMCurrentState == PCM_INPUT_STATE_INIT))
    {
        Control_p->PCMCurrentState = PCM_INPUT_STATE_START;
    }
    else
    {
        return STAUD_ERROR_INVALID_STATE;
    }


    return ST_NO_ERROR;
}

/*****************************************************************************
STAud_StopPCMInput()
Description:
    This routine stops this unit.

Parameters:
    Handle, the handle to this block.

Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t STAud_StopPCMInput(STPCMInput_Handle_t Handle)
{
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;

    if((Handle==(U32)NULL) || (Control_p->Handle!=Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if (Control_p->PCMCurrentState == PCM_INPUT_STATE_START)
    {
        Control_p->PCMCurrentState = PCM_INPUT_STATE_STOP;
    }
    else
    {
        return STAUD_ERROR_INVALID_STATE;
    }
    return ST_NO_ERROR;
}

/*****************************************************************************
STAud_PCMInputBufferParams()
Description:
    This routine returns the buffer params.

Parameters:
    Handle, the handle to this block.
    DataParams_p, Buffer params.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t      STAud_PCMInputBufferParams(STPCMInput_Handle_t Handle, STAUD_BufferParams_t* DataParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;

    if((Handle == (U32)NULL) || (Control_p->Handle != Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Control_p->PCMBlockHandle)
    {
        error = MemPool_GetBufferParams(Control_p->PCMBlockHandle, DataParams_p);
    }
    else
    {
        error = ST_ERROR_NO_MEMORY;
    }

    return error;
}

/*****************************************************************************
STAud_PCMInputHandleEOF()
Description:
    This routine Marks EOF for the next outgoing buffer.

Parameters:
    Handle, the handle to this block.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_INVALID_HANDLE,       Handle invalid or not open.
See Also:

*****************************************************************************/
ST_ErrorCode_t      STAud_PCMInputHandleEOF(STPCMInput_Handle_t Handle)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    STAudPCMInputControl_t * Control_p = (STAudPCMInputControl_t *)Handle;

    if((Handle == (U32)NULL) || (Control_p->Handle != Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if(Control_p->PCMCurrentState != PCM_INPUT_STATE_START)
    {
        return STAUD_ERROR_INVALID_STATE;
    }

    Control_p->EOFFlag = TRUE;

    return error;

}

