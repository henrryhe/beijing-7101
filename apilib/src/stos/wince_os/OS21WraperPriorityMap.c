/*
 *****************************************************************************
 * MODULE: wince_os
 *
 * FILE  : OS21WrapperPrioritymap.c
 *
 * AUTHOR: Noreen Tolland (STMicroelectronics, San Jose)
 * EMAIL : 
 * PHONE : 
 *
 * PURPOSE: assigns a task priority to every task from a pool of WinCE
 *          task priorities available.  	   
 *
 * $Revision:  $
 * $History:  $
 * 
 * *****************************************************************************
 */

/*
******************************************************************************
Includes
******************************************************************************
*/
#include <stdio.h>
#include <string.h>


/*
******************************************************************************
Private Constants
******************************************************************************
*/

/*
******************************************************************************
Private Types
*****************************************************************************
*/
/*
 * This represents the STAPI drivers used by MSTV
 *  153 = HIGHest Pri
 *  247 = LOWest  Pri
 *
 * NOTE: In WiNCE 0-96: Kernel, 97-152 WinCE Drivers, 
 *      153-247 Real-time Drivers, 248-255 Application
 *
 */
#if 0		//3B mapping 
typedef enum WinCE_PriorityType_e
{
  STFDMA_CALLBACK                  = 10,
  CONTROLLERTASK				   = 10,
  STBLIT_MASTER                    = 28,  
  STBLIT_SLAVE		               = 28, 
  STBLIT_INTERRUPT_PROCESS         = 27,
  STLAYER_GDP1					   = 26,
  STLAYER_GDP2					   = 26,
  STLAYER_GDP3					   = 26,  /* added for cut3.0 */
  STLAYER_CURSOR			       = 26,
  STLAYER_ALPHA					   = 26,
  STLAYER_VIDEO					   = 19,
  STAUDLX_PCM_PLAYER			   = 26,
  STAUDLX_DATAPROCESSER			   = 26,
  STAUDLX_STREAMING_AUDIO_DECODER  = 26,
  STAUDLX_AUDIO_MIXER			   = 26,
  STAUDLX_PP_TASK				   = 26,
  STAUDLX_SPDIF_TASK			   = 26,
  STVID_DISPLAY					   = 18,
  STVID_DECODER					   = 22,
  STVID_DECODE					   = 22,
  STVID_ERROR_RECOVERY			   = 31,
  STVID_TRICK_MODE	   			   = STVID_DISPLAY+2,
  STVID_PRIORITY_INJECTERS		   = 25,
  STVID_PARSER		   			   = 24,
  STVID_PRODUCER	   			   = 21,
  STVID_SPEED		   			   = 20,
  PES_ES_PARSER_PRIORITY		   = 21,
  PRE_PROC			               = 23,
  MPEGTIMER					       = 26,
  STVOUT_HDMI_PRIORITY             = 28,
  STVOUT_HDMI_INTERRUPT            = 17,
  STVOUT_HDMI_INTERRUPT_PROCESS    = 22,
  STCC_CLOSEDCAPTION			   = 32
} WinCE_PriorityType_t;

#else 

typedef enum WinCE_PriorityType_e
{

  STFDMA_CALLBACK                  = 98,
  CONTROLLERTASK				   = 98,
  STBLIT_MASTER                    = 251,  
  STBLIT_SLAVE		               = 251, 
  STBLIT_INTERRUPT_PROCESS         = 250,
  STLAYER_GDP1					   = 248,
  STLAYER_GDP2					   = 248,
  STLAYER_GDP3					   = 248,  /* added for cut3.0 */
  STLAYER_CURSOR			       = 248,
  STLAYER_ALPHA					   = 248,
  STLAYER_VIDEO					   = 245,
  STAUDLX_PCM_PLAYER			   = 253,
  STAUDLX_DATAPROCESSER			   = 253,
  STAUDLX_STREAMING_AUDIO_DECODER  = 253,
  STAUDLX_AUDIO_MIXER			   = 253,
  STAUDLX_PP_TASK				   = 253,
  STAUDLX_SPDIF_TASK			   = 253,
  STVID_DISPLAY					   = 243,
  STVID_DECODER					   = 245,
  STVID_DECODE					   = 245,
  STVID_ERROR_RECOVERY			   = 246,
  STVID_TRICK_MODE	   			   = 245,
  STVID_PRIORITY_INJECTERS		   = 246,
  STVID_PARSER		   			   = 245,
  STVID_PRODUCER	   			   = 245,
  STVID_SPEED		   			   = 251,
  PES_ES_PARSER_PRIORITY		   = 245,
  PRE_PROC			               = 245,
  MPEGTIMER					       = 247,
  STVOUT_HDMI_PRIORITY             = 230,
  STVOUT_HDMI_INTERRUPT            = 220,
  STVOUT_HDMI_INTERRUPT_PROCESS    = 225,
  STCC_CLOSEDCAPTION			   = 247
} WinCE_PriorityType_t;

#endif 
typedef struct 
{
	char* TaskName; 
	WinCE_PriorityType_t num;
} Priority_Table; 

/*
******************************************************************************
Private variables (static)
******************************************************************************
*/

static const Priority_Table PriorityTable[] =
{
  { "STFDMA_ClbckMgr"                  ,   STFDMA_CALLBACK                   } /* 99 */,
  { "ControllerTask"                       ,   CONTROLLERTASK                    } /* 99 */,
  { "STBLIT_ProcessHighPriorityQueue"      ,   STBLIT_MASTER                     } /* 4 */,
  { "STBLIT_ProcessLowPriorityQueue"       ,   STBLIT_SLAVE                      } /* 4 */,
  { "STBLIT_InterruptProcess"              ,   STBLIT_INTERRUPT_PROCESS          } /* 5 */,
  { "STLAYER-GFX/CUR - LAY_GDP1"		   ,   STLAYER_GDP1                      },
  { "STLAYER-GFX/CUR - LAY_GDP2"		   ,   STLAYER_GDP2                      },
  { "STLAYER-GFX/CUR - LAY_GDP3"		   ,   STLAYER_GDP3                      }, /* Addded for Cut3.0 */
  { "STLAYER-GFX/CUR - LAY_CURS"		   ,   STLAYER_CURSOR                    },
  { "STLAYER-GFX/CUR - LAY_ALPHA"		   ,   STLAYER_ALPHA                     },
  { "STLAYER_Video"                        ,   STLAYER_VIDEO                     } /* 10 */,
  { "PCMPlayerTask"						   ,   STAUDLX_PCM_PLAYER 		         },
  { "DataProcesserTask"                    ,   STAUDLX_DATAPROCESSER             } /* 2 */,
  { "DecoderTask"                          ,   STAUDLX_STREAMING_AUDIO_DECODER   } /* 2 */,
  { "PPTask"                               ,   STAUDLX_PP_TASK                   } /* 2 */,
  { "SPDIFPlayerTask"                      ,   STAUDLX_SPDIF_TASK                } /* 2 */,
  { "MixerTask"                            ,   STAUDLX_AUDIO_MIXER               } /* 2 */,
  { "MPEGTimer"						       ,   MPEGTIMER		 		         },
  { "STVID[0].DisplayTask"                 ,   STVID_DISPLAY                     } /* 12 */,
  { "STVID[0].H264DecoderTask"             ,  STVID_DECODER                      } /* 3 */,
//  { "STVID[0].DecoderTask"               ,  STVID_DECODER                      } /* 3 */,
  { "STVID[0].DecodeTask"                  ,   STVID_DECODE                      } /* 3 */,
  { "STVID[0].ErrorRecoveryTask"           ,   STVID_ERROR_RECOVERY              } /* 9 */,
  { "STVID[0].TrickModeTask"               ,   STVID_TRICK_MODE                  } /* 9 */,
//  { "STVID[0].ParserTask"                ,   STVID_PARSER                  } /* 9 */,
  { "STVID[0].H264ParserTask"              ,   STVID_PARSER                  } /* 9 */,
  { "STVID[0].MP4P2ParserTask"             ,   STVID_PARSER                  } /* 9 */,
  { "STVID[0].VC1ParserTask"               ,   STVID_PARSER                  } /* 9 */,
  { "STVID[0].ProducerTask"                ,   STVID_PRODUCER                  } /* 9 */,
  { "STVID[0].SpeedTask"                   ,   STVID_SPEED                  } /* 9 */,
  { "STVID.InjecterTask"                   ,   STVID_PRIORITY_INJECTERS          } /* 9 */,
  { "STVID_Injecter"                       ,   STVID_PRIORITY_INJECTERS          } /* 9 */,
  { "PESESTask"                            ,   PES_ES_PARSER_PRIORITY            }/* 10 */,
  { "STVID.H264PP[?]PreprocTask"           ,   PRE_PROC            }/* 10 */,
  { "STVOUT[?].InternalTask"               ,   STVOUT_HDMI_PRIORITY},
  { "STVOUT_INFOFRAME_TASK"				   ,   STVOUT_HDMI_INTERRUPT},
  { "STVOUT_STATE_MACHINE_INTERNAL_TASK"   ,   STVOUT_HDMI_INTERRUPT_PROCESS},
  { "STCC_ClosedCaption"   ,				   STCC_CLOSEDCAPTION},
  { "ff",                                      0xff                              }
};



/*
******************************************************************************
Private Macros
******************************************************************************
*/

/*
******************************************************************************
Global variables
******************************************************************************
*/

/*
******************************************************************************
Exported Global variables
******************************************************************************
*/

/*
******************************************************************************
Prototypes of all functions contained in this file (in order of occurance)
******************************************************************************
*/
int OS21toWinCEPriorityMap( const char* taskname );

/*
*****************************************************************************
Public Functions (not static)
******************************************************************************
*/

/*****************************************************************************
* Function Name : OS21toWinCEPriorityMap
* Author        : Noreen Tolland (STMicroelectronics, San Jose)
* Date created  : May/2006
* Description   : map os21 task priorities to WinCE task priorities.  
*
*    Input        -
*    Output       - 
*    Returns      - task priority 
*    
* History :
* Date      Author     Modification                          
*****************************************************************************/
int OS21toWinCEPriorityMap( const char* taskname )
{
  int i; /* loop variables */
  int adjusted;

  /* find the task name in the priority table */
  for (i = 0; PriorityTable[i].num != 0xff; i++)
  {
    char *ptIndex;
    /* do we have a matching task in the table? */
    
    /* do we have a multiple (with index) taskname */
    if ( (ptIndex=strstr(taskname, "[")) != NULL)
    {
      if (!strncmp(taskname, PriorityTable[i].TaskName, ptIndex-taskname))
      {
        if ( (ptIndex=strstr(taskname, "]")) != NULL)
        {
          if (strcmp(ptIndex+1, PriorityTable[i].TaskName+(ptIndex-taskname+1)) == 0)
			  break; //return(PriorityTable[i].num); /* return the task priority */
        }
      }    
    }
    else
    {
      if (strcmp(taskname, PriorityTable[i].TaskName) == 0)
      {
        break; //return(PriorityTable[i].num); /* return the task priority */
      }
    }
  }

#if 1 //ST: NT 11/20/06 
  if (PriorityTable[i].num != 0xff)
  {
	 if (PriorityTable[i].num >= 200)
		 adjusted = PriorityTable[i].num - 140;
		 else
		adjusted = PriorityTable[i].num;
     printf("JLX: OS21toWinCEPriorityMap(%s): found %s in slot %d with priority %d => %d\n", taskname, PriorityTable[i].TaskName, i, PriorityTable[i].num, adjusted);
     return adjusted;
  }
#endif  
  printf("OS21toWinCEPriorityMap: Failed to find task %s\n", taskname);
  
  return -1;  /* Didnt find the task */
}
/* OS21toWinCEPriorityMap */
