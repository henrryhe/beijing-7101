/*=====================================================
Copy Right By:
	R&D  Institute,Sichuan  Changhong  Network  Technologies Co.,Ltd.

File Name:
	QAM0297.C

Version Number:
	1.1
Author:
	


Purpose:


History:
	Date				Author			Actor
------------------------------------------------------
	2006-06-20		zxg				Created
	2007-08-31		Zhouxuchen		Moidify

======================================================*/

/******************************Include files******************************/
#include "qam0297.h"
#include "ch_tuner_mid.h"
#include "tcdrv.h"
#include "d0297e.h"
#include "Stapp_os.h"
/******************************Macro define******************************/
#define 	MAX_ACQUISITION_ATTEMPT_BEFORE_IQ_INVERT 3


#define ACQUISITION_TIMEOUT /*1000*/1000
#define LOCK_TEST_INTERVAL  50
#define MONITOR_LOCK_TEST_INTERVAL 100


/*{{{ enumerations & constants used for the time-being*/

#define TUNER_SETTLING_TIME       100      /* Settling time for freq set */

#define FORCE_TUNER_STATE(__new_state__) \
{ \
	semaphore_wait ( psemTunerStateWriteAccess ); \
	iCurTunerTaskState = __new_state__; \
	semaphore_signal ( psemTunerStateWriteAccess ); \
	}


#define TUNER_PAUSE                                \
{                                                  \
	FORCE_TUNER_STATE ( TUNER_IDLE );                \
	semaphore_wait ( psemTunerIdleStateEntered );    \
	}

#define TUNER_RESUME                               \
{                                                  \
	FORCE_TUNER_STATE ( SIGNAL_DETECTION );                \
	semaphore_signal( psemTunerIdleStateReleased );  \
}

/******************************struct define******************************/


/*************************globle variable define*************************/
static task_t   *ptidTunerTask;

static  int gsi_FrequncyKHz;		/*输入频率*/
static  int gsi_QamSize;        		/*输入QAM*/
static  int gsi_SymbolRateBds;    	 /*输入符号率*/
static  int gsi_SpectrumInverMode = 1;


/*----------------------------------------------------------------------------*/
static void tuner_process( void *pvParam )
{
	BOOL bLocked;
	int acquisition_time = 0;
	tuner_state_t  	TempCurTunerTaskState   = TUNER_IDLE;


	while ( TRUE )
	{
		semaphore_wait ( psemTunerStateWriteAccess ); 
		TempCurTunerTaskState = iCurTunerTaskState;
		semaphore_signal ( psemTunerStateWriteAccess ); 
		switch ( TempCurTunerTaskState )
		{
		case TUNER_IDLE:
			{
				semaphore_signal ( psemTunerIdleStateEntered );
				semaphore_wait ( psemTunerIdleStateReleased );
				break;
			}
		case SIGNAL_DETECTION:
			{
				ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "\n\nTuning Req2 => QAM[%d] F[%d] Sym[%d]\n",
					gsi_QamSize,
					gsi_FrequncyKHz,
					gsi_SymbolRateBds);
				Tuner_UnLock;
				/*we just keep this state in case we might need it one day*/
				acquisition_time = 0;
				demod_d0297E_ScanFrequency( gsi_FrequncyKHz, gsi_SymbolRateBds, gsi_QamSize, gsi_SpectrumInverMode);		
				UPDATE_TUNER_STATE ( SIGNAL_DETECTION, ACQUISITION );
				break;
			}
		case ACQUISITION:
			{		
				demod_d0297E_IsLocked(&bLocked);
				if( bLocked)
				{
					Tuner_Lock;
					ch_TUNER_DebugMessage (MESSAGE_LEVEL_NORMAL, "TSD => Data lock\n" );
					UPDATE_TUNER_STATE( ACQUISITION, MONITOR_TUNING );
										
				}
				else if( acquisition_time > ACQUISITION_TIMEOUT )
				{
					if(gsi_SpectrumInverMode== 0)
						gsi_SpectrumInverMode = 1;
					else if(gsi_SpectrumInverMode== 1)
						gsi_SpectrumInverMode = 0;
					
					do_report( 0, "\nTSD => switching spectrum inversion %s\n",gsi_SpectrumInverMode ? "ON" : "OFF" );

					demod_d0297E_ScanFrequency( gsi_FrequncyKHz, gsi_SymbolRateBds, gsi_QamSize, gsi_SpectrumInverMode);	
					acquisition_time = 0;
				}
				else
				{
					acquisition_time += LOCK_TEST_INTERVAL;
					ch_TUNER_DelayMSEL( LOCK_TEST_INTERVAL );
				}
				
				break;
			}
		case MONITOR_TUNING:
			{
			
				ch_TUNER_DelayMSEL( TUNER_SETTLING_TIME );
				demod_d0297E_IsLocked(&bLocked);
				if( !bLocked)
				{
					ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "TSD => Data lock lost\n");
										
					UPDATE_TUNER_STATE( MONITOR_TUNING, SIGNAL_DETECTION);
				}
			}
			break;/*20060712 add*/
		}
	}
}




/**************************************************************
Fuction Name:
	ch_QAM0297E_SetParameter

Purpose:


Using the globle and struct:

Input parameter:
	ri_FrequencyKHz		Frequency in Khz
	ri_SymbolRateKHz		SymbolRate in KHz

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
int  ch_QAM0297E_SetParameter (int ri_FrequencyKHz, int ri_SymbolRateKBds, int ri_QamMode, int ri_IQInversion)
{

	if (ri_FrequencyKHz==0 || ri_SymbolRateKBds==0 )
		return false;

#if 1
	TUNER_PAUSE;
#endif

	Tuner_UnLock;

	switch(ri_QamMode)
	{
		case 0:
			gsi_QamSize = 0;
			break;
		
		case 1:
		case 16:
			gsi_QamSize = 1;
			break;
		
		case 2:
		case 32:
			gsi_QamSize = 2;
			break;	
		
		case 3:
		case 64:
			gsi_QamSize = 3;
			break;	
		
		case 4:
		case 128:
			gsi_QamSize = 4;
			break;	
		
		case 5:
		case 256:
			gsi_QamSize = 5;
			break;
		
		default:
			gsi_QamSize = 3;
			break;
	}


	gsi_SymbolRateBds 		=	ri_SymbolRateKBds*1000;
	gsi_FrequncyKHz			=	ri_FrequencyKHz; 
	gsi_SpectrumInverMode	= ri_IQInversion/*INITIAL_SPECTRUM_INVERSION*/;

	ch_TUNER_DebugMessage(MESSAGE_LEVEL_NORMAL, "\n\nTuning Req1 => QAM[%d] F[%d] Sym[%d] IQ[%d]\n",
		gsi_QamSize,
		gsi_FrequncyKHz,
		gsi_SymbolRateBds,
		ri_IQInversion);


#if 0
	demod_d0297E_ScanFrequency( iTransponderFreq, iSymbolRate, cQamSize, iSpectrumInversion);

	CH_TUNER_DelayMSEL( TUNER_SETTLING_TIME );
	/* 6 MHz/Sec  SweepRate = 1000.0 * (UserSweepRate/Fs) */
#endif

#if 1
	TUNER_RESUME;
#endif

	return false;
}
/*}}}*/


/**************************************************************
Fuction Name:
	ch_TUNER_Qam0297_Init

Purpose:


Using the globle and struct:

Input parameter:
	NULL

Output parameter:
	NULL

Return Status:
	TRUE	FAILED
	FALSE	SUCCESFULL

Note:
	NULL

**************************************************************/
BOOL ch_TUNER_Qam0297_Init(void)
{
	void *TaskStack = NULL;
	//static tdesc_t desc;
	int iTaskSize ;
	int launch_status;

	demod_d0297E_Init();

	tuner_tdrv_Init();


	iTaskSize = TUNER_PROCESS_STACK_SIZE;
#if 0	
	TaskStack = (void*)ch_TUNER_MemMollac(iTaskSize);
	if (TaskStack == NULL)
	{
		ch_TUNER_DebugMessage (MESSAGE_LEVEL_NORMAL, "Can't allocated the memory for tast static!\n");
		return TRUE;
	}
#endif
	FORCE_TUNER_STATE(TUNER_IDLE);

#if 0
	 launch_status = task_init(tuner_process, NULL,
                          TaskStack, iTaskSize, &ptidTunerTask, &desc, TUNER_PROCESS_PRIORITY, "ch_tuner_Monitor",
                          (task_flags_t) 0);	
#else
	 ptidTunerTask = Task_Create ( tuner_process,NULL,iTaskSize,TUNER_PROCESS_PRIORITY,"ch_tuner_Monitor",0 ) ;

#endif
	if ( ptidTunerTask == NULL)
	{
		ch_TUNER_DebugMessage (MESSAGE_LEVEL_NORMAL, "Failure to create tuner monitor task!\n" );
		ch_TUNER_MemFree((U8*)TaskStack);
		return TRUE;
	}

#if 0
	ch_tuner_TestRegisters();
#endif
	
	return false;
}

/*--eof--------------------------------------------------------------------*/

