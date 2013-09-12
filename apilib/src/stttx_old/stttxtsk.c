/****************************************************************************

File Name   : stttxtsk.c

Description : Teletext Task Routines, holding teletext transfer, VBI and
              STB output tasks, plus Transfer Complete ISR.

Copyright (C) 2007, STMicroelectronics

History :
            DDTS 21988 "TTX corrupts on some streams"
            VSYNC event is subscribed and VBI is enabled on thie event

References  :

$ClearCase (VOB: stttx)

ttxt_api.fm "Teletext API" Reference DVD-API-003 Revision 3.4

****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if !defined(ST_OSLINUX) /* linux changes */
#include <string.h>         /* System includes */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#ifndef ST_OS21
#include <ostime.h>
#endif
#include "stlite.h"        /* MUST be included BEFORE stpti.h */
#include "sttbx.h"
#include "stsys.h"
#endif

#include "stddefs.h"
#include "stcommon.h"
#include "stevt.h"
#include "stvtg.h"

#if defined(DVD_TRANSPORT_PTI) || defined(DVD_TRANSPORT_LINK)
#include "pti.h"
#elif defined(DVD_TRANSPORT_STPTI)
#include "stpti.h"
#elif defined(DVD_TRANSPORT_DEMUX)
#include "stdemux.h"
#endif

#if defined(DVD_TRANSPORT_LINK)
#include "ptilink.h"
#endif

#if defined(ST_OSLINUX)
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#endif

#if defined(STTTX_USE_DENC)
#include "stdevice.h"       /* Needed for the DENC base address */
#else
#include "stvbi.h"
#endif
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#include "stvmix.h"
#endif
#ifdef STTTX_SUBT_SYNC_ENABLE
#include "stclkrv.h"
#endif

#include "stttx.h"          /* internal includes */
#include "stttxdef.h"
#include "stttxdat.h"

#if STTTX_PES_MODE == PES_CAPTURE_MODE
#ifdef ST_OS20
#include "debug.h"          /* required for debugopen/read/close etc.       */
#endif
#endif

#ifdef USE_PROFILE
#include "profile.h"
#endif

#if defined(STTTX_DEBUG1)
U32 DmaWaCnt=0;
int StttxBufferSize=0;
#endif

/* Private Constants ------------------------------------------------------ */

/* #define ALE_DEBUG 1 */

#ifdef ALE_DEBUG
    #define ALE_BUFF_SIZE   100000
    static char alebuf0[ALE_BUFF_SIZE];
    static char *alebuf = alebuf0;
    static BOOL AlePrintEnabled = FALSE;
    #define ALE_PRINT_ENABLE  AlePrintEnabled = TRUE
    #define ALE_PRINT_DISABLE AlePrintEnabled = FALSE
    #define ALE_Print(s,X) {if (AlePrintEnabled) {char s[128];sprintf X;if (((U32)alebuf - (U32)alebuf0) < (ALE_BUFF_SIZE - 1000)) {memcpy(alebuf,s,strlen(s));alebuf += strlen(s);} else {AleCaptureEnd(); alebuf = alebuf0;}}}
#else
    #define ALE_Print(s,X)
    #define ALE_PRINT_ENABLE
    #define ALE_PRINT_DISABLE
#endif

/* The data-unit id for VPS data */
#define VPS_DATA_ID   0xc3
#define EXTRA_SYNC_DELAY    3500

#if defined(STTTX_DEBUG)

#if !defined (ST_7710)
#pragma ST_device(pio3set)
#pragma ST_device(pio3clr)
#pragma ST_device(pio3cfg)
#endif

volatile U8 *pio3set = (volatile U8 *)0x2000F004;
volatile U8 *pio3clr = (volatile U8 *)0x2000F008;
volatile U8 *pio3cfg = (volatile U8 *)0x2000F000 + (13<<2);

#endif

/* For PES capture/insertion; set some defaults */

#ifndef STTTX_PES_FILE
#define STTTX_PES_FILE "default.pes"
#endif

#ifndef STTTX_PESBUFFER_SIZE
#define STTTX_PESBUFFER_SIZE 1000000
#endif

/* PES Packet -> PESHeader
 *            -> EBUDataField -> *EBUDataUnit -> EBUDataBlock
 */

/* PES header */

/*
     PacketStartCodePrefix:24;      set to "0x000001"
     StreamId:8;                    set to private_stream_1 = "1011 1101"
     PacketLength:16;               set to (N x 184)-6; N is no. data fields
     UnusedFlags:2;                 set to "10"
     PESScramblingControl:2;
     PESPriority:1;
     DataAlignmentIndicator:1;      set to "1"
     Copyright:1;
     OriginalOrCopy:1;
     PTSDTSFlags:2;                 set to "0x3" or "0x2"
     ESCRFlag:1;
     ESRateFlag:1;
     DSMTrickModeFlag:1;
     AdditionalCopyInfoFlag:1;
     PESCRCFlag:1;
     PESExtensionFlag:1;
     PESHeaderLength:8;             set to "0x24"
*/

/* EBU data block */


/*
    ReservedForFutureUse:2;
    FieldParity:1;                "1" odd "0" even
    LineOffset:5;                 VBI line offset
    FramingCode:8;                set to "11100100"
    MagazineAndPacketAddress:16;  EBU magazine / packet
    DataBlock[40];                data block
*/

/* EBU data unit */

/*
    DataUnitId:8;      0x2 (non-subtitle), 0x3 (subtitle), 0xc3 (VPS)
    DataUnitLength:8;  set to 0x2C
    EBUDataBlock;      EBU data block
*/

/* EBU data field */

/*
    DataIdentifier:8;  set to 0x10 to 0x1F for EBU data
    *EBUDataUnit;    array of EBU data units
*/

#define HUGE_TIME_DIFFERENCE             90000 /* 1 second difference between
                                                  PTS and STC at 90 KHz*/
#define HUGE_TIME_DIFFERENCE_COUNT_MAX   5
#define DECODER_FAST_COUNT_MAX           5
#define DECODER_SLOW_COUNT_MAX           5
#define NO_STC_COUNT_MAX                 5

/* Obtains the allocated eventid from stored device context for a
 * given event.
 */
#define EventId(y, x)  (y->RegisteredEvents[(x - STTTX_EVENT_DEFAULT)])

/* Private Variables ------------------------------------------------------ */

#if defined(DVD_TRANSPORT_LINK)
U32             Bytes = 0;
#endif
U32             numpackets = 0;

#if defined(DVD_TRANSPORT_LINK)
static U32 LinkTaskDelayTime;

static U32 OverflowBytes = 0;           /* These set by overflow handler */
static clock_t OverflowTime = 0;
#endif

/* The following table inverts hammed 8/4 bytes, returning the nybble value.
   It corrects single bit errors, but flags two or more bit errors with a
   return value of 0xFF.                                                    */
static U8 InvHamming8_4Tab[256] =
        { 0x01,0xFF,0xFF,0x08,0xFF,0x0C,0x04,0xFF,
          0xFF,0x08,0x08,0x08,0x06,0xFF,0xFF,0x08,
          0xFF,0x0A,0x02,0xFF,0x06,0xFF,0xFF,0x0F,
          0x06,0xFF,0xFF,0x08,0x06,0x06,0x06,0xFF,
          0xFF,0x0A,0x04,0xFF,0x04,0xFF,0x04,0x04,
          0x00,0xFF,0xFF,0x08,0xFF,0x0D,0x04,0xFF,
          0x0A,0x0A,0xFF,0x0A,0xFF,0x0A,0x04,0xFF,
          0xFF,0x0A,0x03,0xFF,0x06,0xFF,0xFF,0x0E,
          0x01,0x01,0x01,0xFF,0x01,0xFF,0xFF,0x0F,
          0x01,0xFF,0xFF,0x08,0xFF,0x0D,0x05,0xFF,
          0x01,0xFF,0xFF,0x0F,0xFF,0x0F,0x0F,0x0F,
          0xFF,0x0B,0x03,0xFF,0x06,0xFF,0xFF,0x0F,
          0x01,0xFF,0xFF,0x09,0xFF,0x0D,0x04,0xFF,
          0xFF,0x0D,0x03,0xFF,0x0D,0x0D,0xFF,0x0D,
          0xFF,0x0A,0x03,0xFF,0x07,0xFF,0xFF,0x0F,
          0x03,0xFF,0x03,0x03,0xFF,0x0D,0x03,0xFF,
          0xFF,0x0C,0x02,0xFF,0x0C,0x0C,0xFF,0x0C,
          0x00,0xFF,0xFF,0x08,0xFF,0x0C,0x05,0xFF,
          0x02,0xFF,0x02,0x02,0xFF,0x0C,0x02,0xFF,
          0xFF,0x0B,0x02,0xFF,0x06,0xFF,0xFF,0x0E,
          0x00,0xFF,0xFF,0x09,0xFF,0x0C,0x04,0xFF,
          0x00,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x0E,
          0xFF,0x0A,0x02,0xFF,0x07,0xFF,0xFF,0x0E,
          0x00,0xFF,0xFF,0x0E,0xFF,0x0E,0x0E,0x0E,
          0x01,0xFF,0xFF,0x09,0xFF,0x0C,0x05,0xFF,
          0xFF,0x0B,0x05,0xFF,0x05,0xFF,0x05,0x05,
          0xFF,0x0B,0x02,0xFF,0x07,0xFF,0xFF,0x0F,
          0x0B,0x0B,0xFF,0x0B,0xFF,0x0B,0x05,0xFF,
          0xFF,0x09,0x09,0x09,0x07,0xFF,0xFF,0x09,
          0x00,0xFF,0xFF,0x09,0xFF,0x0D,0x05,0xFF,
          0x07,0xFF,0xFF,0x09,0x07,0x07,0x07,0xFF,
          0xFF,0x0B,0x03,0xFF,0x07,0xFF,0xFF,0x0E };



/* Currently not sure why these are
   needed will take care in future*/
#define ARRAY_MAX 1000

U32 poolarray[ARRAY_MAX];
U32 poolindex = 0;
U32 consarray[ARRAY_MAX];
U32 consindex = 0;

/* Avoids recalculating them every time */
extern U32 ClocksPerSecond;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

static __inline void OutContLines( stttx_context_t  *ThisElem,
                                   STTTX_Page_t     *CurrPage,
                                   U32              OddEvenFlag,
                                   U32              NumberOfLines );

static BOOL HasFilterHit( Handled_Request_t *QueueEnt,
                          STTTX_Page_t      *CurrPage,
                          U32               *MatchedMask);


static BOOL GetPage(STTTX_Request_t    *Filter,
                    BOOL               Even,
                    BOOL               *HeaderFound,
                    U32                *Offset,
                    STTTX_Page_t       *CurrPage);


static void __inline PostEvent(stttx_context_t *ThisElem,
                               data_store_t *PesInfo,
                               STTTX_Handle_t TTXHandle,
                               U32 Event,
                               STTTX_EventData_t *EventData);

static void WaitForData(stttx_context_t *ThisElem,
                        U32 *DataSize,
                        BOOL *FailedPacket,
                        BOOL *HaveData,
                        data_store_t *CurrPes);

static void ProcessVBIOutput(stttx_context_t *ThisElem,
                             page_store_t *Page, U32 Field,
                             U32 NumberOfLines);

static void ProcessSTBOutput(stttx_context_t *ThisElem,
                             page_store_t *Page, U32 Field,
                             U32 NumberOfLines);

#if defined(DVD_TRANSPORT_LINK)
static __inline int FindNextStartCode(U32 *StartAddress,
                                      U32 ByteCount,
                                      U32 *BufferBase,
                                      U32 BufferSize,
                                      U8 **StartCodeAddress,
                                      U8 **NextConsumerPtr);
#endif

/* Debugging functions */
#if defined(STTTX_LINK_DEBUG)
static void DumpLinkBuffer(U8 *Buffer, U32 Size);
#endif



/* Functions -------------------------------------------------------------- */
/****************************************************************************
Name         : AleCaptureEnd

Description  :

Parameters   :

Return Value : 0        success
               -1       failure (couldn't open file)

See Also     : Nothing.
****************************************************************************/
#ifdef ALE_DEBUG
static S32 AleCaptureEnd(void)
{
    S32 file, written;
    U32 count;
    U8 string[15]="AleFile .txt";
    static U8 Number = 'A';

    string[7]=Number;
    Number++;

    count = (U32)((U32)alebuf - (U32)alebuf0);

    /* Write the thing out :) */
    file = (S32)debugopen((char *)string, "wt");
    if (file != -1)
    {
        debugmessage("Writing ALE file\n");
        written = (S32)debugwrite(file, alebuf0, count);
        if (written != count)
        {
            debugmessage("Error writing to file\n");
        }
        debugflush(file);
        debugclose(file);
        debugmessage("Done\n");
    }
    else
    {
        debugmessage("Error opening file\n");
        return -1;
    }

    return 0;
}
#endif

#if defined(STTTX_LINK_DEBUG)
/****************************************************************************
Name         : DumpLinkBuffer()

Description  : Prints out a buffer of U8, hex-dump style (16-bytes per line)

Parameters   : Buffer       Pointer to U8 buffer
               Size         Size of buffer

Return Value : void

See Also     : Nothing.
****************************************************************************/
static void DumpLinkBuffer(U8 *Buffer, U32 Size)
{
    U32 count;

    STTBX_Print(("Buffer contents (%08x to %08x):", (U32)Buffer,
                (U32)Buffer + Size));

    for (count = 0; count < Size; count++)
    {
        if ((count % 16) == 0)
        {
            STTBX_Print(("\n%08x    ", (U32)Buffer + count));
        }
        STTBX_Print(("%02x ", Buffer[count]));
    }
}
#endif
/****************************************************************************
Name         : TTXVBIEnable()

Description  : Routine for enabling/disabling VBI depending on which
               field we should be outputting, and whether we have data
               for it or not. Enabling output without data results in
               last bit value being stretched by DMA, giving bad output.

Parameters   : Reason,              Reason for callback being invoked
               RegistrantName,      Device generating the callback
               Event,               The event which occurred
               EventData,           Any event specific data
               SubscriberData_p,    pointer to device context,

Return Value : void

See Also     : DDTS GNBvd21988.
****************************************************************************/
void TTXVBIEnable( STEVT_CallReason_t Reason,
                  const ST_DeviceName_t RegistrantName,
                  STEVT_EventConstant_t Event,
                  const void *EventData,
                  const void *SubscriberData_p )
{
    ST_ErrorCode_t      error=ST_NO_ERROR;
    STVTG_VSYNC_t       *Field;
    stttx_context_t     *ThisElem;
#ifdef STTTX_SUBT_SYNC_ENABLE
    U32                 STC,ABSDifferenceFromPTS;
    S32                 DifferenceFromPTS;
#endif
    Field = (STVTG_VSYNC_t *)EventData;
    ThisElem = (stttx_context_t *)SubscriberData_p;

#ifdef STTTX_SUBT_SYNC_ENABLE

    /* Call STC only when we have ttx subtitle data ready */
    /* Assuming PTS are always correct */
    if( ( ThisElem->TTXSubTitleData == TRUE) &&
         ((ThisElem->OddDataReady == TRUE) ||
           (ThisElem->EvenDataReady == TRUE ) ) )
    {
        if( STCLKRV_GetSTC( ThisElem->CLKRVHandle, &STC )== ST_NO_ERROR )
        {
            DifferenceFromPTS = ( ThisElem->PTSValue - STC );
            ABSDifferenceFromPTS = abs( DifferenceFromPTS );
            /* Take decision right here
                Case1. Data is too fast/slow to be diplayed return from here
                Case2. Data is too fast/slow many times to be diplayed. Return from here
                       after throwing an event describing whether too slow or too fast.
                Case3. STC with in range.Display!!
                Case4. Should we wait for some more time before display??If so,return this time
                       so that it can be displayed after 40ms.
                Case5. Difference is HUGE, typical PI looping problem
            */

            if ( ( ABSDifferenceFromPTS > ThisElem->SubtSyncOffset)&&
                ( ABSDifferenceFromPTS < HUGE_TIME_DIFFERENCE) )
            {
                /* case 4. */
                if ( ( DifferenceFromPTS > 0) && ( ABSDifferenceFromPTS <
                    ( ThisElem->SubtSyncOffset+EXTRA_SYNC_DELAY ) ) )
                {
                       return;
                }
                else
                {
                    /* case 1. 2. */
                    if ( ( DifferenceFromPTS > 0) && ( ABSDifferenceFromPTS >
                        ( ThisElem->SubtSyncOffset+EXTRA_SYNC_DELAY ) ) )
                    {
                        ThisElem->DecoderFastCount++;
                        if ( ThisElem->DecoderFastCount > DECODER_FAST_COUNT_MAX )
                        {
                            ThisElem->DecoderFastCount = 0;
                            STEVT_Notify(ThisElem->EVTHandle,EventId( ThisElem,
                                                             STTTX_EVENT_DECODER_TOO_FAST), NULL);
                        }
                    }
                    else if ( DifferenceFromPTS < 0 )
                    {
                        ThisElem->DecoderSlowCount++;

                        if ( ThisElem->DecoderSlowCount > DECODER_SLOW_COUNT_MAX )
                        {
                            ThisElem->DecoderSlowCount = 0;
                            STEVT_Notify(ThisElem->EVTHandle,EventId( ThisElem,
                                                             STTTX_EVENT_DECODER_TOO_SLOW), NULL );
                        }
                    }
                    /* Go on to process another PES */
                    /* signal the waiting semaphore */
                    STOS_SemaphoreSignal( ThisElem->ISRComplete_p );
                    return;
                }
            }
            else
            {
                if ( ABSDifferenceFromPTS > HUGE_TIME_DIFFERENCE )
                {
                    /* Currently we are displaying the data, but if time difference
                       is huge many times,an event will be thrown */

                    ThisElem->HugeTimeDiffCount++;
                    if ( ThisElem->DecoderFastCount > HUGE_TIME_DIFFERENCE_COUNT_MAX )
                    {
                        ThisElem->HugeTimeDiffCount = 0;
                        STEVT_Notify(ThisElem->EVTHandle,EventId( ThisElem,
                                                         STTTX_EVENT_TIME_DIFFERENCE_TOO_LARGE), NULL );
                    }
                }
                else
                {
                    /* case 3. */
                    ThisElem->DecoderSlowCount = 0;
                    ThisElem->DecoderFastCount = 0;
                }
            }
        }
        else
        {
            ThisElem->NoSTCCount++;
            if ( ThisElem->NoSTCCount > NO_STC_COUNT_MAX )
            {
                ThisElem->NoSTCCount = 0;
                STEVT_Notify(ThisElem->EVTHandle,EventId( ThisElem,
                                                 STTTX_EVENT_NO_STC), NULL);
            }
        }
    }

#endif

    if ((*Field == STVTG_TOP) && (ThisElem->OddDataReady == TRUE))
    {
        ThisElem->OddDataReady = FALSE;
#if defined(STTTX_USE_DENC)
        STOS_InterruptLock();
#if !defined (ST_5100) && !defined (ST_5105) && !defined (ST_5301) && !defined(ST_5188) && !defined (ST_5107)
        error  = STDENC_OrReg8(ThisElem->DENCHandle,
                               0x22, ThisElem->TTXLineInsertRegs[0]);
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x23,ThisElem->TTXLineInsertRegs[1]);
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x24,ThisElem->TTXLineInsertRegs[2]);
#else
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x24,ThisElem->TTXLineInsertRegs[2]);
        error  = STDENC_OrReg8(ThisElem->DENCHandle,
                               0x25,ThisElem->TTXLineInsertRegs[3]);
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x26,ThisElem->TTXLineInsertRegs[4]);
#endif
        STOS_InterruptUnlock();
#else
        error = STVBI_Enable( ThisElem->VBIHandle );
#endif
#if defined (ST_5105) || defined(ST_7020) || defined(ST_5188) || defined (ST_5107)
        STOS_SemaphoreSignal( ThisElem->ISRComplete_p );
#endif

#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
        STOS_SemaphoreSignal( ThisElem->ISRTimeOutWakeUp_p );
#endif
        if (error != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "Failed to enable odd VBI field! 0x%08x\n",
                      (U32)error));
        }
    }
    else if ((*Field == STVTG_BOTTOM) && (ThisElem->EvenDataReady == TRUE))
    {
        ThisElem->EvenDataReady = FALSE;

#if defined(STTTX_USE_DENC)
        STOS_InterruptLock();
#if !defined (ST_5100) && !defined (ST_5105) && !defined (ST_5301) && !defined(ST_5188) && !defined (ST_5107)
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x24, ThisElem->TTXLineInsertRegs[2]);
        error  = STDENC_OrReg8(ThisElem->DENCHandle,
                               0x25, ThisElem->TTXLineInsertRegs[3]);
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x26, ThisElem->TTXLineInsertRegs[4]);
#else
        error  = STDENC_OrReg8(ThisElem->DENCHandle,
                               0x22,ThisElem->TTXLineInsertRegs[0]);
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x23,ThisElem->TTXLineInsertRegs[1]);
        error |= STDENC_OrReg8(ThisElem->DENCHandle,
                               0x24,ThisElem->TTXLineInsertRegs[2]);
#endif
        STOS_InterruptUnlock();
#else
        error = STVBI_Enable(ThisElem->VBIHandle);
#endif
#if defined (ST_5105) || defined(ST_7020) || defined(ST_5188) || defined (ST_5107)
        STOS_SemaphoreSignal( ThisElem->ISRComplete_p );
#endif
#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
        STOS_SemaphoreSignal(ThisElem->ISRTimeOutWakeUp_p);
#endif
        if (error != ST_NO_ERROR)
        {
            STTBX_Report( (STTBX_REPORT_LEVEL_INFO,
                         "Failed to enable even VBI field! 0x%08x\n",
                         (U32)error) );
        }
    }
}

/****************************************************************************
Name         : PostEvent()

Description  : Routine for handling valid event transitions on a particular
               teletext handle / pti slot.

Parameters   : ThisElem,    pointer to device context,
               PesInfo,     pointer to PES information structure.
               TTXHandle,   handle of notifee.
               Event,       event to be generated.

Return Value : void

See Also     : Nothing.
****************************************************************************/

static void __inline PostEvent(stttx_context_t *ThisElem,
                               data_store_t *PesInfo,
                               STTTX_Handle_t TTXHandle,
                               U32 Event,
                               STTTX_EventData_t *EventData)
{
    ST_ErrorCode_t Error;
    BOOL Post = FALSE;                  /* Always assume we won't post */
#if defined(STTTX_DELIVER_PES)
    /* This is used so that we don't give the user a pointer to a
     * structure which may be changing as they access it.
     */
    STTTX_EventData_t PESEventData;
#endif

    /* Don't perform event processing unless this device instance has
     * been registered with the event handler.
     */
    if (ThisElem->InitParams.EVTDeviceName == NULL)
        return;

    /* Check event to be notified */
    switch( Event )
    {
        case STTTX_EVENT_DATA_OVERFLOW:             /* Data events */
        case STTTX_EVENT_PES_LOST:                  /* PES events  */
        case STTTX_EVENT_PES_NOT_CONSUMED:
        /* This bit's ugly, but... */
#if !defined(STTTX_DELIVER_PES)
        case STTTX_EVENT_PES_INVALID:
        case STTTX_EVENT_PES_AVAILABLE:
            /* Check whether we need to notify this event */
            if ( PesInfo->LastEvent != Event )
            {
                Post = TRUE;
                PesInfo->LastEvent = Event;
            }
            break;
#else
        /* need to deliver PES for STB in parallel with VBI insertion */
        case STTTX_EVENT_PES_AVAILABLE:
            /* PostEvent is only called with this event in the middle of
             * the TTXHandlerTask. Due to STEVT_Notify, this function
             * will have to complete before TTXHandlerTask can continue
             * with its job. Therefore, it's safe to give a reference to
             * the RxLinearBuff (AKA LocalBuffer in TTXHandlerTask). If
             * any of these change, this code should be reviewed.
             */
            if ((TRUE == ThisElem->TasksActive) &&
                (TRUE == CurrPes->TasksActive))
            {
                pesEventData.Data = PesInfo->RxLinearBuff;
                pesEventData.DataLength =
                    (PesInfo->RxLinearBuff[PCKT_LEN_MSB_POS] << 8 ) +
                     PesInfo->RxLinearBuff[PCKT_LEN_LSB_POS] +
                     PCKT_HDR_LENGTH;
                pesEventData.Handle = ThisElem->BaseHandle + PesInfo->Object;
                EventData = &pesEventData;
                Post = TRUE;
            }
            break;
        /* Always need this event for manual insertion tracking */
        case STTTX_EVENT_PES_INVALID:
            /* XXX ??? */
            TTXHandle = ThisElem->BaseHandle + PesInfo->Object;
#endif
        case STTTX_EVENT_PACKET_DISCARDED:          /* EBU events */
        case STTTX_EVENT_PACKET_CONSUMED:
        case STTTX_EVENT_VPS_DATA:
            Post = TRUE;                /* Always send EBU events */
            break;
        default:
            break;
    }

    ALE_PRINT_DISABLE;

    switch( Event )
    {
        case STTTX_EVENT_DATA_OVERFLOW:             /* Data events */
             ALE_Print (st,(st,"\nSTTTX_EVENT_DATA_OVERFLOW"));
             break;
        case STTTX_EVENT_PES_LOST:                  /* PES events */
             ALE_Print (st,(st,"\nSTTTX_EVENT_PES_LOST"));
             break;
        case STTTX_EVENT_PES_NOT_CONSUMED:
             ALE_Print (st,(st,"\nSTTTX_EVENT_PES_NOT_CONSUMED"));
             break;
        case STTTX_EVENT_PES_INVALID:
             ALE_Print (st,(st,"\nSTTTX_EVENT_PES_INVALID"));
             break;
        case STTTX_EVENT_PES_AVAILABLE:
             ALE_Print (st,(st,"\nSTTTX_EVENT_PES_AVAILABLE"));
             break;
        case STTTX_EVENT_PACKET_DISCARDED:          /* EBU events */
             ALE_Print (st,(st,"\nSTTTX_EVENT_PACKET_DISCARDED"));
             break;
        case STTTX_EVENT_PACKET_CONSUMED:
             ALE_Print (st,(st,"\nSTTTX_EVENT_PACKET_CONSUMED"));
             break;
        default:
            break;
    }

    if ( Post )                         /* Post event? */
    {
        if (EventData == NULL)
        {
            /* Allows the user to know which slot has generated the event */
            Error = STEVT_Notify( ThisElem->EVTHandle, EventId(ThisElem, Event),
                                  &TTXHandle );
        }
        else
        {
            Error = STEVT_Notify(ThisElem->EVTHandle,
                                 EventId(ThisElem, Event),
                                 EventData);
        }
    }
}

#if STTTX_PES_MODE == PES_CAPTURE_MODE
/****************************************************************************
Name         : CaptureInit

Description  : Gets memory for pes buffer for capture/insert mode
               If inserting, loads file into buffer

Parameters   : PesBuffer_p   pointer to byte-array for PES packets

Return Value : 0        success
               -1       failure (no memory)
               -2       failure (file-related)

See Also     : Nothing.
****************************************************************************/
int CaptureInit(U8 **PesBuffer_p)
{
    /* Get memory */
    *PesBuffer_p = (U8 *)STOS_MemoryAllocate(
                    (partition_t *)system_partition, STTTX_PESBUFFER_SIZE );
    if (*PesBuffer_p == NULL)
    {
        debugmessage("Unable to allocate memory for PesBuffer_p!\n");
        return -1;
    }

    return 0;
}

/****************************************************************************
Name         : CaptureEnd

Description  : If capturing, writes the PES buffer to disk

Parameters   : PesBuffer_p   pointer to byte-array for PES packets
               BufferUsed_p  pointer to U32 for amount of buffer used

Return Value : 0        success
               -1       failure (couldn't open file)

See Also     : Nothing.
****************************************************************************/
S32 CaptureEnd(U8 *PesBuffer_p, U32 *BufferUsed_p)
{
    S32 file, written;

    /* Write the thing out :) */

    file = (S32)debugopen(STTTX_PES_FILE, "wb");
    if (file != -1)
    {
        debugmessage("Writing PES file\n");
        written = (S32)debugwrite(file, PesBuffer_p, *BufferUsed_p);
        if (written != *BufferUsed_p)
        {
            debugmessage("Error writing to file\n");
        }
        debugflush(file);
        debugclose(file);
        debugmessage("Done\n");
        *BufferUsed_p = 0;
    }
    else
    {
        debugmessage("Error opening file\n");
        return -1;
    }

    return 0;
}

/****************************************************************************
Name         : CaptureMain

Description  : Does the work of either inserting a packet, or adjusting the
               buffer position to keep them safe in the PesBuffer_p

Parameters   : ThisElem         STB object
               CurrPes          structure detailing current pes packet (this is
                                 set or read accordingly)
               PesBuffer_p      pointer to byte-array for PES packets
               buffer_position  where in the PesBuffer_p we are at present
               BytesWritten     the size of the PES packet (read or set
                                 accordingly)
               CaptureAction    called twice during capture-mode; this is which
                                 action we should do at this call

Return Value : void

See Also     : Nothing.
****************************************************************************/
void CaptureMain( stttx_context_t *ThisElem,
                        data_store_t    *CurrPes,
                        U8 *PesBuffer_p,
                        U32 *BufferPosition_p,
                        U32 BytesWritten)
{
    U32 DataSize;

    if ((PesBuffer_p == NULL) || (BufferPosition_p == NULL))
        return;

    DataSize = BytesWritten + sizeof(BytesWritten);
    if (DataSize >= (STTTX_PESBUFFER_SIZE - *BufferPosition_p))
    {
        CaptureEnd(PesBuffer_p, BufferPosition_p);
    }

    memcpy(&PesBuffer_p[*BufferPosition_p],
           &BytesWritten, sizeof(BytesWritten));
    *BufferPosition_p += sizeof(BytesWritten);
    memcpy(&PesBuffer_p[*BufferPosition_p],
           CurrPes->RxLinearBuff, BytesWritten);
    *BufferPosition_p += BytesWritten;
}
#endif /* STTTX_PES_MODE == PES_CAPTURE_MODE */

#if defined(DVD_TRANSPORT_PTI)
/****************************************************************************
Name         : GetCurrentPacketSize

Description  : Looks at the packet present in the circular buffer, and
               returns the size of the packet (+ header)

               NOTE: no checking is done for packet validity!

Parameters   : CirclePtr        Current circular consumer
               CircleBase       Base of the circular buffer
               CircleSize       Size of the circular buffer

Return Value : Packet size, plus 6 bytes for header.

See Also     : WaitForData
****************************************************************************/
static __inline U32 GetCurrentPacketSize(U8 *CirclePtr,
                                         U8 *CircleBase,
                                         U32 CircleSize)
{
    U32 temp = (U32)CirclePtr - (U32)CircleBase;
    U32 temp2;

    /* Extract packet size from header */
    temp2 = (CircleBase[(temp + 4) % CircleSize] << 8) +
                CircleBase[(temp + 5) % CircleSize];
    /* Add on packet header */
    temp2 += 6;
    return temp2;
}
#endif

/****************************************************************************
Name         : WaitForData

Description  : Waits for a PES packet on the appropriate input (depending
               on defined source). It will copy the PES data into the
               TmpLinearBuff array (in the CurrPes) structure. The
               returned DataSize should not exceed MESSAGE_DATA_SIZE.

Parameters   : ThisElem         pointer to the control block
               DataSize         holds the amount of data obtained
               FailedPacket     failed to obtain packet (not timeout)
               HaveData         Whether we failed to get any data
               CurrPes          PES structure to fill

Return Value : none

See Also     : TTXHandlerTask
****************************************************************************/
static void WaitForData(stttx_context_t *ThisElem,
                        U32 *DataSize,
                        BOOL *FailedPacket,
                        BOOL *HaveData,
                        data_store_t *CurrPes)
{
#if defined(DVD_TRANSPORT_STPTI)
    ST_ErrorCode_t Error;
    STPTI_Buffer_t Buffer;
#endif
#if defined(DVD_TRANSPORT_DEMUX)
    ST_ErrorCode_t Error;
    STDEMUX_Buffer_t Buffer;
#endif
#if defined(DVD_TRANSPORT_PTI)
    clock_t timeout;
#endif
#if defined(DVD_TRANSPORT_LINK)
    S8  temp = 0, got_start = 0;
    U32 BytesInQueue, PacketLength;

    BOOL GotWholePacket = FALSE;
    U32 consumer_index, nostart_count;
    /* Poll link at 1ms intervals */
    U32 delaytime = ClocksPerSecond / 1000;

    /* Handle for post event; not used, but needs to be there. */
    STTTX_Handle_t  TTXHandle;
#else
    U32 CurrentPacketSize = 0;
#endif

    /* Initial values */
    *FailedPacket = TRUE;
    *HaveData = FALSE;

#if defined(DVD_TRANSPORT_LINK)
    /* pass back base + index */
    TTXHandle = ThisElem->BaseHandle + CurrPes->Object;
#endif

    /* See what we need to wait for */
    switch (CurrPes->SourceParams.DataSource)
    {
        case STTTX_SOURCE_PTI_SLOT:
#if defined(DVD_TRANSPORT_PTI)        
                *HaveData = TRUE;

                /* signalled by PTI; 0.5s timeout */
                timeout = STOS_time_plus(STOS_time_now(), ClocksPerSecond >> 1);
                STOS_SemaphoreWaitTimeOut(CurrPes->WaitForPES_p, &timeout);

                if ((CurrPes->TasksActive) && (ThisElem->TasksActive))
                {
                    CurrentPacketSize =
                        GetCurrentPacketSize(CurrPes->CircleCons,
                                             CurrPes->CircleBase,
                                             CurrPes->CircleSize);

                    CurrentPacketSize += 6;     /* Header */
                    if (CurrentPacketSize < MESSAGE_DATA_SIZE)
                    {
                        /* copy from circular buffer */
                        *FailedPacket =
                            pti_copy_pes_packet_to_linear(
                                CurrPes->CircleBase,
                                CurrPes->CircleSize,
                                &CurrPes->CircleCons,
                                CurrPes->CircleProd,
                                CurrPes->TmpLinearBuff,
                                MESSAGE_DATA_SIZE,
                                DataSize );

                        if (*FailedPacket == FALSE)
                            numpackets++;
                        else
                        {
                            STTBX_Print(("Failed packet. Processed %i packets.\n",
                                    numpackets));
                        }
                    }
                    else
                    {
                        /* Can't deal with a packet of this size, dump
                         * it.
                         */
                        STTBX_Print(("Packet too big (%i), flushing stream\n",
                                    CurrentPacketSize));
                        pti_flush_stream( CurrPes->Slot );
                    }

                    /* Copy or consumer update fail? */
                    if ((*FailedPacket == TRUE) ||
                        pti_set_consumer_ptr(CurrPes->Slot,
                                          CurrPes->CircleCons))
                    {
                        /* try and tidy up PTI */
                        pti_flush_stream( CurrPes->Slot );
                    }
                }
#endif

#if defined(DVD_TRANSPORT_LINK)
                *HaveData = TRUE;

                /* The only time this should be signalled when using link is
                 * when the close function wants to wake us up. (If that
                 * changes, we'll need to change this, obviously.)
                 */
                temp = STOS_SemaphoreWaitTimeOut(CurrPes->WaitForPES_p,
                                              TIMEOUT_IMMEDIATE);
                BytesInQueue = 0;
                GotWholePacket = FALSE;
                got_start = 0;
                nostart_count = 0;

                while ((temp == -1) &&
                       (GotWholePacket == FALSE) &&
                       (CurrPes->Playing))
                {
                    if ((OverflowBytes != 0) || (OverflowTime != 0))
                    {
                        PostEvent(ThisElem, CurrPes, TTXHandle,
                                  STTTX_EVENT_DATA_OVERFLOW, NULL);
                        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Event: STTTX_EVENT_DATA_OVERFLOW"));

#if defined(STTTX_LINK_DEBUG)
                        STTBX_Print(("OVERFLOW\nBytes %i, overflow bytes: %i\n",
                                Bytes, OverflowBytes));
                        STTBX_Print(("Circle base: %08x, producer: %08x, consumer: %08x\n",
                                (U32)CurrPes->CircleBase, (U32)CurrPes->CircleProd,
                                (U32)CurrPes->CircleCons));
                        STTBX_Print(("Packets: %i\n", numpackets));
                        DumpLinkBuffer(CurrPes->CircleBase, CurrPes->CircleSize);
                        debugexit(0);
                        debugbreak();
#endif
                    }

#if defined(STTTX_LINK_DEBUG)
                    if (nostart_count > 5)
                    {
                        STTBX_Print(("Circle base: %08x, producer: %08x, consumer: %08x\n",
                                (U32)CurrPes->CircleBase, (U32)CurrPes->CircleProd,
                                (U32)CurrPes->CircleCons));
                        STTBX_Print(("Packets: %i, nostart_count: %i\n",
                                    numpackets, nostart_count));
                        DumpLinkBuffer(CurrPes->CircleBase, CurrPes->CircleSize);
                        debugexit(0);
                        debugbreak();
                    }
#endif

                    pti_get_producer(CurrPes->Slot, &CurrPes->CircleProd);
                    consumer_index = CurrPes->CircleCons - CurrPes->CircleBase;

                    pti_get_written_size(CurrPes->Slot, &Bytes);

                    if (Bytes < 6)
                    {
                        /* Do nothing */
                    }
                    else if (got_start == 0)
                    {
                        /* No known start code, more than 6 bytes */
                        if ((CurrPes->CircleBase[(consumer_index)] != 0) ||
                            (CurrPes->CircleBase[(consumer_index + 1) % CurrPes->CircleSize] != 0) ||
                            (CurrPes->CircleBase[(consumer_index + 2) % CurrPes->CircleSize] != 1) ||
                            (CurrPes->CircleBase[(consumer_index + 3) % CurrPes->CircleSize] != 0xbd)
                           )
                        {
                            U8 *startcode;

                            pti_get_producer(CurrPes->Slot, &CurrPes->CircleProd);
                            pti_get_written_size(CurrPes->Slot, &Bytes);

                            /* Not on a start code, trigger a search */
                            if (FindNextStartCode((U32 *)CurrPes->CircleCons,
                                                  Bytes,
                                                  (U32 *)CurrPes->CircleBase,
                                                  CurrPes->CircleSize,
                                                  &startcode,
                                                  &CurrPes->CircleCons))
                            {
                                pti_set_consumer_ptr(CurrPes->Slot, CurrPes->CircleCons);

                                got_start = 1;
                                nostart_count = 0;

                                consumer_index = CurrPes->CircleCons - CurrPes->CircleBase;

                                PacketLength = (CurrPes->CircleBase[(consumer_index + 4) % CurrPes->CircleSize] << 8) |
                                               (CurrPes->CircleBase[(consumer_index + 5) % CurrPes->CircleSize]);
                                PacketLength += 6;

                                /* Recovery code left in, although it shouldn't
                                 * be reached.
                                 */
                                if ((PacketLength > MESSAGE_DATA_SIZE) ||
                                    (PacketLength == 6))
                                {
                                    /* Packet is either too small, or
                                     * larger than we can handle at this
                                     * point.
                                     */

                                    if (consumer_index >= CurrPes->CircleSize)
                                        CurrPes->CircleCons = CurrPes->CircleBase;
                                    else
                                        CurrPes->CircleCons++;
                                    pti_set_consumer_ptr(CurrPes->Slot, CurrPes->CircleCons);
                                    got_start = 0;
                                }
                            }
                            else
                            {
                                /* No start code found, just update pointer */
                                pti_set_consumer_ptr(CurrPes->Slot,
                                                     CurrPes->CircleCons);
                                nostart_count++;
                            }
                        }
                        else
                        {
                            PacketLength = (CurrPes->CircleBase[(consumer_index + 4) % CurrPes->CircleSize] << 8) |
                                           (CurrPes->CircleBase[(consumer_index + 5) % CurrPes->CircleSize]);
                            PacketLength += 6;
                            got_start = 1;
                            nostart_count = 0;

                            consumer_index = CurrPes->CircleCons - CurrPes->CircleBase;
                            pti_set_consumer_ptr(CurrPes->Slot,
                                                 CurrPes->CircleCons);

                            /* Recovery code left in, although it shouldn't
                             * be reached.
                             */
                            if ((PacketLength > MESSAGE_DATA_SIZE) ||
                                (PacketLength == 6))
                            {
                                /* Packet is either too small, or
                                 * larger than we can handle at this
                                 * point.
                                 */

                                if (consumer_index >= CurrPes->CircleSize)
                                    CurrPes->CircleCons = CurrPes->CircleBase;
                                else
                                    CurrPes->CircleCons++;

                                pti_set_consumer_ptr(CurrPes->Slot,
                                                     CurrPes->CircleCons);
                                got_start = 0;
                            }
                        }
                    }

                    if (got_start == 1)
                    {
                        /* Know we're on a start code, more than 6 bytes, packet length
                         * has been set
                         */
                        pti_get_written_size(CurrPes->Slot, &Bytes);
                        pti_get_producer(CurrPes->Slot, &CurrPes->CircleProd);

                        /* Got at least enough bytes for this packet */
                        if (PacketLength <= Bytes)
                        {
                            GotWholePacket = TRUE;
                            got_start = 0;
                            break;
                        }
                    }
                    STOS_TaskDelay(delaytime);
                }

                /* We exited via enough bytes for a packet, rather than
                 * someone waking us up?
                 */
                if (GotWholePacket == TRUE)
                {
                    /* copy from circular buffer */
                    *FailedPacket =
                        pti_copy_pes_packet_to_linear(
                            CurrPes->CircleBase,
                            CurrPes->CircleSize,
                            &CurrPes->CircleCons,
                            CurrPes->CircleProd,
                            CurrPes->TmpLinearBuff,
                            MESSAGE_DATA_SIZE,
                            DataSize );

                    numpackets++;

                    /* Consumer pointer is updated by copy to linear */
                    pti_set_consumer_ptr(CurrPes->Slot, CurrPes->CircleCons);

#if defined(STTTX_DEBUG)
                    if (*FailedPacket)
                    {
                        /* pti_flush_stream doesn't work for link block. */
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                     "Error copying to linear buffer\n"));
                    }
#endif

                }
#endif
                break;

        case STTTX_SOURCE_STPTI_SIGNAL:
#if defined(DVD_TRANSPORT_STPTI)
                *HaveData = TRUE;
                /* 0.5s timeout for new data. */
                Error = STPTI_SignalWaitBuffer(
                    CurrPes->SourceParams.Source_u.STPTISignal_s.Signal,
                    &Buffer,
                    500);
                if (Error != ST_NO_ERROR)
                {
                    if (Error != ST_ERROR_TIMEOUT)
                    {
                        *FailedPacket = TRUE;
                        STTBX_Print(("Got failed packet: 0x%x\n", Error));
                    }
                    else
                    {
                        STTBX_Print(("Got timeout waiting for signal on buffer\n"));
#if defined(STTBX_PRINT)
                        {
                            U16 count, count2,count3;
                            BOOL Overflow;
                            STPTI_HardwareFIFOLevels("PTI", &Overflow,
                                                     &count, &count2, &count3);
                            STTBX_Print(("Overflow: %s; input level: %i\n",
                                        (Overflow == TRUE?"yes":"no"), count));
                        }
#endif
                        *FailedPacket = FALSE;
                        *HaveData = FALSE;
                    }
                }
                else
                {
                    /* We don't actually care about the four below */
                    U8 PesFlags, TrickMode;
                    STPTI_TimeStamp_t PTS, DTS;
                    Error = STPTI_BufferExtractPesPacketData(Buffer, &PesFlags,
                                            &TrickMode,
                                            &CurrentPacketSize, &PTS, &DTS);
                    if (Error == ST_NO_ERROR)
                    {
                        /* Also take into account the header bytes which
                         * get copied.
                         */
#ifdef STTTX_SUBT_SYNC_ENABLE
                        ThisElem->PTSValue = PTS.LSW;
#endif
                        CurrentPacketSize += 6;

                        if (CurrentPacketSize < MESSAGE_DATA_SIZE)
                        {
                            /* Each signal should be associated with
                             * only one buffer, so we don't have to
                             * check if it's correct. Make sure we only
                             * read as much as we have room for.
                             */
                            Error = STPTI_BufferReadPes(Buffer,
                                CurrPes->TmpLinearBuff, MESSAGE_DATA_SIZE,
                                NULL, 0, DataSize,
                                STPTI_COPY_TRANSFER_BY_MEMCPY);

                            if (Error != ST_NO_ERROR)
                            {
                                STTBX_Print(("Got error %i reading\n", Error));
                            }
                        }
                        else
                        {
                            Error = STPTI_BufferReadPes(Buffer,
                                NULL, 0,
                                NULL, 0, DataSize,
                                STPTI_COPY_TRANSFER_BY_MEMCPY);
 
                            if (Error != ST_NO_ERROR)
                            {
                                STTBX_Print(("Got error %i reading\n", Error));
                            }
                        }    

                    }
                    else
                    {
                        STTBX_Print(("BufferExtractPesPacketData: %i\n", Error));
                    }

                    if (Error != ST_NO_ERROR)
                    {
                        *FailedPacket = TRUE;
                        STTBX_Print(("Got failed packet: error %i\n", Error));

                        /* Dump stream */
                        Error = STPTI_BufferFlush(Buffer);
                        if (Error != ST_NO_ERROR)
                        {
                            STTBX_Print(("*** Failed to flush buffer! *** %i\n",
                                        Error));
                        }
                    }
                    else
                    {
                        *FailedPacket = FALSE;
                    }
                }
#endif
                break;

        case STTTX_SOURCE_STDEMUX_SIGNAL:
#if defined(DVD_TRANSPORT_DEMUX)
                *HaveData = TRUE;
                /* 0.5s timeout for new data. */
                Error = STDEMUX_SignalWaitBuffer(
                    CurrPes->SourceParams.Source_u.STDEMUXSignal_s.Signal,
                    &Buffer,
                    500);

                if (Error != ST_NO_ERROR)
                {
                    if (Error != ST_ERROR_TIMEOUT)
                    {
                        *FailedPacket = TRUE;
                        STTBX_Print(("Got failed packet: 0x%x\n", Error));
                    }
                    else
                    {
                        STTBX_Print(("Got timeout waiting for signal on buffer\n"));
                        *FailedPacket = FALSE;
                        *HaveData = FALSE;
                    }
                }
                else
                {
                    /* We don't actually care about the four below */
                    U8 PesFlags, TrickMode;
                    STDEMUX_TimeStamp_t PTS, DTS;
                    Error = STDEMUX_BufferExtractPesPacketData(Buffer, &PesFlags,
                                            &TrickMode,
                                            &CurrentPacketSize, &PTS, &DTS);
                    if (Error == ST_NO_ERROR)
                    {
                        /* Also take into account the header bytes which
                         * get copied.
                         */
#ifdef STTTX_SUBT_SYNC_ENABLE
                        ThisElem->PTSValue = PTS.LSW;
#endif
                        CurrentPacketSize += 6;

                        if (CurrentPacketSize < MESSAGE_DATA_SIZE)
                        {
                            /* Each signal should be associated with
                             * only one buffer, so we don't have to
                             * check if it's correct. Make sure we only
                             * read as much as we have room for.
                             */
                            Error = STDEMUX_BufferReadPes(Buffer,
                                CurrPes->TmpLinearBuff, MESSAGE_DATA_SIZE,
                                NULL, 0, DataSize,
                                STDEMUX_COPY_TRANSFER_BY_MEMCPY);

                            if (Error != ST_NO_ERROR)
                            {
                                STTBX_Print(("Got error %i reading\n", Error));
                            }
                        }
                        else
                        {
                            Error = STDEMUX_BufferReadPes(Buffer,
                                NULL, 0,
                                NULL, 0, DataSize,
                                STPTI_COPY_TRANSFER_BY_MEMCPY);
 
                            if (Error != ST_NO_ERROR)
                            {
                                STTBX_Print(("Got error %i reading\n", Error));
                            }
                        }    
						
                    }
                    else
                    {
                        STTBX_Print(("BufferExtractPesPacketData: %i\n", Error));
                    }

                    if (Error != ST_NO_ERROR)
                    {
                        *FailedPacket = TRUE;
                        STTBX_Print(("Got failed packet: error %i\n", Error));

                        /* Dump stream */
                        Error = STDEMUX_BufferFlush(Buffer);
                        if (Error != ST_NO_ERROR)
                        {
                            STTBX_Print(("*** Failed to flush buffer! *** %i\n",
                                        Error));
                        }
                    }
                    else
                    {
                        *FailedPacket = FALSE;
                    }
                }
#endif
                break;
        case STTTX_SOURCE_USER_BUFFER:
                *HaveData= TRUE;
                STOS_SemaphoreWait(CurrPes->SourceParams.Source_u.UserBuf_s.DataReady_p);
                if (CurrPes->Playing == TRUE)
                {
                    *DataSize = *CurrPes->SourceParams.Source_u.UserBuf_s.BufferSize;
                    if (*DataSize < MESSAGE_DATA_SIZE)
                    {
                        memcpy(CurrPes->TmpLinearBuff,
                               *CurrPes->SourceParams.Source_u.UserBuf_s.PesBuf_p,
                               *DataSize);

                        *FailedPacket = FALSE;
                    }
                    else
                    {
                        *FailedPacket = TRUE;
                    }
                }
                break;
    }

    if (*FailedPacket == TRUE)
    {
        *HaveData = FALSE;
    }
}

#if defined(DVD_TRANSPORT_LINK)
/***************************************************************
Name :  FindNextStartCode
Description : Find next start code in memory (CPU search). Assumes
              it's working on a circular buffer.
Parameters:
 * EndAddress can not be 0
 * returns 0 if no start code found, 1 otherwise
 * returns the exact byte address of the first byte of the start code
 * EndAddress - StarAddress must be at least 1
*****************************************************************/
static __inline int FindNextStartCode(U32 *StartAddress,
                                      U32 ByteCount,
                                      U32 *BufferBase,
                                      U32 BufferSize,
                                      U8 **StartCodeAddr,
                                      U8 **NextConsumerPtr)
{
    U32 WrapRemaining, BytesRemaining;
    U32 WrapStore, ByteStore;
    U8 CompareArray[] = { 0x00, 0x00, 0x01, 0xBD };
    U8 *StartCodePtr = NULL;
    register U8 *Ptr;
    register U8 *ComparePtr = CompareArray;

    /* Setup */
    *NextConsumerPtr = (U8 *)StartAddress;
    BytesRemaining = ByteCount;
    WrapRemaining = BufferSize - (U32)((U32)StartAddress - (U32)BufferBase);

    for (Ptr = (U8 *)StartAddress; BytesRemaining > 0; WrapRemaining--,
                                                       BytesRemaining--)
    {
        if (*Ptr == *ComparePtr)
        {
            /* Keep track of where the possible start code begins */
            if (StartCodePtr == NULL)
            {
                StartCodePtr = Ptr;
                WrapStore = WrapRemaining;
                ByteStore = BytesRemaining;
            }
            ComparePtr++;
            if (ComparePtr == &CompareArray[4])
                break;
        }
        else
        {
            ComparePtr = CompareArray;
            /* Have we started matching...? */
            if (StartCodePtr != NULL)
            {
                /* Yes, so reset back to the start of it (incremented
                 * below so we don't keep looping). We don't just stay
                 * at the current pos. for reasons detailed in GNBvd13784.
                 */
                Ptr = StartCodePtr;
                StartCodePtr = NULL;
                WrapRemaining = WrapStore;
                BytesRemaining = ByteStore;
            }
        }

        Ptr++;
        if (WrapRemaining == 1)
            Ptr = (U8 *)BufferBase;

        /* Ensure the consumer always stays at least 6 bytes back from
         * the end.
         */
        if (BytesRemaining >= 6)
            (*NextConsumerPtr) = Ptr;
    }

    /* We cannot return 'true' if the caller doesn't have the
     * length-bytes yet. Since the NextConsumerPtr stopped being updated
     * some time back (if we're this short of data), we just don't
     * update. We'll find it next time we're called.
     */
    if (StartCodePtr != NULL && BytesRemaining >= 2)
    {
        *StartCodeAddr = StartCodePtr;
        *NextConsumerPtr = StartCodePtr;
        return 1;
    }

    return 0;
}
#endif

/****************************************************************************
Name         : TTXHandlerTask()

Description  : Teletext Handler Task, managing the transfer of PID
               filtered PES data from the PTI when semaphored. One task
               per handle opened.

Parameters   : void *Data points to the stttx_context_t linked list
               element for the current PTI device instance (recast).

Return Value : void

See Also     : None
****************************************************************************/
void TTXHandlerTask( void *Data )
{
    data_store_t    *CurrPes;                       /* pointer into Pes[i] */
    stttx_context_t *ThisElem;                      /* main data pointer */
    page_store_t    *Page_p;
    STTTX_Page_t    *CurrPage;                      /* odd/even page ptr. */
    U32             FieldLineNum;                   /* extracted from packet */
    U32             ValidLineNum;                   /* masked to 0..31 range */
    U32             PacketLength;
    U32             ExtractPoint;                   /* index into linear buff. */
    STTTX_Handle_t  TTXHandle;
    U32             BytesWritten = 0;
    BOOL            HaveData;
    BOOL            FailedPacket;
    U32             NumberOfLines = 0;
    U8              *LocalBuffer = NULL;
    partition_t     *partition_p;

#if (STTTX_PES_MODE == PES_CAPTURE_MODE)
    U32             BufferUsed = 0;
    U32             BufferPosition = 0;
    U8              *PesBuffer_p = NULL;

    CaptureInit(&PesBuffer_p);
#endif
#ifdef ST_OSLINUX
    dma_addr_t      DMA_Handle;
#endif
    /* main data pointer */
    CurrPes = (data_store_t *)Data;
    ThisElem = (stttx_context_t *)CurrPes->Element;
    /* pass back base + index */
    TTXHandle = ThisElem->BaseHandle + CurrPes->Object;    
    partition_p = ThisElem->InitParams.DriverPartition;

    LocalBuffer = (U8 *)STOS_MemoryAllocate( partition_p, MESSAGE_DATA_SIZE );
    if (LocalBuffer == NULL)
    {
        STTBX_Print(("!!! No memory for local buffer !!!\n"));
        return;
    }
    CurrPes->RxLinearBuff = LocalBuffer;

#ifdef ST_OSLINUX
    Page_p = (page_store_t *)dma_alloc_coherent(NULL,
                                    (U32)(sizeof(page_store_t)) ,
                                    &DMA_Handle,
                                    GFP_KERNEL);
#else
   Page_p = (page_store_t *) STOS_MemoryAllocate( partition_p,
                                                (U32)(sizeof(page_store_t) ) );
#endif
    if ( Page_p == NULL )
    {
        STTBX_Print(("!!! No memory for Page_p !!!\n"));
        return;
    }
    CurrPage = &Page_p->OddPage;    /* point to odd page */

#if !defined (ST_7020) && !defined (ST_5105) && !defined(ST_5188) && !defined(ST_5107)
    /* Set up the hardware: */
    /* 27MHz DMA delay ticks + 2 */
    ThisElem->BaseAddress[STTTX_OUT_DELAY] = 2;

    /* Output, not Input */
    /* It is surely required for 5516, MODE reg is out for 5528 */
    ThisElem->BaseAddress[STTTX_MODE] = STTTX_OUTPUT_ENABLED;

    /* disable in/out completion */
    ThisElem->BaseAddress[STTTX_INT_ENABLE] = STTTX_DISABLE_INTS;
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    ThisElem->BaseAddress[STTTX_OUT_ENABLE] = 0x05;
#endif

#if defined(DVD_TRANSPORT_LINK)
    LinkTaskDelayTime = ClocksPerSecond / 1000;
#endif

    STOS_TaskEnter (NULL);
    /* while not closed */
    while ((CurrPes->TasksActive) && (ThisElem->TasksActive))
    {
        ttx_Message_t *CurrMsg;

        /* Go to sleep until we're told to do something */
        STOS_SemaphoreWait(CurrPes->WakeUp_p);

        /* CurrPes->PESPacketReadCounter = 0; */
        Page_p->OddPage.ValidLines = 0;    /* start with empty page */
        Page_p->EvenPage.ValidLines = 0;   /* for Fields 1 and 2 */
        Page_p->Flags = CurrPes->Object;

        /* Have we been told to quit? */
        while ((CurrPes->HandlerRunning) &&
               (CurrPes->TasksActive) &&
               (ThisElem->TasksActive))
        {
            clock_t timeoutvalue;

            /* Update data. Make sure we wake each second regardless. */
            timeoutvalue = STOS_time_plus(STOS_time_now(), ClocksPerSecond);
            CurrMsg = STOS_MessageQueueReceiveTimeout(CurrPes->PESQueue_p,
                                              &timeoutvalue);

            if (CurrMsg == NULL)
            {
                continue;
            }

            /* If not running or timeout, continue around loop */
            if (!CurrPes->HandlerRunning)
            {
                /* DDTS 30604 'memory leak' */
                if (!CurrPes->HandlerRunning)
                {
                    STOS_MessageQueueRelease(CurrPes->PESQueue_p, CurrMsg);
                    /* In case there exist more than One message */
                    while((CurrMsg = STOS_MessageQueueReceiveTimeout(CurrPes->PESQueue_p,
                                                              TIMEOUT_IMMEDIATE))!=NULL)
                    STOS_MessageQueueRelease(CurrPes->PESQueue_p, CurrMsg);
                }
                continue;
            }

            BytesWritten = CurrMsg->BytesWritten;
            FailedPacket = CurrMsg->FailedPacket;
            /* Transport handlers should take care of making sure they
             * fit in a local buffer.
             */
            memcpy(LocalBuffer, CurrMsg->Data, BytesWritten);
            STOS_MessageQueueRelease(CurrPes->PESQueue_p, CurrMsg);

#if defined(STTTX_DEBUG1)
            STOS_InterruptLock();
            StttxBufferSize--;
            STOS_InterruptUnlock();
#endif
            HaveData = TRUE;

            if (FailedPacket == FALSE)
            {
                if ((consindex + 1) < ARRAY_MAX)
                {
                    consarray[consindex++] = STOS_time_now();
                    consarray[consindex++] = (U32)CurrPes->RxLinearBuff;
                }
            }

            if ((CurrPes->HandlerRunning == FALSE) ||
                (CurrPes->TasksActive == FALSE) ||
                (ThisElem->TasksActive == FALSE))
            {
                continue;
            }

            if ((HaveData == FALSE) || (FailedPacket == TRUE))
            {
                if (FailedPacket == TRUE)
                {
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_NOT_CONSUMED, NULL);
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Event: STTTX_EVENT_PES_NOT_CONSUMED"));
                }
            }
            else
            {
#if STTTX_PES_MODE == PES_CAPTURE_MODE
                CaptureMain(ThisElem, CurrPes, PesBuffer_p, &BufferPosition,
                            BytesWritten);
#endif

               /* check for a valid PES packet (start code 0x000001 ) */
               if( ( CurrPes->RxLinearBuff[0] != 0 )  ||
                    ( CurrPes->RxLinearBuff[1] != 0 )  ||
                    ( CurrPes->RxLinearBuff[2] != 1 ) )
                {
                    /* STTTX_EVENT_PES_INVALID - invalid startcode */
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_INVALID, NULL);
                    /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                              "Event: STTTX_EVENT_PES_INVALID (SC)"));*/
                    continue;                    /* discard (whatever it is) */
                }

                /* Teletext data arrives on private_stream_1.  If otherwise
                   marked, discard the whole packet.                         */
                if( CurrPes->
                    RxLinearBuff[STREAM_ID_POS] !=
                                    PRIVATE_STREAM_1 )
                {
                    /* STTTX_EVENT_PES_INVALID - invalid streamid */
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_INVALID, NULL);
                    /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "Event: STTTX_EVENT_PES_INVALID (SID)"));*/
                    continue;               /* not teletext data */
                }

                /* read declared length of PES packet and add header length */
                PacketLength =
                    ( CurrPes->RxLinearBuff[
                        PCKT_LEN_MSB_POS] << 8 ) +
                    CurrPes->RxLinearBuff[
                        PCKT_LEN_LSB_POS] +
                            PCKT_HDR_LENGTH;

                if ((PacketLength > PES_POINTER_SIZE ) ||
                    (PacketLength > BytesWritten))
                {
                    /* STTTX_EVENT_PES_INVALID - invalid packet length */
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_INVALID, NULL);
                    /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "Event: STTTX_EVENT_PES_INVALID (Size)"));*/
                    continue;               /* packet too big (corrupt) */
                }

                /* Looks like a valid teletext packet in the linear buffer */

                Page_p->Flags = CurrPes->Object;
                /* Skip over the first line - this is the PES packet header,
                   ahead of teletext line 0 (header) data.                   */

                ExtractPoint = PES_DATA_START_POS;      /* Teletext data start */

                /* PES_data_field() {
                        data_identifier
                        for (i = 0; i < N; i++) {
                            data_unit_id
                            data_unit_length
                            data_field()
                        }
                   }
                */

                /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                              "New packet: data_identifier = 0x%02x "
                              "packet_length = %02x %02x",
                              CurrPes->RxLinearBuff[ExtractPoint-1],
                              CurrPes->RxLinearBuff[4],
                              CurrPes->RxLinearBuff[5]));*/


                /* Check the data_identifier (teletext substream) is within
                 * the valid range.z
                 */
                if ( CurrPes->RxLinearBuff[ExtractPoint-1] < 0x10 ||
                     CurrPes->RxLinearBuff[ExtractPoint-1] > 0x1F )
                {
                    /* STTTX_EVENT_PES_INVALID - invalid data identifier */
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_INVALID, NULL);
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "Event: STTTX_EVENT_PES_INVALID (Sub)"));
                    continue;
                }

                /* Only process PES packets that match the current data
                 * identifier we are filtering on.
                 */
                if ( CurrPes->RxLinearBuff[ExtractPoint-1] !=
                     CurrPes->DataIdentifier )
                {
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_INVALID, NULL);
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "Event: STTTX_EVENT_PES_INVALID (DI)"));
                    continue;
                }
                else
                {
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_AVAILABLE, NULL);
                }

                /* We iterate over the following elements:

                        data_unit_id        0x2 (non-subtitle) or 0x3 (subtitle)
                        data_unit_length    0x2c for EBU teletext data
                        data_field()        contains 44 bytes teletext data
                */

                while( ExtractPoint < PacketLength ) /* loop on line data */
                {
                    /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                  "data_unit_id      0x%02x "
                                  "data_unit_length  0x%02x",
                                  CurrPes->RxLinearBuff[ExtractPoint],
                                  CurrPes->RxLinearBuff[ExtractPoint+1]));
                    ALE_PRINT_DISABLE;
                    ALE_Print(st,(st,"\ndata_unit_id = 0x%02x    data_unit_length = 0x%02x",
                                  CurrPes->RxLinearBuff[ExtractPoint],
                                  CurrPes->RxLinearBuff[ExtractPoint+1]));*/

                    /* Check to ensure this data is either EBU subtitle or
                     * EBU non-subtitle data.
                     */

                    if ( ( CurrPes->        /* EBU subtitle data */
                           RxLinearBuff[ExtractPoint] !=
                           0x02) &&         /* EBU non-subtitle data */
                         ( CurrPes->
                           RxLinearBuff[ExtractPoint] !=
                           0x03 ) &&
                         ( CurrPes->
                           RxLinearBuff[ExtractPoint] !=
                           VPS_DATA_ID ) )          /* VPS data */
                    {
                        /* STTTX_EVENT_PACKET_DISCARDED - invalid data unit id */
                        PostEvent(ThisElem, CurrPes, TTXHandle,
                                  STTTX_EVENT_PACKET_DISCARDED, NULL);
                        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                      "Event: STTTX_EVENT_PACKET_DISCARDED (0x%02x)",
                                      CurrPes->RxLinearBuff[ExtractPoint]));*/
                        ExtractPoint += STTTX_LINESIZE; /* step to next line */
                        continue;
                    }
                    else
                    {
                        if ( CurrPes->
                         RxLinearBuff[ExtractPoint] ==
                         0x03)
                        {
                            ThisElem->TTXSubTitleData = TRUE;
                        }
                        else
                        {
                            ThisElem->TTXSubTitleData = FALSE;
                        }
                        /* STTTX_EVENT_PACKET_CONSUMED - teletext data is ok */
                        PostEvent(ThisElem, CurrPes, TTXHandle,
                                  STTTX_EVENT_PACKET_CONSUMED, NULL);
                        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                      "Event: STTTX_EVENT_PACKET_CONSUMED"));*/

                        /* VPS data found? */
                        if (CurrPes->RxLinearBuff[ExtractPoint] == VPS_DATA_ID)
                        {
                            STTTX_EventData_t EventData;

                            /* Copy the data out (255 bytes max; see
                             * EBU structure commented above)
                             */
                            memcpy(CurrPes->VpsDataBuffer,
                                   &CurrPes->RxLinearBuff[ExtractPoint + 2],
                                   CurrPes->RxLinearBuff[ExtractPoint + 1]);

                            /* step to next line */
                            ExtractPoint += STTTX_LINESIZE;

                            /* Notify any subscribers */
                            EventData.Handle = TTXHandle;
                            EventData.DataLength =
                                CurrPes->RxLinearBuff[ExtractPoint + 1];
                            EventData.Data = (void *)CurrPes->VpsDataBuffer;
                            PostEvent(ThisElem, CurrPes, TTXHandle,
                                      STTTX_EVENT_VPS_DATA,
                                      &EventData);

                            /* And go back to the loop start, to make
                             * sure it's got valid data-id.
                             */
                            continue;
                        }
                    }


                    /* Determine the field and line number */

                    FieldLineNum = CurrPes->            /* Field & Line number */
                        RxLinearBuff[ExtractPoint + LINE_FIELD_POS];

                    ValidLineNum =                      /* ensure in range 0..31 */
                        FieldLineNum & LINE_NUMB_MASK;  /* stripping off field bit */

#if defined(STTTX_DEBUG) || defined(ALE_DEBUG)
                    {
                        U8 PcktAddr1, PcktAddr2, PacketNo, Magazine;
                        U8 PageNumb1, PageNumb2, TTPageNo;
                        S32 CacheIndex;

                        PcktAddr1 = InvHamming8_4Tab[CurrPes->
                                    RxLinearBuff[(ExtractPoint + PACKET_ADDR_POS1)] ];
                        PcktAddr2 = InvHamming8_4Tab[CurrPes->
                                    RxLinearBuff[(ExtractPoint + PACKET_ADDR_POS2)] ];
                        PacketNo = ( PcktAddr1 >> 3 ) |
                                   ( PcktAddr2 << 1 );
                        Magazine = PcktAddr1 & MAGAZINE_MASK;


                        /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                      "Magazine %d Packet %d Line %d [%s]",
                                      Magazine,
                                      PacketNo,
                                      FieldLineNum & LINE_NUMB_MASK,
                                      (FieldLineNum & ODD_FIELD_FLAG) != 0 ? "ODD" : "EVEN"
                                     ));
                        ALE_Print(st,(st,
                                      "\nMagazine %d Packet %d Line %d [%s]",
                                      Magazine,
                                      PacketNo,
                                      FieldLineNum & LINE_NUMB_MASK,
                                      (FieldLineNum & ODD_FIELD_FLAG) != 0 ? "ODD" : "EVEN"
                                     ));*/


                        if (PacketNo == 0)
                        {
                            PageNumb1 = InvHamming8_4Tab[CurrPes->
                                    RxLinearBuff[ExtractPoint + PAGE_NO_UNIT_POS]];
                            PageNumb2 = InvHamming8_4Tab[CurrPes->
                                    RxLinearBuff[ExtractPoint + PAGE_NO_TENS_POS]];
                            TTPageNo = PageNumb1 |
                                       ( PageNumb2 <<
                                         NYBBLE_SHIFT_CNT );
                            /*STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                          "Page 0x%02x", TTPageNo));*/

                            CacheIndex = ((PcktAddr1 & 0x07) * 100) +
                                        (((TTPageNo >> 4) & 0x0F) * 10) +
                                        (TTPageNo & 0x0F) - 100;

                        }

                    }
#endif

                    if( (FieldLineNum & ODD_FIELD_FLAG) != 0 ) /* odd field? */
                    {
                        /* If we switched pages, or we've got a line
                         * that's already set, output.
                         */
                        if ((CurrPage != &Page_p->OddPage) ||
                            ((CurrPage->ValidLines & (VALID_LINES_BIT <<
                                    ValidLineNum)) != 0))
                        {
                            if (CurrPes->Mode == STTTX_VBI)
                            {
                                if (CurrPage == &Page_p->OddPage)
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                else
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                            }
                            else if (CurrPes->Mode == STTTX_STB_VBI)
                            {
                                if (CurrPage == &Page_p->OddPage)
                                {
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                }
                                else
                                {
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                                }
                            }
                            else if (CurrPes->Mode == STTTX_STB)
                            {
                                if (CurrPage == &Page_p->OddPage)                                 
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);                                  
                                else               
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                            }
                            CurrPage->ValidLines = 0;
                            CurrPage = &Page_p->OddPage;    /* point to odd page */
                            NumberOfLines = 0;
                        }
                    }
                    else
                    {
                        if ((CurrPage != &Page_p->EvenPage) ||
                            ((CurrPage->ValidLines & (VALID_LINES_BIT <<
                                    ValidLineNum)) != 0))
                        {
                            if (CurrPes->Mode == STTTX_VBI)
                            {
                                if (CurrPage == &Page_p->OddPage)
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                else
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                            }
                            else if (CurrPes->Mode == STTTX_STB_VBI)
                            {
                                if (CurrPage == &Page_p->OddPage)
                                {
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                }
                                else
                                {
                                    ProcessVBIOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                                }
                            }
                            else if (CurrPes->Mode == STTTX_STB)
                            {
                                if (CurrPage == &Page_p->OddPage)
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_ODD_FIELD_FLAG, NumberOfLines);
                                else
                                    ProcessSTBOutput(ThisElem, Page_p, STTTX_EVEN_FIELD_FLAG, NumberOfLines);
                            }
                            CurrPage->ValidLines = 0;
                            CurrPage = &Page_p->EvenPage;   /* point to even page */
                            NumberOfLines = 0;
                        }
                    }

                    ALE_PRINT_DISABLE;

                    /* Store the line in the buffer */

                    memcpy(CurrPage->Lines[NumberOfLines],
                           &CurrPes->RxLinearBuff[ ExtractPoint],
                           STTTX_LINESIZE);

                    NumberOfLines++;

                    /* flag line as present, and step to next line */
                    CurrPage->ValidLines |= (VALID_LINES_BIT << ValidLineNum);
                    ExtractPoint += STTTX_LINESIZE;
                }
            }
        }

        /* Ensure that the message queue is now empty of data. We know
         * that it won't be refilled from the other end, since the
         * pooling task doesn't set the flag to tell us to exit until
         * it's come out of the data-retrieval loop. No task delay,
         * since we shouldn't be polling for very long, and it's
         * important to complete the job as soon as possible.
         */
        CurrMsg = STOS_MessageQueueReceiveTimeout(CurrPes->PESQueue_p,
                                          TIMEOUT_IMMEDIATE);
        while (CurrMsg != NULL)
        {
            STOS_MessageQueueRelease(CurrPes->PESQueue_p, CurrMsg);
            CurrMsg = STOS_MessageQueueReceiveTimeout(CurrPes->PESQueue_p,
                                              TIMEOUT_IMMEDIATE);
        }

        /* Now release this semaphore, so that the channel can be
         * started again if required.
         */
        STOS_SemaphoreSignal(CurrPes->Stopping_p);
    }
    STOS_TaskExit (NULL);
    STTBX_Print(("PES 0x%08x\tTTXHandlerTask ending\n", (U32)CurrPes));

    /* problem in 3.0.0, memory leak */
    STOS_MemoryDeallocate(partition_p, LocalBuffer);
#ifdef ST_OSLINUX
   dma_free_coherent(NULL,
                    (U32)(sizeof(page_store_t)),
                    Page_p,
                    DMA_Handle);
#else
    STOS_MemoryDeallocate(partition_p, Page_p);
#endif
#if STTTX_PES_MODE == PES_CAPTURE_MODE
    CaptureEnd(PesBuffer_p, &BufferPosition);
#endif

#ifdef ALE_DEBUG
    AleCaptureEnd();
    alebuf = alebuf0;
#endif
}

/****************************************************************************
Name         : ProcessVBIOutput()

Description  : Takes completed teletext pages and outputs via Teletext
               DMA hardware.

Parameters   :  ThisElem points to a context structure for the current element
                Page contains the details of the page to be output

Return Value : void

See Also     : None
 ****************************************************************************/

static void ProcessVBIOutput(stttx_context_t *ThisElem,
                             page_store_t *Page, U32 Field, U32 NumberOfLines)
{

#if defined(STTTX_DEBUG)
    static U8 cnt = 0;

    *pio3cfg = 128;
#endif

#if defined(STTTX_USE_DENC)
    STOS_InterruptLock();

#if defined(ST_5510)
    STDENC_OrReg8(ThisElem->DENCHandle, DENC_BASE_ADDRESS + 0x04,
                  STTTX_DMA_DELAY_TIME - 2);
    STDENC_OrReg8(ThisElem->DENCHandle, DENC_BASE_ADDRESS + 0x8,
                  0x04);

#elif defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined(ST_7710) ||\
      defined (ST_7100) || defined (ST_5301) || defined(ST_7109) || defined(ST_5188) || defined(ST_5107)

    /* Enable TTX */
    STDENC_WriteReg8(ThisElem->DENCHandle,0x06,0x02);
    /* TTX not MV */
    STDENC_WriteReg8(ThisElem->DENCHandle,0x08,0x24);

    STDENC_WriteReg8(ThisElem->DENCHandle,0x40, 0x30);
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    STDENC_WriteReg8(ThisElem->DENCHandle,0x41, 0x8a);
#endif

#elif defined (ST_7020)

    /* Enable TTX */
    STDENC_OrReg8(ThisElem->DENCHandle,0x06,0x02);
    /* TTX not MV */
    STDENC_OrReg8(ThisElem->DENCHandle,0x08,0x04);

    STDENC_WriteReg8(ThisElem->DENCHandle,0x40, 0x20);

#else
/* ST_5512 || ST_5518 || ST_5514 || ST_5516 || ST_5517 */

#if !defined(ST_5514)
    /* 5514 has a DENC problem; these writes will be done at init time. */
    /* Enable TTX */
    STDENC_OrReg8(ThisElem->DENCHandle, 0x6, 0x02);
    /* TTX not MV */
    STDENC_OrReg8(ThisElem->DENCHandle, 0x8, 0x04);

#endif

    /* STDENC_OrReg8(ThisElem->DENCHandle, 0x40, 0x20);*/
    /* DDTS 28415 leads to two other ddts mem leak in pti and no display
       in USE_DENC mode on any platform */
    {
        U8 rv;
        STDENC_ReadReg8(ThisElem->DENCHandle,0x40,&rv);

        rv &= 0x1F;
        rv |= 0x20;

        STDENC_WriteReg8(ThisElem->DENCHandle,0x40,rv);
    }

    /* We may also want to do something with DencReg[0x41] at some point;
     * controls framing code.
     */
#endif

    STOS_InterruptUnlock();
#endif

#if defined(STTTX_DEBUG)
    if (cnt++ == 4)
        *pio3set = 128;
#endif

    if (Field == STTTX_ODD_FIELD_FLAG)
        OutContLines( ThisElem,                   /* output odd field lines */
                      &Page->OddPage,
                      STTTX_ODD_FIELD_FLAG,
                      NumberOfLines );

    if (Field == STTTX_EVEN_FIELD_FLAG)
        OutContLines( ThisElem,                   /* output even field lines */
                      &Page->EvenPage,
                      STTTX_EVEN_FIELD_FLAG,
                      NumberOfLines );

#if defined(STTTX_DEBUG)
    if (cnt == 8)
    {
        *pio3clr = 128;
        cnt = 0;
    }
#endif

    /* disable in/out completion */
#if !defined (ST_7020) && !defined (ST_5105) && !defined(ST_5188) && !defined(ST_5107)
    ThisElem->BaseAddress[STTTX_INT_ENABLE] = STTTX_DISABLE_INTS;
#endif
}

/****************************************************************************
Name         : ProcessSTBOutput()

Description  : STB Output Task, searching active filters initially for a
               match of Object number/Handle with the PES packet received.

Parameters   : void *Data points to the stttx_context_t linked list
               element for the current PTI device instance (recaste).

Return Value : void

See Also     : None
****************************************************************************/
static void ProcessSTBOutput(stttx_context_t *ThisElem,
                             page_store_t *Page, U32 Field, U32 NumberOfLines)
{
    Handled_Request_t   *QueueEnt;                  /* ptr. to current req. */
    U32                 i;
    BOOL                FilterHit;
    U32                 MatchedMask = 0;
    /* is filter a PAGE_COMLETE type? */
    BOOL                WholePage;
    U32                 Tag;
    STTTX_Page_t        *Odd, *Even;


    /* if there are no requests then return */
    if (ThisElem->NumActiveRequests == 0)
        return;

    for( i = 0;                               /* loop on filter requests */
         i < ThisElem->InitParams.NumRequestsAllowed; i++ )
    {
        QueueEnt = &ThisElem->RequestsPtr[i]; /* point to current element */

        if( ( QueueEnt->Request.RequestTag == /* inactive filter request? */
              STTTX_FREE_REQUEST_TAG ) ||
            ( QueueEnt->OpenOff !=            /* Object/Handle mismatch? */
              Page->Flags ) )
        {
            continue;                         /* no processing on page */
        }

        /* A filter request is active for the object from which the
         * current teletext page has been collected.  Check for a
         * filter hit, and call the notify function if met.
         */

        /* Set the wholepage filter (page_complete) flag */
        if ((QueueEnt->Request.RequestType &
            (STTTX_FILTER_PAGE_COMPLETE_ODD | STTTX_FILTER_PAGE_COMPLETE_EVEN)) != 0)
            WholePage = TRUE;
        else
            WholePage = FALSE;

         /* Call different filtering functions depending on whether the
          * current filter is for a whole page or a normal filter.
          */
        if(WholePage == FALSE)
        {
            /* Apply filter to odd OR even pages */
            if (Field == STTTX_ODD_FIELD_FLAG)
                FilterHit = HasFilterHit(QueueEnt,
                                         &Page->OddPage, &MatchedMask);
            else
                FilterHit = HasFilterHit(QueueEnt,
                                         &Page->EvenPage, &MatchedMask);

            if (FilterHit)
            {
                /* Copy pages to caller-supplied area */
                if (Field == STTTX_ODD_FIELD_FLAG)
                {
                    *QueueEnt->Request.OddPage = Page->OddPage;
                    QueueEnt->Request.OddPage->ValidLines = MatchedMask;
                }
                else
                {
                    *QueueEnt->Request.EvenPage = Page->EvenPage;
                    QueueEnt->Request.EvenPage->ValidLines = MatchedMask;
                }

                /* Copy request parameters */
                Tag  = QueueEnt->Request.RequestTag;
                Odd  = QueueEnt->Request.OddPage;
                Even = QueueEnt->Request.EvenPage;

                /* Copy the matched criteria mask */
                if (Field == STTTX_ODD_FIELD_FLAG)
                    Even->ValidLines = 0;
                else
                    Odd->ValidLines = 0;

                /* Over-ride matched mask, since it doesn't seem to be
                 * right for non-whole pages        XXX
                 */
                {
                    U32 j, mask = 0;
                    for (j = 0; j < NumberOfLines; j++)
                        mask |= (1 << j);

                    if (Field == STTTX_ODD_FIELD_FLAG)
                        Odd->ValidLines  = mask;
                    else
                        Even->ValidLines = mask;
                }

                /* Cancel active request */
                QueueEnt->OpenOff = REQ_SLOT_UNUSED;
                QueueEnt->Request.RequestTag = STTTX_FREE_REQUEST_TAG;
                ThisElem->NumActiveRequests--;

#ifdef ALE_DEBUG
                ALE_PRINT_DISABLE;
                if (Field == STTTX_ODD_FIELD_FLAG)
                {
                    ALE_Print (st,(st,"\nALE: Sending ODD STB page. ValidLines: 0x%x",
                                 Odd->ValidLines));
                }
                else
                {
                    ALE_Print (st,(st,"\nALE: Sending EVEN STB page. ValidLines: 0x%x",
                                 Even->ValidLines));
                }
#endif

                /* call NotifyFunction() */
                ( QueueEnt->Request.NotifyFunction )( Tag, Odd, Even );
            }  /* FilterHit==TRUE */
        }
        else
        {
            /* WholePage == TRUE - filtering for a whole page */
            U32 j;

            /* is it an even page and are we filtering for even pages? */
            if (Field != STTTX_ODD_FIELD_FLAG && QueueEnt->EvenOffset != -1 )
            {
                if (GetPage(&QueueEnt->Request, TRUE,&QueueEnt->HeaderFound,
                            &QueueEnt->EvenOffset, &Page->EvenPage) == TRUE)
                {
                    /* if getPage == True then the end of the filterd
                     * page has been found
                     */
                    QueueEnt->Request.EvenPage->ValidLines = 0;

                    /* set the valid lines mask according to how many
                     * lines were collected (offset)
                     */
                    for (j = 0; j < QueueEnt->EvenOffset; j++)
                        QueueEnt->Request.EvenPage->ValidLines |= (1 << j);

                    /* mark that we are no longer looking for even field
                     * info for this page
                     */
                    QueueEnt->EvenOffset = -1;
                    STTBX_Print(("Collected even page.\n"));
                }
            }
            else if (Field == STTTX_ODD_FIELD_FLAG && QueueEnt->OddOffset != -1)
            {
                /* do the same as above but for odd pages */
                if(GetPage(&QueueEnt->Request, FALSE,&QueueEnt->HeaderFound,
                           &QueueEnt->OddOffset, &Page->OddPage) == TRUE)
                {
                    QueueEnt->Request.OddPage->ValidLines = 0;
                    for (j=0; j < QueueEnt->OddOffset; j++)
                        QueueEnt->Request.OddPage->ValidLines|= (1 << j);
                    QueueEnt->OddOffset = -1;
                    STTBX_Print(("Collected odd page.\n"));
                }
            }

            /* when one of the odd or even offsets == -1 the request has been met */
            if (QueueEnt->OddOffset == -1 || QueueEnt->EvenOffset == -1)
            {
                if (QueueEnt->OddOffset == -1)
                {
                    QueueEnt->Request.EvenPage->ValidLines = 0;
                    for (j = 0; j < QueueEnt->EvenOffset; j++)
                        QueueEnt->Request.EvenPage->ValidLines |= (1 << j);
                    QueueEnt->EvenOffset = -1;
                    STTBX_Print(("Collected even page.\n"));
                }
                else
                {
                    QueueEnt->Request.OddPage->ValidLines = 0;
                    for (j=0; j < QueueEnt->OddOffset; j++)
                        QueueEnt->Request.OddPage->ValidLines|= (1 << j);
                    QueueEnt->OddOffset = -1;
                    STTBX_Print(("Collected odd page.\n"));
                }
                /* activate notify function */
                STTBX_Print(("Complete Page\n"));

                /* Copy request parameters */
                Tag  = QueueEnt->Request.RequestTag;
                Odd  = QueueEnt->Request.OddPage;
                Even = QueueEnt->Request.EvenPage;

                /* Cancel active request */
                QueueEnt->OpenOff = REQ_SLOT_UNUSED;
                QueueEnt->Request.RequestTag = STTTX_FREE_REQUEST_TAG;
                ThisElem->NumActiveRequests--;

                ( QueueEnt->Request.NotifyFunction )(Tag, Odd, Even );
            }
        }
    }
}

#if !defined (ST_7020) && !defined (ST_5105) && !defined(ST_5188) && !defined(ST_5107)
/****************************************************************************
Name         : XferCompleteISR()

Description  : Interrupt Service Routine for Teletext In/Out Complete

Parameters   : void *Data points to the stttx_context_t linked list
               element for the current Init instance (recaste).

Return Value : void

See Also     : None
*****************************************************************************/
STOS_INTERRUPT_DECLARE(XferCompleteISR, Data)
{
    U32             IntStatus;
    stttx_context_t *ThisElem = (stttx_context_t *)Data;

#ifdef USE_PROFILE
    PRO_RecordItTimeStamp( TTX_IT_INDEX );
#endif

    IntStatus =                                    /* read interrupt status */
        ThisElem->BaseAddress[STTTX_INT_STATUS];

    ThisElem->BaseAddress[                         /* disable STTTX interrupts */
        STTTX_INT_ENABLE] = STTTX_DISABLE_INTS;

    if( ( IntStatus &                              /* In/Out Complete? */
                STTTX_STATUS_COMPLETE ) != 0 )
    {
        STOS_SemaphoreSignal( ThisElem->ISRComplete_p ); /* advise of completion */
#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
        /* success fully done, tell this to time out task */
        STOS_SemaphoreSignal(ThisElem->TTXDMATimeOut_p);
#endif

    }

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);

}
#endif /* !(ST_5105/07/88, ST_7020) */

/************************************************************************
* Time out task to find out that ttx insertion for one field was done,
* But dma complete interrupt didn't occur in ~2 milliseconds
* ttx dma should aborted in this case(frequenlty in 5100, rare in 5516)
* This task initialized at init time and should wait for VBI CallBack to
* signal ISRTimeOutWakeUp
************************************************************************/
#if defined (TTXDMA_NOT_COMPLETE_WORKAROUND)
void ISRTimeOutTask(void *Data)
{
    stttx_context_t *ThisElem = (stttx_context_t *)Data;
    clock_t timeout;
    S32 TimeOutError;

    STOS_TaskEnter (NULL);
    /* while not closed */
    while (ThisElem->TimeOutTaskActive)
    {
        STOS_SemaphoreWait(ThisElem->ISRTimeOutWakeUp_p);

        if (ThisElem->TimeOutTaskActive == FALSE)
            break; /* task terminated */

        timeout = STOS_time_plus(STOS_time_now(), ClocksPerSecond/400);
        TimeOutError = STOS_SemaphoreWaitTimeOut(ThisElem->TTXDMATimeOut_p,
                               &timeout );

        if (ThisElem->TimeOutTaskActive == FALSE)
            break; /* task terminated */

        if (TimeOutError == (-1))
        {
            /* ttx dma not complete in time */
            STOS_InterruptLock();
            ThisElem->BaseAddress[                         /* disable STTTX interrupts */
                STTTX_INT_ENABLE] = STTTX_DISABLE_INTS;
            /* dma abort */
            ThisElem->BaseAddress[STTTX_ABORT] = 0x55555555;
            ThisElem->BaseAddress[STTTX_OUT_DELAY] = 2;
            ThisElem->BaseAddress[STTTX_MODE] = STTTX_OUTPUT_ENABLED;
            STOS_InterruptUnlock();
            STOS_SemaphoreSignal( ThisElem->ISRComplete_p ); /* advise of completion */
#if defined(STTTX_DEBUG1)
            DmaWaCnt++;
#endif
        }
        else
        {
            /* ttx dma complete in time signaled by ISR */
            /* Do nothing then */
        }
    }
    STOS_TaskExit (NULL);
}
#endif


/****************************************************************************
Name         : OutContLines()

Description  : Locates and outputs contiguous lines of teletext data to the
               Teletext DMA engine for injection during VBI.

Parameters   : stttx_context_t *ThisElem pointing to current instance,
               data_store_t *CurrPes pointing to the current Pes and
               U32 OddEvenFlag for programming STTTX_MODE hardware register.

Return Value : void

See Also     : None
 ****************************************************************************/

static __inline void OutContLines( stttx_context_t  *ThisElem,
                                   STTTX_Page_t     *CurrPage,
                                   U32              OddEvenFlag,
                                   U32              NumberOfLines )
{
    clock_t TimeoutPoint;
    U32 ValidLines = CurrPage->ValidLines;
    int ret;
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    ST_ErrorCode_t error;
    U8 *TtxBufferOrg_p=NULL,*TtxBuffer_p=NULL;
    U32 LineMask=0,j=0,k=0;
    STVMIX_VBIViewPortParams_t  VBIVPDisplayParamsOdd,VBIVPDisplayParamsEven;
#endif

#if !defined(STTTX_USE_DENC)
    STVBI_DynamicParams_t VBI;
#else
    U32 i =0;
    U8 reg, bitmask;
#endif

    /* There are NumberOfLines contiguous lines of data commencing at
     * index i to output marked with OddEvenFlag via the Teletext DMA
     * engine.  Note that the DMA interrupt is for IDLE, which is why it
     * is not enabled until AFTER the DMA transfer commences.  Otherwise
     * it will fire immediately!
     */
    ALE_PRINT_DISABLE;
    ALE_Print (st,(st,"\nALE: Sending packets. ValidLines: 0x%06x  Number of Lines: %u",
                    ValidLines, NumberOfLines));

    if (ValidLines > 0)
    {
        /* Set up the transfer, before we configure the DENC. Direction
         * is output, with OddEvenFlag field.
         */
#ifdef ST_7020
        for(i=0;i<NumberOfLines;i++)
        {
            memcpy( (void *)(ThisElem->TTXAVMEM_Buffer_Address+(12*i)),(void *)CurrPage->Lines[i], 46);
        }
#elif defined (ST_5105) || defined(ST_5188) || defined(ST_5107)
        TtxBufferOrg_p = (U8 *)STOS_MemoryAllocate(ThisElem->InitParams.DriverPartition, (48*18)+255);
        memset( (void*)(TtxBufferOrg_p), 0x00, (48*18)+255);
        /*Buffer must be 32 byte aligned...*/
        TtxBuffer_p = (U8*)((U32)(TtxBufferOrg_p+255)&(~255));

        if (TtxBuffer_p == NULL)
        {
            STTBX_Print(("!!! No memory for TtxBuffer_p !!!\n"));
            return;
        }
        LineMask = ValidLines>>7;
        while(LineMask!=0)
        {
            if(LineMask&0x01)
            {
               memcpy( (void *)(TtxBuffer_p+(48*j)),&CurrPage->Lines[k][3], 43);
               k++;
            }
            LineMask=LineMask>>1;
            j++;
        }
        if (OddEvenFlag == STTTX_ODD_FIELD_FLAG)
        {
            error = STVMIX_EnableVBIViewPort(ThisElem->VBIVPDisplayOdd);
            VBIVPDisplayParamsOdd.Source_p      = (U8*)(TtxBuffer_p);
            VBIVPDisplayParamsOdd.LineMask      = 0x3ffff;
            error = STVMIX_SetVBIViewPortParams(ThisElem->VBIVPDisplayOdd,&VBIVPDisplayParamsOdd);
        }
        else
        {
            error = STVMIX_EnableVBIViewPort(ThisElem->VBIVPDisplayEven);
            VBIVPDisplayParamsEven.Source_p     = (U8*)(TtxBuffer_p);
            VBIVPDisplayParamsEven.LineMask     = 0x3ffff;
            error = STVMIX_SetVBIViewPortParams(ThisElem->VBIVPDisplayEven,&VBIVPDisplayParamsEven);
        }
#else
        /* MODE is not in *5528* */
        ThisElem->BaseAddress[STTTX_MODE] =
                                STTTX_OUTPUT_ENABLED | OddEvenFlag;

        /* DMA start address */
#ifdef ARCHITECTURE_ST40
        ThisElem->BaseAddress[STTTX_DMA_ADDRESS] = (U32)CurrPage->Lines[0] & 0x1fffffff;
#else
        ThisElem->BaseAddress[STTTX_DMA_ADDRESS] = (U32)CurrPage->Lines[0];
#endif
        /* DMA length/activate */
        STTBX_Print(("NumberOfLines * STTTX_LINESIZE=0x%x\n",NumberOfLines * STTTX_LINESIZE));
        ThisElem->BaseAddress[STTTX_DMA_COUNT] =
                                NumberOfLines * STTTX_LINESIZE;

#endif /* ST_7020 */

#if !defined(STTTX_USE_DENC)
        VBI.VbiType = STVBI_VBI_TYPE_TELETEXT;
        VBI.Type.TTX.Mask = TRUE;
        /* LineCount = 0 tells STVBI to look at linemask instead */
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
        VBI.Type.TTX.LineCount = 16;
        VBI.Type.TTX.LineMask = 0x3fffc0;
#else
        VBI.Type.TTX.LineCount = 0;
        VBI.Type.TTX.LineMask = ValidLines;
#endif
        VBI.Type.TTX.Field =
/* On 5100 vysnc polarity is inverted */
#if !defined(ST_5100) && !defined(ST_5105) && !defined (ST_5301) && !defined(ST_5188) && !defined(ST_5107)
        (OddEvenFlag == STTTX_ODD_FIELD_FLAG) ? TRUE : FALSE;
#else
        (OddEvenFlag == STTTX_ODD_FIELD_FLAG) ? FALSE : TRUE;
#endif
        STVBI_SetDynamicParams( ThisElem->VBIHandle, &VBI);
        /* Enabling is now done in TTXVBIEnable */
#else
        /* Store this for TTXVBIEnable to print */
        ThisElem->NumberOfTTXLines = NumberOfLines;

        /* poke denc register directly */
#if defined(STTTX_DEBUG)
#endif
        STOS_InterruptLock();

        /* Clear the DENC registers and the 'shadows' used by
         * TTXVBIEnable. For offset 0x22, Only bits 0-3 have line
         * values. */

        STDENC_MaskReg8(ThisElem->DENCHandle,  0x22, (U8)(~0x0f), 0);
        STDENC_WriteReg8(ThisElem->DENCHandle, 0x23, 0);
        STDENC_WriteReg8(ThisElem->DENCHandle, 0x24, 0);
        STDENC_WriteReg8(ThisElem->DENCHandle, 0x25, 0);
        STDENC_WriteReg8(ThisElem->DENCHandle, 0x26, 0);

        ThisElem->TTXLineInsertRegs[0] = 0;
        ThisElem->TTXLineInsertRegs[1] = 0;
        ThisElem->TTXLineInsertRegs[2] = 0;
        ThisElem->TTXLineInsertRegs[3] = 0;
        ThisElem->TTXLineInsertRegs[4] = 0;


        if (OddEvenFlag == STTTX_ODD_FIELD_FLAG)
        {
#if !defined(ST_5100) && !defined(ST_5105) && !defined (ST_5301) && !defined(ST_5188) && !defined(ST_5107)
            reg = 0x22;
            bitmask = 0x08; /*0x08;*/ /* We start in line 7 because some teletext
                                         decoders can not use line 6 */
#else
            reg = 0x24;
            bitmask = 0x02;           /* On 5100 vysnc polarity is inverted */
#endif
        }
        else
        {
#if !defined(ST_5100) && !defined(ST_5105) && !defined (ST_5301) && !defined(ST_5188) && !defined(ST_5107)
            reg = 0x24; /*0x24;*/
            bitmask = 0x02; /*0x01;*/
#else
            reg = 0x22;
            bitmask = 0x08;
#endif
        }

        i = 6;

        /* In the next loop we should check that the number of lines to send
           fits in one field ... At the moment we rely in good packets ...*/
        while (i < 24)
        {
            /* Insertion is actually now moved in TTXVBIEnable */
            ThisElem->TTXLineInsertRegs[reg - 0x22] |= bitmask;
            bitmask >>= 1;
            if  (bitmask == 0)
            {
                bitmask = 0x80;
                reg++;
            }
            i++;
        }
        STOS_InterruptUnlock();
#endif

#if !defined(ST_7020) && !defined(ST_5105)&& !defined(ST_5188) && !defined(ST_5107)
        /* enable "idle" interrupt */
        ThisElem->BaseAddress[STTTX_INT_ENABLE] |= STTTX_STATUS_COMPLETE;
#endif
        if (OddEvenFlag == STTTX_ODD_FIELD_FLAG)
            ThisElem->OddDataReady = TRUE;
        else
            ThisElem->EvenDataReady = TRUE;

        /* await DMA completion, 100ms max. Semaphore *should* never
         * time out, but we do (just in case, to stop driver blocking
         * totally).
         */

        TimeoutPoint = STOS_time_plus(STOS_time_now(), ClocksPerSecond /10);
        ret = STOS_SemaphoreWaitTimeOut(ThisElem->ISRComplete_p, &TimeoutPoint);
        /* XXX */
        if (ret != 0)
        {
            ret = ret;
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Timed out waiting for VBI completion"));
        }

#if defined(ST_7020)
        /* Just give 2 ms time for transfer to complete */
        TimeoutPoint = STOS_time_plus(STOS_time_now(), ClocksPerSecond / 400);
        ret = STOS_SemaphoreWaitTimeOut(ThisElem->ISRComplete_p, &TimeoutPoint);
        memset( (void*)(ThisElem->TTXAVMEM_Buffer_Address), 0x00, 48 *16);
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
        /* Just give 2 ms time for transfer to complete */
        TimeoutPoint = STOS_time_plus(STOS_time_now(), ClocksPerSecond /400);
        ret = STOS_SemaphoreWaitTimeOut(ThisElem->ISRComplete_p, &TimeoutPoint);
#endif


#if !defined(STTTX_USE_DENC)
        STVBI_Disable( ThisElem->VBIHandle );
#else
        if (OddEvenFlag == STTTX_ODD_FIELD_FLAG)
        {
            /* Only bits 0-3 have line values. */
            STDENC_MaskReg8(ThisElem->DENCHandle,  0x22, (U8)(~0x0f), 0);
            STDENC_WriteReg8(ThisElem->DENCHandle, 0x23, 0);
            STDENC_MaskReg8(ThisElem->DENCHandle,  0x24, (U8)(~0xfc), 0);			
        }
        else
        {
            STDENC_MaskReg8(ThisElem->DENCHandle,  0x24, (U8)(~0x03), 0);        
            STDENC_WriteReg8(ThisElem->DENCHandle, 0x25, 0);
            STDENC_WriteReg8(ThisElem->DENCHandle, 0x26, 0);
        }
#endif
    }
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    if (OddEvenFlag == STTTX_ODD_FIELD_FLAG)
    {
        error = STVMIX_DisableVBIViewPort(ThisElem->VBIVPDisplayOdd);
    }
    else
    {
        error = STVMIX_DisableVBIViewPort(ThisElem->VBIVPDisplayEven);
    }
    memory_deallocate(ThisElem->InitParams.DriverPartition, TtxBufferOrg_p);
#endif
    ALE_PRINT_DISABLE;
    ALE_Print(st,(st,"\nALE: VBI transfer complete."));
}


/****************************************************************************
Name         : HasFilterHit()

Description  : Applies the current filter to the selected page data.

Parameters   : STTTX_Request_t *Filter, pointer to active filter, and
               STTTX_Page_t *CurrPage pointing to current page data.
           U32 *MatchedMask pointer to a mask marking each line
                            that meets the criteria which gets set.

Return Value : TRUE  if filter has passed
               FALSE if filter has failed

See Also     : None
 ****************************************************************************/
static BOOL HasFilterHit( Handled_Request_t *QueueEnt,
                          STTTX_Page_t      *CurrPage,
                          U32               *MatchedMask)
{
    U8      *CurrLine;                              /* pointer to current line */
    U32     FieldLines;                             /* for shifting ValidLines */
    U32     Magazine, PacketNo;
    U32     TTPageNo = -1, SubPageN = -1;
    U32     i, ii=0;
    U8      PcktAddr1, PcktAddr2;                   /* inverse hamming 8/4 */
    U8      PageNumb1, PageNumb2;                   /* inverse hamming 8/4 */
    U8      SubPCode1, SubPCode2;                   /* inverse hamming 8/4 */
    U8      SubPCode3, SubPCode4;                   /* inverse hamming 8/4 */
    STTTX_FilterType_t CriteriaMatched;             /* for keeping track of filter */
    STTTX_Request_t    Filter;

    Filter = QueueEnt->Request;

    if ( CurrPage->ValidLines == 0 )                /* no data for this page? */
        return FALSE;                               /* nothing more to do */
    else if (Filter.RequestType == STTTX_FILTER_ANY)
    {
        *MatchedMask = CurrPage->ValidLines;
        return TRUE;                                /* page has data  */
    }

    FieldLines = CurrPage->ValidLines;              /* temp. for shifting */

    *MatchedMask = 0;                               /* initially set all lines to not matching */

    /* Loop through the lines */
    for( i = 0; FieldLines != 0; i++ )              /* if valid field lines */
    {
        /* This line is as yet unmatched */
        CriteriaMatched = 0;

        if( ( FieldLines & VALID_LINES_BIT ) == 0 )  /* invalid line? */
        {
            FieldLines >>= 1;                        /* step to next line */
            continue;
        }

        CurrLine = CurrPage->Lines[ii];              /* abbreviate line */
        ii++;

        PcktAddr1 = InvHamming8_4Tab[               /* Inverse Ham Packet Addr */
                CurrLine[PACKET_ADDR_POS1] ];
        PcktAddr2 = InvHamming8_4Tab[
                CurrLine[PACKET_ADDR_POS2] ];

        if( ( PcktAddr1 | PcktAddr2 ) ==            /* more than 1 bit wrong? */
                             BAD_INV_HAMMING )
        {
            CurrPage->ValidLines &=
                    ~( VALID_LINES_BIT << i );      /* mark line as invalid */
            FieldLines >>= 1;                       /* step to next line */
            continue;                               /* skip this line */
        }

        /* Derive packet number */
        PacketNo = ( PcktAddr1 >> 3 ) |             /* range 0..31 (should = i) */
                     ( PcktAddr2 << 1 );

        /* Filter on PacketNo */
        if ( ( Filter.RequestType & STTTX_FILTER_PACKET_0 ) != 0 &&
             PacketNo == 0 )
        {
            CriteriaMatched |= STTTX_FILTER_PACKET_0;
        }

        if ( ( Filter.RequestType & STTTX_FILTER_PACKET_30 ) != 0 &&
             PacketNo == 30 )
        {
            CriteriaMatched |= STTTX_FILTER_PACKET_30;
        }

        /* Page/Sub-Page are only present for a Page Header, Line 0. */
        if( PacketNo == 0 )                     /* header line? */
        {

            /* Calc page number */
            PageNumb1 = InvHamming8_4Tab[       /* LO nybble */
                            CurrLine[PAGE_NO_UNIT_POS]];
            PageNumb2 = InvHamming8_4Tab[       /* HO nybble */
                            CurrLine[PAGE_NO_TENS_POS]];

            if( ( PageNumb1 | PageNumb2 )       /* more than one bit wrong? */
                    == BAD_INV_HAMMING )
            {
                    FieldLines >>= 1;                       /* step to next line */
                    continue;                               /* skip this line */
            }

            TTPageNo = PageNumb1 |
                           ( PageNumb2 << NYBBLE_SHIFT_CNT );


            if( ( Filter.RequestType &         /* filter on page? */
                  STTTX_FILTER_PAGE ) != 0 )
            {
                if( Filter.PageNumber !=           /* different page? */
                    TTPageNo )
                {
                    FieldLines >>= 1;                       /* step to next line */
                    continue;                               /* skip this line */
                }
                else
                {
                    /* Page match */
                    CriteriaMatched |= STTTX_FILTER_PAGE;
                }
            }


            /* page number matches, check for subpage filter if needed*/
            if( ( Filter.RequestType &
                  STTTX_FILTER_SUBPAGE ) != 0 )
            {
                SubPCode4 =                     /* HO nybble */
                    InvHamming8_4Tab[CurrLine[SUBPAGE_THOU_POS]];

                SubPCode3 =
                    InvHamming8_4Tab[CurrLine[SUBPAGE_HUND_POS]];

                SubPCode2 =
                    InvHamming8_4Tab[CurrLine[SUBPAGE_TENS_POS]];

                SubPCode1 =                     /* LO nybble */
                    InvHamming8_4Tab[CurrLine[SUBPAGE_UNIT_POS]];

                if( ( ( ( SubPCode1 |           /* more than one bit wrong? */
                          SubPCode2 ) |
                        SubPCode3 ) |
                      SubPCode4 ) == BAD_INV_HAMMING )
                {
                    FieldLines >>= 1;                       /* step to next line */
                    continue;                               /* skip this line */
                }

                SubPageN = ( SubPCode4 &
                             SUBCODE4_MASK ) <<
                           NYBBLE_SHIFT_CNT;
                SubPageN = ( SubPCode3 |
                             SubPageN ) <<
                           NYBBLE_SHIFT_CNT;
                SubPageN = ( ( SubPCode2 &
                               SUBCODE2_MASK ) |
                             SubPageN ) <<
                           NYBBLE_SHIFT_CNT;
                SubPageN |= SubPCode1;

                if( Filter.PageSubCode !=      /* different subpage? */
                    SubPageN )
                {
                    FieldLines >>= 1;                       /* step to next line */
                    continue;                               /* skip this line */
                }
                else
                {
                    /* Subpage match */
                    CriteriaMatched |= STTTX_FILTER_SUBPAGE;
                }
            }                                   /* end of subpage filter */
        }                                       /* end of header line */

        /* Magazine filter active? */
        if( ( Filter.RequestType & STTTX_FILTER_MAGAZINE ) != 0 )
        {
            Magazine = PcktAddr1 & MAGAZINE_MASK;   /* extract (range 0..7)  */

            if( Filter.                            /* magazine match? */
                MagazineNumber == Magazine )
            {
                CriteriaMatched |= STTTX_FILTER_MAGAZINE;
            }
        }

        FieldLines >>= 1;                           /* step to next line */

        /* If this line meets all criteria mark TRUE in the Matches Mask */
        if(CriteriaMatched == Filter.RequestType)
        {
             *MatchedMask |= (VALID_LINES_BIT << i);
        }
    }                                            /* end of lines loop */

    /* If a matching line was found then MatchedMasked won't be zero -
     * return true.
     */
    return (*MatchedMask == 0)?FALSE:TRUE;
}

/****************************************************************************
Name         : GetPage()

Description  : Called if the request is to filter for a whole page (ODD
               and/or EVEN fields).  Alternative to HasFilterHit().
               Requires the page number and magazine number to also be
               set.

Parameters   : Filter: The current request
               Even: True if packet is even parity (field)
               Offset: value of the next available line in users mem space
               CurrPage: the packet that needs processing

Return Value : Returns TRUE if the request is now met i.e whole page has
               been copied

See Also     : None
****************************************************************************/
BOOL GetPage(STTTX_Request_t   *Filter,
             BOOL              Even,
             BOOL              *HeaderFound,
             U32               *Offset,
             STTTX_Page_t      *CurrPage)
{
    U8      *CurrLine;                              /* pointer to current line */
    U32     FieldLines;                             /* for shifting ValidLines */
    U32     Magazine, PacketNo;
    U32     TTPageNo, SubPageN;
    U32     i, ii = 0;
    U8      PcktAddr1, PcktAddr2;                   /* inverse hamming 8/4 */
    U8      PageNumb1, PageNumb2;                   /* inverse hamming 8/4 */
    U8      SubPCode1, SubPCode2;                   /* inverse hamming 8/4 */
    U8      SubPCode3, SubPCode4;                   /* inverse hamming 8/4 */
    U8      ControlByte;                            /* inverse hamming 8/4 */
    BOOL    SerialMode;                             /* magazine transmission mode */

    if ( CurrPage->ValidLines == 0 )                /* no data for this page ? */
    {
        return FALSE;                               /* nothing more to do */
    }

    FieldLines = CurrPage->ValidLines;              /* temp. for shifting */

    for( i = 0; FieldLines != 0; i++ )              /* if valid field lines */
    {

        if( ( FieldLines & VALID_LINES_BIT ) == 0 ) /* invalid line? */
        {
            FieldLines >>= 1;                       /* step to next line */
            continue;
        }

        CurrLine = CurrPage->Lines[ii];              /* abbreviate line */
        ii++;

        PcktAddr1 = InvHamming8_4Tab[ CurrLine[PACKET_ADDR_POS1] ];
        PcktAddr2 = InvHamming8_4Tab[ CurrLine[PACKET_ADDR_POS2] ];

        if( ( PcktAddr1 | PcktAddr2 ) == BAD_INV_HAMMING )
        {
            CurrPage->ValidLines &=
                    ~( VALID_LINES_BIT << i );      /* mark line as invalid */
            FieldLines >>= 1;                       /* step to next line */
            /* ii-- ? */
            continue;                               /* skip this line */
        }

        /* Derive packet number */
        PacketNo = ( PcktAddr1 >> 3 ) | ( PcktAddr2 << 1 );

        /* Magazine check for normal packets - magazine num must match,
         * else ignored
         */
        Magazine = PcktAddr1 & MAGAZINE_MASK;
        if(PacketNo != 0 && Filter->MagazineNumber != Magazine )
        {
            FieldLines >>= 1;
            continue;
        }

        /* Page/Sub-Page are only present for a Page Header, Packet 0. */
        if( PacketNo == 0 )
        {
            if (*Offset > 0)   /* we are looking for the end of a page */
            {
                /* this header packet is the same mag number */
                /* Don't we get these in any packets, not just packet 0?
                 * might be worth searching for them at any point. -- PW
                 */
                if (Filter->MagazineNumber == Magazine)
                {
                    /* applies in serial and parallel mode */
                    /* end of page found */
                    *HeaderFound = FALSE;
                    return TRUE;
                }
                else
                {
                    /* header of diff mag number */

                    /* in serial or parallel transmission mode ? */
                    ControlByte = InvHamming8_4Tab[CurrLine[MAG_TRANS_MODE_POS]];
                    SerialMode = ((MAG_TRANS_MODE_BIT & ControlByte) == SERIAL_MODE_TRANS_BIT ) ? TRUE : FALSE;

                    if(SerialMode == TRUE)
                    {
                        /* end of page found */
                        return TRUE;
                    }
                    else
                    {
                        FieldLines >>= 1;                       /* step to next line */
                        continue;
                    }
                }
            }
            else if (Filter->MagazineNumber != Magazine)
            {
                /* looking for start of page, wrong mag number, so skip
                 * line.
                 */
                FieldLines >>= 1;                       /* step to next line */
                continue;
            }

            /* We are looking for the start of a page and have found the
             * correct magazine number now check for the correct page number
             */

            /* Calc page number */
            PageNumb1 = InvHamming8_4Tab[CurrLine[PAGE_NO_UNIT_POS]];
            PageNumb2 = InvHamming8_4Tab[CurrLine[PAGE_NO_TENS_POS]];

            if( ( PageNumb1 | PageNumb2 ) == BAD_INV_HAMMING )
            {
                FieldLines >>= 1;                       /* step to next line */
                continue;                               /* skip this line */
            }

            TTPageNo = PageNumb1 |( PageNumb2 << NYBBLE_SHIFT_CNT );

            /* is this the wrong page number */
            if(Filter->PageNumber != TTPageNo)
            {
                FieldLines >>= 1;                       /* step to next line */
                continue;                               /* skip this line */
            }

            /* Page number correct so calc subpage if needed */
            SubPageN = 0;
            SubPCode4 = InvHamming8_4Tab[CurrLine[SUBPAGE_THOU_POS]];
            SubPCode3 = InvHamming8_4Tab[CurrLine[SUBPAGE_HUND_POS]];
            SubPCode2 = InvHamming8_4Tab[CurrLine[SUBPAGE_TENS_POS]];
            SubPCode1 = InvHamming8_4Tab[CurrLine[SUBPAGE_UNIT_POS]];

            if((((SubPCode1 |  SubPCode2 ) | SubPCode3) | SubPCode4 )
               == BAD_INV_HAMMING )
            {
                FieldLines >>= 1;
                continue;
            }

            SubPageN = ( SubPCode4 & SUBCODE4_MASK ) << NYBBLE_SHIFT_CNT;
            SubPageN = ( SubPCode3 | SubPageN )      << NYBBLE_SHIFT_CNT;
            SubPageN = ( ( SubPCode2 & SUBCODE2_MASK ) | SubPageN ) << NYBBLE_SHIFT_CNT;
            SubPageN |= SubPCode1;

            /* Does the filter require a certain subpage number */
            if((Filter->RequestType & STTTX_FILTER_SUBPAGE) == STTTX_FILTER_SUBPAGE)
            {
                /* is this the wrong sub-page number */
                if(Filter->PageSubCode != SubPageN)
                {
                    FieldLines >>= 1;
                    continue;
                }
            }
            else
            {
                /* finds a subpage - now sets the search to find the matching
                   subpage with other parity */
                Filter->PageSubCode = SubPageN;
                Filter->RequestType |= STTTX_FILTER_SUBPAGE;
            }

            /* starting to record page */
            STTBX_Print(("Starting to record page num %d packet num %d sub page %d\n", TTPageNo, PacketNo, SubPageN));

            /* store the first line */
            *Offset += 1;
            *HeaderFound = TRUE;
            if(Even == TRUE)
                memcpy(&Filter->EvenPage->Lines[0], CurrLine, STTTX_LINESIZE);
            else
                memcpy(&Filter->OddPage->Lines[0], CurrLine, STTTX_LINESIZE);

            FieldLines >>= 1;
            continue;
        }
        else  /* packet num != 0 */
        {
            if(Filter->MagazineNumber != Magazine)
            {
                FieldLines >>= 1;                       /* step to next line */
                continue;
            }

            /* if already started storing page, and it is a normal
             * packet then store it
             */

            if( (*HeaderFound == TRUE ) && PacketNo < 29 )
            {
                /* generate valid lines mask & copy line */
                if(Even == TRUE)
                    memcpy(&Filter->EvenPage->Lines[*Offset],
                            CurrLine, STTTX_LINESIZE );
                else
                    memcpy( &Filter->OddPage->Lines[*Offset],
                            CurrLine, STTTX_LINESIZE );

                /* increment offset to keep track of where to store
                   the line in users mem space */
                *Offset += 1;
            }

            FieldLines >>= 1;
            continue;
        }
    }  /* end of lines loop */

    return FALSE;
}

#if defined(DVD_TRANSPORT_LINK)
/****************************************************************************
Name:           overflow_error
Description:    called by the link interrupt handler when an overflow occurs;
                just sets overflowbytes and overflowtime
****************************************************************************/
void overflow_error(slot_t received_slot)
{
    pti_get_written_size(received_slot, &OverflowBytes);
    OverflowTime = STOS_time_now();
}
#endif

/****************************************************************************
Name         : TTXBufferPoolingTask()

Description  : This task call the pooling function periodically to get PES

Parameters   : void *Data points to the stttx_context_t linked list
               element for the current PTI device instance (recast).

Notes        : We assume that Data is a valid pointer.

Return Value : void

See Also     : None
 ****************************************************************************/
void TTXBufferPoolingTask(void *Data)
{
    data_store_t    *CurrPes;                       /* pointer into Pes[i] */
    stttx_context_t *ThisElem;                      /* main data pointer */

    U32             BytesWritten = 0;
    BOOL            HaveData;
    BOOL            FailedPacket;
    U32             PoolingTaskDelayTime;
    STTTX_Handle_t  TTXHandle;

    PoolingTaskDelayTime = ClocksPerSecond / POOLING_TASK_DIVIDER;
    /* main data pointer */
    CurrPes = (data_store_t *)Data;
    ThisElem = (stttx_context_t *)CurrPes->Element;

    /* pass back base + index */
    TTXHandle = ThisElem->BaseHandle + CurrPes->Object;

#if defined(DVD_TRANSPORT_LINK)
    if (pti_register_interrupt_callback(pti_interrupt_dma_overflow,
                                        overflow_error))
    {
        STTBX_Print(("Error installing callback\n"));
    }
    else
    {
        if (pti_enable_interrupt_callback(pti_interrupt_dma_overflow))
        {
            STTBX_Print(("Error enabling callback\n"));
        }
    }
#endif

    STOS_TaskEnter (NULL);
    /* while not closed */
    while ((CurrPes->TasksActive) && (ThisElem->TasksActive))
    {
        STOS_SemaphoreWait(CurrPes->PoolingWakeUp_p);

        /* Have we been told to quit? */
        while ((CurrPes->Playing) &&
               (CurrPes->TasksActive) &&
               (ThisElem->TasksActive))
        {
            STOS_TaskDelay(PoolingTaskDelayTime);

            /* Call pooling function. Places data into
             * CurrPes->TmpLinearBuff.
             */
            WaitForData(ThisElem, &BytesWritten,
                        &FailedPacket, &HaveData, CurrPes);

            if (HaveData == FALSE)
            {
                STTBX_Print(("Didn't get any packets\n"));
            }
            else
            {
                if (FailedPacket == TRUE)
                {
                    STTBX_Print(("Got failed packet\n"));
                }
            }

            if (HaveData == TRUE)
            {
                ttx_Message_t *MsgBuf;

                MsgBuf = (ttx_Message_t *)STOS_MessageQueueClaimTimeout(
                                     CurrPes->PESQueue_p, TIMEOUT_IMMEDIATE);
                if (MsgBuf != NULL)
                {
                    MsgBuf->BytesWritten = BytesWritten;
                    MsgBuf->FailedPacket = FailedPacket;
                    /* WaitForData should ensure BytesWritten <
                     * MESSAGE_DATA_SIZE
                     */
                    memcpy(MsgBuf->Data, CurrPes->TmpLinearBuff, BytesWritten);
                    STOS_MessageQueueSend(CurrPes->PESQueue_p, MsgBuf);
#if defined(STTTX_DEBUG1)
                    STOS_InterruptLock();
                    StttxBufferSize++;
                    STOS_InterruptUnlock();
#endif
                }
                else
                {
                    PostEvent(ThisElem, CurrPes, TTXHandle,
                              STTTX_EVENT_PES_LOST, NULL);
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                                 "Failed to claim message (ie overflow)\n"));

                }
            }
        }

        /* Set this flag for the handler task to stop. Done this way
         * around to ensure no stale data is left behind; see comments
         * in TTXHandlerTask for more details. The handler task should
         * wake up every so often even without a packet (timeout).
         */
        CurrPes->HandlerRunning = FALSE;
        /* DDTS - GNBvd33506, handler task will be waiting to recieve msg */
        {
            ttx_Message_t *MsgBuf;

            MsgBuf = (ttx_Message_t *)STOS_MessageQueueClaimTimeout(
                                CurrPes->PESQueue_p, TIMEOUT_IMMEDIATE);
            if(MsgBuf != NULL)
                STOS_MessageQueueSend(CurrPes->PESQueue_p, MsgBuf);
        }
    }
    STOS_TaskExit (NULL);

    STTBX_Print(("PES 0x%08x\tPooling task ending\n", (U32)CurrPes));
    STTBX_Print(("Processed %i packets\n", numpackets));
}

/* End of stttxtsk.c */
