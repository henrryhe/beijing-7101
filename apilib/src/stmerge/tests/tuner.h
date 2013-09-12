/*****************************************************************************

File Name: tuner.h

Description: TUNER header

Copyright (C) 2003 STMicroelectronics

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __TUNER_H
#define __TUNER_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"    /* STAPI includes */
#include "sttuner.h"
#include "sti2c.h"

/* Exported Types ------------------------------------------------------ */

typedef enum
{
    BAND_US = 0,
    BAND_EU_LO,
    BAND_EU_HI,
    BAND_TER,
    BAND_CABLE,
    MAX_BANDS
} TUNER_BandNum_t;

typedef enum
{
    TUNER_UNKNOWN,
    TUNER_SAT_299_EVAL,
    TUNER_SAT_399_EVAL,
    TUNER_SAT_299_STEM,
    TUNER_TER_360,
    TUNER_CAB_297
}
TUNER_Type_t;

typedef enum SERVICE_Mode_e
{
    SERVICE_MODE_NONE,
    SERVICE_MODE_DVB,
    SERVICE_MODE_DTV
} SERVICE_Mode_t;

typedef enum SERVICE_Display_e
{
    SERVICE_DISPLAY_NONE,
    SERVICE_DISPLAY_PAL,
    SERVICE_DISPLAY_NTSC
} SERVICE_Display_t;


enum
{
    I2C_BACK_BUS,
    I2C_FRONT_BUS,
    I2C_DEVICES
};

/* Exported Constants -------------------------------------------------- */

#define MAX_SCANS       4
#define MAX_THRESHOLDS  5

/* Exported Variables -------------------------------------------------- */

extern STTUNER_Handle_t   TUNER_Handle;
extern ST_DeviceName_t    TUNER_DeviceName;

extern STTUNER_Band_t     TUNER_Bands[MAX_BANDS];
extern STTUNER_BandList_t TUNER_BandList;

extern STTUNER_Scan_t     TUNER_Scans[MAX_SCANS];
extern STTUNER_ScanList_t TUNER_ScanList;

extern ST_ErrorCode_t     InitialiseTestEnvironment(ST_DeviceName_t STPTI_EventHandlerName);

/* Exported Macros ----------------------------------------------------- */

/* Exported Functions -------------------------------------------------- */

ST_ErrorCode_t TUNER_Setup(SERVICE_Mode_t SERVICE_Mode);

ST_ErrorCode_t TUNER_Quit(void);

ST_ErrorCode_t TUNER_SetFrequency(U32 Frequency, STTUNER_Scan_t *STTUNER_Scan_p);

/* Support files */
ST_ErrorCode_t TUNER_WaitandDisplay( void );
void TUNER_WaitforNotifyCallback(STTUNER_EventType_t *EventType_p);
void TUNER_ReportInfo(char *PrefixStr, STTUNER_TunerInfo_t *STTUNER_TunerInfo_p);

char *EventToStr   (STTUNER_EventType_t STTUNER_EventType);
char *FecToStr     (STTUNER_FECRate_t STTUNER_FECRate);
char *LNBToneToStr (STTUNER_LNBToneState_t STTUNER_LNBToneState);
char *ModToStr     (STTUNER_Modulation_t STTUNER_Modulation);
char *PolarityToStr(STTUNER_Polarization_t STTUNER_Polarization);
char *StatusToStr  (STTUNER_Status_t STTUNER_Status);

#endif /* __TUNER_H */

/* EOF --------------------------------------------------------------------- */
