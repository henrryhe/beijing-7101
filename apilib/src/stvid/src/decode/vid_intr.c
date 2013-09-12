/*******************************************************************************

File name   : vid_intr.c

Description : Video decode interrupt source file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
18 Jul 2000        Created                                          HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */
/*#define TRACE_DECODE */
/* #define TRACE_DECODE_ADDITIONAL_IT */ /* DON'T FORGET TO UNCOMMENT the same #define in vid_dec.c IF YOU  */
                                        /* UNCOMMENT HERE (otherwise you receive ITs you don't manage).    */

#if defined(TRACE_DECODE) || defined(TRACE_DECODE_ADDITIONAL_IT)
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */
#endif /* TRACE_DECODE */


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif


#include "stddefs.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"
#endif /* def STVID_STVAPI_ARCHITECTURE */

#include "vid_mpeg.h"

#include "vid_intr.h"

#include "vid_dec.h"
#include "vid_bits.h"
#include "vid_head.h"

#include "stevt.h"

#ifdef STVID_MEASURES
#include "measures.h"
#endif /* STVID_MEASURES */


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */

#ifdef STVID_MEASURES
clock_t BeginInterrupt;
#endif /* STVID_MEASURES */


#ifdef TRACE_DECODE
#ifdef TRACE_INPUT
extern U32 ITs[200];
extern U8 ITsIndex;
#endif   /* TRACE_INPUT */
#endif   /* TRACE_DECODE */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : viddec_InterruptHandler
Description : Recept interrupt event
Parameters  : EVT standard prototype
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_InterruptHandler(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
        STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDDEC_Handle_t DecodeHandle   = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDDEC_Handle_t, SubscriberData_p);
    U32 InterruptStatus;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    /* Read the interrupt status register */
    InterruptStatus = HALDEC_GetInterruptStatus(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle);

    /* When start code hit interrupt occurs, it may have to be discarded if it is during another start code parsing */
    if ((InterruptStatus & HALDEC_START_CODE_HIT_MASK) != 0)
    {
        /* Discard start code interrupt if we know it is not supposed to arrive during start code parsing */
        if (((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOnGoing)
        {
            /* Discard start code interrupt if we know it was triggered manually by reading data (like in user data parsing) */
            InterruptStatus ^= HALDEC_START_CODE_HIT_MASK;
            if (!((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsGettingUserData)
            {
                /* Found undesired start code: while parsing normal start code data, a SCH interrupt occured. It must be an error in the stream. */
                ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOK = FALSE;
#ifdef TRACE_UART
                TraceBuffer(("-cancelSc"));
#endif /* TRACE_UART */
            }
        }
    }

    /* Post interrupt command only if there is one valid interrupt */
    if (InterruptStatus != 0)
    {
#ifdef TRACE_DECODE
#ifdef TRACE_INPUT
        if (ITs[ITsIndex - 1] == 0)
        {
            /* IT was not processed ! */
            ITs[ITsIndex] = 0; /* For debug breakpoint */
        }
        ITs[ITsIndex] = 0;
        if (ITsIndex < (sizeof(ITs) / sizeof(U32)) - 1)
        {
            ITsIndex ++;
        }
#endif /* TRACE_INPUT */
#endif /* TRACE_DECODE */

        PushInterruptCommand(DecodeHandle, InterruptStatus);

        /* Signal controller that a new command is given */
        semaphore_signal(((VIDDEC_Data_t *) DecodeHandle)->DecodeOrder_p);
    } /* InterruptStatus not 0 */
} /* End of viddec_InterruptHandler() function */


/*******************************************************************************
Name        : viddec_ProcessInterrupt
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_ProcessInterrupt(const VIDDEC_Handle_t DecodeHandle, const U32 InterruptStatus)
{
    U32 VideoInterruptStatus;

    /* Mask the IT's not used */
    VideoInterruptStatus = InterruptStatus & ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask;

    /* Process interrupts in logical order of occurence in case many occur at the
    same time (ex: 'beginning of decode' before 'end of decode'. Then process levels,
    then 'start code hit' (parsing). */
    while (VideoInterruptStatus != 0)
    {
        /* Stay inside interrupt routine until all the bits have been tested */

      /* Skip process of first interrupts if only the 2 'end of decode' interrupts are there ! */
      if ((VideoInterruptStatus & ~(HALDEC_PIPELINE_IDLE_MASK | HALDEC_DECODER_IDLE_MASK)) != 0)
      {

        /*----------------------------------------------------------------------
           Inconsistency error.
        ----------------------------------------------------------------------*/
/* Not used for the moment... */
#ifdef TRACE_DECODE_ADDITIONAL_IT
/*        if ((VideoInterruptStatus & HALDEC_INCONSISTENCY_ERROR_MASK) != 0)*/
/*        {*/
/*            VideoInterruptStatus ^= HALDEC_INCONSISTENCY_ERROR_MASK;*/
/*        }*/ /* end HALDEC_INCONSISTENCY_ERROR_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT */


        /*----------------------------------------------------------------------
           Pipeline Start Decode (DSYNC).
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_PIPELINE_START_DECODING_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_PIPELINE_START_DECODING_MASK;

            viddec_HWDecoderStartOfDecode(DecodeHandle);
#ifdef TRACE_UART
            /* TraceBuffer(("-PSD:%d\n",get_time_now()/1000)); */
#endif
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodeInterruptStartDecode ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

        } /* end HALDEC_PIPELINE_START_DECODING_MASK interrupt handling */

#ifdef ST_smoothbackward
        /*----------------------------------------------------------------------
           VLD READY.
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_VLD_READY_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_VLD_READY_MASK;

            ((VIDDEC_Data_t *) DecodeHandle)->NextAction.LaunchDecodeWhenPossible = TRUE;

#ifdef TRACE_UART
            TraceBuffer(("-VLD READY\a"));
#endif /* TRACE_UART */

        } /* end HALDEC_VLD_READY_MASK interrupt handling */
#endif /*ST_smoothbackward*/


        /*----------------------------------------------------------------------
           Overflow Error (Severe Error ?).
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_DECODING_OVERFLOW_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_DECODING_OVERFLOW_MASK;

            /* Error_Recovery_Case_n09 :  Wrong number of slices. */
            viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_OVERFLOW);

#ifdef TRACE_UART
            TraceBuffer(("-SER\a"));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptDecodeOverflowError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

        } /* end HALDEC_DECODING_OVERFLOW_MASK interrupt handling */

        /*----------------------------------------------------------------------
           Decoding syntax error, HW error concealement (not blocking)
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_DECODING_SYNTAX_ERROR_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_DECODING_SYNTAX_ERROR_MASK;

            /* Error_Recovery_Case_n08 : Corruption in macroblock. */
            viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_SYNTAX);

#ifdef ST_OSLINUX
#ifdef TRACE_UART
            TraceBuffer(("-DSE:%d\n",get_time_now()/1000));
#endif
#else
#ifdef TRACE_UART
            TraceBuffer(("-SYE\a"));
#endif /* TRACE_UART */
#endif
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptSyntaxError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end HALDEC_DECODING_SYNTAX_ERROR_MASK interrupt handling */


        /*----------------------------------------------------------------------
           Overtime error.
        ----------------------------------------------------------------------*/
#ifdef TRACE_DECODE_ADDITIONAL_IT
        if ((VideoInterruptStatus & HALDEC_OVERTIME_ERROR_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_OVERTIME_ERROR_MASK;
#ifdef TRACE_UART
            TraceBuffer(("-OVE\a"));
#endif /* TRACE_UART */
        } /* end HALDEC_OVERTIME_ERROR_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT */


        /*----------------------------------------------------------------------
           Underflow error (Picture Decoding Error ?).
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_DECODING_UNDERFLOW_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_DECODING_UNDERFLOW_MASK;

            /* Error_Recovery_Case_n09 :  Wrong number of slices. */
            /* Pipeline reset is necessary to recover from PDE error */
            viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_UNDERFLOW);

#ifdef TRACE_UART
            TraceBuffer(("-PDE\a"));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptDecodeUnderflowError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end HALDEC_DECODING_UNDERFLOW_MASK interrupt handling */

        /*----------------------------------------------------------------------
           HW MISALIGNMENT DETECTED BY SW.
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK;

            viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_MISALIGNMENT);
#ifdef TRACE_UART
            TraceBuffer(("-MIS1\a"));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptMisalignmentError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end HALDEC_MISALIGNMENT_PIPELINE_AHEAD_MASK interrupt handling */

        if ((VideoInterruptStatus & HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK;

            viddec_DecodeHasErrors(DecodeHandle, VIDDEC_DECODE_ERROR_TYPE_MISALIGNMENT);
#ifdef TRACE_UART
            TraceBuffer(("-MIS2\a"));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodePbInterruptMisalignmentError ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end HALDEC_MISALIGNMENT_START_CODE_DETECTOR_AHEAD_MASK interrupt handling */

      } /* end (VideoInterruptStatus & ~(HALDEC_PIPELINE_IDLE_MASK | HALDEC_DECODER_IDLE_MASK)) */

        /*----------------------------------------------------------------------
           Pipeline idle.
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_PIPELINE_IDLE_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_PIPELINE_IDLE_MASK;

            viddec_HWDecoderEndOfDecode(DecodeHandle);

#ifdef TRACE_UART
            /*TraceBuffer(("-PII"));*/
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodeInterruptPipelineIdle ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end HALDEC_PIPELINE_IDLE_MASK interrupt handling */


        /*----------------------------------------------------------------------
           Decoder idle.
           The hardware decoder is blocked on a picture ready to decode it.
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_DECODER_IDLE_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_DECODER_IDLE_MASK;

            viddec_HWDecoderFoundNextPicture(DecodeHandle);

#ifdef TRACE_UART
            /*TraceBuffer(("-DEI\n"));*/
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodeInterruptDecoderIdle ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
        } /* end HALDEC_DECODER_IDLE_MASK interrupt handling */

        /* Exit interrupt handler now if no more flags in status ! */
        if (VideoInterruptStatus == 0)
        {
            /* exit while */
            break;
        }

        /*----------------------------------------------------------------------
           SCD Bit Buffer Empty.
        ----------------------------------------------------------------------*/
/* Not used for the moment... */
#ifdef TRACE_DECODE_ADDITIONAL_IT
/*        if ((VideoInterruptStatus & HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK) != 0)*/
/*        {*/
/*            VideoInterruptStatus ^= HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK;*/
/*        }*/ /* end HALDEC_START_CODE_DETECTOR_BIT_BUFFER_EMPTY_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT */


        /*----------------------------------------------------------------------
           Bitstream (CD) FIFO Full.
        ----------------------------------------------------------------------*/
/* Not used for the moment... */
#ifdef TRACE_DECODE_ADDITIONAL_IT
/*        if ((VideoInterruptStatus & HALDEC_BITSTREAM_FIFO_FULL_MASK) != 0)*/
/*        {*/
/*            VideoInterruptStatus ^= HALDEC_BITSTREAM_FIFO_FULL_MASK;*/
/*        }*/ /* end HALDEC_BITSTREAM_FIFO_FULL_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT */


        /*----------------------------------------------------------------------
           Pipeline Bit Buffer Empty.
        ----------------------------------------------------------------------*/
#if defined TRACE_DECODE_ADDITIONAL_IT || defined STVID_DEBUG_GET_STATISTICS
        if ((VideoInterruptStatus & HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK;

            /* Don't notify UNDERFLOW_EVT, MonitorBitBufferLevel() has already done it ! */
#ifdef TRACE_UART
            TraceBuffer(("-BBE\a"));
#endif /* TRACE_UART */
#ifdef STVID_DEBUG_GET_STATISTICS
            ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodeInterruptBitBufferEmpty ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

            /* When BBE occurs, it may happen many many times. So disable it until bit buffer level goes up. */
            if ((((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask & HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK) != 0)
            {
                /* If flag was there, disable it. Otherwise no need to clear it, the less we read/write this variable, the best it is... */
                ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask &= ~HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK;
                HALDEC_SetInterruptMask(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask);
            }
        } /* end HALDEC_PIPELINE_BIT_BUFFER_EMPTY_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT || STVID_DEBUG_GET_STATISTICS */


        /*----------------------------------------------------------------------
           Bit Buffer Full.
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;

            /* Don't notify OVERFLOW_EVT, MonitorBitBufferLevel() has already done it ! */
#ifdef TRACE_UART
            TraceBuffer(("-BBF\a"));
#endif /* TRACE_UART */
            if (!(((VIDDEC_Data_t *) DecodeHandle)->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT))
            {
                /* VbvSizeReachedAccordingToInterruptBBT is FALSE only once after a SoftReset(). */
                /* Inform task that vbv_size is reached */
                ((VIDDEC_Data_t *) DecodeHandle)->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT = TRUE;
                /* Reset threshold to init level */
                viddec_SetBitBufferThresholdToIdleValue(DecodeHandle);
                /* Now don't need any more the BBF interrupt: disable it */
                viddec_UnMaskBitBufferFullInterrupt(DecodeHandle);
            }
#if defined TRACE_DECODE_ADDITIONAL_IT || defined STVID_DEBUG_GET_STATISTICS
            else
            {
#ifdef STVID_DEBUG_GET_STATISTICS
                ((VIDDEC_Data_t *) DecodeHandle)->Statistics.DecodeInterruptBitBufferFull ++;
#endif /* STVID_DEBUG_GET_STATISTICS */

                /* When BBF occurs, it may happen many many times. So disable it until bit buffer level goes down. */
                if ((((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask & HALDEC_BIT_BUFFER_NEARLY_FULL_MASK) != 0)
                {
                    /* If flag was there, disable it. Otherwise no need to clear it, the less we read/write this variable, the best it is... */
                    ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask &= ~HALDEC_BIT_BUFFER_NEARLY_FULL_MASK;
                    HALDEC_SetInterruptMask(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, ((VIDDEC_Data_t *) DecodeHandle)->VideoInterrupt.Mask);
                }
            }
#endif /* TRACE_DECODE_ADDITIONAL_IT || STVID_DEBUG_GET_STATISTICS */

        } /* end HALDEC_BIT_BUFFER_NEARLY_FULL_MASK interrupt handling */


        /*----------------------------------------------------------------------
           Header Fifo Empty.
        ----------------------------------------------------------------------*/
#ifdef TRACE_DECODE_ADDITIONAL_IT
        if ((VideoInterruptStatus & HALDEC_HEADER_FIFO_EMPTY_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_HEADER_FIFO_EMPTY_MASK;
#ifdef ST_OSLINUX
#ifdef TRACE_UART
            TraceBuffer(("-HFE"));
#endif
#endif

        } /* end HALDEC_HEADER_FIFO_EMPTY_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT */


        /*----------------------------------------------------------------------
           Header Fifo Full.
        ----------------------------------------------------------------------*/
#ifdef TRACE_DECODE_ADDITIONAL_IT
        if ((VideoInterruptStatus & HALDEC_HEADER_FIFO_NEARLY_FULL_MASK) != 0)
        {
            VideoInterruptStatus ^= HALDEC_HEADER_FIFO_NEARLY_FULL_MASK;
#ifdef ST_OSLINUX
#ifdef TRACE_UART
            TraceBuffer(("-HFF"));
#endif
#endif

        } /* end HALDEC_HEADER_FIFO_NEARLY_FULL_MASK interrupt handling */
#endif /* TRACE_DECODE_ADDITIONAL_IT */

        /*----------------------------------------------------------------------
           Start Code Hit.
           * found a start code, it is in header FIFO data
        ----------------------------------------------------------------------*/
        if ((VideoInterruptStatus & HALDEC_START_CODE_HIT_MASK) != 0)
        {

#ifdef TRACE_UART
            /* TraceBuffer(("- Sc:%d\n",get_time_now()/1000)); */
#endif /* TRACE_UART */
            VideoInterruptStatus ^= HALDEC_START_CODE_HIT_MASK;

            /* Inform that SC search is finished */
            ((VIDDEC_Data_t *) DecodeHandle)->HWStartCodeDetector.IsIdle = TRUE;

            /* Normal case: read start code from HW */
#ifdef STVID_MEASURES
/*            BeginInterrupt = time_now();*/
            MeasuresDec[MEASURE_BEGIN][MeasureIndexDec] = time_now();
#endif /* STVID_MEASURES */

            ((VIDDEC_Data_t *) DecodeHandle)->LastStartCode = viddec_HeaderDataGetStartCode(&(((VIDDEC_Data_t *) DecodeHandle)->HeaderData));

#ifdef ST_OSLINUX
#if 0
			printk("[%02x]",((VIDDEC_Data_t *) DecodeHandle)->LastStartCode);
#endif
#endif

#ifdef STVID_MEASURES
            MeasuresDec[MEASURE_END][MeasureIndexDec] = time_now();
            MeasuresDec[MEASURE_ID][MeasureIndexDec]  = MEASURE_ID_GET_START_CODE;
            if (MeasureIndexDec < NB_OF_MEASURES - 1)
            {
                MeasureIndexDec++;
            }
#endif /* STVID_MEASURES */
            viddec_FoundStartCode(DecodeHandle, ((VIDDEC_Data_t *) DecodeHandle)->LastStartCode);

        } /* end VID_ITX_SCH interrupt handling */

    } /* end of while(VideoInterruptStatus != 0) */

} /* End of viddec_ProcessInterrupt() function */




/* End of vid_intr.c */
