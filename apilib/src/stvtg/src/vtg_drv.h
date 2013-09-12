/************************************************************************
File Name   : vtg_drv.h
 *
Description : VTG driver internal exported types and functions
 *
COPYRIGHT (C) STMicroelectronics 2003.
 *
Date               Modification                                     Name
----               ------------                                     ----
04 Mar 1999        Created                                           PL
 ...
01 Sep 2000        STi7015/7020 support added                       HSdLM
08 Feb 2001        Use new interrupt handler and new                HSdLM
 *                 Denc register access
21 Mar 2001        Add PixelClock in RegParams_t                    HSdLM
 *                 Add <CKG/C656/DHDO>BaseAddress_p,
 *                 OptionalConf, DencCellId and
 *                 HVDriveSave in stvtg_Device_t.
28 Jun 2001        Make Unit Validity checking macro global         HSdLM
 *                 Handle new options.
08 Oct 2001        Use compilation flags related to device type     HSdLM
 *                 Decrease INTERRUPT_TASK_STACK_SIZE to 512 bytes
29 Nov 2001        Add WA for ST40GX1 DVP interrupt issue           HSdLM
20 Feb 2002        Add WA_GNBvd10951 definition                     HSdLM
30 May 2002        Add STVTG_VSYNC_WITHOUT_VIDEO option             HSdLM
04 Jul 2002        Display clock for 7020 is out of Synth5          HSdLM
 *                 Remove useless mutex. Add debug statistic.
 *                 Add flag STVTG_NOTIFY_VSYNC_UNDER_IT
17 Sep 2002        Replace SYNTH5 by SYNTH2 for 7020 cut 2.0        HSdLM
 *                 ADISPCLK
27 Sep 2002        Adapt DISP_CLK for 7020, let ADISP_CLK to 72MHz  HSdLM
20 Jan 2003        Remove fix DDTS GNBvd09294                       BS
*                  Change NOTIFY_UNDER_IT into NOTIFY_IN_TASK
16 Apr 2003        Add support of STi5528 (VFE2).                   HSdLM
21 Jan 2004        Add new Clock (VIDPIX_2XCLK) for 5528            AC
13 Sep 2004        Add ST40/OS21 support                            MH
***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_DRV_H
#define __VTG_DRV_H

/* Includes --------------------------------------------------------------- */
#include "stvtg.h"
#include "stdenc.h"
#include "stos.h"

#ifdef STVTG_USE_CLKRV
#include "stclkrv.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* STVTG_DRIVER_ID = 42d = 0x02A -> 0x020A20A0. See code templates */
#define STVTG_VALID_UNIT           0x020A20A0

#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
#define INTERRUPT_TASK_STACK_SIZE  512
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

#if defined(STVTG_VOS)
#if !(defined (ST_7710)||defined (ST_7100)|| defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105))
/* Temporary: work-arounds 7015, let for 7020 */
#define WA_GNBvd06791 /* following rule must be respected : DISP_CLK <= 3 x AUX_CLK and 3 x PIX_CLK */
#define WA_GNBvd06999 /* slave mode V only do not work properly */
#define WA_GNBvd07864 /* VTG2 slave of SDIN does not work */
#endif
/* see  WA_PixClkExtern in stvtg.h */
/* ----------------------------- */
#endif

#if defined(BE_GX1)
/* Temporary: work-arounds ST40GX1 */
#define WA_GNBvd08957 /* TOP and BOTTOM interrupts at the same time, with a delay of 40 ms instead of 20 ms between them (PAL) */
#define WA_DVP_ST40GX1_HARD_INTERRUPT_BUG /* write GAM_DVP_HLL (from DVP 0) to a correct value before starting VTG */
/* ----------------------------- */
#endif


/* there is more than 32 video format defined, so we must split the capability
 * variable (bitn = (is format n supported ?)) into several U32
*/
#define CAPABILITY_1_ALL           0xffffffff
#define CAPABILITY_2_ALL           ((1<<(STVTG_TIMING_MODE_COUNT-32))-1)

/* Used only in Init/Open/Close/Term functions */
/* Here only so that macro IsValidHandle works. */
#if defined(STVTG_VOS)
/* STi7015/20 : 2 VTG cell (in block Video Output Stage) + 1 for internal DENC + 1 for external DENC */
#if defined(ST_7710)||defined(ST_7100)||defined(ST_7109)||defined(ST_7111)||defined (ST_7105)
#define STVTG_MAX_DEVICE  3   /* Max number of Init() allowed */
#elif defined (ST_7200)
#define STVTG_MAX_DEVICE  6   /* Max number of Init() allowed  3 VTG + 2 DENC*/
#else
#define STVTG_MAX_DEVICE  5   /* Max number of Init() allowed */
#endif
#elif defined(STVTG_VFE) || defined (STVTG_VFE2)
/* ST40GX1 : 1 VTG cell (in block Video Front End) + 1 for internal DENC */
#define STVTG_MAX_DEVICE  2   /* Max number of Init() allowed */
#else
/* DENC */
#define STVTG_MAX_DEVICE  1   /* Max number of Init() allowed */
#endif
#ifdef WINCE_MSTV_ENHANCED
#define STVTG_MAX_OPEN    4   /* Max number of Open() allowed per Init() */
#else
#define STVTG_MAX_OPEN    3   /* Max number of Open() allowed per Init() */
#endif
#define STVTG_MAX_UNIT    (STVTG_MAX_OPEN * STVTG_MAX_DEVICE)


typedef enum STVTG_EventId_s
{
    STVTG_VSYNC_ID,
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    STVTG_VSYNC_IT_SELF_INSTALLED_ID,
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */
    STVTG_NB_REGISTERED_EVENTS  /* must stay last */
} STVTG_EventId_t;

/* Exported Types --------------------------------------------------------- */

typedef STVTG_SyncParams_t RegParams_t;

typedef struct ModeParamsLine_s
{
    STVTG_TimingMode_t             Mode;
    STVTG_ModeParams_t             ModeParams;
    STVTG_SyncParams_t             RegParams;
} ModeParamsLine_t;

#if defined(STVTG_VOS) || defined(STVTG_VFE) || defined (STVTG_VFE2)
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
typedef struct VTGTask_s
{
    task_t *                       Task_p;

    BOOL                           IsRunning;          /* TRUE if task is running, FALSE otherwise */
    BOOL                           ToBeDeleted;        /* Set TRUE to ask task to end in order to delete it */
} VTGTask_t;
#endif /* #ifdef STVTG_NOTIFY_VSYNC_IN_TASK */

typedef struct VTGItRegisters_s
{
    U32                            ITS;
    U32                            ITM;
    U32                            MASK;
    U32                            VSType[2];
} VTGItRegisters_t;
#endif /* #if defined(STVTG_VOS) || defined(STVTG_VFE) || ddefined (STVTG_VFE2)*/

typedef struct stvtg_Device_s
{
    /* Device identification */
    ST_DeviceName_t                DeviceName;

    /* Device properties set by init parameters, set at init and not modifiable afterwards */
    STVTG_DeviceType_t             DeviceType;
    void *                         BaseAddress_p;
    U32                            MaxOpen;

#ifdef ST_OSLINUX
	U32 						   VtgMappedWidth;
    U32                            CkgMappedWidth;
    U32                            SYSCFGMappedWidth;
	void *						   VtgMappedBaseAddress_p; /* Address where will be mapped the driver registers */
	void *						   VtgCKGMappedBaseAddress_p; /* Address where will be mapped the CKG driver registers */
    void *                         SYSCFGMappedBaseAddress_p; /* Address where will be mapped the PLL & FS registers */
#endif

#if defined(STVTG_VOS) || defined(STVTG_VFE) || defined (STVTG_VFE2)
    ST_DeviceName_t                InterruptEventName;
    STEVT_EventConstant_t          InterruptEvent;
#endif /* #if defined(STVTG_VOS) || defined(STVTG_VFE) || defined (STVTG_VFE2)*/

#if defined(STVTG_VFE) || defined (STVTG_VFE2) || defined (STVTG_VOS)
    U32                            InterruptNumber;
    U32                            InterruptLevel;
#endif /* #if defined(STVTG_VFE) || defined (STVTG_VFE2)*/

#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    U8 *                           VideoBaseAddress_p;
    U32                            VideoInterruptNumber;
    U32                            VideoInterruptLevel;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */

    /* Device properties induced by init parameters, set at init and not modifiable afterwards */
    STVTG_VtgId_t                  VtgId;
    U32                            CapabilityBank1;
    U32                            CapabilityBank2;
#if defined(STVTG_VOS) || defined (STVTG_VFE2)
    void *                         CKGBaseAddress_p;
    #ifdef STVTG_USE_CLKRV
    STCLKRV_Handle_t               ClkrvHandle;
    #endif
#endif /* #if defined (STVTG_VOS)|| defined (STVTG_VFE2) */
#if defined(STVTG_VOS)
    void *                         C656BaseAddress_p;
    void *                         DHDOBaseAddress_p;
#if defined (ST_7100) || defined (ST_7109)|| defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
    void *                         SYSCFGBaseAddress_p;
#endif /* ST_7100|| ST_7109|| ST_7200||ST_7111||ST_7105*/
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
   void*                           SDGlueBaseAddress_p;
   void*                           HDGlueMainBaseAddress_p;
   void*                           HDGlueAuxBaseAddress_p;
#endif /* ST_7200|| ST_7111|| ST_7105*/
#endif /* #if defined(STVTG_VOS) */
    STDENC_Handle_t                DencHandle;
    STDENC_DeviceType_t            DencDeviceType;
    U8                             DencCellId;

    STEVT_Handle_t                 EvtHandle;
    STEVT_EventID_t                EventID[STVTG_NB_REGISTERED_EVENTS];

    /* Device properties modifiable from API */
    STVTG_TimingMode_t             TimingMode;
    STVTG_SlaveModeParams_t        SlaveModeParams;
    BOOL                           EmbeddedSynchro;
    BOOL                           NoOutputSignal;
    BOOL                           IsHSyncPositive;
    BOOL                           IsVSyncPositive;
    STVTG_OutEdgePosition_t        OutEdges;

    /* Device properties for inside use, not seen from API */
    STVTG_VSYNC_t                  VSyncType;
    STVTG_Option_t                 Option;
    U8                             HVDriveSave;            /* Save of HVDRIVE value when NoOutputSignal */
    U32                            PixelsPerLine;          /* Line size */
    BOOL                           OldNoOutputSignal;
#if defined(STVTG_VOS) || defined(STVTG_VFE) || defined (STVTG_VFE2)
    U32                            InterruptMask;          /* VTG interrupt mask */
    U32                            InterruptStatus;        /* VTG interrupt status */
#ifdef STVTG_NOTIFY_VSYNC_IN_TASK
    semaphore_t                   *InterruptSemaphore_p;     /* VTG interrupt signal */
    VTGTask_t                      ItTask;                 /* interrupt task */
#endif /* STVTG_NOTIFY_VSYNC_IN_TASK */
    VTGItRegisters_t               ItReg;                  /* Interrupt control registers offsets */
    U32                            CurrentSynth3Clock;     /* last programmed SYNTH3 clock */
#ifdef WA_GNBvd06791
    U32                            CurrentSynth6Clock;     /* last programmed SYNTH6 clock */
#endif /* #ifdef WA_GNBvd06791 */
    U32                            CurrentSynth7Clock;     /* last programmed SYNTH7 clock */
#endif /* #if defined(STVTG_VOS) || defined(STVTG_VFE) || defined(STVTG_VFE2)*/
    semaphore_t                   *ModeCtrlAccess_p;         /* protect accessing mode data from API */

#ifdef WA_PixClkExtern
    BOOL                           Upsample;               /* TRUE => pixclk * 2 */
#endif /* WA_PixClkExtern */

#ifdef STVTG_DEBUG_STATISTICS
    STVTG_Statistics_t             Statistics;
#endif /* STVTG_DEBUG_STATISTICS */
} stvtg_Device_t;

typedef struct stvtg_Unit_s
{
    stvtg_Device_t *               Device_p;
    U32                            UnitValidity;
} stvtg_Unit_t;

/* Exported Variables ----------------------------------------------------- */

/* Just exported so that macro IsValidHandle works. Should never be used outside. */
extern stvtg_Unit_t stvtg_UnitArray[STVTG_MAX_UNIT];

/* Exported Macros -------------------------------------------------------- */

/* Passing a (STVTG_Handle_t), returns TRUE if the handle is valid, FALSE otherwise */
#define IsValidHandle(Handle) ((((stvtg_Unit_t *) (Handle)) >= stvtg_UnitArray) &&                    \
                               (((stvtg_Unit_t *) (Handle)) < (stvtg_UnitArray + STVTG_MAX_UNIT)) &&  \
                               (((stvtg_Unit_t *) (Handle))->UnitValidity == STVTG_VALID_UNIT))

/* STVTG_SLAVE_MODE_H_ONLY not supported up to now */
#define IS_VALID_SLAVEMODE(sm) (((sm) == STVTG_SLAVE_MODE_H_AND_V) || ((sm) == STVTG_SLAVE_MODE_V_ONLY))

/* Exported Functions ----------------------------------------------------- */
void stvtg_SignalVSYNC(STVTG_VSYNC_t Type);
#if defined (STVTG_VFE2)
void stvtg_HALSetPixelClockRst(stvtg_Device_t * const Device_p);
#endif
#if defined (STVTG_VOS)
ST_ErrorCode_t stvtg_HALSetLevelSettings(const stvtg_Device_t * const Device_p, STVTG_SyncLevel_t *SyncLevel );
ST_ErrorCode_t stvtg_HALGetLevelSettings(const stvtg_Device_t * const Device_p,  STVTG_SyncLevel_t * SyncLevel);
#endif
ModeParamsLine_t * stvtg_GetModeParamsLine( const STVTG_TimingMode_t Mode);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_DRV_H */

/* End of vtg_drv.h */
