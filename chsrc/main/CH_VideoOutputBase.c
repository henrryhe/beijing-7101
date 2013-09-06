/*
  (c) Copyright changhong
  Name:CH_VideoOutputBase.c
  Description:实现长虹7100 platform视频输出类型、定时模式等转换的底层控制和接口函数
	
  Authors:Yixiaoli
  Date          Remark
  2007-05-14   Created 

  Modifiction : 



*/

#include "appltype.h"
#include "initterm.h"
#include "stddefs.h"
#include "stcommon.h"

#include "..\errors\errors.h"
#include <string.h>
#include <channelbase.h>

#include "..\dbase\vdbase.h"
#include "..\key\keymap.h"
#include "..\main\CH_VideoOutputBase.h"
#include "..\main\ch_hdmi.h"



STVID_DisplayAspectRatioConversion_t CH_GetSTVideoARConversionFromCH(CH_VideoARConversion ARConversion)
{
	STVID_DisplayAspectRatioConversion_t ConversionTemp;
	switch(ARConversion)
	{
	case CONVERSION_LETTETBOX:
		ConversionTemp=STVID_DISPLAY_AR_CONVERSION_LETTER_BOX;
		break;
	case CONVERSION_PANSCAN:
		ConversionTemp=STVID_DISPLAY_AR_CONVERSION_PAN_SCAN;
		break;
	case CONVERSION_COMBINED:
		ConversionTemp=STVID_DISPLAY_AR_CONVERSION_COMBINED;
		break;
	case CONVERSION_FULLSCREEN:
		ConversionTemp=STVID_DISPLAY_AR_CONVERSION_IGNORE;
		break;
	default:
		ConversionTemp=STVID_DISPLAY_AR_CONVERSION_IGNORE;
	}
	return ConversionTemp;
}




STVTG_TimingMode_t CH_GetSTTimingModeFromCH(CH_VideoOutputMode Mode)
{
	STVTG_TimingMode_t TimingModeTemp;
	switch(Mode)	
	{
	case MODE_1080I60:
		TimingModeTemp=STVTG_TIMING_MODE_1080I60000_74250;
		break;
	case MODE_1080I50:
		TimingModeTemp=STVTG_TIMING_MODE_1080I50000_74250_1;
		break;
	case MODE_720P60:
		TimingModeTemp=STVTG_TIMING_MODE_720P60000_74250;
		break;
	case MODE_720P50:
		TimingModeTemp=STVTG_TIMING_MODE_720P50000_74250;
		break;
	case MODE_720P50_VGA:
#if 0	 
		TimingModeTemp=STVTG_TIMING_MODE_720P50000_74250_VGA;
#else
		TimingModeTemp=STVTG_TIMING_MODE_720P50000_74250;/*this is for LCD*/
#endif 
		break;
	case MODE_720P50_DVI: 
#if 0 
		TimingModeTemp=STVTG_TIMING_MODE_720P50000_74250_DVI;
#else
	TimingModeTemp=STVTG_TIMING_MODE_720P50000_74250;
#endif
		break;
	case MODE_576P50:
		TimingModeTemp=STVTG_TIMING_MODE_576P50000_27000;
		break;
	case MODE_576I50:
		TimingModeTemp=STVTG_TIMING_MODE_576I50000_13500;
		break;
	default:
		TimingModeTemp=STVTG_TIMING_MODE_1080I60000_74250;
		break;
	}
	return TimingModeTemp;
}




STGXOBJ_AspectRatio_t CH_GetSTScreenARFromCH(CH_VideoOutputAspect Ratio)
{
	STGXOBJ_AspectRatio_t AspectRatioTemp;
	switch(Ratio)
	{
	case ASPECTRATIO_4TO3:
		AspectRatioTemp=STGXOBJ_ASPECT_RATIO_4TO3;
		break;
	case ASPECTRATIO_16TO9:
		AspectRatioTemp=STGXOBJ_ASPECT_RATIO_16TO9;
		break;
	default :
		AspectRatioTemp=STGXOBJ_ASPECT_RATIO_4TO3;
		break;
	}
	return AspectRatioTemp;
}




STVOUT_OutputType_t CH_GetSTVideoOutputTypeFromCH(CH_VideoOutputType Type)
{
	STVOUT_OutputType_t OutputTypeTemp;
	switch(Type)
	{
	case TYPE_RGB:
		OutputTypeTemp=STVOUT_OUTPUT_HD_ANALOG_RGB;
		break;
	case TYPE_YUV:
		OutputTypeTemp=STVOUT_OUTPUT_HD_ANALOG_YUV;
		break;
#ifdef DVI_FUNCTION  
	case TYPE_DVI:
		OutputTypeTemp=STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
		break;
#endif 
#ifdef HDMI_FUNCTION  
	case TYPE_HDMI:
		OutputTypeTemp=STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
		break;
#endif 
	case TYPE_CVBS:
		OutputTypeTemp=STVOUT_OUTPUT_ANALOG_CVBS;
		break;
	#ifdef MODEY_576I 
	case TYPE_SDYUV:
		OutputTypeTemp=STVOUT_OUTPUT_ANALOG_YUV;
		break;		
	#endif
	default :
		OutputTypeTemp=STVOUT_OUTPUT_ANALOG_CVBS;
	}
	return OutputTypeTemp;
}




#ifdef MODEY_576I 
BOOL CH_SetSDVideoOutputType(CH_VideoOutputType OutputType)
{
	ST_ErrorCode_t ErrCode;
	STVOUT_TermParams_t TermParams;
	TermParams.ForceTerminate=TRUE;

#if 1 
	ErrCode = STVMIX_Disable(VMIXHandle[VMIX_AUX]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[%s]=%s\n", GetErrorText(ErrCode) ));
	}

	ErrCode = STVOUT_Disable(VOUTHandle[VOUT_AUX]);
	if (ErrCode != ST_NO_ERROR)
    {
    	STTBX_Print(("STVOUT_Disable()=%s\n", GetErrorText(ErrCode)));
	}

	ErrCode=STVOUT_Term(VOUTDeviceName[VOUT_AUX],&TermParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Term()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}

	VOUT_Setup(VOUT_AUX,OutputType);
	ErrCode = STVMIX_Enable(VMIXHandle[VMIX_AUX]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[%s]=%s\n", GetErrorText(ErrCode) ));
	}
	{
		STTBX_Print(("\n YxlInfoA:value=%d %d\n",
			STSYS_ReadRegDev32LE((void*)0x38214014),STSYS_ReadRegDev32LE((void*)0x3821417c)));
	}
	


#endif  


#if 1
	ErrCode=STVOUT_Disable(VOUTHandle[VOUT_MAIN]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Disable()=%s\n",GetErrorText(ErrCode)));	
	/*	return TRUE;*/
	}

#endif
	ErrCode=STVOUT_Enable(VOUTHandle[VOUT_AUX]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Enable()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
	
	
	STSYS_WriteRegDev32LE((void*)0x38214014, STSYS_ReadRegDev32LE((void*)0x38214014) & (~0x0E));
	{
		STTBX_Print(("\n YxlInfoC:value=%d %d\n",
			STSYS_ReadRegDev32LE((void*)0x38214014),STSYS_ReadRegDev32LE((void*)0x3821417c)));
	}





	return FALSE;
}
#endif






BOOL CH_SetVideoOutputType(CH_VideoOutputType OutputType)
{
	ST_ErrorCode_t ErrCode;
	STVOUT_TermParams_t TermParams;
	TermParams.ForceTerminate=TRUE;

	ErrCode = STVMIX_Disable(VMIXHandle[VMIX_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[%s]=%s\n", GetErrorText(ErrCode) ));
	}

	ErrCode = STVOUT_Disable(VOUTHandle[VOUT_MAIN]);
	if (ErrCode != ST_NO_ERROR)
    {
    	STTBX_Print(("STVOUT_Disable()=%s\n", GetErrorText(ErrCode)));
	}

	ErrCode=STVOUT_Term(VOUTDeviceName[VOUT_MAIN],&TermParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Term()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}

	VOUT_Setup(VOUT_MAIN,OutputType);
	ErrCode = STVMIX_Enable(VMIXHandle[VMIX_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[%s]=%s\n", GetErrorText(ErrCode) ));
	}

#if 0 /*yxl 2005-12-06 add this section,yxl 2007-05-16 cancel below section*/
	ErrCode=STVOUT_Enable(VOUTHandle[VOUT_MAIN]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVOUT_Enable()=%s\n",GetErrorText(ErrCode)));	
		return TRUE;
	}
#endif  /*yxl 2005-12-06 add this section,yxl 2007-05-16 cancel below section*/

	return FALSE;

}


BOOL CH_SetVideoOut(CH_VideoOutputType Type,CH_VideoOutputMode Mode,
	CH_VideoOutputAspect AspectRatio,CH_VideoARConversion ARConversion) 
{
	STVOUT_OutputType_t OutputTypeTemp;
	STVTG_TimingMode_t TimingModeTemp;
	STGXOBJ_AspectRatio_t AspectRatioTemp;
	STVMIX_ScreenParams_t ScreenParams;
	STVID_DisplayAspectRatioConversion_t ConversionTemp;
	ST_ErrorCode_t ErrCode;
	BOOL IsTypeValid=true;
	BOOL IsModeValid=true;
	BOOL res;

	ConversionTemp=CH_GetSTVideoARConversionFromCH(ARConversion);

	if(Type==TYPE_UNCHANGED&&Mode==MODE_UNCHANGED&&AspectRatio==ASPECTRATIO_UNCHANGED)
	{
		
		return CH_SetVideoAspectRatioConversion(ConversionTemp);
	}

#ifdef MODEY_576I /*yxl 2006-06-26 add this  section*/
	if(Type==TYPE_YUV&&Mode==MODE_576I50)
	Type=TYPE_SDYUV;
#endif /*end yxl 2006-06-26 add this  section*/

	if(Type==TYPE_UNCHANGED) 	IsTypeValid=false;
	else OutputTypeTemp=CH_GetSTVideoOutputTypeFromCH(Type);

	TimingModeTemp=CH_GetSTTimingModeFromCH(Mode);
	AspectRatioTemp=CH_GetSTScreenARFromCH(AspectRatio);

#if 1 /*yxl 2007-05-16 add below section*/

	CH6_AVControl(VIDEO_BLACK,false,CONTROL_VIDEO);

#endif/*end yxl 2007-05-16 add below section*/

#ifdef CH_MUTI_RESOLUTION_RATE
       CH_DeleteOSDViewPort_HighSolution();
#else
	CH6_DeleteViewPort(CH6VPOSD);
#endif
		
	CH_SetSDVideoOutputMode(MODE_576I50,ASPECTRATIO_4TO3/*AspectRatio 20070809 change*/,ARConversion);/**/

	STTBX_Print(("\nYxlInfo:her1"));
	
	CH_SetHDVideoOutputMode(Mode,/*AspectRatio*/ASPECTRATIO_16TO9,ARConversion,Type);
	STTBX_Print(("\nYxlInfo:her11"));

#if 1 /*yxl 2007-05-16 add below section*/
	CH_SetVideoAspectRatioConversion(ConversionTemp);

	CH6_AVControl(VIDEO_DISPLAY,false,CONTROL_VIDEO);

#endif/*end yxl 2007-05-16 add below section*/
#ifdef 	CH_MUTI_RESOLUTION_RATE
        CH_CreateOSDViewPort_HighSolution();
#else
	res=CH_SetupViewPort(LAYER_GFX1,AspectRatio,Mode);
	if(res==TRUE) return res;
	CH_SetViewPortAlpha(CH6VPOSD,GLOBAL_VIEWPORT_ALPHA);


	if(CH6_ShowViewPort(&CH6VPOSD))
	{
		STTBX_Print(("\nCH6_ShowViewPort(&CH6VPOSD) is not successful"));
	}
	else	STTBX_Print(("\nCH6_ShowViewPort(&CH6VPOSD) is successful"));
#endif
#if 0 /*yxl 2007-05-16 add below section*/
	if((Type==TYPE_YUV||Type==TYPE_RGB))
	{
		return CH_SetVideoOutputType(Type);
	}
#endif /*end yxl 2007-05-16 add below section*/

	return FALSE;
}





BOOL CH_SetSDVideoOutputMode(CH_VideoOutputMode Mode,CH_VideoOutputAspect Ratio,
							 CH_VideoOutputMode VConversion)
{
	ST_ErrorCode_t ErrCode;
	STLAYER_LayerParams_t LayerParams;
	STVMIX_ScreenParams_t ScreenParams;
	STVMIX_LayerDisplayParams_t LayerDisParams[3];
	STVMIX_LayerDisplayParams_t *VMIXLayerArray[3];
	STVID_ViewPortParams_t ViewPortParams;
	STLAYER_TermParams_t    TermParams;
	STVMIX_TermParams_t	MixTermParams;
	STVID_Freeze_t      Freeze;

	BOOL res;


	STVTG_TimingMode_t TimingMode;
	STGXOBJ_AspectRatio_t AspectRatio;
	STVID_DisplayAspectRatioConversion_t ARConversion;

	TimingMode=CH_GetSTTimingModeFromCH(Mode);
	AspectRatio=CH_GetSTScreenARFromCH(Ratio);
	ARConversion=CH_GetSTVideoARConversionFromCH(VConversion);

#if 0
	ErrCode= STVID_CloseViewPort(VIDVPHandle[VID_MPEG2][LAYER_VIDEO2]);
  	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVID_CloseViewPort(AUX)=%s\n", GetErrorText(ErrCode) ));
	}
#endif



	ErrCode = STVMIX_Disable(VMIXHandle[VMIX_AUX]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[AUX]=%s\n", GetErrorText(ErrCode) ));
	}


	ErrCode = STVMIX_DisconnectLayers(VMIXHandle[VMIX_AUX]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_DisconnectLayers[AUX]=%s\n", GetErrorText(ErrCode) ));
	}



	ErrCode = STVOUT_Disable(VOUTHandle[VOUT_AUX]);
	if (ErrCode != ST_NO_ERROR)
    {
    	STTBX_Print(("STVOUT_Disable(AUX)=%s\n", GetErrorText(ErrCode)));
	}



	memset((void *)&TermParams, 0, sizeof(STLAYER_TermParams_t));
    TermParams.ForceTerminate = TRUE;
	ErrCode= STLAYER_Close(LAYERHandle[LAYER_GFX2]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print((" STLAYER_Close(%s)=%s",LAYER_DeviceName[LAYER_GFX2],GetErrorText(ErrCode)));
	}

   	ErrCode = STLAYER_Term(LAYER_DeviceName[LAYER_GFX2], &TermParams );
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_Term(%s)=%s",LAYER_DeviceName[LAYER_GFX2],GetErrorText(ErrCode)));

	}

	res=LAYER_Setup(LAYER_GFX2,Ratio);
	if(res==TRUE) return res;



	LayerParams.Width=gVTGAUXModeParam.ActiveAreaWidth;
	LayerParams.Height=gVTGAUXModeParam.ActiveAreaHeight;
	LayerParams.AspectRatio=AspectRatio;
	LayerParams.ScanType=gVTGAUXModeParam.ScanType;
	ErrCode=STLAYER_SetLayerParams(LAYERHandle[LAYER_VIDEO2],&LayerParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_SetLayerParams()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}


	ScreenParams.AspectRatio = AspectRatio;
	ScreenParams.ScanType    = gVTGAUXModeParam.ScanType;
	ScreenParams.FrameRate   = gVTGAUXModeParam.FrameRate;
	ScreenParams.Width       = gVTGAUXModeParam.ActiveAreaWidth;
	ScreenParams.Height      = gVTGAUXModeParam.ActiveAreaHeight;
#if 0 
	ScreenParams.XStart      = gVTGAUXModeParam.ActiveAreaXStart;
	ScreenParams.YStart      = gVTGAUXModeParam.ActiveAreaYStart; 
#else
	ScreenParams.XStart      = gVTGAUXModeParam.DigitalActiveAreaXStart;
	ScreenParams.YStart      = gVTGAUXModeParam.DigitalActiveAreaYStart; 
	if (ScreenParams.YStart%2) /* if odd */
		ScreenParams.YStart++;	
#endif

	ErrCode = STVMIX_SetScreenParams(VMIXHandle[VMIX_AUX], &ScreenParams);
	if (ErrCode!= ST_NO_ERROR)
	{
		STTBX_Print(("VMIX_SetScreenParams(VMIX_AUX)=%s\n", GetErrorText(ErrCode) ));
		return TRUE;
	}

	ErrCode = STVOUT_Enable(VOUTHandle[VOUT_AUX]);
	if (ErrCode != ST_NO_ERROR)
    {
    	STTBX_Print(("STVOUT_Disable(AUX)=%s\n", GetErrorText(ErrCode)));
	}

	ErrCode = STVMIX_Enable(VMIXHandle[VMIX_AUX]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[AUX]=%s\n", GetErrorText(ErrCode) ));
	}

	strcpy(LayerDisParams[0].DeviceName,LAYER_DeviceName[LAYER_VIDEO2]);
	strcpy(LayerDisParams[1].DeviceName,LAYER_DeviceName[LAYER_GFX2]);

	LayerDisParams[0].ActiveSignal=LayerDisParams[1].ActiveSignal=LayerDisParams[2].ActiveSignal=FALSE;

	VMIXLayerArray[0]=&LayerDisParams[0];
	VMIXLayerArray[1]=&LayerDisParams[1];
	task_delay(8000);
	ErrCode=STVMIX_ConnectLayers(VMIXHandle[VMIX_AUX],
		(const STVMIX_LayerDisplayParams_t*  const *)VMIXLayerArray,2);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_ConnectLayers(AUX)=%s",GetErrorText(ErrCode)));
		return TRUE;
	}


#if 0 /*yxl 2007-05-16 cancel below section*/
	ErrCode = STVID_SetDisplayAspectRatioConversion(VIDVPHandle[VID_MPEG2][LAYER_VIDEO2], ARConversion);
	if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_SetDisplayAspectRatioConversion()=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }

		STTBX_Print(("\nYxlInfo:YA-15"));
#endif /*end yxl 2007-05-16 cancel below section*/

	return FALSE;
}



BOOL CH_SetHDVideoOutputMode(CH_VideoOutputMode Mode,CH_VideoOutputAspect Ratio  ,
							 CH_VideoARConversion  VConversion, 
							 CH_VideoOutputType Type)
{
	ST_ErrorCode_t ErrCode;
	STLAYER_LayerParams_t LayerParams;
	STVMIX_ScreenParams_t ScreenParams;
	STVMIX_LayerDisplayParams_t LayerDisParams[3];
	STVMIX_LayerDisplayParams_t *VMIXLayerArray[3];
	STVID_ViewPortParams_t ViewPortParams;
	STLAYER_TermParams_t    TermParams;
	STVMIX_TermParams_t	VMIXTermParams;
	STVOUT_TermParams_t	VOUTTermParams;
	STVID_StartParams_t VIDStartParams;
	STVOUT_State_t State;


	BOOL res;
    STVID_Freeze_t      Freeze;
	STVTG_OpenParams_t OpenParams;
	STVTG_TimingMode_t TimingMode;
	STGXOBJ_AspectRatio_t AspectRatio;
	STVID_DisplayAspectRatioConversion_t ARConversion;

	TimingMode=CH_GetSTTimingModeFromCH(Mode);
	AspectRatio=CH_GetSTScreenARFromCH(Ratio);
	ARConversion=CH_GetSTVideoARConversionFromCH(VConversion);



	ErrCode = STVMIX_Disable(VMIXHandle[VMIX_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Disable[]=%s\n", GetErrorText(ErrCode) ));
	}

	ErrCode = STVMIX_DisconnectLayers(VMIXHandle[VMIX_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_DisconnectLayers[]=%s\n", GetErrorText(ErrCode) ));
	}

	ErrCode = STVMIX_Close(VMIXHandle[VMIX_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Close[]=%s\n", GetErrorText(ErrCode) ));
	}
  
	ErrCode=STVTG_Close(VTGHandle[VTG_MAIN]);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_Close()=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

	ErrCode = STVOUT_Disable(VOUTHandle[VOUT_MAIN]);
	if (ErrCode != ST_NO_ERROR)
    {
    	STTBX_Print(("STVOUT_Disable()=%s\n", GetErrorText(ErrCode)));
	}


#if 1 /*yxl 2006-05-16 move this from top*/
   VMIXTermParams.ForceTerminate = TRUE;
	ErrCode = STVMIX_Term(VMIXDeviceName[VMIX_MAIN], &VMIXTermParams );
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Term[]=%s\n", GetErrorText(ErrCode) ));
	}
#endif 

	ErrCode = STVOUT_Close(VOUTHandle[VOUT_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVOUT_Close[%s]=%s\n", GetErrorText(ErrCode) ));
	}

	VOUTTermParams.ForceTerminate = TRUE;
	ErrCode = STVOUT_Term(VOUTDeviceName[VOUT_MAIN],&VOUTTermParams);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVOUT_Term[%s]=%s\n", GetErrorText(ErrCode) ));
	}

	task_delay(50 * ST_GetClocksPerSecondLow() /1000);

#if 0  /*yxl 2008-09-16 move below section to oher */
	res=VOUT_Setup(VOUT_MAIN,Type);
	if(res==TRUE) return res;
	
#endif /*end yxl 2008-09-16 move below section to oher */

#if 1/*yxl 2007-05-16 add below section*/

	CHHDMI_Term();

	ErrCode = STVOUT_Disable(VOUTHandle[VOUT_HDMI]);
	if (ErrCode != ST_NO_ERROR)
    {
    	STTBX_Print(("STVOUT_Disable()=%s\n", GetErrorText(ErrCode)));
	}
	ErrCode = STVOUT_Close(VOUTHandle[VOUT_HDMI]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVOUT_Close[%s]=%s\n", GetErrorText(ErrCode) ));
	}

	VOUTTermParams.ForceTerminate = TRUE;
	ErrCode = STVOUT_Term(VOUTDeviceName[VOUT_HDMI],&VOUTTermParams);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVOUT_Term[%s]=%s\n", GetErrorText(ErrCode) ));
	}

	task_delay(50 * ST_GetClocksPerSecondLow() /1000);

	res=VOUT_Setup(VOUT_HDMI,TYPE_HDMI);
	if(res==TRUE) return res;


#endif	/*end yxl 2007-05-16 add below section*/
	
	ErrCode=STVTG_Open(VTGDeviceName[VTG_MAIN],&OpenParams,&VTGHandle[VTG_MAIN]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STVTG_Open(VTG_MAIN)=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}


	ErrCode=STVTG_SetMode(VTGHandle[VTG_MAIN],TimingMode);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_SetMode()=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

	/* get VTG mode paramters  */
	ErrCode=STVTG_GetMode(VTGHandle[VTG_MAIN],&TimingMode,&gVTGModeParam);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVTG_GetMode()=%s\n",GetErrorText(ErrCode)));
		return TRUE;
	}

#if 1  /*yxl 2008-09-16 move below section from other */
	res=VOUT_Setup(VOUT_MAIN,Type);
	if(res==TRUE) return res;
	
#endif /*end yxl 2008-09-16 move below section from other */



	memset((void *)&TermParams, 0, sizeof(STLAYER_TermParams_t));
    TermParams.ForceTerminate = TRUE;
	ErrCode= STLAYER_Close(LAYERHandle[LAYER_GFX1]);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print((" STLAYER_Close(%s)=%s",LAYER_DeviceName[LAYER_GFX1],GetErrorText(ErrCode)));
	}
   	ErrCode = STLAYER_Term(LAYER_DeviceName[LAYER_GFX1], &TermParams );
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_Term(%s)=%s",LAYER_DeviceName[LAYER_GFX1],GetErrorText(ErrCode)));

	}

	res=LAYER_Setup(LAYER_GFX1,Ratio);
	if(res==TRUE) return res;

	LayerParams.Width=gVTGModeParam.ActiveAreaWidth;/*1920*/
	LayerParams.Height=gVTGModeParam.ActiveAreaHeight;/*1080*/
	LayerParams.AspectRatio=AspectRatio;
	LayerParams.ScanType=gVTGModeParam.ScanType;
	ErrCode=STLAYER_SetLayerParams(LAYERHandle[LAYER_VIDEO1],&LayerParams);
	if(ErrCode!=ST_NO_ERROR) 
	{
		STTBX_Print(("STLAYER_SetLayerParams()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

	res=MIX_Setup(VMIX_MAIN,Ratio);
	if(res==TRUE) return res;


	ScreenParams.AspectRatio = AspectRatio;
    ScreenParams.ScanType    = gVTGModeParam.ScanType;
    ScreenParams.FrameRate   = gVTGModeParam.FrameRate;
    ScreenParams.Width       = gVTGModeParam.ActiveAreaWidth;
    ScreenParams.Height      = gVTGModeParam.ActiveAreaHeight;


#if 0 
	ScreenParams.XStart      = gVTGModeParam.ActiveAreaXStart; 
    ScreenParams.YStart      = gVTGModeParam.ActiveAreaYStart;

#else
	ScreenParams.XStart      = gVTGModeParam.DigitalActiveAreaXStart;
	ScreenParams.YStart      = gVTGModeParam.DigitalActiveAreaYStart; 
	if (ScreenParams.YStart%2) /* if odd */
		ScreenParams.YStart++;	
#endif

	ErrCode = STVMIX_SetScreenParams(VMIXHandle[VMIX_MAIN], &ScreenParams);
    if (ErrCode!= ST_NO_ERROR)
    {
        STTBX_Print(("VMIX_SetScreenParams=%s\n", GetErrorText(ErrCode) ));
        return TRUE;
    }

#if 0  /*yxl 2006-04-27  add below section,yxl 2007-05-16 cancel */
 		VIDStartParams.RealTime      = TRUE;
		VIDStartParams.UpdateDisplay = TRUE;
		VIDStartParams.StreamID      = STVID_IGNORE_ID;
		VIDStartParams.DecodeOnce    = FALSE;
		VIDStartParams.StreamType    = STVID_STREAM_TYPE_PES;
		VIDStartParams.BrdCstProfile = STVID_BROADCAST_DVB;
		ErrCode = STVID_Start(VIDHandle[VID_MPEG2], &VIDStartParams);
		STTBX_Print(("STVID_Start()=%s\n", GetErrorText(ErrCode) ));
#endif 

	ErrCode = STVMIX_Enable(VMIXHandle[VMIX_MAIN]);
	if (ErrCode != ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_Enable[%s]=%s\n", GetErrorText(ErrCode) ));
	}
	
	strcpy(LayerDisParams[0].DeviceName,LAYER_DeviceName[LAYER_VIDEO1]);
	strcpy(LayerDisParams[1].DeviceName,LAYER_DeviceName[LAYER_GFX1]);

	LayerDisParams[0].ActiveSignal=LayerDisParams[1].ActiveSignal=LayerDisParams[2].ActiveSignal=FALSE;

	VMIXLayerArray[0]=&LayerDisParams[0];
	VMIXLayerArray[1]=&LayerDisParams[1];
	VMIXLayerArray[2]=&LayerDisParams[2];
	task_delay(8000);
	ErrCode=STVMIX_ConnectLayers(VMIXHandle[VMIX_MAIN],(const STVMIX_LayerDisplayParams_t*  const *)VMIXLayerArray,2/*2*/);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STVMIX_ConnectLayers()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}

#if 0 /*yxl 2007-05-16 cancel below section*/
	/*yxl 2006-06-01 add below section*/
	ErrCode = STVID_SetDisplayAspectRatioConversion(VIDVPHandle[VID_MPEG2][LAYER_VIDEO1], ARConversion);
	if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STVID_SetDisplayAspectRatioConversion()=%s\n", GetErrorText(ErrCode)));
        return TRUE;
    }
	/*end yxl 2006-06-01 add below section*/
#endif  /*end yxl 2007-05-16 cancel below section*/
	CHHDMI_Setup();/*yxl 2007-05-16 add this line*/
	return FALSE;
}






/*****************************************************************************
Name:
BOOL CH_LEDDisPWStatus()
Description:
	Diaplay  input password status in the LED.
Parameters:

Return Value:
	TRUE:not success
	FALSE:success 
See Also:
*****************************************************************************/
BOOL CH_LEDDisPWStatus(U8 PWPos )
{
	return FALSE;
}

/*end of file*/
