/*
  Name:chdemux.c
  Description:the bottom component of PTI section which wiil be mainly used for CA interface
  Author:yixiaoli	
  created date :2004-07-16 for changhong QAM5516 DBVC project
  modify:
  date:2004-10-10  by Yixiaoli
  1.add function: CH6_GetFreeFilterCount(void);
  date:2004-10-22  by Yixiaoli
  1.modify the bug of function CH6_GetFreeFilterCount(void)
  2.add function CH6_CAInterface_SemaphoreControl(BOOL IsUseInnerSem ) and static variable IsUseSem,
	and modify all function to add semaphore control

  date:2004-10-24 by Yixiaoli
  1.modify function CH6_Allocate_Section_Filter(),add two variable,boolean MulSection,boolean IsAllocSectionMem
 
   date:2004-10-27 by Yixiaoli
  1.add function BOOL CH6_Get_SlotAssociateFilter_Count(unsigned short SlotIndex,unsigned short* pCount);

  date:2004-11-04 by Yixiaoli
  1.modify function CH6_Start_Slot(unsigned short SlotIndex);
  2.add function CH6_FindSlotByPID(STPTI_Pid_t Pid);

  date:2004-11-08  by Yixiaoli
  1.modify function CH6_StartFilter(),add "SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_IN_USE;"
  2.modify function CH6_StopFilter(),add "SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_DONE;"

  date:2005-03-10  by Yixiaoli   由爱迪得CA 提出
  1.Add new function BOOL CH6_DisAssociate_Slot_Filter(unsigned short SlotIndex,unsigned short FilterIndex),
  2.Add new function BOOL CH6_Flush_Slot_Buffer(unsigned short SlotIndex);

  date:2005-03-14 by Yixiaoli 
  1.Add a new member "partition_t *pPartition" in struct CH6_SECTION_FILTER_t,which is used to 
  specify the memory partition of member "BYTE aucSectionData".
  2.According to new added member "partition_t *pPartition",modify below function:
  1)Modify inner of function CH6_Allocate_Section_Filter(),and add a new  parameter "partition_t* pSectMemPartition"
  2)Modify inner of function CH6_Free_Section_Filter()

  date:2006-12-30 by Yixiaoli
  1.Modify function CH6_Allocate_Section_Slot() and CH6_Allocate_Section_Filter(),
	change global variable PTIHandle to local variable.

date:2008-10-22 by Yixiaoli
1.Modify function CH6_Allocate_Section_Filter(),CH6_Set_Filter(),对不等模式作了更改，但未测试。

date:2008-10-23 by Yixiaoli
1.Add new function CH6_Clear_Slot_Pid().

*/

#include "stddefs.h"
#include "stcommon.h"
#include "sectionbase.h"
#include "..\dbase\section.h"
#include "..\main\initterm.h"
#include "chdemux.h"
#include "..\main\errors.h"

/*#define YXL_DEMUX_DEBUG*/

static BOOL IsUseSem=true;


void CH6_CAInterface_SemaphoreControl(BOOL IsUseInnerSem )
{
	IsUseSem=IsUseInnerSem;
	return;
}


/*Function description:
	Allocate a slot used for getting section data  
Parameters description: 
	STPTI_Signal_t SignalHandle:the signal object which will connect to the allocated slot 
return value:
	-1:stand for no free slot,allocate fail
	other value:the index of the allocated slot
*/
short CH6_Allocate_Section_Slot(STPTI_Signal_t SignalHandle)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp=-1;
	STPTI_Buffer_t BufferHandleTemp=-1;
	STPTI_SlotData_t SlotData;
	int SlotIndexTemp;
	CH6_SLOT_STATUS_t StatusTemp=SECTION_SLOT_FREE;
	/*yxl 2006-12-30 add below section*/
	STPTI_Handle_t PTIHandle=INVALID_VALUE;
	if(CH_GetPTIHandle(&PTIHandle))
	{
		STTBX_Print(("\nYxlInfo:Invalid PTI handle,quit"));
		return -1;
	}
	/*yxl 2006-12-30 add below section*/

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_STATUS,(void*)&StatusTemp);
	if(SlotIndexTemp==-1) goto exit1;

	SlotData.SlotType=STPTI_SLOT_TYPE_SECTION;
	SlotData.SlotFlags.SignalOnEveryTransportPacket=false;
	SlotData.SlotFlags.CollectForAlternateOutputOnly=false;
	SlotData.SlotFlags.AlternateOutputInjectCarouselPacket=false;
	SlotData.SlotFlags.StoreLastTSHeader=false;
	SlotData.SlotFlags.InsertSequenceError=false;
	SlotData.SlotFlags.OutPesWithoutMetadata=false;
	SlotData.SlotFlags.ForcePesLengthToZero=false;
	SlotData.SlotFlags.AppendSyncBytePrefixToRawData=false;
	ErrCode=STPTI_SlotAllocate(PTIHandle,&SlotHandleTemp,&SlotData);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
			
	}

	/*allocate buffer object*/

	ErrCode=STPTI_BufferAllocate(PTIHandle,SECTION_CIRCULAR_BUFFER_SIZE,1,&BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_BufferAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	#if 1
	/*associate slot with buffer*/

	ErrCode=STPTI_SlotLinkToBuffer(SlotHandleTemp,BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	/*STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));*/
	#endif

	/*associate signal with buffer object*/
	ErrCode=STPTI_SignalAssociateBuffer(SignalHandle,BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SignalAssociateBuffer()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	
	/* update the corresponding contents of structure SectionSlot */ 
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_IN_USE;
	SectionSlot[SlotIndexTemp].SlotHandle=SlotHandleTemp;
	SectionSlot[SlotIndexTemp].BufferHandle=BufferHandleTemp;
	SectionSlot[SlotIndexTemp].PidValue=INVALID_VALUE;
	SectionSlot[SlotIndexTemp].SignalHandle=SignalHandle;/*yxl 2004-07-22 add this line*/
	SectionSlot[SlotIndexTemp].FilterCount=0;

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return SlotIndexTemp;


exit1:
	if(SlotHandleTemp!=-1) STPTI_SlotDeallocate(SlotHandleTemp);
	if(BufferHandleTemp!=-1) STPTI_BufferDeallocate(BufferHandleTemp);
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);	
	return -1;
}

/*Function description:
	Set the Pid for a slot,but the PTI doesn't start capture data  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be set pid
	STPTI_Pid_t Pid:the value of the pid to be set 
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Set_Slot_Pid(unsigned short SlotIndex,STPTI_Pid_t Pid)
{
	int MaxSlotCount=MAX_NO_SLOT;
	if(SlotIndex<0||SlotIndex>=MaxSlotCount) 
		return TRUE;
	if(Pid<0||Pid>=0x1fff) 
		return TRUE;

	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	SectionSlot[SlotIndex].PidValue=Pid;
	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;

}


/*Function description:
	Free a section slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be freeed
	STPTI_Signal_t SignalHandle:the signal object which connected to the allocated slot 
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/

/*BOOL CH6_Free_Section_Slot(unsigned short SlotIndex,STPTI_Signal_t SignalHandle) yxl 2004-07-22 modify this line to next line*/
BOOL CH6_Free_Section_Slot(unsigned short SlotIndex) 
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Buffer_t BufferHandleTemp;
	STPTI_Signal_t SignalHandleTemp;

	int SlotIndexTemp=SlotIndex;
	int MaxSlotCount=MAX_NO_SLOT;
	if(SlotIndexTemp<0||SlotIndexTemp>=MaxSlotCount) 
		return TRUE;


    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionSlot[SlotIndexTemp].SlotStatus==SECTION_SLOT_FREE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	/*disassociate buffer with signal*/

	BufferHandleTemp=SectionSlot[SlotIndexTemp].BufferHandle;
	SignalHandleTemp=SectionSlot[SlotIndexTemp].SignalHandle;/*yxl 2004-07-22 add this line*/

	ErrCode=STPTI_SignalDisassociateBuffer(SignalHandleTemp,BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SignalDisassociateBuffer(A)=%s",GetErrorText(ErrCode)));
	}

	/*deallocate buffer object*/

	ErrCode=STPTI_BufferDeallocate(BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_BufferDeallocate()=%s",GetErrorText(ErrCode)));
	}


	/*deallocate slot */
	ErrCode=STPTI_SlotDeallocate(SectionSlot[SlotIndexTemp].SlotHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_BufferDeallocate()=%s",GetErrorText(ErrCode)));
	}

	/*update corresponding contents of structure SectionSlot*/

	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_FREE;
	SectionSlot[SlotIndexTemp].PidValue=INVALID_VALUE;			
	SectionSlot[SlotIndexTemp].SlotHandle=INVALID_VALUE;		
	SectionSlot[SlotIndexTemp].BufferHandle=INVALID_VALUE;   
	SectionSlot[SlotIndexTemp].SignalHandle=INVALID_VALUE; /*yxl 2004-07-22 add this line*/  
	SectionSlot[SlotIndexTemp].FilterCount=0;	

    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FALSE;
}


/*yxl 2008-10-23 add below function*/

BOOL CH6_Clear_Slot_Pid(unsigned short SlotIndex) 
{
	ST_ErrorCode_t ErrCode;	

	int SlotIndexTemp=SlotIndex;
	int MaxSlotCount=MAX_NO_SLOT;
	BOOL ResTemp=FALSE;
	if(SlotIndexTemp<0||SlotIndexTemp>=MaxSlotCount) 
	{
		return TRUE;
	}
	
    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	
	if(SectionSlot[SlotIndexTemp].FilterCount==0)
	{
		SectionSlot[SlotIndexTemp].PidValue=INVALID_VALUE;	
	}
	else
	{
		ResTemp=TRUE;
	}
	
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	
	return ResTemp;
}

/*end yxl 2008-10-23 add below function*/



/*Function description:
	start the data captured operation for a slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be started
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Start_Slot(unsigned short SlotIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Pid_t PidTemp;

	int MaxSlotCount=MAX_NO_SLOT;
	int SlotIndexTemp=SlotIndex;
	if(SlotIndex<0||SlotIndex>=MaxSlotCount) 
		return TRUE;

	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	if(SectionSlot[SlotIndexTemp].SlotStatus==SECTION_SLOT_FREE||\
		SectionSlot[SlotIndexTemp].PidValue==INVALID_VALUE||\
		SectionSlot[SlotIndexTemp].SlotHandle==INVALID_VALUE||\
		SectionSlot[SlotIndexTemp].BufferHandle==INVALID_VALUE)
	
		goto exit1;
	/*yxl 2004-11-04 add this section*/
	PidTemp=SectionSlot[SlotIndexTemp].PidValue;
	ErrCode=STPTI_PidQuery("PTI",PidTemp,&SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_PidQuery()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
/*	STTBX_Print(("\n yxlInfo():SlotHandleTemp=%lx %ld", SlotHandleTemp));*/
	if(SlotHandleTemp==0xfc000) 
	{ /*end yxl 2004-11-04 add this section*/	
	
		SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;
		ErrCode=STPTI_SlotSetPid(SlotHandleTemp,SectionSlot[SlotIndexTemp].PidValue);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotSetPid()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
	}

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;

}


/*Function description:
	Stop the data captured operation for a slot  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be Stopped
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Stop_Slot(unsigned short SlotIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	int MaxSlotCount=MAX_NO_SLOT;
	int SlotIndexTemp=SlotIndex;
	if(SlotIndex<0||SlotIndex>=MaxSlotCount) 
		return TRUE;

	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	if(SectionSlot[SlotIndexTemp].SlotStatus==SECTION_SLOT_FREE||\
		SectionSlot[SlotIndexTemp].PidValue==INVALID_VALUE||\
		SectionSlot[SlotIndexTemp].SlotHandle==INVALID_VALUE||\
		SectionSlot[SlotIndexTemp].BufferHandle==INVALID_VALUE)
	
		goto exit1;
	SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;

	ErrCode=STPTI_SlotClearPid(SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotClearPid()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;

}



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
BOOL CH6_Get_SlotAssociateFilter_Count(unsigned short SlotIndex,unsigned short* pCount)
{
	int MaxSlotCount=MAX_NO_SLOT;
	int SlotIndexTemp=SlotIndex;

	if(SlotIndex<0||SlotIndex>=MaxSlotCount) 
		return TRUE;

	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionSlot[SlotIndexTemp].FilterCount<0)
	{
		SectionSlot[SlotIndexTemp].FilterCount=0;
	}
	*pCount=SectionSlot[SlotIndexTemp].FilterCount;

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;
}

/*end yxl 2004-10-27 add this function*/



/*yxl 2004-11-02 add this function*/

/*Function description:
	Find if the specified pid has been collected by a slot  
Parameters description: 
	STPTI_Pid_t Pid:the pid to be found

return value:short
	-1:stand for the specified pid hasn't been collected by any slot  
	-2:FALSE:stand for the specified pid is an invalid value
	other:stand for the slot index collecting the specified pid
*/
short CH6_FindSlotByPID(STPTI_Pid_t Pid)
{
	int MaxSlotCount=MAX_NO_SLOT;
	int i;
	if(Pid==INVALID_VALUE) return -2;
	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	for(i=0;i<MaxSlotCount;i++)
	{
		if(SectionSlot[i].PidValue==Pid) break;
	}
	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	if(i>=MaxSlotCount) return -1;
	else 
		return i;
}
/*end yxl 2004-11-02 add this function*/



/*yxl 2005-03-10 add this function,needed by 爱迪得interface*/

/*Function description:
	Clear the buffer linked to the specified slot.
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be cleared
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Flush_Slot_Buffer(unsigned short SlotIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Buffer_t BufferHandleTemp;
	int MaxSlotCount=MAX_NO_SLOT;
	int SlotIndexTemp=SlotIndex;

	if(SlotIndex<0||SlotIndex>=MaxSlotCount) 
		return TRUE;

	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionSlot[SlotIndexTemp].BufferHandle==INVALID_VALUE||
		SectionSlot[SlotIndexTemp].SlotHandle==INVALID_VALUE) 
		 goto exit1;
    	
	BufferHandleTemp=SectionSlot[SlotIndexTemp].BufferHandle;
	ErrCode=STPTI_BufferFlush(BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH6_Clear_Slot_Buffer()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;
exit1:
	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}
/*end yxl 2005-03-10 add this function*/


#if 0 /*yxl 2004-10-24 modify this section for add two variable,boolean MulSection,boolean IsAllocSectionMem*/

/*Function description:
	Allocate a filter used for filtering section data
Parameters description: 
	sf_filter_mode_t FilterMode:the mode of the filter,which may be 1K mode or 4K mode 
	app_t FilterApp:the filter sign,may be used for deciding the destination of the captured section data 
	BOOLEAN MatchMode:the sign of section match mode
				TRUE:stand for equal match mode
				FALSE:stand for not equal match mode

  return value:
	-1:stand for no free filter,allocate fail
	other value:the index of the allocated filter
*/
short CH6_Allocate_Section_Filter(sf_filter_mode_t FilterMode,app_t FilterApp,BOOLEAN MatchMode)
{

	ST_ErrorCode_t ErrCode;	
	STPTI_Filter_t FilterHandleTemp=-1;

	int FilterIndexTemp;
	int SizeTemp=1024;

	sf_filter_mode_t ModeTemp=FilterMode;
	if(ModeTemp!=ONE_KILO_SECTION&&ModeTemp!=FOUR_KILO_SECTION) 
		return -1;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_MODE,(void*)&ModeTemp);
	if(FilterIndexTemp==-1) goto exit1;

	if(MatchMode)
	{
#ifdef ST_7109
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);
#else
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE,
			&FilterHandleTemp);
#endif
	}
	else
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE,
			&FilterHandleTemp);

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	if(ModeTemp==FOUR_KILO_SECTION) SizeTemp=4*SizeTemp;
	
	SectionFilter[FilterIndexTemp].aucSectionData=memory_allocate(CHSysPartition,SizeTemp);
	if(SectionFilter[FilterIndexTemp].aucSectionData==NULL)
	{
	   return -1;
	}
	memset(SectionFilter[FilterIndexTemp].aucSectionData,0,SizeTemp);

	/* update the corresponding contents of structure SectionFilter*/ 

	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_IN_USE;
	SectionFilter[FilterIndexTemp].FilterHandle=FilterHandleTemp;
	SectionFilter[FilterIndexTemp].sfFilterMode=FilterMode;
	SectionFilter[FilterIndexTemp].pMsgQueue=NULL;
	SectionFilter[FilterIndexTemp]. appfilter=FilterApp;

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FilterIndexTemp;
exit1:

	if(FilterHandleTemp!=-1) STPTI_FilterDeallocate(FilterHandleTemp);
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return -1;
}

#else

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
short CH6_Allocate_Section_Filter(sf_filter_mode_t FilterMode,app_t FilterApp,BOOLEAN MatchMode
								  ,BOOLEAN MulSection,BOOLEAN IsAllocSectionMem,partition_t* pSectMemPartition) 
								  /*yxl 2005-03-14 add paramter "partition_t* pSectMemPartition"*/
{

	ST_ErrorCode_t ErrCode;	
	STPTI_Filter_t FilterHandleTemp=-1;

	int FilterIndexTemp;
	int SizeTemp=1024;

	sf_filter_mode_t ModeTemp=FilterMode;

	/*yxl 2006-12-30 add below section*/
	STPTI_Handle_t PTIHandle=INVALID_VALUE;
	if(CH_GetPTIHandle(&PTIHandle))
	{
		STTBX_Print(("\nYxlInfo:Invalid PTI handle,quit"));
		return -1;
	}
	/*yxl 2006-12-30 add below section*/

	if(ModeTemp!=ONE_KILO_SECTION&&ModeTemp!=FOUR_KILO_SECTION) 
		return -1;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);

	FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_MODE,(void*)&ModeTemp);
	if(FilterIndexTemp==-1) 
	{
		goto exit1;
	}

	if(MatchMode) 
	{
#ifdef ST_7109
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);
#else
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE,
			&FilterHandleTemp);
#endif

	}
	else
	{
#ifndef ST_7109 /*yxl 2008-10-22 add this line*/
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE,
			&FilterHandleTemp);
#else /*yxl 2008-10-22 add below section*/
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);
#endif  /*end yxl 2008-10-22 add below section*/

	}

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	if(IsAllocSectionMem) 
	{
		if(ModeTemp==FOUR_KILO_SECTION) SizeTemp=4*SizeTemp;
#if 0 /*yxl 2005-03-14 modify this section*/
		SectionFilter[FilterIndexTemp].aucSectionData=memory_allocate(CHSysPartition,SizeTemp);
		if(SectionFilter[FilterIndexTemp].aucSectionData==NULL)
		{
			return -1;
		}
		memset(SectionFilter[FilterIndexTemp].aucSectionData,0,SizeTemp);
#else
		if(IsAllocSectionMem&&pSectMemPartition==NULL) 
		{
			goto exit1;
		}
		SectionFilter[FilterIndexTemp].aucSectionData=memory_allocate(pSectMemPartition,SizeTemp);
		if(SectionFilter[FilterIndexTemp].aucSectionData==NULL)
		{
			goto exit1;
		}
		SectionFilter[FilterIndexTemp].pPartition=pSectMemPartition;
		memset(SectionFilter[FilterIndexTemp].aucSectionData,0,SizeTemp);

#endif/*end yxl 2005-03-14 modify this section*/
	}
	else SectionFilter[FilterIndexTemp].aucSectionData=NULL;



	/* update the corresponding contents of structure SectionFilter*/ 

	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_IN_USE;
	SectionFilter[FilterIndexTemp].FilterHandle=FilterHandleTemp;
	SectionFilter[FilterIndexTemp].sfFilterMode=FilterMode;
	SectionFilter[FilterIndexTemp].pMsgQueue=NULL;
	SectionFilter[FilterIndexTemp].appfilter=FilterApp;

	SectionFilter[FilterIndexTemp].MulSection=MulSection;

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FilterIndexTemp;
exit1:

	if(FilterHandleTemp!=-1) STPTI_FilterDeallocate(FilterHandleTemp);
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return -1;
}

#endif /*end yxl 2004-10-24 modify this section for add two variable,boolean MulSection,boolean IsAllocSectionMem*/

#if 0
/*yxl 2005-03-14 add this function*/
BOOL CH6_ChangeFilter(unsigned short FilterIndex,sf_filter_mode_t FilterMode,app_t FilterApp,
					  BOOLEAN MatchMode,BOOLEAN MulSection,BOOLEAN IsAllocSectionMem)
{

	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Buffer_t BufferHandleTemp;
	STPTI_Filter_t FilterHandleTemp;
	int FilterIndexTemp;
	int SizeTemp=1024;
	int i;
	int SlotIndexTemp;
	int FilterIndexTemp=FilterIndex;
	
	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) return TRUE;

	sf_filter_mode_t ModeTemp=FilterMode;

	if(ModeTemp!=ONE_KILO_SECTION&&ModeTemp!=FOUR_KILO_SECTION)	return TRUE;
    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	SlotHandleTemp=SectionFilter[FilterIndexTemp].SlotHandle;

	ErrCode=STPTI_FilterDisassociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterDisassociate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	ErrCode=STPTI_FilterDeallocate(FilterHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterDeallocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	/*update corresponding content of structure SectionFilter*/
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_FREE;
	SectionFilter[FilterIndexTemp].FilterHandle=INVALID_VALUE;
	SectionFilter[FilterIndexTemp].SlotHandle=INVALID_VALUE;

	SectionFilter[FilterIndexTemp].FirstSearched=false;

	if(SectionFilter[FilterIndexTemp].aucSectionData!=NULL)
	{
		if(SectionFilter[FilterIndexTemp].pPartition!=NULL)
			memory_deallocate(SectionFilter[FilterIndexTemp].pPartition,SectionFilter[FilterIndexTemp].aucSectionData);
		SectionFilter[FilterIndexTemp].aucSectionData =(BYTE *) NULL ;
		SectionFilter[FilterIndexTemp].pPartition=NULL;
	}

    for(i=0;i<256;i++)
	{
		SectionFilter[FilterIndexTemp]. SectionhaveSearch[i]=false;
	}

	SectionFilter[FilterIndexTemp]. appfilter=UNKNOWN_APP;

	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_SLOTHANDLE,(void*)&SlotHandleTemp);
	if(SlotIndexTemp==-1) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);	

		return FALSE;
	}
	SectionSlot[SlotIndexTemp].FilterCount--;
	

	if(MatchMode)
	{
#ifdef  ST_7109
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);
#else
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE,
			&FilterHandleTemp);
#endif
	}
	else
        {
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE,
			&FilterHandleTemp);
        }

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	if(IsAllocSectionMem) 
	{
		if(ModeTemp==FOUR_KILO_SECTION) SizeTemp=4*SizeTemp;

		if(IsAllocSectionMem&&pSectMemPartition==NULL) 
		{
			goto exit1;
		}
		SectionFilter[FilterIndexTemp].aucSectionData=memory_allocate(pSectMemPartition,SizeTemp);
		if(SectionFilter[FilterIndexTemp].aucSectionData==NULL)
		{
			goto exit1;
		}
		SectionFilter[FilterIndexTemp].pPartition=pSectMemPartition;
		memset(SectionFilter[FilterIndexTemp].aucSectionData,0,SizeTemp);

	}
	else SectionFilter[FilterIndexTemp].aucSectionData=NULL;

	/* update the corresponding contents of structure SectionFilter*/ 

	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_IN_USE;
	SectionFilter[FilterIndexTemp].FilterHandle=FilterHandleTemp;
	SectionFilter[FilterIndexTemp].sfFilterMode=FilterMode;
	SectionFilter[FilterIndexTemp].pMsgQueue=NULL;
	SectionFilter[FilterIndexTemp].appfilter=FilterApp;

	SectionFilter[FilterIndexTemp].MulSection=MulSection;

	if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FilterIndexTemp;
exit1:

	if(FilterHandleTemp!=-1) STPTI_FilterDeallocate(FilterHandleTemp);
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return -1;

}
#endif /*end yxl 2005-03-14 add this function*/


/*end yxl 2004-11-19 add this function*/


/*Function description:
	Free a section filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be freeed
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Free_Section_Filter(unsigned short FilterIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Filter_t FilterHandleTemp;

	int i;
	int SlotIndexTemp;
	int FilterIndexTemp=FilterIndex;
	
	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) return TRUE;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	SlotHandleTemp=SectionFilter[FilterIndexTemp].SlotHandle;
	
#if 1
	ErrCode=STPTI_FilterDisassociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterDisassociate()=%s",GetErrorText(ErrCode)));
	}
#endif 

	
	ErrCode=STPTI_FilterDeallocate(FilterHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterDeallocate()=%s",GetErrorText(ErrCode)));
	}

	/*update corresponding content of structure SectionFilter*/
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_FREE;
	SectionFilter[FilterIndexTemp].FilterHandle=INVALID_VALUE;
	SectionFilter[FilterIndexTemp].SlotHandle=INVALID_VALUE;

	SectionFilter[FilterIndexTemp].FirstSearched=false;

	if(SectionFilter[FilterIndexTemp].aucSectionData!=NULL)
	{
#if 0 /*yxl 2005-03-14 modify this section*/
		memory_deallocate(CHSysPartition,SectionFilter[FilterIndexTemp].aucSectionData);
		SectionFilter[FilterIndexTemp].aucSectionData =(BYTE *) NULL ;
#else
		if(SectionFilter[FilterIndexTemp].pPartition!=NULL)
			memory_deallocate(SectionFilter[FilterIndexTemp].pPartition,SectionFilter[FilterIndexTemp].aucSectionData);
		SectionFilter[FilterIndexTemp].aucSectionData =(BYTE *) NULL ;
		SectionFilter[FilterIndexTemp].pPartition=NULL;
#endif/*end yxl 2005-03-14 modify this section*/

	}

    for(i=0;i<256;i++)
	{
		SectionFilter[FilterIndexTemp]. SectionhaveSearch[i]=false;
	}

	SectionFilter[FilterIndexTemp]. appfilter=UNKNOWN_APP;


	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_SLOTHANDLE,(void*)&SlotHandleTemp);
	if(SlotIndexTemp==-1) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);	

		return FALSE;
	}


	SectionSlot[SlotIndexTemp].FilterCount--;
	


    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FALSE;

}


/*Function description:
	Associate a slot with a filter  
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be associated
	unsigned short FilterIndex:the index of the filter which will be associated
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Associate_Slot_Filter(unsigned short SlotIndex,unsigned short FilterIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Filter_t FilterHandleTemp;

	int SlotIndexTemp=SlotIndex;
	int FilterIndexTemp=FilterIndex;
	int MaxSlotCount=MAX_NO_SLOT;
	if(SlotIndexTemp<0||SlotIndexTemp>=MaxSlotCount) 
		return TRUE;


	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) 
		return TRUE;


    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE||\
		SectionFilter[FilterIndexTemp].FilterHandle==INVALID_VALUE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	if(SectionSlot[SlotIndexTemp].SlotStatus==SECTION_SLOT_FREE||\
		SectionSlot[SlotIndexTemp].SlotHandle==INVALID_VALUE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;
	
	/*associate filter with slot*/
	ErrCode=STPTI_FilterAssociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAssociate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}


    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,1,&FilterHandleTemp,0);  
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_ModifyGlobalFilterState(A)=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	/*update the content of struct SectionFilter*/
	SectionFilter[FilterIndexTemp].SlotHandle=SlotHandleTemp;

	/*update the content of struct SectionSlot*/
	SectionSlot[SlotIndexTemp].FilterCount++;

    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}


/*yxl 2005-03-10 add this function for 爱迪得CA 接口*/

/*Function description:
	Disassociate a filter from a slot   
Parameters description: 
	unsigned short SlotIndex:the index of the slot which will be disassociated
	unsigned short FilterIndex:the index of the filter which will be disassociated
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_DisAssociate_Slot_Filter(unsigned short SlotIndex,unsigned short FilterIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Filter_t FilterHandleTemp;

	int SlotIndexTemp=SlotIndex;
	int FilterIndexTemp=FilterIndex;
	int MaxSlotCount=MAX_NO_SLOT;
	if(SlotIndexTemp<0||SlotIndexTemp>=MaxSlotCount) 
		return TRUE;

	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) 
		return TRUE;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE||\
		SectionFilter[FilterIndexTemp].FilterHandle==INVALID_VALUE) 
		goto exit1;
	

	if(SectionSlot[SlotIndexTemp].SlotStatus==SECTION_SLOT_FREE||\
		SectionSlot[SlotIndexTemp].SlotHandle==INVALID_VALUE) 
	{
	   goto exit1;
	}
	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;

	ErrCode=STPTI_FilterDisassociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("CH6_DisAssociate_Slot_Filter()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	if(SectionSlot[SlotIndexTemp].FilterCount>0)
		SectionSlot[SlotIndexTemp].FilterCount--;
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	
	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;

}

/*end yxl 2005-03-10 add this function for 爱迪得CA 接口*/




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
BOOL CH6_Set_Filter(unsigned short FilterIndex,U8* pData,U8* pMask,U8* pPatern,int DataLen,BOOLEAN MatchMode)
{
	ST_ErrorCode_t ErrCode;	


	STPTI_Filter_t FilterHandleTemp;
	STPTI_FilterData_t FilterData;

	U8 ByteTemp[16];
	U8 MaskTemp[16];
	U8 ModePaternTemp[16];

	int FilterIndexTemp=FilterIndex;

	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) 
		return TRUE;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE||\
		SectionFilter[FilterIndexTemp].FilterHandle==INVALID_VALUE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	if(MatchMode)	
	{
#ifdef ST_7109
		FilterData.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
#else
		FilterData.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE;

#endif
	}
	else 
	{
		
#ifndef ST_7109 /*yxl 2008-10-22 add this line*/
		FilterData.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE;
		
#else /*yxl 2008-10-22 add below section*/
	FilterData.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
#endif  /*end yxl 2008-10-22 add below section*/
		
	}

    FilterData.FilterRepeatMode=STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED;
    FilterData.InitialStateEnabled=true;
   
    FilterData.u.SectionFilter.DiscardOnCrcError=false;

#if 1 /*yxl 2008-10-22 modify below line,yxl 2008-1022,top is ok*/
    FilterData.u.SectionFilter.NotMatchMode=false;
#else
	FilterData.u.SectionFilter.NotMatchMode=MatchMode/*对应必须MaskTemp[4]=0x00,false 20060518 change MatchMode*/;
#endif

	FilterData.FilterBytes_p=ByteTemp;
	FilterData.FilterMasks_p=MaskTemp;
	FilterData.u.SectionFilter.ModePattern_p=ModePaternTemp;
   
	memset((void*)ByteTemp,0,16);
	memset((void*)MaskTemp,0,16);
	memset((void*)ModePaternTemp,0xff,16);/*select positive match mode*/

	memcpy((void*)ByteTemp,(const void*)pData,DataLen);
	memcpy((void*)MaskTemp,(const void*)pMask,DataLen);
	memcpy((void*)ModePaternTemp,(const void*)pPatern,DataLen);
	

#ifdef YXL_DEMUX_DEBUG
	{
		int j;
		STTBX_Print(("\n"));
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("%02x ",ByteTemp[j]));
		}
		STTBX_Print(("\n"));
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("%02x ",MaskTemp[j]));
		}
	
		STTBX_Print(("\n"));
		for(j=0;j<8;j++) 
		{
			STTBX_Print(("%02x ",ModePaternTemp[j]));
		}
	}
#endif

	/*set filter data*/
	ErrCode=STPTI_FilterSet(FilterHandleTemp,&FilterData);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterSet()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}


    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}


/*Function description:
	Enable the use of a filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be enabled
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Start_Filter(unsigned short FilterIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Filter_t FilterHandleTemp;

	int FilterIndexTemp=FilterIndex;

	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) 
		return TRUE;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE||\
		SectionFilter[FilterIndexTemp].FilterHandle==INVALID_VALUE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,0,&FilterHandleTemp,1);  

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_ModifyGlobalFilterState(B)=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	/*yxl 2004-11-08 add this line*/
	SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_IN_USE;
	/*end yxl 2004-11-08 add this line*/

    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}


/*Function description:
	Disable the use of a filter  
Parameters description: 
	unsigned short FilterIndex:the index of the filter which will be disabled
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
*/
BOOL CH6_Stop_Filter(unsigned short FilterIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Filter_t FilterHandleTemp;

	int FilterIndexTemp=FilterIndex;

	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) 
		return TRUE;

    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE||\
		SectionFilter[FilterIndexTemp].FilterHandle==INVALID_VALUE) 
	{
	    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}

	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,1,&FilterHandleTemp,0);  
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_ModifyGlobalFilterState(C)=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	/*yxl 2004-11-08 add this line*/
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_DONE;
	/*end yxl 2004-11-08 add this line*/

    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return FALSE;
exit1:
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}


void CH6_Register_Section_CallBack()
{
	
}


/*yxl 2004-10-10 add below function*/

int CH6_GetFreeFilterCount(void)
{
	int i;
	int MaxFilterCount=MAX_NO_FILTER_TOTAL;
	int Count=0;
	if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	for(i=0;i<MaxFilterCount;i++)
	{
		if(SectionFilter[i].FilterStatus==SECTION_FILTER_FREE) Count++;
	}

    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

#if 0 /*yxl 2004-10-22 modify this section*/
	return MaxFilterCount-Count;
#else
	return Count;
#endif /*end yxl 2004-10-22 modify this section*/

}


/*end yxl 2004-10-10 add below function*/


/*yxl 2007-12-26 add below function*/
CH6_FILTER_STATUS_t CH_GetFilterStatus(unsigned short FilterIndex)
{
	CH6_FILTER_STATUS_t StaTemp;
	unsigned short FilterIndexTemp=FilterIndex;


    if(IsUseSem) semaphore_wait(pSemSectStrucACcess);
	StaTemp=SectionFilter[FilterIndexTemp].FilterStatus;
    if(IsUseSem) semaphore_signal(pSemSectStrucACcess);

	return StaTemp;
	
}

/*end yxl 2007-12-26 add below function*/

/*End of file*/
