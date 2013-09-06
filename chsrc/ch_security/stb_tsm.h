/*
STB2TSM.h
定义要求机顶盒模块实现的接口函数
*/

#ifndef SUMAVISION_STB_TO_SecTerminal_H
#define SUMAVISION_STB_TO_SecTerminal_H
#include "Appltype.h"

#ifndef BYTE 
#define BYTE unsigned char
#endif
#ifndef NULL
#define NULL 0
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif
#ifndef WORD //16bit
#define WORD unsigned short
#endif
#ifndef HANDLE
#define HANDLE  long*
#endif


//线程函数地址类型
typedef void (*pThreadFunc) (void *param);



#ifdef __cplusplus
extern "C" {
#endif

/*获取分配给安全模块的flash的大小和地址*/
int SumaSTBSecure_GetFlashAddr(int* plSize, int* plBaseAddr);

/*读flash*/
int SumaSTBSecure_ReadFlash(int lBeginAddr, int* plDataLen, BYTE* pData);
/*写flash*/
int SumaSTBSecure_WriteFlash(int lBeginAddr, int* plDataLen, const BYTE* pData);

/*获得机顶盒方分配给“终端安全模块”所使用的OTP的基地址和大小 */
int SumaSTBSecure_GetOTPAddr(int* plSize, int* plBaseAddr);

/*读取安全相关OTP数据到内存*/
int SumaSTBSecure_ReadOTP(int lBeginAddr, int* plDataLen, BYTE* pData);

/*获得机顶盒方分配给“终端安全模块”所使用的EEPROM(用来保存数据，非代码)的基地址和大小 */
int SumaSTBSecure_GetEepromAddr(int* plSize, int* plBaseAddr);
/*读EEPROM到内存*/
int SumaSTBSecure_ReadEeprom(int lBeginAddr, int* plDataLen, BYTE* pData);
/*写内存到EEPROM；*/
int SumaSTBSecure_WriteEeprom(int lBeginAddr, int* plDataLen, const BYTE* pData);


int SumaSTBSecure_SemaphoreInit( U32* pSemaphore ,U32 lInitCount);

/*信号量加信号。*/
int SumaSTBSecure_SemaphoreSignal( U32 lSemaphore);

/*等待信号量,等待成功之后,信号量为无信号。*/
int  SumaSTBSecure_SemaphoreTimedWait(U32 lSemaphore,U32 lWaitCount);

/*创建进程*/
int SumaSTBSecure_CreateThread (const char * szName, pThreadFunc pTaskFun, int priority, void * param, int StackSize, U32*plThreadID);
/*销毁进程*/
int  SumaSTBSecure_TerminateThread(U32 lThreadID);

/*===================== 获取机顶盒身份信息相关函数 ========================*/
/* 获取机顶盒ID */
int SumaSTBSecure_GetSTBID(char* szSTBID, int* piLen);
/* 获取机顶盒芯片ID */
int SumaSTBSecure_GetChipID(char* szChipID, int* piLen);
/* 获取机顶盒当前智能卡卡号 */
int SumaSTBSecure_GetCardID(char* szCardID, int* piLen);
/* 功能：	获取机顶盒的MAC地址 */
int SumaSTBSecure_GetMacAddr(char* szMacAddr, int* piLen);

/*======================== 获取随机数生成的输入因子 =====================*/

/*功能：获取机顶盒显示时间、机顶盒cpu计数、机顶盒任意一段内存的数据*/
boolean SumaSTBSecure_GetSeeds(BYTE * pbyTime,BYTE * pbyTickCount,BYTE * pbyBuf,int nBufLen);

/*
功能：获取用户私钥保护口令
输出：szUserPIN:	用户私钥保护口令，不超过 10 Bytes   
使用场景：

机顶盒进行初始化时，获取到前端下发证书，同时根据下发的命令，决定是否弹出此界面提示用户输入口令用于保护私钥

*/
int SumaSTBSecure_GetUserPIN(char* szUserPIN);



/*===================== 调试&提示信息相关函数 =======================*/
/*功能：打印调试信息。
要求：用于在移植时打印调试信息使用。移植完毕后，该函数直接返回不做任何处理。
参数：pszMsg:	调试信息内容。*/
void SumaSTBSecure_AddDebugMsg(const char * szMsg, ... );

/*=====================UKey使用的相关函数===============================*/

HANDLE SumaSecure_OpenUKey(void);
int SumaSecure_WriteDataToUKey(HANDLE hUkey,BYTE * pbyinData,WORD *pwLen);
int  SumaSecure_ReadDataFromUKey(HANDLE hUkey,BYTE * pbyoutData,WORD *pwLen);


#ifdef __cplusplus
}
#endif

/*机顶盒用到的FLASH地址*/
#define FLASH_SECURITY_ADDR   /*0xA11E0000 WZ 添加SKIN后的调整011211*/ 0xA1F80000
#define MAX_SECURITY_FLASH_BLOCK 1


#define FLASH_SECURITYDATA_LEN  (128*1024)

#define FLASH_SECURITY_ADDR_SPARE /*0xA1200000 WZ 添加SKIN后的调整011211*/ 0xA1FA0000


#define BIG_PINPIC_ADDR  /*0xA1220000 WZ 添加SKIN后的调整011211*/ 0XA1100000
#define BIG_PINPIC_LEN 0x761ff

#define SMALL_PINPIC_ADDR  (BIG_PINPIC_ADDR+BIG_PINPIC_LEN+1)
#define SMALL_PINPIC_LEN  0X37e17

#define BIG_PINSTAR_ADDR  (SMALL_PINPIC_ADDR+SMALL_PINPIC_LEN+1)
#define BIG_PINSTAR_LEN	0X1B7

#endif
