/*****************************************************************************

File Name  : lists.h

Description: export transponder lists.

Copyright (C) 2003 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __LISTS_H
#define __LISTS_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"    /* STAPI includes */
#include "sttuner.h"
#include "tuner.h" 


#define MAX_NUM_CHANNELS  40

/* Exported Types ------------------------------------------------------ */

typedef enum LISTS_StreamType_e
{
    NOPID   = -1,
    MP1V    =  1,
    MP2V,
    MP1A,
    MP2A,
    PRIVATE,
    TTXT,       /* TTXT & SUBT use same stream type (added to distinguish from TTXT) */
    SUBT,  
    PCR,
    DSMCC,
    AC3     = 0x81,
    OTHER   = 0xff
} LISTS_StreamType_t;

typedef struct LISTS_Transponder_s
{
    int                     Index;
    SERVICE_Mode_t          Mode;           /* tuner mode (DVB/DTV) */

    U32                     Frequency;      /* transponder info */
    STTUNER_Polarization_t  Polarization;
    STTUNER_FECRate_t       FECRates;
    STTUNER_Modulation_t    Modulation;
    STTUNER_LNBToneState_t  LNBToneState;
    U32                     FrequencyBand;
    U32                     AGC;
    U32                     SymbolRate;
    char                   *InfoString;    /* transponder/satellite info */
} LISTS_Transponder_t;


typedef struct LISTS_Channel_s
{
    int                     Index;
    int                     Transponder;    /* Index into transponder list */
    SERVICE_Display_t       Display;        /* display mode (PAL/NTSC) */

    U16                     VideoPid;       /* channel pid information */
    U16                     AudioPid;
    U16                     PcrPid;
    U16                     TtxPid;
    U16                     SubtPid;

    LISTS_StreamType_t      AudioType;

    char                   *ChannelName;    /* channel name to display */
} LISTS_Channel_t;

/* Exported Constants -------------------------------------------------- */

/* Exported Variables -------------------------------------------------- */

extern LISTS_Transponder_t LISTS_TransponderList[];
extern LISTS_Channel_t     LISTS_ChannelList[];
#if defined (DUAL_DECODE) || defined (PIP_SUPPORT)
    extern S32                 LISTS_ChannelSelection[][40];
#else
    extern S32                 LISTS_ChannelSelection[];
#endif


/* Exported Macros ----------------------------------------------------- */
/* shorthand */
#define TONE_OFF    STTUNER_LNB_TONE_OFF
#define TONE_ON     STTUNER_LNB_TONE_22KHZ
#define PLR_VERT    STTUNER_PLR_VERTICAL
#define PLR_HORZ    STTUNER_PLR_HORIZONTAL
#define FEC_NONE    STTUNER_FEC_NONE
#define FEC_2_3     STTUNER_FEC_2_3
#define FEC_3_4     STTUNER_FEC_3_4
#define FEC_5_6     STTUNER_FEC_5_6
#define FEC_6_7     STTUNER_FEC_6_7
#define FEC_7_8     STTUNER_FEC_7_8
#define QPSK        STTUNER_MOD_QPSK
#define QAM64       STTUNER_MOD_64QAM
#define AGC_ON      STTUNER_AGC_ENABLE

#define MODE_NONE   SERVICE_MODE_NONE
#define MODE_DVB    SERVICE_MODE_DVB
#define MODE_DTV    SERVICE_MODE_DTV

#define DISP_NONE   SERVICE_DISPLAY_NONE
#define DISP_PAL    SERVICE_DISPLAY_PAL
#define DISP_NTSC   SERVICE_DISPLAY_NTSC
#define ENDOFLIST   -1

/* Exported Functions -------------------------------------------------- */


#endif /* __LISTS_H */

/* EOF --------------------------------------------------------------------- */
