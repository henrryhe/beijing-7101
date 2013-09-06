#ifdef    TRACE_CAINFO_ENABLE
#define  MGGENERAL_DRIVER_DEBUG
#endif

/******************************************************************************
*
* File : MgGenDrv.C
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
*           
*
* Copyright: Changhong 2004 (c)
*
*****************************************************************************/

/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/
#include "MgGenDrv.h"


char MgString_p[256] = "";



/*extern     unsigned   char    ChSTBID[STBSerialNumberLength];  */
/*****************************************************************************
 *Interface description
 *****************************************************************************/
/*******************************************************************************
 *Function name: MGDDIGetSTBData
 *           
 *
 *Description: This function returns the terminal data.This data is entered into
 *             the terminal data structure provided at the address given in 
 *             parameter pSTBData     
 *    
 *Prototype:
 *     TMGDDIStatus  MGDDIGetSTBData(TMGDDISTBData  *pSTBData)           
 *           
 *
 *
 *input:
 *     pSTBData:  STB data structure.      
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *     MGDDIOk:        STB data available.
 *     MGDDIBadParam:  pSTBData parameter incorrect. 
 *     MGDDIError:     STB data unavailable
 * 
 *
 *Comments:
 *           
 *******************************************************************************/
TMGDDIStatus  MGDDIGetSTBData(TMGDDISTBData  *pSTBData)
 {
      TMGDDIStatus     StatusMgDdi = MGDDIOK;
   
      /*do_report(severity_info,"\n MGDDIGetSTBData  \n");*/


      if(pSTBData ==NULL)
      {
             StatusMgDdi = MGDDIBadParam;
#ifdef   MGGENERAL_DRIVER_DEBUG
             do_report(severity_info,"\n pSTBData parameter incorrect \n");
#endif
             return (StatusMgDdi);
      }

#if 0
#if 1/*just for test*/
      memcpy(pSTBData->ID.SerialNumber,ChSTBID_Test,STBSerialNumberLength);
#else
      memcpy(pSTBData->ID.SerialNumber,ChSTBID,STBSerialNumberLength);
#endif
#endif

     if(CHCA_MgGetStbID(pSTBData->ID.SerialNumber)==true)
     {
           StatusMgDdi =MGDDIError; 
#ifdef   MGGENERAL_DRIVER_DEBUG
             do_report(severity_info,"\n STB id data unavailable\n");
#endif
             return (StatusMgDdi);
     }


     if(CHCA_GetSTBVersion(pSTBData->Ver.HVN,pSTBData->Ver.SVN)==true)
     {
             StatusMgDdi =MGDDIError; 
#ifdef   MGGENERAL_DRIVER_DEBUG
             do_report(severity_info,"\n STB version data unavailable\n");
#endif
             return (StatusMgDdi);
             
     }

#ifdef   MGGENERAL_DRIVER_DEBUG
      do_report(severity_info,"\n The Hardware versrion[%d][%d][%d][%d] \n",
                        pSTBData->Ver.HVN[0],
                        pSTBData->Ver.HVN[1],
                        pSTBData->Ver.HVN[2],
                        pSTBData->Ver.HVN[3]);

      do_report(severity_info,"\n The Software versrion[%d][%d][%d][%d] \n",
                        pSTBData->Ver.SVN[0],
                        pSTBData->Ver.SVN[1],
                        pSTBData->Ver.SVN[2],
                        pSTBData->Ver.SVN[3]);
#endif

      return (StatusMgDdi);	
  
 }

/*******************************************************************************
 *Function name: MGDDIInhibitSTB
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
 *      void  MGDDIInhibitSTB(void)     
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
 void  MGDDIInhibitSTB(void)
 {
         /*do_report(severity_info,"\n MGDDIInhibitSTB  \n");*/
	  CHCA_BOOL             bErrCode;

	  bErrCode = CHCA_InhibitStb();	 

	  if(bErrCode)
	  {
                do_report(severity_info,"\n[MGDDIInhibitSTB::]fail to  inhibit the stb \n");
	  }

 } 

/*******************************************************************************
 *Function name:MGDDIUnsubscribe
 *           
 *
 *Description:This function is used to cancel the subscription identified by the
 *            hSubscription parameter.This identifier is obtained through a DDI
 *            subscription.
 *
 *Prototype:
 *      TMGDDIStatus   MGDDIUnsubscribe     
 *           (TMGDDIEventSubscriptionHandle   hSubscription)
 *
 *
 *input:
 *    hSubscription:            Event subscription handle       
 *           
 *
 *output:
 *           
 *
 *
 *Return code:
 *    MGDDIOk:        Subscription cancelled. 
 *    MGDDIError:     Subscription handle unknown.
 *
 *Comments:
 *           
 *******************************************************************************/
 TMGDDIStatus   MGDDIUnsubscribe
	 (TMGDDIEventSubscriptionHandle  hSubscription)
 {
         TMGDDIStatus                                       StatusMgDdi = MGDDIOK;
         CHCA_UCHAR                                       SystypeBaseadd[2];
	  CHCA_USHORT                                      /*iHandleCount,*/iHandle;	
	  CHCA_UINT                                           i;
         WORD2BYTE                                          SubscriptionHandle; 

         if(hSubscription == NULL)
         { 
              StatusMgDdi = MGDDIError;
#ifdef    MGGENERAL_DRIVER_DEBUG
              do_report(severity_info,"\n %s %d >[MGDDIUnsubscribe::] Subscription handle unknown\n",
                                __FILE__,
                                __LINE__);
#endif
              return StatusMgDdi;
	  }

         SubscriptionHandle.uiWord32 = (CHCA_UINT)hSubscription;

	  SystypeBaseadd[0] = SubscriptionHandle.byte.ucByte2;
	  SystypeBaseadd[1] = SubscriptionHandle.byte.ucByte3;

         iHandle = (CHCA_USHORT)SubscriptionHandle.byte.ucByte0;

         if((iHandle==0)||(iHandle>=CTRL_MAX_NUM_SUBSCRIPTION))  
         { 
               StatusMgDdi = MGDDIError;
#ifdef     MGGENERAL_DRIVER_DEBUG
               do_report(severity_info,"\n %s %d >[MGDDIUnsubscribe::] Subscription handle unknown\n",
                                 __FILE__,
                                 __LINE__);
#endif
              return StatusMgDdi;
	  }

         if((SystypeBaseadd[0] | (SystypeBaseadd[1]<<8))!= CHCA_MGDDI_SRC_BASEADDRESS)
         {
                 /*the card subscription*/
                 if((iHandle==0) ||( iHandle>=2))
		   {
                         StatusMgDdi = MGDDIError;
#ifdef               MGGENERAL_DRIVER_DEBUG
                         do_report(severity_info,"\n %s %d >[MGDDIUnsubscribe::] Subscription handle unknown\n",
                                           __FILE__,
                                           __LINE__);
#endif
                        return StatusMgDdi;
           
		   }

		   if(CHCA_CardUnsubscribe(iHandle)==true)
		   {
                         StatusMgDdi = MGDDIError;
#ifdef               MGGENERAL_DRIVER_DEBUG
                         do_report(severity_info,"\n %s %d >[MGDDIUnsubscribe::] card interface error\n",
                                           __FILE__,
                                           __LINE__);
#endif
                        return StatusMgDdi;
		   }
		   
#ifdef       MGGENERAL_DRIVER_DEBUG
		   do_report(severity_info,"\n Card Subscription cancelled \n");
#endif
	  }
	  else
	  {
	         /*the ctrl subscription*/
	         if(CHCA_CtrlUnsubscribe(iHandle)==true)
	         {
                        StatusMgDdi = MGDDIError;
#ifdef              MGGENERAL_DRIVER_DEBUG
                        do_report(severity_info,"\n %s %d >[MGDDIUnsubscribe::] ctrl interface error\n",
                                          __FILE__,
                                          __LINE__);
#endif
                        return StatusMgDdi;
		  }
			 
#ifdef      MGGENERAL_DRIVER_DEBUG		  
		  do_report(severity_info,"\n Stream Source Subscription cancelled \n");
#endif
	  }


	  return StatusMgDdi;
 	 
 }


/******************************************************************************
*
* File : MgUtilityDrv.C
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
*           
*
* Copyright: Changhong 2004 (c)
*
*****************************************************************************/


/*****************************************************************************
 *Interface description
 *****************************************************************************/


/*******************************************************************************
 *Function name : MGDDIGetTickTime
 *           
 *
 *Description: This function recovers the number of CPU or timer cycles that have
 *             elapsed since the STB was powered up.This value progresses in cycles,
 *             i.e.once the maximum value has been reached,it restarts from 0
 *
 *Prototype:
 *     udword  MGDDIGetTickTime()       
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
 *       Absolute time value from power-up,expressed in CPU or timer cycles.
 *
 *
 *Comments:
 *           
 *******************************************************************************/
 udword  MGDDIGetTickTime(void) 
 {
       CHCA_TICKTIME          iCurTime;
 
       iCurTime = CHCA_GetCurrentClock();

       do_report(severity_info,"\n [MGDDIGetTickTime]  the number[%d] of CPU",iCurTime);

        return  (udword)iCurTime;

 }	  

/*******************************************************************************
 *Function name: MGDDITraceString
 *           
 *
 *Description: This function displays a character string on a debugging peripheral
 *             (if any).The character string ends with a null byte and consists of
 *             a maximum of 255 characters.
 *
 *
 *Prototype:
 *    void   MGDDITraceString(ubyte* pStr)      
 *           
 *
 *
 *input:
 *    pStr:     Character string to be displayed.       
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
 void   MGDDITraceString(ubyte* pStr)
 {



#if  0 /*no trace data on 050508*/
       CHCA_Report(pStr);
#endif
	  
 }
/*******************************************************************************
 *Function name:MGDDITraceSetAsync
 *           
 *
 *Description: This function switches trace mode from asunchronous to sunchronous
 *             depending on the value of bAsync flag.If FALSE,trace mode is 
 *             synchronous.Otherwise it is asynchronous. 
 *
 *
 *Prototype:
 *      void  MGDDITraceSetAsync(boolean bAsync)       
 *           
 *
 *
 *input:
 *      bAsync:       Indicates if the trace mode is asynchronous.     
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
 void  MGDDITraceSetAsync(boolean bAsync)
 {
      do_report(severity_info,"\n MGDDITraceSetAsync \n");
 }	  

