#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"
#include "stsys.h"

#include "stlayer.h"
#include "stvtg.h"

#include "stgxobj.h"
#include "sttbx.h"
#include "stevt.h"
#include "initterm.h"

 



/*-----------------------------For debug purposes ...---------------------------------*/
#define VIDEO_DEBUG_EVENT/**/		/*To subscribe all STVID event and print*/
/*#define AUDIO_DEBUGPRINT*/		/*To subscribe all STAUD event and print*/
#define PTI_DEBUG_EVENT/**/		/*To subscribe all PTI event and print*/
#define CLKRV_DEBUG_EVENT/**/		/*To subscribe all CLKRV event and print*/
/*CS*/


/* Functions prototypes --------------------------------------------------- */
#ifdef VIDEO_DEBUG_EVENT
static U32                   VideoEvent_STVID_DISPLAY_NEW_FRAME_EVT_Count = 0;
static U32                   VideoEvent_Errors_Count = 0;

void avc_VideoEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p);
#endif
#ifdef AUDIO_DEBUGPRINT
static U32                   AudioEvent_Errors_Count = 0;
static task_t  		     *AudioEventTask = NULL;
static semaphore_t	     semSignalAudEvt;
static char		     AudioEventText[30];
static void avc_AudioEvent_Task ( void *pvParam );
static void avc_AudioEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p);
#endif
#ifdef PTI_DEBUG_EVENT
static U32                   PTIEvent_Errors_Count = 0;
void avc_PTIEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p);
#endif
#ifdef CLKRV_DEBUG_EVENT
static U32                  CLKRVEvent_Errors_Count = 0;
void avc_CLKRVEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p);
#endif


 void avc_Vid1DispNewFrameCallBack(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p)
{
	ST_ErrorCode_t ST_ErrorCode;
	STVID_PictureParams_t *VID_PictureParams; /*cs_FrameRate*/	
	/*STVID_SynchronisationInfo_t *VID_SyncInfo;		*/
	


	switch (Event)
	{							
		case STVID_OUT_OF_SYNC_EVT:											
			#ifdef VIDEO_DEBUG_EVENT
			STTBX_Print(("\n error!! STVID_OUT_OF_SYNC_EVT\n"));
			#endif
			break;
			
		case STVID_BACK_TO_SYNC_EVT:						
			#ifdef VIDEO_DEBUG_EVENT
			STTBX_Print(("\n error!! STVID_BACK_TO_SYNC_EVT\n"));
			#endif
			
			break;
		
		case STVID_FRAME_RATE_CHANGE_EVT:
			STTBX_Print(("\n error!! STVID_FRAME_RATE_CHANGE_EVT e\n"));
			
			#ifdef AUTO_FRAME_DETECTION
			VID_PictureParams = (STVID_PictureParams_t *) EventData;
			
			if (iFrameRate == 25000 || iFrameRate == 50000) 
			{							
				/*if switching from 50hz stream to 60hz or vice versa, change video display setup*/
				if (VID_PictureParams->FrameRate == 30000
					|| VID_PictureParams->FrameRate == 59940
				          || VID_PictureParams->FrameRate == 60000)
				{ 
					iFrameRate = VID_PictureParams->FrameRate;
    					SRMNU_FeedbackMsg(feedback_kChannel,msg_FrameRateConvert);
					STTBX_Print(("\n error!! STVID_FRAME_RATE_CHANGE_EVT f %d\n", iFrameRate));				
				}
			}
			else if (iFrameRate == 30000 || iFrameRate == 59940 || iFrameRate == 60000) 			
			{
				/*if switching from 50hz stream to 60hz or vice versa, change video display setup*/
				if (VID_PictureParams->FrameRate == 25000
					|| VID_PictureParams->FrameRate == 50000) 
				{ 
					iFrameRate = VID_PictureParams->FrameRate;
    					SRMNU_FeedbackMsg(feedback_kChannel,msg_FrameRateConvert);    				
					STTBX_Print(("\n error!! STVID_FRAME_RATE_CHANGE_EVT g %d\n", iFrameRate));
				}
			}
				
			/*if detected framerate is not 50 hz or 60 hz, always display in 60hz*/
			if (iFrameRate != 25000 
				&& iFrameRate != 30000 
				    && iFrameRate != 50000 
				       && iFrameRate != 59940
				          && iFrameRate != 60000)
			{
				iFrameRate = 30000;
				SRMNU_FeedbackMsg(feedback_kChannel,msg_FrameRateConvert);
				STTBX_Print(("\n error!! STVID_FRAME_RATE_CHANGE_EVT %d\n", iFrameRate));
			}		
			#endif	
			break;		
		
		case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
						
		/*	if ( _waflag1 == TRUE)
			{
				_waflag1=FALSE;				
				ST_ErrorCode = SRAVC_EnableOutput(VID_1_MPEG, LAYER_VIDEO1);
				if (ST_ErrorCode == ST_NO_ERROR)
				{
					STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO1)=%s",
											GetErrorText(ST_ErrorCode)));	
				}
				else
				{
					STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO1)=%s",	
											GetErrorText(ST_ErrorCode)));
				}
				
				if( HD_OutputType[VOUT_AUX] != NULL )
				{
					ST_ErrorCode = SRAVC_EnableOutput(VID_1_MPEG, LAYER_VIDEO2);
					if (ST_ErrorCode == ST_NO_ERROR)
					{
						STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO2)=%s",	
												GetErrorText(ST_ErrorCode)));	
					}
					else
					{
						STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO2)=%s",	
												GetErrorText(ST_ErrorCode)));
					}
				}
			}*/
			break;
	}

}


#ifdef VIDEO_DEBUG_EVENT
void avc_VideoEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p)
{    
        switch(Event)
        {
            case STVID_DISPLAY_NEW_FRAME_EVT:
            case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
	    case STVID_NEW_PICTURE_DECODED_EVT:
            case STVID_USER_DATA_EVT:            	
            case STVID_SEQUENCE_INFO_EVT:            	
            case STVID_SYNCHRO_EVT:
            	break;
				
				

            /* general report on these events with error count */
            case STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT:
            	VideoEvent_Errors_Count++; 		            	
 		STTBX_Print(("\n error!! STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT\t %d \n",   VideoEvent_Errors_Count));  
                break;
				
            case STVID_DATA_ERROR_EVT:
            	            	VideoEvent_Errors_Count++; 		
 		STTBX_Print(("\n error!! STVID_DATA_ERROR_EVT\t %d \n",   VideoEvent_Errors_Count));  
                break;
				
            case STVID_DATA_OVERFLOW_EVT:
 		            	VideoEvent_Errors_Count++; 		
 		STTBX_Print(("\n error!! STVID_DATA_OVERFLOW_EVT\t %d \n",   VideoEvent_Errors_Count));  
                break;
				
            case STVID_DATA_UNDERFLOW_EVT:
		VideoEvent_Errors_Count++; 		
 		STTBX_Print(("\n error!! STVID_DATA_UNDERFLOW_EVT\t %d \n",   VideoEvent_Errors_Count));  
                break;

            case STVID_PICTURE_DECODING_ERROR_EVT:
 		VideoEvent_Errors_Count++; 		
 		STTBX_Print(("\n error!! STVID_PICTURE_DECODING_ERROR_EVT\t %d \n",   VideoEvent_Errors_Count));  
                break;
            
            case STVID_OUT_OF_SYNC_EVT:            
		VideoEvent_Errors_Count++; 		
 		STTBX_Print(("\n error!! STVID_OUT_OF_SYNC_EVT\t %d \n",   VideoEvent_Errors_Count));
                break;	
                
            case STVID_BACK_TO_SYNC_EVT:
            	STTBX_Print(("\n error!! STVID_BACK_TO_SYNC_EVT ee\n"));
                break;	
                
            case STVID_DIGINPUT_WIN_CHANGE_EVT:
            	STTBX_Print(("\n error!! STVID_DIGINPUT_WIN_CHANGE_EVT ee\n"));  
            	break;
            case STVID_RESOLUTION_CHANGE_EVT:
            	STTBX_Print(("\n error!! STVID_RESOLUTION_CHANGE_EVT ee\n"));  
            	break;
	     case STVID_SPEED_DRIFT_THRESHOLD_EVT:
	     	STTBX_Print(("\n error!! STVID_SPEED_DRIFT_THRESHOLD_EVT ee\n"));
            	break;
            case STVID_STOPPED_EVT:            
            	STTBX_Print(("\n error!! STVID_STOPPED_EVT ee\n"));  
            	break;
            case STVID_ASPECT_RATIO_CHANGE_EVT:
            	STTBX_Print(("\n error!! STVID_ASPECT_RATIO_CHANGE_EVT ee\n"));  
            	break;
            case STVID_SCAN_TYPE_CHANGE_EVT:
            	STTBX_Print(("\n error!! STVID_SCAN_TYPE_CHANGE_EVT ee\n"));  
                break;
	    case STVID_SYNCHRONISATION_CHECK_EVT:	
	    	STTBX_Print(("\n error!! STVID_SYNCHRONISATION_CHECK_EVT ee \n"));
	    	break;
            default:
		STTBX_Print(("default error\n"));
                break;
        }         
}
#endif

#ifdef AUDIO_DEBUGPRINT
static void avc_AudioEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p)
{    	        		
        switch(Event)
        {
  	    case STAUD_NEW_FRAME_EVT:				       		
	    case STAUD_EMPHASIS_EVT:
       	    case STAUD_PREPARED_EVT:
            case STAUD_RESUME_EVT:
            case STAUD_PCM_BUF_COMPLETE_EVT:            	
                break;

	    /* general report on these events with error count */
            case STAUD_DATA_ERROR_EVT:	              		
		AudioEvent_Errors_Count++;
		semaphore_signal (&semSignalAudEvt);		
		strcpy (AudioEventText,"STAUD_DATA_ERROR_EVT");		
		break;
		
	    case STAUD_SYNC_ERROR_EVT:	    	
		AudioEvent_Errors_Count++;     
		semaphore_signal (&semSignalAudEvt);		
		strcpy (AudioEventText,"STAUD_SYNC_ERROR_EVT");		
		break;

	    case STAUD_STOPPED_EVT:            	
   	    case STAUD_NEW_ROUTING_EVT:            	
	    case STAUD_NEW_FREQUENCY_EVT:            	
	    	break;
	    	
   	    case STAUD_LOW_DATA_LEVEL_EVT:   	    	
		AudioEvent_Errors_Count++;     
		semaphore_signal (&semSignalAudEvt);		 
		strcpy (AudioEventText,"STAUD_LOW_DATA_LEVEL_EVT");		
		break;
	    case STAUD_PCM_UNDERFLOW_EVT:	        
		AudioEvent_Errors_Count++;  	
		semaphore_signal (&semSignalAudEvt);			
		strcpy (AudioEventText,"STAUD_PCM_UNDERFLOW_EVT");		
		break;
            case STAUD_FIFO_OVERF_EVT:                
		AudioEvent_Errors_Count++;  	    
		semaphore_signal (&semSignalAudEvt);		
		strcpy (AudioEventText,"STAUD_FIFO_OVERF_EVT");		
		break;
	    case STAUD_CDFIFO_UNDERFLOW_EVT:                	    	
		AudioEvent_Errors_Count++;  
		semaphore_signal (&semSignalAudEvt);		
		strcpy (AudioEventText,"STAUD_CDFIFO_UNDERFLOW_EVT");		
	        break;
	
	    default:
		break;
        }        	
}

static void avc_AudioEvent_Task ( void *pvParam ) 
{
	while (1) 
	{
		semaphore_wait (&semSignalAudEvt); 
		STTBX_Print(("%s\t %d \n",   AudioEventText, AudioEvent_Errors_Count));
	}
}
#endif

#ifdef PTI_DEBUG_EVENT
void avc_PTIEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p)
{
	switch(Event)
        {        	                    	        	
            case STPTI_EVENT_BUFFER_OVERFLOW_EVT:
            	PTIEvent_Errors_Count++; 		
 		STTBX_Print(("\n STPTI_EVENT_BUFFER_OVERFLOW_EVT\t %d \n",   PTIEvent_Errors_Count));  
 		break;
            case STPTI_EVENT_CC_ERROR_EVT:
            	PTIEvent_Errors_Count++; 		
 		STTBX_Print(("\n STPTI_EVENT_CC_ERROR_EVT\t %d \n",   PTIEvent_Errors_Count));  	                                        
 		break;
 	    case STPTI_EVENT_INTERRUPT_FAIL_EVT:
 	    	PTIEvent_Errors_Count++; 		
 		STTBX_Print(("\n STPTI_EVENT_INTERRUPT_FAIL_EVT\t %d \n",   PTIEvent_Errors_Count));
 		break;
            case STPTI_EVENT_INVALID_PARAMETER_EVT:
            	PTIEvent_Errors_Count++; 		
 		STTBX_Print(("\n STPTI_EVENT_INVALID_PARAMETER_EVT\t %d \n",   PTIEvent_Errors_Count));  
 		break;
            case STPTI_EVENT_PACKET_ERROR_EVT:
            	PTIEvent_Errors_Count++; 		
 		STTBX_Print(("\n STPTI_EVENT_PACKET_ERROR_EVT\t %d \n",   PTIEvent_Errors_Count));               		             		
 		break;
            case STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT:
		PTIEvent_Errors_Count++; 		
 		STTBX_Print(("\n STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT\t %d \n",   PTIEvent_Errors_Count));  
            	break;                  
            case STPTI_EVENT_TC_CODE_FAULT_EVT:            
				PTIEvent_Errors_Count++; 		
				STTBX_Print(("\n STPTI_EVENT_TC_CODE_FAULT_EVT\t %d \n",   PTIEvent_Errors_Count));  
			case STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT:
			case STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT:
			case STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT:
			case STPTI_EVENT_PCR_RECEIVED_EVT:            			 		
			case STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT:	    	 		
			case STPTI_EVENT_DMA_COMPLETE_EVT:      
			case STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT:            	 		      	
			case STPTI_EVENT_INVALID_LINK_EVT:
			case STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT:
			case STPTI_EVENT_CWP_BLOCK_EVT:
			case STPTI_EVENT_INDEX_MATCH_EVT:

				PTIEvent_Errors_Count++; 		
			/*	STTBX_Print(("STPTI_EVENT_YXL %d Event=%x %x %x\n", 
					PTIEvent_Errors_Count,Event,STPTI_EVENT_BASE,STPTI_EVENT_PCR_RECEIVED_EVT)); */ 
                break;                  
	}
}
#endif                                               
#ifdef CLKRV_DEBUG_EVENT
void avc_CLKRVEvent_CallbackProc(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p)
{
	switch(Event)
        {        	                    	        	
            case STCLKRV_PCR_VALID_EVT:
            	CLKRVEvent_Errors_Count++; 		
 		STTBX_Print(("STCLKRV_PCR_VALID_EVT\t %d \n",   CLKRVEvent_Errors_Count));  
 		break;
            case STCLKRV_PCR_DISCONTINUITY_EVT:
            	CLKRVEvent_Errors_Count++; 		
 		STTBX_Print(("STCLKRV_PCR_DISCONTINUITY_EVT\t %d \n",   CLKRVEvent_Errors_Count));  	                                        
 		break;
 	    case STCLKRV_PCR_GLITCH_EVT: 	    	
 	    	CLKRVEvent_Errors_Count++; 		
 		STTBX_Print(("STCLKRV_PCR_GLITCH_EVT\t %d \n",   CLKRVEvent_Errors_Count));
                break;                  
	}
}
#endif           

#if 0

static void avc_Vid1DispNewFrameCallBack(STEVT_CallReason_t     Reason,
                           const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t  Event, 
                           const void            *EventData,
                           const void            *SubscriberData_p)
{
	ST_ErrorCode_t ST_ErrorCode;
	STVID_PictureParams_t *VID_PictureParams; /*cs_FrameRate*/	
	/*STVID_SynchronisationInfo_t *VID_SyncInfo;		*/
	
	//TMTM test only
	//STTBX_Print(("avc evt=%d\n", Event));

	switch (Event)
	{							
		case STVID_OUT_OF_SYNC_EVT:											
			#ifdef VIDEO_DEBUG_EVENT
			STTBX_Print(("STVID_OUT_OF_SYNC_EVT\n"));
			#endif
			break;
			
		case STVID_BACK_TO_SYNC_EVT:						
			#ifdef VIDEO_DEBUG_EVENT
			STTBX_Print(("STVID_BACK_TO_SYNC_EVT\n"));
			#endif
			
			break;
		
		case STVID_FRAME_RATE_CHANGE_EVT:
			#ifdef AUTO_FRAME_DETECTION
			VID_PictureParams = (STVID_PictureParams_t *) EventData;
			
			if (iFrameRate == 25000 || iFrameRate == 50000) 
			{							
				/*if switching from 50hz stream to 60hz or vice versa, change video display setup*/
				if (VID_PictureParams->FrameRate == 30000
					|| VID_PictureParams->FrameRate == 59940
				          || VID_PictureParams->FrameRate == 60000)
				{ 
					iFrameRate = VID_PictureParams->FrameRate;
    					SRMNU_FeedbackMsg(feedback_kChannel,msg_FrameRateConvert);
					STTBX_Print(("STVID_FRAME_RATE_CHANGE_EVT %d\n", iFrameRate));				
				}
			}
			else if (iFrameRate == 30000 || iFrameRate == 59940 || iFrameRate == 60000) 			
			{
				/*if switching from 50hz stream to 60hz or vice versa, change video display setup*/
				if (VID_PictureParams->FrameRate == 25000
					|| VID_PictureParams->FrameRate == 50000) 
				{ 
					iFrameRate = VID_PictureParams->FrameRate;
    					SRMNU_FeedbackMsg(feedback_kChannel,msg_FrameRateConvert);    				
					STTBX_Print(("STVID_FRAME_RATE_CHANGE_EVT %d\n", iFrameRate));
				}
			}
				
			/*if detected framerate is not 50 hz or 60 hz, always display in 60hz*/
			if (iFrameRate != 25000 
				&& iFrameRate != 30000 
				    && iFrameRate != 50000 
				       && iFrameRate != 59940
				          && iFrameRate != 60000)
			{
				iFrameRate = 30000;
				SRMNU_FeedbackMsg(feedback_kChannel,msg_FrameRateConvert);
				STTBX_Print(("STVID_FRAME_RATE_CHANGE_EVT %d\n", iFrameRate));
			}		
			#endif	
			break;		
		
		case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT:
						
		/*	if ( _waflag1 == TRUE)
			{
			/*	_waflag1=FALSE;			
				ST_ErrorCode = SRAVC_EnableOutput(VID_1_MPEG, LAYER_VIDEO1);
				if (ST_ErrorCode == ST_NO_ERROR)
				{
					STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO1)=%s",
											GetErrorText(ST_ErrorCode)));	
				}
				else
				{
					STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO1)=%s",	
											GetErrorText(ST_ErrorCode)));
				}
				
				if( HD_OutputType[VOUT_AUX] != NULL )
				{
					ST_ErrorCode = SRAVC_EnableOutput(VID_1_MPEG, LAYER_VIDEO2);
					if (ST_ErrorCode == ST_NO_ERROR)
					{
						STTBX_Report((STTBX_REPORT_LEVEL_INFO, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO2)=%s",	
												GetErrorText(ST_ErrorCode)));	
					}
					else
					{
						STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "SRAVC_EnableOutput(VID_1_MPEG,LAYER_VIDEO2)=%s",	
												GetErrorText(ST_ErrorCode)));
					}
				}
			}*/
			break;
	}

}

#endif




/*EOF*/
