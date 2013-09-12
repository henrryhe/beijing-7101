/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: dvb.c
 Description: Provide dvb part of STPTI API

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#include <assert.h>
#endif /* #if !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"
#include "memget.h"


/*
 * We need at least the first three bytes in the section in order to extract
 * a valid section length
 */
#define SectionMinLength  3
#define CRCStateSize 1

#if !defined ( STPTI_BSL_SUPPORT )
static void TC_ConvertPTSToTimestamp(STPTI_TimeStamp_t * TimeStamp, U8 *PesRaw);
#endif
/* session.c */
extern TCSessionInfo_t *stptiHAL_GetSessionFromFullHandle(FullHandle_t DeviceHandle);

#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : TC_ConvertPTSToTimestamp
  Description :
   Parameters :
******************************************************************************/
static void TC_ConvertPTSToTimestamp(STPTI_TimeStamp_t * TimeStamp, U8 *PesRaw)
{
    U32 LSW;
    U8  Bit32;

    LSW  =  ((PesRaw[0] & 0x06) << 29);
    LSW |=   (PesRaw[1] << 22);
    LSW |=  ((PesRaw[2] & 0xfe) << 14);
    LSW |=   (PesRaw[3] << 7);
    LSW |=  ((PesRaw[4] & 0xfe) >> 1);

    Bit32 = ((PesRaw[0] & 0x08) >> 3);

    TimeStamp->LSW = LSW;
    TimeStamp->Bit32 = Bit32;
}


/******************************************************************************
Function Name : stptiHAL_DVBSlotDescramblingControl
  Description : stpti_TCSlotDescramblingControl
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBSlotDescramblingControl( FullHandle_t SlotHandle,
                                                    BOOL status)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32 SlotIdent = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t *MainInfo_p = &((TCMainInfo_t *) TC_Params_p->TC_MainInfoStart)[SlotIdent];

    if ( status )
    {
                STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_MAIN_INFO_SLOT_MODE_IGNORE_SCRAMBLING) );
    }
    else
    {
                STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_MAIN_INFO_SLOT_MODE_IGNORE_SCRAMBLING) );
    }
    
    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name :stptiHAL_DVBSlotSetAlternateOutputAction
  Description : stpti_TCSetSlotAlternateOutputAction
   Parameters :
******************************************************************************/
ST_ErrorCode_t
stptiHAL_DVBSlotSetAlternateOutputAction( FullHandle_t SlotHandle,
                                          STPTI_AlternateOutputType_t Method )
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    U32 SlotIdent = stptiMemGet_Slot(SlotHandle)->TC_SlotIdent;

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t *MainInfo_p = &((TCMainInfo_t *) TC_Params_p->TC_MainInfoStart)[SlotIdent];

    STSYS_ClearTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_FIELD));

    if (Method == STPTI_ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS)
    {
        STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode, (TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_CLEAR));
    }
    if (Method == STPTI_ALTERNATE_OUTPUT_TYPE_DESCRAMBLED)
    {
	STSYS_SetTCMask16LE((void*)&MainInfo_p->SlotMode,(TC_MAIN_INFO_SLOT_MODE_ALTERNATE_OUTPUT_DESCRAMBLED));
    }
    
    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name :stptiHAL_DVBBufferExtractPartialPesPacketData
  Description : stpti_TCExtractPartialPESData
   Parameters :
******************************************************************************/
ST_ErrorCode_t
stptiHAL_DVBBufferExtractPartialPesPacketData( FullHandle_t FullBufferHandle,
                                               BOOL *PayloadUnitStart,
                                               BOOL *CCDiscontinuity,
                                               U8 *ContinuityCount,
                                               U16 *DataLength )
{
    U16 DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_p  = DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_p;
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p
                                            : (Top_p - Read_p) + (Write_p - Base_p);
    U32 ReadBufferIndex = Read_p - Base_p;

    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);
    Write_p = MappedStart_p + (Write_p - Base_p);
    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
    
    if (BytesInBuffer < 2)
    {
        return( STPTI_ERROR_NO_PACKET );
    }
    
    /* Invalidate the cache of any data we are about to read */
    stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, 2 );
        
    *PayloadUnitStart = (*(U8 *) Read_p & PAYLOAD_UNIT_START) != 0;
    *CCDiscontinuity  = (*(U8 *) Read_p & DISCONTINUITY_INDICATOR) != 0;
    *ContinuityCount  = (*(U8 *) Read_p & CC_FIELD);
    *DataLength       =  ((U8 *) Base_p)[(ReadBufferIndex + 1) % BufferSize];

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DVBBufferExtractPesPacketData
  Description : stpti_TCExtractPESData
   Parameters :
******************************************************************************/
ST_ErrorCode_t
stptiHAL_DVBBufferExtractPesPacketData( FullHandle_t FullBufferHandle,
                                        U8 *PesFlags_p,
                                        U8 *TrickModeFlags_p,
                                        U32 *PacketLength,
                                        STPTI_TimeStamp_t *PTSValue,
                                        STPTI_TimeStamp_t *DTSValue)
{
    U16 DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_p  = DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_p;
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p
                                            : (Top_p - Read_p) + (Write_p - Base_p);
    U32 ReadBufferIndex = Read_p - Base_p;

    U8  PTS_DTS_Flags;
    U8  pts[5], dts[5], TrickModeFlag;
    U8  ESCRFlag, ESRateFlag;
    int TrickModeFlagIndex = 0;

    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);
    Write_p = MappedStart_p + (Write_p - Base_p);
    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
    
    if (BytesInBuffer == 0)
    {
        return( STPTI_ERROR_NO_PACKET );
    }
    if (BytesInBuffer < 6)
    {
        return( STPTI_ERROR_INCOMPLETE_PES_IN_BUFFER );
    }
    
    /* Invalidate cache for the next 19 bytes */
    stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, 19 );
    
    /* check for PES header signature */
    if ( (((U8 *) Base_p)[(ReadBufferIndex + 0) % BufferSize] != 0) ||
         (((U8 *) Base_p)[(ReadBufferIndex + 1) % BufferSize] != 0) ||
         (((U8 *) Base_p)[(ReadBufferIndex + 2) % BufferSize] != 1) )
    {
        return( STPTI_ERROR_CORRUPT_DATA_IN_BUFFER );
    }

    *PacketLength = (((U8 *) Base_p)[(ReadBufferIndex + 4) % BufferSize] << 8) |
                    (((U8 *) Base_p)[(ReadBufferIndex + 5) % BufferSize]);

    *PesFlags_p = ((U8 *) Base_p)[(ReadBufferIndex + 7) % BufferSize];

    *TrickModeFlags_p = 0;

    PTS_DTS_Flags = ((U8 *) Base_p)[(ReadBufferIndex + 7) % BufferSize] & 0xc0;

    switch (PTS_DTS_Flags)
    {
        case 0xc0:
            memcpy(dts, &((U8 *) Base_p)[(ReadBufferIndex + 14) % BufferSize], 5);
            TC_ConvertPTSToTimestamp(DTSValue, dts);
            TrickModeFlagIndex += 5;    /* Fall through to do PTS */

        case 0x80:
            memcpy(pts, &((U8 *) Base_p)[(ReadBufferIndex + 9) % BufferSize], 5);
            TC_ConvertPTSToTimestamp(PTSValue, pts);
            TrickModeFlagIndex += 5;
            break;

        default:
            break;
    }

    /* Determine if Trickmode flags are present */
    TrickModeFlag = ((U8 *) Base_p)[(ReadBufferIndex + 7) % BufferSize] & 0x08;

    if (TrickModeFlag != 0)
    {
        ESCRFlag = ((U8 *) Base_p)[(ReadBufferIndex + 7) % BufferSize] & 0x20;

        if (ESCRFlag != 0)
        {
            TrickModeFlagIndex += 6;
            ESRateFlag = ((U8 *) Base_p)[(ReadBufferIndex + 7) % BufferSize] & 0x10;

            if (ESRateFlag != 0)
            {
                TrickModeFlagIndex += 3;
            }
        }

        *TrickModeFlags_p = ((U8 *) Base_p)[(ReadBufferIndex + 9 + TrickModeFlagIndex) % BufferSize];
    }

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DVBBufferReadPartialPesPacket
  Description : stpti_TCReadPartialPESData
   Parameters :
******************************************************************************/
ST_ErrorCode_t
stptiHAL_DVBBufferReadPartialPesPacket( FullHandle_t FullBufferHandle,
                                        U8 *Destination0_p,
                                        U32 DestinationSize0,
                                        U8 *Destination1_p,
                                        U32 DestinationSize1,
                                        BOOL *PayloadUnitStart_p,
                                        BOOL *CCDiscontinuity_p,
                                        U8 *ContinuityCount_p,
                                        U32 *DataSize_p,
                                        STPTI_Copy_t DmaOrMemcpy )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U16 DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_p  = DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_p;
    U16 Packet = 0;
    U32 BytesCopied;

    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);
    Write_p = MappedStart_p + (Write_p - Base_p);
    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
    
    assert(NULL != DataSize_p);

    /* PTI DMA does not work so use memcpy */
    if (DmaOrMemcpy == STPTI_COPY_TRANSFER_BY_DMA)
    {
        DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;
    }

    BytesCopied = 0;
    *DataSize_p = 0;    /* default */

    if (Read_p == Write_p)
    {
        return( STPTI_ERROR_NO_PACKET );
    }

    for (Packet = 0; Packet < ((stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff); Packet++)
    {
        /* (Re)Calculate Buffer Parameters based on Read_p */
        U32 ReadBufferIndex = Read_p - Base_p;
        U32 BytesBeforeWrap = Top_p - Read_p;
        U32 BytesInBuffer   = (Write_p >= Read_p) ? Write_p - Read_p
                                                  : (Top_p - Read_p) + (Write_p - Base_p);

        U8  DataLength;
        U32 SizeToCopy;
        U32 SrcSize0;
        U8 *Src0_p;
        U32 SrcSize1;
        U8 *Src1_p;

        if (BytesInBuffer < 2)
        {
            Error = STPTI_ERROR_NO_PACKET;
            break;  /* for(Packet) */
        }

        /* Invalidate the cache of any data we are about to read */
        stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, 2 );

        /* deal with pre-payload meta data, only return the first */

        if (Packet == 0)
        {
            *PayloadUnitStart_p = ((*(U8 *) Read_p & PAYLOAD_UNIT_START)      != 0) ? TRUE : FALSE;
            *CCDiscontinuity_p  = ((*(U8 *) Read_p & DISCONTINUITY_INDICATOR) != 0) ? TRUE : FALSE;
            *ContinuityCount_p  =  (*(U8 *) Read_p & CC_FIELD);
        }

        DataLength = ((U8 *) Base_p)[(ReadBufferIndex + 1) % BufferSize];

        if (BytesInBuffer < DataLength + 2)
        {
            Error = STPTI_ERROR_NO_PACKET;
            break; /* for(Packet) */
        }

        /* To get here we must have a valid packet in the buffer, copy it to the user buffer(s) */

        SizeToCopy = DataLength;
        stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, SizeToCopy );

        if (BytesBeforeWrap > 2)
        {
            SrcSize0 = MINIMUM (BytesBeforeWrap - 2, SizeToCopy);
            Src0_p   = &(((U8 *) Base_p)[(ReadBufferIndex + 2) % BufferSize]);
            SrcSize1 = (SizeToCopy - SrcSize0);
            Src1_p   = (SrcSize1 > 0) ? (U8 *) Base_p : NULL;
        }
        else
        {
            SrcSize0 = SizeToCopy;
            Src0_p   = &(((U8 *) Base_p)[(ReadBufferIndex + 2) % BufferSize]);
            SrcSize1 = 0;
            Src1_p   = NULL;
        }

        BytesCopied += stptiHelper_CircToCircCopy (SrcSize0, /* Number useful bytes in PTIBuffer Part0 */
                                       Src0_p,               /* Start of PTIBuffer Part0               */
                                       SrcSize1,             /* Number useful bytes in PTIBuffer Part1 */
                                       Src1_p,               /* Start of PTIBuffer Part1               */
                                       &DestinationSize0,    /* Space in UserBuffer0                   */
                                       &Destination0_p,      /* Start of UserBuffer0                   */
                                       &DestinationSize1,    /* Space in UserBuffer1                   */
                                       &Destination1_p,      /* Start of UserBuffer1                   */
                                       DmaOrMemcpy,          /* Copy Method                            */
                                       FullBufferHandle);

        /* Advance Read_p to point to next packet */
        Read_p = (U32) & (((U8 *) Base_p)[(ReadBufferIndex + DataLength + 2) % BufferSize]);

    }   /* for(Packet) */

    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, (Read_p-MappedStart_p) + DMAConfig_p->DMABase_p );
    
    stptiHelper_SignalEnable(FullBufferHandle, Packet); /* Signal or Re-enable interrupts as appropriate */

    *DataSize_p = BytesCopied;

    return( Error );
}


/******************************************************************************
Function Name : stptiHAL_DVBBufferReadPes
  Description : stpti_TCReadPES
   Parameters :
******************************************************************************/
ST_ErrorCode_t
stptiHAL_DVBBufferReadPes( FullHandle_t FullBufferHandle,
                           U8 *Destination0_p,
                           U32 DestinationSize0,
                           U8 *Destination1_p,
                           U32 DestinationSize1,
                           U32 *DataSize_p,
                           STPTI_Copy_t DmaOrMemcpy )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U16 DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *) TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_p  = DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_p;
    size_t BytesCopied;
    U16 Packet;

    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);

    Write_p = MappedStart_p + (Write_p - Base_p);

    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;
        
    BytesCopied = 0;
    *DataSize_p = 0;

    /* PTI DMA does not work so use memcpy */
    if (DmaOrMemcpy == STPTI_COPY_TRANSFER_BY_DMA)
    {
        DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;
    }

    if (Read_p == Write_p)
    {
        return( STPTI_ERROR_NO_PACKET );
    }

    for (Packet = 0; Packet < ((stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff); ++Packet)
    {
        /* (Re)Calculate Buffer Parameters based on Read_p */

        U32 ReadBufferIndex = Read_p - Base_p;
        U32 BytesBeforeWrap = Top_p - Read_p;
        U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p
                                                : (Top_p - Read_p) + (Write_p - Base_p);

        /* There is no pre-payload metadata to deal with */
        if (BytesInBuffer == 0)
        {
            break;          /* Emptied the buffer so no more PES packets */
        }
        else if (BytesInBuffer < 6)
        {
            return STPTI_ERROR_CORRUPT_DATA_IN_BUFFER;
        }
        else
        {
            /* Invalidate the cache of any data we are about to read */
            stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, 6 );
            
            /* Check we have a valid PES header */
            if ( (((U8 *) Base_p)[(ReadBufferIndex + 0) % BufferSize] != 0) ||
                 (((U8 *) Base_p)[(ReadBufferIndex + 1) % BufferSize] != 0) ||
                 (((U8 *) Base_p)[(ReadBufferIndex + 2) % BufferSize] != 1) )
            {
                Error = STPTI_ERROR_CORRUPT_DATA_IN_BUFFER;
            }
            else
            {
                /* PMC 09/04/02 Changed from U16 to U32 as indeterminate length PES can exceed 16 bit sizes. */
                U32 PacketSize;

                /* Get PES packet length */

                PacketSize = (((U8 *) Base_p)[(ReadBufferIndex + 4) % BufferSize] << 8) |
                             (((U8 *) Base_p)[(ReadBufferIndex + 5) % BufferSize]);
                
                if (PacketSize == 0)
                {
                    U32 End_p = Write_p;
                    S32 EndIndex = ReadBufferIndex;
                    
                    /* We have a non-determiniastic packet calculate size using the post-payload metadata */
                    /* Will always enter this loop once because End_p != Read_p at first iteration) */
                    while (End_p != Read_p)
                    {
                        EndIndex = End_p - Base_p - 4;

                        if( (End_p - Base_p) < 4)
                        {
                            EndIndex += BufferSize;
                        }
                        
                        if (EndIndex < BufferSize)
                        {                            
                            /* Invalidate the cache of any data we are about to read */
                            stpti_InvalidateRegionCircular(Base_p, Top_p, Base_p + (EndIndex % BufferSize), 6 );
                            
                            End_p =  (((U8 *) Base_p)[(EndIndex + 3) % BufferSize] << 24) |
                                                        (((U8 *) Base_p)[(EndIndex + 2) % BufferSize] << 16) |
                                                        (((U8 *) Base_p)[(EndIndex + 1) % BufferSize] <<  8) |
                                     (((U8 *) Base_p)[ EndIndex      % BufferSize]);

                            /* Need to calculate as a virtual address */
                            End_p = Base_p + (End_p - DMAConfig_p->DMABase_p);
                        }
                        else
                        {
                            Error = STPTI_ERROR_CORRUPT_DATA_IN_BUFFER;
                            break;
                        }
                    }

                    if (Error == ST_NO_ERROR)
                    {
                        PacketSize = EndIndex - ReadBufferIndex;
                        if (EndIndex < ReadBufferIndex)
                        {
                            PacketSize += BufferSize;
                        }
                    }
                }
                else    /* Packet is determiniastic ( PacketSize != 0) */
                {
                    PacketSize += 6;
                }

                if (Error == ST_NO_ERROR)
                {
                    if (BytesInBuffer < PacketSize + 4)
                    {
                        Error = STPTI_ERROR_INCOMPLETE_PES_IN_BUFFER;
                    }
                    else
                    {
                        /* To get here we must have a valid packet in the buffer, copy it to the user buffer(s) */

                        U32 SizeToCopy;
                        U32 SrcSize0;
                        U8 *Src0_p;
                        U32 SrcSize1;
                        U8 *Src1_p;

                        SizeToCopy = PacketSize;

                        /* Invalidate the cache of any data we are about to read */
                        stpti_InvalidateRegionCircular(Base_p, Top_p, (U32) &((U8 *) Base_p)[ReadBufferIndex], SizeToCopy );

                        SrcSize0 = MINIMUM (BytesBeforeWrap, SizeToCopy);
                        Src0_p = &((U8 *) Base_p)[ReadBufferIndex];
                        SrcSize1 = (SizeToCopy == SrcSize0) ? 0 : SizeToCopy - SrcSize0;
                        Src1_p = (SrcSize1 > 0) ? (U8 *) Base_p : NULL;
                        
                        BytesCopied += stptiHelper_CircToCircCopy (SrcSize0, /* Number useful bytes in PTIBuffer Part0 */
                                                       Src0_p,               /* Start of PTIBuffer Part0 */
                                                       SrcSize1,             /* Number useful bytes in PTIBuffer Part1 */
                                                       Src1_p,               /* Start of PTIBuffer Part1  */
                                                       &DestinationSize0,    /* Space in UserBuffer0 */
                                                       &Destination0_p,      /* Start of UserBuffer0 */
                                                       &DestinationSize1,    /* Space in UserBuffer1 */
                                                       &Destination1_p,      /* Start of UserBuffer1 */
                                                       DmaOrMemcpy,          /* Copy Method */
                                                       FullBufferHandle);
                        
                        /* Advance Read_p to point to next packet */
                        Read_p = (U32) & (((U8 *) Base_p)[(ReadBufferIndex + PacketSize + 4) % BufferSize]);                  
                    }
                }
            }
        }
    }

    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, DMAConfig_p->DMABase_p + (Read_p - Base_p));
    
    stptiHelper_SignalEnable(FullBufferHandle, Packet); /* Signal or Re-enable interrupts as appropriate */

    *DataSize_p = BytesCopied;

    return( Error );
}

/******************************************************************************
Function Name : stptiHAL_DVBBufferExtractSectionData
  Description : stpti_TCExtractSectionData
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBBufferExtractSectionData( FullHandle_t FullBufferHandle,
                                                     STPTI_Filter_t MatchedFilterList[],
                                                     U16 MaxLengthofFilterList,
                                                     U16 *NumOfFilterMatches_p,
                                                     BOOL *CRCValid_p,
                                                     U32 *SectionHeader_p )
{
    U16 DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_p  = DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_p;
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p
                                            : (Top_p - Read_p) + (Write_p - Base_p);

    size_t FilterMaskSize;
    int i;

    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);

    Write_p = MappedStart_p + (Write_p - Base_p);

    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;

    /*
     * The "automatic" section filter adds metadata before and after the section in the buffer...
     *
     * For PTI4 (5528)
     *   The first 8bytes are the 64bits of matched result (sf_match_result) CAMA, then CAMB.
     *   The section then follows (including its header)
     *   The last byte is the CRC.
     *
     * For PTI4L (5100, 5301, 7100) and PTI4SL (7109, 5525)
     *   The first 12bytes are the 96bits of matched result (sf_match_result) CAMA, then CAMB.
     *   The section then follows (including its header)
     *   The last byte is the CRC.
     */
    
    if (TC_Params_p->TC_AutomaticSectionFiltering != TRUE)
    {
        FilterMaskSize = TcCam_GetMetadataSizeInBytes();
    }
    else
    {
        FilterMaskSize = 0;
    }
    
    /* Make sure Early return signals returns appropriate values */
    *CRCValid_p = FALSE;
    *SectionHeader_p = 0;
    *NumOfFilterMatches_p = 0;
    
    /*
     * Check that we have at least the minimum bytes in the buffer to determine
     * section length
     */
    if (BytesInBuffer >= (SectionMinLength + FilterMaskSize + CRCStateSize))
    {
        U32 ReadBufferIndex = Read_p - Base_p;
        size_t SectionLength = 3;

        /* Invalidate the cache for data we are about to read */
        stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, (SectionMinLength + FilterMaskSize + CRCStateSize) );

        /* We know we can recover the section length legally - because we have enough bytes */
        SectionLength += ((((U8 *) Base_p)[(ReadBufferIndex + 1 + FilterMaskSize) % BufferSize] & 0x0f) << 8);
        SectionLength +=   ((U8 *) Base_p)[(ReadBufferIndex + 2 + FilterMaskSize) % BufferSize];

        /*
         * Check that we have at least the minimum bytes in the buffer to hold
         * the section and its associated data
         */
        if (BytesInBuffer >= (SectionLength + FilterMaskSize + CRCStateSize))
        {
            U8     MetaData[16] = { 0 };

            /* Extract MetaData from the buffer */
            for(i=0;i<FilterMaskSize;i++)
            {
                MetaData[i] = ((U8 *) Base_p) [(ReadBufferIndex + i) % BufferSize];
            }

            /* Invalidated the cache for data we are about to read */
            stpti_InvalidateRegionCircular(Base_p, Top_p, Base_p + ((ReadBufferIndex + SectionLength + FilterMaskSize) % BufferSize), 1 );

            /* Load CRC state, NOTE: in PTI3 SSI bit is accounted for in TC */
            /* Does this Section have a CRC error */
            if ((((U8 *) Base_p)[(ReadBufferIndex + SectionLength + FilterMaskSize) % BufferSize] == 0))
            {
                /* Report the lack of a CRC Error */
                *CRCValid_p = TRUE;
            }
            /* else *CRCValid_p is already FALSE */

            /* Load Section Header */
            *SectionHeader_p  = ((((U8 *) Base_p)[(ReadBufferIndex + 0 + FilterMaskSize) % BufferSize]) << 16);
            *SectionHeader_p |= ((((U8 *) Base_p)[(ReadBufferIndex + 1 + FilterMaskSize) % BufferSize]) << 8);
            *SectionHeader_p |=   ((U8 *) Base_p)[(ReadBufferIndex + 2 + FilterMaskSize) % BufferSize];

            {
                U32 NumOfFilterMatches32;

                /* Convert bit-map to a list of filters */
                stptiHAL_MaskConvertToList( MetaData,
                                            MatchedFilterList,
                                            MaxLengthofFilterList,
                                            *CRCValid_p,
                                            &NumOfFilterMatches32,
                                            PrivateData_p,
                                            FullBufferHandle,
                                            FALSE );
                *NumOfFilterMatches_p = NumOfFilterMatches32;
            }
        }
        else
        {
            Error = ( STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER );
        }
    }
    else
    {
        Error = ((BytesInBuffer == 0) ? STPTI_ERROR_NO_PACKET
                                      : STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER);
    }

    return Error;
}


/******************************************************************************
Function Name : stptiHAL_DVBFiltersFlush
  Description : stpti_TCFlushFilters
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFiltersFlush(FullHandle_t FullBufferHandle, STPTI_Filter_t FilterHandles[], U32 NumberOfFilters)
{
     ST_ErrorCode_t Error = ST_NO_ERROR;

    U16                   DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   =      &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p   = DMAConfig_p->DMARead_p;
    U32 Write_p  = DMAConfig_p->DMAWrite_p;
    U32 QWrite_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p    = (DMAConfig_p->DMATop_p & ~0X0F)+ 16;
    U32 Base_p   = DMAConfig_p->DMABase_p;

    U32 BufferSize = Top_p - Base_p;

    size_t SectionLength=0;
    size_t FilterMaskSize = TcCam_GetMetadataSizeInBytes();

    U32 filter, i;
    U32 FilterIdent;
    U8  FilterMask[32] = {0};                   /* Array needs to be the same size or larger than TcCam_GetMetadataSizeInBytes() */
    FullHandle_t FullFilterHandle;

    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_p);

    Write_p = MappedStart_p + (Write_p - Base_p);

    QWrite_p = MappedStart_p + (QWrite_p - Base_p);
    
    Top_p = MappedStart_p + (Top_p - Base_p);
    Base_p = MappedStart_p;

    
    for (filter = 0; (filter < NumberOfFilters); ++filter)
    {
        FullFilterHandle.word = FilterHandles[filter];
        FilterIdent = TcCam_RelativeMetaBitMaskPosition(FullFilterHandle);
        FilterMask[FilterIdent / 8] |= (1 << (FilterIdent % 8));
    }

    while (Read_p != QWrite_p)
    {
        U32 BytesInBuffer   = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_p);
        U32 ReadBufferIndex = Read_p - Base_p;

        /* Invalidated the cache for data we are about to read (FilterMaskSize and 3 bytes of section header) */
        stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, FilterMaskSize+3 );

        /* This function only strips for the last filter! */

        SectionLength  = ((((U8 *) Base_p)[( ReadBufferIndex + 1 +FilterMaskSize ) % BufferSize] & 0x0f) << 8);
        SectionLength +=   ((U8 *) Base_p)[( ReadBufferIndex + 2 +FilterMaskSize ) % BufferSize];
        SectionLength += 3;

        if (TC_Params_p->TC_AutomaticSectionFiltering != TRUE)
        {
            for(i=0;i<FilterMaskSize;i++)
            {
                ((U8 *) Base_p)[(ReadBufferIndex + i) % BufferSize] &= ~FilterMask[i];
            }

            /* Flush the cache for data we have written */
            stpti_FlushRegionCircular(Base_p, Top_p, (U32) &((U8 *) Base_p)[ReadBufferIndex % BufferSize], FilterMaskSize );
        }

        if (BytesInBuffer < (SectionLength + FilterMaskSize + CRCStateSize))
        {
            STTBX_Print(("%d < %d + %d + %d\n", BytesInBuffer, SectionLength, FilterMaskSize, CRCStateSize));
            Error = STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER;
            break;
        }
        else
        {
            Read_p = Base_p + ((ReadBufferIndex + SectionLength + FilterMaskSize + CRCStateSize) % BufferSize);
        }
    }

    if (Write_p != QWrite_p)
    {
        U32 BytesInBuffer = (QWrite_p >= Read_p) ? QWrite_p - Read_p : (Top_p - Read_p) + (QWrite_p - Base_p);
        U32 ReadBufferIndex = Read_p - Base_p + SectionLength;

        /* Invalidate the cache for data we are about to read (FilterMaskSize and 3 bytes of section header) */
        stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, FilterMaskSize+3 );

        if ((BytesInBuffer >= FilterMaskSize) && (TC_Params_p->TC_AutomaticSectionFiltering != TRUE))
        {
            for(i=0;i<FilterMaskSize;i++)
            {
                ((U8 *) Base_p)[(ReadBufferIndex + i) % BufferSize] &= ~FilterMask[i];
            }

            /* Flush the cache for data we have written */
            stpti_FlushRegionCircular(Base_p, Top_p, (U32) &((U8 *) Base_p)[ReadBufferIndex % BufferSize], FilterMaskSize );
        }
    }

    return Error;
}
#endif /* #if !defined (STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stptiHAL_DVBBufferReadSection
  Description : stpti_TCReadSection
                Attempts to read one section into supplied destination buffers
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBBufferReadSection( FullHandle_t FullBufferHandle,
                                              STPTI_Filter_t MatchedFilterList[],
                                              U32 MaxLengthofFilterList,
                                              U32 *NumOfFilterMatches_p,
                                              BOOL *CRCValid_p,
                                              U8 *Destination0_p,
                                              U32 DestinationSize0,
                                              U8 *Destination1_p,
                                              U32 DestinationSize1,
                                              U32 *DataSize_p,
                                              STPTI_Copy_t DmaOrMemcpy )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    U16 DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t *DMAConfig_p = &((TCDMAConfig_t *)  TC_Params_p->TC_DMAConfigStart)[DMAIdent];

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;
    U32 Top_p   = (DMAConfig_p->DMATop_p & ~0xf) + 16;
    U32 Base_phys_p  = DMAConfig_p->DMABase_p;
    U32 Base_p;
    U32 BufferSize = Top_p - Base_phys_p;
    size_t FilterMaskSize = 0;
    U8  MetaData[16] = { 0 };
    U32 BytesInBuffer = (Write_p >= Read_p) ? Write_p - Read_p : (Top_p - Read_p) + (Write_p - Base_phys_p);
    
    size_t BytesCopied = 0;
    U16 Section;
    int i;
    
    /* Convert to virtual non-cached address */
    U32 MappedStart_p = (U32)stptiMemGet_Buffer(FullBufferHandle)->MappedStart_p;

    Read_p = MappedStart_p + (Read_p - Base_phys_p);
    Write_p = MappedStart_p + (Write_p - Base_phys_p);
    Top_p = MappedStart_p + (Top_p - Base_phys_p);
    Base_p = MappedStart_p;
    
    if (NULL != DataSize_p)
    {
        *DataSize_p = 0;
    }
    
    *CRCValid_p = FALSE;

    /* PTI DMA does not work so use memcpy */
    if (DmaOrMemcpy == STPTI_COPY_TRANSFER_BY_DMA)
    {
        DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;
    }
    
    if (TC_Params_p->TC_AutomaticSectionFiltering != TRUE)
    {
        FilterMaskSize = TcCam_GetMetadataSizeInBytes();
    }

#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    if(((stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff0000) == 0)       /* Conventional multi packet operation? */
    {
#endif
        for (Section = 0; Section < ((stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff); ++Section)
        {
            /*
             * The "automatic" section filter adds metadata before and after the section in the buffer...
             *
             * For PTI4 (5528)
             *   The first 8bytes are the 64bits of matched result (sf_match_result) CAMA, then CAMB.
             *   The section then follows (including its header)
             *   The last byte is the CRC (0x00=CRC_ok, 0xFF=CRC_bad).
             *
             * For PTI4L (5100, 5301, 7100) and PTI4SL (7109, 5525)
             *   The first 12bytes are the 96bits of matched result (sf_match_result) CAMA, then CAMB.
             *   The section then follows (including its header)
             *   The last byte is the CRC (0x00=CRC_ok, 0xFF=CRC_bad).
             */

            /* Invalidate the cache for data we are about to read */
            stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, (SectionMinLength + FilterMaskSize + CRCStateSize) );

             /* If we have enough bytes in the buffer to allow us to check for a section */
            if (BytesInBuffer >= (SectionMinLength + FilterMaskSize))
            {
               /* Calculate Buffer Parameters based on Read_p */
                U32 ReadBufferIndex = Read_p - Base_p;
                U32 BytesBeforeWrap = Top_p  - Read_p;
                size_t SectionLength = 3;

                SectionLength += ((((U8 *) Base_p)[(ReadBufferIndex + 1 + FilterMaskSize) % BufferSize] & 0x0f) << 8);
                SectionLength +=   ((U8 *) Base_p)[(ReadBufferIndex + 2 + FilterMaskSize) % BufferSize];

                /* Invalidate the cache for data we are about to read */
                stpti_InvalidateRegionCircular(Base_p, Top_p, Read_p, (SectionLength + FilterMaskSize + CRCStateSize) );

                /* Extract MetaData from the buffer */
                for(i=0;i<FilterMaskSize;i++)
                {
                    MetaData[i] = ((U8 *) Base_p) [(ReadBufferIndex + i) % BufferSize];
                }

                /* Does this Section have a CRC error */
                if ((((U8 *) Base_p)[(ReadBufferIndex + SectionLength + FilterMaskSize) % BufferSize] == 0))
                {
                    /* Report the CRC as correct */
                    *CRCValid_p = TRUE;
                }
                /* else CRCValid is already set FALSE */

#if defined(STPTI_INTERNAL_DEBUG_SUPPORT)
                STTBX_Print(("BytesInBuffer    : %d\n", BytesInBuffer));
                STTBX_Print(("SectionLength    : %d\n", SectionLength));
                STTBX_Print(("FilterMask       : 0x%08x 0x%08x\n", MatchedFilterMask[0], MatchedFilterMask[1]));
                if (BytesInBuffer >= (SectionLength + FilterMaskSize + CRCStateSize))
                {
                    STTBX_Print(("CRCValid: %s\n", (*CRCValid_p) ? "TRUE" : "FALSE"));
                }
                else
                {
                    STTBX_Print(("CRCValid: Not enough bytes in buffer\n"));
                }

                {
                    U32 i;
                    for (i = 0; i < BytesInBuffer; ++i)
                    {
                        STTBX_Print(("%02x ", (((U8 *) Base_p)[(ReadBufferIndex + i) % BufferSize])));
                        if ((i % 16) == 15)
                        {
                            STTBX_Print(("\n"));
                        }
                    }
                    STTBX_Print(("\n"));
                }
#endif
                if (BytesInBuffer >= (SectionLength + FilterMaskSize + CRCStateSize))
                {
                    U32 SizeToCopy;
                    U32 SizeCopied;
                    U32 SrcSize0;
                    U8 *Src0_p;
                    U32 SrcSize1;
                    U8 *Src1_p;

                    Read_p = (U32)&(((U8 *) Base_p)[(ReadBufferIndex + FilterMaskSize) % BufferSize]);

                    ReadBufferIndex = Read_p - Base_p;
                    BytesBeforeWrap = Top_p  - Read_p;

                    SizeToCopy = SectionLength;

                    SrcSize0 = MINIMUM(BytesBeforeWrap, SizeToCopy);
                    Src0_p = &((U8 *) Base_p)[ReadBufferIndex];
                    SrcSize1 = SizeToCopy - SrcSize0;
                    Src1_p = (U8 *) Base_p;

                    SizeCopied = stptiHelper_CircToCircCopy( SrcSize0, /* Number useful bytes in PTIBuffer Part0 */
                                                             Src0_p, /* Start of PTIBuffer Part0 */
                                                             SrcSize1, /* Number useful bytes in PTIBuffer Part1 */
                                                             Src1_p, /* Start of PTIBuffer Part1  */
                                                             &DestinationSize0, /* Space in UserBuffer0 */
                                                             &Destination0_p, /* Start of UserBuffer0 */
                                                             &DestinationSize1, /* Space in UserBuffer1 */
                                                             &Destination1_p, /* Start of UserBuffer1 */
                                                             DmaOrMemcpy, /* Copy Method */
                                                             FullBufferHandle );

                    /* Advance Read_p to point to next section */
                    if ( SizeCopied != TC_ERROR_TRANSFERRING_BUFFERED_DATA )
                    {
                        BytesCopied   += SizeCopied;
                        BytesInBuffer -= (SizeCopied + FilterMaskSize + CRCStateSize);

                        Read_p = (U32)&(((U8 *) Base_p)[(ReadBufferIndex + SectionLength + CRCStateSize) % BufferSize]);

                        STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, (Read_p-Base_p+Base_phys_p) );

                        stptiHAL_MaskConvertToList( MetaData,
                                                    MatchedFilterList,
                                                    MaxLengthofFilterList,
                                                    *CRCValid_p,
                                                    NumOfFilterMatches_p,
                                                    PrivateData_p,
                                                    FullBufferHandle,
                                                    FALSE );

                        /* Update Read pointer in DMA structure */
                        if (NULL != DataSize_p)
                        {
                            *DataSize_p = BytesCopied;
                        }
                    }
                    else
                    {
                        /* Indicate an error to caller */
                        if (NULL != DataSize_p)
                        {
                            *DataSize_p = SizeCopied;
                        }
                    }
                }
                else
                {
                    /*
                     * The Section in the buffer is incomplete, though enough bytes
                     * exist to determine what its length will be
                     */
                    Error = STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER;
                }
            }
            else
            {
                    /*
                     * too few (or no) Bytes exist in the buffer to even determine the
                     * section length
                     */
                    if (BytesInBuffer == 0)
                    {
                        if(Section>0) break;                    /* if at least one packet has been delivered we're ok */
                        Error = STPTI_ERROR_NO_PACKET;
                    }
                    else
                    {
                        Error = STPTI_ERROR_INCOMPLETE_SECTION_IN_BUFFER;
                    }

                    STTBX_Print(("BytesInBuffer    : %d\n", BytesInBuffer));
            }
        }
    /* Signal or Re-enable interrupts as appropriate */
    
#if !defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
        /* PTI4, PTI4L don't have the capability of counting sections, hence we signal if ANY data is waiting */
        stptiHelper_SignalEnableIfDataWaiting(FullBufferHandle, Section);
#else
        stptiHelper_SignalEnable(FullBufferHandle, Section);
#endif
        
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    }
    else        /* Operation for BufferLevelThreshold, comparing byte count to a threshold, instead of counting sections */
    {                                   /* Note that threshold size is middle 8 bits of 32 bit value, shift 8 bits */
        while (BytesCopied < ((stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize) & 0xffff0000)>>8)
        {
             /* For PTI4SL (7109, 5525)
             *   The first 12bytes are the 96bits of matched result (sf_match_result) CAMA, then CAMB.
             *   The section then follows (including its header)
             *   The last byte is the CRC (0x00=CRC_ok, 0xFF=CRC_bad).
             */
             /* If we have enough bytes in the buffer to allow us to check for a section */
            if (BytesInBuffer >= (SectionMinLength + FilterMaskSize))
            {
               /* Calculate Buffer Parameters based on Read_p */
                U32 ReadBufferIndex = Read_p - Base_p;
                U32 BytesBeforeWrap = Top_p  - Read_p;
                size_t SectionLength = 3;

                SectionLength += ((((U8 *) Base_p)[(ReadBufferIndex + 1 + FilterMaskSize) % BufferSize] & 0x0f) << 8);
                SectionLength +=   ((U8 *) Base_p)[(ReadBufferIndex + 2 + FilterMaskSize) % BufferSize];

                /* Extract MetaData from the buffer */
                for(i=0;i<FilterMaskSize;i++)
                {
                    MetaData[i] = ((U8 *) Base_p) [(ReadBufferIndex + i) % BufferSize];
                }

                /* Does this Section have a CRC error */
                if ((((U8 *) Base_p)[(ReadBufferIndex + SectionLength + FilterMaskSize) % BufferSize] == 0))
                {
                    /* Report the CRC as correct */
                    *CRCValid_p = TRUE;
                }
                /* else CRCValid is already set FALSE */

#if defined(STPTI_INTERNAL_DEBUG_SUPPORT)
                STTBX_Print(("BytesInBuffer    : %d\n", BytesInBuffer));
                STTBX_Print(("SectionLength    : %d\n", SectionLength));
                STTBX_Print(("FilterMask       : 0x%08x 0x%08x\n", MatchedFilterMask[0], MatchedFilterMask[1]));
                if (BytesInBuffer >= (SectionLength + FilterMaskSize + CRCStateSize))
                {
                    STTBX_Print(("CRCValid: %s\n", (*CRCValid_p) ? "TRUE" : "FALSE"));
                }
                else
                {
                    STTBX_Print(("CRCValid: Not enough bytes in buffer\n"));
                }

                {
                    U32 i;
                    for (i = 0; i < BytesInBuffer; ++i)
                    {
                        STTBX_Print(("%02x ", (((U8 *) Base_p)[(ReadBufferIndex + i) % BufferSize])));
                        if ((i % 16) == 15)
                        {
                            STTBX_Print(("\n"));
                        }
                    }
                    STTBX_Print(("\n"));
                }
#endif
                if (BytesInBuffer >= (SectionLength + FilterMaskSize + CRCStateSize))
                {
                    U32 SizeToCopy;
                    U32 SizeCopied;
                    U32 SrcSize0;
                    U8 *Src0_p;
                    U32 SrcSize1;
                    U8 *Src1_p;

                    Read_p = (U32)&(((U8 *) Base_p)[(ReadBufferIndex + FilterMaskSize) % BufferSize]);

                    ReadBufferIndex = Read_p - Base_p;
                    BytesBeforeWrap = Top_p  - Read_p;

                    SizeToCopy = SectionLength;

                    SrcSize0 = MINIMUM(BytesBeforeWrap, SizeToCopy);
                    Src0_p = &((U8 *) Base_p)[ReadBufferIndex];
                    SrcSize1 = SizeToCopy - SrcSize0;
                    Src1_p = (U8 *) Base_p;

                    SizeCopied = stptiHelper_CircToCircCopy( SrcSize0, /* Number useful bytes in PTIBuffer Part0 */
                                                             Src0_p, /* Start of PTIBuffer Part0 */
                                                             SrcSize1, /* Number useful bytes in PTIBuffer Part1 */
                                                             Src1_p, /* Start of PTIBuffer Part1  */
                                                             &DestinationSize0, /* Space in UserBuffer0 */
                                                             &Destination0_p, /* Start of UserBuffer0 */
                                                             &DestinationSize1, /* Space in UserBuffer1 */
                                                             &Destination1_p, /* Start of UserBuffer1 */
                                                             DmaOrMemcpy, /* Copy Method */
                                                             FullBufferHandle );

                    /* Advance Read_p to point to next section */
                    if ( SizeCopied != TC_ERROR_TRANSFERRING_BUFFERED_DATA )
                    {
                        BytesCopied   += SizeCopied;
                        BytesInBuffer -= (SizeCopied + FilterMaskSize + CRCStateSize);

                        Read_p = (U32)&(((U8 *) Base_p)[(ReadBufferIndex + SectionLength + CRCStateSize) % BufferSize]);

                        STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, (Read_p-Base_p+Base_phys_p) );

                        stptiHAL_MaskConvertToList( MetaData,
                                                    MatchedFilterList,
                                                    MaxLengthofFilterList,
                                                    *CRCValid_p,
                                                    NumOfFilterMatches_p,
                                                    PrivateData_p,
                                                    FullBufferHandle,
                                                    FALSE );

                        if (NULL != DataSize_p)
                        {
                            *DataSize_p = BytesCopied;
                        }
                    }
                    else
                    {
                        /* Indicate an error to caller */
                        if (NULL != DataSize_p)
                        {
                            *DataSize_p = SizeCopied;
                        }
                    }
                }
                else
                {
                    /*
                     * The Section in the buffer is incomplete, though enough bytes
                     * exist to determine what its length will be - for this threshold mode we just stop copying
                     */
                    break;
                }
            }
            else
            {
                    /*
                     * too few (or no) Bytes exist in the buffer to even determine the
                     * section length
                     */
                break;
                STTBX_Print(("BytesInBuffer    : %d\n", BytesInBuffer));
            }
        }
    }
#endif /* defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE) */      
    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHAL_DVBFilterEnableSection
  Description : stpti_TCEnableSectionFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterEnableSection( FullHandle_t FilterHandle )
{
    return( TcCam_EnableFilter( FilterHandle, TRUE) );
}


/******************************************************************************
Function Name : stptiHAL_DVBFilterDisableSection
  Description : stpti_TCDisableSectionFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterDisableSection( FullHandle_t FilterHandle )
{
    return( TcCam_EnableFilter( FilterHandle, FALSE) );
}


/******************************************************************************
Function Name : stptiHAL_DVBFilterEnableTransport
  Description : stpti_EnableTransportFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterEnableTransport( FullHandle_t FilterHandle )
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_DVBFilterDisableTransport
  Description : stpti_TCDisableTransportFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterDisableTransport(FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_DVBFilterDisablePES
  Description : stpti_TCDisablePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterDisablePES(FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_DVBFilterEnablePES
  Description : stpti_EnablePESFilter
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterEnablePES(FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}

/******************************************************************************
Function Name : stptiHAL_DVBGetPacketErrorCount
  Description : stpti_TCGetPacketErrorCount
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBGetPacketErrorCount( FullHandle_t DeviceHandle,
                                                U32 *Count_p )
{
    Device_t             *Device_p       = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p  = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *TCSessionInfo_p;

    TCSessionInfo_p =  &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[Device_p->Session];

    *Count_p = STSYS_ReadRegDev16LE((void*)&TCSessionInfo_p->SessionInputErrorCount); 

    return( ST_NO_ERROR );
}


/******************************************************************************
Function Name : stptiHAL_DVBFilterSetMatchAction
  Description : stpti_TCSectionFilterSetMatchAction
   Parameters :

       Match Action on TC is simply a table of 32 bit maps, one for each filter
      each entry being a bit map defining which filters need to be enabled.

******************************************************************************/
ST_ErrorCode_t stptiHAL_DVBFilterSetMatchAction(FullHandle_t FilterHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* EOF  -------------------------------------------------------------------- */

