/*
  (C) Copyright changhong
  Name:CH_VideoOutput.h
  Description:header file for长虹7100 platform视频输出类型、定时模式等转换的应用控制和接口函数
				(与客户需求相关）
  Authors:Yixiaoli
  Date          Remark
  2007-05-14   Created (将原7710 相关部分移植过来)

  Modifiction : 

*/



#ifdef LEDANDBUTTON_CONTROL_MODE_BY_ATMEL

#define LED_H 0x2c
#define LED_h 0x2c
#define LED_E 0x15
#define LED_MLINE 0x7f
#define LED_0 0x84
#define LED_1 0xee
#define LED_2 0x45
#define LED_3 0x46
#define LED_4 0x2E
#define LED_5 0x16

#define LED_NOP 0xFF
#define LED_P 0x0d
#define LED_R 0x7d
#define LED_D 0x64
#define LED_C 0x95

#define LED_Y 0x26

#define LED_V 0xa4
#define LED_G 0x94
#define LED_A 0x0c
#define LED_I 0xbd

#define LED_B 0x34




#define FIRSTBIT  0x01
#define SECONDBIT 0x03
#define THIRDBIT  0x04
#define FOURTHBIT 0x02

#endif 

#ifdef LEDANDBUTTON_CONTROL_MODE_BY_PT6964

#define LED_H 0x76
#define LED_h 0x74
#define LED_E 0x79
#define LED_MLINE 0x40
#define LED_0 0x3f
#define LED_1 0x06
#define LED_2 0x5b
#define LED_3 0x4f
#define LED_4 0x66
#define LED_5 0x6d

#define LED_NOP 0x0
#define LED_P 0x73
#define LED_R 0x50
#define LED_D 0x5e
#define LED_C 0x39

#define LED_Y 0x6e
#if 1 
#define LED_V 0x3e
#define LED_G 0x3d
#define LED_A 0x77
#define LED_I 0x30
#endif

#define LED_B 0x7c

#define FIRSTBIT  0x01
#define SECONDBIT 0x02
#define THIRDBIT  0x03
#define FOURTHBIT 0x04

#endif 



extern BOOL CH_LEDDisVOutType(CH_VideoOutputType Type,U8 Mode );

extern BOOL CH_VOutTypeDisplayAndControl(CH_VideoOutputType*  pType);

extern BOOL CH_VOutTypeAndTimingMatchCheck(CH_VideoOutputType* pType,CH_VideoOutputMode* pTimingMode,BOOL IsFindBestMode);

extern void CH_VideoFormatChange(int ChangeKey);

extern BOOL CH_LEDDisPWStatus(U8 PWPos );

extern void CH_StartupVideoFormatSet(void);


/*End of file*/

