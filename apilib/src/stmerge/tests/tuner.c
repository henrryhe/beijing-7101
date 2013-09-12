/*****************************************************************************

File name  : tuner.c
Description: tuner driver interface

COPYRIGHT (C) STMicroelectronics 2004.

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <string.h>

#include "stddefs.h"  /* standard definitions */
#include "stcommon.h"
#include "tuner.h"
#include "stmergetst.h"

/* Private Types ------------------------------------------------------ */
/* parameters for tuner ioctl call 'STTUNER_IOCTL_PROBE' */
typedef struct TUNER_ProbeParams_s
{
    STTUNER_IODriver_t Driver;
    ST_DeviceName_t    DriverName;
    U32                Address;
    U32                SubAddress;
    U32                Value;
    U32                XferSize;
    U32                TimeOut;
} TUNER_ProbeParams_t;


/* Private Constants -------------------------------------------------- */


/* Private Variables -------------------------------------------------- */

static int  I2C_Bus = -1;
static U8   LnbAddress;
TUNER_Type_t TunerType = TUNER_UNKNOWN;

#ifndef ST_OS21
static semaphore_t TunerEventSemaphore;
#endif

static semaphore_t *TunerEventSemaphore_p = NULL;

static STTUNER_EventType_t     LastEvent = STTUNER_EV_NO_OPERATION;

/* Private Macros ----------------------------------------------------- */

#define DEMOD_360_AND_297_I2C_ADDRESS   0x38  /* !! Shared address */
#define DEMOD_360_I2C_ADDRESS2          0x3A
#define DEMOD_360_I2C_ADDRESS3          0x3C
#define DEMOD_360_I2C_ADDRESS4          0x3E

#define DEMOD_299_399_I2C_ADDRESS1      0xD0 /*299 & one 399 I2C Addresses are same*/
#define DEMOD_299_399_I2C_ADDRESS2      0xD2 /*299 & one 399 I2C Addresses are same*/

/* Global Variables ------------------------------------------------------- */
extern ST_ClockInfo_t      ST_ClockInfo;
extern STI2C_Handle_t  I2C_Handle[];
extern ST_DeviceName_t I2C_DeviceName[];

STTUNER_Handle_t   TUNER_Handle;
ST_DeviceName_t    TUNER_DeviceName = "TUNER";

STTUNER_Band_t     TUNER_Bands[MAX_BANDS];
STTUNER_BandList_t TUNER_BandList;

STTUNER_Scan_t     TUNER_Scans[MAX_SCANS];
STTUNER_ScanList_t TUNER_ScanList;

STTUNER_SignalThreshold_t  TUNER_Thresholds[MAX_THRESHOLDS];
STTUNER_ThresholdList_t    TUNER_ThresholdList;

/* Global Functions ----------------------------------------------------- */
/*-------------------------------------------------------------------------
 * Function : NotifyCallback
 * Input    : called when a tuner event happens.
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
void NotifyCallback(STEVT_CallReason_t     Reason,
                    const ST_DeviceName_t  RegistrantName,
                    STEVT_EventConstant_t  Event,
                    const void            *EventData,
                    const void            *SubscriberData_p)
{
    LastEvent = (STTUNER_EventType_t) Event;
    semaphore_signal(TunerEventSemaphore_p);
}
/*-------------------------------------------------------------------------
 * Function : TUNER_Setup
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_Setup(SERVICE_Mode_t SERVICE_Mode)
{
    ST_ErrorCode_t                Error = ST_NO_ERROR;
    ST_ErrorCode_t                StoredError = ST_NO_ERROR;
    STTUNER_InitParams_t          TUNER_InitParams;
    STTUNER_OpenParams_t          TUNER_OpenParams;
    TUNER_ProbeParams_t           ProbeParams;
    U8                            index = 0;
    U8                            AddressIndex=0;        
    int                           ScanBus[] =  \
                                  {
                                      I2C_BACK_BUS, 
                                      I2C_FRONT_BUS,
                                      -1
                                  };
    
    U32                           AddressList[]= \
                                  {

                                      DEMOD_299_399_I2C_ADDRESS1,
                                      DEMOD_299_399_I2C_ADDRESS2,
                                      DEMOD_360_AND_297_I2C_ADDRESS,
                                      DEMOD_360_I2C_ADDRESS2,
                                      DEMOD_360_I2C_ADDRESS3,
                                      DEMOD_360_I2C_ADDRESS4,
                                      0xFF
                                  }; 
                                  
    /* Now TUNER setup is done */
    /* Look for tuner on both I2C busses for first pass of tuner open
     * (see also: sttuner satellite unitary test "sat_test.c" )
     */
    if (I2C_Bus == -1)
    {
        ProbeParams.Driver     = STTUNER_IODRV_I2C;
        ProbeParams.SubAddress = 0;
        ProbeParams.XferSize   = 1;
        ProbeParams.TimeOut    = 50;

        while( (ScanBus[index] != -1) && (I2C_Bus == -1) )
        {
            strcpy(ProbeParams.DriverName, I2C_DeviceName[ScanBus[index]]);  /* set I2C bus to scan */

            for( AddressIndex = 0; AddressList[AddressIndex] != 0xFF; AddressIndex ++)
            {
                ProbeParams.Value = 0;    /* clear value for return */
                ProbeParams.Address = AddressList[AddressIndex]; /* address of next chip to try */
                                
                Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
                if ( Error != ST_NO_ERROR )
                {
                    if (( Error != ST_ERROR_TIMEOUT) &&
                        ( Error != STI2C_ERROR_ADDRESS_ACK ))
                    {
                        return( Error );
                    }
                }
                else /* no error */
                {
                    switch(ProbeParams.Address)
                    {

                        case DEMOD_299_399_I2C_ADDRESS1:  /* Demod address from 299 */
                       
                            switch(ProbeParams.Value & 0xF0)
                            {
                                /* Value of the software version in 299 register */ 
                                case 0xA0:
                                    STTBX_Print(("Found STV0299 tuner on bus '%s'\n", I2C_DeviceName[ ScanBus[index] ] ));
                                    I2C_Bus = ScanBus[index];
                                    TunerType = TUNER_SAT_299_EVAL;

                                    /* check to see if 299 tuner is on a DUAL STEM card by checking that address */
                                    /* NOTES:
                                    1. I2C address 0x40 is already in use by a PCF8575 on the mb361. 
                                       This PCF8575 default address can be changed by option resistors on the mb361 board. 
                                    2. If the STEM switches J21,J22 & J23 (A0, A1, A2) are all OFF (address 0x40)
                                       then the STEM board LNB control register will NOT be detected.
                                       The TunerType will remain as EVAL0299 with the result that STTUNER Init() and Open()
                                       may work but no frequency may be tuned to. */
                                    for (LnbAddress = 0x42; LnbAddress <= 0x4E; LnbAddress +=2)
                                    {
                                        ProbeParams.Value = 0;    /* clear value for return */
                                        ProbeParams.Address = LnbAddress; /* address of DUAL STEM LNB i/o port */

                                        Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID,
                                                                     STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
                                        if (Error != ST_NO_ERROR)
                                        {
                                            if (( Error != ST_ERROR_TIMEOUT) &&
                                                ( Error != STI2C_ERROR_ADDRESS_ACK ))
                                            {                                         
                                                
                                                return( Error );
                                            }
                                        }
                                        else
                                        {
                                            STTBX_Print(("Reply from address 0x%02x indicating DUAL STEM card.\n", LnbAddress));
                                            TunerType = TUNER_SAT_299_STEM;
                                            break;  /* out of for(LnbAddress) */
                                        }

                                    }   /* for(LnbAddress) */
                                    break;
                                    
                                /* Value of the software version in 399 register */ 
                                case 0xB0:  /* the release number of the circuit (in ID register) in order to ensure software compatibility.*/
                                    STTBX_Print(("Found STV0399 tuner on bus '%s'\n", I2C_DeviceName[ ScanBus[index] ] ));
                                    I2C_Bus = ScanBus[index];
                                    TunerType=TUNER_SAT_399_EVAL;                  
                                    break;
                                    
                                default:
                                    break;
                            } /* switch(ProbeParams.Value & 0xF0) */                        
                            break;
                            
                         case DEMOD_299_399_I2C_ADDRESS2:
                            switch(ProbeParams.Value & 0xF0)
                            {
                                /* Value of the software version in 299 register */
                                case 0xA0:
                                    STTBX_Print(("Found STV0299 tuner on bus '%s', DemodAddress %d\n",
                                        I2C_DeviceName[ ScanBus[index] ], ProbeParams.Address  ));
                                    I2C_Bus = ScanBus[index];
                                    TunerType    = TUNER_SAT_299_EVAL;
                                    break;
                                
                                /* Value of the software version in 399 register */ 
                                case 0xB0:  /* the release number of the circuit (in ID register) in order to ensure software compatibility.*/
                                    STTBX_Print(("Found STV0399 tuner on bus '%s'\n", I2C_DeviceName[ ScanBus[index] ] ));
                                    I2C_Bus = ScanBus[index];
                                    TunerType=TUNER_SAT_399_EVAL;                  
                                    break;

                                default:
                                    break;
                            } /* switch(ProbeParams.Value & 0xF0) */

                            break;               

                        case DEMOD_360_AND_297_I2C_ADDRESS:  /* Demod address for 360 and 297 */
                            switch(ProbeParams.Value)
                            {
                                /* reset value of sub-address 0x00 of 360 */
                                case 0x20:
                                case 0x21:
                                    STTBX_Print(("Found STV0360 tuner on bus '%s'\n", I2C_DeviceName[ScanBus[index]]));
                                    I2C_Bus = ScanBus[index];
                                    TunerType = TUNER_TER_360;
                                    break;
                                /* reset value of sub-address 0x00 of 297 */
                                case 0x09:
                                    STTBX_Print(("Found STV0297 tuner on bus '%s'\n", I2C_DeviceName[ScanBus[index]]));
                                    I2C_Bus = ScanBus[index];
                                    TunerType = TUNER_CAB_297;
                                    break;

                                default:
                                    break;
                                    
                            } /* switch(ProbeParams.Value & 0xF0) */                            
                                                    
                            break;

                        case DEMOD_360_I2C_ADDRESS2: /* Other possible demod addresses for 360 */
                        case DEMOD_360_I2C_ADDRESS3:
                        case DEMOD_360_I2C_ADDRESS4:
                            switch(ProbeParams.Value & 0xF0)
                            {
                                /* Value of software version register for 360 */ 
                                case 0x20:
                                case 0x21:
                                    STTBX_Print(("Found STV0360 tuner on bus '%s'\n", I2C_DeviceName[ScanBus[index]]));
                                    I2C_Bus = ScanBus[index];
                                    TunerType = TUNER_TER_360;
                                    break;

                                default:
                                    break;
                            } /* switch(ProbeParams.Value & 0xF0) */                         
                            break;
                            
                        default:
                            break;                        
                            
                    } /* switch(ProbeParams.Address) */
                    
                } /* if errorcode ... */

            } /* for(AddressList .... */

            /* Start looking at the start of the address list on the next bus */
            index++;

        }   /* while(ScanBus[index]) */

        /* failed to find tuner? */
        if (I2C_Bus == -1)
        {
            STTBX_Print(("Failed to find tuner!\n"));
            return( ST_ERROR_UNKNOWN_DEVICE );
        }
    }    
    
    
#ifndef ST_OS21
        TunerEventSemaphore_p = &TunerEventSemaphore;
#endif
        TunerEventSemaphore_p = semaphore_create_fifo(0);

#if defined(ST_5100) || defined(ST_5301)
    /* Found tuner POKE for EPLD TS routing for 299 */
    if ( TunerType == TUNER_SAT_299_STEM  /*|| TunerType == TUNER_SAT_299_EVAL*/)
    {        
        *((U32*)0x41800000) = 0x00080008;
    }    
#endif

    /* Set Init Params */
    TUNER_InitParams.Max_BandList      = (U32) MAX_BANDS;
    TUNER_InitParams.Max_ScanList      = MAX_SCANS;
    TUNER_InitParams.Max_ThresholdList = MAX_THRESHOLDS;
    TUNER_InitParams.DriverPartition   = DriverPartition;
    
    switch(TunerType)
    {
        case TUNER_SAT_299_STEM:
        case TUNER_SAT_299_EVAL:

            /* Set Init Params - STV0299 Satellite tuner */
            TUNER_InitParams.Device            = STTUNER_DEVICE_SATELLITE;
            TUNER_InitParams.DemodType         = STTUNER_DEMOD_STV0299;
            TUNER_InitParams.DemodIO.Route     = STTUNER_IO_DIRECT;
            TUNER_InitParams.DemodIO.Driver    = STTUNER_IODRV_I2C;
            TUNER_InitParams.DemodIO.Address   = 0xD0;
            /* Selecteable depending on sat board type */
            if ( TunerType == TUNER_SAT_299_STEM )
            {
                TUNER_InitParams.TunerType         = STTUNER_TUNER_HZ1184;
                TUNER_InitParams.TunerIO.Route     = STTUNER_IO_REPEATER;
                TUNER_InitParams.TunerIO.Driver    = STTUNER_IODRV_I2C;
                TUNER_InitParams.TunerIO.Address   = 0xC0;
                TUNER_InitParams.LnbType           = STTUNER_LNB_STEM;
                TUNER_InitParams.LnbIO.Route       = STTUNER_IO_DIRECT;
                TUNER_InitParams.LnbIO.Driver      = STTUNER_IODRV_I2C;
                TUNER_InitParams.LnbIO.Address     = LnbAddress;
                STTBX_Print(("TUNER_Setup(299_DUAL_STEM,"));
            }
            else    /* EVAL0299 */
            {
                /*
                ** If a 299 STEM board is in the system and set to address 0x40
                ** (not detected, treated as an EVAL0299 board)
                **  then give developer a clue as to why the tuner may init and open but won't tune.
                */
                STTBX_Print(("NB: If using 299 STEM then change STEM switch/jumper J21, 22 or 23 to ON\n"));
#ifdef  TUNER_TYPE_MAX2116
                TUNER_InitParams.TunerType         = STTUNER_TUNER_MAX2116;
                TUNER_InitParams.TunerIO.Address   = 0xC8;
#else
                TUNER_InitParams.TunerType         = STTUNER_TUNER_STB6000;
                TUNER_InitParams.TunerIO.Address   = 0xC0;
#endif                
                TUNER_InitParams.TunerIO.Route     = STTUNER_IO_REPEATER;
                TUNER_InitParams.TunerIO.Driver    = STTUNER_IODRV_I2C;
                TUNER_InitParams.LnbType           = STTUNER_LNB_STV0299;
                TUNER_InitParams.LnbIO.Route       = STTUNER_IO_PASSTHRU;
                TUNER_InitParams.LnbIO.Driver      = STTUNER_IODRV_I2C;
                TUNER_InitParams.LnbIO.Address     = 0x00; /* dummy value */            
                STTBX_Print(("TUNER_Setup(EVAL0299,"));
            }
            
            TUNER_InitParams.ExternalClock     = 4000000;
            TUNER_InitParams.TSOutputMode      = STTUNER_TS_MODE_SERIAL; /* or STTUNER_TS_MODE_PARALLEL */
            TUNER_InitParams.SerialClockSource = STTUNER_SCLK_VCODIV6;
            TUNER_InitParams.SerialDataMode    = STTUNER_SDAT_VALID_RISING;

            break; /* TUNER_SAT .... */
        
        case TUNER_SAT_399_EVAL:
            STTBX_Print(("399 EVAL initparams configuration\n"));
            TUNER_InitParams.Device            = STTUNER_DEVICE_SATELLITE;
            TUNER_InitParams.DriverPartition   = system_partition;
            TUNER_InitParams.DemodType         = STTUNER_DEMOD_STV0399;
    
#if defined(LNB_21)
            TUNER_InitParams.LnbType           = STTUNER_LNB_LNB21;
#else
            TUNER_InitParams.LnbType           = STTUNER_LNB_STV0399;
#endif
       
            TUNER_InitParams.DemodIO.Address   = 0xD0;
            TUNER_InitParams.TunerType         = STTUNER_TUNER_NONE;
            TUNER_InitParams.DemodIO.Route     = STTUNER_IO_DIRECT;       /* I/O direct to hardware */
            TUNER_InitParams.DemodIO.Driver    = STTUNER_IODRV_I2C;

            TUNER_InitParams.TunerIO.Route     = STTUNER_IO_PASSTHRU;     /* ignored */
            TUNER_InitParams.TunerIO.Driver    = STTUNER_IODRV_I2C;       /* ignored */
            TUNER_InitParams.TunerIO.Address   = 0;                       /* ignored */

#if defined(LNB_21)
            TUNER_InitParams.LnbIO.Route       = STTUNER_IO_DIRECT; 
#else
            TUNER_InitParams.LnbIO.Route       = STTUNER_IO_PASSTHRU;
#endif
            TUNER_InitParams.LnbIO.Driver      = STTUNER_IODRV_I2C;
#if defined(mb376)
            TUNER_InitParams.LnbIO.Address     = 0x00;  
#else
            TUNER_InitParams.LnbIO.Address     = 0x10;       
#endif

            TUNER_InitParams.ExternalClock     = 144000000;
#if defined(mb390)
            TUNER_InitParams.SerialDataMode     = STTUNER_SDAT_VALID_RISING;   
            TUNER_InitParams.BlockSyncMode      = STTUNER_SYNC_FORCED;
            TUNER_InitParams.DataFIFOMode       = STTUNER_DATAFIFO_ENABLED;
            TUNER_InitParams.OutputFIFOConfig.OutputRateCompensationMode = STTUNER_COMPENSATE_DATAVALID;
            TUNER_InitParams.OutputFIFOConfig.CLOCKPERIOD = 2;  /* optimum value for 399 serial mode working */
#endif    
            TUNER_InitParams.TSOutputMode      = STTUNER_TS_MODE_SERIAL; /* or STTUNER_TS_MODE_PARALLEL */
            TUNER_InitParams.SerialClockSource = STTUNER_SCLK_DEFAULT;
      
            break; /* TUNER 399 */

        case TUNER_TER_360:

            /* Set Init Params - STV0360 Terrestrial tuner */
            TUNER_InitParams.Device            = STTUNER_DEVICE_TERR;
            TUNER_InitParams.DemodType         = STTUNER_DEMOD_STV0360;
            TUNER_InitParams.DemodIO.Route     = STTUNER_IO_DIRECT;        /* I/O direct to hardware */
            TUNER_InitParams.DemodIO.Driver    = STTUNER_IODRV_I2C;
            TUNER_InitParams.DemodIO.Address   = 0x38;                     /* stv0360 address */
            TUNER_InitParams.TunerType         = STTUNER_TUNER_DTT7572;
            TUNER_InitParams.TunerIO.Route     = STTUNER_IO_REPEATER;      /* use demod repeater functionality to pass tuner data*/
            TUNER_InitParams.TunerIO.Driver    = STTUNER_IODRV_I2C;
            TUNER_InitParams.TunerIO.Address   = 0xC0;                     /* address for Tuner (e.g. VG1011 = 0xC0) */
            TUNER_InitParams.ExternalClock     = 4000000;
            TUNER_InitParams.TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
            TUNER_InitParams.SerialClockSource = STTUNER_SCLK_DEFAULT;
            TUNER_InitParams.SerialDataMode    = STTUNER_SDAT_DEFAULT;
#if defined (STTUNER_USE_TER) 
            TUNER_InitParams.FECMode           = STTUNER_FEC_MODE_DVB;
            TUNER_InitParams.Guard             = STTUNER_GUARD_1_4;
            TUNER_InitParams.Hierarchy         = STTUNER_HIER_NONE;
            TUNER_InitParams.Mode              = STTUNER_MODE_8K;
#endif            
            TUNER_InitParams.LnbType           = STTUNER_LNB_NONE;            
            STTBX_Print(("TUNER_Setup(360_TER,"));            
            break; /* TUNER_TER ... */
          
        default:
            break;
        
    }

    strcpy( TUNER_InitParams.EVT_DeviceName,  EVENT_HANDLER_NAME );
    strcpy( TUNER_InitParams.EVT_RegistrantName, (char*)"EVT_TUNER");

    strcpy( TUNER_InitParams.DemodIO.DriverName, I2C_DeviceName[I2C_Bus] );
    strcpy( TUNER_InitParams.TunerIO.DriverName, I2C_DeviceName[I2C_Bus] );
    strcpy( TUNER_InitParams.LnbIO.DriverName,   I2C_DeviceName[I2C_Bus] );

    STTBX_Print(("%s,%s,%s)=", TUNER_DeviceName, I2C_DeviceName[I2C_Bus], EVENT_HANDLER_NAME ));
    Error = STTUNER_Init(TUNER_DeviceName, &TUNER_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }
        
    STTBX_Print(("%s\n", STTUNER_GetRevision() ));

    /* Set Open Params */
    TUNER_OpenParams.NotifyFunction = NULL;

    STTBX_Print(("TUNER_Open="));
    Error = STTUNER_Open(TUNER_DeviceName, &TUNER_OpenParams, &TUNER_Handle );
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }

    /* Set all bands in one list */
    TUNER_Bands[BAND_US].BandStart    = 12200000;
    TUNER_Bands[BAND_US].BandEnd      = 12700000;
    TUNER_Bands[BAND_US].LO           = 11250000;
    TUNER_Bands[BAND_US].DownLink     = STTUNER_DOWNLINK_Ku;
#if defined(C_BAND_TESTING)
    TUNER_Bands[BAND_EU_LO].BandStart = 3200000;
    TUNER_Bands[BAND_EU_LO].BandEnd   = 5150000;
    TUNER_Bands[BAND_EU_LO].LO        = 5150000;
    TUNER_Bands[BAND_EU_LO].DownLink  = STTUNER_DOWNLINK_C;
#else
    TUNER_Bands[BAND_EU_LO].BandStart = 10600000;
    TUNER_Bands[BAND_EU_LO].BandEnd   = 11700000;
    TUNER_Bands[BAND_EU_LO].LO        = 9750000;
    TUNER_Bands[BAND_EU_LO].DownLink  = STTUNER_DOWNLINK_Ku;
#endif

    TUNER_Bands[BAND_EU_HI].BandStart = 11700000;
    TUNER_Bands[BAND_EU_HI].BandEnd   = 12750000;
    TUNER_Bands[BAND_EU_HI].LO        = 10600000;
    TUNER_Bands[BAND_EU_HI].DownLink  = STTUNER_DOWNLINK_Ku;

    TUNER_Bands[BAND_TER].BandStart   = 474000;
    TUNER_Bands[BAND_TER].BandEnd     = 858000;
    TUNER_Bands[BAND_TER].LO          = 0;
    TUNER_Bands[BAND_TER].DownLink    = STTUNER_DOWNLINK_Ku; /* unused */
    
    TUNER_Bands[BAND_CABLE].BandStart = 40000000;
    TUNER_Bands[BAND_CABLE].BandEnd   = 900000000;
    TUNER_Bands[BAND_CABLE].LO        = 0;
    TUNER_Bands[BAND_TER].DownLink    = STTUNER_DOWNLINK_Ku; /* unused */

    TUNER_BandList.NumElements = (U32) MAX_BANDS;
    TUNER_BandList.BandList    = TUNER_Bands;
    Error = STTUNER_SetBandList(TUNER_Handle, &TUNER_BandList);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    /* Defaults set in TUNER_Scans[0]
     * Used as a reference for here and SetFrequency
     */

    TUNER_Scans[0].Polarization = STTUNER_PLR_ALL;        /* sat */
    TUNER_Scans[0].FECRates     = STTUNER_FEC_ALL;        /* sat & ter */
    TUNER_Scans[0].Modulation   = STTUNER_MOD_QPSK;
#if defined (STTUNER_USE_TER)     
    TUNER_Scans[0].Mode         = STTUNER_MODE_8K;        /* ter */
    TUNER_Scans[0].Guard        = STTUNER_GUARD_1_4;      /* ter */
    TUNER_Scans[0].Force        = STTUNER_FORCE_NONE;     /* ter */
    TUNER_Scans[0].Hierarchy    = STTUNER_HIER_NONE;      /* ter */
    TUNER_Scans[0].Spectrum     = STTUNER_INVERSION_AUTO; /* ter & cab */
    TUNER_Scans[0].FreqOff      = STTUNER_OFFSET;         /* ter */
    TUNER_Scans[0].ChannelBW    = STTUNER_CHAN_BW_8M;     /* ter */
    TUNER_Scans[0].EchoPos      = 0;                      /* ter */
    TUNER_Scans[0].IQMode       = STTUNER_IQ_MODE_AUTO;   /* sat */
#endif
    
#if defined(C_BAND_TESTING)
    TUNER_Scans[0].Band         = (U32) BAND_EU_LO ;
    TUNER_Scans[0].AGC          = STTUNER_AGC_ENABLE;
    TUNER_Scans[0].SymbolRate   = 27000000 ;
    TUNER_Scans[0].LNBToneState = STTUNER_LNB_TONE_22KHZ ;   /* sat */
#else
    TUNER_Scans[0].Band         = (U32) BAND_US;
    TUNER_Scans[0].AGC          = STTUNER_AGC_ENABLE;
    TUNER_Scans[0].SymbolRate   = 20000000;
    TUNER_Scans[0].LNBToneState = STTUNER_LNB_TONE_OFF;		
#endif
#if defined(STTUNER_USE_CAB)
    TUNER_Scans[0].J83          = STTUNER_J83_NONE;       /* cab */
    TUNER_Scans[0].SpectrumIs   = STTUNER_INVERSION_NONE; /* cab */
#endif
    TUNER_ScanList.NumElements  = 1;

    /* Set other scans to default */
    TUNER_Scans[1] = TUNER_Scans[0];

    /* Set specific values based on TunerType and Service */
    switch(TunerType)
    {
        case TUNER_SAT_299_STEM:
        case TUNER_SAT_299_EVAL:
        case TUNER_SAT_399_EVAL:
            if (SERVICE_Mode == SERVICE_MODE_DTV)
            {
                TUNER_Scans[0].Band         = (U32) BAND_US;
                TUNER_Scans[0].SymbolRate   = 20000000;
            }
            else /* if (SERVICE_Mode == SERVICE_MODE_DVB) */
            {
                TUNER_Scans[0].Band         = (U32) BAND_EU_LO;
                TUNER_Scans[0].SymbolRate   = 27000000;
                TUNER_Scans[0].LNBToneState = STTUNER_LNB_TONE_22KHZ;

                TUNER_Scans[1].Band         = (U32) BAND_EU_HI;
                TUNER_Scans[1].SymbolRate   = 22000000;
                TUNER_Scans[1].LNBToneState = STTUNER_LNB_TONE_22KHZ;

                TUNER_ScanList.NumElements  = 2;
            }

            break;

        case TUNER_TER_360: 
            TUNER_Scans[0].Band         = (U32) BAND_TER;
            TUNER_Scans[0].SymbolRate   = 20000000;
            TUNER_Scans[0].Modulation   = STTUNER_MOD_ALL;
            break;  

        default:
            Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
            break;
    }
    
    if (Error == ST_NO_ERROR)
    {
        TUNER_ScanList.ScanList = TUNER_Scans;
        Error = STTUNER_SetScanList(TUNER_Handle, &TUNER_ScanList);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    }
    
    return( Error );
}

/*-------------------------------------------------------------------------
 * Function : TUNER_Quit
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_Quit(void)
{
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    STTUNER_TermParams_t TUNER_TermParams;

    Error = STTUNER_Unlock(TUNER_Handle);
    if (Error != ST_NO_ERROR)
    {
    	return(Error);
    }

    Error = STTUNER_Close(TUNER_Handle);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }

    TUNER_TermParams.ForceTerminate = TRUE;

    Error = STTUNER_Term(TUNER_DeviceName, &TUNER_TermParams);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }

    semaphore_delete(TunerEventSemaphore_p);

    return( Error );
}


/*-------------------------------------------------------------------------
 * Function : TUNER_SetFrequency
 *            Function to set tuner frequency
 * Input    : Scan Parameters
 * Output   :
 * Return   : ErrorCode
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_SetFrequency(U32 Frequency, STTUNER_Scan_t *TUNER_Scan_p)
{
    ST_ErrorCode_t       Error = ST_NO_ERROR;
    U32  Element;
    
    /* Addition with STTUNER 3.5.0 */
    TUNER_Scan_p->IQMode = STTUNER_IQ_MODE_AUTO;
    for ( Element = 0; Element < TUNER_ScanList.NumElements; Element ++)
    {
        if (( Frequency >= TUNER_Bands[TUNER_Scans[Element].Band].BandStart ) &&
            (Frequency <= TUNER_Bands[TUNER_Scans[Element].Band].BandEnd ))
        {
            break;
        }
    }
    
    if ( Element == TUNER_ScanList.NumElements )
    {
        return ST_ERROR_BAD_PARAMETER;
    }

            
    STTBX_Print(("STTUNER_SetFrequency(%s,%d)=", TUNER_DeviceName, Frequency ));
    Error = STTUNER_SetFrequency(TUNER_Handle, Frequency, TUNER_Scan_p, 0);
    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }

    if (Error == ST_NO_ERROR ) 
    {
        Error = TUNER_WaitandDisplay();
    }

    return ( Error );
  
}

/* --------------------------------------------------------------------------- */
/* ---------------------------- support functions ---------------------------- */
/* --------------------------------------------------------------------------- */

/*-------------------------------------------------------------------------
 * Function : TUNER_WaitandDisplay
 * Input    :
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TUNER_WaitandDisplay( void )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_EventType_t TUNER_EventType;
    STTUNER_TunerInfo_t TUNER_TunerInfo;
    
    TUNER_WaitforNotifyCallback(&TUNER_EventType);
    if (TUNER_EventType == STTUNER_EV_LOCKED)
    {
        /* make sure we don't pick up any previous status information */
        memset(&TUNER_TunerInfo, '\0', sizeof( STTUNER_TunerInfo_t ) );

        Error = STTUNER_GetTunerInfo(TUNER_Handle, &TUNER_TunerInfo);
        TUNER_ReportInfo("TUNER_GetTunerInfo", &TUNER_TunerInfo);
    }
    else
    {
        STTBX_Print(("TUNER event: %s\n", EventToStr(TUNER_EventType) ));
        Error = (ST_ErrorCode_t) STTUNER_ERROR_UNSPECIFIED;
    }

    return Error;
}

/*-------------------------------------------------------------------------
 * Function : TUNER_WaitforNotifyCallback
 *            wait for tuner event to happen (with timeout)
 * Input    : None
 * Output   :
 * Return   : A tuner event or timeout.
 * ----------------------------------------------------------------------*/
void TUNER_WaitforNotifyCallback(STTUNER_EventType_t *EventType_p)
{
    clock_t time;
    int result; 

    time = time_plus(time_now(), 60 * ST_GetClocksPerSecond());

    result = semaphore_wait_timeout(TunerEventSemaphore_p, &time);
    if (result == -1) 
    {
        *EventType_p = STTUNER_EV_TIMEOUT;
    }
    else  
    {
        *EventType_p = LastEvent;
    }

}/* TUNER_WaitforNotifyCallback */

/*-------------------------------------------------------------------------
 * Function : TUNER_ReportInfo
 * Input    : PrefixStr, TUNER_TunerInfo params
 * Output   :
 * Return   : 
 * ----------------------------------------------------------------------*/
void TUNER_ReportInfo(char *PrefixStr, STTUNER_TunerInfo_t *TUNER_TunerInfo_p)
{
    STTBX_Print(("%s: FREQ=%u STATUS=%s PLR=%s FEC=%s MOD=%s LNBTONE=%s BAND=%d AGC=%s SYMRATE=%u\n",
                 PrefixStr,     TUNER_TunerInfo_p->Frequency,
                 StatusToStr  ( TUNER_TunerInfo_p->Status ),
                 PolarityToStr( TUNER_TunerInfo_p->ScanInfo.Polarization ),
                 FecToStr     ( TUNER_TunerInfo_p->ScanInfo.FECRates ),
                 ModToStr     ( TUNER_TunerInfo_p->ScanInfo.Modulation ),
                 LNBToneToStr ( TUNER_TunerInfo_p->ScanInfo.LNBToneState ),
                 TUNER_TunerInfo_p->ScanInfo.Band,
                 (TUNER_TunerInfo_p->ScanInfo.AGC ? "TRUE":"FALSE"),
                 TUNER_TunerInfo_p->ScanInfo.SymbolRate ));
}

/*-------------------------------------------------------------------------
 * Function : EventToStr
 * Input    : TUNER_EventType
 * Output   :
 * Return   : Event String
 * ----------------------------------------------------------------------*/
char *EventToStr(STTUNER_EventType_t TUNER_EventType)
{
    switch (TUNER_EventType)
    {
        case STTUNER_EV_NO_OPERATION:  
            return("NO_OPERATION");

        case STTUNER_EV_LOCKED:        
            return("LOCKED");

        case STTUNER_EV_UNLOCKED:      
            return("UNLOCKED");

        case STTUNER_EV_SCAN_FAILED:   
            return("SCAN_FAILED");

        case STTUNER_EV_TIMEOUT:       
            return("TIMEOUT");

        case STTUNER_EV_SIGNAL_CHANGE: 
            return("SIGNAL_CHANGE");

        default:
            break;
    }

    return("UNKNOWN EVENT");
}

/*-------------------------------------------------------------------------
 * Function : FecToStr
 * Input    : TUNER_FECRate
 * Output   :
 * Return   : Event String
 * ----------------------------------------------------------------------*/
char *FecToStr(STTUNER_FECRate_t TUNER_FECRate)
{
    switch(TUNER_FECRate)
    {
        case STTUNER_FEC_NONE:
            return("NONE");

        case STTUNER_FEC_ALL:
            return("ALL");

        case STTUNER_FEC_1_2:
            return("1_2");

        case STTUNER_FEC_2_3:
            return("2_3");

        case STTUNER_FEC_3_4:
            return("3_4");

        case STTUNER_FEC_4_5:
            return("4_5");

        case STTUNER_FEC_5_6:
            return("5_6");

        case STTUNER_FEC_6_7:
            return("6_7");

        case STTUNER_FEC_7_8:
            return("7_8");

        case STTUNER_FEC_8_9:
            return("8_9");

        default:
            break;
    }

    return("???");
}

/*-------------------------------------------------------------------------
 * Function : ModToStr
 * Input    : TUNER_Modulation
 * Output   :
 * Return   : Event String
 * ----------------------------------------------------------------------*/
char *ModToStr(STTUNER_Modulation_t TUNER_Modulation)
{
    switch(TUNER_Modulation)
    {
        case STTUNER_MOD_NONE:
            return("NONE");

        case STTUNER_MOD_ALL:
            return("ALL");

        case STTUNER_MOD_QPSK:
            return("QPSK");

        case STTUNER_MOD_8PSK:
            return("8PSK");

        case STTUNER_MOD_QAM:
            return("QAM");

        case STTUNER_MOD_16QAM:
            return("16QAM");

        case STTUNER_MOD_32QAM:
            return("32QAM");

        case STTUNER_MOD_64QAM:
            return("64QAM");

        case STTUNER_MOD_128QAM:
            return("128QAM");

        case STTUNER_MOD_256QAM:
            return("256QAM");

        case STTUNER_MOD_BPSK:
            return("BPSK");

        default:
            break;
    }

    return("???");

}

/*-------------------------------------------------------------------------
 * Function : StatusToStr
 * Input    : TUNER_Status
 * Output   :
 * Return   : Event String
 * ----------------------------------------------------------------------*/
char *StatusToStr(STTUNER_Status_t TUNER_Status)
{
    switch (TUNER_Status)
    {
        case STTUNER_STATUS_UNLOCKED:  
            return("UNLOCKED");

        case STTUNER_STATUS_SCANNING:  
            return("SCANNING");

        case STTUNER_STATUS_LOCKED:    
            return("LOCKED");

        case STTUNER_STATUS_NOT_FOUND: 
            return("NOT_FOUND");

        default:
            break;
    }

    return("???");
}

/*-------------------------------------------------------------------------
 * Function : PolarityToStr
 * Input    : TUNER_Polarization
 * Output   :
 * Return   : Event String
 * ----------------------------------------------------------------------*/
char *PolarityToStr(STTUNER_Polarization_t TUNER_Polarization)
{
    switch (TUNER_Polarization)
    {
        case STTUNER_PLR_ALL:  
            return("ALL");

        case STTUNER_PLR_HORIZONTAL:  
            return("HORIZONTAL");

        case STTUNER_PLR_VERTICAL:    
            return("VERTICAL");

        default:
            break;
    }

    return("???");
}

/*-------------------------------------------------------------------------
 * Function : LNBToneToStr
 * Input    : TUNER_LNBToneState
 * Output   :
 * Return   : Event String
 * ----------------------------------------------------------------------*/
char *LNBToneToStr(STTUNER_LNBToneState_t TUNER_LNBToneState)
{
    switch (TUNER_LNBToneState)
    {
        case STTUNER_LNB_TONE_DEFAULT:  
            return("DEFAULT");

        case STTUNER_LNB_TONE_OFF:  
            return("OFF");

        case STTUNER_LNB_TONE_22KHZ:    
            return("22KHZ");

        default:
            break;
    }

    return("???");
}


/* EOF --------------------------------------------------------------------- */
