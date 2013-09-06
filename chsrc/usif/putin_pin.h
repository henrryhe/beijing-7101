#include "stddefs.h"
#include "stlite.h"
#ifndef DIALOG_H
#define DIALOG_H

#ifndef OSD_COLOR_TYPE_RGB16  
#define DIALOG_TEXT_COLOR        0xff000000
#define DIALOG_BUTTON_TEXT_COLOR  0xfff8f8f8

#define DIALOG_PIN_COLOR          0xfff8f8f8
#define DIALOG_PIN_BORDER_COLOR  0xff6080a0
#define DIALOG_PIN_TEXT_COLOR    0xfff08020
#else/*ARGB1555*/
#define DIALOG_TEXT_COLOR        0x8000
#define DIALOG_BUTTON_TEXT_COLOR  0xffff

#define DIALOG_PIN_COLOR          0xffff
#define DIALOG_PIN_BORDER_COLOR  0xB214
#define DIALOG_PIN_TEXT_COLOR    0xFA04
#endif

extern int  CH6_InputPinCode(boolean ReponseProKey);
extern int	CH6_PIN_PopupDialog (char *DialogTile ,int WaitSeconds,boolean ResponsePKEY);

/*extern boolean	CH6_PopupDialog(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *DialogContent,char Buttons,int WaitSeconds);*/

extern boolean	CH6_AutoPopupDialog(STGFX_Handle_t GHandle,STLAYER_ViewPortHandle_t VPHandle,char *DialogContent,char Buttons,int WaitSeconds);
extern U8* CH6_GetDialogMemory(int *MemmorySize);
extern void CH6_SetDialogMemory(U8 *MemmoryData);
extern void CH6_ClearDialog(void);
#ifdef STBID_FROM_E2PROM

extern int CH6_PIN_PopupDialog_stbid (char *DialogTile ,int WaitSeconds,boolean ResponsePKEY);
extern int CH6_InputPinCode_stbid(boolean ReponseProKey);
#endif

#endif


