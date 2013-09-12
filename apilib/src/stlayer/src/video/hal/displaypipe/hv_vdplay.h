/*******************************************************************************

File name   : hv_vdplay.h

Description : HAL video display displaypipe family header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
18 Oct 2005        Created                                          DG
********************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HAL_VIDEO_LAYER_DISPLAY_DISPLAYPIPE_H
#define __HAL_VIDEO_LAYER_DISPLAY_DISPLAYPIPE_H

/* Includes ----------------------------------------------------------------- */
#include "hv_vdpreg.h"

#ifndef __HAL_VIDEO_LAYER_DISPLAY_SDDISPO2_H
#include "stddefs.h"
#include "halv_lay.h"

#include "stsys.h"
#include "stgxobj.h"

#endif  /* __HAL_VIDEO_LAYER_DISPLAY_SDDISPO2_H */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Video DisplayPipe Options --------------------------------------------------- */

/* Interpolation option:
   When this option is set, the interpolation will be allowed only if the Fields Previous, Current, Next are consecutive.
   Non consecutive Fields will be discarded. As a result only the Current Field may be available so the Deinterlacing may be
   less performant */
/*#define INTERPOLATE_ONLY_WITH_CONSECUTIVE_FIELDS */

/* Use this define to check that DEI, VHSRC and Compo settings are consistent */
/*#define DBG_CHECK_VDP_CONSISTENCY*/

/* Use this define to track corruptions of the Horizontal/Vertical Filter Coeffs */
/*#define DBG_FILTER_COEFFS*/

/* Use this define to see the VHSRC settings and to be able to modify them on the fly */
/*#define DBG_USE_MANUAL_VHSRC_PARAMS */

#ifdef VIDEO_DEBUG
    /* Use this define monitor the field inversions */
    #define DBG_FIELD_INVERSIONS
#endif

#if defined (HW_7200)
#define MAIN_DISPLAY STLAYER_DISPLAYPIPE_VIDEO2
#define AUX_DISPLAY  STLAYER_DISPLAYPIPE_VIDEO3
#define SEC_DISPLAY  STLAYER_DISPLAYPIPE_VIDEO4
#endif  /* defined(HW_7200) */

#if defined(HW_7109)
#define MAIN_DISPLAY STLAYER_DISPLAYPIPE_VIDEO1
/* Aux display is same as 7100 */
#define AUX_DISPLAY  STLAYER_HDDISPO2_VIDEO2
#endif  /* defined(HW_7109) */

/* Exported Constants -------------------------------------------------------- */

#ifndef __HAL_VIDEO_LAYER_DISPLAY_SDDISPO2_HH

#define HFC_NB_COEFS                          35
#define VFC_NB_COEFS                          22

#define NO_ZOOM_FACTOR              (1<<13)     /* As SRC are 3.13 (2^13)=8192 */

#define LUMA_VERTICAL_MAX_ZOOM_OUT   (4 * NO_ZOOM_FACTOR)  /* Max zoom out is 4 */
#define LUMA_VERTICAL_NO_ZOOM        (1 * NO_ZOOM_FACTOR)  /* no zoom */

#define LUMA_HORIZONTAL_MAX_ZOOM_OUT (4 * NO_ZOOM_FACTOR)  /* Max zoom out is 4 */

#define WA_SHIFT_TINT  /* Work-around for TINT hard ware issue: left shift done by the hard*/

/* Max Luma Data Rate in MB/s */
#define MAX_LUMA_DATA_RATE      100

/* Exported Macros ---------------------------------------------------------- */

#define HAL_Read32(Address_p)               STSYS_ReadRegDev32LE ((void *) (Address_p))
#define HAL_Write32(Address_p, Value)       STSYS_WriteRegDev32LE((void *) (Address_p), (Value))

#define HAL_WriteGam32(Address_p, Value)\
    STSYS_WriteRegDev32LE((void *) (((U8 *)LayerProperties_p->GammaBaseAddress_p) + Address_p), (Value))

#define HAL_ReadGam32(Address_p)\
    STSYS_ReadRegDev32LE((void *) (((U8 *)LayerProperties_p->GammaBaseAddress_p) + Address_p))

#define HAL_ReadDisp32(Address_p)\
    STSYS_ReadRegDev32LE((void *) (((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p) + Address_p))

#define HAL_WriteDisp32(Address_p, Value)\
    STSYS_WriteRegDev32LE((void *) (((U8 *)LayerProperties_p->VideoDisplayBaseAddress_p) + Address_p), (Value))

#define HAL_SetRegister32DefaultValue(Address_p, RegMask, Value)                                \
{                                                                                               \
    U32 Tmp32;                                                                                  \
    Tmp32 = HAL_Read32 (Address_p);                                                             \
    HAL_Write32(Address_p, (Tmp32 & (~RegMask)) | (Value & RegMask));                           \
}

#define HAL_SetRegister32Value(Address_p, RegMask, Mask, Emp, Value)                            \
{                                                                                               \
    U32 Tmp32;                                                                                  \
    Tmp32 = HAL_Read32 (Address_p);                                                             \
    HAL_Write32 (Address_p, (Tmp32 & RegMask & (~((U32)Mask << Emp)) ) | ((U32)Value << Emp));  \
}

#endif  /* __HAL_VIDEO_LAYER_DISPLAY_SDDISPO2_H */

/* Define the maximum number of available display*/
#if defined (HW_7200)
    #define MAX_AVAILABLE_DISPLAY 3
#else
    #define MAX_AVAILABLE_DISPLAY 2
#endif /* HW_7200 */

/* Exported Types ----------------------------------------------------------- */
typedef struct
{
    U32                           Width;
    U32                           Height;
} HALLAYER_VDPDimensions_t;

typedef struct
{
    U32                         XStart;
    U32                         YStart;
    U32                         XOffset;
    U32                         YOffset;
} HALLAYER_VDPPosition_t;

typedef struct
{
    U32 LumaVertical;
    U32 LumaHorizontal;
    U32 ChromaVertical;
    U32 ChromaHorizontal;
} HALLAYER_VDPIncrement_t;

typedef struct
{
  U32 LumaVerticalTop;
  U32 LumaVerticalBottom;
  U32 ChromaVerticalTop;
  U32 ChromaVerticalBottom;
  U32 LumaHorizontal;
  U32 ChromaHorizontal;
} HALLAYER_VDPPhase_t;

typedef struct
{
  U32 LumaTop;
  U32 LumaBottom;
  U32 ChromaTop;
  U32 ChromaBottom;
  U32 LumaProgressive;
  U32 ChromaProgressive;
} HALLAYER_VDPFirstPixelRepetition_t;

typedef struct
{
  U32 X1;
  U32 X2;
  U32 Y1Top;
  U32 Y1Bottom;
  U32 Y2Top;
  U32 Y2Bottom;
} HALLAYER_VDPMemoryPosition_t;

typedef struct
{
  U32 MemoryPosition;
  U32 Phase;
  U32 FirstPixelRepetition;
} HALLAYER_VDPPolyphaseFilterSettings_t;

typedef struct
{
  HALLAYER_VDPPhase_t                  Phase;
  HALLAYER_VDPFirstPixelRepetition_t   FirstPixelRepetition;
} HALLAYER_VDPPositionDefinition_t;

typedef struct
{
  U32 Height1Top;
  U32 Height1Bottom;
  U32 Height2Top;
  U32 Height2Bottom;
  U32 Width1;
  U32 Width2;
} HALLAYER_VDPMemorySize_t;

typedef struct
{
    U32 RegDISP_VHSRC_CTL;
    U32 RegDISP_VHSRC_Y_HSRC;
    U32 RegDISP_VHSRC_Y_VSRC;
    U32 RegDISP_VHSRC_C_HSRC;
    U32 RegDISP_VHSRC_C_VSRC;
    U32 RegDISP_VHSRC_TARGET_SIZE;
    U32 RegDISP_VHSRC_NLZZD_Y;
    U32 RegDISP_VHSRC_NLZZD_C;
    U32 RegDISP_VHSRC_PDELTA;
    U32 RegDISP_VHSRC_Y_SIZE;
    U32 RegDISP_VHSRC_C_SIZE;
    U32 RegDISP_VHSRC_HCOEF_BA;
    U32 RegDISP_VHSRC_VCOEF_BA;
}HALLAYER_VDPVhsrcRegisterMap_t;


typedef struct
{
/*  U32 RegDISP_DEI_CTL;        Now handled by VDP_DEI_SetDeiMode() */
    U32 RegDISP_DEI_VIEWPORT_ORIG;
    U32 RegDISP_DEI_VIEWPORT_SIZE;
    U32 RegDISP_DEI_VF_SIZE;
    U32 RegDISP_DEI_T3I_CTL;
/*  U32 RegDISP_DEI_CYF_BA;     Handled in video driver */
/*  U32 RegDISP_DEI_CCF_BA;     Handled in video driver */
    U32 RegDISP_DEI_YF_FORMAT;
    U32 RegDISP_DEI_YF_STACK[8];
    U32 RegDISP_DEI_CF_FORMAT;
    U32 RegDISP_DEI_CF_STACK[8];
    U32 RegDISP_DEI_MF_STACK_L0;     /* Motion Line Address Stack */
    U32 RegDISP_DEI_MF_STACK_P0;     /* Motion Pixel Address Stack */
} HALLAYER_VDPDeiRegisterMap_t;


typedef struct
{
    HALLAYER_VDPVhsrcRegisterMap_t VhsrcRegisterMapTop;
    HALLAYER_VDPVhsrcRegisterMap_t VhsrcRegisterMapBottom;
    HALLAYER_VDPDeiRegisterMap_t    DeiRegisterMap;                 /* DEI registers are now common to Top and Bottom */
} HALLAYER_VDPRegisterConfiguration_t;

typedef struct
{
    STAVMEM_BlockHandle_t       HorizontalBlockHandle;          /* Horizontal Filters buffer avmem allocation/deallocation */
    STAVMEM_BlockHandle_t       VerticalBlockHandle;            /* Vertical Filters buffer avmem allocation/deallocation   */
    void *                      HorizontalAddress_p;            /* Horizontal Filters buffer address                       */
    void *                      VerticalAddress_p;              /* Vertical Filters buffer address                         */

    STAVMEM_BlockHandle_t       AlternateHorizontalBlockHandle; /* Alternate Horizontal Filters buffer avmem allocation/deallocation */
    STAVMEM_BlockHandle_t       AlternateVerticalBlockHandle;   /* Alternate Vertical Filters buffer avmem allocation/deallocation   */
    void *                      AlternateHorizontalAddress_p;   /* Alternate Horizontal Filters buffer address                       */
    void *                      AlternateVerticalAddress_p;     /* Alternate Vertical Filters buffer address                         */

    void *                      HorizontalAddressUsedByHw_p;            /* Horizontal Filters buffer address currently used by hardware.      */
    void *                      HorizontalAddressAvailForWritting_p;    /* Horizontal Filters buffer address available for writting           */
    void *                      VerticalAddressUsedByHw_p;              /* Vertical Filters buffer address currently used by hardware.      */
    void *                      VerticalAddressAvailForWritting_p;      /* Vertical Filters buffer address available for writting           */
} HALLAYER_VDPFilterBuffer_t;

typedef enum {
    VDP_FILTER_NO_SET,
    VDP_FILTER_NULL,
    VDP_FILTER_A,
    VDP_FILTER_B,
    VDP_FILTER_Y_C,
    VDP_FILTER_C_C,
    VDP_FILTER_Y_D,
    VDP_FILTER_C_D,
    VDP_FILTER_Y_E,
    VDP_FILTER_C_E,
    VDP_FILTER_Y_F,
    VDP_FILTER_C_F,
    VDP_FILTER_H_CHRX2,
    VDP_FILTER_V_CHRX2
} HALLAYER_VDPFilter_t;

typedef struct
{
    HALLAYER_VDPFilterBuffer_t FilterBuffer;

    HALLAYER_VDPFilter_t    CurrentVerticalLumaFilter;        /* Currently loaded vertical filters                      */
    HALLAYER_VDPFilter_t    CurrentVerticalChromaFilter;      /* Currently loaded vertical filters                      */
    HALLAYER_VDPFilter_t    CurrentHorizontalLumaFilter;      /* Currently loaded horizontal filters                    */
    HALLAYER_VDPFilter_t    CurrentHorizontalChromaFilter;    /* Currently loaded horizontal filters                    */

    HALLAYER_VDPRegisterConfiguration_t RegisterConfiguration;
} HALLAYER_VDPDisplayInstance_t;

typedef struct HALLAYER_VDPMotionBuffer_s
{
    STAVMEM_BlockHandle_t       BufferHandle;
    void *                                  BaseAddress_p;
} HALLAYER_VDPMotionBuffer_t;

typedef enum HALLAYER_VDPMotionDetection_e
{
    HALLAYER_VDP_MOTION_DETECTION_OFF,
    HALLAYER_VDP_MOTION_DETECTION_INIT,
    HALLAYER_VDP_MOTION_DETECTION_LOW,
    HALLAYER_VDP_MOTION_DETECTION_FULL
} HALLAYER_VDPMotionDetection_t;

typedef struct HALLAYER_VDPFormatInfo_s
{

    BOOL   IsFormat422Not420;
    BOOL   IsInputScantypeProgressive;         /* Picture Scantype */
    BOOL   IsFormatRasterNotMacroblock;
} HALLAYER_VDPFormatInfo_t;

typedef struct HALLAYER_VDPFieldArray_s
{
    STLAYER_Field_t PreviousField;
    STLAYER_Field_t CurrentField;
    STLAYER_Field_t NextField;
} HALLAYER_VDPFieldArray_t;


typedef struct
{
    /*************************  Video LAYER parameters     *************************/
    U8                          MaxViewportNumber;                  /* Maximum allowed opened viewports.                      */
    STLAYER_LayerParams_t       LayerParams;                        /* Layer parameters (Height, Width, ScanType, ...).       */
    U32                         VTGFrameRate;                       /* Output frame rate.                                     */
    ST_DeviceName_t             VTGName;                            /* VTG name                                               */
    HALLAYER_VDPPosition_t      LayerPosition;                      /* Complementary layer parameters (Start and Offset).     */
    void *                      ViewportProperties_p;               /* Viewport properties...                                 */
    BOOL                        PresentTopRegisters;
    U32                         LumaDataRateInBypassMode;           /* Luma Data Rate when DEI is Bypassed */
    U32                         VSyncCounter;                       /* Counter used to display periodical status */

    /*************************  Video VIEWPORT parameters  *************************/
    /* NB: These parameters should be viewport properties, but hardware doesn't allow separate viewport management. So the    */
    /*     viewport properties are store at hal_layer level. So only one viewport is then managerd.                           */
    STLAYER_ViewPortParams_t    ViewPortParams;                     /* Input/Output rectangle properties (position & size).   */
    BOOL                        IsViewportEnabled;                  /* Flag to know the enable/disable state of viewport.     */
    U32                         ViewportOffset;                     /* Viewport pos    (formula: (OutVStart<<16) | OutHStart) */
    U32                         ViewportSize;                       /* Viewport size   (formula: (OutHeight <<16) | OutWidth) */

    /*************************  Video Source parameters    *************************/
    HALLAYER_VDPDimensions_t    SourcePictureDimensions;            /* Source picture dimensions (Width and Height).          */
    STGXOBJ_ColorType_t         ColorType;                          /* Color type of the source picture.                      */
    HALLAYER_VDPFormatInfo_t    SourceFormatInfo;
    STGXOBJ_Rectangle_t         LastInputRectangle;                 /* Input Rectangle used at last VSync */
    STGXOBJ_Rectangle_t         LastOutputRectangle;                /* Ouput Rectangle used at last VSync */
    BOOL                        UpdateViewport;                     /* Used to update VID plane registers */
    BOOL                        IsDecimationActivated;
    BOOL                        AdvancedHDecimation;
    U32                         CurrentPictureHorizontalDecimationFactor;
    U32                         CurrentPictureVerticalDecimationFactor;
    STLAYER_DecimationFactor_t  RecommendedVerticalDecimation;
    STLAYER_DecimationFactor_t  RecommendedHorizontalDecimation;

    /*************************  Post DEI Format Info    *************************/
    HALLAYER_VDPFormatInfo_t    PostDeiFormatInfo;

    /*************************  Output Format Info    *************************/
    BOOL                        IsOutputScantypeProgressive;

    /*************************   Display Instances   *************************/
    HALLAYER_VDPDisplayInstance_t  DisplayInstance[MAX_AVAILABLE_DISPLAY];
    BOOL                           IsPeriodicalUpdateNeeded;
    BOOL                           ReloadOfHorizontalFiltersInProgress;         /* Is a reload of horizontal filters in progress? */
    BOOL                           ReloadOfVerticalFiltersInProgress;           /* Is a reload of vertical filters in progress? */
    BOOL                           FilterCoeffsReloaded;                        /* A new VSync has reloaded the filter coeffs */

    /*************************   Motion Buffers Parameters    *************************/
    HALLAYER_VDPMotionBuffer_t       MotionBuffer1;
    HALLAYER_VDPMotionBuffer_t       MotionBuffer2;
    HALLAYER_VDPMotionBuffer_t       MotionBuffer3;

    /* Pointers to the Previous, Current, Next motion buffers (they are rotating) */
    HALLAYER_VDPMotionBuffer_t *     PreviousMotionBuffer_p;
    HALLAYER_VDPMotionBuffer_t *     CurrentMotionBuffer_p;
    HALLAYER_VDPMotionBuffer_t *     NextMotionBuffer_p;
    HALLAYER_VDPMotionDetection_t    CurrentMotionMode;

    /*************************  Alloc/ Deallocations  *************************/
    STAVMEM_PartitionHandle_t   AvmemPartitionHandle;               /* Used for avmem allocation/deallocation.                */

    /*************************  PSI parameters      *************************/
    struct VDPPSI_Parameters_s
    {
        STLAYER_PSI_t           CurrentPSIParameters[STLAYER_MAX_VIDEO_FILTER_POSITION];    /* PSI parameters for the layer.      */
        U32                     ActivityMask;                                               /* Internal mask to summurize PSI activity.               */
    }VDPPSI_Parameters;

    /*************************  DEI parameters      *************************/
    STLAYER_DeiMode_t           RequestedDeiMode;
    STLAYER_DeiMode_t           AppliedDeiMode;
    HALLAYER_VDPFieldArray_t    FieldsToUseAtNextVSync;
    HALLAYER_VDPFieldArray_t    FieldsCurrentlyUsedByHw;

    /*************************  FMD parameters      *************************/
    BOOL                        FmdReportingEnabled;
    U32                         FmdFieldRepeatCount;
    HALLAYER_VDPDimensions_t    FMDSourcePictureDimensions;            /* Source picture dimensions (Width and Height).          */

#if 0  /* old FMD code from OL */
#ifdef ST_fmdenabled
    /*************************  FMD parameters      *************************/
    task_t*                     FmdTask_p;
    void*                       FmdTaskStack;
    tdesc_t                     FmdTaskDesc;
    BOOL                        FmdTaskContinue;
    /* Semaphore used to detect when FMD processing is finished */
    semaphore_t *               FmdProcessingDoneSemaphore_p;

    U32                         FmdConsecutiveFieldDifferenceMaxValue;
    U32                         FmdConsecutiveFieldDifferenceThreshold;
    BOOL                        MotionBetweenPreviousAndCurrentFields;
#endif /* ST_fmdenabled */
#endif

#ifdef DBG_FILTER_COEFFS
    /*************************                       Debug Filter Coeffs                              *************************/
    U32                         DbgLastU32OfHorizontalLumaBuffer;
    U32                         DbgLastU32OfVerticalLumaBuffer;
#endif

#ifdef DBG_USE_MANUAL_VHSRC_PARAMS
    /*************************                       Debug Vertical Pixels Position                   *************************/
    BOOL                        ManualParams;
    U32                         FirstPixelRepetitionLumaTop;
    U32                         FirstPixelRepetitionLumaBottom;
    U32                         PhaseRepetitionLumaTop;
    U32                         PhaseRepetitionLumaBottom;
#endif

#ifdef DBG_FIELD_INVERSIONS
    STLAYER_Field_t             OldCurrentField;
    U32                         FieldsWithFieldInversion;
    U32                         FieldsSetOnOddInputPosition;
    U32                         FieldsSetOnOddOutputPosition;
#endif
} HALLAYER_VDPLayerCommonData_t;

/* Exported Variables ------------------------------------------------------- */
extern const HALLAYER_FunctionsTable_t HALLAYER_DisplayPipeFunctions;


/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_LAYER_DISPLAY_DISPLAYPIPE_H */

/* End of hv_vdplay.h */
