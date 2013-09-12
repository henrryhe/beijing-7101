#ifndef H_360ECHO
	#define H_360ECHO		
	#ifdef HOST_PC 
	#include "gen_types.h"              
	#endif
	/************************
		Echo structure 
	*************************/
	typedef struct
	{
	U8 best_EPQ_val;
	U8 bad_EPQ_val;
	S8 best_EPQ_val_idx ;
	U8 EPQ_ref;
	U8 I2CSpeed;
	U8 past_EPQ_val[8];
	U8 EPQ_val[16];
	S8  L1s2va3vp4;
	} FE_OFDMEchoParams_t;
	

#ifdef STTUNER_MINIDRIVER 

void Echo_Init         ( FE_OFDMEchoParams_t *pParams);
int  Get_EPQ(U8 Speed); 
void Dlong_scan(void)        ;
void Echo_long_scan    	( FE_OFDMEchoParams_t *pParams);
void active_monitoring 	( FE_OFDMEchoParams_t *pParams);
void passive_monitoring	( FE_OFDMEchoParams_t *pParams);  
void test_ber			( FE_OFDMEchoParams_t *pParams);
void Ack_long_scan		(void);
#endif

#ifndef STTUNER_MINIDRIVER
void Echo_Init         (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams);
int  Get_EPQ(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U8 Speed); 
void Dlong_scan(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)        ;
void Echo_long_scan    	(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams);
void active_monitoring 	(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams);
void passive_monitoring	(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams);  
void test_ber			(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_OFDMEchoParams_t *pParams);
void Ack_long_scan		(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
#endif

#endif

