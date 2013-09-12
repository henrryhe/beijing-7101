/*! Time-stamp: <@(#)OS21WrapperFile.c   5/11/2005 - 6:32:38 PM   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperFile.c
 *
 *  Project : 
 *
 *  Package : 
 *
 *  Company : TeamLog 
 *
 *  Author  : William Hennebois            Date: 5/11/2005
 *
 *  Purpose :  Implementation of methods for file handling, because Win CE 
 *			    remove the libs open() read() etc  from stdlib, we have to 
 *				re-implement from file stream function 
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  5/11/2005  WH : First Revision
 * V 0.20  5/27/2005  WH : Add the network support
 *
 *********************************************************************
 */
#include "OS21WrapperFile.h"
#include <stdio.h>
#include "Winnetwk.h"

#undef fopen	// use of the real fopen

#define OLD_TARGET_BASE_PATH "Release/StapiBinImage"


#ifndef WINCE_NET_USER_PASS
	#define WINCE_NET_USER_PASS   share
#endif


#ifndef WINCE_NET_USER_LOGIN
	#define WINCE_NET_USER_LOGIN  windowscedevice
#endif



#ifdef STANDALONE
#define WINCE_TARGET_BASE_PATH ""
#else
#ifndef WINCE_TARGET_BASE_PATH
	#define WINCE_TARGET_BASE_PATH "\\\\GNB300817\\ShareRes\\PRJ-WINCE"
#endif
#endif
#ifndef WINCE_TARGET_SHARE_PATH
	#define WINCE_TARGET_SHARE_PATH "\\\\GNB300817\\ShareRes"
#endif


// for master, No overload of File System
#if  WINCE_MASTER != 1
static char gBasePathName[MAX_PATH]="";

static void PathConcat(char *pBuffer,LPCSTR pConcat)
{
	char *pEnd = strchr(pBuffer,0);
	// add '\\';
	pEnd[0] = '\\';
	pEnd[1] = 0;

	// remove separtor 
	if(pConcat[0] && (pConcat[0] == '\\' || pConcat[0] == '/')) pConcat++;
	strcat(pBuffer,pConcat);
	
}


static FILE *fopen_GoodSlash(LPCSTR pFileName, LPCSTR pmode)
{
	char tGoodSlashName[MAX_PATH],*pGoodSlash;
	pGoodSlash  = tGoodSlashName;
	while(*pFileName)
	{

		if(*pFileName == '/')
		{
			*pGoodSlash++ = '\\';
			pFileName++;
		}
		else
		{
			*pGoodSlash++ = *pFileName++;
		}



	}
	*pGoodSlash++= 0;

	return fopen(tGoodSlashName,pmode);
}



/*!-------------------------------------------------------------------------------------
 * Stdlib open emulation
 *
 * @param *pname : 
 * @param mode : 
 * @param ... : 
 *
 * @return int   : 
 */
int  open(const char *pname, int mode, ...)
{
	char pmode[5]="";
	FILE *pFile;
	if((mode & 7) == O_RDONLY) strcat(pmode,"r");
	if((mode & 7) == O_WRONLY) strcat(pmode,"w");
	if((mode & 7) == O_RDWR) strcat(pmode,"rw");
	if(mode & O_APPEND) strcat(pmode,"a");
	if(mode & O_BINARY) strcat(pmode,"b");
	pFile = fopenCE(pname,pmode);
	if(pFile == 0) return -1;
	// cast en opaque
	return (int)pFile;;
}

/*!-------------------------------------------------------------------------------------
 * Stdlib read emulation
 *
 * @param handle : 
 * @param *pbuffer : 
 * @param size : 
 *
 * @return int   : 
 */
int  read(int handle, void *pbuffer, unsigned int size)
{
	FILE *pFile;
	WCE_ASSERT(handle != -1);
	// opque cast
	pFile = (FILE *)handle;
	WCE_ASSERT(pFile);
	return fread(pbuffer,1,size,pFile);		
}

/*!-------------------------------------------------------------------------------------
 * Stdlib write emulation
 *
 * @param handle : 
 * @param *pbuffer : 
 * @param size : 
 *
 * @return int   : 
 */
int  write(int handle, void *pbuffer, unsigned int size)
{
	FILE *pFile;
	WCE_ASSERT(handle != -1);
	// opque cast
	pFile = (FILE *)handle;
	WCE_ASSERT(pFile);
	return fwrite(pbuffer,1,size,pFile);		

}

/*!-------------------------------------------------------------------------------------
 * Stdlib close emulation
 *
 * @param handle : 
 *
 * @return int   : 
 */
int  close(int handle)
{
	FILE *pFile;
	WCE_ASSERT(handle != -1);
	// opque cast
	pFile = (FILE *)handle;
	WCE_ASSERT(pFile);
	fclose(pFile);
	return 1;
}


/*!-------------------------------------------------------------------------------------
 * Stapi Use only the st_size field 
 *
 * @param handle : 
 * @param *pstat : 
 *
 * @return int   : 
 */
int  fstat(int handle , struct stat *pstat)
{
	int curSeek;
	memset(pstat,0,sizeof(struct stat));
	curSeek = ftell((FILE*)handle);
	fseek( (FILE*)handle,0,SEEK_END);
	pstat->st_size = ftell((FILE*)handle);
	fseek( (FILE*)handle,curSeek,SEEK_SET);
	return 0;
}



/*!-------------------------------------------------------------------------------------
 * Try to connect the network ressource
 *
 * @param *pName : 
 *
 * @return static void  : 
 */
static int CheckConnectDirectNetWork(const char *pName)
{
	
	if(strncmp(pName,"\\\\",2) == 0 || strncmp(pName,"//",2)==0) 
	{
		NETRESOURCE nr;
		int dwResult;
		char	pathconnect[_MAX_PATH];
		WCHAR   pathconnectW[_MAX_PATH];
		WCHAR   pLoginW[150];
		WCHAR   pPassW[150];

		char *pDst = pathconnect;
		
		strcpy(pathconnect,pName);
		// copy the server Name
		pName+=2; // skip //
		*pDst++ = '\\';
		*pDst++ = '\\';
		while(*pName && (*pName != '/' && *pName != '\\' ))
		{
			*pDst++ = *pName++;
		}
		
		if(*pName) *pDst++ = *pName++; // copy '/'

		// Add Shared  Folder
		while(*pName && (*pName != '/' && *pName != '\\' ))
		{
			*pDst++ = *pName++;
		}
		*pDst = 0;

		mbstowcs(pathconnectW,pathconnect,   _MAX_PATH);

		nr.dwScope		 = RESOURCE_GLOBALNET;                    
		nr.dwType		 = RESOURCETYPE_ANY ;                    
		nr.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;            
		nr.dwUsage		 = RESOURCEUSAGE_CONNECTABLE;            
		nr.lpLocalName   =  TEXT("");        
		nr.lpRemoteName  = pathconnectW;        
		nr.lpProvider	 = 0;      


		// Make a connection to the network resource.

		mbstowcs(pLoginW,WINCE_NET_USER_LOGIN,   150);
		mbstowcs(pPassW,WINCE_NET_USER_PASS,   150);

		dwResult = WNetAddConnection3 (
						0,                     // Handle to the owner window.
						&nr,                   // Retrieved from enumeration.
						pPassW,   // No password.
						pLoginW,  // Logged-in user.
						CONNECT_UPDATE_PROFILE);  // Update profile with
 
		
		if(dwResult) return FALSE;
		return TRUE;
		
	}
	return FALSE;
}

/*!-------------------------------------------------------------------------------------
 * Try to connect the network ressource
 *
 * @param *pName : 
 *
 * @return static void  : 
 */
static FILE * OpenShareResNetWork(const char *pName,const char *pMode)
{
	
	if(strncmp(pName,"\\\\",2) == 0 || strncmp(pName,"//",2)==0) 
	{

		FILE	*pfile=0;
		char	pathconnect[_MAX_PATH];
		char	*pDst;


		// name of the server becomes a folder
		strcpy(pathconnect,WINCE_TARGET_SHARE_PATH);
		// copy the server Name
		pName+=2; // skip //
		pDst = strchr(pathconnect,0);
		*pDst++ = '/';
		*pDst++ = 0;
		strcat(pathconnect,pName);
		pfile = fopen_GoodSlash(pathconnect,pMode);
		if(pfile==0)
		{
			if(CheckConnectDirectNetWork(pathconnect))
			{
				pfile = fopen_GoodSlash(pathconnect,pMode);
			}
			
		}

		return pfile;

		
	}
	return 0;
}



/*!-------------------------------------------------------------------------------------
 * <Detailed description of the method>
 *
 * @param *pfilename : 
 * @param *newpath : 
 * @param *basepath : 
 *
 * @return static void  : 
 */
static void ComputeFullPath(const char *pfilename,char *newpath)
{
	while(*pfilename && *pfilename == '.')
	{
		char *pend;
		if(strlen(pfilename) >= 2 && pfilename[1] == '.')
		{
			// simulate ../
			pfilename += 2;
			WCE_ASSERT(*pfilename == '/' || *pfilename == '\\'); // must be ../ or \ 
			pfilename++;
			// remove one level
			pend = strchr(newpath,0);
			while (pend != newpath && *pend != '/') pend--;
			*pend = 0;
		}
		else
		{
			// simulate ./ juste remove ./
			pfilename += 2;
		}

	}
	strcat(newpath,"/");
	strcat(newpath,pfilename);
}
/*!-------------------------------------------------------------------------------------
 * Because the path for all data file are different in the source code but also in the scripts, 
 * the fopen changes on fly the relatfis path and compute a full path using the function  SetFopenBasePath
 * @param *filename : 
 * @param *mode : 
 *
 * @return FILE  : 
 */
FILE *fopenCE( const char *pfilename, const char *pmode )
{
	FILE *pFile;
    char *s;

    if(strlen(gBasePathName)!= 0) 
	{
		char newpath[MAX_PATH]="";
		if(strlen(pfilename) > 1 && pfilename[0] == '.')
		{
			strcpy(newpath,WINCE_TARGET_BASE_PATH);
			strcat(newpath,gBasePathName);


			ComputeFullPath(pfilename,newpath);
			// convert all \ into /
			WCE_MSG(MDL_MSG,"Call fopen with real path of %s\n",newpath);
			// First chance to open the file 
			pFile =  fopen_GoodSlash(newpath,pmode);
			if(pFile ==0)
			{
				if(CheckConnectDirectNetWork(newpath))
				{
					// Second chance to open the file 
					pFile =  fopen_GoodSlash(newpath,pmode);
				}
				if(pFile ==0)
				{

					// if network doesn't work 
					// let's use the release folder
					strcpy(newpath,OLD_TARGET_BASE_PATH);
					strcat(newpath,gBasePathName);

					ComputeFullPath(pfilename,newpath);
					pFile =  fopen_GoodSlash(newpath,pmode);

				}
			}

		return pFile;
		}
		else
		{
			strcpy(newpath,WINCE_TARGET_BASE_PATH);
			PathConcat(newpath,pfilename);
			WCE_MSG(MDL_MSG,"Call fopen with real path of %s\n",newpath);
			// First chance to open the file 
			pFile =  fopen_GoodSlash(newpath,pmode);
			if(pFile ==0)
			{
				if(CheckConnectDirectNetWork(newpath))
				{
					// Second chance to open the file
					pFile =  fopen_GoodSlash(newpath,pmode);
					if(pFile==0)
					{
						// last chance 
						pFile = OpenShareResNetWork(pfilename,pmode);
					}
				}
				else
				{

					// if network doesn't work 
					// let's use the release folder
					strcpy(newpath,OLD_TARGET_BASE_PATH);
					PathConcat(newpath,pfilename);
					pFile =  fopen_GoodSlash(newpath,pmode);

				}

			}
			return pFile;
		}
	}
	else
	{
		return fopen_GoodSlash(pfilename,pmode);

	}
}

/*!-------------------------------------------------------------------------------------
 * Because win CE donsn't undersand the consept of current dir, all relatif path 
 *
 * @param none
 *
 * @return void  : 
 */
void SetFopenBasePath(const char *pBasePathName)
{
	char *pMap;
	char *pMapCached;
	LPVOID m_pPhysicalBufferAddr;
	ULONG  m_dwNormalPA;

	// assume that all path uses '/' separator
	// and BasePathNam start with '/'
	WCE_ASSERT(pBasePathName[0] == '/');
	strcpy(gBasePathName,pBasePathName);


	
	
	
}

#endif
