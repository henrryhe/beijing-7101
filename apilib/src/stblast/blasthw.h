/*****************************************************************************
File Name   : blasthw.h
Description : IR blaster register definitions.

Copyright (C) 2000 STMicroelectronics

History     :

Reference   :

IR Blaster functional specification (ADCS 7133158)
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __BLASTHW_H
#define __BLASTHW_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */
#include "blastint.h"

#include "stos.h"

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_7100) || defined(ST_5301) \
 || defined(ST_8010) || defined(ST_7109)  || defined(ST_5525) || defined(ST_5188) ||defined(ST_5107) \
 || defined(ST_7200)
#ifndef NEW_INTERRUPT_MECHANISM
#define    NEW_INTERRUPT_MECHANISM
#endif
#endif

/* Exported Types --------------------------------------------------------- */

typedef struct
{
    /* IR transmitter registers */
    U32 TxScalerIr;
    U32 TxSubCarrierIr;
    U32 TxSymbolTimeIr;
    U32 TxOnTimeIr;
    U32 TxIntEnableIr;
    U32 TxIntStatusIr;
    U32 TxEnableIr;
    U32 TxClrUnderrunIr;
    U32 TxSubcarrierWidth;              /* Only present on 5514 */
#ifdef NEW_INTERRUPT_MECHANISM
    U32 TxStatusIr ;
#endif

} BLAST_TxRegs_s;

#ifdef ST_OS20
#if !defined(PROCESSOR_C1)
#pragma ST_device(BLAST_TxRegs_t)
#endif
#endif

typedef volatile BLAST_TxRegs_s BLAST_TxRegs_t;

typedef struct
{
    /* IR receiver registers */
    U32 RxOnTimeIr;
    U32 RxSymbolTimeIr;
    U32 RxIntEnableIr;
    U32 RxIntStatusIr;
    U32 RxEnableIr;
    U32 RxMaxSymbolTimeIr;
    U32 RxClrOverrunIr;
    U32 RxNoiseSuppressWidthIr;
#ifdef NEW_INTERRUPT_MECHANISM
    U32 IrdaAscControl;
    U32 RxSamplingRateCommon;
    U32 RxPolinvRegIr;
    U32 RxStatusIr;
#endif
} BLAST_RxRegs_s;

#ifdef ST_OS20
#if !defined(PROCESSOR_C1)
#pragma ST_device(BLAST_RxRegs_t)
#endif
#endif

typedef volatile BLAST_RxRegs_s BLAST_RxRegs_t;

typedef struct
{
    /* IrDA interface registers */
    U32 BaudRateGenIrda;
    U32 BaudGenEnableIrda;
    U32 TxEnableIrda;
    U32 RxEnableIrda;
    U32 IrdaAscControl;
    U32 RxPulseStatusIrda;
    U32 RxSamplingRateIrda;
    U32 RxMaxSymbolTimeIrda;
} BLAST_IrdaRegs_s;

#ifdef ST_OS20
#if !defined(PROCESSOR_C1)
#pragma ST_device(BLAST_IrdaRegs_t)
#endif
#endif



typedef volatile BLAST_IrdaRegs_s BLAST_IrdaRegs_t;

#ifdef ST_OS20
#if !defined(PROCESSOR_C1)
#pragma ST_device(BLAST_DU32)
#endif
#endif

typedef volatile U32 BLAST_DU32;

/* Register map (approximate) for IR Blaster
 * Receive/transmit symbol and on times have (at time of writing) a four-word
 * FIFO in place, of which three are accessible by the software.
 */
typedef struct BLAST_BlastRegs_s
{
    /* IR transmitter registers */
    BLAST_TxRegs_t      *TxRegs_p;

    /* IR receiver registers */
    BLAST_RxRegs_t      *RxRegs_p;

    /* IrDA interface registers */
    BLAST_IrdaRegs_t    *IrdaRegs_p;

    /* "Unique" registers */
    BLAST_DU32          *RxIrdaControl;

    /* Common to both UHF and IR RC receivers */
    BLAST_DU32          *RxSamplingRateCommon;

    /* Polarity Invert for RX */
    BLAST_DU32          *RxPolarityInvert;

} BLAST_BlastRegs_t;

/* Exported Constants ----------------------------------------------------- */

#define TX_REGS_OFFSET              0x00
#define RX_REGS_OFFSET              0x40
#define RX_SAMPLING_RATE_OFFSET     0x64
#define RX_POLARITY_INVERT_OFFSET   0x68
#define IRB_POLINV_REG_UHF          0xA8

#ifdef NEW_INTERRUPT_MECHANISM
    #define FIFO_SIZE   8
    #define DATA_IN_TX_FIFO_MASK    0x00ff
#else
    #define FIFO_SIZE   4
#endif


/* Transmitter interrupt enable (and when)
Bit  0      int enable
Bits 4:5    interrupt generation - 00 invalid, 01 single word empty,
                                   10 half empty, 11 totally empty
*/
#ifdef NEW_INTERRUPT_MECHANISM
  #define TXIE_ENABLE          0x01
  #define TXIE_UNDERRUN        0x02
  #define TXIE_ONEEMPTY        0x10
  #define TXIE_TWOEMPTY        0x08 /* actually it is 4 empty */
  #define TXIE_THREEEMPTY      0x04 /* actually it is 8 empty */
  #define TXIE_SEVENEMPTY      TXIE_THREEEMPTY
  #define TXIE_HALFEMPTY       TXIE_TWOEMPTY
#else
  #define TXIE_ENABLE          0x01
  #define TXIE_ONEEMPTY        0x10
  #define TXIE_TWOEMPTY        0x20
  #define TXIE_THREEEMPTY      0x30
#endif
/* Transmitter status register
 Bit 0      interrupt enabled
 Bit 1      underrun occurred
Bits 4:5    00 fifo full, 01 one block empty, 10 two empty, 11 three empty
*/
#ifdef NEW_INTERRUPT_MECHANISM
  #define TXIS_ENABLE          0x01
  #define TXIS_UNDERRUN        0x02
  #define TXIS_ONEEMPTY        0x10
  #define TXIS_TWOEMPTY        0x08 /* actually it is 4 empty */
  #define TXIS_THREEEMPTY      0x04 /* actually it is 8 empty */
  #define TXIS_SEVENEMPTY      TXIS_THREEEMPTY
  #define TXIS_HALFEMPTY       TXIS_TWOEMPTY
#else
  #define TXIS_ENABLE          0x01
  #define TXIS_UNDERRUN        0x02
  #define TXIS_ONEEMPTY        0x10
  #define TXIS_TWOEMPTY        0x20
  #define TXIS_THREEEMPTY      0x30
#endif

/* Receiver interrupt enable (and when)
Bit    0    int enable
Bit    1    leave interrupt enabled on last symbol (1 == true)
Bits 4:5    interrupt generation - 00 invalid, 01 single word present,
                                   01 half full, 11 totally full
 */
#ifdef NEW_INTERRUPT_MECHANISM
  #define RXIE_ENABLE          0x01
  #define RXIE_LEAVEENABLED    0x02
  #define RXIE_OVERRUN         0x04
  #define RXIE_ONEFULL         0x20  /* 1 word full */
  #define RXIE_TWOFULL         0x10  /* 4 word full */
  #define RXIE_THREEFULL       0x08  /* 8 Word full */
  #define RXIE_SEVENFULL       RXIE_THREEFULL
  #define RXIE_HALFFULL        RXIE_TWOFULL
#else
  #define RXIE_ENABLE          0x01
  #define RXIE_LEAVEENABLED    0x02
  #define RXIE_ONEFULL         0x10
  #define RXIE_TWOFULL         0x20
  #define RXIE_THREEFULL       0x30
#endif


/* Receiver status register
Bit 0       interrupt enabled
Bit 1       last symbol interrupt active
Bit 2       receiver overrun
Bits 4:5    FIFO level
*/
#ifdef NEW_INTERRUPT_MECHANISM
  #define RXIS_ENABLE          0x01
  #define RXIS_LEAVEENABLED    0x02
  #define RXIS_OVERRUN         0x04
  #define RXIS_ONEFULL         0x20  /* 1 word full */
  #define RXIS_TWOFULL         0x10  /* 4 word full */
  #define RXIS_THREEFULL       0x08  /* 8 Word full */
  #define RXIS_SEVENFULL       RXIS_THREEFULL
  #define RXIS_HALFFULL        RXIS_TWOFULL
#else
  #define RXIS_ENABLE          0x01
  #define RXIS_LEAVEENABLED    0x02
  #define RXIS_OVERRUN         0x04
  #define RXIS_ONEFULL         0x10
  #define RXIS_TWOFULL         0x20
  #define RXIS_THREEFULL       0x30
#endif

/* Exported Macros -------------------------------------------------------- */

/* RC Tx interface (IR) */
#if defined(ST_OSLINUX)
#define BLAST_TxSetPrescaler(base, v)   STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxScalerIr)), \
                                                                (v & 0x00FF))
#define BLAST_TxSetSubCarrier(base, v)  STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxSubCarrierIr)), \
                                                                (v & 0xFFFF))
#define BLAST_TxSetSymbolTime(base, v)  STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxSymbolTimeIr)), \
                                                                (v & 0xFFFF))
#define BLAST_TxSetOnTime(base, v)      STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxOnTimeIr)), \
                                                                (v & 0xFFFF))
#else
#define BLAST_TxSetPrescaler(base, v)     base->TxRegs_p->TxScalerIr = (v & 0x00FF)
#define BLAST_TxSetSubCarrier(base, v)    base->TxRegs_p->TxSubCarrierIr = (v & 0xFFFF)
#define BLAST_TxSetSymbolTime(base, v)    base->TxRegs_p->TxSymbolTimeIr = (v & 0xFFFF)
#define BLAST_TxSetOnTime(base, v)        base->TxRegs_p->TxOnTimeIr = (v & 0xFFFF)
#endif

/* Various Tx interrupt flags */
#if defined(ST_OSLINUX) && defined(LINUW_FULL_KERNEL_VERSION)
#define BLAST_TxEnableInt(base)           STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxIntEnableIr)), \
                                              (STSYS_ReadRegDev32LE((void*) \
                                                                    (&(base->TxRegs_p->TxIntEnableIr))) \
                                                  | TXIE_ENABLE))

#define BLAST_TxDisableInt(base)          STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxIntEnableIr)), \
                                              (STSYS_ReadRegDev32LE((void*) \
                                                                    (&(base->TxRegs_p->TxIntEnableIr))) \
                                                  | ~TXIE_ENABLE))

#define BLAST_TxIntIsEnabled(base)        (STSYS_ReadRegDev32LE((void*)(&(base->TxRegs_p->TxIntEnableIr))))

#define BLAST_TxSetIE(base, v)            STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxIntEnableIr)), \
                                              (STSYS_ReadRegDev32LE((void*) \
                                                                    (&(base->TxRegs_p->TxIntEnableIr))) \
                                                  | v))

#define BLAST_TxGetIE(base)             (STSYS_ReadRegDev32LE((void*)(&(base->TxRegs_p->TxIntEnableIr))))
#else
#define BLAST_TxEnableInt(base)           base->TxRegs_p->TxIntEnableIr |= TXIE_ENABLE
#define BLAST_TxDisableInt(base)          base->TxRegs_p->TxIntEnableIr &= ~TXIE_ENABLE
#define BLAST_TxIntIsEnabled(base)        (base->TxRegs_p->TxIntEnableIr)
#define BLAST_TxSetIE(base, v)            base->TxRegs_p->TxIntEnableIr |= v
#define BLAST_TxGetIE(base)               (base->TxRegs_p->TxIntEnableIr)
#endif

/* Disable/enable transmissions */
#if defined(ST_OSLINUX)
#define BLAST_TxEnable(base)              STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxEnableIr)), 1)

#define BLAST_TxDisable(base)             STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxEnableIr)), 0)

#define BLAST_TxIsEnabled(base)           (STSYS_ReadRegDev32LE((void*)(&(base->TxRegs_p->TxEnableIr)))!= 0)

#define BLAST_ClrUnderrun(base)           STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxClrUnderrunIr)), 1)

#define BLAST_TXClrInts(base)             STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxClrUnderrunIr)), 0x1E)
#else
#define BLAST_TxEnable(base)              base->TxRegs_p->TxEnableIr = 1
#define BLAST_TxDisable(base)             base->TxRegs_p->TxEnableIr = 0
#define BLAST_TxIsEnabled(base)           (base->TxRegs_p->TxEnableIr != 0)
#define BLAST_ClrUnderrun(base)           base->TxRegs_p->TxClrUnderrunIr = 1
#define BLAST_TXClrInts(base)             base->TxRegs_p->TxClrUnderrunIr = 0x1E
#endif

#if defined(ST_OSLINUX)
#define BLAST_TxGetPreScaler(base)        ((STSYS_ReadRegDev32LE((void*)(&(base->TxRegs_p->TxScalerIr)))) \
                                           & 0xFFFF)

#define BLAST_TxGetSubCarrier(base)       ((STSYS_ReadRegDev32LE((void*)(&(base->TxRegs_p->TxSubCarrierIr)))) \
                                           & 0xFFFF)
#else
#define BLAST_TxGetPreScaler(base)        (base->TxRegs_p->TxScalerIr & 0xFFFF)
#define BLAST_TxGetSubCarrier(base)       (base->TxRegs_p->TxSubCarrierIr & 0xFFFF)
#endif


#ifdef NEW_INTERRUPT_MECHANISM
#if defined(ST_OSLINUX)
#define BLAST_TxGetStatus(base)           (STSYS_ReadRegDev32LE((void*)(&(base->TxRegs_p->TxStatusIr))))
#else
#define BLAST_TxGetStatus(base)           (base->TxRegs_p->TxStatusIr)
#endif
#else /* NEW_INTERRUPT_MECHANISM */
#if defined(ST_OSLINUX)
#define BLAST_TxGetStatus(base)           (STSYS_ReadRegDev32LE((void*) \
                                                                    (&(base->TxRegs_p->TxIntStatusIr))))
#else
#define BLAST_TxGetStatus(base)           (base->TxRegs_p->TxIntStatusIr)
#endif
#endif /* NEW_INTERRUPT_MECHANISM */

/* See if the bits of the FIFO we can touch are empty */
#if defined(ST_OSLINUX)
#define BLAST_TxIsEmpty(base)             ((STSYS_ReadRegDev32LE((void*) \
                                                                 (&(base->TxRegs_p->TxIntStatusIr))) \
                                           & TXIS_THREEMPTY) == TXIS_THREEMPTY)
#else
#define BLAST_TxIsEmpty(base)             ((base->TxRegs_p->TxIntStatusIr & TXIS_THREEEMPTY) == TXIS_THREEEMPTY)
#endif

/* Bits 4:5 both zero == TX is full */
#if defined(ST_OSLINUX)
#define BLAST_TxIsFull(base)              ((STSYS_ReadRegDev32LE((void*) \
                                                                 (&(base->TxRegs_p->TxIntStatusIr))) \
                                           & TXIS_THREEMPTY) == 0)

#define BLAST_TxSetSubcarrierWidth(base, v) \
                                          (STSYS_WriteRegDev32LE((&(base->TxRegs_p->TxSubcarrierWidth)),(v & 0xFFFF)))

#define BLAST_TxGetSubcarrierWidth(base)  (STSYS_ReadRegDev32LE((void*) \
                                                                (&(base->TxRegs_p->TxSubcarrierWidth))))
#else
#define BLAST_TxIsFull(base)              ((base->TxRegs_p->TxIntStatusIr & TXIS_THREEEMPTY) == 0)

#define BLAST_TxSetSubcarrierWidth(base, v) \
                                          (base->TxRegs_p->TxSubcarrierWidth = v)
#define BLAST_TxGetSubcarrierWidth(base)  (base->TxRegs_p->TxSubcarrierWidth)
#endif

/* RC Rx interface */
#if defined(ST_OSLINUX)
#define BLAST_RxGetOnTime(base)           (STSYS_ReadRegDev32LE((void*)(&(base->RxRegs_p->RxOnTimeIr))) \
                                           & 0xFFFF)

#define BLAST_RxGetSymbolTime(base)       (STSYS_ReadRegDev32LE((void*) \
                                                                (&(base->RxRegs_p->RxSymbolTimeIr))) \
                                           & 0xFFFF)

#define BLAST_RxEnableInt(base)           STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxIntEnableIr)), \
                                              (STSYS_ReadRegDev32LE((void*) \
                                                 (&(base->RxRegs_p->RxIntEnableIr))) | RXIE_ENABLE))

#define BLAST_RxDisableInt(base)          STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxIntEnableIr)), \
                                              (STSYS_ReadRegDev32LE((void*) \
                                                 (&(base->RxRegs_p->RxIntEnableIr))) & ~RXIE_ENABLE))

#define BLAST_RxSetIE(base, v)            STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxIntEnableIr)), \
                                              (STSYS_ReadRegDev32LE((void*) \
                                                 (&(base->RxRegs_p->RxIntEnableIr))) | v))

#define BLAST_RxGetIE(base)               (STSYS_ReadRegDev32LE((void*)(&(base->RxRegs_p->RxIntEnableIr))))

#define BLAST_RxEnable(base)              STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxEnableIr)), 1)

#define BLAST_RxDisable(base)             STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxEnableIr)), 0)

#define BLAST_RxIsEnabled(base)           (STSYS_ReadRegDev32LE((void*)(&(base->RxRegs_p->RxIntEnableIr))))

#ifdef NEW_INTERRUPT_MECHANISM
#define BLAST_RxGetStatus(base)           (STSYS_ReadRegDev32LE((void*)(&(base->RxRegs_p->RxStatusIr))))
#else
#define BLAST_RxGetStatus(base)           (STSYS_ReadRegDev32LE((void*)(&(base->RxRegs_p->RxIntStatusIr))))
#endif
#else /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */
#define BLAST_RxGetOnTime(base)           (base->RxRegs_p->RxOnTimeIr & 0xFFFF)
#define BLAST_RxGetSymbolTime(base)       (base->RxRegs_p->RxSymbolTimeIr & 0xFFFF)

#define BLAST_RxEnableInt(base)           base->RxRegs_p->RxIntEnableIr |= RXIE_ENABLE
#define BLAST_RxDisableInt(base)          base->RxRegs_p->RxIntEnableIr &= ~RXIE_ENABLE
#define BLAST_RxSetIE(base, v)            base->RxRegs_p->RxIntEnableIr |= v
#define BLAST_RxGetIE(base)               (base->RxRegs_p->RxIntEnableIr)

#define BLAST_RxEnable(base)              base->RxRegs_p->RxEnableIr = 1
#define BLAST_RxDisable(base)             base->RxRegs_p->RxEnableIr = 0
#define BLAST_RxIsEnabled(base)           (base->RxRegs_p->RxEnableIr)

#ifdef NEW_INTERRUPT_MECHANISM
#define BLAST_RxGetStatus(base)           (base->RxRegs_p->RxStatusIr)
#else
#define BLAST_RxGetStatus(base)           (base->RxRegs_p->RxIntStatusIr)
#endif
#endif /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */


/* Bits 4:5 both 0 == RX is empty */
#define BLAST_RxIsEmpty(value)            ((value & 0x30) == 0)
#define BLAST_RxIsFull(value)             ((value & RXIS_THREEFULL) == RXIS_THREEFULL)

#if defined(ST_OSLINUX)
#define BLAST_RxSetMaxSymTime(base, v)    STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxMaxSymbolTimeIr)), \
                                                                (v & 0xFFFF))
#define BLAST_RxGetMaxSymTime(base)       STSYS_ReadRegDev32LE((void*) \
                                                               (&(base->RxRegs_p->RxMaxSymbolTimeIr)))
#else
#define BLAST_RxSetMaxSymTime(base, v)    base->RxRegs_p->RxMaxSymbolTimeIr = (v & 0xFFFF)
#define BLAST_RxGetMaxSymTime(base)       (base->RxRegs_p->RxMaxSymbolTimeIr)
#endif

#define BLAST_IsOverrun(value)            ((value & RXIS_OVERRUN) != 0)
#if defined(ST_OSLINUX)
#define BLAST_ClrOverrun(base)            STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxClrOverrunIr)), 4)

#ifdef NEW_INTERRUPT_MECHANISM
#define BLAST_ClrONEFULL(base)        STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxClrOverrunIr)), \
                                                                 ((RXIS_ONEFULL) | RXIS_ENABLE))
#endif

#ifdef NEW_INTERRUPT_MECHANISM
#define BLAST_RxSetNoiseSuppress(base, v) \
                                         STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxNoiseSuppressWidthIr)),\
                                                               (v & 0xFFFF))
#else
#define BLAST_RxSetNoiseSuppress(base, v) \
                                         STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxNoiseSuppressWidthIr)),\
                                                               (v & 0x00FF))
#endif

#define BLAST_RxGetNoiseSuppress(base)    (STSYS_ReadRegDev32LE((void*) \
                                                                (&(base->RxRegs_p->RxNoiseSuppressWidthIr)))

#define BLAST_RxEnableIrdaMode(base)      STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxIrdaControl)), 1)

#define BLAST_RxDisableIrdaMode(base)     STSYS_WriteRegDev32LE((&(base->RxRegs_p->RxIrdaControl)), 0)

#define BLAST_RxGetIrdaMode(base)         (STSYS_ReadRegDev32LE((void*)(&(base->RxRegs_p->RxIrdaControl))))

#else /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */
#define BLAST_ClrOverrun(base)            base->RxRegs_p->RxClrOverrunIr = 1

#ifdef NEW_INTERRUPT_MECHANISM
    #define BLAST_ClrONEFULL(base)           base->RxRegs_p->RxClrOverrunIr = (RXIS_ONEFULL) | RXIS_ENABLE
#endif

#ifdef NEW_INTERRUPT_MECHANISM
#define BLAST_RxSetNoiseSuppress(base, v) base->RxRegs_p->RxNoiseSuppressWidthIr = (v & 0xFFFF)
#else
#define BLAST_RxSetNoiseSuppress(base, v) base->RxRegs_p->RxNoiseSuppressWidthIr = (v & 0x00FF)
#endif

#define BLAST_RxGetNoiseSuppress(base)    (base->RxRegs_p->RxNoiseSuppressWidthIr)

#define BLAST_RxEnableIrdaMode(base)      base->RxRegs_p->RxIrdaControl = 1
#define BLAST_RxDisableIrdaMode(base)     base->RxRegs_p->RxIrdaControl = 0
#define BLAST_RxGetIrdaMode(base)         (base->RxRegs_p->RxIrdaControl)
#endif /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */

/* Common to both UHF and IR Rx. Yes, it is only 4 bits. */
#if defined(ST_OSLINUX)
#define BLAST_SetSamplingRate(base, v)    STSYS_WriteRegDev32LE(base->RxSamplingRateCommon, v & 0x000f)
#else
#define BLAST_SetSamplingRate(base, v)    *base->RxSamplingRateCommon = (v & 0x000f)
#endif


/* IrDA interface registers (not currently used) */
#if defined(ST_OSLINUX)
#define BLAST_SetBaudRateGenIrda(base, v)   STSYS_WriteRegDev32LE((base->IrdaRegs->BaudRateGenIrda), \
                                                                  (v & 0xFFFF))

#define BLAST_GetBaudRateGenIrda(base)      (STSYS_ReadRegDev32LE((void*) \
                                                                  (&(base->IrdaRegs->BaudRateGenIrda))) \
                                             & 0xFFFF)

#define BLAST_TxEnableIrda(base)            STSYS_WriteRegDev32LE((&(base->IrdaRegs->TxEnableIrda)), 1)

#define BLAST_TxDisableIrda(base)           STSYS_WriteRegDev32LE((&(base->IrdaRegs->TxEnableIrda)), 0)

#define BLAST_RxEnableIrda(base)            STSYS_WriteRegDev32LE((&(base->IrdaRegs->RxEnableIrda)), 1)

#define BLAST_RxDisableIrda(base)           STSYS_WriteRegDev32LE((&(base->IrdaRegs->RxEnableIrda)), 0)

#define BLAST_IrdaASCMode(base)             STSYS_WriteRegDev32LE((&(base->IrdaRegs->IrdaAscControl)), 1)

#define BLAST_IrdaIrdaMode(base)            STSYS_WriteRegDev32LE((&(base->IrdaRegs->IrdaAscControl)), 0)

#define BLAST_RxGetPulseStatusIrda(base)    (STSYS_ReadRegDev32LE((void*) \
                                                                  (&(base->IrdaRegs->IrdaPulseStatus))))

#define BLAST_RxSetSamplingRateIrda(base, v) \
                                            STSYS_WriteRegDev32LE((&(base->IrdaRegs->RxSamplingRateIrda)), \
                                                                  (v & 0x00FF))

#define BLAST_RxSetMaxSymTimeIrda(base, v)  STSYS_WriteRegDev32LE((&(base->IrdaRegs->RxMaxSymbolTimeIrda)),\
                                                                  (v & 0xFFFF))

#else
#define BLAST_SetBaudRateGenIrda(base, v)   base->IrdaRegs->BaudRateGenIrda = (base & 0xFFFF) /* ??? */
#define BLAST_GetBaudRateGenIrda(base)      (base->IrdaRegs->BaudRateGenIrda & 0xFFFF)

#define BLAST_TxEnableIrda(base)            base->IrdaRegs->TxEnableIrda = 1
#define BLAST_TxDisableIrda(base)           base->IrdaRegs->TxEnableIrda = 0
#define BLAST_RxEnableIrda(base)            base->IrdaRegs->RxEnableIrda = 1
#define BLAST_RxDisableIrda(base)           base->IrdaRegs->RxEnableIrda = 0

#define BLAST_IrdaASCMode(base)             base->IrdaRegs->IrdaAscControl = 1
#define BLAST_IrdaIrdaMode(base)            base->IrdaRegs->IrdaAscControl = 0

#define BLAST_RxGetPulseStatusIrda(base)    (base->IrdaRegs->IrdaPulseStatus)
#define BLAST_RxSetSamplingRateIrda(base, v) \
                                            base->IrdaRegs->RxSamplingRateIrda = (v & 0x00FF)
#define BLAST_RxSetMaxSymTimeIrda(base, v)  base->IrdaRegs->RxMaxSymbolTimeIrda = (v & 0xFFFF)
#endif

/* Polarity Invert Rx. Yes it is only 1 bit. */
#if defined(ST_OSLINUX)
#define BLAST_RxPolarityInvert(base)    STSYS_WriteRegDev32LE((base->RxPolarityInvert), (1 & 0x0001))
#else
#define BLAST_RxPolarityInvert(base)    *base->RxPolarityInvert = (1 & 0x0001)
#endif

#endif
