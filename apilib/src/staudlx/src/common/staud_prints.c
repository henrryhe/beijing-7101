/************************************************************************
COPYRIGHT STMicroelectronics (C) 2004
Source file name : staud.c

Author : Udit Kumar

History

Note : Last version on the top

---------------------------------------------------------
Date            |       Activity        |       Owner
---------------------------------------------------------
21-Nov-2007         |       Started     |       UK
---------------------------------------------------------
                |                   |
---------------------------------------------------------


************************************************************************/


/*****************************************************************************
Includes
*****************************************************************************/
#define STTBX_PRINT 1
#ifndef ST_OSLINUX
    #include "sttbx.h"
#endif
#include "staudlx_prints.h"

char * GetObjectNameFromID( STAUD_Object_t Object);
char * GetObjectNameFromID( STAUD_Object_t Object)
{

    switch(Object)
    {

        case STAUD_OBJECT_INPUT_CD0 :
            return "STAUD_OBJECT_INPUT_CD0";

        case STAUD_OBJECT_INPUT_CD1 :
            return "STAUD_OBJECT_INPUT_CD1";

        case STAUD_OBJECT_INPUT_CD2 :
            return "STAUD_OBJECT_INPUT_CD2";

        case STAUD_OBJECT_INPUT_PCM0 :
            return "STAUD_OBJECT_INPUT_PCM0";

        case STAUD_OBJECT_INPUT_PCMREADER0 :
            return "STAUD_OBJECT_INPUT_PCMREADER0";

        case STAUD_OBJECT_DECODER_COMPRESSED0 :
            return "STAUD_OBJECT_DECODER_COMPRESSED0";


        case STAUD_OBJECT_DECODER_COMPRESSED1 :
            return "STAUD_OBJECT_DECODER_COMPRESSED1";


        case STAUD_OBJECT_DECODER_COMPRESSED2 :
            return "STAUD_OBJECT_DECODER_COMPRESSED2";

        case STAUD_OBJECT_ENCODER_COMPRESSED0 :
            return "STAUD_OBJECT_ENCODER_COMPRESSED0";

        case STAUD_OBJECT_POST_PROCESSOR0 :
            return "STAUD_OBJECT_POST_PROCESSOR0";

        case STAUD_OBJECT_POST_PROCESSOR1 :
            return "STAUD_OBJECT_POST_PROCESSOR1";


        case STAUD_OBJECT_POST_PROCESSOR2 :
            return "STAUD_OBJECT_POST_PROCESSOR2";

        case STAUD_OBJECT_OUTPUT_PCMP0 :
            return "STAUD_OBJECT_OUTPUT_PCMP0";


        case STAUD_OBJECT_OUTPUT_PCMP1 :
            return "STAUD_OBJECT_OUTPUT_PCMP1";


        case STAUD_OBJECT_OUTPUT_PCMP2 :
            return "STAUD_OBJECT_OUTPUT_PCMP2";


        case STAUD_OBJECT_OUTPUT_PCMP3 :
            return "STAUD_OBJECT_OUTPUT_PCMP3";


        case STAUD_OBJECT_OUTPUT_HDMI_PCMP0 :
            return "STAUD_OBJECT_OUTPUT_HDMI_PCMP0";


        case STAUD_OBJECT_OUTPUT_SPDIF0 :
            return "STAUD_OBJECT_OUTPUT_SPDIF0";


        case STAUD_OBJECT_OUTPUT_HDMI_SPDIF0 :
            return "STAUD_OBJECT_OUTPUT_HDMI_SPDIF0";


        case STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER :
            return "STAUD_OBJECT_SPDIF_FORMATTER_BYTE_SWAPPER";

        case STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER :
            return "STAUD_OBJECT_SPDIF_FORMATTER_BIT_CONVERTER";

        case STAUD_OBJECT_FRAME_PROCESSOR :
            return "STAUD_OBJECT_FRAME_PROCESSOR";

        case STAUD_OBJECT_FRAME_PROCESSOR1 :
            return "STAUD_OBJECT_FRAME_PROCESSOR1";

        case STAUD_OBJECT_BLCKMGR :
            return "STAUD_OBJECT_BLCKMGR";

        default :
            return "Invalid Object";
    }

}

/************************************************************************************
Name         : PrintSTAUD_Init()

Description  : Prints all STAUD_Init
************************************************************************************/
void  PrintSTAUD_Init(const ST_DeviceName_t DeviceName, STAUD_InitParams_t *InitParams_p)
{


    STTBX_Print(("STAUD_Init DeviceName=[%s],InitParams_p=[%x]\n ",DeviceName,(U32)InitParams_p ));

    STTBX_Print(("Device Type =%d, \n Configuration =%d \n CPUPartition =0x%x \n ", InitParams_p->DeviceType,
                        InitParams_p->Configuration, (U32)InitParams_p->CPUPartition_p));

    STTBX_Print(("Interrupt Number =%d \n Interrupt Level =%d \n RegistrantDevice Name =%s \n ",
                        InitParams_p->InterruptNumber, InitParams_p->InterruptLevel, InitParams_p->RegistrantDeviceName));

    STTBX_Print(("Max Open =%d \n Event Handler Name =%s \n AVMemPartation=0x%x \n ",
                        InitParams_p->MaxOpen, InitParams_p->EvtHandlerName, InitParams_p->AVMEMPartition));

    STTBX_Print(("AVMem Buffer Partition =0x%x \n EMBX Allocation =%d \n Clock Recovery Name =%s \n ",
                        InitParams_p->BufferPartition, InitParams_p->AllocateFromEMBX, InitParams_p->ClockRecoveryName));

    STTBX_Print(("DACClock to FS Ratio =%d \n ** PCM Player Config ** \n", InitParams_p->DACClockToFsRatio));

    STTBX_Print((" \t InvetWordClock =%d \n \t InvertBit Clock=%d \n \t DACFormat =%s \n", InitParams_p->PCMOutParams.InvertWordClock,
                        InitParams_p->PCMOutParams.InvertBitClock, (InitParams_p->PCMOutParams.Format==0? "I2S": "Standard")));

    STTBX_Print(("\t DACDataPrecision =%d \n \t DACDataAlignment =%d \n \t MBSFirst =%d \n",
                        InitParams_p->PCMOutParams.Precision,InitParams_p->PCMOutParams.Alignment, InitParams_p->PCMOutParams.MSBFirst));


    STTBX_Print(("\t Freq Mutliplier =%d \n \t MemoryFormat =%s \n ** SPDIF Player Config ** \n",
                        InitParams_p->PCMOutParams.PcmPlayerFrequencyMultiplier,(InitParams_p->PCMOutParams.MemoryStorageFormat16==0?"32Bit":"16 bit")));


    STTBX_Print((" \t AutoLatency =%d \n \t Latency Clock =%d \n \t CopyPermitted =%d \n", InitParams_p->SPDIFOutParams.AutoLatency,
                        InitParams_p->SPDIFOutParams.Latency, InitParams_p->SPDIFOutParams.CopyPermitted));

    STTBX_Print(("\t AutoCategoryCode =%d \n \t CategoryCode =%d \n \t AutoDTDI =%d \n",
                        InitParams_p->SPDIFOutParams.AutoCategoryCode,InitParams_p->SPDIFOutParams.CategoryCode, InitParams_p->SPDIFOutParams.AutoDTDI));


    STTBX_Print(("\t DTDI =%d \n \t Emphasis =%s \n \t SPDIFDataPrecisionPCMMode= %d  \n \t SPDIFPlayerFrequencyMultiplier =%d \n ",
                        InitParams_p->SPDIFOutParams.DTDI,(InitParams_p->SPDIFOutParams.Emphasis==0?"No Emph":"EMPH CD"),
                        InitParams_p->SPDIFOutParams.SPDIFDataPrecisionPCMMode, InitParams_p->SPDIFOutParams.SPDIFPlayerFrequencyMultiplier));


    STTBX_Print(("SPDIF Mode =%d \n PCM Mode =%d \n ** PCM Reader Params ** \n  ",
                        InitParams_p->SPDIFMode, InitParams_p->PCMMode));

    STTBX_Print(("\t MSBFirst =%d \n \t DAC AlignMent  =%s \n \t Padding =%d  \n \t FallingSCLK =%d \n",
                        InitParams_p->PCMReaderMode.MSBFirst,(InitParams_p->PCMReaderMode.Alignment==0?"Left ":"Right"),
                        InitParams_p->PCMReaderMode.Padding, InitParams_p->PCMReaderMode.FallingSCLK));

    STTBX_Print(("\t LeftLRHigh =%d \n \t DAC BitsPerSubFrame  =%s \n \t Precision =%d  \n ",
                        InitParams_p->PCMReaderMode.LeftLRHigh,(InitParams_p->PCMReaderMode.BitsPerSubFrame==0?"32 Bits ":"16 Bits"),
                        InitParams_p->PCMReaderMode.Precision));

    STTBX_Print(("\t Frequency =%d \n \t  Rounding  =%s \n \t MemFormat= %s  \n ",
                        InitParams_p->PCMReaderMode.Frequency,(InitParams_p->PCMReaderMode.Rounding==0?"No Rounding ":"16 Bits Rounding"),
                        (InitParams_p->PCMReaderMode.MemFormat==0?"16_0" : "16_16")));

    STTBX_Print(( "Driver Index =%d \n NumChannel =%d \n Num Ch Reader =%d \n MSPP Support =%d \n",
                        InitParams_p->DriverIndex,  InitParams_p->NumChannels,  InitParams_p->NumChPCMReader,
                        InitParams_p->EnableMSPPSupport));

}

/************************************************************************************
Name         : PrintSTAUD_Open)

Description  : Prints all STAUD_Open
************************************************************************************/

void  PrintSTAUD_Open(  const ST_DeviceName_t DeviceName,
                        const STAUD_OpenParams_t *Params,
                        STAUD_Handle_t *Handle_p)
{
    STTBX_Print(("STAUD_Open DeviceName=[%s],OpenParams[SyncDelay=%d], Handle[%p]\n",DeviceName,Params->SyncDelay,Handle_p));
}



/************************************************************************************
Name         : PrintSTAUD_Close()

Description  : Prints all STAUD_Close .

************************************************************************************/
void  PrintSTAUD_Close(const STAUD_Handle_t Handle)
{
    STTBX_Print(("STAUD_Close Handle =%p", (void *)Handle));
}

/************************************************************************************
Name         : PrintSTAUD_Term()

Description  : Prints all STAUD_Term .

************************************************************************************/

void  PrintSTAUD_Term(  const ST_DeviceName_t DeviceName,
                        const STAUD_TermParams_t *TermParams_p)
{
    STTBX_Print(("STAUD_Term Device Name =%s \n",DeviceName));
    STTBX_Print(("STAUD_Term Params \n Forced Term =%d \n", TermParams_p->ForceTerminate));
}

/************************************************************************************
Name         : PrintSTAUD_GetBufferParam()

Description  : Prints all STAUD_GetBufferParam .

************************************************************************************/


void  PrintSTAUD_GetBufferParam (   STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject,
                                    STAUD_GetBufferParam_t *BufferParam)
{
    STTBX_Print(("STAUD_GetBufferParam : Params \n"));
    STTBX_Print(("Handle =%p \n Object =%s \n Buffer Address =%p \n", (void *)Handle, GetObjectNameFromID(InputObject), (void *)BufferParam));
    STTBX_Print(("Buffer Address=%p \n Length =%d \n ", BufferParam->StartBase,BufferParam->Length));

}


/************************************************************************************
Name         : PrintSTAUD_GetCapability()

Description  : Prints all STAUD_GetCapability .

************************************************************************************/
void   PrintSTAUD_GetCapability (   const ST_DeviceName_t DeviceName,
                                    STAUD_Capability_t * Capability_p)
{
    STTBX_Print(("STAUD_GetCapability \n Device Name =%s \n Cap ptr =%p \n ", DeviceName, Capability_p));
}


/************************************************************************************
Name         : PrintSTAUD_DRConnectSource()

Description  : Prints all STAUD_DRConnectSource .

************************************************************************************/

void  PrintSTAUD_DRConnectSource (STAUD_Handle_t Handle,
                                  STAUD_Object_t DecoderObject,
                                  STAUD_Object_t InputSource)
{

    STTBX_Print(("STAUD_DRConnectSource  Params \n "));
    STTBX_Print(("Handle =%p \n Decoder Object=%s \n Input Source =%s \n", (void *)Handle,GetObjectNameFromID(DecoderObject),
                 GetObjectNameFromID(InputSource)));
}



/************************************************************************************
Name         : PrintSTAUD_ConnectSrcDst()

Description  : Prints all STAUD_ConnectSrcDst .

************************************************************************************/

void  PrintSTAUD_ConnectSrcDst (STAUD_Handle_t Handle,
                                STAUD_Object_t SrcObject,U32 SrcId,
                                STAUD_Object_t DestObject,U32 DstId)
{
    STTBX_Print(("STAUD_ConnectSrcDst   Params \n "));
    STTBX_Print(("Handle =%p \n SrcObject=%s \n srcId=%p \n DestObject =%s \n DstId =%p \n", (void *)Handle,GetObjectNameFromID(SrcObject),
                  (void *)SrcId,GetObjectNameFromID(DestObject),(void *)DstId));
}

/************************************************************************************
Name         : PrintSTAUD_DisconnectInput()

Description  : Prints all STAUD_ConnectSrcDst .

************************************************************************************/

void  PrintSTAUD_DisconnectInput(   STAUD_Handle_t Handle,
                                    STAUD_Object_t Object,U32 InputId)
{
    STTBX_Print(("STAUD_DisconnectInput   Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n InputId=%p \n", (void *)Handle,GetObjectNameFromID(Object),
                        (void *)InputId));
}


/************************************************************************************
Name         : PrintSTAUD_PPDisableDownsampling()

Description  : Prints all STAUD_PPDisableDownsampling .

************************************************************************************/

void  PrintSTAUD_PPDisableDownsampling( STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject)
{
    STTBX_Print(("STAUD_PPDisableDownsampling   Params \n "));
    STTBX_Print(("Handle =%p \n PP Object=%s \n", (void *)Handle,GetObjectNameFromID(PostProcObject)));
}


/************************************************************************************
Name         : PrintSTAUD_OPDisableSynchronization()

Description  : Prints all STAUD_PPDisableDownsampling .

************************************************************************************/


void PrintSTAUD_OPDisableSynchronization (  STAUD_Handle_t Handle,
                                            STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_OPDisableSynchronization   Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n", (void *)Handle,GetObjectNameFromID(OutputObject)));
}

/************************************************************************************
Name         : PrintSTAUD_OPEnableHDMIOutput()

Description  : Prints all STAUD_OPEnableHDMIOutput .

************************************************************************************/

void  PrintSTAUD_OPEnableHDMIOutput (   STAUD_Handle_t Handle,
                                        STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_OPEnableHDMIOutput    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n", (void *)Handle,GetObjectNameFromID(OutputObject)));
}
/************************************************************************************
Name         : PrintSTAUD_OPDisableHDMIOutput ()

Description  : Prints all STAUD_OPDisableHDMIOutput  .

************************************************************************************/
void PrintSTAUD_OPDisableHDMIOutput (   STAUD_Handle_t Handle,
                                        STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_OPDisableHDMIOutput    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n", (void *)Handle,GetObjectNameFromID(OutputObject)));
}


/************************************************************************************
Name         : PrintSTAUD_PPEnableDownsampling ()

Description  : Prints all STAUD_PPEnableDownsampling  .

************************************************************************************/
void  PrintSTAUD_PPEnableDownsampling(  STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        U32 OutPutFrequency)
{
    STTBX_Print(("STAUD_PPEnableDownsampling    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Freq=%d \n", (void *)Handle,GetObjectNameFromID(PostProcObject),OutPutFrequency));
}

/************************************************************************************
Name         : PrintSTAUD_OPEnableSynchronization ()

Description  : Prints all STAUD_OPEnableSynchronization  .

************************************************************************************/
void  PrintSTAUD_OPEnableSynchronization (STAUD_Handle_t Handle,
STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_OPEnableSynchronization    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n", (void *)Handle,GetObjectNameFromID(OutputObject)));
}

/************************************************************************************
Name         : PrintSTAUD_PGetBroadcastProfile ()

Description  : Prints all STAUD_OPEnableSynchronization  .

************************************************************************************/
void PrintSTAUD_IPGetBroadcastProfile ( STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        STAUD_BroadcastProfile_t *
                                        BroadcastProfile_p)
{
    STTBX_Print(("STAUD_IPGetBroadcastProfile    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n BroadCast PTR=%p \n", (void *)Handle,GetObjectNameFromID(InputObject),BroadcastProfile_p));
}


/************************************************************************************
Name         : PrintSTAUD_PGetBroadcastProfile ()

Description  : Prints all STAUD_OPEnableSynchronization  .

************************************************************************************/

void PrintSTAUD_DRGetCapability(const ST_DeviceName_t DeviceName,
                                STAUD_Object_t DecoderObject,
                                STAUD_DRCapability_t * Capability_p)
{
   STTBX_Print(("STAUD_DRGetCapability    Params \n "));
   STTBX_Print(("DeviceName =%s \n Object=%s \n Capability PTR=%p \n", DeviceName,GetObjectNameFromID(DecoderObject),(void *)Capability_p));
}


/************************************************************************************
Name         : PrintSTAUD_GetBroadcastProfile ()

Description  : Prints all PrintSTAUD_GetBroadcastProfile  .

************************************************************************************/


void PrintSTAUD_DRGetDynamicRangeControl (  STAUD_Handle_t Handle,
                                            STAUD_Object_t DecoderObject,
                                            STAUD_DynamicRange_t * DynamicRange_p)
{
    STTBX_Print(("STAUD_DRGetDynamicRangeControl     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n DRNG PTR=%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),DynamicRange_p));
}


/************************************************************************************
Name         : PrintSTAUD_DRGetDownMix ()

Description  : Prints all PrintSTAUD_DRGetDownMix  .

************************************************************************************/

void PrintSTAUD_DRGetDownMix (  STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject,
                                STAUD_DownmixParams_t * DownMixParam_p)
{
    STTBX_Print(("STAUD_DRGetDownMix    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n DownMix PTR=%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),DownMixParam_p));
}


/************************************************************************************
Name         : PrintSTAUD_IPGetSamplingFrequency  ()

Description  : Prints all PrintSTAUD_IPGetSamplingFrequency  .

************************************************************************************/

void PrintSTAUD_IPGetSamplingFrequency (STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        U32 * SamplingFrequency_p)
{
    STTBX_Print(("STAUD_IPGetSamplingFrequency    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Freq PTR=%p \n",(void *)Handle,GetObjectNameFromID(InputObject),SamplingFrequency_p));
}


/************************************************************************************
Name         : PrintSTAUD_DRGetSpeed ()

Description  : Prints all PrintSTAUD_DRGetSpeed  .

************************************************************************************/

void PrintSTAUD_DRGetSpeed (STAUD_Handle_t Handle,
                            STAUD_Object_t DecoderObject,
                            S32            * Speed_p)
{
    STTBX_Print(("STAUD_IPGetSamplingFrequency    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Speed PTR=%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Speed_p));
}


/************************************************************************************
Name         : PrintSTAUD_IPGetStreamInfo ()

Description  : Prints all PrintSTAUD_IPGetStreamInfo  .

************************************************************************************/

void  PrintSTAUD_IPGetStreamInfo(   STAUD_Handle_t      Handle,
                                    STAUD_Object_t      InputObject,
                                    STAUD_StreamInfo_t  *StreamInfo_p)
{
    STTBX_Print(("STAUD_IPGetStreamInfo    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Stream Info PTR=%p \n",(void *)Handle,GetObjectNameFromID(InputObject),StreamInfo_p));
}


/************************************************************************************
Name         : PrintSTAUD_IPGetBitBufferFreeSize ()

Description  : Prints all PrintSTAUD_IPGetBitBufferFreeSize  .

************************************************************************************/
void  PrintSTAUD_IPGetBitBufferFreeSize(STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        U32 * Size_p)
{
    STTBX_Print(("STAUD_IPGetBitBufferFreeSize    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Size PTR=%p \n",(void *)Handle,GetObjectNameFromID(InputObject),Size_p));
}


/************************************************************************************
Name         : PrintSTAUD_DRGetSyncOffset  ()

Description  : Prints all PrintSTAUD_DRGetSyncOffset  .

************************************************************************************/

void PrintSTAUD_DRGetSyncOffset (   STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    S32 *Offset_p)
{
    STTBX_Print(("STAUD_DRGetSyncOffset    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Offset PTR=%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Offset_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRPause  ()

Description  : Prints all PrintSTAUD_DRPause  .

************************************************************************************/
void  PrintSTAUD_DRPause (  STAUD_Handle_t Handle,
                            STAUD_Object_t InputObject,
                            STAUD_Fade_t * Fade_p)
{
    STTBX_Print(("STAUD_DRPause    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n FadeType=%d \n",(void *)Handle,GetObjectNameFromID(InputObject),Fade_p->FadeType));
}

/************************************************************************************
Name         : PrintSTAUD_DRPrepare   ()

Description  : Prints all PrintSTAUD_DRPrepare   .

************************************************************************************/
void  PrintSTAUD_DRPrepare (STAUD_Handle_t Handle,
                            STAUD_Object_t DecoderObject,
                            STAUD_StreamParams_t *StreamParams_p)
{
    STTBX_Print(("STAUD_DRPrepare  [ST_ERROR_FEATURE_NOT_SUPPORTED] \n "));
}


/************************************************************************************
Name         : PrintSTAUD_DRResume   ()

Description  : Prints all PrintSTAUD_DRResume   .

************************************************************************************/

void  PrintSTAUD_DRResume ( STAUD_Handle_t Handle,
                            STAUD_Object_t InputObject)
{
    STTBX_Print(("STAUD_DRResume    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n",(void *)Handle,GetObjectNameFromID(InputObject)));
}


/************************************************************************************
Name         : PrintSTAUD_IPSetBroadcastProfile   ()

Description  : Prints all PrintSTAUD_IPSetBroadcastProfile   .

************************************************************************************/

void  PrintSTAUD_IPSetBroadcastProfile (STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        STAUD_BroadcastProfile_t
                                        BroadcastProfile)
{
    STTBX_Print(("STAUD_IPSetBroadcastProfile    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n BroadcastProfile=%d \n",(void *)Handle,GetObjectNameFromID(InputObject),BroadcastProfile));
}


/************************************************************************************
Name         : PrintSTAUD_IPHandleEOF   ()

Description  : Prints all PrintSTAUD_IPHandleEOF   .

************************************************************************************/
void PrintSTAUD_IPHandleEOF (   STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject)
{
    STTBX_Print(("STAUD_IPHandleEOF    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n",(void *)Handle,GetObjectNameFromID(InputObject)));
}

/************************************************************************************
Name         : PrintSTAUD_GetBeepToneFrequency()

Description  : Prints all PrintSTAUD_GetBeepToneFrequency  .

************************************************************************************/

void  PrintSTAUD_DRGetBeepToneFrequency (   STAUD_Handle_t Handle,
                                            STAUD_Object_t DecoderObject,
                                            U32 * BeepToneFreq_p)
{
    STTBX_Print(("STAUD_DRGetBeepToneFrequency    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Beep Tone  PTR=%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),BeepToneFreq_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetBeepToneFrequency ()

Description  : Prints all PrintSTAUD_DRSetBeepToneFrequency  .

************************************************************************************/

void  PrintSTAUD_DRSetBeepToneFrequency (   STAUD_Handle_t Handle,
                                            STAUD_Object_t DecoderObject,
                                            U32 BeepToneFrequency)
{
    STTBX_Print(("STAUD_DRSetDynamicRangeControl  Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n BeepTone Freq =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),BeepToneFrequency));
}



/************************************************************************************
Name         : PrintSTAUD_DRSetDynamicRangeControl ()

Description  : Prints all PrintSTAUD_DRSetDynamicRangeControl  .

************************************************************************************/

void  PrintSTAUD_DRSetDynamicRangeControl ( STAUD_Handle_t Handle,
                                            STAUD_Object_t DecoderObject,
                                            STAUD_DynamicRange_t * DynamicRange_p)
{
    STTBX_Print(("STAUD_DRSetDynamicRangeControl     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ** DRNG Params \n ",(void *)Handle,GetObjectNameFromID(DecoderObject)));
    STTBX_Print(("\t Apply =%d \n \t Cut =%d \n \t Boost =%d \n",DynamicRange_p->Enable,DynamicRange_p->CutFactor,
                        DynamicRange_p->BoostFactor));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetDownMix ()

Description  : Prints all PrintSTAUD_DRSetDownMix  .

************************************************************************************/

void PrintSTAUD_DRSetDownMix (  STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject,
                                STAUD_DownmixParams_t * DownMixParam_p)
{
    STTBX_Print(("STAUD_DRSetDownMix    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ** Down Mix Params \n ",(void *)Handle,GetObjectNameFromID(DecoderObject)));
    STTBX_Print(("\t Apply=%d \n \t stereoUpmix=%d \n \t monoUpmix=%d \n", DownMixParam_p->Apply,
                        DownMixParam_p->stereoUpmix,DownMixParam_p->monoUpmix));
    STTBX_Print(("\t meanSurround=%d \n \t secondStereo=%d \n \t normalize=%d \n", DownMixParam_p->meanSurround,
                        DownMixParam_p->secondStereo,DownMixParam_p->normalize));
    STTBX_Print(("\t normalizeTableIndex=%d \n \t dialogEnhance=%d \n", DownMixParam_p->normalizeTableIndex,
                        DownMixParam_p->dialogEnhance));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetSpeed ()

Description  : Prints all PrintSTAUD_DRSetSpeed  .

************************************************************************************/

void PrintSTAUD_DRSetSpeed (STAUD_Handle_t Handle,
                            STAUD_Object_t DecoderObject,
                            S32 Speed)
{
    STTBX_Print(("STAUD_DRSetSpeed    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Speed =%d \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Speed));
}


/************************************************************************************
Name         : PrintSTAUD_DRGetSamplingFrequency  ()

Description  : Prints all PrintSTAUD_DRGetSamplingFrequency  .

************************************************************************************/

void PrintSTAUD_DRGetSamplingFrequency (STAUD_Handle_t Handle,
STAUD_Object_t InputObject, U32 * SamplingFrequency_p)
{
    STTBX_Print(("STAUD_DRGetSamplingFrequency    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n FReq  PTR=%p \n",(void *)Handle,GetObjectNameFromID(InputObject),SamplingFrequency_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetInitParams  ()

Description  : Prints all PrintSTAUD_DRSetInitParams  .

************************************************************************************/

void  PrintSTAUD_DRSetInitParams (  STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject,
                                    STAUD_DRInitParams_t * InitParams)
{
    STTBX_Print(("STAUD_DRSetInitParams    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n SBR =%d \n",(void *)Handle,GetObjectNameFromID(InputObject),InitParams->SBRFlag));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetStreamInfo  ()

Description  : Prints all PrintSTAUD_DRGetStreamInfo  .

************************************************************************************/
void  PrintSTAUD_DRGetStreamInfo (  STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    STAUD_StreamInfo_t * StreamInfo_p)
{
    STTBX_Print(("STAUD_DRGetStreamInfo    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n StreamInfo PTR =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),StreamInfo_p));
}


/************************************************************************************
Name         : PrintSTAUD_IPSetStreamIDo  ()

Description  : Prints all PrintSTAUD_IPSetStreamID  .

************************************************************************************/

void PrintSTAUD_IPSetStreamID ( STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject,
                                U8 StreamID)
{
    STTBX_Print(("STAUD_IPSetStreamID    Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Stream ID =%d \n",(void *)Handle,GetObjectNameFromID(InputObject),StreamID));
}


/************************************************************************************
Name         : PrintSTAUD_IPGetSynchroUnit   ()

Description  : Prints all PrintSTAUD_IPGetSynchroUnit  .

************************************************************************************/

void  PrintSTAUD_IPGetSynchroUnit ( STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject,
                                    STAUD_SynchroUnit_t *SynchroUnit_p)
{
    STTBX_Print(("STAUD_IPGetSynchroUnit     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Synchro PTR =%p \n",(void *)Handle,GetObjectNameFromID(InputObject),SynchroUnit_p));
}


/************************************************************************************
Name         : PrintSTAUD_IPSkipSynchro   ()

Description  : Prints all PrintSTAUD_IPSkipSynchro  .

************************************************************************************/
void PrintSTAUD_IPSkipSynchro ( STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject,
                                U32 Delay)
{
    STTBX_Print(("STAUD_IPSkipSynchro     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Delay=%d \n",(void *)Handle,GetObjectNameFromID(InputObject),Delay));
}


/************************************************************************************
Name         : PrintSTAUD_IPPauseSynchro   ()

Description  : Prints all PrintSTAUD_IPSkipSynchro  .

************************************************************************************/

void PrintSTAUD_IPPauseSynchro (STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject,
                                U32 Delay)
{
    STTBX_Print(("STAUD_IPPauseSynchro     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Delay=%d \n",(void *)Handle,GetObjectNameFromID(InputObject),Delay));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetWMAStreamID   ()

Description  : Prints all PrintSTAUD_IPSetWMAStreamID  .

************************************************************************************/

void PrintSTAUD_IPSetWMAStreamID(STAUD_Handle_t Handle,
                                 STAUD_Object_t InputObject,
                                 U8 WMAStreamNumber)
{
    STTBX_Print(("STAUD_IPSetWMAStreamID     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n WMA Stream ID=%d \n",(void *)Handle,GetObjectNameFromID(InputObject),WMAStreamNumber));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetClockRecoverySource  ()

Description  : Prints all PrintSTAUD_DRSetClockRecoverySource  .

************************************************************************************/
#ifndef STAUD_REMOVE_CLKRV_SUPPORT
void PrintSTAUD_DRSetClockRecoverySource (  STAUD_Handle_t Handle,
                                            STAUD_Object_t Object,
                                            STCLKRV_Handle_t ClkSource)
{
    STTBX_Print(("STAUD_DRSetClockRecoverySource     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n CLKRV Handle=%p \n", (void *)Handle,GetObjectNameFromID(Object),(void *)ClkSource));
}
#endif // Added Because of 8010 //


/************************************************************************************
Name         : PrintSTAUD_OPSetEncDigitalOutput  ()

Description  : Prints all PrintSTAUD_OPSetEncDigitalOutput  .

************************************************************************************/
void  PrintSTAUD_OPSetEncDigitalOutput( const STAUD_Handle_t Handle,
                                        STAUD_Object_t OutputObject,
                                        BOOL EnableDisableEncOutput)
{
    STTBX_Print(("STAUD_OPSetEncDigitalOutput     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Encoder Out=%d \n", (void *)Handle,GetObjectNameFromID(OutputObject),EnableDisableEncOutput));
}

#if defined(ST_7200)

/************************************************************************************
Name         : PrintSTAUD_OPSetAnalogMode   ()

Description  : Prints all PrintSTAUD_OPSetAnalogMode  .

************************************************************************************/

void  PrintSTAUD_OPSetAnalogMode (  STAUD_Handle_t Handle,STAUD_Object_t OutputObject,
                                    STAUD_PCMMode_t OutputMode)
{
    STTBX_Print(("STAUD_OPSetAnalogMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n PCM Mode =%d \n", (void *)Handle,GetObjectNameFromID(OutputObject),OutputMode));
}

/************************************************************************************
Name         : PrintSTAUD_OPGetAnalogMode   ()

Description  : Prints all PrintSTAUD_OPGetAnalogMode  .

************************************************************************************/


void PrintSTAUD_OPGetAnalogMode (   STAUD_Handle_t Handle,STAUD_Object_t OutputObject,
                                    STAUD_PCMMode_t * OutputMode)
{
    STTBX_Print(("STAUD_OPGetAnalogMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n PCM Mode PTR =%p \n", (void *)Handle,GetObjectNameFromID(OutputObject),OutputMode));
}

#endif

/************************************************************************************
Name         : PrintSTAUD_OPSetDigitalMode    ()

Description  : Prints all PrintSTAUD_OPSetDigitalMode  .

************************************************************************************/

void PrintSTAUD_OPSetDigitalMode (  STAUD_Handle_t Handle,
                                    STAUD_Object_t OutputObject,
                                    STAUD_DigitalMode_t OutputMode)
{
    STTBX_Print(("STAUD_OPSetDigitalMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n SPDIF Out Mode =%d \n", (void *)Handle,GetObjectNameFromID(OutputObject),OutputMode));
}


/************************************************************************************
Name         : PrintSTAUD_OPGetDigitalMode    ()

Description  : Prints all PrintSTAUD_OPGetDigitalMode  .

************************************************************************************/

void PrintSTAUD_OPGetDigitalMode (  STAUD_Handle_t Handle,
                                    STAUD_Object_t OutputObject,
                                    STAUD_DigitalMode_t * OutputMode)
{
    STTBX_Print(("STAUD_OPGetDigitalMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n SPDIF Mode PTR =%p \n", (void *)Handle,GetObjectNameFromID(OutputObject),OutputMode));
}
/************************************************************************************
Name         : PrintSTAUD_DRSetSyncOffset    ()

Description  : Prints all PrintSTAUD_DRSetSyncOffset  .

************************************************************************************/

void  PrintSTAUD_DRSetSyncOffset (  STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    S32 Offset)
{
    STTBX_Print(("STAUD_DRSetSyncOffset     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Offset =%d \n", (void *)Handle,GetObjectNameFromID(DecoderObject),Offset));
}


/************************************************************************************
Name         : PrintSTAUD_OPSetLatency    ()

Description  : Prints all PrintSTAUD_OPSetLatency  .

************************************************************************************/


void PrintSTAUD_OPSetLatency (  STAUD_Handle_t Handle,
                                STAUD_Object_t OutputObject, U32 Latency)
{
    STTBX_Print(("STAUD_OPSetLatency     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Latency =%d \n", (void *)Handle,GetObjectNameFromID(OutputObject),Latency));
}


/************************************************************************************
Name         : PrintSTAUD_DRStart    ()

Description  : Prints all PrintSTAUD_DRStart  .

************************************************************************************/

void PrintSTAUD_DRStart (   STAUD_Handle_t Handle,
                            STAUD_Object_t DecoderObject,
                            STAUD_StreamParams_t * StreamParams_p)
{
    STTBX_Print(("STAUD_DRStart     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ** Start Params ** \n", (void *)Handle,GetObjectNameFromID(DecoderObject)));
    STTBX_Print(("\t StreamType=%d \n \t StreamContents=%d \n \t Freq=%d \n \t StreamId=%d \n",StreamParams_p->StreamType,
                        StreamParams_p->StreamContent,StreamParams_p->SamplingFrequency,StreamParams_p->StreamID));
}

/************************************************************************************
Name         : PrintSTAUD_DRStop     ()

Description  : Prints all PrintSTAUD_DRStop  .

************************************************************************************/

void PrintSTAUD_DRStop (STAUD_Handle_t Handle,
                        STAUD_Object_t DecoderObject,
                        STAUD_Stop_t StopMode, STAUD_Fade_t * Fade_p)
{
    STTBX_Print(("STAUD_DRStop     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n StopMode=%s \n", (void *)Handle,GetObjectNameFromID(DecoderObject),
                        (StopMode==STAUD_STOP_WHEN_END_OF_DATA? "Stop end of data":"Stop Now")));
    STTBX_Print((" Fade Type=%d \n",Fade_p->FadeType));
}

/************************************************************************************
Name         : PrintSTAUD_IPGetInputBufferParams     ()

Description  : Prints all PrintSTAUD_IPGetInputBufferParams  .

************************************************************************************/
void PrintSTAUD_IPGetInputBufferParams( STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        STAUD_BufferParams_t* DataParams_p)
{
    STTBX_Print(("STAUD_IPGetInputBufferParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Data Buf PTR =%p \n", (void *)Handle,GetObjectNameFromID(InputObject),DataParams_p));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetDataInputInterface     ()

Description  : Prints all PrintSTAUD_IPSetDataInputInterface  .

************************************************************************************/

void  PrintSTAUD_IPSetDataInputInterface(   STAUD_Handle_t Handle,
                                            STAUD_Object_t InputObject,
                                            GetWriteAddress_t   GetWriteAddress_p,
                                            InformReadAddress_t InformReadAddress_p,
                                            void * const BufferHandle_p)
{
    STTBX_Print(("STAUD_IPGetInputBufferParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ReadFn PTR =%p \n ", (void *)Handle,GetObjectNameFromID(InputObject),InformReadAddress_p));
    STTBX_Print(("WriteFn PTR =%p \n Handle =%p \n", InformReadAddress_p,BufferHandle_p));
}

/************************************************************************************
Name         : PrintSTAUD_IPGetCapability     ()

Description  : Prints all PrintSTAUD_IPGetCapability .

************************************************************************************/

void PrintSTAUD_IPGetCapability(const ST_DeviceName_t DeviceName,
                                STAUD_Object_t InputObject,
                                STAUD_IPCapability_t * Capability_p)
{
    STTBX_Print(("STAUD_IPGetCapability     Params \n "));
    STTBX_Print(("Device Name =%s \n Object=%s \n CapabiltyPTR =%p \n", DeviceName,GetObjectNameFromID(InputObject),Capability_p));
}


/************************************************************************************
Name         : PrintSTAUD_IPGetDataInterfaceParams

Description  : Prints all PrintSTAUD_IPGetDataInterfaceParams .

************************************************************************************/

void  PrintSTAUD_IPGetDataInterfaceParams ( STAUD_Handle_t Handle,
                                            STAUD_Object_t InputObject,
                                            STAUD_DataInterfaceParams_t *DMAParams_p)
{
    STTBX_Print(("STAUD_IPGetDataInterfaceParams  [ST_ERROR_FEATURE_NOT_SUPPORTED] \n "));
}


/************************************************************************************
Name         : PrintSTAUD_IPGetParams

Description  : Prints all PrintSTAUD_IPGetParams .

************************************************************************************/
void  PrintSTAUD_IPGetParams (  STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject,
                                STAUD_InputParams_t * InputParams_p)
{
    STTBX_Print(("STAUD_IPGetParams  [ST_ERROR_FEATURE_NOT_SUPPORTED] \n "));
}

/************************************************************************************
Name         : PrintSTAUD_IPQueuePCMBuffer

Description  : Prints all PrintSTAUD_IPQueuePCMBuffer .

************************************************************************************/

void PrintSTAUD_IPQueuePCMBuffer (  STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject,
                                    STAUD_PCMBuffer_t * PCMBuffer_p,
                                    U32 NumBuffers, U32 * NumQueued_p)
{
    STTBX_Print(("STAUD_IPQueuePCMBuffer     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n NumQueued_p PTR =%p \n ", (void *)Handle,GetObjectNameFromID(InputObject),NumQueued_p));
    STTBX_Print(("NumBuffers =%d \n PCMBuffer Block =%d \n PCMBuffer Offset =%d \n PCM Buffer Length=%d \n",
                        NumBuffers,PCMBuffer_p->Block,PCMBuffer_p->StartOffset,PCMBuffer_p->Length));

}

/************************************************************************************
Name         : PrintSTAUD_IPGetPCMBuffer

Description  : Prints all PrintSTAUD_IPGetPCMBuffer .

************************************************************************************/
void  PrintSTAUD_IPGetPCMBuffer (   STAUD_Handle_t Handle,
                                    STAUD_Object_t InputObject,
                                    STAUD_PCMBuffer_t * PCMBuffer_p)
{
    STTBX_Print(("STAUD_IPGetPCMBuffer     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Buffer PTR =%p \n ", (void *)Handle,GetObjectNameFromID(InputObject),PCMBuffer_p));
}

/************************************************************************************
Name         : PrintSTAUD_IPGetPCMBufferSize

Description  : Prints all PrintSTAUD_IPGetPCMBufferSize .

************************************************************************************/
void  PrintSTAUD_IPGetPCMBufferSize (   STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        U32 * BufferSize_p)
{
    STTBX_Print(("STAUD_IPGetPCMBufferSize     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n BufferSize PTR =%p \n ",(void *)Handle,GetObjectNameFromID(InputObject),BufferSize_p));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetLowDataLevelEvent

Description  : Prints all PrintSTAUD_IPSetLowDataLevelEvent .

************************************************************************************/
void PrintSTAUD_IPSetLowDataLevelEvent (STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        U8             Level)
{
    STTBX_Print(("STAUD_IPSetLowDataLevelEvent     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Level  =%d \n ",(void *)Handle,GetObjectNameFromID(InputObject),Level));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetParams

Description  : Prints all PrintSTAUD_IPSetParams .

************************************************************************************/
void  PrintSTAUD_IPSetParams (  STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject,
                                STAUD_InputParams_t * InputParams_p)
{
    STTBX_Print(("STAUD_IPSetParams[ERROR] FEATURE NOT SUPPORTED \n "));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetPCMParams

Description  : Prints all PrintSTAUD_IPSetPCMParams .

************************************************************************************/

void PrintSTAUD_IPSetPCMParams (STAUD_Handle_t Handle,
                                STAUD_Object_t InputObject,
                                STAUD_PCMInputParams_t * InputParams_p)
{
    STTBX_Print(("STAUD_IPSetPCMParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Freq  =%d \n DataPrecision =%d \n NumChannel=%d \n",
                        (void *)Handle,GetObjectNameFromID(InputObject),InputParams_p->Frequency,
                        InputParams_p->DataPrecision,InputParams_p->NumChannels));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetPCMReaderParams

Description  : Prints all PrintSTAUD_IPSetPCMParams .
************************************************************************************/
void PrintSTAUD_IPSetPCMReaderParams(   STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        STAUD_PCMReaderConf_t *InputParams_p)
{
    STTBX_Print(("STAUD_IPSetPCMReaderParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(InputObject)));

    STTBX_Print(("** PCM Reader Params ** \n  "));

    STTBX_Print(("\t MSBFirst =%d \n \t DAC AlignMent  =%s \n \t Padding =%d  \n \t FallingSCLK =%d \n",
                        InputParams_p->MSBFirst,(InputParams_p->Alignment==0?"Left ":"Right"),
                        InputParams_p->Padding, InputParams_p->FallingSCLK));

    STTBX_Print(("\t LeftLRHigh =%d \n \t DAC BitsPerSubFrame  =%s \n \t Precision =%d  \n ",
                        InputParams_p->LeftLRHigh,(InputParams_p->BitsPerSubFrame==0?"32 Bits ":"16 Bits"),
                        InputParams_p->Precision));

    STTBX_Print(("\t Frequency =%d \n \t  Rounding  =%s \n \t MemFormat= %s  \n ",
                        InputParams_p->Frequency,(InputParams_p->Rounding==0?"No Rounding ":"16 Bits Rounding"),
                        (InputParams_p->MemFormat==0?"16_0" : "16_16")));

}

/************************************************************************************
Name         : PrintSTAUD_IPSetPCMReaderParams

Description  : Prints all PrintSTAUD_IPSetPCMParams .
************************************************************************************/
void PrintSTAUD_IPGetPCMReaderParams(   STAUD_Handle_t Handle,
                                        STAUD_Object_t InputObject,
                                        STAUD_PCMReaderConf_t *InputParams_p)
{
    STTBX_Print(("STAUD_IPGetPCMReaderParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Params PTR=%p \n ",(void *)Handle,GetObjectNameFromID(InputObject),InputParams_p));
}

/************************************************************************************
Name         : PrintSTAUD_IPSetPCMReaderParams

Description  : Prints all PrintSTAUD_IPSetPCMParams .
************************************************************************************/
void PrintSTAUD_IPGetPCMReaderCapability(   STAUD_Handle_t Handle,
                                            STAUD_Object_t InputObject,
                                            STAUD_ReaderCapability_t *InputParams_p)
{
    STTBX_Print(("STAUD_IPGetPCMReaderCapability     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Params PTR=%p \n ",(void *)Handle,GetObjectNameFromID(InputObject),InputParams_p));
}

/************************************************************************************
Name         : PrintSTAUD_MXConnectSource

Description  : Prints all PrintSTAUD_MXConnectSource .
************************************************************************************/

void PrintSTAUD_MXConnectSource (   STAUD_Handle_t Handle,
                                    STAUD_Object_t MixerObject,
                                    STAUD_Object_t *InputSources_p,
                                    STAUD_MixerInputParams_t * InputParams_p,
U32 NumInputs)
{
    STTBX_Print(("STAUD_MXConnectSource[ERROR] FEATURE NOT SUPPORTED \n "));
}


/************************************************************************************
Name         : PrintSTAUD_MXGetCapability

Description  : Prints all PrintSTAUD_MXGetCapability .
************************************************************************************/
void PrintSTAUD_MXGetCapability(const ST_DeviceName_t DeviceName,
                                STAUD_Object_t MixerObject,
                                STAUD_MXCapability_t * Capability_p)
{
    STTBX_Print(("STAUD_MXGetCapability     Params \n "));
    STTBX_Print(("Device =%s \n Mixer Object=%s \n Cap PTR=%p \n ", DeviceName,GetObjectNameFromID(MixerObject),Capability_p));
}


/************************************************************************************
Name         : PrintSTAUD_MXSetInputParams

Description  : Prints all PrintSTAUD_MXSetInputParams .
************************************************************************************/

void PrintSTAUD_MXSetInputParams (  STAUD_Handle_t Handle,
                                    STAUD_Object_t MixerObject,
                                    STAUD_Object_t InputSource,
                                    STAUD_MixerInputParams_t *
                                    InputParams_p)
{
    STTBX_Print(("STAUD_MXSetInputParams[ERROR] FEATURE NOT SUPPORTED \n "));
}
/************************************************************************************
Name         : PrintSTAUD_PPGetAttenuation

Description  : Prints all PrintSTAUD_PPGetAttenuation .
************************************************************************************/

void PrintSTAUD_PPGetAttenuation (  STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProcObject,
                                    STAUD_Attenuation_t * Attenuation_p)
{
    STTBX_Print(("STAUD_PPGetAttenuation     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Atte PTR=%p \n ",(void *)Handle,GetObjectNameFromID(PostProcObject),Attenuation_p));
}

/************************************************************************************
Name         : PrintSTAUD_PPGetEqualizationParams

Description  : Prints all PrintSTAUD_PPGetEqualizationParams .
************************************************************************************/

void PrintSTAUD_PPGetEqualizationParams(STAUD_Handle_t Handle,
STAUD_Object_t PostProcObject,
STAUD_Equalization_t *Equalization_p)
{
    STTBX_Print(("STAUD_PPGetEqualizationParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n EQ PTR=%p \n ",(void *)Handle,GetObjectNameFromID(PostProcObject),Equalization_p));
}


/************************************************************************************
Name         : PrintSTAUD_OPGetCapability

Description  : Prints all PrintSTAUD_OPGetCapability .
************************************************************************************/
void PrintSTAUD_OPGetCapability(const ST_DeviceName_t DeviceName,
                                STAUD_Object_t OutputObject,
                                STAUD_OPCapability_t * Capability_p)
{
    STTBX_Print(("STAUD_OPGetCapability     Params \n "));
    STTBX_Print(("Device =%s \n Object=%s \n Cap PTR=%p \n ", DeviceName,GetObjectNameFromID(OutputObject),Capability_p));
}


/************************************************************************************
Name         : PrintSTAUD_PPGetDelay

Description  : Prints all PrintSTAUD_PPGetDelay .
************************************************************************************/

void  PrintSTAUD_PPGetDelay (   STAUD_Handle_t Handle,
                                STAUD_Object_t PostProcObject,
                                STAUD_Delay_t *Delay_p)
{
    STTBX_Print(("STAUD_PPGetDelay     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Delay PTR=%p \n ",(void *)Handle,GetObjectNameFromID(PostProcObject),Delay_p));
}


/************************************************************************************
Name         : PrintSTAUD_OPGetParams

Description  : Prints all PrintSTAUD_OPGetParams .
************************************************************************************/
void PrintSTAUD_OPGetParams (   STAUD_Handle_t Handle,
                                STAUD_Object_t OutputObject,
                                STAUD_OutputParams_t * Params_p)
{
    STTBX_Print(("STAUD_OPGetParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Params PTR=%p \n ", (void *)Handle,GetObjectNameFromID(OutputObject),(void *)Params_p));
}

/************************************************************************************
Name         : PrintSTAUD_PPGetSpeakerEnable

Description  : Prints all PrintSTAUD_PPGetSpeakerEnable .
************************************************************************************/

void  PrintSTAUD_PPGetSpeakerEnable (   STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_SpeakerEnable_t *Speaker_p)
{
    STTBX_Print(("STAUD_PPGetSpeakerEnable[ERROR] FEATURE NOT SUPPORTED \n "));
}


/************************************************************************************
Name         : PrintSTAUD_PPGetSpeakerEnable

Description  : Prints all PrintSTAUD_PPGetSpeakerEnable .
************************************************************************************/

void  PrintSTAUD_PPGetSpeakerConfig (   STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_SpeakerConfiguration_t *SpeakerConfig_p)
{
    STTBX_Print(("STAUD_PPGetSpeakerConfig     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Params PTR=%p \n ", (void *)Handle,GetObjectNameFromID(PostProcObject),(void *)SpeakerConfig_p));
}

/************************************************************************************
Name         : PrintSTAUD_OPMute

Description  : Prints all PrintSTAUD_OPMute .
************************************************************************************/

void PrintSTAUD_OPMute (STAUD_Handle_t Handle,
                        STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_OPMute     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(OutputObject)));
}

/************************************************************************************
Name         : PrintSTAUD_DRMute

Description  : Prints all PrintSTAUD_DRMute .
************************************************************************************/

void PrintSTAUD_DRMute (STAUD_Handle_t Handle,
                        STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_DRMute     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(OutputObject)));
}


/************************************************************************************
Name         : PrintSTAUD_MXSetMixLevel

Description  : Prints all PrintSTAUD_MXSetMixLevel .
************************************************************************************/

void PrintSTAUD_MXSetMixLevel ( STAUD_Handle_t Handle,
                                STAUD_Object_t MixerObject,
                                U32 InputID, U16 MixLevel)
{
    STTBX_Print(("STAUD_MXSetMixLevel     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Input ID =%d \n MixLevel =%d \n",(void *)Handle,GetObjectNameFromID(MixerObject),InputID,MixLevel));
}

/************************************************************************************
Name         : PrintSTAUD_PPSetAttenuation

Description  : Prints all PrintSTAUD_MXSetMixLevel .
************************************************************************************/

void PrintSTAUD_PPSetAttenuation (  STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProcObject,
                                    STAUD_Attenuation_t * Attenuation_p)
{
    STTBX_Print(("STAUD_PPSetAttenuation      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(PostProcObject)));
    STTBX_Print(("Left =%d \n Right =%d \n Center =%d \n SubW =%d \n ", Attenuation_p->Left,
                        Attenuation_p->Right,Attenuation_p->Center,Attenuation_p->Subwoofer));

    STTBX_Print(("Left W =%d \n Right W =%d \n CS Left  =%d \n Cs Right =%d \n ", Attenuation_p->LeftSurround,
                        Attenuation_p->RightSurround,Attenuation_p->CsLeft,Attenuation_p->CsRight));

    STTBX_Print(("VCRL=%d \n VCRR =%d \n  ", Attenuation_p->VcrLeft,
                        Attenuation_p->VcrRight));
}

/************************************************************************************
Name         : PrintSTAUD_PPSetDelay

Description  : Prints all PrintSTAUD_PPSetDelay .
************************************************************************************/

void  PrintSTAUD_PPSetDelay (   STAUD_Handle_t Handle,
                                STAUD_Object_t PostProcObject,
                                STAUD_Delay_t * Delay_p)
{
    STTBX_Print(("STAUD_PPSetDelay      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(PostProcObject)));

    STTBX_Print(("Left =%d \n Right =%d \n Center =%d \n SubW =%d \n ", Delay_p->Left,
                        Delay_p->Right,Delay_p->Center,Delay_p->Subwoofer));

    STTBX_Print(("Left W =%d \n Right W =%d \n CS Left  =%d \n Cs Right =%d \n ", Delay_p->LeftSurround,
                        Delay_p->RightSurround,Delay_p->CsLeft,Delay_p->CsRight));
}


/************************************************************************************
Name         : PrintSTAUD_PPSetEqualizationParams

Description  : Prints all PrintSTAUD_PPSetEqualizationParams .
************************************************************************************/

void PrintSTAUD_PPSetEqualizationParams(STAUD_Handle_t Handle,
                                        STAUD_Object_t PostProcObject,
                                        STAUD_Equalization_t *Equalization_p)
{
    STTBX_Print(("STAUD_PPSetEqualizationParams      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(PostProcObject)));
    STTBX_Print(("Enable =%d \n Band 0 =%d \n Band 1 =%d \n Band 2 =%d \n ", Equalization_p->enable,
                        Equalization_p->equalizerBand_0,Equalization_p->equalizerBand_1,Equalization_p->equalizerBand_2));

    STTBX_Print(("Band 3=%d \n Band 4 =%d \n Band 5 =%d \n Band 6 =%d \n Band 7 =%d \n ", Equalization_p->equalizerBand_3,
                        Equalization_p->equalizerBand_4,Equalization_p->equalizerBand_5,Equalization_p->equalizerBand_6,
                        Equalization_p->equalizerBand_7));
}

/************************************************************************************
Name         : PrintSTAUD_DPSetCallback

Description  : Prints all PrintSTAUD_DPSetCallback .
************************************************************************************/

void  PrintSTAUD_DPSetCallback (STAUD_Handle_t Handle,
                                STAUD_Object_t DPObject,
                                FrameDeliverFunc_t Func_fp,
                                void * clientInfo)
{
    STTBX_Print(("STAUD_DPSetCallback      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(DPObject)));
    STTBX_Print(("Func PTR =%p \n clientInfo=%p \n ",Func_fp,clientInfo));
}

/************************************************************************************
Name         : PrintSTAUD_OPSetParams

Description  : Prints all PrintSTAUD_OPSetParams .
************************************************************************************/

void  PrintSTAUD_OPSetParams (  STAUD_Handle_t Handle,
                                STAUD_Object_t OutputObject,
                                STAUD_OutputParams_t * Params_p)
{
    STTBX_Print(("STAUD_OPSetParams      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(OutputObject)));

    if (OutputObject ==STAUD_OBJECT_OUTPUT_PCMP0 || OutputObject ==STAUD_OBJECT_OUTPUT_PCMP1 ||
    OutputObject ==STAUD_OBJECT_OUTPUT_PCMP2 || OutputObject ==STAUD_OBJECT_OUTPUT_PCMP3||
    OutputObject ==STAUD_OBJECT_OUTPUT_HDMI_PCMP0)
    {

        STTBX_Print((" ** PCM Player Config ** \n"));

        STTBX_Print((" \t InvetWordClock =%d \n \t InvertBit Clock=%d \n \t DACFormat =%s \n", Params_p->PCMOutParams.InvertWordClock,
                            Params_p->PCMOutParams.InvertBitClock, (Params_p->PCMOutParams.Format==0? "I2S": "Standard")));

        STTBX_Print(("\t DACDataPrecision =%d \n \t DACDataAlignment =%d \n \t MBSFirst =%d \n",
                            Params_p->PCMOutParams.Precision,Params_p->PCMOutParams.Alignment, Params_p->PCMOutParams.MSBFirst));


        STTBX_Print(("\t Freq Mutliplier =%d \n \t MemoryFormat =%s \n ",
                            Params_p->PCMOutParams.PcmPlayerFrequencyMultiplier,(Params_p->PCMOutParams.MemoryStorageFormat16==0?"32Bit":"16 bit")));

    }
    else if (OutputObject==STAUD_OBJECT_OUTPUT_SPDIF0|| OutputObject== STAUD_OBJECT_OUTPUT_HDMI_SPDIF0)
    {
        STTBX_Print((" ** SPDIF Player Config ** \n"));

        STTBX_Print((" \t AutoLatency =%d \n \t Latency Clock =%d \n \t CopyPermitted =%d \n", Params_p->SPDIFOutParams.AutoLatency,
                            Params_p->SPDIFOutParams.Latency, Params_p->SPDIFOutParams.CopyPermitted));

        STTBX_Print(("\t AutoCategoryCode =%d \n \t CategoryCode =%d \n \t AutoDTDI =%d \n",
                            Params_p->SPDIFOutParams.AutoCategoryCode,Params_p->SPDIFOutParams.CategoryCode, Params_p->SPDIFOutParams.AutoDTDI));


        STTBX_Print(("\t DTDI =%d \n \t Emphasis =%s \n \t SPDIFDataPrecisionPCMMode= %d  \n \t SPDIFPlayerFrequencyMultiplier =%d \n ",
                            Params_p->SPDIFOutParams.DTDI,(Params_p->SPDIFOutParams.Emphasis==0?"No Emph":"EMPH CD"),
                            Params_p->SPDIFOutParams.SPDIFDataPrecisionPCMMode, Params_p->SPDIFOutParams.SPDIFPlayerFrequencyMultiplier));
    }

}

/************************************************************************************
Name         : PrintSTAUD_PPSetSpeakerEnable

Description  : Prints all PrintSTAUD_PPSetSpeakerEnable .
************************************************************************************/

void  PrintSTAUD_PPSetSpeakerEnable (   STAUD_Handle_t Handle,
                                        STAUD_Object_t OutputObject,
                                        STAUD_SpeakerEnable_t * Speaker_p)
{
    STTBX_Print(("STAUD_PPSetSpeakerEnable[ERROR] Feature Not Supported  \n "));
}


/************************************************************************************
Name         : PrintSTAUD_PPSetSpeakerConfig

Description  : Prints all PrintSTAUD_PPSetSpeakerConfig .
************************************************************************************/

void PrintSTAUD_PPSetSpeakerConfig (STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProcObject,
                                    STAUD_SpeakerConfiguration_t *
                                    SpeakerConfig_p, STAUD_BassMgtType_t BassType)
{
    STTBX_Print(("STAUD_PPSetSpeakerConfig      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(PostProcObject)));
    STTBX_Print(("Front =%d \n Center=%d \n LeftS =%d \n RightS =%d \n",
                        SpeakerConfig_p->Front, SpeakerConfig_p->Center, SpeakerConfig_p->LeftSurround, SpeakerConfig_p->RightSurround));
    STTBX_Print((" CsLeft=%d \n CsRight=%d \n VcrLeft=%d \n VcrR=%d \n LFE=%d \n  BassType=%d \n", SpeakerConfig_p->CsLeft,
                        SpeakerConfig_p->CsRight,SpeakerConfig_p->VcrLeft,SpeakerConfig_p->VcrRight,SpeakerConfig_p->LFEPresent,BassType));
}


/************************************************************************************
Name         : PrintSTAUD_OPUnMute

Description  : Prints all PrintSTAUD_OPUnMute .
************************************************************************************/
void  PrintSTAUD_OPUnMute ( STAUD_Handle_t Handle,
                            STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_OPUnMute     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(OutputObject)));
}
/************************************************************************************
Name         : PrintSTAUD_DRUnMute

Description  : Prints all PrintSTAUD_DRUnMute .
************************************************************************************/

void  PrintSTAUD_DRUnMute ( STAUD_Handle_t Handle,
                            STAUD_Object_t OutputObject)
{
    STTBX_Print(("STAUD_DRUnMute     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(OutputObject)));
}

/************************************************************************************
Name         : PrintSTAUD_PPConnectSource

Description  : Prints all PrintSTAUD_PPConnectSource .
************************************************************************************/

void PrintSTAUD_PPConnectSource (   STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProcObject,
                                    STAUD_Object_t InputSource)
{
    STTBX_Print(("STAUD_PPSetSpeakerEnable[ERROR] Feature Not Supported  \n "));
}

/************************************************************************************
Name         : PrintSTAUD_MXGetMixLevel

Description  : Prints all PrintSTAUD_MXGetMixLevel
************************************************************************************/

void PrintSTAUD_MXGetMixLevel ( STAUD_Handle_t Handle,
                                STAUD_Object_t MixerObject,
                                U32 InputID, U16 *MixLevel_p)
{
    STTBX_Print(("STAUD_MXGetMixLevel     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Input ID =%d \n MixLevel PTR =%p \n",(void *)Handle,GetObjectNameFromID(MixerObject),InputID,MixLevel_p));
}

/************************************************************************************
Name         : PrintSTAUD_PPGetCapability

Description  : Prints all PrintSTAUD_PPGetCapability
************************************************************************************/
void PrintSTAUD_PPGetCapability(const ST_DeviceName_t DeviceName,
                                STAUD_Object_t PostProcObject,
                                STAUD_PPCapability_t * Capability_p)
{
    STTBX_Print(("STAUD_PPGetCapability     Params \n "));
    STTBX_Print(("Device =%s \n Object=%s \n cap PTR  =%p \n", DeviceName,GetObjectNameFromID(PostProcObject),Capability_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetCircleSurroundParams

Description  : Prints all PrintSTAUD_DRGetCircleSurroundParams
************************************************************************************/

void  PrintSTAUD_DRGetCircleSurroundParams (STAUD_Handle_t Handle,
                                            STAUD_Object_t DecoderObject,
                                            STAUD_CircleSurrondII_t * Csii)
{
    STTBX_Print(("STAUD_DRGetCircleSurroundParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n CSII PTR  =%p \n", (void*)Handle,GetObjectNameFromID(DecoderObject),(void*)Csii));
}


/************************************************************************************
Name         : PrintSTAUD_DRGetDeEmphasisFilter

Description  : Prints all PrintSTAUD_DRGetDeEmphasisFilter
************************************************************************************/

void  PrintSTAUD_DRGetDeEmphasisFilter (STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        BOOL *Emphasis_p)
{
    STTBX_Print(("STAUD_DRGetDeEmphasisFilter     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n EMPH PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Emphasis_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetDolbyDigitalEx

Description  : Prints all PrintSTAUD_DRGetDolbyDigitalEx
************************************************************************************/

void  PrintSTAUD_DRGetDolbyDigitalEx (  STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        BOOL * DolbyDigitalEx_p)
{
    STTBX_Print(("STAUD_DRGetDolbyDigitalEx     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n AC3EX PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),DolbyDigitalEx_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetEffect

Description  : Prints all PrintSTAUD_DRGetEffect
************************************************************************************/

void  PrintSTAUD_DRGetEffect (  STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject,
                                STAUD_Effect_t * Effect_p)
{
    STTBX_Print(("STAUD_DRGetDolbyDigitalEx     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Effect PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Effect_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetOmniParams

Description  : Prints all PrintSTAUD_DRGetOmniParams
************************************************************************************/

void  PrintSTAUD_DRGetOmniParams (  STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    STAUD_Omni_t * Omni_p)
{
    STTBX_Print(("STAUD_DRGetOmniParams  Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Omni PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Omni_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetSRSEffectParams

Description  : Prints all PrintSTAUD_DRGetSRSEffectParams
************************************************************************************/

void PrintSTAUD_DRGetSRSEffectParams (  STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_EffectSRSParams_t ParamType,
                                        S16 *Value_p)
{
    STTBX_Print(("STAUD_DRGetSRSEffectParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n Effect=%d \n val PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),ParamType,Value_p));
}

/************************************************************************************
Name         : PrintSTAUD_PPGetKaraoke

Description  : Prints all PrintSTAUD_PPGetKaraoke
************************************************************************************/
void  PrintSTAUD_PPGetKaraoke ( STAUD_Handle_t Handle,
                                STAUD_Object_t PostProcObject,
                                STAUD_Karaoke_t * Karaoke_p)
{
    STTBX_Print(("STAUD_PPGetKaraoke [ERROR] Feature Not Supported  \n "));
}


/************************************************************************************
Name         : PrintSTAUD_DRGetPrologic

Description  : Prints all PrintSTAUD_DRGetPrologic
************************************************************************************/

void PrintSTAUD_DRGetPrologic(  STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject,
                                BOOL * Prologic_p)
{
    STTBX_Print(("STAUD_DRGetPrologic     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Prologic  PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),Prologic_p));
}
/************************************************************************************
Name         : PrintSTAUD_DRGetPrologicAdvance

Description  : Prints all PrintSTAUD_DRGetPrologicAdvance
************************************************************************************/
 void PrintSTAUD_DRGetPrologicAdvance ( STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_PLIIParams_t *PLIIParams)
{
    STTBX_Print(("STAUD_DRGetPrologicAdvance     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  PrologicII  PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),PLIIParams));
}
/************************************************************************************
Name         : PrintSTAUD_DRGetStereoMode

Description  : Prints all PrintSTAUD_DRGetStereoMode
************************************************************************************/

void  PrintSTAUD_DRGetStereoMode (  STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    STAUD_StereoMode_t * StereoMode_p)
{
    STTBX_Print(("STAUD_DRGetStereoMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Stereo  PTR  =%p \n",(void *)Handle,GetObjectNameFromID(DecoderObject),StereoMode_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetCircleSurroundParams

Description  : Prints all PrintSTAUD_DRSetCircleSurroundParams
************************************************************************************/

void  PrintSTAUD_DRSetCircleSurroundParams (STAUD_Handle_t Handle,
                                            STAUD_Object_t DecoderObject,
                                            STAUD_CircleSurrondII_t Csii)
{
    STTBX_Print(("STAUD_DRSetCircleSurroundParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Phanton =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),Csii.Phantom));
    STTBX_Print(("CenterFB =%d \n IS525Mode=%d \n  CenterR =%d \n ", Csii.CenterFB, Csii.IS525Mode, Csii.CenterRear));
    STTBX_Print(("RCenterFB =%d \n TBSub=%d \n  TBFront =%d \n ", Csii.RCenterFB, Csii.TBSub, Csii.TBFront));
    STTBX_Print(("FocusEnable =%d \n TBSize=%d \n  OutMode =%d \n ", Csii.FocusEnable, Csii.TBSize, Csii.OutMode));
    STTBX_Print(("Mode =%d \n InputGain=%d \n  TBLevel =%d \n FocusElevation =%d \n ", Csii.Mode, Csii.InputGain, Csii.TBLevel,Csii.FocusElevation));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetDeEmphasisFilter

Description  : Prints all PrintSTAUD_DRSetDeEmphasisFilter
************************************************************************************/

void  PrintSTAUD_DRSetDeEmphasisFilter (STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        BOOL Emphasis)
{
    STTBX_Print(("STAUD_DRSetDeEmphasisFilter     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Enable =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),Emphasis));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetDolbyDigitalEx

Description  : Prints all PrintSTAUD_DRSetDolbyDigitalEx
************************************************************************************/
void  PrintSTAUD_DRSetDolbyDigitalEx (  STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        BOOL DolbyDigitalEx)
{
    STTBX_Print(("STAUD_DRSetDolbyDigitalEx     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Dolby Ex =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),DolbyDigitalEx));
}


/************************************************************************************
Name         : PrintSTAUD_DRSetEffect

Description  : Prints all PrintSTAUD_DRSetEffect
************************************************************************************/

void  PrintSTAUD_DRSetEffect (STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,STAUD_Effect_t Effect)
{
    STTBX_Print(("STAUD_DRSetEffect     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Effect =%p \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),(void*)Effect));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetOmniParams

Description  : Prints all PrintSTAUD_DRSetOmniParams
************************************************************************************/

void  PrintSTAUD_DRSetOmniParams (  STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    STAUD_Omni_t Omni)
{
    STTBX_Print(("STAUD_DRSetOmniParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  omni Enable =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),Omni.omniEnable));
    STTBX_Print(("InputMode=%d \n SurroundMode=%d \n  Dialog Mode =%d \n ", Omni.omniInputMode, Omni.omniSurroundMode, Omni.omniDialogMode));
    STTBX_Print(("LFE Mode=%d \n Dialog Level=%d \n ", Omni.omniLfeMode, Omni.omniDialogLevel));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetSRSEffectParams

Description  : Prints all PrintSTAUD_DRSetSRSEffectParams
************************************************************************************/
void PrintSTAUD_DRSetSRSEffectParams (  STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_EffectSRSParams_t ParamsType, S16 Value)
{
    STTBX_Print(("STAUD_DRSetSRSEffectParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Effect Type =%p \n Value=%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),(void*)ParamsType,Value));
}
/************************************************************************************
Name         : PrintSTAUD_PPSetKaraoke

Description  : Prints all PrintSTAUD_PPSetKaraoke
************************************************************************************/
void PrintSTAUD_PPSetKaraoke (  STAUD_Handle_t Handle,
                                STAUD_Object_t PostProcObject,
                                STAUD_Karaoke_t Karaoke)
{
    STTBX_Print(("STAUD_PPSetKaraoke[Error] Feature not Supported \n "));
}
/************************************************************************************
Name         : PrintSTAUD_DRSetPrologic

Description  : Prints all PrintSTAUD_DRSetPrologic
************************************************************************************/
void  PrintSTAUD_DRSetPrologic (STAUD_Handle_t Handle,
                                STAUD_Object_t Decoder,BOOL Prologic)
{
    STTBX_Print(("STAUD_DRSetPrologic     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Prologic =%s \n ",(void *)Handle,GetObjectNameFromID(Decoder),(Prologic?"YES":"No")));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetPrologicAdvance

Description  : Prints all PrintSTAUD_DRSetPrologicAdvance
************************************************************************************/
void PrintSTAUD_DRSetPrologicAdvance (  STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_PLIIParams_t PLIIParams)
{
    STTBX_Print(("STAUD_DRSetPrologicAdvance     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Apply =%d \n DecMode=%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),PLIIParams.Apply,PLIIParams.DecMode));
    STTBX_Print(("Dim Control =%d \n CenterWidth=%d \n  Pano =%d \n ChannelPol=%d \n ", PLIIParams.DimensionControl,
                        PLIIParams.CentreWidth, PLIIParams.Panaroma, PLIIParams.ChannelPolarity));
    STTBX_Print(("Surround Filter =%d \n OutMode=%d \n  Pano =%d \n ChannelPol=%d \n ", PLIIParams.SurroundFilter,
                        PLIIParams.OutMode,PLIIParams.Panaroma,PLIIParams.ChannelPolarity));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetStereoMode

Description  : Prints all PrintSTAUD_DRSetStereoMode
************************************************************************************/
void PrintSTAUD_DRSetStereoMode (   STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    STAUD_StereoMode_t StereoMode)
{
    STTBX_Print(("STAUD_DRSetStereoMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  StereoMode =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),StereoMode));
}
/************************************************************************************
Name         : PrintSTAUD_SelectDac

Description  : Prints all PrintSTAUD_SelectDac
************************************************************************************/
extern BOOL Internal_Dacs;
void PrintSTAUD_SelectDac(void)
{
#ifndef ST_51XX
    STTBX_Print(("STAUD_SelectDac Internal DAC=%s \n", (Internal_Dacs?"Internal":"External")));
#endif
}
/************************************************************************************
Name         : PrintSTAUD_PPDcRemove

Description  : Prints all PrintSTAUD_PPDcRemove
************************************************************************************/
void PrintSTAUD_PPDcRemove( STAUD_Handle_t Handle,
                            STAUD_Object_t PostProcObject,
                            STAUD_DCRemove_t *Params_p)
{
    STTBX_Print(("STAUD_PPDcRemove     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  remove =%d \n ",(void *)Handle,GetObjectNameFromID(PostProcObject),*Params_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetCompressionMode

Description  : Prints all PrintSTAUD_DRSetCompressionMode
************************************************************************************/
void  PrintSTAUD_DRSetCompressionMode ( STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_CompressionMode_t CompressionMode_p)
{
    STTBX_Print(("STAUD_DRSetCompressionMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  CompMode =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),CompressionMode_p));
}
/************************************************************************************
Name         : PrintSTAUD_DRGetCompressionMode

Description  : Prints all PrintSTAUD_DRGetCompressionMode
************************************************************************************/

void PrintSTAUD_DRGetCompressionMode (  STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_CompressionMode_t *CompressionMode_p)
{
    STTBX_Print(("STAUD_DRGetCompressionMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  CompMode PTR =%p \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),CompressionMode_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetAudioCodingMode

Description  : Prints all PrintSTAUD_DRSetAudioCodingMode
************************************************************************************/

void  PrintSTAUD_DRSetAudioCodingMode ( STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_AudioCodingMode_t AudioCodingMode_p)
{
    STTBX_Print(("STAUD_DRSetAudioCodingMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  Coding Mode =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),AudioCodingMode_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRGetAudioCodingMode

Description  : Prints all PrintSTAUD_DRGetAudioCodingMode
************************************************************************************/
void  PrintSTAUD_DRGetAudioCodingMode ( STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_AudioCodingMode_t * AudioCodingMode_p)
{
    STTBX_Print(("STAUD_DRGetCompressionMode     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  Coding Mode PTR =%p \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),AudioCodingMode_p));
}

/************************************************************************************
Name         : PrintSTAUD_DRSetDDPOP

Description  : Prints all PrintSTAUD_DRSetDDPOP
************************************************************************************/

void  PrintSTAUD_DRSetDDPOP (   STAUD_Handle_t Handle,
                                STAUD_Object_t DecoderObject,
                                U32 DDPOPSetting)
{
    STTBX_Print(("STAUD_DRSetDDPOP     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  DDPOPSetting  =%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),DDPOPSetting));
}

/************************************************************************************
Name         : PrintSTAUD_MXUpdatePTSStatus

Description  : Prints all PrintSTAUD_MXUpdatePTSStatus
************************************************************************************/
void PrintSTAUD_MXUpdatePTSStatus(  STAUD_Handle_t Handle,
                                    STAUD_Object_t MixerObject,
                                    U32 InputID, BOOL PTSStatus)
{
    STTBX_Print(("STAUD_MXUpdatePTSStatus     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  InputID  =%d \n PTSStatus=%d \n",(void *)Handle,GetObjectNameFromID(MixerObject),InputID,PTSStatus));
}

/************************************************************************************
Name         : PrintSTAUD_MXUpdateMaster

Description  : Prints all PrintSTAUD_MXUpdateMaster
************************************************************************************/

void PrintSTAUD_MXUpdateMaster( STAUD_Handle_t Handle,
                                STAUD_Object_t MixerObject,
                                U32 InputID, BOOL FreeRun)
{
    STTBX_Print(("STAUD_MXUpdateMaster     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  DC  InputID  =%d \n FreeRun=%d \n",(void *)Handle,GetObjectNameFromID(MixerObject),InputID,FreeRun));
}

/************************************************************************************
Name         : PrintSTAUD__ENCGetCapability

Description  : Prints all PrintSTAUD__ENCGetCapability
************************************************************************************/

void  PrintSTAUD_ENCGetCapability(  const ST_DeviceName_t DeviceName,
                                    STAUD_Object_t EncoderObject,
                                    STAUD_ENCCapability_t *Capability_p)
{
    STTBX_Print(("STAUD_ENCGetCapability     Params \n "));
    STTBX_Print(("Device =%s \n Object=%s \n  Enc Cap  PTR  =%p \n ", DeviceName,GetObjectNameFromID(EncoderObject),Capability_p));
}

/************************************************************************************
Name         : PrintSTAUD__ENCGetOutputParams

Description  : Prints all PrintSTAUD__ENCGetOutputParams
************************************************************************************/
void  PrintSTAUD_ENCGetOutputParams(STAUD_Handle_t Handle,
                                    STAUD_Object_t EncoderObject,
                                    STAUD_ENCOutputParams_s *EncoderOPParams_p)
{
    STTBX_Print(("STAUD_ENCGetOutputParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n  Enc OP  PTR  =%p \n ",(void *)Handle,GetObjectNameFromID(EncoderObject),EncoderOPParams_p));
}

/************************************************************************************
Name         : PrintSTAUD_ENCSetOutputParams

Description  : Prints all PrintSTAUD_ENCSetOutputParams
************************************************************************************/
void PrintSTAUD_ENCSetOutputParams( STAUD_Handle_t Handle,
                                    STAUD_Object_t EncoderObject,
                                    STAUD_ENCOutputParams_s EncoderOPParams)
{
    STTBX_Print(("STAUD_ENCSetOutputParams     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(EncoderObject)));
    STTBX_Print(("Encoder AutoParams =%d \n BitRate=%d \n OutputFreq=%d \n", EncoderOPParams.AutoOutputParam,
                        EncoderOPParams.BitRate,EncoderOPParams.OutputFrequency));

    STTBX_Print((" ** DD Config ** \n \t Comp on =%d \n \t CompOnSec =%d \n", EncoderOPParams.EncoderStreamSpecificParams.DDConfig.CompOn,
                        EncoderOPParams.EncoderStreamSpecificParams.DDConfig.CompOnSec));

    STTBX_Print((" ** AAC Config ** \n \t quantqual on =%d \n \t VBR ON=%d \n", (U32)EncoderOPParams.EncoderStreamSpecificParams.AacConfig.quantqual,
                        EncoderOPParams.EncoderStreamSpecificParams.AacConfig.VbrOn));

    STTBX_Print((" ** MP3 Config ** \n \t bandWidth=%d  \n \t Intensity=%d  \n", EncoderOPParams.EncoderStreamSpecificParams.MP3Config.bandWidth,
                        EncoderOPParams.EncoderStreamSpecificParams.MP3Config.Intensity));


    STTBX_Print(("\t Vbr Mode=%d \n \t Vbr Quality=%d  \n \t Full Huffman=%d  \n", EncoderOPParams.EncoderStreamSpecificParams.MP3Config.VbrMode,
                        EncoderOPParams.EncoderStreamSpecificParams.MP3Config.VbrQuality, EncoderOPParams.EncoderStreamSpecificParams.MP3Config.FullHuffman));

    STTBX_Print(("\t Padding=%d \n \t CRC=%d  \n \t PrivateBit=%d  \n", EncoderOPParams.EncoderStreamSpecificParams.MP3Config.paddingMode,
                        EncoderOPParams.EncoderStreamSpecificParams.MP3Config.Crc, EncoderOPParams.EncoderStreamSpecificParams.MP3Config.privateBit));

    STTBX_Print(("\t copyRightBit=%d  \n \t originalCopyBit =%d \n \t EmphasisFlag=%d  \n \t donwmix =%d \n", EncoderOPParams.EncoderStreamSpecificParams.MP3Config.copyRightBit,
                        EncoderOPParams.EncoderStreamSpecificParams.MP3Config.originalCopyBit, EncoderOPParams.EncoderStreamSpecificParams.MP3Config.EmphasisFlag,
                        EncoderOPParams.EncoderStreamSpecificParams.MP3Config.downmix));

    STTBX_Print((" ** MP2 Config ** \n \t mode=%d  \n \t CRC ON=%d \n \t Emph on=%d  \n", EncoderOPParams.EncoderStreamSpecificParams.MP2Config.Mode,
                        EncoderOPParams.EncoderStreamSpecificParams.MP2Config.CrcOn,EncoderOPParams.EncoderStreamSpecificParams.MP2Config.Emphasis));

    STTBX_Print(("\t CopyRight =%d \n \t Org =%d \n", EncoderOPParams.EncoderStreamSpecificParams.MP2Config.Copyrighted,
                        EncoderOPParams.EncoderStreamSpecificParams.MP2Config.Original));

}

/************************************************************************************
Name         : PrintSTAUD_ModuleStart

Description  : Prints all PrintSTAUD_ModuleStart .
************************************************************************************/


void  PrintSTAUD_ModuleStart (  STAUD_Handle_t Handle,
                                STAUD_Object_t ModuleObject,
                                STAUD_StreamParams_t * StreamParams_p)
{
    STTBX_Print(("STAUD_ModuleStart     Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ** Start Params ** \n",(void *)Handle,GetObjectNameFromID(ModuleObject)));
    STTBX_Print(("\t StreamType=%d \n \t StreamContents=%d \n \t Freq=%d \n \t StreamId=%d \n",StreamParams_p->StreamType,
                        StreamParams_p->StreamContent,StreamParams_p->SamplingFrequency,StreamParams_p->StreamID));
}

/************************************************************************************
Name         : PrintSTAUD_GenericStart

Description  : Prints all PrintSTAUD_GenericStart .
************************************************************************************/
void  PrintSTAUD_GenericStart (STAUD_Handle_t Handle)
{
    STTBX_Print(("STAUD_GenericStart   \n "));
    STTBX_Print(("Handle =%p \n  ",(void *)Handle));
}

/************************************************************************************
Name         : PrintSTAUD_ModuleStop

Description  : Prints all PrintSTAUD_ModuleStop .
************************************************************************************/

void  PrintSTAUD_ModuleStop (STAUD_Handle_t Handle, STAUD_Object_t ModuleObject)
{
    STTBX_Print(("STAUD_ModuleStop      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(ModuleObject)));
}

/************************************************************************************
Name         : PrintSTAUD_ModuleStop

Description  : Prints all PrintSTAUD_ModuleStop .
************************************************************************************/
void  PrintSTAUD_GenericStop(STAUD_Handle_t Handle)
{
    STTBX_Print(("STAUD_GenericStop   \n "));
    STTBX_Print(("Handle =%p \n  ",(void *)Handle));
}

/************************************************************************************
Name         : PrintSTAUD_SetModuleAttenuation

Description  : Prints all PrintSTAUD_ModuleStop .
************************************************************************************/

void  PrintSTAUD_SetModuleAttenuation ( STAUD_Handle_t Handle,
                                        STAUD_Object_t ModuleObject,
                                        STAUD_Attenuation_t * Attenuation_p)
{
    STTBX_Print(("STAUD_SetModuleAttenuation      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s \n ",(void *)Handle,GetObjectNameFromID(ModuleObject)));

    STTBX_Print(("Left =%d \n Right =%d \n Center =%d \n SubW =%d \n ", Attenuation_p->Left,
                        Attenuation_p->Right,Attenuation_p->Center,Attenuation_p->Subwoofer));

    STTBX_Print(("Left W =%d \n Right W =%d \n CS Left  =%d \n Cs Right =%d \n ", Attenuation_p->LeftSurround,
                        Attenuation_p->RightSurround,Attenuation_p->CsLeft,Attenuation_p->CsRight));

    STTBX_Print(("VCRL=%d \n VCRR =%d \n  ", Attenuation_p->VcrLeft,
                        Attenuation_p->VcrRight));
}

/************************************************************************************
Name         : PrintSTAUD_GetModuleAttenuation

Description  : Prints all PrintSTAUD_GetModuleAttenuation .
************************************************************************************/

void  PrintSTAUD_GetModuleAttenuation ( STAUD_Handle_t Handle,
                                        STAUD_Object_t ModuleObject,
                                        STAUD_Attenuation_t * Attenuation_p)
{
    STTBX_Print(("STAUD_GetModuleAttenuation      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s Atte PTR=%p \n ",(void *)Handle,GetObjectNameFromID(ModuleObject),Attenuation_p));

}
/************************************************************************************
Name         : PrintSTAUD_DRSetDTSNeoParams

Description  : Prints all PrintSTAUD_GetModuleAttenuation .
************************************************************************************/
  void  PrintSTAUD_DRSetDTSNeoParams(   STAUD_Handle_t Handle,
                                        STAUD_Object_t DecoderObject,
                                        STAUD_DTSNeo_t *DTSNeo_p)
{
    STTBX_Print(("STAUD_DRSetDTSNeoParams      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s Enable=%d \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),DTSNeo_p->Enable));
    STTBX_Print(("C Gain=%d \n Neo Mode=%d \n NeoAUXMode=%d \n OPChannel=%d \n ",DTSNeo_p->CenterGain,
                        DTSNeo_p->NeoMode,DTSNeo_p->NeoAUXMode,DTSNeo_p->OutputChanels));
}
/************************************************************************************
Name         : PrintSTAUD_DRGetDTSNeoParams

Description  : Prints all PrintSTAUD_DRGetDTSNeoParams .
************************************************************************************/
void  PrintSTAUD_DRGetDTSNeoParams( STAUD_Handle_t Handle,
                                    STAUD_Object_t DecoderObject,
                                    STAUD_DTSNeo_t *DTSNeo_p)
{
    STTBX_Print(("STAUD_DRGetDTSNeoParams      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s DTS Neo PTR=%p \n ",(void *)Handle,GetObjectNameFromID(DecoderObject),DTSNeo_p));
}

/************************************************************************************
Name         : PrintSTAUD_PPSetBTSCParams

Description  : Prints all PrintSTAUD_PPSetBTSCParams .
************************************************************************************/
  void  PrintSTAUD_PPSetBTSCParams( STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProObject,
                                    STAUD_BTSC_t *BTSC_p)
{
    STTBX_Print(("STAUD_PPSetBTSCParams      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s Enable=%d \n ",(void *)Handle,GetObjectNameFromID(PostProObject),BTSC_p->Enable));
    STTBX_Print(("IP Gain=%d \n TX Gain=%d \n Dual Sig=%d \n",BTSC_p->InputGain,
                        BTSC_p->TXGain,BTSC_p->DualSignal));
}

/************************************************************************************
Name         : PrintSTAUD_PPGetBTSCParams

Description  : Prints all PrintSTAUD_PPGetBTSCParams .
************************************************************************************/
void  PrintSTAUD_PPGetBTSCParams(   STAUD_Handle_t Handle,
                                    STAUD_Object_t PostProObject,
                                    STAUD_BTSC_t *BTSC_p)
{
    STTBX_Print(("STAUD_PPGetBTSCParams      Params \n "));
    STTBX_Print(("Handle =%p \n Object=%s BTSC PTR=%p \n ",(void *)Handle,GetObjectNameFromID(PostProObject),BTSC_p));
}
/************************************************************************************
Name         : PrintSTAUD_SetScenario

Description  : Prints all PrintSTAUD_SetScenario .
************************************************************************************/
void  PrintSTAUD_SetScenario(STAUD_Handle_t Handle, STAUD_Scenario_t Scenario)
{
    STTBX_Print(("STAUD_SetScenario     \n "));
    STTBX_Print(("Handle =%p \n Scenario=%d \n ", (void *)Handle,Scenario));
}



