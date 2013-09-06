#ifndef	IPANEL_APP_DOWNLOADER_H__
#define IPANEL_APP_DOWNLOADER_H__

#ifdef __cplusplus
extern "C" {
#endif

enum{
	IPANEL_APP_EVENT_TYPE_NONE,
	IPANEL_APP_EVENT_TYPE_DOWNLOADER,//下载器消息
	IPANEL_APP_EVENT_TYPE_UNDEFINED
};

enum{
	IPANEL_APP_DOWNLOADER_NONE,
	IPANEL_APP_DOWNLOADER_START,//通知第三方应用已开始下载文件
	IPANEL_APP_DOWNLOADER_STOP,//通知第三方应用已停止下载文件
	IPANEL_APP_DOWNLOADER_SUCCESS,//通知第三方应用某个文件下载成功
	IPANEL_APP_DOWNLOADER_FAILED,//通知第三方应用某个文件下载失败
	IPANEL_APP_DOWNLOADER_UNDEFINED
};

struct FileInfo{
	char *name;
	char *buf;
	int32 len;
};

int32 ipanel_app_process_event(int msg, unsigned int p1, unsigned int p2);
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
/* 以下是通用下载器用，客户只G枰迪终獠糠纸涌冢渌慕涌贗PANEL会在库諫实现 */
enum {
	IPANEL_DOWNLOADER_UNDEF,
	IPANEL_DOWNLOADER_SUCCESS,
	IPANEL_DOWNLOADER_TIMEOUT,
	IPANEL_DOWNLOADER_NOT_FOUND,
	IPANEL_DOWNLOADER_SAVE_FAILED,
	IPANEL_DOWNLOADER_HALT
};

typedef INT32_T (*IPANEL_DOWNLOADER_NOTIFY)(VOID *tag, CONST CHAR_T *uri, INT32_T status);

INT32_T ipanel_download_file(VOID *handle, CONST CHAR_T *uri, UINT32_T timeout, IPANEL_DOWNLOADER_NOTIFY func, VOID *tag);
//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif /* IPANEL_APP_DOWNLOADER_H__ */

