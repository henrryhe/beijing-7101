/************************************************************************
File Name   : stvtg.h

Description : Exported types and functions for VTG driver

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                     Name
----               ------------                                     ----
04 Mar 1999        Created                                           PL
 ...
01 Sep 2000        STi7015/7020 support added                       HSdLM
16 Nov 2000        Order & comment STVTG_TimingMode_t               HSdLM
18 Jan 2001        Modify init parameters & add error constants     HSdLM
21 Mar 2001        Remove old timing modes NTSC & PAL, useless      HSdLM
 *                 unsupported timing modes,
 *                 Add digital xy start in STVTG_ModeParams_t
 *                 Add Optional Configuration feature
28 Jun 2001        New DeviceType STVTG_DEVICE_TYPE_VFE, new        HSdLM
 *                 STVTG_SlaveOf_t DVP0 & DVP1, re-organize
 *                 STVTG_OptionalConfiguration_t.
20 Feb 2002        Fix DDTS GNBvd10185 "wrapper around STAPI"       HSdLM
16 Apr 2002        New DeviceType for STi7020                       HSdLM
 *                 Add STVTG_VSYNC_WITHOUT_VIDEO option
08 Jul 2002        Add optional statistics collection.              HSdLM
26 Mar 2003        Add new STVTG_SlaveOf_t for 5528                 HSdLM
16 Apr 2003        New DeviceType STVTG_DEVICE_TYPE_VFE2, new       HSdLM
 *                 STVTG_SlaveOf_t EXT0 & EXT1
02 Jun 2003        New format 1080i(1250)                           HSdLM
***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STVTG_H
#define __STVTG_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stgxobj.h"
#include "stevt.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
#define STVTG_DRIVER_ID       42
#define STVTG_DRIVER_BASE     (STVTG_DRIVER_ID << 16)

enum
{
    STVTG_VSYNC_EVT = STVTG_DRIVER_BASE
};

enum
{
    STVTG_ERROR_INVALID_MODE = STVTG_DRIVER_BASE,
    STVTG_ERROR_EVT_REGISTER,
    STVTG_ERROR_EVT_UNREGISTER,
    STVTG_ERROR_EVT_SUBSCRIBE,
    STVTG_ERROR_DENC_OPEN,
    STVTG_ERROR_CLKRV_OPEN,
    STVTG_ERROR_CLKRV_CLOSE,
    STVTG_ERROR_MAP_VTG ,
    STVTG_ERROR_MAP_CKG ,
    STVTG_ERROR_MAP_SYS,
    STVTG_ERROR_DENC_ACCESS
};


typedef enum STVTG_DeviceType_e
{
    STVTG_DEVICE_TYPE_DENC,
    STVTG_DEVICE_TYPE_VFE,  /* VFE for Video Front End */
    STVTG_DEVICE_TYPE_VFE2, /* VFE but interrupt not managed by STINTMR */
    STVTG_DEVICE_TYPE_VTG_CELL_7015,
    STVTG_DEVICE_TYPE_VTG_CELL_7020,
    STVTG_DEVICE_TYPE_VTG_CELL_7710,
    STVTG_DEVICE_TYPE_VTG_CELL_7100,
    STVTG_DEVICE_TYPE_VTG_CELL_7200
} STVTG_DeviceType_t;

typedef enum STVTG_VtgId_e
{
    STVTG_VTG_ID_1,
    STVTG_VTG_ID_2,
    STVTG_VTG_ID_3,
    STVTG_VTG_ID_DENC
} STVTG_VtgId_t;

typedef U32 STVTG_Handle_t;

typedef enum STVTG_TimingMode_e
{
    STVTG_TIMING_MODE_SLAVE,
    STVTG_TIMING_MODE_480I60000_13514,    /* NTSC 60Hz */
    STVTG_TIMING_MODE_480P60000_27027,    /* ATSC 60P */
    STVTG_TIMING_MODE_480P30000_13514,    /* ATSC 30P */
    STVTG_TIMING_MODE_480P24000_10811,    /* ATSC 24P */
    STVTG_TIMING_MODE_480I59940_13500,    /* NTSC, PAL M */
    STVTG_TIMING_MODE_480P59940_27000,    /* ATSC 60P/1.001 */
    STVTG_TIMING_MODE_480P29970_13500,    /* ATSC 30P/1.001 */
    STVTG_TIMING_MODE_480P23976_10800,    /* ATSC 24P/1.001 */
    STVTG_TIMING_MODE_480I60000_12285,    /* NTSC 60Hz square */
    STVTG_TIMING_MODE_480P60000_24570,    /* ATSC 60P square */
    STVTG_TIMING_MODE_480P30000_12285,    /* ATSC 30P square */
    STVTG_TIMING_MODE_480P24000_9828 ,    /* ATSC 24P square */
    STVTG_TIMING_MODE_480I59940_12273,    /* NTSC square, PAL M square */
    STVTG_TIMING_MODE_480P59940_24545,    /* ATSC 60P/1.001 square */
    STVTG_TIMING_MODE_480P29970_12273,    /* ATSC 30P/1.001 square  */
    STVTG_TIMING_MODE_480P23976_9818 ,    /* ATSC 24P/1.001 square */
    STVTG_TIMING_MODE_576I50000_13500,    /* PAL B,D,G,H,I,N, SECAM */
    STVTG_TIMING_MODE_576I50000_14750,    /* PAL B,D,G,H,I,N, SECAM square */
    STVTG_TIMING_MODE_1080P60000_148500,  /* SMPTE 274M #1  P60 */
    STVTG_TIMING_MODE_1080P59940_148352,  /* SMPTE 274M #2  P60 /1.001 */
    STVTG_TIMING_MODE_1080P50000_148500,  /* SMPTE 274M #3  P50 */
    STVTG_TIMING_MODE_1080I60000_74250,   /* EIA770.3 #3 I60 = SMPTE274M #4 I60 */
    STVTG_TIMING_MODE_1080I59940_74176,   /* EIA770.3 #4 I60 /1.001 = SMPTE274M #5 I60 /1.001 */
    STVTG_TIMING_MODE_1080I50000_74250,   /* SMPTE 295M #2  I50 */
    STVTG_TIMING_MODE_1080I50000_74250_1, /* SMPTE 274M   I50 */
    STVTG_TIMING_MODE_1080P30000_74250,   /* SMPTE 274M #7  P30 */
    STVTG_TIMING_MODE_1080P29970_74176,   /* SMPTE 274M #8  P30 /1.001 */
    STVTG_TIMING_MODE_1080P25000_74250,   /* SMPTE 274M #9  P25 */
    STVTG_TIMING_MODE_1080P24000_74250,   /* SMPTE 274M #10 P24 */
    STVTG_TIMING_MODE_1080P23976_74176,   /* SMPTE 274M #11 P24 /1.001 */
    STVTG_TIMING_MODE_1035I60000_74250,   /* SMPTE 240M #1  I60 */
    STVTG_TIMING_MODE_1035I59940_74176,   /* SMPTE 240M #2  I60 /1.001 */
    STVTG_TIMING_MODE_720P60000_74250,    /* EIA770.3 #1 P60 = SMPTE 296M #1 P60 */
    STVTG_TIMING_MODE_720P59940_74176,    /* EIA770.3 #2 P60 /1.001= SMPTE 296M #2 P60 /1.001 */
    STVTG_TIMING_MODE_720P30000_37125,    /* ATSC 720x1280 30P */
    STVTG_TIMING_MODE_720P29970_37088,    /* ATSC 720x1280 30P/1.001 */
    STVTG_TIMING_MODE_720P24000_29700,    /* ATSC 720x1280 24P */
    STVTG_TIMING_MODE_720P23976_29670,    /* ATSC 720x1280 24P/1.001 */
    STVTG_TIMING_MODE_1080I50000_72000,   /* AS 4933-1 200x 1080i(1250) */
    STVTG_TIMING_MODE_720P50000_74250,
    STVTG_TIMING_MODE_576P50000_27000,  /*Australian mode*/
    STVTG_TIMING_MODE_1152I50000_48000,
    STVTG_TIMING_MODE_480P60000_25200,   /*VGA modes*/
	STVTG_TIMING_MODE_480P59940_25175,
    STVTG_TIMING_MODE_768P75000_78750,
    STVTG_TIMING_MODE_768P60000_65000,
    STVTG_TIMING_MODE_768P85000_94500,
    STVTG_TIMING_MODE_COUNT             /* must stay last */
} STVTG_TimingMode_t;

typedef enum STVTG_SlaveOf_e
{
    STVTG_SLAVE_OF_PIN,
    STVTG_SLAVE_OF_REF,
    STVTG_SLAVE_OF_SDIN1,
    STVTG_SLAVE_OF_SDIN2,
    STVTG_SLAVE_OF_HDIN,
    STVTG_SLAVE_OF_DVP0,
    STVTG_SLAVE_OF_DVP1,
    STVTG_SLAVE_OF_EXT0,
    STVTG_SLAVE_OF_EXT1
} STVTG_SlaveOf_t;

typedef enum STVTG_DencSlaveOf_e
{
    STVTG_DENC_SLAVE_OF_ODDEVEN,
    STVTG_DENC_SLAVE_OF_VSYNC,
    STVTG_DENC_SLAVE_OF_SYNC_IN_DATA
} STVTG_DencSlaveOf_t;

typedef enum STVTG_SlaveMode_e
{
    STVTG_SLAVE_MODE_H_AND_V,
    STVTG_SLAVE_MODE_V_ONLY,
    STVTG_SLAVE_MODE_H_ONLY
} STVTG_SlaveMode_t;

typedef enum STVTG_ExtVSyncShape_e
{
    STVTG_EXT_VSYNC_SHAPE_BNOTT,
    STVTG_EXT_VSYNC_SHAPE_PULSE
} STVTG_ExtVSyncShape_t;

typedef enum STVTG_Option_e
{
    STVTG_OPTION_EMBEDDED_SYNCHRO,
    STVTG_OPTION_NO_OUTPUT_SIGNAL,
    STVTG_OPTION_HSYNC_POLARITY,
    STVTG_OPTION_VSYNC_POLARITY,
    STVTG_OPTION_OUT_EDGE_POSITION
} STVTG_Option_t;

typedef enum STVTG_VSYNC_e
{
    STVTG_TOP,
    STVTG_BOTTOM
} STVTG_VSYNC_t;

/* Exported Types --------------------------------------------------------- */

typedef struct STVTG_Capability_s
{
    U32                            TimingMode;
    U32                            TimingModesAllowed2;
    BOOL                           UserModeCapable;
} STVTG_Capability_t;

typedef struct STVTG_ModeParams_s
{
    U32                            FrameRate;
    STGXOBJ_ScanType_t             ScanType;
    U32                            ActiveAreaWidth;
    U32                            ActiveAreaHeight;
    U32                            ActiveAreaXStart;
    U32                            ActiveAreaYStart;
    U32                            DigitalActiveAreaXStart;
    U32                            DigitalActiveAreaYStart;
} STVTG_ModeParams_t;

typedef struct STVTG_SyncParams_s
{
    U32                            PixelsPerLine;
    U32                            LinesByFrame;
    U32                            PixelClock;
    BOOL                           HSyncPolarity;
    U32                            HSyncPulseWidth;
    BOOL                           VSyncPolarity;
    U32                            VSyncPulseWidth;
}  STVTG_SyncParams_t;

typedef struct STVTG_OutEdgePosition_s
{
    U32                            HSyncRising;
    U32                            HSyncFalling;
    U32                            VSyncRising;
    U32                            VSyncFalling;
} STVTG_OutEdgePosition_t;

typedef struct STVTG_OptionalConfiguration_s
{
    STVTG_Option_t                 Option;
    union
    {
        BOOL                       EmbeddedSynchro;
        BOOL                       NoOutputSignal;
        BOOL                       IsHSyncPositive;
        BOOL                       IsVSyncPositive;
        STVTG_OutEdgePosition_t    OutEdges;
    } Value;
} STVTG_OptionalConfiguration_t;

typedef struct STVTG_DencSlaveParams_s
{
    STVTG_DencSlaveOf_t            DencSlaveOf;
    BOOL                           FreeRun;
    BOOL                           OutSyncAvailable;
    U8                             SyncInAdjust;
} STVTG_DencSlaveParams_t;

typedef struct STVTG_VtgCellSlaveParams_s
{
    STVTG_SlaveOf_t                HSlaveOf;
    STVTG_SlaveOf_t                VSlaveOf;
    STVTG_VtgId_t                  HRefVtgIn;
    STVTG_VtgId_t                  VRefVtgIn;
    U32                            BottomFieldDetectMin;
    U32                            BottomFieldDetectMax;
    STVTG_ExtVSyncShape_t          ExtVSyncShape;
    BOOL                           IsExtVSyncRisingEdge;
    BOOL                           IsExtHSyncRisingEdge;
} STVTG_VtgCellSlaveParams_t;

typedef struct STVTG_SlaveModeParams_s
{
    STVTG_SlaveMode_t              SlaveMode;
    union
    {
        STVTG_VtgCellSlaveParams_t VtgCell;
        STVTG_SlaveOf_t            VfeSlaveOf;
        STVTG_DencSlaveParams_t    Denc;
    } Target;
} STVTG_SlaveModeParams_t;

typedef struct STVTG_VtgCellAccess_s
{
    void *                         BaseAddress_p;
    void *                         DeviceBaseAddress_p;
    ST_DeviceName_t                InterruptEventName;
    STEVT_EventConstant_t          InterruptEvent;
} STVTG_VtgCellAccess_t;

typedef struct STVTG_VtgCellAccess2_s
{
    void *                         BaseAddress_p;
    void *                         DeviceBaseAddress_p;
    U32                            InterruptNumber;
    U32                            InterruptLevel;

} STVTG_VtgCellAccess2_t;

typedef struct STVTG_VtgCellAccess3_s
{
    void *                         BaseAddress_p;
    void *                         DeviceBaseAddress_p;
    U32                            InterruptNumber;
    U32                            InterruptLevel;
    ST_DeviceName_t                ClkrvName;
} STVTG_VtgCellAccess3_t;


typedef struct STVTG_InitParams_s
{
    /* common parameters */
    STVTG_DeviceType_t             DeviceType;
    U32                            MaxOpen;
    ST_DeviceName_t                EvtHandlerName;
#ifdef STVTG_VSYNC_WITHOUT_VIDEO
    void *                         VideoBaseAddress_p;
    void *                         VideoDeviceBaseAddress_p;
    U32                            VideoInterruptNumber;
    U32                            VideoInterruptLevel;
#endif /* #ifdef STVTG_VSYNC_WITHOUT_VIDEO */
    /* device type dependant parameters */
    union
    {
        STVTG_VtgCellAccess_t      VtgCell;
        STVTG_VtgCellAccess2_t     VtgCell2;
        STVTG_VtgCellAccess3_t     VtgCell3;
        ST_DeviceName_t            DencName;
    } Target;


} STVTG_InitParams_t;

typedef struct STVTG_OpenParams_s
{
    U8                      NotUsed;
} STVTG_OpenParams_t;

typedef struct STVTG_TermParams_s
{
    BOOL                    ForceTerminate;
} STVTG_TermParams_t;

typedef struct
{
    U16                 Sync_ZeroLevel;
    U16                 Sync_MidLevel;
    U16                 Sync_HighLevel;
    U16                 Sync_MidLowLevel;
    U16                 Sync_LowLevel;
    U16                 Sync_YOff;
    U16                 Sync_COff;
    U16                 Sync_YMult;
    U16                 Sync_CMult;
} STVTG_SyncInLevel_t;

typedef struct
{
    STVTG_TimingMode_t   TimingMode;
    STVTG_SyncInLevel_t  Sync;
}STVTG_SyncLevel_t;

#ifdef STVTG_DEBUG_STATISTICS
typedef struct STVTG_Statistics_s
{
    U32                     CountSimultaneousTopAndBottom;
    U32                     CountVsyncItDuringNotify;
} STVTG_Statistics_t;
#endif /* STVTG_DEBUG_STATISTICS */


/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */
ST_Revision_t  STVTG_GetRevision( void );

ST_ErrorCode_t STVTG_GetCapability( const ST_DeviceName_t DeviceName,
                                    STVTG_Capability_t *  const Capability_p
                                  );

ST_ErrorCode_t STVTG_Init( const ST_DeviceName_t      DeviceName,
                           const STVTG_InitParams_t * const InitParams_p
                         );

ST_ErrorCode_t STVTG_Open( const ST_DeviceName_t      DeviceName,
                           const STVTG_OpenParams_t * const OpenParams_p,
                           STVTG_Handle_t *           const Handle_p
                         );

ST_ErrorCode_t STVTG_Close( const STVTG_Handle_t Handle);

ST_ErrorCode_t STVTG_Term( const ST_DeviceName_t      DeviceName,
                           const STVTG_TermParams_t * const TermParams_p
                         );

ST_ErrorCode_t STVTG_GetVtgId( const ST_DeviceName_t  DeviceName,
                               STVTG_VtgId_t *        const VtgId_p
                             );

ST_ErrorCode_t STVTG_SetMode( const STVTG_Handle_t      Handle,
                              const STVTG_TimingMode_t  Mode
                            );

ST_ErrorCode_t STVTG_SetOptionalConfiguration( const STVTG_Handle_t                  Handle,
                                               const STVTG_OptionalConfiguration_t * const OptionalConfiguration_p
                                             );

ST_ErrorCode_t STVTG_SetSlaveModeParams( const STVTG_Handle_t            Handle,
                                         const STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                       );

ST_ErrorCode_t STVTG_GetMode( STVTG_Handle_t       Handle,
                              STVTG_TimingMode_t * const TimingMode_p,
                              STVTG_ModeParams_t * const ModeParams_p
                            );

ST_ErrorCode_t STVTG_GetModeSyncParams( STVTG_Handle_t       Handle,
                              STVTG_SyncParams_t * const SyncParams_p
                            );

ST_ErrorCode_t STVTG_GetOptionalConfiguration( const STVTG_Handle_t            Handle,
                                               STVTG_OptionalConfiguration_t * const OptionalConfiguration_p
                                            );

ST_ErrorCode_t STVTG_GetSlaveModeParams( const STVTG_Handle_t      Handle,
                                         STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                       );

#ifdef STVTG_DEBUG_STATISTICS
ST_ErrorCode_t STVTG_GetStatistics(  const STVTG_Handle_t    Handle
                                   , STVTG_Statistics_t * const Statistics_p
                                   , BOOL ResetAll
                                  );
#endif /* #ifdef STVTG_DEBUG_STATISTICS */

ST_ErrorCode_t STVTG_CalibrateSyncLevel( const STVTG_Handle_t    Handle , STVTG_SyncLevel_t   *SyncLevel);
ST_ErrorCode_t STVTG_GetLevelSettings( const STVTG_Handle_t   Handle , STVTG_SyncLevel_t   *SyncLevel);

#ifdef ST_7020_CUT1 /* for STi7020 cut1. This define is never set so far, just to remember. */
#define WA_PixClkExtern /* Pixel Clock is generated from an external synthesizer because internal is bad */
/* function exported for STVMIX only, must not be called from outside STVMIX  */
void stvtg_SetPixelClockRatio( const STVTG_Handle_t Handle, BOOL Upsample);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVTG_H */


/* End of stvtg.h */
