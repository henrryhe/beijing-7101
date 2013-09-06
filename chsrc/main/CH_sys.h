
/*#include "stapp_os.h"
#include "initterm.h"
#include "sttbx.h"*/
#include "stcommon.h"
/*2007-01-09*/
typedef struct SYS_FileHandle_s
{
	U32   FileSystem;
	void *FileHandle;
} SYS_FileHandle_t;

extern void *SYS_FOpen(const char *filename,const char *mode);
extern U32 SYS_FRead(void *ptr,U32 size,U32 nmemb,void *FileContext);
extern U32 SYS_FSeek(void *FileContext,U32 offset,U32 whence);
extern U32 SYS_FClose(void *FileContext);
extern U32 SYS_FTell(void *FileContext);
