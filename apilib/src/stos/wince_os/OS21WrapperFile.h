/*! Time-stamp: <@(#)OS21WrapperFile.h   5/11/2005 - 6:29:26 PM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperFile.h
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 5/11/2005
 *
 *  Purpose :  Declaration for file  
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  5/11/2005  WH : First Revision
 *
 *********************************************************************
 */


#ifndef __OS21WrapperFile__
#define __OS21WrapperFile__

#define _O_RDONLY       0x0000  /* open for reading only */
#define _O_WRONLY       0x0001  /* open for writing only */
#define _O_RDWR         0x0002  /* open for reading and writing */
#define _O_APPEND       0x0008  /* writes done at eof */
#define _O_TEXT         0x4000  /* file mode is text (translated) */
#define _O_BINARY       0x8000  /* file mode is binary (untranslated) */

#define O_RDONLY        _O_RDONLY
#define O_WRONLY        _O_WRONLY
#define O_RDWR          _O_RDWR
#define O_APPEND        _O_APPEND
#define O_TEXT          _O_TEXT
#define O_BINARY        _O_BINARY



struct stat {
        unsigned short st_mode;
        short st_nlink;
        short st_uid;
        short st_gid;
        int   st_rdev;
        int   st_size;
        time_t st_atime;
        time_t st_mtime;
        time_t st_ctime;
        };

#if  WINCE_MASTER != 1
	int  open(const char *pname, int mode, ...);
	int  read(int handle, void *pbuffer, unsigned int size);
	int  write(int handle, void *pbuffer, unsigned int size);
	int  close(int handle);
	int  fstat(int handle , struct stat *pstat);

	#define fopen(a,b)	fopenCE(a,b)
	FILE *fopenCE( const char *filename, const char *mode );
	void SetFopenBasePath(const char *pBasePathName);
#else

	#define   open(a,b) (-1)
	#define   read(a,b,c) (0)
	#define   write(a,b,c) (0)
	#define   close(a) (0)
	#define	  fstat(a,b) (0)
	#define   SetFopenBasePath(a) 

#endif
	
#endif
