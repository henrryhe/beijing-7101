/*********************************************************************************
File Name   : cec.c

Description : VOUT driver module. Consumer Electronic Control


COPYRIGHT (C) STMicroelectronics 2006.

Date                   Modification                                     Name
----                   ------------                                     ----
04 Dec 2006            Creation                                         WF
*********************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <string.h>
#include <stdio.h>
#endif /* ST_OSLINUX */

#include "stsys.h"

#include "cec.h"
#include "hdmi_api.h"
#include "stpwm.h"

#if defined (STVOUT_TRACE)
#include "..\..\..\dvdgr-prj-stapigat\src\trace.h"
#endif

#ifdef STVOUT_CEC_CAPTURE0
    #define STPWM_CompareCEC    STPWM_Compare1
    #define STPWM_CHANNEL_CECTX STPWM_CHANNEL_1
    #define STPWM_CaptureCEC    STPWM_Capture0
    #define STPWM_CHANNEL_CECRX STPWM_CHANNEL_0
#else
    #define STPWM_CompareCEC    STPWM_Compare0
    #define STPWM_CHANNEL_CECTX STPWM_CHANNEL_0
    #define STPWM_CaptureCEC    STPWM_Capture1
    #define STPWM_CHANNEL_CECRX STPWM_CHANNEL_1
#endif
/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */
#ifdef CEC_DEBUG_PIO_RX_RECEPTION
    #define CEC_PIO_DEBUG_BIT0       PIO_BIT_6
#endif
#ifdef CEC_DEBUG_PIO_TX_INT
    #define CEC_PIO_DEBUG_BIT1       PIO_BIT_5
#endif
#ifdef CEC_DEBUG_RX_ACK_SEND
    #define CEC_PIO_DEBUG_BIT2       PIO_BIT_3
#endif
#if defined(CEC_DEBUG_TX_ACK_SAMPLE) || defined(CEC_DEBUG_PIO_ERROR)
    #define CEC_PIO_DEBUG_BIT3       PIO_BIT_6
#endif

/*#define CEC_DEBUG_TASK*/

#ifdef CEC_DEBUG_TASK
#define STTBX_Task_Debug(X) STTBX_Print X
#else
#define STTBX_Task_Debug(X)
#endif

#ifdef CEC_DEBUG_API
#define STTBX_CEC_API_Debug(X) STTBX_Print X
#else
#define STTBX_CEC_API_Debug(X)
#endif

#ifdef CEC_DEBUG_PARAMS
#define STTBX_CEC_Params_Debug(X) STTBX_Print X
#else
#define STTBX_CEC_Params_Debug(X)
#endif


/* Env 1 is STVOUT */
/* Env 2 is STHDMI */
/* Env 3 is STFAE */
/*#define STVOUT_CEC_ENV3*/

#ifdef STVOUT_CEC_ENV3
    #ifndef STVOUT_CEC_ONE_SECOND
    #define STVOUT_CEC_ONE_SECOND 3125000
    #endif
    #ifndef STVOUT_CEC_LOW_OFFSET
    #define STVOUT_CEC_LOW_OFFSET 15
    #endif
    #ifndef STVOUT_CEC_HIGH_OFFSET
    #define STVOUT_CEC_HIGH_OFFSET 15
    #endif
#elif defined(STVOUT_CEC_ENV2)
    #ifndef STVOUT_CEC_ONE_SECOND
    #define STVOUT_CEC_ONE_SECOND 3125000
    #endif
    #ifndef STVOUT_CEC_LOW_OFFSET
    #define STVOUT_CEC_LOW_OFFSET 15
    #endif
    #ifndef STVOUT_CEC_HIGH_OFFSET
    #define STVOUT_CEC_HIGH_OFFSET 15
    #endif
#else /*defined(STVOUT_CEC_ENV1)*/
    #ifndef STVOUT_CEC_ONE_SECOND
    #define STVOUT_CEC_ONE_SECOND 2777778
    #endif
    #ifndef STVOUT_CEC_LOW_OFFSET
    #define STVOUT_CEC_LOW_OFFSET 15
    #endif
    #ifndef STVOUT_CEC_HIGH_OFFSET
    #define STVOUT_CEC_HIGH_OFFSET 15
    #endif
#endif


#ifndef ST_OSLINUX

#define CEC_OneSecond()         STVOUT_CEC_ONE_SECOND
#define CEC_OneMiliSecond()     CEC_OneSecond()/1000
#define CEC_OneMicroSecond()    CEC_OneMiliSecond()/1000
#define CEC_NominalBitPeriod()  2.4*CEC_OneMiliSecond()

#else

#ifdef STVOUT_CEC_ENV1
#define CEC_OneSecond()         STVOUT_CEC_ONE_SECOND
#define CEC_OneMiliSecond()     3125
#define CEC_OneMicroSecond()    3.125
#define CEC_NominalBitPeriod()  7500
#endif

#ifdef STVOUT_CEC_ENV2
#define CEC_OneSecond()         STVOUT_CEC_ONE_SECOND
#define CEC_OneMiliSecond()     3472
#define CEC_OneMicroSecond()    3.472
#define CEC_NominalBitPeriod()  8333
#endif

#ifdef STVOUT_CEC_ENV3
#define CEC_OneSecond()         STVOUT_CEC_ONE_SECOND
#define CEC_OneMiliSecond()     3472
#define CEC_OneMicroSecond()    3.472
#define CEC_NominalBitPeriod()  8333
#endif



#endif /* !ST_OSLINUX */

#ifndef ST_OSLINUX
#define LOW_OFFSET          STVOUT_CEC_LOW_OFFSET  * CEC_OneMicroSecond()
#define HIGH_OFFSET         STVOUT_CEC_HIGH_OFFSET * CEC_OneMicroSecond()
#else
#ifdef STVOUT_CEC_ENV1
#define LOW_OFFSET          /*641*/  47
#define HIGH_OFFSET         /*1031*/  47
#endif

#ifdef STVOUT_CEC_ENV2
#define LOW_OFFSET          712
#define HIGH_OFFSET         1146
#endif

#ifdef STVOUT_CEC_ENV3
#define LOW_OFFSET          712
#define HIGH_OFFSET         1146
#endif



#endif /*ST_OSLINUX*/


/* Private Macros ---------------------------------------------------------- */
#define GetPos(BitPos)  ((BitPos)==PIO_BIT_0 ? 0 :    \
                         (BitPos)==PIO_BIT_1 ? 1 :    \
                         (BitPos)==PIO_BIT_2 ? 2 :    \
                         (BitPos)==PIO_BIT_3 ? 3 :    \
                         (BitPos)==PIO_BIT_4 ? 4 :    \
                         (BitPos)==PIO_BIT_5 ? 5 :    \
                         (BitPos)==PIO_BIT_6 ? 6 : 7 )


/* CEC_Minimal_Timing */


#ifndef ST_OSLINUX

#ifndef CEC_Time_Start_bit_Low
#define CEC_Time_Start_bit_Low()        3.6 * CEC_OneMiliSecond() - LOW_OFFSET
#endif
#ifndef CEC_Time_Start_bit_High
#define CEC_Time_Start_bit_High()       4.4 *  CEC_OneMiliSecond() - HIGH_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_Low
#define CEC_Time_Zero_bit_Low()         (1.4+0.15) * CEC_OneMiliSecond() - LOW_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_High
#define CEC_Time_Zero_bit_High()        2.2 *  CEC_OneMiliSecond() - HIGH_OFFSET
#endif
#ifndef CEC_Time_One_bit_Low
#define CEC_Time_One_bit_Low()          0.6 * CEC_OneMiliSecond()  - LOW_OFFSET
#endif
#ifndef CEC_Time_One_bit_High
#define CEC_Time_One_bit_High()         2.2 *  CEC_OneMiliSecond() - HIGH_OFFSET
#endif

#else


#ifdef STVOUT_CEC_ENV1
#ifndef CEC_Time_Start_bit_Low
#define CEC_Time_Start_bit_Low()        11250 - LOW_OFFSET
#endif
#ifndef CEC_Time_Start_bit_High
#define CEC_Time_Start_bit_High()       13750 - HIGH_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_Low
#define CEC_Time_Zero_bit_Low()         4844 - LOW_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_High
#define CEC_Time_Zero_bit_High()        6875 - HIGH_OFFSET
#endif
#ifndef CEC_Time_One_bit_Low
#define CEC_Time_One_bit_Low()          1875 - LOW_OFFSET
#endif
#ifndef CEC_Time_One_bit_High
#define CEC_Time_One_bit_High()         6875 - HIGH_OFFSET
#endif
#endif

#ifdef STVOUT_CEC_ENV2
#ifndef CEC_Time_Start_bit_Low
#define CEC_Time_Start_bit_Low()        12499 - LOW_OFFSET
#endif
#ifndef CEC_Time_Start_bit_High
#define CEC_Time_Start_bit_High()       15277 - HIGH_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_Low
#define CEC_Time_Zero_bit_Low()         4861 - LOW_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_High
#define CEC_Time_Zero_bit_High()        7638 - HIGH_OFFSET
#endif
#ifndef CEC_Time_One_bit_Low
#define CEC_Time_One_bit_Low()          2083 - LOW_OFFSET
#endif
#ifndef CEC_Time_One_bit_High
#define CEC_Time_One_bit_High()         7638 - HIGH_OFFSET
#endif
#endif

#ifdef STVOUT_CEC_ENV3
#ifndef CEC_Time_Start_bit_Low
#define CEC_Time_Start_bit_Low()        12499 - LOW_OFFSET
#endif
#ifndef CEC_Time_Start_bit_High
#define CEC_Time_Start_bit_High()       15277 - HIGH_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_Low
#define CEC_Time_Zero_bit_Low()         4861 - LOW_OFFSET
#endif
#ifndef CEC_Time_Zero_bit_High
#define CEC_Time_Zero_bit_High()        7638 - HIGH_OFFSET
#endif
#ifndef CEC_Time_One_bit_Low
#define CEC_Time_One_bit_Low()          2083 - LOW_OFFSET
#endif
#ifndef CEC_Time_One_bit_High
#define CEC_Time_One_bit_High()         7638 - HIGH_OFFSET
#endif
#endif



#endif /*ST_OSLINUX*/


#ifndef STVOUT_CEC_CAPTURE
#define TOLERANT_IDENTIFICATION
#endif
/*
 *  - Needed for PIO interrupt without Capture :
 * some interruptions delay are usually 50 us but
 * on 1% of times more than 250 us so the limit of tolerance
 * should be more than 200 us
 *  - Needed to interface not compliant Equipment:
 * */

#ifdef TOLERANT_IDENTIFICATION

#define CEC_Time_Start_bit_Low_min()        2.7  * CEC_OneMiliSecond()
#define CEC_Time_Start_bit_Low_MAX()        4.7  * CEC_OneMiliSecond()
#define CEC_Time_Start_bit_Finish_min()     3.5  * CEC_OneMiliSecond()
#define CEC_Time_Start_bit_Finish_MAX()     5.5  * CEC_OneMiliSecond()

#define CEC_Time_Zero_bit_Low_min()         1.1  * CEC_OneMiliSecond()
#define CEC_Time_ACK_bit_Low_min()          1.0* CEC_OneMiliSecond()
#define CEC_Time_ACK_bit_Low()              1.325* CEC_OneMiliSecond()
#define CEC_Time_ACK_bit_Low_MAX()          2.5* CEC_OneMiliSecond()
#define CEC_Time_Zero_bit_Low_MAX()         2.5  * CEC_OneMiliSecond()

#define CEC_Time_One_bit_Low_min()          0.2  * CEC_OneMiliSecond()
#define CEC_Time_One_bit_Low_MAX()          1.0  * CEC_OneMiliSecond()

#define CEC_Time_Data_bit_Finish_min()      2.0  * CEC_OneMiliSecond()
#define CEC_Time_Data_bit_Finish()          2.4  * CEC_OneMiliSecond()
#define CEC_Time_Data_bit_Finish_MAX()      3.0 * CEC_OneMiliSecond()

#else

#ifndef ST_OSLINUX
#define CEC_Time_Start_bit_Low_min()        3.5  * CEC_OneMiliSecond()
#define CEC_Time_Start_bit_Low_MAX()        (3.9+0.1)  * CEC_OneMiliSecond()
#define CEC_Time_Start_bit_Finish_min()     4.3  * CEC_OneMiliSecond()
#define CEC_Time_Start_bit_Finish_MAX()     (4.7+0.1)  * CEC_OneMiliSecond()
#define CEC_Time_Zero_bit_Low_min()         1.3  * CEC_OneMiliSecond()
/*ACK Bit is in middle between Best and Worst Cases 1.3 and 1.35*/
#define CEC_Time_ACK_bit_Low_min()          1.0* CEC_OneMiliSecond()
#define CEC_Time_ACK_bit_Low()              1.325* CEC_OneMiliSecond()
#define CEC_Time_ACK_bit_Low_MAX()          2.0* CEC_OneMiliSecond()
#define CEC_Time_Zero_bit_Low_MAX()         1.7  * CEC_OneMiliSecond()
#define CEC_Time_One_bit_Low_min()          0.4  * CEC_OneMiliSecond()
#define CEC_Time_One_bit_Low_MAX()          0.8  * CEC_OneMiliSecond()
#define CEC_Time_Data_bit_Finish_min()      2.05 * CEC_OneMiliSecond()
#define CEC_Time_Data_bit_Finish()          2.4 * CEC_OneMiliSecond()
#define CEC_Time_Data_bit_Finish_MAX()      (2.75+0.1) * CEC_OneMiliSecond()
#else



#ifdef STVOUT_CEC_ENV1
#define CEC_Time_Start_bit_Low_min()        10938
#define CEC_Time_Start_bit_Low_MAX()        12500
#define CEC_Time_Start_bit_Finish_min()     13438
#define CEC_Time_Start_bit_Finish_MAX()     15000
#define CEC_Time_Zero_bit_Low_min()         4063
/*ACK Bit is in middle between Best and Worst Cases 1.3 and 1.35*/
#define CEC_Time_ACK_bit_Low_min()          3125
#define CEC_Time_ACK_bit_Low()              4141
#define CEC_Time_ACK_bit_Low_MAX()          6250
#define CEC_Time_Zero_bit_Low_MAX()         5313
#define CEC_Time_One_bit_Low_min()          1250
#define CEC_Time_One_bit_Low_MAX()          2500
#define CEC_Time_Data_bit_Finish_min()      6406
#define CEC_Time_Data_bit_Finish()          7500
#define CEC_Time_Data_bit_Finish_MAX()      8906
#endif

#ifdef STVOUT_CEC_ENV2
#define CEC_Time_Start_bit_Low_min()        12152
#define CEC_Time_Start_bit_Low_MAX()        13541
#define CEC_Time_Start_bit_Finish_min()     14930
#define CEC_Time_Start_bit_Finish_MAX()     16318
#define CEC_Time_Zero_bit_Low_min()         4514
/*ACK Bit is in middle between Best and Worst Cases 1.3 and 1.35*/
#define CEC_Time_ACK_bit_Low_min()          3472
#define CEC_Time_ACK_bit_Low()              4600
#define CEC_Time_ACK_bit_Low_MAX()          6944
#define CEC_Time_Zero_bit_Low_MAX()         5902
#define CEC_Time_One_bit_Low_min()          1389
#define CEC_Time_One_bit_Low_MAX()          2778
#define CEC_Time_Data_bit_Finish_min()      7118
#define CEC_Time_Data_bit_Finish()          8333
#define CEC_Time_Data_bit_Finish_MAX()      9548
#endif

#ifdef STVOUT_CEC_ENV3
#define CEC_Time_Start_bit_Low_min()        12152
#define CEC_Time_Start_bit_Low_MAX()        13541
#define CEC_Time_Start_bit_Finish_min()     14930
#define CEC_Time_Start_bit_Finish_MAX()     16318
#define CEC_Time_Zero_bit_Low_min()         4514
/*ACK Bit is in middle between Best and Worst Cases 1.3 and 1.35*/
#define CEC_Time_ACK_bit_Low_min()          3472
#define CEC_Time_ACK_bit_Low()              4600
#define CEC_Time_ACK_bit_Low_MAX()          6944
#define CEC_Time_Zero_bit_Low_MAX()         5902
#define CEC_Time_One_bit_Low_min()          1389
#define CEC_Time_One_bit_Low_MAX()          2778
#define CEC_Time_Data_bit_Finish_min()      7118
#define CEC_Time_Data_bit_Finish()          8333
#define CEC_Time_Data_bit_Finish_MAX()      9548
#endif



#endif

#endif /* TOLERANT_IDENTIFICATION */


#define CEC_IdleBusDelay_SendAgain()        7 * CEC_NominalBitPeriod()
#define CEC_IdleBusDelay_NewSending()       5 * CEC_NominalBitPeriod()
#define CEC_IdleBusDelay_Retransmission()   5 * CEC_NominalBitPeriod()  /* 3 * CEC_NominalBitPeriod() */



#ifndef ST_OSLINUX
#define CEC_ErrorDelay()                    1.5 * CEC_Time_Data_bit_Finish()
#define MAX_CEC_PIO_Rise_Time()             5 * CEC_OneMicroSecond()
#else
#ifdef STVOUT_CEC_ENV1
#define CEC_ErrorDelay()                    11250
#define MAX_CEC_PIO_Rise_Time()             16
#endif
#ifdef STVOUT_CEC_ENV2
#define CEC_ErrorDelay()                    14322
#define MAX_CEC_PIO_Rise_Time()             17
#endif
#ifdef STVOUT_CEC_ENV3
#define CEC_ErrorDelay()                    14322
#define MAX_CEC_PIO_Rise_Time()             17
#endif


#endif /*ST_OSLINUX*/



#define Pulse_Start_Low(L)       (  ( L > CEC_Time_Start_bit_Low_min()  )  &&  ( L < CEC_Time_Start_bit_Low_MAX()  )  )
#define Pulse_Start_Finish(H)      ( ( H > CEC_Time_Start_bit_Finish_min() ) && ( H < CEC_Time_Start_bit_Finish_MAX() ) )

#define Pulse_Zero_Low(L)        (   ( L > CEC_Time_Zero_bit_Low_min()  )  &&  ( L < CEC_Time_Zero_bit_Low_MAX()  )  )
#define Pulse_Zero_Ack_Low(L)    (   ( L > CEC_Time_ACK_bit_Low_min()  )  &&  ( L < CEC_Time_ACK_bit_Low_MAX()  )  )
#define Pulse_Zero_Finish(H)     (  ( H > CEC_Time_Data_bit_Finish_min() ) && ( H < CEC_Time_Data_bit_Finish_MAX() ) )

#define Pulse_One_Low(L)         (  ( L > CEC_Time_One_bit_Low_min()   )  &&  ( L < CEC_Time_One_bit_Low_MAX()   )  )
#define Pulse_One_Finish(H)      ( ( H > CEC_Time_Data_bit_Finish_min() ) && ( H < CEC_Time_Data_bit_Finish_MAX() ) )

/* Global Variables -------------------------------------------------------- */
#ifdef CEC_DEBUG
        U32 DebugCount1=0;
        U32 ModuloCount1=0;
        U32 DebugTimes1[304];
        U32 DebugDiffs1[304];
        CEC_Bit_Type_t  Debug_Bit_Array[200];
        U32             Debug_Bit_Array_Counter=0;
#endif

extern HDMI_Data_t *HDMI_Driver_p;  /*as PIO Int Handler do not give param and
                                     * CEC timing should better not go through semaphores
                                     * as time is measured on execution not by hard*/

/* Functions (private)------------------------------------------------------ */

static void CEC_Tx_Int_Enable(HDMI_Data_t *HDMI_Data_p,BOOL Enable);
static void CEC_Rx_Int_Enable(HDMI_Data_t *HDMI_Data_p, BOOL Enable,BOOL StateCausingInterrupt);
static U32 Time_Minus(U32 Plus, U32 Minus);
static BOOL Time_First_Previous(U32 First,U32 Second);
static U32  CEC_CPTCMP_TimeNow(const HDMI_Handle_t Handle);
static void CEC_Set_PIO(STPIO_Handle_t Handle,BOOL State);
static void CEC_ErroNotification(BOOL PIOState);
static void CEC_Error(CEC_Error_t r_error);
static void CEC_Release_PIO(STPIO_Handle_t Handle);
static void CEC_AddBit(CEC_Bit_Type_t CEC_Bit);
static void CEC_Bit_Recognise(U32 Low_State_Width,U32 All_Bit_Width);
static void CEC_Send_Acknowledge(U32 PIO_Int_Time);
#ifdef STVOUT_CEC_CAPTURE
static void stvout_CEC_Capture_Interrupt(void * Param);
#else
static void stvout_CEC_PIO_Interrupt(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
#endif
static void stvout_CEC_CompareBitTx_Interrupt(void * Param);
static void CEC_StartMessageTransmission(const HDMI_Handle_t HdmiHandle);
static ST_ErrorCode_t CEC_SendLabeledMessage(  const HDMI_Handle_t HdmiHandle,
                                               const STVOUT_CECMessage_t * const Message_p,
                                               const CEC_Message_Request_t MessageLabel);
static void  CEC_TaskFunction (HDMI_Handle_t const Handle);



/* Exported Functions ------------------------------------------------------ */

/*******************************************************************************
Name        : CEC_Tx_Int_Enable
Description : CEC PIO interrupt event
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Tx_Int_Enable(HDMI_Data_t *HDMI_Data_p,BOOL Enable)
{
    ST_ErrorCode_t ErroCode;

    ErroCode = STPWM_InterruptControl(HDMI_Data_p->CEC_Params.PwmCompareHandle, STPWM_CompareCEC, Enable);

}/*end of CEC_Tx_Int_Enable() */

/*******************************************************************************
Name        : CEC_Rx_Int_Enable
Description : CEC PIO interrupt event
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Rx_Int_Enable(HDMI_Data_t *HDMI_Data_p, BOOL Enable,BOOL StateCausingInterrupt)
{
#ifdef STVOUT_CEC_CAPTURE
    UNUSED_PARAMETER(StateCausingInterrupt); /* For capture, it'll be updated just after interrupt by reading the PIO */
    STPWM_InterruptControl(HDMI_Data_p->CEC_Params.PwmCaptureHandle, STPWM_CaptureCEC, Enable);
#else
    HDMI_Data_p->CEC_Params.IsReception_Interrupt_State_High = StateCausingInterrupt;
    if(!StateCausingInterrupt)
    {
        HDMI_Data_p->CEC_Params.Compare.ComparePattern |= HDMI_Data_p->CECPIO_BitNumber; /*Interrupt if different than this*/
    }
    else
    {
        HDMI_Data_p->CEC_Params.Compare.ComparePattern &= ~HDMI_Data_p->CECPIO_BitNumber;/*Interrupt if different than this*/
    }
    if(Enable)
    {
        HDMI_Data_p->CEC_Params.Compare.CompareEnable  |= HDMI_Data_p->CECPIO_BitNumber;
    }
    else
    {
        HDMI_Data_p->CEC_Params.Compare.CompareEnable  &= ~HDMI_Data_p->CECPIO_BitNumber;
    }
    STPIO_SetCompare(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, &HDMI_Data_p->CEC_Params.Compare);
#endif /* STVOUT_CEC_CAPTURE */
}/*end of CEC_Rx_Int_Enable() */

/*******************************************************************************
Name        : Time_Minus
Description : Difference
 *              with respect of overflow
 *              with hypotheses, difference is less than half size max
Returns     : result = Plus - Minus
*******************************************************************************/
static U32 Time_Minus(U32 Plus, U32 Minus)
{
    if(Plus >= Minus)
    {
        return ( Plus - Minus );
    }
    else
    {
        return (  (0xFFFFFFFF - Minus)+1 + Plus  ); /* 0xFFFFFFFF = ((U32)(-1)) */
    }
}/* end of Time_Minus() */

/*******************************************************************************
Name        : Time_First_Previous
Description : Comparison
 *              with respect of overflow
 *              with hypotheses, difference is less than half size max
 * Function tested with (10,20)T1 (0x10,0x20)T1 (0x20,0x10)F1
 *                      (0xFFFFFFF0,0x20)T2 (0x20,0xFFFFFFF0)F2
 *
Returns     : result = First < Second
*******************************************************************************/
static BOOL Time_First_Previous(U32 First,U32 Second)
{
    BOOL FirstIsPrevious;
    if(Second > First)
    {
        if( (Second - First) < (((U32)(-1))/2) )
        {
            FirstIsPrevious = TRUE;
        }
        else
        {
            FirstIsPrevious = FALSE;
        }
    }
    else
    {
        if( (First - Second) > (((U32)(-1))/2) )
        {
            FirstIsPrevious = TRUE;
        }
        else
        {
            FirstIsPrevious = FALSE;
        }
    }
    return (FirstIsPrevious);
} /* end of Time_First_Previous() */

/*******************************************************************************
Name        : CEC_CPTCMP_TimeNow
Description : Idle period during which the bus cannot be used
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static U32 CEC_CPTCMP_TimeNow(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;
    U32 Time = 0;

    STPWM_GetCMPCPTCounterValue(HDMI_Data_p->CEC_Params.PwmCompareHandle,&Time);

    return(Time);
}/*end of CEC_CPTCMP_TimeNow() */

/*******************************************************************************
Name        : CEC_ErroNotification
Description : We're not correctly reading the current data,
 *              or a higher priority writer started
 *
 * Note : As long as you don't handle Write -> Read Transition,
 * do not hold the line low
 *
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_ErroNotification(BOOL PIOState)
{
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;

    /* HDMI_Data_p->CEC_Params.IsBusFree = FALSE; Already False for sure */
    HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
    STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, CEC_CPTCMP_TimeNow(HDMI_Data_p) + CEC_ErrorDelay());
    /* Clears The PIO for Error Notification */
    if (!PIOState)
    {
        CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,0);
    }
    CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/

}/*end of CEC_ErroNotification() */

/*******************************************************************************
Name        : CEC_IdleBusDelay
Description :
Parameters  :
Returns     : Nothing
*******************************************************************************/
static void CEC_IdleBusDelay(HDMI_Data_t *HDMI_Data_p, U32 Delay_Time)
{
    /* HDMI_Data_p->CEC_Params.IsBusFree = FALSE; Already False for sure */
    HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
    STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, CEC_CPTCMP_TimeNow(HDMI_Data_p) + Delay_Time);
    CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/

}/*end of CEC_IdleBusDelay() */

/*******************************************************************************
Name        : CEC_Error
Description : An error has been detected on interrupt level
 *              interrupt should be handled on task level
Parameters  : A Pulse, state and width
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Error(CEC_Error_t r_error)
{
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;
#ifdef CEC_DEBUG_PIO_ERROR
STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
#endif
    /*For now, general for all errors, and on both edges*/
    /*Reset General*/
    /*Must not block while sending ACK*/
        if(HDMI_Data_p->CEC_Params.IsRxSendingAckBit)
        {
            CEC_Release_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle);
            HDMI_Data_p->CEC_Params.IsRxSendingAckBit = FALSE;
        }
        if(HDMI_Data_p->CEC_Params.IsTransmitting)
        {
            CEC_Release_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle);

            HDMI_Data_p->CEC_Params.IsTxWaitingAckBit = FALSE;
            HDMI_Data_p->CEC_Params.IsTxAckReceived = FALSE;
            HDMI_Data_p->CEC_Params.IsTransmitting = FALSE; /* Bus will be Freed by the error notification */

            HDMI_Data_p->CEC_Params.IsPreviousUnsuccessful = TRUE;
        }

        /*Reset Receiver */
        HDMI_Data_p->CEC_Params.RxCurrent_Word_Number = 0;/*Why 0 ? 0 is start bit*/
        HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number = 1;
        HDMI_Data_p->CEC_Params.IsMsgToAck = FALSE ;/*By default no, unless needed address is found*/
        HDMI_Data_p->CEC_Params.IsReceiving = FALSE;/*We're not recieving, state is idle till a high to low edge is detected*/

    switch(r_error)
    {   /*CEC_ERROR_START_BIT_UNEXPECTED*/

        case CEC_ERROR_BADINIT_FIRSTEDGELOWTOHIGH :
            /*CEC_ErroNotification(1);Special 6 Nominal Bit*/
            HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
            STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, CEC_CPTCMP_TimeNow(HDMI_Data_p) + 6*CEC_NominalBitPeriod() );/*5 plus 1 current*/
            CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/
            break;

        case CEC_ERROR_LOW_PERIOD_UNDEFINED :
            /*CEC_ErroNotification(1); Special 6 Nominal Bit*/
            HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
            STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, CEC_CPTCMP_TimeNow(HDMI_Data_p) + 6*CEC_NominalBitPeriod() );/*5 plus 1 current*/
            CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/
            break;

        case CEC_ERROR_START_THEN_HIGH_PERIOD_ERROR :
        case CEC_ERROR_ZERO_THEN_HIGH_PERIOD_ERROR:
        case CEC_ERROR_ONE_THEN_HIGH_PERIOD_ERROR:
            CEC_ErroNotification(0);                /* Only here we notify a pulse error */
            break;

        case CEC_ERROR_START_BIT_MISSING :
            /*CEC_ErroNotification(1); Special 6 Nominal bit*/
            HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
            STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, CEC_CPTCMP_TimeNow(HDMI_Data_p) + 6*CEC_NominalBitPeriod() );/*5 plus 1 current*/
            CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/
            break;

        case  CEC_ERROR_CANNOT_TX_DURING_RX :
            CEC_ErroNotification(1);
            break;

        case CEC_ERROR_TX_INT_AND_NOT_ACK_READ :
            CEC_ErroNotification(1);
            break;

        case CEC_ERROR_USUAL_ARBITRATION_LOSS :
            /*Should disable Transmission, and try to paste the Rx to continue reception*/
            /*CEC_ErroNotification(1); Special 6 Nominal bit*/
            HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
            STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, CEC_CPTCMP_TimeNow(HDMI_Data_p) + 6*CEC_NominalBitPeriod() );/*5 plus 1 current*/
            CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/
            break;

        case CEC_ERROR_COLLISION_DETECTED_AFTER_INITIATOR :
            /*Should disable Transmission, and generate error*/
            CEC_ErroNotification(1);
            break;

        case CEC_ERROR_ACK_LOW_PERIOD_UNDEFINED :
            CEC_ErroNotification(1);
            break;

        case CEC_ERROR_ACK_WAS_NOT_RECEIVED :
            CEC_ErroNotification(1);
            /*----------------------------*/
            /*  Word Wasn't Acknowledged, Not yet implemented */
            /*----------------------------*/
            break;

        default:
            CEC_ErroNotification(1);
            break;
    }

    if (HDMI_Data_p->CEC_Params.State == CEC_RXERROR)
    {
        /*Handle RX Error - Sem Signal - */
    }
    else if (HDMI_Data_p->CEC_Params.State == CEC_TXERROR)
    {
        /*Handle TX Error - Sem Signal - */
    }
    else /*Do not care about the error*/
    {
    }


#ifdef CEC_DEBUG_PIO_ERROR
STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
#endif
}/*end of CEC_Error() */

/*******************************************************************************
Name        : CEC_AddBit
Description : Adds the recognised bit : Start bit, One bit, Zero bit
Parameters  : The CEC bit Type
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_AddBit(CEC_Bit_Type_t CEC_Bit)
{
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;

    #ifdef CEC_DEBUG_ADDBIT
    Debug_Bit_Array[Debug_Bit_Array_Counter++] = CEC_Bit;
    if(Debug_Bit_Array_Counter>180)
    {
        Debug_Bit_Array_Counter = 0;
    }
    #endif

    if ( HDMI_Data_p->CEC_Params.RxCurrent_Word_Number == 0)
    {
        if(CEC_Bit == CEC_BIT_START)
        {
            HDMI_Data_p->CEC_Params.RxCurrent_Word_Number++;
        }
        else
        {
            CEC_Error(CEC_ERROR_START_BIT_MISSING);
        }
    }
    else
    {
        if(CEC_Bit == CEC_BIT_ERROR)/*CEC_AddBit Should never be called with this bit*/
        {
            /*CEC_ERROR();*/
        }
        else if(CEC_Bit != CEC_BIT_START)
        {
            if(HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number<9)
            {
                if(CEC_Bit == CEC_BIT_ZERO)
                {
                    HDMI_Data_p->CEC_Params.RxMessageList[HDMI_Data_p->CEC_Params.RxCurrent_Word_Number - 1] &= ~( 1<<(8-HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number) );
                }
                else if(CEC_Bit == CEC_BIT_ONE)
                {
                    HDMI_Data_p->CEC_Params.RxMessageList[HDMI_Data_p->CEC_Params.RxCurrent_Word_Number - 1] |= ( 1<<(8-HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number) );
                }

                if(HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number == 8)/* End Of Destination Address on Header Block*/
                {
                    if(HDMI_Data_p->CEC_Params.RxCurrent_Word_Number == 1) /*Header Block*/
                    {
                        HDMI_Data_p->CEC_Params.RxCurrent_Destination = HDMI_Data_p->CEC_Params.RxMessageList[0] & 0x0F;
                        if(     (HDMI_Data_p->CEC_Params.RxCurrent_Destination == HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex])
                                &&
                                (HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex] != CEC_UNREGISTRED)
                                /* We do not Ack when we're unregistred(15), this message is not just for us but for Broadcast(15) */
                                /* We do not handle the broadcast <not ack> that is inversed and Low on Broadcast */
                          )
                        {
                            HDMI_Data_p->CEC_Params.IsMsgToAck = TRUE;
                        }
                        else /* Check if we are in the case where additional address is allocated */
                        if (    ( HDMI_Data_p->CEC_Params.FirstLogicAddress != -1)
                                &&
                                (HDMI_Data_p->CEC_Params.FirstLogicAddress != CEC_UNREGISTRED)
                           )
                        {   /* ACK if the destination is our first address */
                            if ( HDMI_Data_p->CEC_Params.RxCurrent_Destination == HDMI_Data_p->CEC_Params.FirstLogicAddress )
                            {
                               HDMI_Data_p->CEC_Params.IsMsgToAck = TRUE;
                            }

                        }
                        else
                        {   /*This is not of our concern, but we keep listening anyway
                               or it's a Broadcast That we're Acknowledging by this way !*/
                            /*Our policy is to keep that it's a broadcast information just on the Destination Address*/
                            HDMI_Data_p->CEC_Params.IsMsgToAck = FALSE;
                        }
                    }
                }

                HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number++;
            }
            else if(HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number == 9)/*EOM*/
            {
                if(CEC_Bit == CEC_BIT_ZERO)
                {
                    HDMI_Data_p->CEC_Params.IsEndOfMsg = FALSE;
                }
                else if(CEC_Bit == CEC_BIT_ONE)
                {
                    HDMI_Data_p->CEC_Params.IsEndOfMsg = TRUE;
                }
                HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number++;
            }
            else /*if(HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number == 10)  ACK*/
            {
                /*Acknowledge reception is managed in interrupt handler, if transmission
                I do not care if we're just listening, if someone else acknowledges or not !
                But I have to manage the word count increment and bit count restart*/
                HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number = 1;
                HDMI_Data_p->CEC_Params.RxCurrent_Word_Number++;
                if ( HDMI_Data_p->CEC_Params.IsEndOfMsg )
                {
                    HDMI_Data_p->CEC_Params.RxMessageLength = HDMI_Data_p->CEC_Params.RxCurrent_Word_Number-1; /*Can be placed Here, to test*/
                    if(HDMI_Data_p->CEC_Params.IsMsgToAck) /*then Notify this Message Maybe better test on address here? No.*/
                    {
                        /*I do not check if the bit is ACKnowledge, because I must have put it Low myself by CEC Send Acknowledge!!!!*/
                        /* Notify a Message was Received */
                        HDMI_Data_p->CEC_Params.Notify |= CEC_MESSAGE_RECEIVED;
                        STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);
                    }
                    else /*if(Not Concerned by this message) then Notify That This is the time when the line Got Free !!!!!*/
                    {
                        if(HDMI_Data_p->CEC_Params.RxCurrent_Destination == 0xF) /*It's a broadcast Message, so Notify of it too*/
                        {
                            /* Notify a Message was Received */
                            HDMI_Data_p->CEC_Params.Notify |= CEC_MESSAGE_RECEIVED;
                            STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);
                        }
                    }

                    /* Very important Note !!!! Everything do not End here, but on the End of the ACK that is managed
                     * on the Tx interrupt, there, is managed the idle delay then the bus Free
                     * Cannot place delay here, because it would be overrided by the StateDuration of Tx Int,
                     * in fact, here we're called from the Tx interrupt to reach this point,
                     * why ? cause our strategy is not to have Rx interruptions generated after Tx ones */

                    /*Init Reception ---------------------------------------------------------*/
                    HDMI_Data_p->CEC_Params.RxCurrent_Word_Number = 0;/*Why 0 ? 0 is start bit*/
                    HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number = 1;
                    HDMI_Data_p->CEC_Params.IsReceiving = FALSE;/*We're not recieving, state is idle till a high to low edge is detected*/
                    HDMI_Data_p->CEC_Params.IsMsgToAck = FALSE ;/*By default no, unless needed address is found*/
                    CEC_IdleBusDelay(HDMI_Data_p, CEC_IdleBusDelay_NewSending() );  /* As soon as IsReceiving  becomes False, IdleBus Delay must get true */
                }
            }
        }
        else
        {
            CEC_Error(CEC_ERROR_START_BIT_UNEXPECTED);
        }
    }
}/*end of CEC_AddBit() */


/*******************************************************************************
Name        : CEC_Bit_Recognise
Description : Recognises what kind of Start bit, One bit, Zero bit was
 *              recieved or an error
Parameters  : Low state and high state width
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Bit_Recognise(U32 Low_State_Width,U32 All_Bit_Width)
{
    if( Pulse_Start_Low(Low_State_Width) )
    {
        if( Pulse_Start_Finish(All_Bit_Width) )
        {
            CEC_AddBit(CEC_BIT_START);
        }
        else
        {
            CEC_Error(CEC_ERROR_START_THEN_HIGH_PERIOD_ERROR);
        }
    }
    else if ( Pulse_Zero_Low(Low_State_Width) )
    {
        if( Pulse_Zero_Finish(All_Bit_Width) )
        {
            CEC_AddBit(CEC_BIT_ZERO);
        }
        else
        {
            CEC_Error(CEC_ERROR_ZERO_THEN_HIGH_PERIOD_ERROR);
        }
    }
    else if ( Pulse_One_Low(Low_State_Width) )
    {
        if( Pulse_One_Finish(All_Bit_Width) )
        {
            CEC_AddBit(CEC_BIT_ONE);
        }
        else
        {
            CEC_Error(CEC_ERROR_ONE_THEN_HIGH_PERIOD_ERROR);
        }
    }
    else
   {
        CEC_Error(CEC_ERROR_LOW_PERIOD_UNDEFINED);
   }

}/*end of CEC_Bit_Recognise() */


/*******************************************************************************
Name        : CEC_Send_Acknowledge
Description : Keep low for acknowledging
Parameters  : Starting time of the low level where the ack should occur
Assumptions : This Function'd better be called from interrupt
 *       if to avoid simultaneous Transmission Reception interruption conflict
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Send_Acknowledge(U32 PIO_Int_Time)/*Acknowledge low level since new Fall Time*/
{
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;

    #ifdef CEC_DEBUG_RX_ACK_SEND
    STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT2);
    #endif

    /* Broadcast negative acknowledgement not supported */

    CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,0); /*Confirm the Low State*/
    STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, PIO_Int_Time + CEC_Time_ACK_bit_Low());
    HDMI_Data_p->CEC_Params.IsRxSendingAckBit = TRUE;

    CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/

}/*end of CEC_Send_Acknowledge() */

/*******************************************************************************
Name        : stvout_CEC_ReceptionInterrupt   :           Reception Interruppt
Description : Recept interrupt event
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void stvout_CEC_ReceptionInterrupt(HDMI_Data_t *HDMI_Data_p,BOOL StateHigh, U32 InterruptTime)
{
    if(HDMI_Data_p->CEC_Params.IsTransmitting)
    {/*This must be whether Ack detection or a collision detected*/
        if((HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number < 5) && (HDMI_Data_p->CEC_Params.TxCurrent_Word_Number < 2))/*This is arbitration detection phase*/
        {
            /*must be StateHigh */
            CEC_Error(CEC_ERROR_USUAL_ARBITRATION_LOSS);
        }
        else if((HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 1)&&(HDMI_Data_p->CEC_Params.IsTxWaitingAckBit))/*1 as 10 incremented becomes 1*/
        {
            #ifdef CEC_DEBUG_TX_ACK_SAMPLE
            STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
            #endif
            if(StateHigh)/*Rising Edge*/
            {
                if( Pulse_Zero_Ack_Low(Time_Minus(InterruptTime, HDMI_Data_p->CEC_Params.TxStart_Of_ACK_Bit)) ) /* changed ACK + Tie minus */
                {
                    HDMI_Data_p->CEC_Params.IsTxAckReceived = TRUE;
                    #ifdef CEC_DEBUG_TX_ACK_SAMPLE
                    STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
                    #endif
                }
                else
                {
                    /*CEC_Error(CEC_ERROR_ACK_LOW_PERIOD_UNDEFINED);*/
                    /* No error, it just wasn't acknowledged */
                }
            }
            else
            {/*Falling Edge*/
                /* This happens at once after releasing PIO during the ACK Bit
                    * PIO_RELEASE_DELAY it's not an error
                    * Should be avoided by the PIO release polling  */
            }
        }
        else
        {
            CEC_Error(CEC_ERROR_COLLISION_DETECTED_AFTER_INITIATOR);
        }
    }
    else if(!HDMI_Data_p->CEC_Params.IsReceiving)/*Not Tranmitting, Not receiving, State is idle, let's receive*/
    {/*We were Idle, Then this must be the falling edge of start bit*/

        HDMI_Data_p->CEC_Params.IsBusFree = FALSE; /* Bus is no more Free on Start of reception */

        if(!StateHigh)/*Falling edge*/
        {
            /* We want to start reception, so we should stop the Idle delaying mechanism */
            CEC_Tx_Int_Enable(HDMI_Data_p,0);/*Disabled*/
            HDMI_Data_p->CEC_Params.IsBusIdleDelaying = FALSE;

            HDMI_Data_p->CEC_Params.IsReceiving = TRUE;
            HDMI_Data_p->CEC_Params.Last_Bit_Fall_Time = InterruptTime;
            #ifdef CEC_DEBUG_PIO_RX_RECEPTION
            STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT0);
            #endif
        }
        else
        {
            CEC_Error(CEC_ERROR_BADINIT_FIRSTEDGELOWTOHIGH);
        }
    }
    else/*this is not the first reception, at least one bit was already started*/
    {
        if(!StateHigh)/*Falling edge*/
        {
            if(HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number == 9 ) /*BUG Found 9 -> 10 nine should be replaced by ten after testing*/
            {/*Next we will forward 9 th EOM  bit, so now, maybe we have to acknowledge*/

                if(HDMI_Data_p->CEC_Params.IsMsgToAck)
                {
                    CEC_Send_Acknowledge(InterruptTime);/*Acknowledge low level since new Fall Time*/
                }
            }
            /*Forwarding information about previous bit*/
            CEC_Bit_Recognise(  Time_Minus(HDMI_Data_p->CEC_Params.Last_Bit_Rise_Time, HDMI_Data_p->CEC_Params.Last_Bit_Fall_Time),
                                Time_Minus(InterruptTime, HDMI_Data_p->CEC_Params.Last_Bit_Fall_Time) );
            /*Now think about the new starting bit*/
            HDMI_Data_p->CEC_Params.Last_Bit_Fall_Time = InterruptTime;
            #ifdef CEC_DEBUG_PIO_RX_RECEPTION
            STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT0);
            #endif
        }
        else/*if (StateHigh)/ /Rising edge*/
        {
            /*Here we can check for low pulse width if we want instantenous error reaction*/
            HDMI_Data_p->CEC_Params.Last_Bit_Rise_Time = InterruptTime;
            #ifdef CEC_DEBUG_PIO_RX_RECEPTION
            STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT0);
            #endif

            if(HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number == 10)/*Only if it's not acknowledged*/
            {/*This is last bit, might be one, might be our own acknowledge, might ba a different acknowledge*/

                if(HDMI_Data_p->CEC_Params.IsEndOfMsg)/*EOM ?*/
                {
                    CEC_Bit_Recognise(  Time_Minus(HDMI_Data_p->CEC_Params.Last_Bit_Rise_Time, HDMI_Data_p->CEC_Params.Last_Bit_Fall_Time),
                                        CEC_Time_Data_bit_Finish() );
                }
                /*else if (it's not the EOM)*/
                /*{it will be forwarded on the next Falling edge of the next bit}*/
            }
        }
    }
}/* end of stvout_CEC_ReceptionInterrupt() */


/*******************************************************************************
Name        : CEC_Release_PIO
Description : Just release the PIO, do not check for collision
Parameters  : State of the CEC Line
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Release_PIO(STPIO_Handle_t Handle)
{
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;
    UNUSED_PARAMETER(Handle);

    HDMI_Data_p->CEC_Params.BitConfigure[GetPos(HDMI_Data_p->CECPIO_BitNumber)] = STPIO_BIT_INPUT;
    STPIO_SetConfig(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, HDMI_Data_p->CEC_Params.BitConfigure);
    CEC_Rx_Int_Enable(HDMI_Data_p ,1,0); /* Enable Rx Int, Low causes interrupt */
}/* end of CEC_Release_PIO() */
/*******************************************************************************
Name        : CEC_Set_PIO
Description :
Parameters  : State of the CEC Line
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void CEC_Set_PIO(STPIO_Handle_t Handle,BOOL State)
{
    U8 PIO_Buffer;
    U32 Max_Poll_Time_Before_Collision;
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;
    BOOL StateLevel;
    U32 EventTime;

    if(State)
    {

        /*STPIO_Set(Handle, HDMI_Data_p->CECPIO_BitNumber); This call is not needed as it will go to input High impedance state */
        HDMI_Data_p->CEC_Params.BitConfigure[GetPos(HDMI_Data_p->CECPIO_BitNumber)] = STPIO_BIT_INPUT;
        STPIO_SetConfig(Handle,HDMI_Data_p->CEC_Params.BitConfigure);
        /* There must be a delay here to avoid reception interrupt triggering, but how  can we make a delay inside an interrupt  ?? */
        /* We actively Poll for the PIO till it gets high before we activate its interrupt or else we would have an instantanuous
         * non-useful interrupt, at worst case, it would be considered as a collision, and if such case would be handled, that
         * would in any case charge more CPU time than this polling
         * - We limit the polling for a fixed Time because on unusual conditions like collisions, we would have to serve
         * this interrupt soon or late ! */
        Max_Poll_Time_Before_Collision = CEC_CPTCMP_TimeNow(HDMI_Data_p) + MAX_CEC_PIO_Rise_Time();
        do
        {
            STPIO_Read(Handle,&PIO_Buffer);
        }
        while(      Time_First_Previous(CEC_CPTCMP_TimeNow(HDMI_Data_p),Max_Poll_Time_Before_Collision) &&
                    ((PIO_Buffer&HDMI_Data_p->CECPIO_BitNumber) == 0)                                   &&
                    (HDMI_Data_p->CEC_Params.PwmCompareHandle != 0)
             );

#ifdef STVOUT_CEC_CAPTURE
        /* Handle Collision Manually Now, cause the capture detects only at edge changes */
        STPIO_Read(Handle,&PIO_Buffer);
        if( (PIO_Buffer & HDMI_Data_p->CECPIO_BitNumber) == 0 )
        {   /* A Collision is happening */
            StateLevel = FALSE;
            EventTime = CEC_CPTCMP_TimeNow(HDMI_Data_p);
            stvout_CEC_ReceptionInterrupt(HDMI_Data_p, StateLevel, EventTime);
        }
#endif
        /* Collision handled at Rx interrupt level for non Capture, no effect for Capture */
        CEC_Rx_Int_Enable(HDMI_Data_p,1,0);/*Enabled, Low cause Interrupt*/
    }
    else
    {
        CEC_Rx_Int_Enable(HDMI_Data_p,0,0);             /*Int Disabled*/
        STPIO_Clear(Handle, HDMI_Data_p->CECPIO_BitNumber);      /*PIO Cleared*/
        HDMI_Data_p->CEC_Params.BitConfigure[GetPos(HDMI_Data_p->CECPIO_BitNumber)] = STPIO_BIT_OUTPUT;
        STPIO_SetConfig(Handle,HDMI_Data_p->CEC_Params.BitConfigure);
    }
}/* end of CEC_Set_PIO() */

#ifdef STVOUT_CEC_CAPTURE
/*******************************************************************************
Name        : stvout_CEC_Capture_Interrupt: Capture Reception Interruppt
Parameters  : Param, HDMI Driver Handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void stvout_CEC_Capture_Interrupt(void * Param)
{
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;
    U32 PIO_Int_TimeCapture;
    BOOL StateHigh;
    U8 PIO_Buffer;
    UNUSED_PARAMETER(Param);

    STPWM_GetCaptureValue(HDMI_Data_p->CEC_Params.PwmCaptureHandle,&PIO_Int_TimeCapture);

    STPIO_Read(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, &PIO_Buffer);
    if((PIO_Buffer & HDMI_Data_p->CECPIO_BitNumber) == 0 )
    {
        StateHigh = FALSE;
    }
    else
    {
        StateHigh = TRUE;
    }

    stvout_CEC_ReceptionInterrupt(HDMI_Data_p, StateHigh, PIO_Int_TimeCapture);

}/*end of stvout_CEC_Capture_Interrupt() */

#else /* If not defined STVOUT_CEC_CAPTURE */
/*******************************************************************************
Name        : stvout_CEC_PIO_Interrupt   :           Reception Interruppt
Description : Recept interrupt event
Parameters  : EVT standard prototype
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void stvout_CEC_PIO_Interrupt(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)
{
    #ifdef CEC_DEBUG_1
    U32 Debug;
    #endif
    HDMI_Data_t *HDMI_Data_p = HDMI_Driver_p;
    U32 PIO_Int_Time;
    UNUSED_PARAMETER(Handle);

    if(ActiveBits & HDMI_Data_p->CECPIO_BitNumber)
    {
        PIO_Int_Time = CEC_CPTCMP_TimeNow(HDMI_Data_p);

        stvout_CEC_ReceptionInterrupt(HDMI_Data_p,HDMI_Data_p->CEC_Params.IsReception_Interrupt_State_High, PIO_Int_Time);

        if(HDMI_Data_p->CEC_Params.IsReception_Interrupt_State_High)/*interrupt is high, so now high so next is low*/
        {
            CEC_Rx_Int_Enable(HDMI_Data_p ,1,0);/*next is low*/

        }else
        {
            CEC_Rx_Int_Enable(HDMI_Data_p,1,1);/*next is high*/
        }
    }
}/*end of stvout_CEC_PIO_Interrupt() */
#endif /* STVOUT_CEC_CAPTURE */

/*******************************************************************************
Name        : stvout_CEC_CompareBitTx_Interrupt: Transmission Interruppt
Description : Transmit interrupt event, decide what bit state to set
 *              and pre-program the time of the next interrupt
Parameters  : Param, HDMI Driver Handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void stvout_CEC_CompareBitTx_Interrupt(void * Param)
{
    HDMI_Data_t  *HDMI_Data_p;
    U32 StateDuration=0; /* Still can cause re-interrupt if enabled */
    U8  CurrentWord;
    BOOL    BitLogic;
    static U32     CMP_Time_PIO_Change, Last_CMP_Time_PIO_Change;
    HDMI_Data_p = (HDMI_Data_t *)Param;

    #ifdef CEC_DEBUG_PIO_TX_INT
    STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT1);
    #endif

    Last_CMP_Time_PIO_Change = CMP_Time_PIO_Change;

    CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);

    if(HDMI_Data_p->CEC_Params.IsBusIdleDelaying) /* This is the end of the delaying period */
    {
        HDMI_Data_p->CEC_Params.IsBusFree = TRUE;
        HDMI_Data_p->CEC_Params.IsBusIdleDelaying = FALSE;
        CEC_Tx_Int_Enable(HDMI_Data_p,0);/*Int Disabled*/
        /* Release The PIO after Error Notification */
        CEC_Release_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle);
        STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);
    }
    else
    if(HDMI_Data_p->CEC_Params.IsTransmitting)
    {
        if(HDMI_Data_p->CEC_Params.TxMessageLength != 0)
        {
            if(HDMI_Data_p->CEC_Params.TxCurrent_Word_Number == 0)
            {
                if(HDMI_Data_p->CEC_Params.IslineHigh)
                {
                    /*Start of the Transmission on the Line*/
                    /* Everything is fine for the Start bit, for minimal timing use */
                    /* ! ! ! Interrupt Lock ! ! ! */
                    CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,0);/*This must be the first falling edge of the start bit*/
                    CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                    /* ! ! ! Interrupt UnLock ! ! ! */

                    HDMI_Data_p->CEC_Params.TxInterrupt_Time = CMP_Time_PIO_Change;
                    #ifdef CEC_DEBUG_PIO_TX_TRANSMISSION
                    STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT2);/*Start Transmission*/
                    #endif
                    HDMI_Data_p->CEC_Params.IslineHigh = FALSE;
                    StateDuration = CEC_Time_Start_bit_Low();
                }
                else
                {

                    /* ! ! ! Interrupt Lock ! ! ! */
                    CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,1); /* Rising edge during start bit */
                    /* Transmission + Collision case : */
                    if(!HDMI_Data_p->CEC_Params.IsBusIdleDelaying)
                    {
                        CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                        /* ! ! ! Interrupt UnLock ! ! ! */
                        HDMI_Data_p->CEC_Params.IslineHigh = TRUE;
                        StateDuration = CEC_Time_Start_bit_High() - Time_Minus( CMP_Time_PIO_Change , Last_CMP_Time_PIO_Change);
                        HDMI_Data_p->CEC_Params.TxCurrent_Word_Number++;/*Next will be beginning of header word*/
                    }
                    else
                    {
                        CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                        StateDuration = 6*CEC_NominalBitPeriod();
                    }
                }
            }
            else/*Send the current word number*/
            {
                CurrentWord = HDMI_Data_p->CEC_Params.TxMessageList[HDMI_Data_p->CEC_Params.TxCurrent_Word_Number - 1];     /*word 0 is start bit*/

                if((HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 1)&&(HDMI_Data_p->CEC_Params.IsTxWaitingAckBit))
                {
                    #ifdef CEC_DEBUG_TX_ACK_SAMPLE
                    STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
                    #endif
                    if(HDMI_Data_p->CEC_Params.IsTxAckReceived)
                    {
                        /*ACK Received with Success*/
                        HDMI_Data_p->CEC_Params.IsTxAllMsgNotAcknowledged = FALSE;
                        #ifdef CEC_DEBUG_TX_ACK_SAMPLE
                        STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
                        #endif
                    }
                    else
                    {
                        HDMI_Data_p->CEC_Params.IsTxAllMsgAcknowledged = FALSE;
                        /*CEC_Error(CEC_ERROR_ACK_WAS_NOT_RECEIVED);*/ /*Be careful, this error disables the transmission*/
                    }
                    HDMI_Data_p->CEC_Params.IsTxAckReceived = FALSE;
                    HDMI_Data_p->CEC_Params.IsTxWaitingAckBit = FALSE;/*ACK Period Finished*/
                }

                if( (HDMI_Data_p->CEC_Params.TxCurrent_Word_Number) <= HDMI_Data_p->CEC_Params.TxMessageLength )            /*word 0 is start bit*/
                {
                    if(HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 9)
                    {
                        if((HDMI_Data_p->CEC_Params.TxCurrent_Word_Number) == HDMI_Data_p->CEC_Params.TxMessageLength )   /*word 0 is start bit*/
                        {
                            BitLogic = 1; /*This is the End Of Message*/
                        }
                        else
                        {
                            BitLogic = 0; /*This is not the End Of Message*/
                        }
                    }
                    else if(HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 10)
                    {
                        BitLogic = 1;/*The acknowledge is always One*/
                    }
                    else
                    {
                        BitLogic = (1<<(8-HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number)) & CurrentWord;
                    }

                    if(HDMI_Data_p->CEC_Params.IslineHigh)
                    {
                        /* ! ! ! Interrupt Lock ! ! ! */
                        CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,0); /* New data bit just started */
                        CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                        /* ! ! ! Interrupt UnLock ! ! ! */
                        #ifdef CEC_DEBUG_TX_ACK_SAMPLE
                        if(HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 10)
                        {
                            STPIO_Set(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
                        }
                        #endif
                        if(HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 10)
                        {
                            HDMI_Data_p->CEC_Params.TxStart_Of_ACK_Bit = CMP_Time_PIO_Change;
                        }


                        HDMI_Data_p->CEC_Params.IslineHigh = FALSE;
                        if(BitLogic)
                        {
                            StateDuration = CEC_Time_One_bit_Low();
                        }
                        else
                        {
                            StateDuration = CEC_Time_Zero_bit_Low();
                        }
                    }
                    else/*This is the Rising Edge*/
                    {

                        if(HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 10)/*ACK Tx is releasing the Line*/
                        {/*From here should start the ACK Checking !! !! !!*/
                            HDMI_Data_p->CEC_Params.IsTxWaitingAckBit = TRUE;/*In few instructions forward, will be the release of the PIO,*/
                            HDMI_Data_p->CEC_Params.IsTxAckReceived = FALSE;
                            /* ! ! ! Interrupt Lock ! ! ! */
                            /* JUST Releasing the PIO */
                            CEC_Release_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle);
                            #ifdef CEC_DEBUG_TX_ACK_SAMPLE
                            STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT3);
                            #endif
                        }
                        else
                        {
                            /* ! ! ! Interrupt Lock ! ! ! */
                            CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,1);
                        }
                        if(!HDMI_Data_p->CEC_Params.IsBusIdleDelaying)
                        {
                            CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                            /* ! ! ! Interrupt UnLock ! ! ! */
                            HDMI_Data_p->CEC_Params.IslineHigh = TRUE;
                            if(BitLogic)
                            {
                                StateDuration = CEC_Time_One_bit_High() - Time_Minus( CMP_Time_PIO_Change , Last_CMP_Time_PIO_Change);
                            }
                            else
                            {
                                StateDuration = CEC_Time_Zero_bit_High() - Time_Minus( CMP_Time_PIO_Change , Last_CMP_Time_PIO_Change);
                            }

                            HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number++;
                            if(HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number == 11)
                            {/*Last bit of word (ACK) is beeing sent, next should start next word*/
                                HDMI_Data_p->CEC_Params.TxCurrent_Word_Number++;
                                HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number = 1;/*Reset the bit counter*/

                                /* if(HDMI_Data_p->CEC_Params.TxCurrent_Word_Number > HDMI_Data_p->CEC_Params.TxMessageLength)
                                * We finished sending the message, but now we're just rising the line a final time
                                * and should wait for the last high period
                                * */
                            }
                        }
                        else
                        {
                            CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                            StateDuration = 6*CEC_NominalBitPeriod();
                        }
                    }
                }
                else/*The message ended on the previous byte*/
                {
                    CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);

                    #ifdef CEC_DEBUG_PIO_TX_TRANSMISSION
                    STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT2);/*End of Transmission*/
                    #endif
                    HDMI_Data_p->CEC_Params.IsTransmitting = FALSE;

                    if(HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].CEC_Message.Destination == CEC_BROADCAST)
                    {
                        /*Here we should*/
                        if(HDMI_Data_p->CEC_Params.IsTxAllMsgNotAcknowledged)
                        {
                            HDMI_Data_p->CEC_Params.Notify |= CEC_MESSAGE_NOT_SENT;
                        }
                        else
                        {
                            HDMI_Data_p->CEC_Params.Notify |= CEC_MESSAGE_SENT;
                        }
                    }
                    else
                    {
                        if(HDMI_Data_p->CEC_Params.IsTxAllMsgAcknowledged)
                        {
                            HDMI_Data_p->CEC_Params.Notify |= CEC_MESSAGE_SENT;
                        }
                        else
                        {
                            HDMI_Data_p->CEC_Params.Notify |= CEC_MESSAGE_NOT_SENT;
                            /* Notify Message was lost and would need retransmission */
                        }
                    }
                    STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);

                    /*On both Sent, not sent cases, the bus wiil be kept idle As we reached this step,
                     * even if not acknowledged, the transmission is considered to be successfully sent*/
                    HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
                    StateDuration = CEC_IdleBusDelay_SendAgain();
                    /* We disable no more cause we need it for the Bus delay */
                    /* CEC_Tx_Int_Enable(HDMI_Data_p,0); */ /* no, Keep enabled */
                }
            }
        }
    }
    else
    if(HDMI_Data_p->CEC_Params.IsReceiving)
    {
        /*HDMI_Data_p->CEC_Params.IsMsgToAck*/
        if(HDMI_Data_p->CEC_Params.IsRxSendingAckBit)/*End of Sending ACK*/
        {
            HDMI_Data_p->CEC_Params.IsRxSendingAckBit = FALSE;/*OK, Already sent*/
            /*CEC_Tx_Int_Enable(HDMI_Data_p,0);*/ /*Int Disabled*/ /* Will be reenabled later by the reception idle period */
            /*Now we have lost the rising edge of this last ACK bit, we have to notify it
            CEC_AddBit(CEC_BIT_ZERO); no this do not update the interrupt state that is useful for next state
            PIO set just now couldn't be notified to the reception interruption, so we do it manually :
            in fact, we do not receive what we manually change, here's the only place where during
            reception we change something manually */

            CEC_Set_PIO(HDMI_Data_p->CEC_Params.CEC_PIO_Handle,1);
            stvout_CEC_ReceptionInterrupt(HDMI_Data_p, TRUE, CMP_Time_PIO_Change); /* (Why?)? because when we rise it manually we don't receive it /!\ */

            #ifdef CEC_DEBUG_RX_ACK_SEND
            STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT2);
            #endif




            /* On this point, ACK ended */
            if(HDMI_Data_p->CEC_Params.IsEndOfMsg)/*EOM ?*/
            {   /* Last ACK of the whole message ended just now*/
                if(HDMI_Data_p->CEC_Params.IsPreviousUnsuccessful)
                {
                    StateDuration = CEC_IdleBusDelay_Retransmission();
                }
                else
                {
                    StateDuration = CEC_IdleBusDelay_NewSending();
                }
                HDMI_Data_p->CEC_Params.IsPreviousUnsuccessful = FALSE;
                HDMI_Data_p->CEC_Params.IsBusIdleDelaying = TRUE;
                CMP_Time_PIO_Change = CEC_CPTCMP_TimeNow(HDMI_Data_p);
                /*CEC_Tx_Int_Enable(HDMI_Data_p,1);*/ /*Already Enabled*/
            }
            else
            {   /* ACK of a byte with no EOM, hope it really won't the last !
                 * we no more need Tx as we're on reception, this was just for ACK */
                CEC_Tx_Int_Enable(HDMI_Data_p,0); /*Disable*/
            }
        }
        else
        {
            CEC_Error(CEC_ERROR_TX_INT_AND_NOT_ACK_READ);
        }
    }


    /* ! ! ! Interrupt Lock ! ! ! */
    /* only Minimal Timing way was kept */
    STPWM_CounterControl(HDMI_Data_p->CEC_Params.PwmCompareHandle, STPWM_CompareCEC, DISABLE);
        HDMI_Data_p->CEC_Params.TxInterrupt_Time = CMP_Time_PIO_Change + StateDuration;
        if(Time_First_Previous(HDMI_Data_p->CEC_Params.TxInterrupt_Time,   CEC_CPTCMP_TimeNow(HDMI_Data_p)) )  /* safe against overflow*/
        {
            HDMI_Data_p->CEC_Params.TxInterrupt_Time = CEC_CPTCMP_TimeNow(HDMI_Data_p) + 1;
        }
        STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, HDMI_Data_p->CEC_Params.TxInterrupt_Time);
    STPWM_CounterControl(HDMI_Data_p->CEC_Params.PwmCompareHandle, STPWM_CompareCEC, ENABLE);
    /* ! ! ! Interrupt UnLock ! ! ! */

    #ifdef CEC_DEBUG_PIO_TX_INT
    STPIO_Clear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, CEC_PIO_DEBUG_BIT1);
    #endif
}/* end of stvout_CEC_CompareBitTx_Interrupt() */


/*******************************************************************************
Name        : CEC_InterruptsInstall
Description : Install Call back Capture and Compare.
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t CEC_InterruptsInstall(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p = (HDMI_Data_t *) Handle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STPWM_OpenParams_t          PwmOpenParams;
    STPIO_OpenParams_t          PioOpenParams;/*PIO 1.0  PIO 2.3*/
    STPWM_CallbackParam_t       CallbackParam;
#ifdef STVOUT_CEC_CAPTURE
    STPWM_EdgeControl_t     EdgeControl;
#endif
    U8 AddressIndex;

    /*Open The CEC PIO Handle */
#ifdef STVOUT_CEC_CAPTURE
    PioOpenParams.IntHandler            = NULL;
#else
    PioOpenParams.IntHandler            = stvout_CEC_PIO_Interrupt;
#endif
    PioOpenParams.ReservedBits          = HDMI_Data_p->CECPIO_BitNumber;
    PioOpenParams.BitConfigure[GetPos(HDMI_Data_p->CECPIO_BitNumber)] = STPIO_BIT_INPUT;

    #ifdef CEC_DEBUG_PIO_RX_RECEPTION
    PioOpenParams.ReservedBits |= CEC_PIO_DEBUG_BIT0;
    PioOpenParams.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT0)] = STPIO_BIT_OUTPUT;
    HDMI_Data_p->CEC_Params.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT0)] = STPIO_BIT_OUTPUT;
    #endif
    #ifdef CEC_DEBUG_PIO_TX_INT
    PioOpenParams.ReservedBits |= CEC_PIO_DEBUG_BIT1;
    PioOpenParams.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT1)] = STPIO_BIT_OUTPUT;
    HDMI_Data_p->CEC_Params.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT1)] = STPIO_BIT_OUTPUT;
    #endif
    #ifdef CEC_DEBUG_RX_ACK_SEND
    PioOpenParams.ReservedBits |= CEC_PIO_DEBUG_BIT2;
    PioOpenParams.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT2)] = STPIO_BIT_OUTPUT;
    HDMI_Data_p->CEC_Params.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT2)] = STPIO_BIT_OUTPUT;
    #endif
    #if defined(CEC_DEBUG_TX_ACK_SAMPLE) || defined(CEC_DEBUG_PIO_ERROR)
    PioOpenParams.ReservedBits |= CEC_PIO_DEBUG_BIT3;
    PioOpenParams.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT3)] = STPIO_BIT_OUTPUT;
    HDMI_Data_p->CEC_Params.BitConfigure[GetPos(CEC_PIO_DEBUG_BIT3)] = STPIO_BIT_OUTPUT;
    #endif

    ErrorCode = STPIO_Open(HDMI_Data_p->CECPIOName, &PioOpenParams, &HDMI_Data_p->CEC_Params.CEC_PIO_Handle);
    /*ErrorCode = STPIO_Open("PIO5", &PioOpenParams, &HDMI_Data_p->CEC_Params.CEC_PIO_Handle);*/
    if (ErrorCode != ST_NO_ERROR)
        return(ST_ERROR_OPEN_HANDLE);
    ErrorCode = STPIO_SetCompareClear(HDMI_Data_p->CEC_Params.CEC_PIO_Handle, STPIO_COMPARE_CLEAR_AUTO);/*Automatic acknowledge*/
        if (ErrorCode != ST_NO_ERROR)
        return(ST_ERROR_BAD_PARAMETER);

    /* Open PWM for CEC TX */
    PwmOpenParams.PWMUsedFor = STPWM_CompareCEC;
    PwmOpenParams.C4.ChannelNumber = STPWM_CHANNEL_CECTX;
    PwmOpenParams.C4.PrescaleFactor = STPWM_CPTCMP_PRESCALE_MAX;
        ErrorCode = STPWM_Open (HDMI_Data_p->PWMName, &PwmOpenParams, &HDMI_Data_p->CEC_Params.PwmCompareHandle);
        if (ErrorCode != ST_NO_ERROR)
        return(ST_ERROR_OPEN_HANDLE);


#ifdef STVOUT_CEC_CAPTURE
    /* Open PWM for CEC RX Capture */
    PwmOpenParams.PWMUsedFor = STPWM_CaptureCEC;
    PwmOpenParams.C4.ChannelNumber = STPWM_CHANNEL_CECRX;
    PwmOpenParams.C4.PrescaleFactor = STPWM_CPTCMP_PRESCALE_MAX;
        ErrorCode = STPWM_Open (HDMI_Data_p->PWMName, &PwmOpenParams, &HDMI_Data_p->CEC_Params.PwmCaptureHandle);
        if (ErrorCode != ST_NO_ERROR)
        return(ST_ERROR_OPEN_HANDLE);

    /* Set Edge of Capture Interrupt */
    EdgeControl = STPWM_RisingOrFallingEdge;
    ErrorCode = STPWM_SetCaptureEdge(HDMI_Data_p->CEC_Params.PwmCaptureHandle,EdgeControl);
        if (ErrorCode != ST_NO_ERROR)
        return(ST_ERROR_BAD_PARAMETER);

    /* Set CallBack for CEC Reception */
    CallbackParam.CallbackData_p = (void *) HDMI_Data_p;
    CallbackParam.CallBack = stvout_CEC_Capture_Interrupt;
    ErrorCode = STPWM_SetCallback(HDMI_Data_p->CEC_Params.PwmCaptureHandle,&CallbackParam,STPWM_CaptureCEC);
    if (ErrorCode != ST_NO_ERROR)
    return(ErrorCode);
#endif

    /* Set CallBack for CEC Transmission */
    CallbackParam.CallbackData_p = (void *) HDMI_Data_p;
    CallbackParam.CallBack = stvout_CEC_CompareBitTx_Interrupt;
    ErrorCode = STPWM_SetCallback(HDMI_Data_p->CEC_Params.PwmCompareHandle,&CallbackParam,STPWM_CompareCEC);
    if (ErrorCode != ST_NO_ERROR)
    return(ErrorCode);

    /*---------------The First General Initialisation-------------------*/
    /*Init General-------------------------------------------------*/
    /* Message_Buffer.Init */
    HDMI_Data_p->CEC_Params.MesBuf_Read_Index = 0;
    HDMI_Data_p->CEC_Params.MesBuf_Write_Index = 0;
    HDMI_Data_p->CEC_Params.IsBufferEmpty = TRUE;

    HDMI_Data_p->CEC_Params.IsRxSendingAckBit = FALSE;
    HDMI_Data_p->CEC_Params.IsTxWaitingAckBit = FALSE;
    HDMI_Data_p->CEC_Params.IsTxAckReceived = FALSE;
    STPWM_CounterControl(HDMI_Data_p->CEC_Params.PwmCompareHandle, STPWM_CompareCEC, ENABLE); /*Counter Enabled for Both Transmission and Reception*/
    HDMI_Data_p->CEC_Params.IsTransmitting = FALSE;

    HDMI_Data_p->CEC_Params.Compare.ComparePattern = 0;
    HDMI_Data_p->CEC_Params.Compare.CompareEnable  = 0;
    HDMI_Data_p->CEC_Params.BitConfigure[GetPos(HDMI_Data_p->CECPIO_BitNumber)] = STPIO_BIT_INPUT;

    HDMI_Data_p->CEC_Params.Notify = 0;

    HDMI_Data_p->CEC_Params.IsBusFree = TRUE;
    HDMI_Data_p->CEC_Params.IsBusIdleDelaying = FALSE;
    HDMI_Data_p->CEC_Params.IsPreviousUnsuccessful = FALSE;

    HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated = FALSE;
    HDMI_Data_p->CEC_Params.IsValidPhysicalAddressObtained = FALSE;
    HDMI_Data_p->CEC_Params.IsFirstPingStarted = FALSE;
    HDMI_Data_p->CEC_Params.Role = CEC_DEVICE_ROLE;
    switch(HDMI_Data_p->CEC_Params.Role)
    {
        /* for future developement */
        case STVOUT_CEC_ROLE_PLAYBACK :
            AddressIndex = 0;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_PLAYBACK_DEVICE1;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_PLAYBACK_DEVICE2;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_PLAYBACK_DEVICE3;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_FREEUSE;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_UNREGISTRED;
            HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex = AddressIndex; /* Startup with Last Address, Must be "unregistred" */
            break;
        default :
            AddressIndex = 0;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER1;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER2;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER3;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER4;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_FREEUSE;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_UNREGISTRED;
            HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex = AddressIndex; /* Startup with Last Address, Must be "unregistred" */
            break;
    }

    HDMI_Data_p->CEC_Params.FirstLogicAddress = -1;

    /*Init Reception-----------------------------------------------*/
    #ifdef CEC_DEBUG_1
    DebugTimes1[DebugCount1++] = CEC_CPTCMP_TimeNow(HDMI_Data_p);
    #endif

    /*RxMessageList Considered only through Word and bit numbers*/
    /*RxMessageLength; This will be a resul*/
    HDMI_Data_p->CEC_Params.RxCurrent_Word_Number = 0;/*Q Why 0 ? A 0 is start bit*/
    HDMI_Data_p->CEC_Params.RxCurrent_Bit_Number = 1;
    HDMI_Data_p->CEC_Params.IsMsgToAck = FALSE ;/*By default no, unless needed address is found*/
    HDMI_Data_p->CEC_Params.IsReceiving = FALSE;/*We're not recieving, state is idle till a high to low edge is detected*/

    /*Allow Reception to Start-------------------------------------------------*/
    HDMI_Data_p->CEC_Params.State =CEC_DEBUG_TX_RX;/* CEC_IDLE; Idle means it allow either reception or transmission to start*/
    CEC_Rx_Int_Enable(HDMI_Data_p,1,0);/*Enable, Low causes interrupt*/


    /*Ensure Transmission is Disabled-------------------------------------------------*/
    CEC_Tx_Int_Enable(HDMI_Data_p,0);/*Int Disabled*/

    return(ST_NO_ERROR);
}/*end of CEC_InterruptsInstall() */

/*******************************************************************************
Name        : CEC_StartMessageTransmission
Description : This function is called when the line is ready for message
 *          Transmission
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void CEC_StartMessageTransmission(const HDMI_Handle_t HdmiHandle)
{

    HDMI_Data_t * HDMI_Data_p;
    U8 i;
    U32                         Time;
    STVOUT_CECMessage_t * Message_p;
    HDMI_Data_p = (HDMI_Data_t *) HdmiHandle;


    /* Exculsive Access to the buffer structure */
    STOS_SemaphoreWait(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
    /* Message_Buffer.GetMessage */
        HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].Trials++;

        if( HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].Trials >
            HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].MAX_Retries+1  )
        {
            /* Message_Buffer.RemoveMessage : cause collision Errors made it end its trials*/
                    HDMI_Data_p->CEC_Params.MesBuf_Read_Index = (HDMI_Data_p->CEC_Params.MesBuf_Read_Index+1) & CEC_MESSAGE_INDEX_MASK;
                    if ( HDMI_Data_p->CEC_Params.MesBuf_Read_Index == HDMI_Data_p->CEC_Params.MesBuf_Write_Index)
                    {
                        HDMI_Data_p->CEC_Params.IsBufferEmpty = TRUE;
                        STTBX_Task_Debug((("Task::: Buffer Empty Again \n")));
                        /* Can not to signal the semaphore just because the test for IsBufferEmpty comes after this step */
                        /* NO when true, it do not get inside here */
                    }
            STOS_SemaphoreSignal(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
        }
        else
        {
                /* ----- Point onto the message to transmit ----- */
                Message_p = &HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].CEC_Message;

                /*Params to send-----------------------------------------------*/
                HDMI_Data_p->CEC_Params.TxMessageList[0] =  ( (Message_p->Source<<4)   & 0xF0)  |
                                                            ( (Message_p->Destination) & 0x0F);
                for ( i=0;i<Message_p->DataLength;i++ )  /*opti change to while and check Message_Read_p==0 */
                {
                    HDMI_Data_p->CEC_Params.TxMessageList[i+1] = Message_p->Data[i];
                }
                HDMI_Data_p->CEC_Params.TxMessageLength = Message_p->DataLength + 1;
            STOS_SemaphoreSignal(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
            /*Init Transmission-----------------------------------------------*/
            HDMI_Data_p->CEC_Params.TxCurrent_Word_Number = 0;    /*Word 0 is Start bit*/
            HDMI_Data_p->CEC_Params.TxCurrent_Bit_Number = 1;     /* First bit is 1 */
            HDMI_Data_p->CEC_Params.IslineHigh = TRUE;                   /*Off*/
            HDMI_Data_p->CEC_Params.IsTxAllMsgAcknowledged = TRUE;
            HDMI_Data_p->CEC_Params.IsTxAllMsgNotAcknowledged = TRUE;
            HDMI_Data_p->CEC_Params.IsTxWaitingAckBit = FALSE;
            HDMI_Data_p->CEC_Params.IsTxAckReceived = FALSE;
            HDMI_Data_p->CEC_Params.State = CEC_DEBUG_TX_RX;/*CEC_TX*/


            /*Start Transmission Now-----------------------------------------------*/

            /* Bus is Free For sure HDMI_Data_p->CEC_Params.IsBusFree*/

            /* Should be next IsTransmitting=TRUE, but error can recover it on the other case */
            HDMI_Data_p->CEC_Params.IsBusFree = FALSE;
            if(!HDMI_Data_p->CEC_Params.IsReceiving)
            {
                HDMI_Data_p->CEC_Params.IsTransmitting = TRUE;
                /*This is a tricky way to make the CPT CMP counter interrupt immidiatly*/
                STPWM_CounterControl(HDMI_Data_p->CEC_Params.PwmCompareHandle, STPWM_CompareCEC, DISABLE);
                    Time= CEC_CPTCMP_TimeNow(HDMI_Data_p) + 1;
                    STPWM_SetCompareValue(HDMI_Data_p->CEC_Params.PwmCompareHandle, Time);
                    CEC_Tx_Int_Enable(HDMI_Data_p,1);/*Enabled*/
                STPWM_CounterControl(HDMI_Data_p->CEC_Params.PwmCompareHandle, STPWM_CompareCEC, ENABLE);
            }
            else
            {
                CEC_Error(CEC_ERROR_CANNOT_TX_DURING_RX);
            }
        }
} /* End of CEC_StartMessageTransmission() */

/*******************************************************************************
Name        : CEC_SendLabeledMessage
Description : This function is called from the API and gives a message to
 *              handle its sending
Parameters  : MessageLabel will makes difference between Driver calls and Appli calls
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t CEC_SendLabeledMessage(  const HDMI_Handle_t HdmiHandle,
                                        const STVOUT_CECMessage_t * const Message_p,
                                        const CEC_Message_Request_t MessageLabel)
{

    HDMI_Data_t * HDMI_Data_p;
    U8 i,BufInd;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_p = (HDMI_Data_t *) HdmiHandle;



    STOS_SemaphoreWait(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
    /* Message_Buffer.AddMessage */
        BufInd = (HDMI_Data_p->CEC_Params.MesBuf_Write_Index+1) & CEC_MESSAGE_INDEX_MASK;
        if (BufInd != HDMI_Data_p->CEC_Params.MesBuf_Read_Index)
        {
            HDMI_Data_p->CEC_Params.MesBuf_Write_Index = BufInd; /* Effectively change the new writing buffer index */
            if(BufInd == 0)
            {
                BufInd = CEC_MESSAGE_INDEX_MASK;
            }
            else
            {
                BufInd--; /* But we use the old location still free */
            }

            HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].Request = MessageLabel;

            HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].CEC_Message.Source = Message_p->Source;
            HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].CEC_Message.Destination = Message_p->Destination;
            if(Message_p->Retries == 0)
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].MAX_Retries = 1; /* to continue in-spec in case of non-handled error */
            }
            else if(Message_p->Retries > 5)
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].MAX_Retries = 5; /* to continue in-spec in case of non-handled error */
            }
            else /* Good Param */
            {
                HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].MAX_Retries = Message_p->Retries;
            }
            HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].Trials = 0;
            if (Message_p->DataLength <= STVOUT_MAX_CEC_MESSAGE_LENGTH)
            {
                HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].CEC_Message.DataLength = Message_p->DataLength;
                for(i=0;i<Message_p->DataLength;i++)
                    {
                        HDMI_Data_p->CEC_Params.MessagesBuffer[BufInd].CEC_Message.Data[i] = Message_p->Data[i];
                    }

                HDMI_Data_p->CEC_Params.IsBufferEmpty = FALSE;
                STTBX_CEC_API_Debug((("API_Send::: Into - Buffer Filled \n")));
            }
            else /*DataLength too big*/
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        else /*Buffer Overflow*/
        {
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
    STOS_SemaphoreSignal(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);

    /*After Filling the Buffer, Unleash the Transmitting Task*/
    STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);



    return (ErrorCode);
} /*CEC_SendLabeledMessage*/

/*******************************************************************************
Name        : CEC_SendMessage
Description : This function is called from the API and gives a message to
 *              handle its sending
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t CEC_SendMessage(const HDMI_Handle_t HdmiHandle, const STVOUT_CECMessage_t * const Message_p)
{
    return CEC_SendLabeledMessage(HdmiHandle, Message_p, CEC_MESSAGE_APPLI_REQUEST);
}/* End of CEC_SendMessage() */


/*******************************************************************************
Name        : CEC_TaskFunction
Description : Task of CEC processing messages
Parameters  : Hdmi handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void  CEC_TaskFunction (HDMI_Handle_t const Handle)
{
  HDMI_Data_t *     const HDMI_Data_p = (HDMI_Data_t *)Handle;
  STVOUT_CECMessageInfo_t   MessageInfo;
  STVOUT_CECDevice_t CECDevice;
  U8 i;
  STVOUT_CECMessage_t CEC_Message;


STOS_TaskEnter(NULL);

    do
    {
        STOS_SemaphoreWait(HDMI_Data_p->CEC_Sem_p);

STTBX_CEC_Params_Debug((("-> BufEmpty %d -PrevErr %d -Notify %d | -Tx %d -Rx %d -Idle %d -Free %d \n",
                HDMI_Data_p->CEC_Params.IsBufferEmpty,
                HDMI_Data_p->CEC_Params.IsPreviousUnsuccessful,
                HDMI_Data_p->CEC_Params.Notify,
                HDMI_Data_p->CEC_Params.IsTransmitting,
                HDMI_Data_p->CEC_Params.IsReceiving,
                HDMI_Data_p->CEC_Params.IsBusIdleDelaying,
                HDMI_Data_p->CEC_Params.IsBusFree
                )));
        /*Here we put the for loop*/

        /*Here we should lock the interrupts in order to avoid conflicting access*/

        if( ((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_NOT_SENT) == CEC_MESSAGE_NOT_SENT) || ((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_SENT) == CEC_MESSAGE_SENT))
        {
            /* IF(Message, not broadcast, wasn't ACK, retry again?) */
            if  (    ((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_NOT_SENT) == CEC_MESSAGE_NOT_SENT) &&
                     (  HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].Trials <=
                        HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].MAX_Retries  ) &&
                     (HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].CEC_Message.Destination != CEC_BROADCAST)/* Not if it's a broadcast, rather develop opposite case of ACK on Broadcast */
                )
            {
                /* Just Acknowledge this without removing the message from the buffer, and the sending part of the Task will RE-send it*/
                HDMI_Data_p->CEC_Params.Notify &= ~CEC_MESSAGE_NOT_SENT;
            }
            /* ELSE IF(Broadcast, was ACK, retry again?) */
            else if(    ((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_SENT) == CEC_MESSAGE_SENT) &&
                        (  HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].Trials <=
                           HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].MAX_Retries  ) &&
                        (HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].CEC_Message.Destination == CEC_BROADCAST)
                   )
            {
                /* Just Acknowledge this without removing the message from the buffer, and the sending part of the Task will RE-send it*/
                /* Farther MORE !! This broadcast wasn't accepted by at least one listener that Put a negative acknowledge : Low for a Broadcast */
                HDMI_Data_p->CEC_Params.Notify &= ~CEC_MESSAGE_SENT;
            }
            /* ELSE, Message ACK or Broadcast not stopped by NACK */
            else
            {
                if(HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].CEC_Message.Destination == CEC_BROADCAST)
                {
                    if((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_NOT_SENT) == CEC_MESSAGE_NOT_SENT)
                    {
                        STTBX_Task_Debug((("CEC Message Broadcasted \n")));
                        MessageInfo.Status = STVOUT_CEC_STATUS_TX_SUCCEED;
                    }
                    if((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_SENT) == CEC_MESSAGE_SENT)
                    {
                        STTBX_Task_Debug((("CEC Message <NOT> Broadcasted (ACK) present \n")));
                        MessageInfo.Status = STVOUT_CEC_STATUS_TX_FAILED;
                    }
                }
                else
                {
                    if((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_NOT_SENT) == CEC_MESSAGE_NOT_SENT)
                    {
                        STTBX_Task_Debug((("CEC Message <NOT> Sent (no ACK) \n")));
                        MessageInfo.Status = STVOUT_CEC_STATUS_TX_FAILED;
                    }
                    if((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_SENT) == CEC_MESSAGE_SENT)
                    {
                        STTBX_Task_Debug((("CEC Message Sent \n")));
                        MessageInfo.Status = STVOUT_CEC_STATUS_TX_SUCCEED;
                    }
                }
                MessageInfo.Message.Source = HDMI_Data_p->CEC_Params.TxMessageList[0]>>4;
                MessageInfo.Message.Destination = HDMI_Data_p->CEC_Params.TxMessageList[0] & 0x0F;
                 /* We give back the real number of attempted trials, if succeeded from first hit, this will be 1, thus max is 6 */
                MessageInfo.Message.Retries = HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].Trials;
                STTBX_Task_Debug((("     Header : From %u to %u \n",          MessageInfo.Message.Source,
                                                                        MessageInfo.Message.Destination)));
                if ( HDMI_Data_p->CEC_Params.TxMessageLength > 15)
                {
                    HDMI_Data_p->CEC_Params.TxMessageLength = 0;/*A problem Happened*/
                }

                MessageInfo.Message.DataLength = HDMI_Data_p->CEC_Params.TxMessageLength-1;
                for ( i=0;i<HDMI_Data_p->CEC_Params.TxMessageLength-1;i++)
                {
                    MessageInfo.Message.Data[i] = HDMI_Data_p->CEC_Params.TxMessageList[i+1];               /*Fill the message for the user*/
                    STTBX_Task_Debug((("     CEC Data_%u : %u \n",i,HDMI_Data_p->CEC_Params.TxMessageList[i])));
                }

                /*if("It's a driver Ping")*/
                if(HDMI_Data_p->CEC_Params.MessagesBuffer[HDMI_Data_p->CEC_Params.MesBuf_Read_Index].Request == CEC_MESSAGE_DRIVER_PING)
                {
                    STTBX_Task_Debug((("Task::: Driver Ping \n")));
                    if ((HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_NOT_SENT) == CEC_MESSAGE_NOT_SENT) /* Address is Free */
                    {
                        /* HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex] == MessageInfo.Message.Destination */
                        HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated = TRUE;
                    }
                    else /* Address is reserved */
                    {
                        HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex++;
                        if ( HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex] == CEC_UNREGISTRED )
                        {
                            HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated = TRUE;
                        }
                        else
                        {
                            CEC_Message.DataLength = 0;
                            CEC_Message.Source = CEC_UNREGISTRED;
                            CEC_Message.Destination = HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex];
                            CEC_Message.Retries = 1;
                            CEC_SendLabeledMessage(Handle, &CEC_Message, CEC_MESSAGE_DRIVER_PING);      /* This is a Ping from the driver */  /* Ping next address */
                        }
                    }
                    if(HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated)
                    {
                        /* Here we notify the application */
                        STTBX_Task_Debug((("Task::: Address %d Allocated \n", HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex])));
                        CECDevice.LogicalAddress = HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex];
                        CECDevice.Role = HDMI_Data_p->CEC_Params.Role ;

                        #if defined ST_OSLINUX
                        STEVT_Notify(HDMI_Data_p->EventsHandle,
                                        HDMI_Data_p->RegisteredEventsID[HDMI_CEC_LOGIC_ADDRESS_NOTIFY_EVT_ID],
                                        STEVT_EVENT_DATA_TYPE_CAST &CECDevice);
                        #else

                        STEVT_Notify(HDMI_Data_p->EventsHandle,
                                        HDMI_Data_p->RegisteredEventsID[HDMI_CEC_LOGIC_ADDRESS_NOTIFY_EVT_ID],
                                        (const void*) &CECDevice);
                        #endif
                    }
                }
                else /* Request == CEC_MESSAGE_APPLI_REQUEST */
                {
                    #if defined ST_OSLINUX
                    STEVT_Notify(HDMI_Data_p->EventsHandle,
                                    HDMI_Data_p->RegisteredEventsID[HDMI_CEC_MESSAGE_EVT_ID],
                                    STEVT_EVENT_DATA_TYPE_CAST &MessageInfo);
                    #else

                    STEVT_Notify(HDMI_Data_p->EventsHandle,
                                    HDMI_Data_p->RegisteredEventsID[HDMI_CEC_MESSAGE_EVT_ID],
                                    (const void*) &MessageInfo);
                    #endif
                }

                HDMI_Data_p->CEC_Params.Notify &= ~CEC_MESSAGE_SENT;
                HDMI_Data_p->CEC_Params.Notify &= ~CEC_MESSAGE_NOT_SENT;

                /* ----- Finished dealing with this message, remove it from buffer ----- */
                STOS_SemaphoreWait(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
                /* Message_Buffer.RemoveMessage */
                    HDMI_Data_p->CEC_Params.MesBuf_Read_Index = (HDMI_Data_p->CEC_Params.MesBuf_Read_Index+1) & CEC_MESSAGE_INDEX_MASK;
                    if ( HDMI_Data_p->CEC_Params.MesBuf_Read_Index == HDMI_Data_p->CEC_Params.MesBuf_Write_Index)
                    {
                        HDMI_Data_p->CEC_Params.IsBufferEmpty = TRUE;
                        STTBX_Task_Debug((("Task::: Buffer Empty Again \n")));
                        /* Can not to signal the semaphore just because the test for IsBufferEmpty comes after this step */
                        /* NO when true, it do not get inside here */
                    }
                STOS_SemaphoreSignal(HDMI_Data_p->CEC_Params.CECStruct_Access_Sem_p);
            }
        }

        if( (HDMI_Data_p->CEC_Params.Notify & CEC_MESSAGE_RECEIVED) == CEC_MESSAGE_RECEIVED)
        {
            MessageInfo.Message.DataLength =  HDMI_Data_p->CEC_Params.RxMessageLength-1; /* -1, because Header won't be counted within Data */
            MessageInfo.Message.Source = (HDMI_Data_p->CEC_Params.RxMessageList[0])>>4;
            MessageInfo.Message.Destination = (HDMI_Data_p->CEC_Params.RxMessageList[0]) & 0x0F;

            STTBX_Task_Debug((("CEC Received Header + %u Data \n",HDMI_Data_p->CEC_Params.RxMessageLength-1)));
            STTBX_Task_Debug((("     Header : From %u to %u \n",          MessageInfo.Message.Source,
                                                                    MessageInfo.Message.Destination)));
            if ( HDMI_Data_p->CEC_Params.RxMessageLength>15)
            {
                HDMI_Data_p->CEC_Params.RxMessageLength = 0;/*A problem Happened*/
            }
            for ( i=1;i<HDMI_Data_p->CEC_Params.RxMessageLength;i++)
            {
                MessageInfo.Message.Data[i-1] = HDMI_Data_p->CEC_Params.RxMessageList[i];               /*Fill the message for the user*/
                STTBX_Task_Debug((("     CEC Data_%u : %u \n",i,HDMI_Data_p->CEC_Params.RxMessageList[i])));
            }

            MessageInfo.Status = STVOUT_CEC_STATUS_RX_SUCCEED;

            HDMI_Data_p->CEC_Params.Notify &= ~CEC_MESSAGE_RECEIVED;
            #if defined ST_OSLINUX
            STEVT_Notify(HDMI_Data_p->EventsHandle,
                            HDMI_Data_p->RegisteredEventsID[HDMI_CEC_MESSAGE_EVT_ID],
                            STEVT_EVENT_DATA_TYPE_CAST &MessageInfo);
            #else

            STEVT_Notify(HDMI_Data_p->EventsHandle,
                            HDMI_Data_p->RegisteredEventsID[HDMI_CEC_MESSAGE_EVT_ID],
                            (const void*) &MessageInfo);
            #endif
        }

        if(!HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated)
        {
            if(HDMI_Data_p->CEC_Params.IsValidPhysicalAddressObtained)
            {   /* Ping First */
                if(!HDMI_Data_p->CEC_Params.IsFirstPingStarted)
                {
                    HDMI_Data_p->CEC_Params.IsFirstPingStarted = TRUE;
                    STTBX_Task_Debug((("Task::: Into - First Pinging \n")));
                    CEC_Message.DataLength = 0;
                    CEC_Message.Source = CEC_UNREGISTRED;
                    HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex = 0;
                    CEC_Message.Destination = HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex];
                    CEC_Message.Retries = 1;
                    CEC_SendLabeledMessage(Handle, &CEC_Message, CEC_MESSAGE_DRIVER_PING);      /* This is a Ping from the driver */  /* First Ping */
                }
            }
        }

        if(!HDMI_Data_p->CEC_Params.IsBufferEmpty)
        {
            STTBX_Task_Debug((("Task::: Into - Buffer not Empty\n")));
            /* Later will test here the BusFree var */
            if(HDMI_Data_p->CEC_Params.IsBusFree)
            {
                STTBX_Task_Debug((("Task::: Into - Bus Free \n")));
                CEC_StartMessageTransmission(Handle);
            }
        }

STTBX_CEC_Params_Debug((("-< BufEmpty %d -PrevErr %d -Notify %d | -Tx %d -Rx %d -Idle %d -Free %d \n",
                HDMI_Data_p->CEC_Params.IsBufferEmpty,
                HDMI_Data_p->CEC_Params.IsPreviousUnsuccessful,
                HDMI_Data_p->CEC_Params.Notify,
                HDMI_Data_p->CEC_Params.IsTransmitting,
                HDMI_Data_p->CEC_Params.IsReceiving,
                HDMI_Data_p->CEC_Params.IsBusIdleDelaying,
                HDMI_Data_p->CEC_Params.IsBusFree
                )));

    }while(!(HDMI_Data_p->CEC_Task.ToBeDeleted));



STOS_TaskExit(NULL);

} /* end of CEC_TaskFunction() */

/*******************************************************************************
Name        : CEC_TaskStart
Description : Start the Task that will handle the High level of CEC treatement
 *              like message re sending and notifying received messages
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
ST_ErrorCode_t CEC_TaskStart(const HDMI_Handle_t Handle)
{
    HDMI_Task_t * const task_p  = &(((HDMI_Data_t *) Handle)->CEC_Task);

    ST_ErrorCode_t          Error = ST_NO_ERROR;

    if (task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* Start the task for the internal state machine */
    task_p->ToBeDeleted = FALSE;
    Error = STOS_TaskCreate ((void (*) (void*))CEC_TaskFunction,
                        (void*)Handle,
                        (((HDMI_Data_t *) Handle)->CPUPartition_p),
                         1024*16,
                        &(task_p->TaskStack),
                        (((HDMI_Data_t *) Handle)->CPUPartition_p),
                        &(task_p->Task_p),
                        &(task_p->TaskDesc),
                        STVOUT_TASK_CEC_PRIORITY,
                        "STVOUT[?].CECTask",
                        0 /*task_flags_high_priority_process*/);

    if(Error != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "CEC task created.\n"));
    return(ST_NO_ERROR);
} /* End of CEC_TaskStart() */

/*******************************************************************************
Name        : CEC_TaskStop
Description : Stop the Task that will handle the High level of CEC treatement
 *              like message re sending and notifying received messages
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
ST_ErrorCode_t CEC_TaskStop(const HDMI_Handle_t Handle)
{
    HDMI_Task_t * const task_p  = &(((HDMI_Data_t *) Handle)->CEC_Task);
    ST_ErrorCode_t          Error = ST_NO_ERROR;
    HDMI_Data_t * HDMI_Data_p;
    HDMI_Data_p = (HDMI_Data_t *) Handle;

    if (!(task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* End the function of the task here... */
    task_p->ToBeDeleted = TRUE;
    STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);
    STOS_TaskWait( &task_p->Task_p, TIMEOUT_INFINITY );
    Error = STOS_TaskDelete ( task_p->Task_p,
            HDMI_Data_p->CPUPartition_p,
            task_p->TaskStack,
            HDMI_Data_p->CPUPartition_p);
    if(Error != ST_NO_ERROR)
    {
        return(Error);
    }
    task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
}/*end of CEC_TaskStop() */
/*******************************************************************************
Name        : CEC_InterruptsUnInstall
Description : UnInstall Call back of CAPT COMP
Parameters  : Handle (IN/OUT)
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
ST_ErrorCode_t CEC_InterruptsUnInstall(const HDMI_Handle_t Handle)
{
    HDMI_Data_t * HDMI_Data_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    HDMI_Data_p = (HDMI_Data_t *) Handle;

    ErrorCode = STPIO_Close(HDMI_Data_p->CEC_Params.CEC_PIO_Handle);
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STPWM_Close (HDMI_Data_p->CEC_Params.PwmCompareHandle);
    }
    HDMI_Data_p->CEC_Params.PwmCompareHandle = 0;

#ifdef STVOUT_CEC_CAPTURE
    if(ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STPWM_Close (HDMI_Data_p->CEC_Params.PwmCaptureHandle);
    }
#endif


    return(ST_NO_ERROR);
}/*end of CEC_InterruptsUnInstall() */

/*******************************************************************************
Name        : CEC_PhysicalAddressAvailable
Description : Used just to notify the STVOUT CEC driver that the Physical
 *              address has been obtained
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void CEC_PhysicalAddressAvailable(const HDMI_Handle_t HdmiHandle)
{
    HDMI_Data_t * HDMI_Data_p;
    HDMI_Data_p = (HDMI_Data_t *) HdmiHandle;

    /* Assumed to be a new physical address so the logical address is lost in any case */
    HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated = FALSE;
    HDMI_Data_p->CEC_Params.IsValidPhysicalAddressObtained = TRUE;
    HDMI_Data_p->CEC_Params.IsFirstPingStarted = FALSE;

    STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);

}/* End of CEC_PhysicalAddressAvailable() */

/*******************************************************************************
Name        : CEC_SetAdditionalAddress
Description : Used to set additionnal logical address when
               another device (for example PVR) added to actual device
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void CEC_SetAdditionalAddress( const HDMI_Handle_t HdmiHandle, const STVOUT_CECRole_t Role )
{
    U8 AddressIndex = 0;
    HDMI_Data_t * HDMI_Data_p;
    HDMI_Data_p = (HDMI_Data_t *) HdmiHandle;

    /* Save First Address */
    if ( HDMI_Data_p->CEC_Params.FirstLogicAddress == -1)
    {
      HDMI_Data_p->CEC_Params.FirstLogicAddress = HDMI_Data_p->CEC_Params.LogicAddressList[HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex];
    }
    else /* Only 2 addresses are supported by CEC */
    {
      STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Can't allocate another logical address\n"));
      return;
    }

    /* The role will be sent to high level */
    HDMI_Data_p->CEC_Params.Role = Role ;

    switch(Role)
    {
        case STVOUT_CEC_ROLE_RECORDER :
            AddressIndex = 0;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_RECORDING_DEVICE1;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_RECORDING_DEVICE2;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_RECORDING_DEVICE3;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_FREEUSE;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_UNREGISTRED;
            HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex = AddressIndex; /* Startup with Last Address, Must be "unregistred" */
            break;

        case STVOUT_CEC_ROLE_PLAYBACK :
            AddressIndex = 0;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_PLAYBACK_DEVICE1;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_PLAYBACK_DEVICE2;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_PLAYBACK_DEVICE3;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_FREEUSE;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_UNREGISTRED;
            HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex = AddressIndex; /* Startup with Last Address, Must be "unregistred" */
            break;
        default :
            AddressIndex = 0;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER1;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER2;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER3;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_TUNER4;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_FREEUSE;
            AddressIndex++;
            HDMI_Data_p->CEC_Params.LogicAddressList[AddressIndex] = CEC_UNREGISTRED;
            HDMI_Data_p->CEC_Params.CurrentLogicalAddressIndex = AddressIndex; /* Startup with Last Address, Must be "unregistred" */
            break;
    }


    /* turn the flag */
    HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated = FALSE;
    HDMI_Data_p->CEC_Params.IsValidPhysicalAddressObtained = TRUE;
    HDMI_Data_p->CEC_Params.IsFirstPingStarted = FALSE;

    STOS_SemaphoreSignal(HDMI_Data_p->CEC_Sem_p);

}/* End of CEC_SetAdditionalAddress() */




/* End of cec.c file */
/* --------------------------------- End of file ---------------------------- */



