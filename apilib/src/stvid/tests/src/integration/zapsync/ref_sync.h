/*******************************************************************************

File name   : ref_sync.h

Description : video sync reference file
              Linux file ONLY

COPYRIGHT (C) STMicroelectronics 2006

Note : 

Date               Modification                                     Name
----               ------------                                     ----
23 Jan  2005        Created                                          LM

*******************************************************************************/

/* Assumptions :

   Only one PTI device
   Two video slots
   One audio slot
   7100 Rev B Board
   DVD_TRANSPORT_STPTI4
   DVB
   STVOUT_OUTPUT_ANALOG_CVBS
   Single decode (MPEG or H264), Single display (MAIN) CVBS
   PAL or NTSC format
   VID1 ONLY
   LMI_VID used                                                                              
   
*/

/* Includes ----------------------------------------------------------------- */

#include "stos.h"

#include "stevt.h"
#include "stpti.h"
#include "stclkrv.h"
#include "staudlx.h"
#include "stmerge.h"
#include "stdenc.h"
#include "stvout.h"
#include "stvmix.h"
#include "stlayer.h"
#include "stvid.h" /* contains 7100.h */
#include "stvidtest.h"

/* Defines ------------------------------------------------------------------ */

/* -*- 7100 -*- */
/* Interface Config - really the base address for all audio register access */

/* -*- EVT -*- */
#define STEVT_DEVICE_NAME                       "EVT"
#define EVT_EVENT_MAX_NUM   50
#define EVT_CONNECT_MAX_NUM 50
#define EVT_SUBSCR_MAX_NUM  50

/* -*- PTI -*- */
#define STPTI_DEVICE_NAME                     "PTI"

#define PTICMD_BASE_VIDEO      (0)
#define PTICMD_BASE_AUDIO      (PTICMD_MAX_VIDEO_SLOT)
#define PTICMD_BASE_PCR        (PTICMD_MAX_VIDEO_SLOT+PTICMD_MAX_AUDIO_SLOT)

#define PTICMD_MAX_VIDEO_SLOT  2
#define PTICMD_MAX_AUDIO_SLOT  1
#define PTICMD_MAX_PCR_SLOT    1

#define PTICMD_MAX_SLOTS (PTICMD_MAX_VIDEO_SLOT+PTICMD_MAX_AUDIO_SLOT+PTICMD_MAX_PCR_SLOT)

#define MIN_PCR_STCLKRV_STATE_PCR_OK    3

#define ST7100_PTI_INTERRUPT                  160

/* Maximum duration to wait for STCLKRV_PCR_DISCONTINUITY_EVT = 2 PCR < 100ms */
#define PTI_STCLKRV_PCR_DISCONTINUITY_TIMEOUT   (ST_GetClocksPerSecond() * 100/1000)
/* Maximum duration to wait for STCLKRV_PCR_VALID_EVT = 2s */
#define PTI_STCLKRV_PCR_VALID_TIMEOUT           (ST_GetClocksPerSecond() * 2)

/* -*- MERGE -*- */

/* -*- CLKRV -*- */
#define STCLKRV_DEVICE_NAME                   "CLKRV"
#define ST7100_MPEG_CLK_REC_INTERRUPT         138 /*dcxo*/
#define STANDARD_STC_SHIFT                    (-7200) /* 80ms */

/* -*- AUDLX -*- */
#define STAUD_DEVICE_NAME                     "AUDIO"
/* Maximum duration to wait for STAUD_IN_SYNC_EVT = 3s */
#define AUD_STAUDLX_AUD_VALID_TIMEOUT         (ST_GetClocksPerSecond() * 3)

#define STDENC_DEVICE_NAME                    "DENC"
#define STVOUT_DEVICE_NAME                    "VOUT"
#define STVOUT_DEVICE_NAME_HD                 "VOUT-HD"

/* -*- VTG -*- */
#define STVTG_DEVICE_NAME                     "VTG"
#define ST7100_VTG_0_INTERRUPT                154

/* -*- VMIX -*- */
#define MAX_VOUT                              6
#define STVMIX_DEVICE_NAME                    "VMIX"

/* -*- LAYER -*- */
#define MAX_LAYER                             10
#define STLAYER_GFX_DEVICE_NAME               "LAYGFX"
/*#define STLAYER_VID_DEVICE_NAME               "LAYVID"*/
#define LAYER_DEVICE_TYPE                     "GAMMA_GDP"
#define LAYER_MAX_VIEWPORT                    4
#define LAYER_MAX_OPEN_HANDLES                2
#define STLAYER_DEVICE_NAME                   "LAYER"

/* -*- VID -*- */
#define STVID_DEVICE_NAME                     "VID1"
#define ST7100_DELTA_BASE_ADDRESS             0xB9214000
#define ST7100_DELTA_PP0_INTERRUPT_NUMBER     ST7100_VID_DPHI_PRE0_INTERRUPT

/* -*- VIDTEST -*- */
#define STVIDTEST_DEVICE_NAME                 "VIDTEST"

/* Enum --------------------------------------------------------------------  */
/* -*- PTI -*- */
enum
{
   PTI_STCLKRV_STATE_PCR_OK = 0,
   PTI_STCLKRV_STATE_PCR_INVALID,
   PTI_STCLKRV_STATE_PCR_GLITCH
};

/* -*- AUDLX -*- */
enum
{
   AUD_STAUDLX_STATE_IN_SYNC = 0,
   AUD_STAUDLX_STATE_OUT_OF_SYNC
};

typedef struct {
    BOOL Initialized;
    BOOL InternalPLL;
    U32  DACClockToFsRatio;
    STAUD_PCMDataConf_t DACConfiguration;
} LocalDACConfiguration_t;

/* -*- REF -*- */
typedef enum REF_Format_s
{
    STREAM_FORMAT_PAL,
    STREAM_FORMAT_NTSC,
    STREAM_FORMAT_HD1080I,
    STREAM_FORMAT_HD720P
} REF_Format_e;

typedef enum REF_Codec_s
{
    STREAM_CODEC_MPEG,
    STREAM_CODEC_H264
} REF_Codec_e;

typedef unsigned int ProgId_t; /* pid_t defined as int for pti/link, but U16 for spti/stpti4 */

/* Enum --------------------------------------------------------------------  */

/* Compilation switch ------------------------------------------------------- */

/* Function prototypes ---------------------------------------------- */
BOOL startup(void);
BOOL sub_start(int VideoPID, int AudioPID, int PcrPID);
BOOL channel_change(int NewVideoPID, int NewAudioPID, int NewPcrPID);
BOOL sub_stop(void);
void enddown(void);

void CallBack_STCLKRV_PCR_VALID_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p);
void CallBack_STCLKRV_PCR_DISCONTINUITY_EVT(
                            STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p);


/* End of ref_sync.h */
