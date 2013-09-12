/*******************************************************************************

File name   : halv_vin.h

Description : HAL video input header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
03/10/2000         Created                                          JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_INPUT_H
#define __HAL_VIDEO_INPUT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvin.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#ifdef ST_7710
#define WA_WRONG_CIRCULAR_DATA_WRITE
#define WA_WRONG_DATA_WRITE
#endif
/* Input mask IT : They are the same for all the HALs */
#define HALVIN_VERTICAL_SYNC_BOTTOM_MASK                 0x00000001
#define HALVIN_VERTICAL_SYNC_TOP_MASK                    0x00000002

typedef enum
{
    HALVIN_ANCILLARY_DATA_NIBBLE_ENCODED,
    HALVIN_ANCILLARY_DATA_STD_ENCODED
} HALVIN_AncillaryEncodedMode_t;

typedef enum
{
    HALVIN_ANCILLARY_DATA_CAPTURE_MODE,
    HALVIN_ANCILLARY_DATA_PACKETS_MODE
} HALVIN_AncillaryDataType_t;

typedef enum
{
    HALVIN_EMBEDDED_SYNC_MODE,
    HALVIN_RGB_MODE,
    HALVIN_LUMA_CHROMA_MODE,
    HALVIN_SD_PIXEL_INTERFACE
} HALVIN_InterfaceMode_t;

typedef enum
{
    HALVIN_DISABLE_RGB_TO_YCC_CONVERSION,
    HALVIN_ENABLED_RGB_TO_YCC_CONVERSION
} HALVIN_ConvertMode_t;


/* Exported Types ----------------------------------------------------------- */

/* Input Handle type */
typedef void * HALVIN_Handle_t;

typedef struct
{
    void                (*DisableAncillaryDataCapture)(const HALVIN_Handle_t InputHandle);
    void                (*DisableInterface)(const HALVIN_Handle_t InputHandle);
    void                (*EnableAncillaryDataCapture)(const HALVIN_Handle_t InputHandle);
    void                (*EnabledInterface)(const HALVIN_Handle_t InputHandle);
    U8                  (*GetInterruptStatus)(const HALVIN_Handle_t InputHandle);
    U32                 (*GetLineCounter)(const HALVIN_Handle_t InputHandle);
    U8                  (*GetStatus)(const HALVIN_Handle_t InputHandle);
    ST_ErrorCode_t      (*Init)(const HALVIN_Handle_t  InputHandle);
    ST_ErrorCode_t      (*SelectInterfaceMode)(const HALVIN_Handle_t InputHandle, const HALVIN_InterfaceMode_t InterfaceMode);
    ST_ErrorCode_t      (*SetAncillaryDataCaptureLength)(const HALVIN_Handle_t InputHandle, const U16 DataCaptureLength);
    void                (*SetAncillaryDataPointer)(const HALVIN_Handle_t InputHandle, void * const BufferAddress_p, const U32 DataBufferLength);
    ST_ErrorCode_t      (*SetAncillaryDataType)(const HALVIN_Handle_t InputHandle, const HALVIN_AncillaryDataType_t DataType);
    ST_ErrorCode_t      (*SetAncillaryEncodedMode)(const HALVIN_Handle_t InputHandle, const HALVIN_AncillaryEncodedMode_t EncodedMode);
    void                (*SetBlankingOffset)(const HALVIN_Handle_t  InputHandle, const U16 Vertical, const U16 Horizontal);
    ST_ErrorCode_t      (*SetInputPath)(const HALVIN_Handle_t InputHandle,const STVIN_InputPath_t InputPath);
    ST_ErrorCode_t      (*SetClockActiveEdge)(const HALVIN_Handle_t InputHandle, const STVIN_ActiveEdge_t ClockEdge, const STVIN_PolarityOfUpperFieldOutput_t Polarity);
    ST_ErrorCode_t      (*SetConversionMode)(const HALVIN_Handle_t InputHandle, const HALVIN_ConvertMode_t ConvertMode);
    ST_ErrorCode_t      (*SetFieldDetectionMethod)(const HALVIN_Handle_t InputHandle, const STVIN_FieldDetectionMethod_t DetectionMethod, const U16 LowerLimit, const U16 UpperLimit);
    void                (*SetHSyncOutHorizontalOffset)(const HALVIN_Handle_t  InputHandle, const U16 HorizontalOffset);
    void                (*SetInterruptMask)(const HALVIN_Handle_t InputHandle, const U8 Mask);
    void                (*SetReconstructedFrameMaxSize)(const HALVIN_Handle_t InputHandle, const U16 FrameWidth, const U16 FrameHeight);
    void                (*SetReconstructedFramePointer)(const HALVIN_Handle_t InputHandle, const STVID_PictureStructure_t PictureStructure, void * const BufferAddress1_p, void * const BufferAddress2_p);
    void                (*SetReconstructedFrameSize)(const HALVIN_Handle_t InputHandle, const U16 FrameWidth, const U16 FrameHeight);
    ST_ErrorCode_t      (*SetReconstructedStorageMemoryMode)(const HALVIN_Handle_t InputHandle,
                                                             const STVID_DecimationFactor_t  H_Decimation,
                                                             const STVID_DecimationFactor_t  V_Decimation,
                                                             const STGXOBJ_ScanType_t ScanType);
    ST_ErrorCode_t      (*SetScanType)(const HALVIN_Handle_t InputHandle, const STGXOBJ_ScanType_t ScanType);
    ST_ErrorCode_t      (*SetSizeOfTheFrame)(const HALVIN_Handle_t  InputHandle, const U16 FrameWidth, const U16 FrameHeightTop, const U16 FrameHeightBottom);
    void                (*SetSyncActiveEdge)(const HALVIN_Handle_t InputHandle, const STVIN_ActiveEdge_t HorizontalSyncEdge, const STVIN_ActiveEdge_t VerticalSyncEdge);
    ST_ErrorCode_t      (*SetSyncType)(const HALVIN_Handle_t InputHandle, const STVIN_SyncType_t SyncType);
    void                (*SetVSyncOutLineOffset)(const HALVIN_Handle_t  InputHandle, const U16 Horizontal, const U16 Vertical);
    void                (*Term)(const HALVIN_Handle_t InputHandle);
} HALVIN_FunctionsTable_t;


/* Parameters required to initialise HALINP */
typedef struct
{
    ST_Partition_t *    CPUPartition_p;         /* Where the module can allocate memory for its internal usage */
    STVIN_DeviceType_t  DeviceType;             /* 7015-7020 */
    STVIN_InputMode_t   InputMode;

    void *              DeviceBaseAddress_p;
    void *              RegistersBaseAddress_p;  /* Base address of the HALVIN registers (SD) */
    STVIN_VideoParams_t *       VideoParams_p;
    STAVMEM_PartitionHandle_t   AVMEMPartionHandle;
} HALVIN_InitParams_t;

typedef struct
{
    S32 InputWinX;
    S32 InputWinY;
    U32 InputWinWidth;
    U32 InputWinHeight;
    U32 OutputWinWidth;
    U32 OutputWinHeight;
} HALVIN_InputWin_t;

typedef struct
{
    const HALVIN_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *            CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STAVMEM_PartitionHandle_t   AVMEMPartitionHandle;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    STVIN_DeviceType_t          DeviceType;              /* 7015-7020 */
    void *                      RegistersBaseAddress_p;  /* Base address of the HALVIN registers (SD) */
    U32                         ValidityCheck;
    U8                          InputNumber;
    STVIN_VideoParams_t *       VideoParams_p;
    STVIN_DefInputMode_t        DefInputMode;           /* SD or HD ! */
    HALVIN_InputWin_t           InputWindow;
    STVIN_InputMode_t           InputMode;
} HALVIN_Properties_t;


/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define HALVIN_DisableAncillaryDataCapture(InputHandle)                           ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->DisableAncillaryDataCapture(InputHandle)
#define HALVIN_DisableInterface(InputHandle)                                      ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->DisableInterface(InputHandle)
#define HALVIN_EnableAncillaryDataCapture(InputHandle)                            ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->EnableAncillaryDataCapture(InputHandle)
#define HALVIN_EnabledInterface(InputHandle)                                      ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->EnabledInterface(InputHandle)
#define HALVIN_GetInterruptStatus(InputHandle)                                    ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->GetInterruptStatus(InputHandle)
#define HALVIN_GetLineCounter(InputHandle)                                        ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->GetLineCounter(InputHandle)
#define HALVIN_GetStatus(InputHandle)                                             ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->GetStatus(InputHandle)
#define HALVIN_SelectInterfaceMode(InputHandle,InterfaceMode)                     ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SelectInterfaceMode(InputHandle,InterfaceMode)
#define HALVIN_SetAncillaryDataCaptureLength(InputHandle,DataCaptureLength)       ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetAncillaryDataCaptureLength(InputHandle,DataCaptureLength)
#define HALVIN_SetAncillaryDataPointer(InputHandle,BufferAddress_p,DataBufferLength)               ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetAncillaryDataPointer(InputHandle,BufferAddress_p,DataBufferLength)
#define HALVIN_SetAncillaryDataType(InputHandle,DataType)                         ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetAncillaryDataType(InputHandle,DataType)
#define HALVIN_SetAncillaryEncodedMode(InputHandle,EncodedMode)                   ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetAncillaryEncodedMode(InputHandle,EncodedMode)
/* #define HALVIN_SetBlankingOffset(InputHandle,Vertical,Horizontal)                 ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetBlankingOffset(InputHandle,Vertical,Horizontal) */
#define HALVIN_SetClockActiveEdge(InputHandle,ClockEdge,Polarity)                 ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetClockActiveEdge(InputHandle,ClockEdge,Polarity)
#define HALVIN_SetConversionMode(InputHandle,ConvertMode)                         ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetConversionMode(InputHandle,ConvertMode)
#define HALVIN_SetFieldDetectionMethod(InputHandle,DetectionMethod,LowerLimit,UpperLimit) ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetFieldDetectionMethod(InputHandle,DetectionMethod,LowerLimit,UpperLimit)
#define HALVIN_SetHSyncOutHorizontalOffset(InputHandle,HorizontalOffset)          ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetHSyncOutHorizontalOffset(InputHandle,HorizontalOffset)
#define HALVIN_SetInterruptMask(InputHandle,Mask)                                 ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetInterruptMask(InputHandle,Mask)
#define HALVIN_SetReconstructedFrameMaxSize(InputHandle,FrameWidth, FrameHeight)  ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetReconstructedFrameMaxSize(InputHandle,FrameWidth,FrameHeight)
#define HALVIN_SetReconstructedFramePointer(InputHandle,PictureStructure,BufferAddress1_p,BufferAddress2_p)    ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetReconstructedFramePointer(InputHandle,PictureStructure,BufferAddress1_p,BufferAddress2_p)
#define HALVIN_SetReconstructedFrameSize(InputHandle,FrameWidth, FrameHeight)     ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetReconstructedFrameSize(InputHandle,FrameWidth,FrameHeight)
#define HALVIN_SetReconstructedStorageMemoryMode(InputHandle,H_Decimation,V_Decimation,ScanType) ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetReconstructedStorageMemoryMode(InputHandle,H_Decimation,V_Decimation,ScanType)
#define HALVIN_SetScanType(InputHandle,ScanType)                                  ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetScanType(InputHandle,ScanType)
#define HALVIN_SetSyncActiveEdge(InputHandle,HorizontalSyncEdge,VerticalSyncEdge) ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetSyncActiveEdge(InputHandle,HorizontalSyncEdge,VerticalSyncEdge)
/* #define HALVIN_SetSyncType(InputHandle,SyncMode)                                  ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetSyncType(InputHandle,SyncMode) */
#define HALVIN_SetVSyncOutLineOffset(InputHandle,Horizontal,Vertical)             ((HALVIN_Properties_t *)(InputHandle))->FunctionsTable_p->SetVSyncOutLineOffset(InputHandle,Horizontal,Vertical)


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t HALVIN_Init(const HALVIN_InitParams_t * const InitParams_p, HALVIN_Handle_t * const InputHandle_p);
ST_ErrorCode_t HALVIN_Term(const HALVIN_Handle_t InputHandle);

ST_ErrorCode_t HALVIN_AllocateMemoryForAncillaryData(const HALVIN_Handle_t InputHandle, U32 NumberOfTable, U32 TableSize, void * AncillaryDataTable_p);
ST_ErrorCode_t HALVIN_DeAllocateMemoryForAncillaryData(const HALVIN_Handle_t InputHandle, U32 NumberOfTable, void * AncillaryDataTable_p);

ST_ErrorCode_t HALVIN_SetSizeOfTheFrame(const HALVIN_Handle_t InputHandle, const U16 FrameWidth, const U16 FrameHeight);
ST_ErrorCode_t HALVIN_SetSyncType(const HALVIN_Handle_t InputHandle, const STVIN_SyncType_t SyncType);
ST_ErrorCode_t HALVIN_SetInputPath(const HALVIN_Handle_t InputHandle, const STVIN_InputPath_t InputPath);
void HALVIN_SetBlankingOffset(const HALVIN_Handle_t  InputHandle, const U16 Vertical, const U16 Horizontal);
STVIN_DefInputMode_t HALVIN_CheckInputParam(STVIN_DeviceType_t DeviceType, STVIN_InputType_t InputType, STVIN_SyncType_t SyncType);
ST_ErrorCode_t HALVIN_SetIOWindow(const HALVIN_Handle_t InputHandle, S32 InputWinX, S32 InputWinY, U32 InputWinWidth, U32 InputWinHeight, U32 OutputWinWidth, U32 OutputWinHeight);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_INPUT_H */

/* End of halv_inp.h */
