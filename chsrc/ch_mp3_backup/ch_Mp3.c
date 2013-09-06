/**************************源文件头注释：开始*********************************/
/*版权所有: 四川长虹网络科技有限公司(2008)                                   */
/*文件名: ch_Mp3.c                                                           */
/*版本号: V1.0                                                               */
/*作者:   张强                                                               */
/*创建日期:2008.2.25                                                         */
/*源文件内容概述:  MP3软解码控制文件.                    					 */
/*主要函数概述:                                                              */
/*修改历史:                                                                  */
/*--  时间      作者    修改部分及原因                                       */
/*--1:                                            							 */
/*                                                                           */
/**************************源文件头注释：结束*********************************/

/***************************include部分：开始*********************************/

#include "stddefs.h"
#include "stapp_os.h"

/*OSPLUS Header File*/
#include <osplus/device.h>
#include <osplus/vfs.h>

#include "ch_Mp3.h"

typedef struct
{
	BOOL ForceForNewMp3;/*抢占播出新歌*/
	U8*  MP3FilePath;/*需要播放Mp3文件路径*/
	U8*  Play_Buffer;/*PCM注入Buffer*/
	U8*  Decode_Buffer;/*Mp3读取buffer*/ 
	U32  PlayBuf_Length;/*PCM注入Buffer长度*/
	task_t       *PlayTaskID,*DecodeTaskID;/*PCM注入进程*/	
	semaphore_t  *semDataBufFull;/*缓存满信号量*/
	semaphore_t  *semDataBufUsed;/*缓存使用完毕信号量*/
	semaphore_t  *semBeginTodecode;/*开始解码新mp3*/	
	ch_pcmstatus_e PlayStatus;/*当前播放状态*/

}ch_mp3play_t;

/*外部设置PCM播出进程优先级*/
U8                    gs_PCMInjectProcessPriority = 15;

/*MP3控制结构*/
static ch_mp3play_t   gs_mp3CtrlHdl;
static semaphore_t  *semMp3DataBuf=NULL;
/*外部设置的内存分区*/
static ST_Partition_t *gs_STPartition;

#ifdef USE_IPANEL
#include "..\eis\eis_api_media.h"

int  mp3_data_len=0;
unsigned char *  mp3_data_buf=NULL;
#endif

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_DecodesentlastData	                                         */
/*  函数功能: 一首歌播放完成,释放最后的数据播出信号量						 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: 无								                             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_DecodesentlastData(void)
{	
	if(gs_mp3CtrlHdl.PlayBuf_Length != 0)
	{
		//最后一次播出的
		semaphore_signal( gs_mp3CtrlHdl.semDataBufFull );
		//等待播完信号量
		semaphore_wait( gs_mp3CtrlHdl.semDataBufUsed ); 	

	}
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_PushDecodedPCMData	                                         */
/*  函数功能: 解码出的PCM压入解码128K缓存									 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: uST_GetBuffer - 需要拷出的buffer	uST_Length - 长度            */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_PushDecodedPCMData(U8* uST_GetBuffer,
											 U32 uST_Length ) 
{
	gs_mp3CtrlHdl.PlayBuf_Length += uST_Length;
	if(gs_mp3CtrlHdl.PlayBuf_Length < PCM_INJECT_BUFFER_SIZE)
	{
		memcpy( (gs_mp3CtrlHdl.Play_Buffer + gs_mp3CtrlHdl.PlayBuf_Length
				- uST_Length),
				uST_GetBuffer,
				uST_Length );
	}
	else
	{
		//发送Buffer满信号量
		gs_mp3CtrlHdl.PlayBuf_Length -= uST_Length;
		semaphore_signal(gs_mp3CtrlHdl.semDataBufFull);
		//等待播完信号量
		semaphore_wait(gs_mp3CtrlHdl.semDataBufUsed);

		gs_mp3CtrlHdl.PlayBuf_Length = uST_Length;
		memcpy(gs_mp3CtrlHdl.Play_Buffer,uST_GetBuffer,uST_Length );
	}
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_GetDecodedPCMData	                                         */
/*  函数功能: Inject Task取出解码PCM数据									 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: uST_GetBuffer - 需要拷入的buffer	uST_Length - 长度            */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
//#define DEBUG_PCM_FILE

void CH_GetDecodedPCMData(U16* uST_GetBuffer,
										   U32* uST_Length )
{
	int	i_Loop;

#if 1 	/*sqzow20100607*/
	if(NULL != semMp3DataBuf)
	{
		semaphore_wait(semMp3DataBuf);
		
		if(NULL == mp3_data_buf)
		{
			semaphore_signal(semMp3DataBuf);
			return ;
		}
		
		if(mp3_data_len )
		{
			memcpy(uST_GetBuffer,mp3_data_buf,mp3_data_len);
			*uST_Length = mp3_data_len;
			mp3_data_len=0;
		}
		else
		{
			*uST_Length = 0;
			sttbx_Print("==");
		}
		
		semaphore_signal(semMp3DataBuf);
	}
	else
	{
		return;
	}
#else
	//等待缓存满信号量
	semaphore_wait(gs_mp3CtrlHdl.semDataBufFull);
	memcpy(uST_GetBuffer,
			gs_mp3CtrlHdl.Play_Buffer,
			gs_mp3CtrlHdl.PlayBuf_Length);
	
	*uST_Length = gs_mp3CtrlHdl.PlayBuf_Length;	
	//发送使用完Buf信号量
	gs_mp3CtrlHdl.PlayBuf_Length = 0;
	semaphore_signal(gs_mp3CtrlHdl.semDataBufUsed);
#endif
#if 0/*放到注入时变换 sqzow20100607*/
	for ( i_Loop = 0 ; i_Loop < (*uST_Length >> 1); i_Loop++ )
	{
		__asm__("swap.b %1, %0" : "=r" (uST_GetBuffer[i_Loop]): 
				"r" (uST_GetBuffer[i_Loop]));
	}
#endif
}

#ifdef IPANEL_AAC_FILE_PLAY
void CH_GetDecodedAACData(U16* uST_GetBuffer,
										   U32* uST_Length )
{
	int	i_Loop;
	if(NULL != semMp3DataBuf)
	{
		semaphore_wait(semMp3DataBuf);
		
		if(NULL == mp3_data_buf)
		{
			semaphore_signal(semMp3DataBuf);
			return ;
		}
		
		if(mp3_data_len )
		{
			memcpy(uST_GetBuffer,mp3_data_buf,mp3_data_len);
			*uST_Length = mp3_data_len;
			mp3_data_len=0;
		}
		else
		{
			*uST_Length = 0;
		}
		
		semaphore_signal(semMp3DataBuf);
	}

}
void CH_AAC_Pause(void)
{
	if(gs_mp3CtrlHdl.PlayTaskID)
	{
		task_suspend(gs_mp3CtrlHdl.PlayTaskID);
	}
	gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_PAUSE_E;
}

#endif

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_MemoryAllocate                                            */
/*  函数功能: 内存分配封装函数												 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: uSize   - 分配内存大小			                             */
/*  输出参数: 无                                                             */
/*  返回值:   分配的缓存区;  NULL - 未分配到内存                             */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void* CH_MP3_MemoryAllocate(U32 uSize)
{
	void * Ptmp;
	
	if ( (Ptmp = memory_allocate( gs_STPartition, uSize)) == NULL )
	{
		do_report(0,"[MP3 Player]Mp3 buffer allocate error \n");
		return NULL;
	}
	return Ptmp;
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_MemoryDeallocate                                          */
/*  函数功能: 内存释放封装函数												 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: pvMemory   - 释放缓存区			                             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_MemoryDeallocate(void* pvMemory)
{
	if(pvMemory)
	memory_deallocate(gs_STPartition, pvMemory);
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_SetPlayTaskID                                             */
/*  函数功能: 设置播出进程ID值,主要用于暂停控制								 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: taskID - 	播出进程ID值							             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_SetPlayTaskID(task_t* taskID)
{
	gs_mp3CtrlHdl.PlayTaskID = taskID;
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_SetSamplingFreq                                           */
/*  函数功能: 设置播出进程ID值,主要用于暂停控制								 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: SamplingFrequency - 	采样率						             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_SetSamplingFreq(U32 SamplingFrequency)
{
	PCM_SetSamplingFreq(SamplingFrequency);
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_CheckBreakForNew	                                         */
/*  函数功能: 判断一首歌播出时，是否有新歌需要播出							 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: 无												             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
BOOL CH_MP3_CheckBreakForNew( void )
{
	if(gs_mp3CtrlHdl.ForceForNewMp3)
    {
		return true;
	}
	return false;
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_Pause			                                         */
/*  函数功能: 暂停播出														 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: 无								                             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_Pause(void)
{

	if(gs_mp3CtrlHdl.PlayTaskID)
	{
		task_suspend(gs_mp3CtrlHdl.PlayTaskID);
	}
	
	#ifdef USE_IPANEL
		if(NULL != semMp3DataBuf)
		semaphore_wait(semMp3DataBuf);
	#endif
	
	if(gs_mp3CtrlHdl.DecodeTaskID)
	{
		task_suspend(gs_mp3CtrlHdl.DecodeTaskID);
	}
	
	#ifdef USE_IPANEL
		if(NULL != semMp3DataBuf)
		semaphore_signal(semMp3DataBuf);
	#endif
	
	gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_PAUSE_E;
	do_report(0,"[MP3 Player]Pause! \n");	
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_Pause			                                         */
/*  函数功能: 恢复暂停播出													 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: 无								                             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_Resume(void)
{
	if(gs_mp3CtrlHdl.PlayTaskID)
	{
		task_resume(gs_mp3CtrlHdl.PlayTaskID);
	}
	
	if(gs_mp3CtrlHdl.DecodeTaskID)
	{
		task_resume(gs_mp3CtrlHdl.DecodeTaskID);
	}
	
	gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_PLAY_E;
	do_report(0,"[MP3 Player]Pause Resume! \n");
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_Mute				                                         */
/*  函数功能: MP3播出静音控制												 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: 无												             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_Mute(void)
{
	PCM_MuteAudio();
	do_report(0,"[MP3 Player]MP3 Muted! \n");	
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_UnMute			                                         */
/*  函数功能: MP3播出取消静音控制											 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: 无												             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_UnMute(U32 CurVolume)
{
	/*0 <= CurVolume <= 63*/
	PCM_UnmuteAudio(CurVolume);
	do_report(0,"[MP3 Player]MP3 Unmuted! \n");	
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_SetVolume		                                         */
/*  函数功能: 设置音量衰减													 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: 无												             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_SetVolume(U32 CurVolume)
{
	PCM_SetAttenuation(CurVolume);
	do_report(0,"[MP3 Player]Set Volume [%d] ! \n",CurVolume);	
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_CheckPlayOver	                                         */
/*  函数功能: 判断一首歌是否播出完成,用于模式控制							 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: 无												             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
BOOL CH_MP3_CheckPlayOver( void )
{
    if(gs_mp3CtrlHdl.PlayStatus == PCM_STATUS_OVER_E)
	{
		return true;
	}
	return false;
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_NewMp3			                                         */
/*  函数功能: 播出新的mp3,在播新Mp3时调用一次								 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: FilePath - 逻辑卷(USB设备,硬盘)绝对路径 SFreq	- 采样频率       */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_NewMp3(U8* FilePath,U32 SFreq )
{
	gs_mp3CtrlHdl.PlayBuf_Length    = 0;
	if(gs_mp3CtrlHdl.PlayStatus == PCM_STATUS_PAUSE_E)
	{
		CH_MP3_Resume();
	}
	gs_mp3CtrlHdl.ForceForNewMp3 = true;
	sprintf(gs_mp3CtrlHdl.MP3FilePath,"%s",FilePath);
	semaphore_signal( gs_mp3CtrlHdl.semBeginTodecode );
	
	/*延时的目的在于:当音频有数据播出时才能设置采样频率*/
	task_delay(ST_GetClocksPerSecond());
	/*新歌设置采样率*/
	CH_MP3_SetSamplingFreq(SFreq);
	
	do_report(0,"[MP3 Player]Enter New MP3... \n");
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_Mp3_DecoderTask		                                         */
/*  函数功能: 解码进程														 */
/*  用到的全局变量和结构: 无              		                             */
/*  输入参数: 无												             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
#ifdef USE_IPANEL

boolean  CH_MP3_InitMp3State(  )
{
	gs_mp3CtrlHdl.ForceForNewMp3=false;
	mp3_data_len=0;
	semMp3DataBuf= semaphore_create_fifo(1);
	#ifdef MP3_DATA
	mp3_data_buf = CH_MP3_MemoryAllocate(MP3_DECODE_BUFFER_SIZE);
	#else
	mp3_data_buf = CH_MP3_MemoryAllocate(PCM_INJECT_BUFFER_SIZE);
	#endif
	if((NULL == mp3_data_buf) || (NULL == semMp3DataBuf))
	{
		do_report(0,"\n CH_MP3_InitMp3State failed!");
		return false;
	}
	else
	{
		return true;
	}
}

void CH_CleanBuf(void)
{
	mp3_data_len=0;
	gs_mp3CtrlHdl.PlayBuf_Length = 0 ;
	
}
void  CH_MP3_TermMp3State( void )
{
	if(semMp3DataBuf)
	{
		semaphore_delete( semMp3DataBuf  );
		semMp3DataBuf=NULL;
	}
	
	if(mp3_data_buf)
	{
		CH_MP3_MemoryDeallocate(mp3_data_buf);
		semMp3DataBuf=NULL;
	}
}

boolean CH_IPANEL_MP3Start(unsigned char * mp3_buf, int len)
{
#if 1 	/*直接推送 sqzow20100607*/
	{
	if((len < 0) || (NULL == mp3_buf))
	{
		return false;
	}
	
	if(NULL != semMp3DataBuf)
	{
#if 1/*放到注入时变换 sqzow20100607*/
{
	int i_Loop, len_16;
	U16 *mp3_buf_16;

	if(mp3_buf && len > 0)
	{
		mp3_buf_16 = (U16*)mp3_buf;
		len_16 = len >> 1;
		for ( i_Loop = 0 ; i_Loop < len_16 ; i_Loop++ )
		{
			__asm__("swap.b %1, %0" : "=r" (mp3_buf_16[i_Loop]): 
					"r" (mp3_buf_16[i_Loop]));
		}
	}
}
#endif
	
		
		semaphore_wait(semMp3DataBuf);

		if(NULL == mp3_data_buf)
		{
			semaphore_signal(semMp3DataBuf);
			return false;
		}
		
		
		#ifdef MP3_DATA
		if((mp3_data_len + len) <= MP3_DECODE_BUFFER_SIZE)
		#else
		if((mp3_data_len + len) <= PCM_INJECT_BUFFER_SIZE)
		#endif
		{
			memcpy(mp3_data_buf + mp3_data_len, mp3_buf, len);
			mp3_data_len += len;
			if(mp3_data_len + len < PCM_INJECT_BUFFER_SIZE)
			{
				CH_IPANEL_MP3CallBack();
			}
			sttbx_Print(" inject %d\n", mp3_data_len);
		}
		else
		{
			sttbx_Print("ignore data\n");
			semaphore_signal(semMp3DataBuf);
			return false;
		}
		
		semaphore_signal(semMp3DataBuf);
	}
	else
	{
		return false;
	}
	
	return true;
}
#else
	if((len < 0) || (NULL == mp3_buf))
	{
		return false;
	}
	
	if(NULL != semMp3DataBuf)
	{
		semaphore_wait(semMp3DataBuf);
		
		if(NULL == mp3_data_buf)
		{
			semaphore_signal(semMp3DataBuf);
			return false;
		}
		//do_report(0,"\n 应用层推送数据len=%d",len);

		#ifdef MP3_DATA
		if((mp3_data_len + len) <= MP3_DECODE_BUFFER_SIZE)
		#else
		if((mp3_data_len + len) <= PCM_INJECT_BUFFER_SIZE)
		#endif
		{
			memcpy(mp3_data_buf + mp3_data_len, mp3_buf, len);
			mp3_data_len += len;
			
		}
		else
		{
			mp3_data_len = len;
			memcpy(mp3_data_buf, mp3_buf, len);
		}
		
		semaphore_signal(semMp3DataBuf);
	}
	else
	{
		return false;
	}

	if(NULL != gs_mp3CtrlHdl.semBeginTodecode )
		semaphore_signal( gs_mp3CtrlHdl.semBeginTodecode );
#endif
	return true;
}

boolean CH_IPANEL_AACStart(unsigned char * mp3_buf, int len)
{
	if((len < 0) || (NULL == mp3_buf))
	{
		return false;
	}
	
	if(NULL != semMp3DataBuf)
	{
		
		semaphore_wait(semMp3DataBuf);

		if(NULL == mp3_data_buf)
		{
			semaphore_signal(semMp3DataBuf);
			return false;
		}
		
		
		#ifdef MP3_DATA
		if((mp3_data_len + len) <= MP3_DECODE_BUFFER_SIZE)
		#else
		if((mp3_data_len + len) <= PCM_INJECT_BUFFER_SIZE)
		#endif
		{
			memcpy(mp3_data_buf + mp3_data_len, mp3_buf, len);
			mp3_data_len += len;
			eis_report(" inject %d\n", mp3_data_len);
			#if 0
			{
				FILE *fp = NULL;
				fp = fopen("h://mpeg3.aac","ab");
				if(fp != NULL)
			    	{
					fwrite(mp3_buf,len,1,fp);
					fclose(fp);
				}
			}		
			#endif
		}
		else
		{
			eis_report("ignore data\n");
			semaphore_signal(semMp3DataBuf);
			return false;
		}
		
		semaphore_signal(semMp3DataBuf);
	}
	else
	{
		return false;
	}
	
	return true;
}

static void CH_Mp3_DecoderTask (void *pvParam)
{
	int len_bak = 0;
	int loop;
	
	if(gs_mp3CtrlHdl.semBeginTodecode)
	{
		while(true)
		{
			semaphore_wait( gs_mp3CtrlHdl.semBeginTodecode );
			gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_PLAY_E;
			if((NULL != semMp3DataBuf) && (NULL != gs_mp3CtrlHdl.Decode_Buffer)&& (NULL != mp3_data_buf) && (mp3_data_len > 0))
			{
				semaphore_wait(semMp3DataBuf);
				#ifdef MP3_DATA
				if(mp3_data_len < MP3_DECODE_BUFFER_SIZE)
				#else
				if(mp3_data_len < PCM_INJECT_BUFFER_SIZE)
				#endif
				{
					memcpy(gs_mp3CtrlHdl.Decode_Buffer, mp3_data_buf, mp3_data_len);
					len_bak = mp3_data_len;
				}
				mp3_data_len = 0;
				
				semaphore_signal(semMp3DataBuf);
				#ifdef MP3_DATA
				decode((unsigned char const*)gs_mp3CtrlHdl.Decode_Buffer, len_bak);
				#else
				CH_PushDecodedPCMData((unsigned char const*)gs_mp3CtrlHdl.Decode_Buffer, len_bak);
				#endif
			}
			
			CH_IPANEL_MP3CallBack();
			gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_OVER_E;
			//Task_Delay(ST_GetClocksPerSecond() / 200);  
		}
	}
}

#else
static void CH_Mp3_DecoderTask (void *pvParam)
{
	vfs_file_t *file_ptr;
	U32        File_size,PieceFileCount,ReadInSize;
	int        iLoop;

	gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_FREE_E;
	while(true)
	{
		//等待开始解码信号量
		semaphore_wait( gs_mp3CtrlHdl.semBeginTodecode );
		
		file_ptr = vfs_fopen(gs_mp3CtrlHdl.MP3FilePath, "r");	
		if(file_ptr != NULL)
		{
			gs_mp3CtrlHdl.ForceForNewMp3 = false;
			gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_PLAY_E;
			vfs_fseek(file_ptr, 0, OSPLUS_SEEK_END );
			File_size = vfs_ftell(file_ptr);
			vfs_fseek(file_ptr, 0, OSPLUS_SEEK_SET );
			
			if(File_size % MP3_DECODE_BUFFER_SIZE == 0)
				PieceFileCount = File_size/MP3_DECODE_BUFFER_SIZE;
			else
				PieceFileCount = File_size/MP3_DECODE_BUFFER_SIZE + 1;
			
			for(iLoop = 0;iLoop < PieceFileCount;iLoop++ )
			{
			    if(gs_mp3CtrlHdl.ForceForNewMp3)
			    {
					gs_mp3CtrlHdl.ForceForNewMp3 = false;
					break;
				}
				if( PieceFileCount == 1 )
				{
					ReadInSize = File_size;
				}
				else if( iLoop == (PieceFileCount - 1) )
				{
					ReadInSize = File_size % MP3_DECODE_BUFFER_SIZE;
				}
				else
				{
					ReadInSize = MP3_DECODE_BUFFER_SIZE;
				}
				
				ch_vfs_fread(gs_mp3CtrlHdl.Decode_Buffer,
						  sizeof(U8),
						  ReadInSize,
						  file_ptr);
				
		    	decode((unsigned char const*)gs_mp3CtrlHdl.Decode_Buffer,
						ReadInSize);
				
			}

			CH_DecodesentlastData();	
			vfs_fclose(file_ptr);
			do_report(0,"[MP3 Player]File been Closed!\n");
			gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_OVER_E;
		}
	}
}
#endif

void CH_MP3_PartitionStatus(void)
{
	partition_status_t PartitionStatus;

	if(partition_status(gs_STPartition,&PartitionStatus,0)
						 == OS21_SUCCESS)
	{
		do_report(0,
		"[MP3 Player]Appl_p Status:Free space = %d,Allocated Space = %d \n", 
		PartitionStatus.partition_status_free,
		PartitionStatus.partition_status_used);
	}
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_PCMPlayerInitlize                                         */
/*  函数功能: 初始化PCM播出,在进入播Mp3功能时调用一次						 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: 无								                             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
BOOL CH_MP3_PlayerInitlize(ST_Partition_t *ST_Partition,
									 U8 MP3_DecoderPriority,
									 U8 PCM_InjecterPriority )
{
    ST_ErrorCode_t ST_ErrorCode;

	do_report(0,"\n[MP3 Player]MP3 Player Initlize...\n");
	
	if(	  
		 MP3_DecoderPriority  > 15
		|| PCM_InjecterPriority > 15 )
	{
		do_report(0,"\n[MP3 Player]Parameter Error \n");	
		return FALSE;
	}
		
	gs_PCMInjectProcessPriority = PCM_InjecterPriority;
	if( ST_Partition == NULL)
		{
		extern ST_Partition_t *system_partition;
		gs_STPartition                = system_partition;
		}
	else
	gs_STPartition                = ST_Partition;
	
	CH_MP3_PartitionStatus();
	
	//初始化semaphore和分配内存
	#ifdef USE_IPANEL
	if(false==CH_MP3_InitMp3State())
		return FALSE;
	#endif
#if 0 	/*放到start时进行初始化 sqzow20100603*/
	
	gs_mp3CtrlHdl.semDataBufFull    = semaphore_create_fifo(0);
	gs_mp3CtrlHdl.semDataBufUsed    = semaphore_create_fifo(0);
	gs_mp3CtrlHdl.semBeginTodecode  = semaphore_create_fifo(0);
	gs_mp3CtrlHdl.PlayBuf_Length    = 0;
	
	gs_mp3CtrlHdl.MP3FilePath = (char *)CH_MP3_MemoryAllocate(256);
	if( gs_mp3CtrlHdl.MP3FilePath == NULL )
	{
			return FALSE;
	}
	
	gs_mp3CtrlHdl.Play_Buffer = 
		(char *)CH_MP3_MemoryAllocate(PCM_INJECT_BUFFER_SIZE);
	if( gs_mp3CtrlHdl.Play_Buffer == NULL )
	{
		CH_MP3_MemoryDeallocate(gs_mp3CtrlHdl.MP3FilePath);
		return FALSE;
	}
	
	gs_mp3CtrlHdl.Decode_Buffer = 
		(char *)CH_MP3_MemoryAllocate(MP3_DECODE_BUFFER_SIZE);
	if( gs_mp3CtrlHdl.Decode_Buffer == NULL)
	{
		CH_MP3_MemoryDeallocate(gs_mp3CtrlHdl.MP3FilePath);
 		CH_MP3_MemoryDeallocate(gs_mp3CtrlHdl.Play_Buffer);	
		return FALSE;
	}
	
	//初始化播出部分
	#ifdef IPANEL_AAC_FILE_PLAY
	if(CH_get_AAC_play_state())
	{
		ST_ErrorCode = AAC_Comp_Start();
	}
	else
	#endif
    ST_ErrorCode = CH_MP3_PCMDecodeInit();
    if (ST_ErrorCode != ST_NO_ERROR)
    {
            /*删除控制semaphore*/
            semaphore_delete( gs_mp3CtrlHdl.semDataBufFull   );
            semaphore_delete( gs_mp3CtrlHdl.semDataBufUsed   );
            semaphore_delete( gs_mp3CtrlHdl.semBeginTodecode );
            
            /*删除控制semaphore*/
            semaphore_delete( gs_mp3CtrlHdl.semDataBufFull   );
            semaphore_delete( gs_mp3CtrlHdl.semDataBufUsed   );
            semaphore_delete( gs_mp3CtrlHdl.semBeginTodecode );
        return FALSE;
    }

	//初始化解码部分
	gs_mp3CtrlHdl.DecodeTaskID = Task_Create(CH_Mp3_DecoderTask,
												NULL,
												16*1024,
												MP3_DecoderPriority,
												"MP3_DecoderTask",
												0);

	if(gs_mp3CtrlHdl.DecodeTaskID == NULL)
	{
		CH_MP3_ExitPlayer();
		return FALSE;
	}

#endif

	do_report(0,"[MP3 Player]Task_Create[MP3_DecoderTask]!  \n");
	do_report(0,"[MP3 Player]MP3 Player Initlized! \n");	
	return (TRUE);
}

/***************************** 函数定义注释头*********************************/
/*  函数名: CH_MP3_ExitPlayer		                                         */
/*  函数功能: 退出MP3播出,在退出播Mp3时调用									 */
/*  用到的全局变量和结构: 无                                                 */
/*  输入参数: 无								                             */
/*  输出参数: 无                                                             */
/*  返回值:   无                            								 */
/*  调用注意事项:无                                                          */
/*  其他说明:    无                                                          */
/*******************************函数体定义************************************/
void CH_MP3_ExitPlayer( void )
{
	int iLoop = 0;

#if 0 	/*sqzow20100608*/
	/*当在暂停状态下，需要恢复decodeTaskID并让其释放相关资源*/
	/*任务将关闭打开文件,释放解码器内存资源*/
	if(gs_mp3CtrlHdl.PlayStatus == PCM_STATUS_PAUSE_E)
	{
		if(gs_mp3CtrlHdl.DecodeTaskID)
		{
			task_resume(gs_mp3CtrlHdl.DecodeTaskID);
		}
		gs_mp3CtrlHdl.PlayStatus = PCM_STATUS_PLAY_E;	
	}

	/*当在播放状态下,需要等待解码器释放内存资源*/
	iLoop = 0;
	if(gs_mp3CtrlHdl.PlayStatus == PCM_STATUS_PLAY_E)	
	{
		gs_mp3CtrlHdl.ForceForNewMp3 = true;
		/*释放一次该信号量的目的在于,让解码进程不处于等待状态,运行起来,总共有两次*/
		semaphore_signal(gs_mp3CtrlHdl.semDataBufUsed);
		semaphore_signal(gs_mp3CtrlHdl.semDataBufUsed);
		while(gs_mp3CtrlHdl.PlayStatus != PCM_STATUS_OVER_E && iLoop < 50)
		{
			task_delay(ST_GetClocksPerSecond()/10);
			iLoop++;
		}
		if(iLoop >= 50)
		{
			do_report(0,"[MP3 Player]Memory in decoder may not be deleted!\n");
		}
	}
#endif

	if(gs_mp3CtrlHdl.PlayStatus == PCM_STATUS_PAUSE_E)/*如是暂停状态，先恢复 sqzow20100611*/
	{
		CH_MP3_Resume();
	}
	Comp_Stop();

#if 0 	/*sqzow20100608*/
	if(gs_mp3CtrlHdl.DecodeTaskID)
	{
		task_suspend( gs_mp3CtrlHdl.DecodeTaskID  );	
		task_kill( gs_mp3CtrlHdl.DecodeTaskID,0,0 );
		task_delete( gs_mp3CtrlHdl.DecodeTaskID  );
	}
	
	do_report(0,"[MP3 Player]Decode Task Deleted! \n");

	/*Comp_Stop已经删除了播出进程,这儿把它设为空*/
	gs_mp3CtrlHdl.PlayTaskID = NULL;

	/*删除控制semaphore*/
	semaphore_delete( gs_mp3CtrlHdl.semDataBufFull   );
	semaphore_delete( gs_mp3CtrlHdl.semDataBufUsed   );
	semaphore_delete( gs_mp3CtrlHdl.semBeginTodecode );
#endif

	/*删除分配的内存*/
	CH_MP3_MemoryDeallocate( gs_mp3CtrlHdl.Play_Buffer   );
	CH_MP3_MemoryDeallocate( gs_mp3CtrlHdl.Decode_Buffer );
	CH_MP3_MemoryDeallocate( gs_mp3CtrlHdl.MP3FilePath   ); 
	
	#ifdef USE_IPANEL
	CH_MP3_TermMp3State();
	#endif

	/*do_report(0,"[MP3 Player]Exited MP3 player! \n");  sqzow20100610*/
}


