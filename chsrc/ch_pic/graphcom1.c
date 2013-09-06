#include "graphcom.h"
#include "..\main\initterm.h" 

/*已添加的窗口列表*/
U32 ch_AllWinodws[ch_MaxWindowsNum + 1] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; 

/*系统默认前景背景颜色*/
PCH_RGB_COLOUR pAutoBackgroundColor, pAutoCurrentColor;


#define USE_ARGB8888;
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
#else
	colorbyte = 2;
#endif

	/*初始化缓冲区*/
#ifdef ENABLE_CH_PRINT
	CH_Printf("create windows: %d, %d\n", iWindowWidth, iWindowHigh);
#endif
	ch_this = (PCH_WINDOW)memory_allocate(appl_partition,sizeof(CH_WINDOW));
	if(ch_this == NULL)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("!!!ERROR graphcom :  Not enough memory, CH_CreateWindow return NULL.\n");
#endif
		return NULL;
	}
	
	ch_this->scrBuffer.dwHandle = 0;
	
	ch_this->scrBuffer.dwHandle = (U32)memory_allocate(appl_partition,iWindowWidth* iWindowHigh* colorbyte);
	ch_this->ColorByte = colorbyte;
	if(ch_this->scrBuffer.dwHandle == 0)
	{
#ifdef ENABLE_CH_PRINT	
		CH_Printf("!!!ERROR graphcom :  Not enough memory, CH_CreateWindow return NULL.\n");
#endif
		memory_deallocate(appl_partition,ch_this);
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
	S8* buf;
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
			buf = (S8*)ptr1->dwHandle;
			memory_deallocate(appl_partition,buf);
			ptr1->dwHandle = 0;
		}
		ptr1 = ptr1->pNext;
	}
	/*释放缓冲区链表*/
	ptr1 = ch_this->scrBuffer.pNext;
	while(ptr1)
	{
		ptr2 = ptr1->pNext;
		memory_deallocate(appl_partition,ptr1);
		ptr1 = ptr2;
	}
	
	
	/*释放窗口*/
	memory_deallocate(appl_partition,ch_this);
	
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
#ifdef USE_ARGB1555			
				if((*((U8*)sbuf + 1)) & 0x80)
#else
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
