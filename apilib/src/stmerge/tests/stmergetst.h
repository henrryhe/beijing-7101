/****************************************************************************

File Name   : stmergetst.h

Description : Test harness header file

Copyright (C) 2003, STMicroelectronics

References  :

****************************************************************************/

#ifndef __STMERGETST_H
#define __STMERGETST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */
#include "stlite.h"
#include "stdevice.h"
#include "stevt.h"
#include "stmerge.h"
#include "stmergeerr.h"

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#include "testtool.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Exported Constants ----------------------------------------------------- */
/* Memory partitions */


/* Exported Types --------------------------------------------------------- */
#define  EVENT_HANDLER_NAME   "Event\0"

#ifndef DEFAULT_SYNC_LOCK
#define DEFAULT_SYNC_LOCK 1
#define DEFAULT_SYNC_DROP 3
#endif

#define MAX_SESSIONS_PER_CELL   3

#ifndef MAX_INJECTION_TIME
#define MAX_INJECTION_TIME      200
#endif

#define STMERGE_IGNORE_ID     0xFFFF

#ifdef ST_OS21
#define TASK_SIZE 20*1024
#else
#define TASK_SIZE 4*1024
#endif

#define MAX_PTI 3
/* Injection task structure */
typedef struct STMERGE_TaskParams_s
{
    U32 NoInterfacesSet;
    ST_DeviceName_t DeviceName[MAX_PTI];
    ST_DeviceName_t InputInterface[MAX_PTI];
    STPTI_Slot_t SlotHandle[MAX_PTI];
    void  *InjectTerm_p;
} STMERGE_TaskParams_t;


void Injection_Status(STMERGE_TaskParams_t *Params_p);

#if defined(STMERGE_INTERRUPT_SUPPORT)
#if defined(ST_7710)
#define INTERRUPT_NUM    55
#define INTERRUPT_LEVEL  1
#elif defined(ST_7100) || defined(ST_7109)
#define INTERRUPT_NUM   TSMERGE_INTERRUPT
#define INTERRUPT_LEVEL  2
#endif
#endif

/* Number of i/ps & o/ps available */
#if defined(ST_5528)
#define NUMBER_OF_TSIN          4 /* TSIN 0-3 */
#define NUMBER_OF_SWTS          2
#define NUMBER_OF_PTI           2
#elif defined(ST_7109)
#define NUMBER_OF_TSIN          3 
#define NUMBER_OF_SWTS          3
#define NUMBER_OF_PTI           2
#elif defined(ST_5525)
#define NUMBER_OF_TSIN          4 
#define NUMBER_OF_SWTS          4
#define NUMBER_OF_PTI           2
#else /* 5100,7710 etc */
#define NUMBER_OF_TSIN          3 /* but only TSIN0 & TSIN1 are connected */
#define NUMBER_OF_SWTS          1
#define NUMBER_OF_PTI           1
#endif

#if defined(ST_5100) || defined(ST_5301) || defined(ST_7710)
#define TS_EPLD_REG          0x41800000
#elif defined(ST_7100) || defined(ST_7109) /* TBC for 7109 */
#define TS_EPLD_REG          0xA3800000
#endif

typedef struct stmergetst_TestResult_s
{
    U32  Passed;
    U32  Failed;
}stmergetst_TestResult_t;

typedef volatile U32 device_word_t;

#ifndef ST_OS21
extern ST_Partition_t *InternalPartition;
extern ST_Partition_t *NcachePartition;
extern ST_Partition_t *SystemPartition;
extern ST_Partition_t *DriverPartition;
#else
extern ST_Partition_t *system_partition;
extern ST_Partition_t *DriverPartition;
extern ST_Partition_t *NcachePartition;
#endif

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions  ---------------------------------------------------- */
BOOL stmerge_ErrantTest(parse_t *pars_p, char *result_sym_p);
BOOL stmerge_LeakTest(parse_t * pars_p, char *result_sym_p);
BOOL stmerge_NormalTest(parse_t * pars_p, char *result_sym_p);
BOOL stmerge_Merge_2_1_Test(parse_t * pars_p, char *result_sym_p);
BOOL stmerge_Merge_3_1_Test(parse_t * pars_p, char *result_sym_p);
BOOL stmerge_ByPass(parse_t * pars_p, char *result_sym_p);
BOOL stmerge_Runall(parse_t * pars_p, char *result_sym_p);
BOOL stmerge_AltoutTest(parse_t * pars_p, char *result_sym_p);
#ifdef STMERGE_CHANNEL_CHANGE
BOOL stmerge_ChannelChangeTest(parse_t * pars_p, char *result_sym_p);
#endif
#ifdef STMERGE_INTERRUPT_SUPPORT
BOOL stmerge_RAMOverflowIntTest(parse_t * pars_p, char *result_sym_p);
#endif
#ifdef STMERGE_1394_PRODUCER_CONSUMER
BOOL stmerge_1394_Producer_Consumer(parse_t * pars_p, char *result_sym_p);
#endif
#ifdef STMERGE_DVBCI_TEST
BOOL stmerge_DVBCITest(parse_t * pars_p, char *result_sym_p);
#endif
#if defined(ST_7100) || defined(ST_7109)
BOOL stmerge_ConfigTest(parse_t * pars_p, char *result_sym_p);
#endif

BOOL stmerge_DuplicateInputTest(parse_t * pars_p, char *result_sym_p);

BOOL stmerge_DisconnectTest(parse_t * pars_p, char *result_sym_p);
void stmergetst_SetSuccessState(BOOL Value);
BOOL stmergetst_IsTestSuccessful(void);
void stmergetst_ClearTestCount(void);

void stmergetst_TestConclusion(char *Msg);
void stmergetst_ErrorReport( ST_ErrorCode_t *ErrorStore, ST_ErrorCode_t ErrorGiven,
                           ST_ErrorCode_t ExpectedErr);

#ifndef ST_OS21
partition_t *stmergetst_GetInternalPartition(void);
partition_t *stmergetst_GetNCachePartition(void);
partition_t *stmergetst_GetSystemPartition(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STMERGETST_H */
