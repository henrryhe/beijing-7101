/*******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name: stcommon.h

Interface to revision reporting and clock info functions

********************************************************************************/

#ifndef __STCOMMON_H
#define __STCOMMON_H

#include "stddefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STCOMMON_REVISION "STCOMMON-REL_2.1.1"
#define STCOMMON_GetRevision() ((ST_Revision_t) STCOMMON_REVISION)

#ifdef ST_OSLINUX
/* In user mode, it is up to us to define a base here. Let's set it to ms */
#define USER_MODE_TICK_VALUE 1000
#endif

typedef struct ST_ClockInfo_s
{
    U32     C2;
    U32     STBus;
    U32     CommsBlock;
    U32     HDDI;
    U32     VideoMem;
    U32     Video2;
    U32     Video3;
    U32     AudioDSP;
    U32     EMI;
    U32     MPX;
    U32     Flash;
    U32     SDRAM;
    U32     PCM;
    U32     SmartCard;
    U32     Aux;
    U32     HPTimer;
    U32     LPTimer;
    
    /* Added for 5528 */
    U32     ST40;
    U32     ST40Per;
    U32     UsbIrda;
    U32     PWM;
    U32     PCI;
    U32     VideoPipe;
    
    /*Added for ST2xx devices like 5301, 8010*/
    U32     ST200;
    
    /* Pixel clocks (27 MHz), audbit and SPDIF Formatter DSP clocks omitted.
      Also DVP clock since its a direct input pin */
} ST_ClockInfo_t;


U32 ST_ConvRevToNum( ST_Revision_t *ptRevision );
ST_ErrorCode_t ST_SetClockSpeed( U32 );
U32 ST_GetClockSpeed( void );
U32 ST_GetClocksPerSecond( void );
U32 ST_GetClocksPerSecondHigh( void );
U32 ST_GetClocksPerSecondLow( void );
U32 ST_GetMajorRevision( ST_Revision_t *ptRevision );
U32 ST_GetMinorRevision( ST_Revision_t *ptRevision );
U32 ST_GetPatchRevision( ST_Revision_t *ptRevision );
ST_ErrorCode_t ST_GetClockInfo (ST_ClockInfo_t *ClockInfo_p);
BOOL ST_AreStringsEqual(const char *DeviceName1_p, const char *DeviceName2_p);
U32 ST_GetCutRevision(void);


#if defined(DVD_USE_GLOBAL_PRIORITIES)
	#if defined(ST_OSLINUX)
		#include "linux_priorities.h"
	#elif defined(ST_OS21) || defined(ST_OS20)
		#include "os2x_priorities.h"
	#elif defined(ST_OSWINCE)
		#include "wince_priorities.h"
	#endif
#endif


#ifdef __cplusplus
}
#endif

#endif /* __STCOMMON_H */
