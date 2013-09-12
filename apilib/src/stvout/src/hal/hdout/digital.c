/*******************************************************************************

File name   : digital.c

Description : VOUT driver module : digital outputs.

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
25 Jul 2000        Created                                          JG
29 Oct 2001        Disable digital output when not used because of  HSdLM
 *                 noise set on 7015 HD DACS.
06 Dec 2001        New function to switch mode CCIR601/656 for      HSdLM
 *                 Digital Output (added for STi5514)
02 Jul 2003        Add pre-compiler option to make code lighter     HSdLM
21 Aou 2003        Split files/directories for better understanding HSdLM
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */

#include "vout_drv.h"
#include "digital.h"
#include "hd_reg.h"

/*******************************************************************************
Name        : stvout_HalEnableDigital
Description : enable digital output
Parameters  : BaseAddress : Configuration registers base address for Chip configuration
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalEnableDigital(stvout_Device_t * Device_p)
{
    #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
       UNUSED_PARAMETER(Device_p);
    #else
        U32 Value, RegAddress;
        #if defined (ST_7020)||defined (ST_7015)
            RegAddress = (U32)Device_p->DeviceBaseAddress_p + CFG_CCF;
            STOS_InterruptLock();
            Value = stvout_rd32(RegAddress);
            Value |= (U32)CFG_CCF_EVI;
            stvout_wr32( RegAddress, Value);
            STOS_InterruptUnlock();
        #else/*7710-7100-7109*/
            RegAddress = (U32)Device_p->BaseAddress_p + DSPCFG_DIG;
            STOS_InterruptLock();
            Value = stvout_rd32(RegAddress);
            Value |= (U32)DSPCFG_DIG_EN;
            stvout_wr32( RegAddress, Value);
            STOS_InterruptUnlock();
        #endif
    #endif
    return (ST_NO_ERROR);
}

/*******************************************************************************
Name        : stvout_HalDisableDigital
Description : disable digital output
Parameters  : BaseAddress : Configuration registers base address for Chip configuration
Assumptions :
Limitations :
Returns     : ST_ErrorCode_t
*******************************************************************************/
ST_ErrorCode_t stvout_HalDisableDigital( stvout_Device_t * Device_p)
{
    #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(Device_p);
    #else
        U32 Value, RegAddress;
        #if defined (ST_7020)||defined (ST_7015)
            RegAddress = (U32)Device_p->DeviceBaseAddress_p + CFG_CCF;
            STOS_InterruptLock();
            Value = stvout_rd32(RegAddress);
            Value &= (U32)~CFG_CCF_EVI;
            stvout_wr32( RegAddress, Value);
            STOS_InterruptUnlock();
        #else/*7710-7100-7109*/
            RegAddress = (U32)Device_p->BaseAddress_p + DSPCFG_DIG;
            STOS_InterruptLock();
            Value = stvout_rd32(RegAddress);
            Value &= (U32)~DSPCFG_DIG_EN;
            stvout_wr32( RegAddress, Value);
            STOS_InterruptUnlock();
        #endif
    #endif
    return (ST_NO_ERROR);
}


ST_ErrorCode_t stvout_HalSetDigitalRgbYcbcr( void* BaseAddress, U8 RgbYcbcr)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(RgbYcbcr);
    #else
        U32 Value;
        U32 Offset = DSPCFG_DIG;
        U32 Add = (U32)BaseAddress;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);
        switch ( RgbYcbcr)
        {
            case VOUT_OUTPUTS_HD_RGB :
                Value |= DSPCFG_DIG_RGB;
                break;
            case VOUT_OUTPUTS_HD_YCBCR :
                Value &= (U32)~DSPCFG_DIG_RGB;
                break;
            default:
                ErrorCode = ST_ERROR_BAD_PARAMETER;
                break;
        }
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif
    return (ErrorCode);
}

ST_ErrorCode_t stvout_HalSetDigitalEmbeddedSyncHd( void* BaseAddress, BOOL EmbeddedSync)
{
#if defined (ST_7020)||defined (ST_7015)
    U32 Value;
    U32 Offset = DSPCFG_DIG;
    U32 Add = (U32)BaseAddress;

    STOS_InterruptLock();
    Value = stvout_rd32( Add + Offset);
    if ( EmbeddedSync)
    {
        Value |= DSPCFG_DIG_SYHD;
    }
    else
    {
        Value &= (U32)~DSPCFG_DIG_SYHD;
    }
    stvout_wr32( Add + Offset, Value);
    STOS_InterruptUnlock();
#else
    UNUSED_PARAMETER (BaseAddress);
    UNUSED_PARAMETER(EmbeddedSync);
#endif
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSetDigitalEmbeddedSyncSd( void* BaseAddress, BOOL EmbeddedSync)
{
#if defined (ST_7020)||defined (ST_7015)
    U32 Value;
    U32 Offset = DSPCFG_DIG;
    U32 Add = (U32)BaseAddress;

    STOS_InterruptLock();
    Value = stvout_rd32( Add + Offset);
    if ( EmbeddedSync)
    {
        Value |= DSPCFG_DIG_SYSD;
    }
    else
    {
        Value &= (U32)~DSPCFG_DIG_SYSD;
    }
    stvout_wr32( Add + Offset, Value);
    STOS_InterruptUnlock();
#else
    UNUSED_PARAMETER (BaseAddress);
    UNUSED_PARAMETER(EmbeddedSync);
#endif
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSetDigitalMainAuxiliary( void* BaseAddress, U8 Hds)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    #if defined (ST_7200)|| defined (ST_7111)|| defined (ST_7105)
        UNUSED_PARAMETER(BaseAddress);
        UNUSED_PARAMETER(Hds);
    #else
        U32 Value;
        U32 Offset = DSPCFG_DIG;
        U32 Add = (U32)BaseAddress;

        STOS_InterruptLock();
        Value = stvout_rd32( Add + Offset);
        #if defined (ST_7020)||defined (ST_7015)
            switch ( Hds) {
                case MAIN :
                    Value |= DSPCFG_DIG_HDS;
                    break;
                case AUXILIARY :
                    Value &= (U32)~DSPCFG_DIG_HDS;
                    break;
                default:
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
        #else
            switch ( Hds)
            {
                case MAIN :
                    Value &= (U32)~DSPCFG_DIG_HDS;
                    break;
                case AUXILIARY :
                    Value |= DSPCFG_DIG_HDS;
                    break;
                default:
                    ErrorCode = ST_ERROR_BAD_PARAMETER;
                    break;
            }
        #endif
        stvout_wr32( Add + Offset, Value);
        STOS_InterruptUnlock();
    #endif
    return (ErrorCode);
}

/* ----------------------------- End of file ------------------------------ */
