#ifndef __TSPACKET_H__
#define __TSPACKET_H__
#include "appltype.h"

#define SUMA_STB_GETTS_OK			0   /*成功*/
#define SUMA_STB_GETTS_FAILED		1   /*失败*/
#define SUMA_STB_GETTS_LENERROR	2   /*长度错误*/
#define SUMA_STB_GETTS_GETTING     3   /*正在获取中*/


#define SECURITY_TSPACKET_LENGTH          188
#define CH_SECURITY_MAX_TSCAPTURE_NUM 10


typedef enum
{
SUMA_STB_GETTS_STOP = 0,         /*停止获取TS包*/
SUMA_STB_GETTS_START            /*开始获取TS包*/
}tsm_porting_tsnotify_s;



typedef struct  TsCapturePara
{
  int i_frequency;      /*频率*/
  int i_symbol_rate;    /*符号率*/
  int i_modulation;     /*QAM*/
  int i_audiopid;        /*音频pid*/
} CH_Security_TsCapturePara_t; 


typedef struct
{
	int  i_xptrnumber;
	CH_Security_TsCapturePara_t stru_CaptureParalist[CH_SECURITY_MAX_TSCAPTURE_NUM];
}CH_Security_tspacket_t;


#endif
