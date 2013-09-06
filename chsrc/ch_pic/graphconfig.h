#ifndef _GRAPHCNFIG_H__/*图形层的输入输出环境定义*/
#define _GRAPHCNFIG_H__
#include "br_file.h"
#include "os_plat.h"
#define _USE_VECTORFONT_ /*使用矢量字体*/

/*#define ENABLE_CH_PRINT*/
#define SQ_MDF_20061103
/*
1。为解决AV INIT为音频时认为无效而跳出的问题
2。 为解决"新闻专递"里的 gif有个包一直搜不到,绘制后引起死机的问题,
	解决这个问题的方法是对于不完整的图片不绘制
	同时发现在不同服务里存在多个不同的对象具有相同的PID, 因此
	在索引对象时需要再增加一个PID的标石
3.  防止因信号不好数据收不完整, 增加了一倍的等待时间
*/


#define SQ_MDF_1218 
/*改如果上一个页面是跨频点播放， 而下一个页面的INIT＝＝3时， 不能正常播出的问题*/

#define MDF_070306
/*
1. 横向滚动字幕换行方式
2。STBNUM只处理属于长虹的序列号
3。SRC如果为0， 单向客户端不处理
*/

#define DISABLE_GIF_CACHE
/*
关闭gif动画缓存
*/

#define TXTLINK_SCROLL
/*
打开链接的字幕滚动效果
*/

#define H070529 
/*
1。 关闭返回时的搜索。
2。 在AV DAV VODAV失败后， 锁回当前频点
*/

#define H070714
/*
VOD播放后 继续判断是否成功

*/

#if 0
#ifndef NULL
#undef  NULL
#define NULL 0
#endif

/*无符号整数,根据不同的平台应该有所改变*/
#ifndef DEFINED_U8
#define DEFINED_U8
#define U8 unsigned char
#endif

#ifndef DEFINED_BOOL
#define DEFINED_BOOL
#define CH_BOOL int
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
#define U16 unsigned short
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
#define U32 unsigned  long
#endif


#ifndef UINT
#define UINT unsigned int/* 尽量少用，最好不用,而用U32类型替代，避免产生编译警告 */
#endif

/*符号整数，根据不同的平台应该有所改变*/
#ifndef DEFINED_S8
#define DEFINED_S8
#define S8 signed char
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
#define S16 signed short int
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
#define S32 signed  long int
#endif

#if 1
#ifndef INT
#define INT int /* 尽量少用，最好不用,而用S32类型替代，避免产生编译警告 */
#endif
#endif
#ifndef CH_FLOAT 
#define CH_FLOAT float
#endif
/*
#ifndef DEFINED_EOF
#define DEFINED_EOF
#define EOF -1
#endif
*/
#endif
#ifdef CH_BOOL
#undef CH_BOOL 
#define CH_BOOL int
#endif

/*CQ_BORWSER模块的打印控制， 置为1则打印 ， 置为0不输出打印*/
/*{{{*/
#define BR_BIDIRECT_C 		1
#define BR_CACHE_C 			1
#define BR_CQHTMLMAIN_C 	1
#define BR_RECEIVE_C 		1
#define BR_SOCKET_C 			0
#define BR_CQINFOSERVICE_C	0
#define BR_DRAWJPEG_C		0
#define J_AV_C				0

/*}}}*/
#endif


