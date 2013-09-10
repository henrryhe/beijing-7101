/******************************************************************************\
 *
 * File Name   : stsubt.h
 *
 * Description : Exported types and functions for the subtitling decoder.
 *
 * Copyright 1998, 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Dec 1998, Raciti Nunzio 2001
 *
\******************************************************************************/

#ifndef __STSUBT_H
#define __STSUBT_H

/* -------------------------------------------------------------------------- */
/* --- Includes ------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#include <stddefs.h>   		/* for common defs 	 	 */
#include <stgxobj.h>       	/* for graphics obj 	 */

#include <stlayer.h>
#include <stblit.h>

#ifdef DVD_TRANSPORT_STPTI
#include <stpti.h>
#elif defined DVD_TRANSPORT_PTI
#include <pti.h>
#elif defined DVD_TRANSPORT_LINK
#include <pti.h>
#include <ptilink.h>
#elif defined DVD_TRANSPORT_DEMUX
#include <stdemux.h>
#elif defined DVD_TRANSPORT_STPTI4
#include "stpti.h"
#else
#error "Only PTI, LINK, STPTI or STDEMUX is supported !!"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* --- Exported Constants --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* Driver revision value */
/*This is precisely this long:".............................."*/

#define STSUBT_REVISION       "STSUBT-REL_3.3.0A0            "

/* --- Decoder identity number --- */

#define STSUBT_DRIVER_ID        	61

/* --- Maximum number of STSUBT instances that can be created --- */

#define STSUBT_MAX_NUM_INSTANCES	1

/* --- Number of supported events --- */

#define STSUBT_NUM_EVENTS 		23

/* --- Decoder identity base number --- */

#define STSUBT_DRIVER_BASE      	(STSUBT_DRIVER_ID << 16)

/* --- Define SubT decoder specific errors codes --- */

enum {
    STSUBT_ERROR_INVALID_HANDLE = STSUBT_DRIVER_BASE,
    STSUBT_ERROR_ALREADY_INITIALIZED,
    STSUBT_ERROR_TOO_MANY_INSTANCES,
    STSUBT_ERROR_ALLOCATING_MEMORY,
    STSUBT_ERROR_OPEN_HANDLE,
    STSUBT_ERROR_OPENING_STEVT,
    STSUBT_ERROR_OPENING_STCLKRV,
    STSUBT_ERROR_OPENING_STPTI,
    STSUBT_ERROR_CLOSING_STPTI,
    STSUBT_ERROR_CLOSING_STEVT,
    STSUBT_ERROR_CLOSING_STCLKRV,
    STSUBT_ERROR_EVENT_REGISTER,
    STSUBT_ERROR_EVENT_SUBSCRIBE,
    STSUBT_ERROR_EVENT_UNREGISTER,
    STSUBT_ERROR_SETTING_SLOT,
    STSUBT_ERROR_DRIVER_RUNNING,
    STSUBT_ERROR_DRIVER_NOT_RUNNING,
    STSUBT_ERROR_DRIVER_NOT_SUSPENDED,
    STSUBT_ERROR_DRIVER_NOT_RESUMED,
    STSUBT_ERROR_DRIVER_DISPLAYING,
    STSUBT_ERROR_DRIVER_NOT_DISPLAYING,
    STSUBT_ERROR_INVALID_STANDARD,
    STSUBT_ERROR_NO_STREAM_SET,
    STSUBT_ERROR_CREATING_FILTER,
    STSUBT_ERROR_DELETING_FILTER,
    STSUBT_ERROR_STARTING_FILTER,
    STSUBT_ERROR_STOPPING_FILTER,
    STSUBT_ERROR_CREATING_PROCESSOR,
    STSUBT_ERROR_DELETING_PROCESSOR,
    STSUBT_ERROR_STARTING_PROCESSOR,
    STSUBT_ERROR_STOPPING_PROCESSOR,
    STSUBT_ERROR_INITIALIZING_DISPLAY,
    STSUBT_ERROR_STARTING_ENGINE,
    STSUBT_ERROR_STARTING_TIMER,
    STSUBT_ERROR_STOPPING_ENGINE,
    STSUBT_ERROR_STOPPING_TIMER,
    STSUBT_ERROR_DELETING_ENGINE,
    STSUBT_ERROR_DELETING_TIMER,
    STSUBT_ERROR_UNKNOWN_DISPLAY_SERVICE,
    STSUBT_ERROR_INIT_DISPLAY_SERVICE,
    STSUBT_ERROR_TERM_DISPLAY_SERVICE,

    STSUBT_ERROR_INVALID_WORKING_MODE,
    STSUBT_ERROR_OVERLAY_MODE_NOT_ENABLED,
    STSUBT_ERROR_OBJECT_CACHE_NOT_ENABLED,
    STSUBT_ERROR_INVALID_CALL_MODE,
    STSUBT_ERROR_NO_PACKET_AVAILABLE,
    STSUBT_ERROR_UNKNOWN_APDU,
    STSUBT_ERROR_INVALID_APDU,
    STSUBT_ERROR_UNEXPECTED_APDU,
    STSUBT_ERROR_LOST_APDU_PACKET,
    STSUBT_ERROR_BAD_PARAMETER,
    STSUBT_ERROR_DELETING_EXTPES,  /* new error messages start point */
    STSUBT_ERROR_STOPPING_EXTPES,
    STSUBT_ERROR_STARTING_EXTPES,
   	STSUBT_ERROR_OPENING_STDEMUX,
	STSUBT_ERROR_CLOSING_STDEMUX
#ifdef STSUBT_OSD_SUPPORT
    ,
    STSUBT_ERROR_INVALID_REGION_HANDLE,
    STSUBT_ERROR_INVALID_DST_REGION_HANDLE,
    STSUBT_ERROR_INVALID_SRC_REGION_HANDLE,
    STSUBT_ERROR_MAX_REGION,
    STSUBT_ERROR_NO_AV_SPACE,
    STSUBT_ERROR_OUT_OF_REGION,
    STSUBT_ERROR_TRUE_COLOR_REGION,
    STSUBT_ERROR_EXCEED_ORIGINAL_REGION_MEMORY_SIZE,
    STSUBT_ERROR_DEVICE_NOT_INITIALIZED
#endif
};


/* --- Define decoder specific events --- */

enum {
    STSUBT_EVENT_NO_PES_RECEIVED = STSUBT_DRIVER_BASE,  /* Filter timed out waiting for PES packet */
    STSUBT_EVENT_BAD_PES,          /* Incorrect PES data packet received      */
    STSUBT_EVENT_BAD_SEGMENT,      /* Incorrect segment data received         */
    STSUBT_EVENT_LOST_PES,         /* Error copying of PES from pti buffer    */
    STSUBT_EVENT_PES_NOT_CONSUMED, /* consumer pointer has not been updated   */
    STSUBT_EVENT_PES_AVAILABLE,    /* resumption of valid pes in the PID      */
    STSUBT_EVENT_SEGMENT_AVAILABLE,/* resumption of valid segment in the PID  */
    STSUBT_EVENT_UNKNOWN_REGION,   /* Undefined region referenced             */
    STSUBT_EVENT_UNKNOWN_CLUT,     /* Undefined CLUT referenced               */
    STSUBT_EVENT_UNKNOWN_OBJECT,   /* Undefined object referenced             */
    STSUBT_EVENT_DTG_STANDARD,     /* A DTG standard has been detected        */
    STSUBT_EVENT_DISPLAY_ERROR,    /* Error detected by the display system    */
    STSUBT_EVENT_DECODER_SLOW,     /* decoder is running slow                 */
    STSUBT_EVENT_DEFINITELY_SLOW,  /* decoder is definitely slow              */
    STSUBT_EVENT_INCOMPATIBLE_DEPTH,   /* object depth incompatible in region */
    STSUBT_EVENT_NOT_SUPPORTED_OBJECT_TYPE,/* object type is not supported    */
    STSUBT_EVENT_NOT_SUPPORTED_OBJECT_CODING,/* object coding is not supported*/
    STSUBT_EVENT_COMP_BUFFER_NO_SPACE, /* no space in Composition Buffer      */
    STSUBT_EVENT_PIXEL_BUFFER_NO_SPACE,/* no space in Pixel Buffer            */

	/* RDE specific events */
    STSUBT_EVENT_RDE_BUFFER_IS_SMALL, /* The RDEBufferSize passed by
				         STSUBT_ShowParams_t  is small         */
    STSUBT_EVENT_RDE_TIMEOUT, 	      /* The flag "LoockState" in the
					STSUBT_RDE_Data_t  structure, stilled
			   		in STSUBT_RDE_DATA_VALOED or in
					STSUBT_RDE/_DATA_READING_ states.  */

    STSUBT_EVENT_OBJECT_CACHE_NO_SPACE, /* no space in Object Cache           */
    STSUBT_EVENT_RDE_COMMAND_IS_POSTED /* New command is available from
				        STSUBT_RDE_DisplayService to RDE     */
};

/* --- Define APDU packet tags for the Enhanced Common Interface --- */

#define STSUBT_SUBTITLE_SEGMENT_LAST_TAG        0x009F880E
#define STSUBT_SUBTITLE_SEGMENT_MORE_TAG        0x009F880F
#define STSUBT_DISPLAY_MESSAGE_TAG              0x009F8810
#define STSUBT_SCENE_END_MARK_TAG               0x009F8811
#define STSUBT_SCENE_DONE_MESSAGE_TAG           0x009F8812
#define STSUBT_SCENE_CONTROL_TAG                0x009F8813
#define STSUBT_SUBTITLE_DOWNLOAD_LAST_TAG       0x009F8814
#define STSUBT_SUBTITLE_DOWNLOAD_MORE_TAG       0x009F8815
#define STSUBT_FLUSH_DOWNLOAD_TAG               0x009F8816
#define STSUBT_DOWNLOAD_REPLY_TAG               0x009F8817
#define STSUBT_DECODER_PAUSE_TAG                0x01000001
#define STSUBT_DECODER_RESUME_TAG               0x01000002


/* -------------------------------------------------------------------------- */
/* --- Exported Types ------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


/* --- Handle of the subtitling decoder --- */

typedef U32 STSUBT_Handle_t;

/* --- Init() parameters --- */

typedef struct STSUBT_InitParams_s {
  BOOL      FullScreenDisplayEnabled;   /* TRUE to enable full screen display */
  BOOL      CommonInterfaceEnabled;     /* TRUE to enable the Common Interface*/
  U32       ObjectCacheSize;            /* Num of bytes for the object cache. */
                                        /* Zero to disable caching mechanism. */
  ST_Partition_t    *MemoryPartition;
  ST_Partition_t    *NoCacheMemoryPartition;

#ifdef STSUBT_OSD_SUPPORT
  U32                       MaxRegion;
  STAVMEM_PartitionHandle_t AVMEMPartition;
  ST_DeviceName_t    BlitterName;
  STBLIT_Handle_t    BLIT_Handle;
  STLAYER_Handle_t   LAYER_Handle;
  ST_DeviceName_t    EventHandlerName;
#endif
} STSUBT_InitParams_t;

/* --- Define the recovery Level  --- */

typedef enum STSUBT_RecoveryLevel_e {
  STSUBT_NoRecovery,
  STSUBT_Recovery,
  STSUBT_RecoveryLevel_NUMBER_OF        /* Must be last entry */
} STSUBT_RecoveryLevel_t;

/* --- Enumerates the decoder working modes --- */

typedef enum STSUBT_WorkingMode_e{
  STSUBT_NormalMode,                    /* Usual decoder working mode    */
  STSUBT_OverlayMode                    /* Supports the Common Interface */
} STSUBT_WorkingMode_t;

/* --- Open() parameters --- */

typedef struct STSUBT_OpenParams_s {
  STSUBT_WorkingMode_t WorkingMode;     /* Decoder working mode              */
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  slot_t        pti_slot;               /* assigned pti slot for subtitling  */
#endif
#if defined (DVD_TRANSPORT_PTI) || defined DVD_TRANSPORT_LINK ||  defined (PTI_EMULATION_MODE)
  U8           *pti_buffer;             /* pointer to pti buffer             */
#endif
  U32           pti_buffer_size;        /* size of pti or stpti or demux buffer */
  ST_DeviceName_t stclkrv_name;         /* Name of initialized STCLKRV driver*/
  ST_DeviceName_t stevt_name;           /* Name of initialized STEVT driver  */
  ST_DeviceName_t stpti_name;           /* Name of initialized STPTI/Demux driver  */
} STSUBT_OpenParams_t;

/* RDE types */

typedef enum STSUBT_RDE_CMD_e
{
	STSUBT_RDE_REGION_CREATE=STSUBT_DRIVER_BASE,
	STSUBT_RDE_REGION_HIDE,
	STSUBT_RDE_SET_PALETTE,
	STSUBT_RDE_SET_DEFAULT_PALETTE,
	STSUBT_RDE_REGION_SHOW,
	STSUBT_RDE_REGION_MOVE,
	STSUBT_RDE_REGION_DELETE,
	STSUBT_RDE_REGION_UPDATE,
	STSUBT_RDE_RESET,
	STSUBT_RDE_NOP      /* The last entry */
} STSUBT_RDE_CMD_t;

typedef enum STSUBT_RDE_DataFlag_e
{
	STSUBT_RDE_DATA_VALID, 		/* The Buffer is read for CONSUMER */
	STSUBT_RDE_DATA_READING,	/* The Buffer is locked by CONSUMER */
	STSUBT_RDE_DATA_DIRTY,		/* The Buffer is read for PRODUCER */
	STSUBT_RDE_DATA_WRITING		/* The Buffer is locked by PRODUCER */
} STSUBT_RDE_DataFlag_t;


typedef U32 STSUBT_RDE_RegionId_t;

typedef struct STSUBT_RDE_MoveParams_s {
  U32 PositionX;
  U32 PositionY;
} STSUBT_RDE_MoveParams_t;

typedef struct STSUBT_RDE_PaletteColorParams_s {
  U8 ColorEntry;
  STGXOBJ_ColorUnsignedAYCbCr_t	Color;
} STSUBT_RDE_PaletteColorParams_t;

typedef struct STSUBT_RDE_RegionParams_s {
  U32 PositionX;
  U32 PositionY;
  U32 Width;
  U32 Height;
  U8 NumBitsPixel; 		/* Number of byte for pixel */
} STSUBT_RDE_RegionParams_t;


typedef struct STSUBT_RDE_Param_s {
  STSUBT_RDE_RegionParams_t         RegionParams;
  STSUBT_RDE_MoveParams_t           MoveParams;
  STSUBT_RDE_PaletteColorParams_t   PaletteColor;
  STGXOBJ_Palette_t                 Palette;
  STGXOBJ_Bitmap_t                  Bitmap;
} STSUBT_RDE_Param_t;


typedef struct STSUBT_RDE_Data_s {
  STSUBT_RDE_DataFlag_t LockState;     /* Arbitration flag for this data structure */
  STSUBT_RDE_CMD_t      Command;       /* Command to sent to RDE */
  STSUBT_RDE_RegionId_t RegionId;      /* Handle of RDE region   */
  STSUBT_RDE_Param_t    Params;        /* Union of data types used by some RDE function  */
} STSUBT_RDE_Data_t;


/* --- Supported subtitles stream standards --- */

typedef enum STSUBT_Standard_e {
  STSUBT_StandardNotSpecified,  /* Decoder will try to detect the standard   */
  STSUBT_DVBStandard,   	/* DVB subtitles stream                      */
  STSUBT_DTGStandard    	/* DTG subtitles stream                      */
} STSUBT_Standard_t;

/* --- Subtitle stream definition --- */

typedef struct STSUBT_Stream_s {
#if defined DVD_TRANSPORT_PTI || defined DVD_TRANSPORT_LINK
  pid_t  		subtitle_pid;
#endif
#if defined DVD_TRANSPORT_STPTI || defined DVD_TRANSPORT_STPTI4
  STPTI_Pid_t  	subtitle_pid;
#endif
#ifdef DVD_TRANSPORT_DEMUX
  STDEMUX_Pid_t  subtitle_pid;
#endif
  U16    composition_page_id;
  U16    ancillary_page_id;
} STSUBT_Stream_t;

/* --- Predefined Display Services for common Devices --- */

typedef enum STSUBT_DisplayServiceName_e {
#ifdef USER_DISPLAY_SERVICE
  STSUBT_UserDefined_DisplayService,     /* User Defined Display Service     */
#endif

#ifndef DISABLE_OSD_DISPLAY_SERVICE
  STSUBT_OSD_DisplayService,	         /* Predefined OSD Display Service   */
#endif

#ifdef TEST_DISPLAY_SERVICE
  STSUBT_TEST_DisplayService,            /* Predefined TEST Display Service  */
#endif

#ifdef RDE_DISPLAY_SERVICE
  STSUBT_RDE_DisplayService,             /* Predefined RDE Display Service  */
#endif

  STSUBT_NULL_DisplayService  	         /* Predefined Null Display Service  */

} STSUBT_DisplayServiceName_t;

/* --- Show() parameters --- */

typedef struct STSUBT_ShowParams_s {
  void  *DisplayDeviceHandle;                     /* Handle of display device */
  STSUBT_DisplayServiceName_t DisplayServiceName; /* Display Service name     */
  U8*	RDEBuffer_p;							  /* Pointer of the display service buffer */
  U32   RDEBufferSize;							  /* Size of the Display Service Buffer */
} STSUBT_ShowParams_t;

/* --- Term() parameters --- */

typedef struct STSUBT_TermParams_s {
	BOOL ForceTerminate;
} STSUBT_TermParams_t;

/* --- Calling mode for STSUBT_GetPacket --- */

typedef enum STSUBT_CallingMode_e{
  STSUBT_Blocking,    /* application suspended until an APDU is produced      */
  STSUBT_NonBlocking  /* application immediately returns if APDU not produced */
} STSUBT_CallingMode_t;

/* -------------------------------------------------------------------------- */
/* --- Exported Variables --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* none */

/* -------------------------------------------------------------------------- */
/* --- Exported Macros ------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

/* none */

/* -------------------------------------------------------------------------- */
/* --- Exported Functions --------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/* --- Init/Term subtitling decoder --- */

ST_ErrorCode_t STSUBT_Init (
                      ST_DeviceName_t DeviceName,
                      STSUBT_InitParams_t *InitParams);

ST_ErrorCode_t STSUBT_Term (
                      ST_DeviceName_t DeviceName, STSUBT_TermParams_t *TermParams);

/* --- Open/Close the subtitling decoder --- */

ST_ErrorCode_t STSUBT_Open (
                      ST_DeviceName_t DeviceName,
                      STSUBT_OpenParams_t *OpenParams,
                      STSUBT_Handle_t *SubtHandle);

ST_ErrorCode_t STSUBT_Close (
                      STSUBT_Handle_t SubtHandle);

/* --- Start/Stop the subtitling decoder --- */

ST_ErrorCode_t STSUBT_Start (
                      STSUBT_Handle_t SubtHandle);

ST_ErrorCode_t STSUBT_Stop (
                      STSUBT_Handle_t SubtHandle);

/* --- Set a subtitles stream --- */

ST_ErrorCode_t STSUBT_SetStream (
                      STSUBT_Handle_t SubtHandle,
                      STSUBT_Stream_t SubtStream,
                      STSUBT_Standard_t SubtStandard);

/* --- Set/Get Recovery Level of Pixel (object) --- */

ST_ErrorCode_t STSUBT_SetRecovery (
                      STSUBT_Handle_t SubtHandle,
                      STSUBT_RecoveryLevel_t RecoveryByte);

ST_ErrorCode_t STSUBT_GetRecovery (
                      STSUBT_Handle_t SubtHandle,
		      STSUBT_RecoveryLevel_t *RecoveryByte);


/* --- Enable/Disable display of subtitles --- */

ST_ErrorCode_t STSUBT_Show (
                      STSUBT_Handle_t SubtHandle,
                      STSUBT_ShowParams_t ShowParams);

ST_ErrorCode_t STSUBT_OutStart (
                      STSUBT_Handle_t SubtHandle,
                      STSUBT_ShowParams_t ShowParams);

ST_ErrorCode_t STSUBT_Hide (
                      STSUBT_Handle_t SubtHandle);

ST_ErrorCode_t STSUBT_OutStop (
                      STSUBT_Handle_t SubtHandle);

/* --- Get revision level of the subtitling decoder --- */

ST_Revision_t STSUBT_GetRevision (void);


/* -------------------------------------------------------------------------- */
/* --- Extra functions for the Enhanced Common Interface -------------------- */
/* -------------------------------------------------------------------------- */

/* --- Notify APDU packets to the decoder --- */

ST_ErrorCode_t STSUBT_InsertPacket (
                      STSUBT_Handle_t  SubtHandle,
                      U32              PacketTag,
                      U32              PacketLength,
                      void            *PacketData
               );

/* --- Get APDU reply packets from the decoder --- */

ST_ErrorCode_t STSUBT_GetReplyPacket (
                      STSUBT_Handle_t  SubtHandle,
                      U32             *PacketTag,
                      U32             *PacketLength,
                      U32             *PacketData,
                      STSUBT_CallingMode_t Mode
               );

/* --- Get message APDU packets from the decoder --- */

ST_ErrorCode_t STSUBT_GetMessagePacket (
                      STSUBT_Handle_t  SubtHandle,
                      U32             *PacketTag,
                      U32             *PacketLength,
                      U32             *PacketData
               );

#if defined DVD_TRANSPORT_STPTI || defined DVD_TRANSPORT_STPTI4
ST_ErrorCode_t STSUBT_GetSlotCount (STSUBT_Handle_t SubtHandle,
                                    STPTI_Pid_t     Pid,
                                    U32             *SubSlotCount);
#elif defined DVD_TRANSPORT_DEMUX
ST_ErrorCode_t STSUBT_GetSlotCount (STSUBT_Handle_t SubtHandle,
                                    STDEMUX_Pid_t   Pid,
                                    U32             *SubSlotCount);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __STSUBT_H */

