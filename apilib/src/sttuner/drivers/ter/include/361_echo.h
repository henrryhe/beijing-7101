#ifndef H_361ECHO
	#define H_361ECHO		
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
	} FE_361_OFDMEchoParams_t;



void Echo_Init_361         (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_361_OFDMEchoParams_t *pParams);
int  Get_EPQ_361(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,U8 Speed); 
void Dlong_scan_361(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)        ;
void Echo_long_scan_361    	(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_361_OFDMEchoParams_t *pParams);
void active_monitoring_361 	(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_361_OFDMEchoParams_t *pParams);
void passive_monitoring_361	(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, FE_361_OFDMEchoParams_t *pParams);  
void Ack_long_scan_361		(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
#endif

