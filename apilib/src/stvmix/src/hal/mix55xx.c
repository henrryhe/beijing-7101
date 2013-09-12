/*******************************************************************************

File name   : mix55xx.c

Description : video mixer driver for 55xx type

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
08 Nov 2000         Created                                          BS
19 Mar  2002        Compilation chip selection exported in makefile  BS
******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stsys.h"
#include "mix55xx.h"
#include "sttbx.h"

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Compilation switch ------------------------------------------------------- */

/* Private Constants ----------------------------------------------------------- */
#ifdef ST_5514
#define WA_GNBvd30484
#endif

/* Private Macros ----------------------------------------------------------- */
/**********************************/
/* Register configuration for ALL */
/**********************************/
#define VID_OUT         0x90  /* Video output gain [3:0]                        */
#define VID_OUT_NOS     0x08
#define VID_OUT_SPO     0x04
#define VID_OUT_LAY     0x03

#define VID_BCK_Y       0x98
#define VID_BCK_U       0x99
#define VID_BCK_V       0x9A

/**********************************/
/* Software configuration for ALL */
/**********************************/
#define OMEGA1_CURSOR   0x01
#define OMEGA1_OSD      0x02
#define OMEGA1_VIDEO    0x04
#define OMEGA1_STILL    0x08

#define ReadRegDev8(Add)         STSYS_ReadRegDev8((U32)Device_p->InitParams.BaseAddress_p \
                                    +(U32)Device_p->InitParams.DeviceBaseAddress_p + Add);
#define WriteRegDev8(Add,Value)  STSYS_WriteRegDev8((U32)Device_p->InitParams.BaseAddress_p \
                                    +(U32)Device_p->InitParams.DeviceBaseAddress_p + Add, Value);

/* Private Function prototypes --------------------------------------------------------- */


/************************************************************************************************************/
/*************************************** BEGIN OF OMEGA FUNCTIONS *******************************************/
/************************************************************************************************************/


/*******************************************************************************
Name        : HalOmega1ConnectLayer
Description : Check supported layer for Omega1 device
Parameters  : Pointer on device, Stucture of all the layer to connect, HD or SD output
Assumptions : Device is valid
Limitations :
Returns     : ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t  stvmix_Omega1ConnectLayer(stvmix_Device_t * const Device_p, stvmix_HalLayerConnect Purpose)
{
    STLAYER_Layer_t Type;
    STLAYER_OutputParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    U8 RegValue, RegMask, PlaneEnable=0;
    U32 i;

    switch(Purpose)
    {
        case LAYER_CHECK:
            i = Device_p->Layers.NbTmp;
            break;
        case LAYER_SET_REGISTER:
            i = Device_p->Layers.NbConnect;
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
            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }
        switch(Type)
        {
            case STLAYER_OMEGA1_CURSOR:
                if(PlaneEnable & OMEGA1_CURSOR)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if(PlaneEnable & OMEGA1_VIDEO)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if(PlaneEnable & OMEGA1_STILL)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if (i == Device_p->Layers.NbConnect-1)
                    RegMask = VID_OUT_SPO;
                else
                    RegMask = (U8)~VID_OUT_SPO;
                PlaneEnable |= OMEGA1_CURSOR;
                break;

            case STLAYER_OMEGA1_OSD:
                if(PlaneEnable & OMEGA1_OSD)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if(PlaneEnable & OMEGA1_VIDEO)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if(PlaneEnable & OMEGA1_STILL)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                PlaneEnable |= OMEGA1_OSD;
                break;

            case STLAYER_OMEGA1_VIDEO:
                if(PlaneEnable & OMEGA1_VIDEO)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if(PlaneEnable & OMEGA1_STILL)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                PlaneEnable |= OMEGA1_VIDEO;
                break;

            case STLAYER_OMEGA1_STILL:
                if(PlaneEnable & OMEGA1_STILL)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                if (i != 0)
                    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                PlaneEnable |= OMEGA1_STILL;
                break;

            default:
                return(ST_ERROR_FEATURE_NOT_SUPPORTED);
                break;
        }
    }

    /* Emulate here the setting of register to enable the plane */
    if(Purpose == LAYER_SET_REGISTER)
    {
        /* Order correctly Cursor and OSD */
        if (PlaneEnable & OMEGA1_CURSOR)
        {
            STOS_InterruptLock();
            RegValue = ReadRegDev8(VID_OUT);

            if (RegMask == VID_OUT_SPO)
                RegValue |= VID_OUT_SPO;
            else
                RegValue &= RegMask;

            WriteRegDev8(VID_OUT,RegValue);
            STOS_InterruptUnlock();
        }

        /* Enables planes */
        LayerParams.UpdateReason = STLAYER_DISPLAY_REASON;
        LayerParams.DisplayEnable  = TRUE;
        for (i=0; i<Device_p->Layers.NbConnect; i++)
        {
            /* Always no error return */
            ErrCode = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[i].Handle, &LayerParams);
        }
    }
    return (ST_NO_ERROR);
}


/*******************************************************************************
Name        : stvmix_Omega1DefaultConfig
Description : Default configuration for 55XX registers
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     :
*******************************************************************************/
void stvmix_Omega1DefaultConfig(stvmix_Device_t * const Device_p)
{

    STLAYER_OutputParams_t LayerParams;
    ST_ErrorCode_t ErrCode;
    U32 i;
    U8 RegValue;

    LayerParams.UpdateReason = STLAYER_DISPLAY_REASON;
    LayerParams.DisplayEnable  = FALSE;
    for (i=0; i<Device_p->Layers.NbConnect; i++)
    {
        /* Always no error return */
        ErrCode = STLAYER_UpdateFromMixer(Device_p->Layers.ConnectArray_p[i].Handle, &LayerParams);
    }

    STOS_InterruptLock();

    /* Store default order */
    RegValue = ReadRegDev8(VID_OUT);
    #ifndef WA_GNBvd30484
        RegValue |= VID_OUT_SPO;
    #endif
    WriteRegDev8(VID_OUT, RegValue);

    STOS_InterruptUnlock();
}

/*******************************************************************************
Name        : stvmix_Omega1SetBackgroundParameters
Description : Configuration of the background
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     :
*******************************************************************************/
ST_ErrorCode_t stvmix_Omega1SetBackgroundParameters(stvmix_Device_t * const Device_p,
                                                    stvmix_HalLayerConnect Purpose)
{
    /* default is black color */
    S32 Y = 16, Cb = 128, Cr = 128;

    if( Purpose == LAYER_SET_REGISTER)
    {
        if((Device_p->Status & API_ENABLE) && (Device_p->Background.Enable == TRUE))
        {
            /* Translate color RGB to YCbCr */
            Y  = ( 66*Device_p->Background.Color.R +
                   129*Device_p->Background.Color.G +
                   25*Device_p->Background.Color.B)/256 + 16;
            Cb = (-38*Device_p->Background.Color.R -
                  74*Device_p->Background.Color.G +
                  112*Device_p->Background.Color.B)/256 + 128;
            Cr = (112*Device_p->Background.Color.R -
                  94*Device_p->Background.Color.G -
                  18*Device_p->Background.Color.B)/256 + 128;
            if(Y < 16)
                Y = 16;
            else if(Y > 235)
                Y = 235;
            if(Cb < 16)
                Cb = 16;
            else if(Cb > 240)
                Cb = 240;
            if(Cr < 16)
                Cr = 16;
            else if(Cr > 240)
                Cr = 240;

        }

        /* Write to register */
        WriteRegDev8(VID_BCK_Y, (U8)Y);
        WriteRegDev8(VID_BCK_U, (U8)Cb);
        WriteRegDev8(VID_BCK_V, (U8)Cr);
    }

    return(ST_NO_ERROR);
}


/* End of mix55xx.c */
