#include "CH_sys.h"
#include "initterm.h"
/* ========================================================================
Name:        SYS_FOpen
Description: Open a file
======================================================================== */

void *SYS_FOpen(const char *filename,const char *mode)
{
	SYS_FileHandle_t *File;
	
	/* Allocate a file handle descriptor */
	/* ================================= */
	File=Memory_Allocate(SystemPartition,sizeof(SYS_FileHandle_t));
	if (File==NULL)
	{
		return(NULL);
	}
	
	/* If filename start with a '/', we assume it's the virtual file system */
	/* ==================================================================== */
#if defined(OSPLUS)
	if (filename[0]=='/')
	{
		U32 i,j;
		char local_mode[8];
		File->FileSystem = 1; /* For VFS */
		/* Remove 'b'inary flag from the mode string for the virtual file system */
		for (i=0,j=0;i<strlen(mode);i++)
		{
			if (mode[i]!='b') local_mode[j++]=mode[i];
		}
		local_mode[j]='\0';
		File->FileHandle = (void *)vfs_fopen(filename,local_mode);
	}
#endif
	
	/* For JTAG OS20 */
	/* ============= */
#if defined(ST_OS20)
	if (filename[0]!='/')
	{
		File->FileSystem = 2;
		File->FileHandle = (SYS_FileHandle_t *)debugopen(filename,mode);
		if (File->FileHandle==((SYS_FileHandle_t *)0xFFFFFFFF)) File->FileHandle=0; 
	}
#endif
	
	/* For JTAG 0S21 */
	/* ============= */
#if defined(ST_OS21)
	if (filename[0]!='/')
	{
		File->FileSystem = 2;
		File->FileHandle = (SYS_FileHandle_t *)fopen(filename,mode);
	}
#endif
	
	/* Return file system */
	/* ================== */
	if (File->FileHandle==NULL)
	{
		Memory_Deallocate(SystemPartition,File);
		File=NULL;
	}
	
	/* Return file handle */
	/* ================== */
	return(File); 
}


/* ========================================================================
Name:        SYS_FClose
Description: Close a file
======================================================================== */

U32 SYS_FClose(void *FileContext)
{
	U32 ReturnValue;
	SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;
	
	/* VFS File system */
	/* =============== */
#if defined(OSPLUS)
	if (File->FileSystem==1)
	{
		ReturnValue=vfs_fclose(File->FileHandle);
	}
#endif
	
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	if (File->FileSystem==2)
	{
		ReturnValue=(U32)debugclose((S32)File->FileHandle);
	}
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	if (File->FileSystem==2)
	{
		ReturnValue=fclose(File->FileHandle);
	}
#endif
	
	/* Free the handle */
	/* =============== */
	Memory_Deallocate(SystemPartition,File);
	return(ReturnValue);
}

/* ========================================================================
Name:        SYS_FRead
Description: Read datas from file 
======================================================================== */

U32 SYS_FRead(void *ptr,U32 size,U32 nmemb,void *FileContext)
{
	U32 ReturnValue;
	SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;
	
	/* VFS File system */
	/* =============== */
#if defined(OSPLUS)
	if (File->FileSystem==1)
	{
		ReturnValue=vfs_fread(ptr,size,nmemb,File->FileHandle);
	}
#endif
	
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	if (File->FileSystem==2)
	{
		ReturnValue=(U32)debugread((S32)File->FileHandle,ptr,size*nmemb);
		ReturnValue=ReturnValue/size;
	}
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	if (File->FileSystem==2)
	{
		ReturnValue=fread(ptr,size,nmemb,File->FileHandle);
	}
#endif
	
	/* Return nb bytes read */
	/* ==================== */
	return(ReturnValue);
}

/* ========================================================================
Name:        SYS_FWrite
Description: Write datas to file 
======================================================================== */

U32 SYS_FWrite(void *ptr,U32 size,U32 nmemb,void *FileContext)
{
	U32 ReturnValue;
	SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;
	
	/* VFS File system */
	/* =============== */
#if defined(OSPLUS)
	if (File->FileSystem==1)
	{
		ReturnValue=vfs_fwrite(ptr,size,nmemb,File->FileHandle);
	}
#endif
	
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	if (File->FileSystem==2)
	{
		ReturnValue=(U32)debugwrite((S32)File->FileHandle,ptr,size*nmemb);
		ReturnValue=ReturnValue/size;
	}
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	if (File->FileSystem==2)
	{
		ReturnValue=fwrite(ptr,size,nmemb,File->FileHandle);
	}
#endif
	
	/* Return nb bytes read */
	/* ==================== */
	return(ReturnValue);
}

/* ========================================================================
Name:        SYS_FSeek
Description: Move file position 
======================================================================== */

U32 SYS_FSeek(void *FileContext,U32 offset,U32 whence)
{
	U32 ReturnValue;
	SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;
	
	/* VFS File system */
	/* =============== */
#if defined(OSPLUS)
	if (File->FileSystem==1)
	{
		ReturnValue=vfs_fseek(File->FileHandle,offset,whence);
	}
#endif
	
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	if (File->FileSystem==2)
	{
		ReturnValue=(U32)debugseek((S32)File->FileHandle,offset,whence);
	}
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	if (File->FileSystem==2)
	{
		ReturnValue=fseek(File->FileHandle,offset,whence);
	}
#endif
	
	/* Return nb bytes read */
	/* ==================== */
	return(ReturnValue);
}

/* ========================================================================
Name:        SYS_FTell
Description: Get file position 
======================================================================== */

U32 SYS_FTell(void *FileContext)
{
	U32 ReturnValue;
	SYS_FileHandle_t *File=(SYS_FileHandle_t *)FileContext;
	
	/* VFS File system */
	/* =============== */
#if defined(OSPLUS)
	if (File->FileSystem==1)
	{
		ReturnValue=vfs_ftell(File->FileHandle);
	}
#endif
	
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	if (File->FileSystem==2)
	{
		ReturnValue=(U32)debugtell((S32)File->FileHandle);
	}
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	if (File->FileSystem==2)
	{
		ReturnValue=ftell(File->FileHandle);
	}
#endif
	
	/* Return nb bytes read */
	/* ==================== */
	return(ReturnValue);
}

/* ========================================================================
Name:        SYS_GetString
Description: Get a string from stdin 
======================================================================== */

U32 SYS_GetString(U8 *Buffer,U32 Size)
{
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	return((U32)debuggets(Buffer,Size));
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	{
		gets(Buffer);
		return(strlen(Buffer)+1);
	}
#endif
}

/* ========================================================================
Name:        SYS_GetKey
Description: Get a key pressed 
======================================================================== */

U32 SYS_GetKey(U32 *Key)
{
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	return((U32)debuggetkey((long int *)Key));
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	{
		*Key=(U32)(getchar());
		return(1);
	}
#endif
}

/* ========================================================================
Name:        SYS_PollKey
Description: Check if there is a key pressed 
======================================================================== */

U32 SYS_PollKey(U32 *Key)
{
	/* For OS20 */
	/* ======== */
#if defined(ST_OS20)
	return((U32)debugpollkey((long int *)Key));
#endif
	
	/* For OS21 */
	/* ======== */
#if defined(ST_OS21)
	{
		*Key=(U32)(getc(stdin));
		return(1);
		/*return(_SH_posix_poll_key(key));*/ 
	}
#endif
}


