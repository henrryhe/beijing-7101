/* ----------------------------------------------------------------------------
File Name: reg0297.c (was reg0297.c)

Description:

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

#ifndef ST_OSLINUX
   
#include <string.h>

#endif


/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "ioarch.h"     /* IO access layer */
#include "ioreg.h"      /* register mapping */

#include "d0297.h"      /* top level driver header */
#include "reg0297.h"    /* header for this file */



/* functions --------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
Name: Reg0297_Install()

Description:
    install driver register table

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297_Install(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_Install()";
#endif
   ST_ErrorCode_t Error = ST_NO_ERROR;

   
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
        STTBX_Print(("%s fail error=%d\n", identity, Error));
#endif
    }
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
        STTBX_Print(("%s installed ok\n", identity));
#endif
    }

    return (Error);
}



/* ----------------------------------------------------------------------------
Name: Reg0297_Open()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297_Open(STTUNER_IOREG_DeviceMap_t *DeviceMap, U32 ExternalClock)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

    DeviceMap->RegExtClk = ExternalClock;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s External clock=%u KHz\n", identity, ExternalClock));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: Reg0297_Close()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297_Close(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s\n", identity));
#endif
    return(Error);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_UnInstall()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
ST_ErrorCode_t Reg0297_UnInstall(STTUNER_IOREG_DeviceMap_t *DeviceMap)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_UnInstall()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s\n", identity));
#endif
    return(Error);
}


/* ----------------------------------------------------------------------------
Name: Reg0297_SetQAMSize()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_SetQAMSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, STTUNER_Modulation_t Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_SetQAMSize()";
#endif

    switch(Value)
    {
        case STTUNER_MOD_16QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM16\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_MODE_SELECT, D0297_QAM16);
            break;
        case STTUNER_MOD_32QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM32\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_MODE_SELECT, D0297_QAM32);
            break;
        case STTUNER_MOD_64QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM64\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_MODE_SELECT, D0297_QAM64);
            break;
        case STTUNER_MOD_128QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM128\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_MODE_SELECT, D0297_QAM128);
            break;
        case STTUNER_MOD_256QAM:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM256\n", identity));
#endif
            STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_MODE_SELECT, D0297_QAM256);
            break;
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s fail Bad Parameter\n", identity));
#endif
            break;
    }
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetQAMSize()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
STTUNER_Modulation_t Reg0297_GetQAMSize(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetQAMSize()";
#endif
    STTUNER_Modulation_t Value;

    Value = STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297_MODE_SELECT);

    switch(Value)
    {
        case D0297_QAM16:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM16\n", identity));
#endif
            return STTUNER_MOD_16QAM;
        case D0297_QAM32:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM32\n", identity));
#endif
            return STTUNER_MOD_32QAM;
        case D0297_QAM64:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM64\n", identity));
#endif
            return STTUNER_MOD_64QAM;
        case D0297_QAM128:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM128\n", identity));
#endif
            return STTUNER_MOD_128QAM;
        case D0297_QAM256:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s QAM256\n", identity));
#endif
            return STTUNER_MOD_256QAM;
        default:
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
            STTBX_Print(("%s fail Bad Parameter\n", identity));
#endif
            return STTUNER_MOD_ALL;
            break;
    }
}


/* ----------------------------------------------------------------------------
Name: Reg0297_WBAGCOff()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_WBAGCOff(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_WBAGCOff()";
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> WAGCOFF\n", identity));
#endif

    STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, R0297_WBAGC_3, D0297_WBAGCON);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_WBAGCOn()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_WBAGCOn(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_WBAGCOn()";
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> WAGCON\n", identity));
#endif

    STTUNER_IOREG_SetRegister(DeviceMap, IOHandle, R0297_WBAGC_3, D0297_WBAGCOFF);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_SetAGC2()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_SetAGC2(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U16 Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_SetAGC2()";
#endif

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> AGC2 %u\n", identity, Value));
#endif
U8 temp[1],try1;
    
    try1=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R0297_WBAGC_2);

    temp[0]=(int)(MAC0297_B0(Value));
    try1=try1&0xFC;
    temp[1]=try1|(int)(MAC0297_B1(Value));
    STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0297_WBAGC_1, temp, 2);

}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetAGC()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U16 Reg0297_GetAGC(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetAGC()";
#endif
    U16 Value;
    U8 temp[1];
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297_WBAGC_1, 2, temp);
    temp[1]=temp[1]&0x03;
     Value = MAC0297_MAKEWORD(temp[1],temp[0]);
   
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> AGC2 %u\n", identity, Value));
#endif

    return (Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_SetSymbolRate()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_SetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned long Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_SetSymbolRate()";
#endif
    long    ExtClk;
    U8      temp[4];
    /**/
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> SR %u\n", identity, Value));
#endif
    ExtClk = DeviceMap->RegExtClk;
    
    if (ExtClk != 0)
    {
        Value   /= 1000;
        ExtClk  /= 2;
        Value   *= 131072L;
        Value   /= ExtClk;
        Value   *= 8192L;
        Value   *= 2;
       
        temp[0]=(int)(MAC0297_B0(Value));
        temp[1]=(int)(MAC0297_B1(Value));
        temp[2]=(int)(MAC0297_B2(Value));
        temp[3]=(int)(MAC0297_B3(Value));
        STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0297_STLOOP_5, temp, 4);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
        STTBX_Print(("%s fail RegExtClk = 0 !\n", identity));
#endif
        return;
    }
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetSymbolRate()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
unsigned long Reg0297_GetSymbolRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetSymbolRate()";
#endif
    unsigned long   Value;
    long            ExtClk;
    U8 temp[3];

    /**/
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297_STLOOP_5, 4, temp);

    Value = temp[0] + (temp[1])*256L;
    Value += (temp[2])*65536L + (temp[3])*16777216L;

    ExtClk   = DeviceMap->RegExtClk;

    Value   /= 4;
    Value   /= 8192L;
    Value   *= ExtClk;
    Value   /= 131072L;
    Value   *= 1000;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> SR %u\n", identity, Value));
#endif

    return(Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_SetSweepRate()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_SetSweepRate(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, unsigned long Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_SetSweepRate()";
#endif
    long   long_tmp;
    long   RegSymbolRate ;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> SWEEP %u\n", identity, Value));
#endif

    RegSymbolRate = Reg0297_GetSymbolRate(DeviceMap, IOHandle);        /* in Baud/s */
    if ( RegSymbolRate == 0 || Value == 0 )
    {
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
        STTBX_Print(("%s fail SymbolRate = 0 !\n", identity));
#endif
    }
    else
    {
        RegSymbolRate      /= 1000;

        long_tmp = Value * 262144L;
        long_tmp /= RegSymbolRate;
        long_tmp *= 1024;
        /* rounding */
        if(long_tmp >= 0)
            long_tmp += 500000 ;
        else
            long_tmp -= 500000 ;
        long_tmp /= 1000000 ;
        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_SWEEP_LO, (int)(MAC0297_B0(long_tmp)));
        STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_SWEEP_HI, (int)(MAC0297_B1(long_tmp)));
    }
}

/* ----------------------------------------------------------------------------
Name: Reg0297_SetFrequencyOffset()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_SetFrequencyOffset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long Value)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_SetFrequencyOffset()";
#endif
    long    long_tmp;
    U8      temp[3],try1;

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> FrequencyOffset %d\n", identity, Value));
#endif
    long_tmp = Value * 26844L ;                 /* (2**28)/10000 */
    if(long_tmp < 0) long_tmp += 0x10000000 ;
    long_tmp &= 0x0FFFFFFF ;

    try1=STTUNER_IOREG_GetRegister(DeviceMap,IOHandle,R0297_CRL_6);
    temp[0]=(int)(MAC0297_B0(long_tmp));
    temp[1]=(int)(MAC0297_B1(long_tmp));
    temp[2]=(int)(MAC0297_B2(long_tmp));
    try1=try1&0xF0;
    temp[3]=try1|(int)(MAC0297_B3(long_tmp));
    STTUNER_IOREG_SetContigousRegisters(DeviceMap, IOHandle, R0297_CRL_6, temp, 4);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetFrequencyOffset()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
long Reg0297_GetFrequencyOffset(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetFrequencyOffset()";
#endif
    long    Value;
    U8 temp[3];

    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297_CRL_6, 4, temp);
    temp[3]=temp[3]&0x0F;
    Value = temp[0] + (temp[1])*256L;
    Value += (temp[2])*65536L + (temp[3])*16777216L;

    if (Value >= 0x08000000)
    {
        Value -= 0x10000000 ;                   /* 0x10000000 = 2**28  */
    }
    Value /= 2684354L ;                      /* (2**28)/100 */
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> FrequencyOffset %d\n", identity, Value));
#endif
    return(Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetSTV0297Id()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U8 Reg0297_GetSTV0297Id(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetSTV0297Id()";
#endif
    U8    Value=0xFF;

    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_VERSION, Value);      /* To Prevent I2C access Pb */
    Value =   STTUNER_IOREG_GetField(DeviceMap, IOHandle, F0297_VERSION);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> Version %x\n", identity, Value));
#endif
    return(Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_StartBlkCounter()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_StartBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_CT_HOLD, 0);
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_CT_CLEAR, 0);
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_CT_CLEAR, 1);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_StopBlkCounter()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
void Reg0297_StopBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
    STTUNER_IOREG_SetField(DeviceMap, IOHandle, F0297_CT_HOLD, 1);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetBlkCounter()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U16 Reg0297_GetBlkCounter(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetBlkCounter()";
#endif
    U16    Value;
    U8 temp[1];
    U16 val;

    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297_RS_DESC_0, 2, temp);
    val=temp[1];
    Value = temp[0]+ (val<< 8);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> BlkCounter %u\n", identity, Value));
#endif
    return(Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetCorrBlk()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U16 Reg0297_GetCorrBlk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetCorrBlk()";
#endif
    U16    Value,val;
    U8 temp[1];
    
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297_RS_DESC_2, 2, temp);
    val=temp[1];
    Value = temp[0] + (val<< 8);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> CorrBlk %u\n", identity, Value));
#endif
    return(Value);
}

/* ----------------------------------------------------------------------------
Name: Reg0297_GetUncorrBlk()

Description:

Parameters:

Return Value:
---------------------------------------------------------------------------- */
U16 Reg0297_GetUncorrBlk(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle)
{
#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    const char *identity = "STTUNER reg0297.c Reg0297_GetUncorrBlk()";
#endif
    U16    Value,val;
    U8 temp[1];
    STTUNER_IOREG_GetContigousRegisters(DeviceMap, IOHandle, R0297_RS_DESC_4, 2, temp);
    val=temp[1];
    Value = temp[0]+ (val<< 8);

#ifdef STTUNER_DEBUG_MODULE_CABDRV_REG0297
    STTBX_Print(("%s ==> UncorrBlk %u\n", identity, Value));
#endif
    return(Value);
}
