/*****************************************************************************
File Name   : stclkrv.h

Description : API Interfaces for the Clock Recovery driver.

Copyright (C) 2007 STMicroelectronics

Reference   :

$ClearCase (VOB: stclkrv)

ST API Definition "Clock Recovery API" DVD-API-072 Release 5.0
*****************************************************************************/

/* Modified to include multi-instance support */


/* Define to prevent recursive inclusion */
#ifndef __STCLKRV_H
#define __STCLKRV_H

/* Includes --------------------------------------------------------------- */

#include "stcommon.h"

#ifndef STCLKRV_NO_PTI
#ifdef ST_5188
#include "stdemux.h"
#else
#include "stpti.h"
#endif
#endif /* STCLKRV_NO_PTI */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */

#define STCLKRV_DRIVER_ID               72
#define STCLKRV_DRIVER_BASE             ( STCLKRV_DRIVER_ID << 16 )

#define STCLKRV_GETREVISION              "STCLKRV-REL_5.4.0"

/* External return codes */
#define STCLKRV_ERROR_BASE              STCLKRV_DRIVER_BASE

#define STCLKRV_ERROR_HANDLER_INSTALL   ( STCLKRV_ERROR_BASE )
#define STCLKRV_ERROR_PCR_UNAVAILABLE   ( STCLKRV_ERROR_BASE + 1 )
#define STCLKRV_ERROR_EVT_REGISTER      ( STCLKRV_ERROR_BASE + 2 )
#define STCLKRV_ERROR_INVALID_FREQUENCY ( STCLKRV_ERROR_BASE + 3 )
#define STCLKRV_ERROR_INVALID_SLAVE     ( STCLKRV_ERROR_BASE + 4 )

#define STCLKRV_NB_REGISTERED_EVENTS    3      /* Number of events registered with STEVT */
#define STCLKRV_NB_SUBSCRIBED_EVENTS    0      /* Number of events subscribed to   */

/* General Init paramaters values */

/* Filter window size parameters, Defaults given for Satellite */

/* Larger the Window stable is the PWM, Slower the response */
#define STCLKRV_MIN_SAMPLE_THRESHOLD 3      /* Number of samples to recieve before PWM correction */
                                            /* Min Window For Satellite - 3 */
                                            /* Min Window For Terrestrial - 50 */
#define STCLKRV_MAX_WINDOW_SIZE      30     /* Maximum number samples in moving average window */
                                            /* Max Window For Satellite - 30 */
                                            /* Max Window For Terrestrial - 150 */

#define STCLKRV_PCR_DRIFT_THRES      200    /* For Satellite = 200 */
                                            /* For Terrestrial(1/8 gaurd interval) = 10000 */
                                            /* For Terrestrial(1/32 gaurd interval) = 2500 */
#define STCLKRV_PCR_MAX_GLITCH       2      /* number of bad PCRs pre re-basing */
                                            /* threshold for Error */
#if !defined(STCLKRV_NO_TBX)
#define STCLKRV_Print(x)          STTBX_Print(x);
#else
#define STCLKRV_Print(x)          printf x;
#endif

/* Exported Types --------------------------------------------------------- */

/* Application Modes */
#if defined (ST_7710)||defined(ST_7100)||defined(ST_7109)
typedef enum STCLKRV_ApplicationMode_e
{
    STCLKRV_APPLICATION_MODE_NORMAL,
    STCLKRV_APPLICATION_MODE_SD_ONLY

} STCLKRV_ApplicationMode_t;
#endif

/* Clock Sources */
typedef enum STCLKRV_Clock_e
{
    STCLKRV_CLOCK_SD_0,            /* Primary SD/pixel Clock on All chips */
    STCLKRV_CLOCK_SD_1,            /* Secondary SD/pixel Clock on 5525/7200 */
    STCLKRV_CLOCK_PCM_0,           /* Audio main PCM Clock on All chips */
    STCLKRV_CLOCK_PCM_1,           /* FS Audio PCM_1 Clock on 7100/5525/7200 */
    STCLKRV_CLOCK_PCM_2,           /* FS Audio PCM_2 Clock on 5525/7200 */
    STCLKRV_CLOCK_PCM_3,           /* FS Audio PCM_3 Clock on 5525/7200 */
    STCLKRV_CLOCK_SPDIF_HDMI_0,    /* FS Audio HDMI Clock on 7200 */
    STCLKRV_CLOCK_SPDIF_0,         /* Audio SPDIF 5100/5105/5301/5107/7100/5525/7200 (on 7710 spdif derived by PCM0) */
    STCLKRV_CLOCK_HD_0,            /* High Definition 7710/7100/7109/7200 */
    STCLKRV_CLOCK_HD_1             /* High Definition 7200 */

}STCLKRV_ClockSource_t;


/* Clock Decodes
   On 5525, Dual decode is possible */
typedef enum STCLKRV_Decode_e
{
    STCLKRV_DECODE_PRIMARY,        /* Primary decode by default on all chips */
    STCLKRV_DECODE_SECONDARY       /* Supported only on 5525 */
}STCLKRV_Decode_t;


/* PCR Source */
typedef enum STCLKRV_PCRSource_e
{
    STCLKRV_PCR_SOURCE_PTI

}STCLKRV_PCRSource_t;

/* STCLKRV STC source select values */
typedef enum STCLKRV_STCSource_e
{
    STCLKRV_STC_SOURCE_PCR,
    STCLKRV_STC_SOURCE_BASELINE

}STCLKRV_STCSource_t;

#ifndef STCLKRV_NO_PTI
/* PCR Source parameters type */
typedef struct STCLKRV_SourceParams_s
{
    STCLKRV_PCRSource_t Source;
    union
    {
        struct
        {
#ifdef ST_5188
            STDEMUX_Slot_t Slot;
#else
            STPTI_Slot_t Slot;
#endif
        }STPTI_s;
    }Source_u;

}STCLKRV_SourceParams_t;
#endif /* NO PTI */

/* Event Types */
typedef enum STCLKRV_Event_s
{
    STCLKRV_PCR_VALID_EVT          = STCLKRV_DRIVER_BASE,
    STCLKRV_PCR_DISCONTINUITY_EVT  = STCLKRV_DRIVER_BASE + 1,
    STCLKRV_PCR_GLITCH_EVT         = STCLKRV_DRIVER_BASE + 2

} STCLKRV_Event_t;

/*=========================================================
  5100,5301 and 5105,5107 device type = STCLKRV_DEVICE_TYPE_5100
  7710 device type = STCLKRV_DEVICE_TYPE_7710
  7100, 7109 device type = STCLKRV_DEVICE_TYPE_7100
  5525 device type = STCLKRV_DEVICE_TYPE_5525
  7200 device type = STCLKRV_DEVICE_TYPE_7200
  =========================================================*/
/* STCLKRV Device type */
typedef enum STCLKRV_Device_e
{
    STCLKRV_DEVICE_TYPE_5100,               /* SD/PCM/SPDIF and clkrv tracking registers in clkgen block */
    STCLKRV_DEVICE_TYPE_7710,               /* SD/HD clkrv tracking registers in clkgen block,PCM clk in ADSC, PTI(STC) access */
    STCLKRV_DEVICE_TYPE_7100,               /* SD/HD clkrv tracking registers in clkgen block,PCM clk in AUDCFG, PTI(STC) access */
    STCLKRV_DEVICE_TYPE_5525,               /* Two decodes, Primary/Secondary */
    STCLKRV_DEVICE_TYPE_7200,               /* Two decodes, Primary/Secondary */
    STCLKRV_DEVICE_TYPE_BASELINE_ONLY
}STCLKRV_Device_t;

/* Clock recovery init params */
typedef struct STCLKRV_InitParams_s
{
    STCLKRV_Device_t    DeviceType;
    ST_Partition_t      *Partition_p;       /* Memory partition to use (mi) */

    /* STPTI & Clock recovery Events */
    ST_DeviceName_t     EVTDeviceName;      /* Clkrv event handler name */
    ST_DeviceName_t     PCREvtHandlerName;  /* PCR Event handler name (STPTI) */
    ST_DeviceName_t     PTIDeviceName;      /* To get access to PTI STC timer */

    /* Clock recovery Filter */
    U32                 PCRMaxGlitch;       /* 2 suggested */
    U32                 PCRDriftThres;      /* 200 suggested for SaTellite */
    U32                 MinSampleThres;     /* Minimum number of PCRs to average, 3 suggested (Satellite) */
    U32                 MaxWindowSize;      /* Maximum number of PCRs to average, 30 suggested (Satellite)*/

#ifdef ST_OS21                              /* Clock recovery module HW interrupt number */
    interrupt_name_t    InterruptNumber;
#else
    U32                 InterruptNumber;
#endif
    U32                 InterruptLevel;     /* Clock recovery module HW interrupt number */

    /* device type dependant parameters */
    void                *FSBaseAddress_p;   /* SD/PCM/SPDIF or HD clock FS and clkrv tracking registers 5100/7710 */
#if defined(ST_7100)||defined(ST_7109)||defined(ST_7200)
    void                *AUDCFGBaseAddress_p; /* Audio clocks FS registers base address */
#else
    void                *ADSCBaseAddress_p; /* Audio clocks FS registers base address in 7710 only */
#endif
    /* On 5525.7200 there could be Primary/Secondary decode, Default 0 is primary */
    STCLKRV_Decode_t    DecodeType;
    void                *CRUBaseAddress_p;    /* Clock recovery unit base address for 7200 */

} STCLKRV_InitParams_t;

typedef struct STCLKRV_TermParams_s
{
    BOOL ForceTerminate;
} STCLKRV_TermParams_t;


typedef struct STCLKRV_ExtendedSTC_s
{
    U32 BaseBit32;        /* Bit 32 of the STC */
    U32 BaseValue;        /* Bits 31..0 of the STC in 90KHz units */
    U32 Extension;        /* 27MHz portion of the STC (modulo 300) */
} STCLKRV_ExtendedSTC_t;


typedef void* STCLKRV_OpenParams_t;         /* none required presently */
typedef U32  STCLKRV_Handle_t;


/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

#define STCLKRV_GetDecodeClk( a, b )    STCLKRV_GetSTC( (a), (b) )

/* Exported Functions ----------------------------------------------------- */

ST_Revision_t STCLKRV_GetRevision( void );

ST_ErrorCode_t STCLKRV_Init( const ST_DeviceName_t      Name,
                             const STCLKRV_InitParams_t *InitParams );

ST_ErrorCode_t STCLKRV_Open( const ST_DeviceName_t      Name,
                             const STCLKRV_OpenParams_t *OpenParams,
                             STCLKRV_Handle_t           *Handle );



#if defined (ST_7710) || defined(ST_7100) || defined(ST_7109)
ST_ErrorCode_t STCLKRV_SetApplicationMode( STCLKRV_Handle_t    Handle,
                                           STCLKRV_ApplicationMode_t AppMode);
#endif

ST_ErrorCode_t STCLKRV_SetNominalFreq( STCLKRV_Handle_t      Handle,
                                       STCLKRV_ClockSource_t ClockSource,
                                       U32                   Frequency );

ST_ErrorCode_t STCLKRV_Close( STCLKRV_Handle_t Handle );

ST_ErrorCode_t STCLKRV_Term( const ST_DeviceName_t Name,
                             const STCLKRV_TermParams_t *TermParams );

#ifndef STCLKRV_NO_PTI

ST_ErrorCode_t STCLKRV_Enable( STCLKRV_Handle_t Handle );

ST_ErrorCode_t STCLKRV_SetSTCSource( STCLKRV_Handle_t    Handle,
                                     STCLKRV_STCSource_t STCSource);

ST_ErrorCode_t STCLKRV_SetPCRSource( STCLKRV_Handle_t       ClkHandle,
                                     STCLKRV_SourceParams_t *PCRSource);

ST_ErrorCode_t STCLKRV_SetSTCBaseline( STCLKRV_Handle_t      Handle,
                                       STCLKRV_ExtendedSTC_t *STC);

ST_ErrorCode_t STCLKRV_SetSTCOffset( STCLKRV_Handle_t      Handle,
                                     S32 STCOffset);

ST_ErrorCode_t STCLKRV_InvDecodeClk( STCLKRV_Handle_t Handle );

ST_ErrorCode_t STCLKRV_Disable( STCLKRV_Handle_t Handle);

ST_ErrorCode_t STCLKRV_GetSTC( STCLKRV_Handle_t Handle,
                               U32 *STC );

ST_ErrorCode_t STCLKRV_GetExtendedSTC( STCLKRV_Handle_t Handle,
                                       STCLKRV_ExtendedSTC_t *ExtendedSTC );

#endif /* NO PTI */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STCLKRV_H */


/* End of stclkrv.h */
