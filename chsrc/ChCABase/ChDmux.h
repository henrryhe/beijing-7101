#ifndef __CHDEMUX_H__
#define __CHDEMUX_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include "ChSys.h"

#if 1
#define    MGCA_MAX_NO_EMMBUFF                               30  /*add this on 050120*/

#define    MGCA_NORMAL_TABLE_MAXSIZE                    CHCA_ONE_KILO_SECTION/*CHCA_FOUR_KILO_SECTION*/
#define    MGCA_PRIVATE_TABLE_MAXSIZE                   CHCA_SPECIAL_CADATA_MAX_LEN



extern semaphore_t                                                            *psemMgDescAccess;
extern semaphore_t                                                            *pSemMgApiAccess;       

extern  semaphore_t                                                           *psemSectionEmmQueueAccess;





/*****************************************************************************
the Ctrl structure type 
*****************************************************************************/


#if      /*PLATFORM_16 20070514 change*/1
typedef struct
{
      TCHCAPTIDescrambler             DescramblerKey;
      STPTI_Slot_t                                       avslot;
}Descram_Channel_Struct;
#endif


typedef struct DscrSlot_s
{
     CHCA_INT /*CHCA_UINT */                                       SlotValue;
      CHCA_USHORT                                    PIDValue;	  
      CHCA_BOOL                                        DscrAssociated;	  
      TCHCAPTIDescrambler                         Descramblerhandle;	  
      CHCA_BOOL                                        DscrStarted;	  
} DscrSlot_t;

typedef struct DDI_DscrCh_s
{
      /*U32                                    DscrKeyValide;
      U8                                         DscrChAssociated;	*/  
      CHCA_UCHAR                         DscrChAllocated;
      /*U32                                    slot[CHCA_SlOTORPID_NUM_PERKEY];
      U16                                        PID[CHCA_SlOTORPID_NUM_PERKEY];*/
      DscrSlot_t                               DscrSlotInfo[2];
      /*U16                                     PID[2];*/
      TCHCAPTIDescrambler            Descramblerhandle;	  
      /*U16                                     SlotCount;	  */
      CHCA_BOOL                           SigKeyMultiSlotSet;  	  
      TCHCAPTIDescrambler            OtherDscrCh; 	  
} DDI_DscrCh_t;



typedef struct Mg_filter_struct_tag
{
       CHCA_INT		       iSectionSlotId;
	CHCA_UCHAR             aucMgSectionData[MGCA_PRIVATE_TABLE_MAXSIZE];
	CHCA_UCHAR             OnUse ; /* true = buffer is lock ; false = buffer is unlock */
}MG_FILTER_STRUCT;


typedef struct tagMgQueue
{
    struct
    {
        CHCA_UCHAR                     buff [MGCA_PRIVATE_TABLE_MAXSIZE ] ;
        CHCA_UCHAR                     status;
	 CHCA_INT		               iSectionSlotId;	/* reporting section id (0-31) */
    }data[MGCA_MAX_NO_EMMBUFF];

    CHCA_SHORT sFront;
    CHCA_SHORT sRear;
}MgQueue;



typedef struct tagEmmQueue
{
        struct
        {
              CHCA_UCHAR           buff [MGCA_PRIVATE_TABLE_MAXSIZE];
              CHCA_UCHAR           status;
		CHCA_USHORT         iSectionFilterId;	  
              CHCA_SHORT           next;
        }data[MGCA_MAX_NO_EMMBUFF];

        CHCA_SHORT           sHeader;
        CHCA_SHORT           sTailer;
}QUEUE_EMM;


typedef struct DDICtrl_EvenSubscribe_s
{
       CHCA_BOOL                                    enabled;
       CHCA_CALLBACK_FN	                   CardNotifiy_fun;
       /*TCHCAEventSubscriptionHandle 	     iCtrlSubHandle;*/
} DDICtrl_EvenSubscribe_t;


typedef struct SRCOperationInfo_s
{
       CHCA_BOOL                                    bInUsed; 
	CHCA_BOOL                                    bSrcConnected;
	TCHCASysSrcHandle                        hSource;
	DDICtrl_EvenSubscribe_t                  EvenSubscribeInfo[CTRL_MAX_NUM_SUBSCRIPTION];
}SRCOperationInfo_t;


extern SRCOperationInfo_t                      SRCOperationInfo;  

#endif

typedef enum
{
      CHCA_SECTION_MODULE,
      CHCA_SCARD_MODULE,
      CHCA_TIMER_MODULE,
      CHCA_USIF_MODULE
} Message_Module_t;


typedef  enum 
{
       EMM_DATA,
	ECM_DATA,
	CAT_DATA,
	PMT_DATA,
	OTHER_DATA
}CHCA_MGData_t;


typedef	struct  chca_mg_cmd_struct_tag
{
       Message_Module_t                                               from_which_module;  
	union
	{
		struct
		{
		       /*U32                                                       iSectionSlotId;*/
			CHCA_MGData_t                                     TableId;   
			CHCA_USHORT		                           iSectionFilterId;	  /* reporting filter  id (0-31) */
                     CHCA_BOOL                                                  TableType;
			TMGDDIEvSrcDmxFltrTimeoutExCode       return_status;
			TMGDDIEventCode                                 TmgEventCode; 
		} sDmxfilter;

		struct
              {
                   CHCA_INT                                                         iCurProgIndex;      /* tuned to which program ID */
                   CHCA_INT                                                         iCurXpdrIndex;
                   chca_pmt_monitor_type                                      cmd_received;   /* start or stop database building */
		     PMTStaus                                                          ModuleType;       /*modify this on 050405*/ 
              }pmtmoni ;
		
		struct
              {
                   CHCA_INT                                                         iCurProgIndex;      /* tuned to which program ID */
                   CHCA_INT                                                         iCurXpdrIndex;
                   chca_cat_monitor_type                                       cmd_received;   /* start or stop database building */
		     CHCA_BOOL                                                      DisStatus;        /*true: disconnected, false: transponder changed*/
              }catmoni ;

	} contents;
	
}chca_mg_cmd_t;

extern MG_FILTER_STRUCT                                                   MgEmmFilterData;

extern partition_t                                                                 *CHSysPartition ;




/*chprog.c*/
extern CHCA_DDIStatus  CHCA_CtrlGetSources(TCHCASysSrcHandle* lhSource,CHCA_USHORT*  nSources);
extern CHCA_DDIStatus  CHCA_CtrlGetCaps(TCHCASysSrcHandle hSource);
extern CHCA_DDIStatus CHCA_CtrlGetStatus(TCHCASysSrcHandle  hSource);
extern CHCA_BOOL CHCA_CtrlUnsubscribe(CHCA_USHORT  iHandleCount);
extern  CHCA_BOOL  CHCA_CtrlSubscribe
	 (TCHCASysSrcHandle  hSource, CHCA_CALLBACK_FN  hCallback,CHCA_UCHAR  *SubHandle);
/*chprog.c*/




extern CHCA_MGData_t   CHCA_GetDataType(CHCA_UCHAR  bType);


/*interface function*/
#if 0 /*modify this on 050115*/
extern CHCA_SHORT  CHCA_Allocate_Section_Slot(void);
#else /*modify this on 050115*/
extern CHCA_SHORT  CHCA_Allocate_Section_Slot(CHCA_BOOL    iEmmSection);
#endif
extern CHCA_SHORT CHCA_Allocate_Section_Filter(void);
#if 0 /*modify this on 050115*/
extern CHCA_BOOL CHCA_Set_Slot_Pid(CHCA_USHORT SlotIndex,CHCA_PTI_Pid_t Pid);
#else
extern CHCA_BOOL CHCA_Set_Slot_Pid(CHCA_USHORT SlotIndex,CHCA_PTI_Pid_t Pid,CHCA_BOOL    iEmmSection);
#endif
extern CHCA_BOOL CHCA_Free_Section_Slot(CHCA_USHORT SlotIndex);
extern CHCA_BOOL CHCA_Start_Slot(CHCA_USHORT   SlotIndex);
extern CHCA_BOOL CHCA_Stop_Slot(CHCA_USHORT SlotIndex);
extern CHCA_BOOL CHCA_Stop_Filter(CHCA_USHORT  FilterIndex);
extern CHCA_BOOL CHCA_Start_Filter(CHCA_USHORT  FilterIndex);
extern CHCA_BOOL   CHCA_StartDemuxFilter(CHCA_USHORT  FilterIndex);
extern CHCA_BOOL   CHCA_StopDemuxFilter(CHCA_USHORT  FilterIndex); 
extern CHCA_BOOL  CHCA_PtiQueryPid ( CHCA_SHORT* SlotIndex,CHCA_PTI_Pid_t  Pid,CHCA_BOOL*  bSlotAllocated );
extern CHCA_BOOL CHCA_Free_Section_Filter(CHCA_USHORT   FilterIndex);
extern void CHCA_SectionDataAccess_Lock(void);
extern void CHCA_SectionDataAccess_UnLock(void);
extern CHCA_BOOL CHCA_Set_Filter(CHCA_USHORT           FilterIndex,
                                                    CHCA_UCHAR*           pData,
                                                    CHCA_UCHAR*           pMask,
                                                    CHCA_UCHAR*           pPatern,
                                                    CHCA_INT                  DataLen,
                                                    CHCA_BOOL               MatchMode);
extern CHCA_BOOL CHCA_MgDataNotifyRegister(CHCA_USHORT iFliter,
	                                                                                CHCA_CALLBACK_FN  hCallback);  
extern CHCA_BOOL  CHCA_MgDataNotifyUnRegister(CHCA_USHORT  iFliter);  
extern CHCA_INT   CHCA_GetFreeFilter(void);
extern  CHCA_BOOL CHCA_Associate_Slot_Filter(CHCA_USHORT SlotIndex,CHCA_USHORT  FilterIndex);
extern  CHCA_BOOL CHCA_CheckMultiFilter(CHCA_SHORT SlotIndex,CHCA_USHORT*  FilterCount); /*add this on 041027*/


/*descrambler*/
extern  CHCA_BOOL   CHCA_Allocate_Descrambler(CHCA_USHORT*    Deshandle );
extern  CHCA_BOOL   CHCA_Deallocate_Descrambler(CHCA_USHORT   DescramblerIndex);
extern  CHCA_BOOL  CHCA_Associate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid);
extern  CHCA_BOOL CHCA_Disassociate_Descrambler(CHCA_USHORT   DescramblerIndex,CHCA_USHORT  SlotOrPid);
extern  CHCA_BOOL  CHCA_Set_OddKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data);
extern CHCA_BOOL  CHCA_Set_EvenKey2Descrambler(CHCA_USHORT  DescramblerIndex,CHCA_UCHAR*  Data);
extern CHCA_BOOL  CHCA_Start_Descrambler(CHCA_UINT  iSlotIndex);
extern CHCA_BOOL  CHCA_Stop_Descrambler(CHCA_UINT  iSlotIndex);
extern void   CHCA_Descrambler_Init(void);
extern CHCA_USHORT   CHCA_Get_FreeKeyCounter(void);

#endif                                  /* __CHDEMUX_H__ */

