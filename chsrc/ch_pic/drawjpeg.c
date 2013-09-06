#include <stdio.h>
#include "graphcom.h"
/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#include "jpeglib.h"

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>

#ifdef CH_Printf
#undef CH_Printf
#endif

#if BR_DRAWJPEG_C
#define CH_Printf(x) sttbx_Print x
#else
#define CH_Printf(x) {}
#endif

#define chz_debug_timeused(bol, str) NULL

struct my__error__mgr {
  struct jpeg__error__mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my__error__mgr * my__error__ptr;
S8* CH_ScaleMyImage(char *imgBuf,PCH_RECT oldsize, PCH_RECT newsize, int colorbits);
/*
 * Here's the routine that will replace the standard error__exit method:
 */

METHODDEF(void)
my__error__exit (j__common__ptr cinfo)
{
  /* cinfo->err really points to a my__error__mgr struct, so coerce pointer */
  my__error__ptr myerr = (my__error__ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output__message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_GETJPEGSIZE
    Purpose: 获得jpeg图片的实际大小
    Reference: 
    in:
        FileName >> 图片数据资源的objid
    retval:
        success >> 返回0
        fail >> 返回其他
   
notice : 
    sqZow 2007/11/09 09:11:53 
---------------------------------函数体定义----------------------------------------*/
PCH_RECT CH_GetJpegSize(S8* FileName,int FileSize)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg__decompress__struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
	struct my__error__mgr jerr;
	/* More stuff */
	graph_file * infile;		/* source file */
	PCH_RGB_COLOUR pRgbColor = NULL;
	PCH_RECT JpegSize = NULL;
 
  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = graph_open((char*)FileName, (char*)"rb",FileSize)) == NULL)
  {
    CH_Printf(( "can't open %s\n", FileName));
    return 0;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error__exit. */
  cinfo.err = jpeg__std__error(&jerr.pub);
  jerr.pub.error__exit = my__error__exit;
  /* Establish the setjmp return context for my__error__exit to use. */
  if (setjmp(jerr.setjmp_buffer))
  {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg__destroy__decompress(&cinfo);
    graph_close(infile);
    return NULL;
  }
  /* Now we can initialize the JPEG decompression object. */
  jpeg__create__decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg__stdio__src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg__read__header() */

  (void) jpeg__read__header(&cinfo, TRUE);
  if((JpegSize = (PCH_RECT)CH_AllocMem(sizeof(CH_RECT))) != NULL)
  {
	JpegSize->iStartX = 0;
	JpegSize->iStartY = 0;
	JpegSize->iHeigh = cinfo.image__height;
	JpegSize->iWidth = cinfo.image__width;
  }
   jpeg__destroy__decompress(&cinfo);
   graph_close(infile);
  return JpegSize;
}



/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_SHOWMYJPEG
    Purpose: 这个函数作为显示jpeg的实例。 
    Reference: 
    in:
        CH_this >> 窗口句柄， 类型可以根据需要重新定义， 比如先绘制在内存中， 或者直接显示到屏幕上，等等
        imgBuf >> 图像数据指针
        imgLen >> 图像数据长度
        RectSize >> 显示区域
        repeat >> 平铺属性
    retval:
        success >> 返回0
        fail >> 返回其他
   
notice : 
    sqZow 2007/11/28 10:29:49 
---------------------------------函数体定义----------------------------------------*/
CH_STATUS ch_showMyjpeg(char* imgBuf, unsigned long imgLen, PCH_WINDOW CH_this,  PCH_RECT RectSize, id_backgroundRepeat repeat)
{
	return 
		CH_DrawJpeg(imgBuf,  CH_this,   RectSize,  repeat, imgLen);
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_DRAWJPEG
    Purpose: 解析图片并将结果输出到窗口CH_this
    Reference: 
    in:
        CH_this >> 输出窗口
        FileName >> 图片数据的objid
        RectSize >> 目标位置
        repeat >> 是否翻转、平铺等
    retval:
        success >> 返回0
        fail >> 返回其他
   
notice : 
    sqZow 2007/11/09 09:09:01 
---------------------------------函数体定义----------------------------------------*/
CH_STATUS CH_DrawJpeg(S8* FileName, PCH_WINDOW CH_this,  PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len)
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
 

  struct jpeg__decompress__struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct my__error__mgr jerr;
  /* More stuff */
  graph_file * infile;		/* source file */
  JSAMPARRAY buf;		/* Output row buffer */
  JSAMPROW ScreenBuffer = NULL,buffer;
  int row__stride, widthlen;		/* physical row width in output buffer */
  S16 i,  j, w, h, w__limit, h__limit;
  JSAMPARRAY colormap ;
 PCH_RGB_COLOUR pRgbColor = NULL;
 CH_RECT jpegsize;
 U8 *t__buffer;
 
#ifdef USE_ARGB8888
U32 * t__buf;
#else
 S16 * t__buf;
#endif
  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  /* check__memStatus("start jpeg");*/
    do_report(0, "CH_DrawJpeg parse start\n");
    CH_Printf(("start draw jpeg\n"));
  if ((infile = graph_open((char*)FileName, /*(char*)"rb"*/NULL,ri_len)) == NULL)
  {
    CH_Printf(( "can't open %s\n", FileName));
    return CH_FAILURE;
  }
   #if 0 /*打印到文件*/
   {
   	FILE* fp;
   	fp = fopen("c:\\jpeg.jpg", "wb+");
   	if(fp)
   	{
   		fwrite(((z__linkedobj__struct*)infile)->linked__obj__addr, 1,  ((z__linkedobj__struct*)infile)->obj__size, fp);
   		fclose(fp);
   	}
   }
   #endif


  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error__exit. */
  cinfo.err = jpeg__std__error(&jerr.pub);
  jerr.pub.error__exit = my__error__exit;
  /* Establish the setjmp return context for my__error__exit to use. */
  if (setjmp(jerr.setjmp_buffer))
  {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg__destroy__decompress(&cinfo);
    graph_close(infile);
    return CH_FAILURE;
  }
  do_report(0, "CH_DrawJpeg decompress create\n"); 
  /* Now we can initialize the JPEG decompression object. */
  jpeg__create__decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg__stdio__src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg__read__header() */

  (void) jpeg__read__header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg__read__header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg__read__header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */
  do_report(0, "CH_DrawJpeg decompress start\n"); 	
  (void) jpeg__start__decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg__start__decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  row__stride = cinfo.output__width * cinfo.output__components;
  /* Make a one-row-high sample array that will go away when done with image */
/*  buffer = (*cinfo.mem->alloc__sarray)
		((j__common__ptr) &cinfo, JPOOL__IMAGE, row__stride, 1);*/

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg__read__scanlines(...); */

  /* Here we use the library's state variable cinfo.output__scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
 
  ScreenBuffer = (JSAMPROW)CH_AllocMem( cinfo.output__width  * cinfo.jpeg__color__space * sizeof(JSAMPROW));
  if(!ScreenBuffer)
  	return CH_FAILURE;
  buf = &ScreenBuffer;
  buffer = ScreenBuffer;
  jpegsize.iWidth = cinfo.output__width;
  jpegsize.iHeigh = cinfo.output__height;
  jpegsize.iStartX = RectSize->iStartX;
  jpegsize.iStartY = RectSize->iStartY;
   pRgbColor = (PCH_RGB_COLOUR)CH_AllocMem(sizeof(CH_RGB_COLOUR));
   colormap = cinfo.colormap;
   widthlen = CH_this->ColorByte * CH_this->SingleScrWidth;
   t__buffer = (U8*)(CH_this->scrBuffer.dwHandle 		\
   	+ widthlen * RectSize->iStartY 		\
   	+ RectSize->iStartX * CH_this->ColorByte);

   do_report(0, "CH_DrawJpeg parse end and draw start\n");
   CH_Printf(("sizeof-buffer[0] = %d\n", sizeof(buffer[0])));
   w = CH_MIN(cinfo.output__width, RectSize->iWidth);
   h = CH_MIN(cinfo.output__height, RectSize->iHeigh);
  while (cinfo.output__scanline < h) 
  {
    /* jpeg__read__scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
   /*  chz_debug_timeused("line 0");*/
	/*chz_debug_timeused(0, "scanlines start");*/
    (void) jpeg__read__scanlines(&cinfo,buf, 1);
    /* Assume put__scanline__someplace wants a pointer and sample count. */
    /*put__scanline__someplace(buffer[0], row__stride);*/
	buffer = *buf;
	
#ifdef USE_ARGB8888
   t__buf = (U32*)t__buffer;
#else
     t__buf = (S16*)t__buffer;
#endif
    /* chz_debug_timeused(0, "scanlines end");*/
    if(cinfo.colormap)
    {
    	for(j = 0 ; j < w; j++)
	{
#ifdef USE_ARGB8888
		//CHZ_DrawColorPoint(colormap[0][buffer[0]], colormap[1][buffer[0]], colormap[2][buffer[0]], &t__buf[j]);
	
			t__buf[j] = 0xFF000000|((colormap[0][buffer[0]]<<16)&0x00FF0000 )|( ( colormap[1][buffer[0]]<<8)&0x0000FF00)|(colormap[2][buffer[0]]&0x000000FF);
		
#else
		CHZ_DrawColorPoint(colormap[0][buffer[0]], colormap[1][buffer[0]], colormap[2][buffer[0]], &t__buf[j]);
#endif
		buffer += cinfo.jpeg__color__space;
		/*t__buf += CH_this->ColorByte;*/

	}
		
		t__buffer += widthlen;
    }
    else
    {
    	for(j = 0 ; j < w; j++)
	{
#ifdef USE_ARGB8888
		//CHZ_DrawColorPoint(buffer[0], buffer[1], buffer[2], &t__buf[j]);

		
			t__buf[j] = 0xFF000000|((buffer[0]<<16)&0x00FF0000 )|( (buffer[1]<<8)&0x0000FF00)|( buffer[2]&0x000000FF);
	
#else
		CHZ_DrawColorPoint(buffer[0], buffer[1], buffer[2], &t__buf[j]);
#endif
		buffer += cinfo.jpeg__color__space;
		/*t__buf += CH_this->ColorByte;*/
	}
    }
	t__buffer += widthlen;
	/* chz_debug_timeused("line end");*/
  }

   do_report(0, "CH_DrawJpeg draw ok\n");
    (void) jpeg__finish__decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

   
  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg__destroy__decompress(&cinfo);

  /* After finish__decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg__destroy can do an error exit, but why assume anything...)
   */
  graph_close(infile);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num__warnings is nonzero).
   */
   

   
  
 /*CH_RepeatImageInArea(CH_this, &jpegsize, RectSize, repeat);*/
  /* Step 7: Finish decompression */
  /* And we're done! */
 CH_FreeMem(pRgbColor);
 CH_FreeMem(ScreenBuffer);

 /*check__memStatus("exit jpeg");*/
 return CH_SUCCESS;
}


S8* CH_ScaleMyImage(char *imgBuf,PCH_RECT oldsize, PCH_RECT newsize, int colorbits)
{
    int i,j, k;    
    int sx,sy,ex,ey;
    unsigned char *newd,*p;
    unsigned char *l1;
	
   if(colorbits < 1 || colorbits > 4)
   {
   	colorbits = 1;
   }
    newd=(unsigned char *)CH_AllocMem(newsize->iHeigh*newsize->iWidth*colorbits);
    if(!newd) 
    {
        CH_Printf(("memory allocation error in ScaleImage\n"));
	 return NULL;
    }
    p=newd;
    sy=0;
    for(i=0;i<newsize->iHeigh;i++)
    {
        ey=(i+1)*oldsize->iHeigh/newsize->iHeigh;
        sx=0;
        l1=(unsigned char*)((unsigned int)imgBuf+sy*oldsize->iWidth*colorbits);
        for(j=0;j<newsize->iWidth;j++)
	{
		ex=(j+1)*oldsize->iWidth/newsize->iWidth*colorbits;
		for(k = 0 ; k < colorbits; k++, p++)
		{
		  *p = l1[sx + k];
		}
		sx=ex;
	}
	sy=ey;
   }

    CH_FreeMem(imgBuf);
    return (S8*)newd;
 }

CH_STATUS CH_CopySingleRowBitmap(PCH_WINDOW CH_this, S16 src__x, S16 src__y, S16 dest__x, S16 dest__y, S16 width)
{
	static S16 src__n = -1, dest__n = -1;
	static PCH_WINDOW sCH_this = NULL;
	static PCH_WINDOW_BUF src__winbuf = NULL, dest__winbuf = NULL;
	S16 i;
	U32 src__excursion, dest__excursion;
	
	if(src__y / CH_this->SingleScrHeight != src__n || sCH_this != CH_this)
	{
		src__n = src__y / CH_this->SingleScrHeight;
		for(i = 0, src__winbuf = &(CH_this->scrBuffer); i < src__n; i ++)
		{
			src__winbuf = src__winbuf->pNext;
		}
		sCH_this = CH_this;
	}
	
	if(dest__y / CH_this->SingleScrHeight != dest__n || sCH_this != CH_this)
	{
		dest__n = dest__y / CH_this->SingleScrHeight;
		for(i = 0, dest__winbuf = &(CH_this->scrBuffer); i < dest__n; i ++)
		{
			dest__winbuf = dest__winbuf->pNext;
		}
		sCH_this = CH_this;
	}
	if(dest__winbuf == NULL)
	{
		for(dest__winbuf = &(CH_this->scrBuffer); 		\
			dest__winbuf && dest__winbuf->pNext; 		\
			dest__winbuf = dest__winbuf->pNext);
		dest__winbuf->pNext = (PCH_WINDOW_BUF)CH_AllocMem(sizeof(CH_WINDOW_BUF));
		dest__winbuf = dest__winbuf->pNext;
		dest__winbuf->dwHandle = 	\
			(U32)CH_AllocMem(CH_this->SingleScrHeight * CH_this->SingleScrWidth * CH_this->ColorByte);
		dest__winbuf->pNext = NULL;
		CH_this->Screen.iHeigh += CH_this->SingleScrHeight;
		if(dest__winbuf->dwHandle == 0)
		{
			CH_Printf(("drawjpeg.c @ alloc memory fail\n"));
			return CH_ALLOCMEM_ERROR;
		}
		CH_MemSet((void *) dest__winbuf->dwHandle, 	\
			0xff, 	\
			CH_this->SingleScrHeight * CH_this->SingleScrWidth * CH_this->ColorByte);
	}
	dest__excursion = ((dest__y % CH_this->SingleScrHeight) * CH_this->SingleScrWidth + dest__x) 		\
				* CH_this->ColorByte;
	src__excursion = ((src__y % CH_this->SingleScrHeight) * CH_this->SingleScrWidth + src__x) 		\
				* CH_this->ColorByte;
	/*CH_DebugPut("[%d, %d, %d, %d],",src__x, src__y, dest__x, dest__y);*/
	CH_MemCopy((S8*)(dest__winbuf->dwHandle + dest__excursion), 		\
				(S8*)(src__winbuf->dwHandle + src__excursion), 		\
				width * CH_this->ColorByte);
	return CH_SUCCESS;
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_REPEATIMAGEINAREA
    Purpose: 根据需要平铺图片
    Reference: 
    in:
        CH_this >> 
        pDestRect >> 
        pImageRect >> 
        repeat >> 
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2007/11/28 10:32:26 
---------------------------------函数体定义----------------------------------------*/
CH_STATUS CH_RepeatImageInArea(PCH_WINDOW CH_this,  PCH_RECT pImageRect, PCH_RECT pDestRect,  id_backgroundRepeat repeat)
{
	S16 i, j, h, w, n, nx, ny;
	S8* pdata1, *pdata2;
	CH_RECT rect;
	
	switch(repeat)
	{
		case id_backgroundRepeat(room):
		case id_backgroundRepeat(no_repeat):
			return CH_SUCCESS;
			
		case id_backgroundRepeat(repeat):
			CH_RepeatImageInArea(CH_this,  pImageRect, pDestRect,  id_backgroundRepeat(repeat_x));
			CH_MemCopy(&rect, pImageRect, sizeof(CH_RECT));
			rect.iWidth = pDestRect->iWidth;
			CH_RepeatImageInArea(CH_this,  &rect, pDestRect,  id_backgroundRepeat(repeat_y));
			break;
			
		case id_backgroundRepeat(repeat_x):

			if(pDestRect->iWidth <= pImageRect->iWidth)
				break;
			if(pDestRect->iWidth + pDestRect->iStartX > CH_this->SingleScrWidth)
				pDestRect->iWidth = CH_this->SingleScrWidth - pDestRect->iStartX;
			if(pDestRect->iStartX == pImageRect->iStartX 		\
				&& pDestRect->iStartY== pImageRect->iStartY)
			{
				nx = pDestRect->iStartX + pImageRect->iWidth;
				n = pDestRect->iWidth / pImageRect->iWidth + 1;
			}
			else
			{
				nx = pDestRect->iStartX;
				n = pDestRect->iWidth / pImageRect->iWidth;
			}
			h = CH_MIN(pImageRect->iHeigh, pDestRect->iHeigh);
			
			
			w = pImageRect->iWidth;
			for(i = 0; i < n; i ++)
			{
				if(nx + w > pDestRect->iStartX + pDestRect->iWidth)
					w = pDestRect->iStartX + pDestRect->iWidth - nx;
				if(nx + w > CH_this->Screen.iStartX + CH_this->SingleScrWidth)
					w = CH_this->Screen.iStartX + CH_this->SingleScrWidth - nx;
				if(w < 1)
					break;
				for(j = 0; j < h; j ++)
				{
					CH_CopySingleRowBitmap(CH_this,pImageRect->iStartX, pImageRect->iStartY + j, nx, pDestRect->iStartY + j, w);
				}
				nx += pImageRect->iWidth;
				
			}
			break;
			
		case id_backgroundRepeat(repeat_y):
			if(pDestRect->iHeigh <= pImageRect->iHeigh)
				break;
			if(pDestRect->iStartX == pImageRect->iStartX 		\
				&& pDestRect->iStartY== pImageRect->iStartY)
			{
				n = pDestRect->iHeigh / pImageRect->iHeigh + 1;
				ny = pDestRect->iStartY + pImageRect->iHeigh;
			}
			else
			{
				n = pDestRect->iHeigh / pImageRect->iHeigh;
				ny = pDestRect->iStartY;
			}
			w = CH_MIN(pImageRect->iWidth, pDestRect->iWidth);
			h = pImageRect->iHeigh;
			for(i = 0; i < n; i ++)
			{
				
				if(ny + h >= pDestRect->iStartY + pDestRect->iHeigh)
					h = pDestRect->iStartY + pDestRect->iHeigh - ny;
				if(h < 1)
					break;
				for(j = 0; j < h; j ++)
				{
					CH_CopySingleRowBitmap(CH_this,pImageRect->iStartX, pImageRect->iStartY + j, pDestRect->iStartX, ny + j, w);
				}
				ny += pImageRect->iHeigh;
			}
			break;
			
		default:
			break;
	}
	return CH_SUCCESS;
}



