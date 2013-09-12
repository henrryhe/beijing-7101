/****************************************************************************

File Name   : stmergeerr.h

Description : error file  header file

Copyright (C) 2003, STMicroelectronics

References  :
****************************************************************************/

#ifndef __STMERGEERR_H
#define __STMERGEERR_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes --------------------------------------------------------------- */
#include "stpti.h"
#include "testtool.h"

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

#define STMERGE_BASE_ADDRESS        TSMERGE_BASE_ADDRESS

#if defined(ST_5528)

#define PTIA_BASE_ADDRESS     ST5528_PTIA_BASE_ADDRESS
#define PTIB_BASE_ADDRESS     ST5528_PTIB_BASE_ADDRESS
#define PTIA_INTERRUPT        ST5528_PTIA_INTERRUPT
#define PTIB_INTERRUPT        ST5528_PTIB_INTERRUPT

#elif defined(ST_5100)

#define PTIA_BASE_ADDRESS     ST5100_PTI_BASE_ADDRESS
#define PTIB_BASE_ADDRESS     ST5100_PTI_BASE_ADDRESS /* there is only 1 PTI on 5100 */
#define PTIA_INTERRUPT        ST5100_PTI_INTERRUPT
#define PTIB_INTERRUPT        ST5100_PTI_INTERRUPT    /* there is only 1 PTI on 5100 */

#elif defined(ST_7710)

#define PTIA_BASE_ADDRESS     ST7710_PTI_BASE_ADDRESS
#define PTIB_BASE_ADDRESS     ST7710_PTI_BASE_ADDRESS /* there is only 1 PTI on 7710 */
#define PTIA_INTERRUPT        ST7710_PTI_INTERRUPT
#define PTIB_INTERRUPT        ST7710_PTI_INTERRUPT    /* there is only 1 PTI on 7710 */

#elif defined(ST_7100)

#define PTIA_BASE_ADDRESS     ST7100_PTI_BASE_ADDRESS
#define PTIB_BASE_ADDRESS     ST7100_PTI_BASE_ADDRESS /* there is only 1 PTI on 7100 */
#define PTIA_INTERRUPT        ST7100_PTI_INTERRUPT
#define PTIB_INTERRUPT        ST7100_PTI_INTERRUPT    /* there is only 1 PTI on 7100 */

#elif defined(ST_5301)

#define PTIA_BASE_ADDRESS     ST5301_PTI_BASE_ADDRESS
#define PTIB_BASE_ADDRESS     ST5301_PTI_BASE_ADDRESS /* there is only 1 PTI on 5301 */
#define PTIA_INTERRUPT        ST5301_PTI_INTERRUPT
#define PTIB_INTERRUPT        ST5301_PTI_INTERRUPT    /* there is only 1 PTI on 5301 */

#elif defined(ST_7109)

#define PTIA_BASE_ADDRESS     ST7109_PTIA_BASE_ADDRESS  
#define PTIB_BASE_ADDRESS     ST7109_PTIB_BASE_ADDRESS  
#ifdef PTIA_INTERRUPT
#undef PTIA_INTERRUPT
#define PTIA_INTERRUPT        /*ST7109_PTIA_INTERRUPT*/544 /*taken from cpu_7109.c TBR */
#endif
#ifdef PTIB_INTERRUPT
#undef PTIB_INTERRUPT
#define PTIB_INTERRUPT        /*ST7109_PTIB_INTERRUPT*/545 /*taken from cpu_7109.c TBR */
#endif
 

#elif defined(ST_5525) || defined(ST_5524)

#define PTIA_BASE_ADDRESS     ST5525_PTIA_BASE_ADDRESS
#define PTIB_BASE_ADDRESS     ST5525_PTIB_BASE_ADDRESS 
#define PTIA_INTERRUPT        ST5525_PTIA_INTERRUPT
#define PTIB_INTERRUPT        ST5525_PTIB_INTERRUPT    

#endif

#if defined(TSIN_PID)
    #undef TSIN_PID
#endif

#if defined(STMERGE_DTV_PACKET)
   /* The stream is sync01.bts . For this the SCID is 0x64 */
   #define TSIN_PID 0x64
#else
   /* The stream is mux188.trp . For this the PID is 0x280 */
   #define TSIN_PID 0x280
#endif
/* Exported Functions  ---------------------------------------------------- */

void CompareWithDefaultValues(STMERGE_ObjectConfig_t *DefaultConfig_TSIN,
                              STMERGE_ObjectConfig_t *TestData_TSIN,
                              STMERGE_ObjectConfig_t *DefaultConfig_SWTS,
                              STMERGE_ObjectConfig_t *TestData_SWTS);
void SetupDefaultInitparams( STPTI_InitParams_t *STPTI_InitParams );
STPTI_SupportedTCCodes_t SupportedTCCodes (void);
STPTI_DevicePtr_t GetBaseAddress(U32 DestinationId);
S32 GetInterruptNumber (U32 DestinationId);
ST_ErrorCode_t EVENT_Setup(ST_DeviceName_t EventHandlerName);
ST_ErrorCode_t EVENT_Term(ST_DeviceName_t EventHandlerName);
#if !defined(ST_OSLINUX)
typedef ST_ErrorCode_t (*TCLoader_t)(STPTI_DevicePtr_t, void *);
TCLoader_t SupportedLoaders (void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STMERGEERR_H */
