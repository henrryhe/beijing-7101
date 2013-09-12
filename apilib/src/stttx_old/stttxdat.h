/*****************************************************************************
File Name   : stttxdat.h

Description : Hardware Access Offsets for the Teletext driver.

Copyright (C) 2005 STMicroelectronics

Reference   :

$ClearCase (VOB: stttx)

STi5510 "Set Top Box Backend Decoder with Integrated Host Processor"
(June 1998  Preliminary Data)

ST API Definition "Teletext Driver API" DVD-API-003 Revision 3.4
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STTTXDAT_H
#define __STTTXDAT_H

/* Includes --------------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Note that for ALL hardware accesses,
   U32* addressing is used.  Thus offsets
   are a quarter of the size of the byte
   addresses specified in the hardware
   documentation.                          */

#define STTTX_DMA_ADDRESS           0
#define STTTX_DMA_COUNT             1
#define STTTX_OUT_DELAY             2
#define STTTX_OUT_ENABLE            3
#define STTTX_IN_CB_DELAY           4
#define STTTX_MODE                  5
#define STTTX_INT_STATUS            6
#define STTTX_INT_ENABLE            7
#define STTTX_ACK_ODD_EVEN          8
#define STTTX_ABORT                 9

/* bit & field definitions */

#define STTTX_DISABLE_INTS          0x00            /* b0 of STTTX_INT_ENABLE */
#define STTTX_STATUS_COMPLETE       0x01            /* STTTX_INT_STATUS */
#define STTTX_OUTPUT_ENABLED        0x00            /* b0 of STTTX_MODE */
#define STTTX_EVEN_FIELD_FLAG       0x00            /* b1 of STTTX_MODE */
#define STTTX_ODD_FIELD_FLAG        0x02            /* b1 of STTTX_MODE */
#if defined(ST_5510)
#define STTTX_DMA_DELAY_TIME        0x04            /* STTTX_OUT_DELAY plus 2 */
#else
#define STTTX_DMA_DELAY_TIME        0x02
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */


#endif /* #ifndef __STTTXDAT_H */

/* End of stttxdat.h */

