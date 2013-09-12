/*****************************************************************************

File name   :  trictest.c

Description :  Utilities to test trickmode quality

COPYRIGHT (C) ST-Microelectronics 2005

Date               Modification                 Name
----               ------------                 ----
 06/11/07          Created                      OB

*****************************************************************************/

/* Includes */
/* -------- */
#ifdef STFAE
#include "stapp_main.h"
#include "stdebug_main.h"
#include "trictest.h"
#else
#include <stdio.h>
#include "hdd.h"
#include "trictest.h"
#include "vid_inj.h"
#include "testcfg.h"
#include "stdevice.h"
#include "stevt.h"
#include "stvid.h"
#include "stavmem.h"
#include "stddefs.h"
#include "sttbx.h"
#include "stcommon.h"
#include "testtool.h"
#include "swconfig.h"
#include "startup.h"
#include <string.h>
#include "clevt.h"
#endif

#ifdef STFAE
#define EVT_DEVICE_NAME EVT_DeviceName[0]
#define message(x) print x
#else
#define EVT_DEVICE_NAME STEVT_DEVICE_NAME
#define SYS_FWrite(a,b,c,d) STTBX_Print((a))
#define SYS_FClose(a)
#define SYS_FOpen(a,b) ((void*)1)
#define message(x)
#endif

typedef enum
{
    STOPPED,
    STARTED
} PlayStatus_t;

static STEVT_Handle_t EvtHandle;
static char CurrentStreamName[256];
static void *FILE_Handle = NULL;
static PlayStatus_t CurrentPlayStatus = STOPPED;
static S32 Speed = 100;

static ST_ErrorCode_t VideoEventsSubscription(void);
static void VideoEventsCallBack(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData);
static void PlaybackEventsCallBack(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData);
static ST_ErrorCode_t PlaybackEventsSubscription(void);

BOOL TT_TRICKMOD_WaitMinute(parse_t *pars_p,char *result);
BOOL TT_TRICKMOD_LogPTS(parse_t *pars_p,char *result);

/* ========================================================================
   Name: TRICKMOD_Debug       
   Description: Init function to subscribe to events and register testtool
                commands
   ======================================================================== */
ST_ErrorCode_t TRICKMOD_Debug(void)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STEVT_OpenParams_t            EVT_OpenParams;

    /* Get an Evt handle */
    memset(&EVT_OpenParams, 0, sizeof(STEVT_OpenParams_t));
    memset(CurrentStreamName, 0, sizeof(CurrentStreamName));
    ErrorCode = STEVT_Open(EVT_DEVICE_NAME, &EVT_OpenParams, &(EvtHandle));

    /* Subscribe to events */
    ErrorCode |= VideoEventsSubscription();
    ErrorCode |= PlaybackEventsSubscription();

    if(ErrorCode != ST_NO_ERROR)
    {
        return ErrorCode;
    }
    
    register_command("TRICKMOD_WAIT", TT_TRICKMOD_WaitMinute, "Waits one second");
    register_command("TRICKMOD_LOG", TT_TRICKMOD_LogPTS, "<filename> Logs PTS to filename");

    return ErrorCode;
} /* End of function TRICKMOD_Debug() */

/* ========================================================================
   Name: TT_TRICKMOD_WaitMinute
   Description: Testtool command to wait for exactely one minute
   ======================================================================== */
BOOL TT_TRICKMOD_WaitMinute(parse_t *pars_p,char *result)
{
    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(result);

    STOS_TaskDelay(60*ST_GetClocksPerSecond());
    
    return(FALSE);
} /* End of function TT_TRICKMOD_WaitMinute() */

/* ========================================================================
   Name: VideoEventsSubscription       
   Description: Init function that subscribes to the video events
   ======================================================================== */
static ST_ErrorCode_t VideoEventsSubscription(void)
{
    STEVT_SubscribeParams_t HddSubscribeParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    /* Warning : Events subscription and configuration supposes one instance */
    HddSubscribeParams.NotifyCallback = VideoEventsCallBack;
    ErrorCode  = STEVT_Subscribe (EvtHandle, STVID_DATA_UNDERFLOW_EVT,       &HddSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, STVID_DATA_OVERFLOW_EVT,        &HddSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, STVID_SPEED_DRIFT_THRESHOLD_EVT,&HddSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT, &HddSubscribeParams);

    return ErrorCode;
} /* End of function VideoEventsSubscription() */

/* ========================================================================
   Name: VideoEventsCallBack       
   Description: Callback function to handle video events
   ======================================================================== */
static void VideoEventsCallBack(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    char                        buffer[256];
    STVID_PictureInfos_t PictureInfos;
    UNUSED_PARAMETER(Reason);

    switch (Event)
    {
        case STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT :
            memcpy(&PictureInfos, EventData, sizeof(PictureInfos));
            if(FILE_Handle != NULL)
            {
#if defined(ARCHITECTURE_ST40)||defined(PROCESSOR_SH4)
                sprintf(buffer, "%llu:0x%x%08x\r\n",
                        time_now(),
                        PictureInfos.VideoParams.PTS.PTS33,
                        PictureInfos.VideoParams.PTS.PTS);
#else
                sprintf(buffer, "%d:0x%x%08x\r\n",
                        time_now(),
                        PictureInfos.VideoParams.PTS.PTS33,
                        PictureInfos.VideoParams.PTS.PTS);
#endif /* ARCHITECTURE_ST40 */
                SYS_FWrite(buffer, strlen(buffer), 1, FILE_Handle);
            }
            break;
        default:            
            break;
    }
} /* End of function VideoEventsCallBack() */

/* ========================================================================
   Name: TT_TRICKMOD_LogPTS        
   Description: Testtool command to log PTS/time arrivals for pictures 
                displayed
   ======================================================================== */
BOOL TT_TRICKMOD_LogPTS(parse_t *pars_p,char *result)
{
    char FileName[256];
    char buffer[256];
    char DefaultName[256];
    void *temp_FILE_Handle = NULL;
    UNUSED_PARAMETER(result);
    
    sprintf(DefaultName, "trick_%s_%d.log", CurrentStreamName, Speed);

    /* Get Filename to load */
    /* -------------------- */
    if (cget_string(pars_p, DefaultName, FileName,sizeof(FileName))==TRUE)
    {
        tag_current_line(pars_p,"Invalid Filename, should be [\"pts.log\"]");
        return(TRUE);
    }

    if(FILE_Handle != NULL) 
    {
        SYS_FClose(FILE_Handle);
    }
    
    /* Open file */
    temp_FILE_Handle = SYS_FOpen((char *)FileName,"w");
    if (temp_FILE_Handle==NULL) 
    {
        char TempBuffer[256];
        
        sprintf(TempBuffer, "Couldn't open the file %s !", FileName);
        tag_current_line(pars_p, TempBuffer);
        return(TRUE);
    }

    message(("Started logging to %s\r\n", FileName));


    sprintf(buffer, "StreamName:%s\r\n", CurrentStreamName);
    SYS_FWrite(buffer, strlen(buffer), 1, temp_FILE_Handle);

#ifdef STFAE
    sprintf(buffer, "Speed:%d\r\n\r\n", Speed);
#else
    STVID_GetSpeed(VID_Inj_GetDriverHandle(0), &Speed);
    sprintf(buffer, "Speed:%d\r\n\r\n", Speed);
#endif
    SYS_FWrite(buffer, strlen(buffer), 1, temp_FILE_Handle);

    FILE_Handle = temp_FILE_Handle;

    return(FALSE);
} /* End of function TT_TRICKMOD_LogPTS() */


/* ========================================================================
   Name: PlaybackEventsSubscription       
   Description: Function subscribing to playback events
   ======================================================================== */
static ST_ErrorCode_t PlaybackEventsSubscription(void)
{
    STEVT_SubscribeParams_t PlaybackSubscribeParams;
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;

    PlaybackSubscribeParams.NotifyCallback = PlaybackEventsCallBack;
    ErrorCode  = STEVT_Subscribe (EvtHandle, PLAYREC_EVT_PLAY_STARTED, &PlaybackSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, PLAYREC_EVT_PLAY_STOPPED, &PlaybackSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, PLAYREC_EVT_PLAY_SPEED_CHANGED, &PlaybackSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, PLAYREC_EVT_PLAY_JUMP, &PlaybackSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, PLAYREC_EVT_END_OF_FILE, &PlaybackSubscribeParams);
    ErrorCode |= STEVT_Subscribe (EvtHandle, PLAYREC_EVT_END_OF_PLAYBACK, &PlaybackSubscribeParams);
    
    return ErrorCode;
} /* End of function PlaybackEventsSubscription() */

/* ========================================================================
   Name: PlaybackEventsCallBack       
   Description: Callback function handling playback events
   ======================================================================== */
static void PlaybackEventsCallBack(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    int i;
    UNUSED_PARAMETER(Reason);

    switch (Event)
    {
        case PLAYREC_EVT_PLAY_STARTED:
            memcpy(CurrentStreamName, EventData, sizeof(CurrentStreamName));
            for(i = strlen(CurrentStreamName) - 1; i >= 0; i--)
            {
                if((CurrentStreamName[i] == '\\') || (CurrentStreamName[i] == '/'))
                {
                    strcpy(CurrentStreamName, &CurrentStreamName[i + 1]);
                    break;
                }
            }
            CurrentPlayStatus = STARTED;
            break;
        case PLAYREC_EVT_PLAY_STOPPED:
        case PLAYREC_EVT_END_OF_FILE:
        case PLAYREC_EVT_END_OF_PLAYBACK:
            if(FILE_Handle != NULL)
            {
                SYS_FClose(FILE_Handle);
                FILE_Handle = NULL;
            }
            CurrentPlayStatus = STOPPED;
            break;
        case PLAYREC_EVT_PLAY_SPEED_CHANGED:
            memcpy(&Speed, EventData, sizeof(Speed));
            break;
        case PLAYREC_EVT_PLAY_JUMP:
            break;
    }
} /* End of function PlaybackEventsCallBack() */
