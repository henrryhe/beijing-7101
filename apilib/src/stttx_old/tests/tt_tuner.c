/*****************************************************************************

File name   : tt_tuner.c

Description : Testtool Tuner Command

COPYRIGHT (C) STMicroelectronics 1999.

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>

#include "stddefs.h"  /* standard definitions                    */

#include "stdenc.h"
#include "sttuner.h"
#include "sttbx.h"
#include "pti.h"
#include "testtool.h"

/*
#include "app_data.h"
#include "tt_api.h"
#include "tt_tuner.h"
*/

/* Global Definitions ----------------------------------------------------- */

#define MAX_BANDS              3
#define MAX_SCANS              12
#define MAX_SIGNALS            10

/* Global Variables ------------------------------------------------------- */

extern STTUNER_Handle_t  TUNERHandle;

/* Function prototypes ---------------------------------------------------- */
void AwaitTunerEvent(STTUNER_EventType_t *EventType_p);


/* Local Variables -------------------------------------------------------- */

static STTUNER_EventType_t     LastEvent;

static STTUNER_BandList_t      Bands;
static STTUNER_Band_t          BandList[MAX_BANDS];
static STTUNER_ScanList_t      Scans;
static STTUNER_Scan_t          ScanList[MAX_SCANS];
static U32 API_ErrorCount = 0;
static U32 API_EnableError = 0;

/*  Tuner Display functions -------------------------------------------- */
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
        if (( 1 << i ) & bit )
            break;
    }

    return ++i;

} /* end bit2num */

/*-------------------------------------------------------------------------
 * Function : TUNER_FecToString
 *            Convert Forward Error Correction value to text
 * Input    : FEC value
 * Output   :
 * Return   : pointer to FEC text
 * ----------------------------------------------------------------------*/
char *TUNER_FecToString(STTUNER_FECRate_t Fec)
{
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
        return FecRate[bit2num(Fec)];

} /* end TUNER_FecToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_PolarizationToString
 *            Convert Polarization value to text
 * Input    : Polarization value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
char *TUNER_PolarityToString(STTUNER_Polarization_t Plr)
{
    static char Polarity[][8] =
    {
        "Invalid",
        "HORIZ",
        "VERT",
        "ALL"
    };

    return Polarity[Plr];;

} /* end TUNER_PolarityToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_EventToString
 *            Convert Event value to text
 * Input    : Event value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
char *TUNER_EventToString(STTUNER_EventType_t Evt)
{
    static char EventDescription[][25] =
    {
        "STTUNER_EV_NO_OPERATION",
        "STTUNER_EV_LOCKED",
        "STTUNER_EV_UNLOCKED",
        "STTUNER_EV_SCAN_FAILED",
        "STTUNER_EV_TIMEOUT"
    };

    return EventDescription[Evt];

} /* end TUNER_EventToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_StatusToString
 *            Convert Status value to text
 * Input    : Event value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
char *TUNER_StatusToString(STTUNER_Status_t Status)
{
    static char StatusDescription[][10] =
    {
        "UNLOCKED",
        "SCANNING",
        "LOCKED",
        "NOT_FOUND"
    };

    return StatusDescription[Status];

} /* end TUNER_StatusToString */

/* Testtool Functions ---------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function : TTTUNER_Continue
 *            Function to continue scanning
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTUNER_Continue( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TUNER_Continue()) != ST_NO_ERROR )
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTUNER_Continue */

/*-------------------------------------------------------------------------
 * Function : TTTUNER_DisplayInfo
 *            Function to Display Tuner Info
 * Input    :  *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTUNER_DisplayInfo( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t          ErrCode;

    if (( ErrCode = TUNER_DisplayInfo()) != ST_NO_ERROR )
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TUNER_DisplayInfo */


/*-------------------------------------------------------------------------
 * Function : TTTUNER_Scan
 *            TESTTOOL Function to Start scanning
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTUNER_Scan( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t   ErrCode;
    int              ScanStartFreq;          /* Freq in MHz */
    int              ScanEndFreq;

    cget_integer( pars_p, 0x0, (long *)&ScanStartFreq );
    if ( (int) ScanStartFreq == 0 )
    {
        STTBX_Print(("expected Start Frequency\n"));
        return TRUE;
    }

    cget_integer( pars_p, 0x0, (long *)&ScanEndFreq );
    if ( (int) ScanEndFreq == 0 )
    {
        ScanEndFreq = ScanStartFreq;
    }

    if (( ErrCode =  TUNER_Scan( ScanStartFreq, ScanEndFreq )) != ST_NO_ERROR )
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

}  /* end TTTUNER_Scan */


/*-------------------------------------------------------------------------
 * Function : TTTUNER_SetFrequency
 *            Testtool Function to set tuner frequency
 * Input    : *pars_p, *Result
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static BOOL TTTUNER_SetFrequency( parse_t *pars_p, char *Result )
{
    ST_ErrorCode_t  ErrCode;
    S32             Freq;           /* Freq in MHz */
    STTUNER_Scan_t  ScanParams;

    cget_integer( pars_p, 0x0, (long *)&Freq );
    if ( (int) Freq == 0 )
    {
        STTBX_Print(("expected Frequency\n"));
        return TRUE;
    }

    /* input Band */
    if ( cget_integer( pars_p, 0, (long *)&ScanParams.Band ))
        STTBX_Print(("expected Band Index\n" ));

    /* input Tone State */
    if ( cget_integer( pars_p, STTUNER_LNB_TONE_DEFAULT, (long *)&ScanParams.LNBToneState ))
        STTBX_Print(("expected LNB Tone State (%d=OFF, %d=22KHZ, %d=DEFAULT)\n",
            STTUNER_LNB_TONE_OFF, STTUNER_LNB_TONE_22KHZ, STTUNER_LNB_TONE_DEFAULT ));

    /* input Polarity */
    if ( cget_integer( pars_p, STTUNER_PLR_VERTICAL, (long *)&ScanParams.Polarization ))
        STTBX_Print(("expected Polarity (VERT, HORIZ)\n" ));

    /* input FEC rate */
    if ( cget_integer( pars_p, STTUNER_FEC_3_4, (long *)&ScanParams.FECRates ))
        STTBX_Print(("expected FECRates (0x%x=1/2,0x%x=2/3,0x%x=3/4, etc.)\n",
            STTUNER_FEC_1_2, STTUNER_FEC_2_3, STTUNER_FEC_3_4 ));

    /* input Symbol Rate */
    if ( cget_integer( pars_p, 27500, (long *)&ScanParams.SymbolRate ))
        STTBX_Print(("expected SymbolRate\n" ));

    /* input Modulation */
    if ( cget_integer( pars_p, STTUNER_MOD_QPSK, (long *)&ScanParams.Modulation ))
        STTBX_Print(("expected Modulation (%d=QPSK)\n", STTUNER_MOD_QPSK ));

    /* input AGC Enable */
    if ( cget_integer( pars_p, STTUNER_AGC_ENABLE, (long *)&ScanParams.AGC ))
        STTBX_Print(("expected AGC Enable(%d)/Disable(%d\n",
             STTUNER_AGC_DISABLE, STTUNER_AGC_ENABLE ));

    if (( ErrCode = TUNER_SetFrequency( Freq, &ScanParams )) != ST_NO_ERROR )
        API_ErrorCount++;

    return ( API_EnableError ? (ErrCode == ST_NO_ERROR ? FALSE : TRUE) : FALSE );

} /* end TTTUNER_SetFrequency */


/* Global Functions -----------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function : TUNER_Close
 *            Function to Close Tuner
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_Close( void )
{
    ST_ErrorCode_t          ErrCode;

    ErrCode = STTUNER_Close(TUNERHandle);

    STTBX_Print(("STTUNER_Close(0x%08x)=%s\n", TUNERHandle, GetErrorText(ErrCode) ));

    return (ErrCode);

} /* end TUNER_Close */

/*-------------------------------------------------------------------------
 * Function : TUNER_Continue
 *            Function to continue scanning
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TUNER_Continue( void )
{
    ST_ErrorCode_t          ErrCode;
    STTUNER_TunerInfo_t     TUNERInfo;

    if (( ErrCode = STTUNER_GetTunerInfo(TUNERHandle, &TUNERInfo)) != ST_NO_ERROR )
    {
        STTBX_Print(("STTUNER_GetTunerInfo(0x%08x)=%s\n", TUNERHandle, GetErrorText(ErrCode)));
        return TRUE;
    }

    if (( ErrCode = STTUNER_ScanContinue(TUNERHandle, 0)) != ST_NO_ERROR )
        STTBX_Print(("STTUNER_ScanContinue(0x%08x)=%s\n", TUNERHandle, GetErrorText(ErrCode) ));
    else
    {
        STTUNER_EventType_t Event;

        STTBX_Print(("Tuner Continue from %d kHz (%s) ... ",
            TUNERInfo.Frequency, TUNER_PolarityToString(TUNERInfo.ScanInfo.Polarization) ));

        /* Wait for notify function to return */
        AwaitTunerEvent(&Event);

        if ( Event != STTUNER_EV_LOCKED )
        {
            STTBX_Print(("Failed\n"));
            return TRUE;
        }
        else
            STTBX_Print(("Locked\n"));
    }

    return (ErrCode == ST_NO_ERROR ? FALSE : TRUE);

} /* end TUNER_Continue */

/*-------------------------------------------------------------------------
 * Function : TUNER_DisplayInfo
 *            Function to Display Tuner Info
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_DisplayInfo( void )
{
    ST_ErrorCode_t          ErrCode;
    STTUNER_TunerInfo_t     TUNERInfo;

    if (( ErrCode = STTUNER_GetTunerInfo(TUNERHandle, &TUNERInfo)) != ST_NO_ERROR )
        STTBX_Print(("STTUNER_GetTunerInfo(0x%08x)=%s\n", TUNERHandle, GetErrorText(ErrCode)));
    else
        STTBX_Print(("F = %u MHz (%s), FEC = %s, Status \'%s\', Quality = %u%%, BER %d\n",
            TUNERInfo.Frequency / 1000,
            TUNER_PolarityToString(TUNERInfo.ScanInfo.Polarization),
            TUNER_FecToString(TUNERInfo.ScanInfo.FECRates),
            TUNER_StatusToString(TUNERInfo.Status),
            TUNERInfo.SignalQuality,
            TUNERInfo.BitErrorRate ));

    return (ErrCode);

} /* end TUNER_DisplayInfo */


/*-------------------------------------------------------------------------
 * Function : TUNER_Scan
 *            Function to Start Tuner Scan
 * Input    : ScanStart (MHz), ScanEnd (MHz)
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TUNER_Scan( int ScanStart, int ScanEnd )
{
    ST_ErrorCode_t          ErrCode;

    /* Scan from ScanStart to ScanEnd */
    if (( ErrCode = STTUNER_Scan(TUNERHandle, (ScanStart*1000), (ScanEnd*1000), 0, 0)) != ST_NO_ERROR )
    {
        STTBX_Print(("STTUNER_Scan(0x%08x, %d,%d)=%s\n", TUNERHandle,
            ScanStart, ScanEnd, GetErrorText(ErrCode) ));
        return TRUE;
    }
    else
    {
        STTUNER_EventType_t Event;

        STTBX_Print(("Tuner Scanning from %d to %d MHz ... ", ScanStart, ScanEnd ));

        /* Wait for notify function to return */
        AwaitTunerEvent(&Event);

        if ( Event != STTUNER_EV_LOCKED )
        {
            STTBX_Print(("Failed\n"));
            return TRUE;
        }
        else
            STTBX_Print(("Locked\n"));
    }

    return FALSE;

} /* end TUNER_ScanStart */

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
    if ( NewNotAdd == TRUE )
        Scans.NumElements = 1;
    else if ( Scans.NumElements < MAX_SCANS )
        Scans.NumElements ++;
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Max Scan Elements reached\n", MAX_SCANS ));
        return ST_ERROR_NO_FREE_HANDLES;
    }

    ScanIndex = Scans.NumElements-1;
    ScanList[ScanIndex].Polarization = ScanParams_p->Polarization;
    ScanList[ScanIndex].FECRates     = ScanParams_p->FECRates;
    ScanList[ScanIndex].Modulation   = ScanParams_p->Modulation;
    ScanList[ScanIndex].LNBToneState = ScanParams_p->LNBToneState;
    ScanList[ScanIndex].Band         = ScanParams_p->Band;
    ScanList[ScanIndex].AGC          = ScanParams_p->AGC;
    ScanList[ScanIndex].SymbolRate   = ScanParams_p->SymbolRate*1000;  /* convert to M/Symbols */

    if (( ErrCode = STTUNER_SetScanList(TUNERHandle, &Scans)) != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STTUNER_SetScanList(0x%08x)=%s\n",
            TUNERHandle, GetErrorText(ErrCode) ));
    }
    else
        STTBX_Print(("Scan #%d = Pol %s, FEC %s, Mod %d, LNB %d, Band %u, AGC %u SRate %u\n",
            ScanIndex,
            TUNER_PolarityToString(ScanList[ScanIndex].Polarization),
            TUNER_FecToString(ScanList[ScanIndex].FECRates),
            ScanList[ScanIndex].Modulation,
            ScanList[ScanIndex].LNBToneState,
            ScanList[ScanIndex].Band,
            ScanList[ScanIndex].AGC,
            ScanList[ScanIndex].SymbolRate ));

    return (ErrCode);

} /* end TUNER_SetScanList */

/*-------------------------------------------------------------------------
 * Function : TUNER_SetFrequency
 *            Function to set tuner frequency
 * Input    : Scan Parameters
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TUNER_SetFrequency( S32 Freq, STTUNER_Scan_t *ScanParams_p )
{
    ST_ErrorCode_t   ErrCode;
    S32              FreqkHz;
    U32              BandIndex;

    FreqkHz = Freq * 1000;

    /* if Band not set, find a Band containing Freq */
    if ( ScanParams_p->Band > Bands.NumElements )
    {
        BandIndex = 1; /* Europe DVB */

        for ( ; BandIndex < Bands.NumElements; BandIndex++ )
        {
            if (( FreqkHz > Bands.BandList[BandIndex].BandStart ) &&
                ( FreqkHz < Bands.BandList[BandIndex].BandEnd ))
            {
                ScanParams_p->Band = BandIndex;
                break;
            }
        }
    }

    /* if LNBTone not set, assume using default Band List */
    /*   band 1 = STTUNER_LNB_TONE_OFF */
    /*   band 2 = STTUNER_LNB_TONE_22KHZ */
    if (( ScanParams_p->LNBToneState != STTUNER_LNB_TONE_DEFAULT ) &&
        ( ScanParams_p->LNBToneState != STTUNER_LNB_TONE_OFF ) &&
        ( ScanParams_p->LNBToneState != STTUNER_LNB_TONE_22KHZ ))
    {
        if ( ScanParams_p->Band == 1 )
            ScanParams_p->LNBToneState = STTUNER_LNB_TONE_OFF;
        else if ( ScanParams_p->Band == 2 )
            ScanParams_p->LNBToneState = STTUNER_LNB_TONE_22KHZ;
        else
            ScanParams_p->LNBToneState = STTUNER_LNB_TONE_DEFAULT;
    }

    if (( ScanParams_p->AGC != STTUNER_AGC_ENABLE ) &&
        ( ScanParams_p->AGC != STTUNER_AGC_DISABLE ))
        ScanParams_p->AGC = STTUNER_AGC_ENABLE;

    ScanParams_p->SymbolRate = ScanParams_p->SymbolRate*1000;  /* convert to Mega Symbols */

    STTBX_Print(("Freq %d, Polarity %s, FEC Rates 0x%x, Mod %d, LNB %d, Band %u, AGC %u SRate %u\n",
        FreqkHz,
        TUNER_PolarityToString(ScanParams_p->Polarization),
        ScanParams_p->FECRates,
        ScanParams_p->Modulation,
        ScanParams_p->LNBToneState,
        ScanParams_p->Band,
        ScanParams_p->AGC,
        ScanParams_p->SymbolRate ));

    if (( ErrCode = STTUNER_SetFrequency(TUNERHandle, FreqkHz, ScanParams_p, 0)) != ST_NO_ERROR )
        STTBX_Print(("STTUNER_SetFrequency(0x%08x, %d)=%s\n", TUNERHandle, Freq, GetErrorText(ErrCode) ));
    else
    {
        STTUNER_EventType_t Event;

        STTBX_Print(("Tuner Searching for %d MHz ... ", Freq ));

        /* Wait for notify function to return */
        AwaitTunerEvent(&Event);

        if ( Event != STTUNER_EV_LOCKED )
        {
            STTBX_Print(("Failed\n"));
            return TRUE;
        }
    else
            STTBX_Print(("Locked\n"));
    }

    return (ErrCode == ST_NO_ERROR ? FALSE : TRUE);

} /* end TUNER_SetFrequency */


/*-------------------------------------------------------------------------
 * Function : TUNER_InitCommands
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TUNER_InitCommand( void )
{
    BOOL RetErr;

    RetErr = FALSE;

    Bands.BandList = BandList;

    RetErr |= register_command("TUNER_CONT",     TTTUNER_Continue, "Continue Tuner Scan");
    RetErr |= register_command("TUNER_INFO",     TTTUNER_DisplayInfo,"Display Tuner Info");
    RetErr |= register_command("TUNER_SCAN",     TTTUNER_Scan, "<Start> <End> Start Tuner Scan");
    RetErr |= register_command("TUNER_SETFREQ",  TTTUNER_SetFrequency, "<Freq><BandNum><LNBTone><Polarity><FEC><Symbol><Mod> Set Tuner Frequency");

    /* Assign Variables */

    RetErr |= assign_integer("VERT",   STTUNER_PLR_VERTICAL, TRUE);
    RetErr |= assign_integer("HORIZ",  STTUNER_PLR_HORIZONTAL, TRUE);
    RetErr |= assign_integer("ALLPOL", STTUNER_PLR_ALL, TRUE);
    RetErr |= assign_integer("QPSK",   STTUNER_MOD_QPSK, TRUE);
    RetErr |= assign_integer("LNBOFF", STTUNER_LNB_TONE_OFF, TRUE);
    RetErr |= assign_integer("LNBON",  STTUNER_LNB_TONE_22KHZ, TRUE);

    STTBX_Print(("TUNER_InitCommand() : macros registrations "));
    if ( RetErr == TRUE )
        STTBX_Print(("failure !\n"));
    else
        STTBX_Print(("ok\n"));

    return( RetErr );

}

/* EOF */

