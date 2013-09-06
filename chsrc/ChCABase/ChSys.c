#ifdef    TRACE_CAINFO_ENABLE
#define  CHSYS_DEBUG
#endif


/*#define    PAIR_TEST */            /*zxg 20060319 add 注释只用于配对测试,在E2PROM写入测试记号*/
/******************************************************************************
*
* File : ChSys.C
*
* Description : SoftCell driver
*
*
* NOTES :
*
*
* Author :
*
* Status : 
*
* History : 0.0   2004-06-30  Start coding
*               0.1  2004-09-28   copy the code from 5518 platform
                 0.3   2006-3       zxg  change for STi7710        
*
* Copyright: Changhong 2004 (c)
*
*****************************************************************************/
#include  "ChSys.h"


CHCA_StbId_Info_t      StbIDInfo;
	
#ifdef    PAIR_TEST
#ifndef    NAGRA_PRODUCE_VERSION

CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x1,0x19,0x5f,0x0,0x0,0x1,0x0,0x1}; 
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x01,0x01,0x12};*/
#else
/*01010501010000001*/
#if      /*PLATFORM_16 20070514 change*/1
#if 0/*20060302 add DVB-C7000B STB ID*/
CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x01,0x01,0x12};
#else/*01020603010000001*/

CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x00,0xc6,0x10,0x00,0x01,0x03/*01 5516,02,7710,03,5105*/,0x12};
#endif
#endif


/*00030822001441*/
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0x00,0x00,0x38,0x4e,0x00,0x01};*/
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x02,0x01,0x12};*/
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x03,0x01,0x12};*/
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x04,0x01,0x12};*/
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x05,0x01,0x12};*/
/*CHCA_UCHAR              ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0xa2,0x10,0x00,0x06,0x01,0x12};*/
/*CHCA_UCHAR          ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0x92,0x80,0xd7,0xb8,0x01,0x12};  */
/*CHCA_UCHAR          ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0x92,0x80,0xd7,0xb6,0x01,0x12};*/   
/*CHCA_UCHAR          ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0x92,0x80,0xd7,0xb5,0x01,0x12};*/   
/*CHCA_UCHAR          ChSTBID_Test[CHCA_STBID_MAX_LEN]={0x00,0x10,0x92,0x80,0xd7,0xb4,0x01,0x12};*/   
#endif
#endif

#if 0
/*kernel event message queue*/
static    message_queue_t  *pstCHMGKerEvInfoMsgQueue = NULL;
static    task_t                    *ptidKEventprocessTaskId;

#define  CHMG_KERNELEVINFO_MSG_QUEUE_SIZE                              (U32)sizeof(chmg_KerEvent_Cmd_t) 
#define  CHMG_KERNELEVINFO_MSG_QUEUE_NO_OF_MESSAGES        20  

/*kernel event process priority and workspace*/
const int  CHMG_KEvent_PROCESS_PRIORITY    =   12;
const int  CHMG_KEvent_PROCESS_WORKSPACE   =   1024*10;
#endif

#ifdef    PAIR_TEST
/*******************************************************************************
 *Function name:  GetSerialNumTest
 * 
 *
 *Description:  
 *                
 *
 *Prototype:
 *          CHCA_BOOL  GetSerialNumTest (void)
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
CHCA_BOOL  GetSerialNumTest (CHCA_UCHAR*    SerialNum)
{
       CHCA_INT                         error;
	CHCA_INT                         i;
	unsigned short                   a;

	CHCA_UCHAR                    SN[8];
	
	if(ReadNvmData(START_E2PROM_CASN,2,&a))
       {
	       do_report(severity_info,"\n fail to read the serial_nvm!! \n ");
	       return (true);
	}
	
	do_report(severity_error,"serial tag = [%08X]\n",a);

      /*a=0;  add for force update test serial*/
	if(a != 0x6534)
	{
		a=0x6534;
		if(WriteNvmData(START_E2PROM_CASN,2,&a))
              {
                     do_report(severity_error," \n fail to write serial data to EEPROM!\n"); 		   
			return (true);
		}

              error = WriteNvmData(START_E2PROM_CASN+2,CHCA_STBID_MAX_LEN,ChSTBID_Test);
			  
	}

       error = ReadNvmData(START_E2PROM_CASN+2,CHCA_STBID_MAX_LEN,SN);

#if 0
       for(i=0;i<8;i++)
	     do_report(severity_info,"%4x",SN[i]);	

	do_report(severity_info,"\n");   
#endif	
	
       memcpy(SerialNum,SN,8);

	return error;
	
}
#endif


/*******************************************************************************
 *Function name: ChKerEvProcessInit
 *           
 *
 *Description: 
 *             
 *             
 *    
 *Prototype:
 *    CHCA_BOOL ChKerEvProcessInit(void )          
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
 #if 0
CHCA_BOOL ChKerEvProcessInit(void )
{
       message_queue_t      *pstMessageQueue = NULL;

#ifdef  CHSYS_DEBUG	  
       do_report ( severity_info, "%s %d> Kernel Event Information Table collection initialisation..\n",
                          __FILE__,
                          __LINE__ );
#endif

      do_report(severity_info,"\n[ChKerEvProcessInit==>]The message len[%d]\n",CHMG_KERNELEVINFO_MSG_QUEUE_SIZE);

       /* choose a kernel  event message queue for mailbox communications */
	if ( ( pstMessageQueue  = message_create_queue_timeout ( CHMG_KERNELEVINFO_MSG_QUEUE_SIZE,CHMG_KERNELEVINFO_MSG_QUEUE_NO_OF_MESSAGES ) ) == NULL )
	{
		STTBX_Report( ( STTBX_REPORT_LEVEL_ERROR, "Unable to create kernel event message queue\n" ) );
	}
	else
	{
		/* register this symbol for other module access */
		symbol_register ( "kerevinfo_queue", ( void * ) pstMessageQueue );
	}     	  

       if ( symbol_enquire_value ( "kerevinfo_queue", ( void ** ) &pstCHMGKerEvInfoMsgQueue ) )
	{
             do_report ( severity_error, "%s %d> Can't  find kernel event  message queue\n",
                                __FILE__,
                                __LINE__ );

             return  true;
	}

      /*  Create KEvent process task   */
#ifdef  CHSYS_DEBUG	  
      do_report (severity_info,"creating KEvent PROCESS process Prio[%d] Stack[%d]\n",
                        CHMG_KEvent_PROCESS_PRIORITY,
                        CHMG_KEvent_PROCESS_WORKSPACE );
#endif

       if ( ( ptidKEventprocessTaskId = Task_Create ( CHMG_KerEvProcess,
                                                                                NULL,
                                                                                CHMG_KEvent_PROCESS_WORKSPACE,
                                                                                CHMG_KEvent_PROCESS_PRIORITY,
                                                                                "CHMG_KerEvProcess",
                                                                                0 ) ) == NULL )
	{
             do_report ( severity_error, "MGDDI_INIT=> Unable to create CHMG_KerEvProcess\n" );
	      return  true;
	}

       return false;
}
#endif

/*******************************************************************************
 *Function name: CHMG_SubCallback
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_SubCallback     
 *         (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2)
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
 *Comments:
 *    event list:           
 *     MGAPIEvSysError      when a major sytem error occurs        
 *     MGAPIEvDumpText      with the request to display a text on the screen     
 *     MGAPIEvNoResource    when a type of resource is no longer available      
 *     MGAPIEvMsgNotify     on filtering a public EMM      
 *     MGAPIEvUnknown       an unrecognized smart card is inserted in a MG card reader       
 *     MGAPIEvUsrNotify     when specific application data are to be notified
 *     MGAPIEvPassword      with the result of an action concerning the password.
 *
 *     when call the interface "MGAPISubscribe(CALLBACK hCallback)",
 *     the callback function is registered
 *
 *******************************************************************************/
  void CHMG_SubCallback
	 (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2)
 {
                    switch(EventCode)
                    {
                           case MGAPIEvSysError:
#ifdef                          CHSYS_DEBUG
                                    do_report(severity_info,"\n MGAPIEvSysError \n");
#endif		
                                    CHMG_CheckSysError(Excode,ExCode2);
			               break; 	 

                           case MGAPIEvDumpText:
#ifdef                         CHSYS_DEBUG
                                   do_report(severity_info,"\n MGAPIEvDumpText \n");
#endif
                                   CHMG_CtrlDisplayText((char*)ExCode2);
			              break; 	 

                           case MGAPIEvNoResource:
#ifdef                          CHSYS_DEBUG
                                    do_report(severity_info,"\n MGAPIEvNoResource \n");
#endif			  	
                                    CHMG_CheckNoResource(Excode,ExCode2);
			               break; 	 

                           case MGAPIEvMsgNotify:/*复位和邮*/
#ifdef                        CHSYS_DEBUG
                                    do_report(severity_info,"\n MGAPIEvMsgNotify \n");
#endif	
                                    CHMG_CtrlNotifyMessage(Excode,(TMGAPIBlk*)ExCode2);
			               break; 	 

                           case MGAPIEvUnknown:
#ifdef                          CHSYS_DEBUG
                                    do_report(severity_info,"\n MGAPIEvUnknown \n");
#endif		
                                    CHMG_CardUnknown((TMGSysSCRdrHandle)ExCode2);
			               break; 	 

                           case MGAPIEvUsrNotify:
#ifdef                          CHSYS_DEBUG
                                    do_report(severity_info,"\n MGAPIEvUsrNotify \n");
#endif			  	
                                    CHMG_UsrNotify(Excode,ExCode2);
			               break; 	

                           case MGAPIEvPassword:
#ifdef                          CHSYS_DEBUG
                                    do_report(severity_info,"\n MGAPIEvPassword \n");
#endif		
                                    CHMG_ResetPassword(Excode);
			               break; 	 
   	             }

 }

/*******************************************************************************
 *Function name  CHMG_CtrlCallback
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *      void CHMG_CtrlCallback     
 *         (HANDLE hSubscription,byte EventCode,word Excode,dword ExCode2)  
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
 *Comments:
 *   event list:           
 *      MGAPIEvBadCard        when reset or checking of a smart card has failed
 *      MGAPIEvReady          when a inserted card becomes ready
 *      MGAPIEvCmptStatus     with the result of analysis of the descrambling rights
 *      MGAPIEvRejected       when the control messages are rejected by the smart card
 *      MGAPIEvAcknowledged   when the control messages are accepted by the smart card     
 *      MGAPIEvHalted         complete shutdown of process of an instance     
 *      MGAPIEvClose          On destruction of an instance
 *
 *      when call the interface "MGAPICtrlOpen(CALLBACK hCallback,TMGSysSrcHandle hSource)",
 *      the callback function is registered
 *
 *******************************************************************************/
 void CHMG_CtrlCallback
   (HANDLE hSubscription,byte EventCode,word Excode,dword ExCode2)
 {
  	   	
	           switch(EventCode)
                  {
                      case MGAPIEvBadCard:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvBadCard is coming \n");
#endif	
			 	   break; 	 	

                      case MGAPIEvReady:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvReady is coming \n");
#endif	
                               CHMG_CheckCardReady((TMGAPICardContent*)ExCode2);
				   break;	  	

			 case MGAPIEvCmptStatus:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvCmptStatus is coming \n");
#endif	
                               CHMG_CtrlCheckCmptStatus(Excode,(TMGAPICmptStatusList*)ExCode2);
			 	   break;

       		 case MGAPIEvRejected:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvRejected is coming \n");
#endif	
                               CHMG_CtrlCheckRejectedStatus(Excode,(TMGAPIPIDList*)ExCode2);
			 	   break;

         		 case MGAPIEvAcknowledged:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvAcknowledged is coming \n");
#endif
                               CHMG_CtrlCheckRightAccepted(Excode,(TMGAPIPIDList*)ExCode2);
			 	   break;

         		 case MGAPIEvHalted:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvHalted is coming \n");
#endif			
                               CHMG_CheckHalted(Excode,ExCode2);
			 	   break;

			 case MGAPIEvClose:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgProgCtrl_Callback::]MGAPIEvClose is coming \n");
#endif				 	
			 	   break;

					  
	          }
 }

/*******************************************************************************
 *Function name: CHMG_CardCallback
 *           
 *
 *Description:
 *           
 *
 *Prototype:
 *         void CHMG_CardCallback  
 *            (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2)
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
 *     void
 *
 *
 *Comments:
 *      event list:           
 *      MGAPIEvExtract        when a smart card is extracted from a reader     
 *      MGAPIEvBadCard        when reset or checking of a smart card has failed     
 *      MGAPIEvReady          when a card inserted becomes ready     
 *      MGAPIEvContent        to communicate the application content of the smart card     
 *                            inserted
 *      MGAPIEvAddFuncData    to communicate a card add-on function
 *      MGAPIEvRating         to communicate reating information
 *      MGAPIEvClose          on destruction of an instance
 *
 *     when call the interface "MGAPICardOpen(CALLBACK hCallback)",the callback function
 *     is registered
 *******************************************************************************/
 void CHMG_CardCallback
   (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2)
 {
	
                  switch(EventCode)
                  {
                      case MGAPIEvContent:
#ifndef                   CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvContent is coming \n");
#endif	

                               CHMG_CardContent((TMGAPICardContent*)ExCode2);   /*add this on 041110*/
			 	   break; 	 	

                      case MGAPIEvAddFuncData:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvAddFuncData is coming \n");
#endif	
				   break;	  	

			 case MGAPIEvRating:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvRating is coming \n");
#endif	

                               CHMG_CardRating((TMGAPIRating*)ExCode2);
			 	   break;

       		 case MGAPIEvExtract:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvExtract is coming \n");
#endif				 	
			 	   break;

         		 case MGAPIEvBadCard:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvBadCard is coming \n");
#endif				 	
			 	   break;

         		 case MGAPIEvReady:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvReady is coming \n");
#endif	
                               CHMG_CheckCardReady((TMGAPICardContent*)ExCode2);
                               break;

			 case MGAPIEvClose:
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvClose is coming \n");
#endif				 	
			 	   break;

			 case MGAPIEvUpdate: /*add this on 041027*/
#ifdef                     CHSYS_DEBUG
                               do_report(severity_info,"\n [MgCardControl_Callback::]MGAPIEvUpdate is coming \n");
#endif			
                               CHMG_CheckCardContentUpdate();
			 	   break;
					  
	          }
 }


/*******************************************************************************
 *Function name: CHMG_KerEvProcess
 *           
 *
 *Description: the ddi implementation handle mediaguard kernel event notification from
 *                  a single separate task queuing the events in their order of occurence
 *             
 *    
 *Prototype:
 *    void CHMG_KerEvProcess ( void *pvParam )           
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
 *  event list         
 *     MGAPIEvSysError,
 *	 MGAPIEvNoResource,
 * 	 MGAPIEvDumpText,
 *	 MGAPIEvMsgNotify,
 *	 MGAPIEvClose,
 *	 MGAPIEvHalted,
 *	 MGAPIEvExtract,
 *	 MGAPIEvBadCard,
 *	 MGAPIEvUnknown,
 *	 MGAPIEvReady,
 *	 MGAPIEvPassword,
 *	 MGAPIEvCmptStatus,
 *	 MGAPIEvRejected,
 *	 MGAPIEvAcknowledged,
 *	 MGAPIEvContent,
 *	 MGAPIEvAddFuncData,
 *	 MGAPIEvRating,
 *	 MGAPIEvUsrNotify,
 *	 MGAPIEvFirstExt
 *
 *******************************************************************************/
 #if 0
void CHMG_KerEvProcess ( void *pvParam )
{
    chmg_KerEvent_Cmd_t     *msg_p=NULL;
    clock_t                             timer_temp;

	
	
    while(true)
    {
           /*setup the KEventInfoMsgQueue*/
           /*time out process*/	  
#if 0		   
           timer_temp = time_plus(time_now(), ST_GetClocksPerSecond() * 30);
	    
           if((msg_p = ( chmg_KerEvent_Cmd_t * ) message_receive_timeout ( pstCHMGKerEvInfoMsgQueue,&timer_temp ))==NULL)
#else
           if((msg_p = ( chmg_KerEvent_Cmd_t * ) message_receive_timeout ( pstCHMGKerEvInfoMsgQueue,TIMEOUT_INFINITY))==NULL)
#endif
           {
                  /*do_report(severity_info,"\n[CHMG_KerEvProcess::] Receive kernel event message timeout\n")*/;		                   
	    }

	   /*do_report(severity_info,"\n[CHMG_KerEvProcess::] Receive Message pointor[%4x]\n",msg_p);*/		 

          if(msg_p != NULL)
          {
                  message_release ( pstCHMGKerEvInfoMsgQueue, msg_p );
		    continue;		  
	   }
 
        
         if ( msg_p != NULL )
         {
             switch(msg_p->from_which_instance)
             {
                    case  CARD_INSTANCE:
		                switch(msg_p->KerEventCode)
		                {
	                               case MGAPIEvClose:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n[Card event process::] MGAPIEvClose Event\n");
#endif				 	
				                   break;
				 
	                               case MGAPIEvExtract:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n [Card event process::]MGAPIEvExtract Event\n");
#endif				
                                              break;
				 
	                               case MGAPIEvBadCard:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n[Card event process::]MGAPIEvBadCard Event\n");
#endif				 	
				                  break;
				 
	                               case MGAPIEvReady:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n[Card event process::] MGAPIEvReady Event\n");
#endif		
				                  break;

	                               case MGAPIEvContent:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n[Card event process::] MGAPIEvContent Event\n");
#endif				 	
				                   break;
				 
	                               case MGAPIEvAddFuncData:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n[Card event process::] MGAPIEvAddFuncData Event\n");
#endif				 	
				                  break;
				 
	                               case MGAPIEvRating:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n [Card event process::]MGAPIEvRating Event\n");
#endif				 	
				                   break;

			                 case MGAPIEvUpdate: /*add this on 041027*/
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n [Card event process::]MGAPIEvRating Event \n");
#endif
			 	                   break;

		                }
				  break;

 		      case  CTRL_INSTANCE:
			  	  do_report(severity_info,"\nCtrl instance event process\n");		
		                switch(msg_p->KerEventCode)
		                {
	                               case MGAPIEvClose:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvClose Event\n");
#endif				 	
				                   break;
				 
	                               case MGAPIEvHalted:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvHalted Event\n");
#endif				 	
                                               /*PairCheckPass = false;*/
				                   break;
				 
	                               case MGAPIEvBadCard:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n MGAPIEvBadCard Event\n");
#endif				 	
				                  break;
				 
	                               case MGAPIEvReady:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n MGAPIEvReady Event\n");
#endif				 	
				                  break;
				 
	                               case MGAPIEvCmptStatus:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvCmptStatus Event\n");
#endif		
				                   break;
				 
	                               case MGAPIEvRejected:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvRejected Event\n");
#endif			
				                   break;
				 
	                               case MGAPIEvAcknowledged:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvAcknowledged Event\n");
#endif	
				                   break;
		                }
				  break;

 		      case SYS_INSTANCE:
			  	 /*do_report(severity_info,"\nSys instance event process\n");		*/
		                switch(msg_p->KerEventCode)
		                {
                                      case MGAPIEvSysError:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvSysError Event\n");
#endif
				                   break;

	                               case MGAPIEvNoResource:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvNoResource Event\n");
#endif				 	
				                   break;
	 
	                               case MGAPIEvDumpText:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvDumpText Event\n");
#endif				 	
				                   break;
				 
	                               case MGAPIEvMsgNotify:
#ifndef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvMsgNotify Event\n");
#endif				 	
				                   break;
				 
	                               case MGAPIEvUnknown:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n MGAPIEvUnknown Event\n");
#endif				 	
				                  break;
				 
	                               case MGAPIEvPassword:
#ifdef                                    CHSYS_DEBUG
                                              do_report(severity_info,"\n MGAPIEvPassword Event\n");
#endif				 	
				                  break;
				 
	                               case MGAPIEvUsrNotify:
#ifdef                                     CHSYS_DEBUG
                                               do_report(severity_info,"\n MGAPIEvUsrNotify Event\n");
#endif		
				                   break;
		                }
				 break;

		      default:
			  	 break;
			 
             }

              message_release ( pstCHMGKerEvInfoMsgQueue, msg_p );
	 }
    }		
}

#endif


/*******************************************************************************
 *Function name: CHMG_KerFunModuleInit
 *           
 *
 *Description: 
 *             
 *             
 *    
 *Prototype:
 *     CHCA_BOOL CHMG_KerFunModuleInit(void)           
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
 *     MGAPISubscribe      
 *            event list related to the "kernel administration" component: 
 *                    MGAPIEvSysError  : when a major system error occurs
 *                    MgAPIEvDumpText : with the request to display a text on the screen
 *                    MgAPIEvNoResource: when a type of resource is no longer available 
 *                    MgAPIEvMsgNotify: on filtering a public EMM
 *                    MgAPIEvUnknown: when an unrecognized smart cad is inswrted in a MG card reader
 *                    MgAPIEvUsrNotify: when specific application data are to be notified
 *                    MgAPIEvPassword:when specific application data are to be notify.
 *           
 *           
 *******************************************************************************/
CHCA_BOOL CHMG_KerFunModuleInit(void) 
{
        TMGAPISubscriptionHandle         hSubscriptionHandle;
	 TMGAPIStatus                           iApiStatus;	
	 BitField32                                 bEvMask;
	 TMGAPIConfig                          KernelConfigPara;
        /*STSMART_Status_t                   *Status_p;		*/
	 /*TMGSysSrcHandle                     hSource;*/

        /*read the stb id */
	 memset(StbIDInfo.CStbId,0xff,CHCA_STBID_MAX_LEN);
		
#ifdef   PAIR_TEST/*modify this on 050119*/
        GetSerialNumTest(StbIDInfo.CStbId);
#else

	 CH_CAReadSTBID(StbIDInfo.CStbId); 

#endif

        StbIDInfo.Enabled = true;
		
 	 /*mg ram init*/
        /*MgKerPartitionInit();*/
        semaphore_wait(pSemMgApiAccess);	
        task_lock();
#ifndef  PAIR_CHECK		
	 /*config the parameter for  the kernel module*/	  
        KernelConfigPara.DscrType= MGAPIDVBCS|MGAPIDES;
        KernelConfigPara.bCheckPairing = false;
        KernelConfigPara.bCheckBlackList = false;
        KernelConfigPara.bCardRatingBypass = false;
        KernelConfigPara.nAdmSubscribers = 1;
        KernelConfigPara.nCtrlInst= 1;
        KernelConfigPara.nCardInst = 1;
       
	 MGAPIInit(&KernelConfigPara);
#else
	 /*config the parameter for  the kernel module*/	  
        KernelConfigPara.DscrType= MGAPIDVBCS|MGAPIDES;
        KernelConfigPara.bCheckPairing = true;
        KernelConfigPara.bCheckBlackList = true;
        KernelConfigPara.bCardRatingBypass = true;
        KernelConfigPara.nAdmSubscribers = 1;
        KernelConfigPara.nCtrlInst= 1;
        KernelConfigPara.nCardInst = 1;

	 MGAPIPairedInit(&KernelConfigPara);
#endif


#ifdef   CHSYS_DEBUG		 
	 do_report(severity_info,"\nDscrType[%d] CheckPairing[%d],CheckBlackList[%d],CardRatingBypass[%d],nAdmSubscribers[%d],nCtrlInst[%d],nCardInst[%d]\n",
	                                        KernelConfigPara.DscrType,
	       	                          KernelConfigPara.bCheckPairing,
				                   KernelConfigPara.bCheckBlackList,
				                   KernelConfigPara.bCardRatingBypass,
				                   KernelConfigPara.nAdmSubscribers,
				                   KernelConfigPara.nCtrlInst,
				                   KernelConfigPara.nCardInst);
#endif
 
        /*register sys event related to the "kernel administration" component*/
        hSubscriptionHandle = MGAPISubscribe(CHMG_SubCallback);
	 semaphore_signal(pSemMgApiAccess);	
            task_unlock();
        if(hSubscriptionHandle!=NULL)
        {
              /*set the sys event mask*/
              bEvMask = MGAPIEvSysErrorMask|\
                               MGAPIEvDumpTextMask|\
                               MGAPIEvNoResourceMask|\
                               MGAPIEvPasswordMask|\
                               MGAPIEvMsgNotifyMask|\
                               MGAPIEvUnknownMask|\
                               MGAPIEvUsrNotifyMask;

	        iApiStatus = MGAPISetEventMask(hSubscriptionHandle,bEvMask);

              if(iApiStatus != MGAPIOk)
              {
#ifdef          CHSYS_DEBUG		 
                    do_report(severity_info,"\n[MGAPISetEventMask]  iSysApiStatus[%d] bEvMask[%d]\n",iApiStatus,bEvMask);
#endif      
                    return true;
		}
        }
	 else
	 {
#ifdef     CHSYS_DEBUG		 
               do_report(severity_info,"\n[MGAPISubscribe] The Interface execution error \n");
#endif      
              return true;
	 }

#if   1 /*remove this to cardprocess on 050124*/
	 if(CHMG_CtrlInstanceInit())
	 {
#ifdef      CHSYS_DEBUG		 
               do_report(severity_info,"\n[CHMG_KerFunModuleInit] fail to init the ctrl instance \n");
#endif      
               return true;
	 }
#endif	 

        if(CHMG_GetCardStatus())
        {
#ifdef     CHSYS_DEBUG		 
               do_report(severity_info,"\n[CHMG_KerFunModuleInit] can not get the card status \n");
#endif      
               return true;
	 }

        return false;
		
  }


#if 1/*add this function */
/*******************************************************************************
 *Function name: CHCA_GetStbID
 *           
 *
 *Description: 
 *             
 *             
 *    
 *Prototype:
 *     boolean  CHCA_GetKernelRelease(char*   KRelease)
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
 *     true :  
 *     flase:  
 *     
 * 
 *
 *Comments:
 *
 *******************************************************************************/
boolean  CHCA_GetKernelRelease(char*   KRelease)
{
        TMGAPIID                KerVersionID;
        TMGAPIStatus           iApiStatus;	
        boolean                    bErrCode=true;
        int                            i,j;

        if(KRelease==NULL)
        {
		return bErrCode;	  
 	 }

        semaphore_wait(pSemMgApiAccess);	
        iApiStatus = MGAPIGetID(&KerVersionID);
	 semaphore_signal(pSemMgApiAccess);	
       	
	 switch(iApiStatus)
	 {
              case MGAPIOk:
			  bErrCode=false;	
			  memcpy(KRelease,KerVersionID.Ver[0].Release,16);
#if   0
			  for(j=0;j<15;j++)
                               do_report(severity_info,"%c",KRelease[j]);
                            do_report(severity_info,"\n");
							
			  for(i=0;i<8;i++)
			  {
                            do_report(severity_info,"\nVerName::");
			       for(j=0;j<24;j++)
                               do_report(severity_info,"%c",KerVersionID.Ver[i].Name[j]);
                            do_report(severity_info,"\n");
							
                            do_report(severity_info,"\nData::");
			       for(j=0;j<9;j++)
                               do_report(severity_info,"%c",KerVersionID.Ver[i].Date[j]);
                            do_report(severity_info,"\n");
                            do_report(severity_info,"\nVerName::");
			       for(j=0;j<15;j++)
                               do_report(severity_info,"%c",KerVersionID.Ver[i].Release[j]);
                            do_report(severity_info,"\n");
			  }
#endif			  
			  break;

		case MGAPIInvalidParam:
			 break;

		case MGAPINotAvailable:
			 break;

		default:
			break;
	 }
       	  
        return bErrCode;	   

}



#if  1 /*add this on 050511*/
/*******************************************************************************
 *Function name: CHCA_GetPairRelease
 *           
 *
 *Description: get the version of the mediaguard kernel pairing
 *             
 *             
 *    
 *Prototype:
 *     boolean  CHCA_GetPairRelease(char*   PRelease)
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
 *     true :  
 *     flase:  
 *     
 * 
 *
 *Comments:
 *
 *******************************************************************************/
boolean  CHCA_GetPairRelease(char*   PRelease)
{
        boolean                    bErrCode=false;
        int                            i,j;
	#if 0/*20070102 change*/
	 char                         PairRelease[16]={'/','0','1','.','0','1','R','0','2'};	
            #else
	 char                         PairRelease[16]={'/','0','1','.','0','1','R','0','7'};	

	#endif
        if(PRelease==NULL)
        {
              bErrCode = true;
		return bErrCode;	  
 	 }

        memcpy(PRelease,PairRelease,16);
        return bErrCode;	  


}
#endif

#endif
/*******************************************************************************
 *Function name: CHCA_GetStbID
 *           
 *
 *Description: 
 *             
 *             
 *    
 *Prototype:
 *     CHCA_BOOL  CHCA_GetStbID(CHCA_UCHAR*   iStbID)
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
 *       for application    
 *******************************************************************************/
CHCA_BOOL  CHCA_GetStbID(CHCA_UCHAR*   iStbID)
{
      CHCA_BOOL       bErrCode = false;
      CHCA_INT          i,w,serialnum=0;
      CHCA_UCHAR    YearTemp,MonthTemp,DayTemp;
 
      if(iStbID==NULL)
      {
             bErrCode = true;
             do_report(severity_info,"\n[CHCA_GetStbID::] the input parameter is wrong\n");	   
             return bErrCode;	   
      }
	  
      if(StbIDInfo.Enabled)
      {
#if 0
             for(w=0;w<8;w++)
             {
                  do_report(severity_info,"\nCHCA_GetStbID==>");
		    do_report(severity_info,"%4x",StbIDInfo.CStbId[w]);
                  do_report(severity_info,"\n");
	      }
#endif	  


#if      /*PLATFORM_16 20070514 change*/1
            iStbID[7] = 0x01;
            iStbID[6] = StbIDInfo.CStbId[6];
            iStbID[5] = StbIDInfo.CStbId[5];
            iStbID[4] = StbIDInfo.CStbId[4];

#if 0			
            DayTemp = ((StbIDInfo.CStbId[3] & 0xf0)>>4) |(( StbIDInfo.CStbId[2]&0x1)<<4);
            MonthTemp = (StbIDInfo.CStbId[2]&0x1f)>>1;	   
            YearTemp = ((StbIDInfo.CStbId[1]&0xf)<<3) |((StbIDInfo.CStbId[2]&0xe0)>>5)+10;
            
            do_report(severity_info,"\nYearTemp[%d] MonthTemp[%d]DayTemp[%d]\n",YearTemp,MonthTemp,DayTemp);
            
            iStbID[3] = (StbIDInfo.CStbId[3]&0xf)|((DayTemp&0x7)<<5)|0x10;
            iStbID[2] = ((DayTemp&0x18)>>3)|((MonthTemp&0xf)<<2)|((YearTemp&0x3)<<6);
            iStbID[1] =  (YearTemp&0x7c)>>2;
            
            iStbID[0] = 0x8; 
#else
            iStbID[3] = StbIDInfo.CStbId[3];
            iStbID[2] = StbIDInfo.CStbId[2];
            iStbID[1] = StbIDInfo.CStbId[1];
            iStbID[0] = StbIDInfo.CStbId[0];
#endif
			
#endif
      }
      else
      {
           bErrCode = true;
	    do_report(severity_info,"\n[CHCA_GetStbID::] read stb id operation is wrong\n");	   
	    return bErrCode;	   
      }
       	  
      return bErrCode;	   

}


/*for kernel call*/
/*******************************************************************************
 *Function name: CHCA_MgGetStbID
 *           
 *
 *Description: the general interface for the nagra france ca 
 *             
 *             
 *    
 *Prototype:
 *     CHCA_BOOL  CHCA_MgGetStbID(CHCA_UCHAR*   iStbID)
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
CHCA_BOOL  CHCA_MgGetStbID(CHCA_UCHAR*   iStbID)
{
      CHCA_BOOL       bErrCode = false;
      CHCA_INT          i,serialnum=0;
      CHCA_UCHAR    YearTemp,MonthTemp,DayTemp;
 
      if(iStbID==NULL)
      {
             bErrCode = true;
             do_report(severity_info,"\n[CHCA_GetStbID::] the input parameter is wrong\n");	   
             return bErrCode;	   
      }
	  
      if(StbIDInfo.Enabled)
      {
#ifdef    PAIR_TEST
             memcpy(iStbID,ChSTBID_Test,CHCA_STBID_MAX_LEN);
#else
             memcpy(iStbID,StbIDInfo.CStbId,CHCA_STBID_MAX_LEN);
#endif
      }
      else
      {
            bErrCode = true;
	     do_report(severity_info,"\n[CHCA_GetStbID::] read stb id operation is wrong\n");	   
	     return bErrCode;	   
      }
       	  
      return bErrCode;	   

}



/*******************************************************************************
 *Function name: CHCA_GetSTBVersion
 *           
 *
 *Description: get the version of the STB
 *             
 *             
 *    
 *Prototype:
 *     CHCA_BOOL   CHCA_GetSTBVersion(CHCA_UCHAR*  HVersion,CHCA_UCHAR*  SVersion)
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

CHCA_BOOL   CHCA_GetSTBVersion(CHCA_UCHAR*  HVersion,CHCA_UCHAR*  SVersion)
{
      CHCA_BOOL      bErrCode = false;
      CHCA_UCHAR    STBSoftVersionMajor;	  
      CHCA_UCHAR    STBSoftVersionMinor;
	  
 
      if((HVersion==NULL) ||(SVersion==NULL))
      {
             bErrCode = true;
	      return bErrCode;		 
      }

      GetSTBVersion(&STBSoftVersionMajor,&STBSoftVersionMinor);/*add this on 050123*/

#if      /*PLATFORM_16 20070514 change*/1
      HVersion[0] = 0x12; /*the manufacturer code*/  
      HVersion[1] = 0x01;   /*byte 2 and 1: the hardware version number,MINOR VERSION*/	  
      HVersion[2] = 0x1;  /*MAJOR VERSION*/		  
      HVersion[3] = 0x00; /*is reserved for future use and must be set to 00*/	
#endif	




      SVersion[0] = 0x00; /*byte 0 and 1: the ddi specification version*/ 
      SVersion[1] = 0x01;     
      SVersion[2] = STBSoftVersionMinor;	  
      SVersion[3] = STBSoftVersionMajor; /*byte 2 and 3: the manufacture software number*/  
	  
      return bErrCode;
	  
}


/*******************************************************************************
 *Function name: CHCA_InhibitStb
 *           
 *
 *Description: This function inhibits the terminal.The actions needed to inhibit
 *             the terminal are left to the manufacturer discretion.The following
 *             actions including in the STB inhibiton are described as a recommendation 
 *             and/or can be read as an example:
 *                1) Remote control access and command are immediately inhibited.From 
 *                   this call,any user action on the remote control are not taken
 *                   into account by the STB
 *                2) Front panel access and command are immediately inhibited.From
 *                   this interface,any user action on the front panel are not taken
 *                   into account by the STB
 *                3) The STB are put in standby mode 2 minutes after the call to this 
 *                   interface.If the STB has not been designed with a standby mode,a 
 *                   blank screen may be displayed imitating a standby mode.The two 
 *                   minutes are defined for the Mediaguard kernel to display a message
 *
 *             These inhibition actions may be handled in the order described in the 
 *             above list.Only the power off and on of the STB might restore these 
 *             functionnalities.Software reset and hardware reset if possible may be
 *             inhibited.
 *
 *Prototype:
 *      CHCA_BOOL  CHCA_InhibitStb(void)  
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
 *Comments:
 *           
 *******************************************************************************/
 CHCA_BOOL  CHCA_InhibitStb(void)
 {
         /*do_report(severity_info,"\n MGDDIInhibitSTB  \n");*/
	  CHCA_BOOL             bErrCode=false;


 	  CH_SuspendUsif(); 	 

    
	  return  bErrCode;	 
 
 } 

/*******************************************************************************
 *Function name: CHCAModuleSetup
 *           
 *
 *Description: the general interface for the nagra france ca initialise
 *             
 *             
 *    
 *Prototype:
 *     boolean  CHCAModuleSetup(void)
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
boolean  CH_CAModuleSetup(void)
{
        /*read the stb id from the flash*/

#if 0		
        if(ChKerEvProcessInit())
        {
#ifdef    CHSYS_DEBUG		 
              do_report(severity_info,"\n[CHMG_KerFunModuleInit::] fail to creat the kernel event process \n");
#endif        
              return true;
	 }
#endif
        /*创建卡处理模块*/
         if(CHCA_CardInit())
	 {
	        do_report(0,"CHCA_CardInit error\n");
               return true;
	 }			

       /*创建CA数据接收模块*/
        if(CHCA_DemuxInit())
        {
               do_report(0,"CHCA_DemuxInit error\n");
               return true;
        }

       /*创建CA数据处理进程*/
        if(CHCA_ProgInit())
        {
               do_report(0,"CHCA_ProgInit error\n");
               return true;
        }
		
       /*创建CA内存区*/
        CHCA_PartitionInit();	

       /*创建CA定事器模块*/
	 if(CHCA_TimerInit())
	 {
	         do_report(0,"CHCA_TimerInit error\n");
               return true;
	 } 

	 if(CHCA_MutexInit())
	 {
	 	 do_report(0,"CHCA_MutexInit error\n");
               return true;
	 }
	     /*MG KERNEL初始化*/
        if(CHMG_KerFunModuleInit())
        {
               do_report(0,"CHMG_KerFunModuleInit error\n");

               return true;
	 }
        return  false;
		
}




