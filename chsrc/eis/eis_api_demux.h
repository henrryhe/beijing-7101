/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_demux.h
  * 描述: 	定义单向数据收取相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-24
  * ===================================================================================
  */

#ifndef __EIS_API_DEMUX__
#define __EIS_API_DEMUX__

#define EIS_FILTER_SUM 					48 /* filter的最大索引值 */
#define EIS_SLOT_SUM 					32	/* slot的最大索引值 */
#define ulMaxFilterNumber 				32
#define EIS_DMX_INVALID_CHANNEL_ID 	(0xffffffff) /* Invalid channel ID */

#define EIS_MEM_PIECE				4096	/* 一块数据的大小 */
#define EIS_MEM_NUM				250

typedef struct ipanel_mem_s
{
	//semaphore_t *         p_memsemaphore;
	int				channelindex;
	int                         filterindex;/* 是对应那个filter的索引*/
	U8				mem_node[EIS_MEM_PIECE];
	U16				mem_len;
	U8				uOccupy;			/* 是否占用 */
}eis_mem_t;


/* define filter struct */
typedef struct EIS_FILTER_s
{
	boolean 				enable; 			/* 是否已经打开 */
	boolean 				bOccupy; 		/* 是否占用 */
	short 				iActFilter; 		/* 实际分配的filter值*/
	signed char 			slotDemux; 		/* 对应的slot的索引 */
	unsigned char 		irFilterLength; 	/* 过滤器长度 */
	boolean 				notequal;
	short 				table_id;
	unsigned char 		coef[16]; 		/* 过滤器长度 */
	unsigned char 		mask[16]; 		/* 过滤器长度 */
	unsigned char 		excl[16]; 		/* 过滤器长度 */
} EIS_FILTER_t;

/* 定义irdeto使用的slot 结构 */
typedef struct EIS_SLOT_s
{
	boolean 						bOccupy; 				/* 是否占用 */
	short 						iActSlot; 				/* 真实值 */
	unsigned int 					PidValue; 				/* slot对应的pid值 */
	IPANEL_DMX_NOTIFY_FUNC 		fctcallback; 				/* 钩子函数 */
	unsigned char 				ucMaxNoofFilters; 		/* slot对应的filter的最大个数 */
	unsigned char 				ucNoofFiltersEngaged; 	/* slot使用的filter的个数 */
	unsigned char 				ucFiltersStartNum; 		/* slot使用的正在打开的 filter的个数 */
	boolean 						bInUse; 					/* True :in use ,false no use*/
	IPANEL_DMX_CHANNEL_TYPE_e	e_ChannelType;			/* channel的类型 */
}EIS_SLOT_t;

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
extern INT32_T ipanel_porting_demux_set_notify(IPANEL_DMX_NOTIFY_FUNC func);

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
extern UINT32_T ipanel_porting_demux_create_channel(INT32_T poolsize, IPANEL_DMX_CHANNEL_TYPE_e type);

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
extern INT32_T ipanel_porting_demux_set_channel_pid(UINT32_T channel,INT16_T pid);

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
extern UINT32_T ipanel_porting_demux_create_filter(UINT32_T channel);

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
extern INT32_T ipanel_porting_demux_set_filter(UINT32_T channel, UINT32_T filter, UINT32_T wide, BYTE_T coef[], BYTE_T mask[], BYTE_T excl[]);


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
extern INT32_T ipanel_porting_demux_start_channel(UINT32_T channel);

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
extern INT32_T ipanel_porting_demux_stop_channel(UINT32_T channel);

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
extern INT32_T ipanel_porting_demux_destroy_channel(UINT32_T channel);

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
extern INT32_T ipanel_porting_demux_disable_filter(UINT32_T channel,UINT32_T filter);


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
extern INT32_T ipanel_porting_demux_enable_filter(UINT32_T channel, UINT32_T filter);

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
extern INT32_T ipanel_porting_demux_destroy_filter(UINT32_T channel, UINT32_T filter);

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
extern INT32_T ipanel_porting_demux_ioctl(IPANEL_DEMUX_IOCTL_e op, VOID *arg);

/*
说明：init EnReach DMX data
返回：OC_DEMUX_STATUS。
*/
extern BOOL Eis_DMX_Init(void);


/*
说明：init EnReach DMX data
返回：OC_DEMUX_STATUS。
*/
extern void Eis_DMX_term ( void );

#endif

/*--eof-----------------------------------------------------------------------------------------------------*/

