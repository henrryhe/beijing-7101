/*******************************************************************************

File name   : hxvp_fgt.c

Description : FGT source file
              
COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
19 Feb 2007        Created                                           JPB
                   
*******************************************************************************/

#if !defined(ST_OSLINUX)
#include <string.h>
#endif

/*#define STTBX_REPORT*/
#include "sttbx.h"

#include "stsys.h"

#include "stos.h"

#include "hxvp_main.h"
#include "hxvp_fgt.h"
#include "hxvp_xp70.h"
#include "hxvp_cluster.h"
#include "hxvp_firmware.h"

/* needed to access to VHSRC / DEI / AUXPLUG */
#include "hv_vdpreg.h"
#include "hv_vdplay.h"

const XVP_PluginFuncTable_t FGT_PluginFunc =
{
    hxvp_FGTInit,
    hxvp_FGTTerm,
    hxvp_FGTSetParam,
    hxvp_FGTSetExtraParam,
    hxvp_FGTCompute
};

/*******************************************************
 * hxvp_FGTInit                                        *
 *******************************************************/
 
ST_ErrorCode_t hxvp_FGTInit(const XVPHandle_t  XVPHandle, ProcessHandle_t *ProcessHandle_p)
{
    ST_ErrorCode_t		            ErrorCode = ST_NO_ERROR;
    XVP_XP70Init_t                  Xp70InitParam;
    XVP_clusterInit_t               ClusterInitParam;
    XVP_CommonData_t                *XVPProperties_p;
    FGT_CommonData_t                *FGTProperties_p;

    /* get properties from XVPHandle */   
    XVPProperties_p = (XVP_CommonData_t *)XVPHandle;
    if (!XVPProperties_p)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
        
    /* allocate memory */           
    FGTProperties_p = (void*)STOS_MemoryAllocate(   XVPProperties_p->CPUPartition_p,
                                                    sizeof(FGT_CommonData_t));        
    if (!FGTProperties_p)
    {
        return ST_ERROR_NO_MEMORY;
    }

    /* set properties */
    FGTProperties_p->CPUPartition_p     = XVPProperties_p->CPUPartition_p;
    FGTProperties_p->NCachePartition_p  = XVPProperties_p->NCachePartition_p;
    FGTProperties_p->LayerHandle        = XVPProperties_p->LayerHandle;
    *ProcessHandle_p = (ProcessHandle_t*)FGTProperties_p; /* return pointer on FGT handle */
   
    /* FGT needs an xp70 */
    Xp70InitParam.CPUPartition_p                = FGTProperties_p->CPUPartition_p;
    Xp70InitParam.IMEMBaseAddress               = (void*)XVP_XP70_IMEM_BASEADDRESS;
    Xp70InitParam.IMEMSize                      = XVP_XP70_IMEM_SIZE;
    Xp70InitParam.DMEMBaseAddress               = (void*)XVP_XP70_DMEM_BASEADDRESS;
    Xp70InitParam.DMEMSize                      = XVP_XP70_DMEM_SIZE;
    Xp70InitParam.MailboxBaseAddress            = (void*)XVP_XP70_MBOX_BASEADDRESS;
    Xp70InitParam.MailboxSize                   = XVP_XP70_MBOX_SIZE;
#ifdef XVP_READ_FW_FROM_FILE
    strdup(Xp70InitParam.FirmwareFileName, XVP_XP70_FIRMWARE_FILENAME);
#else
    Xp70InitParam.Firmware_p                    = xvp_fw_array;
    Xp70InitParam.FirmwareSize                  = XVP_FW_SIZE;
#endif
    STOS_memcpy(&FGTProperties_p->Xp70initParam, &Xp70InitParam, sizeof(XVP_XP70Init_t));    
    hxvp_XP70Init(&Xp70InitParam, &(FGTProperties_p->XP70Handle));
    
    /* 1 cluster */    
    ClusterInitParam.StreamerBaseAddress        = (void*)XVP_CLUSTER_STREAMER_BASEADDRESS;
    ClusterInitParam.StreamerSize               = XVP_CLUSTER_STREAMER_SIZE;
    ClusterInitParam.StbusRDPlugBaseAddress     = (void*)XVP_CLUSTER_STBUS_RDPLUG_BASEADDRESS;
    ClusterInitParam.StbusRDPlugSize            = XVP_CLUSTER_STBUS_RDPLUG_SIZE;
    ClusterInitParam.StbusWRPlugBaseAddress     = (void*)XVP_CLUSTER_STBUS_WRPLUG_BASEADDRESS;
    ClusterInitParam.StbusWRPlugSize            = XVP_CLUSTER_STBUS_WRPLUG_SIZE;
    ClusterInitParam.MicrocodeRD_p              = xvp_microcodeRD;
    ClusterInitParam.MicrocodeRDSize            = UCODE_PLUG_RD_SIZE;
    ClusterInitParam.MicrocodeWR_p              = xvp_microcodeWR;
    ClusterInitParam.MicrocodeWRSize            = UCODE_PLUG_WR_SIZE;
    hxvp_clusterInit(&ClusterInitParam);
    
    /* and 1 VDP AUX PLUG */
    FGTProperties_p->VDPAuxPlugBaseAddress      = (void*)XVP_VDP_AUX_PLUG_BASEADDRESS;
    
    /* allocate and init shared data with firmware */
    /* must be allocated in non-cachable memory, otherwise it doesn't work !!! */
    FGTProperties_p->ParamTop_p = (FGT_TransformParam_t*)STOS_MemoryAllocate(
        FGTProperties_p->NCachePartition_p, sizeof(FGT_TransformParam_t));
        
    FGTProperties_p->ParamBot_p = (FGT_TransformParam_t*)STOS_MemoryAllocate(
        FGTProperties_p->NCachePartition_p, sizeof(FGT_TransformParam_t));
        
    if (!FGTProperties_p->ParamTop_p || !FGTProperties_p->ParamBot_p)
        return ST_ERROR_NO_MEMORY;
                
    /* set default modes */
    FGTProperties_p->Mode           = STLAYER_FGT_PICTUREONLY_MODE;
    FGTProperties_p->IsBypassed     = TRUE;
    FGTProperties_p->State          = STLAYER_FGT_DISABLED_STATE;
    FGTProperties_p->ExternalMem    = FALSE;
    FGTProperties_p->Send_0         = FALSE;
    FGTProperties_p->IsStarted      = FALSE;
    
    STSYS_WriteRegDev32LE(  FGTProperties_p->VDPAuxPlugBaseAddress,
                            STLAYER_FGT_PICTUREONLY_MODE);
    STAVMEM_GetSharedMemoryVirtualMapping(&FGTProperties_p->VirtualMapping);
    
    return ErrorCode;
}

/*******************************************************
 * hxvp_FGTTerm                                        *
 *******************************************************/
ST_ErrorCode_t hxvp_FGTTerm(ProcessHandle_t ProcessHandle)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    FGT_CommonData_t    *FGTProperties_p;
    
    /* deallocate memory */
    FGTProperties_p = (FGT_CommonData_t*)ProcessHandle;
    if (FGTProperties_p)
    {
        /* BYPASS process */
        STSYS_WriteRegDev32LE(  FGTProperties_p->VDPAuxPlugBaseAddress,
                                STLAYER_FGT_PICTUREONLY_MODE);
 
        /* reset BILBO */
        hxvp_XP70Term(FGTProperties_p->XP70Handle);
        
        /* free local allocations */            
#ifdef XVP_READ_FW_FROM_FILE    
        free(Xp70InitParam.FirmwareFileName);
#endif
        STOS_MemoryDeallocate(  FGTProperties_p->NCachePartition_p,
                                FGTProperties_p->ParamTop_p);
        STOS_MemoryDeallocate(  FGTProperties_p->NCachePartition_p,
                                FGTProperties_p->ParamBot_p);
        STOS_MemoryDeallocate(  FGTProperties_p->CPUPartition_p,
                                FGTProperties_p);
    }
    
    return ErrorCode;
}

/*******************************************************
 * hxvp_FGTSetParam                                    *
 *******************************************************/
ST_ErrorCode_t hxvp_FGTSetParam(const ProcessHandle_t ProcessHandle, STLAYER_XVPParam_t *Param_p)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    STLAYER_FGTParam_t      *FGTData_p = NULL;
    FGT_Error_t             FlexVpStatus;
    char                    FlexString[32];
    FGT_TransformParam_t    *SharedParam_p;
    FGT_CommonData_t        *FGTProperties_p;
    U8                      Index;
    U32                     VerticalRatio;
    
    FGTData_p = &(Param_p->FGTParams);
    FGTData_p->IsBypassed           = FALSE; 
    
    FGTProperties_p = (FGT_CommonData_t*)ProcessHandle;
    if (!FGTProperties_p)
    {    
        return ST_ERROR_BAD_PARAMETER;
    }
    FGTProperties_p->IsBypassed     = FALSE;
    
    /* manage FGT status */
    FlexVpStatus = (FGT_Error_t)STSYS_ReadRegDev32LE(
        FGTProperties_p->Xp70initParam.MailboxBaseAddress + MBOX_INFO_xP70_OFFSET);
    
    if (FlexVpStatus != FGT_NO_ERROR)
    {
        switch(FlexVpStatus & 3)
        {
        case FGT_FIRM_TOO_SLOW :
#if !defined(ST_OSLINUX)
            strcpy(FlexString, "FIRMWARE TOO SLOW");
#endif
            break;
            
        case FGT_DOWNGRADED_GRAIN :
#if !defined(ST_OSLINUX)
            strcpy(FlexString, "DOWNGRADED GRAIN");
#endif
            break;
                        
        default :
#if !defined(ST_OSLINUX)
            strcpy(FlexString, "UNKNOWN STATUS");
#endif
            break;
        }
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FGT Status : %s",FlexString));
        
        /* reset status */
        STSYS_WriteRegDev32LE(FGTProperties_p->Xp70initParam.MailboxBaseAddress + MBOX_INFO_xP70_OFFSET, 0);
    }
    
    /* select buffer between Button or Top field */
    SharedParam_p = (FGTData_p->FieldType == XVP_TOP_FIELD) ?
        FGTProperties_p->ParamTop_p : FGTProperties_p->ParamBot_p;
    FGTProperties_p->CurrentParam_p = SharedParam_p;
    
    /* compute vertical ratio : HW limitation, temporary code */
    /* To avoid using float variable, multiply by constant 10 is a not bad turn around */
    VerticalRatio = (FGTData_p->ViewportOutHeight*10) / FGTData_p->ViewportInHeight;

    /* set FGT parameters */    
    if (    (FGTData_p->State != TRUE) ||
            (FGTData_p->Flags & FGT_FLAGS_BYPASS_MASK) ||
            (FGTProperties_p->State == STLAYER_FGT_DISABLED_STATE) ||
            (VerticalRatio < 8) /* HW limitation */)
    {  
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "FGT BYPASSED"));
        FGTProperties_p->IsBypassed     = TRUE;
        FGTData_p->IsBypassed           = TRUE;
        
        return ErrorCode;
    }    

    /* set / compute FLAGS param */             
    SharedParam_p->Flags =      FGTData_p->Flags  |
                                FGT_FLAGS_VDP_MASK |
                                FGT_FLAGS_BYPASS_DEI_MASK;

    if (FGTData_p->FieldType == XVP_TOP_FIELD)            
        SharedParam_p->Flags |= FGT_FLAGS_TOP_FIELD_MASK;               
    else
        SharedParam_p->Flags &= ~FGT_FLAGS_TOP_FIELD_MASK;                           
                                
    SharedParam_p->Flags &=     FGT_FLAGS_BYPASS_MASK |
                                FGT_FLAGS_DOWNLOAD_LUTS_MASK |
                                FGT_FLAGS_LUMA_GRAIN_MASK |
                                FGT_FLAGS_CHROMA_GRAIN_MASK |
                                FGT_FLAGS_VDP_MASK |
                                FGT_FLAGS_EXT_MEM_MASK |
                                FGT_FLAGS_BYPASS_DEI_MASK |
                                FGT_FLAGS_PROGRESSIVE_DISPLAY_MASK |
                                FGT_FLAGS_TOP_FIELD_MASK;                                                

    /* set other FLAGS */
    SharedParam_p->ViewPortOrigin        = (FGTData_p->ViewportInY << 16) | FGTData_p->ViewportInX;     
    SharedParam_p->ViewPortSize          = (FGTData_p->ViewportInHeight << 16) | FGTData_p->ViewportInWidth;

    SharedParam_p->LumaAccBufferAddr     = FGTData_p->LumaAccBufferAddr;
    SharedParam_p->ChromaAccBufferAddr   = FGTData_p->ChromaAccBufferAddr;
    SharedParam_p->PictureSize           = FGTData_p->PictureSize;
    SharedParam_p->UserZoomOut           = 0x08; /* temporary value ?! */
    SharedParam_p->UserZoomOutThreshold  =   (INTL_THRESHOLD2 << FGT_USER_ZOOM_INTL_THR2_BIT) +
                                             (INTL_THRESHOLD1 << FGT_USER_ZOOM_INTL_THR1_BIT) +
                                             (PROG_THRESHOLD2 << FGT_USER_ZOOM_PROG_THR2_BIT) +
                                             (PROG_THRESHOLD1 << FGT_USER_ZOOM_PROG_THR1_BIT);
    SharedParam_p->LutAddr               = (U32)STAVMEM_CPUToDevice(FGTData_p->LutAddr, &FGTProperties_p->VirtualMapping);
    SharedParam_p->LumaSeed              = FGTData_p->LumaSeed;
    SharedParam_p->CbSeed                = FGTData_p->CbSeed;
    SharedParam_p->CrSeed                = FGTData_p->CrSeed;
    SharedParam_p->Log2ScaleFactor       = FGTData_p->Log2ScaleFactor;
    SharedParam_p->LumaGrainBufferAddr   = 0; /* not used, only for external memory mode */
    SharedParam_p->CbGrainBufferAddr     = 0; /* not used, only for external memory mode */
    SharedParam_p->CrGrainBufferAddr     = 0; /* not used, only for external memory mode */            
    SharedParam_p->GrainPatternCount     = FGTData_p->GrainPatternCount;
    for (Index = 0; Index < SharedParam_p->GrainPatternCount; Index++) 
    { 
        SharedParam_p->GrainPatternAddrArray[Index] =
            (U32)STAVMEM_CPUToDevice(   FGTData_p->GrainPatternAddrArray[Index],
                                        &FGTProperties_p->VirtualMapping);
    }    
    for (Index = SharedParam_p->GrainPatternCount; Index < STLAYER_FGT_MAX_GRAIN_PATTERN; Index++) 
    { 
        SharedParam_p->GrainPatternAddrArray[Index] = 0;
    }
                           
    return ErrorCode;
}

/*******************************************************
 * hxvp_FGTSetExtraParam                               *
 *******************************************************/
ST_ErrorCode_t hxvp_FGTSetExtraParam(const ProcessHandle_t ProcessHandle, STLAYER_XVPExtraParam_t *ExtraParam_p)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    FGT_CommonData_t            *FGTProperties_p;
    
    FGTProperties_p = (FGT_CommonData_t*)ProcessHandle;
    if (!FGTProperties_p || !ExtraParam_p) return ST_ERROR_BAD_PARAMETER; 

    switch(ExtraParam_p->FGTExtraParams.Type)
    {
    case STLAYER_FGT_EXTRAPARAM_MODE :
        /* set mode */
        FGTProperties_p->Mode = ExtraParam_p->FGTExtraParams.Mode;        
        break;
        
    case STLAYER_FGT_EXTRAPARAM_STATE :
        /* set state : ENABLED or DISABLED */
        FGTProperties_p->State = ExtraParam_p->FGTExtraParams.State;
        FGTProperties_p->Mode = (FGTProperties_p->State) ? 
            STLAYER_FGT_PICTUREANDGRAIN_MODE : STLAYER_FGT_PICTUREONLY_MODE;
        break;
            
    default :
        return ST_ERROR_BAD_PARAMETER;
    }
    return ErrorCode;
}

/*******************************************************
 * hxvp_FGTCompute                                     *
 *******************************************************/
ST_ErrorCode_t hxvp_FGTCompute(const ProcessHandle_t ProcessHandle)
{
    ST_ErrorCode_t          ErrorCode = ST_NO_ERROR;
    FGT_TransformParam_t    *SharedParam_p;
    FGT_CommonData_t        *FGTProperties_p;
      
    FGTProperties_p = (FGT_CommonData_t*)ProcessHandle;
    if (!FGTProperties_p)
    {    
        return ST_ERROR_BAD_PARAMETER;
    }
    
    /* select current buffer (selected during SetParam) */
    SharedParam_p = FGTProperties_p->CurrentParam_p;
 
    if (FGTProperties_p->IsBypassed == TRUE)
    {
        /* set VDPAuxPlug to right mode PROCESS_ONLY */
        STSYS_WriteRegDev32LE(  FGTProperties_p->VDPAuxPlugBaseAddress,
                                STLAYER_FGT_PICTUREONLY_MODE);

        if (FGTProperties_p->IsStarted == TRUE)
        {
            /* program anyway the bilbo, because it starts on each hard VSYNC */
            SharedParam_p->Flags = 1 << FGT_FLAGS_BYPASS_BIT;
            STSYS_WriteRegDev32LE(  FGTProperties_p->Xp70initParam.MailboxBaseAddress + MBOX_NEXT_CMD_OFFSET,
                                    (U32)STAVMEM_VirtualToDevice(SharedParam_p, &FGTProperties_p->VirtualMapping));                                                                            
        }                                                                    
    }
    else
    {
        /* compute other data */
        HALLAYER_LayerProperties_t      *LayerProperties_p;
        HALLAYER_VDPLayerCommonData_t   *LayerPrivateData_p;
        HALLAYER_VDPVhsrcRegisterMap_t  *VhsrcRegisterMap_p;   
        
        /* set params from VDP properties */        
        LayerProperties_p = (HALLAYER_LayerProperties_t *)FGTProperties_p->LayerHandle;
        if (!LayerProperties_p)
        {             
            return(ST_ERROR_BAD_PARAMETER);
        }
        LayerPrivateData_p = (HALLAYER_VDPLayerCommonData_t *)LayerProperties_p->PrivateData_p;
        if (LayerPrivateData_p == NULL)
        {                
            return(ST_ERROR_BAD_PARAMETER);
        }

        /* get VHSRC params */                                                      
        VhsrcRegisterMap_p = (SharedParam_p->Flags & FGT_FLAGS_TOP_FIELD_MASK) ?
            &LayerPrivateData_p->DisplayInstance[0].RegisterConfiguration.VhsrcRegisterMapTop :
            &LayerPrivateData_p->DisplayInstance[0].RegisterConfiguration.VhsrcRegisterMapBottom;

        SharedParam_p->LumaZoom = ((VhsrcRegisterMap_p->RegDISP_VHSRC_Y_VSRC & 0x0000FFFF) << 16) |
                                  (VhsrcRegisterMap_p->RegDISP_VHSRC_Y_HSRC & 0x0000FFFF);

        if (    LayerPrivateData_p->SourceFormatInfo.IsInputScantypeProgressive
                && LayerPrivateData_p->SourceFormatInfo.IsInputScantypeConsideredAsProgressive)
        {
            SharedParam_p->Flags |= FGT_FLAGS_PROGRESSIVE_DISPLAY_MASK;
        }

        /* send param to firmware */ 
        STSYS_WriteRegDev32LE(  FGTProperties_p->Xp70initParam.MailboxBaseAddress + MBOX_NEXT_CMD_OFFSET,
                                (U32)STAVMEM_VirtualToDevice(SharedParam_p, &FGTProperties_p->VirtualMapping));               

        /* start process */              
        if (FGTProperties_p->IsStarted == FALSE)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "START FGT"));
            hxvp_XP70Start(FGTProperties_p->XP70Handle);
            FGTProperties_p->IsStarted = TRUE;
        }
                                     
        /* set VDPAuxPlug to right mode PICTURE_ONLY (bypassed), PROCESS_ONLY or BOTH */
        STSYS_WriteRegDev32LE(  FGTProperties_p->VDPAuxPlugBaseAddress,
                                FGTProperties_p->Mode);
    }
                               
    return ErrorCode;
}

