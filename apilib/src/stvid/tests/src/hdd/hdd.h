/*****************************************************************************

  File name   : hdd.h

  Description : Contains the high level calls of the hdd libray

  COPYRIGHT (C) ST-Microelectronics 2005.

  Date               Modification                 Name
  ----               ------------                 ----
  07/15/99           Created                      FR
  08/01/00           Adapted for STVID (STAPI)    AN
*****************************************************************************/
/* --- Define to prevent recursive inclusion ------------------------------ */
#ifndef __HDD_H
#define __HDD_H

#include "stvid.h"


/* Private Types ------------------------------------------------------------ */
typedef struct {
                            /*****   Task   feeding  management   *****/

BOOL                    ProcessRunning;             /* Flag that indicate that this process is running. */
BOOL                    EndOfLoopReached;           /* Flag to signal this process has reached its end. */

                            /*****   Data   feeding  management   *****/

S32                     NumberOfBlockToSkip;        /* Number of block of memory to skip (for very fast forward and backward).*/
S32                     LBAStart;                   /* Safe in memory of the start adress of the file. */
S32                     LBAEnd;                     /* Safe in memory of the end adress of the file.   */
S32                     LBANumber;                  /* Current LBA position read from HDD. */
S32                     NumberOfPlay;               /* Number of time, the file should be played (0 for infinity). */
BOOL                    InfinitePlaying;            /* Flag that indicate the playing has no limit. */

                            /***** Video driver events management *****/

task_t*                 TrickModeEventTask_p;       /* Task pointer to the process that manages trickmode events */
semaphore_t            *TrickModeDriverRequest_p;   /* Semaphore to signal en event occur */

BOOL                            UnderFlowEventOccured;              /* Flag that indicates the STVID_DATA_UNDERFLOW_EVT occurence*/
STVID_DataUnderflow_t           UnderflowEventData;                 /* Data associated with STVID_DATA_UNDERFLOW_EVT        */
BOOL                            SpeedDriftThresholdEventOccured;    /* Flag that indicates the STVID_SPEED_DRIFT_THRESHOLD_EVT occurence*/
STVID_SpeedDriftThreshold_t     SpeedDriftThresholdData;            /* Data associated with STVID_SPEED_DRIFT_THRESHOLD_EVT */

}   STHDD_Handle_t;

/* Private Constants -------------------------------------------------------- */
#ifdef ST_OS21
#define HDD_TASK_SIZE       16384
#endif
#ifdef ST_OS20
#define HDD_TASK_SIZE       4096
#endif

/* Define the priority level of the HDD task.
 * Notice: Change this level to a upper level may cause problem in case of use of
 * the STPTI module. With pti or link, no problem have been noticed. */
#define HDD_TASK_PRIORITY  0

/* Definition of the allocation area for a stream in the HDD. */
/* This means that the maximum size for a stream to be stored is 100 Mo */
#define MAX_SIZE_OF_HDD_STREAM 102400000
/* Equivalency  in term of LBA */
#define LBA_SIZE_OF_STREAM (MAX_SIZE_OF_HDD_STREAM/ATA_BLOCK_SIZE)

/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */

ST_ErrorCode_t  HDD_Init (void);
ST_ErrorCode_t  HDD_Term (void);
BOOL            HDD_CmdStart(void);

#endif /* HDD_H */


