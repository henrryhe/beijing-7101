/*****************************************************************************
File Name   : stpwm.h

Description : API Interfaces for the PWM driver.

Copyright (C) 2005 STMicroelectronics

Reference   :

$ClearCase (VOB: stpwm)

ST API Definition "PWM (Driver) API" DVD-API-10 Revision 1.5
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STPWM_H
#define __STPWM_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stpio.h"        /* Needed for STPWM_C4InitParams_t */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */
/* Driver revision number */
#define  STPWM_REVISION 	"STPWM-REL_2.0.0"

typedef enum STPWM_Device_e
{
    STPWM_C4
} STPWM_Device_t;

typedef enum STPWM_Mode_e
{
    STPWM_Timer,
    STPWM_Capture0,
    STPWM_Capture1,
    STPWM_Compare0,
    STPWM_Compare1
} STPWM_Mode_t;

/* Input capture*/
typedef enum STPWM_EdgeControl_e
{
    STPWM_Disabled = 0,
    STPWM_RisingEdge,
    STPWM_FallingEdge,
    STPWM_RisingOrFallingEdge
} STPWM_EdgeControl_t;

typedef struct STPWM_CallbackParams_e
{
   void *CallbackData_p;
   void (*CallBack)(void*);
}STPWM_CallbackParam_t;

typedef U32 STPWM_Handle_t;

typedef struct STPWM_C4InitParams_s
{
    U32            		 *BaseAddress;
    STPIO_PIOBit_t 		 PIOforPWM0;
    STPIO_PIOBit_t 		 PIOforPWM1;
    STPIO_PIOBit_t 		 PIOforPWM2;
#ifdef ST_OS21
    interrupt_name_t     InterruptNumber;
#else
    U32                  InterruptNumber;
#endif
    U32                  InterruptLevel;
} STPWM_C4InitParams_t;

typedef struct STPWM_C4OpenParams_s
{
    U32 ChannelNumber;
    U32 PrescaleFactor;
} STPWM_C4OpenParams_t;

typedef struct STPWM_InitParams_s
{
    STPWM_Device_t       Device;
    STPWM_C4InitParams_t C4;
} STPWM_InitParams_t;

typedef struct STPWM_TermParams_s
{
    BOOL ForceTerminate;
} STPWM_TermParams_t;

typedef struct STPWM_OpenParams_s
{
    STPWM_C4OpenParams_t      C4;
    STPWM_Mode_t			  PWMUsedFor;
} STPWM_OpenParams_t;
  
                                       
/* Exported Constants ----------------------------------------------------- */

#define STPWM_DRIVER_ID        10
#define STPWM_DRIVER_BASE      (STPWM_DRIVER_ID << 16)

/* External return codes */
#define STPWM_ERROR_BASE       STPWM_DRIVER_BASE

#define STPWM_ERROR_PIO        (STPWM_ERROR_BASE + 1)

/* Others */
#define STPWM_CHANNEL_CNT      3    /* number of PWM channels */

#define STPWM_CHANNEL_0        0
#define STPWM_CHANNEL_1        1
#define STPWM_CHANNEL_2        2

#if defined(ST_7710)

#define STPWM_PRESCALE_MIN               1
#define STPWM_PRESCALE_MAX               65536
#define STPWM_CPTCMP_PRESCALE_MIN 		 1
#define STPWM_CPTCMP_PRESCALE_MAX        32
#define STPWM_COMPARE_VAL_MIN            0
#define STPWM_COMPARE_VAL_MAX            (U32)-1    
#define STPWM_CAPTURE_VAL_MIN            0
#define STPWM_CAPTURE_VAL_MAX            (U32)-1    
#define STPWM_CMPCPTCOUNTER_VAL_MIN      0
#define STPWM_CMPCPTCOUNTER_VAL_MAX      (U32)-1  

#elif defined(ST_5525) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define STPWM_PRESCALE_MIN       		 1
#define STPWM_PRESCALE_MAX        		 256
#define STPWM_CPTCMP_PRESCALE_MIN 		 1
#define STPWM_CPTCMP_PRESCALE_MAX        32
#define STPWM_COMPARE_VAL_MIN            0
#define STPWM_COMPARE_VAL_MAX            (U32)-1    
#define STPWM_CAPTURE_VAL_MIN            0
#define STPWM_CAPTURE_VAL_MAX            (U32)-1    
#define STPWM_CMPCPTCOUNTER_VAL_MIN      0
#define STPWM_CMPCPTCOUNTER_VAL_MAX      (U32)-1    

#elif defined(ST_5100)

#define STPWM_PRESCALE_MIN    			 1
#define STPWM_PRESCALE_MAX   		     16
#define STPWM_CPTCMP_PRESCALE_MIN 		 0
#define STPWM_CPTCMP_PRESCALE_MAX        0
#define STPWM_COMPARE_VAL_MIN            0
#define STPWM_COMPARE_VAL_MAX            0    
#define STPWM_CAPTURE_VAL_MIN            0
#define STPWM_CAPTURE_VAL_MAX            0    
#define STPWM_CMPCPTCOUNTER_VAL_MIN      0
#define STPWM_CMPCPTCOUNTER_VAL_MAX      0

#else
#define STPWM_PRESCALE_MIN     1
#define STPWM_PRESCALE_MAX     16
#endif

#if defined(ST_5100) || defined(ST_8010)
/*16 bit PWM_nVAL Register*/
#define STPWM_MARK_MIN         1
#define STPWM_MARK_MAX         65536
#else
#define STPWM_MARK_MIN         1
#define STPWM_MARK_MAX         256
#endif

#define STPWM_PULSES_PER_CYCLE 256

#define STPWM_CHAN_NO_MIN          STPWM_CHANNEL_0
#define STPWM_CHAN_NO_MAX          STPWM_CHANNEL_2
#define STPWM_CHAN_NO_CPT_MAX      STPWM_CHANNEL_1


/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_Revision_t STPWM_GetRevision( void );

ST_ErrorCode_t STPWM_Init( const ST_DeviceName_t    Name,
                           const STPWM_InitParams_t *InitParams );

ST_ErrorCode_t STPWM_Open( ST_DeviceName_t    Name,
                           STPWM_OpenParams_t *OpenParams,
                           STPWM_Handle_t     *Handle );

ST_ErrorCode_t STPWM_CounterControl(STPWM_Handle_t   Handle,
                                    STPWM_Mode_t     PWMUsedFor,
                                    BOOL             CounterEnable);   
ST_ErrorCode_t STPWM_InterruptControl(STPWM_Handle_t Handle,
                                      STPWM_Mode_t   PWMUsedFor,
                                      BOOL           InterruptEnable);                                                              
/* For using PWM timer output*/
ST_ErrorCode_t STPWM_SetRatio( STPWM_Handle_t Handle,
                               U32            MarkValue );  

ST_ErrorCode_t STPWM_GetRatio( STPWM_Handle_t Handle,
                               U32            *MarkValue );
                               
/*For using PWM Output Compare for generating interrupts with varied time interval*/                             
ST_ErrorCode_t STPWM_SetCompareValue(STPWM_Handle_t Handle,
                                       U32          CompareValue);   
ST_ErrorCode_t STPWM_GetCompareValue(STPWM_Handle_t Handle,
                                       U32          *CompareValue);  
                                       
/*Input capture For any input signal detection*/                                       
ST_ErrorCode_t STPWM_SetCaptureEdge(STPWM_Handle_t          Handle,
                                    STPWM_EdgeControl_t     EdgeControl); 
ST_ErrorCode_t STPWM_GetCaptureEdge(STPWM_Handle_t          Handle,
                                    STPWM_EdgeControl_t     *EdgeControl);  
ST_ErrorCode_t STPWM_GetCaptureValue(STPWM_Handle_t         Handle,
                                     U32                    *CaptureValue);
                                                                        
/*Set and Retrieves the Compare counter register value*/                                       
ST_ErrorCode_t STPWM_SetCMPCPTCounterValue(STPWM_Handle_t  Handle,
                                           U32             Value);
ST_ErrorCode_t STPWM_GetCMPCPTCounterValue(STPWM_Handle_t  Handle,
                                           U32             *Value); 

ST_ErrorCode_t STPWM_SetCallback(STPWM_Handle_t  		 Handle,
                                 STPWM_CallbackParam_t   *CallbackParams,
                                 STPWM_Mode_t            PWMUsedFor);
                                                                                                                                                                                                        
ST_ErrorCode_t STPWM_Close( STPWM_Handle_t Handle );

ST_ErrorCode_t STPWM_Term( const ST_DeviceName_t DeviceName,
                           const STPWM_TermParams_t *Params );
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STPWM_H */

/* End of stpwm.h */

