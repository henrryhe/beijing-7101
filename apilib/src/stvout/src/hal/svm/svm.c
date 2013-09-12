/************************************************************************
File Name   : svm.c
Description : VOUT driver module for svm output.
COPYRIGHT (C) STMicroelectronics 2000

************************************************************************/
/*
 01.24.01  creation.
*/
/*
******************************************************************************
Includes
******************************************************************************
*/
#include "vout_drv.h"
#include "svm.h"
#include "svm_reg.h"

#include <stdio.h>

/* Private Constants */
#define BLANKDACSVM 0x00000100

/*
------------------------------------------------------------------------------
Disable/Enable the output signals by writing useful Analog register
parameter Number : 0-4 -> Dac 0-4
------------------------------------------------------------------------------
*/
ST_ErrorCode_t stvout_HalEnableSVMDac( void* BaseAddress, U8 Dac)
{
    U32 Value;
    U32 Offset = DSPCFG_DAC;
    U32 Add = (U32)BaseAddress;

    STOS_InterruptLock();
    Value = stvout_rd32( Add + Offset);
    Value &= (U32)~DSPCFG_DAC_BLRGB;         /* don't use BLRGB bit */
    Value &= (U32)~(BLANKDACSVM << Dac) ;
    stvout_wr32( Add + Offset, Value);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalDisableSVMDac( void* BaseAddress, U8 Dac)
{
    U32 Value;
    U32 Offset = DSPCFG_DAC;
    U32 Add = (U32)BaseAddress;

    STOS_InterruptLock();
    Value = stvout_rd32( Add + Offset);
    Value &= (U32)~DSPCFG_DAC_BLRGB;         /* don't use BLRGB bit */
    Value |= (BLANKDACSVM << Dac) ;
    stvout_wr32( Add + Offset, Value);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSVMDelayComp( void* BaseAddress, U32 DelayComp)
{
    STOS_InterruptLock();
    stvout_wr32( (void*)((U32)(BaseAddress) + (U32)SVM_DELAY_COMP), DelayComp);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSVMModulationShapeControl( void* BaseAddress, U32 ShapeType)
{
    STOS_InterruptLock();
    stvout_wr32( (void*)((U32)(BaseAddress) + (U32)SVM_SHAPE), ShapeType);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSVMGainControl( void* BaseAddress, U32 Gain)
{
    STOS_InterruptLock();
    stvout_wr32( (void*)((U32)(BaseAddress) + (U32)SVM_GAIN), Gain);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSVMOSDFactor( void* BaseAddress, U32 Factor)
{
    STOS_InterruptLock();
    stvout_wr32( (void*)((U32)(BaseAddress) + (U32)SVM_OSD_FACTOR), Factor);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

ST_ErrorCode_t stvout_HalSVMFilter( void* BaseAddress, U32 Filter)
{
    STOS_InterruptLock();
    stvout_wr32( (void*)((U32)(BaseAddress) + (U32)SVM_FILTER), Filter);
    STOS_InterruptUnlock();
    return (ST_NO_ERROR);
}

/* ----------------------------- End of file ------------------------------ */
