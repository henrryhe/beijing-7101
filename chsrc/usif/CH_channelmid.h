/*
(c) Copyright changhong
  Name:CH_channelmid.h
  Description:Header file for CH_channelmid.c,
  Authors:yixiaoli
  Date          Remark
  2007-02-21    Created

  Modifiction:

Date:yxl 2007-07-04 by yixiaoli  
1.Add new function CH_DisplayIFrameFromMemory(),
主要用于从内存显示I帧

 */


extern BOOL CH6_AVControl(CH6_VideoControl_Type VideoControl,BOOL AudioControl,CH6_AVControl_Type ControlType);

extern void CH66_NewChannel(int VideoPid,int AudeoPid,int PCRPid,BOOL EnableVideo,BOOL EnableAudeo,
					STAUD_StreamContent_t AudType,BYTE VidType);

extern BOOL  CH6_SetVideoWindowSize(BOOL Small,unsigned short PosX,unsigned short PosY,
						 unsigned short Width,unsigned short Height );

extern void CH6_TerminateChannel(void);

extern BOOL CH_DisplayIFrame(char *PICFileName,U32 Loops,U32 Priority);

extern void CH_SetVideoAspectRatioConversion(STVID_DisplayAspectRatioConversion_t ARConversion);

extern BOOL CH_DisplayIFrameFromMemory(U8* pData,U32 Size,U32 Loops,U32 Priority);

