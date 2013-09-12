/*******************************************************************************
File name   : hdmi_src.h

Description : HDMI driver header file for source side header functions.

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/

#ifndef __HDMI_SRC_H
#define __HDMI_SRC_H

/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include "sttbx.h"
#endif        /* ST_OSLINUX */
#include "source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
typedef struct
{
    U32                    VidIdCode;
    STVTG_TimingMode_t     Mode;
    U32                    FrameRate;
    STGXOBJ_ScanType_t     ScanType;
    STGXOBJ_AspectRatio_t  AspectRatio;
    U32                    PixelRepetition;
}sthdmi_VideoIdentification_t;

typedef struct
{
    U32                    NumberOfFrontChannels;
    U32                    NumberOfLFEChannels;
    U32                    NumberOfRearChannels;
    U32                    ChannelAllocConfig;
}sthdmi_ChannelAllocation_t;


/* Private Macros --------------------------------------------------------- */

/* Exported Macros--------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
sthdmi_VideoIdentification_t*  sthdmi_GetVideoCode (const STVTG_TimingMode_t Mode, const U32 FrameRate,
                                                   const STGXOBJ_ScanType_t ScanType, const STGXOBJ_AspectRatio_t AspectRatio
                                                   );

ST_ErrorCode_t sthdmi_CollectAviInfoFrameVer1( sthdmi_Unit_t * Unit_p, STHDMI_AVIInfoFrame_t* const AVIBuffer_p);
ST_ErrorCode_t sthdmi_CollectAviInfoFrameVer2( sthdmi_Unit_t * Unit_p, STHDMI_AVIInfoFrame_t* const AVIBuffer_p);
ST_ErrorCode_t sthdmi_CollectSPDInfoFrameVer1 (sthdmi_Unit_t * Unit_p, STHDMI_SPDInfoFrame_t* const SPDBuffer_p);
ST_ErrorCode_t sthdmi_CollectMSInfoFrameVer1 (sthdmi_Unit_t * Unit_p, STHDMI_MPEGSourceInfoFrame_t* const  MSBuffer_p);
ST_ErrorCode_t sthdmi_CollectAudioInfoFrameVer1 (sthdmi_Unit_t * Unit_p, STHDMI_AUDIOInfoFrame_t* const AudioBuffer_p);
ST_ErrorCode_t sthdmi_CollectVSInfoFrameVer1(sthdmi_Unit_t * Unit_p, STHDMI_VendorSpecInfoFrame_t* const  VSBuffer_p);

ST_ErrorCode_t sthdmi_FillAviInfoFrame(sthdmi_Unit_t * Unit_p, STHDMI_AVIInfoFrame_t* const AVIInformation_p);
ST_ErrorCode_t sthdmi_FillSPDInfoFrame ( sthdmi_Unit_t * Unit_p, STHDMI_SPDInfoFrame_t* const SPDInformation_p);
ST_ErrorCode_t sthdmi_FillMSInfoFrame (sthdmi_Unit_t * Unit_p, STHDMI_MPEGSourceInfoFrame_t* const MSInformation_p);
ST_ErrorCode_t sthdmi_FillAudioInfoFrame (sthdmi_Unit_t * Unit_p, STHDMI_AUDIOInfoFrame_t* const AudioInformation_p);
ST_ErrorCode_t sthdmi_FillVSInfoFrame (sthdmi_Unit_t * Unit_p, STHDMI_VendorSpecInfoFrame_t* const VSInformation_p);
ST_ErrorCode_t sthdmi_FillACRInfoFrame (sthdmi_Unit_t * Unit_p);

#if defined(STHDMI_CEC)
ST_ErrorCode_t sthdmi_CECFormatMessage (sthdmi_Device_t * Device_p, const STHDMI_CEC_Command_t* CEC_CommandInfo_p, STVOUT_CECMessage_t* const CEC_MessageInfo_p);
ST_ErrorCode_t sthdmi_CECParseMessage  (sthdmi_Device_t * Device_p, const STVOUT_CECMessage_t* CEC_MessageInfo_p, STHDMI_CEC_Command_t* const CEC_CommandInfo_p);
BOOL sthdmi_HandleCECCommand (sthdmi_Device_t * Device_p, STHDMI_CEC_Command_t*   const      CEC_CommandInfo_p);
ST_ErrorCode_t sthdmi_CEC_Send_Command (sthdmi_Device_t * Device_p, STHDMI_CEC_Command_t*   const CEC_Command_p);
#endif /* STHDMI_CEC */
/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* #ifndef __HDMI_SRC_H */


