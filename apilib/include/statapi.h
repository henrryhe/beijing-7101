/************************************************************************

Source file name : statapi.h

Description:       Interface to ATA/ATAPI driver.

COPYRIGHT (C) STMicroelectronics 2000

************************************************************************/

#ifndef __STATAPI_H
#define __STATAPI_H

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Variables ----------------------------------------------------- */
/* Exported macros    ----------------------------------------------------- */
/* Exported Constants ----------------------------------------------------- */

#define STATAPI_DRIVER_ID            144
#define STATAPI_DRIVER_BASE          ( STATAPI_DRIVER_ID << 16 )

#define STATAPI_REVISION	    "PRJ-STATAPI-REL_2.1.0       "

/* External return codes */
#define STATAPI_ERROR_BASE                       STATAPI_DRIVER_BASE

#define STATAPI_ERROR_CMD_ABORT                 ( STATAPI_ERROR_BASE )
#define STATAPI_ERROR_CMD_ERROR                 ( STATAPI_ERROR_BASE +  1 )
#define STATAPI_ERROR_DEVICE_NOT_PRESENT        ( STATAPI_ERROR_BASE +  2 )
#define STATAPI_ERROR_PKT_CHECK                 ( STATAPI_ERROR_BASE +  3 )
#define STATAPI_ERROR_PKT_NOT_SUPPORTED         ( STATAPI_ERROR_BASE +  4 )
#define STATAPI_ERROR_USER_ABORT                ( STATAPI_ERROR_BASE +  5 )
#define STATAPI_ERROR_TERMINATE_BAT             ( STATAPI_ERROR_BASE +  6 )
#define STATAPI_ERROR_DIAGNOSTICS_FAIL          ( STATAPI_ERROR_BASE +  7 )
#define STATAPI_ERROR_MODE_NOT_SET              ( STATAPI_ERROR_BASE +  8 )
#define STATAPI_ERROR_PROTOCOL_NOT_SUPPORTED    ( STATAPI_ERROR_BASE +  9 )
#define STATAPI_ERROR_CRC_ERROR                 ( STATAPI_ERROR_BASE + 10 )

#ifdef ATAPI_POLLING 
    #define ATAPI_USING_INTERRUPTS      0  
    
#else
    #define ATAPI_USING_INTERRUPTS      1  
#endif

/*Exported events    ---------------------------------------------------------*/
typedef enum STATAPI_Event_e
{
    STATAPI_HARD_RESET_EVT          = STATAPI_DRIVER_BASE,
    STATAPI_SOFT_RESET_EVT          = STATAPI_DRIVER_BASE + 1,
    STATAPI_CMD_COMPLETE_EVT        = STATAPI_DRIVER_BASE + 2,
    STATAPI_PKT_COMPLETE_EVT        = STATAPI_DRIVER_BASE + 3
} STATAPI_Event_t;

typedef enum STATAPI_CmdCode_e
{
    STATAPI_CMD_CFA_ERASE_SECTORS=0,
    STATAPI_CMD_CFA_REQUEST_EXT_ERR_CODE,
    STATAPI_CMD_CFA_TRANSLATE_SECTOR,
    STATAPI_CMD_CFA_WRITE_MULTIPLE_WO_ERASE,
    STATAPI_CMD_CFA_WRITE_SECTORS_WO_ERASE,
    STATAPI_CMD_CHECK_POWER_MODE,
    STATAPI_CMD_DEVICE_RESET,
    STATAPI_CMD_EXECUTE_DEVICE_DIAGNOSTIC,
    STATAPI_CMD_FLUSH_CACHE,
    STATAPI_CMD_IDENTIFY_DEVICE,            /* 9 */
    STATAPI_CMD_IDENTIFY_PACKET_DEVICE,
    STATAPI_CMD_IDLE,
    STATAPI_CMD_IDLE_IMMEDIATE,
    STATAPI_CMD_INITIALIZE_DEVICE_PARAMETERS,
    STATAPI_CMD_MEDIA_EJECT,
    STATAPI_CMD_NOP,                    
    STATAPI_CMD_PACKET,
    STATAPI_CMD_READ_BUFFER,
    STATAPI_CMD_READ_DMA,
    STATAPI_CMD_READ_DMA_QUEUED,            /* 19 */          
    STATAPI_CMD_READ_MULTIPLE,              
    STATAPI_CMD_READ_SECTORS,
    STATAPI_CMD_READ_VERIFY_SECTORS,
    STATAPI_CMD_SEEK,
    STATAPI_CMD_SET_FEATURES,           
    STATAPI_CMD_SET_MULTIPLE_MODE,
    STATAPI_CMD_SLEEP,
    STATAPI_CMD_STANDBY,                   
    STATAPI_CMD_STANDBY_IMMEDIATE,          
    STATAPI_CMD_WRITE_BUFFER,               /* 29 */
    STATAPI_CMD_WRITE_DMA,
    STATAPI_CMD_WRITE_DMA_QUEUED,
    STATAPI_CMD_WRITE_MULTIPLE,
    STATAPI_CMD_WRITE_SECTORS,
    STATAPI_CMD_SECURITY_SET_PASSWORD,
    STATAPI_CMD_SECURITY_DISABLE_PASSWORD,
    STATAPI_CMD_SECURITY_UNLOCK,
    STATAPI_CMD_READ_SECTORS_EXT,
    STATAPI_CMD_READ_DMA_EXT,
    STATAPI_CMD_READ_DMA_QUEUED_EXT,        /* 39 */
    STATAPI_CMD_READ_NATIVE_MAX_ADDRESS_EXT,
    STATAPI_CMD_READ_MULTIPLE_EXT,
    STATAPI_CMD_WRITE_SECTORS_EXT,
    STATAPI_CMD_WRITE_DMA_EXT,
    STATAPI_CMD_WRITE_DMA_QUEUED_EXT,
    STATAPI_CMD_SET_MAX_ADDRESS_EXT,
    STATAPI_CMD_WRITE_MULTIPLE_EXT,
    STATAPI_CMD_READ_VERIFY_SECTORS_EXT,
    STATAPI_CMD_FLUSH_CACHE_EXT,
    STATAPI_CMD_SMART_IN,                   /* 49 */
    STATAPI_CMD_SMART_OUT,
    STATAPI_CMD_SMART_NODATA
} STATAPI_CmdCode_t;

/*Enumeration type that defines all supported ATAPI interfaces host devices*/
typedef enum STATAPI_Device_e
{
    STATAPI_EMI_PIO4,
    STATAPI_HDD_UDMA4,
    STATAPI_SATA
} STATAPI_Device_t;

/* Enumeration type that defines the possible device addresses.*/

typedef enum STATAPI_DeviceAddr_e
{
    STATAPI_DEVICE_0=0,
    STATAPI_DEVICE_1
    
} STATAPI_DeviceAddr_t;

/*Enumeration type that defines the possible device drive type (ATA or ATAPI).*/
typedef enum STATAPI_DriveType_e
{
    STATAPI_ATA_DRIVE=0,
    STATAPI_ATAPI_DRIVE,
    STATAPI_UNKNOWN_DRIVE
    
} STATAPI_DriveType_t;

/*Enumeration type that defines the supported Multi-Word and Ultra DMA modes.*/

typedef enum STATAPI_DmaMode_e
{
    STATAPI_DMA_MWDMA_MODE_0,
    STATAPI_DMA_MWDMA_MODE_1,
    STATAPI_DMA_MWDMA_MODE_2,
    STATAPI_DMA_UDMA_MODE_0,
    STATAPI_DMA_UDMA_MODE_1,
    STATAPI_DMA_UDMA_MODE_2,
    STATAPI_DMA_UDMA_MODE_3,
    STATAPI_DMA_UDMA_MODE_4,
    STATAPI_DMA_UDMA_MODE_5,
    STATAPI_DMA_NOT_SUPPORTED
    
} STATAPI_DmaMode_t;

/*Enumeration type that defines the supported PIO modes.*/

typedef enum STATAPI_PioMode_e
{
    STATAPI_PIO_MODE_0,
    STATAPI_PIO_MODE_1,
    STATAPI_PIO_MODE_2,
    STATAPI_PIO_MODE_3,
    STATAPI_PIO_MODE_4
    
} STATAPI_PioMode_t;

/*Enumeration type defining all supported packet op-codes from the ATAPI
specification*/

typedef enum STATAPI_PktOpCode_e
{
    STATAPI_PKT_BLANK=0,
    STATAPI_PKT_CLOSE_TRACK_RZONE_SESSION_BORDER,
    STATAPI_PKT_COMPARE_SPC,
    STATAPI_PKT_ERASE_10_SBC,
    STATAPI_PKT_FLUSH_CACHE,
    STATAPI_PKT_FORMAT_UNIT,
    STATAPI_PKT_GET_CONFIGURATION,
    STATAPI_PKT_GET_EVENT_STATUS_NOTIFICATION,
    STATAPI_PKT_GET_PERFORMANCE,
    STATAPI_PKT_INQUIRY,                /* 9 */
    STATAPI_PKT_LOAD_UNLOAD_MEDIUM,
    STATAPI_PKT_LOCK_UNLOCK_CACHE_SBC,
    STATAPI_PKT_LOG_SELECT_SPC,
    STATAPI_PKT_LOG_SENSE_SPC,
    STATAPI_PKT_MECHANISM_STATUS,
    STATAPI_PKT_MODE_SELECT_10,
    STATAPI_PKT_MODE_SENSE_10,
    STATAPI_PKT_PAUSE_RESUME,
    STATAPI_PKT_PLAY_AUDIO_10,
    STATAPI_PKT_PLAY_AUDIO_MSF,         /* 19 */
    STATAPI_PKT_PLAY_CD,
    STATAPI_PKT_PRE_FETCH_SBC,
    STATAPI_PKT_PREVENT_ALLOW_MEDIUM_REMOVAL,
    STATAPI_PKT_READ_10,
    STATAPI_PKT_READ_12,
    STATAPI_PKT_READ_BUFFER_SPC,
    STATAPI_PKT_READ_CAPACITY,
    STATAPI_PKT_READ_CD,
    STATAPI_PKT_READ_CD_MSF,
    STATAPI_PKT_READ_DISC_INFORMATION,  /* 29 */
    STATAPI_PKT_READ_DVD_STRUCTURE,
    STATAPI_PKT_READ_FORMAT_CAPACITIES,
    STATAPI_PKT_READ_HEADER,
    STATAPI_PKT_READ_SUBCHANNEL,
    STATAPI_PKT_READ_TOC_PMA_ATIP,
    STATAPI_PKT_READ_TRACK_RZONE_INFORMATION,
    STATAPI_PKT_RECEIVE_DIAGNOSTIC_RESULTS_SPC,
    STATAPI_PKT_RELEASE_6_SPC,
    STATAPI_PKT_RELEASE_10_SPC,
    STATAPI_PKT_REPAIR_RZONE,           /* 39 */
    STATAPI_PKT_REPORT_KEY,
    STATAPI_PKT_REQUEST_SENSE,
    STATAPI_PKT_RESERVE_6_SPC,
    STATAPI_PKT_RESERVE_10_SPC,
    STATAPI_PKT_RESERVE_TRACK_RZONE,
    STATAPI_PKT_SCAN,
    STATAPI_PKT_SEEK,
    STATAPI_PKT_SEND_CUE_SHEET,         
    STATAPI_PKT_SEND_DIAGNOSTIC_SPC,
    STATAPI_PKT_SEND_DVD_STRUCTURE,     /* 49 */
    STATAPI_PKT_SEND_EVENT,
    STATAPI_PKT_DVD_CSS_SEND_KEY,
    STATAPI_PKT_SEND_OPC_INFORMATION,
    STATAPI_PKT_SET_READ_AHEAD,
    STATAPI_PKT_SET_STREAMING,
    STATAPI_PKT_START_STOP_UNIT,        
    STATAPI_PKT_STOP_PLAY_SCAN,
    STATAPI_PKT_TEST_UNIT_READY,
    STATAPI_PKT_VERIFY_10,
    STATAPI_PKT_WRITE_10,               /* 59 */
    STATAPI_PKT_WRITE_AND_VERIFY_10,
    STATAPI_PKT_WRITE_BUFFER_SPC
    
} STATAPI_PktOpCode_t;    

/*Exported Types --------------------------------------------------------- */

typedef void* STATAPI_Handle_t;

/***************************************************
Structure defining the device and driver capabilities.
******************************************************/
typedef struct STATAPI_Capability_s
{
    STATAPI_Device_t DeviceType;
    U32 SupportedPioModes;
    U32 SupportedDmaModes;
} STATAPI_Capability_t;


/************************************************ 
Structure defining the ATA command response status.
************************************************/
typedef struct STATAPI_CmdStatus_s
{
    U8  Error;
    U8  Status;
    U32 LBA;
    U16 LBAExtended;           /* The extra 16-bits for LBA-48 */
    U8  Sector;
    U8  CylinderLow;
    U8  CylinderHigh;
    U8  Head;
    U16 SectorCount;
} STATAPI_CmdStatus_t;

/************************************************ 
Structure defining an ATA command.
************************************************/
typedef struct STATAPI_Cmd_s
{
    STATAPI_CmdCode_t CmdCode;
    U8  Features;
    BOOL UseLBA;        /* Ignored if CMDCODE is one of the ext. set */
    U32 LBA;
    U16 LBAExtended;           /* The extra 16-bits for LBA-48 */
    U8  Sector;
    U8  CylinderLow;
    U8  CylinderHigh;
    U8  Head;
    U16 SectorCount;
} STATAPI_Cmd_t;

/***************************************************
Structure defining the register map for overrides.
****************************************************/
typedef struct STATAPI_RegisterMap_s
{
    U32     AlternateStatus;
    U32     Data;
    U32     Error;
    U32     SectorCount;
    U32     SectorNumber;
    U32     CylinderLow;
    U32     CylinderHigh;
    U32     DeviceHead;
    U32     Status;
} STATAPI_RegisterMap_t;

/************************************************ 
Structure defining the parameters required to initialize the driver.
************************************************/
typedef struct STATAPI_InitParams_s
{
    STATAPI_Device_t    DeviceType;
    ST_Partition_t      *DriverPartition;
#ifdef ATAPI_FDMA
    ST_Partition_t      *NCachePartition;
#endif
    volatile U32        *BaseAddress;
    volatile U16*       HW_ResetAddress;

#ifdef STATAPI_SET_EMIREGISTER_MAP
    const STATAPI_RegisterMap_t *RegisterMap_p;
#endif
    
    U32                 InterruptNumber;
    U32                 InterruptLevel;
    ST_DeviceName_t     EVTDeviceName;

    /* Only valid for HDD_UDMA4 device */
    U32                 ClockFrequency;
} STATAPI_InitParams_t;


/************************************************
Structure defining the Multi-Word DMA mode timing 
parameters.
************************************************/
typedef struct STATAPI_MwDmaTiming_s
{
    U32 NotDIoRwAssertedT;
    U32 WriteDataHoldT;
    U32 DIoRwToDMAckHoldT;
    U32 NotDIoRNegatedT;
    U32 NotDIoWNegatedT;
    U32 CsToNotDIoRwT;
    U32 CsHoldT;
} STATAPI_MwDmaTiming_t;

/************************************************
Structure defining the PIO mode timing parameters.
************************************************/
typedef struct STATAPI_PioTiming_s
{
    U32 InitSequenceDelay;
    U32 IoRdySetupDelay;
    U32 WaitTime;
    U32 WriteDelay;
    U32 ReadDelay;
    U32 AddressHoldDelay;
    U32 WriteEnableOutDelay;
    U32 WriteRecoverDelay;
    U32 ReadRecoverDelay;
} STATAPI_PioTiming_t;

/************************************************/

/***************************************************
Structure defining the Ultra DMA mode timing parameters.
****************************************************/
typedef struct STATAPI_UltraDmaTiming_s
{
    U32 AckT;
    U32 InitEnvelopeT;
    U32 MinAssertStopNegateT;
    U32 MinInterlockT;
    U32 LimitedInterlockT;          /* 5514 only */

    U32 DataOutSetupT;
    U32 DataOutHoldT;

    U32 HostStrobeToStopSetupT;
    U32 ReadyToFinalStrobeT;    

    U32 CRCSetupT;                  /* 5528 and 8010 only */
} STATAPI_UltraDmaTiming_t;

/**********************************************************
Structure defining the DMA timing parameters.
*********************************************************/
typedef struct STATAPI_DmaTiming_s
{
    union
    {
        STATAPI_MwDmaTiming_t MwDmaTimingParams;
        STATAPI_UltraDmaTiming_t UltraDmaTimingParams;
    } DmaTimingParams;
} STATAPI_DmaTiming_t;
 
/*****************************************************
Structure defining the parameters used when opening a 
handle to the driver.
********************************************************/
typedef struct STATAPI_OpenParams_s
{
    STATAPI_DeviceAddr_t DeviceAddress;
} STATAPI_OpenParams_t;

/************************************************
    Structure defining an ATAPI packet.
************************************************/
typedef struct STATAPI_Packet_s
{
    STATAPI_PktOpCode_t OpCode;
    U8 Data[15];
    
} STATAPI_Packet_t;

/************************************************
Structure defining the device parameters.
************************************************/
typedef struct STATAPI_Params_s
{
    STATAPI_DeviceAddr_t DeviceAddress;
    U32 SupportedPioModes;
    U32 SupportedDmaModes;
    STATAPI_PioMode_t CurrentPioMode;
    STATAPI_DmaMode_t CurrentDmaMode;
    
} STATAPI_Params_t;

/****************************************************
Structure defining the ATAPI packet response status.
************************************************/
typedef struct STATAPI_PktStatus_s
{
    U8 Error;
    U8 InterruptReason;
    U8 Status;
} STATAPI_PktStatus_t;


/************************************************
Structure defining the driver termination parameters.
************************************************/
typedef struct STATAPI_TermParams_s
{
    BOOL ForceTerminate;
} STATAPI_TermParams_t;

/*****************************************************
Structure defining the parameters returned by the Event
 Handler driver for all ST ATAPI events.
********************************************************/
typedef struct STATAPI_EvtParams_s
{
    STATAPI_DeviceAddr_t DeviceAddress;
    ST_ErrorCode_t Error;
    union
    {
        struct
        {   
            U32 Reserved;
        } SoftResetParams;
        
        struct
        {   
           U32 Reserved;
        } HardResetParams;
        
        struct
        {
          STATAPI_CmdStatus_t *CmdStatus;
        } CmdCompleteParams;
        
        struct
        {
           STATAPI_PktStatus_t *PktStatus;
        } PktCompleteParams;
        
    } EvtParams;
    
} STATAPI_EvtParams_t;

/*Exported Functions ---------------------------------------------------*/

/* Abort */
ST_ErrorCode_t STATAPI_Abort(STATAPI_Handle_t Handle);

/* Close */
ST_ErrorCode_t STATAPI_Close(STATAPI_Handle_t Handle);

/*Command In*/
ST_ErrorCode_t STATAPI_CmdIn(STATAPI_Handle_t Handle,
                            const STATAPI_Cmd_t *Cmd,
                            U8 *Data,
                            U32 BufferSize,
                            U32 *NumberRead,
                            STATAPI_CmdStatus_t *CmdStatus);

/*Command no data*/                              
ST_ErrorCode_t STATAPI_CmdNoData(STATAPI_Handle_t Handle,
                                 const STATAPI_Cmd_t *Cmd,
                                 STATAPI_CmdStatus_t *CmdStatus);   

/*Command Out*/
ST_ErrorCode_t STATAPI_CmdOut(STATAPI_Handle_t Handle,
                              const STATAPI_Cmd_t *Cmd,
                              const U8 *Data,
                              U32 NumberToWrite,
                              U32 *NumberWritten,
                              STATAPI_CmdStatus_t *CmdStatus);

/* Pass back some device registers */
ST_ErrorCode_t STATAPI_GetATAStatus(STATAPI_Handle_t Handle,
                                    U8 *Status,
                                    U8 *AltStatus,
                                    U8 *Error);

/*Get Capabilities*/
ST_ErrorCode_t STATAPI_GetCapability(const ST_DeviceName_t DeviceName,
                                     STATAPI_Capability_t *Capability); 
                                     
/*Get Drive type */
ST_ErrorCode_t STATAPI_GetDriveType(STATAPI_Handle_t Handle,
                                    STATAPI_DriveType_t *Type); 
                                                                         
ST_ErrorCode_t STATAPI_GetExtendedError(STATAPI_Handle_t Handle,
                                        U8 *ErrorCode);

/*Get Params*/
ST_ErrorCode_t STATAPI_GetParams(STATAPI_Handle_t Handle,
                                 STATAPI_Params_t *Params); 

                               
                                                                                  

/*Get DMA details */
ST_ErrorCode_t STATAPI_GetDmaMode(STATAPI_Handle_t Handle,
                                  STATAPI_DmaMode_t *Mode,
                                  STATAPI_DmaTiming_t *TimingParams);

/*Get PIO timings*/
ST_ErrorCode_t STATAPI_GetPioMode(STATAPI_Handle_t Handle,
                                  STATAPI_PioMode_t *Mode,
                                  STATAPI_PioTiming_t *TimingParams);
                                  /*Set DMA timings*/
ST_ErrorCode_t STATAPI_SetDmaTiming(STATAPI_Handle_t Handle,
                                    STATAPI_DmaTiming_t *TimingParams);
                                    
                                    /*Set PIO timings*/                                  
ST_ErrorCode_t STATAPI_SetPioTiming(STATAPI_Handle_t Handle,
                                    STATAPI_PioTiming_t *TimingParams);
                            

/* Get register map */
ST_ErrorCode_t STATAPI_GetRegisters(STATAPI_RegisterMap_t *RegisterMap_p);

/* GetRevision */
ST_Revision_t STATAPI_GetRevision(void);

/* Hard Reset */
ST_ErrorCode_t STATAPI_HardReset(const ST_DeviceName_t DeviceName);

/* Init */
ST_ErrorCode_t STATAPI_Init(const ST_DeviceName_t DeviceName,
                            const STATAPI_InitParams_t *InitParams);

/* Open */
ST_ErrorCode_t STATAPI_Open(const ST_DeviceName_t DeviceName,
                            const STATAPI_OpenParams_t *OpenParams,
                            STATAPI_Handle_t *Handle);
 
/*Packet In*/     
ST_ErrorCode_t STATAPI_PacketIn(STATAPI_Handle_t Handle,
                                const STATAPI_Packet_t *Packet,
                                U8 *Data,
                                U32 NumberToRead,
                                U32 *NumberRead,
                                STATAPI_PktStatus_t *PktStatus);                                  
/*Packet no data*/
ST_ErrorCode_t STATAPI_PacketNoData(STATAPI_Handle_t Handle,
                                const STATAPI_Packet_t *Packet,
                                STATAPI_PktStatus_t *PktStatus);
/*Packet Out*/                                
ST_ErrorCode_t STATAPI_PacketOut(STATAPI_Handle_t Handle,
                                 const STATAPI_Packet_t *Packet,
                                 const U8 *Data,
                                 U32 NumberToWrite,
                                 U32 *NumberWritten,
                                 STATAPI_PktStatus_t *PktStatus);

/* Reset the HDD interface, if available */
ST_ErrorCode_t STATAPI_ResetInterface(void);

/*Set DMA mode*/
ST_ErrorCode_t STATAPI_SetDmaMode(STATAPI_Handle_t Handle,
                                  STATAPI_DmaMode_t Mode);                                 
                                  
/*Set PIO mode*/
ST_ErrorCode_t STATAPI_SetPioMode(STATAPI_Handle_t Handle,
                                  STATAPI_PioMode_t Mode);                                    


/* Register map change. This might actually have been better off in
 * Init, but that would require a major version rev; maybe move it from
 * here to there when we can't avoid one.
 */
ST_ErrorCode_t STATAPI_SetRegisters(const STATAPI_RegisterMap_t *RegisterMap_p);

/*Software Reset*/
ST_ErrorCode_t STATAPI_SoftReset(const ST_DeviceName_t DeviceName);      

/* Term */
ST_ErrorCode_t STATAPI_Term(const ST_DeviceName_t DeviceName,
                            const STATAPI_TermParams_t *TermParams);

#ifdef __cplusplus
}
#endif

#endif /* _STATAPI_H */
