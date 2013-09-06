#include "j_osd.h"
#include "..\usif\graphicmid.h"
#include "j_gendef.h"
#include "graphcom.h"
#ifdef USE_ARGB8888
#include "..\main\initterm.h"
#include "..\include\Graphicbase.h"
#elif USE_ARGB1555	
#include "..\main\initterm.h"
#include "..\include\Graphicbase.h"
#elif USE_ARGB4444	
#include "..\main\initterm.h"
#include "..\include\Graphicbase.h"

extern ST_Partition_t* system_partition ;
#endif


#if 0
extern ST_Partition_t* CHSysPartition;
extern STGFX_Handle_t GFXHandle;
extern CH_ViewPort_t CH6VPOSD;
extern STGFX_GC_t gGC;
#endif
S8 CH_EnableCursorShow = FALSE;
void CH_AudioPicDrawFunc(UINT8* pcPicData, UINT32 nWidth, UINT32 nHeight);
#if 0
boolean CH6_AVIT_GetRectangle(STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, U8* pMemory, int PosX, int PosY, int Width, int Height);
#endif
UINT8 *sqCursorArrowData, *sqCursorHandData,*sqCursorWaitData,*sqCursorEditData, *sqSaveCurrentdata = NULL;

/*设置显示分辨率*/
INT32 J_OSD_SetResolution(UINT32 width, UINT32 heigth)
{

	return J_SUCCESS;
}

/*打开OSD显示*/
void J_OSD_EnableDisplay(void)
{
/*	CH6_MixLayer( TRUE, TRUE, TRUE );*/
	return;
}

/*关闭OSD显示*/
void J_OSD_DisableDisplay(void)
{
/*	CH6_MixLayer( TRUE, TRUE, FALSE );*/
	return;
}

/*清除整个屏幕上的文字和图形*/
void J_OSD_ClearScreen(void)
{
	CH6_ClearFullArea(GFXHandle, CH6VPOSD.ViewPortHandle);
	return;
}


/*清除屏幕中某一个矩形区域的文字和图形*/
INT32 J_OSD_ClearRect(INT32 nX, INT32 nY, INT32 nWidth, INT32 nHeight)
{
	/*int i , j;*/
	if(nWidth &&nHeight )
		CH6_ClearArea(GFXHandle, CH6VPOSD.ViewPortHandle,nX, nY, nWidth, nHeight);
	else 
		return J_FAILURE;
	/*	
	CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,
		&gGC, nX, nY, nWidth, nHeight,(INT32)0x0fff,(INT32)0x0fff,1);*/
  #ifdef ENABLE_CH_PRINT		
	CH_PutError("J_OSD_ClearRect(%d, %d, %d, %d)\n",nX, nY, nWidth, nHeight );
  #endif
	return J_SUCCESS;
}

/*保存OSD透明级别,SetOrGet == true表示保存数据, 
SetOrGet == false表示获取数据, 如果是获取,返回透明级别*/
INT32 CH_SaveLevel(BOOL SetOrGet, INT32 pLevel)
{
 	static INT32 sLevel = 100;
	
	if(SetOrGet)
	{
		sLevel = pLevel;
	}
	return sLevel;
}

/*设置OSD的透明级别*/
INT32 J_OSD_SetTransparency(INT32 nLevel)
{
	CH_SaveLevel(true, nLevel);
	return J_SUCCESS;
}

/*取得当前OSD的透明级别*/
INT32 J_OSD_GetTransparency(void)
{
	return CH_SaveLevel(false, 0);
}


/*画点*/
INT32 J_OSD_DrawPixel(INT32 nX, INT32 nY, T_OSD_ColorRGB * pColor)
{
	UINT16 nColor;
	/*
	nColor  =   CH_SaveLevel(false, 0) ;*/
	nColor  = 0x8000;;
    	nColor |= ((((UINT16)(pColor->cRed)) << 4)  & 0x0f00);
    	nColor |= (((UINT16)(pColor->cGreen)) & 0x00f0);
    	nColor |= (((UINT16)(pColor->cBlue) >> 4)  & 0x000f);
	#if 0
	UINT8 color[2];
	color[0] =/*(((UINT8)(CH_SaveLevel(false, 0) * 0xff / 100)) & 0xf0)*/ 0xf0 + (pColor->cRed >> 4);
	color[1] = (pColor->cGreen & 0xf0) + (pColor->cBlue >> 4);
	CH_PutError("+");
	#endif 
#ifdef USE_ARGB8888
	if(!CH6_SetPixelColor(GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,	\
					 nX,nY,(INT32)nColor))
#elif USE_ARGB1555	
	if(!CH6_SetPixelColor(GFXHandle, CH6VPOSD,&gGC,	\
					 nX,nY,(INT32)nColor))
#elif USE_ARGB4444	
	if(!CH6_SetPixelColor(GFXHandle, CH6VPOSD,&gGC,	\
					 nX,nY,(INT32)nColor))					 
#endif
		return J_SUCCESS;
	return J_FAILURE;
}

/*填充矩形*/
INT32 J_OSD_FillRect(INT32 nX, INT32 nY, INT32 nWidth, INT32 nHeight, T_OSD_ColorRGB * pColor)
{
	UINT16 nColor;
	/*
	nColor  =   CH_SaveLevel(false, 0) ;*/
	nColor  = 0x8000;;
    	nColor |= ((((UINT16)(pColor->cRed)) << 4)  & 0x0f00);
    	nColor |= (((UINT16)(pColor->cGreen)) & 0x00f0);
    	nColor |= (((UINT16)(pColor->cBlue) >> 4)  & 0x000f);
	#if 0
	UINT8 nColor[2];
	nColor[0] = (((UINT8)(CH_SaveLevel(false, 0) * 0xff / 100)) & 0xf0 )+ (pColor->cRed >> 4);
	nColor[1] = (pColor->cGreen & 0xf0) + (pColor->cBlue >> 4);
	#endif
	
 	if(CH6m_DrawRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,
		&gGC, nX, nY, nWidth, nHeight,(INT32)nColor,(INT32)nColor,1))
		return J_SUCCESS;
#ifdef ENABLE_CH_PRINT	
	CH_PutError("ch_interface : CH6m_DrawRectangle() return error!\n");
#endif
	return J_FAILURE;
	
}
#if 0
/*bk_map :  背景图片. 如果不需要背景叠加, 置为NULL即可*/
INT32 J_OSD_BlockCopy(T_OSD_Rect * pDest, void * pMemory, int b_haveBk, void* bk_map,T_OSD_Rect* bk_rect)
{
	T_OSD_Rect pSrc;
	
	pSrc.nHeight = pDest->nHeight;
	pSrc.nWidth = pDest->nWidth;
	pSrc.nX = 0;
	pSrc.nY = 0;
	return 
		J_OSD_BlockCopyEx(&pSrc, pDest, pMemory,  b_haveBk,  bk_map, bk_rect);
}

/*
b_haveBk == 1: 需要背景叠加,  如果此时bk_map为NULL, 则从屏幕直接获取背景,否则使用bk_map指定的图像进行叠加
                == 0 :直接图像输出, 无须叠加 , 此时bk_map和bk_rect无效
bk_map :  背景图片. 
bk_rect : 背景图片的位置参数
*/
INT32 J_OSD_BlockCopyEx(T_OSD_Rect *pSrc, T_OSD_Rect *pDest, void* pMemory, int b_haveBk, void* bk_map,T_OSD_Rect* bk_rect)
{
	UINT8 *dest, *smap, *dmap; 
	UINT8 * src, *mid;
	
#ifdef USE_ARGB8888	
U32 *sp, *dp;

#else
	short *sp, *dp;
#endif
	INT32 new_w, new_h,out_X,out_Y, i, j, src_pitch, dest_pitch;
	T_OSD_Rect rect;
	STGXOBJ_Bitmap_t OSDBMPTemp;

	if(pMemory == NULL || pSrc == NULL || pDest == NULL)
	{
		return J_FAILURE;
	}
	
	if((pDest->nWidth>pSrc->nWidth)||(pDest->nHeight > pSrc->nHeight))
		return J_FAILURE;


	if(CH6_GetBitmapInfo(CH6VPOSD.ViewPortHandle,&OSDBMPTemp))
	{
		return J_FAILURE;
	}
#ifdef USE_ARGB8888	
/*做一个偏移处理*/
	memcpy(&rect, pDest, sizeof(T_OSD_Rect));
	rect.nX += ch_SCREEN_ADJUST_X;
	rect.nY +=  ch_SCREEN_ADJUST_Y;
	pDest = &rect;
	
	out_X = (pDest->nX < 0) ? 0 : pDest->nX;
	out_Y = (pDest->nY < 0) ? 0 : pDest->nY;
	new_w = ((out_X+pDest->nWidth) > OSDBMPTemp.Width) ?( OSDBMPTemp.Width-out_X) : pDest->nWidth;
	new_h = ((out_Y+pDest->nHeight) > OSDBMPTemp.Height) ?  (OSDBMPTemp.Height-out_Y) : pDest->nHeight;

	new_w = new_w > ch_SCREEN_WIDTH ? ch_SCREEN_WIDTH : new_w;
	new_h = new_h > ch_SCREEN_HIGH ? ch_SCREEN_HIGH : new_h;
	
	dest = (UINT8*) OSDBMPTemp.Data1_p+  out_Y  * OSDBMPTemp.Pitch +4 * out_X;
	
	src = (UINT8*)pMemory + 4*(pSrc->nY*pSrc->nWidth + pSrc->nX);
	
	
/*处理需要背景叠加的情况 sqzow 20061126*/
	if(b_haveBk == 1)
	{
		mid = (UINT8*)CH_AllocMem(pDest->nHeight * pDest->nWidth * 4);
		if(mid == NULL)
		{
			do_report(0,"j_osd.c : blockcpy can't get memory!\n");
			return J_FAILURE;
		}
		if(bk_map == NULL)/*从屏幕拷贝*/
		{
	
			pDest->nX -=  ch_SCREEN_ADJUST_X;
			pDest->nY -=  ch_SCREEN_ADJUST_Y;
			J_OSD_BlockRead(pDest, mid);
			
		}
		else/*直接使用bk_map*/
		{
			src_pitch = bk_rect->nWidth * 4;
			dest_pitch = pDest->nWidth * 4;
			smap = (UINT8*)bk_map + bk_rect->nY * src_pitch + bk_rect->nX * 4;
			dmap = mid;
			for(i = 0;  i < new_h; i++)
			{
				memcpy(dmap, smap, dest_pitch);
				smap += src_pitch;
				dmap += dest_pitch;
			}
		}
		src_pitch = pSrc->nWidth * 4;
		dest_pitch = pDest->nWidth * 4;
		smap = src;
		dmap = mid;
		for(i = 0;  i < new_h; i++)
		{
			sp = (U32*)smap;
			dp = (U32*)dmap;
			for(j = 0; j < new_w; j ++)
			{
				if(sp[j] & 0xff000000)
				{
					dp[j] = sp[j];
				}
			}
			smap += src_pitch;
			dmap += dest_pitch;
		}
		if(STAVMEM_CopyBlock2D((void*)mid, pDest->nWidth * 4, pDest->nHeight, pDest->nWidth*4,(void*)dest, OSDBMPTemp.Pitch)!=ST_NO_ERROR)
		{
			CH_FreeMem(mid);
			return J_FAILURE;
		}
		CH_FreeMem(mid);
		return J_SUCCESS;
	}
/*sqzow end*/

	if(STAVMEM_CopyBlock2D((void*)src, new_w * 4, new_h, pSrc->nWidth*4,(void*)dest, OSDBMPTemp.Pitch)!=ST_NO_ERROR)
	{
		return J_FAILURE;
	}


	/*更新光标的显示
	if(CH_EnableCursorShow)
		J_OSD_CursorShow();*/
	return J_SUCCESS;


#else
	
	/*做一个偏移处理*/
	memcpy(&rect, pDest, sizeof(T_OSD_Rect));
	rect.nX += ch_SCREEN_ADJUST_X;
	rect.nY +=  ch_SCREEN_ADJUST_Y;
	pDest = &rect;
	
	out_X = (pDest->nX < 0) ? 0 : pDest->nX;
	out_Y = (pDest->nY < 0) ? 0 : pDest->nY;
	new_w = ((out_X+pDest->nWidth) > OSDBMPTemp.Width) ?( OSDBMPTemp.Width-out_X) : pDest->nWidth;
	new_h = ((out_Y+pDest->nHeight) > OSDBMPTemp.Height) ?  (OSDBMPTemp.Height-out_Y) : pDest->nHeight;

	new_w = new_w > ch_SCREEN_WIDTH ? ch_SCREEN_WIDTH : new_w;
	new_h = new_h > ch_SCREEN_HIGH ? ch_SCREEN_HIGH : new_h;
	
	dest = (UINT8*) OSDBMPTemp.Data1_p+  out_Y  * OSDBMPTemp.Pitch +2 * out_X;
	
	src = (UINT8*)pMemory + 2*(pSrc->nY*pSrc->nWidth + pSrc->nX);
	
	
/*处理需要背景叠加的情况 sqzow 20061126*/
	if(b_haveBk == 1)
	{
		mid = (UINT8*)CH_AllocMem(pDest->nHeight * pDest->nWidth * 2);
		if(mid == NULL)
		{
			do_report(0,"j_osd.c : blockcpy can't get memory!\n");
			return J_FAILURE;
		}
		if(bk_map == NULL)/*从屏幕拷贝*/
		{
		
			pDest->nX -=  ch_SCREEN_ADJUST_X;
			pDest->nY -=  ch_SCREEN_ADJUST_Y;
			J_OSD_BlockRead(pDest, mid);
			
		}
		else/*直接使用bk_map*/
		{
			src_pitch = bk_rect->nWidth * 2;
			dest_pitch = pDest->nWidth * 2;
			smap = (UINT8*)bk_map + bk_rect->nY * src_pitch + bk_rect->nX * 2;
			dmap = mid;
			for(i = 0;  i < new_h; i++)
			{
				memcpy(dmap, smap, dest_pitch);
				smap += src_pitch;
				dmap += dest_pitch;
			}
		}
		src_pitch = pSrc->nWidth * 2;
		dest_pitch = pDest->nWidth * 2;
		smap = src;
		dmap = mid;
		for(i = 0;  i < new_h; i++)
		{
			sp = (short*)smap;
			dp = (short*)dmap;
			for(j = 0; j < new_w; j ++)
			{
				if(sp[j] & 0xf000)
				{
					dp[j] = sp[j];
				}
			}
			smap += src_pitch;
			dmap += dest_pitch;
		}
		if(STAVMEM_CopyBlock2D((void*)mid, pDest->nWidth * 2, pDest->nHeight, pDest->nWidth*2,(void*)dest, OSDBMPTemp.Pitch)!=ST_NO_ERROR)
		{
			CH_FreeMem(mid);
			return J_FAILURE;
		}
		CH_FreeMem(mid);
		return J_SUCCESS;
	}
/*sqzow end*/

	if(STAVMEM_CopyBlock2D((void*)src, new_w * 2, new_h, pSrc->nWidth*2,(void*)dest, OSDBMPTemp.Pitch)!=ST_NO_ERROR)
	{
		return J_FAILURE;
	}


	/*更新光标的显示
	if(CH_EnableCursorShow)
		J_OSD_CursorShow();*/
	return J_SUCCESS;
	#endif
}

#endif

/*把屏幕上某一矩形区域的图形拷贝到内存*/
INT32 J_OSD_BlockRead(T_OSD_Rect * pRect, void * pMemory)
{
	BOOL error;
	T_OSD_Rect rect;
	U8 *up_Data;
	int i_width;
	int i_height;
/*
	CH_PutError("ch_interface : CH6_GetRectangle[%d, %d, %d, %d, memaddr = 0x%x] !\n",pRect->nX, pRect->nY, pRect->nWidth, pRect->nHeight,pMemory);
*/
/*做一个偏移处理*/
	memcpy(&rect, pRect, sizeof(T_OSD_Rect));
	rect.nX  += ch_SCREEN_ADJUST_X;
	rect.nY +=  ch_SCREEN_ADJUST_Y;
	pRect = &rect;

	i_width = (pRect->nWidth > 720 -pRect->nX) ?(720 -pRect->nX) : pRect->nWidth ;
	i_height = (pRect->nHeight > 576 - pRect->nY) ? ( 576 - pRect->nY) : pRect->nHeight;
#if 0	
	error = CH6_AVIT_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC, (U8*)pMemory, pRect->nX, pRect->nY, 			\
		(pRect->nWidth > 720 -pRect->nX) ?(720 -pRect->nX) : pRect->nWidth  ,			\
		(pRect->nHeight > 576 - pRect->nY) ? ( 576 - pRect->nY) : pRect->nHeight);
#else
#ifdef USE_ARGB8888
up_Data = CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC, pRect->nX, pRect->nY,i_width,i_height); 	
#elif USE_ARGB1555	
up_Data = CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC, pRect->nX, pRect->nY,i_width,i_height); 	
#elif USE_ARGB4444	
up_Data = CH6_GetRectangle(GFXHandle,CH6VPOSD.ViewPortHandle, &gGC,pRect->nX, pRect->nY,i_width,i_height); 

#endif
if(up_Data!=NULL)
{
#ifdef USE_ARGB8888	
   memcpy(pMemory,up_Data,i_width*i_height*4);
#elif USE_ARGB1555
   memcpy(pMemory,up_Data,i_width*i_height*2);
#elif USE_ARGB4444
   memcpy(pMemory,up_Data,i_width*i_height*2);

#endif
#ifdef USE_ARGB8888
   memory_deallocate(CHSysPartition, up_Data);
#elif USE_ARGB1555	
   memory_deallocate(chsys_partition, up_Data);
#elif USE_ARGB4444	
   memory_deallocate(system_partition, up_Data);

#endif
}


#endif
	if(error)
	{
  #ifdef ENABLE_CH_PRINT	
		CH_PutError("ch_interface : CH6_GetRectangle[%d, %d, %d, %d] return error!\n",pRect->nX, pRect->nY, pRect->nWidth, pRect->nHeight);
  #endif
		return J_FAILURE;
	}
/*
	CH_PutError("1ch_interface : CH6_Ge");
	CH_PutError("2ch_interface : CH6_Ge");
	CH_PutError("3ch_interface : CH6_Ge");
	CH_PutError("4ch_interface : CH6_Ge");*/
	return J_SUCCESS;
}
#if 0
boolean CH6_AVIT_GetRectangle(STGFX_Handle_t Handle, STLAYER_ViewPortHandle_t VPHandle,STGFX_GC_t* pGC, U8* pMemory, int PosX, int PosY, int Width, int Height)
{
  	ST_ErrorCode_t ErrCode;
  	
	STGXOBJ_Bitmap_t OSDBMPTemp;

	STGXOBJ_Bitmap_t BMPTemp;
	STGFX_GC_t* pGCTemp;
	STGXOBJ_Rectangle_t RectTemp;

#if 0 /*yxl 2004-11-10 modify this line*/
	pGCTemp=&gGC;
#else
	pGCTemp=pGC;
#endif/*end yxl 2004-11-10 modify this line*/

	if(CH6_GetBitmapInfo(VPHandle,&OSDBMPTemp))
	{
		/*return TRUE;*/
		return NULL;		
	}	


	memcpy((void*)&BMPTemp,(const void*)&OSDBMPTemp,sizeof(STGXOBJ_Bitmap_t));

	BMPTemp.Width=Width;
	BMPTemp.Height=Height;
	BMPTemp.Offset=0;
/*	BMPTemp.Data1_p=pBMPData;*/


/*	BMPTemp.Pitch=Width;
	BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height;
*/
	switch(BMPTemp.ColorType)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:
			BMPTemp.Pitch=Width;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height;
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:
		case STGXOBJ_COLOR_TYPE_ARGB4444:
		case STGXOBJ_COLOR_TYPE_RGB565:
			BMPTemp.Pitch=Width*2;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*2;	
			break;
		case STGXOBJ_COLOR_TYPE_ARGB8888:
				BMPTemp.Pitch=Width*4;
			BMPTemp.Size1=BMPTemp.Width*BMPTemp.Height*4;	
			 break;
		default:
		  #ifdef ENABLE_CH_PRINT	
			CH_PutError(("\n YxlInfo():not supported color type \n"));
		  #endif
			return TRUE;
	}

	BMPTemp.Data1_p = pMemory;

	RectTemp.PositionX=PosX;
	RectTemp.PositionY=PosY;

	RectTemp.Width=Width;
	RectTemp.Height=Height;

/*	ErrCode=STGFX_CopyArea(Handle,pOSDBMPTemp,&BMPTemp,pGCTemp,&RectTemp,0,0);*/

	ErrCode=STGFX_CopyArea(Handle,&OSDBMPTemp,&BMPTemp,pGCTemp,&RectTemp,0,0);
	if(ErrCode!=ST_NO_ERROR)
	{
  #ifdef ENABLE_CH_PRINT	
		CH_PutError("CH6_GetRectangle()=%s",GetErrorText(ErrCode));
  #endif
		return TRUE;
	}

	ErrCode=STGFX_Sync(Handle);
	if(ErrCode!=ST_NO_ERROR)
	{
  #ifdef ENABLE_CH_PRINT	
		CH_PutError("STGFX_Sync()=%s",GetErrorText(ErrCode));
  #endif
		return TRUE;
	}

	return FALSE;
}
#endif
