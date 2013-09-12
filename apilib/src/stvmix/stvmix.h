/*******************************************************************************

File name   : stvmix.h

Description : video mixer module header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
21 July 2000        Created                                          BS
26 Oct  2000        Release A2                                       BS
10 July 2001        Release Beta 7015                                BS
12 December 2001    Correct DDTS10183                                BS
21 March 2002       Add function & device type                       BS
07 March 2005       Add VBI function for 5105                        SBEBA
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVMIX_H
#define __STVMIX_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stgxobj.h"
#ifdef WA_GNBvd31390 /*WA for Chroma Luma delay bug on STi5100*/
#include "stvtg.h"
#endif
/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STVMIX_DRIVER_ID    160       /* API number for STVMIX */
#define STVMIX_DRIVER_BASE  (STVMIX_DRIVER_ID << 16)


/* Software workarounds for hardware bugs implemented */
/*----------------------------------------------------*/
#define WA_GNBvd06549
/* Mix1 AVO Bits 9,10,11,25,26 & Mix2 AV0 Bits 9,25 not funct on 7015         */
/* WA_GNBvd06551: Mixer1 definition bit affects both graphics planes on 7015  */
/* WA_Valid_7015_Cut1.1 4.1: Start of mixer horz window id odd on 7015        */

#define WA_GNBvd16602
/* Default background is not blck but gray. Set a black background when unused*/

/* STVMIX return codes */
enum
{
    STVMIX_ERROR_LAYER_UNKNOWN = STVMIX_DRIVER_BASE,
    STVMIX_ERROR_LAYER_UPDATE_PARAMETERS,
    STVMIX_ERROR_LAYER_ALREADY_CONNECTED,
    STVMIX_ERROR_LAYER_ACCESS,
    STVMIX_ERROR_VOUT_UNKNOWN,
    STVMIX_ERROR_VOUT_ALREADY_CONNECTED,
    STVMIX_ERROR_NO_VSYNC
};


/* Exported Types ----------------------------------------------------------- */

typedef enum STVMIX_DeviceType_e
{
    STVMIX_OMEGA1_SD,
    STVMIX_GAMMA_TYPE_7015,
    STVMIX_GAMMA_TYPE_GX1,
    STVMIX_GENERIC_GAMMA_TYPE_7020,
    STVMIX_GENERIC_GAMMA_TYPE_5528,
    STVMIX_COMPOSITOR,
    STVMIX_GENERIC_GAMMA_TYPE_7710,
    STVMIX_GENERIC_GAMMA_TYPE_7100,
    STVMIX_GENERIC_GAMMA_TYPE_7109,
    STVMIX_GENERIC_GAMMA_TYPE_7200,
    STVMIX_COMPOSITOR_FIELD,
    STVMIX_COMPOSITOR_FIELD_SDRAM,
    STVMIX_COMPOSITOR_FIELD_COMBINED,
    STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM,
    STVMIX_COMPOSITOR_422,
    STVMIX_COMPOSITOR_FIELD_422,
    STVMIX_COMPOSITOR_FIELD_SDRAM_422,
    STVMIX_COMPOSITOR_FIELD_COMBINED_422,
    STVMIX_COMPOSITOR_FIELD_COMBINED_SDRAM_422
} STVMIX_DeviceType_t;

 /* type used for VBIViewPort*/
typedef enum STVMIX_VBIType_e
{
    STVMIX_VBI_TXT_ODD,
    STVMIX_VBI_TXT_EVEN
} STVMIX_VBIViewPortType_t;


typedef U32 STVMIX_Handle_t;
typedef U32 STVMIX_VBIViewPortHandle_t;

typedef U8 STVMIX_VBISource_t;
typedef struct STVMIX_VBIViewPortParams_s
{

    STVMIX_VBISource_t*       Source_p;
    U32                       LineMask; /* = (1 << line_offset[n]) | .. | (1 << line_offset[m])
                                         for teletext lines n, ..., m Start from line 7 */

} STVMIX_VBIViewPortParams_t;





typedef struct STVMIX_Capability_s
{
    S8                      ScreenOffsetVerticalMin;
    S8                      ScreenOffsetVerticalMax;
    S8                      ScreenOffsetHorizontalMin;
    S8                      ScreenOffsetHorizontalMax;
} STVMIX_Capability_t;

typedef struct STVMIX_RegisterInfo_s
{
    void*  CompoBaseAddress_p;     /* COMPO registers related */
    void*  GdmaBaseAddress_p;      /* GDMA registers related  */
    void*  VoutBaseAddress_p;      /* VOUT registers related  */

} STVMIX_RegisterInfo_t;

typedef struct STVMIX_InitParams_s
{
    ST_Partition_t*             CPUPartition_p;
    void*                       BaseAddress_p;
    void*                       DeviceBaseAddress_p;
    STVMIX_DeviceType_t         DeviceType;
    U32                         MaxOpen;
    U16                         MaxLayer;
    ST_DeviceName_t             VTGName;
    ST_DeviceName_t             EvtHandlerName;
    ST_DeviceName_t*            OutputArray_p;
    STAVMEM_PartitionHandle_t   AVMEM_Partition;
    STAVMEM_PartitionHandle_t   AVMEM_Partition2;/* Cached Memory Partition used for OutputBitmap Used Only for 51xx &53xx */
    STVMIX_RegisterInfo_t       RegisterInfo;
    STGXOBJ_Bitmap_t*           CacheBitmap_p;
} STVMIX_InitParams_t;

typedef struct STVMIX_LayerDisplayParams_s
{
    ST_DeviceName_t         DeviceName;
    BOOL                    ActiveSignal;
    /* For type omega1, not used             */
    /* For type gamma, graphic active signal */
} STVMIX_LayerDisplayParams_t;

typedef struct
{
    U8                      Dummy;
} STVMIX_OpenParams_t;

typedef struct STVMIX_ScreenParams_s
{
    STGXOBJ_AspectRatio_t   AspectRatio;
    STGXOBJ_ScanType_t      ScanType;
    U32                     FrameRate; /* Frame rate of picture in milli Herz */
    U32                     Width;
    U32                     Height;
    U32                     XStart;
    U32                     YStart;
} STVMIX_ScreenParams_t;

typedef struct STVMIX_TermParams_s
{
    BOOL    ForceTerminate;
} STVMIX_TermParams_t;




/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* For function ConnectLayers and GetConnectedLayers the order is defined     */
/* from the farthest to the closest from the eyes                             */

ST_ErrorCode_t STVMIX_ConnectLayers(
        const STVMIX_Handle_t                      Handle,
        const STVMIX_LayerDisplayParams_t*  const  LayerDisplayParams[],
        const U16                                  NbLayerParams
);

ST_ErrorCode_t STVMIX_Close( const STVMIX_Handle_t   Handle);

ST_ErrorCode_t STVMIX_Disable( const STVMIX_Handle_t   Handle);

ST_ErrorCode_t STVMIX_DisconnectLayers( const STVMIX_Handle_t   Handle);

ST_ErrorCode_t STVMIX_Enable( const STVMIX_Handle_t   Handle);

ST_ErrorCode_t STVMIX_GetBackgroundColor(
        const STVMIX_Handle_t     Handle,
        STGXOBJ_ColorRGB_t* const RGB888_p,
        BOOL* const               Enable_p
);

ST_ErrorCode_t STVMIX_GetCapability(
        const ST_DeviceName_t   DeviceName,
        STVMIX_Capability_t* const Capability_p
);

ST_ErrorCode_t STVMIX_GetConnectedLayers(const STVMIX_Handle_t Handle,
                                         const U16 LayerPosition,
                                         STVMIX_LayerDisplayParams_t* const LayerParams_p
);

ST_Revision_t STVMIX_GetRevision( void );

ST_ErrorCode_t STVMIX_GetScreenOffset(
        const STVMIX_Handle_t     Handle,
        S8* const           Horizontal_p,
        S8* const           Vertical_p
);

ST_ErrorCode_t STVMIX_GetScreenParams(
        const STVMIX_Handle_t   Handle,
        STVMIX_ScreenParams_t*  const ScreenParams_p
);

ST_ErrorCode_t STVMIX_GetTimeBase(
        const STVMIX_Handle_t   Handle,
        ST_DeviceName_t* const  VTGDriver_p
);

ST_ErrorCode_t STVMIX_Init(
        const   ST_DeviceName_t       DeviceName,
        const   STVMIX_InitParams_t* const InitParams_p
);

ST_ErrorCode_t STVMIX_Open(
        const   ST_DeviceName_t         DeviceName,
        const   STVMIX_OpenParams_t*    Params_p,
        STVMIX_Handle_t* const          Handle_p
);

ST_ErrorCode_t STVMIX_SetBackgroundColor(
        const STVMIX_Handle_t    Handle,
        const STGXOBJ_ColorRGB_t RGB888,
        const BOOL               Enable
);

ST_ErrorCode_t STVMIX_SetScreenOffset(
        const STVMIX_Handle_t   Handle,
        const S8                Horizontal,
        const S8                Vertical
);

ST_ErrorCode_t STVMIX_SetScreenParams(
        const STVMIX_Handle_t   Handle,
        const STVMIX_ScreenParams_t* const ScreenParams_p
);

ST_ErrorCode_t STVMIX_SetTimeBase(
        const STVMIX_Handle_t  Handle,
        const ST_DeviceName_t  VTGDriver
);

ST_ErrorCode_t STVMIX_Term(
        const   ST_DeviceName_t            DeviceName,
        const   STVMIX_TermParams_t* const TermParams_p
);

   /* STVMIX VBI funtions */
ST_ErrorCode_t STVMIX_OpenVBIViewPort(
                                      STVMIX_Handle_t Handle,
                                      STVMIX_VBIViewPortType_t VBIType,
                                      STVMIX_VBIViewPortHandle_t* VPHandle_p);

ST_ErrorCode_t STVMIX_CloseVBIViewPort(
                                      STVMIX_VBIViewPortHandle_t  VPHandle);

ST_ErrorCode_t STVMIX_EnableVBIViewPort(
                                      STVMIX_VBIViewPortHandle_t  VPHandle);

ST_ErrorCode_t STVMIX_DisableVBIViewPort(
                                      STVMIX_VBIViewPortHandle_t  VPHandle);

ST_ErrorCode_t STVMIX_SetVBIViewPortParams(
                                      STVMIX_VBIViewPortHandle_t  VPHandle,
                                      STVMIX_VBIViewPortParams_t* Params_p);

   /* Global Flicker Filter management */
ST_ErrorCode_t STVMIX_EnableGlobalFlickerFilter(const STVMIX_Handle_t   Handle);

ST_ErrorCode_t STVMIX_DisableGlobalFlickerFilter(const STVMIX_Handle_t   Handle);









#ifdef WA_GNBvd31390 /*WA for Chroma Luma delay bug on STi5100*/
ST_ErrorCode_t STVMIX_CLDelayWA(
        STVMIX_Handle_t Handle,
        STVTG_TimingMode_t Mode
);
#endif /*WA_GNBvd31390*/
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVMIX_H */

/* End of stvmix.h */
