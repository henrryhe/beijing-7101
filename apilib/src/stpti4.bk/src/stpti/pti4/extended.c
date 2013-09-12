/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005, 2006.  All rights reserved.

   File Name: extended.c
 Description: Provides interface to PTI transport controller

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT  
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #endif !defined(ST_OSLINUX)  */

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX)  */
#include "stpti.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"
#if defined (STPTI_FRONTEND_HYBRID)
#include "stfe.h"
#endif

#if !defined(ST_OSLINUX)
#define ioremap( _address_, _size_ )       ((void *)_address_)
#define iounmap( _address_ )
#endif /* #endif !defined(ST_OSLINUX)  */
/* Functions --------------------------------------------------------------- */


/* session.c */
extern TCSessionInfo_t *stptiHAL_GetSessionFromFullHandle(FullHandle_t DeviceHandle);


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_SetStreamID
  Description : Changes the streamID on a PTI session.
   Parameters : Device to reconfigure, and the new streamID to change to.
******************************************************************************/

ST_ErrorCode_t stptiHAL_SetStreamID(FullHandle_t DeviceHandle, STPTI_StreamID_t StreamID)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   STCSource      = 0;

    /* Look for a session already using the specified StreamID */
#if !defined (STPTI_FRONTEND_HYBRID)
    TCSessionInfo_t      *ExistingSession_p = stptiHAL_GetSessionPtrForStreamId(DeviceHandle, StreamID);
#endif
    TCSessionInfo_t      *TCSessionInfo_p;
    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];
   /*Update the streamId, but retain the value of STC source.  */

    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];

#if !defined (STPTI_FRONTEND_HYBRID)
    if (( StreamID == STPTI_STREAM_ID_NONE ) ||
        ( ExistingSession_p == NULL ))
#endif
    {
        /* StreamId is available */
        /* MSBit contains the value of STC source. Clear StreamID leaving STC source */
        STCSource = (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionTSmergerTag) & (~SESSION_MASK_STC_SOURCE));
        STSYS_WriteTCReg16LE((void*)&TCSessionInfo_p->SessionTSmergerTag, ((StreamID & SESSION_MASK_STC_SOURCE) | STCSource));
    }
#if !defined (STPTI_FRONTEND_HYBRID)
    else if ( TCSessionInfo_p == ExistingSession_p )
    {
        /* StreamId already in use by intended session!  No need to do anything */
    }
    else
    {
        /* StreamId already in use by other session */
        Error = ST_ERROR_BAD_PARAMETER;
    }
#endif
    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : stptiHAL_SetDiscardParams
  Description : Changes the DiscardSync Period on a session.
   Parameters : Device to reconfigure, The discard period
******************************************************************************/

ST_ErrorCode_t stptiHAL_SetDiscardParams(FullHandle_t DeviceHandle, U8 NumberOfDiscardBytes)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    TCSessionInfo_t      *TCSessionInfo_p;
    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];
   /*Update the streamId, but retain the value of STC source.  */

    return Error;
}



#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_SlotSetCCControl
  Description : stpti_TCSlotSetCCControl
   Parameters : SetOnOff when TRUE CC checking will be disabled,
                         when FALSE CC checking will be enabled.
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotSetCCControl(FullHandle_t FullSlotHandle, BOOL SetOnOff)
{
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(FullSlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   SlotIdent     = stptiMemGet_Slot(FullSlotHandle)->TC_SlotIdent;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );


    if ( SetOnOff )
    {
        STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_DISABLE_CC_CHECK);
    }
    else
    {
        STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_DISABLE_CC_CHECK);
    }

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_SlotSetCorruptionParams
  Description :
   Parameters :
******************************************************************************/
void stptiHAL_SlotSetCorruptionParams( FullHandle_t FullSlotHandle, U8 Offset, U8 Value )
{
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(FullSlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   SlotIdent     = stptiMemGet_Slot(FullSlotHandle)->TC_SlotIdent;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );

    /* Shift two bytes into 16 bit tc word */
    STSYS_WriteTCReg16LE((void*)&MainInfo_p->RawCorruptionParams, ((Value << 8) | Offset));

}


/******************************************************************************
Function Name : stptiHAL_SlotPacketCount
  Description : stpti_TCGetSlotPacketCount
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotPacketCount( FullHandle_t SlotHandle, U16 *Count_p)
{
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   SlotIdent     = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    TCMainInfo_t         *MainInfo_p    = TcHal_GetMainInfo( TC_Params_p, SlotIdent );

    *Count_p = STSYS_ReadRegDev16LE((void*)&MainInfo_p->PacketCount);

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_AlternateOutputSetDefaultAction
  Description : stpti_TCSetDefaultAlternateOutputAction
   Parameters : No change for multiple slots per session - this acts on ALL sessions
******************************************************************************/
ST_ErrorCode_t stptiHAL_AlternateOutputSetDefaultAction(FullHandle_t SessionHandle, STPTI_AlternateOutputType_t Method)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 slot;
    U16 DefaultSlotMode = 0;
    FullHandle_t SlotHandle;

    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(SessionHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCGlobalInfo_t       *TC_Global_p   = TcHal_GetGlobalInfo( TC_Params_p );
    TCMainInfo_t         *MainInfo_p;


    if (Method == STPTI_ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS)
        DefaultSlotMode |= TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_CLEAR;

    for (slot = 0; slot < TC_Params_p->TC_NumberSlots; ++slot)
    {
        SlotHandle.word = PrivateData_p->SlotHandles_p[slot];

        if (SlotHandle.word != STPTI_NullHandle())
        {
            if ( stptiMemGet_Slot(SlotHandle)->UseDefaultAltOut )
            {
                MainInfo_p = TcHal_GetMainInfo( TC_Params_p, slot );
                STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_FIELD);
                STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, DefaultSlotMode);
            }
        }
    }

    /* Set Alternate Output Flag bit */
    STSYS_ClearTCMask16LE((void*)&TC_Global_p->GlobalModeFlags, TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_FIELD);
    STSYS_SetTCMask16LE((void*)&TC_Global_p->GlobalModeFlags, DefaultSlotMode);

    return( Error );
}


/******************************************************************************
Function Name : stptiHAL_BufferExtractTSHeaderData
  Description : stpti_TCExtractTSHeaderData
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferExtractTSHeaderData(FullHandle_t FullBufferHandle, U32 *TSHeader_p)
{
    U16              DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p =  TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    U32 Read_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMARead_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Write_p = TRANSLATE_LOG_ADD(DMAConfig_p->DMAQWrite_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Top_p   = TRANSLATE_LOG_ADD((DMAConfig_p->DMATop_p & ~0xf) + 16, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Base_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMABase_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);

    U32 BufferSize = Top_p - Base_p;
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_p);
    U32 ReadBufferIndex = Read_p - Base_p;
    U32 Header;

#if defined(ST_OSLINUX)
    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);
    Write_p = MappedStart_p + (Write_p - Base_p);
    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
#endif

    *TSHeader_p = 0;

    if ( (Read_p == Write_p) || (BytesInBuffer < 4) )
        return( STPTI_ERROR_NO_PACKET );

    Header  = (((U8 *) Base_p)[ReadBufferIndex]) << 24;
    Header |= (((U8 *) Base_p)[(ReadBufferIndex + 1) % BufferSize]) << 16;
    Header |= (((U8 *) Base_p)[(ReadBufferIndex + 2) % BufferSize]) << 8;
    Header |= (((U8 *) Base_p)[(ReadBufferIndex + 3) % BufferSize]);

    *TSHeader_p = Header;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_BufferRead
  Description : stpti_TCReadBuffer
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferRead(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                   U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    U16              TC_DMAIdent   = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p =  TcHal_GetTCDMAConfig( TC_Params_p, TC_DMAIdent );

    U32 Read_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMARead_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Write_p = TRANSLATE_LOG_ADD(DMAConfig_p->DMAQWrite_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Top_p   = TRANSLATE_LOG_ADD((DMAConfig_p->DMATop_p & ~0xf) + 16, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Base_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMABase_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);

    U32 BufferSize = Top_p - Base_p;
    U32 ReadBufferIndex = Read_p - Base_p;
    U32 BytesBeforeWrap = Top_p - Read_p;
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_p);

    U32 BytesCopied = 0;

    U32 SizeToCopy;
    U32 SrcSize0;
    U8 *Src0_p;
    U32 SrcSize1;
    U8 *Src1_p;

#if defined(ST_OSLINUX)
    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);

    Write_p = MappedStart_p + (Write_p - Base_p);

    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
#endif

    /* PTI DMA does not work so use memcpy */
    if (DmaOrMemcpy == STPTI_COPY_TRANSFER_BY_DMA)
        DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;

    SizeToCopy = BytesInBuffer;

    SrcSize0 = MINIMUM(BytesBeforeWrap, SizeToCopy);
    Src0_p = &((U8 *) Base_p)[ReadBufferIndex];
    SrcSize1 = SizeToCopy - SrcSize0;
    Src1_p = (SrcSize1 > 0) ? (U8 *) Base_p : NULL;

    BytesCopied += stptiHelper_CircToCircCopy(SrcSize0,      /* Number useful bytes in PTIBuffer Part0 */
                                  Src0_p,        /* Start of PTIBuffer Part0 */
                                  SrcSize1,      /* Number useful bytes in PTIBuffer Part1 */
                                  Src1_p,        /* Start of PTIBuffer Part1  */
                                  &DestinationSize0, /* Space in UserBuffer0 */
                                  &Destination0_p, /* Start of UserBuffer0 */
                                  &DestinationSize1, /* Space in UserBuffer1 */
                                  &Destination1_p, /* Start of UserBuffer1 */
                                  DmaOrMemcpy, /* Copy Method */
                                  FullBufferHandle);

    /* Advance Read_p to point to next packet */

    Read_p = (U32) &(((U8 *) Base_p)[(ReadBufferIndex + BytesInBuffer) % BufferSize]);

    *DataSize_p = BytesCopied;

#if defined(ST_OSLINUX)
    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, virt_to_bus((void*)Read_p) );
#else
    DMAConfig_p->DMARead_p = (U32) TRANSLATE_PHYS_ADD(Read_p);    /* Update Read pointer in DMA structure */
#endif /* #if defined(ST_OSLINUX)  */

    /* Signal or Re-enable interrupts as appropriate */
    stptiHelper_SignalEnableIfDataWaiting( FullBufferHandle, (stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff );

    return ( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_BufferExtractData
  Description : stpti_TCExtractData
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferExtractData(FullHandle_t FullBufferHandle, U32 Offset, U32 NumBytesToExtract,
                                     U8 * Destination0_p, U32 DestinationSize0, U8 * Destination1_p,
                                     U32 DestinationSize1, U32 * DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U16              DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData (FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p =  TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );


    U32 Read_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMARead_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Write_p = TRANSLATE_LOG_ADD(DMAConfig_p->DMAQWrite_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Top_p   = TRANSLATE_LOG_ADD((DMAConfig_p->DMATop_p & ~0xf) + 16, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Base_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMABase_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);

    U32 BufferSize = Top_p - Base_p;
    U32 ReadBufferIndex = Read_p - Base_p;
    U32 BytesBeforeWrap = Top_p - Read_p;

    U32 SizeToCopy = 0; /* Avoids warning. Stieglitzp 140105. */
    U32 SrcSize0;
    U8 *Src0_p;
    U32 SrcSize1;
    U8 *Src1_p;

    U32 MetaDataSize = TcCam_GetMetadataSizeInBytes();

#if defined(ST_OSLINUX)
    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);
    Write_p = MappedStart_p + (Write_p - Base_p);
    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
#endif

    *DataSize_p = 0;

    switch ( stptiMemGet_Buffer(FullBufferHandle)->Type )
    {
        case STPTI_SLOT_TYPE_SECTION:
        {
            U32 SectionLength;

            /* Extract Section length */

            SectionLength  = ((((U8 *) Base_p)[(ReadBufferIndex + 1 +MetaDataSize) % BufferSize] & 0x0f) << 8);
            SectionLength +=   ((U8 *) Base_p)[(ReadBufferIndex + 2 +MetaDataSize) % BufferSize];
            SectionLength += 3;

            if (Offset >= SectionLength)
            {
                Error = STPTI_ERROR_OFFSET_EXCEEDS_PACKET_SIZE;
            }
            else
            {
                SizeToCopy = MINIMUM (SectionLength - Offset, NumBytesToExtract);
            }

            Offset += MetaDataSize;  /* Skip the filter info */
            break;
        }

        case STPTI_SLOT_TYPE_PES:
        {
            U32 PesPacketLength = 0;

            /* Extract ( or work out ) PES packet length */

            if (   (((U8 *) Base_p)[(ReadBufferIndex + 0) % BufferSize] == 0)
                && (((U8 *) Base_p)[(ReadBufferIndex + 1) % BufferSize] == 0)
                && (((U8 *) Base_p)[(ReadBufferIndex + 2) % BufferSize] == 1) )
            {

                /* Extract ( or work out ) PES packet length */

                PesPacketLength =
                    (((U8 *) Base_p)[(ReadBufferIndex + 4) % BufferSize] << 8) |
                    (((U8 *) Base_p)[(ReadBufferIndex + 5) % BufferSize]);

                if (PesPacketLength == 0)
                {
                    U32 End_p = Write_p;
                    U32 EndIndex = End_p - Base_p;

                    /* We have a non-determiniastic packet calculate size using the post-payload metadata */

                    while (End_p != Read_p)
                    {
                        EndIndex = End_p - Base_p;

                        if (EndIndex < BufferSize)
                        {
                            End_p =
                                ((((U8 *) Base_p)[(EndIndex - 1) % BufferSize] << 24) |
                                 (((U8 *) Base_p)[(EndIndex - 2) % BufferSize] << 16) |
                                 (((U8 *) Base_p)[(EndIndex - 3) % BufferSize] << 8) |
                                 (((U8 *) Base_p)[(EndIndex - 4) % BufferSize]));


                        }
                        else
                        {
                            Error = STPTI_ERROR_CORRUPT_DATA_IN_BUFFER;
                            break;
                        }

                    }   /* while */

                    if (Error == ST_NO_ERROR)
                    {
                        EndIndex -= 4;

                        if (EndIndex < ReadBufferIndex)
                        {
                            EndIndex += BufferSize;
                        }

                        PesPacketLength = EndIndex - ReadBufferIndex;
                    }
                }
                else
                {
                    /* Deterministic PES */
                    /* Remember to extract the PES header as well */
                    PesPacketLength += 6;
                }
            }

            if (Error == ST_NO_ERROR)
            {
                if (Offset >= PesPacketLength)
                {
                    Error = STPTI_ERROR_OFFSET_EXCEEDS_PACKET_SIZE;
                }
                else
                {
                    SizeToCopy = MINIMUM (PesPacketLength - Offset, NumBytesToExtract);
                }
            }
            break;
        }
        case STPTI_SLOT_TYPE_RAW:
        {
            U32 TSPacketLength;

            {
                TSPacketLength = 188; /* DVB Packet */
            }

            if (Offset >= TSPacketLength)
            {
                Error = STPTI_ERROR_OFFSET_EXCEEDS_PACKET_SIZE;
            }
            else
            {
                SizeToCopy = MINIMUM (TSPacketLength - Offset, NumBytesToExtract);
            }
            break;

        }
/*
    All of the remaining cases fall through to default:

        case STPTI_SLOT_TYPE_NULL:
        case STPTI_SLOT_TYPE_EMM:
        case STPTI_SLOT_TYPE_ECM:
#ifdef 0
        case STPTI_SLOT_TYPE_VIDEO_ES:
        case STPTI_SLOT_TYPE_AUDIO_ES:
        case STPTI_SLOT_TYPE_APG:
        case STPTI_SLOT_TYPE_CAP:
        case STPTI_SLOT_TYPE_DAP:
        case STPTI_SLOT_TYPE_MPT:
        case STPTI_SLOT_TYPE_PIP:
#endif
        case STPTI_SLOT_TYPE_PCR: */

        default:
            Error = STPTI_ERROR_FUNCTION_NOT_SUPPORTED;
            break;
    }

    if (Error != ST_NO_ERROR)
    {
        *DataSize_p = 0;    /* Just return the error */
    }
    else
    {
        if (BytesBeforeWrap > Offset)
        {
            Src0_p = &((U8 *) Base_p)[ReadBufferIndex + Offset];
            SrcSize0 = MINIMUM (BytesBeforeWrap - Offset, SizeToCopy);
            SrcSize1 = SizeToCopy - SrcSize0;
            Src1_p = (SrcSize1 > 0) ? (U8 *) Base_p : NULL;
        }
        else
        {
            Src0_p = &((U8 *) Base_p)[Offset - BytesBeforeWrap];
            SrcSize0 = SizeToCopy;
            SrcSize1 = 0;
            Src1_p = NULL;
        }

        *DataSize_p = stptiHelper_CircToCircCopy (SrcSize0,     /* Number useful bytes in PTIBuffer Part0 */
                                      Src0_p,                   /* Start of PTIBuffer Part0 */
                                      SrcSize1,                 /* Number useful bytes in PTIBuffer Part1 */
                                      Src1_p,                   /* Start of PTIBuffer Part1  */
                                      &DestinationSize0,        /* Space in UserBuffer0 */
                                      &Destination0_p,          /* Start of UserBuffer0 */
                                      &DestinationSize1,        /* Space in UserBuffer1 */
                                      &Destination1_p,          /* Start of UserBuffer1 */
                                      DmaOrMemcpy,              /* Copy Method */
                                      FullBufferHandle);

    }

    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : stptiHAL_BufferTestForData
  Description : stpti_TCGetBytesInBuffer
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferTestForData(FullHandle_t FullBufferHandle, U32 *Count_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 BytesInBuffer = 0;
    /* To avoid warning */
    U32 Read_p=0, Write_p=0, Top_p=0, Base_p=0;
    U16                   DirectDma     = stptiMemGet_Buffer(FullBufferHandle)->DirectDma;
    U16                   DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
    TCDevice_t           *TC_Device_p   = (TCDevice_t *)stptiMemGet_Device(FullBufferHandle)->TCDeviceAddress_p;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p =  TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    switch (DirectDma)
    {
        case 0:
            Read_p  =  DMAConfig_p->DMARead_p;
            Write_p =  DMAConfig_p->DMAQWrite_p;
            Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
            Base_p  =  DMAConfig_p->DMABase_p;
            break;

        case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA1Read, TC_Params_p);
                Write_p =   ReadDMAReg32((void *)&TC_Device_p->DMA1Write, TC_Params_p);
                Top_p   =   ReadDMAReg32((void *)&TC_Device_p->DMA1Top, TC_Params_p);
                Base_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA1Base, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  = TC_Device_p->DMA1Read;
                Write_p = TC_Device_p->DMA1Write;
                Top_p   = TC_Device_p->DMA1Top;
                Base_p  = TC_Device_p->DMA1Base;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA2Read, TC_Params_p);
                Write_p =   ReadDMAReg32((void *)&TC_Device_p->DMA2Write, TC_Params_p);
                Top_p   =   ReadDMAReg32((void *)&TC_Device_p->DMA2Top, TC_Params_p);
                Base_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA2Base, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  = TC_Device_p->DMA2Read;
                Write_p = TC_Device_p->DMA2Write;
                Top_p   = TC_Device_p->DMA2Top;
                Base_p  = TC_Device_p->DMA2Base;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA3Read, TC_Params_p);
                Write_p =   ReadDMAReg32((void *)&TC_Device_p->DMA3Write, TC_Params_p);
                Top_p   =   ReadDMAReg32((void *)&TC_Device_p->DMA3Top, TC_Params_p);
                Base_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA3Base, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  = TC_Device_p->DMA3Read;
                Write_p = TC_Device_p->DMA3Write;
                Top_p   = TC_Device_p->DMA3Top;
                Base_p  = TC_Device_p->DMA3Base;
            } STPTI_CRITICAL_SECTION_END;
#endif            
            break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

    BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_p);

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("stptiHAL_BufferTestForData: DirectDma: %d\n", DirectDma));
    STTBX_Print(("           Base_p: 0x%08x\n", Base_p   ));
    STTBX_Print(("         DMATop_p: 0x%08x\n", Top_p    ));
    STTBX_Print(("       DMAWrite_p: 0x%08x\n", Write_p  ));
    STTBX_Print(("        DMARead_p: 0x%08x\n", Read_p   ));
    STTBX_Print(("    BytesInBuffer: %d\n", BytesInBuffer   ));
#endif

    *Count_p = BytesInBuffer;

    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_FilterAllocatePES
  Description : stpti_TCGetFreePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterAllocatePES(FullHandle_t DeviceHandle, U32 *FilterIdent)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_FilterInitialisePES
  Description : stpti_TCInitialisePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterInitialisePES(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_FilterAssociatePES
  Description : stpti_TCAssociatePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterAssociatePES(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}



/******************************************************************************
Function Name : stptiHAL_FilterDisassociatePES
  Description : stpti_TCDisassociatePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDisassociatePES(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}

/******************************************************************************
Function Name : stptiHAL_FilterAllocatePESStreamId
  Description : stpti_TCGetFreePESStreamIdFilter
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_FilterAllocatePESStreamId(FullHandle_t DeviceHandle, U32 *FilterIdent)
{
    Device_t              	*Device        = stptiMemGet_Device(DeviceHandle);
    TCPrivateData_t       	*PrivateData_p = &Device->TCPrivateData;
    STPTI_TCParameters_t  	*TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    volatile STPTI_Filter_t *FilterHandle_p;
    TCSessionInfo_t       	*TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device->Session ];

    FullHandle_t Handle;
    U32 Index, Filter, MinAddr, MaxAddr;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("\nstptiHAL_FilterAllocatePESStreamId()\n"));
#endif
    
    FilterHandle_p = PrivateData_p->PesStreamIdFilterHandles_p;

    MinAddr = STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionPIDFilterStartAddr );
    MaxAddr = MinAddr + (STSYS_ReadRegDev16LE( (void*)&TCSessionInfo_p->SessionPIDFilterLength )*2 )- 1;

    assert( MinAddr != 0 ); /* if  MinAddr == 0 then its an internal error (slotlist.c:Init) */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("Scanning from 0x%x to 0x%x \n", MinAddr, MaxAddr));
#endif
    
    for (Filter = MinAddr; Filter <= MaxAddr; Filter+=2)
    {
        Index = (Filter - 0x8000) >> 1;   /* count words */

        Handle.word = FilterHandle_p[Index];

        assert( Index < TC_Params_p->TC_NumberSlots );

        if (Handle.word == STPTI_NullHandle())
        {
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            STTBX_Print(("Found free handle at index %d for PesStreamIdFilter 0x%x\n", Index, Filter));
#endif
            *FilterIdent = Index;
            return( ST_NO_ERROR );
        }
    }

    return( STPTI_ERROR_NO_FREE_FILTERS );
}

/******************************************************************************
Function Name : stptiHAL_FilterInitialisePESStreamId
  Description : stpti_TCInitialisePESStreamIdFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterInitialisePESStreamId(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle)
{
    stptiMemGet_Device(DeviceHandle)->TCPrivateData.PesStreamIdFilterHandles_p[FilterIdent] = FilterHandle;

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_FilterAssociatePESStreamId
  Description : stpti_TCAssociatePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterAssociatePESStreamId(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    ST_ErrorCode_t 		  Error 			= ST_NO_ERROR;
    TCPrivateData_t       *PrivateData_p 	=  stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t  *TC_Params_p   	= (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   SlotIdent     	= stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    TCMainInfo_t          *MainInfo_p    	= TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    FullHandle_t		  FilterListHandle	= ( stptiMemGet_Slot( SlotHandle ))->FilterListHandle;

    if ( FilterListHandle.word == STPTI_NullHandle())
    {
        STSYS_WriteRegDev16LE((void*)&MainInfo_p->SectionPesFilter_p,
                                (U32) (( stptiMemGet_Filter( FilterHandle ))->PESStreamIdFilterData));
        if ( ((stptiMemGet_Filter( FilterHandle ))->Enabled) == TRUE )
            STSYS_SetTCMask16LE((void*)&MainInfo_p->SectionPesFilter_p, TC_MAIN_INFO_PES_STREAM_ID_FILTER_ENABLED );
    }
    else
    {
        /* A PES Filter is already associated with this slot */

        Error = STPTI_ERROR_FILTER_ALREADY_ASSOCIATED;
    }

    return( Error );
}

/******************************************************************************
Function Name : stptiHAL_FilterDisassociatePESStreamId
  Description : stpti_TCDisassociatePESStreamIdFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDisassociatePESStreamId(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    TCPrivateData_t       *PrivateData_p 	=  stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t  *TC_Params_p   	= (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   SlotIdent     	= stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;
    TCMainInfo_t          *MainInfo_p    	= TcHal_GetMainInfo( TC_Params_p, SlotIdent );

    STSYS_WriteRegDev16LE((void*)&MainInfo_p->SectionPesFilter_p, TC_INVALID_LINK);
    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_FilterAllocateTransport
  Description : stpti_TCGetFreeTSFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterAllocateTransport(FullHandle_t DeviceHandle, U32 *FilterIdent)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_FilterInitialiseTransport
  Description : stpti_TCInitialiseTSFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterInitialiseTransport(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_FilterAssociateTransport
  Description : stpti_TCAssociateTSFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterAssociateTransport(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_FilterDisassociateTransport
  Description : stpti_TCDisassociateTSFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDisassociateTransport(FullHandle_t SlotHandle, FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_HardwarePause
  Description : stpti_TCHardwarePause
   Parameters :  No change for multiple slots per session since we stop/start the
   WHOLE pti.
******************************************************************************/
ST_ErrorCode_t stptiHAL_HardwarePause(FullHandle_t DeviceHandle)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    TCDevice_t           *TC_Device     = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32 slot;
    S8 TimeOut = 20; /* wait up to 20 packet (approx.) times, modify if doing BIG transfers that may take some time */
    U32 EnablesMask;


    stptiHelper_ClearLookupTable(DeviceHandle);
    stptiHelper_WaitPacketTime();

    TC_Device->IIFFIFOEnable = 0;

    for (slot = 0; slot < TC_Params_p->TC_NumberSlots; ++slot)
    {
        stptiHelper_WindBackDMA(DeviceHandle, slot);
    }

    /* PMC 13/12/01 Add in check to see if DMAx have completed
       before returning. This is only possible in PTI3 due to
       new register */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
    STTBX_Print(("\n     TC_Device->DMAEnable = 0x%x\n", ReadDMAReg32((void *)&TC_Device->DMAEnable, TC_Params_p)));
#else
    STTBX_Print(("\n     TC_Device->DMAEnable = 0x%x\n", TC_Device->DMAEnable));
#endif    
    STTBX_Print(("   TC_Device->DMAempty_EN = 0x%x\n",   TC_Device->DMAempty_EN   ));
    STTBX_Print((" TC_Device->DMAempty_STAT = 0x%x\n",   TC_Device->DMAempty_STAT ));
#endif
    /* GJP 16/Oct/2K3. The DMAempty_STAT we get is ANDed in the hardware with
      the corresponding bits in DMAEnable so we have to do a comparison in
      software between DMAempty_STAT and DMAEnable so we only wait on the DMAs
      in action rather than just checking all are empty by DMAempty_STAT
      being equal to 0x07.

   Bit  Bit field       Function
    0   DMAEnable0      '1'/'0' enable/disable ch 0.
    1   DMAEnable1      '1'/'0' enable/disable ch 1.
    2   DMAEnable2      '1'/'0' enable/disable ch 2.
    3   DMAEnable3      '1'/'0' enable/disable ch 3.

    Bit Bit Field       Function
    0   DMAempty_STAT1  DMA channel 1 buffer empty.
    1   DMAempty_STAT2  DMA channel 2 buffer empty.
    2   DMAempty_STAT3  DMA channel 3 buffer empty.
*/

    while( TimeOut >= 1 )
    {
        /* constantly update EnablesMask in case this is happening to DMAEnable as well. */
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
        EnablesMask = (ReadDMAReg32((void *)&TC_Device->DMAEnable, TC_Params_p) >> 1);
#else
        EnablesMask = (TC_Device->DMAEnable >> 1);
#endif        

        if (EnablesMask == 0)       /* no DMAs waiting to complete */
            return( ST_NO_ERROR );

        if ( TC_Device->DMAempty_STAT == EnablesMask )  /* all enabled DMAs have completed */
            return( ST_NO_ERROR );

        stptiHelper_WaitPacketTime();
        TimeOut--;
    }

    return( ST_ERROR_TIMEOUT );
}


/******************************************************************************
Function Name : stptiHAL_HardwareResume
  Description : stpti_TCHardwareResume
   Parameters : No change for multiple slots per session since we stop/start the
   WHOLE pti.
******************************************************************************/
ST_ErrorCode_t stptiHAL_HardwareResume(FullHandle_t DeviceHandle)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    TCDevice_t           *TC_Device     = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    FullHandle_t SlotHandle;
    U32 slot;


    TC_Device->IIFFIFOEnable = 1;

    for (slot = 0; slot < TC_Params_p->TC_NumberSlots; ++slot)
    {
        SlotHandle.word = PrivateData_p->SlotHandles_p[slot];

        if ( SlotHandle.word != STPTI_NullHandle() )
        {
            TCMainInfo_t *MainInfo_p = TcHal_GetMainInfo( TC_Params_p, slot );

            stptiHelper_SetLookupTableEntry(DeviceHandle, slot, stptiMemGet_Slot(SlotHandle)->Pid );
            STSYS_WriteRegDev16LE((void*)&MainInfo_p->SlotState, 0);

        }
    }

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_HardwareReset
  Description : stpti_TCHardwareReset
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_HardwareReset(FullHandle_t DeviceHandle)
{
      Device_t *Device_p  = stptiMemGet_Device( DeviceHandle );
    TCDevice_t *TC_Device = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;


    U16 DirectDma;
    U16 IIFSyncPeriod = 188;    /* default is 188 for DVB */
    U16 TagBytes = 0;           /* default is TSMERGER in bypass mode (or no TSMERGER) */

    STPTI_CRITICAL_SECTION_BEGIN {

        TC_Device->IIFFIFOEnable = 0;       /* Stop the IIF */
        TC_Device->TCMode = 0;              /* Stop the TC */

        TC_Device->DMAPTI3Prog = 1;         /* PTI3 Mode   */
        TC_Device->DMAFlush    = 1;         /* Flush DMA 0 */

        /* For PTI4SL, if we do not enable the DMA here, the flush never occurs... */
        TC_Device->DMAEnable   = 1;         /* Enable the DMA */

        while (TC_Device->DMAFlush&0X01)
        {
            /* Wait for 12 NOP, to flush the DMA. i.e for 12 CPU cycles. GNBvd17097, GNBvd17119 */
            __ASM_12_NOPS;
        }

        TC_Device->TCMode   = 8;            /* Full Reset the TC  */

        /* Wait for 24 NOP, i.e for 24 CPU cycles. GNBvd17097, GNBvd17119  */
        __ASM_12_NOPS;
        __ASM_12_NOPS;

        TC_Device->TCMode   = 0;            /* Finish Full Reset the TC  */

        /* Reset the DMA's */
        TC_Device->DMAEnable   = 0;          /* Disable DMAs */
        TC_Device->DMAPTI3Prog = 1;          /* PTI3 Mode */

        TC_Device->DMA0Base = 0;
        TC_Device->DMA0Top = 0;
        TC_Device->DMA0Write = 0;
        TC_Device->DMA0Read = 0;
        TC_Device->DMA0Setup = 0;
        TC_Device->DMA0Holdoff = (1 | (1 << 16));
        TC_Device->DMA0Status = 0;

        TC_Device->DMA1Base = 0;
        TC_Device->DMA1Top = 0;
        TC_Device->DMA1Write = 0;
        TC_Device->DMA1Read = 0;
        TC_Device->DMA1Setup = 0;
        TC_Device->DMA1Holdoff = (1 | (1 << 16));
        TC_Device->DMA1CDAddr = 0;
        TC_Device->DMASecStart = 0;

        TC_Device->DMA2Base = 0;
        TC_Device->DMA2Top = 0;
        TC_Device->DMA2Write = 0;
        TC_Device->DMA2Read = 0;
        TC_Device->DMA2Setup = 0;
        TC_Device->DMA2Holdoff = (1 | (1 << 16));
        TC_Device->DMA2CDAddr = 0;
        TC_Device->DMAFlush = 0;

        TC_Device->DMA3Base = 0;
        TC_Device->DMA3Top = 0;
        TC_Device->DMA3Write = 0;
        TC_Device->DMA3Read = 0;
        TC_Device->DMA3Setup = 0;
        TC_Device->DMA3Holdoff = (1 | (1 << 16));
        TC_Device->DMA3CDAddr = 0;

        TC_Device->DMAEnable = 0xf;          /* Enable DMAs */

    } STPTI_CRITICAL_SECTION_END;

    /* Reset IIF Registers */

    TC_Device->IIFFIFOEnable = 0;
    TC_Device->IIFAltLatency = 64;

    TC_Device->IIFSyncLock = Device_p->SyncLock;
    TC_Device->IIFSyncDrop = Device_p->SyncDrop;


    if ( Device_p->StreamID != STPTI_STREAM_ID_NOTAGS )
        TagBytes = 6;

    TC_Device->IIFSyncPeriod = IIFSyncPeriod + TagBytes;


    TC_Device->IIFCAMode = 1;

    /* Reset Direct DMA values, allow user to set back buffering again <GNBvd25428>*/
    for (DirectDma = 1; (DirectDma < 4); ++DirectDma)
    {
        Device_p->TCPrivateData.TCUserDma_p->DirectDmaCDAddr[DirectDma] = 0;
        Device_p->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
        Device_p->TCPrivateData.TCUserDma_p->DirectDmaCompleteAddress[DirectDma] = 0;
    }

    /* Restart the PTI */

    TC_Device->TCMode = 2;              /* Reset I Ptr */
    TC_Device->TCMode = 1;              /* Start the TC */
    TC_Device->IIFFIFOEnable = 1;       /* Start the IIF */

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_SetSystemKey
  Description :
   Parameters :
         NOTE : new function
******************************************************************************/
ST_ErrorCode_t stptiHAL_SetSystemKey(FullHandle_t DeviceHandle, U8 *Data, U16 KeyNumber)
{
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    TCSystemKey_t    *SystemKey_p = TcHal_GetSystemKey(TC_Params_p, KeyNumber);

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("System key address 0x%08x\n",SystemKey_p));
#endif
    
    SystemKey_p->SystemKey7 = Data[28] << 24 | Data[29] << 16 | Data[30] << 8 | Data[31];
    SystemKey_p->SystemKey6 = Data[24] << 24 | Data[25] << 16 | Data[26] << 8 | Data[27];
    SystemKey_p->SystemKey5 = Data[20] << 24 | Data[21] << 16 | Data[22] << 8 | Data[23];
    SystemKey_p->SystemKey4 = Data[16] << 24 | Data[17] << 16 | Data[18] << 8 | Data[19];
    SystemKey_p->SystemKey3 = Data[12] << 24 | Data[13] << 16 | Data[14] << 8 | Data[15];
    SystemKey_p->SystemKey2 = Data[8] << 24 | Data[9] << 16 | Data[10] << 8 | Data[11];
    SystemKey_p->SystemKey1 = Data[4] << 24 | Data[5] << 16 | Data[6] << 8 | Data[7];
    SystemKey_p->SystemKey0 = Data[0] << 24 | Data[1] << 16 | Data[2] << 8 | Data[3];

    return ( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DescramblerAllocate
  Description : stpti_TCGetFreeDescrambler
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerAllocate(FullHandle_t DeviceHandle, U32 *DescIdent)
{
    U32 Descrambler;

    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;


    for (Descrambler = 0; Descrambler < TC_Params_p->TC_NumberDescramblerKeys; ++Descrambler)
    {
        if (PrivateData_p->DescramblerHandles_p[Descrambler] == STPTI_NullHandle())
            break;
    }

    if (Descrambler < TC_Params_p->TC_NumberDescramblerKeys)
    {
        *DescIdent = Descrambler;

        return( ST_NO_ERROR );
    }

    return( STPTI_ERROR_NO_FREE_DESCRAMBLERS );
}


/******************************************************************************
Function Name : stptiHAL_DescramblerInitialise
  Description : stpti_TCInitialiseDescrambler
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerInitialise(FullHandle_t DescramblerHandle)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DescramblerHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32                   DescIdent     = stptiMemGet_Descrambler(DescramblerHandle)->TC_DescIdent;
    TCKey_t              *Key_p         = TcHal_GetDescramblerKey( TC_Params_p, DescIdent );

    stptiMemGet_Device(DescramblerHandle)->TCPrivateData.DescramblerHandles_p[DescIdent] = DescramblerHandle.word;

    /* mark as not valid */
    STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, 0);

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_ModifySyncLockAndDrop
  Description : stpti_TCModifySyncLockAndDrop
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_ModifySyncLockAndDrop(FullHandle_t DeviceHandle, U8 SyncLock, U8 SyncDrop)
{
    TCDevice_t *TC_Device = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

    TC_Device->IIFSyncLock = SyncLock;
    TC_Device->IIFSyncDrop = SyncDrop;


    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DescramblerDisassociate
  Description : stpti_TCDisassociateDescrambler
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerDisassociate(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle)
{
#ifdef STPTI_LOADER_SUPPORT
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
#else
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DescramblerHandle);
    U32                   SlotIdent     = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;

    TcHal_MainInfoDisassociateDescramblerWithSlot( (STPTI_TCParameters_t *)&PrivateData_p->TC_Params, SlotIdent );

    return( ST_NO_ERROR );
#endif
}





/******************************************************************************
Function Name : stptiHAL_DescramblerSet
  Description : stpti_TCSetDescramblerKey
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerSet(FullHandle_t DescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DescramblerHandle);
    U32                   DescIdent     = stptiMemGet_Descrambler(DescramblerHandle)->TC_DescIdent;

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCKey_t              *Key_p         = TcHal_GetDescramblerKey( TC_Params_p, DescIdent );
    U16 KeyCheck;
    BOOL AESMode = FALSE;
    BOOL AESIVLoad = FALSE;
    BOOL TDESIVLoad = FALSE;
    BOOL MULTI2Mode = FALSE;
    BOOL TDESMode = FALSE;
    U16 KeyModeSet = 0;
    
    if ( PrivateData_p->DescramblerHandles_p[DescIdent] != DescramblerHandle.word )
        return( STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE );

    STSYS_ClearTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_ALGORITHM_MASK);
    STSYS_ClearTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_CHAIN_ALG_MASK | TCKEY_CHAIN_MODE_SYN));

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    KeyModeSet = STSYS_ReadRegDev16LE((void*)&Key_p->KeyMode);
    KeyModeSet &= ~(TCKEY_MODE_EAVS|TCKEY_MODE_PERM0|TCKEY_MODE_PERM1);   /* (Shared TC register) Mask off bits affected by this function */
#endif
    
    switch(stptiMemGet_Descrambler(DescramblerHandle)->Type)
    {
        case STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER:
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        case STPTI_DESCRAMBLER_TYPE_AES_SYN_DESCRAMBLER:
#endif
            AESMode = TRUE;
            AESIVLoad = TRUE;
            break;


        case STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DVS042_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER:
            MULTI2Mode = TRUE;
            break;
        
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        case STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER:
        case STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER:
            TDESMode = TRUE;
            TDESIVLoad = TRUE;
            break;
#endif

        default:        /* not AES or MULT2 so ignore */
            break;
    }

    switch ( stptiMemGet_Descrambler(DescramblerHandle)->Type )
    {

        case STPTI_DESCRAMBLER_TYPE_DVB_EAVS_DESCRAMBLER:
            KeyModeSet |= TCKEY_MODE_EAVS;
            /* fall through to case STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER:*/

        case STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER: 
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_ALGORITHM_DVB);
            break;

        case STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER:
/*  DSS descrambling mode fix to prevent transition 1 -> 0 -> 1, this is due to
   masking off the bit and telling the TC to go into the wrong mode for a packet
   or two before the bit can be set again, a typical symptom are "random" video glitches
   on DSS encoded streams.
                Key_p->KeyValidity &= ~TCKEY_ALGORITHM_MASK; */
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_ALGORITHM_DSS);
            break;

        case STPTI_DESCRAMBLER_TYPE_DES_CBC_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_DSS | TCKEY_CHAIN_ALG_CBC));
            break;

        case STPTI_DESCRAMBLER_TYPE_DES_ECB_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_DSS | TCKEY_CHAIN_ALG_ECB_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_DES_CBC_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_DSS | TCKEY_CHAIN_ALG_CBC_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_DES_OFB_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_DSS | TCKEY_CHAIN_ALG_OFB));
            break;

        case STPTI_DESCRAMBLER_TYPE_DES_CTS_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_DSS | TCKEY_CHAIN_ALG_CTS));
            break;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        case STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_TDES | TCKEY_CHAIN_ALG_CBC));
            break;

        case STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_TDES | TCKEY_CHAIN_ALG_ECB_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_TDES | TCKEY_CHAIN_ALG_CBC_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_TDES | TCKEY_CHAIN_ALG_OFB));
            break;

        case STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_TDES | TCKEY_CHAIN_ALG_CTS));
            break;
#endif
        case STPTI_DESCRAMBLER_TYPE_FASTI_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_ALGORITHM_FAST_I);
            break;

        case STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_MULTI2 | TCKEY_CHAIN_ALG_ECB));
            break;

        case STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_MULTI2 | TCKEY_CHAIN_ALG_CBC));
            break;

        case STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_MULTI2 | TCKEY_CHAIN_ALG_ECB_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_MULTI2 | TCKEY_CHAIN_ALG_CBC_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_MULTI2 | TCKEY_CHAIN_ALG_OFB));
            break;

        case STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_MULTI2 | TCKEY_CHAIN_ALG_CTS));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_ECB));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_CBC));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_ECB_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_CBC_IV));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_OFB));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_CTS));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_NSA_MDD));
            KeyModeSet |= TCKEY_MODE_MddnotMdi;
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_NSA_MDI));
            break;

        case STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_IPTV_CSA));
            break;

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
        case STPTI_DESCRAMBLER_TYPE_AES_SYN_DESCRAMBLER:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_ALGORITHM_AES | TCKEY_CHAIN_ALG_ECB | TCKEY_CHAIN_MODE_SYN));
            break;
#endif
            
        default:
                break;
    }

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    STSYS_WriteTCReg16LE((void*)&Key_p->KeyMode, KeyModeSet);
#endif

    if ((Usage & STPTI_KEY_USAGE_MASK) == STPTI_KEY_USAGE_INVALID)   /* Parity & Usage already checked by STPTI_DescramblerSet() */
    {
        if (Parity == STPTI_KEY_PARITY_EVEN_PARITY)
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey0, 0);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey1, 0);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey2, 0);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey3, 0);

            if(AESMode || TDESMode)
            {
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey4, 0);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey5, 0);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey6, 0);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey7, 0);
            }
        }
        else       /* STPTI_KEY_PARITY_ODD_PARITY */
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey0, 0);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey1, 0);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey2, 0);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey3, 0);

            if(AESMode || TDESMode)
            {
                STSYS_WriteTCReg16LE((void*)&Key_p->OddKey4, 0);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddKey5, 0);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddKey6, 0);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddKey7, 0);
            }
        }

        KeyCheck  = STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey0) |
                    STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey1) |
                    STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey2) |
                    STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey3);
        KeyCheck |= STSYS_ReadRegDev16LE((void*)&Key_p->OddKey0)  |
                    STSYS_ReadRegDev16LE((void*)&Key_p->OddKey1)  |
                    STSYS_ReadRegDev16LE((void*)&Key_p->OddKey2)  |
                    STSYS_ReadRegDev16LE((void*)&Key_p->OddKey3);

        if(AESMode || TDESMode)
        {
            KeyCheck |= STSYS_ReadRegDev16LE((void*)&Key_p->OddKey4)  |
                        STSYS_ReadRegDev16LE((void*)&Key_p->OddKey5)  |
                        STSYS_ReadRegDev16LE((void*)&Key_p->OddKey6)  |
                        STSYS_ReadRegDev16LE((void*)&Key_p->OddKey7);
            KeyCheck |= STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey4)  |
                        STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey5)  |
                        STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey6)  |
                        STSYS_ReadRegDev16LE((void*)&Key_p->EvenKey7);
        }

        if (0 == KeyCheck)
        {
             /* if all keys zero then mark as not valid */
            STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, 0);
        }

        return( ST_NO_ERROR );
    }

    /* --- valid key of some type to process --- */

    if (Parity == STPTI_KEY_PARITY_EVEN_PARITY)
    {
        if(MULTI2Mode)
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey4, (Data[8] << 8) | Data[9]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey5, (Data[10] << 8) | Data[11]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey6, (Data[12] << 8) | Data[13]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey7, (Data[14] << 8) | Data[15]);
        }
        
        if(AESMode || TDESMode)
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey4, (Data[0] << 8) | Data[1]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey5, (Data[2] << 8) | Data[3]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey6, (Data[4] << 8) | Data[5]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey7, (Data[6] << 8) | Data[7]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey0, (Data[8] << 8) | Data[9]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey1, (Data[10] << 8) | Data[11]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey2, (Data[12] << 8) | Data[13]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey3, (Data[14] << 8) | Data[15]);

            if(AESIVLoad || TDESIVLoad)
            {
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV0,  (Data[16] << 8) | Data[17]);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV1,  (Data[18] << 8) | Data[19]);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV2,  (Data[20] << 8) | Data[21]);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV3,  (Data[22] << 8) | Data[23]);
            }
            if(AESIVLoad)
            {
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV4,  (Data[24] << 8) | Data[25]);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV5,  (Data[26] << 8) | Data[27]);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV6,  (Data[28] << 8) | Data[29]);
                STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV7,  (Data[30] << 8) | Data[31]);
            }
        }
        else
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey0, (Data[0] << 8) | Data[1]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey1, (Data[2] << 8) | Data[3]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey2, (Data[4] << 8) | Data[5]);
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenKey3, (Data[6] << 8) | Data[7]);
        }

        /* set validity after key is set */
        switch (Usage & STPTI_KEY_USAGE_MASK)
        {
            case STPTI_KEY_USAGE_VALID_FOR_PES:
                STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_VALIDITY_PES_EVEN);
                break;

            case STPTI_KEY_USAGE_VALID_FOR_TRANSPORT:
                STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_VALIDITY_TS_EVEN);
                break;

            case STPTI_KEY_USAGE_VALID_FOR_ALL:
                STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_VALIDITY_TS_EVEN | TCKEY_VALIDITY_PES_EVEN));
                break;
#ifdef ST_OSLINUX
                /* Case was not handled. */
            case STPTI_KEY_USAGE_INVALID:
                STTBX_Print(("stptiHAL_DescramblerSet() - Invalid key usage\n"));
                break;
#endif
            default:
                break;
        }

    }
    else      /* STPTI_KEY_PARITY_ODD_PARITY */
    {
        if(MULTI2Mode)
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV4, (Data[8] << 8) | Data[9]);     /* EvenIV4 is used for the OddKey for Multi2 */
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV5, (Data[10] << 8) | Data[11]);   /* EvenIV5 is used for the OddKey for Multi2 */
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV6, (Data[12] << 8) | Data[13]);   /* EvenIV6 is used for the OddKey for Multi2 */
            STSYS_WriteTCReg16LE((void*)&Key_p->EvenIV7, (Data[14] << 8) | Data[15]);   /* EvenIV7 is used for the OddKey for Multi2 */
        }

        if(AESMode || TDESMode)
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey4, (Data[0] << 8) | Data[1]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey5, (Data[2] << 8) | Data[3]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey6, (Data[4] << 8) | Data[5]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey7, (Data[6] << 8) | Data[7]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey0, (Data[8] << 8) | Data[9]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey1, (Data[10] << 8) | Data[11]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey2, (Data[12] << 8) | Data[13]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey3, (Data[14] << 8) | Data[15]);

            if(AESIVLoad || TDESIVLoad)
            {
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV0,  (Data[16] << 8) | Data[17]);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV1,  (Data[18] << 8) | Data[19]);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV2,  (Data[20] << 8) | Data[21]);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV3,  (Data[22] << 8) | Data[23]);
            }
            if(AESIVLoad)
            {
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV4,  (Data[24] << 8) | Data[25]);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV5,  (Data[26] << 8) | Data[27]);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV6,  (Data[28] << 8) | Data[29]);
                STSYS_WriteTCReg16LE((void*)&Key_p->OddIV7,  (Data[30] << 8) | Data[31]);
            }
        }
        else
        {
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey0, (Data[0] << 8) | Data[1]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey1, (Data[2] << 8) | Data[3]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey2, (Data[4] << 8) | Data[5]);
            STSYS_WriteTCReg16LE((void*)&Key_p->OddKey3, (Data[6] << 8) | Data[7]);
        }

        /* set validity after setting key */
        switch (Usage & STPTI_KEY_USAGE_MASK)
        {
            case STPTI_KEY_USAGE_VALID_FOR_PES:
                STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_VALIDITY_PES_ODD);
                break;

            case STPTI_KEY_USAGE_VALID_FOR_TRANSPORT:
                STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, TCKEY_VALIDITY_TS_ODD);
                break;

            case STPTI_KEY_USAGE_VALID_FOR_ALL:
                STSYS_SetTCMask16LE((void*)&Key_p->KeyValidity, (TCKEY_VALIDITY_TS_ODD | TCKEY_VALIDITY_PES_ODD));
                break;
#ifdef ST_OSLINUX
                /* Case was not handled. */
            case STPTI_KEY_USAGE_INVALID:
                STTBX_Print(("stptiHAL_DescramblerSet() - Invalid key usage\n"));
                break;
#endif
            default:
                break;
        }
    }

    return( ST_NO_ERROR );
}



/******************************************************************************
Function Name : stptiHAL_DescramblerSetType
  Description : set descrambler type on-the-fly
   Parameters : new Type
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerSetType(FullHandle_t DescramblerHandle, STPTI_DescramblerType_t DescramblerType)
{
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DescramblerHandle);
    U32                   DescIdent     = stptiMemGet_Descrambler(DescramblerHandle)->TC_DescIdent;

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCKey_t              *Key_p         = TcHal_GetDescramblerKey( TC_Params_p, DescIdent );
    
    if ( PrivateData_p->DescramblerHandles_p[DescIdent] != DescramblerHandle.word )
        return( STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE );

    STSYS_ClearTCMask16LE((void*)&Key_p->KeyValidity, STPTI_KEY_USAGE_MASK);
    stptiMemGet_Device(DescramblerHandle)->DescramblerAssociationType = DescramblerType;
    stptiMemGet_Descrambler(DescramblerHandle)->Type = DescramblerType;
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DescramblerSetSVP
  Description : set the options for SVP
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DescramblerSetSVP(FullHandle_t DescramblerHandle, BOOL Clear_SCB, STPTI_NSAMode_t mode)
{

#if !defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
    
#else  /* STPTI_ARCHITECTURE_PTI4_SECURE_LITE is defined */
    
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(DescramblerHandle);
    U32                   DescIdent     = stptiMemGet_Descrambler(DescramblerHandle)->TC_DescIdent;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCKey_t              *Key_p         = TcHal_GetDescramblerKey( TC_Params_p, DescIdent );
    
    if ( PrivateData_p->DescramblerHandles_p[DescIdent] != DescramblerHandle.word )
        return( STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE );

    
    if( Clear_SCB )
    {
        STSYS_SetTCMask16LE((void*)&Key_p->KeyMode, TCKEY_MODE_ClearSCB);
    }
    else
    {
        STSYS_ClearTCMask16LE((void*)&Key_p->KeyMode, TCKEY_MODE_ClearSCB);
    }    
   
    switch(mode)
    {
        case STPTI_NSA_MODE_MDD:
            STSYS_SetTCMask16LE((void*)&Key_p->KeyMode, TCKEY_MODE_MddnotMdi);
            break;
            
        case STPTI_NSA_MODE_MDI:
            STSYS_ClearTCMask16LE((void*)&Key_p->KeyMode, TCKEY_MODE_MddnotMdi);
            break;
            
        default:
            return( ST_ERROR_BAD_PARAMETER );
    }
    
    return( ST_NO_ERROR );
    
#endif
}

/******************************************************************************
Function Name : stptiHAL_GetPacketArrivalTime
  Description : stpti_TCGetPacketArrivalTime
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_GetPacketArrivalTime(FullHandle_t DeviceHandle, STPTI_TimeStamp_t * ArrivalTime, U16 *ArrivalTimeExtension)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8  RawData[5]       = {0,0,0,0,0};
    int i;
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

#if defined (STPTI_FRONTEND_HYBRID)
    U32 MSW,LSW;
#endif

    /* if we are in the bit of TC code where the arrival time has been reset to
       zero and not set then try again (Max 3 times) */
    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];

    for (i=0; i<3; i++)
    {
        if (Device_p->PacketArrivalTimeSource == STPTI_ARRIVAL_TIME_SOURCE_TSMERGER)
        {
            /*Because TSMerger derived STCs are in a different format to
            those derived in the PTI STC, we have to do some nasty
            transformations*/

#if defined (STPTI_FRONTEND_HYBRID)
            /* Check for a valid frontend handle & type */
            if ( Device_p->FrontendListHandle.word != STPTI_NullHandle() && ((Device_p->StreamID >> 8) & STPTI_STREAM_IDTYPE_TSIN) )
            {
                StfeHal_ReadTagCounter ( Device_p->StreamID, &MSW, &LSW );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
                STTBX_Print(("stptiHAL_GetPacketArrivalTime %08x %08x %08x %08x\n",MSW,LSW,TAG_COUNTER_REG_COUNTER_MSW((Device_p->StreamID & 0x3F)),TAG_COUNTER_REG_COUNTER_LSW((Device_p->StreamID & 0x3F)) ));
#endif
                RawData[0] =  (MSW & 0x3fc) >> 2;
                RawData[1] =  (((MSW & 0x3) << 6) | (LSW & 0xfc000000) >> 26);
                RawData[2] =  ((LSW & 0x3fc0000) >> 18);
                RawData[3] =  ((LSW & 0x3fc00) >> 10) ;
                RawData[4] = ((LSW & 0x200) >> 2);
            }
#else
            RawData[0] =  ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord0) & 0x3fc) >> 2);
            RawData[1] =  ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord0) & 0x3) << 6) |
                      ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord1) & 0xfc00) >> 10) ;
            RawData[2] =  ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord1) & 0x3fc) >> 2);
            RawData[3] =  ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord1) & 0x3) << 6) |
                      ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord2) & 0xfc00) >> 10) ;
            RawData[4] = ((STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord2) & 0x200) >> 2);
#endif
        }
        else
        {
            RawData[0] = (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord0) & 0xff00) >> 8;
            RawData[1] =  STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord0) & 0xff;
            RawData[2] = (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord1) & 0xff00) >> 8;
            RawData[3] =  STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord1) & 0xff;
            RawData[4] = (STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord2) & 0xff00) >> 8;

        }
        stptiHAL_PcrToTimestamp(ArrivalTime, RawData);

        if (ArrivalTime->LSW > 0)
        {
            Error = ST_NO_ERROR;
            break;
        }
        else
        {
            Error = STPTI_ERROR_NO_PACKET;
        }
    }
#if defined (STPTI_FRONTEND_HYBRID)
    *ArrivalTimeExtension = LSW & 0x1ff;
#else
    *ArrivalTimeExtension = STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionSTCWord2) & 0x1ff;
#endif
    return( Error );
}


/******************************************************************************
Function Name : stptiHAL_GetInputPacketCount
  Description : stpti_TCGetInputPacketCount
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_GetInputPacketCount(FullHandle_t DeviceHandle, U16 *Count_p)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;


    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];

    *Count_p = STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionInputPacketCount);
    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_GetPtiPacketCount
  Description : stpti_TCGetInputPacketCount
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_GetPtiPacketCount(FullHandle_t DeviceHandle, U32 *Count_p)
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCGlobalInfo_t       *TCGlobalInfo_p;


    TCGlobalInfo_p =  (TCGlobalInfo_t *)TC_Params_p->TC_GlobalDataStart;

    *Count_p = TCGlobalInfo_p->GlobalPktCount;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_GetCurrentPTITimer
  Description : stpti_TCGetCurrentPTITimer
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_GetCurrentPTITimer(FullHandle_t DeviceHandle, STPTI_TimeStamp_t * TimeStamp)
{
    TCDevice_t *TC_Device = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;


    /* PMC 09/02/02 Only look at bottom bit of STCTimer1 register */
    TimeStamp->Bit32 = (TC_Device->STCTimer1 & 0x01);
    TimeStamp->LSW   = TC_Device->STCTimer0;

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_BufferLinkToCdFifo
  Description : stpti_TCSetUpBufferDirectDMA
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_BufferLinkToCdFifo(FullHandle_t BufferHandle, STPTI_DMAParams_t *CdFifoParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(BufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32 DirectDma;
    U16 MainInfoMask = 0;
    U32 base = (U32)  stptiMemGet_Buffer(BufferHandle)->Start_p;
    U32 top  = (U32) (base + stptiMemGet_Buffer(BufferHandle)->ActualSize);


    /* Use the generic DMA setup function */
    /* PMC 26/09/02 - GNBvd16003, Must run back-buffered DMA transfers in byte mode or it
       will cause corruption on external CD-FIFO's (e.g. STi7020 */

    Error = stptiHAL_UserDataWrite(BufferHandle,
                                        (U8*)base, 0,
                                        (U8*)base, (U8*)top, NULL,
                                        CdFifoParams, FALSE, (CdFifoParams->BurstSize & 0x01),
                                        &DirectDma);
    if (Error == ST_NO_ERROR)
    {
        /* We must now modify all the MainInfos for the slots that feed this buffer, so that they
           know to update the 'direct dma's' write pointers */

        FullHandle_t SlotListHandle = stptiMemGet_Buffer(BufferHandle)->SlotListHandle;

        stptiMemGet_Buffer(BufferHandle)->DirectDma = DirectDma;

        switch (DirectDma)
        {
            case 1:
                MainInfoMask = TC_MAIN_INFO_SLOT_MODE_DMA_1;
                break;
            case 2:
                MainInfoMask = TC_MAIN_INFO_SLOT_MODE_DMA_2;
                break;
            case 3:
                MainInfoMask = TC_MAIN_INFO_SLOT_MODE_DMA_3;
                break;
        }

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
                    TCMainInfo_t *MainInfo_p = &((TCMainInfo_t *) TC_Params_p->TC_MainInfoStart)[SlotIdent];

                    STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, TC_MAIN_INFO_SLOT_MODE_DMA_FIELD);
                    STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, MainInfoMask);
                }
            }
        }

        {
            TCDMAConfig_t *DMAConfig_p;

            U32 dma = stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent;
            DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[dma];

            /* we must remove the quatisation flags from the dma structure ( feeding the buffer )
               to prevent further signalling */
            STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, (TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS | TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION));
            STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
        }
    }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print((" stptiHAL_BufferLinkToCdFifo() = %d\n", Error  ));
#endif

    return( Error );
}

/******************************************************************************
Function Name : stptiHAL_UserDataWrite
  Description : stpti_TCSetUpUserDirectDMA
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_UserDataWrite( FullHandle_t DeviceHandle, U8 *DataPtr_p, size_t DataLength,
                                       U8           *BufferPtr_p, U8 *BufferEnd_p, U8  **NextPtr_p,
                                       STPTI_DMAParams_t *DMAParams, BOOL DMASignal, U32 DMASetup,
                                       U32 *DirectDmaReturn)
{
#if !defined ( STPTI_MULTI_DMA_SUPPORT ) || !defined( STPTI_HARDWARE_CDFIFO_SUPPORT )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#endif

    /* To avoid warning */
    U32 DMAempty_EN=0;
    void *Cfg_p = NULL;

    /* This structure maps the STPTI_CDReqLine_t enum to the config values */
    static S16 CDRegMode[] =
    {

#if defined(ST_5528)
/*  0   000: video0 cdreq  (see: ADCS 7516982)
    1   001: video1 cdreq
    2/3 01X: reserved
    4   100: audio0 cdreq
    5   101: audio1 cdreq
    6   110: audio2 cdreq
    7   111: reserved
          U    V0   A0                                                         V1  A1  A2 */
          2,   0,   4,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  1,  5,  6
#else
#if defined(ST_5100) || defined(ST_5301) || defined (ST_7100)
/*  In 5100/5301/7100/7109 there is no Video CDREQ. The available cdreq are as follows.

    0  000: Audio cdreq  (see: ADCS 7603604)
    1  001: TSMERGER_SWTS_REQ
                    A                 SWTS                                                */
         -1,  -1,   1,  -1,  -1,  -1,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1
#else
#if defined(ST_7710)
/*  In 7710 there is no Video CDREQ. The available cdreq are as follows.

    0  000: Audio cdreq  (see: ADCS 7571273A)
    1  001: TSMERGER_SWTS_REQ
                    A                 SWTS                                                */
         -1,  -1,   0,  -1,  -1,  -1,   1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1, -1, -1

#else
#error Please specify cdreq for chip!

#endif
#endif
#endif

    };

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 DirectDma;
    U32 Holdoff     = DMAParams->Holdoff;
    U32 WriteLength = DMAParams->WriteLength;
    U32 Base        =  (U32)BufferPtr_p & ~0X0F;       /* Round Down to 16 bytes */
    U32 Top         = ((U32)BufferEnd_p & ~0X0F) -1;   /* Round Down to 16 bytes */
    U32 Write       = Base;

    if (0 > CDRegMode[DMAParams->CDReqLine])
        return ( ST_ERROR_BAD_PARAMETER );  /* Not a supported CDReg for this chip */


    for (DirectDma = 1; DirectDma <= 3; DirectDma++)
    {
        if ( stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] == FALSE )
        {
            TCDevice_t *TC_Device_p = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

            /* Set up the DMA */

            if (Top > Base)
            {
                /* Circular buffer */
                U32 Offset = ((U32)DataPtr_p - Base) +DataLength;
                Write = (Base + (Offset%(Top+1-Base)));
            }
            else
            {
                /* Linear buffer */
                Write = (U32) DataPtr_p + DataLength;
            }

            /* Mux the CDREQ lines */

            {
                U32 Config = 0;
                U32 BitPos = 0;

                Cfg_p = ioremap( CFG_BASE_ADDRESS, sizeof(U32));

#if defined(ST_OSLINUX)
                    if( Cfg_p == NULL )
                    {
                            printk(KERN_ALERT"Failed to ioremap config registers\n");
                            return( STPTI_ERROR_IOREMAPFAILED );
                    }
                #endif

                Config = STSYS_ReadRegDev32LE((void*)Cfg_p);

                iounmap( Cfg_p );

#if defined(ST_5528)
                    Cfg_p = ioremap( CFG_BASE_ADDRESS, sizeof(U32));
                #else
                    Cfg_p = ioremap( PTI_BASE_ADDRESS, sizeof(U32));
                #endif

#if defined(ST_OSLINUX)
                    if( Cfg_p == NULL )
                    {
                            printk(KERN_ALERT"Failed to ioremap config registers\n");
                            return( STPTI_ERROR_IOREMAPFAILED );
                    }
                #endif

#if defined(ST_5528)
                    BitPos = ((DirectDma-1) * 3);
                #else
                    BitPos = ( 16 - (DirectDma));
                #endif

#if defined(NATIVE_CORE)
#if defined(ST_7100)
                Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x0058/4));

                Config &= ~(0x1 << (BitPos));                   
                Config |= (CDRegMode[DMAParams->CDReqLine] << (BitPos));
                        
                STSYS_WriteRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x058/4), Config );
#else
    #error Only 7100 and 7109 platforms supported at present
#endif

#else
                switch ( (U32)stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p )
                {

#if defined(ST_5528)
                    case ST5528_PTIA_BASE_ADDRESS:
                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x030/4));

                        Config &= ~(0x7 << (3 + BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (3 + BitPos));

                        STSYS_WriteRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x030/4), Config );
                        break;

                    case ST5528_PTIB_BASE_ADDRESS:
                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x034/4));

                        Config &= ~(0x7 << (3 + BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (3 + BitPos));

                        STSYS_WriteRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x034/4), Config );
                        break;
                    #endif

#if defined(ST_5100)
                    case ST5100_PTI_BASE_ADDRESS:
                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x0058/4));

                        Config &= ~(0x1 << (BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (BitPos));

                        STSYS_WriteRegDev32LE((U32)((STPTI_DevicePtr_t)Cfg_p + 0x058/4), Config );
                        break;
                    #endif

#if defined(ST_5301)
                    case ST5301_PTI_BASE_ADDRESS:

                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x0058/4));

                        Config &= ~(0x1 << (BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (BitPos));

                        STSYS_WriteRegDev32LE((U32)((STPTI_DevicePtr_t)Cfg_p + 0x058/4), Config );
                        break;
                    #endif


#if defined (ST_5525)
                    case ST5525_PTI_BASE_ADDRESS:
                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x0058/4));

                        Config &= ~(0x1 << (BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (BitPos));

                        STSYS_WriteRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x058/4), Config );
                        break;
                    #endif


#if defined (ST_7100)
                    case ST7100_PTI_BASE_ADDRESS:
                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x0058/4));

                        Config &= ~(0x1 << (BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (BitPos));

                        STSYS_WriteRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x058/4), Config );
                        break;
                    #endif


#if defined(ST_7710)
                    case ST7710_PTI_BASE_ADDRESS:
                        Config = STSYS_ReadRegDev32LE((void*)((STPTI_DevicePtr_t)ST7710_PTI_BASE_ADDRESS + 0x058/4));

                        Config &= ~(0x1 << (BitPos));
                        Config |= (CDRegMode[DMAParams->CDReqLine] << (BitPos));

                        STSYS_WriteRegDev32LE((void*)((STPTI_DevicePtr_t)Cfg_p + 0x058/4), Config );
                        break;
                    #endif
                }
#endif      /* #endif NATIVE_CORE */        
            }

            iounmap( Cfg_p ); /* Unmap the memory */

            switch ( DirectDma )
            {

            case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                WriteDMAReg32( (void*)&TC_Device_p->DMAEnable    , (~TC_GLOBAL_DMA_ENABLE_DMA1) &
                                                                     TC_GLOBAL_DMA_ENABLE_MASK,
                                                                     TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1Base     , TRANSLATE_PHYS_ADD(Base), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1Top      , TRANSLATE_PHYS_ADD(Top), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1Write    , TRANSLATE_PHYS_ADD(Write), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1Read     , (U32) TRANSLATE_PHYS_ADD(DataPtr_p), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1Setup    , DMASetup, TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1Holdoff  , (Holdoff | (WriteLength << 16)), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA1CDAddr   , TRANSLATE_PHYS_ADD( DMAParams->Destination), TC_Params_p );
#else
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMAEnable    , (~TC_GLOBAL_DMA_ENABLE_DMA1) &
                                                                           TC_GLOBAL_DMA_ENABLE_MASK);
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1Base     , TRANSLATE_PHYS_ADD(Base));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1Top      , TRANSLATE_PHYS_ADD(Top));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1Write    , TRANSLATE_PHYS_ADD(Write));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1Read     , (U32) TRANSLATE_PHYS_ADD(DataPtr_p));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1Setup    , DMASetup);
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1Holdoff  , (Holdoff | (WriteLength << 16)));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA1CDAddr   , TRANSLATE_PHYS_ADD( DMAParams->Destination));
#endif
                if (DMASignal)
                    DMAempty_EN = STSYS_ReadRegDev32LE( (void*)&TC_Device_p->DMAempty_EN ) | 0X01;
                else
                    DMAempty_EN = STSYS_ReadRegDev32LE( (void*)&TC_Device_p->DMAempty_EN ) & ~0X01;
                break;

            case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                WriteDMAReg32( (void*)&TC_Device_p->DMAEnable    , (~TC_GLOBAL_DMA_ENABLE_DMA2) &
                                                                     TC_GLOBAL_DMA_ENABLE_MASK,
                                                                     TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2Base     , TRANSLATE_PHYS_ADD(Base), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2Top      , TRANSLATE_PHYS_ADD(Top), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2Write    , TRANSLATE_PHYS_ADD(Write), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2Read     , (U32) TRANSLATE_PHYS_ADD(DataPtr_p), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2Setup    , DMASetup, TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2Holdoff  , (Holdoff | (WriteLength << 16)), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA2CDAddr   , TRANSLATE_PHYS_ADD( DMAParams->Destination ), TC_Params_p);
#else
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMAEnable    , (~TC_GLOBAL_DMA_ENABLE_DMA2) &
                                                                           TC_GLOBAL_DMA_ENABLE_MASK);
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2Base     , TRANSLATE_PHYS_ADD(Base));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2Top      , TRANSLATE_PHYS_ADD(Top));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2Write    , TRANSLATE_PHYS_ADD(Write));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2Read     , (U32) TRANSLATE_PHYS_ADD(DataPtr_p));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2Setup    , DMASetup);
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2Holdoff  , (Holdoff | (WriteLength << 16)));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA2CDAddr   , TRANSLATE_PHYS_ADD( DMAParams->Destination));
#endif
                if (DMASignal)
                    DMAempty_EN = STSYS_ReadRegDev32LE( (void*)&TC_Device_p->DMAempty_EN ) | 0X02;
                else
                    DMAempty_EN = STSYS_ReadRegDev32LE( (void*)&TC_Device_p->DMAempty_EN ) & ~0X02;
                break;

            case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                WriteDMAReg32( (void*)&TC_Device_p->DMAEnable    , (~TC_GLOBAL_DMA_ENABLE_DMA3) &
                                                                     TC_GLOBAL_DMA_ENABLE_MASK,
                                                                     TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3Base     , TRANSLATE_PHYS_ADD(Base), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3Top      , TRANSLATE_PHYS_ADD(Top), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3Write    , TRANSLATE_PHYS_ADD(Write), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3Read     , (U32) TRANSLATE_PHYS_ADD(DataPtr_p), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3Setup    , DMASetup, TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3Holdoff  , (Holdoff | (WriteLength << 16)), TC_Params_p );
                WriteDMAReg32( (void*)&TC_Device_p->DMA3CDAddr   , TRANSLATE_PHYS_ADD( DMAParams->Destination ), TC_Params_p);
#else
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMAEnable    , (~TC_GLOBAL_DMA_ENABLE_DMA3) &
                                                                           TC_GLOBAL_DMA_ENABLE_MASK);
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3Base     , TRANSLATE_PHYS_ADD(Base));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3Top      , TRANSLATE_PHYS_ADD(Top));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3Write    , TRANSLATE_PHYS_ADD(Write));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3Read     , (U32) TRANSLATE_PHYS_ADD(DataPtr_p));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3Setup    , DMASetup);
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3Holdoff  , (Holdoff | (WriteLength << 16)));
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMA3CDAddr   , TRANSLATE_PHYS_ADD( DMAParams->Destination));
#endif
                if (DMASignal)
                    DMAempty_EN = STSYS_ReadRegDev32LE( (void*)&TC_Device_p->DMAempty_EN ) | 0X04;
                else
                    DMAempty_EN = STSYS_ReadRegDev32LE( (void*)&TC_Device_p->DMAempty_EN ) & ~0X04;
                break;

            default:
                    assert(1);  /* a cannot happen case of DirectDma not in[1,2,3] */
                break;
            }

            if (NULL != DirectDmaReturn)
                *DirectDmaReturn = DirectDma;

            stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaCDAddr[DirectDma] = (U32) TRANSLATE_PHYS_ADD( DMAParams->Destination );
            stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] = TRUE;
            stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaCompleteAddress[DirectDma] = (U32) TRANSLATE_PHYS_ADD(Write);

            if (NULL != NextPtr_p)
                *NextPtr_p = (U8 *)Write;    /* Return next free address if required */


            /* enable the dmas after values have been written in PrivateData.
             In case of very small data blocks, we don't want transfer to be over even before
             values in PrivateData are set.<GNBvd27032>*/

#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMAempty_EN, DMAempty_EN );
                WriteDMAReg32( (void*)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p );
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMAempty_EN, DMAempty_EN );
                STSYS_WriteRegDev32LE( (void*)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK );
            } STPTI_CRITICAL_SECTION_END;
#endif

            break;
        }
    }

    if (DirectDma == 4)
        Error = STPTI_ERROR_DMA_UNAVAILABLE;

    return Error;

#endif
}

/******************************************************************************
Function Name : stptiHAL_FilterSetPES
  Description : stpti_TCSetUpPESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterSetPES(FullHandle_t FilterHandle, STPTI_FilterData_t *FilterData_p)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}

/******************************************************************************
Function Name : stptiHAL_FilterSetPESStreamId
  Description : stpti_TCSetUpPESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterSetPESStreamId(FullHandle_t FilterHandle, STPTI_FilterData_t *FilterData_p)
{
	( stptiMemGet_Filter( FilterHandle ))->PESStreamIdFilterData = FilterData_p->u.PESStreamIDFilter.StreamID;

    return( ST_NO_ERROR );
}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : stptiHAL_FilterSetSection
  Description : stpti_TCSetUpSectionFilter
   Parameters :
         TODO : split 32 & 64 filters into 2 functions.
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterSetSection(FullHandle_t FilterHandle, STPTI_FilterData_t *FilterData_p)
{
    return( TcCam_FilterSetSection( FilterHandle,FilterData_p ) );
}

#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_FilterSetTransport
  Description : stpti_TCSetUpTSFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterSetTransport(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_UserDataSynchronize
  Description : stpti_TCSyncUserDirectDMA
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_UserDataSynchronize(FullHandle_t DeviceHandle, STPTI_DMAParams_t *DMAParams)
{
#if !defined ( STPTI_MULTI_DMA_SUPPORT ) || !defined( STPTI_HARDWARE_CDFIFO_SUPPORT )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#endif
    U32 DirectDma;

    for (DirectDma = 1; DirectDma < 4; ++DirectDma)
    {
        if ((stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaCDAddr[DirectDma] == (U32) TRANSLATE_PHYS_ADD( DMAParams->Destination )) &&
            (stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma]   == TRUE))
        {
            TCDevice_t    *TC_Device_p   = (TCDevice_t *)stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;
            volatile U32  *EndAddress    = &(stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaCompleteAddress[DirectDma]);
            volatile BOOL *DirectDmaUsed = &(stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma]);

            U32 LastAddress = *EndAddress;
            stpti_device_word_t *DMARead_p = NULL;

            switch (DirectDma)
            {
                case 1:
                    STPTI_CRITICAL_SECTION_BEGIN {
                        TC_Device_p->DMAempty_EN &= ~0X01;
                        DMARead_p = &(TC_Device_p->DMA1Read);
                    } STPTI_CRITICAL_SECTION_END;
                    break;

                case 2:
                    STPTI_CRITICAL_SECTION_BEGIN {
                        TC_Device_p->DMAempty_EN &= ~0X02;
                        DMARead_p = &(TC_Device_p->DMA2Read);
                    } STPTI_CRITICAL_SECTION_END;
                    break;

                case 3:
                    STPTI_CRITICAL_SECTION_BEGIN {
                        TC_Device_p->DMAempty_EN &= ~0X04;
                        DMARead_p = &(TC_Device_p->DMA3Read);
                    } STPTI_CRITICAL_SECTION_END;
                    break;

                case 0:
                default:
                    break;
            }

            do
            {
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                if (ReadDMAReg32((void *)DMARead_p, TC_Params_p) == TRANSLATE_PHYS_ADD(LastAddress))  /* We have sync'd */
#else
                if ((*DMARead_p) == (U32) TRANSLATE_PHYS_ADD(LastAddress))  /* We have sync'd */
#endif
                {
                    (*DirectDmaUsed) = FALSE;   /* Free up the DMA engine */
                    break;                      /* Quit out */
                }
                else    /* Wait for it to finish */
                {
                    /* AS 28/03/03, Calling function which can be desheduled
                       while waiting for DMA to complete */
                    stptiHelper_WaitPacketTimeDeschedule();
                }
            }

            while ((*EndAddress) == LastAddress);   /* If this has been reallocated - the original must have sync'd */

            stptiHelper_WaitPacketTime();           /* Wait for this packet to complete */
            break;                                  /* We have done it so quit the loop */
        }
    }

    if (DirectDma == 4)
        return( STPTI_ERROR_DMA_UNAVAILABLE );

    return( ST_NO_ERROR );

#endif
}


/******************************************************************************
Function Name : stptiHAL_UserDataCircularAppend
  Description : stpti_TCAppendUserDirectDMA

 Extend the existing DMA write opperation.
   Parameters :
******************************************************************************/

ST_ErrorCode_t stptiHAL_UserDataCircularAppend(FullHandle_t DeviceHandle, size_t DataLength, U8 **NextPtr_p, STPTI_DMAParams_t * DMAParams)
{
#if !defined ( STPTI_MULTI_DMA_SUPPORT ) || !defined( STPTI_HARDWARE_CDFIFO_SUPPORT )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#endif
    U32 DirectDma;
    U32 Write = 0;
    U32 Base = 0;
    U32 Top = 0;

    for (DirectDma = 1; DirectDma < 4; ++DirectDma)
    {
        if ((stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaCDAddr[DirectDma] == (U32) TRANSLATE_PHYS_ADD( DMAParams->Destination )) &&
            (stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma]   == TRUE))
        {
            TCDevice_t *TC_Device_p = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

            /* Read the DMA setup */
            switch( DirectDma )
            {
                case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    Base  = ReadDMAReg32((void *)&TC_Device_p->DMA1Base, TC_Params_p);
                    Top   = ReadDMAReg32((void *)&TC_Device_p->DMA1Top, TC_Params_p);
                    Write = ReadDMAReg32((void *)&TC_Device_p->DMA1Write, TC_Params_p);
#else
                    Base  = TC_Device_p->DMA1Base;
                    Top   = TC_Device_p->DMA1Top;
                    Write = TC_Device_p->DMA1Write;
#endif
                    break;

                case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    Base  = ReadDMAReg32((void *)&TC_Device_p->DMA2Base, TC_Params_p);
                    Top   = ReadDMAReg32((void *)&TC_Device_p->DMA2Top, TC_Params_p);
                    Write = ReadDMAReg32((void *)&TC_Device_p->DMA2Write, TC_Params_p);
#else
                    Base  = TC_Device_p->DMA2Base;
                    Top   = TC_Device_p->DMA2Top;
                    Write = TC_Device_p->DMA2Write;
#endif
                    break;

                case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    Base  = ReadDMAReg32((void *)&TC_Device_p->DMA3Base, TC_Params_p);
                    Top   = ReadDMAReg32((void *)&TC_Device_p->DMA3Top, TC_Params_p);
                    Write = ReadDMAReg32((void *)&TC_Device_p->DMA3Write, TC_Params_p);
#else
                    Base  = TC_Device_p->DMA3Base;
                    Top   = TC_Device_p->DMA3Top;
                    Write = TC_Device_p->DMA3Write;
#endif
                    break;

                case 0:
                default:
                    break;
            }

            /* Calculate the new write pointer */
            if (Top > Base)
            {
                /* Circular buffer */

                U32 Offset = (Write - Base) +DataLength;
                Write = (Base + (Offset%(Top+1-Base)));
            }
            else
            {
                /* Linear buffer */
                Write += DataLength;
            }

            /* Set the new write pointer */

            switch (DirectDma)
            {
                case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    STPTI_CRITICAL_SECTION_BEGIN {
                        WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~TC_GLOBAL_DMA_ENABLE_DMA1) &
                                                                         TC_GLOBAL_DMA_ENABLE_MASK,
                                                                         TC_Params_p);
                        WriteDMAReg32((void *)&TC_Device_p->DMA1Write, Write, TC_Params_p);
                        WriteDMAReg32((void *)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
                    } STPTI_CRITICAL_SECTION_END;
#else
                    STPTI_CRITICAL_SECTION_BEGIN {
                        TC_Device_p->DMAEnable = (~TC_GLOBAL_DMA_ENABLE_DMA1) & TC_GLOBAL_DMA_ENABLE_MASK;
                        TC_Device_p->DMA1Write = Write;
                        TC_Device_p->DMAEnable = TC_GLOBAL_DMA_ENABLE_MASK;
                    } STPTI_CRITICAL_SECTION_END;
#endif
                    break;

                case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    STPTI_CRITICAL_SECTION_BEGIN {
                        WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~TC_GLOBAL_DMA_ENABLE_DMA2) &
                                                                         TC_GLOBAL_DMA_ENABLE_MASK,
                                                                         TC_Params_p);
                        WriteDMAReg32((void *)&TC_Device_p->DMA2Write, Write, TC_Params_p);
                        WriteDMAReg32((void *)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
                    } STPTI_CRITICAL_SECTION_END;
#else
                    STPTI_CRITICAL_SECTION_BEGIN {
                        TC_Device_p->DMAEnable = (~TC_GLOBAL_DMA_ENABLE_DMA2) & TC_GLOBAL_DMA_ENABLE_MASK;
                        TC_Device_p->DMA2Write = Write;
                        TC_Device_p->DMAEnable = TC_GLOBAL_DMA_ENABLE_MASK;
                    } STPTI_CRITICAL_SECTION_END;
#endif
                    break;

                case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    STPTI_CRITICAL_SECTION_BEGIN {
                        WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~TC_GLOBAL_DMA_ENABLE_DMA3) &
                                                                         TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
                        WriteDMAReg32((void *)&TC_Device_p->DMA3Write, Write, TC_Params_p);
                        WriteDMAReg32((void *)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
                    } STPTI_CRITICAL_SECTION_END;
#else
                    STPTI_CRITICAL_SECTION_BEGIN {
                        TC_Device_p->DMAEnable = (~TC_GLOBAL_DMA_ENABLE_DMA3) & TC_GLOBAL_DMA_ENABLE_MASK;
                        TC_Device_p->DMA3Write = Write;
                        TC_Device_p->DMAEnable = TC_GLOBAL_DMA_ENABLE_MASK;
                    } STPTI_CRITICAL_SECTION_END;
#endif
                    break;

                case 0:
                default:
                    break;
            }

            stptiMemGet_Device(DeviceHandle)->TCPrivateData.TCUserDma_p->DirectDmaCompleteAddress[DirectDma] = Write;

            if (NULL != NextPtr_p) *NextPtr_p = (U8*)Write; /* Return the next free address if required */

            break;  /* We have done it so quit the loop */
        }
    }

    if (DirectDma == 4)
        return( STPTI_ERROR_DMA_UNAVAILABLE );

    return( ST_NO_ERROR );

#endif
}


/******************************************************************************
Function Name : stptiHAL_UserDataRemaining
  Description : stpti_TCUserDataRemaining
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_UserDataRemaining(FullHandle_t DeviceHandle, STPTI_DMAParams_t *DMAParams, U32 *UserDataRemaining)
{
#if !defined ( STPTI_MULTI_DMA_SUPPORT ) || !defined( STPTI_HARDWARE_CDFIFO_SUPPORT )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#endif
    U32 DirectDma = 0;
    U32 Read_p = 0;
    Device_t *Device_p = stptiMemGet_Device(DeviceHandle);
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("\nstptiHAL_UserDataRemaining\n"));
#endif

    *UserDataRemaining = 0; /* default return value */

    for (DirectDma = 1; DirectDma < 4; ++DirectDma)
    {
        /* Also check if the DMA is being used.<GNBvd27032>*/
        if (( Device_p->TCPrivateData.TCUserDma_p->DirectDmaCDAddr[DirectDma] == (U32) TRANSLATE_PHYS_ADD( DMAParams->Destination ))&&
                ( Device_p->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] == TRUE))
        {
            U32 LastAddress = Device_p->TCPrivateData.TCUserDma_p->DirectDmaCompleteAddress[DirectDma];
            TCDevice_t *TCDevice_p  = (TCDevice_t *) Device_p->TCDeviceAddress_p;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            STTBX_Print(("TCDeviceAddress_p: 0x%08x, DirectDma: %d\n", (U32)TCDevice_p, DirectDma));
#endif
            /* interrupt_lock and interrupt_unlock added in all cases <GNBvd27032>*/
            switch (DirectDma)
            {
                case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    STPTI_CRITICAL_SECTION_BEGIN {
                        Read_p = ReadDMAReg32((void *)&TCDevice_p->DMA1Read, TC_Params_p);
                        *UserDataRemaining = LastAddress - Read_p;

                        if (LastAddress < Read_p)
                            /* Read in one loop behind Write */
                            *UserDataRemaining += (ReadDMAReg32((void *)&TCDevice_p->DMA1Top, TC_Params_p) - 
                                                    ReadDMAReg32((void *)&TCDevice_p->DMA1Base, TC_Params_p)) + 1;
                    } STPTI_CRITICAL_SECTION_END;
#else
                    STPTI_CRITICAL_SECTION_BEGIN {
                        Read_p = TCDevice_p->DMA1Read;
                        *UserDataRemaining = LastAddress - Read_p;

                        if (LastAddress < Read_p)
                            /* Read in one loop behind Write */
                            *UserDataRemaining += (TCDevice_p->DMA1Top - TCDevice_p->DMA1Base) + 1;
                    } STPTI_CRITICAL_SECTION_END;
#endif
                    break;

                    case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    STPTI_CRITICAL_SECTION_BEGIN {
                        Read_p = ReadDMAReg32((void *)&TCDevice_p->DMA2Read, TC_Params_p);
                        *UserDataRemaining = LastAddress - Read_p;

                        if (LastAddress < Read_p)
                            /* Read in one loop behind Write */
                            *UserDataRemaining += (ReadDMAReg32((void *)&TCDevice_p->DMA2Top, TC_Params_p) - 
                                                    ReadDMAReg32((void *)&TCDevice_p->DMA2Base, TC_Params_p)) + 1;
                    } STPTI_CRITICAL_SECTION_END;
#else
                    STPTI_CRITICAL_SECTION_BEGIN {
                        Read_p = TCDevice_p->DMA2Read;
                        *UserDataRemaining = LastAddress - Read_p;

                        if (LastAddress < Read_p)
                            *UserDataRemaining += (TCDevice_p->DMA2Top - TCDevice_p->DMA2Base) + 1;
                    } STPTI_CRITICAL_SECTION_END;
#endif
                    break;

                case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
                    STPTI_CRITICAL_SECTION_BEGIN {
                        Read_p = ReadDMAReg32((void *)&TCDevice_p->DMA3Read, TC_Params_p);
                        *UserDataRemaining = LastAddress - Read_p;

                        if (LastAddress < Read_p)
                            /* Read in one loop behind Write */
                            *UserDataRemaining += (ReadDMAReg32((void *)&TCDevice_p->DMA3Top, TC_Params_p) - 
                                                    ReadDMAReg32((void *)&TCDevice_p->DMA3Base, TC_Params_p)) + 1;
                    } STPTI_CRITICAL_SECTION_END;
#else
                    STPTI_CRITICAL_SECTION_BEGIN {
                        Read_p = TCDevice_p->DMA3Read;
                        *UserDataRemaining = LastAddress - Read_p;

                        if (LastAddress < Read_p)
                            /* Read in one loop behind Write */
                            *UserDataRemaining += (TCDevice_p->DMA3Top - TCDevice_p->DMA3Base) + 1;
                    } STPTI_CRITICAL_SECTION_END;
#endif
                    break;
                    
                    
                case 0:
                default:
                    STTBX_Print(("DMA 0!!\n"));
                    break;
            }

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            STTBX_Print(("     Read_p:  0x%08x\n", Read_p));
            STTBX_Print(("LastAddress:  0x%08x (%d)\n", LastAddress, (LastAddress < Read_p)));
#endif
            /* break out of loop after processing the dma <GNBvd27032>*/
            break;
        }
    }

    /* send error if no DMA matches <GNBvd27032>*/
    if (DirectDma == 4)
    {
        Error = STPTI_ERROR_DMA_UNAVAILABLE;
    }

    return( Error );

#endif
}


/******************************************************************************
Function Name : stptiHAL_FilterDeallocatePES
  Description : stpti_TCTerminatePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDeallocatePES(FullHandle_t DeviceHandle, U32 FilterIdent)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_FilterDeallocatePES
  Description : stpti_TCTerminatePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDeallocatePESStreamId(FullHandle_t DeviceHandle, U32 FilterIdent)
{

    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);

    PrivateData_p->PesStreamIdFilterHandles_p[FilterIdent] = STPTI_NullHandle();

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stptiHAL_FilterDeallocateTransport
  Description : stpti_TCTerminateTSFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_FilterDeallocateTransport(FullHandle_t DeviceHandle, U32 FilterIdent)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/* -------------------------
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_BufferGetFreeLength
              HAL: 4
      description: This function is used to assess the free length remaining in a PTI buffer (the write pointer
                   is used - not the Qwrite pointer) This function is useful for flow control in pull mode.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferGetFreeLength(FullHandle_t FullBufferHandle, U32 *FreeLength_p)
{
 ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 BytesInBuffer = 0;
    /* To avoid warnings */
    U32 Read_p=0, Write_p=0, Top_p=0, Base_p=0;
    U16                   DirectDma     = stptiMemGet_Buffer(FullBufferHandle)->DirectDma;
    U16                   DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
    TCDevice_t           *TC_Device_p   = (TCDevice_t *)stptiMemGet_Device(FullBufferHandle)->TCDeviceAddress_p;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p =  TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    switch (DirectDma)
    {
        case 0:
            Read_p  =  DMAConfig_p->DMARead_p;
            Write_p =  DMAConfig_p->DMAWrite_p;
            Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
            Base_p  =  DMAConfig_p->DMABase_p;
            break;

        case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA1Read, TC_Params_p);
                Write_p =   ReadDMAReg32((void *)&TC_Device_p->DMA1Write, TC_Params_p);
                Top_p   =   ReadDMAReg32((void *)&TC_Device_p->DMA1Top, TC_Params_p);
                Base_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA1Base, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  = TC_Device_p->DMA1Read;
                Write_p = TC_Device_p->DMA1Write;
                Top_p   = TC_Device_p->DMA1Top;
                Base_p  = TC_Device_p->DMA1Base;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA2Read, TC_Params_p);
                Write_p =   ReadDMAReg32((void *)&TC_Device_p->DMA2Write, TC_Params_p);
                Top_p   =   ReadDMAReg32((void *)&TC_Device_p->DMA2Top, TC_Params_p);
                Base_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA2Base, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  = TC_Device_p->DMA2Read;
                Write_p = TC_Device_p->DMA2Write;
                Top_p   = TC_Device_p->DMA2Top;
                Base_p  = TC_Device_p->DMA2Base;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA3Read, TC_Params_p);
                Write_p =   ReadDMAReg32((void *)&TC_Device_p->DMA3Write, TC_Params_p);
                Top_p   =   ReadDMAReg32((void *)&TC_Device_p->DMA3Top, TC_Params_p);
                Base_p  =   ReadDMAReg32((void *)&TC_Device_p->DMA3Base, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                Read_p  = TC_Device_p->DMA3Read;
                Write_p = TC_Device_p->DMA3Write;
                Top_p   = TC_Device_p->DMA3Top;
                Base_p  = TC_Device_p->DMA3Base;
            } STPTI_CRITICAL_SECTION_END;
#endif
            break;

        default:
            return(ST_ERROR_BAD_PARAMETER);
    }

    BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_p);

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("stptiHAL_BufferTestForData: DirectDma: %d\n", DirectDma));
    STTBX_Print(("           Base_p: 0x%08x\n", Base_p   ));
    STTBX_Print(("         DMATop_p: 0x%08x\n", Top_p    ));
    STTBX_Print(("       DMAWrite_p: 0x%08x\n", Write_p  ));
    STTBX_Print(("        DMARead_p: 0x%08x\n", Read_p   ));
    STTBX_Print(("    BytesInBuffer: %d\n", BytesInBuffer   ));
#endif

    *FreeLength_p = (Top_p-Base_p)-BytesInBuffer;

    return( Error );

}


/******************************************************************************
Function Name : stptiHAL_BufferReadNBytes
  Description : stpti_TCReadNBytes

  A function to move n bytes from the contents of an internal buffer to a
 user-supplied buffer.

   Parameters : BufferHandle        Handle to a buffer.
                Destination0_p      Base of the linear buffer that the data will be copied into.
                DestinationSize0    Size of the linear buffer.
                Destination1_p      Base of the second buffer that will be copied into.
                                    Used if destination is a circular buffer.
                                    Set to NULL for basic linear copy.
                DestinationSize1    Size of second buffer
                DataSize_p          Amount of data actually moved
                DmaOrMemcpy         Request transfer to be performed by DMA or ST20
                BytesToCopy         Number of bytes to read from the internal circular buffer.

          NOTE : Moved here from pti_ba3.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t stptiHAL_BufferReadNBytes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                   U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy)
{
    U16              TC_DMAIdent   = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   =      &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[TC_DMAIdent];

    U32 Read_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMARead_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Write_p = TRANSLATE_LOG_ADD(DMAConfig_p->DMAQWrite_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Top_p   = TRANSLATE_LOG_ADD((DMAConfig_p->DMATop_p & ~0xf) + 16, stptiMemGet_Buffer(FullBufferHandle)->Start_p);
    U32 Base_p  = TRANSLATE_LOG_ADD(DMAConfig_p->DMABase_p, stptiMemGet_Buffer(FullBufferHandle)->Start_p);

    U32 BufferSize = Top_p - Base_p;
    U32 ReadBufferIndex = Read_p - Base_p;
    U32 BytesBeforeWrap = Top_p - Read_p;
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_p);

    U32 BytesCopied = 0;

    U32 SizeToCopy;
    U32 SrcSize0;
    U8 *Src0_p;
    U32 SrcSize1;
    U8 *Src1_p;

#if defined(ST_OSLINUX)
    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);

    Write_p = MappedStart_p + (Write_p - Base_p);

    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
#endif

    /* PTI DMA does not work so use memcpy */
    if (DmaOrMemcpy == STPTI_COPY_TRANSFER_BY_DMA)
        DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;

    SizeToCopy = (BytesInBuffer >= BytesToCopy) ? BytesToCopy : BytesInBuffer;


    if ((DestinationSize0 == 0) && (DestinationSize1 == 0))
    {
        BytesCopied = SizeToCopy;
    }
    else
    {
        SrcSize0 = MINIMUM(BytesBeforeWrap, SizeToCopy);
        Src0_p = &((U8 *) Base_p)[ReadBufferIndex];
        SrcSize1 = SizeToCopy - SrcSize0;
        Src1_p = (SrcSize1 > 0) ? (U8 *) Base_p : NULL;

        BytesCopied += stptiHelper_CircToCircCopy(SrcSize0,      /* Number useful bytes in PTIBuffer Part0 */
                                      Src0_p,        /* Start of PTIBuffer Part0 */
                                      SrcSize1,      /* Number useful bytes in PTIBuffer Part1 */
                                      Src1_p,        /* Start of PTIBuffer Part1  */
                                      &DestinationSize0, /* Space in UserBuffer0 */
                                      &Destination0_p, /* Start of UserBuffer0 */
                                      &DestinationSize1, /* Space in UserBuffer1 */
                                      &Destination1_p, /* Start of UserBuffer1 */
                                      DmaOrMemcpy, /* Copy Method */
                                      FullBufferHandle);
    }
    /* Advance Read_p to point to the end of the data that we have read */

    Read_p = (U32) &(((U8 *) Base_p)[(ReadBufferIndex + BytesCopied) % BufferSize]);

    *DataSize_p = BytesCopied;

#if defined(ST_OSLINUX)
    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, virt_to_bus((void*)Read_p) );
#else
    DMAConfig_p->DMARead_p = (U32) TRANSLATE_PHYS_ADD(Read_p);    /* Update Read pointer in DMA structure */
#endif /* #endif defined(ST_OSLINUX) */

    /* Signal or Re-enable interrupts as appropriate */
    stptiHelper_SignalEnableIfDataWaiting( FullBufferHandle, (stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff );

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_LoaderGetCapability
  Description :
   Parameters :
         NOTE : new function
******************************************************************************/
ST_ErrorCode_t stptiHAL_LoaderGetCapability(FullHandle_t DeviceHandle, STPTI_Capability_t *DeviceCapability)
{
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;


    DeviceCapability->Device          = stptiHAL_Device();
    DeviceCapability->TCDeviceAddress = (U32) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;
    DeviceCapability->Association     =       stptiMemGet_Device(DeviceHandle)->DescramblerAssociationType;

    memcpy((void *) &DeviceCapability->EventHandlerName,
           (void *) &stptiMemGet_Device(DeviceHandle)->EventHandlerName, sizeof(ST_DeviceName_t));

    DeviceCapability->NumberSlots          = TC_Params_p->TC_NumberSlots;
    DeviceCapability->NumberDMAs           = TC_Params_p->TC_NumberDMAs;
    DeviceCapability->NumberKeys           = TC_Params_p->TC_NumberDescramblerKeys;
    DeviceCapability->NumberSectionFilters = TC_Params_p->TC_NumberSectionFilters;

#ifdef STPTI_CAROUSEL_SUPPORT
    if (TC_Params_p->TC_NumberCarousels > 0) DeviceCapability->CarouselOutputSupport = TRUE;
#endif

    return ( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_SlotLinkToCdFifo
  Description : stpti_TCSetUpSlotDirectDMA
   Parameters :
         NOTE : moved here from pti_ba3.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotLinkToCdFifo(FullHandle_t SlotHandle)
{
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U16                   slot          = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;

    U32 dma;


    for (dma = 0; dma < TC_Params_p->TC_NumberDMAs; ++dma)
    {
        if (PrivateData_p->BufferHandles_p[dma] == STPTI_NullHandle())
            break;
    }

    if (dma < TC_Params_p->TC_NumberDMAs)
    {
        TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[dma];
        U32 base = (U32) stptiMemGet_Slot(SlotHandle)->CDFifoAddr;
        U32 top  = 0;

        DMAConfig_p->DMABase_p   = base;
        DMAConfig_p->DMATop_p    = top;
        DMAConfig_p->DMARead_p   = base;
        DMAConfig_p->DMAWrite_p  = base;
        DMAConfig_p->DMAQWrite_p = base;
        DMAConfig_p->BufferPacketCount = 0;

        /* Clear quantise flags to stop signalling */

        STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, (TC_DMA_CONFIG_SIGNAL_MODE_TYPE_EVERY_TS | TC_DMA_CONFIG_SIGNAL_MODE_TYPE_QUANTISATION));
        STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
        TcHal_MainInfoAssociateDmaWithSlot(TC_Params_p,slot,dma);
        stptiMemGet_Slot(SlotHandle)->TC_DMAIdent = dma;
        PrivateData_p->BufferHandles_p[dma] = SlotHandle.word;

        return ( ST_NO_ERROR );
    }

    return ( STPTI_ERROR_NO_FREE_DMAS );
}


/******************************************************************************
Function Name : stptiHAL_HardwareFIFOLevels
  Description :
   Parameters :
         NOTE :
******************************************************************************/
ST_ErrorCode_t stptiHAL_HardwareFIFOLevels(FullHandle_t DeviceHandle, BOOL *Overflow, U16 *InputLevel, U16 *AltLevel, U16 *HeaderLevel)
{
    U32 Level;
    TCDevice_t *TC_Device = (TCDevice_t *) stptiMemGet_Device(DeviceHandle)->TCDeviceAddress_p;

    Level = TC_Device->IIFFIFOCount;

    if (NULL != Overflow)
        *Overflow = (Level&0X0080)?(TRUE):(FALSE);

    if (NULL != InputLevel)
        *InputLevel = (Level&0X007F);

    Level = TC_Device->IIFAltFIFOCount;

    if (NULL != AltLevel)
        *AltLevel = (Level&0X00FF);

    if (NULL != HeaderLevel)
        *HeaderLevel = 65535;   /* TODO */

    return ( ST_NO_ERROR );

}


/******************************************************************************
Function Name : stptiHAL_PrintDebug
  Description : Printout PTI debug information
   Parameters :
   
   To use this function define STTBX_PRINT at the top of this file (you may wish
   to avoid defining STPTI_INTERNAL_DEBUG_SUPPORT to prevent other prints from 
   being displayed).
   
   Alternatively define here, STTBX_PRINT and also STTBX_Print with your printf 
   mechanism.
   
******************************************************************************/
ST_ErrorCode_t stptiHAL_PrintDebug(FullHandle_t DeviceHandle)
{
#if defined( STTBX_PRINT )
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;
    TCDevice_t           *hw_p           = (TCDevice_t *)Device_p->TCDeviceAddress_p;
    U32 i, iptr;
    U16 *tcram_buffer = NULL;

    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];

    STTBX_Print(("STPTI4 Device Address  0x%08x\n\n",(unsigned)hw_p));

    /* Printout TC REGISTERS (useful if TC has stalled */
    STTBX_Print((" A:0x%04x ", hw_p->TCRegA ));
    STTBX_Print((" B:0x%04x ", hw_p->TCRegB ));
    STTBX_Print((" C:0x%04x ", hw_p->TCRegC ));
    STTBX_Print((" D:0x%04x ", hw_p->TCRegD ));
    STTBX_Print((" P:0x%04x ", hw_p->TCRegP ));
    STTBX_Print((" Q:0x%04x ", hw_p->TCRegQ ));
    STTBX_Print((" I:0x%04x ", hw_p->TCRegI ));
    STTBX_Print((" O:0x%04x ", hw_p->TCRegO ));
    STTBX_Print(("\n"));
    STTBX_Print(("E0:0x%04x ", hw_p->TCRegE0 ));
    STTBX_Print(("E1:0x%04x ", hw_p->TCRegE1 ));
    STTBX_Print(("E2:0x%04x ", hw_p->TCRegE2 ));
    STTBX_Print(("E3:0x%04x ", hw_p->TCRegE3 ));
    STTBX_Print(("E4:0x%04x ", hw_p->TCRegE4 ));
    STTBX_Print(("E5:0x%04x ", hw_p->TCRegE5 ));
    STTBX_Print(("E6:0x%04x ", hw_p->TCRegE6 ));
    STTBX_Print(("E7:0x%04x ", hw_p->TCRegE7 ));
    {
        iptr=hw_p->TCIPtr;
        STTBX_Print(( "IPTR:0x%04x", iptr ));
        for(i=8;i>0;i--)
        {
            if( hw_p->TCIPtr != iptr ) break;
        }
        if(i==0)
        {
            STTBX_Print(("(Stalled!) "));
        }
        else
        {
            STTBX_Print(("(FreeRunning) "));
        }
    }
    STTBX_Print(("\n"));

    /* TC Memory Dumps */
    tcram_buffer=STOS_MemoryAllocate( stptiMemGet_Session( DeviceHandle )->DriverPartition_p, TC_DATA_RAM_SIZE );
    if( tcram_buffer != NULL )
    {
        /* Printout TC DATA RAM  */
        memcpy(tcram_buffer, hw_p->TC_Data, TC_DATA_RAM_SIZE);
        for(i=0;i<TC_DATA_RAM_SIZE/sizeof(U16);i++)
        {
            if(i%16==0) STTBX_Print(("\n#%08x  ", (unsigned int) &(hw_p->TC_Data[i/2]) ));
            STTBX_Print(("%04x ", tcram_buffer[i]));
        }
        STTBX_Print(("\n"));

        STOS_MemoryDeallocate( stptiMemGet_Session( DeviceHandle )->DriverPartition_p, tcram_buffer );
    }
    else
    {
        return ( ST_ERROR_NO_MEMORY );
    }
#endif  /* #if defined(STTBX_PRINT) ... #endif */
    
    return ( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DumpInputTS
  Description : Provide facilities for dumping the raw input Transport Stream
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DumpInputTS(FullHandle_t FullBufferHandle, U16 bytes_to_capture_per_packet)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    Buffer_t             *BufferStruct_p = stptiMemGet_Buffer(FullBufferHandle);
    TCPrivateData_t      *PrivateData_p  = stptiMemGet_PrivateData(FullBufferHandle);
    STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCGlobalInfo_t       *TC_Global_p    = TcHal_GetGlobalInfo( TC_Params_p );
    U32                  dma;
    
    dma = BufferStruct_p->TC_DMAIdent;

    if( dma == UNININITIALISED_VALUE )
    {
        /* Find a free DMA */
        for (dma = 0; dma < TC_Params_p->TC_NumberDMAs; ++dma)
        {
            if (PrivateData_p->BufferHandles_p[dma] == STPTI_NullHandle())
                break;
        }
    }

    if (dma < TC_Params_p->TC_NumberDMAs)
    {
        TCDMAConfig_t *DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, dma );

        U32 base = (U32) TRANSLATE_PHYS_ADD(BufferStruct_p->Start_p);
        U32 top  = (U32) (base + BufferStruct_p->ActualSize);

        STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMABase_p, base );
        STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMATop_p, (top - 1) & ~0xf );
        STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMARead_p, base );
        STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAWrite_p, base );
        STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->DMAQWrite_p, base );
        STSYS_WriteRegDev32LE( (void*)&DMAConfig_p->BufferPacketCount, 0 );

        /*Reset SignalModeFlags as this could have been a previously used DMA and the flags may be in an
        un-defined state */
        STSYS_WriteTCReg16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_TYPE_NO_SIGNAL );       
        STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, ( BufferStruct_p->MultiPacketSize ) & 0xffff );

        BufferStruct_p->TC_DMAIdent = dma;
        PrivateData_p->BufferHandles_p[dma] = FullBufferHandle.word;

        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalTSDumpCount, bytes_to_capture_per_packet );
        
        /* Last write should be GlobalTSDumpDMA_p as this starts the dump */
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalTSDumpDMA_p, (U32) DMAConfig_p );       
    }
    else
    {
        Error = STPTI_ERROR_NO_FREE_DMAS;
    }
   
    return Error;
}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* EOF  -------------------------------------------------------------------- */

