#ifdef   TRACE_CAINFO_ENABLE
#define  CHSC_DRIVER_DEBUG
#endif

/*#define    CARD_DATA_TEST*/
/******************************************************************************
*
* File : ChCard.C
*
* Description : SoftCell driver
*
*
* NOTES :
*
*
* Author : LY
*
* Status :
*
* History : 0.0 2004-6-11  Start coding
*           
* Copyright: Changhong 2004 (c)
* 
*   history  
*   2005-02-21   modification   add a new function 'ChMgCardTest()' for test smartcard
    2006-3       zxg  change for STi7710        
*   2007-5-14    zxg  change for STi7100/STi7101 
*****************************************************************************/
#include    "ChCard.h"
#include   "ChUtility.h"

#if      /*PLATFORM_16 20070514 change*/1/*add this on 050607*/
#include   "stcommon.h"
#endif

#if 1/*051114 xjp add*/
#include  "..\MgKernel\MGAPI.h"
#endif
#ifdef CH_IPANEL_MGCA
#include "..\MGCA_Ipanel\stb_ca_app.h"
#include "..\MGCA_Ipanel\MGCA_Ipanel.h"

#endif

extern ST_Partition_t *SystemPartition;


static  task_t                                                          *ptidSCprocessTaskId;/* 060118 xjp change from *ptidSCprocessTaskId to ptidSCprocessTaskId  for adopt task_init()*/
const int   SC_PROCESS_PRIORITY                         =  8; /*modify from 8 to10 on 060113*//*modify this from 12 to 8 on 050319*/
const int   SC_PROCESS_WORKSPACE                     = (1024*20);
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_SCardStack;
	static tdesc_t g_SCardTaskDesc;
#endif
 
CardOperation_t                                                      CardOperation;
/*20070119 add*/
int CardInserCout=-1;

extern boolean                                                        PairCheckPass;    

boolean                                                                  PairfirstPownOn=false; /*add this on 050509*/

#ifdef   PAIR_CHECK /*add this on 050114*/
CHCA_INT                                                              AntiIndex;     /*add this on 050203*/               
#endif

boolean                                                                  StreamRunning = false;

ST_DeviceName_t                                                   SMARTDeviceName[]={"SC0","SC"};  /* ly add this for smart card on 010614 */
ST_DeviceName_t                                                   EVTDeviceName_SC[]={"EVT0","EVT1"};
ST_DeviceName_t                                                   UARTDeviceName_SC[]  = {"UART0","UART1","UART2","UART3"};  /* ly modify this for smart card on 010614 */


SCOperationInfo_t                                                  SCOperationInfo[MAX_APP_NUM];
CHCA_UINT                                                            MgCardIndex = 0;


TCHCAOperatorInfo                                                OperatorList[CHCA_MAX_NO_OPI];
CHCA_UINT                                                            OPITotalNum = 0;

TCHCACardContent                                                 CaCardContent;  /*card content struct,add it on 041101*/




TCHCARatingInfo                                                      CardRatingInfo; /*add this on 041101*/
semaphore_t                                                            *psemPmtOtherAppParseEnd; 

STBStaus                                                                 App_CA_CurRight;
CHCA_BOOL                                                             CurAppWait = false;

								   
extern CHCA_BOOL                                                   CatReady;
task_status_t                                                             ttStatus;  
CHCA_BOOL                                                              CardContenUpdate = FALSE;
/*extern   semaphore_t                                                 semWaitCatsignal;delete this on 050327*/


semaphore_t                                                            *psemAVSeparate;  /*add this on 041202*/


extern    CHCA_BOOL                                                EMMDataProcessing;
extern    CHCA_BOOL                                                ECMDataProcessing; /*add this on 050116*/
extern    CHCA_BOOL                                                CATDataProcessing; /*add this on 050116*/
extern    CHCA_BOOL                                                TimerDataProcessing; /*add this on 050521*/

extern    CHCA_BOOL                                                EMMDataRightUpdated; /*add this on 050225*/

/*20060402 add judge whether force update smart mirror image such as in menu*/
boolean gb_ForceUpdateSmartMirror=false;
/*************************************************************/

#ifdef    CARD_DATA_TEST
U8                                                   CardSerial[6];
OPIInfoStr                                       OpiInfo[16];
U32                                                 OpiNum;


CHCA_UCHAR                                  pMesss1[5]={0xc1,0xe,0x0,0x0,0x8}; 	  
CHCA_UCHAR                                  pMesss2[6] ={0xc1,0x48,0x0,0x0,0x1,0x10};
CHCA_UCHAR                                  pMesss3[21]={0xc1,0x30,0x0,0x0,0x10,0x1,0x1,0x1,0x2,0x3,0x7,0x17,0xa4,0x1,0x1,0x1,0x2,0x3,0x7,0x17,0xa4};	  
CHCA_UCHAR                                  pMesss4[14]={0xc1,0x30,0x0,0x2,0x9,0x1,0x1,0x1,0x2,0x3,0x7,0x17,0xa4,0x2};
CHCA_UCHAR                                  pMesss5[5]={0xc1,0x16,0x0,0x0,0x6};
CHCA_UCHAR                                  pMesss6[5]={0xc1,0x12,0x1,0x0,0x19};
CHCA_UCHAR                                  pMesss7[5]={0xc1,0x12,0x1,0x0,0x19};
CHCA_UCHAR                                  pMesss8[8]={0xc1,0x34,0x0,0x0,0x3,0x4,0x0,0x0};
CHCA_UCHAR                                  pMesss9[5]={0xc1,0x32,0x0,0x0,0xd};
CHCA_UCHAR                                  pMesss10[8]={0xc1,0x34,0x0,0x0,0x3,0x0,0x0,0x0};
CHCA_UCHAR                                  pMesss11[5]={0xc1,0x32,0x1,0x0,0xa};
CHCA_UCHAR                                  pMesss12[5]={0xc1,0x32,0x0,0x0,0xa};
#endif



/*******************************************************************************
 *Function name: ChSCEvtNotifyFunc
 * 
 *
 *Description: Card event notifiy function   
 *              
 *
 *
 *Prototype:
 *     void  ChSCEvtNotifyFunc
 *       (STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void*EventData)
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *           none
 *     
 *
 *Comments:
 *     process two type card event :
 *                      1.STSMART_EV_CARD_INSERTED
 *                      2.STSMART_EV_CARD_REMOVED
 * 
 * 
 *******************************************************************************/
void  ChSCEvtNotifyFunc
    (STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void*EventData)
{
        CHCA_INT                                          iCardIndex; 


        iCardIndex = MgCardIndex;

		
        switch(Event)
        {
             case STSMART_EV_CARD_INSERTED:
#ifndef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"THE SMART CARD HAS BEEN INSERTED!!!\n");
#endif
                      /*semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);*/
                      SCOperationInfo[iCardIndex].SC_Status = CH_INSERTED; 
                      CardOperation.SCStatus =CH_INSERTED;
					  CardInserCout=-1;/*20070119 add*/
			 /*semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	*/	  
                      break;

             case STSMART_EV_CARD_REMOVED:
#ifndef          CHSC_DRIVER_DEBUG
                      do_report(severity_info,"THE SMART CARD HAS BEEN REMOVED!!!\n");
#endif
                      /*CHCA_CardAbort(SCOperationInfo[iCardIndex].CardReaderHandle);*/
                      /*semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); */
			 SCOperationInfo[iCardIndex].ExtractEvOk = true;		  
                      SCOperationInfo[iCardIndex].SC_Status = CH_EXTRACTED;
                      CardOperation.SCStatus =CH_EXTRACTED;
			
			 /*semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	*/	  
			 break;
        }

	 return;	

}


/*******************************************************************************
 *Function name: ChSCardContentInit
 *           
 *
 *Description: 
 *             init the content of the smart card ,including all the operation info list,and the card serial
 *             
 *    
 *Prototype:
 *            void ChSCardContentInit(CHCA_UINT   iCardIndex)
 * 
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
void ChSCardContentInit(CHCA_UINT   iCardIndex)
{
       CHCA_UINT                       OpiIndex,iAppIndex;  
 
       for(OpiIndex=0;OpiIndex<CHCA_MAX_NO_OPI;OpiIndex++)
       {
               OperatorList[OpiIndex].OPINum = 0;
               OperatorList[OpiIndex].ExpirationDate[0] = 0;
               OperatorList[OpiIndex].ExpirationDate[1] = 0;
               memset(OperatorList[OpiIndex].OPIName,0,16);
               memset(OperatorList[OpiIndex].OffersList,0,8);
               OperatorList[OpiIndex].GeoZone = 0;
 	}

       OPITotalNum = 0;

        /*reset the card serial number as oxff*/
	memset(CaCardContent.CardID.CA_SN_Number,0xff,6);	
	memset(CaCardContent.CardID.CA_Product_Code,0xff,2);	

	for(iAppIndex=0;iAppIndex<MAX_APP_NUM;iAppIndex++)
	{
              CaCardContent.CardType.SCAppData[iAppIndex].AppID = CHCAUnknowApp;
              CaCardContent.CardType.SCAppData[iAppIndex].MajorVersion = 0;
              CaCardContent.CardType.SCAppData[iAppIndex].MinorVersion = 0;
	}
	
	CaCardContent.CardType.SCAppNum = 0;
	CaCardContent.CardType.SCType= CHCAUnknownCard;

       CardRatingInfo.RatingValid = false;
	CardRatingInfo.RatingInfo = 0;
	CardRatingInfo.RatingCheckedValid = false;
	CardRatingInfo.RatingChecked = true;

       return;
}





/*******************************************************************************
 *Function name: CHCA_CaDataOperation
 *           
 *
 *Description: process all MG CA  data(such as:: cat,ecm,emm)
 *             
 *             
 *    
 *Prototype:
 *               void  CHCA_CaDataOperation(void)
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     none
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
void  CHCA_CaDataOperation(void)
{
#ifdef   CHSC_DRIVER_DEBUG
       int             i=0;
#endif
	   
       if(CardOperation.bCardReady)
       {
               while(true)
              {
                     if(ChSCTransStart(MgCardIndex)&&CardOperation.bCardReady)
                     {
                           /*do_report(severity_info,"\n CHMG_CardTransOperation Start! \n");*/
			      /*task_delay(2000);	 add this on 050319*/	
#ifdef   CHSC_DRIVER_DEBUG
			       i = i+1;		   
#endif
                           CHMG_CardTransOperation(CardOperation.iErrorCode,MgCardIndex);
			      	
	  
			      if(CardOperation.iErrorCode!=ST_NO_ERROR)	
				  break;	
		       }
		       else
		       {
		             break;
		       }
	       }
       }


#ifdef   CHSC_DRIVER_DEBUG
	do_report(severity_info,"\n CHMG_CardTransOperation END,TransCount[%d]! \n",i);  
#endif

}


/*******************************************************************************
 *Function name: CHCA_CaEmmDataOperation
 *           
 *
 *Description: process all MG CA EMM  data(emm)
 *             
 *             
 *    
 *Prototype:
 *               void  CHCA_CaEmmDataOperation(void)
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     none
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
void  CHCA_CaEmmDataOperation(void)
{

      /*20060402 add 增加传送EMM数据次数*/
	  int TransferTimes=1;

	 while(TransferTimes>0)/*20060402 add */
	 {
          if(ChSCTransStart(MgCardIndex)&&CardOperation.bCardReady)
          {
            CHMG_CardTransOperation(CardOperation.iErrorCode,MgCardIndex);
#ifdef  CHSC_DRIVER_DEBUG 			
	     do_report(severity_info,"\n CHCA_CaEmmDataOperation END!\n");  
#endif
	     break;/*20060402 add */
          }
	   else
	   {
	   /*
	     do_report(severity_info,"\n>>>>>>>>>>> CHCA_CaEmmDataOperation Error!\n");  
	  */ }
	   TransferTimes--;/*20060402 add */
	  /* task_delay(ST_GetClocksPerSecond()/1000);*//*20060406 change*/
	 }

}



/*******************************************************************************
 *Function name: ChSCTransStart
 *           
 *
 *Description: 
 *              
 *             
 *    
 *Prototype:
 *              CHCA_BOOL  ChSCTransStart(U32 iCardIndex)
 * 
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
CHCA_BOOL ChSCTransStart(U32 iCardIndex)
{
       CHCA_BOOL      CardSendStatus = false;
	   
       if(iCardIndex>=MAX_APP_NUM)
       {
              do_report(severity_info,"\n[ChSCTransStart::]Unkown Card Index\n");
              return(CardSendStatus);
	}

	semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);   
	if(SCOperationInfo[iCardIndex].transport_operation_over)
	{ 
	       SCOperationInfo[iCardIndex].transport_operation_over = false;
		CardSendStatus = true; 
              /*do_report(severity_info,"\n[ChSCTransStart::] Card Sent the data!!!\n");*/
	}	 
	semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	return(CardSendStatus);
}



/*******************************************************************************
 *Function name: CHCA_ResetCatStartPMT
 *           
 *
 *Description: 
 *             
 *    
 *Prototype:
 *     void  CHCA_ResetCatStartPMT(void)
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *
 *Comments:
 *           
 *           
 *           
 *******************************************************************************/
void  CHCA_ResetCatStartPMT(void)
{
           SHORT	 tempsCurProgram=CH_GetsCurProgramId();
           SHORT        tempCurTransponderId=CH_GetsCurTransponderId();		   

           if(!StreamRunning)
           {
                        StreamRunning = true;

			{
			        CHCA_BOOL   SendPmtMessage = true;
                              switch(CH_GetCurApplMode())
                              {
                                    case APP_MODE_STOCK:
                                    case APP_MODE_HTML:
                                    case APP_MODE_GAME:
					   				case APP_MODE_SEARCH:/*20060830 add*/
						   do_report(severity_info,"\n[CHCA_ResetCatStartPMT==>]SHG APP\n");				
                                            SendPmtMessage = false;
                                            CHCA_MepgEnable(false,true);					   	
                                            break;
#ifdef BEIJING_MOSAIC
                                    case APP_MODE_MOSAIC:
						  do_report(severity_info,"\n[CHCA_ResetCatStartPMT==>]M APP\n");				
                                            switch(CH_GetCurMosaicPageType())
                                            {
                                                  case VEDIO_TYPE:
                                                          CHCA_MosaicMepgEnable(true);								  	 
                                                          break;
        										  
                                                  case AUDIO_TYPE:
                                                          CHCA_MosaicMepgEnable(false);	
                                                          break;
        										  
                                                  case OTHER_TYPE:
                                                          SendPmtMessage = false;
                                                          CHCA_MosaicMepgEnable(false);	  
                                                          break;	
                                            }
                                            break;
#endif
                                    default:
						  do_report(severity_info,"\n[CHCA_ResetCatStartPMT==>]OTHER APP\n");				
                                           /*CHCA_MepgEnable(true,true);*/
                                          break;
                               }
			}
						  
 #ifndef CH_IPANEL_MGCA				 
				
                        if((tempsCurProgram != CHCA_INVALID_LINK)&&(tempCurTransponderId!=CHCA_INVALID_LINK))	
#endif							
                        {
                              CHCA_BOOL   SendPmtMessage = true;
              

                               if(ChSendMessage2PMT(tempsCurProgram,tempCurTransponderId,START_MGCA))
                               {
                                    do_report(severity_info,"\n[ChSCardProcess==>]fail to send the 'START_MGCA' message\n");
                               }
#ifdef CH_IPANEL_MGCA

                           CHCA_InsertLastPMTData();
#endif
				   
							   
                        }
           }
}


/*******************************************************************************
 *Function name: ChSCardProcess
 *           
 *
 *Description: 
 *             the process for the smart card
 *             
 *    
 *Prototype:
 *     void ChSCardProcess ( void *pvParam )
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *
 *Comments:
 *    four type status for the smartcard
 *             1) CH_EXTRACTED 
 *             2) CH_INSERTED
 *             3) CH_RESETED
 *             4) CH_OPERATIONNAL
 *           
 *    2005-03-12   delete the noright info displaying process
 *******************************************************************************/
void ChSCardProcess ( void *pvParam )
{
	/*TMGAPICardStatus                        ApiCardStatus;*/
	
    static   int                                     ResetNum=0;	
	clock_t		                            waitTime;
	SHORT	                                   tempsCurProgram; 
	SHORT                                         tempCurTransponderId;
	
	while(true)
	{
		switch(CardOperation.SCStatus)
		{
		case CH_EXTRACTED:
			if(SCOperationInfo[MgCardIndex].ExtractEvOk)
			{   
				
				CardOperation.bCardReady = false;
				
				tempsCurProgram = CH_GetsCurProgramId();
			        tempCurTransponderId = CH_GetsCurTransponderId();					  
						 
				 if(ChSendMessage2PMT(tempsCurProgram,tempCurTransponderId,STOP_MGCA))
				 {
					 do_report(severity_info,"\n[ChSCardProcess==>]fail to send the 'STOP_MGCA' message\n");
				 }
						 
#ifdef ADD_SMART_CONTROL
				 if(CH_GetSmartControlStatus()==true)/*需要卡操作模式*/
#endif		 	
				 {
 #ifndef CH_IPANEL_MGCA				 
					 CHCA_CardSendMess2Usif(NO_CARD);  
#else
                                    CHCA_CardSendMess2Usif(CH_CA2IPANEL_EXTRACT_CARD,NULL,0);
#endif
				 }
						 
					 
				 CHMG_CardExtractOperation(MgCardIndex);	
				if(pstBoxInfo->abiBoxPoweredState==true)
				 {
#ifndef     NAGRA_PRODUCE_VERSION
						 CHCA_PairMepgDisable(true,true);
					 
#else
						 CHCA_MepgDisable(true,true);
					 
#endif
				 }
#ifdef   PAIR_CHECK
						 AntiIndex = -1; 	/*add this on 050203*/			  
#endif
						 StreamRunning = false;
						 PairCheckPass = false; 	
						 CatReady = false;	  
						 CardContenUpdate = FALSE;/*add this on 041125*/ 	  
						 ResetNum = 0;	  

						 Ch_CardUnlock(0);   /*add this on 050510 for test*/	  
			}
#if 1/*20060117 change*/
			task_delay(3000);
#else 
			task_delay( ST_GetClocksPerSecond()/5);
#endif	
			break;
			
		      case CH_INSERTED:
			  	/*do_report(0,"CardInserCout=%d\n",CardInserCout);*/
				  if(CardInserCout==-1)
				  {
					  ResetNum=0;
 #ifndef CH_IPANEL_MGCA				 					  
			        CHCA_CardSendMess2Usif(NORMAL_STATUS); 
#else
                             //CHCA_CardSendMess2Usif(CH_CA2IPANEL_OK_CARD,NULL,0); 
#endif
				  }
				  CardOperation.bCardReady = false;
				  PairfirstPownOn = true;  /*add this on 050509*/
				  if(ResetNum==0)/*modify this from 0 to 3 on 050308*/
				  {
				      CardInserCout=1;
					  CHMG_CardInsertOperation(MgCardIndex);
					  if(CardOperation.SCStatus!=CH_RESETED) 
					  {
 #ifndef CH_IPANEL_MGCA				 					  
				            CHCA_CardSendMess2Usif(ERROR_CARD); 
 #else
                                        CHCA_CardSendMess2Usif(CH_CA2IPANEL_BAD_CARD,NULL,0);    
 #endif
						  ResetNum++;
					  }
					 

				  }
#if 1/*20070117 change short delay for quick inserr and remove*/
				  task_delay(3000);
#else 
				  task_delay( ST_GetClocksPerSecond()/5);
#endif		

				  break;
				  
			  case CH_RESETED:
				  if(!CardOperation.bCardReady && (ResetNum==0))
				  {
					  /*char           ReleaseVersion[16];  add this on 050311 just for test*/
					  
					  CHMG_CardResetOperation(CardOperation.iErrorCode,MgCardIndex);
					  
					  if(CardOperation.bCardReady)
					  {
					 
						  CHCA_InitMgEmmQueue();/*add this on 050121, init the emm queue*/
						  CHMG_GetOPIInfoList(MgCardIndex);  /*add this on 041027*/ 
						 
					  }
					  ResetNum++;   
				  }	
#if 1/*20060117 change*/
				  task_delay(3000);
#else 
				  task_delay( ST_GetClocksPerSecond()/5);
#endif	
				  break;
				  
			  case CH_OPERATIONNAL:

				  if(CardOperation.bCardReady )
				  {
					  CHCA_ResetCatStartPMT();  /*add this on 050327*/
					 				  		
				  }	
#if 0
				  task_delay(3000);
#else 
				  task_delay( ST_GetClocksPerSecond()/5);
#endif
				  break;
				}
				
      }
	  
}



/*******************************************************************************
 *Function name: ChCheckExpiration
 * 
 *
 *Description: Check the expiration date
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  ChCheckExpiration (CHCA_USHORT   Open_zone_check)
 *
 *input:
 *      CHCA_USHORT   Open_zone_check:           opi index
 *
 *output:
 *
 *Return Value:
 *       true:  the data is expiration
 *       false: not expiration
 *
 *Comments:
 * 
 *******************************************************************************/
CHCA_BOOL  ChCheckExpiration (CHCA_INT  DateCurrent,CHCA_USHORT   Open_zone_check)
{
       CHCA_INT                   /*DateCurrent,*/DateExpiration;
       CHCA_BOOL                Expirated = false;


/*需添加OPI Index 的判断，否则有可能浴出*/
	if(Open_zone_check<OPITotalNum)
	{

	#if 1/*050705 change for date compare*/
	       DateExpiration = (OperatorList[Open_zone_check].ExpirationDate[1]<<8)|(OperatorList[Open_zone_check].ExpirationDate[0]);

	#else
		DateExpiration = (OperatorList[Open_zone_check].ExpirationDate[0]<<8)|(OperatorList[Open_zone_check].ExpirationDate[1]);
	#endif
	#ifdef    CHSC_DRIVER_DEBUG
	       do_report(severity_info,"\n[ChCheckExpiration] CurrentData[%d] DataExpiration[%d]\n",DateCurrent,DateExpiration);
	#endif
		
		if( DateExpiration < DateCurrent)
	           Expirated = true;
	}
	return (Expirated);

}


/*******************************************************************************
 *Function name: ChSubOfferCheck
 * 
 *
 *Description: Check the offer map of the opi have the right or not
 *                  
 *
 *Prototype:
 *     CHCA_BOOL ChSubOfferCheck(CHCA_UCHAR *pucSectionData,CHCA_USHORT Pz)
 *
 *input:
 *      CHCA_UCHAR *pucSectionData  : offer map
 *      CHCA_UCHAR Pz :                      opi index
 *
 *output:
 *
 *Return Value:
 *       true: the offer with the right
 *       false: the offer without the right
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL ChSubOfferCheck(CHCA_UCHAR *pucSectionData,CHCA_USHORT Pz)
{
       CHCA_BOOL       OfferRightOk=false;
       CHCA_USHORT   i = 0 ;

	if(Pz<OPITotalNum)
	{
	        while ( i < 8 )
	        {
	               if ( ( pucSectionData[ i ] ) & ( OperatorList[Pz].OffersList[i]) )
	               {
	                     OfferRightOk = true;
	                     break;
	               }
	               i++;
	        }
	}
		
  /*      if ( i == 8 )
        {
               OfferRightOk = false;
               return ( OfferRightOk ) ;
        }
	*/	
        return ( OfferRightOk );
}


/*******************************************************************************
 *Function name: ChCrossCheckOPI
 * 
 *
 *Description: return the index of the opi to be checked 
 *                  
 *
 *Prototype:
 *     CHCA_USHORT  ChCrossCheckOPI ( CHCA_USHORT iOPI_to_check)
 *
 *input:
 *      the operator numbere to be checked
 * 
 *
 *output:
 *
 *Return Value:
 *      the opi index of the operator 
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_USHORT  ChCrossCheckOPI ( CHCA_USHORT iOPI_to_check)
{
    CHCA_USHORT    iOpiIndex;

    for ( iOpiIndex = 0; iOpiIndex < CHCA_MAX_NO_OPI; iOpiIndex++ )
    {
        if ( OperatorList[iOpiIndex].OPINum == iOPI_to_check )
            break;
    }

    return iOpiIndex;
}





/*******************************************************************************
 *Function name: ChCaParseCaDescriptor
 * 
 *
 *Description: Parse the ca descriptor
 *                  
 *
 *Prototype:
 *     CHCA_INT   ChCaParseCaDescriptor(CHCA_UCHAR  *pucSectionData)
 *
 *input:
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *
 *Comments:
 *    2005-03-17          compare the current sys data with the data of the card.    
 * 
 *******************************************************************************/
CHCA_INT   ChParseCaDescriptor(CHCA_UCHAR  *pucSectionData)
{
      int                                      iDescriptorLength;
      int                                      iNoOfBytesParsed;
      SHORT2BYTE                       stPid_temp;
      UINT                                   iTempforCA;
      int                                       iTemp;
      UINT                                    iOPI_from_Pmt;
      int                                        iExpiration_date_from_pmt;
      CHCA_TDTDATE                   current_sys_date; /*add this on 050317*/
	  

      CHCA_INT                            NoRight = 1;
      CHCA_UINT                          TempOPIIndex=CHCA_MAX_NO_OPI;
		
      iNoOfBytesParsed = 0;
      iDescriptorLength = pucSectionData [ 1 ];
        
      pucSectionData += 2;
        
      iTempforCA = ( pucSectionData [ 0 ] << 8 ) | pucSectionData [ 1 ];  /* get the CA_system_id */

      /* Judge if the CA_system_id is CANAL+  */
      if (iTempforCA != CANAL_CA_SYSTEM_ID )
      {
              do_report(severity_info,"There is no the CANAL_CA_SYSTEM_ID in the descriptor\n");
              return NoRight;
      }

      pucSectionData += 2;
      iNoOfBytesParsed += 2;

      while ( iNoOfBytesParsed < iDescriptorLength )  /* begin to parse the private data loop */
      {
              /*For the CA_Descriptor has no OPI in the info_loop */
              
              stPid_temp . byte . ucByte1  = pucSectionData [ 0 ] & 0x1F;
              stPid_temp . byte . ucByte0  = pucSectionData [ 1 ];

              if ( ( iNoOfBytesParsed + 2 ) == iDescriptorLength )
              {
                     do_report(severity_info,"ECM_PID%x!!!\n",stPid_temp . sWord16);
                     return NoRight;
              }

              iOPI_from_Pmt = ( pucSectionData [ 2 ]<<8 ) | pucSectionData [ 3 ];
              if ( iOPI_from_Pmt != CHCA_MG_INVALID_OPI )
              {
              	TempOPIIndex=ChCrossCheckOPI( iOPI_from_Pmt);
                    if (TempOPIIndex>= 16 )
                    {
                            pucSectionData += 15;
                            iNoOfBytesParsed +=15;
                            continue;
                    }
              }
		else
		{
			 pucSectionData += 15;
                      iNoOfBytesParsed +=15;
                      continue;
		}

              if ( ( pucSectionData [ 4 ] & 0x80 ) != 0x80 )
              {
                       /* For subscribing_mode_flag == ppv service */
                       do_report(severity_info,"Found a ppv service in Parse_info_loop\n");
#if 1                /*laiyan add it for ecm test on 020202*/
                       pucSectionData += 15;
                       iNoOfBytesParsed +=15;
                       continue;
#endif
              }
              else
              {
                       /*For subscribing_offer  */
#if   0
                       do_report(severity_info,"\n");

		         for ( iTemp = 0 ;iTemp < 8 ; iTemp++)
                       {
                            do_report(severity_info,"%3x ",pucSectionData[iTemp+5]);
                       }
                       do_report(severity_info,"\n");
#endif
            
		        if ( !ChSubOfferCheck( pucSectionData + 5, TempOPIIndex/*ChCrossCheckOPI(iOPI_from_Pmt) */) )
                      {
                           do_report(severity_info,"No subscribing offer.\n");
                           pucSectionData += 15;
                           iNoOfBytesParsed +=15;
                           continue;
                      }

                      /* For expiration data check */
#if 1 /*050705 change for compare date*/	

			CHCA_GetCurrentDate(&current_sys_date);
                     iExpiration_date_from_pmt = ((((current_sys_date.ucMonth & 0x7) <<5) | (current_sys_date.ucDay & 0x1f)))|((((current_sys_date.sYear - 1990)&0x7f)<<9 )|(((current_sys_date.ucMonth>>3) & 0x1) <<8 ));

#else /*the date is the current system date*/
                      CHCA_GetCurrentDate(&current_sys_date);
	               iExpiration_date_from_pmt = ((((current_sys_date.ucMonth & 0x7) << 5) | (current_sys_date.ucDay & 0x1f))<<8)|((((current_sys_date.sYear - 1990)&0x7f)<<1 )|((current_sys_date.ucMonth & 0x8) >> 3 ));
#endif				   
					  
                      if ( !ChCheckExpiration( iExpiration_date_from_pmt, TempOPIIndex/*ChCrossCheckOPI(iOPI_from_Pmt)*/) )
                      {
                           NoRight = 0;
                           do_report(severity_info,"OK,You have got rights\n");
			      return(NoRight);/*add this on 050222,if find a right,then return*/			   
                      }
                      else
                      {
                           do_report(severity_info,"Obsolescence rights  %x\n",iExpiration_date_from_pmt);
                           pucSectionData += 15;
                           iNoOfBytesParsed += 15;
                           continue;
                      }
        }

        pucSectionData += 15;
        iNoOfBytesParsed += 15;
    }

    return (NoRight);

}


/*******************************************************************************
 *Function name: CHMG_CaContentUpdate
 *           This function is used to force the update of the smart card mirror image of the mg kernel 
 *
 *Description:
 *           void CHMG_CaContentUpdate(void)
 *               
 *
 *Prototype:
 *      
 *
 *
 *input:
 *           
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *      none
 *
 *
 *Comments:
 *    2005-02-25       modify    add 
 *    2005-03-11       modify    updated new emm
 *******************************************************************************/
/*extern CHCA_USHORT                                                          CatFilterIndex;*/
/*extern  CHCA_BOOL                                                              AlreadyReadCat;*/
/*extern  void   CHMG_CatTemp(void);delete this on 050311*/
/*extern  void   CHMG_EcmTemp(void);*/
/*extern  void  CHCA_CardResetTemp( void);just for test*/
/*extern  BOOL  EmmCardUpdated;*/
/*add this on 050311*/

void CHMG_CaContentUpdate(void)
{
        SHORT           tempsCurProgram;
        SHORT           tempCurTransponderId;		   

        if(CardContenUpdate) 
	 {
	        /*do_report(severity_info,"\n[CHMG_CaContentUpdate==>] Emm update the mirror image of the card\n");*/
	        CardContenUpdate = false;
	        if(!CurAppWait)
	        {
	    	       /*CHMG_GetCardContent(0);delete this on 050128*/
                     tempsCurProgram=CH_GetsCurProgramId();
                     tempCurTransponderId=CH_GetsCurTransponderId();	
			if(!CurAppWait)
			{
                           if(ChSendMessage2PMT(tempsCurProgram,tempCurTransponderId,START_PMT_UPDATE))
                           {
                                 do_report(severity_info,"\n[CHMG_CaContentUpdate==>] fail to send the 'Stop PMT' message\n");
                           }
			      /*else
			      {
                                 do_report(severity_info,"\n[CHMG_CaContentUpdate==>] success to send the 'Stop PMT' message\n");
			      }*/
			}
		 }
#ifdef CH_IPANEL_MGCA

                           CHCA_InsertLastPMTData();
#endif
			
         }	  
	   	
         return;
}

/*add this on 050311*/



/*******************************************************************************
 *Function name: CHMG_CardExtractOperation
 *           
 *
 *Description: 
 *             the extract process for the smart card
 *             
 *    
 *Prototype:
 *     void CHMG_CardExtractOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex)
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *
 *Comments:
 *           
 *******************************************************************************/
void CHMG_CardExtractOperation(U32 iCardIndex)
{
      TMGDDICardReaderHandle                 iDDICardReaderHandle;
	  TMGAPICardHandle                       hApiCardlHandle;   
	  TMGAPIStatus                           StatusMgApi1;	
	   /*BitField32                                         bCardEvMask; */
	   CHCA_CALLBACK_FN	                       CardNotifiyfun = NULL;
      
      
	  if(iCardIndex >= MAX_APP_NUM)
	  {
         do_report(severity_info,"\n[CHMG_CardExtractOperation==>] Unknow Card Index\n");
		 return;	   	
	  }

         semaphore_wait(pSemMgApiAccess); 
		
         semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
	  SCOperationInfo[iCardIndex].ExtractEvOk = false; 	 
         hApiCardlHandle = SCOperationInfo[iCardIndex].iApiCardHandle;
	  SCOperationInfo[iCardIndex].iApiCardHandle = NULL;	

	  iDDICardReaderHandle = (TMGDDICardReaderHandle)SCOperationInfo[iCardIndex].CardReaderHandle;
         if(SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardExtract].enabled)
         {
                CardNotifiyfun = SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardExtract].CardNotifiy_fun;
	  }
	  semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);


	  if(hApiCardlHandle==NULL)
	  {
                 /*do_report(severity_info,"\n[CHMG_CardExtractOperation==>] The Card is not Open\n");*/
	          semaphore_signal(pSemMgApiAccess);     			 
		   return;	   	
	  }

         
         StatusMgApi1 = MGAPICardClose(hApiCardlHandle);
 	  switch(StatusMgApi1)
 	  {
                   case MGAPIOk:
 				/*do_report(severity_info,"\n[MGAPICardClose==>] Execution OK\n");	*/	 	
 		              break;				 	                       
                   case MGAPINotFound:
 				do_report(severity_info,"\n[MGAPICardClose==>] Unknown instance handle\n");		 	
 		              break;				 	                       
                   case MGAPIInvalidParam:
 				do_report(severity_info,"\n[MGAPICardClose==>]Parameter null,aberrant or wrongly formatted\n");		 	
 		              break;				 	                       
                   case MGAPISysError:
 				do_report(severity_info,"\n[MGAPICardClose==>] Instance destroyed,but with an indeterminate status.\n");		 	
 		              break;				 	                       
          }
		
	   if(CardNotifiyfun != NULL)
	       CardNotifiyfun((HANDLE)iDDICardReaderHandle,(byte)MGDDIEvCardExtract,NULL,NULL);	   	
		
	   semaphore_signal(pSemMgApiAccess);
		
          return; 
		 
}


/*******************************************************************************
 *Function name: CHMG_CardInsertOperation
 *           
 *
 *Description: 
 *             the insert process for the smart card
 *             
 *    
 *Prototype:
 *     void CHMG_CardInsertOperation(U32 iCardIndex)
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *
 *Comments:
 *           
 *******************************************************************************/
void CHMG_CardInsertOperation(U32 iCardIndex)
{
         TMGDDICardReaderHandle                iDDICardReaderHandle;
         /*int                                                    N;*/
	  TMGAPICardHandle                           hApiCardHandle/*,hApiCardlHandle1[2]*/;   
	  TMGAPIStatus                                   StatusMgApi;	
	  BitField32                                         bCardEvMask;
	  CHCA_CALLBACK_FN	                      CardNotifiyfun = NULL;


 	   if(iCardIndex >= MAX_APP_NUM)
	   {
               do_report(severity_info,"\n[ChCardInsertProcess..] Unknow Card Index\n");
		 return;	   	
	   }	  

          semaphore_wait(pSemMgApiAccess);  

	   /*init the opi info list,add this function on 041027*/	  
          /*ChSCInitOPIInfoList(iCardIndex); */
	   ChSCardContentInit(iCardIndex);
		  
          if(SCOperationInfo[iCardIndex].iApiCardHandle==NULL)
          {
                  if(MGAPICardIsRunning())
                  {
#ifdef              CHSC_DRIVER_DEBUG
                        do_report(severity_info,"\n[CHMG_CardInsertOperation==>] The Card Module has been initialized\n");       
#endif
                        hApiCardHandle = MGAPICardOpen(CHMG_CardCallback); 

#ifdef              CHSC_DRIVER_DEBUG		 
                        do_report(severity_info,"\n[MgCard_init::]  hApiCardHandle[%d]\n",hApiCardHandle);
#endif      

                       if(hApiCardHandle!=NULL)
                      {
			        StatusMgApi = MGAPICardExist(hApiCardHandle);  

			        if(StatusMgApi == MGAPIOk)
			       {
#ifdef                          CHSC_DRIVER_DEBUG		 
                                    do_report(severity_info,"\n[CHMG_CardInsertOperation==>]  MGAPICardExist  MGAPIOk\n");
#endif  
			        }
			        else
			        {
#ifdef                           CHSC_DRIVER_DEBUG		 
                                     do_report(severity_info,"\n[CHMG_CardInsertOperation==>]  MGAPICard not Exist  MGAPINotFound\n");
#endif  
			         }
		 
                              /*????*/
				  semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
                              SCOperationInfo[iCardIndex].iApiCardHandle = hApiCardHandle;
				  semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
           			  
			         /*????*/		  
					 
                              /*set the Cardl event mask*/
                              bCardEvMask = MGAPICardEvCloseMask|\
                                                      MGAPICardEvExtractMask|\
                                                      MGAPICardEvBadCardMask|\
                                                      MGAPICardEvReadyMask|\
                                                      MGAPICardEvContentMask|\
                                                      MGAPICardEvAddFuncDataMask|\
                                                      MGAPICardEvRatingMask|\
                                                      MGAPICardEvUpdateMask;   /*add this update mask on 041027*/

	                       StatusMgApi = MGAPICardSetEventMask(hApiCardHandle,bCardEvMask);
                              if(StatusMgApi != MGAPIOk)
                              {
#ifdef                           CHSC_DRIVER_DEBUG		 
                                     do_report(severity_info,"\n[CHMG_CardInsertOperation==>]  CardSetEventMask Status[%d]\n",StatusMgApi);
#endif      
                                      semaphore_signal(pSemMgApiAccess);  
                                      return;
	                        }
			          else
			          {
#ifdef                            CHSC_DRIVER_DEBUG		 
                                      do_report(severity_info,"\n[CHMG_CardInsertOperation==>]  CardSetEventMask[%d] OK\n",bCardEvMask);
#endif      
			          }
					  
		            }
		            else
		           {
#ifdef                      CHSC_DRIVER_DEBUG
                                do_report(severity_info,"\n[CHMG_CardInsertOperation==>] API Card Open fail for the maximum number of instances has been reached\n");       
#endif
                                semaphore_signal(pSemMgApiAccess);  
                                return; 
                          }

	         }
	         else
	         {
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n[CHMG_CardInsertOperation==>] The Card Module has not been initialized\n");       
#endif
                      semaphore_signal(pSemMgApiAccess);  
                      return;
	         }
        }

        semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); 
        iDDICardReaderHandle = (TMGDDICardReaderHandle)SCOperationInfo[iCardIndex].CardReaderHandle;

        if(SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardInsert].enabled)
        {
                CardNotifiyfun = SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardInsert].CardNotifiy_fun;
	 }
	 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);    


        if(CardNotifiyfun != NULL)
             CardNotifiyfun((HANDLE)iDDICardReaderHandle,(byte)MGDDIEvCardInsert,NULL,NULL);	   
  
        semaphore_signal(pSemMgApiAccess);  

       return; 

}


/*******************************************************************************
 *Function name: CHMG_CardResetOperation
 *           
 *
 *Description: 
 *             the reset process for the smart card
 *             
 *    
 *Prototype:
 *     void CHMG_CardResetOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex)
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/


void CHMG_CardResetOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex)
{  
      TMGDDIEvCardReportExCode            StatusMgDdi;
      TMGDDICardATR                              pATR;
      TMGDDICardReaderHandle                iDDICardReaderHandle;
      TMGDDIEventCode                            EventCode; 
      /*TMGAPICardStatus                            iMGAPICardStatus;
      TMGAPIStatus                                   iAPIStatus;   */
      CHCA_CALLBACK_FN	                   CardNotifiyfun = NULL;
      U8                                                   CardSendBuffer[MAX_SCTRANS_BUFFER]; 
      	  
      if(iCardIndex >= MAX_APP_NUM)
      {
             do_report(severity_info,"\n[CHMG_CardResetOperation==>] Unknow Card Index\n");
	      return;	   	
       }
	  
       semaphore_wait(pSemMgApiAccess); 
	  
       semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); 
       iDDICardReaderHandle = (TMGDDICardReaderHandle)SCOperationInfo[iCardIndex].CardReaderHandle;

        if(SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardResetRequest].enabled)  
           CardNotifiyfun = SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardResetRequest].CardNotifiy_fun;
        semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 	  
   
        if(CardNotifiyfun != NULL)
        {
#ifdef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"THE SMART CARD[%d] Reset Request!!!\n",iDDICardReaderHandle);
#endif
               CardNotifiyfun((HANDLE)iDDICardReaderHandle,(byte)MGDDIEvCardResetRequest,NULL,NULL);
       }

       semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); 
       if(SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardReset].enabled)  
           CardNotifiyfun = SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardReset].CardNotifiy_fun;
   
       semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 	  

      if(ErrorCode == ST_NO_ERROR)
      {
#ifdef    CHSC_DRIVER_DEBUG      
              U32                      DataIndex;
#endif
			  
              StatusMgDdi = MGDDICardOk;/*MGDDICardError;*/
			  
              semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); 
              if(SCOperationInfo[iCardIndex].NumberRead>MAX_SCTRANS_BUFFER)
		{
                    pATR.Size = MAX_SCTRANS_BUFFER;
		}
		else		 
              {
                    pATR.Size = SCOperationInfo[iCardIndex].NumberRead;
		}
				  
		memcpy(CardSendBuffer,SCOperationInfo[iCardIndex].Read_Smart_Buffer,pATR.Size);			   
              semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 

              pATR.pData = CardSendBuffer;

		pATR.bATR = MGDDICardATR;  /*MGDDICardHistoric*/
		
		EventCode =  MGDDIEvCardReset;  

#ifdef    CHSC_DRIVER_DEBUG 		
              do_report(severity_info,"\n[ChCardResetProcess::]\n");
		do_report(severity_info,"\n ATR num[%d]",pATR.Size);
		 for(DataIndex = 0; DataIndex < pATR.Size; DataIndex++)
			do_report(severity_info,"%4x",*(pATR.pData+DataIndex));	
		 do_report(severity_info,"\n");
#endif		 

              task_delay(1000);/*  *//*20060207 change 1000 to ST_GetClocksPerSecond()/5*//*2000 for test on 050510*/
		    /*  task_delay( ST_GetClocksPerSecond()*2);*/
#ifdef    CARD_DATA_TEST
              CHCA_CardSendTest(pMesss1,5);
		task_delay(4000);
            
              CHCA_CardSendTest(pMesss2,6);
		task_delay(4000);

             CHCA_CardSendTest(pMesss3,21);
		task_delay(4000);

              CHCA_CardSendTest(pMesss4,14);
		task_delay(4000);

              CHCA_CardSendTest(pMesss5,5);
		task_delay(4000);

              CHCA_CardSendTest(pMesss6,5);
		task_delay(4000);

              CHCA_CardSendTest(pMesss7,5);
		task_delay(4000);

              CHCA_CardSendTest(pMesss8,8);
		task_delay(4000);

              CHCA_CardSendTest(pMesss9,5);
		task_delay(4000);

              CHCA_CardSendTest(pMesss10,8);
		task_delay(4000);

              CHCA_CardSendTest(pMesss11,5);
		task_delay(4000);

              CHCA_CardSendTest(pMesss12,5);
#endif

               if(CardNotifiyfun != NULL)
               {
                     CardNotifiyfun( (HANDLE)iDDICardReaderHandle,MGDDIEvCardReset,(word)StatusMgDdi,(dword)&pATR);
#if 0
			task_status(ptidSCprocessTaskId,&ttStatus,task_status_flags_stack_used);
                     do_report(severity_info,"\n[CHMG_CardResetOperation==>] stack used = %d\n",ttStatus.task_stack_used);		  
#endif
		}
			   

		 /*add this on 040816*/


               while(true)
               {
				   if(ChSCTransStart(iCardIndex))
				   { 
					   if(SCOperationInfo[iCardIndex].SC_Status == CH_RESETED)
					   {
						   /*task_delay(1000); add this on 050510*/
						   CHMG_CardTransOperation(CardOperation.iErrorCode,iCardIndex);			   
					   }
					   else
					   {
						   break;  
					   }
				   }
				   else
				   {
					   if(SCOperationInfo[iCardIndex].SC_Status == CH_RESETED)
					   {
						   if(MGAPIMGCardIsReady())
						   {
							   CardOperation.bCardReady = true;
							   /*do_report(severity_info,"\n[CHMG_CardResetOperation==>]The mg card is already ready\n");*/	 
							   
							   CardOperation.SCStatus = CH_OPERATIONNAL;
							   SCOperationInfo[iCardIndex].SC_Status = CH_OPERATIONNAL;
							   /*task_delay(1000);	add this on 050515*/
 #ifndef CH_IPANEL_MGCA				 					  							   
							   CHCA_CardSendMess2Usif(NORMAL_STATUS); 	
 #else
							   CHCA_CardSendMess2Usif(CH_CA2IPANEL_OK_CARD,NULL,0); 	

 #endif
							   
#ifdef CHSC_DRIVER_DEBUG
							   do_report(severity_info,"\n  Smart Card Reset Successfully! \n");
#endif
						   }
						   else
						   {
 #ifndef CH_IPANEL_MGCA				 					  							   						   
							   CHCA_CardSendMess2Usif(ERROR_CARD); 
 #else
							   CHCA_CardSendMess2Usif(CH_CA2IPANEL_BAD_CARD,NULL,0); 	

 #endif
							   
#ifdef CHSC_DRIVER_DEBUG
							   do_report(severity_info,"\n  Smart Card Reset Unsuccessfully! \n");
#endif
						   }
					   }				
					   else
					   {
 #ifndef CH_IPANEL_MGCA				 					  							   						   					   
						   CHCA_CardSendMess2Usif(ERROR_CARD); 
#else
						   CHCA_CardSendMess2Usif(CH_CA2IPANEL_BAD_CARD,NULL,0); 	

#endif
					   }					
					   break;
				   }
			   }
		 /*add this on 040816*/
	}
	else
	{
#ifdef    	 CHSC_DRIVER_DEBUG	 
               do_report(severity_info,"\n<<  STSMART_EV_CARD_RESET[%d] Fail ,Event err code[%d]>>  \n",iDDICardReaderHandle,ErrorCode);				
#endif
               StatusMgDdi = MGDDICardError;
               if(CardNotifiyfun!=NULL)
               {
                     CardNotifiyfun((HANDLE)iDDICardReaderHandle,MGDDIEvCardReset,(word)StatusMgDdi,NULL);	   
		 }
 #ifndef CH_IPANEL_MGCA				 					  							   						   					   
		 CHCA_CardSendMess2Usif(ERROR_CARD); 	   
 #else
             CHCA_CardSendMess2Usif(CH_CA2IPANEL_BAD_CARD,NULL,0); 	
 #endif
	}

       semaphore_signal(pSemMgApiAccess); 

       return;

}


/*******************************************************************************
 *Function name: CHMG_CardTransOperation
 *           
 *
 *Description: 
 *             the transmit process for the smart card
 *             
 *    
 *Prototype:
 *     void CHMG_CardTransOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex)
 * 
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
void CHMG_CardTransOperation(ST_ErrorCode_t  ErrorCode,U32 iCardIndex)
{
       TMGDDIEvCardReportExCode            StatusMgDdi;
       TMGDDICardReport                           pTData;
       TMGDDICardReaderHandle                iDDICardReaderHandle;
	U8                                                    CardSendBuffer[MAX_SCTRANS_BUFFER+2]; 
	CHCA_UINT                                      DataIndex;
   
       /*TMGAPIStatus                                   iAPIStatus;  */
	/*TMGAPICardStatus                            iMGAPICardStatus;*/
	CHCA_CALLBACK_FN	                    CardNotifiyfun = NULL;
 	
       if(iCardIndex >= MAX_APP_NUM)
       {
             do_report(severity_info,"\n[ChCardTransProcess..] Unknow Card Index\n");
	      return;	   	
       }	

       semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
       iDDICardReaderHandle = (TMGDDICardReaderHandle)SCOperationInfo[iCardIndex].CardReaderHandle;

       if(SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardReport].enabled)  
	    CardNotifiyfun = SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardReport].CardNotifiy_fun;
       semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

       if(ErrorCode != ST_NO_ERROR) 
           do_report(severity_info,"\n[CHMG_CardTransOperation==>] ErrCode[%d]\n",ErrorCode);
	   
       if(ErrorCode==ST_NO_ERROR)
       {
#ifdef    CHSC_DRIVER_DEBUG       
              CHCA_UINT                      DataIndex;
#endif
			  
              StatusMgDdi = MGDDICardOk;

              semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
		if(SCOperationInfo[iCardIndex].NumberRead>MAX_SCTRANS_BUFFER)
		{
                    pTData.Size = MAX_SCTRANS_BUFFER;
		}
		else
		{
                   pTData.Size = SCOperationInfo[iCardIndex].NumberRead;
		}

              memset(CardSendBuffer,0,MAX_SCTRANS_BUFFER+2/*258 20060723 change*/);		
		memcpy(CardSendBuffer,SCOperationInfo[iCardIndex].Read_Smart_Buffer,pTData.Size);	  
              CardSendBuffer[pTData.Size]= SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0];	  
	       CardSendBuffer[pTData.Size+1]= SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1];	  
              semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

              pTData.Size = pTData.Size + 2; 
	       pTData.bLocked = false;
              pTData.pData = &CardSendBuffer[0];

#ifdef    CHSC_DRIVER_DEBUG	 
              do_report(severity_info,"\n[ChCardTransProcess::]\n");
              for(DataIndex = 0; DataIndex<pTData.Size; DataIndex++)
		     do_report(severity_info,"%4x",*(pTData.pData+DataIndex));	
		 do_report(severity_info,"\n");		   
#endif	
	        if(CardNotifiyfun != NULL)
	        {  
			  	
			  CardNotifiyfun((HANDLE)iDDICardReaderHandle,MGDDIEvCardReport,(word)StatusMgDdi,(dword)&pTData);	   

		 	 }
	}
	else
	{
	      if(ErrorCode == ST_ERROR_INVALID_HANDLE)
	      {
                  StatusMgDdi = MGDDICardProtocol;
#ifdef    	    CHSC_DRIVER_DEBUG	 
                  do_report(severity_info,"\n<< [ChCardTransProcess::] MGDDICardProtocol error  \n");				
#endif
	      	}
	      else	  	
             {
                  StatusMgDdi = MGDDICardError;
#ifdef    	    CHSC_DRIVER_DEBUG	 
                  do_report(severity_info,"\n<< [ChCardTransProcess::] MGDDICardError\n");				
#endif
	      }

		  
             if(CardNotifiyfun != NULL)
             {
                   CardNotifiyfun((HANDLE)iDDICardReaderHandle,MGDDIEvCardReport,(word)StatusMgDdi,NULL);
                   /*task_status(ptidSCprocessTaskId,&ttStatus,task_status_flags_stack_used);
                   do_report(severity_info,"\n[CHMG_CardTransOperation==>] stack used = %d\n",ttStatus.task_stack_used);		  */
             	} 	     
	}
 
       return;
}






/*******************************************************************************
 *Function name: CHMG_GetRatingInfo
 *           
 *
 *Description: 
 *             get the rating info
 *             
 *    
 *Prototype:
 *            void CHMG_GetRatingInfo(CHCA_UINT   iCardIndex)
 * 
 *           
 *
 *
 *input:
 *      CHCA_UINT   iCardIndex : the index of the card
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     none
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
void CHMG_GetRatingInfo(CHCA_UINT   iCardIndex)
{
        TMGAPICardHandle                          hApiCardHandle;
	 TMGAPIStatus                                  iAPIStatus;	

        if(iCardIndex >= MAX_APP_NUM)
        {
 #ifdef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[ChSCGetRatingInfo..] Unknow Card Index\n");
 #endif
	      return;	   	
        }
 	
        semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
	 if(SCOperationInfo[iCardIndex].iApiCardHandle != NULL)
	 {
            hApiCardHandle = SCOperationInfo[iCardIndex].iApiCardHandle;
	 }		
	 else
	 {
#ifdef  CHSC_DRIVER_DEBUG      
              do_report(severity_info,"\n[CHMG_GetRatingInfo==>] The Mg Card Handle is wrong\n");
 #endif
             semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
             return;
	 }
	 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);


        semaphore_wait(pSemMgApiAccess); 
        iAPIStatus = MGAPICardGetRating(hApiCardHandle);
      	 semaphore_signal(pSemMgApiAccess); 
		

	 if(iAPIStatus!=MGAPIOk)
	 {
#ifdef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[CHMG_GetRatingInfo==>] The function execution error\n");
#endif
	 }
	 else
	 {
#ifdef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[CHMG_GetRatingInfo..] The function 'MGAPICardGetRating' execution ok\n");
#endif
	 }

        return;

}


/*******************************************************************************
 *Function name: CHMG_GetOPIInfoList
 *           
 *
 *Description: 
 *             get all the opi list
 *             
 *    
 *Prototype:
 *            void CHMG_GetOPIInfoList(CHCA_UINT   iCardIndex)
 * 
 *           
 *
 *
 *input:
 *        CHCA_UINT   iCardIndex : index of the card
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *       none
 *      
 *     
 * 
 *
 *Comments:
 *      2005-02-24     sort the opi list from small to big     
 *******************************************************************************/
void CHMG_GetOPIInfoList(CHCA_UINT   iCardIndex)
{
        TMGAPIStatus                                  iAPIStatus,iiAPIStatus;  
        TMGAPICardHandle                          hApiCardHandle;

        TMGAPIOperator                              OPIList;
 	 CHCA_UINT                                     OpiIndex=0;   
	 CHCA_UINT                                     i;
	 TCHCAOperatorInfo                         OperatorListTemp;
		
         
#ifdef    CHSC_DRIVER_DEBUG
	 do_report(severity_info,"\n[CHMG_GetOPIInfoList==>] Read data before TotalOPINum[%d]::\n",OPITotalNum);	 
        for(OpiIndex=0;OpiIndex<OPITotalNum;OpiIndex++)
	 {
	        for(i=0;i<8;i++)
	        {
                   do_report(severity_info,"%4x",OperatorList[OpiIndex].OffersList[i]);
	        }
               do_report(severity_info,"\n");
        }
#endif	

        OPITotalNum = 0; 
	
        if(iCardIndex >= MAX_APP_NUM)
        {
 #ifdef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[CHMG_GetOPIInfoList==>] Unknow Card Index\n");
 #endif
	      return;	   	
        }	

#if 0 /*add this on 050107*/
        hApiCardHandle = SCOperationInfo[iCardIndex].iApiCardHandle;

        if(hApiCardHandle != NULL)
	 {
             iAPIStatus = MGAPICardUpdate(hApiCardHandle);
        }
#endif

        semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
        hApiCardHandle = SCOperationInfo[iCardIndex].iApiCardHandle;
        semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	if(hApiCardHandle==NULL)
	{
             do_report(severity_info,"\n[CHMG_GetOPIInfoList==>] The Mg Card Handle is Null\n");
             return;
	}


        /*semaphore_wait(pSemMgApiAccess);  add this on 050307*/
        iAPIStatus = MGAPICardGetFirstOperator(hApiCardHandle,&OPIList);
	 /*semaphore_signal(pSemMgApiAccess);  add this on 050307*/
	
        /*do_report(severity_info,"\n[CHMG_GetOPIInfoList==>] read offer status[%d]\n",iAPIStatus);*/
        switch(iAPIStatus) 
        {
             case MGAPIOk:
  			 OperatorList[OpiIndex].OPINum = OPIList.OPI;
			 OperatorList[OpiIndex].ExpirationDate[0] = OPIList.Date&0xff;
			 OperatorList[OpiIndex].ExpirationDate[1] = (OPIList.Date>>8)&0xff;
			 memcpy(OperatorList[OpiIndex].OPIName,OPIList.Name,16);
			 memcpy(OperatorList[OpiIndex].OffersList,OPIList.Offers,8);
			 OperatorList[OpiIndex].GeoZone = OPIList.Geo;
                      /*do_report(severity_info,"\nOpiIndex[%d] GeoZone[%d]\n",OpiIndex,OPIList.Geo);*/
			 OpiIndex++;
                      OPITotalNum++;

                      do
                      {
                               if(OpiIndex >=CHCA_MAX_NO_OPI)
                               {
                                     break;
				   }
					  
				   /*semaphore_wait(pSemMgApiAccess); add this on 050307*/  
                               iiAPIStatus = MGAPICardGetNextOperator(hApiCardHandle,&OPIList);
				   /*semaphore_signal(pSemMgApiAccess); add this on 050307*/	
				   
                               if(iiAPIStatus == MGAPIOk)
                               {
                                     if(OPIList.OPI)
                                     {
                                           OperatorList[OpiIndex].OPINum = OPIList.OPI;
                                           OperatorList[OpiIndex].ExpirationDate[0] = OPIList.Date&0xff;
			                      OperatorList[OpiIndex].ExpirationDate[1] = (OPIList.Date>>8)&0xff;
			                      memcpy(OperatorList[OpiIndex].OPIName,OPIList.Name,16);
                                           memcpy(OperatorList[OpiIndex].OffersList,OPIList.Offers,8);
                                           OperatorList[OpiIndex].GeoZone = OPIList.Geo;
                                           /*do_report(severity_info,"\nOpiIndex[%d] GeoZone[%d]\n",OpiIndex,OPIList.Geo); */
                                           OpiIndex++; 
                                           OPITotalNum++;
                                     }				  
                               }
                               else
                               {
                                       break;
                               }
                      
                      }while(iiAPIStatus!=MGAPIIdling);
                      do_report(severity_info,"\n[CHMG_GetOPIInfoList==>] Total OPI NUM[%d]\n",OPITotalNum);
			 break;

		case MGAPIEmpty:
#ifdef             CHSC_DRIVER_DEBUG      
                       do_report(severity_info,"\n[CHMG_GetOPIInfoList==>]Empty reader or inserted smart card invalid,unknown or not ready\n");
#endif		  	 
			 break;

		case MGAPIIdling:
#ifdef             CHSC_DRIVER_DEBUG      
                       do_report(severity_info,"\n[CHMG_GetOPIInfoList==>]No Operator inscribed in the smart card\n");
#endif		  	 
			 break;

		case MGAPINotFound:
#ifdef             CHSC_DRIVER_DEBUG      
                       do_report(severity_info,"\n[CHMG_GetOPIInfoList==>]Unknown instance handle\n");
#endif		  	 
			 break;

		case MGAPIInvalidParam:
#ifdef             CHSC_DRIVER_DEBUG      
                       do_report(severity_info,"\n[CHMG_GetOPIInfoList==>]Parameter null,aberrant or wrongly formatted\n");
#endif		  	 
			 break;

		case MGAPISysError:
#ifdef             CHSC_DRIVER_DEBUG      
                       do_report(severity_info,"\n[CHMG_GetOPIInfoList==>]A system error has occurred.\n");
#endif		  	 
			 break;
	}
       /*semaphore_signal(pSemMgApiAccess); */

	/*do_report(severity_info,"\n[Leave off  CHMG_GetOPIInfoList..]\n");	*/ 
#if 0	   
	do_report(severity_info,"\n[CHMG_GetOPIInfoList==>] Read data After TotalOPINum[%d]::\n",OPITotalNum);	 
       for(OpiIndex=0;OpiIndex<OPITotalNum;OpiIndex++)
	{
	        for(i=0;i<8;i++)
	        {
                   do_report(severity_info,"%4x",OperatorList[OpiIndex].OffersList[i]);
	        }
               do_report(severity_info,"\n");
       }
#endif

#if  1 /*modify this for bubblesort the opi of the list on 050224*/
      {
            int                        i,j;
	     boolean                exchange;
	     CHCA_SHORT       TempOpi;	 

#if  0            /*just for test*/
            if(i=0;i<OPITotalNum-1;i++)
            {
                  OperatorList[i].OPINum = OPITotalNum-i;
	     }
	      /*just for test*/	
#endif		  

	     for(i=0;i<(OPITotalNum-1);i++)	 
	     {
                  exchange=false;
                  for(j=OPITotalNum-2;j>=i;j--)
                  {
                        if(OperatorList[j+1].OPINum<OperatorList[j].OPINum)
                        {
                              OperatorListTemp.OPINum= OperatorList[j+1].OPINum;
                              OperatorListTemp.ExpirationDate[0]= OperatorList[j+1].ExpirationDate[0];
                              OperatorListTemp.ExpirationDate[1]= OperatorList[j+1].ExpirationDate[1];
                              OperatorListTemp.GeoZone= OperatorList[j+1].GeoZone;
				  memcpy(OperatorListTemp.OPIName,OperatorList[j+1].OPIName,16);
                              memcpy(OperatorListTemp.OffersList,OperatorList[j+1].OffersList,8);
			  
				 	
				  OperatorList[j+1].OPINum = OperatorList[j].OPINum; 
                              OperatorList[j+1].OPINum= OperatorList[j].OPINum;
                              OperatorList[j+1].ExpirationDate[0]= OperatorList[j].ExpirationDate[0];
                              OperatorList[j+1].ExpirationDate[1]= OperatorList[j].ExpirationDate[1];
                              OperatorList[j+1].GeoZone= OperatorList[j].GeoZone;
				  memcpy(OperatorList[j+1].OPIName,OperatorList[j].OPIName,16);
                              memcpy(OperatorList[j+1].OffersList,OperatorList[j].OffersList,8);

				  
				  OperatorList[j].OPINum = OperatorListTemp.OPINum; 
                              OperatorList[j].OPINum= OperatorListTemp.OPINum;
                              OperatorList[j].ExpirationDate[0]= OperatorListTemp.ExpirationDate[0];
                              OperatorList[j].ExpirationDate[1]= OperatorListTemp.ExpirationDate[1];
                              OperatorList[j].GeoZone= OperatorListTemp.GeoZone;
				  memcpy(OperatorList[j].OPIName,OperatorListTemp.OPIName,16);
                              memcpy(OperatorList[j].OffersList,OperatorListTemp.OffersList,8);

				  exchange=true;
				  
			   }
		    }

		    if(!exchange)
			 break;	
	     }
 
#if 0
            for(i=0;i<(OPITotalNum-1);i++)
            {
                    do_report(severity_info,"\n");
                    do_report(severity_info,"%4x",OperatorList[i].OPINum);
            }
#endif			
      }
	  
#endif


       return;
}

/*******************************************************************************
 *Function name: CHMG_GetCardContent
 *           
 *
 *Description: 
 *             get the content of the card
 *             
 *    
 *Prototype:
 *            void CHMG_GetCardContent(CHCA_UINT   iCardIndex)
 * 
 *           
 *
 *
 *input:
 *           CHCA_UINT   iCardIndex : index of the card
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *       none
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
void CHMG_GetCardContent(CHCA_UINT   iCardIndex)
{
        TMGAPICardHandle                          hApiCardHandle;
	 TMGAPIStatus                                  iAPIStatus;	

        if(iCardIndex >= MAX_APP_NUM)
        {
 #ifdef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[CHMG_GetCardContent==>] Unknow Card Index\n");
 #endif
	      return;	   	
        }
	
        semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
            hApiCardHandle = SCOperationInfo[iCardIndex].iApiCardHandle;
 	 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

		
	 if(hApiCardHandle == NULL)
	 {
             do_report(severity_info,"\n[CHMG_GetCardContent==>] The Mg Card Handle is Null\n");
             return;
	 }

        /*semaphore_wait(pSemMgApiAccess); */
        iAPIStatus = MGAPICardContent(hApiCardHandle);
        /*semaphore_signal(pSemMgApiAccess); */
		

	 if(iAPIStatus!=MGAPIOk)
	 {
#ifndef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[CHMG_GetCardContent==>] The function execution error\n");
#endif
	 }
	 else
	 {
#ifndef  CHSC_DRIVER_DEBUG      
             do_report(severity_info,"\n[CHMG_GetCardContent==>] The function 'MGAPICardContent' execution ok\n");
#endif
	 }

		
        return;

}





/*******************************************************************************
 *Function name: CHMG_CheckCardContentUpdate
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *     void CHMG_CheckCardContentUpdate(void)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     2005-3-11   add calling function CHMG_GetOPIInfoList() 
 * 
 * 
 *******************************************************************************/


void CHMG_CheckCardContentUpdate(void)
{

          /*CHMG_GetOPIInfoList(MgCardIndex);add this on 050311*/

       /*******zxg 20060402 add************/
        if(gb_ForceUpdateSmartMirror==true)
        	{
        	    gb_ForceUpdateSmartMirror=false;
                 return ;
        	}
       /*******************************/
#ifndef  NAGRA_PRODUCE_VERSION
{
extern CHCA_BOOL                                                              MpegEcmProcess;
	    MpegEcmProcess = false;
}
#endif
     //   do_report(severity_info,"\n[CHMG_CheckCardContentUpdate==>] \n");
		
         /*CHMG_GetCardContent(0);*/
         if(EMMDataProcessing)
        {
              /*CardContenUpdate = true;*/
#ifdef     CHSC_DRIVER_DEBUG 		   
              do_report(severity_info,"\n[CHMG_CheckCardContentUpdate==>] The Card mirror image is updated when receive the new emm data\n");
#endif
        }
		 

	 /*if(EmmCardUpdated)
	 {
              CardContenUpdate = true;
              do_report(severity_info,"\n[CHMG_CheckCardContentUpdate==>] EmmCardUpdated cattemp\n");
			  
	 }*/

        if(CATDataProcessing)
	 {
	       /*CardContenUpdate = true;*/
#ifdef     CHSC_DRIVER_DEBUG 		   
              do_report(severity_info,"\n[CHMG_CheckCardContentUpdate1==>] CATDataProcessing \n");
#endif
	 }

        if(TimerDataProcessing)
	 {
	       CardContenUpdate = true;
#ifdef     CHSC_DRIVER_DEBUG 		   
              do_report(severity_info,"\n[CHMG_CheckCardContentUpdate1==>] TimerDataProcessing \n");
#endif
	 }

		

         if(ECMDataProcessing)
	 {
	       CardContenUpdate = true;/*add this on 050501*/
#ifdef     CHSC_DRIVER_DEBUG 		   
              do_report(severity_info,"\n[CHMG_CheckCardContentUpdate2==>] ECMDataProcessing\n");
#endif
	 }

}


/*******************************************************************************
 *Function name: CHMG_CheckCardReady
 * 
 *
 *Description: get the status of the card
 *                  
 *
 *Prototype:
 *     void CHMG_CheckCardReady(TMGAPICardContent* pContent)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
void CHMG_CheckCardReady(TMGAPICardContent* pContent)
{
       CHCA_UINT                   i;
       CHCA_UINT                   iAppIndex;

	/*do_report(severity_info,"\n[CHMG_CheckCardReady==>]enter into the \n");  */ 
	   
       if(pContent==NULL)
       {
             do_report(severity_info,"\n[CHMG_CheckCardReady==>] pContent is null\n");
             return;
	}

       if((pContent->Type.nApp==0)||( pContent->Type.nApp>MAX_APP_NUM))
       {
              do_report(severity_info,"\n[CHMG_CheckCardReady==>] App Num is equal to zero\n");
              return;  
	}

        /*reset the card serial number as oxff*/
	memcpy(CaCardContent.CardID.CA_SN_Number,pContent->ID.SerialNumber,6);	
	memcpy(CaCardContent.CardID.CA_Product_Code,pContent->ID.ProductCode,2);	

	for(iAppIndex=0;iAppIndex<pContent->Type.nApp;iAppIndex++)
	{
              CaCardContent.CardType.SCAppData[iAppIndex].AppID = pContent->Type.pAppData->ID;
              CaCardContent.CardType.SCAppData[iAppIndex].MajorVersion = pContent->Type.pAppData->Major;
              CaCardContent.CardType.SCAppData[iAppIndex].MinorVersion = pContent->Type.pAppData->Major;
	}
	
	CaCardContent.CardType.SCType = pContent->Type.Type;
	CaCardContent.CardType.SCAppNum = pContent->Type.nApp;

#ifdef  PAIR_CHECK
#ifndef       CHSC_DRIVER_DEBUG
       do_report(severity_info,"\nRead CARDE==>");
       for(i=0;i<6;i++)	
       {
              do_report(severity_info,"%4x",pastPairInfo.CaSerialNumber[i]);
       }
       do_report(severity_info,"\n");

       do_report(severity_info,"\nRead CARDF==>");
       for(i=0;i<6;i++)	
       {
              do_report(severity_info,"%4x",CaCardContent.CardID.CA_SN_Number[i]);
       }
       do_report(severity_info,"\n");
	   
#endif
#endif	

       /*delete this on 050221*/
#ifdef  PAIR_CHECK /*add this on 050113*/
       if(memcmp(pastPairInfo.CaSerialNumber,CaCardContent.CardID.CA_SN_Number,6))
       {
                 CardPairStatus = CHCAPairError;
#ifdef       CHSC_DRIVER_DEBUG				 
                 do_report(severity_info,"\n[CHMG_CheckCardReady==>]New Card Pairing Error\n");	
#endif

       }
       else
       {
#ifdef       CHSC_DRIVER_DEBUG				 
                 do_report(severity_info,"\n[CHMG_CheckCardReady==>]New Card Pairing OK\n");
#endif

                 CardPairStatus = CHCAPairOK;
       }
#endif


	/*do_report(severity_info,"\n[CHMG_CheckCardReady==>]leave off the \n");*/

       return;
		 
 
}


/*******************************************************************************
 *Function name: CHMG_GetCardStatus
 * 
 *
 *Description: get the status of the card
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHMG_GetCardStatus(void)           
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL CHMG_GetCardStatus(void)
{
         CHCA_BOOL                              bErrCode = false;
	  ST_ErrorCode_t  	                      stErrCode;
	  STSMART_Status_t                    *Status_p;

	  if(!SCOperationInfo[MgCardIndex].InUsed)
	  {
	        bErrCode = true;
               do_report(severity_info,"\n[CHCA_GetCardStatus==>] The Card instance has not been opened!\n");
		 return bErrCode;
	  }

	  stErrCode = STSMART_GetStatus(SCOperationInfo[MgCardIndex].SCHandle,Status_p);

	  switch(stErrCode)
	  {
           case STSMART_ERROR_NOT_INSERTED:

			     CHCA_MepgDisable(true,true);	 /*add this on 050407*/ 

			     SCOperationInfo[MgCardIndex].SC_Status = CH_EXTRACTED;
    	                   CardOperation.SCStatus = CH_EXTRACTED;	 
 #ifndef CH_IPANEL_MGCA				 					  							   						   					   
						   
			     if(CHCA_CardSendMess2Usif(NO_CARD))
			     {
                                do_report(severity_info,"\n[CHMG_GetCardStatus::] Fail to send the no card message to usif\n");
			     }
#else
       if(CHCA_CardSendMess2Usif(CH_CA2IPANEL_EXTRACT_CARD,NULL,0))
			     {
                                do_report(severity_info,"\n[CHMG_GetCardStatus::] Fail to send the no card message to usif\n");
			     }
#endif
			     break;
  
		   case ST_ERROR_INVALID_HANDLE:

		   	     CHCA_MepgDisable(true,true); /*add this on 050407*/ 

		   	     bErrCode = true;
 #ifndef CH_IPANEL_MGCA				 					  							   						   					   
				 
			     if(CHCA_CardSendMess2Usif(NO_CARD))
			     {
                                do_report(severity_info,"\n[CHMG_GetCardStatus::] Fail to send the no card message to usif\n");
			     }
#else
       if(CHCA_CardSendMess2Usif(CH_CA2IPANEL_EXTRACT_CARD,NULL,0))
			     {
                                do_report(severity_info,"\n[CHMG_GetCardStatus::] Fail to send the no card message to usif\n");
			     }
#endif
                          break;
			
	          default:
			     SCOperationInfo[MgCardIndex].SC_Status = CH_INSERTED;
	                   CardOperation.SCStatus = CH_INSERTED;	
			     break;	
	  }

         return bErrCode;     

}


/*******************************************************************************
 *Function name: CHMG_CardContent
 * 
 *
 *Description: get the status of the card,called in the callback function
 *                  
 *
 *Prototype:
 *     void  CHMG_CardContent(TMGAPICardContent*   pCardContent)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
void  CHMG_CardUnknown(TMGSysSCRdrHandle   hReader)
{
       do_report(severity_info,"\n[CHMG_CardUnknown==>]The Card present in the reader is not recognized,The Sys Handle[%4x]\n",hReader);      
 
}


/*******************************************************************************
 *Function name: CHMG_CardContent
 * 
 *
 *Description: get the status of the card,called in the callback function
 *                  
 *
 *Prototype:
 *     void  CHMG_CardContent(TMGAPICardContent*   pCardContent)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
void  CHMG_CardContent(TMGAPICardContent*   pCardContent)
{
       CHCA_UINT                   iAppIndex;

	/*do_report(severity_info,"\n[CHMG_CardContent==>]get the status of the card \n"); */  
	   
       if(pCardContent==NULL)
       {
             do_report(severity_info,"\n[CHMG_CardContent==>] App Num is equal to zero\n");
             return;
	}

       if((pCardContent->Type.nApp==0)||( pCardContent->Type.nApp>MAX_APP_NUM))
       {
              do_report(severity_info,"\n[CHMG_CardContent==>] App Num is equal to zero\n");
              return;  
	}

        /*reset the card serial number as oxff*/
	memcpy(CaCardContent.CardID.CA_SN_Number,pCardContent->ID.SerialNumber,6);	
	memcpy(CaCardContent.CardID.CA_Product_Code,pCardContent->ID.ProductCode,2);	

	for(iAppIndex=0;iAppIndex<pCardContent->Type.nApp;iAppIndex++)
	{
              CaCardContent.CardType.SCAppData[iAppIndex].AppID = pCardContent->Type.pAppData->ID;
              CaCardContent.CardType.SCAppData[iAppIndex].MajorVersion = pCardContent->Type.pAppData->Major;
              CaCardContent.CardType.SCAppData[iAppIndex].MinorVersion = pCardContent->Type.pAppData->Major;
	}
	
	CaCardContent.CardType.SCType = pCardContent->Type.Type;
	CaCardContent.CardType.SCAppNum = pCardContent->Type.nApp;



       return;
}


/*******************************************************************************
 *Function name: CHMG_CardRating
 * 
 *
 *Description: get the rating info of the card,called in the callback function
 *                  
 *
 *Prototype:
 *     void  CHMG_CardRating(TMGAPIRating*  pCardRating)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
void  CHMG_CardRating(TMGAPIRating*  pCardRating)
{
       if(pCardRating==NULL)
       {
             do_report(severity_info,"\n[CHMG_CardRating==>] input pointor is null\n");
	      return;		 
	}

       CardRatingInfo.RatingValid = (pCardRating->Valid) & MGAPIRating;
	CardRatingInfo.RatingInfo = pCardRating->Rating;
	CardRatingInfo.RatingCheckedValid = ((pCardRating->Valid) & MGAPIRatingCheck)>>1;
	CardRatingInfo.RatingChecked = pCardRating->IsChecked;

       return;
}


#ifdef      MOSAIC_APP
/*******************************************************************************
 *Function name: CheckGeoZoneRight
 * 
 *
 *Description: check the rigth of the geographical  zone
 *                  
 *
 *Prototype:
 *     CHCA_BOOL   CheckGeoZoneRight(CHCA_UCHAR PageIndex,CHCA_UCHAR CellIndex,CHCA_USHORT pz)
 *
 *input:
 *     CHCA_UCHAR        PageIndex
 *     CHCA_UCHAR        CellIndex
 *     CHCA_USHORT      pz
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *
 *Comments:
 *  for mosaic application
 *     
 * 
 *******************************************************************************/
CHCA_BOOL   CheckGeoZoneRight(CHCA_UCHAR PageIndex,CHCA_UCHAR CellIndex,CHCA_USHORT pz)
{
       U32                          i;
       U8                           geo_zones;
       U32                         GeoIndex=0;
#ifdef BEIJING_MOSAIC	
	/*do_report(0,"Geo num1=%d,com OPi=%d\n",CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM,OperatorList[pz].OPINum);*/
	if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones==NULL||
	   /*CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones->Cur_zone_number<=0*/
	CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM<=0)
	{
	     return true;
	}	 
       for(GeoIndex=0;GeoIndex<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM;GeoIndex++)
       {
	/*do_report(0,"Geo num2=%d\n",CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Cur_zone_number);*/
       		/*do_report(0,"List_mode=%d,OPI=%d,num=%d\n",CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].List_mode,
				CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].OPI,CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones->Cur_zone_number);*/

              if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].OPI==OperatorList[pz].OPINum)
              {
                        /* do_report(0,"fin index=%d\n",GeoIndex);*/
                     break;
              }
       }

	if(GeoIndex>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM)
	{
             return true;
	}
	
       /*	if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones->OPI != OperatorList[pz].OPINum)
	{
	     return true;
	}
       */
       
       geo_zones=OperatorList[pz].GeoZone;
	 
        /*do_report(0,"List_mode=%d,oPI=%d\n",
	  	CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].List_mode,CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones->OPI);*/
	if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].List_mode==0x00)
	{
		U8 index1;
		U8 index2;
		index1=geo_zones/8;
		index2=geo_zones%8;
		if(((CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Concealed_zone_number[index1]<<index2)&
			0x01))
		{
			return true;
		}
		else
		{
                    return false;
		}
	}
	else if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].List_mode==0x01)
	{
		for(i=0;i<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Cur_zone_number;i++)
		{
		      /*do_report(0,"compare geo_zones[%d] vs [%d] \n",geo_zones,CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Concealed_zone_number[i]);*/
			if(geo_zones==CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Concealed_zone_number[i])
				return false;
		}
		return true;
	}
	else if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].List_mode==0x02)
	{
		for(i=0;i<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Cur_zone_number;i++)
		{
			if(geo_zones==CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[GeoIndex].Concealed_zone_number[i])
				return true;
		}
		return false;
	}	
#else
   return true;
#endif
    

}


/*******************************************************************************
 *Function name: CHCA_CheckOfferRight
 * 
 *
 *Description: check the rigth of the offer
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_CheckOfferRight(Authorized_offers_des *Authorized_offers)
 *
 *input:
 *     Authorized_offers_des *Authorized_offers,CA OFFER信息
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *
 *Comments:
 *  for mosaic application
 *  2005-02-23     modify      if one opi can not find on the opilist of the mosaic application descriptor,  
 *                                        then the opi has no the right
 *  2005-02-24      modify     add new status "UNKNOWN_STATUS"
 *  2005-02-25      modify      
 *******************************************************************************/
CAOfferEnum  CHCA_CheckOfferRight(U8 PageIndex,U8 CellIndex)
{
        CHCA_BOOL             CheckSuccess = false;
	 CHCA_USHORT         iOpiIndex,CheckOPI;
        CHCA_INT                CurrentDate;
        CHCA_TDTDATE        current_sys_date;
	 CHCA_BOOL             expired=false;
	 CAOfferEnum            OfferRight;
	 CHCA_INT                MinOPINum,i,CurSelectItem;
        CHCA_USHORT         iOpiNum;
	 CHCA_BOOL             NOfferInfo,NGeoInfo;	

	 CAOfferEnum           ALLOpiStatus[16];

	 CHCA_INT               j;

#ifdef BEIJING_MOSAIC
	 if((CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones==NULL||
	     CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM<=0)&&\
	    (CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM<=0||
	     CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers==NULL))
	 {
              OfferRight = HAVE_RIGHT;
	       return OfferRight;	
	 }	 

	 for(i=0;i<OPITotalNum;i++)
	 {
	   	   /*first check date expire*/
		   /*do_report(0,"opi=%d,OPITotalNum=%d\n",OperatorList[i].OPINum,OPITotalNum);*/
		   CheckOPI=i;
		   NOfferInfo=false;
		   NGeoInfo=false;

#if 1 /*add this on 050228*/
                 for(j=0;j<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM;j++) 
                 {
                       if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].OPI==OperatorList[i].OPINum)
                       {
                             break;
                       }
                 }

                 if(j>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM)
                 {
                        NOfferInfo = true;
		   }


                 for(j=0;j<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM;j++)
                 {
                     if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Concealed_geo_zones[j].OPI==OperatorList[i].OPINum)
                     {
                          break;
                     }
                 }

	          if(j>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].GEO_OPI_NUM)
	          {
                    NGeoInfo = true;
	          }
				 
                 if(NGeoInfo&&NOfferInfo)
       	   {
			  ALLOpiStatus[i]=UNKNOWN_STATUS;	
			  continue;		   
		   }
#endif		   

                 CHCA_GetCurrentDate(&current_sys_date);
	          /*CurrentDate = (((((current_sys_date.sYear - 1990)&0x7f)<<1 )|((current_sys_date.ucMonth & 0x1f) >> 4 )) << 8)| (((current_sys_date.ucMonth & 0x1f) << 4) | (current_sys_date.ucDay & 0xf));*/
	#if 1/*20050704*/
	          CurrentDate = ((((current_sys_date.ucMonth & 0x7) <<5) | (current_sys_date.ucDay & 0x1f)))|((((current_sys_date.sYear - 1990)&0x7f)<<9 )|(((current_sys_date.ucMonth>>3) & 0x1) <<8 ));
	#else
		   CurrentDate = ((((current_sys_date.ucMonth & 0x7) << 5) | (current_sys_date.ucDay & 0x1f))<<8)|((((current_sys_date.sYear - 1990)&0x7f)<<1 )|((current_sys_date.ucMonth & 0x8) >> 3 ));
	#endif
		   /*do_report(severity_info,"\n[CHCA_CheckOfferRight==>]Year[%d]Month[%d]Date[%d]\n",current_sys_date.sYear,current_sys_date.ucMonth,current_sys_date.ucDay);	*/  
		   if(!ChCheckExpiration(CurrentDate,CheckOPI))
                 {
                       /*second check geo*/
		  	  if(CheckGeoZoneRight(PageIndex,CellIndex,CheckOPI)==true)
		  	  {
                                   /*third check ca offer*/
					if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM<=0||
	                              CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers==NULL)
	                           {
#ifdef                             CHSC_DRIVER_DEBUG
                                       do_report(severity_info,"\n[CHCA_CheckOfferRight==>] input parameter is wrong\n");
#endif
                                       OfferRight = HAVE_RIGHT;
					    return OfferRight;
	                           }
					
                                  for(j=0;j<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM;j++) 
                                  {
                                  	       /*do_report(0,"[%d]offer find opi=%d\n",j,CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].OPI);*/
                                          if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].OPI==OperatorList[i].OPINum)
                                          {
                                                   if(ChSubOfferCheck(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].Offers_Map,CheckOPI)==true)
                                                   {
                                                           /*do_report(severity_info,"\n[CHCA_CheckOfferRight==>] HAVE_RIGHT,OperatorList.OPINum[%d]\n",OperatorList[i].OPINum);*/
                                                           OfferRight = HAVE_RIGHT;
					                        return OfferRight;	
                                                   }
							  else
							  {
                                                           ALLOpiStatus[i]=NO_OFFER;
							  }
							  break;
                                          }
                                  	}

#if 1/*modify this on 050223*/
					if(j>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM)
					{
				              ALLOpiStatus[i]=UNKNOWN_STATUS;
					}
#else
					/*do_report(severity_info,"index[%d ]offeropi[%d ],OfferRight=%d\n",i,OperatorList[i].OPINum,OfferRight);*/
					if(j>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM)
					{
				              OfferRight = HAVE_RIGHT;
					       return OfferRight;		
					}
#endif					
		  	  }
			  else
			  {
                              ALLOpiStatus[i]=GEO_BOUT;
			  }
		}
		else
		{
                      ALLOpiStatus[i]=EXPIRE_DATE;
		}
	}

#if 0 
       j=0;
	MinOPINum=OperatorList[0].OPINum;
	for(i=1;i<OPITotalNum;i++)
	{
            if(OperatorList[i].OPINum<MinOPINum)
            {
                 MinOPINum=OperatorList[i].OPINum;
                 j=i;
            }
	}

	OfferRight=ALLOpiStatus[j];

#else
       /*modify this on 050224*/
       /*the opi list has already been sorted from small to large*/
	for(i=0;i<OPITotalNum;i++)
	{
            if(ALLOpiStatus[i] != UNKNOWN_STATUS)
            {
                  OfferRight=ALLOpiStatus[i];
                  break;
	     }
	}

       if(i>=OPITotalNum)
       {
            OfferRight = NO_OFFER;
	}
#endif
                  
	return OfferRight;	
#else
    return OfferRight;
#endif

}

#else
CAOfferEnum  CHCA_CheckOfferRight(U8 PageIndex,U8 CellIndex)
{
        CHCA_BOOL             CheckSuccess = false;
	 CHCA_USHORT         iOpiIndex,CheckOPI;
        CHCA_INT                CurrentDate;
        CHCA_TDTDATE        current_sys_date;
	 CHCA_BOOL             expired=false;
	 CAOfferEnum            OfferRight;
	 CHCA_INT                MinOPINum,i,CurSelectItem;
        CHCA_USHORT         iOpiNum;

	 CAOfferEnum           ALLOpiStatus[16];

	 CHCA_INT               j;

	 
	 
	 for(i=0;i<OPITotalNum;i++)
	 {
	   	   /*first check date expire*/
		   /*do_report(0,"opi=%d,OPITotalNum=%d\n",OperatorList[i].OPINum,OPITotalNum);*/
		   CheckOPI=i;
                 CHCA_GetCurrentDate(&current_sys_date);
	          CurrentDate = (((((current_sys_date.sYear - 1990)&0x7f)<<1 )|((current_sys_date.ucMonth & 0x1f) >> 4 )) << 8)| (((current_sys_date.ucMonth & 0x1f) << 4) | (current_sys_date.ucDay & 0xf));
                 if(!ChCheckExpiration(CurrentDate,CheckOPI))
                 {
                       /*second check geo*/
		  	  if(CheckGeoZoneRight(PageIndex,CellIndex,CheckOPI)==true)
		  	  {
                                   /*third check ca offer*/
					if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM<=0||
	                              CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers==NULL)
	                           {
#ifdef                             CHSC_DRIVER_DEBUG
                                       do_report(severity_info,"\n[CHCA_CheckOfferRight==>] input parameter is wrong\n");
#endif
                                       OfferRight = HAVE_RIGHT;
					    return OfferRight;
	                           }
					
                                  for(j=0;j<CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM;j++) 
                                  {
                                  	       /*do_report(0,"[%d]offer find opi=%d\n",j,CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].OPI);*/
                                          if(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].OPI==OperatorList[i].OPINum)
                                          {
                                                   if(ChSubOfferCheck(CurMosaic.PageHead[PageIndex].CellHead[CellIndex].Authorized_offers[j].Offers_Map,CheckOPI)==true)
                                                   {
                                                           /*do_report(severity_info,"\n[CHCA_CheckOfferRight==>] HAVE_RIGHT,OperatorList.OPINum[%d]\n",OperatorList[i].OPINum);*/
                                                           OfferRight = HAVE_RIGHT;
					                        return OfferRight;	
                                                   }
							  else
							  {
                                                           ALLOpiStatus[i]=NO_OFFER;
							  }
							  break;
                                          }
                                  	}

#if 1/*modify this on 050223*/
					if(j>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM)
					{
				              ALLOpiStatus[i]=UNKNOWN_STATUS;
					}
#else
					/*do_report(severity_info,"index[%d ]offeropi[%d ],OfferRight=%d\n",i,OperatorList[i].OPINum,OfferRight);*/
					if(j>=CurMosaic.PageHead[PageIndex].CellHead[CellIndex].AUTHORIZE_OPI_NUM)
					{
				              OfferRight = HAVE_RIGHT;
					       return OfferRight;		
					}
#endif					
		  	  }
			  else
			  {
                              ALLOpiStatus[i]=GEO_BOUT;
			  }
		}
		else
		{
                      ALLOpiStatus[i]=EXPIRE_DATE;
		}
	}

#if 0 
       j=0;
	MinOPINum=OperatorList[0].OPINum;
	for(i=1;i<OPITotalNum;i++)
	{
            if(OperatorList[i].OPINum<MinOPINum)
            {
                 MinOPINum=OperatorList[i].OPINum;
                 j=i;
            }
	}

	OfferRight=ALLOpiStatus[j];

#else
       /*modify this on 050224*/
       /*the opi list has already been sorted from small to large*/
	for(i=0;i<OPITotalNum;i++)
	{
            if(ALLOpiStatus[i] != UNKNOWN_STATUS)
            {
                  OfferRight=ALLOpiStatus[i];
                  break;
	     }
	}

       if(i>=OPITotalNum)
       {
            OfferRight = NO_OFFER;
	}
#endif
              
	return OfferRight;	
}
#endif



/*******************************************************************************
 *Function name: CHCA_CheckGeoGraphicalRight
 * 
 *
 *Description: check the rigth of the Geographical
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_CheckGeoGraphicalRight(Conditionnal_actions_des  *conditional_action)
 *
 *input:
 *     Authorized_offers_des *Authorized_offers,CA OFFER信息
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *   typedef struct
 *   {  
 *	    U16              OPI;
 *	    U8                Cur_zone_number;
 *	    U8                Geographical_zone_numbe[MAX_GEOGRAPHICAL_ZONE_NUM];
 *	    Action_des     action[MAX_GEOGRAPHICAL_ZONE_NUM];
 *   }Conditionnal_actions_des;
 *
 * 
 * 
 *******************************************************************************/
 #ifdef BEIJING_MOSAIC

BOOL CHCA_CheckGeoGraphicalRight(Conditionnal_actions_des  *conditional_action,U16   iOpiNum)
{
         CHCA_BOOL          RightOk=false;
         CHCA_USHORT      iOpiIndex,CheckOPI,GZoneIndex;  

         if((conditional_action==NULL)||(iOpiNum==0))
         {
#ifndef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n[CHCA_CheckGeoGraphicalRight] input parameter is wrong\n");
#endif
               return RightOk;
	  }

         for(iOpiIndex=0;iOpiIndex<iOpiNum;iOpiIndex++)
         {
		  CheckOPI = ChCrossCheckOPI(conditional_action[iOpiIndex].OPI);

		  if(CheckOPI<CHCA_MAX_NO_OPI)
		  {
                        for(GZoneIndex=0;GZoneIndex<conditional_action[iOpiIndex].Cur_zone_number;GZoneIndex++)
                        {
                               if(conditional_action[iOpiIndex].Geographical_zone_numbe[GZoneIndex]==OperatorList[CheckOPI].GeoZone)
                               {
                                       RightOk = true;
                                       /*do_report(severity_info,"\n[CHCA_CheckGeoGraphicalRight::] The Geo Right of the OPI[%d] is ok\n",conditional_action[iOpiIndex].OPI);*/
					    return RightOk;				 
				   }
			   }
		  }
		  else
		  {
                       do_report(severity_info,"\n[CHCA_CheckGeoGraphicalRight::] The OPI[%d] is wrong\n",conditional_action[iOpiIndex].OPI);
		  }
	  }

	  return RightOk;		 
		 
}
#endif

/*******************************************************************************
 *Function name: CHCA_CheckCountryRight
 * 
 *
 *Description: check the rigth of the country
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_CheckCountryRight(Country_availability_des  *Country_availability)
 *
 *input:
 *     Country_availability_des  *Country_availability
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 * typedef struct
 *{
 *  	 BOOL Country_availability_flag[MAX_LANGUAGE_ID];
 * }Country_availability_des;
 *
 * 
 * 
 *******************************************************************************/
#ifdef BEIJING_MOSAIC
BOOL CHCA_CheckCountryRight(Country_availability_des  *Country_availability)
{
         CHCA_BOOL          RightOk=false;
         CHCA_USHORT      iOpiIndex,CheckOPI,GZoneIndex;  

         if(Country_availability==NULL)
         {
#ifdef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n[CHCA_CheckCountryRight] input parameter is wrong\n");
#endif
               return RightOk;
	  }

         RightOk=true;

	  return RightOk;	 
 
}
#endif
/*******************************************************************************
 *Function name: CHCA_CheckCountryRight
 * 
 *
 *Description: check the rigth of the ParentalRight
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_CheckCountryRight(Country_availability_des  *Country_availability)
 *
 *input:
 *     Country_availability_des  *Country_availability
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
typedef struct
{
   U8 Rating[MAX_LANGUAGE_ID]; 
                                                 *0x00	Undefined
							0x01 to 0x0F	minimum age = rating + 3 years
							0x10	(priv) Code CSA all
							0x11	(priv) Code CSA Accord parental souhaitable
							0x12	(priv) Code CSA -12 ans
							0x13	(priv) Code CSA -16 ans
							0x14	(priv) Code CSA -18 ans / pour adultes seulement
							0x15 to 0xFF	Defined by the broadcaster
 }Parental_rating_des; 
 * 
 * 
 *******************************************************************************/
#ifdef BEIJING_MOSAIC
BOOL CHCA_CheckParentalRight(Parental_rating_des  *Parental_rating)
{
        CHCA_BOOL          RightOk=false;
	 CHCA_UINT           CountryIndex;	

        if((Parental_rating==NULL)||(Parental_rating->Rating_num == 0))
        {
#ifndef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n[CHCA_CheckCountryRight] input parameter is wrong\n");
#endif
               return RightOk;
	 }

        if(!CardRatingInfo.RatingValid)		
        {
#ifndef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n[CHCA_CheckCountryRight] input parameter is wrong\n");
#endif
               return RightOk;
	 }

	 for(CountryIndex=0;CountryIndex<Parental_rating->Rating_num;CountryIndex++) 
	 {
	          if(Parental_rating->Rating[CountryIndex]<=CardRatingInfo.RatingInfo)
	          {
                      RightOk = true;
                      /*do_report(severity_info,"\n[CHCA_CheckParentalRight::] The Parental Right of the channel  is ok\n");*/
		        return RightOk;
		   }
	 }
  
        return RightOk;		
 
}
#endif


/*******************************************************************************
 *Function name: CHCA_SCardHwInit
 *           
 *
 *Description: init the smart card hardware
 *             
 *             
 *    
 *Prototype:
 *     BOOL CHCA_SCardHwInit(void)           
 *           
 *
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *     
 *     
 * 
 *
 *Comments:
 *         called in the initterm.c  
 *******************************************************************************/
 

 BOOL  CHCA_SCardHwInit(void)
{
    ST_ErrorCode_t                             ErrCode = ST_NO_ERROR;   

#define SYSTEM_CONFIG_BASE               (U32)0xB9001000                     /*系统基地址*/
#define SYS_CONFIG_7_OFFSET              0x11C                               /*偏移地址*/               
#define SYSTEM_CONFIG_7                  (SYSTEM_CONFIG_BASE + SYS_CONFIG_7_OFFSET)  /*SYSTEM_CONFIG_7地址*/

    STUART_InitParams_t                      UartInitParams;
    ST_ClockInfo_t     ClockInfo;  /*add this on 050607*/
	
    STUART_Params_t                        UartDefaultParams; 	
    STSMART_InitParams_t                 SmartInitParams;
	
    STEVT_InitParams_t                     EVTInitParams;
    
    STSYS_WriteRegDev32LE(SYSTEM_CONFIG_7,0x1B0  );/*zxg 20070417 change,配置智能卡时钟源*/
 
    ST_GetClockInfo(&ClockInfo);/*add this on 050607*/

    UartDefaultParams.RxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.RxMode.StopBits = STUART_STOP_0_5;
    UartDefaultParams.RxMode.FlowControl = STUART_NO_FLOW_CONTROL/*STUART_SW_FLOW_CONTROL*/;
    UartDefaultParams.RxMode.BaudRate = /*9600*/115200;
    UartDefaultParams.RxMode.TermString = NULL;
    UartDefaultParams.RxMode.NotifyFunction = NULL;
    UartDefaultParams.TxMode.DataBits = STUART_8BITS_EVEN_PARITY;
    UartDefaultParams.TxMode.StopBits = STUART_STOP_0_5;
    UartDefaultParams.TxMode.FlowControl =/* STUART_NO_FLOW_CONTROL*/STUART_SW_FLOW_CONTROL;
    UartDefaultParams.TxMode.BaudRate =115200;
    UartDefaultParams.TxMode.TermString = NULL;
    UartDefaultParams.TxMode.NotifyFunction = NULL;
    UartDefaultParams.SmartCardModeEnabled = TRUE;
    UartDefaultParams.GuardTime = 22;
    UartDefaultParams.Retries = 6;
    UartDefaultParams.NACKSignalDisabled = FALSE;
    UartDefaultParams.HWFifoDisabled = FALSE;
    UartDefaultParams.SmartCardModeEnabled = TRUE;	


#ifdef SMART_SLOT0     
	
    /* UART0  initialization params */       
    UartInitParams.UARTType = STUART_ISO7816;  
    UartInitParams.DriverPartition = SystemPartition;
    UartInitParams.BaseAddress = (U32 *)ASC_0_BASE_ADDRESS;
    UartInitParams.InterruptNumber = ASC_0_INTERRUPT;
    UartInitParams.InterruptLevel = ASC_0_INTERRUPT_LEVEL;
    UartInitParams.ClockFrequency =/*ClockInfo.CommsBlock*/66500000 /*zxg 20070417 change ClockInfo.ckga.com_clk*/;
    UartInitParams.SwFIFOEnable = TRUE;
    UartInitParams.FIFOLength = 256;
    strcpy(UartInitParams.RXD.PortName, PIO_DeviceName[0]);
    UartInitParams.RXD.BitMask = PIO_BIT_1;
    strcpy(UartInitParams.TXD.PortName, PIO_DeviceName[0]);
    UartInitParams.TXD.BitMask = PIO_BIT_0;
    strcpy(UartInitParams.RTS.PortName, "\0");
    UartInitParams.RTS.BitMask = 0;
    strcpy(UartInitParams.CTS.PortName, "\0");
    UartInitParams.CTS.BitMask = 0;
#if 0/*20060207 change NULL to &UartDefaultParams*/
    UartInitParams.DefaultParams = &UartDefaultParams;
#else
    UartInitParams.DefaultParams = NULL;
#endif
    /* Smartcard0  initialization params */
    strcpy(SmartInitParams.UARTDeviceName, UARTDeviceName_SC[0]);
    SmartInitParams.DeviceType = STSMART_ISO;
    SmartInitParams.DriverPartition = SystemPartition;
    SmartInitParams.BaseAddress = (U32 *)SMARTCARD0_BASE_ADDRESS;
    strcpy(SmartInitParams.ClkGenExtClk.PortName, "\0");
    SmartInitParams.ClkGenExtClk.BitMask = 0;
    strcpy(SmartInitParams.Clk.PortName, PIO_DeviceName[0]);
    SmartInitParams.Clk.BitMask = PIO_BIT_3;    
    strcpy(SmartInitParams.Reset.PortName,  PIO_DeviceName[0]);
    SmartInitParams.Reset.BitMask = PIO_BIT_4;
    strcpy(SmartInitParams.CmdVcc.PortName, PIO_DeviceName[0]);
    SmartInitParams.CmdVcc.BitMask = PIO_BIT_5;
    strcpy(SmartInitParams.CmdVpp.PortName, "\0");
    SmartInitParams.CmdVpp.BitMask = 0;
    strcpy(SmartInitParams.Detect.PortName, PIO_DeviceName[0]);
    SmartInitParams.Detect.BitMask = PIO_BIT_7;
    SmartInitParams.ClockSource = STSMART_CPU_CLOCK;
    SmartInitParams.ClockFrequency =66500000/*ClockInfo.CommsBlock *//*zxg 20070417 change ClockInfo.ckga.com_clk*/;
    SmartInitParams.MaxHandles = 32;
    strcpy(SmartInitParams.EVTDeviceName, EVTDeviceName_SC[0]);       
	
	SmartInitParams.IsInternalSmartcard=false;
    /* Event initialization params */
    EVTInitParams.MemoryPartition = SystemPartition;
    EVTInitParams.EventMaxNum = STSMART_NUMBER_EVENTS * 2;
    EVTInitParams.ConnectMaxNum = 3;
    EVTInitParams.SubscrMaxNum = STSMART_NUMBER_EVENTS * 2;    
	
    /* UART0 initialization */
#if 0/*add this on 041207*/
    ErrCode = STSMARTUART_Init(UARTDeviceName_SC[0], &UartInitParams);
#else
    ErrCode = STUART_Init(UARTDeviceName_SC[0], &UartInitParams);
#endif

	/* EVT_SC0  initialization */
    ErrCode |= STEVT_Init(EVTDeviceName_SC[0], &EVTInitParams);
	/* SC0  initialization */
    ErrCode |= STSMART_Init(SMARTDeviceName[0], &SmartInitParams);
	/* SC0  Open */  
    /*ErrCode |= STSMART_Open(SMARTDeviceName[0], &SmartOpenParams, &SCHandle);*/
	
    if (ErrCode != ST_NO_ERROR)
    {
		do_report(severity_error, "%s,%d>Unable to init SC0\n",__FILE__,__LINE__);
		return FALSE; 	
    }
	
    /* EVT_SC0 Open and Subscribe Smart_card Event */  
    /*ErrCode = STEVT_Open(EVTDeviceName_SC[0], &EVTOpenParams, &EVTHandle);                        
    ErrCode |= STEVT_Subscribe(EVTHandle,STSMART_EV_CARD_INSERTED,&SubscribeParams);            
	
    if (ErrCode != ST_NO_ERROR)
		return FALSE; 	*/
	
#endif    
    
	
#ifdef SMART_SLOT1
    /* UART1  initialization params, pio1 initialization params*/
    UartInitParams.UARTType = STUART_ISO7816;  
    UartInitParams.DriverPartition = (ST_Partition_t *)system_partition;
    UartInitParams.BaseAddress = (U32 *)ASC_1_BASE_ADDRESS;
    UartInitParams.InterruptNumber = ASC_1_INTERRUPT;
    UartInitParams.InterruptLevel = ASC_1_INTERRUPT_LEVEL;
    UartInitParams.ClockFrequency = /*ST_GetClockSpeed()*/ClockInfo.CommsBlock;
    UartInitParams.SwFIFOEnable = TRUE;
    UartInitParams.FIFOLength = 256;
    strcpy(UartInitParams.RXD.PortName, "PIO1");
    UartInitParams.RXD.BitMask = PIO_BIT_0;
    strcpy(UartInitParams.TXD.PortName, "PIO1");
    UartInitParams.TXD.BitMask = PIO_BIT_0;
    strcpy(UartInitParams.RTS.PortName, "\0/*PIO6*/");
    UartInitParams.RTS.BitMask = 0;
    strcpy(UartInitParams.CTS.PortName, "\0/*PIO6*/");
    UartInitParams.CTS.BitMask = 0;
    UartInitParams.DefaultParams = NULL;

#if 0
    UartInitParams[1].UARTType = STUART_ISO7816;
    UartInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
    UartInitParams[1].BaseAddress = (U32 *)ASC_1_BASE_ADDRESS;
    UartInitParams[1].InterruptNumber = ASC_1_INTERRUPT;
    UartInitParams[1].InterruptLevel = ASC_1_INTERRUPT_LEVEL;
    UartInitParams[1].ClockFrequency = CLOCK_COMMS;
    UartInitParams[1].SwFIFOEnable = TRUE;
    UartInitParams[1].FIFOLength = 256;
    strcpy(UartInitParams[1].RXD.PortName, PioDeviceName[ASC_1_RXD_DEV]);
    UartInitParams[1].RXD.BitMask = ASC_1_RXD_BIT;
    strcpy(UartInitParams[1].TXD.PortName, PioDeviceName[ASC_1_TXD_DEV]);
    UartInitParams[1].TXD.BitMask = ASC_1_TXD_BIT;
    strcpy(UartInitParams[1].RTS.PortName, PioDeviceName[ASC_1_RTS_DEV]);
    UartInitParams[1].RTS.BitMask = ASC_1_RTS_BIT;
    strcpy(UartInitParams[1].CTS.PortName, PioDeviceName[ASC_1_CTS_DEV]);
    UartInitParams[1].CTS.BitMask = ASC_1_CTS_BIT;
    UartInitParams[1].DefaultParams = NULL;

#endif

    
    /* Smartcard1 initialization params */
    strcpy(SmartInitParams.UARTDeviceName, UARTDeviceName_SC[1]);
    SmartInitParams.DeviceType = STSMART_ISO;
    SmartInitParams.DriverPartition = system_partition;
    SmartInitParams.BaseAddress = (U32 *)SMARTCARD1_BASE_ADDRESS;
    strcpy(SmartInitParams.ClkGenExtClk.PortName, "PIO6");
    SmartInitParams.ClkGenExtClk.BitMask = 0;
    strcpy(SmartInitParams.Clk.PortName,  "PIO1");
    SmartInitParams.Clk.BitMask = PIO_BIT_3;
    strcpy(SmartInitParams.Reset.PortName, "PIO1");
    SmartInitParams.Reset.BitMask = PIO_BIT_4;
    strcpy(SmartInitParams.CmdVcc.PortName, "PIO1");
    SmartInitParams.CmdVcc.BitMask = PIO_BIT_5;
    strcpy(SmartInitParams.CmdVpp.PortName, "PIO6");
    SmartInitParams.CmdVpp.BitMask = 0;
    strcpy(SmartInitParams.Detect.PortName, "PIO1");
    SmartInitParams.Detect.BitMask = PIO_BIT_7;
    SmartInitParams.ClockSource = STSMART_CPU_CLOCK;
    SmartInitParams.ClockFrequency = /*ST_GetClockSpeed()*/ClockInfo.CommsBlock;
    SmartInitParams.MaxHandles = 1;
    strcpy(SmartInitParams.EVTDeviceName, EVTDeviceName_SC[1]);    

    EVTInitParams.MemoryPartition = system_partition;
    EVTInitParams.EventMaxNum = STSMART_NUMBER_EVENTS * 2;
    EVTInitParams.ConnectMaxNum = 3;
    EVTInitParams.SubscrMaxNum = STSMART_NUMBER_EVENTS * 2;    
    
   /* UART1 initialization */
#if 1/*add this on 041207*/
    ErrCode = STSMARTUART_Init(UARTDeviceName_SC[1], &UartInitParams);
#else
    ErrCode = STUART_Init(UARTDeviceName_SC[1], &UartInitParams);
#endif

	/* EVT_SC1  initialization */
    ErrCode |= STEVT_Init(EVTDeviceName_SC[1], &EVTInitParams);
	/* SC1  initialization */
    ErrCode |= STSMART_Init(SMARTDeviceName[1], &SmartInitParams);
	/* SC1  Open */  
    /*ErrCode |= STSMART_Open(SMARTDeviceName[1], &SmartOpenParams, &SCHandle);     */
    if (ErrCode != ST_NO_ERROR)
    {
	    do_report(severity_error,"%s,%d>Unable to Init SC1\n",__FILE__,__LINE__);
           return TRUE; 	
    }
    else
    {
           do_report(severity_error,"\n[SCard1_Init] == OK\n");
    }
	
    /* EVT_SC1 Open and Subscribe Smart_card Event */  
    /*ErrCode = STEVT_Open(EVTDeviceName_SC[1], &EVTOpenParams, &EVTHandle);
    ErrCode |= STEVT_Subscribe(EVTHandle,STSMART_EV_CARD_INSERTED,&SubscribeParams);
    ErrCode |= STEVT_Subscribe(EVTHandle,STSMART_EV_CARD_REMOVED,&SubscribeParams);
	
    if (ErrCode != ST_NO_ERROR)
	 return TRUE; 	*/
	
#endif        
	
    return FALSE;
}


/*******************************************************************************
 *Function name: CHCA_CardInit
 * 
 *
 *Description: create the sc process
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_CardInit(void)            
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL  CHCA_CardInit(void)
{
       CHCA_INT      iCardIndex,iCEventIndex;
	/*int                itemp;  */
	   
	CardOperation.SCStatus = CH_EXTRACTED;
	CardOperation.iErrorCode = 0;
	CardOperation.bCardReady = false;




#if 1
       /*init the card instance*/
       for (iCardIndex = 0; iCardIndex < MAX_APP_NUM; iCardIndex++) 	  
       {
            SCOperationInfo[iCardIndex].SCHandle = 0;
            SCOperationInfo[iCardIndex].InUsed = false;

	     SCOperationInfo[iCardIndex].SC_Status= CH_EXTRACTED;

	     SCOperationInfo[iCardIndex].ExtractEvOk = false;	 

	     SCOperationInfo[iCardIndex].ControlLock=semaphore_create_fifo( 1);

            SCOperationInfo[iCardIndex].CardLock.Semaphore=semaphore_create_fifo_timeout(1);

            for(iCEventIndex=0;iCEventIndex<card_max_number_of_callback;iCEventIndex++)
	     	{
	           SCOperationInfo[iCardIndex].ddicard_callbacks[iCEventIndex].CardNotifiy_fun= NULL;	   
	           SCOperationInfo[iCardIndex].ddicard_callbacks[iCEventIndex].enabled= false;	 	 	
            }
       }

	psemPmtOtherAppParseEnd   = semaphore_create_fifo_timeout ( 0 );

       psemAVSeparate = semaphore_create_fifo_timeout ( 0 );
 #endif      


        /*  Create Canal+ smart card process task   */
       do_report (severity_info,"creating SmartCard PROCESS process Prio[%d] Stack[%d]\n",
                          SC_PROCESS_PRIORITY,
                          SC_PROCESS_WORKSPACE );


       if ( ( ptidSCprocessTaskId = Task_Create ( ChSCardProcess,
                                                                         NULL,
                                                                         SC_PROCESS_WORKSPACE,
                                                                         SC_PROCESS_PRIORITY,
                                                                         "ChSCardProcess",
                                                                         0 ) ) == NULL )
		{
	        do_report ( severity_error, "ChCardInit=> Unable to create ChSCardProcess\n" );
			return  true;
		}


	   /*  Create Canal+ smart card process task   */
#ifdef SIMPLE_CA_TRACE
       do_report (severity_info,"[CHCA_CardInit]creating SmartCard PROCESS process Prio[%d] Stack[%d]\n",
                          SC_PROCESS_PRIORITY,
                          SC_PROCESS_WORKSPACE );
       /*itemp =  task_status(ptidSCprocessTaskId,&Status,task_status_flags_stack_used);*/
#endif

    
       return false;        

}



/*******************************************************************************
 *Function name: CHCA_CatDataOperation
 *           
 *
 *Description: process the cat  data
 *             
 *             
 *    
 *Prototype:
 *              CHCA_BOOL   CHCA_CatDataOperation(void)
 * 
 *
 *input:
 *     
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     
 *     
 *     
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
/*add this on 050116*/
CHCA_BOOL   CHCA_CatDataOperation(void)
{
       if(CardOperation.bCardReady)
       {
               while(true)
              {
                     if(ChSCTransStart(MgCardIndex)&&CardOperation.bCardReady)
                     {
                           CHMG_CardTransOperation(CardOperation.iErrorCode,MgCardIndex);
			      if(CardOperation.iErrorCode!=ST_NO_ERROR)	
				  break;	
		       }
		       else
		       {
		             /*do_report(severity_info,"\n CA DATA PROCESS END! \n");*/
		             break;
		       }
	       }
       }		
}
/*******************************************************************************
 *Function name: CHCA_CardSendMess2Usif
 * 
 *
 *Description: send the status of the stb to usif 
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_CardSendMess2Usif (STBStaus  CurStatus)
 *
 *input:
 *      STBStaus  CurStatus  : the stb and card status
 *
 *output:
 *
 *Return Value:
 *       true:  the interface execute error
 *       false: the message send successfully
 *
 *Comments:
 * 
 *******************************************************************************/
 #ifdef CH_IPANEL_MGCA
boolean  CHCA_CardSendMess2Usif (CH_CA2IPANEL_EVT_t  r_evt,U8 *Data,int ri_len)
{
       CHCA_BOOL                              ReturnCode = false;
       CH_MGCA_Ipanel_UpdateCardStatus(r_evt,Data,ri_len);
       return ReturnCode;	  
}

#endif
/*******************************************************************************
 *Function name: CHCA_AppPmtParseDes
 * 
 *
 *Description: Parse the ca descriptor before entering into the application
 *                  
 *
 *Prototype:
 *     CHCA_INT   CHCA_AppPmtParseDes(CHCA_UCHAR*     iPMTBuffer)
 *
 *input:
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *
 *Comments:
 * 
 * 
 *******************************************************************************/
CHCA_INT   CHCA_AppPmtParseDes(CHCA_UCHAR*     iPMTBuffer)
{
        BYTE *                     aucBuffer;
	 CHCA_INT                NoRight = 1; 	
        INT                          iDespLength,iLengthTemp;

        if ( iPMTBuffer == NULL )
            return NoRight;

        aucBuffer = iPMTBuffer + 12;
        iDespLength = iPMTBuffer [10]&0xf << 8 | iPMTBuffer[11];

        while ( iDespLength > 0)
        {
            if ( aucBuffer[0] == 0x9 )
            {
                  NoRight = ChParseCaDescriptor(aucBuffer);

#if 1/*ly add this for multi ca descriptor on 050202*/
		    if(!NoRight)	
		    { 
		         /*do_report(severity_info,"\nThe App has the right\n");*/
                       return NoRight; 
		    }
#endif			
            }			  

            iLengthTemp = aucBuffer [1];
            aucBuffer += (iLengthTemp + 2);
            iDespLength -= (iLengthTemp+2);
        }

        return NoRight;
}





/*******************************************************************************
 *Function name: CHCA_CheckCardReady
 * 
 *
 *Description: check the card ready or not
 *                  
 *
 *Prototype:
 *        CHCA_BOOL  CHCA_CheckCardReady(void)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *      true:  the smart card is ready  
 *      false: the smart card is not ready
 *
 *Comments:
 *     
 * 
 *******************************************************************************/
CHCA_BOOL  CHCA_CheckCardReady(void)
{
       CHCA_BOOL    ReadyOk;

	ReadyOk = CardOperation.bCardReady;

	return  ReadyOk;
	
}



/*******************************************************************************
 *Function name: CHCA_GetCardContent
 * 
 *
 *Description: get the card content (such as card serial number)
 *                  
 *
 *Prototype:
 *     CHCA_BOOL CHCA_GetCardContent(CHCA_UCHAR    *STB_SN)       
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *           false:  the function execution ok
 *           true:  the function execution error
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
BOOL CHCA_GetCardContent(U8    *STB_SN)
{
       CHCA_BOOL    bErrCode = false;

	if(STB_SN==NULL)   
	{
             bErrCode = true;
     	      return  bErrCode;        		 
	}

	memcpy(STB_SN,CaCardContent.CardID.CA_SN_Number,6);	

	  #ifdef ADD_SMART_CONTROL	
        if(!CHCA_CheckCardReady() )
        	{
                 bErrCode = true;
		}
	#endif
	return  bErrCode;  
}


/*******************************************************************************
 *Function name: CHCA_GetOperatorInfo
 * 
 *
 *Description: get the operator information 
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_GetOperatorInfo(OPIInfoStr*  OpiInfo,CHCA_UINT*  OpiNum)          
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *           false:  the function execution ok
 *           true:  the function execution error
 *
 *Comments:
 *    SHORT                     OPINo;                   ::   'OPI Number
 *    U8                           OPIName[16];         ::   OPI name
 *    U8                           geography_zero;     ::    OPI地理区域
 *    SHORT                     Subdate;                ::    定购期限
 *    U8                           CaOffer[8];             ::CA OFFER
 * 2005-03-11   add calling new function MGAPICardUpdate() for update the card content image
 * 
 *******************************************************************************/

BOOL  CHCA_GetOperatorInfo(OPIInfoStr*  OpiInfo,U32*  OpiNum)
{
      CHCA_BOOL         bErrCode = false;
      CHCA_UINT          iOpiIndex;
	  
      if((OpiInfo==NULL)||( OpiNum==NULL))
      {
            bErrCode = true; 
			
            return bErrCode;
      }

#if 1 /*add this on 050311*/
{
        TMGAPIStatus                        iAPIStatus; 
        TMGAPICardHandle                hApiCardHandle=NULL;

        hApiCardHandle = SCOperationInfo[MgCardIndex].iApiCardHandle;

       
	  semaphore_wait(pSemMgApiAccess); /*20060407 add*/
	
        if(hApiCardHandle != NULL)
	 {
	    /*semaphore_wait(pSemMgApiAccess);*/
           /*******zxg 20060402 add************/
           gb_ForceUpdateSmartMirror=true;
           /*******************************/
           iAPIStatus = MGAPICardUpdate(hApiCardHandle);
	      /*semaphore_signal(pSemMgApiAccess);	*/	 
	      do_report(severity_info,"\n[CHCA_GetOperatorInfo==>] MGAPICardUpdate\n");		 
        }	

       /*semaphore_wait(pSemMgApiAccess);*/
       CHCA_CaDataOperation();/*add this on 050521*/		
       /*semaphore_signal(pSemMgApiAccess);*/

	  semaphore_signal(pSemMgApiAccess); /*20060407 add*/

}
#endif


#if 1 /*delete this on 050114*/
      /*semaphore_wait(pSemMgApiAccess);*/
      CHMG_GetOPIInfoList(MgCardIndex);
      /*semaphore_signal(pSemMgApiAccess);*/	   
#endif	  

      *OpiNum = OPITotalNum; 	  

      if(!OPITotalNum)
      {
            return bErrCode;
      }

      for(iOpiIndex=0;iOpiIndex<CHCA_MAX_NO_OPI;iOpiIndex++) 	
      {

 		OpiInfo[iOpiIndex].OPINo=OperatorList[iOpiIndex].OPINum;

              OpiInfo[iOpiIndex].Subdate.TYear = ((OperatorList[iOpiIndex].ExpirationDate[1]& 0xfe)>>1)+1990;
              OpiInfo[iOpiIndex].Subdate.TMonth = ((OperatorList[iOpiIndex].ExpirationDate[1]& 1)<<3)| ((OperatorList[iOpiIndex].ExpirationDate[0]& 0xe0)>>5);
              OpiInfo[iOpiIndex].Subdate.TDay = OperatorList[iOpiIndex].ExpirationDate[0]& 0x1f;
   		memcpy(OpiInfo[iOpiIndex].OPIName,OperatorList[iOpiIndex].OPIName,16);
		memcpy(OpiInfo[iOpiIndex].CaOffer,OperatorList[iOpiIndex].OffersList,8);
		OpiInfo[iOpiIndex].geography_zero =  OperatorList[iOpiIndex].GeoZone;
     }

     /*do_report(severity_info,"\n[CHCA_GetOperatorInfo==>]OpiNum[%d]\n",OPITotalNum);	*/  

      return bErrCode;
}


#ifdef CH_IPANEL_MGCA 

/*******************************************************************************
 *Function name: CHCA_IPanelGetOperatorInfo
 * 
 *
 *Description: get the operator information 
 *                  
 *
 *Prototype:
 *     CHCA_BOOL  CHCA_IPanelGetOperatorInfo(OPIInfoStr*  OpiInfo,CHCA_UINT*  OpiNum)          
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *           false:  the function execution ok
 *           true:  the function execution error
 *
 *Comments:
 *    SHORT                     OPINo;                   ::   'OPI Number
 *    U8                           OPIName[16];         ::   OPI name
 *    U8                           geography_zero;     ::    OPI地理区域
 *    SHORT                     Subdate;                ::    定购期限
 *    U8                           CaOffer[8];             ::CA OFFER
 * 2005-03-11   add calling new function MGAPICardUpdate() for update the card content image
 * 
 *******************************************************************************/

BOOL  CHCA_IPanelGetOperatorInfo(STB_CA_OPERATOR_S*  OpiInfo,U32*  OpiNum)
{
      CHCA_BOOL         bErrCode = false;
      CHCA_UINT          iOpiIndex;
	  
      if((OpiInfo==NULL)||( OpiNum==NULL))
      {
            bErrCode = true; 
			
            return bErrCode;
      }

#if 1 /*add this on 050311*/
{
        TMGAPIStatus                        iAPIStatus; 
        TMGAPICardHandle                hApiCardHandle=NULL;

        hApiCardHandle = SCOperationInfo[MgCardIndex].iApiCardHandle;

       
	  semaphore_wait(pSemMgApiAccess); /*20060407 add*/
	
        if(hApiCardHandle != NULL)
	 {
	    /*semaphore_wait(pSemMgApiAccess);*/
           /*******zxg 20060402 add************/
           gb_ForceUpdateSmartMirror=true;
           /*******************************/
           iAPIStatus = MGAPICardUpdate(hApiCardHandle);
	      /*semaphore_signal(pSemMgApiAccess);	*/	 
	      do_report(severity_info,"\n[CHCA_GetOperatorInfo==>] MGAPICardUpdate\n");		 
        }	

       /*semaphore_wait(pSemMgApiAccess);*/
       CHCA_CaDataOperation();/*add this on 050521*/		
       /*semaphore_signal(pSemMgApiAccess);*/

	  semaphore_signal(pSemMgApiAccess); /*20060407 add*/

}
#endif


#if 1 /*delete this on 050114*/
      /*semaphore_wait(pSemMgApiAccess);*/
      CHMG_GetOPIInfoList(MgCardIndex);
      /*semaphore_signal(pSemMgApiAccess);*/	   
#endif	  

      *OpiNum = OPITotalNum; 	  

      if(!OPITotalNum)
      {
            return bErrCode;
      }

      for(iOpiIndex=0;iOpiIndex<CHCA_MAX_NO_OPI;iOpiIndex++) 	
      {

 		OpiInfo[iOpiIndex].uwOPI =OperatorList[iOpiIndex].OPINum;

              OpiInfo[iOpiIndex].uwDate= OperatorList[iOpiIndex].ExpirationDate[0]|OperatorList[iOpiIndex].ExpirationDate[1]<<8;
      		memcpy(OpiInfo[iOpiIndex].aucName,OperatorList[iOpiIndex].OPIName,16);
		memcpy(OpiInfo[iOpiIndex].aucOffers,OperatorList[iOpiIndex].OffersList,8);
		OpiInfo[iOpiIndex].ucGeo=  OperatorList[iOpiIndex].GeoZone;
     }

     /*do_report(severity_info,"\n[CHCA_GetOperatorInfo==>]OpiNum[%d]\n",OPITotalNum);	*/  

      return bErrCode;
}
#endif
BOOL  CHCA_OnlyGetOperatorInfo(OPIInfoStr*  OpiInfo,U32*  OpiNum)
{
      CHCA_BOOL         bErrCode = false;
      CHCA_UINT          iOpiIndex;
	  
      if((OpiInfo==NULL)||( OpiNum==NULL))
      {
            bErrCode = true; 
			
            return bErrCode;
      }




#if 1 /*delete this on 050114*/
      /*semaphore_wait(pSemMgApiAccess);*/
      CHMG_GetOPIInfoList(MgCardIndex);
      /*semaphore_signal(pSemMgApiAccess);*/	   
#endif	  

      *OpiNum = OPITotalNum; 	  

      if(!OPITotalNum)
      {
            return bErrCode;
      }

      for(iOpiIndex=0;iOpiIndex<CHCA_MAX_NO_OPI;iOpiIndex++) 	
      {

 		OpiInfo[iOpiIndex].OPINo=OperatorList[iOpiIndex].OPINum;

              OpiInfo[iOpiIndex].Subdate.TYear = ((OperatorList[iOpiIndex].ExpirationDate[1]& 0xfe)>>1)+1990;
              OpiInfo[iOpiIndex].Subdate.TMonth = ((OperatorList[iOpiIndex].ExpirationDate[1]& 1)<<3)| ((OperatorList[iOpiIndex].ExpirationDate[0]& 0xe0)>>5);
              OpiInfo[iOpiIndex].Subdate.TDay = OperatorList[iOpiIndex].ExpirationDate[0]& 0x1f;
   		memcpy(OpiInfo[iOpiIndex].OPIName,OperatorList[iOpiIndex].OPIName,16);
		memcpy(OpiInfo[iOpiIndex].CaOffer,OperatorList[iOpiIndex].OffersList,8);
		OpiInfo[iOpiIndex].geography_zero =  OperatorList[iOpiIndex].GeoZone;
     }

     /*do_report(severity_info,"\n[CHCA_GetOperatorInfo==>]OpiNum[%d]\n",OPITotalNum);	*/  

      return bErrCode;
}




/******************************************************************************
 * DDI INTERFACE FUNCTION
 *******************************************************************************/
/*****************************************************************************
 *Function name: CHCA_GetMGCardReaders
 *            
 *
 *Description: This function returns the list of handles of all MG card
 *             readers.
 *
 *Prototype:
 *      CHCA_DDIStatus CHCA_GetMGCardReaders(TCHCASysSCRdrHandle* lhReaders, U8* nReaders) 
 *
 *input:
 *      lhReaders:  Address of the list of MG card reader handle.
 *      nReaders:   Address of the number of MG card readers(maximum 255)
 *
 *output:
 *            
 *            
 *
 *return value
 *      MGDDIOK:         The list of card readers has been filled      
 *      MGDDIBadParam:   The nReaders parameter is incorrect       
 *      MGDDINoResource: The size of the memory block is insufficient      
 *      MGDDIError:      The list of MG card readers is unavailable      
 *************************************************************************/
CHCA_DDIStatus CHCA_GetMGCardReaders(TCHCASysSCRdrHandle* lhReaders, CHCA_UCHAR* nReaders)
{
      CHCA_DDIStatus     StatusMgDdi = CHCADDIOK;

      if(lhReaders != NULL)
      {
             *lhReaders = (TCHCASysSCRdrHandle)SMARTDeviceName[MG_SMARTCARD_INDEX];
	      /*strcpy(*lhReaders,SMARTDeviceName[MG_SMARTCARD_INDEX]);*/
	      *nReaders =  CHCA_MAX_SC_READER;	

#ifdef   CHSC_DRIVER_DEBUG
             do_report(severity_info,"\n The list of card readers has been filled  \n");
#endif
      }
      else
      {
             StatusMgDdi =  CHCADDINoResource;	
#ifdef   CHSC_DRIVER_DEBUG
             do_report(severity_info,"\n The size of the memory block is insufficient  \n");
#endif
			 
      }			 
	  
      return (StatusMgDdi);	  	

}


/*******************************************************************************
 *Function name: CHCA_CardOpen
 * 
 *
 *Description: This function opens the card reader identified by the hReader 
 *                   handle.  
 *
 *Prototype:
 *      U32  CHCA_CardOpen( ST_DeviceName_t  hReader )
 *
 *input:
 *      hReader:   MG card reader system handle
 * 
 *
 *output:
 *
 *Return Value:
 *     Handle of a DDI card reader instance if not null. 
 *     NULL,if the MG card reader system handle is not valid
 *
 *Comments:
 *     1) Exclusivity of the MG card reader does not prevent the use of this function.
 *     2) The fact is that two applications using the same MG card reader must not 
 *        have the same DDI card reader instance handle for this MG card reader.
 *******************************************************************************/
TCHCADDICardReaderHandle  CHCA_CardOpen( TCHCASysSCRdrHandle  hReader )
{
        TCHCADDICardReaderHandle                                  iCardReaderHandle = NULL;
        ST_ErrorCode_t                                                      ErrCode;
        CHCA_UINT                                                            iCardIndex;
	 STSMART_OpenParams_t                                         SCOpenParams; 		

	     if(hReader != NULL)
	     {
                   /*if(!strcmp(hReader,SMARTDeviceName[MG_SMARTCARD_INDEX]))*/
                   if(hReader == (TCHCASysSCRdrHandle)SMARTDeviceName[MG_SMARTCARD_INDEX])
                   {
                         for (iCardIndex=0; iCardIndex<MAX_APP_NUM; iCardIndex++)
			    {
 				    if(!SCOperationInfo[iCardIndex].InUsed)		 	
					break;
                         } 		  

			    if(iCardIndex<MAX_APP_NUM)			 
			    {
			           SCOpenParams.NotifyFunction = NULL;
                       SCOpenParams.BlockingIO = TRUE;
				  
			            ErrCode = STSMART_Open(SMARTDeviceName[MG_SMARTCARD_INDEX],
                                                                           &SCOpenParams,
                                                                           &SCOperationInfo[iCardIndex].SCHandle );

                                 switch(ErrCode)
                                 {
                                        case  ST_NO_ERROR:
                                                  /*iCardReaderHandle = (CHCA_UINT )SCOperationInfo[iCardIndex].SCHandle;*/
							 iCardReaderHandle = iCardIndex + 1;
							 SCOperationInfo[iCardIndex].CardReaderHandle = iCardReaderHandle;				  
					               SCOperationInfo[iCardIndex].InUsed = true;	
                                                  MgCardIndex =  iCardIndex;  /*add this on 040817*/
							 interrupt_lock();
							  /* Initialize the card control data block */
							 SCOperationInfo[iCardIndex].CardExAccessOwner=0;
							 SCOperationInfo[iCardIndex].CardLock.Owner = 0;	
							 strcpy(SCOperationInfo[iCardIndex].CardDeviceName,SMARTDeviceName[MG_SMARTCARD_INDEX]);
							 /*memcpy(SCOperationInfo[iCardIndex].CardDeviceName,SMARTDeviceName[MG_SMARTCARD_INDEX],ST_MAX_DEVICE_NAME);*/
                                                  /*check the status of the card*/							 
				                      /*if(STSMART_GetStatus(SCOperationInfo[i].SCHandle,Status_p) != STSMART_ERROR_NOT_INSERTED)
                                 				SCOperationInfo[i].SC_Status = CH_INSERTED;			 */
                                                  interrupt_unlock();							  
#ifdef                                         CHSC_DRIVER_DEBUG
                                                  do_report(severity_info,"\n The appl get the card reader handle[%d] \n",iCardReaderHandle);
#endif
							 break;

					     case  ST_ERROR_NO_FREE_HANDLES:
#ifdef                                        CHSC_DRIVER_DEBUG
                                                  do_report(severity_info,"\n The appl can't get the handle for 'NO_FREE_HANDLES' \n");
#endif						 	
						 	 break;

					     case  ST_ERROR_UNKNOWN_DEVICE:
#ifdef                                        CHSC_DRIVER_DEBUG
                                                  do_report(severity_info,"\n The appl can't get the handle for 'UNKNOW_DEVICE'\n");
#endif						 	
						 	 break;

					     default:
#ifdef                                        CHSC_DRIVER_DEBUG
                                                  do_report(severity_info,"\n The appl can't get the handle for 'OTHER_ERR' \n");
#endif						 	
						 	 break;
 
     				     }
								 
                          }	
			     else 	
			     {
#ifdef                      CHSC_DRIVER_DEBUG
                                do_report(severity_info,"\n The Num of the appl is larger than the MAX_APP_NUM\n");
#endif
			     }
		     }
		     else
		     {
#ifdef                CHSC_DRIVER_DEBUG
                          do_report(severity_info,"\n Card Sys handle is wrong\n");
#endif
		     }
				   
	     }
	                        
            return (iCardReaderHandle);
 
 }



/*******************************************************************************
 *Function name: CHCA_CardClose
 *
 *
 *Description: this function closes the logical channel to a MG card reader 
 *             identified by the hCard handle
 *
 *Prototype
 *      CHCA_DDIStatus  CHCA_CardClose( U32  hCard)
 *
 *
 *input:
 *      hCard:  DDI card reader instance handle.
 * 
 * 
 *
 *output:
 *
 *Return value:
 *      MGDDIOk :               DDI card reader instance close. 
 *      MGDDIBadParam:     DDI card reader instance handle unknown
 *      CHCADDIError:             Other error.
 *
 * 
 *
 *Comment:
 *       1) if this CHCA card reader instance has locked temporarily the MG card 
 *          reader or have been granted exclusivity to it,the locking or 
 *          exclusivity is automatically canceled
 *       2)Exclusivity of the MG card reader does not prevent the use of this function
 *******************************************************************************/
 CHCA_DDIStatus  CHCA_CardClose( TCHCADDICardReaderHandle  hCard)
 {
         ST_ErrorCode_t                    ErrCode;
         CHCA_DDIStatus                  StatusMgDdi = CHCADDIError;
	  CHCA_UINT                          iCardIndex;

         iCardIndex  = hCard - 1;

	  do_report(severity_info,"\nCHCA_CardClose\n");
	 

	  if(iCardIndex>=MAX_APP_NUM)	  
	  {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n [CHCADDICardClose]Smart card reader handle unknown \n");
#endif				 
                return  (StatusMgDdi);
	   }


          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);

	    /*if this MG card reader instance has locked temporarily the MG card 
             reader or have been granted exclusivity to it,the locking or 
             exclusivity is automatically canceled	  */
          /*if ((SCOperationInfo[iCardIndex].CardExAccessOwner != 0) &&( SCOperationInfo[iCardIndex].SCHandle == SCOperationInfo[iCardIndex].CardExAccessOwner))*/
          if (SCOperationInfo[iCardIndex].CardExAccessOwner != 0)
          {
                 SCOperationInfo[iCardIndex].CardExAccessOwner = 0;
          }

          /*if ((SCOperationInfo[iCardIndex].CardLock.Owner != 0) &&( SCOperationInfo[iCardIndex].SCHandle == SCOperationInfo[iCardIndex].CardLock.Owner))*/
          if (SCOperationInfo[iCardIndex].CardLock.Owner != 0)
          {
                 SCOperationInfo[iCardIndex].CardLock.Owner = 0;
		   if(SCOperationInfo[iCardIndex].CardLock.Semaphore != NULL)		 
		      semaphore_signal(SCOperationInfo[iCardIndex].CardLock.Semaphore);
		   	
          }

	   ErrCode = STSMART_Close(SCOperationInfo[iCardIndex].SCHandle);

	   switch(ErrCode)
	   {
                case ST_NO_ERROR:
			    StatusMgDdi = CHCADDIOK;
#ifdef               CHSC_DRIVER_DEBUG
                        do_report(severity_info,"\n DDI card reader instance close.  \n");
#endif				
			    break;

				
	         case ST_ERROR_INVALID_HANDLE:
			    StatusMgDdi = CHCADDIBadParam;	
#ifdef               CHSC_DRIVER_DEBUG
                         do_report(severity_info,"\n Smart card reader handle unknown \n");
#endif			 	
			    break;

	   }
	   
	   SCOperationInfo[iCardIndex].InUsed = false;
		  
	   semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	  


          return(StatusMgDdi);
 
 }



/*********************************************************************
 *Function name: CHCA_CardSubscribe
 *            
 *
 *Description:This function is used to subscribe to the different 
 *            unexpected events that can be generated by the MG
 *            card reader associated to the hCard handle. 
 *Prototype:
 *   TMGDDIEventSubscriptionHandle  CHCA_CardSubscribe
 *	    (U32  hCard, CALLBACK hCallback)
 *
 *input:
 *      hCard:     DDI card reader instance handle.
 *      hCallback: Callback function handle.
 *
 *output:
 *            
 *            
 *
 *return value
 *      Event subscription handle, if not null.      
 *      NULL, if a problem has occurred when recording.      
 *            - Unknown DDI card reader instance handle. 
 *            - Null callback function address
 *            
 *comment:
 *      1) Exclusivity of the MG card reader dones not prevent the use of 
 *         this function
 *
 *
 *      2) these events are as follows
 *         MGDDIEvCardInsert:       insertion of a smart card in the reader
 *         MGDDIEvCardExtract:      extraction of a smart card from the reader
 *         MGDDIEvCardResetRequest: request to (re)initialize the reader smart card
 *         MGDDIEvCardReset:        (re)initialization of the reader smart card 
 *         MGDDIEvCardProtocol:     change of smart card protocol.
 ***********************************************************************/
 CHCA_BOOL  CHCA_CardSubscribe
	 (TCHCADDICardReaderHandle  hCard, CHCA_CALLBACK_FN  hCallback,CHCA_UCHAR  *SubHandle)
 {
          CHCA_DDIStatus                                  StatusMgDdi = CHCADDIError;
          CHCA_UINT                                          iCardIndex;
          CHCA_UCHAR                                       CardBaseAdd[2];   
          /*CHCA_UCHAR                                       SubscriptionHandle[4]; */

	   ST_ErrorCode_t                                    ErrCode;
          STEVT_OpenParams_t                           EVTOpenParams;
	   /*STEVT_InitParams_t                              EVTInitParams;*/
	   STEVT_Handle_t                                    SCEVTHandle[MAX_APP_NUM];  
          STEVT_SubscribeParams_t                     SubscribeParams = {ChSCEvtNotifyFunc/*,NULL,NULL*/};	  

	   iCardIndex = hCard -1;	  
	   if((hCard==NULL)||(iCardIndex>MAX_APP_NUM)||(hCallback==NULL)||(SubHandle==NULL))
	   {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n [MGDDICardSubscribe::]DDI card reader instance handle unknown or parameter incorrect \n");
#endif				 
                return true;
	   }

 	   semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); 
          /* Initialize each event handler */
          ErrCode = STEVT_Open(EVTDeviceName_SC[MG_SMARTCARD_INDEX], &EVTOpenParams, &SCEVTHandle[iCardIndex]);
          if(ErrCode !=ST_NO_ERROR)
         {
                 do_report(severity_info,"\n [CHCA_CardSubscribe==>] open the stevt failed!\n" );
	          semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 
                 return true;
          } 		

	   SCOperationInfo[iCardIndex].iSCEVTHandle = SCEVTHandle[iCardIndex];

          /*STSMART_EV_CARD_INSERTED*/
          ErrCode= STEVT_Subscribe(SCEVTHandle[iCardIndex],STSMART_EV_CARD_INSERTED,&SubscribeParams);
          if(ErrCode !=ST_NO_ERROR)
          {
                do_report(severity_info,"subscribe event 'STSMART_EV_CARD_INSERTED' failed!!\n" );
	         semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 
                return true;
         }

	   /*STSMART_EV_CARD_REMOVED*/   
          ErrCode = STEVT_Subscribe(SCEVTHandle[iCardIndex],STSMART_EV_CARD_REMOVED,&SubscribeParams);
          if(ErrCode !=ST_NO_ERROR)
          {
                do_report(severity_info,"subscribe event 'STSMART_EV_CARD_REMOVED' failed!!\n" );
	         semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 
                return true;
          }

          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardExtract].enabled =true;
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardExtract].CardNotifiy_fun = hCallback;
		  
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardInsert].enabled =true;
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardInsert].CardNotifiy_fun = hCallback;
		  
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardResetRequest].enabled =true;
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardResetRequest].CardNotifiy_fun = hCallback;
		  
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardReset].enabled =true;
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardReset].CardNotifiy_fun = hCallback;
		  
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardProtocol].enabled =true;
          SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardProtocol].CardNotifiy_fun = hCallback;

          /*card0 baseaddress: high 8 bit is equal to 0xflow 8 bit is equal to cardnum, for example:card 0 is equal 1*/
          CardBaseAdd[0] =  ((CHCA_UCHAR)(iCardIndex+1))&0xff;
          CardBaseAdd[1] =  (CHCA_MGDDI_CARD_BASEADDRESS>>8)&0xff;

                                                                                                                                                      
          /*Card subscriptionhandle: high 16 bit is equal to cardbaseadd,the low 16 bit is equal to scription handle index*/
           SubHandle[3] = CardBaseAdd[1];
           SubHandle[2] = CardBaseAdd[0];
           SubHandle[1] = 0x00;
	    SubHandle[0] = 0x1;
           
	    semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 

#ifdef    CHPROG_DRIVER_DEBUG			  
           do_report(severity_info,"\n %s %d >[CHCA_CardSubscribe::] DDICard_SubEventHandle[%2x][%2x][%2x][%2x] \n",
                            __FILE__,
                            __LINE__,
                           SubHandle[3],
                           SubHandle[2],
                           SubHandle[1],
                           SubHandle[0]);
#endif


         return  false;
	  
 }                           


/*******************************************************************************
 *Function name:  CHCA_CardUnsubscribe
 * 
 *
 *Description:  cancel the subscribe of the card
 *                  
 *
 *Prototype:
 *         CHCA_BOOL CHCA_CardUnsubscribe(CHCA_USHORT    iCardNum)
 *
 *input:
 *      
 * 
 *
 *output:
 *        false:  success to cancel the subscribe
 *        true:   fail to cancel the ctrl subscribe 
 *
 *Return Value:
 *
 *
 *Comments:
 * 
 * 
 *******************************************************************************/
CHCA_BOOL CHCA_CardUnsubscribe(CHCA_USHORT    iCardNum)
{
       CHCA_BOOL            bErrCode=false;
	CHCA_USHORT        iCardIndex;

	iCardIndex = iCardNum - 1;
       if((iCardNum==0)||(iCardIndex>=MAX_APP_NUM))
       {
              bErrCode = true;
	       return bErrCode;		  
	}
	   
       semaphore_wait(SCOperationInfo[iCardIndex].ControlLock); 
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardExtract].enabled = false;
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardExtract].CardNotifiy_fun = NULL;
		  
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardInsert].enabled = false;
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardInsert].CardNotifiy_fun = NULL;
		  
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardResetRequest].enabled = false;
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardResetRequest].CardNotifiy_fun = NULL;
		  
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardReset].enabled = false;
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardReset].CardNotifiy_fun = NULL;
		  
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardProtocol].enabled = false;
       SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardProtocol].CardNotifiy_fun = NULL;
       semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 
 
       return bErrCode;      
}




/*******************************************************************************
 *Function name: CHCA_CardEmpty
 * 
 *
 *Description: This function indicates whether the MG card reader associated to 
 *             the hCard handle has a smart card inserted in it, or not. 
 *
 *Prototype:
 *     CHCA_DDIStatus CHCA_CardEmpty( U32 hCard)
 *
 *input:
 *     hCard:   DDI card reader instance handle.
 * 
 *
 *output:
 *
 *Return Value:
 *      MGDDIOK:              A smart card is present in the MG card reader.      
 *      CHCADDIBadParam:    The DDI card reader instance handle is unknown.       
 *      CHCADDINotFound:     There is no smart card in the CHCA card reader.      
 *      CHCADDIError:           Other error      
 *
 *
 *Comments:
 *      Exclusivity of the MG card reader does not prevent the use of this 
 *      function 
 *******************************************************************************/
 CHCA_DDIStatus CHCA_CardEmpty( TCHCADDICardReaderHandle  hCard)
 {
         /*ST_ErrorCode_t                    ErrCode;*/
         CHCA_DDIStatus                  StatusMgDdi = CHCADDIError;
	  CHCA_UINT                          iCardIndex;	  

         iCardIndex = hCard -1;	   
	  if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM))
	  {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n [CHCA_CardEmpty::]DDI card reader instance handle unknown \n");
#endif				 
               return (StatusMgDdi);
	  }
      
 	  semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);  
         switch(SCOperationInfo[iCardIndex].SC_Status) 
         {
             case CH_EXTRACTED:
			 StatusMgDdi = CHCADDINotFound;	
#ifdef             CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n  There is no smart card in the MG card reader.  \n");
#endif			 	
			break;

	      case CH_INSERTED: 
	      case CH_RESETED:	  	
	      case CH_OPERATIONNAL:
			 StatusMgDdi = CHCADDIOK;	
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n A smart card is present in the MG card reader.  \n");
#endif		  	
		  	break;
	  }
	  semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	  return (StatusMgDdi);
    
 }


/*******************************************************************************
 *Function name: CHCA_CardSetExclusive
 *
 *
 *Description: This function requests the exclusivity of the MG card reader related
 *             to the hCard handle.The exclusivity is granted if no application has 
 *             been granted it.    
 *
 *             if smart card reader exclusivity is granted,no other application than 
 *             the one using the hCard handle will be able to use the reader
 *
 *        
 *Prototype
 *             CHCA_DDIStatus  CHCA_CardSetExclusive( U32  hCard )
 *
 *
 *input:
 *     hCard:   DDI card reader instance handle.
 * 
 * 
 *
 *output:
 *
 *Return value:
 *    MGDDIOk:               The exclusivity has been granted. 
 *    CHCADDIBadParam:    The DDI card reader instance handle is unknown.
 *    CHCADDINoResource:  Exclusive use of the smart card reader has already been 
 *                                  granted.   
 *    CHCADDIError:       Other error.
 * 
 *******************************************************************************/
 CHCA_DDIStatus  CHCA_CardSetExclusive( TCHCADDICardReaderHandle  hCard )
 {
        ST_ErrorCode_t               ErrCode;
        CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
	 CHCA_UINT                     iCardIndex;	  
 
         iCardIndex = hCard -1;	   
	  if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM))
	 {
               StatusMgDdi = CHCADDIBadParam;
#ifdef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n [CHCA_CardSetExclusive::]DDI card reader instance handle unknown \n");
#endif				 
               return (StatusMgDdi);
	 }

        semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);   
	 
        ErrCode = STSMART_Lock(SCOperationInfo[iCardIndex].SCHandle, true);

        switch(ErrCode)
        {
             case  ST_NO_ERROR:
                       StatusMgDdi = CHCADDIOK;
			  SCOperationInfo[iCardIndex].CardExAccessOwner = SCOperationInfo[iCardIndex].SCHandle;
#ifdef             CHSC_DRIVER_DEBUG
                       do_report(severity_info,"\n The exclusivity has been granted \n");
#endif				 
                       break;

             case  ST_ERROR_INVALID_HANDLE:
                      StatusMgDdi = CHCADDIBadParam;
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n DDI card reader instance handle unknown \n");
#endif					
			 break;

	      case  ST_ERROR_DEVICE_BUSY:
                      StatusMgDdi = CHCADDINoResource;
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n Exclusive use of the smart card reader has already been granted \n");
#endif	
		 	 break;

             default :
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n Other error \n");
#endif	
                      break;			 	

	 }
	 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);   	
	 
	 return  (StatusMgDdi);
 }


/*********************************************************************************
 *Function name: CHCA_CardClearExclusive
 *            
 *
 *Description: This function cancels the exclusivity of the MG card reader related 
 *             to the hCard handle. The exclusivity must have been granted previously  
 *             by the mean of MGDDICardSetExclusive interface. 
 *   
 *Prototype:
 *     CHCA_DDIStatus  CHCA_CardClearExclusive( U32  hCard)  
 *          
 *
 *input:
 *
 *output:
 *            
 *            
 *
 *return value
 *       CHCADDIOk:                   The exclusivity has been canceled.     
 *       CHCADDIBadParam:        The DDI card reader instance handle is unknown.    
 *       CHCADDINoResource:      Exclusive use of the reader has been granted to 
 *                                          another application.
 *       CHCADDIError:        Other error(reader is not locked for exclusivity)     
 *********************************************************************************/
CHCA_DDIStatus  CHCA_CardClearExclusive( TCHCADDICardReaderHandle  hCard)
{
        ST_ErrorCode_t               ErrCode;
        CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
	 CHCA_UINT                     iCardIndex;	  
 
         iCardIndex = hCard -1;	   
	  if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM))
	 {
               StatusMgDdi = CHCADDIBadParam;
#ifdef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n [CHCA_CardClearExclusive::] DDI card reader instance handle unknown \n");
#endif				 
               return (StatusMgDdi);
	 }

        semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);   
     
	 ErrCode =  STSMART_Unlock(SCOperationInfo[iCardIndex].SCHandle);

	 switch(ErrCode)
        {
             case  ST_NO_ERROR:
                       StatusMgDdi = CHCADDIOK;
			  SCOperationInfo[iCardIndex].CardExAccessOwner = 0;
#ifdef             CHSC_DRIVER_DEBUG
                       do_report(severity_info,"\n The exclusivity has been canceled \n");
#endif				 
                       break;

            case  ST_ERROR_INVALID_HANDLE:
                      StatusMgDdi = CHCADDIBadParam;
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n DDI card reader instance handle unknown \n");
#endif					
			 break;

	     case  ST_ERROR_BAD_PARAMETER:
                      StatusMgDdi = CHCADDINoResource;
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif	
		 	 break;

             default :
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n Other error \n");
#endif	
                      break;			 	

	 }
	  
        semaphore_signal(SCOperationInfo[iCardIndex].ControlLock); 


	 return (StatusMgDdi);	

}


 CHCA_DDIStatus  CHCA_CardReset( TCHCADDICardReaderHandle  hCard)
 {
        ST_ErrorCode_t               ErrCode;
        CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
 	 CHCA_UINT                     iCardIndex;	  
 
        iCardIndex = hCard -1;	   
	 if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM))
	 {
               StatusMgDdi = CHCADDIBadParam;
#ifdef     CHSC_DRIVER_DEBUG
               do_report(severity_info,"\n[MGDDICardReset::] DDI card reader instance handle unknown \n");
#endif				 
               return (StatusMgDdi);
	 }

	 semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
  
        if(SCOperationInfo[iCardIndex].SC_Status != CH_EXTRACTED)
        {
		 /*execlusively this private data structure*/
              if (SCOperationInfo[iCardIndex].CardExAccessOwner != 0 && SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner)
              {
                    StatusMgDdi = CHCADDINoResource;
				 
#ifdef          CHSC_DRIVER_DEBUG
                    do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif

                    semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		      return  (StatusMgDdi);		 
              } 

              /*flash the transport status of the atr data structure before send the data*/
	      memset(SCOperationInfo[iCardIndex].Read_Smart_Buffer,0,MAX_SCTRANS_BUFFER);

	      SCOperationInfo[iCardIndex].NumberRead = 0;
		  
             SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0] = 0x0;
             SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1] = 0x0; 

             SCOperationInfo[iCardIndex].transport_operation_over = false; 		 
             SCOperationInfo[iCardIndex].pts_operation_over = false;
   
             ErrCode = STSMART_Reset(SCOperationInfo[iCardIndex].SCHandle,
                                                        SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                        &SCOperationInfo[iCardIndex].NumberRead);

             CardOperation.iErrorCode = ErrCode;

#ifdef   CHSC_DRIVER_DEBUG
             do_report(severity_info,"\n The smart card reset errcode[%d]\n",ErrCode);
#endif
			 

	      switch(ErrCode) 
             {
                  case ST_NO_ERROR:
				  	#if 0/*2006080 del for new smart drver*/
/*zxg 2006326 复位成功后得到当前的值*/
                  CH_GetGlobalRxTime(SCOperationInfo[iCardIndex].SCHandle);
                  CH_SetDefaultRxtime(SCOperationInfo[iCardIndex].SCHandle);
				  #endif
/*
{
	ST_ClockInfo_t     ClockInfo; 
	U32 ActualFrequency;
	ST_GetClockInfo(&ClockInfo);
 STSMART_SetClockFrequency(
SCOperationInfo[iCardIndex].SCHandle ,
ClockInfo.SmartCard,
&ActualFrequency);
 do_report(0,"set  ActualFrequency=%d\n",ActualFrequency);
}*/

/****************/
			      StatusMgDdi = CHCADDIOK;	
#ifdef                 CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n The smart card reset request has been accepted\n");
#endif
                           SCOperationInfo[iCardIndex].SC_Status = CH_RESETED;
                           CardOperation.SCStatus = CH_RESETED;
			      break;

                  case  STSMART_ERROR_NOT_INSERTED:
 	                     StatusMgDdi = CHCADDINotFound;
 #ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n Empty CHCA card reader or smart card insert not ready\n");
 #endif		
                            SCOperationInfo[iCardIndex].SC_Status = CH_EXTRACTED;
                            CardOperation.SCStatus = CH_EXTRACTED;
                            break;

		    case  ST_ERROR_BAD_PARAMETER:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for BAD_PARAMETER\n");
#endif		
                            break;
							
	           case  ST_ERROR_NO_MEMORY:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for NO_MEMORY\n");
#endif				   	
			   	break;
				
		    case  ST_ERROR_UNKNOWN_DEVICE:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for UNKNOWN_DEVICE\n");
#endif				
			   	break;
				
		    case  ST_ERROR_ALREADY_INITIALIZED:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for ALREADY_INITIALIZED\n");
#endif				
			   	break;
				
		    case  ST_ERROR_NO_FREE_HANDLES:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for NO_FREE_HANDLES\n");
#endif				
			   	break;
				
		    case  ST_ERROR_OPEN_HANDLE:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for OPEN_HANDLE\n");
#endif				
			   	break;
				
		    case  ST_ERROR_INVALID_HANDLE:
#ifdef                  CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the reset request fail for INVALID_HANDLE\n");
#endif
			   	break;

		    case  ST_ERROR_FEATURE_NOT_SUPPORTED:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for FEATURE_NOT_SUPPORTED\n");
#endif				
			   	break;
				
		    case  ST_ERROR_TIMEOUT:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for TIMEOUT\n");
#endif
			   	break;
				
		    case  ST_ERROR_DEVICE_BUSY:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for DEVICE_BUSY\n");
#endif
                            break;

		    default:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the reset request fail for Unknow reason\n");
#endif
				break;
				
	     }
		  
	 }
	 else
	 {
	         StatusMgDdi = CHCADDINotFound;

#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n Empty CHCA card reader or smart card insert not ready \n");
#endif				 
                 
	 }
        
	 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
	 	

	 return(StatusMgDdi);


 }


/*******************************************************************************
 *Function name: CHCA_CardGetCaps
 *
 *
 *Description: This function indicates the possibilities of the smart card inserted  
 *             in the MG card reader related to the hCard handle.The information is 
 *             placed in the information structure provided at the address given in  
 *             the pCaps parameter.
 *
 *Prototype
 *   CHCA_DDIStatus  CHCA_CardGetCaps
 *             ( U32 hCard, TMGDDICardCaps* pCaps)
 *
 *input:
 *     hCard:    DDI card reader instance handle 
 *     pCaps:    Address of information structure to be filled
 * 
 *
 *output:
 *
 *Return value:
 *     CHCADDIOk:        The protocol information has been recovered.
 *     CHCADDIBadParam:  DDI card reader instance handle unknown or parameter incorrect.
 *     CHCADDINotFound:  Empty MG card reader or smart card inserted not ready.
 *     CHCADDIError:     Information unavailable.
 * 
 *Comments: 
 *     1) Exclusivity of the MG card reader does not prevent the use of this function
 * 
 *******************************************************************************/
 CHCA_DDIStatus  CHCA_CardGetCaps
        ( TCHCADDICardReaderHandle    hCard, TCHCADDICardCaps* pCaps)
 {
         ST_ErrorCode_t                      ErrCode;
         CHCA_DDIStatus                    StatusMgDdi = CHCADDIError;
	  STSMART_Capability_t             card_Capability;
 	  CHCA_UINT                            iCardIndex;	  
 
         iCardIndex = hCard -1;	   
	  if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM)||(pCaps==NULL))
	  {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n [MGDDICardGetCaps::]DDI card reader instance handle unknown or parameter incorrect \n");
#endif				 
                return (StatusMgDdi);
	  }
      
 	  semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);  
	  
	  if((SCOperationInfo[iCardIndex].SC_Status==CH_RESETED) ||(SCOperationInfo[iCardIndex].SC_Status==CH_OPERATIONNAL))
	  {
	        ErrCode = STSMART_GetCapability(SCOperationInfo[iCardIndex].CardDeviceName,
                                                                       &card_Capability);

	        switch(ErrCode)
	        {
                      case ST_NO_ERROR:
			          StatusMgDdi = CHCADDIOK;		
			          pCaps->MaxBaudRate = SCOperationInfo[iCardIndex].MaxBaudRate;
			          pCaps->SupportedISOProtocols = card_Capability.SupportedISOProtocols;
				
#ifdef                     CHSC_DRIVER_DEBUG
                               do_report(severity_info,"\n The protocol information has been recovered \n");
#endif	

			         break;

		        case ST_ERROR_UNKNOWN_DEVICE:
				  StatusMgDdi = CHCADDIBadParam; 	
#ifdef                    CHSC_DRIVER_DEBUG
                              do_report(severity_info,"\n DDI card reader instance handle unknown \n");
#endif		  	
		  	         break;

		  
	        } 
	  }
	  else
	  {
	          StatusMgDdi = CHCADDINotFound;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Empty MG card reader or smart card inserted not ready \n");
#endif		  	

	  }
         semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);  
   
         return (StatusMgDdi);
       
 } 		


/*********************************************************************
 *Function name: CHCA_CardGetProtocol
 *            
 *
 *Description: This function indicates the protocol used by the smart card
 *             inserted in the smart card reader associated to the hCard 
 *             handle.The protocol information used is placed in the protocol
 *             information structure provided at the address given in the 
 *             pProtocol parameter.
 *
 *
 *Prototype:
 *   CHCA_DDIStatus  CHCA_CardGetProtocol
 *         ( U32  hCard, TMGDDICardProtocol* pProtocol)
 *
 *input:
 *     hCard:       DDI card reader instance handle
 *     pProtocol:   Address of protocol information structure to be filled.
 *
 *output:
 *
 *return value
 *     CHCADDIOk:         The protocol information has been recovered.                
 *     CHCADDIBadParam:   DDI card reader instance handle unknown or parameter incorrect      
 *     CHCADDINotFound:   Empty card reader or smart card inserted not ready.       
 *     CHCADDIError:      Protocol information unavailable.       
 *            
 *Commtents:            
 *     1) In the Type field of the MGDDICardProtocol,only one bit is set,       
 *         corresponding to the protocol currently used.     
 *     2) Exclusivity of the MG card reader does not prevent the use of this function       
 ***********************************************************************/
 CHCA_DDIStatus  CHCA_CardGetProtocol
       ( TCHCADDICardReaderHandle  hCard, TCHCADDICardProtocol* pProtocol)
 {
         /*ST_ErrorCode_t               ErrCode;*/
         CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
 	  CHCA_UINT                     iCardIndex;	  
 
         iCardIndex = hCard -1;	   
	  if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM)||(pProtocol==NULL))
	  {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n [MGDDICardGetProtocol::] DDI card reader instance handle unknown or parameter incorrect \n");
#endif				 
                return (StatusMgDdi);
	  }
      
 	  semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);  
	  if((SCOperationInfo[iCardIndex].SC_Status==CH_RESETED) ||(SCOperationInfo[iCardIndex].SC_Status==CH_OPERATIONNAL))
	  {
	          StatusMgDdi = CHCADDIOK;   
		   pProtocol->SupportedProtocolTypes = SCOperationInfo[iCardIndex].FirstT;
		   pProtocol->CurrentBaudRate = SCOperationInfo[iCardIndex].BaudRate;

#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n The protocol information has been recovered \n");
#endif		  	

	  }
	  else
	  {
	          StatusMgDdi = CHCADDINotFound;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Empty MG card reader or smart card inserted not ready \n");
#endif		  	

	  }
         semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);  
   
 	
         return (StatusMgDdi);		
 
 }


/*******************************************************************************
 *Function name: CHCA_CardChangeProtocol
 * 
 *
 *Description: This function is usded to modify the dialog protocol with the smart
 *             card inserted in the MG card reader related to the hCard handle.This  
 *             protocol is specified by the protocol information structure provided 
 *             at the address given by bProtocol parameter.The change in the protocol
 *             is notified to the subscribing instances via th MGDDIEvProtocol event.
 *         
 *Prototype:
 *   CHCA_DDIStatus  CHCA_CardChangeProtocol
 *            ( U32 hCard, TMGDDICardProtocol* pProtocol)
 *
 *input:
 *     hCard:         DDI card reader instance handle.
 *     pProtocol:     Address of information structure of protocol to be used
 *
 *output:
 *
 *Return Value:
 *     MGDDIOk:          The protocol change request will be executed
 *     MGDDIBadParam:    Parameter(s) incorrect.
 *     MGDDINoResource:  Exclusive use of the reader has been granted to another application[3]
 *     MGDDILocked:      The reader is temporarily locked by another application[4]
 *     CHCADDIBusy:        The reader is currently processing a command[4]
 *     CHCADDIProtocol:    Protocol unknown.
 *     CHCADDINotFound:    Empty MG card reader or smart card inserted not ready 
 *     CHCADDIError:       Interface execution error.
 *
 *   
 *Comments:
 *    1) This event must be generated even for the instance that initiated the change in the protocol. 
 *    2) The Type field of the MGDDICardProtocol structure must contain only a single bit,
 *       corresponding to the protocol currently used. 
 *    3) Exclusivity of the MG card reader can prevent the use of this function
 *    4) A locking sequence, a smart card exchange or a protocol change in progress may delay this 
 *       request until the end. This interface returns MGDDIOk in this case
 *
 *******************************************************************************/
 CHCA_DDIStatus  CHCA_CardChangeProtocol
          ( TCHCADDICardReaderHandle hCard, TCHCADDICardProtocol* pProtocol)
 {
          CHCA_UCHAR                                 PPSS,PPS0,PPS1,PCK=0;
          ST_ErrorCode_t                              ErrCode;
          CHCA_DDIStatus                            StatusMgDdi = CHCADDIError;
	   CHCA_UINT                                    iCardIndex,itemp,index;

          iCardIndex = hCard -1;	   
	   if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM)||(pProtocol==NULL))
	   {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n Parameter(s) incorrect \n");
#endif				 
                return (StatusMgDdi);
	   }
      
 	   semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);  
	   /*execlusively this private data structure*/
          if (SCOperationInfo[iCardIndex].CardExAccessOwner != 0 && SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner)
          {
                  StatusMgDdi = CHCADDINoResource;
				 
#ifdef        CHSC_DRIVER_DEBUG
                  do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif

                  semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		    return  (StatusMgDdi);		 
          }    

          if(((SCOperationInfo[iCardIndex].BaudRate == pProtocol->CurrentBaudRate)&&(SCOperationInfo[iCardIndex].FirstT == pProtocol->SupportedProtocolTypes))||\
	       ((pProtocol->CurrentBaudRate > SCOperationInfo[iCardIndex].MaxBaudRate)||(pProtocol->CurrentBaudRate == 0))) 
          {
                StatusMgDdi = CHCADDIBadParam;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n Parameter(s) incorrect \n");
#endif	                  
                semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		  return  (StatusMgDdi);
	   }

	   if(pProtocol->SupportedProtocolTypes!=SCOperationInfo[iCardIndex].FirstT)
	   {
                StatusMgDdi = CHCADDIProtocol;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n Protocol unknown \n");
#endif	                  
                semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		  return  (StatusMgDdi);
	   }

	   if(SCOperationInfo[iCardIndex].NegotiationMode) /*the card support the negotiation operation*/
	   {
                /*mg_pts_request();*/
                PPSS = 0xFF;
                PPS0 = 0x10;

                PPS1 = 0x10 | (SCOperationInfo[iCardIndex].PtsMuxValue & 0xF);     
		 
                /*pts bytes*/
                index = 0;

                SCOperationInfo[iCardIndex].PtsTxBuf[index++] = 0;
                SCOperationInfo[iCardIndex].PtsTxBuf[index++] = PPSS;
                SCOperationInfo[iCardIndex].PtsTxBuf[index++] = PPS0;
                SCOperationInfo[iCardIndex].PtsTxBuf[index++] = PPS1;

                SCOperationInfo[iCardIndex].PtsRxBuf[0] = PPS0;
                SCOperationInfo[iCardIndex].PtsRxBuf[1] = PPS1;

                /* Compute the PCK check byte -- should be XOR of all other bytes */
               for (itemp = 1; itemp < index; itemp++)
                     PCK ^= SCOperationInfo[iCardIndex].PtsTxBuf[itemp];

               SCOperationInfo[iCardIndex].PtsTxBuf[index] = PCK;

               SCOperationInfo[iCardIndex].PtsTxBuf[0] = index;

               /*sent the pps_request to the smartcard*/
               ErrCode=  STSMART_SetProtocol(SCOperationInfo[iCardIndex].SCHandle,
                        		                                SCOperationInfo[iCardIndex].PtsRxBuf);

	        switch(ErrCode)	 
	        {
                      case ST_NO_ERROR:
			         StatusMgDdi = CHCADDIOK;
			         SCOperationInfo[iCardIndex].pts_operation_over = true;	 
#ifdef                    CHSC_DRIVER_DEBUG
                              do_report(severity_info,"\n The protocol change request will be executed \n");
#endif				 
		  	         break;

		        case STSMART_ERROR_NOT_INSERTED:
                      case STSMART_ERROR_NOT_RESET:
			          StatusMgDdi = CHCADDINotFound;	
#ifdef                      CHSC_DRIVER_DEBUG
                               do_report(severity_info,"\n Empty MG card reader or smart card inserted not ready \n");
#endif				   
			          break;

		        case ST_ERROR_DEVICE_BUSY:
			          StatusMgDdi = CHCADDIBusy;	
#ifdef                     CHSC_DRIVER_DEBUG
                               do_report(severity_info,"\n  The reader is currently processing a command \n");
#endif			 	
			          break;

		        case ST_ERROR_INVALID_HANDLE:
		     	          break;	   

			 default:			  
			 	  break;

	        }
	  }

	  semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);  

         return (StatusMgDdi); 

 }




#ifdef    CARD_DATA_TEST
/*******************************************************************************
 *Function name: CHCA_CardSendTest
 *
 *
 *Description: just for test the send function
 *            
 *
 *Prototype
 *CHCA_DDIStatus CHCA_CardSendTest
 * 	  ( U32  hCard,CALLBACK  hCallback, U8* pMsg,U16  Size)
 *
 *input:
 *    hCard:          DDI card reader instance handle.    
 *    hCallback:     Callback function address
 *    pMsg:            Address of memory block providing message to be sent.  
 *    Size:             Message size.
 * 
 *
 *output:
 *
 *Return value:
 *    MGDDIOK:         The message sending request will be executed.
 *    CHCADDIBadParam:   DDI card reader instance handle unknown or parameter incorrect.
 *    CHCADDINoResource: Exclusive use of the reader has been granted to another application
 *    CHCADDILocked:     The reader is temporarily locked by another application
 *    CHCADDIBusy:       The reader is currently processing a command
 *    CHCADDINotFound:   Empty MG card reader or smart card inserted not ready.
 *    CHCADDIError:      Interface execution error. 
 * 
 * 
 *Comments:
 *    1) Exclusivity of the MG card reader can prevent the use of this function
 *    2) A locking sequence, a smart card exchange or a protocol change in progress may 
 *       delay this request until the end.This interface returns MGDDIOK in this case
 * 
 * 
 *******************************************************************************/
CHCA_DDIStatus CHCA_CardSendTest
	  (CHCA_UCHAR* pMsg,CHCA_USHORT  Size)
 {

 
          ST_ErrorCode_t               ErrCode;
          CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
	   CHCA_UINT                     iCardIndex,j,SizeSent,SizeRead;	  
	   
	   SizeSent = 	Size;   
	   SizeRead =  0;

          iCardIndex = 0;	

#if 0		  
	   if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM)||(SizeSent==0)||(SizeSent>=MAX_SCTRANS_BUFFER)||(pMsg==NULL))
	   {
                 StatusMgDdi = CHCADDIBadParam;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n [CHCA_CardSend::]DDI card reader instance handle unknown \n");
#endif				 
                 return  (StatusMgDdi);
	   }
#endif	   

          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);

          if (SCOperationInfo[iCardIndex].CardExAccessOwner != 0 && SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner)
          {
                 StatusMgDdi = CHCADDINoResource;
				 
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		   return  (StatusMgDdi);		 
          } 
		  
          /*flash the transport status of the data structure before send the data*/
	   memset(SCOperationInfo[iCardIndex].Write_Smart_Buffer, 0, MAX_SCTRANS_BUFFER);	  
          memset(SCOperationInfo[iCardIndex].Read_Smart_Buffer, 0, MAX_SCTRANS_BUFFER);

	   SCOperationInfo[iCardIndex].NumberWritten = 0;	  
          SCOperationInfo[iCardIndex].NumberRead = 0;
		  
          SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0] = 0x0;
          SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1] = 0x0;	  

	    /*copy the message to be sent to the write buffer*/
          memcpy(SCOperationInfo[iCardIndex].Write_Smart_Buffer,pMsg,SizeSent);

          if(SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] == 0)
              SizeRead = MAX_SCTRANS_BUFFER;

          if(SizeSent== 5)
              SizeRead = SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] ;


#ifdef       CHSC_DRIVER_DEBUG
           do_report(severity_info,"\nCHCA_CardSend size[%d] readsize[%d]\n",SizeSent,SizeRead);
           for(j=0;j<SizeSent;j++)   
                 do_report(severity_info,"%4x",pMsg[j]);
	    do_report(severity_info,"\n");	   
#endif


          ErrCode = STSMART_Transfer( SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Write_Smart_Buffer,
                                                           SizeSent,
                                                           &SCOperationInfo[iCardIndex].NumberWritten,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                           SizeRead,
                                                           &SCOperationInfo[iCardIndex].NumberRead,
                                                           &SCOperationInfo[iCardIndex].SmartCardStatus);
#if 0/*2006080 del for new smart driver*/
          /*zxg 20060326 add 复位后第一次为1000ms,恢复后面RXtime时间*/
          if(CH_RestoreRxtime(SCOperationInfo[iCardIndex].SCHandle)==true&&
		 ErrCode!=ST_NO_ERROR)
          	{
                     ErrCode = STSMART_Transfer( SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Write_Smart_Buffer,
                                                           SizeSent,
                                                           &SCOperationInfo[iCardIndex].NumberWritten,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                           SizeRead,
                                                           &SCOperationInfo[iCardIndex].NumberRead,
                                                           &SCOperationInfo[iCardIndex].SmartCardStatus);

          	}
          /********************************************************/
		  #endif
          do_report(severity_info,"\n[MGDDICardSend::]Trans status: sw1[%d],sw2[%d] ErrorCode[%d]\n",\
		  	                           SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0],\
                                                SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1],\
                                                ErrCode);

           CardOperation.iErrorCode= ErrCode;  

           SCOperationInfo[iCardIndex].transport_operation_over = true; /*add this on 040817*/


#ifdef       CHSC_DRIVER_DEBUG
           for(j=0;j<SCOperationInfo[iCardIndex].NumberRead;j++)   
                 do_report(severity_info,"%4x",SCOperationInfo[iCardIndex].Read_Smart_Buffer[j]);
	    do_report(severity_info,"\n");	   
#endif


#ifdef    CHSC_DRIVER_DEBUG
          do_report(severity_info,"\n STSMART_Transfer end errcode[%d]\n",ErrCode);
#endif

           switch(ErrCode) 
           {
                  case ST_NO_ERROR:
			      StatusMgDdi = CHCADDIOK;	
#ifdef                 CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the communication success between sc and the stb\n");
#endif
                           break;

                  case  STSMART_ERROR_NOT_INSERTED:
	            case  STSMART_ERROR_NOT_RESET:  	
                           StatusMgDdi = CHCADDINotFound;
#ifdef                  CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the communication fail for no card or card inserted not ready\n");
#endif							
				 break;

		    case  ST_ERROR_BAD_PARAMETER:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for BAD_PARAMETER\n");
#endif					
                            break;
							
	           case  ST_ERROR_NO_MEMORY:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for NO_MEMORY\n");
#endif				   	
			   	break;
				
		    case  ST_ERROR_UNKNOWN_DEVICE:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for UNKNOWN_DEVICE\n");
#endif				
			   	break;
				
		    case  ST_ERROR_ALREADY_INITIALIZED:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for ALREADY_INITIALIZED\n");
#endif				
			   	break;
				
		    case  ST_ERROR_NO_FREE_HANDLES:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for NO_FREE_HANDLES\n");
#endif				
			   	break;
				
		    case  ST_ERROR_OPEN_HANDLE:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for OPEN_HANDLE\n");
#endif				
			   	break;
				
		    case  ST_ERROR_INVALID_HANDLE:
                            StatusMgDdi = CHCADDIBadParam;
#ifdef                  CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the communication fail for INVALID_HANDLE\n");
#endif
			   	break;

		    case  ST_ERROR_FEATURE_NOT_SUPPORTED:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for FEATURE_NOT_SUPPORTED\n");
#endif				
			   	break;
				
		    case  ST_ERROR_TIMEOUT:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for TIMEOUT\n");
#endif
			   	break;
				
		    case  ST_ERROR_DEVICE_BUSY:
				StatusMgDdi = CHCADDIBusy;
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for DEVICE_BUSY\n");
#endif
				break;
				
                 default:
#ifdef                CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n Unknow error,errcode[%d]\n",ErrCode);
#endif
			      break;	 	
				
	    }


	    semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	   

           return  (StatusMgDdi);
 }
#endif



/*******************************************************************************
 *Function name: CHCA_CardSend
 *
 *
 *Description: This function is used to send a command to the smart card inserted 
 *             in the smart card reader related to the hCard handle.The command is  
 *             contained in a memory block provided at the address given in the 
 *             pMsg parameter.The size this memory block is defined by the Size  
 *             parameter.The message report is notified later,by means of the  
 *             notification function provided at the address given in the hCallback 
 *             parameter,and via the MGDDIEvCardReport event.The processing command
 *             can be cancelled before completion,via the MGDDICardAbort interface,
 *             by the application handling the hCard handle.
 *
 *             No assumptions have been made as to the protocol used by the smart card.       
 *             This interface must enable a command of variable length to be sent.If the
 *             Smart card recognizes the command,it returns an acknowledgement of variable
 *             length to be sent.If the smart card recognizes the command,it returns an 
 *             acknowledgement of variable length that is notified immediately on reception.
 *
 *            
 *
 *Prototype
 *CHCA_DDIStatus CHCA_CardSend
 * 	  ( U32  hCard,CALLBACK  hCallback, U8* pMsg,U16  Size)
 *
 *input:
 *    hCard:          DDI card reader instance handle.    
 *    hCallback:     Callback function address
 *    pMsg:            Address of memory block providing message to be sent.  
 *    Size:             Message size.
 * 
 *
 *output:
 *
 *Return value:
 *    MGDDIOK:         The message sending request will be executed.
 *    CHCADDIBadParam:   DDI card reader instance handle unknown or parameter incorrect.
 *    CHCADDINoResource: Exclusive use of the reader has been granted to another application
 *    CHCADDILocked:     The reader is temporarily locked by another application
 *    CHCADDIBusy:       The reader is currently processing a command
 *    CHCADDINotFound:   Empty MG card reader or smart card inserted not ready.
 *    CHCADDIError:      Interface execution error. 
 * 
 * 
 *Comments:
 *    1) Exclusivity of the MG card reader can prevent the use of this function
 *    2) A locking sequence, a smart card exchange or a protocol change in progress may 
 *       delay this request until the end.This interface returns MGDDIOK in this case
 * 
 * 2005-3-10   delete the wrong process 
 * 
 *******************************************************************************/
 CHCA_DDIStatus CHCA_CardSend
	  ( TCHCADDICardReaderHandle  hCard,CHCA_CALLBACK_FN   hCallback,CHCA_UCHAR* pMsg,CHCA_USHORT  Size)
 {
          ST_ErrorCode_t               ErrCode;
          CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
	   CHCA_UINT                     iCardIndex,SizeSent,SizeRead,j;	  

	   EMMDataRightUpdated = false;
	   SizeSent = 	Size;   
	   SizeRead =  0;

          iCardIndex = hCard -1;	   
	   if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM)||(SizeSent==0)||(SizeSent>=MAX_SCTRANS_BUFFER)||(pMsg==NULL))
	   {
                 StatusMgDdi = CHCADDIBadParam;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n [CHCA_CardSend::]DDI card reader instance handle unknown \n");
#endif				 
                 return  (StatusMgDdi);
	   }

          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
          if (SCOperationInfo[iCardIndex].CardExAccessOwner != 0 && SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner)
          {
                 StatusMgDdi = CHCADDINoResource;
				 
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n[CHCA_CardSend==>]Exclusive use of the reader has been granted to another application \n");
#endif
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		   return  (StatusMgDdi);		 
          } 
		  
          /*flash the transport status of the data structure before send the data*/
	   memset(SCOperationInfo[iCardIndex].Write_Smart_Buffer, 0, MAX_SCTRANS_BUFFER);	  
          memset(SCOperationInfo[iCardIndex].Read_Smart_Buffer, 0, MAX_SCTRANS_BUFFER);

	   SCOperationInfo[iCardIndex].NumberWritten = 0;	  
          SCOperationInfo[iCardIndex].NumberRead = 0;
		  
          SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0] = 0x0;
          SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1] = 0x0;	  

	    /*copy the message to be sent to the write buffer*/
          memcpy(SCOperationInfo[iCardIndex].Write_Smart_Buffer,pMsg,SizeSent);
#if 0/*zxg 20060812 change*/
          if(SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] == 0)
              SizeRead = MAX_SCTRANS_BUFFER;

          if(SizeSent== 5)
              SizeRead = SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] ;

#else
       if(SizeSent<=4)/*第一种APDU结构*/
       	{
                 SizeRead=0;
       	}
	   else if(SizeSent==5)/*第二种APDU结构*/
	   	{
                  SizeRead=SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] ;
		        if(SizeRead == 0)
                            SizeRead = MAX_SCTRANS_BUFFER;
	   	}
	   	else/*第三、四种APDU结构*/
	   	{
                    if(SizeSent==(SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] +5))
			/*第三APDU结构*/
                    {
                        SizeRead=0;                  
                    }
			else/*四种APDU结构*/
			{
                       SizeRead=SCOperationInfo[iCardIndex].Write_Smart_Buffer[SizeSent-1];
			     if(SizeRead == 0)
                            SizeRead = MAX_SCTRANS_BUFFER;

			}
	   	}
        

#endif

#ifdef     CHSC_DRIVER_DEBUG
           do_report(severity_info,"\n[CHCA_CardSend==>]size[%d] readsize[%d]\n",SizeSent,SizeRead);
           for(j=0;j<SizeSent;j++)   
                 do_report(severity_info,"0x%x,",pMsg[j]);
	    do_report(severity_info,"\n");	   
#endif

	   SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardReport].CardNotifiy_fun= hCallback;	   
	   SCOperationInfo[iCardIndex].ddicard_callbacks[MGDDIEvCardReport].enabled= true;	   
          /*semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	*/

          ErrCode = STSMART_Transfer( SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Write_Smart_Buffer,
                                                           SizeSent,
                                                           &SCOperationInfo[iCardIndex].NumberWritten,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                           SizeRead,
                                                           &SCOperationInfo[iCardIndex].NumberRead,
                                                           &SCOperationInfo[iCardIndex].SmartCardStatus);
	if(ErrCode != ST_NO_ERROR)
	{
		do_report(0, "STSMART_Transfer return %s\n", GetErrorText(ErrCode));
	}
#if 0/*20060808 del for new smart driver*/
 /*zxg 20060326 add 复位后第一次为1000ms,恢复后面RXtime时间*/
          if(CH_RestoreRxtime(SCOperationInfo[iCardIndex].SCHandle)==true&&
		 ErrCode!=ST_NO_ERROR)/*如果失败,重新传输*/
          	{
                     ErrCode = STSMART_Transfer( SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Write_Smart_Buffer,
                                                           SizeSent,
                                                           &SCOperationInfo[iCardIndex].NumberWritten,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                           SizeRead,
                                                           &SCOperationInfo[iCardIndex].NumberRead,
                                                           &SCOperationInfo[iCardIndex].SmartCardStatus);

          	}
          /********************************************************/
#endif	



	   /*add this on 050227*/
          if((SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0]==0x90) && \
		(SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1]==0x19))
          {
                    EMMDataRightUpdated = true;
                    do_report(severity_info,"\n[CHCA_CardSend==>]New emm is comming,EMMDataRightUpdated[%d]\n",EMMDataRightUpdated); 				   
	   }
		  #if 1
	/*20060811 add for ERROR handle OPI 147*/
         else if((SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0]==0x90) &&
	      (SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1]==0x02))
         	{
         	    
				  
                   do_report(0,"\n[CHCA_CardSend==>] Tranfer Error PB[1]=%d\n",SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1]); 
		   /*  ErrCode=STSMART_INVALID_STATUS_BYTE;*/
#if 0
			   ErrCode = STSMART_Transfer( SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Write_Smart_Buffer,
                                                           SizeSent,
                                                           &SCOperationInfo[iCardIndex].NumberWritten,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                           SizeRead,
                                                           &SCOperationInfo[iCardIndex].NumberRead,
                                                           &SCOperationInfo[iCardIndex].SmartCardStatus);
#endif
		 }
		 #endif
/************************************/
         /* do_report(severity_info,"\n[CHCA_CardSend==>]Trans status: sw1[%4x],sw2[%4x] ErrorCode[%d]\n",SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0],\
                                                SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1],\
                                                ErrCode);*/

           if(ErrCode)
           {
                 do_report(severity_info,"\n[MGDDICardSend==>]Trans status: sw1[%4x],sw2[%4x] ErrorCode[%d]\n",SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0],\
                                                SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1],\
                                                ErrCode);
#if 0	/*zxg 20060808 add for error reset and retry*/	   
		   
#if 0
                 task_delay(ST_GetClocksPerSecond()/20);
                 ErrCode = STSMART_Reset(SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                          &SCOperationInfo[iCardIndex].NumberRead);

		  if(ErrCode == ST_NO_ERROR)		
#endif		  	
		  {
		  #if 0
                      /*flash the transport status of the data structure before send the data*/
                      memset(SCOperationInfo[iCardIndex].Write_Smart_Buffer, 0, MAX_SCTRANS_BUFFER);	  
                      memset(SCOperationInfo[iCardIndex].Read_Smart_Buffer, 0, MAX_SCTRANS_BUFFER);
                      
                      SCOperationInfo[iCardIndex].NumberWritten = 0;	  
                      SCOperationInfo[iCardIndex].NumberRead = 0;
                      
                      SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[0] = 0x0;
                      SCOperationInfo[iCardIndex].SmartCardStatus.StatusBlock.T0.PB[1] = 0x0;	  
                      
                      /*copy the message to be sent to the write buffer*/
                      memcpy(SCOperationInfo[iCardIndex].Write_Smart_Buffer,pMsg,SizeSent);

			 SizeRead = 0;	  

                      if(SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] == 0)
                          SizeRead = MAX_SCTRANS_BUFFER;
            
                      if(SizeSent == 5)
                          SizeRead = SCOperationInfo[iCardIndex].Write_Smart_Buffer[4] ;
					  
			  task_delay(ST_GetClocksPerSecond()/20);	
			  #endif
                      ErrCode = STSMART_Transfer( SCOperationInfo[iCardIndex].SCHandle,
                                             SCOperationInfo[iCardIndex].Write_Smart_Buffer,
                                             SizeSent,
                                             &SCOperationInfo[iCardIndex].NumberWritten,
                                             SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                             SizeRead,
                                             &SCOperationInfo[iCardIndex].NumberRead,
                                             &SCOperationInfo[iCardIndex].SmartCardStatus);

                      do_report(severity_info,"\n STSMART_Transfer again errcode[%d] \n",ErrCode);
					  
		  }
#endif		  
	    }

           CardOperation.iErrorCode= ErrCode;  

           SCOperationInfo[iCardIndex].transport_operation_over = true; /*add this on 040817*/
		   
           
	    semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	   
		   

           switch(ErrCode) 
           {
                  case ST_NO_ERROR:
			      StatusMgDdi = CHCADDIOK;	
#ifdef                 CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the communication success between sc and the stb\n");
#endif
                           break;

                  case  STSMART_ERROR_NOT_INSERTED:
	            case  STSMART_ERROR_NOT_RESET:  	
                           StatusMgDdi = CHCADDINotFound;
#ifdef                  CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the communication fail for no card or card inserted not ready\n");
#endif							
				 break;

		    case  ST_ERROR_BAD_PARAMETER:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for BAD_PARAMETER\n");
#endif					
                            break;
							
	           case  ST_ERROR_NO_MEMORY:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for NO_MEMORY\n");
#endif				   	
			   	break;
				
		    case  ST_ERROR_UNKNOWN_DEVICE:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for UNKNOWN_DEVICE\n");
#endif				
			   	break;
				
		    case  ST_ERROR_ALREADY_INITIALIZED:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for ALREADY_INITIALIZED\n");
#endif				
			   	break;
				
		    case  ST_ERROR_NO_FREE_HANDLES:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for NO_FREE_HANDLES\n");
#endif				
			   	break;
				
		    case  ST_ERROR_OPEN_HANDLE:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for OPEN_HANDLE\n");
#endif				
			   	break;
				
		    case  ST_ERROR_INVALID_HANDLE:
                            StatusMgDdi = CHCADDIBadParam;
#ifdef                  CHSC_DRIVER_DEBUG
                           do_report(severity_info,"\n the communication fail for INVALID_HANDLE\n");
#endif
			   	break;

		    case  ST_ERROR_FEATURE_NOT_SUPPORTED:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for FEATURE_NOT_SUPPORTED\n");
#endif				
			   	break;
				
		    case  ST_ERROR_TIMEOUT:
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for TIMEOUT\n");
#endif
			   	break;
				
		    case  ST_ERROR_DEVICE_BUSY:
				StatusMgDdi = CHCADDIBusy;
#ifdef                  CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n the communication fail for DEVICE_BUSY\n");
#endif
				break;
				
                 default:
			     /*20060723 add reset unkonow error*/
			     do_report(0,"zxg other  card transfer errror\n");

#if 0/*20060816 add*/
                 task_delay(ST_GetClocksPerSecond()/20);
                 ErrCode = STSMART_Reset(SCOperationInfo[iCardIndex].SCHandle,
                                                           SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                          &SCOperationInfo[iCardIndex].NumberRead);
                  task_delay(ST_GetClocksPerSecond()/20);

#endif
			     /*******************************/
#ifdef                CHSC_DRIVER_DEBUG
                            do_report(severity_info,"\n Unknow error,errcode[%d]\n",ErrCode);
#endif
			      break;	 	
				
	    }

           return  (StatusMgDdi);
 }


/*********************************************************************
 *Function name: CHCA_CardAbort
 *            
 *
 *Description: this interface cancels the exchange processed by the 
 *             smart card inserted in the MG card reader related to the             
 *             hCard handle.If the MG card reader is locked by the DDI
 *             card reader instance aborting,the locking sequence is  
 *             cancelled.
 * 
 *Prototype:
 *      CHCA_DDIStatus CHCA_CardAbort( U32  hCard)    
 *          
 *
 *input:
 *     hCard:    DDI card reader instance handle
 *
 *output:
 *            
 *return value
 *     MGDDIOk:          The exchange processed by the reader smart card is cancelled       
 *     CHCADDIBadParam:    DDI card reader instance handle unknown.      
 *     CHCADDINoResource:  Exclusive use of the reader has been granted to another       
 *                       application 
 *     CHCADDILocked:      The reader is temporarily locked by another application       
 *     CHCADDIBusy:        The reader is currently processing a command of another        
 *                       application 
 *     CHCADDINotFound:    Empty reader or smart card inserted not ready.       
 *     CHCADDIError:       Interface execution error(no exchange currently processed).
 *
 *comments:
 *     Exclusivity of the MG card reader can prevent the use of this fucntion  
 *
 ***********************************************************************/
  CHCA_DDIStatus CHCA_CardAbort( TCHCADDICardReaderHandle  hCard)
 {
          ST_ErrorCode_t               ErrCode;
          CHCA_DDIStatus             StatusMgDdi = CHCADDIError;
	   CHCA_UINT                     iCardIndex;


          iCardIndex = hCard -1;	   
	   if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM))
	   {
                 StatusMgDdi = CHCADDIBadParam;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n [CHCA_CardAbort::]Smart card reader handle unknown \n");
#endif				 
                 return  (StatusMgDdi);
	   }

          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
          if ((SCOperationInfo[iCardIndex].CardExAccessOwner != 0) &&( SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner))
          {
                 StatusMgDdi = CHCADDINoResource;
		 
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	
		   return  (StatusMgDdi);		 
          }

          if((SCOperationInfo[iCardIndex].SC_Status==CH_RESETED) ||(SCOperationInfo[iCardIndex].SC_Status==CH_OPERATIONNAL))
          { 
#if 0           /*delete this on 041129*/    
                  if((SCOperationInfo[iCardIndex].CardLock.Owner != 0)&&(SCOperationInfo[iCardIndex].CardLock.Owner != SCOperationInfo[iCardIndex].SCHandle))
                  {
	                  StatusMgDdi = CHCADDINotFound;
#ifdef               CHSC_DRIVER_DEBUG
                         do_report(severity_info,"\n The reader is temporarily locked by another application \n");
#endif			 
                   	    semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
                         return  (StatusMgDdi);
		    }
#endif				  
				  
		    if(SCOperationInfo[iCardIndex].CardLock.Owner!=0)
                  {
                       SCOperationInfo[iCardIndex].CardLock.Owner = 0;
			  if(SCOperationInfo[iCardIndex].CardLock.Semaphore != NULL)		   
	                    semaphore_signal(SCOperationInfo[iCardIndex].CardLock.Semaphore);
		    }			

                  ErrCode = STSMART_Abort(SCOperationInfo[iCardIndex].SCHandle);

		    switch(ErrCode)	
		    {
                          case ST_NO_ERROR:
                                   StatusMgDdi = CHCADDIOK;
#ifdef                         CHSC_DRIVER_DEBUG
                                   do_report(severity_info,"\n The exchange processed by the reader smart card is cancelled \n");
#endif										   
					break;
                          case STSMART_ERROR_NOT_INSERTED:
			     case STSMART_ERROR_NOT_RESET:
	 			       StatusMgDdi = CHCADDINotFound;
#ifdef                         CHSC_DRIVER_DEBUG
                                   do_report(severity_info,"\n Empty reader or smart card inserted not ready \n");
#endif										   
					break;

                          case  ST_ERROR_INVALID_HANDLE:
				        StatusMgDdi = CHCADDIBadParam;
#ifdef                          CHSC_DRIVER_DEBUG
                                    do_report(severity_info,"\n Smart card reader handle unknown \n");
#endif										   
					 break;		  	

		    }
 				
          }
	   else
	   {
	         StatusMgDdi = CHCADDINotFound;
#ifdef      CHSC_DRIVER_DEBUG
                do_report(severity_info,"\n Empty reader or smart card inserted not ready \n");
#endif			 
	   }	  
	   semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
	   return (StatusMgDdi);
 
 }

 
/*******************************************************************************
 *Function name: CHCA_CardLock
 * 
 *
 *Description: This interface is used to send a continuous sequence of commands
 *             to the card. The group of commands to be strung together begin
 *             with a MGDDICardLock call and end with a MGDDICardUnlock call.
 *             The LockTime parameter indicates the time during which access to
 *             The MG card reader must be locked out temporarily for the 
 *             application that handles the hCard handle.This time is expressed in ms. 
 *
 *             While locked out,only the application that handles the hCard handles 
 *             can retransmit through this MG card reader.Temporary locking is 
 *             automatically cancelled by the device driver if no new command is  
 *             sent to the card after a timeout defined by the LockTime parameter on 
 *             completion of reception of the response of the last chained command. 
 *             This automatic cancellation is notified by the callback function provided 
 *             at the address given in the hCallback parameter via the MGDDIEvCardLockTimeout
 *             event.The locking sequence can be cancelled by the lock owner at any time 
 *             with MGDDICardUnlock interface.
 *
 *
 *Prototype:
 *  CHCA_DDIStatus   CHCA_CardLock
 *   	  ( U32  hCard,CALLBACK  hCallback,unsigned long  LockTime)  
 *
 *input:
 *     hCard:          DDI card reader instance handle.       
 *     hCallback:      Callback function address.
 *     LockTime:       Maximum duration of MG card reader locking.
 *
 *
 *output:
 *
 *Return Value:
 *     CHCADDIOk:          Access to the smart card reader is unlocked.
 *     CHCADDIBadParam:    Smart card reader handle unknown
 *     CHCADDINoResource:  Exclusive use of the reader has been granted to another application
 *     CHCADDILocked:      The reader is temporary locked by another application
 *     CHCADDIBusy:        The reader is currently processing a command.
 *     CHCADDINotFound:    Empty reader or smart card inserted not ready.
 *     CHCADDIError:       Interface execution error.
 *
 *
 *
 *Comments:
 *     1) Exclusivity of the MG card reader can prevent the use of this function 
 *     2) A locking sequence, a smart card exchange or a protocol change in progress may
 *        delay this request until their completion.This interface returns MGDDIOk in 
 *        this case
 *      The MGDDLocked code is returned only if locking is requested a second time by the same DDI 
 *     instance without having prevously unlocked the intended MG card reader
 *******************************************************************************/
 CHCA_DDIStatus   CHCA_CardLock
	  ( TCHCADDICardReaderHandle  hCard,CHCA_CALLBACK_FN  hCallback,CHCA_ULONG  LockTime)  
 {
          /*ST_ErrorCode_t                     ErrCode;*/
          CHCA_DDIStatus                   StatusMgDdi = CHCADDIError;
	   CHCA_UINT                           iCardIndex;
	   CHCA_TICKTIME                    CardLock_time;

          /*do_report(severity_info,"\n [CHCA_CardLock::]\n");

	   StatusMgDdi = CHCADDIOK;
	   return StatusMgDdi;*/


          iCardIndex = hCard -1;	   
	   if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM)|| (hCallback==NULL)||(LockTime==0))
	   {
                 StatusMgDdi = CHCADDIBadParam;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n [CHCA_CardLock::]Smart card reader handle unknown \n");
#endif				 
                 return  (StatusMgDdi);
	   }

          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
          if ((SCOperationInfo[iCardIndex].CardExAccessOwner != 0) &&( SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner))
          {
                 StatusMgDdi = CHCADDINoResource;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	
		   return  (StatusMgDdi);		 
          }

         if((SCOperationInfo[iCardIndex].CardLock.Owner != 0 )&&( SCOperationInfo[iCardIndex].SCHandle == SCOperationInfo[iCardIndex].CardLock.Owner))
          {
                 StatusMgDdi = CHCADDILocked;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n The reader is temporary locked by another application \n");
#endif
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	
		   return  (StatusMgDdi);		 
          }
 
          if((SCOperationInfo[iCardIndex].SC_Status==CH_RESETED) ||(SCOperationInfo[iCardIndex].SC_Status==CH_OPERATIONNAL))
          {
               CardLock_time = time_plus(time_now(),(clock_t)ST_GetClocksPerSecond()*0.001*LockTime);

		 /*CardLock_time = CHCA_ClockADD(LockTime);*/

               SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardLockTimeout].enabled = true;
	        SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardLockTimeout].CardNotifiy_fun = hCallback;


 	        if(semaphore_wait_timeout(SCOperationInfo[iCardIndex].CardLock.Semaphore,&CardLock_time)==0) 
               {
                      StatusMgDdi = CHCADDIOK;
		        SCOperationInfo[iCardIndex].CardLock.Owner =  SCOperationInfo[iCardIndex].SCHandle;
			 
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\nIn %s,%d,Smart Card Command Information OK",__FILE__,__LINE__);
#endif
	        }
	        else
	        {
                     if((SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardLockTimeout].enabled)&&\
			  (SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardLockTimeout].CardNotifiy_fun!=NULL))
		       {
		            SCOperationInfo[iCardIndex].ddicard_callbacks[CHCADDIEvCardLockTimeout].CardNotifiy_fun((void*)SCOperationInfo[iCardIndex].SCHandle,
				 	                                                                                                                                    CHCADDIEvCardLockTimeout,
				 	                                                                                                                                    NULL,
				 	                                                                                                                                    NULL);
                     }	   
			
#ifdef           CHSC_DRIVER_DEBUG			
	              do_report(severity_info,"\nIn %s,%d,waiting for the Command Infor time out!",__FILE__,__LINE__);
#endif
	       }	
          }
	   else
	   {
		    StatusMgDdi = CHCADDINotFound;
#ifdef        CHSC_DRIVER_DEBUG			
	           do_report(severity_info,"\n Empty reader or smart card inserted not ready");
#endif
	   }
   	   semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	   return (StatusMgDdi);
 
 }


/*******************************************************************************
 *Function name:  CHCA_CardUnlock
 *
 *
 *Description: this interface is used to cancel temporary locking of access to the
 *             card present in the card reader related to the hCard handle.If the
 *             exchange initiating the request has not been completed,it must terminate
 *             normally              
 *
 *Prototype
 *     CHCA_DDIStatus  CHCA_CardUnlock( U32  hCard)
 *
 *
 *input:
 *     hCard:   DDI card reader instance handle.
 * 
 * 
 *
 *output:
 *
 *Return value:
 *     CHCADDIOk:          Access to the smart card reader is unlocked.
 *     CHCADDIBadParam:    DDI card reader instance handle unknown.
 *     CHCADDINoResource:  Exclusive use of the reader has been granted to another application
 *     CHCADDILocked:      The reader is temporary locked by another application
 *     CHCADDINotFound:    Empty reader or smart card inserted not ready.
 *     CHCADDIError:       Interface execution error.
 * 
 * 
 *******************************************************************************/
 CHCA_DDIStatus  CHCA_CardUnlock( TCHCADDICardReaderHandle  hCard)
 {
          /*ST_ErrorCode_t                   ErrCode;*/
          CHCA_DDIStatus                 StatusMgDdi = CHCADDIError;
	   CHCA_UINT                         iCardIndex;


          /*do_report(severity_info,"\n [CHCA_CardUnlock::]\n");

	   StatusMgDdi = CHCADDIOK;
	   return StatusMgDdi;*/

	   
          iCardIndex = hCard -1;	   
	   if((hCard==NULL)||(iCardIndex>=MAX_APP_NUM))
	   {
                 StatusMgDdi = CHCADDIBadParam;
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n[CHCA_CardUnlock::] Smart card reader handle unknown \n");
#endif				 
                 return  (StatusMgDdi);
	   }

          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
          if ((SCOperationInfo[iCardIndex].CardExAccessOwner != 0) &&( SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner))
          {
                 StatusMgDdi = CHCADDINoResource;
		 
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);	
		   return  (StatusMgDdi);		 
          }
 
          if((SCOperationInfo[iCardIndex].SC_Status==CH_RESETED) ||(SCOperationInfo[iCardIndex].SC_Status==CH_OPERATIONNAL))
          {
                if ((SCOperationInfo[iCardIndex].CardLock.Owner != 0 )&&( SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardLock.Owner))
               {
                      StatusMgDdi = CHCADDILocked;
#ifdef            CHSC_DRIVER_DEBUG
                      do_report(severity_info,"\n The reader is temporary locked by another application \n");
#endif			 
	         }
	         else
	         {
                       StatusMgDdi = CHCADDIOK;
                       SCOperationInfo[iCardIndex].CardLock.Owner = 0;
		         semaphore_signal(SCOperationInfo[iCardIndex].CardLock.Semaphore);		
	         }
           }
	    else
	    {
	            StatusMgDdi = CHCADDINotFound;
#ifdef         CHSC_DRIVER_DEBUG
                   do_report(severity_info,"\n Empty reader or smart card inserted not ready \n");
#endif	
                   if((SCOperationInfo[iCardIndex].CardLock.Owner != 0 )&&( SCOperationInfo[iCardIndex].SCHandle == SCOperationInfo[iCardIndex].CardLock.Owner))
                   {
                         SCOperationInfo[iCardIndex].CardLock.Owner = 0;
		           semaphore_signal(SCOperationInfo[iCardIndex].CardLock.Semaphore);		
		     }
	    }
           semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	    /*do_report(severity_info,"\n MGDDICardUnlock leave\n");	   */

	    return(StatusMgDdi);	   
 
 }

#if  1  /*add this on 050510 just for test*/
/*********************************************************************
 *Function name: Ch_CardAbort
 *            
 *
 *Description: this interface cancels the exchange processed by the 
 *             smart card inserted in the MG card reader related to the             
 *             hCard handle.If the MG card reader is locked by the DDI
 *             card reader instance aborting,the locking sequence is  
 *             cancelled.
 * 
 *Prototype:
 *      void   Ch_CardAbort( CHCA_UINT   hCard)  
 *          
 *
 *input:
 *     hCard:    DDI card reader instance handle
 *
 *output:
 *            
 *return value
 *
 *comments:
 *add this on 050510
 ***********************************************************************/
void   Ch_CardAbort( CHCA_UINT   hCard)
 {
	   CHCA_UINT                     iCardIndex;
          ST_ErrorCode_t               ErrCode;

          iCardIndex = hCard;	   

 
         ErrCode = STSMART_Abort(SCOperationInfo[iCardIndex].SCHandle);


         do_report(severity_info,"\n[Ch_CardAbort==>]ErrCode[%d] \n",ErrCode);
	   
         return ;
 }


/*******************************************************************************
 *Function name: CHCA_CardLock
 * 
 *
 *Description: This interface is used to send a continuous sequence of commands
 *             to the card. The group of commands to be strung together begin
 *             with a MGDDICardLock call and end with a MGDDICardUnlock call.
 *             The LockTime parameter indicates the time during which access to
 *             The MG card reader must be locked out temporarily for the 
 *             application that handles the hCard handle.This time is expressed in ms. 
 *
 *             While locked out,only the application that handles the hCard handles 
 *             can retransmit through this MG card reader.Temporary locking is 
 *             automatically cancelled by the device driver if no new command is  
 *             sent to the card after a timeout defined by the LockTime parameter on 
 *             completion of reception of the response of the last chained command. 
 *             This automatic cancellation is notified by the callback function provided 
 *             at the address given in the hCallback parameter via the MGDDIEvCardLockTimeout
 *             event.The locking sequence can be cancelled by the lock owner at any time 
 *             with MGDDICardUnlock interface.
 *
 *
 *Prototype:
 *  CHCA_DDIStatus   CHCA_CardLock
 *   	  ( U32  hCard,CALLBACK  hCallback,unsigned long  LockTime)  
 *
 *input:
 *     hCard:          DDI card reader instance handle.       
 *     hCallback:      Callback function address.
 *     LockTime:       Maximum duration of MG card reader locking.
 *
 *
 *output:
 *
 *Return Value:
 *     CHCADDIOk:          Access to the smart card reader is unlocked.
 *     CHCADDIBadParam:    Smart card reader handle unknown
 *     CHCADDINoResource:  Exclusive use of the reader has been granted to another application
 *     CHCADDILocked:      The reader is temporary locked by another application
 *     CHCADDIBusy:        The reader is currently processing a command.
 *     CHCADDINotFound:    Empty reader or smart card inserted not ready.
 *     CHCADDIError:       Interface execution error.
 *
 *
 *
 *Comments:
 *     1) Exclusivity of the MG card reader can prevent the use of this function 
 *     2) A locking sequence, a smart card exchange or a protocol change in progress may
 *        delay this request until their completion.This interface returns MGDDIOk in 
 *        this case
 *      The MGDDLocked code is returned only if locking is requested a second time by the same DDI 
 *     instance without having prevously unlocked the intended MG card reader
 *******************************************************************************/
void  Ch_CardUnlock( CHCA_UINT  hCard)
 {
          /*ST_ErrorCode_t                   ErrCode;*/
	   CHCA_UINT                         iCardIndex;

	   /*StatusMgDdi = CHCADDIOK;
	   return StatusMgDdi;*/

          do_report(severity_info,"\n [Ch_CardUnlock::]\n");
	   
          iCardIndex = hCard ;	   
	   
          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);
 
          if((SCOperationInfo[iCardIndex].CardLock.Owner != 0 )&&( SCOperationInfo[iCardIndex].SCHandle == SCOperationInfo[iCardIndex].CardLock.Owner))
          {
                SCOperationInfo[iCardIndex].CardLock.Owner = 0;
                semaphore_signal(SCOperationInfo[iCardIndex].CardLock.Semaphore);		
          }
           semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	    /*do_report(severity_info,"\n MGDDICardUnlock leave\n");	   */

	    return;	   
 
 }
#endif


/*******************************************************************************
 *Function name:  ChMgCardTest
 *
 *
 *Description: smart card test function
 *
 *
 *Prototype
 *     boolean  ChMgCardTest(void)
 *
 *
 *input:
 *     void
 * 
 * 
 *
 *output:
 *
 *Return value:
 *    false: fail to test the card
 *    true: test ok
 *comment:
 *add    050221          card test function
 *******************************************************************************/
boolean  ChMgCardTest(void)
{
          ST_ErrorCode_t               ErrCode;
	   CHCA_UINT                     iCardIndex;	  
	   boolean                           CardTestOk=true;

          iCardIndex = 0;	   

          /*execlusively this private data structure*/
          semaphore_wait(SCOperationInfo[iCardIndex].ControlLock);

          if (SCOperationInfo[iCardIndex].CardExAccessOwner != 0 && SCOperationInfo[iCardIndex].SCHandle != SCOperationInfo[iCardIndex].CardExAccessOwner)
          {
#ifdef       CHSC_DRIVER_DEBUG
                 do_report(severity_info,"\n Exclusive use of the reader has been granted to another application \n");
#endif
                 CardTestOk = false; 
                 semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);
		   return CardTestOk;		 
          } 
		  

          ErrCode = STSMART_Reset(SCOperationInfo[iCardIndex].SCHandle,
                                                     SCOperationInfo[iCardIndex].Read_Smart_Buffer,
                                                     &SCOperationInfo[iCardIndex].NumberRead);

	   if(ErrCode != ST_NO_ERROR)		
	   {
		 CardTestOk = false;			  
	   }

	   semaphore_signal(SCOperationInfo[iCardIndex].ControlLock);

	   return CardTestOk;

 }


void CH_CardOk(void)
{
CHCA_CardSendMess2Usif(CH_CA2IPANEL_OK_CARD,NULL,0); 
}

