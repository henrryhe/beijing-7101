/*******************************************************************************

File name   : sd_dig.c

Description : VOUT driver module : SD digital outputs.
 *            5514/16/17/28 Digital Output (CCIR 656) YCbCr4:2:2

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
21 Aug 2003        Extracted from old 'hdout/digital.c' file        HSdLM
10 Mar 2004        Add support on STi5100                           AT
*******************************************************************************/

/* Includes ----------------------------------------------------------------- */

#include "stsys.h"

#include "vout_drv.h"
#include "sd_dig.h"

/* Private Constants -------------------------------------------------------- */

#define VID_656     0x32   /*  CCIR656 mode control register for STi5514/16/17 */

#define DVO_CTL_656 0x00   /*  CCIR656 mode control register for STi5528 */
#if defined(ST_5528)||defined(ST_5100)||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
#define DVO_CTL_656_POE             0x00000002
#define DVO_CTL_656_ADDYDS          0x00000008
#define DVO_CTL_656_EN_656_MODE     0x00000001
#define DVO_CTL_656_DIS_1_254       0x00010000
#define DVO_CTL_PADS                0x04
#define DVO_CTL_PADS_RESET_VALUE    0x00
#define DVO_VPO                     0x08
#define DVO_VPS                     0x0C
#endif

#ifdef STVOUT_DVO
#define DVO_CTL     DVO_CTL_656
#else
#define DVO_CTL     VID_656
#endif

#define DVO_CTL_E6M 0x01   /*  Enable 656 mode register 0: CCIR601 /  1: CCIR656 */

/* Viewport params for 625 lines systems */
#define XDO_625 131
#define YDO_625 46
#define XDS_625 852
#define YDS_625 621

/* Viewport params for 525 lines systems */
#define XDO_525 121
#define YDO_525 34
#define XDS_525 842
#define YDS_525 519

/* Private types ------------------------------------------------------- */
typedef struct ViewPortSettings_s
{
    U16 XOffset;
    U16 YOffset;
    U16 XStop;
    U16 YStop;
} ViewPortSettings_t;
/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Private functions prototypes--------------------------------------------------------- */
ST_ErrorCode_t stvout_HalSetDigitalOutputViewport( void* BaseAddress_p, ViewPortSettings_t* ViewPortSettings_p);

/* Private functions --------------------------------------------------------- */
ST_ErrorCode_t stvout_HalSetDigitalOutputViewport( void* BaseAddress_p, ViewPortSettings_t* ViewPortSettings_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

#if defined(ST_5528)||defined(ST_5100)||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
    STSYS_WriteRegDev32LE((void*)((U8*) BaseAddress_p + DVO_VPO), (ViewPortSettings_p->YOffset << 16) | ViewPortSettings_p->XOffset);
    STSYS_WriteRegDev32LE((void*)((U8*) BaseAddress_p + DVO_VPS), (ViewPortSettings_p->YStop << 16) | ViewPortSettings_p->XStop);
#endif /* ST_5528-ST_5100-ST_5301 */

    return(ErrorCode);
} /* stvout_HalSetDigitalOutputViewport */

/* Exported functions ---------------------------------------------------------*/
ST_ErrorCode_t stvout_HalSetDigitalOutputCCIRMode( void* BaseAddress_p, BOOL EmbeddedSync)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#if defined(ST_5528)||defined(ST_5100)||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
    U32 Value;
#else
    U8 Value;
#endif
    U8* Add_p = (U8*)BaseAddress_p + DVO_CTL;

#if defined(ST_5528)||defined(ST_5100)||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
    Value = STSYS_ReadRegDev32LE((void *)Add_p);
#else
    Value = STSYS_ReadRegDev8((void *)Add_p);
#endif
    if ( EmbeddedSync)
    {
        Value |= DVO_CTL_E6M;
    }
    else
    {
        Value &= (U8)~DVO_CTL_E6M;
    }
#if defined(ST_5528)||defined(ST_5100)||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
    Value |= DVO_CTL_656_DIS_1_254;
    STSYS_WriteRegDev32LE((void *)Add_p, Value);
#else
    STSYS_WriteRegDev8((void *)Add_p, Value);
#endif
    return (ErrorCode);
} /* stvout_HalSetDigitalOutputCCIRMode */

ST_ErrorCode_t stvout_HalSetEmbeddedSystem( void* BaseAddress_p, STVOUT_EmbeddedSystem_t EmbeddedSystem)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#if defined(ST_5528)||defined(ST_5100)||defined (ST_5301)|| defined(ST_5107)|| defined(ST_5162)
    U32 Value;
    U8* Add_p;
    ViewPortSettings_t ViewPortSettings;


    Add_p = (U8*)BaseAddress_p + DVO_CTL;
    Value = STSYS_ReadRegDev32LE((void *)Add_p);
    switch( EmbeddedSystem )
    {
        case STVOUT_EMBEDDED_SYSTEM_525_60:
            Value |= (U32)DVO_CTL_656_POE;
            Value |= (U32)DVO_CTL_656_ADDYDS;
            Value |= (3 << 8);
            ViewPortSettings.XOffset = XDO_525;
            ViewPortSettings.YOffset = YDO_525;
            ViewPortSettings.XStop   = XDS_525;
            ViewPortSettings.YStop   = YDS_525;
            break;
        case STVOUT_EMBEDDED_SYSTEM_625_50:
            Value &= ~(U32)DVO_CTL_656_POE;
            Value &= ~(U32) DVO_CTL_656_ADDYDS;
            Value |= (2 << 8);
            ViewPortSettings.XOffset = XDO_625;
            ViewPortSettings.YOffset = YDO_625;
            ViewPortSettings.XStop   = XDS_625;
            ViewPortSettings.YStop   = YDS_625;
            break;
        default:
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }
    if ( ErrorCode == ST_NO_ERROR)
    {
        STSYS_WriteRegDev32LE((void *)Add_p, Value);
        Add_p = (U8*)BaseAddress_p + DVO_CTL_PADS;
        STSYS_WriteRegDev8((void *)Add_p, DVO_CTL_PADS_RESET_VALUE);
        ErrorCode = stvout_HalSetDigitalOutputViewport ((void*) BaseAddress_p, &ViewPortSettings);
    }
#endif /* ST_5528 - ST_5100 - ST_5301*/
    return (ErrorCode);
} /* stvout_HalSetDigitalOutputEncoding */

/* ----------------------------- End of file ------------------------------ */
