#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>   

#include "stddefs.h"
#include "stlite.h"
#include "stdevice.h"
#include "stcommon.h"
#include "appltype.h"


#include "graphcom.h"
#include "ch_CQInfoService.h"


#include "..\usif\Graphicmid.h"

#include "..\key\keymap.h"
#include "..\ch_pic\draw_gif.h"
#include "..\dbase\vdbase.h"
#ifdef USE_ARGB8888
#include "..\main\initterm.h"
#include "..\include\Graphicbase.h"
#elif USE_ARGB1555	
#include "..\main\initterm.h"
#include "..\include\Graphicbase.h"

#elif USE_ARGB4444	
#include "..\main\initterm.h"
#include "..\include\Graphicbase.h"


#endif
/*已添加的窗口列表*/
U32 ch_AllWinodws[ch_MaxWindowsNum + 1] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; 

/*屏幕缓冲区地址*/
U32	ch_ScrenBufferHand = 0;


/*系统默认字体信息*/
CH_FONT AutoFonts; 

/*系统默认前景背景颜色*/
PCH_RGB_COLOUR pAutoBackgroundColor, pAutoCurrentColor;

#ifdef USE_ARGB8888
extern ST_Partition_t* CHSysPartition ;
#elif USE_ARGB1555	
extern ST_Partition_t* chsys_partition ;
#elif USE_ARGB4444	
extern ST_Partition_t* system_partition ;

#endif

/*windows 专有*/


INT ch_GetMyKey = 0;



/*
* 以下函数仅限于内部使用，所以放在此处
*/
CH_STATUS CH_DrawJpeg(S8* FileName,PCH_WINDOW ch_this,  PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len);
CH_STATUS CH_DrawGif(S8* FileName,PCH_WINDOW ch_this,  PCH_SHOWRECT p_ShowRect, PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len);
/*
CH_STATUS CH_DrawPng(S8* FileName,PCH_WINDOW ch_this,  PCH_RECT RectSize, id_backgroundRepeat repeat);
*/

CH_STATUS CH_DrawPcx(S8* FileName,PCH_WINDOW ch_this,  PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len);

CH_STATUS CH_DrawBmp(S8* FileName,PCH_WINDOW ch_this,  PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len);

PCH_RECT CH_GetJpegSize(S8* FileName, int FileSize);
PCH_RECT CH_GetGifSize(S8* FileName,int FileSize);
PCH_RECT GetGifSize(U8 *Filebuffer, int FileSize);
PCH_RECT CH_GetPicSize(S8* FileName,int FileType,int FileSize);

/*
PCH_RECT CH_GetPngSize(S8* FileName);
*/
PCH_RECT CH_GetBmpSize(S8* FileName, int FileSize);
PCH_RECT CH_GetPcxSize(S8* FileName);
/*
* 声明结束
*/


/*****************************************************************
* 新建一个窗口
* 输入：
*	iWindowWidth		:窗口显示宽度（像素）
*     iWindowHigh		:窗口显示高度（像素）
*				一般使用默认的窗口宽度和高度即可
* 返回值：
*	成功返回窗口句柄，失败返回NULL；
*****************************************************************/
PCH_WINDOW CH_CreateWindow(S16 iWindowWidth, S16 iWindowHigh)
{
	PCH_WINDOW ch_this = 0;
	int i;
	int colorbyte;
	
#ifdef USE_ARGB8888
	colorbyte = 4;
#elif USE_ARGB1555	
	colorbyte = 2;
#elif USE_ARGB4444	
	colorbyte = 2;

#endif

	/*初始化缓冲区*/
#ifdef ENABLE_CH_PRINT
	CH_Printf("create windows: %d, %d\n", iWindowWidth, iWindowHigh);
#endif
	ch_this = (PCH_WINDOW)CH_AllocMem(sizeof(CH_WINDOW));
	if(ch_this == NULL)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("!!!ERROR graphcom :  Not enough memory, CH_CreateWindow return NULL.\n");
#endif
		return NULL;
	}
	
	ch_this->scrBuffer.dwHandle = 0;
	
	ch_this->scrBuffer.dwHandle = (U32)CH_AllocMem(iWindowWidth* iWindowHigh* colorbyte);
	ch_this->ColorByte = colorbyte;
	if(ch_this->scrBuffer.dwHandle == 0)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("!!!ERROR graphcom :  Not enough memory, CH_CreateWindow return NULL.\n");
#endif
		CH_FreeMem(ch_this);
		return NULL;
	}
	CH_MemSet((void *) ch_this->scrBuffer.dwHandle, 0, iWindowWidth* iWindowHigh * ch_this->ColorByte);
	
	ch_this->scrBuffer.pNext = NULL;
	
	/*初始化屏幕参数*/
	ch_this->SingleScrHeight =  		\
		ch_this->Screen.iHeigh = iWindowHigh;
	ch_this->SingleScrWidth = 		\
		ch_this->Screen.iWidth = iWindowWidth;
	
	ch_this->Screen.iStartX = 0;
	ch_this->Screen.iStartY = 0;
	
	/*初始化颜色*/
	ch_this->BackgroundColor = pAutoBackgroundColor;
	ch_this->CurrentColor = pAutoCurrentColor;
	
	/*初始化活动结点*/
	ch_this->ActNode = NULL;
	ch_this->DlgControl = NULL;
	ch_this->ControlHead = NULL;
	
	
	/*把当前窗口添加到窗口列表*/
	for(i = 0; i < ch_MaxWindowsNum; i++)
	{
		if(ch_AllWinodws[i] == 0)
		{
			ch_AllWinodws[i] = (U32)ch_this;
			break;
		}
	}
	return ch_this;
}

/*****************************************************************
* 销毁一个窗口
* 输入：
*	ch_this	:虚设备句柄

  * 返回值：
  *	(参照 CH_STATUS 类型定义)
*****************************************************************/
CH_STATUS CH_DestroyWindow(PCH_WINDOW ch_this)
{
	PCH_WINDOW_BUF ptr1, ptr2;
	S16 i;
	U8* buf;
	if(ch_this == NULL) 
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("ERROR graphcom : The window is not exist, can't destroy.\n");
#endif
		return CH_ERROR_PARAM;
	}
	/*从列表删除*/
	for(i = 0; i < ch_MaxWindowsNum; i++)
	{
		if(ch_AllWinodws[i] == (U32)ch_this)
		{
			ch_AllWinodws[i] = 0;
			break;
		}
	}
	if(i == ch_MaxWindowsNum)
	{
		return CH_FAILURE;
	}
	
	/*释放屏幕缓冲区*/
	ptr1 = &ch_this->scrBuffer;
	while(ptr1)
	{
		if(ptr1->dwHandle)
		{
			buf = (U8*)ptr1->dwHandle;
			CH_FreeMem(buf);
			ptr1->dwHandle = 0;
		}
		ptr1 = ptr1->pNext;
	}
	
	/*释放缓冲区链表*/
	ptr1 = ch_this->scrBuffer.pNext;
	while(ptr1)
	{
		ptr2 = ptr1->pNext;
		CH_FreeMem(ptr1);
		ptr1 = ptr2;
	}
	
	
	/*释放窗口*/
	CH_FreeMem(ch_this);
	
	
	return CH_SUCCESS;
}


/*****************************************************************
* 销毁所有打开的窗口
* 输入：
*	（无）

  * 返回值：
  *	(参照 CH_STATUS 类型定义)
*****************************************************************/
CH_STATUS CH_DestroyAllWindows(void)
{
	int i;
	
	for(i = 0; i < ch_MaxWindowsNum; i++)
	{
		if(ch_AllWinodws[i] != 0)
		{
			CH_DestroyWindow((PCH_WINDOW)ch_AllWinodws[i]);
			ch_AllWinodws[i] = 0;
		}
	}
	ch_AllWinodws[ch_MaxWindowsNum] = 0;
	return CH_SUCCESS;
}










/*****************************************************************
* 在指定窗口上画1幅gif,jpeg,png,或bmp图象
* 输入：
* 	 FileName: 图象文件完整的路径名称
*  	ch_this: 窗口句柄
*  	RectSize: 图象位置,大小
*  	repeat: 指定是否平铺,定义参考ch_css.h中id_backgroundRepeat的定义
*
* 返回值：
*	(参照 CH_STATUS 类型定义)
*****************************************************************/
#if 0
#include "br_file.h"
#endif
#if 0
CH_STATUS CH_DrawPictrue(S8* FileName,PCH_WINDOW ch_this, PCH_SHOWRECT p_ShowRect,  PCH_RECT RectSize,id_backgroundRepeat repeat)
{

    CH_STATUS valu;
    chz_debug_timeused(0, "CH_DrawPictrue start");
	
    RectSize->iHeigh = CH_MIN(RectSize->iHeigh, ch_SCREEN_HIGH); 
    RectSize->iWidth = CH_MIN(RectSize->iWidth, ch_SCREEN_WIDTH); 
    RectSize->iHeigh = CH_MIN(RectSize->iHeigh, ch_SCREEN_HIGH - RectSize->iStartY); 
    RectSize->iWidth = CH_MIN(RectSize->iWidth, ch_SCREEN_WIDTH- RectSize->iStartX); 

#if 0
{
	char str[20];
	graph_file* file;
	sprintf(str, "c:\\mode2\\pic%d", FileName);
	file = graph_open((char*)FileName, NULL);
	if(file)
	{
		chz_put2c(str, file->linked_obj_addr, file->obj_size);
		graph_close(file);
	}
}
#endif
    switch(CH_GetPicType( FileName))
	{
   	case CPT_JPG:
	case CPT_JPEG:
	case CPT_JPE:
	case CPT_JFIF:
		
		valu =  
			CH_DrawJpeg( FileName,ch_this,  RectSize, repeat);
		break;
		
	case CPT_GIF:
		valu =  
			CH_DrawGif( FileName,ch_this, p_ShowRect, RectSize, repeat);
		break;
		
	case CPT_BMP:
	case CPT_DIB:
		valu =  
			CH_DrawBmp( FileName,ch_this,  RectSize, repeat);
		break;
		
	case CPT_PNG:
  		/*return 
		CH_DrawPng( FileName,ch_this,  RectSize, repeat);*/
		valu = CH_FAILURE;
		break;
	case CPT_PCX:
		valu =  
			CH_DrawPcx( FileName,ch_this,  RectSize, repeat);
		break;
	default:
		valu = CH_FAILURE;
		break;
	}
	chz_debug_timeused(0, "CH_DrawPictrue end");
	return valu;
	
}

#endif

CH_STATUS CH_PutBitmapToScr3(PCH_WINDOW ch_this, PCH_RECT psrcRect, PCH_RECT pdestRect, int trans, S8* bitmap)
{
	S32 i , j, num, startx, starty, copyheigh;
	S8* srcmap, *sbuf,*dbuf, *destmap;
	CH_RECT RangleSize;
	PCH_WINDOW_BUF winbuf;
	
	if(bitmap == NULL || ch_this == NULL || psrcRect == NULL ||pdestRect== NULL )
		return CH_FAILURE;
	num = pdestRect->iStartY / ch_this->SingleScrHeight;
	winbuf = &(ch_this->scrBuffer);
	for(i = 0; i < num ; i++)
		winbuf = winbuf->pNext;
	startx = pdestRect->iStartX;
	starty = pdestRect->iStartY;
	destmap = (S8*)( winbuf->dwHandle + (startx + starty * ch_this->SingleScrWidth) * ch_this->ColorByte);
	srcmap = (S8*)( bitmap + (psrcRect->iStartX+ psrcRect->iStartY * psrcRect->iWidth) * ch_this->ColorByte);
	copyheigh = CH_MIN(pdestRect->iHeigh, ch_this->SingleScrHeight -starty);
	if(trans)
	{
		for(i = 0;  i < copyheigh; i++)
		{
			dbuf=destmap;
			sbuf= srcmap;
			for(j = 0; j < pdestRect->iWidth; j ++)
			{
#ifdef USE_ARGB8888			
				if((*((U8*)sbuf + 3)) & 0xFF)
#elif  USE_ARGB1555
			    if((*((U8*)sbuf + 1)) & 0x80)
#elif  USE_ARGB4444
			    if((*((U8*)sbuf + 1)) & 0xf0)					
#endif
				{
					memcpy(dbuf, sbuf, ch_this->ColorByte);
				}
				sbuf += ch_this->ColorByte;
				dbuf += ch_this->ColorByte;
			}
			destmap += ch_this->SingleScrWidth * ch_this->ColorByte;
			srcmap += psrcRect->iWidth * ch_this->ColorByte;
		}
	}
	else
	{
		for(i = 0;  i < copyheigh; i++)
		{
			memcpy(destmap,srcmap ,pdestRect->iWidth * ch_this->ColorByte);
			destmap += ch_this->SingleScrWidth * ch_this->ColorByte;
			srcmap += psrcRect->iWidth * ch_this->ColorByte;
		}
	}
	if(i == pdestRect->iHeigh)
		return CH_SUCCESS;
	return CH_SUCCESS;
}

/*****************************************************************
* 在指定窗口上画1幅gif,jpeg,png,或bmp图象
* 输入：
* 	 FileName: 图象文件完整的路径名称
*  	ch_this: 窗口句柄
*  	RectSize: 图象位置,大小
*  	repeat: 指定是否平铺,定义参考ch_css.h中id_backgroundRepeat的定义
*
* 返回值：
*	(参照 CH_STATUS 类型定义)
*****************************************************************/
#if 0
#include "br_file.h"
#endif
CH_STATUS CH_DrawPictrue(U8* FileName,PCH_WINDOW ch_this, id_backgroundRepeat repeat,int ri_len,CH_PictureType rch_FileType)
{
    CH_STATUS valu;
	/*PCH_WINDOW ch_this;*//*窗口*/
	PCH_SHOWRECT p_ShowRect;/**/
	CH_RECT RectSize;

    memset(&p_ShowRect,0,sizeof(PCH_SHOWRECT));
		
		
    /*chz_debug_timeused(0, "CH_DrawPictrue start");*/

	
	RectSize.iStartX=0;
	RectSize.iStartY=0;
	RectSize.iWidth=ch_this->SingleScrWidth;
	RectSize.iHeigh=ch_this->SingleScrHeight;/**/
	
    RectSize.iHeigh = CH_MIN(RectSize.iHeigh, ch_SCREEN_HIGH); 
    RectSize.iWidth = CH_MIN(RectSize.iWidth, ch_SCREEN_WIDTH); 
    RectSize.iHeigh = CH_MIN(RectSize.iHeigh, ch_SCREEN_HIGH - RectSize.iStartY); 
    RectSize.iWidth = CH_MIN(RectSize.iWidth, ch_SCREEN_WIDTH- RectSize.iStartX); 

    switch(rch_FileType)
	{
   	case CPT_JPG:
	case CPT_JPEG:
	case CPT_JPE:
	case CPT_JFIF:
		
		valu =  
			CH_DrawJpeg( (S8*)FileName,ch_this,  &RectSize, repeat,ri_len);
		break;
		
	case CPT_GIF:
		valu =  
			CH_DrawGif( (S8*)FileName,ch_this, p_ShowRect, &RectSize, repeat,ri_len);
		break;
		
	case CPT_BMP:
	case CPT_DIB:
		valu =  
			CH_DrawBmp( (S8*)FileName,ch_this,  &RectSize, repeat,ri_len);
		break;
		
	case CPT_PNG:
  		/*return 
		CH_DrawPng( FileName,ch_this,  RectSize, repeat);*/
		valu = CH_FAILURE;
		break;
	case CPT_PCX:
		/*valu =  */
		/*	CH_DrawPcx( FileName,ch_this,  RectSize, repeat,ri_len);*/
		break;
	default:
		valu = CH_FAILURE;
		break;
	}
	/*chz_debug_timeused(0, "CH_DrawPictrue end");*/
	return valu;
}
/*****************************************************************
* 得到FileName表示的图象类型
* 输入：
*  	FileName: 图象文件完整的路径名称
*
* 返回值：
*	成功返回图象类型,失败返回-1.
*	其中, 类型的对应关系为参见graphcom.h中CH_PictureType的定义
*****************************************************************/
CH_PictureType CH_GetPicType(S8* FileName)
{
    S8  j = 0;
	
    
	return j;
}


/*****************************************************************
* draw a point with the specifed color.
* Inputs:
*		dwHandle :a virtual drawing device handle or pointer.
*				(for example:region,drawing layer,or windows)
*				窗口句柄
*		pPoint       :Pointer of CH_POINT
*		pRgbColor    :point color
* 返回值：
*		(参照 CH_STATUS 类型定义)
*****************************************************************/

CH_STATUS CH_DrawColorPoint(PCH_WINDOW ch_this,PCH_POINT pPoint,PCH_RGB_COLOUR pRgbColor)
{
	int i;
	static PCH_WINDOW_BUF pBuf = NULL;
	PCH_WINDOW sch_this = NULL;
	
#ifdef USE_ARGB8888
	U8* buffer;
#elif USE_ARGB1555
	U16* buffer;
#elif USE_ARGB4444
	U16* buffer;

#endif

	static S16 n = - 1;
	
	
	if(ch_this == NULL || pPoint == NULL)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("ERROR graphcom : The window or the point is not exist, i can't draw it.\n");
#endif
		return CH_ERROR_PARAM;
	}
	
	if((pPoint->x < 0)||(pPoint->y < 0)		\
		||(pPoint->x > ch_this->Screen.iWidth)	\
		||(pPoint->y > ch_this->Screen.iHeigh))
		return CH_ERROR_PARAM;
	/*超出屏幕，失败*/
	
	pBuf = &(ch_this->scrBuffer);
	if(!pRgbColor)
		pRgbColor = ch_this->CurrentColor;
#ifdef USE_ARGB8888
	buffer = (U8*)(ch_this->scrBuffer.dwHandle+(( pPoint->y ) * ch_this->Screen.iWidth + pPoint->x )* ch_this->ColorByte);
	*(buffer + 3) = 0xff;
	*(buffer + 2) = CH_GET_RGB_COLOR(pRgbColor).uRed;
	*(buffer + 1) = CH_GET_RGB_COLOR(pRgbColor).uGreen;
	*(buffer + 0) = CH_GET_RGB_COLOR(pRgbColor).uBlue;
#elif USE_ARGB1555
	buffer = (U16*)(ch_this->scrBuffer.dwHandle+(( pPoint->y ) * ch_this->Screen.iWidth + pPoint->x )* ch_this->ColorByte);
	*buffer = 0x8000 + ((((U16)CH_GET_RGB_COLOR(pRgbColor).uRed)>>3) <<10) + ((((U16)CH_GET_RGB_COLOR(pRgbColor).uGreen)>>3) <<5) + (((U16)CH_GET_RGB_COLOR(pRgbColor).uBlue)>>3);
#elif USE_ARGB4444
		buffer = (U16*)(ch_this->scrBuffer.dwHandle+(( pPoint->y ) * ch_this->Screen.iWidth + pPoint->x )* ch_this->ColorByte);
		*buffer = 0xf000 + ((((U16)CH_GET_RGB_COLOR(pRgbColor).uRed)>>4) <<8) + ((((U16)CH_GET_RGB_COLOR(pRgbColor).uGreen)>>4) <<4) + (((U16)CH_GET_RGB_COLOR(pRgbColor).uBlue)>>4);

#endif
	return CH_SUCCESS;
	/*成功*/
}

void CH_FreeMem(void * pAddr) 
{
if(pAddr != NULL)
#ifdef USE_ARGB8888
	memory_deallocate(CHSysPartition, pAddr);
#elif USE_ARGB1555
	memory_deallocate(chsys_partition, pAddr);
#elif USE_ARGB4444
	memory_deallocate(system_partition, pAddr);

#endif
}
void *CH_AllocMem(U32 size)
{
#ifdef USE_ARGB8888
return    memory_allocate( CHSysPartition, size );
#elif USE_ARGB1555
return    memory_allocate( chsys_partition, size );
#elif USE_ARGB4444
return    memory_allocate( system_partition, size );

#endif
}


/*****************************************************************/
/*函数名:
BOOL OpenPic( U8 *Filebuffer, int FileSize, int FileType, int StartX, int StartY)
*/
/*开发人和开发时间:heming 2007-12-21                                */
/*函数功能描述: 显示图片入口函数                                */
/*函数原理说明:                                                           */
/*输入参数：Filebuffer 图片内存指针，FileSize		图片文件大小 FileType	
图片类型	JPG	1 GIF	5 BMP	6，	StartX，StartY	图片显示起始位置。                                                            */
/*输出参数:  无                                                           */
/*使用的全局变量、表和结构：gifhandle                                              */
/*调用的主要函数列表：                                                    */
/*返回值说明：0	SUCCESS 				-1	FAILURE				

					*/
/*调用注意事项: 图片类型     
 wz 20081218 添加了对纯黑当作透明的处理

*/
/*其他说明:动态透明GIF背景有点问题                                                             */ 
BOOL OpenPic( U8 *Filebuffer, int FileSize, int FileType, int StartX, int StartY)
{
	int 	error=-1;
	PCH_WINDOW		ch_this;
	PCH_RECT	FileInfo=NULL;
	int		DisplayTime;

	clock_t  starttime;
	int totaltime  = 90;

	int keycode;
	

	FileInfo=CH_GetPicSize((S8*)Filebuffer, FileType,FileSize);
	
	ch_this = CH_CreateWindow((S16)FileInfo->iWidth,(S16)FileInfo->iHeigh);
	if(ch_this != NULL)
	{
		error=CH_DrawPictrue( Filebuffer, ch_this, id_backgroundRepeat(no_repeat), FileSize,FileType);
		if (error==0)
		{
			if(gifhandle == NULL)
			{
#ifdef USE_ARGB8888
			CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,StartX,StartY,FileInfo->iWidth,FileInfo->iHeigh,(U8 *)ch_this->scrBuffer.dwHandle ); 
#elif USE_ARGB1555
			CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,StartX,StartY,FileInfo->iWidth,FileInfo->iHeigh,(U8 *)ch_this->scrBuffer.dwHandle ); 
#elif USE_ARGB4444
			CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,StartX,StartY,FileInfo->iWidth,FileInfo->iHeigh,(U8 *)ch_this->scrBuffer.dwHandle ); 

#endif
			CH_UpdateAllOSDView();
		}
		}
		else
		{
			CH_DestroyWindow(ch_this);
#ifdef USE_ARGB8888
			memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
			memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
			memory_deallocate(system_partition, FileInfo);


#endif
			if(gifhandle!=NULL)
				{
					mgif_DestroyHandle(gifhandle);
					gifhandle = NULL;
				}

			return error;
		}

		starttime = time_now();
			
		while(totaltime > 0)
		{
			if(pstBoxInfo->abiBoxPoweredState == false)
			{
				break;
			}
			  if(gifhandle!=NULL)
			   {
					DisplayTime=10*mgif_ShowNextFrame_pelstransparency(ch_this,gifhandle,StartX,StartY);
			   }
			  else
			  {
					break;
			  }

 #ifdef USE_ARGB8888
	   		  CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,StartX,StartY, FileInfo->iWidth,FileInfo->iHeigh,(U8 *)ch_this->scrBuffer.dwHandle ); 
#elif USE_ARGB1555
			  CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,StartX,StartY, FileInfo->iWidth,FileInfo->iHeigh,(U8 *)ch_this->scrBuffer.dwHandle ); 
#elif USE_ARGB4444
			  CH6_PutRectangle(GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,StartX,StartY, FileInfo->iWidth,FileInfo->iHeigh,(U8 *)ch_this->scrBuffer.dwHandle ); 

#endif
	   		
			  keycode = GetMilliKeyFromKeyQueue(DisplayTime);
			  	
			  if( keycode != -1)/* 有按键读入*/
			   {
		   		
		   		PutKeyIntoKeyQueue(keycode);
			   	break;
			   }


			totaltime = CH_GetLeaveTime(starttime,totaltime);
		  
		}
	CH_DestroyWindow(ch_this);
#ifdef USE_ARGB8888
	memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
	memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
	memory_deallocate(system_partition, FileInfo);

#endif
	if(gifhandle!=NULL)
		{
			mgif_DestroyHandle(gifhandle);
			gifhandle = NULL;
		}
	
		return error;
	}
	else
	{
		CH_DestroyWindow(ch_this);
#ifdef USE_ARGB8888
		memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
		memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
		memory_deallocate(system_partition, FileInfo);

#endif
		return error = 1;
	}
}

/*得到GIF图片的宽高*/
PCH_RECT GetGifSize(U8 *Filebuffer,int FileSize)
{
	S16		PicWidth;
	S16		PicHeigh;
	PCH_RECT	PicSize=NULL;
	graph_file* pFile = graph_open((char*)Filebuffer, "rb",FileSize);
	
		if(!pFile)
		{
			return NULL;
		}
		graph_seek(pFile,6,SEEK_SET);
		graph_read(&PicWidth,2,1,pFile);
		graph_read(&PicHeigh,2,1,pFile);
	
		PicSize = (PCH_RECT)CH_AllocMem(sizeof(CH_RECT));
		if(!PicSize)
		{
			return NULL;
		}
		PicSize->iStartX = 0;
		PicSize->iStartY = 0;
		PicSize->iWidth = PicWidth;
		PicSize->iHeigh = PicHeigh;
		return PicSize;
}
/*****************************************************************
* 得到1幅gif,jpeg,png,或bmp图象的长和宽
* 输入：
*  	Filebuffer:指向 图象文件的内存指针
	FileType:文件类型JPG	1 GIF	5 BMP	6，
*	FileSize:文件大小
*	
* 返回值：
*	成功返回图象大小,失败返回NULL
*****************************************************************/
PCH_RECT CH_GetPicSize(S8* Filebuffer,int FileType,int FileSize)
{
	PCH_RECT fileinfo = NULL;
#ifdef ENABLE_CH_PRINT
	CH_Printf("picture name = %s\n", Filebuffer);
#endif
	switch(FileType)
	{
	case CPT_JPG:
	case CPT_JPEG:
	case CPT_JPE:
	case CPT_JFIF:
			fileinfo = CH_GetJpegSize(Filebuffer,FileSize);
			
			return fileinfo;
		break;
	case CPT_GIF:
			fileinfo = GetGifSize((U8 *)Filebuffer,FileSize);
			
			return fileinfo;
		break;
	case CPT_BMP:
	case CPT_DIB:
		 
			fileinfo = CH_GetBmpSize(Filebuffer,FileSize);
			fileinfo->iHeigh= abs(fileinfo->iHeigh);
			return fileinfo;
		break;
	case CPT_PNG:
	/*
	return 
		CH_GetPngSize(Filebuffer);*/
		break;
	default:
		break;
	}
	return NULL;
}



#ifdef SUMA_SECURITY

int CH_ParsePicture( U8* SourceData,int SourceLEN,int type, int*data,int* i_width,int* i_height)
{
	int 	error=-1;
	PCH_WINDOW	ch_this;
	PCH_RECT	FileInfo=NULL;
	int width,height;

	int size = 0;
   
	int FileType = type;
	int FileSize = SourceLEN;

	U8* picdata = NULL;
	
	
	FileInfo=CH_GetPicSize((S8*)SourceData, FileType,FileSize);
	/* 解析得到图片的宽高*/
	if( FileInfo != NULL)
	{
		width= FileInfo->iWidth;
		height = FileInfo->iHeigh; 
#ifdef USE_ARGB8888
		size = width * height*4;
#else
		size = width * height*2;
#endif
		ch_this=CH_CreateWindow((S16)width,(S16)height);
	}
	else
	{
		return -1;
	}
	if(ch_this != NULL)
	{
		error=CH_DrawPictrue( SourceData, ch_this, id_backgroundRepeat(no_repeat), FileSize,FileType);

		if (error==0)
		{
			*i_width = width;
			*i_height = height;
	
#ifdef USE_ARGB8888
				picdata= memory_allocate(CHSysPartition,size);
#elif USE_ARGB1555
				picdata= memory_allocate(chsys_partition,size);
#elif USE_ARGB4444
			picdata= memory_allocate(system_partition,size);

#endif		
		
			

			if(picdata == NULL)
			{
				CH_DestroyWindow(ch_this);
#ifdef USE_ARGB8888
				memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
				memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
				memory_deallocate(system_partition, FileInfo);


#endif
				if(gifhandle!=NULL)
				{
					mgif_DestroyHandle(gifhandle);
					gifhandle = NULL;
				}
				
				return -1;
			}
			else
			{
				memcpy(picdata,(U8*)ch_this->scrBuffer.dwHandle,size);
				*data = (int)picdata;
			}	
		
	
		}
		else
		{
			CH_DestroyWindow(ch_this);
#ifdef USE_ARGB8888
			memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
			memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
			memory_deallocate(system_partition, FileInfo);

#endif
			if(gifhandle!=NULL)
			{
				mgif_DestroyHandle(gifhandle);
				gifhandle = NULL;
			}

			
			return -1;
		}

		
		
		CH_DestroyWindow(ch_this);
		
#ifdef USE_ARGB8888
			memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
			memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
			memory_deallocate(system_partition, FileInfo);


#endif
		if(gifhandle!=NULL)
		{
			mgif_DestroyHandle(gifhandle);
			gifhandle = NULL;
		}

	
		return error;
	}
	else
	{
	/*解析图片错误，图片数据无效*/
	
		CH_DestroyWindow(ch_this);
		
#ifdef USE_ARGB8888
		memory_deallocate(CHSysPartition, FileInfo);
#elif USE_ARGB1555
		memory_deallocate(chsys_partition, FileInfo);
#elif USE_ARGB4444
		memory_deallocate(system_partition, FileInfo);

#endif
		
		return -1;
	}


	
}

#endif
