/**********************************************************************************

File Name   : denc_drv.c

Description : API specific DENC driver module.

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                             Name
----               ------------                                             ----
22 Jun 2001        Created from stdenc.c, add setting option feature        HSDLM
30 Aou 2001        Remove dependency upon STI2C if not needed, to support   HSdLM
 *                 ST40GX1
16 Apr 2002        Add support for STi7020                                  HSdLM
24 aug 2003        Add support for 4629                                     SC
05 Jul 2004        Add 7710/mb391 support                                   TA
13 Sep 2004        Add support for ST40/OS21                                MH
***********************************************************************************/

/* Includes ----------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
/* Include system files only if not in Kernel mode */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

#include "stddefs.h"

/*#if !defined(ST_OSLINUX)*/
#include "sttbx.h"
/*#endif*/

#include "stdenc.h"
#include "denc_drv.h"
#include "denc_hal.h"
#include "denc_reg.h"
#include "reg_rbus.h"

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */
#if defined WA_GNBvd36366
   U8 TempReg;
#endif

/* Private Macros ----------------------------------------------------------- */

#define MODE_IS_PAL(Mode) (   ((Mode) == STDENC_MODE_PALBDGHI) || ((Mode) == STDENC_MODE_PALM) \
                           || ((Mode) == STDENC_MODE_PALN)     || ((Mode) == STDENC_MODE_PALN_C))

#define MODE_IS_NTSC(Mode) (((Mode) == STDENC_MODE_NTSCM) || ((Mode) == STDENC_MODE_NTSCM_J) || ((Mode) == STDENC_MODE_NTSCM_443))

#define MODE_IS_SECAM(Mode) (((Mode) == STDENC_MODE_SECAM) || ((Mode) == STDENC_MODE_SECAM_AUX))

/* Private Function prototypes ---------------------------------------------- */

static void DoReport( const char * const String, const ST_ErrorCode_t Error);
static BOOL IsChromaDelayValid( const U8 CellId, const S8 Value);

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : IsChromaDelayValid
Description : Check chroma delay validity (min, max, step) versus DENC version
Parameters  : IN : CellId : DENC version
              IN : Value  : Chroma delay value to check
Assumptions :
Limitations :
Returns     : TRUE if valid, FALSE if not.
*******************************************************************************/
static BOOL IsChromaDelayValid( const U8 CellId, const S8 Value)
{
    if (CellId <10)
    {
        return (   (Value >= STDENC_MIN_CHROMA_DELAY)
                && (Value <= STDENC_MAX_CHROMA_DELAY_V10_LESS)
                && ((Value - STDENC_MIN_CHROMA_DELAY) % STDENC_STEP_CHROMA_DELAY_V10_LESS == 0));
    }
    else
    {
        return (   (Value >= STDENC_MIN_CHROMA_DELAY)
                && (Value <= STDENC_MAX_CHROMA_DELAY_V10_MORE)
                && ((Value - STDENC_MIN_CHROMA_DELAY) % STDENC_STEP_CHROMA_DELAY_V10_MORE == 0));
    }
}


/*******************************************************************************
Name        : DoReport
Description : STTBX_REPORT information or error
Parameters  : IN : String : additional text to display,
              IN : Error : error number to report
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
static void DoReport( const char * const String, const ST_ErrorCode_t Error)
{
#ifdef STTBX_REPORT
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "%sfailed, error %x", String, Error));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "%sdone", String));
    }
#else
    UNUSED_PARAMETER(String);
    UNUSED_PARAMETER(Error);
#endif /*STTBX_REPORT */

} /* end of DoReport */

/*
--------------------------------------------------------------------------------
API routine : Set encoding mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STDENC_SetEncodingMode(
                const STDENC_Handle_t         Handle,
                const STDENC_EncodingMode_t * const EncodingMode_p
                )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DENC_t *Unit_p;

    if (STDENC_CheckHandle(Handle) != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (EncodingMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p = (DENC_t *)Handle;

    /* Protect access */
    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    /* check mode is valid */
    switch(EncodingMode_p->Mode)
    {
        case STDENC_MODE_NTSCM : /* no break */
        case STDENC_MODE_NTSCM_J : /* no break */
        case STDENC_MODE_NTSCM_443 :
            if (   (EncodingMode_p->Option.Ntsc.SquarePixel)    /* Square Pixel feature is only */
                && (Unit_p->Device_p->DencVersion < 6)          /* available for v>=6 */
                )
            {
                ErrorCode = STDENC_ERROR_INVALID_ENCODING_MODE;
            }
            else
            {
                if (EncodingMode_p->Mode == STDENC_MODE_NTSCM_J)
                {
                    Unit_p->Device_p->BlackLevelPedestal = FALSE; /* BlackLevelPedestalAux is set below */
                }
                else /* STDENC_MODE_NTSCM or STDENC_MODE_NTSCM_443*/
                {
                    Unit_p->Device_p->BlackLevelPedestal = TRUE;
                }
                if (Unit_p->Device_p->DencVersion < 10)
                {
                    Unit_p->Device_p->ChromaDelay = 0;
                }
                else /* Id >= 10 */
                {
                    if ( (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7020))
                    {
                        /* SwRWA of DDTS GNBvd07130, topic 4.3 of STi7015 Cut 1.1 validation report */
                        Unit_p->Device_p->ChromaDelay = -3;
                    }
                    else
                    {
                        Unit_p->Device_p->ChromaDelay = -1; /* in 4:2:2 format on CVBS, 4:4:4 format on CVBS is not supported */
                        if (Unit_p->Device_p->IsAuxEncoderOn)
                        {
                            Unit_p->Device_p->ChromaDelayAux = -1;
                            Unit_p->Device_p->BlackLevelPedestalAux = Unit_p->Device_p->BlackLevelPedestal;
                        }
                    }
                }
            }
            break;
        case STDENC_MODE_PALBDGHI : /* no break */
        case STDENC_MODE_PALM :     /* no break */
        case STDENC_MODE_PALN :     /* no break */
        case STDENC_MODE_PALN_C :
            if (   (EncodingMode_p->Option.Pal.SquarePixel)     /* Square Pixel feature is only */
                && (Unit_p->Device_p->DencVersion < 6)          /* available for v>=6 */
                )
            {
                ErrorCode = STDENC_ERROR_INVALID_ENCODING_MODE;
            }
            else
            {
                if ( (EncodingMode_p->Mode == STDENC_MODE_PALBDGHI) || (EncodingMode_p->Mode == STDENC_MODE_PALN_C))
                {
                    Unit_p->Device_p->BlackLevelPedestal = FALSE; /* BlackLevelPedestalAux is set below */
                }
                else /* STDENC_MODE_PALN or STDENC_MODE_PALM*/
                {
                    Unit_p->Device_p->BlackLevelPedestal = TRUE;
                }
                if (Unit_p->Device_p->DencVersion < 10)
                {
                    Unit_p->Device_p->ChromaDelay = 0;
                }
                else /* Id >= 10 */
                {
                    if ( (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7020))
                    {
                        /* SwRWA of DDTS GNBvd07130, topic 4.3 of STi7015 Cut 1.1 validation report */
                        Unit_p->Device_p->ChromaDelay = -3;
                    }
                    else
                    {
                        Unit_p->Device_p->ChromaDelay = 3;/*yxl 2007-09-06 modify to 3*/ /* in 4:2:2 format on CVBS, 4:4:4 format on CVBS is not supported */
                        if (Unit_p->Device_p->IsAuxEncoderOn)
                        {
                            Unit_p->Device_p->ChromaDelayAux = 3;/*yxl 2007-09-06 modify to 3*/
                            Unit_p->Device_p->BlackLevelPedestalAux = Unit_p->Device_p->BlackLevelPedestal;
                        }
                    }
                }
            }
            break;
        case STDENC_MODE_SECAM :
            if (   (Unit_p->Device_p->DencVersion < 6)   /* SECAM is not supported by v3 and v5 */
                || (   (Unit_p->Device_p->SECAMSquarePixel)     /* Square Pixel feature is only */
                    && (Unit_p->Device_p->DencVersion < 10)))   /* available for v>=10 */
            {
                ErrorCode = STDENC_ERROR_INVALID_ENCODING_MODE;
            }
            else
            {
                Unit_p->Device_p->BlackLevelPedestal = FALSE;  /* BlackLevelPedestalAux is set below switch() */
                Unit_p->Device_p->LumaTrapFilter     = TRUE;   /* mandatory */
                if ( (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) || (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7020))
                {
                    /* SwRWA of DDTS GNBvd07130, topic 4.3 of STi7015 Cut 1.1 validation report */
                    Unit_p->Device_p->ChromaDelay = -3;
                }
                else
                {
                    Unit_p->Device_p->ChromaDelay = 0; /* in 4:2:2 format on CVBS, 4:4:4 format on CVBS is not supported */
                    if (Unit_p->Device_p->IsAuxEncoderOn)
                    {
                        Unit_p->Device_p->ChromaDelayAux = 0;
                        Unit_p->Device_p->LumaTrapFilterAux = TRUE;   /* mandatory */
                    }
                }
            }
            break;
        case STDENC_MODE_SECAM_AUX :
            if ((Unit_p->Device_p->DencVersion < 12) || !(Unit_p->Device_p->IsAuxEncoderOn))   /* AUX is supported since V12 */
            {
                ErrorCode = STDENC_ERROR_INVALID_ENCODING_MODE;
            }
            else
            {
                Unit_p->Device_p->BlackLevelPedestalAux = FALSE;  /* BlackLevelPedestalAux is set below switch() */
                Unit_p->Device_p->ChromaDelayAux        = 0;
                Unit_p->Device_p->LumaTrapFilterAux     = TRUE;   /* mandatory */
            }
            break;
        case STDENC_MODE_NONE : /* no break */
        default :
            ErrorCode = STDENC_ERROR_INVALID_ENCODING_MODE;
            break;
    } /* end of switch(EncodingMode_p->Mode) */

    if (ErrorCode == ST_NO_ERROR)
    {
        /* update only if new EncodingMode is different from current */
        if ( memcmp(EncodingMode_p, &(Unit_p->Device_p->EncodingMode), sizeof(STDENC_EncodingMode_t)) != 0)
        {
            Unit_p->Device_p->EncodingMode = *EncodingMode_p;
            ErrorCode = stdenc_HALSetEncodingMode(Unit_p->Device_p);
        }
    } /* end of if (ErrorCode == ST_NO_ERROR) */

    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    DoReport("STDENC_SetEncodingMode ", ErrorCode);
    return(ErrorCode);
} /* end of STDENC_SetEncodingMode */

/*
--------------------------------------------------------------------------------
API routine : Get encoding mode
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STDENC_GetEncodingMode(
                const STDENC_Handle_t    Handle,
                STDENC_EncodingMode_t *  const EncodingMode_p
                )
{
    ST_ErrorCode_t ErrorCode;
    DENC_t *Unit_p;

    if (STDENC_CheckHandle(Handle) != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (EncodingMode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Unit_p = (DENC_t *)Handle;

    /* Protect access */
    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    *EncodingMode_p = Unit_p->Device_p->EncodingMode;
    ErrorCode = ST_NO_ERROR;

    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    DoReport("STDENC_GetEncodingMode ", ErrorCode);
    return(ErrorCode);
} /* end of STDENC_GetEncodingMode */

/*
--------------------------------------------------------------------------------
API routine : Set mode option
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_SetModeOption (
                    const STDENC_Handle_t          Handle,
                    const STDENC_ModeOption_t *    const ModeOption_p
                    )
{
    BOOL IsThereChange = FALSE; /* update only if new Option is different from current */
    DENC_t *Unit_p;
    STDENC_EncodingMode_t *CurrentMode_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (STDENC_CheckHandle(Handle) != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ModeOption_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p = (DENC_t *)Handle;

    /* Protect access */
    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    CurrentMode_p = &(Unit_p->Device_p->EncodingMode);
    /* check Option is valid, then that it is supported by current encoding mode and DENC cell version */
    /* then update only if new Option is different from current */
    switch(ModeOption_p->Option)
    {
        case STDENC_OPTION_BLACK_LEVEL_PEDESTAL :
            if (ModeOption_p->Value.BlackLevelPedestal != Unit_p->Device_p->BlackLevelPedestal)
            {
                Unit_p->Device_p->BlackLevelPedestal = ModeOption_p->Value.BlackLevelPedestal;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_BLACK_LEVEL_PEDESTAL_AUX :
            if (Unit_p->Device_p->DencVersion < 12 || !(Unit_p->Device_p->IsAuxEncoderOn))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (ModeOption_p->Value.BlackLevelPedestalAux != Unit_p->Device_p->BlackLevelPedestalAux)
            {
                Unit_p->Device_p->BlackLevelPedestalAux = ModeOption_p->Value.BlackLevelPedestalAux;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_CHROMA_DELAY :
            if (!IsChromaDelayValid(Unit_p->Device_p->DencVersion, ModeOption_p->Value.ChromaDelay))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else if (ModeOption_p->Value.ChromaDelay != Unit_p->Device_p->ChromaDelay)
            {
                Unit_p->Device_p->ChromaDelay = ModeOption_p->Value.ChromaDelay;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_CHROMA_DELAY_AUX :
            if (Unit_p->Device_p->DencVersion < 12 || !(Unit_p->Device_p->IsAuxEncoderOn))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (!IsChromaDelayValid(Unit_p->Device_p->DencVersion, ModeOption_p->Value.ChromaDelayAux))
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else if (ModeOption_p->Value.ChromaDelayAux != Unit_p->Device_p->ChromaDelayAux)
            {
                Unit_p->Device_p->ChromaDelayAux = ModeOption_p->Value.ChromaDelayAux;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_LUMA_TRAP_FILTER :
            /* For SECAM trap filter must always be set */
            if ((CurrentMode_p->Mode == STDENC_MODE_SECAM) && !ModeOption_p->Value.LumaTrapFilter)
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (ModeOption_p->Value.LumaTrapFilter != Unit_p->Device_p->LumaTrapFilter)
            {
                Unit_p->Device_p->LumaTrapFilter = ModeOption_p->Value.LumaTrapFilter;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_LUMA_TRAP_FILTER_AUX :
            if (Unit_p->Device_p->DencVersion < 12 || !(Unit_p->Device_p->IsAuxEncoderOn))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (CurrentMode_p->Mode == STDENC_MODE_SECAM) /* should be SECAM_AUX */
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            /* For SECAM trap filter must always be set */
            else if ((CurrentMode_p->Mode == STDENC_MODE_SECAM_AUX) && !ModeOption_p->Value.LumaTrapFilterAux)
            {
                ErrorCode = ST_ERROR_BAD_PARAMETER;
            }
            else if (ModeOption_p->Value.LumaTrapFilterAux != Unit_p->Device_p->LumaTrapFilterAux)
            {
                Unit_p->Device_p->LumaTrapFilterAux = ModeOption_p->Value.LumaTrapFilterAux;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_FIELD_RATE_60HZ :
            if (   (   (MODE_IS_PAL(CurrentMode_p->Mode))
                    || (MODE_IS_SECAM(CurrentMode_p->Mode)))
                && (ModeOption_p->Value.FieldRate60Hz))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (ModeOption_p->Value.FieldRate60Hz != CurrentMode_p->Option.Ntsc.FieldRate60Hz)
            {
                CurrentMode_p->Option.Ntsc.FieldRate60Hz = ModeOption_p->Value.FieldRate60Hz;
                IsThereChange = TRUE;
            }
            break;
        case STDENC_OPTION_NON_INTERLACED :
            if ( (MODE_IS_SECAM(CurrentMode_p->Mode)) && (!ModeOption_p->Value.Interlaced))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                if (   (MODE_IS_PAL(CurrentMode_p->Mode))
                    && (ModeOption_p->Value.Interlaced != CurrentMode_p->Option.Pal.Interlaced))
                {
                    CurrentMode_p->Option.Pal.Interlaced = ModeOption_p->Value.Interlaced;
                    IsThereChange = TRUE;
                }
                else if (   (MODE_IS_NTSC(CurrentMode_p->Mode))
                            && (ModeOption_p->Value.Interlaced != CurrentMode_p->Option.Ntsc.Interlaced))
                {
                    CurrentMode_p->Option.Ntsc.Interlaced = ModeOption_p->Value.Interlaced;
                    IsThereChange = TRUE;
                }
            }
            break;
        case STDENC_OPTION_SQUARE_PIXEL :
            if (   ((CurrentMode_p->Mode == STDENC_MODE_SECAM) && (Unit_p->Device_p->DencVersion < 10))
                && (ModeOption_p->Value.SquarePixel))
            /* STDENC_MODE_SECAM_AUX is only supported for DencVersion >= 12 so ok */
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                if (   (MODE_IS_PAL(CurrentMode_p->Mode))
                    && (ModeOption_p->Value.SquarePixel != CurrentMode_p->Option.Pal.SquarePixel))
                {
                    CurrentMode_p->Option.Pal.SquarePixel = ModeOption_p->Value.SquarePixel;
                    IsThereChange = TRUE;
                }
                else if (   (MODE_IS_NTSC(CurrentMode_p->Mode))
                            && (ModeOption_p->Value.SquarePixel != CurrentMode_p->Option.Ntsc.SquarePixel))
                {
                    CurrentMode_p->Option.Ntsc.SquarePixel = ModeOption_p->Value.SquarePixel;
                    IsThereChange = TRUE;
                }
                else if (   (CurrentMode_p->Mode == STDENC_MODE_SECAM)
                            && (ModeOption_p->Value.SquarePixel != Unit_p->Device_p->SECAMSquarePixel))
                {
                    Unit_p->Device_p->SECAMSquarePixel = ModeOption_p->Value.SquarePixel;
                    IsThereChange = TRUE;
                }
            }
            break;
        case STDENC_OPTION_YCBCR444_INPUT :
            if (   (    (Unit_p->Device_p->DencVersion < 6)
                     || (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7015) )
                    && (ModeOption_p->Value.YCbCr444Input))
            {
                /* 4:4:4 is not supported */
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (  (   (Unit_p->Device_p->DencVersion >= 12)
                        || (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_GX1)
                        || (Unit_p->Device_p->DeviceType == STDENC_DEVICE_TYPE_7020))
                     && !(ModeOption_p->Value.YCbCr444Input))
            {
                /* 4:2:2 is not supported */
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else if (ModeOption_p->Value.YCbCr444Input != Unit_p->Device_p->YCbCr444Input)
            {
                Unit_p->Device_p->YCbCr444Input = ModeOption_p->Value.YCbCr444Input;
                IsThereChange = TRUE;
            }
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* end of switch(ModeOption_p->Option) */

    if ( (ErrorCode == ST_NO_ERROR) && (IsThereChange))
    {
        ErrorCode = stdenc_HALSetEncodingMode(Unit_p->Device_p);
    }
    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    DoReport("STDENC_SetModeOption ", ErrorCode);
    return(ErrorCode);
} /* end of STDENC_SetModeOption */


/*
--------------------------------------------------------------------------------
API routine : Get mode option
ATTENTION : ModeOption_p->Option is an INPUT although in a "Get" function
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STDENC_GetModeOption (
                    const STDENC_Handle_t  Handle,
                    STDENC_ModeOption_t *  const ModeOption_p
                    )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STDENC_EncodingMode_t *CurrentMode_p;
    DENC_t *Unit_p;

    if (STDENC_CheckHandle(Handle) != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }
    if (ModeOption_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    Unit_p = (DENC_t *)Handle;

    /* Protect access */
    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    CurrentMode_p = &(Unit_p->Device_p->EncodingMode);
    switch(ModeOption_p->Option)
    {
        case STDENC_OPTION_BLACK_LEVEL_PEDESTAL :
            ModeOption_p->Value.BlackLevelPedestal = Unit_p->Device_p->BlackLevelPedestal;
            break;
        case STDENC_OPTION_BLACK_LEVEL_PEDESTAL_AUX :
            if (Unit_p->Device_p->DencVersion < 12 || !(Unit_p->Device_p->IsAuxEncoderOn))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                ModeOption_p->Value.BlackLevelPedestalAux = Unit_p->Device_p->BlackLevelPedestalAux;
            }
            break;
        case STDENC_OPTION_CHROMA_DELAY :
            ModeOption_p->Value.ChromaDelay = Unit_p->Device_p->ChromaDelay;
            break;
        case STDENC_OPTION_CHROMA_DELAY_AUX :
            if (Unit_p->Device_p->DencVersion < 12 || !(Unit_p->Device_p->IsAuxEncoderOn))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                ModeOption_p->Value.ChromaDelayAux = Unit_p->Device_p->ChromaDelayAux;
            }
            break;
        case STDENC_OPTION_LUMA_TRAP_FILTER :
            ModeOption_p->Value.LumaTrapFilter = Unit_p->Device_p->LumaTrapFilter;
            break;
        case STDENC_OPTION_LUMA_TRAP_FILTER_AUX :
            if (Unit_p->Device_p->DencVersion < 12 || !(Unit_p->Device_p->IsAuxEncoderOn))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                ModeOption_p->Value.LumaTrapFilterAux = Unit_p->Device_p->LumaTrapFilterAux;
            }
            break;
        case STDENC_OPTION_FIELD_RATE_60HZ :
            if (   (MODE_IS_PAL(CurrentMode_p->Mode))
                || (MODE_IS_SECAM(CurrentMode_p->Mode)))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                ModeOption_p->Value.FieldRate60Hz = CurrentMode_p->Option.Ntsc.FieldRate60Hz;
            }
            break;
        case STDENC_OPTION_NON_INTERLACED :
            if (CurrentMode_p->Mode == STDENC_MODE_SECAM)
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                if (MODE_IS_PAL(CurrentMode_p->Mode))
                {
                    ModeOption_p->Value.Interlaced = CurrentMode_p->Option.Pal.Interlaced;
                }
                else if (MODE_IS_NTSC(CurrentMode_p->Mode))
                {
                    ModeOption_p->Value.Interlaced = CurrentMode_p->Option.Ntsc.Interlaced;
                }
            }
            break;
        case STDENC_OPTION_SQUARE_PIXEL :
            if ((CurrentMode_p->Mode == STDENC_MODE_SECAM) && (Unit_p->Device_p->DencVersion < 10))
            {
                ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
            }
            else
            {
                if (MODE_IS_PAL(CurrentMode_p->Mode))
                {
                    ModeOption_p->Value.SquarePixel = CurrentMode_p->Option.Pal.SquarePixel;
                }
                else if (MODE_IS_NTSC(CurrentMode_p->Mode))
                {
                    ModeOption_p->Value.SquarePixel = CurrentMode_p->Option.Ntsc.SquarePixel;
                }
                else if (CurrentMode_p->Mode == STDENC_MODE_SECAM)
                {
                    ModeOption_p->Value.SquarePixel = Unit_p->Device_p->SECAMSquarePixel;
                }
            }
            break;
        case STDENC_OPTION_YCBCR444_INPUT :
            ModeOption_p->Value.YCbCr444Input = Unit_p->Device_p->YCbCr444Input;
            break;
        default :
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    } /* end of switch(ModeOption_p->Option) */

    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    DoReport("STDENC_GetModeOption ", ErrorCode);
    return(ErrorCode);
} /* end of STDENC_GetModeOption */

/*
--------------------------------------------------------------------------------
API routine : Set colorbar pattern
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t  STDENC_SetColorBarPattern(
                const STDENC_Handle_t       Handle,
                const BOOL                  ColorBarPattern
                )
 {
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    DENC_t *Unit_p;
    char string[30];

    sprintf(string, "STDENC_SetColorBarPattern ");

    if (STDENC_CheckHandle(Handle) != ST_NO_ERROR)
    {
        return(ST_ERROR_INVALID_HANDLE);
    }

    Unit_p = (DENC_t *)Handle;

    /* Protect access */
    STOS_SemaphoreWait(Unit_p->Device_p->ModeCtrlAccess_p);

    if (ColorBarPattern)
    {
        strcat(string, " On");
        ErrorCode = stdenc_HALSetColorBarPattern(Unit_p->Device_p);
    }
    else
    {
        strcat(string, " Off");
        ErrorCode = stdenc_HALRestoreConfiguration0(Unit_p->Device_p);
    }
    /* Free access */
    STOS_SemaphoreSignal(Unit_p->Device_p->ModeCtrlAccess_p);

    DoReport(string, ErrorCode);
    return(ErrorCode);
 } /* end of STDENC_SetColorBarPattern */

/* ----------------------------- End of file ------------------------------ */



