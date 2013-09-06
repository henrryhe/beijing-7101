/*
  Name:chdemux.h
  Description:header file of the bottom component of PTI section which wiil be mainly used for CA interface
  Authors:yxl
  Date          Remark
  2004-07-16    Created  
*/


/*Function description:
	Control whether the CA interface function use the inter semaphore or the outer semaphore,
	in default state,all the CA interface function use the inter semaphore 
Parameters description: 
	BOOL IsUseInnerSem:
				TRUE: stand for using inter semaphore
				FALSE:stand for using outer semaphore
return value:no

*/
extern void CH6_CAInterface_SemaphoreControl(BOOL IsUseInnerSem );

/*Function description:
	Allocate a slot used for getting section data  
Parameters description: 
	STPTI_Signal_t SignalHandle:the signal object which will connected to the allocated slot 
return value:
	-1:stand for no free slot,allocate fail
	other value:the index of the allocated slot
*/

extern short CH6_Allocate_Section_Slot(STPTI_Signal_t SignalHandle);


/*Function description:
	Set the Pid for a slot,but the PTI doesn't start capture data  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be set pid
	STPTI_Pid_t Pid:the value of the pid to be set 
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Set_Slot_Pid(unsigned short SlotIndex,STPTI_Pid_t Pid);


/*Function description:
	Free the section slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be freeed
	STPTI_Signal_t SignalHandle:the signal object which connected to the allocated slot 
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
/*extern BOOL CH6_Free_Section_Slot(unsigned short SlotIndex,STPTI_Signal_t SignalHandle);yxl 2004-07-22 modify this line to next line*/
extern BOOL CH6_Free_Section_Slot(unsigned short SlotIndex);


/*Function description:
	start the data captured operation for a slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be started
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Start_Slot(unsigned short SlotIndex);

/*Function description:
	Stop the data captured operation for a slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be Stopped
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Stop_Slot(unsigned short SlotIndex);


/*Function description:
	Allocate a filter used for filtering section data
Parameters description: 
	sf_filter_mode_t FilterMode:the mode of the filter,which may be 1K mode or 4K mode 
	app_t FilterApp:the filter sign,may be used for deciding the destination of the captured section data 

  	BOOLEAN MatchMode:the sign of section match mode
				TRUE:stand for equal match mode
				FALSE:stand for not equal match mode
  	BOOLEAN MulSection:Sign standing for whether allocate a Multi section filter or not
				TRUE:stand for allocating a Multi section filter
				FALSE:stand for not allocating a Multi section filter

	BOOLEAN IsAllocSectionMem:Sign standing for whether allocate section data store memory at the same time
				TRUE:stand for allocating section data store memory
				FALSE:stand for not allocating section data store memory
	return value:
	-1:stand for no free filter,allocate fail
	other value:the index of the allocated filter
*/

extern short CH6_Allocate_Section_Filter(sf_filter_mode_t FilterMode,app_t FilterApp,BOOLEAN MatchMode
								  ,BOOLEAN MulSection,BOOLEAN IsAllocSectionMem,partition_t* pSectMemPartition);
 /*yxl 2005-03-14 add paramter "partition_t* pSectMemPartition"*/
/*Function description:
	Free a section filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be freeed
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Free_Section_Filter(unsigned short FilterIndex);


/*Function description:
	Associate a slot with a filter  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be associated
	unsigned short FilterIndex:the index of the filter which will be associated
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Associate_Slot_Filter(unsigned short SlotIndex,unsigned short FilterIndex);


/*Function description:
	Set the filter data for a filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be set data
	U8* pData:
	U8* pMask:
	U8* pPatern:
	int DataLen:the data length
	BOOLEAN MatchMode:the sign of section match mode
				TRUE:stand for equal match mode
				FALSE:stand for not equal match mode
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Set_Filter(unsigned short FilterIndex,U8* pData,U8* pMask,U8* pPatern,int DataLen,BOOLEAN MatchMode);


/*Function description:
	Enable the use of a filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be enabled
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Start_Filter(unsigned short FilterIndex);


/*Function description:
	Disable the use of a filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be disabled
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Stop_Filter(unsigned short FilterIndex);


/*Function description:
	Get the free filter count  
Parameters description: NO
	
return value:
	int:the number of the free filter

*/
extern int CH6_GetFreeFilterCount(void);



/*yxl 2004-10-27 add this function*/
/*Function description:
	Get filter count associating with specified slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be Stopped
	unsigned short* pCount:
						IN:Pointer to an allocated unsigned short variable
						OUT:returned filter count
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Get_SlotAssociateFilter_Count(unsigned short SlotIndex,unsigned short* pCount);



/*yxl 2004-11-04 add this function*/
/*Function description:
	Find if the specified pid has been collected by a slot  
Parameters description: 
	STPTI_Pid_t Pid:the pid to be found

return value:short
	-1:stand for the specified pid hasn't been collected by any slot  
	-2:FALSE:stand for the specified pid is an invalid value
	other:stand for the slot index collecting the specified pid
*/
extern short CH6_FindSlotByPID(STPTI_Pid_t Pid);

/*yxl 2005-03-10 add this function */
/*Function description:
	Disassociate a filter from a slot   
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be disassociated
	unsigned short FilterIndex:the index of the filter which will be disassociated
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_DisAssociate_Slot_Filter(unsigned short SlotIndex,unsigned short FilterIndex);

/*yxl 2005-03-10 add this function*/
/*Function description:
	Clear the buffer linked to the specified slot.
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be cleared
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
extern BOOL CH6_Flush_Slot_Buffer(unsigned short SlotIndex);

