
#ifndef H_LLTUN0360

	#define H_LLTUN0360

	#ifdef HOST_PC

    
    
	typedef unsigned int	U32;  
	typedef unsigned int    ST_ErrorCode_t;     
	typedef unsigned int    TUNER_Handle_t;
	
	
	#endif

	
	/* Tuner properties structure	*/
	
	



	/* exported functions */
	void TunerInfSetChargePump(int CpValue);
	TUNER_ERROR LL_TunerGetError(TUNTDRV_InstanceData_t *Tuner);
		
	void LL_TunerSetProperties(TUNTDRV_InstanceData_t* Tuner,TUNTDRV_InstanceData_t tnr);
	void LL_TunerGetProperties(TUNTDRV_InstanceData_t *Tuner,TUNTDRV_InstanceData_t *tnr);
	        
	void LL_TunerResetError(TUNTDRV_InstanceData_t* Tuner);
	void LL_TunerSetNbSteps(TUNTDRV_InstanceData_t* Tuner,int nbsteps);
	void LL_TunerInit(TUNTDRV_InstanceData_t* Tuner);
	int  LL_TunerGetNbSteps(TUNTDRV_InstanceData_t* Tuner);
	int  LL_TunerGetFrequency(TUNTDRV_InstanceData_t* Tuner);
	int  LL_TunerSelectBandwidth(TUNTDRV_InstanceData_t* Tuner, int bandwidth);

	int  LL_TunerGetTunerLock(TUNTDRV_InstanceData_t* Tuner);
	
	void LL_TunerSetRepeator(TUNTDRV_InstanceData_t* Tuner, unsigned short int  repeator);
	int  LL_TunerGetRepeator(TUNTDRV_InstanceData_t* Tuner);
	void LL_TunerSelect(TUNTDRV_InstanceData_t* Tuner,STTUNER_TunerType_t type,unsigned char address);
    ST_ErrorCode_t LL_TunerSetFrequency(TUNTDRV_InstanceData_t* Tuner,U32 frequency,U32* returned_freq);  
    U32 TunerGetFrequency(TUNTDRV_InstanceData_t* Tuner);
    ST_ErrorCode_t LNA_Calibration(TUNTDRV_InstanceData_t* Tuner, U32 Frequency);
    U32 TunerGetLO_Divider(TUNTDRV_InstanceData_t* Tuner);
    ST_ErrorCode_t TunerSetBandwidth(TUNTDRV_InstanceData_t* Tuner,U32 Bandwidth);
    
	

#endif 

