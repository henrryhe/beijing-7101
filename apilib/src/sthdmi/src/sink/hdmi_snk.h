/*******************************************************************************
File name   : hdmi_snk.h

Description : HDMI driver header file for SINK (receiver)side header functions.

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/

#ifndef __HDMI_SNK_H
#define __HDMI_SNK_H

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif        /* ST_OSLINUX */
#include "sink.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */
typedef enum
{
    STHDMI_RED_X_COLOR =1,
    STHDMI_RED_Y_COLOR =2,
    STHDMI_GREEN_X_COLOR =4,
    STHDMI_GREEN_Y_COLOR =8,
    STHDMI_BLUE_X_COLOR =16,
    STHDMI_BLUE_Y_COLOR =32,
    STHDMI_WHITE_X_COLOR =64,
    STHDMI_WHITE_Y_COLOR =128
}STHDMI_ColorType_t;

typedef enum
{
    STHDMI_ASPECT_RATIO_16_10,
    STHDMI_ASPECT_RATIO_4_3,
    STHDMI_ASPECT_RATIO_5_4,
    STHDMI_ASPECT_RATIO_16_9
}STHDMI_AspectRatio_t;

typedef enum
{
    STHDMI_EDIDTIMING_EXT_TYPE_NONE,
    STHDMI_EDIDTIMING_EXT_TYPE_VER_ONE,
    STHDMI_EDIDTIMING_EXT_TYPE_VER_TWO,
    STHDMI_EDIDTIMING_EXT_TYPE_VER_THREE
}STHDMI_EDIDTimingExtType_t;


/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */



/* Private Macros --------------------------------------------------------- */

/* Exported Macros--------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t sthdmi_FillEDIDTimingDescriptor(U8* Buffer_p, STHDMI_EDIDTimingDescriptor_t* const  TimingDesc_p);
ST_ErrorCode_t sthdmi_FillSinkEDID(sthdmi_Unit_t * Unit_p, STHDMI_EDIDSink_t* const EDIDStruct_p);
ST_ErrorCode_t sthdmi_GetIdManufacterName (U32 Code, char* IdManufacterName_p);
ST_ErrorCode_t sthdmi_FreeEDIDMemory (sthdmi_Unit_t * Unit_p);
void  sthdmi_GetSinkParams ( U32 ValueStored, STHDMI_EDIDSinkParams_t* const SinkParams);
void  sthdmi_FillShortVideoDescriptor (sthdmi_Unit_t * Unit_p,U8* Buffer_p, STHDMI_VideoDataBlock_t * const VideoData_p);
void  sthdmi_FillShortAudioDescriptor (sthdmi_Unit_t * Unit_p, U8* Buffer_p, STHDMI_AudioDataBlock_t* const AudioData_p);
void  sthdmi_FillSpeakerAllocation (sthdmi_Unit_t * Unit_p,U8* Buffer_p, STHDMI_SpeakerAllocation_t* const SpeakerAllocation_p);
void  sthdmi_FillVendorSpecific (sthdmi_Unit_t * Unit_p, U8* Buffer_p, STHDMI_VendorDataBlock_t* const VendorDataBlock_p);


/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* #ifndef __HDMI_SNK_H */


