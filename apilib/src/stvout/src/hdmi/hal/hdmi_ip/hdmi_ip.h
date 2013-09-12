/*******************************************************************************
File name   : hdmi_ip.h

Description : HAL HDMI header file

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
25 Mar 2004        Created                                           DB
08 Apr 2004        Updated                                           AC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HDMI_IP_H
#define __HDMI_IP_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stevt.h"
#include "hal_hdmi.h"
#include "hdmi_reg.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif
/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* Private variables (static) ----------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */
#if defined(STVOUT_STATE_MACHINE_TRACE)
U32 PIO_Int_Time;
#endif

/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    ST_Partition_t *        CPUPartition_p;
    U8                      HPD_Bit;
    STPIO_Handle_t          PioHandle;
    BOOL                    IsHPDInversed;
    ST_DeviceName_t         PIODevice;
} HALHDMI_IP_Properties_t;

/* Private Macros ----------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
ST_ErrorCode_t Init            (const HAL_Handle_t  Handle);
ST_ErrorCode_t Term            (const HAL_Handle_t  Handle);
void HAL_PioIntHandler         (const STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
void HAL_ControlInterface      (const HAL_Handle_t Handle, BOOL Enable);
void HAL_SetInterfaceTiming    (const HAL_Handle_t Handle, HAL_TimingInterfaceParams_t const Params);
U32  HAL_GetInterruptStatus    (const HAL_Handle_t Handle);
void HAL_SetInterruptMask      (const HAL_Handle_t Handle, U32 Mask);
void HAL_EnableInterrupt       (const HAL_Handle_t Handle, U32 EnableInt);
void HAL_ClearInterrupt        (const HAL_Handle_t Handle, U32 ClearInt);
void HAL_SetOutputSize         (const HAL_Handle_t Handle,U32 XStart, U32 XStop, U32 YStart, U32 YStop);
void HAL_SetSyncPolarity       (const HAL_Handle_t Handle, BOOL Polarity);
void HAL_SetInputFormat        (const HAL_Handle_t Handle, HAL_OutputFormat_t const Format);
void HAL_SetOutputParams       (const HAL_Handle_t Handle, HAL_OutputParams_t const Params);
void HAL_SetAudioParams        (const HAL_Handle_t Handle, HAL_AudioParams_t const Params);
void HAL_SetAudioSamplePacket  (const HAL_Handle_t Handle, HAL_AudioSamplePacket_t SampleFlat);
void HAL_SetAudioRegeneration  (const HAL_Handle_t Handle, U32 AudN);
void HAL_SetGPConfig           (const HAL_Handle_t Handle, U32 GPConfig);
void HAL_ControlInfoFrame      (const HAL_Handle_t Handle, HAL_ControlParams_t CtlParams);
void HAL_ComputeInfoFrame      (const HAL_Handle_t Handle, U8 *FrameBuffer_p, U32* PacketWord_p, U32 BufferSize);
void HAL_SendInfoFrame         (const HAL_Handle_t Handle, U32* PacketWord_p);
void HAL_GetCapturedPixel      (const HAL_Handle_t Handle, U8 *chl0);
void HAL_AVMute                (const HAL_Handle_t Handle, BOOL Enable);
void HAL_GetHPDStatus          (const HAL_Handle_t Handle, BOOL * const ReceiverConnected);
void HAL_ResetEnable           (const HAL_Handle_t Handle);
void HAL_ResetDisable          (const HAL_Handle_t Handle);
void HAL_SetDefaultDataChannel (const HAL_Handle_t Handle,U8 chl0, U8 chl1, U8 chl2);
void HAL_SetChannelStatusBit   (const HAL_Handle_t Handle, HAL_AudioSamplingFreq_t SamplingFrequency);
#ifdef STVOUT_HDCP_PROTECTION
void  HAL_EnableDefaultOuput (const HAL_Handle_t Handle, BOOL IsHDCPEnable);
#endif
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HDMI_IP_H */

/* End of hdmi_ip.h */
/* ------------------------------- End of file ---------------------------- */

