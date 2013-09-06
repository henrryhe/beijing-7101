/******************************************************************************/
/*    Copyright (c) 2005 iPanel Technologies, Ltd.                            */
/*    All rights reserved. You are not allowed to copy or distribute          */
/*    the code without permission.                                            */
/*    This is the demo implenment of the basic type definitions needed by     */
/*    iPanel MiddleWare.                                                      */
/*                                                                            */
/*    此文件只能放置全局性的数据类型定义或者宏定义，不得依赖任何别的头文件!!! */	
/*										---huangsl  2008-02                   */
/******************************************************************************/

/*
** 任何struct/union体, 如果是模块私有的(不是用来在模块间按约定传递数据的), 严禁放在公共header里! 如, 可以放类型
** 定义: typedef struct tagMyStruct MyStruct; 但struct tagMyStruct的实体定义一定要放在模块内部的.c文件中!
*/

#ifndef __IPANEL_TYPEDEF_H__
#define __IPANEL_TYPEDEF_H__

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
/* generic type redefinitions */
typedef int					INT32_T;
typedef unsigned int		UINT32_T;
typedef short				INT16_T;
typedef unsigned short		UINT16_T;
typedef char				CHAR_T;
typedef unsigned char		BYTE_T;
#define CONST				const
#define VOID				void

typedef struct
{
    UINT32_T uint64_32h;
    UINT32_T uint64_32l;
}IPANEL_UINT64_T;

typedef struct
{
    UINT32_T uint64_32h;
    UINT32_T uint64_32l;
}IPANEL_INT64_T;

/* general return values */
#define IPANEL_OK	0
#define IPANEL_ERR	(-1)
/* 根据讨论的结果, 应tujz的要求, 改一下这个宏的定义, 避免很多的编译问题. --McKing 2008-12-4
#define IPANEL_NULL (void *)0 */
#define IPANEL_NULL 0

/* general iPanel version values */
#define IPANEL_MAJOR_VERSION   3
#define IPANEL_MINOR_VERSION   0

/******************************************************************************/
typedef struct tagGeneralControl GeneralControl;
typedef struct STBContext STBContext;
typedef struct tagiPanelAppParams iPanelAppParams;

/** Abstract browser object. */
typedef struct tagiPanelBrowser iPanelBrowser;
/** Control parameters for \ref iPanelBrowser_new. */
typedef struct tagiPanelBrowserParams iPanelBrowserParams;
/* 
** other type definitions *****************************************************
*/

/* AV sources */
typedef enum 
{
	IPANEL_AV_SOURCE_DEMUX,	//从Demux模块输入数据
	IPANEL_AV_SOURCE_MANUAL	//系统软件推入的数据
} IPANEL_AV_SOURCE_TYPE_e;

/* iPanel switch modes */
typedef enum{
	IPANEL_DISABLE,
	IPANEL_ENABLE
} IPANEL_SWITCH_e;

typedef enum
{
    IPANEL_DEV_USE_SHARED,               /* 和其他用户共享使用设备 */
    IPANEL_DEV_USE_EXCUSIVE              /* 独占使用设备 */
} IPANEL_DEV_USE_MODE;


/**************************Defines for  stream data exchange******************/
// 定义数据块释放函数指针
typedef VOID (*IPANEL_XMEM_FREE)(VOID *pblk);

typedef struct
{
	VOID *pdes;                          /* pbuf中的数据属性描述 */
	IPANEL_XMEM_FREE pfree;   /* IPANEL_XMEMBLK数据块释放函数指针 */    
	UINT32_T *pbuf;                      /* 数据缓冲区地址 */
	UINT32_T len;                        /* 缓冲区长度 */
} IPANEL_XMEMBLK, *pIPANEL_XMEMBLK;

/* XMEM block descriptor types*/
typedef enum
{
	IPANEL_XMEM_PCM = 1,
	IPANEL_XMEM_MP3	= 2,
	IPANEL_XMEM_TS	= 3,
	IPANEL_XMEM_ES	= 4,
	IPANEL_XMEM_GEN	= 5
} IPANEL_XMEM_PAYLOAD_TYPE_e;

typedef struct 
{
	IPANEL_XMEM_PAYLOAD_TYPE_e destype;    
	UINT32_T len;      /* buffer中数据长度 */
} IPANEL_XMEM_GEN_DES;


/* MP3 descriptor definition*/
typedef struct
{
	IPANEL_XMEM_PAYLOAD_TYPE_e destype;  /* 1，表示PCM数据描叙符。见流式数据描叙符类型定义 */
	UINT32_T samplerate;                 /* 采样频率 HZ */
	UINT16_T channelnum;                 /* 通道个数，1单声道，2双声道 */
	UINT16_T bitspersample;              /* 采样精度，bpp */
	UINT16_T bsigned;                    /* 有符号还是无符号, 1有符号，0无符号 */
	UINT16_T bmsbf;                      /* 是否高位在前（most significant bit first）。1，高位在前，0低位在前 */
	UINT32_T samples;                    /* 采样个数 */
} IPANEL_PCMDES, *pIPANEL_PCMDES;

/* ES descriptor definition*/
typedef struct 
{
	IPANEL_XMEM_PAYLOAD_TYPE_e destype;  /* IPANEL_XMEM_ES，表示ES数据描叙符。见流式数据描叙符类型定义 */
	UINT32_T timestamp;    /* 时间戳 ，以90K为单位[因为dvb是以33bit记录90k时钟的，是否有必要将单位定义成180K] */
	BYTE_T *ppayload;  /*有效数据起始地址*/   
	UINT32_T len;      /*有效数据长度 */
} IPANEL_XMEM_ES_DES;

/**************************Defines for  stream data exchange end *******************/


/******************************************************************************/
typedef struct _StbGlobalVar StbGlobalVar;
struct _StbGlobalVar {
	char *name;
	char *value;
	StbGlobalVar *next;
};


/******************************************************************************/
#ifndef MEDIA_URL_LENGTH
#define MEDIA_URL_LENGTH		255
#endif
#ifndef MEDIA_TYPE_LENGTH
#define MEDIA_TYPE_LENGTH		32
#endif

struct tagEisMediaInstanceCtx
{
#if defined(PROJECT_HUAWEI_SPECIAL)
#define MEDIA_URL_LENGTH 1024
	char url[MEDIA_URL_LENGTH];
	char mediaType[MEDIA_TYPE_LENGTH];
#else
#define MEDIA_URL_LENGTH 255
	char url[MEDIA_URL_LENGTH];
	char mediaType[MEDIA_TYPE_LENGTH];
#endif
};
typedef struct tagEisMediaInstanceCtx EisMediaInstanceCtx;

/******************************************************************************/
#ifdef HAVE_IPANEL_LOADER
typedef struct{
    unsigned int man_id;
    unsigned int hard_ver;
    unsigned int soft_ver;	
    unsigned int freq;
    unsigned int symbol;
    unsigned short qam;
    unsigned short pid;
    unsigned char tid;
    unsigned char load_seq;
    unsigned char used;
} iPanelOTADownloadPara;
#endif


#ifdef __cplusplus
}
#endif

#endif // __IPANEL_TYPEDEF_H__
