#ifndef __MGSYS_H__
#define __MGSYS_H__

/*****************************************************************************
  include
 *****************************************************************************/
/*nagra france ca library*/
#define            DEFINE_BOOLEAN
#include           "..\MgKernel\MGDDI.h"
#include           "..\MgKernel\MGAPI.h"

#ifdef PAIR_CHECK
	#include           "..\MgKernel\MGAPIPaired.h"
#endif
/*nagra france ca library*/

/*st base include*/
#include           <stdarg.h>
#include           <stdio.h>/* fot the definition(such as NULL) */

#include           <string.h>
#include           <math.h>   

#include           "stddefs.h"/* for the standard definition (such as U8,U32...) */
#include           "stdevice.h"

#include           "stcommon.h"

#include           "..\report\report.h"
#include           "stsmart.h"/* for the definition of the smart card type */
#include           "stpti.h"       /*20070514 add*/
#include           "sectionbase.h"  /*20070514 add*/
#if 0/*051114 xjp comment*/
#include           "..\src\stsmart\stsmartuart.h"    /*add this on 041214*/
#endif

#include          "stevt.h"              /* for the definition of the evt type*/
#include          "..\symbol\symbol.h"
#include          "Appltype.h"

#if 0/*051114 xjp comment*/
#include          "pti.h"
#endif

#include          <time.h>

#ifdef          BEIJING_MOSAIC
#include            "..\mosaic\mosaic.h"
#include           "..\mosaic\mosaicCAIF.h"
#endif

/*for timer */
#include             "..\util\Ch_time.h"
/*for timer*/

/*st base include*/
#ifdef INTEGRATE_NVOD
#include            "..\ch_nvod\ch_nvodusif.h"  /*add this on 050107 for the stb status*/
#endif

#if      /*PLATFORM_16 20070514 change*/1
/*for demux part */
/*#include           "stgfx.h"*/

#include            "..\dbase\vdbase.h"
#include            "chdemux.h"
/*for demux part */

#include            "..\main\initterm.h"

#include            "Channelbase.h"   /*add this on 041117 for av control*/
extern STPTI_Signal_t gSignalHandle; /*add this on 050424*/


/*pti*/
typedef            STPTI_Descrambler_t                                           TCHCAPTIDescrambler;
typedef            STPTI_SlotOrPid_t                                                CHCA_PTI_SlotOrPid_t; 
typedef            STPTI_Descrambler_t                                           CHCA_PTI_Descrambler_t;
/*pti*/
#define             CHCA_MAX_NUM_FILTER                                    MAX_NO_FILTER_TOTAL /*32  defined in section.h*/
#endif

#if 0/*060106 xjp add*/
typedef unsigned int       key_t;
typedef unsigned int       slot_or_pid_t;
#endif


#define            CTRL_MAX_NUM_SUBSCRIPTION                16
#define            CHCA_MAX_NO_OPI                                    16
#define            CHCA_MGDDI_FILTERSIZE                         8

#define            CHCA_MAX_TRACK_NUM                             30 /*modify this on 050121,from 10 to 30*/
#define            CHCA_SlOTORPID_NUM_PERKEY                 2   /*maximum number of pids or slot per hardware channel is one*/

#define            CHCA_EVEN_PARITY_KEY                           0
#define            CHCA_ODD_PARITY_KEY                            1
#define            CHCA_KEY_INVALID                                   0
#define            CHCA_MAX_KEY_SIZE                                8
#define            CHCA_MGDDI_SRC_BASEADDRESS             0xf000
#define            CHCA_MGDDI_CARD_BASEADDRESS           0x0f00
#define            CHCA_MGDDI_CARDREADER_NUM              1
#define            CHCA_MGDDI_STREAMSOURCE_NUM          1
#define            CHCA_SPECIAL_CADATA_MAX_LEN            260
#define            CHCA_ONE_KILO_SECTION                         1024  	/* 1KB long section */
#define     	  CHCA_FOUR_KILO_SECTION                       4096  	/* 4KB long section */
#define            CHCA_MAX_DBUFFER_LEN                          CHCA_FOUR_KILO_SECTION
#define            CHCA_PMT_TABLE_ID                                 0x2
#define            CHCA_CAT_TABLE_ID                                 0x1     
#define            CHCA_MUTEX_MAX_NUM                             128   /*mutex*/
#define            CHCA_STBID_MAX_LEN                               8                        /*len of the stbid*/
#define            CHCA_MG_INVALID_PID                              0x2000
#define            CHCA_MG_INVALID_OPI                              0xffff
#define            CANAL_CA_SYSTEM_ID                                0x100
#define            CHCA_MAX_NO_TIMER                                 16              /*The max number of the timer*/

#define            CHCA_CHAR                                                  signed char
#define            CHCA_UCHAR                                                unsigned char
#define            CHCA_SHORT                                                signed short
#define            CHCA_USHORT                                              unsigned short
#define            CHCA_LONG                                                  long
#define            CHCA_ULONG                                                unsigned long
#define            CHCA_INT                                                     signed int
#define            CHCA_UINT                                                   unsigned int
#define            CHCA_BOOL                                                   int

#define            CHCA_TICKTIME                                            clock_t

typedef          CALLBACK                                                       CHCA_CALLBACK_FN;   /*ca call back typedef*/
typedef          void*                                                               TCHCA_HANDLE; 
typedef          CHCA_UINT*                                                    TCHCASysSCRdrHandle;   /*card drv handle typedef*/
typedef          CHCA_UINT*                                                    TCHCASysSrcHandle;    /*stream drv handle typedef*/
typedef          void*                                                               TCHCAMemHandle;   /*ram handle typedef*/
typedef          CHCA_UINT                                                     TCHCAMutexHandle;  /*mutext handle typedef*/
typedef          CHCA_UINT                                                     ChCA_Timer_Handle_t;  /*time handle typedef*/

/*partition*/
typedef            ST_Partition_t                                                 CHCA_Partition_t;
typedef            TDTDATE                                                        CHCA_TDTDATE;   /*date typedef*/
typedef            TDTTIME                                                        CHCA_TDTTIME;  /*time typedef*/
typedef            STPTI_Signal_t                                                CHCA_PTI_Signal_t;
typedef            STPTI_Pid_t                                                     CHCA_PTI_Pid_t;

/*partition*//*051114 xjp change*/
#define             CHCA_INVALID_LINK                                       INVALID_LINK         /*-1*/
#define             CHCA_DEMUX_INVALID_SLOT                         -1 /*20061209 change PTI_INVALID_SLOT  /*32*/
#define             CHCA_DEMUX_INVALID_PID                            ((unsigned int)0x7FFF)/*PTI_INVALID_PID /*32*/
#define             CHCA_DEMUX_INVALID_KEY                            /*PTI_NUMBER_OF_KEYS*/    8
#define             CHCA_MAX_DSCR_CHANNEL                             /*PTI_NUMBER_OF_KEYS*/   8


#define             CHCA_ILLEGAL_FILTER                                       -1
#define             CHCA_ILLEGAL_CHANNEL                                   -1
#define             CHCA_ILLEGAL_DESCRAMBLER                           -1

/*#define             CHCA_DEMUX_INVALID_PID                              0x1FFF*/



/*timer*/
#define  CHCA_GET_TICKPERMS       (CHCA_UINT)(ST_GetClocksPerSecond()*(CHCA_UINT)0.001)
#define  CHCA_WAIT_N_MS(x)         task_delay((clock_t)(x * CHCA_GET_TICKPERMS))
/*timer*/

/*card*/
/*card pairing status*/


typedef   enum
{
      CHCAUnknownStatus,
      CHCANoPair,
      CHCAPairError,
      CHCAPairOK
}TCHCAPairingStatus;


#ifdef               PAIR_CHECK
typedef struct
{
       CHCA_UCHAR                             CaSerialNumber[6]; 
	CHCA_UCHAR                             ScrambledBit0;
       CHCA_UCHAR                             ClearBit1;
	CHCA_UCHAR                             PairStatus2;/*add this on 050221*/    
}PAIR_NVM_INFO_STRUCT;
#endif


typedef struct
{
       CHCA_UCHAR                             SupportedProtocolTypes;        
       CHCA_UINT                                CurrentBaudRate;                                
} TCHCADDICardProtocol;


typedef struct
{
	CHCA_USHORT                           SupportedISOProtocols;	               
	CHCA_UINT		                     MaxBaudRate;			
} TCHCADDICardCaps;
/*card*/


/*status returned*/
typedef enum
{
	CHCADDIOK,
	CHCADDINoResource,
	CHCADDIBadParam,
	CHCADDIProtocol,
	CHCADDINotFound,
	CHCADDIBusy,
	CHCADDILocked,
	CHCADDINotSupported,
	CHCADDIError
} CHCA_DDIStatus;
/*status returned*/

/*ddi event*/
typedef enum
{
	CHCADDIEvCardExtract,
	CHCADDIEvCardInsert,
	CHCADDIEvCardResetRequest,
	CHCADDIEvCardReset,
	CHCADDIEvCardProtocol,
	CHCADDIEvCardReport,
	CHCADDIEvCardLockTimeout,

	CHCADDIEvSrcDmxFltrReport,
	CHCADDIEvSrcDmxFltrTimeout,
	CHCADDIEvSrcNoResource,
	CHCADDIEvSrcStatusChanged,

	CHCADDIEvDelay

} CHCA_DDIEventCode;
/*ddi event*/


typedef enum
{
    CHCA_FIRSTTIME_PMT_COME,
    CHCA_NONFIRSTTIME_PMT_COME,
    CHCA_PMT_UNALLOCTAED,
    CHCA_PMT_ALLOCTAED,
    CHCA_START_BUILDER_PMT,
    CHCA_STOP_BUILDER_PMT,
    CHCA_PMT_COLLECTED
}chca_pmt_monitor_type;


/*add this on 041122 for cat monitor*/
typedef enum
{
    CHCA_FIRSTTIME_CAT_COME,
    CHCA_NONFIRSTTIME_CAT_COME,
    CHCA_CAT_UNALLOCTAED,
    CHCA_CAT_ALLOCTAED,
    CHCA_START_BUILDER_CAT,
    CHCA_STOP_BUILDER_CAT,
    CHCA_CAT_COLLECTED
}chca_cat_monitor_type;
/*add this on 041122 for cat monitor*/

typedef struct
{
	CHCA_SHORT   	    OPINum;
	CHCA_UCHAR   	    ExpirationDate[2];
	CHCA_CHAR             OPIName[16];
	CHCA_UCHAR           OffersList[8];
	CHCA_UCHAR     	    GeoZone;
} TCHCAOperatorInfo;
extern     TCHCAOperatorInfo       OperatorList[];



typedef  struct
{
       CHCA_BOOL           Enabled;
	CHCA_UCHAR         CStbId[CHCA_STBID_MAX_LEN];     
}CHCA_StbId_Info_t;


typedef  enum
{
      CARD_INSTANCE,
      CTRL_INSTANCE,
      SYS_INSTANCE,
      OTHER_INSTANCE
}chmg_instance_reporting_t;

typedef struct  Kernel_EventCmd_struct_tag
{
       chmg_instance_reporting_t	       from_which_instance;
  	TMGAPIEventCode  	              KerEventCode;
       HANDLE                                      hInst;
	word		                            Result;  
	dword                                        ExResult;
}chmg_KerEvent_Cmd_t;

 /*add this on 050313*/
typedef  enum
{
      CAT_FILTER,
      PMT_FILTER,
      ECM_FILTER,
      EMM_FILTER,
      OTHER_FILTER
}chmg_filter_t;
  /*add this on 050313*/

typedef  struct   DDI_FilterInfo_S
{
       CHCA_PTI_Pid_t                  FilterPID;
       CHCA_BOOL                       Lock;
       CHCA_UCHAR                     DataBuffer[CHCA_ONE_KILO_SECTION]; /*from 300 to 1024*/
       CHCA_UCHAR                     DataCrc[4]; /*add this on 050317*/
	CHCA_SHORT                     SlotId;
       CHCA_BOOL                       InUsed;    /*add this on 040719*/
       CHCA_BOOL                       bdmxactived;      /*add this on 040625*/
	CHCA_BOOL                       ValidSection;       /*add this on 040625*/   
	CHCA_BOOL                       TTriggerMode;             /*add this on 040625*/
	clock_t                               CStartTime;                /*add this on 040625*/
	CHCA_ULONG                     uTmDelay;                 /*add this on 040525*/
       CHCA_BOOL                       MulSection;               /*add this on 041104*/
	CHCA_CALLBACK_FN          cMgDataNotify;
	chmg_filter_t                      FilterType;  /*add this on 050313*/
}DDI_FilterInfo_t;

extern   DDI_FilterInfo_t                FDmxFlterInfo[CHCA_MAX_NUM_FILTER];


#if      /*PLATFORM_16 20070514 change*/1 /*for ca flash address*/
      #if 0/*xjp 051228 add*/
	      #define     MGKERNEL_FLASH_DATADRESS    0x7ffbd808/*0x7ffdd808*/
	      #define     MGKERNEL_FLASH_ENDDRESS      0x7ffc0000/*0x7ffe0000*/
      #else 
	      #define     MGKERNEL_FLASH_DATADRESS   0xA12E0000/*0x403Ed808*/ /*zxg0061214 change Îª2¸ö8K¿éFLASH for 5107 4M */
	      #define     MGKERNEL_FLASH_ENDDRESS     (0xA12E0000+16*1024) 
      #endif
#endif



#endif                                  /*__MGSYS_H__ */

