/*******************************************************************************

File name   : hxvp_fgt.h

Description : FGT hearder file
              (Film Grain Technology)
              
COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __HXVP_FGT_H
#define __HXVP_FGT_H

/* Includes ----------------------------------------------------------------- */
#include "sti7200.h"
#include "stvid.h"
#include "stlayer.h"
#include "hxvp_main.h"
#include "hxvp_xp70.h"
#include "FGT_VideoTransformerTypes.h" /* file from firmware dev */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HW_7200

    #define FLEXVP_VERSION                          20
    #define XVP_BASEADDRESS                         ST7200_FLEX_VPE_BASE_ADDRESS

    #define XVP_VDP_BASEADDRESS                     XVP_BASEADDRESS + 0x00006000
    #define XVP_VDP_T3_DEI_BASEADDRESS              XVP_VDP_BASEADDRESS + 0x00000000
    #define XVP_VDP_AUX_PLUG_BASEADDRESS            XVP_VDP_BASEADDRESS + 0x00000500 
    #define XVP_VDP_AUX_PLUG_SIZE                   0x0001FFF
    
    #define XVP_CLUSTER_STREAMER_BASEADDRESS        XVP_BASEADDRESS + 0x00000000
    #define XVP_CLUSTER_STREAMER_SIZE               0x0000FFF
    #define XVP_CLUSTER_STBUS_WRPLUG_BASEADDRESS    XVP_BASEADDRESS + 0x00002000
    #define XVP_CLUSTER_STBUS_WRPLUG_SIZE           0x0001FFF
    #define XVP_CLUSTER_STBUS_RDPLUG_BASEADDRESS    XVP_BASEADDRESS + 0x00004000
    #define XVP_CLUSTER_STBUS_RDPLUG_SIZE           0x0001FFF
    
    #define XVP_XP70_MBOX_BASEADDRESS               XVP_BASEADDRESS + 0x00001000
    #define XVP_XP70_MBOX_SIZE                      0x00000FFF
    #define XVP_XP70_IMEM_BASEADDRESS               XVP_BASEADDRESS + 0x00030000
    #define XVP_XP70_IMEM_SIZE                      0x00001000
    #define XVP_XP70_DMEM_BASEADDRESS               XVP_BASEADDRESS + 0x00020000
    #define XVP_XP70_DMEM_SIZE                      0x00004000
    #ifdef XVP_READ_FW_FROM_FILE
        #define XVP_XP70_FIRMWARE_FILENAME          "../../FGT_VideoTransformer_bc.bin"
    #endif
    
#endif /* HW_7200 */

#define INTL_THRESHOLD1     0x09 
#define	INTL_THRESHOLD2     0x0B 
#define	PROG_THRESHOLD1     0x0B 
#define	PROG_THRESHOLD2     0x12

typedef struct
{
    ST_Partition_t                          *CPUPartition_p;
    ST_Partition_t                          *NCachePartition_p;
    STLAYER_Handle_t                        LayerHandle;
    XVP_XP70Init_t                          Xp70initParam;
    XP70Handle_t                            XP70Handle;
    void                                    *VDPAuxPlugBaseAddress;
    FGT_TransformParam_t                    *ParamTop_p;
    FGT_TransformParam_t                    *ParamBot_p;
    STLAYER_FGTMode_t                       Mode;
    STLAYER_FGTState_t                      State;
    BOOL                                    IsStarted;
    BOOL                                    ExternalMem;
    BOOL                                    Send_0; /* for debug ?! */
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    BOOL                                    IsBypassed;
    FGT_TransformParam_t                    *CurrentParam_p;
}FGT_CommonData_t;

/* Exported Variables ------------------------------------------------------- */
extern const XVP_PluginFuncTable_t FGT_PluginFunc;

/* prototypes ----------------------------------------------------------------*/   
ST_ErrorCode_t hxvp_FGTInit(const XVPHandle_t  XVPHandle, ProcessHandle_t *ProcessHandle_p);
ST_ErrorCode_t hxvp_FGTTerm(ProcessHandle_t ProcessHandle);
ST_ErrorCode_t hxvp_FGTSetParam(ProcessHandle_t ProcessHandle, STLAYER_XVPParam_t *Param_p);
ST_ErrorCode_t hxvp_FGTSetExtraParam(ProcessHandle_t ProcessHandle, STLAYER_XVPExtraParam_t *ExtraParam_p);
ST_ErrorCode_t hxvp_FGTCompute(ProcessHandle_t ProcessHandle);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __XVP_FGT_H */
