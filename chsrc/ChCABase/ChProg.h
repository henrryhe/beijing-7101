
#ifndef __CHPROG_H__
#define __CHPROG_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include        "ChDmux.h" 
#include        "ChCard.h"






/*****************************************************************************
  type definition
 *****************************************************************************/
#define         DSMCC_STREAM                                            0x0b


typedef enum
{
       PMTFIRST_LOOP,
       PMTSECOND_LOOP
} psit_loop_type;


typedef enum
{
       FTA_PROGRAM,
       SCRAMBLED_PROGRAM
} p_prog_type;


typedef enum
{
      VIDEO_TRACK,
      AUDIO_TRACK,
      MAUDIO_TRACK,  /*MOSAIC AUDIO TRACK add this on 050122*/
      PCR_TRACK,
      PRIVATE_TRACK,
      UNKNOWN_TRACK,
      DSMCC_TRACK,
      CHCA_INVALID_TRACK = -1
}t_track_type ;


typedef enum
{
      TelevisionService,
      RadioService,
      MosaicVideoService,
      UnknowService=-1
}t_service_type ;

 typedef struct tagstreampmt
{
     t_track_type                         track_type [ CHCA_MAX_TRACK_NUM ];
     CHCA_USHORT                     ElementPid [CHCA_MAX_TRACK_NUM];   	 
     p_prog_type                         ElementType[CHCA_MAX_TRACK_NUM];  /*add this on 041130*/   	 

     t_service_type                      ProgServiceType;
     CHCA_USHORT                     ProgElementPid [2];   	 
     CHCA_UINT                          ProgElementSlot [2]; 
     p_prog_type	                    ProgElementType[2]; 
     t_track_type                         ProgTrackType[2];	
     CHCA_INT                            ProgElementCurRight[2];	 
     CHCA_INT                            iProgNumofMaxTrack;

     CHCA_INT                            iNumofMaxTrack;

     CHCA_INT                            iVideo_Track;
     CHCA_INT                            iVideo_VA_Track;
     CHCA_INT                            iVideo_Track_Filter;

     CHCA_INT                            iAudio_Track;
     CHCA_INT                            iAudio_VA_Track;
     CHCA_INT                            iAudio_Track_Filter;

     CHCA_INT                            iVersionNum;
     CHCA_INT                            iProgramID;
     CHCA_INT                            iTransponderID;
     p_prog_type                         ProgType;	 
}STREAM_PMT_STRUCT;



#if 1 /*add this from same ecm filter process on 050310*/
typedef struct tagstreamecm
{
      CHCA_UCHAR                               ECM_Buffer[MGCA_PRIVATE_TABLE_MAXSIZE];
      CHCA_SHORT	                             EcmFilterId;
      CHCA_PTI_Pid_t                           FilterPID;/*add this on 050328*/	  
}STREAM_ECM_STRUCT;

extern  STREAM_ECM_STRUCT            EcmFilterInfo[2]; 
#endif

extern STREAM_PMT_STRUCT                                              stTrackTableInfo;
extern ST_DeviceName_t                                                    MGTUNERDeviceName;


/*called by the ch function*/
extern void CHMG_GetApiStatusReport (TMGAPIStatus  iChApiStatus);
extern BOOL  CHMG_CtrlOperationStop(void);
extern BOOL CHMG_CtrlCAStart(TMGAPICtrlHandle    pCtrlApiHandle);
extern void CHMG_SRCStatusChange(S16   iSrcChangedStatus);
extern BOOL CHMG_FTAProgPairCheck(void);
extern void   CHMG_ProgCtrlChange(TMGAPIOrigin* ipOrigin,U8*  PMTBuffer);
extern CHCA_BOOL  CHMG_ProgCtrlUpdate(U8*  PMTBuffer);
extern CHCA_BOOL   CHCA_StoreMulAudPids (CHCA_UCHAR *aucSectionData );
/*called by the ch function*/

/*20070514 add*/
extern void  GetPairInfo(void);
extern void  StopAllEcmFilter(void);
extern void  CH_FreeAllPMTMsg(void);
extern void  CH_FreeAllCATMsg(void);

extern void CH_CANVOD_ClearAVControl(void);
extern void CH_CANVOD_SetAVControl(void);
extern boolean CH_CANVOD_IsAVControl(void);
extern void CH_FreeCAPmt(void);
#endif                                  /*__CHPROG_H__ */
