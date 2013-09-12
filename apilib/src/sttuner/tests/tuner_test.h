/*----------------------------------------------------------------------------
File Name: tuner_test.h

Description: Common STTUNER test harness header file.

Copyright (C) 1999-2001 STMicroelectronics

Reference:

ST API Definition "TUNER Driver API" DVD-API-06
  ----------------------------------------------------------------------------*/

#ifndef __TUNER_TEST_H
#define __TUNER_TEST_H

/*----------------------------------------------------------------------------
          Prototypes for testing STTUNER driver's internal routines:
  ----------------------------------------------------------------------------*/


/* Pass/fail counts */
typedef struct
{
    U32 NumberPassed;
    U32 NumberFailed;
} TUNER_TestResult_t;


/* Error messages */
typedef struct
{
    ST_ErrorCode_t Error;
    char ErrorMsg[128];
} TUNER_ErrorMessage;


/* Test parameters -- passed to every test routine */
typedef struct
{
    U32 Ref;
    U32 Tuner;
} TUNER_TestParams_t;


/* Defines a test harness function */
struct TUNER_TestEntry_s
{
    TUNER_TestResult_t(*TestFunction) (TUNER_TestParams_t *);
    char TestInfo[120];
    U32 RepeatCount;
};


typedef struct TUNER_TestEntry_s TUNER_TestEntry_t;


/* types for ioctl operations */

typedef struct
{
    int RegIndex;
    U32 Value;
} SETREG_InParams_t;  /* see sioctl.h (sat driver ioctl types) for SATIOCTL_SETREG_InParams_t */


typedef enum
{
    TEST_IOARCH_READ = 0,
    TEST_IOARCH_WRITE = 1,
    TEST_IOARCH_SA_READ = 2,
    TEST_IOARCH_SA_WRITE = 3
} TEST_IOARCH_Operation_t;


typedef struct
{
    TEST_IOARCH_Operation_t Operation;
    U16 SubAddr;
    U8 *Data;
    U32 TransferSize;
    U32 Timeout;
} TEST_IOARCH_Params_t;   /* see sioctl.h (also ioarch.h) */


typedef struct
{
    STTUNER_IODriver_t Driver;
    ST_DeviceName_t DriverName;
    U32 Address;
    U32 SubAddress;
    U32 Value;
    U32 XferSize;
    U32 TimeOut;
} TEST_ProbeParams_t;

typedef enum
 {
  LNB_IOCTL_PROTECTION_NOCHANGE = 0 ,	
  LNB_IOCTL_PROTECTION_STATIC,
  LNB_IOCTL_PROTECTION_DYNAMIC 	
 }STTUNER_TEST_IOCTL_LNBShortCircuitProtectionMode_t;
 
 typedef enum
{
 LNB_IOCTL_POWERBLCOKS_NOCHANGE = 0,
 LNB_IOCTL_POWERBLCOKS_ENABLED ,
 LNB_IOCTL_POWERBLCOKS_DISABLED 
}STTUNER_TEST_IOCTL_PowerControl_t ;
typedef enum
{
 LNB_IOCTL_TTXMODE_NOCHANGE = 0,
 LNB_IOCTL_TTXMODE_RX,
 LNB_IOCTL_TTXMODE_TX  
}STTUNER__TEST_IOCTL_TTX_Mode_t;

typedef enum
{
 LNB_IOCTL_LLCMODE_NOCHANGE = 0,
 LNB_IOCTL_LLCMODE_ENABLED,
 LNB_IOCTL_LLCMODE_DISABLED  
}STTUNER_TEST_IOCTL_LLC_Mode_t;

typedef struct
{
 STTUNER__TEST_IOCTL_TTX_Mode_t                 TTX_Mode;
 STTUNER_TEST_IOCTL_PowerControl_t             PowerControl;
 STTUNER_TEST_IOCTL_LNBShortCircuitProtectionMode_t ShortCircuitProtectionMode;
 STTUNER_TEST_IOCTL_LLC_Mode_t     LLC_Mode;
}TEST_LNB_IOCTL_Config_t;

typedef struct
{
    int           RegIndex;
    unsigned char Value;
} TERIOCTL_SETREG_InParams_t;

typedef struct
    {
    	U8	RPLLDIV;
    	U8      TRLNORMRATELSB; 
	U8	TRLNORMRATELO; 
	U8	TRLNORMRATEHI; 
	U8 	INCDEROT1;
	U8 	INCDEROT2;
	 
    	
    	}STTUNER_demod_IOCTL_30MHZ_REG_t  ;
 typedef struct
    {
    	
    	U8      TRLNORMRATELSB; 
	U8	TRLNORMRATELO; 
	U8	TRLNORMRATEHI; 
	    	
    	}STTUNER_demod_TRL_IOCTL_t  ;
/* Debug message prefix string (always appended with a ':') */
#define TUNER_DEBUG_PREFIX           "STTUNER"

#ifdef DISABLE_TOOLBOX
#define TUNER_DebugPrintf(args)      printf("%s(%08d): ", TUNER_DEBUG_PREFIX, (int)STOS_time_now()); printf args
#define TUNER_DebugError(msg,err)    printf("%s(%08d): %s = %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(),msg, TUNER_ErrorString(err))
#define TUNER_DebugMessage(msg)      printf("%s(%08d): %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(), msg)
#else
#ifdef ST_OS21

#define TUNER_DebugPrintf(args)      STTBX_Print(("%s(%08d): ", TUNER_DEBUG_PREFIX, (int)STOS_time_now())); STTBX_Print(args)
#define TUNER_DebugError(msg,err)    STTBX_Print(("%s(%08d): %s = %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(),msg, TUNER_ErrorString(err)))
#define TUNER_DebugMessage(msg)      STTBX_Print(("%s(%08d): %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(), msg))
#else

#define TUNER_DebugPrintf(args)      STTBX_Print(("%s(%08d): ", TUNER_DEBUG_PREFIX, (int)STOS_time_now())); STTBX_Print(args)
#define TUNER_DebugError(msg,err)    STTBX_Print(("%s(%08d): %s = %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(),msg, TUNER_ErrorString(err)))
#define TUNER_DebugMessage(msg)      STTBX_Print(("%s(%08d): %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(), msg))
#endif
#endif 

char *TUNER_ErrorString(ST_ErrorCode_t Error);

/* Test success indicator */
#define TUNER_TestPassed(x) x.NumberPassed++; TUNER_DebugPrintf(("Result: **** PASS ****\n"))
#define TUNER_TestFailed(x,reason) x.NumberFailed++; TUNER_DebugPrintf(("Result: !!!! FAIL !!!! (%s)\n", reason))

/* Zero out test result structure */
#define TEST_RESULT_ZERO    {0, 0}
extern  ST_ErrorCode_t STTUNER_PIOHandleInInstDbase(U32 index,U32 HandlePIO);

#endif /* __TUNER_TEST_H */

/* End of header */
