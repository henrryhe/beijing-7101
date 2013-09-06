/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_osd.c
  * 描述: 	实现绘图相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "eis_api_osd.h"
#include "..\dbase\vdbase.h"
#include "eis_include\ipanel_graphics.h"
#include "channelbase.h"
#include "..\main\initterm.h"

#ifdef __EIS_API_DEBUG_ENABLE__
//#define __EIS_API_OSD_DEBUG__
#endif

#ifdef SUMA_SECURITY
extern semaphore_t *pst_OsdSema ;/* 用于IPNAEL的显示控制*/
#endif
static unsigned char 	*draw_buffer 	= NULL;
#ifdef EIS_OSD_YASUO
unsigned char 	*cvbs_draw_buffer 	= NULL;
#endif
unsigned char 	*Bak_OSD_buffer 	= NULL;

int act_src_w=720,act_src_h=576;

IPANEL_GRAPHICS_WIN_RECT   g_IpanelDiaplyRect;

 CH_RESOLUTION_MODE_e HighSolution_change=CH_1280X720_MODE;

void CH_RestoreOsd();
void CH_PutBakOsd( );


/*
功能说明：
	获取显示窗口的信息，如iPanel MiddleWare使用显示窗口的尺寸、显示屏幕缓存
	区的地址、显示屏幕缓冲区的尺寸。该函数是在iPanel Middleware 系统运行初始的
	时候调用一次，在正常运行过程当中不再调用。
	注意：如果屏幕的输出有颜色失真，花屏等情况，请确认底层所用的颜色格式是否
	和库的颜色格式一致，库的颜色格式可以在调试信息中找到。如果开机有黑屏，绿屏
	等情况，请确认一下底层透明色的设置。
参数说明：
	输入参数：
		无
	输出参数：
		width,height: iPanel MiddleWare输出窗口的大小；
		buf_width，buf_height: 实际显示屏幕缓冲区的大小，目前iPanel
		Middleware只处理输出窗口和显示屏幕缓冲区大小相等的情况，所以buf_width必
		须等于width，所以buf_height必须等于height。
		pbuf:返回显示的缓冲区地址指针,如果*pbuf = IPANEL_NULL,iPanel
		Middleware自己分配缓冲区，然后调用ipanel_porting_draw_image 输出到屏幕
		显示缓冲区中。如果*pbuf!=IPANEL_NULL, iPanel Middleware直接将结果输出到
		屏幕显示缓冲区中，ipanel_porting_draw_image可以是空函数。
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_graphics_get_info(INT32_T *width, INT32_T *height, VOID **pbuf, INT32_T *buf_width, INT32_T *buf_height)
{

	if(width == NULL
		|| height == NULL
		|| buf_width == NULL
		|| pbuf == NULL)
	{
		eis_report("\nipanel_porting_graphics_get_info para error!");
		return IPANEL_ERR;
	}
	*width			= EIS_REGION_WIDTH;
	*height			= EIS_REGION_HEIGHT;
	*buf_width		= EIS_REGION_WIDTH;
	*buf_height		= EIS_REGION_HEIGHT;

	*pbuf 			= /*NULL*/draw_buffer;

      g_IpanelDiaplyRect.x = 0;
      g_IpanelDiaplyRect.y = 0;
      g_IpanelDiaplyRect.w = EIS_REGION_WIDTH;
      g_IpanelDiaplyRect.h=   EIS_REGION_HEIGHT;
	eis_report ( "\n++>eis osd ipanel_porting_graphics_get_info=<%d,%d,%d,%d>", *width, *height, *buf_width, *buf_height );
	
	return IPANEL_OK;	
	
}

/*
功能说明：
	安装调色板，iPanel MiddleWare 要求调色板中的0 号色为透明色。当输出格式
	小于等于8bpp时，需要实现该函数，其他格式时该函数应置空。
参数说明：
	输入参数：
		npals：调色板大小，8 位颜色时256；
		pal[]：是颜色板数据，所有的颜色都定义成32bit 整数型，表示颜色格式是：
		0x00RRGGBB。该函数是在IPanel Middleware 系统每次开始运行时调用一次，在
		正常运行过程当中不再调用。大于8位的颜色模式不会使用调色板。
	输出参数：
		无。
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。		
*/
INT32_T ipanel_porting_graphics_install_palette(UINT32_T *pal, INT32_T npals)
{
	return IPANEL_OK;
}

/*
功能说明：
	当使用调色板工作模式时，设置调色板中颜色值透明度。注意这里设置的alpha不
	在作为区域alpha的控制方式。
参数说明：
	输入参数：
		alpha：0～100，0为全透，100为完全不透明。
	输出参数：
		无。
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_graphics_set_alpha(INT32_T alpha)
{
	U8 u8_ActAlpha;

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
/*
  * Func: eis_clear_all
  * Desc: 清除整个OSD区
  */
void eis_clear_all()
{
	CH6_AVControl ( VIDEO_BLACK, false, CONTROL_VIDEO );
}

/*
 * del the eis region and restore the osd region
 */
void eis_del_region( void )
{
#ifdef CH_MUTI_RESOLUTION_RATE
{
     CH_RESOLUTION_MODE_e enum_Mode;
     enum_Mode = CH_GetReSolutionRate() ;
    if(enum_Mode == CH_OLD_MOE)
    	{
	   CH6_ClearFullArea( GFXHandle, CH6VPOSD.ViewPortHandle );
    	}
	else
	{
           CH_ClearFUll_HighSolution();
	}
}
#else
	/* Clear full viewport */
	CH6_ClearFullArea( GFXHandle, CH6VPOSD.ViewPortHandle );
#endif
	/* 打开背景层 
	CH6_MixLayer( true, true, true );*/

	CH6_AVControl( VIDEO_CLOSE, false, CONTROL_VIDEO );

	if ( NULL != draw_buffer )
	{
		memory_deallocate( CHSysPartition, draw_buffer );
		draw_buffer = NULL;
	}

	return;
}

/*
 * init the eis region
 */
void eis_init_region( void )
{

	/* 必须在系统区内分配 */
#if 1
	draw_buffer = (unsigned char *)memory_allocate ( CHSysPartition, EIS_REGION_WIDTH * EIS_REGION_HEIGHT * 4 +10);
	if(NULL!=draw_buffer)
	memset(draw_buffer,0,EIS_REGION_WIDTH * EIS_REGION_HEIGHT * 4);
#endif
	HighSolution_change= CH_GetReSolutionRate() ;

#ifdef EIS_OSD_YASUO
	cvbs_draw_buffer = (unsigned char *)memory_allocate ( CHSysPartition, EIS_REGION_WIDTH*576 * 4 +10);
	if(NULL!=cvbs_draw_buffer)
	memset(cvbs_draw_buffer,0, EIS_REGION_WIDTH*576 * 4 );
#endif	
	
	Bak_OSD_buffer=(unsigned char *)memory_allocate ( CHSysPartition, 640*526*4+10);
	if(NULL!=Bak_OSD_buffer)
		memset(Bak_OSD_buffer,0, 640*526 * 4 );
}

/*
功能说明：
	从(x,y)坐标开始，在屏幕上画一个矩形的点阵图到屏幕上。
参数说明：
	输入参数：
		x, y, w, h: 表示输出区域的位置和大小；
		bits: 输出区域数据起始地址；注意:bits 的内容是连续的，即从（x，y）
		开始，一整行接着下一整行（w_src），但矩形框所需要的内容不一定是连续的，
		即从（x，y）开始，取连续w 个像素，再从下一行开始。如下图所示。
		w_src: iPanel Middleware 内部缓冲区每行的宽度（以像素为单位）。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
其他说明：
	该函数在iPanel Middleware 正常运行过程当中频繁调用，用以更新OSD 上
	显示的数据区；要求运行的效率很高。另外，请注意，x，y 的坐标，有时在未知
	情况下，会超出可显示范围，在实现代码中务必添加越界的判断。
  */
INT32_T ipanel_porting_graphics_draw_image(INT32_T x, INT32_T y, INT32_T w, INT32_T h, BYTE_T *bits,INT32_T w_src)
{
#if 1
	int i,j,k;
	U32 uColor = 0xffff0000;
	U32	uRed, uGreen, uBlue;
	U32	*pBuff, *p32Src;
	U16	*pSrc;
	
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>draw_image start time=%d,x=%d,y=%d,w=%d,h=%d",ipanel_porting_time_ms(),x,y,w,h );
	#endif

	if(NULL==draw_buffer)
	{
		 return 0;
	}
	
#ifdef SUMA_SECURITY
		semaphore_wait(pst_OsdSema);
#endif
#ifndef USE_EIS_2D
#ifdef SUMA_SECURITY	
	if( CH_CheckSKBDisplay(x,y,w,h) )
	{
			do_report(0,"\n SumaSecure_SKB_updateSKB \n");
			SumaSecure_SKB_updateSKB();
			do_report(0,"\n SumaSecure_SKB_updateSKB over) \n");
	}
#endif
#endif
	if( x < 0 )
		x = 0;
	if( y < 0 )
		y = 0;
	if( ( x + w ) > EIS_REGION_WIDTH ) 
			w = EIS_REGION_WIDTH - x;
	if( ( y + h ) > EIS_REGION_HEIGHT )
			h = EIS_REGION_HEIGHT - y;


#ifdef CH_MUTI_RESOLUTION_RATE
{
	CH_RESOLUTION_MODE_e enum_Mode;
	int  i_DesX;
	int  i_DesY;
	enum_Mode = CH_GetReSolutionRate() ;

	 if(HighSolution_change != enum_Mode)
 	{
	
	       CH_SetReSolutionRate(HighSolution_change);
		CH_ChangeOSDViewPort_HighSolution(HighSolution_change,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
		enum_Mode=HighSolution_change;
		if((enum_Mode == CH_720X576_MODE)&&((w<act_src_w)||(h<act_src_h)))
		{
			CH_PutBakOsd();
		}
		eis_report("\n CH_ChangeOSDViewPort_HighSolution");
 	}

#ifdef EIS_OSD_YASUO
	if((enum_Mode == CH_1280X720_MODE)&&(NULL!=cvbs_draw_buffer)	)
	{
		int y_src,y_dst;
		
		if((y / 5 ) > 0)
		{
			y_src=(y / 5) * 5 - 1;
			y_dst=(y / 5) * 4 - 1;
			memcpy(cvbs_draw_buffer+y_dst*EIS_REGION_WIDTH*4,draw_buffer+y_src*EIS_REGION_WIDTH*4,EIS_REGION_WIDTH* 4 * 4);
			y_src+=5;
			y_dst+=4;
		}
		else
		{
			y_src=0;
			y_dst=0;

			memcpy(cvbs_draw_buffer,draw_buffer,EIS_REGION_WIDTH * 4 * 3);
			y_src+=4;
			y_dst+=3;
		}
		
		while((y_src-y)<h)
		{
			if((y_src+5)<g_IpanelDiaplyRect.h)
			{
				memcpy(cvbs_draw_buffer+y_dst*EIS_REGION_WIDTH*4,draw_buffer+y_src*EIS_REGION_WIDTH*4,EIS_REGION_WIDTH * 4 * 4);
				y_src+=5;
				y_dst+=4;
			}
			else
			{
				while(y_src<g_IpanelDiaplyRect.h)
				{
					memcpy(cvbs_draw_buffer+y_dst*EIS_REGION_WIDTH*4,draw_buffer+y_src*EIS_REGION_WIDTH*4,EIS_REGION_WIDTH * 4 );
					y_src+=1;
				}
				break;
			}
			
		}
		
	}
#endif
	
    if(enum_Mode == CH_OLD_MOE)
    	{
		CH6_PutRectangle ( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
				 g_IpanelDiaplyRect.x , y+ g_IpanelDiaplyRect.y,  g_IpanelDiaplyRect.w, h, draw_buffer );
		CH_DisplayCHVP(CH6VPOSD,CH6VPOSD.ViewPortHandle);
    	}
    else if(enum_Mode == CH_1280X720_MODE)
    	{
    	   i_DesX = x+g_IpanelDiaplyRect.x;
	   i_DesY = y+ g_IpanelDiaplyRect.y;
          CH_PutRectangle_HighSolution(  x+g_IpanelDiaplyRect.x , y+ g_IpanelDiaplyRect.y, w, h, draw_buffer,i_DesX,i_DesY);
    	}
	else
	{
	   i_DesX = x+g_IpanelDiaplyRect.x+EIS_REGION_STARTX;
	   i_DesY = y+ g_IpanelDiaplyRect.y+EIS_REGION_STARTY;
          CH_PutRectangle_HighSolution(  x+g_IpanelDiaplyRect.x , y+ g_IpanelDiaplyRect.y, w, h, draw_buffer,i_DesX ,i_DesY);
	}
}
#else
	CH6_PutRectangle ( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
					EIS_REGION_STARTX, y+EIS_REGION_STARTY, w_src, h, draw_buffer );
	CH_DisplayCHVP(CH6VPOSD,CH6VPOSD.ViewPortHandle);
#endif	

#ifdef SUMA_SECURITY
		semaphore_signal(pst_OsdSema);
#endif

#ifdef __EIS_API_OSD_DEBUG__
eis_report ( "\n++>eis ipanel_porting_graphics_draw_image end time=%d",ipanel_porting_time_ms());
#endif
	return 0;
#endif
}


/*
功能说明：
	对Graphics 进行一个操作，或者用于设置和获取Graphics 设备的参数和属性。
参数说明：
	输入参数：
		op － 操作命令
		typedef enum
		{
			IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY =1,
		} IPANEL_GRAPHICS_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
	op, arg 取值见下表：
	op 									arg 											说明
	
										指向IPANEL_GRAPHICS_WIN_RECT				通知外部
										结构的指针;							模块，浏览
										typedef struct {								器实际输
												int x,								出的窗口
	IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY			int y,								的大小和
												int w,								位置。
												int h,
										}IPANEL_GRAPHICS_WIN_RECT;
	
	输出参数：无
	返 回：
	IPANEL_OK:成功;
	IPANEL_ERR:失败。
  */
INT32_T ipanel_porting_graphics_ioctl(IPANEL_GRAPHICS_IOCTL_e op, VOID *arg)
{
	IPANEL_GRAPHICS_WIN_RECT *p_Struct;
	static short win_h_temp=0;
	eis_report ( "\n++>eis ipanel_porting_graphics_ioctl op=%d",op );
	
	switch ( op )
	{
		case IPANEL_GRAPHICS_AVAIL_WIN_NOTIFY:
		p_Struct = ( IPANEL_GRAPHICS_WIN_RECT* )arg;
		eis_report("\n ipanel_porting_graphics_ioctl p_Struct->h =%d",p_Struct->h );
	
      		if(p_Struct->h  <= 576 )/*标清OSD*/
	      	{
			g_IpanelDiaplyRect.x = p_Struct->x;
			g_IpanelDiaplyRect.y = p_Struct->y;
			g_IpanelDiaplyRect.w = p_Struct->w;
			g_IpanelDiaplyRect.h = p_Struct->h;

			act_src_w=p_Struct->w-1;
			act_src_h=p_Struct->h-1;
			if(win_h_temp==0)
			{
				win_h_temp=act_src_h;
			}

			if((win_h_temp!=act_src_h)&&(HighSolution_change==CH_720X576_MODE))
			{

				CH_ChangeOSDViewPort_HighSolution(HighSolution_change,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
			}

			HighSolution_change=CH_720X576_MODE;
			win_h_temp=act_src_h;
			CH_RestoreOsd();

	      	}
  		else
		{                           /*高清OSD界面*/
			g_IpanelDiaplyRect.x = p_Struct->x;
			g_IpanelDiaplyRect.y = p_Struct->y;
			g_IpanelDiaplyRect.w = p_Struct->w;
			g_IpanelDiaplyRect.h = p_Struct->h;
			// CH_DeleteOSDViewPort_HighSolution();
#if 0
			CH_SetReSolutionRate(CH_1280X720_MODE);
			CH_ChangeOSDViewPort_HighSolution(CH_1280X720_MODE,pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
#endif
			HighSolution_change=CH_1280X720_MODE;
       	} 
		return IPANEL_OK;
		break;
	default:
		return IPANEL_ERR;
		break;
	}
	return IPANEL_ERR;
}

/*
功能说明：
	获取鼠标的当前位置，屏幕左上角顶点为原点。
参数说明：
	输入参数：
		无。
	输出参数：
		x，y 坐标值。
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_cursor_get_position(INT32_T *x, INT32_T *y)
{
	#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_cursor_get_position 未完成" );
	#endif

	return IPANEL_OK;
}

/*
功能说明：
	设置鼠标的位置。x, y 相对于屏幕左上角顶点的位置。
参数说明：
	输入参数：
		x，y：坐标值。
	输出参数：无；
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_cursor_set_position(INT32_T x,INT32_T y)
{
#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_cursor_set_position 未完成" );
#endif

	return IPANEL_OK;
}

/*
功能说明：
	获取当前鼠标/光标的形状代码。（1 ~ 4）1：箭头形状，2：手形状，3：沙漏形状，
	4：“I”形状。
参数说明：
	输入参数：无
	输出参数：无
	返 回：
		>=0:鼠标/光标的形状代码。
		IPANEL_ERR: 函数执行失败。
  */
INT32_T ipanel_porting_cursor_get_shape(VOID)
{
#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_cursor_get_shape 未完成" );
#endif

	return 0;
}

/*
功能说明：
	设置鼠标/光标的类型。（1 ~ 4）1：箭头形状，2：手形状，3：沙漏形状，4：“I”
	形状。
参数说明：
	输入参数：
		shape：鼠标/光标的类型。（1 ~ 4）有效,其他无效。
	输出参数：无；
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_cursor_set_shape(INT32_T shape)
{
#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_cursor_set_shape 未完成" );
#endif

	return 0;
}

/*
功能说明：
	控制鼠标/光标是否可见。
参数说明：
	输入参数：
		flag：0、隐藏光标；1、显示光标。
	输出参数：无；
	返 回：
		IPANEL_OK: 函数执行成功;
		IPANEL_ERR: 函数执行失败。
*/
INT32_T ipanel_porting_cursor_show(INT32_T flag)
{
#ifdef __EIS_API_OSD_DEBUG__
	eis_report ( "\n++>eis ipanel_porting_cursor_show 未完成" );
#endif

	return 0;
}


#define EIS_EEPROM_ADDR	256	
#define EIS_EEPROM_SIZE		0x1000	/* 4K */

int ipanel_porting_eeprom_burn(U32 addr, CONST BYTE_T *buf_addr, U32 len)
{
	if(buf_addr!=NULL)
	{
		eis_report ( "\n++>eis ipanel_porting_eeprom_burn addr=%x, buf_addr=%x,len=%d", addr,buf_addr,len);
		WriteNvmData(addr,len,buf_addr);
	}
	return IPANEL_OK;
}

int ipanel_porting_eeprom_read(U32 addr,BYTE_T *buf_addr, U32 len)
{
	if(buf_addr!=NULL)
	{
		ReadNvmData(addr,len,buf_addr);
		eis_report ( "\n++>eis ipanel_porting_eeprom_read addr=%x, buf_addr=%x,len=%d", addr,buf_addr,len);
	}
	return IPANEL_OK;
}

int ipanel_porting_eeprom_info(BYTE_T **addr, U32 *size)
{
	if((addr!=NULL )&&(size!=NULL))
	{
		*addr=(BYTE_T *)EIS_EEPROM_ADDR;
		*size=EIS_EEPROM_SIZE;
		eis_report ( "\n++>eis ipanel_porting_eeprom_info addr=%x, size=%d", *addr,*size);
	}
	return IPANEL_OK;
}
/*切换制式时恢复所有的OSD菜单*/
void CH_RstoreALLOSD(void)
{
     CH_RESOLUTION_MODE_e enum_Mode;
     int  i_DesX;
     int  i_DesY;
     int y = 0;
     int h = 0;
#ifdef USE_EIS_2D
	 ipanel_porting_graphics_update_osd( 0,0,g_IpanelDiaplyRect.w,g_IpanelDiaplyRect.h);
	 return;
#else	 
	enum_Mode = CH_GetReSolutionRate() ;
	if(enum_Mode == CH_OLD_MOE)
	{
		CH6_PutRectangle ( GFXHandle, CH6VPOSD.ViewPortHandle, &gGC, 
		 0, 0,  g_IpanelDiaplyRect.w, g_IpanelDiaplyRect.h, draw_buffer );
		CH_DisplayCHVP(CH6VPOSD,CH6VPOSD.ViewPortHandle);
	}
	else if(enum_Mode == CH_1280X720_MODE)
	{
		CH_PutRectangle_HighSolution( 0 , 0, g_IpanelDiaplyRect.w, g_IpanelDiaplyRect.h, draw_buffer,0,0);
	}
	else
	{
		CH_PutRectangle_HighSolution(  0 , 0, g_IpanelDiaplyRect.w, g_IpanelDiaplyRect.h, draw_buffer,0 ,0);
	}
#endif
}
  /*恢复显示数据*/
  /*切换制式时恢复所有的OSD菜单*/
void CH_RestoreOsd()
  {
  	int i,j,k;
	if(NULL==Bak_OSD_buffer)
		return;
	
	for(k=0;k<2;k++)
	{
		for(i=0;i<263;i++)
		{
			for(j=0;j<640;j++)
			{
				memcpy(Bak_OSD_buffer+k*263*640*4+i*640*4+j*4,
					draw_buffer+k*360*EIS_REGION_WIDTH*4+/*(osd_640_526[i])*/(i*720/526)*EIS_REGION_WIDTH*4+j*2*4,4);
			}
		}
	}

  }
  
  void CH_PutBakOsd( )
{
	if(NULL==Bak_OSD_buffer)
		return;
	CH_PutBakOSD(  0 , 0, 640, 526, Bak_OSD_buffer,0,0);

}

void CH_SwitchIOOsd(int ri_index)
{
  	CH_ChangeOSDIO_HighSolution(CH_GetReSolutionRate(),pstBoxInfo->VideoOutputAspectRatio,pstBoxInfo->HDVideoTimingMode);
}

int CH_Switch_CVBS_Osd_y(int y)
{
	int yushu=y%5;
	int cvbs_y=0;
	switch(yushu)
	{
		case 0:
			cvbs_y=(y/5)*4;
			break;
		case 1:
		case 2:
			cvbs_y=(y/5)*4+1;
			break;
		case 3:
			cvbs_y=(y/5)*4+2;
			break;
		case 4:
			cvbs_y=(y/5)*4+3;
			break;
		default:
			break;
			
	}
	return cvbs_y;
}

#ifdef SUMA_SECURITY
void CH_Eis_DrawDot(int x,int y,int color)
{
	int*  Dot = NULL;
	Dot = (int*)((int)draw_buffer + y*g_IpanelDiaplyRect.w*4+x*4);
	*Dot = color;

}


void CH_Eis_drawLine (int x1, int y1,int x2,int y2, int color)
{
	int x,y;
	int temp;
	

	if(x1 == x2)/*竖线*/
	{
		if(y1 < y2)
		{
			temp = 1;
		}
		else
		{
			temp = -1;
		}

		x = x1;
		y = y1;
		while(y != y2)
		{
			CH_Eis_DrawDot(x,y,color);
			y += temp;
		}

	}
	else if( y1 == y2)
	{
		if(x1 < x2)
		{
			temp = 1;
		}
		else
		{
			temp = -1;
		}

		x = x1;
		y = y1;
		while(x != x2)
		{
			CH_Eis_DrawDot(x,y,color);
			x += temp;
		}	


	}

	
}
void CH_Eis_DrawRectangle (int x, int y, int w, int h, int color)
{
    	int i,i_y;
	for(i  = 0 ;i <= h ; i ++)
	{
		i_y = y+i;
		CH_Eis_drawLine( x,  i_y, (x+w), i_y, color);
	}

}
#ifndef USE_EIS_2D
U8* CH_Eis_GetRectangle(int PosX, int PosY, int Width, int Height)
{
  	U8*  pTemp = NULL;
	int     i_bufsize = Width*Height*4;
	int i;
	U8* pStart;

	U8* temp;
	
	pTemp=memory_allocate(CHSysPartition,i_bufsize);
	if(pTemp==NULL) 	
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
		temp = pTemp;
		memset(pTemp,0,i_bufsize);
	}
	pStart = draw_buffer + PosY*g_IpanelDiaplyRect.w*4+PosX*4;
	for(i = 0;i < Height;i++)
	{
		memcpy(pTemp,pStart, Width*4);
		pTemp += Width*4;
		pStart +=  g_IpanelDiaplyRect.w*4;	
	}
	
	pTemp = pTemp -Height*Width*4;
	if(pTemp != temp)
	{
		return NULL;
	}

	return  pTemp;
}
#endif

BOOL CH_Eis_PutRectangle( int PosX, int PosY,int Width,int Height,U8* pBMPData)
{
	U8*  pTemp = pBMPData;

	int i;
	U8* pStart;
	

	pStart = draw_buffer + PosY*g_IpanelDiaplyRect.w*4+PosX*4;
	for(i = 0;i < Height;i++)
	{
		memcpy(pStart,pTemp, Width*4);
		pTemp += Width*4;
		pStart +=  g_IpanelDiaplyRect.w*4;	
	}
	  	
	return FALSE;
}


BOOL CH_Eis_DisplayPicdata( int PosX, int PosY,int Width,int Height,U8* pBMPData)
{
	U8*  pTemp = pBMPData;

	int i,j;
	U8* pStart;
	

	pStart = draw_buffer + PosY*g_IpanelDiaplyRect.w*4+PosX*4;
	for(i = 0;i < Height;i++)
	{
		for(j = 0;j < Width;j++)
		{
			if(*(U32*)pTemp != 0X80000000)
			{
				memcpy(pStart,pTemp,4);
			}
			
			pTemp += 4;
			pStart +=  4;	
		}
		pStart += ( g_IpanelDiaplyRect.w -Width)*4;	
	}
	 ipanel_porting_graphics_draw_image( PosX, PosY,Width, Height, NULL, EIS_REGION_WIDTH);
	return FALSE;
}

#endif


/*--eof----------------------------------------------------------------------------------------------------*/

