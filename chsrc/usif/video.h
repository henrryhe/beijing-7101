
#ifndef VEDIO_USIF

#define VEDIO_USIF

#define LANGUAGES 	2
#define VIDEO_STEP 1
#define VIDEO_MAX 100
#define VIDEO_MIN 1
#define R_MAX  	254

#ifndef OSD_COLOR_TYPE_RGB16  

#define VIDEO_TEXTBK_COLOR  	0xff506090
#define VIDEO_TEXT_COLOR  		0xff205070
#else/*ARGB1555*/
#define VIDEO_TEXTBK_COLOR  	0xA992
#define VIDEO_TEXT_COLOR  		0x914E
#endif

/*ljg 040827 add*/
#define VIDEO_GAP 71
#define TRIANGLE_GAP 75
#define VIDEO_TXBCK_START_X 277
#define VIDEO_TXSBCK_TART_Y 163
#define VIDEO_BARSTART_X 283
#define VIDEO_BARSTART_Y 200
#define VIDEO_TXSTART_X VIDEO_BARSTART_X
#define VIDEO_TXSTART_Y VIDEO_TXSBCK_TART_Y
#define VIDEO_TXBCK_HEIGHT 30
#define VIDEO_BAR_WIDTH 316
/*end ljg 040827 add*/

extern U8 Lum_Convert(U8 luminance);
extern U8 Chr_Convert(U8 Chroma);
extern U8 Con_Convert(U8 contrast);
extern boolean CH6_DrawRoundRecText(STGFX_Handle_t Ghandle,STLAYER_ViewPortHandle_t VPHandle,U32 StartX ,U32 StartY , U32 Width , U32 Height ,int  BackColor,int TextColor, int Align,char* pItemMessage);
extern int CH6_GetTextWidth(char* text);
extern int CH6_DrawHorizontalSlider(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,U32 StartX,U32 StartY,U32 Height,U32 sWidth, U32 sHeight,U32 sLineWidth,U32 lHeight,U32 ItemMax,U32 Step,U32 Index,int Color,int sColor);
extern void CH6_VideoChange(U8* luminance,U8* contrast,U8* chroma);
extern void CH6_SetVideo(void);
extern void BootSETHDVideo(void);/* cqj 20071111 add waitting for check " "*/


#endif

