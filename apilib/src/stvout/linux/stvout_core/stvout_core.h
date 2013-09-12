/*****************************************************************************
 *
 *  Module      : stvout_core
 *  Date        : 23-10-2005
 *  Author      : ATROUSWA
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STVOUT_CORE_H
#define STVOUT_CORE_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */

#include "hdmi.h"
#if defined (STVOUT_HDCP_PROTECTION)
#include "hdcp.h"
#include "hdcp_reg.h"
#endif


/*** IOCtl defines ***/

#define STVOUT_CORE_TYPE   0XFF     /* Unique module id - See 'ioctl-number.txt' */

/* This mapping is for DENC, VTG, DVO, VOS and HDMI HDCP together if defined STVOUT_HDMI*/
#define STVOUT_MAPPING_WIDTH        0x1000
#define STVOUT_SYSCFG_WIDTH         0x400
#if defined(STVOUT_HDMI)
#define HDMI_MAPPING_WIDTH          0x400
#define HDCP_MAPPING_WIDTH          0x400
#endif /*STVOUT_HDMI*/

/*I2C Module */
/* Scanned client addresses : value of the first 7 bits only */
#define I2C_HDMI_5       0x00
#define I2C_HDMI_4       0x0b
#define I2C_HDMI_3       0x30
#define I2C_HDMI_2       0x3a
#define I2C_HDMI_1       0x50

/* HDMI I2C client addresses on 8 bits */
#define I2C_HDMI_LINUX_1  0xA0
#define I2C_HDMI_LINUX_2  0x74

/* COMMAND used  for hdmi transfer on I2C bus*/
enum hdmi_i2c__command {
        HDMI_I2C_GET_SINK_INFO                  = 0,
        HDMI_I2C_GET_SINK_INFO_BY_PAGE          = 1,
        HDMI_I2C_GET_CAPABILITIES               = 2,
        HDMI_I2C_SET_IRATE                      = 3,
        HDMI_I2C_SET_ENHANCED_COMPUTATION_MODE  = 4,
        HDMI_I2C_EESS_USED_BY_HDCP_TRANSMITTER  = 5,
        HDMI_RECEIVER_IS_HDCP_CAPABLE           = 6,
        HDMI_GET_KSV_RECEIVER_1_3               = 7,
        HDMI_GET_KSV_RECEIVER_4                 = 8,
        HDMI_GET_FIRST_BYTE_RI                  = 9,
        HDMI_GET_SECOND_BYTE_RI                 = 10,
        HDMI_I2C_SET_PORT_AN_1_3                = 11,
        HDMI_I2C_SET_PORT_AN_4_7                = 12,
        HDMI_I2C_SET_PORT_AKSV_1_3              = 13,
        HDMI_I2C_SET_PORT_AKSV_4                = 14,
        HDMI_I2C_GET_HDCP_PORT_BSTATUS          = 15,
        HDMI_I2C_GET_HDCP_PORT_BSTATUS_1        = 16,
        HDMI_I2C_GET_KSV_FIFO_REPEATER          = 17,
        HDMI_I2C_CHECK_V_0                      = 18,
        HDMI_I2C_CHECK_V_1                      = 19,
        HDMI_I2C_CHECK_V_2                      = 20,
        HDMI_I2C_CHECK_V_3                      = 21,
        HDMI_I2C_CHECK_V_4                      = 22,
};
typedef struct STVOUTMod_Param_s
{
    int           VoutBaseMappingNumber;  /* the number the base adress mapping is needed by other drivers */
    unsigned long VoutBaseAddress;        /* Vout Register base address to map */
    unsigned long VoutAddressWidth;       /* Vout address range */
    unsigned long SysCFGBaseAddress;      /* PLL & FS base address to map */
    unsigned long SysCFGAddressWidth;     /* PLL & FS address range */
#if defined(STVOUT_HDMI)
    unsigned long HdmiBaseAddress;        /* Hdmi Register base address to map */
    unsigned long HdmiAddressWidth;       /* Hdmi address range */
    unsigned long HdcpBaseAddress;        /* Hdmi Register base address to map */
    unsigned long HdcpAddressWidth;       /* Hdmi address range */
#endif
} STVOUTMod_Param_t;


#define STVOUT_CORE_CMD0   _IO  (STVOUT_CORE_TYPE, 0)         /* icoctl() - no argument */
#define STVOUT_CORE_CMD1   _IOR (STVOUT_CORE_TYPE, 1, int)    /* icoctl() - read an int argument */
#define STVOUT_CORE_CMD2   _IOW (STVOUT_CORE_TYPE, 2, long)   /* icoctl() - write a long argument */
#define STVOUT_CORE_CMD3   _IOWR(STVOUT_CORE_TYPE, 3, double) /* icoctl() - read/write a double argument */

int hdmi_command_client_1(unsigned int cmd, void *arg);
int hdmi_command_client_2(unsigned int cmd, void *arg);
int hdmi_command_client_3(unsigned int cmd, void *arg, U32 N);
#endif
