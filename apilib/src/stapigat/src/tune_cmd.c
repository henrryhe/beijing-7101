/*****************************************************************************

File name   : tune_cmd.c

Description : Testtool Tuner Command

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                          Name
----               ------------                          ----
 30 Jul 03         Updated for STTUNER rel 4.4.0         CL

*****************************************************************************/

/*

  ----------------------------------------------------------------------------
  Just set STVID_TUNER environment variable to use tuner in stapigat and tests
  ----------------------------------------------------------------------------

  Default device used is  use STEMSTV0299 (Dual tuner)

  Supported devices are :

  * EVAL0299 :
      Be aware of switch configuration on EVAL0299, STEM or Board
      mb314, mb361, and mb290, board can be used without changing any switch
      in both tuner or packet injector input mode !

      But a crossed cable between EVAL299 and DB499B sould be used :
      - inverting all signals 1->20, 2->19,...
      and then
      - inverting pin 5 and pin 7

  * STEM_STV0299 (DUAL):
      The working configuration is using :
         Switchs  | A0  | A1  | A2  | JP13| JP4
         ---------------------------------------
         Position | ON  | OFF | OFF | OFF | ON

      It means LNB add = 0x42 and selection of tuner1/2 is done by PIO(bit0)

  * MB314B + STEM_STV0299 (DUAL):
      The working MB314B board configuration is using :
      SW2 - 1 => ON
      SW2 - 2 => OFF
      SW2 - 3 => OFF
      SW2 - 4 => OFF

  */


/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stddefs.h"

#include "tune_cmd.h"
#include "sttbx.h"
#include "sti2c.h"
#include "testtool.h"
#include "api_cmd.h"
#include "sttuner.h"
#include "clevt.h"
#include "cli2c.h"
#include "startup.h"
#include "testcfg.h"

/* Global Definitions ----------------------------------------------------- */
#ifdef SAT5107
#define USE_TUNER_STX0288
#else
#ifndef USE_TUNER_EVAL0299
#ifndef USE_TUNER_STEM_STV0299
#define USE_TUNER_STEM_STV0299
#endif
#endif
#endif

#ifdef USE_TUNER_STX0288
#define TUNER_DEVICE_TYPE          STTUNER_DEVICE_SATELLITE
#define TUNER_I2C_BUS              STI2C_BACK_DEVICE_NAME
#define TUNER_DEMOD_TYPE           STTUNER_DEMOD_STX0288
#define TUNER_LNB_TYPE             STTUNER_LNB_LNBH21
#define TUNER_TYPE                 STTUNER_TUNER_STB6000
#define TUNER_I2C                  0xC0
#define TUNER_DEMOD_I2C            0xD0
#define TUNER_LNB_ROUTE            STTUNER_IO_DIRECT
#define TUNER_LNB_ADDRESS          0x10
#define TUNER_EXT_CLOCK            4000000
#define TUNER_MODE                 STTUNER_MOD_QPSK
#define TUNER_TS_OUTPUT_MODE       STTUNER_TS_MODE_PARALLEL
#endif


#ifdef USE_TUNER_EVAL0299
/* External tuner Eval299 */
#define TUNER_DEVICE_TYPE          STTUNER_DEVICE_SATELLITE
#define TUNER_I2C_BUS              STI2C_FRONT_DEVICE_NAME
#define TUNER_DEMOD_TYPE           STTUNER_DEMOD_STV0299
#define TUNER_LNB_TYPE             STTUNER_LNB_STV0299
#define TUNER_TYPE                 STTUNER_TUNER_VG1011
#define TUNER_I2C                  0xC0
#define TUNER_DEMOD_I2C            0xD0
#define TUNER_LNB_ROUTE            STTUNER_IO_PASSTHRU
#define TUNER_LNB_ADDRESS          0x0 /* ignored */
#define TUNER_EXT_CLOCK            4000000
#define TUNER_MODE                 STTUNER_MOD_QPSK
#define TUNER_TS_OUTPUT_MODE       STTUNER_TS_MODE_PARALLEL

#endif /* USE_TUNER_EVAL0299 */

#ifdef USE_TUNER_STEM_STV0299
/* 299-STEM Tuner */
#define TUNER_DEVICE_TYPE          STTUNER_DEVICE_SATELLITE
#define TUNER_I2C_BUS              STI2C_FRONT_DEVICE_NAME
#define TUNER_DEMOD_TYPE           STTUNER_DEMOD_STV0299
#define TUNER_DEMOD_I2C            0xD0
#define TUNER_EXT_CLOCK            4000000
#define TUNER_MODE                 STTUNER_MOD_QPSK
#if defined (mb376) || defined (espresso)
#define TUNER_LNB_TYPE             STTUNER_LNB_STEM
#define TUNER_TYPE                 STTUNER_TUNER_HZ1184
#define TUNER_I2C                  0xC0
#define TUNER_LNB_ROUTE            STTUNER_IO_DIRECT
#define TUNER_LNB_ADDRESS          0x42
#define TUNER_TS_OUTPUT_MODE       STTUNER_TS_MODE_SERIAL
#elif defined (espresso)
#define TUNER_LNB_TYPE             STTUNER_LNB_STV0299
#define TUNER_TYPE                 STTUNER_TUNER_MAX2116
#define TUNER_I2C                  0xC8
#define TUNER_LNB_ROUTE            STTUNER_IO_PASSTHRU
#define TUNER_LNB_ADDRESS          0x0
#define TUNER_TS_OUTPUT_MODE       STTUNER_TS_MODE_SERIAL
#else
#define TUNER_LNB_TYPE             STTUNER_LNB_STEM
#define TUNER_TYPE                 STTUNER_TUNER_HZ1184
#define TUNER_I2C                  0xC0
#define TUNER_LNB_ROUTE            STTUNER_IO_DIRECT
#define TUNER_LNB_ADDRESS          0x42
#define TUNER_TS_OUTPUT_MODE       STTUNER_TS_MODE_PARALLEL
#endif
#endif /* USE_TUNER_STEM_STV0299 */

#define MAX_BANDS               3
#define MAX_SCANS              12
#define MAX_SIGNALS            10
#define MAX_THRESHOLDS         10

#define NUM_TRANSPONDERS  (sizeof(TransponderList)/sizeof(ST_TransponderInfo_t))

#define SIZEOF_STRING(x) sizeof(x)/sizeof(char)
#define TUNER_PRINT(x) { \
    /* Check lenght */ \
    if(strlen(x)>sizeof(TUNER_Msg)){ \
        sprintf(x, "TUNER message too long (%d)!!\n", strlen(x)); \
        STTBX_Print((x)); \
        return(TRUE); \
    } \
    STTBX_Print((x)); \
} \

typedef enum
{
    ST_REGION_EUROPE_DVB,
    ST_REGION_USA_DVB,
    ST_REGION_USA_DSS
} ST_Region_e;

typedef struct
{
    U16                     TSPid;
    U16                     Freq;
    STTUNER_Polarization_t  Polarization;
    STTUNER_FECRate_t       FECRates;
    STTUNER_Modulation_t    Modulation;
    STTUNER_LNBToneState_t  LNBToneState;
    U32                     Band;
    U32                     AGC;
    U32                     SymbolRate;
} ST_TransponderInfo_t;


/* Global Variables ------------------------------------------------------- */

static STTUNER_Handle_t TUNERHandle = 0;
static char TUNER_Msg[250];            /* text for trace */

/* Local Variables -------------------------------------------------------- */

static semaphore_t             *TunerSemaphore_p;
static STTUNER_EventType_t     LastEvent = STTUNER_EV_NO_OPERATION;

static STTUNER_BandList_t      Bands;
static STTUNER_Band_t          BandList[MAX_BANDS];
static STTUNER_ScanList_t      Scans;
static STTUNER_Scan_t          ScanList[MAX_SCANS];

static const char *MSG_Unknown = "???";
static const char *MSG_Expected_Region  = "expected Region";
static const char *MSG_Expected_Frequency  = "expected Frequency MHz";
static const char *MSG_Expected_Band       = "expected Band Index";
static const char *MSG_Expected_Polarity   = "expected Polarity (VERT/HORIZ)";
static const char *MSG_Expected_FECRate    = "expected FEC Rate (1_2/2_3/3_4 etc)";
static const char *MSG_Expected_LNBTone    = "expected LNB Tone State (LNBON/LNBOFF/LNBDEF)";
static const char *MSG_Expected_SymbolRate = "expected SymbolRate";
static const char *MSG_Expected_Modulation = "expected Modulation (QPSK)";
static const char *MSG_Expected_AGC_Enable = "expected AGC Enable (TRUE/FALSE)";

/*  Tuner Display functions -------------------------------------------- */
static BOOL TUNER_SubSetFrequency( S32 Freq, STTUNER_Scan_t *ScanParams_p );
static ST_ErrorCode_t TUNER_DisplayInfo( void );

/*-----------------------------------------------------------------------------
 * Function : TUNER_TextError
 *
 * Input    : char *, char *, ST_ErrorCode_t
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------*/
static BOOL TUNER_TextError(ST_ErrorCode_t Error, char *Text)
{
    BOOL RetErr = FALSE;

    /* Error not found in common ones ? */
    if(API_TextError(Error, Text) == FALSE)
    {
        RetErr = TRUE;
        switch ( Error )
        {
            case STTUNER_ERROR_UNSPECIFIED:
                strcat( Text, "Unspecified error !\n");
                break;
            case STTUNER_ERROR_HWFAIL:
                strcat( Text, "Hardware error !\n");
                break;
            case STTUNER_ERROR_EVT_REGISTER:
                strcat( Text, "event registration error !\n");
                break;
            case STTUNER_ERROR_ID:
                strcat( Text, "id error !\n");
                break;
            case STTUNER_ERROR_POINTER:
                strcat( Text, "pointer error !\n");
                break;
            case STTUNER_ERROR_INITSTATE:
                strcat( Text, "init state error !\n");
                break;
            default:
                sprintf( Text, "%s Unexpected error [0x%X] !\n", Text,  Error );
                break;
        }
    }

    TUNER_PRINT(Text);
    return( API_EnableError ? RetErr : FALSE);
}

/*-------------------------------------------------------------------------
 * Function : SetChannelFreq
 *
 * Input    : Freq
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t SetChannelFreq( ST_TransponderInfo_t *TSInfo_p )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    U32 TunerFreq;
    STTUNER_Scan_t ScanParams;
    STTUNER_TunerInfo_t TUNERInfo;

    /* Check program linked to a transponder */
    if ( TSInfo_p == NULL )
    {
        STTBX_Print(("Program not linked to Transponder\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Get current tuner frequency, compare and if the same
    ** dont bother re-tuning
    */
    if ( STTUNER_GetTunerInfo(TUNERHandle, &TUNERInfo) == ST_NO_ERROR )
        TunerFreq = TUNERInfo.Frequency / 1000;
    else
        TunerFreq = 0;

    /* if 'new' frequency is outside the range
    ** set the 'new' frequency
    */
    if (( TunerFreq == 0 ) ||
        ( TunerFreq < ( TSInfo_p->Freq-3 )) ||
        ( TunerFreq > ( TSInfo_p->Freq+3 )))
    {
        TunerFreq = TSInfo_p->Freq;
        ScanParams.Band         = TSInfo_p->Band;
        ScanParams.LNBToneState = TSInfo_p->LNBToneState;
        ScanParams.Polarization = TSInfo_p->Polarization;
        ScanParams.FECRates     = TSInfo_p->FECRates;
        ScanParams.SymbolRate   = TSInfo_p->SymbolRate;
        ScanParams.Modulation   = TSInfo_p->Modulation;
        ScanParams.AGC          = TSInfo_p->AGC;
        ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;

        ErrCode = TUNER_SubSetFrequency( TSInfo_p->Freq, &ScanParams );
        if ( ErrCode == ST_NO_ERROR )
        {
            ErrCode = STTUNER_GetTunerInfo(TUNERHandle, &TUNERInfo);
            if ( ErrCode == ST_NO_ERROR )
            {
                if ( TUNERInfo.Status != STTUNER_STATUS_LOCKED )
                    ErrCode = STTUNER_EV_SCAN_FAILED; /* failed */
            }
        }
    }
    else
        TUNER_DisplayInfo();

    sprintf(TUNER_Msg ,"SetChannelFreq(%d): ",TunerFreq);
    TUNER_TextError(ErrCode, TUNER_Msg);
    return ErrCode;
}

/*-------------------------------------------------------------------------
 * Function : TUNER_EventToString
 *            Return STTUNER event descriptions
 * Input    : Error Code
 * Output   :
 * Return   : pointer to error description
 * ----------------------------------------------------------------------*/
static const char *TUNER_EventToString( ST_ErrorCode_t Error )
{
    switch( Error )
    {
    case STTUNER_EV_NO_OPERATION: return "STTUNER_EV_NO_OPERATION";
    case STTUNER_EV_LOCKED: return "STTUNER_EV_LOCKED";
    case STTUNER_EV_UNLOCKED: return "STTUNER_EV_UNLOCKED";
    case STTUNER_EV_SCAN_FAILED: return "STTUNER_EV_SCAN_FAILED";
    case STTUNER_EV_TIMEOUT: return "STTUNER_EV_TIMEOUT";
    default: break;
    }
    return NULL;
}

/*-------------------------------------------------------------------------
 * Function : bit2num
 *            Converts bit to number for array selection
 * Input    : bit selected
 * Output   :
 * Return   : position of bit
 * ----------------------------------------------------------------------*/
static int bit2num( int bit )
{
    int i;
    for ( i = 0; i < 16; i ++)
    {
        if (( 1 << i ) == bit )
            break;
    }

    return ++i;

} /* end bit2num */

/*-------------------------------------------------------------------------
 * Function : TUNER_FecToString
 *            Convert Forward Error Correction value to text
 * Input    : FEC Bit value
 * Output   :
 * Return   : pointer to FEC text
 * ----------------------------------------------------------------------*/
static char *TUNER_FecToString(STTUNER_FECRate_t Fec)
{
    S8 FecIndex;
    static char FecRate[][4] =
    {
        "ALL",
        "1_2",
        "2_3",
        "3_4",
        "4_5",
        "5_6",
        "6_7",
        "7_8",
        "8_9"
    };

    if ( Fec == STTUNER_FEC_ALL )
        return FecRate[0];
    else
    {
        FecIndex = bit2num(Fec);
        if ( FecIndex > 8 )
            return (char*)MSG_Unknown;
        else
            return FecRate[FecIndex];
    }

} /* end TUNER_FecToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_LNBToneToString
 *            Convert Tone State to text
 * Input    : Tone State value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
static char *TUNER_LNBToneToString(STTUNER_LNBToneState_t Tone)
{
    static char LNBTone[][8] =
    {
        "LNBDEF",
        "LNBOFF",
        "LNBON"
    };

    if ( Tone > 3 )
        return (char*)MSG_Unknown;
    return LNBTone[Tone];

} /* end TUNER_LNBToneToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_ModulationToString
 *            Convert Modulation value to text
 * Input    : Modulation Bit value
 * Output   :
 * Return   : pointer to FEC text
 * ----------------------------------------------------------------------*/
static char *TUNER_ModulationToString(STTUNER_Modulation_t Mod)
{
    S8 ModIndex;
    static char *Modulation[] =
    {
        "MODALL",
        "QPSK",
        "8PSK",
        "QAM",
        "16QAM",
        "32QAM",
        "64QAM",
        "128QAM",
        "256QAM",
        "BPSK"
    };

    if ( Mod == STTUNER_MOD_ALL )
        return Modulation[0];
    else
    {
        ModIndex = bit2num(Mod);
        if ( ModIndex > 9 )
            return (char*)MSG_Unknown;
        else
            return Modulation[ModIndex];
    }

} /* end TUNER_ModulationToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_PolarizationToString
 *            Convert Polarization value to text
 * Input    : Polarization value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
static char *TUNER_PolarityToString(STTUNER_Polarization_t Plr)
{
    static char Polarity[][8] =
    {
        "Invalid",
        "HORIZ",
        "VERT",
        "ALL"
    };

    if ( Plr > 4 )
        return (char*)MSG_Unknown;
    return Polarity[Plr];

} /* end TUNER_PolarityToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_StatusToString
 *            Convert Status value to text
 * Input    : Event value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
static char *TUNER_StatusToString(STTUNER_Status_t Status)
{
    switch ( Status )
    {
    case STTUNER_STATUS_UNLOCKED:  return "UNLOCKED";
    case STTUNER_STATUS_SCANNING:  return "SCANNING";
    case STTUNER_STATUS_LOCKED:    return "LOCKED";
    case STTUNER_STATUS_NOT_FOUND: return "NOT_FOUND";
    default:
        break;
    }
    return (char*)MSG_Unknown;

} /* end TUNER_StatusToString */

/*-------------------------------------------------------------------------
 * Function : TunerNotifyFunction
 *            Function to notify when Tuner has an event
 * Input    : The tuner event that has occurred
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static void TunerNotifyFunction(STTUNER_Handle_t Handle, STTUNER_EventType_t Event, ST_ErrorCode_t Error)
{
    if ( Handle == TUNERHandle )
    {
        LastEvent = Event;
        semaphore_signal(TunerSemaphore_p);
    }
    else
        STTBX_Print(("TUNER_NotifyFunction()=Invalid Handle"));
}

/*-------------------------------------------------------------------------
 * Function : AwaitNotifyFunction
 *            Function to wait for tuner event
 * Input    : None
 * Output   :
 * Return   : The tuner event that has occurred
 * ----------------------------------------------------------------------*/
static void AwaitNotifyFunction(STTUNER_EventType_t *EventType_p)
{
    semaphore_wait(TunerSemaphore_p);
    *EventType_p = LastEvent;
}

/*-------------------------------------------------------------------------
 * Function : WaitForTunerEvent
 *            Function to wait for tuner event
 * Input    : None
 * Output   :
 * Return   : The tuner event that has occurred
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t WaitForTunerEvent(void)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STTUNER_EventType_t Event;

    /* Wait for notify function to return */
    AwaitNotifyFunction(&Event);

    if ( Event == STTUNER_EV_LOCKED )
        ErrCode = TUNER_DisplayInfo();
    else
        STTBX_Print(("Failed %s\n", TUNER_EventToString(Event)));

    return ErrCode;
}

/* Global Functions -----------------------------------------------------*/


/*-------------------------------------------------------------------------
 * Function : TUNER_Continue
 *            Function to continue scanning
 * Input    :
 * Output   :
 * Return   : ErrCode
 * ----------------------------------------------------------------------*/
static BOOL TUNER_Continue( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    ErrCode = STTUNER_ScanContinue(TUNERHandle, 0);

    sprintf(TUNER_Msg,"STTUNER_ScanContinue(0x%08x): ", TUNERHandle);
    TUNER_TextError(ErrCode, TUNER_Msg);

    if(ErrCode == ST_NO_ERROR)
        ErrCode = WaitForTunerEvent();

    return (ErrCode);

} /* end TUNER_Continue */

/*-------------------------------------------------------------------------
 * Function : TUNER_DisplayInfo
 *            Function to Display Tuner Info
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t TUNER_DisplayInfo( void )
{
    ST_ErrorCode_t          ErrCode;
    STTUNER_TunerInfo_t     TUNERInfo;
    ErrCode = STTUNER_GetTunerInfo(TUNERHandle, &TUNERInfo);

    sprintf(TUNER_Msg, "STTUNER_GetTunerInfo(0x%08x): ", TUNERHandle);
    TUNER_TextError(ErrCode, TUNER_Msg);

    if ( TUNERInfo.Status != STTUNER_STATUS_LOCKED )
        ErrCode = STTUNER_EV_UNLOCKED;

    sprintf(TUNER_Msg,"%-9.9s on %5u MHz, Band %d, %-6s, %-5.5s, FEC_%3s, " \
            "SRate %5d, Mod %-6.6s, AGC %-5.5s ",
            TUNER_StatusToString(TUNERInfo.Status),
            TUNERInfo.Frequency / 1000,
            TUNERInfo.ScanInfo.Band,
            TUNER_LNBToneToString(TUNERInfo.ScanInfo.LNBToneState),
            TUNER_PolarityToString(TUNERInfo.ScanInfo.Polarization),
            TUNER_FecToString(TUNERInfo.ScanInfo.FECRates),
            TUNERInfo.ScanInfo.SymbolRate / 1000,
            TUNER_ModulationToString(TUNERInfo.ScanInfo.Modulation),
            TUNERInfo.ScanInfo.AGC ? "TRUE":"FALSE");
    TUNER_PRINT(TUNER_Msg);

    sprintf(TUNER_Msg,"(Quality %.2u%%, BER %d)\n",
            TUNERInfo.SignalQuality,
            TUNERInfo.BitErrorRate );
    TUNER_PRINT(TUNER_Msg);

    return (ErrCode);

} /* end TUNER_DisplayInfo */

/*-------------------------------------------------------------------------
 * Function : TUNER_init
 *            Function to Init Tuner
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t TUNER_init( void )
{
    ST_ErrorCode_t          ErrCode;
    STTUNER_InitParams_t    InitParams;

    memset(&InitParams, 0, sizeof(STTUNER_InitParams_t));
    /* Set EVTDeviceName if Tuner Event Handling is required */

    InitParams.Device = TUNER_DEVICE_TYPE;
    InitParams.Max_BandList = MAX_BANDS;
    InitParams.Max_ThresholdList = MAX_THRESHOLDS;
    InitParams.Max_ScanList = MAX_SCANS;

    strcpy( InitParams.EVT_DeviceName, STEVT_DEVICE_NAME );
    InitParams.DriverPartition = DriverPartition_p;

    InitParams.DemodType = TUNER_DEMOD_TYPE;
    InitParams.LnbType   = TUNER_LNB_TYPE;
    InitParams.TunerType = TUNER_TYPE;

    InitParams.DemodIO.Route     = STTUNER_IO_DIRECT;
    InitParams.DemodIO.Driver    = STTUNER_IODRV_I2C;
    InitParams.DemodIO.Address   = TUNER_DEMOD_I2C;

    InitParams.TunerIO.Route     = STTUNER_IO_REPEATER;
    InitParams.TunerIO.Driver    = STTUNER_IODRV_I2C;
    InitParams.TunerIO.Address   = TUNER_I2C ;

    InitParams.LnbIO.Route       = TUNER_LNB_ROUTE;
    InitParams.LnbIO.Driver      = STTUNER_IODRV_I2C;
    InitParams.LnbIO.Address     = TUNER_LNB_ADDRESS;

    InitParams.ExternalClock     = TUNER_EXT_CLOCK;
    InitParams.TSOutputMode      = TUNER_TS_OUTPUT_MODE;
#if defined (ST_5528)
    InitParams.SerialClockSource = STTUNER_SCLK_VCODIV6;
#else
    InitParams.SerialClockSource = STTUNER_SCLK_DEFAULT;
#endif
    InitParams.SerialDataMode    = STTUNER_SDAT_DEFAULT;
#if defined(STVID_DIRECTTV)
    InitParams.FECMode = STTUNER_FEC_MODE_DIRECTV;
#else
    InitParams.FECMode = STTUNER_FEC_MODE_DVB;
#endif

    strcpy( InitParams.DemodIO.DriverName, TUNER_I2C_BUS );
    strcpy( InitParams.TunerIO.DriverName, TUNER_I2C_BUS );
    strcpy( InitParams.LnbIO.DriverName, TUNER_I2C_BUS );

    strcpy( InitParams.EVT_RegistrantName, STEVT_DEVICE_NAME);

    /* Init TUNER */

    if (( ErrCode = STTUNER_Init( STTUNER_DEVICE_NAME, &InitParams )) == ST_NO_ERROR )
        TunerSemaphore_p = semaphore_create_fifo(0);

    sprintf(TUNER_Msg,"STTUNER_Init(%s): ", STTUNER_DEVICE_NAME);
    TUNER_TextError(ErrCode, TUNER_Msg);

    return (ErrCode);

} /* end TUNER_init */

/*-------------------------------------------------------------------------
 * Function : TUNER_Open
 *            Function to Open Tuner
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_Open( void )
{
    ST_ErrorCode_t          ErrCode;
    STTUNER_OpenParams_t    TUNEROpenParams;

    /* Setup open params for tuner */
    TUNEROpenParams.NotifyFunction = TunerNotifyFunction;

    ErrCode = STTUNER_Open(STTUNER_DEVICE_NAME, &TUNEROpenParams, &TUNERHandle );

    sprintf(TUNER_Msg,"STTUNER_Open(%s): ", STTUNER_DEVICE_NAME);
    TUNER_TextError(ErrCode, TUNER_Msg);

    return (ErrCode);

} /* end TUNER_Open */

/*-------------------------------------------------------------------------
 * Function : TUNER_Scan
 *            Function to Start Tuner Scan
 * Input    : ScanStart (MHz), ScanEnd (MHz)
 * Output   :
 * Return   : ErrorCode
 * ----------------------------------------------------------------------*/
BOOL TUNER_Scan( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;
    S32 ScanStartFreq;          /* Freq in MHz */
    S32 ScanEndFreq;

    if ( STTST_GetInteger( pars_p, 0x0, (S32*)&ScanStartFreq ) ||
        ( (int) ScanStartFreq == 0 ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_Frequency );
        return TRUE;
    }

    STTST_GetInteger( pars_p, 0x0, (S32*)&ScanEndFreq );
    if ((int) ScanEndFreq == 0 )
        ScanEndFreq = ScanStartFreq;

    /* Scan from ScanStart to ScanEnd */
    ErrCode = STTUNER_Scan(TUNERHandle, (ScanStartFreq*1000), (ScanEndFreq*1000), 0, 0);

    sprintf(TUNER_Msg,"STTUNER_Scan(0x%08x,Start %d MHz, Stop %d MHz): ", TUNERHandle, ScanStartFreq, ScanEndFreq);
    TUNER_TextError(ErrCode, TUNER_Msg);

    if(ErrCode == ST_NO_ERROR)
        ErrCode = WaitForTunerEvent();

    return (ErrCode);

} /* end TUNER_ScanStart */

/*-------------------------------------------------------------------------
 * Function : TUNER_SetBandList
 *            Function to Set Tuner Band List
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_SetBandList( BOOL NewNotAdd, STTUNER_Band_t *BandParams_p )
{
    ST_ErrorCode_t          ErrCode;
    U16                     BandIndex;

   /* Set tuner band list */

    Bands.BandList = BandList;
    if (NewNotAdd)
        Bands.NumElements = 1;
    else if ( Bands.NumElements < MAX_BANDS )
        Bands.NumElements ++;
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Max Band Elements reached (%d)\n", MAX_BANDS ));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    BandIndex = Bands.NumElements-1;
    BandList[BandIndex].BandStart = BandParams_p->BandStart * 1000;
    BandList[BandIndex].BandEnd   = BandParams_p->BandEnd * 1000;
    BandList[BandIndex].LO        = BandParams_p->LO * 1000;
    BandList[BandIndex].DownLink  = BandParams_p->DownLink;

    ErrCode = STTUNER_SetBandList(TUNERHandle, &Bands);
    sprintf(TUNER_Msg,"STTUNER_SetBandList(0x%08x): ", TUNERHandle);
    TUNER_TextError(ErrCode, TUNER_Msg);

    sprintf(TUNER_Msg,"Band #%d = Start %d, End %d, LOsc %d\n",
                BandIndex,
                BandList[BandIndex].BandStart,
                BandList[BandIndex].BandEnd,
                BandList[BandIndex].LO );
    TUNER_PRINT(TUNER_Msg);

    return (ErrCode);

} /* end TUNER_SetBandList */

/*-------------------------------------------------------------------------
 * Function : TUNER_SetScanList
 *            Function to Set Tuner Scan parameters
 * Input    : ScanParams_p
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_SetScanList( BOOL NewNotAdd, STTUNER_Scan_t *ScanParams_p )
{
    ST_ErrorCode_t          ErrCode;
    U16                     ScanIndex;

    /* setup structure */
    Scans.ScanList = ScanList;
    if (NewNotAdd)
        Scans.NumElements = 1;
    else if ( Scans.NumElements < MAX_SCANS )
        Scans.NumElements ++;
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Max Scan Elements reached\n", MAX_SCANS ));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    ScanIndex = Scans.NumElements-1;
    ScanList[ScanIndex].Polarization = STTUNER_PLR_ALL;
    ScanList[ScanIndex].FECRates     = STTUNER_FEC_ALL;
    ScanList[ScanIndex].Modulation   = TUNER_MODE;
    ScanList[ScanIndex].LNBToneState = ScanParams_p->LNBToneState;
    ScanList[ScanIndex].Band         = ScanParams_p->Band;
    ScanList[ScanIndex].AGC          = STTUNER_AGC_ENABLE;
    ScanList[ScanIndex].SymbolRate   = ScanParams_p->SymbolRate*1000;  /* convert to M/Symbols */
    ScanList[ScanIndex].IQMode       = STTUNER_IQ_MODE_AUTO;
    #if !defined(SAT5107)
    ScanList[ScanIndex].Mode         = STTUNER_MODE_2K;            /* (ter) */
    ScanList[ScanIndex].Guard        = STTUNER_GUARD_1_32;         /* (ter) */
    ScanList[ScanIndex].Force        = STTUNER_FORCE_NONE;         /* (ter) */
    ScanList[ScanIndex].Hierarchy    = STTUNER_HIER_NONE;          /* (ter) */
    ScanList[ScanIndex].Spectrum     = STTUNER_INVERSION_NONE;     /* (ter & cable) */
    ScanList[ScanIndex].FreqOff      = STTUNER_OFFSET_NONE;        /* (ter) */
    ScanList[ScanIndex].ChannelBW    = STTUNER_CHAN_BW_6M;         /* (ter) */
    ScanList[ScanIndex].EchoPos      = 0;                          /* (ter) */
    ScanList[ScanIndex].J83          = STTUNER_J83_NONE;           /* (cab) */
    ScanList[ScanIndex].SpectrumIs   = STTUNER_INVERSION_NONE;     /* (cable) */
    #endif

    ErrCode = STTUNER_SetScanList(TUNERHandle, &Scans);
    sprintf(TUNER_Msg,"STTUNER_SetScanList(0x%08x): ", TUNERHandle);
    TUNER_TextError(ErrCode, TUNER_Msg);

    if(ErrCode == ST_NO_ERROR)
    {
        sprintf(TUNER_Msg,"Scan #%d = Band %d, %-6s, %-5.5s, FEC %s, " \
                "SRate %5u, Mod %-6.6s, AGC %-5.5s\n",
                ScanIndex,
                ScanList[ScanIndex].Band,
                TUNER_LNBToneToString(ScanList[ScanIndex].LNBToneState),
                TUNER_PolarityToString(ScanList[ScanIndex].Polarization),
                TUNER_FecToString(ScanList[ScanIndex].FECRates),
                ScanList[ScanIndex].SymbolRate / 1000,
                TUNER_ModulationToString(ScanList[ScanIndex].Modulation),
                ScanList[ScanIndex].AGC ? "ON":"OFF");
        TUNER_PRINT(TUNER_Msg);
    }

    return (ErrCode);

} /* end TUNER_SetScanList */

/*-------------------------------------------------------------------------
 * Function : TUNER_SetFrequency
 *            Function to set tuner frequency
 * Input    : Scan Parameters
 * Output   :
 * Return   : ErrorCode
 * ----------------------------------------------------------------------*/
static BOOL TUNER_SubSetFrequency( S32 Freq, STTUNER_Scan_t *ScanParams_p )
{
    ST_ErrorCode_t   ErrCode;
    S32              FreqkHz;

    /* convert both Frequency & Symbol Rate to external format */

    FreqkHz = Freq * 1000;
    ScanParams_p->SymbolRate = ScanParams_p->SymbolRate*1000;  /* convert to Mega Symbols */

    if (( ErrCode = STTUNER_SetFrequency(TUNERHandle, FreqkHz, ScanParams_p, 0)) != ST_NO_ERROR )
    {
        sprintf(TUNER_Msg, "STTUNER_SetFrequency(0x%08x,%d Mhz): ", TUNERHandle, Freq);
        TUNER_TextError(ErrCode, TUNER_Msg);
    }
    else
        ErrCode = WaitForTunerEvent();

    return (ErrCode);

} /* end TUNER_SetFrequency */

/*-------------------------------------------------------------------------
 * Function : TUNER_Setup
 *            Initialise drivers
 * Input    : None
 * Output   :
 * Return   : Error code
 * ----------------------------------------------------------------------*/
BOOL TUNER_Setup(STTST_Parse_t *pars_p, char *Result)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    S32 Region;
    ST_Revision_t           RevisionNumber;

    if ( STTST_GetInteger(pars_p, ST_REGION_EUROPE_DVB, (S32*)&Region))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_Region );
        return TRUE;
    }

    RevisionNumber = STTUNER_GetRevision();
    STTBX_Print(("STTUNER_GetRevision() : revision=%s\n", RevisionNumber));

    ErrCode = TUNER_init();
    if ( ErrCode == ST_NO_ERROR )
    {
        ErrCode = TUNER_Open();
        if ( ErrCode == ST_NO_ERROR )
        {
            STTUNER_Band_t  BandParams;
            STTUNER_Scan_t  ScanParams;

            ScanParams.Polarization = STTUNER_PLR_ALL;
            ScanParams.FECRates     = STTUNER_FEC_ALL;
            ScanParams.Modulation   = TUNER_MODE;
            ScanParams.AGC          = STTUNER_AGC_ENABLE;

            if ( Region == ST_REGION_USA_DVB )
            {
                /* USA Band */
                BandParams.BandStart = 12200;
                BandParams.BandEnd   = 12700;
                BandParams.LO        = 11250;
                BandParams.DownLink  = STTUNER_DOWNLINK_Ku;
                ErrCode = TUNER_SetBandList( TRUE, &BandParams );

                /* Scan for USA */
                ScanParams.Band         = 0;
                ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
                ScanParams.SymbolRate   = 20000;
                ErrCode |= TUNER_SetScanList( TRUE, &ScanParams );
            }
            else /* if ( Environment.Region == ST_REGION_EUROPE_DVB ) */
            {
                /* Low Band - 0 LNB */
                BandParams.BandStart = 10600;
                BandParams.BandEnd   = 11700;
                BandParams.LO        =  9750;
                BandParams.DownLink  = STTUNER_DOWNLINK_Ku;

                ErrCode |= TUNER_SetBandList( TRUE, &BandParams );

                /* High Band - 22kHz LNB */
                BandParams.BandStart = 11700;
                BandParams.BandEnd   = 12750;
                BandParams.LO        = 10600;
                BandParams.DownLink  = STTUNER_DOWNLINK_Ku;

                ErrCode |= TUNER_SetBandList( FALSE, &BandParams );

                /* Scans for Europe */
                ScanParams.Band         = 0;
                ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
                ScanParams.SymbolRate   = 22000;
                ErrCode = TUNER_SetScanList( TRUE, &ScanParams );

                ScanParams.Band         = 0;
                ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
                ScanParams.SymbolRate   = 27500;
                ErrCode |= TUNER_SetScanList( FALSE, &ScanParams );

                ScanParams.Band         = 1;
                ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
                ScanParams.SymbolRate   = 22000;
                ErrCode = TUNER_SetScanList( FALSE, &ScanParams );

                ScanParams.Band         = 1;
                ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
                ScanParams.SymbolRate   = 27500;
                ErrCode |= TUNER_SetScanList( FALSE, &ScanParams );
            }
        }
    }

    /* Ignore Error Code from tuner initialisation */
    STTBX_Print (("\nSelected configuration is: "));
#ifdef USE_TUNER_EVAL0299
    STTBX_Print (("Eval0299 board\n"));
#elif defined USE_TUNER_STEM_STV0299
    STTBX_Print (("STEM STV0299 (dual tuner)\nSwitch configuation should be:\n"));
    STTBX_Print (("     Switchs  | A0  | A1  | A2  | JP13| JP4\n"));
    STTBX_Print (("     ---------------------------------------\n"));
    STTBX_Print (("     Position | ON  | OFF | OFF | OFF | ON\n"));
#endif /* USE_TUNER_STEM_STV0299 */
    return ( ST_NO_ERROR );

} /* end TUNER_Setup */

/*-------------------------------------------------------------------------
 * Function : TUNER_Exit
 *            Terminate Tuner driver
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static BOOL TUNER_Exit( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STTUNER_TermParams_t TermParams;

    ErrCode = STTUNER_Close(TUNERHandle);

    sprintf(TUNER_Msg,"STTUNER_Close(0x%08x): ", TUNERHandle);

    TUNER_TextError(ErrCode, TUNER_Msg);

    if(ErrCode != ST_NO_ERROR)
    {
        TermParams.ForceTerminate = TRUE;
    }
    else
    {
        TermParams.ForceTerminate = FALSE;
    }

    if (( ErrCode = STTUNER_Term(STTUNER_DEVICE_NAME, &TermParams)) == ST_NO_ERROR )
    {
        semaphore_delete(TunerSemaphore_p);
    }

    sprintf(TUNER_Msg,"STTUNER_Term(%s): ", STTUNER_DEVICE_NAME);
    TUNER_TextError(ErrCode, TUNER_Msg);

    return (ErrCode == ST_NO_ERROR ? FALSE : TRUE);

} /* end TUNER_Exit */

/* Testtool Functions ---------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function : TUNER_GetInfo
 *            Testtool Function to return tuner info
 * Input    : STTST_Parse_t *pars_p
 * Output   : char *Result
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TUNER_GetInfo( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode;

    ErrCode = TUNER_DisplayInfo();

    return (ErrCode == ST_NO_ERROR ? FALSE : TRUE);
}

/*-------------------------------------------------------------------------
 * Function : TUNER_SetFrequency
 *            Testtool Function to set tuner frequency
 * Input    : STTST_Parse_t *pars_p
 * Output   : char *Result
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TUNER_SetFrequency( STTST_Parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode;
    S32             Freq;           /* Freq in MHz */
    S32             LVar;
    STTUNER_Scan_t  ScanParams;

    /* Set default param for structure fields */
    ScanParams.Polarization =   STTUNER_PLR_ALL;    /* (sat) */
    ScanParams.FECRates     =   STTUNER_FEC_NONE;   /* (sat & terr) */
    ScanParams.Modulation   =   STTUNER_MOD_NONE;
    ScanParams.LNBToneState =   STTUNER_LNB_TONE_DEFAULT;   /* (sat) */
    #if !defined(SAT5107)
    ScanParams.Mode         =   STTUNER_MODE_2K;            /* (ter) */
    ScanParams.Guard        =   STTUNER_GUARD_1_32;         /* (ter) */
    ScanParams.Force        =   STTUNER_FORCE_NONE;         /* (ter) */
    ScanParams.Hierarchy    =   STTUNER_HIER_NONE;          /* (ter) */
    ScanParams.Spectrum     =   STTUNER_INVERSION_NONE;     /* (ter & cable) */
    ScanParams.FreqOff      =   STTUNER_OFFSET_NONE;        /* (ter) */
    ScanParams.ChannelBW    =   STTUNER_CHAN_BW_6M;         /* (ter) */
    ScanParams.EchoPos      =   0;                          /* (ter) */
    #endif
    ScanParams.IQMode       =   STTUNER_IQ_MODE_NORMAL;     /* (sat) */
    ScanParams.Band         =   0;
    ScanParams.AGC          =   0;
    ScanParams.SymbolRate   =   0;
    #if !defined(SAT5107)
    ScanParams.J83          =   STTUNER_J83_NONE;           /* (cab) */
    ScanParams.SpectrumIs   =   STTUNER_INVERSION_NONE;     /* (cable) */
    #endif

    if ( STTST_GetInteger( pars_p, 0x0, (S32*)&Freq ) ||
         ( Freq == 0 ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_Frequency );
        return TRUE;
    }

    /* input Band */
    if ( STTST_GetInteger( pars_p, 0, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_Band );
        return TRUE;
    }
    ScanParams.Band = (U32)LVar;

    /* input Tone State */
    if ( STTST_GetInteger( pars_p, STTUNER_LNB_TONE_DEFAULT, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_LNBTone );
        return TRUE;
    }
    ScanParams.LNBToneState = (STTUNER_LNBToneState_t)LVar;

    /* input Polarity */
    if ( STTST_GetInteger( pars_p, STTUNER_PLR_VERTICAL, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_Polarity );
        return TRUE;
    }
    ScanParams.Polarization = (STTUNER_Polarization_t)LVar;

    /* Use default values for the rest */

    /* input FEC rate */
    if ( STTST_GetInteger( pars_p, STTUNER_FEC_3_4, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_FECRate );
    }
    ScanParams.FECRates = (STTUNER_FECRate_t)LVar;

    /* input Symbol Rate */
    if ( STTST_GetInteger( pars_p, 27500, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_SymbolRate );
    }
    ScanParams.SymbolRate = (U32)LVar;

    /* input Modulation */
    if ( STTST_GetInteger( pars_p, TUNER_MODE, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_Modulation);
    }
    ScanParams.Modulation = (STTUNER_Modulation_t)LVar;

    /* input AGC Enable */
    if ( STTST_GetInteger( pars_p, STTUNER_AGC_ENABLE, (S32*)&LVar ))
    {
        STTST_TagCurrentLine( pars_p, (char*) MSG_Expected_AGC_Enable );
    }
    ScanParams.AGC = (U32)LVar;

    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;

    ErrCode = TUNER_SubSetFrequency( Freq, &ScanParams );
    STTST_AssignInteger(Result, ErrCode, FALSE);

    return (ErrCode == ST_NO_ERROR ? FALSE : TRUE);

} /* end TUNER_SetFrequency */

/*-------------------------------------------------------------------------
 * Function : TUNER_RegisterCommands
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TUNER_RegisterCmd( void )
{
    BOOL RetErr = FALSE;

    /* Register Test Functions */
    RetErr |= STTST_RegisterCommand("TUNER_SETUP", TUNER_Setup, "Setup Tuner <Region>");
    RetErr |= STTST_RegisterCommand("TUNER_EXIT", TUNER_Exit, "Exit Tuner");

    RetErr |= STTST_RegisterCommand("TUNER_CONT", TUNER_Continue, "Continue Tuner Scan");
    RetErr |= STTST_RegisterCommand("TUNER_INFO", TUNER_GetInfo,"Display Tuner Info");
    RetErr |= STTST_RegisterCommand("TUNER_SCAN", TUNER_Scan, "<Start> <End> Start Tuner Scan");
    RetErr |= STTST_RegisterCommand("TUNER_SETFREQ", TUNER_SetFrequency,
                                    "<Freq><BandNum><LNBTone><Polarity><FEC><Symbol><Mod> Set Tuner Frequency");
    /* Assign Variables */

    RetErr |= STTST_AssignInteger("VERT",   STTUNER_PLR_VERTICAL, TRUE);
    RetErr |= STTST_AssignInteger("HORIZ",  STTUNER_PLR_HORIZONTAL, TRUE);
    RetErr |= STTST_AssignInteger("ALLPLR", STTUNER_PLR_ALL, TRUE);
    RetErr |= STTST_AssignInteger("QPSK",   STTUNER_MOD_QPSK, TRUE);
    RetErr |= STTST_AssignInteger("MODALL", STTUNER_MOD_ALL, TRUE);
    RetErr |= STTST_AssignInteger("LNBDEF", STTUNER_LNB_TONE_DEFAULT, TRUE);
    RetErr |= STTST_AssignInteger("LNBOFF", STTUNER_LNB_TONE_OFF, TRUE);
    RetErr |= STTST_AssignInteger("LNBON",  STTUNER_LNB_TONE_22KHZ, TRUE);
    RetErr |= STTST_AssignInteger("FEC_ALL", STTUNER_FEC_ALL, TRUE);
    RetErr |= STTST_AssignInteger("FEC_1_2", STTUNER_FEC_1_2, TRUE);
    RetErr |= STTST_AssignInteger("FEC_2_3", STTUNER_FEC_2_3, TRUE);
    RetErr |= STTST_AssignInteger("FEC_3_4", STTUNER_FEC_3_4, TRUE);
    RetErr |= STTST_AssignInteger("FEC_4_5", STTUNER_FEC_4_5, TRUE);
    RetErr |= STTST_AssignInteger("FEC_5_6", STTUNER_FEC_5_6, TRUE);
    RetErr |= STTST_AssignInteger("FEC_6_7", STTUNER_FEC_6_7, TRUE);
    RetErr |= STTST_AssignInteger("FEC_7_8", STTUNER_FEC_7_8, TRUE);
    RetErr |= STTST_AssignInteger("FEC_8_9", STTUNER_FEC_8_9, TRUE);

    if ( !RetErr )
    {
        sprintf( TUNER_Msg, "TUNER_RegisterCmd() \t: ok           %s\n", STTUNER_GetRevision() );
    }
    else
    {
        sprintf( TUNER_Msg, "TUNER_RegisterCmd() \t: failed !\n");
    }
    TUNER_PRINT(TUNER_Msg);

    return(!RetErr);

} /* end TUNER_RegisterCommand */

/* EOF */
