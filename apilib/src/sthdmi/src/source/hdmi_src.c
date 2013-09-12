/*******************************************************************************
File name   : hdmi_src.c

Description : HDMI driver header file for SOURCE side functions.

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC

*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#include "hdmi_src.h"
#if !defined(ST_OSLINUX)
#include "stsys.h"
#endif


/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

#define SCAN_I                 STGXOBJ_INTERLACED_SCAN
#define SCAN_P                 STGXOBJ_PROGRESSIVE_SCAN
#define RATIO_4TO3             STGXOBJ_ASPECT_RATIO_4TO3
#define RATIO_16TO9            STGXOBJ_ASPECT_RATIO_16TO9

#ifndef SHDMI_MAX_LEGTH_VS_INFOFRAME
#define SHDMI_MAX_LEGTH_VS_INFOFRAME 28
#endif
#define STHDMI_VS_HEADER_LENGTH      7
#define STHDMI_MAX_VIDEO_CODE  33
#define STHDMI_MAX_CHANNEL     31
#define STHDMI_INFOFRAME_SHIFT 0x80

/* Private variables (static) --------------------------------------------- */
static sthdmi_VideoIdentification_t  VideoCodes[]={
  /* {Video Identification Code, Timing Mode, Frame Rate, Scan Type, Aspect Ratio, Pixel Repetition} */
  {1,  STVTG_TIMING_MODE_480P60000_25200,    60000, SCAN_P,  RATIO_4TO3,  0},
  {1,  STVTG_TIMING_MODE_480P59940_25175,    59940, SCAN_P,  RATIO_4TO3,  0},
  {1,  STVTG_TIMING_MODE_480P60000_24570,    60000, SCAN_P,  RATIO_4TO3,  0},
  {2,  STVTG_TIMING_MODE_480P59940_27000,    59940, SCAN_P,  RATIO_4TO3,  0},
  {2,  STVTG_TIMING_MODE_480P60000_27027,    60000, SCAN_P,  RATIO_4TO3,  0},
  {3,  STVTG_TIMING_MODE_480P59940_27000,    59940, SCAN_P,  RATIO_16TO9, 0},
  {3,  STVTG_TIMING_MODE_480P60000_27027,    60000, SCAN_P,  RATIO_16TO9, 0},
  {4,  STVTG_TIMING_MODE_720P60000_74250,    60000, SCAN_P,  RATIO_16TO9, 0},
  {4,  STVTG_TIMING_MODE_720P59940_74176,    59940, SCAN_P,  RATIO_16TO9, 0},
  {5,  STVTG_TIMING_MODE_1080I60000_74250,   60000, SCAN_I,  RATIO_16TO9, 0},
  {5,  STVTG_TIMING_MODE_1080I59940_74176,   59940, SCAN_I,  RATIO_16TO9, 0},
  {6,  STVTG_TIMING_MODE_480I59940_13500,    59940, SCAN_I,  RATIO_4TO3,  1},
  {6,  STVTG_TIMING_MODE_480I60000_13514,    60000, SCAN_I,  RATIO_4TO3,  1},
  {7,  STVTG_TIMING_MODE_480I59940_13500,    59940, SCAN_I,  RATIO_16TO9, 1},
  {7,  STVTG_TIMING_MODE_480I60000_13514,    60000, SCAN_I,  RATIO_16TO9, 1},
  {16, STVTG_TIMING_MODE_1080P60000_148500,  60000, SCAN_P,  RATIO_16TO9, 0},
  {16, STVTG_TIMING_MODE_1080P59940_148352,  59940, SCAN_P,  RATIO_16TO9, 0},
  {17, STVTG_TIMING_MODE_576P50000_27000,    50000, SCAN_P,  RATIO_4TO3,  0},
  {18, STVTG_TIMING_MODE_576P50000_27000,    50000, SCAN_P,  RATIO_16TO9, 0},
  {19, STVTG_TIMING_MODE_720P50000_74250,    50000, SCAN_P,  RATIO_16TO9, 0},
  {20, STVTG_TIMING_MODE_1080I50000_72000,   50000, SCAN_I,  RATIO_16TO9, 0},
  {20, STVTG_TIMING_MODE_1080I50000_74250,   50000, SCAN_I,  RATIO_16TO9, 0},
  {20, STVTG_TIMING_MODE_1080I50000_74250_1, 50000, SCAN_I,  RATIO_16TO9, 0},
  {21, STVTG_TIMING_MODE_576I50000_13500,    50000, SCAN_I,  RATIO_4TO3,  1},
  {22, STVTG_TIMING_MODE_576I50000_13500,    50000, SCAN_I,  RATIO_16TO9, 1},
  {31, STVTG_TIMING_MODE_1080P50000_148500,  50000, SCAN_P,  RATIO_16TO9, 0},
  {32, STVTG_TIMING_MODE_1080P24000_74250,   24000, SCAN_P,  RATIO_16TO9, 0},
  {33, STVTG_TIMING_MODE_1080P25000_74250,   25000, SCAN_P,  RATIO_16TO9, 0},
  {34, STVTG_TIMING_MODE_1080P30000_74250,   30000, SCAN_P,  RATIO_16TO9, 0}
};

/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes--------------------------------------------- */
static U32  sthdmi_CheckSum(U8* InfoFrame_p, U32 Length);
static ST_ErrorCode_t  sthdmi_AspectRatioValidity(const STVTG_TimingMode_t Mode,
                                                  const STGXOBJ_AspectRatio_t AspectRatio);
ST_ErrorCode_t sthdmi_BCDToDecimal ( const  U8  BCD, U8*  const  Decimal_p);
ST_ErrorCode_t sthdmi_DecimalToBCD ( const  U8 Decimal, U8*  const  BCD_p);

#ifdef STHDMI_CEC
ST_ErrorCode_t sthdmi_CECParse_UserControlPressed ( const  STVOUT_CECMessage_t*       CEC_MessageInfo_p,
                                                    STHDMI_CEC_Command_t*   const      CEC_CommandInfo_p);
ST_ErrorCode_t sthdmi_CECParse_DigitalServiceIdentification (   const  STVOUT_CECMessage_t*   CEC_MessageInfo_p,
                                                                const  U8                      ParseOffset,
                                                        STHDMI_CEC_DigitalServiceIdentification_t*   const DigitalServiceIdentification_p);
ST_ErrorCode_t sthdmi_CECParse_RecordSource (   const  STVOUT_CECMessage_t*   CEC_MessageInfo_p,
                                                const  U8                      ParseOffset,
                                                STHDMI_CEC_RecordSource_t*   const RecordSource_p);
ST_ErrorCode_t sthdmi_CECFormat_DigitalServiceIdentification (  const  STHDMI_CEC_DigitalServiceIdentification_t*   DigitalServiceIdentification_p,
                                                                const  U8   FormatOffset,
                                                                STVOUT_CECMessage_t*   const CEC_MessageInfo_p);
#endif

/* Exported Functions------------------------------------------------------ */
/*******************************************************************************
Name        :   sthdmi_CheckSum
Description :   Make the sum of all bytes of the info frame
Parameters  :   Info frame (pointer), length of info frame
Assumptions :
Limitations :
Returns     :  checksum of info frame.
*******************************************************************************/
static U32 sthdmi_CheckSum(U8* InfoFrame_p, U32 Length)
{
    U8* tmp;
    U32 CheckSum, Index;

    CheckSum=0;
    tmp = InfoFrame_p;
    for (Index=0; (Index<Length)&&(tmp!=NULL); Index++)
    {
        CheckSum += (U8)*tmp;
        tmp++;
    }
    CheckSum = (1+ ~CheckSum)&0xFF;
    return(CheckSum);
}

/*******************************************************************************
Name        :   sthdmi_GetVideoCode
Description :   Get the video identification code
Parameters  :   Timing mode (input) and Aspect ratio (input).
Assumptions :
Limitations :
Returns     :   Video identification Code structure(output).
*******************************************************************************/
sthdmi_VideoIdentification_t*  sthdmi_GetVideoCode (const STVTG_TimingMode_t Mode,
                                                    const U32 FrameRate,
                                                    const STGXOBJ_ScanType_t ScanType,
                                                    const STGXOBJ_AspectRatio_t AspectRatio
                                                   )
{
    U32 Index;
    for (Index=0;Index <= STHDMI_MAX_VIDEO_CODE;Index++)
    {
       if ((VideoCodes[Index].Mode == Mode)&&(VideoCodes[Index].FrameRate == FrameRate)&&
            (VideoCodes[Index].ScanType == ScanType)&&(VideoCodes[Index].AspectRatio == AspectRatio))
       {
        break; /* found  */
       }
    }
    return((&VideoCodes[Index]));
} /* sthdmi_GetVideoCode()*/

/*******************************************************************************
Name        :   sthdmi_AspectRatioValidity
Description :   Check the picture aspect ratio validity
Parameters  :   Timing mode (input) and Aspect ratio (input).
Assumptions :
Limitations :
Returns     :   Video identification Code structure(output).
*******************************************************************************/
static ST_ErrorCode_t  sthdmi_AspectRatioValidity(const STVTG_TimingMode_t Mode,
                                                  const STGXOBJ_AspectRatio_t AspectRatio
                                                  )
{

    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    BOOL IsFound = FALSE;
    int Index = 0;

    for (Index=0; Index <= STHDMI_MAX_VIDEO_CODE; Index++)
    {
        if ((VideoCodes[Index].Mode == Mode)&&(VideoCodes[Index].AspectRatio == AspectRatio))
        {
           IsFound =TRUE;
           break;
        }
    }
    (IsFound)? (ErrorCode = ST_NO_ERROR): (ErrorCode = ST_ERROR_BAD_PARAMETER);
    return (ErrorCode);
}/* end of sthdmi_AspectRatioValidity()*/

/*******************************************************************************
Name        :   sthdmi_CollectAviInfoFrameVer1
Description :   Collect Auxiliary Video information compatible with AVI version1
Parameters  :   Handle , AVIBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CollectAviInfoFrameVer1( sthdmi_Unit_t * Unit_p, STHDMI_AVIInfoFrame_t* const AVIBuffer_p)
{
    ST_ErrorCode_t  ErrorCode = ST_NO_ERROR;
    STVTG_TimingMode_t              TimingMode;
    STVTG_ModeParams_t              ModeParams;
    S32 OutputWinX, OutputWinY;
    U32 OutputWinWidth, OutputWinHeight;

    if (Unit_p->Device_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE)
    {
        AVIBuffer_p->AVI861A.FrameType= STHDMI_INFOFRAME_TYPE_AVI;
        AVIBuffer_p->AVI861A.FrameVersion =1;
        AVIBuffer_p->AVI861A.FrameLength =13;
        AVIBuffer_p->AVI861A.OutputType = Unit_p->Device_p->OutputType;

        AVIBuffer_p->AVI861A.HasActiveFormatInformation = TRUE;
        AVIBuffer_p->AVI861A.ScanInfo = STHDMI_SCAN_INFO_OVERSCANNED;

        AVIBuffer_p->AVI861A.Colorimetry = Unit_p->Device_p->Colorimetry;
        AVIBuffer_p->AVI861A.ActiveAspectRatio = Unit_p->Device_p->ActiveAspectRatio;
        AVIBuffer_p->AVI861A.AspectRatio      = Unit_p->Device_p->AspectRatio;


         /* Non Uniform-picture scaling ... should be verified!!!*/
        AVIBuffer_p->AVI861A.PictureScaling = (STHDMI_PictureScaling_t)0;


        OutputWinX = Unit_p->Device_p->OutputWindows.OutputWinX;
        OutputWinY = Unit_p->Device_p->OutputWindows.OutputWinX;
        OutputWinWidth = Unit_p->Device_p->OutputWindows.OutputWinWidth;
        OutputWinHeight = Unit_p->Device_p->OutputWindows.OutputWinHeight;

        /* Set Bar Information */
        ErrorCode = STVTG_GetMode(Unit_p->Device_p->VtgHandle,&TimingMode,&ModeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVTG_GetMode() failed. Error = 0x%x !", ErrorCode));
            return(STHDMI_ERROR_INFOMATION_NOT_AVAILABLE);
        }
        else
        {
           /* Check the aspect ratio validity */
           ErrorCode = sthdmi_AspectRatioValidity(TimingMode, Unit_p->Device_p->AspectRatio);
           if (ErrorCode != ST_NO_ERROR)
           {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_AspectRatioValidity() failed. Error = 0x%x !", ErrorCode));
              return(ST_ERROR_BAD_PARAMETER);
           }
           else
           {

                if (((OutputWinX >0)|| ((OutputWinX + OutputWinWidth)< ModeParams.ActiveAreaWidth))&&
                    ((OutputWinY >0)|| ((OutputWinY + OutputWinHeight)< ModeParams.ActiveAreaHeight)))
                {
                    AVIBuffer_p->AVI861A.BarInfo = STHDMI_BAR_INFO_VH;
                }
                else if ((OutputWinX >0)|| ((OutputWinX + OutputWinWidth)< ModeParams.ActiveAreaWidth))
                {
                    AVIBuffer_p->AVI861A.BarInfo =  STHDMI_BAR_INFO_V;
                }
                else if ((OutputWinY >0)|| ((OutputWinY + OutputWinHeight)< ModeParams.ActiveAreaHeight))
                {
                    AVIBuffer_p->AVI861A.BarInfo = STHDMI_BAR_INFO_H ;
                }
                else
                {
                    AVIBuffer_p->AVI861A.BarInfo = STHDMI_BAR_INFO_NOT_VALID;
                }
           }
        }
        /* Set line and pixel Number of the picture */

        AVIBuffer_p->AVI861A.LineNEndofTopLower      = OutputWinY;
        AVIBuffer_p->AVI861A.LineNEndofTopUpper      = OutputWinY>>8;
        AVIBuffer_p->AVI861A.LineNStartofBotLower    = OutputWinY+OutputWinHeight;
        AVIBuffer_p->AVI861A.LineNStartofBotUpper    = (OutputWinY+OutputWinHeight)>>8;
        AVIBuffer_p->AVI861A.PixelNEndofLeftLower    = OutputWinX;
        AVIBuffer_p->AVI861A.PixelNEndofLeftUpper    = OutputWinX>>8;
        AVIBuffer_p->AVI861A.PixelNStartofRightLower = OutputWinX+OutputWinWidth ;
        AVIBuffer_p->AVI861A.PixelNStartofRightUpper = (OutputWinX+OutputWinWidth)>>8;
    }
    else
    {
        /* Error: Collect information could not be occured for this AVI!*/
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Source AVI: Could not Collect information for this Type of AVI !"));
        ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
    }

    return(ErrorCode);
} /* end of sthdmi_CollectAviInfoFrameVer1()*/

/*******************************************************************************
Name        :   sthdmi_CollectAviInfoFrameVer2
Description :   Collect Auxiliary Video information compatible with AVI version2
Parameters  :   Handle , AVIBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CollectAviInfoFrameVer2( sthdmi_Unit_t * Unit_p, STHDMI_AVIInfoFrame_t* const AVIBuffer_p)
{
    ST_ErrorCode_t                  ErrorCode = ST_NO_ERROR;
    sthdmi_VideoIdentification_t*   VideoIdentity_p;
    STVTG_TimingMode_t              TimingMode;
    STVTG_ModeParams_t              ModeParams;
    S32  OutputWinX, OutputWinY;
    U32  OutputWinWidth, OutputWinHeight;


    if (Unit_p->Device_p->AVIType == STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO)
    {

        AVIBuffer_p->AVI861B.FrameType= STHDMI_INFOFRAME_TYPE_AVI;
        AVIBuffer_p->AVI861B.FrameVersion =2;
        AVIBuffer_p->AVI861B.FrameLength =13;
        AVIBuffer_p->AVI861B.OutputType = Unit_p->Device_p->OutputType;
        AVIBuffer_p->AVI861B.HasActiveFormatInformation = TRUE;        /* should be set by the application*/
        AVIBuffer_p->AVI861B.ScanInfo = STHDMI_SCAN_INFO_OVERSCANNED;  /* Overscanned is for television application */

        AVIBuffer_p->AVI861B.Colorimetry = Unit_p->Device_p->Colorimetry;
        AVIBuffer_p->AVI861B.ActiveAspectRatio = Unit_p->Device_p->ActiveAspectRatio;
        AVIBuffer_p->AVI861B.AspectRatio      = Unit_p->Device_p->AspectRatio;

         /* Non Uniform-picture scaling ... but is set by the application */
        AVIBuffer_p->AVI861B.PictureScaling =(STHDMI_PictureScaling_t)0;


        OutputWinX = Unit_p->Device_p->OutputWindows.OutputWinX;
        OutputWinY = Unit_p->Device_p->OutputWindows.OutputWinX;
        OutputWinWidth = Unit_p->Device_p->OutputWindows.OutputWinWidth;
        OutputWinHeight = Unit_p->Device_p->OutputWindows.OutputWinHeight;

        ErrorCode = STVTG_GetMode(Unit_p->Device_p->VtgHandle,&TimingMode,&ModeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVTG_GetMode() failed. Error = 0x%x !", ErrorCode));
            return(STHDMI_ERROR_INFOMATION_NOT_AVAILABLE);
        }
        else
        {
             /* Check the aspect ratio validity */
            ErrorCode = sthdmi_AspectRatioValidity(TimingMode, Unit_p->Device_p->AspectRatio);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_AspectRatioValidity() failed. Error = 0x%x !", ErrorCode));
                return(ST_ERROR_BAD_PARAMETER);
            }
            else
            {

                VideoIdentity_p = sthdmi_GetVideoCode(TimingMode, \
                                                    ModeParams.FrameRate, \
                                                    ModeParams.ScanType,  \
                                                    AVIBuffer_p->AVI861B.AspectRatio \
                                                    );
                /* Set the video identification Code and the valid pixel repetition for the corresponding video format timing*/
                AVIBuffer_p->AVI861B.VideoFormatIDCode = VideoIdentity_p->VidIdCode;
                AVIBuffer_p->AVI861B.PixelRepetition = VideoIdentity_p->PixelRepetition;

                /* Set Bar information */
                if (((OutputWinX >0)|| ((OutputWinX + OutputWinWidth)< ModeParams.ActiveAreaWidth))&&
                ((OutputWinY >0)|| ((OutputWinY + OutputWinHeight)< ModeParams.ActiveAreaHeight)))
                {
                    AVIBuffer_p->AVI861B.BarInfo = STHDMI_BAR_INFO_VH;
                }
                else if ((OutputWinX >0)|| ((OutputWinX + OutputWinWidth)< ModeParams.ActiveAreaWidth))
                {
                    AVIBuffer_p->AVI861B.BarInfo =  STHDMI_BAR_INFO_V;
                }
                else if ((OutputWinY >0)|| ((OutputWinY + OutputWinHeight)< ModeParams.ActiveAreaHeight))
                {
                    AVIBuffer_p->AVI861B.BarInfo = STHDMI_BAR_INFO_H ;
                }
                else
                {
                    AVIBuffer_p->AVI861B.BarInfo = STHDMI_BAR_INFO_NOT_VALID;
                }
            }
        }

        /* Set Line and Pixel Number */
        AVIBuffer_p->AVI861B.LineNEndofTopLower     = OutputWinY;
        AVIBuffer_p->AVI861B.LineNEndofTopUpper     = OutputWinY>>8;
        AVIBuffer_p->AVI861B.LineNStartofBotLower   = OutputWinY+OutputWinHeight;
        AVIBuffer_p->AVI861B.LineNStartofBotUpper   = (OutputWinY+OutputWinHeight)>>8;
        AVIBuffer_p->AVI861B.PixelNEndofLeftLower   = OutputWinX;
        AVIBuffer_p->AVI861B.PixelNEndofLeftUpper   = OutputWinX>>8;
        AVIBuffer_p->AVI861B.PixelNStartofRightLower = OutputWinX+OutputWinWidth ;
        AVIBuffer_p->AVI861B.PixelNStartofRightUpper = (OutputWinX+OutputWinWidth)>>8;
    }
    else
    {
        ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
    }

    return(ErrorCode);
} /* end sthdmi_CollectAviInfoFrameVer2() */

/*******************************************************************************
Name        :   sthdmi_CollectSPDInfoFrameVer1
Description :   Collect Source Product information compatible with version 1
Parameters  :   Handle , SPDBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CollectSPDInfoFrameVer1 (sthdmi_Unit_t * Unit_p, STHDMI_SPDInfoFrame_t* const SPDBuffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Index;

    if (Unit_p->Device_p->SPDType == STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE)
    {
         /* Set SPD info frame type */
         SPDBuffer_p->FrameType = STHDMI_INFOFRAME_TYPE_SPD;
         /* Set SPD info frame version */
         SPDBuffer_p->FrameVersion = 1;
         /* Set SPD info frame length */
         SPDBuffer_p->FrameLength = 25;

         for (Index=0;Index<8; Index++)
         {
            SPDBuffer_p->VendorName[Index]= Unit_p->Device_p->VendorName[Index];
         }

         for (Index=0;Index<16 ;Index++)
         {
            SPDBuffer_p->ProductDescription[Index]= Unit_p->Device_p->ProductDesc[Index];
         }

         SPDBuffer_p->SourceDeviceInfo = Unit_p->Device_p->SrcDeviceInfo;
    }
    else
    {
        ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
    }
    return(ErrorCode);
} /* end of sthdmi_CollectSPDInfoFrameVer1()*/

/*******************************************************************************
Name        :   sthdmi_CollectMSInfoFrameVer1
Description :   Collect MPEG Source information compatible with version 1
Parameters  :   Handle , MSBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CollectMSInfoFrameVer1 (sthdmi_Unit_t * Unit_p, STHDMI_MPEGSourceInfoFrame_t* const MSBuffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Unit_p->Device_p->MSType == STHDMI_MS_INFOFRAME_FORMAT_VER_ONE)
    {
        MSBuffer_p->FrameType =  STHDMI_INFOFRAME_TYPE_MPEG_SOURCE;
        MSBuffer_p->FrameVersion = 1;
        MSBuffer_p->FrameLength = 10;
        MSBuffer_p->MPEGBitRate = Unit_p->Device_p->MPEGBitRate;
        MSBuffer_p->MepgFrameType = Unit_p->Device_p->MepgFrameType;
        MSBuffer_p->IsFieldRepeated = Unit_p->Device_p->IsFieldRepeated;
    }
    else
    {
        ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
    }
   return(ErrorCode);
} /* end sthdmi_CollectMSInfoFrameVer1()*/
/*******************************************************************************
Name        :   sthdmi_CollectAudioInfoFrameVer1
Description :   Collect Audio information compatible with version 1
Parameters  :   Handle , AudioBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CollectAudioInfoFrameVer1 (sthdmi_Unit_t * Unit_p, STHDMI_AUDIOInfoFrame_t* const AudioBuffer_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (Unit_p->Device_p->AUDIOType == STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE)
    {

        /* Set the audio frame type, version and length */
        AudioBuffer_p->FrameType = STHDMI_INFOFRAME_TYPE_AUDIO;
        AudioBuffer_p->FrameVersion = 1;
        AudioBuffer_p->FrameLength = 10;
        AudioBuffer_p->SampleSize = Unit_p->Device_p->SampleSize;
        AudioBuffer_p->LevelShift = Unit_p->Device_p->LevelShift;
        AudioBuffer_p->DownmixInhibit = Unit_p->Device_p->DownmixInhibit;
        AudioBuffer_p->ChannelCount = Unit_p->Device_p->ChannelCount;
        AudioBuffer_p->CodingType = Unit_p->Device_p->CodingType;
        AudioBuffer_p->SamplingFrequency = Unit_p->Device_p->SamplingFrequency;
        AudioBuffer_p->ChannelAlloc = Unit_p->Device_p->ChannelAlloc;
        AudioBuffer_p->DecoderObject = Unit_p->Device_p->DecoderObject;

    }
    else
    {
        ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
    }
    return(ErrorCode);
} /* end of sthdmi_CollectAudioInfoFrameVer1()*/
/*******************************************************************************
Name        :   sthdmi_CollectVSInfoFrameVer1
Description :   Collect Vendor Specific information compatible with version 1.
Parameters  :   Handle , VSBuffer_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CollectVSInfoFrameVer1(sthdmi_Unit_t * Unit_p, STHDMI_VendorSpecInfoFrame_t* const VSBuffer_p)
{
     ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

     if (Unit_p->Device_p->VSType == STHDMI_VS_INFOFRAME_FORMAT_VER_ONE)
     {
         VSBuffer_p->FrameType = STHDMI_INFOFRAME_TYPE_VENDOR_SPEC;
         VSBuffer_p->FrameVersion=1;
         VSBuffer_p->FrameLength=Unit_p->Device_p->FrameLength ;
         VSBuffer_p->RegistrationId = Unit_p->Device_p->RegistrationId;

         /*Copy of vendor Specific payload*/
         if (VSBuffer_p->VSPayload_p != NULL )
		     memcpy(Unit_p->Device_p->VSPayload, VSBuffer_p->VSPayload_p, STHDMI_VSPAYLOAD_LENGTH);


     }
     else
     {
         ErrorCode = STHDMI_ERROR_NOT_AVAILABLE;
     }
     return(ErrorCode);
} /* end of sthdmi_CollectVSInfoFrameVer1() */
/*******************************************************************************
Name        :   sthdmi_FillAviInfoFrame
Description :   Fill AVI info frames (version1 or version2)tacking into account
*               the display capabilities.
Parameters  :   Handle , AVIInformation_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_FillAviInfoFrame(sthdmi_Unit_t * Unit_p,
                                       STHDMI_AVIInfoFrame_t* const AVIInformation_p)

{
    ST_ErrorCode_t ErrorCode= ST_NO_ERROR;
    U8 AviInfoFrame[17];
    U8 AviInfoFrameByte = 0;
    U32 CheckSum = 0;

    memset(AviInfoFrame, 0, sizeof(AviInfoFrame));
    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break */
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :

            switch (Unit_p->Device_p->AVIType)
            {
                case STHDMI_AVI_INFOFRAME_FORMAT_NONE :
                     break;
                case STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE :
                     /* Collect AVI information related to the AVI info frame version 1 */
                     ErrorCode = sthdmi_CollectAviInfoFrameVer1(Unit_p,AVIInformation_p);

                     if (ErrorCode!=ST_NO_ERROR)
                     {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CollectAviInfoFrameVer1() failed. Error = 0x%x !", ErrorCode));
                        return(ErrorCode);
                     }
                     else
                     {
                        /* Filling Info Frame packet for the AVI  */
                        /* Fill The Info frame type code*/
                        AviInfoFrame[0] = (U8)(AVIInformation_p->AVI861A.FrameType + STHDMI_INFOFRAME_SHIFT);
                        /* Fill The Info Frame Version Number */
                        AviInfoFrame[1] = (U8)AVIInformation_p->AVI861A.FrameVersion;
                        /* Fill The length of AVI info frame */
                        AviInfoFrame[2] = (U8)AVIInformation_p->AVI861A.FrameLength;

                        /* CheckSum will be updated at the end. when the AVIInfoFrame was filled. */
                        AviInfoFrame[3] = CheckSum;


                        /* Fill Data byte 1 of the AVI info frame */
                        /* Scan information bits 0-1*/
                        switch (AVIInformation_p->AVI861A.ScanInfo)
                        {
                            case STHDMI_SCAN_INFO_NO_DATA :
                                AviInfoFrameByte |= (U8)STHDMI_SCAN_INFO_NO_DATA;
                                break;
                            case STHDMI_SCAN_INFO_OVERSCANNED :
                                AviInfoFrameByte |= (U8)STHDMI_SCAN_INFO_OVERSCANNED;
                                break;
                            case STHDMI_SCAN_INFO_UNDERSCANNED :
                                AviInfoFrameByte |= (U8)STHDMI_SCAN_INFO_UNDERSCANNED;
                                break;
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                        }
                        /* Bar Information bits 2-3*/
                        switch (AVIInformation_p->AVI861A.BarInfo)
                        {
                            case STHDMI_BAR_INFO_NOT_VALID :
                                AviInfoFrameByte |= (U8) STHDMI_BAR_INFO_NOT_VALID;
                                break;
                            case STHDMI_BAR_INFO_V :
                                AviInfoFrameByte |= (U8) STHDMI_V_BAR_INFO_VALID;
                                break;
                            case STHDMI_BAR_INFO_H :
                                AviInfoFrameByte |= (U8) STHDMI_H_BAR_INFO_VALID;
                                break;
                            case STHDMI_BAR_INFO_VH :
                                AviInfoFrameByte |= (U8) STHDMI_VH_BAR_INFO_VALID;
                                break;
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                        }
                        /* Active information bit 4*/
                        if (AVIInformation_p->AVI861A.HasActiveFormatInformation)
                        {
                            AviInfoFrameByte |= (U8)STHDMI_ACTIVE_FORMAT_VALID;
                        }
                        else
                        {
                            AviInfoFrameByte &= ~(U8)STHDMI_ACTIVE_FORMAT_VALID;
                        }
                        /* Output Type bits 5-6*/

                        switch (AVIInformation_p->AVI861A.OutputType)
                        {
                            case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                                AviInfoFrameByte |= (U8)STHDMI_SOURCE_RGB888_OUTPUT;
                                break;
                            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                                AviInfoFrameByte |= (U8)STHDMI_SOURCE_YCBCR444_OUTPUT;
                                break;
                            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                                AviInfoFrameByte |= (U8)STHDMI_SOURCE_YCBCR422_OUTPUT;
                                break;
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                        }
                        AviInfoFrame[4]= (U8)(AviInfoFrameByte & STHDMI_AVI_DATABYTE1_MASK);
                        AviInfoFrameByte=0;

                        /* Fill Data byte 2 of the AVI Info Frame version one*/
                        /* Active Format aspect ratio bits 0-3 */

                        /*The active format codes shall be coded in accordance with the Active Format Description9
                          (AFD) in the DVB specification[3] related to CEA 861 rev C*/
                        /*AviInfoFrameByte |= (U8) AVIInformation_p->AVI861A.ActiveAspectRatio;*/

                        switch (AVIInformation_p->AVI861A.ActiveAspectRatio)
                        {
                            case STGXOBJ_ASPECT_RATIO_4TO3 :
                                AviInfoFrameByte |= (U8) STHDMI_FORMAT_ASPECTRATIO_43;
                                break;
                            case STGXOBJ_ASPECT_RATIO_16TO9 :
                                AviInfoFrameByte |= (U8) STHDMI_FORMAT_ASPECTRATIO_169;
                                break;
                            /* case STGXOBJ_ASPECT_RATIO_14TO9: not yet supported in STGXOBJ !!!
                            *             break;                 */
                            /*case  STGXOBJ_ASPECT_RATIO_XX  value for same as picture aspect ratio*/
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                         }

						/* Picture aspect ratio bits 4-5*/
                        switch (AVIInformation_p->AVI861A.AspectRatio)
                        {
                           case STGXOBJ_ASPECT_RATIO_4TO3 :
                                    AviInfoFrameByte |= (U8) STHDMI_PIC_ASPECTRATIO_43;
                                   break;
                                case STGXOBJ_ASPECT_RATIO_16TO9 :
                                    AviInfoFrameByte |= (U8) STHDMI_PIC_ASPECTRATIO_169;
                                    break;
                                default :
                                    AviInfoFrameByte |=  (U8) STHDMI_PIC_ASPECTRATIO_NO_DATA;
                                    break;
                         }


                            /* Colorimetry bits 6-7 of data byte2*/
                            switch (AVIInformation_p->AVI861A.Colorimetry)
                            {
                                case STVOUT_ITU_R_601 :
                                    AviInfoFrameByte |= (U8)STHDMI_COLORIMETRY_ITU601;
                                    break;
                                case STVOUT_ITU_R_709 :
                                    AviInfoFrameByte |= (U8)STHDMI_COLORIMETRY_ITU709;
                                    break;
                                default :
                                    AviInfoFrameByte |= (U8)STHDMI_COLORIMETRY_NO_DATA;
                                    break;
                            }

                            AviInfoFrame[5]= (U8)(AviInfoFrameByte &STHDMI_AVI_DATABYTE2_MASK);
                            AviInfoFrameByte=0;

                            /* Fill data Byte 3: Picture Scaling 0-1bits and 2-7 bits reserved for future use*/
                            switch (AVIInformation_p->AVI861A.PictureScaling)
                            {
                                case STHDMI_PICTURE_NON_UNIFORM_SCALING :
                                    AviInfoFrameByte |= (U8)STHDMI_NO_KNOWN_PIC_SCALING;
                                    break;
                                case STHDMI_PICTURE_SCALING_H :
                                    AviInfoFrameByte |= (U8)STHDMI_H_PIC_SCALING;
                                    break;
                                case STHDMI_PICTURE_SCALING_V :
                                    AviInfoFrameByte |= (U8)STHDMI_V_PIC_SCALING;
                                    break;
                                case STHDMI_PICTURE_SCALING_HV :
                                    AviInfoFrameByte |= (U8)STHDMI_H_V_PIC_SCALING;
                                    break;
                                default :
                                    ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                    break;
                            }
                            AviInfoFrame[6]= (U8)(AviInfoFrameByte&STHDMI_AVI_DATABYTE3_MASK);
                            AviInfoFrameByte=0;

                            /* Data byte 4 and 5 Reserved for Future use  */
                            AviInfoFrame[7] |=STHDMI_RESERVED_FOR_FUTURE_USE;
                            AviInfoFrame[8] |=STHDMI_RESERVED_FOR_FUTURE_USE;

                            /* Fill Data byte 6  */
                            AviInfoFrame[9] = (U8)(AVIInformation_p->AVI861A.LineNEndofTopLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 7  */
                            AviInfoFrame[10] = (U8)(AVIInformation_p->AVI861A.LineNEndofTopUpper&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 8  */
                            AviInfoFrame[11] = (U8)(AVIInformation_p->AVI861A.LineNStartofBotLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 9  */
                            AviInfoFrame[12] = (U8)(AVIInformation_p->AVI861A.LineNStartofBotUpper&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 10  */
                            AviInfoFrame[13] = (U8)(AVIInformation_p->AVI861A.PixelNEndofLeftLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 11  */
                            AviInfoFrame[14] = (U8)(AVIInformation_p->AVI861A.PixelNEndofLeftUpper&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 12  */
                            AviInfoFrame[15] = (U8)(AVIInformation_p->AVI861A.PixelNStartofRightLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 13  */
                            AviInfoFrame[16] = (U8)(AVIInformation_p->AVI861A.PixelNStartofRightUpper&STHDMI_AVI_DATABYTE_MASK);

                     }
                     /* Check Sum of the info frame, Update data byte 3 of the info frame  */
                     AviInfoFrame[3] = sthdmi_CheckSum (AviInfoFrame, AVIInformation_p->AVI861A.FrameLength+4);

                     #if 0
                     STTBX_Print(("The Content of the AVI version1 info frame is:\n"));
                     printf("The Content of the AVI version1 info frame is:\n");
                     for (Index=0; Index<17; Index++)
                     {
                        STTBX_Print(("0x%x\t",AviInfoFrame[Index]));
                        printf("0x%x\t",AviInfoFrame[Index]);
                     }
                     printf("\n");
                     #endif
                     /* Send the info frame... */
                     if (ErrorCode == ST_NO_ERROR)
                     {
                        ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle, STVOUT_INFOFRAME_TYPE_AVI, AviInfoFrame , \
                                               AVIInformation_p->AVI861A.FrameLength+4);

                        if (ErrorCode!= ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));
                            return(ErrorCode);
                        }
                     }
                     break;
                 case STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO :
                    /* Collect AVI information related to the AVI info frame version 2 */
                    ErrorCode = sthdmi_CollectAviInfoFrameVer2(Unit_p,AVIInformation_p);

                     if (ErrorCode!=ST_NO_ERROR)
                     {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CollectAviInfoFrameVer2() failed. Error = 0x%x !", ErrorCode));
                        return(ErrorCode);
                     }
                     else
                     {
                            /* Filling Info Frame packet for the AVI  */
                            /* Fill The Info frame type code*/
                            AviInfoFrame[0] = (U8)(AVIInformation_p->AVI861B.FrameType + STHDMI_INFOFRAME_SHIFT);
                            /* Fill The Info Frame Version Number */
                            AviInfoFrame[1] = (U8)AVIInformation_p->AVI861B.FrameVersion;
                            /* Fill The length of AVI info frame */
                            AviInfoFrame[2] = (U8)AVIInformation_p->AVI861B.FrameLength;

                            /* CheckSum will be updated at the end. when the AVIInfoFrame was filled. */
                            AviInfoFrame[3] = CheckSum;


                        /* Fill Data byte 1 of the AVI info frame */
                        /* Scan information bits 0-1*/
                        switch (AVIInformation_p->AVI861B.ScanInfo)
                        {
                            case STHDMI_SCAN_INFO_NO_DATA :
                                AviInfoFrameByte |= (U8)STHDMI_SCAN_INFO_NO_DATA;
                                break;
                            case STHDMI_SCAN_INFO_OVERSCANNED :
                                AviInfoFrameByte |= (U8)STHDMI_SCAN_INFO_OVERSCANNED;
                                break;
                            case STHDMI_SCAN_INFO_UNDERSCANNED :
                                AviInfoFrameByte |= (U8)STHDMI_SCAN_INFO_UNDERSCANNED;
                                break;
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                        }
                        /* Bar Information bits 2-3*/
                        switch (AVIInformation_p->AVI861B.BarInfo)
                        {
                            case STHDMI_BAR_INFO_NOT_VALID :
                                AviInfoFrameByte |= (U8) STHDMI_BAR_INFO_NOT_VALID;
                                break;
                            case STHDMI_BAR_INFO_V :
                                AviInfoFrameByte |= (U8) STHDMI_V_BAR_INFO_VALID;
                                break;
                            case STHDMI_BAR_INFO_H :
                                AviInfoFrameByte |= (U8) STHDMI_H_BAR_INFO_VALID;
                                break;
                            case STHDMI_BAR_INFO_VH :
                                AviInfoFrameByte |= (U8) STHDMI_VH_BAR_INFO_VALID;
                                break;
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                        }
                        /* Active information bit 4*/
                        if (AVIInformation_p->AVI861B.HasActiveFormatInformation)
                        {
                            AviInfoFrameByte |= (U8)STHDMI_ACTIVE_FORMAT_VALID;
                        }
                        else
                        {
                            AviInfoFrameByte &= ~(U8)STHDMI_ACTIVE_FORMAT_VALID;
                        }
                        /* Output Type bits 5-6*/
                        switch (AVIInformation_p->AVI861B.OutputType)
                        {
                            case STVOUT_OUTPUT_DIGITAL_HDMI_RGB888 :
                                AviInfoFrameByte |= (U8)STHDMI_SOURCE_RGB888_OUTPUT;
                                break;
                            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR444 :
                                AviInfoFrameByte |= (U8)STHDMI_SOURCE_YCBCR444_OUTPUT;
                                break;
                            case STVOUT_OUTPUT_DIGITAL_HDMI_YCBCR422 :
                                AviInfoFrameByte |= (U8)STHDMI_SOURCE_YCBCR422_OUTPUT;
                                break;
                            default :
                                ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                break;
                        }
                        AviInfoFrame[4]= (U8)(AviInfoFrameByte&STHDMI_AVI_DATABYTE1_MASK);
                        AviInfoFrameByte=0;
                        /* Fill Data byte 2 of the AVI Info Frame version one*/
                        /* Active Format aspect ratio bits 0-3 */

                        /*The active format codes shall be coded in accordance with the Active Format Description9
                          (AFD) in the DVB specification[3] related to CEA 861 rev C*/
                        switch (AVIInformation_p->AVI861B.ActiveAspectRatio)
                        {
                            case STGXOBJ_ASPECT_RATIO_4TO3 :
                                AviInfoFrameByte |= (U8) STHDMI_FORMAT_ASPECTRATIO_43;
                                break;
                            case STGXOBJ_ASPECT_RATIO_16TO9 :
                                AviInfoFrameByte |= (U8) STHDMI_FORMAT_ASPECTRATIO_169;
                                break;
                            /* case STGXOBJ_ASPECT_RATIO_14TO9: not yet supported in STGXOBJ !!!
                            *             break;                 */
                            /*case  STGXOBJ_ASPECT_RATIO_XX  value for same as picture aspect ratio*/
                            default :
                                AviInfoFrameByte |= (U8) AVIInformation_p->AVI861B.ActiveAspectRatio;
                                break;
                         }


                        /* Picture aspect ratio bits 4-5*/
                        switch (AVIInformation_p->AVI861B.AspectRatio)
                        {

                            case STGXOBJ_ASPECT_RATIO_4TO3 :
                                 AviInfoFrameByte |= (U8) STHDMI_PIC_ASPECTRATIO_43;
                                 break;
                            case STGXOBJ_ASPECT_RATIO_16TO9 :
                                 AviInfoFrameByte |= (U8) STHDMI_PIC_ASPECTRATIO_169;
                                    break;
                                default :
                                 AviInfoFrameByte |=  (U8) STHDMI_PIC_ASPECTRATIO_NO_DATA;
                                 break;
                         }

                            /* Colorimetry bits 6-7 of data byte2*/
                            switch (AVIInformation_p->AVI861B.Colorimetry)
                            {
                                case STVOUT_ITU_R_601 :
                                    AviInfoFrameByte |= (U8)STHDMI_COLORIMETRY_ITU601;
                                    break;
                                case STVOUT_ITU_R_709 :
                                    AviInfoFrameByte |= (U8)STHDMI_COLORIMETRY_ITU709;
                                    break;
                                default :
                                    AviInfoFrameByte |= (U8)STHDMI_COLORIMETRY_NO_DATA;
                                    break;
                            }
                            AviInfoFrame[5] = (U8)(AviInfoFrameByte&STHDMI_AVI_DATABYTE2_MASK);
                            AviInfoFrameByte=0;

                            /* Fill data Byte 3: Picture Scaling 0-1bits and 2-7 bits reserved for future use*/
                            switch (AVIInformation_p->AVI861B.PictureScaling)
                            {
                                case STHDMI_PICTURE_NON_UNIFORM_SCALING :
                                    AviInfoFrameByte |= (U8)STHDMI_NO_KNOWN_PIC_SCALING;
                                    break;
                                case STHDMI_PICTURE_SCALING_H :
                                    AviInfoFrameByte |= (U8)STHDMI_H_PIC_SCALING;
                                    break;
                                case STHDMI_PICTURE_SCALING_V :
                                    AviInfoFrameByte |= (U8)STHDMI_V_PIC_SCALING;
                                    break;
                                case STHDMI_PICTURE_SCALING_HV :
                                    AviInfoFrameByte |= (U8)STHDMI_H_V_PIC_SCALING;
                                    break;
                                default :
                                    ErrorCode = STHDMI_ERROR_INVALID_PACKET_FORMAT;
                                    break;
                            }
                            AviInfoFrame[6] = (U8)(AviInfoFrameByte&STHDMI_AVI_DATABYTE3_MASK);
                            AviInfoFrameByte=0;

                            /* Video indentification data Code, Data byte 4 */
                            AviInfoFrame[7] =(U8)(AVIInformation_p->AVI861B.VideoFormatIDCode&STHDMI_AVI_VER2_DATABYTE4_MASK);

                            /* Pixel repetition, Data Byte 5 */
                            AviInfoFrame[8]= (U8)(AVIInformation_p->AVI861B.PixelRepetition& STHDMI_AVI_VER2_DATABYTE5_MASK);

                            /* Fill Data byte 6  */
                            AviInfoFrame[9] = (U8)(AVIInformation_p->AVI861B.LineNEndofTopLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 7  */
                            AviInfoFrame[10] = (U8)(AVIInformation_p->AVI861B.LineNEndofTopUpper&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 8  */
                            AviInfoFrame[11] = (U8)(AVIInformation_p->AVI861B.LineNStartofBotLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 9  */
                            AviInfoFrame[12] = (U8)(AVIInformation_p->AVI861B.LineNStartofBotUpper&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 10  */
                            AviInfoFrame[13] = (U8)(AVIInformation_p->AVI861B.PixelNEndofLeftLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 11  */
                            AviInfoFrame[14] = (U8)(AVIInformation_p->AVI861B.PixelNEndofLeftUpper&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 12  */
                            AviInfoFrame[15] = (U8)(AVIInformation_p->AVI861B.PixelNStartofRightLower&STHDMI_AVI_DATABYTE_MASK);

                            /* Fill Data byte 13  */
                            AviInfoFrame[16] = (U8)(AVIInformation_p->AVI861B.PixelNStartofRightUpper&STHDMI_AVI_DATABYTE_MASK);
                     }
                     /* Check Sum of the info frame, Update data byte 3 of the info frame  */
                     AviInfoFrame[3] = sthdmi_CheckSum (AviInfoFrame, AVIInformation_p->AVI861B.FrameLength+4);
                     #if 0
                     STTBX_Print(("The Content of the AVI version2 info frame is:\n"));
                     for (Index=0; Index<17; Index++)
                     {
                        STTBX_Print(("0x%x\t",AviInfoFrame[Index]));

                     }
                     STTBX_Print(("\n"));
                     #endif

                     if (ErrorCode == ST_NO_ERROR)
                     {
                        /* Send the info frame... 17 Bytes taking info account the checksum*/
                        ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle,STVOUT_INFOFRAME_TYPE_AVI,AviInfoFrame , \
                                                AVIInformation_p->AVI861B.FrameLength+4);

                        if (ErrorCode!= ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));
                            return(ErrorCode);
                        }
                     }
                     break;
                default :
                     ErrorCode = ST_ERROR_BAD_PARAMETER;
                     break;
            }
            break;
       default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return(ErrorCode);

} /* end of sthdmi_FillAviInfoFrame()*/

/*******************************************************************************
Name        :   STHDMI_FillSPDInfoFrame
Description :   Fill SPD info frames tacking into account the display capabilities
Parameters  :   Unit_p , SPDInformation_p .
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_FillSPDInfoFrame ( sthdmi_Unit_t * Unit_p, STHDMI_SPDInfoFrame_t* const SPDInformation_p)
{

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8  SPDInfoFrame[29];
    U8  SPDInfoFrameByte =0;
    U8  CheckSum = 0;
    U32 Index;

    memset(SPDInfoFrame, 0, sizeof(SPDInfoFrame));
    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710: /* no break */
        case STHDMI_DEVICE_TYPE_7100: /* no break */
        case STHDMI_DEVICE_TYPE_7200:

             /* Collect Source Product Description information */
             switch (Unit_p->Device_p->SPDType)
             {
                case STHDMI_SPD_INFOFRAME_FORMAT_NONE :
                     break;
                case STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE :
                     ErrorCode = sthdmi_CollectSPDInfoFrameVer1(Unit_p, SPDInformation_p);
                     if (ErrorCode != ST_NO_ERROR)
                     {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CollectSPDInfoFrameVer1() failed. Error = 0x%x !", ErrorCode));
                        return(ErrorCode);
                     }
                     else  /* Fill SPD info Frame ... */
                     {
                          /* Fill the SPD info Frame type Code */
                          SPDInfoFrame[0] = (U8)(SPDInformation_p->FrameType+ STHDMI_INFOFRAME_SHIFT);

                         /* Fill the SPD info frame Version Number */
                          SPDInfoFrame[1] = (U8) SPDInformation_p->FrameVersion;

                         /* Fill the SPD info Frame length  */
                          SPDInfoFrame[2] = (U8) SPDInformation_p->FrameLength;

                          /* Fill the checksum of SPD info Frame */
                          SPDInfoFrame[3] = CheckSum;

                         /* Fill 8 data bytes of vendor Name character(7 bits ASCII code)*/
                         for (Index=0;Index <8; Index++)
                         {
                            if (SPDInformation_p->VendorName[Index])
                            {
                                SPDInfoFrame[Index+4] = (U8)(SPDInformation_p->VendorName[Index]&STHDMI_SPD_INFOFRAME_MASK);
                            }
                            else /* The Vendor Name is Null */
                            {
                                SPDInfoFrame[Index+4] = STHDMI_SPD_INFOFRAME_NULL;
                            }
                         }
                        /* Fill 16 data bytes of Product description character (16 bits ASCII code) */
                         for (Index =0; Index<16; Index++)
                         {
                            if (SPDInformation_p->ProductDescription[Index])
                            {
                                SPDInfoFrame[Index+12] = (U8)(SPDInformation_p->ProductDescription[Index] &STHDMI_SPD_INFOFRAME_MASK);
                            }
                            else
                            {
                                SPDInfoFrame[Index+12] = STHDMI_SPD_INFOFRAME_NULL;
                            }
                         }
                        /* Fill Source Device Information, Data Byte 25 */
                        switch (SPDInformation_p->SourceDeviceInfo)
                        {
                            case STHDMI_SPD_DEVICE_UNKNOWN :
                                SPDInfoFrameByte = (U8) STHDMI_DEVICE_UNKNOWN;
                                break;
                            case STHDMI_SPD_DEVICE_DIGITAL_STB :
                                SPDInfoFrameByte = (U8) STHDMI_DEVICE_DIGITAL_STB;
                                break;
                            case STHDMI_SPD_DEVICE_DVD :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_DVD;
                                break;
                            case STHDMI_SPD_DEVICE_D_VHS :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_D_VHS;
                                break;
                            case STHDMI_SPD_DEVICE_HDD_VIDEO :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_HDD_VIDEO;
                                break;
                            case STHDMI_SPD_DEVICE_DVC :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_DVC;
                                break;
                            case STHDMI_SPD_DEVICE_DSC :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_DSC;
                                break;
                            case STHDMI_SPD_DEVICE_VIDEO_CD :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_VIDEO_CD;
                                break;
                            case STHDMI_SPD_DEVICE_GAME :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_GAME;
                                break;
                            case STHDMI_SPD_DEVICE_PC_GENERAL :
                                SPDInfoFrameByte  = (U8) STHDMI_DEVICE_PC_GENERAL;
                                break;
                            default :
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                                break;
                        }
                        SPDInfoFrame[28] = (U8)SPDInfoFrameByte;
                     }
                     break;
                default :
                     ErrorCode = ST_ERROR_BAD_PARAMETER;
                     break;
             }
             /* Check Sum of the info frame, Update data byte 3 of the info frame  */
             SPDInfoFrame[3] = sthdmi_CheckSum (SPDInfoFrame, SPDInformation_p->FrameLength+4);
             #if 0
             STTBX_Print(("The Content of the SPD version1 info frame is:\n"));
             printf("The Content of the SPD version2 info frame is:\n");
             for (Index=0; Index<29; Index++)
             {
                STTBX_Print(("0x%x\t",SPDInfoFrame[Index]));
                printf("0x%x\t",SPDInfoFrame[Index]);

             }
             printf("\n");
             #endif
             /* Send the SPD info frame ... */
            if (ErrorCode == ST_NO_ERROR)
            {
                ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle,STVOUT_INFOFRAME_TYPE_SPD, SPDInfoFrame, \
                                           SPDInformation_p->FrameLength+4);
                if (ErrorCode!= ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));
                    return(ErrorCode);
                }
            }

            break;
        default :
             ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    return(ErrorCode);
} /*sthdmi_FillSPDInfoFrame() */

/*******************************************************************************
Name        :   sthdmi_FillMSInfoFrame
Description :   Fill MS info frames tacking into account the display capabilities
Parameters  :   Handle , MSInformation_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_FillMSInfoFrame (sthdmi_Unit_t * Unit_p, STHDMI_MPEGSourceInfoFrame_t* const MSInformation_p)

{
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
   U8  MSInfoFrame[14];
   U8  MSInfoFrameByte=0;
   U8  CheckSum =0;
   U32 Index;

   memset(MSInfoFrame, 0, sizeof(MSInfoFrame));
   switch (Unit_p->Device_p->DeviceType)
   {
     case STHDMI_DEVICE_TYPE_7710 : /* no break */
     case STHDMI_DEVICE_TYPE_7100 : /* no break */
     case STHDMI_DEVICE_TYPE_7200 :

          /* Collect the MPEG Source information for the MS info frame version one */
          switch (Unit_p->Device_p->MSType)
          {
            case STHDMI_MS_INFOFRAME_FORMAT_NONE:
                break;
            case STHDMI_MS_INFOFRAME_FORMAT_VER_ONE :
                ErrorCode = sthdmi_CollectMSInfoFrameVer1 (Unit_p, MSInformation_p);
                if (ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CollectMSInfoFrameVer1() failed. Error = 0x%x !", ErrorCode));
                    return(ErrorCode);
                }
                else
                {
                    /* Fill MPEG Source Info Frame Type */
                     MSInfoFrame[0] = (U8)(MSInformation_p->FrameType + STHDMI_INFOFRAME_SHIFT);

                    /* Fill MS Info Frame version */
                     MSInfoFrame[1] = (U8)MSInformation_p->FrameVersion;

                    /* Fill MS Info Frame Length */
                     MSInfoFrame[2] = (U8)MSInformation_p->FrameLength;

                    /* Set the Checksum of the MS info frame... */
                     MSInfoFrame[3] = (U8)CheckSum;

                    /* Fill MPEG bit Rate in Hz*/
                    for (Index=0; Index<4; Index++)
                    {
                         MSInfoFrame[Index+4] = (U8)MSInformation_p->MPEGBitRate;
                         MSInformation_p->MPEGBitRate >>=8;
                    }
                    /* Fill MPEG Frame type */
                    switch (MSInformation_p->MepgFrameType)
                    {
                        case STVID_MPEG_FRAME_I :
                            MSInfoFrameByte |= STHDMI_MPEG_FRAME_I;
                            break;
                        case STVID_MPEG_FRAME_P :
                            MSInfoFrameByte |= STHDMI_MPEG_FRAME_P;
                            break;
                        case STVID_MPEG_FRAME_B :
                            MSInfoFrameByte |= STHDMI_MPEG_FRAME_B;
                            break;
                        default :
                            MSInfoFrameByte |= STHDMI_MPEG_FRAME_UNKNOWN;
                            break;
                    }
                    /* Fill Field Repeat */
                     if (MSInformation_p->IsFieldRepeated)
                     {
                        MSInfoFrameByte |= (U8)STHDMI_MPEG_REP_FIELD;

                     }
                     else
                     {
                        MSInfoFrameByte &=~(U8)STHDMI_MPEG_REP_FIELD;
                     }
                     MSInfoFrame[8]= (U8)(MSInfoFrameByte&STHDMI_MS_INFOFRAME_MASK);

                     /* Data byte 9=>10  shall be 0*/
                     for (Index=0; Index<5; Index++)
                     {
                        MSInfoFrame[Index+9]= STHDMI_MS_INFOFRAME_NULL;

                     }
                     /* Check Sum of the info frame, Update data byte 3 of the info frame  */
                    MSInfoFrame[3] = sthdmi_CheckSum (MSInfoFrame, MSInformation_p->FrameLength+4);
                    #if 0
                    STTBX_Print(("The Content of the MS version1 info frame is:\n"));
                    printf("The Content of the MS version1 info frame is:\n");
                    for (Index=0; Index<14; Index++)
                    {
                        STTBX_Print(("0x%x\t",MSInfoFrame[Index]));
                        printf("0x%x\t",MSInfoFrame[Index]);

                    }
                    printf("\n");
                    #endif
                    /* Send the MS info frame ... */
                    ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle,STVOUT_INFOFRAME_TYPE_MPEG, MSInfoFrame, \
                                               MSInformation_p->FrameLength);
                    if (ErrorCode!= ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));
                        return(ErrorCode);
                    }
                }
                break;
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
          }
          break;
     default :
         ErrorCode = ST_ERROR_BAD_PARAMETER;
         break;
   }

      return(ErrorCode);
} /* end of sthdmi_FillMSInfoFrame()*/

/*******************************************************************************
Name        :   sthdmi_FillAudioInfoFrame
Description :   Fill Audio info frames tacking into account the display capabilities
Parameters  :   Handle , AudioInformation_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  sthdmi_FillAudioInfoFrame (sthdmi_Unit_t * Unit_p,
                                           STHDMI_AUDIOInfoFrame_t* const AudioInformation_p)


{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U8 AudioInfoFrame[14];
    U8 AudioInfoFrameByte=0;
    U8 CheckSum=0;

    memset(AudioInfoFrame, 0, sizeof(AudioInfoFrame));
    switch (Unit_p->Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break*/
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
             #if defined (WA_GNBvd44290) || defined(WA_GNBvd54182)
             /* Send ACR audio info frame first */
             /*This is for the audio WA, WA_GNBvd44290 on 7710 and WA_GNBvd54182 for 7100 and 7109... */
                  sthdmi_FillACRInfoFrame(Unit_p);
             #endif

             /* Collect Audio information for the Audio info frames version one */

             switch (Unit_p->Device_p->AUDIOType)
             {
                case STHDMI_AUDIO_INFOFRAME_FORMAT_NONE:
                    break;
                case STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE :
                    ErrorCode = sthdmi_CollectAudioInfoFrameVer1(Unit_p, AudioInformation_p);
                    if (ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CollectAudioInfoFrameVer1() failed. Error = 0x%x !", ErrorCode));
                        return(ErrorCode);
                    }
                    else
                    {
                        /* Fill Audio Info Frame type */
                        AudioInfoFrame[0] = (U8) (AudioInformation_p->FrameType+STHDMI_INFOFRAME_SHIFT);
                        /* Fill Audio Info Frame version */
                        AudioInfoFrame[1] = (U8) AudioInformation_p->FrameVersion;
                       /* Fill Audio Info Frame Length */
                        AudioInfoFrame[2] = (U8) AudioInformation_p->FrameLength;
                       /* Set the Check Sum of the Info Frame */
                        AudioInfoFrame[3] = CheckSum;

                       /* Fill Audio Channel Count (Data Byte 1) */
                       switch (AudioInformation_p->ChannelCount)
                       {
                        case 2 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_2_CHANNEL;
                            break;
                        case 3 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_3_CHANNEL;
                            break;
                        case 4 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_4_CHANNEL;
                            break;
                        case 5 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_5_CHANNEL;
                            break;
                        case 6 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_6_CHANNEL;
                            break;
                        case 7 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_7_CHANNEL;
                            break;
                        case 8 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_8_CHANNEL;
                            break;
                        default :
                            AudioInfoFrameByte |= STHDMI_AUDIO_NO_REF_CHANNEL;
                            break;
                       }
                       /* Fill Audio Coding Type (Data Byte 1)*/
                       switch (AudioInformation_p->CodingType)
                       {
                        case STAUD_STREAM_CONTENT_AC3 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_CODING_AC3;
                            break;

                        case STAUD_STREAM_CONTENT_DTS :
                            AudioInfoFrameByte |= STHDMI_AUDIO_CODING_DTS;
                            break;

                        case STAUD_STREAM_CONTENT_MPEG1 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_CODING_MPEG1;
                            break;

                        case STAUD_STREAM_CONTENT_MPEG2 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_CODING_MPEG2;
                            break;

                        case STAUD_STREAM_CONTENT_PCM :
                           AudioInfoFrameByte |= STHDMI_AUDIO_CODING_PCM;
                            break;

                        case STAUD_STREAM_CONTENT_MP3 :
                           AudioInfoFrameByte|= STHDMI_AUDIO_CODING_MP3;
                            break;

                        case STAUD_STREAM_CONTENT_MPEG_AAC :
                           AudioInfoFrameByte |= STHDMI_AUDIO_CODING_AAC;
                            break;

                        case STAUD_STREAM_CONTENT_NULL :
                        default :
                            AudioInfoFrameByte |= STHDMI_AUDIO_CODING_NO_REF;
                            break;
                       }
                       AudioInfoFrame[4] = (U8)(AudioInfoFrameByte& STHDMI_AUDIO_DATABYTE1_MASK);
                       AudioInfoFrameByte=0;

                       /* Fill Sample Size (Data Byte 2) bits 0=>1*/
                        switch (AudioInformation_p->SampleSize)
                       {
                        case STAUD_DAC_DATA_PRECISION_16BITS :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SS_16_BIT;
                            break;

                        case STAUD_DAC_DATA_PRECISION_20BITS :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SS_20_BIT;
                            break;

                        case STAUD_DAC_DATA_PRECISION_24BITS :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SS_24_BIT;
                            break;
                        default :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SS_NO_REF;
                            break;
                       }

                       /* Fill Sample Frequency (Data Byte 2)bits 2=>4*/
                       switch (AudioInformation_p->SamplingFrequency)
                       {
                        case 32000 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_32KHZ;
                            break;
                        case 44100 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_44KHZ;
                            break;
                        case 48000 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_48KHZ;
                            break;
                        case 88200 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_88KHZ;
                            break;
                        case 96000 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_96KHZ;
                            break;
                        case 176400 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_176KHZ;
                            break;
                        case 192000 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_192KHZ;
                            break;
                        default :
                            AudioInfoFrameByte |= STHDMI_AUDIO_SF_NO_REF;
                            break;
                       }
                       AudioInfoFrame[5] = (U8)(AudioInfoFrameByte& STHDMI_AUDIO_DATABYTE2_MASK);
                       AudioInfoFrameByte=0;


                       /* Fill the Bit rate coefficient for the compressed audio format (Data byte 3)*/
                       switch (AudioInformation_p->CodingType)
                       {
                        case STAUD_STREAM_CONTENT_AC3 :
                        case STAUD_STREAM_CONTENT_DTS :
                        case STAUD_STREAM_CONTENT_MPEG1 :
                        case STAUD_STREAM_CONTENT_MPEG2 :
                        case STAUD_STREAM_CONTENT_MP3 :
                        case STAUD_STREAM_CONTENT_MPEG_AAC :
                            AudioInfoFrame[6] = (U8)(AudioInformation_p->BitRate&STHDMI_AUDIO_INFOFRAME_MASK);
                            break;
                        case STAUD_STREAM_CONTENT_PCM :
                            AudioInfoFrame[6] = STHDMI_AUDIO_DATABYTE3_NULL;
                            break;
                        case STAUD_STREAM_CONTENT_NULL :
                        default :
                            break;
                       }

                       /* Fill Channel allocation (Data Byte 4) */
                       AudioInfoFrame[7] = (U8)(AudioInformation_p->ChannelAlloc &STHDMI_AUDIO_INFOFRAME_MASK);

                      /* Fill Level Shift (Data Byte 5) */
                      switch (AudioInformation_p->LevelShift)
                      {
                        case 0 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_0_DB;
                            break;

                        case 1 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_1_DB;
                            break;

                        case 2 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_2_DB;
                            break;

                        case 3 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_3_DB;
                            break;

                        case 4 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_4_DB;
                            break;

                        case 5 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_5_DB;
                            break;

                        case 6 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_6_DB;
                            break;

                        case 7 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_7_DB;
                            break;

                        case 8 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_8_DB;
                            break;

                        case 9 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_9_DB;
                            break;

                        case 10 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_10_DB;
                            break;

                        case 11 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_11_DB;
                            break;

                        case 12 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_12_DB;
                            break;

                        case 13 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_13_DB;
                            break;

                        case 14 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_14_DB;
                            break;

                        case 15 :
                            AudioInfoFrameByte |= STHDMI_AUDIO_LEVEL_SHIFT_15_DB;
                            break;

                        default :
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                            break;
                     }
                     /* Fill Down mix inhibit flag */
                     if (AudioInformation_p->DownmixInhibit)
                     {
                        AudioInfoFrameByte |= STHDMI_DOWN_MIX_PROHIBETED;
                     }
                     else
                     {
						AudioInfoFrameByte &= ~STHDMI_DOWN_MIX_PROHIBETED;
                     }

                   }
                   AudioInfoFrame[8] = (U8)(AudioInfoFrameByte&STHDMI_AUDIO_DATABYTE5_MASK);
                   AudioInfoFrame[3] = sthdmi_CheckSum (AudioInfoFrame, AudioInformation_p->FrameLength+4);

                   /* Send the SPD info frame ... */
                   ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle,STVOUT_INFOFRAME_TYPE_AUDIO, AudioInfoFrame, \
                                             (AudioInformation_p->FrameLength+4));
                   if (ErrorCode!= ST_NO_ERROR)
                   {
                       STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));
                       return(ErrorCode);
                   }
                   break;
                default :
                   ErrorCode = ST_ERROR_BAD_PARAMETER;
                   break;
             }

            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return(ErrorCode);
 } /* end of sthdmi_FillAudioInfoFrame() */
/*******************************************************************************
Name        :   sthdmi_FillVSInfoFrame
Description :   Fill VS info frames tacking into account the display capabilities
Parameters  :   Handle , VSInformation_p (pointer).
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_FillVSInfoFrame (sthdmi_Unit_t * Unit_p,
                                       STHDMI_VendorSpecInfoFrame_t* const VSInformation_p)
{


   U8  VSInfoFrame[SHDMI_MAX_LEGTH_VS_INFOFRAME+4];
   U8  CheckSum=0;
   U32 Index;
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

   memset(VSInfoFrame, 0, sizeof(VSInfoFrame));
   switch (Unit_p->Device_p->DeviceType)
   {
     case STHDMI_DEVICE_TYPE_7710 : /* no break*/
     case STHDMI_DEVICE_TYPE_7100 : /* no break */
     case STHDMI_DEVICE_TYPE_7200 :

          switch (Unit_p->Device_p->VSType)
          {
            case STHDMI_VS_INFOFRAME_FORMAT_NONE:
                break;
            case STHDMI_VS_INFOFRAME_FORMAT_VER_ONE :
                 /* Collect the Vendor Specefic information for the VS info frame version one */
                 ErrorCode = sthdmi_CollectVSInfoFrameVer1(Unit_p, VSInformation_p);
                 if (ErrorCode != ST_NO_ERROR)
                 {
                      STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CollectVSInfoframeVer1() failed. Error = 0x%x !", ErrorCode));
                      return(ErrorCode);
                 }
                 else
                 {
                     /* Fill Info Frame type */
                     VSInfoFrame[0] =(U8) (VSInformation_p->FrameType+STHDMI_INFOFRAME_SHIFT);

                     /* Fill Info Frame version */
                     VSInfoFrame[1] =(U8) VSInformation_p->FrameVersion;

                     /* Fill Info Frame length */
                     VSInfoFrame[2] =(U8) VSInformation_p->FrameLength +STHDMI_VS_HEADER_LENGTH ;  /* VendorSpecific length + STHDMI_VS_HEADER_LENGTH */

                     /* Set the Checksum of vendor specific info frame */
                     VSInfoFrame[3] = CheckSum;

                     /* Fill Registration id assigned by IEEE to the organization*/
                     for (Index=0; Index<3;Index++)
                     {
                        VSInfoFrame[Index+4] = (U8) VSInformation_p->RegistrationId;
                        VSInformation_p->RegistrationId >>=8;
                     }

                     /* Fill Vendor Specific payload  */
                    if (VSInformation_p->VSPayload_p != NULL)
                    {
                       memcpy((void*)&VSInfoFrame[VSInformation_p->FrameLength],(void*)VSInformation_p->VSPayload_p,VSInformation_p->FrameLength);
                    }
                    /* Check Sum of the info frame, Update data byte 3 of the info frame  */
                    VSInfoFrame[3] = sthdmi_CheckSum (VSInfoFrame, SHDMI_MAX_LEGTH_VS_INFOFRAME+4);
                    #if 0
                    STTBX_Print(("The Content of the VS version1 info frame is:\n"));
                    printf("The Content of the VS version1 info frame is:\n");
                    for (Index=0; Index< SHDMI_MAX_LEGTH_VS_INFOFRAME+4; Index++)
                    {
                        STTBX_Print(("0x%x\t",VSInfoFrame[Index]));
                        printf("0x%x\t",VSInfoFrame[Index]);

                    }
                    printf("\n");
                    #endif
                    /* Send the MS info frame ... */
                    ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle, STVOUT_INFOFRAME_TYPE_VS,
                            VSInfoFrame , SHDMI_MAX_LEGTH_VS_INFOFRAME+4);
                    if (ErrorCode!= ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));
                        return(ErrorCode);
                    }

                }
                break;
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
          }
          break;
     default :
         ErrorCode = ST_ERROR_BAD_PARAMETER;
         break;
   }
   return(ErrorCode);
} /* end of sthdmi_FillVSInfoFrame */
/*******************************************************************************
Name        :   sthdmi_FillACRInfoFrame
Description :   Fill Audio ACR info frames that will be sent by HDMI IP.
Parameters  :   Handler.
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t  sthdmi_FillACRInfoFrame (sthdmi_Unit_t * Unit_p)
{
    U8 ACRBuffer[20];
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    memset(ACRBuffer, 0, sizeof(ACRBuffer));

    if(strncmp(Unit_p->Device_p->IDManufactureName, "JVC", 3))
    {
        /* Set software generatation of ACR frame => WA activated */
        ACRBuffer[16] = TRUE;
    }
    else
    {
        /* Set hardware generate ACR frame => WA disabled */
        ACRBuffer[16] = FALSE;
    }
    /* Send the ACR info frame ... */
    ErrorCode = STVOUT_SendData(Unit_p->Device_p->VoutHandle, STVOUT_INFOFRAME_TYPE_ACR,
                                ACRBuffer, 14-4);
    if (ErrorCode!= ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendData() failed. Error = 0x%x !", ErrorCode));

    }
    return(ErrorCode);
}

#ifdef STHDMI_CEC
/*******************************************************************************
Name        :   sthdmi_CEC_Send_Command
Description :
Parameters  :   Device_p (pointer) , CEC_CommandInfo_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t sthdmi_CEC_Send_Command(  sthdmi_Device_t * Device_p,
                                         STHDMI_CEC_Command_t*   const CEC_Command_p
                                      )
{
    ST_ErrorCode_t  ErrorCode;
    STVOUT_CECMessage_t      CECMessage;
    U8  index = 0;
/* Format CEC Operand contain to get Binary structure */
            ErrorCode = sthdmi_CECFormatMessage(Device_p, CEC_Command_p, &CECMessage);
            if (ErrorCode != ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "sthdmi_CECFormatMessage() failed. Error = 0x%x !", ErrorCode));
                return(ErrorCode);
            }

/* For test purpose */
           #if defined(ST_OSLINUX)
           printk("The Length of the table is : %x \n\r", CECMessage.DataLength);
           for (index=0; index<CECMessage.DataLength; index++)
           {
                printk("The data is : %x \n\r", CECMessage.Data[index]);
           }
           #else
           /*STTBX_PRINT("CEC Message[%u]:",CECMessage.DataLength);*/
           for (index=0; index<CECMessage.DataLength; index++)
           {
                /*STTBX_PRINT(" %x", CECMessage.Data[index]);*/
           }
           /*STTBX_PRINT("\n\r");*/
           #endif

/* Add The Command-Message in the Buffer */
            STOS_SemaphoreWait(Device_p->CECStruct_Access_Sem_p);
                index=0;
                while( (index < CEC_MESSAGES_BUFFER_SIZE)&&(!Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].IsCaseFree) )
                {
                    index++;
                }
                if(index == CEC_MESSAGES_BUFFER_SIZE)
                {
                    STOS_SemaphoreSignal(Device_p->CECStruct_Access_Sem_p);
                    return(ST_ERROR_NO_MEMORY);
                }
                Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].Message = CECMessage;
                Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].Command = (*CEC_Command_p);
                Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].IsCaseFree = FALSE;
            STOS_SemaphoreSignal(Device_p->CECStruct_Access_Sem_p);

/* Added to buffer, now Send the CEC Message */
           ErrorCode = STVOUT_SendCECMessage(Device_p->VoutHandle, &CECMessage);
           if (ErrorCode != ST_NO_ERROR)
           {
                /* Not Sent due to error, remove it from Buffer */
                Device_p->CEC_Params.CEC_Cmd_Msg_Buffer[index].IsCaseFree = TRUE;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVOUT_SendCECMessage() failed. Error = 0x%x !", ErrorCode));
                return(ErrorCode);
           }
    return(ErrorCode);
}
/**********************************************************************************
Name        :   sthdmi_HandleCECCommand
Description :   Automatically answers received CEC Commands
Parameters  :
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
BOOL sthdmi_HandleCECCommand  (sthdmi_Device_t * Device_p,
                                        STHDMI_CEC_Command_t*   const      CEC_CommandInfo_p
                                       )
{
    STHDMI_CEC_Command_t Command_Response;
    BOOL IsCommandHandled = FALSE;
    switch (Device_p->DeviceType)
    {
        case STHDMI_DEVICE_TYPE_7710 : /* no break*/
        case STHDMI_DEVICE_TYPE_7100 : /* no break */
        case STHDMI_DEVICE_TYPE_7200 :
        switch (CEC_CommandInfo_p->CEC_Opcode)
        {
            case STHDMI_CEC_OPCODE_ABORT_MESSAGE   :
                Command_Response.Source = CEC_CommandInfo_p->Destination;
                Command_Response.Destination = CEC_CommandInfo_p->Source;
                Command_Response.CEC_Opcode = STHDMI_CEC_OPCODE_FEATURE_ABORT;
                Command_Response.CEC_Operand.FeatureAbort.FeatureOpcode = STHDMI_CEC_OPCODE_ABORT_MESSAGE;
                Command_Response.CEC_Operand.FeatureAbort.AbortReason   = STHDMI_CEC_ABORT_REASON_INVALID_OPERAND;
                sthdmi_CEC_Send_Command(Device_p, &Command_Response);
                IsCommandHandled = TRUE;
                break;
            case STHDMI_CEC_OPCODE_GIVE_PHYSICAL_ADDRESS   :
                if ( Device_p->CEC_Params.IsPhysicalAddressValid )
                {
                    Command_Response.Source = CEC_CommandInfo_p->Destination;
                    Command_Response.Destination = 15; /*Broadcast*/
                    Command_Response.CEC_Opcode = STHDMI_CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
                    Command_Response.CEC_Operand.ReportPhysicalAddress.PhysicalAddress = Device_p->CEC_Params.PhysicalAddress;
                    Command_Response.CEC_Operand.ReportPhysicalAddress.DeviceType   = STHDMI_CEC_DEVICE_TYPE_TUNER;
                    sthdmi_CEC_Send_Command(Device_p, &Command_Response);
                }
                IsCommandHandled = TRUE;
                break;
            case STHDMI_CEC_OPCODE_ACTIVE_SOURCE   :
                if(CEC_CommandInfo_p->Source == 15) /* Unregistred */
                {
                    IsCommandHandled = TRUE; /* So that it's forcely ignored when coming from unregistred */
                }
                break;
            case STHDMI_CEC_OPCODE_SET_MENU_LANGUAGE :
                if(CEC_CommandInfo_p->Source != 0) /* TV */
                {
                    IsCommandHandled = TRUE; /* Only TV can set the language */
                }
                break;
            default                            :
                break;
        }
    }/* switch (Device_p->DeviceType) */

    return (IsCommandHandled);
}
/**********************************************************************************
Name        :   sthdmi_BCDToDecimal
Description :   Converts from BCD to decimal format
 *              so assuming no digit can be bigger than 9
Parameters  :   BCD ,Decimal_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_BCDToDecimal ( const  U8  BCD,
                                     U8*  const  Decimal_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(( (BCD & 0x0F) > 9 ) || ( ((BCD>>4) & 0x0F) > 9 ))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        (*Decimal_p) = ((BCD>>4) & 0x0F)*10 + (BCD & 0x0F);
    }
    return (ErrorCode);
}/* end of sthdmi_BCDToDecimal() */

/**********************************************************************************
Name        :   sthdmi_DecimalToBCD
Description :   Converts from decimal to BCD format
Parameters  :   Decimal ,BCD_p (pointer)Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_DecimalToBCD ( const  U8 Decimal,
                                     U8*  const  BCD_p
                                   )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(Decimal > 99)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        (*BCD_p) = ( (Decimal/10)<<4 ) | ( Decimal%10 );
    }
    return (ErrorCode);
}/* end of sthdmi_DecimalToBCD() */

/**********************************************************************************
Name        :   sthdmi_CECParse_UserControlPressed
Description :   Handle just case of Operand [ User Control Pressed ]
 *              with operand type {UIControl}
Parameters  :   CEC_MessageInfo_p (pointer) ,CEC_CommandInfo_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_CECParse_UserControlPressed ( const  STVOUT_CECMessage_t*       CEC_MessageInfo_p,
                                                    STHDMI_CEC_Command_t*   const      CEC_CommandInfo_p
                                                  )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(CEC_MessageInfo_p->DataLength-1 < 1)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        CEC_CommandInfo_p->CEC_Operand.UICommand.Opcode = CEC_MessageInfo_p->Data[1];
        if(CEC_MessageInfo_p->DataLength == 2)  /* Operand, UIC_Operand */
        {
            CEC_CommandInfo_p->CEC_Operand.UICommand.UseAdditionalOperands = FALSE;
        }
        else
        {
            CEC_CommandInfo_p->CEC_Operand.UICommand.UseAdditionalOperands = TRUE;
            if(CEC_MessageInfo_p->DataLength-2 < 1) /* All Operands of Switch Structure have at least 1, so we optimize */
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else
            {
                switch (CEC_CommandInfo_p->CEC_Operand.UICommand.Opcode)
                {
                    case STHDMI_CEC_UICOMMAND_OPCODE_PLAY_FUNCTION               :
                        CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.PlayMode = CEC_MessageInfo_p->Data[2];
                        if(CEC_MessageInfo_p->Data[2] > 0x1B)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                        break;
                    case STHDMI_CEC_UICOMMAND_OPCODE_TUNE_FUNCTION               :
                        if(CEC_MessageInfo_p->DataLength-2 < 4) /* Only here we have 4 bytes */
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                        else
                        {
                            CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.ChannelNumberFormat =  (CEC_MessageInfo_p->Data[2]<<8) & 0xFC00;
                            if(CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.ChannelNumberFormat == STHDMI_CEC_CHANNEL_NUMBER_FORMAT_TWO_PART) /* need to be tested */
                            {
                                CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MajorChannelNumber  = ((((U16)(CEC_MessageInfo_p->Data[2]))<<8) | (U16)(CEC_MessageInfo_p->Data[3]))& 0x3FF;
                                CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MinorChannelNumber  = ((((U16)(CEC_MessageInfo_p->Data[4]))<<8) | (U16)(CEC_MessageInfo_p->Data[5]));
                            }
                            else
                            {
                                CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MinorChannelNumber  = ((((U16)(CEC_MessageInfo_p->Data[5]))<<8) | (U16)(CEC_MessageInfo_p->Data[6]));
                            }
                        }
                        break;
                    case STHDMI_CEC_UICOMMAND_OPCODE_SELECT_MEDIA_FUNCTION        :
                            CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.UIFunctionMedia = CEC_MessageInfo_p->Data[2]; /* 1 -> 255 , 0 increment */
                        break;
                    case STHDMI_CEC_UICOMMAND_OPCODE_SELECT_AV_INPUT_FUNCTION      :
                            CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.UIFunctionSelectAVInput = CEC_MessageInfo_p->Data[2];
                        break;
                    case STHDMI_CEC_UICOMMAND_OPCODE_SELECT_AUDIO_INPUT_FUNCTION   :
                            CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.UIFunctionSelectAudioInput = CEC_MessageInfo_p->Data[2];
                        break;
                    default                             :
                        /* There was a mistake, operands won't be parsed */
                        break;
                }
            }
        }
    }

    return (ErrorCode);
} /* end of sthdmi_CECParse_UserControlPressed() */

/**********************************************************************************
Name        :   sthdmi_CECParse_DigitalServiceIdentification
Description :
Parameters  :   CEC_MessageInfo_p (pointer) ,
 *              ParseOffset, From where starts the parsing on the Data[] array
 *              DigitalServiceIdentification_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_CECParse_DigitalServiceIdentification (   const  STVOUT_CECMessage_t*   CEC_MessageInfo_p,
                                                        const  U8   ParseOffset,
                                                        STHDMI_CEC_DigitalServiceIdentification_t*   const DigitalServiceIdentification_p
                                                    )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(CEC_MessageInfo_p->DataLength < (7+ParseOffset))
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        DigitalServiceIdentification_p->ServiceIdentificationMethod      = CEC_MessageInfo_p->Data[0+ParseOffset] & 0x80;
        DigitalServiceIdentification_p->DigitalBroadcastSystem           = CEC_MessageInfo_p->Data[0+ParseOffset] & 0x7F;
        if(DigitalServiceIdentification_p->ServiceIdentificationMethod == STHDMI_CEC_SERVICE_IDENTIFICATION_METHOD_BY_DIGITAL_IDS)
        {
            switch (DigitalServiceIdentification_p->DigitalBroadcastSystem)
            {
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_GENERIC      :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_BS           :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_CS           :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_T            :
                    DigitalServiceIdentification_p->ServiceIdentification.ARIBData.TransportStreamID = ((((U16)(CEC_MessageInfo_p->Data[1+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[2+ParseOffset]));
                    DigitalServiceIdentification_p->ServiceIdentification.ARIBData.ServiceID         = ((((U16)(CEC_MessageInfo_p->Data[3+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[4+ParseOffset]));
                    DigitalServiceIdentification_p->ServiceIdentification.ARIBData.OriginalNetworkID = ((((U16)(CEC_MessageInfo_p->Data[5+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[6+ParseOffset]));
                    break;
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_GENERIC     :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_CABLE       :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_SATELLITE   :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_TERRESTRIAL :
                    DigitalServiceIdentification_p->ServiceIdentification.ATSCData.TransportStreamID = ((((U16)(CEC_MessageInfo_p->Data[1+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[2+ParseOffset]));
                    DigitalServiceIdentification_p->ServiceIdentification.ATSCData.ProgramNumber     = ((((U16)(CEC_MessageInfo_p->Data[3+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[4+ParseOffset]));
                    break;
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_GENERIC       :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_C            :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_S            :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_S2           :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_T            :
                    DigitalServiceIdentification_p->ServiceIdentification.DVBData.TransportStreamID = ((((U16)(CEC_MessageInfo_p->Data[1+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[2+ParseOffset]));
                    DigitalServiceIdentification_p->ServiceIdentification.DVBData.ServiceID         = ((((U16)(CEC_MessageInfo_p->Data[3+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[4+ParseOffset]));
                    DigitalServiceIdentification_p->ServiceIdentification.DVBData.OriginalNetworkID = ((((U16)(CEC_MessageInfo_p->Data[5+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[6+ParseOffset]));
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER; /* Param was other than 0x00 -> 0x1B */
                    break;
            }
        }
        else     /* Service Identified By Channel */
        {
            DigitalServiceIdentification_p->ServiceIdentification.ChannelData.ChannelNumberFormat =  (CEC_MessageInfo_p->Data[1+ParseOffset])& 0x3F;
            /*if(DigitalServiceIdentification_p->ServiceIdentification.ChannelData.ChannelNumberFormat == Two_PartChannelNumber)
             * we don't test this cause Major should be initialized anyway */
            DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MajorChannelNumber  = ((((U16)(CEC_MessageInfo_p->Data[1+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[2+ParseOffset]))& 0x3FF ;
            DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MinorChannelNumber  = ((((U16)(CEC_MessageInfo_p->Data[3+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[4+ParseOffset]));
        }
    }
    return (ErrorCode);
} /* end of sthdmi_CECParse_DigitalServiceIdentification() */

/**********************************************************************************
Name        :   sthdmi_CECParse_RecordSource
Description :
Parameters  :   CEC_MessageInfo_p (pointer) ,
 *              ParseOffset, From where starts the parsing on the Data[] array
 *              RecordSource_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_CECParse_RecordSource (   const  STVOUT_CECMessage_t*   CEC_MessageInfo_p,
                                                        const  U8   ParseOffset,
                                                        STHDMI_CEC_RecordSource_t*   const RecordSource_p
                                                    )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(CEC_MessageInfo_p->DataLength < 1+ParseOffset)
    {
        ErrorCode = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        switch (CEC_MessageInfo_p->Data[0+ParseOffset]) /* Data[1] contains the RecordSourceType information */
        {
            case STHDMI_CEC_RECORD_SOURCE_TYPE_OWN_SOURCE                      :
                RecordSource_p->OwnSource = CEC_MessageInfo_p->Data[0+ParseOffset]; /* Own Source */
                break;
            case STHDMI_CEC_RECORD_SOURCE_TYPE_DIGITAL_SERVICE :
                if(CEC_MessageInfo_p->DataLength < 8+ParseOffset)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    RecordSource_p->DigitalService.RecordSourceType = CEC_MessageInfo_p->Data[0+ParseOffset]; /* Digital Service */
                    ErrorCode = sthdmi_CECParse_DigitalServiceIdentification(CEC_MessageInfo_p, 1+ParseOffset, &RecordSource_p->DigitalService.DigitalServiceIdentification);
                }
                break;
            case STHDMI_CEC_RECORD_SOURCE_TYPE_ANALOGUE_SERVICE :
                if(CEC_MessageInfo_p->DataLength < 5+ParseOffset)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    RecordSource_p->AnalogueService.RecordSourceType = CEC_MessageInfo_p->Data[0+ParseOffset]; /* Analogue Service */
                    RecordSource_p->AnalogueService.AnalogueBroadcastType = CEC_MessageInfo_p->Data[1+ParseOffset];
                    RecordSource_p->AnalogueService.AnalogueFrequency = ((((U16)(CEC_MessageInfo_p->Data[2+ParseOffset]))<<8) | (U16)(CEC_MessageInfo_p->Data[3+ParseOffset]));
                    RecordSource_p->AnalogueService.BroadcastSystem = CEC_MessageInfo_p->Data[4+ParseOffset];
                    if(    (CEC_MessageInfo_p->Data[1+ParseOffset] > 0x02)
                        || ( (RecordSource_p->AnalogueService.AnalogueFrequency == 0x0000) || (RecordSource_p->AnalogueService.AnalogueFrequency == 0xFFFF) )
                        || ( (RecordSource_p->AnalogueService.BroadcastSystem > 9)  && (RecordSource_p->AnalogueService.BroadcastSystem != 31) )
                    )
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                }
                break;
            case  STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PLUG :
                if(CEC_MessageInfo_p->DataLength < 2+ParseOffset)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    RecordSource_p->ExternalPlug.RecordSourceType = CEC_MessageInfo_p->Data[0+ParseOffset]; /* ExternalPlug */
                    RecordSource_p->ExternalPlug.ExternalPlug = CEC_MessageInfo_p->Data[1+ParseOffset];
                    if(RecordSource_p->ExternalPlug.ExternalPlug == 0)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                }
                break;
            case STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PHYSCIAL_ADDRESS :
                if(CEC_MessageInfo_p->DataLength < 3+ParseOffset)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    RecordSource_p->ExternalPhysicalAddress.RecordSourceType = CEC_MessageInfo_p->Data[0+ParseOffset]; /* ExternalPhysicalAddress */
                    RecordSource_p->ExternalPhysicalAddress.ExternalPhysicalAddress.A = (CEC_MessageInfo_p->Data[1+ParseOffset]>>4) & 0x0F;
                    RecordSource_p->ExternalPhysicalAddress.ExternalPhysicalAddress.B =  CEC_MessageInfo_p->Data[1+ParseOffset]     & 0x0F;
                    RecordSource_p->ExternalPhysicalAddress.ExternalPhysicalAddress.C = (CEC_MessageInfo_p->Data[2+ParseOffset]>>4) & 0x0F;
                    RecordSource_p->ExternalPhysicalAddress.ExternalPhysicalAddress.D =  CEC_MessageInfo_p->Data[2+ParseOffset]     & 0x0F;
                }
                break;
            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER; /* Record Source Type 1 -> 5 */
                break;
        }
    }

    return (ErrorCode);
} /* end of sthdmi_CECParse_RecordSource() */
/**********************************************************************************
Name        :   sthdmi_CECParseMessage
Description :   Parse the Message and constitute the Command
Parameters  :   Handler,CEC_MessageInfo_p (pointer) ,CEC_CommandInfo_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_CECParseMessage  (sthdmi_Device_t * Device_p,
                                        const  STVOUT_CECMessage_t*   CEC_MessageInfo_p,
                                        STHDMI_CEC_Command_t*   const      CEC_CommandInfo_p
                                       )

{
   U32 Index = 0;
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

   switch (Device_p->DeviceType)
   {
     case STHDMI_DEVICE_TYPE_7710 : /* no break*/
     case STHDMI_DEVICE_TYPE_7100 : /* no break */
     case STHDMI_DEVICE_TYPE_7200 :
    CEC_CommandInfo_p->Source = CEC_MessageInfo_p->Source;
    CEC_CommandInfo_p->Destination = CEC_MessageInfo_p->Destination;
    CEC_CommandInfo_p->Retries = CEC_MessageInfo_p->Retries;

        if(CEC_MessageInfo_p->DataLength == 0)
        {
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
        }
        else /*CEC_MessageInfo_p->DataLength != 0*/
        {
           CEC_CommandInfo_p->CEC_Opcode = CEC_MessageInfo_p->Data[0];
           switch (CEC_CommandInfo_p->CEC_Opcode)
            {
                case STHDMI_CEC_OPCODE_IMAGE_VIEW_ON                   :
                case STHDMI_CEC_OPCODE_TEXT_VIEW_ON                    :
                case STHDMI_CEC_OPCODE_REQUEST_ACTIVE_SOURCE           :
                case STHDMI_CEC_OPCODE_STANDBY                         :
                case STHDMI_CEC_OPCODE_RECORD_OFF                      :
                case STHDMI_CEC_OPCODE_RECORD_TV_SCREEN                :
                case STHDMI_CEC_OPCODE_GET_CEC_VERSION                 :
                case STHDMI_CEC_OPCODE_GIVE_PHYSICAL_ADDRESS           :
                case STHDMI_CEC_OPCODE_GET_MENU_LANGUAGE               :
                case STHDMI_CEC_OPCODE_TUNER_STEP_DECREMENT            :
                case STHDMI_CEC_OPCODE_TUNER_STEP_INCREMENT            :
                case STHDMI_CEC_OPCODE_GIVE_DEVICE_VENDOR_ID           :
                case STHDMI_CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP         :
                case STHDMI_CEC_OPCODE_GIVE_OSD_NAME                   :
                case STHDMI_CEC_OPCODE_USER_CONTROL_RELEASED           :
                case STHDMI_CEC_OPCODE_GIVE_DEVICE_POWER_STATUS        :
                case STHDMI_CEC_OPCODE_ABORT_MESSAGE                   :
                case STHDMI_CEC_OPCODE_GIVE_AUDIO_STATUS               :
                case STHDMI_CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS   :
                    /* These Opcodes do not have any parameters */
                    break;
                case STHDMI_CEC_OPCODE_ACTIVE_SOURCE                   :
                case STHDMI_CEC_OPCODE_INACTIVE_SOURCE                 :
                case STHDMI_CEC_OPCODE_ROUTING_INFORMATION             :
                case STHDMI_CEC_OPCODE_SET_STREAM_PATH                 :
                case STHDMI_CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST       :
                    if(CEC_MessageInfo_p->DataLength-1 < 2)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.A = (CEC_MessageInfo_p->Data[1]>>4) & 0x0F;
                        CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.B =  CEC_MessageInfo_p->Data[1] & 0x0F;
                        CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.C = (CEC_MessageInfo_p->Data[2]>>4) & 0x0F;
                        CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.D =  CEC_MessageInfo_p->Data[2] & 0x0F;
                    }
                    break;

                case STHDMI_CEC_OPCODE_ROUTING_CHANGE                 :
                    if(CEC_MessageInfo_p->DataLength-1 < 4)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.A = (CEC_MessageInfo_p->Data[1]>>4) & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.B =  CEC_MessageInfo_p->Data[1] & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.C = (CEC_MessageInfo_p->Data[2]>>4) & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.D =  CEC_MessageInfo_p->Data[2] & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.A      = (CEC_MessageInfo_p->Data[3]>>4) & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.B      =  CEC_MessageInfo_p->Data[3] & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.C      = (CEC_MessageInfo_p->Data[4]>>4) & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.D      =  CEC_MessageInfo_p->Data[4] & 0xF;
                    }
                   break;

                case STHDMI_CEC_OPCODE_REPORT_PHYSICAL_ADDRESS         :
                    if(CEC_MessageInfo_p->DataLength-1 < 3)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.A = (CEC_MessageInfo_p->Data[1]>>4) & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.B =  CEC_MessageInfo_p->Data[1] & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.C = (CEC_MessageInfo_p->Data[2]>>4) & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.D =  CEC_MessageInfo_p->Data[2] & 0xF;
                        CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.DeviceType        = CEC_MessageInfo_p->Data[3];
                        if(CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.DeviceType > 5)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_CEC_VERSION                    :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.CECVersion        = CEC_MessageInfo_p->Data[1];
                        if(CEC_CommandInfo_p->CEC_Operand.CECVersion > 0x04)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_REPORT_AUDIO_STATUS             :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.AudioStatus.AudioMuteStatus     = CEC_MessageInfo_p->Data[1] & 0x80;
                        /* when using should first test if Var.UnknownAudioVolumeStatus == UnknownAudioVolumeStatus */
                        if((CEC_MessageInfo_p->Data[1]&0x7F) == 0x7F)
                        {
                            CEC_CommandInfo_p->CEC_Operand.AudioStatus.AudioVolumeStatus.UnknownAudioVolumeStatus   =  0x7F; /* UnknownAudioVolumeStatus */
                        }
                        else
                        {
                            CEC_CommandInfo_p->CEC_Operand.AudioStatus.AudioVolumeStatus.KnownAudioVolumeStatus   =  CEC_MessageInfo_p->Data[1]&0x7F;
                        }
                        if( (CEC_MessageInfo_p->Data[1] > 0x64) && (CEC_MessageInfo_p->Data[1] != 0x7F)) /* used Data for not using both unions at same time, though possible*/
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_SET_SYSTEM_AUDIO_MODE            :
                case STHDMI_CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS         :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.SystemAudioStatus = CEC_MessageInfo_p->Data[1];
                        if(CEC_MessageInfo_p->Data[1] > 1)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_SET_AUDIO_RATE                  :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.AudioRate         = CEC_MessageInfo_p->Data[1];
                        if(CEC_MessageInfo_p->Data[1] > 6)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_REPORT_POWER_STATUS             :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.PowerStatus       = CEC_MessageInfo_p->Data[1];
                        if(CEC_MessageInfo_p->Data[1] > 0x03)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_FEATURE_ABORT                  :
                    if(CEC_MessageInfo_p->DataLength-1 < 2)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.FeatureAbort.FeatureOpcode     = CEC_MessageInfo_p->Data[1]; /* Should be testing with all opcodes */
                        CEC_CommandInfo_p->CEC_Operand.FeatureAbort.AbortReason       = CEC_MessageInfo_p->Data[2];
                        if(CEC_MessageInfo_p->Data[2] > 4)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_USER_CONTROL_PRESSED            :
                    ErrorCode = sthdmi_CECParse_UserControlPressed(CEC_MessageInfo_p, CEC_CommandInfo_p);
                    break;

                case STHDMI_CEC_OPCODE_MENU_STATUS                    :
                    if(CEC_MessageInfo_p->DataLength-1 < 2)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.MenuState         = CEC_MessageInfo_p->Data[1];
                        if(CEC_MessageInfo_p->Data[1] > 1)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_MENU_REQUEST                   :
                    if(CEC_MessageInfo_p->DataLength-1 < 2)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.MenuRequestType   = CEC_MessageInfo_p->Data[1];
                        if(CEC_MessageInfo_p->Data[1] > 2)
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;
                case STHDMI_CEC_OPCODE_SELECT_DIGITAL_SERVICE          :
                    ErrorCode = sthdmi_CECParse_DigitalServiceIdentification(CEC_MessageInfo_p, 1, &CEC_CommandInfo_p->CEC_Operand.DigitalServiceIdentification);
                    break;
                case STHDMI_CEC_OPCODE_CLEAR_ANALOGUE_TIMER            :
                case STHDMI_CEC_OPCODE_SET_ANALOGUE_TIMER              :
                    /*ErrorCode = sthdmi_CECParse_TimerParameters(CEC_MessageInfo_p, 1, &CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer); Have to create types wrapper */
                    if(CEC_MessageInfo_p->DataLength-1 < 11)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        /* ---- Common Part ---- */
                        CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.DayOfMonth                          = CEC_MessageInfo_p->Data[1];
                        CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.MonthOfYear                         = CEC_MessageInfo_p->Data[2];
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[3], &CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.StartTime.Hour); /* Ored Just because they return the same kind of error */
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[4], &CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.StartTime.Minute);
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[5], &CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.Duration.DurationHour); /* no test max is already 99 */
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[6], &CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.Duration.Minute);
                        CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.RecordingSequence                   = CEC_MessageInfo_p->Data[7] & 0x7F;
                        /* ---- Specific Part ---- */
                        CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueBroadcastType               = CEC_MessageInfo_p->Data[8];
                        CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueFrequency                   = ((((U16)(CEC_MessageInfo_p->Data[9]))<<8) | (U16)(CEC_MessageInfo_p->Data[10]));
                        CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.BroadcastSystem                     = CEC_MessageInfo_p->Data[11];
                        /* ---- Error Testing ---- */
                        if(  ( (CEC_MessageInfo_p->Data[1] == 0) || (CEC_MessageInfo_p->Data[1] > 31) ) || ( (CEC_MessageInfo_p->Data[2] == 0) || (CEC_MessageInfo_p->Data[2] > 12) )
                          || ( CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.StartTime.Hour > 23 )    || ( CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.StartTime.Minute > 59)
                          || ( CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.Duration.Minute > 59)
                          || (CEC_MessageInfo_p->Data[8] > 0x02)
                          || ( (CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueFrequency == 0x0000) || (CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueFrequency == 0xFFFF) )
                          || ( (CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.BroadcastSystem > 9)  && (CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.BroadcastSystem != 31) )
                          )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;
                case STHDMI_CEC_OPCODE_CLEAR_DIGITAL_TIMER             :
                case STHDMI_CEC_OPCODE_SET_DIGITAL_TIMER               :
                    if(CEC_MessageInfo_p->DataLength-1 < 14)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        /* ---- Common Part ---- */
                        CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.DayOfMonth                          = CEC_MessageInfo_p->Data[1];
                        CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.MonthOfYear                         = CEC_MessageInfo_p->Data[2];
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[3], &CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.StartTime.Hour); /* Ored Just because they return the same kind of error */
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[4], &CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.StartTime.Minute);
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[5], &CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.Duration.DurationHour); /* no test max is already 99 */
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[6], &CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.Duration.Minute);
                        CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.RecordingSequence                   = CEC_MessageInfo_p->Data[7] & 0x7F;
                        /* ---- Specific Part ---- */
                        ErrorCode |= sthdmi_CECParse_DigitalServiceIdentification(CEC_MessageInfo_p, 8, &CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.DigitalServiceIdentification);
                        /* ---- Error Testing ---- */
                        if(  ( (CEC_MessageInfo_p->Data[1] == 0) || (CEC_MessageInfo_p->Data[1] > 31) ) || ( (CEC_MessageInfo_p->Data[2] == 0) || (CEC_MessageInfo_p->Data[2] > 12) )
                          || ( CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.StartTime.Hour > 23 )    || ( CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.StartTime.Minute > 59)
                          || ( CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.Duration.Minute > 59)
                          )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;
                case STHDMI_CEC_OPCODE_CLEAR_EXTERNAL_TIMER            :
                case STHDMI_CEC_OPCODE_SET_EXTERNAL_TIMER              :
                    if(CEC_MessageInfo_p->DataLength-1 < 9)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        /* ---- Common Part ---- */
                        CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.DayOfMonth                          = CEC_MessageInfo_p->Data[1];
                        CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.MonthOfYear                         = CEC_MessageInfo_p->Data[2];
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[3], &CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.StartTime.Hour); /* Ored Just because they return the same kind of error */
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[4], &CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.StartTime.Minute);
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[5], &CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.Duration.DurationHour); /* no test max is already 99 */
                        ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[6], &CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.Duration.Minute);
                        CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.RecordingSequence                   = CEC_MessageInfo_p->Data[7] & 0x7F;
                        /* ---- Specific Part ---- */
                        CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalSourceSpecifier             = CEC_MessageInfo_p->Data[8];
                        if(CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalSourceSpecifier == STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PLUG)
                        {
                            CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPlug = CEC_MessageInfo_p->Data[9];
                            if(CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPlug == 0)
                            {
                                ErrorCode |= ST_ERROR_BAD_PARAMETER;
                            }
                        }
                        else        /* External Source Specifier is External Physical Address */
                        {
                            if(CEC_MessageInfo_p->DataLength-1 < 10)
                            {
                                ErrorCode = ST_ERROR_BAD_PARAMETER; /* If condition is like |= */
                            }
                            else
                            {
                                CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.A = (CEC_MessageInfo_p->Data[9]>>4) & 0xF;
                                CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.B =  CEC_MessageInfo_p->Data[9] & 0xF;
                                CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.C = (CEC_MessageInfo_p->Data[10]>>4) & 0xF;
                                CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.D =  CEC_MessageInfo_p->Data[10] & 0xF;
                            }
                        }

                        /* ---- Error Testing ---- */
                        if(  ( (CEC_MessageInfo_p->Data[1] == 0) || (CEC_MessageInfo_p->Data[1] > 31) ) || ( (CEC_MessageInfo_p->Data[2] == 0) || (CEC_MessageInfo_p->Data[2] > 12) )
                          || ( CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.StartTime.Hour > 23 )    || ( CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.StartTime.Minute > 59)
                          || ( CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.Duration.Minute > 59)
                          || ( (CEC_MessageInfo_p->Data[8]!= 4)  || (CEC_MessageInfo_p->Data[8]!= 5) )
                          )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_SET_TIMER_PROGRAM_TITLE          :
                    for (Index = 1; Index < CEC_MessageInfo_p->DataLength; Index++)
                    {
                        CEC_CommandInfo_p->CEC_Operand.ProgramTitleString[Index-1] = CEC_MessageInfo_p->Data[Index];
                    }
                    if(CEC_MessageInfo_p->DataLength < 15) /* If DataLength == 14, TextLEngth == 13 */
                    {
                        CEC_CommandInfo_p->CEC_Operand.ProgramTitleString[CEC_MessageInfo_p->DataLength-1] = 0; /* NULL Terinated String */
                    }
                    break;

                case STHDMI_CEC_OPCODE_SET_OSD_NAME                    :
                    for (Index = 1; Index < CEC_MessageInfo_p->DataLength; Index++)
                    {
                        CEC_CommandInfo_p->CEC_Operand.OSDName[Index-1] = CEC_MessageInfo_p->Data[Index];
                    }
                    if(CEC_MessageInfo_p->DataLength < 15) /* If DataLength == 14, TextLength == 13 */
                    {
                        CEC_CommandInfo_p->CEC_Operand.OSDName[CEC_MessageInfo_p->DataLength-1] = 0; /* NULL Terinated String */
                    }
                    break;

                case STHDMI_CEC_OPCODE_SET_OSD_STRING                  :
                    if(CEC_MessageInfo_p->DataLength-1 < 2)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.SetOSDString.DisplayControl = CEC_MessageInfo_p->Data[1] & 0xC0;
                        for (Index = 2; Index <= CEC_MessageInfo_p->DataLength ; Index++)
                        {
                            CEC_CommandInfo_p->CEC_Operand.SetOSDString.OSDString[Index-2] = CEC_MessageInfo_p->Data[Index];
                        }
                        if(CEC_MessageInfo_p->DataLength < 15) /* If DataLength == 14, TextLength == 12 */
                        {
                            CEC_CommandInfo_p->CEC_Operand.OSDName[CEC_MessageInfo_p->DataLength-2] = 0; /* NULL Terinated String */
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_DEVICE_VENDOR_ID                :
                    if(CEC_MessageInfo_p->DataLength-1 < 3)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.VendorID = (     (( (  (U32)CEC_MessageInfo_p->Data[1]  ) <<16)& 0xFF0000)   |
                                                                        (( (  (U32)CEC_MessageInfo_p->Data[2]  ) <<8)& 0xFF00)      |
                                                                        (  (  (U32)CEC_MessageInfo_p->Data[3]  ) & 0xFF)            );
                    }
                    break;

                case STHDMI_CEC_OPCODE_VENDOR_COMMAND                 :
                    for (Index = 1; Index < CEC_MessageInfo_p->DataLength; Index++)
                    {
                        CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.Data[Index-1] = CEC_MessageInfo_p->Data[Index];
                    }
                    CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength = CEC_MessageInfo_p->DataLength - 1;
                    break;

                case STHDMI_CEC_OPCODE_VENDOR_COMMAND_WITH_ID           :
                    if(CEC_MessageInfo_p->DataLength-1 < 3)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.VendorCommandWithID.VendorID = (     (( (  (U32)CEC_MessageInfo_p->Data[1]  ) <<16)& 0xFF0000)   |
                                                                                            (( (  (U32)CEC_MessageInfo_p->Data[2]  ) <<8)& 0xFF00)      |
                                                                                            (  (  (U32)CEC_MessageInfo_p->Data[3]  ) & 0xFF)            );

                        for (Index = 4; Index <= CEC_MessageInfo_p->DataLength ; Index++)
                        {
                            CEC_CommandInfo_p->CEC_Operand.VendorCommandWithID.VendorSpecificData.Data[Index-4] = CEC_MessageInfo_p->Data[Index];
                        }
                        if(CEC_MessageInfo_p->DataLength > 4)
                        {
                            CEC_CommandInfo_p->CEC_Operand.VendorCommandWithID.VendorSpecificData.DataLength = CEC_MessageInfo_p->DataLength - 5;
                        }
                        else
                        {
                            CEC_CommandInfo_p->CEC_Operand.VendorCommandWithID.VendorSpecificData.DataLength = 0;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN        :
                    for (Index = 1; Index < CEC_MessageInfo_p->DataLength; Index++)
                    {
                        CEC_CommandInfo_p->CEC_Operand.VendorSpecificRCCode.Data[Index-1] = CEC_MessageInfo_p->Data[Index];
                    }
                    CEC_CommandInfo_p->CEC_Operand.VendorSpecificRCCode.DataLength = CEC_MessageInfo_p->DataLength - 1;
                    break;

                case STHDMI_CEC_OPCODE_TIMER_CLEARED_STATUS            :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.TimerClearedStatusData = CEC_MessageInfo_p->Data[1];
                        if( (CEC_MessageInfo_p->Data[1] > 0x02) && (CEC_MessageInfo_p->Data[1] != 0x80) )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;
                case STHDMI_CEC_OPCODE_TIMER_STATUS                   :
                    if(CEC_MessageInfo_p->DataLength-1 < 1) /* 1 or 3 */
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerOverlapWarning                                   =  CEC_MessageInfo_p->Data[1]  & 0x7F;
                        CEC_CommandInfo_p->CEC_Operand.TimerStatusData.MediaInfo                                             =  CEC_MessageInfo_p->Data[1]  & 0x30;
                        CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedIndicator               =  CEC_MessageInfo_p->Data[1]  & 0x10;
                        if(CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedIndicator == STHDMI_CEC_PROGRAMMED_INDICATOR_PROGRAMMED)
                        {
                             CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.ProgrammedInfo = CEC_MessageInfo_p->Data[1] & 0xF;
                        }
                        else
                        {
                             CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.NotProgrammedErrorInfo = CEC_MessageInfo_p->Data[1] & 0xF;
                        }
                        if(  (CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.ProgrammedInfo == STHDMI_CEC_PROGRAMMED_INFO_NOT_ENOUGH_SPACE_AVAILABLE_FOR_RECORDING) ||
                             (CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.ProgrammedInfo == STHDMI_CEC_PROGRAMMED_INFO_MAY_NOT_BE_ENOUGH_SPACE_AVAILABLE) ||
                             (CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.NotProgrammedErrorInfo == STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_DUPLICATE) )
                        {
                            if(CEC_MessageInfo_p->DataLength-1 >= 3) /* DurationAvailable is an optional parameter */
                            {
                                CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.IsDurationAvailable = TRUE;
                                ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[2], &CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.Duration.DurationHour);
                                ErrorCode |= sthdmi_BCDToDecimal(CEC_MessageInfo_p->Data[3], &CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.Duration.Minute);
                            }
                            else
                            {
                                CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.IsDurationAvailable = FALSE;
                            }
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_RECORD_ON                      :
                    ErrorCode = sthdmi_CECParse_RecordSource(CEC_MessageInfo_p, 1, &CEC_CommandInfo_p->CEC_Operand.RecordSource);
                    break;

                case STHDMI_CEC_OPCODE_RECORD_STATUS                  :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.RecordStatusInfo = CEC_MessageInfo_p->Data[1];
                        if(      (CEC_MessageInfo_p->Data[1] == 0x00)   ||   (CEC_MessageInfo_p->Data[1] == 0x08)
                              || (CEC_MessageInfo_p->Data[1] == 0x0F)
                              || ( (CEC_MessageInfo_p->Data[1] > 0x17) && (CEC_MessageInfo_p->Data[1] < 0x1A) )
                              || ( (CEC_MessageInfo_p->Data[1] > 0x1B) && (CEC_MessageInfo_p->Data[1] < 0x1F) )
                              || (CEC_MessageInfo_p->Data[1] > 0x1F)
                          )
                          {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                          }
                    }
                    break;

                case STHDMI_CEC_OPCODE_SET_MENU_LANGUAGE               :
                    if(CEC_MessageInfo_p->DataLength-1 < 3)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.Language[0]  =   CEC_MessageInfo_p->Data[1];
                        CEC_CommandInfo_p->CEC_Operand.Language[1]  =   CEC_MessageInfo_p->Data[2];
                        CEC_CommandInfo_p->CEC_Operand.Language[2]  =   CEC_MessageInfo_p->Data[3];
                    }
                    break;

                case STHDMI_CEC_OPCODE_DECK_CONTROL                   :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.DeckControlMode = CEC_MessageInfo_p->Data[1];
                        if(  (CEC_MessageInfo_p->Data[1] == 0) || (CEC_MessageInfo_p->Data[1] > 4) )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_DECK_STATUS                    :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.DeckControlMode = CEC_MessageInfo_p->Data[1];
                        if(  (CEC_MessageInfo_p->Data[1] < 0x11) || (CEC_MessageInfo_p->Data[1] > 0x1F) )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_GIVE_DECK_STATUS                :
                case STHDMI_CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS         :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.StatusRequest = CEC_MessageInfo_p->Data[1];
                        if(  (CEC_MessageInfo_p->Data[1] < 1) || (CEC_MessageInfo_p->Data[1] > 3) )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;
                case STHDMI_CEC_OPCODE_PLAY                          :
                    if(CEC_MessageInfo_p->DataLength-1 < 1)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.PlayMode      = CEC_MessageInfo_p->Data[1];
                        switch(CEC_MessageInfo_p->Data[1])
                        {
                            case STHDMI_CEC_PLAY_MODE_FORWARD                     :
                            case STHDMI_CEC_PLAY_MODE_REVERSE                     :
                            case STHDMI_CEC_PLAY_MODE_STILL                       :
                            case STHDMI_CEC_PLAY_MODE_FAST_FORWARD_MIN_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_FAST_FORWARD_MEDUIM_SPEED   :
                            case STHDMI_CEC_PLAY_MODE_FAST_FORWARD_MAX_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_FAST_REVERSE_MIN_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_FAST_REVERSE_MEDUIM_SPEED   :
                            case STHDMI_CEC_PLAY_MODE_FAST_REVERSE_MAX_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_SLOW_FORWARD_MIN_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_SLOW_FORWARD_MEDUIM_SPEED   :
                            case STHDMI_CEC_PLAY_MODE_SLOW_FORWARD_MAX_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_SLOW_REVERSE_MIN_SPEED      :
                            case STHDMI_CEC_PLAY_MODE_SLOW_REVERSE_MEDUIM_SPEED   :
                            case STHDMI_CEC_PLAY_MODE_SLOW_REVERSE_MAX_SPEED      :
                                /* Param OK */
                                break;
                            default  :
                                ErrorCode = ST_ERROR_BAD_PARAMETER;
                                break;
                        }
                    }
                    break;

                case STHDMI_CEC_OPCODE_SELECT_ANALOGUE_SERVICE         :
                    if(CEC_MessageInfo_p->DataLength-1 < 4)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueBroadcastType = CEC_MessageInfo_p->Data[1];
                        CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueFrequency     = ((((U16)(CEC_MessageInfo_p->Data[2]))<<8) | (U16)(CEC_MessageInfo_p->Data[3]));
                        CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.BroadcastSystem       = CEC_MessageInfo_p->Data[4];
                        if  (    (CEC_MessageInfo_p->Data[1] > 0x02)
                              || (CEC_CommandInfo_p->CEC_Operand.BroadcastSystem > 0x1F)
                              || ( (CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueFrequency == 0x0000) || (CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueFrequency == 0xFFFF) )
                              || ( (CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.BroadcastSystem > 9)  && (CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.BroadcastSystem != 31) )
                            )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                    }
                    break;
                case STHDMI_CEC_OPCODE_TUNER_DEVICE_STATUS             :
                    if(CEC_MessageInfo_p->DataLength-1 < 5)
                    {
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                    }
                    else
                    {
                        CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.RecordingFlag     =  CEC_MessageInfo_p->Data[1] &0x80;
                        CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.TunerDisplayInfo  =  CEC_MessageInfo_p->Data[1] &0x7F;
                        if( (CEC_MessageInfo_p->Data[1] &0x7F) > 2 )
                        {
                            ErrorCode = ST_ERROR_BAD_PARAMETER;
                        }
                        else
                        {
                            if(CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.TunerDisplayInfo == STHDMI_CEC_TUNER_DISPLAY_INFO_DISPLAYING_DIGITAL_TUNER)
                            {
                                ErrorCode = sthdmi_CECParse_DigitalServiceIdentification(CEC_MessageInfo_p, 2, &CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.DigitalSystem);
                            }
                            else /* Handle rest of cases, it's not digital so it's 5 bytes */
                            {
                                CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueBroadcastType               = CEC_MessageInfo_p->Data[2];
                                CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueFrequency                   = ((((U16)(CEC_MessageInfo_p->Data[3]))<<8) | (U16)(CEC_MessageInfo_p->Data[4]));
                                CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.BroadcastSystem                     = CEC_MessageInfo_p->Data[5];
                                if  (    (CEC_MessageInfo_p->Data[2] > 0x02)
                                    || (CEC_CommandInfo_p->CEC_Operand.BroadcastSystem > 0x1F)
                                    || ( (CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueFrequency == 0x0000) || (CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueFrequency == 0xFFFF) )
                                    || ( (CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.BroadcastSystem > 9)  && (CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.BroadcastSystem != 31) )
                                    )
                                {
                                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                                }
                            }
                        }
                    }
                    break;
                default :
                    ErrorCode = ST_ERROR_BAD_PARAMETER; /* Unrecognised Opcode */
                    break;
            } /* switch (CEC_CommandInfo_p->CEC_Opcode) */
        } /* if(CEC_MessageInfo_p->DataLength == 0) */
     break;
     default :
         ErrorCode = ST_ERROR_BAD_PARAMETER;
         break;
   } /* switch (Device_p->DeviceType) */
   return(ErrorCode);
} /* end of sthdmi_CECParseMessage() */

/**********************************************************************************
Name        :   sthdmi_CECFormat_DigitalServiceIdentification
Description :   Read the Command Structure and constitute the binary Message
 *              of the Digital Service Identification parameter type
Parameters  :   DigitalServiceIdentification_p (pointer) ,  FormatOffset
 *              CEC_MessageInfo_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_CECFormat_DigitalServiceIdentification (  const  STHDMI_CEC_DigitalServiceIdentification_t*   DigitalServiceIdentification_p,
                                                                const  U8   FormatOffset,
                                                                STVOUT_CECMessage_t*   const CEC_MessageInfo_p
                                                             )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    CEC_MessageInfo_p->Data[0+FormatOffset]     = DigitalServiceIdentification_p->ServiceIdentificationMethod | DigitalServiceIdentification_p->DigitalBroadcastSystem;
    if(DigitalServiceIdentification_p->ServiceIdentificationMethod == STHDMI_CEC_SERVICE_IDENTIFICATION_METHOD_BY_DIGITAL_IDS)
    {
            switch (DigitalServiceIdentification_p->DigitalBroadcastSystem)
            {
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_GENERIC      :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_BS           :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_CS           :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_T            :
                    CEC_MessageInfo_p->Data[1+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ARIBData.TransportStreamID>>8);
                    CEC_MessageInfo_p->Data[2+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ARIBData.TransportStreamID & 0xFF );
                    CEC_MessageInfo_p->Data[3+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ARIBData.ServiceID>>8);
                    CEC_MessageInfo_p->Data[4+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ARIBData.ServiceID & 0xFF);
                    CEC_MessageInfo_p->Data[5+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ARIBData.OriginalNetworkID>>8);
                    CEC_MessageInfo_p->Data[6+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ARIBData.OriginalNetworkID & 0xFF);
                    break;

                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_GENERIC      :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_CABLE        :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_SATELLITE    :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_TERRESTRIAL  :
                    CEC_MessageInfo_p->Data[1+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ATSCData.TransportStreamID>>8);
                    CEC_MessageInfo_p->Data[2+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ATSCData.TransportStreamID & 0xFF );
                    CEC_MessageInfo_p->Data[3+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ATSCData.ProgramNumber>>8);
                    CEC_MessageInfo_p->Data[4+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ATSCData.ProgramNumber & 0xFF);
                    CEC_MessageInfo_p->Data[5+FormatOffset]     = 0; /* Reserved */
                    CEC_MessageInfo_p->Data[6+FormatOffset]     = 0; /* Reserved */
                    break;

                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_GENERIC       :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_C             :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_S             :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_S2            :
                case STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_T             :
                    CEC_MessageInfo_p->Data[1+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.DVBData.TransportStreamID>>8);
                    CEC_MessageInfo_p->Data[2+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.DVBData.TransportStreamID & 0xFF );
                    CEC_MessageInfo_p->Data[3+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.DVBData.ServiceID>>8);
                    CEC_MessageInfo_p->Data[4+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.DVBData.ServiceID & 0xFF);
                    CEC_MessageInfo_p->Data[5+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.DVBData.OriginalNetworkID>>8);
                    CEC_MessageInfo_p->Data[6+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.DVBData.OriginalNetworkID & 0xFF);
                    break;
                default :
                    break;
            }
    }
    else     /* ServiceIdentifiedByChannel */
    {
        if(DigitalServiceIdentification_p->ServiceIdentification.ChannelData.ChannelNumberFormat == STHDMI_CEC_CHANNEL_NUMBER_FORMAT_TWO_PART)
        {
            CEC_MessageInfo_p->Data[1+FormatOffset]     = (DigitalServiceIdentification_p->ServiceIdentification.ChannelData.ChannelNumberFormat | ((U8)(DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MajorChannelNumber>>8)) );
            CEC_MessageInfo_p->Data[2+FormatOffset]     = (U8) (DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MajorChannelNumber & 0xFF );
            CEC_MessageInfo_p->Data[3+FormatOffset]     = (U8) (DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MinorChannelNumber>>8);
            CEC_MessageInfo_p->Data[4+FormatOffset]     = (U8) (DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MinorChannelNumber & 0xFF);
        }
        else
        {
            CEC_MessageInfo_p->Data[1+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ChannelData.ChannelNumberFormat);
            CEC_MessageInfo_p->Data[2+FormatOffset]     = (U8)0;
            CEC_MessageInfo_p->Data[3+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MinorChannelNumber>>8);
            CEC_MessageInfo_p->Data[4+FormatOffset]     = (U8)(DigitalServiceIdentification_p->ServiceIdentification.ChannelData.MinorChannelNumber & 0xFF);
        }
        CEC_MessageInfo_p->Data[5+FormatOffset]     = (U8)0; /* Reserved */
        CEC_MessageInfo_p->Data[6+FormatOffset]     = (U8)0; /* Reserved */
    }
    CEC_MessageInfo_p->DataLength += 7;

    return (ErrorCode);
} /* end of sthdmi_CECFormat_DigitalServiceIdentification() */
/**********************************************************************************
Name        :   sthdmi_CECFormatMessage
Description :   Read the Command Structure and constitute the binary Message
Parameters  :   Handler, CEC_CommandInfo_p (pointer) ,  CEC_MessageInfo_p (pointer)
Assumptions :
Limitations :
Returns     :   ST_ErrorCode_t
***********************************************************************************/
ST_ErrorCode_t sthdmi_CECFormatMessage (sthdmi_Device_t * Device_p,
                                        const  STHDMI_CEC_Command_t*      CEC_CommandInfo_p,
                                        STVOUT_CECMessage_t*   const     CEC_MessageInfo_p
                                       )

{
   U8 i, LengthString= 0;
   ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

   switch (Device_p->DeviceType)
   {
     case STHDMI_DEVICE_TYPE_7710 : /* no break*/
     case STHDMI_DEVICE_TYPE_7100 : /* no break */
     case STHDMI_DEVICE_TYPE_7200 :
        CEC_MessageInfo_p->Source      = CEC_CommandInfo_p->Source;
        CEC_MessageInfo_p->Destination = CEC_CommandInfo_p->Destination;
        CEC_MessageInfo_p->Retries = CEC_CommandInfo_p->Retries;

        CEC_MessageInfo_p->Data[0] = CEC_CommandInfo_p->CEC_Opcode;
        CEC_MessageInfo_p->DataLength = 1;

        switch (CEC_CommandInfo_p->CEC_Opcode)
        {
            case STHDMI_CEC_OPCODE_POLLING_MESSAGE                 :
                /* No Data in the Ping message */
                CEC_MessageInfo_p->DataLength = 0;
                break;
            case STHDMI_CEC_OPCODE_IMAGE_VIEW_ON                   :
            case STHDMI_CEC_OPCODE_TEXT_VIEW_ON                    :
            case STHDMI_CEC_OPCODE_REQUEST_ACTIVE_SOURCE           :
            case STHDMI_CEC_OPCODE_STANDBY                         :
            case STHDMI_CEC_OPCODE_RECORD_OFF                      :
            case STHDMI_CEC_OPCODE_RECORD_TV_SCREEN                :
            case STHDMI_CEC_OPCODE_GET_CEC_VERSION                 :
            case STHDMI_CEC_OPCODE_GIVE_PHYSICAL_ADDRESS           :
            case STHDMI_CEC_OPCODE_GET_MENU_LANGUAGE               :
            case STHDMI_CEC_OPCODE_TUNER_STEP_DECREMENT            :
            case STHDMI_CEC_OPCODE_TUNER_STEP_INCREMENT            :
            case STHDMI_CEC_OPCODE_GIVE_DEVICE_VENDOR_ID           :
            case STHDMI_CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP         :
            case STHDMI_CEC_OPCODE_GIVE_OSD_NAME                   :
            case STHDMI_CEC_OPCODE_USER_CONTROL_RELEASED           :
            case STHDMI_CEC_OPCODE_GIVE_DEVICE_POWER_STATUS        :
            case STHDMI_CEC_OPCODE_ABORT_MESSAGE                   :
            case STHDMI_CEC_OPCODE_GIVE_AUDIO_STATUS               :
            case STHDMI_CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS    :
                /* No parameters with these messages */
                CEC_MessageInfo_p->Data[0] = CEC_CommandInfo_p->CEC_Opcode;
                CEC_MessageInfo_p->DataLength = 1;
                break;

            case STHDMI_CEC_OPCODE_ACTIVE_SOURCE                  :
            case STHDMI_CEC_OPCODE_INACTIVE_SOURCE                :
            case STHDMI_CEC_OPCODE_ROUTING_INFORMATION            :
            case STHDMI_CEC_OPCODE_SET_STREAM_PATH                 :
            case STHDMI_CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST        :
                CEC_MessageInfo_p->Data[1] = ((CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.A<<4)&0xF0) | ((CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.B)&0x0F);
                CEC_MessageInfo_p->Data[2] = ((CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.C<<4)&0xF0) | ((CEC_CommandInfo_p->CEC_Operand.PhysicalAddress.D)&0x0F);
                CEC_MessageInfo_p->DataLength += 2;
                break;

            case STHDMI_CEC_OPCODE_ROUTING_CHANGE                 :
                CEC_MessageInfo_p->Data[1] = ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.A<<4)&0xF0) | ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.B)&0x0F);
                CEC_MessageInfo_p->Data[2] = ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.C<<4)&0xF0) | ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.OriginalAddress.D)&0x0F);
                CEC_MessageInfo_p->Data[3] = ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.A<<4)&0xF0)      | ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.B)&0x0F);
                CEC_MessageInfo_p->Data[4] = ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.C<<4)&0xF0)      | ((CEC_CommandInfo_p->CEC_Operand.RoutingChange.NewAddress.D)&0x0F);
                CEC_MessageInfo_p->DataLength += 4;
                break;

            case STHDMI_CEC_OPCODE_REPORT_PHYSICAL_ADDRESS         :
                CEC_MessageInfo_p->Data[1] = ((CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.A<<4)&0xF0) | ((CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.B)&0x0F);
                CEC_MessageInfo_p->Data[2] = ((CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.C<<4)&0xF0) | ((CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.PhysicalAddress.D)&0x0F);
                CEC_MessageInfo_p->Data[3]     = CEC_CommandInfo_p->CEC_Operand.ReportPhysicalAddress.DeviceType;
                CEC_MessageInfo_p->DataLength += 3;
                break;

            case STHDMI_CEC_OPCODE_CEC_VERSION                    :
                CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.CECVersion;
                CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_REPORT_AUDIO_STATUS             :
                CEC_MessageInfo_p->Data[1]     = (CEC_CommandInfo_p->CEC_Operand.AudioStatus.AudioMuteStatus) | (CEC_CommandInfo_p->CEC_Operand.AudioStatus.AudioVolumeStatus.KnownAudioVolumeStatus);
                CEC_MessageInfo_p->DataLength += 1;
                break;
            case STHDMI_CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS         :
            case STHDMI_CEC_OPCODE_SET_SYSTEM_AUDIO_MODE            :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.SystemAudioStatus;
                    CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_SET_AUDIO_RATE                  :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.AudioRate;
                    CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_REPORT_POWER_STATUS             :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.PowerStatus;
                    CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_FEATURE_ABORT                  :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.FeatureAbort.FeatureOpcode;
                    CEC_MessageInfo_p->Data[2]     = CEC_CommandInfo_p->CEC_Operand.FeatureAbort.AbortReason;
                    CEC_MessageInfo_p->DataLength += 2;
                break;

            case STHDMI_CEC_OPCODE_USER_CONTROL_PRESSED            :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.UICommand.Opcode;
                    CEC_MessageInfo_p->DataLength += 1;
                    if(CEC_CommandInfo_p->CEC_Operand.UICommand.UseAdditionalOperands)
                    {
                        switch (CEC_CommandInfo_p->CEC_Operand.UICommand.Opcode)
                        {
                            case STHDMI_CEC_UICOMMAND_OPCODE_PLAY_FUNCTION               :
                                CEC_MessageInfo_p->Data[2] = CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.PlayMode;
                                CEC_MessageInfo_p->DataLength += 1;
                                break;
                            case STHDMI_CEC_UICOMMAND_OPCODE_TUNE_FUNCTION               :
                                if(CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.ChannelNumberFormat == STHDMI_CEC_CHANNEL_NUMBER_FORMAT_TWO_PART) /* need to be tested */
                                {
                                    CEC_MessageInfo_p->Data[2] = (CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.ChannelNumberFormat | ((U8)(CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MajorChannelNumber>>8)) );
                                    CEC_MessageInfo_p->Data[3] = (U8) (CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MajorChannelNumber & 0xFF);
                                    CEC_MessageInfo_p->Data[4] = (U8) ((CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MinorChannelNumber>>8)&0xFF);
                                    CEC_MessageInfo_p->Data[5] = (U8) ((CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MinorChannelNumber)&0xFF);
                                }
                                else
                                {
                                    CEC_MessageInfo_p->Data[2] = (U8) (CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.ChannelNumberFormat);
                                    CEC_MessageInfo_p->Data[3] = 0; /* Should be ignored */
                                    CEC_MessageInfo_p->Data[4] = (U8) ((CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MinorChannelNumber>>8)&0xFF);
                                    CEC_MessageInfo_p->Data[5] = (U8) ((CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.ChannelIdentifier.MinorChannelNumber)&0xFF);
                                }
                                CEC_MessageInfo_p->DataLength += 4;
                                break;
                            case STHDMI_CEC_UICOMMAND_OPCODE_SELECT_MEDIA_FUNCTION        :
                                CEC_MessageInfo_p->Data[2] = CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.UIFunctionMedia;
                                CEC_MessageInfo_p->DataLength += 1;
                                break;
                            case STHDMI_CEC_UICOMMAND_OPCODE_SELECT_AV_INPUT_FUNCTION      :
                                CEC_MessageInfo_p->Data[2] = CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.UIFunctionSelectAVInput;
                                CEC_MessageInfo_p->DataLength += 1;
                                break;
                            case STHDMI_CEC_UICOMMAND_OPCODE_SELECT_AUDIO_INPUT_FUNCTION   :
                                CEC_MessageInfo_p->Data[2] = CEC_CommandInfo_p->CEC_Operand.UICommand.Operand.UIFunctionSelectAudioInput;
                                CEC_MessageInfo_p->DataLength += 1;
                                break;
                            default                             :
                                /* Pb : No optional parameters to add */
                                break;
                        }
                    }
                break;

            case STHDMI_CEC_OPCODE_MENU_STATUS                    :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.MenuState;
                    CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_MENU_REQUEST                   :
                    CEC_MessageInfo_p->Data[1]     = CEC_CommandInfo_p->CEC_Operand.MenuRequestType;
                    CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_SELECT_DIGITAL_SERVICE          :
                ErrorCode = sthdmi_CECFormat_DigitalServiceIdentification(&CEC_CommandInfo_p->CEC_Operand.DigitalServiceIdentification, 1, CEC_MessageInfo_p);
                /* DataLength Updated in sthdmi_CECFormat_DigitalServiceIdentification */
                break;

            case STHDMI_CEC_OPCODE_CLEAR_ANALOGUE_TIMER            :
            case STHDMI_CEC_OPCODE_SET_ANALOGUE_TIMER              :
                CEC_MessageInfo_p->Data[1]     =      CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.DayOfMonth;
                CEC_MessageInfo_p->Data[2]     = (U8)(CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.MonthOfYear);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.StartTime.Hour, &CEC_MessageInfo_p->Data[3]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.StartTime.Minute, &CEC_MessageInfo_p->Data[4]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.Duration.DurationHour, &CEC_MessageInfo_p->Data[5]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.Duration.Minute, &CEC_MessageInfo_p->Data[6]);
                CEC_MessageInfo_p->Data[7]     =      CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.RecordingSequence;

                CEC_MessageInfo_p->Data[8]     =      CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueBroadcastType;
                CEC_MessageInfo_p->Data[9]     = (U8)(CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueFrequency>>8);
                CEC_MessageInfo_p->Data[10]    = (U8)(CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.AnalogueFrequency & 0xFF);
                CEC_MessageInfo_p->Data[11]    =      CEC_CommandInfo_p->CEC_Operand.SetAnalogueTimer.BroadcastSystem;
                CEC_MessageInfo_p->DataLength += 11;
                break;

            case STHDMI_CEC_OPCODE_CLEAR_DIGITAL_TIMER             :
            case STHDMI_CEC_OPCODE_SET_DIGITAL_TIMER               :
                CEC_MessageInfo_p->Data[1]     =      CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.DayOfMonth;
                CEC_MessageInfo_p->Data[2]     = (U8)(CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.MonthOfYear);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.StartTime.Hour, &CEC_MessageInfo_p->Data[3]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.StartTime.Minute, &CEC_MessageInfo_p->Data[4]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.Duration.DurationHour, &CEC_MessageInfo_p->Data[5]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.Duration.Minute, &CEC_MessageInfo_p->Data[6]);
                CEC_MessageInfo_p->Data[7]     =      CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.RecordingSequence;
                CEC_MessageInfo_p->DataLength += 7;

                ErrorCode |= sthdmi_CECFormat_DigitalServiceIdentification(&CEC_CommandInfo_p->CEC_Operand.SetDigitalTimer.DigitalServiceIdentification, 8, CEC_MessageInfo_p);
                /* DataLength Updated in sthdmi_CECFormat_DigitalServiceIdentification */
                break;

            case STHDMI_CEC_OPCODE_CLEAR_EXTERNAL_TIMER            :
            case STHDMI_CEC_OPCODE_SET_EXTERNAL_TIMER              :
                CEC_MessageInfo_p->Data[1]     =      CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.DayOfMonth;
                CEC_MessageInfo_p->Data[2]     = (U8)(CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.MonthOfYear);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.StartTime.Hour, &CEC_MessageInfo_p->Data[3]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.StartTime.Minute, &CEC_MessageInfo_p->Data[4]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.Duration.DurationHour, &CEC_MessageInfo_p->Data[5]);
                ErrorCode |= sthdmi_DecimalToBCD(     CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.Duration.Minute, &CEC_MessageInfo_p->Data[6]);
                CEC_MessageInfo_p->Data[7]     =      CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.RecordingSequence;

                CEC_MessageInfo_p->Data[8]     =      CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalSourceSpecifier;
                if(CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalSourceSpecifier == STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PLUG)
                {
                    CEC_MessageInfo_p->Data[9]     =  CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPlug;
                    CEC_MessageInfo_p->DataLength += 9;
                }
                else        /* External Source Specifier is External Physical Address */
                {
                    CEC_MessageInfo_p->Data[9]      = CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.A<<4 | CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.B;
                    CEC_MessageInfo_p->Data[10]     = CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.C<<4 | CEC_CommandInfo_p->CEC_Operand.SetExternalTimer.ExternalPlugType.ExternalPhysicalAddress.D;
                    CEC_MessageInfo_p->DataLength += 10;
                }
                break;

            case STHDMI_CEC_OPCODE_SET_TIMER_PROGRAM_TITLE          :
                LengthString = 0;
                while( (CEC_CommandInfo_p->CEC_Operand.ProgramTitleString[LengthString] != '\0')&&(LengthString <= 13))
                {
                    CEC_MessageInfo_p->Data[LengthString+1] = CEC_CommandInfo_p->CEC_Operand.ProgramTitleString[LengthString];
                    LengthString ++;
                }
                CEC_MessageInfo_p->DataLength += LengthString;
                break;

            case STHDMI_CEC_OPCODE_SET_OSD_NAME                    :
                LengthString = 0;
                while( (CEC_CommandInfo_p->CEC_Operand.OSDName[LengthString] != '\0')&&(LengthString <= 13))
                {
                    CEC_MessageInfo_p->Data[LengthString+1] = CEC_CommandInfo_p->CEC_Operand.ProgramTitleString[LengthString];
                    LengthString ++;
                }
                CEC_MessageInfo_p->DataLength += LengthString;
                break;


            case STHDMI_CEC_OPCODE_SET_OSD_STRING                  :
                CEC_MessageInfo_p->Data[1]     = (U8)(CEC_CommandInfo_p->CEC_Operand.SetOSDString.DisplayControl);
                LengthString = 0;
                while( (CEC_CommandInfo_p->CEC_Operand.SetOSDString.OSDString[LengthString] != '\0')&&(LengthString <= 12))
                {
                    CEC_MessageInfo_p->Data[LengthString+1 + 1] = CEC_CommandInfo_p->CEC_Operand.SetOSDString.OSDString[LengthString];
                    LengthString ++;
                }
                CEC_MessageInfo_p->DataLength += LengthString + 1;
                break;

            case STHDMI_CEC_OPCODE_DEVICE_VENDOR_ID                :
                CEC_MessageInfo_p->Data[1]     = (U8)((CEC_CommandInfo_p->CEC_Operand.VendorID>>16)&0xFF);
                CEC_MessageInfo_p->Data[2]     = (U8)((CEC_CommandInfo_p->CEC_Operand.VendorID>>8)&0xFF);
                CEC_MessageInfo_p->Data[3]     = (U8)(CEC_CommandInfo_p->CEC_Operand.VendorID &0xFF);
                CEC_MessageInfo_p->DataLength += 3;
                break;

            case STHDMI_CEC_OPCODE_VENDOR_COMMAND                 :
                if(CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength > 13)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    for(i=0; i < CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength; i++)
                    {
                        CEC_MessageInfo_p->Data[i+1] = CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.Data[i];
                    }
                    CEC_MessageInfo_p->DataLength += CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength;
                }
                break;

            case STHDMI_CEC_OPCODE_VENDOR_COMMAND_WITH_ID           :

                if(CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength > 10)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    CEC_MessageInfo_p->Data[1]     = (U8)(CEC_CommandInfo_p->CEC_Operand.VendorID>>16);
                    CEC_MessageInfo_p->Data[2]     = (U8)(CEC_CommandInfo_p->CEC_Operand.VendorID>>8);
                    CEC_MessageInfo_p->Data[3]     = (U8)(CEC_CommandInfo_p->CEC_Operand.VendorID &0xFF);
                    for(i=0; i < CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength; i++)
                    {
                        CEC_MessageInfo_p->Data[i+1+3] = CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.Data[i];
                    }
                    CEC_MessageInfo_p->DataLength += (3 + CEC_CommandInfo_p->CEC_Operand.VendorSpecificData.DataLength);
                }
                break;

            case STHDMI_CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN        :
                if(CEC_CommandInfo_p->CEC_Operand.VendorSpecificRCCode.DataLength > 13)
                {
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                }
                else
                {
                    for(i=0; i < CEC_CommandInfo_p->CEC_Operand.VendorSpecificRCCode.DataLength; i++)
                    {
                        CEC_MessageInfo_p->Data[i+1] = CEC_CommandInfo_p->CEC_Operand.VendorSpecificRCCode.Data[i];
                    }
                    CEC_MessageInfo_p->DataLength += CEC_CommandInfo_p->CEC_Operand.VendorSpecificRCCode.DataLength;
                }
                break;

            case STHDMI_CEC_OPCODE_TIMER_CLEARED_STATUS             :
                    CEC_MessageInfo_p->Data[1]     = (U8)CEC_CommandInfo_p->CEC_Operand.TimerClearedStatusData;
                    CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_TIMER_STATUS                   :
                    if(CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedIndicator == STHDMI_CEC_PROGRAMMED_INDICATOR_PROGRAMMED)
                    {
                        CEC_MessageInfo_p->Data[1]     =     CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerOverlapWarning
                                                          |  CEC_CommandInfo_p->CEC_Operand.TimerStatusData.MediaInfo
                                                          |  CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedIndicator
                                                          |  CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.ProgrammedInfo;
                        CEC_MessageInfo_p->DataLength += 1;
                        if(      (      (CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.ProgrammedInfo == STHDMI_CEC_PROGRAMMED_INFO_NOT_ENOUGH_SPACE_AVAILABLE_FOR_RECORDING)
                                     || (CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.ProgrammedInfo == STHDMI_CEC_PROGRAMMED_INFO_MAY_NOT_BE_ENOUGH_SPACE_AVAILABLE)  )
                             &&          CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.IsDurationAvailable
                            )
                        {
                            ErrorCode |= sthdmi_DecimalToBCD(CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.Duration.DurationHour,&CEC_MessageInfo_p->Data[2]);
                            ErrorCode |= sthdmi_DecimalToBCD(CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.Duration.Minute,&CEC_MessageInfo_p->Data[3]);
                            CEC_MessageInfo_p->DataLength += 2;
                        }
                    }
                    else
                    {
                        CEC_MessageInfo_p->Data[1]     =     CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerOverlapWarning
                                                          |  CEC_CommandInfo_p->CEC_Operand.TimerStatusData.MediaInfo
                                                          |  CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedIndicator
                                                          |  CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.NotProgrammedErrorInfo;
                        CEC_MessageInfo_p->DataLength += 1;
                        if( (CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.ProgrammedInfoType.NotProgrammedErrorInfo == STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_DUPLICATE)
                             &&          CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.IsDurationAvailable
                          )
                        {
                            ErrorCode |= sthdmi_DecimalToBCD(CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.Duration.DurationHour,&CEC_MessageInfo_p->Data[2]);
                            ErrorCode |= sthdmi_DecimalToBCD(CEC_CommandInfo_p->CEC_Operand.TimerStatusData.TimerProgrammedInfo.Duration.Minute,&CEC_MessageInfo_p->Data[3]);
                            CEC_MessageInfo_p->DataLength += 2;
                        }
                    }
                break;

            case STHDMI_CEC_OPCODE_RECORD_ON                      :
                switch (CEC_CommandInfo_p->CEC_Operand.RecordSource.OwnSource) /* Here we have to count on fact that RecordSourceType is always at same location, so we can fit structure closest to the spec */
                {
                    case STHDMI_CEC_RECORD_SOURCE_TYPE_OWN_SOURCE                      :
                        CEC_MessageInfo_p->Data[1]     = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.OwnSource);
                        CEC_MessageInfo_p->DataLength += 1;
                        break;

                    case STHDMI_CEC_RECORD_SOURCE_TYPE_DIGITAL_SERVICE                 :
                        CEC_MessageInfo_p->Data[1]     = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.DigitalService.RecordSourceType);
                        ErrorCode |= sthdmi_CECFormat_DigitalServiceIdentification(&CEC_CommandInfo_p->CEC_Operand.RecordSource.DigitalService.DigitalServiceIdentification, 2, CEC_MessageInfo_p);
                        /* DataLength Updated in sthdmi_CECFormat_DigitalServiceIdentification */
                        break;

                    case STHDMI_CEC_RECORD_SOURCE_TYPE_ANALOGUE_SERVICE                :
                        CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.AnalogueService.RecordSourceType);
                        CEC_MessageInfo_p->Data[2]        = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.AnalogueService.AnalogueBroadcastType);
                        CEC_MessageInfo_p->Data[3]        = (U8)((CEC_CommandInfo_p->CEC_Operand.RecordSource.AnalogueService.AnalogueFrequency>>8) & 0xFF);
                        CEC_MessageInfo_p->Data[4]        = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.AnalogueService.AnalogueFrequency & 0xFF);
                        CEC_MessageInfo_p->Data[5]        = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.AnalogueService.BroadcastSystem);
                        CEC_MessageInfo_p->DataLength += 5;
                        break;

                    case STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PLUG                   :
                        CEC_MessageInfo_p->Data[1]     = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPlug.RecordSourceType);
                        CEC_MessageInfo_p->Data[2]     = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPlug.ExternalPlug);
                        CEC_MessageInfo_p->DataLength += 2;
                        break;

                    case STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PHYSCIAL_ADDRESS        :
                        CEC_MessageInfo_p->Data[1] = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPhysicalAddress.RecordSourceType);
                        CEC_MessageInfo_p->Data[2] = (U8)((CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPhysicalAddress.ExternalPhysicalAddress.A<<4) | (CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPhysicalAddress.ExternalPhysicalAddress.B));
                        CEC_MessageInfo_p->Data[3] = (U8)((CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPhysicalAddress.ExternalPhysicalAddress.C<<4) | (CEC_CommandInfo_p->CEC_Operand.RecordSource.ExternalPhysicalAddress.ExternalPhysicalAddress.D));
                        CEC_MessageInfo_p->DataLength += 3;
                        break;

                    default :
                        ErrorCode = ST_ERROR_BAD_PARAMETER;
                        break;
                }
                break;

            case STHDMI_CEC_OPCODE_RECORD_STATUS                  :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.RecordStatusInfo);
                CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_SET_MENU_LANGUAGE               :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.Language[0]);
                CEC_MessageInfo_p->Data[2]        = (U8)(CEC_CommandInfo_p->CEC_Operand.Language[1]);
                CEC_MessageInfo_p->Data[3]        = (U8)(CEC_CommandInfo_p->CEC_Operand.Language[2]);
                CEC_MessageInfo_p->DataLength += 3;
                break;

            case STHDMI_CEC_OPCODE_DECK_CONTROL                   :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.DeckControlMode);
                CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_DECK_STATUS                    :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.DeckInfo);
                CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_GIVE_DECK_STATUS                :
            case STHDMI_CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS         :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.StatusRequest);
                CEC_MessageInfo_p->DataLength += 1;
                break;
            case STHDMI_CEC_OPCODE_PLAY                          :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.PlayMode);
                CEC_MessageInfo_p->DataLength += 1;
                break;

            case STHDMI_CEC_OPCODE_SELECT_ANALOGUE_SERVICE         :
                CEC_MessageInfo_p->Data[1]        = (U8)(CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueBroadcastType);
                CEC_MessageInfo_p->Data[2]        = (U8)((CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueFrequency>>8)& 0xFF);
                CEC_MessageInfo_p->Data[3]        = (U8)(CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.AnalogueFrequency & 0xFF);
                CEC_MessageInfo_p->Data[4]        = (U8)(CEC_CommandInfo_p->CEC_Operand.AnalogueServiceIdentification.BroadcastSystem);
                CEC_MessageInfo_p->DataLength += 4;
                break;

            case STHDMI_CEC_OPCODE_TUNER_DEVICE_STATUS             :
                CEC_MessageInfo_p->Data[1]        = (CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.RecordingFlag) | (CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.TunerDisplayInfo);
                CEC_MessageInfo_p->DataLength += 1;
                if(CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.TunerDisplayInfo == STHDMI_CEC_TUNER_DISPLAY_INFO_DISPLAYING_DIGITAL_TUNER)
                {
                    ErrorCode |= sthdmi_CECFormat_DigitalServiceIdentification(&CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.DigitalSystem, 2, CEC_MessageInfo_p);
                    /* DataLength Updated in sthdmi_CECFormat_DigitalServiceIdentification */
                }
                else if(CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.TunerDisplayInfo == STHDMI_CEC_TUNER_DISPLAY_INFO_DISPLAYING_ANALOGUE_TUNER)
                {
                    CEC_MessageInfo_p->Data[2]        = (U8)(CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueBroadcastType);
                    CEC_MessageInfo_p->Data[3]        = (U8)((CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueFrequency>>8)& 0xFF);
                    CEC_MessageInfo_p->Data[4]        = (U8)(CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.AnalogueFrequency & 0xFF);
                    CEC_MessageInfo_p->Data[5]        = (U8)(CEC_CommandInfo_p->CEC_Operand.TunerDeviceInfo.A_DSystem.AnalogueSystem.BroadcastSystem);
                    CEC_MessageInfo_p->DataLength += 4;
                }
                else  /* Not displaying tuner */
                {
                    /* Do not update DataLength */
                }
                break;

            default :
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }/* end of the switch (CEC_CommandInfo_p->CEC_Opcode) */
         break;
     default :
         ErrorCode = ST_ERROR_BAD_PARAMETER;
         break;
   }
   return(ErrorCode);
} /* end of sthdmi_CECFormatMessage */
#endif /* STHDMI_CEC */
/* End of hdmi_src.c */
/* ------------------------------- End of file ---------------------------- */


