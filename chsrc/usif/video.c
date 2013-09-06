#include "Stvout.h"
#include "Stddefs.h"

#include "..\main\initterm.h"
#include "..\key\key.h"
#include "..\key\keymap.h"
#include "graphicmid.h"
#include "video.h"
#include "..\dbase\vdbase.h"
#include "..\dbase\st_nvm.h"
#include "stlayer.h"

typedef enum
{
    BRIGHTNESSGAIN, 
	CONTRASTGAIN,
	TINTANGLE,
	SATURATIONGAIN    
} PSI_ColorProcessing_t;


int Lum_ConvertHD(int luminance)
{
	if(luminance<50)
	return (-(50-luminance)*2);
	else
	return (2*(luminance-50));
}

int Chr_ConvertHD(int Chroma)
{
	if(Chroma<50)
	return (-(50-Chroma)*2);
	else
	return (2*(Chroma-50));
}

int Con_ConvertHD(int contrast)
{
	return contrast;
}



U8 Lum_Convert(U8 luminance)
{
	return ((luminance * VIDEO_STEP * R_MAX)/ VIDEO_MAX);
}

U8 Chr_Convert(U8 Chroma)
{
	return ((Chroma * VIDEO_STEP * R_MAX) / VIDEO_MAX);
}

U8 Con_Convert(U8 contrast)
{
	return ((contrast * VIDEO_STEP * R_MAX)/ VIDEO_MAX -128);
}


ST_ErrorCode_t ColorProcessingMPEG(PSI_ColorProcessing_t eColorProcess,int ActualValue)/* cqj 20070801 add  only for MPEG2 high definition interface*/
{
	ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    STLAYER_PSI_t   stVPPSI;
	STLAYER_PSI_t   stReadVPPSI;
	STVID_ViewPortHandle_t VIEWPORT_Handler;
	
	VIEWPORT_Handler = VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];/*high definition interface*/

	switch(eColorProcess)
		{
			case BRIGHTNESSGAIN:
			case CONTRASTGAIN:
						stVPPSI.VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI.VideoFiltering=STLAYER_VIDEO_LUMA_BC;
						stReadVPPSI.VideoFiltering=STLAYER_VIDEO_LUMA_BC;
				break;
			case TINTANGLE:
						stVPPSI.VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI.VideoFiltering=STLAYER_VIDEO_CHROMA_TINT;
						stReadVPPSI.VideoFiltering=STLAYER_VIDEO_CHROMA_TINT;
				break;
			case SATURATIONGAIN:
						stVPPSI.VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI.VideoFiltering=STLAYER_VIDEO_CHROMA_SAT;
						stReadVPPSI.VideoFiltering=STLAYER_VIDEO_CHROMA_SAT;
				break;
			default:
				break;
		}

	ErrCode=STVID_GetViewPortPSI(VIEWPORT_Handler,&stReadVPPSI);/* Get all initial PSI values */
		
	if (eColorProcess==BRIGHTNESSGAIN)/* Keep Contrast value */
		{
			stVPPSI.VideoFilteringParameters.BCParameters.ContrastGainControl=stReadVPPSI.VideoFilteringParameters.BCParameters.ContrastGainControl;
		}
	else if (eColorProcess==CONTRASTGAIN)/* Keep Br value */
		{
			stVPPSI.VideoFilteringParameters.BCParameters.BrightnessGainControl=stReadVPPSI.VideoFilteringParameters.BCParameters.BrightnessGainControl;
		}
	
	if (eColorProcess==BRIGHTNESSGAIN)/*set value for new color process*/
		{
			stVPPSI.VideoFilteringParameters.BCParameters.BrightnessGainControl=ActualValue;

		}
	else if (eColorProcess==CONTRASTGAIN)
		{
			stVPPSI.VideoFilteringParameters.BCParameters.ContrastGainControl=ActualValue;

		}
	else if (eColorProcess==TINTANGLE)
		{
			stVPPSI.VideoFilteringParameters.TintParameters.TintRotationControl=ActualValue;

		}
	else if (eColorProcess==SATURATIONGAIN)
		{
			stVPPSI.VideoFilteringParameters.SatParameters.SaturationGainControl=ActualValue;

		}
	
	ErrCode=STVID_SetViewPortPSI(VIEWPORT_Handler,&stVPPSI);/*carry new value into effect*/

	STTBX_Print(( "\n New Value for %d is : %d",eColorProcess,ActualValue));		
	return ErrCode;

}











void BootSETHDVideo(void)/* cqj 20071121 modify waitting for check "  优化性能"*/
{
      int tempS ;
	int tempT ;
	int tempB ;
	int tempC ;
      static boolean b_first = true;

	  do_report(0,"\n call BootSETHDVideo\n");
	  
	  if(b_first == true)
	  	{
	 tempS = Chr_ConvertHD((int)pstBoxInfo->abiTVSaturation);
	 tempT = Chr_ConvertHD((int)pstBoxInfo->abiTVContrast);
	 tempB = Lum_ConvertHD((int)pstBoxInfo->abiTVBright);
	 tempC = Con_ConvertHD((int)pstBoxInfo->abiTVContrast);
	#if 0
	tempS = tempS == 0 ? 1 :tempS;
	//tempT= tempT== 0 ? 1 :tempT;
	tempB= tempB == 0 ? 1 :tempB;
	tempC= tempC == 50 ? 51 :tempC;
#endif
	ColorProcessingMPEG(SATURATIONGAIN,tempS);
	//ColorProcessingMPEG(TINTANGLE,tempT);
	ColorProcessingMPEG(BRIGHTNESSGAIN,tempB);
	ColorProcessingMPEG(CONTRASTGAIN,tempC);
	b_first = false;
	  	}
}





