/*****************************************************************************

File name : hal_vos.h

Description :  HAL for Video Output Stage VTG programmation

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
28 Jun 2001        Creation from vtg_hal2.h                         HSdLM
04 Sep 2001        Move interrupt init routine in 'vos'             HSdLM
09 Oct 2001        Use STINTMR for VFE                              HSdLM
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_HAL_VOS_H
#define __VTG_HAL_VOS_H

/* Includes --------------------------------------------------------------- */
#include "stvtg.h"
#include "vtg_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */
#if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
typedef enum HVSyncType_e
{
  HSYNC_VSYNC_REF  = 0x1,
  HSYNC_VSYNC_1    = 0x2,
  HSYNC_VSYNC_2    = 0x4,
  HSYNC_VSYNC_3    = 0x8,
  HSYNC_VSYNC_PAD  = 0x10
}HVSyncType_t;


typedef struct SyncPosition_s
{
    U32  VerticalPositionEdge;
    U32  HorizontalPositionEdge;
}SyncPosition_t;

typedef struct OutEdgePosition_s
{
    U32                HSyncRising;
    U32                HSyncFalling;
    SyncPosition_t     VSyncTopRising;
    SyncPosition_t     VSyncTopFalling;
    SyncPosition_t     VSyncBotRising;
    SyncPosition_t     VSyncBotFalling;
}OutEdgePosition_t;

typedef struct VideoActiveParams_s
{
  STVTG_TimingMode_t  TimingMode;
  OutEdgePosition_t   OutEdgePosition1;
  OutEdgePosition_t   OutEdgePosition2;
  OutEdgePosition_t   OutEdgePosition3;
  HVSyncType_t        SyncEnabled;
}VideoActiveParams_t;
#endif


/* Exported Functions  -----------------------------------------------------*/

ST_ErrorCode_t stvtg_HALGetVtgId(  const void *    const BaseAddress_p
                                 , STVTG_VtgId_t * const VtgId_p
                                );

void stvtg_HALVosSetCapability( stvtg_Device_t * const Device_p);

ST_ErrorCode_t stvtg_HALVosCheckSlaveModeParams(   const STVTG_SlaveModeParams_t * const SlaveModeParams_p
                                                 , const STVTG_VtgId_t VtgId
                                               );

ST_ErrorCode_t stvtg_HALVosSetModeParams(  stvtg_Device_t *            const Device_p
                                         , const STVTG_TimingMode_t Mode
                                        );

ST_ErrorCode_t stvtg_HALVosSetAWGSettings( const stvtg_Device_t * const Device_p);

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VTG_HAL_VOS_H */
