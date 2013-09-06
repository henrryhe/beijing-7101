#ifndef __CHCARD_H__
#define __CHCARD_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include          "ChSys.h"

typedef          CHCA_UINT                                                         TCHCADDICardReaderHandle;
/*typedef          ST_DeviceName_t                                                TCHCASysSCRdrHandle;*/
/*typedef          STEVT_Handle_t                                                   TCHCAEventSubscriptionHandle;*/

/*MODE definition*/
#define           T0_MODE                                  0
#define           T1_MODE                                  1

/*select smartcard*/
#define        SMART_SLOT0                   /*card 0*/
/*#define           SMART_SLOT1                       card 1, c2000b machine is for card1*/ 

#ifdef            SMART_SLOT1
#define           MG_SMARTCARD_INDEX           1
#define           MG_SMARTCARD_NUMBER        2
#endif

#ifdef            SMART_SLOT0
#define           MG_SMARTCARD_INDEX           0
#define           MG_SMARTCARD_NUMBER        1
#endif

#define           CHCA_MAX_SC_READER           1

#define           MAX_SCTRANS_BUFFER           /*255 zxg 200608121 change*/256
#define           MAX_APP_NUM                         1


typedef  struct
{
      CHCA_BOOL           RatingValid;
      CHCA_UCHAR         RatingInfo;
      CHCA_BOOL           RatingCheckedValid;
      CHCA_UCHAR        	RatingChecked;  
}TCHCARatingInfo;

typedef   enum
{
      CHCAUnknownCard,
      CHCAMGCard,
      CHCAMultipleCard
}TCHCACardTypeID;


typedef   enum
{
    CHCAUnknowApp,
	CHCAMGApp   	
}TCHCACardAppID;

typedef   struct
{
       CHCA_UCHAR             CA_SN_Number[6];
       CHCA_UCHAR	          CA_Product_Code[2];
}TCHCACardID;

typedef   struct
{
       TCHCACardAppID         AppID;
       CHCA_UCHAR             MajorVersion;
       CHCA_UCHAR             MinorVersion;
}TCHCACardApp;

typedef   struct
{
    TCHCACardTypeID       SCType;
	CHCA_UCHAR            SCAppNum;
	TCHCACardApp          SCAppData[MAX_APP_NUM];
}TCHCACardType;

typedef   struct
{
       TCHCACardID            CardID;       
       TCHCACardType        CardType;
}TCHCACardContent;



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
#define   card_max_number_of_callback      8

typedef struct DDICard_callback_s
{
     boolean                        enabled;
     CALLBACK	                    CardNotifiy_fun;
} DDICard_callback_t;


/*************************************************************************
  *  the Lock data structure of the smart card
  *************************************************************************/
typedef struct
{
    semaphore_t                    *Semaphore;
    STSMART_Handle_t               Owner;
} DDI_CardLock_t;


/*****************************************************************************
the card structure type 
*****************************************************************************/
/*a new typedef  including some information on the opration of the smartcard on 011019 */
typedef struct SCOperationInfo_s
{
     ST_DeviceName_t                    CardDeviceName;                                                         /*the name of the device*/
     STSMART_Handle_t                   SCHandle;                                                /*the handler of the operation*/
     STEVT_Handle_t                     iSCEVTHandle;                                         /*the handler of the sc event */
     ST_ErrorCode_t                     ErrorCode;                                               /*the error code for ST*/
     U32                                NumberRead;                                               /*the actual data length read */
     U32                                NumberWritten;                                           /*the actual data length written by calling the STSMART_TRANSFER() function*/
     U8                                 Write_Smart_Buffer[MAX_SCTRANS_BUFFER];                                 /*the buffer for sending*/
     U8                                 Read_Smart_Buffer[MAX_SCTRANS_BUFFER];                                  /*the buffer for receiving*/
     STSMART_Status_t                   SmartCardStatus;                                         /*status of transmission by calling the STSMART_TRANSFER() function*/
     CH_smard_card_state                SC_Status;
     boolean                            transport_operation_over;
     boolean                            pts_operation_over;                                      /*added for pps on 020514*/
     U8                                 PtsMuxValue;                                             /*added for pps on 020514*/
     U8                                 PtsBytes[50];                                           /*added for pps on 020514*/
     U8                                 PtsTxBuf[6];                                            /*added for pps on 020514*/
     U8                                 PtsRxBuf[6];                                            /*added for pps on 020514*/

     U8                                 FirstT;                                                 /*SupportedProtocolTypes*/
     U32                                BaudRate;                                           /*add this on 030615,current rate*/
     U32                                MaxBaudRate;                                       /*add this on 030615,max rate*/ 
     boolean                            NegotiationMode;                                  /*add this on 030615,true: pps operation,false:no pps operation*/

     boolean                            InUsed; 
     DDICard_callback_t                 ddicard_callbacks[card_max_number_of_callback];
     TCHCADDICardReaderHandle           CardReaderHandle;	 

     CHCA_BOOL                           ExtractEvOk;	 

    /* boolean                               enabled;
     CALLBACK	                              CardNotifiy_fun;*/

     TMGAPICardHandle                   iApiCardHandle;                       /*add this on 040702*/

     semaphore_t                        *ControlLock;                                      /* For exclusive access to private data */	 
     STSMART_Handle_t                   CardLockOwner;	 
     STSMART_Handle_t                   CardExAccessOwner; 	 
     DDI_CardLock_t                     CardLock;                                         /*add this on 040616,Card Lock structure*/	 
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

extern CHCA_DDIStatus CHCA_GetMGCardReaders(TCHCASysSCRdrHandle* lhReaders, CHCA_UCHAR* nReaders);
extern U32  CHCA_CardOpen( TCHCASysSCRdrHandle  hReader );

/*extern TMGDDIStatus CHCA_GetMGCardReaders(ST_DeviceName_t* lhReaders, U8* nReaders);*/
/*extern U32  CHCA_CardOpen( ST_DeviceName_t  hReader );*/
#if 1/*20070514 change*/
extern CHCA_DDIStatus  CHCA_CardClose( TCHCADDICardReaderHandle  hCard);
extern  CHCA_DDIStatus CHCA_CardEmpty( TCHCADDICardReaderHandle  hCard);
extern   CHCA_DDIStatus  CHCA_CardSetExclusive( TCHCADDICardReaderHandle  hCard );
extern CHCA_DDIStatus  CHCA_CardClearExclusive( TCHCADDICardReaderHandle  hCard);
extern  CHCA_DDIStatus  CHCA_CardReset( TCHCADDICardReaderHandle  hCard);
extern CHCA_DDIStatus CHCA_CardSend
	  ( TCHCADDICardReaderHandle  hCard,CHCA_CALLBACK_FN   hCallback,CHCA_UCHAR* pMsg,CHCA_USHORT  Size);
extern   CHCA_DDIStatus CHCA_CardAbort( TCHCADDICardReaderHandle  hCard);
extern void  Ch_CardUnlock( CHCA_UINT  hCard);
extern CHCA_DDIStatus   CHCA_CardLock
	  ( TCHCADDICardReaderHandle  hCard,CHCA_CALLBACK_FN  hCallback,CHCA_ULONG  LockTime)  ;
#else
extern TMGDDIStatus  CHCA_CardClose( U32  hCard);
/*extern TMGDDIEventSubscriptionHandle  CHCA_CardSubscribe(U32  hCard, CALLBACK hCallback);*/
extern TMGDDIStatus CHCA_CardEmpty( U32 hCard);
extern TMGDDIStatus  CHCA_CardSetExclusive( U32  hCard );
extern TMGDDIStatus  CHCA_CardClearExclusive( U32  hCard);
extern TMGDDIStatus  CHCA_CardReset( U32  hCard);
/*extern TMGDDIStatus  CHCA_CardGetCaps( U32 hCard, TMGDDICardCaps* pCaps);*/
/*extern TMGDDIStatus  CHCA_CardGetProtocol( U32  hCard, TMGDDICardProtocol* pProtocol);*/
/*extern TMGDDIStatus  CHCA_CardChangeProtocol( U32 hCard, TMGDDICardProtocol* pProtocol);*/
extern TMGDDIStatus CHCA_CardSend( U32  hCard,CALLBACK  hCallback, U8* pMsg,U16  Size);
extern TMGDDIStatus CHCA_CardAbort( U32  hCard);
extern TMGDDIStatus   CHCA_CardLock( U32  hCard,CALLBACK  hCallback,unsigned long  LockTime);  
extern TMGDDIStatus  CHCA_CardUnlock( U32  hCard);
#endif
extern  CHCA_DDIStatus  CHCA_CardGetProtocol
       ( TCHCADDICardReaderHandle  hCard, TCHCADDICardProtocol* pProtocol);
extern  CHCA_DDIStatus  CHCA_CardChangeProtocol
          ( TCHCADDICardReaderHandle hCard, TCHCADDICardProtocol* pProtocol);


/*extern TCHCAEventSubscriptionHandle  CHCA_CardSubscribe
	 (TCHCADDICardReaderHandle  hCard, CHCA_CALLBACK_FN  hCallback);*/
/*extern  CHCA_BOOL  CHCA_CardSubscribe
	 (TCHCADDICardReaderHandle  hCard, CHCA_CALLBACK_FN  hCallback,TCHCAEventSubscriptionHandle  *SubHandle);*/
extern   CHCA_BOOL  CHCA_CardSubscribe
	 (TCHCADDICardReaderHandle  hCard, CHCA_CALLBACK_FN  hCallback,CHCA_UCHAR  *SubHandle);

	 

extern   CHCA_DDIStatus  CHCA_CardGetCaps
        ( TCHCADDICardReaderHandle    hCard, TCHCADDICardCaps* pCaps);


extern CHCA_BOOL CHMG_GetCardStatus(void);
extern CHCA_BOOL  ChSCTransStart(U32 iCardIndex);
extern void CHMG_CardTransOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex);
extern void CHMG_CardExtractOperation(U32 iCardIndex);
extern void CHMG_CardInsertOperation(U32 iCardIndex);
extern void CHMG_CardResetOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex);

extern void  CHCA_CaDataOperation(void);
extern void  CHCA_CaEmmDataOperation(void); /*add this on 050518*/

extern CHCA_INT   ChParseCaDescriptor(CHCA_UCHAR  *pucSectionData);
extern void CHMG_GetRatingInfo(CHCA_UINT   iCardIndex);
extern void CHMG_GetOPIInfoList(CHCA_UINT   iCardIndex);



extern void CHMG_GetCardContent(CHCA_UINT   iCardIndex);




#endif                                  /* __MGCARDDRV_H__ */






