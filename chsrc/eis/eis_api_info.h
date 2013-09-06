/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_info.h
  * 描述: 	定义info相关的接口
  * 作者:	蒋庆洲
  * 时间:	2008-10-22
  * ===================================================================================
  */

#ifndef __EIS_API_INFO__
#define __EIS_API_INFO__
#define UI_ADDRESS 0xA0B00000
#define UI_SIZE  0x400000

/*
功能说明：
	获取系统的相关信息，以字符串的方式返回，一般是要在页面上作显示用的。Name
	参数为一枚举值（见下表），该值可能会增加，集成时或更新库文件时请以头文件为准。
参数说明：
	输入参数：
		name：希望获取的信息类型（见上表中的定义，使用时以头文件为准）
		buf：用来保存获取信息的buffer 的地址
		len：用来保存获取信息的buffer 的长度
	输出参数：无
	返 回：
		> 0:获取信息的实际长度;
		IPANEL_ERR:没有获取到指定信息
  */
extern INT32_T ipanel_porting_system_get_info ( IPANEL_STB_INFO_e name, CHAR_T *buf, INT32_T len);

/*
功能说明：
	如果将iPanel Middleware 的内部数据(字库，UI 之类)独立存储在 Flash 内存中,
	则必须实现该函数；内部数据由 iPanel 在发布iPanel Middleware 库文件的同时提
	供, 由 STB 集成商独立存放进 Flash 内存中，然后通过正确实现该函数，告诉
	iPanel Middleware 相关数据的存储信息。
 参数说明:
	输出参数：
		address：返回iPanel Middleware 请求数据的存储起始地址；
		size：返回iPanel Middleware 请求数据的长度。
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_get_outside_dat_info(CHAR_T **address, INT32_T *size, IPANEL_RESOURCE_TYPE_e type );

#endif

/*--eof------------------------------------------------------------------------------------------------*/

