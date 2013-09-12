/*******************************************************************************

File name   : stccinit.h

Description : stcc init manager header file

COPYRIGHT (C) STMicroelectronics 2001.

Date               Modification                                     Name
----               ------------                                     ----
10 July 2001        Created                                  Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STCC_INIT_H
#define __STCC_INIT_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#ifdef ST_OS20
#include "task.h"
#endif
#ifdef ST_OS21
#include "os21/task.h"
#endif

#if !defined (ST_OSLINUX)
#include "sttbx.h"
#endif

#include "stos.h"

#include "stcc.h"
#include "stvbi.h"
#include "stevt.h"
#include "stvtg.h"
#include "stvid.h"
#ifdef IN_DEV
#include "dtv_cc/dtv_cc.h"
#endif

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* functional params : */
#ifdef ST_OS21
#define STCC_TASK_STACK_SIZE        1024*16
#endif
#ifdef ST_OS20
#define STCC_TASK_STACK_SIZE        3000
#endif
#ifdef ST_OSLINUX
#define STCC_TASK_STACK_SIZE        1024*16
#endif

#define NB_SLOT               10 /* max slot = cc extracted from a copy     */
#define NB_COPIES             4  /* max user data copies bufferised         */
#define COPY_SIZE             300/* max size of a user data copy            */
#define NB_CCDATA             64 /* max 2bytes cc in a slot                 */

#define STCC_MAX_DEVICE       3  /* Max number of Init() allowed            */
#define STCC_MAX_OPEN         1  /* Max num of Open() allowed per Init()    */
#define STCC_MAX_UNIT         (STCC_MAX_OPEN * STCC_MAX_DEVICE)

/* driver defines */
#define BAD_SLOT              0xffff
#define STCC_VALID_UNIT       0x13063061

#define STCC_EMPTY      0
#define STCC_IN_FILL    1
#define STCC_FILLED     2
#define STCC_IN_READ    3

/* Exported Macros ---------------------------------------------------------- */

#define CC_Defense(Error) STTBX_Report((STTBX_REPORT_LEVEL_INFO, \
        "STCC : A function returned an error num 0x%x",Error))

#define CC_Trace(Msg) STTBX_Report( (STTBX_REPORT_LEVEL_INFO,Msg) )

#define IsValidHandle(Handle) ((((stcc_Unit_t *) (Handle)) >= stcc_UnitArray) \
          &&  (((stcc_Unit_t *) (Handle)) < (stcc_UnitArray + STCC_MAX_UNIT)) \
          &&  (((stcc_Unit_t *) (Handle))->UnitValidity == STCC_VALID_UNIT))

/* Exported Types ----------------------------------------------------------- */

typedef struct
{
    U32         Field;
    U8          Byte1;
    U8          Byte2;
}stcc_Data_t;

typedef struct
{
    U32         SlotAvailable;
    STVID_PTS_t PTS;
    U32         Previous;
    U32         Next;
    U32         DataLength;
    stcc_Data_t CC_Data[NB_CCDATA];
}stcc_Slot_t;

typedef struct {

#ifdef ST_OSLINUX
#ifdef MODULE
    struct semaphore    TaskSem;       /* End of task synchronisation */
#endif
    task_t            * Task_p;
#else
    task_t  * Task_p;
#endif
    /*tdesc_t TaskDesc;*/
    U8      Stack[STCC_TASK_STACK_SIZE];
    BOOL    IsRunning;
    BOOL    ToBeDeleted;
    semaphore_t* SemProcessCC_p;      /* To synchronise orders in the task */

} stcc_Task_t;

typedef struct
{
    U32         Used;
    U32         Length;
    U8          Data[COPY_SIZE];
    STVID_PTS_t PTS;
}UserDataCopy_t;

typedef struct
{
    STCC_InitParams_t   InitParams;
    STEVT_Handle_t      EVTHandle;
    STVBI_Handle_t      VBIHandle;
    stcc_Task_t         Task;
    STCC_FormatMode_t   FormatMode;
    STCC_OutputMode_t   OutputMode;
    BOOL                LastPacket708;
    BOOL                CCDecodeStarted;
    BOOL                SchedByVsync;
    BOOL                SchedByUserData;
    BOOL                SchedBySequencer;
    U32                 SequencerCount;
    semaphore_t*         SemAccess_p;      /* To synchronise orders in the task */
    stcc_Slot_t         CaptionSlot[NB_SLOT];
    UserDataCopy_t      Copies[NB_COPIES];
    STEVT_EventID_t     Notify_id;
    U32                 FirstSlot;
    U32                 NbUsedSlot;
    /* dtv decoder context : */
#ifdef IN_DEV
    DTVContext_t *      DTVContext_p;
#endif
} stcc_DeviceData_t;


typedef struct
{
    ST_DeviceName_t     DeviceName;
    stcc_DeviceData_t * DeviceData_p;
} stcc_Device_t;

typedef struct
{
    stcc_Device_t* Device_p;
    STCC_Handle_t Handle;
    U32 UnitValidity;
} stcc_Unit_t;

typedef struct
{
   U32 NbUserDataReceived;
   U32 NbUserDataParsed;
   U32 NBCaptionDataFound;
   U32 NBCaptionDataPresented;
}STCC_DebugStats_t;

/* Exported Var ----------------------------------------------------------- */

#ifdef CC_RESERV
#define VARIABLE /* not extern */
#else
#define VARIABLE extern
#endif

VARIABLE stcc_Unit_t       stcc_UnitArray[STCC_MAX_UNIT];
VARIABLE STCC_DebugStats_t STCC_DebugStats;

#undef CC_RESERV
#undef VARIABLE

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STCC_INIT_H */

/* End of stccinit.h */

