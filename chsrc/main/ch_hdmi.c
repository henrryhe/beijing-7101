/*
(c) Copyright changhong
Name:ch_hdmi.c
Description:hdmi contol for changhong 7100 platform
Authors:yixiaoli
Date          Remark
2007-01-30    Created

Modify record: 
		
*/

#include "appltype.h"
#include "initterm.h"
#include "stddefs.h"
#include "stcommon.h"

#include "..\errors\errors.h"
#include <string.h>

#include "..\main\ch_hdmi.h"



ST_ErrorCode_t CH_HDMI_FillSPD(U32 DeviceId);
ST_ErrorCode_t CH_HDMI_FillAudio(U32 DeviceId);

ST_ErrorCode_t CH_HDMI_FillAVI(U32 DeviceId);
ST_ErrorCode_t CH_HDMI_FillMS(U32 DeviceId);
ST_ErrorCode_t CH_HDMI_FillVS(U32 DeviceId);

ST_ErrorCode_t CH_HDMI_EnableInfoframe(U32 HandleId);




#if 0
#define YXL_HDMI_DEBUG
#endif 


#define HDMI_MAX_NUMBER 1

HDMI_Context_t  HDMI_Context[HDMI_MAX_NUMBER/*0*/];

/* Public HDCP keys */
/* ---------------- */

#ifndef HDMI_IV_0_KEY_VALUE
#define HDMI_IV_0_KEY_VALUE  0x00000000 /* Initialization value Key LSB */
#endif
#ifndef HDMI_IV_1_KEY_VALUE
#define HDMI_IV_1_KEY_VALUE  0x00000000 /* Initialization value Key MSB */
#endif
#ifndef HDMI_KSV_0_KEY_VALUE
#define HDMI_KSV_0_KEY_VALUE 0x00000000 /* Key selection vector */
#endif
#ifndef HDMI_KSV_1_KEY_VALUE
#define HDMI_KSV_1_KEY_VALUE 0x00000000 /* Key selection vector */
#endif
/* Secret device keys of the HDCP transmitter from the digital content protection LLC */
static const U32 HDMI_DeviceKeys[80] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };


/*#define YXL_HDMI_DEBUG  yxl 2006-05-23 add this macro*/

/*yxl 2006-03-28 add below function*/



STHDMI_Handle_t HDMIHandle;
ST_DeviceName_t HDMIDeviceName="HDMI";


/*yxl 2006-04-27 add below section*/
BOOL bHDMI_restart = FALSE;

/*end yxl 2006-04-27 add below section*/

/*---------------------------------------------------------------------
 * Function     : HDMI_IsScreenDVIOnly
 * Description : This function checks if the screen is DVI only
 * Input          : HDMI Handle ID, DVIOnlyNotHDMI as boolean
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_IsScreenDVIOnly(U32 DeviceId,BOOL *DVIOnlyNotHDMI)
{
    ST_ErrorCode_t    ErrCode = ST_NO_ERROR;
    STVOUT_State_t    VOUT_State;
    STHDMI_EDIDSink_t HDMI_EDIDSink;
    BOOL              SinkIsDVIOnly;

    /* Try to get HDMI state */
    /* ---------------------------- */
    ErrCode = STVOUT_GetState(HDMI_Context[DeviceId].VOUT_Handle , &VOUT_State);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get state of the HDMI !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }
    if (VOUT_State == STVOUT_NO_RECEIVER)
    {
        STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! There is not screen on HDMI !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Get EDID of the sink */
    /* This call is required for proper operation of audio workaround */
    /* -------------------- */
    memset(&HDMI_EDIDSink, 0, sizeof(STHDMI_EDIDSink_t));
    ErrCode = STHDMI_FillSinkEDID(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_EDIDSink);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get sink informations !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    #if 1
    /* Analyse EDID */
    /* ------------ */
    SinkIsDVIOnly = TRUE;
    if (HDMI_EDIDSink.EDIDExtension_p != NULL)
    {
        if ((HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.Tag == 0x02) &&
                        (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.RevisionNumber == 3))
        {
            if ((HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[0] == 0x03)&&
                (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[1] == 0x0C)&&
                (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[2] == 0x00))
            {
                SinkIsDVIOnly = FALSE;
            }
        }
    }
    #endif

    /* Free EDID */
    /* --------- */
    if (HDMI_EDIDSink.EDIDExtension_p != NULL)
    {
        ErrCode = STHDMI_FreeEDIDMemory(HDMI_Context[DeviceId].HDMI_Handle);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to deallocate EDID informations !!!\n"));
            return (ST_ERROR_BAD_PARAMETER);
        }
    }

    /* Return no errors */
    /* ---------------- */
    *DVIOnlyNotHDMI = SinkIsDVIOnly;
    return (ST_NO_ERROR);
}/* HDMI_IsScreenDVIOnly() */



/*yxl 2007-01-30 add below functions*/


BOOL CHHDMI_EnableOutput(U32 DeviceId)
{
	STHDMI_OutputWindows_t  HDMIOutputWindows;
	STHDMI_AVIInfoFrame_t   HDMIAVIInfoFrame;
	STHDMI_AUDIOInfoFrame_t HDMIAUDIOInfoFrame;
	STVOUT_OutputParams_t   VOUTOutputParams;
	STVOUT_HDCPParams_t     VOUTHDCPParams;
	STVTG_TimingMode_t      VTGTimingMode;
	STVTG_ModeParams_t      VTGModeParams;
	ST_ErrorCode_t          ErrCode=ST_NO_ERROR;
	
	if (DeviceId>=HDMI_MAX_NUMBER)
	{
		STTBX_Print(("CHHDMI_EnableOutput(?):**ERROR** !!! Invalid HDMI device identifier !!!\n"));
		return TRUE;
	}
	if (HDMI_Context[DeviceId].HDMI_Handle==0)
	{
		STTBX_Print(("CHHDMI_EnableOutput(?):**ERROR** !!! HDMI device identifier not initialized !!!\n"));
		return TRUE;
	}
	
	Mutex_Lock(HDMI_Context[DeviceId].MutexLock);
	
	ErrCode=STVOUT_GetOutputParams(HDMI_Context[DeviceId].VOUT_Handle,&VOUTOutputParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to get output params !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}

#if 1 /*yxl 2007-02-01 cancel*/	
	/* Check if the screen is DVI only or not */
	{
		BOOL IsDVIOnly = FALSE;
		ErrCode=CH_HDMI_IsScreenDVIOnly(DeviceId,&IsDVIOnly);	/*! Louie(4/17/2006): Some problem in EDID parsing, disabled for now */
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to detect if this is a DVI or HDMI screen !!!\n",DeviceId));
			Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
                      IsDVIOnly = FALSE; 
			/*return TRUE; 20090110 解决部分电视断电情况下，也检测为有HDMI设备问题*/
		}
		VOUTOutputParams.HDMI.ForceDVI=IsDVIOnly;
	}
#endif /*end yxl 2007-02-01 cancel*/	

#if 0 /*yxl 2007-02-10 temp add for test*/
	{
		STHDMI_EDIDSink_t HDMI_EDIDSink;
		memset(&HDMI_EDIDSink,0,sizeof(STHDMI_EDIDSink_t));
		ErrCode=STHDMI_FillSinkEDID(HDMI_Context[DeviceId].HDMI_Handle,&HDMI_EDIDSink);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STHDMI_FillSinkEDID()=%s\n",GetErrorText(ErrCode)));
			return(ST_ERROR_BAD_PARAMETER);
		}
	}

#endif/*end yxl 2007-02-10 temp add for test*/

	ErrCode=STVOUT_SetOutputParams(HDMI_Context[DeviceId].VOUT_Handle,&VOUTOutputParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to set output params !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}
	
#if 0 /*def ST_7100*/ /*yxl 2007-05-31 add this line*/
	if (VOUTOutputParams.HDMI.IsHDCPEnable==TRUE)
	{
		memset(&VOUTHDCPParams,0,sizeof(STVOUT_HDCPParams_t));
		VOUTHDCPParams.IV_0        = (U32)HDMI_Context[DeviceId].HDMI_IV0_Key;
		VOUTHDCPParams.IV_1        = (U32)HDMI_Context[DeviceId].HDMI_IV1_Key;
		VOUTHDCPParams.KSV_0       = (U32)HDMI_Context[DeviceId].HDMI_KV0_Key;
		VOUTHDCPParams.KSV_1       = (U32)HDMI_Context[DeviceId].HDMI_KV1_Key;
		VOUTHDCPParams.IRate       = 0;
		VOUTHDCPParams.IsACEnabled = FALSE;
		memcpy(&VOUTHDCPParams.DeviceKeys,HDMI_Context[DeviceId].HDMI_DeviceKeys,sizeof(HDMI_Context[DeviceId].HDMI_DeviceKeys));
		ErrCode=STVOUT_SetHDCPParams(HDMI_Context[DeviceId].VOUT_Handle,&VOUTHDCPParams);
		if (ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to setup HDCP keys !!!\n",DeviceId));
			Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
			return TRUE;
		}
	}
#endif
	ErrCode=STVOUT_Enable(HDMI_Context[DeviceId].VOUT_Handle);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to enable the HDMI interface !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}
	HDMI_Context[DeviceId].HDMI_IsEnabled=TRUE;
	
	ErrCode=STVTG_GetMode(HDMI_Context[DeviceId].VTG_Handle,&VTGTimingMode,&VTGModeParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to get the VTG parameters !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}

	memset(&HDMIOutputWindows,0,sizeof(STHDMI_OutputWindows_t));
	
#if 0 /*def ST_7100*/
	HDMIOutputWindows.OutputWinX_p      = 0;                                   
	HDMIOutputWindows.OutputWinY_p      = 0;                                   
	HDMIOutputWindows.OutputWinWidth_p  = VTGModeParams.ActiveAreaWidth;  
	HDMIOutputWindows.OutputWinHeight_p = VTGModeParams.ActiveAreaHeight;
#endif

#if 1 /*def ST_7109	*/
	HDMIOutputWindows.OutputWinX      = 0;                                   
	HDMIOutputWindows.OutputWinY      = 0;                                   
	HDMIOutputWindows.OutputWinWidth  = VTGModeParams.ActiveAreaWidth;  
	HDMIOutputWindows.OutputWinHeight = VTGModeParams.ActiveAreaHeight;
#endif
	ErrCode=STHDMI_SetOutputWindows(HDMI_Context[DeviceId].HDMI_Handle,&HDMIOutputWindows);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to set the HDMI I/O window size !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}
#if 0
	memset(&HDMIAVIInfoFrame,0,sizeof(STHDMI_AVIInfoFrame_t));
	HDMIAVIInfoFrame.AVI861B.FrameType                  = STHDMI_INFOFRAME_TYPE_AVI;
	HDMIAVIInfoFrame.AVI861B.FrameVersion               = 2;
	HDMIAVIInfoFrame.AVI861B.FrameLength                = 13;
	HDMIAVIInfoFrame.AVI861B.HasActiveFormatInformation = TRUE;
	HDMIAVIInfoFrame.AVI861B.OutputType                 = HDMI_Context[DeviceId].HDMI_OutputType;
	HDMIAVIInfoFrame.AVI861B.ScanInfo                   = HDMI_Context[DeviceId].HDMI_ScanInfo;
	HDMIAVIInfoFrame.AVI861B.Colorimetry                = HDMI_Context[DeviceId].HDMI_Colorimetry;
	HDMIAVIInfoFrame.AVI861B.AspectRatio                = HDMI_Context[DeviceId].HDMI_AspectRatio; 
	HDMIAVIInfoFrame.AVI861B.ActiveAspectRatio          = HDMI_Context[DeviceId].HDMI_ActiveAspectRatio;
	HDMIAVIInfoFrame.AVI861B.PictureScaling             = HDMI_Context[DeviceId].HDMI_PictureScaling;
	ErrCode=STHDMI_FillAVIInfoFrame(HDMI_Context[DeviceId].HDMI_Handle,&HDMIAVIInfoFrame);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to send the AVI Info frame !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}
	
	memset(&HDMIAUDIOInfoFrame,0,sizeof(STHDMI_AUDIOInfoFrame_t));
	HDMIAUDIOInfoFrame.FrameType         = STHDMI_INFOFRAME_TYPE_AUDIO;
	HDMIAUDIOInfoFrame.FrameVersion      = 2;
	HDMIAUDIOInfoFrame.CodingType        = STAUD_STREAM_CONTENT_NULL;
	HDMIAUDIOInfoFrame.SampleSize        = 0xFF;
	HDMIAUDIOInfoFrame.DownmixInhibit    = FALSE;
	ErrCode=STHDMI_FillAudioInfoFrame(HDMI_Context[DeviceId].HDMI_Handle,&HDMIAUDIOInfoFrame);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_EnableOutput(%d):**ERROR** !!! Unable to send the AUDIO Info frame !!!\n",DeviceId));
		Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}
#else
       CH_HDMI_EnableInfoframe(0);
#endif
	Mutex_Unlock(HDMI_Context[DeviceId].MutexLock); 

	return FALSE;
}


/* ========================================================================
Name:        CHHDMI_DisableOutput
Description: Disable the HDMI output 
======================================================================== */
BOOL CHHDMI_DisableOutput(U32 DeviceId)
{
	ST_ErrorCode_t ErrCode=ST_NO_ERROR;

	if (DeviceId>=HDMI_MAX_NUMBER)
	{
		STTBX_Print(("CHHDMI_DisableOutput(?):**ERROR** !!! Invalid HDMI device identifier !!!\n"));
		return TRUE;
	}
	if (HDMI_Context[DeviceId].HDMI_Handle==0)
	{
		STTBX_Print(("CHHDMI_DisableOutput(?):**ERROR** !!! HDMI device identifier not initialized !!!\n"));
		return TRUE;
	}

	Mutex_Lock(HDMI_Context[DeviceId].MutexLock);
	

	ErrCode=STVOUT_Disable(HDMI_Context[DeviceId].VOUT_Handle);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CHHDMI_DisableOutput(%d):**ERROR** !!! Unable to disable the HDMI interface !!!\n",DeviceId));
		Mutex_Lock(HDMI_Context[DeviceId].MutexLock);
		return TRUE;
	}
	HDMI_Context[DeviceId].HDMI_IsEnabled=FALSE;

	Mutex_Unlock(HDMI_Context[DeviceId].MutexLock);
	return FALSE;
}



static void CHHDMI_HotplugCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
	U32            i;
	ST_ErrorCode_t ErrCode=ST_NO_ERROR;

	for (i=0;i<HDMI_MAX_NUMBER;i++)
	{
		if (strcmp(HDMI_Context[i].VTG_DeviceName,RegistrantName)==0) break;
	} 
	if (i==HDMI_MAX_NUMBER)
	{
/*		STTBX_Print(("CHHDMI_HotplugCallback(?):**ERROR** !!! Event Received from unknow HDMI interface !!!\n",i));*/
		return;
	}
	if (EventData==NULL)
	{
		STTBX_Print(("CHHDMI_HotplugCallback(%d):**ERROR** !!! EventData pointer is NULL !!!\n",i));
		return;
	}

	Mutex_Lock(HDMI_Context[i].MutexLock);
	
#ifdef YXL_HDMI_DEBUG
	STTBX_Print(("CHHDMI_HotplugCallback(%d): Changing state from ",i));
	switch(HDMI_Context[i].VOUT_State)
	{
	case STVOUT_DISABLED                   : 
		STTBX_Print(("STVOUT_DISABLED to "));
		break;
	case STVOUT_ENABLED                    : 
		STTBX_Print(("STVOUT_ENABLED to "));
		break;
	case STVOUT_NO_RECEIVER                : 
		STTBX_Print(("STVOUT_NO_RECEIVER to "));
		break;
	case STVOUT_RECEIVER_CONNECTED         :
		STTBX_Print(("STVOUT_RECEIVER_CONNECTED to "));
		break;
	case STVOUT_NO_ENCRYPTION              : 
		STTBX_Print(("STVOUT_NO_ENCRYPTION to ")); 
		break;
	case STVOUT_NO_HDCP_RECEIVER           : 
		STTBX_Print(("STVOUT_NO_HDCP_RECEIVER to "));
		break;
	case STVOUT_AUTHENTICATION_IN_PROGRESS : 
		STTBX_Print(("STVOUT_AUTHENTICATION_IN_PROGRESS to "));
		break;
	case STVOUT_AUTHENTICATION_FAILED      : 
		STTBX_Print(("STVOUT_AUTHENTICATION_FAILED to "));
		break;
	case STVOUT_AUTHENTICATION_SUCCEEDED   :
		STTBX_Print(("STVOUT_AUTHENTICATION_SUCCEEDED to "));
		break;
	default                                :
		STTBX_Print(("STVOUT_UNKNOWN to ")); 
		break;
	}
#endif 

	HDMI_Context[i].VOUT_State=*(STVOUT_State_t *)EventData;

#ifdef YXL_HDMI_DEBUG
	switch(HDMI_Context[i].VOUT_State)
	{
	case STVOUT_DISABLED                   : 
		STTBX_Print(("STVOUT_DISABLED\n"));  
		break;
	case STVOUT_ENABLED                    : 
		STTBX_Print(("STVOUT_ENABLED\n")); 
		break;
	case STVOUT_NO_RECEIVER                : 
		STTBX_Print(("STVOUT_NO_RECEIVER\n")); 
		break;
	case STVOUT_RECEIVER_CONNECTED         : 
		STTBX_Print(("STVOUT_RECEIVER_CONNECTED\n")); 
		break;
	case STVOUT_NO_ENCRYPTION              : 
		STTBX_Print(("STVOUT_NO_ENCRYPTION\n")); 
		break;
	case STVOUT_NO_HDCP_RECEIVER           :
		STTBX_Print(("STVOUT_NO_HDCP_RECEIVER\n")); 
		break;
	case STVOUT_AUTHENTICATION_IN_PROGRESS : 
		STTBX_Print(("STVOUT_AUTHENTICATION_IN_PROGRESS\n"));
		break;
	case STVOUT_AUTHENTICATION_FAILED      : 
		STTBX_Print(("STVOUT_AUTHENTICATION_FAILED\n"));
		break;
	case STVOUT_AUTHENTICATION_SUCCEEDED   : 
		STTBX_Print(("STVOUT_AUTHENTICATION_SUCCEEDED\n")); 
		break;
	default                                :
		STTBX_Print(("STVOUT_UNKNOWN\n"));  
		break;
	}
#endif	

	switch(HDMI_Context[i].VOUT_State)
	{
		
	case STVOUT_NO_RECEIVER:
		if (HDMI_Context[i].HDMI_AutomaticEnableDisable==TRUE)
		{
			ErrCode=CHHDMI_DisableOutput(i);
			if (ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("CHHDMI_HotplugCallback(%d):**ERROR** !!! Unable to disable the HDMI interface !!!\n",i));
				break;
			}
		}
		break;
		
	case STVOUT_RECEIVER_CONNECTED :
		
		if (HDMI_Context[i].HDMI_AutomaticEnableDisable==TRUE)
		{
			ErrCode=CHHDMI_EnableOutput(i);
			if (ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("CHHDMI_HotplugCallback(%d):**ERROR** !!! Unable to enable the HDMI interface !!!\n",i));
				break;
			}
		}
		break;
	} 

	Mutex_Unlock(HDMI_Context[i].MutexLock); 
	
}


/* ========================================================================
Name:        CHHDMI_AudioCallback
Description: When the audio frequency changed, this callback is called
======================================================================== */
static void CHHDMI_AudioCallback(STEVT_CallReason_t Reason,const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event,const void *EventData,const void *SubscriberData_p)
{
	U32                   i,NewAudioFrequency;
	STVOUT_OutputParams_t VOUTOutputParams;
	ST_ErrorCode_t        ErrCode=ST_NO_ERROR;
	   
	for (i=0;i<HDMI_MAX_NUMBER;i++)
	{
		if (HDMI_Context[i].HDMI_Handle!=0)
		{
			if (strcmp(RegistrantName,HDMI_Context[i].AUD_DeviceName)==0) break;
		}
	}

	if (i==HDMI_MAX_NUMBER)
	{
/*		STTBX_Print(("CHHDMI_AudioCallback(?):**ERROR** !!! Event Received from unknow HDMI interface !!!\n",i));*/
		return;
	}
	if (EventData==NULL)
	{
		STTBX_Print(("CHHDMI_AudioCallback(%d):**ERROR** !!! EventData pointer is NULL !!!\n",i));
		return;
	}

	NewAudioFrequency=*(U32*)EventData;

	Mutex_Lock(HDMI_Context[i].MutexLock);

	if (HDMI_Context[i].HDMI_AudioFrequency!=NewAudioFrequency)
	{
		HDMI_Context[i].HDMI_AudioFrequency=NewAudioFrequency;

		if (HDMI_Context[i].HDMI_IsEnabled==TRUE)
		{
			ErrCode=STVOUT_GetOutputParams(HDMI_Context[i].VOUT_Handle,&VOUTOutputParams);
			if (ErrCode!=ST_NO_ERROR)
			{
				STTBX_Print(("CHHDMI_HotplugCallback(%d):**ERROR** !!! Unable to get output params !!!\n",i));
				Mutex_Unlock(HDMI_Context[i].MutexLock);
				return;
			 }
			 VOUTOutputParams.HDMI.AudioFrequency=HDMI_Context[i].HDMI_AudioFrequency;
			 ErrCode=STVOUT_SetOutputParams(HDMI_Context[i].VOUT_Handle,&VOUTOutputParams);
			 if (ErrCode!=ST_NO_ERROR)
			 {
				STTBX_Print(("CHHDMI_HotplugCallback(%d):**ERROR** !!! Unable to set output params !!!\n",i));
				Mutex_Unlock(HDMI_Context[i].MutexLock);
				return;
			  }
		}
	}

	Mutex_Unlock(HDMI_Context[i].MutexLock);
	   
}




/*end yxl 2007-01-30 add below functions*/


BOOL CHHDMI_Setup(void)
{
	   
	ST_ErrorCode_t ErrCode;
	STHDMI_InitParams_t InitParams;
	STHDMI_OpenParams_t OpenParams;
	STEVT_DeviceSubscribeParams_t EVT_SubcribeParams;
	   
	memset((void *)&InitParams, 0, sizeof(STHDMI_InitParams_t));
	InitParams.DeviceType      = STHDMI_DEVICE_TYPE_7100;
	InitParams.MaxOpen         = 1;
	InitParams.CPUPartition_p  = SystemPartition;
	InitParams.OutputType      = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
	   
	InitParams.AVIType = STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO;
	InitParams.SPDType = STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE;
	InitParams.MSType = STHDMI_MS_INFOFRAME_FORMAT_VER_ONE;
	InitParams.AUDIOType = STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE;
	InitParams.VSType = STHDMI_VS_INFOFRAME_FORMAT_VER_ONE;
	   
	/*   strcpy(InitParams.Target.OnChipHdmiCell.AUDDeviceName, AUDDeviceName);
	   
	strcpy(InitParams.Target.OnChipHdmiCell.VIDDeviceName, VIDDeviceName);
	strcpy(InitParams.Target.OnChipHdmiCell.VMIXDeviceName, VMIXDeviceName[VMIX_MAIN]);*/
	   
	strcpy(InitParams.Target.OnChipHdmiCell.EvtDeviceName, EVTDeviceName);
#if 0 
	strcpy(InitParams.Target.OnChipHdmiCell.VOUTDeviceName, VOUTDeviceName[VOUT_MAIN]);
#else
	strcpy(InitParams.Target.OnChipHdmiCell.VOUTDeviceName, VOUTDeviceName[VOUT_HDMI]);
#endif
	strcpy(InitParams.Target.OnChipHdmiCell.VTGDeviceName, VTGDeviceName[VTG_MAIN]);
	   
	ErrCode = STHDMI_Init(HDMIDeviceName, &InitParams);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STHDMI_Init()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	}
	   
	   
   memset((void *)&OpenParams, 0, sizeof(STHDMI_OpenParams_t));
	   
   ErrCode = STHDMI_Open(HDMIDeviceName,&OpenParams,&HDMIHandle);
   if (ErrCode != ST_NO_ERROR)
   {
	   STTBX_Print(("STHDMI_Open()=%s\n",GetErrorText(ErrCode)));	
	   return TRUE; 
   }
	   
   HDMI_Context[0].HDMI_Handle                 = HDMIHandle;
   HDMI_Context[0].VTG_Handle                  = VTGHandle[VTG_MAIN];
   HDMI_Context[0].VOUT_Handle                 = VOUTHandle[VOUT_HDMI];
   HDMI_Context[0].VOUT_State                  = STVOUT_NO_RECEIVER;
   HDMI_Context[0].HDMI_AutomaticEnableDisable = TRUE;
   HDMI_Context[0].HDMI_IsEnabled              = FALSE;
   HDMI_Context[0].HDMI_OutputType             = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
   HDMI_Context[0].HDMI_ScanInfo               = STHDMI_SCAN_INFO_OVERSCANNED;        
   HDMI_Context[0].HDMI_Colorimetry            = STVOUT_ITU_R_601;       
   HDMI_Context[0].HDMI_AspectRatio            = STGXOBJ_ASPECT_RATIO_16TO9;       
   HDMI_Context[0].HDMI_ActiveAspectRatio      = STGXOBJ_ASPECT_RATIO_16TO9;       
   HDMI_Context[0].HDMI_PictureScaling         = STHDMI_PICTURE_NON_UNIFORM_SCALING;       
   HDMI_Context[0].HDMI_AudioFrequency         = 48000;
   HDMI_Context[0].HDMI_IV0_Key                = HDMI_IV_0_KEY_VALUE;
   HDMI_Context[0].HDMI_IV1_Key                = HDMI_IV_1_KEY_VALUE;
   HDMI_Context[0].HDMI_KV0_Key                = HDMI_KSV_0_KEY_VALUE;
   HDMI_Context[0].HDMI_KV1_Key                = HDMI_KSV_1_KEY_VALUE;
   HDMI_Context[0].EVT_Handle=EVTHandle;
   Mutex_Init_Fifo(HDMI_Context[0].MutexLock);
   memcpy(HDMI_Context[0].HDMI_DeviceKeys,HDMI_DeviceKeys,sizeof(HDMI_DeviceKeys));
   strcpy(HDMI_Context[0].AUD_DeviceName,AUDDeviceName);
   strcpy(HDMI_Context[0].VTG_DeviceName,VTGDeviceName[VTG_MAIN]);
	   
	       /* Use this Poke to Enable HDMI audio    */
    /* ===================================== */
 	   
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
   
	EVT_SubcribeParams.NotifyCallback=CHHDMI_HotplugCallback;
	ErrCode=STEVT_SubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,HDMI_Context[0].VTG_DeviceName,
	   STVOUT_CHANGE_STATE_EVT,&EVT_SubcribeParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STEVT_SubscribeDeviceEvent()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	 }   
	   
	memset(&EVT_SubcribeParams,0,sizeof(STEVT_DeviceSubscribeParams_t));
	EVT_SubcribeParams.NotifyCallback=CHHDMI_AudioCallback;
	ErrCode=STEVT_SubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,HDMI_Context[0].AUD_DeviceName,
	STAUD_NEW_FREQUENCY_EVT,&EVT_SubcribeParams);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STEVT_SubscribeDeviceEvent()=%s\n",GetErrorText(ErrCode)));	
		 return TRUE; 
	 }   
	   
	   
	ErrCode=STVOUT_Start(HDMI_Context[0].VOUT_Handle);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVOUT_Start()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	}   
   
	return FALSE;   
}


BOOL CHHDMI_Term(void)
{
	
	ST_ErrorCode_t ErrCode;
	STHDMI_TermParams_t TermParams;

	
#if 1	/*yxl 2007-05-16 add below section*/
	
	ErrCode=STEVT_UnsubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,HDMI_Context[0].VTG_DeviceName,
	   STVOUT_CHANGE_STATE_EVT);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STEVT_UnsubscribeDeviceEvent()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
	 }   
	   


	ErrCode=STEVT_UnsubscribeDeviceEvent(HDMI_Context[0].EVT_Handle,HDMI_Context[0].AUD_DeviceName,
	STAUD_NEW_FREQUENCY_EVT);
	if (ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STEVT_UnsubscribeDeviceEvent()=%s\n",GetErrorText(ErrCode)));	
		 return TRUE; 
	 }   
#endif	/*end yxl 2007-05-16 add below section*/
	
	ErrCode = STHDMI_Close(HDMIHandle);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print(("STHDMI_Close()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
    }
	
    memset((void *)&TermParams, 0, sizeof(STHDMI_TermParams_t));
	
	/*TermParams.ForceTerminate = TRUE;*/
    ErrCode = STHDMI_Term(HDMIDeviceName,&TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
		STTBX_Print(("STHDMI_Term()=%s\n",GetErrorText(ErrCode)));	
		return TRUE; 
    }
	return FALSE;   
}



BOOL CH_HDMIAudeoSetup(void)
{
	
	return TRUE;
}



BOOL CH_FillAVI(void)
{ 
	
	
    return FALSE;
}

BOOL CH_FillSinkEDID(void)
{ 
	
	return FALSE;
	
}


BOOL VOUT_SubscribeEvent(void)
{
	
	return FALSE;
}




#ifndef YXL_060615 /*yxl 2006-06-15 add this line*/

/*yxl 2006-06-13 add below section*/

#define DAUDSET_MSG_QUEUE_SIZE            20
#define DAUDSET_MSG_QUEUE_NO_OF_MESSAGES  10
static message_queue_t *pstDigitalAUDSetMsgueue=NULL;


static void DigitalAUDSetProcess(void *pvParam)
{
	clock_t timeout;
	usif_cmd_t* pMsg;
	usif_cmd_t TempMsg;
	usif_cmd_t *pTempMsg;
	
	pMsg=&TempMsg;
	
	
	
	while(true)
	{
		
		
		
		pTempMsg = (usif_cmd_t *)message_receive_timeout(pstDigitalAUDSetMsgueue,TIMEOUT_INFINITY);
		
		if(pTempMsg==NULL) 	continue;
		
		memcpy(pMsg,pTempMsg,sizeof(usif_cmd_t));
		
		message_release(pstDigitalAUDSetMsgueue, pTempMsg); 
		switch (pMsg->from_which_module)
		{
		case DAUDOUT_SET_MODULE:
			if(pMsg->contents.DAUDSet.OutStatus==TRUE)
			{
				STTBX_Print(("\nYxlInfo:msg received ->AudStreamType=AC3"));
			
				CH_SetDigitalAudioOut(STAUD_STREAM_CONTENT_AC3);*/
				CH_SetDigitalAudioOut(STAUD_STREAM_CONTENT_AC3);
			}
			else
			{
				STTBX_Print(("\nYxlInfo:msg received ->AudStreamType!=AC3"));
		
				
				CH_SetDigitalAudioOut(STAUD_STREAM_CONTENT_MPEG1);
				/*		CH_SetDigitalAudioOut(STAUD_STREAM_CONTENT_MPEG1);*/
			}
			
			break;
		default:
			break;
			
		}
	}
}


BOOL CH_AUDDigitalOutSetup(void)
{
	
	task_t *ptidDigitalAUDDSetTask;	
	
	message_queue_t  *pstMessageQueue = NULL;
	
	if ( ( pstMessageQueue = message_create_queue_timeout (DAUDSET_MSG_QUEUE_SIZE,
		DAUDSET_MSG_QUEUE_NO_OF_MESSAGES ) ) == NULL )
	{
		STTBX_Print(("Unable to create VIDEORESET message queue\n"));
		return TRUE;
	}
	else
	{
		symbol_register ( "DigitalAUDDSet_Queue", ( void * ) pstMessageQueue );
		
	}
	
	if (symbol_enquire_value("DigitalAUDDSet_Queue", (void **)&pstDigitalAUDSetMsgueue))
    {
		STTBX_Print(("\nCan't find DigitalAUDSet message queue\n"));
		return TRUE;
    }
	
	
	if ((ptidDigitalAUDDSetTask = Task_Create(DigitalAUDSetProcess,NULL,2048/*512*/ ,5+3,
		"DigitalAUDDSetTask",0))==NULL)
    {
		
		STTBX_Print(("Failed to start DigitalAUDDSetTask task\n"));
		return TRUE;
    }
	else 
	{
		STTBX_Print(("creat DigitalAUDDSetTask task successfully \n"));
	}
	
	
	
	return FALSE;
}


BOOLEAN CH_SendMsgToDAUDSet(STAUD_StreamContent_t AudStreamType)
{
	
	usif_cmd_t *msg_p;
	
    clock_t timeout;
	
	STTBX_Print(("\nYxlInfo:AudStreamType=%d",AudStreamType));
	if(pstDigitalAUDSetMsgueue==NULL) 
		return FALSE;
	
	timeout= time_plus(time_now(), ST_GetClocksPerSecond()*3);
	
	msg_p = ( usif_cmd_t* ) message_claim_timeout (pstDigitalAUDSetMsgueue, &timeout );
	if(  msg_p == NULL)
	{
		do_report(0,"Claim search_display msg error\n");
		return TRUE;
	}
	
	msg_p ->from_which_module = DAUDOUT_SET_MODULE;
	
	if(AudStreamType==STAUD_STREAM_CONTENT_AC3)
	{
		msg_p -> contents.DAUDSet.OutStatus=TRUE;
	}
	else
	{
		msg_p -> contents.DAUDSet.OutStatus=FALSE;
	}
	
	STTBX_Print(("\nYxlInfo:msg send out ->AudStreamType=%d",AudStreamType));
	message_send (pstDigitalAUDSetMsgueue , msg_p );
	return	FALSE;
}





/*end yxl 2006-06-13 add below section*/

#endif /*yxl 2006-06-15 add this line*/

/*---------------------------------------------------------------------
 * Function     : HDMI_EnableInfoframe
 * Description : Enable HDMI info frame transmission
 * Input          : HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_EnableInfoframe(U32 HandleId)
{
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;

    /* Send the AVI info frame */
    /* ======================= */
    ErrCode = CH_HDMI_FillAVI(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the AVI Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* This is mandatory for proper audio workaround operation */
    /* Send the audio info frame */
    /* ========================= */
    ErrCode = CH_HDMI_FillAudio(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the AUDIO Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* This is not mondatory for HDMI certfication*/
    /* Send the MPEG Source info Frame*/
    /*================================*/
    ErrCode = CH_HDMI_FillMS(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the MPEG SOURCE Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Send the SPD info Frame*/
    /*========================*/
    ErrCode = CH_HDMI_FillSPD(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the SPD Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Send the SPD info Frame*/
    /*========================*/
    ErrCode = CH_HDMI_FillVS(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the VS Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }
    return (ST_NO_ERROR);
}/* HDMI_EnableInfoframe() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillAVI
 * Description : Fill AVI Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_FillAVI(U32 DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_AVIInfoFrame_t HDMI_AVIInfoFrame;
    STVTG_TimingMode_t    VTGTimingMode;
    STVTG_ModeParams_t    VTGModeParams;

    memset((void *)&HDMI_AVIInfoFrame, 0, sizeof(STHDMI_AVIInfoFrame_t));

    HDMI_AVIInfoFrame.AVI861B.FrameType                  = STHDMI_INFOFRAME_TYPE_AVI;
    HDMI_AVIInfoFrame.AVI861B.FrameVersion               = 2;
    HDMI_AVIInfoFrame.AVI861B.FrameLength                = 13;
    HDMI_AVIInfoFrame.AVI861B.HasActiveFormatInformation = TRUE;
    HDMI_AVIInfoFrame.AVI861B.OutputType                 = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
    HDMI_AVIInfoFrame.AVI861B.ScanInfo                   = STHDMI_SCAN_INFO_OVERSCANNED;
    HDMI_AVIInfoFrame.AVI861B.Colorimetry                = STVOUT_ITU_R_601;
    HDMI_AVIInfoFrame.AVI861B.ActiveAspectRatio          = STGXOBJ_ASPECT_RATIO_16TO9;
    HDMI_AVIInfoFrame.AVI861B.PictureScaling             = STHDMI_PICTURE_NON_UNIFORM_SCALING;

    ErrCode = STVTG_GetMode(/*VTG_Handler[MAIN_PATH]*/HDMI_Context[DeviceId].VTG_Handle, &VTGTimingMode, &VTGModeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                       "STVTG_GetMode()=%s",
                       GetErrorText(ErrCode)));
    }
    else
    {
        switch (VTGTimingMode)
        {
            case STVTG_TIMING_MODE_480I60000_13514 :
            case STVTG_TIMING_MODE_480I59940_13500 :
            case STVTG_TIMING_MODE_480P60000_27027 :
            case STVTG_TIMING_MODE_480P30000_13514 :
            case STVTG_TIMING_MODE_480P24000_10811 :
            case STVTG_TIMING_MODE_480P59940_27000 :  /* no break */
            case STVTG_TIMING_MODE_480P29970_13500 :
            case STVTG_TIMING_MODE_480P23976_10800 :
            case STVTG_TIMING_MODE_480P60000_24570:  /* no break */
            case STVTG_TIMING_MODE_480P60000_25200:  /* no break */
            case STVTG_TIMING_MODE_480P59940_25175:  /* no break */
                 HDMI_AVIInfoFrame.AVI861B.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
                 break;
            case STVTG_TIMING_MODE_1080I60000_74250:  /* no break */
            case STVTG_TIMING_MODE_1080I59940_74176:  /* no break */
            case STVTG_TIMING_MODE_1080I50000_74250:  /* no break */
            case STVTG_TIMING_MODE_1080I50000_74250_1: /* no break */
            case STVTG_TIMING_MODE_720P60000_74250: /* no break */
            case STVTG_TIMING_MODE_720P59940_74176: /* no break */
            case STVTG_TIMING_MODE_720P50000_74250:
                 HDMI_AVIInfoFrame.AVI861B.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
                 break;
            default:
                 HDMI_AVIInfoFrame.AVI861B.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
                 break;
        }

        ErrCode = STHDMI_FillAVIInfoFrame(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_AVIInfoFrame);
        if (ErrCode != ST_NO_ERROR)
        {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                                  "THDMI_FillAVI(%s)=%s",
                                                  HDMI_Names[0],
                                                  GetErrorText(ErrCode)));
     }
    }

    return (ErrCode);
}/* HDMI_FillAVI() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillSPD
 * Description : Fill SPD Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_FillSPD(U32 DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_SPDInfoFrame_t HDMI_SPDInfoFrame;

    memset((void *)&HDMI_SPDInfoFrame, 0, sizeof(STHDMI_SPDInfoFrame_t));

    HDMI_SPDInfoFrame.FrameLength = 25 ;
    HDMI_SPDInfoFrame.FrameType = STHDMI_INFOFRAME_TYPE_SPD;
    HDMI_SPDInfoFrame.FrameVersion = 2;

    strncpy(HDMI_SPDInfoFrame.ProductDescription, "ST7109", 8);
    strncpy(HDMI_SPDInfoFrame.VendorName, "STM", 16);
    HDMI_SPDInfoFrame.SourceDeviceInfo =  STHDMI_SPD_DEVICE_DIGITAL_STB;

    ErrCode = STHDMI_FillSPDInfoFrame(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_SPDInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillSPD(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillSPD() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillAudio
 * Description : Fill Audio Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_FillAudio(U32 DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_AUDIOInfoFrame_t HDMI_AUDIOInfoFrame;

    memset((void *)&HDMI_AUDIOInfoFrame, 0, sizeof(STHDMI_AUDIOInfoFrame_t));

    HDMI_AUDIOInfoFrame.FrameType         = STHDMI_INFOFRAME_TYPE_AUDIO;
    HDMI_AUDIOInfoFrame.FrameVersion      = 2;
    HDMI_AUDIOInfoFrame.CodingType        = STAUD_STREAM_CONTENT_NULL;
    HDMI_AUDIOInfoFrame.SampleSize        = 0xFF;
    HDMI_AUDIOInfoFrame.DownmixInhibit    = FALSE;

    ErrCode = STHDMI_FillAudioInfoFrame(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_AUDIOInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillAudio(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillAudio() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillMS
 * Description : Fill MS Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_FillMS(U32 DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_MPEGSourceInfoFrame_t HDMI_MPEGSourceInfoFrame;

    memset((void *)&HDMI_MPEGSourceInfoFrame, 0, sizeof(STHDMI_MPEGSourceInfoFrame_t));

    HDMI_MPEGSourceInfoFrame.FrameLength = 10;
    HDMI_MPEGSourceInfoFrame.FrameType = STHDMI_INFOFRAME_TYPE_MPEG_SOURCE;
    HDMI_MPEGSourceInfoFrame.FrameVersion = 2;
    HDMI_MPEGSourceInfoFrame.IsFieldRepeated = FALSE;
    HDMI_MPEGSourceInfoFrame.MepgFrameType = STVID_MPEG_FRAME_I;  /* Should be updated for every frame displayed. */
    HDMI_MPEGSourceInfoFrame.MPEGBitRate = 10000000;

    ErrCode = STHDMI_FillMSInfoFrame(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_MPEGSourceInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillMS(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillMS() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillVS
 * Description : Fill VS Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t CH_HDMI_FillVS(U32 DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_VendorSpecInfoFrame_t HDMI_VendorSpecInfoFrame;
    char VSBuffer[3] = {'S','T','M'};

    memset((void *)&HDMI_VendorSpecInfoFrame, 0, sizeof(STHDMI_VendorSpecInfoFrame_t));

    HDMI_VendorSpecInfoFrame.FrameLength = 1;
    HDMI_VendorSpecInfoFrame.FrameType = STHDMI_INFOFRAME_TYPE_VENDOR_SPEC;
    HDMI_VendorSpecInfoFrame.FrameVersion = 2;
    HDMI_VendorSpecInfoFrame.RegistrationId = 0;
    HDMI_VendorSpecInfoFrame.VSPayload_p = "STM";
    memcpy(HDMI_VendorSpecInfoFrame.VSPayload_p, VSBuffer, 3);

    ErrCode = STHDMI_FillVSInfoFrame(HDMI_Context[DeviceId].HDMI_Handle, &HDMI_VendorSpecInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillVS(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillVS() */
/*end of file*/

