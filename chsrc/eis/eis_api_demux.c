/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_demux.c
  * 描述: 	实现单向数据收取相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-24
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "sectionbase.h"
/*#include "stpti4.h"*/

#include "eis_include\ipanel_demux.h"
#include "eis_api_demux.h"
#include "../main/initterm.h"
/*#define IPANEL_DEMUX_DEBUG*/

static IPANEL_DMX_NOTIFY_FUNC eis_oc_back_func;
static STPTI_Signal_t  			Eis_DataSignalHandle;
static semaphore_t				*pSemaAccessQueue = NULL;
extern STPTI_Handle_t     		PTIHandle;

#define AUDIO_TYPE_IPANEL 1111
#define VIDIO_TYPE_IPANEL 1112
#define PCR_TYPE_IPANEL      1113
U16 vPid =0x1fff,aPid=0x1fff,pcrPid=0x1fff;
#define EIS_DATA_QUEUE_COUNT	300

boolean eis_slot_state[EIS_SLOT_SUM];/* true: start  false: stop */
boolean eis_filter_state[EIS_FILTER_SUM];/* true: start  false: stop */
boolean eis_filter_enble[EIS_FILTER_SUM];/* true: start  false: stop */
int channel_list[EIS_SLOT_SUM];
/*
功能说明：
		设置demux 数据到达时的通知回调函数。
		当数据来的时侯通过这个回调函数，将数据发送给iPanel MiddleWare。底层应该保证
	回调函数返回之前，缓存中的数据不被改动，完整Section(PES packet)应该在一个连续的
	buffer 中，如果底层使用循环buffer 管理数据，当遇到循环buffer 的尾部时，必须要做
	处理，完成section(PES packet)的组装工作。
		由于iPanel MiddleWare 的回调函数中使用了信号量互斥机制，所以这个函数不能在中
	断中被调用，建议客户实现demux 数据的接收管理线程，然后再此线程中调用iPanel
	MiddleWare 的回调函数。
	回调函数的定义是：
		typedef VOID (*IPANEL_DMX_NOTIFY_FUNC)(UINT32_T channel, UINT32_T
		filter, BYTE_T *buf, INT32_T len)；
	各个参数说明如下：
		channel –通道过滤器句柄；
		filter – 二级过滤器句柄；
		buf– 底层存放的完整Section(PES packet)数据的起始地址；
		len – buf 中有效数据的长度。
参数说明：
	输入参数：
		func： 回调函数的入口地址；
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_demux_set_notify(IPANEL_DMX_NOTIFY_FUNC func)
{
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n time =%d, ipanel_porting_demux_set_notify ",get_time_ms());
#endif

	eis_oc_back_func = func;

	return IPANEL_OK;
}

/*
功能说明：
	创建一个通道过滤器。
参数说明：
	输入参数：
		poolsize：底层设置的该通道的缓存池的尺寸（如0x10000(64kB)等，可能实
		际分配缓存的时侯需要根据这个值调整）；建议每个通道的buffer
		不小于32KB。(64KB?)
		type: 通道类型，从下面的枚举类型中取值。
		typedef enum
		{
			IPANEL_DMX_DATA_PSI_CHANNEL = 0,
			IPANEL_DMX_DATA_PES_CHANNEL,
			IPANEL_DMX_VIDEO_CHANNEL,
			IPANEL_DMX_AUDIO_CHANNEL,
			IPANEL_DMX_PCR_CHANNEL
		} IPANEL_DMX_CHANNEL_TYPE_e;
	输出参数：无
	返 回：
		== IPANEL_NULL: 创建通道不成功；
		!= IPANEL_NULL: 返回一个通道过滤器的句柄。
*/
UINT32_T ipanel_porting_demux_create_channel(INT32_T poolsize, IPANEL_DMX_CHANNEL_TYPE_e type)
{
	int  channelID = 0;
	INT32_T realpoolsize;
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n time =%d, ipanel_porting_demux_create_channel been called poolsize is 0x%x,type=%d",get_time_ms(),poolsize,type);
#endif

	if ( IPANEL_DMX_DATA_PSI_CHANNEL == type
		|| IPANEL_DMX_IGNORE_CRC_CHANNEL == type)
	{
		channelID = CH6_Allocate_Section_Slot( Eis_DataSignalHandle );
		if(channelID == -1)
		{
			do_report(0,"\ncan't allocate slot for ipanel\n");
			return IPANEL_NULL;
		}
	}
	else if ( IPANEL_DMX_AUDIO_CHANNEL == type )
	{
		return AUDIO_TYPE_IPANEL;
	}
	else if ( IPANEL_DMX_VIDEO_CHANNEL == type )
	{
		return VIDIO_TYPE_IPANEL;
	}
	else if(  IPANEL_DMX_PCR_CHANNEL == type )
	{
		return PCR_TYPE_IPANEL;
	}	
	else
	{
		do_report(0,"\nipanel_porting_demux_create_channel not recognized type = %d\n", type);
		return IPANEL_NULL;
	}
#ifdef IPANEL_DEMUX_DEBUG
eis_report ( "\n ipanel_porting_demux_create_channel create ok channel = %d ",channelID+1);
#endif
	eis_slot_state[channelID]=false;
	return channelID+1;
}

/*
功能说明：
	将pid 设置到通道过滤器。
参数说明：
	输入参数：
		channel：通道过滤器句柄
		pid： 要过滤的pid
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_demux_set_channel_pid(UINT32_T channel,INT16_T pid)
{

#ifdef IPANEL_DEMUX_DEBUG
eis_report ( "\n time=%d, ipanel_porting_demux_set_channel_pid been called channel [%d],pid [%d]",get_time_ms(),channel,pid);
#endif

	if(channel<EIS_SLOT_SUM && channel>=0)
		channel_list[channel]=pid;

	if ( AUDIO_TYPE_IPANEL== channel ) 
	{
		aPid = pid;
		return IPANEL_OK; 
	}
	else if ( VIDIO_TYPE_IPANEL== channel )
	{
		vPid = pid;
		return IPANEL_OK; 
	}
	else if ( PCR_TYPE_IPANEL== channel )
	{
		pcrPid = pid;
		return IPANEL_OK; 
	}
	if(CH6_Set_Slot_Pid(channel-1, pid) == true)
	{
		eis_report ( "\n ipanel_porting_demux_set_channel_pid error channel== [%d],pid ==[%x]\n",channel,pid);
		return IPANEL_ERR;
	}
	return IPANEL_OK;
}

/*
功能说明：
	创建一个二级过滤器。
参数说明：
	输入参数：
		channel：通道过滤器句柄；
	输出参数：无
	返 回：
		＝＝IPANEL_NULL:创建不成功；
		!=IPANEL_NULL:返回一个二级过滤器的句柄。
*/
UINT32_T ipanel_porting_demux_create_filter(UINT32_T channel)
{
	int            channelID = channel;
	int            filterID = 0;
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n time=%d, ipanel_porting_demux_create_filter been called channel [%d]",get_time_ms(),channel);
#endif

	filterID = CH6_Allocate_Section_Filter(FOUR_KILO_SECTION, HTML_APP, false, true, false, NULL);
	if(filterID == -1)
	{
		eis_report ( " \n ipanel_porting_demux_create_filter can't create filter for channel[%d]",channel);
		return  0; 
	}
	if(CH6_Associate_Slot_Filter(channel-1, filterID))
	{
		eis_report ( "\nipanel_porting_demux_create_filter  associate filter and slot failed!");
		return  0; 
	}
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n ipanel_porting_demux_create_filter create ok ,channel [%d],fileter [%d]",channel,filterID+1);
#endif
	eis_filter_state[filterID]=false;
	eis_filter_enble[filterID]=false;

	return filterID+1;
}

/*
功能说明：
	修改一个二级过滤器的过滤参数。
参数说明：
	输入参数：
		channel： 通道过滤器句柄。
		filter： 二级过滤器的句柄；
		wide：过滤宽度；
		coef：需要过滤的section 头的匹配数组，该数组包含wide 个有效元素
		coef[0]~coef[wide－1]：数据满足data&mask == coef 的条件。
		coef[0]：二级过滤中的section 头的第一个字节，即table_id；
		coef[1]：二级过滤中的section 头的第二个字节；
		coef[2]：二级过滤中的section 头的第三个字节；
		coef[3]：二级过滤中的section 头的第四个字节；
		余此类推，直到第wide 个字节。
		mask：需要过滤的section 头的掩码数组，该数组包含wide 个有效元素
		mask[0]~mask[wide－1]：
		mask[0]：二级过滤中的section 头的第一个字节的mask；
		mask[1]：二级过滤中的section 头的第二个字节的mask；
		mask[2]：二级过滤中的section 头的第三个字节的mask；
		mask[3]：二级过滤中的section 头的第四个字节的mask；
		余此类推，直到第wide 个字节。
		excl ： 需要过滤的section 头的掩码数组， 该数组包含wide 个有效元素
		excl[0]~excl[wide－1]：对条件进行反向选择。
		excl[0]：二级过滤中的section 头的第一个字节的excl；
		excl[1]：二级过滤中的section 头的第二个字节的excl；
		excl[2]：二级过滤中的section 头的第三个字节的excl；
		excl[3]：二级过滤中的section 头的第四个字节的excl；
		余此类推，直到第wide 个字节。
		过滤逻辑满足下面公式：
		在excl==0 时：
		src&mask == coef&mask
		在excl!＝0 时：
		(src & (mask & ~excl))==(coef & (mask & ~excl))
		&& (src & (mask & excl))!=(coef & (mask & excl))
		使用方法举例：针对过滤section 数据的过滤器，传的wide 为11。如果需要将table id
		为0x4E 且Section 长度域后第一个字节的低4bit 不为1100B 的数据过滤出来，过滤器的过
		滤条件应该设置为：
		coef：[0x4E 0x00 0x00 0x0C 0x00 0x00(后面5byte 的0)]
		mask：[0xFF 0x00 0x00 0xFF 0x00 0x00(后面5byte 的0)]
		excl：[0x00 0x00 0x00 0x0C 0x00 0x00(后面5byte 的0)]
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。	
*/
INT32_T ipanel_porting_demux_set_filter(UINT32_T channel, UINT32_T filter, UINT32_T wide, BYTE_T coef[], BYTE_T mask[], BYTE_T excl[])
{
	BYTE_T myFilter[16],myMask[16],myPatern[16];
	int loop = 0;
	BOOL bReturn ;

#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n time=%d , ipanel_porting_demux_set_filter been called channel [%d],filter[%d],table_id=%d",get_time_ms(),channel,filter,coef[0]);
	eis_report ( "\n coef=%x , %x ,%x ,%x ,%x ,%x ,%x ,%x",coef[0],coef[1],coef[2],coef[3],coef[4],coef[5],coef[6],coef[7]);
	eis_report ( "\n mask=%x , %x ,%x ,%x ,%x ,%x ,%x ,%x",mask[0],mask[1],mask[2],mask[3],mask[4],mask[5],mask[6],mask[7]);
	eis_report ( "\n excl=%x , %x ,%x ,%x ,%x ,%x ,%x ,%x",excl[0],excl[1],excl[2],excl[3],excl[4],excl[5],excl[6],excl[7]);
#endif

	memset(myFilter,0,16);
	memset(myMask,0,16);
	memset(myPatern,0xff,16);
	myFilter[0]= coef[0];
	myMask[0] = mask[0];
	if(wide >14)
		wide = 14;
	myPatern[0] = ~excl[0];
	for(loop = 1; loop < wide;loop ++)
	{
		myFilter[loop] = coef[loop+2];
		myMask[loop] = mask[loop+2];
		myPatern[loop] = ~excl[loop+2];
	}
#ifdef CH_IPANEL_MGCA
      if(CH6_Get_Slot_Pid(channel-1) == 0x01)/*如果为CAT不处理，避免和CA冲突*/
      	{
	   bReturn = false;
      	}
     else
     	{
	   bReturn = CH6_Set_Filter(filter-1, (U8*)myFilter, (U8 *) myMask, (U8 *) myPatern, wide, /*true*/false);/*暂时用相等模式*/
     	}
#else
	bReturn = CH6_Set_Filter(filter-1, (U8*)myFilter, (U8 *) myMask, (U8 *) myPatern, wide, /*true*/false);/*暂时用相等模式*/
#endif
	if(bReturn == true)
	{
		eis_report ( "\nipanel_porting_demux_set_filter failed!");
		return IPANEL_ERR;

	}
	return IPANEL_OK;
}

/*
功能说明：
	启动一个通道过滤器来过滤数据；
参数说明：
	输入参数：
		channel： 通道过滤器句柄。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败
*/
INT32_T ipanel_porting_demux_start_channel(UINT32_T channel)
{
	INT16_T pid;
	short otherslot;
#ifdef IPANEL_DEMUX_DEBUG
eis_report ( "\n time=%d,  ipanel_porting_demux_start_channel been called channel [%d]",get_time_ms(),channel);
#endif
	if(channel == VIDIO_TYPE_IPANEL ||channel == AUDIO_TYPE_IPANEL||channel == PCR_TYPE_IPANEL)
		return IPANEL_OK;
	if(eis_slot_state[channel-1]==true)
		return IPANEL_OK;
#ifdef CH_IPANEL_MGCA
       if(CH6_Get_Slot_Pid(channel-1) == 0x01)/*如果为CAT不处理，避免和CA冲突*/
       {
          ;
      	 }
	  else
	  {
		if(CH6_Start_Slot(channel-1) == true)
		{
		
			eis_report ( "\nipanel_porting_demux_start_channel error channel== [%d]\n",channel);
			pid=CH6_Get_Slot_Pid(channel-1);
			otherslot=CH6_FindOtherSlotByPID(channel-1,pid);
			if(otherslot>-1)
			{
				CH6_Stop_Slot(otherslot);
				CH6_Start_Slot(channel-1) ;
			}
			return IPANEL_OK;
		}
	  }
#else	
	if(CH6_Start_Slot(channel-1) == true)
	{
		eis_report ( "\nipanel_porting_demux_start_channel error channel== [%d]\n",channel);
		return IPANEL_ERR;
	}
#endif	
	eis_slot_state[channel-1]=true;
	return IPANEL_OK;

}

/*
功能说明：
	停止一个接收数据的通道过滤器，让它不再过滤数据
参数说明：
	输入参数：
		channel： 通道过滤器句柄
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败
*/
INT32_T ipanel_porting_demux_stop_channel(UINT32_T channel)
{
#ifdef IPANEL_DEMUX_DEBUG
eis_report ( "\n ipanel_porting_demux_stop_channel been called channel [%d]",channel);
#endif
return IPANEL_OK;
#if 0 	/*sqzow20100720*/
	if(channel == VIDIO_TYPE_IPANEL ||channel == AUDIO_TYPE_IPANEL||channel == PCR_TYPE_IPANEL)
		return IPANEL_OK;
	if(CH6_Stop_Slot(channel-1) == true)
	{
		eis_report ( "\n ipanel_porting_demux_stop_channel error channel== [%d]\n",channel);
		return IPANEL_ERR;
	}
	return IPANEL_OK;
#endif

}

/*
功能说明：
	销毁一个通道过滤器。
参数说明：
	输入参数：
		channel：通道过滤器句柄
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
INT32_T ipanel_porting_demux_destroy_channel(UINT32_T channel)
{
#ifdef IPANEL_DEMUX_DEBUG
eis_report ( "\n ipanel_porting_demux_destroy_channel been called channel [%d]",channel);
#endif
	if(channel == VIDIO_TYPE_IPANEL ||channel == AUDIO_TYPE_IPANEL||channel == PCR_TYPE_IPANEL)
		return IPANEL_OK;
	CH6_Stop_Slot(channel-1) ;
	if(CH6_Free_Section_Slot(channel-1) == true)
	{
		eis_report ( "\nipanel_porting_demux_destroy_channel error channel== [%d]\n",channel);
		return IPANEL_ERR;
	}
	return IPANEL_OK;

}

/*
功能说明：
	使指定的二级过滤器不能再接收数据。
参数说明：
	输入参数：
		channel：通道过滤器句柄；
		filter：二级过滤器句柄
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
*/
INT32_T ipanel_porting_demux_disable_filter(UINT32_T channel,UINT32_T filter)
{
	boolean bErr ;
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n ipanel_porting_demux_disable_filter been called channel [%d],filter[%d]",channel,filter);
#endif

	eis_filter_enble[filter-1]=false;
	//return IPANEL_OK;
	bErr = CH6_Stop_Filter(filter-1);
	if(bErr)
	{
		eis_report ( "ipanel_porting_demux_disable_filter failed\n");
		return IPANEL_ERR;
	}
	return IPANEL_OK;
}

/*
功能说明：
	使指定的二级过滤器可以接收数据。
参数说明：
	输入参数：
		channel：通道过滤器句柄；
		filter：二级过滤器句柄
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败
*/
INT32_T ipanel_porting_demux_enable_filter(UINT32_T channel, UINT32_T filter)
{
	boolean bErr ;
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\ntime=%d, ipanel_porting_demux_enable_filter been called channel [%d],filter[%d]",get_time_ms(),channel,filter);
#endif
	eis_filter_enble[filter-1]=true;

	
	bErr = CH6_Start_Filter(filter-1);
	if(bErr)
	{
		eis_report ( "ipanel_porting_demux_enable_filter failed\n");
		return IPANEL_ERR;
	}
	eis_filter_state[filter-1]=true;
	return IPANEL_OK;
}

/*
功能说明：
	销毁指定的二级过滤器。
参数说明：
	输入参数：
		channel：通道过滤器句柄；
		filter：二级过滤器句柄
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
INT32_T ipanel_porting_demux_destroy_filter(UINT32_T channel, UINT32_T filter)
{
	boolean bErr ;
#ifdef IPANEL_DEMUX_DEBUG
	eis_report ( "\n ipanel_porting_demux_destroy_filter been called channel [%d],filter[%d]",channel,filter);
#endif
	//CH6_Stop_Filter(filter-1);
	bErr = CH6_Free_Section_Filter(filter-1);
	if(bErr)
	{
		eis_report ( "ipanel_porting_demux_destroy_filter failed\n");
		return IPANEL_ERR;
	}
	return IPANEL_OK;
}

/*
功能说明：
	对Demux 进行一个操作，或者用于设置和获取Demux 设备的参数和属性。
参数说明：
	输入参数：
		op － 操作命令
		typedef enum
		{
			IPANEL_DEMUX_SET_CHANNEL_NOTIFY =1,
			IPANEL_DEMUX_SET_SRC,
			IPANEL_DEMUX_SET_STREAM_TYPE,
			IPANEL_DEMUX_GET_BUFFER,
			IPANEL_DEMUX_PUSH_STREAM,
			IPANEL_DEMUX_STC_FETCH_MODE,
		} IPANEL_DEMUX_IOCTL_e;
		arg – 操作命令所带的参数，当传递枚举型或32 位整数值时，arg 可强制转换
		成对应数据类型。
		op, arg 取值见移植文档附表：
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
INT32_T ipanel_porting_demux_ioctl(IPANEL_DEMUX_IOCTL_e op, VOID *arg)
{
	#ifdef  IPANEL_DEMUX_DEBUG
	eis_report ( "\++>eis ipanel_porting_demux_ioctl op=%d", op );
	#endif


	switch ( op )
	{
	case IPANEL_DEMUX_SET_CHANNEL_NOTIFY:		
	case IPANEL_DEMUX_SET_SRC:
	case IPANEL_DEMUX_SET_STREAM_TYPE:
	case IPANEL_DEMUX_GET_BUFFER:
	case IPANEL_DEMUX_PUSH_STREAM:
	case IPANEL_DEMUX_STC_FETCH_MODE:
		//eis_report ( "\n++>eis ipanel_porting_demux_ioctl 目前不支持" );
	case IPANEL_DEMUX_GET_CURRENT_STC:
		break;
	}

	return IPANEL_OK;
}

/* EisSectionMonitor task 参数*/
static void* g_EisSectionMonitorStack;
static tdesc_t g_EisSectionMonitorTaskDesc;
static task_t* gp_EisSectionMonitorTask;

/*
说明：Eis 数据监控进程
*/
boolean debug_control_demux = false;

static void Eis_DMX_SectionMonitor(void *ptr)
{
	ST_ErrorCode_t 	ErrCode;
	STPTI_Buffer_t 	BufferHandleTemp;
	STPTI_Filter_t 	MatchedFilterHandle[16];
	U32 				MatchedFilterCount = 0;
	int 				FilterIndexTemp;
	U8				*tempdata;
	int 				i;
	clock_t 			TimeTemp;
	BOOL  			SecCRCTemp	= false;

	U16				uSecLen, uIndex;
	U32				u_CRC;

	U8	pSecDataTemp[FOUR_KILO_SECTION];
	
	U32 SecLenRead	= 0;
	int lastsection=-1;
	  int channel,filter;

	#ifdef  IPANEL_DEMUX_DEBUG
	do_report(0, "\nEis_DMX_SectionMonitor Start!" );
	#endif

	TimeTemp = (clock_t)ST_GetClocksPerSecond() / 2;

	while( true )
	{
		/*do_report(0,"\n STPTI_SignalWaitBuffer  tm=%d ",get_time_ms());*/
		ErrCode = STPTI_SignalWaitBuffer( Eis_DataSignalHandle, &BufferHandleTemp, STPTI_TIMEOUT_INFINITY );

		#ifdef  IPANEL_DEMUX_DEBUG
		//eis_report ( "\n Eis_DMX_SectionMonitor: STPTI_SignalWaitBuffer()=%s", GetErrorText( ErrCode ) );/**/
		#endif

		if( ErrCode != ST_NO_ERROR ) 
		{
			STPTI_BufferFlush( BufferHandleTemp );	
			/*do_report(0,"\n STPTI_SignalWaitBuffer error! tm=%d ",get_time_ms());*/
			continue;
		}

		ErrCode = STPTI_BufferReadSection ( BufferHandleTemp, MatchedFilterHandle, 16, &MatchedFilterCount,
											&SecCRCTemp, (U8 *)pSecDataTemp,
											(FOUR_KILO_SECTION),
											NULL,0, &SecLenRead,
											STPTI_COPY_TRANSFER_BY_MEMCPY);
		if( ErrCode != ST_NO_ERROR )
		{
			STPTI_BufferFlush( BufferHandleTemp );	
			/*do_report(0,"\n STPTI_BufferReadSection error! tm=%d ",get_time_ms());*/
			continue;
		}
		if( SecCRCTemp == false )
		{
			/*do_report(0, "\nEis Section Monitor:CRC wrong in Filter tm=%d  index=%d\n",get_time_ms(),FilterIndexTemp );*/
			continue;
		}

		//semaphore_wait(gp_SemaEISDemux);
		for( i = 0; i < MatchedFilterCount; i ++ )
		{
			FilterIndexTemp = CH6_FindMatchFilter( FILTER_MATCH_BY_FILTERHANDLE, (void*)&MatchedFilterHandle[i] );

			if( FilterIndexTemp == -1 ) 
			{
				continue;
			}
			uSecLen = ( ( ( pSecDataTemp[1] & 0x0f ) << 8 ) | pSecDataTemp[2] ) + 3;
			u_CRC	= (pSecDataTemp[uSecLen-4] << 24) | (pSecDataTemp[uSecLen-3] << 16) | (pSecDataTemp[uSecLen-2] << 8) | pSecDataTemp[uSecLen-1];
			  channel = CH6_FindMatchSlot(SLOT_MATCH_BY_SLOTHANDLE,(void*)&SectionFilter[FilterIndexTemp].SlotHandle) + 1;
			  filter =  FilterIndexTemp+1;


			if ( (NULL != eis_oc_back_func ))
			{
				eis_oc_back_func(channel, filter, pSecDataTemp, SecLenRead );

			}
		}
		//semaphore_signal(gp_SemaEISDemux);
	}
}
Eis_DMX_Init()
{
	int 					index;
	eis_oc_back_func = NULL;
}
/*
说明：init EnReach DMX data
返回：OC_DEMUX_STATUS。
*/
BOOL Eis_Init(void)
{
	ST_ErrorCode_t           	ErrCode;
	int 					index;
	int					i_Error;
	static boolean			b_Init = false;
	
	eis_report( "\n Eis-->in < Eis_DMX_Init>" );

	pSemaAccessQueue = semaphore_create_fifo (1);
	Eis_DMX_Init();
	/*
	* initialise semaphores.
	*/
	ErrCode = STPTI_SignalAllocate ( PTIHandle, &Eis_DataSignalHandle );
	if ( ErrCode != ST_NO_ERROR )
	{
		eis_report ( "\nEis->DMX init STPTI_SignalAllocate=%s", GetErrorText( ErrCode ) );
		return 1;
	}
	
	if ( ( gp_EisSectionMonitorTask = Task_Create( Eis_DMX_SectionMonitor, NULL,
					  1024*256, 13,
					  "Eis_DMX_SectionMonitor", 0 ) ) == NULL )

	{
		eis_report ( "\n Eis_DMX_SectionMonitor creat failed\n");
		return;
	}
	
	b_Init = true;
	
	
//	return Eis_task_creat();/* lzf 20090323 add*/
	return 0;
}

/*
说明：init EnReach DMX data
返回：OC_DEMUX_STATUS。
*/
void Eis_DMX_term ( void )
{
 	int 					index;
	#ifdef  CH_DMX_EIS_PRINT
	do_report( 0, "\n++>eis->in < EnReach_DMX_term>" );
	#endif
	eis_oc_back_func = NULL;

}

extern STPTI_Signal_t gApplSignalHandle;
extern CH6_SECTION_SLOT_t SectionSlotTuner1[MAX_NO_SLOT];
extern CH6_SECTION_FILTER_t SectionFilterTuner1[MAX_NO_FILTER_TOTAL]; 
extern STPTI_Signal_t gSignalHandle;
void eis_bake(UINT32_T ch, UINT32_T filter, UINT32_T DataStartAddr, INT32_T len)
{
}
#include "..\dbase\vdbase.h"



/*--eof-----------------------------------------------------------------------------------------------------*/

