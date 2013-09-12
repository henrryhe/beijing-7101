/*******************************************************************************
File name   : cec.h

Description : VOUT driver header file for cec functions

COPYRIGHT (C) STMicroelectronics 2006.

Date               Modification                                     Name
----               ------------                                     ----
4 Dec 2006         Created                                          WF

*******************************************************************************/

#ifndef __CEC_H
#define __CEC_H

/* Includes ----------------------------------------------------------------- */
#include "vout_drv.h"
#include "stddefs.h"
#include "stcommon.h"


#include "stpwm.h"

#include "sttbx.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Constants    ---------------------------------------------------------- */
#define    CEC_MESSAGE_RECEIVED        0x01
#define    CEC_MESSAGE_SENT            0x02
#define    CEC_MESSAGE_NOT_SENT        0x04
#define    CEC_MESSAGE_ERROR           0x08

#define    CEC_MESSAGES_BUFFER_SIZE    16
#define    CEC_MESSAGE_INDEX_MASK      0x0F

/* HDMI CEC Version 1.2 */
#define CEC_STB1                    3
#define CEC_STB2                    6
#define CEC_STB3                    7
#define CEC_RECORDING_DEVICE1       1
#define CEC_RECORDING_DEVICE2       2
#define CEC_RECORDING_DEVICE3       9

/* HDMI CEC Version 1.3 */
#define CEC_TUNER1                  3
#define CEC_TUNER2                  6
#define CEC_TUNER3                  4
#define CEC_TUNER4                 10
#define CEC_PLAYBACK_DEVICE1        4
#define CEC_PLAYBACK_DEVICE2        8
#define CEC_PLAYBACK_DEVICE3       11
#define CEC_FREEUSE                14
#define CEC_UNREGISTRED            15
#define CEC_BROADCAST              15



#ifndef CEC_DEVICE_ROLE
#define CEC_DEVICE_ROLE            STVOUT_CEC_ROLE_TUNER
#endif


#define MAX_LOGIC_ADDRESS_LIST      6

#define DISABLE     FALSE
#define ENABLE      TRUE
/* Private Types ---------------------------------------------------------- */

/* Exported Types--------------------------------------------------------- */

typedef enum CEC_Bit_Type_e
{
    CEC_BIT_START,
    CEC_BIT_ZERO,
    CEC_BIT_ONE,
    CEC_BIT_ERROR
}CEC_Bit_Type_t;

typedef enum CEC_Error_e
{
    CEC_ERROR_BADINIT_FIRSTEDGELOWTOHIGH,
    CEC_ERROR_LOW_PERIOD_UNDEFINED,
    CEC_ERROR_START_THEN_HIGH_PERIOD_ERROR,
    CEC_ERROR_ZERO_THEN_HIGH_PERIOD_ERROR,
    CEC_ERROR_ONE_THEN_HIGH_PERIOD_ERROR,
    CEC_ERROR_START_BIT_MISSING,
    CEC_ERROR_START_BIT_UNEXPECTED,
    CEC_ERROR_CANNOT_TX_DURING_RX,
    CEC_ERROR_TX_INT_AND_NOT_ACK_READ,
    CEC_ERROR_USUAL_ARBITRATION_LOSS,
    CEC_ERROR_COLLISION_DETECTED_AFTER_INITIATOR,
    CEC_ERROR_ACK_LOW_PERIOD_UNDEFINED,
    CEC_ERROR_ACK_WAS_NOT_RECEIVED

}CEC_Error_t;

typedef enum CEC_State_e
{
    CEC_STANDBY,
    CEC_IDLE,
    CEC_TX,
    CEC_RX,
    CEC_DEBUG_TX_RX,
    CEC_TXERROR,
    CEC_RXERROR
}CEC_State_t;

typedef enum CEC_Message_Request_e
{
    CEC_MESSAGE_APPLI_REQUEST,
    CEC_MESSAGE_DRIVER_PING
}CEC_Message_Request_t;

typedef struct CEC_Message_Data_s
{
    U8                      Trials;
    U8                      MAX_Retries;
    CEC_Message_Request_t   Request;
    STVOUT_CECMessage_t     CEC_Message;
}CEC_Message_Data_t;


typedef struct HDMI_CECStruct_s
{
    STVOUT_CECRole_t        Role;   /* Tuner By default */
    /*-------- Messages Buffer --------------*/
    CEC_Message_Data_t      MessagesBuffer[CEC_MESSAGES_BUFFER_SIZE];
    U8                      MesBuf_Read_Index;
    U8                      MesBuf_Write_Index;
    BOOL                    IsBufferEmpty;
    /*------Transmission Params-------------*/
    U8                      TxMessageList[STVOUT_MAX_CEC_MESSAGE_LENGTH];
    U8                      TxMessageLength;
    U8                      TxCurrent_Word_Number;
    U8                      TxCurrent_Bit_Number;
    BOOL                    IslineHigh;
    BOOL                    IsTransmitting;
    U32                     TxInterrupt_Time;
    BOOL                    IsTxWaitingAckBit;
    BOOL                    IsTxAckReceived;
    BOOL                    IsTxAllMsgAcknowledged;
    BOOL                    IsTxAllMsgNotAcknowledged;
    U32                     TxStart_Of_ACK_Bit;


    /*------Reception Params-----------------*/
    BOOL                    IsReception_Interrupt_State_High;
    U32                     Last_Bit_Fall_Time;
    U32                     Last_Bit_Rise_Time;
    BOOL                    IsReceiving;
    BOOL                    IsMsgToAck;/*I'm concerned about the message*/
    BOOL                    IsEndOfMsg;

    BOOL                    IsRxSendingAckBit;
    U8                      RxMessageList[STVOUT_MAX_CEC_MESSAGE_LENGTH];
    U8                      RxMessageLength;
    U8                      RxCurrent_Destination;
    U8                      RxCurrent_Word_Number;
    U8                      RxCurrent_Bit_Number;
    /*-------Logical Address-----------------*/
    BOOL                    IsLogicalAddressAllocated;
    BOOL                    IsValidPhysicalAddressObtained;
    U8                      LogicAddressList[MAX_LOGIC_ADDRESS_LIST]; /* List must end by Unregistred ! ! ! */
    U8                      CurrentLogicalAddressIndex;

    /*------General Params-----------------*/
    STPWM_Handle_t              PwmCompareHandle;
#ifdef STVOUT_CEC_CAPTURE
    STPWM_Handle_t              PwmCaptureHandle;
#endif
    CEC_State_t                 State;
    STPIO_Handle_t              CEC_PIO_Handle;
    STPIO_BitConfig_t           BitConfigure[8];
    STPIO_Compare_t             Compare;
    U8                          Notify;
    semaphore_t*                CECStruct_Access_Sem_p;
    BOOL                        IsBusFree;
    BOOL                        IsBusIdleDelaying;
    BOOL                        IsPreviousUnsuccessful;
    BOOL                        IsFirstPingStarted;
    S8                          FirstLogicAddress;

}HDMI_CECStruct_t;

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t CEC_InterruptsInstall(const HDMI_Handle_t Handle);
ST_ErrorCode_t CEC_InterruptsUnInstall(const HDMI_Handle_t Handle);
ST_ErrorCode_t CEC_TaskStart(const HDMI_Handle_t Handle);
ST_ErrorCode_t CEC_TaskStop(const HDMI_Handle_t Handle);
ST_ErrorCode_t CEC_SendMessage(const HDMI_Handle_t HdmiHandle, const STVOUT_CECMessage_t * const Message_p);
void CEC_PhysicalAddressAvailable(const HDMI_Handle_t HdmiHandle);
void CEC_SetAdditionalAddress( const HDMI_Handle_t HdmiHandle, const STVOUT_CECRole_t Role );
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CEC_H */
