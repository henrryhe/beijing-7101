/*******************************************************************************

File name   : vin_init.h

Description : vin init header file (device relative definitions)

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
27 Sep 2000        Created                                           JA
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIN_INIT_H
#define __VIN_INIT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "halv_vin.h"

#include "stevt.h"
#include "stclkrv.h"
#include "stvid.h"

#include "vin_com.h"
#include "vin_ctcm.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */
#define CONTROLLER_COMMANDS_BUFFER_SIZE 50
#define INTERRUPTS_BUFFER_SIZE          10

/* No more than 128 commands: it needs to be casted to U8 */
typedef enum {
    CONTROLLER_COMMAND_TERMINATE,
    CONTROLLER_COMMAND_FLUSH,
    CONTROLLER_COMMAND_RESET,
    CONTROLLER_COMMAND_DATA_ERROR,
    CONTROLLER_COMMAND_STOP,
    NB_CONTROLLER_COMMANDS   /* Keep this one as the last one */
} ControllerCommandID_t;

/* Queue these commands to execute just after VSync*/
typedef enum {
    SYNC_SET_IO_WINDOW,
    SYNC_SET_SYNC_TYPE,
    SYNC_NB_CONTROLLER_COMMANDS   /* Keep this one as the last one */
} AfterVsyncCommandID_t;

typedef enum
{
    VIN_DEVICE_STATUS_STOPPED,
    VIN_DEVICE_STATUS_RUNNING
} VinDeviceStatus_t;

typedef enum {
    NONE                = 0x00,
    FRAME_OR_TOP_BUFFER = 0x01,
    BOT_BUFFER          = 0x02,
    BOTH_BUFFER         = FRAME_OR_TOP_BUFFER | BOT_BUFFER
} BufferStructureIdentity_t;

#define STVIN_MAX_NB_BUFFER_FOR_ANCILLARY_DATA  10
#define STVIN_MAX_NB_FRAME_IN_VIDEO             6

#define STVIN_MIN_BUFFER_TO_USE_DUMMY_BUFFER    4

/* Exported Types ----------------------------------------------------------- */

/* Parameters required to know everything about a buffer allocation (frame buffer or bit buffer) */
typedef struct
{
    U32     TotalSize;          /* Total size of the buffer in bytes.           */
    void *  Address_p;          /* Device address of buffer.                    */
    void *  VirtualAddress_p;   /* CPU address of Buffer.                       */
    STAVMEM_BlockHandle_t AvmemBlockHandle; /* Used if AllocationMode is SHARED_MEMORY */
} stvin_AncillaryBufferParams_t;


typedef struct
{
    ST_DeviceName_t DeviceName;
    ST_DeviceName_t VideoDeviceName;
    ST_DeviceName_t IntmrDeviceName;
    /* common informations for all open on a particular device */
    ST_Partition_t *CPUPartition_p;  /* Where the module allocated memory for its own usage,
                                   also where it will have to be freed at termination */
    U32                 MaxOpen;

    /* Dependency's */
    STVID_Handle_t      VideoHandle;
    STCLKRV_Handle_t    STClkrvHandle;
    STEVT_Handle_t      EventsHandle;
    STEVT_EventConstant_t InterruptEvent;

    /* Events */
    STEVT_EventID_t     RegisteredEventsID[STVIN_NB_REGISTERED_EVENTS_IDS];

    /* IT, BB @ ... */
    STVIN_DeviceType_t          DeviceType;             /* 7015 / 7020 / ST40GX1 / ... */
    STVIN_VideoParams_t         VideoParams;
    U32                         InputFrameRate;         /* Input Frame Rate according to InputMode */

    HALVIN_Handle_t     HALInputHandle;

    /* --------- NEW NEW NEW ----------- */

    VINCOM_Task_t       InputTask;

    /* Buffers of commands: 2 for each Diginput */
    U8 CommandsData[CONTROLLER_COMMANDS_BUFFER_SIZE];
    U8 VSyncCommandsData[INTERRUPTS_BUFFER_SIZE];
    U32 InterruptsData[INTERRUPTS_BUFFER_SIZE];

    CommandsBuffer_t    CommandsBuffer;                 /* Data concerning the input controller commands queue */
    CommandsBuffer_t    VSyncCommandsBuffer;            /* Data concerning the input controller commands queue with VSync*/
    CommandsBuffer32_t  InterruptsBuffer;               /* Data concerning the interrupts commands queue */

    VINCOM_Interrupt_t  VideoInterrupt;                 /* Data concerning the video interrupt */

    /* Ancillary Data Pointer */
    BOOL                AncillaryDataTableAllocated;    /* Flag to remember the status of table allocation.         */
    BOOL                AncillaryDataEventEnabled;      /* Flag to remember the ancillary data management status.   */
    BOOL                EnableAncillaryDatatEventAfterStart;
    stvin_AncillaryBufferParams_t AncillaryDataTable[STVIN_MAX_NB_BUFFER_FOR_ANCILLARY_DATA];
    U32                 AncillaryDataBufferNumber;      /* Number of ancillary data buffer.                         */
    U32                 AncillaryDataBufferSize;        /* Ancillary data buffer size.                              */
#ifdef WA_WRONG_CIRCULAR_DATA_WRITE
    BOOL                CircularLoopDone;                /* Detect if we are in the first loop */
#endif
    U32                 LastPositionInAncillaryDataBuffer;     /* Last reached data offset in the Ancillary Data buffer */

    /*   */
    STVID_PictureBufferDataParams_t     PictureBufferParams[STVIN_MAX_NB_FRAME_IN_VIDEO];
    STVID_PictureBufferHandle_t         PictureBufferHandle[STVIN_MAX_NB_FRAME_IN_VIDEO];
    STVID_PictureInfos_t                PictureInfos[STVIN_MAX_NB_FRAME_IN_VIDEO];
    U32                                 ExtendedTemporalReference[STVIN_MAX_NB_FRAME_IN_VIDEO];

    BufferStructureIdentity_t           PreviousBufferUseStatus;

    BufferStructureIdentity_t           DummyBufferUseStatus;
    STVID_PictureBufferHandle_t         DummyProgOrTopFieldBufferHandle;
    STVID_PictureBufferDataParams_t     DummyProgOrTopFieldBufferParams;
    STVID_PictureInfos_t                DummyProgOrTopFieldPictureInfos;
    STVID_PictureBufferHandle_t         DummyBotFieldBufferHandle;
    STVID_PictureBufferDataParams_t     DummyBotFieldBufferParams;
    STVID_PictureInfos_t                DummyBotFieldPictureInfos;

    STVID_MemoryProfile_t               CurrentVideoMemoryProfile;

    struct
    {
        STOS_Clock_t         MaxWaitOrderTime;

        /* To synchronise orders in the task */
        semaphore_t*         InputOrder_p;

        /* Ancillary Data */
        U8              AncillaryDataCurrentIndex;

        /*   */
        U8              PictureBufferCurrentIndex;
        BOOL            FirstStart;
        BOOL            SkipNextBufferAllocation;

        /* Additional information */
        U32                         ExtendedTemporalReference;
        STVID_TimeCode_t            TimeCode;
        STVID_PictureStructure_t    NextPictureStructureExpected;


    } ForTask;

    /* Run, Stop ... */
    VinDeviceStatus_t   Status;

    /* Device Mapped Base Address */
#ifdef ST_OSLINUX
    U32                     VinMappedWidth;
    void *                  VinMappedBaseAddress_p;  /* Address where will be mapped the driver registers */
#endif
    STVIN_InputMode_t   InputMode;

    BOOL                     InputWinAutoMode;     /* To enable configuring manually the input window */
    BOOL                     OutputWinAutoMode;     /* To enable configuring manually the output window */

} stvin_Device_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
/* StartCode should be U8 ! */

#define PushControllerCommand(Device_p, U8Var) vincom_PushCommand(&(((stvin_Device_t *) Device_p)->CommandsBuffer), ((U8) (U8Var)))
#define PopControllerCommand(Device_p, U8Var_p) vincom_PopCommand(&(((stvin_Device_t *) Device_p)->CommandsBuffer), ((U8 *) (U8Var_p)))

#define PushVSyncControllerCommand(Device_p, U8Var) vincom_PushCommand(&(((stvin_Device_t *) Device_p)->VSyncCommandsBuffer), ((U8) (U8Var)))
#define PopVSyncControllerCommand(Device_p, U8Var_p) vincom_PopCommand(&(((stvin_Device_t *) Device_p)->VSyncCommandsBuffer), ((U8 *) (U8Var_p)))

#define PushInterruptCommand(Device_p, U32Var) vincom_PushCommand32(&(((stvin_Device_t *) Device_p)->InterruptsBuffer), ((U32) (U32Var)))
#define PopInterruptCommand(Device_p, U32Var_p) vincom_PopCommand32(&(((stvin_Device_t *) Device_p)->InterruptsBuffer), ((U32 *) (U32Var_p)))


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIN_INIT_H */

/* End of vin_init.h */
