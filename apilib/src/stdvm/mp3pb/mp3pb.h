/*****************************************************************************

File Name  : mp3pb.h

Description: MP3 Playback header

COPYRIGHT (C) 2005 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __MP3PB_H
#define __MP3PB_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"    /* STAPI includes */

/* Exported Constants -------------------------------------------------- */
#define MP3PB_DRIVER_ID         (0x0FAE)
#define MP3PB_DRIVER_BASE       (MP3PB_DRIVER_ID<<16)

#define MP3PB_MAX_NAME_LENGTH   (256)

/* Exported Types ------------------------------------------------------ */

typedef enum MP3PB_Event_s
{
    MP3PB_EVT_END_OF_PLAYBACK=MP3PB_DRIVER_BASE, /* Last audio frame played */
    MP3PB_EVT_END_OF_FILE,                       /* Reach end of file                                  */
    MP3PB_EVT_ERROR                              /* An error has occur, check error parameter          */
} MP3PB_Event_t;


typedef enum MP3PB_EventIDs_s
{
    MP3PB_EVT_END_OF_PLAYBACK_ID,
    MP3PB_EVT_END_OF_FILE_ID,
    MP3PB_EVT_ERROR_ID,
    MP3PB_MAX_EVTS
};


/* MP3PB error code definitions */
enum
{
    MP3PB_ERROR_AUD_START_FAILED = MP3PB_DRIVER_BASE
};


typedef struct MP3PB_InitParams_s
{
    ST_DeviceName_t     EVTDeviceName;   /* Device name used to notify MP3 events */
    ST_Partition_t     *CachePartition;  /* Memory cache partition                */
    ST_Partition_t     *NCachePartition; /* Memory non cache partition            */
} MP3PB_InitParams_t;


typedef struct MP3PB_TermParams_s
{
    BOOL                ForceTerminate;
} MP3PB_TermParams_t;


typedef struct MP3PB_OpenParams_s
{
    ST_DeviceName_t     AUDDeviceName;   /* AUD Device name used to subscribe AUD events */
    STAUD_Handle_t      AUDHandle;
} MP3PB_OpenParams_t;


typedef U32 MP3PB_Handle_t;


typedef struct MP3PB_PlayStartParams_s
{
    char               SourceName[MP3PB_MAX_NAME_LENGTH];      /* Name of the source file */
} MP3PB_PlayStartParams_t;


typedef struct MP3PB_PlayStopParams_s
{
    STAUD_Stop_t       AudioStopMethod;     /* Define the way to stop the audio decoder          */
    STAUD_Fade_t       AudioFadingMethod;   /* Define the fading to perform on the audio decoder */
} MP3PB_PlayStopParams_t;



/* Exported Variables -------------------------------------------------- */

/* Exported Macros ----------------------------------------------------- */

/* Exported Functions -------------------------------------------------- */

ST_ErrorCode_t MP3PB_Init(ST_DeviceName_t DeviceName, MP3PB_InitParams_t *InitParams);
ST_Revision_t  MP3PB_GetRevision(void);
ST_ErrorCode_t MP3PB_Term(ST_DeviceName_t DeviceName, MP3PB_TermParams_t *TermParams);
ST_ErrorCode_t MP3PB_Open(ST_DeviceName_t DeviceName, MP3PB_OpenParams_t *OpenParams, MP3PB_Handle_t *Handle);
ST_ErrorCode_t MP3PB_Close(MP3PB_Handle_t Handle);
ST_ErrorCode_t MP3PB_PlayStart(MP3PB_Handle_t Handle, MP3PB_PlayStartParams_t *PlayStartParams);
ST_ErrorCode_t MP3PB_PlayStop(MP3PB_Handle_t Handle, MP3PB_PlayStopParams_t *PlayStopParams);


#endif /* __MP3PB_H */

/* EOF --------------------------------------------------------------------- */

