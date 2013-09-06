/****************************************************************************
** Notice:		Copyright (c)2004 AVIT - All Rights Reserved
**
** File Name:	j_gendef.h
**
** Revision:	1.0
** Date:		2004.9.10
**
** Description: Header file of general definations for AVIT.
**             
****************************************************************************/

#ifndef __J_GENDEF_H__
#define __J_GENDEF_H__
#include "stddefs.h"
/****************************************************************************
** System wide definations
****************************************************************************/
/*
#ifndef  _BOOL_t
#define _BOOL_t
typedef unsigned char    BOOL;
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef  BOOL
#define  BOOL unsigned char 	
#endif
*/
#ifndef  INT8
typedef signed char			INT8;
#endif

#ifndef  UINT8
typedef unsigned char		UINT8;
#endif

#ifndef  INT16
typedef short				INT16;
#endif

#ifndef  UINT16
typedef unsigned short		UINT16;
#endif

#ifndef  INT32
typedef signed long			INT32;
#endif

#ifndef	 UINT32
typedef unsigned long		UINT32;
#endif

#ifndef BYTE
typedef unsigned char		BYTE;
#endif

#ifndef WORD
typedef unsigned short		WORD;
#endif

#ifndef DWORD
typedef unsigned long		DWORD;
#endif

#ifndef T_AVIT_ReturnType
typedef INT32				T_AVIT_ReturnType;
#endif

#ifndef FALSE
#define FALSE				(0)
#endif

#ifndef TRUE
#define TRUE				(1)
#endif

#ifndef NULL
#define NULL				(0)
#endif

#ifndef J_SUCCESS
#define J_SUCCESS			(0)
#endif

#ifndef J_FAILURE
#define J_FAILURE			(-1)
#endif

#ifndef J_OS_ERROR
#define J_OS_ERROR			(-2)
#endif

#ifndef J_PARA_ERROR
#define J_PARA_ERROR		(-3)
#endif

#ifndef J_HARDWARE_ERROR
#define J_HARDWARE_ERROR	(-4)
#endif

#ifndef J_ENTER_EPG
#define J_ENTER_EPG	        (-5)
#endif

#endif	/* __J_GENDEF_H__ */

