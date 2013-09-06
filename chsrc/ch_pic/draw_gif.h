

#ifndef __mgif__
#define __mgif__
#include "stlite.h"
#include "pub_st.h"
#define ICHG(c1,c2)	c2*256+c1

#define boolea boolean
/*#define WORD long  /*00_GIF*/
#define LPCSTR char*

/*#define No_GIF_Back  00_060808*/



typedef struct
{	
	boolea 	active;					/* 扩展控制块是否有效 */
	U32 		disposalMethod;			/* 处置方法 */
	boolea 	userInputFlag;			/* 用户输入标志 */
	boolea 	trsFlag;					/* 透明色标志，置位表示使用透明色 */
	WORD 	delayTime;				/* 该视图的延迟时间 */
	U32 		trsColorIndex;			/* 透明色索引 */
}GCTRLEXT;

typedef struct
{
	WORD 	imageLPos;				/* 视图X方向偏移量 		*/
	WORD 	imageTPos;				/* 视图Y方向偏移量 		*/
	WORD 	imageWidth;				/* 视图宽度 					*/
	WORD 	imageHeight;			/* 视图高度 					*/
	boolea 	lFlag;					/* 局部色表标记 			*/
	boolea 	interlaceFlag;			/* 交织标记 					*/
	boolea 	sortFlag;				/* 分类标记，如果置位表示局部色表分类排列 */
	U32 		lSize;					/* 局部色表深度 			*/
	U8 		*pColorTable;			/* 指向句柄色表的指针 	*/
	U8 		*dataBuf;				/* 指向数据的指针 		*/
	GCTRLEXT ctrlExt;				/* 扩展控制块 				*/
}FRAME;

typedef FRAME *LPFRAME;
typedef const  FRAME *LPCFRAME;

typedef struct
{	
	U32 		frames;					/* 包含视图个数 */
	WORD 	scrWidth, scrHeight;		/* 逻辑宽度和高度 */
	boolea 	gFlag;					/* 全局色表标志 */
	U32 		colorRes;				/* 颜色深度 */
	boolea 	gSort;					/* 全局色表是否要分类排列 */
	U32 		gSize;					/* 全局色表大小 */
	U32 		BKColorIdx;				/* 背景色索引 */
	U32 		pixelAspectRatio;		/* 象素比 */
	U8 		*gColorTable;			/* 色表数据指针 */
}GLOBAL_INFO;

typedef GLOBAL_INFO *LPGLOBAL_INFO;
typedef const GLOBAL_INFO *LPCGLOBAL_INFO;

/* 字符表入口 */
typedef struct
{	
	U32 				len;			/* 长度 */
	unsigned char* 	p;			/* 数据指针 */
}STRING_TABLE_ENTRY;

/*
 * 结构: 每帧图像的信息
 */
typedef struct mgif_frame_info_s{
	unsigned int	m_OffsetX;		/* x方向偏移	*/
	unsigned int	m_OffsetY;		/* y方向偏移	*/
	unsigned char	m_HandleMethod;	/* 处置方法		*/
	unsigned char	m_DelayTime;	/* 延迟时间		*/
	unsigned char	m_LocalPalette;	/* 是否使用局部调色板 */
	FRAME		p_Frame;		/* 视图指针 */
}mgif_frame_info_t;

/*
 * 结构: 每个gif的信息
 */
typedef struct m_gif_file_info_s{
	unsigned char*		p_ActData;		/* 实际数据指针			*/
	unsigned char*		p_BackupData;	/* 背景数据指针			*/
	unsigned char*		p_DrawData;	/* 画板数据指针			*/
	unsigned char*		p_PaletteData;	/* 色表数据指针 			*/
	unsigned char*		p_MethodData;	/* 用于处置方法的指针  */
	unsigned int		m_StartX;		/* 画图起始X */
	unsigned int		m_StartY;		/* 画图起始Y */
	unsigned int		m_Size;			/* 文件大小				*/
	unsigned int		m_Width;		/* 图像宽度				*/
	unsigned int		m_Height;		/* 图像高度				*/
	unsigned int 		u_Width;		/*显示区域的高度和宽度*/
	unsigned int 		u_Height;
	unsigned char		m_TotalFrame;	/* 共有多少帧图像		*/
	unsigned char		m_CurFrameIndex;/* 当前图像帧的序号		*/
	char				m_PreFrameIndex;/* 上一帧图像的序号 */
	char				m_PrevPrevIndex; /* 上一帧的上一帧状态 */
	int 			b_enalbe;/*缓存动画时的有效标志*/
	mgif_frame_info_t	m_FrameInfo[100];/* 每帧图像的信息数组	*/
	LPCGLOBAL_INFO	p_GlobeInfo;		/* 全局指针 */
}mgif_info_t, *mgif_handle;

extern	mgif_handle gifhandle;

#endif


extern void chz_ShowADGif( U8* rp_Data, U32 ru_Size, U16 ru_StartX, U16 ru_StartY, U8 ru_Nouse );
extern void chz_GetADGifSize( U8* rp_Data, U32 ru_Size, U16* ru_Width, U16* ru_Height);
extern void* mgif_malloc ( unsigned int size );
extern 	void mgif_DestroyHandle ( mgif_handle r_GifHandle );
//extern  STGFX_Handle_t GFXHandle;
/*--eof----------------------------------------------------------------------------*/

