/*******************************************************************************
File name   : hal_hdmi.h

Description : VOUT driver header for hdmi ip

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
3 Apr 2004         Created                                          AC

*******************************************************************************/

#ifndef __HAL_HDMI_H
#define __HAL_HDMI_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stevt.h"
#include "vout_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
typedef void * HAL_Handle_t;

typedef enum HAL_TmdsClock_e
{
    TMDS_CLOCK_2X,
    TMDS_CLOCK_4X
}HAL_TmdsClock_t;

typedef enum HAL_OutputFormat_e
{
    HDMI_RGB888,
    HDMI_YCBCR444,
    HDMI_YCBCR422
}HAL_OutputFormat_t;

typedef enum HAL_MaxDelay_e
{
    MAX_DELAY_DEFAULT_VALUE,
    MAX_DELAY_16_VSYNCS,
    MAX_DELAY_1_VSYNCS
}HAL_MaxDelay_t;

typedef enum HAL_MinExts_e
{
    MIN_EXTS_DEFAULT_VALUE,
    MIN_EXTS_CTL_PERIOD

}HAL_MinExts_t;

typedef enum Hal_FiFoOverrunStatus_e
{
 FIFO_0_OVERRUN = 1,
 FIFO_1_OVERRUN = 2,
 FIFO_2_OVERRUN = 4,
 FIFO_3_OVERRUN = 8
}Hal_FiFoOverrunStatus_t;

typedef enum HAL_ConfigRatio_e
{
  SD_DllCONFIG_RATIO = 1,
  HD_DllCONFIG_RATIO = 2
}HAL_DllConfigRatio_t;

typedef enum HAL_AudioSamplePacket_e
{
  SAMPLE_FLAT_BIT0 = 1,
  SAMPLE_FLAT_BIT1 = 2,
  SAMPLE_FLAT_BIT2 = 4,
  SAMPLE_FLAT_BIT3 = 8
}HAL_AudioSamplePacket_t;

typedef enum HAL_CipherType_e
{
    HAL_PSEUDO_RANDOM_AN,             /* The pseudo random Key */
    HAL_SHARED_SECRET_VALUE_KM,       /* The shared secret Key */
    HAL_BLOCK_CIPHER_VALUES,          /* Ks, Mi and Ri Keys*/
    HAL_BLOCK_CIPHER_VALUES_RPT,      /* Ks, Mi and Ri Keys when the device receiver is a hdcp repeater*/
    HAL_KEY_SELECTION_VECTOR_KSV      /* The secret device Key set: KSV */
}HAL_CipherType_t;

typedef struct {

    ST_Partition_t *        CPUPartition_p;  /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t          EventsHandle;
    STVOUT_DeviceType_t     DeviceType;
    STVOUT_OutputType_t     OutputType;
    void *                  DeviceBaseAddress_p;     /* Device base addresss of DVI/HDMI Cell */
    void *                  RegisterBaseAddress_p;   /* Base address of the HALHDMI registers */
    void *                  SecondBaseAddress_p;     /* Base address of HDCP cell*/
    STPIO_BitConfig_t       HPD_Bit;
    BOOL                    IsHPDInversed;
    ST_DeviceName_t         PIODevice;
    BOOL                    HSyncActivePolarity;   /* It is TRUE when HSync is High and FALSE when it is low*/
    BOOL                    VSyncActivePolarity;   /* It is TRUE when VSync is High and FALSE when it is low*/
} HALHDMI_InitParams_t;

typedef struct {

    ST_Partition_t *        CPUPartition_p;       /* Where the module can allocate memory for its internal usage */
    STEVT_Handle_t          EventsHandle;
    STVOUT_DeviceType_t     DeviceType;
    STVOUT_OutputType_t     OutputType;
    void *                  DeviceBaseAddress_p;      /* Device base addresss of DVI/HDMI Cell */
    void *                  RegisterBaseAddress_p;    /* Base address of the HALHDMI registers */
    void *                  SecondBaseAddress_p;      /* Base address of HDCP cell*/
    U32                     ValidityCheck;
    BOOL                    HSyncActivePolarity;
    BOOL                    VSyncActivePolarity;
    STPIO_BitConfig_t       HPD_Bit;
    BOOL                    IsHPDInversed;
    ST_DeviceName_t         PIODevice;
    void*                   PrivateData_p;

} HALHDMI_Properties_t;

typedef struct HAL_TimingInterfaceParams_s
{
    HAL_TmdsClock_t       BCHClockRatio;     /* BCH Clock ratio*/
    HAL_DllConfigRatio_t  DllConfig ;       /* Configuration between Dll Clock and TMDS clock*/
    BOOL                  EnaPixrepetition;  /* Enable/Disable the pixel repetition by 2*/
}HAL_TimingInterfaceParams_t;

typedef struct HAL_OutputParams_s
{
    BOOL  DVINotHDMI;
    BOOL  HDCPEnable;  /* it is a variable for debug*/
    BOOL  EssNotOess;
}HAL_OutputParams_t;

typedef struct HAL_AudioParams_s
{
    U32   Numberofchannel;
    U32   SpdifDiv;
    Hal_FiFoOverrunStatus_t  ClearFiFos;
}HAL_AudioParams_t;

typedef struct HAL_ControlParams_s
{
    BOOL                 IsControlEnable;     /* Enable genaral control packet Xmission */
    BOOL                 IsBufferUsed;        /* If this flag was set, the data in the buffer are transmitted */
    BOOL                 IsAVMute;            /* The status in the AVMute was sent in general control packet when buffer_not_reg is reset*/
    HAL_MaxDelay_t       MaxDelay;            /* Max delay between extended control periods */
    HAL_MinExts_t        MinExts;             /* Specifies the minimum duration of extented control in TMDS Clocks*/
}HAL_ControlParams_t;

typedef struct HAL_Cipher_s
{
    U32 An0;
    U32 An1;
    U32 KsMSB;
    U32 KsLSB;
    U32 MiMSB;
    U32 MiLSB;
    U32 KSV_0;
    U32 KSV_1;
    U32 Ri;
 }HAL_Cipher_t;

typedef enum HAL_AudioSamplingFreq_e
{
    SAMPLING_FREQ_32_KHZ,
    SAMPLING_FREQ_44_1_KHZ,
    SAMPLING_FREQ_48_KHZ,
    SAMPLING_FREQ_LAST
} HAL_AudioSamplingFreq_t;

typedef enum HAL_PixelClockFreq_e
{
    PIXEL_CLOCK_25200,
    PIXEL_CLOCK_25174,
    PIXEL_CLOCK_27000,
    PIXEL_CLOCK_27027,
    PIXEL_CLOCK_54000,
    PIXEL_CLOCK_54054,
    PIXEL_CLOCK_74176,
    PIXEL_CLOCK_74250,
    PIXEL_CLOCK_148361,
    PIXEL_CLOCK_148500,
    PIXEL_CLOCK_LAST
} HAL_PixelClockFreq_t;

typedef enum HAL_VsyncFreq_e
{
    VSYNC_FREQ_60,
    VSYNC_FREQ_50,
    VSYNC_FREQ_30,
	VSYNC_FREQ_25,
    VSYNC_FREQ_24,
    VSYNC_FREQ_LAST
} HAL_VsyncFreq_t;

typedef struct HAL_ACR_s
{
    U32  N;
    U32  CTS;
} HAL_ACR_t;



/* Private Macros --------------------------------------------------------- */

/* Passing (HALHDMI_Properties_t), returns TRUE if the handle is valid, FALSE otherwise  */
#define IsHalValidHandle(Handle)     (((HALHDMI_Properties_t*)Handle)->ValidityCheck== HAL_VALID_HANDLE)

/* Exported Functions ----------------------------------------------------- */
ST_ErrorCode_t HAL_Init                  (HALHDMI_InitParams_t* const HDMIInitParams,
                                          HAL_Handle_t*  const Handle_p);
ST_ErrorCode_t HAL_Term                  (HAL_Handle_t const Handle);
ST_ErrorCode_t HAL_Open                  (HAL_Handle_t const Handle);
ST_ErrorCode_t HAL_Close                 (HAL_Handle_t const Handle);
ST_ErrorCode_t HAL_GenControlInfoFrame   (HAL_Handle_t const Handle);
ST_ErrorCode_t HAL_Enable                (HAL_Handle_t const Handle);
ST_ErrorCode_t HAL_Disable               (HAL_Handle_t const Handle);
BOOL           HAL_IsReceiverConnected   (HAL_Handle_t const Handle);
ST_ErrorCode_t Hal_SendDefaultData       (HAL_Handle_t const Handle, U8 Chl0, U8 Chl1, U8 Chl2);
ST_ErrorCode_t  HAL_SetAudioNValue       (HAL_Handle_t const Handle, HAL_PixelClockFreq_t PixelFreq,
                                          HAL_AudioSamplingFreq_t SamplingFreq, HAL_VsyncFreq_t VSyncFreq);
ST_ErrorCode_t  HAL_SetACRPacket         (HAL_Handle_t const Handle, const HAL_PixelClockFreq_t EnumPixClock,  \
                                          const HAL_AudioSamplingFreq_t  EnumAudioFreq, HAL_ACR_t * ACR);

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_HDMI_H */

