/*
  Name:tuner.h
  Description:header file of tuner  for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-05-27   Created  
*/ 

/*
extern void TunerNotifyCallback(STEVT_CallReason_t Reason, const ST_DeviceName_t  RegistrantName,
                           STEVT_EventConstant_t Event, const void *EventData,
                           const void *SubscriberData_p);
*/


/*Function description:
	decide if the tuner locked the signal
Parameters description: NO
return value:
			TRUE:standfor the tuner is locked the signal 	
			FALSE:standfor the tuner isn't locked the signal 	
*/
extern BOOL CH6_IsSignalLocked(void);


/*Function description:
	set the tuner according to parameters
Parameters description: 
	Handle:the handle of the opened tuner
	U32 Frequency:the Frequency will be set in hz
	U32 SymbolRate:the symbolRate will be set in hz
	unsigned short QAMMode:the QAMMode will be set 
			0,4:standfor 4 QAM
			1,16:standfor 16 QAM
			2,64:standfor 64 QAM
			3,128:standfor 128 QAM
			4,256:standfor 256 QAM


return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 	
*/
extern BOOL CH6_SetTuner(STTUNER_Handle_t Handle,U32 Frequency,U32 SymbolRate,unsigned short QAMMode);


/*Function description:
	Initliaze the 0297J module inside the QAM5516 chip
Parameters description: no

return value:
			TRUE:standfor the function is unsuccessful operated 		
			FALSE:standfor the function is successful operated 
*/
extern BOOL CH6_0297JInit(void);/*yxl 2004-08-27 add this function*/
