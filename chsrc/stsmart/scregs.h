/*****************************************************************************
File Name   : scregs.h

Description : Smartcard register abstraction.

Copyright (C) 2006 STMicroelectronics

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SCREGS_H
#define __SCREGS_H

/* Includes --------------------------------------------------------------- */
#if defined(ST_OSLINUX)
#include <linux/stpio.h>
#endif

/* Exported Constants ----------------------------------------------------- */

/* ScClkCon register definitions */
#define CLOCK_ENABLE_MASK       0x2
#define CLOCK_SOURCE_MASK       0x1
#define CLOCK_SOURCE_GLOBAL     0x0
#define CLOCK_SOURCE_EXTERNAL   0x1

#define USE_STSYS   1
/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Smartcard clock control registers layout */
typedef struct
{
    U32 ScClkVal;
    U32 ScClkCon;
} SMART_ClkRegs_t;

/* Exported Macros -------------------------------------------------------- */

/* Smartcard clock manipulation */
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define SMART_ClockEnable(base)             STSYS_WriteRegDev32LE((&(base->ScClkCon)),(STSYS_ReadRegDev32LE((void *)(&(base->ScClkCon)))|((U32)CLOCK_ENABLE_MASK)));
#else
#define SMART_ClockEnable(base)             iowrite32((ioread32(base->ScClkCon)|((U32)CLOCK_ENABLE_MASK)) , base->ScClkCon);
#endif
#else
#define SMART_ClockEnable(base)             base->ScClkCon |= CLOCK_ENABLE_MASK
#endif
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define SMART_ClockDisable(base)            STSYS_WriteRegDev32LE((&(base->ScClkCon)), (STSYS_ReadRegDev32LE((void*)(&(base->ScClkCon)))&(~(U32)CLOCK_ENABLE_MASK)))
#else
#define SMART_ClockDisable(base)            iowrite32((ioread32(base->ScClkCon)&(~(U32)CLOCK_ENABLE_MASK)) , base->ScClkCon)
#endif
#else
#define SMART_ClockDisable(base)            base->ScClkCon &= ~CLOCK_ENABLE_MASK
#endif
#ifdef ST_OSLINUX
#ifdef USE_STSYS
#define SMART_SetClockSource(base, source)  STSYS_WriteRegDev32LE((&(base->ScClkCon)),(STSYS_ReadRegDev32LE((void*)(&(base->ScClkCon)))&(~(U32)CLOCK_SOURCE_MASK))); \
              STSYS_WriteRegDev32LE((&(base->ScClkCon)),(STSYS_ReadRegDev32LE((void*)(&(base->ScClkCon)))|source))
#else
#define SMART_SetClockSource(base, source)  iowrite32((ioread32(base->ScClkCon)&(~(U32)CLOCK_SOURCE_MASK)) , base->ScClkCon); \
              iowrite32((ioread32(base->ScClkCon)|source) , base->ScClkCon)
#endif
#else
#define SMART_SetClockSource(base, source)  base->ScClkCon &= ~CLOCK_SOURCE_MASK; \
                                            base->ScClkCon |= source
#endif

/* Vpp macros */
#define SMART_EnableVpp(sc)         STPIO_Set(sc->CmdVppHandle, \
                                              sc->InitParams.CmdVpp.BitMask); \
                                    sc->VppState = TRUE
#define SMART_DisableVpp(sc)        STPIO_Clear(sc->CmdVppHandle, \
                                                sc->InitParams.CmdVpp.BitMask); \
                                    sc->VppState = FALSE

/* Vcc macros (ACTIVE HIGH) */
#define SMART_EnableVcc(sc)         STPIO_Clear(sc->CmdVccHandle, \
                                                sc->InitParams.CmdVcc.BitMask)
#define SMART_DisableVcc(sc)        STPIO_Set(sc->CmdVccHandle, \
                                              sc->InitParams.CmdVcc.BitMask)

/* Reset line macros */
#define SMART_SetReset(sc)          STPIO_Set(sc->ResetHandle, \
                                              sc->InitParams.Reset.BitMask)
#define SMART_ClearReset(sc)        STPIO_Clear(sc->ResetHandle, \
                                                sc->InitParams.Reset.BitMask)

/* Clock line macros */
#define SMART_SetClock(sc)          STPIO_Set(sc->ClkHandle, \
                                              sc->InitParams.Clk.BitMask)
#define SMART_ClearClock(sc)        STPIO_Clear(sc->ClkHandle, \
                                                sc->InitParams.Clk.BitMask)

/* Detect line macro */
#if defined(STSMART_DETECT_INVERTED)
#define SMART_IsCardInserted(sc, pioreg) \
                 ((pioreg & sc->InitParams.Detect.BitMask) != 0) ? FALSE : TRUE
#else
#define SMART_IsCardInserted(sc, pioreg) \
                 ((pioreg & sc->InitParams.Detect.BitMask) != 0) ? TRUE : FALSE
#endif/*STSMART_DETECT_INVERTED*/

/* Exported Functions ----------------------------------------------------- */

#endif /* __SCREGS_H */

/* End of scregs.h */
