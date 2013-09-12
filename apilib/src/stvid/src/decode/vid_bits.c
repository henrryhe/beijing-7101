/*******************************************************************************

File name   : vid_bits.c

Description : Header data management source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
06 Jan 2000        Created                                          HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define to add debug info and traces */
/*#define TRACE_DECODE*/

#ifdef TRACE_DECODE
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif
#define DECODE_DEBUG
#endif

/* Includes ----------------------------------------------------------------- */

#if !defined ST_OSLINUX
#include <limits.h>
#endif

#include "stddefs.h"
#include "stos.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"
#endif /* def STVID_STVAPI_ARCHITECTURE */

#include "vid_mpeg.h"
#include "vid_com.h"
#include "vid_bits.h"
#include "vid_dec.h"
#include "halv_dec.h"
#include "stcommon.h"
#include "sttbx.h"
#include "stsys.h"
#include "vid_head.h"
/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

/* Max header FIFO wait time: 2 fields display time, not to block decode task too long */
#define MAX_WAIT_HEADER_FIFO    (2 * STVID_MAX_VSYNC_DURATION)


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

#ifndef ST_OSWINCE
static __inline void HeaderDataFlushBits(HeaderData_t * const HeaderData_p);
#else
static void HeaderDataFlushBits(HeaderData_t * const HeaderData_p);
#endif

/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : HeaderDataFlushBits
Description : Discard current bits, force next viddec_HeaderDataGetBits() to load new data
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#ifndef ST_OSWINCE
static __inline void HeaderDataFlushBits(HeaderData_t * const HeaderData_p)
#else
static void HeaderDataFlushBits(HeaderData_t * const HeaderData_p)
#endif
{
    HeaderData_p->NumberOfBits = 0;       /* No remaining bits */
    HeaderData_p->Bits = 0;
} /* End of HeaderDataFlushBits() function */


/*******************************************************************************
Name        : viddec_HeaderDataResetBitsCounter
Description : Reset counter of bits retrieved
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
#ifdef STVID_CONTROL_HEADER_BITS
__inline void viddec_HeaderDataResetBitsCounter(HeaderData_t * const HeaderData_p)
{
    HeaderData_p->BitsCounter = 0;
    HeaderData_p->MissingBitsCounter = 0;
} /* End of viddec_HeaderDataResetBitsCounter() function */

#endif

/*******************************************************************************
Name        : viddec_HeaderDataGetBitsCounter
Description : Get number of bits retrieved since last call to viddec_HeaderDataResetBitsCounter()
Parameters  :
Assumptions : BitsCounter is incremented (in other functions) each time bits are retrieved
Limitations :
Returns     : Number of bits retrieved since last call to viddec_HeaderDataResetBitsCounter()
*******************************************************************************/
#ifdef STVID_CONTROL_HEADER_BITS
__inline U32 viddec_HeaderDataGetBitsCounter(HeaderData_t * const HeaderData_p)
{
    return(HeaderData_p->BitsCounter);
} /* End of viddec_HeaderDataGetBitsCounter() function */
#endif


/*******************************************************************************
Name        : viddec_HeaderDataGetMissingBitsCounter
Description : Get number of bits missing since last call to viddec_HeaderDataResetBitsCounter()
              0 means there was no problem, but if header data FIFO ran empty then it says how many bits have been missed
Parameters  :
Assumptions : MissingBitsCounter is incremented (in other functions) each time bits are missing
Limitations :
Returns     : Number of bits missed since last call to viddec_HeaderDataResetBitsCounter()
*******************************************************************************/
#ifdef STVID_CONTROL_HEADER_BITS
__inline U32 viddec_HeaderDataGetMissingBitsCounter(HeaderData_t * const HeaderData_p)
{
    return(HeaderData_p->MissingBitsCounter);
} /* End of viddec_HeaderDataGetMissingBitsCounter() function */
#endif

/*******************************************************************************
Name        : viddec_HeaderDataGetBits
Description : Retrieves n bits from buffer, 1 <= n <= 32
Parameters  : - Header data structure
              - number of bits requested (1 to 32)
Assumptions :
Limitations : number of requested bits cannot be more than 32
              return 0 if no data available
Returns     : value of the requested bits (with 0's appended to the left up to 32 bits)
*******************************************************************************/
U32 viddec_HeaderDataGetBits(HeaderData_t * const HeaderData_p, U32 NbRequestedBits)
{
    U32                 RequestedBits;
    clock_t             MaxWaitHeaderFIFO; /* Max time for waiting for data in Header FIFO */

    if (HeaderData_p->IsHeaderDataSpoiled)
    {
        /* Spurious start code was read during data read: read more data to ensure SC in completly out ? */
        return(0);
    }

    RequestedBits  = HeaderData_p->Bits >> (32 - NbRequestedBits);
#ifdef STVID_CONTROL_HEADER_BITS
    HeaderData_p->BitsCounter   += NbRequestedBits;
#endif
    if (NbRequestedBits <= HeaderData_p->NumberOfBits)
    {
        /* If enough bits, update headerData and return the bits */
        HeaderData_p->Bits          = HeaderData_p->Bits << (NbRequestedBits);
        HeaderData_p->NumberOfBits  -= NbRequestedBits;
        return(RequestedBits);
    }
    else/* If not enough bits, take bits from buffer and get more */
    {
        NbRequestedBits -= HeaderData_p->NumberOfBits;

        /* !!! Must get data: check if there are some */
        if (HeaderData_p->SearchThroughHeaderDataNotCPU)
        {
            /* HW header data retrieval */
            if (HALDEC_IsHeaderDataFIFOEmpty(HeaderData_p->HALDecodeHandle))
            {
                /* Wait a little bit and try again, for furtive empty's. */
			    STOS_TaskDelayUs(1000);     /* 1ms */

                /* If header FIFO is still empty, we have to wait. But we cannot wait
                for ever as we would be blocked if injection is stalled. So wait for
                a certain time, then leave with invalid start code. */
                MaxWaitHeaderFIFO = time_plus(time_now(), MAX_WAIT_HEADER_FIFO);
                while (HALDEC_IsHeaderDataFIFOEmpty(HeaderData_p->HALDecodeHandle))
                {
                    if (STOS_time_after(time_now(), MaxWaitHeaderFIFO) != 0)
                    {
                        /* Security to go out anyway: return an invalid start code */
                        /* Say some bits are missing ! */
                        HeaderData_p->NumberOfBits = 0;
#ifdef STVID_CONTROL_HEADER_BITS
                        HeaderData_p->BitsCounter         -= NbRequestedBits;
                        HeaderData_p->MissingBitsCounter  += NbRequestedBits;
#endif /* STVID_CONTROL_HEADER_BITS */
#ifdef STVID_DEBUG_GET_STATISTICS
                        HeaderData_p->StatisticsDecodePbHeaderFifoEmpty ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

                        /* Remember data have been lost...  */
                        HeaderData_p->IsHeaderDataSpoiled = TRUE;

                        return(RequestedBits);
                    }
                    /* Wait more... */
			        STOS_TaskDelayUs(1000);     /* 1ms */

                }/* end 2nd try is empty : the function returns */
                /* else : fifo not empty : continue */
            }/* end 1st try */
            /* else : fifo not empty : continue */
        } /* HW header data retrieval */

        /* Read bits, but never more than the minimum necessary: we may read too much bits */
        /* and take data of next start code ! */
        if ((NbRequestedBits <= 8) && (!(HeaderData_p->SearchThroughHeaderDataNotCPU)))
        {
            /* SW CPU memory access data retrieval */
            HeaderData_p->NumberOfBits = 8;
            HeaderData_p->Bits = ((STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p    ) << 24));
            HeaderData_p->CPUSearchLastReadAddressPlusOne_p += 1;
        }
        else if (NbRequestedBits <= 16)
        {
            /* Write number of bits BEFORE getting bits. So that if it is */
            /* intercepted by SCH IT, it is not overwritten. Bits will be */
            /* overwritten but it should be OK for the Picture sc !!! */
            HeaderData_p->NumberOfBits = 16;
            if (HeaderData_p->SearchThroughHeaderDataNotCPU)
            {
                /* HW header data retrieval */
                HeaderData_p->Bits = HALDEC_Get16BitsHeaderData(HeaderData_p->HALDecodeHandle) << 16;
            }
            else
            {
                /* SW CPU memory access data retrieval */
                HeaderData_p->Bits = ((STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p    ) << 24) |
                                      (STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p + 1) << 16));
                HeaderData_p->CPUSearchLastReadAddressPlusOne_p += 2;
            }
        }
        else if ((NbRequestedBits <= 24) && (!(HeaderData_p->SearchThroughHeaderDataNotCPU)))
        {
            /* SW CPU memory access data retrieval */
            HeaderData_p->NumberOfBits = 24;
            HeaderData_p->Bits = ((STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p    ) << 24) |
                                  (STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p + 1) << 16) |
                                  (STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p + 2) << 8));
            HeaderData_p->CPUSearchLastReadAddressPlusOne_p += 3;
        }
        else
        {
            /* Write number of bits BEFORE getting bits. So that if it is */
            /* intercepted by SCH IT, it is not overwritten. Bits will be */
            /* overwritten but it should be OK for the Picture sc !!! */
            HeaderData_p->NumberOfBits = 32;
            if (HeaderData_p->SearchThroughHeaderDataNotCPU)
            {
                /* HW header data retrieval */
                HeaderData_p->Bits = HALDEC_Get32BitsHeaderData(HeaderData_p->HALDecodeHandle);
            }
            else
            {
                /* SW CPU memory access data retrieval */
                HeaderData_p->Bits = ((STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p    ) << 24) |
                                      (STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p + 1) << 16) |
                                      (STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p + 2) <<  8) |
                                      (STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p + 3)));
                HeaderData_p->CPUSearchLastReadAddressPlusOne_p += 4;
            }
        }
        if (HeaderData_p->IsHeaderDataSpoiled)
        {
            /* Spurious start code was read during data read: read more data */
            /* to ensure SC in completly out ? */
            return(0);
        }
    }
    RequestedBits              |= (HeaderData_p->Bits >> (32 - NbRequestedBits));
    HeaderData_p->Bits          = HeaderData_p->Bits << (NbRequestedBits);
    HeaderData_p->NumberOfBits -= NbRequestedBits;
    return(RequestedBits);
} /* End of viddec_HeaderDataGetBits() function */


/*******************************************************************************
Name        : viddec_HeaderDataGetStartCode
Description : Get a byte (byte aligned) from memory or from header data FIFO.
              If from header data FIFO, retrieves 8 or 16 bits, then may save 8 bits.
Parameters  : Header data structure
Assumptions : Start code was just found (SCH interrupt), and no data was read since
              then from header data FIFO
Limitations :
Returns     : Start code
*******************************************************************************/
U8 viddec_HeaderDataGetStartCode(HeaderData_t * const HeaderData_p)
{
    U16 StartCode;
    BOOL StartCodeOnMSB;
    clock_t MaxWaitHeaderFIFO; /* Max time for waiting for data in Header FIFO */
#ifdef TRACE_UART
    clock_t FirstTime;
    clock_t EndTime;
    BOOL WasWaitingForNotEmpty = FALSE;
#endif

    /* Inform the bit retrieve routine to read data word from header FIFO before starting */
    HeaderDataFlushBits(HeaderData_p);

#ifdef TRACE_UART
    /* Get the current time. */
    FirstTime = time_now();
#endif

    if (HeaderData_p->SearchThroughHeaderDataNotCPU)
    {
        /* HW header data retrieval */
        /* We have to check that header FIFO is not empty before getting start code.
        If it is empty, we have to wait. But we cannot wait for ever as we would be blocked
        if injection is stalled. So wait for a certain time, then leave with invalid start code. */
        MaxWaitHeaderFIFO = time_plus(time_now(), MAX_WAIT_HEADER_FIFO);
        while (HALDEC_IsHeaderDataFIFOEmpty(HeaderData_p->HALDecodeHandle))
        {

#ifdef TRACE_UART
        WasWaitingForNotEmpty = TRUE;
#endif /* TRACE_UART */

            /* Got SCH interrupt but cannot read start code: must be very furtive condition, wait until SC comes ! */
            if (STOS_time_after(time_now(), MaxWaitHeaderFIFO) != 0)
            {
                /* Security to go out anyway: return an invalid start code */
                /* Particular case for decode once but the particular case could be enlarge to all the playback. */
                /* When the SC id is stuck in the CDFIFO, we wait for him until he comes thanks to a flush or an */
                /* injection continuation or stop.                                                               */
/*                if ((!(VIDDEC_Data_p->DecodeOnce)) || (VIDDEC_Data_p->RealTime) ||
                    ((VIDDEC_Data_p->Stop.WhenNextReferencePicture) ||
                    ((VIDDEC_Data_p->Stop.WhenNoData) && (!(VIDDEC_Data_p->Stop.IsPending))) ||
                    (VIDDEC_Data_p->Stop.WhenNextIPicture) || (VIDDEC_Data_p->DecoderState == VIDDEC_DECODER_STATE_STOPPED)))*/
                if (!(HeaderData_p->WaitForEverWhenHeaderDataEmpty))
                {
#ifdef TRACE_UART
                    TraceBuffer(("-HFIFO empty, return(invalid-SC) !\r\n"));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
                    HeaderData_p->StatisticsDecodePbHeaderFifoEmpty ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

                    return(GREATEST_SYSTEM_START_CODE);
                }
                /* else we are stuck here but we give the hand to the app thanks to the STOS_TaskDelayUs below. */
            }
            /* Wait more... */
			STOS_TaskDelayUs(1000);     /* 1ms */
        }

#ifdef TRACE_UART
        if (WasWaitingForNotEmpty)
        {
            /* Get the current time. */
            EndTime = time_now();
            TraceBuffer(("-HFIFO empty for %dus\r\n", (time_minus (EndTime, FirstTime) * 1000000)/ST_GetClocksPerSecond()));
        }
#endif

        /* Get one byte. If start code is in the second byte, get one more byte */
        StartCode = HALDEC_Get16BitsStartCode(HeaderData_p->HALDecodeHandle, &StartCodeOnMSB);

        if (StartCodeOnMSB)
        {
            /* Start code is in the first byte: second byte is data to remember ! */
            HeaderData_p->Bits = StartCode << 24; /* Put byte as 8 MSB */
            HeaderData_p->NumberOfBits = 8;

            StartCode = StartCode >> 8;
        }
    }
    else
    {
        /* SW CPU memory access data retrieval */
        StartCode = STSYS_ReadRegMemUncached8(HeaderData_p->CPUSearchLastReadAddressPlusOne_p);
        HeaderData_p->CPUSearchLastReadAddressPlusOne_p += 1;
    }

    return((U8) StartCode);
} /* End of viddec_HeaderDataGetStartCode() function */


/*******************************************************************************
Name        : viddec_SearchAndGetNextStartCode
Description : Looks for a start code (byte pattern 0x00 0x00 0x01 0xNN) and return it
Parameters  : - Decode handle (used only for search through HeaderData)
              - SearchThroughHeaderDataNotCPU is TRUE  : search through HeaderData (standard HW data retrieval functions)
                SearchThroughHeaderDataNotCPU is FALSE : search through CPU parsing (so called manual parsing)
              - CPUSearchAddressStart_p : For CPU search, first address to search SC from in memory (byte aligned)
              - MaxByteSearchMinusFour : Max bytes to search through, minus 4. If no SC found after this amount of data, function returns FALSE.
Assumptions :
Limitations :
Returns     : - return parameter : TRUE if start code found, FALSE if not found
              - in *StartCode_p : start code value
              - only for CPU search, in *StartCodeAddress_p : address of the first byte of the start code found
*******************************************************************************/
BOOL viddec_SearchAndGetNextStartCode(HeaderData_t * const HeaderData_p,
                                      const U8 * const CPUSearchAddressStart_p,
                                      const U32 MaxByteSearchMinusFour,
                                      U8 * const StartCode_p,
                                      const U8 ** StartCodeAddress_p)
{
    U8 Tmp8;
    U32 BytesRemaining = MaxByteSearchMinusFour + 4;
    U8 ZeroBytes = 0;
    BOOL StartCodeFound = FALSE;

    /* Look for the start code prefix 0x00 0x00 0x01 */
    HeaderData_p->CPUSearchLastReadAddressPlusOne_p = CPUSearchAddressStart_p;
    do
    {
        /* Get next one byte of data into Tmp8, to analyse it */
        Tmp8 = viddec_HeaderDataGetStartCode(HeaderData_p);
        BytesRemaining--;
        /* Analyse byte */
        switch (Tmp8)
        {
            case 0x00 :
                if (ZeroBytes < UCHAR_MAX)
                {
                    ZeroBytes++;
                }
                break;

            case 0x01 :
                if (ZeroBytes >= 2)
                {
                    StartCodeFound = TRUE;
                    BytesRemaining = 1;
                }
                else
                {
                    ZeroBytes = 0;
                }
                break;

            default :
                ZeroBytes = 0;
        } /* end of switch */
    } while (BytesRemaining > 1);

    if (StartCodeFound)
    {
        /* Now get start code value */
        *StartCode_p = viddec_HeaderDataGetStartCode(HeaderData_p);
        if (!(HeaderData_p->SearchThroughHeaderDataNotCPU))
        {
            *StartCodeAddress_p = HeaderData_p->CPUSearchLastReadAddressPlusOne_p - 4; /* Because pointer is now after the SC value, and need to return first SC byte address */
        }
    }

    /* Now return values */
    return(StartCodeFound);
} /* End of viddec_SearchAndGetNextStartCode() function */

#ifdef ST_SecondInstanceSharingTheSameDecoder
/*******************************************************************************
Name        : viddec_FindOneSequenceAndPicture
Description : Looks for a sequence + picture information. This is used to parse a still manually (not with help of HW SCD).
Parameters  : - Decode handle (used only for search through HeaderData)
              - SearchThroughHeaderDataNotCPU is TRUE  : search through HeaderData (standard HW data retrieval functions)
                SearchThroughHeaderDataNotCPU is FALSE : search through CPU parsing (so called manual parsing)
              - CPUSearchAddressStart_p : For CPU search, first address to search SC from in memory (byte aligned)
              - Stream_p and others... : parameters for viddec_ProcessStartCode(), but don't know yet if this is the best way to do !
Assumptions :
Limitations :
Returns     : - return parameter : TRUE if a full picture information is found, FALSE otherwise
              - only for CPU search, in *PictureInfos_p : address of the picture found
*******************************************************************************/
BOOL viddec_FindOneSequenceAndPicture(HeaderData_t * const HeaderData_p,
                                      const BOOL SearchThroughHeaderDataNotCPU,
                                      const U8 * const CPUSearchAddressStart_p,
                                      MPEG2BitStream_t * const Stream_p,
                                      STVID_PictureInfos_t * const PictureInfos_p,
                                      StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                      PictureStreamInfo_t * const PictureStreamInfo_p,
                                      VIDDEC_SyntaxError_t * const SyntaxError_p,
                                      U32 * const TemporalReferenceOffset_p,
                                      void ** PictureStartAddress_p)
{
    BOOL    SequenceFound = FALSE;
    BOOL    PictureFound = FALSE;
    BOOL    BeginOfPictureDataFound = FALSE;
    BOOL    EndOfPictureFound = FALSE;
    BOOL    TmpBool;
    U8      StartCode;

    const U8 * StartCodeAddress_p = CPUSearchAddressStart_p - 4;
    U32     MaxLengthStartCodeSearch = 100;
    ST_ErrorCode_t ErrorCode;

    do
    {
        TmpBool = viddec_SearchAndGetNextStartCode(HeaderData_p, StartCodeAddress_p + 4, MaxLengthStartCodeSearch, &StartCode, &StartCodeAddress_p);
        if (!(TmpBool))
        {
            return(FALSE);
        }
        if (StartCode == SEQUENCE_HEADER_CODE)
        {
            SequenceFound = TRUE;
        }
        if ((SequenceFound) && (StartCode == PICTURE_START_CODE))
        {
            PictureFound = TRUE;
            *PictureStartAddress_p = (void *) StartCodeAddress_p;
        }
        if ((PictureFound) && (StartCode <= GREATEST_SLICE_START_CODE))
        {
            BeginOfPictureDataFound = TRUE;
            MaxLengthStartCodeSearch = 100000;
        }
        if ((BeginOfPictureDataFound) && (StartCode > GREATEST_SLICE_START_CODE))
        {
            EndOfPictureFound = TRUE;
        }
        if (SequenceFound)
        {
            /* As soon as a sequence is found, process all start codes found */
            ErrorCode = viddec_ParseStartCodeForStill(HeaderData_p, StartCode, Stream_p, PARSER_MODE_STREAM_ANALYSER);
        }

    } while (!(EndOfPictureFound));

    return(TRUE);

} /* End of viddec_FindOneSequenceAndPicture() function */
#endif /* ST_SecondInstanceSharingTheSameDecoder */


/* End of vid_bits.c */
