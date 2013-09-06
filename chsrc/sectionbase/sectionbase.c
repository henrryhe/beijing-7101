/*

File name   : sectionbase.c
Description : Section filter base implementation for changhong QAM5516 DBVC project
Revision    :  1.0.0
Authors:yxl
Date          Remark
2004-05-17    Created

  modify date:
  2004-11-04
  1) Optimize function CH6_FreePidReq();
 
2004-11-07
1)Modify 

  date:2004-11-08
  1.Modify function CH6_ReenableFilter(),
  1) add SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_IN_USE;
  2) modify return value from SLOT_ID to B0OL
  2.Modify function CH6_StopFilter(),
  1)add SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_DONE;
  2)add another parameter "IsUseInnerSem ",TRUE stand for use inner semephore
  date:2004-11-11
  1.modify CH6_FreeSectionReg(),add 	
	SectionFilter[FilterIndexTemp].TableId=0xff;
	SectionFilter[FilterIndexTemp].VersionNo=0xff;
	SectionFilter[FilterIndexTemp].SectionNo=0xff;
	SectionFilter[FilterIndexTemp].StreamId=0;

  date:2004-11-16 
  1.modify function CH6_SectionReq(),eliminate a big bug
  date :2004-11-20
  1.modify CH6_FreeSection(),add SectionFilter[FilterIndexTemp].sfFilterMode=ONE_KILO_SECTION,
    to keep consistent with iniliatise
  2.modify CH6_SectionReq() and CH6_PrepareFilter(),add parameter "int ProgMask",
  (mainly for HTML application for particular section filter)
   为了统一所有SECTION REQUEST，保持数据结构的完整,
	
 date:2004-11-21
  1.modify CH6_FreeSectionReg() and ,add CH6_SectionFilterInit()	
		SectionFilter[i]. MulSection=false;
		SectionFilter[i]. LastSection=0;
	It's for 保持数据结构的完整(SectionFilter,SectionSlot) 

  date:2004-11-22
  1.modify CH6_ReenableFilter(),add if(FilterHandleTemp==INVALID_VALUE) 的特殊处理，
  此为保持程序的健壮性

  date:2005-01-27
  1.Clear the bug of function CH6_PrepareFilter(),which is below
#if 0 
		if(ProgMask=-1)
#else
		if(ProgMask==-1)
#endif 

   date:2005-03-14 
  1.Add a new member "partition_t *pPartition" in struct CH6_SECTION_FILTER_t,which is used to 
  specify the memory partition of member "BYTE aucSectionData" in sectionbase.h
  2.According to new added member "partition_t *pPartition",modify below function:
  1)Modify inner of function CH6_SectionFilterInit()
  2)Modify inner of function CH6_FreeSectionReq()
   
  date:2005-03-15 
  1.Modify a bug of function CH6_PrepareFilter(),which is related to the filter mode,that's 
  filter match mode is always in equel mode becase of "pFilDataTemp->u.SectionFilter.NotMatchMode=false" is 
  always exist 因为笔误;
  


*/
#include <string.h>
#include "stddefs.h"
#include "stcommon.h"

#include "sectionbase.h"
//#include "..\dbase\eit_section.h"
#include "..\dbase\section.h"
#include "..\main\initterm.h"


//#define STTBX_PRINT
//debug info out switch 
//#define print_sb_info

CH6_SECTION_SLOT_t SectionSlot[MAX_NO_SLOT];
CH6_SECTION_FILTER_t SectionFilter[MAX_NO_FILTER_TOTAL]; 
semaphore_t* pSemSectStrucACcess=NULL;

static void CH6_PrepareFilter(STPTI_FilterData_t* pFilterData, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
							  int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,int ProgMask );

static void CH6_PrepareFilter2(STPTI_FilterData_t* pFilterData, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
							  int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,U8 tabMask );


BOOLEAN CH6_SectionFilterInit ( int MaxSlotNumber,int MaxFilterNumber  )
{
	ST_ErrorCode_t ErrCode;
	int i,j;
	int BufSizeTemp;
	
	/*initialize section slot struct*/ 
	for(i=0;i<MaxSlotNumber;i++)
	{
		SectionSlot[i].SlotStatus=SECTION_SLOT_FREE;		
		SectionSlot[i].PidValue=INVALID_VALUE;			
		SectionSlot[i].SlotHandle=INVALID_VALUE;		
		SectionSlot[i].BufferHandle=INVALID_VALUE;   
		SectionSlot[i].SignalHandle=INVALID_VALUE; 
		SectionSlot[i].FilterCount=0;	
	}
	
	/*initialize section filter struct*/ 
	for(i=0;i<MaxFilterNumber;i++)
	{
		SectionFilter[i].FilterStatus=SECTION_FILTER_FREE;
		SectionFilter[i].sfFilterMode=ONE_KILO_SECTION; 		
		SectionFilter[i].SlotHandle=INVALID_VALUE;		
		SectionFilter[i].FilterHandle=INVALID_VALUE;		
		SectionFilter[i].pMsgQueue=NULL; 
		SectionFilter[i].aucSectionData=NULL; 
		SectionFilter[i].pPartition=NULL;
		SectionFilter[i].TableId=0xff;
		SectionFilter[i].VersionNo=0xff;
		SectionFilter[i].SectionNo=0xff;
		SectionFilter[i].StreamId=0;
		
		for(j=0;j<256;j++)
		{
			SectionFilter[i]. SectionhaveSearch[j]=false;
		}
		
		SectionFilter[i]. FirstSearched=false;
		SectionFilter[i]. appfilter=UNKNOWN_APP;
		SectionFilter[i]. MulSection=false;
		SectionFilter[i]. LastSection=0;

	}
	
    pSemSectStrucACcess=semaphore_create_fifo(1);
	CH6_demux_SemaphoreInit();
	
	return FALSE;
	
}

/*SLOT_ID	CH6_SectionReq ( message_queue_t *ReturnQueueId,
sf_filter_mode_t  FilterMode, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,
boolean MulSection) yxl 2004-07-22 modify this line to next line*/
/*yxl 2004-11-20 add paramter Progmask*/
SLOT_ID	CH6_SectionReq ( message_queue_t *ReturnQueueId,
						sf_filter_mode_t  FilterMode, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
						int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,
						boolean MulSection,STPTI_Signal_t SignalHandle, app_t app_type,int ProgMask)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp=-1;
	STPTI_Buffer_t BufferHandleTemp=-1;
	STPTI_Filter_t FilterHandleTemp=-1;
	STPTI_SlotData_t SlotData;
	STPTI_FilterData_t FilterData;
	U8 BufTempByte[16];
	U8 BufTempMask[16];
	U8 BufTempPattern[16];
	int i;
	int FilterIndexTemp;
	int SlotIndexTemp;
	BOOLEAN Sign=false; /*false stand for no slot collect request pid,
	true stand for there is already slot collecting  request pid*/
	
	sf_filter_mode_t ModeTemp=FilterMode;
	CH6_SLOT_STATUS_t StatusTemp=SECTION_SLOT_FREE;
	STPTI_Pid_t PIDtemp=ReqPid;
	
    semaphore_wait(pSemSectStrucACcess);
	
	
	FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_MODE,(void*)&ModeTemp);
	if(FilterIndexTemp==-1) goto exit1;
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_PID,(void*)&PIDtemp);
	if(SlotIndexTemp==-1) 
	{
		SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_STATUS,(void*)&StatusTemp);
		if(SlotIndexTemp==-1) goto exit1;
	}
	else Sign=true;
	if(Sign==false) 
	{
		/*allocate slot object*/
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
			do_report(0,"STPTI_SlotAllocate()=%s",GetErrorText(ErrCode));
			goto exit1;
			
		}
		
		/*allocate buffer object*/
		
		ErrCode=STPTI_BufferAllocate(PTIHandle,SECTION_CIRCULAR_BUFFER_SIZE,1,&BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			do_report(0,"STPTI_BufferAllocate()=%s",GetErrorText(ErrCode));
			goto exit1;
		}
		
		/*associate slot with buffer*/
		ErrCode=STPTI_SlotLinkToBuffer(SlotHandleTemp,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			do_report(0,"STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode));
			goto exit1;
		}
		/*STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));*/

		
		/*associate signal with buffer object*/
		ErrCode=STPTI_SignalAssociateBuffer(SignalHandle,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			do_report(0,"STPTI_SignalAssociateBuffer()=%s",GetErrorText(ErrCode));
			goto exit1;
		}
		
	}
	else
	{
		SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;
		STPTI_SlotClearPid(SlotHandleTemp);
	}
	
	
/*zxg20051215 del*/	if(iNotEqualIndex!=3)
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp); 
	else
		ErrCode=STPTI_FilterAllocate(PTIHandle, /*STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE*/STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);

	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterAllocate()=%s",GetErrorText(ErrCode));
		goto exit1;
	}

	
	FilterData.FilterBytes_p=BufTempByte;
	FilterData.FilterMasks_p=BufTempMask;
	FilterData.u.SectionFilter.ModePattern_p=BufTempPattern;
	
	CH6_PrepareFilter(&FilterData,ReqPid,ReqTableId,ReqProgNo,ReqSectionNo,ReqVersionNo ,
		iNotEqualIndex,ProgMask);
	

	ErrCode=STPTI_FilterSet(FilterHandleTemp,&FilterData);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterSet()=%s",GetErrorText(ErrCode));
		goto exit1;
	}
	

	ErrCode=STPTI_FilterAssociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterAssociate()=%s",GetErrorText(ErrCode));
		goto exit1;
	}	

	ErrCode=STPTI_SlotSetPid(SlotHandleTemp,ReqPid);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_SlotSetPid()=%s",GetErrorText(ErrCode));
		goto exit1;
	}
	
	/* update the corresponding contents of structure SectionSlot and SectionFilter*/ 
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_IN_USE;
	SectionSlot[SlotIndexTemp].SlotHandle=SlotHandleTemp;
	if(Sign==false)
		SectionSlot[SlotIndexTemp].BufferHandle=BufferHandleTemp;
	
	SectionSlot[SlotIndexTemp].PidValue=ReqPid;
	SectionSlot[SlotIndexTemp].FilterCount++;
	SectionSlot[SlotIndexTemp].SignalHandle=SignalHandle;
	
    SectionFilter[FilterIndexTemp].MulSection=MulSection;
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_IN_USE;
	SectionFilter[FilterIndexTemp].FilterHandle=FilterHandleTemp;
	SectionFilter[FilterIndexTemp].SlotHandle=SlotHandleTemp;
	SectionFilter[FilterIndexTemp].sfFilterMode=FilterMode;
	SectionFilter[FilterIndexTemp].pMsgQueue=ReturnQueueId;
	SectionFilter[FilterIndexTemp].SectionNo=ReqSectionNo;
	SectionFilter[FilterIndexTemp].StreamId=ReqProgNo;
	SectionFilter[FilterIndexTemp].TableId=ReqTableId;
	SectionFilter[FilterIndexTemp].VersionNo=ReqVersionNo;
	SectionFilter[FilterIndexTemp]. appfilter= app_type;
	
	semaphore_signal(pSemSectStrucACcess);
	
	return FilterIndexTemp;
exit1:
	if(Sign==false)
	{
		if(SlotHandleTemp!=-1) STPTI_SlotDeallocate(SlotHandleTemp);
		if(BufferHandleTemp!=-1) STPTI_BufferDeallocate(BufferHandleTemp);
	}
	if(FilterHandleTemp!=-1) STPTI_FilterDeallocate(FilterHandleTemp);
    semaphore_signal(pSemSectStrucACcess);
	return -1;
	
}

SLOT_ID	CH6_SectionReq2 ( message_queue_t *ReturnQueueId,
						sf_filter_mode_t  FilterMode, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
						int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,
						boolean MulSection,STPTI_Signal_t SignalHandle, app_t app_type,U8 TabMask)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp=-1;
	STPTI_Buffer_t BufferHandleTemp=-1;
	STPTI_Filter_t FilterHandleTemp=-1;
	STPTI_SlotData_t SlotData;
	STPTI_FilterData_t FilterData;
	U8 BufTempByte[16];
	U8 BufTempMask[16];
	U8 BufTempPattern[16];
	int i;
	int FilterIndexTemp;
	int SlotIndexTemp;
	BOOLEAN Sign=false; /*false stand for no slot collect request pid,
	true stand for there is already slot collecting  request pid*/
	
	sf_filter_mode_t ModeTemp=FilterMode;
	CH6_SLOT_STATUS_t StatusTemp=SECTION_SLOT_FREE;
	STPTI_Pid_t PIDtemp=ReqPid;
	
    semaphore_wait(pSemSectStrucACcess);
	
	
	FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_MODE,(void*)&ModeTemp);
	if(FilterIndexTemp==-1) goto exit1;
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_PID,(void*)&PIDtemp);
	if(SlotIndexTemp==-1) 
	{
		SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_STATUS,(void*)&StatusTemp);
		if(SlotIndexTemp==-1) goto exit1;
	}
	else Sign=true;
	
	if(Sign==false) 
	{
		/*allocate slot object*/
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
			do_report(0,"STPTI_SlotAllocate()=%s",GetErrorText(ErrCode));
			goto exit1;
			
		}
		
		/*allocate buffer object*/
		
		ErrCode=STPTI_BufferAllocate(PTIHandle,SECTION_CIRCULAR_BUFFER_SIZE,1,&BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			do_report(0,"STPTI_BufferAllocate()=%s",GetErrorText(ErrCode));
			goto exit1;
		}
		
		/*associate slot with buffer*/
		ErrCode=STPTI_SlotLinkToBuffer(SlotHandleTemp,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			do_report(0,"STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode));
			goto exit1;
		}
		/*STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));*/

		
		/*associate signal with buffer object*/
		ErrCode=STPTI_SignalAssociateBuffer(SignalHandle,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			do_report(0,"STPTI_SignalAssociateBuffer()=%s",GetErrorText(ErrCode));
			goto exit1;
		}
		
	}
	else
	{
		SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;
		STPTI_SlotClearPid(SlotHandleTemp);
	}
	
	
	/* zxg20051215 del */if(iNotEqualIndex!=3)
		ErrCode=STPTI_FilterAllocate(PTIHandle, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp); 
	else
		ErrCode=STPTI_FilterAllocate(PTIHandle, /*STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE*/STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	
	FilterData.FilterBytes_p=BufTempByte;
	FilterData.FilterMasks_p=BufTempMask;
	FilterData.u.SectionFilter.ModePattern_p=BufTempPattern;
	
	CH6_PrepareFilter2(&FilterData,ReqPid,ReqTableId,ReqProgNo,ReqSectionNo,ReqVersionNo ,
		iNotEqualIndex,TabMask);
	

	ErrCode=STPTI_FilterSet(FilterHandleTemp,&FilterData);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterSet()=%s",GetErrorText(ErrCode));
		goto exit1;
	}
	

	ErrCode=STPTI_FilterAssociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterAssociate()=%s",GetErrorText(ErrCode));
		goto exit1;
	}	

	ErrCode=STPTI_SlotSetPid(SlotHandleTemp,ReqPid);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_SlotSetPid()=%s",GetErrorText(ErrCode));
		goto exit1;
	}
	
	/* update the corresponding contents of structure SectionSlot and SectionFilter*/ 
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_IN_USE;
	SectionSlot[SlotIndexTemp].SlotHandle=SlotHandleTemp;
	if(Sign==false)
		SectionSlot[SlotIndexTemp].BufferHandle=BufferHandleTemp;
	
	SectionSlot[SlotIndexTemp].PidValue=ReqPid;
	SectionSlot[SlotIndexTemp].FilterCount++;
	SectionSlot[SlotIndexTemp].SignalHandle=SignalHandle;
	
    SectionFilter[FilterIndexTemp].MulSection=MulSection;
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_IN_USE;
	SectionFilter[FilterIndexTemp].FilterHandle=FilterHandleTemp;
	SectionFilter[FilterIndexTemp].SlotHandle=SlotHandleTemp;
	SectionFilter[FilterIndexTemp].sfFilterMode=FilterMode;
	SectionFilter[FilterIndexTemp].pMsgQueue=ReturnQueueId;
	SectionFilter[FilterIndexTemp].SectionNo=ReqSectionNo;
	SectionFilter[FilterIndexTemp].StreamId=ReqProgNo;
	SectionFilter[FilterIndexTemp].TableId=ReqTableId;
	SectionFilter[FilterIndexTemp].VersionNo=ReqVersionNo;
	SectionFilter[FilterIndexTemp]. appfilter= app_type;
	
	semaphore_signal(pSemSectStrucACcess);
	
	return FilterIndexTemp;
exit1:
	if(Sign==false)
	{
		if(SlotHandleTemp!=-1) STPTI_SlotDeallocate(SlotHandleTemp);
		if(BufferHandleTemp!=-1) STPTI_BufferDeallocate(BufferHandleTemp);
	}
	if(FilterHandleTemp!=-1) STPTI_FilterDeallocate(FilterHandleTemp);
    semaphore_signal(pSemSectStrucACcess);
	return -1;
	
}


/*使某个FIRLTER重新传输数据*/
BOOL CH6_ReenableFilter (SLOT_ID FilterIndex)
{

	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Filter_t FilterHandleTemp;
	
    if ( FilterIndex < 0 || FilterIndex >= MAX_NO_FILTER_TOTAL )
	{
		return TRUE;
	}
	
    semaphore_wait(pSemSectStrucACcess);
	
	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;

	if(FilterHandleTemp==INVALID_VALUE)
	{
		do_report(0,"\nYxlInfo:Invalid filter index ");
		goto exit1;
	}

    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,0,&FilterHandleTemp,1);  
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_ModifyGlobalFilterState(AA)=%s",GetErrorText(ErrCode));
		goto exit1;
	}
	
    SectionFilter [FilterIndex ].FilterStatus=SECTION_FILTER_IN_USE;
	semaphore_signal(pSemSectStrucACcess);


	return FALSE;
exit1:
	semaphore_signal(pSemSectStrucACcess);
	return TRUE;


}


/*return value:YXL 2004-07-15 add this description
FALSE:stand for success
TRUE: stand for fail

*/

BOOLEAN CH6_FreeSectionReq (SLOT_ID FilterIndex)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Buffer_t BufferHandleTemp;
	STPTI_Filter_t FilterHandleTemp;
	STPTI_Signal_t SignalHandleTemp;
	
	int i;
	int SlotIndexTemp;
	int FilterIndexTemp=FilterIndex;
	
	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) return TRUE;

    semaphore_wait(pSemSectStrucACcess);

	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE) 
	{
		semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}
	
	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	SlotHandleTemp=SectionFilter[FilterIndexTemp].SlotHandle;


        /*20070130 add*/
         ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,1,&FilterHandleTemp,0);  
         if(ErrCode!=ST_NO_ERROR)
	  {
		do_report(0,"in CH6_FreeSectionReq STPTI_ModifyGlobalFilterState()=%s",GetErrorText(ErrCode));
	   }
	  /**************/

	
	ErrCode=STPTI_FilterDisassociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterDisassociate()=%s",GetErrorText(ErrCode));
	}

	
	ErrCode=STPTI_FilterDeallocate(FilterHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_FilterDeallocate()=%s",GetErrorText(ErrCode));
	}
	
	/*update corresponding content of structure SectionFilter*/
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_FREE;
	SectionFilter[FilterIndexTemp].FilterHandle=INVALID_VALUE;
	SectionFilter[FilterIndexTemp].SlotHandle=INVALID_VALUE;
	SectionFilter[FilterIndexTemp].FirstSearched=false;
	if(SectionFilter[FilterIndexTemp].aucSectionData!=NULL)
	{

		if(SectionFilter[FilterIndexTemp].pPartition!=NULL)
			memory_deallocate(SectionFilter[FilterIndexTemp].pPartition,
				SectionFilter[FilterIndexTemp].aucSectionData);	
		SectionFilter[FilterIndexTemp].aucSectionData =(BYTE *) NULL ;
		SectionFilter[FilterIndexTemp].pPartition=NULL;


	}
	
    for(i=0;i<256;i++)
	{
		SectionFilter[FilterIndexTemp]. SectionhaveSearch[i]=false;
	}
	
	SectionFilter[FilterIndexTemp]. appfilter=UNKNOWN_APP;/*yxl 2004-07-15 add this line*/
	SectionFilter[FilterIndexTemp].pMsgQueue=NULL;/*yxl 2004-11-06 add this line*/	
	SectionFilter[FilterIndexTemp].sfFilterMode=ONE_KILO_SECTION; /*yxl 2004-11-20 add this line*/	


	SectionFilter[FilterIndexTemp].TableId=0xff;
	SectionFilter[FilterIndexTemp].VersionNo=0xff;
	SectionFilter[FilterIndexTemp].SectionNo=0xff;
	SectionFilter[FilterIndexTemp].StreamId=0;

	SectionFilter[FilterIndexTemp]. MulSection=false;
	SectionFilter[FilterIndexTemp]. LastSection=0;
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_SLOTHANDLE,(void*)&SlotHandleTemp);
	if(SlotIndexTemp==-1) 
	{
		semaphore_signal(pSemSectStrucACcess);	
		STTBX_Print(("\n YxlInfo(%s,%d L):FreeSectionReq C",__FILE__,__LINE__));
		return TRUE;
	}
	
	SectionSlot[SlotIndexTemp].FilterCount--;
	
	if(SectionSlot[SlotIndexTemp].FilterCount>0) 
		
	{
		semaphore_signal(pSemSectStrucACcess);
		/*do_report(0,"\n YxlInfo(%s,%d L):FreeSectionReq B\n",__FILE__,__LINE__);*/
		return FALSE;
	}
	
	
	/*disassociate buffer with signal*/	
	BufferHandleTemp=SectionSlot[SlotIndexTemp].BufferHandle;
	SignalHandleTemp=SectionSlot[SlotIndexTemp].SignalHandle;

       /*20070130 add*/
      ErrCode=STPTI_SlotClearPid(SectionSlot[SlotIndexTemp].SlotHandle);
      if(ErrCode)
      	{
           do_report(0,"CH6_FreeSectionReq STPTI_SlotClearPid()=%s",GetErrorText(ErrCode));
      	}
     /*************/
	
	ErrCode=STPTI_SignalDisassociateBuffer(SignalHandleTemp,BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_SignalDisassociateBuffer()=%s",GetErrorText(ErrCode));
	}
	
	/*deallocate buffer object*/
	ErrCode=STPTI_BufferDeallocate(BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_BufferDeallocate()=%s",GetErrorText(ErrCode));
	}
	
	
	/*deallocate slot */
	ErrCode=STPTI_SlotDeallocate(SectionSlot[SlotIndexTemp].SlotHandle);
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_SlotDeallocate()=%s",GetErrorText(ErrCode));
	}
	
	/*update corresponding contents of structure SectionSlot*/	
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_FREE;	
	SectionSlot[SlotIndexTemp].PidValue=INVALID_VALUE;			
	SectionSlot[SlotIndexTemp].SlotHandle=INVALID_VALUE;		
	SectionSlot[SlotIndexTemp].BufferHandle=INVALID_VALUE;   
	SectionSlot[SlotIndexTemp].SignalHandle=INVALID_VALUE; 
	SectionSlot[SlotIndexTemp].FilterCount=0;	
	
    semaphore_signal(pSemSectStrucACcess);
	return FALSE;
	
}


/*return value: YXL 2004-07-15 add this description
FALSE:stand for success
TRUE: stand for fail
  */
BOOLEAN CH6_FreeAllSectionReq (app_t AppType )
{
	int i;
	BOOLEAN res;
	app_t AppTypeTemp;

	AppTypeTemp=AppType;
	do_report(0,"\n YxlInfo(%s,%d L):FreeAllSectionReq\n",__FILE__,__LINE__);
	if(AppTypeTemp==UNKNOWN_APP) 
	{
		for(i=0;i<MAX_NO_FILTER_TOTAL;i++)
		{
			res=CH6_FreeSectionReq (i );
		}
		
	}
	else
	{

		for(i=0;i<MAX_NO_FILTER_TOTAL;i++)
		{
		
		  semaphore_wait(pSemSectStrucACcess);
		  if(SectionFilter[i].appfilter!=AppTypeTemp) 
		  {
			  semaphore_signal(pSemSectStrucACcess);			
			  continue;
		  }
		  semaphore_signal(pSemSectStrucACcess);

		  res=CH6_FreeSectionReq (i );
		}
	}
	return res;
	  
}


BOOLEAN CH6_FreeSlot(CH6_SLOT_MATCH_MODE MatchMode,void* PMatchValue )
{
	  int SlotIndexTemp=-1;
	  
	  BOOLEAN res;
	  
	  void* pTemp=PMatchValue;
	  semaphore_wait(pSemSectStrucACcess);
	  SlotIndexTemp=CH6_FindMatchSlot(MatchMode,(void*)pTemp);
	  if(SlotIndexTemp==-1) 
		  goto exit1;
	  
	  semaphore_signal(pSemSectStrucACcess);
	  
	  CH6_Stop_Slot(SlotIndexTemp);
	  CH6_Free_Section_Slot(SlotIndexTemp);
	  
	  return FALSE;
exit1:
	  semaphore_signal(pSemSectStrucACcess);
	  return TRUE;
  }
  
  
int CH6_FindMatchSlot(CH6_SLOT_MATCH_MODE MatchMode,void* PMatchValue)
  {
	  int i;
	  int MaxSlotCount=MAX_NO_SLOT;
	  void* pTemp=PMatchValue;

/*	  STTBX_Print(("\n YxlInfo:value=%d",(*(STPTI_Pid_t*)pTemp)));*/
	  
	  switch(MatchMode)
	  {
	  case SLOT_MATCH_BY_SLOTHANDLE:
		  for(i=0;i<MaxSlotCount;i++)
		  {
			  if(SectionSlot[i].SlotHandle==(*(STPTI_Slot_t*)pTemp)) return i;
		  }
		  return -1;
	  case SLOT_MATCH_BY_STATUS:
		  for(i=0;i<MaxSlotCount;i++)
		  {
			  if(SectionSlot[i].SlotStatus==(*(CH6_SLOT_STATUS_t*)pTemp)) return i;
		  }
		  return -1;
	  case SLOT_MATCH_BY_PID:
		  for(i=0;i<MaxSlotCount;i++)
		  {
			 /* STTBX_Print(("\n YxlInfo:i=%d pid=%d %lx",i, SectionSlot[i].PidValue,
				  (*(STPTI_Pid_t*)pTemp)));
			  */
			  if(SectionSlot[i].PidValue==(*(STPTI_Pid_t*)pTemp)) 
			  {
				/*STTBX_Print(("\n YxlInfo:i=%d pid=%d %d",i, SectionSlot[i].PidValue,
				  (*(STPTI_Pid_t*)pTemp)));
				  */
			  			
				  return i;
			  }
		  }
		  return -1;
	  default:
		  break;
	  }
	  return -1;
  }
  
  
  
int CH6_FindMatchFilter(CH6_FILTER_MATCH_MODE MatchMode,void* PMatchValue)
  {
	  int i;
	  int MaxFilterCount=MAX_NO_FILTER_TOTAL;
	  void* pTemp=PMatchValue;
	  sf_filter_mode_t ModeTemp;
	  switch(MatchMode)
	  {
	  case FILTER_MATCH_BY_MODE:	
		  ModeTemp=(*(sf_filter_mode_t*)pTemp);
#if 0 /*yxl 2004-07-19 modify this section*/
		  if(ModeTemp==ONE_KILO_SECTION)
		  {
			  for(i=0;i<MAX_NO_FILTER_1KMODE;i++)
			  {
				  if(SectionFilter[i].FilterStatus==SECTION_SLOT_FREE) return i;
			  }
			  
			  return -1;
		  }
		  else if(ModeTemp==FOUR_KILO_SECTION)
		  {
			  for(i=MAX_NO_FILTER_1KMODE;i<MAX_NO_FILTER_TOTAL;i++)
			  {
				  if(SectionFilter[i].FilterStatus==SECTION_SLOT_FREE) return i;
			  }
			  return -1;		
		  }
#else
		  for(i=0;i<MaxFilterCount;i++)
		  {
			  if(SectionFilter[i].FilterStatus==SECTION_FILTER_FREE) return i;
		  }
		  do_report(0,"no free filter\n");
#if 0/*20070205 del*/
		    for(i=0;i<MaxFilterCount;i++)
		  {
			 do_report(0,"Table ID[%d]=%d,type=%d\n",i,SectionFilter[i].TableId,SectionFilter[i].appfilter);
		  }
#endif
		  return -1;
#endif/*end yxl 2004-07-19 modify this section*/
		  
		  
	  case FILTER_MATCH_BY_FILTERHANDLE:
		  for(i=0;i<MaxFilterCount;i++)
		  {
			  if(SectionFilter[i].FilterHandle==(*(STPTI_Filter_t*)pTemp)) return i;
		  }
		  return -1;
		  
#if 1 /*yxl 2004-07-19 add this section*/
	  case FILTER_MATCH_BY_STATUS:
		  for(i=0;i<MaxFilterCount;i++)
		  {
			  if(SectionFilter[i].FilterStatus==(*(CH6_FILTER_STATUS_t*)pTemp)) return i;
		  }
		  return -1;
		  
#endif /*end yxl 2004-07-19 add this section*/
		  
	  default:
		  break;
	  }
	  return -1;
  }


BOOL CH6_StopFilter(SLOT_ID FilterIndex,BOOL IsUseInnerSem )
{

	ST_ErrorCode_t ErrCode;
	STPTI_Filter_t FilterHandleTemp;
    if ( FilterIndex < 0 || FilterIndex >= MAX_NO_FILTER_TOTAL )
	{
		return TRUE;
	}

   if(IsUseInnerSem) semaphore_wait(pSemSectStrucACcess);	


	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
	
    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,1,&FilterHandleTemp,0);  
	if(ErrCode!=ST_NO_ERROR)
	{
		do_report(0,"STPTI_ModifyGlobalFilterState(CC)=%s",GetErrorText(ErrCode));
		goto exit1;
	}
	SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_DONE;

	if(IsUseInnerSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;
exit1:
	if(IsUseInnerSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}



/*zxg 20040711 add following*/
SHORT CH_GetSlotIndex(SLOT_ID FilterIndex)
{
	
	STPTI_Slot_t SlotHandleTemp;
	int SlotIndexTemp;
	int FilterIndexTemp;
	
	FilterIndexTemp=FilterIndex;
	
	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL)
		return -1;
	
    semaphore_wait(pSemSectStrucACcess);
	
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE) 
	{
		semaphore_signal(pSemSectStrucACcess);
		return -1;
	}
	
	
	SlotHandleTemp=SectionFilter[FilterIndexTemp].SlotHandle;	
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_SLOTHANDLE,(void*)&SlotHandleTemp);
	
    semaphore_signal(pSemSectStrucACcess);
	
	return SlotIndexTemp;
}

int CH6_FindePid(int PID)
{
	STPTI_Pid_t PIDtemp=PID;
    SHORT SlotIndexTemp;
	
	semaphore_wait(pSemSectStrucACcess);
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_PID,(void*)&PIDtemp);
	
	semaphore_signal(pSemSectStrucACcess);	
	
	return SlotIndexTemp;
	
}


BOOLEAN CH6_FreePidReq ( int FreePID )
{
	ST_ErrorCode_t ErrCode;	
	int i;
	int SlotIndexTemp;
	
	STPTI_Pid_t PIDtemp=FreePID;
	int MaxFilterCount=MAX_NO_FILTER_TOTAL;
	
/*    STTBX_Print(("\n YxlInfo:in CH6_FreePidReq"));*/

    semaphore_wait(pSemSectStrucACcess);
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_PID,(void*)&PIDtemp);
	
	if(SlotIndexTemp==-1) 
	{
		semaphore_signal(pSemSectStrucACcess);	
		return TRUE;
	}	
	semaphore_signal(pSemSectStrucACcess);	
	for(i=0;i<MaxFilterCount;i++)
	{
#if 0 /*yxl 2004-11-04 modify this section*/
		if(SectionFilter[i].SlotHandle==SectionSlot[SlotIndexTemp].SlotHandle)
			CH6_FreeSectionReq(i);
#else
		if(SectionFilter[i].SlotHandle==SectionSlot[SlotIndexTemp].SlotHandle\
			&&SectionFilter[i].SlotHandle!=INVALID_VALUE)
			CH6_FreeSectionReq(i);
#endif/*end yxl 2004-11-04 modify this section*/
	}
	
	return FALSE;
}

/*==============================
*/

/*yxl 2004-11-20 add parameter  int ProgMask,main for html*/
static void CH6_PrepareFilter(STPTI_FilterData_t* pFilterData, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
							  int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,int ProgMask )
{
	U8 ByteTemp[16];
	U8 MaskTemp[16];
	U8 ModePaternTemp[8];
	int i;
	
	STPTI_FilterData_t* pFilDataTemp=pFilterData;
	/* zxg20051215 del */if(iNotEqualIndex==3)
		pFilDataTemp->FilterType=/*STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE*/STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
	/*zxg20051215 del */else
		pFilDataTemp->FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;


    pFilDataTemp->FilterRepeatMode=STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED;
    pFilDataTemp->InitialStateEnabled=true;
	

    pFilDataTemp->u.SectionFilter.DiscardOnCrcError=false;
    if(iNotEqualIndex==3)
		pFilDataTemp->u.SectionFilter.NotMatchMode=true;
	else
		pFilDataTemp->u.SectionFilter.NotMatchMode=false;

	memset((void*)ByteTemp,0,16);
	memset((void*)MaskTemp,0,16);
	memset((void*)ModePaternTemp,0xff,8);/*select positive match mode*/
	
	/*set the first byte,that's TableId Section*/
	MaskTemp[0]=0xff;
	ByteTemp[0]=ReqTableId;
	
	/*set the second and third byte,that's program number section,etc*/
	if(ReqProgNo!=-1&&ReqProgNo!=0xffff)
	{
#if 0 /*yxl 2005-01-27 modify this section*/
		if(ProgMask=-1)
#else
		if(ProgMask==-1)
#endif /*end yxl 2005-01-27 modify this section*/
		{
			MaskTemp[1]=0xff;
			MaskTemp[2]=0xff;
		}
		else
		{
			MaskTemp[1]=(U8)(ProgMask/256);
			MaskTemp[2]=(U8)(ProgMask%256);
		}

		ByteTemp[1]=(U8)(ReqProgNo/256);
		ByteTemp[2]=(U8)(ReqProgNo%256);
		
	}
	
	/*set the fourth byte,that's version section*/
	if(ReqVersionNo!=-1)
	{
	MaskTemp[3]=	0x00;/*0x3e; 20060518 change for 5105 not match mode*/
		ByteTemp[3]=ReqVersionNo<<1;
		if(iNotEqualIndex==3)
			ModePaternTemp[3]=0x00;

/*		STTBX_Print(("\n ByteTemp[3]=%x ReqVersionNo=%x",ByteTemp[3],ReqVersionNo));*/

	}
	
	/*set the fifth byte,that's section_number*/
	if(ReqSectionNo!=-1)
	{
		MaskTemp[4]=0xff;
		ByteTemp[4]=ReqSectionNo;
	}
	
	memcpy((void*)pFilDataTemp->FilterBytes_p,(const void*)ByteTemp,16);
	memcpy((void*)pFilDataTemp->FilterMasks_p,(const void*)MaskTemp,16);
	memcpy((void*)pFilDataTemp->u.SectionFilter.ModePattern_p,(const void*)ModePaternTemp,8);
	
#ifdef print_sb_info
	{
		int j;
		STTBX_Print(("\n"));
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("Byte[]:=%02x ",ByteTemp[j]));
		}
		STTBX_Print(("\n"));
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("Mask[]:=%02x ",MaskTemp[j]));
		}
		STTBX_Print(("\n"));
		
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("pate[]:=%02x ",ModePaternTemp[j]));
		}
	}
#endif
	return;	

}

static void CH6_PrepareFilter2(STPTI_FilterData_t* pFilterData, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
							  int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,U8 tabMask )
{
	U8 ByteTemp[16];
	U8 MaskTemp[16];
	U8 ModePaternTemp[8];
	int i;
	
	STPTI_FilterData_t* pFilDataTemp=pFilterData;
/*zxg20051215 del*/	if(iNotEqualIndex==3) 
		pFilDataTemp->FilterType=/*STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE*/STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
/*zxg20051215 del*/	else
		pFilDataTemp->FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;


    pFilDataTemp->FilterRepeatMode=STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED;
    pFilDataTemp->InitialStateEnabled=true;
	

    pFilDataTemp->u.SectionFilter.DiscardOnCrcError=false;
    if(iNotEqualIndex==3)
		pFilDataTemp->u.SectionFilter.NotMatchMode=true;
	else
		pFilDataTemp->u.SectionFilter.NotMatchMode=false;

	memset((void*)ByteTemp,0,16);
	memset((void*)MaskTemp,0,16);
	memset((void*)ModePaternTemp,0xff,8);/*select positive match mode*/
	
	/*set the first byte,that's TableId Section*/
	MaskTemp[0]=(U8)tabMask;
	ByteTemp[0]=ReqTableId;
	
	/*set the second and third byte,that's program number section,etc*/
	if(ReqProgNo!=-1&&ReqProgNo!=0xffff)
	{
#if 0 /*yxl 2005-01-27 modify this section*/
		if(ProgMask=-1)
#else
		if(1/*ProgMask==-1*/)
#endif /*end yxl 2005-01-27 modify this section*/
		{
			MaskTemp[1]=0xff;
			MaskTemp[2]=0xff;
		}
		else
		{
//			MaskTemp[1]=(U8)(ProgMask/256);
//			MaskTemp[2]=(U8)(ProgMask%256);
		}

		ByteTemp[1]=(U8)(ReqProgNo/256);
		ByteTemp[2]=(U8)(ReqProgNo%256);
		
	}
	
	/*set the fourth byte,that's version section*/
	if(ReqVersionNo!=-1)
	{
	MaskTemp[3]=	0x00;/*0x3e; 20060518 change for 5105 not match mode*/
		ByteTemp[3]=ReqVersionNo<<1;
		if(iNotEqualIndex==3) ModePaternTemp[3]=0x00;

/*		STTBX_Print(("\n ByteTemp[3]=%x ReqVersionNo=%x",ByteTemp[3],ReqVersionNo));*/

	}
	
	/*set the fifth byte,that's section_number*/
	if(ReqSectionNo!=-1)
	{
		MaskTemp[4]=0xff;
		ByteTemp[4]=ReqSectionNo;
	}
	
	memcpy((void*)pFilDataTemp->FilterBytes_p,(const void*)ByteTemp,16);
	memcpy((void*)pFilDataTemp->FilterMasks_p,(const void*)MaskTemp,16);
	memcpy((void*)pFilDataTemp->u.SectionFilter.ModePattern_p,(const void*)ModePaternTemp,8);
	
#ifdef print_sb_info
	{
		int j;
		STTBX_Print(("\n"));
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("Byte[]:=%02x ",ByteTemp[j]));
		}
		STTBX_Print(("\n"));
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("Mask[]:=%02x ",MaskTemp[j]));
		}
		STTBX_Print(("\n"));
		
		for(j=0;j<16;j++) 
		{
			STTBX_Print(("pate[]:=%02x ",ModePaternTemp[j]));
		}
	}
#endif
	return;	

}


void CH_SetPTIHandle(STPTI_Handle_t ch_PTIHandle)
{
     return;
}