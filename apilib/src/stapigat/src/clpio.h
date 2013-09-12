/*******************************************************************************

File name   : clpio.h

Description : PIO configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
31 Jan 2002        Created                                           HSdLM
26 Jun 2002        Change configuration for 5514/5516                HSdLM
11 Oct 2002        Add support for tuner and 5517                    BS/HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLPIO_H
#define __CLPIO_H


/* Includes ----------------------------------------------------------------- */

#ifdef ST_OSLINUX
#include "compat.h"
#endif

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#if defined(ST_5514) || defined (ST_5516) || defined (ST_5517)
#define NUMBER_PIO   6
#elif defined(ST_7200) || defined(ST_5528)
#define NUMBER_PIO   8
#elif defined(ST_5100) || defined(ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5525) || defined(ST_5301) || defined(ST_8010) /* to be verified for STi5105, 5107, 5301, 8010, 5525, 5188 */
#define NUMBER_PIO   4
#elif defined(ST_7710)
#define NUMBER_PIO   6
#elif defined(ST_7100) || defined(ST_7109)
#define NUMBER_PIO   6
#else
#define NUMBER_PIO   5
#endif

#define STPIO_0_DEVICE_NAME           "PIO0"
#define STPIO_1_DEVICE_NAME           "PIO1"
#define STPIO_2_DEVICE_NAME           "PIO2"
#define STPIO_3_DEVICE_NAME           "PIO3"
#define STPIO_4_DEVICE_NAME           "PIO4"
#define STPIO_5_DEVICE_NAME           "PIO5"
#define STPIO_6_DEVICE_NAME           "PIO6"
#define STPIO_7_DEVICE_NAME           "PIO7"



#if defined(ST_5510) || defined(ST_5512)
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM1_PORTNAME         ""
#define PIO_FOR_PWM1_BITMASK          0
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_5508) || defined(ST_5518)
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         ""
#define PIO_FOR_PWM0_BITMASK          0
#define PIO_FOR_PWM1_PORTNAME         ""
#define PIO_FOR_PWM1_BITMASK          0 /* PIO_BIT_4 */
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_5514) || defined(ST_5516) || defined (ST_5517) \
   || defined(ST_5528)
#define PIO_FOR_SDA_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_1
#define PIO_FOR_SDA2_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_SDA2_BITMASK          PIO_BIT_2
#define PIO_FOR_SCL2_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL2_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM0_PORTNAME         STPIO_2_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM1_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM2_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM2_BITMASK          PIO_BIT_7

#elif defined(ST_5100) || defined (ST_5105) || defined(ST_5107) || defined(ST_5188) || defined(ST_5525) || defined (ST_5301) || defined (ST_8010) /* to be verified for STi5105, 5107, 5301, 8010, 5525, 5188*/
#define PIO_FOR_SDA_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_1
#define PIO_FOR_SDA2_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_SDA2_BITMASK          PIO_BIT_2
#define PIO_FOR_SCL2_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL2_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM0_PORTNAME         STPIO_2_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_5
#define PIO_FOR_PWM1_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_2
#define PIO_FOR_PWM2_PORTNAME         STPIO_2_DEVICE_NAME
#define PIO_FOR_PWM2_BITMASK          PIO_BIT_7

#elif defined(ST_7710)
#define PIO_FOR_SDA_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_1
#define PIO_FOR_SCL_PORTNAME          STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_0
#define PIO_FOR_SDA2_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_SDA2_BITMASK          PIO_BIT_1
#define PIO_FOR_SCL2_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL2_BITMASK          PIO_BIT_0

#define PIO_FOR_PWM0_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM1_PORTNAME         STPIO_5_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_4
#define PIO_FOR_PWM2_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM2_BITMASK          PIO_BIT_5

#elif defined (ST_7100) || defined (ST_7109)
#define PIO_FOR_SDA_PORTNAME          STPIO_4_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_1
#define PIO_FOR_SCL_PORTNAME          STPIO_4_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_0
#define PIO_FOR_SDA2_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_SDA2_BITMASK          PIO_BIT_1
#define PIO_FOR_SCL2_PORTNAME         STPIO_3_DEVICE_NAME
#define PIO_FOR_SCL2_BITMASK          PIO_BIT_0

#if defined WA_GNBvd54182
#define PIO_FOR_PWM0_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_6
#define PIO_FOR_PWM1_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0
#else /*WA_GNBvd54182*/
#define PIO_FOR_PWM0_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM1_PORTNAME         STPIO_5_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_4
#define PIO_FOR_PWM2_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM2_BITMASK          PIO_BIT_5
#endif /*WA_GNBvd54182*/

#elif defined(ST_7200)
#define PIO_FOR_SDA_PORTNAME          STPIO_2_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_1
#define PIO_FOR_SCL_PORTNAME          STPIO_2_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_0
#define PIO_FOR_SDA2_PORTNAME         STPIO_7_DEVICE_NAME
#define PIO_FOR_SDA2_BITMASK          PIO_BIT_7
#define PIO_FOR_SCL2_PORTNAME         STPIO_7_DEVICE_NAME
#define PIO_FOR_SCL2_BITMASK          PIO_BIT_6

#define PIO_FOR_PWM0_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_6
#define PIO_FOR_PWM1_PORTNAME         STPIO_4_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_7
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_TP3)
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#define PIO_FOR_PWM0_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM0_BITMASK          PIO_BIT_3
#define PIO_FOR_PWM1_PORTNAME         STPIO_1_DEVICE_NAME
#define PIO_FOR_PWM1_BITMASK          PIO_BIT_4
#define PIO_FOR_PWM2_PORTNAME         ""
#define PIO_FOR_PWM2_BITMASK          0

#elif defined(ST_GX1)
#define PIO_FOR_SDA_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SDA_BITMASK           PIO_BIT_0
#define PIO_FOR_SCL_PORTNAME          STPIO_1_DEVICE_NAME
#define PIO_FOR_SCL_BITMASK           PIO_BIT_2
#endif

#if defined(SAT5107)
#define PIO_PnOUT                   0x00    /* R/W */
#define PIO_SET_PnOUT               0x04    /* W   */
#define PIO_CLEAR_PnOUT             0x08    /* W   */
#define PIO_PnIN                    0x10    /* R   */
#define PIO_PnC0                    0x20    /* R/W */
#define PIO_SET_PnC0                0x24    /* W   */
#define PIO_CLEAR_PnC0              0x28    /* W   */
#define PIO_PnC1                    0x30    /* R/W */
#define PIO_SET_PnC1                0x34    /* W   */
#define PIO_CLEAR_PnC1              0x38    /* W   */
#define PIO_PnC2                    0x40    /* R/W */
#define PIO_SET_PnC2                0x44    /* W   */
#define PIO_CLEAR_PnC2              0x48    /* W   */
#define PIO_PnCOMP                  0x50    /* R/W */
#define PIO_SET_PnCOMP              0x54    /* W   */
#define PIO_CLEAR_PnCOMP            0x58    /* W   */
#define PIO_PnMASK                  0x60    /* R/W */
#define PIO_SET_PnMASK              0x64    /* W   */
#define PIO_CLEAR_PnMASK            0x68    /* W   */

typedef enum
{
  UNKNOWN,
  BIRECTIONNAL,
  OUTPUT,
  INPUT,
  ALTERNATIVE_FUNCTION_OUTPUT,
  ALTERNATIVE_FUNCTION_BIRECTIONNAL
} PIOConfiguration_t;

typedef PIOConfiguration_t      PIOBitConfiguration_t[8];

BOOL    GetPIOBaseAddress(char PIONumber, int* PIOBaseAddress);
BOOL    SetPIOBitConfiguration(char PIONumber, char BitNumber, PIOConfiguration_t PIOBitConfiguration);
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

extern ST_DeviceName_t PioDeviceName[];

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

BOOL PIO_Init(void);
void PIO_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLPIO_H */

/* End of clpio.h */
