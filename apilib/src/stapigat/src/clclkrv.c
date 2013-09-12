/*******************************************************************************

File name   : clclkrv.c

Description : CLKRV configuration initialisation source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
30 Apr 2002        Created                                           BS
11 Oct 2002        Add support for mb382 (5517)                      HS
14 May 2003        Upgrade to STCLKRV 4.0.0                          HS
16 Sep 2004        Add support for ST40/OS21                         MH
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */


#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#endif

#ifdef ST_OS20
#include <debug.h>
#endif /* ST_OS20 */

#ifdef ST_OS21
#include <os21debug.h>
#endif /* ST_OS21 */

#include "testcfg.h"

#ifdef USE_CLKRV

#include "stddefs.h"
#include "stdevice.h"


#ifdef ST_OSLINUX
#include "compat.h"
/*#else*/
#endif

#include "sttbx.h"
#include "clevt.h"
#include "clpwm.h"

#include "stclkrv.h"
#include "clclkrv.h"

#ifdef USE_PTI
# include "pti_cmd.h"
#endif

#ifdef USE_DEMUX
# include "demux_cmd.h"
#endif

/* Private Types ------------------------------------------------------------ */

/* Private Constants -------------------------------------------------------- */

/* PWM channel to use with Clock recovery to adjust the PTI 27MHz */
#if defined(mb231) ||  defined(mb282b) || defined(mb295) || defined(mb314) \
 || defined(mb361) || defined(mb382) || defined(mb290) || defined (mb376)  \
 || defined (espresso)
#define CLKRV_VCXO_CHANNEL  STPWM_CHANNEL_0

#elif defined(mb275) || defined(mb275_64)
#define CLKRV_VCXO_CHANNEL  STPWM_CHANNEL_1

#elif defined(mb391)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_7710
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* USE_PTI */
#define CLKRV_INTERRUPT_NUMBER  ST7710_MPEG_CLK_REC_INTERRUPT
#define ADSC_BASE_ADDRESS       ST7710_AUDIO_BASE_ADDRESS;

#elif defined(mb390)
#if defined(ST_5100)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5100
#define PTI_DEVICE_NAME         "" /* Not needed */
#define CLKRV_INTERRUPT_NUMBER  ST5100_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL
#elif defined(ST_5301)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5100
#define PTI_DEVICE_NAME         "" /* Not needed */
#define CLKRV_INTERRUPT_NUMBER  ST5301_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL
#else /* !ST_5100&&!ST_5301 */
#error chip not defined for the platform mb390 !
#endif /* ST_5100 */

#elif defined(mb400)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5100
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* USE_PTI */
#define CLKRV_INTERRUPT_NUMBER  ST5105_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL

#elif defined(mb436) || defined(SAT5107)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5100
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* USE_PTI */
#define CLKRV_INTERRUPT_NUMBER  ST5107_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL

#elif defined(maly3s)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5100
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* USE_PTI */
#define CLKRV_INTERRUPT_NUMBER  ST5105_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL

#elif defined(mb457)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5100
#ifdef USE_DEMUX
#define PTI_DEVICE_NAME         STDEMUX_DEVICE_NAME
#else /* !USE_DEMUX */
#define PTI_DEVICE_NAME         ""
#endif /* USE_DEMUX */
#define CLKRV_INTERRUPT_NUMBER  ST5188_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL

#elif defined(mb428)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_5525
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* USE_PTI */
#define CLKRV_INTERRUPT_NUMBER  ST5525_DCO_CORRECTION_INTERRUPT
#define ADSC_BASE_ADDRESS       NULL

#elif defined(mb411)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_7100
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* USE_PTI */
#if defined (ST_7100)
#define CLKRV_INTERRUPT_NUMBER  ST7100_MPEG_CLK_REC_INTERRUPT
#elif defined (ST_7109)
#define CLKRV_INTERRUPT_NUMBER  ST7109_MPEG_CLK_REC_INTERRUPT
#endif

#elif defined(mb519)
#define CLKRV_DEVICE_TYPE       STCLKRV_DEVICE_TYPE_7200
#ifdef USE_PTI
#define PTI_DEVICE_NAME         STPTI_DEVICE_NAME
#else /* !USE_PTI */
#define PTI_DEVICE_NAME         ""
#endif /* !USE_PTI */

#define CLKRV_INTERRUPT_NUMBER  ST7200_MPEG_CLK_REC_INTERRUPT

#else
#error platform not defined !
#endif




/* Private Variables (static)------------------------------------------------ */
static STCLKRV_Handle_t CLKRVHndl0;

/* Global Variables --------------------------------------------------------- */
extern ST_Partition_t *DriverPartition_p;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

/* Functions ---------------------------------------------------------------- */

/*******************************************************************************
Name        : CLKRV_Init
Description : Initialise the CLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
#if defined(ST_7200)
BOOL CLKRV_Init(ST_DeviceName_t DeviceName, void * CRUBaseAddress_p, STCLKRV_Decode_t DecodeType)
#else
BOOL CLKRV_Init(void)
#endif
{
    BOOL RetOk = TRUE;

    STCLKRV_InitParams_t stclkrv_InitParams_s;
    STCLKRV_OpenParams_t stclkrv_OpenParams;
    /* Initialization & Open code fragments */
    /* assumed that all dependent drivers are already initialised */

/* Devices that STCLKRV doesn't use PWM */
#if defined(mb390)|| defined(mb400)|| defined(mb436)|| defined(mb411)|| defined(mb457)|| defined(mb428)|| defined(maly3s) \
    || defined(mb519) || defined(SAT5107)
    /* Specific initializations (with 5.0.0 revision and later) */

    stclkrv_InitParams_s.DeviceType         = CLKRV_DEVICE_TYPE;
    stclkrv_InitParams_s.Partition_p        = DriverPartition_p;

    /* STPTI & Clock recovery Events */
    strcpy( stclkrv_InitParams_s.EVTDeviceName,     STEVT_DEVICE_NAME);
    strcpy( stclkrv_InitParams_s.PCREvtHandlerName, STEVT_DEVICE_NAME);
    strcpy( stclkrv_InitParams_s.PTIDeviceName,     PTI_DEVICE_NAME);

    /* Clock recovery Filter */
    stclkrv_InitParams_s.PCRMaxGlitch   = STCLKRV_PCR_MAX_GLITCH;
    stclkrv_InitParams_s.PCRDriftThres  = STCLKRV_PCR_DRIFT_THRES;
    stclkrv_InitParams_s.MinSampleThres = STCLKRV_MIN_SAMPLE_THRESHOLD;
    stclkrv_InitParams_s.MaxWindowSize  = STCLKRV_MAX_WINDOW_SIZE;
    stclkrv_InitParams_s.InterruptNumber= CLKRV_INTERRUPT_NUMBER;
    stclkrv_InitParams_s.InterruptLevel = 6;

    /* device type dependant parameters */
    stclkrv_InitParams_s.FSBaseAddress_p   = (U32 *)CKG_BASE_ADDRESS;  /* SD/PCM/SPDIF or HD clock FS and clkrv tracking registers */
    #if defined (ST_7100)
    stclkrv_InitParams_s.AUDCFGBaseAddress_p = (U32 *)ST7100_AUDIO_IF_BASE_ADDRESS ;
    #elif defined (ST_7109)
    stclkrv_InitParams_s.AUDCFGBaseAddress_p = (U32 *)ST7109_AUDIO_IF_BASE_ADDRESS ;
    #elif defined (ST_7200)
    stclkrv_InitParams_s.AUDCFGBaseAddress_p = (U32 *)ST7200_AUDIO_IF_BASE_ADDRESS ;
    #else
    stclkrv_InitParams_s.ADSCBaseAddress_p = (U32 *)ADSC_BASE_ADDRESS; /* PCM clock FS registers base address */
    #endif

#else /* !(mb391|mb390|mb400|mb436|mb411|mb457|mb428|maly3s|SAT5107) */
#ifndef mb391
#if defined(mb382)
        /* Set special PWM ratio for mb382. */
        /* STPWM_MARK_MAX/2 is not suitable for this board to get colour on NTSC display. (checked with P. Ley) */
    stclkrv_InitParams_s.InitialMark       = 0x6E;
#else /* mb382 */
    stclkrv_InitParams_s.InitialMark       = STCLKRV_INIT_MARK_VALUE;
#endif /* mb382 */
    stclkrv_InitParams_s.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    stclkrv_InitParams_s.PCRDriftThres     = STCLKRV_PCR_DRIFT_THRES;
    stclkrv_InitParams_s.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    stclkrv_InitParams_s.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    stclkrv_InitParams_s.Partition_p       = DriverPartition_p;
    strcpy( stclkrv_InitParams_s.EVTDeviceName, STEVT_DEVICE_NAME);
    strcpy( stclkrv_InitParams_s.PCREvtHandlerName, STEVT_DEVICE_NAME);
    strcpy(stclkrv_InitParams_s.PWMDeviceName, STPWM_DEVICE_NAME);
    stclkrv_InitParams_s.VCXOChannel       = CLKRV_VCXO_CHANNEL;
    stclkrv_InitParams_s.MinimumMark       = STCLKRV_MIN_MARK_VALUE;
    stclkrv_InitParams_s.MaximumMark       = STCLKRV_MAX_MARK_VALUE;
    stclkrv_InitParams_s.TicksPerMark      = STCLKRV_TICKS_PER_MARK;
    stclkrv_InitParams_s.DeviceType        = STCLKRV_DEVICE_PWMOUT;
#endif
#endif /* (mb391|mb390|mb400|mb436|mb457|mb428|maly3s|SAT5107) */
#if defined (mb391)         /* Only for C1 based devices using FS based Clock Recovery */
    /*ST_DeviceName_t         PTIDeviceNam;*/
    strcpy(stclkrv_InitParams_s.PTIDeviceName, PTI_DEVICE_NAME/*"STPTI1"*/);

    stclkrv_InitParams_s.DeviceType        =  CLKRV_DEVICE_TYPE;
    stclkrv_InitParams_s.ADSCBaseAddress_p = (U32*)ST7710_AUDIO_IF_BASE_ADDRESS;
    stclkrv_InitParams_s.InterruptNumber   = ST7710_MPEG_CLK_REC_INTERRUPT;
    stclkrv_InitParams_s.FSBaseAddress_p   = (U32*)CKG_BASE_ADDRESS;
    stclkrv_InitParams_s.InterruptLevel    = 7;
    stclkrv_InitParams_s.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    stclkrv_InitParams_s.PCRDriftThres     = STCLKRV_PCR_DRIFT_THRES;
    stclkrv_InitParams_s.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    stclkrv_InitParams_s.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    stclkrv_InitParams_s.Partition_p       = DriverPartition_p;
    strcpy( stclkrv_InitParams_s.EVTDeviceName, STEVT_DEVICE_NAME);
#endif
#if defined (DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_LINK) || defined (DVD_TRANSPORT_DEMUX)
    strcpy( stclkrv_InitParams_s.PCREvtHandlerName, STEVT_DEVICE_NAME);
#endif

#if defined(ST_7200)
    stclkrv_InitParams_s.CRUBaseAddress_p = CRUBaseAddress_p;
    stclkrv_InitParams_s.DecodeType       = DecodeType;
#endif /* #if defined(ST_7200) ... */

#if defined(ST_7200)
    if( STCLKRV_Init(DeviceName, &stclkrv_InitParams_s) != ST_NO_ERROR )
#else  /* #if defined(ST_7200) ... */
    if( STCLKRV_Init(STCLKRV_DEVICE_NAME, &stclkrv_InitParams_s) != ST_NO_ERROR )
#endif /* #if defined(ST_7200) ... else ... */
    {
        RetOk = FALSE;
        STTBX_Print(("CLKRV_Init() failed !\n"));
    }
    else
    {
#if defined(ST_7200)
       if( STCLKRV_Open(DeviceName, &stclkrv_OpenParams, &CLKRVHndl0) != ST_NO_ERROR)
#else  /* #if defined(ST_7200) ... */
       if( STCLKRV_Open(STCLKRV_DEVICE_NAME, &stclkrv_OpenParams, &CLKRVHndl0) != ST_NO_ERROR)
#endif /* #if !defined(ST_7200) ... else ... */
        {
            STTBX_Print(("CLKRV_Open() failed !\n"));
            RetOk = FALSE;
        }
        else
        {
            STTBX_Print(("STCLKRV initialized, \trevision=%s\n", STCLKRV_GetRevision() ));
        }
    }
    return(RetOk);
} /* end CLKRV_Init() */


/*******************************************************************************
Name        : CLKRV_Term
Description : Terminate the CLKRV driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
void CLKRV_Term(void)
{
    ST_ErrorCode_t       ErrorCode = ST_NO_ERROR;
    STCLKRV_TermParams_t TermParams;

    STCLKRV_Close(CLKRVHndl0);
    TermParams.ForceTerminate = FALSE;
    ErrorCode = STCLKRV_Term(STCLKRV_DEVICE_NAME, &TermParams);
    if (ErrorCode == ST_ERROR_OPEN_HANDLE)
    {
        STTBX_Print(("STCLKRV_Term(%s): Warning !! Still open handle\n", STCLKRV_DEVICE_NAME));
        TermParams.ForceTerminate = TRUE;
        ErrorCode = STCLKRV_Term(STCLKRV_DEVICE_NAME, &TermParams);
    }
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STCLKRV_Term(%s): ok\n", STCLKRV_DEVICE_NAME));
    }
    else
    {
        STTBX_Print(("STCLKRV_Term(%s): error 0x%0x\n", STCLKRV_DEVICE_NAME, ErrorCode));
    }
} /* end CLKRV_Term() */

#endif /* USE_CLKRV */


/* End of clclkrv.c */
