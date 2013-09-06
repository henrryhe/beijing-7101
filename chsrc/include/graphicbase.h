/*
  Name:graphicbase.h
  Description:header file of bottom function about display using STGFX in QAM5516 platform
  Authors:yxl
  Date          Remark
  2004-05-17    Created
  2004-08-13     modify   version:1.1

  Modify:
  Date:Yixiaoli 2007-08-14，2007-09-03 by yixiaoli  
1.Add ARGB8888 格式支持，由MACRO YXL_ARGB8888控制，并修改如下函数原型支持ARGB8888
（主要是将与颜色相关的参数由int 等改为unsigned int)
(1)CH6_TextOut(...);
(2)CH6_SetColorValue(...);
(3)CH6_GetPixelColor(...);
(4)BOOL CH6_SetPixelColor(...);

2.Add new function char* CHDRV_OSDB_GetVision(void),用于查询图形底层的版本，
 目前版本为：CHDRV_OSDB_R3.1.1.

*/

#include "stgfx.h" 
#include "stlayer.h"  
#include "stavmem.h"  
#include "stgxobj.h"


#ifndef GRAPH_BASE
#define GRAPH_BASE 

#define FONT_SIZE 24
#define  LEFT_ARROW  0x10
#define  RIGHT_ARROW  0x11
#define  UP_ARROW     0x1e
#define  DOWN_ARROW   0x1f


#define USED_PIC_FILE_NUM  73/*33*/

#if 0 /*yxl 2004-11-10 modify this section*/

#define FLASH_FONT_ADDR /*0x7fe30000*//*0x7fa00000*/0x7ff20000
#else

#if 0/*yxl 2004-11-18 modify this section*/
#define FLASH_FONT_ADDR FLASH_PIC_ADDRESS+468610
#else
#define FLASH_FONT_ADDR 0xB0000000/*0xA0580000 20070709 change to 0xB0000000*/ /*0xa0400000,yxl 2007-03-05 modify */
#endif/*end yxl 2004-11-18 modify this section*/

#endif/*end yxl 2004-11-10 modify this section*/

#ifndef OSD_COLOR_TYPE_RGB16 
#define FLASH_PIC_ADDRESS   FLASH_FONT_ADDR+548208/*0x7fc00000*/ /*this is for new interface still layer test*/
#else
/*#define FLASH_PIC_ADDRESS  0x7fd40000*/
#define FLASH_PIC_ADDRESS   FLASH_FONT_ADDR+548208/*0x7fc00000*/ /*this is for new interface still layer test*/

#endif
/*20070709 add*/
#define FONTPIC_FLASHZIP_ADDR        0xA19A0000
#define COMPANISON_FLASHZIP_ADDR 0xA1000000 

#define MAX_FLASHFILE_NAME   12

#if 0 /*yxl 2004-08-12 modify this section for new pic structure*/
typedef struct/*this is from zxg*/
{
  unsigned int  FileNum;       /**flash中文件个数*/
  unsigned int  FileHeadLen;   /*FLASH中文件头信息长度*/
  unsigned char GlRGB[256][3]; /*全局调色板*/
}FlashFileHead;
#else
typedef struct/*this is from jqz*/
{
  unsigned char Magic[4]; /*     yxl 2004-11-10 add this variable,CHPC*/
	unsigned short  FileNum;       /*flash中pic文件个数*/
  unsigned short  IsHavePallette;   /*0 stand for no pallette info in FLASH,*/
/*  unsigned char GlRGB[256][3]; 全局调色板*/
}FlashFileHead;
#endif


typedef enum
{
  BMPM,
  BMPS,
  GIF,
  MPEGI,
  RGB565 ,
  ARGB1555,
  ARGB4444,
	YCrCb422,
	ARGB8888  /*yxl 2007-09-03 add this line*/
}FlashFileType;

typedef struct 
{
  FlashFileType FileType;
  char FlashFIleName[MAX_FLASHFILE_NAME];
  U32  FlasdAddr;  
  U32 iWidth;
  U32 iHeight;
  U32 FileLen;
  U32 ZipSign;/*1->stand for zip,other value,not zip,yxl 2004-08-24 modify this line,the old is U32 CRC32;*/
}FlashFileStr;


/*yxl 2007-02-09 add below struc*/
typedef struct 
{
	unsigned int PicStartAdr;
/*	int PicNumber;*/
	int PicMaxNameLen;
	unsigned int FontStartAdr;
	ST_Partition_t* pPartition;  

}CH_GraphicInitParams;
/*end yxl 2007-02-09 add below struc*/


#if 0 /*yxl 2007-02-05 modify below section*/

/*define paremeters of a CH viewport,2004-09-21 ADD*/
typedef struct 
{
	int PosX;     /*Input parameter,start X position of the viewport*/
	int PosY;	  /*Input parameter,start Y position of the viewport*/
	unsigned int Width; /* Input parameter,Width of the viewport*/
	unsigned int Height;/* Input parameter,Height of the viewport*/
	STGXOBJ_ColorType_t  ColorType;/*Input parameter, Color type of the viewport*/
	STGXOBJ_BitmapType_t  BitmapType;/*Input parameter, bitmap type of the viewport*/
	STGXOBJ_AspectRatio_t AspectRatio; /*yxl 2005-09-07 add this member*/
	STLAYER_Handle_t LayerHandle;/*Input parameter,Handle of the layer which the viewport belongs to*/
	STAVMEM_PartitionHandle_t ViewportPartitionHandle;/*Input paramter,Partition handle of the memory which the viewport will uses,yxl 2006-11-13 add this member */
}CH6_ViewPortParams_t;


#else

#define MAX_LAYERVIEWPORT_NUMBER 2

typedef struct 
{
	int PosX;     /*Input parameter,start X position of the viewport*/
	int PosY;	  /*Input parameter,start Y position of the viewport*/
	unsigned int Width; /* Input parameter,Width of the viewport*/
	unsigned int Height;/* Input parameter,Height of the viewport*/
	STGXOBJ_ColorType_t  ColorType;/*Input parameter, Color type of the viewport*/
	STGXOBJ_BitmapType_t  BitmapType;/*Input parameter, bitmap type of the viewport*/
	STGXOBJ_AspectRatio_t AspectRatio; /*yxl 2005-09-07 add this member*/
	STLAYER_Handle_t LayerHandle;/*Input parameter,Handle of the layer which the viewport belongs to*/
	STAVMEM_PartitionHandle_t ViewportPartitionHandle;/*Input paramter,Partition handle of the memory which the viewport will uses,yxl 2006-11-13 add this member */
}CH6_SingleViewPortParams_t;


typedef struct 
{
	unsigned short int ViewPortCount;
	CH6_SingleViewPortParams_t ViewPortParams[MAX_LAYERVIEWPORT_NUMBER];

}CH6_ViewPortParams_t;

#endif /*end yxl 2007-02-05 modify below section*/




#if 0  /*yxl 2005-10-29 modify below struct,
and rename struct CH_ViewPort_t to CH_ViewPort_t,and define CH_ViewPortHandle_t*/

typedef struct /*Handle of a CH viewport*/
{
	STLAYER_ViewPortHandle_t ViewPortHandle;

}CH_ViewPort_t;

#else

typedef U32 CH_ViewPortHandle_t;

typedef struct
{
	CH_ViewPortHandle_t ViewPortHandle;
	CH_ViewPortHandle_t ViewPortHandleAux;
	unsigned int OSDLayerCount;
	STLAYER_ViewPortHandle_t LayerViewPortHandle[MAX_LAYERVIEWPORT_NUMBER]; /*yxl 2007-02-02 temp add [2] for test*/
	
}CH_ViewPort_t;
#endif /*end yxl 2005-10-29 modify below struct,and rename struct CH_ViewPort_t to CH_ViewPort_t*/




extern FlashFileStr  *GlobalFlashFileInfo;
extern FlashFileHead *GlobalFlashFileHead;


/*Function description:
	display a bitmap in OSD Layer 
Parameters description: 
	STGFX_Handle_t Handle:the handle of the opened STGFX
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
	int PosX:the start X position of the displayed bitmap
	int PosY:the start y position of the displayed bitmap
	char *PICFileName:the string pointer pointed to the displayed bitmap name
	BOOL IsDisplayBackground:the sign standing for whether mixing with background when displaying picture on OSD layer
						TRUE:stand for mixing with background	
						FALSE:stand for not mixing with background						

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_DisplayPIC(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,
						   int PosX,int PosY,char *PICFileName,BOOL IsDisplayBackground);


/*yxl 2004-10-18 add this function*/

/*Function description:
	display a bitmap in OSD Layer 
Parameters description: 
	STGFX_Handle_t Handle:the handle of the opened STGFX
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
	int PosX:the start X position of the displayed bitmap
	int PosY:the start y position of the displayed bitmap
	int Width:the width of the displayed bitmap
	int Height:the Height of the displayed bitmap
	U8* pdata:Pointer to the bitmap data to be displayed
	BOOL IsDisplayBackground:the sign standing for whether mixing with background when displaying picture on OSD layer
						TRUE:stand for mixing with background	
						FALSE:stand for not mixing with background						

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_DisplayPICByData(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,
					int Width,int Height,U8* pData,BOOL IsDisplayBackground);



#if 0 /*yxl 2007-02-09 cancel this function*/
 
#if 1  /*yxl 2007-02-09 modify this section*/
extern BOOL InitFlashFileInfo(unsigned int StartAdr,/*int PicNumber,*/int MaxPicNameLen);/**/
#else
extern BOOL InitFlashFileInfo(unsigned int StartAdr,int PicNumber,int MaxPicNameLen);
#endif   /*end yxl 2007-02-09 modify this section*/

#endif 

/*extern void InitFlashFileInfo();*/

/*Function description:
	display a text string in OSD Layer 
Parameters description: 
	STGFX_Handle_t Handle:the handle of the opened STGFX
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
	int PosX:the start X position of the displayed text string
	int PosY:the start y position of the displayed text string
	unsigned int TextCorlorIndex:the color index of the displayed text 
	char *PICFileName:the string pointer pointed to the displayed text

return value:NO
*/
extern BOOL CH6_TextOut(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,
						unsigned int TextCorlorIndex,unsigned char* pTextString);/*yxl 2007-03-05 add unsigned */


/*Function description:
	Extracts bitmap data from a specified rectangle area of the OSD layer 
Parameters description: 
	STGFX_Handle_t Handle:the handle of the STGFX
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
	int PosX:the start X position of the specified rectangle
	int PosY:the start Y position of the specified rectangle
	int Width:the width of the specified rectangle
	int Height:the height of the specified rectangle

return value:
	U8* :pointed to the allocated storage area for the extracted bitmap data
*/
/*extern BOOL CH6_GetRectangle(STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,int PosX, int PosY, int Width, int Height, 
				 U8* pBMPData);
yxl 2004-08-16 modify this section*/
extern U8* CH6_GetRectangle(STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC,int PosX, int PosY, int Width, int Height);


/*Function description:

Parameters description: 

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_PutRectangle(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, int PosX, int PosY,int Width,int Height,U8* pBMPData);

#if 0 /* yxl 2004-10-07 modify this section*/

/*Function description:
	Enable the display of the OSD Layer 
Parameters description: 
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_EnableOSD(STLAYER_ViewPortHandle_t VPHandle);

/*Function description:
	Disable the display of the OSD Layer 
Parameters description: 
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_DisableOSD(STLAYER_ViewPortHandle_t VPHandle);

#else

/*Function description:
	Show the relative viewport
Parameters description: 
	CH_ViewPort_t* CH6VPHandle:pointer to a CH viewport handle which will be show
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_ShowViewPort(CH_ViewPort_t* pCH6VPHandle);

/*Function description:
	Hide the relative viewport
Parameters description: 
	STLAYER_ViewPortHandle_t* VPHandle:pointer to a CH viewport handle which will be hide
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_HideViewPort(CH_ViewPort_t* pCH6VPHandle);


/*Function description:
	Get the display status of the relative viewport
Parameters description: 
	CH_ViewPort_t* CH6VPHandle:the handle pointer of the viewport which will be operated
return value:
			TRUE:stand for the viewport is current in show status 		
			FALSE:stand for the viewport is current in hide status
*/
#if 0 /* yxl 2004-12-04 modify this function*/
extern BOOL CH6_GetViewPortDisplayStatus(CH_ViewPort_t* pCH6VPHandle,BOOL* pDisplayStatus);
#else
extern BOOL CH6_GetViewPortDisplayStatus(CH_ViewPort_t* pCH6VPHandle,BOOL* pDisplayStatus);
#endif/*end yxl 2004-12-04 modify this function*/

#endif /*end yxl 2004-10-07 modify this section*/


/*Function description:
	Clear the the display content of the appointed rectangle area from the OSD Layer 
Parameters description: 
	STGFX_Handle_t GHandle:the handle of the opened STGFX
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
	int PosX:the start X position of the cleared area
	int PosY:the start Y position of the cleared area
	int Width:the width of the cleared area
	int Height:the height of the cleared area

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_ClearArea(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,int Width,int Height);


/*Function description:
	Clear the all display content of the OSD Layer 
Parameters description: 
	STGFX_Handle_t GHandle:the handle of the opened STGFX
	STLAYER_ViewPortHandle_t VPHandle:the handle of the viewport which will be operated
return value:NO
*/
extern BOOL CH6_ClearFullArea(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle);

/*Function description:
	Set color value according to color type 
Parameters description: 
	STGXOBJ_Color_t*:the handle of the opened STGFX

	unsigned int Value:

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_SetColorValue(STGXOBJ_Color_t* pColor,unsigned int Value);




/*yxl 2004-08-21 add this section for Still display support*/

/*Function description:
	Show still layer 
Parameters description: no
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_EnableStillLayer(void);

/*Function description:
	Hide still layer 
Parameters description: no
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_DisableStillLayer(void);

/*end yxl 2004-08-21 add this section for Still display support*/


/*yxl 2004-08-21 add this section for Still display support*/
/*Function description:
	According to input paramters,mix layer
Parameters description: 
	BOOL ShowStillLaye:true stand for mixing still layer
	                   false stand for not mixing still layer
	BOOL ShowVodeoLayer:true stand for mixing video layer
	                   false stand for not mixing video layer
	BOOL ShowOSDLayer:true stand for mixing OSD layer
	                   false stand for not mixing OSD layer
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
#if 0 /*yxl 2005-06-08 modify this section for 优化该函数的显示*/
extern BOOL CH6_MixLayer(BOOL ShowStillLayer,BOOL ShowVodeoLayer,BOOL ShowOSDLayer);
#else
extern BOOL CH6_MixLayer(BOOL ShowStillLayer,char* StillLayerName, BOOL ShowVodeoLayer,char* VodeoLayerName,
				  BOOL ShowOSDLayer,char* OSDLayerName);
#endif/*end yxl 2005-06-08 modify this section for 优化该函数的显示*/

/*extern BOOL SetPaletteColor(STGXOBJ_Palette_t* pPalette,U8* pRGBValue,unsigned short IndexCount);*/

/*yxl 2004-09-21 add below function*/

/*Function description:
	Create a viewport in the OSD Layer or Still layer according to appointed parameters which is defined in CH6_ViewPortParams_t structure
Parameters description: 
	CH6_ViewPortParams_t*:pointer to an allocated CH6_ViewPortParams_t structure containing the Ch viewport parameters which will be created
	                  
	CH_ViewPort_t*:
				IN:Pointer to a allocated CH viewport handle
				OUT:Returned CH viewport handle
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_CreateViewPort(CH6_ViewPortParams_t* Params,CH_ViewPort_t* pCH6VPHandle,unsigned short int VPNumber);


/*Function description:
	Delete a created CH viewport 
Parameters description: 
	CH6_ViewPortParams_t*:pointer to an allocated CH6_ViewPortParams_t structure containing the Ch viewport parameters which will be deleted
          
return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
#if 0 /*yxl 2004-10-30 modify this section*/
extern BOOL CH6_DeleteViewPort(CH6_ViewPortParams_t* Params);
#else

#if 0 /*yxl 2004-12-04 modify this function name*/
extern BOOL CH6_DeleteViewPort(CH_ViewPort_t VPHandle,CH6_ViewPortParams_t* Params);
#else
extern BOOL CH6_DeleteViewPort(CH_ViewPort_t VPHandle);
#endif/*end yxl 2004-12-04 modify this function name*/
#endif/*end yxl 2004-10-30 modify this section*/

/*Function description:
	Set pallette color for a viewport
Parameters description: 
	STLAYER_ViewPortHandle_t VPHandle:Handle of the CH viewport which will be set a pallette
	U8* pRGBValue:pointer to the color value which will be set to a pallette
	IndexCount:the color count of the pallette
         
return value:
			TRUE:stand for the function is unsuccessful operated 		
			FALSE:stand for the function is successful operated 	
*/
extern BOOL CH6_SetViewPortPaletteColor(STLAYER_ViewPortHandle_t VPHandle,U8* pRGBValue,unsigned short IndexCount);
/*end yxl 2004-09-21 add below function*/


/* yxl 2004-10-24 add below function*/

/*Function description:
	Set the color value of a pixel at a given position in a viewport
Parameters description: 
	STGFX_Handle_t GFXHandle:Handle to the STGFX instance

	STLAYER_ViewPortHandle_t CHVPHandle:Handle of the CH viewport which will be set 
	STGFX_GC_t* pGC:pointer to a graphics context
	int PosX:the x position of the pixel
	int PosY:the y position of the pixel
	unsigned int Color: the color value to be set to a pixel,
		the meaning of this variable is related to the member varaible "ColorType" of the structure "CH6_ViewPortParams_t" 
		,which is used to create a  CH viewport.
		If the ColorType is STGXOBJ_COLOR_TYPE_CLUT8,the unsigned short variable stand for color index of a palette with 8 precision,
		if the ColorType is STGXOBJ_COLOR_TYPE_ARGB4444,the meaning of this unsigned short variable is as follows:
		the most four bits of the most byte stand for alpha component of the color,
		the low four bits of the most byte stand for R componentof the color,
		the most four bits of the low byte stand for G componentof the color,
		the low four bits of the low byte stand for B componentof the color,
         
return value:
			TRUE:stand for the function is unsuccessful operated 		
			FALSE:stand for the function is successful operated 	
*/
extern BOOL CH6_SetPixelColor(STGFX_Handle_t GFXHandle,/*CH_ViewPort_t*/
					   CH_ViewPortHandle_t CHVPHandle,STGFX_GC_t* pGC,
				  int PosX,int PosY,unsigned int Color);

/*Function description:
	Get the color value of a pixel at a given position in a viewport
Parameters description: 
	STGFX_Handle_t GFXHandle:Handle to the STGFX instance

	STLAYER_ViewPortHandle_t CHVPHandle:Handle of the CH viewport which will be set 

	int PosX:the x position of the pixel
	int PosY:the y position of the pixel
	unsigned int* pColorValue: pointer to a unsigned short through which to return the color value of the pixel 
   		the meaning of this variable is related to the member varaible "ColorType" of the structure "CH6_ViewPortParams_t" 
		,which is used to create a  CH viewport.
		If the ColorType is STGXOBJ_COLOR_TYPE_CLUT8,the unsigned short variable stand for color index of a palette with 8 precision,
		if the ColorType is STGXOBJ_COLOR_TYPE_ARGB4444,the meaning of this unsigned short variable is as follows:
		the most four bits of the most byte stand for alpha component of the color,
		the low four bits of the most byte stand for R componentof the color,
		the most four bits of the low byte stand for G componentof the color,
		the low four bits of the low byte stand for B componentof the color,
return value:
			TRUE:stand for the function is unsuccessful operated 		
			FALSE:stand for the function is successful operated 	
*/
extern BOOL CH6_GetPixelColor(STGFX_Handle_t GFXHandle, /*CH_ViewPort_t*/
					   CH_ViewPortHandle_t  CHVPHandle,
				  int PosX,int PosY,unsigned int* pColorValue);

/*end yxl 2004-10-24 add below function*/

#if 0 /*yxl 2007-02-09 cancel below function*/
/*yxl 2004-11-02 add this function*/
extern void CH6_InitFont(unsigned int FontBaseAdr);
/*end yxl 2004-11-02 add this function*/
#endif /*end yxl 2007-02-09 cancel below function*/


extern BOOL CH6_GetBitmapInfo(STLAYER_ViewPortHandle_t VPHandle,STGXOBJ_Bitmap_t* pBitmap);


/*yxl 2005-09-20 add this function*/
extern BOOL CH_SetViewPortAlpha(CH_ViewPort_t CHVPHandle,int AlphaValue);


/*yxl 2005-10-25 add this function*/
extern BOOL CH_DisplayCHVP(CH_ViewPort_t LayerVPHandle,CH_ViewPortHandle_t VPHandle);

/*yxl 2005-11-09 add this function*/
/*Display I-FRAM from memory*/
#if 0/*yxl 2007-04-28 modify below section,add new paramters add modify inner*/
extern BOOL CH7_FrameDisplay(U8 *pMembuf,U32 Size);
#else
/*extern BOOL CH7_FrameDisplay(U8 *pMembuf,U32 Size,Partition_t Ption,CH_VIDCELL_t CHVIDCell);  */

#endif/*end yxl 2007-04-28 modify below section,add new paramters add modify inner*/


extern BOOL CH_InitGraphicBase(CH_GraphicInitParams* pInitParams); /*yxl 2007-02-09 add this function*/

#endif


extern BOOL CH6_GetBitmapInfo(U32 VPHandle,STGXOBJ_Bitmap_t* pBitmap);

extern int CH6_GetCharWidth(char Char);


extern BOOL CH6_LDR_TextOut(STGFX_Handle_t Handle,STLAYER_ViewPortHandle_t VPHandle,int PosX,int PosY,int TextCorlorIndex,
				 unsigned char* pTextString);

/*yxl 2007-09-03 add below function*/
extern char* CHDRV_OSDB_GetVision(void);


/*End of file*/
