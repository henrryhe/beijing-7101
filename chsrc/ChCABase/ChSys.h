#ifndef __CHSYS_H__
#define __CHSYS_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include             "ChPlatform.h"


#define              CHCA_MULTI_AUDIO /*add this on 041130*/

#define              MOSAIC_APP     /*add this on 041205*/
 #define              MAIL_APP   /*add this on 041213*/
 #define              PINRESET_APP   /*add this on 041213*/

 #define START_E2PROM_CASN                 7500  /*用于临时存放机号的问题2005年1月19日*/



extern                message_queue_t                                          *pstCHCAPMTMsgQueue;  /*defined in the file chprog.c*/
extern                message_queue_t                                          *pstCHCACATMsgQueue;	/*defined in the file chprog.c*/
extern                message_queue_t                                          *pstCHCAECMMsgQueue;	/*defined in the file chprog.c*/
extern                message_queue_t                                          *pstCHCAEMMMsgQueue;	/*defined in the file chprog.c*/

extern                semaphore_t                                                 *pSemMgApiAccess;                                               

#ifdef               PAIR_CHECK
extern                PAIR_NVM_INFO_STRUCT                                         pastPairInfo;
extern                TCHCAPairingStatus                                                 CardPairStatus;
extern                CHCA_BOOL  PairMgrInit (void);
extern                void  WritePairInfo(void);
extern    void  WritePairStautsInfo(void);  /*add this on 050515*/
#endif

/*chprog.c*/
extern CHCA_BOOL    CheckPairFlashStatus(void); /*add this on 050319*/
extern void   CHCA_CheckGeoRightStatus(void);/*add this on 050118*/
extern BOOL  ChSendMessage2PMT ( S16 sProgramID,S16  sTransponderID,PMTStaus  iModuleType ); /*add this on 050405*/
extern BOOL CHCA_StopBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType);
extern BOOL CHCA_StartBuildPmt(S16 sProgramID,S16  sTransponderID,BOOL  iModuleType);
extern BOOL CHCA_StopBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected);
extern BOOL CHCA_StartBuildCat(S16 sProgramID,S16  sTransponderID,BOOL  Disconnected);
extern BOOL CHCA_ProgInit ( void );
extern BOOL  CHMG_CtrlInstanceInit(void);  
extern CHCA_BOOL CHCA_SendFltrTimeout2CAT( CHCA_USHORT                        iFilterId,
                                                                    CHCA_BOOL                                      bTableType,
                                                                    TMGDDIEvSrcDmxFltrTimeoutExCode  ExCode,
                                                                    TMGDDIEventCode                             EvCode);
extern CHCA_BOOL CHCA_SendTimerInfo2CAT( CHCA_USHORT  iFilterId,CHCA_DDIEventCode  EvCode);
extern  void CHMG_CtrlCheckRejectedStatus(word iRejectCode,TMGAPIPIDList*  PIDData);
extern void CHMG_CtrlCheckCmptStatus(word CmptStatus,TMGAPICmptStatusList*  pList);
extern void CHMG_CtrlCheckRightAccepted(word AckCode,TMGAPIPIDList*  lpPID);
extern void CHMG_CtrlDisplayText(char*  pText);
extern void CHMG_CtrlNotifyMessage(word Type,TMGAPIBlk* pBlk);
extern void CHMG_UsrNotify(word  ExCode,udword EventData);
extern void CHMG_CaContentUpdate(void);
extern void CHMG_CheckNoResource(word  Type,dword  hSrc);
extern void CHMG_ResetPassword(word Result);
extern void CHMG_CheckSysError(word  ExCode,dword  ExCode2);
extern void CHMG_CheckHalted(word  ExCode,dword  ExCode2);
extern void CHMG_CheckClosed(word  ExCode,dword  ExCode2);
extern void CHMG_CheckCardReady(TMGAPICardContent* pContent);
extern  CHCA_BOOL  CHCA_MepgEnable(CHCA_BOOL  bVideoOpen,CHCA_BOOL  bAudioOpen);
extern CHCA_BOOL  CHCA_MosaicMepgEnable(CHCA_BOOL  iVideoOpen);/*add this on 050122*/
extern  CHCA_BOOL  CHCA_MepgDisable(CHCA_BOOL  bVideoClose,CHCA_BOOL  bAudioClose);
extern void   CHCA_CheckAVRightStatus(void);
extern CHCA_BOOL   CHCA_GetAVRightStatus(void);
extern CHCA_BOOL    CHCA_ResetCATFilterInfo(CHCA_USHORT  iFilterId);  /*add this on 050323*/
/*chprog.c*/


/*chdmux.c*/
extern CHCA_UINT  ChEcmFilterCheck( void); /*add this on 050313*/
extern  BOOL     CHCA_DemuxInit ( void );
/*extern void CHCA_EmmQueueInit ( void );*/
extern CHCA_UINT   CHCA_MgEmmSectionUnlock (void);
extern void  CHCA_InitMgEmmQueue(void);
CHCA_UINT     CHCA_GetQEmmLen(void);
extern void CHCA_SendNewTuneReq(SHORT sNewTransSlot, TRANSPONDER_INFO_STRUCT *pstTransInfoPassed);
/*chdmux.c*/


/*chtimer.c*/  
extern CHCA_BOOL  CHCA_RestartTimer(void);  /*add this on 050325*/
extern CHCA_DDIStatus  CHCA_GetTimeInfo(time_t*   pTime);
extern  CHCA_BOOL CHCA_TimerInit(void);
extern  CHCA_BOOL CHCA_TimerOperation(CHCA_USHORT    iTimer);
extern  CHCA_TICKTIME CHCA_GetCurrentClock(void);
extern  CHCA_TICKTIME CHCA_GetDiffClock(CHCA_TICKTIME  TimeEnd, CHCA_TICKTIME  TimeStart);
extern  CHCA_TICKTIME CHCA_ClockADD(CHCA_ULONG  DelayTimeMs);
extern  ChCA_Timer_Handle_t  CHCA_SetTimerOpen(CHCA_ULONG   DelayTime,CHCA_CALLBACK_FN  hCallback);
extern CHCA_DDIStatus  CHCA_SetTimerCancel(ChCA_Timer_Handle_t   hTHandle);
extern CHCA_DDIStatus  CHCA_GetTimeInfo(time_t*   pTime);
extern CHCA_BOOL  CHCA_SetTimerStop(CHCA_UINT   iTimerIndex);
/*chtimer.c*/

/*chreport.c*/
extern   void   CHCA_Report(CHCA_UCHAR * pStr,...);
/*chreport.c*/

/*chmemory.c*/
extern void CHCA_PartitionInit(void);
extern void CHCA_PartitionDel(void);
extern  TCHCAMemHandle   CHCA_MemAllocate(CHCA_ULONG  MemSize);
extern  CHCA_DDIStatus CHCA_MemDeallocate( TCHCAMemHandle  hMemHandle);
/*chmemory.c*/

/*chmutex.c*/
extern  TCHCAMutexHandle   CHCA_MutexCreate(void);
extern  CHCA_DDIStatus  CHCA_MutexDestroy(TCHCAMutexHandle  hMutex);
extern  CHCA_DDIStatus  CHCA_MutexLock(TCHCAMutexHandle  hMutex); 
extern  CHCA_DDIStatus   CHCA_MutexUnlock( TCHCAMutexHandle   hMutex);
extern  CHCA_BOOL   CHCA_MutexInit( void);
/*chmutex.c*/


/*chcard.c*/
extern  void  CHCA_ResetCatStartPMT(void);/*add this on 050201*/
extern void CHMG_CaContentUpdate(void);
#if 0/*delete this on 050311*/
extern CHCA_BOOL   CHCA_EcmDataOperation(void);
extern CHCA_BOOL   CHCA_CatDataOperation(void);/*add this on 050116*/
extern CHCA_BOOL   CHCA_EmmDataOperation(void);
#endif
extern  BOOL CHCA_GetCardContent(U8    *STB_SN);
extern  void ChSCGetCardContent(CHCA_UINT   iCardIndex);
extern CHCA_BOOL  CHCA_CheckCardReady(void);  /*add this on 041103*/
extern CHCA_BOOL CHCA_CardUnsubscribe(CHCA_USHORT    iCardNum);
extern CHCA_BOOL  CHCA_CardInit(void);
extern CHCA_BOOL CHMG_GetCardStatus(void);
extern void  CHMG_CardContent(TMGAPICardContent*   pCardContent);
/*extern  CHCA_PairingStatus_t  CHCA_GetPairStatus(void);*/
extern void  CHMG_CardRating(TMGAPIRating*  pCardRating);
extern void  CHMG_CardUnknown(TMGSysSCRdrHandle   hReader);
extern void CHMG_CheckCardContentUpdate(void);
 #ifndef CH_IPANEL_MGCA

extern CHCA_BOOL  CHCA_CardSendMess2Usif (STBStaus  CurStatus);

#endif
extern CHCA_INT   CHCA_AppPmtParseDes(CHCA_UCHAR*     iPMTBuffer);

#if  1
extern CHCA_DDIStatus CHCA_CardSendTest
	  ( CHCA_UCHAR* pMsg,CHCA_USHORT  Size);

#endif
/*chcard.c*/


/*chsys.c*/
extern  boolean  CHCA_GetPairRelease(char*   PRelease);   /*add this on 050511*/
extern  boolean  CHCA_GetKernelRelease(char*   KRelease);/*add this on 050311*/
extern  CHCA_BOOL  CHCA_MgGetStbID(CHCA_UCHAR*   iStbID);
extern CHCA_BOOL  CHCA_GetStbID(CHCA_UCHAR*   iStbID);
extern CHCA_BOOL   CHCA_GetSTBVersion(CHCA_UCHAR*  HVersion,CHCA_UCHAR*  SVersion);
extern CHCA_BOOL  CHCA_InhibitStb(void);
extern void CHMG_SubCallback
	 (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2);
extern void CHMG_CtrlCallback
   (HANDLE hSubscription,byte EventCode,word Excode,dword ExCode2);
extern void CHMG_CardCallback
   (HANDLE  hSubscription,byte EventCode,word Excode,dword ExCode2);
/*chsys.c*/

#endif                                  /*__CHSYS_H__ */
