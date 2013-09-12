/*****************************************************************************
File Name   : clkrvsim.h

Description :

Copyright (C) 2004 STMicroelectronics

Reference   :

$ClearCase (VOB: stclkrv)

ST API Definition "Clock Recovery API"
*****************************************************************************/


/* Define to prevent recursive inclusion */
#ifndef __CLKRVSIM_H
#define __CLKRVSIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"


/* Exported Constants ------------------------------------------------------ */
/* Default Values for various Tests */

#define PCM_CLOCK_FREQ1                 2048000
#define PCM_CLOCK_FREQ2                 2822400
#define PCM_CLOCK_FREQ3                 3072000
#define PCM_CLOCK_FREQ4                 4096000
#define PCM_CLOCK_FREQ5                 5644800
#define PCM_CLOCK_FREQ6                 6144000
#define PCM_CLOCK_FREQ7                 8192000
#define PCM_CLOCK_FREQ8                11289600
#define PCM_CLOCK_FREQ9                11300000
#define PCM_CLOCK_FREQ10               12288000
#define PCM_CLOCK_FREQ11               16384000
#define PCM_CLOCK_FREQ12               22579200
#define PCM_CLOCK_FREQ13               24576000
#define PCM_CLOCK_FREQ14               32768000
#define PCM_CLOCK_FREQ15               33868800
#define PCM_CLOCK_FREQ16               36864000



#define SPDIF_CLOCK_FREQ1              45158400
#define SPDIF_CLOCK_FREQ2              49152000
#define SPDIF_CLOCK_FREQ3              74250000
#define SPDIF_CLOCK_FREQ4             148500000




#define CLKRV_SLOT_1                1
#define CLKRV_SLOT_2                2

#define MASK_BIT_PACKET_DROP        0x01        /* 00000001 */
#define MASK_BIT_GENERATE_GLITCH    0x02        /* 00000010 */
#define MASK_BIT_BASE_CHANGE        0x04        /* 00000100 */

#define DIV_90KHZ_27MHZ             300
#define GLITCH_TICKS                400

#define DEFAULT_SD_FREQUENCY        27000000

/* Exported Types ------------------------------------------------------ */
typedef U32 CLKRVBaseAddress;

typedef struct
{
    FILE            *File_p;
    U32             PCR_Interval;
    U32             PCRDriftThreshold;
    BOOL            FileRead;
    char            *TestFile;

}STCLKRVSIM_StartParams_t;

typedef struct
{
    ST_DeviceName_t EVTDevNamPcr;
    U32             InterruptNumber;       /* clock recovery module HW interrupt number */
    ST_Partition_t  *SysPartition_p;       /* Memory partition to use (mi) */
    ST_Partition_t  *InternalPartition_p;  /* Memory partition to use (mi) */

}STCLKRVSIM_InitParams_t;


/*
* This data is not for debugging but for logging
* after each PCR is processed latest values of PCR, STC is logged.
*/

typedef struct
{
    U32 Code;
    U32 PCR;
    U32 STC;
    U32 SDFreq;
    U32 PCMFreq;
    U32 SPDIFreq;
    S32 AvgErrorInHz;
    S32 ErrorInHz;
    S32 WindowSum;
    U32 WindowCount;
    S32 ResidualErr;

}STCLKRV_Log_Data_t;

/* Exported Macros -----------------------------------------------------    */


#define PACKET_DROP (a)             ((a) & MASK_BIT_PACKET_DROP)
#define RESET_PACKET_DROP(a)        ((a) ^ MASK_BIT_PACKET_DROP)

#define GENERATE_GLITCH(a)          ((a) & MASK_BIT_GENERATE_GLITCH)
#define RESET_GENERATE_GLITCH(a)    ((a) ^ MASK_BIT_GENERATE_GLITCH)

#define BASE_CHANGE(a)              ((a) & MASK_BIT_BASE_CHANGE)
#define RESET_BASE_CHANGE(a)        ((a) ^ MASK_BIT_BASE_CHANGE)

/* Exported Variables -------------------------------------------------- */


/* Exported Functions -------------------------------------------------- */

ST_ErrorCode_t STCLKRVSIM_Init(STCLKRVSIM_InitParams_t *SimInitParams_p,CLKRVBaseAddress **BaseAddr_p);

ST_ErrorCode_t STCLKRVSIM_Term(void);

/* Start generating PCR,STC values */
ST_ErrorCode_t STCLKRVSIM_Start(STCLKRVSIM_StartParams_t* StartParams_p);

/* Stop generating PCR,STC values  */
void STCLKRVSIM_Stop(void);

ST_ErrorCode_t STCLKRVSIM_SetTestParameters(U32 Freq ,U32 Jitter ,U8 StreamStatus);
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef CLKRVSIM_H_  */


/* End of clkrvsim.h */
