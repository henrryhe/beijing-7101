/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: carousel.c
 Description: see carousel API

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#include <string.h>

#include "stddefs.h"
#include "stdevice.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "memget.h"

#ifndef STPTI_CAROUSEL_SUPPORT
#error Incorrect build options!
#endif


#include "cam.h"
#include "tchal.h"

#include "pti4.h"


/******************************************************************************
Function Name : stptiHAL_CarouselGetSubstituteDataStart
  Description : new function.
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_CarouselGetSubstituteDataStart(FullHandle_t CarouselHandle, STPTI_DevicePtr_t *Control_p)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(CarouselHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    *Control_p = (STPTI_DevicePtr_t)TC_Params_p->TC_SubstituteDataStart;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_CarouselSetAllowOutput
  Description : stpti_TCSetCarouselOutput
   Parameters : No change for multiple slots per session - this acts on ALL sessions
******************************************************************************/
ST_ErrorCode_t stptiHAL_CarouselSetAllowOutput(FullHandle_t CarouselHandle)
{
    ST_ErrorCode_t          Error           = ST_NO_ERROR;    
    Device_t                *Device_p       = stptiMemGet_Device(CarouselHandle);
    TCPrivateData_t         *PrivateData_p  = stptiMemGet_PrivateData(CarouselHandle);
    STPTI_TCParameters_t    *TC_Params_p    = (STPTI_TCParameters_t *) & PrivateData_p->TC_Params;
       
    if (TC_Params_p->TC_NumberCarousels == 0)
    {
        Error = STPTI_ERROR_ALT_OUT_TYPE_NOT_SUPPORTED;
    }
    else
    {
        U32 slot;
        
        TCSessionInfo_t   *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ]; /*PJW is this device_p->session  ok or same for all inits on a device*/        
        
        {   /* Reset previous carousel setup on this session */            
            /*Reset UnmatchedMainInfo bit */
            STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionUnmatchedSlotMode, TC_MAIN_INFO_SLOT_MODE_SUBSTITUTE_STREAM );
            STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, STATUS_FLAGS_SUBSTITUTE_COMPLETE );

            /* Reset all matched maininfo carousel bits on this session */               
            for (slot = 0; slot < TC_Params_p->TC_NumberSlots; ++slot)
            {
                FullHandle_t SlotHandle;

                SlotHandle.word = PrivateData_p->SlotHandles_p[slot];
                if (SlotHandle.word != STPTI_NullHandle())
                {
                    TCMainInfo_t *MainInfo_p = &((TCMainInfo_t *) TC_Params_p->TC_MainInfoStart)[slot];
                    STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_SUBSTITUTE_STREAM );
                }
            }
        }        
        /*Set SUBSTITUTE_STREAM bit in UnmatchedMainInfo*/
        if ( stptiMemGet_Carousel(CarouselHandle)->OutputAllowed == STPTI_ALLOW_OUTPUT_UNMATCHED_PIDS )
        {
            STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionUnmatchedSlotMode, (TC_MAIN_INFO_SLOT_MODE_SUBSTITUTE_STREAM) );
        } 
        
        /*set SUBSTITUTE_STREAM bit in required matched slots*/
        else if ( stptiMemGet_Carousel(CarouselHandle)->OutputAllowed == STPTI_ALLOW_OUTPUT_SELECTED_SLOTS)
        {            
            for (slot = 0; slot < TC_Params_p->TC_NumberSlots; ++slot)
            {
                FullHandle_t SlotHandle;

                SlotHandle.word = PrivateData_p->SlotHandles_p[slot];
                if (SlotHandle.word != STPTI_NullHandle())
                {
                    if ( stptiMemGet_Slot(SlotHandle)->Flags.AlternateOutputInjectCarouselPacket)
                    {
			TCMainInfo_t *MainInfo_p = &((TCMainInfo_t *) TC_Params_p->TC_MainInfoStart)[slot];
                        STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_SUBSTITUTE_STREAM); 
                    }
                }
            }
        }
        else
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        } 
    }    
    return Error;
}


/******************************************************************************
Function Name : stptiHAL_CarouselLinkToBuffer
  Description : stpti_TCSetUpCarouselDMA
   Parameters : No change for multiple slots per session - this acts on ALL sessions
******************************************************************************/
ST_ErrorCode_t stptiHAL_CarouselLinkToBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle, BOOL DMAExists)
{
    ST_ErrorCode_t       Error          = ST_NO_ERROR;
    Device_t             *Device_p      = stptiMemGet_Device(FullCarouselHandle);
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullCarouselHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) & PrivateData_p->TC_Params;
    
    
    if ( stptiMemGet_Carousel(FullCarouselHandle)->OutputAllowed == STPTI_ALLOW_OUTPUT_SELECTED_SLOTS)
    {
        U32 Slot;

        /* Scan through all slots on this device, looking for those selected for Carousel Output */
        
        for (Slot = 0; Slot < TC_Params_p->TC_NumberSlots; ++Slot)
        {
            FullHandle_t FullSlotHandle;

            FullSlotHandle.word = PrivateData_p->SlotHandles_p[Slot];
            if (FullSlotHandle.word != STPTI_NullHandle())
            {
                if ( stptiMemGet_Slot(FullSlotHandle)->Flags.AlternateOutputInjectCarouselPacket)
                {
                    U32 dma;

                    /* When found either use the specified dma structure of find a free one to use */

                    if (DMAExists)
                    {
                        dma = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
                        TcHal_MainInfoAssociateDmaWithSlot(TC_Params_p,Slot,dma);
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
                            TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[dma];
                            U32 base = (U32) TRANSLATE_PHYS_ADD(stptiMemGet_Buffer(FullBufferHandle)->Start_p);
                            U32 top = (U32) (base + stptiMemGet_Buffer(FullBufferHandle)->ActualSize);

                            DMAConfig_p->DMABase_p = base;
                            DMAConfig_p->DMATop_p = (top - 1) & ~0xf;
                            DMAConfig_p->DMARead_p = base;
                            DMAConfig_p->DMAWrite_p = base;
                            DMAConfig_p->DMAQWrite_p = base;
                            DMAConfig_p->BufferPacketCount = 0;
                            STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, (TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS | TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION) );
                            STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, (TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS));
                            STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, (stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff);
                            TcHal_MainInfoAssociateDmaWithSlot(TC_Params_p,Slot,dma);
                            stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent = dma;
                            PrivateData_p->BufferHandles_p[dma] = FullBufferHandle.word;
                            DMAExists = TRUE;
                        }
                        else
                        {
                            Error = STPTI_ERROR_NO_FREE_DMAS;
                        }
                    }
                }
            }
        }
    }
    else if ( stptiMemGet_Carousel(FullCarouselHandle)->OutputAllowed == STPTI_ALLOW_OUTPUT_UNMATCHED_PIDS)
    {
        TCSessionInfo_t   *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];              /*PJW is this device_p->session  ok or same for all inits on a device*/        
        U32 Dma;

        /* set up dma in unmatched SessionInfo */
        if (DMAExists)
        {
            TCDMAConfig_t *DMAConfig_p;

            Dma = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
            DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[Dma];
            STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionUnmatchedDMACntrl_p, (U32)((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart + Dma));
        }
        else
        {

            for (Dma = 0; Dma < TC_Params_p->TC_NumberDMAs; ++Dma)
            {
                if (PrivateData_p->BufferHandles_p[Dma] == STPTI_NullHandle())
                    break;
            }

            if (Dma < TC_Params_p->TC_NumberDMAs)
            {
                TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[Dma];
                U32 base = (U32) TRANSLATE_PHYS_ADD(stptiMemGet_Buffer(FullBufferHandle)->Start_p);
                U32 top  = (U32) base + stptiMemGet_Buffer(FullBufferHandle)->ActualSize;
                U32 RelativeDMAConfig_p=0;
                
                DMAConfig_p->DMABase_p = base;
                DMAConfig_p->DMATop_p = (top - 1) & ~0xf;
                DMAConfig_p->DMARead_p = base;
                DMAConfig_p->DMAWrite_p = base;
                DMAConfig_p->DMAQWrite_p = base;
                DMAConfig_p->BufferPacketCount = 0;

                STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, (TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS | TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION) );
                STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, (TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS));
                STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, (stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff);
                RelativeDMAConfig_p = (U32)((U8 *) DMAConfig_p - (U8 *) TC_Params_p->TC_DataStart + (U8 *) 0x8000);
                STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionUnmatchedDMACntrl_p,  (U32)RelativeDMAConfig_p );
                stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent = Dma;
                PrivateData_p->BufferHandles_p[Dma] = FullBufferHandle.word;
            }
            else
            {
                Error = STPTI_ERROR_NO_FREE_DMAS;
            }
        }
    }
    else
    {
        Error = STPTI_ERROR_INVALID_ALLOW_OUTPUT_TYPE;
    }
    return Error;
}


/* EOF  -------------------------------------------------------------------- */
