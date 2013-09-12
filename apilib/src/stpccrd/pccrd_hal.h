/****************************************************************************
File Name   : pccrd_hal.h

Description : Header file containing the private functions prototypes,
              definition of strucutres and constants for internal use

History     :

Copyright (C) 2005 STMicroelectronics

Reference  : ST API Definition "STPCCRD Driver API" STB-API-332
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __PCCRD_HAL_H
#define __PCCRD_HAL_H

#include "stos.h"                       /* Includes compat.h for LINUX */

#if defined( ST_OSLINUX )
#include <linux/i2c.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

/*Macros*********************************************************************/
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    /* To check if the STEVT is used or not */
    #define PCCRD_UsingEventHandler(ControlBlock_p) \
            ((ControlBlock_p->EVT_Handle != 0) ? TRUE : FALSE)
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

/*Constants******************************************************************/

/* IO registers Offset */
#define PCCRD_DATA                          0x0     /* Data register */

#ifdef ST_OSLINUX
#define STPCCRD_EPLD_MAPPING_WIDTH          0x28
#define STPCCRD_DVBCI_MAPPING_WIDTH         0x1c0000
#define STPCCRD_EMICFG_MAPPING_WIDTH        0x18
#endif


#if defined(ST_7100) || defined(ST_7109)
#define PCCRD_CMDSTATUS                     0x4     /* Command/Status register */
#define PCCRD_SIZE_LS                       0x8     /* Size register (LS) */
#define PCCRD_SIZE_MS                       0xC     /* Size register (MS) */
#else
#define PCCRD_CMDSTATUS                     0x1     /* Command/Status register */
#define PCCRD_SIZE_LS                       0x2     /* Size register (LS) */
#define PCCRD_SIZE_MS                       0x3     /* Size register (MS) */
#endif

/* Status register bits */
#define PCCRD_RE                            0x01    /* Read error */
#define PCCRD_WE                            0x02    /* Write error */
#define PCCRD_FR                            0x40    /* Free */
#define PCCRD_DA                            0x80    /* Data available */

/* Command register bits */
#define PCCRD_SETHC                         0x01    /* Host control bit set */
#define PCCRD_SETSW                         0x02    /* Size write set */
#define PCCRD_SETSR                         0x04    /* Size read set  */
#define PCCRD_SETRS                         0x08    /* Interface reset */
#define PCCRD_RESETHC                       0x00    /* Reset Host Control Bit */
#define PCCRD_RESET_COMMAND                 0x00    /* Command Register reset */

/* EN50221 Specific Constants */
#define STCE_EV                             "DVBCI_HOST"
#define STCE_PD                             "DVB_CI_MODULE"
#define STCF_LIB                            "DVB_CI_V1.00"
#define TPCE_IO                             0x22
#define TPCE_IF                             0x04
#define MAX_COR_BASE_ADDRESS                0xFFE
#define STCI_IFN_HIGH                       0x02
#define STCI_IFN_LOW                        0x41
#define IND_TPLLV1_MINOR                    0x01
#define IND_TPLLV1_MAJOR                    0x00
#define VAL_TPLLV1_MINOR                    0x00
#define VAL_TPLLV1_MAJOR                    0x05
#define SZ_TPLLMANFID                       0x04
#define IND_TPLMID_MANF_LSB                 0x00
#define IND_TPLMID_MANF_MSB                 0x01
#define IND_TPLMID_CARD_LSB                 0x02
#define IND_TPLMID_CARD_MSB                 0x03

/* PC Card Vol 2 related Constants */
#define CISTPL_DEVICE                       0x01     /* Tupple Tags */
#define CISTPL_NO_LINK                      0x14
#define CISTPL_VERS_1                       0x15
#define CISTPL_17                           0x17
#define CISTPL_CONFIG                       0x1a
#define CISTPL_CFTABLE_ENTRY                0x1b
#define CISTPL_DEVICE_OC                    0x1c
#define CISTPL_DEVICE_OA                    0x1d
#define CISTPL_MANFID                       0x20
#define CISTPL_FUNCID                       0x21
#define CISTPL_END                          0xff

#define NOT_READY_WAIT                      0x1F     /* Represent neither WAIT# nor READY# asserted */
#define WAIT_SCALE_0                        0x1C     /* WAIT_SCALE allowable value of WAIT# assertion */
#define WAIT_SCALE_1                        0x1D     /* depending upon possible settings of TPCE_TD(0-1) bits*/
#define WAIT_SCALE_2                        0x1E

#define READY_SCALE_0                       0x03     /* READY_SCALE allowable value of READY# assertion */
#define READY_SCALE_1                       0x07     /* depending upon possible settings of TPCE_TD(2-4) bits*/
#define READY_SCALE_2                       0x0B
#define READY_SCALE_3                       0x0F
#define READY_SCALE_4                       0x13
#define READY_SCALE_5                       0x17
#define READY_SCALE_6                       0x1b

#define TPCC_RADR_1_BYTE                    0x0      /* TPCC_RADR is encoded in one byte */
#define TPCC_RADR_2_BYTE                    0x1      /* TPCC_RADR is encoded in two byte */
#define TPCC_RADR_3_BYTE                    0x2      /* TPCC_RADR is encoded in thre byte */
#define TPCC_RADR_4_BYTE                    0x3      /* TPCC_RADR is encoded in four byte */

#define MASK_TPCC_RFSZ                      0xC0     /* Masking Bit Pattern for TPCC_RFSZ in TPCC_SZ */
#define MASK_TPCC_RMSZ                      0x3C     /* Masking Bit Pattern for TPCC_RMSZ in TPCC_SZ */
#define MASK_TPCC_RASZ                      0x03     /* Masking Bit Pattern for TPCC_RASZ in TPCC_SZ */

#define MASK_IO_ADDLINES                    0x1f     /* Masking Bit Pattern for IO Tupple */
#define MASK_TPCE_INDX                      0x3F     /* Masking Bit Pattern for Config Index */
#define MASK_RW_SCALE                       0x1F     /* Masking Bit Pattern for Read Write Timing Tupple */

/* Private Constants */
#define PCCRD_TASK_STACK_SIZE               1024     /* Stack size */
#define I2C_BUS_RD_TIMEOUT                  100      /* read access timeout in ms units */
#define I2C_BUS_WR_TIMEOUT                  100      /* write access timeout in ms units */
#define PCCRD_MAX_CI_WRITE_RETRY            0xff     /* Number of retries while writing the last byte */
#define PCCRD_MAX_CI_RESET_RETRY            4500     /* Number of retries while doing CI Reset(This is H/w work around) */
#define PCCRD_MAX_CHECK_CIS_RETRY           4500     /* Number of retries while checking CIS(This is H/w work around) */
#define PCCRD_TIME_DELAY                    1        /* Value for the delay in some operation  */
#define PCCRD_CARD_A_INSERTED               0x0180   /* Status of CD# pins when Card A is inserted */
#define PCCRD_CARD_B_INSERTED               0x0030   /* Status of CD# pins when Card B is inserted */

#if defined(ST_7100) || defined(ST_7109)
#define EVEN_ADDRESS_SHIFT                  0x03     /* For accessing the Even Address in Attribute Memory Space - A2 = A0 on */
#else
#define EVEN_ADDRESS_SHIFT                  0x01     /* For accessing the Even Address in Attribute Memory Space */
#endif

#define MAX_TUPPLE_BUFFER                   0xff     /* Maximum Tupple Buffer Size */
#define UPPER_BYTE                          0xff00   /* To extract upper byte off 16 bit data */
#define LOWER_BYTE                          0x00ff   /* To extract lower byte off 16 bit data */
#define SHIFT_EIGHT_BIT                     0x08     /* Shift the value left by 8 bits */
#define BIT_VALUE_HIGH                      0x01     /* Showing bit is High */
#define BIT_VALUE_LOW                       0x00     /* Showing bit is Low */
#define MASK_0_BIT                          0x01     /* Masking bit 0 of byte */
#define MASK_1_BIT                          0x02     /* Masking bit 1 of byte */
#define MASK_2_BIT                          0x04     /* Masking bit 2 of byte */
#define MASK_3_BIT                          0x08     /* Masking bit 3 of byte */
#define MASK_4_BIT                          0x10     /* Masking bit 4 of byte */
#define MASK_5_BIT                          0x20     /* Masking bit 5 of byte */
#define MASK_6_BIT                          0x40     /* Masking bit 6 of byte */
#define MASK_7_BIT                          0x80     /* Masking bit 7 of byte */

#define STATUS_CIS_CHECKED                  0x00001111  /* To check the presence of all required Tupple */
#define STATUS_CIS_MANFID_CHECKED           0x00000010  /* To check the presence of MANFID Tupple */
#define STATUS_CIS_VERS_1_CHECKED           0x00000001  /* To check the presence of VERS Tupple */
#define STATUS_CIS_CONFIG_CHECKED           0x00000100  /* To check the presence of CONFIG Tupple */
#define STATUS_CIS_CFTENTRY_CHECKED         0x00001000  /* To check the presence of CFENTRY Tupple */

#define I2C_DATA_LENGTH                     0x02        /* Length of the data to be transfered over I2C Bus */


/* Constants specific to ST5516 and ST5517 boards */
#if defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)

/* I2CPIO Pins to control the slots for functions
   like Reset , Card Detect , TSIN, Power Setting  */


#if defined(ST_7100) || defined(ST_7109)

/* Reset Pins for Module A or B */
/* These are a values in a Register of the EPLD */
#define DVB_CI_RESET_B                      0x20
#define DVB_CI_RESET_A                      0x02

/* Card Detect Pins for Module A or B */
/* These are a values in a Register of the EPLD */
#define DVB_CI_CD1_A                        0x04
#define DVB_CI_CD2_A                        0x08
#define DVB_CI_CD1_B                        0x40
#define DVB_CI_CD2_B                        0x80

/* Power Pins for Module A or B */
/* These are a values in a Register of the EPLD */
#define DVB_CI_VCC5V_NOTVCC3V_A             0x01
#define DVB_CI_VCC3V_NOTVCC5V_A             0x02
#define DVB_CI_VCC5V_NOTVCC3V_B             0x10
#define DVB_CI_VCC3V_NOTVCC5V_B             0x20

/* Initial Pins status for DVB CI Functioning */
#define DVB_CI_ENABLE                       0xfffb

#define NOTSTRB_BUF_SIG                      0x1  /*CardCmd_reg(0)*/
#define NOTADDROE_SIG                        0x2  /*CardCmd_reg(1)*/
#define NOTDATAOE_SIG                        0x4  /*CardCmd_reg(2)*/
#define PDDIR_SIG                            0x8  /*CardCmd_reg(3)*/
#define NOTPDOE_SIG                          0x10 /*CardCmd_reg(4)*/

#else   /* 5516/5517*/

/* Reset Pins for Module A or B */
#define DVB_CI_RESET_B                      0x0008
#define DVB_CI_RESET_A                      0x0040

/* Card Detect Pins for Module A or B */
#define DVB_CI_CD1_B                        0x0010
#define DVB_CI_CD2_B                        0x0020
#define DVB_CI_CD1_A                        0x0080
#define DVB_CI_CD2_A                        0x0100

/* Power Pins for Module A or B */
#define DVB_CI_VCC5V_NOTVCC3V_A             0x1000
#define DVB_CI_VCC5V_NOTVCC3V_B             0x2000

/* Initial Pins status for DVB CI Functioning */
#define DVB_CI_ENABLE                       0xffb0

#endif

/* Pins to control TS flow for Module A or B */
#define DVB_CI_NO_CARD                      0xffff

#define DVB_CI_NO_CARD_MASK                 0x0e00
#define DVB_CI_SLOT_A_ONLY                  0xffff
#define DVB_CI_SLOT_A_ONLY_MASK             0x0e00
#define DVB_CI_SLOT_B_ONLY                  0xf9ff
#define DVB_CI_SLOT_B_ONLY_MASK             0x0800
#define DVB_CI_SLOT_A_AND_B                 0xf5ff
#define DVB_CI_SLOT_A_AND_B_MASK            0x0400

/* Enable all Card Detect Pins at logic high */
#define DVB_CI_CD_A_B                       0x01b0

/* I2C Interfaced Device specific constants*/
#define PCF8575_I2C_ADDESS                  0x40
#define I2C_ADDRESS_OFFSET                  0x00

#if defined(ST_7100) || defined(ST_7109)

/* EMI Configuration Register Values */
#define DVB_CI_B_OFFSET                     0x00

/* I2C Interfaced Device specific constants*/
#define PCF8575_I2C_ADDESS                  0x40
#define I2C_ADDRESS_OFFSET                  0x00

#if !defined(ST_OSLINUX)
/* IO and MEM Addresses Offset */
#define DVB_CI_A_REG_IO_BASE_ADDRESS        0x400000
#define DVB_CI_B_REG_IO_BASE_ADDRESS        0x480000
#define DVB_CI_A_REG_MEM_BASE_ADDRESS       0x500000
#define DVB_CI_B_REG_MEM_BASE_ADDRESS       0x580000
#define DVB_CI_A_IO_BASE_ADDRESS            0x440000
#define DVB_CI_B_IO_BASE_ADDRESS            0x4C0000
#define DVB_CI_A_MEM_BASE_ADDRESS           0x540000
#define DVB_CI_B_MEM_BASE_ADDRESS           0x5C0000

#else

#define DVB_CI_A_REG_IO_BASE_ADDRESS        0x000000
#define DVB_CI_B_REG_IO_BASE_ADDRESS        0x080000
#define DVB_CI_A_REG_MEM_BASE_ADDRESS       0x100000
#define DVB_CI_B_REG_MEM_BASE_ADDRESS       0x180000
#define DVB_CI_A_IO_BASE_ADDRESS            0x040000
#define DVB_CI_B_IO_BASE_ADDRESS            0x0C0000
#define DVB_CI_A_MEM_BASE_ADDRESS           0x140000
#define DVB_CI_B_MEM_BASE_ADDRESS           0x1C0000

#endif/*ST_OSLINUX*/

#else

/* IO and MEM Addresses Offset */
#define DVB_CI_A_REG_IO_BASE_ADDRESS        0x100000
#define DVB_CI_B_REG_IO_BASE_ADDRESS        0x120000
#define DVB_CI_A_REG_MEM_BASE_ADDRESS       0x108000
#define DVB_CI_B_REG_MEM_BASE_ADDRESS       0x128000
#define DVB_CI_B_OFFSET                     0x0

#endif/*7100,7109*/

/* EMI Configuration Register Offset */
#define EMI_CONFIGDATA0                     0x00000000
#define EMI_CONFIGDATA1                     0x00000008
#define EMI_CONFIGDATA2                     0x00000010
#define EMI_CONFIGDATA3                     0x00000018

/* EMI Configuration Register Values */
#define EMI_CONFIG_REG0_VALUE               0x003607f9
#define EMI_CONFIG_REG1_VALUE               0xf066ca44
#define EMI_CONFIG_REG2_VALUE               0xf066ca44
#define EMI_CONFIG_REG3_VALUE               0x0000000a


/* CONTROL E Register Offset */
#define CONFIG_CONTROL_REG_E                0x00000028

#endif/*ST_5516 or ST_5517*/


#if defined(ST_5100) || defined(ST_5301) || defined(ST_7710)

/* Card Detect Pins for Module A or B */
#define DVB_CI_CD1_A                        0x0010
#define DVB_CI_CD2_A                        0x0020
#define DVB_CI_CD1_B                        0x0040
#define DVB_CI_CD2_B                        0x0080

/* Power Pins for Module A or B */
#define DVB_CI_VCC5V_NOTVCC3V_A             0x04
#define DVB_CI_VCC5V_NOTVCC3V_B             0x08
#define DVB_CI_VCC3V_NOTVCC5V_A             0x01
#define DVB_CI_VCC3V_NOTVCC5V_B             0x02

/* Pins to control TS flow for Module A or B */
#define DVB_CI_NO_CARD                      0xFFFF

#define DVB_CI_NO_CARD_MASK                 0x0E00
#define DVB_CI_SLOT_A_ONLY                  0xffff
#define DVB_CI_SLOT_A_ONLY_MASK             0x0e00
#define DVB_CI_SLOT_B_ONLY                  0xf9ff
#define DVB_CI_SLOT_B_ONLY_MASK             0x0800
#define DVB_CI_SLOT_A_AND_B                 0xf5ff
#define DVB_CI_SLOT_A_AND_B_MASK            0x0400


/* Enable all Card Detect Pins at logic high */
#define DVB_CI_CD_A_B                       0x01b0

/* IO and MEM Addresses Offset */
#define DVB_CI_A_REG_IO_BASE_ADDRESS        0x00000
#define DVB_CI_B_REG_IO_BASE_ADDRESS        0x00000
#define DVB_CI_A_REG_MEM_BASE_ADDRESS       0x08000
#define DVB_CI_B_REG_MEM_BASE_ADDRESS       0x08000


/* EMI Configuration Register Offset */
#define EMI_CONFIGDATA0                     0x00000000
#define EMI_CONFIGDATA1                     0x00000008
#define EMI_CONFIGDATA2                     0x00000010
#define EMI_CONFIGDATA3                     0x00000018


#if defined(ST_7710)
/* EMI Configuration Register Values */
#define BANKS_ENABLED                       0x20102860
#define EPLD_DVBCI_RESET_REG                0x41B00000
#define EPLD_EPM3256A_ADDR                  0x41900000
#define EPLD_EPM3256B_ADDR                  0x41900000
#define EPLD_DVBCI_ENABLE                   0x41A00000
#define EPLD_DVBCI_TSMUX	                0x41800000

#elif defined(ST_5100)

/* EMI Configuration Register Values */
#define EPLD_DVBCI_RESET_REG                0x41B00000
#define EPLD_DVBCI_TSMUX	                0x41800000
#define EPLD_DVBCI_ENABLE                   0x41A00000
#define BANKS_ENABLED                       0x202FF860
#define FMI_CONFIGDATA0_BANK_1              0x20200140
#define FMI_CONFIGDATA1_BANK_1              0x20200148
#define FMI_CONFIGDATA2_BANK_1              0x20200150
#define FMI_CONFIGDATA3_BANK_1              0x20200158
#define EPLD_EPM3256A_ADDR                  0x41900000
#define EPLD_EPM3256B_ADDR                  0x41900000
#define DVB_CI_B_OFFSET                     0x100000
#endif

#endif /* 7710, 7100 ,5100,5105 */

#if defined(ST_5105) || defined(ST_5107)

/* Card Detect Pins for Module A */
#define DVB_CI_CD1_A                        0x04
#define DVB_CI_CD2_A                        0x08

/* Power Pins for Module A or B */
#define DVB_CI_VCC5V_NOTVCC3V_A             0x01
#define DVB_CI_VCC3V_NOTVCC5V_A             0x02

/* Pins to control TS flow for Module A or B */
#define DVB_CI_NO_CARD                      0xFF

/* Enable all Card Detect Pins at logic high */
#define DVB_CI_CD_A_B                       0x0C

/* IO and MEM Addresses Offset */
#define DVB_CI_A_REG_IO_BASE_ADDRESS        0x40000
#define DVB_CI_B_REG_IO_BASE_ADDRESS        0x40000
#define DVB_CI_A_REG_MEM_BASE_ADDRESS       0x48000
#define DVB_CI_B_REG_MEM_BASE_ADDRESS       0x48000

/* EMI Configuration Register Values */
#define EPLD_DVBCI_RESET_REG                0x45800000
#define EPLD_DVBCI_CS                       0x45780000
#define EPLD_DVBCI_STEMCS                   0x45700000
#define EPLD_DVBCI_TSMUX	                0x45580000  /* Read/Write TsMuxAddr TSMUX control */
#define EPLD_MEMREQ_MDVBCI_IOWR             0x45480000
#define BANKS_ENABLED                       0x202FF860
#define FMI_CONFIGDATA0_BANK_1              0x20200140
#define FMI_CONFIGDATA1_BANK_1              0x20200148
#define FMI_CONFIGDATA2_BANK_1              0x20200150
#define FMI_CONFIGDATA3_BANK_1              0x20200158
#define EPLD_EPM3256A_ADDR                  0x45400000
#define EPLD_EPM3256B_ADDR                  0x45400000
#define DVB_CI_B_OFFSET                     0x0

/* CONTROL Register Offset */
#define CONFIG_CONTROL_C                    0x20402000
#define CONFIG_MONITOR_G                    0x20402030
#define CONFIG_CONTROL_G                    0x20402010
#endif


#if defined(ST_5514) || defined(ST_5518)

/* Using Bank 2 on 5514 for DVB CI - PC-Card */
#define BANK_BASE_ADDRESS                   0x60000000 /* DVB CI Bank Base Address */
#define DVB_CI_B_OFFSET                     0x0

#elif defined(ST_5528)

#define DVB_CI_B_OFFSET                     0x0
#define BANK_BASE_ADDRESS                   0x42000000 /* DVB CI Bank Base Address */
#define DVB_CI_BANK_ADDR_OFFSET             0x180
#define EMI_GENCFG                          0x1A100028

/* EMI Configuration Register Offset */
#define EMI_CONFIGDATA0                     0x00000000
#define EMI_CONFIGDATA1                     0x00000008
#define EMI_CONFIGDATA2                     0x00000010
#define EMI_CONFIGDATA3                     0x00000018

#elif defined(ST_7100) || defined(ST_7109)

/* Using Bank 2 on 7100 for DVB CI - PC-Card */
#define BANK_BASE_ADDRESS                   0xA2000000 /* DVB CI Bank Base Address */
#define DVB_CI_BANK_ADDR_OFFSET             0x0180    /*EMI CFG registers offfset from EMI_BASE_ADDRESS*/

#if !defined(ST_OSLINUX)
/* These are the Registers of the EPLD */
#define REVID_REG          (BANK_BASE_ADDRESS |  0x100000)
#define POD_REG            (BANK_BASE_ADDRESS |  0x100004)
#define INTR_REG           (BANK_BASE_ADDRESS |  0x100008)
#define TEST_REG           (BANK_BASE_ADDRESS |  0x10000C)
#define LED_REG            (BANK_BASE_ADDRESS |  0x100010)
#define DEVID_REG          (BANK_BASE_ADDRESS |  0x10001C)
#define CARD_REG           (BANK_BASE_ADDRESS |  0x100020)
#define CARD_CMD_REG       (BANK_BASE_ADDRESS |  0x100024)
#define CARD_PWR_REG       (BANK_BASE_ADDRESS |  0x100028)

#else
#define EPLD_BANK_BASE_ADDRESS  0xA2100000 /* DVB CI Bank Base Address */
#define REVID_REG          0x00
#define POD_REG            0x04
#define INTR_REG           0x08
#define TEST_REG           0x0C
#define LED_REG            0x10
#define DEVID_REG          0x1C
#define CARD_REG           0x20
#define CARD_CMD_REG       0x24
#define CARD_PWR_REG       0x28

#endif/*ST_OSLINUX*/

#elif defined(ST_7710)

/* Using Bank 3/4 on 7710 for DVB CI - PC-Card */
#define BANK_BASE_ADDRESS                   0x44000000  /* DVB CI Bank Base Address */
#define EMI_GENCFG                          0x20102028
#if defined(STPCCRD_USE_ONE_MODULE)
#define DVB_CI_B_OFFSET                     0x000000
#else
#define DVB_CI_B_OFFSET                     0x800000
#endif
#define DVB_CI_BANK_ADDR_OFFSET             0x200

#elif defined(ST_5100)

#define BANK_BASE_ADDRESS                   0x41E00000 /* DVB CI Bank Base Address */
#define FMI_GENCFG                          0x20200028

#elif defined(ST_5105) || defined(ST_5107)

#define BANK_BASE_ADDRESS                   0x41000000 /* DVB CI Bank Base Address */
#define FMI_GENCFG                          0x20200028


#elif defined(ST_5516)

/* Using Bank 3 on 5516 for DVB-CI */
#define BANK_BASE_ADDRESS                   0x70000000 /* DVB CI Bank Base Address */
#define DVB_CI_BANK_ADDR_OFFSET             0x000001C0 /* DVB CI Bank Address offset */
#define CONFIG_EMISS_PORTSIZEBANK_DVB_CI    0x00040000 /* Bit to Control Port Size */
#define CONFIG_EMISS_DVBCI_ENABLE           0x00020000 /* Bit to Enable or Disable Padlogic for DVB CI */

#elif defined(ST_5517)

#ifdef STPCCRD_SELECT_5517_EMI4
/* Using Bank 4 on 5517 for DVB-CI */
#define BANK_BASE_ADDRESS                   0x7f000000 /* DVB CI Bank (EMI Bank4) Base Address */
#define DVB_CI_BANK_ADDR_OFFSET             0x00000200 /* DVB CI Bank Address offset */
#define CONFIG_EMISS_PORTSIZEBANK_DVB_CI    0x00000000 /* Bit to Control Port Size */
#define CONFIG_EMISS_DVBCI_ENABLE           0x00100000 /* Bit to Enable or Disable Padlogic for DVB CI */

#else

/* Using Bank 3 on 5517 for DVB-CI */
#define BANK_BASE_ADDRESS                   0x70000000 /* DVB CI Bank Base Address */
#define DVB_CI_BANK_ADDR_OFFSET             0x000001C0 /* DVB CI Bank Address offset */
#define CONFIG_EMISS_PORTSIZEBANK_DVB_CI    0x00040000 /* Bit to Control Port Size */
#define CONFIG_EMISS_DVBCI_ENABLE           0x00020000 /* Bit to Enable or Disable Padlogic for DVB CI */
#endif /*STPCCRD_SELECT_5517_EMI4*/
#endif /* ST_5514 || ST_5518 */


#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#define FALLING_EDGE_TRIG_MODE              0x04
#define RISING_EDGE_TRIG_MODE               0x03
/* Constants related to the Interrupt Level Controller Register */
#if defined(ST_5516) || defined(ST_5517) || defined(ST_5514) || defined(ST_7100) || defined(ST_7710) \
|| defined(ST_5528) || defined(ST_7109)
#define ILC_OFFSET                          0x800
#define ILC_OFFSET_MULTIPLIER               0x08
#define ILC_REG_OFFSET                      0x04
#endif/* ST_5516 || ST_5517 || ST_5514 */
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */


/* Constants specific to ST5514 and ST5518 boards */
#if defined(ST_5514) || defined(ST_5518) || defined(ST_5528)

/* Address of the Registers in STV0700/701 */
#define MODA        0x00 /* MCR */
#define ASMHA       0x01
#define ASMLA       0x02
#define ASMPHA      0x03
#define ASMPLA      0x04
#define MACTA       0x05 /* MACTR */
#define IIMRA       0x06 /* IIMR */
#define MODB        0x09
#define ASMHB       0x0A
#define ASMLB       0x0B
#define ASMPHB      0x0C
#define ASMPLB      0x0D
#define MACTB       0x0E
#define IIMRB       0x0F
#define ASMHE       0x12
#define ASMLE       0x13
#define ASMPHE      0x14
#define ASMPLE      0x15
#define DSR         0x17 /* DSR */
#define PCR         0x18 /* PCR */
#define STA         0x19 /* STA */ /* on 701 */
#define INTSTA      0x1A /* ISR */
#define INTMSK      0x1B /* IMR */
#define INTCONF     0x1C /* ICR */
#define UCSG1       0x1D /* MICR */
#define UCSG2       0x1E /* MACR */
#define CICNTRL     0x1F /* CR */

/* Address of STV0700/701 Controller on I2C  bus */
#define STV0700_I2C_ADDESS_1                0x80
#define STV0700_I2C_ADDESS_2                0x82
#define STV0700_I2C_ADDESS_3                0x84
#define STV0700_I2C_ADDESS_4                0x86

/* STV0700/701 Controller specific register bits */
/* Bit Settings for Module A/B Control Registers */
#define STV0700_MOD_DET                     0x01
#define STV0700_MOD_RST                     0x80
#define STV0700_MOD_TS_BYPASS_DISABLE       0x60
#define STV0700_MOD_AUTO                    0x02
#define STV0700_MOD_IO_ACCESS               0x04
#define STV0700_MOD_RESERVED                0x0C
#define STV0700_MOD_COMMON_ACCESS           0x08

/* Bit Settings for STV0700 Control Register */
#define STV0700_CICNTRL_RESET               0x80
#define STV0700_CICNTRL_LOCK                0x01
#define STV0700_CICNTRL_OPHS                0x02

/* Bit Settings for Power Control Register   */
#define STV0700_CARD_POWER_ON               0x20  /* VCC=1 */
#define STV0701_PCR_ARSE                    0x01
#define STV0701_PCR_VPDRV                   0x10
#define STV0701_PCR_VCC                     0x20
#define STV0701_PCR_VCLVL                   0x40
#define STV0701_PCR_VCDRV                   0x80

/* Bit Setting for Interrupt Status/Mask Register */
#define STV0700_INTSTA_DETA                 0x01
#define STV0700_INTSTA_DETB                 0x02
#define STV0700_INTSTA_IRQA                 0x04
#define STV0700_INTSTA_IRQB                 0x08
#define STV0700_INTSTA_EXT                  0x10

/* Bit Settings for Destination Select Register */
#define STV0700_DSR_AUTOSEL                 0x01
#define STV0701_DSR_SEL0                    0x02
#define STV0700_DSR_SEL1                    0x04

/* Bit Settings for Interrupt Configuration Register */
#define STV0700_INTCONF_ITDRV               0x04
#define STV0700_INTCONF_ITLVL               0x02
#define STV0700_INTCONF_EXTLVL              0x01

/* Bit Settings for Universal Control Signal Generator 1 Register */
#define STV0700_UCSG1_CSLVL                 0x80

/* Bit Settings for Universal Control Signal Generator 2 Register */
#define STV0700_UCSG2_WDRV                  0x02
#define STV0700_UCSG2_WLVL                  0x01

/* Bit Settings for External Access Auto Select Mask High Register */
#define STV0700_ASMHE_EXTCS                 0x80

/* Bit Settings for Module A Auto Select Register */
#define STV0700_ASMHA_MA25                  0x04
#define STV0700_ASMHA_MA24                  0x02
#define STV0700_ASMHA_MA23                  0x01

/* Bit Settings for Module A Auto Select Mask Low Register */
#define STV0700_ASMLA_MA19                  0x10
#define STV0700_ASMLA_MA18                  0x08
#define STV0700_ASMLA_MA17                  0x04

/* Bit Settings for Module A Auto Select Pattern High Register */
#define STV0700_ASMPHA_PA25                 0x04
#define STV0700_ASMPHA_PA24                 0x02
#define STV0700_ASMPHA_PA23                 0x01

/* Bit Settings for Module A Auto Select Pattern Low Register */
#define STV0700_ASMPLA_PA17                 0x04

/* Bit Settings for Module B Auto Select Pattern Low Register */
#define STV0700_ASMPLB_PA18                 0x08

/* Bit Settings for Memory access cycle time module A Register */
#define STV0700_MACTA_AM2                   0x40
#define STV0700_MACTA_AM1                   0x20
#define STV0700_MACTA_AM0                   0x10
#define STV0700_MACTA_CM2                   0x04
#define STV0700_MACTA_CM1                   0x02
#define STV0700_MACTA_CM0                   0x01

/* IO and MEM Addresses Offset */
#define DVB_CI_A_REG_IO_BASE_ADDRESS        0x20000
#define DVB_CI_B_REG_IO_BASE_ADDRESS        0x40000
#define DVB_CI_A_REG_MEM_BASE_ADDRESS       0x20000
#define DVB_CI_B_REG_MEM_BASE_ADDRESS       0x40000

#endif /*defined(ST_5514) || defined(ST_5518)*/

/* Private Enums********************************************************************/

/* Enums to specify the Mode whether IO or Memory */
typedef enum
{
    PCCRD_IO_ACCESS,
    PCCRD_ATTRIB_ACCESS,
    PCCRD_COMMON_ACCESS
}PCCRD_AccessMode_t;

/* Enums to specify if DVB CI Module is present or absent */
enum {
    MOD_ABSENT,
    MOD_PRESENT
};

/* Enums to specify the condition for Transport Stream Interface  */
enum{
    NO_PCCARD,
    PCCARD_A_ONLY,
    PCCARD_B_ONLY,
    PCCARD_A_AND_B
};

/*Private Data Structures***********************************************/

/* Tupple Data Structure */
typedef struct {
    U8  TuppleTag;
    U8  TuppleLink;
    U8  TuppleData[MAX_TUPPLE_BUFFER];
}PCCRD_Tupple_t;

/* Open Block Structure */
typedef struct
{
    U32                         CommandBaseAddress;     /* Command Interface Base Address */
    U32                         AttributeBaseAddress ;  /* Attributes address */
    U32                         COR_Address;            /* Configuration registers base address                 */
    U8                          ConfigurationIndex;     /* Index value written to COR to enable configuration   */
    PCCRD_AccessMode_t          AccessState;            /* IO or Memory Access */
    BOOL                        DeviceOpen;             /* State of Device Open or Close */
    BOOL                        DeviceLock;             /* Lock or Free state of device */
    STPCCRD_Handle_t            Handle;                 /* Identifier to the Device */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    U8                          ModPresenceStatus ;     /* Presence or Absence or Module */
    void (* NotifyFunction)(STPCCRD_Handle_t Handle,    /* Callback Function Registered */
                            STPCCRD_EventType_t Event,
                            ST_ErrorCode_t ErrorCode);
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
}PCCRD_OpenBlock_t;

/* Control Block Structure */
struct PCCRD_ControlBlock_s
{
    struct PCCRD_ControlBlock_s    *Next_p;                     /* Pointer to next (else NULL) */
    U32                            BaseHandle;                  /* Shifted pointer to this structure */
    U32                            NumberOpen;                  /* Number of currently Open Handles */
    ST_DeviceName_t                DeviceName;                  /* Initialization device name */
#if !defined(ST_7710)  && !defined(ST_5100) && !defined(ST_5301)
#if !defined(ST_OSLINUX)
    STI2C_Handle_t                 I2C_Handle;                  /* returned on STI2C_Open() call */
#else
    struct i2c_msg                 I2C_Message;
    struct i2c_adapter             *Adapter_299;
      /* Linux ioremap address */
    unsigned long                 *MappingEPLDBaseAddress;
    unsigned long                  PccrdEPLDMappingWidth;
    unsigned long                 *MappingDVBCIBaseAddress;
    unsigned long                  PccrdDVBCIMappingWidth;
    unsigned long                 *MappingEMICFGBaseAddress;
    unsigned long                  PccrdEMICFGMappingWidth;

#endif/*ST_OSLINUX*/
#endif

    U32                            MaxHandle;                   /* Number of Handles given in Initparams */
    BOOL                           DeviceLock;                  /* State of the PCCCRD Device */
    PCCRD_OpenBlock_t              *OpenBlkPtr_p;               /* Pointer to OpenBlock */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifdef ST_OS21
    interrupt_name_t               InterruptNumber;
#else
    U32                            InterruptNumber;
#endif
    U32                            InterruptLevel;              /* Interrupt Level */
    U8                             TaskStack[PCCRD_TASK_STACK_SIZE];/* Task Stack */
#ifndef ST_OS21
    semaphore_t                    CardDetectSemaphore;         /*  for Plug In /Out Support */
    task_t                         task;                        /* Task Structure */
    tdesc_t                        tdesc;                       /* Task Descriptor Structure */
#else
    task_t                         *task;
#endif
    semaphore_t                    *CardDetectSemaphore_p;      /*  for Plug In /Out Support */
    BOOL                           WaitingToQuit;               /*State of the Notify Task */
    STEVT_Handle_t                 EVT_Handle;                  /* returned on STEVT_Open() call */
    STEVT_EventID_t                Mod_A_InsertedEvtId;         /* Module A Insert Event ID */
    STEVT_EventID_t                Mod_B_InsertedEvtId;         /* Module B Insert Event ID */
    STEVT_EventID_t                Mod_A_RemovedEvtId;          /* Module A Removed Event ID */
    STEVT_EventID_t                Mod_B_RemovedEvtId;          /* Module B Removed Event ID */
#endif
    BOOL                           MOD_A_TS_PROCESSING_ENABLED; /* State of TS Interface of Module A */
    BOOL                           MOD_B_TS_PROCESSING_ENABLED; /* State of TS Interface of Module B */
    ST_Partition_t                 *DriverPartition_p;          /* Driver Partition */
};

typedef struct PCCRD_ControlBlock_s PCCRD_ControlBlock_t;

/* Internal Functions Prototypes ***********************************************************************/
ST_ErrorCode_t PCCRD_HardInit( PCCRD_ControlBlock_t  *ControlBlkPtr_p);    /* Pointer to control block structure */

ST_ErrorCode_t PCCRD_ReadRegister( PCCRD_ControlBlock_t  *ControlBlkPtr_p, /* Pointer to control block structure */
                                   U32                   Address,          /* Address of the Device */
                                   U8*                   Buffer_p);        /* Pointer to buffer*/

ST_ErrorCode_t PCCRD_WriteRegister( PCCRD_ControlBlock_t  *ControlBlkPtr_p,/* Pointer to control block structure */
                                    U32                   Address,         /* Address of the Device */
                                    U16                   Data);           /* Data to be written */

ST_ErrorCode_t PCCRD_ReadTupple( PCCRD_OpenBlock_t      *OpenBlkPtr_p,    /* Pointer to open block structure */
                                 U16                    TuppleOffset,     /* Current offset in Attribute Memory Space*/
                                 PCCRD_Tupple_t         *Tupple_p);       /* Pointer to tupple structure*/

ST_ErrorCode_t PCCRD_ChangeAccessMode( PCCRD_ControlBlock_t    *ControlBlkPtr_p, /* Pointer to control block structure */
                                       U32                     OpenIndex,        /* Index of Open Block structure */
                                       PCCRD_AccessMode_t      AccessMode);      /* Access Mode */

ST_ErrorCode_t PCCRD_ControlTS( PCCRD_ControlBlock_t   *ControlBlkPtr_p,  /* Pointer to control block structure */
                                U8                     Option);           /* Specifying the condition for TS flow */


#ifdef __cplusplus
}
#endif
#endif /*__PCCRD_HAL_H*/

