#include "Stvout.h"
#include "Stddefs.h"

#include "..\main\initterm.h"
#include "..\key\key.h"
#include "..\key\keymap.h"
#include "graphicmid.h"
#include "video.h"
#include "..\dbase\vdbase.h"
#include "..\dbase\st_nvm.h"
#if 0
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
#else
#include "stlayer.h"
typedef enum
{
    BRIGHTNESSGAIN, 
	CONTRASTGAIN,
	TINTANGLE,
	SATURATIONGAIN,
    
} PSI_ColorProcessing_t;

#define PSIMinGain -100 
#define PSIMaxGain  100
#define MaxVP_Hdl		2
#if 0
int Lum_Convert(int luminance)
{
	if(luminance<50)
	return (-(50-luminance)*2);
	else
	return (2*(luminance-50));
}

int Chr_Convert(int Chroma)
{
	if(Chroma<50)
	return (-(50-Chroma)*2);
	else
	return (2*(Chroma-50));
}

int Con_Convert(int contrast)
{
	return contrast;
}
#endif


void CH6_DrawVideoText(int ItemSelect,int TextColor,int TextBKColor,char* String1,char* String2)
{	
	int xNum=VIDEO_TXSTART_X+VIDEO_BAR_WIDTH-CHARACTER_WIDTH*3;
	int yNum=VIDEO_TXSTART_Y;
	
	CH6m_DrawRectangle(
		GFXHandle,
		CH6VPOSD.ViewPortHandle,&gGC,
		VIDEO_TXSTART_X,
		VIDEO_TXSTART_Y+ItemSelect*VIDEO_GAP,
		VIDEO_BAR_WIDTH,
		VIDEO_TXBCK_HEIGHT,		
		TextBKColor,
		TextBKColor,
		1);
	CH6_DisRectTxtInfoNobck(
		GFXHandle,
		CH6VPOSD.ViewPortHandle,
		VIDEO_TXSTART_X+8,
		VIDEO_TXSTART_Y+ItemSelect*VIDEO_GAP,
		CHARACTER_WIDTH*2*12,
		VIDEO_TXBCK_HEIGHT,
		String1,		
		TextColor,
		0);	
	CH6_DisRectTxtInfoNobck(
		GFXHandle,
		CH6VPOSD.ViewPortHandle,
		xNum,
		yNum+ItemSelect*VIDEO_GAP,
		CHARACTER_WIDTH*3,
		VIDEO_TXBCK_HEIGHT,
		String2,		
		TextColor,		
		0);	
}
#endif
/*-----------------------------------------------------------------------------
 * Function    : ColorProcessingTest
 * Description : Color processing including saturation test
 * Input       : ViewPortHandle
 * Output      : None
 * Return      : ErrCode
 * ---------------------------------------------------------------------------*/
ST_ErrorCode_t ColorProcessingTest(/*STVID_ViewPortHandle_t ViewPortHandle,*/PSI_ColorProcessing_t eColorProcess,/*STLAYER_VideoFiltering_t eFiltering,*/int ActualValue)
{
	ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    STLAYER_PSI_t   *stVPPSI, *stVPPSI_OSD;
	STLAYER_PSI_t   *stReadVPPSI, *stReadVPPSI_OSD;
	STVID_ViewPortHandle_t *VIEWPORT_Handler[2];
	STLAYER_ViewPortHandle_t *VIEWPORTOSD_Handler[2];
	int Loop_i;


	
	stVPPSI=malloc(MaxVP_Hdl*sizeof(STLAYER_PSI_t));
	stReadVPPSI=malloc(MaxVP_Hdl*sizeof(STLAYER_PSI_t));
	stVPPSI_OSD=malloc(MaxVP_Hdl*sizeof(STLAYER_PSI_t));
	stReadVPPSI_OSD=malloc(MaxVP_Hdl*sizeof(STLAYER_PSI_t));
	
	VIEWPORT_Handler[0]=&VIDVPHandle[VID_MPEG2][LAYER_VIDEO2];
	VIEWPORT_Handler[1]=&VIDVPHandle[VID_MPEG2][LAYER_VIDEO1];

	VIEWPORTOSD_Handler[0]=&(CH6VPOSD.LayerViewPortHandle[0]);
	VIEWPORTOSD_Handler[1]=&(CH6VPOSD.LayerViewPortHandle[1]);
	
	
	switch(eColorProcess)
		{
			case BRIGHTNESSGAIN:
			case CONTRASTGAIN:
				for (Loop_i=0;Loop_i<MaxVP_Hdl;Loop_i++)
					{
						stVPPSI[Loop_i].VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI[Loop_i].VideoFiltering=STLAYER_VIDEO_LUMA_BC;
						stReadVPPSI[Loop_i].VideoFiltering=STLAYER_VIDEO_LUMA_BC;
						
						stVPPSI_OSD[Loop_i].VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI_OSD[Loop_i].VideoFiltering=STLAYER_VIDEO_LUMA_BC;
						stReadVPPSI_OSD[Loop_i].VideoFiltering=STLAYER_VIDEO_LUMA_BC;						
					}
				break;
			case TINTANGLE:
				for (Loop_i=0;Loop_i<MaxVP_Hdl;Loop_i++)
					{
						stVPPSI[Loop_i].VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_TINT;
						stReadVPPSI[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_TINT;

						stVPPSI_OSD[Loop_i].VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI_OSD[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_TINT;
						stVPPSI_OSD[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_TINT;
					}
				break;
			case SATURATIONGAIN:
				for (Loop_i=0;Loop_i<MaxVP_Hdl;Loop_i++)
					{
						stVPPSI[Loop_i].VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_SAT;
						stReadVPPSI[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_SAT;

						stVPPSI_OSD[Loop_i].VideoFilteringControl=STLAYER_ENABLE_MANUAL;
						stVPPSI_OSD[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_SAT;
						stVPPSI_OSD[Loop_i].VideoFiltering=STLAYER_VIDEO_CHROMA_SAT;
					}
				break;
			default:
				break;
		}

	/* Get all initial PSI values */
		for (Loop_i=0;Loop_i<2;Loop_i++)
			{
				ErrCode=STVID_GetViewPortPSI(*VIEWPORT_Handler[Loop_i],&stReadVPPSI[Loop_i]);
			    ErrCode=STLAYER_GetViewPortPSI(CH6VPOSD.LayerViewPortHandle[Loop_i],&stReadVPPSI_OSD[Loop_i]);
			}
		
	if (eColorProcess==BRIGHTNESSGAIN)/* Keep Contrast value */
		{
			for (Loop_i=0;Loop_i<2;Loop_i++)
				{
					stVPPSI[Loop_i].VideoFilteringParameters.BCParameters.ContrastGainControl=stReadVPPSI[Loop_i].VideoFilteringParameters.BCParameters.ContrastGainControl;

					stVPPSI_OSD[Loop_i].VideoFilteringParameters.BCParameters.ContrastGainControl=stReadVPPSI_OSD[Loop_i].VideoFilteringParameters.BCParameters.ContrastGainControl;
				}
		}
	else if (eColorProcess==CONTRASTGAIN)/* Keep Br value */
		{
			for (Loop_i=0;Loop_i<2;Loop_i++)
				{
					stVPPSI[Loop_i].VideoFilteringParameters.BCParameters.BrightnessGainControl=stReadVPPSI[Loop_i].VideoFilteringParameters.BCParameters.BrightnessGainControl;

					stVPPSI_OSD[Loop_i].VideoFilteringParameters.BCParameters.BrightnessGainControl=stReadVPPSI_OSD[Loop_i].VideoFilteringParameters.BCParameters.BrightnessGainControl;
				}
		}

	
	for (Loop_i=0;Loop_i<MaxVP_Hdl;Loop_i++)
		{
			if (eColorProcess==BRIGHTNESSGAIN)
				{
					stVPPSI[Loop_i].VideoFilteringParameters.BCParameters.BrightnessGainControl=ActualValue;

					stVPPSI_OSD[Loop_i].VideoFilteringParameters.BCParameters.BrightnessGainControl=ActualValue;
				}
			else if (eColorProcess==CONTRASTGAIN)
				{
					stVPPSI[Loop_i].VideoFilteringParameters.BCParameters.ContrastGainControl=ActualValue;

					stVPPSI_OSD[Loop_i].VideoFilteringParameters.BCParameters.ContrastGainControl=ActualValue;
				}
			else if (eColorProcess==TINTANGLE)
				{
					stVPPSI[Loop_i].VideoFilteringParameters.TintParameters.TintRotationControl=ActualValue;

					stVPPSI_OSD[Loop_i].VideoFilteringParameters.TintParameters.TintRotationControl=ActualValue;
				}
			else if (eColorProcess==SATURATIONGAIN)
				{
					stVPPSI[Loop_i].VideoFilteringParameters.SatParameters.SaturationGainControl=ActualValue;

					stVPPSI_OSD[Loop_i].VideoFilteringParameters.SatParameters.SaturationGainControl=ActualValue;
				}
		}
	
	/* Set new */
	for(Loop_i=0;Loop_i<MaxVP_Hdl;Loop_i++)
	{
		ErrCode=STVID_SetViewPortPSI(*VIEWPORT_Handler[Loop_i],&stVPPSI[Loop_i]);
	ErrCode=STLAYER_SetViewPortPSI(CH6VPOSD.LayerViewPortHandle[Loop_i],&stVPPSI_OSD[Loop_i]);
	}
		
	STTBX_Print(( "\n Write Value :%d",ActualValue));		
	free(stVPPSI);
	free(stReadVPPSI);
	free(stVPPSI_OSD);
	free(stReadVPPSI_OSD);
	stVPPSI=NULL;
	stReadVPPSI=NULL;
	stVPPSI_OSD=NULL;
	stReadVPPSI_OSD=NULL;
	return ErrCode;

}

#if 0
/*函数描述:图像亮度、对比度、色度调整
 *参数:无
 *返回值:无
*/
void CH6_SetVideo(void)
{
	int luminance,chroma,contrast;				
	int iKeyScanCode;
	ST_ErrorCode_t Err;
	signed char CurSelect=0;
	int PreSelect;
	char video_Str[][LANGUAGES][20]={"亮  度","Brightness","对比度","Contrast","色  度","Color"};
	int pCurrent[3];
	int PreValue[3];
	char Str[5]; 
	int i,j;
	int  tempLuminance,tempContrast,tempChroma;
	STVOUT_OutputParams_t tempOutputParams;
	int xNum=VIDEO_TXSTART_X+VIDEO_BAR_WIDTH-36;
	int yNum=VIDEO_TXSTART_Y;
	
	/*定义六个三角形顶点坐标 */
	STGFX_Point_t Points[18];
	for(i=0;i<18;i+=6)
	{
		Points[i].X=265;
		Points[i].Y=209+(i/6)*VIDEO_GAP;		
	}
	for(i=1;i<18;i+=6)
	{
		Points[i].X=277;
		Points[i].Y=202+(i/6)*VIDEO_GAP;		
	}
	for(i=2;i<18;i+=6)
	{
		Points[i].X=277;
		Points[i].Y=216+(i/6)*VIDEO_GAP;		
	}
	#if 0/* cqj 20070725 modify*/
	for(i=3;i<18;i+=6)
	{
		Points[i].X=605;
		Points[i].Y=202+(i/6)*VIDEO_GAP;		
	}
	for(i=4;i<18;i+=6)
	{
		Points[i].X=617;
		Points[i].Y=209+(i/6)*VIDEO_GAP;		
	}
	for(i=5;i<18;i+=6)
	{
		Points[i].X=605;
		Points[i].Y=216+(i/6)*VIDEO_GAP;		
	}	
	#else
	for(i=3;i<18;i+=6)
	{
		Points[i].X=617;
		Points[i].Y=209+(i/6)*VIDEO_GAP;		
	}
	for(i=4;i<18;i+=6)
	{
		Points[i].X=605;
		Points[i].Y=202+(i/6)*VIDEO_GAP;		
	}
	for(i=5;i<18;i+=6)
	{
		Points[i].X=605;
		Points[i].Y=216+(i/6)*VIDEO_GAP;		
	}
	
	#endif
	/*定义六个三角形顶点坐标 */
	
	luminance = pstBoxInfo->abiTVBright;
	chroma = pstBoxInfo->abiTVSaturation;
	contrast	= pstBoxInfo->abiTVContrast;	
	
	#if 0
	STVOUT_GetOutputParams(VOUTHandle[VOUT_MAIN], &tempOutputParams);	
	#endif
	STVOUT_GetOutputParams(VOUTHandle[VOUT_AUX], &tempOutputParams);
	
	
	tempOutputParams.Analog.StateBCSLevel=STVOUT_PARAMS_AFFECTED;
	tempOutputParams.Analog.StateBCSLevel=STVOUT_PARAMS_AFFECTED;
	
	tempLuminance=tempOutputParams.Analog.BCSLevel.Brightness;
	tempContrast=tempOutputParams.Analog.BCSLevel.Contrast;
	tempChroma=tempOutputParams.Analog.BCSLevel.Saturation;	
	
	pCurrent[0]=luminance;
	pCurrent[1]=contrast;
	pCurrent[2]=chroma;
	
	PreValue[0]=pCurrent[0];
	PreValue[1]=pCurrent[1];
	PreValue[2]=pCurrent[2];
	
	PreSelect=CurSelect;
	
	{
		int x11=368;
		int y11=394;
		
		CH6_DisplayButton(
			x11, 
			y11,
			138,
			32,
			GLOBAL_THINBORDER_WIDTH,
			5,
			SYSTEM_BUTTON_COLOR,
			SYSTEM_BUTTONBORDER_COLOR,
			SYSTEM_FONT_NORMA_COLORL, 
			(pstBoxInfo->abiLanguageInUse==0)?"缺省设置":"Default", 
			RED_BUTTON);		
	}
	
	for(i=0;i<3;i++)
	{  
		sprintf(Str,"%d",pCurrent[i]);
		j=i*6;
		if(i==CurSelect)
		{
			DrawTriangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Points[j],Points[j+1],Points[j+2],VIDEO_TEXT_COLOR);
			DrawTriangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Points[j+3],Points[j+4],Points[j+5],VIDEO_TEXT_COLOR);
			CH6_DrawAdjustBar(
				GFXHandle,
				CH6VPOSD.ViewPortHandle,
				VIDEO_BARSTART_X,
				VIDEO_BARSTART_Y+CurSelect*VIDEO_GAP,
				0,
				pCurrent[CurSelect],
				TRUE,
				0);
			CH6_DrawVideoText(
				CurSelect,
				whitecolor,
				VIDEO_TEXTBK_COLOR,
				video_Str[CurSelect][pstBoxInfo->abiLanguageInUse],
				Str);
		}
		else
		{
			CH6_DrawAdjustBar(
				GFXHandle,
				CH6VPOSD.ViewPortHandle,
				VIDEO_BARSTART_X,
				VIDEO_BARSTART_Y+i*VIDEO_GAP,
				0,
				pCurrent[i],
				TRUE,
				1);
			
			CH6_DrawVideoText(
				i,
				VIDEO_TEXT_COLOR,
				SYSTEM_INNER_FRAM_COLOR,
				video_Str[i][pstBoxInfo->abiLanguageInUse],
				Str);
		}
	}	
	CH_UpdateAllOSDView();/*20051215 add*/
	while(1)
	{
		iKeyScanCode=GetKeyFromKeyQueue(5);
		switch(iKeyScanCode)
		{
		case UP_ARROW_KEY:
		case DOWN_ARROW_KEY:
			PreSelect=CurSelect;
			(iKeyScanCode==UP_ARROW_KEY) ? (CurSelect--):(CurSelect++) ; 
			M_CHECK(CurSelect,0,2);
			
			j=CurSelect*6;
			{
				sprintf(Str,"%d",pCurrent[CurSelect]);
				DrawTriangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Points[j],Points[j+1],Points[j+2],VIDEO_TEXT_COLOR);
				DrawTriangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Points[j+3],Points[j+4],Points[j+5],VIDEO_TEXT_COLOR);
				CH6_DrawAdjustBar(
					GFXHandle,
					CH6VPOSD.ViewPortHandle,
					VIDEO_BARSTART_X,
					VIDEO_BARSTART_Y+CurSelect*VIDEO_GAP,
					0,
					pCurrent[CurSelect],
					TRUE,
					0);
				CH6_DrawVideoText(
					CurSelect,
					whitecolor,
					VIDEO_TEXTBK_COLOR,
					video_Str[CurSelect][pstBoxInfo->abiLanguageInUse],
					Str);
			}
			
			{
				sprintf(Str,"%d",pCurrent[PreSelect]);
				CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,VIDEO_BARSTART_X-20,VIDEO_BARSTART_Y+PreSelect*VIDEO_GAP,18,18,SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);
				CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,VIDEO_BARSTART_X+VIDEO_BAR_WIDTH,VIDEO_BARSTART_Y+PreSelect*VIDEO_GAP,20,18,SYSTEM_INNER_FRAM_COLOR,SYSTEM_INNER_FRAM_COLOR, 1);
				CH6_DrawAdjustBar(
					GFXHandle,
					CH6VPOSD.ViewPortHandle,
					VIDEO_BARSTART_X,
					VIDEO_BARSTART_Y+PreSelect*VIDEO_GAP,
					0,
					pCurrent[PreSelect],
					TRUE,
					1);
				CH6_DrawVideoText(
					PreSelect,
					VIDEO_TEXT_COLOR,
					SYSTEM_INNER_FRAM_COLOR,
					video_Str[PreSelect][pstBoxInfo->abiLanguageInUse],
					Str);
			}
			CH_UpdateAllOSDView();/*20051215 add*/
			break;
			
		case LEFT_ARROW_KEY:
		case RIGHT_ARROW_KEY:
			(iKeyScanCode==LEFT_ARROW_KEY) ? (pCurrent[CurSelect]-=1):(pCurrent[CurSelect]+=1);
					if( (pCurrent[CurSelect])<VIDEO_MIN)	(pCurrent[CurSelect])=VIDEO_MIN;/*charle 这些值最后来修改*/
			if( (pCurrent[CurSelect])>VIDEO_MAX )	(pCurrent[CurSelect])=VIDEO_MAX;
			if((pCurrent[CurSelect] >= VIDEO_MIN) && (pCurrent[CurSelect] <=VIDEO_MAX))
			{

						CH6_DrawAdjustBar(GFXHandle,	CH6VPOSD.ViewPortHandle,
											VIDEO_BARSTART_X,VIDEO_BARSTART_Y+CurSelect*VIDEO_GAP,
											PreValue[CurSelect],pCurrent[CurSelect],
											FALSE,0);		
			}	
			
			PreValue[CurSelect]=pCurrent[CurSelect];			
			
			sprintf(Str,"%d",(pCurrent[CurSelect]));	
			CH6_DisRectTxtInfo(
				GFXHandle,
				CH6VPOSD.ViewPortHandle,
				xNum,
				yNum+CurSelect*VIDEO_GAP,
				CHARACTER_WIDTH*3,
				VIDEO_TXBCK_HEIGHT,
				Str,
				VIDEO_TEXTBK_COLOR, 
				whitecolor,
				1,
				0);			
			CH_UpdateAllOSDView();/*20051215 add*/
			switch(CurSelect)
			{
			case 0:				
				tempLuminance=Lum_Convert(pCurrent[CurSelect]);
				luminance=pCurrent[CurSelect];
				ColorProcessingTest(BRIGHTNESSGAIN,tempLuminance);
				break;
			case 1:
				tempContrast=Con_Convert(pCurrent[CurSelect]);
				contrast=pCurrent[CurSelect];
				ColorProcessingTest(CONTRASTGAIN,tempContrast);

				break;
			case 2:
				tempChroma=Chr_Convert(pCurrent[CurSelect]);
				chroma=pCurrent[CurSelect];
				ColorProcessingTest(SATURATIONGAIN,tempChroma);
				break;
			}
#if 0			
			tempOutputParams.Analog.BCSLevel.Brightness= tempLuminance;
			tempOutputParams.Analog.BCSLevel.Contrast= tempContrast;
			tempOutputParams.Analog.BCSLevel.Saturation= tempChroma;
#if 0
			STVOUT_SetOutputParams(VOUTHandle[VOUT_MAIN], &tempOutputParams);	
#endif			
			STVOUT_SetOutputParams(VOUTHandle[VOUT_AUX], &tempOutputParams);
			
			
			/*2006028 add*/
			/*       STVOUT_SetOutputParams(VOUT_Handle[1],&tempOutputParams);*/
#endif			
			break;
			
			case RED_KEY:
				luminance=pCurrent[0]=50;
				contrast=pCurrent[1]=50;
				chroma=pCurrent[2]=50;
				
				for(i=0;i<3;i++)
				{  
					sprintf(Str,"%d",pCurrent[i]);
					j=i*6;
					if(i==CurSelect)
					{
						DrawTriangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Points[j],Points[j+1],Points[j+2],VIDEO_TEXT_COLOR);
						DrawTriangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,Points[j+3],Points[j+4],Points[j+5],VIDEO_TEXT_COLOR);
						CH6_DrawAdjustBar(
							GFXHandle,
							CH6VPOSD.ViewPortHandle,
							VIDEO_BARSTART_X,
							VIDEO_BARSTART_Y+CurSelect*VIDEO_GAP,
							PreValue[CurSelect],
							pCurrent[CurSelect],
							FALSE,
							0);
						CH6_DrawVideoText(
							CurSelect,
							whitecolor,
							VIDEO_TEXTBK_COLOR,
							video_Str[CurSelect][pstBoxInfo->abiLanguageInUse],
							Str);
					}
					else
					{
						CH6_DrawAdjustBar(
							GFXHandle,
							CH6VPOSD.ViewPortHandle,
							VIDEO_BARSTART_X,
							VIDEO_BARSTART_Y+i*VIDEO_GAP,
							PreValue[i],
							pCurrent[i],
							FALSE,
							1);			
						CH6_DrawVideoText(
							i,
							VIDEO_TEXT_COLOR,
							SYSTEM_INNER_FRAM_COLOR,
							video_Str[i][pstBoxInfo->abiLanguageInUse],
							Str);
						
					}
					CH_UpdateAllOSDView();/*20051215 add*/
					PreValue[i]=pCurrent[i];
				}
			
			ColorProcessingTest(BRIGHTNESSGAIN,0);
				ColorProcessingTest(CONTRASTGAIN,0);
				ColorProcessingTest(TINTANGLE,0);
				ColorProcessingTest(SATURATIONGAIN,0);
		
#if 0				
				tempLuminance=Lum_Convert(50);
				tempContrast=Con_Convert(50);
				tempChroma=Chr_Convert(50);
				
				tempOutputParams.Analog.BCSLevel.Brightness=tempLuminance;
				tempOutputParams.Analog.BCSLevel.Contrast= tempContrast;
				tempOutputParams.Analog.BCSLevel.Saturation= tempChroma;
#if 0
				STVOUT_SetOutputParams(VOUTHandle[VOUT_MAIN], &tempOutputParams);	
#endif				
				STVOUT_SetOutputParams(VOUTHandle[VOUT_AUX], &tempOutputParams);
#endif				
				break;
			case EXIT_KEY:		
				/*	case MENU_KEY:*/
				{
					CH6_ClearFullnnerFram();
					CH_UpdateAllOSDView();/*20051215 add*/
					pstBoxInfo->abiTVBright=luminance;
					pstBoxInfo->abiTVSaturation=chroma;
					pstBoxInfo->abiTVContrast=contrast;
					CH_NVMUpdate( idNvmBoxInfoId);
					return;
				}
			default:
				CH6_DisplayMenuTime();
				CH_UpdateAllOSDView();/*20051215 add*/
				break;
		}			
	}	
}
#endif

