/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005,2006. All rights reserved.

   File Name: basic.c
 Description: Provides minimum required interface to PTI transport controller

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stpti.h"
#include "osx.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "memget.h"

#include "pti4.h"
#include "tchal.h"
#include "slotlist.h"
#include "isr.h"

#if defined (STPTI_FRONTEND_HYBRID)
#include "../frontend/stfe/stfe.h"
#include "../frontend/tsgdma/tsgdma_regs.h"
#endif

#ifdef ST_OSLINUX
#include "stpti4_core_proc.h"
#endif

/* ------------------------------------------------------------------------- */

#define START_INDEXING 0x02

#define STC_WORKAROUND  /* !!!!!! */

/* ------------------------------------------------------------------------- */

/* tcinit.c */
extern void           stptiHelper_TCInit_Stop            ( TCDevice_t *TC_Device );
extern void           stptiHelper_TCInit_Start           ( TCDevice_t *TC_Device );
extern void           stptiHelper_TCInit_Hardware        ( FullHandle_t DeviceHandle );
extern void           stptiHelper_TCInit_PidSearchEngine ( TCDevice_t *TC_Device, STPTI_TCParameters_t *TC_Params_p );
extern void           stptiHelper_TCInit_MainInfo        ( TCDevice_t *TC_Device, STPTI_TCParameters_t *TC_Params_p );
extern void           stptiHelper_TCInit_SessionInfo     ( TCDevice_t *TC_Device, STPTI_TCParameters_t *TC_Params_p );
extern ST_ErrorCode_t stptiHelper_TCInit_InterruptDMA    ( FullHandle_t DeviceHandle );
extern ST_ErrorCode_t stptiHelper_TCInit_AllocPrivateData( FullHandle_t DeviceHandle );
extern void           stptiHelper_TCInit_GlobalInfo      ( STPTI_TCParameters_t *TC_Params_p );
extern void stptiHelper_TCInit_SessionInfoModeFlags( FullHandle_t DeviceHandle);
/* tcterm.c */
#if !defined ( STPTI_BSL_SUPPORT )
extern void stptiHelper_TCTerm_FreePrivateData( FullHandle_t DeviceHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* session.c */
extern TCSessionInfo_t *stptiHAL_GetSessionFromFullHandle(FullHandle_t DeviceHandle);


/* Private Function Prototypes --------------------------------------------- */


static ST_ErrorCode_t InitializeDevice(FullHandle_t DeviceHandle, ST_ErrorCode_t (*TCLoader) (STPTI_DevicePtr_t, STPTI_TCParameters_t *));

static void EnableBufferInterrupts(FullHandle_t BufferHandle);


/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : EnableBufferInterrupts
  Description :
   Parameters :
******************************************************************************/
static void EnableBufferInterrupts(FullHandle_t BufferHandle)
{
    TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData( BufferHandle );
    U16                   DMAIdent       = stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *TC_DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    STSYS_ClearTCMask16LE((void*)&TC_DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE );
}



#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_Device
  Description :
   Parameters :
******************************************************************************/
STPTI_Device_t stptiHAL_Device(void)
{
    return( STPTI_DEVICE_PTI_4 );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stptiHAL_FilterAssociateSection
  Description : stpti_TCAssociateSectionFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_FilterAssociateSection(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 SFStatus;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    U32 SlotIdent   = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    
    Filter_t              *FilterInfo_p = stptiMemGet_Filter(FilterHandle);
    U32                    FilterIdent = FilterInfo_p->TC_FilterIdent;
    STPTI_FilterType_t     FilterType = FilterInfo_p->Type;

    TCMainInfo_t          *MainInfo_p = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    TCSessionInfo_t       *TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( SlotHandle );
    TCSessionInfo_t       *TCSessionInfoF_p = stptiHAL_GetSessionFromFullHandle( FilterHandle );
    TCSectionFilterInfo_t *TCSectionFilterInfo_p;

    if( TCSessionInfoF_p != TCSessionInfo_p )  /* sanity check */
    {
        return STPTI_ERROR_INVALID_SLOT_HANDLE;
    }
    if ( STSYS_ReadRegDev16LE((void*)&MainInfo_p->SectionPesFilter_p) != TC_INVALID_LINK)
    {
        /* Get the section/PES filter data block already associated with this slot */

        TCSectionFilterInfo_p =
            (TCSectionFilterInfo_t *) ((U32)STSYS_ReadRegDev16LE((void*)&MainInfo_p->SectionPesFilter_p) -
            (U32)0x8000 + (U32) TC_Params_p->TC_DataStart ); 

    }
    else    /* find a free section/PES filter data block */
    {
        for ( SFStatus = 0; SFStatus < TC_Params_p->TC_NumberSectionFilters; SFStatus++ )
        {
            if ( PrivateData_p->SectionFilterStatus_p[SFStatus] == STPTI_NullHandle() )
                break;
        }

        if (SFStatus >= TC_Params_p->TC_NumberSectionFilters)
            return( STPTI_ERROR_NO_FREE_FILTERS );

        TCSectionFilterInfo_p = &((TCSectionFilterInfo_t *)TC_Params_p->TC_SFStatusStart)[SFStatus];
        
        

#if !defined ( STPTI_BSL_SUPPORT )
        TcCam_ClearSectionInfoAssociations( TCSectionFilterInfo_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

        STTBX_Print(("TCSessionInfo_p = 0x%08x\n", (U32)TCSessionInfo_p ));
        STTBX_Print(("SectionFilterOperatingMode: 0x%08x\n", (U32) stptiMemGet_Device(SlotHandle)->SectionFilterOperatingMode ));
        STTBX_Print(("SessionSectionParams: 0x%08x\n",
            (U32)STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSectionParams )));
        /* do the allocation on the driver & TC sides */
        
        /* Set the section filter running mode */
        TcCam_SetMode(TCSectionFilterInfo_p, stptiMemGet_Slot(SlotHandle), FilterType);
                
        {
            /* Find any buffer that is linked to this slot */
            FullHandle_t    FullBufferHandle;
            Slot_t          *Slot_p = stptiMemGet_Slot( SlotHandle );
            U8              Index;
            List_t          *List_p;
            
            U16                   DMAIdent;
            TCDMAConfig_t        *DMAConfig_p = NULL;
            
            /* Check to see if a BufferList has been created (might not if no buffer linked yet) */
            if( (Slot_p != NULL) && (Slot_p->BufferListHandle.word != STPTI_NullHandle()) )
            {
                /* We have buffer linked, so get the buffer list */
                List_p = stptiMemGet_List(Slot_p->BufferListHandle);

                for(Index = 0 ; Index < STPTI_MAX_BUFFERS_PER_SLOT ; Index++)
                {
                    FullBufferHandle.word = (&List_p->Handle)[Index];
                    if(List_p->MaxHandles <= Index)
                    {
                        break;
                    }
                    if (FullBufferHandle.word != STPTI_NullHandle())
                    {
                        if (FALSE == stptiMemGet_Buffer(FullBufferHandle)->IsRecordBuffer)
                        {
                            /* find the appropriate DMA_Config structure */
                            DMAIdent    = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
                            DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

                            /* Wind back DMA for any partially processed sections */            
                            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAWrite_p, DMAConfig_p->DMAQWrite_p );
                        }
                    }
                }
            }
            /* and reset SectionState - state machine */                         
            STSYS_WriteTCReg16LE((void*)&TCSectionFilterInfo_p->SectionState, 0 );
            
        }                
               
                
        PrivateData_p->SectionFilterStatus_p[SFStatus] = SlotHandle.word;
        STSYS_WriteTCReg16LE( (void*)&MainInfo_p->SectionPesFilter_p,
                              (U16)((U32)TCSectionFilterInfo_p - (U32)TC_Params_p->TC_DataStart + 0x8000) ); 

    }

    /* now set the relevant bit positions in the filter SectionAssociations (filter enable masks) */
    switch ( FilterType )
    {

        case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
            if ((U16)STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionFilterMode) != TC_SESSION_INFO_FILTER_TYPE_LONG)
                return(STPTI_ERROR_INVALID_FILTER_DATA);
            Error = TcCam_AssociateFilterWithSectionInfo( FilterHandle, TCSectionFilterInfo_p, FilterIdent );
            break;
#endif

        case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
            if ((U16)STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionFilterMode) != TC_SESSION_INFO_FILTER_TYPE_NEG_MATCH)
                return(STPTI_ERROR_INVALID_FILTER_DATA);
            Error = TcCam_AssociateFilterWithSectionInfo( FilterHandle, TCSectionFilterInfo_p, FilterIdent );
            break;
#endif

        default:
        case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
            if ((U16)STSYS_ReadRegDev16LE((void*)&TCSectionFilterInfo_p->SectionFilterMode) != TC_SESSION_INFO_FILTER_TYPE_SHORT)
                return(STPTI_ERROR_INVALID_FILTER_DATA);
#endif
            Error = TcCam_AssociateFilterWithSectionInfo( FilterHandle, TCSectionFilterInfo_p, FilterIdent );
            break;
        break;
    }

    return Error;
}


/******************************************************************************
Function Name : stptiHAL_BufferFlush
  Description : stpti_TCBufferFlush
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_BufferFlush(FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Buffer_t             *Buffer_p       = stptiMemGet_Buffer( FullBufferHandle );
    U16                   DMAIdent       = Buffer_p->TC_DMAIdent;
    U32                   DirectDma      = Buffer_p->DirectDma;
    FullHandle_t          SlotListHandle = Buffer_p->SlotListHandle;

    Device_t             *Device         = stptiMemGet_Device( FullBufferHandle );
    TCPrivateData_t      *PrivateData_p  = &Device->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p    =  TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );
    TCDevice_t           *TC_Device      = (TCDevice_t *) Device->TCDeviceAddress_p;
/*    TCGlobalInfo_t       *TC_Global_p    = TcHal_GetGlobalInfo( TC_Params_p ); REMOVE WORKAROUND */
    
    if (SlotListHandle.word != STPTI_NullHandle())
    {
        U32 i;
        U32 NumberOfSlots = stptiMemGet_List(SlotListHandle)->MaxHandles;

        for (i = 0; i < NumberOfSlots; ++i)
        {
            FullHandle_t SlotHandle;
            U32 SlotIdent;

            SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[i];
            if (SlotHandle.word != STPTI_NullHandle())
            {
                SlotIdent = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
                stptiHelper_SetLookupTableEntry(FullBufferHandle, SlotIdent, TC_INVALID_PID);
                /* FIX PMC 29/01/03 - remove as TC_MAIN_INFO_SLOT_STATE_DMA_IN_PROGRESS bit not used any more */
                /* stptiHelper_WaitPacketTime();
                   stptiHelper_WaitForDMAtoComplete(FullBufferHandle, SlotIdent); */
                /* /FIX PMC 29/01/03 */
            }
        }
    }

    /* FIX PMC 29/01/03  -  all pids cleared so only wait for the last to finish */
    stptiHelper_WaitPacketTime();
    /* /FIX PMC 29/01/03 */

    DMAConfig_p->DMARead_p   = DMAConfig_p->DMABase_p;
    DMAConfig_p->DMAWrite_p  = DMAConfig_p->DMABase_p;
    DMAConfig_p->DMAQWrite_p = DMAConfig_p->DMABase_p;

    STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold,
                        (stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff );
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STSYS_WriteTCReg16LE((void*)&DMAConfig_p->BufferLevelThreshold,
                        ((stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff0000) >>16 );
#endif

    DMAConfig_p->BufferPacketCount = 0;

    /* If this buffer is linked to a CDFIFO (DMA 1, 2 or 3) output then reset the CDFIFO read and
      write pointers as well, but not the top & base pointers as in normal operation they are
      the same. Note that the DMA/CDFIFO that outputs data FROM the buffer should not be in
      operation, i.e. allowed to 'run dry' or disabled */

    if ( PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] == TRUE )
    {
    	switch( DirectDma )
        {
            case 1:
                STPTI_CRITICAL_SECTION_BEGIN {
                    /* Read modify & write Flag to tc to reset DMA1 WORKAROUND */
                    TC_Device->DMA1Write = DMAConfig_p->DMARead_p;
                    TC_Device->DMA1Read  = DMAConfig_p->DMAWrite_p;
                } STPTI_CRITICAL_SECTION_END;
                break;

            case 2:
                STPTI_CRITICAL_SECTION_BEGIN {
                    TC_Device->DMA2Write = DMAConfig_p->DMARead_p;
                    TC_Device->DMA2Read  = DMAConfig_p->DMAWrite_p;
                } STPTI_CRITICAL_SECTION_END;
                break;

            case 3:
                STPTI_CRITICAL_SECTION_BEGIN {
                    TC_Device->DMA3Write = DMAConfig_p->DMARead_p;
                    TC_Device->DMA3Read  = DMAConfig_p->DMAWrite_p; 
                } STPTI_CRITICAL_SECTION_END;
                break;

            default:
                Error = STPTI_ERROR_INVALID_CD_FIFO_ADDRESS;    /* something is really wrong! */
                break;
        }
    }

    {   /* BLOCK */
        FullHandle_t SignalListHandle;

        /*Should this flush be done before re-starting the PID filtering??*/
        SignalListHandle = stptiMemGet_Buffer(FullBufferHandle)->SignalListHandle;
        if (SignalListHandle.word != STPTI_NullHandle())
        {
            FullHandle_t FullSignalHandle;
            FullSignalHandle.word = (&stptiMemGet_List(SignalListHandle)->Handle)[0];

            Error = stptiHAL_FlushSignalsForBuffer(FullSignalHandle, FullBufferHandle);
            
        }   /* if (SignalListHandle.word != STPTI_NullHandle()) */

    }   /* /BLOCK */


    EnableBufferInterrupts(FullBufferHandle);   /* Re-enable interrupts on all the flushed buffer  */

    if (SlotListHandle.word != STPTI_NullHandle())
    {
        U32 i;
        U32 MaxNumberOfSlots = stptiMemGet_List(SlotListHandle)->MaxHandles;

        for (i = 0; i < MaxNumberOfSlots; ++i)
        {
            FullHandle_t SlotHandle;

            SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[i];
            if (SlotHandle.word != STPTI_NullHandle())
            {
                U32 SlotIdent = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
                U16 Pid       = stptiMemGet_Slot(SlotHandle)->Pid;
                TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( TC_Params_p, SlotIdent );

                stptiHelper_SetLookupTableEntry(FullBufferHandle, SlotIdent, Pid);
                STSYS_WriteTCReg16LE((void*)&MainInfo_p->SlotState,0);
            }
        }
    }
    return Error;
}


/******************************************************************************
Function Name : stptiHAL_FlushSignalsForBuffer
  Description : Removes and messages (signals) for a buffer (ref'd by FullBufferHandle) 
                from the signal queue (ref'd by FullSignalHandle)
   Parameters : FullBufferHandle, FullSignalHandle
******************************************************************************/

ST_ErrorCode_t stptiHAL_FlushSignalsForBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle)
{
    STOS_MessageQueue_t *Queue_p;
    Queue_p = stptiMemGet_Signal(FullSignalHandle)->Queue_p;

    /* FIX PMC 29/01/03 changed mechanism now parse all the signals in the buffer
       and remove those from the buffer being flushed and resert those from
       all other buffers. Stop when its gone through all the messages
       Before, Signals were flushed and TC signal bit was re-enabled.
    */
    if (Queue_p != NULL)
    {
        STPTI_Buffer_t *buffer_p;
        FullHandle_t    TmpBufferHandle[SIGNAL_QUEUE_SIZE];
        U32             MessageIndex = 0;
#if !defined ( STPTI_BSL_SUPPORT )
        U32             i;
#endif

        STOS_TaskLock();

        while ((buffer_p = STOS_MessageQueueReceiveTimeout(Queue_p, TIMEOUT_IMMEDIATE)) != NULL)
        {
            TmpBufferHandle[MessageIndex].word = buffer_p[0];     /* get handle */
            STOS_MessageQueueRelease(Queue_p, buffer_p);     /* before we throw the message away */

            if (MessageIndex > (int)SIGNAL_QUEUE_SIZE)  /* go through all the queue (or a null if queue not full) */
            {
                break;
            }

            MessageIndex++;
        }

#if !defined ( STPTI_BSL_SUPPORT )
        for(i=0;i<MessageIndex;i++)
        {
            /* Double check for valid buffer handles to prevent exception */
            if ( ST_NO_ERROR ==  stpti_BufferHandleCheck(TmpBufferHandle[i]) )
            {
                if ( TmpBufferHandle[i].word != FullBufferHandle.word)  /* if not our signal then repost it */
                {
                    stptiHelper_SignalBuffer(TmpBufferHandle[i], 0XCCCCCCCC);
                }
            }
        }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

       STOS_TaskUnlock();
    }
    /* /FIX PMC 29/01/03 */
    
    return(ST_NO_ERROR);
}        




/******************************************************************************
Function Name : stptiHAL_FilterDisassociateSection
  Description : stpti_TCDisassociateSectionFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_FilterDisassociateSection(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCSectionFilterInfo_t *TCSectionFilterInfo_p;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    U32 SlotIdent   = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    U32 FilterIdent = stptiMemGet_Filter(FilterHandle)->TC_FilterIdent;

    TCMainInfo_t    *MainInfo_p = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    U32 SFStatus;

    assert(STSYS_ReadRegDev16LE((void*)&MainInfo_p->SectionPesFilter_p) != TC_INVALID_LINK);

    TCSectionFilterInfo_p =
        (TCSectionFilterInfo_t *) ((U32)STSYS_ReadRegDev16LE((void*)&MainInfo_p->SectionPesFilter_p) -
        0x8000 + (U32)TC_Params_p->TC_DataStart );

    /*  Remove bit in filter association fields of section filter status structure, if, as a result,
       the association fields are empty then remove the link between the slot and the filter status
       structure. i.e. free the TC side section filter info block */

#if !defined ( STPTI_BSL_SUPPORT )
    switch ( stptiMemGet_Filter(FilterHandle)->Type )
    {

    default:                                    /* Non-DTV filter type */
        Error = TcCam_DisassociateFilterFromSectionInfo( FilterHandle, TCSectionFilterInfo_p, FilterIdent );
        break;
    }

    if ( TcCam_SectionInfoNothingIsAssociated( TCSectionFilterInfo_p ) )
    {
        /* Free the sectionfilter driver side status (info) structure */

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
        for (SFStatus = 0; (SFStatus < TC_Params_p->TC_NumberSectionFilters); ++SFStatus)
        {
            if (PrivateData_p->SectionFilterStatus_p[SFStatus] == SlotHandle.word)
            {
                PrivateData_p->SectionFilterStatus_p[SFStatus] = STPTI_NullHandle();
            }
        }
        STSYS_WriteTCReg16LE((void*)&MainInfo_p->SectionPesFilter_p,TC_INVALID_LINK);
#if !defined ( STPTI_BSL_SUPPORT )
    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    return Error;
}





/******************************************************************************
Function Name : stptiHAL_FilterAllocateSection
  Description : stpti_TCGetFreeSectionFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_FilterAllocateSection(FullHandle_t DeviceHandle, STPTI_FilterType_t FilterType, U32 *SFIdent)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* To remove warning: "warning: 'filter' might be used uninitialized in this function" */
    U32 filter = 0;

    switch (FilterType)
    {

        default:    /* DVB filter type */
            Error = TcCam_FilterAllocate( DeviceHandle, FilterType, &filter );
            break;
    }

    if (Error == ST_NO_ERROR)
    {
        *SFIdent = filter;
    }

    return Error;
}


/******************************************************************************
Function Name : stptiHAL_SlotAllocate
  Description : stpti_TCGetFreeSlot
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_SlotAllocate(FullHandle_t DeviceHandle, U32 *SlotIdent)
{
    Device_t              *Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t       *PrivateData_p = &Device->TCPrivateData;
    STPTI_TCParameters_t  *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    volatile STPTI_Slot_t *SlotHandle_p;
    TCSessionInfo_t       *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device->Session ];

    FullHandle_t Handle;
    U32 Index, Slot, MinAddr, MaxAddr;


    STTBX_Print(("\nstptiHAL_SlotAllocate()\n"));

    SlotHandle_p = PrivateData_p->SlotHandles_p;

    MinAddr = STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionPIDFilterStartAddr );
    MaxAddr = MinAddr + (STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionPIDFilterLength )*2 )- 1;

    assert( MinAddr != 0 ); /* if  MinAddr == 0 then its an internal error (slotlist.c:Init) */

    STTBX_Print(("Scanning from 0x%x to 0x%x \n", MinAddr, MaxAddr));

    for (Slot = MinAddr; Slot <= MaxAddr; Slot+=2)
    {
        Index = (Slot - 0x8000) >> 1;   /* count words */

        Handle.word = SlotHandle_p[Index];

        assert( Index < TC_Params_p->TC_NumberSlots );

        if (Handle.word == STPTI_NullHandle())
        {
            STTBX_Print(("Found free handle at index %d for slot 0x%x\n", Index, Slot));

            *SlotIdent = Index;
            return( ST_NO_ERROR );
        }
    }

    return( STPTI_ERROR_NO_FREE_SLOTS );
}


/******************************************************************************
Function Name : stptiHAL_Init
  Description : stpti_TCInit
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_Init(FullHandle_t DeviceHandle, FullHandle_t ExistingSessionDeviceHandle, ST_ErrorCode_t (*TCLoader) (STPTI_DevicePtr_t, void *))
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Device_t        *Device_p      = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t *PrivateData_p = &Device_p->TCPrivateData;
    TCDevice_t      *TC_Device     = (TCDevice_t *)  Device_p->TCDeviceAddressPhysical_p;
    TCPrivateData_t *ePrivateData_p; /* don't yet point to anything in case of side effects from random reads. */
    TCSessionInfo_t *TCSessionInfo_p;
    
    if ( ExistingSessionDeviceHandle.word != STPTI_NullHandle() )   /* does a previous session exists on this pti cell? */
    {
        ePrivateData_p = &stptiMemGet_Device(ExistingSessionDeviceHandle)->TCPrivateData;

        /* copy commoned params & pointers from existing to new device (handle) */
        STTBX_Print(("\n stptiHAL_Init: TCPrivateData_t: copying %d bytes from %x to %x\n", sizeof( TCPrivateData_t ), (int)ePrivateData_p, (int)PrivateData_p ));

        /* cast to 'void *' to avoid compiler barf. */
        memcpy( (void *)PrivateData_p,  (void *)ePrivateData_p, sizeof( TCPrivateData_t ) );
        
        Device_p->TCDeviceAddress_p = stptiMemGet_Device(ExistingSessionDeviceHandle)->TCDeviceAddress_p;
        
    }
    else    /* it is the first session on this specific pti hardware cell - according to STPTI_Init(). */
    {
        char MapRegistersString[16] = {0};
        
        memset( (void *)PrivateData_p, 0, sizeof( TCPrivateData_t ) );
        
#if defined (STPTI_FRONTEND_HYBRID)
        if ( STPTI_Driver.Physical_PTIs_Mapped == 0 )
        {
            /* Map 7200 FrontEnd */
            STPTI_Driver.STFE_Base_Address = (U32)STOS_MapRegisters((void *)ST7200_STFE_BASE_ADDRESS, STFE_REGION_SIZE, "STFE");
            STPTI_Driver.STFE_Internal_RAM_Base_Address = (U32)STOS_MapRegisters((void *)(ST7200_STFE_BASE_ADDRESS + 0x01100000), STFE_INTERNAL_RAM_SIZE, "STFE_RAM");
            STPTI_Driver.TSGDMA_Base_Address = (U32)STOS_MapRegisters((void *)TSG_DMA_PTI_BASE_ADDRESS, TSGDMA_REGION_SIZE, "TSGDMA");
        }
#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */
    
        /* Need to map the PTI register and TC Memory region */
        memcpy( MapRegistersString, "STPTI4_x", 9);     /* 8 chars plus null terminator */
        MapRegistersString[7]='0'+(char)DeviceHandle.member.Device;
#if defined(ST_OS20) || defined(ST_5301)
        /* OS20 not properly supported under STOS yet */
#else
        TC_Device=(TCDevice_t *)STOS_MapRegisters((void*)TC_Device,sizeof(TCDevice_t),MapRegistersString);
#endif
        Device_p->TCDeviceAddress_p = (STPTI_DevicePtr_t) TC_Device;
        STPTI_Driver.Physical_PTIs_Mapped++;

        Error = InitializeDevice(DeviceHandle, (ST_ErrorCode_t (*)(STPTI_DevicePtr_t, STPTI_TCParameters_t *))TCLoader);
    }

    if (Error == ST_NO_ERROR)
    {
        /* setup session id - after any possibility of loader being installed by InitializeDevice() at it may erase this info from tc ram! */
        /* Not only gets the session, but also sets the session in the device struct */
        if ( stptiHAL_GetNextFreeSession( DeviceHandle ) == SESSION_ILLEGAL_INDEX )
        {
            /* StreamId already in use */
            Error = ST_ERROR_BAD_PARAMETER;
        }
    }

    if ( Error == ST_NO_ERROR )
    {
        stptiHelper_TCInit_SessionInfoModeFlags(DeviceHandle);
        STTBX_Print(("\n stptiHAL_Init: Session:%d, Error:%d\n", Device_p->Session, Error));

        Error = stptiHelper_SlotList_Alloc( DeviceHandle );

        if (Error == ST_NO_ERROR)
        {
            /* install irq handler for a specific pti cell if it is the very first session on that cell */
            if ( ExistingSessionDeviceHandle.word == STPTI_NullHandle() )
            {
                STTBX_Print(("interrupt_install...  "));

                if( STOS_InterruptInstall( Device_p->InterruptNumber, Device_p->InterruptLevel,
                                           stptiHAL_InterruptHandler, "PTI", (void *) DeviceHandle.word ) == STOS_FAILURE )
                {
                    STTBX_Print(("failed\n"));
                    return( ST_ERROR_INTERRUPT_INSTALL );
                }
                else
                {
                    Device_p->IRQHolder = TRUE;
                    STTBX_Print(("successful\n"));
                }

                STTBX_Print(("interrupt_enable...  "));
                if ( STOS_InterruptEnable( Device_p->InterruptNumber, Device_p->InterruptLevel ) == STOS_FAILURE )
                {
                    STTBX_Print(("failed\n"));
                }
                else
                {
                    STTBX_Print(("successful\n"));
                }

                STPTI_SYSTEM_IRQ_ENABLE; /* enable interrupt(s) to the Host. */
               
                STSYS_WriteRegDev32LE((void*)&TC_Device->PTIIntEnable0, 3);
            }

            TCSessionInfo_p = stptiHAL_GetSessionFromFullHandle( DeviceHandle );
            Device_p = stptiMemGet_Device( DeviceHandle );  /* re-get in case of allocation shift */

            Error = TcCam_FilterAllocateSession( DeviceHandle );
        }

    }
    else
    {
        STTBX_Print(("\nstptiHAL_Init() FAILED to initialise device\n"));
    }

    STTBX_Print(("\nstptiHAL_Init() END\n"));
    
    return( Error );
}


/******************************************************************************
Function Name : stptiHAL_FilterInitialiseSection
  Description : stpti_TCInitialiseSectionFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_FilterInitialiseSection(FullHandle_t DeviceHandle, U32 SFIdent, STPTI_Filter_t FilterHandle,
                                                STPTI_FilterType_t FilterType)
{
    stptiMemGet_Device(DeviceHandle)->TCPrivateData.SectionFilterHandles_p[SFIdent] = FilterHandle;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_SlotInitialise
  Description : stpti_TCInitialiseSlot
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_SlotInitialise(FullHandle_t DeviceHandle, STPTI_SlotData_t * SlotData, U32 SlotIdent,
                                       STPTI_Slot_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

#ifdef STPTI_CAROUSEL_SUPPORT
    if (SlotData->SlotFlags.AlternateOutputInjectCarouselPacket && (TC_Params_p->TC_NumberCarousels == 0))
    {
        Error = STPTI_ERROR_ALT_OUT_TYPE_NOT_SUPPORTED;
    }
    else
#endif
    {
        TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( TC_Params_p, SlotIdent );

   		STSYS_WriteTCReg16LE((void*)&MainInfo_p->SlotState,          0);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->SlotMode,           0);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->DescramblerKeys_p,  TC_INVALID_LINK);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->DMACtrl_indices,    0xFFFF);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->StartCodeIndexing_p,0);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->SectionPesFilter_p, TC_INVALID_LINK);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->RemainingPESLength, 0);
		STSYS_WriteTCReg16LE((void*)&MainInfo_p->PacketCount,        0);

        PrivateData_p->SlotHandles_p[SlotIdent]    = SlotHandle;
        PrivateData_p->PCRSlotHandles_p[SlotIdent] = STPTI_NullHandle();

        switch (SlotData->SlotType)
        {

        case STPTI_SLOT_TYPE_NULL:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_SLOT_TYPE_NULL );


#ifdef STPTI_CAROUSEL_SUPPORT
            if (SlotData->SlotFlags.AlternateOutputInjectCarouselPacket)
            {
                FullHandle_t CarouselHandle;

                CarouselHandle.word = stptiMemGet_Device(DeviceHandle)->CarouselHandle;
                if (CarouselHandle.word != STPTI_NullHandle())
                {
                    if ( stptiMemGet_Carousel(CarouselHandle)->OutputAllowed == STPTI_ALLOW_OUTPUT_SELECTED_SLOTS )
                    {
                        STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, SLOT_MODE_SUBSTITUTE_STREAM );
                    }
                }
                else
                {
                    STTBX_Print(("stptiHAL_SlotInitialise->We have an invalid carousel handle at this stage\n"));
                }
            }
#endif
            break;

        case STPTI_SLOT_TYPE_SECTION:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_SLOT_TYPE_SECTION );
            break;

        case STPTI_SLOT_TYPE_PES:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_SLOT_TYPE_PES );
            break;

        case STPTI_SLOT_TYPE_EMM:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_SLOT_TYPE_SECTION );
            break;

        case STPTI_SLOT_TYPE_ECM:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_SLOT_TYPE_SECTION );
            break;

        case STPTI_SLOT_TYPE_RAW:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_SLOT_TYPE_RAW);

            break;

        case STPTI_SLOT_TYPE_PCR:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_SLOT_TYPE_NULL) );
            break;


#ifdef STPTI_DC2_SUPPORT
        case STPTI_SLOT_TYPE_DC2_PRIVATE:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_SLOT_TYPE_DC2_PRIVATE) );
            break;

        case STPTI_SLOT_TYPE_DC2_MIXED:
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_SLOT_TYPE_DC2_MIXED) );
            break;
#endif
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
        }

        if (SlotData->SlotFlags.StoreLastTSHeader)
        {
            ;
        }
    }
    return Error;
}

/******************************************************************************
Function Name : InitializeDevice
  Description : Initialize the transport controller, zero its memory,
                clear its data structures, load its code, and start it
                running.

   Parameters : Session - if zero then this is the very first session for the pti cell.
******************************************************************************/
static ST_ErrorCode_t InitializeDevice(FullHandle_t DeviceHandle, ST_ErrorCode_t (*TCLoader) (STPTI_DevicePtr_t, STPTI_TCParameters_t *))
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    TCDevice_t           *TC_Device     =           (TCDevice_t *)  stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;


    STTBX_Print(("\nInitializeDevice(): *** FULL LOADER INITALIZATION ***\n"));
    STTBX_Print(("InitializeDevice():     TC_Device = 0x%08x\n", (int)TC_Device ));
    STTBX_Print(("InitializeDevice():   TC_Params_p = 0x%08x\n", (int)TC_Params_p ));
    STTBX_Print(("InitializeDevice(): PrivateData_p = 0x%08x\n", (int)PrivateData_p ));

    stptiHelper_TCInit_Stop( TC_Device );

    Error = TCLoader(&TC_Device->TC_Code[0], (void *) TC_Params_p);
    if (Error != ST_NO_ERROR)
        return( Error );

#if defined(STPTI_710x_EARLY_CUT_WORKAROUND)
    stpti_710xEarlyCutWorkaround((U32)TC_Params_p->TC_CodeStart, (U32)TC_Params_p->TC_CodeSize);
#endif /* STPTI_710x_EARLY_CUT_WORKAROUND */
    
    stptiHelper_TCInit_Hardware( DeviceHandle );

    stptiHelper_TCInit_PidSearchEngine( TC_Device, TC_Params_p );

    stptiHelper_TCInit_GlobalInfo( TC_Params_p );

    stptiHelper_TCInit_MainInfo( TC_Device, TC_Params_p );
    stptiHelper_TCInit_SessionInfo( TC_Device, TC_Params_p );

    Error = stptiHelper_SlotList_Init( DeviceHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    Error = stptiHelper_TCInit_InterruptDMA( DeviceHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    Error = stptiHelper_TCInit_AllocPrivateData( DeviceHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    Error = TcCam_Initialize( DeviceHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    stptiHelper_TCInit_Start( TC_Device );

    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_SlotUnLinkGeneral
  Description : stpti_TCRemoveGeneralDMA
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotUnLinkGeneral(FullHandle_t SlotHandle, FullHandle_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData(SlotHandle);
    U32                   SlotIdent      = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    FullHandle_t          SlotListHandle = stptiMemGet_Buffer(BufferHandle)->SlotListHandle;

    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    TcHal_MainInfoUnlinkDmaWithSlot(TC_Params_p, SlotIdent);

    if (SlotListHandle.word != STPTI_NullHandle())
    {
        U32 UsedHandles = stptiMemGet_List(SlotListHandle)->UsedHandles;

        if (UsedHandles == 1)
        {
            U32             TC_DMAIdent;
            TCDMAConfig_t  *DMAConfig_p;

            /* this is the only slot linked to this buffer so.... clean up !! */
            TC_DMAIdent = stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent;

            PrivateData_p->BufferHandles_p[TC_DMAIdent] = STPTI_NullHandle();
            stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent = UNININITIALISED_VALUE;

            /* Disable signalling on this buffer */
            DMAConfig_p  = &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[TC_DMAIdent];
            
            STSYS_SetTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
            STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, ( TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK | TC_DMA_CONFIG_WINDBACK_ON_ERROR | TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA ) );
        }
    }
    return Error;
}

/******************************************************************************
Function Name : stptiHAL_SlotUnLinkRecord
  Description : 
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotUnLinkRecord(FullHandle_t SlotHandle, FullHandle_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData(SlotHandle);
    U32                   SlotIdent      = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    FullHandle_t          SlotListHandle = stptiMemGet_Buffer(BufferHandle)->SlotListHandle;

    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p     = TcHal_GetMainInfo( TC_Params_p, SlotIdent );


    /* Clear MainInfo watch & record data */
    STSYS_ClearTCMask16LE((void*)&MainInfo_p->StartCodeIndexing_p, (TC_MAIN_INFO_REC_BUFFER_MODE_ENABLE | TC_MAIN_INFO_REC_BUFFER_MODE_DESCRAMBLE));

    TcHal_MainInfoUnlinkRecordDmaWithSlot(TC_Params_p, SlotIdent);

    if (SlotListHandle.word != STPTI_NullHandle())
    {
        U32 UsedHandles = stptiMemGet_List(SlotListHandle)->UsedHandles;

        if (UsedHandles == 1)
        {
            U32              TC_DMAIdent;
            TCDMAConfig_t   *DMAConfig_p;

            /* this is the last slot linked to this buffer so.... clean up !! */
            TC_DMAIdent = stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent;

            PrivateData_p->BufferHandles_p[TC_DMAIdent] = STPTI_NullHandle();
            stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent = UNININITIALISED_VALUE;

            /* Disable signalling on this buffer */
            DMAConfig_p  = &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[TC_DMAIdent];

            STSYS_SetTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
            STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, ( TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK | TC_DMA_CONFIG_WINDBACK_ON_ERROR | TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA ) );

            stptiMemGet_Buffer( BufferHandle )->IsRecordBuffer = FALSE;
        }
    }
    return Error;
}


/******************************************************************************
Function Name : stptiHAL_SlotUnLinkDirect
  Description : stpti_TCRemoveSlotDirectDMA
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotUnLinkDirect(FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32                   slot          = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    U32                   TC_DMAIdent   = stptiMemGet_Slot(SlotHandle)->TC_DMAIdent;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    TcHal_MainInfoUnlinkDmaWithSlot(TC_Params_p, slot);
    
    PrivateData_p->BufferHandles_p[TC_DMAIdent] = STPTI_NullHandle();

    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT )    */

/******************************************************************************
Function Name : stptiHAL_ErrorEventModifyState
  Description : SetTCInterruptMask
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_ErrorEventModifyState(FullHandle_t DeviceHandle, STPTI_Event_t EventName, BOOL Enable)
{
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    Device_t             *Device_p        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];
    TCDevice_t           *TC_Device_p = (TCDevice_t *)  Device_p->TCDeviceAddress_p;

    switch (EventName)
    {
        case STPTI_EVENT_BUFFER_OVERFLOW_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_BUFFER_OVERFLOW);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_BUFFER_OVERFLOW);
            }
            break;

        case STPTI_EVENT_CC_ERROR_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_CC);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_CC);
            }
            break;

#if !defined ( STPTI_BSL_SUPPORT )
       case STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_DESCRAMBLE_KEY);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_DESCRAMBLE_KEY);
            }
            break;
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
            
        case STPTI_EVENT_INVALID_LINK_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_LINK);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_LINK);
            }
            break;

        case STPTI_EVENT_INVALID_PARAMETER_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_PARAMETER);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_INVALID_PARAMETER);
            }
            break;

        case STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_SECTION_CRC_ERROR);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_SECTION_CRC_ERROR);
            }
            break;

        case STPTI_EVENT_PACKET_ERROR_EVT:
            if (Enable)
            {
                STSYS_SetRegMask32LE( (void*)&TC_Device_p->PTIIntEnable0, INTERRUPT_STATUS_PACKET_ERROR_INTERRUPT );
            }
            else
            {
                STSYS_ClearRegMask32LE( (void*)&TC_Device_p->PTIIntEnable0, INTERRUPT_STATUS_PACKET_ERROR_INTERRUPT );
            }
            break;

#ifdef STPTI_CAROUSEL_SUPPORT
        case STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_SUBSTITUTE_COMPLETE);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_SUBSTITUTE_COMPLETE);
            }
            break;
#endif
#if !defined ( STPTI_BSL_SUPPORT )
        case  STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT:
        case  STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, (STATUS_FLAGS_SCRAMBLE_CHANGE >> 16));
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, (STATUS_FLAGS_SCRAMBLE_CHANGE >> 16));
            }
            break;
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

	case STPTI_EVENT_PES_ERROR_EVT:
            if (Enable)
            {
                STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_PES_ERROR);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_PES_ERROR);
            }
            break;

        default:
            break;
    }

    return Error;
}



/******************************************************************************
Function Name : stptiHAL_SlotLinkToBuffer
  Description : stpti_TCSetUpGeneralDMA
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_SlotLinkToBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle, BOOL DMAExists)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData(FullSlotHandle);
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    Slot_t               *SlotStruct_p   = stptiMemGet_Slot(FullSlotHandle);
    Buffer_t             *BufferStruct_p = stptiMemGet_Buffer(FullBufferHandle);
    U32                   SlotIdent      = SlotStruct_p->TC_SlotIdent;
    U32                   dma;

    if (DMAExists)
    {
        dma = BufferStruct_p->TC_DMAIdent;
        TcHal_MainInfoAssociateDmaWithSlot( TC_Params_p, SlotIdent, dma );
    }
    else
    {
        for (dma = 0; dma < TC_Params_p->TC_NumberDMAs; ++dma)
        {
            if (PrivateData_p->BufferHandles_p[dma] == STPTI_NullHandle())
                break;
        }

        if (dma < TC_Params_p->TC_NumberDMAs)
        {
            TCDMAConfig_t *DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, dma );

            U32 base = (U32) BufferStruct_p->Start_p;
            U32 top  = (U32) (base + BufferStruct_p->ActualSize);

            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMABase_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMATop_p, (top - 1) & ~0xf );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMARead_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAWrite_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAQWrite_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->BufferPacketCount, 0 );

            /*Reset SignalModeFlags as this could have been a previously used DMA and the flags may be in an
            un-defined state */
            
            STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK);

            /* By default, allow winding back (to the last quantisation point) on PES error */                
            STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_WINDBACK_ON_ERROR);
            
            /* if a signal has been associated with the buffer then set up signalling */
            if (BufferStruct_p->SignalListHandle.word != STPTI_NullHandle() )
            {
                if (SlotStruct_p->Flags.SignalOnEveryTransportPacket)
                {
                    /* Force quantisation unit to be 1 transport packet */
                    STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS);
                }
                else
                {
                    /* For Normal case, or SoftwareCDFifo with a signal assoc'd, used quantised signalling */
                    STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION);
                }

#if !defined( STPTI_SWCDFIFO_QUANTISED_PES )
                if(SlotStruct_p->Flags.SignalOnEveryTransportPacket)
                {
                    /* SoftwareCDFifo means don't wind back on Error */
                    STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_WINDBACK_ON_ERROR);
                }
#endif
                
                /* Enable signalling */
                STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
            }
            else
            {
                /* No signals assoc'd so disable signalling */
                STSYS_SetTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);

                if(SlotStruct_p->Flags.SoftwareCDFifo)
                {
                    STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_SWCDFIFO);
#if !defined( STPTI_SWCDFIFO_QUANTISED_PES )
                    /* Prevent winding back on PES error */
                    STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_WINDBACK_ON_ERROR);
#endif
                }

            }
            
            /* Add flag to indicate if Metadata is needed to be output for PES packets (note overridden by SWCDFIFO */
            if (SlotStruct_p->Flags.OutPesWithoutMetadata || SlotStruct_p->Flags.SignalOnEveryTransportPacket || SlotStruct_p->Flags.SoftwareCDFifo)
            {
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA);
            }
            
            STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, ( BufferStruct_p->MultiPacketSize ) & 0xffff );
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
            STSYS_WriteTCReg16LE((void*)&DMAConfig_p->BufferLevelThreshold, 0 );
            BufferStruct_p->MultiPacketSize &= 0x0000ffff;
#endif
            TcHal_MainInfoAssociateDmaWithSlot( TC_Params_p, SlotIdent, dma );

            BufferStruct_p->TC_DMAIdent = dma;
            PrivateData_p->BufferHandles_p[dma] = FullBufferHandle.word;
        }
        else
        {

            Error = STPTI_ERROR_NO_FREE_DMAS;
        }
    }

    return Error;
}

#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_SlotLinkToRecordBuffer
  Description : Setup DMA params for session record buffer.
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_SlotLinkToRecordBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle, BOOL DMAExists, BOOL DescrambleTS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCPrivateData_t      *PrivateData_p    = stptiMemGet_PrivateData(FullSlotHandle);
    STPTI_TCParameters_t *TC_Params_p      = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    Slot_t               *SlotStruct_p     = stptiMemGet_Slot(FullSlotHandle);
    Buffer_t             *BufferStruct_p   = stptiMemGet_Buffer(FullBufferHandle);
    U32                   SlotIdent        = SlotStruct_p->TC_SlotIdent;
    TCMainInfo_t         *MainInfoStruct_p = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    U32                   dma;

    if (DMAExists)
    {
        /* Set Slot MainInfo to use record buffer & whether to record scrambled or clear data */
        /* NOTE we reuse StartCodeIndexing_p in Main Info for this */
        STSYS_ClearTCMask16LE((void*)&MainInfoStruct_p->StartCodeIndexing_p, (TC_MAIN_INFO_REC_BUFFER_MODE_ENABLE | TC_MAIN_INFO_REC_BUFFER_MODE_DESCRAMBLE));
        STSYS_SetTCMask16LE((void*)&MainInfoStruct_p->StartCodeIndexing_p, TC_MAIN_INFO_REC_BUFFER_MODE_ENABLE );
	
        if ( TRUE == DescrambleTS )
        {
            STSYS_SetTCMask16LE((void*)&MainInfoStruct_p->StartCodeIndexing_p, TC_MAIN_INFO_REC_BUFFER_MODE_DESCRAMBLE );
        }

        dma = BufferStruct_p->TC_DMAIdent;
        TcHal_MainInfoAssociateRecordDmaWithSlot( TC_Params_p, SlotIdent, dma );
    }
    else
    {
         /* Create new DMA for session watch & record */
        for (dma = 0; dma < TC_Params_p->TC_NumberDMAs; ++dma)
        {
            if (PrivateData_p->BufferHandles_p[dma] == STPTI_NullHandle())
                break;
        }

        if (dma < TC_Params_p->TC_NumberDMAs)
        {
            TCDMAConfig_t *DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, dma );

            U32 base = (U32) BufferStruct_p->Start_p;
            U32 top  = (U32) (base + BufferStruct_p->ActualSize);

            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMABase_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMATop_p, (top - 1) & ~0xf );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMARead_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAWrite_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAQWrite_p, base );
            STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->BufferPacketCount, 0 );

            /*Reset SignalModeFlags as this could have been a previously used DMA and the flags may be in an
            un-defined state */
            STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK);

            /* if a signal has been associated with the buffer then set up signalling */
            if (BufferStruct_p->SignalListHandle.word != STPTI_NullHandle() )
            {
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS);
                STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
            }
            else
            {
                STSYS_SetTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
            }

            STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, ( BufferStruct_p->MultiPacketSize ) & 0xffff );
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
            STSYS_WriteTCReg16LE((void*)&DMAConfig_p->BufferLevelThreshold, 0 );
            BufferStruct_p->MultiPacketSize &= 0x0000ffff;
#endif

            /* Set Slot MainInfo to use record buffer & whether to record scrambled or clear data */
            /* NOTE we reuse StartCodeIndexing_p in Main Info for this */
            STSYS_ClearTCMask16LE((void*)&MainInfoStruct_p->StartCodeIndexing_p, (TC_MAIN_INFO_REC_BUFFER_MODE_ENABLE | TC_MAIN_INFO_REC_BUFFER_MODE_DESCRAMBLE));
            STSYS_SetTCMask16LE((void*)&MainInfoStruct_p->StartCodeIndexing_p, TC_MAIN_INFO_REC_BUFFER_MODE_ENABLE );

            if ( TRUE == DescrambleTS )
            {
                STSYS_SetTCMask16LE((void*)&MainInfoStruct_p->StartCodeIndexing_p, TC_MAIN_INFO_REC_BUFFER_MODE_DESCRAMBLE );
            }

            /* Update the DMA pointer in the Session info in Data RAM */
            TcHal_MainInfoAssociateRecordDmaWithSlot( TC_Params_p, SlotIdent, dma );

            stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent = dma;
            /* Save the buffer handle in the dma list */
            PrivateData_p->BufferHandles_p[dma] = FullBufferHandle.word;
        }
        else
        {

            Error = STPTI_ERROR_NO_FREE_DMAS;
        }
    }

    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : stptiHAL_SlotClearPid
  Description : stpti_TCSlotClearPid
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_SlotClearPid(FullHandle_t SlotHandle, BOOL WindbackDMA, U32 AlternativeTCIdentIfPCR)
{
    U32                   TC_SlotIdent  = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
#if !defined (STPTI_BSL_SUPPORT)
    U32                   SlotType      = stptiMemGet_Slot(SlotHandle)->Type;

    Device_t             *Device_p      = stptiMemGet_Device(SlotHandle);
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#if !defined (STPTI_BSL_SUPPORT)
    STPTI_Pid_t OldPid                  = ( stptiMemGet_Slot( SlotHandle ))->Pid;
    TCGlobalInfo_t       *TC_Global_p   = TcHal_GetGlobalInfo( TC_Params_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    TCMainInfo_t         *MainInfo      = TcHal_GetMainInfo( TC_Params_p, TC_SlotIdent );

#if !defined (STPTI_BSL_SUPPORT)
    U32 InterruptMaskFlags = STATUS_FLAGS_PCR_FLAG | STATUS_FLAGS_DISCONTINUITY_INDICATOR;
    TCSessionInfo_t   *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    stptiHelper_SetLookupTableEntry(SlotHandle, TC_SlotIdent, TC_INVALID_PID);
    stptiHelper_WaitPacketTime();
    if ( WindbackDMA == TRUE )
    {
        stptiHelper_WindBackDMA(SlotHandle, TC_SlotIdent);

        /* Reset Slot Status Word in slot's Main Info */
        STSYS_WriteTCReg16LE((void*)&MainInfo->SlotState, 0 );
    }

#if !defined (STPTI_BSL_SUPPORT)
    if ( OldPid == STPTI_WildCardPid() )
    {    
        STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionModeFlags, TC_SESSION_MODE_FLAGS_WILDCARD_PID_SET );
    }

    if ( OldPid == STPTI_NegativePid() )
    {
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalNegativePidMatchingEnable, 0x0000 );
    }


    if ( ( SlotType == STPTI_SLOT_TYPE_PCR ) && ( AlternativeTCIdentIfPCR != 0xFFFFFFFF ) )
    {
        /* Disable the PCR interrupts on the active slot (probably video) */
        MainInfo = TcHal_GetMainInfo( TC_Params_p, AlternativeTCIdentIfPCR );
        STSYS_ClearTCMask16LE((void*)&MainInfo->IndexMask, (InterruptMaskFlags>>16) );
    }
    else if ( stptiMemGet_Slot(SlotHandle)->PCRReturnHandle != STPTI_NullHandle() )
    {
        /* Disable the PCR interrupts if we are collecting pcrs on this slot */
        STSYS_ClearTCMask16LE((void*)&MainInfo->IndexMask, (InterruptMaskFlags>>16) );
        STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1, START_INDEXING );
    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    return( ST_NO_ERROR );
}

#if !defined (STPTI_BSL_SUPPORT)

/******************************************************************************
Function Name : stptiHAL_SlotSetPCRReturn
  Description : stpti_TCSlotSetPCRReturn
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_SlotSetPCRReturn(FullHandle_t SlotHandle)
{
    U32          TC_SlotIdent    = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    STPTI_Slot_t PCRReturnHandle = stptiMemGet_Slot(SlotHandle)->PCRReturnHandle;

    stptiMemGet_Device(SlotHandle)->TCPrivateData.PCRSlotHandles_p[TC_SlotIdent] = PCRReturnHandle;

    return ST_NO_ERROR;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stptiHAL_SlotSetPid
  Description : stpti_TCSlotSetPid
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotSetPid(FullHandle_t SlotHandle, U16 Pid, U16 MapPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32                   TC_SlotIdent  = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
#if !defined ( STPTI_BSL_SUPPORT )
    U32                   SlotType      = stptiMemGet_Slot(SlotHandle)->Type;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, TC_SlotIdent );
    TCGlobalInfo_t       *TC_Global_p   = TcHal_GetGlobalInfo( TC_Params_p );
    Device_t             *Device_p      = stptiMemGet_Device(SlotHandle);
    TCSessionInfo_t      *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    if ( Pid == STPTI_WildCardPid() )
    {
        MapPid = 0; /* The mapped PID must be zero */
        STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionModeFlags, TC_SESSION_MODE_FLAGS_WILDCARD_PID_SET );        
    }

    if ( Pid == STPTI_NegativePid() )
    {
        MapPid = 0; /* The mapped PID must be zero */
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalNegativePidSlotIdent, TC_SlotIdent );
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalNegativePidMatchingEnable, STPTI_NegativePid() );
    }

    /*Set the slot to allow PCR interrupts on PCR slot or if the PCRReturn Handle is set */
    if (SlotType == STPTI_SLOT_TYPE_PCR || stptiMemGet_Slot(SlotHandle)->PCRReturnHandle != STPTI_NullHandle())
    {
        Error = stptiHAL_SlotEnableIndexing( SlotHandle );
    }

    STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotState, (SLOT_STATE_INITIAL_SCRAMBLE_STATE) );

    /* Clear the PID */
    stptiHelper_SetLookupTableEntry(SlotHandle, TC_SlotIdent, STPTI_InvalidPid());

    if (MapPid != STPTI_InvalidPid())
    {
        /* Set the Mapping PID */
        STSYS_WriteTCReg16LE((void*)&MainInfo_p->RemainingPESLength, MapPid);
    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    /* Set the PID */
    stptiHelper_SetLookupTableEntry(SlotHandle, TC_SlotIdent, Pid);

    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_SlotEnableIndexing
  Description : Allow the PCR indexing event to happen without linking INDEX_SUPPORT
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotEnableIndexing(FullHandle_t FullSlotHandle)
{
    U32                   TC_SlotIdent  = stptiMemGet_Slot(FullSlotHandle)->TC_SlotIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullSlotHandle);
    Device_t             *Device_p      = stptiMemGet_Device(FullSlotHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo      = TcHal_GetMainInfo( TC_Params_p, TC_SlotIdent );
    TCSessionInfo_t   *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    U32 InterruptMaskFlags = STATUS_FLAGS_PCR_FLAG | STATUS_FLAGS_DISCONTINUITY_INDICATOR;

    STSYS_SetTCMask16LE((void*)&MainInfo->IndexMask, (InterruptMaskFlags >> 16) );
    STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1, (START_INDEXING) );

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_SlotState
  Description : stpti_TCSlotState
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotState(FullHandle_t FullSlotHandle, U32 *SlotCount_p,
                                  STPTI_ScrambleState_t *ScrambleState_p, STPTI_Pid_t *Pid_p, U32 RecurseDepth)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullSlotHandle);
    U32                   SlotIdent     = stptiMemGet_Slot(FullSlotHandle)->TC_SlotIdent;

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    STPTI_Pid_t           PidInSlotStructure;


    TcHal_LookupTableGetPidForSlot( TC_Params_p, SlotIdent, Pid_p );

    PidInSlotStructure = stptiMemGet_Slot(FullSlotHandle)->Pid;

    RecurseDepth = RecurseDepth & 0xff; /* protect RecurseDepth against large values in case of memory corruption. */
    if (RecurseDepth == 0)              /* if countdown from MAX_RECURSE_DEPTH then bounce back up with error. */
        return( ST_ERROR_NO_MEMORY );   /* inform app of the near death experience. */

    if ( *Pid_p == stptiMemGet_Slot(FullSlotHandle)->Pid )
    {

        *SlotCount_p = STSYS_ReadRegDev16LE((void*)&MainInfo_p->PacketCount);
    	/*MAS 14/07/2003, Slot Count need not to be checked as it can wrap around and give false zero value. GNBvd 24241 */

        if ((STSYS_ReadRegDev16LE((void*)&MainInfo_p->SlotState) & TC_MAIN_INFO_SLOT_STATE_SCRAMBLED) == 0)
        {
            *ScrambleState_p = STPTI_SCRAMBLE_STATE_CLEAR;
        }
        else if ((STSYS_ReadRegDev16LE((void*)&MainInfo_p->SlotState) & TC_MAIN_INFO_SLOT_STATE_TRANSPORT_SCRAMBLED) == 0)
        {
            *ScrambleState_p = STPTI_SCRAMBLE_STATE_PES_SCRAMBLED;
        }
        else
        {
            *ScrambleState_p = STPTI_SCRAMBLE_STATE_TRANSPORT_SCRAMBLED;
        }
    }
    /* pid in TC is not == to pid in st20 slot struct.  This happens when we have pes & pcr on the same pid */
    else if (PidInSlotStructure != TC_INVALID_PID)
    {
        STPTI_Slot_t ExistingSlot = STPTI_NullHandle();
        FullHandle_t FullExistingSlotHandle;

        /* check for other slots collecting this pid */
        stpti_SlotQueryPid(FullSlotHandle, PidInSlotStructure, &ExistingSlot);

        if (ExistingSlot != STPTI_NullHandle())
        {
            FullExistingSlotHandle.word = ExistingSlot;
            RecurseDepth--; /* calling  us (stptiHAL_SlotState) again so decrement the danger counter */
            Error = stptiHAL_SlotState(FullExistingSlotHandle, SlotCount_p, ScrambleState_p, Pid_p, RecurseDepth);
        }
    }
    return Error;
}


/******************************************************************************
Function Name : stptiHAL_Term
  Description : stpti_TCTerm
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_Term(FullHandle_t DeviceHandle)
{
    ST_ErrorCode_t Error;
    Device_t   *Device_p = stptiMemGet_Device( DeviceHandle );
    Device_t   *OtherDevice_p;
    TCDevice_t *TC_Device = (TCDevice_t *)Device_p->TCDeviceAddress_p;
    int index;

    (void)TcCam_FilterFreeSession( DeviceHandle );

    (void)stptiHelper_SlotList_Free( DeviceHandle );

    Error = stptiHAL_ReleaseSession( DeviceHandle );
    STTBX_Print(("\nstptiHAL_Term: stptiHAL_ReleaseSession(0x%08x) = 0x%08x\n", DeviceHandle.word, Error ));

    /* uninstall old handler */
    STTBX_Print(("interrupt_disable...  "));
    if ( STOS_InterruptDisable( Device_p->InterruptNumber, Device_p->InterruptLevel ) == STOS_FAILURE )
    {
        STTBX_Print(("failed\n"));
    }
    else
    {
        STTBX_Print(("successful\n"));
    }

#if defined(ST_OSLINUX)
    local_irq_disable();
#endif

    if( Device_p->IRQHolder == TRUE )
    { /* Do we need to uninstall the irq handler */
        STTBX_Print(("interrupt_uninstall...  "));
        STTBX_Print(("Freeing irq %x\n",Device_p->InterruptNumber));
        
        if ( STOS_InterruptUninstall( Device_p->InterruptNumber, Device_p->InterruptLevel, (void *) Device_p->Handle.word ) == STOS_FAILURE )
        {
            STTBX_Print(("failed\n"));
            Error = ST_ERROR_INTERRUPT_UNINSTALL;
        }
        else
        {
            STTBX_Print(("successful\n"));
        }
    }
    
    /* if last session on a specific device then really remove all data for it */
    if ( stptiHAL_NumberOfSessions( DeviceHandle ) == 0 )
    {
        /* Not the current holder of the IRQ. */
        Device_p->IRQHolder = FALSE;
        
        STTBX_Print(("\nstptiHAL_Term: Freeing last session\n"));
        stptiHelper_TCTerm_FreePrivateData( DeviceHandle );

        /* no point in acting upon error returns from these two functions */
        (void)stptiHelper_SlotList_Term( DeviceHandle );
        (void)TcCam_Terminate( DeviceHandle );

        STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFFIFOEnable, 0 );
        STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 0 );

#if !defined(ST_5301)
        STOS_UnmapRegisters((void *)TC_Device, sizeof(TCDevice_t));
#endif
        STPTI_Driver.Physical_PTIs_Mapped--;
        
#if defined (STPTI_FRONTEND_HYBRID)
        if ( STPTI_Driver.Physical_PTIs_Mapped == 0 )
        {
            /* UnMap 7200 FrontEnd */
            STOS_UnmapRegisters((void *)STPTI_Driver.STFE_Base_Address, STFE_REGION_SIZE);
            STOS_UnmapRegisters((void *)STPTI_Driver.STFE_Internal_RAM_Base_Address, STFE_INTERNAL_RAM_SIZE);
            STOS_UnmapRegisters((void *)STPTI_Driver.TSGDMA_Base_Address, TSGDMA_REGION_SIZE);
            STPTI_Driver.STFE_Base_Address = 0;
            STPTI_Driver.STFE_Internal_RAM_Base_Address = 0;
            STPTI_Driver.TSGDMA_Base_Address = 0;
        }
#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */
                
#if defined(ARCHITECTURE_ST20)
        /* Needed for level interrupt modes on the ST20 */
        STTBX_Print(("re-enable the level interrupt...  "));
        if ( STOS_InterruptEnable( Device_p->InterruptNumber, Device_p->InterruptLevel ) == STOS_FAILURE)
        {
            STTBX_Print(("failed\n"));
        }
        else
        {
            STTBX_Print(("successful\n"));
        }
#endif

#if defined (ST_OSLINUX)
        local_irq_enable();
#endif
    }
    else
    {
        if( Device_p->IRQHolder == TRUE )
        { /* Only install a new handle if this device was the holder. */
            Device_p->IRQHolder = FALSE; /* No longer the IRQ holder */
            
            /* If not last session then we _could_ be holding the interrupt rights - if we were the very */
            /*    first session to be initalized.                                                         */
            /* Since this is not the last session lets go and find a 'DeviceHandle' that does not match ours */
            /*   and is on the same device, let that handle the interrupts for the remaining sessions. */

            for (index = 0; index < stpti_GetNumberOfExistingDevices(); index++) 
            {
                OtherDevice_p = stpti_GetDevice( index );

                if ( OtherDevice_p != NULL )    /* allocated */
                {
                    if ( OtherDevice_p != Device_p )    /* different sessions... */
                    {
                        if ( OtherDevice_p->TCDeviceAddress_p == Device_p->TCDeviceAddress_p )  /* but on the same PTI cell. */
                        {
                            STTBX_Print(("\nstptiHAL_Term: using '%s' for irq, ", OtherDevice_p->DeviceName ));

                            /* slap in the new one */
                            STTBX_Print(("install the interrupt for the other device...  "));

                            if( STOS_InterruptInstall( OtherDevice_p->InterruptNumber, OtherDevice_p->InterruptLevel,
                                                       stptiHAL_InterruptHandler, "PTI", (void *)STPTI_Driver.MemCtl.Handle_p[index].Hndl_u.word ) == STOS_FAILURE )
                            {
                                STTBX_Print(("failed\n"));
                            }
                            else
                            {
                                OtherDevice_p->IRQHolder = TRUE; /* This device is now the IRQ holder */
                                STTBX_Print(("successful\n"));
                            }

                            /* note: we pass the handle (cast to a void *) rather than a ptr to it as might nomally be expected */
                            STTBX_Print(("which is handle 0x%08x\n", STPTI_Driver.MemCtl.Handle_p[index].Hndl_u.word ));

                            STTBX_Print(("re-enable the interrupt...  "));
                            if ( STOS_InterruptEnable( OtherDevice_p->InterruptNumber, OtherDevice_p->InterruptLevel ) == STOS_FAILURE)
                            {
                                STTBX_Print(("failed\n"));
                            }
                            else
                            {
                                STTBX_Print(("successful\n"));
                            }
#if defined(ST_OSLINUX)
                            local_irq_enable();
#endif

                            /* sanity check */

                            assert( Device_p->InterruptLevel  == OtherDevice_p->InterruptLevel  );
                            assert( Device_p->InterruptNumber == OtherDevice_p->InterruptNumber );
                        
                            return( Error );
                        }

                    }
                }
            }

            Error = ST_ERROR_INTERRUPT_INSTALL; /* something went badly wrong */
        }
        else
        {
            STTBX_Print(("re-enable the interrupt...  "));
            if ( STOS_InterruptEnable( Device_p->InterruptNumber, Device_p->InterruptLevel ) == STOS_FAILURE)
            {
                STTBX_Print(("failed\n"));
            }
            else
            {
                STTBX_Print(("successful\n"));
            }

        }
#if defined(ST_OSLINUX)
        local_irq_enable();
#endif
    }
   
    return( Error );
}

/******************************************************************************
Function Name : stptiHAL_DescramblerDeallocate
  Description : stpti_TCTerminateDescrambler
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerDeallocate(FullHandle_t DescramblerHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 DescIdent = stptiMemGet_Descrambler(DescramblerHandle)->TC_DescIdent;


    stptiMemGet_Device(DescramblerHandle)->TCPrivateData.DescramblerHandles_p[DescIdent] = STPTI_NullHandle();

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_FilterDeallocateSection
  Description : stpti_TCTerminateSectionFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDeallocateSection(FullHandle_t DeviceHandle, U32 FilterIdent)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);


    PrivateData_p->SectionFilterHandles_p[FilterIdent] = STPTI_NullHandle();

    Error = TcCam_FilterDeallocate( DeviceHandle, FilterIdent );

    return Error;
}


/******************************************************************************
Function Name : stptiHAL_SlotDeallocate
  Description : stpti_TCTerminateSlot
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotDeallocate(FullHandle_t DeviceHandle, U32 SlotIdent)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );


    TcHal_LookupTableSetPidForSlot( TC_Params_p, SlotIdent, TC_INVALID_PID );

    stptiHelper_WaitPacketTime();

    PrivateData_p->SlotHandles_p[SlotIdent] = STPTI_NullHandle();
    PrivateData_p->PCRSlotHandles_p[SlotIdent] = STPTI_NullHandle();

    STSYS_WriteTCReg16LE((void*)&MainInfo_p->SlotState,          0);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->SlotMode,           0);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->DescramblerKeys_p,  TC_INVALID_LINK);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->DMACtrl_indices,    0xFFFF);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->StartCodeIndexing_p,0);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->SectionPesFilter_p, TC_INVALID_LINK);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->RemainingPESLength, 0);
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->PacketCount,        0);

    return Error;
}


/******************************************************************************
Function Name : stptiHAL_PcrToTimestamp
  Description : stpti_TCConvertPCRToTimestamp
   Parameters :
******************************************************************************/
void stptiHAL_PcrToTimestamp(STPTI_TimeStamp_t * TimeStamp, U8 *RawPCR)
{
    U32 LSW;
    U8  Bit32;


    LSW    = ((RawPCR[0] & 0x7f) << 25);
    LSW   |=  (RawPCR[1] << 17);
    LSW   |=  (RawPCR[2] << 9);
    LSW   |=  (RawPCR[3] << 1);
    LSW   |= ((RawPCR[4] & 0x80) >> 7);

    Bit32  = ((RawPCR[0] & 0x80) >> 7);

    TimeStamp->LSW = LSW;
    TimeStamp->Bit32 = Bit32;
}

/******************************************************************************
Function Name : stptiHAL_STCToTimestamp
  Description :     
   Parameters :
******************************************************************************/
void stptiHAL_STCToTimestamp(Device_t *Device_p,
                             U16 STC0,
                             U16 STC1,
                             U16 STC2, 
                             STPTI_TimeStamp_t *TimeStamp,
                             U16 *ArrivalTimeExtension_p )
{
    if (Device_p->PacketArrivalTimeSource == STPTI_ARRIVAL_TIME_SOURCE_TSMERGER)
    {
        /* TSMERGER/TSGDMA Tagged STC counter to be used */

        /* Extension part in lower 9 bits of word 2 */
        *ArrivalTimeExtension_p = STC2 & 0x1ff;

        /* 33 bit 90 KHz, first 7 Bits goes here */
        TimeStamp->LSW  = STC2 >> 9;

        /* get another 16 bits, 23 so far */
        TimeStamp->LSW  |= ((U32)STC1) << 7;

        /* another 9 goes here */
        TimeStamp->LSW  |= ((U32) STC0) << 23;

        /* Bit 33 goes last */
        TimeStamp->Bit32 = (STC0 >> 9) & 0x01;
    }
    else
    {
        /* PTI HARDWARE STC counter to be used */

        /* Extension part */
        *ArrivalTimeExtension_p = STC2 & 0x1ff;

        /* Bit 32  */
        TimeStamp->Bit32 = (STC0 >> 15) & 0x01;

        /* 33 bit 90 KHz, (31-17) MSbits here */
        TimeStamp->LSW  = ((U32) STC0) << 16;

        /* get another (16-1) bits, 23 so far */
        TimeStamp->LSW |= ((U32) STC1) << 1;

        /* 90 kHz 0th Lsbit here */
        TimeStamp->LSW  |= ((U32) STC2) >> 15;
   }
}



/******************************************************************************
Function Name : stptiHAL_SignalRenable
                stpti_SignalRenable

  Description : Signal if there is data in the buffer. This is useful if we
                have lost a signal for any reason.
   Parameters :
******************************************************************************/
void stptiHAL_SignalRenable(FullHandle_t FullBufferHandle)
{
    U16                   DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );


    /* Is signaling disabled and there is data in the buffer ? */

    if ((STSYS_ReadRegDev16LE((void*)&DMAConfig_p->SignalModeFlags) & TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE) &&
        (DMAConfig_p->DMAQWrite_p != DMAConfig_p->DMARead_p))
    {
        stptiHelper_SignalBuffer(FullBufferHandle, 0XBBBBBBBB);
    }
}

/******************************************************************************
Function Name : stptiHAL_SignalSetup
                STPTI_SignalAssociateBuffer

  Description : If a slot has been associated to this buffer set up signalling
                for the buffer according to the slot flags.
   Parameters :
******************************************************************************/
void stptiHAL_SignalSetup(FullHandle_t FullBufferHandle)
{
    U16                   DMAIdent       = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p    = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );
    Buffer_t             *BufferStruct_p = stptiMemGet_Buffer(FullBufferHandle);
    ST_ErrorCode_t        Error          = ST_NO_ERROR;


    /* if a slots has been associated with the buffer then set signals according to slot flags */
    if (BufferStruct_p->SlotListHandle.word != STPTI_NullHandle() )
    {
        FullHandle_t  FullSlotHandle;
        Slot_t       *SlotStruct_p;

        Error = stpti_GetFirstHandleFromList(BufferStruct_p->SlotListHandle, &FullSlotHandle);
        if (Error == ST_NO_ERROR)
        {
            SlotStruct_p   = stptiMemGet_Slot(FullSlotHandle);
         
            /*Reset SignalModeFlags as this could have been a previously used DMA and the flags may be in an undefined state */
            /* this clears the signalling type but leaves the TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE unchanged (so we don't miss signals) */
            STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK );

            if (SlotStruct_p->Flags.SignalOnEveryTransportPacket)
            {
                /* Force Quantisation unit to a single packet */
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS);
            }
            else
            {
                /* For Normal case, or SoftwareCDFifo with a signal assoc'd, used quantised signalling */
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION);
            }


#if defined( STPTI_SWCDFIFO_QUANTISED_PES )
            /* Allow winding back (to the last quantisation unit) on PES error */                
            STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_WINDBACK_ON_ERROR);
#else
            if(SlotStruct_p->Flags.SignalOnEveryTransportPacket)
            {
                /* SoftwareCDFifo means don't wind back on Error */
                STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_WINDBACK_ON_ERROR);
            }
            else
            {
                /* Allow winding back (to the last quantisation unit) on PES error */                
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_WINDBACK_ON_ERROR);
            }
#endif
            
            /* Add flag to indicate if Metadata is needed to be output for PES packets (note overridden by SWCDFIFO */
            if (SlotStruct_p->Flags.OutPesWithoutMetadata || SlotStruct_p->Flags.SignalOnEveryTransportPacket || SlotStruct_p->Flags.SoftwareCDFifo)
            {
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA);
            }
            else
            {
                STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_OUTPUT_WITHOUT_META_DATA);
            }

            /* Enable signalling */
            STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
            
        }

        /* Is signaling disabled and there is data in the buffer ? */
        if ((STSYS_ReadRegDev16LE((void*)&DMAConfig_p->SignalModeFlags) & TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE) &&
            (STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAQWrite_p) != STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMARead_p)))
        {
            stptiHelper_SignalBuffer(FullBufferHandle, 0XBBBBBBBB);
        }
    }
    else
    {
        /* As there is no slots associated to this buffer don't allow signalling */
        STSYS_SetTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
        STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK);
    }

}

/******************************************************************************
Function Name : stptiHAL_SignalUnsetup
                stpti_SignalDisassociateBuffer

  Description : Stop signals on this buffer
   Parameters :
******************************************************************************/
void stptiHAL_SignalUnsetup(FullHandle_t FullBufferHandle)
{
    Buffer_t             *BufferStruct_p = stptiMemGet_Buffer(FullBufferHandle);
    U32                   DMAIdent       = BufferStruct_p->TC_DMAIdent;

    if( DMAIdent != UNININITIALISED_VALUE )
    {
        TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData(FullBufferHandle);
        STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
        TCDMAConfig_t        *DMAConfig_p    = TcHal_GetTCDMAConfig( TC_Params_p, (U16) DMAIdent );

        STSYS_SetTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
        STSYS_ClearTCMask16LE ((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_MASK);
    }
}

/******************************************************************************
Function Name : stptiHAL_BufferUnLink
  Description : stpti_TCRemoveBufferDirectDMA
   Parameters :
         NOTE : moved from extended.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferUnLink(FullHandle_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(BufferHandle);
    TCDevice_t      *TC_Device_p   = (TCDevice_t *)stptiMemGet_Device(BufferHandle)->TCDeviceAddress_p;
    U32              DirectDma     = stptiMemGet_Buffer(BufferHandle)->DirectDma;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;


    switch (DirectDma)
    {
        case 0:
            Error = STPTI_ERROR_INVALID_CD_FIFO_ADDRESS;
            break;

        case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~0x2) & 0xf, TC_Params_p);
                PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                TC_Device_p->DMAEnable = (~0x2) & 0xf;
                PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~0x4) & 0xf, TC_Params_p);
                PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                TC_Device_p->DMAEnable = (~0x4) & 0xf;
                PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~0x8) & 0xf, TC_Params_p);
                PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                TC_Device_p->DMAEnable = (~0x8) & 0xf;
                PrivateData_p->TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        default:
            Error = STPTI_ERROR_INVALID_CD_FIFO_ADDRESS;
            break;
    }

    if (Error == ST_NO_ERROR)
    {
        /* Stop Slots causing the direct dma's write pointer being updated by setting the DirectDMA
           field in SlotMode to zero for all the slots feeding the buffer */

        /* While we are messing with the slots that feed the buffer, record the quantisation mode
           for a slot ( they will all be the same ) it will be needed so that the signalModeFlags
           of the DMA structure can be set-up */

        FullHandle_t SlotListHandle = stptiMemGet_Buffer(BufferHandle)->SlotListHandle;

        if (SlotListHandle.word != STPTI_NullHandle())
        {
            U32 slot;
            U32 NumberOfSlots = stptiMemGet_List(SlotListHandle)->MaxHandles;

            for (slot = 0; slot < NumberOfSlots; ++slot)
            {
                FullHandle_t SlotHandle;

                SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[slot];
                if (SlotHandle.word != STPTI_NullHandle())
                {
                    U32 SlotIdent = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
                    TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
                    STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_DMA_FIELD);
                }
            }
        }
    }

    return Error;
}


/******************************************************************************
Function Name : stptiHAL_BufferReadTSPacket
  Description : stpti_TCReadTSPacket
   Parameters :
         NOTE : moved here from extended.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferReadTSPacket(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                     U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p,
                                     STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U16                   DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    U32 Read_p      =  DMAConfig_p->DMARead_p;
    U32 Write_p     =  DMAConfig_p->DMAQWrite_p;
    U32 Top_p       =  (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_p;
    U32 Base_phys_p =  DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_phys_p;

    U32 PacketSize = DVB_TS_PACKET_LENGTH;                        /* Assume DVB packet */
    U32 Packets = (stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff;
    U32 BytesCopied;
    U32 SizeToCopy;
    U32 BytesInBuffer = (Write_p < Read_p)?(BufferSize + Write_p - Read_p):(Write_p - Read_p);

    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_phys_p);
    Write_p = MappedStart_p + (Write_p - Base_phys_p);
    Top_p = MappedStart_p + (Top_p - Base_phys_p);
    Base_p = MappedStart_p;

    assert(NULL != DataSize_p);


    BytesCopied = 0;
    SizeToCopy = Packets * PacketSize;

    /* Less than a MultiPacketSize */
    if (BytesInBuffer < SizeToCopy)
    {
        Packets = BytesInBuffer / PacketSize;
        SizeToCopy = Packets * PacketSize;          /* Round down to multiple of PacketSize */
    }

    if (SizeToCopy == 0)
    {
        Error = STPTI_ERROR_NO_PACKET;
    }
    else
    {
        /* (Re)Calculate Buffer Parameters based on Read_p */

        U32 ReadBufferIndex = Read_p - Base_p;

        /* To get here we must have a valid packet in the buffer, copy it to the user buffer(s)
         */

        U32 SrcSize0;
        U8 *Src0_p;
        U32 SrcSize1;
        U8 *Src1_p;

        /* Invalidate the Buffer Region of interest */
        stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, SizeToCopy );
        
        if (Read_p <= Write_p)  /* New code -- David */
        {
            SrcSize0 = MINIMUM (Write_p - Read_p, SizeToCopy);
            SrcSize1 = 0;
        }
        else
        {
            SrcSize0 = MINIMUM (Top_p - Read_p, SizeToCopy);
            SrcSize1 = MINIMUM (Write_p - Base_p, (SizeToCopy - SrcSize0));
        }

        Src0_p = &((U8 *) Base_p)[ReadBufferIndex];
        Src1_p = (SrcSize1 > 0) ? (U8 *) Base_p : NULL;

        BytesCopied += stptiHelper_CircToCircCopy (SrcSize0, /* Number useful bytes in PTIBuffer Part0 */
                                       Src0_p, /* Start of PTIBuffer Part0 */
                                       SrcSize1, /* Number useful bytes in PTIBuffer Part1 */
                                       Src1_p, /* Start of PTIBuffer Part1  */
                                       &DestinationSize0, /* Space in UserBuffer0 */
                                       &Destination0_p, /* Start of UserBuffer0 */
                                       &DestinationSize1, /* Space in UserBuffer1 */
                                       &Destination1_p, /* Start of UserBuffer1 */
                                       DmaOrMemcpy, /* Copy Method */
                                       FullBufferHandle);

        /* Advance Read_p to point to next packet */

        Read_p = (U32) & (((U8 *) Base_p)[(ReadBufferIndex + SizeToCopy) % BufferSize]); /* Modified code -- David */

        /* Update Read pointer in DMA structure */
        STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, (Read_p-Base_p+Base_phys_p) );

    }

    *DataSize_p = BytesCopied;

    /* Signal or Re-enable interrupts as appropriate */
    stptiHelper_SignalEnable(FullBufferHandle, (U16) Packets);

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_DescramblerAssociate
  Description : stpti_TCAssociateDescramblerWithSlot
   Parameters :
         NOTE : moved from extended.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerAssociate(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle)
{
    U32                     SlotIdent = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    U32                     DescIdent = stptiMemGet_Descrambler(DescramblerHandle)->TC_DescIdent;
    TCPrivateData_t    *PrivateData_p = stptiMemGet_PrivateData(DescramblerHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCKey_t         *DescramblerKey_p = TcHal_GetDescramblerKey ( TC_Params_p, DescIdent );

    TcHal_MainInfoAssociateDescramblerPtrWithSlot(TC_Params_p, SlotIdent, DescramblerKey_p );

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_EnableScramblingEvents
  Description : new
   Parameters :
         NOTE :
******************************************************************************/
ST_ErrorCode_t stptiHAL_EnableScramblingEvents(FullHandle_t FullSlotHandle)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullSlotHandle);
    U32                   SlotIdent     = stptiMemGet_Slot(FullSlotHandle)->TC_SlotIdent;
    Device_t             *Device_p        = stptiMemGet_Device(FullSlotHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    TCSessionInfo_t      *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1, (START_INDEXING));
    STSYS_SetTCMask16LE((void*)&MainInfo_p->IndexMask, (STATUS_FLAGS_SCRAMBLE_CHANGE>>16));
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DisableScramblingEvents
  Description : new
   Parameters :
         NOTE :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DisableScramblingEvents(FullHandle_t FullSlotHandle)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullSlotHandle);
    U32                   SlotIdent     = stptiMemGet_Slot(FullSlotHandle)->TC_SlotIdent;
    Device_t             *Device_p        = stptiMemGet_Device(FullSlotHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    TCSessionInfo_t      *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask1, (START_INDEXING));
    STSYS_ClearTCMask16LE((void*)&MainInfo_p->IndexMask, (STATUS_FLAGS_SCRAMBLE_CHANGE>>16));
    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_GetPresentationSTC
  Description : new
   Parameters :
         NOTE :
******************************************************************************/
ST_ErrorCode_t stptiHAL_GetPresentationSTC(FullHandle_t DeviceHandle, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp)
{
    TCDevice_t *TC_Device = (TCDevice_t *)stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;


    switch( Timer )
    {
        case STPTI_AUDIO_TIMER0:
            TimeStamp->Bit32 = TC_Device->PTIAudPTS_32;
            TimeStamp->LSW   = TC_Device->PTIAudPTS_31_0;
            break;

        case STPTI_VIDEO_TIMER0:
            TimeStamp->Bit32 = TC_Device->PTIVidPTS_32;
            TimeStamp->LSW   = TC_Device->PTIVidPTS_31_0;
            break;

        default :
            return( ST_ERROR_BAD_PARAMETER );
            /* break; */
    }

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_SlotQuery
  Description : stpti_TCSlotQuery
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotQuery(FullHandle_t SlotHandle, BOOL *PacketsSeen, BOOL *TransportScrambledPacketsSeen, BOOL *PESScrambledPacketsSeen)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32 			      SlotIdent   	= stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    TCMainInfo_t          *MainInfo_p 	= TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    
    BOOL ModeIsDVB = ( ( stptiMemGet_Device(SlotHandle)->TCCodes == STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB ) 
            );

    STPTI_CRITICAL_SECTION_BEGIN {

        *TransportScrambledPacketsSeen = FALSE;
        *PESScrambledPacketsSeen = FALSE;
        *PacketsSeen = FALSE;

        if ((STSYS_ReadRegDev16LE((void*)&MainInfo_p->SlotState)&TC_MAIN_INFO_SLOT_STATE_SCRAMBLED) == TC_MAIN_INFO_SLOT_STATE_SCRAMBLED)
        {
            STTBX_Print(("\n\nstptiHAL_DVBSlotQuery->We have a scrambled state in MainInfo\n\n"));
            if ((STSYS_ReadRegDev16LE((void*)&MainInfo_p->SlotState)&TC_MAIN_INFO_SLOT_STATE_TRANSPORT_SCRAMBLED) == TC_MAIN_INFO_SLOT_STATE_TRANSPORT_SCRAMBLED)
            {
                STTBX_Print(("\n\nstptiHAL_DVBSlotQuery->and it's transport scrambled\n\n"));
                *TransportScrambledPacketsSeen = TRUE;
            }
            else
            {
                if ( ModeIsDVB )
                {
                    STTBX_Print(("\n\nstptiHAL_DVBSlotQuery->and it's pes scrambled\n\n"));
                    *PESScrambledPacketsSeen = TRUE;
                }
            }
        }

        if (STSYS_ReadRegDev16LE((void*)&MainInfo_p->PacketCount) > 0)
        {
            *PacketsSeen = TRUE;
        }

    } STPTI_CRITICAL_SECTION_END;

    return( ST_NO_ERROR );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/* If the Architecture is PTI4 Lite, do not use the normal WriteTC macro, this fixes a Lockout in
   the HW GNBvd47215. 
   If the TC is running, and if the address range of the access indicates a DMA register, protect
   the access with a Mutex
   To minimise TC load, Mutex normally stays granted, and is only cleared when TC wants to poll DMA
   Host checks both before and after assertion of request to avoid race condition, and relinquishes
   REQUEST when a race condition occurs.
   Thus TC effectively gets priority over host, but must not proceed while REQUEST is high */

#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
void WriteDMAReg32( void *reg, U32 u32value, STPTI_TCParameters_t *TC_Params_p)
{
    U16             LocalCAMArbiterInhibit=0;
    TCGlobalInfo_t  *TC_Global_p    = (TCGlobalInfo_t *) TC_Params_p->TC_GlobalDataStart;

#if defined(ST_OSLINUX)
    preempt_disable();
#endif

    STPTI_CRITICAL_SECTION_BEGIN {

        if((STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_RUNNING))         
        {
            do
            {
                LocalCAMArbiterInhibit = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit);
                LocalCAMArbiterInhibit &= ~TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST;
                STSYS_WriteRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit,LocalCAMArbiterInhibit);    /* Back off - Clear the Mutex request */

                while(!(STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_GRANT));    /* Wait for TC to grant Mutex */

                LocalCAMArbiterInhibit |= TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST;
                STSYS_WriteRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit,LocalCAMArbiterInhibit);         /* Set the Mutex request */

            }while(!(STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_GRANT));
        }


        *(U32 *)reg = u32value;        /* Do the write */


        if((STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_RUNNING))         /* If the TC is running */
        {
            LocalCAMArbiterInhibit &= ~TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST;
            STSYS_WriteRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit,LocalCAMArbiterInhibit);  /* Clear the Mutex request */
        }

    } STPTI_CRITICAL_SECTION_END;

#if defined(ST_OSLINUX)
    preempt_enable();
#endif

}

U32 ReadDMAReg32( void *reg, STPTI_TCParameters_t *TC_Params_p)
{
    U16             LocalCAMArbiterInhibit=0;
    TCGlobalInfo_t  *TC_Global_p    = (TCGlobalInfo_t *) TC_Params_p->TC_GlobalDataStart;
    U32             u32value;

#if defined(ST_OSLINUX)
    preempt_disable();
#endif

    STPTI_CRITICAL_SECTION_BEGIN {

        if((STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_RUNNING))         
        {
            do
            {
                LocalCAMArbiterInhibit = STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit);
                LocalCAMArbiterInhibit &= ~TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST;
                STSYS_WriteRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit,LocalCAMArbiterInhibit);    /* Back off - Clear the Mutex request */

                while(!(STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_GRANT));    /* Wait for TC to grant Mutex */

                LocalCAMArbiterInhibit |= TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST;
                STSYS_WriteRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit,LocalCAMArbiterInhibit);         /* Set the Mutex request */

            }while(!(STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_GRANT));
        }

        u32value = *(U32 *)reg;        /* Do the read */

        if((STSYS_ReadRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterIdle) & TC_GLOBAL_MUTEX_FLAGS_TC_RUNNING))         /* If the TC is running */
        {
            LocalCAMArbiterInhibit &= ~TC_GLOBAL_MUTEX_FLAGS_HOST_REQUEST;
            STSYS_WriteRegDev16LE((void*)&TC_Global_p->GlobalCAMArbiterInhibit,LocalCAMArbiterInhibit);  /* Clear the Mutex request */
        }

    } STPTI_CRITICAL_SECTION_END;

#if defined(ST_OSLINUX)
    preempt_enable();
#endif

   return(u32value);

}


#endif





/* EOF  -------------------------------------------------------------------- */
