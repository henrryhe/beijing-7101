/*

File name   : sectionbase.c
Description : Section filter base implementation
Revision    :  1.0.0
created date :2004-10-10  by yxl for changhong QAM5516 DBVC project

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

   date:2005-03-14 by Yixiaoli 
  1.Add a new member "partition_t *pPartition" in struct CH6_SECTION_FILTER_t,which is used to 
  specify the memory partition of member "BYTE aucSectionData" in sectionbase.h
  2.According to new added member "partition_t *pPartition",modify below function:
  1)Modify inner of function CH6_SectionFilterInit()
  2)Modify inner of function CH6_FreeSectionReq()
   
  date:2005-03-15 by Yixiaoli
  1.Modify a bug of function CH6_PrepareFilter(),which is related to the filter mode,that's 
  filter match mode is always in equel mode becase of "pFilDataTemp->u.SectionFilter.NotMatchMode=false" is 
  always exist 因为笔误;


  Date:2005-06-07,2005-07-26 by Yixiaoli
  1)Add macro FOR_CH_5100_7710_USE,定义为5510和7710芯片,不定义为QAMI5516芯片

  Date:2005-08-07 by Yixiaoli
   1) Add function CH6_SectionReq_ByData(),由调用者确定filter的三种数据，主要
   用于HTML的需要

  Date:2006-12-25 by Yixiaoli
  1)Delete macro FOR_CH_5100_7710_USE 及函数中的相关部分,因为5100和7710的处理是完全一样的。


Date:2008-10-22 by Yixiaoli
1.Modify function CH6_SectionReq(),CH6_SectionReq_ByData(),对不等模式作了更改，但未测试。
2.Modify function CH6_FreeSectionReq().
	 
*/
#include <string.h>
#include "stddefs.h"
#include "stcommon.h"

#include "sectionbase.h"
/*#include "..\dbase\eit_section.h"*/
#include "..\dbase\section.h"
/*#include "..\main\initterm.h"*/
#include "..\main\errors.h"
#include "chdemux.h"
#include "sttbx.h"


/*#define SECTIONBASE_DEBUG  yxl 2006-12-25 add this macro*/


CH6_SECTION_SLOT_t SectionSlot[MAX_NO_SLOT];
CH6_SECTION_FILTER_t SectionFilter[MAX_NO_FILTER_TOTAL];

semaphore_t* pSemSectStrucACcess=NULL;

/*yxl 2005-07-26 add this section*/
#define FILTER_EQ_MODE 1
#define FILTER_NEQ_MODE 2

static STPTI_Handle_t PTIEQHandle=INVALID_VALUE;
static STPTI_Handle_t PTINEQHandle=INVALID_VALUE;

BOOL CH_SetPTIHandle(STPTI_Handle_t Handle)
{
#if 0 /*yxl 2005-08-18 modify this section*/
	switch(Type)
	{
	case FILTER_EQ_MODE:
		PTIEQHandle=Handle;
		return FALSE;
	case FILTER_NEQ_MODE:
		PTINEQHandle=Handle;
		return FALSE;
	default:
		return TRUE;

	}
#else
		PTIEQHandle=Handle;
		return FALSE;
#endif /*end yxl 2005-08-18 modify this section*/

}

BOOL CH_GetPTIHandle(STPTI_Handle_t* pHandle)
{

#if 0 /*yxl 2005-08-18 modify this section*/
	switch(Type)
	{
	case FILTER_EQ_MODE:
		if(PTIEQHandle!=INVALID_VALUE)
		{
			*pHandle=PTIEQHandle;
			return FALSE;
		}
		else return TRUE;

	case FILTER_NEQ_MODE:
		if(PTINEQHandle!=INVALID_VALUE)
		{
			*pHandle=PTINEQHandle;
			return FALSE;
		}
		else return TRUE;
	default:
		return TRUE;
	}
#else
		if(PTIEQHandle!=INVALID_VALUE)
		{
			*pHandle=PTIEQHandle;
			return FALSE;
		}
		else return TRUE;
#endif/*yxl 2005-08-18 modify this section*/

}
/*end yxl 2005-07-26 add this section*/


BOOLEAN CH6_SectionFilterInit ( int MaxSlotNumber,int MaxFilterNumber  )
{
	int i,j;
	
	/*initialize section slot struct*/ 
	for(i=0;i<MaxSlotNumber;i++)
	{
		SectionSlot[i].SlotStatus=SECTION_SLOT_FREE;
		
		SectionSlot[i].PidValue=INVALID_VALUE;			
		SectionSlot[i].SlotHandle=INVALID_VALUE;		
		SectionSlot[i].BufferHandle=INVALID_VALUE;   
		SectionSlot[i].SignalHandle=INVALID_VALUE; 	/*yxl 2004-07-22 add this line*/
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
		SectionFilter[i].pPartition=NULL;/*yxl 2005-03-14 add this line*/
		SectionFilter[i].TableId=0xff;
		SectionFilter[i].VersionNo=0xff;
		SectionFilter[i].SectionNo=0xff;
		SectionFilter[i].StreamId=0;
		
		for(j=0;j<256;j++)
		{
			SectionFilter[i]. SectionhaveSearch[j]=false;
		}
		SectionFilter[i]. FirstSearched=false;
		SectionFilter[i]. appfilter=UNKNOWN_APP;/*yxl 2004-07-15 add this line*/
#if 1
		SectionFilter[i]. MulSection=false;/*yxl 2004-11-21 add this line*/
		SectionFilter[i]. LastSection=0;/*yxl 2004-11-21 add this line*/
#endif
	}
	
    pSemSectStrucACcess=semaphore_create_fifo(1);

	
	return FALSE;

}



/*yxl 2004-11-20 add parameter  int ProgMask,main for html*/
void CH6_PrepareFilter(STPTI_FilterData_t* pFilterData, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
							  int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,int ProgMask )
{
	U8 ByteTemp[16];
	U8 MaskTemp[16];
	U8 ModePaternTemp[8];
#ifdef SECTIONBASE_DEBUG 

	int i;
#endif	
	STPTI_FilterData_t* pFilDataTemp=pFilterData;
	if(iNotEqualIndex==3)
	{
		pFilDataTemp->FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE;
	}
	else
	{
#ifdef ST_7109 /*yxl 2007-06-13 modify below section*/
		pFilDataTemp->FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
#else
		pFilDataTemp->FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE;

#endif/*end yxl 2007-06-13 modify below section*/
	}

    pFilDataTemp->FilterRepeatMode=STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED;
    pFilDataTemp->InitialStateEnabled=true;
	

    pFilDataTemp->u.SectionFilter.DiscardOnCrcError=false;
    if(iNotEqualIndex==3)
		pFilDataTemp->u.SectionFilter.NotMatchMode=true;
	else
		pFilDataTemp->u.SectionFilter.NotMatchMode=false;

	pFilDataTemp->u.SectionFilter.OverrideSSIBit    = FALSE;/*yxl 2006-12-27 add this line*/

	/*	pFilDataTemp->u.SectionFilter.NotMatchMode=false;yxl 2005-03-14 cancel this line*/
	
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
		MaskTemp[3]=0x3e;
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
	
#ifdef SECTIONBASE_DEBUG 
	{
		int j;
		STTBX_Print(("\nByte[]:= "));
		for(j=0;j<16;j++) 
		{
			STTBX_Print((" %02x",ByteTemp[j]));
		}
		STTBX_Print(("\nMask[]:="));
		for(j=0;j<16;j++) 
		{
			STTBX_Print((" %02x",MaskTemp[j]));
		}
		STTBX_Print(("\npate[]:="));
		
		for(j=0;j<16;j++) 
		{
			STTBX_Print((" %02x ",ModePaternTemp[j]));
		}
	}
#endif

	
	return;	
}





#if 1
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
	STPTI_Handle_t PTIHandleTemp;/*yxl 2005-07-26 add this line*/
	U8 BufTempByte[16];
	U8 BufTempMask[16];
	U8 BufTempPattern[16];

	int FilterIndexTemp;
	int SlotIndexTemp;
	BOOLEAN Sign=false; /*false stand for no slot collect request pid,
	true stand for there is already slot collecting  request pid*/
	
	sf_filter_mode_t ModeTemp=FilterMode;
	CH6_SLOT_STATUS_t StatusTemp=SECTION_SLOT_FREE;
	STPTI_Pid_t PIDtemp=ReqPid;
	
    semaphore_wait(pSemSectStrucACcess);

#if 0 /*yxl 2005-08-18 modify this section*/

	/* yxl 2005-07-26 add this section*/
	if(iNotEqualIndex==3)
	{
		if(CH_GetPTIHandle(&PTIHandleTemp,2))
		{
			semaphore_signal(pSemSectStrucACcess);			
			return -1;
		}
	}
	else
	{
		if(CH_GetPTIHandle(&PTIHandleTemp,1))
		{
			semaphore_signal(pSemSectStrucACcess);			
			return -1;
		}	
	}
	/*end  yxl 2005-07-26 add this section*/

#else
		if(CH_GetPTIHandle(&PTIHandleTemp))
		{
			semaphore_signal(pSemSectStrucACcess);			
			return -1;
		}
#endif/*end yxl 2005-08-18 modify this section*/

	FilterIndexTemp=CH6_FindMatchFilter(FILTER_MATCH_BY_MODE,(void*)&ModeTemp);
	if(FilterIndexTemp==-1) goto exit1;/*return -1;*/
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_PID,(void*)&PIDtemp);
	if(SlotIndexTemp==-1) 
	{
		SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_STATUS,(void*)&StatusTemp);
		if(SlotIndexTemp==-1) goto exit1;/*return -1;*/
	}
	else Sign=true;
	
	if(Sign==false) 
	{
		/*allocate slot object*/
		SlotData.SlotType=STPTI_SLOT_TYPE_SECTION;
		/*SlotData.SlotType=STPTI_SLOT_TYPE_RAW;*/
		SlotData.SlotFlags.SignalOnEveryTransportPacket=false;
		SlotData.SlotFlags.CollectForAlternateOutputOnly=false;
		SlotData.SlotFlags.AlternateOutputInjectCarouselPacket=false;
		SlotData.SlotFlags.StoreLastTSHeader=false;
		SlotData.SlotFlags.InsertSequenceError=false;
		SlotData.SlotFlags.OutPesWithoutMetadata=false;
		SlotData.SlotFlags.ForcePesLengthToZero=false;
		SlotData.SlotFlags.AppendSyncBytePrefixToRawData=false;
		ErrCode=STPTI_SlotAllocate(PTIHandleTemp,&SlotHandleTemp,&SlotData);
		if(ErrCode!=ST_NO_ERROR)
		{
		/*	STTBX_Print(("STPTI_SlotAllocate()=%s",GetErrorText(ErrCode)));*/
			goto exit1;
			
		}
		
		/*allocate buffer object*/
		ErrCode=STPTI_BufferAllocate(PTIHandleTemp,SECTION_CIRCULAR_BUFFER_SIZE,1,&BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferAllocate()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
		

		/*associate slot with buffer*/
		ErrCode=STPTI_SlotLinkToBuffer(SlotHandleTemp,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
		/*STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));*/

		
		/*associate signal with buffer object*/
#if 0 /*yxl 2004-07-22 modify this section*/
		ErrCode=STPTI_SignalAssociateBuffer(gSignalHandle,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SignalAssociateBuffer()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
#else
		ErrCode=STPTI_SignalAssociateBuffer(SignalHandle,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SignalAssociateBuffer()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
#endif /*end yxl 2004-07-22 modify this section*/
		
	}
	else
	{
		SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;
		STPTI_SlotClearPid(SlotHandleTemp);
	}
	
	if(iNotEqualIndex!=3)/* match mode*/
	{
#ifdef ST_7109 /*yxl 2007-06-13 modify below section*/
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp); 
#else
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE,
			&FilterHandleTemp); 
#endif
	}
	else
	{
#ifndef ST_7109 /*yxl 2008-10-22 add this line*/
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE,
			&FilterHandleTemp);
#else /*yxl 2008-10-22 add below section*/
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);

#endif  /*end yxl 2008-10-22 add below section*/

	}

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAllocate()=%s",GetErrorText(ErrCode)));
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
		STTBX_Print(("STPTI_FilterSet()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	/*else STTBX_Print(("STPTI_FilterSet()=%s",GetErrorText(ErrCode)));*/
	

	ErrCode=STPTI_FilterAssociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAssociate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	/*else STTBX_Print(("STPTI_FilterAssociate()=%s",GetErrorText(ErrCode)));*/
	
	

	ErrCode=STPTI_SlotSetPid(SlotHandleTemp,ReqPid);

	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotSetPid()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	/*else 	STTBX_Print(("STPTI_SlotSetPid()=%s",GetErrorText(ErrCode)));*/
	
	/* update the corresponding contents of structure SectionSlot and SectionFilter*/ 
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_IN_USE;
	SectionSlot[SlotIndexTemp].SlotHandle=SlotHandleTemp;
	if(Sign==false) /*yxl 2004-11-16 add this line*/
		SectionSlot[SlotIndexTemp].BufferHandle=BufferHandleTemp;
	
	SectionSlot[SlotIndexTemp].PidValue=ReqPid;
	SectionSlot[SlotIndexTemp].FilterCount++;
	SectionSlot[SlotIndexTemp].SignalHandle=SignalHandle;/*yxl 2004-07-22 add this line*/
	
	/*STTBX_Print(("A out of FilterIndexTemp=%d",FilterIndexTemp));*/
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
	SectionFilter[FilterIndexTemp]. appfilter= app_type;/*yxl 2004-07-15 add this line*/

	semaphore_signal(pSemSectStrucACcess);
	
	/*STTBX_Print(("out of FilterIndexTemp=%d",FilterIndexTemp));*/
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
#endif 



#if 1 /*yxl 2005-08-07 add this function for Html section request*/ 

SLOT_ID	CH6_SectionReq_ByData( message_queue_t *ReturnQueueId,sf_filter_mode_t  FilterMode,
						 STPTI_Pid_t ReqPid,int iNotEqualIndex,	boolean MulSection,
						 STPTI_Signal_t SignalHandle, app_t app_type,
						 U8 FilterDataCount,U8*FilterData,
						 U8 FilterMaskCount,U8*FilterMask,
						 U8 FilterPaternCount,U8*FilterModePatern)
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp=-1;
	STPTI_Buffer_t BufferHandleTemp=-1;
	STPTI_Filter_t FilterHandleTemp=-1;
	STPTI_SlotData_t SlotData;
	STPTI_FilterData_t FilterDataTemp;
	STPTI_Handle_t PTIHandleTemp;/*yxl 2005-07-26 add this line*/
	U8 BufTempByte[16];
	U8 BufTempMask[16];
	U8 BufTempPattern[16];
	int FilterIndexTemp;
	int SlotIndexTemp;
	BOOLEAN Sign=false; /*false stand for no slot collect request pid,
	true stand for there is already slot collecting  request pid*/
	
	sf_filter_mode_t ModeTemp=FilterMode;
	CH6_SLOT_STATUS_t StatusTemp=SECTION_SLOT_FREE;
	STPTI_Pid_t PIDtemp=ReqPid;
	
	if(FilterDataCount>16||FilterDataCount==0\
		||FilterMaskCount>16||FilterMaskCount==0)
	{
		STTBX_Print(("\nYxlInfo:Invalid filter data or mask count "));
		return -1;
	}
	if(FilterDataCount!=FilterMaskCount)
	{
		STTBX_Print(("\nYxlInfo:filter data count not equeal filter mask count"));
		return -1;
	}
	if(FilterPaternCount>8)
	{
		STTBX_Print(("\nYxlInfo:Invalid filter mode patern count "));
		return -1;
	}
    semaphore_wait(pSemSectStrucACcess);
#if 0 /*yxl 2005-08-18 modify this section*/

	/* yxl 2005-07-26 add this section*/
	if(iNotEqualIndex==3)
	{
		if(CH_GetPTIHandle(&PTIHandleTemp,2))
		{
			semaphore_signal(pSemSectStrucACcess);			
			return -1;
		}
	}
	else
	{
		if(CH_GetPTIHandle(&PTIHandleTemp,1))
		{
			semaphore_signal(pSemSectStrucACcess);			
			return -1;
		}	
	}
	/*end  yxl 2005-07-26 add this section*/

#else
		if(CH_GetPTIHandle(&PTIHandleTemp))
		{
			semaphore_signal(pSemSectStrucACcess);			
			return -1;
		}
#endif/*end yxl 2005-08-18 modify this section*/

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
		ErrCode=STPTI_SlotAllocate(PTIHandleTemp,&SlotHandleTemp,&SlotData);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotAllocate()=%s",GetErrorText(ErrCode)));
			goto exit1;
			
		}
		
		/*allocate buffer object*/
		
		ErrCode=STPTI_BufferAllocate(PTIHandleTemp,SECTION_CIRCULAR_BUFFER_SIZE,1,&BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_BufferAllocate()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
		

		/*associate slot with buffer*/
		ErrCode=STPTI_SlotLinkToBuffer(SlotHandleTemp,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SlotLinkToBuffer()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}
		
		/*associate signal with buffer object*/
		ErrCode=STPTI_SignalAssociateBuffer(SignalHandle,BufferHandleTemp);
		if(ErrCode!=ST_NO_ERROR)
		{
			STTBX_Print(("STPTI_SignalAssociateBuffer()=%s",GetErrorText(ErrCode)));
			goto exit1;
		}

		
	}
	else
	{
		SlotHandleTemp=SectionSlot[SlotIndexTemp].SlotHandle;
		STPTI_SlotClearPid(SlotHandleTemp);
	}
	
	
	if(iNotEqualIndex!=3)
	{
#ifdef ST_7109 /*yxl 2007-06-13 modify below section*/
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp); 
#else
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE,
			&FilterHandleTemp); 
#endif
	}
	else
	{


#ifndef ST_7109 /*yxl 2008-10-22 add this line*/
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE,
			&FilterHandleTemp);
#else /*yxl 2008-10-22 add below section*/
		ErrCode=STPTI_FilterAllocate(PTIHandleTemp, STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
			&FilterHandleTemp);

#endif  /*end yxl 2008-10-22 add below section*/


	}
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAllocate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	
	if(iNotEqualIndex==3)
	{
#ifndef ST_7109 /*yxl 2008-10-22 add this line*/
		FilterDataTemp.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE;
#else /*yxl 2008-10-22 add below section*/
		FilterDataTemp.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
#endif  /*end yxl 2008-10-22 add below section*/

	}
	else
	{
#ifdef ST_7109 /*yxl 2007-06-13 modify below section*/
		FilterDataTemp.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE;
#else
		FilterDataTemp.FilterType=STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE;
#endif
	}
	
    FilterDataTemp.FilterRepeatMode=STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED;
    FilterDataTemp.InitialStateEnabled=true;
	
	
    FilterDataTemp.u.SectionFilter.DiscardOnCrcError=false;
    if(iNotEqualIndex==3)
		FilterDataTemp.u.SectionFilter.NotMatchMode=true;
	else
		FilterDataTemp.u.SectionFilter.NotMatchMode=false;
	
	memset((void*)BufTempByte,0,16);
	memset((void*)BufTempMask,0,16);
	memset((void*)BufTempPattern,0xff,8);
	memcpy((void*)BufTempByte,(const void*)FilterData,FilterDataCount);
	memcpy((void*)BufTempMask,(const void*)FilterMask,FilterMaskCount);
	memcpy((void*)BufTempPattern,(const void*)FilterModePatern,FilterPaternCount);



	FilterDataTemp.FilterBytes_p=BufTempByte;
	FilterDataTemp.FilterMasks_p=BufTempMask;
	FilterDataTemp.u.SectionFilter.ModePattern_p=BufTempPattern;

	ErrCode=STPTI_FilterSet(FilterHandleTemp,&FilterDataTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterSet()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}

	ErrCode=STPTI_FilterAssociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterAssociate()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	
	

	ErrCode=STPTI_SlotSetPid(SlotHandleTemp,ReqPid);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SlotSetPid()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	
	/* update the corresponding contents of structure SectionSlot and SectionFilter*/ 
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_IN_USE;
	SectionSlot[SlotIndexTemp].SlotHandle=SlotHandleTemp;
	if(Sign==false) /*yxl 2004-11-16 add this line*/
		SectionSlot[SlotIndexTemp].BufferHandle=BufferHandleTemp;
	
	SectionSlot[SlotIndexTemp].PidValue=ReqPid;
	SectionSlot[SlotIndexTemp].FilterCount++;
	SectionSlot[SlotIndexTemp].SignalHandle=SignalHandle;/*yxl 2004-07-22 add this line*/
	
	/*STTBX_Print(("A out of FilterIndexTemp=%d",FilterIndexTemp));*/
    SectionFilter[FilterIndexTemp].MulSection=MulSection;
	SectionFilter[FilterIndexTemp].FilterStatus=SECTION_FILTER_IN_USE;
	SectionFilter[FilterIndexTemp].FilterHandle=FilterHandleTemp;
	SectionFilter[FilterIndexTemp].SlotHandle=SlotHandleTemp;
	SectionFilter[FilterIndexTemp].sfFilterMode=FilterMode;
	SectionFilter[FilterIndexTemp].pMsgQueue=ReturnQueueId;
#if 0
	SectionFilter[FilterIndexTemp].SectionNo=ReqSectionNo;
	SectionFilter[FilterIndexTemp].StreamId=ReqProgNo;
	SectionFilter[FilterIndexTemp].TableId=ReqTableId;
	SectionFilter[FilterIndexTemp].VersionNo=ReqVersionNo;
#else
	SectionFilter[FilterIndexTemp].TableId=BufTempByte[0];
#endif
	SectionFilter[FilterIndexTemp]. appfilter= app_type;/*yxl 2004-07-15 add this line*/
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
#endif /*yxl 2005-08-07 add this function for Html section request*/ 

#if 0 /*yxl 2004-11-08 modify this function*/ 

/*使某个FIRLTER重新传输数据*/
SLOT_ID	CH6_ReenableFilter (SLOT_ID FilterIndex)
{
#if 0/*yxl 2004-11-08 modify this section*/
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Buffer_t BufferHandleTemp;
	STPTI_Filter_t FilterHandleTemp;
	
    if ( FilterIndex < 0 || FilterIndex >= MAX_NO_FILTER_TOTAL )
	{
		return -1;
	}
	
    semaphore_wait(pSemSectStrucACcess);
	
    SectionFilter [FilterIndex ].FilterStatus=SECTION_FILTER_IN_USE;
	
	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,0,&FilterHandleTemp,1);  
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_ModifyGlobalFilterState()=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	
	semaphore_signal(pSemSectStrucACcess);


	return FilterIndex;
exit1:
	semaphore_signal(pSemSectStrucACcess);
	return -1;

#else
	if(CH6_Start_Filter(FilterIndex)==TRUE)
		return -1;
	else	return FilterIndex;
	
	
#endif/*end yxl 2004-11-08 modify this section*/
}

#else

/*使某个FIRLTER重新传输数据*/
BOOL CH6_ReenableFilter (SLOT_ID FilterIndex)
{

	ST_ErrorCode_t ErrCode;	
	STPTI_Filter_t FilterHandleTemp;
	
    if ( FilterIndex < 0 || FilterIndex >= MAX_NO_FILTER_TOTAL )
	{
		return TRUE;
	}
	
    semaphore_wait(pSemSectStrucACcess);
	
	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
#if 1 /*yxl 2004-11-22 add this section*/
	if(FilterHandleTemp==INVALID_VALUE)
	{
		STTBX_Print(("\nYxlInfo:Invalid filter index "));
		goto exit1;
	}
#endif/*end yxl 2004-11-22 add this section*/
    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,0,&FilterHandleTemp,1);  
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_ModifyGlobalFilterState(AA)=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	
    SectionFilter [FilterIndex ].FilterStatus=SECTION_FILTER_IN_USE;
	semaphore_signal(pSemSectStrucACcess);


	return FALSE;
exit1:
	semaphore_signal(pSemSectStrucACcess);
	return TRUE;


}


#endif /*end yxl 2004-11-08 modify this function*/ 


/*return value:YXL 2004-07-15 add this description
FALSE:stand for success
TRUE: stand for fail

*/
#if 0 /*yxl 2004-10-10 modify this sectionj*/
BOOLEAN CH6_FreeSectionReq (SLOT_ID FilterIndex,STPTI_Signal_t SignalHandle)
#else
BOOLEAN CH6_FreeSectionReq (SLOT_ID FilterIndex)
#endif /*end yxl 2004-10-10 modify this sectionj*/
{
	ST_ErrorCode_t ErrCode;	
	STPTI_Slot_t SlotHandleTemp;
	STPTI_Buffer_t BufferHandleTemp;
	STPTI_Filter_t FilterHandleTemp;
	STPTI_Signal_t SignalHandleTemp;/*yxl 2004-10-10 add this variable*/
	
	int i;
	int SlotIndexTemp;
	int FilterIndexTemp=FilterIndex;
	
	if(FilterIndexTemp<0||FilterIndexTemp>=MAX_NO_FILTER_TOTAL) return TRUE;
/*	STTBX_Print(("\n Yxl___1"));*/
    semaphore_wait(pSemSectStrucACcess);
/*	STTBX_Print(("\n Yxl___2"));*/
	if(SectionFilter[FilterIndexTemp].FilterStatus==SECTION_FILTER_FREE) 
	{
		semaphore_signal(pSemSectStrucACcess);
		return TRUE;
	}
	
	FilterHandleTemp=SectionFilter[FilterIndexTemp].FilterHandle;
	SlotHandleTemp=SectionFilter[FilterIndexTemp].SlotHandle;
	

        /*yxl 2008-10-22 add below section*/
         ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,1,&FilterHandleTemp,0);  
         if(ErrCode!=ST_NO_ERROR)
	  {
		STTBX_Print(("STPTI_ModifyGlobalFilterState()=%s",GetErrorText(ErrCode)));
	   }
        /*end yxl 2008-10-22 add below section*/

	

#if 1 /*yxl 2004-07-03 cancel this section,2004-07-15 reeable it*/
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
			memory_deallocate(SectionFilter[FilterIndexTemp].pPartition,
				SectionFilter[FilterIndexTemp].aucSectionData);	
		SectionFilter[FilterIndexTemp].aucSectionData =(BYTE *) NULL ;
		SectionFilter[FilterIndexTemp].pPartition=NULL;
#endif/*end yxl 2005-03-14 modify this section*/

	}
	
    for(i=0;i<256;i++)
	{
		SectionFilter[FilterIndexTemp]. SectionhaveSearch[i]=false;
	}
	
	SectionFilter[FilterIndexTemp]. appfilter=UNKNOWN_APP;/*yxl 2004-07-15 add this line*/
	SectionFilter[FilterIndexTemp].pMsgQueue=NULL;/*yxl 2004-11-06 add this line*/	
	SectionFilter[FilterIndexTemp].sfFilterMode=ONE_KILO_SECTION; /*yxl 2004-11-20 add this line*/	

/*yxl 2004-11-10 add this line*/	
	SectionFilter[FilterIndexTemp].TableId=0xff;
	SectionFilter[FilterIndexTemp].VersionNo=0xff;
	SectionFilter[FilterIndexTemp].SectionNo=0xff;
	SectionFilter[FilterIndexTemp].StreamId=0;
/*end yxl 2004-11-10 add this line*/

#if 1
/*yxl 2004-11-21 add this line*/
	SectionFilter[FilterIndexTemp]. MulSection=false;
	SectionFilter[FilterIndexTemp]. LastSection=0;
/*end yxl 2004-11-21 add this line*/
#endif
	
	SlotIndexTemp=CH6_FindMatchSlot(SLOT_MATCH_BY_SLOTHANDLE,(void*)&SlotHandleTemp);
	if(SlotIndexTemp==-1) 
	{
		semaphore_signal(pSemSectStrucACcess);	
		STTBX_Print(("\n YxlInfo(%s,%d L):FreeSectionReq C",__FILE__,__LINE__));
		return TRUE;
	}
	
	SectionSlot[SlotIndexTemp].FilterCount--;
	
	if(SectionSlot[SlotIndexTemp].FilterCount>0) /*yxl 2004-06-30 modify >=0 to >0*/
		
	{
		semaphore_signal(pSemSectStrucACcess);
	/*	STTBX_Print(("\n YxlInfo(%s,%d L):FreeSectionReq B",__FILE__,__LINE__));*/
		return FALSE;
	}
	
	
	/*disassociate buffer with signal*/
	
	
	BufferHandleTemp=SectionSlot[SlotIndexTemp].BufferHandle;
	SignalHandleTemp=SectionSlot[SlotIndexTemp].SignalHandle;/*yxl 2004-10-10 add this variable*/
	
	ErrCode=STPTI_SignalDisassociateBuffer(SignalHandleTemp,BufferHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_SignalDisassociateBuffer()=%s",GetErrorText(ErrCode)));
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
		STTBX_Print(("STPTI_SlotDeallocate()=%s",GetErrorText(ErrCode)));
	}
	
	/*update corresponding contents of structure SectionSlot*/
	
	SectionSlot[SlotIndexTemp].SlotStatus=SECTION_SLOT_FREE;
	
	SectionSlot[SlotIndexTemp].PidValue=INVALID_VALUE;			
	SectionSlot[SlotIndexTemp].SlotHandle=INVALID_VALUE;		
	SectionSlot[SlotIndexTemp].BufferHandle=INVALID_VALUE;   
	SectionSlot[SlotIndexTemp].SignalHandle=INVALID_VALUE; /*yxl 2004-07-22 add this line*/
	SectionSlot[SlotIndexTemp].FilterCount=0;	
	
    semaphore_signal(pSemSectStrucACcess);
/*	do_report(0,"\n YxlInfo(%s,%d L):FreeSectionReq A",__FILE__,__LINE__);*/
	return FALSE;
}


#if 1 /*yxl 2004-10-10 modify this section*/
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
	STTBX_Print(("\n YxlInfo(%s,%d L):FreeAllSectionReq",__FILE__,__LINE__));
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
#else
BOOLEAN CH6_FreeAllSectionReq (void )
{
	  int i;
	  BOOLEAN res;
	  
/*	  STTBX_Print(("\n YxlInfo(%s,%d L):FreeAllSectionReq",__FILE__,__LINE__));*/
	  for(i=0;i<MAX_NO_FILTER_TOTAL;i++)
	  {
#if 1 /*yxl 2004-07-15 add this section*/
		  semaphore_wait(pSemSectStrucACcess);
		  if(SectionFilter[i].appfilter!=NORMAL_APP) 
		  {
			  semaphore_signal(pSemSectStrucACcess);			
			  continue;
		  }
		  semaphore_signal(pSemSectStrucACcess);
#endif /*end yxl 2004-07-15 add this section*/
		  res=CH6_FreeSectionReq (i );
	  }
	  return res;
	  
}

#endif /*end yxl 2004-10-10 modify this section*/


  
#if 1 /*yxl 2004-07-22 add this function*/
  
BOOLEAN CH6_FreeSlot(CH6_SLOT_MATCH_MODE MatchMode,void* PMatchValue )
{
	  int SlotIndexTemp=-1;
	  

	  
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
  
#endif/*end yxl 2004-07-22 add this function*/
  
  
int CH6_FindMatchSlot(CH6_SLOT_MATCH_MODE MatchMode,void* PMatchValue)
  {
	  int i;
	  int MaxSlotCount=MAX_NO_SLOT;
	  void* pTemp=PMatchValue;

	  
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


#if 0 /*yxl 2004-11-08 modify this function*/
BOOL CH6_StopFilter(SLOT_ID FilterIndex)
{
#if 0 /*yxl 2004-07-15 cancel this section*/
	ST_ErrorCode_t ErrCode;
	STPTI_Filter_t FilterHandleTemp;
	STPTI_Slot_t SlotHandleTemp;
	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
	SlotHandleTemp=SectionFilter[FilterIndex].SlotHandle;
	
	ErrCode=STPTI_FilterDisassociate(FilterHandleTemp,SlotHandleTemp);
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_FilterDisassociate()=%s",GetErrorText(ErrCode)));
		return TRUE;
	}
#else /*yxl 2004-07-15 add this section*/
	ST_ErrorCode_t ErrCode;
	STPTI_Filter_t FilterHandleTemp;
	FilterHandleTemp=SectionFilter[FilterIndex].FilterHandle;
	
    ErrCode = STPTI_ModifyGlobalFilterState(&FilterHandleTemp,1,&FilterHandleTemp,0);  
	if(ErrCode!=ST_NO_ERROR)
	{
		STTBX_Print(("STPTI_ModifyGlobalFilterState(BB)=%s",GetErrorText(ErrCode)));
		return TRUE;
	}


#endif/*end yxl 2004-07-15 add this section*/
	return FALSE;
} 

#else/*yxl 2004-11-08 modify this function*/

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
		STTBX_Print(("STPTI_ModifyGlobalFilterState(CC)=%s",GetErrorText(ErrCode)));
		goto exit1;
	}
	SectionFilter[FilterIndex].FilterStatus=SECTION_FILTER_DONE;

	if(IsUseInnerSem) semaphore_signal(pSemSectStrucACcess);
	return FALSE;
exit1:
	if(IsUseInnerSem) semaphore_signal(pSemSectStrucACcess);
	return TRUE;
}
#endif /*end yxl 2004-11-08 modify this function*/


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


/*End of file*/ 