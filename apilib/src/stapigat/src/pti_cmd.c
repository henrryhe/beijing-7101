/************************************************************************

File name   : pti_cmd.c

Description : Commands for STPTI

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                      Initials
----               ------------                                      --------
12 Mar 1999        Creation                                          FQ
08 Sep 1999        Change STTBX_Output() by STTBX_Print()            FQ
30 Sep 1999        Move init. PTI buffer in PTI_InitComponent()      FQ
04 Oct 1999        Use stlite.h instead of os20.h                    FQ
12 Jan 2001        Add STPTI commands                                FQ
29 Aug 2001        Move the PTI calls in the pti_ocmd.c file         AN
14 Oct 2002        Add support for 5517 and 5578                     HSdLM
04 Apr 2003        Add audio support and STPTI errors reporting      CL
09 Apr 2003        Add 7020 STEM (db573) support                     CL
16 Sep 2004        Add support for ST40/OS21                         FQ
************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include "testcfg.h"

#ifdef USE_PTI

#define USE_AS_FRONTEND

#ifdef ST_OSLINUX
#include "compat.h"
#include "stvidtest.h"
#ifdef USE_AUDIO
#include "staudlx.h"
#endif
/*#else*/
#endif

#include "sttbx.h"

#include "stevt.h"

#include "genadd.h"
#include "stddefs.h"
#include "stdevice.h"

#include "stcommon.h"
#include "testtool.h"
#ifdef USE_CLKRV
#include "stclkrv.h"
#endif
#include "stpti.h"

#include "clevt.h"
#ifdef USE_CLKRV
#include "clclkrv.h"
#endif
#include "api_cmd.h"
#include "pti_cmd.h"

#ifdef USE_VIDEO
#include "vid_util.h"
#endif
#if defined(ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_7710) || defined(ST_7100) \
 || defined(ST_7109)
#include "stmerge.h"

#elif defined(ST_7200)
#include "stfe.h"
#include "cli2c.h"
#include "sti2c.h"
#endif


/*###########################################################################*/
/*############################## DEFINITION #################################*/
/*###########################################################################*/

/*#######################################################################
 *# Platform # Configuration
 *#######################################################################
 *# STi5105  # - Packet injector (clock : rise, format : parallel)
 *#          # - NIM DB580A switch (1,2,3,4 : OFF,ON,OFF,OFF)
 *#######################################################################
 *# STi5100  # - Packet injector (format : serie)
 *#          # - STEM DB499B
 *#######################################################################*/

/* --- Constants ---------------------------------------------------------- */
#if defined (ST_7020) && (defined(mb382) || defined(mb314))
#define db573 /* clearer #define for below */
#endif

#if defined (mb290)  || defined (mb376) || defined (espresso)
#define BACK_BUFFER_MODE
#define PTI_VIDEO_BUFFER_SIZE         (64*1024) /* 64 KBytes */
#define PTI_AUDIO_BUFFER_SIZE         (8*1024) /* 8 KBytes */
#endif
/* TODO : stpti_init for more than 2 devices */
#if defined (ST_7200)
#define PTICMD_MAX_DEVICES     2
#define TSIN_MAX_NUMBER        4
#else
#define PTICMD_MAX_DEVICES     1
#endif /* defined(ST_7200) */

#if defined(ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_5105) || defined(ST_7710) \
 || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
/* to use stmerge, define SETUP_TSIN0_BYPASS (by-pass mode) or SETUP_TSIN0 (normal mode) */
/*#define SETUP_TSIN0*/
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define SETUP_TSIN0
#endif
#if !( defined SETUP_TSIN0 || defined SETUP_TSIN1 || defined SETUP_TSIN0_BYPASS )
 #define SETUP_TSIN0_BYPASS
#endif
#endif
#if defined (ST_7020) && defined(mb376)
 #define PTICMD_MAX_VIDEO_SLOT  7
#elif defined (mb290) || defined (mb295) || defined (db573)
 #define PTICMD_MAX_VIDEO_SLOT  5
#elif defined (mb314) || defined (ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_5105) \
   || defined(ST_7710)|| defined (ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
 #define PTICMD_MAX_VIDEO_SLOT  2
#else
 #define PTICMD_MAX_VIDEO_SLOT  1
#endif

#define PTICMD_MAX_PCR_SLOT    1

#ifdef USE_AUDIO
#define PTICMD_MAX_AUDIO_SLOT  1
#else
#define PTICMD_MAX_AUDIO_SLOT  0
#endif

#ifdef USE_AUDIO
  #ifdef ST_OSLINUX
/* Offset requested for STAUDLX */
    #define STC_AUDIO_OFFSET    (-7200)
  #else
    #define STC_AUDIO_OFFSET    0
  #endif
#else
  #define STC_AUDIO_OFFSET    0
#endif

/* Define for code readibility */
#define PTICMD_MAX_SLOTS (PTICMD_MAX_VIDEO_SLOT+PTICMD_MAX_AUDIO_SLOT+PTICMD_MAX_PCR_SLOT)
#define PTICMD_BASE_VIDEO (0)
#define PTICMD_BASE_AUDIO (PTICMD_MAX_VIDEO_SLOT)
#define PTICMD_BASE_PCR   (PTICMD_MAX_VIDEO_SLOT+PTICMD_MAX_AUDIO_SLOT)

typedef unsigned int ProgId_t; /* pid_t defined as int for pti/link, but U16 for spti/stpti4 */

/* Definitions for CLKRV event checks */
enum
{
   PTI_STCLKRV_STATE_PCR_OK = 0,
   PTI_STCLKRV_STATE_PCR_INVALID,
   PTI_STCLKRV_STATE_PCR_GLITCH
};

/* Maximum duration to wait for STCLKRV_PCR_DISCONTINUITY_EVT = 2 PCR < 100ms */
#define PTI_STCLKRV_PCR_DISCONTINUITY_TIMEOUT   (ST_GetClocksPerSecond() * 100/1000)
/* Maximum duration to wait for STCLKRV_PCR_VALID_EVT = 2s */
#define PTI_STCLKRV_PCR_VALID_TIMEOUT           (ST_GetClocksPerSecond() * 2)

/* --- Global variables --------------------------------------------------- */

BOOL            PCRValidCheck   = FALSE; /* Flag to enable the check of STCLKRV state machine */
BOOL            PCRInvalidCheck = FALSE; /* Flag to enable the check of STCLKRV state machine */
int             PCRState        = PTI_STCLKRV_STATE_PCR_INVALID;
semaphore_t     *PCRValidFlag_p,*PCRInvalidFlag_p;

#if defined BACK_BUFFER_MODE
static STPTI_Buffer_t VideoBufferHandle[PTICMD_MAX_VIDEO_SLOT];
#ifdef USE_AUDIO
static STPTI_Buffer_t AudioBufferHandle[PTICMD_MAX_AUDIO_SLOT];
#endif
#endif

/* Following could be move to the common injection structure */
static BOOL PtiCmdPCRStarted[PTICMD_MAX_DEVICES][PTICMD_MAX_PCR_SLOT];
#ifdef USE_AUDIO
static BOOL PtiCmdAudioStarted[PTICMD_MAX_DEVICES][PTICMD_MAX_AUDIO_SLOT];
#endif
STPTI_Handle_t STPTIHandle[PTICMD_MAX_DEVICES];
/* Order is first video, then audio and then PCR */
#ifdef ST_OSLINUX
STPTI_Slot_t SlotHandle[PTICMD_MAX_DEVICES][PTICMD_MAX_SLOTS];
#else
static STPTI_Slot_t SlotHandle[PTICMD_MAX_DEVICES][PTICMD_MAX_SLOTS];
#endif

static char PTI_Msg[200]; /* for trace */

static STEVT_Handle_t Evt_Handle_For_STPTI_Errors, Evt_Handle_Clkrv = 0 ;
static U32 Sections_Discarded_On_Crc_Check_Errors[PTICMD_MAX_SLOTS];
static U32 Buffer_Overflow_Errors[PTICMD_MAX_SLOTS];
static U32 CC_Errors[PTICMD_MAX_SLOTS];
static U32 Packet_Errors[PTICMD_MAX_SLOTS];
static U32 TC_Code_Fault_Errors[PTICMD_MAX_SLOTS];
static U32 Total_Stpti_Errors;

/* #pragma ST_device (device_byte_t)
   typedef volatile unsigned char device_byte_t; */

#if defined(ST_5514)
/* 5514 PTI3 setup parameters */
static U32 DeviceAddress[3] = {
    ST5514_PTIA_BASE_ADDRESS,
    ST5514_PTIB_BASE_ADDRESS,
    ST5514_PTIC_BASE_ADDRESS
};

static S32 InterruptNumber[3] = {
    ST5514_PTIA_INTERRUPT,
    ST5514_PTIB_INTERRUPT,
    ST5514_PTIC_INTERRUPT
};
#endif /* 5514 */

#if defined(ST_5516)
/* 5516 PTI3 setup parameters */
static U32 DeviceAddress[1] = {
    ST5516_PTIA_BASE_ADDRESS
};

static S32 InterruptNumber[1] = {
    ST5516_PTIA_INTERRUPT
};
#endif /* 5516 */

#if defined(ST_5517)
/* 5517 PTI3 setup parameters */
static U32 DeviceAddress[1] = {
    ST5517_PTIA_BASE_ADDRESS
};

static S32 InterruptNumber[1] = {
    ST5517_PTIA_INTERRUPT
};
#endif /* 5517 */

#if defined(ST_5528)
/* 5517 PTI3 setup parameters */
static U32 DeviceAddress[1] = {
    ST5528_PTIA_BASE_ADDRESS
};

static S32 InterruptNumber[1] ;

#endif /* 5528 */

#if defined(ST_5100)
/* 5100 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
    ST5100_PTI_BASE_ADDRESS
};

static S32 InterruptNumber[1] = {
    ST5100_PTI_INTERRUPT
};
#endif

#if defined(ST_5301)
/* 5301 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
    ST5301_PTI_BASE_ADDRESS
};

static S32 InterruptNumber[1];
#endif

#if defined(ST_5525)
/* 5301 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
    ST5525_PTI_BASE_ADDRESS
};

static S32 InterruptNumber[1];
#endif


#if defined(ST_5105)
/* 5105 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
    ST5105_PTI_BASE_ADDRESS
};

static S32 InterruptNumber[1] = {
    ST5105_PTI_INTERRUPT
};
#endif

#if defined(ST_5107)
/* 5107 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
    ST5107_PTI_BASE_ADDRESS
};

static S32 InterruptNumber[1] = {
    ST5107_PTI_INTERRUPT
};
#endif

#if defined(ST_7100) || defined(ST_7109)
/* 7100 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
#ifndef NATIVE_CORE
    PTI_BASE_ADDRESS
#else
	0
#endif
};

static S32 InterruptNumber[1] ;
#endif

#if defined(ST_7200)

/* 7200 PTI4 setup parameters */
static U32 DeviceAddress[2] = {
#ifndef NATIVE_CORE
    PTIA_BASE_ADDRESS,
    PTIB_BASE_ADDRESS
#else
	0,
	0
#endif
};

static S32 InterruptNumber[1] ;
#endif

#if defined(ST_7710)
/* 7710 PTI4 setup parameters */
static U32 DeviceAddress[1] = {
    ST7710_PTI_BASE_ADDRESS
};
static S32 InterruptNumber[1] = {
    ST7710_PTI_INTERRUPT
};
#endif


#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) \
 || defined(ST_5100) || defined(ST_5105) || defined(ST_5301)|| defined(ST_5525) || defined (ST_7100) || defined (ST_7710) \
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)

#if (defined (ST_5528) || defined (ST_5100) || defined (ST_5105) || defined (ST_5301)|| defined(ST_5525) ||defined (ST_7100) \
  || defined (ST_7710) || defined (ST_7109) || defined(ST_7200) || defined (ST_5107)) && defined (DVD_TRANSPORT_STPTI4)
    static STPTI_Device_t DeviceType = STPTI_DEVICE_PTI_4;
#ifdef ST_OSLINUX
    static BOOL PTICMD_DeviceTypeEventErrorsCapable = FALSE; /* FALSE for linux test purpose, TRUE instead */
#else
    static BOOL PTICMD_DeviceTypeEventErrorsCapable = TRUE;
#endif

#elif (defined (ST_5514) || defined (ST_5516) || defined (ST_5517)) && defined (DVD_TRANSPORT_STPTI)
    static STPTI_Device_t DeviceType = STPTI_DEVICE_PTI_3;
    static BOOL PTICMD_DeviceTypeEventErrorsCapable = TRUE;
#else
    /* Gives an error message during the compilation. The aim is that the user reminds to set   */
    /* DVD_TRANSPORT to STPTI4 (and not STPTI) on 5528 and 5100.                                */
    #error STPTI not compatible with the chip
#endif
#ifdef STVID_DIRECTV
#if defined (ST_5528) || defined(ST_5301) || defined(ST_5525) || defined (ST_7710)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DTVTCLoader;
#elif defined(ST_5105) || defined(ST_5107)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DTVTCLoaderUL;
#elif defined(ST_7100) || defined(ST_5100)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DTVTCLoaderL;
#elif defined(ST_7109) || defined(ST_7200)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DTVTCLoaderSL;
#else
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DTVTC3Loader;
#endif
    static U32 SyncLock = 0 ;
    static U32 SyncDrop = 0 ;
#else /* DVB */
#if defined (ST_5528) || defined(ST_5301) || defined(ST_5525) || defined (ST_7710)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTCLoader;
#elif defined(ST_5105) || defined(ST_5107)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTCLoaderUL;
#elif defined (ST_7100) || defined(ST_5100)
#if !defined ST_OSLINUX
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTCLoaderL;
#endif /* ST_OSLINUX */
#elif defined(ST_7109) || defined(ST_7200)
#if !defined ST_OSLINUX
    #if defined(ST_7109)
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTCLoaderSL;
    #else
        static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTCLoaderSL2;
    #endif /* defined(ST_7109) */
#endif /* ST_OSLINUX */
#else
    static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTC3Loader;
#endif

#if defined(mb290) || defined (mb376) || defined (espresso) || defined(mb391) || defined(mb400) || defined(maly3s)
/* set HW synchro detection for ATV2 and mb376/espresso , ...*/
    static U32 SyncLock =  0 ;
    static U32 SyncDrop =  0 ;
#else /* Other than ATV2 : */
#if defined(mb390)
    static U32 SyncLock =  1 ;
    static U32 SyncDrop =  3 ;
#else
#if defined(mb411)
    #ifdef SETUP_TSIN0_BYPASS
    static U32 SyncLock =  3;
    static U32 SyncDrop =  5;
    #else
    static U32 SyncLock =  0;
    static U32 SyncDrop =  0;
    #endif /* SETUP_TSIN0_BYPASS */
#else
    static U32 SyncLock =  3 ;
    static U32 SyncDrop =  1 ;
#endif
#endif
#endif

#endif /* STVID_DIRECTV */
#if defined(ST_5528) || defined(ST_5100) || defined(ST_5301) || defined(ST_5525) || defined(ST_5105) || defined (ST_7710) \
 || defined(ST_7100) || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
    static STPTI_FilterOperatingMode_t SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_32x8;
#else
    static STPTI_FilterOperatingMode_t SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_32xANY;
#endif /* ST_5528 for STPTI_FilterOperatingMode_t */

#else /* if not 5514 5516 5517 5528 5100 7100 7710 7109 7200 */

/* other than 5516/5514 setup parameters */
static U32 DeviceAddress[1] = { PTI_BASE_ADDRESS };
static S32 InterruptNumber[1] = { PTI_INTERRUPT };
static STPTI_Device_t DeviceType = STPTI_DEVICE_PTI_1;
static BOOL PTICMD_DeviceTypeEventErrorsCapable = FALSE;
static ST_ErrorCode_t (*TCLoader_p) (STPTI_DevicePtr_t, void *) = STPTI_DVBTC1LoaderAutoInputcount;
static U32 SyncLock =  0 ;
static U32 SyncDrop =  0 ;
static STPTI_FilterOperatingMode_t SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_32x8;

#endif /* 5514 5516 5517 5528 5100 7100 7710 7109 7200 */

static ST_DeviceName_t PTIDeviceName[5] = {
    STPTI_DEVICE_NAME,
    "STPTI2",
    "STPTI3",
    "STPTI4",
    "STPTI5"
};


static osclock_t MaxDelayForLastPacketTransferWhenStptiPidCleared = 0;

#if defined (ST_7200)
STPTI_Frontend_t         STPTI_FrontendHandle;
static STI2C_Handle_t    I2CHndl=-1;
/* Clock info structure setup here (for global use)  */
ST_ClockInfo_t ST_ClockInfo;
#endif /* defined (ST_7200) */

#ifdef ST_OSLINUX
#ifdef USE_AUDIO
/* Imported and used by aud_cmd.c */
STPTI_Buffer_t  PTI_AudioBufferHandle;
STPTI_Buffer_t  PTI_AudioBufferHandle_p[2];
#endif
#endif

/* --- Extern variables (from startup.c) ---------------------------------- */
extern ST_Partition_t *DriverPartition_p;
extern ST_Partition_t *NCachePartition_p;

#ifdef USE_VIDEO
extern VID_Injection_t VID_Injection[];
extern void VID_ReportInjectionAlreadyOccurs(U32 CDFifoNb);
#endif

#ifdef USE_STFAE_CorrectPTI
ST_ErrorCode_t  STFAE_CorrectPTI(STPTI_Handle_t PTI_Handle, U32 ShiftValue);
#endif

/* Private Function prototypes ---------------------------------------------- */
#ifdef STVID_INJECTION_BREAK_WORKAROUND
void PTI_VideoWorkaroundPleaseStopInjection(
                                STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                const void *EventData,
                                const void *SubscriberData_p);
void PTI_VideoWorkaroundThanksInjectionCanGoOn(
                                STEVT_CallReason_t Reason,
                                const ST_DeviceName_t RegistrantName,
                                STEVT_EventConstant_t Event,
                                const void *EventData,
                                const void *SubscriberData_p);
#endif /* STVID_INJECTION_BREAK_WORKAROUND */

#ifndef ST_OSLINUX
ST_ErrorCode_t GetWritePtrFct(void * const Handle, void ** const Address_p);
void SetReadPtrFct(void * const Handle, void * const Address_p);
#endif
BOOL PTI_VideoStop(U32 DeviceNb, U32 SlotNumber, BOOL Verbose);
BOOL PTI_VideoStart(ProgId_t PID, U32 DeviceNb, U32 SlotNumber, BOOL Verbose);
BOOL PTI_PCRStop(U32 DeviceNb, U32 SlotNumber);
BOOL PTI_VideoStopPID(parse_t *pars_p, char *result_sym_p);



/*##########################################################################*/
/*######################### INTERNAL FUNCTIONS #############################*/
/*##########################################################################*/

#define PORT4              4
#define I2CADDR_PCF8575  0x4E



#ifdef USE_STFAE_CorrectPTI
ST_ErrorCode_t STFAE_CorrectPTI(STPTI_Handle_t PTI_Handle, U32 ShiftValue)
{
  U32 i;
  U32 CodeSize = 0;
  U32 *TCIAddr = (U32*)0xb923c000;
  U32 Offset = 0;
  U32 toto = 0;
  U32 Interrupt_Mask[4];
  U32 TSInValue[5];
  U32 JumpValue = 0xc0000000 | (ShiftValue << 6);
  static U32  TCIRam[9*1024];

  TCPrivateData_t      *PrivateData_p  = (TCPrivateData_t *)stptiMemGet_PrivateData( (PTI_Handle) );
  STPTI_TCParameters_t *TC_Params_p    = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
  CodeSize = TC_Params_p->TC_CodeSize;

  if (CodeSize + ShiftValue >= 9*1024)
    return (ST_ERROR_NO_MEMORY);

  memset(TCIRam, 0, 9*1024);

  *(U32*)0xb9236008 = 0;

  for (i = 0; i < 5; i++)
  {
    TSInValue[i] = *(U32*)(0xb9242000 + i * 0x20);
    *(U32*)(0xb9242000 + i * 0x20) &= ~0x80;
  }
  for (i = 0; i < 4; i++)
  {
    Interrupt_Mask[i] = ((U32*)0xb9230010)[i];
    ((U32*)0xb9230010)[i] = 0;
  }
  *(U32*)0xb9230030 = 0;
  for (i = 0; i < CodeSize/4; i++)
  {
    toto = TCIAddr[i];
    TCIRam[i] = toto;
  }

  for (i = 0; i < CodeSize/4; i++)
  {
    if (((U32)(TCIRam[i]) & 0x80000000) != 0)
    {
      U32 Instr = 0;

      Instr = TCIRam[i];

      if ((Instr >> 26) != 0x38)
      {
        Offset = Instr & 0x007FFFC0;

        Instr &= ~0x007FFFC0;

        Offset >>= 6;

        Offset += ShiftValue;

        Offset <<= 6;

        Instr |= (Offset & 0x7FFFC0);
        TCIRam[i] = Instr;
      }
    }
  }


  for (i = 0; i < ShiftValue/4; i++)
  {
   // TCIAddr[i] = 0x2c000040;
    TCIAddr[i] = JumpValue;
  }

  for (i = 0; i < (CodeSize/4); i++)
  {
    TCIAddr[i + ShiftValue/4] = TCIRam[i];
  }

  *(U32*)0xb9230030 = 2;

  for (i = 0; i < 4; i++)
  {
    ((U32*)0xb9230010)[i] = Interrupt_Mask[i];
  }
  *(U32*)0xb9236008 = 1;
  *(U32*)0xb9230030 = 1;
  for (i = 0; i < 5; i++)
  {
    *(U32*)(0xb9242000 + i * 0x20) = TSInValue[i];
  }

  return ST_NO_ERROR;
}
#endif

#ifdef DVD_TRANSPORT_STPTI4
/* INTERFACE OBJECT: WRAPPER FCT TO LINK PTI AND VIDEO */
#ifdef ST_OSLINUX

    /* These 2 functions have been directly integrated into the driver */
    /* To be changed later ... */

#else
ST_ErrorCode_t GetWritePtrFct(void * const Handle, void ** const Address_p)
{
    ST_ErrorCode_t Err;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;

    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        Err = STPTI_BufferGetWritePointer((STPTI_Buffer_t)Handle,Address_p);
        *Address_p = STAVMEM_VirtualToDevice(*Address_p, &VirtualMapping);
        return(Err);
    }
    return(ST_ERROR_BAD_PARAMETER);
}

void SetReadPtrFct(void * const Handle, void * const Address_p)
{
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    /* registered handle = index in cd-fifo array */
    if (STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping) == ST_NO_ERROR)
    {
        /* just cast the handle */
        STPTI_BufferSetReadPointer((STPTI_Buffer_t)Handle, STAVMEM_DeviceToVirtual(Address_p,&VirtualMapping));
    }
}
#endif /* DVD_TRANSPORT_STPTI4 */
#endif /* ST_OSLINUX */


static void ptiErrorsCallBack(STEVT_CallReason_t Reason,
                          const ST_DeviceName_t RegistrantName,
                          STEVT_EventConstant_t Event,
                          const void *EventData,
                          const void *SubscriberData_p);

/*******************************************************************************
Name        : ptiErrorsCallBack
Description : Counts error events reported by STPTI
Parameters  : Reason, type of the event.
              Event, nature of the event.
              EventData, data associted to the event.
Assumptions : At least one event have been subscribed.
Limitations :
Returns     : Nothing.
*******************************************************************************/
static void ptiErrorsCallBack(STEVT_CallReason_t Reason,
                          const ST_DeviceName_t RegistrantName,
                          STEVT_EventConstant_t Event,
                          const void *EventData,
                          const void *SubscriberData_p)
{
    int SlotIndex;
    STPTI_Slot_t EvtSlotHandle;
    int i;
    U32 DeviceNb;

    SlotIndex = -1;
    i = 0;
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);

    DeviceNb = (U32) SubscriberData_p;
    EvtSlotHandle = ((STPTI_EventData_t *)(U32) EventData)->SlotHandle;
    while ((SlotIndex == -1) && (i < PTICMD_MAX_SLOTS))
    {
        if (EvtSlotHandle == SlotHandle[DeviceNb][i])
        {
            SlotIndex = i;
        }
        i++;
    }
    if (SlotIndex==-1)
    {
        SlotIndex=0;
    }

    Total_Stpti_Errors++;

    switch (Event)
    {
        case STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT :
            Sections_Discarded_On_Crc_Check_Errors[SlotIndex]++;
            break;

        case STPTI_EVENT_BUFFER_OVERFLOW_EVT :
            Buffer_Overflow_Errors[SlotIndex]++;
            break;

        case STPTI_EVENT_CC_ERROR_EVT :
            CC_Errors[SlotIndex]++;
            break;

        case STPTI_EVENT_PACKET_ERROR_EVT :
            Packet_Errors[SlotIndex]++;
            break;

        case STPTI_EVENT_TC_CODE_FAULT_EVT :
            TC_Code_Fault_Errors[SlotIndex]++;
            break;

        default :
            /* Nothing special to do !!! */
            break;
    }

} /* end of ptiErrorsCallBack */



/*****************************************************************************
Name        : PTI_Subscribe_Error_Events
Description : Enable and register to some STPTI error events
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL PTI_Subscribe_Error_Events(U32 DeviceNb)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetOK;
    STEVT_OpenParams_t      OpenParams;
    STEVT_DeviceSubscribeParams_t   LocSubscribeParams;

    RetOK = TRUE;

    Evt_Handle_For_STPTI_Errors = 0;
    ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &Evt_Handle_For_STPTI_Errors);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("STEVT_Open for STPTI errors subscription failed. Error 0x%x ! ", ErrCode));
        RetOK = FALSE;
    }
    else
    {
        LocSubscribeParams.NotifyCallback       = ptiErrorsCallBack;
        LocSubscribeParams.SubscriberData_p     = (void *) DeviceNb;

        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT, &LocSubscribeParams);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_BUFFER_OVERFLOW_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_BUFFER_OVERFLOW_EVT, &LocSubscribeParams);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_CC_ERROR_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_CC_ERROR_EVT, &LocSubscribeParams);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_PACKET_ERROR_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_PACKET_ERROR_EVT, &LocSubscribeParams);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_TC_CODE_FAULT_EVT);
        ErrCode |= STEVT_SubscribeDeviceEvent (Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_TC_CODE_FAULT_EVT, &LocSubscribeParams);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STPTI errors subscription failed. Error 0x%x ! ", ErrCode));
            RetOK = FALSE;
        }
    }

    return(RetOK);
} /* end of PTI_Subscribe_Error_Events() */


/*****************************************************************************
Name        : PTI_Unsubscribe_Error_Events
Description : Enable and register to some STPTI error events
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL PTI_Unsubscribe_Error_Events(U32 DeviceNb)
{
    ST_ErrorCode_t          ErrCode;
    BOOL                    RetOK;

    RetOK = TRUE;
    ErrCode = ST_NO_ERROR;

    if (Evt_Handle_For_STPTI_Errors==0)
    {
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_TC_CODE_FAULT_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_TC_CODE_FAULT_EVT);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_PACKET_ERROR_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_PACKET_ERROR_EVT);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_CC_ERROR_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_CC_ERROR_EVT);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_BUFFER_OVERFLOW_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_BUFFER_OVERFLOW_EVT);
        ErrCode |= STPTI_EnableErrorEvent(PTIDeviceName[DeviceNb], STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
        ErrCode |= STEVT_UnsubscribeDeviceEvent(Evt_Handle_For_STPTI_Errors, PTIDeviceName[DeviceNb],
                STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STPTI errors Unsubscription failed. Error 0x%x ! ", ErrCode));
            RetOK = FALSE;
        }
        STEVT_Close(Evt_Handle_For_STPTI_Errors);
    }

    return(RetOK);
} /* end of PTI_Unsubscribe_Error_Events() */


/*****************************************************************************
Name        : CallBack_STCLKRV_PCR_VALID_EVT
Description :
Parameters  : None
Assumptions :
Limitations :
Returns     :
*****************************************************************************/
static void CallBack_STCLKRV_PCR_VALID_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);

/*    STTBX_Print(("CLKRV: PCR_VALID_EVT\n"));*/

    PCRState = PTI_STCLKRV_STATE_PCR_OK;

    if (PCRValidCheck == TRUE)
    {
        /* We want to get a signal if we fall into this state */
        semaphore_signal(PCRValidFlag_p);
    }

    return;
}

/*****************************************************************************
Name        : CallBack_STCLKRV_PCR_DISCONTINUITY_EVT
Description :
Parameters  : None
Assumptions :
Limitations :
Returns     :
*****************************************************************************/
static void CallBack_STCLKRV_PCR_DISCONTINUITY_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);

/*    STTBX_Print(("CLKRV: PCR_DISCONTINUITY_EVT\n"));*/
    PCRState = PTI_STCLKRV_STATE_PCR_INVALID;

    if (PCRInvalidCheck == TRUE)
    {
        /* We want to get a signal if we fall into this state */
        semaphore_signal(PCRInvalidFlag_p);
    }

    return;
}

/*****************************************************************************
Name        : CallBack_STCLKRV_PCR_GLITCH_EVT
Description :
Parameters  : None
Assumptions :
Limitations :
Returns     :
*****************************************************************************/
static void CallBack_STCLKRV_PCR_GLITCH_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData);
    UNUSED_PARAMETER(SubscriberData_p);

/*    STTBX_Print(("CLKRV: STCLKRV_PCR_GLITCH_EVT\n"));*/

    PCRState = PTI_STCLKRV_STATE_PCR_GLITCH;

    return;
}


/*****************************************************************************
Name        : PTI_Init
Description : Initialise the STPTI driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*****************************************************************************/
static BOOL PTI_Init(U32 DeviceNb, U32 TsinNb)
{
    BOOL                            RetOK;
    STPTI_InitParams_t              STPTIInitParams;
    STPTI_OpenParams_t              STPTIOpenParams;
    STPTI_SlotData_t                SlotData;
    ST_ErrorCode_t                  ErrCode;
    STEVT_DeviceSubscribeParams_t   DevSubscribeParams;
    STEVT_OpenParams_t              STEVT_OpenParams;
#if defined (ST_7200)
    STPTI_FrontendParams_t          STPTIFrontendParams;
#endif /* defined (ST_7200) */

#if defined BACK_BUFFER_MODE
    U32 SlotNumber;
#else
#if !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5525) && !defined(ST_5105) && !defined(ST_7710) && !defined(ST_7100) \
 && !defined(ST_7109) && !defined(ST_7200) && !defined(ST_5107)
    STPTI_DMAParams_t               VID_InjectionParams_p;
#endif /* !ST_5100 && !ST_5301 && !ST_7100 && !ST_7710 && !ST_7109 && !ST_7200 */
#endif /* BACK_BUFFER_MODE */
#if defined (ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_7710) || defined(ST_7100) \
 || defined(ST_7109)
    STMERGE_InitParams_t            STMInitParams;
    #if defined(SETUP_TSIN0) || defined(SETUP_TSIN1)
    STMERGE_ObjectConfig_t          TSIN;
    #endif
#endif
    U32 i;
#if defined(ST_7100) || defined(ST_7109)
#ifndef ST_OSLINUX
    /* U32 TSMERGER_SRAM_MemoryMap[6][2] = {{STMERGE_TSIN_0,6*128},{STMERGE_TSIN_1,6*128},{STMERGE_TSIN_2,1*128},{STMERGE_SWTS_0,6*128},{STMERGE_ALTOUT_0,1*128},{0,0}}; */
#endif
#endif /* ST_7100 || ST_7109 */

    RetOK = TRUE;


#if defined(ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_7710) || defined(ST_7100) \
 || defined(ST_7109)
    /* --- TS merger initialization --- */

	/* Initialise TSMerge so TSIN bypass block */
    STTBX_Print(( "STMERGE initializing, \trevision=%-21.21s ...\n", STMERGE_GetRevision() ));

    STMInitParams.DeviceType = STMERGE_DEVICE_1;
    STMInitParams.DriverPartition_p = DriverPartition_p;
    STMInitParams.BaseAddress_p = (U32 *)TSMERGE_BASE_ADDRESS;
    STMInitParams.MergeMemoryMap_p = STMERGE_DEFAULT_MERGE_MEMORY_MAP;

    #if defined(SETUP_TSIN0_BYPASS)
      STMInitParams.Mode = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
    #else
        STMInitParams.Mode = STMERGE_NORMAL_OPERATION;
    #endif /* defined(SETUP_TSIN0_BYPASS) */

    ErrCode = STMERGE_Init("MERGE0",&STMInitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s init. error 0x%x ! ", "MERGE0", ErrCode));
        RetOK = FALSE;
    }

   	/*Setup TSin0 - this is a serial input on the board*/
#if defined(SETUP_TSIN0) || defined(SETUP_TSIN1)
    memset(&TSIN,0,sizeof(TSIN));
#endif /* SETUP_TSIN0 || SETUP_TSIN1 */

    #if defined(SETUP_TSIN0_BYPASS)
        STTBX_Print(("BY-PASS mode for TSIN0\n"));
        /*BYPASS mode*/
        /* 0xF0603 stream_conf_2 (TSin0, tagging off, Synced, serial input)*/
        /* STMERGE_SetParams() not allowed in bypass mode*/
    #endif
    #if defined(SETUP_TSIN0)
        STTBX_Print(("NORMAL mode, Setup for TSin0\n"));
        /* 0xF0603 stream_conf_2 (TSin0, tagging off, Synced, serial input)*/
        /*PTI0_destination - TSin0*/
        /*stream_sync_2 (TSin0) - 188 byte packets, 47 syncbyte, 3 and 1 sync lock and dro p*/
        /*stream_conf_2 Enable)*/
        /* note : tested with LVDS Serial, and setup like in stmerge test application */
        TSIN.Priority = STMERGE_PRIORITY_HIGH;
        TSIN.SyncLock = 0;
        TSIN.SyncDrop = 0;
        TSIN.SOPSymbol = 0x47;
        #ifdef STVID_DIRECTV
        TSIN.PacketLength = STMERGE_PACKET_LENGTH_DTV;
        TSIN.Channel_rst = FALSE;
        TSIN.Disable_mid_pkt_err = FALSE;
        TSIN.Disable_start_pkt_err = FALSE;
        #else
        TSIN.PacketLength = STMERGE_PACKET_LENGTH_DVB;
        #endif
        TSIN.u.TSIN.SerialNotParallel = TRUE;
        TSIN.u.TSIN.SyncNotAsync = FALSE;
        TSIN.u.TSIN.InvertByteClk = FALSE;
        TSIN.u.TSIN.ByteAlignSOPSymbol = TRUE;
        TSIN.u.TSIN.ReplaceSOPSymbol = FALSE;
#if defined(ST_7100) || defined(ST_7109)
        #ifndef STVID_DIRECTV
        TSIN.SyncLock = 3;
        TSIN.SyncDrop = 3;
        #endif
        TSIN.u.TSIN.SerialNotParallel = FALSE;
        TSIN.u.TSIN.ByteAlignSOPSymbol = FALSE;
#endif
        ErrCode = STMERGE_SetParams(STMERGE_TSIN_0, &TSIN);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("%s set params error 0x%x for TSin0 ! \n", "MERGE0", ErrCode));
            RetOK = FALSE;
        }
        /* Connect to PTI */
        ErrCode = STMERGE_Connect(STMERGE_TSIN_0, STMERGE_PTI_0);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("%s connect TSIN_0 to PTI_0 error 0x%x for TSin1 ! \n", "MERGE0", ErrCode));
            RetOK = FALSE;
        }
    #endif

	#if defined(SETUP_TSIN1)
        STTBX_Print(("NORMAL mode, Setup for TSin1\n"));
		/*Setup TSin1 - this is a serial input on the board*/
        /* 0xF0603 stream_conf_1 (TSin1, tagging off, Synced, serial input)*/
        /*stream_sync_2 (TSin2) - 188 byte packets, 47 syncbyte, 0 and 0 sync lock and dro p*/
        /*stream_conf_1 Enable)*/
        TSIN.Priority = STMERGE_PRIORITY_HIGH;
        TSIN.SyncLock = 0;
        TSIN.SyncDrop = 0;
        TSIN.SOPSymbol = 0x47;
        #ifdef STVID_DIRECTV
        TSIN.PacketLength = STMERGE_PACKET_LENGTH_DTV;
        #else
        TSIN.PacketLength = STMERGE_PACKET_LENGTH_DVB;
        #endif
        TSIN.u.TSIN.SerialNotParallel = TRUE;
        TSIN.u.TSIN.SyncNotAsync = FALSE;
        TSIN.u.TSIN.InvertByteClk = FALSE;
        TSIN.u.TSIN.ByteAlignSOPSymbol = TRUE;
        TSIN.u.TSIN.ReplaceSOPSymbol = FALSE;
        ErrCode = STMERGE_SetParams(STMERGE_TSIN_1, &TSIN);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("%s set params error 0x%x for TSin1 ! \n", "MERGE0", ErrCode));
            RetOK = FALSE;
        }
        /* Connect to PTI */
        ErrCode = STMERGE_Connect(STMERGE_TSIN_1, STMERGE_PTI_0);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("%s connect TSIN_1 to PTI_0 error 0x%x for TSin1 ! \n", "MERGE0", ErrCode));
            RetOK = FALSE;
        }
    #endif /* end SETUP_TSIN1 */

#endif /* defined(ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109) */

#if defined(WA_PTI_CLOCK_142MHZ)
    /* configure FS1 to have 142Mhz on pti clock */
    *(U32*)0xb9000010 = 0x0000c0de;
    *(U32*)0xb9000070 = 0x0000001b;
    *(U32*)0xb9000074 = 0x00007a98;
    *(U32*)0xb9000078 = 0x00000001;
    *(U32*)0xb900007c = 0x00000000;
    *(U32*)0xb9000010 = 0x0000c1a0;
#endif /* WA_PTI_CLOCK_142MHZ */

    /* --- PTI initialization --- */

    STTBX_Print(( "STPTI initializing, \trevision=%-21.21s ...", STPTI_GetRevision() ));
    memset((void *)&STPTIInitParams, 0, sizeof(STPTI_InitParams_t));

    STPTIInitParams.Device                  = DeviceType ;
    STTBX_Print(("Device adress      %x " , DeviceAddress[DeviceNb])) ;
    STPTIInitParams.TCDeviceAddress_p       = (STPTI_DevicePtr_t) DeviceAddress[DeviceNb];
#ifdef STVID_DIRECTV
    STPTIInitParams.TCCodes                 = STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV;
    STPTIInitParams.DiscardSyncByte         = FALSE; /*  FALSE for DTV with P.I., TRUE for DTV with tuner */
#else
    STPTIInitParams.TCCodes                 = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
    /* No DiscardSyncByte member in STPTI_InitParams_t in DVB */
#endif /* STVID_DIRECTV */
    STPTIInitParams.DescramblerAssociation  = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;

#if !defined ST_OSLINUX
    STPTIInitParams.TCLoader_p              = TCLoader_p;
#endif
    STPTIInitParams.Partition_p             = DriverPartition_p;
    STPTIInitParams.NCachePartition_p       = NCachePartition_p;
    strcpy( STPTIInitParams.EventHandlerName, STEVT_DEVICE_NAME);
    STPTIInitParams.EventProcessPriority    = 1;
    STPTIInitParams.InterruptProcessPriority = 14;
#if defined(STPTI_CAROUSEL_SUPPORT)
    STPTIInitParams.CarouselProcessPriority = 0;
    STPTIInitParams.CarouselEntryType       = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif
    STPTIInitParams.InterruptLevel          = PTI_INTERRUPT_LEVEL;
    STPTIInitParams.SectionFilterOperatingMode = SectionFilterOperatingMode;
#if defined(STPTI_INDEX_SUPPORT)
    STPTIInitParams.IndexAssociation        = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitParams.IndexProcessPriority    = 0;
#endif
#if defined(ST_5528)
    InterruptNumber[0]                      = ST5528_PTIA_INTERRUPT ;
#endif
#if defined(ST_5301)
    InterruptNumber[0]                      = ST5301_PTI_INTERRUPT ;
#endif
#if defined(ST_5301)
    InterruptNumber[0]                      = ST5525_PTI_INTERRUPT ;
#endif
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    InterruptNumber[0]                      = PTI_INTERRUPT ;
#endif

    STPTIInitParams.InterruptNumber         = InterruptNumber[DeviceNb];
    STPTIInitParams.SyncLock                = SyncLock;
    STPTIInitParams.SyncDrop                = SyncDrop;

#if defined (DVD_TRANSPORT_STPTI4)
    STPTIInitParams.StreamID                = STPTI_STREAM_ID_NOTAGS; /* take all non tagged data by-passed from TS Merger */
    #if defined(SETUP_TSIN0)
    STPTIInitParams.StreamID                = STPTI_STREAM_ID_TSIN0; /* receive tagged data added by TS Merger */
    #endif
    #if defined(SETUP_TSIN1)
    STPTIInitParams.StreamID                = STPTI_STREAM_ID_TSIN1; /* receive tagged data added by TS Merger */
    #endif
    STPTIInitParams.NumberOfSlots           = 8;
    STPTIInitParams.AlternateOutputLatency  = 42;
    STPTIInitParams.DiscardOnCrcError       = FALSE;
    #if defined(SETUP_TSIN0_BYPASS)
    STPTIInitParams.PacketArrivalTimeSource = STPTI_ARRIVAL_TIME_SOURCE_PTI;
    #else
    STPTIInitParams.PacketArrivalTimeSource = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
    #endif
#endif

#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)

 #ifdef ST_OSLINUX
    STPTIInitParams.Device                     = 0; /* The driver knows */
    STPTIInitParams.TCLoader_p                 = NULL; /* The driver knows */
    STPTIInitParams.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_16x8;
 #else  /* LINUX */
    STPTIInitParams.TCDeviceAddress_p          = (U32 *)PTI_BASE_ADDRESS;
    STPTIInitParams.EventProcessPriority       = 12;
    STPTIInitParams.InterruptProcessPriority   = 12;
    STPTIInitParams.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_32x8;
 #endif /* ST_OSLINUX */

 #if defined(SETUP_TSIN0_BYPASS)
    STPTIInitParams.SyncLock                   = 3;
    STPTIInitParams.SyncDrop                   = 5;
 #else
    STPTIInitParams.SyncLock                   = 0;
    STPTIInitParams.SyncDrop                   = 0;
 #endif

 #ifdef STVID_DIRECTV
    STPTIInitParams.TCCodes                 = STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV;
 #else
    STPTIInitParams.TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
 #endif
    STPTIInitParams.DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
 #ifdef STPTI_CAROUSEL_SUPPORT
    STPTIInitParams.CarouselEntryType          = STPTI_CAROUSEL_ENTRY_TYPE_TIMED;
    STPTIInitParams.CarouselProcessPriority    = STPTI_CAROUSEL_TASK_PRIORTY;
 #endif
 #ifdef STPTI_INDEX_SUPPORT
    STPTIInitParams.IndexAssociation           = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitParams.IndexProcessPriority       = STPTI_INDEX_TASK_PRIORTY;
 #endif
 #if defined(SETUP_TSIN0_BYPASS)
    STPTIInitParams.StreamID                   = STPTI_STREAM_ID_NOTAGS;
 #else
    STPTIInitParams.StreamID                   = TsinNb;
 #endif
    STPTIInitParams.NumberOfSlots              = 10;
    STPTIInitParams.AlternateOutputLatency     = 0;
    STPTIInitParams.DiscardOnCrcError          = TRUE;

 #ifdef ST_OSLINUX
  #if defined(SETUP_TSIN0_BYPASS)
    STPTIInitParams.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_PTI;
  #else
    STPTIInitParams.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
   /* Can be STPTI_ARRIVAL_TIME_SOURCE_PTI in this mode too */
   /* STPTIInitParams.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_PTI;*/
  #endif  /* SETUP_TSIN0_BYPASS */
 #else
    STPTIInitParams.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
 #endif  /*  LINUX */

    strcpy(STPTIInitParams.EventHandlerName, STEVT_DEVICE_NAME);
#endif  /* defined(ST_7100) || defined(ST_7109) || defined(ST_7200) */

    ErrCode = STPTI_Init(PTIDeviceName[DeviceNb], &STPTIInitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s init. error 0x%x ! ", PTIDeviceName[DeviceNb], ErrCode));
        RetOK = FALSE;
    }
    else
    {
        STTBX_Print(("%s Initialized, \trevision=%-21.21s\n", PTIDeviceName[DeviceNb], STPTI_GetRevision()));
        if (PTICMD_DeviceTypeEventErrorsCapable==TRUE)
        {
            PTI_Subscribe_Error_Events(DeviceNb);
        }
    }

    /* --- Event from clkrv setting --- */

    if ((RetOK) && (Evt_Handle_Clkrv == 0))
    {
        ErrCode = STEVT_Open(STEVT_DEVICE_NAME, &STEVT_OpenParams, &Evt_Handle_Clkrv);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STEVT_Open for STCLKRV subscription failed. Error 0x%x ! ", ErrCode));
            RetOK = FALSE;
        }
        else
        {
            /* The CLKRV events must not be signaled */
            PCRValidCheck = FALSE;

            /* callback subcription */
            DevSubscribeParams.SubscriberData_p = NULL;
            DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) CallBack_STCLKRV_PCR_VALID_EVT ;

            ErrCode = STEVT_SubscribeDeviceEvent(Evt_Handle_Clkrv,
                                                         STCLKRV_DEVICE_NAME,
                                                         (STEVT_EventConstant_t) STCLKRV_PCR_VALID_EVT,
                                                         &DevSubscribeParams);

            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("Failed to subscribe STCLKRV_PCR_VALID_EVT  !!\n"));
            }
            else
            {
                STTBX_Print(("STCLKRV_PCR_VALID_EVT subcribed\n"));
            }

            DevSubscribeParams.SubscriberData_p = NULL;
            DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) CallBack_STCLKRV_PCR_DISCONTINUITY_EVT ;

            ErrCode = STEVT_SubscribeDeviceEvent(Evt_Handle_Clkrv,
                                                         STCLKRV_DEVICE_NAME,
                                                         (STEVT_EventConstant_t) STCLKRV_PCR_DISCONTINUITY_EVT,
                                                         &DevSubscribeParams);

            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("Failed to subscribe STCLKRV_PCR_DISCONTINUITY_EVT  !!\n"));
            }
            else
            {
                STTBX_Print(("STCLKRV_PCR_DISCONTINUITY_EVT subcribed\n"));
            }

            DevSubscribeParams.SubscriberData_p = NULL;
            DevSubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) CallBack_STCLKRV_PCR_GLITCH_EVT ;

            ErrCode = STEVT_SubscribeDeviceEvent(Evt_Handle_Clkrv,
                                                         STCLKRV_DEVICE_NAME,
                                                         (STEVT_EventConstant_t) STCLKRV_PCR_GLITCH_EVT,
                                                         &DevSubscribeParams);

            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("Failed to subscribe STCLKRV_PCR_GLITCH_EVT  !!\n"));
            }
            else
            {
                STTBX_Print(("STCLKRV_PCR_GLITCH_EVT subcribed\n"));
            }
        }
    }

    /* --- Connection opening --- */

    if (RetOK)
    {
        memset(&STPTIOpenParams,0,sizeof(STPTI_OpenParams_t));
        STPTIOpenParams.DriverPartition_p = DriverPartition_p;
        STPTIOpenParams.NonCachedPartition_p = NCachePartition_p;

        ErrCode = STPTI_Open(PTIDeviceName[DeviceNb], &STPTIOpenParams, &STPTIHandle[DeviceNb]);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STPTI_Open(%s,hndl=%d) : failed ! error=%d ",
                         PTIDeviceName[DeviceNb], STPTIHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            STTBX_Print(("                                 %s opened as device %d, hndl=%d\n",
                     PTIDeviceName[DeviceNb], DeviceNb, STPTIHandle[DeviceNb]));
        }
    }

    /* --- STPTI Frontend configuration --- */
#if defined(ST_7200)
    if(RetOK)
    {
        /* Configure the TSIN3 via I2C */
        /* The IC22 is programmed via the IC23 (addr=0x4E) via the I2C bus */
        if ( TsinNb == STPTI_STREAM_ID_TSIN3 )
        {
            STI2C_InitParams_t STI2C_InitParams;
            static STPIO_InitParams_t   PioInitParams[8];

            char clr_string[]   = {0x00, 0x00};
            char set_string[]   = {0xFD, 0x7F};

            U32                 ActLen;
            STI2C_OpenParams_t  OpenParams;

            /* Get clock info structure - for global use  */
            (void)ST_GetClockInfo(&ST_ClockInfo);

            volatile unsigned int *transport_config0 = (volatile unsigned int *)(ST7200_CFG_BASE_ADDRESS+0x100);

            *transport_config0 = 0x003BABF0;

            memset(&STI2C_InitParams, '\0', sizeof(STI2C_InitParams_t) );

            /* Back I2C bus */
            STI2C_InitParams.BaseAddress       = (U32 *)SSC_4_BASE_ADDRESS;
            STI2C_InitParams.InterruptNumber   = SSC_4_INTERRUPT;
            STI2C_InitParams.InterruptLevel    = 10;
            STI2C_InitParams.PIOforSDA.BitMask = PIO_BIT_7;
            STI2C_InitParams.PIOforSCL.BitMask = PIO_BIT_6;
            STI2C_InitParams.MasterSlave       = STI2C_MASTER;
            STI2C_InitParams.BaudRate          = STI2C_RATE_NORMAL;
            STI2C_InitParams.MaxHandles        = 5;
            STI2C_InitParams.ClockFrequency    = ST_ClockInfo.CommsBlock;
            STI2C_InitParams.DriverPartition   = DriverPartition_p;
            strcpy(STI2C_InitParams.PIOforSDA.PortName, "PIO7");
            strcpy(STI2C_InitParams.PIOforSCL.PortName, "PIO7");

            ErrCode = STI2C_Init("I2C_BACK", &STI2C_InitParams);
            if(ErrCode != ST_NO_ERROR)
            {
                    STTBX_Print(("Failed to initialize I2C : Error code = %d\n", ErrCode));
            } else
                STTBX_Print(("Initialize I2C done.\n"));

            OpenParams.I2cAddress        = 0x4E; /* IC23 address */
            OpenParams.AddressType       = STI2C_ADDRESS_7_BITS;
            OpenParams.BusAccessTimeOut  = 100000;

            ErrCode=STI2C_Open("I2C_BACK", &OpenParams, &I2CHndl);
            if(ErrCode != ST_NO_ERROR)
            {
                    STTBX_Print(("Failed to open back I2C driver : Error code = %d\n", ErrCode));
            } else
                STTBX_Print(("Open I2C done.\n"));

            /* Clear */
            ErrCode=STI2C_Write(I2CHndl, clr_string, 2, 1000, &ActLen);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print (("Unable to initialize IC23\n"));
            }
            /* Set */
            ErrCode=STI2C_Write(I2CHndl, set_string, 2, 1000, &ActLen);
            if(ErrCode != ST_NO_ERROR)
            {
                STTBX_Print (("Unable to set IC23\n"));
            }
            if(ErrCode == ST_NO_ERROR)
                STTBX_Print (("Write successfully 2 bytes via I2C to IC23\n"));
        }

        memset(&STPTIFrontendParams,0,sizeof(STPTI_FrontendParams_t));

        ErrCode = STPTI_FrontendAllocate(STPTIHandle[DeviceNb], &STPTI_FrontendHandle, STPTI_FRONTEND_TYPE_TSIN);
        if (ErrCode != ST_NO_ERROR)
        {
            printf ("Failed to allocate STPTI Frontend object  !!\n" );
        }
        else
        {
            printf ("STPTI Frontend object allocated\n");
        }

        if ( TsinNb == STPTI_STREAM_ID_TSIN3 )
            STPTIFrontendParams.u.TSIN.SerialNotParallel = FALSE;
        else
            STPTIFrontendParams.u.TSIN.SerialNotParallel = TRUE;
        STPTIFrontendParams.u.TSIN.AsyncNotSync      = FALSE;
        STPTIFrontendParams.u.TSIN.AlignByteSOP      = TRUE;
        STPTIFrontendParams.u.TSIN.InvertTSClk       = FALSE;
        STPTIFrontendParams.u.TSIN.IgnoreErrorInByte = TRUE;
        STPTIFrontendParams.u.TSIN.IgnoreErrorInPkt  = TRUE;
        STPTIFrontendParams.u.TSIN.IgnoreErrorAtSOP  = TRUE;
        STPTIFrontendParams.u.TSIN.InputBlockEnable  = TRUE;
        STPTIFrontendParams.u.TSIN.MemoryPktNum      = 32;

        STPTIFrontendParams.PktLength =  STPTI_FRONTEND_PACKET_LENGTH_DVB;

        ErrCode = STPTI_FrontendSetParams(STPTI_FrontendHandle, &STPTIFrontendParams);
        if (ErrCode != ST_NO_ERROR)
        {
            printf ("Failed to set STPTI Frontend parameters !!\n" );
        }
        else
        {
            printf ("STPTI Frontend parameters successfully set\n");
        }

        ErrCode = STPTI_FrontendLinkToPTI(STPTI_FrontendHandle, STPTIHandle[DeviceNb]);
        if (ErrCode != ST_NO_ERROR)
        {
            printf ("Failed to link STPTI Frontend to PTI !!\n" );
        }
        else
        {
            printf ("Link STPTI Frontend to PTI done successfully\n");
        }
    }

#endif /* #if defined(ST_7200) */

#ifdef USE_STFAE_CorrectPTI
    if (RetOK)
    {
        ErrCode = STFAE_CorrectPTI(STPTIHandle[DeviceNb], 0x200);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STFAE_CorrectPTI(hndl=%d) : failed ! error=%d ", STPTIHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
    }
#endif /* ST_7100 */

    /* --------------------- */
    /* --- Slots opening --- */
    /* --------------------- */

    /* Allocation for VIDEO */
#ifdef STVID_DIRECTV
    SlotData.SlotType = STPTI_SLOT_TYPE_VIDEO_ES;
#else
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
#endif /* STVID_DIRECTV */
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
#ifndef STPTI_5_1_6
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE; /* ?value: not supported in DVB */
/*#ifdef STPTI_CAROUSEL_SUPPORT*/
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
/*#endif*/
#endif
#ifdef DVD_TRANSPORT_STPTI4
    #if defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_5105) || defined (ST_7710) || defined(ST_7100) \
     || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
    SlotData.SlotFlags.SoftwareCDFifo = TRUE;
    #else
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
    #endif
#endif

    /* Initialise all video slots */
    for(i=PTICMD_BASE_VIDEO; (i<PTICMD_BASE_VIDEO+PTICMD_MAX_VIDEO_SLOT) && RetOK; i++)
    {
        ErrCode = STPTI_SlotAllocate( STPTIHandle[DeviceNb], &SlotHandle[DeviceNb][i], &SlotData);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STPTI_SlotAllocate(hndl=%d) : failed for video ! error=%d ",
                         STPTIHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            /* Initialize video slot */
            ErrCode = STPTI_SlotClearPid(SlotHandle[DeviceNb][i]);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_SlotClearPid(slotHndl=%d) : failed for video ! error=%d ",
                             SlotHandle[DeviceNb][i], ErrCode));
                RetOK = FALSE;
            }
        }
    }
    if (RetOK)
    {
        STTBX_Print(("                                 %d video slot(s) ready for PTI device=%d\n",
                 PTICMD_MAX_VIDEO_SLOT, DeviceNb));
    }

#ifdef USE_AUDIO
    /* Allocation for AUDIO */
#ifdef STVID_DIRECTV
    SlotData.SlotType = STPTI_SLOT_TYPE_AUDIO_ES;
#else
    SlotData.SlotType = STPTI_SLOT_TYPE_PES;
#endif /* STVID_DIRECTV */
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
    SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
#ifndef STPTI_5_1_6
    SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
/*#ifdef STPTI_CAROUSEL_SUPPORT*/
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
/*#endif*/
#endif

#ifdef ST_OSLINUX
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
    SlotData.SlotFlags.AppendSyncBytePrefixToRawData       = FALSE;
    SlotData.SlotFlags.SoftwareCDFifo = TRUE;
#else
#ifdef DVD_TRANSPORT_STPTI4
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif
#endif /* ST_OSLINUX */

    /* Initialise all audio slots */
    for(i=PTICMD_BASE_AUDIO; (i<(PTICMD_BASE_AUDIO+PTICMD_MAX_AUDIO_SLOT)) && RetOK; i++)
    {
        ErrCode = STPTI_SlotAllocate( STPTIHandle[DeviceNb], &SlotHandle[DeviceNb][i], &SlotData);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STPTI_SlotAllocate(hndl=%d) : failed for audio ! error=%d ",
                         STPTIHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        else
        {
            ErrCode = STPTI_SlotClearPid(SlotHandle[DeviceNb][i]);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_SlotClearPid(slotHndl=%d) : failed for audio ! error=%d ",
                             SlotHandle[DeviceNb][i], ErrCode));
                RetOK = FALSE;
            }
        }
    }
    if (RetOK)
    {
        STTBX_Print(("                                 %d audio slot(s) ready for PTI device=%d\n",
                 PTICMD_MAX_AUDIO_SLOT, DeviceNb));
    }
#endif /* #ifdef USE_AUDIO */

    /* Allocation for PCR */
    SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
    SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
    SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
    SlotData.SlotFlags.StoreLastTSHeader = FALSE;
    SlotData.SlotFlags.InsertSequenceError = FALSE;
#ifndef STPTI_5_1_6
/*#ifdef STPTI_CAROUSEL_SUPPORT*/
    SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
/*#endif*/
#endif
#ifdef DVD_TRANSPORT_STPTI4
    SlotData.SlotFlags.SoftwareCDFifo = FALSE;
#endif

    /* Initialise all pcr slots */
    for(i=PTICMD_BASE_PCR; (i<(PTICMD_BASE_PCR+PTICMD_MAX_PCR_SLOT)) && RetOK; i++)
    {
        ErrCode = STPTI_SlotAllocate( STPTIHandle[DeviceNb], &SlotHandle[DeviceNb][i], &SlotData);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("STPTI_SlotAllocate(hndl=%d) : failed ! error=%d ",
                         STPTIHandle[DeviceNb], ErrCode));
            RetOK = FALSE;
        }
        /* Don't clear PCR ?? Why ??*/
    }
    if (RetOK)
    {
        STTBX_Print(("                                 %d pcr slot(s) ready for PTI device=%d\n",
                 PTICMD_MAX_PCR_SLOT, DeviceNb));
    }

    /* --- FIFO selection --- */
    if (RetOK)
    {
#if defined BACK_BUFFER_MODE
        /* **************************************************************************
         * intermediate buffer for VIDEO data transfer from STi5514 to STi7020 Fifo
         * **************************************************************************/
        SlotNumber = 0;
        while ( SlotNumber < PTICMD_MAX_VIDEO_SLOT )
        {
            ErrCode = STPTI_BufferAllocate( STPTIHandle[DeviceNb],
                                            PTI_VIDEO_BUFFER_SIZE, 1,
                                            &VideoBufferHandle[SlotNumber] );
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("PTI_Init() : STPTI_BufferAllocate() failure for Video slot %d !", SlotNumber ));
                RetOK = FALSE ;
            }
            else
            {
                ErrCode = STPTI_SlotLinkToBuffer(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber],
                                    VideoBufferHandle[SlotNumber]);
                if ( ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STPTI_SlotLinkToBuffer(slothndl=%d,bufferhndl=0x%x) : error=0x%x with Video slot=%d !\n",
                                SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber],
                                VideoBufferHandle[SlotNumber], ErrCode, SlotNumber));
                    RetOK = FALSE;
                }
            }
            SlotNumber++;
        } /* end while */

        if (RetOK)
        {
            STTBX_Print(("                                 %d back-buffers allocated and linked to Video slots\n",
                 SlotNumber));
        }

#ifdef USE_AUDIO
        /* **************************************************************************
         * intermediate buffer for AUDIO data transfer from STi5514 to STi7020 Fifo
         * **************************************************************************/
        SlotNumber = 0;
        while ( SlotNumber < PTICMD_MAX_AUDIO_SLOT )
        {
            ErrCode = STPTI_BufferAllocate( STPTIHandle[DeviceNb],
                                            PTI_AUDIO_BUFFER_SIZE, 1,
                                            &AudioBufferHandle[SlotNumber] );
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("PTI_Init() : STPTI_BufferAllocate() failure for Audio slot %d !", PTICMD_MAX_VIDEO_SLOT+SlotNumber ));
                RetOK = FALSE ;
            }
            else
            {
                ErrCode = STPTI_SlotLinkToBuffer(SlotHandle[DeviceNb][PTICMD_BASE_AUDIO+SlotNumber],
                                    AudioBufferHandle[SlotNumber]);
                if ( ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("STPTI_SlotLinkToBuffer(slothndl=%d,bufferhndl=0x%x) : error=0x%x with Audio slot=%d !\n",
                                SlotHandle[DeviceNb][PTICMD_BASE_AUDIO+SlotNumber],
                                AudioBufferHandle[SlotNumber], ErrCode, PTICMD_MAX_VIDEO_SLOT+SlotNumber));
                    RetOK = FALSE;
                }
            }
            SlotNumber++;
        } /* end while */

        if (RetOK)
        {
            STTBX_Print(("                                 %d back-buffers allocated and linked to Audio slots\n",
                 SlotNumber));
        }
#endif /* #ifdef USE_AUDIO */
        /* note : do not link buffer to cd fifo here, because memory injection will fail (fifo reserved for PI) */

        /* Rule : Associate slot N of to cdfifo N */
        /* Same association in case of more pti */
        /* testtool pti_xxxstart will check if ressource available */

#else /* defined BACK_BUFFER_MODE */
        /* Video Link */
#if !defined(ST_5100) && !defined(ST_5301)&& !defined(ST_5525) && !defined(ST_5105) && !defined (ST_7710) && !defined(ST_7100) \
 && !defined(ST_7109) && !defined(ST_7200) && !defined(ST_5107)
        for(i=PTICMD_BASE_VIDEO; (i<PTICMD_BASE_VIDEO+PTICMD_MAX_VIDEO_SLOT) && RetOK; i++)
        {
            VID_InjectionParams_p.Destination = VIDEO_CD_FIFO;
#if defined (ST_7015)
            if ( i==PTICMD_BASE_VIDEO+1 )
                VID_InjectionParams_p.Destination = VIDEO_CD_FIFO2;
            if ( i==PTICMD_BASE_VIDEO+2 )
                VID_InjectionParams_p.Destination = VIDEO_CD_FIFO3;
            if ( i==PTICMD_BASE_VIDEO+3 )
                VID_InjectionParams_p.Destination = VIDEO_CD_FIFO4;
            if ( i==PTICMD_BASE_VIDEO+4 )
                VID_InjectionParams_p.Destination = VIDEO_CD_FIFO5;
#endif /* ST_7015 */
#if defined (ST_7020)
 #if defined (mb290) || defined (mb295) || defined (db573)
            if ( i==PTICMD_BASE_VIDEO )
                VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO1_OFFSET);
            if ( i==PTICMD_BASE_VIDEO+1 )
                VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO2_OFFSET);
            if ( i==PTICMD_BASE_VIDEO+2 )
                VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO3_OFFSET);
            if ( i==PTICMD_BASE_VIDEO+3 )
                VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO4_OFFSET);
            if ( i==PTICMD_BASE_VIDEO+4 )
                VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO5_OFFSET);
 #endif /* mb290 || mb295 || db573 */
#endif /* ST_7020 */
#if defined (ST_5528)
            if ( i==PTICMD_BASE_VIDEO+1 )
                VID_InjectionParams_p.Destination = VIDEO_CD_FIFO2;
#endif /* ST_5528 */
#if defined(mb290) || defined (db573)
            VID_InjectionParams_p.Holdoff = 4;
            VID_InjectionParams_p.WriteLength = 1;
#elif defined (ST_5514) || defined(ST_5516) || defined(ST_5517) || defined (ST_5528)
            VID_InjectionParams_p.Holdoff = 4;
            VID_InjectionParams_p.WriteLength = 4;
#else /* ST_5514  ST_5516  ST_5517 */
            VID_InjectionParams_p.Holdoff = 1;
            VID_InjectionParams_p.WriteLength = 1;
#endif /* ST_5514 ST_5516 ST_5517 */
            #ifndef STPTI_5_1_6
#ifndef db573
            VID_InjectionParams_p.CDReqLine = STPTI_CDREQ_VIDEO;
#else /* not db573 */
            /* As handcheck is not connected between STEM7020   */
            /* and STi5517, no CD_REQ to initialize.            */
            VID_InjectionParams_p.CDReqLine = STPTI_CDREQ_UNUSED;
#endif /* not db573 */
            #endif
#ifdef DVD_TRANSPORT_STPTI4
            VID_InjectionParams_p.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
#endif

            ErrCode = STPTI_SlotLinkToCdFifo(SlotHandle[DeviceNb][i], &VID_InjectionParams_p);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_SlotLinkToCdFifo(slothndl=%d,&fifoparam=0x%x) : failed ! device=%d slot=%d destination=0x%x error=%d\n",
                             SlotHandle[DeviceNb][i], &VID_InjectionParams_p,
                             DeviceNb, i, VID_InjectionParams_p.Destination, ErrCode));
                RetOK = FALSE;
            }
        }
#endif /* !ST_5100 && !ST_7710 && !ST_7100 &&!ST_7109 && !ST_7200 */

#ifdef USE_AUDIO
#if !defined(ST_5100) && !defined(ST_5301)&& !defined(ST_5525) && !defined(ST_5105) && !defined(ST_7710) && !defined(ST_7100) \
 && !defined(ST_7109) && !defined(ST_7200) && !defined(ST_5107)
        /* Audio Link */
        for(i=PTICMD_BASE_AUDIO; (i<PTICMD_BASE_AUDIO+PTICMD_MAX_AUDIO_SLOT) && RetOK; i++)
        {
            VID_InjectionParams_p.Destination = AUDIO_CD_FIFO;
#if defined (ST_7020)
            VID_InjectionParams_p.Destination = STI7020_BASE_ADDRESS+ST7020_AUDIO_CD_FIFO1_OFFSET;
#endif
            VID_InjectionParams_p.Holdoff = 31;
            VID_InjectionParams_p.WriteLength = 1;
            #ifndef STPTI_5_1_6
            VID_InjectionParams_p.CDReqLine = STPTI_CDREQ_AUDIO;
            #endif
#ifdef DVD_TRANSPORT_STPTI4
            VID_InjectionParams_p.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
#endif
            ErrCode = STPTI_SlotLinkToCdFifo(SlotHandle[DeviceNb][i], &VID_InjectionParams_p);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("STPTI_SlotLinkToCdFifo(device=%d,slot=%d) : failed for audio error=%d ",
                             DeviceNb, i, ErrCode));
                RetOK = FALSE;
            }
        }
#endif
#endif /* #ifdef USE_AUDIO */
#endif /* BACK_BUFFER_MODE */
    }

    if (!RetOK)
    {
        STTBX_Print((": FAILED !\n"));
    }

    PCRValidFlag_p = semaphore_create_fifo_timeout(0);
    PCRInvalidFlag_p = semaphore_create_fifo_timeout(0);

    return(RetOK);
} /* end of PTI_Init() */

#if defined BACK_BUFFER_MODE
/*-------------------------------------------------------------------------
 * Function : PTI_LinkBufferToFifo
 *            Link the back-buffer X to the CD Fifo Y
 * Assumption: back-buffer X is associated to slot X
 *             X=0,1,2... Y=1,2,...
 * Input    : buffer number
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_LinkBufferToFifo(U32 SlotNumber, U32 FifoNumber)
{
    BOOL                            RetErr;
    STPTI_DMAParams_t               VID_InjectionParams_p;
    ST_ErrorCode_t                  ErrCode;

    RetErr = FALSE;
    VID_InjectionParams_p.Destination = VIDEO_CD_FIFO;
    #ifndef STPTI_5_1_6
#ifndef db573
    VID_InjectionParams_p.CDReqLine = STPTI_CDREQ_VIDEO;
#else  /* not db573 */
    /* As handcheck is not connected between STEM7020   */
    /* and STi5517, no CD_REQ to initialize.            */
    VID_InjectionParams_p.CDReqLine = STPTI_CDREQ_UNUSED;
#endif /* not db573 */
    #endif
#if defined (ST_7015)
    if ( FifoNumber==1 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO1;
    if ( FifoNumber==2 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO2;
    if ( FifoNumber==3 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO3;
    if ( FifoNumber==4 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO4;
    if ( FifoNumber==5 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO5;
#endif /* ST_7015 */
#if (defined (ST_5528) && !defined (ST_7020))
    if ( FifoNumber==1 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO1;
    if ( FifoNumber==2 )
        VID_InjectionParams_p.Destination = VIDEO_CD_FIFO2;
#endif /* ST_7015 */

#if defined (ST_7020)
 #if defined (mb290) || defined (mb295) || defined (db573) || defined (mb376)
    if ( FifoNumber==1 )
        VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO1_OFFSET);
    if ( FifoNumber==2 )
        VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO2_OFFSET);
    if ( FifoNumber==3 )
        VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO3_OFFSET);
    if ( FifoNumber==4 )
        VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO4_OFFSET);
    if ( FifoNumber==5 )
        VID_InjectionParams_p.Destination = (STI7020_BASE_ADDRESS+ST7020_VIDEO_CD_FIFO5_OFFSET);
 #endif /* mb290 || mb295 || db573 */
#if defined (ST_5528)
    if ( FifoNumber==6 )
        VID_InjectionParams_p.Destination = ST5528_VIDEO_CD_FIFO1_BASE_ADDRESS;
    if ( FifoNumber==7 )
        VID_InjectionParams_p.Destination = ST5528_VIDEO_CD_FIFO2_BASE_ADDRESS;
#endif /* ST_7015 */
#endif /* ST_7020 */

#if defined (mb314)
    VID_InjectionParams_p.Holdoff = 4;
    VID_InjectionParams_p.WriteLength = 4;
#elif defined(mb290) || defined (db573)
    VID_InjectionParams_p.Holdoff = 4;
    VID_InjectionParams_p.WriteLength = 1;
#else
    VID_InjectionParams_p.Holdoff = 1;
    VID_InjectionParams_p.WriteLength = 1;
#endif
#ifdef DVD_TRANSPORT_STPTI4
    VID_InjectionParams_p.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
#endif

    ErrCode = STPTI_BufferLinkToCdFifo(VideoBufferHandle[SlotNumber], &VID_InjectionParams_p);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_BufferLinkToCdFifo(bufferhndl=0x%x,&fifoparam=0x%x) : failed ! \n   Video slot=%d fifo=%d destination=0x%x error=0x%x\n",
                    VideoBufferHandle[SlotNumber], &VID_InjectionParams_p,
                    SlotNumber, FifoNumber, VID_InjectionParams_p.Destination, ErrCode));
         RetErr = TRUE;
    }

    return(RetErr);

} /* end of PTI_LinkBufferToFifo() */

/*-------------------------------------------------------------------------
 * Function : PTI_UnlinkBufferToFifo
 *            Unlink back-buffer with CD Fifo (allow memory injection again)
 * Assumption: back-buffer N is associated to slot N
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_UnlinkBufferToFifo(U32 SlotNumber)
{
    BOOL            RetErr;
    ST_ErrorCode_t  ErrCode;

    RetErr = FALSE;
    ErrCode = STPTI_BufferUnLink(VideoBufferHandle[SlotNumber]);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_BufferUnLinkToVID_Injection(bufferhndl=0x%x) : Video slot=%d error=0x%x !\n",
                    VideoBufferHandle[SlotNumber],
                    SlotNumber, ErrCode));
         RetErr = TRUE;
    }

    return(RetErr);

} /* end of PTI_UnlinkBufferToFifo() */



#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : PTI_AudioLinkBufferToFifo
 *            Link the back-buffer X to the CD Fifo Y
 * Assumption: back-buffer X is associated to slot X
 *             X=0,1,2... Y=1,2,...
 * Input    : buffer number
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioLinkBufferToFifo(U32 SlotNumber, U32 FifoNumber)
{
    BOOL                            RetErr;
    STPTI_DMAParams_t               VID_InjectionParams_p;
    ST_ErrorCode_t                  ErrCode;

    RetErr = FALSE;
    VID_InjectionParams_p.Destination = AUDIO_CD_FIFO;
#if defined (ST_7020)
    VID_InjectionParams_p.Destination = STI7020_BASE_ADDRESS+ST7020_AUDIO_CD_FIFO1_OFFSET;
#endif
    #ifndef STPTI_5_1_6
    VID_InjectionParams_p.CDReqLine = STPTI_CDREQ_AUDIO;
    #endif

#if defined (mb314)
    VID_InjectionParams_p.Holdoff = 4;
    VID_InjectionParams_p.WriteLength = 4;
#elif defined(mb290) || defined (db573)
    VID_InjectionParams_p.Holdoff = 4;
    VID_InjectionParams_p.WriteLength = 1;
#else
    VID_InjectionParams_p.Holdoff = 1;
    VID_InjectionParams_p.WriteLength = 1;
#endif
#ifdef DVD_TRANSPORT_STPTI4
    VID_InjectionParams_p.BurstSize = STPTI_DMA_BURST_MODE_ONE_BYTE;
#endif

    ErrCode = STPTI_BufferLinkToCdFifo(AudioBufferHandle[SlotNumber], &VID_InjectionParams_p);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_BufferLinkToCdFifo(bufferhndl=0x%x,&fifoparam=0x%x) : failed ! \n   Audio slot=%d fifo=%d destination=0x%x error=0x%x\n",
                    AudioBufferHandle[SlotNumber], &VID_InjectionParams_p,
                    PTICMD_MAX_VIDEO_SLOT+SlotNumber, FifoNumber, VID_InjectionParams_p.Destination, ErrCode));
         RetErr = TRUE;
    }

    return(RetErr);

} /* end of PTI_AudioLinkBufferToFifo() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioUnlinkBufferToFifo
 *            Unlink back-buffer with CD Fifo (allow memory injection again)
 * Assumption: back-buffer N is associated to slot N
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioUnlinkBufferToFifo(U32 SlotNumber)
{
    BOOL            RetErr;
    ST_ErrorCode_t  ErrCode;

    RetErr = FALSE;
    ErrCode = STPTI_BufferUnLink(AudioBufferHandle[SlotNumber]);
    if ( ErrCode != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_BufferUnLinkToVID_Injection(bufferhndl=0x%x) : Audio slot=%d error=0x%x !\n",
                    AudioBufferHandle[SlotNumber],
                    PTICMD_MAX_VIDEO_SLOT+SlotNumber, ErrCode));
         RetErr = TRUE;
    }

    return(RetErr);

} /* end of PTI_AudioUnlinkBufferToFifo() */
#endif /* #ifdef USE_AUDIO */

#endif /* BACK_BUFFER_MODE */


#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : PTI_AudioStop
 *            Stop sending current audio program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioStop(U32 DeviceNb, U32 SlotNumber)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    RetErr = TRUE;
    if(SlotNumber < PTICMD_MAX_AUDIO_SLOT)
    {
        /* Stop previous audio DMA's */
        ErrCode = STPTI_SlotClearPid(SlotHandle[DeviceNb][PTICMD_BASE_AUDIO+SlotNumber]);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("PTI_AudioStop(device=%d,slot=%d) : failed ! err=%d\n",
                         DeviceNb, PTICMD_MAX_VIDEO_SLOT+SlotNumber, ErrCode));
        }
        else
        {
            RetErr = FALSE;
            STTBX_Print(("PTI_AudioStop(device=%d,slot=%d) : done\n",
                         DeviceNb, PTICMD_MAX_VIDEO_SLOT+SlotNumber));
        }
    }
    else
    {
        STTBX_Print(("PTI_AudioStop(device=%d,slot=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, PTICMD_MAX_VIDEO_SLOT+SlotNumber, PTICMD_MAX_AUDIO_SLOT));
    }
#if defined BACK_BUFFER_MODE
    RetErr |= PTI_AudioUnlinkBufferToFifo( SlotNumber) ;
#endif
    return(RetErr);
} /* end of PTI_AudioStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_AudioStart
 *            Select/Start sending an audio program
 * Input    : audio PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTI_AudioStart(ProgId_t PCR_PID, ProgId_t AUD_PID, U32 DeviceNb)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;
    U32 SlotNumber = 0;

    RetErr = TRUE;

    if(SlotNumber < PTICMD_MAX_AUDIO_SLOT)
    {
#if defined BACK_BUFFER_MODE
        RetErr = PTI_AudioLinkBufferToFifo(SlotNumber, SlotNumber+1) ;
#endif

        ErrCode = STPTI_SlotSetPid(SlotHandle[DeviceNb][PTICMD_BASE_AUDIO+SlotNumber], AUD_PID);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("PTI_AudioStart(device=%d,slot=%d,pid=%d): failed ! error=%d\n",
                         DeviceNb, PTICMD_MAX_VIDEO_SLOT+SlotNumber, AUD_PID, ErrCode));
        }
        else
        {
            STTBX_Print(("PTI_AudioStart(device=%d,slot=%d,pid=%d): done\n",
                         DeviceNb, PTICMD_MAX_VIDEO_SLOT+SlotNumber, AUD_PID ));
            RetErr = FALSE;
        }
    }
    else
    {
        STTBX_Print(("PTI_AudioStart(device=%d,slot=%d,pid=%d) : Slot number too high. max=%d !!\n",
                     STPTIHandle[DeviceNb], PTICMD_MAX_VIDEO_SLOT+SlotNumber, AUD_PID, PTICMD_MAX_AUDIO_SLOT));
    }

    return(RetErr);

} /* end of PTI_AudioStart() */
#endif /* #ifdef USE_AUDIO */



/*-------------------------------------------------------------------------
 * Function : PTI_VideoStop
 *            Stop sending current video program
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStop(U32 DeviceNb, U32 SlotNumber, BOOL Verbose)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    /* Stops video DMA's */
    RetErr = TRUE;

    if(SlotNumber < PTICMD_MAX_VIDEO_SLOT)
    {
        ErrCode = STPTI_SlotClearPid(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber]);
        if ( ErrCode != ST_NO_ERROR )
        {
            if (Verbose)
            {
                STTBX_Print(("PTI_VideoStop(device=%d,slot=%d): failed ! err=%d\n",
                             DeviceNb, SlotNumber, ErrCode));
            }
        }
        else
        {
            if (Verbose)
            {
                STTBX_Print(("PTI_VideoStop(device=%d,slot=%d) : done\n",
                             DeviceNb, SlotNumber));
            }
            RetErr = FALSE;
            /* Caution: return from STPTI_SlotClearPid() is immediate, but there can
               still be transfer of last packect going on. So wait a little bit. */
            /* at least this should be done for the STVID_INJECTION_BREAK_WORKAROUND */
            task_delay(MaxDelayForLastPacketTransferWhenStptiPidCleared);
        }
    }
    else
    {
        STTBX_Print(("PTI_VideoStop(device=%d,slot=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, PTICMD_MAX_VIDEO_SLOT));
    }
#if defined BACK_BUFFER_MODE
        /* allow Fifo to be filled from back_buffer */
        RetErr |= PTI_UnlinkBufferToFifo( SlotNumber) ;
#endif
    return(RetErr);
} /* end of PTI_VideoStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_VideoStart
 *            Select/Start sending a video program
 * Input    : video PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStart(ProgId_t PID, U32 DeviceNb, U32 SlotNumber, BOOL Verbose)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    RetErr = TRUE;

    if(SlotNumber < PTICMD_MAX_VIDEO_SLOT)
    {
        /* --- Inject video according to the PID and Fifo --- */

#if defined BACK_BUFFER_MODE
        /* allow Fifo N+1 to be filled from back_buffer N */
        /* slot 0 --> buffer 0 --> fifo 1 --> "VID1" (decoder 0) */
        /* slot 1 --> buffer 1 --> fifo 2 --> "VID2" (decoder 1) */
        /* etc... */
        RetErr = PTI_LinkBufferToFifo(SlotNumber, SlotNumber+1) ;
#endif
        ErrCode = STPTI_SlotSetPid(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber], PID);
        if ( ErrCode != ST_NO_ERROR)
        {
            if (Verbose)
            {
                STTBX_Print(("PTI_VideoStart(device=%d,slot=%d,pid=%d) : failed ! error=%d\n",
                             DeviceNb, SlotNumber, PID, ErrCode));
            }
        }
        else
        {
            if (Verbose)
            {
                STTBX_Print(("PTI_VideoStart(device=%d,slot=%d,pid=%d) : done\n",
                             DeviceNb, SlotNumber, PID ));
            }
            RetErr = FALSE;
        }
    }
    else
    {
        STTBX_Print(("PTI_VideoStart(device=%d,slot=%d,pid=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, PID, PTICMD_MAX_VIDEO_SLOT));
    }
    return(RetErr);
} /*  end of PTI_VideoStart() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRStop
 *            Definition of the macros
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_PCRStop(U32 DeviceNb, U32 SlotNumber)
{
    BOOL RetErr;
    ST_ErrorCode_t ErrCode;

    RetErr = TRUE;

    if(SlotNumber < PTICMD_MAX_PCR_SLOT)
    {
        ErrCode = STPTI_SlotClearPid(SlotHandle[DeviceNb][PTICMD_BASE_PCR+SlotNumber]);
        if ( ErrCode != ST_NO_ERROR )
        {
            STTBX_Print(("PTI_PCRStop(device=%d,slot=%d) : failed ! error=%d \n",
                         DeviceNb, SlotNumber, ErrCode));
        }
        else
        {
            STTBX_Print(("PTI_PCRStop(device=%d,slot=%d) : done\n",
                         DeviceNb, SlotNumber));
            RetErr = FALSE;
       }
    }
    else
    {
        STTBX_Print(("PTI_PCRStop(device=%d,slot=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, PTICMD_MAX_PCR_SLOT));
    }
    return(RetErr);
} /* end of PTI_PCRStop() */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRStart
 *            Select the PCR
 * Input    : PCR PID
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTI_PCRStart(ProgId_t PID, U32 DeviceNb, U32 SlotNumber, ST_DeviceName_t ClkrvName)
{
    BOOL                    RetErr;
#ifdef USE_CLKRV
    ST_ErrorCode_t          ErrCode;
    STCLKRV_SourceParams_t  SourceParams;
    STCLKRV_ExtendedSTC_t   ExtendedSTC;
    ST_DeviceName_t         ClkrvDevName_Associated;
    STCLKRV_Handle_t        ClkrvHdl;
    STCLKRV_OpenParams_t    stclkrv_OpenParams;
    osclock_t               PCRTimeout;
#endif

    RetErr = FALSE;
#ifdef USE_CLKRV
    ClkrvHdl=0;

    if(SlotNumber < PTICMD_MAX_PCR_SLOT)
    {
        strcpy(ClkrvDevName_Associated, ClkrvName);
        ErrCode = STCLKRV_Open(ClkrvDevName_Associated, &stclkrv_OpenParams, &ClkrvHdl);
        if ( ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("PTI_PCRStart(device=%d) : STCLKRV_Open(devname=%s) failed. Err=0x%x !\n",
                            DeviceNb, ClkrvDevName_Associated, ErrCode));
            RetErr = TRUE;
            ClkrvHdl=0;
        }
        else
        {
            /* Invalidate the PCR synchronization on PCR channel change */
            STCLKRV_InvDecodeClk(ClkrvHdl);

            ErrCode = STPTI_SlotSetPid(SlotHandle[DeviceNb][PTICMD_BASE_PCR+SlotNumber], PID);
            if ( ErrCode != ST_NO_ERROR )
            {
                STTBX_Print(("----> PTI_PCRStart(device=%d,slot=%d,pid=%d) : failed error=%d!\n",
                            DeviceNb, SlotNumber, PID, ErrCode));
                RetErr = TRUE;
            }
            else
            {
                STTBX_Print(("PTI_PCRStart(device=%d,slot=%d,pid=%d) : done\n",
                            DeviceNb, SlotNumber, PID));

#if defined (ST_7100) || defined(ST_7109)
                ErrCode = STCLKRV_SetApplicationMode( ClkrvHdl, STCLKRV_APPLICATION_MODE_NORMAL );
                if (ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("PTI_PCRStart(device=%d) : STCLKRV_SetApplicationMode (devname=%s) failed. Err=0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                    RetErr = TRUE;
                }
#endif

                SourceParams.Source = STCLKRV_PCR_SOURCE_PTI;
                SourceParams.Source_u.STPTI_s.Slot = SlotHandle[DeviceNb][PTICMD_BASE_PCR+SlotNumber];

                ErrCode = STCLKRV_SetPCRSource(ClkrvHdl, &SourceParams );
                if (ErrCode != ST_NO_ERROR)
                {
                    STTBX_Print(("PTI_PCRStart(device=%d) : STCLKRV_SetPCRSource(devname=%s) failed. Err=0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                    RetErr = TRUE;
                }

                if (!(RetErr))
                {
                    ErrCode = STCLKRV_SetSTCSource(ClkrvHdl, STCLKRV_STC_SOURCE_PCR);
                    if (ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("PTI_PCRStart(device=%d) : STCLKRV_SetSTCSource(devname=%s) failed. Err=0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                        RetErr = TRUE;
                    }
                }

#ifdef USE_AUDIO
                if (!(RetErr))
                {
                    ErrCode = STCLKRV_SetSTCOffset(ClkrvHdl, STC_AUDIO_OFFSET);
                    if (ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("Call STCLKRV_SetSTCOffset with %d offset FAILED: RetCode:%x\n", STC_AUDIO_OFFSET, ErrCode));
                        RetErr = TRUE;
                    }
                }
#endif


                /* **************************************** */
                /* Starting process of STCLKRV status check */
                /* **************************************** */

                if (!(RetErr))
                {
                    STCLKRV_Enable(ClkrvHdl);
                    if (ErrCode != ST_NO_ERROR)
                    {
                        printf("STCLKRV_Enable error:%x\n",ErrCode);
                    }

                    /* LM: Invalidate the PCR synchronization because STCLKRV_SetSTCSource always sends
                           STCLKRV_PCR_VALID_EVT events.
                           We want to be sure to catch the PCR_DISCONTINUITY_EVT, so we enable the
                           detection here.
                    */
                    while (semaphore_wait_timeout(PCRInvalidFlag_p, TIMEOUT_IMMEDIATE) == 0);
                    PCRTimeout = time_plus(time_now(), PTI_STCLKRV_PCR_DISCONTINUITY_TIMEOUT);
                    if (PCRState != PTI_STCLKRV_STATE_PCR_INVALID)
                    {
                        PCRInvalidCheck = TRUE; /* enable check  */

                        if (semaphore_wait_timeout(PCRInvalidFlag_p, &PCRTimeout) == 0)
                        {
                            STTBX_Print(("STCLKRV PCR discontinuity event\n"));
                        }
                        else
                        {
                            STTBX_Print(("STCLKRV PCR no discontinuity event !\n"));
                        }

                        PCRInvalidCheck = FALSE; /* disabled check */
                    }
                    STTBX_Print(("STCLKRV in discontinuity state\n"));

                    /* We are now sure STCLKRV is in PTI_STCLKRV_STATE_PCR_INVALID state */
                    /* We will wait and check STCLKRV_PCR_VALID_EVT during that time */
                    while (semaphore_wait_timeout(PCRValidFlag_p, TIMEOUT_IMMEDIATE) == 0);
                    PCRTimeout = time_plus(time_now(), PTI_STCLKRV_PCR_VALID_TIMEOUT);
                    if (PCRState != PTI_STCLKRV_STATE_PCR_OK)
                    {
                        PCRValidCheck = TRUE; /* enable check  */

                        if (semaphore_wait_timeout(PCRValidFlag_p, &PCRTimeout) == 0)
                        {
                            STTBX_Print(("STCLKRV PCR valid event\n"));
                        }
                        else
                        {
                            STTBX_Print(("STCLKRV PCR timeout: STCLKRV not in sync !\n"));
                        }

                        PCRValidCheck = FALSE; /* disabled check */

                    }
                    else
                    {
                        STTBX_Print(("STCLKRV in valid state\n"));
                    }
                    /* The next call should be valid if the PCR pid is valid */
                    ErrCode = STCLKRV_GetExtendedSTC(ClkrvHdl, &ExtendedSTC);
                    if (ErrCode != ST_NO_ERROR)
                    {
                        STTBX_Print(("PTI_PCRPID(device=%d) : STCLKRV_GetExtendedSTC(devname=%s) error 0x%x !\n",
                                    DeviceNb, ClkrvDevName_Associated, ErrCode));
                        RetErr = TRUE;
                    }
                    else
                    {
                        STTBX_Print(("PTI_PCRPID(device=%d) : STC=0x%x,%x\n",
                                    DeviceNb, ExtendedSTC.BaseBit32, ExtendedSTC.BaseValue));
                    }
                }
            }
            ErrCode = STCLKRV_Close(ClkrvHdl);
            if (ErrCode!=ST_NO_ERROR)
            {
                STTBX_Print(("PTI_PCRStart(device=%d) : STCLKRV_Close() failed. Err=0x%x !\n", DeviceNb, ErrCode));
            }
        }
    }
    else
    {
        STTBX_Print(("PTI_PCRStart(device=%d,slot=%d,pid=%d) : Slot number too high. max=%d !!\n",
                     DeviceNb, SlotNumber, PID, PTICMD_MAX_PCR_SLOT));
    }
#else /* not USE_CLKRV */
    STTBX_Print(("STCLKRV not in use !\n"));
#endif /* not USE_CLKRV */
    return(RetErr);

} /* end of PTI_PCRStart() */


/*#######################################################################*/
/*########################### PTI COMMANDS ##############################*/
/*#######################################################################*/

/*-------------------------------------------------------------------------
 * Function : PtiCmdErrorDeviceNumber
 *            Dsiplay error for DeviceNb parameter parsing
 * Input    : *pars_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void PtiCmdErrorDeviceNumber(parse_t *pars_p)
{
#if PTICMD_MAX_DEVICES == 1
    sprintf(PTI_Msg, "expected Device Number 0 (only one device initalized)");
#else
    sprintf(PTI_Msg, "expected Device Number (between 0 and %d)", PTICMD_MAX_DEVICES);
#endif
    STTST_TagCurrentLine(pars_p, PTI_Msg);
}

/*-------------------------------------------------------------------------
 * Function : PtiCmdErrorSlotNumber
 *            Dsiplay error for DeviceNb parameter parsing
 * Input    : *pars_p
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void PtiCmdErrorSlotNumber(parse_t *pars_p, U32 MaxSlot)
{
    if( MaxSlot == 1)
    {
        sprintf(PTI_Msg, "expected slot Number 0 (only one slot reserved)");
    }
    else
    {
        sprintf(PTI_Msg, "expected slot Number (between 0 and %d)", MaxSlot - 1);
    }
    STTST_TagCurrentLine(pars_p, PTI_Msg);
}

/*-------------------------------------------------------------------------
 * Function : PTI_InitCmd
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_InitCmd( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr;
    U32 DeviceNb;
    U32 TsinNb;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
    }
    else
    {
#if defined(ST_7200)
        RetErr = STTST_GetInteger( pars_p, 0, (S32*)&TsinNb);
        if ((RetErr) || (TsinNb >= TSIN_MAX_NUMBER ))
        {
            sprintf(PTI_Msg, "PTI_Init() : Invalid TSIN number TsinNb=%d : err=%d\n", TsinNb, RetErr);
        }
        else
        {
            RetErr = PTI_Init(DeviceNb, TsinNb);
            sprintf(PTI_Msg, "PTI_Init() : DeviceNb=%d TsinNb=%d : err=%d\n", DeviceNb, TsinNb, RetErr);
        }
#else
        RetErr = PTI_Init(DeviceNb, STPTI_STREAM_ID_TSIN0);
        sprintf(PTI_Msg, "PTI_Init() : DeviceNb=%d : err=%d\n", DeviceNb, RetErr);
#endif

        STTBX_Print((PTI_Msg));
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_InitCmd() */


/*******************************************************************************
Name        : PTI_TermComponent
Description : Terminate the STPTI driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void PTI_TermComponent(void)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U32                 DeviceNb;
    STPTI_TermParams_t  TermParams;

    DeviceNb = 0;
    TermParams.ForceTermination = TRUE;

    if (Evt_Handle_Clkrv != 0)
    {
        ErrCode = STEVT_UnsubscribeDeviceEvent(Evt_Handle_Clkrv,
                                               STCLKRV_DEVICE_NAME,
                                               (STEVT_EventConstant_t) STCLKRV_PCR_VALID_EVT);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STCLKRV errors Unsubscription failed. Error 0x%x ! ", ErrCode));
        }

        ErrCode = STEVT_UnsubscribeDeviceEvent(Evt_Handle_Clkrv,
                                               STCLKRV_DEVICE_NAME,
                                               (STEVT_EventConstant_t) STCLKRV_PCR_DISCONTINUITY_EVT);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("STCLKRV errors Unsubscription failed. Error 0x%x ! ", ErrCode));
        }

        STEVT_Close(Evt_Handle_Clkrv);
    }

    if (PTICMD_DeviceTypeEventErrorsCapable==TRUE)
    {
        ErrCode = PTI_Unsubscribe_Error_Events(DeviceNb);
    }
    ErrCode = STPTI_Term(PTIDeviceName[DeviceNb], &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s term. error 0x%x ! \n",
                        PTIDeviceName[DeviceNb], ErrCode));
    }
    else
    {
        STTBX_Print(("%s Terminated\n", PTIDeviceName[DeviceNb]));
    }

#if defined (ST_7200)
    /* Terminate PTI Frontend */
    ErrCode = STPTI_FrontendUnLink ( STPTI_FrontendHandle );
    if (ErrCode != ST_NO_ERROR)
    {
        printf ("Failed to unlink the Frontend !!\n" );
    }
    else
    {
        printf ("STPTI Frontend successfully unlinked\n");
    }

    ErrCode =  STPTI_FrontendDeallocate ( STPTI_FrontendHandle );
    if (ErrCode != ST_NO_ERROR)
    {
        printf ("Failed to deallocate the STPTI Frontend !!\n" );
    }
    else
    {
        printf ("STPTI Frontend deallocation successfully performed\n");
    }
#endif /* #if defined (ST_7200) */

#if defined(ST_5528) || defined(ST_5100) || defined(ST_5301)|| defined(ST_5525) || defined(ST_7710) || defined(ST_7100) \
 || defined(ST_7109)
    ErrCode = STMERGE_Term("MERGE0", NULL );
    STTBX_Print(("STMERGE_Term(MERGE0) : error=0x%x \n", ErrCode));
#endif
} /* end PTI_TermComponent() */


#ifdef USE_VIDEO
/*-------------------------------------------------------------------------
 * Function : PTI_VideoPid
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_VideoPID( parse_t *pars_p, char *result_sym_p )
{
    BOOL            RetErr  = FALSE;
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    U32 DeviceNb, SlotNumber;
    ProgId_t PID;
    ST_ErrorCode_t (*GetWriteAdd)(void * const ExtHandle, void ** const Address_p);
    void (*InformReadAdd)(void * const ExtHandle, void * const Address); /* These are pointers on kernel functions */

#if defined(ST_5100) || defined(ST_5301) || defined(ST_5105) || defined(ST_5525)|| defined (ST_7710) || defined(ST_7100) \
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
    void * Add;
    U32 Size;
#endif /* ST_5100 || ST_5301 || ST_7710 || ST_7100 || ST_7109 || ST_7200 */

    STTBX_Print(("VID_InjectionPTI_PID:%x\n", (U32)VID_Injection));

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_VIDEO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    STTBX_Print(("PTI_VideoPID() : VID_Injection=0x%x",(U32)VID_Injection));
    STTBX_Print((" slot=%d Access=0x%x\n",SlotNumber,(U32)(VID_Injection[SlotNumber].Access_p)));

    /* Lock injection access */
    semaphore_wait(VID_Injection[SlotNumber].Access_p);

    if((VID_Injection[SlotNumber].Type != NO_INJECTION) &&
       (VID_Injection[SlotNumber].Type != VID_LIVE_INJECTION))
    {
        VID_ReportInjectionAlreadyOccurs(SlotNumber+1);
    }
    else
    {
        if((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
           (VID_Injection[SlotNumber].Config.Live.DeviceNb != DeviceNb) &&
           (VID_Injection[SlotNumber].Config.Live.SlotNb != SlotNumber))
        {
            VID_ReportInjectionAlreadyOccurs(SlotNumber+1);
        }
        else
        {
            if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
                (VID_Injection[SlotNumber].State == STATE_INJECTION_NORMAL))
            {
                RetErr = PTI_VideoStop(DeviceNb, SlotNumber, TRUE);
                if (!(RetErr))
                {
                    VID_Injection[SlotNumber].Type = NO_INJECTION;
#if defined(ST_5100) || defined(ST_5301) || defined(ST_5525)|| defined(ST_5105) || defined (ST_7710) || defined(ST_7100) \
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
                    RetErr = STVID_DeleteDataInputInterface(VID_Injection[SlotNumber].Driver.Handle);
                    STTBX_Print(("STVID_DeleteDataInputInterface(Handle=%x,slot=%d): Ret code=%x\n",
                         VID_Injection[SlotNumber].Driver.Handle, SlotNumber, RetErr));
                    RetErr = STPTI_SlotUnLink(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber]);
                    STTBX_Print(("STPTI_SlotUnLink(device=%d,slot=%d): Ret code=%x\n",
                         DeviceNb, SlotNumber, RetErr));
                    ErrCode = STPTI_BufferDeallocate (VID_Injection[SlotNumber].HandleForPTI);
#endif /* ST_5100 || ST_5301 || ST_7710 || ST_7100 || ST_7109 || ST_7200 */
                }
            }
            if (VID_Injection[SlotNumber].Type == NO_INJECTION)
            {
#if defined(ST_5100) || defined(ST_5301) || defined(ST_5525) || defined(ST_5105) || defined (ST_7710) || defined(ST_7100) \
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
                /* No cd-fifo: Initialise a destination buffer */
                ErrCode = STVID_GetDataInputBufferParams(VID_Injection[SlotNumber].Driver.Handle, &Add, &Size);

                if (ErrCode == ST_NO_ERROR)
                {
                    VID_Injection[SlotNumber].Base_p = Add;
                    VID_Injection[SlotNumber].Top_p = (void*)((U32)Add + Size -1);

                    /* Align the write and read pointers to the beginning of the input buffer */
                    VID_Injection[SlotNumber].Write_p = VID_Injection[SlotNumber].Base_p;
                    VID_Injection[SlotNumber].Read_p  = VID_Injection[SlotNumber].Base_p;

                    STTBX_Print(("   Injection buffer: Base=0x%x Top=0x%x\n", (U32)VID_Injection[SlotNumber].Base_p, (U32)VID_Injection[SlotNumber].Top_p));

                    ErrCode = STPTI_BufferAllocateManual(STPTIHandle[DeviceNb],
                                (U8*)Add,Size,1,&VID_Injection[SlotNumber].HandleForPTI);
                }
                /* No cd-fifo: Initialise the link between pti and video */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("   Injection PTI: SlotHandle=0x%x PTIHandle=0x%x\n",SlotHandle[DeviceNb][SlotNumber],VID_Injection[SlotNumber].HandleForPTI));
                    ErrCode = STPTI_SlotLinkToBuffer(SlotHandle[DeviceNb][SlotNumber],
                            VID_Injection[SlotNumber].HandleForPTI);
                }
                if (ErrCode == ST_NO_ERROR)
                {
#ifdef ST_OSLINUX
                    /* In Linux, we need to get function addresses in kernel space */
                    /* So we ask the kernel */
                    ErrCode = STVIDTEST_GetKernelInjectionFunctionPointers(STVIDTEST_INJECT_PTI, &GetWriteAdd, &InformReadAdd);

                    STTBX_Print(("GetWriteAdd returned to kernel: %x\n", (U32)GetWriteAdd));
                    STTBX_Print(("InformReadAdd returned to kernel: %x\n", (U32)InformReadAdd));
#else
                    GetWriteAdd = GetWritePtrFct;
                    InformReadAdd = SetReadPtrFct;
#endif
                    /* Share the living pointers=register fcts */
                    if (ErrCode == ST_NO_ERROR)
                    {
                        ErrCode = STVID_SetDataInputInterface(
                                VID_Injection[SlotNumber].Driver.Handle,
                                GetWriteAdd, InformReadAdd,
                                (void * const)VID_Injection[SlotNumber].HandleForPTI);
                    }
                }
                if (ErrCode == ST_NO_ERROR)
                {
                    BOOL IgnoreOverflow=TRUE;
                    ErrCode = STPTI_BufferSetOverflowControl(VID_Injection[SlotNumber].HandleForPTI,IgnoreOverflow);
                }
#endif /* ST_5100 || ST_5301 || ST_5105 || ST_5107 || ST_7710 || ST_7100 || ST_7109 || ST_7200 */
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Print(("   Injection PTI: PID=h%x DeviceNb=%d SlotNb=%d\n",PID,DeviceNb,SlotNumber));
                    RetErr = PTI_VideoStart(PID, DeviceNb, SlotNumber, TRUE);
                }
                else
                {
                    RetErr = TRUE;
                }
                if (!(RetErr))
                {
                    VID_Injection[SlotNumber].Type = VID_LIVE_INJECTION;
                    VID_Injection[SlotNumber].Config.Live.Pid = PID;
                    VID_Injection[SlotNumber].Config.Live.SlotNb = SlotNumber;
                    VID_Injection[SlotNumber].Config.Live.DeviceNb = DeviceNb;
                }
            }
            if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
                (VID_Injection[SlotNumber].State == STATE_INJECTION_STOPPED_BY_EVT))
            {
                /* Stopped by event => Now order by user to  restart with new PID */
                VID_Injection[SlotNumber].Config.Live.Pid = PID;
            }
        }
    }
    /* Free injection access */
    semaphore_signal(VID_Injection[SlotNumber].Access_p);
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_VideoPID() */

/*-------------------------------------------------------------------------
 * Function : PTI_DisableCCControl
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_DisableCCControl( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U32 DeviceNb, SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_VIDEO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    ErrCode = STPTI_SlotSetCCControl(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber], TRUE);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s STPTI_SlotSetCCControl failed for video slot %d. error 0x%x ! \n",
                        PTIDeviceName[DeviceNb], SlotNumber, ErrCode));
    }
    else
    {
        STTBX_Print(("%s CC check DISABLED for video slot %d.\n", PTIDeviceName[DeviceNb], SlotNumber));
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return(API_EnableError ? RetErr : FALSE );
} /* end of PTI_DisableCCControl() */

/*-------------------------------------------------------------------------
 * Function : PTI_EnableCCControl
 *            Start sending the specified video program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_EnableCCControl( parse_t *pars_p, char *result_sym_p )
{
    BOOL RetErr = FALSE;
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    U32 DeviceNb, SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_VIDEO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    ErrCode = STPTI_SlotSetCCControl(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber], FALSE);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("%s STPTI_SlotSetCCControl failed for video slot %d. error 0x%x ! \n",
                        PTIDeviceName[DeviceNb], SlotNumber, ErrCode));
    }
    else
    {
        STTBX_Print(("%s CC check ENABLED for video slot %d.\n", PTIDeviceName[DeviceNb], SlotNumber));
    }
    STTST_AssignInteger(result_sym_p, ErrCode, FALSE);
    return(API_EnableError ? RetErr : FALSE );
} /* end of PTI_EnableCCControl() */
#endif /* #ifdef USE_VIDEO */

#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : PTI_AudioPID
 *            Start sending the specified audio program
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_AudioPID(parse_t *pars_p, char *result_sym_p)
{
    ProgId_t    AUD_PID,PCR_PID = 0; /* PCR not used here, already set for video first */
    BOOL        RetErr;
    U32         DeviceNb, SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&AUD_PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID for AUD" );
        RetErr = TRUE;
        goto exit;
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        RetErr = TRUE;
        goto exit;
    }

#ifdef ST_OSLINUX
    SlotNumber = 0;
#else
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr)|| (SlotNumber >= PTICMD_MAX_AUDIO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_AUDIO_SLOT);
        RetErr = TRUE;
        goto exit;
    }
#endif

#if !defined ST_OSLINUX
    if (PtiCmdAudioStarted[DeviceNb][SlotNumber])
    {
        RetErr = PTI_AudioStop(DeviceNb, SlotNumber);
    }
#endif

    if (!RetErr)
    {
        PtiCmdAudioStarted[DeviceNb][SlotNumber] = FALSE;
        RetErr = PTI_AudioStart(PCR_PID, AUD_PID, DeviceNb);
        if (!RetErr)
        {
            PtiCmdAudioStarted[DeviceNb][SlotNumber] = TRUE;
        }
    }

exit:
    return(API_EnableError ? RetErr : FALSE );

 } /* end of PTI_AudioPID() */
#endif /* #ifdef USE_AUDIO */

/*-------------------------------------------------------------------------
 * Function : PTI_PCRPID
 *            Start the selected PCR
 * Input    : *pars_p, *result_sym_p
 * Output   :
 * Return   : TRUE if ok, else FALSE
 * ----------------------------------------------------------------------*/
static BOOL PTI_PCRPID(parse_t *pars_p, char *result_sym_p)
{
    ProgId_t PID;
    BOOL RetErr;
    U32 DeviceNb, SlotNumber;
    ST_DeviceName_t ClkrvName;

    /* get PCR PID */
    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&PID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected PID" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_PCR_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_PCR_SLOT);
        return(TRUE);
    }

#ifdef USE_CLKRV
    RetErr = STTST_GetString( pars_p, STCLKRV_DEVICE_NAME, ClkrvName, sizeof(ClkrvName) );
#else
    RetErr = STTST_GetString( pars_p, "", ClkrvName, sizeof(ClkrvName) );
#endif
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected clock recovery device name" );
        return(TRUE);
    }

    if (PtiCmdPCRStarted[DeviceNb][SlotNumber])
    {
        RetErr = PTI_PCRStop(DeviceNb, SlotNumber);
    }
    if (!(RetErr))
    {
        PtiCmdPCRStarted[DeviceNb][SlotNumber] = FALSE;
        RetErr = PTI_PCRStart(PID, DeviceNb, SlotNumber, ClkrvName);
        if (!(RetErr))
        {
            PtiCmdPCRStarted[DeviceNb][SlotNumber] = TRUE;
        }
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_PCRPID() */

#ifdef USE_AUDIO
/*-------------------------------------------------------------------------
 * Function : PTI_AudioStopPID
 *            Stop the audio program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_AudioStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U32 DeviceNb, SlotNumber;

    RetErr = FALSE;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_AUDIO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_AUDIO_SLOT);
        return(TRUE);
    }

    RetErr = PTI_AudioStop(DeviceNb, SlotNumber);
    PtiCmdAudioStarted[DeviceNb][SlotNumber] = FALSE;

    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_AudioStopPID() */
#endif /* #ifdef USE_AUDIO */

#ifdef USE_VIDEO
/*-------------------------------------------------------------------------
 * Function : PTI_VideoStopPID
 *            Stop the video program
 * Input    : mode
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL PTI_VideoStopPID(parse_t *pars_p, char *result_sym_p)
{
    BOOL RetErr;
    U32 DeviceNb,SlotNumber;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_VIDEO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    /* Lock injection access */
    semaphore_wait(VID_Injection[SlotNumber].Access_p);

    if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
        (VID_Injection[SlotNumber].Config.Live.SlotNb == SlotNumber) &&
        (VID_Injection[SlotNumber].Config.Live.DeviceNb == DeviceNb))
    {
        if(VID_Injection[SlotNumber].State == STATE_INJECTION_NORMAL)
        {
            RetErr = PTI_VideoStop(DeviceNb, SlotNumber, TRUE);
            if (!(RetErr))
            {
                VID_Injection[SlotNumber].Type = NO_INJECTION;
            }
#if defined(ST_5100) || defined(ST_5301) || defined(ST_5525)|| defined(ST_5105) || defined (ST_7710) || defined(ST_7100) \
 || defined(ST_7109) || defined(ST_7200) || defined(ST_5107)
            /* Now Unlinking slot and buffer to free dmas */
            RetErr = STVID_DeleteDataInputInterface(VID_Injection[SlotNumber].Driver.Handle);
            STTBX_Print(("STVID_DeleteDataInputInterface(Handle=%x,slot=%d): Ret code=%x\n",
                         VID_Injection[SlotNumber].Driver.Handle, SlotNumber, RetErr));
            RetErr = STPTI_SlotUnLink(SlotHandle[DeviceNb][PTICMD_BASE_VIDEO+SlotNumber]);
            STTBX_Print(("STPTI_SlotUnLink(device=%d,slot=%d): Ret code=%x\n",
                         DeviceNb, SlotNumber, RetErr));
            RetErr = STPTI_BufferDeallocate(VID_Injection[SlotNumber].HandleForPTI);
            STTBX_Print(("STPTI_BufferDeallocate(device=%d,slot=%d): Ret code=%x\n",
                         DeviceNb, SlotNumber, RetErr));
#endif /* ST_5100 || ST_5105 || ST_5107 || ST_7710 || ST_7100 || ST7109 || ST_7200 || ST_5107 */
        }
        else
        {
            VID_Injection[SlotNumber].Type = NO_INJECTION;
        }
    }
    /* Free injection access */
    semaphore_signal(VID_Injection[SlotNumber].Access_p);
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );

} /* end of PTI_VideoStopPID() */
#endif /* #ifdef USE_VIDEO */

#if defined (USE_VIDEO)
/*-------------------------------------------------------------------------
 * Function : PTI_VidZapPID
 *            Zap video PID on given slot from given slot
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTI_VidZapPID(parse_t *pars_p, char *result_sym_p)
{
    ProgId_t NewPID;
    BOOL RetErr;
    U32 DeviceNb, SlotNumber;

    /* get new PID */
    RetErr = STTST_GetInteger( pars_p, 16, (S32 *)&NewPID);
    if (RetErr)
    {
        STTST_TagCurrentLine( pars_p, "expected new PID" );
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&DeviceNb);
    if ((RetErr) || (DeviceNb >= PTICMD_MAX_DEVICES))
    {
        PtiCmdErrorDeviceNumber(pars_p);
        return(TRUE);
    }

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&SlotNumber);
    if ((RetErr) || (SlotNumber >= PTICMD_MAX_VIDEO_SLOT))
    {
        PtiCmdErrorSlotNumber(pars_p, PTICMD_MAX_VIDEO_SLOT);
        return(TRUE);
    }

    /* Stopping previous PID */
    /* Lock injection access */
    semaphore_wait(VID_Injection[SlotNumber].Access_p);

    if ((VID_Injection[SlotNumber].Type == VID_LIVE_INJECTION) &&
        (VID_Injection[SlotNumber].Config.Live.SlotNb == SlotNumber) &&
        (VID_Injection[SlotNumber].Config.Live.DeviceNb == DeviceNb))
    {
        if(VID_Injection[SlotNumber].State == STATE_INJECTION_NORMAL)
        {
            RetErr = PTI_VideoStop(DeviceNb, SlotNumber, TRUE);
        }
        else
        {
            /* There is no injection: aborting */
            VID_Injection[SlotNumber].Type = NO_INJECTION;
            return(TRUE);
        }
    }
    /* Free injection access */
    semaphore_signal(VID_Injection[SlotNumber].Access_p);

    /* Setting new PID */
    RetErr = PTI_VideoStart(NewPID, DeviceNb, SlotNumber, TRUE);
    if (RetErr)
    {
        STTBX_Print(("PTI_VideoStart(device=%d,slot=%d,new PID=%d): Ret code=%x\n",
                     DeviceNb, SlotNumber, NewPID, RetErr));
        return(TRUE);
    }
    else
    {
        VID_Injection[SlotNumber].Type = VID_LIVE_INJECTION;
        VID_Injection[SlotNumber].Config.Live.Pid = NewPID;
        VID_Injection[SlotNumber].Config.Live.SlotNb = SlotNumber;
        VID_Injection[SlotNumber].Config.Live.DeviceNb = DeviceNb;
    }
    STTST_AssignInteger(result_sym_p, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
}
#endif /* USE_VIDEO || USE_AUDIO */

/*-------------------------------------------------------------------------
 * Function : PTIGetInputPacketCount
 *            Display PTI Input Packet count
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTIGetInputPacketCount( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode;
    U16 Count;
    U32 PtiNum;

    UNUSED_PARAMETER(Result);

    STTST_GetInteger( pars_p, 0, (S32*)&PtiNum );

    if ((ErrCode = STPTI_GetInputPacketCount( PTIDeviceName[PtiNum], &Count)) != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_GetInputPacketCount(%s) : Failed Error 0x%x\n", PTIDeviceName[PtiNum], ErrCode));
        return TRUE;
    }

    STTBX_Print(("STPTI_GetInputPacketCount(%s)=%d\n", PTIDeviceName[PtiNum], Count));

    return FALSE;
}

/*-------------------------------------------------------------------------
 * Function : PTIGetSlotPacketCount
 *            Display PTI Packet Error count
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTIGetSlotPacketCount( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode;
    U16 Count;
    S32 Slot;
    U32 PtiNum;

    UNUSED_PARAMETER(Result);

    STTST_GetInteger( pars_p, 0, &Slot );
    if ((Slot < 0) || (Slot >= PTICMD_MAX_SLOTS))
    {
        STTBX_Print(("Invalid slot (0..%d)\n", PTICMD_MAX_SLOTS-1));
        return TRUE;
    }

    STTST_GetInteger( pars_p, 0, (S32*)&PtiNum );

    if ((ErrCode = STPTI_SlotPacketCount( SlotHandle[PtiNum][Slot], &Count)) != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_SlotPacketCount(%d,%s)= Failed Error 0x%x\n",
                     Slot, PTIDeviceName[PtiNum], ErrCode ));
        return TRUE;
    }

    STTBX_Print(("STPTI_GetSlotPacketCount(%d,%s)=%d\n", Slot, PTIDeviceName[PtiNum], Count ));
    return FALSE;
}

#ifndef ST_OSLINUX
#if defined (STPTI_DVB_SUPPORT) || !defined (DVD_TRANSPORT_STPTI4)
/*-------------------------------------------------------------------------
 * Function : PTIGetPacketErrorCount
 *            Display PTI Packet Error count
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTIGetPacketErrorCount( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode;
    unsigned int Count;
    U32 PtiNum;

    UNUSED_PARAMETER(Result);

    STTST_GetInteger( pars_p, 0, (S32*)&PtiNum );

    if ((ErrCode = STPTI_GetPacketErrorCount( PTIDeviceName[PtiNum], &Count)) != ST_NO_ERROR )
    {
        STTBX_Print(("STPTI_GetPacketErrorCount(%s) : Failed. Error 0x%x\n", PTIDeviceName[PtiNum], ErrCode ));
        return TRUE;
    }

    STTBX_Print(("STPTI_GetPacketErrorCount(%s)=%d\n", PTIDeviceName[PtiNum], Count ));
    return FALSE;
}
#endif

#endif /* ST_OSLINUX */

/*-------------------------------------------------------------------------
 * Function : PTIResetErrorsPTI
 *
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTIResetErrorsPTI( STTST_Parse_t *pars_p, char *Result )
{
    S32 SlotNum;

    UNUSED_PARAMETER(Result);

    if (PTICMD_DeviceTypeEventErrorsCapable==FALSE)
    {
        STTBX_Print(("STPTI Error events are only available with PTI3 or PTI4 device !\n"));
        return FALSE;
    }

    STTST_GetInteger( pars_p, -1, &SlotNum );
    if ((SlotNum < -1) || (SlotNum >= PTICMD_MAX_SLOTS))
    {
        STTBX_Print(("Invalid slot. Type (0..%d) or -1 for all\n", PTICMD_MAX_SLOTS-1));
        return TRUE;
    }
    if (SlotNum==-1)
    {
        int i;
        for (i = 0; i < PTICMD_MAX_SLOTS; i++)
        {
            Sections_Discarded_On_Crc_Check_Errors[i]=0;
            Buffer_Overflow_Errors[i]=0;
            CC_Errors[i]=0;
            Packet_Errors[i]=0;
            TC_Code_Fault_Errors[i]=0;
        }
        Total_Stpti_Errors=0;
        STTBX_Print(("STPTI error events count reset for all slots\n"));
    }
    else
    {
        Total_Stpti_Errors-=Sections_Discarded_On_Crc_Check_Errors[SlotNum];
        Sections_Discarded_On_Crc_Check_Errors[SlotNum]=0;
        Total_Stpti_Errors-=Buffer_Overflow_Errors[SlotNum];
        Buffer_Overflow_Errors[SlotNum]=0;
        Total_Stpti_Errors-=CC_Errors[SlotNum];
        CC_Errors[SlotNum]=0;
        Total_Stpti_Errors-=Packet_Errors[SlotNum];
        Packet_Errors[SlotNum]=0;
        Total_Stpti_Errors-=TC_Code_Fault_Errors[SlotNum];
        TC_Code_Fault_Errors[SlotNum]=0;
        STTBX_Print(("STPTI error events count reset for slot %d\n", SlotNum));
    }

    return FALSE;
}

/*-------------------------------------------------------------------------
 * Function : PTIShowErrorsPTI
 *
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL PTIShowErrorsPTI( STTST_Parse_t *pars_p, char *Result )
{
    S32 SlotNum;

    UNUSED_PARAMETER(Result);

    if (PTICMD_DeviceTypeEventErrorsCapable==FALSE)
    {
        STTBX_Print(("STPTI Error events are only available with PTI3 or PTI4 device !\n"));
        return FALSE;
    }

    STTST_GetInteger( pars_p, -1, &SlotNum );
    if ((SlotNum < -1) || (SlotNum >= PTICMD_MAX_SLOTS))
    {
        STTBX_Print(("Invalid slot. Type (0..%d) or -1 for all\n", PTICMD_MAX_SLOTS-1));
        return TRUE;
    }

    STTBX_Print(("\tCrc_Errors\tBuf_Overflow\tCC_Errors\tPkt_Errors\tTC_Code_Fault\n"));
    if (SlotNum==-1)
    {
        for (SlotNum = 0; SlotNum < PTICMD_MAX_SLOTS; SlotNum++)
        {
           STTBX_Print(("Slot%2d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", SlotNum,
                Sections_Discarded_On_Crc_Check_Errors[SlotNum],
                Buffer_Overflow_Errors[SlotNum],
                CC_Errors[SlotNum],
                Packet_Errors[SlotNum],
                TC_Code_Fault_Errors[SlotNum]
            ));
        }
        STTBX_Print(("Total_Stpti_Errors = %d\n", Total_Stpti_Errors));

    }
    else
    {
        STTBX_Print(("Slot%2d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n", SlotNum,
            Sections_Discarded_On_Crc_Check_Errors[SlotNum],
            Buffer_Overflow_Errors[SlotNum],
            CC_Errors[SlotNum],
            Packet_Errors[SlotNum],
            TC_Code_Fault_Errors[SlotNum]
        ));
    }
#ifdef USE_AUDIO
        STTBX_Print(("First video slot = %d, first audio slot = %d, first PCR slot = %d\n",
                PTICMD_BASE_VIDEO, PTICMD_BASE_AUDIO, PTICMD_BASE_PCR));
#else
        STTBX_Print(("First video slot = %d, first PCR slot = %d\n",
                PTICMD_BASE_VIDEO, PTICMD_BASE_PCR));
#endif

    return FALSE;
}

#ifdef STVID_INJECTION_BREAK_WORKAROUND
 /*-------------------------------------------------------------------------
 * Function : PTI_GetVideoDeviceNumber
 * Input    : Video device base address
 * Output   : PTI used for this video
 * Return   : -1 if Device Base Address not found
 * ----------------------------------------------------------------------*/
static S32 PTI_GetVideoDeviceNumber(U32* VideoDeviceBaseAdd_p)
{
    S32 DeviceNb;

    /* As we associated slot N to cd fifo N in the init of PTI */
    switch((U32)VideoDeviceBaseAdd_p)
    {

#if defined ST_5510 || defined ST_5512 || \
    defined ST_5508 || defined ST_5518 || defined ST_5578 || \
    defined ST_5514 || defined ST_5516 || defined ST_5517 || defined ST_5528
        case VIDEO_BASE_ADDRESS:
            DeviceNb = 0;
            break;
#endif/* ST_5510 ST_5512 ST_5508 ST_5518 ST_5578 ST_5514 ST_5516 ST_5517 */
#if defined ST_7015
        case ST7015_VID1_OFFSET:
            DeviceNb = 0;
            break;
        case ST7015_VID2_OFFSET:
            DeviceNb = 1;
            break;
        case ST7015_VID3_OFFSET:
            DeviceNb = 2;
            break;
        case ST7015_VID4_OFFSET:
            DeviceNb = 3;
            break;
        case ST7015_VID5_OFFSET:
            DeviceNb = 4;
            break;
#elif defined ST_7020
        case ST7020_VID1_OFFSET:
            DeviceNb = 0;
            break;
        case ST7020_VID2_OFFSET:
            DeviceNb = 1;
            break;
        case ST7020_VID3_OFFSET:
            DeviceNb = 2;
            break;
        case ST7020_VID4_OFFSET:
            DeviceNb = 3;
            break;
        case ST7020_VID5_OFFSET:
            DeviceNb = 4;
            break;
#endif /* ST_7020 */
        default :
            DeviceNb = -1;
            break;
    }
    return(DeviceNb);
}

 /*-------------------------------------------------------------------------
 * Function : PTI_VideoWorkaroundPleaseStopInjection
              Decoder Soft reset must be done with video injection stopped
              Should be called by decode HAL just before Soft reseting
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void PTI_VideoWorkaroundPleaseStopInjection(
    STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData,
    const void *SubscriberData_p)
{
    ST_ErrorCode_t RetErr;
    S32 DeviceNb;

    if(Reason != CALL_REASON_NOTIFY_CALL)
    {
        return;
    }

    DeviceNb = PTI_GetVideoDeviceNumber((U32*)SubscriberData_p);
    /* If unknown decoder exit */
    if(DeviceNb != -1)
    {
        /* Lock injection access */
        semaphore_wait(VID_Injection[DeviceNb].Access_p);

        if(VID_Injection[DeviceNb].Type == VID_LIVE_INJECTION)
        {
            if(VID_Injection[DeviceNb].State == STATE_INJECTION_NORMAL)
            {
#ifdef PTICMD_ENABLE_INJECT_WORKAROUND_TRACE
                STTBX_Print(( "Received event STVID_WORKAROUND_PLEASE_STOP_INJECTION_EVT\r\n" ));
                RetErr = PTI_VideoStop(VID_Injection[DeviceNb].Config.Live.DeviceNb,
                                       VID_Injection[DeviceNb].Config.Live.SlotNb, TRUE);
#else
                RetErr = PTI_VideoStop(VID_Injection[DeviceNb].Config.Live.DeviceNb,
                                       VID_Injection[DeviceNb].Config.Live.SlotNb, FALSE);
#endif
                if(RetErr != ST_NO_ERROR)
                {
                    STTBX_Print(("\nError 0x%x in WorkaroundPleaseStopInjection!!\n", RetErr));
                }
                VID_Injection[DeviceNb].State = STATE_INJECTION_STOPPED_BY_EVT;
            }
        }

        /* Free injection access */
        semaphore_signal(VID_Injection[DeviceNb].Access_p);
    }
}

 /*-------------------------------------------------------------------------
 * Function : PTI_VideoWorkaroundThanksInjectionCanGoOn
              Decoder Soft reset must be done with video injection stopped
              Should be called by decode HAL just after Soft reseting
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
void PTI_VideoWorkaroundThanksInjectionCanGoOn(
    STEVT_CallReason_t Reason,
    const ST_DeviceName_t RegistrantName,
    STEVT_EventConstant_t Event,
    const void *EventData,
    const void *SubscriberData_p)
{
    ST_ErrorCode_t RetErr;
    S32 DeviceNb;

    if(Reason != CALL_REASON_NOTIFY_CALL)
    {
        return;
    }

    DeviceNb = PTI_GetVideoDeviceNumber((U32*)SubscriberData_p);
    /* If unknown decoder exit */
    if(DeviceNb != -1)
    {
        /* Lock injection access */
        semaphore_wait(VID_Injection[DeviceNb].Access_p);

        if(VID_Injection[DeviceNb].Type == VID_LIVE_INJECTION)
        {
            if(VID_Injection[DeviceNb].State == STATE_INJECTION_STOPPED_BY_EVT)
            {
#ifdef PTICMD_ENABLE_INJECT_WORKAROUND_TRACE
                STTBX_Print(( "Received event STVID_WORKAROUND_THANKS_INJECTION_CAN_GO_ON_EVT\r\n" ));
                RetErr = PTI_VideoStart(VID_Injection[DeviceNb].Config.Live.Pid,
                                        VID_Injection[DeviceNb].Config.Live.DeviceNb,
                                        VID_Injection[DeviceNb].Config.Live.SlotNb, TRUE);
#else
                RetErr = PTI_VideoStart(VID_Injection[DeviceNb].Config.Live.Pid,
                                        VID_Injection[DeviceNb].Config.Live.DeviceNb,
                                        VID_Injection[DeviceNb].Config.Live.SlotNb, FALSE);
#endif
                if(RetErr != ST_NO_ERROR)
                {
                    STTBX_Print(("\nError 0x%x in WorkaroundThanksInjectionCanGoOn!!\n", RetErr));
                }
            }
            /* Evt received => back to normal */
            VID_Injection[DeviceNb].State = STATE_INJECTION_NORMAL;
        }
        /* Free injection access */
        semaphore_signal(VID_Injection[DeviceNb].Access_p);
    }
} /* end of PTI_VideoWorkaroundThanksInjectionCanGoOn() */
#endif /* STVID_INJECTION_BREAK_WORKAROUND */


#if defined (ST_5528) || defined(ST_5100) || defined(ST_5301) || defined(ST_5525)|| defined(ST_7710) || defined(ST_7100) \
 || defined(ST_7109)
static BOOL MERGE_GetStatus( STTST_Parse_t *pars_p, char *Result )
{
    static ST_ErrorCode_t ErrCode;
    STMERGE_ObjectId_t Id;
    STMERGE_Status_t Status;

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(Result);

    Id=STMERGE_TSIN_0;
    #if defined(SETUP_TSIN1)
        Id=STMERGE_TSIN_1;
    #endif
    ErrCode = STMERGE_GetStatus(Id, &Status);
    STTBX_Print(("STMERGE_GetStatus(Id=0x%x,&Status) : errcode=0x%x\n", Id, ErrCode ));
    STTBX_Print(("\tLock=%d InputOverflow=%d RAMOverflow=%d \n\tErrorPackets=%d Counter=%d DestinationId=0x%x\n",
                Status.Lock, Status.InputOverflow, Status.RAMOverflow,
                Status.ErrorPackets, Status.Counter, Status.DestinationId ));
    return(FALSE);
}
#endif

#if defined(ST_7200)
static BOOL PTIFrontEnd_GetStatus( STTST_Parse_t *pars_p, char *Result )
{
    static ST_ErrorCode_t ErrCode;
    STPTI_FrontendStatus_t          STPTIFrontendStatus;

    ErrCode = STPTI_FrontendGetStatus  ( STPTI_FrontendHandle, &STPTIFrontendStatus );

    UNUSED_PARAMETER(pars_p);
    UNUSED_PARAMETER(Result);

    STTBX_Print(("PTIFrontEnd_GetStatus(&Status) : errcode=0x%x\n", ErrCode ));
    STTBX_Print(("\tLock=%d\n\t FifoOverflow=%d\n\t BufferOverflow=%d\n\t OutOfOrderRP=%d\n\t PktOverflow=%d\n\t DMAPointerError=%d\n\t DMAOverflow=%d\n\t ErrorPackets=%d\n\t ShortPackets=%d\n\t StreamID=%d\n",
                STPTIFrontendStatus.Lock,
                STPTIFrontendStatus.FifoOverflow,
                STPTIFrontendStatus.BufferOverflow,
                STPTIFrontendStatus.OutOfOrderRP,
                STPTIFrontendStatus.PktOverflow,
                STPTIFrontendStatus.DMAPointerError,
                STPTIFrontendStatus.DMAOverflow,
                STPTIFrontendStatus.ErrorPackets,
                STPTIFrontendStatus.ShortPackets,
                STPTIFrontendStatus.StreamID));

    return(FALSE);
}

static BOOL PTIFrontEnd_DumpReg( STTST_Parse_t *pars_p, char *Result )
{
    BOOL RetErr=0;
    S32 TsinNb;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&TsinNb);
    if ((RetErr) || (TsinNb > 4))
    {
        sprintf(PTI_Msg, "PTIFrontEnd_DumpReg() : The TSIN number must be in [0;4]");
        STTBX_Print((PTI_Msg));
    }
    else
    {
        switch (TsinNb) {
#if !defined(ST_OSLINUX)
            case 1 : StfeHAL_IBRegisterDump( STPTI_STREAM_ID_TSIN0 ); break;
            case 2 : StfeHAL_IBRegisterDump( STPTI_STREAM_ID_TSIN1 ); break;
            case 3 : StfeHAL_IBRegisterDump( STPTI_STREAM_ID_TSIN2 ); break;
            case 4 : StfeHAL_IBRegisterDump( STPTI_STREAM_ID_TSIN3 ); break;
#endif
        default:
            break;
        }
        sprintf(PTI_Msg, "PTIFrontEnd_DumpReg() : TsinNb=%d : err=%d\n", TsinNb, RetErr);
        STTBX_Print((PTI_Msg));
    }
    STTST_AssignInteger(Result, RetErr, FALSE);
    return(API_EnableError ? RetErr : FALSE );
}
#endif /* defined(ST_7200) */

#if defined(USE_CLKRV) && (defined(ST_7100) || defined(ST_7109) || defined(ST_7200))
static BOOL CLKRV_SetOffset(STTST_Parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t          ErrorCode;
    BOOL                    RetErr;
    S32                     STCOffset;
    STCLKRV_OpenParams_t    stclkrv_OpenParams;
    STCLKRV_Handle_t        ClkrvHdl;

    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&STCOffset);
    if ( RetErr )
    {
        STTST_TagCurrentLine( pars_p, "expected # of 90khz" );
        return(TRUE);
    }

    ErrorCode = STCLKRV_Open(STCLKRV_DEVICE_NAME, &stclkrv_OpenParams, &ClkrvHdl);
    printf("ClkrvHdl:%x\n",ClkrvHdl);
    if ( ErrorCode == ST_NO_ERROR)
    {
        /* worst case is 50 fields/second, offset = #fields * 90khz / 50Hz */
        ErrorCode = STCLKRV_SetSTCOffset(ClkrvHdl, STCOffset);
        STTBX_Print(("Call STCLKRV_SetSTCOffset with %d offset (RetCode:%x)\n", STCOffset, ErrorCode));
        STCLKRV_Close(ClkrvHdl);
    }
    STTST_AssignInteger(result_sym_p, ErrorCode, FALSE);
    return(FALSE);

}
#endif /* defined(USE_CLKRV) && (defined(ST_7100) || defined(ST_7109) || defined(ST_7200)) */

/*-------------------------------------------------------------------------
 * Function : PTI_InitCommand
 *            Register the PTI commands
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
static BOOL PTI_InitCommand (void)
{
    BOOL RetErr;
    RetErr = FALSE;
    RetErr |= STTST_RegisterCommand( "PTI_Init", PTI_InitCmd,
                                     "<DeviceNb> Initialize a PTI device <TsinNb> Initialize a TSIN number (for 7200 only)");
#ifdef USE_AUDIO
    RetErr |= STTST_RegisterCommand( "PTI_AudStartPID", PTI_AudioPID,
                                     "<PID><DeviceNb> Start sending audio PID to audio decoder");
    RetErr |= STTST_RegisterCommand( "PTI_AudStopPID",  PTI_AudioStopPID,
                                     "<DeviceNb> Stop sending audio to audio decoder");

#endif /* #ifdef USE_AUDIO */

#ifdef USE_VIDEO
    RetErr |= STTST_RegisterCommand( "PTI_VidStartPID", PTI_VideoPID,
                                     "<PID> <DeviceNb> <Slot> Start sending video PID to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_VidStopPID",  PTI_VideoStopPID,
                                     "<DeviceNb> <Slot> Stop sending video to video decoder");
    RetErr |= STTST_RegisterCommand( "PTI_VidZapPID",  PTI_VidZapPID,
                                     "<PID> <DeviceNb> <slot> Zap to new Video PID on given slot");

    RetErr |= STTST_RegisterCommand( "PTI_DisableCCControl", PTI_DisableCCControl,
                                     "<DeviceNb> <Slot>  Disable the CC check");
    RetErr |= STTST_RegisterCommand( "PTI_EnableCCControl", PTI_EnableCCControl,
                                     "<DeviceNb> <Slot>  Enable the CC check");
#endif /* #ifdef USE_VIDEO */

    RetErr |= STTST_RegisterCommand( "PTI_PCRPID",      PTI_PCRPID,
                                     "<PID> <DeviceNb> <Slot> Start sending data PCR PID");
    RetErr |= STTST_RegisterCommand("PTI_GetInputCnt", PTIGetInputPacketCount,
                                    "<pti> Display PTI packet count");
    RetErr |= STTST_RegisterCommand("PTI_GetSlotCnt", PTIGetSlotPacketCount,
                                    "<Slot><pti> Display PTI Slot packet count");
#if !defined ST_OSLINUX
#if defined (STPTI_DVB_SUPPORT) || !defined (DVD_TRANSPORT_STPTI4)
    RetErr |= STTST_RegisterCommand("PTI_GetErrCnt", PTIGetPacketErrorCount,
                                    "<pti> Display PTI packet error count");
#endif
#endif /* ST_OSLINUX */

    RetErr |= STTST_RegisterCommand("PTI_ResetErrorsPTI", PTIResetErrorsPTI,
                                    "<Slot> Reset PTI error events count");
    RetErr |= STTST_RegisterCommand("PTI_ShowerrorsPTI", PTIShowErrorsPTI,
                                    "<Slot> Show PTI error events count");

#if defined (ST_5528) || defined(ST_5100) || defined(ST_5301) || defined(ST_5525)|| defined(ST_7710) || defined(ST_7100) \
 || defined(ST_7109)
    RetErr |= STTST_RegisterCommand("MERGE_GetStatus", MERGE_GetStatus,
                                    "Get TS Merger status");
#endif
#if defined (ST_7200)
    RetErr |= STTST_RegisterCommand("PTI_FtEdGetStatus", PTIFrontEnd_GetStatus,
                                    "Get TS Merger status");
#endif /* defined (ST_5528) */
#if defined(USE_CLKRV) && (defined(ST_7100) || defined(ST_7109) || defined(ST_7200))
    RetErr |= STTST_RegisterCommand("CLKRV_SetOffset", CLKRV_SetOffset,
                                    "<#90KHz> set stclkrv offset +/-");
#endif

#if defined (ST_7200)
    RetErr |= STTST_RegisterCommand("STFE_Regdump", PTIFrontEnd_DumpReg,
                                    "Dump IB register");
#endif /* #if defined (ST_7200) */

    return(RetErr ? FALSE : TRUE);

} /* end of PTI_InitCommand() */



/*##########################################################################*/
/*########################### EXPORTED FUNCTIONS ###########################*/
/*##########################################################################*/


 /*-------------------------------------------------------------------------
 * Function : PTI_InitComponent
 *            Initialization of PTI and its slots
 * Input    :
 * Output   :
 * Return   : TRUE if ok
 * ----------------------------------------------------------------------*/
BOOL PTI_InitComponent(void)
{
    BOOL RetOk;
    U8 i,j;

    /* Min bit rate = 1000000 bit/s ? (we have streams at 500000, but we take low risk)
       Max size of one packet in STPTI = 188 bytes ? */
    MaxDelayForLastPacketTransferWhenStptiPidCleared = (ST_GetClocksPerSecond() * 188 * 8) / 1000000;
    if (MaxDelayForLastPacketTransferWhenStptiPidCleared == 0)
    {
        MaxDelayForLastPacketTransferWhenStptiPidCleared = 1;
    }

    for (i = 0; i < PTICMD_MAX_DEVICES; i++)
    {
#ifdef USE_AUDIO
        for(j=0; j< PTICMD_MAX_AUDIO_SLOT; j++)
        {
            PtiCmdAudioStarted[i][j] = FALSE;
        }
#endif
        for(j=0; j< PTICMD_MAX_PCR_SLOT; j++)
        {
            PtiCmdPCRStarted[i][j] = FALSE;
        }
        STPTIHandle[i] = 0;
    }

    for (i = 0; i < PTICMD_MAX_SLOTS; i++)
    {
        Sections_Discarded_On_Crc_Check_Errors[i]=0;
        Buffer_Overflow_Errors[i]=0;
        CC_Errors[i]=0;
        Packet_Errors[i]=0;
        TC_Code_Fault_Errors[i]=0;
    }
    Total_Stpti_Errors=0;


#if defined(ST_7200)
    RetOk = PTI_Init(0,STPTI_STREAM_ID_TSIN3);
    RetOk = PTI_Init(1,STPTI_STREAM_ID_TSIN1);
#else
    RetOk = PTI_Init(0,STPTI_STREAM_ID_TSIN0);
#endif /* ST_7200 */
    return(RetOk);

} /* end of PTI_InitComponent() */

 /*-------------------------------------------------------------------------
 * Function : PTI_RegisterCmd
 *            Register the commands
 * Input    :
 * Output   :
 * Return   : TRUE if success, FALSE if error
 * ----------------------------------------------------------------------*/

BOOL PTI_RegisterCmd(void)
{
    BOOL RetOk;

    RetOk = PTI_InitCommand();
    if (!RetOk)
    {
        STTBX_Print(( "PTI_RegisterCmd() \t: failed ! (STPTI)\n" ));
    }
    else
    {
        STTBX_Print(( "PTI_RegisterCmd() \t: ok           %s\n", STPTI_GetRevision()));
    }
    return(RetOk);
} /* end of PTI_RegisterCmd() */


#endif /* USE_PTI */
/* End of pti_cmd.c */

