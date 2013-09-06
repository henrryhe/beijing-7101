/*
  Name:sectionbase.h
  Description:header file of Section  filter  base implementation for changhong QAM5516 DBVC platform
  Authors:yxl
  Date          Remark
  2004-10-10    Created  Revision    :  1.0.0
*/

#include "stpti.h"
#include "appltype.h"

#ifndef __SECTIONBASE__
#define __SECTIONBASE__

#if 1
#define MAX_NO_SLOT 48 /*32,yxl 2004-09-02 modify 32->48*/
#define MAX_NO_FILTER_TOTAL 32
#define MAX_NO_FILTER_1KMODE 22
#define MAX_NO_FILTER_4KMODE MAX_NO_FILTER_TOTAL-MAX_NO_FILTER_1KMODE
#endif

#define  ONE_KILO_SECTION_MAX_LENGTH       1152
#define SECTION_CIRCULAR_BUFFER_SIZE      ( ONE_KILO_SECTION_MAX_LENGTH * 8/*20060627change*/ )/*4*/

#define INVALID_VALUE -1

//#define   USE_NOT_MATCH_MODE
#if defined(USE_NOT_MATCH_MODE) 
#define	CH_FILTER_TYPE				STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE
#define	CH_NEG_FILTER_TYPE		STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE
#else
#define	CH_FILTER_TYPE				STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE
#define	CH_NEG_FILTER_TYPE		STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE
#endif

typedef	enum
{
	SECTION_SLOT_FREE,					/* no section filter req for this slot */
	SECTION_SLOT_IN_USE				/* slot is	in progress */
} CH6_SLOT_STATUS_t;

typedef	enum
{
	UNDIFINED=0,
	ONE_KILO_SECTION=1024,	/* 1KB long section */
	FOUR_KILO_SECTION=4096	/* 4KB long section */
} sf_filter_mode_t;

typedef enum
{
	SLOT_MATCH_BY_SLOTHANDLE,
	SLOT_MATCH_BY_STATUS,
	SLOT_MATCH_BY_PID
}CH6_SLOT_MATCH_MODE;

typedef enum
{
	FILTER_MATCH_BY_FILTERHANDLE,
	FILTER_MATCH_BY_STATUS,
	FILTER_MATCH_BY_MODE
}CH6_FILTER_MATCH_MODE;


typedef	struct CH6_SECTION_SLOT_s
{
	/*sf_slot_status_t	SlotStatus;*/
	CH6_SLOT_STATUS_t	SlotStatus;
	/*sf_filter_mode_t	Mode;  */
	STPTI_Pid_t			PidValue;			
	STPTI_Slot_t		SlotHandle;		
	STPTI_Buffer_t		BufferHandle;
	STPTI_Signal_t		SignalHandle;/*yxl 2004-07-22 add this line*/
	U8					FilterCount;/*associate filter count*/                                       
}CH6_SECTION_SLOT_t;

/*yxl 2004-07-15 add this section*/
typedef enum
{
	UNKNOWN_APP=0,
	NORMAL_APP,
	EPG_APP,
	NVOD_APP,
    HTML_APP,
    MOSAIC_APP,
	CA_APP, /*yxl 2004-07-19 add this line for test*/
	STOCK_APP,
	BAT_APP
	,CA_DMUX_APP
	,IR_DFA_APP
	,VOD_APP
	,BOOTPIC_APP
#ifdef SOFT_MONITTOR_CDTSMT	
       ,CDTSMT_APP	
 #endif
 /*20060404 add TDT APP*/
,TDT_APP
 /**********************/
}app_t;
/*end yxl 2004-07-15 add this section*/

typedef	struct CH6_SECTION_FILTER_s
{
	/*sf_slot_status_t	SlotStatus;*/
	CH6_FILTER_STATUS_t FilterStatus;
	sf_filter_mode_t	sfFilterMode; 
	
	STPTI_Slot_t		SlotHandle;		
	STPTI_Filter_t	    FilterHandle;
  
	message_queue_t	*pMsgQueue; 
	

	/*
	unsigned char	FilterData [ FILTER_DEPTH_SIZE ];   
	unsigned char	FilterMask [ FILTER_DEPTH_SIZE ];
	*/
	/*unsigned char	LinearBuffer [ONE_KILO_SECTION_MAX_LENGTH];*/
	unsigned char*	pLinearBuffer; 
/*	semaphore_t* pSemFilterBuf; yxl 2004-07-15 cancel this member*/

	/*unsigned char	*SectionData;*/

	BYTE              *aucSectionData;  /*该数据BUFFER动态分配*/
	partition_t       *pPartition;       /*yxl 2005-03-14 add this member,指数据BUFFER的分配的内存分区*/
	BYTE              bBufferLock ;   /*  yxl 2004-11-21 cancel this memeber */
    boolean          MulSection;           /*是否多SECTION搜索*/
    boolean           FirstSearched;           /*是否搜到第一个SECTION数据*/
	SHORT             LastSection;             /**当前FILTER数据的SECTION个数*/
	boolean           SectionhaveSearch[256];  /*各个SECTION数据是否搜到状态*/

	unsigned char	TableId;
	unsigned char VersionNo;
	unsigned char SectionNo;
	unsigned short StreamId;
	app_t            appfilter;/*yxl 2004-07-15 add this member*/
                                     
}CH6_SECTION_FILTER_t;


#if 1
extern CH6_SECTION_SLOT_t SectionSlot[MAX_NO_SLOT];
extern CH6_SECTION_FILTER_t SectionFilter[MAX_NO_FILTER_TOTAL];
#endif

extern semaphore_t* pSemSectStrucACcess;


extern int CH6_FindMatchSlot(CH6_SLOT_MATCH_MODE MatchMode,void* PMatchValue);
extern int CH6_FindMatchFilter(CH6_FILTER_MATCH_MODE MatchMode,void* PMatchValue);


#if 1
/*Function description:
	Confirm whether a pid is being captured  
Parameters description: 
	int PID:the pid to be confirmed
return value:
	-1:the pid isn't captured 
	other value:the pid is being capturing and the value stand for which slot captured this pid  
	
*/
extern SLOT_ID	CH6_SectionReq ( message_queue_t *ReturnQueueId,
                     sf_filter_mode_t  FilterMode, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
                     int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,
					 boolean MulSection,STPTI_Signal_t SignalHandle,app_t app_type,int ProgMask);

#endif

extern SLOT_ID	CH6_SectionReq2 ( message_queue_t *ReturnQueueId,
						sf_filter_mode_t  FilterMode, STPTI_Pid_t ReqPid,unsigned char ReqTableId,
						int ReqProgNo,int ReqSectionNo, int ReqVersionNo,int iNotEqualIndex,
						boolean MulSection,STPTI_Signal_t SignalHandle, app_t app_type,U8 TabMask);
/*Function description:
	Free a section request according to appointed filter index 
Parameters description: 
	SLOT_ID FilterIndex:the filter index  to be freeed
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  		
*/
extern BOOLEAN CH6_FreeSectionReq ( SLOT_ID FilterIndex);


/*Function description:
	Free some section request according to pointed filter application type 
Parameters description: 
	app_t:the filter application type  to be freeed,UNKNOWN_APP type stand for free all section request
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  		
*/
extern BOOLEAN CH6_FreeAllSectionReq (app_t AppType );


#if 0  /*yxl 2004-11-08 modify this function*/
/*Function description:
	Reenable a section request  according to appointed filter index 
Parameters description: 
	SLOT_ID FilterIndex:the filter index  to be reenabled
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  	
*/
extern SLOT_ID	CH6_ReenableFilter (SLOT_ID FilterIndex);

#else
/*Function description:
	Reenable a section request  according to appointed filter index 
Parameters description: 
	SLOT_ID FilterIndex:the filter index  to be reenabled
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  	
*/
extern BOOL	CH6_ReenableFilter (SLOT_ID FilterIndex);


#endif /*end yxl 2004-11-08 modify this function*/


/*Function description:
	Find whether a slot is capturing the section request  according to appointed filter index 
Parameters description: 
	SLOT_ID FilterIndex:the filter index  to be found
return value:
	-1:stand for no slot is capturing the section request
	other value:stand for the slot index which is capturing the section request	
*/
extern SHORT CH_GetSlotIndex(SLOT_ID FilterIndex);




/*Function description:
	Confirm whether a pid is being captured  
Parameters description: 
	int PID:the pid to be confirmed
return value:
	-1:the pid isn't captured 
	other value:the pid is being capturing and the value stand for which slot captured this pid  
	
*/
extern int CH6_FindePid(int PID);


/*Function description:
	Release the slot resource which a Pid used  
Parameters description: 
	int FreePID:the pid to be released 
return value:
	TRUE:stand for the  function is unsuccessful operated 
	FALSE:stand for the function is successful operated  
	
*/
extern BOOLEAN CH6_FreePidReq ( int FreePID );



/*yxl 2004-11-08 modify this function*/
extern BOOL CH6_StopFilter(SLOT_ID FilterIndex,BOOL IsUseInnerSem );

#endif

