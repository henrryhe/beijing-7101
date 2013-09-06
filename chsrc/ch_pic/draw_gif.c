/*{{{    COMMENT Standard C */
/************************************************************************
COPYRIGHT (C) CHANG HONG 2001

  Source file name : gif.c
  Author(s) : ZXG
  
	
	  Original Work: 
	  
		Date       Initials	Modification                                    
		----       --------	------------ 
		20011214   ChenHeng Modified
************************************************************************/
/*}}}  */

/*{{{ Include files*/
#include "stddefs.h"
#include "draw_gif.h"
#ifdef USE_ARGB8888
#include "../main/initterm.h"
#elif USE_ARGB1555	
#include "..\main\initterm.h"
#elif USE_ARGB4444
#include "..\main\initterm.h"
#endif

/*00_GIF*/
#include <string.h> 
#include "stdlib.h"
#include "stdio.h"
#include "stddefs.h"
#include "graphcom.h"
#include "graphconfig.h"
#include "ch_CQInfoService.h"




#if 1
/*#define WINDOWS_DEMO*/
#else  /*00_GIF*/
#define WINDOWS_DEMO
#endif

#ifdef WINDOWS_DEMO
#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif


typedef enum			/* defination of cursor type */
{
    J_OSD_CURSOR_HAND,	/* Hand cursor type */
    J_OSD_CURSOR_WAIT,	/* Wait cursor type */
    J_OSD_CURSOR_EDIT,	/* Edit cursor type */
    J_OSD_CURSOR_ARROW	/* Arrow cursor type */
} E_OSD_CursorType;

typedef struct			/* defination of rectangle structure */
{
	int nX;
	int nY;
	int nWidth;
	int nHeight;
} T_OSD_Rect;

typedef struct			/* defination of RGB structure */
{
    U8 cRed;			/* red */
    U8 cGreen;		/* green */
    U8 cBlue;		/* blue */
} T_OSD_ColorRGB;



#ifdef WINDOWS_DEMO
typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	U32;

typedef signed char	S8;
typedef signed short	S16;
typedef signed int		S32;
#endif

extern ST_Partition_t	*appl_partition;

FRAME 		*allFrames;
unsigned char *ifs;
char 		gif_version[4];
boolea 		errorone;
boolea 		opened;
boolea 		inMem;
U8 			gColorTable[256*3];
/*U8 			lColorTable[256*3];*/
long 		dataStart;
U32 			curIndex;
U8 			c1, c2;
U32 			pos;
U32 			max;

#if 0  /*00_GIF*/
//add to run on windows 
#else
/*add to run on windows*/ 
#endif
#ifdef WINDOWS_DEMO
extern HWND		g_Hwnd;
#endif

U8*			g_Buffer;
extern int		iFileLen;

#ifndef WINDOWS_DEMO
/* 16位色在windows上的调色板 */
unsigned short myx_16_pal[256];
/* 32位色在windows上的调色板 */

U32 myx_32_pal[256];

#endif

U32 checkFrames(unsigned char *ifs);
boolea getAllFrames(unsigned char *ifs);
boolea extractData(FRAME* f,unsigned char *ifs);
boolea initStrTable(STRING_TABLE_ENTRY* strTable,U32 rootSize);
boolea addStrTable(STRING_TABLE_ENTRY* strTable,U32 addIdx,U32 idx,unsigned char c);

GLOBAL_INFO 	gInfo;
FRAME 			curFrame;
GCTRLEXT 		ctrlExt;

void CGif89a(unsigned char *fileName,boolea inMem);

boolea openfile(unsigned char *fileName,boolea inMem);
void closefile(void);
LPCSTR getVer(void);
LPCFRAME getNextFrame( FRAME *curFrame );
LPCGLOBAL_INFO getGlobalInfo(void);
U8* gif(unsigned char *argv,int size);


#define	EXIT_KEY		0x3ac5  /*00_GIF*/


#ifdef CQ_BROWSER
#define mgif_free  CH_FreeMem	
#else
/*
 * Func: mgif_free
 * Desc: 释放空间
 */
void mgif_free ( void* rp_data )
{
	if(rp_data!=NULL)
	{
		memory_deallocate ( appl_partition, rp_data );
		rp_data=NULL;
	}
	return;
}
#endif




/*
  * Func: CGif89a
  * Desc: 打开文件并检错
  */
void CGif89a( unsigned char *fileName, boolea inMem )
{
	pos			= 0;

	opened 		= FALSE;
	errorone 	= FALSE;
	
	opened 		= FALSE;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"fileName=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",g_Buffer[0],g_Buffer[1],g_Buffer[2],g_Buffer[3],g_Buffer[4],g_Buffer[5],g_Buffer[6],g_Buffer[7],g_Buffer[8],g_Buffer[9],g_Buffer[10]);*/
	#endif
  
	if( openfile( fileName, inMem ) )
	{	
		opened 		= TRUE;
		errorone 	= FALSE;
	}
	else
	{
		opened 		= FALSE;
		errorone 	= TRUE;
	}

}

/*
  * Func: openfile
  * Desc: 打开文件，检错，并得到全局结构
  */
boolea openfile(unsigned char *fileName, boolea b )
{	
	char 	cc[4];
	U8 		be;
	boolea 	fileEnd = FALSE;
	int 			i;

	inMem 				= b;					/* 是否在内存内 */
	allFrames 			= NULL;
	curIndex 			= 0;
	curFrame.pColorTable 	= NULL;
	curFrame.dataBuf 	= NULL;
	ctrlExt.active 			= FALSE;

	if( opened )
		return FALSE;
	ifs	= (unsigned char*)(&g_Buffer[(int)fileName]);
	if( !ifs )
		return FALSE;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"g_Buffer=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",g_Buffer[0],g_Buffer[1],g_Buffer[2],g_Buffer[3],g_Buffer[4],g_Buffer[5],g_Buffer[6],g_Buffer[7],g_Buffer[8],g_Buffer[9],g_Buffer[10]);
	do_report(0,"ifs=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",ifs[0],ifs[1],ifs[2],ifs[3],ifs[4],ifs[5],ifs[6],ifs[7],ifs[8],ifs[9],ifs[10]);*/
	#endif

#if 0	/* jqz060802 rmk */
#ifdef YY_GIF_SHOW
	{
		int i;
		 for(i=0;i<10000;i++)
	      	{
		if((strncmp((char*)fileName[3*i],"G",1)==0)&&(strncmp((char*)fileName[3*i+1],"I",1)==0)&&(strncmp((char*)fileName[3*i+2],"F",1)==0))
			{
			pos=i*3;
			break;
			}
	      	}
	} 
#endif
#endif

	/* 得到幻数 */
	cc[0]=ifs[pos];
	pos++;
	cc[1]=ifs[pos];
	pos++;
	cc[2]=ifs[pos];
	pos++;
	if(strncmp(cc,"GIF",3) != 0)
		goto errorone;

	/* 得到版本号 */
	gif_version[0]=ifs[pos];
	pos++;
	gif_version[1]=ifs[pos];
	pos++;
	gif_version[2]=ifs[pos];
	pos++;
	gif_version[3] = 0;
	/*
	if(strncmp(version,"89a",3) != 0)
		goto errorone;*/
	/* 得到宽高 */
	c1	= ifs[pos];
	pos ++;
	c2	= ifs[pos];
	pos ++;
	gInfo.scrWidth	= ICHG( c1, c2 );
	c1	= ifs[pos];
	pos ++;
	c2	= ifs[pos];
	pos ++;
	gInfo.scrHeight	= ICHG( c1, c2 );

	be	= ifs[pos];
	pos++;

	/* 全局色表标志 */
	if( ( be & 0x80 ) != 0 )
		gInfo.gFlag = TRUE;
	else
		gInfo.gFlag = FALSE;

	/* 颜色深度 */
	gInfo.colorRes = ( ( be & 0x70 ) >> 4 ) + 1;

	/* 如果有全局色表 */
	if( gInfo.gFlag )
	{	
		/* 颜色色表分类排列标志 */
		if( ( be & 0x08 ) != 0 )
			gInfo.gSort = TRUE;
		else
			gInfo.gSort = FALSE;

		/* 色表位数 */
		gInfo.gSize = 1;
		gInfo.gSize <<= ((be&0x07)+1);
	}

	/* 背景色 */
	be	= ifs[pos];
	pos++;
	gInfo.BKColorIdx = be;

	/* 宽高比 */
	be	= ifs[pos];
	pos++;
	gInfo.pixelAspectRatio = be;

	/* 装入全局色表 */
	if( gInfo.gFlag )
	{	
		for( i = 0; i < gInfo.gSize * 3; i++ )
		{
			gColorTable[i] = ifs[pos];
			pos++;
		}
		
		gInfo.gColorTable = gColorTable;
	}
	else
		gInfo.gColorTable = NULL;

	/* 得到数据实际起始地址 */
	dataStart = pos;

	/* 得到一共有多少视图帧 */
	if( ( gInfo.frames = checkFrames( ifs ) ) == 0 )
		goto errorone;

	/* 如果是内存中的数据 */
	if( inMem )
	{
		/* 分配帧数组空间 */
		if( ( allFrames = (FRAME*)mgif_malloc( sizeof(FRAME) * gInfo.frames ) ) == NULL )
			goto errorone;

		/* 得到所有的帧 */
		if( !getAllFrames( ifs ) )
		{
			mgif_free( allFrames );
			allFrames = NULL;
			goto errorone;
		}

	}

	opened = TRUE;
	return TRUE;

errorone:

	return FALSE;
}

/*
  * Func: checkFrames
  * Desc: 检查该图片中一共含有多少视图帧
  */
U32 checkFrames( unsigned char* ifs )
{	
	U8 		be;
	boolea 	fileEnd 	= FALSE;
	U32 		frames	= 0;
	long 	pos1 	= pos;

	while( pos <= max && !fileEnd )
	{	
		/* 得到数据块首标记 */
		be	= ifs[pos];
		pos	++;

		/* 按标记进行分类 */
		switch( be )
		{	
		case 0x21:						/* 扩展标记 */	
			be = ifs[pos];
			pos ++;
			switch( be )
			{	
			case 0xf9:					/* 注释标签块，固定为0xfe */
			case 0xfe:
			case 0x01:
			case 0xff:
				while( pos <= max )		/* 得到块长度 */
				{	
					be = ifs[pos];
					pos ++;
					if( be == 0 )
						break;
					
					pos += be;
				}
				break;
			default:						/* 没找到的块标识，返回视图个数为0 */
				return 0;
			}
			break;
		case 0x2c:						/* 视图数据标识符 */
			{
				U8 		bp;
				boolea 	lFlag 	= FALSE;
				U32 		lSize 	= 1;

				/* 视图个数增加 */
				frames ++;

				/* 跳过X,Y偏移量，及图像宽高度 */
				pos += 8;
				
				bp	= ifs[pos];
				pos ++;

				/* 是否要使用局部色板 */
				if( ( bp & 0x80 )  != 0 )
					lFlag = TRUE;

				/* 局部色板大小 */
				lSize <<= ( ( bp & 0x07 ) + 1 );

				if( lFlag )
					pos += lSize * 3;
								
				be = ifs[pos];
				pos++;

				/* 现在达到图像数据块 */
				while( pos < max )
				{	
					/* 得到LZW的编码长度 */
					be = ifs[pos];
					pos ++;
					if( be == 0 )
						break;

					/* 跳过改数据块 */
					pos += be;
				}
				break;
			}
		case 0x3b:						/* 文件终结器 */
			fileEnd = TRUE;
			break;
		case 0x00:
			break;
		default:							/* 其他未知标识符 */
			return 0;
		}
	}
	
	pos	= pos1;
	
	return frames;
}

/*
  * Func: getAllFrames
  * Desc: 得到所有的帧数据
  */
boolea getAllFrames( unsigned char* ifs )
{	
	U8 		be;
	boolea 	fileEnd 	= FALSE;
	FRAME 	*pf 		= allFrames;
	long 	pos1 	= pos;
	int 		i;

	/* 循环文件所有的部分 */
	while( ( pos <= max ) && !fileEnd )
	{
		be	= ifs[pos];
		pos ++;
		
		switch(be)
		{	
		case 0x21:						/* 扩展标记块 */		

			be	= ifs[pos];				/* 得到注释块标签 */
			pos ++;
			
			switch( be )					/* 根据标签块进行处理 */
			{
			case 0xf9:					/* 图形控制扩展块 */
				while( pos <= max )
				{
					/* 得到块大小(固定为4) */
					be = ifs[pos];
					pos ++;

					if( be == 0 )
						break;
					
					if( be == 4 )
					{
						/* 图形控制有效 */
						ctrlExt.active = TRUE;

						be	= ifs[pos];
						pos++;

						/* 处置方式 */
						ctrlExt.disposalMethod = ( be & 0x1c ) >> 2;

						/* 用户输入标记 */
						if( ( be & 0x02 ) != 0 )
							ctrlExt.userInputFlag = TRUE;
						else
							ctrlExt.userInputFlag = FALSE;

						/* 是否使用透明色 */
						if( ( be & 0x01 ) != 0 )
							ctrlExt.trsFlag = TRUE;
						else
							ctrlExt.trsFlag = FALSE;

						/* 得到该帧图的延迟时间 */
						c1	= ifs[pos];
						pos ++;
						c2	= ifs[pos];
						pos ++;
						
						ctrlExt.delayTime = ICHG( c1, c2 );

						/* 透明色索引 */
						be	= ifs[pos];
						pos ++;

						ctrlExt.trsColorIndex = be;
					}
					else					
						pos += be;
				}
				break;
			case 0xfe:	/* 其他扩展标记块都跳过 */
			case 0x01:
			case 0xff:
				while( pos <= max )
				{	
					be = ifs[pos];
					pos ++;
					if( be == 0 )
						break;					
					pos += be;
				}
				break;
			default:
				goto errorone;
			}
			break;
		case 0x2c:
			{
				U8 bp;

				/* 视图X偏移 */
				c1	= ifs[pos];
				pos ++;
				c2	= ifs[pos];
				pos ++;
				pf->imageLPos	= ICHG( c1, c2 );

				/* 视图Y偏移 */
				c1	= ifs[pos];
				pos ++;
				c2	= ifs[pos];
				pos ++;
				pf->imageTPos	= ICHG( c1, c2 );

				/* 视图宽度 */
				c1	 = ifs[pos];
				pos ++;
				c2	= ifs[pos];
				pos ++;
				pf->imageWidth	= ICHG( c1, c2 );

				/* 视图高度 */
				c1	= ifs[pos];
				pos++;
				c2	= ifs[pos];
				pos++;
				pf->imageHeight	= ICHG( c1, c2 );

				
				bp	= ifs[pos];
				pos++;

				/* 是否使用局部色表 */
				if( ( bp & 0x80 ) != 0 )
					pf->lFlag = TRUE;
				/* 交织标志 */
				if( ( bp & 0x40 ) != 0 )
					pf->interlaceFlag = TRUE;
				/* 局部色表是否要按颜色排序 */
				if( ( bp & 0x20 ) != 0 )
					pf->sortFlag = TRUE;

				/* 得到局部数据库大小 */
				pf->lSize = 1;
				pf->lSize <<= ( ( bp & 0x07 ) + 1 );
				if( pf->lFlag )
				{	
					/* 分配局部数据库，并对其赋值 */
					if( ( pf->pColorTable = ( U8* )mgif_malloc( sizeof(U8) * pf->lSize * 3 ) ) == NULL )
						goto errorone;
					
					for( i = 0; i < pf->lSize * 3; i++ )
					{
						pf->pColorTable[i] = ifs[pos];
						pos ++;
					}
				}

				/* 对LZW数据进行解码 */
				if( !extractData( pf, ifs ) )
					goto errorone;

				/* 如果图形控制块有效 */
				if( ctrlExt.active )
				{
					/* 给图形控制块赋值 */
					pf->ctrlExt 	= ctrlExt;
					ctrlExt.active 	= FALSE;
				}

				pf ++;
				break;
			}
		case 0x3b:
			fileEnd = TRUE;
			break;

		case 0x00:
			break;

		default:
			goto errorone;
		}
	}
	
	pos	= pos1;
	return TRUE;

errorone:

	/* 错误处理 */
	pf = allFrames;

	/* 释放所有已分配的空间 */
	for( i = 0; i < (int)gInfo.frames; i++ )
	{
		if( pf->pColorTable != NULL )
		{	
			mgif_free( pf->pColorTable );
			pf->pColorTable = NULL;
		}
		if( pf->dataBuf != NULL )
		{
			mgif_free( pf->dataBuf );
			pf->dataBuf = NULL;
		}
	}

	return FALSE;
}

LPCFRAME getNextFrame( FRAME *curFrame )
{ 
	int    			i, j, Row, Col, Width, Height, i_SrcLine;
	unsigned char 	*p_Swap = NULL;
	int    			InterlacedOffset[4] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
	int    			InterlacedJumps[4] = { 8, 8, 4, 2 };   /* be read - offsets and jumps... */

	if(inMem)
	{	
		FRAME* p =  allFrames+curIndex;
		curIndex++;
		if(curIndex >= gInfo.frames)
			curIndex = 0;
		return p;
	}
	else
	{	
		U8 be;
		boolea fileEnd = FALSE;
		if(curFrame->pColorTable != NULL)
		{
			mgif_free(curFrame->pColorTable);
			curFrame->pColorTable = NULL;
		}
		if(curFrame->dataBuf != NULL)
		{	
			mgif_free(curFrame->dataBuf);
			curFrame->dataBuf = NULL;
		}

		curFrame->ctrlExt.active=0;
		curFrame->ctrlExt.delayTime=0;
		curFrame->ctrlExt.disposalMethod=0;
		curFrame->ctrlExt.trsColorIndex=0;
		curFrame->ctrlExt.trsFlag=0;
		curFrame->ctrlExt.userInputFlag=0;
		curFrame->dataBuf=0;
		curFrame->imageHeight=0;
		curFrame->imageLPos=0;
		curFrame->imageTPos=0;
		curFrame->imageWidth=0;
		curFrame->interlaceFlag=0;
		curFrame->lFlag=0;
		curFrame->lSize=0;
		curFrame->pColorTable=0;
		curFrame->sortFlag=0;
		while(TRUE)
		{	
			be=ifs[pos];
			pos++;
			switch(be)
			{	case 0x21:
			
			be=ifs[pos];
			pos++;
			switch(be)
			{	case 0xf9:
			while(pos<=max)
			{	
				be=ifs[pos];/*Extension Introducer*/
				pos++;
				if(be == 0)
					break;
				if(be == 4)/*blocksize*/
				{	ctrlExt.active = TRUE;
				
				be=ifs[pos];
				pos++;
				ctrlExt.disposalMethod = (be&0x1c)>>2;
				if((be&0x02) != 0)
					ctrlExt.userInputFlag = TRUE;
				else
					ctrlExt.userInputFlag = FALSE;
				if((be&0x01) != 0)
					ctrlExt.trsFlag = TRUE;
				else
					ctrlExt.trsFlag = FALSE;
				
				c1=ifs[pos];
				pos++;
				c2=ifs[pos];
				pos++;
				ctrlExt.delayTime=ICHG(c1,c2);
				
				
				be=ifs[pos];
				pos++;
				ctrlExt.trsColorIndex = be;
				}
				else
					pos+=be;
			}
			break;
			case 0xfe:
			case 0x01:
			case 0xff:
				while(pos<=max)
				{	
					be=ifs[pos];
					pos++;
					
					if(be == 0)
						break;
					
					pos+=be;
				}
				break;
			default:
				
				goto errorone;
				
			}
			break;
			case 0x2c:
				{	U8 bp;
				
				
				c1=ifs[pos];
				pos++;
				c2=ifs[pos];
				pos++;
				curFrame->imageLPos=ICHG(c1,c2);
				
				c1=ifs[pos];
				pos++;
				c2=ifs[pos];
				pos++;
				curFrame->imageTPos=ICHG(c1,c2);
				
				c1=ifs[pos];
				pos++;
				c2=ifs[pos];
				pos++;
				curFrame->imageWidth=ICHG(c1,c2);
				
				c1=ifs[pos];
				pos++;
				c2=ifs[pos];
				pos++;
				curFrame->imageHeight=ICHG(c1,c2);
				
				
				bp=ifs[pos];
				pos++;
				
				if((bp&0x80) != 0)
					curFrame->lFlag = TRUE;
				if((bp&0x40) != 0)
					curFrame->interlaceFlag = TRUE;
				if((bp&0x20) != 0)
					curFrame->sortFlag = TRUE;
				curFrame->lSize = 1;
				curFrame->lSize <<= ((bp&0x07)+1);
				if((curFrame->pColorTable = (U8*)mgif_malloc( sizeof(U8)*curFrame->lSize*3)) == NULL)
				{
					goto errorone;
				}
				
				if(curFrame->lFlag)
					
					for(i=0;i<curFrame->lSize*3;i++)
					{
						curFrame->pColorTable[i]=ifs[pos];
					pos++;
					}

					if(!extractData(curFrame,ifs))
					{
						goto errorone;
					}

					/* 处理交织*/
					if ( true == curFrame->interlaceFlag )
					{
						Row  	= curFrame->imageTPos;  	/* GifFile->Image.Top;  Image Position relative to Screen. */
						Col  		= curFrame->imageLPos;  	/* GifFile->Image.Left; */
						Width	= curFrame->imageWidth;  	/* GifFile->Image.Width; */
						Height 	= curFrame->imageHeight;  	/* GifFile->Image.Height; */

						/* 分配交换行变量 */
						p_Swap = (unsigned char*)mgif_malloc ( Width * Height );

						/* 分配失败 */
						if ( !p_Swap )
						{
							return NULL;
						}

						memcpy ( p_Swap, curFrame->dataBuf, Width * Height );

						/* 交换 */
						for ( i_SrcLine = 0, i = 0; i < 4; i++)
						{
							for ( j = Row + InterlacedOffset[i]; j < Row + Height;
								j += InterlacedJumps[i] )
							{        
								memcpy ( curFrame->dataBuf + j * Width,
											p_Swap + i_SrcLine * Width,
											Width
											);
								i_SrcLine ++;        
							}
						}

						mgif_free ( p_Swap );
					}
					
					curFrame->ctrlExt = ctrlExt;
					if(ctrlExt.active == TRUE)
						ctrlExt.active = FALSE;
					
					return curFrame;
				}
			case 0x3b:
				
				pos=dataStart;
				break;
			case 0x00:
				break;
			default:
				
				goto errorone;
				
				
		}
		}
}
return curFrame;

errorone:
if(curFrame->pColorTable != NULL)
{	mgif_free(curFrame->pColorTable);
	curFrame->pColorTable = NULL;
}
if(curFrame->dataBuf != NULL)
{	mgif_free(curFrame->dataBuf);
	curFrame->dataBuf = NULL;
}

return NULL;

}

boolea initStrTable(STRING_TABLE_ENTRY* strTable,U32 rootSize)
{	
	U32 i;
	unsigned char *cc;
	for(i=0;i<rootSize;i++)
	{
		if((cc =(unsigned char*)mgif_malloc(sizeof(unsigned char)*2)) == NULL)
			goto errorone;
		cc[0] =(unsigned char)i,cc[1] = 0;
		strTable[i].p = cc;
		strTable[i].len = 1;
	}
	return TRUE;
errorone:
	for(i=0;i<rootSize;i++)
		if(strTable[i].p != NULL)
		{	 mgif_free(strTable[i].p);
		strTable[i].p = NULL;
		}
		return FALSE;
}

boolea addStrTable(STRING_TABLE_ENTRY* strTable,U32 addIdx,U32 idx,unsigned char c)
{	
	unsigned char *cc;
	U32 i;
	U32 l = strTable[idx].len;
	if(addIdx >= 4096 || strTable[idx].len > 0xffff)
		return FALSE;
	if((cc =(unsigned char*)mgif_malloc(sizeof(unsigned char)*(l+2))) == NULL)
		return FALSE;
	for(i=0;i<l;i++)
		cc[i] = strTable[idx].p[i];
	cc[l] = c;
	cc[l+1] = 0;
	strTable[addIdx].p = cc;
	strTable[addIdx].len = strTable[idx].len +1;
	return TRUE;
}

boolea extractData( FRAME* f, unsigned char *ifs )
{	
	STRING_TABLE_ENTRY *strTable;
	U32 					codeSize, rootSize, tableIndex, codeSizeBK;
	int 					remainInBuf = 0, i;
	U32 					bufIndex = 0, outIndex = 0;
	U32 					bitIndex = 0;
	unsigned long int 		code, oldCode;
	unsigned char 		temp[4];
	U32 					bufLen;
	U8 					be, *outP;
	U8 					buf[262];
	boolea 				readOK = FALSE;


	bufLen = f->imageWidth * f->imageHeight;

	/* 分配字典查找表 */

	if( ( strTable = (STRING_TABLE_ENTRY*)mgif_malloc( sizeof(STRING_TABLE_ENTRY) * 4096 ) ) == NULL )
	{
		return FALSE;  
	}

#if 0 /*sqzow 061214*/
	for( i = 0; i < 4096; i++ )
	{
		( strTable + i )->len 	= 0;
		( strTable + i )->p		= NULL;
	}
#else
	memset(strTable, 0, sizeof(STRING_TABLE_ENTRY) * 4096 );
#endif
	/* 分配目标数据指针 */

	outP	= f->dataBuf = /*(U8*)malloc(sizeof(U8)*bufLen);*/(U8*)mgif_malloc(sizeof(U8)*bufLen);

	/* 出错处理 */
	if( f->dataBuf == NULL )
	{	
		mgif_free( strTable );
		return FALSE;
	}
	
	/* 得到数据块的大小 */
	be	= ifs[pos];
	pos ++;
	
	codeSizeBK = codeSize = be + 1;
	rootSize = 1;
	rootSize <<= be;

	tableIndex = rootSize + 2;

	if( !initStrTable( strTable, rootSize ) )
	{
		do_report( 0, "init strtavle" );
		goto errorone;
	}
	
begin:
	if( remainInBuf <= 4 && !readOK )
	{	
		for( i = 0; i < remainInBuf; i++ )
			buf[i] = buf[bufIndex+i];

		bufIndex = 0;
		
		be = ifs[pos];
		pos++;
		if( be != 0 )
		{			
			for( i = remainInBuf; i < remainInBuf + be; i++ )
			{
				buf[i]	= ifs[pos];
				pos++;
			}
			remainInBuf += be;
		}
		else
			readOK = TRUE;
		
	}
	if(remainInBuf<=4)
		if(remainInBuf<=0 || codeSize > (remainInBuf*8-bitIndex))
			goto done;
		temp[0]= buf[bufIndex];
		temp[1]= buf[bufIndex+1];
		temp[2]= buf[bufIndex+2];
		temp[3]= buf[bufIndex+3];
		code = *((long int*)(temp));
		code <<= 32-codeSize-bitIndex;
		code >>= 32-codeSize;
		bitIndex += codeSize;
		bufIndex += bitIndex/8;
		remainInBuf -= bitIndex/8;
		bitIndex %= 8;
		if(code >= rootSize+1)
		{
			do_report(0,"not done1\n");
			goto errorone;}
		if(code == rootSize)
			goto begin;
		else
		{	outP[outIndex++] = *strTable[code].p;
		oldCode = code;
		}
		for(;;)
		{	
			if(remainInBuf<=4 && !readOK)
			{	
				for(i=0;i<remainInBuf;i++)
					buf[i] = buf[bufIndex+i];
				bufIndex = 0;
				
				be=ifs[pos];
				pos++;
				if(be != 0)
				{	
					for(i=remainInBuf;i<remainInBuf+be;i++)
					{buf[i]=ifs[pos];
					pos++;
					}
					remainInBuf += be;
				}
				else
					readOK = TRUE;
				
			}
			if(remainInBuf<=4)
				if(remainInBuf<=0 || codeSize > (remainInBuf*8-bitIndex))
					break;
				temp[0]= buf[bufIndex];
				temp[1]= buf[bufIndex+1];
				temp[2]= buf[bufIndex+2];
				temp[3]= buf[bufIndex+3];
				code = *((long int*)(temp));
				code <<= 32-codeSize-bitIndex;
				code >>= 32-codeSize;
				bitIndex += codeSize;
				bufIndex += bitIndex/8;
				remainInBuf -= bitIndex/8;
				bitIndex %= 8;
				if(code == rootSize)
				{	
					codeSize = codeSizeBK;
					for(i=rootSize;i<4096;i++)
						if(strTable[i].p != NULL)
						{	
							mgif_free(strTable[i].p);
							strTable[i].p = NULL;
							strTable[i].len = 0;
						}
						tableIndex = rootSize+2;
						goto begin;
				}
				else if(code == rootSize+1)
					break;
				else
				{	
					unsigned char *p = strTable[code].p;
					int l = strTable[code].len;
					unsigned char c;
					if(p != NULL)
					{
						
						c = *p;
						if(outIndex+l <= bufLen)
							for(i=0;i<l;i++)
								outP[outIndex++] = *p++;
							else
							{
								goto errorone;
							}
							if(!addStrTable(strTable,tableIndex++,oldCode,c))
							{
					
								goto errorone;
							}
							oldCode = code;
					}
					else
					{	
						p = strTable[oldCode].p;
						l = strTable[oldCode].len;
						c = *p;
						if(outIndex+l+1 <= bufLen)
						{	
							for(i=0;i<l;i++)
								outP[outIndex++] = *p++;
							outP[outIndex++] = c;
						}
						else
						{
							goto errorone;      
						}
						if(!addStrTable(strTable,tableIndex++,oldCode,c))
						{
							goto errorone;
						}
						oldCode = code;
						
					}
					if(tableIndex == (((U32)1)<<codeSize) && codeSize != 12)
						codeSize++;
				}
		}
done:   
		if(strTable!=NULL)
		for(i=0;i<4096;i++)
			if(strTable[i].p != NULL)
			{	
				mgif_free(strTable[i].p);
				strTable[i].p = NULL;
			}
			if(strTable!=NULL)
			{
				mgif_free(strTable);
				strTable=NULL;
			}
			
			return TRUE;
errorone:
			if(strTable!=NULL)
			for(i=0;i<4096;i++)
				if(strTable[i].p != NULL)
				{
					mgif_free(strTable[i].p);
					strTable[i].p = NULL;
				}
				mgif_free(strTable);
		    	
				mgif_free(f->dataBuf);
				f->dataBuf = NULL;
				return FALSE;
}

LPCGLOBAL_INFO getGlobalInfo()
{
	return &gInfo;
}

void closefile()
{   
	U32 i;
	if(opened)
	{	opened = FALSE;
	if(inMem && allFrames != NULL)
	{	FRAME* pf = allFrames;
	for(i=0;i<gInfo.frames;i++)
	{	if(pf->pColorTable != NULL)
	{	mgif_free(pf->pColorTable);
	pf->pColorTable = NULL;
	}
	if(pf->dataBuf != NULL)
	{	mgif_free(pf->dataBuf);
	pf->dataBuf = NULL;
	}
	}
	mgif_free(allFrames);
	allFrames = NULL;
	}
	if(!inMem)
	{	if(curFrame.pColorTable != NULL)
	{	mgif_free(curFrame.pColorTable);
	curFrame.pColorTable = NULL;
	}
	if(curFrame.dataBuf != NULL)
	{
		mgif_free(curFrame.dataBuf);
		curFrame.dataBuf = NULL;
	}
	
	}
	}
}

LPCSTR getVer()
{	
	return gif_version;
}

/*
 * Func: mgif_malloc
 * Desc: 分配空间
 */
void* mgif_malloc ( unsigned int size )
{
       U8 *DataAdd=NULL;

       DataAdd=memory_allocate ( appl_partition, size );
	if(DataAdd!=NULL)
	{
	      memset(DataAdd,0,size);
	      return (void *)DataAdd;
	}
	return NULL;
}
#if 0
mgif_handle gifhandle = NULL, cache_one_gif = NULL;
CH_STATUS CH_DrawGif(S8* FileName,PCH_WINDOW ch_this,  PCH_RECT RectSize, id_backgroundRepeat repeat)
{
	graph_file *file;
	FILE* fp;
	static S8* static_file = NULL;
#ifdef SQ_MDF_20061103
	static unsigned short int s_pid = 0;/*保存当前的PID*/
	extern unsigned short int z_html_pid;
#endif
	extern mgif_handle mgif_Init( unsigned char* rp_Data, unsigned int r_Size, unsigned int r_StartX, unsigned int r_StartY );
	

#ifdef SQ_MDF_20061103
	if(cache_one_gif && static_file == FileName && z_html_pid == s_pid)
#else
	if(cache_one_gif && static_file == FileName)
#endif		
	{
		
		gifhandle = cache_one_gif;
		mgif_Show2ch_this(gifhandle);
		gifhandle->b_enalbe ++;
		return 0;
	}
	else
	{
		file = graph_open((char*)FileName, NULL);
		if(file == NULL)
			return 1;
		#if 0
		{
			fp = fopen("c:\\gif8.gif", "wb+");
			if(fp)
			{
				fwrite(file->linked_obj_addr, 1,  file->obj_size, fp);
				fclose(fp);
			}
		}
		#endif
		/*初始化*/
		{
			allFrames = NULL;
			ifs = NULL;
			errorone	= 0;
			opened = 0;
			inMem = 0;
			dataStart = 0;
			curIndex = 0;
			c1 = 0; 
			c2 = 0;
			pos = 0;
			max = 0;
			memset(&gInfo, 0, sizeof(GLOBAL_INFO));
			memset(&curFrame, 0, sizeof(FRAME));
			memset(&ctrlExt, 0, sizeof(GCTRLEXT));
			memset(&gColorTable, 0, sizeof(U8)*256*3);

		}
		gifhandle = mgif_Init( (unsigned char*)file->linked_obj_addr,(unsigned int) file->obj_size, RectSize->iStartX, RectSize->iStartY);
	}
	if(gifhandle == NULL)
	{
		return 0;
	}
	gifhandle->u_Height = RectSize->iHeigh;
	gifhandle->u_Width = RectSize->iWidth;
	if(mgif_Show2ch_this(gifhandle))
	{
		mgif_DestroyHandle(gifhandle);
		gifhandle = NULL;
	}
	else
	{
		if(cache_one_gif)
		{
			mgif_DestroyHandle(cache_one_gif);
		}
		cache_one_gif = gifhandle;
		static_file = FileName;
#ifdef SQ_MDF_20061103
		s_pid = z_html_pid;/*把当前的PID也保存下来*/
#endif
		gifhandle->b_enalbe ++;
	}
	return 0;
}
#else
mgif_handle gifhandle = NULL;
#ifndef DISABLE_GIF_CACHE
mgif_handle cache_one_gif = NULL;
#endif
CH_STATUS CH_DrawGif(S8* FileName,PCH_WINDOW ch_this, PCH_SHOWRECT p_ShowRect, PCH_RECT RectSize, id_backgroundRepeat repeat,int ri_len)
{
	graph_file *file;
	FILE* fp;
	static S8* static_file = NULL;
	static unsigned short int s_pid = 0;/*保存当前的PID*/
	static long int s_freq = 0;
	extern unsigned short int z_html_pid;
	extern long int z_Current_freqkhz;
	extern mgif_handle mgif_Init( unsigned char* rp_Data, unsigned int r_Size, unsigned int r_StartX, unsigned int r_StartY );

#ifndef DISABLE_GIF_CACHE
	if(cache_one_gif != NULL 		\
		&& static_file == FileName 		\
		&& z_html_pid == s_pid		\
		&& z_Current_freqkhz == s_freq
		&& cache_one_gif->m_StartX == RectSize->iStartX	\
		&& cache_one_gif->m_StartY == RectSize->iStartY)
	{
		
		gifhandle = cache_one_gif;
		mgif_Show2ch_this(ch_this, gifhandle);
		gifhandle->b_enalbe ++;
		return 0;
	}
	else
#endif		
	{
		file = graph_open((char*)FileName, NULL,ri_len);
		if(file == NULL)
			return 1;
		#if 0
		{
			static int i = 0;
			char str[50];
			sprintf(str, "d:\\sqzow\\gif\\gif%d.gif", i);
			fp = fopen(str, "wb+");
			if(fp)
			{
				fwrite(file->linked_obj_addr, 1,  file->obj_size, fp);
				fclose(fp);
			}
			i ++;
		}
		#endif
		/*初始化*/
		{
			allFrames = NULL;
			ifs = NULL;
			errorone	= 0;
			opened = 0;
			inMem = 0;
			dataStart = 0;
			curIndex = 0;
			c1 = 0; 
			c2 = 0;
			pos = 0;
			max = 0;
			memset(&gInfo, 0, sizeof(GLOBAL_INFO));
			memset(&curFrame, 0, sizeof(FRAME));
			memset(&ctrlExt, 0, sizeof(GCTRLEXT));
			memset(&gColorTable, 0, sizeof(U8)*256*3);

		}
	

		gifhandle = mgif_Init( (unsigned char*)file->linked_obj_addr,(unsigned int) file->obj_size, RectSize->iStartX, RectSize->iStartY);
	}
	if(gifhandle == NULL)
	{
		/*sqzow test*/
		#if  0/*打印到文件*/
		{
			FILE* hhfpgif = NULL;

			if(hhfpgif == NULL)
			  	hhfpgif = fopen("c:\\mode2\\gif44.gif", "wb+");
			if(hhfpgif)
			{
				fwrite(file->linked_obj_addr, 1,  file->obj_size, hhfpgif);
				fflush(hhfpgif);
				fclose(hhfpgif);
			}
		}
		#endif
		/*sqzow test*/
		return 1;
	}
	gifhandle->u_Height = CH_MIN(RectSize->iHeigh, gifhandle->m_Height);
	gifhandle->u_Width = CH_MIN(RectSize->iWidth, gifhandle->m_Width);
	if(mgif_Show2ch_this(ch_this, gifhandle))
		{
			mgif_DestroyHandle(gifhandle);
			gifhandle = NULL;
		}
		else
		{
		/*	(gifhandle->m_CurFrameIndex)++;*/
		/*	(gifhandle->m_PreFrameIndex)++;*/
		/*	(gifhandle->m_PrevPrevIndex)++;*/
		}
#if 0
	if(mgif_Show2ch_this(ch_this, gifhandle))
	{
		mgif_DestroyHandle(gifhandle);
		gifhandle = NULL;
	}
	else
	{
	#if 0
		if(chz_is_keepout(-1, p_ShowRect, 1))
		{
#ifndef DISABLE_GIF_CACHE		
			if(cache_one_gif)
			{
				mgif_DestroyHandle(cache_one_gif);
		}
	
			cache_one_gif = gifhandle;
			static_file = FileName;
			s_pid = z_html_pid;/*把当前的PID也保存下来*/
			s_freq = z_Current_freqkhz;
			gifhandle->b_enalbe ++;
#endif			
		}
		else/*如果动画不能完全显示在外，就只显示第一镇*/
		#endif	
		#if 0
		{
			mgif_DestroyHandle(gifhandle);
			gifhandle = NULL;
		}
		#endif
	}
#endif	
	return 0;
}
#endif
/*离开html时调用，清除缓存中的GIF句柄*/

void CH_ClearCacheGif()
{
#ifndef DISABLE_GIF_CACHE
	if(cache_one_gif)
	{
		cache_one_gif->b_enalbe = 0;/*强制清除*/
		mgif_DestroyHandle(cache_one_gif);
	}
	cache_one_gif = NULL;
#endif	
}
PCH_RECT CH_GetGifSize(S8* FileName)
{
	return NULL;
}


mgif_handle mgif_get_handle()
{
	return gifhandle;
}

void mgif_reset_handle()
{
	gifhandle = NULL;
}

int mgif_Show2ch_this ( PCH_WINDOW ch_this, mgif_handle r_GifHandle )
{
	unsigned int	uStartX, uStartY;
	unsigned int	uIndex, uIndex1;
	unsigned int	sPx = 0, sPy = 0;
	CH_RECT rect, destrect;
	LPFRAME 		frame;
	unsigned char	*pPalette = NULL;
	unsigned int	uMiniSec;
	unsigned int	uLoop;

#ifdef USE_ARGB8888	
	U32* p_32DataFront;
#else
    unsigned short* p_16DataFront;

#endif
	/*画第一次*/

	/*check_memStatus("mgif_Show2ch_this start");*/
	frame = &(r_GifHandle->m_FrameInfo[0].p_Frame);
	if( frame->lFlag == 0 )
	{
		pPalette	= r_GifHandle->p_PaletteData;
	}
	else
	{
		pPalette	= frame->pColorTable;
	}

	/* 画图片*/
	for ( uLoop = 0; uLoop < 256; uLoop ++ )
	{
#ifdef USE_ARGB8888	


myx_32_pal[uLoop]  = 0xFF000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);

#elif USE_ARGB1555	
		myx_16_pal[uLoop] = 0x8000 | (((pPalette[uLoop*3]>>3) & 0x00ff ) <<10 ) | \
							((( pPalette[uLoop*3+1] >>3) & 0x00ff) <<5) | ((pPalette[uLoop*3+2] >>3) & 0x00ff) ;
#elif USE_ARGB4444
        myx_16_pal[uLoop] = 0xf000 | (((pPalette[uLoop*3]>>4) & 0x00ff ) <<8 ) | \
					        ((( pPalette[uLoop*3+1] >>4) & 0x00ff) <<4) | ((pPalette[uLoop*3+2] >>4) & 0x00ff) ;

#endif
	}
#ifdef USE_ARGB8888	
p_32DataFront = (U32*)(r_GifHandle->p_DrawData);

#else
	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);
#endif
	
	for( uStartY = frame->imageTPos+sPy; uStartY < frame->imageHeight + frame->imageTPos + sPy; uStartY ++ )
	{
		for( uStartX = frame->imageLPos+ sPx; uStartX < frame->imageWidth + frame->imageLPos + sPx; uStartX ++ )
		{
			uIndex = ( uStartY - frame->imageTPos - sPy ) * frame->imageWidth + uStartX - frame->imageLPos - sPx;
			uIndex1	= uStartY * r_GifHandle->m_Width + uStartX;
			
	
#ifdef USE_ARGB8888	
	
		if(frame->ctrlExt.trsFlag == 0
				|| frame->dataBuf[uIndex] != frame->ctrlExt.trsColorIndex)
			{
				p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
			}
			else
			{
				p_32DataFront[uIndex1] = 0;
			}
		#else
			if(frame->ctrlExt.trsFlag == 0
				|| frame->dataBuf[uIndex] != frame->ctrlExt.trsColorIndex)
			{
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			}
			else
			{
				p_16DataFront[uIndex1] = 0;
			}
		#endif		
		}
	}

	CH_ZERO(rect);
	rect.iHeigh = r_GifHandle->m_Height;
	rect.iWidth = r_GifHandle->m_Width;
	destrect.iStartX = r_GifHandle->m_StartX;
	destrect.iStartY = r_GifHandle->m_StartY;
	destrect.iHeigh = CH_MIN(rect.iHeigh, r_GifHandle->u_Height);
	destrect.iWidth = CH_MIN(rect.iWidth, r_GifHandle->u_Width);
/*	
	CH_PutBitmapToScr3(ch_this, 		\
							&rect, 		\
							&destrect,		\
							r_GifHandle->m_TotalFrame > 1 ? 1 : frame->ctrlExt.trsColorIndex ,		\
							(S8*)r_GifHandle->p_DrawData);
*/
	/*改单帧的透明GIF无法显示的问题 sqzow070320*/
	CH_PutBitmapToScr3(ch_this, 		\
							&rect, 		\
							&destrect,		\
							(frame->ctrlExt.trsFlag == 1)? 1 : 0 ,		\
							(S8*)r_GifHandle->p_DrawData);

	/*check_memStatus("mgif_Show2ch_this end");*/
	if( r_GifHandle->m_TotalFrame == 1 )
		return 1;
	return 0;
}



/*
 * Func: mgif_Init
 * Desc: 动态图像初始化函数
 */
 int leftgif_num = 0;
mgif_handle mgif_Init( unsigned char* rp_Data, unsigned int r_Size, unsigned int r_StartX, unsigned int r_StartY )
{
	mgif_handle		gif_data;
	LPCGLOBAL_INFO 	gi;
	LPFRAME 		fm;
	unsigned int	i;
	unsigned int	u_DrawDataLen;
	unsigned int	uCount;
	LPFRAME 		frame;



	#ifdef YY_GIF_SHOW
	/*do_report(0,"rp_Data=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",rp_Data[0],rp_Data[1],rp_Data[2],rp_Data[3],rp_Data[4],rp_Data[5],rp_Data[6],rp_Data[7],rp_Data[8],rp_Data[9],rp_Data[10]);  */
	#endif
	
	/* 检查参数的有效性 */
	if ( NULL == rp_Data )
	{
		return NULL;
	}

	r_Size		+= 1;

	max			= r_Size;				/* 得到文件的大小 */

	g_Buffer	= rp_Data;

	/* 打开文件, 检查帧个数 */
	
	CGif89a( 0, FALSE );
	

	/* 得到版本号 */
	getVer();

	/* 得到全局信息结构 */
	gi = getGlobalInfo();

	/* 分配空间 */
	#ifdef CQ_BROWSER
	gif_data = ( mgif_handle )CH_AllocMemAndZero( sizeof( mgif_info_t ) );
	#else
	gif_data = ( mgif_handle )mgif_malloc( sizeof( mgif_info_t ) );
	#endif
	/* 分配失败 */
	if ( NULL == gif_data )
	{
		return NULL;
	}

	leftgif_num ++;
	/*STTBX_Print("left %d gif in memory\n", leftgif_num);*/
	/* 分配背景和绘画板 */
#ifdef USE_ARGB8888
u_DrawDataLen = gi->scrHeight * gi->scrWidth * 4;

#elif USE_ARGB1555
	u_DrawDataLen = gi->scrHeight * gi->scrWidth * 2;
#elif USE_ARGB4444
    u_DrawDataLen = gi->scrHeight * gi->scrWidth * 2;
#endif
	gif_data->p_BackupData 	= NULL;
	gif_data->p_MethodData	= NULL; 

	/* 分配画板 */
	gif_data->p_DrawData = (unsigned char*)mgif_malloc ( u_DrawDataLen );

	#ifndef No_GIF_Back
	if ( NULL == gif_data->p_DrawData )
	{
		mgif_free ( gif_data->p_MethodData );
		mgif_free ( gif_data->p_BackupData );
		mgif_free ( gif_data );
		return NULL;
	}
	#endif

	/* 拷贝数据 */
	#ifndef No_GIF_Back
	if ( gif_data->p_BackupData )
	{
#ifdef USE_ARGB8888
		if ( gif_data->p_DrawData ) 
			memcpy( gif_data->p_DrawData, gif_data->p_BackupData, gif_data->m_Width*gif_data->m_Height*4 );
		if ( gif_data->p_MethodData ) 
			memcpy( gif_data->p_MethodData, gif_data->p_BackupData, gif_data->m_Width*gif_data->m_Height*4 );
#elif USE_ARGB1555
		if ( gif_data->p_DrawData ) 
			memcpy( gif_data->p_DrawData, gif_data->p_BackupData, gif_data->m_Width*gif_data->m_Height*2 );
		if ( gif_data->p_MethodData ) 
			memcpy( gif_data->p_MethodData, gif_data->p_BackupData, gif_data->m_Width*gif_data->m_Height*2 );
#elif USE_ARGB4444	
        if ( gif_data->p_DrawData ) 
			memcpy( gif_data->p_DrawData, gif_data->p_BackupData, gif_data->m_Width*gif_data->m_Height*2 );
		if ( gif_data->p_MethodData ) 
			memcpy( gif_data->p_MethodData, gif_data->p_BackupData, gif_data->m_Width*gif_data->m_Height*2 );
#endif
	}
	#endif

	/* 分配色表 */
	gif_data->p_PaletteData = (unsigned char*)mgif_malloc ( 256*3 );

	if ( NULL == gif_data->p_PaletteData )
	{
		mgif_free ( gif_data->p_MethodData );
		mgif_free ( gif_data->p_PaletteData );
		mgif_free ( gif_data->p_DrawData );
		mgif_free ( gif_data );
		return NULL;
	}

	/* 拷贝色表 */
	memcpy ( gif_data->p_PaletteData, gColorTable, 256*3 );

	/* 设置gif图像属性 */
	gif_data->m_CurFrameIndex	= 0;
	gif_data->m_PreFrameIndex	= -1;
	gif_data->m_TotalFrame		= gi->frames;
	gif_data->m_Height			= gi->scrHeight;
	gif_data->m_Width			= gi->scrWidth;
	gif_data->m_StartX			= r_StartX;
	gif_data->m_StartY			= r_StartY;
	gif_data->m_Size				= r_Size;
	gif_data->p_ActData			= rp_Data;
	gif_data->p_GlobeInfo			= gi;
	gif_data->b_enalbe = 0;
	/* 循环所有的帧 */	
	for( i = 0, uCount = 0; i < gi->frames; i ++ )
	{
		/* 得到下一帧图像 */

		fm = (LPFRAME)getNextFrame ( &gif_data->m_FrameInfo[uCount].p_Frame );

		if ( NULL == fm )
		{
			mgif_free ( gif_data->p_MethodData );
			mgif_free ( gif_data->p_PaletteData );
			#ifndef No_GIF_Back
			mgif_free ( gif_data->p_BackupData );
			#endif
			mgif_free ( gif_data->p_DrawData );
			mgif_free ( gif_data );

			return NULL;
		}

		frame	= fm;

		gif_data->m_FrameInfo[uCount].m_DelayTime		= ctrlExt.delayTime;		/* 延迟时间 */
		gif_data->m_FrameInfo[uCount].m_HandleMethod	= ctrlExt.disposalMethod;	/* 处置方法 */
		gif_data->m_FrameInfo[uCount].m_OffsetX		= frame->imageLPos;			/* X方向偏移 */
		gif_data->m_FrameInfo[uCount].m_OffsetY		= frame->imageTPos;			/* Y方向偏移 */
		gif_data->m_FrameInfo[uCount].m_LocalPalette	= frame->lFlag;				/* 0->使用全局色板,1->使用局部色板 */

		uCount	++;
		if(uCount>=100)
			break;
	}

	/* 得到所有的帧数 */
	gif_data->m_TotalFrame = uCount;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"gif_data->m_PreFrameIndex=%x\n",gif_data->m_PreFrameIndex);
	do_report(0,"gif_data->m_CurFrameIndex=%x\n",gif_data->m_CurFrameIndex);*/  
	#endif

	return gif_data;
}


/*
 * 浏览器专用
 */
void mgif_DestroyHandle ( mgif_handle r_GifHandle )
{
	unsigned int uIndex;

	leftgif_num --;
	/*STTBX_Print("left %d gif in memory 2\n", leftgif_num);*/
	
	/* 检查句柄的有效性 */
	if ( NULL == r_GifHandle )
	{
		return;
	}

	if(r_GifHandle->b_enalbe > 0)/*保留在缓存中 sqzow 061013*/
	{
		r_GifHandle->b_enalbe --;
		return;
	}
	
	/* 恢复背景 */
	#if 0
	if ( NULL != r_GifHandle->p_BackupData )
	{
	#if 0
		CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, 
							r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
							r_GifHandle->m_Width, r_GifHandle->m_Height, 
							r_GifHandle->p_BackupData );
	#else  /*00_GIF*/
		if ( NULL != r_GifHandle->p_BackupData )
		{
			CH6_PutRectangle( GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
							r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
							r_GifHandle->m_Width, r_GifHandle->m_Height, 
							r_GifHandle->p_BackupData );
		}
	#endif

		#ifdef ST_5105
		CH6_AreaUpdate( r_GifHandle->m_StartX, r_GifHandle->m_StartY, r_GifHandle->m_Width, r_GifHandle->m_Height );
		#endif
	}
	#endif

	for ( uIndex = 0; uIndex < r_GifHandle->m_TotalFrame; uIndex ++ )
	{
		if ( NULL != r_GifHandle->m_FrameInfo[uIndex].p_Frame.pColorTable )
		{
			mgif_free ( r_GifHandle->m_FrameInfo[uIndex].p_Frame.pColorTable );
			r_GifHandle->m_FrameInfo[uIndex].p_Frame.pColorTable = NULL;
		}

		if ( NULL != r_GifHandle->m_FrameInfo[uIndex].p_Frame.dataBuf )
		{
			mgif_free ( r_GifHandle->m_FrameInfo[uIndex].p_Frame.dataBuf );
			r_GifHandle->m_FrameInfo[uIndex].p_Frame.dataBuf = NULL;			
		}
	}

	/* 依次删除所有的数据 */
	mgif_free ( r_GifHandle->p_PaletteData );
	#if 0
	if(r_GifHandle->p_BackupData)
	{
		memory_deallocate ( SystemPartition, r_GifHandle->p_BackupData);
	}
	#else
	mgif_free ( r_GifHandle->p_BackupData );
	#endif
	mgif_free ( r_GifHandle->p_DrawData );
	mgif_free ( r_GifHandle->p_MethodData );
	mgif_free ( r_GifHandle );

	r_GifHandle = NULL;
	g_Buffer	= NULL;

	return;
}


/*
 * Func: mgif_Term
 * Desc: 动态gif终止
 */
void mgif_Term ( mgif_handle r_GifHandle )
{
	unsigned int uIndex;


	/* 检查句柄的有效性 */
	if ( NULL == r_GifHandle )
	{
		return;
	}

	
	/* 恢复背景 */
	#ifndef No_GIF_Back
	if ( NULL != r_GifHandle->p_BackupData )
	{
	#if 0
		CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, 
							r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
							r_GifHandle->m_Width, r_GifHandle->m_Height, 
							r_GifHandle->p_BackupData );
	#else  /*00_GIF*/
		if ( NULL != r_GifHandle->p_BackupData )
		{
#ifdef USE_ARGB8888
			CH6_PutRectangle( GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
							r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
							r_GifHandle->m_Width, r_GifHandle->m_Height, 
							r_GifHandle->p_BackupData );
#elif USE_ARGB1555	
			CH6_PutRectangle( GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
							r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
							r_GifHandle->m_Width, r_GifHandle->m_Height, 
							r_GifHandle->p_BackupData );
#elif USE_ARGB4444	
			CH6_PutRectangle( GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
							r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
							r_GifHandle->m_Width, r_GifHandle->m_Height, 
							r_GifHandle->p_BackupData );
#endif
		}
	#endif

		#ifdef ST_5105
		CH6_AreaUpdate( r_GifHandle->m_StartX, r_GifHandle->m_StartY, r_GifHandle->m_Width, r_GifHandle->m_Height );
		#endif
	}
	#endif

	for ( uIndex = 0; uIndex < r_GifHandle->m_TotalFrame; uIndex ++ )
	{
		if ( NULL != r_GifHandle->m_FrameInfo[uIndex].p_Frame.pColorTable )
		{
			mgif_free ( r_GifHandle->m_FrameInfo[uIndex].p_Frame.pColorTable );
			r_GifHandle->m_FrameInfo[uIndex].p_Frame.pColorTable = NULL;
		}

		if ( NULL != r_GifHandle->m_FrameInfo[uIndex].p_Frame.dataBuf )
		{
			mgif_free ( r_GifHandle->m_FrameInfo[uIndex].p_Frame.dataBuf );
			r_GifHandle->m_FrameInfo[uIndex].p_Frame.dataBuf = NULL;			
		}
	}

	/* 依次删除所有的数据 */
	mgif_free ( r_GifHandle->p_PaletteData );
	if(r_GifHandle->p_BackupData)
	{
		memory_deallocate ( SystemPartition, r_GifHandle->p_BackupData);
	}
	mgif_free ( r_GifHandle->p_DrawData );
	mgif_free ( r_GifHandle->p_MethodData );
	mgif_free ( r_GifHandle );

	r_GifHandle = NULL;
	g_Buffer	= NULL;

	return;
}


/*
 * Func: mgif_ShowNext wz 20081218 添加了象素的透明处理
 * Desc: 显示下一帧图像
 * return:该帧要延迟的毫秒数
 */
int mgif_ShowNextFrame_pelstransparency(PCH_WINDOW ch_this, mgif_handle r_GifHandle, int StartX, int StartY)/*在指定大小的区域里画 sqzow 0921*/

{
	int				iFirst = true;
	LPCFRAME		frame;
	#ifdef WINDOWS_DEMO
	HDC				hdc;
	#endif
	unsigned char	*pPalette = NULL;
	unsigned int	sPx, sPy;
	unsigned int	uStartX, uStartY;
	unsigned int	uIndex, uIndex1;
	unsigned int	uMiniSec;
	unsigned int	uLoop;
	unsigned short* p_16DataFront;
#ifdef USE_ARGB8888	

	U32* p_32DataFront;

#endif	
	T_OSD_Rect src_rect, dest_rect;
	sPx = 0;
	sPy = 0;
	/*check_memStatus("mgif_ShowNextFrame start");*/

	/*
	do_report ( 0, "\ngif_handle->0x%x, %d, %d", r_GifHandle, ru_StartX, ru_StartY );
	*/
	if ( NULL == r_GifHandle )
		return 50;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"r_GifHandle->m_PreFrameIndex=%x\n",r_GifHandle->m_PreFrameIndex);
	do_report(0,"r_GifHandle->m_CurFrameIndex=%x\n",r_GifHandle->m_CurFrameIndex);*/ 
	#endif

	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		#ifndef No_GIF_Back
	
		if ( NULL == r_GifHandle->p_BackupData )
		{
#ifdef USE_ARGB8888	
         r_GifHandle->p_BackupData = (unsigned char*)mgif_malloc(r_GifHandle->m_Width * r_GifHandle->m_Height *4);

#elif USE_ARGB1555
			r_GifHandle->p_BackupData = (unsigned char*)mgif_malloc(r_GifHandle->m_Width * r_GifHandle->m_Height * 2);
#elif USE_ARGB4444
			r_GifHandle->p_BackupData = (unsigned char*)mgif_malloc(r_GifHandle->m_Width * r_GifHandle->m_Height * 2);

#endif			
			if(r_GifHandle->p_BackupData == NULL)
				return -1;
			dest_rect.nHeight = r_GifHandle->m_Height;
			dest_rect.nWidth = r_GifHandle->m_Width;
			dest_rect.nX = StartX;/*r_GifHandle->m_StartX;*/
			dest_rect.nY = StartY;/*r_GifHandle->m_StartY;*/
			J_OSD_BlockRead(&dest_rect, (void*)r_GifHandle->p_BackupData);
			iFirst = false;
		}
		#endif
#ifdef USE_ARGB8888	

#if 1
if ( NULL == r_GifHandle->p_MethodData )
	r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*4 );

if ( r_GifHandle->p_BackupData && false == iFirst )
{
	if ( r_GifHandle->p_DrawData ) 
		memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*4 );
	if ( r_GifHandle->p_MethodData ) 
		memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*4 );
}
#endif

#elif USE_ARGB1555
		#if 1
		if ( NULL == r_GifHandle->p_MethodData )
			r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

		if ( r_GifHandle->p_BackupData && false == iFirst )
		{
			if ( r_GifHandle->p_DrawData ) 
				memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
			if ( r_GifHandle->p_MethodData ) 
				memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		}
		#endif
#elif USE_ARGB4444
		#if 1
		if ( NULL == r_GifHandle->p_MethodData )
			r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

		if ( r_GifHandle->p_BackupData && false == iFirst )
		{
			if ( r_GifHandle->p_DrawData ) 
				memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
			if ( r_GifHandle->p_MethodData ) 
				memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		}
		#endif		
	#endif	
	}

	/* 如果只有一帧图像 */
	if ( r_GifHandle->m_PreFrameIndex == r_GifHandle->m_CurFrameIndex )
	{
		uMiniSec = r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].m_DelayTime;

		if ( 0 == uMiniSec )
		{
			uMiniSec = 5;
		}

		return uMiniSec;
	}

	/* 得到下一帧的指针 */
	frame = &(r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].p_Frame);

	/* 如果帧指针为空 */
	if ( NULL == frame )
	{
		return;
	}

	#ifdef WINDOWS_DEMO
	hdc = GetDC( g_Hwnd );
	#endif

	if( frame->lFlag == 0 )
	{
		pPalette	= r_GifHandle->p_PaletteData;
	}
	else
	{
		pPalette	= frame->pColorTable;
	}

	/* 画图片*/
	for ( uLoop = 0; uLoop < 256; uLoop ++ )
	{
#ifdef USE_ARGB8888	
		
 	 if(frame->ctrlExt.trsColorIndex == uLoop && frame->ctrlExt.trsFlag == 1)
		 myx_32_pal[uLoop]	= 0x00000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);
	 else
		myx_32_pal[uLoop]  = 0xFF000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);
		
#elif USE_ARGB1555	
	
		myx_16_pal[uLoop] = 0x8000 | (((pPalette[uLoop*3]>>3) & 0x00ff ) <<10 ) | \
							((( pPalette[uLoop*3+1] >>3) & 0x00ff) <<5) | ((pPalette[uLoop*3+2] >>3) & 0x00ff) ;
/* wz 20081218  add 如果R G B都等于0,将这个像素点置为透明的*/
		if(myx_16_pal[uLoop] == 0x8000)
		{
			myx_16_pal[uLoop] = 0;
		}
/*************************************/
#elif USE_ARGB4444	
	
		myx_16_pal[uLoop] = 0xf000 | (((pPalette[uLoop*3]>>4) & 0x00ff ) <<8 ) | \
							((( pPalette[uLoop*3+1] >>4) & 0x00ff) <<4) | ((pPalette[uLoop*3+2] >>4) & 0x00ff) ;

#endif
	}
#ifdef USE_ARGB8888	
p_32DataFront = (U32*)(r_GifHandle->p_DrawData);

#elif USE_ARGB1555	
	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);
#elif USE_ARGB4444	
	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);

#endif

	/* 根据上一帧图片的方法处置上一幅图片 */
	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		if ( -1 != r_GifHandle->m_PreFrameIndex )
		{
			/* 按照处置方法进行处置 */
			switch( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
			{
			case 0:	/* 什么都不做 */
				break;
		
#ifdef USE_ARGB8888	
case 1: /* 不处置图形，把图形从当前位置移去 */
	break;
case 3: /* 恢复到先前状态 */
	if ( p_32DataFront && r_GifHandle->p_MethodData )
		
		memcpy ( p_32DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 4 );
	break;
case 2: /* 回复到背景色 */
	if ( p_32DataFront && r_GifHandle->p_BackupData )

	
			{
	#if 0
	            dest_rect.nHeight = r_GifHandle->m_Height;
				dest_rect.nWidth = r_GifHandle->m_Width;
				dest_rect.nX = StartX;//r_GifHandle->m_StartX;
				dest_rect.nY = StartY;//r_GifHandle->m_StartY;

				J_OSD_BlockRead(&dest_rect, (void*)r_GifHandle->p_BackupData);
	#endif		 
		memcpy ( p_32DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 4 );
	
			}
	break;

#elif USE_ARGB1555	
			case 1:	/* 不处置图形，把图形从当前位置移去 */
				break;
			case 3:	/* 恢复到先前状态 */
				if ( p_16DataFront && r_GifHandle->p_MethodData )
					memcpy ( p_16DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			case 2:	/* 回复到背景色 */
				if ( p_16DataFront && r_GifHandle->p_BackupData )
					memcpy ( p_16DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
#elif USE_ARGB4444	
			case 1:	/* 不处置图形，把图形从当前位置移去 */
				break;
			case 3:	/* 恢复到先前状态 */
				if ( p_16DataFront && r_GifHandle->p_MethodData )
					memcpy ( p_16DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			case 2:	/* 回复到背景色 */
				if ( p_16DataFront && r_GifHandle->p_BackupData )
					memcpy ( p_16DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;				
#endif				
			default:
				break;
			}
		}
	}

	for( uStartY = frame->imageTPos+sPy; uStartY < frame->imageHeight + frame->imageTPos + sPy; uStartY ++ )
	{
		for( uStartX = frame->imageLPos+ sPx; uStartX < frame->imageWidth + frame->imageLPos + sPx; uStartX ++ )
		{
			uIndex = ( uStartY - frame->imageTPos - sPy ) * frame->imageWidth + uStartX - frame->imageLPos - sPx;
			uIndex1	= uStartY * r_GifHandle->m_Width + uStartX;
#ifdef USE_ARGB8888	
		

			if ( 0 == r_GifHandle->m_CurFrameIndex )
						p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
			else
				{

			if (  frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] 		\
				|| frame->ctrlExt.trsFlag == 0)
			{
				p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
		

			}
			else
				{
				#if 0
				if(r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod == 2)
					{
					p_32DataFront[uIndex1] = myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx];
			}
				#endif
				
				}
			
			
				}
			#if 0
			if(r_GifHandle->p_BackupData != NULL)
			{
			//r_GifHandle->p_BackupData [uIndex1*4] = /*myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx]*/p_32DataFront[uIndex1] ;
			memcpy(&r_GifHandle->p_BackupData [uIndex1*4],&myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx]/*p_32DataFront[uIndex1]*/,4);
			}
		   #endif

#elif USE_ARGB1555	
			if ( 0 == r_GifHandle->m_CurFrameIndex )
						p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			else
			{

			if (  frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] 		\
				|| frame->ctrlExt.trsFlag == 0)
			{
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			}
			}
#elif USE_ARGB4444	
			if ( 0 == r_GifHandle->m_CurFrameIndex )
						p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			else
			{

			if (  frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] 		\
				|| frame->ctrlExt.trsFlag == 0)
			  {
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			   }
			}			
			
#endif
		}
	}
	
	src_rect.nX = /*0*/StartX;
	src_rect.nY = /*0*/StartX;
	src_rect.nWidth = r_GifHandle->m_Width;
	src_rect.nHeight = r_GifHandle->m_Height;
	dest_rect.nX =/* r_GifHandle->m_StartX;*/StartX;
	dest_rect.nY =/*  r_GifHandle->m_StartY;*/StartY;
	dest_rect.nHeight = r_GifHandle->u_Height;
	dest_rect.nWidth = r_GifHandle->u_Width;
#ifdef USE_ARGB8888	
//J_OSD_BlockCopyEx(&src_rect, &dest_rect, (void*)p_32DataFront, 0, NULL, NULL);/**/
memcpy((U8 *)ch_this->scrBuffer.dwHandle,(U8 *)p_32DataFront, r_GifHandle->m_Width*r_GifHandle->u_Height*4);

#elif USE_ARGB1555	
	/*J_OSD_BlockCopyEx(&src_rect, &dest_rect, (void*)p_16DataFront, 0, NULL, NULL);*/
	
memcpy((U8 *)ch_this->scrBuffer.dwHandle,(U8 *)p_16DataFront, r_GifHandle->m_Width*r_GifHandle->u_Height*2);
#elif USE_ARGB4444	
	
memcpy((U8 *)ch_this->scrBuffer.dwHandle,(U8 *)p_16DataFront, r_GifHandle->m_Width*r_GifHandle->u_Height*2);

#endif

#ifdef ST_5105
	CH6_AreaUpdate( r_GifHandle->m_StartX, r_GifHandle->m_StartY, r_GifHandle->m_Width, r_GifHandle->m_Height );
#endif

	uMiniSec = frame->ctrlExt.delayTime;

	

	if ( 0 == uMiniSec )
	{
		uMiniSec = 5;
	}

	/*  保持上一帧图像的序号 */
	r_GifHandle->m_PreFrameIndex = r_GifHandle->m_CurFrameIndex;

	/* 更新下一帧的序号 */
	if ( r_GifHandle->m_CurFrameIndex == r_GifHandle->m_TotalFrame - 1 )
		r_GifHandle->m_CurFrameIndex = 0;
	else
		r_GifHandle->m_CurFrameIndex ++;

	switch ( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
	{
	case 0:	/* 什么都不做 */
		break;
	case 1:	/* 不处置图形，把图形从当前位置移去 */
		break;
	case 3:	/* 恢复到先前状态 */
#ifdef USE_ARGB8888	

if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
	memcpy ( (r_GifHandle->p_MethodData),/* (r_GifHandle->p_DrawData)*/p_32DataFront, r_GifHandle->m_Width*r_GifHandle->m_Height*4);

#elif USE_ARGB1555	
		if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
			memcpy ( (r_GifHandle->p_MethodData), /*(r_GifHandle->p_DrawData)*/p_16DataFront, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
#elif USE_ARGB4444	
				if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
					memcpy ( (r_GifHandle->p_MethodData), /*(r_GifHandle->p_DrawData)*/p_16DataFront, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

#endif		
		break;
	case 2:	/* 回复到背景色 */
		#if 0
		{
		int i;
		int j;
		for (i=0;i<r_GifHandle->m_Width;i++)
			{
			for(j=0;j<r_GifHandle->m_Height;j++)
				{
                        if(r_GifHandle->p_BackupData != NULL)
                        	{
							if(r_GifHandle->p_BackupData [i*r_GifHandle->m_Height+j] == myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx])
								{

								}

                        	}
				}
			}

		}
		#endif
		break;
	default:
		break;
	}

	#ifdef WINDOWS_DEMO
	ReleaseDC ( g_Hwnd, hdc );
	#endif
	/*check_memStatus("mgif_ShowNextFrame end");*/
	return uMiniSec;
}


/*
 * Func: mgif_ShowNext
 * Desc: 显示下一帧图像
 * return:该帧要延迟的毫秒数
 */
int mgif_ShowNextFrame (PCH_WINDOW ch_this, mgif_handle r_GifHandle, int StartX, int StartY)/*在指定大小的区域里画 sqzow 0921*/

{
	int				iFirst = true;
	LPCFRAME		frame;
	#ifdef WINDOWS_DEMO
	HDC				hdc;
	#endif
	unsigned char	*pPalette = NULL;
	unsigned int	sPx, sPy;
	unsigned int	uStartX, uStartY;
	unsigned int	uIndex, uIndex1;
	unsigned int	uMiniSec;
	unsigned int	uLoop;
	unsigned short* p_16DataFront;
#ifdef USE_ARGB8888	

	U32* p_32DataFront;

#endif	
	T_OSD_Rect src_rect, dest_rect;
	sPx = 0;
	sPy = 0;
	/*check_memStatus("mgif_ShowNextFrame start");*/

	/*
	do_report ( 0, "\ngif_handle->0x%x, %d, %d", r_GifHandle, ru_StartX, ru_StartY );
	*/
	if ( NULL == r_GifHandle )
		return 50;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"r_GifHandle->m_PreFrameIndex=%x\n",r_GifHandle->m_PreFrameIndex);
	do_report(0,"r_GifHandle->m_CurFrameIndex=%x\n",r_GifHandle->m_CurFrameIndex);*/ 
	#endif

	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		#ifndef No_GIF_Back
	
		if ( NULL == r_GifHandle->p_BackupData )
		{
#ifdef USE_ARGB8888	
         r_GifHandle->p_BackupData = (unsigned char*)mgif_malloc(r_GifHandle->m_Width * r_GifHandle->m_Height *4);

#elif USE_ARGB1555
			r_GifHandle->p_BackupData = (unsigned char*)mgif_malloc(r_GifHandle->m_Width * r_GifHandle->m_Height * 2);
#elif USE_ARGB4444
			r_GifHandle->p_BackupData = (unsigned char*)mgif_malloc(r_GifHandle->m_Width * r_GifHandle->m_Height * 2);

#endif			
			if(r_GifHandle->p_BackupData == NULL)
				return -1;
			dest_rect.nHeight = r_GifHandle->m_Height;
			dest_rect.nWidth = r_GifHandle->m_Width;
			dest_rect.nX = StartX;/*r_GifHandle->m_StartX;*/
			dest_rect.nY = StartY;/*r_GifHandle->m_StartY;*/
			J_OSD_BlockRead(&dest_rect, (void*)r_GifHandle->p_BackupData);
			iFirst = false;
		}
		#endif
#ifdef USE_ARGB8888	

#if 1
if ( NULL == r_GifHandle->p_MethodData )
	r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*4 );

if ( r_GifHandle->p_BackupData && false == iFirst )
{
	if ( r_GifHandle->p_DrawData ) 
		memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*4 );
	if ( r_GifHandle->p_MethodData ) 
		memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*4 );
}
#endif

#elif USE_ARGB1555
		#if 1
		if ( NULL == r_GifHandle->p_MethodData )
			r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

		if ( r_GifHandle->p_BackupData && false == iFirst )
		{
			if ( r_GifHandle->p_DrawData ) 
				memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
			if ( r_GifHandle->p_MethodData ) 
				memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		}
		#endif
#elif USE_ARGB4444
		#if 1
		if ( NULL == r_GifHandle->p_MethodData )
			r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

		if ( r_GifHandle->p_BackupData && false == iFirst )
		{
			if ( r_GifHandle->p_DrawData ) 
				memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
			if ( r_GifHandle->p_MethodData ) 
				memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		}
		#endif		
	#endif	
	}

	/* 如果只有一帧图像 */
	if ( r_GifHandle->m_PreFrameIndex == r_GifHandle->m_CurFrameIndex )
	{
		uMiniSec = r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].m_DelayTime;

		if ( 0 == uMiniSec )
		{
			uMiniSec = 5;
		}

		return uMiniSec;
	}

	/* 得到下一帧的指针 */
	frame = &(r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].p_Frame);

	/* 如果帧指针为空 */
	if ( NULL == frame )
	{
		return;
	}

	#ifdef WINDOWS_DEMO
	hdc = GetDC( g_Hwnd );
	#endif

	if( frame->lFlag == 0 )
	{
		pPalette	= r_GifHandle->p_PaletteData;
	}
	else
	{
		pPalette	= frame->pColorTable;
	}

	/* 画图片*/
	for ( uLoop = 0; uLoop < 256; uLoop ++ )
	{
#ifdef USE_ARGB8888	
		
 	 if(frame->ctrlExt.trsColorIndex == uLoop && frame->ctrlExt.trsFlag == 1)
		 myx_32_pal[uLoop]	= 0x00000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);
	 else
		myx_32_pal[uLoop]  = 0xFF000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);
		
#elif USE_ARGB1555	
	
		myx_16_pal[uLoop] = 0x8000 | (((pPalette[uLoop*3]>>3) & 0x00ff ) <<10 ) | \
							((( pPalette[uLoop*3+1] >>3) & 0x00ff) <<5) | ((pPalette[uLoop*3+2] >>3) & 0x00ff) ;
#elif USE_ARGB4444	
	
		myx_16_pal[uLoop] = 0xf000 | (((pPalette[uLoop*3]>>4) & 0x00ff ) <<8 ) | \
							((( pPalette[uLoop*3+1] >>4) & 0x00ff) <<4) | ((pPalette[uLoop*3+2] >>4) & 0x00ff) ;

#endif
	}
#ifdef USE_ARGB8888	
p_32DataFront = (U32*)(r_GifHandle->p_DrawData);

#elif USE_ARGB1555	
	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);
#elif USE_ARGB4444	
	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);

#endif

	/* 根据上一帧图片的方法处置上一幅图片 */
	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		if ( -1 != r_GifHandle->m_PreFrameIndex )
		{
			/* 按照处置方法进行处置 */
			switch( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
			{
			case 0:	/* 什么都不做 */
				break;
		
#ifdef USE_ARGB8888	
case 1: /* 不处置图形，把图形从当前位置移去 */
	break;
case 3: /* 恢复到先前状态 */
	if ( p_32DataFront && r_GifHandle->p_MethodData )
		
		memcpy ( p_32DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 4 );
	break;
case 2: /* 回复到背景色 */
	if ( p_32DataFront && r_GifHandle->p_BackupData )

	
			{
	#if 0
	            dest_rect.nHeight = r_GifHandle->m_Height;
				dest_rect.nWidth = r_GifHandle->m_Width;
				dest_rect.nX = StartX;//r_GifHandle->m_StartX;
				dest_rect.nY = StartY;//r_GifHandle->m_StartY;

				J_OSD_BlockRead(&dest_rect, (void*)r_GifHandle->p_BackupData);
	#endif		 
		memcpy ( p_32DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 4 );
	
			}
	break;

#elif USE_ARGB1555	
			case 1:	/* 不处置图形，把图形从当前位置移去 */
				break;
			case 3:	/* 恢复到先前状态 */
				if ( p_16DataFront && r_GifHandle->p_MethodData )
					memcpy ( p_16DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			case 2:	/* 回复到背景色 */
				if ( p_16DataFront && r_GifHandle->p_BackupData )
					memcpy ( p_16DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
#elif USE_ARGB4444	
			case 1:	/* 不处置图形，把图形从当前位置移去 */
				break;
			case 3:	/* 恢复到先前状态 */
				if ( p_16DataFront && r_GifHandle->p_MethodData )
					memcpy ( p_16DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			case 2:	/* 回复到背景色 */
				if ( p_16DataFront && r_GifHandle->p_BackupData )
					memcpy ( p_16DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;				
#endif				
			default:
				break;
			}
		}
	}

	for( uStartY = frame->imageTPos+sPy; uStartY < frame->imageHeight + frame->imageTPos + sPy; uStartY ++ )
	{
		for( uStartX = frame->imageLPos+ sPx; uStartX < frame->imageWidth + frame->imageLPos + sPx; uStartX ++ )
		{
			uIndex = ( uStartY - frame->imageTPos - sPy ) * frame->imageWidth + uStartX - frame->imageLPos - sPx;
			uIndex1	= uStartY * r_GifHandle->m_Width + uStartX;
#ifdef USE_ARGB8888	
		

			if ( 0 == r_GifHandle->m_CurFrameIndex )
						p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
			else
				{

			if (  frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] 		\
				|| frame->ctrlExt.trsFlag == 0)
			{
				p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
		

			}
			else
				{
				#if 0
				if(r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod == 2)
					{
					p_32DataFront[uIndex1] = myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx];
			}
				#endif
				
				}
			
			
				}
			#if 0
			if(r_GifHandle->p_BackupData != NULL)
			{
			//r_GifHandle->p_BackupData [uIndex1*4] = /*myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx]*/p_32DataFront[uIndex1] ;
			memcpy(&r_GifHandle->p_BackupData [uIndex1*4],&myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx]/*p_32DataFront[uIndex1]*/,4);
			}
		   #endif

#elif USE_ARGB1555	
			if ( 0 == r_GifHandle->m_CurFrameIndex )
						p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			else
			{

			if (  frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] 		\
				|| frame->ctrlExt.trsFlag == 0)
			{
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			}
			}
#elif USE_ARGB4444	
			if ( 0 == r_GifHandle->m_CurFrameIndex )
						p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			else
			{

			if (  frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] 		\
				|| frame->ctrlExt.trsFlag == 0)
			  {
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			   }
			}			
			
#endif
		}
	}
	
	src_rect.nX = /*0*/StartX;
	src_rect.nY = /*0*/StartX;
	src_rect.nWidth = r_GifHandle->m_Width;
	src_rect.nHeight = r_GifHandle->m_Height;
	dest_rect.nX =/* r_GifHandle->m_StartX;*/StartX;
	dest_rect.nY =/*  r_GifHandle->m_StartY;*/StartY;
	dest_rect.nHeight = r_GifHandle->u_Height;
	dest_rect.nWidth = r_GifHandle->u_Width;
#ifdef USE_ARGB8888	
//J_OSD_BlockCopyEx(&src_rect, &dest_rect, (void*)p_32DataFront, 0, NULL, NULL);/**/
memcpy((U8 *)ch_this->scrBuffer.dwHandle,(U8 *)p_32DataFront, r_GifHandle->m_Width*r_GifHandle->u_Height*4);

#elif USE_ARGB1555	
	/*J_OSD_BlockCopyEx(&src_rect, &dest_rect, (void*)p_16DataFront, 0, NULL, NULL);*/
	
memcpy((U8 *)ch_this->scrBuffer.dwHandle,(U8 *)p_16DataFront, r_GifHandle->m_Width*r_GifHandle->u_Height*2);
#elif USE_ARGB4444	
	
memcpy((U8 *)ch_this->scrBuffer.dwHandle,(U8 *)p_16DataFront, r_GifHandle->m_Width*r_GifHandle->u_Height*2);

#endif

#ifdef ST_5105
	CH6_AreaUpdate( r_GifHandle->m_StartX, r_GifHandle->m_StartY, r_GifHandle->m_Width, r_GifHandle->m_Height );
#endif

	uMiniSec = frame->ctrlExt.delayTime;

	if ( 0 == uMiniSec )
	{
		uMiniSec = 5;
	}

	/*  保持上一帧图像的序号 */
	r_GifHandle->m_PreFrameIndex = r_GifHandle->m_CurFrameIndex;

	/* 更新下一帧的序号 */
	if ( r_GifHandle->m_CurFrameIndex == r_GifHandle->m_TotalFrame - 1 )
		r_GifHandle->m_CurFrameIndex = 0;
	else
		r_GifHandle->m_CurFrameIndex ++;

	switch ( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
	{
	case 0:	/* 什么都不做 */
		break;
	case 1:	/* 不处置图形，把图形从当前位置移去 */
		break;
	case 3:	/* 恢复到先前状态 */
#ifdef USE_ARGB8888	

if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
	memcpy ( (r_GifHandle->p_MethodData),/* (r_GifHandle->p_DrawData)*/p_32DataFront, r_GifHandle->m_Width*r_GifHandle->m_Height*4);

#elif USE_ARGB1555	
		if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
			memcpy ( (r_GifHandle->p_MethodData), /*(r_GifHandle->p_DrawData)*/p_16DataFront, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
#elif USE_ARGB4444	
				if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
					memcpy ( (r_GifHandle->p_MethodData), /*(r_GifHandle->p_DrawData)*/p_16DataFront, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

#endif		
		break;
	case 2:	/* 回复到背景色 */
		#if 0
		{
		int i;
		int j;
		for (i=0;i<r_GifHandle->m_Width;i++)
			{
			for(j=0;j<r_GifHandle->m_Height;j++)
				{
                        if(r_GifHandle->p_BackupData != NULL)
                        	{
							if(r_GifHandle->p_BackupData [i*r_GifHandle->m_Height+j] == myx_32_pal[r_GifHandle->p_GlobeInfo->BKColorIdx])
								{

								}

                        	}
				}
			}

		}
		#endif
		break;
	default:
		break;
	}

	#ifdef WINDOWS_DEMO
	ReleaseDC ( g_Hwnd, hdc );
	#endif
	/*check_memStatus("mgif_ShowNextFrame end");*/
	return uMiniSec;
}
#if 1
/*
 * Func: mgif_ShowNext
 * Desc: 显示下一帧图像
 * return:该帧要延迟的毫秒数
 */
int mgif_ShowNext ( mgif_handle r_GifHandle )
{
	LPCFRAME		frame;
	#ifdef WINDOWS_DEMO
	HDC				hdc;
	#endif
	unsigned char	*pPalette = NULL;
	unsigned int	sPx, sPy;
	unsigned int	uStartX, uStartY;
	unsigned int	uIndex, uIndex1;
	unsigned int	uMiniSec;
	unsigned int	uLoop;
	unsigned short* p_16DataFront;
	#ifdef USE_ARGB8888	

unsigned short* p_32DataFront;

#endif
	int			iFirst = true;

	sPx = 0;
	sPy = 0;

	/*
	do_report ( 0, "\ngif_handle->0x%x, %d, %d", r_GifHandle, ru_StartX, ru_StartY );
	*/
	if ( NULL == r_GifHandle )
		return 50;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"r_GifHandle->m_PreFrameIndex=%x\n",r_GifHandle->m_PreFrameIndex);
	do_report(0,"r_GifHandle->m_CurFrameIndex=%x\n",r_GifHandle->m_CurFrameIndex);*/ 
	#endif

	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		#ifndef No_GIF_Back
		#if 0
		gif_data->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, 
																r_StartX, r_StartY, 
																gi->scrWidth, gi->scrHeight );
		#else  /*00_GIF*/
		#if 1
		if ( NULL == r_GifHandle->p_BackupData )
		{
#ifdef USE_ARGB8888
			r_GifHandle->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,
																	r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
																	r_GifHandle->m_Width, r_GifHandle->m_Height );
#elif USE_ARGB1555	
			r_GifHandle->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,
																	r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
																	r_GifHandle->m_Width, r_GifHandle->m_Height );
#elif USE_ARGB4444	
			r_GifHandle->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,
																	r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
																	r_GifHandle->m_Width, r_GifHandle->m_Height );
#endif
			iFirst = false;
		}
		#endif
		#endif
		#endif

		#if 1
		if ( NULL == r_GifHandle->p_MethodData )
			r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

		if ( r_GifHandle->p_BackupData && false == iFirst )
		{
			if ( r_GifHandle->p_DrawData ) 
				memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
			if ( r_GifHandle->p_MethodData ) 
				memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		}
		#endif
	}

	/* 如果只有一帧图像 */
	if ( r_GifHandle->m_PreFrameIndex == r_GifHandle->m_CurFrameIndex )
	{
		uMiniSec = r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].m_DelayTime;

		if ( 0 == uMiniSec )
		{
			uMiniSec = 50;
		}

		return uMiniSec;
	}

	/* 得到下一帧的指针 */
	frame = &(r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].p_Frame);

	/* 如果帧指针为空 */
	if ( NULL == frame )
	{
		return;
	}

	#ifdef WINDOWS_DEMO
	hdc = GetDC( g_Hwnd );
	#endif

	if( frame->lFlag == 0 )
	{
		pPalette	= r_GifHandle->p_PaletteData;
	}
	else
	{
		pPalette	= frame->pColorTable;
	}

	/* 画图片*/
	for ( uLoop = 0; uLoop < 256; uLoop ++ )
	{
#ifdef USE_ARGB8888	
		
		
		myx_32_pal[uLoop]  = 0xFF000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);
		
#else	
	
		myx_16_pal[uLoop] = 0x0f000 | ( ( pPalette[uLoop*3] & 0x0f0 ) << 4 ) | \
							( pPalette[uLoop*3+1] & 0x0f0 ) | ( ( pPalette[uLoop*3+2] & 0x0f0 ) >> 4 );
#endif
	}
#ifdef USE_ARGB8888	
	p_32DataFront = (U32*)(r_GifHandle->p_DrawData);
	
#else

	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);
#endif
	/* 根据上一帧图片的方法处置上一幅图片 */
	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		if ( -1 != r_GifHandle->m_PreFrameIndex )
		{
			/* 按照处置方法进行处置 */
			switch( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
			{
			case 0:	/* 什么都不做 */
				break;
			case 1:	/* 不处置图形，把图形从当前位置移去 */
			case 3:	/* 恢复到先前状态 */				
				if ( p_16DataFront && r_GifHandle->p_MethodData )
					memcpy ( p_16DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			case 2:	/* 回复到背景色 */
				if ( p_16DataFront && r_GifHandle->p_BackupData )
					memcpy ( p_16DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			default:
				break;
			}
		}
	}

	for( uStartY = frame->imageTPos+sPy; uStartY < frame->imageHeight + frame->imageTPos + sPy; uStartY ++ )
	{
		for( uStartX = frame->imageLPos+ sPx; uStartX < frame->imageWidth + frame->imageLPos + sPx; uStartX ++ )
		{
			uIndex = ( uStartY - frame->imageTPos - sPy ) * frame->imageWidth + uStartX - frame->imageLPos - sPx;
			uIndex1	= uStartY * r_GifHandle->m_Width + uStartX;

			if ( 0 == r_GifHandle->m_CurFrameIndex )
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			else
				if ( ( r_GifHandle->p_GlobeInfo->BKColorIdx != frame->dataBuf[uIndex] || frame->ctrlExt.disposalMethod == 0 ) &&
					 frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] )
				{
					p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
				}
		}
	}

	/* 根据上一帧图片的方法处置上一幅图片 */
	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		if ( -1 != r_GifHandle->m_PreFrameIndex )
		{
			/* 按照处置方法进行处置 */
			switch( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
			{
			case 0:	/* 什么都不做 */
				break;
			case 1:	/* 不处置图形，把图形从当前位置移去 */
			case 3:	/* 恢复到先前状态 */	
				if ( NULL != r_GifHandle->p_MethodData )
#ifdef USE_ARGB8888
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_MethodData );
#elif USE_ARGB1555	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_MethodData );
#elif USE_ARGB4444	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_MethodData );

#endif
				break;
			case 2:	/* 回复到背景色 */
				/*if ( 0 == r_GifHandle->m_CurFrameIndex )*/
				if ( NULL != r_GifHandle->p_BackupData )
#ifdef USE_ARGB8888
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_BackupData );
#elif USE_ARGB1555	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_BackupData );
#elif USE_ARGB4444	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_BackupData );

#endif
				break;
			default:
				break;
			}
		}
	}

#ifdef USE_ARGB8888
	CH6_PutRectangle(  GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
	#if 0
						ru_StartX + frame->imageLPos, ru_StartY + frame->imageTPos, 
						frame->imageWidth, frame->imageHeight,
	#else
						r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
						r_GifHandle->m_Width, r_GifHandle->m_Height,
	#endif
						(U8*)p_16DataFront );
#elif USE_ARGB1555	
	CH6_PutRectangle(  GFXHandle,CH6VPOSD.ViewPortHandle, &gGC,

						r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
						r_GifHandle->m_Width, r_GifHandle->m_Height,
						(U8*)p_16DataFront );
#elif USE_ARGB4444	
	CH6_PutRectangle(  GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,

						r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
						r_GifHandle->m_Width, r_GifHandle->m_Height,
	
						(U8*)p_16DataFront );
#endif
#ifdef ST_5105
	CH6_AreaUpdate( r_GifHandle->m_StartX, r_GifHandle->m_StartY, r_GifHandle->m_Width, r_GifHandle->m_Height );
#endif

	uMiniSec = frame->ctrlExt.delayTime;

	if ( 0 == uMiniSec )
	{
		uMiniSec = 50;
	}

	/*  保持上一帧图像的序号 */
	r_GifHandle->m_PreFrameIndex = r_GifHandle->m_CurFrameIndex;

	/* 更新下一帧的序号 */
	if ( r_GifHandle->m_CurFrameIndex == r_GifHandle->m_TotalFrame - 1 )
		r_GifHandle->m_CurFrameIndex = 0;
	else
		r_GifHandle->m_CurFrameIndex ++;

	switch ( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
	{
	case 0:	/* 什么都不做 */
		break;
	case 1:	/* 不处置图形，把图形从当前位置移去 */
	case 3:	/* 恢复到先前状态 */	
		if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
			memcpy ( (r_GifHandle->p_MethodData), (r_GifHandle->p_DrawData), r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		break;
	case 2:	/* 回复到背景色 */
		break;
	default:
		break;
	}

	#ifdef WINDOWS_DEMO
	ReleaseDC ( g_Hwnd, hdc );
	#endif

	return uMiniSec;
}
#endif

#if 1
/*
 * Func: mgif_ShowNextXY
 * Desc: 显示下一帧图像
 * return:该帧要延迟的毫秒数
 */
int mgif_ShowNextXY( mgif_handle r_GifHandle, U32 ru_StartX, U32 ru_StartY )
{
	LPCFRAME		frame;
	#ifdef WINDOWS_DEMO
	HDC				hdc;
	#endif
	unsigned char	*pPalette = NULL;
	unsigned int	sPx, sPy;
	unsigned int	uStartX, uStartY;
	unsigned int	uIndex, uIndex1;
	unsigned int	uMiniSec;
	unsigned int	uLoop;
	unsigned short* p_16DataFront;
	
#ifdef USE_ARGB8888	

unsigned short* p_32DataFront;

#endif
   int				iFirst = true;

	sPx = 0;
	sPy = 0;

	/*
	do_report ( 0, "\ngif_handle->0x%x, %d, %d", r_GifHandle, ru_StartX, ru_StartY );
	*/
	if ( NULL == r_GifHandle )
		return 50;

	#ifdef YY_GIF_SHOW
	/*do_report(0,"r_GifHandle->m_PreFrameIndex=%x\n",r_GifHandle->m_PreFrameIndex);
	do_report(0,"r_GifHandle->m_CurFrameIndex=%x\n",r_GifHandle->m_CurFrameIndex);*/ 
	#endif

	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		#ifndef No_GIF_Back
		#if 0
		gif_data->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, 
																r_StartX, r_StartY, 
																gi->scrWidth, gi->scrHeight );
		#else  /*00_GIF*/
		#if 1
		if ( NULL == r_GifHandle->p_BackupData )
		{
#ifdef USE_ARGB8888
			r_GifHandle->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,
																	r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
																	r_GifHandle->m_Width, r_GifHandle->m_Height );
#elif USE_ARGB1555	
			r_GifHandle->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
																	r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
																	r_GifHandle->m_Width, r_GifHandle->m_Height );
#elif USE_ARGB4444	
			r_GifHandle->p_BackupData = (unsigned char*)CH6_GetRectangle( GFXHandle, CH6VPOSD.ViewPortHandle,&gGC,
																	r_GifHandle->m_StartX, r_GifHandle->m_StartY, 
																	r_GifHandle->m_Width, r_GifHandle->m_Height );

#endif
			iFirst = false;
		}
		#endif
		#endif
		#endif

		#if 1
		if ( NULL == r_GifHandle->p_MethodData )
			r_GifHandle->p_MethodData = (unsigned char*)mgif_malloc ( r_GifHandle->m_Width*r_GifHandle->m_Height*2 );

		if ( r_GifHandle->p_BackupData && false == iFirst )
		{
			if ( r_GifHandle->p_DrawData ) 
				memcpy( r_GifHandle->p_DrawData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
			if ( r_GifHandle->p_MethodData ) 
				memcpy( r_GifHandle->p_MethodData, r_GifHandle->p_BackupData, r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		}
		#endif
	}

	/* 如果只有一帧图像 */
	if ( r_GifHandle->m_PreFrameIndex == r_GifHandle->m_CurFrameIndex )
	{
		uMiniSec = r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].m_DelayTime;

		if ( 0 == uMiniSec )
		{
			uMiniSec = 50;
		}

		return uMiniSec;
	}

	/* 得到下一帧的指针 */
	frame = &(r_GifHandle->m_FrameInfo[r_GifHandle->m_CurFrameIndex].p_Frame);

	/* 如果帧指针为空 */
	if ( NULL == frame )
	{
		return;
	}

	#ifdef WINDOWS_DEMO
	hdc = GetDC( g_Hwnd );
	#endif

	if( frame->lFlag == 0 )
	{
		pPalette	= r_GifHandle->p_PaletteData;
	}
	else
	{
		pPalette	= frame->pColorTable;
	}

	/* 画图片*/
	for ( uLoop = 0; uLoop < 256; uLoop ++ )
	{
#ifdef USE_ARGB8888	
		
		
		myx_32_pal[uLoop]  = 0xFF000000|(( pPalette[uLoop*3]<<16)&0x00FF0000 )|(( pPalette[uLoop*3+1] <<8)&0x0000FF00)|(pPalette[uLoop*3+2]&0x000000FF);
		
#else	
	
		myx_16_pal[uLoop] = 0x0f000 | ( ( pPalette[uLoop*3] & 0x0f0 ) << 4 ) | \
							( pPalette[uLoop*3+1] & 0x0f0 ) | ( ( pPalette[uLoop*3+2] & 0x0f0 ) >> 4 );
#endif
	}
#ifdef USE_ARGB8888	
	p_32DataFront = (U32*)(r_GifHandle->p_DrawData);
	
#else

	p_16DataFront = (U16*)(r_GifHandle->p_DrawData);
#endif
	/* 根据上一帧图片的方法处置上一幅图片 */
	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		if ( -1 != r_GifHandle->m_PreFrameIndex )
		{
			/* 按照处置方法进行处置 */
			switch( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
			{
			case 0:	/* 什么都不做 */
				break;
			case 1:	/* 不处置图形，把图形从当前位置移去 */
			case 3:	/* 恢复到先前状态 */
				if ( p_16DataFront && r_GifHandle->p_MethodData )
					memcpy ( p_16DataFront, r_GifHandle->p_MethodData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			case 2:	/* 回复到背景色 */
				if ( p_16DataFront && r_GifHandle->p_BackupData )
					memcpy ( p_16DataFront, r_GifHandle->p_BackupData, r_GifHandle->m_Height * r_GifHandle->m_Width * 2 );
				break;
			default:
				break;
			}
		}
	}

	for( uStartY = frame->imageTPos+sPy; uStartY < frame->imageHeight + frame->imageTPos + sPy; uStartY ++ )
	{
		for( uStartX = frame->imageLPos+ sPx; uStartX < frame->imageWidth + frame->imageLPos + sPx; uStartX ++ )
		{
			uIndex = ( uStartY - frame->imageTPos - sPy ) * frame->imageWidth + uStartX - frame->imageLPos - sPx;
			uIndex1	= uStartY * r_GifHandle->m_Width + uStartX;
#ifdef USE_ARGB8888	

if ( 0 == r_GifHandle->m_CurFrameIndex )
			p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
		else
			if ( ( r_GifHandle->p_GlobeInfo->BKColorIdx != frame->dataBuf[uIndex] || frame->ctrlExt.disposalMethod == 0 ) &&
				 frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] )
			{
				p_32DataFront[uIndex1] = myx_32_pal[frame->dataBuf[uIndex]];
			}

#else
			if ( 0 == r_GifHandle->m_CurFrameIndex )
				p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
			else
				if ( ( r_GifHandle->p_GlobeInfo->BKColorIdx != frame->dataBuf[uIndex] || frame->ctrlExt.disposalMethod == 0 ) &&
					 frame->ctrlExt.trsColorIndex != frame->dataBuf[uIndex] )
				{
					p_16DataFront[uIndex1] = myx_16_pal[frame->dataBuf[uIndex]];
				}
#endif				
		}
	}

	/* 根据上一帧图片的方法处置上一幅图片 */
	if ( r_GifHandle->m_TotalFrame > 1 )
	{
		if ( -1 != r_GifHandle->m_PreFrameIndex )
		{
			/* 按照处置方法进行处置 */
			switch( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
			{
			case 0:	/* 什么都不做 */
				break;
			case 1:	/* 不处置图形，把图形从当前位置移去 */
			case 3:	/* 恢复到先前状态 */
				if ( NULL != r_GifHandle->p_MethodData )
#ifdef USE_ARGB8888
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										ru_StartX, ru_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_MethodData );
#elif USE_ARGB1555	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										ru_StartX, ru_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_MethodData );
#elif USE_ARGB4444	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										ru_StartX, ru_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_MethodData );

#endif
				break;
			case 2:	/* 回复到背景色 */
				/*if ( 0 == r_GifHandle->m_CurFrameIndex )*/
				if ( NULL != r_GifHandle->p_BackupData )
#ifdef USE_ARGB8888
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										ru_StartX, ru_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_BackupData );
#elif USE_ARGB1555	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										ru_StartX, ru_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_BackupData );
#elif USE_ARGB4444	
					CH6_PutRectangle( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC,
										ru_StartX, ru_StartY, 
										r_GifHandle->m_Width, r_GifHandle->m_Height, 
										r_GifHandle->p_BackupData );

#endif
				break;
			default:
				break;
			}
		}
	}

#ifdef USE_ARGB8888
	CH6_PutRectangle(  GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,
	#if 0
						ru_StartX + frame->imageLPos, ru_StartY + frame->imageTPos, 
						frame->imageWidth, frame->imageHeight,
	#else
						ru_StartX, ru_StartY, 
						r_GifHandle->m_Width, r_GifHandle->m_Height,
	#endif
						(U8*)p_16DataFront );
#elif USE_ARGB1555	
	CH6_PutRectangle(  GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,

						ru_StartX, ru_StartY, 
						r_GifHandle->m_Width, r_GifHandle->m_Height,
						(U8*)p_16DataFront );
#elif USE_ARGB4444	
	CH6_PutRectangle(  GFXHandle,CH6VPOSD.ViewPortHandle,&gGC,	

						ru_StartX, ru_StartY, 
						r_GifHandle->m_Width, r_GifHandle->m_Height,
						(U8*)p_16DataFront );
#endif

#ifdef ST_5105
	CH6_AreaUpdate( r_GifHandle->m_StartX, r_GifHandle->m_StartY, r_GifHandle->m_Width, r_GifHandle->m_Height );
#endif

	uMiniSec = frame->ctrlExt.delayTime;

	if ( 0 == uMiniSec )
	{
		uMiniSec = 50;
	}

	/*  保持上一帧图像的序号 */
	r_GifHandle->m_PreFrameIndex = r_GifHandle->m_CurFrameIndex;

	/* 更新下一帧的序号 */
	if ( r_GifHandle->m_CurFrameIndex == r_GifHandle->m_TotalFrame - 1 )
		r_GifHandle->m_CurFrameIndex = 0;
	else
		r_GifHandle->m_CurFrameIndex ++;

	switch ( r_GifHandle->m_FrameInfo[r_GifHandle->m_PreFrameIndex].m_HandleMethod )
	{
	case 0:	/* 什么都不做 */
		break;
	case 1:	/* 不处置图形，把图形从当前位置移去 */
	case 3:	/* 恢复到先前状态 */
		if ( (r_GifHandle->p_DrawData) && r_GifHandle->p_MethodData )
			memcpy ( (r_GifHandle->p_MethodData), (r_GifHandle->p_DrawData), r_GifHandle->m_Width*r_GifHandle->m_Height*2 );
		break;
	case 2:	/* 回复到背景色 */
		break;
	default:
		break;
	}

	#ifdef WINDOWS_DEMO
	ReleaseDC ( g_Hwnd, hdc );
	#endif

	return uMiniSec;
}
#endif

#if 0
void mgif_test1()
{
	U8* pData 		= NULL;
	U32	u_DataLen;
	FILE* 			fp;
	mgif_handle		gif_handle = NULL;
	int				i_KeyRead;
	int				i_WaitTime = 50;
	static int first =  0;
	if(first == 0)
	{
		u_DataLen 	= 43722;
		fp = fopen ( "c:\\gifshu.gif", "rb" );
		first = 0;
	}
	else
	{
		u_DataLen 	= 52739;
		fp = fopen ( "c:\\gif6.gif", "rb" );
		first = 0;
	}
	if ( NULL == fp )
	{
		return;
	}

	/*check_memStatus ( "null" );*/

	pData = memory_allocate( SystemPartition, u_DataLen );

	if ( NULL == pData )
	{
		return;
	}

	fread ( pData, 1, u_DataLen, fp );

	gif_handle = (mgif_handle)mgif_Init( pData, u_DataLen, 200, 200 );

	while ( 1 )
	{
		i_KeyRead = eis_readKey( ST_GetClocksPerSecond() * i_WaitTime / 100 );

		if ( 27 == i_KeyRead )
		{
			break;
		}

		i_WaitTime = mgif_ShowNextXY ( gif_handle, 200, 200 );
	}
	J_OSD_ClearScreen();
	mgif_Term ( gif_handle );

	memory_deallocate ( SystemPartition, pData );

	/*check_memStatus ( "null" );*/

	pData 		= NULL;
	gif_handle	= NULL;

	fclose ( fp );

	return;
}
#endif

/*
  * Func: chz_ShowADGif
  * Desc: 显示广告图片
  * In:	rp_Data	-> 数据地址
  		ru_Size	-> 数据大小
  		ru_StartX-> 左上角位置
  		ru_StartY-> 
  		ru_NoUse
  */
void chz_ShowADGif( U8* rp_Data, U32 ru_Size, U16 ru_StartX, U16 ru_StartY, U8 ru_Nouse )
{
	mgif_handle gif_handle;
	/*
	FILE *fp;

	fp = fopen ( "c:\\1.gif", "wb" );

	if ( fp )
	{
		fwrite ( rp_Data,  1, ru_Size, fp );
	}

	fclose ( fp );

	return;*/

	gif_handle = mgif_Init ( rp_Data, ru_Size, ru_StartX, ru_StartY );

	mgif_ShowNextXY ( gif_handle, ru_StartX, ru_StartY );

	mgif_Term( gif_handle );

	return;
}

/*
获得GIF广告图片的大小
rp_Data， ru_Size  图片数据地址及数据长度
ru_Width， ru_Height 输出大小
*/

void chz_GetADGifSize( U8* rp_Data, U32 ru_Size, U16* ru_Width, U16* ru_Height)
{
	U16 w, h;

	*ru_Width = 0;
	*ru_Height = 0;
	
	if(rp_Data == NULL || ru_Size < 10)
	{
		return;
	}
	w = (rp_Data[7] << 8) + rp_Data[6];
	h = (rp_Data[9] << 8) + rp_Data[8];
	
	if( w > 0)
	{
		*ru_Width = w;
	}
	if( h > 0)
	{
		*ru_Height = h;
	}
}

