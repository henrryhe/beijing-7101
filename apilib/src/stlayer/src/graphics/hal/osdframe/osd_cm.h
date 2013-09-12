/*****************************************************************************

File name   : osd_cm.h

Description :

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                 Name
----               ------------                                 ----
2001-12-20          Creation                                    Michel Bruant

*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __OSDCM_H
#define __OSDCM_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes --------------------------------------------------------------- */
#include "stevt.h"
#include "osd_vp.h"
#include "stlayer.h"

/* Variables --------------------------------------------------------------- */
#ifdef RESERV
#define GLOBAL /* not extern */
#else
#define GLOBAL extern
#endif

GLOBAL struct
{
    /* Init params */
    U32                         stosd_BaseAddress;
    STAVMEM_PartitionHandle_t   stosd_AVMEMPartition;
    ST_Partition_t *            CPUPartition_p;
    U32                         MaxViewPorts;
    /* V synchro */
    ST_DeviceName_t             EvtHandlerName;
    ST_DeviceName_t             VTGName;
    STEVT_Handle_t              EvtHandle;
    semaphore_t *               stosd_VSync_p;
    /* buffers */
    stosd_ViewportDesc *        OSD_ViewPort;
    BOOL                        ViewPortsUserAllocated;
    osd_RegionHeader_t *        HeadersBuffer;
    BOOL                        HeadersUserAllocated;
    STAVMEM_BlockHandle_t       HeadersHandle;
    /* osd variables */
    STAVMEM_SharedMemoryVirtualMapping_t VirtualMapping;
    BOOL                        DriverTermination;
    STLAYER_LayerParams_t       LayerParams;
    BOOL                        UpdateBottom;
    BOOL                        UpdateTop;
    BOOL                        stosd_ActiveTopNotBottom;
    stosd_ViewportDesc *        FirstViewPortLinked;
    U32                         NumViewPortLinked;
    U32                         NumViewPortEnabled;
    osd_RegionHeader_t *        GhostHeader_p;
    semaphore_t *               VPDecriptorAccess_p;
    U32                         OTP_Image;
    U32                         OBP_Image;
    BOOL                        EnableRGB16Mode;
    /* Output params (updated by mixer) */
    S8                          XOffset;
    S8                          YOffset;
    U32                         XStart;
    U32                         YStart;
    U32                         Width;
    U32                         Height;
    U32                         FrameRate;
    U32                         OutputEnabled;
    BOOL                        LayerRecordable[2];
} stlayer_osd_context;


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __OSDCM_H */

/* End of osd_cm.h */
