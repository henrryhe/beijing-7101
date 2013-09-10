/*******************************************************************************

File name   : stlayer.h

Description : STLAYER API Prototypes and types

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                               Name
----               ------------                               ----
2000-05-30         Created                                    Michel Bruant
18 jan 2002        Added STLAYER_GetVTGParams()               HG
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STLAYER_H
#define __STLAYER_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "stgxobj.h"
#include "stevt.h"
#include "stcommon.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Software workarounds for hardware bugs */
/*----------------------------------------*/

/* tunable workarounds: -D<compile-key> are set in compilation command line */
/* by the makefile and depend on env variables */
/* see './makefile' */
/* see 'build options' paragraph in release note for tunable workarounds  */
/*                                              (eq non-auto workarounds) */

/* auto workarounds: depend on chips */
#ifdef HW_7015 /* st7015 workarounds */
#define WA_GNBvd06281 /* Horizontal SRC has not been implemented ...       */
#define WA_GNBvd06293 /* In interlaced mode, if a node ... height of 1 ... */
#define WA_GNBvd06299 /* In interlaced mode, if a node ... height of 1 ... */
#define WA_GNBvd06319 /* The clut update could only be done once           */
#endif

#ifdef HW_7020 /* st7020 workarounds */
#define WA_GNBvd06293 /* In interlaced mode, if a node ... height of 1 ... */
#define WA_GNBvd06299 /* In interlaced mode, if a node ... height of 1 ... */
#endif

#ifdef HW_GX1 /* st40GX1 workarounds */
#define WA_GNBycbcr             /* load YCbCr node crashes the GDP pipeline */
#define WA_taskwait_on_os20emu  /* this WA is a software bug */
#define WA_GdpResizeOnGX1       /* resize is limited */
#endif

/* Exported constants ------------------------------------------------------- */
/* #define DEBUG_RECORD_TIME */
#ifdef DEBUG_RECORD_TIME
/* Recording time in VTG interrupt */
#define NR_TIMEVAL (1024*10) /* length of the array of time values */
#endif

#define STLAYER_DRIVER_ID     158
#define STLAYER_DRIVER_BASE   (STLAYER_DRIVER_ID << 16)


/* STLAYER return codes */
enum
{
  STLAYER_ERROR_INVALID_INPUT_RECTANGLE = STLAYER_DRIVER_BASE,
  STLAYER_ERROR_INVALID_OUTPUT_RECTANGLE,
  STLAYER_ERROR_NO_FREE_HANDLES,
  STLAYER_SUCCESS_IORECTANGLES_ADJUSTED,
  STLAYER_ERROR_IORECTANGLES_NOT_ADJUSTABLE,
  STLAYER_ERROR_INVALID_LAYER_TYPE,
  STLAYER_ERROR_USER_ALLOCATION_NOT_ALLOWED,
  STLAYER_ERROR_OVERLAP_VIEWPORT,
  STLAYER_ERROR_NO_AV_MEMORY ,
  STLAYER_ERROR_OUT_OF_LAYER,
  STLAYER_ERROR_OUT_OF_BITMAP,
  STLAYER_ERROR_INSIDE_LAYER,
  STLAYER_ERROR_EVENT_REGISTRATION
};


/*  TBD to enable multiple Flicker Filter Mode */


  typedef enum STLAYER_FlickerFilterMode_e
{
    STLAYER_FLICKER_FILTER_MODE_SIMPLE,
    STLAYER_FLICKER_FILTER_MODE_ADAPTIVE,
    STLAYER_FLICKER_FILTER_MODE_USING_BLITTER
} STLAYER_FlickerFilterMode_t;



/* STLAYER exported evt */
enum
{
    STLAYER_UPDATE_PARAMS_EVT = STLAYER_DRIVER_BASE,
    STLAYER_UPDATE_DECIMATION_EVT,
    STLAYER_NEW_FMD_REPORTED_EVT
};

typedef enum STLAYER_Layer_e
{
    STLAYER_GAMMA_CURSOR,
    STLAYER_GAMMA_GDP,
    STLAYER_GAMMA_BKL,
    STLAYER_GAMMA_ALPHA,
    STLAYER_GAMMA_FILTER,
    STLAYER_OMEGA2_VIDEO1,
    STLAYER_OMEGA2_VIDEO2,
    STLAYER_7020_VIDEO1,
    STLAYER_7020_VIDEO2,
    STLAYER_OMEGA1_CURSOR,
    STLAYER_OMEGA1_VIDEO,
    STLAYER_OMEGA1_OSD,
    STLAYER_OMEGA1_STILL,
    STLAYER_SDDISPO2_VIDEO1,    /* Main SD for 5528 */
    STLAYER_SDDISPO2_VIDEO2,    /* Aux SD for 5528 */
    STLAYER_HDDISPO2_VIDEO1,    /* Main HD for 7100 */
    STLAYER_HDDISPO2_VIDEO2,    /* Aux SD for 7100/7109 */
    STLAYER_DISPLAYPIPE_VIDEO1, /* Main HD for 7109 */
    STLAYER_DISPLAYPIPE_VIDEO2, /* Main HD for 7200 */
    STLAYER_DISPLAYPIPE_VIDEO3, /* Local SD (SD0) for 7200. Unused otherwise*/
    STLAYER_DISPLAYPIPE_VIDEO4, /* Remote SD(SD1) for 7200. Unused otherwise*/
    STLAYER_COMPOSITOR,
    STLAYER_GAMMA_GDPVBI
} STLAYER_Layer_t;

typedef enum STLAYER_ViewPortSourceType_e
{
    STLAYER_GRAPHIC_BITMAP,
    STLAYER_STREAMING_VIDEO
} STLAYER_ViewPortSourceType_t;

typedef enum STLAYER_CompressionLevel_e
{
    STLAYER_COMPRESSION_LEVEL_NONE    = 1,
    STLAYER_COMPRESSION_LEVEL_1       = 2,
    STLAYER_COMPRESSION_LEVEL_2       = 4
} STLAYER_CompressionLevel_t;

typedef enum STLAYER_DecimationFactor_e
{
    STLAYER_DECIMATION_FACTOR_NONE    = 0,
    STLAYER_DECIMATION_FACTOR_2       = 1,
    STLAYER_DECIMATION_FACTOR_4       = 2,
    STLAYER_DECIMATION_MPEG4_P2       = 4,
    STLAYER_DECIMATION_AVS            = 8
} STLAYER_DecimationFactor_t;

typedef enum STLAYER_UpdateReason_e
{
    STLAYER_DISCONNECT_REASON           = 0x01,
    STLAYER_SCREEN_PARAMS_REASON        = 0x02,
    STLAYER_OFFSET_REASON               = 0x04,
    STLAYER_VTG_REASON                  = 0x08,
    STLAYER_CHANGE_ID_REASON            = 0x10,
    STLAYER_DISPLAY_REASON              = 0x20,
    STLAYER_LAYER_PARAMS_REASON         = 0x40,
    STLAYER_DECIMATION_NEED_REASON      = 0x80,
    STLAYER_IMPOSSIBLE_WITH_PROFILE     = 0x100,
    STLAYER_CONNECT_REASON              = 0x200
}STLAYER_UpdateReason_t;

typedef enum
{
    STLAYER_NORMAL_MODE,            /* Normal mode (initial one).             */
    STLAYER_SPECTACLE_MODE          /* Spectacle mode : non-linear zoom.      */
} STLAYER_OutputMode_t;

typedef enum STLAYER_DeiMode_e
{
    STLAYER_DEI_MODE_OFF = 0,
    STLAYER_DEI_MODE_AUTO,              /* not used for the moment... */
    STLAYER_DEI_MODE_BYPASS,
    STLAYER_DEI_MODE_VERTICAL,
    STLAYER_DEI_MODE_DIRECTIONAL,
    STLAYER_DEI_MODE_FIELD_MERGING,
    STLAYER_DEI_MODE_MEDIAN_FILTER,
    STLAYER_DEI_MODE_MLD,               /* (Default Mode) */
    STLAYER_DEI_MODE_LMU,

    STLAYER_DEI_MODE_MAX                /* This is not a valid mode */
} STLAYER_DeiMode_t;

/* Exported types ----------------------------------------------------------- */

typedef U32 STLAYER_Handle_t;
typedef U32 STLAYER_ViewPortHandle_t;

#ifdef DEBUG_RECORD_TIME
typedef struct STLAYER_Record_Time_s
{
    struct timeval  tv_data;
    int             where;  /* 0 For internal (to handler), 1 for external */
    int             irq_nb; /* identification of IRQ */
    unsigned long   data1; /* data1 */
    unsigned long   data2; /* data2 */
} STLAYER_Record_Time_t;
#endif


typedef enum
{
    STLAYER_TOP_FIELD,
    STLAYER_BOTTOM_FIELD
} STLAYER_FieldType_t;


typedef struct STLAYER_Field_s
{
    BOOL                   FieldAvailable;
    U32                    PictureIndex;
    STLAYER_FieldType_t    FieldType;
} STLAYER_Field_t;


typedef struct STLAYER_StreamingVideo_s
{
    U32                                 SourceNumber;
    STGXOBJ_Bitmap_t                    BitmapParams;
    STGXOBJ_ScanType_t                  ScanType;
    BOOL                                PresentedFieldInverted;
    STLAYER_CompressionLevel_t          CompressionLevel;
    /* Actual horizontal decimation factor of the picture.  */
    STLAYER_DecimationFactor_t          HorizontalDecimationFactor;
    /* Same for vertical...                                 */
    STLAYER_DecimationFactor_t          VerticalDecimationFactor;
    BOOL                                AdvancedHDecimation;
    BOOL                                IsDisplayStopped;
    BOOL                                PeriodicFieldInversionDueToFRC;

    STLAYER_Field_t                 PreviousField;
    STLAYER_Field_t                 CurrentField;
    STLAYER_Field_t                 NextField;
#ifdef  STLAYER_USE_CRC
    BOOL                                CRCCheckMode;
   /* CRCScanType is mandatory because it is not allowed to change original*/
   /* ScanType from parsed picture */
   STGXOBJ_ScanType_t                  CRCScanType;
#endif /* STLAYER_USE_CRC */
} STLAYER_StreamingVideo_t;

typedef struct STLAYER_ViewPortSource_s
{
    STLAYER_ViewPortSourceType_t    SourceType;
    union
    {
        STGXOBJ_Bitmap_t *          BitMap_p;
        STLAYER_StreamingVideo_t *  VideoStream_p;
    }                               Data;
    STGXOBJ_Palette_t *            Palette_p;
} STLAYER_ViewPortSource_t;

typedef struct STLAYER_AllocParams_s
{
    U32     ViewPortDescriptorsBufferSize;
    BOOL    ViewPortNodesInSharedMemory;
    U32     ViewPortNodesBufferSize;
    U32     ViewPortNodesBufferAlignment;
} STLAYER_AllocParams_t;

typedef struct STLAYER_Capability_s
{
    STLAYER_Layer_t LayerType;
    U32             DeviceId;
    BOOL            MultipleViewPorts;
    BOOL            HorizontalResizing;
    BOOL            AlphaBorder;
    BOOL            GlobalAlpha;
    BOOL            ColorKeying;
    BOOL            MultipleViewPortsOnScanLineCapable;
    BOOL            PSI;
    U8              FrameBufferDisplayLatency;
    U8              FrameBufferHoldTime;
} STLAYER_Capability_t;

typedef struct STLAYER_GainParams_s
{
    U8      BlackLevel;
    U8      GainLevel;
} STLAYER_GainParams_t;

typedef struct STLAYER_GlobalAlpha_s
{
    U8 A0;
    U8 A1;
} STLAYER_GlobalAlpha_t;

typedef struct STLAYER_LayerParams_s
{
    U32                     Width;
    U32                     Height;
    STGXOBJ_AspectRatio_t   AspectRatio;
    STGXOBJ_ScanType_t      ScanType;
}STLAYER_LayerParams_t;

typedef struct STLAYER_InitParams_s
{
    STLAYER_Layer_t             LayerType;
    ST_Partition_t *            CPUPartition_p;
    BOOL                        CPUBigEndian;
    /* Base address of all the registers of the chip */
    void *                      DeviceBaseAddress_p;
    /* for Omega2 chips, BaseAddress is the base address of the plane regs */
    /* and BaseAddress2 is the base address of the video dislay registers  */
    /* (DIS_XX and DIP_XX)                                                 */
    /* for Omega1 chips, BaseAddress is the base add of the video display  */
    /* regs (VID_XX)                                                       */
    void *                      BaseAddress_p;
    void *                      BaseAddress2_p;
    void *                      SharedMemoryBaseAddress_p;
    U32                         MaxHandles;
    STAVMEM_PartitionHandle_t   AVMEM_Partition;
    ST_DeviceName_t             EventHandlerName;
    ST_DeviceName_t             InterruptEventName;
    STEVT_EventConstant_t       VideoDisplayInterrupt;
    U32                         MaxViewPorts;
    void *                      ViewPortNodeBuffer_p;
    BOOL                        NodeBufferUserAllocated;
    void *                      ViewPortBuffer_p;
    BOOL                        ViewPortBufferUserAllocated;
    STLAYER_LayerParams_t *     LayerParams_p;
} STLAYER_InitParams_t;

typedef struct STLAYER_OpenParams_s
{
    U32                 Dummy;
} STLAYER_OpenParams_t;

typedef struct STLAYER_OutputParams_s
{
    STLAYER_UpdateReason_t      UpdateReason;
    STGXOBJ_AspectRatio_t       AspectRatio;
    STGXOBJ_ScanType_t          ScanType;
    U32                         FrameRate;
    U32                         Width;
    U32                         Height;
    U32                         XStart;
    U32                         YStart;
    S8                          XOffset;
    S8                          YOffset;
    ST_DeviceName_t             VTGName;
    U32                         DeviceId;
    BOOL                        DisplayEnable;
    STLAYER_Handle_t            BackLayerHandle;
    STLAYER_Handle_t            FrontLayerHandle;
    U32                         DisplayHandle;
    U8                          FrameBufferDisplayLatency;
    U8                          FrameBufferHoldTime;
}STLAYER_OutputParams_t;

/* Video filter enum access (way to access to sepcific video filter)         */
typedef enum STLAYER_VideoFiltering_s
{
    /* Please keep enum order.    */
    STLAYER_VIDEO_CHROMA_AUTOFLESH,
    STLAYER_VIDEO_CHROMA_GREENBOOST,
    STLAYER_VIDEO_CHROMA_TINT,
    STLAYER_VIDEO_CHROMA_SAT,

    STLAYER_VIDEO_LUMA_EDGE_REPLACEMENT,
    STLAYER_VIDEO_LUMA_PEAKING,
    STLAYER_VIDEO_LUMA_DCI,
    STLAYER_VIDEO_LUMA_BC,

    /* Insert new filter names here ...... */

    STLAYER_MAX_VIDEO_FILTER_POSITION /* Last position of filter enum.       */
                                    /* Please insert new one above this line.*/

} STLAYER_VideoFiltering_t;

/* Video filter enum control (action to perform to specific video filter)    */
typedef enum STLAYER_VideoFilteringControl_s
{
    /* Please keep enum order.    */

    STLAYER_DISABLE,              /* Filter is disabled.                     */
                                  /* default values will be loaded !!!       */
    STLAYER_ENABLE_AUTO_MODE1,    /* Filter is enable with auto parameters 1 */
    STLAYER_ENABLE_AUTO_MODE2,    /* Filter is enable with auto parameters 2 */
    STLAYER_ENABLE_AUTO_MODE3,    /* Filter is enable with auto parameters 3 */

    STLAYER_ENABLE_MANUAL         /* Filter is enable with parameters        */
                                  /* set by API.                             */

} STLAYER_VideoFilteringControl_t;

typedef enum
{
    STLAYER_COMPOSITION_RECURRENCE_EVERY_VSYNC,                       /* Normal mode (initial one).                                   */
    STLAYER_COMPOSITION_RECURRENCE_MANUAL_OR_VIEWPORT_PARAMS_CHANGES  /* Spectacle mode : compose viewport only when content changed. */
} STLAYER_CompositionRecurrence_t;

/* Video filters individual manual parameters.      */
typedef struct STLAYER_AutoFleshParameters_s {
    U32 AutoFleshControl;   /* percent unit of maximum effect */
    enum {
        LARGE_WIDTH,
        MEDIUM_WIDTH,
        SMALL_WIDTH
    } QuadratureFleshWidthControl;
    enum {
        AXIS_116_6,         /* 116.60 */
        AXIS_121_0,         /* 121.00 */
        AXIS_125_5,         /* 125.50 */
        AXIS_130_2          /* 130.20 */
    } AutoFleshAxisControl;
} STLAYER_AutoFleshParameters_t;

typedef struct STLAYER_GreenBoostParameters_s {
    S32 GreenBoostControl;  /* percent unit of maximum effect */
                            /* Allowed : -100% to 100%        */
} STLAYER_GreenBoostParameters_t;

typedef struct STLAYER_TintRotationControl_s {
    S32 TintRotationControl;/* percent unit of maximum effect */
                            /* Allowed : -100% to 100%        */
} STLAYER_TintParameters_t;

typedef struct STLAYER_SatParameters_s {
    S32 SaturationGainControl;  /* 1100% of gain to apply to chroma     */
                                /* -100% = Max decrease of saturation.  */
                                /*    0% = No effect.                   */
                                /*  100% = Max Increase of saturation.  */
} STLAYER_SatParameters_t;

typedef struct STLAYER_EdgeReplacementParameters_s {
    U32 GainControl;        /* percent unit of maximum effect */
    enum {
        HIGH_FREQ_FILTER = 1,
        MEDIUM_FREQ_FILTER,
        LOW_FREQ_FILTER
    } FrequencyControl;
} STLAYER_EdgeReplacementParameters_t;

typedef struct STLAYER_PeakingParameters_s {
    S32 VerticalPeakingGainControl; /* percent unit of maximum effect        */
                                    /* Allowed : -100% to 100%               */
    U32 CoringForVerticalPeaking;   /* percent unit of maximum effect        */
    S32 HorizontalPeakingGainControl;   /* percent unit of maximum effect    */
                                        /* Allowed : -100% to 100%           */
    U32 CoringForHorizontalPeaking; /* percent unit of maximum effect        */
    U32 HorizontalPeakingFilterSelection;   /* Ratio (%) = 100 * Fs/Fc       */
                                            /* with Fs : Output sample freq. */
                                            /* with Fs : Centered filter freq*/
    BOOL SINECompensationEnable;    /* Sinx/x compensation enable/disable    */
} STLAYER_PeakingParameters_t;

typedef struct STLAYER_DCIParameters{
    U32 CoringLevelGainControl;     /* percent unit of maximum effect       */
    U32 FirstPixelAnalysisWindow;
    U32 LastPixelAnalysisWindow;
    U32 FirstLineAnalysisWindow;
    U32 LastLineAnalysisWindow;
} STLAYER_DCIParameters_t;

typedef struct STLAYER_BrightnessContrastParameters_s {
    S32 BrightnessGainControl;  /*       adjust luminance intensity     */
                                /* percent unit of maximum effect    */
                                /* Allowed : -100% to 100%           */
    U32 ContrastGainControl;    /*       gain to apply to chroma     */
                                /* percent unit of maximum effect   */
                                /* Allowed : 0% to 100%          */
} STLAYER_BrightnessContrastParameters_t;

/* Video filters overall manual parameters.         */
typedef union STLAYER_VideoFilteringParameters_s
{
    STLAYER_AutoFleshParameters_t       AutoFleshParameters;
    STLAYER_GreenBoostParameters_t      GreenBoostParameters;
    STLAYER_TintParameters_t            TintParameters;
    STLAYER_SatParameters_t             SatParameters;
    STLAYER_EdgeReplacementParameters_t EdgeReplacementParameters;
    STLAYER_PeakingParameters_t         PeakingParameters;
    STLAYER_DCIParameters_t             DCIParameters;
    STLAYER_BrightnessContrastParameters_t              BCParameters;

} STLAYER_VideoFilteringParameters_t;

typedef struct STLAYER_PSI_s
{
  STLAYER_VideoFiltering_t              VideoFiltering;
  STLAYER_VideoFilteringControl_t       VideoFilteringControl;
  STLAYER_VideoFilteringParameters_t    VideoFilteringParameters;

} STLAYER_PSI_t;

typedef struct
{
    U16     NumberOfZone;           /* Number of zone to consider (according  */
                                    /* to hardware constraints).              */
    U16 *   SizeOfZone_p;           /* Size of zone in percent unit of output */
                                    /* window.                                */
    U16     EffectControl;          /* In percent unit of max possible effect.*/

} STLAYER_SpectacleModeParams_t;

typedef union
{
    STLAYER_SpectacleModeParams_t   SpectacleModeParams;    /* CF above.      */
} STLAYER_OutputWindowSpecialModeParams_t;


typedef struct STLAYER_TermParams_s
{
    BOOL ForceTerminate;
} STLAYER_TermParams_t;

typedef struct STLAYER_QualityOptimizations_s
{
    BOOL DoForceStartOnEvenLine;
    BOOL DoNotRescaleForZoomCloseToUnity;
} STLAYER_QualityOptimizations_t;


typedef struct STLAYER_ViewPortParams_s
{
    STLAYER_ViewPortSource_t *  Source_p;
    STGXOBJ_Rectangle_t         InputRectangle;
    STGXOBJ_Rectangle_t         OutputRectangle;
} STLAYER_ViewPortParams_t;


typedef enum
{
    VHSRC_FILTER_A,
    VHSRC_FILTER_B,
    VHSRC_FILTER_C,
    VHSRC_FILTER_D,
    VHSRC_FILTER_E,
    VHSRC_FILTER_F,
    VHSRC_FILTER_CHRX2
} STLAYER_VDPFilter_t;


typedef struct STLAYER_VHSRCHorizontalParams_s
{
    U32 Increment;
    U32 InitialPhase;
    U32 PixelRepeat;
} STLAYER_VHSRCHorizontalParams_t;


typedef struct STLAYER_VHSRCVerticalParams_s
{
    U32 Increment;
    U32 InitialPhase;
    U32 LineRepeat;
} STLAYER_VHSRCVerticalParams_t;


typedef struct STLAYER_VHSRCParams_s
{
   STLAYER_VHSRCHorizontalParams_t LumaHorizontal;
   STLAYER_VHSRCHorizontalParams_t ChromaHorizontal;
   STLAYER_VHSRCVerticalParams_t   LumaTop;
   STLAYER_VHSRCVerticalParams_t   ChromaTop;
   STLAYER_VHSRCVerticalParams_t   LumaBot;
   STLAYER_VHSRCVerticalParams_t   ChromaBot;
} STLAYER_VHSRCParams_t;


typedef struct STLAYER_VHSRCVideoDisplayFilters_s
{
    STLAYER_VDPFilter_t            LumaHorizontalCoefType;
    STLAYER_VDPFilter_t            ChromaHorizontalCoefType;
    STLAYER_VDPFilter_t            LumaVerticalCoefType;
    STLAYER_VDPFilter_t            ChromaVerticalCoefType;
    S8                             LumaHorizontalCoef[140];
    S8                             ChromaHorizontalCoef[140];
    S8                             LumaVerticalCoef[88];
    S8                             ChromaVerticalCoef[88];
} STLAYER_VHSRCVideoDisplayFilters_t;


typedef struct STLAYER_VideoDisplayParams_s
{
    STLAYER_VHSRCParams_t                   VHSRCparams;
    STGXOBJ_Rectangle_t                     InputRectangle;
    STGXOBJ_Rectangle_t                     OutputRectangle;
    STLAYER_DecimationFactor_t              VerticalDecimFactor;
    STLAYER_DecimationFactor_t              HorizontalDecimFactor;
    STLAYER_DeiMode_t                       DeiMode;
    STGXOBJ_ScanType_t                      SourceScanType;
    STLAYER_VHSRCVideoDisplayFilters_t      VHSRCVideoDisplayFilters;
} STLAYER_VideoDisplayParams_t;


typedef struct STLAYER_FMDParams_s
{
    U32 SceneChangeDetectionThreshold; /* SCD threshold used to define SceneCount. This is a threshold of blocks pixels differences sum. */
                                       /* Above this threshold, a block is considered as moving*/
    U32 RepeatFieldDetectionThreshold; /* RFD threshold used to define RepeatStatus. This is a threshold of blocks pixels differences sum.*/
                                       /* Above this threshold, a block is considered as moving*/
    U32 PixelMoveDetectionThreshold; /* Pixels are said moving pixels when their inter-frame (|A-D|) difference is superior to PixelMoveDetectionThreshold.*/
    U32 BlockMoveDetectionThreshold; /* A block is said moving if it contains more than BlockMoveDetectionThreshold moving pixels. */
                                     /* MoveStatus bit is set if at least one block is moving block.*/
    U32 CFDThreshold;   /* CFD pixels differences that are inferior to CFDThreshold are considered as noise so not taken into account. */
} STLAYER_FMDParams_t;

typedef struct STLAYER_FMDResults_s
{
    U32 PictureIndex;   /* Index of the current displayed picture. Note that it's not the picture number found during decode */
    U32 FieldRepeatCount; /* Gives the number of repeated fields since previous FMD event */
    U32 FieldSum;       /* Gives the motion between current and previous same polarity field (N-2 field). Inter-frame motion */
    U32 CFDSum;         /* Gives the motion between current and previous Fields. Inter-field motion  */
    U32  SceneCount;    /* Number of blocks that have their inter-frame pixels differences |A-D| sum superior to threshold SceneChangeDetectionThreshold */
    BOOL MoveStatus;    /* This bit is set if at least one block is a moving block regarding BlockMoveDetectionThreshold.*/
    BOOL RepeatStatus;  /* This bit is set if there is no moving block regarding RepeatFieldDetectionThreshold */
} STLAYER_FMDResults_t;

typedef struct STLAYER_VTGParams_s
{
    ST_DeviceName_t             VTGName;
    /* ... Valid if UpdateReason has STLAYER_VTG_REASON set */
    U32                         VTGFrameRate;
    /* ... Valid if UpdateReason has STLAYER_VTG_REASON set */
} STLAYER_VTGParams_t;

typedef struct STLAYER_UpdateParams_s
{
    STLAYER_UpdateReason_t      UpdateReason;
    STLAYER_LayerParams_t *     LayerParams_p;
    /* ... Valid if UpdateReason has STLAYER_LAYER_PARAMS_REASON set */
    STLAYER_VTGParams_t         VTGParams;
    /* ... Valid if UpdateReason has STLAYER_VTG_REASON set */
    ST_DeviceName_t             VTG_Name;
    /* ... REDUNDANT, kept only for backwards compatibility...  */
    /* ... Valid if UpdateReason has STLAYER_VTG_REASON set     */
    BOOL                        IsDecimationNeeded;
    /* ... Valid if UpdateReason has STLAYER_DECIMATION_NEED_REASON set */
    STLAYER_DecimationFactor_t  RecommendedVerticalDecimation;
    /* ... Valid if UpdateReason has STLAYER_DECIMATION_NEED_REASON set */
    STLAYER_DecimationFactor_t  RecommendedHorizontalDecimation;
    /* ... Valid if UpdateReason has STLAYER_DECIMATION_NEED_REASON set */
    U32                         StreamWidth;
    /* ... Valid if UpdateReason has STLAYER_DECIMATION_NEED_REASON set */
    U32                         StreamHeight;
    /* ... Valid if UpdateReason has STLAYER_DECIMATION_NEED_REASON set */
    struct
    {
        U32                      FrameBufferDisplayLatency; /* Number of VSyncs between video commit and framebuffer display */
        U32                      FrameBufferHoldTime;       /* Number of VSyncs that the display needs the framebuffer
                                                             * is kept unchanged for nodes generation and composition */
    } DisplayParamsForVideo;
    /* ... Valid if UpdateReason has STLAYER_CONNECT_REASON set */
} STLAYER_UpdateParams_t;

#ifdef ST_OSLINUX

#define LAYER_PARTITION_AVMEM         0         /* Which partition to use for allocating in AVMEM memory */
#define LAYER_SECURED_PARTITION_AVMEM           2         /* Which partition to use for allocating in SECURED AVMEM memory */


typedef struct STLAYER_AllocDataParams_s
{
    U32     Size;
    U32     Alignment;
} STLAYER_AllocDataParams_t;

ST_ErrorCode_t STLAYER_MapAddress( void *KernelAddress_p, void **UserAddress_p );
ST_ErrorCode_t STLAYER_AllocData( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p );
ST_ErrorCode_t STLAYER_AllocDataSecure( STLAYER_Handle_t  LayerHandle, STLAYER_AllocDataParams_t *Params_p, void **Address_p );
ST_ErrorCode_t STLAYER_FreeData( STLAYER_Handle_t  LayerHandle, void *Address_p );

U32 *STLAYER_UserToKernel( U32 VirtUserAddress_p );
U32 *STLAYER_KernelToUser( U32 VirtKernelAddress_p );

#endif
/* Exported Functions ------------------------------------------------------- */

/* Common functions */
ST_ErrorCode_t STLAYER_GetCapability(const ST_DeviceName_t DeviceName,
                                     STLAYER_Capability_t * const Capability);

ST_ErrorCode_t STLAYER_GetInitAllocParams(STLAYER_Layer_t LayerType,
                                          U32             ViewPortsNumber,
                                          STLAYER_AllocParams_t * Params);

ST_Revision_t  STLAYER_GetRevision(void);

ST_ErrorCode_t STLAYER_Init(const ST_DeviceName_t        DeviceName,
                            const STLAYER_InitParams_t * const InitParams_p);

ST_ErrorCode_t STLAYER_Term(const ST_DeviceName_t        DeviceName,
                            const STLAYER_TermParams_t * const TermParams_p);

ST_ErrorCode_t STLAYER_Open(const ST_DeviceName_t       DeviceName,
                            const STLAYER_OpenParams_t * const Params,
                            STLAYER_Handle_t *    Handle);

ST_ErrorCode_t STLAYER_Close(STLAYER_Handle_t  Handle);

ST_ErrorCode_t STLAYER_GetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

ST_ErrorCode_t STLAYER_SetLayerParams(STLAYER_Handle_t  Handle,
        STLAYER_LayerParams_t *  LayerParams_p);

/* ViewPort functions */
ST_ErrorCode_t STLAYER_OpenViewPort(
  STLAYER_Handle_t            LayerHandle,
  STLAYER_ViewPortParams_t*   Params,
  STLAYER_ViewPortHandle_t*   VPHandle);

ST_ErrorCode_t STLAYER_CloseViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_EnableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_DisableViewPort(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_AdjustViewPortParams(
  STLAYER_Handle_t LayerHandle,
  STLAYER_ViewPortParams_t*   Params_p
);

ST_ErrorCode_t STLAYER_SetViewPortParams(
  const STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_ViewPortParams_t*       Params_p
);

ST_ErrorCode_t STLAYER_GetViewPortParams(
  const STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_ViewPortParams_t*       Params_p
);

ST_ErrorCode_t STLAYER_SetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t STLAYER_GetViewPortSource(
  STLAYER_ViewPortHandle_t    VPHandle,
  STLAYER_ViewPortSource_t*   VPSource
);

ST_ErrorCode_t STLAYER_SetViewPortIORectangle(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_Rectangle_t*        InputRectangle,
  STGXOBJ_Rectangle_t*        OutputRectangle
);

ST_ErrorCode_t STLAYER_AdjustIORectangle(
  STLAYER_Handle_t       Handle,
  STGXOBJ_Rectangle_t*   InputRectangle,
  STGXOBJ_Rectangle_t*   OutputRectangle
);

ST_ErrorCode_t STLAYER_GetViewPortIORectangle(
  STLAYER_ViewPortHandle_t  VPHandle,
  STGXOBJ_Rectangle_t*      InputRectangle,
  STGXOBJ_Rectangle_t*      OutputRectangle
);

ST_ErrorCode_t STLAYER_SetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32                      XPosition,
  S32                      YPosition
);

ST_ErrorCode_t STLAYER_GetViewPortPosition(
  STLAYER_ViewPortHandle_t VPHandle,
  S32*                     XPosition,
  S32*                     YPosition
);

ST_ErrorCode_t STLAYER_SetViewPortPSI(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_PSI_t*            VPPSI_p
);

ST_ErrorCode_t STLAYER_GetViewPortPSI(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_PSI_t*            VPPSI_p
);

ST_ErrorCode_t STLAYER_SetViewPortSpecialMode (
  const STLAYER_ViewPortHandle_t                  VPHandle,
  const STLAYER_OutputMode_t                      OuputMode,
  const STLAYER_OutputWindowSpecialModeParams_t * const Params_p);

ST_ErrorCode_t STLAYER_GetViewPortSpecialMode (
  const STLAYER_ViewPortHandle_t                  VPHandle,
  STLAYER_OutputMode_t                    * const OuputMode_p,
  STLAYER_OutputWindowSpecialModeParams_t * const Params_p);

ST_ErrorCode_t STLAYER_DisableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_EnableColorKey(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_SetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t STLAYER_GetViewPortColorKey(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorKey_t*         ColorKey
);

ST_ErrorCode_t STLAYER_DisableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_EnableBorderAlpha(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_GetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t STLAYER_SetViewPortAlpha(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GlobalAlpha_t*    Alpha
);

ST_ErrorCode_t STLAYER_SetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);

ST_ErrorCode_t STLAYER_GetViewPortGain(
  STLAYER_ViewPortHandle_t  VPHandle,
  STLAYER_GainParams_t*     Params
);

ST_ErrorCode_t STLAYER_SetViewPortRecordable(STLAYER_ViewPortHandle_t  VPHandle,
                                             BOOL Recordable);

ST_ErrorCode_t STLAYER_GetViewPortRecordable(STLAYER_ViewPortHandle_t  VPHandle,
                                             BOOL  * Recordable_p);

ST_ErrorCode_t STLAYER_GetBitmapAllocParams(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  STGXOBJ_BitmapAllocParams_t* Params1_p,
  STGXOBJ_BitmapAllocParams_t* Params2_p
);

ST_ErrorCode_t STLAYER_GetBitmapHeaderSize(
  STLAYER_Handle_t             LayerHandle,
  STGXOBJ_Bitmap_t*            Bitmap_p,
  U32 *                        HeaderSize_p
);

ST_ErrorCode_t STLAYER_GetPaletteAllocParams(
  STLAYER_Handle_t               LayerHandle,
  STGXOBJ_Palette_t*             Palette_p,
  STGXOBJ_PaletteAllocParams_t*  Params_p
);

ST_ErrorCode_t STLAYER_GetVTGName(
  STLAYER_Handle_t               LayerHandle,
  ST_DeviceName_t * const        VTGName_p
);

ST_ErrorCode_t STLAYER_GetVTGParams(
  STLAYER_Handle_t               LayerHandle,
  STLAYER_VTGParams_t * const    VTGParams_p
);

ST_ErrorCode_t STLAYER_UpdateFromMixer(
  STLAYER_Handle_t              LayerHandle,
  STLAYER_OutputParams_t *      OutputParams_p
);

ST_ErrorCode_t STLAYER_DisableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_EnableViewPortFilter(STLAYER_ViewPortHandle_t VPHandle,
                                            STLAYER_Handle_t FilterHandle);

ST_ErrorCode_t STLAYER_AttachAlphaViewPort(STLAYER_ViewPortHandle_t VPHandle,
                                           STLAYER_Handle_t MaskedLayer);

ST_ErrorCode_t STLAYER_SetViewPortCompositionRecurrence(STLAYER_ViewPortHandle_t        VPHandle,
                                                  const STLAYER_CompositionRecurrence_t CompositionRecurrence);

ST_ErrorCode_t STLAYER_PerformViewPortComposition(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_GetViewPortFlickerFilterMode(
    STLAYER_ViewPortHandle_t          VPHandle,
    STLAYER_FlickerFilterMode_t*      FlickerFilterMode_p);

ST_ErrorCode_t STLAYER_SetViewPortFlickerFilterMode(
    STLAYER_ViewPortHandle_t          VPHandle,
    STLAYER_FlickerFilterMode_t       FlickerFilterMode);

ST_ErrorCode_t STLAYER_DisableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_EnableViewportColorFill(STLAYER_ViewPortHandle_t VPHandle);

ST_ErrorCode_t STLAYER_SetViewportColorFill(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorARGB_t*         ColorFill
);

ST_ErrorCode_t STLAYER_GetViewportColorFill(
  STLAYER_ViewPortHandle_t    VPHandle,
  STGXOBJ_ColorARGB_t*         ColorFill
);

ST_ErrorCode_t STLAYER_InformPictureToBeDecoded(
  const STLAYER_ViewPortHandle_t        VPHandle,
  STGXOBJ_PictureInfos_t*               PictureInfos_p
);

ST_ErrorCode_t STLAYER_CommitViewPortParams(STLAYER_ViewPortHandle_t VPHandle);

#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
ST_ErrorCode_t STLAYER_GetVideoDisplayParams(STLAYER_ViewPortHandle_t VPHandle,
                                    STLAYER_VideoDisplayParams_t * Params_p);

#endif /* STVID_DEBUG_GET_DISPLAY_PARAMS  */
#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
ST_ErrorCode_t STLAYER_GetRequestedDeinterlacingMode(STLAYER_ViewPortHandle_t VPHandle,
                                               STLAYER_DeiMode_t * RequestedMode_p);
ST_ErrorCode_t STLAYER_SetRequestedDeinterlacingMode(STLAYER_ViewPortHandle_t VPHandle,
                                               STLAYER_DeiMode_t RequestedMode);
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */

#ifdef STLAYER_USE_FMD
ST_ErrorCode_t STLAYER_EnableFMDReporting(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t STLAYER_DisableFMDReporting(STLAYER_ViewPortHandle_t VPHandle);
ST_ErrorCode_t STLAYER_GetFMDParams(STLAYER_ViewPortHandle_t VPHandle,
                                    STLAYER_FMDParams_t * Params_p);
ST_ErrorCode_t STLAYER_SetFMDParams(STLAYER_ViewPortHandle_t VPHandle,
                                    const STLAYER_FMDParams_t * Params_p);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STLAYER_H */

/* End of stlayer.h */
