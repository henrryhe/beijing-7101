/*
  (C) Copyright changhong
  Name:CH_VideoOutputBase.h
  Description:header file for实现长虹7100 platform视频输出类型、定时模式等转换的底层控制和接口函数
				(与客户需求相关）
  Authors:Yixiaoli
  Date          Remark
  2007-05-14   Created 

  Modifiction : 

*/


extern BOOL CH_SetVideoOutputType(CH_VideoOutputType OutputType);

extern BOOL CH_SetVideoOut(CH_VideoOutputType Type,CH_VideoOutputMode Mode,
	CH_VideoOutputAspect AspectRatio,CH_VideoARConversion ARConversion); 




/*End of file*/

