/*
  * ===================================================================================
  * CopyRight By CHANGHONG NET L.T.D.
  * 文件: 	eis_api_file.c
  * 描述: 	实现文件系统相关的接口
  * 作者:	 刘战芬，蒋庆洲
  * 时间:	2008-10-23
  * ===================================================================================
  */

#include "eis_api_define.h"
#include "eis_api_globe.h"
#include "eis_api_debug.h"
#include "eis_api_file.h"
#include "eis_api_globe.h"

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
UINT32_T ipanel_porting_file_open(const CHAR_T*filename, const CHAR_T *mode)
{
		return IPANEL_NULL;
}

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
INT32_T ipanel_porting_file_close(UINT32_T fd)
{
	return IPANEL_OK;	
}

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
INT32_T ipanel_porting_file_delete(const CHAR_T *name)
{
	eis_report("\n ipanel_porting_file_delete ");
	
	return IPANEL_OK;
}

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
INT32_T ipanel_porting_file_read(UINT32_T fd, BYTE_T *buf, INT32_T len)
{
	eis_report("\n ipanel_porting_file_read ");

	return 0;
}

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
INT32_T ipanel_porting_file_write(UINT32_T fd,const CHAR_T *buf, INT32_T len)
{
		return IPANEL_ERR;
}

#if 0
/*
  * Func: CHMID_FILE_Init
  * Desc: 文件系统的初始化
  * Notic: 必须在系统初始化的时候调用
  */
void CHMID_FILE_Init ( void )
{
	int i;
	/* 创建互斥信号量 */
	gp_SemaFileSystem = semaphore_create_fifo ( 1 );

	semaphore_wait ( gp_SemaFileSystem );
	for ( i = 0; i < CHMID_FILE_MAX_NUM; i ++ )
	{
		gt_FileSystem[i].cp_FileName 			= NULL;
		gt_FileSystem[i].mp_Data				= NULL;
		gt_FileSystem[i].mp_SemaFile			= NULL;
		gt_FileSystem[i].mu32_CurrFileLen[0]	= 0;
		gt_FileSystem[i].mu32_CurrFileLen[1]	= 0;
		gt_FileSystem[i].mu32_CurrFileLen[2]	= 0;
		gt_FileSystem[i].mu32_CurrFileLen[3]	= 0;
		gt_FileSystem[i].mu32_CurrFileLen[4]	= 0;
		gt_FileSystem[i].mu32_TotalFileLen		= 0;
		gt_FileSystem[i].mu8_OpenCount		= 0;
	}
	Semaphore_Signal ( gp_SemaFileSystem );

	return;
}
#endif
/*--eof-------------------------------------------------------------------------------------------------*/

