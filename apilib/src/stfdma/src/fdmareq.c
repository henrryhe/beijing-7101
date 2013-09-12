/******************************************************************************

    File Name   :

    Description :

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <asm/semaphore.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#endif

#ifdef STFDMA_EMU                       /* For test purposes only */
#define STTBX_Print(x)  printf x
#else
#if !defined (ST_OSLINUX)
#include "sttbx.h"
#endif
#endif

#include "local.h"                      /* FDMA request signal table */
#include "fdmareq.h"                    /* FDMA request signal table */

#if defined (ST_5525)
#define FDMA_MUX_BASE_ADDRESS       0x19F00000
#elif defined (ST_7200)
#define FDMA_MUX_BASE_ADDRESS       0xfd830000
#endif

#if defined (ST_5525) || defined (ST_7200)
#define SET_MUX(Offset, Value)   ( *((volatile U32*) ((FDMA_MUX_BASE_ADDRESS) + ((U32)Offset))) = ((U32)Value) )
#endif


/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */

/*---- Table of Request Line configuration details ----
 *
 * NOTE: External lines should be modified to suite customer requirements
 *       if they are to be used. The current definition is provided for
 *       for use with the STFDMA test harness.
 *  Format:
 *  {  Req  RnW,    Opcode,  TransferCount   Inc,       Hold_off      Initiator, RTF  "(5525)" ),
 */


fdmareq_RequestConfig_t fdmareq_RequestLines[] =
{
#if defined(ST_5517)

/*=========================== 5517 ============================================*/

/*  {  Req  RnW,    Opcode,   TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32      NotUsed        0-32         0-256 Cycles  Not Used)   */

    {  1,   WRITE,  OPCODE_8, 1,             INCSIZE_0,   0,            0 },   /* PCMO hi priority */
    {  2,   READ,   OPCODE_8, 1,             INCSIZE_0,   0,            0 },   /* PCMI hi priority */
    {  3,   WRITE,  OPCODE_4, 1,             INCSIZE_0,   0,            0 },   /* SWTS */
    {  4,   READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },   /* External 0 (PIO2bit5) hi priority */
    {  5,   READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },   /* External 1 (PIO2bit6) hi priority */

    {  6,   READ,   OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 0 rx half full */
    {  7,   READ,   OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 1 rx half full */
    {  8,   READ,   OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 2 rx half full */
    {  9,   READ,   OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 3 rx half full */
    {  10,  READ,   OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 4 rx half full */
    {  11,  WRITE,  OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 0 tx half empty */
    {  12,  WRITE,  OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 1 tx half empty */
    {  13,  WRITE,  OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 2 tx half emtpy */
    {  14,  WRITE,  OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 3 tx half empty */
    {  15,  WRITE,  OPCODE_1, 1,             INCSIZE_0,   0,            0 },  /* UART 4 tx half empty */

    {  16,  READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* SSC 0 rxbuff full */
    {  17,  READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* SSC 1 rxbuff full */
    {  18,  WRITE,  OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* SSC 0 txbuff full */
    {  19,  WRITE,  OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* SSC 1 txbuff full */
    {  20,  WRITE,  OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* Video CDReq */
    {  21,  WRITE,  OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* Audio CDReq */
    {  22,  WRITE,  OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* SP decoder CDReq */
    {  23,  READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* CDReq external 0 */
    {  24,  READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* CDReq external 1 */
    {  25,  UNUSED, UNUSED,   1,             UNUSED,      0,            0 },  /* Entry 4 replicated. Lo priority */
    {  26,  UNUSED, UNUSED,   1,             UNUSED,      0,            0 },  /* Entry 5 replicated. Lo priority */

    {  27,  READ,   OPCODE_4, 1,             INCSIZE_0,   0,            0 },  /* PTI D1 */

    {  28,  WRITE,  OPCODE_8, 1,             INCSIZE_0,   0,            0 },  /* Entry 1 replicated. Lo priority */
    {  29,  READ,   OPCODE_8, 1,             INCSIZE_0,   0,            0 },  /* Entry 2 replicted. Lo priority */
    {  30,  UNUSED, UNUSED,   1,             UNUSED,      0,            0 },  /* NOT CONNECTED */

#elif defined (ST_5100) || defined (ST_5101) || defined (ST_5301)

/*=========================== 5100 ============================================*/

/*  {  Req  RnW,    Opcode,   TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32      1-4            On/Off       0-2           Used) */

    {  0,   WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* SPDIF Testing */

    {  1,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 0 (PIO2bit5) hi priority */
    {  2,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 1 (PIO2bit6) hi priority */

    {  3,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 0 rx half full */
    {  4,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 1 rx half full */
    {  5,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 2 rx half full */
    {  6,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 3 rx half full */
    {  7,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 0 tx half empty */
    {  8,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 1 tx half empty */
    {  9,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 2 tx half emtpy */
    {  10,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 3 tx half empty */

    {  11,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 rxbuff full */
    {  12,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 rxbuff full */
    {  13,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 2 rxbuff full */
    {  14,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 txbuff empty */
    {  15,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */
    {  16,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */

    {  17,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* DiseqC half full */
    {  18,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* DiseqC half empty */

    {  19,  WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* VTG Trigger 0 */
    {  20,  READ,   OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* VTG Trigger 1 */

    {  21,  WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* ILC Trigger 0 */
    {  22,  READ ,  OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* ILC Trigger 1 */

    {  28,  WRITE,  OPCODE_32, 1,             DISABLE_FLG, 0,            1 },  /* SWTS */
    {  29,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* Audio CDReq */
    {  30,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* Audio SPDIF - 2xST4*/


#elif defined (ST_5105)

/*=========================== 5105 ============================================*/

/*  {  Req  RnW,    Opcode,   TransferCount  Inc,       Hold_off        Initiator   PCM Player), */
/*  {               1-32      1-4            On/Off       0-2           Used        Bitfield) */

    {  0,   UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */

    {  1,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* External 0 (PIO2bit5) hi priority */
    {  2,   UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* NOT CONNECTED */

    {  3,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* Audio CDReq */
    {  4,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* Audio Decoder*/
    {  5,   WRITE,  OPCODE_4,  5,             DISABLE_FLG, 0,            0,         1 },  /* Audio PCM */
    {  6,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            0,         0 },  /* Audio SPDIF - 2xST4*/

    {  7,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 0 rx half full */
    {  8,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 1 rx half full */
    {  9,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 1 tx half empty */
    {  10,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 2 tx half emtpy */

    {  11,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 0 rxbuff full */
    {  12,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 1 rxbuff full */
    {  13,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 0 txbuff empty */
    {  14,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 1 txbuff empty */

    {  15,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* IRB rx full */
    {  16,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* IRB tx empty */

    {  17,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            0,         0 },  /* VTG */

    {  18,  WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            0,         0 },  /* ILC Trigger 0 */
    {  19,  READ ,  OPCODE_4,  1,             ENABLE_FLG,  0,            0,         0 },  /* ILC Trigger 1 */

    {  20,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  21,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  22,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  23,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  24,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  25,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  26,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  27,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  28,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  29,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  30,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */
    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            0,         0 },  /* Reserved */

#elif defined (ST_5107)

/*=========================== 5107 ============================================*/
/*  {  Req  RnW,    Opcode,   TransferCount  Inc,       Hold_off        Initiator   PCM Player), */
/*  {               1-32      1-4            On/Off       0-2           Used        Bitfield) */

    {  0,   UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  1,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* Extint0 (PIO36) CVBS_VSYNC */
    {  2,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* Extint1 (PIO35) CVBS_HSYNC */

    {  3,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* Audio CDReq */
    {  4,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* Audio Decoder*/
    {  5,   WRITE,  OPCODE_4,  5,             DISABLE_FLG, 0,            0,         1 },  /* Audio PCM */
    {  6,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            0,         0 },  /* Audio SPDIF - 2xST4*/
    {  7,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 0 rx half full */
    {  8,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 1 rx half full */
    {  9,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 0 tx half empty */
    {  10,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            0,         0 },  /* UART 1 tx half emtpy */
    {  11,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 0 rxbuff full */
    {  12,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 1 rxbuff full */
    {  13,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 0 txbuff empty */
    {  14,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            0,         0 },  /* SSC 1 txbuff empty */
    {  15,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* IRB rx full */
    {  16,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0,         0 },  /* IRB tx empty */
    {  17,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            0,         0 },  /* VTG */
    {  18,  WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            0,         0 },  /* ilc_local_interrupt_out: ILC Trigger 0 */
    {  19,  WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            0,         0 },  /* ilc_remote_interrupt_out: ILC Trigger 1 */

    {  20,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  21,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  22,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  23,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  24,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  25,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  26,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  27,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  28,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  29,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  30,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */
    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            1,         0 },  /* Reserved */

#elif defined (ST_5188)

/*=========================== 5188 ============================================*/

/*  {  Req  RnW,    Opcode,   TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32      1-4            On/Off       0-2           Used) */

    {  0,  	UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */

    {  1,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* Config Bit G[31]. External 0 (PIO2bit5) hi priority */
/*  {  2,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 }, */ /* External 1 (PIO2bit6) hi priority */
    {  2,   READ,   OPCODE_4,  24,            DISABLE_FLG, 0,            1 },  /* fei_data_req ?*/

    {  3,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* Audio CDReq, audio_cd_req_empty */
    {  4,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* Audio Decoder, audio_pcmo_req_full */
    {  5,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* audio_pcmplayer_req */
    {  6,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* audio_spdifplayer_req ? */

    {  7,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* uart[0]_rx_half_full 7 */

    {  8,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* audio_pcmo_half_full NOUSE?*/

    {  9,   WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* uart[0]_tx_half_empty 9 */

    {  10,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* audio_cdin_half_full NOUSE?*/

    {  11,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* ssc[0]_rxbuff_half_full */
    {  12,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* ssc[1]_rxbuff_half_full */
    {  13,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* ssc[0]_txbuff_half_empty */
    {  14,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* ssc[1]_txbuff_half_empty */

    {  15,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* irb_rxbuff_half_full */
    {  16,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* irb_txbuff_half_empty */

    {  17,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* VTG_trig */

    {  18,  WRITE,  OPCODE_4,  1,             ENABLE_FLG, 0,             1 },  /* ilc_local_interrupt_out */

    {  19,  WRITE,  OPCODE_4,  1,             ENABLE_FLG, 0,             1 },  /* ilc_ext_interrupt_out */

    {  20,  READ,   OPCODE_4,  24,            DISABLE_FLG,  0,            1 },  /* fei_dreq */
    {  21,  READ,   OPCODE_4,  24,            DISABLE_FLG,  0,            1 },  /* fei_dreq_start */
    {  22,  READ ,  OPCODE_4,  1,             DISABLE_FLG,  0,            1 },  /* fei_pcm_req */
    {  23,  READ,   OPCODE_4,  1,             DISABLE_FLG,  0,            1 },  /* fei_tvo_req */
    {  24,  READ ,  OPCODE_4,  24,            DISABLE_FLG,  0,            1 },  /* fei_data_dreq */

    {  25,  WRITE,  OPCODE_4,  5,             DISABLE_FLG, 0,            1 },  /* audio_pcmplayer_req ?*/
    {  26,  WRITE , OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* audio_spdif_req ?*/

    {  27,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */
    {  28,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */
    {  29,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */
    {  30,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */

    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */

#elif defined (ST_5162)

/*=========================== 5162 ============================================*/

/*  {  Req  RnW,    Opcode,   TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32      1-4            On/Off       0-2           Used) */

        {  0,   UNUSED, UNUSED,    1,            UNUSED,      0,            1 },  /* Reserved */

        {  1,   READ,   OPCODE_4,  1,            DISABLE_FLG, 0,            0 },  /* Dreq[0:1]*/
        {  2,   READ,   OPCODE_4,  24,           DISABLE_FLG, 0,            0 },  /* Dreq[0:1]*/

        {  3,   READ,   OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart2_rx_half_full */
        {  4,   READ,   OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart3_rx_half_full */
        {  5,   WRITE,  OPCODE_1,  4,            DISABLE_FLG, 0,            0 },  /* uart2_tx_half_empty */
        {  6,   WRITE,  OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart3_tx_half_empty */

        {  7,   UNUSED, UNUSED,    1,            UNUSED,      0,            1 },  /* Reserved */

        {  8,   READ,   OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart[0]_rx_half_full*/
        {  9,   READ,   OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart[1]_rx_half_full */
        {  10,  WRITE,  OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart[0]_tx_half_empty*/
        {  11,  WRITE,  OPCODE_1,  4,            DISABLE_FLG, 0,            0},  /* uart[1]_tx_half_empty */

        {  12,  READ,   OPCODE_2,  4,            DISABLE_FLG, 0,            0},  /* ssc[0]_rxbuff_half_full */
        {  13,  READ,   OPCODE_2,  4,            DISABLE_FLG, 0,            0},   /* ssc[1]_rxbuff_half_full */
        {  14,  WRITE,  OPCODE_2,  4,            DISABLE_FLG, 0,            0 },  /* ssc[0]_txbuff_half_empty */
        {  15,  WRITE,  OPCODE_2,  4,            DISABLE_FLG, 0,            0 },  /* ssc[1]_txbuff_half_empty */

        {  16,  READ,   OPCODE_4,  1,            DISABLE_FLG, 0,            0},  /* irb_rxbuff_half_full */
        {  17,  WRITE,  OPCODE_4,  1,            DISABLE_FLG, 0,            0},  /* irb_txbuff_half_empty */

        {  18,  READ,   OPCODE_4,  2,            DISABLE_FLG, 0,            0},  /* VTG_trig */

        {  19,  WRITE,  OPCODE_4,  1,            ENABLE_FLG, 0,             0},  /* ilc_local_interrupt_out */
        {  20,  WRITE,  OPCODE_4,  1,            ENABLE_FLG,  0,            0},  /* ilc_remote_interrupt_out */

        {  21,  WRITE,  OPCODE_4,  1,            DISABLE_FLG, 0,            0},  /* o_nand_data_Dreq */
        {  22,  WRITE,  OPCODE_4,  1,            DISABLE_FLG, 0,            0},   /*o_nand_seq_Dreq */

        {  23,  WRITE,  OPCODE_32, 1,            DISABLE_FLG, 0,            0},   /* swts0_fdma_req*/

        {  24,  WRITE,  OPCODE_4,  1,            DISABLE_FLG, 0,            0},   /* audio_cdin_empty */
        {  25,  READ,   OPCODE_32,  1,            DISABLE_FLG, 0,            0},   /* audio_pcmo_full*/
        {  26,  WRITE,  OPCODE_4,  2,            DISABLE_FLG, 0,            0 },  /* pcm_fdma_req*/
        {  27,  WRITE,  OPCODE_4,  2,            DISABLE_FLG, 0,            0},   /* spdif_fdma_req */
        {  28,  WRITE,  OPCODE_32, 1,            DISABLE_FLG, 0,            0},  /* swts0_fdma_req */

        {  29,  READ,   OPCODE_4,  1,            DISABLE_FLG, 0,            0},  /* audio_pcmo_half_full */
        {  30,  READ,   OPCODE_4,  1,            DISABLE_FLG, 0,            0},   /* audio_cdin_half_full */

        {  31,  UNUSED, UNUSED,    1,            UNUSED,      0,            1 },  /* counter_req Reserved */
    
#elif defined (ST_5525)

/*=========================== 5525 ============================================*/

/*  {  Req  RnW,    Opcode,    TransferCount  Inc,       Hold_off        Initiator, RTF ), */
/*  {               1-32       1-4            On/Off       0-2           Used)      RTF transfer req */

    {  0,  UNUSED, UNUSED,    1,             UNUSED,      0,            1, 0 },  /* Reserved */

    {  1,   WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0, 0 },  /* Audio PCM Player 0 */
    {  2,   WRITE,  OPCODE_16, 1,             DISABLE_FLG, 0,            0, 0 },  /* SWTS */
    {  3,   WRITE,  OPCODE_16, 1,             DISABLE_FLG, 0,            0, 0 },  /* SWTS1 */
    {  4,   WRITE,  OPCODE_16, 1,             DISABLE_FLG, 0,            0, 0 },  /* SWTS2 */
    {  5,   WRITE,  OPCODE_16, 1,             DISABLE_FLG, 0,            0, 0 },  /* SWTS3 */
    {  6,   WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0, 0 },  /* Audio PCM Player 1 */
    {  7,   WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0, 0 },  /* Audio PCM Player 2 */
    {  8,   WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0, 0 },  /* Audio PCM Player 3 */
    {  9,   WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0, 0 },  /* Analog IO Audio PCM Player */
    {  10,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0, 0 },  /* Audio PCM Reader 0 */
    {  11,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0, 0 },  /* Audio PCM Reader 1 */
    {  12,  WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0, 0 },  /* Analog IO Audio PCM Reader */
    {  13,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1, 0 },  /* DiseqC half empty */
    {  14,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1, 0 },  /* DiseqC half full */
    {  15,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 3 txbuff empty */
    {  16,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 2 txbuff empty */
    {  17,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 1 txbuff empty */
    {  18,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 0 txbuff empty */
    {  19,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 3 rxbuff full */
    {  20,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 2 rxbuff full */
    {  21,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 1 rxbuff full */
    {  22,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1, 0 },  /* SSC 0 rxbuff full */
    {  23,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 3 tx half empty */
    {  24,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 2 tx half empty */
    {  25,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 1 tx half emtpy */
    {  26,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 0 tx half empty */
    {  27,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 2 rx half full */
    {  28,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 1 rx half full */
    {  29,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1, 0 },  /* UART 0 rx half full */
    {  30,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            0, 0 },  /* Audio SPDIF - 2xST4*/

    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            1, 0 },  /* Reserved */

#elif defined (ST_7710)

/*=========================== 7710 ============================================*/

/*  {  Req  RnW,    Opcode,    TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32       1-4            On/Off       0-2           Used) */

    {  0,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */
    {  1,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  2,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  3,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  4,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  5,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  6,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  7,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */

    {  8,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* IRB rx full */

    {  9,   READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 0 rx full */
    {  10,  READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 1 rx full */
    {  11,  READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 2 rx full */
    {  12,  READ,   OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 3 rx full */
    {  13,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 0 tx empty */
    {  14,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 1 tx empty */
    {  15,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 2 tx emtpy */
    {  16,  WRITE,  OPCODE_1,  4,             DISABLE_FLG, 0,            1 },  /* UART 3 tx empty */

    {  17,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 txbuff empty */
    {  18,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */
    {  19,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 2 txbuff empty */
    {  20,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 3 txbuff empty */
    {  21,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 rxbuff full */
    {  22,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 rxbuff full */
    {  23,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 2 rxbuff full */
    {  24,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 3 rxbuff full */

    {  25,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* Audio CDReq */
    {  26,  WRITE,  OPCODE_16, 1,             DISABLE_FLG, 0,            0 },  /* SWTS */

    {  27,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 0 */
    {  28,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 1  */

    {  29,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  30,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */

#elif defined (ST_7100)

/*=========================== 7100 ============================================*/

/*  {  Req  RnW,    Opcode,    TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32       1-4            On/Off       0-2           Used) */

    {  0,   WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* SPDIF Testing */

    {  1,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */
    {  2,   UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* NOT CONNECTED */

    {  3,   READ,   OPCODE_8,  1,             DISABLE_FLG, 0,            1 },  /* Video HDMI */

    {  4,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* DiseqC half empty */
    {  5,   READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* DiseqC half full */

    {  6,   READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* SH4/SCIF */
    {  7,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* SH4/SCIF */

    {  8,   READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 rxbuff full */
    {  9,   READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 rxbuff full */
    {  10,  READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 2 rxbuff full */
    {  11,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 txbuff empty */
    {  12,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */
    {  13,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */

    {  14,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 0 rx half full */
    {  15,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 1 rx half full */
    {  16,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 2 rx half full */
    {  17,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 3 rx half full */
    {  18,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 0 tx half empty */
    {  19,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 1 tx half empty */
    {  20,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 2 tx half emtpy */
    {  21,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 3 tx half empty */

    {  22,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 0 (PIO2bit5) hi priority */
    {  23,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 1 (PIO2bit6) hi priority */

    {  24,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* CPxM decrypted data request */
    {  25,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* CPxm encrypted data request */

    {  26,  WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            1 },  /* Audio PCM Player 0 */
    {  27,  WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            1 },  /* Audio PCM Player 1 */
    {  28,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* Audio PCM Reader */
    {  29,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* Audio SPDIF - 2xST4*/

    {  30,  WRITE,  OPCODE_32, 1,             DISABLE_FLG, 0,            1 },  /* SWTS */

    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            1 },  /* Reserved */

#elif defined (ST_7109)

/*=========================== 7109 ============================================*/

/*  {  Req  RnW,    Opcode,    TransferCount  Inc,       Hold_off        Initiator), */
/*  {               1-32       1-4            On/Off       0-2           Used) */

    {  0,   WRITE,  OPCODE_4,  1,             ENABLE_FLG,  0,            1 },  /* SPDIF Testing */

    {  1,   READ,   OPCODE_8,  1,             DISABLE_FLG, 0,            1 },  /* Video HDMI */

    {  2,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* DiseqC half empty */
    {  3,   READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* DiseqC half full */

    {  4,   READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* SH4/SCIF */
    {  5,   WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* SH4/SCIF */

    {  6,   READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 0 rxbuff full */
    {  7,   READ,   OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 rxbuff full */
    {  8,   READ,   OPCODE_2,  4,             DISABLE_FLG, 0,             1 },  /* SSC 2 rxbuff full */
    {  9,   WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,             1 },  /* SSC 0 txbuff empty */
    {  10,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */
    {  11,  WRITE,  OPCODE_2,  4,             DISABLE_FLG, 0,            1 },  /* SSC 1 txbuff empty */

    {  12,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 0 rx half full */
    {  13,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 1 rx half full */
    {  14,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 2 rx half full */
    {  15,  READ,   OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 3 rx half full */
    {  16,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 0 tx half empty */
    {  17,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 1 tx half empty */
    {  18,  WRITE,  OPCODE_1,  1,             DISABLE_FLG, 0,            1 },  /* UART 2 tx half emtpy */
    {  19,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART 3 tx half empty */

    {  20,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 0 (PIO2bit5) hi priority */
    {  21,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* External 1 (PIO2bit6) hi priority */

    {  22,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* CPxM decrypted data request */
    {  23,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* CPxm encrypted data request */

    {  24,  WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0 },  /* Audio PCM Player 0 */
    {  25,  WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            0 },  /* Audio PCM Player 1 */
    {  26,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* Audio PCM Reader */
    {  27,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            0 },  /* Audio SPDIF - 2xST4*/

    {  28,  WRITE,  OPCODE_32, 1,             DISABLE_FLG, 0,            0 },  /* SWTS 0 */
    {  29,  WRITE,  OPCODE_32, 1,             DISABLE_FLG, 0,            0 },  /* SWTS 1 */
    {  30,  WRITE,  OPCODE_32, 1,             DISABLE_FLG, 0,            0 },  /* SWTS 2 */

    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* Reserved */

#elif defined (ST_7200)

/*=========================== 7200 ============================================*/

/*  {  Req  RnW,    Opcode,    TransferCount  Inc,       Hold_off        Initiator),
    {               1-32       1-4            On/Off       0-2           Used) */

/*  FDMA #0  */

    {  0,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  1,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  2,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  3,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  4,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  5,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  6,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  7,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  8,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  9,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  10,  UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */
    {  11,  UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */

    {  12,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SCIF_RXBUF_READY */
    {  13,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SCIF_TXBUF_READY */

    {  14,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC0_RXBUF_FULL */
    {  15,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC0_TXBUF_EMPTY */

    {  16,  READ,   OPCODE_32, 4,             DISABLE_FLG, 0,            1 },  /* EXT_DMA_REQ1 */

    {  17,  WRITE,  OPCODE_4,  10,            DISABLE_FLG, 0,            1 },  /* AUDIO_SRC_INPUT */
    {  18,  READ,   OPCODE_4,  10,            DISABLE_FLG, 0,            1 },  /* AUDIO_SRC_OUTPUT */

    {  19,  WRITE,  OPCODE_4,  32,            DISABLE_FLG, 0,            1 },  /* CPXM_DECRYPTED_INPUT */
    {  20,  READ,   OPCODE_4,  32,            DISABLE_FLG, 0,            1 },  /* CPXM_DECRYPTED_OUTPUT */
    {  21,  WRITE,  OPCODE_4,  32,            DISABLE_FLG, 0,            1 },  /* CPXM_ENCRYPTED_INPUT */
    {  22,  READ,   OPCODE_4,  32,            DISABLE_FLG, 0,            0 },  /* CPXM_ENCRYPTED_OUTPUT */

    {  23,  WRITE,  OPCODE_4,  20,            DISABLE_FLG, 0,            0 },  /* PCM_PLYR0 */
    {  24,  WRITE,  OPCODE_4,  20,            DISABLE_FLG, 0,            0 },  /* PCM_PLYR1 */
    {  25,  WRITE,  OPCODE_4,  20,            DISABLE_FLG, 0,            0 },  /* PCM_PLYR2 */
    {  26,  WRITE,  OPCODE_4,  20,            DISABLE_FLG, 0,            0 },  /* PCM_PLYR3 */

    {  27,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            0 },  /* PCM_READER*/

    {  28,  WRITE,  OPCODE_4,  4,             DISABLE_FLG, 0,            0 },  /* SPDIF_PLYR */

    {  29,  WRITE,  OPCODE_4,  20,            DISABLE_FLG, 0,            1 },  /* HDMI_PCM_PLYR */
    {  30,  WRITE,  OPCODE_4,  4,             DISABLE_FLG, 0,            1 },  /* HDMI_SPDIF_PLYR */

    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */

    { EOT,  EOT,    EOT,      EOT,           EOT,         EOT,          EOT }, /* End of table marker */

/*  FDMA #1  */

    {  0,   UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */

    {  1,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC1_RXBUF_FULL */
    {  2,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC2_RXBUF_FULL */
    {  3,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC3_RXBUF_FULL */
    {  4,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC4_RXBUF_FULL */

    {  5,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC1_TXBUF_EMPTY */
    {  6,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC2_TXBUF_EMPTY */
    {  7,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC3_TXBUF_EMPTY */
    {  8,   WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* SSC4_TXBUF_EMPTY */

    {  9,   READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART0_RXBUF_HALF_FULL */
    {  10,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART1_RXBUF_HALF_FULL */
    {  11,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART2_RXBUF_HALF_FULL */
    {  12,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART3_RXBUF_HALF_FULL */

    {  13,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART0_TXBUF_HALF_EMPTY */
    {  14,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART1_TXBUF_HALF_EMPTY */
    {  15,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART2_TXBUF_HALF_EMPTY */
    {  16,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UART3_TXBUF_HALF_EMPTY */

    {  17,  READ,   OPCODE_32, 4,             DISABLE_FLG, 0,            1 },  /* EXT_DMA_REQ0 */

    {  18,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UHF2_RXBUF_EMPTY */
    {  19,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            1 },  /* UHF2_RXBUF_FULL */

    {  20,  READ,   OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* MODEM_PCM_PLYR */
    {  21,  WRITE,  OPCODE_4,  2,             DISABLE_FLG, 0,            1 },  /* MODEM_PCM_READER */

    {  22,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* DISEQC0_TX_HALF_EMPTY */
    {  23,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* DISEQC0_RX_HALF_FULL */
    {  24,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* DISEQC1_TX_HALF_EMPTY */
    {  25,  READ,   OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* DISEQC1_RX_HALF_FULL */

    {  26,  WRITE,  OPCODE_4,  1,             DISABLE_FLG, 0,            0 },  /* HDMI_AVI_BUFF_EMPTY */

    {  27,  READ,   OPCODE_32, 1,             DISABLE_FLG, 0,            0 },  /* NAND_DATA */
    {  28,  WRITE,  OPCODE_32, 1,             DISABLE_FLG, 0,            0 },  /* NAND_CMD */

    {  29,  WRITE,  OPCODE_4,  12,            DISABLE_FLG, 0,            0 },  /* TTXT */
    {  30,  WRITE,  OPCODE_4,  12,            DISABLE_FLG, 0,            0 },  /* TTXT_SDTVOUT */

    {  31,  UNUSED, UNUSED,    1,             UNUSED,      0,            0 },  /* RESERVED */

#else

/*=========================== UNKNOWN =========================================*/

    #error This hardware does not have a request line configuration

#endif

/*=========================== END OF TABLE ====================================*/
    { EOT,  EOT,    EOT,      EOT,           EOT,         EOT,          EOT } /* End of table marker */
};


/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

/****************************************************************************
Name         : stfdma_ReqSetRead
Description  : Private access function for test purposes. Set the paced
               signal to read.
Parameters   : Paced signal number
Return Value : None
****************************************************************************/
void stfdma_ReqSetRead(int Index)
{
    U32 ReqControl = GET_REQ_CONT(Index, STFDMA_2);

#if defined(ST_5517)
    ReqControl &= (U32)~(1 << 8);            /*  " 8  */
    ReqControl |= (U32)(READ & 0x01) << 8;   /*  " 8  */
#else
    ReqControl &= (U32)~(1 << 14);           /*  " 14 */
    ReqControl |= (U32)(READ & 0x01) << 14;  /*  " 14 */
#endif

    SET_REQ_CONT(Index, ReqControl, STFDMA_2);
}

/****************************************************************************
Name         : stfdma_ReqSetWrite
Description  : Private access function for test purposes. Set the paced
               signal to write.
Parameters   : Paced signal number
Return Value : None
****************************************************************************/
void stfdma_ReqSetWrite(int Index)
{
    U32 ReqControl = GET_REQ_CONT(Index, STFDMA_2);

#if defined(ST_5517)
    ReqControl &= (U32)~(1 << 8);             /*  " 8  */
    ReqControl |= (U32)(WRITE & 0x01) << 8;   /*  " 8  */
#else
    ReqControl &= (U32)~(1 << 14);            /*  " 14 */
    ReqControl |= (U32)(WRITE & 0x01) << 14;  /*  " 14 */
#endif

    SET_REQ_CONT(Index, ReqControl, STFDMA_2);
}

/****************************************************************************
Name         : stfdma_ReqSetIncrement
Description  : Private access function for test purposes. Enable/disable address
               incrementing for the CDFIFO address.
Parameters   : Paced signal number
Return Value : None
****************************************************************************/
void stfdma_ReqSetIncrement(int Index, int On)
{
    U32 ReqControl = GET_REQ_CONT(Index, STFDMA_2);
    U32 State      = DISABLE_FLG;

    if (On) State = ENABLE_FLG;

#if defined(ST_5517)
    ReqControl &= ~(1 << 16);     /*  " 23..16 */
    ReqControl |= (State << 16);  /*  " 23..16 */
#else
    ReqControl &= ~(1 << 29);     /*  " 29 */
    ReqControl |= (State << 29);  /*  " 29 */
#endif

    SET_REQ_CONT(Index, ReqControl, STFDMA_2);
}

/****************************************************************************
Name         : stfdma_ReqConf
Description  : Configure the paced signals in the FDMA block.
Parameters   :
Return Value : None
****************************************************************************/
void stfdma_ReqConf(ControlBlock_t *ControlBlock_p, STFDMA_Block_t DeviceNo)
{
    int i = 0;
    U32 ReqControl = 0;

    /* Load request signal configuration.
     * Request signal details held in external code table.
     * Data in table used to contruct a single 32 bit word for each request line which is
     * loaded to the FDMA.
     */
    STTBX_Print(("Loading request signal config values\n"));

    if (DeviceNo == STFDMA_1)
        i = 0;
    else if (DeviceNo == STFDMA_2)
        i = 33;

    while (fdmareq_RequestLines[i].Index != REQUEST_SIGNAL_TABLE_END)
    {
        /* If request signal to be used, configure it, otherwise leave as default */
        if (fdmareq_RequestLines[i].Access != REQUEST_SIGNAL_UNUSED)
        {
            /* Build request control word according to table values */
            ReqControl = 0;
#if defined(ST_5517)
            ReqControl |= (U32)(fdmareq_RequestLines[i].HoldOff   & 0xff) << 24;  /* Bits 31...24 */
            ReqControl |= (U32)(fdmareq_RequestLines[i].Increment & 0xff) << 16;  /*  " 23..16 */
            ReqControl |= (U32)(fdmareq_RequestLines[i].OpCode    & 0x3f) << 9;   /*  " 15...9 */
            ReqControl |= (U32)(fdmareq_RequestLines[i].Access    & 0x01) << 8;   /*  " 8  */
#else
            ReqControl |= (U32)(fdmareq_RequestLines[i].HoldOff   & 0x0f) <<  0;  /* Bits 3...0 */
            ReqControl |= (U32)(fdmareq_RequestLines[i].OpCode    & 0x0f) <<  4;  /*  " 7...4 */
#if defined (ST_5525)
            ReqControl |= (U32)(fdmareq_RequestLines[i].RTF    & 0x01) << 12;  /*  " 12 " */
#endif
            ReqControl |= (U32)(fdmareq_RequestLines[i].Access    & 0x01) << 14;  /*  " 14 */
            ReqControl |= (U32)(fdmareq_RequestLines[i].Initiator & 0x03) << 22;  /*  " 23...22 */
            ReqControl |= (U32)((fdmareq_RequestLines[i].Count-1) & 0x1F) << 24;  /*  " 28...24 */
            ReqControl |= (U32)(fdmareq_RequestLines[i].Increment & 0x01) << 29;  /*  " 29 */
#if defined (ST_5105) || defined (ST_5107)
            ReqControl |= (U32)(fdmareq_RequestLines[i].PCMPBitField & 0x01) << 30;  /*  " 30 " */
#endif
#endif
            /* Write to device */
            SET_REQ_CONT(fdmareq_RequestLines[i].Index, ReqControl, DeviceNo);

#ifdef REQLINE_DEBUG
            STTBX_Print(("REQ ReqAddr: 0x%x, FullAddr: 0x%x, ReqNum %d : 0x%x\n",
                   (REQ_CONT_BASE + ((fdmareq_RequestLines[i].Index) * REQ_CONT_OFFSET)),
                   ((U32)ControlBlock_p->BaseAddress_p[DeviceNo] +
                    (REQ_CONT_BASE + ((fdmareq_RequestLines[i].Index) * REQ_CONT_OFFSET))),
                   fdmareq_RequestLines[i].Index, ReqControl));
#endif
        }
        /* Move to next request signal in table */
        i++;
    }

#if defined (ST_5525) || defined (ST_7200)
    stfdma_MuxConf();
#endif
}


#if defined(ST_5525) || defined (ST_7200)
void   stfdma_MuxConf(void)
{
#if defined (ST_5525)
    SET_MUX(STFDMA_REQUEST_SIGNAL_PCMP_0*4, PCM_PLAYER0_REQ);

    SET_MUX(STFDMA_REQUEST_SIGNAL_SWTS*4, SWTS0_REQ);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SWTS1*4, SWTS1_REQ);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SWTS2*4, SWTS2_REQ);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SWTS3*4, SWTS3_REQ);

    SET_MUX(STFDMA_REQUEST_SIGNAL_PCMP_1*4, PCM_PLAYER1_REQ);
    SET_MUX(STFDMA_REQUEST_SIGNAL_PCMP_2*4, PCM_PLAYER2_REQ);
    SET_MUX(STFDMA_REQUEST_SIGNAL_PCMP_3*4, PCM_PLAYER3_REQ);

    SET_MUX(STFDMA_REQUEST_SIGNAL_ANALOGIO_PCM_P*4, MODEM_PLAYER_REQ);

    SET_MUX(STFDMA_REQUEST_SIGNAL_PCMR_0*4, PCM_READER0_REQ);
    SET_MUX(STFDMA_REQUEST_SIGNAL_PCMR_1*4, PCM_READER1_REQ);

    SET_MUX(STFDMA_REQUEST_SIGNAL_ANALOGIO_PCM_R*4, MODEM_READER_REQ);

    SET_MUX(STFDMA_REQUEST_SIGNAL_DISEQTX_EMPTY*4, DISEQC_TX_HALF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_DISEQRX_FULL*4, DISEQC_RX_HALF_FULL);

    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC3_TX*4, SSC3_TX_BUFF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC2_TX*4, SSC2_TX_BUFF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC1_TX*4, SSC1_TX_BUFF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC0_TX*4, SSC0_TX_BUFF_EMPTY);

    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC3_RX*4, SSC3_RX_BUFF_FULL);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC2_RX*4, SSC2_RX_BUFF_FULL);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC1_RX*4, SSC1_RX_BUFF_FULL);
    SET_MUX(STFDMA_REQUEST_SIGNAL_SSC0_RX*4, SSC0_RX_BUFF_FULL);

    SET_MUX(STFDMA_REQUEST_SIGNAL_UART3_TX*4, UART3_TX_HALF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_UART2_TX*4, UART2_TX_HALF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_UART1_TX*4, UART1_TX_HALF_EMPTY);
    SET_MUX(STFDMA_REQUEST_SIGNAL_UART0_TX*4, UART0_TX_HALF_EMPTY);


    SET_MUX(STFDMA_REQUEST_SIGNAL_UART2_RX*4, UART2_RX_HALF_FULL);
    SET_MUX(STFDMA_REQUEST_SIGNAL_UART1_RX*4, UART1_RX_HALF_FULL);
    SET_MUX(STFDMA_REQUEST_SIGNAL_UART0_RX*4, UART0_RX_HALF_FULL);

    SET_MUX(STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO*4, SDPIF_PLAYER_REQ);

#elif defined (ST_7200)

    /* FDMA #0 */

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SCIF_RX                  | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_7);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SCIF_TX                  | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_8);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC0_RX                  | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_9);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC0_TX                  | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_14);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_CD_EXT1                  | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_28);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_AUDIO_SRC_INPUT          | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_47);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_AUDIO_SRC_OUTPUT         | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_48);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_CPXM_DECRYPTED_INPUT     | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_29);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_CPXM_DECRYPTED_OUTPUT    | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_30);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_CPXM_ENCRYPTED_INPUT     | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_31);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_CPXM_ENCRYPTED_OUTPUT    | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_32);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_PCM0                     | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_33);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_PCM1                     | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_34);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_PCM2                     | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_35);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_PCM3                     | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_36);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_PCMREADER                | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_37);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SPDIF_AUDIO              | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_38);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_HDMI_PCM_PLYR            | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_39);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_HDMI_SPDIF_PLYR          | ((U32)STFDMA_1 << 5)) * 4, FDMA_MUX_40);

    /* FDMA #1 */

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC1_RX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_10);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC2_RX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_11);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC3_RX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_12);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC4_RX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_13);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC1_TX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_15);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC2_TX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_16);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC3_TX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_17);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_SSC4_TX               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_18);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART0_RX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_19);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART1_RX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_20);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART2_RX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_21);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART3_RX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_22);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART0_TX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_23);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART1_TX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_24);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART2_TX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_25);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UART3_TX              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_26);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_CD_EXT0               | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_27);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UHF2_RXBUF_EMPTY      | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_45);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_UHF2_RXBUF_FULL       | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_46);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_MODEM_PCM_PLYR        | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_43);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_MODEM_PCM_READER      | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_44);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_DISEQC0_EMPTY         | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_3);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_DISEQC0_FULL          | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_4);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_DISEQC1_EMPTY         | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_5);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_DISEQC1_FULL          | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_6);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_HDMI_AVI_BUFF_EMPTY   | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_2);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_NAND_DATA             | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_49);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_NAND_CMD              | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_50);

    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_TTXT                  | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_41);
    SET_MUX(((U32)STFDMA_REQUEST_SIGNAL_TTXT_SDTVOUT          | ((U32)STFDMA_2 << 5)) * 4, FDMA_MUX_42);

#endif
}
#endif

