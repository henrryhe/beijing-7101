/*******************************************************************************

File name   : stvbi.h

Description : API Interfaces for the STVBI driver.

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
16 Jun 2000        Created                                           JG
16 Oct 2000        1.0.0A2                                           JG
12 Dec 2000        I2C address : U8 -> U16                           HSdLM
22 Feb 2001        Use STDENC mutual exclusion register accesses     HSdLM
04 Jul 2001        Remove STVBI_ERROR_VBI_CONFLICT error code        HSdLM
 *                 Add a STVBI_CCMode_t in STVBI_CCDynamicParams_t
14 Nov 2001        Select TTX lines individually (DDTS GNBvd08671)   HSdLM
 *                 New DeviceType STVBI_DEVICE_TYPE_GX1 for ST40GX1
 *                 New function to select TTX source on ST40GX1
 *                 Fix DDTS GNBvd10180 "wrapper around STAPI"
 14 Apr 2006       Add support CGMS-A Type B                         SBEBA
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STVBI_H
#define __STVBI_H

/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stavmem.h"
#include "stlayer.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
#define STVBI_DRIVER_ID      145
#define STVBI_DRIVER_BASE    (STVBI_DRIVER_ID << 16)

enum
{
    STVBI_ERROR_VBI_ALREADY_REGISTERED = STVBI_DRIVER_BASE,
    STVBI_ERROR_VBI_NOT_ENABLE,
    STVBI_ERROR_VBI_ENABLE,
    STVBI_ERROR_UNSUPPORTED_VBI,
    STVBI_ERROR_DENC_OPEN,
    STVBI_ERROR_DENC_ACCESS
};

typedef enum STVBI_DeviceType_e
{
    STVBI_DEVICE_TYPE_DENC,
    STVBI_DEVICE_TYPE_GX1,
    STVBI_DEVICE_TYPE_VIEWPORT
} STVBI_DeviceType_t;

typedef enum STVBI_VbiType_e
{
    STVBI_VBI_TYPE_NONE          = 0,
    STVBI_VBI_TYPE_TELETEXT      = 1,
    STVBI_VBI_TYPE_CLOSEDCAPTION = 2,
    STVBI_VBI_TYPE_WSS           = 4,
    STVBI_VBI_TYPE_CGMS          = 8,
    STVBI_VBI_TYPE_VPS           = 16,
    STVBI_VBI_TYPE_MACROVISION   = 32,
    STVBI_VBI_TYPE_CGMSFULL      = 64,
    STVBI_VBI_TYPE_MAX_SUPPORTED = 7
} STVBI_VbiType_t;

/* Closed Caption */
typedef enum STVBI_CCMode_e
{
    STVBI_CCMODE_NONE   = 1,
    STVBI_CCMODE_FIELD1 = 2,
    STVBI_CCMODE_FIELD2 = 4,
    STVBI_CCMODE_BOTH   = 8
} STVBI_CCMode_t;

/* Copy Protection */
typedef enum STVBI_CopyProtectionAlgorithm_e
{
    STVBI_COPY_PROTECTION_MV6 = 1,
    STVBI_COPY_PROTECTION_MV7 = 2
} STVBI_CopyProtectionAlgorithm_t;

typedef enum STVBI_MVPredefined_e
{
    STVBI_MV_PREDEFINED_0,
    STVBI_MV_PREDEFINED_1,
    STVBI_MV_PREDEFINED_2,
    STVBI_MV_PREDEFINED_3,
    STVBI_MV_PREDEFINED_4,
    STVBI_MV_PREDEFINED_5
} STVBI_MVPredefined_t;

typedef enum STVBI_MVUserDefined_e
{
    STVBI_MV_USER_DEFINED_0=0x1,
    STVBI_MV_USER_DEFINED_1=0x02,
    STVBI_MV_USER_DEFINED_2=0x4,
    STVBI_MV_USER_DEFINED_3=0x8,
    STVBI_MV_USER_DEFINED_4=0x10,
    STVBI_MV_USER_DEFINED_5=0x20 ,
    STVBI_MV_USER_DEFINED_6=0x40
} STVBI_MVUserDefined_t;

/* Teletext */
typedef enum STVBI_TeletextAmplitude_e
{
    STVBI_TELETEXT_AMPLITUDE_70IRE  = 1,
    STVBI_TELETEXT_AMPLITUDE_100IRE = 2
} STVBI_TeletextAmplitude_t;

typedef enum STVBI_TeletextStandard_e
{
    STVBI_TELETEXT_STANDARD_A = 1,
    STVBI_TELETEXT_STANDARD_B = 2,
    STVBI_TELETEXT_STANDARD_C = 4,
    STVBI_TELETEXT_STANDARD_D = 8
} STVBI_TeletextStandard_t;

typedef enum STVBI_CGMSFULLStandard_e
{
    STVBI_CGMS_HD_1080I60000 = 1,
    STVBI_CGMS_HD_720P60000  = 2,
    STVBI_CGMS_SD_480P60000  = 4,
    STVBI_CGMS_TYPE_B_HD_1080I60000 = 8,
    STVBI_CGMS_TYPE_B_HD_720P60000  = 16,
    STVBI_CGMS_TYPE_B_SD_480P60000  = 32
} STVBI_CGMSFULLStandard_t;


typedef enum STVBI_TeletextSource_e
{
    STVBI_TELETEXT_SOURCE_DMA = 1,
    STVBI_TELETEXT_SOURCE_PIN = 2
} STVBI_TeletextSource_t;

/* Exported Types --------------------------------------------------------- */

typedef U8 STVBI_TeletextLatency_t;

/* Closed Caption */
typedef struct STVBI_CCConfiguration_s
{
    STVBI_CCMode_t Mode;
} STVBI_CCConfiguration_t;

typedef struct STVBI_CCDynamicParams_s
{
    STVBI_CCMode_t Mode;
    U16            LinesField1;
    U16            LinesField2;
} STVBI_CCDynamicParams_t;

/* CGMS */
typedef struct STVBI_CGMSConfiguration_s
{
    U8 NotUsed;
} STVBI_CGMSConfiguration_t;

typedef struct STVBI_CGMSDynamicParams_s
{
    U8 NotUsed;
} STVBI_CGMSDynamicParams_t;

/* FULL-CGMS */
typedef struct STVBI_CGMSFULLConfiguration_s
{
    STVBI_CGMSFULLStandard_t      CGMSFULLStandard;

} STVBI_CGMSFULLConfiguration_t;

typedef struct STVBI_CGMSFULLDynamicParams_s
{
    STVBI_CGMSFULLStandard_t      CGMSFULLStandard;
} STVBI_CGMSFULLDynamicParams_t;


/* Copy Protection */
typedef struct STVBI_MacrovisionConfiguration_s
{
    STVBI_CopyProtectionAlgorithm_t Algorithm;
} STVBI_MacrovisionConfiguration_t;

typedef struct STVBI_MacrovisionDynamicParams_s
{
    STVBI_MVPredefined_t  PredefinedMode;
    STVBI_MVUserDefined_t UserMode;
} STVBI_MacrovisionDynamicParams_t;

/* Teletext */
typedef struct STVBI_TeletextConfiguration_s
{
    STVBI_TeletextLatency_t   Latency;
    BOOL                      FullPage; /* TRUE => Full Page */
    STVBI_TeletextAmplitude_t Amplitude;
    STVBI_TeletextStandard_t  Standard;
} STVBI_TeletextConfiguration_t;

typedef struct STVBI_TeletextDynamicParams_s
{
    BOOL Mask;   /* TRUE => Framing mask on */
    U16  LineCount; /* 1 to 16 for contiguous lines, 0 to use LineMask */
    U32  LineMask; /* = (1 << line_offset[n]) | .. | (1 << line_offset[m]) for teletext lines n, ..., m */
    BOOL Field;  /* TRUE => ODD */
} STVBI_TeletextDynamicParams_t;

/* VPS */
typedef struct STVBI_VPSConfiguration_s
{
    U8 NotUsed;
} STVBI_VPSConfiguration_t;

typedef struct STVBI_VPSDynamicParams_s
{
    U8 NotUsed;
} STVBI_VPSDynamicParams_t;

/* WSS */
typedef struct STVBI_WSSConfiguration_s
{
    U8 NotUsed;
} STVBI_WSSConfiguration_t;

typedef struct STVBI_WSSDynamicParams_s
{
    U8 NotUsed;
} STVBI_WSSDynamicParams_t;


typedef struct STVBI_Configuration_s
{
    STVBI_VbiType_t VbiType;
    union
    {
        STVBI_CCConfiguration_t          CC;
        STVBI_CGMSConfiguration_t        CGMS;
        STVBI_MacrovisionConfiguration_t MV;
        STVBI_TeletextConfiguration_t    TTX;
        STVBI_VPSConfiguration_t         VPS;
        STVBI_WSSConfiguration_t         WSS;
        STVBI_CGMSFULLConfiguration_t    CGMSFULL;
    } Type;
} STVBI_Configuration_t;

typedef struct STVBI_DynamicParams_s
{
    STVBI_VbiType_t VbiType;
    union
    {
        STVBI_CCDynamicParams_t          CC;
        STVBI_CGMSDynamicParams_t        CGMS;
        STVBI_MacrovisionDynamicParams_t MV;
        STVBI_TeletextDynamicParams_t    TTX;
        STVBI_VPSDynamicParams_t         VPS;
        STVBI_WSSDynamicParams_t         WSS;
        STVBI_CGMSFULLDynamicParams_t    CGMSFULL;
    } Type;
} STVBI_DynamicParams_t;


typedef U32 STVBI_Handle_t;

typedef struct STVBI_Capability_s
{
    STVBI_VbiType_t                 SupportedVbi;
    STVBI_TeletextStandard_t        SupportedTtxStd;
    STVBI_CopyProtectionAlgorithm_t SupportedMvAlgo;
    STVBI_TeletextSource_t          SupportedTeletextSource;
} STVBI_Capability_t;

typedef struct STVBI_FullCellAccess_s
{
    ST_DeviceName_t                 DencName;
    void *                          BaseAddress_p;
    void *                          DeviceBaseAddress_p;
} STVBI_FullCellAccess_t;

typedef struct STVBI_ViewportAccess_s
{
    STAVMEM_PartitionHandle_t       AVMemPartitionHandle;
    ST_DeviceName_t                 LayerName;
 } STVBI_ViewportAccess_t;



typedef struct STVBI_InitParams_s
{
    STVBI_DeviceType_t  DeviceType;
    U32                 MaxOpen;
    union
    {
        ST_DeviceName_t         DencName;
        STVBI_FullCellAccess_t  FullCell;
        STVBI_ViewportAccess_t  Viewport;
    } Target;
} STVBI_InitParams_t;

typedef struct STVBI_OpenParams_s
{
    STVBI_Configuration_t Configuration;
} STVBI_OpenParams_t;

typedef struct STVBI_TermParams_s
{
    BOOL    ForceTerminate;
} STVBI_TermParams_t;

typedef struct STVBI_AllocDataParams_s
{
    U32 Size;
    U32 Alignment;
} STVBI_AllocDataParams_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_Revision_t   STVBI_GetRevision (
                void);

ST_ErrorCode_t  STVBI_GetCapability (
                const ST_DeviceName_t         DeviceName,
                STVBI_Capability_t *          const Capability_p);

ST_ErrorCode_t  STVBI_Init (
                const ST_DeviceName_t         DeviceName,
                const STVBI_InitParams_t *    const InitParams_p);

ST_ErrorCode_t  STVBI_Open (
                const ST_DeviceName_t         DeviceName,
                const STVBI_OpenParams_t *    const OpenParams_p,
                STVBI_Handle_t *              const Handle_p);

ST_ErrorCode_t  STVBI_Close (
                const STVBI_Handle_t          Handle);

ST_ErrorCode_t  STVBI_Term (
                const ST_DeviceName_t         DeviceName,
                const STVBI_TermParams_t *    const TermParams_p);

ST_ErrorCode_t  STVBI_Disable (
                const STVBI_Handle_t          Handle);

ST_ErrorCode_t  STVBI_Enable (
                const STVBI_Handle_t          Handle);

ST_ErrorCode_t  STVBI_SetDynamicParams (
                const STVBI_Handle_t          Handle,
                const STVBI_DynamicParams_t * const DynamicParams_p);

ST_ErrorCode_t  STVBI_GetConfiguration (
                const STVBI_Handle_t          Handle,
                STVBI_Configuration_t *       const Configuration_p);

ST_ErrorCode_t  STVBI_WriteData (
                const STVBI_Handle_t          Handle,
                const U8*                     const Data_p,
                const U8                      Length);

ST_ErrorCode_t  STVBI_SetTeletextSource (
                const STVBI_Handle_t          Handle,
                const STVBI_TeletextSource_t  TeletextSource);

ST_ErrorCode_t  STVBI_GetTeletextSource (
                const STVBI_Handle_t          Handle,
                STVBI_TeletextSource_t *      const TeletextSource_p);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STVBI_H */

/* End of stvbi.h */
