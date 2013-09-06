/****************************************************************************
** Notice:		Copyright (c)2004 AVIT - All Rights Reserved
**
** File Name:	j_osd.h
**
** Revision:	1.0
** Date:		2004.9.10
**
** Description: Header file for AVIT data broadcast intergration OSD module.
**             
****************************************************************************/

#ifndef __J_OSD_H__
#define __J_OSD_H__

#if defined(__cplusplus)
extern "C" {
#endif
#include "j_gendef.h"	/* include general definations for AVIT */


/****************************************************************************
** Type definations
****************************************************************************/
typedef enum			/* defination of cursor type */
{
    J_OSD_CURSOR_HAND,	/* Hand cursor type */
    J_OSD_CURSOR_WAIT,	/* Wait cursor type */
    J_OSD_CURSOR_EDIT,	/* Edit cursor type */
    J_OSD_CURSOR_ARROW	/* Arrow cursor type */
} E_OSD_CursorType;

typedef struct			/* defination of rectangle structure */
{
	INT32 nX;
	INT32 nY;
	INT32 nWidth;
	INT32 nHeight;
} T_OSD_Rect;

typedef struct			/* defination of RGB structure */
{
    UINT8 cRed;			/* red */
    UINT8 cGreen;		/* green */
    UINT8 cBlue;		/* blue */
} T_OSD_ColorRGB;


/*****************************************************************************
** Function prototypes
*****************************************************************************/
INT32 J_OSD_Init(void);
INT32 J_OSD_SetResolution(UINT32 width, UINT32 heigth);
void  J_OSD_EnableDisplay(void);
void  J_OSD_DisableDisplay(void);
void  J_OSD_ClearScreen(void);
INT32 J_OSD_ClearRect(INT32 nX, INT32 nY, INT32 nWidth, INT32 nHeight);
INT32 J_OSD_SetTransparency(INT32 nLevel);
INT32 J_OSD_GetTransparency(void);
INT32 J_OSD_DrawPixel(INT32 nX, INT32 nY, T_OSD_ColorRGB *pColor);
INT32 J_OSD_FillRect(INT32 nX, INT32 nY, INT32 nWidth, INT32 nHeight, T_OSD_ColorRGB *pColor);
#if 0
INT32 J_OSD_BlockCopy(T_OSD_Rect * pDest, void * pMemory, int b_haveBk, void* bk_map,T_OSD_Rect* bk_rect);
INT32 J_OSD_BlockCopyEx(T_OSD_Rect *pSrc, T_OSD_Rect *pDest, void* pMemory, int b_haveBk, void* bk_map,T_OSD_Rect* bk_rect);
#endif
INT32 J_OSD_BlockRead(T_OSD_Rect *pRect, void* pMemory);
void  J_OSD_CursorShow(void);
void  J_OSD_CursorHide(void);
INT32 J_OSD_CursorSetType(E_OSD_CursorType eCursor);
INT32 J_OSD_CursorMove(INT32 nX, INT32 nY);

#if defined(__cplusplus)
}
#endif

#endif	/* __J_OSD_H__ */
