/****************************************************************************

File Name   : clkrvfilter.c

Description : Clock Recovery API Routines

Copyright (C) 2007, STMicroelectronics

Revision History    :

    [...]

References  :

$ClearCase (VOB: stclkrv)S

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 5.0

****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include <string.h>
#include <stdlib.h>
#include "stlite.h"        /* MUST be included BEFORE stpti.h */
#include "stsys.h"
#include "sttbx.h"
#include "stdevice.h"
#endif

#include "stos.h"
#include "stevt.h"

#include "stclkrv.h"
#include "clkrvdat.h"
#include "clkrvreg.h"

/* Private Constants ------------------------------------------------------ */

/* Clock recovery Filter & drift correction of slaves clocks Debug Array */
#ifdef CLKRV_FILTERDEBUG
#define CLKRV_MAX_SAMPLES  5000
U32 ClkrvDbg[13][CLKRV_MAX_SAMPLES];
U32 Clkrv_Slave_Dbg[5][CLKRV_MAX_SAMPLES];
U32 SampleCount = 0;
U32 ClkrvCounters[6];
#endif


#if defined (WA_GNBvd44290) || defined(WA_GNBvd54182) || defined(WA_GNBvd56512)
static U32 Count_PCM = 0;
static U32 Count_PixelHD = 0;
void stclkrv_GNBvd44290_ResetClkrvTracking(void);
#endif /* WA_GNBvd44290 || WA_GNBvd54182 || WA_GNBvd56512*/

/* Nominal Frequency Table */

/* First Frequency is SD/STC frequency */

#if defined (ST_5100) || defined (ST_5105) ||defined (ST_5301) || defined (ST_5107)|| \
    defined (ST_7710)
    #define CLKRV_CRYSTAL_27MHZ 1
    #define CLKRV_CRYSTAL_30MHZ 0
#elif defined (ST_5188) || defined (ST_5525)
    #define CLKRV_CRYSTAL_27MHZ 0
    #define CLKRV_CRYSTAL_30MHZ 1
#endif

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    #if (STCLKRV_EXT_CLK_MHZ == 30)
        #define CLKRV_CRYSTAL_27MHZ 0
        #define CLKRV_CRYSTAL_30MHZ 1
    #elif (STCLKRV_EXT_CLK_MHZ == 27)
        #define CLKRV_CRYSTAL_27MHZ 1
        #define CLKRV_CRYSTAL_30MHZ 0
    #elif (STCLKRV_EXT_CLK_MHZ == 0x00)
        #define CLKRV_CRYSTAL_27MHZ 0
        #define CLKRV_CRYSTAL_30MHZ 1
    #endif
#endif


#if CLKRV_CRYSTAL_27MHZ
stclkrv_Freqtable_t NominalFreqTable[MAX_NO_IN_FREQ_TABLE] =
{
    /* Freq    sdiv  md  ipe    StepX100  Ratio/1000 */

    /* SD */
#ifdef ST_7200
    { 27000000,   2, -17,   0,  5100, 1000},
#else
    { 27000000,  16, -17,   0,  5100, 1000},
#endif
    /* PCM */
    { 1024000, 256, -6,  20736,  118,   38},
    { 1411200, 256, -13, 28421,  225,   52},
    { 1536000, 256, -15, 13824,  267,   57},
    { 2048000, 128, -6,  20736,  236,   76},
    { 2822400, 128, -13, 28421,  450,  104},
    { 3072000, 128, -15, 13824,  533,  113},

    { 4096000,  64,  -6,  20736,  474, 151},
    { 4233600,  64,  -7,  16050,  506, 156},
    { 4608000,  64,  -9,  18432,  600, 170},
#ifdef WA_GNBvd56512
    { 5644800,  64, -13,  28420,  900, 209},
#else
    { 5644800,  64, -13,  28421,  900, 209},
#endif
    { 6144000,  64, -15,  13824, 1066, 227},

    { 8192000,  32,  -6,  20736,  948, 303},
    { 8200000,  32,  -6,  21579,  950, 303},
    { 8467200,  32,  -7,  16050, 1012, 313},
    { 9216000,  32,  -9,  18432, 1200, 341},
    { 11289600, 32, -13,  28421, 1800, 418},
    { 11300000, 32, -13,  28998, 1804, 418},
    { 12288000, 32, -15,  13824, 2133, 455},
    { 12300000, 32, -15,  14386, 2137, 455},

    {16384000,  16, -6,  20736,  1896, 606},
    {16934400,  16, -7,  16050,  2025, 627},
    {18432000,  16, -9,  18432,  2400, 682},
    {22579200,  16, -13, 28421,  3601, 836},
    {24576000,  16, -15, 13824,  4267, 910},
    {24600000,  16, -15, 14386,  4275, 911},

    /* SPDIF */

    {32768000,  8,  -6,  20736,  3792, 1213},
    {33868800,  8,  -7,  16050,  4051, 1254},
    {36864000,  8,  -9,  18432,  4800, 1365},
    {45158400,  8,  -13, 28421,  7203, 1672},
    {49140000,  8,  -15, 13683,  8529, 1820},
    {49152000,  8,  -15, 13824,  8534, 1820},

    /* For 640*480P video format */
#ifdef WA_GNBvd56512
    {50350000,  8,  -15, 27530,  8954, 1865},  /* Frame frequency 59.94Hz*/
#else
    {50350000,  8,  -15, 27529,  8954, 1865},  /* Frame frequency 59.94Hz*/
#endif
    {50400000,  8,  -15, 28087,  8973, 1867},  /* Frame frequency 60Hz*/

    /* 54Mhz configuration for 480P/576P/576I on HDMI */
    {54000000,  8, -17,      0, 10300, 2000},
    {54054000,  4,  -1,   1048,  5160, 2002},
    {59000000,  4,  -3,  23327,  6147, 2185},

    {65536000,  4,  -6,  20736,  7585, 2427},
    {67737600,  4,  -7,  16050,  8103, 2508},
    {73728000,  4,  -9,  18432,  9601, 2730},
    {74250000,  4,  -9,  23831,  9737, 2750},
    {90316800,  4, -13,  28421, 14406, 3345},

    /* HD */
    { 96000000, 4, -15,      0, 16276, 3555 },
    { 98304000, 4, -15,  13824, 17067, 3641 },

    {100700000, 4, -15,  27529, 17910, 3730 },
    {100800000, 4, -15,  28087, 17945, 3734 },
    {108000000, 4, -17,      0, 20599, 4000 },
    {108108000, 4, -17,    524, 20641, 4004 },  /* (108000000 * 1001 / 1000) */

    {118800000, 2,  -3,  29789, 12462, 4400 },
    {130000000, 2,  -6,  13611, 14933, 4815 },  /*XGA 1024*768@60Mhz*/
    {144000000, 2,  -9,      0, 18310, 5333 },

#ifdef WA_GNBvd56512
    {148351648, 2,  -9,  23071, 19435, 5494 },  /* (148500000 * 1000 / 1001) */
#else
    {148351648, 2,  -9,  23069, 19435, 5494 },  /* (148500000 * 1000 / 1001) */
#endif
    {148500000, 2,  -9,  23831, 19474, 5500 },
    {157500000, 2, -11,   1872, 21905, 5833 },  /*XGA 1024*768@75Mhz*/
    {189000000, 2, -14,  23406, 31569, 7000 }   /*XGA 1024*768@85Mhz*/
};

#elif CLKRV_CRYSTAL_30MHZ

stclkrv_Freqtable_t NominalFreqTable[MAX_NO_IN_FREQ_TABLE] =
{
    /* Freq    sdiv  md  ipe    StepX100  Ratio/1000 */

    /* SD */
#ifdef ST_7200
    { 27000000,   2, -15, 7282, 4600, 1000},
#else
    { 27000000,  16, -15, 7282, 4600, 1000},
#endif
    /* PCM */
    { 1024000, 256, -3,  23040,  106,   38},
    { 1411200, 256, -11, 24298,  202,   52},
    { 1536000, 256, -13, 15360,  240,   57},
    { 2048000, 128, -3,  23040,  213,   76},
    { 2822400, 128, -11, 24298,  405,  104},
    { 3072000, 128, -13, 15360,  480,  113},

    { 4096000,  64,  -3,  23040,  426, 151},
    { 4233600,  64,  -4,  21474,  455, 156},
    { 4608000,  64,  -6,  31403,  540, 170},
    { 5644800,  64, -11,  24297,  810, 209},
    { 6144000,  64, -13,  15360,  960, 227},

    { 8192000,  32,  -3,  23040,  853,  303},
    { 8200000,  32,  -3,  23977,  855,  303},
    { 8467200,  32,  -4,  21474,  911,  313},
    { 9216000,  32,  -6,  31403, 1080,  341},
    { 11289600, 32, -11,  24297, 1620,  418},
    { 11300000, 32, -11,  24939, 1623,  418},
    { 12288000, 32, -13,  15360, 1920,  455},
    { 12300000, 32, -13,  15984, 1924,  455},

    {16384000,  16, -3,  23040, 1706,  606},
    {16934400,  16, -4,  21474, 1823,  627},
    {18432000,  16, -6,  31403, 2160,  682},
    {22579200,  16, -11, 24297, 3241,  836},
    {24576000,  16, -13, 15360, 3840,  910},
    {24600000,  16, -13, 15984, 3847,  911},

    /* SPDIF */

    {32768000,  8,  -3,  23040,  3413, 1213},
    {33868800,  8,  -4,  21474,  3646, 1254},
    {36864000,  8,  -6,  31403,  4320, 1365},
    {45158400,  8,  -11, 24297,  6482, 1672},
    {49140000,  8,  -13, 15204,  7676, 1820},
    {49152000,  8,  -13, 15360,  7680, 1820},

    /* For 640*480P video format */
    {50350000, 8,  -13,  30588,  8059, 1865},   /* Frame frequency 59.94Hz*/
    {50400000, 8,  -13,  31208,  8076, 1867},   /* Frame frequency 60Hz*/

    /* STFAE - Add 54Mhz configuration for 480P/576P/576I on HDMI */
    {54000000,  8, -15,   7282,  9270, 2000},
    {54054000,  8, -15,   7864,  9288, 2002},  /* (54000000 * 1001 / 1000) */
    {59000000,  8, -16,  23882, 11066, 2185},
    {65536000,  4,  -3,  23040,  6827, 2427},
    {67737600,  4,  -4,  21474,  7293, 2508},
    {73728000,  4,  -6,  31403,  8640, 2730},
    {74250000,  4,  -7,   4634,  8762, 2750},
    {90316800,  4, -11,  24297, 12966, 3345},

    /* HD */
    {96000000,  4, -13,      0, 14648, 3555},
    {98304000,  4, -13,  15360, 15360, 3641},

    {100700000, 4, -13,  30588, 16118, 3730},
    {100800000, 4, -13,  31208, 16151, 3734},
    {108000000, 4, -15,   7282, 18534, 4000},
    {108108000, 4, -15,   7864, 18550, 4004},  /* (108000000 * 1001 / 1000) */

    {118800000, 4, -16,  27472, 20153, 4400},
    {130000000, 2,  -3,  15124, 13433, 4815},  /*XGA 1024*768@60Mhz*/
    {144000000, 2,  -6,  10923, 16479, 5333},

    {148351648, 2,  -7,   3787, 17490, 5494},  /* (148500000 * 1000 / 1001) */
    {148500000, 2,  -7,   4634, 17525, 5500},
    {157500000, 2,  -8,  20285, 19715, 5833},  /*XGA 1024*768@75Mhz*/
    {189000000, 2, -12,  22365, 28395, 7000}   /*XGA 1024*768@85Mhz*/
};

#else
#error "error in crystal freq. selection"
#endif
/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

/* Program FS registers*/
ST_ErrorCode_t clkrv_Program_FS_Registers( STCLKRV_ControlBlock_t *CB_p,
                                           STCLKRV_ClockSource_t Clock);
/* Change Clocks by Control value */
void clkrv_ChangeClockFreqBy( STCLKRV_ControlBlock_t *CB_p,
                              STCLKRV_ClockSource_t Clock,
                              S32 Control );

/* Calculate new values of Sdiv, Mde and Ipe to apply the correction value */
void clkrv_CorrectSlaveClocks( STCLKRV_ControlBlock_t *CB_p,
                               S32 CntrlVal,
                               STCLKRV_ClockSource_t Clock );
/* Correctio for crystal error typically on 7100/7109 */
#if (STCLKRV_CRYSTAL_ERROR != 0x00)
void clkrv_CorrectCrystalError(void);
#endif
/* Reset 5100 H/W Block */
ST_ErrorCode_t clkrv_InitHW( STCLKRV_ControlBlock_t *CB_p );

/* Register Read Write Function */
__inline void clkrv_writeReg(STSYS_DU32 *Base, U32 Reg, U32 Value);
__inline void clkrv_writeRegUnLock(STSYS_DU32 *Base, U32 Reg, U32 Value);
__inline void clkrv_readReg (STSYS_DU32 *Base, U32 Reg, U32 *Value);

#ifndef STCLKRV_NO_PTI
/* Filter Functions */
U32 ClockRecoveryFilter(U32 ExtdPCRTime, U32 ExtdSTCTime, STCLKRV_ControlBlock_t *Instance_p);
/* Filter related internal functions */
static ST_ErrorCode_t CalculateFrequencyError(FilterData *FilterData_p,
                                              STCLKRV_ControlBlock_t *CB_p);
static ST_ErrorCode_t LowPassFilter(FilterData *FilterData_p,
                                    STCLKRV_ControlBlock_t *CB_p);
static void ApplyWeighingFilter(STCLKRV_ControlBlock_t *CB_p);
#ifdef ST_5188
void STCLKRV_ActionPCR(STEVT_CallReason_t EventReason,
                       ST_DeviceName_t RegistrantName,
                       STEVT_EventConstant_t EventOccured,
                       STDEMUX_EventData_t *PcrData_p,
                       STCLKRV_Handle_t *InstanceHandle_p);
#else
void STCLKRV_ActionPCR(STEVT_CallReason_t EventReason,
                       ST_DeviceName_t RegistrantName,
                       STEVT_EventConstant_t EventOccured,
                       STPTI_EventData_t *PcrData_p,
                       STCLKRV_Handle_t *InstanceHandle_p);
#endif

void ActionPCR( STCLKRV_ControlBlock_t *InstanceHandle_p);

void ClockRecoveryFilterTask(STCLKRV_ControlBlock_t *CB_p);

/* Utility Functions */
static S32 DivideAndRound(S32 Num, U32 Denom);
U32 FindClockIndex(STCLKRV_ControlBlock_t *CB_p, STCLKRV_ClockSource_t Clock);

#endif /* STCLKRV_NO_PTI */

/* Utility Functions */
void UpdateStateMachine(STCLKRV_ControlBlock_t *Instance_p,BOOL DiscontinuousPCR);



/* Functions -------------------------------------------------------------- */
#if defined (WA_GNBvd44290) || defined(WA_GNBvd54182)|| defined(WA_GNBvd56512)
void stclkrv_GNBvd44290_ResetClkrvTracking(void)
{
    Count_PCM = 0;
    Count_PixelHD = 0;
}
#endif /* stclkrv_GNBvd44290_ResetClkrvTracking */

/****************************************************************************
Name         : FindClockIndex

Description  : Find the given clock in clock array

Parameters   : Pointer to STCLKRV_ControlBlock_t, Clock

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_BAD_PARAMETER          Bad parameter
See Also     :
****************************************************************************/

U32 FindClockIndex(STCLKRV_ControlBlock_t *CB_p, STCLKRV_ClockSource_t Clock)
{
    U32 i=0;

    /* Start from 0 as pixel at Zero */
    for (i=0;i<CB_p->HWContext.ClockIndex;i++){
        if (Clock == CB_p->HWContext.Clocks[i].ClockType)
            break;
    }

    return (i);
}

/****************************************************************************
Name         : GetRegIndex

Description  : Find the given clock recovery instance (Primary/Secondary)

Parameters   : Pointer to STCLKRV_ControlBlock_t, Clock

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_BAD_PARAMETER          Bad parameter
See Also     :
****************************************************************************/

U32 GetRegIndex(STCLKRV_ControlBlock_t *CB_p)
{

#if !defined (ST_5525) && !defined (ST_7200)
    return 0;
#elif defined (ST_5525) || defined (ST_7200)
    if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
        return 0;
    else if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_SECONDARY)
        return 1;
    else
        return 0;
#endif
}

/****************************************************************************
Name         : GetCounterOffset

Description  : Find the given clock counter register offset

Parameters   : Pointer to STCLKRV_ControlBlock_t, Clock

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_BAD_PARAMETER          Bad parameter
See Also     :
****************************************************************************/

static U32 GetCounterOffset(STCLKRV_ControlBlock_t *CB_p, STCLKRV_ClockSource_t Clock)
{
    U32 CaptureCntOffset = 0;

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)\
 || defined (ST_5188)

        switch(Clock)
        {
            case STCLKRV_CLOCK_PCM_0:
                CaptureCntOffset = CAPTURE_COUNTER_PCM;
                break;
            case STCLKRV_CLOCK_SPDIF_0:
                CaptureCntOffset = CAPTURE_COUNTER_SPDIF;
                break;
            default:
                /* what else */
                CaptureCntOffset = CAPTURE_COUNTER_PCM;
                break;
        }
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)

        switch(Clock)
        {
            case STCLKRV_CLOCK_PCM_0:
                CaptureCntOffset = CAPTURE_COUNTER_PCM;
                break;
            case STCLKRV_CLOCK_HD_0:
                CaptureCntOffset = CAPTURE_COUNTER_HD;
                break;
            default:
                /* what else */
                CaptureCntOffset = CAPTURE_COUNTER_PCM;
                break;
        }
#elif defined (ST_5525)

        switch(Clock)
        {
            case STCLKRV_CLOCK_PCM_0:
                if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
                    CaptureCntOffset = CAPTURE_COUNTER_PCM;
                else
                    CaptureCntOffset = CAPTURE_COUNTER_1_PCM;
                break;
            case STCLKRV_CLOCK_PCM_1:
                if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
                    CaptureCntOffset = CAPTURE_COUNTER_PCM_1;
                else
                    CaptureCntOffset = CAPTURE_COUNTER_1_PCM_1;
                break;
            case STCLKRV_CLOCK_PCM_2:
                if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
                    CaptureCntOffset = CAPTURE_COUNTER_PCM_2;
                else
                    CaptureCntOffset = CAPTURE_COUNTER_1_PCM_2;
                break;
            case STCLKRV_CLOCK_PCM_3:
                if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
                    CaptureCntOffset = CAPTURE_COUNTER_PCM_3;
                else
                    CaptureCntOffset = CAPTURE_COUNTER_1_PCM_3;
                break;
            case STCLKRV_CLOCK_SPDIF_0:
                if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
                    CaptureCntOffset = CAPTURE_COUNTER_SPDIF;
                else
                    CaptureCntOffset = CAPTURE_COUNTER_1_SPDIF;
                break;
            default:
                /* what else */
                CaptureCntOffset = CAPTURE_COUNTER_PCM;
                break;
        }
#elif defined (ST_7200)

        switch(Clock)
        {
            case STCLKRV_CLOCK_PCM_0:
            case STCLKRV_CLOCK_PCM_1:
            case STCLKRV_CLOCK_PCM_2:
            case STCLKRV_CLOCK_PCM_3:
                if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
                    CaptureCntOffset = CAPTURE_COUNTER_PCM;
                else
                    CaptureCntOffset = CAPTURE_COUNTER_PCM_1;
                break;
            case STCLKRV_CLOCK_HD_0:
                CaptureCntOffset = CAPTURE_COUNTER_HD;
                break;
            case STCLKRV_CLOCK_HD_1:
                CaptureCntOffset = CAPTURE_COUNTER_HD_1;
                break;
            default:
                /* what else */
                CaptureCntOffset = CAPTURE_COUNTER_PCM;
                break;
        }
#endif

return CaptureCntOffset;

}

/****************************************************************************
Name         : SlaveHaveCounter

Description  : To find whether slave has a HW counter associated.

Parameters   : Pointer to STCLKRV_ControlBlock_t, Clock

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_BAD_PARAMETER          Bad parameter
See Also     :
****************************************************************************/
BOOL SlaveHaveCounter(STCLKRV_ClockSource_t ClockSource )
{

#if defined(ST_7100) || defined (ST_7109) || defined (ST_7710)
    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_HD_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#elif defined (ST_5525)

    /* all possible salves */
    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_PCM_1) ||
        (ClockSource == STCLKRV_CLOCK_PCM_2) ||
        (ClockSource == STCLKRV_CLOCK_PCM_3) ||
        (ClockSource == STCLKRV_CLOCK_SPDIF_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#elif defined (ST_7200)

    /* all possible salves */
    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_PCM_1) ||
        (ClockSource == STCLKRV_CLOCK_PCM_2) ||
        (ClockSource == STCLKRV_CLOCK_PCM_3) ||
        (ClockSource == STCLKRV_CLOCK_HD_0)  ||
        (ClockSource == STCLKRV_CLOCK_HD_1) )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#else

    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_SPDIF_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#endif

}

/****************************************************************************
Name         : clkrv_InitHW

Description  : Resets the Clock Recovery Hardware Block during Initialization
               FS0 to 27MHz and invalidate FS1 ans FS2 in SW

Parameters   : Pointer to STCLKRV_ControlBlock_t

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_BAD_PARAMETER          Bad parameter


See Also     : STCLKRV_Init
****************************************************************************/
ST_ErrorCode_t clkrv_InitHW( STCLKRV_ControlBlock_t *CB_p )
{
    U16 i = 0;
    STSYS_DU32 *BaseAddress = NULL;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Index =0;
    STCLKRV_ClockSource_t MasterClock;

#ifdef ST_OSLINUX
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
#endif

#if (STCLKRV_CRYSTAL_ERROR != 0x00)
        /* Update nminal frequency table values -
           Incase such as crystal is too far off */
        clkrv_CorrectCrystalError();
#endif


    /* First clock to be set (Pixel normally) */
    CB_p->HWContext.ClockIndex = 0;

    /* Invalidate all clocks */

    for ( i = 0; i < STCLKRV_NO_OF_CLOCKS; i++ )
    {
        CB_p->HWContext.Clocks[i].Valid = FALSE;
    }

    i = 0;  /* Pixel clock Always at Zero */
    Index = CB_p->HWContext.ClockIndex;


    /* In 5525, two decodes possible Pixel clock 0/1*/

    if ( (CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_5525) ||
         (CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200) ){
        if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
            MasterClock = STCLKRV_CLOCK_SD_0;
        else
            MasterClock = STCLKRV_CLOCK_SD_1;
    }
    else {
        MasterClock = STCLKRV_CLOCK_SD_0;
    }

    CB_p->HWContext.Clocks[Index].ClockType = MasterClock;
    CB_p->HWContext.Clocks[Index].Frequency = NominalFreqTable[i].Frequency;
    CB_p->HWContext.Clocks[Index].SDIV      = NominalFreqTable[i].SDIV;
    CB_p->HWContext.Clocks[Index].MD        = NominalFreqTable[i].MD;
    CB_p->HWContext.Clocks[Index].IPE       = NominalFreqTable[i].IPE;
    CB_p->HWContext.Clocks[Index].Step      = NominalFreqTable[i].Step;
    CB_p->HWContext.Clocks[Index].Valid     = TRUE;
    CB_p->HWContext.Clocks[Index].SlaveWithCounter = FALSE;

    /* Following parameter is not Valid for SD the master clock */
    CB_p->HWContext.Clocks[Index].PrevCounter   = 0;
    CB_p->HWContext.Clocks[Index].DriftError    = 0;

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
    CB_p->HWContext.ApplicationMode = STCLKRV_APPLICATION_MODE_NORMAL;
#endif

    /* Setup FS setup */

    /*
    *   Input freq divided by 1
    *   refernece is 27 Mhz
    *   Analog part switched off
    */


#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)

    /* Initialize  the FS_A to its Default Value */
    /* It should be removed */
    clkrv_writeRegUnLock( BaseAddress,
                             FSA_SETUP,
                             DEFAULT_VAL_FS_A_SETUP);
#elif defined (ST_5188) || defined (ST_5525)

    /* TB coded */

#endif

#if defined(ST_7100) || defined(ST_7109)
    /* Unlock the registers */
    clkrv_writeReg(BaseAddress, 0x00010, 0xC0DE);

    /* Set the reference clock source */
    clkrv_writeReg(BaseAddress, CKGB_REF_CLK_SRC, 0);

#if (STCLKRV_EXT_CLK_MHZ == 27) || (STCLKRV_EXT_CLK_MHZ == 30)
    clkrv_writeReg(BaseAddress, CKGB_REF_CLK_SRC, 1);
#endif

#endif

#if defined(ST_7200)
    /* Set the reference clock source */
    {
        U32 Regvalue=0;
        clkrv_readReg(BaseAddress, CKGB_REF_CLK_SRC, &Regvalue);
        /* By default clk_sysbclkin as reference source */
        clkrv_writeReg(BaseAddress, CKGB_REF_CLK_SRC, Regvalue&0xFFFFFFF1);

#if (STCLKRV_EXT_CLK_MHZ == 27) || (STCLKRV_EXT_CLK_MHZ == 30)
        /* Set sysclkinalt as reference source */
        clkrv_writeReg(BaseAddress, CKGB_REF_CLK_SRC, Regvalue|0x0E);
#endif
        clkrv_writeReg(BaseAddress, CKGB_OUT_MUX_CFG, 0x00);
    }
#endif

    /* Set Pixel - 27 Mhz Nominal */
    ErrorCode = clkrv_setClockFreq( CB_p, MasterClock, Index);

    if (ErrorCode==ST_NO_ERROR)
        CB_p->HWContext.ClockIndex++;

#if defined (ST_7710)
    {
        U32 RegValue;
        STSYS_DU32 *BaseAddress_AUDFS = (STSYS_DU32*)(CB_p->InitPars.ADSCBaseAddress_p);
        clkrv_readReg(BaseAddress_AUDFS, ADSC_IOCONTROL_PROG, &RegValue);
        RegValue |= 0x00000002;
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_IOCONTROL_PROG, RegValue);
    }
#elif defined (ST_7100)  || defined (ST_7109)
    {
        U32 RegValue;
        STSYS_DU32 *BaseAddress_AUDFS = (STSYS_DU32*)(CB_p->InitPars.AUDCFGBaseAddress_p);
        clkrv_readReg(BaseAddress_AUDFS, AUD_IOCONTROL_PROG, &RegValue);
        RegValue |= 0x00000007;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_IOCONTROL_PROG, RegValue);

        /* Select reference clock source for Audio clocks */
        clkrv_readReg(BaseAddress_AUDFS, AUD_FSYN_CFG, &RegValue);

        /* STFAE - Reset synthetizer to avoid bad audio on internal dac */
        RegValue=RegValue|0x00000001;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYN_CFG, RegValue);
        RegValue=RegValue&0xFFFFFFFE;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYN_CFG, RegValue);
#if ( (STCLKRV_EXT_CLK_MHZ == 27) || (STCLKRV_EXT_CLK_MHZ == 30) )
        RegValue |= 0x800000;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYN_CFG, RegValue);
#endif
    }
#elif defined (ST_7200)
    {
        U32 RegValue, RegValue1;
        STSYS_DU32 *BaseAddress_AUDFS = (STSYS_DU32*)(CB_p->InitPars.AUDCFGBaseAddress_p);
        clkrv_readReg(BaseAddress_AUDFS, AUD_IOCONTROL_PROG, &RegValue);
        RegValue |= 0x0000001F;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_IOCONTROL_PROG, RegValue);

        clkrv_readReg(BaseAddress_AUDFS, AUD_FSYNA_CFG, &RegValue);
        clkrv_readReg(BaseAddress_AUDFS, AUD_FSYNB_CFG, &RegValue1);

        /* Reset synthetizer to avoid bad audio on internal dac */
        RegValue = RegValue|0x00007C01;
        RegValue1 = RegValue1|0x00007C01;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNA_CFG, RegValue);
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNB_CFG, RegValue1);
        /* Complete reset and set internal 30MHz clock as reference source */
        RegValue=RegValue&0xFEFFFFFE;
        RegValue1=RegValue1&0xFEFFFFFE;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNA_CFG, RegValue);
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNB_CFG, RegValue1);
        clkrv_writeReg(BaseAddress_AUDFS, 0x208, 0x11);
#if ( (STCLKRV_EXT_CLK_MHZ == 27) || (STCLKRV_EXT_CLK_MHZ == 30) )
        /* Select SYSBCLKINALT as reference clock source */
        RegValue |= 0x1000000;
        RegValue1 |= 0x1000000;
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNA_CFG, RegValue);
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNB_CFG, RegValue1);
#endif
    }
#endif

    /* Handle this error in Init if something went wrong */
    return ErrorCode;
}

/****************************************************************************
Name         : STCLKRV_ActionPCR()

Description  : Call back function called by STPTI to notify the PCR and STC
               values. It signals the waiting clock recovery task and store
               current (PCR,STC) data in control block for later references.

               ActionPCR will NOT process the PCR that caused it to be
               invoked if the PCRHandling is disabled or,
               if the SlotID passed is not valid.

Parameters   : STEVT_CallReason_t        EventReason : Cause
               ST_DeviceName_t           RegistrantName: Null
               STEVT_EventConstant_t     EventOccured : Type
               STPTI_EventData_t         *PcrData_p : Event data
               STCLKRV_Handle_t          *InstanceHandle_p : Valid handle

Return Value : None

See Also     :
 ****************************************************************************/
/* Use device event callback to get slotID from event handler */
#ifndef STCLKRV_NO_PTI
void STCLKRV_ActionPCR(STEVT_CallReason_t        EventReason,
                              ST_DeviceName_t           RegistrantName,
                              STEVT_EventConstant_t     EventOccured,
#ifdef ST_5188
                              STDEMUX_EventData_t       *PcrData_p,
#else
                              STPTI_EventData_t         *PcrData_p,
#endif
                              STCLKRV_Handle_t          *InstanceHandle_p)
{
    STCLKRV_ControlBlock_t *TmpCB_p;

    TmpCB_p = (STCLKRV_ControlBlock_t *)InstanceHandle_p;

    /* Check handling control status */
    if (TmpCB_p->ActvRefPcr.PCRHandlingActive != TRUE)
    {
        return;
    }

    /* Check Slot validity */
    if ( (TmpCB_p->SlotEnabled != TRUE) ||
#ifdef ST_5188
         (PcrData_p->ObjectHandle != TmpCB_p->Slot) )   /* GNBVd06147 */
#else
         (PcrData_p->SlotHandle != TmpCB_p->Slot) )   /* GNBVd06147 */
#endif
    {
        STTBX_Report( (STTBX_REPORT_LEVEL_ERROR, "Invalid handle or slot not enabled"));
        return;
    }

    /* Take semap. STPTI callbakck from event queue, so semap ok */
    STOS_SemaphoreWait(TmpCB_p->InstanceSemaphore_p);

    /* Store data for future reference */
    TmpCB_p->InstancePcrData = *PcrData_p;

    STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);

    /* Signal waiting Filter task */
    STOS_SemaphoreSignal(TmpCB_p->FilterSemaphore_p);

    /* Simply return */
    return;
}

/****************************************************************************
Name         : ClockRecoveryFilterTask()

Description  : Filter task implemneted to remove PTI callback blocking

Parameters   : pointer to control block

Return Value : None

See Also     : ActionPCR()
 ****************************************************************************/
void ClockRecoveryFilterTask(STCLKRV_ControlBlock_t *CB_p)
{
    STCLKRV_Print(("ClockRecoveryFilterTask Created..\n"));

    STOS_TaskEnter (NULL);

    while(1)
    {
        STOS_SemaphoreWait(CB_p->FilterSemaphore_p);

        if (CB_p->FilterTaskActive != TRUE)
        break;

        ActionPCR(CB_p);

    }

    STOS_TaskExit (NULL);

    STCLKRV_Print(("ClockRecoveryFilterTask Quit\n"));
}

/****************************************************************************
Name         : ActionPCR()

Description  : Performs the Clock Recovery function from the clock values
               passed with the call, and previous history. Note that since
               the generation of the TP3 implementation on which this code
               is based, extended PTI functions have been added to return
               bit32 for both PCR and STC clock values. The extra bit will
               only become significant when there is a large difference
               (over 13 hours for b32 on 90kHz ticks) between the two
               clock values; this is unlikely to affect real-time Transport
               Streams.  The extended PCR function returns an extra boolean
               value, flagging PCR discontinuities. Its use should give
               faster recovery from (PTI expected) PCR dis-continuities, by
               effecting immediate re-basing.  The implementation would
               still have to sensibly handle unexpected PCR glitches and
               discontinuities anyway.

Parameters   : STCLKRV_ControlBlock_t InstanceHandle_p

Return Value : None

See Also     :
 ****************************************************************************/

void ActionPCR( STCLKRV_ControlBlock_t *InstanceHandle_p )
{
    /* For local storage of pcr event data passed on callback */
    BOOL DiscontinuousPCR;
    U32 PCRBaseBit32;
    U32 PCRBaseValue;
    U32 PCRExtension;
    U32 STCBaseBit32;
    U32 STCBaseValue;
    U32 STCExtension;


    STCLKRV_ControlBlock_t *TmpCB_p;
    U32     PCRTimeTotal;
    U32     STCTimeTotal;
    U32     RetVal = 0;


#ifdef CLKRV_FILTERDEBUG
    /* Debug use only */
    FilterData FData;
#endif

    TmpCB_p = (STCLKRV_ControlBlock_t *)InstanceHandle_p;

    /* Take semap. STPTI callback from event queue, so semap ok */
    STOS_SemaphoreWait(TmpCB_p->InstanceSemaphore_p);

    /* Load passed data to local vars */
#ifdef ST_5188
    DiscontinuousPCR =  TmpCB_p->InstancePcrData.member.PCREventData.DiscontinuityFlag;
    PCRBaseBit32 =      (U32) TmpCB_p->InstancePcrData.member.PCREventData.PCRBase.Bit32;
    PCRBaseValue =      (U32) TmpCB_p->InstancePcrData.member.PCREventData.PCRBase.LSW;
    PCRExtension =      (U32) TmpCB_p->InstancePcrData.member.PCREventData.PCRExtension;
    STCBaseBit32 =      (U32) TmpCB_p->InstancePcrData.member.PCREventData.PCRArrivalTime.Bit32;
    STCBaseValue =      (U32) TmpCB_p->InstancePcrData.member.PCREventData.PCRArrivalTime.LSW;
    STCExtension =      (U32) TmpCB_p->InstancePcrData.member.PCREventData.PCRArrivalTimeExtension;
#else
    DiscontinuousPCR =  TmpCB_p->InstancePcrData.u.PCREventData.DiscontinuityFlag;
    PCRBaseBit32 =      (U32) TmpCB_p->InstancePcrData.u.PCREventData.PCRBase.Bit32;
    PCRBaseValue =      (U32) TmpCB_p->InstancePcrData.u.PCREventData.PCRBase.LSW;
    PCRExtension =      (U32) TmpCB_p->InstancePcrData.u.PCREventData.PCRExtension;
    STCBaseBit32 =      (U32) TmpCB_p->InstancePcrData.u.PCREventData.PCRArrivalTime.Bit32;
    STCBaseValue =      (U32) TmpCB_p->InstancePcrData.u.PCREventData.PCRArrivalTime.LSW;
    STCExtension =      (U32) TmpCB_p->InstancePcrData.u.PCREventData.PCRArrivalTimeExtension;
#endif

    PCRTimeTotal = ( PCRBaseValue * CONV90K_TO_27M ) + PCRExtension;
    STCTimeTotal = ( STCBaseValue * CONV90K_TO_27M ) + STCExtension;

    /* determine whether we have a PCR glitch */
    if (DiscontinuousPCR || TmpCB_p->ActvRefPcr.AwaitingPCR)
    {
        /* First PCR so can't make comparison.  Use first
           PCR as reference - it may not be correct though */
        EnterCriticalSection();
        TmpCB_p->ActvRefPcr.Valid        = FALSE;
        TmpCB_p->ActvRefPcr.PcrBaseBit32 = PCRBaseBit32;
        TmpCB_p->ActvRefPcr.PcrBaseValue = PCRBaseValue;
        TmpCB_p->ActvRefPcr.PcrExtension = PCRExtension;
        TmpCB_p->ActvRefPcr.TotalValue   = PCRTimeTotal;
        TmpCB_p->ActvRefPcr.PcrArrivalBaseBit32 = STCBaseBit32;
        TmpCB_p->ActvRefPcr.PcrArrivalBaseValue = STCBaseValue;
        TmpCB_p->ActvRefPcr.PcrArrivalExtension = STCExtension;
        TmpCB_p->ActvRefPcr.Time = STCTimeTotal;
        TmpCB_p->ActvRefPcr.GlitchCount = 0;
        TmpCB_p->ActvRefPcr.Glitch      = FALSE;
        TmpCB_p->ActvRefPcr.AwaitingPCR = FALSE;
        TmpCB_p->ActvContext.Reset      = TRUE;       /* tell PWMFilter */
        LeaveCriticalSection();

#ifdef CLKRV_FILTERDEBUG
        /* Debug use only */

        /* Initialize the local filter data */
        memset(&FData, 0, sizeof(FilterData));

        FData.TotalSTCTime = STCTimeTotal;
        FData.TotalPCRTime = PCRTimeTotal;

        Dump_Debug_Data(1111, &FData, TmpCB_p);
#endif

    }
    else
    {

        /* PCR & reference seem plausible */
        TmpCB_p->ActvRefPcr.Valid = TRUE;

        /* adjust decode VCXO */
        RetVal = ClockRecoveryFilter(PCRTimeTotal, STCTimeTotal, TmpCB_p );

        EnterCriticalSection();
        switch(RetVal)
        {

            case PCR_NO_UPDATE:
                TmpCB_p->ActvRefPcr.GlitchCount++;
                break;

            case PCR_UPDATE_THIS_LAST_GLITCH:
                TmpCB_p->ActvRefPcr.Valid        = TRUE; /* ??? Added*/
                TmpCB_p->ActvRefPcr.PcrBaseBit32 = PCRBaseBit32;
                TmpCB_p->ActvRefPcr.PcrBaseValue = PCRBaseValue;
                TmpCB_p->ActvRefPcr.PcrExtension = PCRExtension;
                TmpCB_p->ActvRefPcr.TotalValue   = PCRTimeTotal;
                TmpCB_p->ActvRefPcr.PcrArrivalBaseBit32 = STCBaseBit32;
                TmpCB_p->ActvRefPcr.PcrArrivalBaseValue = STCBaseValue;
                TmpCB_p->ActvRefPcr.PcrArrivalExtension = STCExtension;
                TmpCB_p->ActvRefPcr.Time        = STCTimeTotal;
                TmpCB_p->ActvRefPcr.GlitchCount = 0;
                TmpCB_p->ActvRefPcr.Glitch      = TRUE; /* ??? Added*/
                break;

            case PCR_UPDATE_OK:
                /*Pcr Ok Update Reference*/
                TmpCB_p->ActvRefPcr.Valid        = TRUE; /*Added*/
                TmpCB_p->ActvRefPcr.PcrBaseBit32 = PCRBaseBit32;
                TmpCB_p->ActvRefPcr.PcrBaseValue = PCRBaseValue;
                TmpCB_p->ActvRefPcr.PcrExtension = PCRExtension;
                TmpCB_p->ActvRefPcr.TotalValue   = PCRTimeTotal;
                TmpCB_p->ActvRefPcr.PcrArrivalBaseBit32 = STCBaseBit32;
                TmpCB_p->ActvRefPcr.PcrArrivalBaseValue = STCBaseValue;
                TmpCB_p->ActvRefPcr.PcrArrivalExtension = STCExtension;
                TmpCB_p->ActvRefPcr.Time        = STCTimeTotal;
                TmpCB_p->ActvRefPcr.GlitchCount = 0;
                TmpCB_p->ActvRefPcr.Glitch      = FALSE;
                break;

            case PCR_INVALID:
                TmpCB_p->ActvRefPcr.Valid        = TRUE;
                TmpCB_p->ActvRefPcr.PcrBaseBit32 = PCRBaseBit32;
                TmpCB_p->ActvRefPcr.PcrBaseValue = PCRBaseValue;
                TmpCB_p->ActvRefPcr.PcrExtension = PCRExtension;
                TmpCB_p->ActvRefPcr.TotalValue   = PCRTimeTotal;
                TmpCB_p->ActvRefPcr.PcrArrivalBaseBit32 = STCBaseBit32;
                TmpCB_p->ActvRefPcr.PcrArrivalBaseValue = STCBaseValue;
                TmpCB_p->ActvRefPcr.PcrArrivalExtension = STCExtension;
                TmpCB_p->ActvRefPcr.Time = STCTimeTotal;
                TmpCB_p->ActvRefPcr.GlitchCount = 0;
                TmpCB_p->ActvRefPcr.Glitch      = TRUE;
                TmpCB_p->ActvRefPcr.AwaitingPCR = FALSE;
                TmpCB_p->ActvContext.Reset      = TRUE;       /* tell PWMFilter */

            case PCR_BURST:
                /* Ignore this PCR altogether, since
                   Previous reference PCR is OK anyway */
                break;

            default:
                break;
        }
        LeaveCriticalSection();
    }

    UpdateStateMachine( TmpCB_p, DiscontinuousPCR );

    STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);

}

/****************************************************************************
Name         : ClockRecoveryFilter()

Description  : performs filter function for Clock Recovery, programming
               FS registers

               Filtering is performed in three steps

               1. Error sample calculation
                    a. CalculateFrequencyError()
                    b. Pass the error through the LowPassfilter

               2. Averaging the error samples
                    a. ApplyWeighingFilter over moving window
                    b. Calculate Average error

               3. Calculate Correction & apply using FS registers


Parameters   : U32 PCR Total Time and
               U32 Captured Time of the Transport Stream in which PCR arrived
               STCLKRV_ControlBlock_t *Instance_p) instance to operate on

Return Value : Return code
               PCR_UPDATE_OK specifies valid pcr
               PCR_NO_UPDATE specifies unvalid pcr
               PCR_UPDATE_THIS_LAST_GLITCH specifies Glitch due to jitter
****************************************************************************/

U32 ClockRecoveryFilter( U32 TotalPCRTime,
                         U32 TotalSTCTime,
                         STCLKRV_ControlBlock_t *Instance_p )
{
    U32 ReturnCode    = (U32) PCR_UPDATE_OK;
    U8  SlaveClock = 0;
    FilterData FD;
    S32 CorrectionValue = 0;
    S32 SD_STC_Step = (S32) Instance_p->HWContext.Clocks[STCLKRV_CLOCK_SD_0].Step;

#ifdef CLKRV_FILTERDEBUG
    U32 Time2                   = 0;
    U32 Time1                   = 0;
#endif


    /* Initialize the local filter data */
    memset(&FD, 0, sizeof(FilterData));

    /* Store Current STC,PCR sample */
    FD.TotalSTCTime = TotalSTCTime;
    FD.TotalPCRTime = TotalPCRTime;

    /* Find current relative difference */
    FD.Difference = STOS_time_minus(FD.TotalSTCTime, FD.TotalPCRTime);

    /* If reset request then just reinitialise the pcr data */
    if( Instance_p->ActvContext.Reset )
    {
        Instance_p->ActvContext.Reset = FALSE;
        Instance_p->ActvContext.PrevDiff = FD.Difference;
        /* to find PCR time for frequeny error calculation */
        Instance_p->ActvContext.PrevPcr = FD.TotalPCRTime;

#ifdef CLKRV_FILTERDEBUG
        Dump_Debug_Data(5001, &FD, Instance_p);
#endif

        return  (U32) PCR_UPDATE_OK;
    }

                /****************************
                * CALCULATE AN ERROR SAMPLE *
                ****************************/

    ReturnCode = CalculateFrequencyError(&FD, Instance_p);

    if( ReturnCode != ST_NO_ERROR )
    {
        /* Reutrns in two cases either 1.Glitch or 2.PCR Burst */
        return ReturnCode;
    }

                /*********************************************
                * DISCARD ANY LARGE ERRORS (LOW PASS FILTER) *
                *********************************************/

    /* This return code to be preserved */
    ReturnCode = LowPassFilter(&FD, Instance_p);

    /* If error is too large than Discard the (PCR, STC) sample */
    if((ReturnCode == (U32)PCR_INVALID) || (ReturnCode ==  (U32)PCR_NO_UPDATE))
    {
        return ReturnCode;
    }

    /* If error sample is good then Process the error sample */

    /* Store the current difference for use in next call */
    Instance_p->ActvContext.PrevDiff = FD.Difference;

    /* store the current pcr for use in next call*/
    Instance_p->ActvContext.PrevPcr = FD.TotalPCRTime ;


                /*********************************************
                * MAINTAIN CIRCULAR LIST FOR MOVING AVERAGE  *
                *********************************************/

    /* Store current error value  */
    Instance_p->ActvContext.ErrorStore_p[Instance_p->ActvContext.Head] = FD.CurrentFreqErrorValue;
    if (Instance_p->ActvContext.ErrorStoreCount < (S32)Instance_p->InitPars.MaxWindowSize)
    {
        Instance_p->ActvContext.ErrorStoreCount++;
    }

    if (Instance_p->ActvContext.ErrorStoreCount < (S32)Instance_p->InitPars.MinSampleThres)
    {
        /* Maintain circular list head pointer */
        Instance_p->ActvContext.Head++;

        /* Store current error for averaging later */
        Instance_p->ActvContext.ErrorSum += FD.CurrentFreqErrorValue;

#ifdef CLKRV_FILTERDEBUG
        Dump_Debug_Data(2000, &FD, Instance_p);
#endif

        /* Not got enough data to average now, so exit */
        return  (U32) PCR_UPDATE_OK;
    }

    /* Maintain circular list head pointer */
    Instance_p->ActvContext.Head++;
    if (Instance_p->ActvContext.Head == Instance_p->InitPars.MaxWindowSize)
    {
        Instance_p->ActvContext.Head = 0;
    }

                /**********************************************
                * APPLY WEIGHING FILTER TO THE ERROR SAMPLES  *
                **********************************************/

#if defined (CLKRV_FILTERDEBUG)
    GetHPTimer(Time1);
#endif

    /* Apply Weighing filter to maintain the stability in PWM with Jittery Data */
    ApplyWeighingFilter(Instance_p);

#if defined (CLKRV_FILTERDEBUG)
    GetHPTimer(Time2);
    FD.ExecutionTime = Time2 - Time1;
#endif

                /*************************************************************
                * CALCULATE AVERAGE ERROR, Correction in MASTER/SLAVE clocks *
                *************************************************************/

    /* Recalculate average */
    FD.AverageError = DivideAndRound(Instance_p->ActvContext.ErrorSum,
                                  Instance_p->ActvContext.ErrorStoreCount);


    /* Apply Correction gradually, divide the avg. error by a multiple of the
       number of sample in the window. AEPS may be too small to correct, So it
       carried forward in out standing error */

    /* Multilpied by PRECISION_FACTOR=128 to increase the accuracy to two decimal digits */

    FD.AverageErrorPerSample = ((FD.AverageError * PRECISION_FACTOR)
                                 / (Instance_p->ActvContext.ErrorStoreCount
                                   * GRADUAL_CORRECTION_FACTOR));


    /* Find new control value +/- step in the current frequency to apply. */
    /* SD_STEP is already a multiple of */

    CorrectionValue = ((FD.AverageErrorPerSample + Instance_p->ActvContext.OutStandingErr)
                            / SD_STC_Step);

    /* SD_STEP is already a multiple of PRECIOSION factor */
    Instance_p->ActvContext.OutStandingErr += (FD.AverageErrorPerSample
                                                 - (CorrectionValue * SD_STC_Step));

    /* Upper and Lower Limit controlled inside */

    /* If there is change necessary... */
    if (CorrectionValue != 0)
    {
        /* Correct Pixel clock */
        /* DDTS 51128 fix */
        clkrv_ChangeClockFreqBy( Instance_p,
                             Instance_p->HWContext.Clocks[0].ClockType,
                             CorrectionValue);
        /* Correct other clocks if valid */
        Instance_p->SlaveCorrectionBeingApplied = TRUE;
        for ( SlaveClock = 1; SlaveClock < Instance_p->HWContext.ClockIndex ; SlaveClock++)
        {
            if (Instance_p->HWContext.Clocks[SlaveClock].Valid)
            {
                clkrv_CorrectSlaveClocks( Instance_p,
                                          CorrectionValue,
                                          Instance_p->HWContext.Clocks[SlaveClock].ClockType);
            }
        }
        Instance_p->SlaveCorrectionBeingApplied = FALSE;
    }

#ifdef CLKRV_FILTERDEBUG
    Dump_Debug_Data(555, &FD, Instance_p);
#endif

    return ReturnCode;

}

/****************************************************************************
Name         : CalculateFrequencyError()

Description  : Calculation of Frequency Error between Encoder and Decoder.

                1. tick error = (S1-S0) - (P1-P0)

                2. time = (P1-P0) / 27000000

                3. Frequency Error = Tick Error / time

                3a. Frequency Error = Tick Error * ( 27000000 / time )

Parameters   : FilterData * Pointer to local filter data
               STCLKRV_ControlBlock_t *Instance_p) instance to operate on

Return Value : Return code
               PCR_NO_UPDATE specifies unvalid pcr
****************************************************************************/

static ST_ErrorCode_t CalculateFrequencyError(FilterData *FilterData_p,
                                                STCLKRV_ControlBlock_t *CB_p)
{
    U32 ElapsedPCRTime = 0;

    /* Find current relative difference */
    FilterData_p->Difference = STOS_time_minus(FilterData_p->TotalSTCTime,
                                                FilterData_p->TotalPCRTime );

    /* Time elapsed can only be positive */
    FilterData_p->PCRTickDiff = FilterData_p->TotalPCRTime - CB_p->ActvContext.PrevPcr;

    /* If PCR events are very close then Error calculation gives correct a quite large
       frequency error which is less than threshold. see DDTS - 28442 */

    if((FilterData_p->PCRTickDiff < MINIMUM_TIME_BETWEEN_PCR) &&
            ((FilterData_p->TotalSTCTime - CB_p->ActvRefPcr.Time) < MINIMUM_TIME_BETWEEN_PCR))
    {
#ifdef CLKRV_FILTERDEBUG
        Dump_Debug_Data(6001, FilterData_p, CB_p);
#endif
        return (ST_ErrorCode_t) PCR_BURST;
    }

    /* Tick Error */
    FilterData_p->CurrentTicksErrorValue = (S32)(CB_p->ActvContext.PrevDiff
                                                    - FilterData_p->Difference);
    /*
     * To Handle the OVERFLOW in the equation, 27MHz is
     *  first divided by time and then multiplied by Tick error
    */

    /* Handle Divide by Zero */
    if (FilterData_p->PCRTickDiff != 0)
    {
        /* encoder frequency assumed 27MHz Nominal */
        ElapsedPCRTime = DivideAndRound((S32)ENCODER_FREQUENCY, FilterData_p->PCRTickDiff);
    }
    else
    {
#ifdef CLKRV_FILTERDEBUG
        Dump_Debug_Data(7001, FilterData_p, CB_p);
#endif
        return (ST_ErrorCode_t) PCR_NO_UPDATE;
    }

    FilterData_p->CurrentFreqErrorValue = ((S32)ElapsedPCRTime * FilterData_p->CurrentTicksErrorValue);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : LowPassFilter()

Description  : Discard any Frequency Error sample which is Larger
               than maximum Error. Maximum error is calculated on the basis of
               value of PCRDriftThres init parameter.
               Maximum Error =  PCRDriftThres * 40,
               See details in API document

               It takes new Reference Point if PCRMaxGlitch number of
               continous large error found.

Parameters   : FilterData * Pointer to local filter data
               STCLKRV_ControlBlock_t *Instance_p) instance to operate on

Return Value : Return code
               PCR_UPDATE_OK specifies valid pcr
               PCR_NO_UPDATE specifies unvalid pcr
               PCR_UPDATE_THIS_LAST_GLITCH specifies Glitch due to jitter
****************************************************************************/

static ST_ErrorCode_t LowPassFilter(FilterData *FilterData_p, STCLKRV_ControlBlock_t *CB_p)
{

    U32 Error = 0 ;
    U32 ErrorThres = 0;
    U32 Glitches =0;

    /* Load in local variables */

    Error      = abs(FilterData_p->CurrentFreqErrorValue);
    ErrorThres = CB_p->ActvContext.MaxFrequencyError;
    Glitches   = CB_p->ActvRefPcr.GlitchCount;

    /* reference thought OK so glitch in PCR - if we have
       too many of these we should check the value of PCRDrfit */


    if (CB_p->ActvRefPcr.Valid)
    {
        if((Error >= ErrorThres) && (Glitches == 0))
        {

#ifdef CLKRV_FILTERDEBUG
            Dump_Debug_Data(1001, FilterData_p, CB_p);
#endif
            /* first large error, Discard this ERROR and no updation of the PCR */
            return (ST_ErrorCode_t) PCR_NO_UPDATE;

        }
        else if((Error < ErrorThres) && (Glitches == 1))
        {
            /* single spike of large error, Process this ERROR and update PCR */
             return (ST_ErrorCode_t) PCR_UPDATE_THIS_LAST_GLITCH;
        }
        else if( (Error >= ErrorThres) && (Glitches == 1))
        {

#ifdef CLKRV_FILTERDEBUG
            Dump_Debug_Data(1002, FilterData_p, CB_p);
#endif
            /* Second continous large error, Discard this ERROR and no updation of the PCR */
            return (ST_ErrorCode_t) PCR_NO_UPDATE;

        }
        else if( (Error < ErrorThres) && (Glitches >= CB_p->InitPars.PCRMaxGlitch))
        {
            /* +/- type of frequently occuring large errors */
            /* Process the ERROR and Update the PCR */
            return (ST_ErrorCode_t) PCR_UPDATE_THIS_LAST_GLITCH;
        }
        else if( (Error >= ErrorThres) && (Glitches >= CB_p->InitPars.PCRMaxGlitch))
        {

#ifdef CLKRV_FILTERDEBUG
            Dump_Debug_Data(3333, FilterData_p, CB_p);
#endif
            /* too many of large errors, take new reference point */
            return (ST_ErrorCode_t) PCR_INVALID;
        }
        else
        {
            return (ST_ErrorCode_t) PCR_UPDATE_OK;
        }
    }
    else
    {
        /* take new reference point */
        return (ST_ErrorCode_t) PCR_INVALID;
    }

}

/****************************************************************************
Name         : ApplyWeighingFilter()

Description  : A weighting pattern is applied to the error sample
               data. defined frac = 1 / half the window size, then
               val[0] weighting is frac, val[1] weighting is frac*2, etc.,
               up to the middle, then it starts going down again.
               Frac = 1/(Window Size/2)

               From start to middle

                i=0; i<mid; i++
                    sample = (sample * 2 * ( i - start))/ windowsize

               and from middle to end

                i=mid; i<end; i++
                    sample = (sample * 2 * ( end - i))/ windowsize


Parameters   : STCLKRV_ControlBlock_t *Instance_p) instance to operate on

Return Value : Return code
               NONE
****************************************************************************/

static void ApplyWeighingFilter(STCLKRV_ControlBlock_t *CB_p)
{

    S32 TempI     = 0;
    S32 WinOldest = 0;      /* index of oldest element */
    S32 WinLatest = 0;      /* index of latest element */
    S32 WinMid    = 0;
    S32 WinCount  = 0;      /* error sample count in the window */
    S32 WinEnd    = 0;
    S32 WinSum    = 0;      /* Final sum after weighing */

    WinCount = CB_p->ActvContext.ErrorStoreCount;
    WinSum   = 0;

    if (WinCount < (S32)CB_p->InitPars.MaxWindowSize)
    {
        WinOldest = 0;
        WinLatest = WinCount -1;
        WinMid    = WinCount / 2;
        WinEnd    = WinCount - 1;
    }
    else
    {
        WinLatest = CB_p->ActvContext.Head;

        WinOldest = WinLatest + 1;
        if (WinOldest > ((S32)CB_p->InitPars.MaxWindowSize-1))
            WinOldest = 0;

        WinMid   = WinCount / 2;
        WinEnd   = WinCount - 1;
    }

    /* Starting from element 2, Since first element is ZERO anyway */
    for(TempI=1;TempI<WinMid;TempI++)
    {
        WinSum += (CB_p->ActvContext.ErrorStore_p[(WinOldest + TempI) % WinCount]* TempI);
    }

    /* Last element is ZERO too */
    for(TempI=WinMid;TempI<(WinCount-1);TempI++)
    {
        WinSum += (CB_p->ActvContext.ErrorStore_p[(WinOldest + TempI) % WinCount]* (WinEnd - TempI));
    }

    WinSum *= 2;
    WinSum /= ((S32)CB_p->InitPars.MaxWindowSize);

    /* Store the Sum in context and Return */
    CB_p->ActvContext.ErrorSum = WinSum;
}

/****************************************************************************
Name         : DivideAndRound()

Description  : Divides SIGNED Numerator by UNSIGNED denominator and returns
               value rounded to nearest whole value.
               ASSUMES a postive denom always.

Parameters   : S32 Num : Numerator
               U32 Denom : Denominator

Return Value : S32 to rounded nearest whole number
****************************************************************************/
static S32 DivideAndRound( S32 Num, U32 Denom )
{
    S32 Quo;
    S32 Rem;

    BOOL Neg = FALSE;

    /* If Num is negative */
    if (Num < 0)
    {
        /* set neagitve flag and force all values to positive for calculation */
        Neg = TRUE;
        Num = abs(Num);
    }

    /* Calc whole value */
    Quo = (Num / (S32)Denom);

    /* Calc remainder */
    Rem = (Num % (S32)Denom);
    /* If remainder >= 0.5, round up */
    if ((2 * Rem) >= (S32)Denom)
    {
        Quo++;
    }

    /* If the Num was negative, quo is neg too */
    if (Neg == TRUE)
    {
        Quo = -Quo;
    }

    return (Quo);
}


/****************************************************************************
Name         : Dump_Debug_Data()

Description  : Dumps the key variable of filter

Parameters   : U32 Code specifies execution path
               FilterData *FilterData_p filter data structure
               STCLKRV_ControlBlock_t *Instance_p instance to operate on

Return Value : Return code
               NONE
****************************************************************************/

#ifdef CLKRV_FILTERDEBUG
void Dump_Debug_Data(U32 Code, FilterData *FilterData_p,
                        STCLKRV_ControlBlock_t *CB_p)
{
    if (SampleCount < CLKRV_MAX_SAMPLES)
    {
        ClkrvDbg[0][SampleCount]  = Code;
        ClkrvDbg[2][SampleCount]  = FilterData_p->TotalSTCTime;
        ClkrvDbg[3][SampleCount]  = FilterData_p->TotalPCRTime;
        ClkrvDbg[1][SampleCount]  = CB_p->HWContext.Clocks[STCLKRV_CLOCK_SD_0].Frequency;
                                    /* May be SD frequency*/;
        ClkrvDbg[2][SampleCount]  = FilterData_p->TotalSTCTime;
        ClkrvDbg[3][SampleCount]  = FilterData_p->TotalPCRTime;
        ClkrvDbg[4][SampleCount]  = FilterData_p->CurrentFreqErrorValue;
        ClkrvDbg[5][SampleCount]  = CB_p->ActvContext.ErrorStoreCount;
        ClkrvDbg[6][SampleCount]  = CB_p->ActvContext.ErrorSum;
        ClkrvDbg[7][SampleCount]  = FilterData_p->AverageError;
        ClkrvDbg[8][SampleCount]  = FilterData_p->CurrentTicksErrorValue;
        ClkrvDbg[9][SampleCount]  = FilterData_p->AverageErrorPerSample;
        ClkrvDbg[10][SampleCount] = CB_p->ActvContext.OutStandingErr;
        ClkrvDbg[11][SampleCount] = FilterData_p->ExecutionTime;

        Clkrv_Slave_Dbg[0][SampleCount]  = CB_p->HWContext.Clocks[1].Frequency;
        Clkrv_Slave_Dbg[1][SampleCount]  = CB_p->HWContext.Clocks[2].Frequency;

        SampleCount++;
    }
}
#endif

/****************************************************************************
Name         : clkrv_CorrectSlaveClocks()

Description  : Calculates the Freq Change required by Slave Clocks depending
               on the Freq. change in the Master Clock.

Parameters   : Poniter to STCLKRV_ControlBlock_t instance
               Frequency Error to correct
               Clock Type to apply correction to.

Return Value :
****************************************************************************/

void clkrv_CorrectSlaveClocks( STCLKRV_ControlBlock_t *CB_p,
                                       S32 ControlVal,
                                       STCLKRV_ClockSource_t  Clock )
{
    U32 Index;

    Index = FindClockIndex(CB_p,Clock);

    if(CB_p->HWContext.Clocks[Index].Valid == 0 )
    {
        return ;
    }

    if (CB_p->HWContext.Clocks[Index].DriftError > 0)
    {
        ControlVal--;
        /* Error is corrected */
        CB_p->HWContext.Clocks[Index].DriftError = 0;
    }
    else if (CB_p->HWContext.Clocks[Index].DriftError < 0)
    {
        ControlVal++;
        /* Error is corrected */
        CB_p->HWContext.Clocks[Index].DriftError = 0;
    }
    else
    {/* Nothing */}

    if(ControlVal != 0)
    {
        clkrv_ChangeClockFreqBy(CB_p,Clock, ControlVal);
    }

    return ;
}

#endif /* STCLKRV_NO_PTI */

/****************************************************************************
Name         : clkrv_ChangeClockFreqBy

Description  : Changes a clock freq. by +/- freq (multiple of signed Control)

Parameters   : Pointer to STCLKRV_ControlBlock_t
               STCLKRV_ClockSource_t type clock
               S32 Control value

Return Value : None.

See Also     : clkrv_ProgramFSRegs
****************************************************************************/
void clkrv_ChangeClockFreqBy( STCLKRV_ControlBlock_t *CB_P,
                              STCLKRV_ClockSource_t Clock,
                              S32 Control )
{
    U16 ChangePos_ipe = 0;
    S16 ChangeNeg_ipe = 0;
    U16 ChangeBy      = 0;
    U32 Index;
    S8  md;
    U16 ipe;
    U32 Step;

    STSYS_DU32 *BaseAddress = NULL;

#if defined (WA_GNBvd56512)
    S32 Control1;
    U32 i,ChangeBy1=1;
    if(Clock ==  STCLKRV_CLOCK_SD_0)
    {

        Index = FindClockIndex(CB_P,Clock);
        /* SD can not change */
        md    = CB_P->HWContext.Clocks[Index].MD;
        ipe   = CB_P->HWContext.Clocks[Index].IPE;
        Step  = CB_P->HWContext.Clocks[Index].Step;

#ifdef ST_OSLINUX
        BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
        BaseAddress = (STSYS_DU32 *)CB_P->InitPars.FSBaseAddress_p;
#endif

        /*
                    <- range  ->
        Min............0............Max
        */
        ChangeBy = abs(Control);

        if(ChangeBy > 1)
            ChangeBy1=1;
        else
            ChangeBy1=ChangeBy;

        if( Control > 0)
        {
            Control1= ChangeBy1;

            ChangePos_ipe = (ipe + ChangeBy1);
            if (ChangePos_ipe  > 32767)
            {
                ipe = (ChangeBy1  - (32768 - ipe));
                md--;
            }
            else
                ipe += ChangeBy1;
        }
        else
        {
            Control1= -ChangeBy1;
            ChangeNeg_ipe = ipe - ChangeBy1;
            if (ChangeNeg_ipe < 0)
            {
                ipe = (32768 - (ChangeBy1 - ipe));
                md++;
            }
            else
                ipe -= ChangeBy1;
        }
        /* Switch to new freq. */
        /* Only MD and IPE are supposed to be chnaged */
        CB_P->HWContext.Clocks[Index].MD   = md;
        CB_P->HWContext.Clocks[Index].IPE  = ipe;
        CB_P->HWContext.Clocks[Index].Frequency += ((Control1 * (S32)Step)/ACCURACY_FACTOR);
        /* Switch to new freq. */
        clkrv_setClockFreq(CB_P, Clock, Index);
    }
#else
    Index = FindClockIndex(CB_P,Clock);

    /* SD can not change */
    md    = CB_P->HWContext.Clocks[Index].MD;
    ipe   = CB_P->HWContext.Clocks[Index].IPE;
    Step  = CB_P->HWContext.Clocks[Index].Step;

#ifdef ST_OSLINUX
    BaseAddress = (STSYS_DU32 *)CB_P->FSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_P->InitPars.FSBaseAddress_p;
#endif

    /*
                 <- range  ->
      Min............0............Max
    */

    ChangeBy = abs(Control);

    if( Control > 0)
    {
        ChangePos_ipe = (ipe + ChangeBy);
        if (ChangePos_ipe  > 32767)
        {
            ipe = (ChangeBy  - (32768 - ipe));
            md--;
        }
        else
            ipe += ChangeBy;
    }
    else
    {
        ChangeNeg_ipe = ipe - ChangeBy;
        if (ChangeNeg_ipe < 0)
        {
            ipe = (32768 - (ChangeBy - ipe));
            md++;
        }
        else
            ipe -= ChangeBy;
    }

    /* Update last control value and frequency */
    CB_P->HWContext.Clocks[Index].Frequency += ((Control * (S32)Step)/ACCURACY_FACTOR);
    /* Only MD and IPE are supposed to be chnaged */
    CB_P->HWContext.Clocks[Index].MD   = md;
    CB_P->HWContext.Clocks[Index].IPE  = ipe;

    /* Switch to new freq. */
    clkrv_setClockFreq(CB_P, Clock, Index);

#endif
}

/****************************************************************************
Name         : clkrv_setClockFreq()

Description  : Program the FS registers with the Sdiv, Md and Ipe values

Parameters   : Pointer to STCLKRV_ControlBlock_t
               STCLKRV_ClockSource_t type clock

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_BAD_PARAMETER          Invalid Handle


See Also     : clkrv_ChangeClockFreqBy
****************************************************************************/

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)
ST_ErrorCode_t clkrv_setClockFreq( STCLKRV_ControlBlock_t *CB_p,
                                   STCLKRV_ClockSource_t Clock,
                                   U32 Index )
{

    U32 ClockOffset0 = 0;
    U32 ClockOffset1 = 0;
    U32 RegValue     = 0;
    U16 i = 0;
    U16 Sdiv;
    S8  Md;
    U16 Ipe;
    STSYS_DU32 *BaseAddress = NULL;

#ifdef ST_OSLINUX /* Just in case :-) */
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
#endif

    Sdiv = CB_p->HWContext.Clocks[Index].SDIV;
    Md   = CB_p->HWContext.Clocks[Index].MD;
    Ipe  = CB_p->HWContext.Clocks[Index].IPE;

    if(!((Md  <= -1) && (Md  >= -17)))
        return (ST_ERROR_BAD_PARAMETER);

    if(!((Sdiv == 2)||(Sdiv==4) ||(Sdiv==8)||
         (Sdiv==16) ||(Sdiv==32)||(Sdiv==64)||
         (Sdiv==128)||(Sdiv==256)))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    switch(Clock)
    {
        case STCLKRV_CLOCK_SD_0:
            ClockOffset0 = CLK_SD_0_SETUP0;
            ClockOffset1 = CLK_SD_0_SETUP1;
            break;
        case STCLKRV_CLOCK_PCM_0:
            ClockOffset0 = CLK_PCM_0_SETUP0;
            ClockOffset1 = CLK_PCM_0_SETUP1;
            break;
        case STCLKRV_CLOCK_SPDIF_0:
            ClockOffset0 = CLK_SPDIF_0_SETUP0;
            ClockOffset1 = CLK_SPDIF_0_SETUP1;
            break;
        default:
            return (ST_ERROR_BAD_PARAMETER);
    }

    /**************************************
    **************************************/

    /*clkrv_writeReg(BaseAddress, DCO_MODE_CFG, 0x00);*/


    if(Clock == STCLKRV_CLOCK_SD_0)
    {
        /* Program EN_PRG bit to 0 for SD clock */
        RegValue = 0;
        clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);
        clkrv_writeReg(BaseAddress, ClockOffset0, RegValue&0xFFFFFFDF );
    }
    else
    {
        /* Program the DCO_MODE_CFG 0x00000000 */
        clkrv_writeReg(BaseAddress, DCO_MODE_CFG, 0x00);
    }
    /* Update sdiv / md / ipe */

    /* Update Sdiv */

    RegValue = 0;

    clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);

    RegValue &= (~SDIV_MASK); /*0xFFFFE3F */

    i = 0;
    while(!((Sdiv >>= 1) & 0x01))
    {
        i++;
    }

    RegValue |= ((i << SD_BIT_OFFSET)& SDIV_MASK);

    /* Update Md */
    RegValue &= (~MD_MASK); /*0xFFFFFFE0*/
    RegValue |= (COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md));

    /* "Freq synth normal use" & "Dig. Algo Works Normaly" & "O/P enabled" */
    RegValue |= (U32)FS_OUTPUT_NORMAL;

    /* Program SDIV & MD  value */
    clkrv_writeRegUnLock(BaseAddress, ClockOffset1, (Ipe & IPE_MASK));

    if(Clock == STCLKRV_CLOCK_SD_0)
    {
        /* Program SDIV & MD  value with EN_PRGn still zero */
        clkrv_writeRegUnLock(BaseAddress, ClockOffset0, RegValue&0xFFFFFFDF);
    }
    else
    {
        clkrv_writeRegUnLock(BaseAddress, ClockOffset0, RegValue);
    }

    if(Clock == STCLKRV_CLOCK_SD_0)
    {
        /* Set EN_PRG to 1 for SDIV, MD, IPE values to be taken into account */
        clkrv_writeRegUnLock(BaseAddress, ClockOffset0, RegValue|0x20);
    }
    else
    {
        /* Program the DCO_MODE_CFG 0x00000001 */
        clkrv_writeReg(BaseAddress, DCO_MODE_CFG, 0x01);
    }

    /* 10 neno-seconds Delay */
    /* DO NOT TOUCH these following Instruction they there to Give
     * required delay DDTS -HW for 5100 & 5105,5107
    */

    CLKRV_DELAY(10);

    /* Glitch free clock */

    if(Clock == STCLKRV_CLOCK_SD_0)
    {
        /* Program EN_PRG to 0 */
        clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);
        clkrv_writeReg(BaseAddress, ClockOffset0, RegValue&0xFFFFFFDF );
    }
    else
    {
        /* Program the DCO_MODE_CFG 0x00000000 */
        clkrv_writeReg(BaseAddress, DCO_MODE_CFG, 0x00);
    }

    return ST_NO_ERROR;
}


#elif defined (ST_5188) || defined (ST_5525)

ST_ErrorCode_t clkrv_setClockFreq( STCLKRV_ControlBlock_t *CB_p,
                                   STCLKRV_ClockSource_t Clock,
                                   U32 Index )
{

    U32 ClockOffset0 = 0;
    U32 ClockOffset1 = 0;
    U32 RegValue     = 0;
    U16 i = 0;
    U16 Sdiv;
    S8  Md;
    U16 Ipe;
    STSYS_DU32 *BaseAddress = NULL;

#ifdef ST_OSLINUX /* Just in case :-) */
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
#endif

    Sdiv = CB_p->HWContext.Clocks[Index].SDIV;
    Md   = CB_p->HWContext.Clocks[Index].MD;
    Ipe  = CB_p->HWContext.Clocks[Index].IPE;

    if(!((Md  <= -1) && (Md  >= -17)))
        return (ST_ERROR_BAD_PARAMETER);

    if(!((Sdiv == 2)||(Sdiv==4) ||(Sdiv==8)||
         (Sdiv==16) ||(Sdiv==32)||(Sdiv==64)||
         (Sdiv==128)||(Sdiv==256)))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    switch(Clock)
    {
        case STCLKRV_CLOCK_SD_0:
            ClockOffset0 = CLK_SD_0_SETUP0;
            ClockOffset1 = CLK_SD_0_SETUP1;
            break;
        case STCLKRV_CLOCK_PCM_0:
            ClockOffset0 = CLK_PCM_0_SETUP0;
            ClockOffset1 = CLK_PCM_0_SETUP1;
            break;
        case STCLKRV_CLOCK_SPDIF_0:
            ClockOffset0 = CLK_SPDIF_0_SETUP0;
            ClockOffset1 = CLK_SPDIF_0_SETUP1;
            break;
#if defined (ST_5525)
        case STCLKRV_CLOCK_SD_1:
            ClockOffset0 = CLK_SD_1_SETUP0;
            ClockOffset1 = CLK_SD_1_SETUP1;
            break;
        case STCLKRV_CLOCK_PCM_1:
            ClockOffset0 = CLK_PCM_1_SETUP0;
            ClockOffset1 = CLK_PCM_1_SETUP1;
            break;
        case STCLKRV_CLOCK_PCM_2:
            ClockOffset0 = CLK_PCM_2_SETUP0;
            ClockOffset1 = CLK_PCM_2_SETUP1;
            break;
        case STCLKRV_CLOCK_PCM_3:
            ClockOffset0 = CLK_PCM_3_SETUP0;
            ClockOffset1 = CLK_PCM_3_SETUP1;
            break;
#endif
        default:
            return (ST_ERROR_BAD_PARAMETER);
    }

    /**************************************
    **************************************/

    /* EN_PRG is ANDed with DCO_CFG_BITS
       DCO_CFG bits are enabled for all clocks
       EN_PRG = 0;
    */

    RegValue = 0;

    clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);
    clkrv_writeReg(BaseAddress, ClockOffset0, RegValue&0xFFFFFFDF );

    /* Update sdiv / md / ipe */

    /* Update Sdiv */

    RegValue = 0;

    clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);

    RegValue &= (~SDIV_MASK);

    i = 0;
    while(!((Sdiv >>= 1) & 0x01))
    {
        i++;
    }

    RegValue |= ((i << SD_BIT_OFFSET)& SDIV_MASK);

    /* Update Md */
    RegValue &= (~MD_MASK);
    RegValue |= (COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md));

    /* "Freq synth normal use" & "Dig. Algo Works Normaly" & "O/P enabled" */
    RegValue |= (U32)FS_OUTPUT_NORMAL;

    /* Program SDIV & MD  value */
    clkrv_writeRegUnLock(BaseAddress, ClockOffset0, RegValue);

    /* Assumed that reserved Bits should be ZERO */
    RegValue = (Ipe & IPE_MASK);
    clkrv_writeRegUnLock(BaseAddress, ClockOffset1, RegValue);

    /* SET EN_PRG = 1 */
    clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);
    clkrv_writeReg(BaseAddress, ClockOffset0, RegValue|0x20 );

    /* 10 nano-seconds Delay */
    /* DO NOT TOUCH these following Instruction they there to Give
     * required delay DDTS -HW for 5100 & 5105 5107
    */

    CLKRV_DELAY(10);

    /* Glitch free clock */
    /* EN_PRG = 0 */

    clkrv_readReg(BaseAddress, ClockOffset0, &RegValue);
    clkrv_writeReg(BaseAddress, ClockOffset0, RegValue&0xFFFFFFDF );


    return ST_NO_ERROR;
}




#elif defined (ST_7710)

ST_ErrorCode_t clkrv_setClockFreq( STCLKRV_ControlBlock_t *CB_p,
                                   STCLKRV_ClockSource_t Clock,
                                   U32 Index )
{

    U32 FS_Setup0, FS_Setup1;
    U32 RegValue    = 0;
    U16 i = 0;
    U16 Sdiv;
    S8  Md;
    U16 Ipe;
    STSYS_DU32 *BaseAddress = NULL;
    STSYS_DU32 *BaseAddress_AUDFS = NULL;

#ifdef ST_OSLINUX /* Just in case :-) */
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
    BaseAddress_AUDFS = (STSYS_DU32 *)CB_p->AUDFSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
    BaseAddress_AUDFS = (STSYS_DU32 *)CB_p->InitPars.ADSCBaseAddress_p;
#endif

    Sdiv = CB_p->HWContext.Clocks[Index].SDIV;
    Md   = CB_p->HWContext.Clocks[Index].MD;
    Ipe  = CB_p->HWContext.Clocks[Index].IPE;


    if (Clock == STCLKRV_CLOCK_SD_0 )
    {
        FS_Setup0 = CLK_SD_0_SETUP0;
        FS_Setup1 = CLK_SD_0_SETUP1;
    }
    else if (Clock == STCLKRV_CLOCK_HD_0 )
    {
        FS_Setup0 = CLK_HD_0_SETUP0;
        FS_Setup1 = CLK_HD_0_SETUP1;
    }

    if(!((Md  <= -1) && (Md  >= -17)))
        return (ST_ERROR_BAD_PARAMETER);

    if(!((Sdiv == 2)||(Sdiv==4) ||(Sdiv==8)||
         (Sdiv==16) ||(Sdiv==32)||(Sdiv==64)||
         (Sdiv==128)||(Sdiv==256)))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (Clock == STCLKRV_CLOCK_PCM_0)
    {
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_PROG, 0x00);
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_CFG, 0x00000008);

        /* md */
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_MD,
                                        (U32) (COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md)));

        /* ipe */
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_PE, (U32)Ipe & 0x0000FFFF);

        /* sdiv */
        i = 0;
        while(!((Sdiv >>= 1) & 0x01))
        {
            i++;
        }

        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_SDIV, (U32) i );
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_PSEL, 0x00000001);
        /* programming sequence En_prg */
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_PROG, 0x01);
        clkrv_writeReg(BaseAddress_AUDFS, ADSC_CKG_PROG, 0x00);

    }
    else if((Clock == STCLKRV_CLOCK_SD_0) || (Clock == STCLKRV_CLOCK_HD_0))
    {
        RegValue = 0;
        /* Set the EN_PRG bit to Zero */
        clkrv_readReg(BaseAddress, FS_Setup0, &RegValue);
        RegValue &= (~EN_PRG_BIT_MASK);
        RegValue = RegValue | UNLOCK_KEY;   /* key for registers access */
        clkrv_writeReg(BaseAddress, FS_Setup0, RegValue);

        RegValue = 0;

        /* Program PE */
        RegValue = (U32)Ipe & PE_BITS_MASK;
        RegValue = RegValue | UNLOCK_KEY ;   /* key for registers access */

        clkrv_writeReg(BaseAddress, FS_Setup1, RegValue);

        /* sel_out=1  ,reset_n=0 , en_prog=0 */
        RegValue = 0x00000006;               /* DDTS 39620, NRESET_0_4 bit should be set,
                                                digital algo runs normally */

        /* sdiv */
        i = 0;
        while(!((Sdiv >>= 1) & 0x01))
        {
            i++;
        }


        RegValue |= (U32) (i << SDIV_BITS);

        /* Md */

        RegValue |= (U32)((COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md)) << MD_BITS );

        RegValue |= UNLOCK_KEY;   /* key for registers access */

        clkrv_writeReg(BaseAddress, FS_Setup0, RegValue);


        /* Set the EN_PRG bit to One */
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_Setup0, &RegValue);
        RegValue |= (U32)0x01;
        RegValue = RegValue | UNLOCK_KEY;   /* key for registers access */
        clkrv_writeReg(BaseAddress, FS_Setup0, RegValue);

        /* some delay may be required here DDTS - GNBvd37520 */
        CLKRV_DELAY(10);

        /* Set the EN_PRG bit to Zero */
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_Setup0, &RegValue);
        RegValue &= (~EN_PRG_BIT_MASK);
        RegValue = RegValue | UNLOCK_KEY;   /* key for registers access */
        clkrv_writeReg(BaseAddress, FS_Setup0, RegValue);


    }
    else
    { return (ST_ERROR_BAD_PARAMETER); }

    return ST_NO_ERROR;

}

#elif defined (ST_7100)  || defined (ST_7109)

ST_ErrorCode_t clkrv_setClockFreq( STCLKRV_ControlBlock_t *CB_p,
                                   STCLKRV_ClockSource_t Clock,
                                   U32 Index )
{

    U32 FS_SetMD, FS_SetPE, FS_SetPRG, FS_SetSDIV;
    U32 RegValue    = 0;
    U16 i = 0;
    U16 Sdiv;
    S8  Md;
    U16 Ipe;
    STSYS_DU32 *BaseAddress = NULL;
    STSYS_DU32 *BaseAddress_AUDFS = NULL;

#ifdef ST_OSLINUX
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
    BaseAddress_AUDFS = (STSYS_DU32 *)CB_p->AUDFSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
    BaseAddress_AUDFS = (STSYS_DU32 *)CB_p->InitPars.AUDCFGBaseAddress_p;
#endif

    Sdiv = CB_p->HWContext.Clocks[Index].SDIV;
    Md   = CB_p->HWContext.Clocks[Index].MD;
    Ipe  = CB_p->HWContext.Clocks[Index].IPE;

    switch (Clock)
    {
    case STCLKRV_CLOCK_SD_0:
        FS_SetMD   = CLK_SD_0_MD1;
        FS_SetPE   = CLK_SD_0_PE1;
        FS_SetPRG  = CLK_SD_0_ENPRG1;
        FS_SetSDIV = CLK_SD_0_SDIV1;
        break;

    case STCLKRV_CLOCK_PCM_0:
        FS_SetMD   = AUD_CKG_PCM0_MD;
        FS_SetPE   = AUD_CKG_PCM0_PE;
        FS_SetPRG  = AUD_CKG_PCM0_PROG;
        FS_SetSDIV = AUD_CKG_PCM0_SDIV;
        break;

    case STCLKRV_CLOCK_SPDIF_0:
        FS_SetMD   = AUD_CKG_SPDIF_MD;
        FS_SetPE   = AUD_CKG_SPDIF_PE;
        FS_SetPRG  = AUD_CKG_SPDIF_PROG;
        FS_SetSDIV = AUD_CKG_SPDIF_SDIV;
        break;

    case STCLKRV_CLOCK_HD_0:
        FS_SetMD   = CLK_HD_0_MD1;
        FS_SetPE   = CLK_HD_0_PE1;
        FS_SetPRG  = CLK_HD_0_ENPRG1;
        FS_SetSDIV = CLK_HD_0_SDIV1;
        break;

    case STCLKRV_CLOCK_PCM_1:
    default:
        FS_SetMD   = AUD_CKG_PCM1_MD;
        FS_SetPE   = AUD_CKG_PCM1_PE;
        FS_SetPRG  = AUD_CKG_PCM1_PROG;
        FS_SetSDIV = AUD_CKG_PCM1_SDIV;
        break;
    }

    if(!((Md  <= -1) && (Md  >= -17)))
        return (ST_ERROR_BAD_PARAMETER);

    if(!((Sdiv == 2)||(Sdiv==4) ||(Sdiv==8)||
         (Sdiv==16) ||(Sdiv==32)||(Sdiv==64)||
         (Sdiv==128)||(Sdiv==256)))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    /* Read FSYN_CFG reg */
    clkrv_readReg(BaseAddress_AUDFS, AUD_FSYN_CFG, &RegValue);
    /* Regvalue to select FS clock  */
    if (Clock == STCLKRV_CLOCK_PCM_0 )
    {
        RegValue |= 0x07fe6;
    }
    else if (Clock == STCLKRV_CLOCK_PCM_1 )
    {

        RegValue |= 0x07fea;
    }
    else if (Clock == STCLKRV_CLOCK_SPDIF_0 )
    {
        RegValue |= 0x07ff2;
    }
    if ( (Clock == STCLKRV_CLOCK_PCM_0) || (Clock == STCLKRV_CLOCK_PCM_1)
        || (Clock == STCLKRV_CLOCK_SPDIF_0))
    {
        /* Set the EN_PRG bit to Zero */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPRG, 0x00);
        /* Select FS clock only for the clock specified */
        clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYN_CFG, RegValue);

        /* md */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetMD,
                                        (U32) (COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md)));

        /* ipe */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPE, (U32)Ipe & 0x0000FFFF);

        /* sdiv */
        i = 0;
        while(!((Sdiv >>= 1) & 0x01))
        {
            i++;
        }

        clkrv_writeReg(BaseAddress_AUDFS, FS_SetSDIV, (U32) i );

        /* programming sequence En_prg */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPRG, 0x01);
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPRG, 0x00);

    }
    else if((Clock == STCLKRV_CLOCK_SD_0) || (Clock == STCLKRV_CLOCK_HD_0))
    {
        /* Set the EN_PRG bit to Zero */
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_SetPRG, &RegValue);
        RegValue &= (~EN_PRG_BIT_MASK);
        clkrv_writeReg(BaseAddress, FS_SetPRG, RegValue);

        /* Program PE */
        RegValue = 0;
        RegValue = (U32)Ipe & PE_BITS_MASK;
        clkrv_writeReg(BaseAddress, FS_SetPE, RegValue);

        /* sel_out=1  ,reset_n=0 , en_prog=0 */
        /*RegValue = 0x00000004;*/

        /* sdiv */
        i = 0;
        while(!((Sdiv >>= 1) & 0x01))
        {
            i++;
        }
        RegValue = (U32) (i);
        clkrv_writeReg(BaseAddress, FS_SetSDIV, RegValue);

        /* Md */
        RegValue = 0;
        RegValue = (U32)((COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md))) ;
        clkrv_writeReg(BaseAddress, FS_SetMD, RegValue);

        /* Set EN_PRG bit to 1*/
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_SetPRG, &RegValue);
        RegValue |= (U32)0x01;
        clkrv_writeReg(BaseAddress, FS_SetPRG, RegValue);

        /* some delay may be required here DDTS - GNBvd37520 */
        CLKRV_DELAY(10);

        /* Set the EN_PRG bit to Zero */
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_SetPRG, &RegValue);
        RegValue &= (~EN_PRG_BIT_MASK);
        clkrv_writeReg(BaseAddress, FS_SetPRG, RegValue);

    }
    else
    { return (ST_ERROR_BAD_PARAMETER); }

    return ST_NO_ERROR;

}

#elif defined (ST_7200)

ST_ErrorCode_t clkrv_setClockFreq( STCLKRV_ControlBlock_t *CB_p,
                                   STCLKRV_ClockSource_t Clock,
                                   U32 Index )
{

    U32 FS_ClkCFG = 0, FS_SetMD = 0, FS_SetPE = 0;
    U32 FS_SetPRG = 0, FS_SetSDIV = 0;
    U32 RegValue = 0, MdValue = 0;
    U32 i = 0;
    U32 Sdiv;
    S32  Md;
    U32 Ipe;
    STSYS_DU32 *BaseAddress = NULL;
    STSYS_DU32 *BaseAddress_AUDFS = NULL;

#ifdef ST_OSLINUX
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
    BaseAddress_AUDFS = (STSYS_DU32 *)CB_p->AUDFSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
    BaseAddress_AUDFS = (STSYS_DU32 *)CB_p->InitPars.AUDCFGBaseAddress_p;
#endif

    Sdiv = CB_p->HWContext.Clocks[Index].SDIV;
    Md   = CB_p->HWContext.Clocks[Index].MD;
    Ipe  = CB_p->HWContext.Clocks[Index].IPE;

    switch (Clock)
    {
    case STCLKRV_CLOCK_SD_0:
        FS_ClkCFG = CLK_SD_0_CFG;
        break;

    case STCLKRV_CLOCK_PCM_0:
        FS_SetMD   = AUD_CKG_PCM0_MD;
        FS_SetPE   = AUD_CKG_PCM0_PE;
        FS_SetPRG  = AUD_CKG_PCM0_PROG;
        FS_SetSDIV = AUD_CKG_PCM0_SDIV;
        break;

    case STCLKRV_CLOCK_HD_0:
        FS_ClkCFG = CLK_HD_0_CFG;
        break;

    case STCLKRV_CLOCK_SPDIF_0:
        FS_SetMD   = AUD_CKG_SPDIF_MD;
        FS_SetPE   = AUD_CKG_SPDIF_PE;
        FS_SetPRG  = AUD_CKG_SPDIF_PROG;
        FS_SetSDIV = AUD_CKG_SPDIF_SDIV;
        break;

    case STCLKRV_CLOCK_SD_1:
        FS_ClkCFG = CLK_SD_1_CFG;
        break;

    case STCLKRV_CLOCK_PCM_1:
        FS_SetMD   = AUD_CKG_PCM1_MD;
        FS_SetPE   = AUD_CKG_PCM1_PE;
        FS_SetPRG  = AUD_CKG_PCM1_PROG;
        FS_SetSDIV = AUD_CKG_PCM1_SDIV;
        break;

    case STCLKRV_CLOCK_HD_1:
        FS_ClkCFG = CLK_HD_1_CFG;
        break;

    case STCLKRV_CLOCK_PCM_2:
        FS_SetMD   = AUD_CKG_PCM2_MD;
        FS_SetPE   = AUD_CKG_PCM2_PE;
        FS_SetPRG  = AUD_CKG_PCM2_PROG;
        FS_SetSDIV = AUD_CKG_PCM2_SDIV;
        break;

    case STCLKRV_CLOCK_PCM_3:
        FS_SetMD   = AUD_CKG_PCM3_MD;
        FS_SetPE   = AUD_CKG_PCM3_PE;
        FS_SetPRG  = AUD_CKG_PCM3_PROG;
        FS_SetSDIV = AUD_CKG_PCM3_SDIV;
        break;

    case STCLKRV_CLOCK_SPDIF_HDMI_0:
        FS_SetMD   = AUD_CKG_HDMI_MD;
        FS_SetPE   = AUD_CKG_HDMI_PE;
        FS_SetPRG  = AUD_CKG_HDMI_PROG;
        FS_SetSDIV = AUD_CKG_HDMI_SDIV;
        break;
    default:
        break;
    }

    if(!((Md  <= -1) && (Md  >= -17)))
        return (ST_ERROR_BAD_PARAMETER);

    if(!((Sdiv == 2)||(Sdiv==4) ||(Sdiv==8)||
         (Sdiv==16) ||(Sdiv==32)||(Sdiv==64)||
         (Sdiv==128)||(Sdiv==256)))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if ( (Clock == STCLKRV_CLOCK_PCM_0) || (Clock == STCLKRV_CLOCK_PCM_1)
      || (Clock == STCLKRV_CLOCK_PCM_2) || (Clock == STCLKRV_CLOCK_PCM_3)
      || (Clock == STCLKRV_CLOCK_SPDIF_HDMI_0) || (Clock == STCLKRV_CLOCK_SPDIF_0) )
    {
        /* Set the EN_PRG bit to Zero */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPRG, 0x00);
        /* Select FS clock only for the clock specified */
        if ( (Clock == STCLKRV_CLOCK_SPDIF_HDMI_0) || (Clock == STCLKRV_CLOCK_SPDIF_0) )
            clkrv_readReg(BaseAddress_AUDFS, AUD_FSYNB_CFG, &RegValue);
        else
            clkrv_readReg(BaseAddress_AUDFS, AUD_FSYNA_CFG, &RegValue);

        /* Regvalue to select FS clock  */
        if (Clock == STCLKRV_CLOCK_PCM_0 )
            RegValue &= 0xFFFFFFFB;
        else if (Clock == STCLKRV_CLOCK_PCM_1 )
            RegValue &= 0xFFFFFFF7;
        else if ( (Clock == STCLKRV_CLOCK_PCM_2) || (Clock == STCLKRV_CLOCK_SPDIF_HDMI_0) )
            RegValue &= 0xFFFFFFEF;
        else if ( (Clock == STCLKRV_CLOCK_PCM_3) || (Clock == STCLKRV_CLOCK_SPDIF_0) )
            RegValue &= 0xFFFFFFDF;

        if ( (Clock == STCLKRV_CLOCK_SPDIF_HDMI_0) || (Clock == STCLKRV_CLOCK_SPDIF_0) )
            clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNB_CFG, RegValue);
        else
            clkrv_writeReg(BaseAddress_AUDFS, AUD_FSYNA_CFG, RegValue);

        /* md */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetMD,
                                       (U32) (COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md)));

        /* ipe */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPE, (U32)Ipe & 0x0000FFFF);

        /* sdiv */
        i = 0;
        while(!((Sdiv >>= 1) & 0x01))
        {
            i++;
        }

        clkrv_writeReg(BaseAddress_AUDFS, FS_SetSDIV, (U32) i );

        /* programming sequence En_prg */
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPRG, 0x01);
        clkrv_writeReg(BaseAddress_AUDFS, FS_SetPRG, 0x00);

    }
    else if( (Clock == STCLKRV_CLOCK_SD_0) || (Clock == STCLKRV_CLOCK_HD_0)
            || (Clock == STCLKRV_CLOCK_SD_1) || (Clock == STCLKRV_CLOCK_HD_1) )
    {
        /* Set the EN_PRG bit to Zero */
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_ClkCFG, &RegValue);
        RegValue &= (~EN_PRG_BIT_MASK);
        clkrv_writeReg(BaseAddress, FS_ClkCFG, RegValue);

        /* IPE */
        RegValue = 0;
        RegValue = (U32)Ipe & PE_BITS_MASK;

        /* sdiv */
        i = 0;
        while(!((Sdiv >>= 1) & 0x01))
        {
            i++;
        }
        RegValue |= ((i << SD_BIT_OFFSET)& SDIV_MASK);

        /* Md */
        MdValue = 0;
        MdValue = (U32)((COMPLEMENT2S_FOR_5BIT_VALUE - abs(Md))) ;
        RegValue |= ((MdValue << MD_BIT_OFFSET)& MD_MASK);

        /* Set selclkout to 1 */
        RegValue |= 0x02000000;

        /* Set SDIV, MD and IPE values */
        clkrv_writeReg(BaseAddress, FS_ClkCFG, RegValue);

        /* Set EN_PRG bit to 1*/
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_ClkCFG, &RegValue);
        RegValue |= (U32)EN_PRG_BIT_MASK;
        clkrv_writeReg(BaseAddress, FS_ClkCFG, RegValue);

        /* some delay may be required here DDTS - GNBvd37520 */
        CLKRV_DELAY(10);

        /* Set the EN_PRG bit to Zero */
        RegValue = 0;
        clkrv_readReg(BaseAddress, FS_ClkCFG, &RegValue);
        RegValue &= (~EN_PRG_BIT_MASK);
        clkrv_writeReg(BaseAddress, FS_ClkCFG, RegValue);

    }
    else
    { return (ST_ERROR_BAD_PARAMETER); }
    return ST_NO_ERROR;

}
#endif

/****************************************************************************
Name         : UpdateStateMachine()

Description  : Tracks validity of the PCR flag and generates events when
               the state changes.

Parameters   : STCLKRV_ControlBlock_t *Instance_p : ptr to instance concerned
               boolean DiscontinuousPCR   True if PCR is known to be discontinuous

Return Value : None
 ****************************************************************************/

void UpdateStateMachine(STCLKRV_ControlBlock_t *Instance_p, BOOL DiscontinuousPCR)
{
    U32  State;
    BOOL StateUpdated = FALSE;

    State = Instance_p->ActvRefPcr.MachineState;

     /* Determine if a state change is necessary */
    switch (State)
    {
        case STCLKRV_STATE_PCR_UNKNOWN:
            if( Instance_p->ActvRefPcr.Valid == TRUE )
            {
                State = (U32) STCLKRV_STATE_PCR_OK;
                StateUpdated = TRUE;
            }
            break;

        case STCLKRV_STATE_PCR_OK:
            if( DiscontinuousPCR == TRUE )
            {
                State = (U32) STCLKRV_STATE_PCR_INVALID;
                StateUpdated = TRUE;
            }
            else if ( Instance_p->ActvRefPcr.Glitch == TRUE )
            {
                State = (U32) STCLKRV_STATE_PCR_GLITCH;
                StateUpdated = TRUE;
            }
            break;

        case STCLKRV_STATE_PCR_INVALID:
            if( DiscontinuousPCR == TRUE )
            {
                StateUpdated = TRUE;
            }
            else if( Instance_p->ActvRefPcr.Valid == TRUE )
            {
                State = (U32) STCLKRV_STATE_PCR_OK;
                StateUpdated = TRUE;
            }
            break;

        case STCLKRV_STATE_PCR_GLITCH:
            if( DiscontinuousPCR == TRUE )
            {
              State = (U32) STCLKRV_STATE_PCR_INVALID;
              StateUpdated = TRUE;
            }
            else if(Instance_p->ActvRefPcr.Valid == TRUE )
            {
              State = (U32) STCLKRV_STATE_PCR_OK;
              StateUpdated = TRUE;
            }
            break;

        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STCLKRV PCR state is invalid"));
            EnterCriticalSection();
            Instance_p->ActvRefPcr.MachineState = (U32) STCLKRV_STATE_PCR_UNKNOWN;
            LeaveCriticalSection();
            break;
    }

    /* Generate an event if the state was updated */
    /* and if the PCR Handling is enabled */
    if ((StateUpdated == TRUE) && (Instance_p->ActvRefPcr.PCRHandlingActive == TRUE))
    {
        switch ( State )
        {
            case STCLKRV_STATE_PCR_OK:
                /* Lint warning --> (void) */
                (void)STEVT_Notify(Instance_p->ClkrvEvtHandle,
                            Instance_p->RegisteredEvents[STCLKRV_PCR_VALID_ID], NULL);
                break;

            case STCLKRV_STATE_PCR_INVALID:
                (void)STEVT_Notify(Instance_p->ClkrvEvtHandle,
                            Instance_p->RegisteredEvents[STCLKRV_PCR_INVALID_ID], NULL);
                break;

            case STCLKRV_STATE_PCR_GLITCH:
                (void)STEVT_Notify(Instance_p->ClkrvEvtHandle,
                            Instance_p->RegisteredEvents[STCLKRV_PCR_GLITCH_ID], NULL);
                break;

            default:
                break;
        }

        EnterCriticalSection();
        Instance_p->ActvRefPcr.MachineState = State;
        LeaveCriticalSection();
    }
}


/****************************************************************************
Name         : ClkrvTrackingInterruptHandler()

Description  : It calculates the drift error of slave clocks wr.r.t Master
               SD pixel clock

Parameters   : Instance Control Block

Return Value : NONE

See Also     : STCLKRV_Init()
****************************************************************************/
STOS_INTERRUPT_DECLARE(ClkrvTrackingInterruptHandler, Data)
{
    U32 RegValue    = 0;
    U32 CaptureCntOffset;
    U32 SlaveCounter;
    U32 i,Index;

    /* Wrong Init but to remove Lint warning */

    volatile U32 *BaseAddress = NULL;
    STCLKRV_ControlBlock_t *CB_p = NULL;

    CB_p = (STCLKRV_ControlBlock_t *)Data;

#ifdef ST_OSLINUX
    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
        BaseAddress    = (STSYS_DU32 *)CB_p->CRUMappedAddress_p;
    else
        BaseAddress    = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
        BaseAddress    = (STSYS_DU32 *)CB_p->InitPars.CRUBaseAddress_p;
    else
        BaseAddress    = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
#endif

    Index = CB_p->HWContext.ClockIndex;

#if defined (ST_5188) || defined (ST_5525)
    clkrv_readReg(BaseAddress, CLKRV_REC_STATUS(GetRegIndex(CB_p)), &RegValue );
    if(!(RegValue & 0x01))
#else
    clkrv_readReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), &RegValue );
    if(!(RegValue & 0x02))
#endif
    {
        /* Something Wrong but interrupt has been handled */
        STOS_INTERRUPT_EXIT(STOS_SUCCESS);
    }

    /* Read Latched Counter Values */

    /* Check if Freq are being modified currently in a different context */
    if (CB_p->SlaveCorrectionBeingApplied == FALSE){
        for (i = 1; i<Index; i++) {

            if ((CB_p->HWContext.Clocks[i].Valid == TRUE) &&
                (CB_p->HWContext.Clocks[i].SlaveWithCounter == TRUE))
            {

                CaptureCntOffset = GetCounterOffset(CB_p, CB_p->HWContext.Clocks[i].ClockType);
                clkrv_readReg(BaseAddress, CaptureCntOffset, &SlaveCounter);

#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182) || defined (WA_GNBvd56512)
                if (CB_p->HWContext.Clocks[i].ClockType == STCLKRV_CLOCK_PCM_0)
                {
                    Count_PCM = SlaveCounter;
                }
                else if (CB_p->HWContext.Clocks[i].ClockType == STCLKRV_CLOCK_HD_0)
                {
                    Count_PixelHD = SlaveCounter;
                }
#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182)
                /*Load the reference counter */
                clkrv_writeReg( BaseAddress, CLKRV_REF_MAX_CNT(GetRegIndex(CB_p)),
                    (U32)(ENCODER_FREQUENCY * STCLKRV_INTERRUPT_TIME_SPAN ));
#endif /* WA_GNBvd44290 || WA_GNBvd54182*/

#endif /* WA_GNBvd44290 || WA_GNBvd54182 || WA_GNBvd56512*/

#ifdef CLKRV_FILTERDEBUG
                ClkrvCounters[i] = SlaveCounter;
#endif
                /* Negative Error means slave is running SLOW,
                   since Expected ticks were more */
                CB_p->HWContext.Clocks[i].DriftError = (S32)SlaveCounter
                                   - (S32)CB_p->HWContext.Clocks[i].TicksToElapse;

                /* Drift error zero for application mode SD only */
#if defined (ST_7710) || defined(ST_7100) || defined(ST_7109)
                if ( (CB_p->HWContext.ApplicationMode == STCLKRV_APPLICATION_MODE_SD_ONLY)
                    && (CB_p->HWContext.Clocks[i].ClockType == STCLKRV_CLOCK_HD_0 ) )
                {
                    CB_p->HWContext.Clocks[i].DriftError = 0;
                }
#endif

            }
            else
            {
                CB_p->HWContext.Clocks[i].DriftError = 0;
            }
        }

#if defined(ST_7100) || defined(ST_7109)
        /* Calculate approx. drift error for PCM1 and SPDIF clocks which do not have
           the slave counters */
        for (i = 1; i<Index; i++) {

            if ((CB_p->HWContext.Clocks[i].Valid == TRUE) &&
                (CB_p->HWContext.Clocks[i].SlaveWithCounter == FALSE))
            {
                U32 FreqRatioPCM0, Index2;

                Index2 = FindClockIndex(CB_p,STCLKRV_CLOCK_PCM_0);

                /* Check if PCM0 clock is set or not */
                if( ( Index2 < CB_p->HWContext.ClockIndex) && (CB_p->HWContext.Clocks[Index2].Valid) )
                {
                    /* Calculate approx.drift error based upon PCM0 drift error */
                    if( CB_p->HWContext.Clocks[i].Frequency >= CB_p->HWContext.Clocks[Index2].Frequency )
                    {
                        /* Maximum value can be ~148.5MHz and multiplying by 10 to improve the accuracy will not overflow */
                        FreqRatioPCM0 = (CB_p->HWContext.Clocks[i].Frequency*10)/CB_p->HWContext.Clocks[Index2].Frequency;
                        CB_p->HWContext.Clocks[i].DriftError = ((CB_p->HWContext.Clocks[Index2].DriftError)*
                                                                                                 ((S32)FreqRatioPCM0))/10;
                    }
                    else
                    {
                        /* Multiply by 100 before dividing to improve accuracy */
                        FreqRatioPCM0 = (CB_p->HWContext.Clocks[i].Frequency*100)/CB_p->HWContext.Clocks[Index2].Frequency;
                        CB_p->HWContext.Clocks[i].DriftError = ((CB_p->HWContext.Clocks[Index2].DriftError)*
                                                                                                 ((S32)FreqRatioPCM0))/100;
                    }
                }
                else
                {
                    CB_p->HWContext.Clocks[i].DriftError = 0;
                }
            }
        }
#endif
    }

    /* Clear Interrupt so that it can come again */
    clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x02 );

#if defined(ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5188) \
 || defined (ST_5525) || defined (ST_5107)

    CLKRV_DELAY(5); /* HW Bug DDTS */
#endif

    clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x00 );
    clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x01);

#if defined(ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5188) \
 || defined (ST_5525) || defined (ST_5107)
    CLKRV_DELAY(10);
#endif

    clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x00);

    STOS_INTERRUPT_EXIT(STOS_SUCCESS);
}
#if (STCLKRV_CRYSTAL_ERROR != 0x00)
/****************************************************************************
Name         : clkrv_CorrectCrystalError()

Description  : Workaround to compensate for crystal error on 7100

Parameters   : Pointer to BaseAddress of the Array
               Register Offset
               Value to write to Register
               BOOL UnLocking/Locking Flag

Return Value : void
****************************************************************************/
void clkrv_CorrectCrystalError(void)
{
    S32 FreqError_27=0,FreqError=0;
    U32 AbsFreqError_27=0;
    S16 CorrectioninIpe=0,ChangeInMD;
    U16 CorrectedIPE=0,Carry=0;
    U32 i=0;

    FreqError_27 = STCLKRV_CRYSTAL_ERROR;
    AbsFreqError_27 = abs(FreqError_27);
    for( i = 0; i< MAX_NO_IN_FREQ_TABLE; i++)
    {
        /* Calculate frequency error */
        FreqError = (NominalFreqTable[i].Ratio * AbsFreqError_27)/10;
        FreqError = (FreqError_27 >= 0 ? FreqError : -FreqError);
        /* Calculate correction in IPE */
        CorrectioninIpe = (S16)(FreqError/NominalFreqTable[i].Step);
        CorrectedIPE = (U16)(NominalFreqTable[i].IPE + CorrectioninIpe);
        /* if carry, md needs to be changed */
        Carry = ((CorrectedIPE & 0x8000)>>15);
        /* calculate change in md, +ve or -ve change */
        ChangeInMD = (CorrectioninIpe >= 0 ? 1 : -1);
        /* MD change, +1, -1 or 0*/
        NominalFreqTable[i].MD  -= (ChangeInMD * Carry);
        /* Corrected IPE value */
        NominalFreqTable[i].IPE = CorrectedIPE & 0x7fff;
    }

}
#endif

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
/****************************************************************************
Name         : clkrv_writeReg()

Description  : Write to Register Array

Parameters   : Pointer to BaseAddress of the Array
               Register Offset
               Value to write to Register
               BOOL UnLocking/Locking Flag

Return Value : void
****************************************************************************/
__inline void clkrv_writeReg(STSYS_DU32 *Base, U32 Reg, U32 Value)
{
    volatile U32 *Regaddr;
#if defined(ST_7100) || defined (ST_7109)
    /* Unlock the registers */
    Regaddr = (U32 *)((U32)CKG_BASE_ADDRESS + 0x10);
    *Regaddr = 0xC0DE;
#endif

    Regaddr = (U32 *)((U32)Base + Reg);
    *Regaddr = Value;
}


/****************************************************************************
Name         : clkrv_writeRegUnlock()

Description  : Write to FS registers by Unlocking them and then locking

Parameters   : Pointer to BaseAddress of the Array
               Register Offset
               Value to write to Register
               BOOL UnLocking/Locking Flag

Return Value : void
****************************************************************************/
__inline void clkrv_writeRegUnLock(STSYS_DU32 *Base, U32 Reg, U32 Value)
{
    volatile U32 *Regaddr;

    EnterCriticalSection();
#if !defined (ST_5188)
    clkrv_writeReg(Base, 0x300, 0xF0);
    clkrv_writeReg(Base, 0x300, 0x0F);
#endif

    Regaddr = (U32 *)((U32)Base + Reg);
    *Regaddr = Value;

#if !defined (ST_5188)
#ifdef SYS_SERVICE_LOCKING
    Regaddr = (U32 *)((U32)Base + 0x300);
    *Regaddr = (*Regaddr | 0x00000100);
#endif
#endif
    LeaveCriticalSection();
}


/****************************************************************************
Name         : clkrv_readReg()

Description  : Read from Register Array

Parameters   : Pointer to BaseAddress of the Array
               Register Offset
               Pointer to variable in which to store the Read Value

Return Value : void
****************************************************************************/
__inline void clkrv_readReg(STSYS_DU32 *Base, U32 Reg, U32 *Value)
{
    volatile U32 *Regaddr;

    Regaddr = (U32 *)((U32)Base + Reg);
    *Value = *Regaddr;
}

#if defined (WA_GNBvd44290) || defined (WA_GNBvd54182) || defined (WA_GNBvd56512)
void clkrv_WA_GNBvd44290( U32 * CountHD_p, U32 * CountPCM_p )
{
    *CountHD_p  = Count_PixelHD;
    *CountPCM_p = Count_PCM;
}
#endif /* WA_GNBvd44290 || WA_GNBvd54182 || WA_GNBvd56512*/

#endif /* defined(ST_OS20) || defined(ST_OS21) */

/***************************** End of Utility Functions ********************/

/* End of clkrvfilter.c */
