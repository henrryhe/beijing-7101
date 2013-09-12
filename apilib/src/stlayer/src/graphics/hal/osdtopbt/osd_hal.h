/*****************************************************************************

File name   : osd_hal.h

Description :HW abstract layer definitions

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-03-13          Creation                                Michel Bruant

*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __OSDHAL_H
#define __OSDHAL_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stavmem.h"
#include "stevt.h"
#include "osd_vp.h"
#include "stlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

/* OSD Configuration register stuff*/
#define OSD_CFG             0x91  /* Register address */
#define OSD_CFG_LLG         0x10  /* double granularity for a 64Mbits space. */
#define OSD_CFG_ENA         0x04  /* Enable OSD plane display mask*/
#define OSD_CFG_NOR         0x02  /* Normal display mode mask*/
#define OSD_CFG_FIL         0x01  /* OSD filter mode anti flutter/flicker bit*/
#define OSD_OTP             0x2A  /* OSD Top Field Pointer Regiter address */
#define OSD_OBP             0x2B  /* OSD Bottom Field Pointer Regiter address */
#define OSD_ACT             0x3E  /* OSD Active Signal Regiter address */
#define OSD_ACT_OAM         0x40  /* Active signal mode mask*/
#define OSD_ACT_OAD         0x3F  /* Active signal delay mask [5:0] */
#define OSD_BDW             0x92  /* OSD Boundary Weight Regiter address */

/* Video Registers stuff (seen by the OSD driver) */
#define VID_DCF             0x75  /* Regiter address */
#define VID_DCF_OSDPOL      0x04  /* Pol bit mask */
#define VID_OUT             0x90  /* Register address */
#define VID_OUT_NOS         0x08  /* Not OSD bit */

/* Header spec value */
#define OSD_PALETTE_256     0x0D
#define OSD_PALETTE_16      0x25
#define OSD_PALETTE_4       0x05
#define OSD_NO_PALETTE      0x0F

/* Exported Variables ------------------------------------------------------ */

#ifdef RESERV
#define GLOBAL /* not extern */
#else
#define GLOBAL extern
#endif

GLOBAL struct
{
    semaphore_t *               ContextAcess_p;
    stosd_ViewportDesc *        OSD_ViewPort;
    BOOL                        ViewPortsUserAllocated;
    osd_RegionHeader_t *        HeadersBuffer;
    BOOL                        HeadersUserAllocated;
    STAVMEM_BlockHandle_t       HeadersHandle;
    osd_RegionHeader_t *        GhostHeader_p;
    osd_RegionHeader_t *        FirstDummyHeader_p;
    osd_RegionHeader_t *        SecondDummyHeader_p;
    U32                         stosd_BaseAddress;
    STAVMEM_PartitionHandle_t   stosd_AVMEMPartition;
    ST_Partition_t *            CPUPartition_p;
    U32                         MaxViewPorts;
    U32                         MaxHandles;
    ST_DeviceName_t             EvtHandlerName;
    STEVT_Handle_t              EvtHandle;
    ST_DeviceName_t             VTGName;
    STLAYER_LayerParams_t       LayerParams;
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    /* Output params (updated by mixer) */
    S8                          XOffset;
    S8                          YOffset;
    U32                         XStart;
    U32                         YStart;
    U32                         Width;
    U32                         Height;
    semaphore_t *               stosd_VSync_p;
    BOOL                        stosd_ActiveTopNotBottom;
    BOOL                        DriverTermination;
    stosd_ViewportDesc *        FirstViewPortEnabled;
    U32                         NumViewPortEnabled;
    BOOL                        OutputEnabled;
} stlayer_osd_context;

/* Exported Functions ------------------------------------------------------ */

ST_ErrorCode_t stlayer_EnableDisplay (void);
ST_ErrorCode_t stlayer_DisableDisplay (void);
void stlayer_HardEnable(stosd_ViewportDesc * ViewPort_p, BOOL VSyncIsTop);
void stlayer_HardDisable(void);
void stlayer_HardUpdateViewportList(stosd_ViewportDesc * ViewPort_p,
        BOOL VSyncIsTop);
void stlayer_HardInit(void);
void stlayer_HardTerm(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OSDHAL_H */

/* End of osd_hal.h */
