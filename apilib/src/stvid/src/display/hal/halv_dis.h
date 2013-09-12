/*******************************************************************************

File name   : halv_dis.h

Description : HAL video diplay header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
06 Jul 2000        Created                                           HG
01 Feb 2001        Digital Input merge                               JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_DISPLAY_H
#define __HAL_VIDEO_DISPLAY_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"
#include "stgxobj.h"
#include "stevt.h"
#include "display.h"
#ifdef ST_crc
    #include "crc.h"
#endif /* ST_crc */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


typedef enum
{
  HALDISP_TOP_FIELD,
  HALDISP_BOTTOM_FIELD
} HALDISP_FieldType_t;


typedef struct HALDISP_Field_s
{
    VIDBUFF_PictureBuffer_t         *PictureBuffer_p;
    U32                             PictureIndex;
    HALDISP_FieldType_t             FieldType;
}HALDISP_Field_t;


typedef void * HALDISP_Handle_t;

/* Display Functions Table */
typedef struct
{
    void (*DisablePresentationChromaFieldToggle)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
    void (*DisablePresentationLumaFieldToggle)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
    void (*EnablePresentationChromaFieldToggle)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
    void (*EnablePresentationLumaFieldToggle)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType);
    ST_ErrorCode_t (*Init)(const HALDISP_Handle_t DisplayHandle);
    void (*SelectPresentationChromaBottomField)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
    void (*SelectPresentationChromaTopField)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
    void (*SelectPresentationLumaBottomField)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
    void (*SelectPresentationLumaTopField)(const HALDISP_Handle_t DisplayHandle,
                         const STLAYER_Layer_t     LayerType,
                         const U32 LayerIdent,
                         const VIDBUFF_PictureBuffer_t*  const Picture_p);
    void (*SetPresentationChromaFrameBuffer)(
            const HALDISP_Handle_t        DisplayHandle,
            const STLAYER_Layer_t         LayerType,
            void * const                  BufferAddress_p);
    void (*SetPresentationFrameDimensions)(
            const HALDISP_Handle_t        DisplayHandle,
            const STLAYER_Layer_t         LayerType,
            const U32                     HorizontalSize,
            const U32                     VerticalSize);
    void (*SetPresentationLumaFrameBuffer)(
            const HALDISP_Handle_t        DisplayHandle,
            const STLAYER_Layer_t         LayerType,
            void * const                  BufferAddress_p);
    void (*SetPresentationStoredPictureProperties)(
            const HALDISP_Handle_t              DisplayHandle,
            const STLAYER_Layer_t               LayerType,
            const VIDDISP_ShowParams_t * const  ShowParams_p);
    void (*PresentFieldsForNextVSync)(const HALDISP_Handle_t HALDisplayHandle,
                                                        const STLAYER_Layer_t  LayerType,
                                                        const U32           LayerIdent,
                                                        const HALDISP_Field_t * FieldsForNextVSync);
    void (*Term)(const HALDISP_Handle_t DisplayHandle);
} HALDISP_FunctionsTable_t;

/* Parameters required to initialise HALDISP */
typedef struct
{
    ST_Partition_t *    CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandleForMotionBuffers;
    STVID_DeviceType_t  DeviceType;
    void *              RegistersBaseAddress_p;  /* Base address of the Display registers */
    ST_DeviceName_t     EventsHandlerName;
    ST_DeviceName_t     VideoName;              /* Name of the video instance */
    U8                  DecoderNumber;  /* As defined by HALDEC. See definition there. */
#ifdef ST_crc
    VIDCRC_Handle_t     CRCHandle;
#endif /* ST_crc */
} HALDISP_InitParams_t;

typedef struct {
    const HALDISP_FunctionsTable_t * FunctionsTable_p;
    ST_Partition_t *        CPUPartition_p; /* Where the module can allocate memory for its internal usage */
    STAVMEM_PartitionHandle_t   AvmemPartitionHandleForMotionBuffers;
    STVID_DeviceType_t      DeviceType;
    void *                  RegistersBaseAddress_p;     /* Base address of the Display registers */
    STEVT_Handle_t          EventsHandle;
    ST_DeviceName_t         VideoName;                  /* Name of the video instance */
    U32                     ValidityCheck;
    void *                  PrivateData_p;
    U8                      DecoderNumber;              /* As defined by HALDEC. See definition there. */
    STEVT_EventID_t         UpdateGfxLayerSrcEvtID;
#ifdef ST_crc
    VIDCRC_Handle_t         CRCHandle;
#endif /* ST_crc */
    HALDISP_Field_t *       FieldsUsedAtNextVSync_p ;   /** Store the Picture field information **/
    BOOL                    IsPictureShown;             /** Used to indicate if we are showing a still picture or decoding video **/
    BOOL                    IsDisplayStopped;
    BOOL                    PeriodicFieldInversionDueToFRC; /* Flag indicating that periodic field inversions are normal because of Frame Rate Conversion */
} HALDISP_Properties_t;


/* Exported Macros ---------------------------------------------------------- */
#define HALDISP_DisablePresentationChromaFieldToggle(DisplayHandle,Layer)                             ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->DisablePresentationChromaFieldToggle(DisplayHandle,Layer)
#define HALDISP_DisablePresentationLumaFieldToggle(DisplayHandle,Layer)                               ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->DisablePresentationLumaFieldToggle(DisplayHandle,Layer)
#define HALDISP_EnablePresentationChromaFieldToggle(DisplayHandle,Layer)                              ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->EnablePresentationChromaFieldToggle(DisplayHandle,Layer)
#define HALDISP_EnablePresentationLumaFieldToggle(DisplayHandle,Layer)                                ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->EnablePresentationLumaFieldToggle(DisplayHandle,Layer)
#define HALDISP_SelectPresentationChromaBottomField(DisplayHandle,Layer,LayerIdent,Picture_p)         ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SelectPresentationChromaBottomField(DisplayHandle,Layer,LayerIdent,Picture_p)
#define HALDISP_SelectPresentationChromaTopField(DisplayHandle,Layer,LayerIdent,Picture_p)            ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SelectPresentationChromaTopField(DisplayHandle,Layer,LayerIdent,Picture_p)
#define HALDISP_SelectPresentationLumaBottomField(DisplayHandle,Layer,LayerIdent,Picture_p)           ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SelectPresentationLumaBottomField(DisplayHandle,Layer,LayerIdent,Picture_p)
#define HALDISP_SelectPresentationLumaTopField(DisplayHandle,Layer,LayerIdent,Picture_p)              ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SelectPresentationLumaTopField(DisplayHandle,Layer,LayerIdent,Picture_p)
#define HALDISP_SetPresentationChromaFrameBuffer(DisplayHandle,LayerType,BufferAddress_p)             ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SetPresentationChromaFrameBuffer(DisplayHandle,LayerType,BufferAddress_p)
#define HALDISP_SetPresentationFrameDimensions(DisplayHandle,LayerType,HorizontalSize,VerticalSize)   ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SetPresentationFrameDimensions(DisplayHandle,LayerType,HorizontalSize,VerticalSize)
#define HALDISP_SetPresentationLumaFrameBuffer(DisplayHandle,LayerType,BufferAddress_p)               ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SetPresentationLumaFrameBuffer(DisplayHandle,LayerType,BufferAddress_p)
#define HALDISP_SetPresentationStoredPictureProperties(DisplayHandle,LayerType,ShowParams_p)          ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->SetPresentationStoredPictureProperties(DisplayHandle,LayerType,ShowParams_p)
#define HALDISP_PresentFieldsForNextVSync(DisplayHandle,Layer,LayerIdent,Field_p)                     ((HALDISP_Properties_t *)(DisplayHandle))->FunctionsTable_p->PresentFieldsForNextVSync(DisplayHandle, Layer, LayerIdent, Field_p)


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t HALDISP_Init(const HALDISP_InitParams_t * const InitParams_p, HALDISP_Handle_t * const DisplayHandle_p, const U8 HALDisplayNumber);
ST_ErrorCode_t HALDISP_Term(const HALDISP_Handle_t DisplayHandle);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_DISPLAY_H */

/* End of halv_dis.h */
