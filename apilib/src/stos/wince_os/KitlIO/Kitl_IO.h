/***********************************************************************
 MODULE:   Kitl_IO.h

 PURPOSE: Header for KITL_IO Library implementing simple I/O port using ITL communication stream 
			
 COMMENTS: This lib use 2 ITL streams (1 in, 1 out) to allow communication with developpement station
			connecting using Plateform Builder KITL Transport 

 HISTORY :
 OLGN		18/05/05		Creation
***********************************************************************/

#if !defined WCE_KITLIO_H_INCLUDED_
#define WCE_KITLIO_H_INCLUDED_

#include <winbase.h>

#ifdef __cplusplus
extern "C"{
#endif

BOOL	__cdecl		Wce_KitlIO_Init(void);
void	__cdecl		Wce_KitlIO_Term(void);
int		__cdecl		Wce_KitlIO_Gets(char* str, int size);
int		__cdecl		Wce_KitlIO_Getchar(BOOL blocking);
int	__cdecl			Wce_KitlIO_Printf(const char * format, ...);

#ifdef __cplusplus
}
#endif

/*void	__cdecl		Wce_KitlIO_Puts(char* pInpBuffer);
void	__cdecl		Wce_KitlIO_Putc(VOID* pInpBuffer);*/

#define MAX_KITLBUF_SIZE		1024

#endif // WCE_KITLIO_H_INCLUDED_