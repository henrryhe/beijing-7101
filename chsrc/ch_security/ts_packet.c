

#include "stddefs.h"
#include "stcommon.h"
#include "sectionbase.h"
#include "..\dbase\vdbase.h"
#include "..\main\initterm.h"
#include "..\key\keymap.h"
#include "ts_packet.h"

static STPTI_Signal_t stru_TsmSignalHandle;
static STPTI_Buffer_t stru_BufferHandle = -1;
static STPTI_Slot_t stru_SlotHandle=-1;

static semaphore_t *pst_CaptureSema=NULL;
static semaphore_t *pst_TsReadSema=NULL;
static semaphore_t *pst_RWbufferSema=NULL;
static CH_Security_tspacket_t pstru_tsparameter;
static U8* puc_TSpackagebuffer = NULL;

static U8  puc_TsStatus = 0;/* 0 没有获取 1开始获取*/
static U8  uc_TsReady = 0;/*内存中存在的ts包个数*/

#ifdef DEBUG_PRINT
#undef DEBUG_PRINT
#endif
/*#define DEBUG_PRINT sqzow20100630*/
#define SYNC_TS_PACKET

void CH_TsPackageTask(void* para);

void CH_ResetTsParameter(void)
{
	memset(&pstru_tsparameter, 0, sizeof(CH_Security_tspacket_t));
}
void  CH_AddTsCapturePara(int ri_fre,int ri_sym,int ri_qam,int ri_audiopid)
{
	int i;

	i = pstru_tsparameter.i_xptrnumber;
	
#ifdef DEBUG_PRINT
	do_report(0,"add %d fre %d sym %d qam %d audio 0x%x",i,ri_fre,ri_sym,ri_qam,ri_audiopid);
#endif /* DEBUG_PRINT */
	
	
	pstru_tsparameter.stru_CaptureParalist[i].i_frequency = ri_fre/10 ;
	pstru_tsparameter.stru_CaptureParalist[i].i_symbol_rate= ri_sym/10;
	pstru_tsparameter.stru_CaptureParalist[i].i_modulation= ri_qam;
	pstru_tsparameter.stru_CaptureParalist[i].i_audiopid= ri_audiopid;

	if(pstru_tsparameter.i_xptrnumber  < CH_SECURITY_MAX_TSCAPTURE_NUM - 1)/* sqzow20100623*/
	{
		pstru_tsparameter.i_xptrnumber ++;
	}
	return;

}

void CH_SaveTsCapturePara(void)
{
	CH_Security_tspacket_t  stru_tempitem;
	int len;
	U8 data = 0Xa5;
	len = sizeof(CH_Security_tspacket_t);

	ReadNvmData(START_TSCAPTURE_PARALIST+1,len, stru_tempitem);

	if(memcmp(&stru_tempitem,&pstru_tsparameter,len))
	{
		WriteNvmData(START_TSCAPTURE_PARALIST+1,len,&pstru_tsparameter);
		WriteNvmData(START_TSCAPTURE_PARALIST,1, &data);
	}

	return;
}

BOOL CH_Security_AllocateSlot(STPTI_Pid_t  rui_pid)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_SlotData_t stru_SlotData;
	STPTI_Slot_t stru_SlotTemp;

	
	stru_SlotData.SlotType=STPTI_SLOT_TYPE_RAW;
	stru_SlotData.SlotFlags.SignalOnEveryTransportPacket=false;
	stru_SlotData.SlotFlags.CollectForAlternateOutputOnly=false;
	stru_SlotData.SlotFlags.AlternateOutputInjectCarouselPacket=false;
	stru_SlotData.SlotFlags.StoreLastTSHeader=false;
	stru_SlotData.SlotFlags.InsertSequenceError=false;
	stru_SlotData.SlotFlags.OutPesWithoutMetadata=false;
	stru_SlotData.SlotFlags.ForcePesLengthToZero=false;
	stru_SlotData.SlotFlags.AppendSyncBytePrefixToRawData=false;
	stru_SlotData.SlotFlags.SoftwareCDFifo = false; 
	ErrCode=STPTI_SlotAllocate(PTIHandle,&stru_SlotHandle,&stru_SlotData);
	
	if(ErrCode!=ST_NO_ERROR)
	{
#ifdef DEBUG_PRINT
		do_report(0,"CH_Security_AllocateSlot STPTI_SlotAllocate error%s \n",GetErrorText( ErrCode));
#endif /* DEBUG_PRINT */
		goto EXIT_TAG;
	}

	/*allocate buffer object*/
	ErrCode=STPTI_BufferAllocate(PTIHandle,SECTION_CIRCULAR_BUFFER_SIZE,1,&stru_BufferHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_AllocateSlot STPTI_BufferAllocate error%s \n",GetErrorText( ErrCode));
		goto EXIT_TAG;
	}


	ErrCode=STPTI_SlotLinkToBuffer(stru_SlotHandle,stru_BufferHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_AllocateSlot STPTI_SlotLinkToBuffer error%s \n",GetErrorText( ErrCode));
		goto EXIT_TAG;
	}


	/*associate signal with buffer object*/
	ErrCode=STPTI_SignalAssociateBuffer(stru_TsmSignalHandle,stru_BufferHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_AllocateSlot STPTI_SignalAssociateBuffer error%s \n",GetErrorText( ErrCode));
		goto EXIT_TAG;
	}

 	ErrCode=STPTI_PidQuery(PTIDeviceName,rui_pid,&stru_SlotTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH_Security_AllocateSlot STPTI_PidQuery()=%s",GetErrorText(ErrCode)));
		goto EXIT_TAG;
	}

	ErrCode=STPTI_SlotSetPid(stru_SlotHandle,rui_pid);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_AllocateSlot STPTI_SlotSetPid error%s \n",GetErrorText( ErrCode));
		goto EXIT_TAG;
	}


	return FALSE;
	
EXIT_TAG:
	
	if(stru_SlotHandle!=(STPTI_Slot_t)(-1)) STPTI_SlotDeallocate(stru_SlotHandle);
	if(stru_BufferHandle!=(STPTI_Buffer_t)(-1)) STPTI_BufferDeallocate(stru_BufferHandle);
	return TRUE;
	

}



void CH_Security_FreeSlot(void) 
{
	ST_ErrorCode_t ErrCode;	
	;
	STPTI_Signal_t SignalHandleTemp;

	if(stru_SlotHandle	 == (STPTI_Slot_t)(-1))
	{
		return;	
	}
	
	ErrCode=STPTI_SlotClearPid(stru_SlotHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_FreeSlot STPTI_SlotClearPid error%s \n",GetErrorText( ErrCode));
	}
	ErrCode=STPTI_SignalDisassociateBuffer(stru_TsmSignalHandle,stru_BufferHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_FreeSlot STPTI_SignalDisassociateBuffer error%s \n",GetErrorText( ErrCode));
	}

	/*deallocate buffer object*/

	ErrCode=STPTI_BufferDeallocate(stru_BufferHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_FreeSlot STPTI_BufferDeallocate error%s \n",GetErrorText( ErrCode));
	}


	/*deallocate slot */
	ErrCode=STPTI_SlotDeallocate(stru_SlotHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"CH_Security_FreeSlot STPTI_SlotDeallocate error%s \n",GetErrorText( ErrCode));
	}

	stru_SlotHandle = (STPTI_Slot_t)(-1);
	stru_BufferHandle = (U32)(-1)/* sqzow20100623*/;
	return ;
}

int SumaSTBSecure_Notify_Ts(tsm_porting_tsnotify_s  tsnotify_s)
{
#ifdef DEBUG_PRINT
	do_report(0,"SumaSTBSecure_Notify_Ts cmd %d \n",tsnotify_s);
#endif /* DEBUG_PRINT */

	
	if(tsnotify_s ==SUMA_STB_GETTS_START )
	{
		MakeVirtualkeypress(VIRKEY_CHANGETS_KEY);
		task_delay(1000);
		if(pstru_tsparameter.i_xptrnumber == 0)/*如果没有获取到ts packet参数，则不启动 sqzow20100626*/
		{
			do_report(0, "SumaSTBSecure_Notify_Ts can't get ts capture para!\n");
			return -1;
		}
		puc_TsStatus = 1;
		
#ifdef SYNC_TS_PACKET
		CH_TsPackageTask(NULL);
#else
		semaphore_signal(pst_CaptureSema);
#endif /* SYNC_TS_PACKET */
	}
	else if(tsnotify_s == SUMA_STB_GETTS_STOP)
	{
		puc_TsStatus = 0;
		if(stru_SlotHandle !=(STPTI_Slot_t)(-1))
		{
			CH_Security_FreeSlot();
		}
		if(puc_TSpackagebuffer != NULL)
		{
			memory_deallocate(CHSysPartition, puc_TSpackagebuffer);
			puc_TSpackagebuffer = NULL;
		}
#ifdef SYNC_TS_PACKET	
		CH_TsPackageTask(NULL);
#else
		semaphore_signal(pst_CaptureSema);
#endif /* SYNC_TS_PACKET */		
		task_delay(ST_GetClocksPerSecond());

	}

	 return 0;
}


#if 1/*sqzow20100624*/
int SumaSTBSecure_GetOneTsPacket(unsigned char * ptsData,int nLen)
{
#ifdef DEBUG_PRINT
	do_report(0,"SumaSTBSecure_GetOneTsPacket\n");
#endif /* DEBUG_PRINT */
	
	if(ptsData ==	NULL ||nLen != SECURITY_TSPACKET_LENGTH)
	{
		do_report(0,"SumaSTBSecure_GetOneTsPacket buffer0x%x nlen %d \n",ptsData,nLen);
		return SUMA_STB_GETTS_LENERROR;
	}

	if(puc_TsStatus == 0)
	{
		do_report(0,"SumaSTBSecure_GetOneTsPacket Failed didn't start  \n");
		return SUMA_STB_GETTS_FAILED;
	}

	semaphore_wait(pst_RWbufferSema);

	if(puc_TSpackagebuffer != NULL 
		&& uc_TsReady > 0)
	{
		memcpy(ptsData,puc_TSpackagebuffer,SECURITY_TSPACKET_LENGTH);
		memory_deallocate(CHSysPartition, puc_TSpackagebuffer);
		puc_TSpackagebuffer = NULL;
#ifdef DEBUG_PRINT1
		{
			int i;
			do_report(0, "\n================\n");
			for(i = 0; i < SECURITY_TSPACKET_LENGTH; i++)
			{
				do_report(0, "0x%x ", ptsData[i]);
			}
			do_report(0, "\n================\n");
		} 
#endif /* DEBUG_PRINT */
		semaphore_signal(pst_RWbufferSema);
		semaphore_signal(pst_TsReadSema);
		uc_TsReady  = 0;
		return SUMA_STB_GETTS_OK;
	}
	else
	{
		semaphore_signal(pst_RWbufferSema);
		return SUMA_STB_GETTS_GETTING;
	}
}
#endif

void CH_TsPackageTask(void* para)
{
	int i = 0;
	int number;
	TRANSPONDER_INFO_STRUCT 	t_XpdrInfo;
	int i_Freq, i_symb, i_qam;
	U8 uLoop;
	BOOL b_lock = false;
	BOOL   b_ret;

#ifndef SYNC_TS_PACKET
	while(true)
#endif		
	{
#ifndef SYNC_TS_PACKET		
		semaphore_wait(pst_CaptureSema);
#endif		
		if(puc_TsStatus == 0)
		{			
			i = 0;
		}
		else if(puc_TsStatus == 1 
			&& i < pstru_tsparameter.i_xptrnumber)
		{
			do
			{
				t_XpdrInfo.iTransponderFreq 	= pstru_tsparameter.stru_CaptureParalist[i].i_frequency;
				t_XpdrInfo.iSymbolRate			= pstru_tsparameter.stru_CaptureParalist[i].i_symbol_rate;
				t_XpdrInfo.ucQAMMode			= pstru_tsparameter.stru_CaptureParalist[i].i_modulation;
				t_XpdrInfo.DatabaseType=CH_DVBC;
							
				CH6_SendNewTuneReq(0,&t_XpdrInfo);	

				b_lock = false;
				uLoop = 0;
				while( uLoop <= 7)
				{
					task_delay( ST_GetClocksPerSecondLow() /4);
					if ( TRUE== CH_TUNER_IsLocked() )
					{
						b_lock = true;
#ifdef DEBUG_PRINT
						do_report (0,"TS capture lock %d sucessfully \n",t_XpdrInfo.iTransponderFreq );
#endif /* DEBUG_PRINT */
						break;
					}

					uLoop ++;
				}
				
				if(b_lock == false)
				{
					i++;
					if(i >=  pstru_tsparameter.i_xptrnumber)
					{
						b_lock = true;
						do_report(0,"failed to Locked all Xptr\n");
					}
				}
				else
				{
#ifdef DEBUG_PRINT
					do_report(0,"CH_Security_AllocateSlot %d, i = %d\n",pstru_tsparameter.stru_CaptureParalist[i].i_audiopid, i);
#endif /* DEBUG_PRINT */

					b_ret = CH_Security_AllocateSlot(pstru_tsparameter.stru_CaptureParalist[i].i_audiopid);
					if(b_ret == true)
					{
						do_report(0,"CH_Security_AllocateSlot failed\n");
					}
#ifdef DEBUG_PRINT
					do_report(0,"semaphore_signal(pst_TsReadSema)\n");
#endif /* DEBUG_PRINT */
					semaphore_signal(pst_TsReadSema);
				}
			}while(b_lock == false);
		}
			
	}


}


void CH_Security_TsPackageReceptionTask(void* para)
{
    int BytesTransferred;
    ST_ErrorCode_t Error;
	unsigned char ts_temp[SECURITY_TSPACKET_LENGTH];
	
    STPTI_Buffer_t ReceptionTaskBufferHandle;

    while( TRUE )
    {
	semaphore_wait(pst_TsReadSema);

#ifdef DEBUG_PRINT
	do_report(0,"CH_Security_TsPackageReceptionTask STPTI_SignalWaitBuffer \n");
#else
	task_delay(1000);
#endif /* DEBUG_PRINT */

        Error = STPTI_SignalWaitBuffer(stru_TsmSignalHandle, &ReceptionTaskBufferHandle, 5000);
        if (Error == ST_NO_ERROR)
        {
#ifdef DEBUG_PRINT
        	do_report(0,"CH_Security_TsPackageReceptionTask TSdata  have come \n");
#else
		task_delay(1000);
#endif /* DEBUG_PRINT */

		 Error = STPTI_BufferReadTSPacket( ReceptionTaskBufferHandle,
                                  ts_temp,
                                  SECURITY_TSPACKET_LENGTH,
                                  NULL, 0,
                                  &BytesTransferred,
                                  STPTI_COPY_TRANSFER_BY_MEMCPY );
		    if (Error != ST_NO_ERROR || BytesTransferred != SECURITY_TSPACKET_LENGTH)
	            {
#ifdef DEBUG_PRINT
				do_report(0,"STPTI_BufferReadTSPacket error %s \n",GetErrorText( Error));
#endif /* DEBUG_PRINT */
				STPTI_BufferFlush( ReceptionTaskBufferHandle );
				semaphore_signal(pst_TsReadSema);
				continue;
	            }
		semaphore_wait(pst_RWbufferSema);
		
        	if(puc_TSpackagebuffer == NULL)
        	{
			puc_TSpackagebuffer = memory_allocate(CHSysPartition,SECURITY_TSPACKET_LENGTH);
		}

		if(puc_TSpackagebuffer !=NULL)
		{
			memcpy(puc_TSpackagebuffer, ts_temp, SECURITY_TSPACKET_LENGTH);
	       		uc_TsReady ++;
				 
#ifdef DEBUG_PRINT
			do_report(0,"CH_Security_TsPackageReceptionTask Read TSdata %s\n",GetErrorText(Error));
#endif /* DEBUG_PRINT */

		}
		else
		{
#ifdef DEBUG_PRINT
			do_report(0,"CH_Security_TsPackageReceptionTask Failed to allocate memory\n");
#endif /* DEBUG_PRINT */
		}

		semaphore_signal(pst_RWbufferSema);
        }
	else
	{
		if( Error != ST_ERROR_TIMEOUT ) 
		{
			STPTI_BufferFlush( ReceptionTaskBufferHandle );	
		}

#ifdef DEBUG_PRINT
		do_report(0,"CH_Security_TsPackageReceptionTask STPTI_SignalWaitBuffer error%s \n",GetErrorText( Error));
#endif /* DEBUG_PRINT */
	}
    }
}



BOOL CH_TsCaptureInit(void)
{
	U8 uc_checkdata = 0;
	ST_ErrorCode_t ErrCode;
	int i_len;
	task_t* TsPacketMonitorTask;

#ifndef SYNC_TS_PACKET	
	pst_CaptureSema = semaphore_create_fifo(0);
#endif
	pst_TsReadSema = semaphore_create_fifo(0);
	pst_RWbufferSema = semaphore_create_fifo(1);
	i_len = sizeof(CH_Security_tspacket_t);
/*安全模块E2P数据的初始化*/
	ReadNvmData(START_TSCAPTURE_PARALIST,1, &uc_checkdata);
	if(uc_checkdata != 0xA5)/*E2P没有格式化*/
	{
		CH_ResetTsParameter();
		
		if(WriteNvmData(START_TSCAPTURE_PARALIST+1,i_len,&pstru_tsparameter))
		{	
			uc_checkdata = 0xA5;
			WriteNvmData(START_TSCAPTURE_PARALIST, 1,&uc_checkdata);
		}
	}
	else
	{
		ReadNvmData(START_TSCAPTURE_PARALIST+1,i_len,&pstru_tsparameter);
	}

	

	/*PTI收取任务*/
	ErrCode=STPTI_SignalAllocate(PTIHandle,&stru_TsmSignalHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_SignalAllocate()=%s",GetErrorText(ErrCode));
	}

	if((TsPacketMonitorTask=Task_Create(CH_Security_TsPackageReceptionTask,
		NULL,2048*8,13,"TsPackageReceptionTask",0 )) == NULL )
	{
		STTBX_Print(("can't create SectionMonitorTask \n"));
		return TRUE;			
	}

#ifndef SYNC_TS_PACKET
	if((TsPacketMonitorTask=Task_Create(CH_TsPackageTask,
		NULL,2048*8,13 ,"TsCaptureTask",0 )) == NULL )
	{
		STTBX_Print(("can't create SectionMonitorTask \n"));
		return TRUE;			
	}
#endif /* SYNC_TS_PACKET */

	return FALSE;/* sqzow20100626*/
	

}



int CH_SumaSecurity_init(void)
{
			
		CH_TsCaptureInit();
	      if( SumaSecure_Init( ) != 0)
		{
			do_report(0,"SumaSecure_Init Failed!!");	
		}
	
		return 0;
}


