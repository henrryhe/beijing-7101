/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_file.h
  * 描述: 	定义文件系统相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#ifndef __EIS_API_FILE__
#define __EIS_API_FILE__

#if 0
#define CHMID_FILE_MAX_NUM	100			/* 最多只能打开100个文件 */
#define CHMID_FILE_MAX_OPEN	5			/* 同时能打开文件的线程数 */
#define CHMID_DIRE_MAX_DEEP	100			/* 目录深度 */

/* 文件结构 */
typedef struct
{
	semaphore_t		*mp_SemaFile;
	char				*cp_FileName;
	U8				mu8_OpenCount;							/* 当前打开该文件的进程数量 */
	U32				mu32_TotalFileLen;
	U32				mu32_CurrFileLen[CHMID_FILE_MAX_OPEN];	/* 最多只能有5个进程同时打开一个文件 */
	U8				*mp_Data;								/* 文件实际数据指针 */
}chmid_file_t;
#endif

/*
功能说明：
	打开操作系统中的文件。如果文件不存在，要创建这个文件。如果文件所在目录不
	存在，需要创建这个目录。
参数说明：
	输入参数：
		filename：操作系统支持文件名称字符串；
		mode：打开模式字符串。
	输出参数：无
	返 回：
		！＝IPANEL_NULL：文件句柄；
		＝＝IPANEL_NULL:失败。
*/
extern UINT32_T ipanel_porting_file_open(const CHAR_T*filename, const CHAR_T *mode);

/*
功能说明：
	关闭打开的文件。
参数说明：
	输入参数：
		fd：文件句柄，由ipanel_porting_file_open 获得。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_file_close(UINT32_T fd);

/*
功能说明：
	删除指定的文件或目录，如果是目录应该是以“/”或“\”结尾。
参数说明：
	输入参数：
		name：路径＋文件名（目录名）。
	输出参数：无
	返 回：
		IPANEL_OK:成功;
		IPANEL_ERR:失败。
  */
extern INT32_T ipanel_porting_file_delete(const CHAR_T *name);


/*
功能说明：
	从文件的当前位置读取数据最大不超过len 的数据。
参数说明：
	输入参数：
		fd：ipanel_porting_file_open 函数返回的打开文件的句柄；
		buf：缓存区地址；
		len：缓存区长度。
	输出参数：
		buf：读取的数据。
	返 回：
		>＝0:实际读到数据的长度，0 或小于len，表示到文件结尾了。
		IPANEL_ERR:读出错
*/
extern INT32_T ipanel_porting_file_read(UINT32_T fd, BYTE_T *buf, INT32_T len);

/*
功能说明：
	写len 字节的数据到文件中。
参数说明：
	输入参数：
		fd：ipanel_porting_file_open 函数返回的打开文件的句柄；
		buf：数据块起始地址；
		len：数据块长度。
	输出参数：无。
	返 回：
		>＝0:实际写入数据的长度。；
		IPANEL_ERR:读出错
*/
extern INT32_T ipanel_porting_file_write(UINT32_T fd,const CHAR_T *buf, INT32_T len);

#endif

/*--eof-----------------------------------------------------------------------------------------------------*/

