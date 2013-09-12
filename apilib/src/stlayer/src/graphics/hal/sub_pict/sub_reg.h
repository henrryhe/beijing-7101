/*******************************************************************************

File name   : sub_reg.h

Description : register header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
18 Dec 2000        Created                                           TM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __SUB_REG_H
#define __SUB_REG_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sub_vp.h"
#include "sub_init.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Control register 1 */
#define LAYERSUB_SPD_CTL1_OFFSET               0
#define LAYERSUB_SPD_CTL1_MASK                 0xFF

#define LAYERSUB_SPD_CTL1_S                    1
#define LAYERSUB_SPD_CTL1_D                    2
#define LAYERSUB_SPD_CTL1_V                    4
#define LAYERSUB_SPD_CTL1_H                    8
#define LAYERSUB_SPD_CTL1_B                    16
#define LAYERSUB_SPD_CTL1_P                    32

#define LAYERSUB_SPD_CTL1_BOT_TOP_MASK         3
#define LAYERSUB_SPD_CTL1_BOT_TOP_SHIFT        6
#define LAYERSUB_SPD_CTL1_ANY_FIELD            0
#define LAYERSUB_SPD_CTL1_TOP_FIELD            1
#define LAYERSUB_SPD_CTL1_BOT_FIELD            2

/* Control register 2 */
#define LAYERSUB_SPD_CTL2_OFFSET               0x2
#define LAYERSUB_SPD_CTL2_RC                   1

/* Highlight region contrast */

/* Highlight region color */

/* Highlight region end X */

/* Highlight region end Y */

/* Highlight region Start X */

/* Highlight region Start Y */

/* Main lookup table */
#define LAYERSUB_SPD_LUT_OFFSET                0x3
#define LAYERSUB_SPD_LUT_MASK                  0xFF

/* Soft reset */
#define LAYERSUB_SPD_SPR_OFFSET                0x1
#define LAYERSUB_SPD_SPR                       1

/* Sub picture display area SXD0 */
#define LAYERSUB_SPD_SXDO_LESS_BIT_OFFSET      0x25
#define LAYERSUB_SPD_SXDO_LESS_BIT_MASK        0xFF

#define LAYERSUB_SPD_SXDO_MOST_BIT_OFFSET      0x24
#define LAYERSUB_SPD_SXDO_MOST_BIT_MASK        0x3

/* Sub picture display area SXD1 */
#define LAYERSUB_SPD_SXD1_LESS_BIT_OFFSET      0x29
#define LAYERSUB_SPD_SXD1_LESS_BIT_MASK        0xFF

#define LAYERSUB_SPD_SXD1_MOST_BIT_OFFSET      0x28
#define LAYERSUB_SPD_SXD1_MOST_BIT_MASK        0x3

/* Sub picture display area SYD0 */
#define LAYERSUB_SPD_SYDO_LESS_BIT_OFFSET      0x27
#define LAYERSUB_SPD_SYDO_LESS_BIT_MASK        0xFF

#define LAYERSUB_SPD_SYDO_MOST_BIT_OFFSET      0x26
#define LAYERSUB_SPD_SYDO_MOST_BIT_MASK        0x3

/* Sub picture display area SYD1 */
#define LAYERSUB_SPD_SYD1_LESS_BIT_OFFSET      0x2B
#define LAYERSUB_SPD_SYD1_LESS_BIT_MASK        0xFF

#define LAYERSUB_SPD_SYD1_MOST_BIT_OFFSET      0x2A
#define LAYERSUB_SPD_SYD1_MOST_BIT_MASK        0x3

/* Sub picture X offset XDO*/
#define LAYERSUB_SPD_XDO_LESS_BIT_OFFSET       0x5
#define LAYERSUB_SPD_XDO_LESS_BIT_MASK         0xFF

#define LAYERSUB_SPD_XD0_MOST_BIT_OFFSET       0x4
#define LAYERSUB_SPD_XD0_MOST_BIT_MASK         0x3

/* Sub picture Y offset YDO*/
#define LAYERSUB_SPD_YDO_LESS_BIT_OFFSET       0x7
#define LAYERSUB_SPD_YDO_LESS_BIT_MASK         0xFF

#define LAYERSUB_SPD_YDO_MOST_BIT_OFFSET       0x6
#define LAYERSUB_SPD_YDO_MOST_BIT_MASK         0x3



/* Video registers */
#define LAYERSUB_VID_BASE_ADDRESS             0

/* Sub picture buffer begin */
#define LAYERSUB_VID_SPB_LESS_OFFSET           0x51
#define LAYERSUB_VID_SPB_LESS_MASK             0xFF

#define LAYERSUB_VID_SPB_MOST_OFFSET           0x50
#define LAYERSUB_VID_SPB_MOST_MASK             0x7

/* Sub picture buffer end */
#define LAYERSUB_VID_SPE_LESS_OFFSET           0x53
#define LAYERSUB_VID_SPE_LESS_MASK             0xFF

#define LAYERSUB_VID_SPE_MOST_OFFSET           0x52
#define LAYERSUB_VID_SPE_MOST_MASK             0x7

/* Sub picture read pointer */
#define LAYERSUB_VID_SPREAD_OFFSET             0x4E
#define LAYERSUB_VID_SPREAD_FIRST_CYCLE_MASK   0x7
#define LAYERSUB_VID_SPREAD_SECOND_CYCLE_MASK  0xFF
#define LAYERSUB_VID_SPREAD_THIRD_CYCLE_MASK   0xFF

/* Sub picture write pointer */
#define LAYERSUB_VID_SPWRITE_OFFSET            0x4F
#define LAYERSUB_VID_SPWRITE_FIRST_CYCLE_MASK  0x7
#define LAYERSUB_VID_SPWRITE_SECOND_CYCLE_MASK 0xFF
#define LAYERSUB_VID_SPWRITE_THIRD_CYCLE_MASK  0xFF


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t layersub_SetPalette(layersub_Device_t* Device_p);
ST_ErrorCode_t layersub_SetDisplayArea(layersub_Device_t* Device_p);
ST_ErrorCode_t layersub_SetXDOYDO(layersub_Device_t* Device_p);
ST_ErrorCode_t layersub_SetBufferPointer(layersub_Device_t* Device_p);
ST_ErrorCode_t layersub_StartSPU(layersub_Device_t* Device_p);
ST_ErrorCode_t layersub_StopSPU(layersub_Device_t* Device_p);
void layersub_ReceiveEvtVSync (STEVT_CallReason_t      Reason,
                               const ST_DeviceName_t  RegistrantName,
                               STEVT_EventConstant_t  Event,
                               const void*            EventData_p,
                               const void*            SubscriberData_p);
void layersub_ProcessEvtVSync (void* data_p);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __SUB_REG_H */

/* End of sub_reg.h */


