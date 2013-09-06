#ifndef __CHPLATFORM_H__
#define __CHPLATFORM_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include             "..\MgDDI\MgSys.h"


/*****************************************************************************
  extern  variable 
 *****************************************************************************/
#if     /*PLATFORM_16 20070514 change*/1
extern               STPTI_Slot_t                                                        VideoSlot,AudioSlot;/*defined in the file "initterm.c" */
extern               STPTI_Handle_t                                                    PTIHandle;
extern               semaphore_t*                                                      pSemSectStrucACcess;
#endif





#ifdef BEIJING_MOSAIC
extern Mosaic_Level_Des  CurMosaic; /*add this on 050129*/
#endif

/*****************************************************************************
  extern function
 *****************************************************************************/
/*ch_nvodusif.c*/
#ifdef INTEGRATE_NVOD
extern NVODHideType CH_GetNvodHideTag(void);
extern boolean CH_GetStandByStatus(void);
extern  boolean Nvod_GetMenuStatus();  /*add this on 050113*/
/*ch_nvodusif.c*/
#endif


#ifdef BEIJING_MOSAIC
/*mosaicusif.c*/
extern PageTypenum CH_GetCurMosaicPageType(void);/*add this on 050123*/
/*mosaicusif.c*/
#endif

/*nitandcdt.c*/
extern SHORT CH_GetHardwareID(void);  /*add this on 050123*/
extern void GetSTBVersion(U8 *STBSoftVersionMajor,U8 *STBSoftVersionMinor);/*add this on 050123*/
/*nitandcdt.c*/


/*st_nvm.c*/
extern void  CH_FlashLock(void);
extern void  CH_FlashUnLock(void);
extern S16  CHCA_FlashWrite(unsigned long  offset,unsigned char*  pBuffer,unsigned long  Size);
/*st_nvm.c*/


/*flash.c*/
extern int  ReadStbId(U8*  iStbId); 
/*flash.c*/




/*vdbase.c*/
extern BOOLEAN   CH_StoreMulAudPids ( U8 *aucSectionData );
extern SHORT CH_GetsCurTransponderId(void);
extern SHORT CH_GetsCurProgramId(void);
extern void CH_ResetMultipleAudioData ( void ); /*add this on 050320*/
/*vdbase.c*/

#ifdef   MAIL_APP
/*dbase/ch_timer.c*/
extern boolean CH_NeedResetMailIndex(U8 CARD[6]);
extern void CH_ResetMainIndex(void);
/*dbase/ch_timer.c*/
#endif

extern CHCA_UINT   Ch_GetAudioSlotIndex(void);/*chprog.c*/
extern CHCA_UINT   Ch_GetVideoSlotIndex(void);/*chprog.c*/

/*vusif.c*/
extern void CH_SuspendUsif(void);
/*vusif.c*/



#ifdef  PAIR_CHECK  /*add this on 050113,write the */
/***Read data to E2PROM ****/
extern  int ReadNvmData(U32 addr,U16 number , void* buf);
/***Write data to E2PROM ****/
extern int WriteNvmData(U32 addr,U16 number , void* buf);
#endif




#if      /*PLATFORM_16 20070514 change*/1	 
/*Channel.c*/
extern void CH6_SendNewTuneReq(SHORT sNewTransSlot, TRANSPONDER_INFO_STRUCT *pstTransInfoPassed);
/*Channel.c*/
 #endif

 
#endif                                  /*__CHPLATFORM_H__ */
