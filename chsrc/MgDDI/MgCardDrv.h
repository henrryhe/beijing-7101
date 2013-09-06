#ifndef __MGCARDDRV_H__
#define __MGCARDDRV_H__

/*****************************************************************************
  include
 *****************************************************************************/
/*#include "MgGenDrv.h"*/
#include     "..\chcabase\ChCard.h"


#if 0

/*MODE definition*/
#define           T0_MODE                                  0
#define           T1_MODE                                  1

/*select smartcard*/
/*#define        SMART_SLOT0*/                     /*card 0*/
#define           SMART_SLOT1                       /*card 1, c2000b machine is for card1*/ 

#ifdef            SMART_SLOT1
#define           MG_SMARTCARD_INDEX           1
#define           MG_SMARTCARD_NUMBER        2
#endif

#ifdef            SMART_SLOT0
#define           MG_SMARTCARD_INDEX           0
#define           MG_SMARTCARD_NUMBER        1
#endif

#define           MAX_SC_READER                      1

#define           MAX_SCTRANS_BUFFER            256
#define           MAX_APP_NUM                          1

/*typedef   enum
{
     DDICard_SubEventHandle=1,
     DDISource_SubEventHandle	 
}CH_EventSubHandel_t;*/


typedef enum
{
     CH_EXTRACTED,
     CH_INSERTED,                                   /* but not initialized */
     CH_RESETED,                                     /*reset ok*/
     CH_OPERATIONNAL                                /* mg card ready */
}CH_smard_card_state;


/*****************************************************************************
The card  callback type and its operators
*****************************************************************************/
/*#define   card_max_number_of_callback      8*/

/*typedef struct DDICard_callback_s
{
     boolean                        enabled;
     CALLBACK	                    CardNotifiy_fun;
} DDICard_callback_t;*/


/*************************************************************************
  *  the Lock data structure of the smart card
  *************************************************************************/
typedef struct
{
    semaphore_t                     Semaphore;
    STSMART_Handle_t            Owner;
} DDI_CardLock_t;


/*****************************************************************************
the card structure type 
*****************************************************************************/
/*a new typedef  including some information on the opration of the smartcard on 011019 */
typedef struct SCOperationInfo_s
{
     ST_DeviceName_t                 DeviceName;                                                         /*the name of the device*/
     STSMART_Handle_t               SCHandle;                                                /*the handler of the operation*/
     STEVT_Handle_t                    iSCEVTHandle;                                         /*the handler of the sc event */
     ST_ErrorCode_t                    ErrorCode;                                               /*the error code for ST*/
     U32                                      NumberRead;                                               /*the actual data length read */
     U32                                      NumberWritten;                                           /*the actual data length written by calling the STSMART_TRANSFER() function*/
     U8                                        Write_Smart_Buffer[MAX_SCTRANS_BUFFER];                                 /*the buffer for sending*/
     U8                                        Read_Smart_Buffer[MAX_SCTRANS_BUFFER];                                  /*the buffer for receiving*/
     STSMART_Status_t                 SmartCardStatus;                                         /*status of transmission by calling the STSMART_TRANSFER() function*/
     CH_smard_card_state            SC_Status;
     boolean                                 transport_operation_over;
     boolean                                 pts_operation_over;                                      /*added for pps on 020514*/
     U8                                        PtsMuxValue;                                             /*added for pps on 020514*/
     U8                                        PtsBytes[50];                                           /*added for pps on 020514*/
     U8                                        PtsTxBuf[6];                                            /*added for pps on 020514*/
     U8                                        PtsRxBuf[6];                                            /*added for pps on 020514*/


     U8                                        FirstT;                                                 /*SupportedProtocolTypes*/
     U32                                      BaudRate;                                           /*add this on 030615,current rate*/
     U32                                      MaxBaudRate;                                       /*add this on 030615,max rate*/ 
     boolean                                NegotiationMode;                                  /*add this on 030615,true: pps operation,false:no pps operation*/

     boolean                                 InUsed; 
     /*DDICard_callback_t               ddicard_callbacks[card_max_number_of_callback];*/

     boolean                                 enabled;
     CALLBACK	                            CardNotifiy_fun;
	 

     TMGAPICardHandle                iApiCardHandle;                       /*add this on 040702*/


     semaphore_t                         ControlLock;                                      /* For exclusive access to private data */	 
     STSMART_Handle_t                CardLockOwner;	 
     STSMART_Handle_t                CardExAccessOwner; 	 
     DDI_CardLock_t                    CardLock;                                         /*add this on 040616,Card Lock structure*/	 
}SCOperationInfo_t;
/*a new typedef  including some information on the opration of the smartcard on 011019 */
extern SCOperationInfo_t          SCOperationInfo[MAX_APP_NUM];

typedef struct
{
     CH_smard_card_state                SCStatus;
     ST_ErrorCode_t                         iErrorCode;
     boolean                                     bCardReady;
}CardOperation_t;
extern    CardOperation_t                CardOperation;

#endif
#endif                                  /* __MGCARDDRV_H__ */







