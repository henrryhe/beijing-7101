/*******************************************************************************

File name   : mixgamma.c

Description : video mixer driver for gamma type cell

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
08 Nov 2000         Created                                          BS
...
15 Nov 2001        Add semaphore with timeout to avoid blocking of
                   the task used by mixer                            BS
19 Mar  2002       Compilation chip selection exported in makefile   BS
                   GNBvd11072 : Video1 on mixer 2 not allowed on 7015/20
******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#include "mixgamma.h"
#include "stevt.h"
#include "sttbx.h"
#include "stsys.h"
#include "stvtg.h"

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/*Trace Dynamic Data Size---------------------------------------------------- */
#ifdef TRACE_DYNAMIC_DATASIZE
    U32  DynamicDataSize ;
    #define AddDynamicDataSize(x)       DynamicDataSize += (x)
#else
    #define AddDynamicDataSize(x)
#endif



/**********************************/
/* Register configuration for ALL */
/**********************************/
#define GAM_MIX_CTL                 0x00
#define GAM_MIX_AVO                 0x28
#define GAM_MIX_AVS                 0x2C
#define GAM_MIX_BKC                 0x04
#define GAM_MIX_BCO                 0x0C
#define GAM_MIX_BCS                 0x10
#define GAM_MIX_CTL_RESET_MASK      0x00000000
#define GAM_MIX_CTL_BGC             0x00000001
#define GAM_MIX_CTL_CURSOR          0x00000040
#define GAM_MIX_CTL_BGC_ACTIV       0x00010000
#define GAM_MIX_CTL_CUR_ACTIV       0x00400000

/***********************************/
/* Software configuration for ALL  */
/***********************************/
#define MIX_U16_MASK                0xFFFF0000

/***********************************/
/* Software configuration for 7015 */
/***********************************/
#define MIX_7015_DSPCFG_OFFSET      0x00006100
#define MIX_7015_GAMMA_OFFSET       0x0000A000
#define GAMMA_7015_MIXER1_OFFSET    (MIX_7015_GAMMA_OFFSET+0x700)
#define GAMMA_7015_MIXER2_OFFSET    (MIX_7015_GAMMA_OFFSET+0x800)
#define GAMMA_7015_ID_MASK          0xFFF
#define GAMMA_7015_ID_GFX1          0x000
#define GAMMA_7015_ID_GFX2          0x500

/***********************************/
/* Software workaround for 7015    */
/***********************************/
#ifdef WA_GNBvd06549
#define GAM_7015_AVO_WA_MASK        0x06000E00
#endif /* WA_GNBvd06549 */

/***********************************/
/* Register configuration for 7015 */
/***********************************/
#define GAM_7015_CTL_GFX2           0x00000002
#define GAM_7015_CTL2_GFX2          0x00000020
#define GAM_7015_CTL_VIDEO1         0x00000004
#define GAM_7015_CTL_VIDEO2         0x00000008
#define GAM_7015_CTL_GFX1           0x00000010
#define GAM_7015_CTL_HDnotSD        0x00004000
#define GAM_7015_CTL_GFX2_ACTIV     0x00020000
#define GAM_7015_CTL_VID1_ACTIV     0x00040000
#define GAM_7015_CTL_VID2_ACTIV     0x00080000
#define GAM_7015_CTL_GFX1_ACTIV     0x00100000

#define GAM_7015_CTL_ZVIDEO1        0x00000400
#define GAM_7015_CTL_ZVIDEO2        0x00000200
#define GAM_7015_CTL_ZGFXVID2       0x00000100
#define GAM_7015_CTL_ZMASK          (GAM_7015_CTL_ZVIDEO1 | GAM_7015_CTL_ZVIDEO2 | GAM_7015_CTL_ZVIDEO2)
#define GAM_7015_CTL_VIDEO_MSK      (GAM_7015_CTL_VIDEO1 | GAM_7015_CTL_VIDEO2)
#define GAM_7015_CTL1_RESET_MSK     GAM_7015_CTL_HDnotSD /* Always to 1. WA_GNBvd06551*/

#define GAM_7015_DSP_CFG_GFX2       0x00000020
#define GAM_7015_DSP_CFG_PNR        0x00000001
#define GAM_7015_DSP_CFG_DSD        0x00000002
#define GAM_7015_DSP_CFG_DESL       0x00000004
#define GAMMA_7015_XDS_MSK          0xFFE
#define GAM_7015_MIX1_AV_MASK       0xF800F000
#define GAM_7015_MIX2_AV_MASK       0xFC00FC00

/***********************************/
/* Software configuration for ST40 */
/***********************************/
#define GAMMA_ST40_ID_MASK          0xFFF
#define GAMMA_ST40_ID_GDP1          0x100
#define GAMMA_ST40_ID_GDP2          0x200
#define GAMMA_ST40_ID_GDP3          0x300
#define GAMMA_ST40_ID_GDP4          0x400

/***********************************/
/* Register configuration for ST40 */
/***********************************/
#define GAMMA_ST40_CTL_GDP1         0x00000010
#define GAMMA_ST40_CTL_GDP2         0x00000008
#define GAMMA_ST40_CTL_GDP3         0x00000004
#define GAMMA_ST40_CTL_GDP4         0x00000002
#define GAM_ST40_CTL_GDP4_ACTIV     0x00020000
#define GAM_ST40_CTL_GDP3_ACTIV     0x00040000
#define GAM_ST40_CTL_GDP2_ACTIV     0x00080000
#define GAM_ST40_CTL_GDP1_ACTIV     0x00100000
#define GAM_ST40_AV_MASK            0xFC00F800

/* Private Function prototypes ---------------------------------------------- */
typedef struct
{
    U32 RegCTL;
    STEVT_Handle_t EvtHandle;
    BOOL VSyncToProcess;
    BOOL VSyncRegistered;
    semaphore_t * VSyncProcessed_p; /* Not used in 7015 */
    BOOL TestVsync;             /* Not used in 7015 */
} stvmix_HalGamma_t;

#define ReadRegDev32LE(Add) STSYS_ReadRegDev32LE((U32)Device_p->InitParams.BaseAddress_p \
                            +(U32)Device_p->InitParams.DeviceBaseAddress_p + Add);

#define ReadRegBase32LE(Add) STSYS_ReadRegDev32LE((U32)Device_p->InitParams.DeviceBaseAddress_p + Add);

#define WriteRegDev32LE(Add,Value)  STSYS_WriteRegDev32LE((U32)Device_p->InitParams.BaseAddress_p \
                                    +(U32)Device_p->InitParams.DeviceBaseAddress_p + Add, Value);

#define WriteRegBase32LE(Add,Value)  STSYS_WriteRegDev32LE((U32)Device_p->InitParams.DeviceBaseAddress_p + Add, Value);


/* Private functions */
static ST_ErrorCode_t  stvmix_Gamma7015ConnectLayerMix1(
    stvmix_Device_t * const Device_p, stvmix_HalLayerConnect Purpose);
static ST_ErrorCode_t  stvmix_Gamma7015ConnectLayerMix2(
    stvmix_Device_t * const Device_p, stvmix_HalLayerConnect Purpose);
static ST_ErrorCode_t  stvmix_GammaGX1ConnectLayerMix(
    stvmix_Device_t * const Device_p, stvmix_HalLayerConnect Purpose);

/*******************************************************************************
Name        : stvmix_GammaReceiveEvtVSync
Description : Vsync event callback
Parameters  : Reason, RegistrantName, Event, EventData and Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
static void stvmix_GammaReceiveEvtVSync(STEVT_CallReason_t Reason,
                                        const ST_DeviceName_t RegistrantName,
                                        STEVT_EventConstant_t Event,
                                        const void *EventData,
                                        const void *SubscriberData_p)
{
    STVTG_VSYNC_t EventType;
    stvmix_Device_t*  Device_p;
    stvmix_HalGamma_t * Hal_p;

    EventType = *((STVTG_VSYNC_t*) EventData);
    Device_p = ((stvmix_Device_t*) SubscriberData_p);

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGamma_t *)(Device_p->Hal_p));

    if(Hal_p->VSyncToProcess == TRUE)
    {
        if(Device_p->ScreenParams.ScanType == STGXOBJ_INTERLACED_SCAN)
        {
            if(EventType == STVTG_BOTTOM)
            {
                if(Hal_p->TestVsync == FALSE)
                {
                    /* Store new value for CTL */
                    WriteRegDev32LE(GAM_MIX_CTL, Hal_p->RegCTL);
                }
                else
                {
                    /* Back to default mode */
                    Hal_p->TestVsync = FALSE;
                }

                /* Unsubscribe event */
                Hal_p->VSyncToProcess = FALSE;

                if(Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_GX1)
                {
                    /* Only ST40 is blocked by semaphore */
                    STOS_SemaphoreSignal(Hal_p->VSyncProcessed_p);
                }
            }
        }
        else
        {
            if(Hal_p->TestVsync == FALSE)
            {
                /* Store new value for CTL */
                WriteRegDev32LE(GAM_MIX_CTL, Hal_p->RegCTL);
            }
            else
            {
                /* Back to default mode */
                Hal_p->TestVsync = FALSE;
            }

            /* Unsubscribe event */
            Hal_p->VSyncToProcess = FALSE;

            if(Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_GX1)
            {
                /* Only ST40 is blocked by semaphore */
                STOS_SemaphoreSignal(Hal_p->VSyncProcessed_p);
            }
        }
    }
}

/******************************************************************************/
/************************ BEGIN OF GAMMA FUNCTIONS ****************************/
/******************************************************************************/

/*******************************************************************************
Name        : stvmix_GammaConnectBackground
Description : Enable background for gamma device
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_GammaConnectBackground(stvmix_Device_t * const Device_p)
{
    U32 RegCTL;

    if((Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER2_OFFSET) &&
       (Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015))
    {
        return;
    }

    /* New value of CTL at next VSync if already programmed */
    ((stvmix_HalGamma_t*)(Device_p->Hal_p))->RegCTL |= GAM_MIX_CTL_BGC;

    RegCTL = ReadRegDev32LE(GAM_MIX_CTL);
    if(Device_p->Background.Enable == TRUE)
    {
        RegCTL |= GAM_MIX_CTL_BGC;
    }
    else
    {
        RegCTL &= ~((U32)GAM_MIX_CTL_BGC);
    }
    /* Store new value for CTL */
    WriteRegDev32LE(GAM_MIX_CTL, RegCTL);

}

/*******************************************************************************
Name        : stvmix_GammaConnectLayerMix
Description : Test and set connection of layer on gamma device
Parameters  : Pointer on device, Purpose of connection
Assumptions : Device is valid, Hal pointer is valid
Limitations : In LAYER_SET_REGISTER mode, no error should be returned
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t  stvmix_GammaConnectLayerMix(stvmix_Device_t * const Device_p,
                                            stvmix_HalLayerConnect Purpose)
{
    ST_ErrorCode_t RetErr;

    if((((stvmix_HalGamma_t *)(Device_p->Hal_p))->VSyncRegistered == FALSE) &&
       (Device_p->Status & API_STVTG_CONNECT))
    {
        RetErr = stvmix_GammaEvt(Device_p, VSYNC_SUBSCRIBE, Device_p->VTG);
        if(RetErr != ST_NO_ERROR)
        {
            return(RetErr);
        }
    }

    switch((U32)Device_p->InitParams.DeviceType)
    {
        case STVMIX_GAMMA_TYPE_7015 :
            /* Which mixer of the 7015 ? */
            if((U32)Device_p->InitParams.BaseAddress_p == GAMMA_7015_MIXER2_OFFSET)
            {
                return(stvmix_Gamma7015ConnectLayerMix2(Device_p, Purpose));
            }
            else
            {
                return(stvmix_Gamma7015ConnectLayerMix1(Device_p, Purpose));
            }
            break; /* Never reached */

        case STVMIX_GAMMA_TYPE_GX1 :
            return(stvmix_GammaGX1ConnectLayerMix(Device_p, Purpose));
            break; /* Never reached */

        default:
            break;
    }

    /* No function found */
    /* Return an error */
    return(ST_ERROR_BAD_PARAMETER);
}

/*******************************************************************************
Name        : stvmix_Gamma7015ConnectLayerMix1
Description : Test and set connection of layer on 7015 gamma device
Parameters  : Pointer on device, Purpose of connection
Assumptions : Device is valid, Hal pointer is valid
Limitations : In LAYER_SET_REGISTER mode, no error should be returned
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t  stvmix_Gamma7015ConnectLayerMix1(stvmix_Device_t * const Device_p,
                                                        stvmix_HalLayerConnect Purpose)
{
    U32 i, RegCTL, NbLayers;
    STLAYER_Layer_t Type;

    U32 DspCfg, DetectPos = 0;

    /* Inits */
    RegCTL = ReadRegDev32LE(GAM_MIX_CTL);

    RegCTL &= (GAM_7015_CTL1_RESET_MSK | GAM_MIX_CTL_BGC);

    switch(Purpose)
    {
        case LAYER_CHECK:
            i = NbLayers = Device_p->Layers.NbTmp;
            break;
        case LAYER_SET_REGISTER:
            i = NbLayers = Device_p->Layers.NbConnect;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    while(i != 0)
    {
        i--;
        /* Get Layer Type */
        switch(Purpose)
        {
            case LAYER_CHECK:
                Type = Device_p->Layers.TmpArray_p[i].Type;
                break;
            case LAYER_SET_REGISTER:
                Type = Device_p->Layers.ConnectArray_p[i].Type;
                break;
            default :
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }

        /* Compute register CTL in function of layer Type */
        switch(Type)
        {
            case STLAYER_OMEGA2_VIDEO1 :
                if(DetectPos == 0)
                {
                    RegCTL |= GAM_7015_CTL_ZVIDEO1;
                }
                if(RegCTL & GAM_7015_CTL_VIDEO1)
                    return(ST_ERROR_BAD_PARAMETER);
                RegCTL |= GAM_7015_CTL_VIDEO1;
                if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                {
                    /* No pin out for video active on 7015 */
                    RegCTL |= GAM_7015_CTL_VID1_ACTIV;
                }
                DetectPos++;
                break;

            case STLAYER_OMEGA2_VIDEO2 :
                if(DetectPos == 0)
                {
                    RegCTL |= GAM_7015_CTL_ZVIDEO2;
                }
                if(RegCTL & GAM_7015_CTL_VIDEO2)
                    return(ST_ERROR_BAD_PARAMETER);
                if((DetectPos == 2)&&((RegCTL & GAM_7015_CTL_ZMASK) == 0))
                {
                    RegCTL |= GAM_7015_CTL_ZGFXVID2;
                }
                RegCTL |= GAM_7015_CTL_VIDEO2;
                if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                {
                    /* No pin out for video active on 7015 */
                    RegCTL |= GAM_7015_CTL_VID2_ACTIV;
                }
                DetectPos++;
                break;

            case  STLAYER_GAMMA_BKL :
                /* BKLs layers equal to GFXs */
                /* Information DeviceId isolate GFX1 & GFX2 */
                if((Device_p->Layers.TmpArray_p[i].DeviceId & GAMMA_7015_ID_MASK) == GAMMA_7015_ID_GFX1)
                {
                    if(RegCTL & GAM_7015_CTL_GFX1)
                        return(ST_ERROR_BAD_PARAMETER);
                    RegCTL |=  GAM_7015_CTL_GFX1 ;
                    if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                    {
                        RegCTL |= GAM_7015_CTL_GFX1_ACTIV;
                    }
                    if(DetectPos == 2)
                        RegCTL |= GAM_7015_CTL_ZGFXVID2;
                    DetectPos++;
                }
                if((Device_p->Layers.TmpArray_p[i].DeviceId & GAMMA_7015_ID_MASK) == GAMMA_7015_ID_GFX2)
                {
                    if(RegCTL & GAM_7015_CTL_GFX2)
                        return(ST_ERROR_BAD_PARAMETER);
                    if(i != 0)
                        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                    RegCTL |=  GAM_7015_CTL_GFX2 ;
                    if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                    {
                        RegCTL |= GAM_7015_CTL_GFX2_ACTIV;
                    }
                }
                break;

            case  STLAYER_GAMMA_CURSOR :
                /* Cursor layer should be the closest to the eye */
                if(RegCTL & GAM_MIX_CTL_CURSOR)
                    return(ST_ERROR_BAD_PARAMETER);
                if(NbLayers != (i+1))
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                RegCTL |=  GAM_MIX_CTL_CURSOR ;
                if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                {
                    RegCTL |= GAM_MIX_CTL_CUR_ACTIV;
                }
                break;

            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }
    }

    /* Enable background color */
    if(Device_p->Background.Enable == TRUE)
        RegCTL |= GAM_MIX_CTL_BGC;

    if(Purpose == LAYER_SET_REGISTER)
    {
        /* When Video2 is displayed on mixer 1, VTG1 should be used (PIP feature) */
        /* Protect access */
        STOS_InterruptLock();
        /* Register DSP_CFG to be updated */
        DspCfg = ReadRegBase32LE(MIX_7015_DSPCFG_OFFSET);
        if(RegCTL & GAM_7015_CTL_VIDEO2)
            DspCfg |= GAM_7015_DSP_CFG_PNR;
        else
            DspCfg &= ~((U32)GAM_7015_DSP_CFG_PNR);
        WriteRegBase32LE(MIX_7015_DSPCFG_OFFSET, DspCfg);
        /* Unprotect access */
        STOS_InterruptUnlock();

        if(((stvmix_HalGamma_t*)(Device_p->Hal_p))->VSyncRegistered == TRUE)
        {
            /* New value of CTL at next VSync if already registered */
            ((stvmix_HalGamma_t*)(Device_p->Hal_p))->RegCTL = RegCTL;
            ((stvmix_HalGamma_t*)(Device_p->Hal_p))->VSyncToProcess = TRUE;

        }
        else
        {
            /* Store new value for CTL */
            WriteRegDev32LE(GAM_MIX_CTL, RegCTL);
        }
    }

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stvmix_Gamma7015ConnectLayerMix2
Description : Test and set connection of layer on 7015 gamma device
Parameters  : Pointer on device, Purpose of connection
Assumptions : Device is valid, Hal pointer is valid
Limitations : In LAYER_SET_REGISTER mode, no error should be returned
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
static ST_ErrorCode_t  stvmix_Gamma7015ConnectLayerMix2(stvmix_Device_t * const Device_p,
                                                        stvmix_HalLayerConnect Purpose)
{
    U32 i=0, RegCTL, NbLayers, DspCfg;
    STLAYER_Layer_t Type;

    /* Inits */
    RegCTL = ReadRegDev32LE(GAM_MIX_CTL);

    RegCTL &= GAM_MIX_CTL_RESET_MASK;

    switch(Purpose)
    {
        case LAYER_CHECK:
            NbLayers = Device_p->Layers.NbTmp;
            break;
        case LAYER_SET_REGISTER:
            NbLayers = Device_p->Layers.NbConnect;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    while(i !=  NbLayers)
    {
        /* Get Layer Type */
        switch(Purpose)
        {
            case LAYER_CHECK:
                Type = Device_p->Layers.TmpArray_p[i].Type;
                break;
            case LAYER_SET_REGISTER:
                Type = Device_p->Layers.ConnectArray_p[i].Type;
                break;
            default :
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }
        /* Compute register CTL  */
        switch(Type)
        {
            case STLAYER_OMEGA2_VIDEO1 :
                /* GNBvd11072: Video 1 on mixer2 not allowed on 7015/20 */
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;

            case STLAYER_OMEGA2_VIDEO2 :
                if(RegCTL & GAM_7015_CTL2_GFX2)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if(RegCTL & GAM_7015_CTL_VIDEO2)
                    return(ST_ERROR_BAD_PARAMETER);
                if(RegCTL & GAM_7015_CTL_VIDEO_MSK)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                RegCTL |= GAM_7015_CTL_VIDEO2;
                break;

            case  STLAYER_GAMMA_GDP :
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;

            case  STLAYER_GAMMA_BKL :
                /* BKLs layers equal to GFXs. GFX2 is in front of the other layers */
                if((Device_p->Layers.TmpArray_p[i].DeviceId & GAMMA_7015_ID_MASK) == GAMMA_7015_ID_GFX2)
                {
                   if(RegCTL & GAM_7015_CTL_GFX2)
                        return(ST_ERROR_BAD_PARAMETER);
                    RegCTL |=  GAM_7015_CTL2_GFX2 ;
                }
                else
                {
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
                break;

            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }
        i++;
    }

    if(Purpose == LAYER_SET_REGISTER)
    {
        /* Protect access */
        STOS_InterruptLock();

        /* Register DSP_CFG to be updated */
        DspCfg = ReadRegBase32LE(MIX_7015_DSPCFG_OFFSET);
        if(RegCTL & GAM_7015_CTL2_GFX2)
            DspCfg |= GAM_7015_DSP_CFG_GFX2;
        else
            DspCfg &= ~((U32)GAM_7015_DSP_CFG_GFX2);

        WriteRegBase32LE(MIX_7015_DSPCFG_OFFSET, DspCfg);

        /* Unprotect access */
        STOS_InterruptUnlock();

        if(((stvmix_HalGamma_t*)(Device_p->Hal_p))->VSyncRegistered == TRUE)
        {
            /* New value of CTL at next VSync if already registered */
            ((stvmix_HalGamma_t*)(Device_p->Hal_p))->RegCTL = RegCTL;
            ((stvmix_HalGamma_t*)(Device_p->Hal_p))->VSyncToProcess = TRUE;

        }
        else
        {
            /* Store new value for CTL */
            WriteRegDev32LE(GAM_MIX_CTL, RegCTL);
        }
    }

    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stvmix_GammaGX1ConnectLayerMix
Description : Test and set connection of layer on ST40GX1 gamma device
Parameters  : Pointer on device, Purpose of connection
Assumptions : Device is valid, Hal pointer is valid
Limitations : In LAYER_SET_REGISTER mode, no error should be returned
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED,
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
static ST_ErrorCode_t stvmix_GammaGX1ConnectLayerMix(stvmix_Device_t * const Device_p,
                                                     stvmix_HalLayerConnect Purpose)
{
    U32 i, RegCTL, NbLayers;
    STLAYER_Layer_t Type;
    U32 j,SaveId;
    osclock_t Time;
    stvmix_HalGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGamma_t *)(Device_p->Hal_p));

    /* Inits */
    RegCTL = ReadRegDev32LE(GAM_MIX_CTL);

    RegCTL &= (GAM_MIX_CTL_RESET_MASK | GAM_MIX_CTL_BGC);

    switch(Purpose)
    {
        case LAYER_CHECK:
            i = NbLayers = Device_p->Layers.NbTmp;
            break;
        case LAYER_SET_REGISTER:
            i = NbLayers = Device_p->Layers.NbConnect;
            break;
        default :
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }

    while(i != 0)
    {
        i--;
        /* Get Layer Type */
        switch(Purpose)
        {
            case LAYER_CHECK:
                Type = Device_p->Layers.TmpArray_p[i].Type;
                break;
            case LAYER_SET_REGISTER:
                Type = Device_p->Layers.ConnectArray_p[i].Type;
                break;
            default :
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }

        /* Compute register CTL in function of layer Type */
        switch(Type)
        {
            case  STLAYER_GAMMA_CURSOR :
                /* Cursor layer should be the closest to the eye */
                if(RegCTL & GAM_MIX_CTL_CURSOR)
                    return(ST_ERROR_BAD_PARAMETER);
                if(NbLayers != (i+1))
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                RegCTL |=  GAM_MIX_CTL_CURSOR ;
                if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                {
                    RegCTL |= GAM_MIX_CTL_CUR_ACTIV;
                }
                break;

            case  STLAYER_GAMMA_GDP :
                /* GDP only for ST40GFX */
                /* Display plane */
                switch(Device_p->Layers.TmpArray_p[i].DeviceId & GAMMA_ST40_ID_MASK)
                {
                    case GAMMA_ST40_ID_GDP1:
                        RegCTL |= GAMMA_ST40_CTL_GDP1;
                        if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                        {
                            RegCTL |= GAM_ST40_CTL_GDP1_ACTIV;
                        }
                        break;

                    case GAMMA_ST40_ID_GDP2:
                        if(RegCTL & GAMMA_ST40_CTL_GDP1)
                            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                        RegCTL |= GAMMA_ST40_CTL_GDP2;
                        if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                        {
                            RegCTL |= GAM_ST40_CTL_GDP2_ACTIV;
                        }
                        break;

                    case GAMMA_ST40_ID_GDP3:
                        if(RegCTL & (GAMMA_ST40_CTL_GDP2 | GAMMA_ST40_CTL_GDP1))
                            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                        RegCTL |= GAMMA_ST40_CTL_GDP3;
                        if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                        {
                            RegCTL |= GAM_ST40_CTL_GDP3_ACTIV;
                        }
                        break;

                    case GAMMA_ST40_ID_GDP4:
                        if(RegCTL & (GAMMA_ST40_CTL_GDP3 | GAMMA_ST40_CTL_GDP2 | GAMMA_ST40_CTL_GDP1))
                            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                        if(Device_p->Layers.ConnectArray_p[i].Params.ActiveSignal == TRUE)
                        {
                            RegCTL |= GAM_ST40_CTL_GDP4_ACTIV;
                        }
                        RegCTL |= GAMMA_ST40_CTL_GDP4;
                        break;

                    default:
                        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                }
                /* Arrange order of GDP planes */
                for(j=i; j<(NbLayers-1); j++)
                {
                    /* Switch only if layer type are identical => identical hardware cell */
                    if(Type == (Device_p->Layers.TmpArray_p[j+1].Type))
                    {
                        if((Device_p->Layers.TmpArray_p[j].DeviceId) == (Device_p->Layers.TmpArray_p[j+1].DeviceId))
                        {
                            /* Layer already given */
                            return(ST_ERROR_BAD_PARAMETER);
                        }
                        if((Device_p->Layers.TmpArray_p[j].DeviceId) > (Device_p->Layers.TmpArray_p[j+1].DeviceId))
                        {
                            /* Switch id to display plan in right order */
                            SaveId = Device_p->Layers.TmpArray_p[j+1].DeviceId;
                            Device_p->Layers.TmpArray_p[j+1].DeviceId = Device_p->Layers.TmpArray_p[j].DeviceId;
                            Device_p->Layers.TmpArray_p[j].DeviceId = SaveId;
                            /* Layers have switched */
                            Device_p->Layers.TmpArray_p[j+1].ChangedId = TRUE;
                            Device_p->Layers.TmpArray_p[j].ChangedId = TRUE;
                        }
                    }
                }
                break;

            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }
    }

    /* Enable background color */
    if(Device_p->Background.Enable == TRUE)
        RegCTL |= GAM_MIX_CTL_BGC;

    if(Purpose == LAYER_SET_REGISTER)
    {
        if(Hal_p->VSyncRegistered == TRUE)
        {
            /* New value of CTL at next VSync if already registered */
            Hal_p->RegCTL = RegCTL;
            Hal_p->VSyncToProcess = TRUE;

            /* VSync Generated ? */
            Time = time_plus(time_now(), STVMIX_MAX_VSYNC_DURATION);
            if( STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, &Time) == -1)
            {
                /* If here, enable mixer is request but not connect (see check below) */
                Hal_p->VSyncToProcess = FALSE;
                return(STVMIX_ERROR_NO_VSYNC);
            }
        }
        else
        {
            /* Store new value for CTL */
            WriteRegDev32LE(GAM_MIX_CTL, RegCTL);
        }
    }
    else /* LAYER check mode only */
    {
        if(Hal_p->VSyncRegistered == TRUE)
        {
            /* Test is Vsync are generated before following the process */
            Hal_p->TestVsync = TRUE;
            Hal_p->VSyncToProcess = TRUE;

           /* VSync Generated ? */
            Time = time_plus(time_now(), STVMIX_MAX_VSYNC_DURATION);
            if( STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, &Time) == -1)
            {
                Hal_p->VSyncToProcess = FALSE;
                return(STVMIX_ERROR_NO_VSYNC);
            }
        }
    }
    return(ST_NO_ERROR);
}



/*******************************************************************************
Name        : stvmix_GammaEvt
Description : Subscribe & Unsubscribe to events
Parameters  : Pointer on device, purpose of call
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t stvmix_GammaEvt(stvmix_Device_t * const Device_p,
                               stvmix_HalActionEvt_e Purpose,
                               ST_DeviceName_t VTGName)
{
    STEVT_DeviceSubscribeParams_t SubscribeParams;
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stvmix_HalGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGamma_t *)(Device_p->Hal_p));

    switch(Purpose)
    {
        case VSYNC_SUBSCRIBE:
            if(Hal_p->VSyncRegistered == FALSE)
            {
                /* VTG Name set when reason is LAYER_SET_REGISTER */
                SubscribeParams.NotifyCallback = stvmix_GammaReceiveEvtVSync;
                SubscribeParams.SubscriberData_p = Device_p;
                ErrCode = STEVT_SubscribeDeviceEvent(Hal_p->EvtHandle,
                                                     VTGName,
                                                     STVTG_VSYNC_EVT,
                                                     &SubscribeParams);
                if(ErrCode == ST_NO_ERROR)
                {
                    /* Subscribed to VSync Event */
                    Hal_p->VSyncRegistered = TRUE;
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "Not able to register device event VSync"));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;

        case VSYNC_UNSUBSCRIBE:
            if(Hal_p->VSyncRegistered == TRUE)
            {
                /* Unsubscribe to vtg VSyncEvent */
                ErrCode = STEVT_UnsubscribeDeviceEvent(Hal_p->EvtHandle,
                                                       VTGName,
                                                       STVTG_VSYNC_EVT);
                if(ErrCode == ST_NO_ERROR)
                {
                    /* Subscribe to VSync Event if not done already */
                    Hal_p->VSyncRegistered = FALSE;
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "Failed to unregister device %s event VSync",
                                  VTGName));
                    ErrCode = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;

        default:
            break;
    }
    return (ErrCode);
}

/*******************************************************************************
Name        : stvmix_GammaInit
Description : Initialize hal for gamma device
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED,
              ST_ERROR_NO_MEMORY
*******************************************************************************/
ST_ErrorCode_t  stvmix_GammaInit(stvmix_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    stvmix_HalGamma_t * Hal_p;
    STEVT_OpenParams_t OpenParams;
    STEVT_Handle_t EvtHandle;

    /* Reserve memory for hal structure */
    Device_p->Hal_p = memory_allocate (Device_p->InitParams.CPUPartition_p,
                                       sizeof(stvmix_HalGamma_t));
    AddDynamicDataSize(sizeof(stvmix_HalGamma_t));

    if(Device_p->Hal_p == NULL)
    {
        return(ST_ERROR_NO_MEMORY);
    }

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGamma_t *)(Device_p->Hal_p));

    /* Init hal structure */
    Hal_p->VSyncRegistered = FALSE;
    Hal_p->VSyncToProcess = FALSE;
    Hal_p->TestVsync = FALSE;
    Hal_p->VSyncProcessed_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);

    /* Set default values */
    stvmix_GammaResetConfig(Device_p);
    ErrCode = stvmix_GammaSetScreen(Device_p);
    if (ErrCode == ST_NO_ERROR)
    {
        ErrCode = stvmix_GammaSetOutputPath(Device_p);
    }

    if(ErrCode == ST_NO_ERROR)
    {
        /* Check event handler name  */
        ErrCode = STEVT_Open(Device_p->InitParams.EvtHandlerName, &OpenParams, &EvtHandle);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Bad name %s for event handler name",
                          Device_p->InitParams.EvtHandlerName));
            ErrCode = ST_ERROR_BAD_PARAMETER;
        }
        /* Save EVT Handle */
        Hal_p->EvtHandle = EvtHandle;
    }

    if(ErrCode != ST_NO_ERROR)
    {
        /* Free semaphore */
        STOS_SemaphoreDelete(NULL,Hal_p->VSyncProcessed_p);


        /* Free reserved memory */
        memory_deallocate (Device_p->InitParams.CPUPartition_p,
                           Device_p->Hal_p);
        /* Reset hal structure */
        Device_p->Hal_p = NULL;
    }

    /* Init capability structure */
    Device_p->Capability.ScreenOffsetHorizontalMin = - STVMIX_GAMMA_HORIZONTAL_OFFSET;
    Device_p->Capability.ScreenOffsetHorizontalMax = STVMIX_GAMMA_HORIZONTAL_OFFSET;
    Device_p->Capability.ScreenOffsetVerticalMin = - STVMIX_COMMON_VERTICAL_OFFSET;
    Device_p->Capability.ScreenOffsetVerticalMax = STVMIX_COMMON_VERTICAL_OFFSET;

    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_GammaResetConfig
Description : Default configuration for gamma registers
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_GammaResetConfig(stvmix_Device_t * const Device_p)
{
    U32 DspCfg, RegCTL;
    osclock_t Time;
    stvmix_HalGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGamma_t *)(Device_p->Hal_p));

    RegCTL = ReadRegDev32LE(GAM_MIX_CTL);

    if((Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015) &&
       (Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER1_OFFSET))
    {
        RegCTL &= GAM_7015_CTL1_RESET_MSK;
    }
    else
    {
        RegCTL &= GAM_MIX_CTL_RESET_MASK;
    }

    /* New value of CTL at next VSync if already programmed */
    Hal_p->RegCTL = RegCTL;

    /* Store new value for CTL now if VSync and GX1 */
    if((Hal_p->VSyncRegistered == TRUE) &&
       Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_GX1)
    {
        /* For ST40-GX1, GDP planes switch other. */
        /* Need to wait for Vsync after a mixer disable */
        /* Block the process until VSync occurs with a semaphore */
        Hal_p->VSyncToProcess = TRUE;

        /* Wait for VSync generated with timeout to prevent task blocking */
        Time = time_plus(time_now(), STVMIX_MAX_VSYNC_DURATION);
        if(STOS_SemaphoreWaitTimeOut(Hal_p->VSyncProcessed_p, &Time) == -1)
        {
            /* VSync didn't occured. Anyway, write CTL register now */
            Hal_p->VSyncToProcess = FALSE;
            WriteRegDev32LE(GAM_MIX_CTL, RegCTL);
        }
    }
    else
    {
        /* Immediate write */
        WriteRegDev32LE(GAM_MIX_CTL, RegCTL);
    }


    if((Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015) &&
       (Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER2_OFFSET))
    {
        /* Protect access */
        STOS_InterruptLock();

        /* Register DSP_CFG to be updated */
        DspCfg = ReadRegBase32LE(MIX_7015_DSPCFG_OFFSET);
        DspCfg &= ~((U32)GAM_7015_DSP_CFG_GFX2);

        WriteRegBase32LE(MIX_7015_DSPCFG_OFFSET, DspCfg);
        /* Unprotect access */
        STOS_InterruptUnlock();
    }
}

/*******************************************************************************
Name        : stvmix_GammaSetBackgroundParameters
Description : Set background color for gamma mixer device
Parameters  : Pointer on device, Purpose of call
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_GammaSetBackgroundParameters(stvmix_Device_t * const Device_p,
                                                    stvmix_HalLayerConnect Purpose)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    if((Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER2_OFFSET) &&
       (Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015))
    {
        ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }

    if((ErrCode == ST_NO_ERROR) && (Purpose == LAYER_SET_REGISTER))
    {

        WriteRegDev32LE(GAM_MIX_BKC, (Device_p->Background.Color.R<<16) |
                        (Device_p->Background.Color.G<<8) | (Device_p->Background.Color.B));
    }

    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_GammaSetOutputPath
Description : Check & configure that the internal chip path output
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_GammaSetOutputPath(stvmix_Device_t * const Device_p)
{
    U32  i, DspCfg;

    if(Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015)
    {
        for(i=0; i<Device_p->Outputs.NbConnect; i++)
        {
            switch(Device_p->Outputs.ConnectArray_p[i].Type)
            {

                case STVOUT_OUTPUT_DIGITAL_YCBCR422_8BITSMULTIPLEXED:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register DSP_CFG to be updated */
                    DspCfg = ReadRegBase32LE(MIX_7015_DSPCFG_OFFSET);

                    if(Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER2_OFFSET)
                        DspCfg |= GAM_7015_DSP_CFG_DSD;
                    else
                        DspCfg &= ~((U32)GAM_7015_DSP_CFG_DSD);

                    WriteRegBase32LE(MIX_7015_DSPCFG_OFFSET, DspCfg);

                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;

                case STVOUT_OUTPUT_HD_ANALOG_RGB:
                case STVOUT_OUTPUT_HD_ANALOG_YUV:
                case STVOUT_OUTPUT_DIGITAL_YCBCR444_24BITSCOMPONENTS:
                case STVOUT_OUTPUT_DIGITAL_YCBCR422_16BITSCHROMAMULTIPLEXED:
                case STVOUT_OUTPUT_DIGITAL_RGB888_24BITSCOMPONENTS:
                case STVOUT_OUTPUT_ANALOG_SVM:
                    if(Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER2_OFFSET)
                        return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                    break;

                    /* Internal 7015 DENC here only */
                case STVOUT_OUTPUT_ANALOG_RGB:
                case STVOUT_OUTPUT_ANALOG_YUV:
                case STVOUT_OUTPUT_ANALOG_YC:
                case STVOUT_OUTPUT_ANALOG_CVBS:
                    /* Protect access */
                    STOS_InterruptLock();

                    /* Register DSP_CFG to be updated */
                    DspCfg = ReadRegBase32LE(MIX_7015_DSPCFG_OFFSET);

                    if(Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER2_OFFSET)
                        DspCfg &= ~((U32)GAM_7015_DSP_CFG_DESL);
                    else
                        DspCfg |= GAM_7015_DSP_CFG_DESL;
                    WriteRegBase32LE(MIX_7015_DSPCFG_OFFSET, DspCfg);

                    /* Unprotect access */
                    STOS_InterruptUnlock();
                    break;

                default:
                    break;
            }
        }
    }
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stvmix_GammaSetScreen
Description : Configure register for output screen
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER
*******************************************************************************/
ST_ErrorCode_t  stvmix_GammaSetScreen(stvmix_Device_t * const Device_p)
{
    U32 RegCTL, AV0Y, AV0X, AVSX, AVSY, AVO, AVS;

    /* Intermediate computed values  */
    AV0X = Device_p->ScreenParams.XStart + Device_p->XOffset;
    AV0Y = Device_p->ScreenParams.YStart + Device_p->YOffset;
    AVSX = AV0X + Device_p->ScreenParams.Width - 1;
    AVSY = AV0Y + Device_p->ScreenParams.Height - 1;

    /* Check that AVS & AVO can be computed */
    if((AV0X & MIX_U16_MASK) ||
       (AV0Y & MIX_U16_MASK) ||
       (AVSX & MIX_U16_MASK) ||
       (AVSY & MIX_U16_MASK))
        return(ST_ERROR_BAD_PARAMETER);

    /* Compute AVS */
    AVS = (AVSY << 16) + AVSX;

    /* Compute register AV0 and AVS */
    if(Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015)
    {
        /* Mask with XOffset. Should not be odd because YCbCr */
        /* inverted on 4.2.2 output DDTS 4.1 of validation */
        /* WA_Valid_7015_Cut1.1 4.1 is corrected */
        AVO = (AV0Y << 16) + (AV0X & GAMMA_7015_XDS_MSK);
#ifdef WA_GNBvd06549
        /* For both mixer */
        if(GAM_7015_AVO_WA_MASK & AVO)
            return(ST_ERROR_BAD_PARAMETER);
#endif /* WA_GNBvd06549 */
        if(Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER1_OFFSET)
        {
            if((GAM_7015_MIX1_AV_MASK & AVO) ||
               (GAM_7015_MIX1_AV_MASK & AVS))
                return(ST_ERROR_BAD_PARAMETER);
        }
        else
        {
            if((GAM_7015_MIX2_AV_MASK & AVO) ||
               (GAM_7015_MIX2_AV_MASK & AVS))
                return(ST_ERROR_BAD_PARAMETER);
        }
    }
    else
    {
        /* No bug noticed on platform ST40 */
        AVO = (AV0Y << 16) + AV0X;
        if((GAM_ST40_AV_MASK & AVO) ||
           (GAM_ST40_AV_MASK & AVS))
            return(ST_ERROR_BAD_PARAMETER);
    }

    /* Store in register AV0 and AVS */
    WriteRegDev32LE(GAM_MIX_AVO, AVO);
    WriteRegDev32LE(GAM_MIX_AVS, AVS);

    if(Device_p->InitParams.DeviceType == STVMIX_GAMMA_TYPE_7015)
    {
        if(Device_p->InitParams.BaseAddress_p == (void*)GAMMA_7015_MIXER1_OFFSET)
        {
            /* Same for background color on MIXER1 only */
            WriteRegDev32LE(GAM_MIX_BCO, AVO);
            WriteRegDev32LE(GAM_MIX_BCS, AVS);
        }

        /* Protect access */
        STOS_InterruptLock();

        /* Switch HD/SD mode : Feature Hard 7015 Always one on MIXER1 */
        /* Even if only MIXER2 is used */
        RegCTL = ReadRegBase32LE(GAMMA_7015_MIXER1_OFFSET+GAM_MIX_CTL);
        RegCTL |= GAM_7015_CTL_HDnotSD; /* WA_GNBvd06551 */
        WriteRegBase32LE(GAMMA_7015_MIXER1_OFFSET+GAM_MIX_CTL, RegCTL);

        /* Unprotect access */
        STOS_InterruptUnlock();
    }
    else
    {
        /* Same for background color on other platform */
        WriteRegDev32LE(GAM_MIX_BCO, AVO);
        WriteRegDev32LE(GAM_MIX_BCS, AVS);
    }
    return(ST_NO_ERROR);
}


/*******************************************************************************
Name        : stvmix_GammaTerm
Description : Terminate hal for gamma device
Parameters  : Pointer on device
Assumptions : Device is valid, Hal pointer is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_GammaTerm(stvmix_Device_t * const Device_p)
{
    stvmix_HalGamma_t * Hal_p;

    /* Set hal pointer */
    Hal_p = ((stvmix_HalGamma_t *)(Device_p->Hal_p));

    if(Hal_p->VSyncRegistered == TRUE)
    {
        /* If error, nothing can be done. A warning could be emited */
        STEVT_UnsubscribeDeviceEvent(Hal_p->EvtHandle,
                                     Device_p->VTG, STVTG_VSYNC_EVT);
    }

    /* Free event handler. Should be no error */
    /* If error, nothing can be done. A warning could be emited */
    STEVT_Close(Hal_p->EvtHandle);

    /* Free semaphore */
    STOS_SemaphoreDelete(NULL,Hal_p->VSyncProcessed_p);

    /* Free reserved memory */
    memory_deallocate (Device_p->InitParams.CPUPartition_p, Device_p->Hal_p);

    /* Reset hal structure */
    Device_p->Hal_p = NULL;

    return;
}

/* End of mixgamma.c */
