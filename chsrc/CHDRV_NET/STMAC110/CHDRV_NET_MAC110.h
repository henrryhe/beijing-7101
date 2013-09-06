/*******************头文件说明注释**********************************/
/*公司版权说明：版权（2007）归长虹网络科技有限公司所有。           */
/*文件名：CHDRV_NET_MAC110.h                                              */
/*版本号：V1.0                                                     */
/*作者：  曾祥根                                                   */
/*创建日期：2007-08-10                                             */
/*头文件内容概要描述（功能/变量等）：                              */
/*                                                                 */
/*修改历史：修改时间    作者      修改部分       原因              */
/*                                                                 */
/*******************************************************************/
/*****************条件编译部分**************************************/
#ifndef __CHDRV_NET_MAC110__
#define __CHDRV_NET_MAC110__
/*******************************************************************/
/**************** #include 部分*************************************/
#include "stddef.h"
#include "stddefs.h"

#include "stcommon.h"
#include <osplus.h>
#include <osplus/device.h>
#include <osplus/registry.h>
#include <osplus/stmac110.h>
#include <osplus/dev.h>
#include <osplus/ethernet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************/
/**************** #typedef 申明部分 ********************************/
#define       CHDRV_TCPIP_Handle_t  device_handle_t               

/*******************************************************************/
/***************  外部变量申明部分 *********************************/

extern unsigned char*                  gpc_Buffer_p;                        /*全局buff数据指针, MAC地址信息*/

#ifndef _STMAC110_STMAC110_REGS_H_
#define _STMAC110_STMAC110_REGS_H_


/******************************************************************************
 * STMAC110 DMA Control and Status register offsets
 *****************************************************************************/
#define DMA_BUS_MODE              0x00001000  /* Bus Mode */
#define DMA_XMT_POLL_DEMAND       0x00001004  /* Transmit Poll Demand */
#define DMA_RCV_POLL_DEMAND       0x00001008  /* Received Poll Demand */
#define DMA_RCV_BASE_ADDR         0x0000100c  /* Receive List Base */
#define DMA_XMT_BASE_ADDR         0x00001010  /* Transmit List Base */
#define DMA_STATUS                0x00001014  /* Status Register */
#define DMA_CONTROL               0x00001018  /* Control (Operational Mode) */
#define DMA_INTR_ENA              0x0000101c  /* Interrupt Enable */
#define DMA_MISSED_FRAME_CTR      0x00001020  /* Missed Frame Counter */
#define DMA_CUR_TX_BUF_ADDR       0x00001050  /* Current Host Transmit Buffer */
#define DMA_CUR_RX_BUF_ADDR       0x00001054  /* Current Host Receive Buffer */


/******************************************************************************
 * STMAC110 DMA Bus Mode register defines
 *****************************************************************************/
#define DMA_BUS_MODE_DBO          0x00100000  /* Descriptor Byte Ordering */
#define DMA_BUS_MODE_PBL_MASK     0x00003f00  /* Programmable Burst Length */
#define DMA_BUS_MODE_PBL_SHIFT    8           /*        (in DWORDS)        */
#define DMA_BUS_MODE_BLE          0x00000080  /* Big Endian/Little Endian */
#define DMA_BUS_MODE_DSL_MASK     0x0000007c  /* Descriptor Skip Length */
#define DMA_BUS_MODE_DSL_SHIFT    2           /*       (in DWORDS)      */
#define DMA_BUS_MODE_BAR_BUS      0x00000002  /* Bar-Bus Arbitration */
#define DMA_BUS_MODE_SFT_RESET    0x00000001  /* Software Reset */


/******************************************************************************
 * STMAC110 DMA Receive register defines
 *****************************************************************************/
#define DMA_RCV_BASE_ADDR_MASK    0xfffffffc  /* Start Of Receive List */


/******************************************************************************
 * STMAC110 DMA Transmit register defines
 *****************************************************************************/
#define DMA_XMT_BASE_ADDR_MASK    0xfffffffc  /* Start Of Transmit List */


/******************************************************************************
 * STMAC110 DMA Status register defines
 *****************************************************************************/
#define DMA_STATUS_EB_MASK        0x00380000  /* Error Bits Mask */
#define DMA_STATUS_EB_TX_ABORT    0x00080000  /* Error Bits - TX Abort */
#define DMA_STATUS_EB_RX_ABORT    0x00100000  /* Error Bits - RX Abort */
#define DMA_STATUS_TS_MASK        0x00070000  /* Transmit Process State */
#define DMA_STATUS_TS_SHIFT       20
#define DMA_STATUS_RS_MASK        0x0000e000  /* Receive Process State */
#define DMA_STATUS_RS_SHIFT       17
#define DMA_STATUS_NIS            0x00010000  /* Normal Interrupt Summary */
#define DMA_STATUS_AIS            0x00008000  /* Abnormal Interrupt Summary */
#define DMA_STATUS_ERI            0x00004000  /* Early Receive Interrupt */
#define DMA_STATUS_FBI            0x00002000  /* Fatal Bus Error Interrupt */
#define DMA_STATUS_ETI            0x00000400  /* Early Transmit Interrupt */
#define DMA_STATUS_RWT            0x00000200  /* Receive Watchdog Timeout */
#define DMA_STATUS_RPS            0x00000100  /* Receive Process Stopped */
#define DMA_STATUS_RU             0x00000080  /* Receive Buffer Unavailable */
#define DMA_STATUS_RI             0x00000040  /* Receive Interrupt */
#define DMA_STATUS_UNF            0x00000020  /* Transmit Underflow */
#define DMA_STATUS_OVF            0x00000010  /* Receive Overflow */
#define DMA_STATUS_TJT            0x00000008  /* Transmit Jabber Timeout */
#define DMA_STATUS_TU             0x00000004  /* Transmit Buffer Unavailable */
#define DMA_STATUS_TPS            0x00000002  /* Transmit Process Stopped */
#define DMA_STATUS_TI             0x00000001  /* Transmit Interrupt */


/******************************************************************************
 * STMAC110 DMA Control register defines
 *****************************************************************************/
#define DMA_CONTROL_SF            0x00200000  /* Store And Forward */
#define DMA_CONTROL_TTC_MASK      0x0001c000  /* Transmit Threshold Control */
#define DMA_CONTROL_TTC_32        0x00000000  /* Threshold is 32 DWORDS */
#define DMA_CONTROL_TTC_64        0x00004000  /* Threshold is 64 DWORDS */
#define DMA_CONTROL_TTC_128       0x00008000  /* Threshold is 128 DWORDS */
#define DMA_CONTROL_TTC_256       0x0000c000  /* Threshold is 256 DWORDS */
#define DMA_CONTROL_TTC_18        0x00010000  /* Threshold is 18 DWORDS */
#define DMA_CONTROL_TTC_24        0x00014000  /* Threshold is 24 DWORDS */
#define DMA_CONTROL_TTC__32_      0x00018000  /* Threshold is 32 DWORDS */
#define DMA_CONTROL_TTC_40        0x0001c000  /* Threshold is 40 DWORDS */
#define DMA_CONTROL_ST            0x00002000  /* Start/Stop Transmission */
#define DMA_CONTROL_SE            0x00000008  /* Stop On Empty */
#define DMA_CONTROL_OSF           0x00000004  /* Operate On 2nd Frame */
#define DMA_CONTROL_SR            0x00000002  /* Start/Stop Receive */
#define DMA_CONTROL_DEFAULT       0x00002002  /* Store & forward, RX, TX */


/******************************************************************************
 * STMAC110 DMA Interrupt Enable register defines
 *****************************************************************************/
#define DMA_INTR_ENA_NIE          0x00010000  /* Normal Interrupt Summary */
#define DMA_INTR_ENA_AIE          0x00008000  /* Abnormal Interrupt Summary */
#define DMA_INTR_ENA_ERE          0x00004000  /* Early Receive */
#define DMA_INTR_ENA_FBE          0x00002000  /* Fatal Bus Error */
#define DMA_INTR_ENA_ETE          0x00000400  /* Early Transmit */
#define DMA_INTR_ENA_RWE          0x00000200  /* Receive Watchdog */
#define DMA_INTR_ENA_RSE          0x00000100  /* Receive Stopped */
#define DMA_INTR_ENA_RUE          0x00000080  /* Receive Buffer Unavailable */
#define DMA_INTR_ENA_RIE          0x00000040  /* Receive Interrupt */
#define DMA_INTR_ENA_UNE          0x00000020  /* Underflow */
#define DMA_INTR_ENA_OVE          0x00000010  /* Receive Overflow */
#define DMA_INTR_ENA_TJE          0x00000008  /* Transmit Jabber */
#define DMA_INTR_ENA_TUE          0x00000004  /* Transmit Buffer Unavailable */
#define DMA_INTR_ENA_TSE          0x00000002  /* Transmit Stopped */
#define DMA_INTR_ENA_TIE          0x00000001  /* Transmit Interrupt */
#define DMA_INTR_ENA_DEFAULT      0x0001a3ff  /* Default interrupt mask */


/******************************************************************************
 * STMAC110 DMA Missed Frame Counter register defines
 *****************************************************************************/
#define DMA_MISSED_FRAME_OVE      0x10000000  /* FIFO Overflow Overflow */
#define DMA_MISSED_FRAME_OVE_CNTR 0x0ffe0000  /* Overflow Frame Counter */
#define DMA_MISSED_FRAME_OVE_M    0x00010000  /* Missed Frame Overflow */
#define DMA_MISSED_FRAME_M_CNTR   0x0000ffff  /* Missed Frame Couinter */


/******************************************************************************
 * STMAC110 Ethernet MAC Control and Status register offsets
 *****************************************************************************/
#define MAC_CONTROL               0x00000000  /* MAC Control */
#define MAC_ADDR_HIGH             0x00000004  /* MAC Address High */
#define MAC_ADDR_LOW              0x00000008  /* MAC Address Low */
#define MAC_HASH_HIGH             0x0000000c  /* Multicast Hash Table High */
#define MAC_HASH_LOW              0x00000010  /* Multicast Hash Table Low */
#define MAC_MII_ADDR              0x00000014  /* MII Address */
#define MAC_MII_DATA              0x00000018  /* MII Data */
#define MAC_FLOW_CONTROL          0x0000001c  /* Flow Control */
#define MAC_VLAN1                 0x00000020  /* VLAN1 Tag */
#define MAC_VLAN2                 0x00000024  /* VLAN2 Tag */
#define MAC_WAKEUP_FILTER         0x00000028  /* Wake-up Frame Filter */
#define MAC_WAKEUP_CONTROL_STATUS 0x0000002c  /* Wake-up Control And Status */


/******************************************************************************
 * STMAC110 Ethernet MAC Control register defines
 *****************************************************************************/
#define MAC_CONTROL_RA            0x80000000  /* Receive All Mode */
#define MAC_CONTROL_BLE           0x40000000  /* Endian Mode */
#define MAC_CONTROL_HBD           0x10000000  /* Heartbeat Disable */
#define MAC_CONTROL_PS            0x08000000  /* Port Select */
#define MAC_CONTROL_DRO           0x00800000  /* Disable Receive Own */
#define MAC_CONTROL_EXT_LOOPBACK  0x00400000  /* Reserved (ext loopback?) */
#define MAC_CONTROL_OM            0x00200000  /* Loopback Operating Mode */
#define MAC_CONTROL_F             0x00100000  /* Full Duplex Mode */
#define MAC_CONTROL_PM            0x00080000  /* Pass All Multicast */
#define MAC_CONTROL_PR            0x00040000  /* Promiscuous Mode */
#define MAC_CONTROL_IF            0x00020000  /* Inverse Filtering */
#define MAC_CONTROL_PB            0x00010000  /* Pass Bad Frames */
#define MAC_CONTROL_HO            0x00008000  /* Hash Only Filtering Mode */
#define MAC_CONTROL_HP            0x00002000  /* Hash/Perfect Filtering Mode */
#define MAC_CONTROL_LCC           0x00001000  /* Late Collision Control */
#define MAC_CONTROL_DBF           0x00000800  /* Disable Broadcast Frames */
#define MAC_CONTROL_DRTY          0x00000400  /* Disable Retry */
#define MAC_CONTROL_ASTP          0x00000100  /* Automatic Pad Stripping */
#define MAC_CONTROL_BOLMT_10      0x00000000  /* Back Off Limit 10 */
#define MAC_CONTROL_BOLMT_8       0x00000040  /* Back Off Limit 8 */
#define MAC_CONTROL_BOLMT_4       0x00000080  /* Back Off Limit 4 */
#define MAC_CONTROL_BOLMT_1       0x000000c0  /* Back Off Limit 1 */
#define MAC_CONTROL_DC            0x00000020  /* Deferral Check */
#define MAC_CONTROL_TE            0x00000008  /* Transmitter Enable */
#define MAC_CONTROL_RE            0x00000004  /* Receiver Enable */


/******************************************************************************
 * STMAC110 Ethernet MAC Address High/Low register defines
 *****************************************************************************/
#define MAC_ADDR_HIGH_PADR        0x0000ffff  /* Physical Address [47:32] */
#define MAC_ADDR_LOW_PADR         0xffffffff  /* Physical Address [31:00] */


/******************************************************************************
 * STMAC110 Ethernet MAC MII Address register defines
 *****************************************************************************/
#define MAC_MII_ADDR_PHY_MASK     0x0000001f  /* MII PHY address mask */
#define MAC_MII_ADDR_PHY_SHIFT    11          /* MII PHY address shift */
#define MAC_MII_ADDR_REG_MASK     0x0000001f  /* MII register mask */
#define MAC_MII_ADDR_REG_SHIFT    6           /* MII register shift */
#define MAC_MII_ADDR_WRITE        0x00000002  /* MII Write */
#define MAC_MII_ADDR_BUSY         0x00000001  /* MII Busy */


/******************************************************************************
 * STMAC110 Ethernet MAC Flow Control register defines
 *****************************************************************************/
#define MAC_FLOW_CONTROL_PT_MASK  0xffff0000  /* Pause Time Mask */
#define MAC_FLOW_CONTROL_PT_SHIFT 16
#define MAC_FLOW_CONTROL_PCF      0x00000004  /* Pass Control Frames */
#define MAC_FLOW_CONTROL_FCE      0x00000002  /* Flow Control Enable */
#define MAC_FLOW_CONTROL_PAUSE    0x00000001  /* Flow Control Busy ... */
#define MAC_FLOW_CONTROL_DEFAULT  0xffff0002  /* Default settings */


/******************************************************************************
 * STMAC110 Ethernet MAC Wakeup Control register defines
 *****************************************************************************/
#define MAC_WAKEUP_CONTROL_GUE    0x00000200  /* Global Unicast Enable */
#define MAC_WAKEUP_CONTROL_RWFR   0x00000040  /* Remote Wakeup Frame Received */
#define MAC_WAKEUP_CONTROL_MPR    0x00000020  /* Magic Packet Received */
#define MAC_WAKEUP_CONTROL_WFE    0x00000004  /* Wakeup Frame Enable */
#define MAC_WAKEUP_CONTROL_MPE    0x00000002  /* Magic Packet Enable */


/******************************************************************************
 * STMAC110 Ethernet MAC Management Counters register offsets
 *****************************************************************************/
#define MMC_CONTROL               0x00000100  /* MMC Control */
#define MMC_HIGH_INTR             0x00000104  /* MMC High Interrupt */
#define MMC_LOW_INTR              0x00000108  /* MMC Low Interrupt */
#define MMC_HIGH_INTR_MASK        0x0000010c  /* MMC High Interrupt Mask */
#define MMC_LOW_INTR_MASK         0x00000110  /* MMC Low Interrupt Mask */

#define MMC_RX_ALL_FRMS_CNTR      0x00000200  /* All Frames Received */
#define MMC_RX_OK_FRMS_CNTR       0x00000204  /* Good Frames Received */
#define MMC_RX_CTRL_FRMS_CNTR     0x00000208  /* Control Frames Received */
#define MMC_RX_UNSUP_CTRL_CNTR    0x0000020c  /* Unsupported Control Frames */
#define MMC_RX_ALL_BYTS_CNTR      0x00000210  /* Number of All Bytes Received */
#define MMC_RX_OK_BYTS_CNTR       0x00000214  /* Number of OK Bytes Received */
#define MMC_RX_EQUAL_64_CNTR      0x00000218  /* Frames Length Equal to 64 */
#define MMC_RX_65_127_CNTR        0x0000021c  /* Frames with Length 65-127 */
#define MMC_RX_128_255_CNTR       0x00000220  /* Frames with Length 128-255 */
#define MMC_RX_256_511_CNTR       0x00000224  /* Frames with Length 256-511 */
#define MMC_RX_512_1023_CNTR      0x00000228  /* Frames with Length 512-1023 */
#define MMC_RX_1024_MAX_CNTR      0x0000022c  /* Frames with Length 1024-max */
#define MMC_RX_UNICAST_CNTR       0x00000230  /* Unicast Frames Received */
#define MMC_RX_MULTICAST_CNTR     0x00000234  /* Multicast Frames Received */
#define MMC_RX_BROADCAST_CNTR     0x00000238  /* Broadcast Frames Received */
#define MMC_RX_FIFO_ERR_CNTR      0x0000023c  /* Frames with FIFO Error */
#define MMC_RX_RUNT_FRM_CNTR      0x00000240  /* Runt Frames */
#define MMC_RX_LONG_FRM_CNTR      0x00000244  /* Long Frames */
#define MMC_RX_CRC_ERR_CNTR       0x00000248  /* Frames with CRC Error */
#define MMC_RX_ALGN_ERR_CNTR      0x0000024c  /* Frames with Dribble Bits */
#define MMC_RX_LEN_ERR_CNTR       0x00000250  /* Length Error Frames */
#define MMC_RX_ETH_FRMS_CNTR      0x00000254  /* Ethernet Frames */

#define MMC_TX_ALL_FRMS_CNTR      0x00000300  /* Number of Frames Transmitted */
#define MMC_TX_CTRL_FRMS_CNTR     0x00000304  /* Number of Control Frames */
#define MMC_TX_ALL_BYTS_CNTR      0x00000308  /* Total Number of Bytes */
#define MMC_TX_OK_BYTS_CNTR       0x0000030c  /* Total Number of OK Bytes */
#define MMC_TX_EQUAL_64_CNTR      0x00000310  /* Frames Length Equal to 64 */
#define MMC_TX_65_127_CNTR        0x00000314  /* Frames with Length 65-127 */
#define MMC_TX_128_255_CNTR       0x00000318  /* Frames with Length 128-255 */
#define MMC_TX_256_511_CNTR       0x0000031c  /* Frames with Length 256-511 */
#define MMC_TX_512_1023_CNTR      0x00000320  /* Frames with Length 512-1023 */
#define MMC_TX_1024_MAX_CNTR      0x00000324  /* Frames with Length 1024-max */
#define MMC_TX_UNICAST_CNTR       0x00000328  /* Unicast Frames Transmitted */
#define MMC_TX_MULTICAST_CNTR     0x0000032c  /* Multicast Frames Transmitted */
#define MMC_TX_BROADCAST_CNTR     0x00000330  /* Broadcast Frames Transmitted */
#define MMC_TX_FIFO_UR_CNTR       0x00000334  /* Frames with FIFO error */
#define MMC_TX_BAD_FRMS_CNTR      0x00000338  /* Number of Frames Aborted */
#define MMC_TX_SNGL_COL_CNTR      0x0000033c  /* Frames with Single Collision */
#define MMC_TX_MULT_COL_CNTR      0x00000340  /* Frames Multiple Collisions */
#define MMC_TX_DEF_FRMS_CNTR      0x00000344  /* Frames Deferred */
#define MMC_TX_LATE_COL_CNTR      0x00000348  /* Frames with Late Collision */
#define MMC_TX_ABRT_FRM_CNTR      0x0000034c  /* Number of Frames Aborted */
#define MMC_TX_NO_CRS_CNTR        0x00000350  /* Frames with No Carrier */
#define MMC_TX_XS_DEF_CNTR        0x00000354  /* Frames Excessive Deferral */


/******************************************************************************
 * STMAC110 Ethernet MAC Management Counters Control register defines
 *****************************************************************************/
#define MMC_CONTROL_MAX_FRM_MASK  0x0003ff8   /* Maximum Frame Size */
#define MMC_CONTROL_MAX_FRM_SHIFT 3
#define MMC_CONTROL_RST_ON_READ   0x00000004  /* Reset On Read */
#define MMC_CONTROL_ROLLOVER      0x00000002  /* Rollover */
#define MMC_CONTROL_CNTR_RESET    0x00000001  /* Counters Reset */


/******************************************************************************
 * STMAC110 Ethernet MAC Management Counters Interrupt High register defines
 *****************************************************************************/
#define MMC_HIGH_INTR_TX_XS_DEF   0x00000800  /* TX Excess Deferred */
#define MMC_HIGH_INTR_TX_NO_CRS   0x00000400  /* TX CRS Error Frames */
#define MMC_HIGH_INTR_TX_ABRT_FRM 0x00000200  /* TX Aborted Frames */
#define MMC_HIGH_INTR_TX_LATE_COL 0x00000100  /* TX Late Collision */
#define MMC_HIGH_INTR_TX_DEF_FRMS 0x00000080  /* TX Deferred Frames */
#define MMC_HIGH_INTR_TX_MULT_COL 0x00000040  /* TX Multiple Collision */
#define MMC_HIGH_INTR_TX_SNGL_COL 0x00000020  /* TX Single Collision */
#define MMC_HIGH_INTR_TX_BAD_FRM  0x00000010  /* TX Bad Frames */
#define MMC_HIGH_INTR_TX_FIFO_UR  0x00000008  /* TX FIFO Underrun */
#define MMC_HIGH_INTR_TX_BROADCST 0x00000004  /* TX Broadcast Frames */
#define MMC_HIGH_INTR_TX_MULTICST 0x00000002  /* TX Multicast Frames */
#define MMC_HIGH_INTR_TX_UNICAST  0x00000001  /* TX Unicast Frames */


/******************************************************************************
 * STMAC110 Ethernet MAC Management Counters Interrupt Low register defines
 *****************************************************************************/
#define MMC_LOW_INTR_TX_1024_MAX  0x80000000  /* TX Length 1024-max */
#define MMC_LOW_INTR_TX_512_1023  0x40000000  /* TX Length 512-1023 */
#define MMC_LOW_INTR_TX_256_511   0x20000000  /* TX Length 256-511 */
#define MMC_LOW_INTR_TX_128_255   0x10000000  /* TX Length 128-255 */
#define MMC_LOW_INTR_TX_65_127    0x08000000  /* TX Length 65-127 */
#define MMC_LOW_INTR_TX_EQUAL_64  0x04000000  /* TX Length Equal to 64 */
#define MMC_LOW_INTR_TX_OK_BYTS   0x02000000  /* TX OK Bytes */
#define MMC_LOW_INTR_TX_ALL_BYTS  0x01000000  /* TX All Bytes */
#define MMC_LOW_INTR_TX_CTRL_FRMS 0x00800000  /* TX Control Frames */
#define MMC_LOW_INTR_TX_ALL_FRMS  0x00400000  /* TX Frames */

#define MMC_LOW_INTR_RX_ETH_FRMS  0x00200000  /* RX Ethernet Frames */
#define MMC_LOW_INTR_RX_LEN_ERR   0x00100000  /* RX Length Error */
#define MMC_LOW_INTR_RX_ALGN_ERR  0x00080000  /* RX Dribble Bit Error */
#define MMC_LOW_INTR_RX_CRC_ERR   0x00040000  /* RX CRC Error */
#define MMC_LOW_INTR_RX_LONG_FRM  0x00020000  /* RX Long Frames */
#define MMC_LOW_INTR_RX_RUNT_FRM  0x00010000  /* RX Runt Frames */
#define MMC_LOW_INTR_RX_FIFO_ERR  0x00008000  /* RX FIFO Error Frames */
#define MMC_LOW_INTR_RX_BROADCAST 0x00004000  /* RX Broadcast Frames */
#define MMC_LOW_INTR_RX_MULTICAST 0x00002000  /* RX Multicast Frames */
#define MMC_LOW_INTR_RX_UNICAST   0x00001000  /* RX Unicast Frames */
#define MMC_LOW_INTR_RX_1024_MAX  0x00000800  /* RX Length 1024-max */
#define MMC_LOW_INTR_RX_512_1023  0x00000400  /* RX Length 512-1023 */
#define MMC_LOW_INTR_RX_256_511   0x00000200  /* RX Length 256-511 */
#define MMC_LOW_INTR_RX_128_255   0x00000100  /* RX Length 128-255 */
#define MMC_LOW_INTR_RX_65_127    0x00000080  /* RX Length 65-127 */
#define MMC_LOW_INTR_RX_EQUAL_64  0x00000040  /* RX Length Equal to 64 */
#define MMC_LOW_INTR_RX_OK_BYTS   0x00000020  /* RX OK Bytes */
#define MMC_LOW_INTR_RX_ALL_BYTS  0x00000010  /* RX All Bytes */
#define MMC_LOW_INTR_RX_UNSUP_CTR 0x00000008  /* RX Unsup Control Frames */
#define MMC_LOW_INTR_RX_CTRL_FRMS 0x00000004  /* RX Control Frames */
#define MMC_LOW_INTR_RX_OK_FRMS   0x00000002  /* RX Good Frames */
#define MMC_LOW_INTR_RX_ALL_FRMS  0x00000001  /* RX All Frames */


#endif /* _STMAC110_STMAC110_REGS_H_ */
#define ETHER_DEVICE_NAME       "eth0"                       /*以太网设备驱动名称,必须保持一致*/

/*******************************************************************/
/***************  外部函数申明部分 *********************************/

 static void CH_ReadPacketCallBack0(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack1(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack2(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack3(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack4(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack5(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack6(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack7(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack8(struct ethernet_async_s * r_syn_info);
 static void CH_ReadPacketCallBack9(struct ethernet_async_s * r_syn_info);

/*******************************************************************/
#endif

