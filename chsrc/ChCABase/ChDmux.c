#ifdef    TRACE_CAINFO_ENABLE
#define  CHDEMUX_DRIVER_DEBUG
#endif
/*#define   PMT_TEST*/
/******************************************************************************
*
* File : ChDemux.c
*
* Description : 
*
*
* NOTES :
*
*
* Author : 
*
* Status :
*
* History : 0.0    2004-6-25             Start coding
*                       2004-09-06           all mg ca data are received in one process(MgCAProcess)
*                                                    all mg ca data are process in one process(MgCASectionMonitor)
                           2006-3       zxg  change for STi7710        
                           2007-07     zxg  change for STI7100
                           2007-08     zxg  单独一个进程处理CAT和CAT超时处理
* opyright: Changhong 2004 (c)
*
*****************************************************************************/
/*#include "ChDmux.h"*/
#include          "ChProg.h"

#define SECURITY_KEY

#if 0
#include          "..\mgddi\mgsys.h"
#endif

#if 1  /*add this on 050421*/
extern  CHCA_BOOL             EcmDataStop;
#endif


#if 1 /*add this on 050326*/
Descram_Channel_Struct                                           DescramChannelInfo[CHCA_MAX_DSCR_CHANNEL];  
BYTE                                                                        Invalid_Odd_key[ 8 ] = { 0,0,0,0,0,0,0,0 };
BYTE                                                                        Invalid_Even_key[ 8 ] = { 0,0,0,0,0,0,0,0 };
#endif

#if      /*PLATFORM_16 20070514 change*/1
CHCA_PTI_Signal_t                                                   MgCASignalHandle;
CHCA_PTI_Signal_t                                                   MgEMMSignalHandle; /*add this on 050115*/
/*CHCA_PTI_Signal_t                                                   MgPMTSignalHandle; *//*add this on 050424*/
CHCA_PTI_Signal_t                                                   MgCATSignalHandle; /*add this on 050425*/
CHCA_PTI_Signal_t                                                   CHCAPTIHandle; /*add this on 041028*/
#endif

semaphore_t                                                            *psemMgDescAccess=NULL;
semaphore_t                                                            *pSemMgApiAccess=NULL;                                               

DDI_FilterInfo_t                                                          FDmxFlterInfo[CHCA_MAX_NUM_FILTER];


/*ca data filter monitor task */
task_t                                                                      *ptidCASecFilterMonitorTask;/* 060118 xjp change from *ptidCASecFilterMonitorTask to ptidCASecFilterMonitorTask  for adopt task_init()*/
const int                                                                   CASECFILTER_PROCESS_WORKSPACE = 1024*15;
const int                                                                   CASECFILTER_PROCESS_PRIORITY =  9; /*from  11 to  9 on 060119*/
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_CASecFilterStack;
	static tdesc_t g_CASecFilterTaskDesc;
#endif
/*ca data filter monitor task */

#if 1/*060117 xjp comment CATSecFilterMonitor task*/
/*cat data filter monitor task */
task_t                                                                      *ptidCATSecFilterMonitorTask;
const int                                                                   CATSECFILTER_PROCESS_WORKSPACE = 1024*15;
const int                                                                   CATSECFILTER_PROCESS_PRIORITY =  8; /*from 9 to 11 on 050319*/
/*cat data filter monitor task */

static void* g_CATSecFilterStack;
static tdesc_t g_CATSecFilterTaskDesc;

#endif

#if 0
/*pmt data filter monitor task */
task_t                                                                      *ptidPMTSecFilterMonitorTask;
const int                                                                   PMTSECFILTER_PROCESS_WORKSPACE = 1024*15;
const int                                                                   PMTSECFILTER_PROCESS_PRIORITY =  7; /*from 9 to 11 on 050319*/
/*pmt data filter monitor task */
#endif


/*add this on 050115*/
/*emm data filter monitor task */
task_t                                                                      *ptidEMMSecFilterMonitorTask;/* 060118 xjp change from *ptidEMMSecFilterMonitorTask to ptidEMMSecFilterMonitorTask  for adopt task_init()*/
const int                                                                   EMMSECFILTER_PROCESS_WORKSPACE = 1024*20;/*from 1024*10 to 1024*20   on 060323*/
const int                                                                   EMMSECFILTER_PROCESS_PRIORITY =  9; /*from 8 to 9  on 060323*/
#if 1/*060117 xjp add for task_create() change into task_init()*/
	static void* g_EMMSecFilterStack;
	static tdesc_t g_EMMSecFilterTaskDesc;
#endif
/*emm data filter monitor task */



CHCA_UCHAR                                                           PmtSectionBuffer[1024];
CHCA_UCHAR                                                           CatSectionBuffer[1024];
/*CHCA_UCHAR                                                           EcmSectionBuffer[MGCA_PRIVATE_TABLE_MAXSIZE];delete this on 050327*/

QUEUE_EMM                                                              QEmmData;
MG_FILTER_STRUCT                                                  MgEmmFilterData;

semaphore_t                                                              *psemSectionEmmQueueAccess = NULL;   /*add this on 041129*/
MgQueue                                                                   EmmQueue;


DDI_DscrCh_t                                                            DscrChannel[CHCA_MAX_DSCR_CHANNEL];

CHCA_CHAR   PmtVesion=-1; /*060119 xjp add for not equal mode is invalid*/

/*extern CHCA_SHORT                                                   CatFilterId;  delete this on 050311*/  
/*extern CHCA_SHORT                                                   EcmFilterId;  delete this on 050311*/  

/*CHCA_SHORT                                                             EcmFilterIdTemp;   delete this on 050311*/  
/*CHCA_UCHAR                                                             EcmTableTemp;	delete this on 050311*/
/*add this on 041129*/

#if  0 /*add this on 050306*/
void   CHCA_CADataLock(void)
{
        semaphore_wait(pSemMgApiAccess);
}

void   CHCA_CADataUnLock(void)
{
        semaphore_signal(pSemMgApiAccess);

}
#endif




/*******************************************************************************
 *Function name:  ChEcmFilterCheck(void)
 * 
 *
 *Description: check the ecm filter count
 *                  
 *
 *Prototype: CHCA_UINT  ChEcmFilterCheck( void)
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
 *Return Value:
 *     
 *     
 *
 *Comments:
 *     2005-3-13
 * 
 * 
 *******************************************************************************/
CHCA_UINT  ChEcmFilterCheck( void)
{
      CHCA_UINT      FilterNum=0;
      CHCA_UINT      i; 	  

      for(i=0;i<CHCA_MAX_NUM_FILTER;i++)	
      {
            if(FDmxFlterInfo [ i ].FilterType==ECM_FILTER)
            {
                 FilterNum=FilterNum+1;
	     }
      }
	  
      do_report(severity_info,"\n[ChEcmFilterCheck==>]The ECMFilterNum=%d\n",FilterNum); 
	  
      return FilterNum;	   
}




#if  1/*delete this on 050327*/
/*******************************************************************************
 *Function name:  ChSectionSend2Ecm
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChSectionSend2Ecm( CHCA_USHORT          iFilterId,
 *                                                                      CHCA_MGData_t        bTableId,
 *                                                                      CHCA_BOOL              bTableType,
 *                                                                      TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
 *                                                                     TMGDDIEventCode  EvCode,
 *                                                                      CHCA_BOOL       return_status)
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
CHCA_BOOL  ChSectionSend2Ecm( CHCA_USHORT                                       iFilterId,
                                                                CHCA_MGData_t                                    bTableId,
                                                                CHCA_BOOL                                          bTableType,
                                                                TMGDDIEvSrcDmxFltrTimeoutExCode      ExCode,
                                                                TMGDDIEventCode                                EvCode,
                                                                CHCA_BOOL                                         return_status)
{
       chca_mg_cmd_t                     *msg_p=NULL;
       clock_t                                 cMgcaTime;
       message_queue_t                 *pstQueueId;
	 
	/*if(iFilterId<0 || iFilterId>=CHCA_MAX_NUM_FILTER)		
	{
             return true;
	}*/

#if  0 /*add this on 050501*/
       if(EcmDataStop)
	    return false;	
#endif	   

       if( pstCHCAECMMsgQueue != NULL)
       {
       #if 0/*20060323 change*/
              cMgcaTime = ST_GetClocksPerSecondLow();
	   #else
                cMgcaTime = time_plus(time_now(),ST_GetClocksPerSecond()*1);

	   #endif
              msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAECMMsgQueue, &cMgcaTime );

              if(  msg_p != NULL)
              {
                      /*SectionFilter[iFilterId].bBufferLock = return_status;*/
                      msg_p ->contents.sDmxfilter.TableId   = ECM_DATA;  
                      msg_p ->contents.sDmxfilter.iSectionFilterId   = iFilterId;
                      msg_p ->contents.sDmxfilter.TableType   = bTableType;
                      msg_p ->contents.sDmxfilter.return_status   = ExCode;
                      msg_p ->contents.sDmxfilter.TmgEventCode   = EvCode;
                      msg_p ->from_which_module = CHCA_SECTION_MODULE;
					  
                      message_send ( pstCHCAECMMsgQueue , msg_p );
			 return	false;
              }
		else
		{
		      do_report(severity_info,"\n[ChSectionSend2Ecm==>] fail to send the ecm table fitlerid [%d] \n",iFilterId);	
		}
       }
	else
 	{
               do_report(severity_info,"\n[ChSectionSend2Ecm==>] pstCHCAECMMsgQueue is null \n");			   
	}
	   
       return true;
	   
}
#endif


/*******************************************************************************
 *Function name:  ChSMailFromMGFilter2EMM
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *          CHCA_BOOL  ChSMailFromMGFilter2EMM( CHCA_INT  iSectionFilter)
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
 #if 1
 #if 0 /*add this on 050329*/
CHCA_BOOL  ChSMailFromMGFilter2EMM( CHCA_INT  iSectionFilter)
{
        chca_mg_cmd_t                            *msg_p;
        clock_t                                         timeout;

        /*if(pstCHCAEMMMsgQueue != NULL)*/
        if(pstCHCAPMTMsgQueue != NULL)
        {
                timeout = ST_GetClocksPerSecondLow();
                /*msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAEMMMsgQueue, &timeout );*/
                msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAPMTMsgQueue, &timeout );

	         /*do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>] msg_p[%4x]\n",msg_p);*/
                if(  msg_p != NULL)
                {
                       /*MgEmmFilterData.OnUse = true;*/
                       msg_p ->from_which_module                = CHCA_SECTION_MODULE; 
                       msg_p ->contents.sDmxfilter.TableId   = EMM_DATA;  /*add this on 040828*/
                       msg_p ->contents.sDmxfilter.iSectionFilterId   =  iSectionFilter;
                       msg_p ->contents.sDmxfilter.TableType   = 0;
                       msg_p ->contents.sDmxfilter.return_status   = 0;
                       msg_p ->contents.sDmxfilter.TmgEventCode   = MGDDIEvSrcDmxFltrReport;
                       /*message_send ( pstCHCAEMMMsgQueue,msg_p );*/
                       message_send ( pstCHCAPMTMsgQueue,msg_p );
                       /*do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>]successfully send the emm data,msg_p[%4x]\n",msg_p);*/
              	  return  false;
                }
		  else
		  {
		        do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>] fail to send the emm table fitlerid [%d] \n",iSectionFilter);	
		  }
        }
		
        return true;
		
}
 #else
 CHCA_BOOL  ChSMailFromMGFilter2EMM( CHCA_INT  iSectionFilter)
{
        chca_mg_cmd_t                            *msg_p;
        clock_t                                         timeout;

        if(pstCHCAEMMMsgQueue != NULL)
        /*if(pstCHCACATMsgQueue != NULL)*/
        {
        #if 0
                timeout = ST_GetClocksPerSecondLow();
	#else
                timeout = time_plus(time_now(),ST_GetClocksPerSecond()*1);

	#endif
                msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAEMMMsgQueue, &timeout );
                /*(msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCACATMsgQueue, &timeout );*/

	         /*do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>] msg_p[%4x]\n",msg_p);*/
                if(  msg_p != NULL)
                {
                       /*MgEmmFilterData.OnUse = true;*/
                       msg_p ->from_which_module                = CHCA_SECTION_MODULE; 
                       msg_p ->contents.sDmxfilter.TableId   = EMM_DATA;  /*add this on 040828*/
                       msg_p ->contents.sDmxfilter.iSectionFilterId   =  iSectionFilter;
                       msg_p ->contents.sDmxfilter.TableType   = 0;
                       msg_p ->contents.sDmxfilter.return_status   = 0;
                       msg_p ->contents.sDmxfilter.TmgEventCode   = MGDDIEvSrcDmxFltrReport;
                       message_send ( pstCHCAEMMMsgQueue,msg_p );
                       /*message_send ( pstCHCACATMsgQueue,msg_p );*/
                       /*do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>]successfully send the emm data,msg_p[%4x]\n",msg_p);*/
              	  return  false;
                }
		  else
		  {
		      do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>] fail to send the emm table fitlerid [%d] \n",iSectionFilter);	
		  }
        }
		
        return true;
		
}
 #endif
 #else
CHCA_BOOL  ChSMailFromMGFilter2EMM( CHCA_INT  iSectionFilter)
{
        chca_mg_cmd_t                            *msg_p;
        clock_t                                         timeout;

        if(pstCHCACATMsgQueue != NULL)
        {
                timeout = ST_GetClocksPerSecondLow();
                msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCACATMsgQueue, & timeout );
    
                if(  msg_p != NULL)
                {
                       MgEmmFilterData.OnUse = true;
                       msg_p ->from_which_module                = CHCA_SECTION_MODULE; 
                       msg_p ->contents.sDmxfilter.TableId   = EMM_DATA;  /*add this on 040828*/
                       msg_p ->contents.sDmxfilter.iSectionFilterId   =  iSectionFilter;
                       msg_p ->contents.sDmxfilter.TableType   = 0;
                       msg_p ->contents.sDmxfilter.return_status   = 0;
                       msg_p ->contents.sDmxfilter.TmgEventCode   = MGDDIEvSrcDmxFltrReport;
                       message_send ( pstCHCACATMsgQueue,msg_p );
                       /*do_report(severity_info,"\n[ChSMailFromMGFilter2EMM==>]successfully send the emm data\n");*/
              	  return  FALSE;
                }
        }
		
        return TRUE;
		
}
#endif

/*******************************************************************************
 *Function name:  ChMgQueueLength
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *          CHCA_UINT   ChMgQueueLength(MgQueue * Q)
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
CHCA_UINT   ChMgQueueLength(MgQueue * Q)
{
       /*return the element num of the Queue*/
       CHCA_UINT   Qlen=0;
   
       if(Q != NULL)
       {
             Qlen = (Q -> sRear - Q -> sFront + MGCA_MAX_NO_EMMBUFF) % MGCA_MAX_NO_EMMBUFF;
       }		
       else
       {
      	      do_report(severity_info,"\n The EMM Queue is error \n");
       }

    	return Qlen;
}


/*******************************************************************************
 *Function name:  ChSendEMMData
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 * CHCA_BOOL   ChSendEMMData(CHCA_INT  slotFilter,CHCA_UCHAR * aucInputBuffer,MG_FILTER_STRUCT * aucOutputBuff)
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
CHCA_BOOL   ChSendEMMData(CHCA_INT  slotFilter,CHCA_UCHAR * aucInputBuffer,MG_FILTER_STRUCT * aucOutputBuff)
{
         CHCA_INT      iSectionLength;

         iSectionLength = ((aucInputBuffer[1] << 8 ) | aucInputBuffer[2]) + 3;
         
         if(iSectionLength > MGCA_PRIVATE_TABLE_MAXSIZE)
         {
                 do_report(severity_info,"\n [ChSendEMMData==>]iSectionLength[%d] is error \n",iSectionLength);
                 return  true;
         }
         
         if(slotFilter == -1)
         {
                do_report(severity_info,"\n [ChSendEMMData==>]slotFilter [%d] is error\n",slotFilter);
                return true;
         }
         
         
         if ( aucOutputBuff != NULL )
         memcpy ( ( BYTE *)aucOutputBuff ->aucMgSectionData,
                          ( BYTE *) aucInputBuffer,
                          iSectionLength );
         
         aucOutputBuff->iSectionSlotId = slotFilter;

	  MgEmmFilterData.OnUse = true;	 
         ChSMailFromMGFilter2EMM(slotFilter);
         
         return false;

}





/*******************************************************************************
 *Function name:  ChEnMgBuffer
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChEnMgBuffer(CHCA_INT slotFilter,CHCA_UCHAR * aucInputBuffer,MgQueue * Q)
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
CHCA_BOOL  ChEnMgBuffer(CHCA_INT slotFilter,CHCA_UCHAR * aucInputBuffer,MgQueue * Q)
{
         CHCA_INT     iSectionLength;
         
         if(Q == NULL)
         {
                do_report(severity_info,"\n[ChEnMgBuffer==>] The EMM Queue is NULL \n");
                return true;
         }
         
         /*insert new element as the tailer element of the queue*/
         if(((Q -> sRear + 1) % MGCA_MAX_NO_EMMBUFF) == Q -> sFront)  /*the queue is full*/
         {
                do_report(severity_info,"\n the mg queue is full \n");
                return true;
         } 
         
         iSectionLength = ((aucInputBuffer[1] << 8 ) | aucInputBuffer[2]) + 3;
         
         if(iSectionLength > MGCA_PRIVATE_TABLE_MAXSIZE)
         {
                do_report(severity_info,"\n iSectionLength[%d] is error \n",iSectionLength);
                return  true;
         }
         
         /*do_report(severity_info,"\n iSectionLength[%d] %d, %d\n",iSectionLength,aucInputBuffer[iSectionLength-2],aucInputBuffer[iSectionLength-1]);*/
         
         if(slotFilter == -1)
         {
                 do_report(severity_info,"\n slotFilter [%d] is error\n",slotFilter);
                 return true;
         }
         
         memcpy ( ( BYTE *)Q -> data[Q -> sRear].buff ,          /*insert the new data to tailer of the queue*/
         ( BYTE *) aucInputBuffer,
         /*ONE_KILO_SECTION_MAX_LENGTH*/iSectionLength);
         
         
         Q -> data[Q -> sRear].iSectionSlotId = slotFilter;
         Q -> sRear = (Q -> sRear + 1) % MGCA_MAX_NO_EMMBUFF;       
         
         return false;

}


/*******************************************************************************
 *Function name:  ChOutMgBuffer
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChOutMgBuffer(MG_FILTER_STRUCT * aucOutputBuff, MgQueue * Q)
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
CHCA_BOOL  ChOutMgBuffer(MG_FILTER_STRUCT * aucOutputBuff, MgQueue * Q)
{
   /*if the queue is not empty,then output the front element of the queue,
	using  variable aucOutputBuff ,otherwise return error*/
   CHCA_INT    iSectionLength;
   
   if(Q == NULL )
   {
          do_report(severity_info,"\n The EMM Queue is NULL! \n");
	   return true;
   }
   
   if(Q -> sRear == Q -> sFront)   /*the queue is empty*/
   {
	   /*do_report(severity_info,"\n the mg queue is empty \n");*/
	   return true;
   } 
    
   iSectionLength = (Q->data[Q->sFront].buff[1] << 8  | Q->data[Q->sFront].buff[2]) + 3;
   
   if(iSectionLength > MGCA_PRIVATE_TABLE_MAXSIZE)
   {
       do_report(severity_info,"\n iSectionLength[%d] is error \n",iSectionLength);
	   return true;
   }

   /*do_report(severity_info,"\n iSectionLength[%d] ,%d, %d\n ",iSectionLength,Q->data[Q->sFront].buff[iSectionLength-2],Q->data[Q->sFront].buff[iSectionLength-1]);*/
     
 
   if ( aucOutputBuff != NULL )
		memcpy ( ( BYTE *)aucOutputBuff -> aucMgSectionData,
		( BYTE *) Q->data[Q->sFront].buff,
		/*ONE_KILO_SECTION_MAX_LENGTH*/iSectionLength );

   
   aucOutputBuff->iSectionSlotId = Q -> data[Q -> sFront].iSectionSlotId;
   Q -> sFront = (Q -> sFront + 1) % MGCA_MAX_NO_EMMBUFF;

   return false;
}



/*******************************************************************************
 *Function name:  ChOutMgBuffer
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChOutMgBuffer(MG_FILTER_STRUCT * aucOutputBuff, MgQueue * Q)
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
CHCA_BOOL   ChInsertMgEmmBuffer(CHCA_INT slotFilter, CHCA_UCHAR * aucInputBuffer)
{
       CHCA_INT                                               iSectionLength,i;
	CHCA_BOOL                                            err;
	CHCA_INT                                               Emm_len1;
       CHCA_BOOL                                            SameCaData = false;

       CHCA_UCHAR                                          CrcDataTemp[4];
	MG_FILTER_STRUCT                                 TempEmmFilterData;

	if(!CardOperation.bCardReady)
	{
             do_report(severity_info,"\n[ChInsertMgEmmBuffer==>] The card is not ready!!\n");
	      return true;		 
	}
 
        if(slotFilter>=CHCA_MAX_NUM_FILTER)
        {
              do_report(severity_info,"\n[ChInsertMgEmmBuffer==>] The filter index[%d] is wrong\n",slotFilter);
              return true;
        }
        
        if(aucInputBuffer==NULL)
        {
              do_report(severity_info,"\n[ChInsertMgEmmBuffer==>]auclocalBuffer Poniter is null \n");
              return true;
        }
        
        iSectionLength = (( aucInputBuffer [ 1 ] & 0x0f ) << 8 ) | aucInputBuffer [ 2 ] + 3;
        
        if(iSectionLength<=0 || iSectionLength>=MGCA_PRIVATE_TABLE_MAXSIZE)
        {
              do_report(severity_info,"\n[ChInsertMgEmmBuffer==>]the len is wrong \n");
              return true;
        }

#if   0                           
        do_report(severity_info,"\n[ChInsertMgEmmBuffer==>]EMM DATA:");		  
        for(i=0;i<iSectionLength;i++)
             do_report(severity_info,"%4x",*(aucInputBuffer+i));	
        do_report(severity_info,"\n");	
#endif		
                                
 
       semaphore_wait(psemSectionEmmQueueAccess);
	/*do_report(severity_info,"\n MgEmmFilterData.OnUse [%d]\n",MgEmmFilterData.OnUse);*/   
#if  0	
       /*without the emm buffer*/
       if(!MgEmmFilterData.OnUse)
       {
             MgEmmFilterData.OnUse = true;
	      memcpy(MgEmmFilterData.aucMgSectionData,aucInputBuffer,MGCA_PRIVATE_TABLE_MAXSIZE);
             if(ChSMailFromMGFilter2EMM(MgEmmFilterData.iSectionSlotId))
             {
                    MgEmmFilterData.OnUse = false;
	      }		  
	}
#else	   	
       /*with the emm buffer*/	
       if(!MgEmmFilterData.OnUse)
       {
                Emm_len1 = ChMgQueueLength(&EmmQueue);
                if(Emm_len1)
                {
                       ChEnMgBuffer(slotFilter,aucInputBuffer,&EmmQueue);
                       err =  ChOutMgBuffer(&MgEmmFilterData,&EmmQueue);
                       if(!err)
                       {
                              MgEmmFilterData.OnUse = true;
                              if(ChSMailFromMGFilter2EMM(MgEmmFilterData.iSectionSlotId))
                              {
                                    MgEmmFilterData.OnUse = false;
				  }
                       }
                }
                else /*the mg queue is empty*/
                {
                       /*do_report(severity_info,"\n[ChInsertMgEmmBuffer==>] The emm buffer is empty,send the emm data immediately\n");*/
                       memcpy ( ( BYTE *)MgEmmFilterData.aucMgSectionData,
                                        ( BYTE *) aucInputBuffer,
                                        MGCA_PRIVATE_TABLE_MAXSIZE);
         
                      MgEmmFilterData.iSectionSlotId = slotFilter;
	               MgEmmFilterData.OnUse = true;
			 if(ChSMailFromMGFilter2EMM(MgEmmFilterData.iSectionSlotId))	   
			 {
                            MgEmmFilterData.OnUse = false;
			 }

                      /*ChSendEMMData(slotFilter,aucInputBuffer,&MgEmmFilterData);*/
                }
       }
       else
       {
                Emm_len1 = ChMgQueueLength(&EmmQueue);
		  if(Emm_len1==MGCA_MAX_NO_EMMBUFF-1/*20060623 add -1*/)
		  {
                      ChOutMgBuffer(&TempEmmFilterData,&EmmQueue);
		  }
                ChEnMgBuffer(slotFilter,aucInputBuffer,&EmmQueue);
       }
#endif	   
       semaphore_signal(psemSectionEmmQueueAccess);
 
	return false;

}


/*******************************************************************************
 *Function name:  ChInitCaQueue
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *void ChInitCaQueue(MgQueue * Q)
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
void ChInitCaQueue(MgQueue * Q)
{
        if(Q != NULL)
              Q -> sFront = Q -> sRear = 0;
        else
              do_report(severity_info,"\n[ChInitCaQueue==>] fail to init  the emm queue\n");	 
}


/*******************************************************************************
 *Function name:  ChSectionSend2Pmt
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChSectionSend2Pmt( CHCA_USHORT          iFilterId,
 *                                                                      CHCA_MGData_t        bTableId,
 *                                                                      CHCA_BOOL              bTableType,
 *                                                                      TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
 *                                                                     TMGDDIEventCode  EvCode,
 *                                                                      CHCA_BOOL       return_status)
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
CHCA_BOOL  ChSectionSend2Pmt( CHCA_USHORT          iFilterId,
                                                                       CHCA_MGData_t        bTableId,
                                                                       CHCA_BOOL              bTableType,
                                                                       TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
                                                                       TMGDDIEventCode  EvCode,
                                                                       CHCA_BOOL       return_status)
{
       chca_mg_cmd_t                     *msg_p=NULL;
       clock_t                                 cMgcaTime;
       message_queue_t                 *pstQueueId;
	 
	if(iFilterId<0 || iFilterId>=CHCA_MAX_NUM_FILTER)		
	{
	      do_report(severity_info,"\n[ChSectionSend2Pmt==>] fail to send the pmt table for wrong  filter\n");		
             return true;
	}

	/*pstQueueId = SectionFilter [iFilterId].pMsgQueue;*/
	/*pstQueueId = pstCHCAPMTMsgQueue;*/

       if( pstCHCAPMTMsgQueue != NULL)
       {
       #if 0/*20060323 change*/
              cMgcaTime = ST_GetClocksPerSecondLow();
	   #else
                cMgcaTime = time_plus(time_now(),ST_GetClocksPerSecond()*1);

	   #endif
              msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCAPMTMsgQueue, &cMgcaTime );

              if(  msg_p != NULL)
              {
                      /*SectionFilter[iFilterId].bBufferLock = return_status;*/
                      msg_p ->contents.sDmxfilter.TableId   = bTableId;  /*add this on 040828*/
                      msg_p ->contents.sDmxfilter.iSectionFilterId   = iFilterId;
                      msg_p ->contents.sDmxfilter.TableType   = bTableType;
                      msg_p ->contents.sDmxfilter.return_status   = ExCode;
                      msg_p ->contents.sDmxfilter.TmgEventCode   = EvCode;
                      msg_p ->from_which_module = CHCA_SECTION_MODULE;

			/* if(bTableId == ECM_DATA)		  
                        do_report(severity_info,"\n[ChSectionSend2Pmt==>] success to send the ecm  table filter[%d] \n",iFilterId);		*/
		  
                      message_send ( pstCHCAPMTMsgQueue , msg_p );
			 return	false;
              }
		else
		{
		      do_report(severity_info,"\n[ChSectionSend2Pmt==>] fail to send the pmt  table filter[%d] \n",iFilterId);		
		}
       }
	else
 	{
               do_report(severity_info,"\n[ChSectionSend2Pmt==>] pstCHCAPMTMsgQueue is null \n");			   
	}
	   
       return true;
	   
}


/*******************************************************************************
 *Function name:  ChSectionSend2Cat
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *CHCA_BOOL  ChSectionSend2Cat( CHCA_USHORT  iFilterId,
 *                                                           CHCA_MGData_t  bTableId,
 *                                                           CHCA_BOOL bTableType,
 *                                                           TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
 *                                                           TMGDDIEventCode  EvCode,
 *                                                           CHCA_BOOL  return_status )
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
CHCA_BOOL  ChSectionSend2Cat( CHCA_USHORT     iFilterId,
                                                                           CHCA_MGData_t    bTableId,
                                                                           CHCA_BOOL  bTableType,
                                                                           TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
                                                                           TMGDDIEventCode  EvCode,
                                                                           CHCA_BOOL       return_status)
{
       chca_mg_cmd_t                     *msg_p=NULL;
       clock_t                                  cMgcaTime;
       message_queue_t                  *pstQueueId;
	 
	if(iFilterId<0 || iFilterId>=CHCA_MAX_NUM_FILTER)		
	{
             return true;
	}

	/*pstQueueId = SectionFilter [iFilterId].pMsgQueue;*/
	/*pstQueueId = pstCHCAPMTMsgQueue;*/

       if( pstCHCACATMsgQueue != NULL)
       {
       #if 0/*20060323 change*/
              cMgcaTime = ST_GetClocksPerSecondLow();
	   #else
           cMgcaTime = time_plus(time_now(),ST_GetClocksPerSecond()*1);

	   #endif
              msg_p = ( chca_mg_cmd_t * ) message_claim_timeout ( pstCHCACATMsgQueue, &cMgcaTime );
		/*do_report(severity_info,"\n[ChSectionSend2Cat=>] CatMsp[%x] \n",msg_p);*/			  

              if(  msg_p != NULL)
              {
                      msg_p ->from_which_module   = CHCA_SECTION_MODULE;  /*add this on 041114*/
                      msg_p ->contents.sDmxfilter.TableId   = CAT_DATA;  /*add this on 040828*/
                      msg_p ->contents.sDmxfilter.iSectionFilterId   = iFilterId;
                      msg_p ->contents.sDmxfilter.TableType   = bTableType;
                      msg_p ->contents.sDmxfilter.return_status   = ExCode;
                      msg_p ->contents.sDmxfilter.TmgEventCode   = EvCode;
                      message_send ( pstCHCACATMsgQueue , msg_p );

			 /*do_report(severity_info,"\n[ChSectionSend2Cat=>] success to send the cat,msp[%x] \n",msg_p);	*/		  
			 return	false;
              }
		else
		{
		       do_report(severity_info,"\n[ChSectionSend2Cat==>] fail to send the cat or cat data \n");			  
		}
       }
	else
 	{
               do_report(severity_info,"\n[ChSectionSend2Cat==>] pstCHCACATMsgQueue is null \n");			   
	}
	   
       return true;
	   
}


#if 1/*加入软件判断PMT版本*/
/*******************************************************************************
 *Function name: 
 *                   ChInsertMgData
 *
 *Description: 
 *                  
 *
 *Prototype:
 *        CHCA_BOOL ChInsertMgData(SHORT  iFilterId,CHCA_UCHAR * auclocalBuffer)
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
 *     2005-03-10         add same ecm data process, do not send the same ecm data 
 *     2005-03-19         add same data process
 * 
 *******************************************************************************/
boolean    CHCA_InsertPMTData( short  iFilterId,BYTE*  auclocalBuffer)
{
 	CHCA_INT                                                iSectionLength;  
	CHCA_UCHAR                                           TableId;
  	boolean                                                    bTableType;
	TMGDDIEvSrcDmxFltrTimeoutExCode         ExCode;
	TMGDDIEventCode                                   iEvCode;
       CHCA_INT                                               CAT_Version_Num;
	CHCA_BOOL                                            bReturnCode=false;
	CHCA_MGData_t                                      DataType;
	CHCA_UINT                                             i,iDataIndex;
	CHCA_CHAR                                          TempPmtVersion=-1;

	#if 1/*软件判断PMT版本，以取代不等模式*/
		TempPmtVersion=(auclocalBuffer[5]&0x3e)>>1;
		if((PmtVesion==TempPmtVersion)/*&&(TempPmtVersion!=-1)*/)
		{
			do_report(0,"\n same PmtVesion ");
			//return true;
		}
		
			
	#endif

#ifdef CH_IPANEL_MGCA
       if(iFilterId < 0)
       {
             do_report(severity_info,"\n[CHCA_InsertPMTData==>] The filter index[%d] is wrong\n",iFilterId);
	      bReturnCode = true;
	      return bReturnCode;	  
	}
#else
    if(iFilterId >= MAX)
       {
             do_report(severity_info,"\n[CHCA_InsertPMTData==>] The filter index[%d] is wrong\n",iFilterId);
	      bReturnCode = true;
	      return bReturnCode;	  
	}
#endif
	if(auclocalBuffer == NULL)
	{
             do_report(severity_info,"\n[CHCA_InsertPMTData==>]auclocalBuffer Poniter is null \n");
	      bReturnCode = true;
	      return bReturnCode;	  
	}

       iSectionLength = (( auclocalBuffer [ 1 ] & 0x0f ) << 8 ) | auclocalBuffer [ 2 ] + 3;
      

       if((iSectionLength == 0) || (iSectionLength>MGCA_NORMAL_TABLE_MAXSIZE))
       {
            bReturnCode = true;
            do_report(severity_info,"\n[CHCA_InsertPMTData==>]other section len is wrong! \n");
            return bReturnCode;	  
       }
#ifndef CH_IPANEL_MGCA/*不需要控制FILTER状态*/
	CH6_StopFilter(iFilterId,false);
#endif	
	PmtVesion=TempPmtVersion;
	
	TableId   =  auclocalBuffer[0];
 	DataType = PMT_DATA;
	iEvCode = MGDDIEvSrcDmxFltrReport;
	bTableType = false;
	ExCode = 0;

#if  0                         
       do_report(severity_info,"\n[CHCA_InsertPMTData==>]PMT DATA:");		  
       for(i=0;i<6;i++)
              do_report(severity_info,"%4x",auclocalBuffer[i]);	
       do_report(severity_info,"\n");	
#endif	
  CHCA_SectionDataAccess_Lock();
	memset(PmtSectionBuffer,0,MGCA_NORMAL_TABLE_MAXSIZE);
       memcpy(PmtSectionBuffer,auclocalBuffer,iSectionLength);
  CHCA_SectionDataAccess_UnLock();
    
       bReturnCode = ChSectionSend2Pmt(iFilterId,DataType,bTableType,ExCode,iEvCode,true);
	
#ifdef SIMPLE_CA_TRACE /*060113 xjp add for test*/
	do_report(0,"\n [PMT section monitor] PMT section Data is  captured !\n");
#endif

	return bReturnCode;   

}

#ifdef CH_IPANEL_MGCA

boolean    CHCA_InsertLastPMTData( void)
{
 	CHCA_INT                                                iSectionLength;  
	CHCA_UCHAR                                           TableId;
  	boolean                                                    bTableType;
	TMGDDIEvSrcDmxFltrTimeoutExCode         ExCode;
	TMGDDIEventCode                                   iEvCode;
       CHCA_INT                                               CAT_Version_Num;
	CHCA_BOOL                                            bReturnCode=false;
	CHCA_MGData_t                                      DataType;
	CHCA_UINT                                             i,iDataIndex;
	CHCA_CHAR                                          TempPmtVersion=-1;


  CHCA_SectionDataAccess_Lock();

       iSectionLength = (( PmtSectionBuffer [ 1 ] & 0x0f ) << 8 ) | PmtSectionBuffer [ 2 ] + 3;
      

       if((iSectionLength == 0) || (iSectionLength>MGCA_NORMAL_TABLE_MAXSIZE))
       {
            bReturnCode = true;
	CHCA_SectionDataAccess_UnLock();
            do_report(severity_info,"\n[CHCA_InsertLastPMTData==>]other section len is wrong! \n");
            return bReturnCode;	  
       }
	PmtVesion = TempPmtVersion;
	
	TableId   =  PmtSectionBuffer[0];
 	DataType = PMT_DATA;
	iEvCode = MGDDIEvSrcDmxFltrReport;
	bTableType = false;
	ExCode = 0;

       CHCA_SectionDataAccess_UnLock();
    
       bReturnCode = ChSectionSend2Pmt(1,DataType,bTableType,ExCode,iEvCode,true);
	

	return bReturnCode;   

}

#endif

#else


/*******************************************************************************
 *Function name: 
 *                   ChInsertMgData
 *
 *Description: 
 *                  
 *
 *Prototype:
 *        CHCA_BOOL ChInsertMgData(CHCA_USHORT  iFilterId,CHCA_UCHAR * auclocalBuffer)
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
 *     2005-03-10         add same ecm data process, do not send the same ecm data 
 *     2005-03-19         add same data process
 * 
 *******************************************************************************/
boolean    CHCA_InsertPMTData(unsigned short  iFilterId,BYTE*  auclocalBuffer)
{
 	CHCA_INT                                                iSectionLength;  
	CHCA_UCHAR                                           TableId;
  	boolean                                                    bTableType;
	TMGDDIEvSrcDmxFltrTimeoutExCode         ExCode;
	TMGDDIEventCode                                   iEvCode;
       CHCA_INT                                               CAT_Version_Num;
	CHCA_BOOL                                            bReturnCode=false;
	CHCA_MGData_t                                      DataType;
	CHCA_UINT                                             i,iDataIndex;
	CHCA_UCHAR                                          TempPmtVersion=-1;



       if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[CHCA_InsertPMTData==>] The filter index[%d] is wrong\n",iFilterId);
	      bReturnCode = true;
		  CH6_ReenableFilter(iFilterId);
	      return bReturnCode;	  
	}

	if(auclocalBuffer==NULL)
	{
             do_report(severity_info,"\n[CHCA_InsertPMTData==>]auclocalBuffer Poniter is null \n");
	      bReturnCode = true;
		  CH6_ReenableFilter(iFilterId);
	      return bReturnCode;	  
	}

       iSectionLength = (( auclocalBuffer [ 1 ] & 0x0f ) << 8 ) | auclocalBuffer [ 2 ] + 3;
       TableId   =  auclocalBuffer[0];
 	DataType = PMT_DATA;
       /*DataType = CHCA_GetDataType(TableId);*/

       if((iSectionLength == 0) || (iSectionLength>MGCA_NORMAL_TABLE_MAXSIZE))
       {
            bReturnCode = true;
            do_report(severity_info,"\n[CHCA_InsertPMTData==>]other section len is wrong! \n");
		CH6_ReenableFilter(iFilterId);
            return bReturnCode;	  
       }
	
	
	
	iEvCode = MGDDIEvSrcDmxFltrReport;
	bTableType = false;
	ExCode = 0;

#if  0                         
       do_report(severity_info,"\n[CHCA_InsertPMTData==>]PMT DATA:");		  
       for(i=0;i<6;i++)
              do_report(severity_info,"%4x",auclocalBuffer[i]);	
       do_report(severity_info,"\n");	
#endif	

       memcpy(PmtSectionBuffer,auclocalBuffer,MGCA_NORMAL_TABLE_MAXSIZE);
       
       bReturnCode = ChSectionSend2Pmt(iFilterId,DataType,bTableType,ExCode,iEvCode,true);

#if 1/*060113 xjp add for test*/
	do_report(0,"\n [PMT section monitor] PMT section Data is  captured !\n");
#endif

	return bReturnCode;   

}

#endif




#if 0
CHCA_BOOL ChInsertECMData(CHCA_USHORT  iFilterId,CHCA_UCHAR * auclocalBuffer)
{
 	CHCA_INT                                                iSectionLength;  
	CHCA_UCHAR                                           TableId;
  	boolean                                                    bTableType;
	TMGDDIEvSrcDmxFltrTimeoutExCode         ExCode;
	TMGDDIEventCode                                   iEvCode;
       CHCA_INT                                               CAT_Version_Num;
	CHCA_BOOL                                            bReturnCode=false;
	CHCA_MGData_t                                      DataType;
	CHCA_UINT                                             i,iDataIndex;
	CHCA_BOOL                                           SameCaData=false;


       if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[ChInsertMgData==>] The filter index[%d] is wrong\n",iFilterId);
	      bReturnCode = true;
	      return bReturnCode;	  
	}

	if(auclocalBuffer==NULL)
	{
             do_report(severity_info,"\n[ChInsertMgData==>]auclocalBuffer Poniter is null \n");
	      bReturnCode = true;
	      return bReturnCode;	  
	}

       iSectionLength = (( auclocalBuffer [ 1 ] & 0x0f ) << 8 ) | auclocalBuffer [ 2 ] + 3;
	DataType = ECM_DATA;

	FDmxFlterInfo [ iFilterId ].ValidSection = false;					   
	iEvCode = MGDDIEvSrcDmxFltrReport;
	bTableType = FDmxFlterInfo [ iFilterId ].MulSection;
	ExCode = 0;

       if((iSectionLength >= 0) && (iSectionLength <= MGCA_PRIVATE_TABLE_MAXSIZE))
       {
#if  0                         
              do_report(severity_info,"\nECM DATA:");		  
              for(i=0;i<iSectionLength;i++)
              do_report(severity_info,"%4x",auclocalBuffer[i]);	
              do_report(severity_info,"\n");	
#endif	
       
              /*if((FDmxFlterInfo [ iFilterId ].FilterType==ECM_FILTER) && (!FDmxFlterInfo [ iFilterId ].Lock)&&(!EcmDataStop))*/
              if((FDmxFlterInfo [ iFilterId ].FilterType==ECM_FILTER) && (!FDmxFlterInfo [ iFilterId ].Lock))
              {
                     memcpy(FDmxFlterInfo [ iFilterId ].DataBuffer,auclocalBuffer,iSectionLength);		
                     if(CardOperation.bCardReady)
                     {
                          FDmxFlterInfo [ iFilterId ].Lock = true;
			     bReturnCode = ChSectionSend2Ecm(iFilterId,DataType,bTableType,ExCode,iEvCode,true);				   	
                          /*bReturnCode = ChSectionSend2Pmt(iFilterId,DataType,bTableType,ExCode,iEvCode,true);  */
                     } 					 
              }
       }

	return bReturnCode;   

}
#endif


/*******************************************************************************
 *Function name: 
 *                   ChInsertMgData
 *
 *Description: 
 *                  
 *
 *Prototype:
 *        CHCA_BOOL ChInsertMgData(CHCA_USHORT  iFilterId,CHCA_UCHAR * auclocalBuffer)
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
 *      5516_platform
 *     
 * 
 *******************************************************************************/
CHCA_BOOL ChInsertMgData(CHCA_USHORT  iFilterId,CHCA_UCHAR * auclocalBuffer)
{
 	CHCA_INT                                                iSectionLength;  
	CHCA_UCHAR                                           TableId;
  	boolean                                                    bTableType;
	TMGDDIEvSrcDmxFltrTimeoutExCode         ExCode;
	TMGDDIEventCode                                   iEvCode;
       CHCA_INT                                               CAT_Version_Num;
	CHCA_BOOL                                            bReturnCode=false;
	CHCA_MGData_t                                      DataType;
	CHCA_UINT                                             i,iDataIndex;
	CHCA_BOOL                                           SameCaData=false;
	/*int                                                          CACrcData=0;  add this on 050319*/


       if(iFilterId>=CHCA_MAX_NUM_FILTER)
       {
             do_report(severity_info,"\n[ChInsertMgData==>] The filter index[%d] is wrong\n",iFilterId);
	      bReturnCode = true;
	      return bReturnCode;	  
	}

	if(auclocalBuffer==NULL)
	{
             do_report(severity_info,"\n[ChInsertMgData==>]auclocalBuffer Poniter is null \n");
	      bReturnCode = true;
	      return bReturnCode;	  
	}

       iSectionLength = (( auclocalBuffer [ 1 ] & 0x0f ) << 8 ) | auclocalBuffer [ 2 ] + 3;
       TableId   =  auclocalBuffer[0];
	DataType = CHCA_GetDataType(TableId);

	if(DataType==ECM_DATA)
	{
            if((iSectionLength == 0) && (iSectionLength >= MGCA_PRIVATE_TABLE_MAXSIZE))
            {
	            bReturnCode = true;
                   do_report(severity_info,"\n[ChInsertMgData==>]ecm section len is wrong! \n");
	            return bReturnCode;	  
	     }
	}
	else
	{
              if((iSectionLength == 0) || (iSectionLength>MGCA_NORMAL_TABLE_MAXSIZE))
             {
	            bReturnCode = true;
                   do_report(severity_info,"\n[ChInsertMgData==>]other section len is wrong! \n");
	            return bReturnCode;	  
	      }
	}
	
       /*TableId   =  auclocalBuffer[0];*/
#if 0 
       if(FDmxFlterInfo[iFilterId].TTriggerMode)
       {
              /*the related filter is stopped and set inactive by the demultiplexer driver.*/
		CHCA_Stop_Filter(iFilterId);	  
              FDmxFlterInfo [ iFilterId ].bdmxactived =  false;
 	}
#endif	   
										   
	FDmxFlterInfo [ iFilterId ].ValidSection = false;					   
	iEvCode = MGDDIEvSrcDmxFltrReport;
	bTableType = FDmxFlterInfo [ iFilterId ].MulSection;
	ExCode = 0;

	/*DataType = CHCA_GetDataType(TableId);*/

#if  0               
       do_report(severity_info,"\n[ChInsertMgData==>]CA DATA:");		  
       for(i=0;i<iSectionLength;i++)
              do_report(severity_info,"%4x",auclocalBuffer[i]);	
       do_report(severity_info,"\n");	
#endif	

       switch(DataType)
       {
              case CAT_DATA: /*cat table*/
                      CHCA_StopDemuxFilter(iFilterId); /*add this on 050424*/
                     do_report(severity_info,"\nCAT Table coming\n");  /* */
					   
                        if((FDmxFlterInfo [ iFilterId ].FilterType==CAT_FILTER) && (!FDmxFlterInfo [ iFilterId ].Lock))
                       {
                             /*do_report(severity_info,"\n[ChInsertMgData==>]cat data is comming\n");*/
                             memcpy(CatSectionBuffer,auclocalBuffer,MGCA_NORMAL_TABLE_MAXSIZE);
                             /*memcpy(FDmxFlterInfo [ iFilterId ].DataBuffer,auclocalBuffer,MGCA_NORMAL_TABLE_MAXSIZE);*/
                             
			        bReturnCode = ChSectionSend2Cat(iFilterId,DataType,bTableType,ExCode,iEvCode,true);
		
				if(!bReturnCode)
				{
                             FDmxFlterInfo [ iFilterId ].Lock = true;
				#ifdef SIMPLE_CA_TRACE/*060113 xjp add for test*/	
					do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Succeed in capturing the  CAT section data\n");
				#endif
				}
			
			        /*bReturnCode = ChSectionSend2Pmt(iFilterId,DataType,bTableType,ExCode,iEvCode,true); */ 
                       }
			  /*CatFilterId = iFilterId;delete this on 050311*/
			  break;

#if    0			  
	       case PMT_DATA: /*pmt table*/
			  memcpy(PmtSectionBuffer,auclocalBuffer,MGCA_NORMAL_TABLE_MAXSIZE);
			  /*memcpy(FDmxFlterInfo [ iFilterId ].DataBuffer,auclocalBuffer,MGCA_NORMAL_TABLE_MAXSIZE);*/
                       bReturnCode = ChSectionSend2Pmt(iFilterId,DataType,bTableType,ExCode,iEvCode,true);
			  break;
#endif

	       case ECM_DATA: /*ecm table*/
		   	 /*memcpy(EcmSectionBuffer,auclocalBuffer,MGCA_PRIVATE_TABLE_MAXSIZE);*/
                      /*if((iSectionLength >= 0) && (iSectionLength <= MGCA_PRIVATE_TABLE_MAXSIZE))*/
                      {
#if  0                         
                                do_report(severity_info,"\nECM DATA:");		  
                                for(i=0;i<iSectionLength;i++)
                                      do_report(severity_info,"%4x",auclocalBuffer[i]);	
                                do_report(severity_info,"\n");	
#endif	
               
                              if((FDmxFlterInfo [ iFilterId ].FilterType==ECM_FILTER) && (!FDmxFlterInfo [ iFilterId ].Lock))
                               {
                                     memcpy(FDmxFlterInfo [ iFilterId ].DataBuffer,auclocalBuffer,iSectionLength);		
                                     if(CardOperation.bCardReady)
                                     {
                                           
                                             /*bReturnCode = ChSectionSend2Pmt(iFilterId,DataType,bTableType,ExCode,iEvCode,true);  */
                                             bReturnCode = ChSectionSend2Ecm(iFilterId,DataType,bTableType,ExCode,iEvCode,true); /*modify this on 050308*/
				
						if(!bReturnCode)
						{
							FDmxFlterInfo [ iFilterId ].Lock = true;
							
						#ifdef SIMPLE_CA_TRACE/*060113 xjp add for test*/
							do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Succeed in capturing the  ECM section data\n");
						#endif
						}
				
                                     } 					 
                              }
				  /*else
				  {
                                   do_report(severity_info,"\n[ChInsertMgData==>]Ecm Filter has been filter,no data\n");
				  }*/
                      }
			 break;

#if 0/*add this on 050308*/
	       case EMM_DATA:  /*emm table*/
		   	  if((iSectionLength >= 0) && (iSectionLength <= MGCA_PRIVATE_TABLE_MAXSIZE))
			  {
			           CHCA_BOOL      bError = false; 
#if  0                            
                                do_report(severity_info,"\nEMM DATA:");		  
                                for(i=0;i<iSectionLength;i++)
                                      do_report(severity_info,"%4x",*(auclocalBuffer+i));	
                                do_report(severity_info,"\n");	
#endif		
                                
                                /*do_report(severity_info,"\nEmm[%4x][%4x][%4x][%4x][%4x][%4x]\n",
                                                 auclocalBuffer[0],
                                                 auclocalBuffer[1],
                                                 auclocalBuffer[2],
                                                 auclocalBuffer[iSectionLength-3],
                                                 auclocalBuffer[iSectionLength-2],
                                                 auclocalBuffer[iSectionLength-1]);*/
                                if(CardOperation.bCardReady)
                                ChInsertMgEmmBuffer(iFilterId,auclocalBuffer);
		   	 }
			 break;
#endif
		  	
		default:
			 break;
	}

	return bReturnCode;   

}




/*******************************************************************************
 *Function name:  CHCA_InitMgEmmQueue
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *void CHCA_InitMgEmmQueue(void)
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
void CHCA_InitMgEmmQueue(void)
{
       semaphore_wait( psemSectionEmmQueueAccess );
       ChInitCaQueue(&EmmQueue);  
       MgEmmFilterData.OnUse = false; 
       semaphore_signal( psemSectionEmmQueueAccess );	   
}



/*******************************************************************************
 *Function name:  CHCA_MgEmmSectionUnlock
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 * CHCA_UINT   CHCA_MgEmmSectionUnlock (void) 
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
 #if 1/*modify this on 050115*/
 CHCA_UINT   CHCA_MgEmmSectionUnlock (void)
{
       CHCA_BOOL            bError = false;
	CHCA_UINT             QLen =0;        
       
       semaphore_wait( psemSectionEmmQueueAccess );

       if(MgEmmFilterData.OnUse)
       {
              bError =  ChOutMgBuffer(&MgEmmFilterData,&EmmQueue); 
              if ( bError )
              {
                    /*do_report(severity_info,"\n[CHCA_MgEmmSectionUnlock==>]The emm buffer is empty or wrong!\n");*/
                    MgEmmFilterData.OnUse= false; 
              }			
              else
              {
              #ifdef SIMPLE_CA_TRACE
                    do_report(severity_info,"\n[CHCA_MgEmmSectionUnlock==>]pop the emm data and send to emm monitor because the emm queue is not empty!\n");
		#endif
                    ChSMailFromMGFilter2EMM(MgEmmFilterData.iSectionSlotId);
              }			
       }

       semaphore_signal( psemSectionEmmQueueAccess );
	   
       return (QLen);	

}
#else
CHCA_UINT   CHCA_MgEmmSectionUnlock (void)
{
       CHCA_BOOL            bError = false;
	CHCA_UINT             QLen =0;        
       
       semaphore_wait( psemSectionEmmQueueAccess );
       QLen = ChMgQueueLength(&EmmQueue); 
       /*do_report(severity_info,"\n[CHCA_MgEmmSectionUnlock==>] The len[%d] of the EmmQueue\n",QLen);*/
	if(QLen) 
	{
              bError =  ChOutMgBuffer(&MgEmmFilterData,&EmmQueue); 
              if(bError)
              {
                    QLen = 0;
		}
	}
       semaphore_signal( psemSectionEmmQueueAccess );
	   
       return (QLen);	

}
#endif



/*******************************************************************************
 *Function name:  CHCA_GetQEmmLen
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *          CHCA_UINT  CHCA_GetQEmmLen(void)
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
CHCA_UINT     CHCA_GetQEmmLen(void)
{
       CHCA_UINT    QLen=0;
	   
       semaphore_wait( psemSectionEmmQueueAccess );
       QLen = ChMgQueueLength(&EmmQueue); 
       semaphore_signal( psemSectionEmmQueueAccess );

	return  QLen; 
}




/*******************************************************************************
 *Function name:  CHCA_GetDataType
 * 
 *
 *Description:  get the type of the data
 *                  
 *
 *Prototype:
 *         CHCA_MGData_t   CHCA_GetDataType(BYTE  bType)
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
CHCA_MGData_t   CHCA_GetDataType(CHCA_UCHAR  bType)
{
        CHCA_MGData_t    MgDataType;
 
         switch(bType)
         {
              case 0x1:
			  MgDataType = CAT_DATA; 	
			  break;

		case 0x2:
			 MgDataType = PMT_DATA;
			  break;

		case 0x80:
		case 0x81:
			  MgDataType = ECM_DATA;
			  break;

		case 0x82:	  
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x90:
			 MgDataType = EMM_DATA;
			 break;

		default:
			MgDataType = OTHER_DATA;
			break;
	  }

	  return 	MgDataType; 

}


#if      /*PLATFORM_16 20070514 change*/1
#if 0
/*******************************************************************************
 *Function name:  ChDVBPMTSectionFilterMonitor
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          static void  ChDVBPMTSectionFilterMonitor(void *pvParam )
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
 *     5516_platform
 * 
 * 
 *******************************************************************************/
static void  ChDVBPMTSectionFilterMonitor(void *pvParam )
{
	  ST_ErrorCode_t                           ErrCode;
	  STPTI_Buffer_t                            BufferHandleTemp;
	  STPTI_Filter_t                              MatchedFilterHandle[16];
	  U32                                            MatchedFilterCount=0;
	  S32                                            FilterIndexTemp;
	  S32                                            i;
	  clock_t                                       TimeTemp;	
	  BOOL                                         SecCRCTemp = false;
	  CH6_FILTER_STATUS_t                FilterStatus;
	  U32                                            WaitTimeoutMs = 1;
	  
	  
	  U8*                                            pSecDataTemp;
	  U32                                            SecLenRead=0;

	  CHCA_UINT                                iSectionLength=0;

	  
	  TimeTemp=(clock_t)ST_GetClocksPerSecond()/2;
	  pSecDataTemp=(U8*)memory_allocate(CHSysPartition ,ONE_KILO_SECTION);
	  
	  while(true)
	  { 

#if 0
                 if(CHCA_GetMatchSectionInfo(&BufferHandleTemp))
                {
                        task_delay(3000);
			   do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			
		          continue;
		  }
#endif		

#if 1
	        /*CHCA_MgFilterDelayCheck();*/
				
                /*wait the ca section received*/
          	  ErrCode = STPTI_SignalWaitBuffer(MgPMTSignalHandle,&BufferHandleTemp,WaitTimeoutMs); 
#else				
          	  ErrCode = STPTI_SignalWaitBuffer(MgCASignalHandle,&BufferHandleTemp,STPTI_TIMEOUT_INFINITY); 
#endif


                if(ErrCode!=ST_NO_ERROR)
		 {
			   /*do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			*/
		          continue;
		  }		 


                ErrCode = STPTI_BufferReadSection(BufferHandleTemp,
		                                                            MatchedFilterHandle,
		  	                                                      16,
		  	                                                     &MatchedFilterCount,
			                                                     &SecCRCTemp,
			                                                     (U8 *)pSecDataTemp,
			                                                     ONE_KILO_SECTION,
			                                                     NULL,
			                                                     0,
			                                                     &SecLenRead,
			                                                     STPTI_COPY_TRANSFER_BY_MEMCPY);

#if 0
		  if(CHCA_CopySection2Buffer(BufferHandleTemp,
   	                                                        MatchedFilterHandle,
		  	                                          16,
		  	                                          &MatchedFilterCount,
			                                          &SecCRCTemp,
			                                          (U8 *)pSecDataTemp,
			                                          FOUR_KILO_SECTION,
			                                          &SecLenRead)==true)
		  {
		        do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] copy data err\n");
                      continue;
		  }
#endif

                if(ErrCode!=ST_NO_ERROR)      
                {
		         do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] copy data err\n");
                       continue;
		  }
		  
		  CHCA_SectionDataAccess_Lock();  /*delete this on 050115*/
		  
		  for(i=0;i<MatchedFilterCount;i++)
		  {
                       /*FilterIndexTemp = CHCA_FindMatchFilter(MatchedFilterHandle,i);*/

  			  FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_FILTERHANDLE,(void*)&MatchedFilterHandle[i]);		   
					   
			  if((FilterIndexTemp==-1) || (pSecDataTemp==NULL)) 
			  {
			       continue;
			  }	   

			  /*if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_DONE) 
			  {
                            continue;
			  }*/
			  
			  if(SecCRCTemp==false)
			  {
#ifdef                    CHPROG_DRIVER_DEBUG
				  STTBX_Print(("Section Monitor:CRC wrong in Filter index=%d\n",FilterIndexTemp));
#endif
       			  continue;
				  
			  }


                       if(CHCA_InsertPMTData(FilterIndexTemp,(CHCA_UCHAR*)pSecDataTemp))
                       {
#ifdef                     CHPROG_DRIVER_DEBUG
                               do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Fail to send the CA section data\n");
#endif
 			  }


		  }
		  
		  CHCA_SectionDataAccess_UnLock();	  /*delete this on 050115*/
	}
}
#endif


/*******************************************************************************
 *Function name:  ChCheckMgFilterDelay
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *          boolean   ChCheckMgFilterDelay(void)

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
 *     timeout reason:
 *                                          MGDDISrcDmxNoSection:   No sections at all have been loaded(valid or invalid)
 *     (section filtering only:)     MGDDISrcDmxSctInvalid:   invalid matching sections have been loaded and rejected  
 *     (table filtering only:)        MGDDISrcDmxTblInvalid:   no correct sections have been loaded but invalid sections 
 *                                                   have been encountered
 *     (table filtering only:)        MGDDISrcDmxTblInvalidPart:at on least one correct section has been loaded but invalid   
 *                                                      sections have been encountered.
 *     (table filtering only:)        MGDDISrcDmxTblNotCompleted: at least one correct section has been loaded and no invalid
 *                                                           sections have been encountered.
 *   5516_flatform
 *******************************************************************************/
void  CHCA_MgFilterDelayCheck(void)
{
        /*boolean                         MgFilterTimeout = false;*/
	 int                                    iIndex;	

        CHCA_TICKTIME               cTmDelay;
        CHCA_ULONG                   TempD;
		
        for(iIndex=0;iIndex<CHCA_MAX_NUM_FILTER;iIndex++)
        {
              CHCA_SectionDataAccess_Lock();  
		if((FDmxFlterInfo [ iIndex ].bdmxactived)&&(FDmxFlterInfo [ iIndex ].TTriggerMode)&&(FDmxFlterInfo [ iIndex ].uTmDelay))
              {
                     TempD = 0.001*FDmxFlterInfo [ iIndex ].uTmDelay; 
					 
                     /*cTmDelay = (CHCA_TICKTIME)(CHCA_GET_TICKPERMS*FDmxFlterInfo [ iIndex ].uTmDelay);*/
                     cTmDelay = (CHCA_TICKTIME)((CHCA_ULONG)ST_GetClocksPerSecond()*TempD);
					 

			if(CHCA_GetDiffClock(CHCA_GetCurrentClock(),FDmxFlterInfo [ iIndex ].CStartTime)>=cTmDelay)	
			{
                            TMGDDIEventCode                                 iEvCode;
				TMGDDIEvSrcDmxFltrTimeoutExCode      iExCode;

				iEvCode = MGDDIEvSrcDmxFltrTimeout;
				FDmxFlterInfo [ iIndex ].bdmxactived = false;
                            /*FDmxFlterInfo [ iIndex ].CStartTime = CHCA_GetCurrentClock();*/

 #if   0
                            /*judeg the reason of timeout */
				if(!SectionFilter[iIndex].FirstSearched)
				{
				         if(!FDmxFlterInfo[iIndex].ValidSection)
                                     {
                                          iExCode = MGDDISrcDmxNoSection;
				         }
					  else
					  {
                                          if(SectionFilter[iIndex].MulSection)
                                          {
                                                 iExCode = MGDDISrcDmxTblInvalid;
						}
						else
						{
                                                 iExCode = MGDDISrcDmxSctInvalid;
						} 
					  }
						 
				}
				else
				{
                                    if(FDmxFlterInfo[iIndex].ValidSection)
                                    {
                                          iExCode = MGDDISrcDmxTblInvalidPart;
					 }
					 else
					 {
                                          iExCode = MGDDISrcDmxTblNotCompleted; 
                               
					 }
				}
#endif

                            iExCode = MGDDISrcDmxNoSection;
 				CHCA_SendFltrTimeout2CAT(iIndex,false,iExCode,iEvCode); 		    
				
#ifdef                  CHDEMUX_DRIVER_DEBUG
                            do_report(severity_info,"\n FilterIndex[%d] iExCode[%d]time out[%d] \n",iIndex,iExCode,cTmDelay);
#endif
			}
		}
		CHCA_SectionDataAccess_UnLock();
		
	 }
	 	
}


#if  1
/*******************************************************************************
 *Function name:  ChDVBCATSectionFilterMonitor
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          static void  ChDVBCATSectionFilterMonitor(void *pvParam )
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
 *     5516_platform
 * 
 * 
 *******************************************************************************/
static void  ChDVBCATSectionFilterMonitor(void *pvParam )
{
	  ST_ErrorCode_t                           ErrCode;
	  STPTI_Buffer_t                            BufferHandleTemp;
	  STPTI_Filter_t                              MatchedFilterHandle[16];
	  U32                                            MatchedFilterCount=0;
	  S32                                            FilterIndexTemp;
	  S32                                            i;
	  clock_t                                       TimeTemp;	
	  BOOL                                         SecCRCTemp = false;
	  CH6_FILTER_STATUS_t                FilterStatus;
	  U32                                            WaitTimeoutMs = 1;
	  
	  
	  U8*                                            pSecDataTemp;
	  U32                                            SecLenRead=0;

	  CHCA_UINT                                iSectionLength=0;

	  
	  TimeTemp=(clock_t)ST_GetClocksPerSecond()/2;
	  pSecDataTemp=(U8*)memory_allocate(CHSysPartition ,ONE_KILO_SECTION);
	  
	  while(true)
	  { 

#if            0
                 if(CHCA_GetMatchSectionInfo(&BufferHandleTemp))
                {
                        task_delay(3000);
			   do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			
		          continue;
		  }
#endif		

#if 1
	         CHCA_MgFilterDelayCheck();/*add this on 050521*/
				
                /*wait the ca section received*/
          	  ErrCode = STPTI_SignalWaitBuffer(MgCATSignalHandle,&BufferHandleTemp,WaitTimeoutMs); 
#else				
          	  ErrCode = STPTI_SignalWaitBuffer(MgCASignalHandle,&BufferHandleTemp,STPTI_TIMEOUT_INFINITY); 
#endif


                if(ErrCode!=ST_NO_ERROR)
		 {
			   /*do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			*/
		          continue;
		  }		 


                ErrCode = STPTI_BufferReadSection(BufferHandleTemp,
		                                                            MatchedFilterHandle,
		  	                                                      16,
		  	                                                     &MatchedFilterCount,
			                                                     &SecCRCTemp,
			                                                     (U8 *)pSecDataTemp,
			                                                     ONE_KILO_SECTION,
			                                                     NULL,
			                                                     0,
			                                                     &SecLenRead,
			                                                      /*20060301 change*/STPTI_COPY_TRANSFER_BY_DMA/*STPTI_COPY_TRANSFER_BY_MEMCPY*/);

#if 0
		  if(CHCA_CopySection2Buffer(BufferHandleTemp,
   	                                                        MatchedFilterHandle,
		  	                                          16,
		  	                                          &MatchedFilterCount,
			                                          &SecCRCTemp,
			                                          (U8 *)pSecDataTemp,
			                                          FOUR_KILO_SECTION,
			                                          &SecLenRead)==true)
		  {
		        do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] copy data err\n");
                      continue;
		  }
#endif

                if(ErrCode!=ST_NO_ERROR)      
                {
		         do_report(severity_info,"\n[CAT ChDVBCASectionFilterMonitor==>] copy data err\n");
		        /*zxg 20060314 ad*/
   		           STPTI_BufferFlush(BufferHandleTemp);
   		          /****************/
                       continue;
		  }
		  
		  CHCA_SectionDataAccess_Lock();  /*delete this on 050115*/
		  for(i=0;i<MatchedFilterCount;i++)
		  {
                       /*FilterIndexTemp = CHCA_FindMatchFilter(MatchedFilterHandle,i);*/

  			  FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_FILTERHANDLE,(void*)&MatchedFilterHandle[i]);		   
					   
			  if((FilterIndexTemp==-1) || (pSecDataTemp==NULL)) 
			  {
			       continue;
			  }	   

			  /*if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_DONE) 
			  {
                            continue;
			  }*/
			  
			  if(SecCRCTemp==false)
			  {
#ifdef                    CHPROG_DRIVER_DEBUG
				  STTBX_Print(("Section Monitor:CRC wrong in Filter index=%d\n",FilterIndexTemp));
#endif
       			  continue;
				  
			  }

			   /*if(!FDmxFlterInfo [ FilterIndexTemp ].Lock) 
                        {*/
                             if(ChInsertMgData(FilterIndexTemp,(CHCA_UCHAR*)pSecDataTemp))
                             /*if(ChInsertMgData(FilterIndexTemp,(CHCA_UCHAR*)pSecDataTemp))*/
                             {
#ifdef                          CHPROG_DRIVER_DEBUG
                                    do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Fail to send the EMM section data\n");
#endif
                             }
		#if 1/*060113 xjp add for test*/
				else
				{
					do_report(severity_info,"\n[ChDVBCATSectionFilterMonitor==>] Succeed in capturing the CAT section data\n");
				}
		#endif
			  }
			   
		  /*}*/
		  
		  CHCA_SectionDataAccess_UnLock();	 /* delete this on 050115*/
	}
}
#endif

/*******************************************************************************
 *Function name:  ChDVBEMMSectionFilterMonitor
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          static void  ChDVBEMMSectionFilterMonitor(void *pvParam )
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
 *     5516_platform
 * 
 * 
 *******************************************************************************/
#if 1 /*add this on 050115*/
static void  ChDVBEMMSectionFilterMonitor(void *pvParam )
{
	  ST_ErrorCode_t                           ErrCode;
	  STPTI_Buffer_t                            BufferHandleTemp;
	  STPTI_Filter_t                              MatchedFilterHandle[16];
	  U32                                            MatchedFilterCount=0;
	  S32                                            FilterIndexTemp;
	  S32                                            i;
	  clock_t                                       TimeTemp;	
	  BOOL                                         SecCRCTemp = false;
	  CH6_FILTER_STATUS_t                FilterStatus;
	  U32                                            WaitTimeoutMs = 100;
	  
	  
	  U8*                                            pSecDataTemp;
	  U32                                            SecLenRead=0;

	  CHCA_UINT                                iSectionLength=0;

	  
	  TimeTemp=(clock_t)ST_GetClocksPerSecond()/2;
	  pSecDataTemp=(U8*)memory_allocate(/*CHSysPartition 20070711 chanhe*/SystemPartition ,ONE_KILO_SECTION);
	  
	  while(true)
	  { 

#if 0
                 if(CHCA_GetMatchSectionInfo(&BufferHandleTemp))
                {
                        task_delay(3000);
			   do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			
		          continue;
		  }
#endif		

#if 1
	        /*CHCA_MgFilterDelayCheck();*/
				
                /*wait the ca section received*/
          	  ErrCode = STPTI_SignalWaitBuffer(MgEMMSignalHandle,&BufferHandleTemp,WaitTimeoutMs); 
#else				
          	  ErrCode = STPTI_SignalWaitBuffer(MgCASignalHandle,&BufferHandleTemp,STPTI_TIMEOUT_INFINITY); 
#endif


                if(ErrCode!=ST_NO_ERROR)
		 {
			   /*do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			*/
			  /* STPTI_BufferFlush(BufferHandleTemp); 20060714 del*/
		          continue;
		  }		 


                ErrCode = STPTI_BufferReadSection(BufferHandleTemp,
		                                                            MatchedFilterHandle,
		  	                                                      16,
		  	                                                     &MatchedFilterCount,
			                                                     &SecCRCTemp,
			                                                     (U8 *)pSecDataTemp,
			                                                     ONE_KILO_SECTION,
			                                                     NULL,
			                                                     0,
			                                                     &SecLenRead,
			                                                     /*STPTI_COPY_TRANSFER_BY_DMA*/STPTI_COPY_TRANSFER_BY_MEMCPY);

#if 0
		  if(CHCA_CopySection2Buffer(BufferHandleTemp,
   	                                                        MatchedFilterHandle,
		  	                                          16,
		  	                                          &MatchedFilterCount,
			                                          &SecCRCTemp,
			                                          (U8 *)pSecDataTemp,
			                                          FOUR_KILO_SECTION,
			                                          &SecLenRead)==true)
		  {
		        do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] copy data err\n");
                      continue;
		  }
#endif

                if(ErrCode!=ST_NO_ERROR||SecCRCTemp==false)      
   		 {
                        do_report(severity_info,"\n[EMM ChDVBCASectionFilterMonitor==>] copy data err\n");
   	                 /*zxg 20060314 ad*/
   		            STPTI_BufferFlush(BufferHandleTemp);
   		           /****************/
   			    continue;
		  }
		  
		  /*CHCA_SectionDataAccess_Lock();*/  /*delete this on 050115*/
		  
		  for(i=0;i<MatchedFilterCount;i++)
		  {
                       /*FilterIndexTemp = CHCA_FindMatchFilter(MatchedFilterHandle,i);*/

  			  FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_FILTERHANDLE,(void*)&MatchedFilterHandle[i]);		   
					   
			  if((FilterIndexTemp==-1) || (pSecDataTemp==NULL)) 
			  {
			       continue;
			  }	   

			  /*if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_DONE) 
			  {
                            continue;
			  }*/
			  
#if  0       
                       iSectionLength = (( pSecDataTemp [ 1 ] & 0x0f ) << 8 ) | pSecDataTemp [ 2 ] + 3;
      
                       do_report(severity_info,"\nEMM DATA Filter:");		  
                       for(i=0;i<iSectionLength;i++)
                             do_report(severity_info,"%4x",*(pSecDataTemp+i));	
                       do_report(severity_info,"\n");	
#endif		


#if   1  /*delete  this on 050513 just for test*/
			  if(ChInsertMgEmmBuffer(FilterIndexTemp,(CHCA_UCHAR*)pSecDataTemp))
                       /*if(ChInsertMgData(FilterIndexTemp,(CHCA_UCHAR*)pSecDataTemp))*/
                       {
#ifdef                     CHPROG_DRIVER_DEBUG
                               do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Fail to send the EMM section data\n");
#endif
 			  }
	#ifdef SIMPLE_CA_TRACE/*060113 xjp add for test*/
			else
			{
				do_report(severity_info,"\n[ChDVBEMMSectionFilterMonitor==>] Succeed in capturing the EMM section data\n");
			}
	#endif
#endif			  
		  }
		  
		  /*CHCA_SectionDataAccess_UnLock();	*/  /*delete this on 050115*/
	}
}
#endif

/*******************************************************************************
 *Function name:  ChDVBCASectionFilterMonitor
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          static void  ChDVBCASectionFilterMonitor(void *pvParam )
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
 *     5516platform
 * 
 * 
 *******************************************************************************/
static void  ChDVBCASectionFilterMonitor(void *pvParam )
{
	  ST_ErrorCode_t                           ErrCode;
	  STPTI_Buffer_t                            BufferHandleTemp;
	  STPTI_Filter_t                              MatchedFilterHandle[16];
	  U32                                            MatchedFilterCount=0;
	  S32                                            FilterIndexTemp;
	  S32                                            i;
	  clock_t                                       TimeTemp;	
	  BOOL                                         SecCRCTemp = false;
	  CH6_FILTER_STATUS_t                FilterStatus;
	  U32                                            WaitTimeoutMs = 1;
	  
	  
	  U8*                                            pSecDataTemp;
	  U32                                            SecLenRead=0;

	  
	  TimeTemp=(clock_t)ST_GetClocksPerSecond()/2;
	  pSecDataTemp=(U8*)memory_allocate(/*CHSysPartition 20070711 chanhe*/SystemPartition ,ONE_KILO_SECTION);
	  
	  while(true)
	  { 
#if   0
                 if(CHCA_GetMatchSectionInfo(&BufferHandleTemp))
                {
                        task_delay(3000);
			   do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			
		          continue;
		  }
#endif		

#if 1
	        /*CHCA_MgFilterDelayCheck();*/
				
                /*wait the ca section received*/
          	  ErrCode = STPTI_SignalWaitBuffer(MgCASignalHandle,&BufferHandleTemp,WaitTimeoutMs); 
#else				
          	  ErrCode = STPTI_SignalWaitBuffer(MgCASignalHandle,&BufferHandleTemp,STPTI_TIMEOUT_INFINITY); 
#endif


                if(ErrCode!=ST_NO_ERROR)
		 {
			   /*do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>]Receiver signal timeout\n");			*/
			   /*STPTI_BufferFlush(BufferHandleTemp); 20060714 del*/
		          continue;
		  }		 


                ErrCode = STPTI_BufferReadSection(BufferHandleTemp,
		                                                            MatchedFilterHandle,
		  	                                                      16,
		  	                                                     &MatchedFilterCount,
			                                                     &SecCRCTemp,
			                                                     (U8 *)pSecDataTemp,
			                                                     ONE_KILO_SECTION,
			                                                     NULL,
			                                                     0,
			                                                     &SecLenRead,
			                                                    /* STPTI_COPY_TRANSFER_BY_DMA*/STPTI_COPY_TRANSFER_BY_MEMCPY);

#if 0
		  if(CHCA_CopySection2Buffer(BufferHandleTemp,
   	                                                        MatchedFilterHandle,
		  	                                          16,
		  	                                          &MatchedFilterCount,
			                                          &SecCRCTemp,
			                                          (U8 *)pSecDataTemp,
			                                          FOUR_KILO_SECTION,
			                                          &SecLenRead)==true)
		  {
		        do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] copy data err\n");
                      continue;
		  }
#endif

                 if(ErrCode!=ST_NO_ERROR||SecCRCTemp==false)      
		                 {
				        do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] copy data err\n");
				    	/*zxg 20060314 ad*/
				    	  STPTI_BufferFlush(BufferHandleTemp);
				    	/****************/
		                      continue;
				   }
		  
		  CHCA_SectionDataAccess_Lock();/*add this on 050115*/
		  for(i=0;i<MatchedFilterCount;i++)
		  {
                       /*FilterIndexTemp = CHCA_FindMatchFilter(MatchedFilterHandle,i);*/

  			  FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_FILTERHANDLE,(void*)&MatchedFilterHandle[i]);		   
					   
			  if(FilterIndexTemp==-1) 
			  {
			       continue;
			  }	   

			  /*if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_DONE) 
			  {
                            continue;
			  }*/


                        /*if(!FDmxFlterInfo [ FilterIndexTemp ].Lock) 
                        {*/
                               /*CH6_Stop_Filter(FilterIndexTemp);
                               FDmxFlterInfo [ FilterIndexTemp ].bdmxactived =  false;*/
                               
                               if(ChInsertMgData(FilterIndexTemp,(CHCA_UCHAR*)pSecDataTemp))
                               {
#ifdef                           CHPROG_DRIVER_DEBUG
                                     do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Fail to send the CA section data\n");
#endif
                               }
		#if 0/*060113 xjp add for test*/
				else
				{
					do_report(severity_info,"\n[ChDVBCASectionFilterMonitor==>] Succeed in capturing the ECM or CAT section data\n");
				}
		#endif
			  }

			
		  /*}*/
		  CHCA_SectionDataAccess_UnLock();	

	}
}


/*******************************************************************************
 *Function name:  CHCA_DemuxInit
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          BOOL  CHCA_DemuxInit( void )
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
 *     5516 platform
 * 
 * 
 *******************************************************************************/
BOOL  CHCA_DemuxInit ( void )
{
      CHCA_USHORT             FltrIndex;


	       /*control the semaphore of the section data struct access*/
	//CH6_CAInterface_SemaphoreControl(false);
		   
	     /*Creat the signal handle for the mg ca section receiver task*/
	  if(STPTI_SignalAllocate(PTIHandle,&MgCASignalHandle) != ST_NO_ERROR)
		  {
			  do_report(severity_info,"\n[CHCA_DemuxInit==>]Create the ca signal handle wrong\n");
			  return true;
		  }
		  
#if 1 /*add this on 050115*/
		  /*Creat the signal handle for the mg emm section receiver task*/
		  if(STPTI_SignalAllocate(PTIHandle,&MgEMMSignalHandle) != ST_NO_ERROR)
		  {
              do_report(severity_info,"\n[CHCA_DemuxInit==>]Create the emm signal handle wrong\n");
              return true;
		  }
#endif	


	psemMgDescAccess = semaphore_create_fifo ( 1 );  /*add this on 040816*/

	pSemMgApiAccess = semaphore_create_fifo(1);   /*add this on 040816*/

		  
		  /*init the emm queue*/
		  /*CHCA_EmmQueueInit();*/
		  
		  /*CHCAPTIHandle = PTIHandle;  PTI HANDEL*/ 
		  
		  /*init the section data structure of the  ch mg ca*/
		  for(FltrIndex=0;FltrIndex<CHCA_MAX_NUM_FILTER;FltrIndex++)
		  {
              FDmxFlterInfo [ FltrIndex ].SlotId = CHCA_ILLEGAL_CHANNEL; 
              FDmxFlterInfo [ FltrIndex ].TTriggerMode = false;
              FDmxFlterInfo [ FltrIndex ].bdmxactived = false;	 
              FDmxFlterInfo [ FltrIndex ].uTmDelay = 0;
              FDmxFlterInfo [ FltrIndex ].InUsed = false; 
              FDmxFlterInfo [ FltrIndex ].cMgDataNotify = NULL;
              FDmxFlterInfo [ FltrIndex ].FilterType = OTHER_FILTER; /*add this on 050313*/
		  }
		  
		  /*init the ca buffer */
		  memset(CatSectionBuffer,0,1024);
		  memset(PmtSectionBuffer,0,1024);
		  /*memset(EcmSectionBuffer,0,1024);delete this on 050327*/
		  
   
        /*add this on 041129*/
        psemSectionEmmQueueAccess = semaphore_create_fifo ( 1 );  /*add this on 030305*/
	 CHCA_InitMgEmmQueue();	
	 MgEmmFilterData.OnUse = false;
	 memset(MgEmmFilterData.aucMgSectionData,0,MGCA_PRIVATE_TABLE_MAXSIZE);

#if  0 /*add this on 050424*/
		  /*Creat the signal handle for the mg emm section receiver task*/
		  if(STPTI_SignalAllocate(PTIHandle,&MgPMTSignalHandle) != ST_NO_ERROR)
		  {
              do_report(severity_info,"\n[CHCA_DemuxInit==>]Create the emm signal handle wrong\n");
              return true;
		  }
#endif	
		  
		  
#if  1 /*add this on 050424*//*060117 xjp comment*/
		  /*Creat the signal handle for the mg cat section receiver task*/
		  if(STPTI_SignalAllocate(PTIHandle,&MgCATSignalHandle) != ST_NO_ERROR)
		  {
              do_report(severity_info,"\n[CHCA_DemuxInit==>]Create the cat signal handle wrong\n");
              return true;
		  }
#endif	
		  
		  
		  
#if  0	/*add this on 050424*/	
       /*create the mg ca data receive task */		
       if ( ( ptidPMTSecFilterMonitorTask = Task_Create ( ChDVBPMTSectionFilterMonitor,
			                                                               NULL,
			                                                               PMTSECFILTER_PROCESS_WORKSPACE,
			                                                               PMTSECFILTER_PROCESS_PRIORITY,
			                                                               "ChDVBPMTSectionFilterMonitor",
			                                                               0 ) ) == NULL )
      {
	      do_report ( severity_info,"DVB CA Section Filter  Mointor Create Error !\n" );
	      return true;
      }

#endif      /*create the mg ca data receive task */	

#if 1
       /*create the mg ca data receive task */		
       if ( ( ptidCATSecFilterMonitorTask = Task_Create ( ChDVBCATSectionFilterMonitor,
			                                                               NULL,
			                                                               CATSECFILTER_PROCESS_WORKSPACE,
			                                                               CATSECFILTER_PROCESS_PRIORITY,
			                                                               "ChDVBCATSectionFilterMonitor",
			                                                               0 ) ) == NULL )
      {
	      do_report ( severity_info,"DVB CA Section Filter  Mointor Create Error !\n" );
	      return true;
      }
		
#else

	{		
		int i_Error=-1;


		g_CATSecFilterStack = memory_allocate( SystemPartition, CATSECFILTER_PROCESS_WORKSPACE );
		if( NULL == g_CATSecFilterStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBCATSectionFilterMonitor for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))ChDVBCATSectionFilterMonitor, 
										NULL, 
										g_CATSecFilterStack, 
										CATSECFILTER_PROCESS_WORKSPACE, 
										&ptidCATSecFilterMonitorTask, 
										&g_CATSecFilterTaskDesc, 
										CATSECFILTER_PROCESS_PRIORITY, 
										"ChDVBCATSectionFilterMonitor", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBCATSectionFilterMonitor for task init\n" );
			return  true;	
		}


	}

#endif

 #if 1/*060117 xjp change for STi5105 SRAM is small*/
       if ( ( ptidCASecFilterMonitorTask = Task_Create ( ChDVBCASectionFilterMonitor,
			                                                               NULL,
			                                                               CASECFILTER_PROCESS_WORKSPACE,
			                                                               CASECFILTER_PROCESS_PRIORITY,
			                                                               "ChDVBCASectionFilterMonitor",
			                                                               0 ) ) == NULL )
      {
	      do_report ( severity_info,"DVB CA Section Filter  Mointor Create Error !\n" );
	      return true;
      }
#else
	
	{		
		int i_Error=-1;


		g_CASecFilterStack = memory_allocate( SystemPartition, CASECFILTER_PROCESS_WORKSPACE );
		if( NULL == g_CASecFilterStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBCASectionFilterMonitor for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))ChDVBCASectionFilterMonitor, 
										NULL, 
										g_CASecFilterStack, 
										CASECFILTER_PROCESS_WORKSPACE, 
										&ptidCASecFilterMonitorTask, 
										&g_CASecFilterTaskDesc, 
										CASECFILTER_PROCESS_PRIORITY, 
										"ChDVBCASectionFilterMonitor", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBCASectionFilterMonitor for task init\n" );
			return  true;	
		}


	}
	
#endif


#if 	1/*060117 xjp change for STi5105 SRAM is small*/
       if ( ( ptidEMMSecFilterMonitorTask = Task_Create ( ChDVBEMMSectionFilterMonitor,
			                                                               NULL,
			                                                               EMMSECFILTER_PROCESS_WORKSPACE,
			                                                               EMMSECFILTER_PROCESS_PRIORITY,
			                                                               "ChDVBEMMSectionFilterMonitor",
			                                                               0 ) ) == NULL )
      {
	      do_report ( severity_info,"DVB EMM Section Filter  Mointor Create Error !\n" );
	      return true;
      }
	   
#else
  
	{		
		int i_Error=-1;


		g_EMMSecFilterStack = memory_allocate( SystemPartition, EMMSECFILTER_PROCESS_WORKSPACE );
		if( NULL == g_EMMSecFilterStack )
		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBEMMSectionFilterMonitor for no memory\n" );
			return  true;		
		}
		i_Error = task_init( (void(*)(void *))ChDVBEMMSectionFilterMonitor, 
										NULL, 
										g_EMMSecFilterStack, 
										EMMSECFILTER_PROCESS_WORKSPACE, 
										&ptidEMMSecFilterMonitorTask, 
										&g_EMMSecFilterTaskDesc, 
										EMMSECFILTER_PROCESS_PRIORITY, 
										"ChDVBEMMSectionFilterMonitor", 
										0 );

		if( -1 == i_Error )

		{
			do_report ( severity_error, "ChCardInit=> Unable to create ChDVBEMMSectionFilterMonitor for task init\n" );
			return  true;	
		}


	}
	
#endif
 


	 return false;	

}



/****************************************************************************************
 *Function name: CHCA_FlushCircularBuffer
 * 
 *
 *Description: flush the circular buffer data  
 *             
 *
 *Prototype:
 *          CHCA_BOOL  CHCA_FlushCircularBuffer(STPTI_Buffer_t   BufferHandle)
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
 *
 ****************************************************************************************/
void CHCA_SendNewTuneReq(SHORT sNewTransSlot, TRANSPONDER_INFO_STRUCT *pstTransInfoPassed)
{
     CH6_SendNewTuneReq(sNewTransSlot,pstTransInfoPassed);
}



/****************************************************************************************
  *INTERFACE FUNCTION
 ****************************************************************************************/
				  			 



 
/****************************************************************************************
 *Function name: CHCA_SectionData_Lock
 * 
 *
 *Description: lock the section data struct  
 *             
 *
 *Prototype:
 *          void CHCA_SectionData_Lock(void)
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
 *
 ****************************************************************************************/
void CHCA_SectionDataAccess_Lock(void)
{
     semaphore_wait(pSemSectStrucACcess);
}


/****************************************************************************************
 *Function name: CHCA_SectionData_UnLock
 * 
 *
 *Description: unlock the section data struct  
 *             
 *
 *Prototype:
 *          void CHCA_SectionData_UnLock(void)
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
 *
 ****************************************************************************************/
void CHCA_SectionDataAccess_UnLock(void)
{
     semaphore_signal(pSemSectStrucACcess);
}


 
/****************************************************************************************
 *Function name:   CHCA_Allocate_Section_Slot
 * 
 *
 *Description:  
 *             Allocate a slot used for getting section data      
 *
 *Prototype:
 *           CHCA_SHORT CHCA_Allocate_Section_Slot(STPTI_Signal_t SignalHandle)         
 *       
 *
 *input:
 *      STPTI_Signal_t SignalHandle:the signal object which will connected to the allocated slot 
 * 
 *
 *output:
 *
 *Return Value:
 *      -1:stand for no free slot,allocate fail
 *     other value:the index of the allocated slot
 *
 *Comments:
 *
 *
 *
 *
 ****************************************************************************************/
#if  1 /*modify this on 050115*/
CHCA_SHORT  CHCA_Allocate_Section_Slot(CHCA_BOOL    iEmmSection)
{
     switch(iEmmSection)
     {
            case 0:
                     return CH6_Allocate_Section_Slot(MgCASignalHandle);/*ecm*/
			break;

	     case 1:
		 	return CH6_Allocate_Section_Slot(MgEMMSignalHandle); /*emm*/ 
		 	break;

	     case 2:
                     return CH6_Allocate_Section_Slot(/*MgCASignalHandle */MgCATSignalHandle);/*cat*/
		 	break;
     
}
}
#else
CHCA_SHORT  CHCA_Allocate_Section_Slot(void)
{
      return CH6_Allocate_Section_Slot(MgCASignalHandle);
}
#endif

/****************************************************************************************
 *Function name:   CHCA_Set_Slot_Pid
 * 
 *
 *Description:  
 *             Set the Pid for a slot,but the PTI doesn't start capture data     
 *
 *Prototype:
 *          CHCA_BOOL CHCA_Set_Slot_Pid(CHCA_USHORT SlotIndex,CHCA_PTI_Pid_t Pid)
 *           
 *
 *input:
 *      CHCA_USHORT  SlotIndex:the index of the slot which will be set pid
 *      CHCA_PTI_Pid_t Pid:the value of the pid to be set 
 *
 *output:
 *
 *Return Value:
 *     TRUE:stand for the  function is unsuccessful operated
 *     FALSE:stand for the function is successful operated  
 *
 *Comments:
 *
 *
 *
 *
 ****************************************************************************************/
#if 1/*modify this on 050115*/
CHCA_BOOL CHCA_Set_Slot_Pid(CHCA_USHORT SlotIndex,CHCA_PTI_Pid_t Pid,CHCA_BOOL    iEmmSection)
#else
CHCA_BOOL CHCA_Set_Slot_Pid(CHCA_USHORT SlotIndex,CHCA_PTI_Pid_t Pid)
#endif
{
      return  CH6_Set_Slot_Pid(SlotIndex,Pid);   
}


/****************************************************************************************
 *Function name:   CHCA_Free_Section_Slot
 * 
 *
 *Description:  
 *             Free the section slot   
 *
 *Prototype:
 *          CHCA_BOOL CHCA_Free_Section_Slot(CHCA_USHORT SlotIndex)
 *           
 *
 *input:
 *      U16 SlotIndex:the index of the slot which will be freed
 *      
 *
 *output:
 *
 *Return Value:
 *     TRUE:stand for the  function is unsuccessful operated 
 *     FALSE:stand for the function is successful operated   
 *
 *Comments:
 *
 *
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Free_Section_Slot(CHCA_USHORT SlotIndex)
{
     return CH6_Free_Section_Slot(SlotIndex);
}



/****************************************************************************************
 *Function name:   CHCA_Start_Slot
 * 
 *
 *Description:  
 *             start the data captured operation for a slot  
 *
 *Prototype:
 *          CHCA_BOOL CHCA_Start_Slot(CHCA_USHORT   SlotIndex)
 *           
 *
 *input:
 *      U16 SlotIndex:the index of the slot which will be freeed
 *       
 *
 *output:
 *
 *Return Value:
 *     TRUE:stand for the  function is unsuccessful operated 
 *     FALSE:stand for the function is successful operated   
 *
 *Comments:
 *
 *
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Start_Slot(CHCA_USHORT   SlotIndex)
{
     return CH6_Start_Slot(SlotIndex);
}



/****************************************************************************************
 *Function name:   CHCA_Stop_Slot
 * 
 *
 *Description:  
 *             Stop the data captured operation for a slot  
 *
 *Prototype:
 *           CHCA_BOOL CHCA_Stop_Slot(CHCA_USHORT SlotIndex)
 *           
 *
 *input:
 *      CHCA_USHORT SlotIndex:the index of the slot which will be Stopped
 *       
 *
 *output:
 *
 *
 *Return Value:
 *     TRUE:stand for the  function is unsuccessful operated 
 *     FALSE:stand for the function is successful operated   
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Stop_Slot(CHCA_USHORT SlotIndex)
{
     return CH6_Stop_Slot(SlotIndex);
}



/****************************************************************************************
 *Function name:   CHCA_Allocate_Section_Filter
 * 
 *
 *Description:  
 *             Allocate a filter used for filtering section data
 *
 *Prototype:
 *           CHCA_SHORT  CHCA_Allocate_Section_Filter(void)
 *           
 *
 *input:
 *      sf_filter_mode_t FilterMode:  the mode of the filter,which may be 1K mode or 4K mode 
 *      app_t FilterApp:                   the filter sign,may be used for deciding the destination of the captured section data 
 *      BOOLEAN MatchMode:          the sign of section match mode
 *                                                TRUE:stand for equal match mode
 *                                                FALSE:stand for not equal match mode
 *output:
 *
 *
 *Return Value:
 *     -1:stand for no free filter,allocate fail
 *     other value:the index of the allocated filter
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_SHORT  CHCA_Allocate_Section_Filter(void)
{
        sf_filter_mode_t            FilterMode;
        app_t                           FilterApp; 
        BOOLEAN                     MatchMode,MulSection,IsAllocSectionMem;

        FilterMode = ONE_KILO_SECTION;
        FilterApp = CA_APP;
	 MatchMode = false;
	 MulSection = false;
	 IsAllocSectionMem = false;

/*051114 xjp*/
        return CH6_Allocate_Section_Filter(FilterMode,FilterApp,MatchMode,MulSection,IsAllocSectionMem,/*CHSysPartition 20070711 chanhe*/SystemPartition );
}


/****************************************************************************************
 *Function name:   CHCA_Free_Section_Filter
 * 
 *
 *Description:  
 *             Free a section filter
 *
 *Prototype:
 *          BOOL CHCA_Free_Section_Filter(U16 FilterIndex)
 *           
 *
 *input:
 *      U16 FilterIndex:the index of the filter which will be freed
 *
 *
 *output:
 *
 *
 *Return Value:
 *	TRUE:stand for the  function is unsuccessful operated 
 *	FALSE:stand for the function is successful operated  
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Free_Section_Filter(CHCA_USHORT   FilterIndex)
{
     return CH6_Free_Section_Filter(FilterIndex);

}



/****************************************************************************************
 *Function name:   CHCA_Associate_Slot_Filter
 * 
 *
 *Description:  
 *             Associate a slot with a filter
 *
 *Prototype:
 *          CHCA_BOOL CHCA_Associate_Slot_Filter(CHCA_USHORT SlotIndex,CHCA_USHORT  FilterIndex)
 *           
 *
 *input:
 *	 U16 SlotIndex:the index of the slot which will be associated
 *	 U16 FilterIndex:the index of the filter which will be associated
 *
 *
 *output:
 *
 *
 *Return Value:
 *	TRUE:stand for the  function is unsuccessful operated 
 *	FALSE:stand for the function is successful operated  
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Associate_Slot_Filter(CHCA_USHORT SlotIndex,CHCA_USHORT  FilterIndex)
{
      CHCA_BOOL     bReturnCode;

      bReturnCode = CH6_Associate_Slot_Filter(SlotIndex,FilterIndex);

      return bReturnCode; 	  
}




/****************************************************************************************
 *Function name:   CHCA_Set_Filter
 * 
 *
 *Description:  
 *             Set the filter data for a filter
 *
 *Prototype:
CHCA_BOOL CHCA_Set_Filter(CHCA_USHORT           FilterIndex,
                                                    CHCA_UCHAR*           pData,
                                                    CHCA_UCHAR*           pMask,
                                                    CHCA_UCHAR*           pPatern,
                                                    CHCA_INT                  DataLen,
                                                    CHCA_BOOL               MatchMode)
 *           
 *
 *input:
 *	U16 FilterIndex:the index of the filter which will be set data
 *	U8* pData:
 *	U8* pMask:
 *	U8* pPatern:
 *	int DataLen:the data length
 *	BOOLEAN MatchMode:the sign of section match mode
 *				TRUE:stand for equal match mode
 *				FALSE:stand for not equal match mode
 *
 *
 *output:
 *
 *
 *Return Value:
 *	TRUE:stand for the  function is unsuccessful operated 
 *	FALSE:stand for the function is successful operated  
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Set_Filter(CHCA_USHORT           FilterIndex,
                                                    CHCA_UCHAR*           pData,
                                                    CHCA_UCHAR*           pMask,
                                                    CHCA_UCHAR*           pPatern,
                                                    CHCA_INT                  DataLen,
                                                    CHCA_BOOL               MatchMode)
{
      return CH6_Set_Filter(FilterIndex,pData,pMask,pPatern,DataLen,MatchMode);
}




/****************************************************************************************
 *Function name:   CHCA_Start_Filter
 * 
 *
 *Description:  
 *             Enable the use of a filter  
 *
 *Prototype:
 *         CHCA_BOOL CHCA_Start_Filter(CHCA_USHORT  FilterIndex)
 *           
 *
 *input:
 *	U16 FilterIndex:the index of the filter which will be enabled
 *
 *output:
 *
 *
 *Return Value:
 *	TRUE:stand for the  function is unsuccessful operated 
 *	FALSE:stand for the function is successful operated  
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Start_Filter(CHCA_USHORT  FilterIndex)
{
       return CH6_Start_Filter(FilterIndex);
  
}



/****************************************************************************************
 *Function name:   CHCA_Stop_Filter
 * 
 *
 *Description:  
 *             Disable the use of a filter  
 *
 *Prototype:
 *          CHCA_BOOL CHCA_Stop_Filter(CHCA_USHORT  FilterIndex)
 *           
 *
 *input:
 *	U16 FilterIndex:the index of the filter which will be enabled
 *
 *output:
 *
 *
 *Return Value:
 *	TRUE:stand for the  function is unsuccessful operated 
 *	FALSE:stand for the function is successful operated  
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Stop_Filter(CHCA_USHORT  FilterIndex)
{
       return CH6_Stop_Filter(FilterIndex);
}


/****************************************************************************************
 *Function name:   CHCA_GetFreeFilter
 * 
 *
 *Description:  
 *             Get the free filter count  
 *
 *Prototype:
 *        CHCA_INT   CHCA_GetFreeFilter(void)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	the number of the free filter
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_INT   CHCA_GetFreeFilter(void)
{
       return CH6_GetFreeFilterCount();
}



/*******************************************************************************
 *Function name:  CHCA_MgDataNotifyRegister
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *         CHCA_BOOL CHCA_MgDataNotifyRegister(CHCA_USHORT iFliter,CHCA_CALLBACK_FN  hCallback)  
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
CHCA_BOOL CHCA_MgDataNotifyRegister(CHCA_USHORT iFliter,CHCA_CALLBACK_FN  hCallback)  
{
        if((iFliter<0) ||(iFliter>=CHCA_MAX_NUM_FILTER)||(hCallback==NULL)) 
        {
#ifdef    CHDEMUX_DRIVER_DEBUG
              do_report(severity_info,"\n[ChMgDataNotifyRegister::] filter unknow\n");
#endif
              return true;
	 }


	 FDmxFlterInfo[iFliter].cMgDataNotify = hCallback; 

       return false;    
}


/*******************************************************************************
 *Function name:  CHCA_MgDataNotifyUnRegister
 * 
 *
 *Description: 
 *                  
 *
 *Prototype:
 *          CHCA_BOOL  CHCA_MgDataNotifyUnRegister(CHCA_USHORT  iFliter)  
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
CHCA_BOOL  CHCA_MgDataNotifyUnRegister(CHCA_USHORT  iFliter)  
{
        if((iFliter<0) ||(iFliter>=CHCA_MAX_NUM_FILTER)) 
        {
#ifdef    CHDEMUX_DRIVER_DEBUG
              do_report(severity_info,"\n[ChMgDataNotifyUnRegister::] filter unknow\n");
#endif
              return true;
	 }
		

       if( FDmxFlterInfo[iFliter].cMgDataNotify!=NULL)
	{
	     FDmxFlterInfo[iFliter].cMgDataNotify = (void *)NULL; 
       }	
 
	 /*stSectionFilter [ iFliter ].DmxCallback = (void *)NULL;*/

        return false;   

}


#if 0
/****************************************************************************************
 *Function name: CHCA_FlushCircularBuffer
 * 
 *
 *Description: flush the circular buffer data  
 *             
 *
 *Prototype:
 *          CHCA_BOOL  CHCA_FlushCircularBuffer(STPTI_Buffer_t   BufferHandle)
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
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_FlushCircularBuffer(STPTI_Buffer_t   BufferHandle)
{
      ST_ErrorCode_t       Error;  
      CHCA_BOOL            bReturnCode=false;
  
      Error = STPTI_BufferFlush(BufferHandle);

      if(Error==ST_NO_ERROR)
	   bReturnCode = true;

      return bReturnCode;	  
}


/*******************************************************************************
 *Function name:  CHCA_GetMatchSectionInfo
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          BOOL  CHCA_GetMatchSectionInfo(void    BufferHandle)
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
CHCA_BOOL  CHCA_GetMatchSectionInfo(STPTI_Buffer_t*   BufferHandle)
{
       CHCA_BOOL                                bReturnCode = false;
	ST_ErrorCode_t                           StErrCode;   

	/*STPTI_Buffer_t                         BufferHandleTemp;*/
	/*BufferHandleTemp = BufferHandle;*/

	StErrCode = STPTI_SignalWaitBuffer(MgCASignalHandle,BufferHandle,STPTI_TIMEOUT_INFINITY); 

#ifdef      CHPROG_DRIVER_DEBUG
	do_report(severity_info,"[CHCA_GetMatchSectionInfo::]STPTI_SignalWaitBuffer()=%s",GetErrorText(StErrCode));
#endif

	 if(StErrCode!=ST_NO_ERROR) 
	 {
              bReturnCode = true;
	 }

	 return bReturnCode;

}


/*******************************************************************************
 *Function name:  CHCA_CopySection2Buffer
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          BOOL  CHCA_CopySection2Buffer(void       BufferHandle,
 *	                                                         U32       MatchedFilterList[],
 *	                                                         U32       MaxLengthofFilterList,
 *	                                                         U32       *NumOfFilterMatches_p,
 *	                                                         BOOL    *CRCValid_p,
 *	                                                         U8        *Destination_p,
 *                                                             U32      DestinationSize,
 *                                                             U32      *DataSize_p)
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
BOOL  CHCA_CopySection2Buffer(STPTI_Buffer_t       BufferHandle,
	                                                         U32       MatchedFilterList[],
	                                                         U32       MaxLengthofFilterList,
	                                                         U32       *NumOfFilterMatches_p,
	                                                         BOOL    *CRCValid_p,
	                                                         U8        *Destination_p,
                                                                U32      DestinationSize,
                                                                U32      *DataSize_p)
{
       BOOL                                          bReturnCode = false;
	ST_ErrorCode_t                           StErrCode;   

       STPTI_Buffer_t                             BufferHandleTemp;

	BufferHandleTemp = (STPTI_Buffer_t)BufferHandle;

       StErrCode = STPTI_BufferReadSection(BufferHandleTemp,
		                                                      MatchedFilterList,
		  	                                               16,
		  	                                               NumOfFilterMatches_p,
			                                               CRCValid_p,
			                                               Destination_p,
			                                               DestinationSize,
			                                               NULL,
			                                               0,
			                                               DataSize_p,
			                                               STPTI_COPY_TRANSFER_BY_MEMCPY);

        if(StErrCode!=ST_NO_ERROR) 
	 {
              bReturnCode = true;
	 }

#ifdef      CHPROG_DRIVER_DEBUG
	 STTBX_Print(("STPTI_BufferReadSection()=%s SecLen=%d\n",GetErrorText(StErrCode),*DataSize_p));
#endif		
   
	 return bReturnCode;

}


/*******************************************************************************
 *Function name:  CHCA_FindMatchFilter
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *          S32 CHCA_FindMatchFilter(U32  PMatchValue[],U32  iFilterIndex)
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
S32 CHCA_FindMatchFilter(U32  PMatchValue[],U32  iFilterIndex)
{
      if(iFilterIndex>=16)
	  return -1;
	  
      return CH6_FindMatchFilter(FILTER_MATCH_BY_FILTERHANDLE,(void*)&PMatchValue[iFilterIndex]);

}

/*******************************************************************************
 *Function name:  CHCA_SectionDataSearchEnd
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *        BOOL CHCA_SectionDataSearchEnd(U32 FilterIndex,U8 *DesDataBuffer)  
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
BOOL CHCA_SectionDataSearchEnd(U32 FilterIndex,U8 *DesDataBuffer)
{
      BOOL    bReturnCode;

      bReturnCode = (BOOL)CH6_HaveCompleteSearch(FilterIndex,(BYTE*)DesDataBuffer);

      return bReturnCode;	  
}
#endif 


/*******************************************************************************
 *Function name:  CHCA_CheckMultiFilter
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *        CHCA_BOOL   CHCA_CheckMultiFilter(CHCA_SHORT   SlotIndex,CHCA_SHORT*  FilterCount)
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *        true:   the slot associated multiple filter
 *        false:  the slot associated one filter
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL   CHCA_CheckMultiFilter(CHCA_SHORT   SlotIndex,CHCA_USHORT*  FilterCount)
{
        CHCA_BOOL        bErrCode=false;

        bErrCode = CH6_Get_SlotAssociateFilter_Count(SlotIndex,FilterCount);

        return bErrCode;
}


/*******************************************************************************
 *Function name:  CHCA_PtiQueryPid
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *        CHCA_BOOL  CHCA_PtiQueryPid ( CHCA_SHORT* SlotIndex,CHCA_PTI_Pid_t  Pid,CHCA_BOOL*  bSlotAllocated )
 *       
 *
 *input:
 *      
 *output:
 *
 *
 *Return Value:
 *        
 *        
 *
 *Comments:
 *       -1:stand for the specified pid hasn't been collected by any slot  
 *	   -2:FALSE:stand for the specified pid is an invalid value
 *	   other:stand for the slot index collecting the specified pid
 * 
 * 
 *******************************************************************************/
CHCA_BOOL  CHCA_PtiQueryPid ( CHCA_SHORT* SlotIndex,CHCA_PTI_Pid_t  Pid,CHCA_BOOL*  bSlotAllocated )
{
      CHCA_BOOL       bErrCode = false;  	  

      *SlotIndex =  CH6_FindSlotByPID(Pid);

      if(*SlotIndex==-2)
      {
             bErrCode = true;
      }
      else if(*SlotIndex==-1)
      {
             *bSlotAllocated = false;
      }
      else
      {
             *bSlotAllocated = true;
      }
	
      return bErrCode;	  
}



/*******************************************************************************
 *Function name:  CHCA_StartDemuxData
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *        CHCA_BOOL   CHCA_StartDemuxData(CHCA_SHORT   SlotIndex,CHCA_SHORT*  FilterCount)
 *       
 *
 *input:
 *        CHCA_USHORT  FilterIndex : the index of the filter
 * 
 *
 *output:
 *
 *Return Value:
 *        true:   interface execution error
 *        false:  start demuxdata receive successfully
 *
 *Comments:
 *     
 * 
 * 
 *******************************************************************************/
CHCA_BOOL   CHCA_StartDemuxFilter(CHCA_USHORT  FilterIndex)
{
        CHCA_BOOL        bErrCode = false;

        if(FilterIndex>=CHCA_MAX_NUM_FILTER || (FDmxFlterInfo [ FilterIndex ].SlotId==CHCA_ILLEGAL_CHANNEL))
        {
              do_report(severity_info,"\n[CHCA_StartDemuxFilter==>] Filterid is  wrong or slotid is  illegal\n");
              bErrCode = true;
		return bErrCode;  	  
 	 }

         CH6_Start_Slot(FDmxFlterInfo [ FilterIndex ].SlotId);

         /*start the filter of the demux*/	
         if(CH6_Start_Filter(FilterIndex)==false)
         {
#ifdef     CHDEMUX_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[CHCA_StartDemuxFilter==>] the filterhandle[%d] has been started\n",
                                __FILE__,
                                __LINE__,
                                FilterIndex);
#endif
               FDmxFlterInfo [ FilterIndex ].bdmxactived =  true;	/*add this on 041118*/
	  }
	  else
	  {
                bErrCode = true;
#ifdef      CHDEMUX_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHCA_StartDemuxFilter==>] The filter handle[%4x], filter failed to start!\n",
                                  __FILE__,
                                  __LINE__,
                                  FilterIndex);
#endif	
	  }

         return bErrCode;
}

/*******************************************************************************
 *Function name:  CHCA_StopDemuxFilter
 * 
 *
 *Description:  
 *                  
 *
 *Prototype:
 *        CHCA_BOOL   CHCA_StopDemuxFilter(CHCA_USHORT  FilterIndex)
 *       
 *
 *input:
 *      
 * 
 *
 *output:
 *
 *Return Value:
 *        true:   interface execution error
 *        false:  stop demuxdata receive successfully
 *
 *Comments:
 *     
 *   5516_platform
 * 
 *******************************************************************************/
CHCA_BOOL   CHCA_StopDemuxFilter(CHCA_USHORT  FilterIndex)
{
        CHCA_BOOL                                   bErrCode = false;
  	 CHCA_USHORT                               FltrCount; 

        if(FilterIndex>=CHCA_MAX_NUM_FILTER)
        {
              bErrCode = true;
		return bErrCode;  	  
 	 }

        if(CH6_Get_SlotAssociateFilter_Count(FDmxFlterInfo [ FilterIndex ].SlotId,&FltrCount))
        {
               bErrCode = true;
#ifdef     CHDEMUX_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[CHCA_StopDemuxFilter::] stop filter[%d] ,Get filter count  with the specified slot error \n",
                                 __FILE__,
                                 __LINE__,
                                 FilterIndex);
#endif	
               return (bErrCode);
	  }

         /*stop the filter of the demux*/	
         if(CH6_Stop_Filter(FilterIndex)==false)
         {
#ifdef     CHDEMUX_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[CHCA_StopDemuxFilter::] the filterhandle[%d] has been succeeded in stopping\n",
                                __FILE__,
                                __LINE__,
                                FilterIndex);
#endif

#if  1/*add this on 041118*/
               /*CHCA_MgDataNotifyUnRegister(FilterIndex);	 */
               
               FDmxFlterInfo [ FilterIndex ].bdmxactived =  false;
               /*FDmxFlterInfo[FilterIndex].uTmDelay = 0;*/
#endif
	  }
	  else
	  {
                bErrCode = true;
#ifdef      CHDEMUX_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHCA_StopDemuxFilter::] The filter handle[%4x],Stop Filter error\n",
                                  __FILE__,
                                  __LINE__,
                                  FilterIndex);
#endif	
                return bErrCode;
	  }


#ifdef      CHDEMUX_DRIVER_DEBUG			  
         do_report(severity_info,"\n[CHCA_StopDemuxFilter==>]Filter[%d] bdmxactived[%d] FltrCount[%d], Stop filter Success!\n",FilterIndex,FDmxFlterInfo [ FilterIndex ].bdmxactived,FltrCount);		 
#endif

         /*if FltrCount is eqaul to 0 (not slot associated),is equal to 1(1 slot associated 1 filter),is bigger than 1(1 slot associated associated multiple filter) */
	  if(FltrCount != 1)
	  {
                return bErrCode;
	  }

         if(CH6_Stop_Slot(FDmxFlterInfo [ FilterIndex ].SlotId)==false)
         {
#ifdef     CHDEMUX_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[CHCA_StopDemuxFilter::] channel[%d] has been stopped\n",
                                 __FILE__,
                                 __LINE__,
                                 FDmxFlterInfo [ FilterIndex ].SlotId);
#endif
         }
         else
         {
                bErrCode = true;
#ifdef      CHDEMUX_DRIVER_DEBUG			  
                  do_report(severity_info,"\n %s %d >[CHCA_StopDemuxFilter::] The channel handle[%4x],fail to stop\n",
                                    __FILE__,
                                    __LINE__,
                                    FDmxFlterInfo [ FilterIndex ].SlotId);
#endif	
              
         }


         return bErrCode;
		 
}


/****************************************************************************************
 *Function name:    CHCA_Descrambler_Init
 *  
 *
 *Description: init the instance of the descrambler 
 *             
 *
 *Prototype:
 *          void   CHCA_Descrambler_Init(void)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
void   CHCA_Descrambler_Init(void)
{
       CHCA_USHORT   i,j;

       for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)
       {
	      DscrChannel[i].DscrChAllocated = 0;
	      DscrChannel[i].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;
             DscrChannel[i].SigKeyMultiSlotSet = false;
	      DscrChannel[i].OtherDscrCh = CHCA_DEMUX_INVALID_KEY; 	  
		  
	      for(j=0;j<2;j++)
             {
                   DscrChannel[i].DscrSlotInfo[j].SlotValue = CHCA_DEMUX_INVALID_SLOT;
		     DscrChannel[i].DscrSlotInfo[j].PIDValue = CHCA_DEMUX_INVALID_SLOT;
		     DscrChannel[i].DscrSlotInfo[j].DscrAssociated = false;
		     DscrChannel[i].DscrSlotInfo[j].DscrStarted = false;
		     DscrChannel[i].DscrSlotInfo[j].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;	 
	      	}	 
	} 
}

/****************************************************************************************
 *Function name:    CHCA_Get_FreeKeyCounter
 *  
 *
 *Description: get the remainder free key counter
 *             
 *
 *Prototype:
 *          CHCA_USHORT   CHCA_Get_FreeKeyCounter(void)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_USHORT   CHCA_Get_FreeKeyCounter(void)
{
       CHCA_USHORT       iKeyIndex,iKeyCount; 

	iKeyCount = 0;   
	   
	for(iKeyIndex=0;iKeyIndex<CHCA_MAX_DSCR_CHANNEL;iKeyIndex++)
	{
               if(DscrChannel[iKeyIndex].DscrChAllocated != 1)
               {
                      iKeyCount++;
		 }
	}

	return iKeyCount;
}


#if 0
/****************************************************************************************
 *Function name:    CHCA_Allocate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *           CHCA_BOOL   CHCA_Allocate_Descrambler(CHCA_USHORT*    Deshandle )
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL   CHCA_Allocate_Descrambler(CHCA_USHORT*    Deshandle )
{
        ST_ErrorCode_t                           ErrCode;
        STPTI_DescramblerType_t            DescramblerType;
	 CHCA_USHORT                            iDesIndex,i,j;	
	 TCHCAPTIDescrambler                 Descramblerhandle; 
	 CHCA_BOOL                                bReturnCode=false;

#if  1
       DescramblerType = STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER;		
#else
        DescramblerType = STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER;		
#endif

	 ErrCode = STPTI_DescramblerAllocate(CHCAPTIHandle,&Descramblerhandle,DescramblerType);	

	 if(ErrCode == ST_NO_ERROR)
	 {
             for(iDesIndex=0;iDesIndex<CHCA_MAX_DSCR_CHANNEL;iDesIndex++)
             {
                    if(!DscrChannel[iDesIndex].DscrChAllocated)
                    {
                           break;
                    }
             }

             if(iDesIndex>=CHCA_MAX_DSCR_CHANNEL)	
             {
                    bReturnCode = true;
		      return bReturnCode;			
	      }

             *Deshandle =  iDesIndex;

              DscrChannel[iDesIndex].DscrChAllocated = 1;
              DscrChannel[iDesIndex].Descramblerhandle = Descramblerhandle;
              DscrChannel[iDesIndex].SigKeyMultiSlotSet = false;
              DscrChannel[iDesIndex].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;

              for(i=0;i<2;i++)
              {
                     DscrChannel[iDesIndex].DscrSlotInfo[i].SlotValue = CHCA_DEMUX_INVALID_SLOT;
                     DscrChannel[iDesIndex].DscrSlotInfo[i].PIDValue = CHCA_DEMUX_INVALID_SLOT;
                     DscrChannel[iDesIndex].DscrSlotInfo[i].DscrAssociated = false;
                     DscrChannel[iDesIndex].DscrSlotInfo[i].DscrStarted = false;
                     DscrChannel[iDesIndex].DscrSlotInfo[i].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;
              }	
	 }
	 else
	 {
               bReturnCode = true;
	 }

	 return bReturnCode;

}


/****************************************************************************************
 *Function name:  CHCA_Deallocate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *         CHCA_BOOL  CHCA_Deallocate_Descrambler(CHCA_USHORT   DescramblerIndex)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Deallocate_Descrambler(CHCA_USHORT   DescramblerIndex)
{
       ST_ErrorCode_t       ErrCode;
       CHCA_USHORT     	  i,DscrChId_Temp;
	CHCA_BOOL            bReturnCode=false;   
	   

	 if(DescramblerIndex >= CHCA_MAX_DSCR_CHANNEL)
	 {
               bReturnCode = true;
		 return bReturnCode;	   
	 }

        DscrChId_Temp = DescramblerIndex;

	 if(!DscrChannel[DscrChId_Temp].DscrChAllocated )
	 {
               bReturnCode = true;
		 return bReturnCode;	   
	 }
	   
        ErrCode = STPTI_DescramblerDeallocate(DscrChannel[DscrChId_Temp].Descramblerhandle);

	 if(ErrCode == ST_NO_ERROR)
	 {
	        DscrChannel[DscrChId_Temp].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;	  
               DscrChannel[DscrChId_Temp].DscrChAllocated = 0;
	        DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = false;
               DscrChannel[DscrChId_Temp].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;

	        for(i=0;i<2;i++)
	        {
                     DscrChannel[DscrChId_Temp].DscrSlotInfo[i].SlotValue = CHCA_DEMUX_INVALID_SLOT;
		       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].PIDValue = CHCA_DEMUX_INVALID_SLOT;
		       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated = false;
		       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrStarted = false;
			DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;					
	        }		 
	 }
	 else
	 {
               bReturnCode = true;
	 }

	 return bReturnCode;
}



/****************************************************************************************
 *Function name:  CHCA_Associate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 * CHCA_BOOL  CHCA_Associate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Associate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
{
       ST_ErrorCode_t                    ErrCode;
       CHCA_BOOL                         bReturnCode;
       CHCA_USHORT                     DscrChId_Temp,TempSlotIndex,iTrackIndex;
	TCHCAPTIDescrambler          Descramblerhandle; 
	CHCA_UINT                          islot;
	CHCA_PTI_SlotOrPid_t          slotorpid;    

       DscrChId_Temp = DescramblerIndex;

       if(DscrChId_Temp >= CHCA_MAX_DSCR_CHANNEL)
       {
             bReturnCode = true;
	      return bReturnCode;		 
	}
	   
	/*get the slot id from the PID passed*/	  
	for(iTrackIndex = 0;iTrackIndex < CHCA_MAX_TRACK_NUM;iTrackIndex ++)   
	{
             if((uword)stTrackTableInfo.ProgElementPid[iTrackIndex]== SlotOrPid)
		   break;	 	
	}
	
	if(iTrackIndex >= CHCA_MAX_TRACK_NUM)
	{
             bReturnCode = true;
	      return bReturnCode;		 
	}  	

 	islot =  stTrackTableInfo.ProgElementSlot[iTrackIndex];

	if(islot == CHCA_DEMUX_INVALID_SLOT)
	{
             bReturnCode = true;
	      return bReturnCode;		 
	}
	
#ifdef   CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Associate_Descrambler==>] Pid[%d][%d] Descrambler[%d] inputpid[%d]\n",
                        DscrChannel[DscrChId_Temp].DscrSlotInfo[0].PIDValue,
                        DscrChannel[DscrChId_Temp].DscrSlotInfo[1].PIDValue,
                        DscrChannel[DscrChId_Temp].Descramblerhandle,
                        SlotOrPid);
#endif

       /*slotorpid.Pid = PID;*/
	slotorpid.Slot = islot;
       Descramblerhandle =  DscrChannel[DscrChId_Temp].Descramblerhandle; 

       ErrCode = STPTI_DescramblerAssociate(Descramblerhandle,slotorpid);

	if(ErrCode == ST_NO_ERROR)
	{
              if((DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated) && (DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
              {
#ifdef           CHDEMUX_DRIVER_DEBUG
                     do_report(severity_info,"\n[CHCA_Associate_Descrambler==>] the two pid have already associated,wrong\n");
#endif
                     bReturnCode = true;
	              return bReturnCode;		 
	       } 
	       else
	       {
                    if((!DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated) && (!DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
                    {
                           DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = false;
                           DscrChannel[DscrChId_Temp].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;
					 
                           TempSlotIndex = 0;
			      Descramblerhandle = DscrChannel[DscrChId_Temp].Descramblerhandle;	
			      DscrChannel[DscrChId_Temp].DscrSlotInfo[TempSlotIndex].Descramblerhandle = Descramblerhandle;
#ifdef                 CHDEMUX_DRIVER_DEBUG
                           do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The two pids have not been associted,First time!\n");
#endif
		      }
		      else
		      {
                           if(DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated)
                           {
                                    TempSlotIndex = 1;
#ifdef   CHDEMUX_DRIVER_DEBUG
                                    do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The pid[%d] has not been associated!Slot[0] has been associated !\n",SlotOrPid);
#endif
		             }
			      else
			      {
                                   TempSlotIndex = 0;
#ifdef   CHDEMUX_DRIVER_DEBUG
                                   do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The pid[%d] has not been associted!Slot[1] has been associated !\n",SlotOrPid);
#endif
			      }

                           DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = true;
                           DscrChannel[DscrChId_Temp].OtherDscrCh = Descramblerhandle;	
	  
		        }
	         }

                DscrChannel[DscrChId_Temp].DscrSlotInfo[TempSlotIndex].PIDValue = SlotOrPid; 	  	
                DscrChannel[DscrChId_Temp].DscrSlotInfo[TempSlotIndex].SlotValue = islot;	
                DscrChannel[DscrChId_Temp].DscrSlotInfo[TempSlotIndex].DscrAssociated = true;
                DscrChannel[DscrChId_Temp].DscrSlotInfo[TempSlotIndex].Descramblerhandle = Descramblerhandle;			
	}
	else
	{
             bReturnCode = true;
	}

      	return bReturnCode;	
		
}



/****************************************************************************************
 *Function name:  CHCA_Disassociate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *        CHCA_BOOL CHCA_Disassociate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Disassociate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
{
       ST_ErrorCode_t                     ErrCode;

        CHCA_BOOL                         bReturnCode = false;
	 CHCA_USHORT                     DscrChId_Temp,i;
	 CHCA_PTI_SlotOrPid_t           slotorpid;    
        TCHCAPTIDescrambler          Descramblerhandle; 

        DscrChId_Temp = DescramblerIndex;

        if(DscrChId_Temp>=CHCA_MAX_DSCR_CHANNEL)
        {
                do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>]DscrChId_Temp error\n");
                bReturnCode = true;
   	         return bReturnCode;		 
	 }

#ifdef   CHDEMUX_DRIVER_DEBUG
        do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>] Pid[%d][%d] Descrambler[%d] inputpid[%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].PIDValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].PIDValue,
                                              DscrChannel[DscrChId_Temp].Descramblerhandle,
                                              SlotOrPid);
#endif

        for(i=0;i<2;i++)
        {
               if(DscrChannel[DscrChId_Temp].DscrSlotInfo[i].PIDValue == SlotOrPid)
               {
                      break;
		 }
	 }

        if(i==2)		
        {
               do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>]pid  error\n");
               bReturnCode = true;
	        return bReturnCode;		 
	 }

        slotorpid.Slot = DscrChannel[DscrChId_Temp].DscrSlotInfo[i].SlotValue;
        Descramblerhandle = DscrChannel[DscrChId_Temp].Descramblerhandle;
	   
        ErrCode = STPTI_DescramblerDisassociate(Descramblerhandle,slotorpid);

	 if(ErrCode == ST_NO_ERROR)
	 {
               bReturnCode = false;
		 DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated = false;	   
	 }
	 else
	 {
               bReturnCode = true;
	 }

	 return bReturnCode;		 

}


/****************************************************************************************
 *Function name:  CHCA_Set_OddKey2Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *  CHCA_BOOL  CHCA_Set_OddKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Set_OddKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
{
       ST_ErrorCode_t                      ErrCode;
       CHCA_BOOL                           bReturnCode=false;
	CHCA_USHORT                       DscrChId_Temp;

       STPTI_KeyParity_t                   KeyParity;
	STPTI_KeyUsage_t                   KeyUsage;  
	STPTI_Descrambler_t               KeyHandle;

	DscrChId_Temp = DescramblerIndex;

       if((DscrChId_Temp >= CHCA_MAX_DSCR_CHANNEL)||(Data==NULL))
       {
              bReturnCode = true;
   	       return bReturnCode;		 
	}

       KeyHandle = DscrChannel[DscrChId_Temp].Descramblerhandle;
	KeyParity = STPTI_KEY_PARITY_ODD_PARITY;
	KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;
	
#ifdef   CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>] Slot[%d][%d] Descrambler[%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].SlotValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].SlotValue,
                                              DscrChannel[DscrChId_Temp].Descramblerhandle);
#endif

       ErrCode = STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,Data);

	if(ErrCode != ST_NO_ERROR)
	{
	      do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>]The interface is wrong\n");
             bReturnCode=true;
	} 

   	return bReturnCode;		 
}


/****************************************************************************************
 *Function name:  CHCA_Set_EvenKey2Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 * CHCA_BOOL  CHCA_Set_EvenKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *     STPTI_KEY_USAGE_INVALID
 *     STPTI_KEY_USAGE_VALID_FOR_PES
 *     STPTI_KEY_USAGE_VALID_FOR_TRANSPORT
 *     STPTI_KEY_USAGE_VALID_FOR_ALL 
 ****************************************************************************************/
CHCA_BOOL  CHCA_Set_EvenKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
{
       ST_ErrorCode_t                  ErrCode;
       CHCA_BOOL                       bReturnCode=false;
	CHCA_USHORT                   DscrChId_Temp;
	   

       STPTI_KeyParity_t              KeyParity;
	STPTI_KeyUsage_t              KeyUsage;  
	STPTI_Descrambler_t          KeyHandle;


	DscrChId_Temp = DescramblerIndex;

       if((DscrChId_Temp >= CHCA_MAX_DSCR_CHANNEL)||(Data==NULL))
       {
              bReturnCode = true;
   	       return bReturnCode;		 
	}

       KeyHandle = DscrChannel[DscrChId_Temp].Descramblerhandle;
	KeyParity = STPTI_KEY_PARITY_EVEN_PARITY;
	KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;

#ifdef   CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>] Slot[%d][%d] Descrambler[%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].SlotValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].SlotValue,
                                              DscrChannel[DscrChId_Temp].Descramblerhandle);
#endif

       ErrCode = STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,Data);

	if(ErrCode != ST_NO_ERROR)
	{
	      do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>]The interface is wrong\n");

             bReturnCode=true;
	}

   	return bReturnCode;	

}
#else
/****************************************************************************************
 *Function name:    CHCA_Allocate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *          CHCA_BOOL   CHCA_Allocate_Descrambler(TCHCAPTIDescrambler*    Deshandle )
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL   CHCA_Allocate_Descrambler(CHCA_USHORT*    Deshandle )
{
        CHCA_BOOL                                         bReturnCode=false;
        TCHCAPTIDescrambler                          Descramblerhandle; 
	 CHCA_USHORT                                     iDesIndex,j,i;	
	
         for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)
         {
              if(DescramChannelInfo[i].DescramblerKey != CHCA_DEMUX_INVALID_KEY)
              {
                    break;
		}
	  }

	 if(i==CHCA_MAX_DSCR_CHANNEL)
	 {
	       bReturnCode = true;
               do_report(severity_info,"\n[CHCA_Allocate_Descrambler==>]The Descrambler is not opened!!\n");
	 }
	 else
	 {
              for(iDesIndex=0;iDesIndex<CHCA_MAX_DSCR_CHANNEL;iDesIndex++)
              {
                     if(!DscrChannel[iDesIndex].DscrChAllocated)
                     {
                          break;
		       }
	       }

              if(iDesIndex>=CHCA_MAX_DSCR_CHANNEL)	
              {
                      do_report(severity_info,"\n[CHCA_Allocate_Descrambler==>]No free key\n");						
                      bReturnCode = true;
	       }
		else
		{
		       *Deshandle = iDesIndex;
                     DscrChannel[iDesIndex].DscrChAllocated = 1;
                    /* DscrChannel[iDesIndex].Descramblerhandle = Descramblerhandle;*/
			DscrChannel[iDesIndex].SigKeyMultiSlotSet = false;
			DscrChannel[iDesIndex].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;

                     for(j=0;j<2;j++)
                     {
                            DscrChannel[iDesIndex].DscrSlotInfo[j].SlotValue = CHCA_DEMUX_INVALID_SLOT;
                            DscrChannel[iDesIndex].DscrSlotInfo[j].PIDValue = CHCA_MG_INVALID_PID;
                            DscrChannel[iDesIndex].DscrSlotInfo[j].DscrAssociated = false;
                            DscrChannel[iDesIndex].DscrSlotInfo[j].DscrStarted = false;
                            DscrChannel[iDesIndex].DscrSlotInfo[j].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;
                     }		 
		}
			  
	 }

	 return bReturnCode;

}


/****************************************************************************************
 *Function name:  CHCA_Deallocate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *         CHCA_BOOL  CHCA_Deallocate_Descrambler(TCHCAPTIDescrambler DescramblerHandle)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Deallocate_Descrambler(CHCA_USHORT   DescramblerIndex)
{
        CHCA_BOOL                  bReturnCode;
        CHCA_USHORT     	  i,DscrChId_Temp;

	 if(DescramblerIndex >= CHCA_MAX_DSCR_CHANNEL)
	 {
               bReturnCode = true;
		 do_report(severity_info,"\n[CHCA_Deallocate_Descrambler==>] The DescramblerIndex is invalid!\n");	   
		 return bReturnCode;	   
	 }

        DscrChId_Temp = DescramblerIndex;

        for(i=0;i<2;i++) 
        {
              if(DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle != CHCA_DEMUX_INVALID_KEY)
              {
                     DscrChannel[DscrChId_Temp].DscrSlotInfo[i].SlotValue = CHCA_DEMUX_INVALID_SLOT;
		       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].PIDValue = CHCA_MG_INVALID_PID;
		       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated = false;
		       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrStarted = false;
			DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;					
	       }
	 }

        DscrChannel[DscrChId_Temp].Descramblerhandle = CHCA_DEMUX_INVALID_KEY;	  
        DscrChannel[DscrChId_Temp].DscrChAllocated = 0;
	 DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = false;
        DscrChannel[DscrChId_Temp].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;

        return (bReturnCode); 
	
}

#if 0/*060111 xjp add CHCA_Associate_Descrambler_() for test*/


/****************************************************************************************
 *Function name:  CHCA_Associate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *         CHCA_BOOL  CHCA_Associate_Descrambler_(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Associate_Descrambler_(CHCA_UINT Index,CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
{
       CHCA_BOOL                         bReturnCode;
       CHCA_USHORT                     DscrChId_Temp,TempSlotIndex,iTrackIndex;
	TCHCAPTIDescrambler          Descramblerhandle; 
	CHCA_UINT                          islot;
	CHCA_PTI_SlotOrPid_t          slotorpid;    

       DscrChId_Temp = DescramblerIndex;

       if(DscrChId_Temp>=CHCA_MAX_DSCR_CHANNEL)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The DescramblerIndex is valid \n");		 
	      return bReturnCode;		 
	}

        if(SlotOrPid==CHCA_MG_INVALID_PID)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The pid value is valid \n");		 
	      return bReturnCode;		 
	}

       if(!DscrChannel[DscrChId_Temp].DscrChAllocated)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The key is not allocated! \n");		 
	      return bReturnCode;		 
	}

#if 0 
       /*5518_platform*/
	/*get the slot id from the PID passed*/	  
       for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)
       {
              if(DescramChannelInfo[i].avpid==SlotOrPid)
              {
                    break;
		}
	}
#else
	#if 0/*060111 xjp add for test */
       /*5516_platform*/
       /*get the slot id from the PID passed*/	  
	for(i = 0;i < 2;i ++)   
	{
             if((uword)stTrackTableInfo.ProgElementPid[i]== SlotOrPid)
		   break;	 	
	}
	
	if(i >= 2)
	{
             bReturnCode = true;
	      return bReturnCode;		 
	}  
	if(i>=CHCA_MAX_DSCR_CHANNEL)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]Not find the corrected pid[%d] \n",SlotOrPid);		 
	      return bReturnCode;		 
	}
	#endif
#endif


       

#ifdef  CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Associate_Descrambler==>] Pid[%d][%d] Descrambler[%d][%d] inputpid[%d]Index[%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].PIDValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].PIDValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].Descramblerhandle,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].Descramblerhandle,
                                              SlotOrPid,
                                              Index);
#endif

       if((DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated) && (DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
       {
#ifdef   CHDEMUX_DRIVER_DEBUG
              do_report(severity_info,"\n[CHCA_Associate_Descrambler==>] the two pid have already associated,wrong\n");
#endif
              bReturnCode = true;
	       return bReturnCode;		 
	} 
	else
	{
   	       DscrChannel[DscrChId_Temp].DscrSlotInfo[Index].Descramblerhandle = DescramChannelInfo[Index].DescramblerKey;
              if((!DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated) && (!DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
              {
                     DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = false;
                     DscrChannel[DscrChId_Temp].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;
					 
#ifdef         CHDEMUX_DRIVER_DEBUG
                     do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The two pids have not been associted,First time!\n");
#endif
		}
		else
		{
                     DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = true;
                     DscrChannel[DscrChId_Temp].OtherDscrCh = DescramChannelInfo[Index].DescramblerKey;	
					 
#ifdef          CHDEMUX_DRIVER_DEBUG
                     do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]One Key,two slot!!\n");
#endif
		}
	}

       DscrChannel[DscrChId_Temp].DscrSlotInfo[Index].PIDValue = SlotOrPid; 	  	
       DscrChannel[DscrChId_Temp].DscrSlotInfo[Index].SlotValue = DescramChannelInfo[Index].avslot;	
       DscrChannel[DscrChId_Temp].DscrSlotInfo[Index].DscrAssociated = true;

	return  bReturnCode;

}

#else
/****************************************************************************************
 *Function name:  CHCA_Associate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *         CHCA_BOOL  CHCA_Associate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Associate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
{
       CHCA_BOOL                         bReturnCode;
       CHCA_USHORT                     DscrChId_Temp,TempSlotIndex,iTrackIndex;
	TCHCAPTIDescrambler          Descramblerhandle; 
	CHCA_UINT                          islot,i;
	CHCA_PTI_SlotOrPid_t          slotorpid;    

       DscrChId_Temp = DescramblerIndex;

       if(DscrChId_Temp>=CHCA_MAX_DSCR_CHANNEL)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The DescramblerIndex is valid \n");		 
	      return bReturnCode;		 
	}

        if(SlotOrPid==CHCA_MG_INVALID_PID)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The pid value is valid \n");		 
	      return bReturnCode;		 
	}

       if(!DscrChannel[DscrChId_Temp].DscrChAllocated)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The key is not allocated! \n");		 
	      return bReturnCode;		 
	}

#if 0 
       /*5518_platform*/
	/*get the slot id from the PID passed*/	  
       for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)
       {
              if(DescramChannelInfo[i].avpid==SlotOrPid)
              {
                    break;
		}
	}
#else
	
       /*5516_platform*/
       /*get the slot id from the PID passed*/	  
	for(i = 0;i < 2;i ++)   
	{
             if((uword)stTrackTableInfo.ProgElementPid[i]== SlotOrPid)
		   break;	 	
	}
	
	if(i >= 2)
	{
             bReturnCode = true;
	      return bReturnCode;		 
	}  
	if(i>=CHCA_MAX_DSCR_CHANNEL)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]Not find the corrected pid[%d] \n",SlotOrPid);		 
	      return bReturnCode;		 
	}
	
#endif


       

#ifdef  CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Associate_Descrambler==>] Pid[%d][%d] Descrambler[%d][%d] inputpid[%d]Index[%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].PIDValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].PIDValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].Descramblerhandle,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].Descramblerhandle,
                                              SlotOrPid,
                                              i);
#endif

       if((DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated) && (DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
       {
#ifdef   CHDEMUX_DRIVER_DEBUG
              do_report(severity_info,"\n[CHCA_Associate_Descrambler==>] the two pid have already associated,wrong\n");
#endif
              bReturnCode = true;
	       return bReturnCode;		 
	} 
	else
	{
   	       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle = DescramChannelInfo[i].DescramblerKey;
              if((!DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated) && (!DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
              {
                     DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = false;
                     DscrChannel[DscrChId_Temp].OtherDscrCh = CHCA_DEMUX_INVALID_KEY;
					 
#ifdef         CHDEMUX_DRIVER_DEBUG
                     do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]The two pids have not been associted,First time!\n");
#endif
		}
		else
		{
                     DscrChannel[DscrChId_Temp].SigKeyMultiSlotSet = true;
                     DscrChannel[DscrChId_Temp].OtherDscrCh = DescramChannelInfo[i].DescramblerKey;	
					 
#ifdef          CHDEMUX_DRIVER_DEBUG
                     do_report(severity_info,"\n[CHCA_Associate_Descrambler==>]One Key,two slot!!\n");
#endif
		}
	}

       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].PIDValue = SlotOrPid; 	  	
       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].SlotValue = DescramChannelInfo[i].avslot;	
       DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated = true;

	return  bReturnCode;

}

#endif


/****************************************************************************************
 *Function name:  CHCA_Disassociate_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *CHCA_BOOL CHCA_Disassociate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid) *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL CHCA_Disassociate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid)
{
        CHCA_BOOL                         bReturnCode = false;
	 CHCA_USHORT                     DscrChId_Temp,i;
	 CHCA_PTI_SlotOrPid_t           SlotId;    
        TCHCAPTIDescrambler          Descramblerhandle; 

        DscrChId_Temp = DescramblerIndex;

        if(DscrChId_Temp>=CHCA_MAX_DSCR_CHANNEL)
        {
                bReturnCode = true;
		  do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>] DescramblerIndex[%d] is error \n",DscrChId_Temp);		
   	         return bReturnCode;		 
	 }

        if(SlotOrPid==CHCA_MG_INVALID_PID)
        {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>]The SlotOrPid value is invalid \n");		 
	      return bReturnCode;		 
	 }

       if(!DscrChannel[DscrChId_Temp].DscrChAllocated)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>]The key is not allocated! \n");		 
	      return bReturnCode;		 
	}
		

        for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)
        {
               if(DscrChannel[DscrChId_Temp].DscrSlotInfo[i].PIDValue == SlotOrPid)
               {
                      break;
		 }
	 }

        if(i==CHCA_MAX_DSCR_CHANNEL)		
        {
	        do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>] The pid[%d] error\n",SlotOrPid);
               bReturnCode = true;
	        return bReturnCode;		 
	 }

#ifdef  CHDEMUX_DRIVER_DEBUG
        do_report(severity_info,"\n[CHCA_Disassociate_Descrambler==>] Pid[%d][%d] Descrambler[%d] inputpid[%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].PIDValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].PIDValue,
                                              DscrChannel[DscrChId_Temp].Descramblerhandle,
                                              SlotOrPid);
#endif
		

        DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated = false;

#if  1 /*add this on 050603*/
        DscrChannel[DscrChId_Temp].DscrSlotInfo[i].SlotValue = CHCA_DEMUX_INVALID_SLOT;
        DscrChannel[DscrChId_Temp].DscrSlotInfo[i].PIDValue = CHCA_MG_INVALID_PID;
#endif

	 return bReturnCode;

}


/****************************************************************************************
 *Function name:  CHCA_Set_OddKey2Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *CHCA_BOOL  CHCA_Set_OddKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Set_OddKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
{
       CHCA_BOOL                           bReturnCode=false;
	CHCA_USHORT                       DscrChId_Temp,i;
       TCHCAPTIDescrambler            Descramblerhandle; 

       STPTI_KeyParity_t                   KeyParity;
	STPTI_KeyUsage_t                   KeyUsage;  
	STPTI_Descrambler_t               KeyHandle;
       ST_ErrorCode_t                       ErrCode;

	U8 u_ClearKey[8];   

       DscrChId_Temp = DescramblerIndex;

       if((DscrChId_Temp >= CHCA_MAX_DSCR_CHANNEL)||(Data==NULL))
       {
              bReturnCode = true;
	       do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>] the input parameter is invalid\n");
   	       return bReturnCode;		 
	}

       if(!DscrChannel[DscrChId_Temp].DscrChAllocated)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>]The key is not allocated! \n");		 
	      return bReturnCode;		 
	}

        if((!DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated)&&(!DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>]The key is not associated! \n");		 
	      return bReturnCode;		 
	}

#ifdef   CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>] Slot[%d][%d] Descrambler[%d][%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].SlotValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].SlotValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].Descramblerhandle,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].Descramblerhandle);
#endif

{
#if 0/*060110 xjp add for test*/
unsigned char TempData[8];
memset(TempData,Data[0],8);
do_report(0,"\n Key Address:%x \n",Data);
#endif
       for(i=0;i<2;i++) 
       {
              Descramblerhandle = DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle;

              KeyHandle = DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle;
              KeyParity = STPTI_KEY_PARITY_ODD_PARITY;
              KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;
			  

              /*if(KeyHandle != CHCA_DEMUX_INVALID_KEY)*/
		/*modify this on 050603*/	  
              if((KeyHandle != CHCA_DEMUX_INVALID_KEY)&&(DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated))
              {
              #ifdef SECURITY_KEY
                    /*20070719 add*/
		     CH_CA_LoadCW(Data,0+i,u_ClearKey);

                      ErrCode = STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,u_ClearKey/*TempDataData*/);
                #else

                      ErrCode = STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,Data/*TempDataData*/);

		#endif
                      if(ErrCode != ST_NO_ERROR)
                      {
                            do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>]The interface is wrong\n");
                            bReturnCode=true;
                      } 
		}
		else
		{
		#ifdef SIMPLE_CA_TRACE
	              do_report(severity_info,"\n[CHCA_Set_OddKey2Descrambler==>] the keyindex[%d] is invalid (not Associated)\n",i);
		#endif
		}
		
	}
}
       return bReturnCode;
	
}


/****************************************************************************************
 *Function name:  CHCA_Set_EvenKey2Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *CHCA_BOOL  CHCA_Set_EvenKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *     STPTI_KEY_USAGE_INVALID
 *     STPTI_KEY_USAGE_VALID_FOR_PES
 *     STPTI_KEY_USAGE_VALID_FOR_TRANSPORT
 *     STPTI_KEY_USAGE_VALID_FOR_ALL 
 ****************************************************************************************/
CHCA_BOOL  CHCA_Set_EvenKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data)
{
       CHCA_BOOL                           bReturnCode=false;
	CHCA_USHORT                       DscrChId_Temp,i;
       TCHCAPTIDescrambler            Descramblerhandle; 

       STPTI_KeyParity_t                   KeyParity;
	STPTI_KeyUsage_t                   KeyUsage;  
	STPTI_Descrambler_t               KeyHandle;
       ST_ErrorCode_t                       ErrCode;
      U8 u_ClearKey[8];   
	   

       DscrChId_Temp = DescramblerIndex;

       if((DscrChId_Temp >= CHCA_MAX_DSCR_CHANNEL)||(Data==NULL))
       {
              bReturnCode = true;
	       do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>] the input parameter is invalid\n");
   	       return bReturnCode;		 
	}

       if(!DscrChannel[DscrChId_Temp].DscrChAllocated)
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>]The key is not allocated! \n");		 
	      return bReturnCode;		 
	}

        if((!DscrChannel[DscrChId_Temp].DscrSlotInfo[0].DscrAssociated)&&(!DscrChannel[DscrChId_Temp].DscrSlotInfo[1].DscrAssociated))
       {
             bReturnCode = true;
	      do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>]The key is not associated! \n");		 
	      return bReturnCode;		 
	}

#ifdef   CHDEMUX_DRIVER_DEBUG
       do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>] Slot[%d][%d] Descrambler[%d][%d]\n",
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].SlotValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].SlotValue,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[0].Descramblerhandle,
                                              DscrChannel[DscrChId_Temp].DscrSlotInfo[1].Descramblerhandle);
#endif

{
#if 0/*060110 xjp add for test */
unsigned char TempData[8];
memset(TempData,Data[0],8);
do_report(0,"\n Key Address:%x \n",Data);
#endif

       for(i=0;i<2;i++) 
       {
              Descramblerhandle = DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle;

              KeyHandle = DscrChannel[DscrChId_Temp].DscrSlotInfo[i].Descramblerhandle;
              KeyParity = STPTI_KEY_PARITY_EVEN_PARITY;
              KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;
			  
              /*if(KeyHandle != CHCA_DEMUX_INVALID_KEY)*/
              if((KeyHandle != CHCA_DEMUX_INVALID_KEY)&&(DscrChannel[DscrChId_Temp].DscrSlotInfo[i].DscrAssociated))
              {
                       #ifdef SECURITY_KEY

                      CH_CA_LoadCW(Data,2+i,u_ClearKey);
                      ErrCode = STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,u_ClearKey/*TempDataData*/);
                      #else
                      ErrCode = STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,Data/*TempDataData*/);

			#endif
                      if(ErrCode != ST_NO_ERROR)
                      {
                            do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>]The interface is wrong\n");
                            bReturnCode=true;
                      } 
		}
		else
		{
		#ifdef SIMPLE_CA_TRACE
	              do_report(severity_info,"\n[CHCA_Set_EvenKey2Descrambler==>] the keyindex[%d] is invalid (not associated)\n",i);
		#endif
		}
	
	}
}
       return bReturnCode;
	
}




#if 0/*060111 xjp add for test*/
void Descramble_test(void)
{
	CHCA_USHORT iDesIndexVid,iDesIndexAud;
	unsigned char TempDataEven[8];
	unsigned char TempDataOdd[8];
         STPTI_DescramblerType_t            DescramblerType;
	  CHCA_USHORT                            iDesIndex,i,j;	
	  TCHCAPTIDescrambler                 Descramblerhandle; 
	  CHCA_PTI_SlotOrPid_t                 slotorpid; 
STPTI_KeyParity_t                   KeyParity;
	STPTI_KeyUsage_t                   KeyUsage; 
	ST_ErrorCode_t   ErrorCode=ST_NO_ERROR;
	  
	memset(TempDataEven,0x22,8);
	TempDataEven[3]=TempDataEven[7]=0x66;

	memset(TempDataOdd,0x88,8);
	TempDataOdd[3]=TempDataOdd[7]=0x98;

	#if  0
       DescramblerType = STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER;		
#else
        DescramblerType = STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER;		
#endif

/*slotorpid.Pid=200;*/
slotorpid.Slot=VideoSlot;


	ErrorCode=STPTI_DescramblerAllocate(PTIHandle,&Descramblerhandle,DescramblerType);

	 ErrorCode|=STPTI_DescramblerAssociate(Descramblerhandle,slotorpid);
	
	
	/*task_delay(ST_GetClocksPerSecond()/3);*/

	KeyParity = STPTI_KEY_PARITY_EVEN_PARITY;
        KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;
	ErrorCode|=STPTI_DescramblerSet(Descramblerhandle,KeyParity,KeyUsage,/*TempData*/TempDataEven);
	/*task_delay(ST_GetClocksPerSecond()/3);*/
	
	 KeyParity = STPTI_KEY_PARITY_ODD_PARITY;
	ErrorCode|=STPTI_DescramblerSet(Descramblerhandle,KeyParity,KeyUsage,/*TempData*/TempDataOdd);
	if(ErrorCode==ST_NO_ERROR)
		do_report(0,"\nSucceed in Setting the key to descrambler!\n");
	else
		do_report(0,"\nFail to Setting the key to descrambler!\n");
	
}
#endif
#endif


/****************************************************************************************
 *Function name:  CHCA_Start_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *         CHCA_BOOL  CHCA_Start_Descrambler(CHCA_UINT  iSlotIndex)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Start_Descrambler(CHCA_UINT  iSlotIndex)
{
       ST_ErrorCode_t       ErrCode;

	STPTI_Slot_t            SlotHandle;   
	BOOL                      EnableDescramblingControl;

	SlotHandle = (STPTI_Slot_t)iSlotIndex;
	EnableDescramblingControl = true;

       ErrCode = STPTI_SlotDescramblingControl(SlotHandle,EnableDescramblingControl);

	if(ErrCode == ST_NO_ERROR)
	{

	#ifdef   CHDEMUX_DRIVER_DEBUG
		do_report(severity_info,"\n succeed in starting descrambler! \n");
	#endif
	
             return false;
	}
	else
	{

	#ifdef   CHDEMUX_DRIVER_DEBUG
		do_report(severity_info,"\n fail to start descrambler! \n");
	#endif

            return true;
	}


}




/****************************************************************************************
 *Function name:  CHCA_Stop_Descrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 *         CHCA_BOOL  CHCA_Stop_Descrambler(CHCA_UINT  iSlotIndex)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
CHCA_BOOL  CHCA_Stop_Descrambler(CHCA_UINT  iSlotIndex)
{
       ST_ErrorCode_t       ErrCode;

	STPTI_Slot_t            SlotHandle;   
	BOOL                      EnableDescramblingControl;

	SlotHandle = (STPTI_Slot_t)iSlotIndex;
	EnableDescramblingControl = false;

       ErrCode = STPTI_SlotDescramblingControl(SlotHandle,EnableDescramblingControl);

	if(ErrCode == ST_NO_ERROR)
	{

	#ifdef   CHDEMUX_DRIVER_DEBUG
		do_report(severity_info,"\n succeed in stopping descrambler! \n");
	#endif
	
             return false;
	}
	else
	{

	#ifdef   CHDEMUX_DRIVER_DEBUG
		do_report(severity_info,"\n fail to stop descrambler! \n");
	#endif
	
            return true;
	}


}


/****************************************************************************************
 *Function name:  ClearDescrambler
 *  
 *
 *Description: flash the cw data with the invalid data within the descrambler when zapping  
 *             
 *
 *Prototype:
 * void ClearDescrambler(void)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *
 *
 ****************************************************************************************/
void ClearDescrambler(void)
{
       CHCA_SHORT    i;

       STPTI_KeyParity_t                   KeyParity;
       STPTI_KeyUsage_t                   KeyUsage;  
       STPTI_Descrambler_t               KeyHandle;


        semaphore_wait(psemMgDescAccess); 
        for(i=0;i<2;i++)
	 {
               KeyHandle = DescramChannelInfo[i].DescramblerKey;
               KeyParity = STPTI_KEY_PARITY_ODD_PARITY;
               KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;
               STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,Invalid_Odd_key);
       
               KeyHandle = DescramChannelInfo[i].DescramblerKey;
               KeyParity = STPTI_KEY_PARITY_EVEN_PARITY;
               KeyUsage = STPTI_KEY_USAGE_VALID_FOR_ALL;
               STPTI_DescramblerSet(KeyHandle,KeyParity,KeyUsage,Invalid_Even_key);
        }	
	       semaphore_signal(psemMgDescAccess); 		  

	       return;		  
	 }
		 
/****************************************************************************************
 *Function name:  OpenVDescrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 * void    OpenVDescrambler(STPTI_Slot_t VSlot)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *   i == 0  --> video slot
 *   i == 1  --> audio slot
 ****************************************************************************************/
boolean  OpenVDescrambler(STPTI_Slot_t VSlot)
	            {
	  unsigned int /*key_t  051114 xjp*/                                        key_number=0;
	  CHCA_PTI_SlotOrPid_t                 slotorpid;    

         ST_ErrorCode_t                           ErrCode;
         STPTI_DescramblerType_t            DescramblerType;
	  CHCA_USHORT                            iDesIndex,i,j;	
	  TCHCAPTIDescrambler                 Descramblerhandle; 
	  CHCA_BOOL                                bReturnCode=false;

#if  0/*060112 xjp change for STi7710 descramble*/
       DescramblerType = STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER;		
#else
        DescramblerType = STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER;		
#endif

	 if(VSlot==CHCA_DEMUX_INVALID_SLOT)
	      {
              do_report(severity_info,"\n[OpenVDescrambler==>]video slot is wrong\n");
	       return true;		  
	      }

	 ErrCode = STPTI_DescramblerAllocate(PTIHandle,&Descramblerhandle,DescramblerType);	
		   
	 if (ErrCode != ST_NO_ERROR)
             {
		do_report(severity_info,"\n[OpenVDescrambler==>]can't get the free key for the VideoSlot[%d]\n",VSlot);
		return true;		  
             }
             else
             {
	      DescramChannelInfo[0].DescramblerKey=Descramblerhandle;

	      slotorpid.Slot = VSlot;
            
             ErrCode = STPTI_DescramblerAssociate(Descramblerhandle,slotorpid);

             if ( ErrCode != ST_NO_ERROR)
             {
		    do_report(severity_info,"\n[OpenVDescrambler==>]can't associate the videoslot[%d] to key\n",VSlot);
             }
             else
             {
                  DescramChannelInfo[0].avslot = VSlot;
             }
		
	 }

        return false;
	
}


/****************************************************************************************
 *Function name:  OpenADescrambler
 *  
 *
 *Description:  
 *             
 *
 *Prototype:
 * void    OpenADescrambler(STPTI_Slot_t ASlot)
 *           
 *
 *input:
 *	
 *
 *output:
 *
 *
 *Return Value:
 *	
 *	
 *
 *Comments:
 *   i == 0  --> video slot
 *   i == 1  --> audio slot
 ****************************************************************************************/
boolean  OpenADescrambler(STPTI_Slot_t ASlot)
{
	  unsigned int /*key_t  051114 xjp*/                                           key_number=0;
	  CHCA_PTI_SlotOrPid_t                 slotorpid;    

         ST_ErrorCode_t                           ErrCode;
         STPTI_DescramblerType_t            DescramblerType;
	  CHCA_USHORT                            iDesIndex,i,j;	
	  TCHCAPTIDescrambler                 Descramblerhandle; 
	  CHCA_BOOL                                bReturnCode=false;

	  	 /*CHCAPTIHandle = PTIHandle;  PTI HANDEL*/ 


#if  0/*060112 xjp change for STi7710 descramble*/
       DescramblerType = STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER;		
#else
        DescramblerType = STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER;		
#endif

	 if(ASlot==CHCA_DEMUX_INVALID_SLOT)
      {
              do_report(severity_info,"\n[OpenADescrambler==>]audio slot is wrong\n");
	       return true;		  
      }

	 ErrCode = STPTI_DescramblerAllocate(PTIHandle,&Descramblerhandle,DescramblerType);	
	 
	 if (ErrCode != ST_NO_ERROR)
    {
		do_report(severity_info,"\n[OpenADescrambler==>]can't get the free key for the AudioSlot[%d]\n",ASlot);
		return true;		  
    }
    else
    {
	      DescramChannelInfo[1].DescramblerKey=Descramblerhandle;
			  
	      slotorpid.Slot = ASlot;

             ErrCode = STPTI_DescramblerAssociate(Descramblerhandle,slotorpid);

             if ( ErrCode != ST_NO_ERROR)
    {
		    do_report(severity_info,"\n[OpenADescrambler==>]can't associate the audioslot[%d] to key\n",ASlot);
		    return true;		  
    }
    else
    {
                  DescramChannelInfo[1].avslot = ASlot;
             }
    }
	
        return false;

}
#endif
/*zxg 20060315 add for Reset video and audio Descrambler*/
/*函数原型:void CH_ResetAVDescrambler(void)
   函数说明:复位当前使用的AV解扰器
   返回:TRUE,成功FALSE:失败*/
void CH_ResetAVDescrambler(CHCA_USHORT   DescramblerIndex)
{
        int i;
	 boolean result;
	  ST_ErrorCode_t                           ErrCode;
	 /*首先释放原有的解扰器*/
	 STPTI_SlotOrPid_t   SlotOrPid;
        for(i=0;i<2;i++)
    	 {
    	    SlotOrPid.Slot =DescramChannelInfo[i].avslot;
           ErrCode=STPTI_DescramblerDisassociate(DescramChannelInfo[i].DescramblerKey, SlotOrPid);
		   if(ErrCode!=ST_NO_ERROR)
		   	{
                   do_report(0,"Reset STPTI_DescramblerDisassociate[%d] failed\n",i);
		   	}
	    ErrCode=STPTI_DescramblerDeallocate(DescramChannelInfo[i].DescramblerKey);
		     if(ErrCode!=ST_NO_ERROR)
		   	{
           do_report(0,"Reset STPTI_DescramblerDeallocate[%d] failed\n",i);
		   	}
    	}	
       /*打开新的解扰器*/
       result=OpenVDescrambler(VideoSlot);
	if(result==true)
		{
               do_report(0,"Reset OpenVDescrambler failed\n");
		}
       result=OpenADescrambler(AudioSlot); 
	   	if(result==true)
		{
            do_report(0,"Reset OpenADescrambler failed\n");
		}


       if(DescramblerIndex >= CHCA_MAX_DSCR_CHANNEL)
       {
          do_report(0,"Reset DescramblerIndex Error\n");
	   return;
	}

        for(i=0;i<2;i++)
	   	   DscrChannel[DescramblerIndex].DscrSlotInfo[i].Descramblerhandle = DescramChannelInfo[i].DescramblerKey;

}


/************************************************/

