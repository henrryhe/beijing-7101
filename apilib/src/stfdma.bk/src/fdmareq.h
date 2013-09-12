/*****************************************************************************
File Name   : fdmareq.h

Description : Request signal configuration table.
              - Provide configuration data for all request lines used
                in the system.
              - Lines internal to the ST device are pre-configured.
              - Lines external to the ST device MUST be configured to suite
                the platform.

Copyright (C) 2003 STMicroelectronics

History     :

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __FDMAREQ_H
#define __FDMAREQ_H

/*--- Request signal configuration details ------*/
typedef struct fmdareq_RequestConfig_s
{
    U8 Index;         /* Request line index number */
    U8 Access;        /* Access type: Read or Write */
    U8 OpCode;        /* Size of word access */
    U8 Count;         /* Number of transfers per request */
    U8 Increment;     /* Whether to increment. On 5517, number of bytes to increment per request */
    U8 HoldOff;       /* Holdoff value between req signal samples (clock cycles)*/
    U8 Initiator;     /* Use the default value */
#if defined (ST_5105) || defined (ST_5107)
    U8 PCMPBitField;  /* Request Triggered Freerunning Transfer Bit */
#endif
#if defined (ST_5525)
    U8 RTF;           /* Request Triggered Freerunning Transfer Bit */
#endif
}fdmareq_RequestConfig_t;

/*---- Constants for use in defining the request signals -----*/

/* Access */
#define ENABLE_FLG      1
#define DISABLE_FLG     0

#if defined(ST_5517)

/* Access */
#define READ            1
#define WRITE           0

/* Opcodes */
#define OPCODE_1        0x0C   /* Opcode 0 */
#define OPCODE_2        0x08   /* Opcode 1 */
#define OPCODE_4        0x08
#define OPCODE_8        0x11
#define OPCODE_16       0x22
#define OPCODE_32       0x43

#else  /* FDMA 2 */

/* Access */
#define READ            0
#define WRITE           1

/* Opcodes */
#define OPCODE_1        0x00
#define OPCODE_2        0x01
#define OPCODE_4        0x02
#define OPCODE_8        0x03
#define OPCODE_16       0x04
#define OPCODE_32       0x05

#endif

/* Increment Size */
#define INCSIZE_0       0
#define INCSIZE_4       4
#define INCSIZE_8       8
#define INCSIZE_16      16
#define INCSIZE_32      32

/* Utility values */
#define UNUSED       0xff
#define EOT          0xfe


/* Indicator for end of request signal table */
#define REQUEST_SIGNAL_TABLE_END   EOT

/* Indicator for unused request signal */
#define REQUEST_SIGNAL_UNUSED      UNUSED


/*
 * Channel 0 request signal control. Other channels at
 * (REQ_CONT_BASE + (ReqLine * REQ_CONT_OFFSET))
 */
#if defined(ST_5517)
#define REQ_CONT_BASE       0x402c
#elif defined (ST_5188)
#define REQ_CONT_BASE       0x8470
#elif defined (ST_5525)
#define REQ_CONT_BASE       0x9640
#elif defined (ST_7100)
#define REQ_CONT_BASE       0x9780
#elif defined (ST_7109)
#define REQ_CONT_BASE       0x9180
#elif defined (ST_7200)
#define REQ_CONT_BASE       0x9180
#else
#define REQ_CONT_BASE       0x4470
#endif

#define REQ_CONT_OFFSET     0x04       /* Request control offset = 4 bytes */

#define SET_REQ_CONT(ReqSignal,Value, Device)    ( SET_REG((REQ_CONT_BASE + ((ReqSignal) * REQ_CONT_OFFSET)), (Value), Device) )
#define GET_REQ_CONT(ReqSignal, Device)          ( GET_REG((REQ_CONT_BASE + ((ReqSignal) * REQ_CONT_OFFSET)), Device) )


typedef enum RequestSignal_e
{
#if defined (ST_5525)

    FDMA_0_COUNTER_REQ,
    PCM_PLAYER0_REQ,

    SWTS0_REQ,
    SWTS1_REQ,
    SWTS2_REQ,
    SWTS3_REQ,

    TVOUT0_VTG0_REQ,
    TVOUT0_VTG1_REQ,
    TVOUT1_VTG0_REQ,
    TVOUT1_VTG1_REQ,
    ILC_REMOTE_4_REQ,
    ILC_REMOTE_5_REQ,
    ILC_REMOTE_6_REQ,
    ILC_REMOTE_7_REQ,
    ILC_LOCAL_0_REQ,
    ILC_LOCAL_1_REQ,
    ILC_LOCAL_2_REQ,
    ILC_LOCAL_3_REQ,
    ILC_LOCAL_4_REQ,
    ILC_LOCAL_5_REQ,
    ILC_LOCAL_6_REQ,
    ILC_LOCAL_7_REQ,
    ILC_LOCAL_8_REQ,
    ILC_LOCAL_9_REQ,
    ILC_LOCAL_10_REQ,
    ILC_LOCAL_11_REQ,
    ILC_LOCAL_12_REQ,

    EXTERNAL_DMAREQ_0,
    EXTERNAL_DMAREQ_1,

    ILC_LOCAL_13_REQ,
    ILC_LOCAL_14_REQ,

    FDMA_0_COUNTER_REQ_B,
    FDMA_1_COUNTER_REQ,

    PCM_PLAYER1_REQ,
    PCM_PLAYER2_REQ,
    PCM_PLAYER3_REQ,

    MODEM_PLAYER_REQ,

    PCM_READER0_REQ,
    PCM_READER1_REQ,

    MODEM_READER_REQ,

    DISEQC_TX_HALF_EMPTY,
    DISEQC_RX_HALF_FULL,

    SSC3_TX_BUFF_EMPTY,
    SSC2_TX_BUFF_EMPTY,
    SSC1_TX_BUFF_EMPTY,
    SSC0_TX_BUFF_EMPTY,
    SSC3_RX_BUFF_FULL,
    SSC2_RX_BUFF_FULL,
    SSC1_RX_BUFF_FULL,
    SSC0_RX_BUFF_FULL,

    UART3_TX_HALF_EMPTY,
    UART2_TX_HALF_EMPTY,
    UART1_TX_HALF_EMPTY,
    UART0_TX_HALF_EMPTY,
    UART3_RX_HALF_FULL,
    UART2_RX_HALF_FULL,
    UART1_RX_HALF_FULL,
    UART0_RX_HALF_FULL,

    ILC_LOCAL_15_REQ,
    FDMA2_GP1_REQ,
    FDMA1_GP1_REQ,
    FDMA1_GP2_REQ,
    FDMA2_GP2_REQ,

    SDPIF_PLAYER_REQ

#elif defined (ST_7200)

    FDMA_MUX_0,
    FDMA_MUX_1,
    FDMA_MUX_2,
    FDMA_MUX_3,
    FDMA_MUX_4,
    FDMA_MUX_5,
    FDMA_MUX_6,
    FDMA_MUX_7,
    FDMA_MUX_8,
    FDMA_MUX_9,
    FDMA_MUX_10,
    FDMA_MUX_11,
    FDMA_MUX_12,
    FDMA_MUX_13,
    FDMA_MUX_14,
    FDMA_MUX_15,
    FDMA_MUX_16,
    FDMA_MUX_17,
    FDMA_MUX_18,
    FDMA_MUX_19,
    FDMA_MUX_20,
    FDMA_MUX_21,
    FDMA_MUX_22,
    FDMA_MUX_23,
    FDMA_MUX_24,
    FDMA_MUX_25,
    FDMA_MUX_26,
    FDMA_MUX_27,
    FDMA_MUX_28,
    FDMA_MUX_29,
    FDMA_MUX_30,
    FDMA_MUX_31,
    FDMA_MUX_32,
    FDMA_MUX_33,
    FDMA_MUX_34,
    FDMA_MUX_35,
    FDMA_MUX_36,
    FDMA_MUX_37,
    FDMA_MUX_38,
    FDMA_MUX_39,
    FDMA_MUX_40,
    FDMA_MUX_41,
    FDMA_MUX_42,
    FDMA_MUX_43,
    FDMA_MUX_44,
    FDMA_MUX_45,
    FDMA_MUX_46,
    FDMA_MUX_47,
    FDMA_MUX_48,
    FDMA_MUX_49,
    FDMA_MUX_50,
    FDMA_MUX_51,
    FDMA_MUX_52,
    FDMA_MUX_53,
    FDMA_MUX_54,
    FDMA_MUX_55,
    FDMA_MUX_56,
    FDMA_MUX_57,
    FDMA_MUX_58,
    FDMA_MUX_59,
    FDMA_MUX_60,
    FDMA_MUX_61,
    FDMA_MUX_62,
    FDMA_MUX_63,

#else

    FDMA_0_COUNTER_REQ

#endif
} RequestSignal_t;


/* eof */
#endif
