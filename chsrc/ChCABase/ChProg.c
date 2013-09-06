#ifdef    TRACE_CAINFO_ENABLE
#define                 CHPROG_DRIVER_DEBUG
#endif



/*#define                 CHPROG_DRIVER_DEBUG*/

/*#define AV_CONTROL  *//*zxg20060117 add disable av control*/
/*#define                 CADATA_TEST*/
/*#define                 PMTDATA_TEST*/
/******************************************************************************
*
* File : ChProg.C
*
* Description : 
*
*
* NOTES :
*
*
* Author : ly
*
* Status :
*
* History : 0.0    2004-6-26   Start coding on 5518 platform
*              0.1    2004_9_26   copy from the 5518kernel code.               
                0.3   2006-3       zxg  change for STi7710        
* copyright:Changhong 2004 (c)
*
*****************************************************************************/
#include          "ChProg.h"
#include          "ChUtility.h"
#ifdef CH_IPANEL_MGCA
#include "..\MGCA_Ipanel\MGCA_Ipanel.h"
#endif




#ifdef USE_IPANEL
extern boolean eis_AV_play_state(void);
#endif

#if 1 /*add this on 050322*/
/*about the parity of the table*/
BYTE                        Witch_Parity_to_be_parsed[2];

#define     ECM_EVEN_TABLE_ID                           0x80
#define     ECM_ODD_TABLE_ID                            0x81


CHCA_BOOL          CAT_NoSignal=false;

enum
{
    EVEN_ODD_Parity_to_be_parsed,
    EVEN_Parity_to_be_parsed,
    ODD_Parity_to_be_parsed
};
#endif

#if 1  /*add this on 050421*/
CHCA_BOOL             EcmDataStop=true;
CHCA_BOOL             MGCAStop=false;  /*add this on 050424*/
#endif


#ifdef   PAIR_CHECK /*add this on 050114*/
extern boolean                                                         PairfirstPownOn;/*add this on 050509*/
CHCA_BOOL                                                            PAIR_EMM = false;
CHCA_BOOL                                                            ANTI_EMM = false; /*add this on 050203*/
CHCA_BOOL                                                            PNvmUpdate=false;
semaphore_t                                                           *psemPairStatusAccess = NULL; 
extern  CHCA_INT                                                    AntiIndex;     /*add this on 050203*/               
PAIR_NVM_INFO_STRUCT                                         pastPairInfo;
#endif


ST_DeviceName_t                                                    MGTUNERDeviceName = "MGTUNER";

SRCOperationInfo_t                                                  SRCOperationInfo; 
semaphore_t*                                                          pSemCtrlOperationAccess=NULL;


STREAM_PMT_STRUCT                                              stTrackTableInfo;
CHCA_INT                                                                Video_Track = CHCA_INVALID_TRACK;
CHCA_INT                                                                Audio_Track = CHCA_INVALID_TRACK;
CHCA_INT                                                                HTML_Track = CHCA_INVALID_TRACK;
CHCA_INT                                                                STOCK_Track = CHCA_INVALID_TRACK;
CHCA_INT                                                                APPLICATION_Track = CHCA_INVALID_TRACK;

CHCA_BOOL                                                             PMTUPDATE=false; /*add this on 050316*/
CHCA_BOOL                                                             VideoUpdate = false;
CHCA_BOOL                                                             AudioUpdate = false;

CHCA_UCHAR                                                           PMT_Buffer [MGCA_NORMAL_TABLE_MAXSIZE] ;
message_queue_t                                                     *pstCHCAPMTMsgQueue=NULL;
message_queue_t                                                     *pstCHCAECMMsgQueue=NULL; /*add this on 041029*/
message_queue_t                                                     *pstCHCACATMsgQueue=NULL;
message_queue_t                                                     *pstCHCAEMMMsgQueue=NULL; /*add this on 050115*/

TCHCAPairingStatus                                                  CardPairStatus= CHCAPairError;/*card pair status,add it on 041101*/ 

CHCA_BOOL                                                             NewPMTChannel=false;

extern  CHCA_UINT                                                   EcmFilterNum;/*add this on 050313*/


/*pmt monitor task*/
task_t	                                                                * ptidPMTMonitorTask;/* 060118 xjp change from *ptidPMTMonitorTask to ptidPMTMonitorTask  for adopt task_init()*/
const int PMT_PROCESS_WORKSPACE                      = 1024*30;/*from 20 to 15 modify this on 050309*/
const int PMT_PROCESS_PRIORITY                          =  8/*13*/;   /*from 8 to 10 modify this on 050309*/
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_PMTMonitorStack;
	static tdesc_t g_PMTMonitorTaskDesc;
#endif
/*pmt monitor task*/


/*cat monitor task*/
task_t	                                                                 *ptidCATMonitorTask;/* 060118 xjp change from *ptidCATMonitorTask to ptidCATMonitorTask  for adopt task_init()*/
const int   CAT_PROCESS_WORKSPACE                    = 1024*30;/*from 20 to 15 modify this on 050309*/
const int   CAT_PROCESS_PRIORITY                        =8 /* 8 20061127 change 8-->9*/; /*from 8 to 9 modify this on 050309*/
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_CATMonitorStack;
	static tdesc_t g_CATMonitorTaskDesc;
#endif
/*cat monitor task*/


/*emm monitor task*/
task_t	                                                                 *ptidEMMMonitorTask;/* 060118 xjp change from *ptidEMMMonitorTask to ptidEMMMonitorTask  for adopt task_init()*/
const int   EMM_PROCESS_WORKSPACE                    = 1024*20;/*from 20 to 15 modify this on 050309*/
const int   EMM_PROCESS_PRIORITY                        =  9; /*from 10 to 8 modify this on 060119*/
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_EMMMonitorStack;
	static tdesc_t g_EMMMonitorTaskDesc;
#endif

/*emm monitor task*/


/*ecm monitor task*/
task_t	                                                                 *ptidECMMonitorTask;/* 060118 xjp change from *ptidECMMonitorTask to ptidECMMonitorTask  for adopt task_init()*/
const int   ECM_PROCESS_WORKSPACE                    = 1024*2*20;/*from 20 to 15 modify this on 050309*/
const int   ECM_PROCESS_PRIORITY                        =  9; /*from 12 to 9 modify this on 060119*/
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_ECMMonitorStack;
	static tdesc_t g_ECMMonitorTaskDesc;
#endif
/*ecm monitor task*/


BOOL                                                            ChCaReSearchProgram=false; /*add this on 050302*/
/*BOOL                                                            ChCAStartSearchProgram=false;  add this on 050303*/

/*the variable definition for ch-mg function*/
TMGAPICtrlHandle                                                    pChMgApiCtrlHandle=NULL;
CHCA_BOOL                                                                      PairCheckPass=false;
CHCA_BOOL                                                            PairingCheck_FTA = false;
CHCA_BOOL                                                            PairingCheck_NOFTA = false;
/*the variable definition for ch-mg function*/

CHCA_UCHAR                                                          PMT_Buffer [MGCA_NORMAL_TABLE_MAXSIZE] ;
CHCA_UCHAR                                                          CAT_Buffer [MGCA_NORMAL_TABLE_MAXSIZE] ;
CHCA_UCHAR                                                          ECM_Buffer[MGCA_PRIVATE_TABLE_MAXSIZE];
CHCA_UCHAR                                                          EMM_Buffer[MGCA_PRIVATE_TABLE_MAXSIZE];


STREAM_ECM_STRUCT                                             EcmFilterInfo[2];  /*add this on 050310*/


extern CHCA_UCHAR                                                PmtSectionBuffer[1024];
extern CHCA_UCHAR                                                CatSectionBuffer[1024];
/*extern CHCA_UCHAR                                                EcmSectionBuffer[MGCA_PRIVATE_TABLE_MAXSIZE];delete this on 050327*/

	
CHCA_SHORT                                                         iFilterId_requested=-1;
CHCA_SHORT                                                        iFilterId_received=-1;
chca_pmt_monitor_type                                            pmt_search_state = CHCA_FIRSTTIME_PMT_COME;
chca_pmt_monitor_type                                            pmt_main_state = CHCA_PMT_ALLOCTAED;

CHCA_BOOL                                                            CatReady = false;
/*CHCA_SHORT                                                          CatFilterId = CHCA_MAX_NUM_FILTER; delete this on 050311*/   
/*CHCA_SHORT                                                          EcmFilterId = CHCA_MAX_NUM_FILTER;   */ 

CHCA_INT                                                                Prog_CA_CurRight[2]={-1,-1};

extern STBStaus                                                       App_CA_CurRight;
extern CHCA_BOOL                                                   CurAppWait;
extern semaphore_t                                                  *psemPmtOtherAppParseEnd; 


#ifdef   PMTDATA_TEST
extern CHCA_BOOL CHMG_FreeSectionReq(U16   iiFilterIndex);
extern CHCA_SHORT CHMG_SectionReq(message_queue_t  *ReturnQueueId,
	                                                    CHCA_USHORT       iPidValue,
	                                                    CHCA_USHORT       iTableId);
SHORT   ECMReqId;
#endif

 task_status_t                                                            Status;

CHCA_BOOL                                                              ChCaNewTransponder = false;
/*semaphore_t                                                              semWaitCatsignal;delete this on 050327*/
extern  semaphore_t                                                   *psemAVSeparate;  /*add this on 041202*/
extern CHCA_BOOL                                                     CardContenUpdate; /*add this on 050106*/
CHCA_BOOL                                                               EMMDataProcessing=false;
CHCA_BOOL                                                               ECMDataProcessing=false;
CHCA_BOOL                                                               CATDataProcessing = false;/*delete this on 050311*/
CHCA_BOOL                                                               TimerDataProcessing=false;/*add this on 050521*/

CHCA_INT                                                                  GeoInfoStatus;/*add this on 050119*/
/*CHCA_USHORT                                                           CatTimerHandle;delete this on 050311*/

/*BOOL  EmmCardUpdated=false; add this on 050128*/

CHCA_BOOL                                                              MpegEcmProcess=false;

CHCA_BOOL                                                              EMMDataRightUpdated; /*add this on 050225*/

#if 0/*add this on 050225*/
/*extern  CHCA_SHORT                                                 EcmFilterIdTemp;  delete this on 050311*/  
/*extern CHCA_UCHAR                                                  EcmTableTemp;	delete this on 050311*/
#endif

#ifdef   PAIR_CHECK/*add this on 050114*/
CHCA_BOOL                                                               PairingCheck=false;
/*******************************************************************************
 *Function name:  PNvmUpdateCheck
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_BOOL  PNvmUpdateCheck(CHCA_BOOL *PNvmUpdate)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_BOOL  PNvmUpdateCheck(CHCA_BOOL *PNvmUpdate)
{
	
	unsigned short      a;
	
	if(ReadNvmData(START_E2PROM_CA,2,&a))
      {
	   do_report(severity_info,"\n fail to read the pair_nvm!! \n ");
	   return (true);
	}
	
	/*do_report(severity_error,"pair tag = [%08X]\n",a);*/
	
	if(a != 0x6534)
	{
		*PNvmUpdate = TRUE;
		
		do_report(severity_error,"\n EEPROM has been changed by other!\n");
		
		a=0x6534;
		if(WriteNvmData(START_E2PROM_CA,2,&a))
              {
                     do_report(severity_error," \n fail to write data to EEPROM!\n"); 		   
			return (true);
		}
	}

	return (false);


}


#if  0
/*******************************************************************************
 *Function name:  ResetPairData
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_BOOL   ResetPairData()
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_BOOL   ResetPairData()
{
    
	SendMessage2CAT ( STOP_BUILDER_CAT );

	task_delay(2000);

	if(PairData_Init())
       {
	        do_report( severity_info,"<%s><%d> Pair Nvm INIT Fail!\n",__FILE__,__LINE__);
	        return true;
	}
	else
       {
	       do_report( severity_info,"<%s><%d> Pair Nvm INIT Success!\n",__FILE__,__LINE__); 
	   
	}

	GetPairInfo();

	return (false);

}
#endif


/*******************************************************************************
 *Function name:  InitialiseNvmPairInfo
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          void InitialiseNvmPairInfo ( void )
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
void InitialiseNvmPairInfo ( void )
{
      int    i;

      for (i=0;i<6;i++ )
      {
          pastPairInfo.CaSerialNumber[i] = 0;
      }

      pastPairInfo.ScrambledBit0 = 0;
      pastPairInfo.ClearBit1 = 0;
      pastPairInfo.PairStatus2 = 0;  /*add this on 050221*/	 

      return;
}

/*******************************************************************************
 *Function name:  PairData_Init
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_INT    PairData_Init(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_INT    PairData_Init(void)
{
       CHCA_INT         error;
       CHCA_INT         i;
       CHCA_CHAR      PairData[9];

       semaphore_wait(psemPairStatusAccess);	
   
       InitialiseNvmPairInfo();

       for(i=0;i<6;i++)
       {
            PairData[i] = pastPairInfo.CaSerialNumber[i];
       }

       PairData[6] = pastPairInfo.ScrambledBit0;
	PairData[7] =	pastPairInfo.ClearBit1;
	PairData[8] =	pastPairInfo.PairStatus2;
   
       error = WriteNvmData(START_E2PROM_CA+2,9,PairData); 

       semaphore_signal(psemPairStatusAccess);

      if(error)
      {
           do_report(severity_info,"\n Write Pair data to EEPROM error!!!\n"); 
      }		   
   
      return (error);

}

/*******************************************************************************
 *Function name:  PairMgrInit
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_BOOL  PairMgrInit (void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_BOOL  PairMgrInit (void)
{
       CHCA_INT     i;

	/*psemPairCheckAccess = semaphore_create_fifo ( 1 ); */
       psemPairStatusAccess = semaphore_create_fifo(1);        

       if( PNvmUpdateCheck(&PNvmUpdate) )
       {
            do_report( severity_info,"<%s><%d> Pair Nvm check error!\n",__FILE__,__LINE__);
	     return TRUE;
       }

       /*do_report(severity_info,"\n PNvmUpdate[%d] \n",PNvmUpdate);*/

       if ( PNvmUpdate )
	{
		if(PairData_Init())
              {
		       do_report( severity_info,"<%s><%d> Pair Nvm INIT Fail!\n",__FILE__,__LINE__);
		       return TRUE;
		}
       }

       GetPairInfo();

	/*do_report (severity_info,"E2PROM DATA MGR INIT SUCCESSFUL!\n",__FILE__,__LINE__);*/
    
	return false;
}

#ifdef CH_IPANEL_MGCA



/*******************************************************************************
 *Function name:  CH_IPNAEL_MGCA
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          int  CH_IPNAEL_MGCA(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
   int  CH_IPNAEL_MGCA(void)
   {
       CHCA_INT                         error;
	CHCA_INT                         i;
	CHCA_CHAR                      PairData[9];
	int    i_result;
	
	error = ReadNvmData(START_E2PROM_CA+2,9,PairData);

	if(error)
	{
             return -1;
	} 	
	i_result  = PairData[6]|(PairData[7]<<1)|(PairData[8]<<2);
       return i_result;
}

#endif

/*******************************************************************************
 *Function name:  GetPairInfo
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          void  GetPairInfo(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
void  GetPairInfo(void)
{
       CHCA_INT                         error;
	CHCA_INT                         i;
	CHCA_CHAR                      PairData[9];
	
	error = ReadNvmData(START_E2PROM_CA+2,9,PairData);

	if(error)
	{
	      do_report(severity_info,"\n Read Pairdata from EEPROM error!!!\n"); 
             return;
	} 	

#if 0	
	for(i=0;i<9;i++)
	    do_report(severity_info,"\n ReadPairNvmData==>%d",PairData[i]);
#endif	

       semaphore_wait(psemPairStatusAccess);
	for(i=0;i<6;i++)
       {
	    pastPairInfo.CaSerialNumber[i] = PairData[i];
       } 

	pastPairInfo.ScrambledBit0 = PairData[6];
	pastPairInfo.ClearBit1 = PairData[7];
	pastPairInfo.PairStatus2= PairData[8];
       semaphore_signal(psemPairStatusAccess);


#if 0
	do_report(severity_info,"\n");

	for(i=6;i<9;i++)
	    do_report(severity_info,"\n ReadPairNvmData==>%d",PairData[i]);

	do_report(severity_info,"\n");
#endif	

       return;
}




/*******************************************************************************
 *Function name:  WritePairInfo
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          void  WritePairInfo(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
 #if  0
 /*write the pair status and the card serial number into the eeprom*/
void  WritePairInfo(void)
{
     CHCA_INT                           error;
     CHCA_INT                           i;
     CHCA_CHAR                        PairData[9];

     for(i=0;i<6;i++)
     {
	    PairData[i] = pastPairInfo.CaSerialNumber[i];
     }

     PairData[6] = pastPairInfo.ScrambledBit0;
     PairData[7] = pastPairInfo.ClearBit1;
     PairData[8] = pastPairInfo.PairStatus2;
   
     error = WriteNvmData(START_E2PROM_CA+2,9,PairData ) ;
 
     if(error)
     {
         do_report(severity_info,"\n fail to write pair data to EEPROM\n");
     }		 

     return;	
	
}
#else
/*just write the card serial number into the eeprom,modify this on 050515*/
 void  WritePairInfo(void)
{
     CHCA_INT                           error;
     CHCA_INT                           i;
     CHCA_CHAR                        PairData[6];

     for(i=0;i<6;i++)
     {
	    PairData[i] = pastPairInfo.CaSerialNumber[i];
     }



#ifdef           CHPROG_DRIVER_DEBUG               
                     do_report(severity_info,"\n[WritePairInfo==>\n");
			for(i=0;i<6;i++)	
			{
                           do_report(severity_info,"%4x",pastPairInfo.CaSerialNumber[i]);
			}
			do_report(severity_info,"\n");
#endif			

	 

     error = WriteNvmData(START_E2PROM_CA+2,6,PairData ) ;
 
     if(error)
     {
         do_report(severity_info,"\n fail to write pair data to EEPROM\n");
     }		 


     return;	
	
}
#endif

/*******************************************************************************
 *Function name:  WritePairStautsInfo
 * 
 *
 *Description:  write the pair status in the cat table into the eeprom
 *                
 *
 *Prototype:
 *          void  WritePairStautsInfo(void)
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     write the pair status in the cat table into the eeprom
 *     add this on 050515
 * 
 ******************************************************************************/
void  WritePairStautsInfo(void)
{
     CHCA_INT                           error;
     CHCA_INT                           i;
     CHCA_CHAR                        PairData[3];

     PairData[0] = pastPairInfo.ScrambledBit0;
     PairData[1] = pastPairInfo.ClearBit1;
     PairData[2] = pastPairInfo.PairStatus2;

#ifdef           CHPROG_DRIVER_DEBUG               
     do_report(severity_info,"\n[WritePairStautsInfo==>\n");
     for(i=0;i<3;i++)	
     {
           do_report(severity_info,"%4x",PairData[i]);
     }
     do_report(severity_info,"\n");
#endif	
   
     error = WriteNvmData(START_E2PROM_CA+2+6,3,PairData ) ;
 
     if(error)
     {
          do_report(severity_info,"\n fail to write pair data to EEPROM\n");
     }		 

#if  0 /*just for test*/
     GetPairInfo();
#endif

     return;	
	
}



/*******************************************************************************
 *Function name:  CheckEmmP
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *         void    CheckEmmP(CHCA_UCHAR   *pBlk)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
 #ifdef                 PAIR_CHECK 						   
void    CheckEmmP(CHCA_UCHAR   *pBlk)
{
        PAIR_EMM = false;
        ANTI_EMM = false;	 /*add this on 050203*/	
 
        if(pBlk!=NULL)
        {
               if(pBlk[0]==0x86)
               {
                       switch(pBlk[13])
                       {
                             case 0x2:
                             case 0xb:
                                      break;	  	 
                       
                             case 0x7:
                                     switch(pBlk[15])	  
                                     {
                                          case 0x8:
#ifdef                                        CHPROG_DRIVER_DEBUG					   	
                                                   do_report(severity_info,"\n[CheckEmmP==>] Anti Message Coming Index[%d]\n",pBlk[14]);	  	
#endif
                                                   if(AntiIndex!=pBlk[14])
                                                   {
                                                        ANTI_EMM=true; 
							       AntiIndex = pBlk[14];
                                                   }	
                                                   break;
                       
                                          default:
#ifdef                                         CHPROG_DRIVER_DEBUG					   	
                                                   do_report(severity_info,"\n[CheckEmmP==>] Pairing Message Coming Index[%d]\n",pBlk[14]);	  	
#endif
                                                   PAIR_EMM=true;                                                                      
                                                   break;
                                      }
                                      break;	
                       
                             default:
                                     break;
                       }
		 }
	 }
}
#endif						   


/*******************************************************************************
 *Function name:  CheckPairFlashStatus
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *         void    CheckPairFlashStatus(void)
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     2005-03-19   add the function
 * 
 * 
 ******************************************************************************/
CHCA_BOOL    CheckPairFlashStatus(void)
{
        CHCA_BOOL  FlashWriteOk=false;
		
        if(PAIR_EMM && (CardPairStatus != CHCAPairOK))
        {
             FlashWriteOk = true;
	      do_report(severity_info,"\n the pair status is changed,save it\n");		 
        } 		 

        return FlashWriteOk;	
}



#endif



/*******************************************************************************
 *Function name:  Ch_GetAudioSlotIndex
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_UINT   Ch_GetAudioSlotIndex(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_UINT   Ch_GetAudioSlotIndex(void)
{
#if      /*PLATFORM_16 20070514 change*/1
              return AudioSlot;
#endif


}

/*******************************************************************************
 *Function name:  Ch_GetVideoSlotIndex
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_UINT  Ch_GetVideoSlotIndex(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_UINT  Ch_GetVideoSlotIndex(void)
{
#if      /*PLATFORM_16 20070514 change*/1
              return VideoSlot;
#endif



}

/*******************************************************************************
 *Function name:  ChCheckProgTrackType
 * 
 *
 *Description:  check the type of the prog track( type: FTA_PROGRAM, SCRAMBLED_PROGRAM)
 *                
 *
 *Prototype:
 *          CHCA_BOOL   ChCheckProgTrackType(U16   iElementPid,p_prog_type*    bProgType)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_BOOL   ChCheckProgTrackType(U16   iElementPid,p_prog_type*    bProgType)
{
        CHCA_BOOL           ReturnCode = true;
	 CHCA_UINT            iTrackIndex;	

        if(iElementPid != CHCA_DEMUX_INVALID_PID)
	 {
                for(iTrackIndex=0;iTrackIndex<CHCA_MAX_TRACK_NUM;iTrackIndex++)
                {
                      if(iElementPid == stTrackTableInfo.ElementPid[iTrackIndex])
		            break;			  	
		  }

		  if(iTrackIndex<CHCA_MAX_TRACK_NUM)	
		  {
		         ReturnCode = false;
                       *bProgType = stTrackTableInfo.ElementType[iTrackIndex];
	                /*do_report(severity_info,"\n[ChCheckProgTrackType==>] iTrackIndex[%d]!\n",iTrackIndex);*/
		  }
   	 }
        else
        {
                do_report(severity_info,"\n[ChCheckProgTrackType==>] Invalid Pid!\n");
	 }

        return ReturnCode;  

}


/*******************************************************************************
 *Function name:  ChGetProgAVPid
 * 
 *
 *Description:  get current pids of the program including addio and video
 *                  
 *
 *Prototype:
 *          CHCA_BOOL   ChGetProgAVPid(STREAM_PMT_STRUCT    *sTrackTable,S32  iLocalProgIndex)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
CHCA_BOOL   ChGetProgAVPid(STREAM_PMT_STRUCT    *sTrackTable,CHCA_INT  iLocalProgIndex)
{
     
        CHCA_UINT                     i;	
	 CHCA_BOOL                    AudioApp,VideoApp;	
	 p_prog_type                    ProgType;
        CHCA_BOOL                   bErrCode=false;

#ifdef CH_IPANEL_MGCA
        CH_Service_Info_t                ch_ServiceInfo  ;
        CH_MGCA_Ipanel_GetService(&ch_ServiceInfo);
        if (sTrackTable==NULL)
        {
               bErrCode=true; 
               do_report(severity_info,"\n[ChGetProgAVPid==>]The input parameter is wrong\n");
               return  bErrCode;
        }



    
#endif
        AudioApp=VideoApp=false;


       if(Video_Track != CHCA_INVALID_TRACK)
       {
             VideoApp = true; 
	}

	if(Audio_Track != CHCA_INVALID_TRACK)
       {
             AudioApp = true;
	}
	
		
#ifdef CH_IPANEL_MGCA
if((ch_ServiceInfo.video_pid < 8191) && VideoApp)
	 {
               sTrackTable->ProgElementPid[0] =ch_ServiceInfo.video_pid;
		 sTrackTable->ProgTrackType[0] = VIDEO_TRACK;
		 sTrackTable->iProgNumofMaxTrack++;
		 /*do_report(severity_info,"[ChGetProgAVPid==>]VideoPid[%4x]",sTrackTable->ProgElementPid[0]);*/
		 if(ChCheckProgTrackType(sTrackTable->ProgElementPid[0],&ProgType))
		 {
		       bErrCode = true;
                       do_report(severity_info,"\n[ChGetProgAVPid==>]Can not get the type of the video track\n");
		 }
		 else
		 {
                     sTrackTable->ProgElementType[0] = ProgType;
		 }
	 }


	 if((ch_ServiceInfo.audio_pid  < 8191)&& AudioApp)
	 {
               sTrackTable->ProgElementPid[1] = ch_ServiceInfo.audio_pid;
		 sTrackTable->ProgTrackType[1] = AUDIO_TRACK;
		 sTrackTable->iProgNumofMaxTrack++;
		 /*do_report(severity_info,"[ChGetProgAVPid==>]AudioPid[%4x]",sTrackTable->ProgElementPid[1]);*/
		 if(ChCheckProgTrackType(sTrackTable->ProgElementPid[1],&ProgType))
		 {
		       bErrCode = true;
                     do_report(severity_info,"\n[ChGetProgAVPid==>]Can not get the type of the audio track\n");
		 }
		 else
		 {
                     sTrackTable->ProgElementType[1] = ProgType;
		 }
	 }
 

#else	
	 if((pProgInfoTemp->sVidPid != CHCA_DEMUX_INVALID_PID) && VideoApp)
	 {
               sTrackTable->ProgElementPid[0] = pProgInfoTemp->sVidPid;
		 sTrackTable->ProgTrackType[0] = VIDEO_TRACK;
		 sTrackTable->ProgElementSlot[0] = Ch_GetVideoSlotIndex();
		 sTrackTable->iProgNumofMaxTrack++;
		 /*do_report(severity_info,"[ChGetProgAVPid==>]VideoPid[%4x]",sTrackTable->ProgElementPid[0]);*/
		 if(ChCheckProgTrackType(sTrackTable->ProgElementPid[0],&ProgType))
		 {
		       bErrCode = true;
                       do_report(severity_info,"\n[ChGetProgAVPid==>]Can not get the type of the video track\n");
		 }
		 else
		 {
                     sTrackTable->ProgElementType[0] = ProgType;
		 }
	 }


	 if((pstBoxInfo->astAudStr[iLocalProgIndex].sAudPid != CHCA_DEMUX_INVALID_PID)&& AudioApp)
	 {
               sTrackTable->ProgElementPid[1] = pstBoxInfo->astAudStr[iLocalProgIndex].sAudPid;
		 sTrackTable->ProgTrackType[1] = AUDIO_TRACK;
		 sTrackTable->ProgElementSlot[1] = Ch_GetAudioSlotIndex();
		 sTrackTable->iProgNumofMaxTrack++;
		 /*do_report(severity_info,"[ChGetProgAVPid==>]AudioPid[%4x]",sTrackTable->ProgElementPid[1]);*/
		 if(ChCheckProgTrackType(sTrackTable->ProgElementPid[1],&ProgType))
		 {
		       bErrCode = true;
                     do_report(severity_info,"\n[ChGetProgAVPid==>]Can not get the type of the audio track\n");
		 }
		 else
		 {
                     sTrackTable->ProgElementType[1] = ProgType;
		 }
	 }
 
#endif
        /*if(pastProgramFlashInfoTable[iLocalProgIndex]->cServiceType==DIGITAL_MOSAIC_CHANGHONG)
	 {
              do_report(severity_info,"\nMosaic audio pid[%d]\n",sTrackTable->ProgElementPid[1]);
 	 }*/

        if((sTrackTable->ProgElementType[0]==FTA_PROGRAM) && (sTrackTable->ProgElementType[1]==FTA_PROGRAM))
        {
                 sTrackTable->ProgType = FTA_PROGRAM;
	 }
        else
        {
                 sTrackTable->ProgType = SCRAMBLED_PROGRAM;
	 }

#ifdef    CHPROG_DRIVER_DEBUG
        do_report(severity_info,"\n [ChGetProgAVPid==>]Total PidNum[%d]",sTrackTable->iProgNumofMaxTrack);
#endif

       return  bErrCode;

}


/*******************************************************************************
 *Function name:  ChResetPMTStreamInfo
 * 
 *
 *Description:  init the pmt stream info
 *                  
 *
 *Prototype:
 *          void ChResetPMTStreamInfo(void)
 *       
 *
 *input:
 *      
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
 ******************************************************************************/
void ChResetPMTStreamInfo(void)
{
       U32                                    iTrackIndex;
	CHCA_UINT                        i;

       for ( iTrackIndex = 0;iTrackIndex < CHCA_MAX_TRACK_NUM;iTrackIndex++ )
       {
              stTrackTableInfo.track_type [iTrackIndex] = CHCA_INVALID_TRACK ;
              stTrackTableInfo.ElementPid[iTrackIndex] = CHCA_DEMUX_INVALID_PID;
              stTrackTableInfo.ElementType[iTrackIndex] = FTA_PROGRAM;
       }

       for(i=0;i<2;i++)
       {
             /*add this on 041111*/			
             stTrackTableInfo.ProgElementSlot[i] = CHCA_DEMUX_INVALID_SLOT;
             stTrackTableInfo.ProgElementPid[i] = CHCA_DEMUX_INVALID_PID;
             stTrackTableInfo.ProgElementCurRight[i] = -1;
	      stTrackTableInfo.ProgElementType[i] = FTA_PROGRAM;
	      stTrackTableInfo.ProgTrackType[i]=CHCA_INVALID_TRACK;
       }
	   
        /*add this on 041111*/			
        stTrackTableInfo.iProgNumofMaxTrack = 0;
        stTrackTableInfo.ProgServiceType = UnknowService;
        stTrackTableInfo.iNumofMaxTrack = 0;
        stTrackTableInfo.ProgType = FTA_PROGRAM;
	  
}



/*******************************************************************************
 *Function name:  ChResetPMTSTrackInfo
 * 
 *
 *Description:  init the pmt stream track info
 *                  
 *
 *Prototype:
 *          void ChResetPMTSTrackInfo(void)
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     2005-03-03   modify     init the programid and transponderid when research the program
 * 
 * 
 ******************************************************************************/
void ChResetPMTSTrackInfo(void)
{
        stTrackTableInfo.iVideo_Track = CHCA_INVALID_TRACK;
        stTrackTableInfo.iVideo_VA_Track = CHCA_INVALID_TRACK;
        stTrackTableInfo.iAudio_Track = CHCA_INVALID_TRACK;
        stTrackTableInfo.iAudio_VA_Track = CHCA_INVALID_TRACK;
        stTrackTableInfo.iVersionNum = CHCA_INVALID_LINK;

#if 0
        /*add this on 050303 for initing the programid and transponderid when research the program*/
	 if(ChCAStartSearchProgram)
	 {
	       ChCAStartSearchProgram=false;
              stTrackTableInfo.iProgramID = CHCA_INVALID_LINK;
              stTrackTableInfo.iTransponderID = CHCA_INVALID_LINK; 
	  }
#endif	 
}


/*******************************************************************************
 *Function name:  ChPmtParseDescs
 * 
 *
 *Description:   
 *                  
 *
 *Prototype:
 *          boolean ChPmtParseDescs(int32 track,psit_loop_type loop_type,BYTE  *bPmtBuffer)
 *
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *       true:  there is  a ca descriptor in the current pmt 
 *       false: there is no ca descriptor in the current pmt
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
boolean ChPmtParseDescs(S32 track,psit_loop_type loop_type,BYTE  *bPmtBuffer)
{
       BYTE * aucBuffer;
       S32  iDespLength,iLengthTemp,iTrackNum,iWholeLength;

       if ( bPmtBuffer == NULL )
            return false;

       if ( loop_type == PMTFIRST_LOOP )
       {
              aucBuffer = bPmtBuffer + 12;
              iDespLength = bPmtBuffer [10]&0xf << 8 | bPmtBuffer[11];

              while ( iDespLength > 0)
             {
                   if ( aucBuffer[0] == 0x9 )
                  {
                         return  true;
                  }		   

                  iLengthTemp = aucBuffer [1];
                  aucBuffer += (iLengthTemp + 2);
                  iDespLength -= (iLengthTemp+2);
             }
       }
       else if ( loop_type == PMTSECOND_LOOP )
       {
             iWholeLength = ((bPmtBuffer [1]&0xf) << 8) | bPmtBuffer[2];
             iLengthTemp = ((bPmtBuffer [10]&0xf) << 8) | bPmtBuffer[11];
             aucBuffer = bPmtBuffer + 12 + iLengthTemp ;
             iDespLength = iWholeLength - 9 - iLengthTemp - 4 ;

             for ( iTrackNum = 0; iTrackNum < track ; iTrackNum ++ )
             {
                   iLengthTemp = aucBuffer[3]&0xf << 8 | aucBuffer [4];
                   aucBuffer += iLengthTemp + 5 ;
                   iDespLength -= iLengthTemp + 5 ;
             }

             if ( iDespLength > 0)
             {
                   iDespLength = aucBuffer[3]&0xf << 8 | aucBuffer [4];
                   aucBuffer += 5;

                   while ( iDespLength > 0 )
                   {
                         if ( aucBuffer[0] == 0x9 )
                         {
                               return true;
                         }			 

                         iLengthTemp = aucBuffer [1];
                         aucBuffer += (iLengthTemp + 2);
                         iDespLength -= (iLengthTemp + 2);
                  }
             }
             else 
             	   do_report ( severity_info," %s %d ==>track is error !\n",__FILE__,__LINE__);
    }

    return false;
}


/*******************************************************************************
 *Function name:  ChParsePMTForTrackInfo
 * 
 *
 *Description:  parse the pmt table, get some information of the elementary 
 *                  
 *
 *Prototype:
 *          CHCA_BOOL ChParsePMTForTrackInfo ( U16 iFilterId )
 *       
 *
 *input:
 *      
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
/*modify this on 041127*/
CHCA_BOOL ChParsePMTForTrackInfo ( U16   iFilterId ,U8  *PmtData,S32  sProgIndexTemp)
{
         BYTE	                        *pucSectionData;
         int	                               iInfoLength,iEsInfoLength,iSectionLength;
         SHORT2BYTE	           stTempData16;
         CHCA_BOOL                   bErrCode = false;
		 CHCA_BOOL                  bReturnCode=false;
         CHCA_UINT                    iTrackIndex,i;
         
         p_prog_type                  ProgTypeFirst = FTA_PROGRAM;	
         p_prog_type                  ProgTypeSecond = FTA_PROGRAM;	

#if 1 /*for mosiac info*/
	  CHCA_BOOL                  MOSIAC_DES;	 
#endif
#ifdef CH_IPANEL_MGCA
        CH_Service_Info_t          ch_ServiceInfo  ;
        CH_MGCA_Ipanel_GetService(&ch_ServiceInfo);
#endif


#ifndef CH_IPANEL_MGCA
        if((iFilterId<0) ||(iFilterId>=CHCA_MAX_NUM_FILTER))
        {
          do_report(severity_info,"\n[ChParsePMTForTrackInfo==>] The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      return bErrCode;	  
        }
#endif
	 if((PmtData==NULL) || (PmtData[0] != 0x2))
	 {
             do_report(severity_info,"\n[ChParsePMTForTrackInfo==>] PmtData is wrong!\n");
	      bErrCode = true;
	      return bErrCode;	  
	 }
#ifndef CH_IPANEL_MGCA

	 if(sProgIndexTemp == CHCA_INVALID_LINK )
        {           
              do_report(severity_info,"\n[ChParsePMTForTrackInfo==>] Current Program Index is  invalid!\n");
		bErrCode = true; 	  
	       return bErrCode;
        }



#ifdef   CHCA_MULTI_AUDIO 
       bReturnCode=CHCA_StoreMulAudPids( &PmtData[0]);

       if(bReturnCode=false)/* 20061228 wz add */
	     CH_UpdataProgMulAudioInfo(sProgIndexTemp);
	   
#endif

        
#endif

       Video_Track = CHCA_INVALID_TRACK;
       Audio_Track = CHCA_INVALID_TRACK;


        ChResetPMTStreamInfo();/*add this on 041202*/

        iSectionLength = (( PmtData[ 1 ] & 0x0f ) << 8 ) | PmtData[ 2 ] + 3;

        /* read the program_info_length (12)*/
        stTempData16.byte.ucByte1   = PmtData[ 10 ] & 0xF;
        stTempData16.byte.ucByte0   = PmtData[ 11 ];
        iInfoLength = stTempData16.sWord16;

	if(iInfoLength>0)
	{
         /*parse the descriptor in the first loop added on 040927*/
        if(ChPmtParseDescs(0,PMTFIRST_LOOP,&PmtData[0])) 
        {
#ifdef    CHPROG_DRIVER_DEBUG
              do_report ( severity_info, "\n There is a ca descriptor on the first loop!\n");
#endif
              /*stTrackTableInfo.ProgType = SCRAMBLED_PROGRAM; */ 
              ProgTypeFirst =  SCRAMBLED_PROGRAM;
	 }
	}
        pucSectionData  = PmtData + 12 + iInfoLength;

        iInfoLength = iSectionLength - iInfoLength - 12 - 4;

	/* parse the actual information*/
#ifdef CHPROG_DRIVER_DEBUG
        do_report ( severity_info,"iInfoLength=%d\n",iInfoLength );
#endif

	while ( iInfoLength > 0 )
	{
		/*read in the ES_info_length from byte position 3 & 4 and mask off the most significant 4 reserved bits*/
		stTempData16.byte.ucByte1	= pucSectionData [ 3 ] & 0xF;
		stTempData16.byte.ucByte0	= pucSectionData [ 4 ];

		iEsInfoLength = stTempData16.sWord16;
		
#ifdef    CHPROG_DRIVER_DEBUG
              do_report ( severity_info,"iEsInfoLength=%d\n",iEsInfoLength);
#endif
		/* read in the ES_pid from byte position 1 & 2 and  mask off the most significant 3 reserved bits  */
		stTempData16.byte.ucByte1	= pucSectionData [ 1 ] & 0x1F;
		stTempData16.byte.ucByte0	= pucSectionData [ 2 ];

              if ( stTrackTableInfo.iNumofMaxTrack < CHCA_MAX_TRACK_NUM )
              {
  	   	       stTrackTableInfo.ElementPid[stTrackTableInfo.iNumofMaxTrack] =(unsigned short)stTempData16.sWord16; /*add this on 040628*/
                     switch ( pucSectionData [ 0 ] )
		       {
				case MPEG1_VIDEO_STREAM:			/* 0x01 */
				case MPEG2_VIDEO_STREAM:			/* 0x02 */
				case H264_VIDEO_STREAM:		/*wz add for H264解码*/
				    if ( Video_Track == CHCA_INVALID_TRACK )
                                         Video_Track = stTrackTableInfo.iNumofMaxTrack ;
 
				         stTrackTableInfo.track_type[stTrackTableInfo.iNumofMaxTrack] = VIDEO_TRACK;
                                    /*parse the descriptor in the first loop added on 040927*/
                                    if(ChPmtParseDescs(Video_Track,PMTSECOND_LOOP,&PmtData[0])) 
                                    {
#ifdef                              CHPROG_DRIVER_DEBUG
                                          do_report ( severity_info, "\n There is a ca descriptor on the second loop video track[%d]!\n",Video_Track);
#endif
                                          stTrackTableInfo.ProgType = SCRAMBLED_PROGRAM;  
                                          ProgTypeSecond =  SCRAMBLED_PROGRAM;
                                          stTrackTableInfo.ElementType[stTrackTableInfo.iNumofMaxTrack] = SCRAMBLED_PROGRAM;
	                             }
					 stTrackTableInfo.iNumofMaxTrack++;
				        break;

			       case MPEG1_AUDIO_STREAM:			/* 0x03 */
			       case MPEG2_AUDIO_STREAM:			/* 0x04 */

				   /*zxg20060339 add for AC-3*/
				case AC3_AUDIO_STREAM_AUS: /* 0x06*/
				case AC3_AUDIO_STREAM:/* 0x81	*/ 
#ifdef CH_IPANEL_MGCA
                            {
                               
                                     if ( Audio_Track == CHCA_INVALID_TRACK  && stTempData16.sWord16 == ch_ServiceInfo.audio_pid )
                                     	{
                                                Audio_Track = stTrackTableInfo.iNumofMaxTrack ;
                                          
	                                           stTrackTableInfo.track_type[stTrackTableInfo.iNumofMaxTrack] = AUDIO_TRACK;
	                                           /*parse the descriptor in the second loop added on 040927*/
	                                           if(ChPmtParseDescs(Audio_Track,PMTSECOND_LOOP,&PmtData[0])) 
	                                           {
	                                                  /*stTrackTableInfo.ProgType = SCRAMBLED_PROGRAM; */
	                                                  ProgTypeSecond =  SCRAMBLED_PROGRAM;
	                                                  stTrackTableInfo.ElementType[stTrackTableInfo.iNumofMaxTrack] = SCRAMBLED_PROGRAM;							
	                                           }
                                     	}
					     else
					     	{
                                                 stTrackTableInfo.track_type [ stTrackTableInfo.iNumofMaxTrack++ ] = UNKNOWN_TRACK;
					     	}
                                           stTrackTableInfo.iNumofMaxTrack++;	
						break;
                            }
#endif					

				 
			
			       case DSMCC_STREAM:   /*0x0b,html data*/
				        stTrackTableInfo.track_type [ stTrackTableInfo.iNumofMaxTrack++ ] = DSMCC_TRACK;
#ifdef                          CHPROG_DRIVER_DEBUG
				        do_report(severity_info,"\n DSMCC Type HTML_Track\n");
#endif
				        break;

			       default:
                                    stTrackTableInfo.track_type [ stTrackTableInfo.iNumofMaxTrack++ ] = UNKNOWN_TRACK;
#ifdef                          CHPROG_DRIVER_DEBUG
				        do_report(severity_info,"\n UNKNOWN_TRACK\n");
#endif
				        break;
                    }
               }
		 pucSectionData += ( iEsInfoLength + 5 );
		 iInfoLength -= ( iEsInfoLength + 5 );
    }

#ifdef CH_IPANEL_MGCA

                 if((ProgTypeSecond == FTA_PROGRAM) && (ProgTypeFirst == SCRAMBLED_PROGRAM))
                        {
                             for(i=0;i<stTrackTableInfo.iNumofMaxTrack;i++) 
	                          stTrackTableInfo.ElementType[i] = SCRAMBLED_PROGRAM; 	   	
                        }
                         if(ChGetProgAVPid(&stTrackTableInfo,sProgIndexTemp))
                        {
		                bErrCode = true; 	  
	                       return bErrCode;
                        }	
#else
    switch(pastProgramFlashInfoTable [ sProgIndexTemp ] ->cServiceType)
    { 
		 case  DIGITAL_EPG_CHANGHONG:  /*epg application*/
		 case  DIGITAL_SYSTEM_CHANGHONG:  /*system set application*/
		 case  DIGITAL_NVOD_CHANGHONG: /*nvod application*/
		 case  DIGITAL_GAME_CHANGHONG:/*game application*/
		 case  DIGITAL_MOSAIC_CHANGHONG:  /*mosaic application*/
		 case  DIGITAL_STOCK_SERVICE:  /*stock application*/
               case  DIGITAL_HTML_SERVICE: /*html applicaton */
			   stTrackTableInfo.ProgType=ProgTypeFirst; 	
			   do_report(severity_info,"\nAPP ProgTypeFirst[%d]\n",stTrackTableInfo.ProgType); 
                    	   break;
						  
               case DIGITAL_MOSAIC_SERVICE: /*mosaic video applicaton */
		 case DIGITAL_TELEVISION_SERVICE:
		 case DIGITAL_NVOD_SHIFT_SERVICE:
		 case DIGITAL_RADIO_SERVICE:
                        if((ProgTypeSecond == FTA_PROGRAM) && (ProgTypeFirst == SCRAMBLED_PROGRAM))
                        {
                             /*do_report ( severity_info, "\n[ChParsePMTForTrackInfo==>]There is only a ca descriptor on the first loop!TotalTrackNum[%d]\n",stTrackTableInfo.iNumofMaxTrack);*/
                             for(i=0;i<stTrackTableInfo.iNumofMaxTrack;i++) 
	                          stTrackTableInfo.ElementType[i] = SCRAMBLED_PROGRAM; 	   	
                        }

                        if(ChGetProgAVPid(&stTrackTableInfo,sProgIndexTemp))
                        {
		                bErrCode = true; 	  
	                       return bErrCode;
                        }			
			   break;

               default: /* unknow application*/
  			   break;	 

 }
#endif			   

 return bErrCode;	  
	

}



/*******************************************************************************
 *Function name:  ChProgServiceTypeCheck
 * 
 *
 *Description:  parse the type of the service
 *                  
 *
 *Prototype:
 * CHCA_BOOL ChProgServiceTypeCheck(CHCA_INT  iLocalProgIndex,chca_pmt_monitor_type  ipmt_search_state)
 *       
 *input:
 *
 *output:
 *
 *Return Value:
 *       true: video and audio type,normal service
 *       false: other type
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL ChProgServiceTypeCheck(CHCA_INT  iLocalProgIndex,chca_pmt_monitor_type  ipmt_search_state)
{
        CHCA_BOOL NormalService= false;
	 
#ifdef CH_IPANEL_MGCA
       CH_Service_Info_t   ch_ServiceInfo;
       /*得到当前服务信息*/
        CH_MGCA_Ipanel_GetService(&ch_ServiceInfo);
        if(ch_ServiceInfo.video_pid != 8191 ||  ch_ServiceInfo.audio_pid != 8191)
        {
	      NormalService = true;
	      stTrackTableInfo.ProgServiceType = RadioService;
	     if(ch_ServiceInfo.video_pid != 8191)
	     	{
	          stTrackTableInfo.ProgServiceType = TelevisionService;
	     	}
        }	
#else
         stTrackTableInfo.ProgServiceType = UnknowService;
        switch(pastProgramFlashInfoTable [ iLocalProgIndex ] ->cServiceType)
        { 
		 case  DIGITAL_EPG_CHANGHONG:  /*epg application*/
#ifdef               CHPROG_DRIVER_DEBUG		 	
		 	    if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME)
		              do_report(severity_info,"\n epg application pmt update,epg track[%d] \n",APPLICATION_Track);
			    else
				do_report(severity_info,"\n epg application pmt change,epg track[%d] \n",APPLICATION_Track);
#endif		
#if 1
		 	/******* wz add for EPG广告视频的CA控制 20080401 ***************/
                NormalService = true;	 
		 	   stTrackTableInfo.ProgServiceType = TelevisionService;
			
		/**********************************/
#endif
			    break;
 
		 case  DIGITAL_SYSTEM_CHANGHONG:  /*system set application*/
#ifdef               CHPROG_DRIVER_DEBUG		 	
		 	   if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME)
		              do_report(severity_info,"\n system set application pmt update,system set track[%d] \n",APPLICATION_Track);
			   else
			   	do_report(severity_info,"\n system set application pmt change,system set track[%d] \n",APPLICATION_Track);
#endif			   
			   break;
 
		 case  DIGITAL_NVOD_CHANGHONG: /*nvod application*/
#ifdef               CHPROG_DRIVER_DEBUG		 	
		 	    if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME)
			        do_report(severity_info,"\n nvod application pmt update,nvod track[%d] \n",APPLICATION_Track);
			    else
				 do_report(severity_info,"\n nvod application pmt change,nvod track[%d] \n",APPLICATION_Track);
#endif				
			    break;
 
		 case  DIGITAL_GAME_CHANGHONG:/*game application*/
#ifdef               CHPROG_DRIVER_DEBUG		 	
		 	    if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME)
			        do_report(severity_info,"\n game application pmt update,game track[%d] \n",APPLICATION_Track);
			    else
				 do_report(severity_info,"\n game application pmt change,game track[%d] \n",APPLICATION_Track);
#endif				
			    break;
 
		 case  DIGITAL_MOSAIC_CHANGHONG:  /*mosaic application*/
#ifdef               CHPROG_DRIVER_DEBUG		 	
		 	    if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME)
                             do_report(severity_info,"\n mosaic application pmt update,mosaic track[%d] \n",APPLICATION_Track);
			    else
				 do_report(severity_info,"\n mosaic application pmt change,mosaic track[%d] \n",APPLICATION_Track);	
#endif				
			    break;

		 case DIGITAL_STOCK_SERVICE:  /*stock application*/
#ifdef               CHPROG_DRIVER_DEBUG		 	
		 	   if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 
                            do_report(severity_info,"\n stock application pmt update,stock track[%d] \n",STOCK_Track);
			   else
			   	do_report(severity_info,"\n stock application pmt change,stock track[%d] \n",STOCK_Track);
#endif			   
		         break;

               case DIGITAL_HTML_SERVICE: /*html applicaton */
#ifdef               CHPROG_DRIVER_DEBUG		 	
			   if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 	
		             do_report(severity_info,"\n html application pmt update,html track[%d] \n",HTML_Track);
			   else
			   	do_report(severity_info,"\n html application pmt change,html track[%d] \n",HTML_Track);
#endif			   
                    	  break;
						  
               case DIGITAL_MOSAIC_SERVICE: /*mosaic video applicaton */
#ifdef              CHPROG_DRIVER_DEBUG		 	
			   if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 
                            do_report(severity_info,"\n mosaic video page application pmt update,mosaic video track[%d] \n",Video_Track);
			   else
			   	do_report(severity_info,"\n mosaic video page application pmt change,mosaic video track[%d] \n",Video_Track);
#endif		
                        Audio_Track = CHCA_INVALID_TRACK;
			   NormalService = true;	
			   stTrackTableInfo.ProgServiceType = MosaicVideoService;
		         break;	

		 case DIGITAL_TELEVISION_SERVICE:
#ifdef              CHPROG_DRIVER_DEBUG		 	
                        if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 	
		              do_report(severity_info,"\n television application pmt update \n");
			   else
			   	do_report(severity_info,"\n television application pmt change \n");
#endif			   
			   NormalService = true;	 
		 	   stTrackTableInfo.ProgServiceType = TelevisionService;
			  break;

		 case DIGITAL_NVOD_SHIFT_SERVICE:
#ifdef              CHPROG_DRIVER_DEBUG		 	
                       if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 	
		              do_report(severity_info,"\n nvod shift application pmt update \n");
			  else
			   	do_report(severity_info,"\n nvod shift application pmt change \n");
#endif			  
			   NormalService = true;	 
		 	   stTrackTableInfo.ProgServiceType = TelevisionService;
			   break;

		 case DIGITAL_RADIO_SERVICE:
#ifdef              CHPROG_DRIVER_DEBUG		 	
                        if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 	
		              do_report(severity_info,"\n radio application pmt update \n");
			   else
			   	do_report(severity_info,"\n radio application pmt change \n");
#endif			   
			   NormalService = true;	 
		 	  stTrackTableInfo.ProgServiceType = RadioService;
			  break;

               default: /* unknow application*/
#ifdef              CHPROG_DRIVER_DEBUG		 	
                        if(ipmt_search_state!= CHCA_FIRSTTIME_PMT_COME) 	
		              do_report(severity_info,"\n unknow application pmt update \n");
			   else
			   	do_report(severity_info,"\n unknow application pmt change \n");
#endif			   
			   break;	 

	}
#endif								
       return (NormalService);
	   
}


/*******************************************************************************
 *Function name:  ChProgUpdateStatusCheck
 * 
 *
 *Description:  
 *                  
 *
 *Prototype: CHCA_BOOL ChProgUpdateStatusCheck(chca_pmt_monitor_type  ipmt_search_state,CHCA_INT  iLocalProgIndex)
 *          
 *       
 *input:
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
CHCA_BOOL ChProgUpdateStatusCheck(chca_pmt_monitor_type  ipmt_search_state,CHCA_INT  iLocalProgIndex)
{
       CHCA_BOOL   ProgUpdate;
#ifdef CH_IPANEL_MGCA
       CH_Service_Info_t   ch_ServiceInfo;
       /*得到当前服务信息*/
        CH_MGCA_Ipanel_GetService(&ch_ServiceInfo);
#endif	   
       VideoUpdate = AudioUpdate = false;

       if ( ipmt_search_state != CHCA_FIRSTTIME_PMT_COME )
       {
               ProgUpdate = true;
              /*pmt table update*/   
			  
		/***************************************************************************
		  *VIDEO TRACK PARSE
		  ***************************************************************************/	  
              if ( Video_Track != CHCA_INVALID_TRACK )
		{
                       if ( stTrackTableInfo.iVideo_Track != CHCA_INVALID_TRACK )
			  {
                              if ( Video_Track != stTrackTableInfo.iVideo_Track )
				  {
                                     stTrackTableInfo.iVideo_Track  =  Video_Track;
				         /*old the video stream delete*/
				         /*new the video stream add*/				  
					  /*the elemtary descripton in the pmt has been changed*/
					  
				  }
                              else
				  {
				         /*the video stream update*/
					  /*the elemtary descripton in the pmt has not been changed*/	
					  VideoUpdate= true;	
                              }
			  } 
                       else
			  {
                               stTrackTableInfo.iVideo_Track  =  Video_Track;
			  }
		}
              else
		{
		         if(stTrackTableInfo.iVideo_Track == CHCA_INVALID_TRACK)
		         {
                               VideoUpdate= true;	
			  }
			  else	 
                       {
                            /*the video track has been deleted*/
                            stTrackTableInfo.iVideo_Track = CHCA_INVALID_TRACK;
			  }				
		}
			  

              /***************************************************************************
		  *AUDIO TRACK PARSE
		  ***************************************************************************/			  
              if ( Audio_Track != CHCA_INVALID_TRACK )
		{
                       if (  stTrackTableInfo.iAudio_Track  != CHCA_INVALID_TRACK )
			  {
                                if ( Audio_Track != stTrackTableInfo .iAudio_Track )
				    {
                                         stTrackTableInfo.iAudio_Track = Audio_Track ;
                             	      /*old the audio stream delete*/
					      /*new the audio stream add*/				  
					      /*the elemtary descripton in the pmt has been changed*/
				    }
                                else
				    {
				              /*the audio stream update*/
						/*the elemtary descripton in the pmt has not been changed*/
						AudioUpdate = true; 
                                }
                        }
                        else
			   {
                                stTrackTableInfo.iAudio_Track  = Audio_Track;
			   }
		}
              else
		{
		          if(stTrackTableInfo.iAudio_Track==CHCA_INVALID_TRACK)
		          {
                               AudioUpdate = true; 
			   }
			   else	  
			   {
			          /*the audio track has been deleted*/
                               stTrackTableInfo.iAudio_Track = CHCA_INVALID_TRACK;
			   }			
		}  
	}
	else
	{
             stTrackTableInfo.iVideo_Track  =  Video_Track;
             stTrackTableInfo.iAudio_Track  = Audio_Track; 
	
	      ProgUpdate = false;
#ifdef CH_IPANEL_MGCA

 /*pmt table change*/  
             if ( stTrackTableInfo.iVideo_Track!= CHCA_INVALID_TRACK &&  ch_ServiceInfo.video_pid  != CHCA_DEMUX_INVALID_PID )
	      {
                     /*add the element stream video*/
	      }

            if ( stTrackTableInfo.iAudio_Track!= CHCA_INVALID_TRACK && ch_ServiceInfo.audio_pid != CHCA_DEMUX_INVALID_PID )
            {
                      /*add the element stream audio*/
            }	     	  
	      	 
#else
		  
	      /*pmt table change*/  
             if ( stTrackTableInfo.iVideo_Track!= CHCA_INVALID_TRACK && pastProgramFlashInfoTable [ iLocalProgIndex ] ->sVidPid != CHCA_DEMUX_INVALID_PID )
	      {
                     /*add the element stream video*/
	      }


	      if(pastProgramFlashInfoTable [ iLocalProgIndex ] ->cNoOfValidAudioStruct)		 
	      {
	            if ( stTrackTableInfo.iAudio_Track!= CHCA_INVALID_TRACK && pstBoxInfo->astAudStr[iLocalProgIndex].sAudPid != CHCA_DEMUX_INVALID_PID )
	            {
                          /*add the element stream audio*/
	            }	     	  
	      }	 
#endif		  
	  
	}
 #if 1
       /*20071204 change 如果当前为区域限制，强制设置,设置节目PMT重新LOAD*/
       if(CH_GetCurSTBStaus() == Geo_Blackout)
        {
           /*  CHMG_CtrlOperationStop();
       		task_delay(ST_GetClockSpeed()/50);*/
            VideoUpdate = AudioUpdate = false;
        }
 /*************************************************************************/
#endif

#ifdef CH_IPANEL_MGCA/*20090713增加强制更新PMT 解决跨OPI死机修改*/
#ifdef NAGRA_PRODUCE_VERSION
               VideoUpdate = AudioUpdate = false;
              return true;
#endif			  
#endif
	return (ProgUpdate);
	
}

#if 0
/*******************************************************************************
 *Function name:  ChSendMessage2PMT
 * 
 *
 *Description:  send message to pmt process
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChSendMessage2PMT ( CHCA_SHORT sProgramID,CHCA_SHORT  sTransponderID,chca_pmt_monitor_type type,CHCA_BOOL  iModuleType )
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
 *     2005-3-3   modify       add a new parameter "iModuleType"
 * 
 * 
 *******************************************************************************/
CHCA_BOOL  ChSendMessage2PMT ( CHCA_SHORT sProgramID,CHCA_SHORT  sTransponderID,chca_pmt_monitor_type type,CHCA_BOOL  iModuleType )
{
       chca_mg_cmd_t            *msg_p=NULL;
       clock_t                         cPmtTimeout;

       if( pstCHCAPMTMsgQueue != NULL)
       {
               /*cPmtTimeout = ST_GetClocksPerSecondLow();*/

               cPmtTimeout = time_plus(time_now(), ST_GetClocksPerSecond() * 1);
			   
               msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAPMTMsgQueue, &cPmtTimeout );
        
		 /*do_report(severity_info,"\n[ChSendMessage2PMT==>]msg_p[%4x]\n",msg_p);*/
               if(  msg_p != NULL)
               {
                     msg_p ->from_which_module   = CHCA_USIF_MODULE;
                     msg_p ->contents.pmtmoni.iCurProgIndex = sProgramID;
                     msg_p ->contents.pmtmoni.iCurXpdrIndex = sTransponderID;
                     msg_p ->contents.pmtmoni.cmd_received = type;
                     msg_p ->contents.pmtmoni.ModuleType = iModuleType; /*modify this on 050303*/
                     message_send ( pstCHCAPMTMsgQueue , msg_p );
                     return	false;
               }
		 else
		 {
		       do_report(severity_info,"\n[ChSendMessage2PMT==>]the msg_p is null\n");
		 }
       }

       return true;
}
#endif

/*******************************************************************************
 *Function name:  ChSendMessage2PMT
 * 
 *
 *Description:  send message to pmt process
 *                  
 *
 *Prototype:
 *BOOL  ChSendMessage2PMT ( S16 sProgramID,S16  sTransponderID,PMTStaus  iModuleType )
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
 *     2005-3-3   modify       add a new parameter "iModuleType"
 *     2005-4-5   modify       change the type of the parameter iModuleType from  "chca_bool" to "PMTStaus"
 * 
 *******************************************************************************/
BOOL  ChSendMessage2PMT ( S16 sProgramID,S16  sTransponderID,PMTStaus  iModuleType )
{
       chca_mg_cmd_t                  *msg_p=NULL;
       clock_t                               cPmtTimeout;
       chca_pmt_monitor_type      type;
       
       type = CHCA_START_BUILDER_PMT;

       if( pstCHCAPMTMsgQueue != NULL)
       {
               CH_FreeAllPMTMsg();
               cPmtTimeout = time_plus(time_now(), ST_GetClocksPerSecond() * 1);
			   
               msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAPMTMsgQueue, &cPmtTimeout );
        
		 /*do_report(severity_info,"\n[ChSendMessage2PMT==>]msg_p[%4x]\n",msg_p);*/
               if(  msg_p != NULL)
               {
                     
                     msg_p ->from_which_module   = CHCA_USIF_MODULE;
                     msg_p ->contents.pmtmoni.iCurProgIndex = (CHCA_SHORT)sProgramID;
                     msg_p ->contents.pmtmoni.iCurXpdrIndex = (CHCA_SHORT)sTransponderID;
                     msg_p ->contents.pmtmoni.cmd_received = type;
                     msg_p ->contents.pmtmoni.ModuleType = iModuleType; /*modify this on 050303*/
					 
                     message_send ( pstCHCAPMTMsgQueue , msg_p );
                     return	false;
               }
		 else
		 {
		       do_report(severity_info,"\n[ChSendMessage2PMT==>]the msg_p is null\n");
		 }
       }

       return true;
}




/*******************************************************************************
 *Function name:  ChSendMessage2CAT
 * 
 *
 *Description:  send message to cat process
 *                  
 *
 *Prototype:
 *   CHCA_BOOL  ChSendMessage2CAT ( S16 sProgramID,S16  sTransponderID,chca_cat_monitor_type type,CHCA_BOOL  Disconnected )
 *
 *
 *
 *input:
 *      
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
CHCA_BOOL  ChSendMessage2CAT( CHCA_SHORT sProgramID,CHCA_SHORT  sTransponderID,chca_cat_monitor_type type,CHCA_BOOL  Disconnected )
{
       chca_mg_cmd_t            *msg_p=NULL;
       clock_t                         cCatTimeout;

       if( pstCHCACATMsgQueue != NULL)
       {
               /*cPmtTimeout = ST_GetClocksPerSecondLow();*/
               cCatTimeout = time_plus(time_now(), ST_GetClocksPerSecond() * 1);
			   
               msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCACATMsgQueue, &cCatTimeout );
        
               if(  msg_p != NULL)
               {
                     msg_p ->from_which_module   = CHCA_USIF_MODULE;
                     msg_p ->contents.catmoni.iCurProgIndex = sProgramID;
                     msg_p ->contents.catmoni.iCurXpdrIndex = sTransponderID;
                     msg_p ->contents.catmoni.cmd_received = type;
			msg_p ->contents.catmoni.DisStatus = Disconnected;
                     message_send ( pstCHCACATMsgQueue , msg_p );
                     return	false;
               }
		 else
		 {
		       do_report(severity_info,"\n[ChSendMessage2CAT==>]the msg_p is null\n");
		 }
       }

       return true;
}


/*******************************************************************************
 *Function name:  ChPmtProcessStart
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *CHCA_BOOL   ChPmtProcessStart(CHCA_INT                           sLocalProgIndexTemp,
 *                                                  CHCA_INT                           sLocalTransponderID)
 *
 * 
 * 
 *
 *          
 *       
 *input:
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
 **********************************************************************************/
/*extern CHCA_SHORT CHMG_SectionReq(message_queue_t  *ReturnQueueId,
	                                                    CHCA_USHORT       iPidValue,
	                                                    CHCA_USHORT       iTableId);*/
CHCA_BOOL   ChPmtProcessStart(CHCA_INT                           sLocalProgIndexTemp,
                                                            CHCA_INT                           sLocalTransponderID)
{
        CHCA_BOOL	            bErrCode= false;
	 CHCA_INT                 i;	

	pmt_search_state = CHCA_FIRSTTIME_PMT_COME;
	pmt_main_state = CHCA_PMT_COLLECTED;

	memset ( PMT_Buffer , 0 , MGCA_NORMAL_TABLE_MAXSIZE );/*add this on 050328*/

	/*add this on 050310*/
	/*do_report(severity_info,"\n[ChPmtDataProcess==>]New PMT,Reset the EcmFilterInfo\n");*/
	/* EcmFilterNum = 0; add this on 050313*/  

	PMTUPDATE = false;  /*add this on 050316*/


         return bErrCode;
		 
}



/*******************************************************************************
 *Function name:  ChPmtProcessStop
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *CHCA_BOOL   ChPmtProcessStop(CHCA_USHORT                           iFilterId)
 *
 *          
 *       
 *input:
 *
 *output:
 *
 *Return Value:
 *       
 *       
 *
 *Comments:
 *     2005-3-10     reset the pmt buffer
 * 
 * 
 **********************************************************************************/
boolean   ChCaEcmProcessStop(void)
{
         CHMG_CtrlOperationStop();	
         StopAllEcmFilter();/*add this on 050324*/
}

/*boolean    ChCaFreeAllFilter(void)
{
     int    i;

     for(i=0;i<32;i++)
     {
          
     }

}*/
extern CHCA_UCHAR PmtVesion;/*加入软件判断PMT 不等模式*/
extern boolean  FreeAllMgCaFilter(void);
CHCA_BOOL   ChPmtProcessStop(CHCA_SHORT /*CHCA_USHORT   */           iFilterId)
{
	CHCA_BOOL       bErrCode = false;
	do_report(0,"\n ***********************ChPmtProcessStop ");
	{

		pmt_main_state = CHCA_PMT_UNALLOCTAED;
		if( pmt_search_state == CHCA_NONFIRSTTIME_PMT_COME )
		pmt_search_state = CHCA_FIRSTTIME_PMT_COME;
	}


	CHMG_CtrlOperationStop();	


#if   1  /*flash the cw data on the descrambler */
	ClearDescrambler();	
#endif
		 

         StopAllEcmFilter();/*add this on 050324*/

         ChResetPMTStreamInfo();
	  ChResetPMTSTrackInfo();  /*add this on 041203*/

	  /*FreeAllMgCaFilter(); add this on 050405*/
	 
	  memset ( PMT_Buffer , 0 , MGCA_NORMAL_TABLE_MAXSIZE ); /*add this on 050310*/
	  
	  EcmFilterNum=0;  /* add this on 050313*/
	  
         VideoUpdate= AudioUpdate = false;

	/*加入软件判断PMT 不等模式*/			
	PmtVesion=-1;


  #if 1
	  task_delay(ST_GetClocksPerSecond()/20);
  #else
	  task_delay(2000);	
  #endif	 
	  return 	bErrCode; 

}


/*******************************************************************************
 *Function name:  CHCA_SendFltrTimeout2CAT
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL CHCA_SendFltrTimeout2CAT( CHCA_USHORT                                  iFilterId,
 *                                                               CHCA_BOOL                                      bTableType,
 *                                                               TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
 *                                                               TMGDDIEventCode                             EvCode)
 *
 *input:
 *      
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
CHCA_BOOL CHCA_SendFltrTimeout2CAT( CHCA_USHORT                        iFilterId,
                                                                    CHCA_BOOL                                      bTableType,
                                                                    TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
                                                                    TMGDDIEventCode                             EvCode)
{
       chca_mg_cmd_t           *msg_p;
       clock_t                        cMgcaTime;

       if( pstCHCACATMsgQueue != NULL)
       {
       #if 0/*20060323 change*/
              cMgcaTime = ST_GetClocksPerSecondLow();
	   #else
           cMgcaTime = time_plus(time_now(),ST_GetClocksPerSecond()*3);

	   #endif
              msg_p = ( chca_mg_cmd_t * ) message_claim_timeout (pstCHCACATMsgQueue, &cMgcaTime );

              if(  msg_p != NULL)
              {
                      msg_p -> from_which_module = CHCA_SECTION_MODULE;
			 msg_p ->contents.sDmxfilter.TableId   = 0;
			 msg_p ->contents.sDmxfilter.iSectionFilterId   = iFilterId;
                      msg_p ->contents.sDmxfilter.TableType   = bTableType;
                      msg_p ->contents.sDmxfilter.return_status   = ExCode;
                      msg_p ->contents.sDmxfilter.TmgEventCode   = EvCode;
                      message_send ( pstCHCACATMsgQueue , msg_p );
			 /*do_report(severity_info,"\nfilter data timeout event send ok\n");		  */
                      return	false;
              }
       }
	   
       return true;
}


/*******************************************************************************
 *Function name:  CHCA_SendTimerInfo2CAT
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *          CHCA_BOOL CHCA_SendTimerInfo2CAT( CHCA_USHORT  iFilterId,CHCA_DDIEventCode  EvCode)
 *
 *input:
 *      
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
#if 0 /*add this on 050327*/
CHCA_BOOL CHCA_SendTimerInfo2CAT( CHCA_USHORT  iFilterId,CHCA_DDIEventCode  EvCode)
{
       chca_mg_cmd_t           *msg_p;
       clock_t                        cMgcaTime;

       if( pstCHCAPMTMsgQueue != NULL)
       {
              cMgcaTime = ST_GetClocksPerSecondLow();
              msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAPMTMsgQueue, &cMgcaTime );

              if(  msg_p != NULL)
              {
                      msg_p ->from_which_module = CHCA_TIMER_MODULE;


			 msg_p ->contents.sDmxfilter.TableId   = 1;  /*reserved*/
                      msg_p ->contents.sDmxfilter.TableType = 1   /*reserved*/; 
                      msg_p ->contents.sDmxfilter.iSectionFilterId   = iFilterId;
                      msg_p ->contents.sDmxfilter.return_status   = 1;
                      msg_p ->contents.sDmxfilter.TmgEventCode   = EvCode;

#if  1
                      message_send ( pstCHCAPMTMsgQueue , msg_p );
#endif
					  
                      /*do_report(severity_info,"\n[CHCA_SendTimerInfo2CAT] success to send timer info to pmt\n");*/
							  
                      return	false;
              }
       }
	   
       return true;
}
#else
CHCA_BOOL CHCA_SendTimerInfo2CAT( CHCA_USHORT  iFilterId,CHCA_DDIEventCode  EvCode)
{
       chca_mg_cmd_t           *msg_p;
       clock_t                        cMgcaTime;

       if( pstCHCACATMsgQueue != NULL)
       {
       #if 0/*20060323 change*/
              cMgcaTime = ST_GetClocksPerSecondLow();
	   #else
           cMgcaTime = time_plus(time_now(),ST_GetClocksPerSecond()*3);
	   #endif
              msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCACATMsgQueue, &cMgcaTime );

              if(  msg_p != NULL)
              {
                      msg_p ->from_which_module = CHCA_TIMER_MODULE;


			 msg_p ->contents.sDmxfilter.TableId   = 1;  /*reserved*/
                      msg_p ->contents.sDmxfilter.TableType = 1   /*reserved*/; 
                      msg_p ->contents.sDmxfilter.iSectionFilterId   = iFilterId;
                      msg_p ->contents.sDmxfilter.return_status   = 1;
                      msg_p ->contents.sDmxfilter.TmgEventCode   = EvCode;

                      message_send ( pstCHCACATMsgQueue , msg_p );
					  
                      /*do_report(severity_info,"\n[CHCA_SendTimerInfo2CAT] success to send timer info to cat\n");*/
							  
                      return	false;
              }
       }
	   
       return true;
}
#endif

/*******************************************************************************
 *Function name:  CHCA_PmtDataProcess
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 * CHCA_BOOL  ChPmtDataProcess(CHCA_USHORT                                   iFilterId)
 *
 *          
 *       
 *input:
 *
 *output:
 *
 *Return Value:
 *       
 *       
 *
 *Comments:
 *      2005-03-10     reset the ecm filter data in order that the new ecm data can be received soon 
 *                             after the ecm filter is restarted(new pmt data)
 * 
 * 
 **********************************************************************************/

CHCA_BOOL  ChPmtDataProcess(SHORT                                  iFilterId)
{
	CHCA_INT                                            sLocalProgIndexTemp,i;
	CHCA_INT                                            sLocalStreamIDTemp;
	CHCA_INT                                            sLocalNetworkIDTemp;
	CHCA_INT                                            sLocalProgNumTemp;
	CHCA_INT                                            sLocalTransponderId;

	CHCA_UINT                                          sSectionLength=0;
	CHCA_BOOL                                         bErrCode=false;
	CHCA_INT                                            PMT_Version_Num,iLen;
	CHCA_UINT                                          iPmtDataIndex;
	CHCA_BOOL                                         ResetEcmBuffer=false;  /*add this on 050310*/
	CHCA_BOOL                                         ParsePmtErr=false;
	CHCA_BOOL                                         NotParsePmt;     /*add this on 050328*/
	CHCA_BOOL                                         SamePMT=true;
#ifdef CH_IPANEL_MGCA	
    if(iFilterId < 0)
	{
		do_report(severity_info,"\n[ChPmtDataProcess==>] The filter index[%d] is wrong\n",iFilterId);
		bErrCode = true;
		return bErrCode;	  
	}
#else
	if(iFilterId>=CHCA_MAX_NUM_FILTER)
	{
		do_report(severity_info,"\n[ChPmtDataProcess==>] The filter index[%d] is wrong\n",iFilterId);
		bErrCode = true;
		return bErrCode;	  
	}
#endif
      sLocalTransponderId = stTrackTableInfo.iTransponderID;
      sLocalProgIndexTemp = stTrackTableInfo.iProgramID;	

#ifndef CH_IPANEL_MGCA	

	if(sLocalProgIndexTemp == CHCA_INVALID_LINK )
	{           
		do_report(severity_info,"\n[ChPmtDataProcess==>] Current Program Index is  invalid!\n");
		bErrCode = true; 	  
		return bErrCode;
	}

	if(sLocalTransponderId == CHCA_INVALID_LINK)
	{
		do_report(severity_info,"\n[ChPmtDataProcess==>] Current Transponder is  invalid!\n");
		bErrCode = true; 	  
		return bErrCode;
	}
#endif	
       GeoInfoStatus = -1; /*init the geographical status tag*/
#ifndef CH_IPANEL_MGCA	

       sLocalProgNumTemp = pastProgramFlashInfoTable [ sLocalProgIndexTemp ] ->stProgNo.unShort.sLo16;
       sLocalStreamIDTemp = pastTransponderFlashInfoTable [sLocalTransponderId].stTransponderNo.unShort.sLo16;
       sLocalNetworkIDTemp = pastTransponderFlashInfoTable [sLocalTransponderId].stTransponderNo.unShort.sHi16;
#endif
    CHCA_SectionDataAccess_Lock();
	 if(memcmp(( CHCA_UCHAR* ) PMT_Buffer ,( CHCA_UCHAR *) PmtSectionBuffer,MGCA_NORMAL_TABLE_MAXSIZE))
	{
		SamePMT=false;
		memcpy ( ( CHCA_UCHAR* ) PMT_Buffer,( CHCA_UCHAR *) PmtSectionBuffer,MGCA_NORMAL_TABLE_MAXSIZE );
	}
	 /*memcpy ( ( CHCA_UCHAR* ) PMT_Buffer ,( CHCA_UCHAR *) PmtSectionBuffer,MGCA_NORMAL_TABLE_MAXSIZE );*/
    CHCA_SectionDataAccess_UnLock();

	if(SamePMT)
	{
		do_report(severity_info,"\n[ChPmtDataProcess==>]Same Pmt data\n");/**/
#ifndef CH_IPANEL_MGCA	
		/*加入PMT filter 开始*/
		CH6_ReenableFilter (iFilterId);
#endif

		return false; 	
	}	   
	   
#ifndef CH_IPANEL_MGCA	
	 /*加入PMT filter 开始*/
	  CH6_ReenableFilter (iFilterId);
#endif

	 
	  PMT_Version_Num = ( PMT_Buffer [ 5 ] & 0x3e ) >> 1;
	  do_report ( severity_info,"\n PMT_Version_Num[%d], servid id[%d] tsid[%d]\n",\
		 	                               PMT_Version_Num,
		 	                               sLocalProgIndexTemp,
		 	                             sLocalTransponderId); /**/
#ifdef   CHPROG_DRIVER_DEBUG
		sSectionLength = (CHCA_UINT)(((PMT_Buffer[1] &0xf) <<8) | PMT_Buffer[2]+3);
#endif


#ifndef CH_IPANEL_MGCA	
		  
      switch(CH_GetCurApplMode())
      {
         case APP_MODE_STOCK:
         case APP_MODE_HTML:
         case APP_MODE_GAME:
	  case APP_MODE_SET:		 	
                 do_report(severity_info,"\n[ChPmtDataProcess==>]SHG APP\n");				
                 NotParsePmt = true;
                 CHCA_MepgEnable(false,true);					   	
                 break;
         
                  
          default:
	 			NotParsePmt = false;	  	
                 /*do_report(severity_info,"\n[ChPmtDataProcess==>]OTHER APP\n");*/				
                 break;
      }

	if(NotParsePmt)
		{
		   /*the pmt parsing is not needed in SHG app(stock,html,game)*/
		  return bErrCode;
		}
#endif		  
		  
	   ChParsePMTForTrackInfo (iFilterId,PMT_Buffer,sLocalProgIndexTemp);


#ifdef   CHPROG_DRIVER_DEBUG
	  do_report(severity_info,"\n PMT Section Len[%d]\n", sSectionLength);
	  for(iLen=0;iLen<sSectionLength;iLen++)
	          do_report(severity_info,"%4x ",PMT_Buffer[iLen]);
	  do_report(severity_info,"\n");

	  do_report(severity_info,"\n Program ElementPid[%d,%d]\n",stTrackTableInfo.ElementPid[0],stTrackTableInfo.ElementPid[1]);	  
#endif


#ifdef CH_IPANEL_MGCA	
        if(CHCA_CheckCardReady() )
#else
         if(CHCA_CheckCardReady() && (!ParsePmtErr))
#endif		 	
         {			
		 
                if(ChProgServiceTypeCheck(sLocalProgIndexTemp,pmt_search_state))
                {
                      TMGAPIOrigin     ipOrigin;
#ifdef CH_IPANEL_MGCA	
                    CH_Service_Info_t   ch_ServiceInfo;
                     /*得到当前服务信息*/
                     CH_MGCA_Ipanel_GetService(&ch_ServiceInfo);
                     ipOrigin.ONID   = ch_ServiceInfo.original_network_id;
                      ipOrigin.TSID  = ch_ServiceInfo.transport_stream_id;
                      ipOrigin.SvcID = ch_ServiceInfo.service_id;
                      ipOrigin.EvtID  = 2;
#else
                      ipOrigin.ONID = sLocalNetworkIDTemp;
                      ipOrigin.TSID = sLocalStreamIDTemp;
                      ipOrigin.SvcID = sLocalProgNumTemp;
                      ipOrigin.EvtID = 2;		  
#endif
			 MpegEcmProcess = false;	 /*add this on 050313*/	  


                      if(ChProgUpdateStatusCheck(pmt_search_state,sLocalProgIndexTemp))
                      {
			        		if(VideoUpdate && AudioUpdate)				  
			                {
			                          do_report(severity_info,"\n[ChPmtDataProcess==>]Program update,Track has not been changed\n"); /**/
				                   if(CHMG_ProgCtrlUpdate(PMT_Buffer))
				                   {
				                           CHMG_ProgCtrlChange(&ipOrigin,PMT_Buffer); 
				                        	
							}
									   
					        }
					        else
							{
							     do_report(severity_info,"\n[ChPmtDataProcess==>]Program update,Track of  the pmt has been changed\n"); /**/
							     CHMG_ProgCtrlChange(&ipOrigin,PMT_Buffer);
						
							}
                      }
		        	else
				        {
		                       do_report(severity_info,"\n[ChPmtDataProcess==>]The Program change!!\n"); /**/
						/*第一次PMT处理*/			 
#ifdef                   PMTDATA_TEST
		                       ECMReqId = CHMG_SectionReq(pstCHCAPMTMsgQueue,104,0x81);
#else
				               CHMG_ProgCtrlChange(&ipOrigin,PMT_Buffer);
  
#endif
				        }

		
			 if(MpegEcmProcess) 
		        {
		        
		              MpegEcmProcess = false;
                            CHCA_CheckAVRightStatus();
		        }
			 
 	
                }
		  else
		  {
		        
                      CHMG_CheckAppRight(PMT_Buffer);/*判断应用权限*/
		  }
                            
                pmt_search_state = CHCA_NONFIRSTTIME_PMT_COME;
				
         }	 
	 	 else
		  {
		
	               do_report(severity_info,"\n[ChPmtDataProcess==>]The Card is not Ready or pmt data is wrong\n");
		  }
         /* set the not match mode for the pmt table   */

         return 	bErrCode;
	  
}

#if 0
/*******************************************************************************
 *Function name:  CHCA_ReenableECMFilter
 * 
 *
 *Description: reenable the filter of the ecm
 *                  
 *
 *Prototype:
 * CHCA_BOOL    CHCA_ReenableECMFilter(CHCA_USHORT  iFilterId)
 *                                                 
 *       
 *input:
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
 **********************************************************************************/
CHCA_BOOL    CHCA_ReenableECMFilter(CHCA_USHORT  iFilterId)
{
       CHCA_BOOL                                                  bErrCode=false;

#if      /*PLATFORM_16 20070514 change*/1
       CH6_Start_Filter(iFilterId);
       FDmxFlterInfo [ iFilterId ].bdmxactived =  true;
#endif	   
	   
       return bErrCode;	 
}
#endif

/*******************************************************************************
 *Function name:  CHCA_ResetECMFilter
 * 
 *
 *Description: reset the filter of the ecm
 *                  
 *
 *Prototype:
 * CHCA_BOOL    CHCA_ResetECMFilter(CHCA_USHORT  iFilterId,CHCA_UCHAR  TableId)
 *                                                 
 *                                                 
 *
 *          
 *       
 *input:
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
 **********************************************************************************/
 #if      /*PLATFORM_16 20070514 change*/1
CHCA_BOOL    CHCA_ResetECMFilter(CHCA_USHORT  iFilterId,CHCA_UCHAR  TableId)
{
       CHCA_BOOL                                                  bErrCode=false;

       CHCA_BOOL                                                  EqualValue;
       CHCA_UCHAR                                                filterData[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                filterMask[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                ModePatern[CHCA_MGDDI_FILTERSIZE]; /*mode type select*/
       CHCA_UINT                                                   FilterDataLen=CHCA_MGDDI_FILTERSIZE;

	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHCA_EcmDataProcess==>] The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      return bErrCode;	  
       }

       /*do_report(severity_info,"\n[CHCA_ResetECMFilter==>] iFilterId[%d] TableId[%d]\n",iFilterId,TableId);*/

       CHCA_SectionDataAccess_Lock();
	memset(filterData,0,CHCA_MGDDI_FILTERSIZE);  
	memset(filterMask,0,CHCA_MGDDI_FILTERSIZE);  
	memset(ModePatern,0xff,CHCA_MGDDI_FILTERSIZE);  

	if(TableId==0)
	{
            Witch_Parity_to_be_parsed[0] = EVEN_ODD_Parity_to_be_parsed;
            Witch_Parity_to_be_parsed[1] = EVEN_ODD_Parity_to_be_parsed;
	}
	else if(TableId==1)
	{
            Witch_Parity_to_be_parsed[0] = EVEN_ODD_Parity_to_be_parsed;
            Witch_Parity_to_be_parsed[1] = EVEN_ODD_Parity_to_be_parsed;

            filterData[0]=0x80;
            filterMask[0]=0xff;
            ModePatern[0]=0xff;
            EqualValue = false;		
            
            CHCA_StopDemuxFilter(iFilterId);
            if(CHCA_Set_Filter(iFilterId,filterData,filterMask,ModePatern,FilterDataLen,EqualValue))
            {
                  bErrCode = true;
                  do_report(severity_info,"\n[CHCA_ResetECMFilter==>] set filter err \n");
                  
                  CHCA_SectionDataAccess_UnLock();
                  return bErrCode;	  
            }
            /*SectionFilter[iFilterId].bBufferLock = false;*/
            
            if(CHCA_StartDemuxFilter(iFilterId))
            {
                 bErrCode = true;
                 do_report(severity_info,"\n[CHCA_ResetECMFilter==>] start filter err \n");
                 
                 CHCA_SectionDataAccess_UnLock();
                 return bErrCode;	  
            }
            
#ifdef     CHPROG_DRIVER_DEBUG
            do_report(severity_info,"\n[CHCA_ResetECMFilter==>] Success to Reset the filter, PMT change or update, new set ECM filter\n");
#endif

	}
	else
	{
	#if 0/*zxg 20060614 change for 5105*/
              filterData[0]=TableId;
              filterMask[0]=0xff;
              ModePatern[0]=0x00;
              EqualValue = false;		
       #else
		{
		
           	#if 1
			filterData[0]=(TableId+1)%2+0x80;
		#else
			CHCA_UCHAR TempTableID;

			
			TempTableID=TableId;
			if(TempTableID==0x80)
			{
				filterData[0]=0x81;
			}
			else
			{
				filterData[0]=0x80;
			}
		#endif
		
		filterMask[0]=0xff;
              ModePatern[0]=0xff;
              EqualValue = false;
		}
	
	   #endif
              
bErrCode=CHCA_StopDemuxFilter(iFilterId);
		if(bErrCode)

		{
		
			FDmxFlterInfo [ iFilterId ].Lock = false;
			do_report(severity_info,"\n[CHCA_ResetECMFilter==>] set filter err \n");                    
                     CHCA_SectionDataAccess_UnLock();
			return bErrCode;
		}
		
		              if(CHCA_Set_Filter(iFilterId,filterData,filterMask,ModePatern,FilterDataLen,EqualValue))
              {
                    bErrCode = true;
                    do_report(severity_info,"\n[CHCA_ResetECMFilter==>] set filter err \n");
                    
                    CHCA_SectionDataAccess_UnLock();
                    return bErrCode;	  
              }
              /*SectionFilter[iFilterId].bBufferLock = false;*/
              
              if(CHCA_StartDemuxFilter(iFilterId))
              {
                     bErrCode = true;
                     do_report(severity_info,"\n[CHCA_ResetECMFilter==>] start filter err \n");
                     
                     CHCA_SectionDataAccess_UnLock();
                     return bErrCode;	  
              }
              
#ifdef     CHPROG_DRIVER_DEBUG
              do_report(severity_info,"\n[CHCA_ResetECMFilter==>] Success to Reset the filter, capture ECM data, set not equal\n");
#endif
	}

	/*SectionFilter[iFilterId].bBufferLock = false;	*/
       FDmxFlterInfo [ iFilterId ].Lock = false;
       
       CHCA_SectionDataAccess_UnLock();
	   
       return bErrCode;	 

}

/*5516*/
 CHCA_BOOL    CHCA_ResetCATFilter(CHCA_USHORT  iFilterId,
                                                                  CHCA_UCHAR  TableId,
                                                                  CHCA_UCHAR  iVersionNum,
                                                                  CHCA_UCHAR  iStartFilter,
                                                                  CHCA_UCHAR  NoEqual)
{
       CHCA_BOOL                                                  bErrCode=false;

       CHCA_BOOL                                                  EqualValue;
       CHCA_UCHAR                                                filterData[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                filterMask[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                ModePatern[CHCA_MGDDI_FILTERSIZE]; /*mode type select*/
       CHCA_UINT                                                   FilterDataLen=CHCA_MGDDI_FILTERSIZE;

	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHCA_EcmDataProcess==>] The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      return bErrCode;	  
       }

       /*do_report(severity_info,"\n[CHCA_ResetECMFilter==>] iFilterId[%d] TableId[%d]\n",iFilterId,TableId);*/

       CHCA_SectionDataAccess_Lock();
	memset(filterData,0,CHCA_MGDDI_FILTERSIZE);  
	memset(filterMask,0,CHCA_MGDDI_FILTERSIZE);  
	memset(ModePatern,0xff,CHCA_MGDDI_FILTERSIZE);  
	
	{

                filterData[0]=TableId;
                filterMask[0]=0xff;
                ModePatern[0]=0xff;
		  EqualValue = false;	
		
                if(NoEqual)
                {
                       filterData[3]=iVersionNum<<1;
                       filterMask[3]=0x3e;
                       ModePatern[3]=0x00;
                }

			  
              
              CHCA_StopDemuxFilter(iFilterId);
              if(CHCA_Set_Filter(iFilterId,filterData,filterMask,ModePatern,FilterDataLen,EqualValue))
              {
                    bErrCode = true;
                    do_report(severity_info,"\n[CHCA_ResetCatFilter==>] set filter err \n");
                    
                    CHCA_SectionDataAccess_UnLock();
                    return bErrCode;	  
              }
              /*SectionFilter[iFilterId].bBufferLock = false;*/
              
              if(CHCA_StartDemuxFilter(iFilterId))
              {
                     bErrCode = true;
                     do_report(severity_info,"\n[CHCA_ResetCatFilter==>] start filter err \n");
                     
                     CHCA_SectionDataAccess_UnLock();
                     return bErrCode;	  
              }
              
#ifdef     CHPROG_DRIVER_DEBUG
              do_report(severity_info,"\n[CHCA_ResetCatFilter==>] Success to Reset the filter\n");
#endif
	}

       FDmxFlterInfo [ iFilterId ].Lock = false;
       
       CHCA_SectionDataAccess_UnLock();
	   
       return bErrCode;	 

}
#endif


 

#if 0
CHCA_BOOL    CHCA_ResetCATFilter(CHCA_USHORT  iFilterId,
                                                                  CHCA_UCHAR  TableId,
                                                                  CHCA_UCHAR  iVersionNum,
                                                                  CHCA_UCHAR  iStartFilter,
                                                                  CHCA_UCHAR  NoEqual)
{
       CHCA_BOOL                                                  bErrCode=false;

       CHCA_UCHAR                                                filterData[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                filterMask[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                ModePatern[CHCA_MGDDI_FILTERSIZE]; /*mode type select*/
       CHCA_UINT                                                   FilterDataLen=CHCA_MGDDI_FILTERSIZE;
	CHCA_BOOL                                                  MatchMode= false;   
	CHCA_UINT                                                   slot;


	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHCA_ResetCATFilter==>] The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      return bErrCode;	  
       }


      CHCA_StopDemuxFilter(iFilterId);

       /*CHCA_SectionDataAccess_Lock();*/

	memset(filterData,0,CHCA_MGDDI_FILTERSIZE);  
	memset(filterMask,0,CHCA_MGDDI_FILTERSIZE);  
	memset(ModePatern,0xff,CHCA_MGDDI_FILTERSIZE);  

	filterData[0]=TableId;
	filterMask[0]=0xff;
	ModePatern[0]=0xff;

       if(NoEqual)
 	{
 	       MatchMode = true;
              filterData[3]=iVersionNum<<1;
              filterMask[3]=0x3e;
              ModePatern[3]=0x00;
       }


       if(CHCA_Set_Filter(iFilterId,filterData,filterMask,ModePatern,FilterDataLen,MatchMode))
       {
	      bErrCode = true;
/*#ifdef   CHPROG_DRIVER_DEBUG*/
             do_report(severity_info,"\n[CHCA_ResetCATFilter==>] fail to set the filterId[%d] \n",iFilterId);

/*#endif*/
             /*CHCA_SectionDataAccess_UnLock();*/
	      return bErrCode;	  

	}

	if(iStartFilter)
	{
               if(CHCA_Start_Filter(iFilterId)==false) 
              {
                      FDmxFlterInfo [ iFilterId ].bdmxactived =  true;
              }
               else
               {
                     do_report(severity_info,"\n[CHCA_ResetCATFilter==>]fail to start the filterId[%d]\n",iFilterId);
		       return  true;	  
	        }

               slot = stSectionFilter [ iFilterId ].iChannelID ;
        
              if(CHCA_Start_Slot(slot)==true)
              {
                     do_report(severity_info,"\n[CHCA_ResetCATFilter==>]fail to start the slot[%d]\n",slot);
		       return  true;	  
              }
 
	}
         
       /*CHCA_SectionDataAccess_UnLock();*/
	   
       return bErrCode;	 

}
#endif



/*******************************************************************************
 *Function name:  CHCA_EcmDataProcess
 * 
 *
 *Description: process the ecm data 
 *                  
 *
 *Prototype:
 * BOOL   CHCA_EcmDataProcess(U16                                                    iFilterId,
 *                                               TMGDDIEventCode                              iEvenCode,
 *                                               boolean                                              bTableType, 
 *                                               BYTE                                                  bTableId)
 *                                                 
 *                                                 
 *
 *          
 *       
 *input:
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
 **********************************************************************************/
void     StopAllEcmFilter(void)
{
      int   iFilter,i;

      i=0;
      CHCA_SectionDataAccess_Lock();	  
      for(iFilter=0;iFilter<32;iFilter++)
      {
            if(FDmxFlterInfo[iFilter].FilterType==ECM_FILTER)
            {
                FDmxFlterInfo [ iFilter ].Lock= false; /*add this on 050324*/
		  FDmxFlterInfo [ iFilter ].TTriggerMode = false;
                FDmxFlterInfo [ iFilter ].bdmxactived = false;	 
                FDmxFlterInfo [ iFilter ].InUsed = false; /*add this on 040719*/
		  FDmxFlterInfo[iFilter].MulSection = MGDDISection;	 /*add this on 041104*/	
		  FDmxFlterInfo [ iFilter ].FilterType = OTHER_FILTER; 	  /*delete this on 050313*/	
		  CHCA_MgDataNotifyUnRegister(iFilter);	 
                i++;       
	     }
      }

      EcmFilterNum = 0;	  
      CHCA_SectionDataAccess_UnLock(); 
    
      /*do_report(severity_info,"\n[StopAllEcmFilter==>]The Ecm filter count[%d]\n",i);	  */

      Witch_Parity_to_be_parsed[0] = EVEN_ODD_Parity_to_be_parsed;
      Witch_Parity_to_be_parsed[1] = EVEN_ODD_Parity_to_be_parsed;
	  
      return;	  

}

CHCA_BOOL     StopEcmFilterIndex(void)
{
      int   i;

      if(stTrackTableInfo.ProgType != FTA_PROGRAM)
      {
             CHCA_SectionDataAccess_Lock();	  
             for(i=0;i<2;i++)
             {
			CHCA_StopDemuxFilter(EcmFilterInfo[i].EcmFilterId); 
             }
             CHCA_SectionDataAccess_UnLock();  
       }
}


CHCA_BOOL     StartEcmFilterIndex(void)
{
      int   i;

      if(stTrackTableInfo.ProgType != FTA_PROGRAM)
      {
             CHCA_SectionDataAccess_Lock();	  
             for(i=0;i<2;i++)
             {
			CHCA_StartDemuxFilter(EcmFilterInfo[i].EcmFilterId); 
             }
             CHCA_SectionDataAccess_UnLock();  
     }
}


CHCA_BOOL     GetEcmFilterIndex(void)
{
      int   i,j=0;

      /*return  false;*/

      for(i=0;i<2;i++)
      {
           EcmFilterInfo[i].EcmFilterId=CHCA_MAX_NUM_FILTER;
           EcmFilterInfo[i].FilterPID=CHCA_DEMUX_INVALID_PID;
      }	   

     if(stTrackTableInfo.ProgType != FTA_PROGRAM)
     {
             CHCA_SectionDataAccess_Lock();	  
             for(i=0;i<32;i++)
             {
                       if(FDmxFlterInfo[i].FilterType==ECM_FILTER)
                       {
                              /*CHCA_RECMFilterTest(i);*/
                              if(j<2)
                              {
                                     EcmFilterInfo[j].FilterPID= FDmxFlterInfo [ i ].FilterPID;
                                     EcmFilterInfo[j++].EcmFilterId=i;
                              }				
                              else
                                    do_report(severity_info,"\n[GetEcmFilterIndex==>]The ECM Filter num[%d] is wrong \n",j);	 
                       }
             }
             CHCA_SectionDataAccess_UnLock();  
             
             /*for(i=0;i<j;i++)
                   CHCA_ResetECMFilter(EcmFilterInfo[i].EcmFilterId,2,true);*/
             
             if(j==1)
             {
                    EcmFilterInfo[1].EcmFilterId= EcmFilterInfo[0].EcmFilterId;
                    EcmFilterInfo[1].FilterPID= EcmFilterInfo[0].FilterPID;
                    /*do_report(severity_info,"\n[GetEcmFilterIndex==>] The video and audio with the same filter\n");*/	   
             }
      }

#if  0 /*add this on 050501*/
      StartEcmFilterIndex();
#endif

     for(i=0;i<2;i++)
     {
         if(EcmFilterInfo[i].EcmFilterId!=CHCA_MAX_NUM_FILTER)
           CHCA_ResetECMFilter(EcmFilterInfo[i].EcmFilterId,1);	 /*add this on 050505*/	
     }		 


     /* Witch_Parity_to_be_parsed[0] = EVEN_ODD_Parity_to_be_parsed;
      Witch_Parity_to_be_parsed[1] = EVEN_ODD_Parity_to_be_parsed;*/
	 

      /*do_report(severity_info,"\n[GetEcmFilterIndex==>] ECM Filter[%d][%d]\n",EcmFilterInfo[0].EcmFilterId,EcmFilterInfo[1].EcmFilterId);	*/

}



boolean Parity_Judgement( BYTE sECM_table_id, BYTE *Parity_parse )
{
   if ( *Parity_parse == EVEN_ODD_Parity_to_be_parsed )
   {
      if ( sECM_table_id == ECM_EVEN_TABLE_ID )
      {
         *Parity_parse = EVEN_Parity_to_be_parsed;
      }
      else
      {
         *Parity_parse = ODD_Parity_to_be_parsed;
      }
   }
   else
   {
      if ( *Parity_parse == EVEN_Parity_to_be_parsed )
      {
         if ( sECM_table_id == ECM_EVEN_TABLE_ID )
         {
            /*do_report(severity_info,"have parsed the EVEN TABLE ID\n");*/
            return ( false );
         }
         else
         {
            *Parity_parse = ODD_Parity_to_be_parsed;
         }
      }
      else
      {
         if ( sECM_table_id == ECM_ODD_TABLE_ID )
         {
            /*do_report(severity_info,"have parsed the ODD TABLE ID\n");*/
            return ( false );
         }
         else
         {
            *Parity_parse = EVEN_Parity_to_be_parsed;
         }
      }
   }
   return ( true );
}



BOOL   CHCA_EcmDataProcess(U16                                                    iFilterId,
                                                           TMGDDIEventCode                              iEvenCode,
                                                           boolean                                              bTableType,
                                                           BYTE                                                  bTableId)
{
       CHCA_BOOL                                                  bErrCode=true;
	CHCA_USHORT                                              MgFFilterIndex,i;
 	TMGDDIFilterReport                                        pFilterReport; 

       CHCA_UCHAR                                                EcmTableId;
       CHCA_UCHAR                                                filterData[CHCA_MGDDI_FILTERSIZE];
       CHCA_UCHAR                                                filterMask[CHCA_MGDDI_FILTERSIZE];
       
       CHCA_UCHAR                                                ModePatern[8]; /*mode type select*/
       CHCA_UINT                                                   FilterDataLen=CHCA_MGDDI_FILTERSIZE;
	CHCA_BOOL                                                  ResetEcmFilter=false;   
	CALLBACK                                                     MgDataNotifyTemp;
    
 	MgFFilterIndex = iFilterId + 1;

	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHCA_EcmDataProcess==>] The filter index[%d] is wrong\n",iFilterId);
	      return bErrCode;	  
       }

       CHCA_SectionDataAccess_Lock();
	   
	pFilterReport.Size = (uword)((( FDmxFlterInfo[iFilterId].DataBuffer [ 1 ] & 0x0f ) << 8 ) | FDmxFlterInfo[iFilterId].DataBuffer[ 2 ] + 3); 
	       if((pFilterReport.Size<=3) || (pFilterReport.Size>=MGCA_PRIVATE_TABLE_MAXSIZE))	   
       {
#ifdef      CHPROG_DRIVER_DEBUG
                do_report(severity_info,"\n[CHCA_EcmDataProcess==>] Section len[%d] is  wrong \n",pFilterReport.Size);

#endif
                /*CHCA_SectionDataAccess_Lock();
                FDmxFlterInfo [ iFilterId ].Lock = false;
                CHCA_SectionDataAccess_UnLock();*/
                FDmxFlterInfo [ iFilterId ].Lock = false;
               CHCA_SectionDataAccess_UnLock();
                return bErrCode;
	}
#if 0/*060119 xjp comment*/
       /*copy the ecm section data*/
	if(memcmp(ECM_Buffer,FDmxFlterInfo[iFilterId].DataBuffer,pFilterReport.Size)==0)
	{
		do_report(0,"\nThe same ECM section data is coming!\n");
                FDmxFlterInfo [ iFilterId ].Lock = false;
		CHCA_SectionDataAccess_UnLock();
		return bErrCode;
	}
#endif
       memcpy(ECM_Buffer,FDmxFlterInfo[iFilterId].DataBuffer,MGCA_PRIVATE_TABLE_MAXSIZE);	
       MgDataNotifyTemp = FDmxFlterInfo[iFilterId].cMgDataNotify; 
	/*CHCA_ReenableECMFilter(iFilterId); add this on 050425*/
	/*FDmxFlterInfo [ iFilterId ].Lock = false; 060113 xjp comment*/
	 
	CHCA_SectionDataAccess_UnLock();	


       if(MgDataNotifyTemp==NULL)
       {
             /*do_report(severity_info,"\n[CHCA_EcmDataProcess==>]ECM Callback is null,The ECM Filter has been freed \n");*/
             /*CHCA_SectionDataAccess_Lock();
             FDmxFlterInfo [ iFilterId ].Lock = false;
             CHCA_SectionDataAccess_UnLock();*/
            
	      return 	bErrCode;
       }

       /*if(EcmDataStop)
       {
          do_report(severity_info,"\nEcmDataStop,do not process this ecm data\n");
	   return false;	
       }   */

       /*for(i = 0; i < 2; i ++)
       {
             if(EcmFilterInfo[i].EcmFilterId != CHCA_MAX_NUM_FILTER)
		 break;	 	
	}

	if(i==2)
	{
             do_report(severity_info,"\n[CHCA_EcmDataProcess==>]The Program is FTA \n");
             return  false;
	}*/


#if  0	   
       for(i = 0; i < 2; i ++)
       {
		if(iFilterId == EcmFilterInfo[i].EcmFilterId)
              {
                    /*do_report(severity_info,"\n Current_MG_TrackIndex[%d] \n",i);*/
                    break;
              }
      }

	if(i<2)    
	{
             /*do_report(severity_info,"\n Can not find the valid track index \n");*/
            /* return bErrCode;*/

             if(!Parity_Judgement(ECM_Buffer [ 0 ],&Witch_Parity_to_be_parsed[i]))
             {
                  return bErrCode;
	      }
	   
	     /*do_report(severity_info,"\n[CHCA_EcmDataProcess==>]ECM Table[%d]\n",ECM_Buffer [ 0 ]);*/
	     /*bErrCode = CHCA_ResetECMFilter(iFilterId,i,true); delete this on 050308*/ 
	}
#endif	

      /* if(bErrCode )
       {
               do_report(severity_info,"\nCHCA_ResetECMFilter err\n");
               return bErrCode;
	}*/

	  
	/*pFilterReport.Size = (uword)((( ECM_Buffer [ 1 ] & 0x0f ) << 8 ) | ECM_Buffer[ 2 ] + 3);*/   

#ifdef    CHPROG_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_EcmDataProcess==>]ECM DATA[Length=%d]:",pFilterReport.Size);		  
       for(i=0;i<10;i++)
             do_report(severity_info,"%4x",ECM_Buffer[i]);	
       do_report(severity_info,"\n");	
#endif	


    /*  do_report(severity_info,"\n[CHCA_EcmDataProcess==>]iFilterId[%d] ECMData[%d]\n",iFilterId,ECM_Buffer[0]);*/	

       EcmTableId = ECM_Buffer[0];
       pFilterReport.bTable = bTableType;
       pFilterReport.pData = ECM_Buffer; 

	/*do_report(severity_info,"\n[CHCA_EcmDataProcess==>]11111111111111111111111111\n");   */
	   
       CHCA_ResetECMFilter(iFilterId, ECM_Buffer[0]);



	/*do_report(severity_info,"\n222222222222222222222222222\n");   */
   

       if(CHCA_CheckCardReady())			 
       {   
               bErrCode=false;
	       /*do_report(severity_info,"\nECM DATA PROCESS START\n");*/
		ECMDataProcessing = true; /*delete this on 050311*/
		MpegEcmProcess = false; /*add this on 050221*/   
              semaphore_wait(pSemMgApiAccess);		 
	     
            MgDataNotifyTemp((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); 
			  
	  		CHCA_CaDataOperation();
			   
              if(CardContenUpdate)
              {
                 
                    CardContenUpdate = false;
#if  1		
					 CHCA_ResetECMFilter(iFilterId,0);	
#endif
                  
	      		}


		   
	       ECMDataProcessing = false; 
             
	       semaphore_signal(pSemMgApiAccess);
             	   
		if(MpegEcmProcess) 
		{
		     MpegEcmProcess = false;
                   CHCA_CheckAVRightStatus();
		}
		   
			  
       }
	else
	{
                do_report(severity_info,"\n[CHCA_EcmDataProcess==>]The Card is not ready \n");
	}


        
        return 	bErrCode;

}



/*******************************************************************************
 *Function name:  CHCA_TimerDataProcess
 * 
 *
 *Description: process the emm data 
 *                  
 *
 *Prototype:
 *BOOL   CHCA_TimerDataProcess(U16                                                    iFilterId,
 *                                                TMGDDIEvSrcDmxFltrTimeoutExCode     iExCode,
 *                                                 Message_Module_t                               ModuleType)
 *                                                 
 *                                                 
 *
 *          
 *       
 *input:
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
 **********************************************************************************/
#if  0 
BOOL   CHCA_TimerDataProcess(U16                                                    iFilterId,
                                                             TMGDDIEvSrcDmxFltrTimeoutExCode     iExCode,
                                                             Message_Module_t                               ModuleType)
{
           BOOL    bErrCode = false;
	    U16       MgFFilterIndex;	   

           if(iFilterId>=CHCA_MAX_NUM_FILTER)
           {
                  bErrCode = true;
		    do_report(severity_info,"\n[CHCA_TimerDataProcess::] Current Filter Index is wrong\n");		  
		    return bErrCode; 		  
	    }

	    MgFFilterIndex = iFilterId + 1;
	   
           switch(ModuleType)
           {
                    case CHCA_SECTION_MODULE:
                             semaphore_wait(pSemMgApiAccess);
                             if(FDmxFlterInfo[iFilterId].cMgDataNotify!=NULL)
		                     FDmxFlterInfo[iFilterId].cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrTimeout,iExCode,NULL); 												 	
				 semaphore_signal(pSemMgApiAccess);						 
				 break;

		      case CHCA_TIMER_MODULE:
			        CHCA_TimerOperation(iFilterId);
				 break;
                                         
	   }

	   return  bErrCode;	   

}
#endif

BOOL   CHCA_SectionTimeoutProcess(U16                                                    iFilterId,
                                                             TMGDDIEvSrcDmxFltrTimeoutExCode              iExCode)
{
        BOOL          bErrCode = false;
        U16            MgFFilterIndex;	   
        
        if(iFilterId>=CHCA_MAX_NUM_FILTER)
        {
               bErrCode = true;
               do_report(severity_info,"\n[CHCA_SectionTimeoutProcess::] Current Filter Index is wrong\n");		  
               return bErrCode; 		  
        }
        
        MgFFilterIndex = iFilterId + 1;

        semaphore_wait(pSemMgApiAccess);
        if(FDmxFlterInfo[iFilterId].cMgDataNotify!=NULL)
           FDmxFlterInfo[iFilterId].cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrTimeout,iExCode,NULL); 												 	
        semaphore_signal(pSemMgApiAccess);						 

        /*task_status(ptidCATMonitorTask,&Status,task_status_flags_stack_used);
        do_report(severity_info,"\n[CHCA_TimerOperation==>CHCA_TIMER_MODULE] stack used = %d\n",Status.task_stack_used);	*/
		
        return  bErrCode;	   

}



#if 1  /*add this on 050327*/
/*add this on 050429*/
void   CatDataBufferInit(void)
{
       CHCA_SectionDataAccess_Lock();
       memset(CAT_Buffer,0,MGCA_NORMAL_TABLE_MAXSIZE);
	CHCA_SectionDataAccess_UnLock();	
}
#ifdef CH_IPANEL_MGCA
static int gi_lastCatVersion  = -1;
#endif
/*******************************************************************************
 *Function name:  CHMG_CatDataProcess
 * 
 *
 *Description: process the emm data 
 *                  
 *
 *Prototype:
 *CHCA_BOOL   CHMG_CatDataProcess(CHCA_USHORT                                                    iFilterId,
 *                                                        TMGDDIEventCode                              iEvenCode,
 *                                                        CHCA_BOOL                                              bTableType,
 *                                                        CHCA_UCHAR                                                  bTableId)
 *                                                 
 *                                                 
 *
 *          
 *       
 *input:
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
 
 **********************************************************************************/
CHCA_BOOL   CHMG_CatDataProcess(CHCA_USHORT                                                    iFilterId,
                                                                    TMGDDIEventCode                                              iEvenCode,
                                                                    CHCA_BOOL                                                       bTableType,
                                                                    CHCA_UCHAR                                                  bTableId)
{
       CHCA_BOOL                                                            bErrCode=false;
       CHCA_USHORT                                                        MgFFilterIndex;
       TMGDDIFilterReport                                                 pFilterReport; 
       CHCA_UINT                                                            iCatDataIndex;
       CALLBACK                                                              cMgDataNotify; 
       CHCA_USHORT                                                       catversion;   
       /*CHCA_USHORT                                                    TmpTimerHandle = CHCA_MAX_NO_TIMER; add this on 041119*/
       CHCA_BOOL                                                            NotProcess=false;
       
       SHORT	                                                               tempsCurProgram;
       SHORT                                                                   tempCurTransponderId;		   


 	MgFFilterIndex = iFilterId + 1;

	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHMG_CatDataProcess==>] The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      return bErrCode;	  
       }

       CHCA_SectionDataAccess_Lock();
	/*CatFilterId =  iFilterId;  add this on 041118*/    
       /*copy the emm section data*/
	/*memcpy(CAT_Buffer,CatSectionBuffer,MGCA_NORMAL_TABLE_MAXSIZE);	*/	
       /*catversion = ( CatSectionBuffer[5]&0x3e ) >> 1;
       do_report ( severity_info,"\n[CHMG_CatDataProcess==>]CAT_Version_Num[%d] \n",catversion);*/


#if  0
	if(FDmxFlterInfo[iFilterId].bdmxactived)
	{
              CHCA_ResetCATFilter(iFilterId,0x1,catversion,false,true);
	}
	else
	{
             CHCA_SectionDataAccess_UnLock();
	      return;	 	 
	}
#endif

       /*if(!memcmp(CatSectionBuffer,CAT_Buffer,MGCA_NORMAL_TABLE_MAXSIZE))
       {
             NotProcess=true;
       }*/

	/*memcpy(CAT_Buffer,FDmxFlterInfo[iFilterId].DataBuffer,MGCA_NORMAL_TABLE_MAXSIZE);*/		
	memcpy(CAT_Buffer,CatSectionBuffer,MGCA_NORMAL_TABLE_MAXSIZE);		
	cMgDataNotify =  FDmxFlterInfo[iFilterId].cMgDataNotify;  
       /*FDmxFlterInfo [ iFilterId ].Lock = false; */
    
       catversion = ( CAT_Buffer[5]&0x3e ) >> 1;
                    
	   
#ifdef   CHPROG_DRIVER_DEBUG	   
       do_report ( severity_info,"\n[CHMG_CatDataProcess==>]CAT_Version_Num[%d] \n",catversion);
#endif

	CHCA_SectionDataAccess_UnLock();	

#if 0 /*delete this on 050311*/
       if(CHCA_GetDataType(CAT_Buffer[0]) != CAT_DATA)
       {
             do_report(severity_info,"\n[CHMG_CatDataProcess==>] the data is not the cat section\n");
	      bErrCode = true;
	      return bErrCode;	  
 	}
#endif	   

#if  0
#ifndef     NAGRA_PRODUCE_VERSION
       CHCA_ResetCATFilter(iFilterId,0x1,catversion,true,true);
#endif
#endif

       /*if(NotProcess)
       {
              do_report(severity_info,"\n[CHMG_CatDataProcess==>]Same CAT not process\n");
              return false;
	}*/

      pFilterReport.bTable = bTableType;
      pFilterReport.Size = (uword)((( CAT_Buffer[ 1 ] & 0x0f ) << 8 ) | CAT_Buffer[ 2 ] + 3); 
      if((pFilterReport.Size>3) && (pFilterReport.Size<MGCA_NORMAL_TABLE_MAXSIZE))
      {
            /*catversion = ( CAT_Buffer [ 5 ] & 0x3e ) >> 1;*/

#ifdef   CHPROG_DRIVER_DEBUG	
             do_report(severity_info,"\nCAT Data::");
             for(iCatDataIndex=0;iCatDataIndex<pFilterReport.Size;iCatDataIndex++)
             {
                    do_report(severity_info,"%4x",CAT_Buffer[iCatDataIndex]);
             }
             do_report(severity_info,"\n");
#endif		

#ifdef   CHPROG_DRIVER_DEBUG	
{
            int  t1,t2;
            t1=(CAT_Buffer[15]&0x20)>>5;
	     t2=(CAT_Buffer[15]&0x40)>>6;			
          			
            do_report(severity_info,"\nScram[%d],Fta[%d] \n",t1,t2);
             t1=(CAT_Buffer[19]&0x20)>>5;
             t2=(CAT_Buffer[19]&0x40)>>6;	

	      do_report(severity_info,"\nScram[%d],Fta[%d] \n",t1,t2);

}			
#endif

 
      
             pFilterReport.pData = CAT_Buffer; 

             if(CHCA_CheckCardReady())
             {
                   CatReady = true;
				   
                   if(cMgDataNotify!=NULL)			 
                   {   
                          semaphore_wait(pSemMgApiAccess);
			     CATDataProcessing=true;	 /*add this on 050116*/
#if 1				 
			     cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); /**/
#endif
				 /*CHCA_CaDataOperation();add this on 050311*/
			     /*do_report(severity_info,"\n[CHMG_CatDataProcess==>]3333333333333333333333333333\n");*/
			     /*CHCA_CatDataOperation();	 */
                          /*CHCA_InitMgEmmQueue();
			     do_report(severity_info,"\n[CHMG_CatDataProcess==>]New Cat,Reset the EMM Queue\n");*/ 
			      CATDataProcessing=false;/*add this on 050116*/
			 

			     /*CHMG_CaContentUpdate();	 */				
                          semaphore_signal(pSemMgApiAccess);
                   }

#if  0
                   if(TmpTimerHandle<CHCA_MAX_NO_TIMER)
                   {
                        CHCA_TimerOperation(TmpTimerHandle);
                   }			

                   task_status(ptidCATMonitorTask,&Status,task_status_flags_stack_used);
		     do_report(severity_info,"\n[CHMG_CatDataProcess==>] stack used = %d\n",Status.task_stack_used);		  
				
                   CHCA_ResetCATFilter(iFilterId,0x1,catversion,true,true);
#endif				   
#if 0/*def CH_IPANEL_MGCA/*20090713 add*/
{
             SHORT tempsCurProgram;
		SHORT tempCurTransponderId;
	      if(gi_lastCatVersion == -1)
	  	{
                   gi_lastCatVersion = catversion;
	  	}
	      else if(gi_lastCatVersion != catversion)
	  	{
	  	       do_report(0,"catversion = %d\n",catversion);

                        
                             tempsCurProgram = CH_GetsCurProgramId();
	                      tempCurTransponderId = CH_GetsCurTransponderId();
				 gi_lastCatVersion = catversion;
				 
				 if(CHCA_CheckCardReady())		  
		              { 
		                    if(ChSendMessage2PMT(tempsCurProgram,tempCurTransponderId,START_PMT_CTP)==true)
		                   {
                                       do_report(0,"\n[CHMG_UsrNotify==>] fail to send the 'START_PMT_NCTP' message\n");
				     }
                                 CHCA_InsertLastPMTData();
                         }
               }
}
#endif

                   /*CHCA_ResetCATFilter(iFilterId,0x1,catversion,true,true);*/
             }
		  
      }	 		 
      else
      {
             do_report(severity_info,"\n[CHMG_CatDataProcess==>]The cat data len is wrong\n");
             bErrCode = true;				  
      }

      return bErrCode;
	   
}
#endif

#if 0 /*add this on 050119,just for test*/
void   CHMG_EcmTemp(void)
{
	CHCA_USHORT                                               MgFFilterIndex;
 	TMGDDIFilterReport                                        pFilterReport; 
	CHCA_UINT                                                   iCatDataIndex;
       CALLBACK                                                     cMgDataNotify; 


 	MgFFilterIndex = EcmFilterId + 1;

	if(EcmFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHMG_EcmTemp==>] The filter index[%d] is wrong\n",EcmFilterId);
	      return;	  
       }

	cMgDataNotify =  FDmxFlterInfo[EcmFilterId].cMgDataNotify;  

       pFilterReport.bTable = 1;
       pFilterReport.Size = (uword)((( ECM_Buffer[ 1 ] & 0x0f ) << 8 ) | ECM_Buffer[ 2 ] + 3); 
       if((pFilterReport.Size>3) &&(pFilterReport.Size<MGCA_NORMAL_TABLE_MAXSIZE))
       {

#ifdef   CHPROG_DRIVER_DEBUG	
             do_report(severity_info,"\nEcm Data::");
             for(iCatDataIndex=0;iCatDataIndex<pFilterReport.Size;iCatDataIndex++)
             {
                   do_report(severity_info,"%4x",ECM_Buffer[iCatDataIndex]);
             }
             do_report(severity_info,"\n");
#endif			 
      
             pFilterReport.pData = ECM_Buffer; 
				   
             if(cMgDataNotify!=NULL)			 
             {   
                    cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); 
                    CHCA_EcmDataOperation();	 
                    CHMG_GetOPIInfoList(0);	
             }
      }	 		 
      else
      {
             do_report(severity_info,"\n[CHMG_EcmTemp==>]The ecm data len is wrong\n");
      }

      return;

}
#endif

#if   0/*delete this on 050311*/
void   CHMG_CatTemp(void)
{
	CHCA_USHORT                                               MgFFilterIndex;
 	TMGDDIFilterReport                                         pFilterReport; 
	CHCA_UINT                                                    iCatDataIndex;
       CALLBACK                                                      cMgDataNotify; 


 	MgFFilterIndex = CatFilterId + 1;
	if(CatFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHMG_CatTemp==>] The filter index[%d] is wrong\n",CatFilterId);
	      return;	  
       }

	cMgDataNotify =  FDmxFlterInfo[CatFilterId].cMgDataNotify;  

       pFilterReport.bTable = 1;
       pFilterReport.Size = (uword)((( CAT_Buffer[ 1 ] & 0x0f ) << 8 ) | CAT_Buffer[ 2 ] + 3); 
       if((pFilterReport.Size>3) &&(pFilterReport.Size<MGCA_NORMAL_TABLE_MAXSIZE))
       {
#ifdef   CHPROG_DRIVER_DEBUG	
             do_report(severity_info,"\nCAT Data::");
             for(iCatDataIndex=0;iCatDataIndex<pFilterReport.Size;iCatDataIndex++)
             {
                   do_report(severity_info,"%4x",CAT_Buffer[iCatDataIndex]);
             }
             do_report(severity_info,"\n");
#endif			 
      
             pFilterReport.pData = CAT_Buffer; 
				   
             if(cMgDataNotify!=NULL)			 
             {   
                    /*EmmCardUpdated=true;*/
                    cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); 
                    CHCA_CatDataOperation();
		      /*do
		      {
                         task_delay(300);
		      }while(!CardContenUpdate);*/
		 			
		     /*if(CardContenUpdate)*/
		      {
		            /*CardContenUpdate = false;*/
                          CHMG_GetOPIInfoList(0);	
		     }

             }
      }	 		 
      else
      {
             do_report(severity_info,"\n[CHMG_CatTemp==>]The cat data len is wrong\n");
      }

      return;


}
#endif

/*******************************************************************************
 *Function name:  CHMG_EMMDataProcess
 * 
 *
 *Description: process the emm data 
 *                  
 *
 *Prototype:
 *CHCA_BOOL   CHMG_EMMDataProcess(CHCA_USHORT                                                    iFilterId,
 *                                                           TMGDDIEventCode                              iEvenCode,
 *                                                           CHCA_BOOL                                              bTableType,
 *                                                          CHCA_UCHAR                                                  bTableId)
 *                                                 
 *                                                 
 *
 *          
 *       
 *input:
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
 **********************************************************************************/
/*modify this on 041129*/
#if 1
CHCA_BOOL   CHMG_EMMDataProcess(CHCA_USHORT                                               iFilterId,
                                                                      TMGDDIEventCode                                          iEvenCode,
                                                                      CHCA_BOOL                                                   bTableType,
                                                                      CHCA_UCHAR                                                  bTableId)
{ 
       CHCA_BOOL                                                  bErrCode=false;
	CHCA_USHORT                                              MgFFilterIndex;
 	TMGDDIFilterReport                                        pFilterReport; 
	CHCA_UINT                                                   EmmDataIndex;
       CALLBACK                                                     cMgDataNotify; 
	CHCA_UINT                                                   i,QEmmLen=0;   
	/*CHCA_BOOL                                                  ErrProcess;*/
     	CHCA_UINT                                                   LLen = 0;
 	MgFFilterIndex = iFilterId + 1;

	/*do_report(severity_info,"\n[CHMG_EMMDataProcess==>] Emm Process start\n");*/


	semaphore_wait( psemSectionEmmQueueAccess );
	/*do_report(severity_info,"\n MgEmmFilterData.OnUse [%d]\n",MgEmmFilterData.OnUse);  */  

	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHMG_EMMDataProcess==>]The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      MgEmmFilterData.OnUse = false; 	  
	      semaphore_signal(psemSectionEmmQueueAccess);	  
	      return bErrCode;	  
       }

	if((MgEmmFilterData.aucMgSectionData == NULL)/*||\
	    (CHCA_GetDataType(MgEmmFilterData.aucMgSectionData[0]) != EMM_DATA)*/) 
	{
             do_report(severity_info,"\n[CHMG_EMMDataProcess==>]The emm data is wrong\n");
	      bErrCode = true;
	      MgEmmFilterData.OnUse = false; 	  
	      semaphore_signal(psemSectionEmmQueueAccess);	  
	      return bErrCode;	  
	}

	memset(EMM_Buffer,0,MGCA_PRIVATE_TABLE_MAXSIZE);
 	memcpy(EMM_Buffer,MgEmmFilterData.aucMgSectionData,MGCA_PRIVATE_TABLE_MAXSIZE);

       /*MgEmmFilterData.OnUse = false;  	*/
 	cMgDataNotify = FDmxFlterInfo[iFilterId].cMgDataNotify;
 	semaphore_signal( psemSectionEmmQueueAccess );

	CHCA_MgEmmSectionUnlock(); /*add this on 050115*/

       pFilterReport.bTable = bTableType;

#if   0/*just for test when the crc data of emm is wrong,what are the card do?*/
       EMM_Buffer[25]=0xff;
#endif

       pFilterReport.Size = (uword)((( EMM_Buffer[ 1 ] & 0x0f ) << 8 ) | EMM_Buffer[ 2 ] + 3); 
       pFilterReport.pData  = EMM_Buffer; 

#ifdef   CHPROG_DRIVER_DEBUG
       do_report(severity_info,"\nEMM DATA Len[%d]:",pFilterReport.Size);	
	for(EmmDataIndex=0;EmmDataIndex<pFilterReport.Size;EmmDataIndex++)
	{
              do_report(severity_info,"%4x",EMM_Buffer[EmmDataIndex]);	
	}
       do_report(severity_info,"\n");
#endif


#if 0/*delete this on 050607*/
#if      /*PLATFORM_16 20070514 change*/1
#ifndef     NAGRA_PRODUCE_VERSION  /*add this for deal with "channel 13 problem" on 050227*/
      if(EMM_Buffer[0]==0x84 && EMM_Buffer[2]==0xad)
      	{
              return bErrCode;
	}
#endif	  
#endif
#endif

#if 0
	 if(EMM_Buffer[0]==0x86)
      	{
      		int iLoop;
		do_report(0,"EMM DATA");
		for(iLoop = 0;iLoop < MGCA_PRIVATE_TABLE_MAXSIZE;iLoop++)
		{
			if(iLoop % 10 == 0)
			{
				do_report(0,"\n");
			}
			do_report(0,"0x%02x ",EMM_Buffer[iLoop]);
		}

		do_report(0,"\n");
            
	}
#endif
       if(CHCA_CheckCardReady())
       {
             if((pFilterReport.Size>3) && (pFilterReport.Size<MGCA_PRIVATE_TABLE_MAXSIZE))
             {
                     if(cMgDataNotify!=NULL)			 
                    {   
                           semaphore_wait(pSemMgApiAccess);
                           /*do_report(severity_info,"\nEMM DATA PROCESS START\n");*/
#ifdef                 PAIR_CHECK 						   
			      CheckEmmP(pFilterReport.pData);			   
#endif
                           EMMDataProcessing = true;
                           cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); 
                           CHCA_CaEmmDataOperation();/*delete  this on 050311*/
                           /*CHCA_CaDataOperation();*//*add this on 050311*/
                           EMMDataProcessing = false;  
                           /*CHMG_CaContentUpdate();*/
						   
                           /*do_report(severity_info,"\nEMM DATA PROCESS END\n");*/
                           semaphore_signal(pSemMgApiAccess);

                           /*task_status(ptidCATMonitorTask,&Status,task_status_flags_stack_used);
              	      do_report(severity_info,"\n[CHMG_EMMDataProcess==>] stack used = %d\n",Status.task_stack_used);		  */
		     }
             }
             else
             {
                   do_report(severity_info,"\n[CHMG_EMMDataProcess==>]The emm data len is wrong\n");
                   bErrCode = true;				  
             }

	      /*CHCA_SectionBufferUnlock(iFilterId,bTableId,bTableType,iEvenCode);*/
       }
	else
	{
              do_report(severity_info,"\n[CHMG_EMMDataProcess==>] The Card is not Ready\n");
	}

	/*CHCA_MgEmmSectionUnlock(); add this on 050115*/

	/*do_report(severity_info,"\n[CHMG_EMMDataProcess==>] Emm Process end\n");*/

       return bErrCode;
	   
}
#else
CHCA_BOOL   CHMG_EMMDataProcess(CHCA_USHORT                                               iFilterId,
                                                                      TMGDDIEventCode                                          iEvenCode,
                                                                      CHCA_BOOL                                                   bTableType,
                                                                      CHCA_UCHAR                                                  bTableId)
{ 
       CHCA_BOOL                                                            bErrCode=false;
	CHCA_USHORT                                                               MgFFilterIndex;
 	TMGDDIFilterReport                                        pFilterReport; 
	CHCA_UINT                                                   EmmDataIndex;
       CALLBACK                                                     cMgDataNotify; 
	CHCA_UINT                                                   i,QEmmLen=0;   
	CHCA_BOOL                                                  ErrProcess;
     	CHCA_UINT                                                   LLen = 0;

 	MgFFilterIndex = iFilterId + 1;

	semaphore_wait( psemSectionEmmQueueAccess );
	if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHMG_EMMDataProcess==>]The filter index[%d] is wrong\n",iFilterId);
	      bErrCode = true;
	      MgEmmFilterData.OnUse = false; 	  
	      semaphore_signal(psemSectionEmmQueueAccess);	  
	      return bErrCode;	  
       }

	if((MgEmmFilterData.aucMgSectionData == NULL)||\
	    (CHCA_GetDataType(MgEmmFilterData.aucMgSectionData[0]) != EMM_DATA)) 
	{
             do_report(severity_info,"\n[CHMG_EMMDataProcess==>]The emm data is wrong\n");
	      bErrCode = true;
	      MgEmmFilterData.OnUse = false; 	  
	      semaphore_signal(psemSectionEmmQueueAccess);	  
	      return bErrCode;	  
	}

	memset(EMM_Buffer,0,MGCA_PRIVATE_TABLE_MAXSIZE);
 	memcpy(EMM_Buffer,MgEmmFilterData.aucMgSectionData,MGCA_PRIVATE_TABLE_MAXSIZE);
 	cMgDataNotify = FDmxFlterInfo[iFilterId].cMgDataNotify;
 	semaphore_signal( psemSectionEmmQueueAccess );
 
       pFilterReport.bTable = bTableType;
       pFilterReport.Size = (uword)((( EMM_Buffer[ 1 ] & 0x0f ) << 8 ) | EMM_Buffer[ 2 ] + 3); 
       pFilterReport.pData  = EMM_Buffer; 

 
#if   1
       do_report(severity_info,"\nEMM DATA Len[%d]:",pFilterReport.Size);	
	for(EmmDataIndex=0;EmmDataIndex<pFilterReport.Size;EmmDataIndex++)
	{
              do_report(severity_info,"%4x",EMM_Buffer[EmmDataIndex]);	
	}
       do_report(severity_info,"\n");
#endif	   
 
       if(CHCA_CheckCardReady())
       {
             if((pFilterReport.Size>3) && (pFilterReport.Size<MGCA_PRIVATE_TABLE_MAXSIZE))
             {
                     if(cMgDataNotify!=NULL)			 
                    {   
                           semaphore_wait(pSemMgApiAccess);	

                           /*do_report(severity_info,"\nEMM DATA PROCESS START\n");*/
			      CheckEmmP(pFilterReport.pData);			   
			      EMMDataProcessing = true;
                           cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); 
                           ErrProcess = CHCA_EmmDataOperation();
#ifdef                 NAGRA_PRODUCE_VERSION
                           CHMG_CaContentUpdate();
#endif	 			   
                           EMMDataProcessing = false;
                           /*do_report(severity_info,"\nEMM DATA PROCESS END\n");*/

                           QEmmLen = CHCA_GetQEmmLen();

                           /*LLen = CHCA_MgEmmSectionUnlock();*/

			      if((QEmmLen>=1) && (!ErrProcess))			   
			      /*if((LLen>=1) && (!ErrProcess))	*/		   
			      {
			            for(i=0;i<7;i++)	
			           {
					      LLen = CHCA_MgEmmSectionUnlock();
						  
			                    if(LLen)
			                    {
                                                pFilterReport.bTable = 0;
                                                pFilterReport.Size = (uword)((( MgEmmFilterData.aucMgSectionData[ 1 ] & 0x0f ) << 8 ) | MgEmmFilterData.aucMgSectionData[ 2 ] + 3); 
                                                pFilterReport.pData  = MgEmmFilterData.aucMgSectionData; 
              				      MgFFilterIndex	= MgEmmFilterData.iSectionSlotId + 1;

#if 1								  
                                               do_report(severity_info,"\nEMM DATA Len[%d]:",pFilterReport.Size);	
	                                        for(EmmDataIndex=0;EmmDataIndex<pFilterReport.Size;EmmDataIndex++)
	                                        {
                                                      do_report(severity_info,"%4x",MgEmmFilterData.aucMgSectionData[EmmDataIndex]);	
	                                        }
                                               do_report(severity_info,"\n");
#endif											   

                                                /*do_report(severity_info,"\nEMM DATA PROCESS START\n");*/
                                                EMMDataProcessing = true;
						      CheckEmmP(pFilterReport.pData);			   
              				      cMgDataNotify = FDmxFlterInfo[MgEmmFilterData.iSectionSlotId].cMgDataNotify; 
              				      cMgDataNotify((HANDLE)MgFFilterIndex,MGDDIEvSrcDmxFltrReport,NULL,(dword)&pFilterReport); 
       					      ErrProcess = CHCA_EmmDataOperation();
#ifndef                                      NAGRA_PRODUCE_VERSION
                                                CHMG_CaContentUpdate();
#endif
                                                EMMDataProcessing = false;
                                                /*do_report(severity_info,"\nEMM DATA PROCESS END\n");*/

						      if(ErrProcess)	  
						      {
                                                      break;
						      }
       			             }
					      else
					      {
                                               break;
					      }

					      /*LLen = CHCA_MgEmmSectionUnlock();*/
						  
			            }
			      }	  
                           semaphore_signal(pSemMgApiAccess);

                           /*task_status(ptidCATMonitorTask,&Status,task_status_flags_stack_used);
              	      do_report(severity_info,"\n[CHMG_EMMDataProcess==>] stack used = %d\n",Status.task_stack_used);		  */
                    }
             }
             else
             {
                   do_report(severity_info,"\n[CHMG_EMMDataProcess==>]The emm data len is wrong\n");
                   bErrCode = true;				  
             }

	      /*CHCA_SectionBufferUnlock(iFilterId,bTableId,bTableType,iEvenCode);*/
       }
	else
	{
              do_report(severity_info,"\n[CHMG_EMMDataProcess==>] The Card is not Ready\n");
	}

       semaphore_wait( psemSectionEmmQueueAccess );
       MgEmmFilterData.OnUse = false; 
       semaphore_signal( psemSectionEmmQueueAccess );		
	
       return bErrCode;
	   
}
#endif




/*******************************************************************************
 *Function name:  ChDVBPMTMonitor
 * 
 *
 *Description:  process the pmt section
 *                  
 *
 *Prototype:
 *          void  ChDVBPMTMonitor ( void   *pvParam )
 *       
 *
 *input:
 *      
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



void  ChDVBPMTMonitor ( void   *pvParam )
{ 
       chca_mg_cmd_t                                                  *msg_p=NULL;
       CHCA_INT                                                           sLocalProgIndex/*,sLocalStreamID,sLocalProgNum*/;
       TMGDDIEventCode                                              iEvenCode;
       TMGDDIEvSrcDmxFltrTimeoutExCode                   iExCode;	  
	CHCA_BOOL                                                       bTableType;  
	CHCA_UINT                                                        iMessIndex;

	CHCA_BOOL                                                       StartPmt;

	clock_t                                                               timer_temp;

       while ( TRUE )
       {
              if((msg_p = ( chca_mg_cmd_t * ) message_receive_timeout ( pstCHCAPMTMsgQueue,TIMEOUT_INFINITY))==NULL)
              {
                    continue;
		}

              /*do_report(severity_info,"\n[ChDVBPMTMonitor==>] ReceiveMsg_p[%x]\n",msg_p);		                   */
		switch ( msg_p ->from_which_module )
		{
                     case CHCA_USIF_MODULE :
                             switch (  msg_p ->contents.pmtmoni.cmd_received )
                             {
                                     case CHCA_START_BUILDER_PMT:
						       StartPmt = true;			 	
							if ( pmt_main_state != CHCA_PMT_UNALLOCTAED)			  	
                                                 {
                                                       /*CHCA_BOOL     StartPmt=true;  add this on 050328*/



                                                       /*if((msg_p ->contents.pmtmoni.ModuleType !=START_MGCA) && (msg_p ->contents.pmtmoni.ModuleType !=START_MGCA_EPG))*/
                                                       if(msg_p ->contents.pmtmoni.ModuleType != START_MGCA_EPG)
                                                       {
                                                             
                                                             if(ChPmtProcessStop(iFilterId_requested)==true)
                                                             {
                                                                   do_report(severity_info,"\n[ChDVBPMTMonitor==>]fail to stop the pmt process\n");/**/
                                                             }
								else
									iFilterId_requested=-1;
							       }
		                                 }/*20061120 MOVE TO HERE*/

#if 1  /*add this on 050421*/
                                                       EcmDataStop=true;

                                                       {
                                                             switch(msg_p ->contents.pmtmoni.ModuleType)
                                                             {
                                                                    case START_PMT_UPDATE:  /*when the card content has been changed*/
                                                                             break;
																		
                                                                    case START_PMT_NCTP:/*normal status*/
											break;

									     case STOP_PMT: /*enter into the html app*/
										 	StartPmt = false;
										 	break;

									     case START_MGCA: /*the stream has been closed,need reconnected it*/
#if 1  /*add this on 050423*/
                                                                             MGCAStop=false;
				
#endif								
									#ifdef CHPROG_DRIVER_DEBUG
											do_report(severity_info,"\n Initiate Successfully, Start MGCA ! \n");
									#endif
										 	CHCA_InitMgEmmQueue();	 /*delete this on 050302*/
										 	/*CHCA_RestartTimer();*/
											/*memset(CAT_Buffer,0,1024);*/ /*add this on 050327*/
#if  1	 /*add this on 050510*/										
											CHMG_SRCStatusChange(MGDDISrcDisconnected); 
#endif
											CHMG_SRCStatusChange(MGDDISrcConnected);
										 	break;

									     case START_PMT_CTP:/*the transponder has been changed*/
										 	CHCA_InitMgEmmQueue();	 /*delete this on 050302*/
										 	/*CHCA_RestartTimer();*/
										 	CHMG_SRCStatusChange(MGDDISrcTrpChanged);
										 	break;

              							    case STOP_MGCA: /*extract card, search program,search epg, disconnected*/ 
#if 1  /*add this on 050423*/
                                                                            MGCAStop=true;
				                                                
#endif								
										      StartPmt = false;	
                                                                            /*CHCA_RestartTimer();*/
                                                                            CHMG_SRCStatusChange(MGDDISrcDisconnected); 
              								      CHCA_InitMgEmmQueue();	 /*delete this on 050302*/							  
              								      break;	
											

									     case START_MGCA_EPG: /*epg app, need reconnected it,but don't parse the pmt of the epg again*/
#if 1  /*add this on 050423*/
                                                                             MGCAStop=false;
				
#endif								
										 	CHCA_InitMgEmmQueue();	 /*delete this on 050302*/
										 	StartPmt = false;
										 	/*CHCA_RestartTimer();*/
											/*memset(CAT_Buffer,0,1024); */ /*add this on 050327*/
											CHMG_SRCStatusChange(MGDDISrcConnected);
										 	break;
											
								     }
							      }

	
												   
                                                       if(StartPmt)
                                                       {
 #ifndef CH_IPANEL_MGCA

                                                             if(msg_p ->contents.pmtmoni.iCurProgIndex == CHCA_INVALID_LINK)
                                                             {
                                                                   do_report(severity_info,"\n[ChDVBPMTMonitor==>] Current Program Index is  invalid !\n");
                                                                   break;					 
                                                             }   
                                                             
                                                             if(stTrackTableInfo.iTransponderID != msg_p ->contents.pmtmoni.iCurXpdrIndex)
                                                             {
#ifdef                                                          CHPROG_DRIVER_DEBUG                                                      
                                                                    do_report(severity_info,"\n[ChDVBPMTMonitor==>] New Transponder \n");
#endif
                                                                    ChCaNewTransponder = true; 							 
                                                             }
                                                             else
                                                             {
                                                                   ChCaNewTransponder = false;
                                                             }			
 #endif                                                            
                                                             stTrackTableInfo.iProgramID = sLocalProgIndex = msg_p ->contents.pmtmoni.iCurProgIndex;
                                                             stTrackTableInfo.iTransponderID = msg_p ->contents.pmtmoni.iCurXpdrIndex;


													   
                                                             if(ChPmtProcessStart(sLocalProgIndex,stTrackTableInfo.iTransponderID)== true)
                                                             {
                                                                  do_report(severity_info,"\n[ChDVBPMTMonitor==>]fail to start the pmt process\n");
							            }
                                                       }
							      else
							      {
                                                              pmt_search_state = CHCA_FIRSTTIME_PMT_COME;
                                                              pmt_main_state = CHCA_PMT_COLLECTED;
                                                              /*do_report(severity_info,"\n[ChDVBPMTMonitor==>]No pmt start\n");*/
							      }
#endif			                        
                                             break;

    					 
                                     default:
                                            break;
                             }
                             break;

                     case CHCA_SECTION_MODULE :

  			        /*do_report(severity_info,"\n[ChDVBPMTMonitor==>] Receive the 'PMT DATA'\n");	*/
		               {
                                    CHCA_MGData_t     DataType;
					 BYTE                      TableId;				

		                      iFilterId_received = msg_p ->contents.sDmxfilter.iSectionFilterId;
			               iEvenCode = msg_p ->contents.sDmxfilter.TmgEventCode;
			               iExCode = msg_p ->contents.sDmxfilter.return_status;
			               bTableType = msg_p ->contents.sDmxfilter.TableType;	
					 TableId = msg_p ->contents.sDmxfilter.TableId;

	                             /*DataType =  CHCA_GetDataType(msg_p ->contents.sDmxfilter.TableId);*/			  
				 
                                    switch(TableId)
		                      {
                                           case PMT_DATA:
#ifdef CH_IPANEL_MGCA				 						
                                                         if(ChPmtDataProcess(iFilterId_received)==true)
						                {
                                                               do_report(severity_info,"\n Fail to deal with  the pmt Data\n");
						                 }
#else
			   	                        if(CH_GetCurApplMode()!=APP_MODE_SET)
	
						          {
						                if(ChPmtDataProcess(iFilterId_received)==true)
						                {
                                                               do_report(severity_info,"\n Fail to deal with  the pmt Data\n");
						                 }
			   	                        }	
#endif										
						          break;

							   


				               default:
							   do_report(severity_info,"\n[ChDVBPMTMonitor==>]Unkonw data type\n"); 	  	
							   break;	  	
				         }
			        }
                             break;


					  
                     default :
                            break;
        }
        message_release ( pstCHCAPMTMsgQueue, ( void * ) msg_p );

   }

}


#if   1 /*delete this on 050301 */
/****************************************************************************************
 *Function name:  ChDVBECMMonitor
 * 
 *
 *Description:  process mg ca data including ecm
 *                  
 *
 *Prototype:
 *          static  void   ChDVBECMMonitor( void   *pvParam )
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     data type:
 *               MGDDIEvSrcDmxFltrReport   
 *          	   MGDDIEvSrcDmxFltrTimeout
 *     add this function on 040823
 *     modify  this on 040828  add the cat data process 
 ****************************************************************************************/
static  void   ChDVBECMMonitor( void   *pvParam )
{
        chca_mg_cmd_t                                               *msg_p=NULL; 
	 U16                                                                 iFilterIndex;	
	 TMGDDIEvSrcDmxFltrTimeoutExCode                ExCode;
	 boolean                                                           bTableType;
	 TMGDDIEventCode                                           EvCode;
	 BYTE                                                               bTableId;
	 Message_Module_t                                           ModuleType;
	 CHCA_MGData_t                                             DataType;
 
        while(true)
        {
                msg_p = ( chca_mg_cmd_t * ) message_receive( pstCHCAECMMsgQueue);
                if ( msg_p != NULL )
                {
                        iFilterIndex = msg_p ->contents.sDmxfilter.iSectionFilterId;
                        EvCode = msg_p ->contents.sDmxfilter.TmgEventCode;
                        ExCode = msg_p->contents.sDmxfilter.return_status;
			   bTableType = msg_p ->contents.sDmxfilter.TableType;
                        bTableId = msg_p ->contents.sDmxfilter.TableId;  			
			   ModuleType = msg_p ->from_which_module;

			   if(iFilterIndex>=CHCA_MAX_NUM_FILTER)
			   {
                               do_report(severity_info,"\n[ChDVBECMMonitor==>] Filter  index[%d] is wrong \n",iFilterIndex);           
			   }
			   else
			   {
			      
					   
				   if(CH_GetCurApplMode()!=APP_MODE_SET)	   

                               {
                                     if(CHCA_EcmDataProcess(iFilterIndex,EvCode,ExCode,bTableType)==true)
				         {
                                         /* do_report(severity_info,"\n Fail to deal with  the Ecm Data\n");*/
				         }
			      	   }
 		          }
                        message_release ( pstCHCAECMMsgQueue, msg_p );
		  }
	 }

}
#endif



/****************************************************************************************
 *Function name:  ChDVBEMMMonitor
 * 
 *
 *Description:  process mg emm data 
 *                  
 *
 *Prototype:
 *          static  void   ChDVBEMMMonitor( void   *pvParam )
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     data type:
 *               MGDDIEvSrcDmxFltrReport   
 *          	   MGDDIEvSrcDmxFltrTimeout
 *     add this function on 040823
 *     modify  this on 040828  add the cat data process 
 ****************************************************************************************/
/*add  this on 050115*/
static  void   ChDVBEMMMonitor( void   *pvParam )
{
        chca_mg_cmd_t                                               *msg_p=NULL; 
	 U16                                                                 iFilterIndex;	
	 TMGDDIEvSrcDmxFltrTimeoutExCode                ExCode;
	 boolean                                                           bTableType;
	 TMGDDIEventCode                                           EvCode;
	 CHCA_MGData_t                                              bTableId;
	 CHCA_MGData_t                                              DataType;

	 chca_cat_monitor_type                                     cat_main_state = CHCA_CAT_UNALLOCTAED;

        while(true)
        {
                msg_p = ( chca_mg_cmd_t * ) message_receive_timeout ( pstCHCAEMMMsgQueue,TIMEOUT_INFINITY );

		  if(msg_p == NULL)	
		  {
                       continue;
		  }

                iFilterIndex = msg_p ->contents.sDmxfilter.iSectionFilterId;
                EvCode = msg_p ->contents.sDmxfilter.TmgEventCode;
                ExCode = msg_p->contents.sDmxfilter.return_status;
                bTableType = msg_p ->contents.sDmxfilter.TableType;
                bTableId = msg_p ->contents.sDmxfilter.TableId;  	

#if 1 /*add this on 050424*/
               if(!MGCAStop)
#endif			   	
               {
                      if( CHMG_EMMDataProcess(iFilterIndex,
                                                                  EvCode,
                                                                  bTableType,
                                                                  bTableId)==true)
                      {
                           do_report(severity_info,"\n[ChDVBCATMonitor==>]Fail to process the emm data! \n");
                      }
			else
			 {
                           /* do_report(severity_info,"\n[ChDVBCATMonitor==>]success to process the emm data! \n")*/;

			}
               }
               message_release ( pstCHCAEMMMsgQueue, msg_p );
	 }
}

#if  1
/****************************************************************************************
 *Function name:  ChDVBCATMonitor
 * 
 *
 *Description:  process mg cat  data
 *                  
 *
 *Prototype:
 *          static  void   ChDVBCATMonitor( void   *pvParam )
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     data type:
 *               MGDDIEvSrcDmxFltrReport   
 *          	   MGDDIEvSrcDmxFltrTimeout
 *     add this function on 040823
 *     modify  this on 040828  add the cat data process 
 ****************************************************************************************/
/*modify this on 041122*/
/*CHCA_SHORT                                                          CatFilterIdTT = CHCA_MAX_NUM_FILTER;    add this just for test on 050302*/
#if 1
static  void   ChDVBCATMonitor( void   *pvParam )
{
        chca_mg_cmd_t                                               *msg_p=NULL; 
	 U16                                                                iFilterIndex;	
	 TMGDDIEvSrcDmxFltrTimeoutExCode               ExCode;
	 boolean                                                         bTableType;
	 TMGDDIEventCode                                         EvCode;
	 CHCA_MGData_t                                                             bTableId;
	 /*Message_Module_t                                         ModuleType;*/
	 CHCA_MGData_t                                            DataType;

	 chca_cat_monitor_type                                   cat_main_state = CHCA_CAT_UNALLOCTAED;

        while(true)
        {
                msg_p = ( chca_mg_cmd_t * ) message_receive_timeout ( pstCHCACATMsgQueue,TIMEOUT_INFINITY );

		  if(msg_p == NULL)	
		  {
                       continue;
		  }

#if 1  /*add this on 050423*/
                if(!MGCAStop)
#endif								
                {
		       switch ( msg_p ->from_which_module )
		      {
                           case CHCA_SECTION_MODULE :
				  /*if ( cat_main_state == CHCA_CAT_COLLECTED)*/
				  {
                                     iFilterIndex = msg_p ->contents.sDmxfilter.iSectionFilterId;
					  /*CatFilterIdTT = 	iFilterIndex;	delete this on 050311*/		 
                                     EvCode = msg_p ->contents.sDmxfilter.TmgEventCode;
                                     ExCode = msg_p->contents.sDmxfilter.return_status;
		    	                bTableType = msg_p ->contents.sDmxfilter.TableType;
                                     bTableId = msg_p ->contents.sDmxfilter.TableId;  			
                                     switch(EvCode)
                                     {
                                              case MGDDIEvSrcDmxFltrReport:
                                                       /*DataType = CHCA_GetDataType(bTableId);*/
                                                       switch(bTableId)
                                                       {
					                            case  CAT_DATA:
										   CAT_NoSignal = false;			
                                                                        if( CHMG_CatDataProcess(iFilterIndex,
                                                                                                                  EvCode,
                                                                                                                  bTableType,
                                                                                                                  bTableId)==true)
                                                                        {
                                                                               /*do_report(severity_info,"\n[ChDVBCATMonitor==>]Fail to process the cat data! \n");*/
							                       } 
  						 	                       break;

					                            default:
						 	                     do_report(severity_info,"\n[ChDVBCATMonitor==>] Unkonw Data type!\n");
						 	                     break;
				                           }
				    		             break;

					           case MGDDIEvSrcDmxFltrTimeout:
							      if(!CAT_NoSignal)
							      {   
							           CAT_NoSignal=true;
									   
				                                do_report(severity_info,"\n[ChDVBCATMonitor==>] Dmx Fltr Timeout\n");		 
					   	            								      
							      }	  
					   	             if(CHCA_SectionTimeoutProcess(iFilterIndex,ExCode)==true)
					   	            {
                                                            do_report(severity_info,"\n[ChDVBCATMonitor==>]Fail to process the timer data! \n");
						            }
							     /*do_report(severity_info,"\n[ChDVBCATMonitor==>]CAT_NoSignal[%d]\n",CAT_NoSignal);	*/	 
					   	            break;
				        }
				  }
				  break;	 

			      case CHCA_TIMER_MODULE:
  				 /* if ( cat_main_state == CHCA_CAT_COLLECTED)*/
  				  {
                                    iFilterIndex = msg_p ->contents.sDmxfilter.iSectionFilterId;
                                    EvCode = msg_p ->contents.sDmxfilter.TmgEventCode;
                                    ExCode = msg_p->contents.sDmxfilter.return_status;
                                    bTableType = msg_p ->contents.sDmxfilter.TableType;
                                    bTableId = msg_p ->contents.sDmxfilter.TableId;  			

                                    if(!CAT_NoSignal)
    			                  CHCA_TimerOperation(iFilterIndex);
                                    /* task_status(ptidCATMonitorTask,&Status,task_status_flags_stack_used);
                                   do_report(severity_info,"\n[CHCA_TimerOperation==>CHCA_TIMER_MODULE] stack used = %d\n",Status.task_stack_used);	*/
                                   /*do_report(severity_info,"\n[ChDVBCATMonitor==>]CAT_NoSignal[%d]\n",CAT_NoSignal);*/
				  }
				  break;
		       }
                }
                message_release ( pstCHCACATMsgQueue, msg_p );
	 }
}
#else
static  void   ChDVBCATMonitor( void   *pvParam )
{
        chca_mg_cmd_t                                               *msg_p=NULL; 
	 U16                                                                iFilterIndex;	
	 TMGDDIEvSrcDmxFltrTimeoutExCode               ExCode;
	 boolean                                                         bTableType;
	 TMGDDIEventCode                                         EvCode;
	 CHCA_MGData_t                                                             bTableId;
	 /*Message_Module_t                                         ModuleType;*/
	 CHCA_MGData_t                                            DataType;

	 chca_cat_monitor_type                                   cat_main_state = CHCA_CAT_UNALLOCTAED;

        while(true)
        {
                msg_p = ( chca_mg_cmd_t * ) message_receive_timeout ( pstCHCACATMsgQueue,TIMEOUT_INFINITY );

		  if(msg_p == NULL)	
		  {
                       continue;
		  }

		  switch ( msg_p ->from_which_module )
		  {
                     case CHCA_USIF_MODULE :
                             switch (  msg_p ->contents.catmoni.cmd_received )
                             {
                                     case CHCA_START_BUILDER_CAT:
						    /*do_report(severity_info,"\n[ChDVBCATMonitor==>] Receive the 'CHCA_START_BUILDER_CAT'\n");	*/		 	
                                              if ( cat_main_state == CHCA_CAT_UNALLOCTAED)
                                              {
                                                       S16   iSrcChangedStatus;
											  
                                                       /*do_report(severity_info,"\n[ChDVBCATMonitor==>] ProgIndex[%d] TransIndex[%d]\n",\
													  msg_p ->contents.pmtmoni.iCurProgIndex,
													  msg_p ->contents.pmtmoni.iCurXpdrIndex);*/
													  
 			                                  if(msg_p ->contents.catmoni.iCurProgIndex == CHCA_INVALID_LINK)
		                                         {
                                                            do_report(severity_info,"\n[ChDVBCATMonitor==>] Current Program Index is  invalid !\n");
			                                       break;					 
		                                         }   

							      if(msg_p ->contents.catmoni.iCurXpdrIndex == CHCA_INVALID_LINK)	
							      {
                                                            do_report(severity_info,"\n[ChDVBCATMonitor==>] Current Transponder Index is  invalid !\n");
			                                       break;					 
							      }


#if  1
							      switch(msg_p ->contents.catmoni.DisStatus)
							      {
                                                             case 0:
										/*CHMG_SRCStatusChange(MGDDISrcTrpChanged);	*/				 	
						                            break;

								     case 1:
									 	/*CHCA_RestartTimer();*/
									 	/*do_report(severity_info,"\n[ChDVBCATMonitor==>] before MGDDISrcConnected\n");*/
									 	CHMG_SRCStatusChange(MGDDISrcConnected);	
										/*do_report(severity_info,"\n[ChDVBCATMonitor==>] end MGDDISrcConnected\n");*/
										CHCA_ResetCatStartPMT();
										/*do_report(severity_info,"\n[ChDVBCATMonitor==>] end MGDDISrcConnected\n");*/
									 	break;
										
								      case 2:
									 	break;	 

								      case 3:
									  	/*CHCA_RestartTimer();*/
									  	/*do_report(severity_info,"\n[ChDVBCATMonitor==>] before MGDDISrcTrpChanged\n");*/
									  	CHMG_SRCStatusChange(MGDDISrcTrpChanged);
										/*do_report(severity_info,"\n[ChDVBCATMonitor==>] end MGDDISrcTrpChanged\n");*/
									 	break;	 	
										

							      }
                                                       /*if(msg_p ->contents.catmoni.DisStatus) 
                                                       {
                                                             iSrcChangedStatus = MGDDISrcConnected;
							      }
							      else
							      {
                                                             iSrcChangedStatus = MGDDISrcTrpChanged;
							      }
								  
							      CHMG_SRCStatusChange(iSrcChangedStatus);	*/
							      CHCA_InitMgEmmQueue();	 /*delete this on 050302*/
#endif								  
     					                    cat_main_state = CHCA_CAT_COLLECTED;
			                         }
                                              break;

                                     case CHCA_STOP_BUILDER_CAT:
    						    /*do_report(severity_info,"\n[ChDVBCATMonitor==>] Receive the 'CHCA_STOP_BUILDER_CAT'\n");*/		 	
                                              if ( cat_main_state != CHCA_CAT_UNALLOCTAED )
                                              {

	#if 1										  
                                                       /*CHCA_InitMgEmmQueue();	*/
							      switch(msg_p ->contents.catmoni.DisStatus)
							      {
                                                               case 0:
								                break;	
												
                                                               case 1:
									  	  /*do_report(severity_info,"\n[ChDVBCATMonitor==>] before MGDDISrcDisconnected\n");*/
										  /*CHCA_RestartTimer();*/
									         CHMG_SRCStatusChange(MGDDISrcDisconnected);							   	
										  /*do_report(severity_info,"\n[ChDVBCATMonitor==>] end MGDDISrcDisconnected\n");*/	 
								                break;	
												
                                                               case 2:
								                break;	

                                                               default:
										  break;					   	

							      }
 #endif

#if 0
                                     
							      CHCA_StopDemuxFilter(CatFilterIdTT);	  
#endif
								  
#if 0                                                   	
                                                       if(msg_p ->contents.catmoni.DisStatus) 
                                                       {
								     CHMG_SRCStatusChange(MGDDISrcDisconnected);	

								     /*stop the cat and emm,init the emm queue*/		   
                                                             /*CHCA_EmmQueueInit(); add this on 041125*/
							      }
#endif	

#if 0

                                                       if(CatTimerHandle<CHCA_MAX_NO_TIMER)
							      {    
							              CHCA_SetTimerStop(CatTimerHandle);						   
                                                       }
													   
							      CHCA_ResetCATFilter(CatFilterId,0x1,0,false,false); 
#endif								  
								  
							      cat_main_state = CHCA_CAT_UNALLOCTAED; 						   
                                              }
                                              break;
    					 
                                     default:
                                            break;
                             }
                             break;

                     case CHCA_SECTION_MODULE :
				  if ( cat_main_state == CHCA_CAT_COLLECTED)
				  {
                                     iFilterIndex = msg_p ->contents.sDmxfilter.iSectionFilterId;
					  /*CatFilterIdTT = 	iFilterIndex;	delete this on 050311*/		 
                                     EvCode = msg_p ->contents.sDmxfilter.TmgEventCode;
                                     ExCode = msg_p->contents.sDmxfilter.return_status;
		    	                bTableType = msg_p ->contents.sDmxfilter.TableType;
                                     bTableId = msg_p ->contents.sDmxfilter.TableId;  			
                                     switch(EvCode)
                                     {
                                              case MGDDIEvSrcDmxFltrReport:
                                                       /*DataType = CHCA_GetDataType(bTableId);*/
                                                       switch(bTableId)
                                                       {
 #if 0                                                      
                                                               case  EMM_DATA:
                                                                        if( CHMG_EMMDataProcess(iFilterIndex,
                                                                                                                   EvCode,
                                                                                                                   bTableType,
                                                                                                                   bTableId)==true)
                                                                        {
                                                                                do_report(severity_info,"\n[ChDVBCATMonitor==>]Fail to process the emm data! \n");
							                       }
                                                                        break;
#endif																		

					                            case  CAT_DATA:
                                                                        /*if( CHMG_CatDataProcess(iFilterIndex,
                                                                                                                  EvCode,
                                                                                                                  bTableType,
                                                                                                                  bTableId)==true)
                                                                        {
                                                                               do_report(severity_info,"\n[ChDVBCATMonitor==>]Fail to process the cat data! \n");
							                       }*/
#if 0
									          if(CHCA_StopBuildPmt(sCurProgramId,sCurTransponderId,0))
                                                                         {
                                                                                do_report(severity_info,"\n[Card module::] fail to send the 'Stop PMT' message\n");
				                                             }
				            
								  
                                                                         if(CHCA_StartBuildPmt(sCurProgramId,sCurTransponderId,0))
                                                                         {
                                                                                do_report(severity_info,"\n[Card module::] fail to send the 'Start PMT' message\n");
				                                             }	
#endif																		 
 						 	                       break;

					                            default:
						 	                     do_report(severity_info,"\n[ChDVBCATMonitor==>] Unkonw Data type!\n");
						 	                     break;
				                           }
				    		             break;

					           case MGDDIEvSrcDmxFltrTimeout:
					   	             if(CHCA_SectionTimeoutProcess(iFilterIndex,ExCode)==true)
					   	            {
                                                            do_report(severity_info,"\n[ChDVBCATMonitor==>]Fail to process the timer data! \n");
						            }
					   	            break;
				        }
				  }
				  break;	 

			case CHCA_TIMER_MODULE:
  				  if ( cat_main_state == CHCA_CAT_COLLECTED)
  				  {
                                    iFilterIndex = msg_p ->contents.sDmxfilter.iSectionFilterId;
                                    EvCode = msg_p ->contents.sDmxfilter.TmgEventCode;
                                    ExCode = msg_p->contents.sDmxfilter.return_status;
                                    bTableType = msg_p ->contents.sDmxfilter.TableType;
                                    bTableId = msg_p ->contents.sDmxfilter.TableId;  			

    			               CHCA_TimerOperation(iFilterIndex);
                                    /* task_status(ptidCATMonitorTask,&Status,task_status_flags_stack_used);
                                   do_report(severity_info,"\n[CHCA_TimerOperation==>] stack used = %d\n",Status.task_stack_used);	*/	  
	   
				  }
				  break;
		  } 
                message_release ( pstCHCACATMsgQueue, msg_p );
	 }
}
#endif
#endif
/*******************************************************************************
 *Function name:  ChCtrlInstanceInit
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          void  ChCtrlInstanceInit ( void )
 *       
 *
 *input:
 *      
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
CHCA_BOOL  ChCtrlInstanceInit ( void )
{
        CHCA_UINT               CtrlIndex; 	
        CHCA_BOOL              bErrCode=false;		

	 pSemCtrlOperationAccess = semaphore_create_fifo(1);

	 if(pSemCtrlOperationAccess==NULL)
	 {    
	       bErrCode = true;
	       do_report(severity_info,"\n[ChCtrlInstanceInit==>] fail to init Ctrl \n");
              return bErrCode;
	 }

        SRCOperationInfo.bInUsed = false;
	 SRCOperationInfo.bSrcConnected = false;
	 /*SRCOperationInfo.hSource = NULL;*/

        for(CtrlIndex=0;CtrlIndex<CTRL_MAX_NUM_SUBSCRIPTION;CtrlIndex++)
        {
               SRCOperationInfo.EvenSubscribeInfo[CtrlIndex].enabled = false;
               SRCOperationInfo.EvenSubscribeInfo[CtrlIndex].CardNotifiy_fun = NULL;
        }		   

	 return bErrCode;

}




/*the interface of the mg api function  for ch application*/
/*******************************************************************************
 *Function name: 
 *           
 *
 *Description:
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
 *    
 *******************************************************************************/
void CHMG_CheckClosed(word  ExCode,dword  ExCode2)
{
	do_report(severity_info,"\n[CHMG_CheckClosed==>]ExCode[%4x] ExCode2[%4x]\n",ExCode,ExCode2);
}



/*******************************************************************************
 *Function name: CHMG_CheckSysError
 *           
 *
 *Description:
 *           void CHMG_CheckSysError(word  ExCode,dword  ExCode2)
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
 *    
 *******************************************************************************/
void CHMG_CheckHalted(word  ExCode,dword  ExCode2)
{

      /*do_report(severity_info,"\n[CHMG_CheckHalted::]==>\n");

	do_report(severity_info,"\nExCode[%4x] ExCode2[%4x]\n",ExCode,ExCode2);*/

        switch((TMGAPICtrlEvHaltedExCode)ExCode2)
        {
              case MGAPICtrlUser:
#ifdef             CHPROG_DRIVER_DEBUG			  	
                       do_report(severity_info,"\n[CHMG_CheckHalted==>]program stop following action by the user\n");
#endif
                      break;
					  
              case MGAPICtrlCard:
#ifdef             CHPROG_DRIVER_DEBUG			  	
                       do_report(severity_info,"\n[CHMG_CheckHalted==>]program stop following extraction/reset of the smart card\n");
#endif
                      break;
					 
              case MGAPICtrlSrc:
#ifdef             CHPROG_DRIVER_DEBUG			  	
                       do_report(severity_info,"\n[CHMG_CheckHalted==>]program stop after disconnecting the program source\n");
#endif
                      break;

#ifdef    PAIR_CHECK									 
               case MGAPICtrlPairing:
#ifdef              CHPROG_DRIVER_DEBUG			  	
                        do_report(severity_info,"\n[CHMG_CheckHalted==>]program stop after pairing verificaton failed\n");
#endif
                        break;
#endif

               default:
#ifdef             CHPROG_DRIVER_DEBUG			  	
                        do_report(severity_info,"\n[CHMG_CheckHalted==>]program stop for unkown reason\n");
#endif
                        break;	
        } 

	
}


/*******************************************************************************
 *Function name: CHMG_CheckSysError
 *           
 *
 *Description:
 *           void CHMG_CheckSysError(word  ExCode,dword  ExCode2)
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
 *    
 *******************************************************************************/
void CHMG_CheckSysError(word  ExCode,dword  ExCode2)
{
	do_report(severity_info,"\n[CHMG_CheckSysError==>]ExCode[%4x] ExCode2[%4x]\n",ExCode,ExCode2);
}


/*******************************************************************************
 *Function name: CHMG_CheckNoResource
 *           
 *
 *Description:
 *           void CHMG_CheckNoResource(word  Type,dword  hSrc)
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
 *    
 *******************************************************************************/
void CHMG_CheckNoResource(word  Type,dword  hSrc)
{
	switch(Type)   
	{
               case MGAPIDmxChannel:
#ifdef              CHPROG_DRIVER_DEBUG			  	
			   do_report(severity_info,"\n[CHMG_CheckNoResource==>]the DmxChannel is no longer available for the Resource[%4x]\n",hSrc); 	
#endif
			   break;	
			   
               case MGAPIDscrChannel:
#ifndef              CHPROG_DRIVER_DEBUG			  	
			   do_report(severity_info,"\n[CHMG_CheckNoResource]the Dscr Channel is no longer available for the Resource[%4x]\n",hSrc); 
#endif
			   break;	
			   
               case MGAPIMemory:
			   switch(hSrc)
			   {
                               case MGAPIFlash:
#ifndef                              CHPROG_DRIVER_DEBUG			  	
			                   do_report(severity_info,"\n[CHMG_CheckNoResource] The Flash memory source is no longer available\n");
#endif
					    break;

				   case MGAPIRAM:	
#ifndef                              CHPROG_DRIVER_DEBUG			  	
  		                         do_report(severity_info,"\n[CHMG_CheckNoResource] The Ram source is no longer available\n");
#endif
				   	    break;
			   } 
			   break;	

	}
}



/*******************************************************************************
 *Function name: CHMG_UsrNotify
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_UsrNotify(word  ExCode,udword EventData)
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
 *    modify 
 *    2005-2-21     add the new bit2 for indentify the card and box pairing status
 *******************************************************************************/
void CHMG_UsrNotify(word  ExCode,udword EventData)
{
       /*do_report(severity_info,"\n[CHMG_UsrNotify::] \n");*/
	CHCA_UCHAR           SerialNum[6];
	CHCA_BOOL             PairNvmUpdate=false;  
       CHCA_BOOL             CatChanged=false;  /*add this on 050329*/
       SHORT                     tempsCurProgram;
	SHORT                     tempCurTransponderId;
  
	   


#ifdef   PAIR_CHECK
       if(ExCode == MGAPIPairedStatusChange)
       {    
               CHCA_CHAR    ScrambledBit0,ClearBit1,PairStatus2; 
               /*do_report(severity_info,"\n[CHMG_UsrNotify::]A change in pairing status\n");*/
               ScrambledBit0 = EventData & 0x1;
               ClearBit1 = (EventData >>1)& 0x1;
		 PairStatus2 = (EventData >>2)& 0x1; /*add this on 050221*/	   


#if     0/*add this on 050509*/
              if(PairfirstPownOn)
              {
                     do_report(severity_info,"\n[CHMG_UsrNotify==>] New Card Pair status\n");
			PairNvmUpdate = true;		 
                     PairfirstPownOn = false;
              }
#endif						  

               /*do_report(severity_info,"\n[CHMG_UsrNotify==>]PStatus[%d]for scrambler PStatus[%d]for FTA \n",ScrambledBit0,ClearBit1); 
               do_report(severity_info,"\n[CHMG_UsrNotify==>]EPStatus[%d]for scrambler EPStatus[%d]for FTA \n",pastPairInfo.ScrambledBit0,pastPairInfo.ClearBit1);*/ 
#if 1  /*050707 xjp change */
               if((ScrambledBit0 != pastPairInfo.ScrambledBit0)||(ClearBit1 !=  pastPairInfo.ClearBit1)||(PairStatus2!=pastPairInfo.PairStatus2))
               {
                     pastPairInfo.ScrambledBit0 = ScrambledBit0;
			pastPairInfo.ClearBit1 = ClearBit1;
			pastPairInfo.PairStatus2=PairStatus2;
			CatChanged = true;  /*add this on 050329*/
              
			WritePairStautsInfo();
			PairNvmUpdate = true;
#ifdef CH_IPANEL_MGCA
	           if(ScrambledBit0 == 0 && ClearBit1 == 0)
	           	{
	                    CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_OK,NULL,0);
	           	}
#endif			

		 }
#else
		 if((ScrambledBit0 != pastPairInfo.ScrambledBit0)||(ClearBit1 !=  pastPairInfo.ClearBit1))
               {
                     pastPairInfo.ScrambledBit0 = ScrambledBit0;
			pastPairInfo.ClearBit1 = ClearBit1;
			CatChanged = true;  /*add this on 050329*/
			
			
			WritePairStautsInfo();
			PairNvmUpdate = true;
		 }
#endif

#if  1  /*delete this on 050221*/
		 if(PAIR_EMM)
		 {
		        CHCA_UINT               i;

                      if(CHCA_CheckCardReady())
                      {
                      	PairNvmUpdate = true;   /* move it to here on 050509*/

#if   0 /*delete this on 050515*/						

                            CHCA_GetCardContent(SerialNum);
                            
                            if(memcmp(pastPairInfo.CaSerialNumber,SerialNum,6))
                            {
                                   memcpy(pastPairInfo.CaSerialNumber,SerialNum,6);
                                   /*PairNvmUpdate = true;*/
                            }
                            
                            CardPairStatus = CHCAPairOK; 

				WritePairInfo();	
#endif				
                      } 
			 /*else
			 {
                            do_report(severity_info,"\n[CHMG_UsrNotify==>]The card is not ok\n");
			 }*/
			
#ifdef           CHPROG_DRIVER_DEBUG               
                     /*delete this on 050301*/					 
                     do_report(severity_info,"\n[CHMG_UsrNotify==>] Pairing OK\n");
			for(i=0;i<6;i++)	
			{
                           do_report(severity_info,"%4x",pastPairInfo.CaSerialNumber[i]);
			}
			do_report(severity_info,"\n");
			do_report(severity_info,"\nPairNvmUpdate[%d]\n",PairNvmUpdate);
#endif			
		 }
#else /*modify this on 050221*/
		 if(PairStatus2 != pastPairInfo.PairStatus2)
		 {
                     pastPairInfo.PairStatus2 = PairStatus2; /*add this on 050221*/			
			PairNvmUpdate = true;
                     if(CHCA_GetCardContent(SerialNum))
                     {
                           do_report(severity_info,"\n fail to Read card number \n");
                     }
			else
			{
                          memcpy(pastPairInfo.CaSerialNumber,SerialNum,6);
			}

		       if(PairStatus2)
		       {
                           CardPairStatus = CHCAPairOK; 
		             do_report(severity_info,"\n[CHMG_UsrNotify==>] Pairing OK\n");				 
		       }
		       else
		       {
                           CardPairStatus = CHCAPairError;
			      do_report(severity_info,"\n[CHMG_UsrNotify==>] Pairing Error\n");			 
		       }
		 }
#endif
		 
               PairingCheck_FTA = ClearBit1;
               PairingCheck_NOFTA = ScrambledBit0;


              /*if(PairNvmUpdate)
               {
                      WritePairInfo();
		 }*/

#ifndef           CHPROG_DRIVER_DEBUG
               do_report(severity_info,"\n[CHMG_UsrNotify==>]PStatus[%d]for scrambler PStatus[%d]for FTA PairStatus[%d] for cardandbox\n",ScrambledBit0,ClearBit1,PairStatus2); 
#endif

               if(ScrambledBit0)
               {
#ifdef            CHPROG_DRIVER_DEBUG               
                      do_report(severity_info,"\n[CHMG_UsrNotify==>]The Pairing is activated for Scrambled Program\n");
#endif
	 	 }
		 else
		 {
#ifdef            CHPROG_DRIVER_DEBUG               
                      do_report(severity_info,"\n[CHMG_UsrNotify==>]The Pairing is not activated for Scrambled Program\n");
#endif
		 }

               if(ClearBit1)
               {
#ifdef            CHPROG_DRIVER_DEBUG               
                      do_report(severity_info,"\n[CHMG_UsrNotify==>]The Pairing is activated for Clear Program\n");
#endif
	 	 }
		 else
		 {
#ifdef            CHPROG_DRIVER_DEBUG               
                      do_report(severity_info,"\n[CHMG_UsrNotify==>]The Pairing is not activated for Clear Program\n");
#endif
		 }


		 if(PairStatus2)
		 {
#ifdef            CHPROG_DRIVER_DEBUG               
                      do_report(severity_info,"\n[CHMG_UsrNotify==>]Card and Set Top box Pairing status is paired\n");
#endif
		 }
		 else
		 {
#ifdef            CHPROG_DRIVER_DEBUG               
                      do_report(severity_info,"\n[CHMG_UsrNotify==>]Card and Set Top box Pairing status is not paired\n");
#endif
		 }
		 

               /*add this on 041126*/
               if(CurAppWait )
               {
                     do_report(severity_info,"\nCheck the APP CAT\n");
#if 0
                     /*check the pairing status before enter into the application */
                     if(&semWaitCatsignal != NULL)
                     {
                          semaphore_signal(&semWaitCatsignal);
			     do_report(severity_info,"\n send the cat signal to app process\n");		  
		       }/*delete thid on 050327*/
#endif			   
               }	  
               else
               {
                     if(PairNvmUpdate)
                     {
#if                       0                     
#ifndef                 NAGRA_PRODUCE_VERSION
                             CHCA_PairMepgEnable(true,true);
#endif
#endif
                             tempsCurProgram=CH_GetsCurProgramId();
	                      tempCurTransponderId=CH_GetsCurTransponderId();
				 if(CHCA_CheckCardReady())		  
		              { 
		                    if(ChSendMessage2PMT(tempsCurProgram,tempCurTransponderId,START_PMT_NCTP)==true)
		                   {
                                       do_report(severity_info,"\n[CHMG_UsrNotify==>] fail to send the 'START_PMT_NCTP' message\n");
				     }
#ifdef CH_IPANEL_MGCA
                        
                           CHCA_InsertLastPMTData();
#endif
				
				     /*else
				     {
                                       do_report(severity_info,"\n[CHMG_UsrNotify==>] success to send the 'START_PMT_NCTP' message\n");
				     }*/
				}
		      }
		      else
		      {
#ifdef                  CHPROG_DRIVER_DEBUG  		      
                            do_report(severity_info,"\n[CHMG_UsrNotify==>]The Pair status is not changed!!!\n");
#endif
		      }
		}
       }
       
#endif   
       
}

/*******************************************************************************
 *Function name: CHMG_ResetPassword
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_ResetPassword(word Result)
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
 *    
 *******************************************************************************/
void CHMG_ResetPassword(word Result)
{
       CHCA_CHAR            ResetCode[4]={0,0,0,0};

#if 1
       switch(Result)
       {
              case MGAPIPwdNoCard:
			  do_report(severity_info,"\n[CHMG_ResetPassword]There is no smart card in the reader.\n");	
			  break; 	
              case MGAPIPwdHWFailure:
  			  do_report(severity_info,"\n[CHMG_ResetPassword]A hardware failure has occurred\n");	
			  break; 	
              case MGAPIPwdInvalid:
  			  do_report(severity_info,"\n[CHMG_ResetPassword]The password is incorrect.\n");	
			  break; 	
              case MGAPIPwdReset:
  			  do_report(severity_info,"\n[CHMG_ResetPassword]The password has been reset.\n");	
#if 0
                       CH_ResetPinCode(ResetCode);
#endif
			  break; 	
              case MGAPIPwdValid:
  			  do_report(severity_info,"\n[CHMG_ResetPassword]The password is correct.\n");	
			  break; 	
	}
#endif
	   
}

/*******************************************************************************
 *Function name: CHMG_CtrlNotifyMessage
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_CtrlNotifyMessage(word Type,TMGAPIBlk* pBlk)
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
 *    
 *******************************************************************************/
void CHMG_CtrlNotifyMessage(word Type,TMGAPIBlk* pBlk)
{
        CHCA_UINT             i,DataLen=0;
        CHCA_CHAR            ResetCode[4]={'0','0','0','0'};	 /*modiy this on 050117*/	
	 CHCA_UCHAR          CardSerialNum[6];	
	 CHCA_UINT             OpiNum;
	 CHCA_UINT             Index;
		
        /*do_report(severity_info,"\n[CHMG_CtrlNotifyMessage::]==>\n");*/

        if(pBlk!=NULL)
        {
               switch(Type)
               {
                       case MGAPIEMMC:
#ifdef                      CHPROG_DRIVER_DEBUG					   	
				    do_report(severity_info,"\nThe Data Len[%d],the message content::",pBlk->Size); 
#endif
                                if((pBlk->Size>0) && (pBlk->pData!=NULL))	
				    {
#ifdef                           CHPROG_DRIVER_DEBUG					   	
                                       for(i=0;i<pBlk->Size;i++)
						  do_report(severity_info,"%4x",*(pBlk->pData+i)); 			   	
					    do_report(severity_info,"\n"); 			   	

                                       do_report(severity_info,"\n EMMC Type[%2x]\n",*(pBlk->pData+13));
#endif

                                       switch(*(pBlk->pData+13))
                                       {
                                              case 0x2:
				                  case 0xb:
#ifdef                                             CHPROG_DRIVER_DEBUG					   	
							      do_report(severity_info,"\n[CHMG_CtrlNotifyMessage::] Mail Message Index[%d]\n",*(pBlk->pData+14));	  	
#endif

#ifdef   MAIL_APP /*add this on 041127*/
#ifndef  CH_IPANEL_MGCA    	                  

                                                       CHCA_GetCardContent(CardSerialNum);
                                                       if(CH_NeedResetMailIndex(CardSerialNum))
								   CH_ResetMainIndex();
													   
                                                       DataLen = pBlk->Size - 10; 
							    #if 1 /*modify this on 050117*/
								 OpiNum = (*(pBlk->pData+9)<<8) | (*(pBlk->pData+10));
								 /*do_report(severity_info,"\nOpiNum[%d]\n",OpiNum);*/
								 CH_HandleMailData((pBlk->pData+13),DataLen,OpiNum);
							    #else	
                                                       CH_HandleMailData((pBlk->pData+13),DataLen);           
                                                    #endif			
#else
                                                 CHCA_CardSendMess2Usif(CH_CA2IPANEL_NEW_MAIL,pBlk->pData,pBlk->Size);

#endif
#endif
							      break;	  	 
								  
				                  case 0x7:
							      switch(*(pBlk->pData+15))	  
							      {
                                                              case 0x8:
#ifdef                                                            CHPROG_DRIVER_DEBUG					   	
                     							do_report(severity_info,"\n[CHMG_CtrlNotifyMessage::] Anti Pin Message Index[%d]\n",*(pBlk->pData+14));	  	
#endif
#ifdef   PINRESET_APP

#ifndef  CH_IPANEL_MGCA    	                  
                                                                      #if  1/*modify this on 050120*/
										Index = *(pBlk->pData+14);					    
										CH_ResetPinCode(ResetCode,Index);						   
										#else						   
                                                                      CH_ResetPinCode(ResetCode);
										#endif
#else										
	                                                            CHCA_CardSendMess2Usif(CH_CA2IPANEL_RESET_PIN,pBlk->pData,pBlk->Size);
                                                                                                                 									
#endif										
#endif
									       break;

								      default:
#ifdef                                                          CHPROG_DRIVER_DEBUG					   	
									  	do_report(severity_info,"\n[CHMG_CtrlNotifyMessage::] Pairing Message Index[%d]\n",*(pBlk->pData+14));	  	
#endif
                                                                      /*PAIR_EMM=true;*/                                                                      
									       break;
							      }
							      break;	

						    default:
								break;
					    }
						
				    }
				    break;

			  default:
			  	    break;
		 }
	 }
        else
        {
              /*do_report(severity_info,"\n[CHMG_CtrlNotifyMessage::]the message poniter is null \n")*/;
	 }

		
}

/*******************************************************************************
 *Function name: CHMG_CtrlDisplayText
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_CtrlDisplayText(chcar*  pText)
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
 *    
 *******************************************************************************/
void CHMG_CtrlDisplayText(char*  pText)
{
        /* do_report(severity_info,"\n[CHMG_CtrlDisplayText::]==>\n");*/

         if(pText!=NULL)
         {
		  while(pText!=NULL)	
 		  {
                      do_report(severity_info,"%4x",*(pText++));
		  }
  	  }
	  else
	  {
               do_report(severity_info,"\n the text pointer is null \n");
	  }
}

/*******************************************************************************
 *Function name: CHMG_CtrlCheckRightAccepted
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_CtrlCheckRightAccepted(word AckCode,TMGAPIPIDList*  lpPID)

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
 *    
 *******************************************************************************/
void CHMG_CtrlCheckRightAccepted(word AckCode,TMGAPIPIDList*  lpPID)
{
        CHCA_UINT               PidIndex,i;   
	 CHCA_BOOL             VideoOpen=false;
	 CHCA_BOOL             AudioOpen=false;	
	 CHCA_BOOL             NoProcess = false;
	 STBStaus                  CurSTBStaus;
      
        if(lpPID != NULL)
        {
              /* do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted=>]PIDNUM[%d],PidList::",lpPID->n);*/

               if(lpPID->n >= 2)
		    lpPID->n = 2;	    	

#if    0	
               do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted=>]ElementPid[%d][%d]",stTrackTableInfo.ProgElementPid[0],stTrackTableInfo.ProgElementPid[1]);

		 for(PidIndex=0;PidIndex<lpPID->n;PidIndex++)
               {
                      do_report(severity_info,"%4x",lpPID->pPID[PidIndex]);
		 }
               do_report(severity_info,"\n");
#endif

		 /*do_report(severity_info,"\nElemPID[%d][%d]\n",stTrackTableInfo.ProgElementPid[0],stTrackTableInfo.ProgElementPid[1]);*/	   

               switch(stTrackTableInfo.ProgServiceType) 
               {
                     case TelevisionService:
				  if(lpPID->n == 2)	 
				  {
                                   VideoOpen = true;
					AudioOpen = true;	
					stTrackTableInfo.ProgElementCurRight[0] = 1; 
					stTrackTableInfo.ProgElementCurRight[1] = 1; 
					CurSTBStaus = NORMAL_STATUS; 			   
				  }

				  if(lpPID->n == 1)
				  {
                                   if((stTrackTableInfo.ProgElementType[0]==SCRAMBLED_PROGRAM) && (stTrackTableInfo.ProgElementType[1]==SCRAMBLED_PROGRAM))
                                   {
                                          CHCA_BOOL     GetFirstRight = false;
								  
#if 1/*add this on 050313*/
                                          MpegEcmProcess = true;
#else
                                          if((stTrackTableInfo.ProgElementCurRight[0]==-1)&&(stTrackTableInfo.ProgElementCurRight[1]==-1))
                                          {
                                                 GetFirstRight = true;
						}
#endif										  
  
                                          /*special process*/
					       NoProcess = true;	
#if 0										  
						do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted==>] Video and Audio with different cws\n");	
#endif
                                          for(i=0;i<2;i++)
                                          {
                                                 /*do_report(severity_info,"\n[%d][%d]\n",lpPID->pPID[0],stTrackTableInfo.ProgElementPid[i]);*/
                                                 if(lpPID->pPID[0]==stTrackTableInfo.ProgElementPid[i])
                                                 {
                                                       stTrackTableInfo.ProgElementCurRight[i] = 1;
							      break;						   
							}
						}
										  
                                          if(i<2)
                                          {
                                                /*add this on 050313*/
                                                if(EcmFilterNum==2)
                                                {
#ifdef                                            CHPROG_DRIVER_DEBUG           
  							     do_report(severity_info,"[CHMG_CtrlCheckRightAccepted==>]Track[%d] has the right",i); 
#endif
                                                }	
						       /*add this on 050313*/						

#if 0/*delete this on 050313*/
 
                                                if(GetFirstRight)
                                                {
                                                      GetFirstRight = false;
                                                      if(psemAVSeparate!=NULL)
                                                      {
                                                            do_report(severity_info,"[CHMG_CtrlCheckRightAccepted==>]First Get the Right Info"); 
                                                            semaphore_signal(psemAVSeparate);
            						     }
                                               }
#endif												
                                          }
						else
						{
                                                do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted==>]The PID is wrong\n");
 						}
					}
					else
					{
                                          VideoOpen = true;
                                          AudioOpen = true;	
                                          stTrackTableInfo.ProgElementCurRight[0] = 1; 
                                          stTrackTableInfo.ProgElementCurRight[1] = 1; 
                                          CurSTBStaus = NORMAL_STATUS; 			   
					}
				  }
				  break;

			case RadioService:
				 AudioOpen = true;
				   if(CH_GetCurApplMode() != APP_MODE_MOSAIC)/* 20071106  wz add 避免在马赛克广播页面打开视频*/
				   	{
				 if(stTrackTableInfo.iVideo_Track!=CHCA_INVALID_TRACK)
				 	{
				 	VideoOpen = true;
				 	}/*20061122 add*/
				   	}
				 stTrackTableInfo.ProgElementCurRight[1] = 1; 
				 CurSTBStaus = NORMAL_STATUS; 
				 break;

			case MosaicVideoService:
				  VideoOpen = true;
				  stTrackTableInfo.ProgElementCurRight[0] = 1; 
				  CurSTBStaus = NORMAL_STATUS; 
				  break;

		 }
 	 }

	 switch(AckCode)
	 {
               case MGAPICMAck:
#ifdef CHPROG_DRIVER_DEBUG			   	
			   do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted::] Acceptance  \n");	
#endif
#ifdef  CH_IPANEL_MGCA
                        CHCA_CardSendMess2Usif(CH_CA2IPANEL_Have_Right,NULL,0);
#endif

			   break; 	
			   
               case MGAPICMOverdraft:
#ifdef  CH_IPANEL_MGCA
                        CHCA_CardSendMess2Usif(CH_CA2IPANEL_Have_Right,NULL,0);
#endif
			   	
			   do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted::]Acceptance but recourse to overdraft\n");	
			   break; 	
			   
               case MGAPICMPreview:
#ifdef  CH_IPANEL_MGCA
                        CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);
#endif
   	
			   do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted::]Temporary acceptance(preview)\n");	
			   break; 
			   
               case MGAPICMPPV:
#ifdef  CH_IPANEL_MGCA
                        CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);
#endif
			   	
			   do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted::]The messaged accepted was related to PPV\n");	
			   break; 

		 default:
#ifdef  CH_IPANEL_MGCA
                        CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);
#endif
		 	
			   do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted::]unknow error\n");	
		 	   break;
	 }
#ifndef CH_IPANEL_MGCA

        if(!NoProcess)
        {
              /*send the right message to the usif*/
              CHCA_CardSendMess2Usif(CurSTBStaus); 
#if   0			  
              do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted==>] CHCA_MepgEnable\n");
#endif
              CHCA_MepgEnable(VideoOpen,AudioOpen);/*add this on 041125*/
        }
#endif		
	 /*else
	 {
		do_report(severity_info,"\n[CHMG_CtrlCheckRightAccepted==>] Video and Audio with different cws,no open process\n");				  
	 }*/
		
	 return; 

}


/*******************************************************************************
 *Function name: CHMG_CtrlCheckCmptStatus
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_CtrlCheckCmptStatus(word CmptStatus,TMGAPICmptStatusList*  pList)
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
 *    
 *******************************************************************************/
void CHMG_CtrlCheckCmptStatus(word CmptStatus,TMGAPICmptStatusList*  pList)
{
       CHCA_USHORT            index,i;
       STBStaus                    CurSTBStaus = NO_RIGHT;
       TMGAPIOffer                *pInfo;          
	   

       if(pList != NULL)
       {
               /*do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus::]==>");
		 do_report(severity_info,"PIDValue[%d] TypeNum[%d]\n",pList->PID,pList->nStatus);	  */ 
		 
		 for(index=0;index<pList->nStatus;index++)
		 {
		        switch(pList->lpStatus[index].Type)
		        {
                             case MGAPICATypeOffer:
#ifdef    CHPROG_DRIVER_DEBUG							 	
					   do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus==>]Subscription Type\n");
#endif
                                      pInfo = (TMGAPIOffer*)pList->lpStatus[index].pInfo;

					   /*do_report(severity_info,"\nOPI[%4x],Expirated date[%d] Offerlist:",pInfo->OPI,pInfo->Date); 
					   for(i=0;i<8;i++) 
					       do_report(severity_info,"%4x",pInfo->Offer[i]); */
					   break;
					   
                             case MGAPICATypePPV:
#ifdef    CHPROG_DRIVER_DEBUG							 	
					   do_report(severity_info,"\nPPV Type\n"); 	
#endif
					   break;
					   
                             case MGAPICATypeUnknown:
#ifdef    CHPROG_DRIVER_DEBUG							 	
					   do_report(severity_info,"\nUnknow Type\n"); 	
#endif
					   break;
			 }
                      	   
		 }
	}
	   
 
	switch(CmptStatus)
	{
              case MGAPICmptOk:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Decrypted element,or entitlements acknowledged\n");
                      break;
					  
              case MGAPICmptOverdraft:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Decrypted element,but wiht recourse to overdraft\n");
                      break;
					  
              case MGAPICmptPreview:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Preview in progress\n");
                      break;
					  
              case MGAPICmptNANoCard:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]No Mediaguard smart card present\n");
                      break;
					  
              case MGAPICmptNAOperator:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Operator unknown\n");
                      break;
					  
              case MGAPICmptNAOffer:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Offer unknown or does not match\n");
                      break;
					  
              case MGAPICmptNAExpired:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Validity date has expired\n");
                      break;
					  
              case MGAPICmptNABlackout:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Geographical blackout\n");
                      break;
					  
              case MGAPICmptNARating:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Rating level too high\n");
                      break;
					  
              case MGAPICmptNAMaxShot:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Maximum number of shots has been reached.\n");
                      break;
					  
              case MGAPICmptNANoCredit:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Wallet is empty(IPPV,IPPT)\n");
                      break;
					  
              case MGAPICmptNAPPPV:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]PPV offer in reservation purchase mode\n");
                      break;
					  
              case MGAPICmptNAIPPV:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]PPV offer in token purchase mode,jor already inscribed\n");
                      break;
					  
              case MGAPICmptNAIPPT:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]PPT purchase offer\n");
                      break;
					  
              case MGAPICmptNAUnknown:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Unknow rights\n");
                      break;
					  
              case MGAPICmptNAOthers:
                      do_report(severity_info,"\n[CHMG_CtrlCheckCmptStatus:]Other reasons(MG descriptor missing,lack of hardware resources,other...)\n");
                      break;
					  
  	}

       return;
}


/*******************************************************************************
 *Function name: CHMG_CtrlCheckRejectedStatus
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_CtrlCheckRejectedStatus(word iRejectCode,TMGAPIPIDList*  PIDData)
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
 *    RightType description   ::   -1      ---initi status
                                              0        --- no right 
 *                                           1        --- with the right
 *                                           2        ----geo blackout
 *                                           3        ----paratel control   add this item on 050312
 *                                           4        ----??
 *   2005-03-12     delete the a track with a cw status process 
 *******************************************************************************/
void CHMG_CtrlCheckRejectedStatus(word iRejectCode,TMGAPIPIDList*  PIDData)
{
        CHCA_USHORT                       PIDNumber=0;
        CHCA_USHORT                       PidIndex,i;		
        STBStaus                                CurSTBStaus;
	 CHCA_BOOL                            VideoClose = false;
	 CHCA_BOOL                            AudioClose = false;
	 CHCA_BOOL                            NoProcess = false;
        CHCA_INT                               RightType=0;
		
        CurSTBStaus = NO_RIGHT;                          
		
        if(PIDData==NULL)
        {
                do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>] The input parameter is null\n");
                return;
  	 }

        PIDNumber = PIDData->n;

#ifdef    CHPROG_DRIVER_DEBUG							 	
        do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus=>]ElementPid[%d][%d]",stTrackTableInfo.ProgElementPid[0],stTrackTableInfo.ProgElementPid[1]);
        do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus=>]PIDNUM[%d],PidList::",PIDData->n);
#endif		
        
        if(PIDData->n >= 2)
           PIDData->n = 2;	    	

#ifdef    CHPROG_DRIVER_DEBUG							 	
        for(PidIndex=0;PidIndex<PIDData->n;PidIndex++)
        {
             do_report(severity_info,"%4x",PIDData->pPID[PidIndex]);
        }
        do_report(severity_info,"\n");
#endif

        /*do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]InputPidNUm[%d] CurPidNum[%d]\n",PIDNumber,stTrackTableInfo.iProgNumofMaxTrack);*/

        if(PIDNumber==0 || PIDNumber>2)
        {
               do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>] PIDNumber is wrong\n");
               return;
   	 }

        if((stTrackTableInfo.iProgNumofMaxTrack==0) || (stTrackTableInfo.iProgNumofMaxTrack<PIDNumber))
        {
               do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>] ProgInfo is not same as PIDNumber\n");
               return; 
	 }

        switch(iRejectCode)
        {
               case MGAPICMCardFailure:
			   	
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Smart card error\n"); 	 
                       break;
               case MGAPICMNANoRights:
#ifdef  	         CHPROG_DRIVER_DEBUG		   	
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]No rights have been found\n"); 	
#endif
                       break;
               case MGAPICMNAExpired:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Entitlements have expired\n"); 	  	
                       break;
               case MGAPICMNABlackout:
			  CurSTBStaus = Geo_Blackout;	
			  RightType = 2;
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Geographical blackout\n"); 	  	
                       break;
               case MGAPICMNARating:
			  CurSTBStaus = Parental_Control;	/*add it on 050312*/
			  RightType = 3;/*add it on 050312*/
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Rating level too high\n"); 	  	
                       break;
               case MGAPICMNAPPPV:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Product purchaseble by pre-payment\n"); 	  	
                       break;
               case MGAPICMNAIPPV:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Product purchcaeble in the token purchcase mode\n"); 	  	
                       break;
               case MGAPICMNAIPPT:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Product purchasable on a time basis\n"); 	  	
                       break;
              case MGAPICMNAMaxShot:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Maximum number of shots reached\n"); 	  	
                       break;
              case MGAPICMNANoCredit:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Wallet empty\n"); 	  	
                       break;
              case MGAPICMNAOthers:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Other reasons\n"); 	  	
                       break;
		default:
			  do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Unknown reasons\n"); 	  	
			  break;
					   
   	 }
	#ifdef  CH_IPANEL_MGCA

	#ifndef  NAGRA_PRODUCE_VERSION
       do_report(0,"PIDNumber = %d\n",PIDNumber);
	 if(PIDNumber == 2)	 
	 	{
	 	    MpegEcmProcess = true;
                   NoProcess = false;
	 	}
	else if(PIDNumber == 1)
		{
                  if((stTrackTableInfo.ProgElementType[0]==SCRAMBLED_PROGRAM) && (stTrackTableInfo.ProgElementType[1]==SCRAMBLED_PROGRAM))
                     	{
                     	        MpegEcmProcess = true;
                                NoProcess = true;	
		                         for(i=0;i<2;i++)
                                          {
                                                if(PIDData->pPID[0]==stTrackTableInfo.ProgElementPid[i])
                                                {
                                                       stTrackTableInfo.ProgElementCurRight[i] = RightType;
							      break;						   
                                                }
                                          }
										  
                                          if(i<2)
                                          {
                                                /*add this on 050313*/
                                                if(EcmFilterNum==2)
                                                {
                                                     do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]AV Differ CW,Track[%d] has no right\n",i); 
                                                }	
						      /*add this on 050313*/				  

                                          }
						else
						{
                                                do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Pid[%d] wrong\n",PIDData->pPID[0]);
						}
                     	}
		else
			{
                                NoProcess = false;
			}
		}
	if(!NoProcess)
		{
        	    if(CurSTBStaus == NO_RIGHT )
		    	{
		  	       CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);   

		    	}
		else if(CurSTBStaus == Geo_Blackout)
			{
	                   CHCA_CardSendMess2Usif(CH_CA2IPANEL_BLACKOUT,NULL,0);   
			}
		else
			{
	  	                 CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);   

			}
		}
	
	#else
	    if(CurSTBStaus == NO_RIGHT)
	    	{
      	       CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);   

	    	}
		else if(CurSTBStaus == Geo_Blackout)
			{
	                   CHCA_CardSendMess2Usif(CH_CA2IPANEL_BLACKOUT,NULL,0);   
			}
		else
			{
      	                 CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);   

			}
		#endif
      #else
	  
        switch(stTrackTableInfo.ProgServiceType) 
        {
                 case TelevisionService:
                         if(PIDNumber == 2)	 
                         {
                                VideoClose = true;
                                AudioClose = true;	
                                stTrackTableInfo.ProgElementCurRight[0] = RightType; 
                                stTrackTableInfo.ProgElementCurRight[1] = RightType; 
                                /*CurSTBStaus = NO_RIGHT; 	*/		   
                         }
                         
                         if(PIDNumber == 1)
                         {
                                  if((stTrackTableInfo.ProgElementType[0]==SCRAMBLED_PROGRAM) && (stTrackTableInfo.ProgElementType[1]==SCRAMBLED_PROGRAM))
                                  {
                                          CHCA_BOOL     GetFirstRight = false;
								  
#if  1 /*modify this on 050312*/
                                          MpegEcmProcess = true;
#else
								  
                                          if((stTrackTableInfo.ProgElementCurRight[0]==-1)&&(stTrackTableInfo.ProgElementCurRight[1]==-1))
                                          {
                                                 GetFirstRight = true;
						}
#if 1/*add this on 050221*/										  
						else
						{
                                                 MpegEcmProcess = true;
						}
#endif						
#endif
										  
                                          /*special process*/
                                          NoProcess = true;	
                                          /*do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>] Video and Audio with different cws\n");*/				  
                                          for(i=0;i<2;i++)
                                          {
                                                if(PIDData->pPID[0]==stTrackTableInfo.ProgElementPid[i])
                                                {
                                                       stTrackTableInfo.ProgElementCurRight[i] = RightType;
							      break;						   
                                                }
                                          }
										  
                                          if(i<2)
                                          {
                                                /*add this on 050313*/
                                                if(EcmFilterNum==2)
                                                {
                                                     do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]AV Differ CW,Track[%d] has no right\n",i); 
                                                }	
						      /*add this on 050313*/				  

#if 0/*delete this on 050313*/
                                                if(GetFirstRight)
                                                {
                                                      GetFirstRight = false;
                                                      if(psemAVSeparate!=NULL)
                                                      {
                                                            do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]First Get No Right Info\n"); 
                                                            semaphore_signal(psemAVSeparate);
            						     }
                                                } 								
#endif												
                                          }
						else
						{
                                                do_report(severity_info,"\n[CHMG_CtrlCheckRejectedStatus==>]Pid[%d] wrong\n",PIDData->pPID[0]);
						}
                                   }
                                  else
                                  {
                                          if(stTrackTableInfo.ProgElementType[0]==SCRAMBLED_PROGRAM)
                                          {
                                                 VideoClose = true;
                                                 stTrackTableInfo.ProgElementCurRight[0] = RightType;
							if(CurSTBStaus == NO_RIGHT)					 
							   CurSTBStaus = NO_VEDIO_RIGHT;
                                          }	
										  
                                          if(stTrackTableInfo.ProgElementType[1]==SCRAMBLED_PROGRAM)
                                          {
                                                 AudioClose = true;
							stTrackTableInfo.ProgElementCurRight[1] = RightType; 
							if(CurSTBStaus == NO_RIGHT)	
							   CurSTBStaus = NO_AUDIO_RIGHT;
                                          }                                  
				       }
                         }
                         break;
        
                 case RadioService:
                         AudioClose = true;
                         stTrackTableInfo.ProgElementCurRight[1] = RightType; 
			    if(PIDData->pPID[0] != stTrackTableInfo.ProgElementPid[1])
			    {
                               NoProcess=true;
			    }
                         /*CurSTBStaus = NO_RIGHT; */
                         break;
                 
                 case MosaicVideoService:
                         VideoClose = true;
                         stTrackTableInfo.ProgElementCurRight[0] = RightType; 
			    if(PIDData->pPID[0] != stTrackTableInfo.ProgElementPid[0])
			    {
                              NoProcess=true;
			    }
                         /*CurSTBStaus = NO_RIGHT; */
                         break;
        
        }
		

        if(!NoProcess)
        {
              /*send the right message to the usif*/
      	       CHCA_CardSendMess2Usif(CurSTBStaus);   
              CHCA_MepgDisable(VideoClose,AudioClose); 
        } 
		#endif
	 return;	

}




/*******************************************************************************
 *Function name:  CHMG_GetApiStatusReport
 * 
 *
 *Description:  print some status info returned by the kernel api function 
 *                  
 *
 *Prototype:
 *          void CHMG_GetApiStatusReport (TMGAPIStatus  iChApiStatus)
 *
 *
 *
 *input:
 *      
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
void CHMG_GetApiStatusReport (TMGAPIStatus  iChApiStatus)
{
	switch(iChApiStatus)	
	{
            case MGAPIOk:
			/*do_report(severity_info,"\n Status returned by kernel -->MGAPIOk \n");	*/
			break;	
            case MGAPISysError:
			do_report(severity_info,"Status returned by kernel -->MGAPISysError \n");	
			break;	
            case MGAPIHWError:
			do_report(severity_info,"Status returned by kernel --> MGAPIHWError\n");	
			break;	
            case MGAPIInvalidParam:
			do_report(severity_info,"Status returned by kernel -->MGAPIInvalidParam \n");	
			break;	
            case MGAPINotFound:
			do_report(severity_info,"Status returned by kernel -->MGAPINotFound\n");	
			break;	
            case MGAPIAlreadyExist:
			do_report(severity_info,"Status returned by kernel -->MGAPIAlreadyExist \n");	
			break;	
            case MGAPINotInitialized:
			do_report(severity_info,"Status returned by kernel --> MGAPINotInitialized\n");	
			break;	
            case MGAPIIdling:
			do_report(severity_info,"Status returned by kernel -->MGAPIIdling\n");	
			break;	
            case MGAPIEmpty:
			do_report(severity_info,"Status returned by kernel -->MGAPIEmpty \n");	
			break;	
            case MGAPIRunning:
			do_report(severity_info,"Status returned by kernel --> MGAPIRunning\n");	
			break;	
            case MGAPINotRunning:
			do_report(severity_info,"Status returned by kernel --> MGAPINotRunning\n");	
			break;	
            case MGAPINoResource:
			do_report(severity_info,"Status returned by kernel --> MGAPINoResource\n");	
			break;	
            case MGAPINotReady:
			do_report(severity_info,"Status returned by kernel --> MGAPINotReady\n");	
			break;	
            case MGAPILockError:
			do_report(severity_info,"Status returned by kernel --> MGAPILockError\n");	
			break;	
            case MGAPINotEntitled:
			do_report(severity_info,"Status returned by kernel --> MGAPINotEntitled\n");	
			break;	
            case MGAPINotAvailable:
			do_report(severity_info,"Status returned by kernel --> MGAPINotAvailable\n");	
			break;	
            default:
			do_report(severity_info,"Status returned by kernel --> Not Known error \n");	
			break;	
            
	}

	return;
}






/*******************************************************************************
 *Function name:  CHMG_CtrlOperationStop
 * 
 *
 *Description:  quit the ctrl process,halt, free,close the process about the ctrl part 
 *                  
 *
 *Prototype:
 *          BOOL  CHMG_CtrlOperationStop(void)
 *
 *
 *input:
 *      
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
BOOL  CHMG_CtrlOperationStop(void)
{
       BOOL      bErrCode = false;

 	if(pChMgApiCtrlHandle != NULL)
	{ 
	        TMGAPIStatus       iAPIStatus; 
			
               semaphore_wait(pSemMgApiAccess);
	        iAPIStatus = MGAPICtrlHalt(pChMgApiCtrlHandle);

#ifndef   CHPROG_DRIVER_DEBUG	
               if((iAPIStatus != MGAPIOk) && (iAPIStatus !=MGAPIIdling))
               {
	               do_report(severity_info,"\n[MGAPICtrlHalt==>]");
		        CHMG_GetApiStatusReport(iAPIStatus);	
               }  
#endif		 

		/* if(iAPIStatus != MGAPIOk)
		 {
                      bErrCode = true;
		        semaphore_signal(pSemMgApiAccess);			  
			 return bErrCode;		  
		 }*/

               iAPIStatus = MGAPICtrlFree(pChMgApiCtrlHandle);
		 semaphore_signal(pSemMgApiAccess);
		 
               /*if(iAPIStatus != MGAPIOk)
		 {
                      bErrCode = true;
		 }*/
		 

#ifndef     CHPROG_DRIVER_DEBUG	
               if((iAPIStatus != MGAPIOk) && (iAPIStatus !=MGAPIIdling))
               {
      		       do_report(severity_info,"\n[MGAPICtrlFree==>]");	
      		       CHMG_GetApiStatusReport(iAPIStatus);	
               }
#endif

			    
#if     0/*add this on 050124*/
              if(!CardOperation.bCardReady)
              {
#ifndef        PAIR_CHECK	
                   iAPIStatus =MGAPICtrlClose(pChMgApiCtrlHandle);
		     do_report(severity_info,"\n[MGAPICtrlClose]::");	
#else
                   iAPIStatus =MGAPIPairedCtrlClose(pChMgApiCtrlHandle);
		     do_report(severity_info,"\n[MGAPIPairedCtrlClose]::");	
#endif
		     CHMG_GetApiStatusReport(iAPIStatus);	

                   pChMgApiCtrlHandle = NULL; /* add this on 040824*/
              }
#endif			  
	}
	else
	{
             bErrCode = true;
	}
	
	return bErrCode;
       
}


/*******************************************************************************
 *Function name: CHMG_CtrlInstanceInit
 *           
 *
 *Description: 
 *            
 *             
 *    
 *Prototype:
 *     BOOL  CHMG_CtrlInstanceInit(void)
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
CHCA_BOOL  CHMG_CtrlInstanceInit(void)
 {
       CHCA_BOOL                             bErrCode=false;
 	TMGAPIStatus                           iApiStatus;	
	BitField32                                 bCtrlEvMask;
	TMGSysSrcHandle                     hSource;

       hSource = (TMGSysSrcHandle)MGTUNERDeviceName;
       if(pChMgApiCtrlHandle!=NULL)
          return false;     
	   
       semaphore_wait(pSemMgApiAccess); 
#ifndef  PAIR_CHECK
       pChMgApiCtrlHandle = MGAPICtrlOpen(CHMG_CtrlCallback,hSource);
#else
       pChMgApiCtrlHandle = MGAPIPairedCtrlOpen(CHMG_CtrlCallback,hSource);
#endif

       if(pChMgApiCtrlHandle!=NULL)
	{
	        /*set the Ctrl event mask*/
               bCtrlEvMask = MGAPICtrlEvHaltedMask|\
                                     MGAPICtrlEvCloseMask|\
                                     MGAPICtrlEvBadCardMask|\
                                     MGAPICtrlEvReadyMask|\
                                     MGAPICtrlEvCmptStatusMask|\
                                     MGAPICtrlEvRejectedMask|\
                                     MGAPICtrlEvAcknowledgedMask;

#ifndef  PAIR_CHECK		
               iApiStatus = MGAPICtrlSetEventMask(pChMgApiCtrlHandle,bCtrlEvMask);
#else
	        iApiStatus = MGAPIPairedCtrlSetEventMask(pChMgApiCtrlHandle,bCtrlEvMask);
#endif

               if(iApiStatus != MGAPIOk)
               {
#ifdef          CHDEMUX_DRIVER_DEBUG		 
                    do_report(severity_info,"\n [MGAPISetEventMask::]");
                    CHMG_GetApiStatusReport(iApiStatus);
#endif    
                   bErrCode = true;
	        }
        }
	 else
	 {
	        bErrCode = true;
#ifndef   PAIR_CHECK
#ifdef     CHDEMUX_DRIVER_DEBUG		 
               do_report(severity_info,"\n[MGAPICtrlOpen::]  Can not open the ctrl instance\n");
#endif

#else
#ifdef     CHDEMUX_DRIVER_DEBUG		 
               do_report(severity_info,"\n[MGAPIPairedCtrlOpen::]  Can not open the ctrl instance\n");
#endif
#endif
	 }
	 
        semaphore_signal(pSemMgApiAccess);

        return bErrCode;
 }



/*******************************************************************************
 *Function name:  ChProgCaCtrlStart
 * 
 *
 *Description: start the ctrl process about the mg ca 
 *                  
 *
 *Prototype:
 *          BOOL CHMG_CtrlCAStart(TMGAPICtrlHandle    pCtrlApiHandle)
 *
 *
 *input:
 *      
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
BOOL CHMG_CtrlCAStart(TMGAPICtrlHandle    pCtrlApiHandle)
{
        BOOL                                 bErrCode=false;
        TMGAPIStatus                     iApiStatus,iiApiStatus;
        TMGAPICtrlStatus                iCtrlStatus;

	 U16                                   *iElementPid; 
	 boolean                             bCheck;  
        boolean                             ElementIndex;
	 CHCA_UCHAR                    SerialNum[8];	

	
        if(pCtrlApiHandle==NULL)
	 {
	        bErrCode=true; 
	        do_report(severity_info,"\n [CHMG_CtrlCAStart==>]The CtrlAPI handle  is NULL \n");
	        return bErrCode;		
        }	  


#ifdef   NAGRA_PRODUCE_VERSION /*add this on 050509*/
        /*ChDescramblerInit(); */
        semaphore_wait(pSemMgApiAccess);		  
	 iApiStatus = MGAPICtrlGetStatus(pCtrlApiHandle,&iCtrlStatus);
	 semaphore_signal(pSemMgApiAccess);
	 
        if((iApiStatus != MGAPIOk) || (iCtrlStatus != MGAPICtrlReady))
        {
       	 do_report(severity_info,"\n[MGAPICtrlGetStatus==>] ");
               CHMG_GetApiStatusReport(iApiStatus);	

               bErrCode=true;
	        do_report(severity_info,"\n [CHMG_CtrlCAStart==>]the mg api interface err or  Ctrl instance is not Ready \n");

	        return bErrCode;		
        }	
#endif
		

#ifndef  PAIR_CHECK		
       semaphore_wait(pSemMgApiAccess);	
 	iiApiStatus = MGAPICtrlStart(pCtrlApiHandle);
       semaphore_signal(pSemMgApiAccess);	


	if(iiApiStatus != MGAPIOk)
	{
            do_report(severity_info,"\n[MGAPIPairedCtrlStart==>]");
            CHMG_GetApiStatusReport(iiApiStatus);	
	}
#else
        PairingCheck=PairingCheck_NOFTA;

       semaphore_wait(pSemMgApiAccess);	
       iiApiStatus = MGAPIPairedCtrlStart(pCtrlApiHandle);
       semaphore_signal(pSemMgApiAccess);
	   
	if(iiApiStatus != MGAPIOk)
	{	
            do_report(severity_info,"\n[MGAPIPairedCtrlStart]==>");	
            CHMG_GetApiStatusReport(iiApiStatus);	
	}

	switch(iiApiStatus)
	{
               case MGAPIOk:
#if 1  /*add this on 050421*/
                        EcmDataStop=false;
#endif			   	
 			   if(PairingCheck_NOFTA)
 			   {
 			           /*add this on 050315*/
#if  0					   
#ifndef                     NAGRA_PRODUCE_VERSION 
				    if(PairfirstPownOn && (CardPairStatus==CHCA_PairError))	
				     {
                                       PairfirstPownOn = false;
					    do_report(severity_info,"\n[CHMG_CtrlCAStart==>] The New card is not paired\n");
                                       CHCA_PairMepgDisable(true,true);
					    CHCA_CardSendMess2Usif(NO_STB_CARD);	
				     }
				     else
#endif
#endif
				     {

					     CHCA_GetCardContent(SerialNum);
                                       
                                        if(memcmp(pastPairInfo.CaSerialNumber,SerialNum,6))
                                        {
                                              memcpy(pastPairInfo.CaSerialNumber,SerialNum,6);
                                              WritePairInfo();					   
                                        }
				    
                                       /*add this on 050315*/	
                                       
                                       CardPairStatus = CHCAPairOK;
	#ifdef  CH_IPANEL_MGCA
	                     CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_OK,NULL,0);
	#endif							   
                                       do_report(severity_info,"\n[CHMG_CtrlCAStart==>]scramb Pair success \n"); 
#ifndef                            NAGRA_PRODUCE_VERSION  			  
                                       CHCA_PairMepgEnable(true,true);
#endif
		                 }
 			   }
			   else
			   {
#ifndef                   NAGRA_PRODUCE_VERSION  	
                               do_report(severity_info,"\n[CHMG_CtrlCAStart==>]scramb No Pairing check \n"); 
                               CHCA_PairMepgEnable(true,true);
				   /*CHCA_CardSendMess2Usif(NORMAL_STATUS);	*/		   
#endif
	#ifdef  CH_IPANEL_MGCA/*20090728 add*/
	                     CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_OK,NULL,0);
	#endif							   

			   }
			   break;
			  
		case MGAPINotEntitled:
			  bErrCode = true;
		 	  CardPairStatus = CHCAPairError;
	#ifdef  CH_IPANEL_MGCA
	                 CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_FAIL,NULL,0);
	#else							   
			  
			  CHCA_CardSendMess2Usif(NO_STB_CARD);	 /*add this on 041118*/     
	#endif
		 	  /*do_report(severity_info,"\n[CHMG_CtrlCAStart==>]scramb card stb not paired\n"); */ 
#ifndef            NAGRA_PRODUCE_VERSION  
                        CHCA_PairMepgDisable(true,true);
#endif			  
		 	  break;
			  
		 case MGAPIEmpty:
			   bErrCode = true;
		 	   do_report(severity_info,"\n[CHMG_CtrlCAStart==>]scramb no good card \n");   
		 	   break;
			 
		 default:
			   bErrCode = true;	
		 	   do_report(severity_info,"\n[CHMG_CtrlCAStart==>]scramb unkonw err\n");   
		 	   break;
	}
	#ifndef  CH_IPANEL_MGCA


	 if(bErrCode)
	 {
	         CHCA_BOOL   VideoClose,AudioClose;
                VideoClose = AudioClose = false; 

                stTrackTableInfo.ProgElementCurRight[0] = 0;
                stTrackTableInfo.ProgElementCurRight[1] = 0;
				
 		  switch(stTrackTableInfo.ProgServiceType)	
		  {
                      case TelevisionService:
				   VideoClose=AudioClose=true;	  	
				   break;

			 case RadioService:
			 	   AudioClose = true;
			 	   break;

			 case MosaicVideoService:
			 	   VideoClose = true;
			 	   break;

			 default:
			 	   break;
		  }

                CHCA_MepgDisable(VideoClose,AudioClose);
                /*do_report(severity_info,"\n[CHMG_CtrlCAStart==>]CHCA_MepgDisable::VideoClose[%d] AudioClose[%d]\n",VideoClose,AudioClose);	*/
	 }
	 #endif
#endif

	 
	return bErrCode;	
							                
}


/*******************************************************************************
 *Function name:  CHMG_SRCStatusChange
 * 
 *
 *Description:  check the status of th stream
 *                  
 *
 *Prototype:
 *          void CHMG_SRCStatusChange(S16   iSrcChangedStatus)
 *
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *        src status:
 *            MGDDISrcDisconnected:   Source disconnected
 *            MGDDISrcConnected:   Source Connected
 *            MGDDISrcTrpChanged: New transponder
 *
 *******************************************************************************/
void CHMG_SRCStatusChange(S16   iSrcChangedStatus)
{
        TMGDDIEvSrcStatusChangedExCode     SrcStatus;
        CHCA_CALLBACK_FN                           CallBackFun[CTRL_MAX_NUM_SUBSCRIPTION]; 
        TCHCA_HANDLE                                   CtrlSource;
	 CHCA_UINT                                         iSubIndex,TotalSub;	

#if    0 /*add this on 050429*/
	 CatDataBufferInit();
#endif 


	 SrcStatus = (TMGDDIEvSrcStatusChangedExCode)iSrcChangedStatus; 	 

	 switch(SrcStatus)
	 {
	        case MGDDISrcConnected:
#if    0				
#ifndef            NAGRA_PRODUCE_VERSION /*add this on 050510*/
                        CHCA_RestartTimer(); 
#endif
#endif
			   /*if(!CHCA_CheckCardReady())
			   {
			        do_report(severity_info,"\n[ChStreamStatusCheck==>] Source Connected,but card not ready\n"); 
			        return;
			   }		*/
#ifdef              CHPROG_DRIVER_DEBUG				
			   do_report(severity_info,"\n[ChStreamStatusCheck==>] Source Connected\n"); 	
#endif
			   break;
			   
               case MGDDISrcDisconnected:
#ifdef              CHPROG_DRIVER_DEBUG			   	
			   do_report(severity_info,"\n[ChStreamStatusCheck==>] Source disconnected\n"); 
#endif
			   break;
			   
               case MGDDISrcTrpChanged:
                       /*if(!CHCA_CheckCardReady())
                       {
      			       do_report(severity_info,"\n[ChStreamStatusCheck==>] New transponder,but card not ready\n"); 
                            return;
                       }	*/			
			   	
#ifdef              CHPROG_DRIVER_DEBUG				
			   do_report(severity_info,"\n[ChStreamStatusCheck==>] New transponder\n"); 
#endif
			   break;
	 }

        semaphore_wait(pSemCtrlOperationAccess);
	 /*strcpy(CtrlSource, SRCOperationInfo.hSource);*/
	 CtrlSource = SRCOperationInfo.hSource;
	 TotalSub = 0;
 	 for(iSubIndex=0;iSubIndex<CTRL_MAX_NUM_SUBSCRIPTION;iSubIndex++)
 	 {
               if((SRCOperationInfo.EvenSubscribeInfo[iSubIndex].CardNotifiy_fun!=NULL)&&\
		     (SRCOperationInfo.EvenSubscribeInfo[iSubIndex].enabled))
               {
                       CallBackFun[TotalSub]=SRCOperationInfo.EvenSubscribeInfo[iSubIndex].CardNotifiy_fun;
			  TotalSub++;		   
		  }
		  /*else
		  {
                       CallBackFun[iSubIndex] = NULL; 
		  }*/
	 }
        semaphore_signal(pSemCtrlOperationAccess);
	 

#if 1  /*add this on 050311 just for test*/
        /*do_report(severity_info,"\n[CHMG_SRCStatusChange==>]TotalSub[%d]\n",TotalSub);	*/
	 if(TotalSub==0)	
	 {
	 	do_report(severity_info,"\n[ChStreamStatusCheck==>] TotalSub == 0\n"); 
	       semaphore_signal(pSemCtrlOperationAccess);
              return;
	 }

        semaphore_wait(pSemMgApiAccess);	
        for(iSubIndex=0;iSubIndex<TotalSub;iSubIndex++)
        {
               if(CallBackFun[iSubIndex]!=NULL)	 
               {
 	                CallBackFun[iSubIndex](CtrlSource,(byte)MGDDIEvSrcStatusChanged,(word)SrcStatus,NULL);	
		 }
	 }
        semaphore_signal(pSemMgApiAccess);	
#endif
		
 
        return;
		
}


/*******************************************************************************
 *Function name:  CHMG_CheckAppRight
 * 
 *
 *Description:   
 *                  
 *
 *Prototype:
 *          CHCA_BOOL  CHMG_CheckAppRight(CHCA_UCHAR*    PPBuffer)
 *
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
 *  2005-3-4    modification          delete pair check for application
 * 
 *******************************************************************************/
CHCA_BOOL  CHMG_CheckAppRight(CHCA_UCHAR*    PPBuffer)
{
       CHCA_BOOL                    bErrCode = true;
       TMGAPIStatus                   iApiStatus;
	#ifndef  CH_IPANEL_MGCA
       /*clear the display screen*/
	CHCA_CardSendMess2Usif(NORMAL_STATUS);/*add this on 041125*/   
      #endif


       if(stTrackTableInfo.ProgType==FTA_PROGRAM)
       {
                /*if(PairingCheck_FTA)
                {
                       CardPairStatus = CHCAPairOK;
                }*/  /*delete this on 050222*/
	   
                App_CA_CurRight = NORMAL_STATUS;
       }
	else
	{
                /*if(PairingCheck_NOFTA)
                {
                       CardPairStatus = CHCAPairOK;
                }*//*delete this on 050222*/
	
                if(CHCA_AppPmtParseDes(PPBuffer))
                {
                       App_CA_CurRight = NO_RIGHT;
		  }
		  else
		  {
                       App_CA_CurRight = NORMAL_STATUS;
		  }
	}

       do_report(severity_info,"\n[CHMG_CheckAppRight==>] App_CA_CurRight[%d] ProgType[%d]\n",App_CA_CurRight,stTrackTableInfo.ProgType);

       if(psemPmtOtherAppParseEnd != NULL)
           semaphore_signal(psemPmtOtherAppParseEnd);	 	 	
 

       return bErrCode; 
 
}




/*******************************************************************************
 *Function name:  CHMG_FTAProgPairCheck
 * 
 *
 *Description:  fta program pairing check 
 *                  
 *
 *Prototype:
 *          BOOL CHMG_FTAProgPairCheck(void)
 *
 *
 *
 *input:none
 *      
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
#ifdef       PAIR_CHECK
BOOL CHMG_FTAProgPairCheck(void)
{
        BOOL                               bErrCode=true;
        TMGAPIStatus                   iApiStatus;
	 CHCA_UCHAR                   SerialNum[6];	 

 	 semaphore_wait(pSemMgApiAccess);	
        iApiStatus = MGAPIPairedCheckFTA();
	 semaphore_signal(pSemMgApiAccess);

	 switch(iApiStatus)
	 {
               case MGAPIOk:
			   /*CHCA_CardSendMess2Usif(NORMAL_STATUS);add this on 050118*/	
			   bErrCode = false;
			   PairingCheck=PairingCheck_FTA;
			   if(PairingCheck_FTA)
			   {
#if   0			   
#ifndef                     NAGRA_PRODUCE_VERSION 
				     if(PairfirstPownOn && (CardPairStatus==CHCA_PairError))	
				     {
                                       PairfirstPownOn = false;
					    do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>] The New card is not paired\n");	
                                       CHCA_PairMepgDisable(true,true);
			
					    CHCA_CardSendMess2Usif(NO_STB_CARD);	
				     }
				     else
#endif	
#endif
                                 {
                             		#ifndef  CH_IPANEL_MGCA    
                                       CHCA_CardSendMess2Usif(NORMAL_STATUS);/*add this on 050118*/
						#else
                                      CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_OK,NULL,0);/*add this on 050118*/
					  CHCA_CardSendMess2Usif(CH_CA2IPANEL_Have_Right,NULL,0);/*add this on 050118*/
	
						#endif
					    CHCA_GetCardContent(SerialNum);				   
                                       if(memcmp(pastPairInfo.CaSerialNumber,SerialNum,6))
                                       {
                                            memcpy(pastPairInfo.CaSerialNumber,SerialNum,6);
                                            WritePairInfo();					   
                                       }
                                       
                                       CardPairStatus = CHCAPairOK;
                                       
#ifndef                           NAGRA_PRODUCE_VERSION  			  
                                       CHCA_PairMepgEnable(true,true);
#endif
                                       do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]Fta Pair success\n"); 
                               }
			   }
			   else
			   {
                             		#ifndef  CH_IPANEL_MGCA    
			   
			           CHCA_CardSendMess2Usif(NORMAL_STATUS);	
						#else
					   CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_OK,NULL,0);/*add this on 050118*/
                                      CHCA_CardSendMess2Usif(CH_CA2IPANEL_Have_Right,NULL,0);/*add this on 050118*/

						#endif
#ifndef                    NAGRA_PRODUCE_VERSION 
				    /*do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]No Fta Pair \n");  */ 
                                CHCA_PairMepgEnable(true,true);
#endif
			   }
			   break;
			  
		 case MGAPINotEntitled:
		 	  CardPairStatus = CHCAPairError;
#ifndef  CH_IPANEL_MGCA  			  
 	  		  CHCA_CardSendMess2Usif(NO_STB_CARD);	 /*add this on 041118*/	
#else
                       CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_FAIL,NULL,0);/*add this on 050118*/

#endif
		 	  do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]card stb not paired\n");   
			  
#ifndef           NAGRA_PRODUCE_VERSION  			  
			  CHCA_PairMepgDisable(true,true);
#endif
		 	  break;
			  
		 case MGAPIEmpty:
		 	  do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]no good card \n");   
		 	  break;
			 
		 default:
		 	 do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]unkonw err\n");   
		 	 break;
	 }
		#ifndef  CH_IPANEL_MGCA
 
	 if(bErrCode)
	 {
	         CHCA_BOOL   VideoClose,AudioClose;
                VideoClose = AudioClose = false; 

                stTrackTableInfo.ProgElementCurRight[0] = 0;
                stTrackTableInfo.ProgElementCurRight[1] = 0;
				
 		  switch(stTrackTableInfo.ProgServiceType)	
		  {
             case TelevisionService:
				   VideoClose=AudioClose=true;	  	
				   break;

			 case RadioService:
			 	   AudioClose = true;
			 	   break;

			 case MosaicVideoService:
			 	   VideoClose = true;
				    /*20060823 change for Mosaic*/
                              AudioClose = true;
				    /************************/
			 	   break;

			 default:
			 	   break;
		  }
             if(pstBoxInfo->abiBoxPoweredState==1)/*wz add 20070207 for BEIJING */
             	{
                CHCA_MepgDisable(VideoClose,AudioClose);
             	}
                /*do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]CHCA_MepgDisable,VideoClose[%d] AudioClose[%d]\n",VideoClose,AudioClose);	*/
	 }
	 else
	 {
	         CHCA_BOOL   VideoOpen,AudioOpen;
                VideoOpen = AudioOpen = false; 
				
                stTrackTableInfo.ProgElementCurRight[0] = 1;
                stTrackTableInfo.ProgElementCurRight[1] = 1;
			 
		  switch(stTrackTableInfo.ProgServiceType)	
		  {
              case TelevisionService:
				   VideoOpen=AudioOpen=true;	  	
				   break;

			 case RadioService:
			 	   AudioOpen = true;

				   if(CH_GetCurApplMode() != APP_MODE_MOSAIC)/* 20071106  wz add 避免在马赛克广播页面打开视频*/
				   	{
					if(stTrackTableInfo.iVideo_Track!=CHCA_INVALID_TRACK)
					 	{
					 	VideoOpen = true;
					 	}
				   	}
			
			 	   break;

			 case MosaicVideoService:
			 	   VideoOpen = true;
				   /*20060823 change for Mosaic*/
                   AudioOpen = true;
				   /************************/
			 	   break;

			 default:
			 	   break;
		  }
                CHCA_MepgEnable(VideoOpen,AudioOpen);
                /*do_report(severity_info,"\n[CHMG_FTAProgPairCheck==>]CHCA_MepgEnable,VideoOpen[%d] AudioOpen[%d]\n",VideoOpen,AudioOpen);*/	
	 }
#endif
        return 	bErrCode;	 
}
#else/*xjp 060110 add*/

BOOL CHMG_FTAProgCheck(void)
{

#ifndef  CH_IPANEL_MGCA    
      CHCA_CardSendMess2Usif(NORMAL_STATUS);	
#else
	CHCA_CardSendMess2Usif(CH_CA2IPANEL_Have_Right,NULL,0);	 
#endif
	 do_report(severity_info,"\n[CHMG_FTAProgCheck==>]  Fta check !\n"); 
#ifndef                     NAGRA_PRODUCE_VERSION
        CHCA_PairMepgEnable(true,true);
#endif
	return false;
}

#endif

/*******************************************************************************
 *Function name:  ChProgInfoProcess
 * 
 *
 *Description:  manage some new program info 
 *                  
 *
 *Prototype:
 *          void   CHMG_ProgCtrlChange(TMGAPIOrigin* ipOrigin,U8*  PMTBuffer)
 *
 *
 *
 *input:
 *      
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
void   CHMG_ProgCtrlChange(TMGAPIOrigin* ipOrigin,U8*  PMTBuffer)
{
       TMGAPIStatus                            iApiStatus/*,iiApiStatus*/;
	U16                                           iElementPid[2]; 	
       U8                                            uPidNum,PidIndex;
       BOOL                                        ErrCode = false;
	CHCA_BOOL                              CardReady;   
	clock_t                                      WaitTime;          


       iElementPid[0]=CHCA_DEMUX_INVALID_PID;
	iElementPid[1]=CHCA_DEMUX_INVALID_PID;

	
       /*CHMG_CtrlInstanceInit();*/
       /*CHCA_CardSendMess2Usif(NORMAL_STATUS);delete  this on 050118*/

       /*do_report(severity_info,"\n[CHMG_ProgCtrlChange==>]  Program Type[%d]\n",stTrackTableInfo.ProgType);*/

       if(stTrackTableInfo.ProgType == FTA_PROGRAM)
       {
#ifdef     PAIR_CHECK   
               ErrCode = CHMG_FTAProgPairCheck();
#else
		CHMG_FTAProgCheck();
#endif
		 return;			  
       }

	   
       /*do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] NOFTA Program\n");*/

#if 0 /*add this on 050124*/
	if((Audio_Track==CHCA_INVALID_TRACK)&&(Video_Track != CHCA_INVALID_TRACK))
	{
              do_report(severity_info,"\n[CHMG_ProgCtrlChange==>]Mosaic video page\n");
              return;	
	}
#endif	

       if(stTrackTableInfo.ProgServiceType == TelevisionService)
       { 
             if(stTrackTableInfo.ProgElementType[0] == FTA_PROGRAM)
             {
                      stTrackTableInfo.ProgElementCurRight[0] = 1;
                      CHCA_MepgEnable(true,false);
#ifdef            CHPROG_DRIVER_DEBUG					  
		        do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] Video Clear\n");	
#endif
	      }

             if(stTrackTableInfo.ProgElementType[1] == FTA_PROGRAM)
             {
                      stTrackTableInfo.ProgElementCurRight[1] = 1;
                      CHCA_MepgEnable(false,true);
#ifdef            CHPROG_DRIVER_DEBUG					  
			 do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] Audio Clear\n");	
#endif
	      }
	}


       if(pChMgApiCtrlHandle !=NULL)
       {
                CHCA_UINT     i=0;
                uPidNum = stTrackTableInfo.iProgNumofMaxTrack; 

                if((uPidNum>0) && (uPidNum<=2))
                {
                         for(PidIndex=0;PidIndex<2;PidIndex++)
                         {
                                 if(stTrackTableInfo.ProgElementPid[PidIndex] != CHCA_DEMUX_INVALID_PID)
                                 {
                                        iElementPid[i++] = stTrackTableInfo.ProgElementPid[PidIndex];
		                   }
			    }
		  }
		  else
		  {
                        ErrCode = true;
		  }

		  if(ErrCode)	
		  {
		        do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] Current PID info is wrong \n");
                      return;
		  }

#if   0
                do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] ElementPidNum[%d] ElementPid::",uPidNum);
	         for(i=0;i<uPidNum;i++)
		  do_report(severity_info,"%4x",iElementPid[i]);	
		  do_report(severity_info,"\n");	
#endif	

 	         semaphore_wait(pSemMgApiAccess);  
                iApiStatus = MGAPICtrlLoad(pChMgApiCtrlHandle,ipOrigin,PMTBuffer,uPidNum,iElementPid);
		  semaphore_signal(pSemMgApiAccess);
		  
		  if(iApiStatus != MGAPIOk)
                {
                     do_report(severity_info,"\n[MGAPICtrlLoad==>] ");		   	
                     CHMG_GetApiStatusReport(iApiStatus);
			return;		 
		  }		

                if(CHCA_CheckCardReady())
                {
                        CHMG_CtrlCAStart(pChMgApiCtrlHandle);
                }
		  else
		  {
                        do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] no start the ctrl ca without the card\n");
		  }
     }
     else
     {
              do_report(severity_info,"\n[CHMG_ProgCtrlChange==>] The CtrlApiHandel is NULL\n");
     }       
	   
     return; 
}


/*******************************************************************************
 *Function name:  CHMG_ProgCtrlUpdate
 * 
 *
 *Description:  manage some updated program info 
 *                  
 *
 *Prototype:
 *         void  CHMG_ProgCtrlUpdate(TMGAPIOrigin* ipOrigin,U8*  PMTBuffer)
 *
 *
 *
 *input:
 *      
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


CHCA_BOOL  CHMG_ProgCtrlUpdate(U8*  PMTBuffer)
{
       TMGAPIStatus                            iApiStatus;
	BOOL                                        bErrCode = false; 
	STBStaus                                  CurSTBStaus;
	CHCA_BOOL                              VideoClose,AudioClose;
	CHCA_BOOL                              bReturnCode=false;
	CHCA_UCHAR                            SerialNum[6]; /*add this on 050315*/
	CHCA_BOOL                              AlreadyExist=false;

        PMTUPDATE = true;  /*add this on 050316*/

       /*CHCA_CardSendMess2Usif(NORMAL_STATUS);add this on 050118*/
	
       if(stTrackTableInfo.ProgType == FTA_PROGRAM)
       {
                /*do_report(severity_info,"\n[CHMG_ProgCtrlUpdate==>] FTA Program\n");*/
#ifdef      PAIR_CHECK   
                bErrCode = CHMG_FTAProgPairCheck();
                if(bErrCode)
                {
                     do_report(severity_info,"\n[CHMG_ProgCtrlUpdate::]Fail to pair the clear program\n");
	         }
#else
		CHMG_FTAProgCheck();
#endif
		  /*GetEcmFilterIndex();  add this on 050322,get the Ecm Filter after parsing the pmt table*/		

		  return bReturnCode;			  
       }
	/*else
	{
               do_report(severity_info,"\n[CHMG_ProgCtrlUpdate==>] NOFTA Program\n");
	}*/


       if(stTrackTableInfo.ProgServiceType==TelevisionService)
       { 
             if(stTrackTableInfo.ProgElementType[0] == FTA_PROGRAM)
             {
                      stTrackTableInfo.ProgElementCurRight[0] = 1;
                      CHCA_MepgEnable(true,false);
		        do_report(severity_info,"\n[CHMG_ProgCtrlUpdate==>] Video Clear\n");			  
	      }

             if(stTrackTableInfo.ProgElementType[1] == FTA_PROGRAM)
             {
                      stTrackTableInfo.ProgElementCurRight[1] = 1;
                      CHCA_MepgEnable(false,true);
			 do_report(severity_info,"\n[CHMG_ProgCtrlUpdate==>] Audio Clear\n");			  
	      }
	}

        semaphore_wait(pSemMgApiAccess);
        iApiStatus = MGAPICtrlChange(pChMgApiCtrlHandle,PMTBuffer);
	 semaphore_signal(pSemMgApiAccess);
        /*do_report(severity_info,"\n[MGAPICtrlChange]\n");					
        CHMG_GetApiStatusReport(iApiStatus); 	*/	

        switch(iApiStatus)
        {
              case  MGAPIOk:
#if 1  /*add this on 050421*/
                        EcmDataStop=false;
#endif			   	
	
#ifdef      PAIR_CHECK   
			  	
 			   /*add this on 050315*/
			  if(PairingCheck_NOFTA)
			  {
			           CHCA_GetCardContent(SerialNum);
                                if(memcmp(pastPairInfo.CaSerialNumber,SerialNum,6))
                                {
                                     memcpy(pastPairInfo.CaSerialNumber,SerialNum,6);
                                     WritePairInfo();					   
                                }
				    CardPairStatus = CHCAPairOK;	
			  }		
#endif			  
			   /*add this on 050315*/	
			   break;	

		case  MGAPIIdling:
#ifdef              CHPROG_DRIVER_DEBUG			
			   do_report(severity_info,"\n[MGAPICtrlChange==>]No PMT table has been loaded,call the function chmg_progctrlupdate!!\n");
#endif
			   bReturnCode=true;
			   break;

		case  MGAPIAlreadyExist:/*add this item on 050310*/
#if 1  /*add this on 050421*/
                        EcmDataStop=false;
#endif			   	
			
			   AlreadyExist = true;
			   do_report(severity_info,"\n[MGAPICtrlChange==>]The version of the new PMT table is the same than the one loaded!!\n");
			   break;

		default:
                        VideoClose = AudioClose = false; 
		          switch(stTrackTableInfo.ProgServiceType)	
		          {
                               case TelevisionService:
                                       if(stTrackTableInfo.ProgElementType[0] == SCRAMBLED_PROGRAM)
 	                                {
                                            VideoClose = true;
		                         }

		                         if(stTrackTableInfo.ProgElementType[1] == SCRAMBLED_PROGRAM)
		                         {
                                            AudioClose = true; 
		                         }					  	
				           break;

			          case RadioService:
			 	           AudioClose = true;
			 	           break;

			          case MosaicVideoService:
			 	           VideoClose = true;
			 	           break;

			          default:
			 	          break;
		         }
				  
                       CurSTBStaus = NO_RIGHT;
			  if(PairingCheck_NOFTA)
			  {
                              if(CardPairStatus == CHCAPairError)
                              {
                                     do_report(severity_info,"\n[CHMG_ProgCtrlUpdate==>] CHCAPairError\n");
                                     CurSTBStaus = NO_STB_CARD;
				  }
			  }
	#ifndef  CH_IPANEL_MGCA    	                  
      	                CHCA_CardSendMess2Usif(CurSTBStaus);   
                       CHCA_MepgDisable(VideoClose,AudioClose);
	#else
                    if(CurSTBStaus== NO_STB_CARD)
                    	{
                           CHCA_CardSendMess2Usif(CH_CA2IPANEL_PAIR_FAIL,NULL,0);
                    	}
		else if(CurSTBStaus == NO_RIGHT)
			{
                              CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);
			}
	#endif
                       do_report(severity_info,"\n[CHMG_ProgCtrlUpdate==>]CHCA_MepgDisable,VideoClose[%d] AudioClose[%d]\n",VideoClose,AudioClose);	
			  break;
 	    }

#if 0
	    if(!AlreadyExist)	
	    {
		   GetEcmFilterIndex();  /*add this on 050322,get the Ecm Filter after parsing the pmt table*/		
           }
#endif		

	    return bReturnCode;   
}


/*******************************************************************************
 *Function name:  CHCA_ProgInit
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          BOOL  CHCA_ProgInit ( void )
 *       
 *
 *input:
 *      
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
CHCA_BOOL  CHCA_ProgInit ( void )
{
       message_queue_t      *pstMessageQueue = NULL;
	CHCA_UINT               MessSize=0;   

       MessSize =   (CHCA_UINT)sizeof(chca_mg_cmd_t);

	do_report(severity_info,"\n[CHCA_ProgInit==>]Ca Module Message size[%d]\n",MessSize);   

	if ( ( pstMessageQueue  = message_create_queue_timeout ( MessSize,15) ) == NULL )
	{
		STTBX_Report( ( STTBX_REPORT_LEVEL_ERROR, "Unable to create pmt info  message queue\n" ) );
	}
	else
	{
		/* register this symbol for other module access */
		symbol_register ( "mgpmtinfo_queue", ( void * ) pstMessageQueue );
	}     	  

       if ( symbol_enquire_value ( "mgpmtinfo_queue", ( void ** ) &pstCHCAPMTMsgQueue ) )
	{
              do_report ( severity_error, "%s %d> Can't find Mg PMT  message queue\n",
                                 __FILE__,
                                 __LINE__ );

              return  true;
	}


#if   1
	 if ( ( pstMessageQueue  = message_create_queue_timeout ( MessSize,15 ) ) == NULL )
	 {
		STTBX_Report( ( STTBX_REPORT_LEVEL_ERROR, "Unable to create cat info  message queue\n" ) );
	 }
	 else
	 {
		/* register this symbol for other module access */
		symbol_register ( "mgcatinfo_queue", ( void * ) pstMessageQueue );
	 }     	  

        if ( symbol_enquire_value ( "mgcatinfo_queue", ( void ** ) &pstCHCACATMsgQueue ) )
	 {
              do_report ( severity_error, "%s %d> Can't find Mg CAT  message queue\n",
                                 __FILE__,
                                 __LINE__ );

              return  true;
	}
#endif		

        /*add this on 050115*/
	 if ( ( pstMessageQueue  = message_create_queue_timeout ( MessSize,15 ) ) == NULL )
	 {
		STTBX_Report( ( STTBX_REPORT_LEVEL_ERROR, "Unable to create emm info  message queue\n" ) );
	 }
	 else
	 {
		/* register this symbol for other module access */
		symbol_register ( "mgemminfo_queue", ( void * ) pstMessageQueue );
	 }     	  

        if ( symbol_enquire_value ( "mgemminfo_queue", ( void ** ) &pstCHCAEMMMsgQueue ) )
	 {
              do_report ( severity_error, "%s %d> Can't find Mg EMM  message queue\n",
                                 __FILE__,
                                 __LINE__ );

              return  true;
	}


#if  1/*add this on 050308*/
	 if ( ( pstMessageQueue  = message_create_queue_timeout ( MessSize,20 ) ) == NULL )
	 {
		STTBX_Report( ( STTBX_REPORT_LEVEL_ERROR, "Unable to create ecm info  message queue\n" ) );
	 }
	 else
	 {
		/* register this symbol for other module access */
		symbol_register ( "mgecminfo_queue", ( void * ) pstMessageQueue );
	 }     	  

        if ( symbol_enquire_value ( "mgecminfo_queue", ( void ** ) &pstCHCAECMMsgQueue ) )
	 {
              do_report ( severity_error, "%s %d> Can't find Mg ECM  message queue\n",
                                 __FILE__,
                                 __LINE__ );

              return  true;
	}
#endif

        memset(CatSectionBuffer,0,1024);  /*add this on 050327*/
        memset(CAT_Buffer,0,1024);  /*add this on 050327*/


      	 ChResetPMTStreamInfo();  /*add this on 041111*/
      	 ChResetPMTSTrackInfo();  /*add this on 041203*/
        stTrackTableInfo.iProgramID = CHCA_INVALID_LINK;  /*add this on 050105*/
        stTrackTableInfo.iTransponderID = CHCA_INVALID_LINK; /*add this on 050105*/


#ifdef   PAIR_CHECK/*add this on 050113*/
        if(PairMgrInit())
        {
		do_report ( severity_info,"PairMgrInit error!\n" );
		return true;
	 }
#endif



        if(ChCtrlInstanceInit()==true)
        {
              return true;
	 }


        /*pmt and ecm table*/
	 /*if ( ( pstCHCAPMTMsgQueue = message_create_queue ( 20,50) ) == NULL )
	 {
		do_report ( severity_info, "can't create pmt queue !!\n" );
	 }*/

#ifdef   CADATA_TEST  /*for data capture test on 040929*/
         if ( ( ptidPMTMonitorTask = Task_Create ( DVBCADataProcessTest,
			                                                    NULL,
			                                                    PMT_PROCESS_WORKSPACE,
			                                                    PMT_PROCESS_PRIORITY,
			                                                    "DVBCADataProcessTest",
			                                                    0 ) ) == NULL )
	 {
		  do_report ( severity_info,"CA DATA  PROCESS TEST Create Error !\n" );
		  return true;
	 }
#else
	#if 1/*060117 xjp change for STi5105 SRAM is small*/
        if ( ( ptidPMTMonitorTask = Task_Create ( ChDVBPMTMonitor,
			                                                    NULL,
			                                                    PMT_PROCESS_WORKSPACE,
			                                                    PMT_PROCESS_PRIORITY,
			                                                    "ChDVBPMTMonitor",
			                                                    0 ) ) == NULL )
	 {
		  do_report ( severity_info,"PMT Mointor Create Error !\n" );
		  return true;
	 }
	#else
		{		
			int i_Error=-1;


			g_PMTMonitorStack = memory_allocate( SystemPartition, PMT_PROCESS_WORKSPACE );
			if( NULL == g_PMTMonitorStack )
			{
				do_report ( severity_error, "ChCardInit=> Unable to create ChDVBPMTMonitor for no memory\n" );
				return  true;		
			}
			i_Error = task_init( (void(*)(void *))ChDVBPMTMonitor, 
											NULL, 
											g_PMTMonitorStack, 
											PMT_PROCESS_WORKSPACE, 
											&ptidPMTMonitorTask, 
											&g_PMTMonitorTaskDesc, 
											PMT_PROCESS_PRIORITY, 
											"ChDVBPMTMonitor", 
											0 );

			if( -1 == i_Error )

			{
				do_report ( severity_error, "ChCardInit=> Unable to create ChDVBPMTMonitor for task init\n" );
				return  true;	
			}


		}
	#endif
#endif	


#if   1  /*060117 xjp change for STi5105 SRAM is small*/
        if ( ( ptidCATMonitorTask = Task_Create ( ChDVBCATMonitor,
			                                                    NULL,
			                                                    CAT_PROCESS_WORKSPACE,
			                                                    CAT_PROCESS_PRIORITY,
			                                                    "ChDVBCATMonitor",
			                                                    0 ) ) == NULL )
	 {
		  do_report ( severity_info,"CAT Mointor Create Error !\n" );
		  return true;
	 }
#else
	{		
		int i_Error=-1;


		g_CATMonitorStack = memory_allocate( SystemPartition, CAT_PROCESS_WORKSPACE );
		if( NULL == g_CATMonitorStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBCATMonitor for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))ChDVBCATMonitor, 
										NULL, 
										g_CATMonitorStack, 
										CAT_PROCESS_WORKSPACE, 
										&ptidCATMonitorTask, 
										&g_CATMonitorTaskDesc, 
										CAT_PROCESS_PRIORITY, 
										"ChDVBCATMonitor", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBCATMonitor for task init\n" );
			return  true;	
		}


	}	
#endif		


#if 1  /*060117 xjp change for STi5105 SRAM is small*/
        if ( ( ptidEMMMonitorTask = Task_Create ( ChDVBEMMMonitor,
			                                                    NULL,
			                                                    EMM_PROCESS_WORKSPACE,
			                                                    EMM_PROCESS_PRIORITY,
			                                                    "ChDVBEMMMonitor",
			                                                    0 ) ) == NULL )
	 {
		  do_report ( severity_info,"EMM Mointor Create Error !\n" );
		  return true;
	 }
#else
	{		
		int i_Error=-1;

		
		g_EMMMonitorStack = memory_allocate( SystemPartition, EMM_PROCESS_WORKSPACE );
		if( NULL == g_EMMMonitorStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBEMMMonitor for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))ChDVBEMMMonitor, 
										NULL, 
										g_EMMMonitorStack, 
										EMM_PROCESS_WORKSPACE, 
										&ptidEMMMonitorTask, 
										&g_EMMMonitorTaskDesc, 
										EMM_PROCESS_PRIORITY, 
										"ChDVBEMMMonitor", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBEMMMonitor for task init\n" );
			return  true;	
		}


	}
#endif
		
#if    1 /*060117 xjp change for STi5105 SRAM is small*/ 
        if ( ( ptidECMMonitorTask = Task_Create ( ChDVBECMMonitor,
			                                                    NULL,
			                                                    ECM_PROCESS_WORKSPACE,
			                                                    ECM_PROCESS_PRIORITY,
			                                                    "ChDVBECMMonitor",
			                                                    0 ) ) == NULL )
	 {
		  do_report ( severity_info,"ECM Mointor Create Error !\n" );
		  return true;
	 }
#else
	{		
		int i_Error=-1;


		g_ECMMonitorStack = memory_allocate( SystemPartition, ECM_PROCESS_WORKSPACE );
		if( NULL == g_ECMMonitorStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBECMMonitor for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))ChDVBECMMonitor, 
										NULL, 
										g_ECMMonitorStack, 
										ECM_PROCESS_WORKSPACE, 
										&ptidECMMonitorTask, 
										&g_ECMMonitorTaskDesc, 
										ECM_PROCESS_PRIORITY, 
										"ChDVBECMMonitor", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBECMMonitor for task init\n" );
			return  true;	
		}


	}
#endif		

	 return false;
	 
}



/*******************************************************************************
 *Function name:  CHCA_StopBuildPmt
 * 
 *
 *Description:  stop to build the pmt task
 *                  
 *
 *Prototype:
 *         BOOL CHCA_StopBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType)
 *
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     2005-3-3    modify           add a new input parameter "iModuleType"
 * 
 * 
 *******************************************************************************/
BOOL CHCA_StopBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType)
{
       CHCA_BOOL                        bReturnCode;
       chca_pmt_monitor_type        ComType = CHCA_STOP_BUILDER_PMT;

        CH_FreeAllPMTMsg();

	/*bReturnCode = ChSendMessage2PMT(sProgramID,sTransponderID,ComType,iModuleType);   */ 

       return bReturnCode;
}


/*******************************************************************************
 *Function name:  CHCA_StartBuildPmt
 * 
 *
 *Description:  sart to build the pmt task  
 *                  
 *
 *Prototype:
 *          BOOL CHCA_StartBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType)
 *
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     2005-3-3    modify           add a new input parameter "iModuleType"
 * 
 * 
 *******************************************************************************/
BOOL CHCA_StartBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType)
{
       CHCA_BOOL                        bReturnCode;
       chca_pmt_monitor_type        ComType = CHCA_START_BUILDER_PMT;

	CH_FreeAllPMTMsg(); /*add this on 050405*/

       /*bReturnCode = ChSendMessage2PMT ( sProgramID,sTransponderID,ComType,iModuleType );*/

       return bReturnCode;
}



/*******************************************************************************
 *Function name:  CHCA_StopBuildCat
 * 
 *
 *Description:  stop to build the cat task
 *                  
 *
 *Prototype:
 *          BOOL CHCA_StopBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected)
 *
 *
 *
 *input:
 *      
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
BOOL CHCA_StopBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected )
{    
       CHCA_BOOL                       bReturnCode;
       chca_cat_monitor_type        ComType = CHCA_STOP_BUILDER_CAT;

	CH_FreeAllCATMsg();  /*add this on 050320*/   

	bReturnCode = ChSendMessage2CAT( sProgramID,sTransponderID,ComType,Disconnected );

       return bReturnCode;
}


/*******************************************************************************
 *Function name:  CHCA_StartBuildCat
 * 
 *
 *Description:  sart to build the cat task  
 *                  
 *
 *Prototype:
 *          BOOL CHCA_StartBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected)
 *
 *input:
 *      
 *output:
 *
 *Return Value:
 *     
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
BOOL CHCA_StartBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected)
{
       CHCA_BOOL                       bReturnCode;
       chca_cat_monitor_type        ComType = CHCA_START_BUILDER_CAT;

       bReturnCode = ChSendMessage2CAT( sProgramID,sTransponderID,ComType,Disconnected );  

       return bReturnCode;
}


/*******************************************************************************
 *Function name:  CHCA_DelElementPid
 * 
 *
 *Description:   this function removes the pid element
 *                  
 *
 *Prototype:
 *         CHCA_BOOL    CHCA_DelElementPid(U16  iPid)
 *
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *          true:   interface execution error
 *          false:  Execution OK  
 *
 *Comments:只是针对音频多半音使用
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL    CHCA_DelElementPid(U16  iPid)
{
      CHCA_BOOL           bReturnCode=true;
      TMGAPIStatus     	iApiStatus; 
      p_prog_type           bProgType; 
	  
      if(ChCheckProgTrackType(iPid,&bProgType))
      {
             return bReturnCode;
      }

      /*do_report(severity_info,"\n[CHCA_DelElementPid==>]Delete Audio Pid[%4x],bProgType[%d]\n",iPid,bProgType);	*/  

      semaphore_wait(pSemMgApiAccess); 
      if(bProgType == FTA_PROGRAM)	 
      {
              bReturnCode = false;
      }	  
      else
      {
              if(pChMgApiCtrlHandle !=NULL )	
              {
                    iApiStatus = MGAPICtrlDel(pChMgApiCtrlHandle,iPid);
					
              
                     switch(iApiStatus)
                     {
                          case MGAPIOk:
                                  /*do_report(severity_info,"\n[CHCA_DelElementPid==>]Execution OK\n");	*/  	
                                  bReturnCode = false;	  	
                                  break;	  	
                          case MGAPIIdling:
                                  do_report(severity_info,"\n[CHCA_DelElementPid==>]No PMT table has been loaded\n");	  	
                                  break;	  	
                          case MGAPINotFound:
                                  do_report(severity_info,"\n[CHCA_DelElementPid==>]Instance handle or PID unknown\n");	  	
                                  break;	  	
                          case MGAPIInvalidParam:
                                  do_report(severity_info,"\n[CHCA_DelElementPid==>]Parameter unll,aberrant or wrongly formatted\n");	  	
                                  break;	  	
                          case MGAPISysError:
                                  do_report(severity_info,"\n[CHCA_DelElementPid==>]There are no free descrambler/demultiplexer resources available\n");	  	
                                  break;	  	
                     }
              
             }
      }

      stTrackTableInfo.ProgElementPid[1] = CHCA_DEMUX_INVALID_PID;
      stTrackTableInfo.ProgTrackType[1] = CHCA_INVALID_TRACK;
      stTrackTableInfo.ProgElementSlot[1] = CHCA_DEMUX_INVALID_SLOT;
      
      stTrackTableInfo.ProgElementType[1] = FTA_PROGRAM;
      stTrackTableInfo.ProgElementCurRight[1] = -1;
      stTrackTableInfo.iProgNumofMaxTrack--;
      semaphore_signal(pSemMgApiAccess); 	  
	  
      return bReturnCode;
}

/*******************************************************************************
 *Function name:  CHCA_AddElementPid
 * 
 *
 *Description:   this interface adds the pid element 
 *                  
 *
 *Prototype:
 *        boolean    CHCA_AddElementPid(U16  iPid,U16  iOldPid)
 *
 *
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *          true:   interface execution error
 *          false:  Execution OK  
 *
 *Comments:只是针对音频多半音使用
 *     
 * 
 * 
 *******************************************************************************/
boolean    CHCA_AddElementPid(U16  iPid,U16  iOldPid)
{
      CHCA_BOOL           bReturnCode=true;
      TMGAPIStatus     	iApiStatus;  
      p_prog_type           bProgType; 
      STBStaus               CurSTBStaus;
      CHCA_UINT            i;
	  
      if(ChCheckProgTrackType(iPid,&bProgType))
      {
             return bReturnCode;
      }

      /*do_report(severity_info,"\n[CHCA_AddElementPid==>]Add New Audio Pid[%4x] Old One[%4x]\n",iPid,iOldPid);*/

#if 0
      for(i=0;i<2;i++)
      {
           if(iOldPid==EcmFilterInfo[i].FilterPID)
           {
                break;
	    }
      }

      if(i<2)
      {
            EcmFilterInfo[i].FilterPID = iPid;
            do_report(severity_info,"\n[CHCA_AddElementPid==>]Reset the filter[%d]\n",EcmFilterInfo[i].EcmFilterId);
            CHCA_ResetECMFilter(EcmFilterInfo[i].EcmFilterId,2,true);
      }

      for(i=0;i<2;i++)
      {
             Witch_Parity_to_be_parsed[i]= EVEN_ODD_Parity_to_be_parsed; 
             CHCA_RECMFilterTest(EcmFilterInfo[i].EcmFilterId);
      }
#endif	 
	  

      semaphore_wait(pSemMgApiAccess); 
      stTrackTableInfo.ProgElementPid[1] = iPid;
      stTrackTableInfo.ProgTrackType[1] = AUDIO_TRACK;
      stTrackTableInfo.ProgElementSlot[1] = Ch_GetAudioSlotIndex();
      stTrackTableInfo.iProgNumofMaxTrack++;	  
	  
      if(bProgType == FTA_PROGRAM)	 
      {
           bReturnCode = false;
           /*send the message to usif*/
	    if( stTrackTableInfo.ProgElementCurRight[0])
	    {
                  CurSTBStaus = NORMAL_STATUS;
	    }
	    else
	    {
                  CurSTBStaus = NO_VEDIO_RIGHT;
	    }

           stTrackTableInfo.ProgElementType[1] = FTA_PROGRAM;
           stTrackTableInfo.ProgElementCurRight[1] = 1;
           semaphore_signal(pSemMgApiAccess); 
	#ifndef  CH_IPANEL_MGCA    	                  

	    CHCA_CardSendMess2Usif(CurSTBStaus); 
	#endif
           return bReturnCode;
      }
      
      stTrackTableInfo.ProgElementType[1] = SCRAMBLED_PROGRAM;
 	  
      if(pChMgApiCtrlHandle !=NULL)	
      {
            iApiStatus = MGAPICtrlAdd(pChMgApiCtrlHandle,iPid);

	     switch(iApiStatus)
	     {
                  case MGAPIOk:
			      /*do_report(severity_info,"\n[CHCA_AddElementPid==>]Execution OK\n");*/	  	
			      bReturnCode = false;	  	
			      break;	  	
                 case MGAPIAlreadyExist:
			      do_report(severity_info,"\n[CHCA_AddElementPid==>]The element sprcified is already included in the list\n");	  	
			      break;	  	
                 case MGAPIIdling:
			      do_report(severity_info,"\n[CHCA_AddElementPid==>]No PMT table has been loaded\n");	  	
			      break;	  	
                 case MGAPINoResource:
			      do_report(severity_info,"\n[CHCA_AddElementPid==>]There are no free descrambler/demultiplexer resources available\n");	  	
			      break;	  	
                 case MGAPINotFound:
			      do_report(severity_info,"\n[CHCA_AddElementPid==>]Instance handle or PID unknown\n");	  	
			      break;	  	
                 case MGAPIInvalidParam:
			      do_report(severity_info,"\n[CHCA_AddElementPid==>]Parameter unll,aberrant or wrongly formatted\n");	  	
			      break;	  	

                 case MGAPISysError:
			      do_report(severity_info,"\n[CHCA_AddElementPid==>]There are no free descrambler/demultiplexer resources available\n");	  	
			      break;	  	
	    }
  
      }
      semaphore_signal(pSemMgApiAccess); 

      return bReturnCode;
}

/*******************************************************************************
 *Function name:  CHCA_CtrlUnsubscribe
 * 
 *
 *Description:  cancel the subscribe of the ctrl 
 *                  
 *
 *Prototype:
 *         CHCA_BOOL CHCA_CtrlUnsubscribe(CHCA_USHORT  iHandleCount)
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
CHCA_BOOL CHCA_CtrlUnsubscribe(CHCA_USHORT  iHandleCount)
{
       CHCA_BOOL         bErrCode=false;
       CHCA_USHORT     iHandle; 

	iHandle = iHandleCount -1; 
       if((iHandleCount==0)||(iHandle>=CTRL_MAX_NUM_SUBSCRIPTION))
       {
              bErrCode = true;
	       return bErrCode;		  
	}

#if 1/*delete this on 050306*/
	semaphore_wait(pSemCtrlOperationAccess);   
       SRCOperationInfo.EvenSubscribeInfo[iHandle].CardNotifiy_fun = NULL;
       SRCOperationInfo.EvenSubscribeInfo[iHandle].enabled = false;	
	semaphore_signal(pSemCtrlOperationAccess);
#endif
	

#ifdef   CHPROG_DRIVER_DEBUG
	do_report(severity_info,"\n[CHCA_CtrlUnsubscribe==>]end the unsubscribe\n");
#endif	

       return bErrCode;      
}


/*******************************************************************************
 *Function name: CHCA_CtrlSubscribe
 *           

 *Description: This function is used to subscribe to the different unexpected 
 *             source events for the source related to the hSource handle.The 
 *             subscription handle returned can be used to cancel subscription,
 *             if necessary.The unexpected events are notified by means of the 
 *             callback function provided at the address given in the hCallback
 *             parameter.
 *
 *             these unexpected events may be the following:      
 *             1) MGDDIEvSrcNoResource:   one of the resource related to the source
 *                                        is unavailable 
 *             2) MGDDIEvSrcStatusChanged: the status of a source has changed.
 *
 *Prototype:
 *      TCHCAEventSubscriptionHandle  CHCA_CtrlSubscribe
 *      ( TCHCASysSrcHandle  hSource,CHCA_CALLBACK_FN  hCallback)
 *
 *
 *input:
 *    hSource:   System source handle       
 *    hCallback: Callback function address       
 *
 *output:
 *           
 *Returned code:
 *    Subscription handle,if not null.       
 *       NULL, if a problem has occurred when processing.      
 *           - Source handle unknown.
 *           - Notification function address null.
 *           
 *Comments: 
 *        unexpected event:  
 *              MGDDIEvSrcNoResource  :  one of the resources related to the source is unabailable
 *              MGDDIEvSrcStatusChanged : the status of a source has changed.
 *
 *
 *******************************************************************************/
 /*TCHCAEventSubscriptionHandle  CHCA_CtrlSubscribe
       ( TCHCASysSrcHandle  hSource,CHCA_CALLBACK_FN  hCallback)*/
 CHCA_BOOL  CHCA_CtrlSubscribe
	 (TCHCASysSrcHandle  hSource, CHCA_CALLBACK_FN  hCallback,CHCA_UCHAR  *SubHandle)
      
 {
       CHCA_UCHAR                                   iHandleCount;
	CHCA_UCHAR                                   SrcBaseAdd[2];   
	/*CHCA_UINT                                      SrcEvenHandle;*/
       /*TCHCAEventSubscriptionHandle          SubscriptionHandle; */

       if((hSource==NULL)||(hSource!= (TCHCASysSrcHandle)MGTUNERDeviceName)||(hCallback==NULL)||(SubHandle==NULL))
	{
#ifdef     CHPROG_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHCA_CtrlSubscribe::] A parameter is unknown,null or invalid \n",
                               __FILE__,
                               __LINE__);
#endif
		return true;	  
	}

	SrcBaseAdd[0] =  CHCA_MGDDI_SRC_BASEADDRESS & 0xff;
       SrcBaseAdd[1] =  (CHCA_MGDDI_SRC_BASEADDRESS>>8) & 0xff;

       semaphore_wait(pSemCtrlOperationAccess);   /*add this on 050306*/
       /*find the callback addresss of one  enabled instance index is equal to the "hCallback"*/
	for(iHandleCount = 0; iHandleCount < CTRL_MAX_NUM_SUBSCRIPTION; iHandleCount++)
	{
	        if(SRCOperationInfo.EvenSubscribeInfo[iHandleCount].enabled)
	        {
                     if(SRCOperationInfo.EvenSubscribeInfo[iHandleCount].CardNotifiy_fun == hCallback)
                     {
                            break;
			}
		 }
	}

       if(iHandleCount >= CTRL_MAX_NUM_SUBSCRIPTION)
       {
  	        for(iHandleCount = 0;iHandleCount<CTRL_MAX_NUM_SUBSCRIPTION;iHandleCount++)
	        {
	              if(!SRCOperationInfo.EvenSubscribeInfo[iHandleCount].enabled)
	              {
                            SRCOperationInfo.EvenSubscribeInfo[iHandleCount].enabled = true;
				SRCOperationInfo.EvenSubscribeInfo[iHandleCount].CardNotifiy_fun=hCallback;	
				break;
		       }
	        }

	        if(iHandleCount >= CTRL_MAX_NUM_SUBSCRIPTION)
	        {
#ifdef            CHPROG_DRIVER_DEBUG			  
                      do_report(severity_info,"\n %s %d >[CHCA_CtrlSubscribe::] no src subscription  \n",
                                        __FILE__,
                                        __LINE__);
#endif
                      semaphore_signal(pSemCtrlOperationAccess);  /*add this on 050306*/
		        return true;	  
		 }
	}
	else
	{
#ifdef     CHPROG_DRIVER_DEBUG			  
               do_report(severity_info,"\n the call back function has already been subscribed! \n");
#endif
	}
	
	semaphore_signal(pSemCtrlOperationAccess);  /*add this on 050306*/
	
       iHandleCount ++;

       SubHandle[3] = SrcBaseAdd[1];
       SubHandle[2] = SrcBaseAdd[0];
       SubHandle[1] = 0x00;
       SubHandle[0] = iHandleCount;

#ifdef    CHPROG_DRIVER_DEBUG			  
       do_report(severity_info,"\n %s %d >[CHCA_CtrlSubscribe::] DDISource_SubEventHandle[%2x][%2x][%2x][%2x] \n",
                        __FILE__,
                        __LINE__,
                       SubHandle[3],
                       SubHandle[2],
                       SubHandle[1],
                       SubHandle[0]);
#endif

      return false;   

 }

/*******************************************************************************
 *Function name: CHCA_CtrlGetSources
 *           

 *Description: This function fills the list of source handles,whether active or not.
 *             This list is returned into the memory block provided at the address
 *             given by lhSources parameter,with nSources parameter providing the
 *             address of the number of sources in the list.The Mediaguard kernel is
 *             responsible for the memory allocation and release of the lhSources 
 *             memory block.The structure is no longer valid when the interface            
 *             function returns.
 *           
 *             A source is a possible input for the stream to which a demultiplexer
 *             and descrambler are related. A simple system will have only a single,
 *             whereas a more complex system may have several sources
 *           
 *             When the function is entered,the nSources parameter provides the 
 *             maximum number of source handles the list can contain.If the size of
 *             the memory block is insufficient,the nSources parameter is filled with 
 *             the maximum number of elements of the system,and the MGDDINoResource 
 *             code is returned.
 *           
 *             If the lhSources parameter is null,whereas the nSources parameter is
 *             not null, the function returns only the number of reader and returns
 *             the MGDDIOk code. 
 *           
 *
 *Prototype:
 *   CHCA_DDIStatus  CHCA_CtrlGetSources  
 *       (TCHCASysSrcHandle* lhSource,CHCA_USHORT*  nSources) 
 *
 *
 *input:
 *    lhSource:            Address of system source handle list       
 *    nSource:             Address of number of sources.        
 *
 *output:
 *
 *Returned code:
 *    CHCADDIOk:         The list of sources has been filled.       
 *    CHCADDIBadParam:   The nSources parameter is null       
 *    CHCADDINoResource: The size of the memory block is insufficient.       
 *    CHCADDIError:      Execution error or source list unavailable       
 *           
 *           
 *           
 *Comments:           
 *           
 *           
 *           
 *******************************************************************************/
CHCA_DDIStatus  CHCA_CtrlGetSources  
     (TCHCASysSrcHandle* lhSource,CHCA_USHORT*  nSources)
{
       CHCA_DDIStatus     StatusMgDdi = CHCADDIOK;
 
	if(lhSource==NULL)
       {
              StatusMgDdi = CHCADDINoResource;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHCA_CtrlGetSources::] The size of the memory block is insufficient \n",
                               __FILE__,
                               __LINE__);
#endif
              return (StatusMgDdi);	  
	}

	if(nSources==NULL)
	{
              StatusMgDdi = CHCADDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHCA_CtrlGetSources::] The nSources parameter is null \n",
                               __FILE__,
                               __LINE__);
#endif
              return (StatusMgDdi);	  
	}

	/*strcpy(*lhSource,MGTUNERDeviceName);*/

	*lhSource = (TCHCASysSrcHandle)MGTUNERDeviceName;

	*nSources = CHCA_MGDDI_STREAMSOURCE_NUM;
	
	/*strcpy(SRCOperationInfo.hSource, MGTUNERDeviceName);*/

	SRCOperationInfo.hSource = (TCHCASysSrcHandle)MGTUNERDeviceName;

#ifdef    CHPROG_DRIVER_DEBUG			  
        do_report(severity_info,"\n %s %d >[CHCA_CtrlGetSources::]  The list of sources has been filled \n",
                          __FILE__,
                          __LINE__);
#endif

       return (StatusMgDdi);	  	
}


/*******************************************************************************
 *Function name: MGDDISrcGetCaps
 *           

 *Description: This function returns the capacities of the source related to the 
 *             hSource handle.The capacities of the source are recorded in the 
 *             information structure provided at the address given in the pCaps
 *             parameter.
 *
 *Prototype:
 *     TMGDDIStatus MGDDISrcGetCaps      
 *           (TMGSysSrcHandle hSource,TMGDDISrcCaps* pCaps)     
 *
 *
 *input:
 *     hSource:        System source handle.      
 *     pCaps:          Address of an information structure of source       
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:         The source information has been filled        
 *    MGDDIBadParam:   A parameter is unknown,null or invalid       
 *    MGDDIError:      Interface execution error       
 *           
 *Comments:          
 *           
 *******************************************************************************/
CHCA_DDIStatus  CHCA_CtrlGetCaps
   (TCHCASysSrcHandle hSource)
{
       CHCA_DDIStatus     StatusMgDdi = CHCADDIOK;
 	
	if((hSource==NULL)||(hSource!= (TCHCASysSrcHandle)MGTUNERDeviceName))
	{
              StatusMgDdi =CHCADDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHCA_CtrlGetCaps::] hSource parameter is wrong \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  

	}

	return (StatusMgDdi);
}



/*******************************************************************************
 *Function name: CHCA_CtrlGetStatus
 *           
 *
 *Description: this function returns the status of the source related to the hSource handle
 *             
 *
 *Prototype:
 *      CHCA_DDIStatus CHCA_CtrlGetStatus(TCHCASysSrcHandle  hSource)      
 *           
 *input:
 *     hSource:    System source handle.
 *
 *output:
 *           
 *Returned code:
 *    CHCADDIOk:          The source is connected.       
 *    CHCADDINotFound:    The source is disconnected       
 *    CHCADDIBadParam:    Source handle unknown.       
 *    CHCADDIError:       Interface execution error.       
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
 CHCA_DDIStatus CHCA_CtrlGetStatus(TCHCASysSrcHandle  hSource)
 {
        CHCA_DDIStatus     StatusMgDdi = CHCADDIOK;
        CHCA_BOOL            SrcConnected;		
 	

	
	 if((hSource==NULL)||(hSource!=(TCHCASysSrcHandle)MGTUNERDeviceName))
	 {
              StatusMgDdi =CHCADDIBadParam;
#ifdef    CHPROG_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHCA_CtrlGetStatus::] A parameter is unknown,null or invalid \n",
                               __FILE__,
                               __LINE__);
#endif
	 }

        semaphore_wait(pSemCtrlOperationAccess);
        SrcConnected = SRCOperationInfo.bSrcConnected=true;
        semaphore_signal(pSemCtrlOperationAccess);

	 if(!SrcConnected)  
	 {
              StatusMgDdi =  MGDDINotFound;
#ifdef    CHPROG_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHCA_CtrlGetStatus::] The source is disconnected  \n",
                                __FILE__,
                                __LINE__);
#endif
              return (StatusMgDdi);	
	 }

	 return (StatusMgDdi); 
 
 }


/*******************************************************************************
 *Function name: CHCA_MosaicMepgEnable
 *           
 *
 *Description: Open the mosaic mpeg
 *             
 *
 *Prototype:
 *      CHCA_BOOL  CHCA_MosaicMepgEnable(CHCA_BOOL  iVideoOpen)
 *           
 *input:
 *     none
 *
 *output:
 *           
 *Returned code:
 *    
 *           
 *Comments: 
 *     2005-1-20            add the function      
 *           
 *******************************************************************************/

CHCA_BOOL  CHCA_MosaicMepgEnable(CHCA_BOOL  iVideoOpen)
{
      CHCA_BOOL                                               ReturnErr;
      CH6_VideoControl_Type                              VideoControl;
      CHCA_BOOL                                               AudioControl;
      CH6_AVControl_Type                                  ControlType;
#ifndef CH_IPANEL_MGCA 
      if(CH_GetStandByStatus())
      {
             do_report(severity_info,"\n[CHCA_MosaicMepgEnable==>]The Stb box is on Standby status \n");
             return ReturnErr;  
      }

#endif
      VideoControl = VIDEO_SHOWPIC;
      AudioControl = true; 				  
      ControlType = CONTROL_AUDIO;
      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType);
 

}


/*******************************************************************************
 *Function name: CHCA_MepgEnable
 *           
 *
 *Description: Open the video and audio
 *             
 *
 *Prototype:
 *      CHCA_BOOL  CHCA_MepgEnable(CHCA_BOOL  bVideoOpen,CHCA_BOOL  bAudioOpen)
 *           
 *input:
 *     none
 *
 *output:
 *           
 *Returned code:
 *    
 *           
 *Comments: 
 *        2005-3-12    add NewPMTChannel process becasue the video and audio pid has already been set   
 *                            when the program is switched
 *******************************************************************************/

#ifndef  NAGRA_PRODUCE_VERSION
CHCA_BOOL  CHCA_PairMepgEnable(CHCA_BOOL  bVideoOpen,CHCA_BOOL  bAudioOpen)
{
#ifdef CH_IPANEL_MGCA                                 /*AV控制*/
    return 0;
#else

     CHCA_BOOL                                               ReturnErr=false;
     ApplMode                                                   CAppMode; 	 

     CH6_VideoControl_Type                              VideoControl;
     CHCA_BOOL                                               AudioControl=true;
     CH6_AVControl_Type                                  ControlType;

     /*return ReturnErr; */

     if(CH_GetStandByStatus())
     {
             do_report(severity_info,"\n[CHCA_MepgEnable==>]The Stb box is on Standby status \n");
             return ReturnErr;  
     }

#if      /*PLATFORM_16 20070514 change*/1
#if 0 /*20050515 add*/
     if(OpenVedio!=true)
	  return 	ReturnErr;

     OpenVedio = false;	 
#endif
#endif

     CAppMode = CH_GetCurApplMode();  
#ifdef INTEGRATE_NVOD
     if(CAppMode == APP_MODE_NVOD)
     {
             if(Nvod_GetMenuStatus())					  	
	      {
                  switch(CH_GetNvodHideTag())
                  {
                       case NO_HIDE:
				    bVideoOpen = true;	
				    bAudioOpen = true;	
                                break;
                  
                       case HIDE_VIDEO:
                                bVideoOpen = false;	
                                bAudioOpen = true;	
                                break;
                  
                       case HIDE_AUDIO:
                                bVideoOpen = true;	
                                bAudioOpen = false;	
                                break;
                  
                       case HIDE_VIDEOANDAUDIO:
                                bVideoOpen = false;	
                                bAudioOpen = false;	
                                break;
                  }
	      }
     }
#endif
 
     switch(CAppMode)
     {
            case APP_MODE_MOSAIC:
            case APP_MODE_NVOD:
            case APP_MODE_LIST:
            case APP_MODE_EPG:
            case APP_MODE_PLAY:
                     if(Video_Track == CHCA_INVALID_TRACK)
                     {
                           if(bVideoOpen)
                              bVideoOpen = false;	
                     }
                     
                     if(Audio_Track == CHCA_INVALID_TRACK)
                     {
                           if(bAudioOpen)
                               bAudioOpen = false;	
                     }
				
			if(bVideoOpen && bAudioOpen)		
		       {
		              VideoControl = VIDEO_DISPLAY;
			       AudioControl = true;	
		              ControlType = CONTROL_VIDEOANDAUDIO;
		       }   
			else
			{
                            if(bVideoOpen)
                            {
                                   VideoControl = VIDEO_DISPLAY;
                                   AudioControl = false; 				  
                                   ControlType = CONTROL_VIDEO;
                            }
                            
                            if(bAudioOpen)	
                            { 
                                 VideoControl = VIDEO_SHOWPIC;
                                 AudioControl = true; 				  
                                 ControlType = CONTROL_AUDIO;
                            }
			}
		       ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
		       break;	

	     case APP_MODE_SET:		  
           /* case APP_MODE_STOCK:
            case APP_MODE_HTML:*/
		      break;		

            case APP_MODE_STOCK:
            case APP_MODE_HTML:
            case APP_MODE_GAME:
		      VideoControl = VIDEO_BLACK;
		       AudioControl = true; 	  
    		      ControlType = CONTROL_AUDIO;
  		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
		      break;		

     }

	 
     if(ReturnErr)
     {
           do_report(severity_info,"\n[CHCA_MepgEnable==>]Fail to open the AV \n");
     }

     return ReturnErr;
#endif	 
}
#endif


CHCA_BOOL  CHCA_MepgEnable(CHCA_BOOL  bVideoOpen,CHCA_BOOL  bAudioOpen)
{
#ifdef CH_IPANEL_MGCA                               /*AV控制*/
    return 0;
#else
     CHCA_BOOL                                               ReturnErr=false;
     ApplMode                                                   CAppMode; 	 

     CH6_VideoControl_Type                              VideoControl;
     CHCA_BOOL                                               AudioControl=true;
     CH6_AVControl_Type                                  ControlType;

     if(CH_GetStandByStatus())
     {
             do_report(severity_info,"\n[CHCA_MepgEnable==>]The Stb box is on Standby status \n");
             return ReturnErr;  
     }


/*add this on 050316*/
#ifndef     NAGRA_PRODUCE_VERSION   
          return ReturnErr;
#endif
/*add this on 050316*/

#if 1
     /*add this on 050312*/
     if(NewPMTChannel)
     {
            /*the video and audio pid has already been set*/
             NewPMTChannel = false;
             /*do_report(severity_info,"\n[CHCA_MepgEnable==>]New channel\n");*/
             /*return ReturnErr;  */
     }
     /*add this on 050312*/
#endif	 

     /*modify this on 050127*/
     if(!CardOperation.bCardReady)
     {
             do_report(severity_info,"\n[CHCA_MepgEnable==>]Card is not ready\n");
             return ReturnErr;  
     }


#ifdef AV_CONTROL
       return ReturnErr;
#endif

     CAppMode = CH_GetCurApplMode();  
#ifdef INTEGRATE_NVOD
     if(CAppMode == APP_MODE_NVOD)
     {
             if(Nvod_GetMenuStatus())					  	
	      {
                  switch(CH_GetNvodHideTag())
                  {
                       case NO_HIDE:
				    bVideoOpen = true;	
				    bAudioOpen = true;	
                                break;
                  
                       case HIDE_VIDEO:
                                bVideoOpen = false;	
                                bAudioOpen = true;	
                                break;
                  
                       case HIDE_AUDIO:
                                bVideoOpen = true;	
                                bAudioOpen = false;	
                                break;
                  
                       case HIDE_VIDEOANDAUDIO:
                                bVideoOpen = false;	
                                bAudioOpen = false;	
                                break;
                  }
	      }
     }
#endif
 
     switch(CAppMode)
     {
       
            case APP_MODE_NVOD:
            case APP_MODE_LIST:
            case APP_MODE_EPG:
            case APP_MODE_PLAY:
		#if 0 /*20061210 del*/
                     if(Video_Track == CHCA_INVALID_TRACK)
                     {
                           if(bVideoOpen)
                              bVideoOpen = false;	
                     }
                     
                     if(Audio_Track == CHCA_INVALID_TRACK)
                     {
                           if(bAudioOpen)
                               bAudioOpen = false;	
                     }
		#endif
				
			if(bVideoOpen && bAudioOpen)		
		       {
		              VideoControl = VIDEO_DISPLAY;
			       AudioControl = true;	
		              ControlType = CONTROL_VIDEOANDAUDIO;
		       }   
			else
			{
                            if(bVideoOpen)
                            {
                                   VideoControl = VIDEO_DISPLAY;
                                   AudioControl = false; 				  
                                   ControlType = CONTROL_VIDEO;
                            }
                            
                            if(bAudioOpen)	
                            { 
                                 VideoControl = VIDEO_SHOWPIC;
                                 AudioControl = true; 				  
                                 ControlType = CONTROL_AUDIO;
                            }
			}

				if(bVideoOpen != false || bAudioOpen != false)/*避免因为默认值打开视音频 wz 20071129 */
					{
		      			 ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	
					}
               /*20070813 add协调NVOD AV控制*/
			    if(CAppMode==APP_MODE_NVOD)
			   	{
                  CH_CANVOD_SetAVControl();
			   	}
			    /**********/
		       break;	
		case APP_MODE_MOSAIC:
				
			if(bVideoOpen && bAudioOpen)		
		       {
		              VideoControl = VIDEO_DISPLAY;
			       	  AudioControl = true;	
		              ControlType = CONTROL_VIDEOANDAUDIO;
		       }   
			else
			{
                            if(bVideoOpen)
                            {
                                   VideoControl = VIDEO_DISPLAY;
                                   AudioControl = false; 				  
                                   ControlType = CONTROL_VIDEO;
                            }
                            
                            if(bAudioOpen)	
                            { 
                                 VideoControl = VIDEO_SHOWPIC;
                                 AudioControl = true; 				  
                                 ControlType = CONTROL_AUDIO;
                            }
			}

				 /*20060823 change 控制当在MOSAIC,
                      从无配对的视频的流到有配对的视频流，
                      从无配对的音频流到有配对的音频流
                      */
               
                   	    AudioControl = true; 
                        if(ControlType==CONTROL_VIDEO)
                        	{
                             ControlType=CONTROL_VIDEOANDAUDIO;
                        	}	
                     
			      if(CH_GetCurMosaicPageType()==VEDIO_TYPE&&
				   ControlType==CONTROL_VIDEOANDAUDIO)
                    	{
				  			MakeVirtualkeypress(0xFE);
                    	}
					else
						{
						 ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	 
						}

			break;
	     case APP_MODE_SET:		  
 
		      break;		

           case APP_MODE_SEARCH:
            case APP_MODE_STOCK:
            case APP_MODE_GAME:
		      VideoControl = VIDEO_BLACK;
		       AudioControl = true; 	  
    		   ControlType = CONTROL_AUDIO;
  		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
		      break;		
#ifdef 	USE_IPANEL		  
		/*lzf 20060406 add below*/	  
             case APP_MODE_HTML:
			{
				if(eis_AV_play_state())
			      	      VideoControl = VIDEO_DISPLAY;
				else
				{
					VideoControl = VIDEO_FREEZE;
					eis_back_sound();
				}
				
			      AudioControl = true; 	  
	    		  ControlType = CONTROL_VIDEOANDAUDIO;
	  		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
			 }
		      break;	
			
#endif		  
		/*end lzf 20060406 add below*/	  	  
     }


	 
     if(ReturnErr)
     {
           do_report(severity_info,"\n[CHCA_MepgEnable==>]Fail to open the AV \n");
     }

	 
     return ReturnErr;
#endif	 
}



/*******************************************************************************
 *Function name: CHCA_MepgDisable
 *           
 *
 *Description: close the video and audio
 *             
 *
 *Prototype:
 *      CHCA_BOOL  CHCA_MepgDisable(CHCA_BOOL  bVideoClose,CHCA_BOOL  bAudioClose)
 *           
 *input:
 *     none
 *
 *output:
 *           
 *Returned code:
 *    
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/

#ifndef  NAGRA_PRODUCE_VERSION
CHCA_BOOL  CHCA_PairMepgDisable(CHCA_BOOL  bVideoClose,CHCA_BOOL  bAudioClose)
{
     CHCA_BOOL                                               ReturnErr = false;
     ApplMode                                                    CAppMode; 	 
       	 
     CH6_VideoControl_Type                              VideoControl;
     BOOL                                                         AudioControl;
     CH6_AVControl_Type                                  ControlType;	


     /*return ReturnErr; */

     CAppMode = CH_GetCurApplMode();   
    	 #ifdef AV_CONTROL
       return ReturnErr;
#endif
#if      /*PLATFORM_16 20070514 change*/1
#if  0 /*20050515 add*/
     OpenVedio=true;
#endif
#endif

      switch(CAppMode)
      {
            case APP_MODE_LIST:
            case APP_MODE_EPG:
            case APP_MODE_PLAY:
                    if(Video_Track == CHCA_INVALID_TRACK)
                    {
                          if(bVideoClose)
                             bVideoClose = false;	
                    }
                    
                    if(Audio_Track == CHCA_INVALID_TRACK)
                    {
                         if(bAudioClose)
                            bAudioClose = false;	
                    }
	
                     if(bVideoClose&&bAudioClose)	
                     {
                            VideoControl = VIDEO_SHOWPIC;
                            ControlType = CONTROL_VIDEOANDAUDIO;
                            AudioControl = false;
                     }
                     else
                     {
                            if(bVideoClose)
                            {
                                   VideoControl = VIDEO_SHOWPIC;
                                   ControlType = CONTROL_VIDEO;
				/*	AudioControl = false;	*/		   
                             }
                            
                            if(bAudioClose)
                            {
                                  VideoControl = VIDEO_FREEZE;
                                  AudioControl = false;	  
                                  ControlType = CONTROL_AUDIO;
                            }
                     }
		       ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 
		      break;	

	     case APP_MODE_SET:		  
            /*case APP_MODE_STOCK:
            case APP_MODE_HTML:*/
		      break;	
			
            case APP_MODE_NVOD:
		      VideoControl = VIDEO_CLOSE;
		      AudioControl = false;   
  		      ControlType = CONTROL_VIDEOANDAUDIO;
		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
		      break;	
			  
            case APP_MODE_MOSAIC:
		      if(!CardOperation.bCardReady)		
		      {
                         VideoControl = VIDEO_FREEZE;
                         AudioControl = false;	  
                         ControlType = CONTROL_VIDEOANDAUDIO;
                         ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	
		      }
		      else
		      {
                         VideoControl = VIDEO_FREEZE;
                         ControlType =CONTROL_VIDEOANDAUDIO 
				/*CONTROL_VIDEO 20060322 change for 在
                         MOSAIC中机卡不配对关闭音频和视屏*/;
                         ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	
		      }
		      break;		

            case APP_MODE_STOCK:
            case APP_MODE_HTML:
            case APP_MODE_GAME:
		      VideoControl = VIDEO_BLACK;
		      AudioControl = false;	  
    		      ControlType = CONTROL_AUDIO;
  		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
		      break;		

     }


     if(ReturnErr)
     {
           do_report(severity_info,"\n[CHCA_MepgDisable==>]Fail to close the AV \n");
     }

     return ReturnErr;
}
#endif


CHCA_BOOL  CHCA_MepgDisable(CHCA_BOOL  bVideoClose,CHCA_BOOL  bAudioClose)
{
     CHCA_BOOL                                               ReturnErr = false;
     ApplMode                                                    CAppMode; 	 
       	 
     CH6_VideoControl_Type                              VideoControl;
     BOOL                                                         AudioControl;
     CH6_AVControl_Type                                  ControlType;	

	
#ifdef REMOVE_BEIJING_HANDLE/*20060505 add*/
          return ReturnErr;
#endif

#ifndef     NAGRA_PRODUCE_VERSION   
          return ReturnErr;
#endif

	 
      CAppMode = CH_GetCurApplMode();   
#ifdef AV_CONTROL
       return ReturnErr;
#endif

	if(pstBoxInfo->abiBoxPoweredState == false)
		return ReturnErr;/* cqj 20071126 add */


      switch(CAppMode)
      {
            case APP_MODE_LIST:
            case APP_MODE_EPG:
            case APP_MODE_PLAY:
                    if(Video_Track == CHCA_INVALID_TRACK)
                    {
                          if(bVideoClose)
                             bVideoClose = false;	
                    }
                    
                    if(Audio_Track == CHCA_INVALID_TRACK)
                    {
                         if(bAudioClose)
                            bAudioClose = false;	
                    }
	
                     if(bVideoClose&&bAudioClose)	
                     {
                            VideoControl = VIDEO_SHOWPIC;
                            ControlType = CONTROL_VIDEOANDAUDIO;
                            AudioControl = false;
                     }
                     else
                     {
                            if(bVideoClose)
                            {
                                   VideoControl = VIDEO_SHOWPIC;/*WZ change VIDEO_BLACK to VIDEO_SHOWPIC*/
                                   ControlType = CONTROL_VIDEO;
									/*	AudioControl = false;	*/	   
                             }
                            
                            if(bAudioClose)
                            {
                                  VideoControl = VIDEO_FREEZE;
                                  AudioControl = false;	  
                                  ControlType = CONTROL_AUDIO;
                            }
                     }
					
		       if(bVideoClose||bAudioClose)/*20061210 add*/
				{
		       ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 
				}
		      break;	

	     case APP_MODE_SET:		  
         
		      break;	
			
            case APP_MODE_NVOD:
		      VideoControl = VIDEO_CLOSE;
		      AudioControl = false;   
  		      ControlType = CONTROL_VIDEOANDAUDIO;
		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
 
		      break;	
			  
            case APP_MODE_MOSAIC:

		      if(!CardOperation.bCardReady)		
		      {
                         VideoControl = VIDEO_FREEZE;
                         AudioControl = false;	  
                         ControlType = CONTROL_VIDEOANDAUDIO;
                         ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	
		      }
		      else
		      {
		      #if 0/*20060404 change*/
                         VideoControl = VIDEO_FREEZE;
                         ControlType = CONTROL_VIDEO;
			#else
                     VideoControl = VIDEO_FREEZE;
                         AudioControl = false;	  
                         ControlType = CONTROL_VIDEOANDAUDIO;
			#endif
                         ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	
		      }
			  #ifdef BEIJING_MOSAIC
       /*20060823 add 解决当从MOSAIC中的配对视频
                    流切换到MOSAIC中的不配对视频流静桢，
                    该为显示MOSAIC视频切换的I FRAM*/
                    if(CH_GetCurMosaicPageType()==VEDIO_TYPE)
                    	{
                               /*CH6_DrawRadio();*/
				  MakeVirtualkeypress(0xFF);
                    	}
                   /******************************************/
             #endif
		      break;		

            case APP_MODE_STOCK:
            case APP_MODE_HTML:
            case APP_MODE_GAME:
#ifdef USE_IPANEL
				
		/*wz 20070123 add */
		  if(eis_AV_play_state())
		    {
		        VideoControl = VIDEO_BLACK;
	             AudioControl = false;	  
		      	ControlType = CONTROL_VIDEOANDAUDIO;
		     }
		 else
#endif
		    {
	             VideoControl = VIDEO_BLACK;
	             AudioControl = false;	  
		      ControlType = CONTROL_AUDIO;
		     }
  		      ReturnErr =  CH6_AVControl(VideoControl,AudioControl,ControlType); 	  
		      break;		

     }

  	 	

     if(ReturnErr)
     {
           do_report(severity_info,"\n[CHCA_MepgDisable==>]Fail to close the AV \n");
     }

     return ReturnErr;
}



/*******************************************************************************
 *Function name: CHCA_GetPairStatus
 * 
 *
 *Description: get the pairing status of the card
 *                  
 *
 *Prototype:
 *        TCHCAPairingStatus  CHCA_GetPairStatus(void)     
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *      CHCA_UnknownStatus  init status:    
 *      CHCA_NoPair : no pair action 
 *      CHCA_PairError :  fail to pair 
 *      CHCA_PairOK : success to pair
           
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/

#ifdef   PAIR_CHECK/*050707 xjp change for pair display*/
CHCA_PairingStatus_t  CHCA_GetPairStatus(void)
{
	GetPairInfo();
	if(pastPairInfo.PairStatus2==true)
		return CHCAPairOK;
	else
		return  CHCAPairError;  
}
#else
CHCA_PairingStatus_t  CHCA_GetPairStatus(void)
{
	return  (CHCA_PairingStatus_t)CardPairStatus;  
}
#endif

/*******************************************************************************
 *Function name: CHCA_CheckGeoRightStatus
 * 
 *
 *Description: check the geo right status of  two track of the television program
 *                  
 *
 *Prototype:
 *        void   CHCA_CheckGeoRightStatus(void)   
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
void   CHCA_CheckGeoRightStatus(void)
{
        STBStaus                   CurSTBStaus;

        if((stTrackTableInfo.ProgElementCurRight[0]==2)||(stTrackTableInfo.ProgElementCurRight[1]==2))  
        {
                 if(GeoInfoStatus == -1)
                 {
                       GeoInfoStatus=0;
                       CurSTBStaus=Geo_Blackout; 
	#ifndef  CH_IPANEL_MGCA    	                  
					   
                       CHCA_CardSendMess2Usif(CurSTBStaus);   
                       CHCA_MepgDisable(true,true); 
	#else
                      CHCA_CardSendMess2Usif(CH_CA2IPANEL_BLACKOUT,NULL,0);
	#endif
                 }	
				 
		   return;		 
	 }
}


/*******************************************************************************
 *Function name: CHCA_CheckAVRightStatus
 * 
 *
 *Description: check the right status of  two track of the television program
 *                  
 *
 *Prototype:
 *        void   CHCA_CheckAVRightStatus(void)   
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *
 *Comments:
 *     2005-03-12          add 'Parental_Control' process
 * 
 * 
 *******************************************************************************/
void   CHCA_CheckAVRightStatus(void)
{

   
	#ifndef    NAGRA_PRODUCE_VERSION  		  
     /*add this on 050313*/
	 if(EcmFilterNum==2)	
	 {
            if(stTrackTableInfo.ProgElementCurRight[0]==-1 || stTrackTableInfo.ProgElementCurRight[1]==-1)
            {
#ifdef       CHPROG_DRIVER_DEBUG
                 do_report(severity_info,"\n[CHCA_CheckAVRightStatus==>]Wait the second ecm data\n");
#endif
                 return;
            }
	 }
	  if((stTrackTableInfo.ProgElementCurRight[0]==0)&&(stTrackTableInfo.ProgElementCurRight[1]==0))
	 {

                       CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);
	 }
       #endif	

	#ifndef  CH_IPANEL_MGCA    	                  

        STBStaus                                CurSTBStaus;

#ifdef   CHPROG_DRIVER_DEBUG
	 do_report(severity_info,"\n[CHCA_CheckAVRightStatus==>] Video[%d] Audio[%d]\n",\
	 	stTrackTableInfo.ProgElementCurRight[0],
	 	stTrackTableInfo.ProgElementCurRight[1]);	
#endif

        /*add this on 050313*/
	 if(EcmFilterNum==2)	
	 {
            if(stTrackTableInfo.ProgElementCurRight[0]==-1 || stTrackTableInfo.ProgElementCurRight[1]==-1)
            {
#ifdef       CHPROG_DRIVER_DEBUG
                 do_report(severity_info,"\n[CHCA_CheckAVRightStatus==>]Wait the second ecm data\n");
#endif
                 return;
            }
	 }
        /*add this on 050313*/

        if((stTrackTableInfo.ProgElementCurRight[0]==2)||(stTrackTableInfo.ProgElementCurRight[1]==2))  
        {
                 CurSTBStaus = Geo_Blackout; 
	#ifndef  CH_IPANEL_MGCA    	                  
				 
                 CHCA_CardSendMess2Usif(CurSTBStaus);   
                 CHCA_MepgDisable(true,true); 
	
	#endif
		   return;		 
	 }
	 
        /*add this on 050312*/
        if((stTrackTableInfo.ProgElementCurRight[0]==3)||(stTrackTableInfo.ProgElementCurRight[1]==3))  
        {
                 CurSTBStaus = Parental_Control; 
	#ifndef  CH_IPANEL_MGCA    	                  
				 
                 CHCA_CardSendMess2Usif(CurSTBStaus);   
                 CHCA_MepgDisable(true,true); 
	#endif			 
		   return;		 
	 }
        /*add this on 050312*/
	 
        if((stTrackTableInfo.ProgElementCurRight[0]==1)&&(stTrackTableInfo.ProgElementCurRight[1]==1))  
        {
                CurSTBStaus = NORMAL_STATUS;
	#ifndef  CH_IPANEL_MGCA    	                  
				
                CHCA_CardSendMess2Usif(CurSTBStaus);   
                CHCA_MepgEnable(true,true); 
	#endif			
	 }
	 else if((stTrackTableInfo.ProgElementCurRight[0]==0)&&(stTrackTableInfo.ProgElementCurRight[1]==0))
	 {
                 CurSTBStaus = NO_RIGHT; 
		#ifndef  CH_IPANEL_MGCA    	                  
			 
                 CHCA_CardSendMess2Usif(CurSTBStaus);   
                 CHCA_MepgDisable(true,true); 

			
		#endif

	#ifndef    NAGRA_PRODUCE_VERSION  		  
                       CHCA_CardSendMess2Usif(CH_CA2IPANEL_NO_Right,NULL,0);
       #endif	
	 }
	 else
	 {
              if(stTrackTableInfo.ProgElementCurRight[0]==1)
              {
                     CurSTBStaus = NO_AUDIO_RIGHT;
		#ifndef  CH_IPANEL_MGCA    	                  
				 
			CHCA_MepgEnable(true,false); 
		       CHCA_MepgDisable(false,true); 
			CHCA_CardSendMess2Usif(CurSTBStaus);   
		#endif	
		}
		else
		{
                     CurSTBStaus = NO_VEDIO_RIGHT;
		#ifndef  CH_IPANEL_MGCA    	                  
					 
			CHCA_MepgEnable(false,true); 
			CHCA_MepgDisable(true,true); 
			CHCA_CardSendMess2Usif(CurSTBStaus);   
		#endif	
		}
	 }
#endif
}


/*******************************************************************************
 *Function name: CHCA_GetAVRightStatus
 * 
 *
 *Description: get  the right status of  two track of the television program
 *                  
 *
 *Prototype:
 *        void   CHCA_GetAVRightStatus(void)   
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL   CHCA_GetAVRightStatus(void)
{
      if((stTrackTableInfo.ProgElementCurRight[0]!=-1)&&(stTrackTableInfo.ProgElementCurRight[1]!=-1))
      {

	       do_report(severity_info,"\n[CHCA_GetAVRightStatus==>]A[%d]V[%d]\n",stTrackTableInfo.ProgElementCurRight[1],stTrackTableInfo.ProgElementCurRight[0]);
              return true;
      }
      else
      {
      	       /*do_report(severity_info,"\n[CHCA_GetAVRightStatus==>]A[%d]V[%d]\n",stTrackTableInfo.ProgElementCurRight[1],stTrackTableInfo.ProgElementCurRight[0]);*/
              return false;
      }
}

/*******************************************************************************
 *Function name: CHCA_StoreMulAudPids
 * 
 *
 *Description: store the multi audo pids
 *                  
 *
 *Prototype:
 *        CHCA_BOOL   CHCA_StoreMulAudPids (CHCA_UCHAR *aucSectionData );
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL   CHCA_StoreMulAudPids (CHCA_UCHAR *aucSectionData )
{
       CHCA_BOOL      bReturnCode=false;
#ifndef CH_IPANEL_MGCA
	bReturnCode =  CH_StoreMulAudPids(aucSectionData);
#endif
	return bReturnCode;

}


/*******************************************************************************
 *Function name: CHCA_GetsCurProgramId
 * 
 *
 *Description: get current program id
 *                  
 *
 *Prototype:
 *        CHCA_SHORT CHCA_GetsCurProgramId(void)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *         the current program id
 *
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_SHORT CHCA_GetsCurProgramId(void)
{
      CHCA_SHORT   TempProgId=CHCA_INVALID_LINK;

      TempProgId = 	CH_GetsCurProgramId(); 

      return TempProgId;	  

}

/*******************************************************************************
 *Function name: CHCA_GetsCurTransponderId
 * 
 *
 *Description: get current transponder id
 *                  
 *
 *Prototype:
 *        CHCA_SHORT CHCA_GetsCurTransponderId(void)
 *
 *input:
 *      none
 * 
 *
 *output:
 *
 *Return Value:
 *         the current program id
 *
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_SHORT CHCA_GetsCurTransponderId(void)
{
       CHCA_SHORT     TempTransId=CHCA_INVALID_LINK;

       TempTransId= CH_GetsCurTransponderId();
	   
       return TempTransId;	  
}

/*20050320 add for frell all pstCHCAPMTMsgQueue*/
void CH_FreeAllPMTMsg(void)
{

      chca_mg_cmd_t *msg_p=NULL;	
      int Count=0;
      clock_t                                 cMgcaTime;

	  
	while (TRUE&&Count<=15)
	{
		/*cMgcaTime = ST_GetClocksPerSecondLow();*/
              /*cMgcaTime = time_plus(time_now(), ST_GetClocksPerSecond()); */
		
		msg_p = ( chca_mg_cmd_t * )message_receive_timeout(pstCHCAPMTMsgQueue, TIMEOUT_IMMEDIATE);
	       /*do_report(severity_info,"\n[CH_FreeAllPMTMsg==>]MessageCount[%d] msg_p[%d]\n",Count,msg_p);*/
		
		if(msg_p!=NULL)
		{
		       Count++ ;
			message_release(pstCHCAPMTMsgQueue, msg_p);
		}
		else
		{
			break;
		}
	}

	/*do_report(severity_info,"\n[CH_FreeAllPMTMsg==>]MessageCount[%d] msg_p[%d]\n",Count,msg_p);*/
}


void CH_FreeAllECMMsg(void)
{
      chca_mg_cmd_t                    *msg_p;	
      int                                        Count=0;
      /*clock_t                                 cMgcaTime;*/
      
      /*StopEcmFilterIndex();*/
	  
	while (TRUE&&Count<=15)
	{
	       /*cMgcaTime = ST_GetClocksPerSecondLow();*/
		msg_p = ( chca_mg_cmd_t * )message_receive_timeout(pstCHCAECMMsgQueue, TIMEOUT_IMMEDIATE);
		/*do_report(severity_info,"\n[CH_FreeAllECMMsg==>]MessageCount[%d] msg_p[%d]\n",Count,msg_p);*/

		if(msg_p!=NULL)
		{
		       Count++ ;
			message_release(pstCHCAECMMsgQueue, msg_p);
		}
		else
		{
			break;
		}
	}

	/*do_report(severity_info,"\n[CH_FreeAllECMMsg==>]MessageCount[%d] msg_p[%d]\n",Count,msg_p);*/
}


void CH_FreeAllCATMsg(void)
{

      chca_mg_cmd_t *msg_p;	
      int Count=0;
	while (TRUE&&Count>=30)
	{
		msg_p = message_receive_timeout(pstCHCACATMsgQueue, TIMEOUT_IMMEDIATE);
		if(msg_p!=NULL)
		{
		       Count++ ;
			message_release(pstCHCACATMsgQueue, msg_p);
		}
		else
		{
			break;
		}
	}
}
/*20070813 add在NVOD 状态下切换节目时候CA是否已经控制AV*/
extern semaphore_t *pApplSema;
static boolean g_canvod_AVControl=true;
/*函数名:    void CH_CANVOD_SetAVControl(void)
  *开发人员:zengxianggen
  *开发时间:2007/08/13
  *函数功能:设置在NVOD 状态下切换节目时候CA已经控制AV
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
void CH_CANVOD_SetAVControl(void)
{
  if(pApplSema!=NULL)
  	{
	 semaphore_wait(pApplSema);
     g_canvod_AVControl=true;
     semaphore_signal(pApplSema);
  	}

}
/*函数名:    void CH_CANVOD_ClearAVControl(void)
  *开发人员:zengxianggen
  *开发时间:2007/08/13
  *函数功能:设置在NVOD 状态下切换节目时候CA没有控制AV
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
void CH_CANVOD_ClearAVControl(void)
{
  if(pApplSema!=NULL)
  	{
	 semaphore_wait(pApplSema);
     g_canvod_AVControl=false;
	 semaphore_signal(pApplSema);
  	}
}
/*函数名:  boolean CH_CANVOD_IsAVControl(void)
  *开发人员:zengxianggen
  *开发时间:2007/08/13
  *函数功能:得到NVOD 状态下切换节目时候CA是否已经控制AV
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:TRUE,已经控制，FALSE，没有控制
  *参数表:无                                           */
boolean CH_CANVOD_IsAVControl(void)
{
  boolean b_result=false;
  if(pApplSema!=NULL)
  {
   semaphore_wait(pApplSema);
   b_result=g_canvod_AVControl;
   semaphore_signal(pApplSema);
  }
   return b_result;
}

/*20080313 add 释放PMT的操作*/
/*函数名:  void CH_FreeCAPmt(void)
  *开发人员:zengxianggen
  *开发时间:2008/03/13
  *函数功能:
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:
  *参数表:无                                           */
 void CH_FreeCAPmt(void)
{
if(iFilterId_requested !=-1)
{
 if(ChPmtProcessStop(iFilterId_requested)==true)
 {
       //do_report(severity_info,"\n[ChDVBPMTMonitor==>]fail to stop the pmt process\n");/**/
 }
else
	iFilterId_requested=-1;
}
}

