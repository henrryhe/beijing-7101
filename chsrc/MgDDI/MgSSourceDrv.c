#ifdef    TRACE_CAINFO_ENABLE
#define  MGSSOURCE_DRIVER_DEBUG
#endif


/******************************************************************************
*
* File : MgSSourceDrv.C
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
* History : 0.0   2004-06-08  Start coding
*              1.0   2006-12-09  change
*
* Copyright: Changhong 2004 (c)
*
*****************************************************************************/

/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/
#include  "MgSSourceDrv.h"


/*****************************************************************************
 * Variable description
 *****************************************************************************/
boolean                                       bDscrConfiged = false;

CHCA_INT                                  EcmFilterNum=0;/*add this on 050313*/

#if 0
TMGDDISrcDmxFltrHandle            hFilterTemp;   
TMGDDISrcDscrChnlHandle          hChannelTemp;
#endif


/*****************************************************************************
 * Interfaces description
 *****************************************************************************/

/*******************************************************************************
 *Function name: MGDDISrcGetSources
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
 *    TMGDDIStatus  MGDDISrcGetSources       
 *          (TMGSysSrcHandle* lhSource,uword* nSources)     
 *
 *
 *input:
 *    lhSource:            Address of system source handle list       
 *    nSource:             Address of number of sources.        
 *
 *output:
 *
 *Returned code:
 *    MGDDIOk:         The list of sources has been filled.       
 *    MGDDIBadParam:   The nSources parameter is null       
 *    MGDDINoResource: The size of the memory block is insufficient.       
 *    MGDDIError:      Execution error or source list unavailable       
 *           
 *           
 *           
 *Comments:           
 *           
 *           
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDISrcGetSources  
     (TMGSysSrcHandle* lhSource,uword* nSources)
{
       TMGDDIStatus     StatusMgDdi;
 
       StatusMgDdi = (TMGDDIStatus)CHCA_CtrlGetSources((TCHCASysSrcHandle *)lhSource,nSources);

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
TMGDDIStatus MGDDISrcGetCaps
   (TMGSysSrcHandle hSource,TMGDDISrcCaps* pCaps)
{
       TMGDDIStatus     StatusMgDdi;

       if((hSource==NULL)||(pCaps==NULL))
       {
              StatusMgDdi =MGDDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcGetCaps==>] A parameter is unknown,null or invalid \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}
	   
       StatusMgDdi = (TMGDDIStatus)CHCA_CtrlGetCaps((TCHCASysSrcHandle)hSource);
	   
  	if(StatusMgDdi != MGDDIOK )
	{
              StatusMgDdi =MGDDIError;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcGetCaps==>]  Interface execution error \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}

	pCaps->hSource = hSource;

       pCaps->Type = MGDDIEMMSource |MGDDIECMSource;

#ifdef    MGSSOURCE_DRIVER_DEBUG			  
       do_report(severity_info,"\n %s %d >[MGDDISrcGetCaps==>] hSource[%4x] Type[%4x]\n",
                         __FILE__,
                         __LINE__,
                         pCaps->hSource,
                         pCaps->Type);
#endif

	return (StatusMgDdi);
}



/*******************************************************************************
 *Function name: MGDDISrcGetStatus
 *           
 *
 *Description: this function returns the status of the source related to the hSource handle
 *             
 *
 *Prototype:
 *     TMGDDIStatus MGDDISrcGetStatus(TMGSysSrcHandle  hSource)      
 *           
 *input:
 *     hSource:    System source handle.
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:          The source is connected.       
 *    MGDDINotFound:    The source is disconnected       
 *    MGDDIBadParam:    Source handle unknown.       
 *    MGDDIError:       Interface execution error.       
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
 TMGDDIStatus MGDDISrcGetStatus(TMGSysSrcHandle  hSource)
 {
        TMGDDIStatus     StatusMgDdi = MGDDIOK;

        StatusMgDdi = (TMGDDIStatus)CHCA_CtrlGetStatus((TCHCASysSrcHandle)hSource);
  
	 return (StatusMgDdi); 
 
 }



/*******************************************************************************
 *Function name: MGDDISrcSubscribe
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
 *     TMGDDIEventSubscriptionHandle  MGDDISrcSubscribe      
 *           ( TMGSysSrcHandle  hSource,CALLBACK hCallback)   
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
 TMGDDIEventSubscriptionHandle  MGDDISrcSubscribe
       ( TMGSysSrcHandle  hSource,CALLBACK hCallback)
 {
       TMGDDIEventSubscriptionHandle        SubscriptionHandle; 

	CHCA_UCHAR                                   SubHandle[4];

       WORD2BYTE                                      iSubTemp;


       if((hSource==NULL)||(hCallback==NULL))
       {
              do_report(severity_info,"\n[MGDDISrcSubscribe==>] the parameter is wrong\n");
              return NULL;
	}

	if(CHCA_CtrlSubscribe(( TCHCASysSrcHandle)hSource,hCallback,SubHandle)) 
	{
             return NULL;
	}
	else
	{
               iSubTemp.byte.ucByte0=SubHandle[0];
		 iSubTemp.byte.ucByte1=SubHandle[1];
		 iSubTemp.byte.ucByte2=SubHandle[2];
		 iSubTemp.byte.ucByte3=SubHandle[3];
		
               SubscriptionHandle = (TMGDDIEventSubscriptionHandle)iSubTemp.uiWord32;
 	}

       return SubscriptionHandle;   
 }



/*******************************************************************************
 *Function name: MGDDISrcGetDscrCaps
 *           

 *Description: This function indicates the capacities of the descrambler of the 
 *             source associated to the hSource handle.The capacities are recorded
 *             in the information structure provided at the address given in the  
 *             pCaps parameter.
 * 
 *Prototype:
 *     TMGDDIStatus  MGDDISrcGetDscrCaps      
 *         (TMGSysSrcHandle hSource,TMGDDIDscrCaps* pCaps)  
 *
 *
 *input:
 *    hSource:    System source handle related to the descrambler       
 *    pCaps:      Address of an information structure of a descrambler       
 *
 *output:
 *           
 *           
 *Returned code:
 *    MGDDIOk:         The descrambler information has been filled       
 *    MGDDIBadParam:   The source handle is unknown,or the parameter is null       
 *    MGDDIError:      Interface execution error,the information is not        
 *                     available.
 *           
 *Comments: 
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDISrcGetDscrCaps
     (TMGSysSrcHandle hSource,TMGDDIDscrCaps* pCaps)
 {
       TMGDDIStatus     StatusMgDdi = MGDDIOK;

	if((hSource == NULL)||(pCaps==NULL))
       {
             StatusMgDdi = MGDDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
             do_report(severity_info,"\n %s %d >[MGDDISrcGetDscrCaps==>] The source handle is unknown,or the parameter is null \n",
                              __FILE__,
                              __LINE__);
#endif
              return (StatusMgDdi);		  
	}

       pCaps ->hSource = hSource;
	pCaps ->Type = MGDDIDVBCS | MGDDIDES;
	pCaps ->nChannels = CHCA_MAX_DSCR_CHANNEL;
       pCaps ->nPIDPerChannel = CHCA_SlOTORPID_NUM_PERKEY;  /*maximum number of pids or slot per hardware channel is two*/

#ifdef    MGSSOURCE_DRIVER_DEBUG			  
       do_report(severity_info,"\n %s %d >[MGDDISrcGetDscrCaps==>] handle[%4x],Type[%d],MaxChandel[%d],PidNum[%d]\n",
                         __FILE__,
                         __LINE__,
                         pCaps ->hSource,
                         pCaps ->Type,
                         pCaps ->nChannels,
                         pCaps ->nPIDPerChannel);
#endif
	   
	return (StatusMgDdi);	
 
 }

/*******************************************************************************
 *Function name :  MGDDISrcInitDscr
 *           

 *Description: This function defines the configuration parameters of the descrambler
 *             related to the hSource handle.These parameters are provided in the 
 *             structure at the address given in the pSelPrms parameter.If at least  
 *             one of the configuration parameters cannot be configured on the 
 *             descrambler,the MGDDINotSupported code is returned.
 *
 *Prototype:
 *     TMGDDIStatus  MGDDISrcInitDscr       
 *          (TMGSysSrcHandle  hSource,TMGDDIDscrSelect*  pSelPrms)  
 *
 *
 *input:
 *    hSource:    System source handle related to descrambler           
 *    pSelPrms:   Address of descrambler selection parameters structure.       
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:             The descrambler information has been filled.       
 *    MGDDIBadParam:       Parameter null,aberrant or wrongly formatted.       
 *    MGDDINotSupported:   The provided configuration is not supported by the descrambler.      
 *    MGDDIError:          Interface execution error       
 *           
 *           
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDISrcInitDscr
     (TMGSysSrcHandle  hSource,TMGDDIDscrSelect*  pSelPrms)
 {
       TMGDDIStatus      StatusMgDdi = MGDDIOK;
	
	if((hSource==NULL)||(hSource!=(TMGSysSrcHandle)MGTUNERDeviceName)||(pSelPrms==NULL))
	{
              StatusMgDdi =MGDDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcInitDscr==>] Parameter null,aberrant or wrongly formatted \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}	

       semaphore_wait(psemMgDescAccess);
       if(!(pSelPrms->Type&(MGDDIDVBCS|MGDDIDES))||(pSelPrms->nKeys > CHCA_MAX_DSCR_CHANNEL)||(pSelPrms->KeySize > CHCA_MAX_KEY_SIZE))
       {
             StatusMgDdi = MGDDINotSupported;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcInitDscr==>]  The provided configuration is not supported by the descrambler\n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}

       bDscrConfiged = true;

       CHCA_Descrambler_Init();
	   
	semaphore_signal(psemMgDescAccess);
   
	   
       return (StatusMgDdi);
	
 }



/*******************************************************************************
 *Function name : MGDDISrcGetFreeDscrChnl
 *           
 *
 *Description: This function indicates the number of remaining free channels on 
 *             the descrambler related to the source identified by hSource handle.
 *             This number is returned at the address given in the nChannel parameter.
 *
 *Prototype:
 *     TMGDDIStatus  MGDDISrcGetFreeDscrChnl      
 *          (TMGSysSrcHandle hSource,uword* nChannel)   
 *
 *
 *input:
 *    hSource:    System source handle related to descrambler          
 *    nChannel:   Address of number of free channels to be filled       
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:                 Execution has processed correctly       
 *    MGDDIBadParam:      The source handle is unknown, or the parameter is null    
 *    MGDDIError:              Interface execution error.       
 *           
 *           
 *           
 *           
 *Comments: 
 *           
 *           
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDISrcGetFreeDscrChnl
     (TMGSysSrcHandle hSource,uword* nChannel)
{
       TMGDDIStatus     StatusMgDdi = MGDDIOK;

	if((hSource!=(TMGSysSrcHandle)MGTUNERDeviceName)||(hSource==NULL)||(nChannel==NULL))
	{
              StatusMgDdi =MGDDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcGetFreeDscrChnl==>] The source handle is unknown, or the parameter is null  \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}

       semaphore_wait(psemMgDescAccess);
	*nChannel = CHCA_Get_FreeKeyCounter();
	semaphore_signal(psemMgDescAccess);
         
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
        do_report(severity_info,"\n %s %d >[MGDDISrcGetFreeDscrChnl==>] the number[%d] of remaining free channels  \n",
                          __FILE__,
                          __LINE__,
                          *nChannel);
#endif
	   
	 return (StatusMgDdi);
}	 



/*******************************************************************************
 *Function name: MGDDISrcAllocDscrChnl
 *           

 *Description: This interface reserves a descrambler channel on the source related
 *             to the hSource handle.The descrambler handle returned is an absolute 
 *             handle,i.e. a handle independent from the source
 *
 *Prototype:
 *     TMGDDISrcDscrChnlHandle  MGDDISrcAllocDscrChnl(TMGSysSrcHandle hSource)      
 *
 *input:
 *   hSource:   System source handle related to descrambler               
 *           
 *
 *output:
 *           
 *Returned code:
 *    Descrambling channel handle, if not null            
 *    Null, if a problem has occurred during execution of the function:       
 *           - system source handle unknown,
 *           - descrambler has not been configured
 *           - there are no longer any free descrambling channels 
 *           - a problem has occurred in reservation of the channel.
 *           
 *Comments: 
 *    The channel is simply allocated,but not started.       
 *           
 *******************************************************************************/
TMGDDISrcDscrChnlHandle  MGDDISrcAllocDscrChnl(TMGSysSrcHandle    hSource)
 {
        CHCA_USHORT                                   DscrChId;

	 if((hSource!=(TMGSysSrcHandle)MGTUNERDeviceName)||(hSource==NULL))
	 {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcAllocDscrChnl==>] system source handle unknown\n",
                               __FILE__,
                               __LINE__);
#endif
		return NULL;	  
	 }

        semaphore_wait(psemMgDescAccess);
	 if(!bDscrConfiged)
	 {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcAllocDscrChnl==>] descrambler has not been configured\n",
                               __FILE__,
                               __LINE__);
#endif
              semaphore_signal(psemMgDescAccess);
		return NULL;	  
	 }

        if(CHCA_Allocate_Descrambler(&DscrChId))
        {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcAllocDscrChnl==>] here are no longer any free descrambling channels\n",
                               __FILE__,
                               __LINE__);
#endif
              semaphore_signal(psemMgDescAccess);
		return NULL;	  
	 }

        DscrChId = DscrChId + 1;

#ifdef    MGSSOURCE_DRIVER_DEBUG			  
        do_report(severity_info,"\n %s %d >[MGDDISrcAllocDscrChnl==>] the allocated descrambler channel index[%d]\n",
                          __FILE__,
                          __LINE__,
                          DscrChId);
#endif
        semaphore_signal(psemMgDescAccess);

 	 return (TMGDDISrcDscrChnlHandle )DscrChId;
        		
  }



/*******************************************************************************
 *Function name: MGDDISrcFreeDscrChnl
 *           
 *
 *Description: This interface release the descrambler channel associated to the
 *             hChannel handle.The related hardware resource is freed and available
 *             for the system. 
 * 
 * 
 *Prototype:
 *      TMGDDIStatus  MGDDISrcFreeDscrChnl(TMGDDISrcDscrChnlHandle  hChannel)     
 *           
 *
 *
 *input:
 *      hChannel:   Descrambler channel handle     
 *           
 *
 *output:
 *           
 *Returned code:
 *      MGDDIOk:         The channel has been released.     
 *      MGDDIBadParam:   Descrambler channel handle unknown.      
 *      MGDDIError:      Interface execution error.     
 *           
 *           
 *           
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDISrcFreeDscrChnl(TMGDDISrcDscrChnlHandle  hChannel)
{
        U32                            DscrChId;
        TMGDDIStatus            StatusMgDdi = MGDDIOK;

        semaphore_wait(psemMgDescAccess);
	 
	 DscrChId = (U32)hChannel-1;
	   
	 if((hChannel==0)||(DscrChId>=CHCA_MAX_DSCR_CHANNEL))
	 {
              StatusMgDdi = MGDDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcFreeDscrChnl==>] Descrambler channel handle unknown or the channel has already been released \n",
                                __FILE__,
                                __LINE__);
#endif
              semaphore_signal(psemMgDescAccess);
              return (StatusMgDdi); 
	 }

        if(CHCA_Deallocate_Descrambler(DscrChId)==true)
        {
                StatusMgDdi = MGDDIError;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcFreeDscrChnl==>] Deallocate Descrambler Interface execution error\n",
                                  __FILE__,
                                  __LINE__);
#endif
	 }
	 else
	 {
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcFreeDscrChnl==>] DscrChId[%d] has been released \n",
                                 __FILE__,
                                 __LINE__,
                                 DscrChId);
#endif
	 }

        semaphore_signal(psemMgDescAccess);
 	 return (StatusMgDdi); 
	 
}



/*******************************************************************************
 *Function name: MGDDISrcStartDscrChnl
 *           

 *Description: This interface starts the descrambling process on the descrambler
 *             channel related to the hChannel handle 
 *
 *Prototype:
 *     TMGDDIStatus  MGDDISrcStartDscrChnl( TMGDDISrcChnlHandle  hChannel)
 *
 *
 *input:
 *     hChannel:   Descrambler channel handle      
 *           
 *
 *output:
 *           
 *Returned code:
 *     MGDDIOk:          The channel has been started.      
 *     MGDDIBadParam:    Descrambler channel handle unkonwn.     
 *     MGDDIError:       Interface execution error.      
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
#if 0 
void   CheckSlotInfo(CHCA_UINT  iDscrChId,U32  SlotValue)
{
       CHCA_UINT             i,j;
	CHCA_BOOL            FindSlot=false;   
	   
        for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)
        {
               if((i==iDscrChId)||(!DscrChannel[i].DscrChAllocated))
               {
                      continue;
               }

               for(j=0;j<2;j++)
               {
                      if(SlotValue == DscrChannel[i].DscrSlotInfo[j].SlotValue)
                      {
                             FindSlot = true;
                             CHCA_Stop_Descrambler(SlotValue); 
				 DscrChannel[i].DscrSlotInfo[j].DscrStarted = false;	
				 do_report(severity_info,"\n[CheckSlotInfo::] The SlotVaule has already been started on Descrambler[%d]Index[%d]\n",DscrChannel[i].Descramblerhandle,i);
				 break; 
			 }
		 }

		 if(FindSlot)	
		 {
                    return;
		 }
 
       }

	return;

}
#endif

TMGDDIStatus  MGDDISrcStartDscrChnl( TMGDDISrcDscrChnlHandle  hChannel)
{         
       
       TMGDDIStatus                        StatusMgDdi = MGDDIOK;

#if 0
	 CHCA_UINT                            DscrChId;
	 CHCA_UCHAR                       i,j,w;  
#endif
	 

#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcStartDscrChnl==>] Start DscrChnl \n",
                                __FILE__,
                                __LINE__);
#endif  	
	 

	return StatusMgDdi;   

#if 0
       semaphore_wait(psemMgDescAccess);
       DscrChId = (U32)hChannel - 1; 

	  
       if((hChannel==0) ||(DscrChId>=CHCA_MAX_DSCR_CHANNEL))
       {
	        StatusMgDdi = MGDDIBadParam;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcStartDscrChnl::] Descrambler channel handle unknown \n",
                                __FILE__,
                                __LINE__);
#endif  	
               semaphore_signal(psemMgDescAccess);
               return (StatusMgDdi);
	}

       for(i=0;i<2;i++)
       {  
                     CheckSlotInfo(DscrChId,DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue);
					 
			/*if(DscrChannel[DscrChId].DscrSlotInfo[i].DscrAssociated)*/
			{
                            if(CHCA_Start_Descrambler(DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue)==true)
                            {
#ifdef                         MGSSOURCE_DRIVER_DEBUG			  
                                   do_report(severity_info,"\n %s %d >[MGDDISrcStartDscrChnl::] Interface execution error   \n",
                                                    __FILE__,
                                                    __LINE__);
#endif	
                            }
                            else
                            {
#ifdef                         MGSSOURCE_DRIVER_DEBUG			  
                                   do_report(severity_info,"\n %s %d >[MGDDISrcStartDscrChnl::] Start the Slot[%d] of the descrambler[%d] \n",
                                                    __FILE__,
                                                    __LINE__,
                                                   DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue,
                                                   DscrChannel[DscrChId].Descramblerhandle);
#endif	
                                   DscrChannel[DscrChId].DscrSlotInfo[i].DscrStarted = true;	
                            }
			 }
			 /*else
			 {
                            do_report(severity_info,"\n[MGDDISrcStartDscrChnl::] the slot[%d] has been started\n",DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue);
			 }*/
	   	   
       }   
	
       semaphore_signal(psemMgDescAccess);
       return (StatusMgDdi);
#endif	   
	  
}

/*******************************************************************************
 *Function name: MGDDISrcStopDscrChnl
 *           

 *Description:This interface starts the descrambling proces on the descrambler
 *            channel related to the hChannel handle.
 *
 *Prototype:
 *    TMGDDIStatus  MGDDISrcStopDscrChnl(TMGDDISrcDscrChnlHandle  hChannel)
 *       
 *
 *
 *input:
 *    hChannel:    Descrambler channel handle.
 *           
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:         The channel has been stopped.       
 *    MGDDIBadParam:   Descrambler channel handle unknown.       
 *    MGDDIError:      Interface execution error       
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
 TMGDDIStatus  MGDDISrcStopDscrChnl(TMGDDISrcDscrChnlHandle  hChannel)
{
      
       TMGDDIStatus            StatusMgDdi = MGDDIOK;	

#if 0
	 CHCA_UINT                            DscrChId;
	 CHCA_UCHAR                        i;
#endif	   

#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcStopDscrChnl==>] Stop DscrChnl \n",
                                __FILE__,
                                __LINE__);
#endif  	
	   
        return StatusMgDdi;
 
#if 0	  
       semaphore_wait(psemMgDescAccess);
   
       DscrChId = (U32)hChannel - 1; 	 	
      
 
	   
       if((hChannel==0) ||(DscrChId>=CHCA_MAX_DSCR_CHANNEL))
       {
	        StatusMgDdi = MGDDIBadParam;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcStopDscrChnl::] Descrambler channel handle unknown \n",
                                __FILE__,
                                __LINE__);
#endif  	
               semaphore_signal(psemMgDescAccess);
               return (StatusMgDdi);
	}


#ifdef       MGSSOURCE_DRIVER_DEBUG			  
       do_report(severity_info,"\n[MGDDISrcStopDscrChnl::] Slot[%d][%d] Descrambler[%d]\n",
                                              DscrChannel[DscrChId].DscrSlotInfo[0].SlotValue,
                                              DscrChannel[DscrChId].DscrSlotInfo[1].SlotValue,
                                              DscrChannel[DscrChId].Descramblerhandle);
#endif	
	  
       for(i=0;i<2;i++)
       {
             if(DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue != CHCA_DEMUX_INVALID_SLOT) 
             {
                   /* if(!DscrChannel[DscrChId].DscrSlotInfo[i].DscrAssociated)*/
                    { 
                          if(CHCA_Stop_Descrambler(DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue)==true)
                          {
                                StatusMgDdi =  MGDDIError;
#ifdef                      MGSSOURCE_DRIVER_DEBUG			  
                                do_report(severity_info,"\n %s %d >[MGDDISrcStopDscrChnl::] Interface execution error   \n",
                                                 __FILE__,
                                                 __LINE__);
#endif	
                                semaphore_signal(psemMgDescAccess);
                                return (StatusMgDdi);
	                   }
		            else
		            {
#ifdef                       MGSSOURCE_DRIVER_DEBUG			  
                                 do_report(severity_info,"\n %s %d >[MGDDISrcStopDscrChnl::] stop the Descrambler[%d] channel[%d]\n",
                                                   __FILE__,
                                                   __LINE__,
                                                  DscrChannel[DscrChId].Descramblerhandle,
                                                  DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue);
#endif
                                 DscrChannel[DscrChId].DscrSlotInfo[i].DscrStarted = false; 
		            }
                    }
                    /*else
                    {
                           do_report(severity_info,"\nthe slot[%d] associated to descrambler is already stopped\n",DscrChannel[DscrChId].DscrSlotInfo[i].SlotValue);
                    }*/
					
	       }
       }
       semaphore_signal(psemMgDescAccess);

	   
       return (StatusMgDdi);
#endif	   
	   
}



/*******************************************************************************
 *Function name: MGDDISrcFlushDscrChnl
 *           

 *Description: this function removes the control words currently loaded from the
 *             the descrambler related to the hChannel handel.As with MGDDIStopDscrChnl
 *             interface.If the descrambler channel has been started,the descrambling
 *             process resumes as soon as new control words are loaded
 *
 *Prototype: 
 *    TMGDDIStatus   MGDDISrcFlushDscrChnl(TMGDDISrcDscrChnlHandle hChannel)       
 *           
 *input:
 *    hChannel:   Descrambler channel handle          
 *           
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:          Temporary interruption of descrambling has been processed           
 *    MGDDIBadParam:    Descrambler channel handle unknown       
 *    MGDDIError:       Interface execution error.       
 *           
 *Comments: 
 *        
 *           
 *******************************************************************************/
 TMGDDIStatus   MGDDISrcFlushDscrChnl(TMGDDISrcDscrChnlHandle hChannel)
 {
      U8                      Invalid_keys[ 8 ] = { 0,0,0,0,0,0,0,0 };
      TMGDDIStatus     StatusMgDdi = MGDDIOK;	
      U32                    DscrChId;

#ifdef     MGSSOURCE_DRIVER_DEBUG			  
      do_report(severity_info,"\n[MGDDISrcFlushDscrChnl==>] \n");
#endif

      return    StatusMgDdi;

      semaphore_wait(psemMgDescAccess);
	  
      DscrChId = (U32)hChannel-1;

      if((hChannel==0) ||(DscrChId>=CHCA_MAX_DSCR_CHANNEL))
      {
	        StatusMgDdi =MGDDIBadParam;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcFlushDscrChnl==>] Descrambler channel handle unknown \n",
                                __FILE__,
                                __LINE__);
#endif  	
               semaphore_signal(psemMgDescAccess);

               return (StatusMgDdi);
	}
	  
	if(CHCA_Set_EvenKey2Descrambler(DscrChId,Invalid_keys)==true)
	{
              StatusMgDdi = MGDDIError;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcFlushDscrChnl==>] Interface execution error \n",
                               __FILE__,
                               __LINE__);
#endif 
              semaphore_signal(psemMgDescAccess);

              return(StatusMgDdi);
	}

	if(CHCA_Set_OddKey2Descrambler(DscrChId,Invalid_keys)==true)
	{
              StatusMgDdi = MGDDIError;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcFlushDscrChnl==>] Interface execution error \n",
                               __FILE__,
                               __LINE__);
#endif 
              semaphore_signal(psemMgDescAccess);

              return(StatusMgDdi);
	}
  
       semaphore_signal(psemMgDescAccess);

       return(StatusMgDdi);	  
	  
 }

/*******************************************************************************
 *Function name: MGDDISrcLoadCWToDscrChnl
 *           

 *Description: This function loads pairs of control words into the descrambler 
 *             channel related to the hChannel handle.Descrambling process must
 *             first have been started on this channel. The control word pairs are
 *             provided in the table at the address given in the pCWPeerTable
 *             parameter   
 *   
 *Prototype:
 *      TMGDDIStatus  MGDDISrcLoadCWToDscrChnl     
 *           (TMGDDISrcDscrChnlHandle  hChannel,TMGDDICWPeers*  pCWPeerTable)
 *
 *
 *input:
 *    hChannel:      Descrambler channel handle.
 *    pCWPeerTable:  Address of control words table.
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:         The control words have been loaded into the channel.            
 *    MGDDIBadParam:   Descrambler channel handle unknown      
 *    MGDDIError:      Interface execution error       
 *           
 *Comments: 
 *    the number of key pairs and the size of the keys must first have been          
 *    defined when initializing the descrambler.           
 *           
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDISrcLoadCWToDscrChnl
      (TMGDDISrcDscrChnlHandle  hChannel,TMGDDICWPeers*  pCWPeerTable)
 {
       TMGDDIStatus    StatusMgDdi = MGDDIOK; 
       U32                    DscrChId,i;
 
       semaphore_wait(psemMgDescAccess);
      
      DscrChId = (U32)hChannel-1;

       if((hChannel==0) ||(DscrChId>=CHCA_MAX_DSCR_CHANNEL))
       {
	        StatusMgDdi =MGDDIBadParam;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcLoadCWToDscrChnl==>] Descrambler channel handle unknown \n",
                                __FILE__,
                                __LINE__);
#endif  	
               semaphore_signal(psemMgDescAccess);

               return (StatusMgDdi);
	}
	  
       if((pCWPeerTable==NULL)||(pCWPeerTable->nKeys != 1)||(pCWPeerTable->KeySize != 8))		  
       {
             StatusMgDdi = MGDDIBadParam;
#ifdef   MGSSOURCE_DRIVER_DEBUG			  
             do_report(severity_info,"\n %s %d >[MGDDISrcLoadCWToDscrChnl==>] error parameter dataadd[%4x],nKey[%d],KeySize[%d]\n",
                               __FILE__,
                               __LINE__,
                               pCWPeerTable,
                               pCWPeerTable->nKeys,
                               pCWPeerTable->KeySize);
#endif  	
            semaphore_signal(psemMgDescAccess);

            return(StatusMgDdi);
	}


#ifdef    MGSSOURCE_DRIVER_DEBUG			  
       do_report(severity_info,"\nEven Key:");
       for(i=0;i<8;i++)
       {
             do_report(severity_info,"%4x",*(pCWPeerTable->CWPeer->Even+i));
	}
       do_report(severity_info,"\n");
#endif	

	if(CHCA_Set_EvenKey2Descrambler(DscrChId,pCWPeerTable->CWPeer->Even)==true)
	{
              StatusMgDdi = MGDDIError;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcLoadCWToDscrChnl==>] Interface execution error \n",
                               __FILE__,
                               __LINE__);
#endif 

              /*zxg 20060315 add for Reset Descrambler*/
               CH_ResetAVDescrambler(DscrChId);
              /********************/
              semaphore_signal(psemMgDescAccess);

              return(StatusMgDdi);
	}


	
	if(CHCA_Set_OddKey2Descrambler(DscrChId,pCWPeerTable->CWPeer->Odd)==true)
	{
              StatusMgDdi = MGDDIError;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcLoadCWToDscrChnl==>] Interface execution error \n",
                               __FILE__,
                               __LINE__);
#endif 

         /*zxg 20060315 add for Reset Descrambler*/
        CH_ResetAVDescrambler(DscrChId);
       /********************/
              semaphore_signal(psemMgDescAccess);

              return(StatusMgDdi);
	}


#ifdef    MGSSOURCE_DRIVER_DEBUG			  
       do_report(severity_info,"\nOdd Key:");
       for(i=0;i<8;i++)
       {
             do_report(severity_info,"%4x",*(pCWPeerTable->CWPeer->Odd+i));
	}
	do_report(severity_info,"\n");   
#endif	
     
       semaphore_signal(psemMgDescAccess);

       return(StatusMgDdi);
 } 

/*******************************************************************************
 *Function name: MGDDISrcAddCmptToDscrChnl
 *           
 *
 *Description: This function associates a component PID to the descrambler channel 
 *             related to the hChannel handle.Several elements can be associated to
 *             the same channel.Component PID can be associated at any time whether   
 *             the descrambler channel has been started or not.      
 *     
 *Prototype:
 *     TMGDDIStatus  MGDDISrcAddCmptToDscrChnl      
 *           ( TMGDDISrcDscrChnlHandle  hChannel,uword  PID)
 *
 *
 *input:
 *     hChannel:   Descrambler channel handle      
 *     PID:        Component PID to be added.      
 *
 *output:
 *           
 *Returned code:
 *     MGDDIOk:         The association has been successfully defined.      
 *     MGDDIBadParam:   Descrambler channel handle unknown.      
 *     MGDDINoResource: Too many elements on the same channel       
 *     MGDDIError:      Interface execution error.      
 *           
 *           
 *           
 *Comments: 
 *           
 *******************************************************************************/
 TMGDDIStatus  MGDDISrcAddCmptToDscrChnl
      ( TMGDDISrcDscrChnlHandle  hChannel,uword  PID)
 {
       TMGDDIStatus                        StatusMgDdi = MGDDIOK; 	 
       U32                                       DscrChId;

       semaphore_wait(psemMgDescAccess);
       DscrChId = (U32)hChannel-1;

       if((hChannel==0) ||(DscrChId>=CHCA_MAX_DSCR_CHANNEL) || (PID==CHCA_DEMUX_INVALID_PID))
       {
	        StatusMgDdi =MGDDIBadParam;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcAddCmptToDscrChnl==>] Descrambler channel handle unknown \n",
                                __FILE__,
                                __LINE__);
#endif  	
               semaphore_signal(psemMgDescAccess);
               return (StatusMgDdi);
	}

       if(CHCA_Associate_Descrambler(DscrChId,PID)==true)
       {
	        StatusMgDdi =MGDDIError;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcAddCmptToDscrChnl==>] Interface execution error \n",
                                __FILE__,
                                __LINE__);
#endif  
              semaphore_signal(psemMgDescAccess);
              return (StatusMgDdi);
	}
	else   
	{
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcAddCmptToDscrChnl==>] Interface OK,channel[%d] PID[%d] \n",
                                __FILE__,
                                __LINE__,
                                hChannel,
                                PID);
#endif  
	}

	semaphore_signal(psemMgDescAccess);
       return (StatusMgDdi);

 }


/*******************************************************************************
 *Function name: MGDDISrcDelCmptFromDscrChnl
 *           
 *
 *Description: This function deletes the association between the component PID 
 *             and the descrambler channel related to the hChannel handle.Component
 *             PID can be deleted at any time whether the descrambler channel has 
 *             been started or not.
 *
 *
 *Prototype:
 *     TMGDDIStatus MGDDISrcDelCmptFromDscrChnl      
 *           ( TMGDDISrcDscrChnlHandle  hChannel,uword PID)  
 *
 *
 *input:
 *     hChannel:     Descrambler channel handle      
 *     PID:          Component PID to be removed      
 *
 *output:
 *           
 *Returned code:
 *     MGDDIOk:        The association has been deleted      
 *     MGDDIBadParam:  Descrambler channel handle or PID unknown      
 *     MGDDIError:     Interface execution error.      
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
  TMGDDIStatus MGDDISrcDelCmptFromDscrChnl
      ( TMGDDISrcDscrChnlHandle  hChannel,uword PID)
 {
       TMGDDIStatus                        StatusMgDdi = MGDDIOK; 	  
       unsigned int/*key_t xjp 051114 */                                     DscrChId;

#if 0
       CHCA_PTI_SlotOrPid_t            slotorpid;    
	CHCA_UCHAR                         i;   
#endif

       semaphore_wait(psemMgDescAccess);
 
       DscrChId = (U32)hChannel-1;

       if((hChannel==0) ||(DscrChId>=CHCA_MAX_DSCR_CHANNEL) || (PID==CHCA_DEMUX_INVALID_PID))
       {
	        StatusMgDdi =MGDDIBadParam;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcDelCmptFromDscrChnl==>] Descrambler channel handle unknown \n",
                                __FILE__,
                                __LINE__);
#endif  	
              semaphore_signal(psemMgDescAccess);

               return (StatusMgDdi);
	}

  	 if(CHCA_Disassociate_Descrambler(DscrChId,PID)==true)
        {
	        StatusMgDdi =MGDDIError;
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcDelCmptFromDscrChnl==>] Interface execution error,pid[%d]\n",
                                 __FILE__,
                                 __LINE__,
                                 PID);
#endif  	
              semaphore_signal(psemMgDescAccess);
              return (StatusMgDdi);

	 }
	 else
	 {
#ifdef     MGSSOURCE_DRIVER_DEBUG			  
               do_report(severity_info,"\n %s %d >[MGDDISrcDelCmptFromDscrChnl==>] deassociate the pid[%d] from the descrambler\n",
                                 __FILE__,
                                 __LINE__,
                                 PID);
#endif  

	 }

        semaphore_signal(psemMgDescAccess);

        return (StatusMgDdi);
 }

/*******************************************************************************
 *Function name: MGDDISrcGetFreeDmxFltr
 *           

 *Description: This function returns the number of free demultiplexer filters of 
 *             the source related to the hSource handle.This number is returned 
 *             at the address given in the nFilter parameter. 
 *
 *Prototype:
 *     TMGDDIStatus  MGDDISrcGetFreeDmxFltr      
 *           (TMGSysSrcHandle  hSource,uword*  nFilter)
 *
 *
 *input:
 *    hSource:       Source handle related to demultiplexer       
 *    nFilter:       Address of number of free filters to be filled       
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk:        Execution has processed correctly       
 *    MGDDIBadParam:  Source handle unknown or parameter null       
 *    MGDDIError:     Interface execution error.       
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
 TMGDDIStatus  MGDDISrcGetFreeDmxFltr
	   (TMGSysSrcHandle  hSource,uword*  nFilter)
 {
       TMGDDIStatus     StatusMgDdi = MGDDIOK;
       CHCA_INT           filtercount;
	   
  
	if((hSource==NULL)||(hSource!=(TMGSysSrcHandle)MGTUNERDeviceName)||(nFilter==NULL))
	{
              StatusMgDdi =MGDDIBadParam;
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcGetFreeDmxFltr==>] Source handle unknown or parameter null \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}


	filtercount = CHCA_GetFreeFilter();

	if(filtercount<=0)/*20061128 add =0*/
	{
             StatusMgDdi = MGDDIError; 

#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcGetFreeDmxFltr==>]  Interface execution error \n",
                               __FILE__,
                               __LINE__);
#endif
		return (StatusMgDdi);	  
	}

	*nFilter = (uword)filtercount;  

#ifdef    MGSSOURCE_DRIVER_DEBUG			  
        do_report(severity_info,"\n %s %d >[MGDDISrcGetFreeDmxFltr==>] the num[%d] of free filter \n",
                          __FILE__,
                          __LINE__,
                          *nFilter);
#endif

       return (StatusMgDdi);	  


}


/*******************************************************************************
 *Function name: MGDDISrcAllocDmxFltr
 *           

 *Description: This function reserves a demultiplexer filter. The filter is allocated
 *             on the demultiplexer of the source related to the hSource handle.The    
 *             demultiplexer filter handle returned is an absolution descriptor, 
 *             in other words independent from the source
 *
 *
 *Prototype:
 *     TMGDDISrcDmxFltrHandle  MGDDISrcAllocDmxFltr(TMGSysSrcHandle hSource)       
 *           
 *
 *
 *input:
 *     hSource:          System source handle related to demultiplexer      
 *           
 *
 *output:
 *           
 *Returned code:
 *     Demultiplexer filter handle,provided it is not null      
 *     NULL,if a problem has occurred in executing the function:      
 *           - system source handle unknown or parameter null, 
 *           - there is no free demultiplexer filter, 
 *           - a problem has occurred in reservation of the filter.
 *           
 *           
 *Comments: 
        
 *           
 *******************************************************************************/
 TMGDDISrcDmxFltrHandle  MGDDISrcAllocDmxFltr(TMGSysSrcHandle hSource)
 {
         U16                              FltrHandle;   
         CHCA_SHORT                           TempFltrHandle;
 
         /*sf_filter_mode_t           FilterMode;
	  app_t                          FilterApp;
	  BOOLEAN                    MatchMode;*/

         CHCA_SectionDataAccess_Lock();
	  if((hSource!=(TMGSysSrcHandle)MGTUNERDeviceName)||(hSource==NULL))
	  {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcAllocDmxFltr==>] system source handle unknown or parameter null \n",
                               __FILE__,
                               __LINE__);
#endif
              CHCA_SectionDataAccess_UnLock();
		return NULL;	  

	  } 

	  TempFltrHandle = CHCA_Allocate_Section_Filter(); 

	  if(TempFltrHandle == CHCA_ILLEGAL_FILTER)	 
	  {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcAllocDmxFltr==>] there is no free demultiplexer filter or a problem has occurred in reservation of the filter \n",
                               __FILE__,
                               __LINE__);
#endif
              CHCA_SectionDataAccess_UnLock();
		return NULL;	
	  }

         FltrHandle = TempFltrHandle;

#ifdef    MGSSOURCE_DRIVER_DEBUG			  
         do_report(severity_info,"\n %s %d >[MGDDISrcAllocDmxFltr==>] Demultiplexer filter handle[%d] is allocated!\n",
                           __FILE__,
                            __LINE__,
                            FltrHandle);
#endif

         /*SectionFilter[FltrHandle].bBufferLock = false; */
         FDmxFlterInfo [ FltrHandle ].Lock = false;
         FDmxFlterInfo [ FltrHandle ].SlotId = CHCA_ILLEGAL_CHANNEL; 
	  FDmxFlterInfo [ FltrHandle ].TTriggerMode = false;
         FDmxFlterInfo [ FltrHandle ].bdmxactived = false;	 
	  FDmxFlterInfo [ FltrHandle ].uTmDelay = 0;
         FDmxFlterInfo [ FltrHandle ].InUsed = false; 
         FDmxFlterInfo [ FltrHandle ].cMgDataNotify = NULL;
	  FDmxFlterInfo[FltrHandle].MulSection = MGDDISection;  /*add this on 041104*/
	  FDmxFlterInfo [ FltrHandle ].FilterPID = CHCA_DEMUX_INVALID_PID; /*add this on 050326*/
	  
	  CHCA_SectionDataAccess_UnLock();

#if 0
	  hFilterTemp = (TMGDDISrcDmxFltrHandle )(FltrHandle+1);
#endif
	  
	  return (TMGDDISrcDmxFltrHandle )(FltrHandle+1);
 }


/*******************************************************************************
 *Function name : MGDDISrcFreeDmxFltr
 *           

 *Description: This function releases the filter related to the hFilter handle.The
 *             related hardware resource is freed and available to the system 
 *
 *Prototype:
 *     TMGDDIStatus MGDDISrcFreeDmxFltr(TMGDDISrcDmxFltrHandle hFilter)      
 *           
 *
 *
 *input:
 *     hFilter :  Demultiplexer filter handle.       
 *           
 *
 *output:
 *           
 *Returned code:
 *     MGDDIOk:          The filter has been freed.           
 *     MGDDIBadParam:    The handle of the filter is unknown.       
 *     MGDDIError:       Interface execution error      
 *           
 *Comments: 
 *           
 *******************************************************************************/
TMGDDIStatus MGDDISrcFreeDmxFltr(TMGDDISrcDmxFltrHandle hFilter)
 {
         CHCA_UINT                                         FltrHandleTemp;
	  CHCA_USHORT                                    FltrHandle; 
	  TMGDDIStatus                                     StatusMgDdi = MGDDIOK; 	 
	  CHCA_USHORT                                    FltrCount=0; 

         CHCA_SectionDataAccess_Lock();

         FltrHandleTemp = (CHCA_UINT)hFilter;
		 
         if((FltrHandleTemp==0)||(FltrHandleTemp>CHCA_MAX_NUM_FILTER))
         {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] The filter handle[%4x] or pfilter unknown \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif
                CHCA_SectionDataAccess_UnLock();

                 return (StatusMgDdi);
	  }
 
   
         FltrHandle = (CHCA_USHORT)FltrHandleTemp -1;

         if(CHCA_CheckMultiFilter(FDmxFlterInfo [ FltrHandle ].SlotId,&FltrCount))
        {
                StatusMgDdi = MGDDIError;

#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] Get filter count  error\n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	 }
		 
	  if(CHCA_Free_Section_Filter(FltrHandle)==false)	 
	  {
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] The filter has been freed \n",
                                  __FILE__,
                                  __LINE__);
#endif
                /*SectionFilter[FltrHandle].bBufferLock = false; */
                FDmxFlterInfo [ FltrHandle ].Lock= false; /*add this on 050324*/
		  FDmxFlterInfo [ FltrHandle ].TTriggerMode = false;
                FDmxFlterInfo [ FltrHandle ].bdmxactived = false;	 
                FDmxFlterInfo [ FltrHandle ].InUsed = false; /*add this on 040719*/
		  FDmxFlterInfo[FltrHandle].MulSection = MGDDISection;	 /*add this on 041104*/	
		  if(FDmxFlterInfo[FltrHandle].FilterType==ECM_FILTER)
		  {
		         EcmFilterNum = EcmFilterNum -1;
		  }		 
		  FDmxFlterInfo [ FltrHandle ].FilterType = OTHER_FILTER; 	  /*delete this on 050313*/	
		  CHCA_MgDataNotifyUnRegister(FltrHandle);	 
	  }
	  else
	  {
	         StatusMgDdi = MGDDIError;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] fail to free Interface \n",
                                  __FILE__,
                                  __LINE__);
#endif	

                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);

         }	

	 if(FltrCount>1)
	 {
                StatusMgDdi = MGDDIOK; 
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] Multi Filter,do not need free the channel[%d]\n",
                                   __FILE__,
                                   __LINE__,
                                   FDmxFlterInfo [ FltrHandle ].SlotId);
#endif	

		  CHCA_SectionDataAccess_UnLock();		
                return (StatusMgDdi);
	 }

        if(CHCA_Free_Section_Slot(FDmxFlterInfo [ FltrHandle ].SlotId))
        {
                StatusMgDdi = MGDDIError; 
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] Free channel[%d] failed\n",
                                   __FILE__,
                                   __LINE__,
                                   FDmxFlterInfo [ FltrHandle ].SlotId);
#endif	
 	 }
	 else
	 {
                FDmxFlterInfo [ FltrHandle ].FilterPID = CHCA_DEMUX_INVALID_PID; /*add this on 050326*/
                FDmxFlterInfo [ FltrHandle ].SlotId = CHCA_ILLEGAL_CHANNEL; 

#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcFreeDmxFltr==>] succeed in Freeing channel\n",
                                   __FILE__,
                                   __LINE__,
                                   FDmxFlterInfo [ FltrHandle ].SlotId);
#endif	
	 }

	  CHCA_SectionDataAccess_UnLock();	 

	  return (StatusMgDdi);

 } 


/*******************************************************************************
 *Function name: MGDDISrcSetDmxFltr
 *           

 *Description: This interface defines filter parameters for the filter identified 
 *             by the hFilter handle.Those filter parameters are defined by the               
 *             structure provided at the address given in the pFilter parameter.
 *             Filter parameter can be (re)defined only if the demultiplexer filter
 *             is stopped.Otherwise MGDDIBusy code is returned.
 *           
 *
 *Prototype:
 *     TMGDDIStatus  MGDDISrcSetDmxFltr      
 *          (TMGDDISrcDmxFltrHandle  hFilter,TMGDDIFilter* pFilter) 
 *
 *
 *input:
 *     hFilter:    Demultiplexer filter handle.      
 *     pFilter:    Address of a filter parameter structure.      
 *
 *output:
 *           
 *Returned code:
 *     MGDDIOk:        The filter has been defined           
 *     MGDDIBadParam:  The filter handle is unknown.       
 *     MGDDIBusy:      The filtering process has been started      
 *           
 *Comments: 
 *           
 *           
 *           
*******************************************************************************/
TMGDDIStatus  MGDDISrcSetDmxFltr
       (TMGDDISrcDmxFltrHandle  hFilter,TMGDDIFilter* pFilter)
{ 
         TMGDDIStatus           StatusMgDdi = MGDDIOK;   
         CHCA_UINT               FltrHandleTemp;
	  CHCA_USHORT          FltrHandle; 
         /*BYTE                         bTemp;*/
	  CHCA_SHORT            SlotIndex;
	  CHCA_INT                 iIndex; 
	  CHCA_BOOL              bError = true;
	  CHCA_BOOL              bEmmSection=0; /*modify this on 050327 from false to 0*/

	  CHCA_UCHAR                         filterData[CHCA_MGDDI_FILTERSIZE];
	  CHCA_UCHAR                         filterMask[CHCA_MGDDI_FILTERSIZE];
	  /*U8                            *filterDataTemp,*filterMaskTemp;*/

	  CHCA_UCHAR            ModePatern[8]; /*mode type select*/
	  CHCA_UINT               FilterDataLen=CHCA_MGDDI_FILTERSIZE;
	  
#if 0
	  message_queue_t     *ReturnQueueId;
#endif
	  CHCA_BOOL             bSlotAllocated;

         CHCA_SectionDataAccess_Lock();

	  FltrHandleTemp = (CHCA_UINT)hFilter;	 
         if((FltrHandleTemp==0)||(FltrHandleTemp>CHCA_MAX_NUM_FILTER)||(pFilter==NULL)||(pFilter->PID==CHCA_DEMUX_INVALID_PID))
         {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] The filter handle[%4x] or pfilter unknown \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	  }

	  FltrHandle = (CHCA_USHORT)FltrHandleTemp -1;	 

         if(FDmxFlterInfo [ FltrHandle ].bdmxactived || FDmxFlterInfo [ FltrHandle ].InUsed)
         {
                 StatusMgDdi = MGDDIBusy;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] The filter handle[%4x],The filtering process has been started \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	   }

          if((pFilter->MaxLen <= 0) ||(pFilter->MaxLen > CHCA_MAX_DBUFFER_LEN))
          {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] the Max len[%d] err  \n",
                                   __FILE__,
                                  __LINE__,
                                  pFilter->MaxLen);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	   }

#ifdef       MGSSOURCE_DRIVER_DEBUG			  
         do_report(severity_info,"\n[MGDDISrcSetDmxFltr==>] Request PID[%d],Filter[%d]\n",pFilter->PID,FltrHandle);
#endif

         /*init the value of the filter data and the mask*/ 
	  for ( iIndex = 0; iIndex < CHCA_MGDDI_FILTERSIZE ;iIndex ++ )
	  {
		 filterData[iIndex] =0 ;
		 filterMask[iIndex] =0 ;
		 ModePatern[iIndex] = 0xff;/*0xff valuse is for positive match mode,0x00 is for negative match mode*/
		 
		 filterData[iIndex]  =( (U8) pFilter->Value[iIndex] ) & 0xFF;
		 filterMask[iIndex] = ((U8)pFilter->Mask[iIndex]) & 0xFF;
	  }

#if   0/*add this on 050224*/
	  for ( iIndex = 0; iIndex < CHCA_MGDDI_FILTERSIZE ;iIndex ++ )
	  {
              do_report(severity_info,"\n filterData::");
              do_report(severity_info,"%4x",filterData[iIndex] );
	  }

	  for ( iIndex = 0; iIndex < CHCA_MGDDI_FILTERSIZE ;iIndex ++ )
	  {
              do_report(severity_info,"\n filterMask::");
              do_report(severity_info,"%4x",filterMask[iIndex] );
	  }
	  do_report(severity_info,"\n");
#endif

         /*add this on 050313*/ 
         /*FDmxFlterInfo [ FltrHandle ].DataCrc = -1;  init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[0]=0;/*init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[1]=0;/*init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[2]=0;/*init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[3]=0;/*init the ca data add this on 050319*/

         switch(filterData[0])
         {
                  case 0x1:
                           /*FDmxFlterInfo [ FltrHandle ].DataCrc = -1;  init the cat data add this on 050319*/
			      FDmxFlterInfo [ FltrHandle ].FilterType = CAT_FILTER; 
			      bEmmSection = 2;	 /*modify this on 050327*/		   
			      break;

                  case 0x80:
                  case 0x81:
#if  0/*add this on 050310*/
                          /*do_report(severity_info,"\n[ChPmtDataProcess==>]New PMT,Reset the EcmFilterInfo\n");*/
	                    for(i=0;i<2;i++)
                           {  
		                 EcmFilterInfo[i].EcmFilterId=-1;
		                 memset ( EcmFilterInfo[i].ECM_Buffer , 0 , MGCA_PRIVATE_TABLE_MAXSIZE );
	                    }
#endif	 		
                           EcmFilterNum = EcmFilterNum + 1;
                           /*FDmxFlterInfo [ FltrHandle ].DataCrc = -1;  init the ecm data add this on 050317*/
			      FDmxFlterInfo [ FltrHandle ].FilterType = ECM_FILTER; 	  	
                           /*do_report(severity_info,"\n[MGDDISrcSetDmxFltr==>]The ECM FilterId[%d] FilterNum[%d]\n",FltrHandle,EcmFilterNum);*/
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
                          /* FDmxFlterInfo [ FltrHandle ].DataCrc = -1;  init the emm data add this on 050319*/
			      FDmxFlterInfo [ FltrHandle ].FilterType = EMM_FILTER; 	  	
                           bEmmSection = 1;
			      break;

                  default:
			      FDmxFlterInfo [ FltrHandle ].FilterType = OTHER_FILTER; 	  	
			      break;
         }
	  /*add this on 050313*/ 	 

	  /*do_report(severity_info,"\n[MGDDISrcSetDmxFltr==>]FilterData[%d]\n",filterData[0]);	 */
	  
	  /*do_report(severity_info,"\n[MGDDISrcSetDmxFltr==>]Pid[%d]\n",pFilter->PID);*/
          if(CHCA_PtiQueryPid(&SlotIndex,(CHCA_PTI_Pid_t)pFilter->PID,&bSlotAllocated)==false)
          {
                FDmxFlterInfo [ FltrHandle ].FilterPID = (CHCA_PTI_Pid_t)pFilter->PID; /*add this on 050326*/
                if(bSlotAllocated==false)
                {
                       /*allocate the channel index*/
#if 1/*add this on 050115*/
                       SlotIndex = CHCA_Allocate_Section_Slot(bEmmSection);
#else
                       SlotIndex = CHCA_Allocate_Section_Slot();
#endif
					   
#ifdef             MGSSOURCE_DRIVER_DEBUG						   
                  	  do_report(severity_info,"\n[MGDDISrcSetDmxFltr==>] The New Allocated channel[%d]\n",SlotIndex);	  
#endif		   

	                if(SlotIndex != CHCA_ILLEGAL_CHANNEL)
	                {
                              FDmxFlterInfo [ FltrHandle ].SlotId = SlotIndex;  /*reserve the slot index in the demux data structure*/
                              /*do_report(severity_info,"\n[MGDDISrcSetDmxFltr] TableId[%d]Pid[%d] \n",pFilter->Value[0],pFilter->PID);   */
#if 1/*modify this on 050115*/
                             if(CHCA_Set_Slot_Pid(SlotIndex, (CHCA_PTI_Pid_t)pFilter->PID,bEmmSection) == false)
#else
                             if(CHCA_Set_Slot_Pid(SlotIndex, (CHCA_PTI_Pid_t)pFilter->PID) == false)
#endif							 	
                             {
                                    bError = false;  
		               }
		               else
		               {
                                    CHCA_Free_Section_Slot(SlotIndex);
		               }
	                 }
                }
		  else
		  {
#ifdef              MGSSOURCE_DRIVER_DEBUG						   
                    	   do_report(severity_info,"\n[MGDDISrcSetDmxFltr==>] The channel[%d] has been already allocated!Multifilter SignalSlot\n",SlotIndex);	  
#endif
                        FDmxFlterInfo [ FltrHandle ].SlotId = SlotIndex;  /*reserve the slot index in the demux data structure*/
                        bError = false; 
		  }
          }
	   else
	   {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] The filter handle[%4x] or pfilter unknown \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	   }

	   /*allocate the channel index*/
	   if ( bError )
          { 
                StatusMgDdi = MGDDIBadParam;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] the channel can not be allocated \n",
                                  __FILE__,
                                  __LINE__);
#endif	
                CHCA_SectionDataAccess_UnLock();
                return (StatusMgDdi);
				
          }

	   bError = true;

#if  0 
         /*associate the filter to the channel*/
         SectionFilter[FltrHandle].iChannelID = SlotIndex;
	  SectionSlot[SlotIndex].ucNoofFiltersEngaged ++;
#endif	  


#if 0 /*delete this on 050115*/
        /*init the value of the filter data and the mask*/ 
	  for ( iIndex = 0; iIndex < CHCA_MGDDI_FILTERSIZE ;iIndex ++ )
	  {
		 filterData[iIndex] =0 ;
		 filterMask[iIndex] =0 ;
		 ModePatern[iIndex] = 0xff;/*0xff valuse is for positive match mode,0x00 is for negative match mode*/
		 
		 filterData[iIndex]  =( (U8) pFilter->Value[iIndex] ) & 0xFF;
		 filterMask[iIndex] = ((U8)pFilter->Mask[iIndex]) & 0xFF;
	  }
#endif


#if 0		
         if(filterData[0]==0x80||filterData[0]==0x81)
         {
                 ReturnQueueId =/*pstCHCAECMMsgQueue*/pstCHCAPMTMsgQueue;
	  }
	  else /*cat and emm table*/
	  {
                ReturnQueueId = pstCHCACATMsgQueue;
	  }
#endif	

         if(CHCA_Set_Filter(FltrHandle,filterData,filterMask,ModePatern,FilterDataLen,false) == false)
         {
               if(CHCA_Associate_Slot_Filter(SlotIndex,FltrHandle) == false) 
               {
                        /*SectionFilter[FltrHandle].pMsgQueue =ReturnQueueId;*/
                        FDmxFlterInfo[ FltrHandle ].InUsed = true; /*add this on 040719*/
		          FDmxFlterInfo[FltrHandle].MulSection = pFilter->bTable;/* true : MGDDITable,false: MGDDISection*/
                        bError = false; 
               }		
	  }
	 	 	
         
         if(bError)
         {
                StatusMgDdi = MGDDIBadParam;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] filter Set unSuccessfully! \n",
                                  __FILE__,
                                  __LINE__);
#endif	
	  }
	  else
	  {
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[MGDDISrcSetDmxFltr==>] The filterindex [%4x]  Channel[%4x] filter Set Successfully!\n",
                                  __FILE__,
                                  __LINE__,
                                  FltrHandle,
                                  SlotIndex);
#endif	
	  }
		 
         CHCA_SectionDataAccess_UnLock();

         return (StatusMgDdi);	
 		 
		 
 }

/*******************************************************************************
 *Function name : MGDDISrcStartDmxFltr
 *           
 *Description: This interface activates the filter defined on the demultiplexer
 *             filter related to the hFilter handle.The filter parameters must
 *             first have been set.The pMode structure defines whether the filtering
 *             mode is persistent or not.
 * 
 *             as soon as the demultiplexer finds a section or table corresponding  
 *             to the filter definition,it is notified to the Mediaguard kernel by
 *             means of the hCallback function provided, via the MGDDIEvSrcDmxFltrReport  
 *             event.Likewise,for a non-persistent filter, if the timeout set in this
 *             case elapses,this status is notified by the same function,though via
 *             the MGDDISrcDmxTimeout event.
 *
 *             If a loaded section is scrambled and pDscr parameter is not null,
 *             hardware filter must be applied to the output of the descrambler
 *             channel related to the pDscr handle.Otherwise,if a loaded secton is
 *             scrambled and no descrambler channel is provided,hardware must not be
 *             applied.
 *
 *Prototype:
 *      TMGDDIStatus  MGDDISrcStartDmxFltr     
 *           (TMGDDISrcDmxFltrHandle  hFilter,CALLBACK  hCallback,
 *            TMGDDIFilterMode* pMode,TMGDDISrcDscrChnlHandle  pDscr)
 *
 *input:
 *      hFilter :     Demultiplexer filter handle       
 *      hCallback:    Address of a callback function.        
 *      pMode:        Address of a structure describing the filtering mode.
 *      pDescr:       Descrambler channel handle     
 *
 *           
 *
 *Output:
 *           
 *Returned code:
 *     MGDDIOk:         The filtering process has been activated           
 *     MGDDIBadParam:   The filter handle is unknown      
 *     MGDDIError:      Interface execution error      
 *           
 *Comments: 
 *           
 *           
 *******************************************************************************/
 TMGDDIStatus  MGDDISrcStartDmxFltr
      (TMGDDISrcDmxFltrHandle  hFilter,CALLBACK  hCallback,
	   TMGDDIFilterMode* pMode,TMGDDISrcDscrChnlHandle  pDscr)
 {
          TMGDDIStatus                              StatusMgDdi = MGDDIOK;   
          CHCA_UINT                                  FltrHandleTemp;
	   CHCA_USHORT                             FltrHandle; 

#if 0
   	   CHCA_USHORT                             FltrCount; 
#endif

          CHCA_SectionDataAccess_Lock();		
          FltrHandleTemp = (CHCA_UINT)hFilter;	

          
          if((FltrHandleTemp == 0)||(FltrHandleTemp>CHCA_MAX_NUM_FILTER)||(hCallback==NULL)||(pMode==NULL))
          {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcStartDmxFltr==>] The filter handle[%4x] or pfilter unknown \n",
                 __FILE__,
                 __LINE__,
                 FltrHandle);
#endif	
                CHCA_SectionDataAccess_UnLock();
                return (StatusMgDdi);
          }

	  FltrHandle = (CHCA_USHORT)FltrHandleTemp -1;

         if(FDmxFlterInfo [ FltrHandle ].bdmxactived)
         {

	#ifdef       MGSSOURCE_DRIVER_DEBUG
          	do_report(severity_info,"\n[MGDDISrcStartDmxFltr==>] filter [%d] has already been bdmxactived\n",hFilter);	  
	#endif
                CHCA_SectionDataAccess_UnLock();
                return (StatusMgDdi);
         }

         if(CHCA_MgDataNotifyRegister(FltrHandle,hCallback))
         {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcStartDmxFltr==>] The filter handle[%4x] or pfilter unknown \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	  }
	  else
	  {
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcStartDmxFltr==>] success to register the filter[%d] \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
	  }
        
         if(pMode->TmDelay)
         {
		  FDmxFlterInfo [ FltrHandle ].CStartTime = time_now();		
	  }
		 
         /*FDmxFlterInfo [ FltrHandle ].DataCrc = -1;  init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[0]=0;/*init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[1]=0;/*init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[2]=0;/*init the ca data add this on 050319*/
         FDmxFlterInfo [ FltrHandle ].DataCrc[3]=0;/*init the ca data add this on 050319*/
		 
         FDmxFlterInfo[FltrHandle].uTmDelay = pMode->TmDelay;
         FDmxFlterInfo[FltrHandle].TTriggerMode = pMode->bTrigger;
         FDmxFlterInfo [ FltrHandle ].ValidSection = false;
         FDmxFlterInfo [ FltrHandle ].Lock= false;  /*add this on 050424*/

         if(CHCA_StartDemuxFilter(FltrHandle))
         {
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n[MGDDISrcStartDmxFltr==>]start demux filter error\n");
#endif
         
               CHCA_SectionDataAccess_UnLock();
               return (StatusMgDdi);
	  }

         CHCA_SectionDataAccess_UnLock();
		 
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
         do_report(severity_info,"\n[MGDDISrcStartDmxFltr]  Succeed in Starting filer [%d]\n",FltrHandle);
#endif
		 
         return (StatusMgDdi);

 }


/*******************************************************************************
 *Function name: MGDDISrcStopDmxFltr
 *           

 *Description:This function stops the filter defined on the demultiplexer filter
 *            related to the hFilter handle.The related hardware resource is not
 *            freed but deactivated.The filter definition remains stored.It can 
 *            be modified later through the MGDDISrcSetDmxFlter interface
 * 
 *Prototype:
 *      TMGDDIStatus   MGDDISrcStopDmxFltr(TMGDDISrcDmxFltrHandle  hFilter)        
 *           
 *
 *
 *input:
 *    hFilter:    Demultiplexer filter handle.       
 *           
 *
 *output:
 *           
 *Returned code:
 *    MGDDIOk :       The filter has been stopped.
 *    MGDDIBadParam:  The filter handle is unknown.  
 *    MGDDIError:     Interface execution error.       
 *           
 *           
 *           
 *           
 *           
 *Comments: 
 *    all pending events related to the stopped demultiplexer filter must            
 *    be cancelled.       
 *           
 *******************************************************************************/
 TMGDDIStatus   MGDDISrcStopDmxFltr(TMGDDISrcDmxFltrHandle  hFilter) 
 {
         TMGDDIStatus                               StatusMgDdi = MGDDIOK;   
          CHCA_UINT                                  FltrHandleTemp;
	   CHCA_USHORT                             FltrHandle; 
#if 0
	   CHCA_USHORT                               FltrCount; 
#endif

         CHCA_SectionDataAccess_Lock();
 	  FltrHandleTemp = (CHCA_UINT)hFilter;	 

	  if((FltrHandleTemp==0)||(FltrHandleTemp > CHCA_MAX_NUM_FILTER))
         {
                 StatusMgDdi = MGDDIBadParam;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcStopDmxFltr==>] The filter handle[%4x] or pfilter unknown \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	  }

	  FltrHandle = (CHCA_USHORT)FltrHandleTemp - 1;

         if(CHCA_StopDemuxFilter(FltrHandle))
         {
                 StatusMgDdi = MGDDIError;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcStopDmxFltr==>] fail to stop the filter[%d] \n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
                 CHCA_SectionDataAccess_UnLock();
                 return (StatusMgDdi);
	  }
	  else
	  {
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[MGDDISrcStopDmxFltr==>]success to stop the filter[%d]\n",
                                   __FILE__,
                                   __LINE__,
                                   FltrHandle);
#endif	
	  }


#if 0
         CHCA_MgDataNotifyUnRegister(FltrHandle);	 
        
         FDmxFlterInfo [ FltrHandle ].bdmxactived =  false;
         FDmxFlterInfo[FltrHandle].uTmDelay = 0;
#endif

         CHCA_SectionDataAccess_UnLock();

#ifdef       MGSSOURCE_DRIVER_DEBUG			  
         do_report(severity_info,"\n[MGDDISrcStopDmxFltr==>]success to stop the filter[%d]\n",FltrHandleTemp);
#endif

         return (StatusMgDdi);

}


#if 1  /*just for test, not ok*/
#if 0
 void CHMG_TestCallback
   (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2)
{
       int i;

	i = 0;   
}

void    CHMG_PmtChanged(U16  PidTT)
{
       TMGDDIFilter                             pFilter;

       TMGDDIFilterMode                      pMode;
        pMode.TmDelay = 3000;
	 pMode.bTrigger = 0; 

	 pFilter.PID = PidTT;  
        pFilter.MaxLen = 1024;
	 memset(pFilter.Value,0,8);
        memset(pFilter.Mask,0,8);
	 pFilter.Value[0]=0x80;	
        pFilter.Mask[0]=0xfe;
	 pFilter.bTable = 0;	
	   
#if 1
       MGDDISrcStopDmxFltr(hFilterTemp);
	MGDDISrcFreeDmxFltr(hFilterTemp);

	hFilterTemp = MGDDISrcAllocDmxFltr((TMGSysSrcHandle)MGTUNERDeviceName);
	MGDDISrcSetDmxFltr(hFilterTemp,&pFilter);

	/*MGDDISrcStartDmxFltr(hFilterTemp,CHMG_TestCallback,&pMode, (TMGDDISrcDscrChnlHandle)1);*/


#else
	
       MGDDISrcDelCmptFromDscrChnl(hChannelTemp, 200);
       MGDDISrcStopDmxFltr(hFilterTemp);
	MGDDISrcFreeDmxFltr(hFilterTemp);
	MGDDISrcFlushDscrChnl(hChannelTemp);
	MGDDISrcDelCmptFromDscrChnl(hChannelTemp, 201);
	MGDDISrcFreeDscrChnl(hChannelTemp);
	

	hFilterTemp = MGDDISrcAllocDmxFltr((TMGSysSrcHandle)MGTUNERDeviceName);
	MGDDISrcSetDmxFltr(hFilterTemp,&pFilter);
	hChannelTemp = MGDDISrcAllocDscrChnl((TMGSysSrcHandle)MGTUNERDeviceName);
	MGDDISrcAddCmptToDscrChnl(hChannelTemp, 200);
       MGDDISrcAddCmptToDscrChnl(hChannelTemp, 201);
#endif	   

}

void    CHMG_PmtStart(void)

{
       TMGDDIFilter                             pFilter;

	 pFilter.PID = 103;  
        pFilter.MaxLen = 1024;
	 memset(pFilter.Value,0,8);
        memset(pFilter.Mask,0,8);
	 pFilter.Value[0]=0x80;	
        pFilter.Mask[0]=0xfe;
	 pFilter.bTable = 0;	
	   
	hFilterTemp = MGDDISrcAllocDmxFltr((TMGSysSrcHandle)MGTUNERDeviceName);
	MGDDISrcSetDmxFltr(hFilterTemp,&pFilter);
	hChannelTemp = MGDDISrcAllocDscrChnl((TMGSysSrcHandle)MGTUNERDeviceName);
	MGDDISrcAddCmptToDscrChnl(hChannelTemp, 200);
       MGDDISrcAddCmptToDscrChnl(hChannelTemp, 201);

}

#endif
CHCA_BOOL CHMG_FreeSectionReq(U16   iiFilterIndex)
{
         CHCA_BOOL   bErrCode = false;

         CHCA_SectionDataAccess_Lock();
         if(iiFilterIndex == CHCA_ILLEGAL_FILTER)	
         {
                bErrCode = true;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHMG_FreeSectionReq==>]  Filter Value is wrong\n",
                                  __FILE__,
                                  __LINE__,
                                  iiFilterIndex);
#endif	
                CHCA_SectionDataAccess_UnLock();
	  }
		 
         if(CHCA_Stop_Filter(iiFilterIndex)==true)
         {
                bErrCode = true;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHMG_FreeSectionReq==>] Stop Filter Err\n",
                                  __FILE__,
                                  __LINE__,
                                  iiFilterIndex);
#endif	
                CHCA_SectionDataAccess_UnLock();
	  }


	  if(CHCA_Free_Section_Filter(iiFilterIndex)==false)	 
	  {
                /*SectionFilter[FltrHandle].bBufferLock = false; */
		  FDmxFlterInfo [ iiFilterIndex ].TTriggerMode = false;
                FDmxFlterInfo [ iiFilterIndex ].bdmxactived = false;	 
                FDmxFlterInfo [ iiFilterIndex ].InUsed = false; /*add this on 040719*/
		  FDmxFlterInfo[iiFilterIndex].MulSection = MGDDISection;	 /*add this on 041104*/	
	  }
	  else
	  {
	         bErrCode = true;
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHMG_FreeSectionReq==>] free filter error  \n",
                                  __FILE__,
                                  __LINE__);
#endif	

                 CHCA_SectionDataAccess_UnLock();
                 return (bErrCode);

         }	

         if(CHCA_Free_Section_Slot(FDmxFlterInfo [ iiFilterIndex ].SlotId))
         {
	          bErrCode = true;
#ifdef       MGSSOURCE_DRIVER_DEBUG			  
                 do_report(severity_info,"\n %s %d >[CHMG_FreeSectionReq==>] Free channel[%d] interface execution error\n",
                                   __FILE__,
                                   __LINE__,
                                   FDmxFlterInfo [ iiFilterIndex ].SlotId);
#endif	
 	 }

        CHCA_SectionDataAccess_UnLock();
			 
        return (bErrCode);

}



CHCA_SHORT CHMG_SectionReq(message_queue_t  *ReturnQueueId,
	                                                    CHCA_USHORT       iPidValue,
	                                                    CHCA_USHORT       iTableId)
{
          CHCA_SHORT                           FltrHandle;
          CHCA_BOOL                             bError=true;
          CHCA_SHORT                           SlotIndex;
          
          CHCA_INT                                iIndex; 
          
          CHCA_UCHAR                           filterData[CHCA_MGDDI_FILTERSIZE];
          CHCA_UCHAR                          filterMask[CHCA_MGDDI_FILTERSIZE];
          
           CHCA_UCHAR                          ModePatern[CHCA_MGDDI_FILTERSIZE]; /*mode type select*/
           CHCA_UINT                             FilterDataLen=CHCA_MGDDI_FILTERSIZE;
           
           CHCA_SectionDataAccess_Lock();
           
           FltrHandle = CHCA_Allocate_Section_Filter(); 

	  if(FltrHandle == CHCA_ILLEGAL_FILTER)	 
	  {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[CHMG_SectionReq==>] there is no free demultiplexer filter or a problem has occurred in reservation of the filter \n",
                               __FILE__,
                               __LINE__);
#endif
              CHCA_SectionDataAccess_UnLock();
		return -1;	
	  }
	  else
   	  {
#ifdef    MGSSOURCE_DRIVER_DEBUG			  
              do_report(severity_info,"\n %s %d >[MGDDISrcAllocDmxFltr==>] Demultiplexer filter handle[%d] is allocated!\n",
                                __FILE__,
                                __LINE__,
                               FltrHandle);
#endif
   	  }


	  SlotIndex = CHCA_Allocate_Section_Slot(false);

	  FDmxFlterInfo [ FltrHandle ].SlotId = SlotIndex;

	  if(SlotIndex != CHCA_ILLEGAL_CHANNEL)
	  {
#if 1
                if(CHCA_Set_Slot_Pid(SlotIndex, iPidValue,false) == false)

#else
                if(CHCA_Set_Slot_Pid(SlotIndex, iPidValue) == false)
#endif					
                {
                       bError = false;  
		  }
		  else
		  {
                       CHCA_Free_Section_Slot(SlotIndex);
		  }
	  }
	   /*allocate the channel index*/

	  if ( bError )
         { 
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHMG_SectionReq==>] the channel can not be allocated \n",
                                  __FILE__,
                                  __LINE__);
#endif	
                CHCA_SectionDataAccess_UnLock();
                return -1;
				
          }

	   bError = true;

          /*init the value of the filter data and the mask*/ 
	   for ( iIndex = 0; iIndex < CHCA_MGDDI_FILTERSIZE ;iIndex ++ )
	   {
		 filterData[iIndex] =0 ;
		 filterMask[iIndex] =0 ;
		 ModePatern[iIndex] = 0xff;/*0xff valuse is for positive match mode,0x00 is for negative match mode*/
		 
	   }

	   filterData[0] = iTableId;
	   filterMask[0] = 0xff;
        
          if(CHCA_Set_Filter(FltrHandle,filterData,filterMask,ModePatern,FilterDataLen,false) == false)
          {
               if(CHCA_Associate_Slot_Filter(SlotIndex,FltrHandle) == false) 
               {
                        /*SectionFilter[FltrHandle].pMsgQueue =ReturnQueueId;*/
                        FDmxFlterInfo[ FltrHandle ].InUsed = true; /*add this on 040719*/
                        bError = false; 
               }		
	   }
	 	 	
         
         if(bError)
         {
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHMG_SectionReq==>] process error \n",
                                  __FILE__,
                                  __LINE__);
#endif
                CHCA_SectionDataAccess_UnLock();
                return -1;
	  }
	  else
	  {
#ifdef      MGSSOURCE_DRIVER_DEBUG			  
                do_report(severity_info,"\n %s %d >[CHMG_SectionReq==>] The filterindex [%4x]  Channel[%4x]OK \n",
                                  __FILE__,
                                  __LINE__,
                                  FltrHandle,
                                  SlotIndex);
#endif	
	  }

	  CHCA_SectionDataAccess_UnLock();
	  
	  return FltrHandle;

}

#endif


/*******************************************************************************
 *Function name
 *           

 *Description:
 *           
 *
 *Prototype:
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
 *Returned code:
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *Comments: 
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *******************************************************************************/
#ifdef  DEMUX_TEST
void MgFunc_Callback
   (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2)
{
  
   return;
}

SLOT_ID Mg_SectionReqCA_Test ( message_queue_t  *pstReturnQueueId,
					sf_filter_mode_t  sfFilterMode,
					pid_t pid ,
					BYTE ucTableId,
					int iReqProgNo,
					int iReqSectionNo,
                    int iReqVersionNo ,
                    int iNotEqualIndex )
{
        TMGDDISrcDmxFltrHandle       FilterIndex;
	 TMGSysSrcHandle                   hSource;
	 TMGDDIFilter                          pFilter;
	 TMGDDIFilterMode                   pMode;
	 SLOT_ID                                 Filter;

	 hSource = (TMGSysSrcHandle)MGTUNERDeviceName;


	 pFilter.PID = (uword)pid;
	 pFilter.MaxLen = 1024;
	 pFilter.bTable = false;
	 pFilter.Value[0] = ucTableId;

	 MReqProgNo = iReqProgNo;

	 ppstReturnQueueId = pstReturnQueueId;
       
        FilterIndex = MGDDISrcAllocDmxFltr(hSource);

        MGDDISrcSetDmxFltr(FilterIndex,&pFilter);
 
	 pMode.bTrigger = false;
	 pMode.TmDelay = 10;

        MGDDISrcStartDmxFltr(FilterIndex,MgFunc_Callback,&pMode,(TMGDDISrcDscrChnlHandle)1);

	 Filter =  (SLOT_ID )FFilterIndex;

	 return (Filter);

}

#endif



#if 0
/*******************************************************************************
 *Function name: ChDescramblerInit
 *           

 *Description:
 *           
 *
 *Prototype:
 *            void ChDescramblerInit(void)
 *           
 *
 *
 *input:
 *           
 *           
 *
 *output:
 *           
 *Returned code:
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *Comments: 
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *******************************************************************************/
void ChDescramblerInit(void)
{
        int    i;

        semaphore_wait(psemMgDescAccess);
       
        for(i=0;i<CHCA_MAX_DSCR_CHANNEL;i++)		
        {
              if(DscrChannel[i].PID==CHCA_DEMUX_INVALID_PID)
                  DscrChannel[i].DscrChAllocated = 0; 
	 }
	 semaphore_signal(psemMgDescAccess);
       	

}
#endif


/*******************************************************************************
 *Function name
 *           

 *Description:
 *           
 *
 *Prototype:
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
 *Returned code:
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *Comments: 
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *           
 *******************************************************************************/




