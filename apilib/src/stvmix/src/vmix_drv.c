/*******************************************************************************

File name   : vmix_drv.c

Description : video mixer driver layer file

COPYRIGHT (C) STMicroelectronics 2000.

Note : All following function return by default ST_FEATURE_NOT_SUPPORTED
       When adding a new device, simply add it in switch/case
Date               Modification                                     Name
----               ------------                                     ----
21 July 2000        Created                                          BS
26 Oct  2000        Release A2                                       BS
08 Nov  2000        Re-organizing functions & add GFX support        BS
21 June 2001        Coding rule compliant. Increase checks           BS
15 Nov  2001        Add semaphore with timeout to avoid blocking of
                    the task used by mixer                           BS
19 Mar  2002        Compilation chip selection exported in makefile  BS
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */


#if !defined(ST_OSLINUX)
#include <stdlib.h>
#endif

#include "stsys.h"

#include "vmix_drv.h"


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Compilation switch ------------------------------------------------------- */
#ifdef STVMIX_OMEGA1
#include "mix55xx.h"
#endif

#ifdef STVMIX_GAMMA
#include "mixgamma.h"
#endif

#ifdef STVMIX_GENGAMMA
#include "gengamma.h"
#endif

/* Private Macros ----------------------------------------------------------- */

#define UNUSED(x) (void)(x)

/* Private Function prototypes ---------------------------------------------- */

/*******************************************************************************
Name        : stvmix_ConnectLayer
Description : Switch to approriate device
Parameters  : Pointer on device, Stucture of all the layer to connect
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t stvmix_ConnectLayer(stvmix_Device_t * const Device_p)
{
    U32 i;

    /* Global Inits */
    /* Init switch id */
    for(i=0; i < Device_p->Layers.NbTmp; i++)
    {
        Device_p->Layers.TmpArray_p[i].ChangedId = FALSE;
    }

    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            return(stvmix_GammaConnectLayerMix(Device_p, LAYER_CHECK));
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:

            return(stvmix_GenGammaConnectLayerMix(Device_p, LAYER_CHECK));
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            return(stvmix_Omega1ConnectLayer(Device_p, LAYER_CHECK));
            break;
#endif /* STVMIX_OMEGA1 */

        default:
            return(ST_ERROR_FEATURE_NOT_SUPPORTED);
            break;
    }
    return(ST_ERROR_FEATURE_NOT_SUPPORTED);
}

/*******************************************************************************
Name        : stvmix_Disable
Description : Disable device
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_Disable(stvmix_Device_t * const Device_p)
{
    /* Action depends on device */
    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            /* Reset to default */
            stvmix_GammaResetConfig(Device_p);
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            /* Reset to default */
            stvmix_GenGammaResetConfig(Device_p);
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            /* Inform Layer to disable outputs */
            stvmix_Omega1DefaultConfig(Device_p);
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }
}

/*******************************************************************************
Name        : stvmix_DisconnectLayer
Description : Disconnect all Layers
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_DisconnectLayer(stvmix_Device_t * const Device_p)
{
    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            /* Put back default config */
            stvmix_GammaResetConfig(Device_p);
            if(Device_p->Status & API_ENABLE)
            {
                /* Enable background if any. Background is not a connected layer */
                stvmix_GammaConnectBackground(Device_p);
            }
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            /* Put back default config */
            stvmix_GenGammaResetConfig(Device_p);
            if(Device_p->Status & API_ENABLE)
            {
                /* Enable background if any. Background is not a connected layer */
                stvmix_GenGammaConnectBackground(Device_p);
            }
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            stvmix_Omega1DefaultConfig(Device_p);
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }
}

/*******************************************************************************
Name        : stvmix_Enable
Description : Enable device
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED, ST_ERROR_BAD_PARAMETER
              STVMIX_ERROR_NO_VSYNC
*******************************************************************************/
ST_ErrorCode_t stvmix_Enable(stvmix_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    /* Action depends on device */
    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            ErrCode = ST_NO_ERROR;
            /* Check if API ready to display layers & background */
            if((Device_p->Status & API_DISPLAY_ALL_SET) == API_DISPLAY_ALL_SET)
            {
                /* Try to connect layers */
                ErrCode = stvmix_GammaConnectLayerMix(Device_p, LAYER_SET_REGISTER);
            }
            else if(Device_p->Status & API_ENABLE)
            {
                /* Enable background if any */
                stvmix_GammaConnectBackground(Device_p);
            }
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            ErrCode = ST_NO_ERROR;
            /* Check if API ready to display layers & background */
            if((Device_p->Status & API_DISPLAY_ALL_SET) == API_DISPLAY_ALL_SET)
            {
                /* Try to connect layers */
                ErrCode = stvmix_GenGammaConnectLayerMix(Device_p, LAYER_SET_REGISTER);
            }
            else if(Device_p->Status & API_ENABLE)
            {
                /* Enable background if any => No error checked */
                stvmix_GenGammaConnectBackground(Device_p);
            }
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            ErrCode = ST_NO_ERROR;
            if((Device_p->Status & API_DISPLAY_ALL_SET) == API_DISPLAY_ALL_SET)
            {
                /* Try to connect layers */
                ErrCode = stvmix_Omega1ConnectLayer(Device_p, LAYER_SET_REGISTER);
            }
            /* Enable background if any => No error checked */
            stvmix_Omega1SetBackgroundParameters(Device_p, LAYER_SET_REGISTER);
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }
    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_GetBackGroundColor
Description : Get background color parameter
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_GetBackGroundColor(stvmix_Device_t * const Device_p,
                                         STGXOBJ_ColorRGB_t* const RGB888_p,
                                         BOOL* const Enable_p)
{
    ST_ErrorCode_t Err = ST_ERROR_FEATURE_NOT_SUPPORTED;

    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            Err = stvmix_GammaSetBackgroundParameters(Device_p, LAYER_CHECK);
            if(Err == ST_NO_ERROR)
            {
                *RGB888_p = Device_p->Background.Color;
                *Enable_p = Device_p->Background.Enable;
                Err = ST_NO_ERROR;
            }
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            Err = stvmix_GenGammaSetBackgroundParameters(Device_p, LAYER_CHECK);
            if(Err == ST_NO_ERROR)
            {
                *RGB888_p = Device_p->Background.Color;
                *Enable_p = Device_p->Background.Enable;
                Err = ST_NO_ERROR;
            }
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            Err = stvmix_Omega1SetBackgroundParameters(Device_p, LAYER_CHECK);
            if(Err == ST_NO_ERROR)
            {
                *RGB888_p = Device_p->Background.Color;
                *Enable_p = Device_p->Background.Enable;
                Err = ST_NO_ERROR;
            }
            break;
#endif /* STVMIX_OMEGA1 */
        default :
            break;
    }

    return(Err);
}

/*******************************************************************************
Name        : stvmix_Init
Description : Hal initialisation function
Parameters  : Pointer on device, Pointer on InitParams structure
Assumptions : Device is valid as InitParams structure
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_Init(stvmix_Device_t * const Device_p, const STVMIX_InitParams_t * const InitParams_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    /* Init hal structure */
    Device_p->Hal_p = NULL;

    /* List of supported devices */
    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            ErrCode = stvmix_GammaInit(Device_p);
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            ErrCode = stvmix_GenGammaInit(Device_p);
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            Device_p->Capability.ScreenOffsetHorizontalMin = - STVMIX_OMEGA1_HORIZONTAL_OFFSET;
            Device_p->Capability.ScreenOffsetHorizontalMax = STVMIX_OMEGA1_HORIZONTAL_OFFSET;
            Device_p->Capability.ScreenOffsetVerticalMin = - STVMIX_OMEGA1_VERTICAL_OFFSET;
            Device_p->Capability.ScreenOffsetVerticalMax = STVMIX_OMEGA1_VERTICAL_OFFSET;
            stvmix_Omega1DefaultConfig(Device_p);
            /* Set default black background is hal permits */
            stvmix_Omega1SetBackgroundParameters(Device_p, LAYER_SET_REGISTER);
            /* Return */
            ErrCode = ST_NO_ERROR;
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }

    if(ErrCode == ST_NO_ERROR)
    {
        /* Hal exist and implemented => Store type */
        Device_p->InitParams.DeviceType = InitParams_p->DeviceType;
    }
    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_SetBackGroundColor
Description : Set background color parameter
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_SetBackGroundColor(stvmix_Device_t * const Device_p,
                                         STGXOBJ_ColorRGB_t const RGB888, BOOL const Enable)
{
    ST_ErrorCode_t Err = ST_ERROR_FEATURE_NOT_SUPPORTED;
    STGXOBJ_ColorRGB_t PreviousColor;
    BOOL PreviousEnable;

    PreviousColor = Device_p->Background.Color;
    PreviousEnable = Device_p->Background.Enable;
    Device_p->Background.Color = RGB888;
    Device_p->Background.Enable = Enable;

    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            Err = stvmix_GammaSetBackgroundParameters(Device_p, LAYER_SET_REGISTER);
            if((Err == ST_NO_ERROR) && (Device_p->Status & API_ENABLE))
                stvmix_GammaConnectBackground(Device_p);
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            Err = stvmix_GenGammaSetBackgroundParameters(Device_p, LAYER_SET_REGISTER);
            if((Err == ST_NO_ERROR) && (Device_p->Status & API_ENABLE))
                stvmix_GenGammaConnectBackground(Device_p);
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            Err = stvmix_Omega1SetBackgroundParameters(Device_p, LAYER_SET_REGISTER);
            /* No background to connect. Hardware is always enable */
            break;
#endif /* STVMIX_OMEGA1 */
        default :
            break;
    }
    if(Err == ST_NO_ERROR)
    {
        Device_p->Background.Color = RGB888;
        Device_p->Background.Enable = Enable;
    }
    else
    {
        Device_p->Background.Color = PreviousColor;
        Device_p->Background.Enable = PreviousEnable;
    }
    return(Err);
}

/*******************************************************************************
Name        : stvmix_SetScreen
Description : Compute screen parameters
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_SetScreen(stvmix_Device_t * const Device_p)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;

    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            ErrCode = stvmix_GammaSetScreen(Device_p);
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            ErrCode = stvmix_GenGammaSetScreen(Device_p);
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            ErrCode = ST_NO_ERROR;
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }
    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_SetTimeBase
Description : Compute screen parameters
Parameters  : Pointer on device and old VTG name
Assumptions : Device is valid
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER, ST_ERROR_FEATURE_NOT_SUPPORTED
*******************************************************************************/
ST_ErrorCode_t stvmix_SetTimeBase(stvmix_Device_t * const Device_p, ST_DeviceName_t OldVTG)
{
    ST_ErrorCode_t ErrCode = ST_ERROR_FEATURE_NOT_SUPPORTED;



    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            /* Unsubscribe to previous vtg */
            ErrCode = stvmix_GammaEvt(Device_p, VSYNC_UNSUBSCRIBE, OldVTG);

            if(ErrCode == ST_NO_ERROR)
            {
                /* Subscribe to new vtg */
                ErrCode = stvmix_GammaEvt(Device_p, VSYNC_SUBSCRIBE, Device_p->VTG);
            }
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            /* Inform hal of new vtg */
            ErrCode = stvmix_GenGammaEvt(Device_p, CHANGE_VTG, Device_p->VTG);
            UNUSED(OldVTG);


            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            ErrCode = ST_NO_ERROR;
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }
    return(ErrCode);
}

/*******************************************************************************
Name        : stvmix_Term
Description : Hal termination function
Parameters  : Pointer on device
Assumptions : Device is valid
Limitations :
Returns     : None
*******************************************************************************/
void stvmix_Term(stvmix_Device_t * const Device_p)
{
    /* Reset register to default values */
    switch(Device_p->InitParams.DeviceType)
    {
#ifdef STVMIX_GAMMA
        case STVMIX_GAMMA_TYPE_7015 :
        case STVMIX_GAMMA_TYPE_GX1 :
            stvmix_GammaResetConfig(Device_p);
            stvmix_GammaTerm(Device_p);
            break;
#endif /* STVMIX_GAMMA */

#ifdef STVMIX_GENGAMMA
        case STVMIX_GENERIC_GAMMA_TYPE_7020 :
        case STVMIX_GENERIC_GAMMA_TYPE_5528:
        case STVMIX_GENERIC_GAMMA_TYPE_7710:
        case STVMIX_GENERIC_GAMMA_TYPE_7100:
        case STVMIX_GENERIC_GAMMA_TYPE_7109:
        case STVMIX_GENERIC_GAMMA_TYPE_7200:
            stvmix_GenGammaResetConfig(Device_p);
            stvmix_GenGammaTerm(Device_p);
            break;
#endif /* STVMIX_GENGAMMA */

#ifdef STVMIX_OMEGA1
        case STVMIX_OMEGA1_SD:
            break;
#endif /* STVMIX_OMEGA1 */

        default :
            break;
    }
}
/* End of mix_hal.c */
