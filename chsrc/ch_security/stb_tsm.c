
#include "stdarg.h"  
#include "stddefs.h"
#include "string.h"
#include "..\dbase\vdbase.h"
#include "..\util\ch_time.h"
#include "stb_tsm.h"
#include "..\key\RC6_keymap.h"
#include "..\main\initterm.h"
#include "..\eis\eis_api_osd.h"

#if 0
#define _SUMA_DEBUG_
#endif
#ifdef  _SUMA_DEBUG_
static char suma_debug_buff[1024*4];
#endif
#if 0
#define stb_scms_debug
#endif

/*最大任务数，目前只用到了一个*/
#define MAX_SUMA_TASK_NUM			5
static task_t *gs_SumaTaskIDNeedDelete[MAX_SUMA_TASK_NUM] = {NULL, NULL};
/*信号互斥*/
static semaphore_t *gs_SumaTaskLock = NULL;

extern   ST_Partition_t* CHSysPartition;  /*系统内存区*/

static U8 *SumaSecure_memory =NULL;
static U8 SumaChipID[20] = {0};/*暂存chip id sqzow20100707*/
semaphore_t *p_SumaMemorySema = NULL; /*对全局变量SumaSecure_memory访问的信号量*/

#if 1
static boolean gb_SetUserStatus = false;

boolean CH_GetUserIDStatus(void)
{
	return gb_SetUserStatus;
}

void  CH_SetUserIDStatus(boolean val)
{
	gb_SetUserStatus = val;
}

#endif
/*===================== 机顶盒存储相关函数 ========================*/


/*
1.	接口定义
	int  SumaSTBSecure_GetFlashAddr(long* plSize, long* plBaseAddr);
2.	功能说明
	获得机顶盒方分配给"终端安全模块"所使用的Flash(用来保存数据，非代码)的基地址和大小。
3.	参数说明
	输入参数：无
	输出参数：
        plSize：Flash空间大小
        plBaseAddr|：Flash空间的首地址
	返回值：
       		 成功：0；不成功：非零值，错误号；
4.	应用场景
	终端安全模块需要写数据到Flash中时，需要获得机顶盒分配给终端安全模块的Flash空间的首地址和空间大小。
*/
int SumaSTBSecure_GetFlashAddr(int* plSize, int* plBaseAddr)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_GetFlashAddr\n");
#endif
	p_SumaMemorySema=semaphore_create_fifo(1);
	SumaSecure_memory = memory_allocate(CHSysPartition,FLASH_SECURITYDATA_LEN);
	if(SumaSecure_memory == NULL)
	{
		do_report(0,"SumaSTBSecure_GetFlashAddr err!!\n");
		*plBaseAddr = 0;
		*plSize = 0;
		return -1;
	}
	else
	{
		memset(SumaSecure_memory,0,FLASH_SECURITYDATA_LEN);
		
		if(CH_GetSecurityFlashStatus() != 0X8A)
		{
			do_report(0,"SumaSTBSecure_ReadFlash数据无效需要恢复\n");
			CH_CheckSecurityDataStatus();
		}

		if(CH_GetSecurityFlashStatus() == 0X8A)
		{
			
			ipanel_porting_nvram_read(FLASH_SECURITY_ADDR,SumaSecure_memory,FLASH_SECURITYDATA_LEN);
		
		}

			*plBaseAddr = (int)SumaSecure_memory;
			*plSize = FLASH_SECURITYDATA_LEN;

			return 0;

	}
}

/*
1.	接口定义
	int  SumaSTBSecure_ReadFlash(long lBeginAddr, long* plDataLen, BYTE* pData);
2.	功能说明
	读取Flash中保存的数据到内存。
3.	参数说明
	输入参数：
	lBeginAddr：保存数据的Flash的地址（绝对地址）
		         plDataLen：	所要读取的数据的长度
	        输出参数： 
	pData：		读取的数据
		         plDataLen：	实际读取的数据的长度
        返回值：
                成功：0；不成功：非零值，错误号；
4.	应用场景
	终端安全模块读取保存在Flash指定位置指定大小的数据到程序内存，例如终端安全模块读取保存在Flash中的终端证书等数据；

*/
int SumaSTBSecure_ReadFlash(int lBeginAddr, int* plDataLen, BYTE* pData)
{
	int len = *plDataLen;

#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_ReadFlash 0x%x len=0x%x\n",lBeginAddr,len);
#endif
  	semaphore_wait(p_SumaMemorySema);
	memcpy((BYTE *)pData,(BYTE *)lBeginAddr,len);	
	semaphore_signal(p_SumaMemorySema);
	return 0;
}

/*
1.	接口定义
	int  SumaSTBSecure_WriteFlash(long lBeginAddr, long* plDataLen, const BYTE* pData);
2.	功能说明
	保存内存中数据到Flash中。
3.	参数说明
	输入参数：
 	lBeginAddr：保存数据的Flash的地址（绝对地址）
	 plDataLen： 所要保存的数据的长度
	  pData：	  保存的数据
返回值：
     	成功：0；不成功：非零值，错误号，
4.	应用场景
	终端安全模块保存指定大小的数据到Flash指定位置。例如终端安全模块保存终端证书等数据到Flash中
*/
int SumaSTBSecure_WriteFlash(int lBeginAddr, int* plDataLen, const BYTE* pData)
{
	int i;
	int writelen = *plDataLen;
	
	int i_offset = 0;
	
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_WriteFlash 0x%x len 0x%x\n",lBeginAddr,writelen);
#endif	
	
	/* 拷贝数据到内存区*/
	i_offset = lBeginAddr- (int)SumaSecure_memory;
	memcpy((BYTE *)((int)SumaSecure_memory+i_offset),pData,writelen);


	

	CH_SetSecurityFlashStatus(0x8B);

	CH_Security_Flash_burn(FLASH_SECURITY_ADDR,SumaSecure_memory,FLASH_SECURITYDATA_LEN);
		

	/*FLASH备份*/
	CH_SetSpareSecurityFlashStatus(0x8B);

	CH_Security_Flash_burn(FLASH_SECURITY_ADDR_SPARE,SumaSecure_memory,FLASH_SECURITYDATA_LEN);
		
	
	return 0;


}

/*
1.	接口定义
	int   SumaSTBSecure_GetOTPAddr(long* plSize, long* plBaseAddr);
2.	功能说明
	获得机顶盒方分配给"终端安全模块"所使用的OTP的基地址和大小。
3.	参数说明
        输入参数：无
	输出参数：
   		 plSize：OTP空间大小
   	 	plBaseAddr：OTP空间的首地址
	返回值：
    		成功：0；不成功：非零值，错误号；
4.	应用场景
	终端安全模块需要读取保存在OTP中的根公钥信息时，需要首先获得机顶盒分配给终端安全模块的OTP空间的首地址和大小；

*/
int SumaSTBSecure_GetOTPAddr(int* plSize, int* plBaseAddr)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_GetOTPAddr \n");
#endif
	if(plSize ==  NULL || plBaseAddr==NULL)
		return -1;
	* plSize = 128*1024;
	* plBaseAddr = 0xA1FC0000;
	return 0;

}

/*
1.	接口定义
	int   SumaSTBSecure_ReadOTP(long lBeginAddr, long* plDataLen, BYTE* pData);
2.	功能说明
	读取OTP数据到内存。
3.	参数说明
     	输入参数：
		 lBeginAddr：保存数据的OTP地址（绝对地址）
		  plDataLen：	所要读取的数据的长度
	输出参数： 
		pData：		读取的数据
		plDataLen：	实际读取的数据的长度
      返回值：
         成功：0；不成功：非零值，错误号；
4.	应用场景
	终端安全模块需要读取保存在OTP中的根公钥

*/
int SumaSTBSecure_ReadOTP(int lBeginAddr, int* plDataLen, BYTE* pData)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_ReadOTP \n");
#endif

	if(plDataLen ==  NULL || pData==NULL)
		return -1;
	
	ipanel_porting_nvram_read(lBeginAddr,pData,*plDataLen);
	return 0;
}

/*
1.	接口定义
	int   SumaSTBSecure_GetEepromAddr(long* plSize, long* plBaseAddr);
2.	功能说明
	获得机顶盒方分配给"终端安全模块"所使用的EEPROM(用来保存数据，非代码)的基地址和大小。
3.	参数说明
	输入参数：无
	输出参数：
        	plSize：EEPROM空间大小
        	plBaseAddr：EEPROM空间的首地址
	返回值：
        	成功：0；不成功：非零值，错误号；
4.	应用场景
	终端安全模块保存终端加密私钥等数据到EEPROM时，需要获取EEPROM的首地址和大小。
*/
int SumaSTBSecure_GetEepromAddr(int* plSize, int* plBaseAddr)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_GetEepromAddr \n");
#endif

	if(plSize ==  NULL || plBaseAddr==NULL)
		return -1;
	
	*plBaseAddr = START_E2PROM_SECURITYDATA;
	*plSize = E2PROM_SECURITYDATA_LEN;

	return 0;

}
/*
	1.	接口定义
	int   SumaSTBSecure_ReadEeprom(long lBeginAddr, long* plDataLen, BYTE* pData);
	2.	功能说明
	读EEPROM数据到内存。
	3.	参数说明
	输入参数：
 		 lBeginAddr：保存数据的EEPROM的地址（绝对地址）
		 plDataLen：	所要读取的数据的长度
	输出参数： 
		pData：		读取的数据
		plDataLen：	实际读取的数据的长度
   	返回值：
             成功：0；不成功：非零值，错误号；
	4.	应用场景
		终端安全模块从EEPROM读取加密私钥到内存。

*/
int SumaSTBSecure_ReadEeprom(int lBeginAddr, int* plDataLen, BYTE* pData)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_ReadEeprom adr %d len %d pdata 0x%x\n",lBeginAddr,*plDataLen,pData);
#endif
	if(plDataLen ==  NULL || pData==NULL)
		return -1;
	if((int)(lBeginAddr + (int)*plDataLen) >= START_E2PROM_SECURITYDATA_SPARE )/*写入越界*/
	{
		*plDataLen = 0;
		do_report(0,"ERR!!!SumaSTBSecure_ReadEeprom err!! %d len %d\n",lBeginAddr,*plDataLen);
		return -1;
	}
	if(CH_ReadSecurityData(lBeginAddr, (int)*plDataLen, (void *) pData) == TRUE)/*读数据发生错误*/
	{
		*plDataLen = 0;
		do_report(0,"ERR!!!SumaSTBSecure_ReadEeprom->CH_ReadSecurityData err!! \n");
		return -1;	
	}
	else
	{
		return 0;
	}

}
/*	
	1.	接口定义
		int SumaSTBSecure_WriteEeprom(long lBeginAddr, long* plDataLen, const BYTE* pData);
	2.	功能说明
		把指定长度的内存数据记录到EEPROM的指定位置。
	3.	参数说明
		输入参数：
			lBeginAddr： EEPROM的地址。
			plDataLen：保存数据长度的指针
			pData：保存数据的指针
		输出参数：无
		返回：
		成功：0；不成功：非零值，错误号；
		4.	应用场景
		在终端证书申请成功后，需要把证书对应的私钥加密存储到EEPROM中。
*/
int SumaSTBSecure_WriteEeprom(int lBeginAddr, int* plDataLen, const BYTE* pData)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_WriteEeprom adr %d len %d pdata 0x%x \n",lBeginAddr,*plDataLen,pData);
#endif
	if(plDataLen ==  NULL || pData==NULL)
		return -1;
	if((int)(lBeginAddr + (int)*plDataLen) >= START_E2PROM_SECURITYDATA_SPARE )/*写入越界*/
	{
		*plDataLen = 0;
		do_report(0,"ERR!!!SumaSTBSecure_WriteEeprom err!! %d len %d\n",lBeginAddr,*plDataLen);
		return -1;
	}

	if(CH_WriteSecurityData(lBeginAddr, (int)*plDataLen, (void *) pData) == TRUE)
	{
		*plDataLen = 0;
		do_report(0,"ERR!!!SumaSTBSecure_WriteEeprom->CH_WriteSecurityData err!!\n ");
		return -1;	
	}
	else
	{
		return 0;
	}

}
/****************************************************/
/*
1.	接口定义
	Int   SumaSTBSecure_SemaphoreInit( long * pSemaphore long lInitCount);
2.	功能说明：
	初始化信号量。
3.	参数说明
	输入参数：
	pSemaphore指向信号量的指针；
	lInitCount：初始化信号量值?
4.	应用场景
*/
int SumaSTBSecure_SemaphoreInit( U32* pSemaphore ,U32 lInitCount)
{
	semaphore_t* TempSem  = 0;
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_SemaphoreInit %d \n",lInitCount);
#endif
	if(pSemaphore ==  NULL )
		return -1;
	TempSem =semaphore_create_fifo_timeout(lInitCount);

	*pSemaphore = (U32)TempSem;

	return 0;

}
/*
	1.	接口定义
		Int   SumaSTBSecure_Semaphore Release ( long lSemaphore );
	2.	功能说明：
		释放信号量。
	3.	参数说明
		输入参数：
		pSemaphore指向信号量的指针；
	4.	应用场景

*/
int   SumaSTBSecure_SemaphoreRelease ( U32 pSemaphore )
{

#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_SemaphoreRelease 0x%x \n",pSemaphore);
#endif
	    if(pSemaphore ==0)
	    {
		 return -1;
	    }
	
	semaphore_delete((semaphore_t*)pSemaphore);
	return 0;

}

/*
	1.	接口定义
		int  SumaSTBSecure_SemaphoreSignal( long lSemaphore);
	2.	功能说明
		为信号量加信号
	3.	参数说明
		输出参数：pSemaphore指向信号量的指针；
	4.	应用场景

*/

int SumaSTBSecure_SemaphoreSignal( U32 lSemaphore)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_SemaphoreSignal  0x%x \n",lSemaphore);
#endif
	if(lSemaphore ==0)
	{
	 return -1;
	}

	semaphore_signal((semaphore_t*)lSemaphore);
	return 0;
}

/*
	1.	接口定义
		int  SumaSTBSecure_SemaphoreTimedWait(long lSemaphore ，long lWaitCount);
	2.	功能说明
		等待信号量,等待成功之后,信号量为无信号
	3.	参数说明
		输入参数：
			lSemaphore指向信号量的指针；
			lWaitCount：等待的时间量, 单位是ms，-1表示永久等待。
4.	应用场景

*/
int  SumaSTBSecure_SemaphoreTimedWait(U32 lSemaphore ,U32 lWaitCount)
{
	int ret;
	clock_t  time;
	int val;
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_SemaphoreTimedWait  0x%x  %d \n",lSemaphore,lWaitCount);
#endif
   	 if(lSemaphore ==0)
	    {
		 return -1;
	    }
	

	if(lWaitCount == 0)
	{
    		val = semaphore_wait_timeout((semaphore_t*)lSemaphore,TIMEOUT_IMMEDIATE);
		
	}
	else if(lWaitCount == -1)
	{
		
		val =  semaphore_wait_timeout((semaphore_t*)lSemaphore,TIMEOUT_INFINITY);	

	}
	else if(lWaitCount >0)
	{
		
		time = time_plus(time_now(), ST_GetClocksPerSecond()*lWaitCount/1000);
		val =  semaphore_wait_timeout((semaphore_t*)lSemaphore,&time);	
	}
	else
	{

		return -1;

	}
	
	if(val == 0)
	{

		return 0;
	}
	else
	{

		return -1;
	}
	
 
}

/*
	1.	接口定义
		Int SumaSTBSecure_ CreateThread (const char * szName, pThreadFunc pTaskFun, Int priority, void * param, int StackSize，unsigened long *plThreadID);
	2.	功能说明
		创建进程。
	3.	参数说明
		输入参数：
			szName：创建的进程名称
         		pTaskFun：创建的进程对应的函数
			priority：优先级（0-15）normal 为7 8，高优先级为12
			param：进程函数参数
			StackSize：进程栈大小，参数可选。
         	输出参数：
              		PlThreadID 创建的进程ID
	返回值：
       		成功：0；不成功：错误号
	4.	应用场景

*/
int SumaSTBSecure_CreateThread (const char * szName, pThreadFunc pTaskFun, int priority, void * param, int StackSize, U32*plThreadID)
{
       task_t* SecurityTask;

#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_CreateThread %s priority %d  stacksize %d ",szName,priority,StackSize);
#endif
	StackSize *=2;/*堆栈开大一点，解决证书更新时的死机问题 sqzow20100706*/
   	 if( pTaskFun == NULL)
	    {
		 return -1;
	    }

	   SecurityTask = Task_Create (pTaskFun,param,StackSize,priority,szName,0);

	   *plThreadID = (U32)SecurityTask;
	   
	   return 0;

}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:SumaSTB_DeleteThread
    Purpose: 异步删除获取证书添加的任务
    Reference: 
    in:
        void >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/07/06 19:31:00 
---------------------------------函数体定义----------------------------------------*/
void SumaSTB_DeleteThread(void)
{
	int i;

	if(gs_SumaTaskLock == NULL)
	{
		return;
	}
	semaphore_wait(gs_SumaTaskLock);
	for(i = 0; i < MAX_SUMA_TASK_NUM; i++)
	{
		if(gs_SumaTaskIDNeedDelete[i])
		{
			task_wait(&gs_SumaTaskIDNeedDelete[i], 1, TIMEOUT_INFINITY);
			task_delete(gs_SumaTaskIDNeedDelete[i]);
			gs_SumaTaskIDNeedDelete[i] = NULL;
		}
	}
	semaphore_signal(gs_SumaTaskLock);
}

/*
	1.	接口定义
		int  SumaSTBSecure_TerminateThread(unsigened long lThreadID);
	2.	功能说明
		强制退出线程。
	3.	参数说明
		输入参数：
		hThread：结束进程的句柄 
	4.	应用场景

*/
int  SumaSTBSecure_TerminateThread(U32 lThreadID)
{

#if 1 	/*sqzow20100706*/
	int i;

	if(gs_SumaTaskLock == NULL)
	{
		gs_SumaTaskLock = semaphore_create_fifo(0);
		if(gs_SumaTaskLock == NULL)
		{
			do_report(0,"error !!SumaSTBSecure_TerminateThread create sem fail! ");
		}
	}
	else
	{
		semaphore_wait(gs_SumaTaskLock);
	}
	for(i = 0; i < MAX_SUMA_TASK_NUM; i++)
	{
		if(gs_SumaTaskIDNeedDelete[i] == NULL)
		{
			gs_SumaTaskIDNeedDelete[i] = (task_t*)lThreadID;
			break;
		}
	}
	if(i == MAX_SUMA_TASK_NUM)
	{
		do_report(0,"SumaSTBSecure_TerminateThread, task num too large! ");
	}
	if(gs_SumaTaskLock)
	{
		semaphore_signal(gs_SumaTaskLock);
	}
	MakeVirtualkeypress(VIRKEY_CLOSESUMA_KEY);
#else
	task_delete((task_t*)lThreadID);
#endif

#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_TerminateThread ");
#endif
	return 0;
}

/*===================== 获取机顶盒身份信息相关函数 ========================*/
/*
1.	接口定义
	Int   SumaSTBSecure_GetSTBID(char* szSTBID, int * piLen);
2.	功能说明
	获取机顶盒ID。
3.	参数说明
	输出参数：szSTBID：机顶盒编号
	piLen：编号长度
	返回值：
		成功：0；不成功：非零值，错误号；
4.	应用场景
		在终端证书申请或更新，或者构造终端身份的签名身份凭证时，需要机顶盒提供接口获取当前机顶盒的身份信息，包括STBID、ChipID、CardID、MacAddr	信息。

*/
int SumaSTBSecure_GetSTBID(char* szSTBID, int* piLen)
{
				
	U8 STB_SN[8];
	boolean GetSTB=true;
	U8 ManuID;
	U8 ProductModel;
	int ProYear;
	U8 ProMonth;
	U8 ProDay;
	long int STB_SNnum;
	 char strSTBInfo[20];
	 int len;
#ifdef stb_scms_debug
	 do_report(0,"SumaSTBSecure_GetSTBID buf=%x len=%d\n",szSTBID,* piLen);
#endif
	do_report(0,"SumaSTBSecure_GetSTBID buf=%x len=%d\n",szSTBID,*piLen);

	if(szSTBID ==  NULL || piLen==NULL)
		return -3;
	
	memset(strSTBInfo,0,20);
	
	GetSTB=CHCA_GetStbID(STB_SN);                /*Get STB serial number*/

	STB_SNnum=STB_SN[5]|(STB_SN[4]<<8)|(STB_SN[3]&0xF)<<16;
	ManuID=STB_SN[7];
	ProductModel=STB_SN[6];
	ProYear=((STB_SN[1]&0xf)<<3)|((STB_SN[2]&0xe0)>>5);
	ProMonth=((STB_SN[2]&0x1e)>>1)&0xF;
	ProDay=((STB_SN[2]&0x1)<<4)|((STB_SN[3]>>4)&0xf);

	if(STB_SNnum==0xFFFFF||GetSTB==true)
	{	
		*piLen = 0;
		do_report(0,"SumaSTBSecure_GetSTBID  Failed,num=0x%x, getstb=%d \n",
			STB_SNnum,
			GetSTB);

		return -3;
	}
	else
	{
	       sprintf(strSTBInfo,"%02d%02d%02d%02d%02d%07d",ManuID,ProductModel,ProYear,ProMonth,ProDay,STB_SNnum);
		len = strlen(strSTBInfo);

		{
			*piLen = len;
			memcpy(szSTBID,strSTBInfo,len);
			do_report(0,"SumaSTBSecure_GetSTBID %s \n",szSTBID);

			return 0;
		}	
	}


}
/*
	1.	接口定义
		Int   SumaSTBSecure_ GetChipID (char* szChipID, int * piLen);
	2.	功能说明
		获取机顶盒芯片ID。
	3.	参数说明
		输出参数：szChipID：机顶盒芯片编号
		piLen：编号长度
		返回值：
		成功：0；不成功：非零值，错误号
	4.	应用场景
		在终端证书申请或更新，或者构造终端身份的签名身份凭证时，需要机顶盒提供接口获取当前机顶盒的身份信息，包括STBID、ChipID、CardID、MacAddr	信息。

*/
int SumaSTBSecure_GetChipID(char* szChipID, int* piLen)
{
	if(szChipID && piLen)
	{
		strcpy(szChipID, SumaChipID);
		*piLen = strlen(SumaChipID);
		return 0;
	}
	return -1;
}

/*-------------------------------函数定义注释头--------------------------------------
    FuncName:SumaSTBSecure_GetChipIDPrevate
    Purpose: 内部预读chip id，避免在其它进程获取后无法解扰
    Reference: 
    in:
        piLen >> 
        szChipID >> 
    out:
        none
    retval:
        success >> 
        fail >> 
   
notice : 
    sqZow 2010/07/07 22:23:57 
---------------------------------函数体定义----------------------------------------*/
int SumaSTBSecure_GetChipIDPrevate(void)
{
	U8 NuIDKey[16];
	U32 NuICRC;
	U32 DevPubID;
	char NuIDInfo[10];
	int len;
         ST_ErrorCode_t Error;
#ifdef stb_scms_debug
	// do_report(0,"SumaSTBSecure_GetChipID buf=%x len=%d\n",szChipID,* piLen);
#endif

	
	memset(NuIDKey,0,16);
	memset(NuIDInfo,0,10);
	
#if 0 	/*<!-- ZQ 2009/9/6 16:14:18 --!>*/
	CH_GetNUID((U8 *)NuIDKey,(U32 *)&NuICRC); 
#endif 	/*<!-- ZQ 2009/9/6 16:14:18 --!>*/

        Error = CH_GetNUIDFormTKDMA(NuIDKey);
        if(Error != ST_NO_ERROR)
        {
#ifdef stb_scms_debug        
             //do_report(0,"Error in Get ChipID.\n");
#endif
            return -1;
        }
        
	DevPubID=(U32)(NuIDKey[3] << 24) | \
				(NuIDKey[2] << 16) | \
				(NuIDKey[1] << 8)  |  \
				(NuIDKey[0]);

	sprintf(NuIDInfo,"%x",DevPubID);
	len = strlen(NuIDInfo);
	/*	if( len > *piLen)
	{
		do_report(0,"SumaSTBSecure_GetChipID len %d 小于芯片ID长度%d",*piLen,len);
	
		memcpy(szChipID,NuIDInfo,*piLen);
		return -1;
	}
	else*/
	{
		memcpy(SumaChipID,NuIDInfo,len);
		return 0;
	}

}
/*
	1.	接口定义
		Int   INT   SumaSTBSecure_GetCardID(char* szCardID, int * piLen);
	2.	功能说明
		获取当前智能卡卡号。
	3.	参数说明
		输出参数：szCardID：当前智能卡编号
		piLen：编号长度
		返回值：
		成功：0；不成功：非零值，错误号
	4.	应用场景
		在终端证书申请或更新，或者构造终端身份的签名身份凭证时，需要机顶盒提供接口获取当前机顶盒的身份信息，包括STBID、ChipID、CardID、MacAddr	信息。

*/
int SumaSTBSecure_GetCardID(char* szCardID, int* piLen)
{
    	U8 Card_SN[6];
	long int Tmepcardnum1;
	long int Tmepcardnum2;
	char strCardInfo[20];
	int len;

#ifdef stb_scms_debug
	do_report(0,"\nSumaSTBSecure_GetCardID  buf %x len %d \n",szCardID,*piLen);
#endif
	if(szCardID ==  NULL || piLen==NULL)
		return -3;
	
	memset(strCardInfo,0,20);
		
	if(CHCA_GetCardContent(Card_SN) == TRUE)
	{
		*piLen = 0;
		do_report(0,"SumaSTBSecure_GetCardID ERR\n");
		return -3;
	}
	else
	{

		Tmepcardnum1=Card_SN[0]*256+Card_SN[1];
		Tmepcardnum2=Card_SN[2]*16777216+Card_SN[3]*65536+Card_SN[4]*256+Card_SN[5];
		if(Tmepcardnum1==0)
		{
			sprintf(strCardInfo,"%d",Tmepcardnum2);
		}
		else
		{
			sprintf(strCardInfo,"%d%d",Tmepcardnum1,Tmepcardnum2);
		}

		len = strlen(strCardInfo);
		if( len > *piLen)
		{
			do_report(0,"SumaSTBSecure_GetCardID len %d 小于卡号长度%d",*piLen,len);
	
			memcpy(szCardID,strCardInfo,*piLen);
			return -1;
		}
		else
		{
			*piLen = len;
			memcpy(szCardID,strCardInfo,len);
			return 0;
		}
			
	

	}

	}
/*
	1.	接口定义
		int  SumaSTBSecure_GetMacAddr(char* szMacAddr, int * piLen);
	2.	功能说明
		获取当前机顶盒MAC地址。
	3.	参数说明
		输出参数：szMacAddr:机顶盒MAC地址
						piLen:MAC	地址长度
		返回值：
			成功：0；不成功：非零值，错误号
	4.	应用场景
   	 在终端证书申请或更新，或者构造终端身份的签名身份凭证时，需要机顶盒提供接口获取当前机顶盒的身份信息，包括STBID、ChipID、CardID、MacAddr	信息。

*/
int  SumaSTBSecure_GetMacAddr(char* szMacAddr, int* piLen)
{
#define MAC_ADDRESS_LENGTH 6
	U8  mac[MAC_ADDRESS_LENGTH];
	char s[20];
	int len;
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_GetMacAddr  buf %x len %d \n",szMacAddr,*piLen);
#endif
	if(szMacAddr ==  NULL || piLen==NULL)
		return -3;
	
	memset(s,0,20);
	
	CH_ReadSTBMac(mac);
	
	sprintf(s,"%02x%02x%02x%02x%02x%02x\0",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	len = strlen(s);
/*	if( len > *piLen)
	{
		do_report(0,"SumaSTBSecure_GetMacAddr len %d 小于MAC长度%d",*piLen,len);
		memcpy(szMacAddr,s,*piLen);
		return -1;
	}
	else*/
	{
		*piLen = len;
		memcpy(szMacAddr,s,len);
		return 0;
	}
	
	
}

/*====================内存的相关接口===================================*/
/*
1.	接口定义
void*  SumaSTBSecure_malloc(int size);
2.	功能说明
请求分配指定大小的内存块。内存分配函数，要求每次调用该函数，统分配的内存空间必须是连续的整块内存。
3.	参数说明：
输入参数：
size:请求的内存大小，以字节为单位。大于0有效，小于等于0无效。
输出参数：无
返 回：
成功：返回有效内存块的起始地址；失败：返回NULL（空指针）。
4.	应用场景
*/

void*  SumaSTBSecure_malloc(int size)
{
	U8 * buf = NULL;

	if(size > 0)
	{
		buf = memory_allocate(CHSysPartition,size);
#ifdef stb_scms_debug
	do_report(0,"<<<<SumaSTBSecure_malloc 0x%x size %d \n",buf,size);
#endif
		return buf;
	}
	else
	{
		return NULL;
	}

}

/*
	1.	接口定义
	void  SumaSTBSecure_free(void *ptr);
	2.	功能说明
	释放指针ptr 指定的内存空间，并且该内存空间必须是由SumaSTBSecure_malloc 分配的。
	3.	参数说明：
	输入参数：
	ptr ：指向要释放的内存的指针。
	输出参数：无
	返    回：无
	4.	应用场景
	
*/

void  SumaSTBSecure_free(void *ptr)
{
#ifdef stb_scms_debug
	do_report(0,"<<<<<SumaSTBSecure_free 0x%x\n",ptr);
#endif

	if(ptr != NULL)
	{
		memory_deallocate(CHSysPartition,ptr);
		ptr = NULL;
	}
}

/*

*/
boolean SumaSTBSecure_GetSeeds(BYTE * pbyTime,BYTE * pbyTickCount,BYTE * pbyBuf,int nBufLen)
{
	TDTTIME   curtime;
	clock_t   timenow;

	if(pbyTime == NULL || pbyTickCount == NULL || pbyBuf == NULL || nBufLen <= 0)
		return false;

	GetCurrentTime(&curtime);
  
	pbyTime[0] = curtime.ucHours;
	pbyTime[1] = curtime.ucMinutes;
	pbyTime[2] = curtime.ucSeconds;
	
	timenow = time_now();
 
	memcpy(pbyBuf,(char *)&curtime,nBufLen);

	*pbyTickCount = time_now();


	return true;
    
}

/*======================== 需要机顶盒界面支持的相关函数================================*/


/*
	1.	接口定义
	int  SumaSTBSecure_GetUserPIN(char* szUserPIN);
	2.	功能描述
	获取私钥保护口令。
	3.	参数说明：
	输入参数：无
	输出参数：
		szUserPIN：私钥保护口令串
	返 回：
	0表示成功，其它表示失败。
	4.	应用场景
	当需要系统业务流程需要用户输入私钥保护口令时，调用此接口。


*/
int SumaSTBSecure_GetUserPIN(char* szUserPIN)
{	
	
#if 1
#define MAX_PIN_LEN 16
	boolean                    focus = true;

	int 		  iKeyScanCode;
	boolean      NOEXIT_TAG=true;

	char           tempchar;
	char           retbuf[MAX_PIN_LEN+1];
	int              curinputpos =0;

	int              x,y,w,h,text_x,text_y,char_width;
	int              max_string_len = 0;

	U8            *tempbuf = NULL;

	CH_RESOLUTION_MODE_e enum_Mode;

	CH_SetUserIDStatus(TRUE);

	 enum_Mode = CH_GetReSolutionRate() ;
	 

	 if(enum_Mode == CH_1280X720_MODE)
    	{
  			x = 400 ;
			y = 200;
			w= 420;
			h = 288;
			text_x  = x + 105+10;
			text_y = y+ 144+10;
        }
	else
	{
		x = 246;
		y = 170;
		w= 289;
		h = 198;
		text_x  = x + 46;
		text_y = y+ 108+5;
	 }


	
	max_string_len = MAX_PIN_LEN+1;
	memset(retbuf,0,MAX_PIN_LEN+1);


	
	tempbuf = CH_Eis_GetRectangle(x, y,w, h);
	if(tempbuf == NULL)
	{
		CH_SetUserIDStatus(FALSE);
		return -1;
	}
	 if(enum_Mode == CH_1280X720_MODE)
 	{
		CH_Eis_DisplayPicdata(x, y,w,h ,(U8*)BIG_PINPIC_ADDR);
 	}
	 else
 	{
		CH_Eis_DisplayPicdata(x, y,w,h,(U8*)SMALL_PINPIC_ADDR);
 	}


	while(NOEXIT_TAG)
	{
		   
		iKeyScanCode=GetMilliKeyFromKeyQueue (500 );

               switch(iKeyScanCode)
		{
			case OK_KEY:

				memcpy(szUserPIN,retbuf,MAX_PIN_LEN+1);
		 		NOEXIT_TAG=false;
		   		do_report(0,"<<<<SumaSTBSecure_GetUserPIN %s \n",szUserPIN);
		   		break;
			case EXIT_KEY:

				szUserPIN = "\0";

		  	 	NOEXIT_TAG=false;
		 
		   		break;
			case LEFT_ARROW_KEY:
				if(curinputpos == 0)
				{
					break;
				}
				curinputpos --;
				retbuf[curinputpos] = '\0';

				SumaSecure_SKB_fillBox(text_x+ 11*curinputpos, text_y,11,12, 0x80E6EBF1);
				ipanel_porting_graphics_draw_image( text_x+ 11*curinputpos, text_y,11, 12, NULL, EIS_REGION_WIDTH);
				/* 擦除一个*号*/
				break;
			default:  
				if((iKeyScanCode>=KEY_DIGIT0)&&(iKeyScanCode<=KEY_DIGIT9))
				{
					if(curinputpos <max_string_len)
					{
						tempchar = Convert2Char(iKeyScanCode);
						retbuf[curinputpos] = tempchar;
						
						CH_Eis_DisplayPicdata(text_x+ 11*curinputpos, text_y, 10,11,BIG_PINSTAR_ADDR);
						curinputpos++;
					}

				}
				
		   		break;
		}
		
	   }

		CH_Eis_PutRectangle(x, y, w, h, tempbuf);

	ipanel_porting_graphics_draw_image( x, y, w, h, NULL, EIS_REGION_WIDTH);

		CH_SetUserIDStatus(FALSE);
	   return  0;


EXIT_ERR:
		CH_SetUserIDStatus(FALSE);
		return -1;
#endif	   
	
}
#if 0
int SumaSTBSecure_GetUserID(char* szUserID)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_GetUserID \n");
#endif
	CH_Security_InputDialog("提示信息","请输入身份识别号:(合同号)",szUserID,0);
}
/*
功能：获取用户的登录ID（如合同号）和PIN码，
输出：
szUserID:	用户ID信息 
szPassword:用户口令
使用场景：
1、用户使用uKey时必须要求输入ID和口令
2、用户使用软证书时，根据标志位决定是否需要用户输入id和口令
*/
//如果用网页显示，这个信息首先是网页捕获的，如果是机顶盒提供
//则需要提供个具体的操作步骤
int SumaSTBSecure_GetLogonInfo(char* szUserIDInfo,char * szPIN)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_GetLogonInfo \n");
#endif
	CH_Security_UkeyDialog("请输入UKEY信息","用户名","密码", szUserIDInfo, szPIN);
}

/*
功能：提示用户选择
输出：0 :yes   others: no
szMsg: 界面信息
*/
//啥意思?
int SumaSTBSecure_YesNoBox(const char* szMsg)
{
	int result = 2;
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_YesNoBox \n");
#endif
	result = CH_Security_PopupDialog("请选择",szMsg,NULL,3,-1, 1);

	if(result == 1)
	{
		result = 0;
	}
	

	return result;

}

/*===================== 获取用户输入信息和图形界面提示相关函数 ========================*/
/*
功能：用图形界面向用户输出提示信息
		用户确定后，此图形界面应消失。
输入：szTitle:	提示窗口的标题。如“操作成功”，"操作失败"，"操作正进行，请等待"等
输出：szPromptMsg:	提示内容。如操作失败原因等 

*/
/* 是否支持超时时间，超时时间多少，显示是否采用歌华现有的对话框格式*/
int SumaSTBSecure_ShowPromptMsg(const char* szTitle, const char* szPromptMsg,int waitsecond)
{
#ifdef stb_scms_debug
	do_report(0,"SumaSTBSecure_ShowPromptMsg \n");
#endif
	CH_Security_PopupDialog(szTitle,szPromptMsg,"确认",1,waitsecond, 1);
	return 0;

}

#endif
/*===================== 调试&提示信息相关函数 =======================*/
/*
	1.	接口定义
	void SumaSTBSecure_AddDebugMsg(const char * szMsg);
	2.	功能描述
	用于在移植时打印调试信息使用。移植完毕后，该函数直接返回不做任何处理。
	3.	参数说明
	输入参数：无
	输出参数：
		szMsg：调试信息串
	返 回：
	0表示成功，其它表示失败。
	4.	应用场景
	机顶盒集成时，为方便得到具体的调试信息，可以调用此接口。
*/
void SumaSTBSecure_AddDebugMsg(const char * szMsg, ... )
{
#ifdef _SUMA_DEBUG_
	va_list Argument;

	va_start( Argument, szMsg );
	vsprintf( suma_debug_buff, szMsg, Argument );
	va_end( Argument );
	do_report(0,"%s", suma_debug_buff );
	
#endif


}
#if 0
/*=====================UKey使用的相关函数===============================*/
/*
	1.	接口定义
		HANDLE SumaSTBSecure_OpenUKey(void);
	2.	功能描述
		打开UKey，获得UKey的句柄，如果return NULL，则表示没有UKEY，否则，返回Ukey的句柄。
	3.	参数说明：
			输入参数：无
			输出参数：无
		返 回：
			USBKey句柄。
	4.	应用场景

*/
HANDLE SumaSecure_OpenUKey(void)
{


}
/*
	1.	接口定义
		int  SumaSTBSecure_WriteDataToUKey(HANDLE hUkey,BYTE * pbyinData,WORD *pwLen);
	2.	功能描述
		获取机顶盒显示时间、机顶盒cpu计数、机顶盒任意一段内存的数据。
	3.	参数说明：
		输入参数：
			hUkey：USBKey句柄
			pbyinData：需写入的数据的缓冲区
			pwLen：写书的数据的长度		  
		输出参数：
		pwLen：实际写入的数据的长度
		返 回：
			0表示成功，其它表示失败。
	4.	应用场景
*/
int SumaSecure_WriteDataToUKey(HANDLE  hUkey,BYTE * pbyinData,WORD *pwLen)
{


}
/*
	1.	接口定义
		int SumaSTBSecure_ReadDataFromUKey(HANDLE hUkey,BYTE * pbyoutData,WORD *pwLen);
	2.	功能描述
		从USBKey中读出数据。
	3.	参数说明：
		输入参数：
			hUkey：USBKey句柄
			pbyoutData：读出数据的缓冲区
		输出参数：
			pwLen：实际读出的数据的长度
		返 	回：
			0表示成功，其它表示失败。
		4.	应用场景

*/
int  SumaSecure_ReadDataFromUKey(HANDLE hUkey,BYTE * pbyoutData,WORD *pwLen)
{

}

/*=====================通讯相关函数===============================*/
//发送数据到PDS并接收
// 这个是啥意思
int SumaSTBSecure_SendDataToPDS(BYTE * pbySend,WORD wSendLen,BYTE * pbyRev,WORD *wpRevLen)
{

#define MAX_SEND_LEN 1460
	struct sockaddr* adr;
	struct sockaddr_in  so_ad;
	
	int socketindex;

	int err ;
	int ret;

	
	int ip ;/*要发送数据的IP地址*/
	int port = 8900;

	CH_iptoint("192.166.62.71",&ip);

	 socketindex = socket(AF_INET,SOCK_STREAM,IPPROTO_IP);
	
	memset(&so_ad,0,sizeof(struct sockaddr_in));

	if(socketindex == -1)
	{
		return -3;		
	}

	so_ad.sin_addr.s_addr = htonl(ip);
	so_ad.sin_port = htons(port);
	so_ad.sin_family = AF_INET;
	so_ad.sin_len    = sizeof(struct sockaddr_in);  

		/* 连接 服务器 */
	adr = (struct sockaddr *)&so_ad;

	err = connect(socketindex,adr,sizeof(struct sockaddr));

	if( err < 0)
	{
		return -3;
	}

	if(wSendLen <= MAX_SEND_LEN)
	{
		ret = send(socketindex,pbySend,wSendLen,0);
	}
	else
	{
		int pos=0;
		int i;
		int times = wSendLen/MAX_SEND_LEN;

			do_report(0,"SumaSTBSecure_SendDataToPDS Socket send len=%d tims=%d\n",wSendLen,times);

			for(i = 0;i < times;i++)
			{
				do_report(0,"pos= %d\n",pos);
				ret	= send(socketindex,(pbySend+pos),MAX_SEND_LEN,0);
				
				do_report(0,"send len %d\n",ret);
				
				if( ret  == MAX_SEND_LEN)/* 发送数据正常*/
				{
					pos += MAX_SEND_LEN;			
				}
				else 
				{
					return pos;
				}
				
			}

			if( (wSendLen%MAX_SEND_LEN)  !=  0)
			{

				int templen = wSendLen -times*MAX_SEND_LEN;
				ret = send(socketindex,(pbySend+pos),templen,0);
				if( ret  ==  templen)
				{
					pos += templen;
				}
				else
				{
					return pos;
				}
					
			}
			
		
	}

	ch_recvtimeout(socketindex, pbyRev, wpRevLen,0, 5);
	//recv(socketindex, pbyRev,wpRevLen, 0);

}

#endif


