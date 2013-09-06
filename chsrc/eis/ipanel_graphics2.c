#include "stddefs.h"
#include "eis_include\ipanel_graphics2.h"
#include "stblit.h"
#include "stlayer.h"
#include "..\main\initterm.h"
#include "ipanel_typedef.h"
#include "..\dbase\vdbase.h"
#include "../stgfx/src/gfx/stgfx_init.h"

//#define EIS_DEBUG_2D


#define 	INPUT_WIDTH		1280
#define 	INPUT_HEIGHT		720
#define   COLOR_BYTES		4	/*ARGB4444,占两个字节，如果是RGB8888，占4个字节*/
#define   MAX_SURFACE_NUM	64	/*the max total number of surfaces*/
extern IPANEL_GRAPHICS_WIN_RECT   g_IpanelDiaplyRect;
extern int	act_src_w,act_src_h;

#ifdef SUMA_SECURITY
extern semaphore_t *pst_OsdSema;/* 用于IPNAEL的显示控制*/
#endif

extern CH_RESOLUTION_MODE_e HighSolution_change;

extern STLAYER_Handle_t 		LAYERHandle[7];
extern STGXOBJ_Bitmap_t		g_OSDBitmap_HighSolution[2];	  
/*高清OSD菜单OSD菜单*/
static void get_default_blit_params(STBLIT_BlitContext_t *BLIT_Context);
static INT32_T  eis_check_rect_params(IPANEL_GRAPHICS_RECT *rect,STGXOBJ_Bitmap_t *pGXOBJ_Bitmap);
static void eis_BLT_params_translation(IPANEL_GRAPHICS_BLT_PARAM *param,STBLIT_BlitContext_t * p_BLIT_Context);

/*图层 */
typedef struct eis_surface_s
{
	STGXOBJ_Bitmap_t OSDBitmap;
	boolean bOccupy;
} eis_surface_t;

eis_surface_t				eis_surface[MAX_SURFACE_NUM];/*用于 记录新建图层*/
STGXOBJ_Bitmap_t			eis_OSDBitmap_HighSolution[2];	             /*用于 中转的切换*/
STAVMEM_BlockHandle_t	eis_surface_BlockHandle[MAX_SURFACE_NUM+2+1]; /*用于 记录中转加新建图层的HANDEL*/

static unsigned char m_surfaces_num = 0;/* 逻辑图层个数 */
static INT32_T is_init = 0;/*模块是否初始化*/

STGXOBJ_Bitmap_t *g_pGXOBJ_Bitmap;
STGXOBJ_Bitmap_t   SRC_GXOBJ_Bitmap ;



static STEVT_Handle_t          BlitEvtHandle;/*blit 事件句柄*/
static semaphore_t* 		 gpSemBlitComplete = NULL;/* sqzow20100526*/
static STBLIT_Handle_t         gBlitHandle = NULL;


/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_BlitComplete_Callback
    Purpose: blit完成通知函数
    Reference: 
    in:
        Event >> 
        EventData >> 
        Reason >> 
        RegistrantName >> 
        SubscriberData_p >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/26 13:32:52 
---------------------------------函数体定义----------------------------------------*/
void CH_BlitComplete_Callback(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    ST_ErrorCode_t ErrCode;
	switch (Event)
    {
        case STBLIT_BLIT_COMPLETED_EVT:
			/*if(ErrCode==ST_NO_ERROR) */
			if(gpSemBlitComplete)
			{
				semaphore_signal(gpSemBlitComplete);
			}
            break;
        default:
            break;
    }
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:CH_WaitBlitComplete
    Purpose: 等待blit结束
    Reference: 
    in:
        void >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/26 13:39:15 
---------------------------------函数体定义----------------------------------------*/
int CH_WaitBlitComplete(void)
{
	clock_t wait_time;
	int ret = 0;
	
	if(gpSemBlitComplete)
	{
		 wait_time = time_plus(time_now(), 1 * ST_GetClocksPerSecondLow());
		 ret = semaphore_wait_timeout(gpSemBlitComplete, &wait_time);
	}
	if(ret != 0)
	{
		sttbx_Print("wait blit timeout !!\n");
	}
	return ret;
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:EIS_2D_InitEvent
    Purpose: 初始化blit事件
    Reference: 
    in:
        void >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/28 16:42:18 
---------------------------------函数体定义----------------------------------------*/
ST_ErrorCode_t EIS_2D_InitEvent(void)
{
	STEVT_SubscriberID_t SubscriberID;
	ST_ErrorCode_t          Err;
	
	if(gpSemBlitComplete == NULL)
	{
		STEVT_DeviceSubscribeParams_t EvtSubscribeParams;
		STEVT_OpenParams_t      EvtOpenParams;

		gBlitHandle = stgfx_GetBlitterHandle(GFXHandle);
		/* Open Event handler */
		Err = STEVT_Open(EVTDeviceName, &EvtOpenParams, &BlitEvtHandle);
		if (Err != ST_NO_ERROR)
		{
			STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Event Open : %d\n",Err));
			return (TRUE);
		}
		EvtSubscribeParams.NotifyCallback     = CH_BlitComplete_Callback;
		EvtSubscribeParams.SubscriberData_p   = NULL;
		Err = STEVT_SubscribeDeviceEvent(BlitEvtHandle,BLITDeviceName,STBLIT_BLIT_COMPLETED_EVT,&EvtSubscribeParams);
		if (Err != ST_NO_ERROR)
		{
			return (TRUE);
		}	
#ifndef ST_OS21
	    gpSemBlitComplete=semaphore_init_fifo_timeout(0);
#else
	    gpSemBlitComplete=semaphore_create_fifo(0);
#endif	
	}
}




BOOL EIS_SetColorValue(STGXOBJ_Color_t* pColor,unsigned int Value)
{
	STGXOBJ_Color_t* pColorTemp;
	U8* p1;
	U8 Temp[4];
	pColorTemp=pColor;

	p1=(U8*)&Value;

	Temp[0]=*p1;
	Temp[1]=*(p1+1);
	Temp[2]=*(p1+2);
	Temp[3]=*(p1+3);

	switch(pColorTemp->Type)
	{
		case STGXOBJ_COLOR_TYPE_CLUT8:

			pColorTemp->Value.CLUT8=(U8)Temp[0];
			break;
		case STGXOBJ_COLOR_TYPE_ARGB1555:

			pColorTemp->Value.ARGB1555.Alpha=Temp[1]>>7;
		
			pColorTemp->Value.ARGB1555.R=(Temp[1]&0x7c)>>2;

			pColorTemp->Value.ARGB1555.G=((Temp[1]&0x03)<<3)+((Temp[0]&0xe0)>>5);

			pColorTemp->Value.ARGB1555.B=(Temp[0]&0x1f);
			break;
		case STGXOBJ_COLOR_TYPE_ARGB4444:
			pColorTemp->Value.ARGB4444.Alpha=Temp[1]>>4;
			pColorTemp->Value.ARGB4444.R=Temp[1]&0x0f;
			pColorTemp->Value.ARGB4444.G=(Temp[0]&0xf0)>>4;
			pColorTemp->Value.ARGB4444.B=Temp[0]&0x0f;
			pColorTemp->Value.ARGB4444.Alpha=Temp[1]>>4;
			break;
		case STGXOBJ_COLOR_TYPE_RGB565:

			pColorTemp->Value.RGB565.R=(Temp[1]&0xf1)>>3;
			pColorTemp->Value.RGB565.G=((Temp[1]&0x07)<<3)+((Temp[0]&0xe0)>>5);
			pColorTemp->Value.RGB565.B=Temp[0]&0x1f;
			break;
		
		case STGXOBJ_COLOR_TYPE_ARGB8888:
		case STGXOBJ_COLOR_TYPE_ARGB8888_255:
			pColorTemp->Value.ARGB8888.Alpha=Temp[3];
			pColorTemp->Value.ARGB8888.R=Temp[2];
			pColorTemp->Value.ARGB8888.G=Temp[1];
			pColorTemp->Value.ARGB8888.B=Temp[0];
			break;
	
		
		default:
			STTBX_Print(("\n YxlInfo(A):unsupported color type"));
			return TRUE;

	}
	return FALSE;
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:EIS_2D_GetContent
    Purpose: 获得content参数初始化
    Reference: 
    in:
        BlitContext_p >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/28 16:44:47 
---------------------------------函数体定义----------------------------------------*/
ST_ErrorCode_t EIS_2D_GetContent(STBLIT_BlitContext_t* BlitContext_p)
{
	STEVT_SubscriberID_t            SubscriberID;

	memset(BlitContext_p, 0, sizeof(STBLIT_BlitContext_t));
	EIS_2D_InitEvent();
	STEVT_GetSubscriberID(BlitEvtHandle,&SubscriberID);
	BlitContext_p->EventSubscriberID   = SubscriberID;
	BlitContext_p->ColorKeyCopyMode        = STBLIT_COLOR_KEY_MODE_NONE;
	/* Context.ColorKey              = dummy; */
	BlitContext_p->AluMode                 = STBLIT_ALU_COPY;
	BlitContext_p->EnableMaskWord          = FALSE;
	/*    Context.MaskWord                = 0xFFFFFF00;*/
	BlitContext_p->EnableMaskBitmap        = FALSE;
	/*    Context.MaskBitmap_p            = NULL;*/
	BlitContext_p->EnableColorCorrection   = FALSE;
	/*    Context.ColorCorrectionTable_p  = NULL;*/
	BlitContext_p->Trigger.EnableTrigger   = FALSE;
	BlitContext_p->GlobalAlpha             = 0xff;
	BlitContext_p->EnableClipRectangle     = /*TRUE*/FALSE;
	/*Context.ClipRectangle           = Rectangle;*/
	BlitContext_p->WriteInsideClipRectangle = FALSE;
	BlitContext_p->EnableFlickerFilter     = FALSE;



	BlitContext_p->JobHandle               = NULL;
	/*    Context.JobHandle               = JobHandle[0];*/
	BlitContext_p->NotifyBlitCompletion    = TRUE;

	return FALSE;
}


/*函数名:     boolean CH_AllocBitmap_ChangeHighSolution(int ri_LayerIndex,STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
  *开发人员:zengxianggen
  *开发时间:2007/08/24
  *函数功能:分配显示空间
  *函数算法:
  *调用的全局变量和结构:
  *调用的主要函数:无
  *返回值说明:无
  *参数表:无                                           */
boolean EIS_Creat_Surface(int ri_LayerIndex,STLAYER_Handle_t LayerHandle,STGXOBJ_Bitmap_t *Chbmp,int BMPWidth,int BMPHeight)
 {
	 
	 ST_ErrorCode_t ErrCode;
	 STGXOBJ_BitmapAllocParams_t   BitmapParams1, BitmapParams2;
	 STAVMEM_AllocBlockParams_t    AllocParams;
	 
	 STGXOBJ_Bitmap_t* pBMP;
	 int i;
	 pBMP=Chbmp;
	 
	 
	 pBMP->PreMultipliedColor=FALSE;
	 pBMP->ColorSpaceConversion=STGXOBJ_ITU_R_BT601;
	 pBMP->AspectRatio=STGXOBJ_ASPECT_RATIO_4TO3;
	 pBMP->Width=BMPWidth;
	 pBMP->Height=BMPHeight;
	 pBMP->Pitch=0;
	 pBMP->Offset=0;
	 pBMP->Data1_p=NULL;
	 pBMP->Size1=0;
	 pBMP->Data2_p=NULL;
	 pBMP->Size2=0;
	 pBMP->SubByteFormat=STGXOBJ_SUBBYTE_FORMAT_RPIX_MSB;
	 pBMP->BigNotLittle=FALSE;
	 pBMP->ColorType=STGXOBJ_COLOR_TYPE_ARGB8888_255/*STGXOBJ_COLOR_TYPE_ARGB8888 sqzow20100602*/;
	 pBMP->BitmapType=STGXOBJ_BITMAP_TYPE_RASTER_PROGRESSIVE;
	 
	 
	 ErrCode=STLAYER_GetBitmapAllocParams(LayerHandle,pBMP,&BitmapParams1,&BitmapParams2);
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STLAYER_GetBitmapAllocParams()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 AllocParams.PartitionHandle= AVMEMHandle[1];
	 
	 AllocParams.Size=BitmapParams1.AllocBlockParams.Size+BitmapParams2.AllocBlockParams.Size;
	 
	 AllocParams.Alignment=BitmapParams1.AllocBlockParams.Alignment;
	 AllocParams.AllocMode= STAVMEM_ALLOC_MODE_BOTTOM_TOP;
	 AllocParams.NumberOfForbiddenRanges=0;
	 AllocParams.ForbiddenRangeArray_p=(STAVMEM_MemoryRange_t*)NULL;
	 AllocParams.NumberOfForbiddenBorders=0;
	 AllocParams.ForbiddenBorderArray_p=NULL;
	 
	 
	 pBMP->Pitch=BitmapParams1.Pitch;
	 pBMP->Offset=BitmapParams1.Offset;
	 pBMP->Size1=BitmapParams1.AllocBlockParams.Size;
	 
	 pBMP->Size2=BitmapParams2.AllocBlockParams.Size;
	 
	 
	 ErrCode=STAVMEM_AllocBlock((const STAVMEM_AllocBlockParams_t*)&AllocParams,
		 (STAVMEM_BlockHandle_t *)&eis_surface_BlockHandle[ri_LayerIndex]); 
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_AllocBlock()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 ErrCode=STAVMEM_GetBlockAddress(eis_surface_BlockHandle[ri_LayerIndex], (void **)&(pBMP->Data1_p));
	 if(ErrCode!=ST_NO_ERROR)
	 {
		 STTBX_Print(("STAVMEM_GetBlockAddress()=%s",GetErrorText(ErrCode)));
		 return TRUE;
	 }
	 
	 if(pBMP->BitmapType==STGXOBJ_BITMAP_TYPE_RASTER_TOP_BOTTOM)
	 {
		 pBMP->Data2_p=(U8*)pBMP->Data1_p + pBMP->Size1;
	 }
	 
	 
	 
	 if(pBMP->ColorType==STGXOBJ_COLOR_TYPE_CLUT8)
	 {
		 memset( pBMP->Data1_p, 0x0, pBMP->Size1);
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0, pBMP->Size2);
	 }
	 else
	 {
		 memset( pBMP->Data1_p, 0x00, pBMP->Size1);
		 /*		memset( pBMP->Data1_p, 0xAA ,pBMP->Size1);*/
		 
		 if(pBMP->Data2_p!=NULL) 
			 memset( pBMP->Data2_p, 0x0f, pBMP->Size2);
		 i=0;
		 do
		 {
			 *((U8*)pBMP->Data1_p+i+1)=0x0f;
			 if(pBMP->Data2_p!=NULL) *((U8*)pBMP->Data2_p+i)=0x0f;
			 
			 i+=2;
			 
		 }while(i<=pBMP->Size1);
	 }				
	 return FALSE;		
}
 

 boolean EIS_2D_CopyArea( STGXOBJ_Bitmap_t *   Src_BMPT,STGXOBJ_Bitmap_t   * Dst_BMPT,
 	IPANEL_GRAPHICS_RECT *srcRect,IPANEL_GRAPHICS_RECT *dstRect,STBLIT_BlitContext_t* BlitContext_p)
 {
 #if 0/* sqzow20100528*/
 	EIS_2D_GetContent(BlitContext_p);
 	EIS_2D_CopyBmpt( Src_BMPT, 
		Dst_BMPT,
	 	srcRect,
	 	dstRect,
	 	BlitContext_p);
	if((&g_OSDBitmap_HighSolution[1])==(Dst_BMPT))
	{
		return EIS_2D_CopyBmpt( Src_BMPT, 
			&g_OSDBitmap_HighSolution[0],
		 	srcRect,
		 	dstRect,
		 	BlitContext_p);
	}
	else if((&g_OSDBitmap_HighSolution[0])==(Dst_BMPT))
	{
		return EIS_2D_CopyBmpt( Src_BMPT, 
			&g_OSDBitmap_HighSolution[1],
		 	srcRect,
		 	dstRect,
		 	BlitContext_p);
	}
 	return 0;
 #else
	int i_ErrCode;
	STGXOBJ_Rectangle_t Src_Rect;
	STGXOBJ_Rectangle_t Dst_Rect;
	if(Src_BMPT == NULL || Dst_BMPT == NULL )
	{
		return true;
	}
	  
	Src_Rect.PositionX=srcRect->x;
	Src_Rect.PositionY=srcRect->y;
	Src_Rect.Width=srcRect->width;
	Src_Rect.Height=srcRect->height;
	
	Dst_Rect.PositionX=dstRect->x;
	Dst_Rect.PositionY=dstRect->y;
	Dst_Rect.Width=dstRect->width;
	Dst_Rect.Height=dstRect->height;
	
     /*首先更新高清OSD*/
	if( ((&g_OSDBitmap_HighSolution[1])==(Dst_BMPT))||((&g_OSDBitmap_HighSolution[0])==(Dst_BMPT)))
	{
		EIS_STGFX_CopyArea(GFXHandle,Src_BMPT,&g_OSDBitmap_HighSolution[0],&gGC,&Src_Rect, &Dst_Rect,BlitContext_p);
		STGFX_Sync(GFXHandle);
		/*再次更新标清OSD*/
		EIS_STGFX_CopyArea(GFXHandle,Src_BMPT,&g_OSDBitmap_HighSolution[1],&gGC,&Src_Rect, &Dst_Rect,BlitContext_p);
		STGFX_Sync(GFXHandle);
	}
	else
	{
		EIS_STGFX_CopyArea(GFXHandle,Src_BMPT,Dst_BMPT,&gGC,&Src_Rect, &Dst_Rect,BlitContext_p);
	}
     return false;
#endif	  
 }

 boolean EIS_2D_DrawRectangle( STGXOBJ_Bitmap_t   * Dst_BMPT,IPANEL_GRAPHICS_RECT *dstRect,int color)
 {
      int i_ErrCode;
	  STGXOBJ_Rectangle_t Dst_Rect;
	  STGFX_GC_t  *RGC_P;
	  STGFX_GC_t gGC_temp;

      if( Dst_BMPT == NULL )
      	{
           return true;
      	}
	if( ((&g_OSDBitmap_HighSolution[1])==(Dst_BMPT))||((&g_OSDBitmap_HighSolution[0])==(Dst_BMPT)))
	{
	  RGC_P=&gGC;
	}
	else
	{
	memcpy(&gGC_temp,&gGC,sizeof(STGFX_GC_t));
	RGC_P=&gGC_temp;
	}
	  
	if(color!=-1)
		RGC_P->EnableFilling= TRUE;
	else
		RGC_P->EnableFilling= false;
	
	RGC_P->LineWidth= 1;
	
	RGC_P->FillColor.Type=Dst_BMPT->ColorType;
	RGC_P->DrawColor.Type=Dst_BMPT->ColorType;
	EIS_SetColorValue(&(RGC_P->FillColor),color);
	EIS_SetColorValue(&(RGC_P->DrawColor),color);
	  
	Dst_Rect.PositionX=dstRect->x;
	Dst_Rect.PositionY=dstRect->y;
	Dst_Rect.Width=dstRect->width;
	Dst_Rect.Height=dstRect->height;
	EIS_SetColorValue(&(RGC_P->FillColor),color);
	
	if( ((&g_OSDBitmap_HighSolution[1])==(Dst_BMPT))||((&g_OSDBitmap_HighSolution[0])==(Dst_BMPT)))
	{
		 STGFX_DrawRectangle(GFXHandle,&g_OSDBitmap_HighSolution[0],RGC_P,Dst_Rect,0);
		 STGFX_Sync(GFXHandle);
	 	 STGFX_DrawRectangle(GFXHandle,&g_OSDBitmap_HighSolution[1],RGC_P,Dst_Rect,0);
		 STGFX_Sync(GFXHandle);
	}
	else
	{
		 STGFX_DrawRectangle(GFXHandle,Dst_BMPT,RGC_P,Dst_Rect,0);
	}
	
     return false;
	  
 }

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:EIS_2D_CopyWithRgbColorKey
    Purpose: ColorKey模式拷贝
    Reference: 
    in:
        ColorKey >> 
        dstRect >> 
        Dst_BMPT >> 
        srcRect >> 
        Src_BMPT >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/28 16:45:51 
---------------------------------函数体定义----------------------------------------*/
ST_ErrorCode_t EIS_2D_CopyWithRgbColorKey_2Dst( STGXOBJ_Bitmap_t *Src_BMPT, STGXOBJ_Bitmap_t   *Dst_BMPT,
 	IPANEL_GRAPHICS_RECT *srcRect,IPANEL_GRAPHICS_RECT *dstRect, U32 ColorKey)
{
#if 1 /*软件实现 sqzow20100531*/

	U32 i, j, *src_32, *dst_32, w, h;

	w = dstRect->width > srcRect->width ? srcRect->width : dstRect->width;
	h = dstRect->height > srcRect->height ? srcRect->height : dstRect->height;
	src_32 = (U32*)((U8*)Src_BMPT->Data1_p + Src_BMPT->Pitch * srcRect->y + srcRect->x * 4);
	dst_32 = (U32*)((U8*)Dst_BMPT->Data1_p + Dst_BMPT->Pitch * dstRect->y + dstRect->x * 4);
	ColorKey &= 0x00ffffff;
	for(i=0; i < h; i++)
	{
		for(j=0; j < w; j++)
		{
			if(src_32[j] & 0x00ffffff == ColorKey)
			{
				 dst_32[j] = 0;
			}
			/*else
			{
				dst_32[j] = src_32[j];
			} sqzow20100610*/
		}
		src_32 += Src_BMPT->Width;
		dst_32 += Dst_BMPT->Width;
	}
	
#else/*硬件实现*/
	STBLIT_BlitContext_t BlitContext_p;
	
	EIS_2D_GetContent(&BlitContext_p);
	BlitContext_p.ColorKey.Type = STGXOBJ_COLOR_KEY_TYPE_RGB888;
	BlitContext_p.ColorKey.Value.RGB888.RMin    = (ColorKey >> 16) & 0xff ;
	BlitContext_p.ColorKey.Value.RGB888.RMax    = (ColorKey >> 16) & 0xff;
	BlitContext_p.ColorKey.Value.RGB888.ROut    = 0;
	BlitContext_p.ColorKey.Value.RGB888.REnable = 1;
	BlitContext_p.ColorKey.Value.RGB888.GMin    = (ColorKey >> 8) & 0xff;
	BlitContext_p.ColorKey.Value.RGB888.GMax    = (ColorKey >> 8) & 0xff;
	BlitContext_p.ColorKey.Value.RGB888.GOut    = 0;
	BlitContext_p.ColorKey.Value.RGB888.GEnable = 1;
	BlitContext_p.ColorKey.Value.RGB888.BMin    = (ColorKey) & 0xff;
	BlitContext_p.ColorKey.Value.RGB888.BMax    = (ColorKey) & 0xff;
	BlitContext_p.ColorKey.Value.RGB888.BOut    = 0;
	BlitContext_p.ColorKey.Value.RGB888.BEnable = 1;
	BlitContext_p.ColorKeyCopyMode = STBLIT_COLOR_KEY_MODE_SRC;
	return EIS_2D_CopyBmpt( Src_BMPT, Dst_BMPT, srcRect,dstRect, &BlitContext_p);
#endif
}



/*-------------------------------函数定义注释头--------------------------------------
    FuncName:EIS_2D_CopyWithAlphaColorKey_2Dst
    Purpose: 实现color key-alpha到桌面
    Reference: 
    in:
        ColorKey >> 
        dstRect >> 
        Dst_BMPT >> 
        srcRect >> 
        Src_BMPT >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/31 17:01:06 
---------------------------------函数体定义----------------------------------------*/
ST_ErrorCode_t EIS_2D_CopyWithAlphaColorKey_2Dst( STGXOBJ_Bitmap_t *Src_BMPT, STGXOBJ_Bitmap_t   *Dst_BMPT,
 	IPANEL_GRAPHICS_RECT *srcRect,IPANEL_GRAPHICS_RECT *dstRect, U32 ColorKey)
{
	U32 i, j, *src_32, *dst_32, w, h;

	w = dstRect->width > srcRect->width ? srcRect->width : dstRect->width;
	h = dstRect->height > srcRect->height ? srcRect->height : dstRect->height;

	src_32 = (U32*)((U8*)Src_BMPT->Data1_p + Src_BMPT->Pitch * srcRect->y + srcRect->x * 4);
	dst_32 = (U32*)((U8*)Dst_BMPT->Data1_p + Dst_BMPT->Pitch * dstRect->y + dstRect->x * 4);
	ColorKey &= 0xff000000;
	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			if((src_32[j] & 0xff000000) == ColorKey)
			{
				 dst_32[j] = 0;
			}
			/*else
			{
				dst_32[j] = src_32[j];
			} sqzow20100610*/
		}
		src_32 += Src_BMPT->Width;
		dst_32 += Dst_BMPT->Width;
		task_delay(10);
	}
	return 0;
}



/*-------------------------------函数定义注释头--------------------------------------
    FuncName:EIS_2D_CopyWithArgbColorKey_2Dst
    Purpose: 实现color key-argb到桌面
    Reference: 
    in:
        ColorKey >> 
        dstRect >> 
        Dst_BMPT >> 
        srcRect >> 
        Src_BMPT >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/05/31 17:01:48 
---------------------------------函数体定义----------------------------------------*/
ST_ErrorCode_t EIS_2D_CopyWithArgbColorKey_2Dst(STGXOBJ_Bitmap_t *Src_BMPT, STGXOBJ_Bitmap_t   *Dst_BMPT,
 	IPANEL_GRAPHICS_RECT *srcRect,IPANEL_GRAPHICS_RECT *dstRect, U32 ColorKey)
{
	U32 i, j, *src_32, *dst_32, w, h;

	ColorKey &= 0xffffffff;
	w = dstRect->width > srcRect->width ? srcRect->width : dstRect->width;
	h = dstRect->height > srcRect->height ? srcRect->height : dstRect->height;

	src_32 = (U32*)((U8*)Src_BMPT->Data1_p + Src_BMPT->Pitch * srcRect->y + srcRect->x * 4);
	dst_32 = (U32*)((U8*)Dst_BMPT->Data1_p + Dst_BMPT->Pitch * dstRect->y + dstRect->x * 4);

	for(i=0; i<h; i++)
	{
		for(j=0; j<w; j++)
		{
			if((src_32[j] ) == ColorKey)
			{
				 dst_32[j] = 0;
			}
			/*else
			{
				dst_32[j] = src_32[j];
			} sqzow20100610*/
		}
		src_32 += Src_BMPT->Width;
		dst_32 += Dst_BMPT->Width;
		task_delay(100); 
	}
	return 0;
}







static  INT32_T  ipanel_graphics_get_blitparams(STBLIT_BlitContext_t *BLIT_Context)
{
	STEVT_SubscriberID_t            SubscriberID;
	if(!BLIT_Context)
		goto ERR_EXIT;
		
	BLIT_Context->ColorKeyCopyMode         = STBLIT_COLOR_KEY_MODE_NONE;
	BLIT_Context->AluMode                  = STBLIT_ALU_COPY;
	BLIT_Context->EnableMaskWord           = FALSE;
	BLIT_Context->MaskWord                 = 0;
	BLIT_Context->EnableMaskBitmap         = FALSE;
	BLIT_Context->MaskBitmap_p             = NULL;
	BLIT_Context->MaskRectangle.PositionX  = 0;
	BLIT_Context->MaskRectangle.PositionY  = 0;
	BLIT_Context->MaskRectangle.Width      = 0;
	BLIT_Context->MaskRectangle.Height     = 0;
	BLIT_Context->WorkBuffer_p             = NULL;
	BLIT_Context->EnableColorCorrection    = FALSE;
	BLIT_Context->ColorCorrectionTable_p   = NULL;	
	BLIT_Context->Trigger.EnableTrigger    = FALSE;
	BLIT_Context->Trigger.ScanLine         = 0;
	BLIT_Context->Trigger.Type             = STBLIT_TRIGGER_TYPE_BOTTOM_VSYNC;
	BLIT_Context->Trigger.VTGId            = 0;
	BLIT_Context->GlobalAlpha              = 1;
	BLIT_Context->EnableClipRectangle      = FALSE;
	BLIT_Context->ClipRectangle.PositionX  = 0;
	BLIT_Context->ClipRectangle.PositionY  = 0;
	BLIT_Context->ClipRectangle.Width      = 0;
	BLIT_Context->ClipRectangle.Height     = 0;
	BLIT_Context->WriteInsideClipRectangle = TRUE;
	BLIT_Context->EnableFlickerFilter      = FALSE;
	BLIT_Context->JobHandle                = STBLIT_NO_JOB_HANDLE;
	BLIT_Context->UserTag_p                = NULL;
	BLIT_Context->NotifyBlitCompletion 	   = TRUE;
	EIS_2D_InitEvent();
	STEVT_GetSubscriberID(BlitEvtHandle,&SubscriberID);
	BLIT_Context->EventSubscriberID   = SubscriberID;
	return IPANEL_OK;
	
ERR_EXIT:
	return IPANEL_ERR;
}


/*
	ALPHA + rgb Colorkey ，暂不同时支持缩放
*/
static INT32_T	 ipanel_graphics_hdblt_alphargb(STGXOBJ_Bitmap_t *pSrcBitmap, IPANEL_GRAPHICS_RECT* srcRect,
								STGXOBJ_Bitmap_t *pDstBitmap,IPANEL_GRAPHICS_RECT* dstRect,unsigned int alpha,unsigned int colorkey)
{
	STBLIT_Source_t	 	BlitSource = {0};
	STBLIT_Destination_t	BlitDestination = {0};
	STBLIT_BlitContext_t	 m_ipanel_gfx_blitcontext;
	ST_ErrorCode_t Err;
	
	ipanel_graphics_get_blitparams(&m_ipanel_gfx_blitcontext);
	m_ipanel_gfx_blitcontext.AluMode = STBLIT_ALU_ALPHA_BLEND;
	m_ipanel_gfx_blitcontext.GlobalAlpha = alpha * 128 / 100;

	m_ipanel_gfx_blitcontext.ColorKeyCopyMode 				= STBLIT_COLOR_KEY_MODE_SRC;
	m_ipanel_gfx_blitcontext.ColorKey.Type 					= STGXOBJ_COLOR_KEY_TYPE_RGB888;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.RMin 	= (colorkey>>16)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.RMax 	= (colorkey>>16)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.ROut 	= FALSE; 
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.REnable 	= TRUE;	
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GMin 	= (colorkey>>8)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GMax 	= (colorkey>>8)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GOut 	= FALSE; 
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GEnable 	= TRUE;	
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BMin 	= (colorkey)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BMax 	= (colorkey)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BOut 	= FALSE;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BEnable 	= TRUE;

	BlitSource.Type = STBLIT_SOURCE_TYPE_BITMAP;
	BlitSource.Data.Bitmap_p =  pSrcBitmap;
	BlitSource.Rectangle.PositionX = srcRect->x;
	BlitSource.Rectangle.PositionY = 	srcRect->y;
	BlitSource.Rectangle.Width = srcRect->width;
	BlitSource.Rectangle.Height = srcRect->height;
	
	BlitDestination.Bitmap_p = pDstBitmap;
	BlitDestination.Rectangle.PositionX = dstRect->x;
	BlitDestination.Rectangle.PositionY = dstRect->y;
	BlitDestination.Rectangle.Width =dstRect->width;
	BlitDestination.Rectangle.Height =  dstRect->height;	

	Err = STBLIT_Blit(gBlitHandle,NULL,&BlitSource,&BlitDestination, &m_ipanel_gfx_blitcontext );
	if (Err != ST_NO_ERROR)
	{
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
		return (TRUE);
	}
	Err = CH_WaitBlitComplete();
	return Err;
	
ERR_EXIT:
	return IPANEL_ERR;
}

static INT32_T ipanel_graphics_hdblt_zoom(STGXOBJ_Bitmap_t *pSrcBitmap, IPANEL_GRAPHICS_RECT* srcRect,
								STGXOBJ_Bitmap_t *pDstBitmap,IPANEL_GRAPHICS_RECT* dstRect)
{
	STBLIT_Source_t	 	BlitSource = {0};
	STBLIT_Destination_t	BlitDestination = {0};
	STBLIT_BlitContext_t	 m_ipanel_gfx_blitcontext;
	ST_ErrorCode_t Err;
	
	ipanel_graphics_get_blitparams(&m_ipanel_gfx_blitcontext);
	BlitSource.Type = STBLIT_SOURCE_TYPE_BITMAP;
	BlitSource.Data.Bitmap_p =  pSrcBitmap;
	BlitSource.Rectangle.PositionX = srcRect->x;
	BlitSource.Rectangle.PositionY = 	srcRect->y;
	BlitSource.Rectangle.Width = srcRect->width;
	BlitSource.Rectangle.Height = srcRect->height;
	
	BlitDestination.Bitmap_p = pDstBitmap;
	BlitDestination.Rectangle.PositionX = dstRect->x;
	BlitDestination.Rectangle.PositionY = dstRect->y;
	BlitDestination.Rectangle.Width =dstRect->width;
	BlitDestination.Rectangle.Height =  dstRect->height;	
	
	Err = STBLIT_Blit(gBlitHandle,NULL,&BlitSource,&BlitDestination, &m_ipanel_gfx_blitcontext );
	if (Err != ST_NO_ERROR)
	{
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
		return (TRUE);
	}
	Err = CH_WaitBlitComplete();
	return Err;
	
	return IPANEL_OK;
ERR_EXIT:
	return IPANEL_ERR;
}

static INT32_T	 ipanel_graphics_swblt_srcpiexl(void* srcbuf,unsigned int srcLineWidth, IPANEL_GRAPHICS_RECT * srcRect, 
							void * dstbuf, unsigned int dstLineWidth, IPANEL_GRAPHICS_RECT * dstRect)
{
	unsigned int 	 xpos = 0,ypos = 0;
	unsigned int    *srcptr = (unsigned int *)srcbuf +  srcRect->y*srcLineWidth + srcRect->x;
	unsigned int    *dstptr = (unsigned int *)dstbuf +  dstRect->y*dstLineWidth + dstRect->x;

	if(srcRect->height != dstRect->height || srcRect->width != dstRect->width)
	{
#ifdef EIS_DEBUG_2D
		eis_report("[ ipanel_graphics_swblt_alphacolorkey ] ERROR: Invalid Paramters ! \n");
#endif
		return IPANEL_ERR;
	}
	
	for(ypos = 0; ypos< srcRect->height; ypos++)
	{
		for(xpos = 0; xpos < srcRect->width; xpos++)
		{
			unsigned int    src= srcptr[xpos];
			unsigned int    dst= dstptr[xpos];
			unsigned int    sa=(src>>24)&0xff;
			unsigned int 	sr,sg,sb,dr,dg,db;
			if(((dst >> 24) & 0xff ) != 0 )
			{
				sr=(src>>16)&0xff;
				sg=(src>> 8)&0xff;
				sb=(src ) & 0xff;
				dr=(dst>>16)&0xff;
				dg=(dst>> 8)&0xff;
				db=(dst    )&0xff;			
				dr=(sr*sa+dr*(0xff-sa))/0xff;
				dg=(sg*sa+dg*(0xff-sa))/0xff;
				db=(sb*sa+db*(0xff-sa))/0xff;
				srcptr[xpos]=((dr<<16)+(dg<<8)+db)|(dst&0xff000000);
			}
		}//end for
		srcptr += srcLineWidth;
		dstptr += dstLineWidth;	
	}
	return IPANEL_OK;
}


static INT32_T	 ipanel_graphics_swblt_dstpiexl(void* srcbuf,unsigned int srcLineWidth, IPANEL_GRAPHICS_RECT * srcRect, 
							void * dstbuf, unsigned int dstLineWidth, IPANEL_GRAPHICS_RECT * dstRect)
{
	unsigned int 	 xpos = 0,ypos = 0;
	unsigned int    *srcptr = (unsigned int *)srcbuf +  srcRect->y*srcLineWidth + srcRect->x;
	unsigned int    *dstptr = (unsigned int *)dstbuf +  dstRect->y*dstLineWidth + dstRect->x;

	if(srcRect->height != dstRect->height || srcRect->width != dstRect->width)
	{
#ifdef EIS_DEBUG_2D
		eis_report("[ ipanel_graphics_swblt_piexlalpha ] ERROR: Invalid Paramters ! \n");
#endif
		return IPANEL_ERR;
	}
	
	for(ypos = 0; ypos< srcRect->height; ypos++)
	{
		for(xpos = 0; xpos < srcRect->width; xpos++)
		{
			unsigned int    src= srcptr[xpos];
			unsigned int    dst= dstptr[xpos];
			unsigned int    sa=(src>>24)&0xff;
			unsigned int 	sr,sg,sb,dr,dg,db;
			
			if( ( (dst >> 24) & 0xff) == 0)
			{
				dstptr[xpos] = src;
			}
			else
			{
				sr=(src>>16)&0xff;
				sg=(src>> 8)&0xff;
				sb=(src ) & 0xff;
				dr=(dst>>16)&0xff;	
				dg=(dst>> 8)&0xff;
				db=(dst    )&0xff;		
				dr=(sr*sa+dr*(0xff-sa))/0xff;
				dg=(sg*sa+dg*(0xff-sa))/0xff;
				db=(sb*sa+db*(0xff-sa))/0xff;
				dstptr[xpos]=((dr<<16)+(dg<<8)+db)|(dst&0xff000000);	
			}		
		}
		srcptr += srcLineWidth;
		dstptr += dstLineWidth;	
	}
	
	return IPANEL_OK;
}

static INT32_T	 ipanel_graphics_hdblt_alpha(STGXOBJ_Bitmap_t *pSrcBitmap, IPANEL_GRAPHICS_RECT* srcRect,
								STGXOBJ_Bitmap_t *pDstBitmap,IPANEL_GRAPHICS_RECT* dstRect,unsigned int alpha)
{
	STBLIT_Source_t	 	BlitSource = {0};
	STBLIT_Destination_t	BlitDestination = {0};
	STBLIT_BlitContext_t	 m_ipanel_gfx_blitcontext;
	ST_ErrorCode_t Err;
	
	ipanel_graphics_get_blitparams(&m_ipanel_gfx_blitcontext);

	m_ipanel_gfx_blitcontext.AluMode = STBLIT_ALU_ALPHA_BLEND;
	m_ipanel_gfx_blitcontext.GlobalAlpha = alpha*128/100;
	
	BlitSource.Type = STBLIT_SOURCE_TYPE_BITMAP;
	BlitSource.Data.Bitmap_p =  pSrcBitmap;
	BlitSource.Rectangle.PositionX = srcRect->x;
	BlitSource.Rectangle.PositionY = 	srcRect->y;
	BlitSource.Rectangle.Width = srcRect->width;
	BlitSource.Rectangle.Height = srcRect->height;
	BlitDestination.Bitmap_p = pDstBitmap;
	BlitDestination.Rectangle.PositionX = dstRect->x;
	BlitDestination.Rectangle.PositionY = dstRect->y;
	BlitDestination.Rectangle.Width =	dstRect->width;
	BlitDestination.Rectangle.Height =  dstRect->height;	

	Err = STBLIT_Blit(gBlitHandle,NULL,&BlitSource,&BlitDestination, &m_ipanel_gfx_blitcontext );
	if (Err != ST_NO_ERROR)
	{
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
		return (TRUE);
	}
	Err = CH_WaitBlitComplete();
	return Err;
ERR_EXIT:
	return IPANEL_ERR;
}

static INT32_T	 ipanel_graphics_hdblt_rgbcolorkey(STGXOBJ_Bitmap_t *pSrcBitmap, IPANEL_GRAPHICS_RECT* srcRect,
								STGXOBJ_Bitmap_t *pDstBitmap,IPANEL_GRAPHICS_RECT* dstRect,unsigned int colorkey)
{
	STBLIT_Source_t	 	BlitSource = {0};
	STBLIT_Destination_t	BlitDestination = {0};
	STBLIT_BlitContext_t	 m_ipanel_gfx_blitcontext;
	ST_ErrorCode_t Err;
	
	ipanel_graphics_get_blitparams(&m_ipanel_gfx_blitcontext);
	m_ipanel_gfx_blitcontext.ColorKeyCopyMode 				= STBLIT_COLOR_KEY_MODE_SRC;
	m_ipanel_gfx_blitcontext.ColorKey.Type 					= STGXOBJ_COLOR_KEY_TYPE_RGB888;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.RMin 	= (colorkey>>16)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.RMax 	= (colorkey>>16)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.ROut 	= FALSE; 
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.REnable 	= TRUE;	
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GMin 	= (colorkey>>8)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GMax 	= (colorkey>>8)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GOut 	= FALSE; 
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.GEnable 	= TRUE;	
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BMin 	= (colorkey)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BMax 	= (colorkey)&0xFF;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BOut 	= FALSE;
	m_ipanel_gfx_blitcontext.ColorKey.Value.RGB888.BEnable 	= TRUE;
	
	BlitSource.Type = STBLIT_SOURCE_TYPE_BITMAP;
	BlitSource.Data.Bitmap_p =  pSrcBitmap;
	BlitSource.Rectangle.PositionX = srcRect->x;
	BlitSource.Rectangle.PositionY = 	srcRect->y;
	BlitSource.Rectangle.Width = srcRect->width;
	BlitSource.Rectangle.Height = srcRect->height;
	BlitDestination.Bitmap_p = pDstBitmap;
	BlitDestination.Rectangle.PositionX = dstRect->x;
	BlitDestination.Rectangle.PositionY = dstRect->y;
	BlitDestination.Rectangle.Width =	dstRect->width;
	BlitDestination.Rectangle.Height =  dstRect->height;	

	
	Err = STBLIT_Blit(gBlitHandle,NULL,&BlitSource,&BlitDestination, &m_ipanel_gfx_blitcontext );
	if (Err != ST_NO_ERROR)
	{
		STTBX_Report ((STTBX_REPORT_LEVEL_INFO,"Err Blit : %d\n",Err));
		return (TRUE);
	}
	Err = CH_WaitBlitComplete();
	return Err;
ERR_EXIT:
	return IPANEL_ERR;
}




void ipanel_2d_init( void )
{
	g_IpanelDiaplyRect.x = 0;
	g_IpanelDiaplyRect.y = 0;
	g_IpanelDiaplyRect.w = INPUT_WIDTH;
	g_IpanelDiaplyRect.h=   INPUT_HEIGHT;

}
INT32_T ipanel_porting_graphics_init(void)
{
	ST_ErrorCode_t				error= ST_NO_ERROR;;
	STLAYER_ViewPortParams_t		LayerVpParams;
	int i;
	
#ifdef EIS_DEBUG_2D	
	eis_report("\n DEBUG 2D ipanel_porting_graphics_init");
#endif

	if (is_init) return IPANEL_OK;
	
	for(i=0;i<MAX_SURFACE_NUM; i++)
	{
		eis_surface[i].bOccupy=false;
	}
	
	LayerVpParams.InputRectangle.PositionX	= 0;				
	LayerVpParams.InputRectangle.PositionY	= 0;
	LayerVpParams.InputRectangle.Width		= INPUT_WIDTH;
	LayerVpParams.InputRectangle.Height 	= INPUT_HEIGHT;
	EIS_Creat_Surface(MAX_SURFACE_NUM,LAYERHandle[LAYER_GFX1],&eis_OSDBitmap_HighSolution[0],LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );
	EIS_Creat_Surface(1+MAX_SURFACE_NUM,LAYERHandle[LAYER_GFX1],&eis_OSDBitmap_HighSolution[1],LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );
	
	memcpy(&SRC_GXOBJ_Bitmap,&eis_OSDBitmap_HighSolution[0],sizeof(STGXOBJ_Bitmap_t));
	g_pGXOBJ_Bitmap=&eis_OSDBitmap_HighSolution[1];

  	is_init=true;
	return IPANEL_OK;
	
RETURN_ERR:
#ifdef EIS_DEBUG_2D
	eis_report("\n 2D DEBUG  ipanel_porting_graphics_init ERROR");
#endif
	return IPANEL_ERR;
	
}

INT32_T ipanel_porting_graphics_exit()
{
       STAVMEM_FreeBlockParams_t   st_FreeBlockParas;
	int i;
	if (!is_init) return IPANEL_OK;
	 st_FreeBlockParas.PartitionHandle = AVMEMHandle[1];
    if(eis_OSDBitmap_HighSolution[0].Data1_p != NULL)
    {
       STAVMEM_FreeBlock(&st_FreeBlockParas,&eis_surface_BlockHandle[MAX_SURFACE_NUM]);
    }
 if(eis_OSDBitmap_HighSolution[1].Data1_p != NULL)
    {
       STAVMEM_FreeBlock(&st_FreeBlockParas,&eis_surface_BlockHandle[MAX_SURFACE_NUM+1]);
    }

	return IPANEL_OK;
}

/*
*函数输入:	type		创建图层颜色类型
*			x 			x坐标
*			y 			y坐标
*			width 		宽度
*			height  	高度
*			phSurface 	图层句柄
*
*函数输出:	phSurface 	surface句柄
*
*功能描述:	创建逻辑图层: 初始状态为show
*
*函数返回:	成功 IPANEL_OK
*			失败 IPANEL_ERR
***/
/* 创建逻辑图层: surface创建后宽高不能再被改变直至destroy
*	width, height是surface的大小. 对于缩放或切入切出等特效，surface 的宽高 是图像变化过程中的宽高的最大值，不允许图像超出surface的边界
*  phSurface是surface句柄
*/
INT32_T ipanel_porting_graphics_create_surface( INT32_T type, INT32_T width, INT32_T height, VOID **phSurface )
{
	ST_ErrorCode_t ErrCode = ST_NO_ERROR;
	STLAYER_ViewPortParams_t		LayerVpParams;
	int i;
#ifdef EIS_DEBUG_2D	
	eis_report("\n DEBUG 2D ipanel_porting_graphics_create_surface");
#endif

	if(!(width>0) || !(height>0) )
	{
#ifdef EIS_DEBUG_2D
		eis_report("\n DEBUG 2D [ipanel_porting_graphics_create_surface] param error!,width=%d,height=%d\n",width,height);
#endif
		goto error;
	}
	
	if(  is_init == 0)
	{
#ifdef EIS_DEBUG_2D
		eis_report("\n DEBUG 2D [ipanel_porting_graphics_create_surface] error!not init ! %d\n", is_init);
#endif
		goto error;
	}

	if( m_surfaces_num >= MAX_SURFACE_NUM )
	{
#ifdef EIS_DEBUG_2D
		eis_report("\n DEBUG 2D [ipanel_porting_graphics_create_surface] error!Too much surfaces!\n");
#endif
		goto error;
	}
	
	for(i=0;i<MAX_SURFACE_NUM; i++)
	{
		if(eis_surface[i].bOccupy==false)
			break;
	}
	
	if(i>=MAX_SURFACE_NUM)
	{
		goto error;
	}
	
       /*设置对应的GFX LAYER和输出模式*/
	
	LayerVpParams.InputRectangle.PositionX	= 0;				
	LayerVpParams.InputRectangle.PositionY	= 0;
	LayerVpParams.InputRectangle.Width		= width;
	LayerVpParams.InputRectangle.Height 	= height;
	EIS_Creat_Surface(i,LAYERHandle[LAYER_GFX2],(STGXOBJ_Bitmap_t*)(&eis_surface[i]),LayerVpParams.InputRectangle.Width,LayerVpParams.InputRectangle.Height );
	eis_surface[i].bOccupy=true;
	*phSurface=(VOID *)(&(eis_surface[i].OSDBitmap));
	m_surfaces_num++;
#ifdef EIS_DEBUG_2D	
	eis_report("\n+++++++++++++  create_surface ok *phSurface= 0x%x m_surfaces_num =%d",*phSurface,m_surfaces_num);
#endif

	return IPANEL_OK;
	
	error:
	eis_report("error!\n");
	return IPANEL_ERR;
}
/*
*函数输入:	hSurface	ipanel_porting_graphics_create_surface获得的图层句柄
*函数输出:	释放hSurface使用的资源
*功能描述:	销毁逻辑图层
*函数返回:	成功 IPANEL_OK
*			失败 IPANEL_ERR
***/
INT32_T ipanel_porting_graphics_destroy_surface( void* hSurface )
{
       STAVMEM_FreeBlockParams_t   st_FreeBlockParas;
	 st_FreeBlockParas.PartitionHandle = AVMEMHandle[1];
	 int i;
#ifdef EIS_DEBUG_2D
	eis_report("\n  DEBUG 2D  ipanel_porting_graphics_destroy_surface hSurface= 0x%x *hSurface=%x",hSurface,hSurface ? *((int *)hSurface) : 0);
#endif

	for(i=0;i<MAX_SURFACE_NUM; i++)
	{
		if((unsigned int *)hSurface==(unsigned int *)(&(eis_surface[i].OSDBitmap)))
		{
				eis_surface[i].bOccupy=false;
			    if(eis_surface[i].OSDBitmap.Data1_p != NULL)
			    {
				   STAVMEM_FreeBlock(&st_FreeBlockParas,&eis_surface_BlockHandle[i]);
			    }
			m_surfaces_num--;
			break;
		}
	}
	
	if(i>=MAX_SURFACE_NUM)
	{
		eis_report("[ipanel_porting_graphics_destroy_surface] failed! 0x%x %d\n", (int)hSurface, m_surfaces_num);
	}
	else
	{
		#ifdef EIS_DEBUG_2D	
		eis_report("[ipanel_porting_graphics_destroy_surface] success! 0x%x %d\n", (int)hSurface, m_surfaces_num);
		#endif
	}
	return IPANEL_OK;
error:
	return IPANEL_ERR;
}

/*
*函数输入:	hSurface	ipanel_porting_graphics_create_surface获得的图层句柄
*			pbuffer 	显存
*			width 		宽度
*			height 		高度
*			bufWidth  	BUF宽度
*			bufHeight 	BUF高度
*			format 		图形颜色格式
*
*函数输出:	返回图层属性
*
*功能描述:	取得图层点阵缓冲区
*
*函数返回:	成功 IPANEL_OK
*			失败 IPANEL_ERR
***/
INT32_T ipanel_porting_graphics_get_surface_info( void* hSurface, void **pbuffer, INT32_T *width, INT32_T *height, 
							INT32_T *bufWidth, INT32_T *bufHeight, INT32_T *format )
{
	STGXOBJ_Bitmap_t *pGXOBJ_Bitmap = (STGXOBJ_Bitmap_t *)hSurface;
#ifdef EIS_DEBUG_2D	
	eis_report("\n  DEBUG 2D  ipanel_porting_graphics_get_surface_info hSurface= 0x%x",hSurface);
#endif

	if (!hSurface){
		
		*width = *bufWidth = INPUT_WIDTH/*g_IpanelDiaplyRect.w*/;
		*height = *bufHeight = INPUT_HEIGHT/*g_IpanelDiaplyRect.h*/;
		
			
		if (pbuffer)
		{
			*pbuffer = (void*)g_pGXOBJ_Bitmap->Data1_p;
		}
		*format = 4;
		#ifdef EIS_DEBUG_2D	
		eis_report("[ipanel_porting_graphics_get_surface_info]! 0x%x  width = %d; height = %d;\n",
			(int)hSurface,  *width, *height);
		#endif
	}
	else{
		*pbuffer = pGXOBJ_Bitmap->Data1_p;
		*width = *bufWidth = pGXOBJ_Bitmap->Width;
		*height = *bufHeight = pGXOBJ_Bitmap->Height;
		*format = pGXOBJ_Bitmap->ColorType;
		#ifdef EIS_DEBUG_2D	
		eis_report("[ipanel_porting_graphics_get_surface_info] 0x%x width = %d; height = %d; buf_width = %d; buf_height = %d; pbuf = 0x%p\n",
		                       (int)hSurface, *width, *height, *bufWidth, *bufHeight, *pbuffer);
		#endif
	}
	
	return IPANEL_OK;
}

/*
*函数输入:	hSurface	ipanel_porting_graphics_create_surface获得的图层句柄
*			x 			x坐标
*			y 			y坐标
*			width 		宽度
*			height  	高度
*			color 		ARGB_8888颜色
*
*函数输出:	取得逻辑图层的zorder
*
*功能描述:	在图层上填充矩形: color为ARGB_8888格式
*
*函数返回:	成功 IPANEL_OK
*			失败 IPANEL_ERR
***/
/* 在图层上填充矩形: color为平台支持的格式，x,y 是相对surface起点的位置 */
INT32_T ipanel_porting_graphics_fill_surface_rect( VOID* hSurface, IPANEL_GRAPHICS_RECT *rect, UINT32_T color )
{
	STGXOBJ_Bitmap_t *pGXOBJ_Bitmap 		= NULL;	
#ifdef EIS_DEBUG_2D	
	eis_report("\n DEBUG 2D ipanel_porting_graphics_fill_surface_rect hSurface=%x  rect:%d %d %d %d  color:%x\n",
		hSurface, rect->x, rect->y, rect->width, rect->height, color);
#endif

	if (!hSurface)
	{
		pGXOBJ_Bitmap=g_pGXOBJ_Bitmap;
	}
	else
	{
		pGXOBJ_Bitmap = (STGXOBJ_Bitmap_t *)hSurface;
	}
	
	if(eis_check_rect_params(rect,pGXOBJ_Bitmap)==IPANEL_ERR)
	{
#ifdef EIS_DEBUG_2D
		eis_report("[ipanel_porting_graphics_fill_surface_rect] param error!x=%d,y=%d,width=%d,height=%d,width1=%d,height1=%d\n",
		rect->x,rect->y,rect->width,rect->height,pGXOBJ_Bitmap->Width,pGXOBJ_Bitmap->Height);
#endif
		goto error;
	}
	
	EIS_2D_DrawRectangle(pGXOBJ_Bitmap,rect,color);
	
	return IPANEL_OK;
error:
	eis_report("error!\n");
	return IPANEL_ERR;
}



/* 图层之间拷贝(带stretch/alpha/colorkey功能)
*  hSurface 是目标图层，hSurface==NULL表示直接拷贝到OSD上
*  x,y,width,height目标图层的区域
*  hSrcSurface 是源图层
*  srcx,srcy,srcWidth,srcHeight是srcSurface上的区域
*/
INT32_T ipanel_porting_graphics_stretch_surface( VOID* hSurface, IPANEL_GRAPHICS_RECT* dstRect,
						VOID *hSrcSurface, IPANEL_GRAPHICS_RECT* srcRect, IPANEL_GRAPHICS_BLT_PARAM* param )
{
	STGXOBJ_Bitmap_t   	*pSrcBitmap = 0;
	STGXOBJ_Bitmap_t   	*pDstBitmap = 0;


#ifdef EIS_DEBUG_2D	
	eis_report("\n  DEBUG 2D  ipanel_porting_graphics_stretch_surface hSurface= 0x%x ",hSurface);
#endif
	

	pSrcBitmap = (STGXOBJ_Bitmap_t*)(hSrcSurface);
	pDstBitmap = (STGXOBJ_Bitmap_t*)(hSurface);
	/*直接拷贝到OSD层*/
	if (!hSurface)
	{
		pDstBitmap = g_pGXOBJ_Bitmap;
	}
	if(!srcRect|| !dstRect || !param  || !pSrcBitmap || !pDstBitmap)
	{
		eis_report("[ ipanel_porting_graphics_blt_buffer ] ERROR: Invalid Paramters !! \n");
		goto ERR_EXIT;
	}

	if(srcRect->x <0 ) srcRect->x = 0;
	if(srcRect->y <0)  srcRect->y = 0;
	if( dstRect->x < 0 ) 	dstRect->x = 0;
	if( dstRect->y < 0) 	dstRect->y = 0;
	
	if( srcRect->width >INPUT_WIDTH) srcRect->width = INPUT_WIDTH;
	if( srcRect->height > INPUT_HEIGHT) srcRect->height = INPUT_HEIGHT;
	if( dstRect->width>INPUT_WIDTH)	dstRect->width = INPUT_WIDTH;
	if( dstRect->height > INPUT_HEIGHT ) dstRect->height = INPUT_HEIGHT;
	
#if 0 	/*sqzow20100611*/
	if(param->colorkey_flag == IPANEL_COLORKEY_ARGB && param->alpha_flag == 1)	//ok
	{
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = srcRect->width;
		tmpRect->height = srcRect->height;
		
		memset(SRC_GXOBJ_Bitmap.Data1_p,0,SRC_GXOBJ_Bitmap.Size1);
		ipanel_graphics_hdblt_rgbcolorkey(pSrcBitmap,srcRect, &SRC_GXOBJ_Bitmap, tmpRect,param->colorkey);
		ipanel_graphics_hdblt_alpha( &SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap,dstRect, param->alpha);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ALPHA && param->alpha_flag == 1)
	{
		unsigned int 	ypos = 0;
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = dstRect->width;
		tmpRect->height = dstRect->height;

		ipanel_graphics_hdblt_zoom(pSrcBitmap, srcRect, &SRC_GXOBJ_Bitmap, tmpRect);
		ipanel_graphics_swblt_srcpiexl(SRC_GXOBJ_Bitmap.Data1_p, SRC_GXOBJ_Bitmap.Width,tmpRect, pDstBitmap->Data1_p, pDstBitmap->Width, dstRect);
		ipanel_graphics_hdblt_alpha(&SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap, dstRect, param->alpha);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ARGB && param->alpha_flag == 0)
	{
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = srcRect->width;
		tmpRect->height = srcRect->height;
		
		memset(SRC_GXOBJ_Bitmap.Data1_p,0,SRC_GXOBJ_Bitmap.Size1);
		ipanel_graphics_hdblt_rgbcolorkey(pSrcBitmap,srcRect, &SRC_GXOBJ_Bitmap, tmpRect,param->colorkey);
		ipanel_graphics_hdblt_alpha( &SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap,dstRect, 100);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ALPHA && param->alpha_flag == 0)
	{
		unsigned int 	ypos = 0;
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = dstRect->width;
		tmpRect->height = dstRect->height;

		ipanel_graphics_hdblt_zoom(pSrcBitmap, srcRect, &SRC_GXOBJ_Bitmap, tmpRect);
		ipanel_graphics_swblt_dstpiexl(SRC_GXOBJ_Bitmap.Data1_p, SRC_GXOBJ_Bitmap.Width,tmpRect, pDstBitmap->Data1_p, pDstBitmap->Width, dstRect);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_DISABLE && param->alpha_flag == 1)
#else
	if(param->colorkey_flag != IPANEL_COLORKEY_DISABLE
		|| param->alpha_flag != 0)
#endif
	{
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = dstRect->width;
		tmpRect->height = dstRect->height;

		/*先缩放到混合层，再混合到目标层*/
		memset(SRC_GXOBJ_Bitmap.Data1_p,0, SRC_GXOBJ_Bitmap.Pitch * srcRect->height);
		ipanel_graphics_hdblt_zoom(pSrcBitmap, srcRect, &SRC_GXOBJ_Bitmap, tmpRect);
		ipanel_graphics_hdblt_alpha(&SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap, dstRect,
			param->alpha_flag == 0 ? 100 : param->alpha);
	}
	else
	{
		ipanel_graphics_hdblt_zoom(pSrcBitmap, srcRect, pDstBitmap, dstRect);
	}
	return IPANEL_OK;	
ERR_EXIT:
	return IPANEL_ERR;
}


/*void test_time(char *pstr)
{
	static U32 ticks = 0, secms = 0;
	if(secms == 0)
	{
		secms = ST_GetClocksPerSecondLow() / 1000;
	}
	if(pstr)
	{
		eis_report("\n%s-%d ms\n",pstr, (time_now() - ticks) / secms);
	}
	else
	{
		ticks = time_now();
	}
} sqzow20100624*/
/*
*函数输入:	hSurface	ipanel_porting_graphics_create_surface获得的图层句柄
*			x 			x坐标
*			y 			y坐标
*			width 		宽度
*			height  	高度
*			src 		ARGB_4444颜色
*			srcColorKey colorkey(目前暂时不支持)
*			srcLineWidth 源图形行宽
*			srcWidth 	源宽度
*			srcHeight 	源高度
*功能描述:	向图层上拷贝图像，src与图层的象素格式完全一致
*
*函数返回:	成功 IPANEL_OK
*			失败 IPANEL_ERR
***/
/* 图层之间拷贝(带alpha/colorkey功能)
*  hSurface 是目标图层，hSurface==NULL表示直接拷贝到OSD上
*  rect目标图层的区域
*  hSrcSurface 是源图层
*  srcx,srcy是srcSurface上的位置
*/

INT32_T ipanel_porting_graphics_blt_surface( VOID* hSurface, IPANEL_GRAPHICS_RECT* dstRect,
						VOID *hSrcSurface, INT32_T srcx, INT32_T srcy, IPANEL_GRAPHICS_BLT_PARAM* param )
{
	STGXOBJ_Bitmap_t   	*pSrcBitmap = 0;
	STGXOBJ_Bitmap_t   	*pDstBitmap = 0;
	IPANEL_GRAPHICS_RECT	srcRect[1] = {0};

#ifdef EIS_DEBUG_2D
	/*test_time(NULL); sqzow20100607*/
	eis_report("\n DEBUG 2D [ipanel_porting_graphics_blt_surface] hSurface=0x%x alpha_flag=%d-%d,colorkey_flag=%d-%d  rect:%d %d %d %d \n",
					(int)hSurface,param->alpha_flag,
					param->alpha,
					param->colorkey_flag, 
					 param->colorkey,
					dstRect->x, dstRect->y, dstRect->width, dstRect->height);
#endif
	pSrcBitmap = (STGXOBJ_Bitmap_t*)(hSrcSurface);
	pDstBitmap = (STGXOBJ_Bitmap_t*)(hSurface);
	/*直接拷贝到OSD层*/
	if (!hSurface)
	{
		pDstBitmap = g_pGXOBJ_Bitmap;
	}
	if(!hSrcSurface || !dstRect || !param ||!pSrcBitmap || !pDstBitmap || (pSrcBitmap == pDstBitmap))
	{
		eis_report("[ ipanel_porting_graphics_blt_surface ] ERROR: Invalid Paramters !! \n");
		goto ERR_EXIT;
	}

	if(srcx <0 ) srcx = 0;
	if(srcy <0)  srcy = 0;
	if( dstRect->x < 0 ) 	dstRect->x = 0;
	if( dstRect->y < 0) 	dstRect->y = 0;
	if( dstRect->width > INPUT_WIDTH)	dstRect->width = INPUT_WIDTH;
	if( dstRect->height > INPUT_HEIGHT) dstRect->height = INPUT_HEIGHT;
	
	srcRect->x = srcx;
	srcRect->y = srcy;
	srcRect->height = dstRect->height;
	srcRect->width = dstRect->width;

#if 0 	/*sqzow20100611*/
	if(param->colorkey_flag == IPANEL_COLORKEY_ARGB && param->alpha_flag == 1)
	{
		ipanel_graphics_hdblt_alphargb(pSrcBitmap,srcRect,pDstBitmap,dstRect,param->alpha,param->colorkey);	
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ALPHA && param->alpha_flag == 1)
	{
		unsigned int 	ypos = 0;
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = srcRect->width;
		tmpRect->height = srcRect->height;

		ipanel_graphics_hdblt_zoom(pSrcBitmap, srcRect, &SRC_GXOBJ_Bitmap, tmpRect);
		ipanel_graphics_swblt_srcpiexl(SRC_GXOBJ_Bitmap.Data1_p, SRC_GXOBJ_Bitmap.Width,tmpRect, pDstBitmap->Data1_p, pDstBitmap->Width, dstRect);
		ipanel_graphics_hdblt_alpha(&SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap, dstRect, param->alpha);		
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ARGB && param->alpha_flag == 0)
	{
		ipanel_graphics_hdblt_alphargb(pSrcBitmap,srcRect,pDstBitmap,dstRect,100,param->colorkey);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ALPHA && param->alpha_flag == 0)
	{	
		ipanel_graphics_swblt_dstpiexl(pSrcBitmap->Data1_p,pSrcBitmap->Width, srcRect, pDstBitmap->Data1_p, pDstBitmap->Width, dstRect);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_DISABLE && param->alpha_flag == 1)
 #else
 	if(param->colorkey_flag != IPANEL_COLORKEY_DISABLE
		|| param->alpha_flag != 0)
#endif
	{
		ipanel_graphics_hdblt_alpha(pSrcBitmap, srcRect, pDstBitmap, dstRect,
			param->alpha_flag == 0 ? 100:param->alpha);
	}
	else
	{
		ipanel_graphics_hdblt_zoom(pSrcBitmap, srcRect, pDstBitmap, dstRect);
	}
	return  IPANEL_OK; 
ERR_EXIT:
	return IPANEL_ERR;
}

		
/* 图层预处理 */
#if 1 	/*sqzow20100611*/
INT32_T ipanel_porting_graphics_prepare_surface( void* hSurface,
						IPANEL_GRAPHICS_RECT *rect, IPANEL_GRAPHICS_BLT_PARAM *param )
{
	int i = 0;
	int j = 0;
	STGXOBJ_Bitmap_t *pGXOBJ_Bitmap_src = hSurface;	
	unsigned int colorkey 				= 0;
	unsigned int *src_32 				= NULL;

	IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};



#ifdef EIS_DEBUG_2D	
	eis_report("\n DEBUG 2D [ipanel_porting_graphics_prepare_surface] hSurface=0x%x colorkey_flag=%d rect:%d %d %d %d\n",
		(int)hSurface, param->colorkey_flag, rect->x, rect->y, rect->width, rect->height);
#endif
	
	if (!hSurface)
	{
		eis_report("ipanel_porting_graphics_prepare_surface param error!\n");
		goto error;
	}
	pGXOBJ_Bitmap_src = (STGXOBJ_Bitmap_t *)hSurface;

	/*检查输入参数*/
	if(eis_check_rect_params(rect,pGXOBJ_Bitmap_src)==IPANEL_ERR)
	{
		eis_report("[ipanel_porting_graphics_prepare_surface] dst param error!dw=%d,dh=%d\n",
						rect->width, rect->height );
		goto error;
	}

	tmpRect->x = 0;
	tmpRect->y = 0;
	tmpRect->width = rect->width;
	tmpRect->height = rect->height;

	if(param->colorkey_flag
		&& (!(param->colorkey_flag == IPANEL_COLORKEY_ARGB
		&& param->colorkey == 0)))
	{
		/*先拷贝到混合层，再拷贝回来*/
		memset(SRC_GXOBJ_Bitmap.Data1_p,0,SRC_GXOBJ_Bitmap.Pitch * rect->height);
		ipanel_graphics_hdblt_rgbcolorkey(pGXOBJ_Bitmap_src,
			rect,
			&SRC_GXOBJ_Bitmap,
			tmpRect,
			param->colorkey);
		ipanel_graphics_hdblt_zoom(&SRC_GXOBJ_Bitmap, tmpRect, 
			pGXOBJ_Bitmap_src, rect);
	}
	else
	{
		eis_report("\n~~~~~~~~~~~ignore!!!");
	}

	return IPANEL_OK;
error:
	eis_report("error!\n");
	return IPANEL_ERR;
}
#else
INT32_T ipanel_porting_graphics_prepare_surface( void* hSurface,
						IPANEL_GRAPHICS_RECT *rect, IPANEL_GRAPHICS_BLT_PARAM *param )
{
	int i = 0;
	int j = 0;
	STGXOBJ_Bitmap_t *pGXOBJ_Bitmap_src = hSurface;	
	unsigned int colorkey 				= 0;
	unsigned int *src_32 				= NULL;


#ifdef EIS_DEBUG_2D	
	eis_report("\n DEBUG 2D [ipanel_porting_graphics_prepare_surface] hSurface=0x%x colorkey_flag=%d rect:%d %d %d %d\n",
		(int)hSurface, param->colorkey_flag, rect->x, rect->y, rect->width, rect->height);
#endif
	
	if (!hSurface)
	{
		eis_report("ipanel_porting_graphics_prepare_surface param error!\n");
		goto error;
	}
	pGXOBJ_Bitmap_src = (STGXOBJ_Bitmap_t *)hSurface;

	/*检查输入参数*/
	if(eis_check_rect_params(rect,pGXOBJ_Bitmap_src)==IPANEL_ERR)
	{
		eis_report("[ipanel_porting_graphics_prepare_surface] dst param error!dw=%d,dh=%d\n",
						rect->width, rect->height );
		goto error;
	}


	switch (param->colorkey_flag)
	{
	case IPANEL_COLORKEY_RGB:
		{
			EIS_2D_CopyWithRgbColorKey_2Dst(pGXOBJ_Bitmap_src,
				pGXOBJ_Bitmap_src,
				rect,
				rect,
				param->colorkey);		
		}
		break;
	case IPANEL_COLORKEY_ALPHA:
		{
			EIS_2D_CopyWithAlphaColorKey_2Dst(pGXOBJ_Bitmap_src,
				pGXOBJ_Bitmap_src,
				rect,
				rect,
				param->colorkey);		
		}
		break;
	case IPANEL_COLORKEY_ARGB:
		{
			EIS_2D_CopyWithArgbColorKey_2Dst(pGXOBJ_Bitmap_src,
				pGXOBJ_Bitmap_src,
				rect,
				rect,
				param->colorkey);	
		}
		break;
	case IPANEL_COLORKEY_DISABLE:
		break;
	default:
		eis_report("==default== %d\n", param->colorkey_flag);
		break;
	}

	return IPANEL_OK;
error:
	eis_report("error!\n");
	return IPANEL_ERR;
}
#endif



INT32_T ipanel_porting_graphics_blt_buffer( VOID* hSurface, IPANEL_GRAPHICS_RECT* dstRect,
						VOID *src, INT32_T srcx, INT32_T srcy, INT32_T srcLineWidth, IPANEL_GRAPHICS_BLT_PARAM* param )

{
	STGXOBJ_Bitmap_t   	*pDstBitmap = 0;
	STBLIT_Source_t	 	BlitSource = {0};
	STBLIT_Destination_t	BlitDestination = {0};
	ST_ErrorCode_t		ErrCode = ST_NO_ERROR;
	IPANEL_GRAPHICS_RECT	srcRect[1] = {0};

#ifdef EIS_DEBUG_2D
	/*test_time(NULL); sqzow20100607*/
	eis_report("\n DEBUG 2D [ipanel_porting_graphics_blt_buffer] hSurface=0x%x alpha_flag=%d-%d,colorkey_flag=%d-%d  rect:%d %d %d %d \n",
					(int)hSurface,param->alpha_flag,
					param->alpha,
					param->colorkey_flag, 
					 param->colorkey,
					dstRect->x, dstRect->y, dstRect->width, dstRect->height);
#endif
	if(!src || !dstRect || !param )
	{
		eis_report("[ ipanel_porting_graphics_blt_buffer ] ERROR: Invalid Paramters !! \n");
		goto ERR_EXIT;
	}

	if(srcx <0 ) srcx = 0;
	if(srcy <0)  srcy = 0;
	if( dstRect->x < 0 ) 	dstRect->x = 0;
	if( dstRect->y < 0) 	dstRect->y = 0;
	if( dstRect->width>INPUT_WIDTH)	dstRect->width = INPUT_WIDTH;
	if( dstRect->height > INPUT_HEIGHT ) dstRect->height = INPUT_HEIGHT;
	if(srcLineWidth > INPUT_WIDTH) srcLineWidth =  INPUT_WIDTH;

	pDstBitmap = (STGXOBJ_Bitmap_t*)(hSurface);
		
	/*直接拷贝到OSD层*/
	if (!hSurface)
	{
		pDstBitmap = g_pGXOBJ_Bitmap;
	}
	
	srcRect->x = srcx;
	srcRect->y = srcy;
	srcRect->width = dstRect->width;
	srcRect->height = dstRect->height;

#if 0 	/*sqzow20100611*/
	if(param->colorkey_flag == IPANEL_COLORKEY_ARGB && param->alpha_flag == 0)
	{
		unsigned int ypos = 0;
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = srcRect->width;
		tmpRect->height = srcRect->height;
		
		memset(SRC_GXOBJ_Bitmap.Data1_p,0,SRC_GXOBJ_Bitmap.Size1);
		for(ypos = 0 ; ypos < srcRect->height; ypos++)
		{
			unsigned char*  srcptr = (unsigned char*)src + ( (ypos + srcRect->y)*srcLineWidth + srcRect->x )*4;
			unsigned char* dstptr = (unsigned char*)SRC_GXOBJ_Bitmap.Data1_p+ ypos*INPUT_WIDTH *4 ;
			memcpy(dstptr,srcptr,srcRect->width*4 );	
		}
		ipanel_graphics_hdblt_alphargb( &SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap,dstRect,100,param->colorkey);		
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_ALPHA && param->alpha_flag == 0)
	{
		ipanel_graphics_swblt_dstpiexl(src, srcLineWidth, srcRect, pDstBitmap->Data1_p, pDstBitmap->Width, dstRect);	
	}	
	else if(param->colorkey_flag == IPANEL_COLORKEY_DISABLE && param->alpha_flag == 1)
#else
	if(param->colorkey_flag != IPANEL_COLORKEY_DISABLE
		|| param->alpha_flag != 0)
#endif

	{
		unsigned int ypos = 0;
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = srcRect->width;
		tmpRect->height = srcRect->height;

		memset(SRC_GXOBJ_Bitmap.Data1_p,0,SRC_GXOBJ_Bitmap.Pitch * srcRect->height);
		for(ypos = 0 ; ypos < srcRect->height; ypos++)
		{
			unsigned char*  srcptr = (unsigned char*)src + ( (ypos + srcRect->y)*srcLineWidth + srcRect->x )*4;
			unsigned char* dstptr = (unsigned char*)SRC_GXOBJ_Bitmap.Data1_p+ ypos*INPUT_WIDTH *4 ;
			memcpy(dstptr,srcptr,srcRect->width*4 );	
		}
		ipanel_graphics_hdblt_alpha( &SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap,dstRect,
			param->alpha_flag == 0 ? 100 : param->alpha);
	}
	else if(param->colorkey_flag == IPANEL_COLORKEY_DISABLE && param->alpha_flag == 0)
	{
		unsigned int ypos = 0;
		for(ypos = 0 ; ypos < srcRect->height; ypos++)
		{
			unsigned char*  srcptr = (unsigned char*)src + ( (ypos + srcRect->y)*srcLineWidth + srcRect->x )*4;
			unsigned char* dstptr = (unsigned char*)pDstBitmap->Data1_p+ ( (ypos + dstRect->y)*pDstBitmap->Width+ dstRect->x )*4;
			memcpy(dstptr,srcptr,srcRect->width*4 );	
		}
	}
	return IPANEL_OK;
ERR_EXIT:
	return IPANEL_ERR;
}

INT32_T ipanel_porting_graphics_stretch_buffer( VOID* hSurface, IPANEL_GRAPHICS_RECT* dstRect,
						VOID *src, IPANEL_GRAPHICS_RECT* srcRect, INT32_T srcLineWidth, IPANEL_GRAPHICS_BLT_PARAM* param )
{
	STGXOBJ_Bitmap_t   	*pDstBitmap = 0;
	STBLIT_Source_t	 	BlitSource = {0};
	STBLIT_Destination_t	BlitDestination = {0};
	ST_ErrorCode_t		ErrCode = ST_NO_ERROR;

#ifdef EIS_DEBUG_2D
	/*test_time(NULL); sqzow20100607*/
	eis_report("\n DEBUG 2D [ipanel_porting_graphics_stretch_buffer] hSurface=0x%x alpha_flag=%d-%d,colorkey_flag=%d-%d  rect:%d %d %d %d \n",
					(int)hSurface,param->alpha_flag,
					param->alpha,
					param->colorkey_flag, 
					 param->colorkey,
					dstRect->x, dstRect->y, dstRect->width, dstRect->height);
#endif
	if(!src ||!srcRect|| !dstRect || !param )
	{
		eis_report("[ ipanel_porting_graphics_blt_buffer ] Invalid Paramters !! \n");
		goto ERR_EXIT;
	}

	/*直接拷贝到OSD层*/
	if (!hSurface)
	{
		hSurface = (void*)g_pGXOBJ_Bitmap;
	}
	if(srcRect->x <0 ) srcRect->x = 0;
	if(srcRect->y <0)  srcRect->y = 0;
	if( dstRect->x < 0 ) 	dstRect->x = 0;
	if( dstRect->y < 0) 	dstRect->y = 0;
	
	if( srcRect->width >INPUT_WIDTH) 	srcRect->width = INPUT_WIDTH;
	if( srcRect->height > INPUT_HEIGHT) 	srcRect->height = INPUT_HEIGHT;
	if( dstRect->width>INPUT_WIDTH)	dstRect->width = INPUT_WIDTH;
	if( dstRect->height > INPUT_HEIGHT ) dstRect->height = INPUT_HEIGHT;
	if(srcLineWidth > INPUT_WIDTH) 	srcLineWidth =  INPUT_WIDTH;

	pDstBitmap = (STGXOBJ_Bitmap_t*)(hSurface);
	if(param->colorkey_flag == IPANEL_COLORKEY_DISABLE && param->alpha_flag == 0)
	{
		unsigned int ypos = 0;
		IPANEL_GRAPHICS_RECT	tmpRect[1] = {0};
		tmpRect->x = 0;
		tmpRect->y = 0;
		tmpRect->width = srcRect->width;
		tmpRect->height = srcRect->height;
		
		memset(SRC_GXOBJ_Bitmap.Data1_p,0,SRC_GXOBJ_Bitmap.Pitch * srcRect->height);
		for(ypos = 0 ; ypos < srcRect->height; ypos++)
		{
			unsigned char*  srcptr = (unsigned char*)src + ( (ypos + srcRect->y)*srcLineWidth + srcRect->x )*4;
			unsigned char* dstptr = (unsigned char*)SRC_GXOBJ_Bitmap.Data1_p+ ypos*INPUT_WIDTH*4 ;
			memcpy(dstptr,srcptr,srcRect->width*4 );	
		}
		ipanel_graphics_hdblt_zoom( &SRC_GXOBJ_Bitmap, tmpRect, pDstBitmap,dstRect);
	}
	return  IPANEL_OK;
ERR_EXIT:
	
	return IPANEL_ERR;
}




/* 图片预处理 */
//这个函数目前的应用上基本上还没用到
INT32_T ipanel_porting_graphics_prepare_buffer( VOID *src, INT32_T srcLineWidth,
						IPANEL_GRAPHICS_RECT *rect, IPANEL_GRAPHICS_BLT_PARAM *param )
{
	STGXOBJ_Bitmap_t GXOBJ_Bitmap_src;


 #ifdef EIS_DEBUG_2D	
	eis_report("\n DEBUG 2D [ipanel_porting_graphics_prepare_buffer]rect: %d %d %d %d  param:%d 0x%x\n",
			rect->x, rect->y, rect->width, rect->height,param->colorkey_flag, param->colorkey);
#endif
	
	if (!src)
	{
		eis_report("ipanel_porting_graphics_prepare_buffer param error!\n");
		goto error;
	}

	memcpy(&GXOBJ_Bitmap_src, 
		&SRC_GXOBJ_Bitmap,
		sizeof(STGXOBJ_Bitmap_t));
	GXOBJ_Bitmap_src.Pitch = srcLineWidth * 4;
	GXOBJ_Bitmap_src.Size1 = GXOBJ_Bitmap_src.Pitch * rect->height;
	GXOBJ_Bitmap_src.Width = srcLineWidth;
	GXOBJ_Bitmap_src.Height = rect->height;

		
	switch (param->colorkey_flag)
	{
		
	case IPANEL_COLORKEY_RGB:
		{
			EIS_2D_CopyWithRgbColorKey_2Dst(&GXOBJ_Bitmap_src,
				&GXOBJ_Bitmap_src,
				rect,
				rect,
				param->colorkey);		
		}
		break;
	case IPANEL_COLORKEY_ALPHA:
		{
			EIS_2D_CopyWithAlphaColorKey_2Dst(&GXOBJ_Bitmap_src,
				&GXOBJ_Bitmap_src,
				rect,
				rect,
				param->colorkey);		
		}
		break;
	case IPANEL_COLORKEY_ARGB:
		{
			EIS_2D_CopyWithArgbColorKey_2Dst(&GXOBJ_Bitmap_src,
				&GXOBJ_Bitmap_src,
				rect,
				rect,
				param->colorkey);	
		}
		break;
	case IPANEL_COLORKEY_DISABLE:
		break;
	default:
		eis_report("==default== %d\n", param->colorkey_flag);
		break;
	}

	//eis_report("stretch_surface: %dms!\n", (ipanel_porting_time_ms()-start));
	return IPANEL_OK;
error:
	eis_report("error!\n");
	return IPANEL_ERR;
}


INT32_T ipanel_porting_graphics_draw_line(void* hSurface, INT32_T x1, INT32_T y1, INT32_T x2,INT32_T y2, INT32_T color, INT32_T thickness)
{
	STGXOBJ_Bitmap_t *pGXOBJ_Bitmap_src = NULL;	
	IPANEL_GRAPHICS_RECT rect;
 #ifdef EIS_DEBUG_2D	
 	eis_report("\n DEBUG 2D [ipanel_porting_graphics_draw_line]hSurface=0x%x %d %d -> %d %d :%d 0x%x\n",
								(int)hSurface, x1, y1, x2, y2, thickness, color);
#endif

	if (!hSurface)
	{
		eis_report("ipanel_porting_graphics_draw_line param error!\n");
		goto error;
	}
	
	if (!hSurface)
		pGXOBJ_Bitmap_src = g_pGXOBJ_Bitmap;
	/*拷贝到目标surface*/
	else 
		pGXOBJ_Bitmap_src = (STGXOBJ_Bitmap_t *)hSurface;


	/*检查输入参数*/
	if (x1<0){
		eis_report("%d \n", x1);
		x1 = 0;
	}	
	if (y1<0){
		eis_report("%d \n", y1);
		y1 = 0;
	}	
	if (x2<0){
		eis_report("%d \n", x2);
		x1 = 0;
	}	
	if (y2<0){
		eis_report("%d \n", y2);
		y1 = 0;
	}	
	if (y1 == y2 && x1 == x2){
		goto error;
	}
	if (!color){
		eis_report("color NULL error! \n");
		goto error;
	}	

	rect.x=x1;
	rect.y=y1;
	rect.width=(x2>x1)? (x2-x1):(x1-x2);
	rect.height=(y2>y1)? (y2-y1):(y1-y2);
		
	EIS_2D_DrawRectangle(pGXOBJ_Bitmap_src,&rect,color);
	
	return IPANEL_OK;
error:
	eis_report("error!\n");
	return IPANEL_ERR;

}


/* 是把osdsurface的指定区域送到屏幕上显示 
*/
INT32_T ipanel_porting_graphics_update_osd(INT32_T x, INT32_T y, INT32_T width, INT32_T height )
{
	IPANEL_GRAPHICS_RECT rect;
	STBLIT_BlitContext_t BLIT_Context 	= {0};
	CH_RESOLUTION_MODE_e enum_Mode;
#ifdef EIS_DEBUG_2D	
	eis_report(" DEBUG 2D [ipanel_porting_graphics_update_osd x=%d,y=%d,w=%d,h=%d]\n",x,y,width,height);
#endif
	if(CH_GetUserIDStatus() == true)
	{
		return IPANEL_OK;
	}

#ifdef SUMA_SECURITY
		semaphore_wait(pst_OsdSema);
#endif
	
	rect.x=x;
	rect.y=y;
	rect.width=width;
	rect.height=height;
	get_default_blit_params(&BLIT_Context);
	
	enum_Mode = CH_GetReSolutionRate() ;
	 if(HighSolution_change != enum_Mode)
 	{
 #ifdef EIS_DEBUG_2D
		eis_report(" DEBUG 2D [CH_ChangeOSDViewPort_HighSolution ");
 #endif
		CH_SetReSolutionRate(HighSolution_change);
		CH_ChangeOSDViewPort_HighSolution(HighSolution_change,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);

	}
	EIS_2D_CopyArea(g_pGXOBJ_Bitmap,&g_OSDBitmap_HighSolution[1],&rect,&rect,&BLIT_Context);

#ifdef SUMA_SECURITY
		semaphore_signal(pst_OsdSema);
#endif
#ifdef SUMA_SECURITY	
	if( CH_CheckSKBDisplay(x,y,width,height) )
	{
			do_report(0,"\n SumaSecure_SKB_updateSKB \n");
			SumaSecure_SKB_updateSKB();
			do_report(0,"\n SumaSecure_SKB_updateSKB over) \n");
	}
#endif

	return IPANEL_OK;
}

INT32_T ipanel_porting_graphics_stretch_buffer2( VOID* dst, INT32_T dstLineWidth, IPANEL_GRAPHICS_RECT rect,
						VOID *src, IPANEL_GRAPHICS_RECT srcRect, INT32_T srcLineWidth, IPANEL_GRAPHICS_BLT_PARAM param )
{
 #ifdef EIS_DEBUG_2D	
	eis_report(" DEBUG 2D [ipanel_porting_graphics_stretch_buffer2 ]\n");
#endif

	return IPANEL_OK;
}



INT32_T ipanel_porting_graphics_ioctl_ex(IPANEL_GRAPHICS_IOCTL_e op, VOID *arg)
{
	IPANEL_GRAPHICS_WIN_RECT *p_Struct;
#ifdef EIS_DEBUG_2D
	eis_report ( "\n++>eis ipanel_porting_graphics_ioctl_ex op=%d",op );
#endif
	switch ( op )
	{
		case IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY:
			p_Struct = ( IPANEL_GRAPHICS_WIN_RECT* )arg;

			eis_report("\n ipanel_porting_graphics_ioctl_ex p_Struct  x %d y %d w %d h %d",p_Struct->x,p_Struct->y,p_Struct->w,p_Struct->h );


			if(p_Struct->h  <= 576 )/*标清OSD*/
			{
				HighSolution_change=CH_720X576_MODE;
				act_src_w=p_Struct->w;
				act_src_h=p_Struct->h;
			}
	 		else
			{                           /*高清OSD界面*/
				HighSolution_change=CH_1280X720_MODE;
				act_src_w=p_Struct->w-1;
				act_src_h=p_Struct->h-1;
			} 
			
			g_IpanelDiaplyRect.x = p_Struct->x;
			g_IpanelDiaplyRect.y = p_Struct->y;
			g_IpanelDiaplyRect.w = p_Struct->w;
			g_IpanelDiaplyRect.h = p_Struct->h;
		
			eis_report ( " width=%d, height=%d",act_src_w,act_src_h );
		
			return IPANEL_OK;
			
			break;
		case IPANEL_GRAPHICS_SET_ALPHA_NOTIFY:
		{
			int alpha;
			int u8_ActAlpha;

			alpha = *(int*)arg;

		      if(alpha > 100)
			{
			  	alpha= 100;
			}
			else if(alpha < 0)
			{
				alpha = 0;
			}
			u8_ActAlpha = alpha * 128 / 100;
		       
			CH_SetViewPortAlpha_HighSolution (  u8_ActAlpha );/**/

			return IPANEL_OK;
		}
			break;
		default:
			return IPANEL_ERR;
			break;
	}
	return IPANEL_ERR;
}

static INT32_T  eis_check_rect_params(IPANEL_GRAPHICS_RECT *rect,STGXOBJ_Bitmap_t *pGXOBJ_Bitmap)
{

	if (rect->x<0) return IPANEL_ERR;
	if (rect->y<0) return IPANEL_ERR;
	if ( rect->width<=0 || rect->height<=0)    return IPANEL_ERR;
	if ((rect->x+rect->width)>(pGXOBJ_Bitmap->Width+1))	return IPANEL_ERR;
	if ((rect->y+rect->height)>(pGXOBJ_Bitmap->Height+1))   return IPANEL_ERR;
	return IPANEL_OK;
}
static void eis_BLT_params_translation(IPANEL_GRAPHICS_BLT_PARAM *param,STBLIT_BlitContext_t * p_BLIT_Context)
{
	switch (param->colorkey_flag)
	{
	case IPANEL_COLORKEY_ARGB:
		p_BLIT_Context->ColorKeyCopyMode 				= STBLIT_COLOR_KEY_MODE_SRC;
		p_BLIT_Context->ColorKey.Type 					= STGXOBJ_COLOR_KEY_TYPE_RGB888;
		p_BLIT_Context->ColorKey.Value.RGB888.RMin 	= (param->colorkey>>16)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.RMax 	= (param->colorkey>>16)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.ROut 	= false; //true; //false; //这个true是取反的
		p_BLIT_Context->ColorKey.Value.RGB888.REnable 	= true;	
		p_BLIT_Context->ColorKey.Value.RGB888.GMin 	= (param->colorkey>>8)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.GMax 	= (param->colorkey>>8)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.GOut 	= false; //true; //false; //这个true是取反的
		p_BLIT_Context->ColorKey.Value.RGB888.GEnable 	= true;	
		p_BLIT_Context->ColorKey.Value.RGB888.BMin 	= (param->colorkey)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.BMax 	= (param->colorkey)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.BOut 	= false; //true; //false; //这个true是取反的
		p_BLIT_Context->ColorKey.Value.RGB888.BEnable 	= true;	

		p_BLIT_Context->AluMode 	= STBLIT_ALU_ALPHA_BLEND;
		p_BLIT_Context->GlobalAlpha= 0xff;
		break;
	case IPANEL_COLORKEY_RGB:
		p_BLIT_Context->ColorKeyCopyMode 				= STBLIT_COLOR_KEY_MODE_SRC;
		p_BLIT_Context->ColorKey.Type 					= STGXOBJ_COLOR_KEY_TYPE_RGB888;
		p_BLIT_Context->ColorKey.Value.RGB888.RMin 	= (param->colorkey>>16)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.RMax 	= (param->colorkey>>16)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.ROut 	= false; //true; //false; //这个true是取反的
		p_BLIT_Context->ColorKey.Value.RGB888.REnable 	= true;	
		p_BLIT_Context->ColorKey.Value.RGB888.GMin 	= (param->colorkey>>8)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.GMax 	= (param->colorkey>>8)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.GOut 	= false; //true; //false; //这个true是取反的
		p_BLIT_Context->ColorKey.Value.RGB888.GEnable 	= true;	
		p_BLIT_Context->ColorKey.Value.RGB888.BMin 	= (param->colorkey)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.BMax 	= (param->colorkey)&0xFF;
		p_BLIT_Context->ColorKey.Value.RGB888.BOut 	= false; //true; //false; //这个true是取反的
		p_BLIT_Context->ColorKey.Value.RGB888.BEnable 	= true;	
		break;
	case IPANEL_COLORKEY_ALPHA:
		p_BLIT_Context->AluMode 	= STBLIT_ALU_ALPHA_BLEND;
		p_BLIT_Context->GlobalAlpha= 0xff;
		break;
	case IPANEL_COLORKEY_DISABLE:
		break;
	default:
		break;
	}
	
	if (param->alpha_flag == IPANEL_ALPHA_GLOBAL)
	{
		p_BLIT_Context->AluMode 	=STBLIT_ALU_ALPHA_BLEND;
		p_BLIT_Context->GlobalAlpha=(param->alpha*255)/100;	// param->alpha;//
#ifdef EIS_DEBUG_2D
		eis_report(" \n p_BLIT_Context->GlobalAlpha=%d param->alpha=%d",p_BLIT_Context->GlobalAlpha,param->alpha);
#endif
		//task_delay(ST_GetClocksPerSecond());
	}
	else if (param->alpha_flag == IPANEL_ALPHA_SRCOVER)
	{
		p_BLIT_Context->AluMode 	= STBLIT_ALU_AND; //STBLIT_ALU_AND_REV
	}
	//if(param->alpha_flag != IPANEL_ALPHA_GLOBAL)
	//eis_report(" \n p_BLIT_Context->GlobalAlpha=%d ",p_BLIT_Context->GlobalAlpha);



}


/*
*函数输入:	blit类型指针STBLIT_BlitContext_t *BLIT_Context
*
*函数输出:	BLIT_Context赋初始值
*
*功能描述:	BLIT_Context赋初始值
*
*函数返回:	无 
***/
static void get_default_blit_params(STBLIT_BlitContext_t *BLIT_Context)
{
	/* Set blit context */
	BLIT_Context->ColorKeyCopyMode         = STBLIT_COLOR_KEY_MODE_NONE;
	BLIT_Context->AluMode                  = STBLIT_ALU_COPY;
	
	BLIT_Context->EnableMaskWord           = FALSE;
	BLIT_Context->MaskWord                 = 0;
	BLIT_Context->EnableMaskBitmap         = FALSE;
	BLIT_Context->MaskBitmap_p             = NULL;
	BLIT_Context->MaskRectangle.PositionX  = 0;
	BLIT_Context->MaskRectangle.PositionY  = 0;
	BLIT_Context->MaskRectangle.Width      = 0;
	BLIT_Context->MaskRectangle.Height     = 0;
	BLIT_Context->WorkBuffer_p             = NULL;
	BLIT_Context->EnableColorCorrection    = FALSE;
	BLIT_Context->ColorCorrectionTable_p   = NULL;
	BLIT_Context->Trigger.EnableTrigger    = FALSE;
	BLIT_Context->Trigger.ScanLine         = 0;
	BLIT_Context->Trigger.Type             = STBLIT_TRIGGER_TYPE_BOTTOM_VSYNC;
	BLIT_Context->Trigger.VTGId            = 0;
	BLIT_Context->GlobalAlpha              = 255;
	BLIT_Context->EnableClipRectangle      = TRUE;
	BLIT_Context->ClipRectangle.PositionX  = 0;
	BLIT_Context->ClipRectangle.PositionY  = 0;
	BLIT_Context->ClipRectangle.Width      = 0;
	BLIT_Context->ClipRectangle.Height     = 0;
	BLIT_Context->WriteInsideClipRectangle = TRUE;
	BLIT_Context->EnableFlickerFilter      = FALSE;
	BLIT_Context->JobHandle                = STBLIT_NO_JOB_HANDLE;
	BLIT_Context->UserTag_p                = NULL;
	BLIT_Context->NotifyBlitCompletion 	   = TRUE;
	
}


#ifdef USE_EIS_2D
#ifdef SUMA_SECURITY

U8* CH_Eis_GetRectangle(int PosX, int PosY, int Width, int Height)
{
  	U8*  pTemp = NULL;
	int     i_bufsize = Width*Height*4;
	STGXOBJ_Bitmap_t TempBMP;
	STGXOBJ_Rectangle_t SrcRectangle;
	STBLIT_BlitContext_t	 m_ipanel_gfx_blitcontext;
	 STAVMEM_FreeBlockParams_t   st_FreeBlockParas;
	SrcRectangle.PositionX = PosX;
	SrcRectangle.PositionY= PosY;
	SrcRectangle.Width= Width;
	SrcRectangle.Height= Height;
	pTemp=memory_allocate(CHSysPartition,i_bufsize);
	EIS_Creat_Surface(MAX_SURFACE_NUM+2,LAYERHandle[LAYER_GFX1],&TempBMP,Width,Height );
	memcpy(pTemp,TempBMP.Data1_p,i_bufsize);

	TempBMP.ColorType=STGXOBJ_COLOR_TYPE_ARGB8888;
	 STGFX_CopyArea(GFXHandle,g_pGXOBJ_Bitmap,&TempBMP,&gGC,&SrcRectangle,0,0);
      STGFX_Sync(GFXHandle);
	  
	
	if(pTemp==NULL || TempBMP.Data1_p == NULL) 	
	{
		partition_status_t Teststaus;
		partition_status_flags_t flags=0;
		if( partition_status(CHSysPartition,&Teststaus,flags)==0)
		{
			do_report(0,"i_bufsize \n");
			do_report(0,"chsys_partition  free size=%x,total used size%x,lArge free=%x\n",
				Teststaus.partition_status_free,Teststaus.partition_status_size,
				Teststaus.partition_status_free_largest);
		}
		return NULL;	

	}
	else
	{
		memcpy(pTemp,TempBMP.Data1_p,i_bufsize);
	}
	 st_FreeBlockParas.PartitionHandle = AVMEMHandle[1];
       STAVMEM_FreeBlock(&st_FreeBlockParas,&eis_surface_BlockHandle[MAX_SURFACE_NUM+2]);
	return  pTemp;
}


#endif
#endif




