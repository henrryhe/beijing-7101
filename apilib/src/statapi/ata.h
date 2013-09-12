/************************************************************************

Source file name : ata.h

Description:       header file of the driver's middle layer

COPYRIGHT (C) STMicroelectronics 2000

************************************************************************/

#ifndef __STATAPI_ATA_H
#define __STATAPI_ATA_H

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"
#include "stevt.h"
#include "statapi.h"
#include "hal_atapi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define        TWO_MS                (ST_GetClocksPerSecond() / 500)

#define        ATAPI_NUM_EVENTS        4

/*Bug 50836*/
/* Ticks required for 400ns delay*/
#define        TICKS_FOR_400NS         (( ST_GetClocksPerSecond() + ( 2500000 - \
										(ST_GetClocksPerSecond() % 2500000)))/2500000)
									   /*  c= ((a +( b - ((a%b) ))/b ) */

#define        WAIT400NS             {    clock_t time;                 \
                                          time=time_now();              \
                                          while(time_now() < time_plus(time,TICKS_FOR_400NS))\
                                           break;\
                                        }

#define ATA_TIMEOUT                  0x00020000
                                /* Timeout for polling bits
                                    and when killing the BAT   */
#define ATAPI_TIMEOUT                0x00100000
                            /* different TimeOut because ATAPI drives
                                    are slower*/

#if defined( ARCHITECTURE_ST40 )
#define TRANSLATE_PHYS_ADD(_add_)               ST40_PHYS_ADDR((_add_))
#define TRANSLATE_LOG_ADD(_add_, _region_)      (((U32)(_add_) | ((U32) (_region_) & ~ST40_PHYS_MASK )))
#define NONCACHED_TRANSLATION(_add_)            ST40_NOCACHE_NOTRANSLATE((_add_))
#endif

#define   SECTOR_WSIZE        256
#define   SECTOR_BSIZE        512
#define	  DVD_SECTOR_BSIZE    2048
#define	  DVD_SECTOR_WSIZE    1024
#define	  CD_SECTOR_BSIZE     2352
#define	  CD_SECTOR_WSIZE     1176
#define   READ_CD_OPCODE      0xBE /*Opcode for Reading from CD*/

#define   DEVICE_0               0
#define   DEVICE_1               1
#define   DEVICE_SELECT_1_MASK   0x10
#define   DEVICE_NONE            255


/*  Command Types */

/* This bit should be well out of the way of the values below. */
#define         ATA_EXT_BIT             128

#define         ATA_CMD_NODATA          1
#define         ATA_CMD_NODATA_EXT      (ATA_CMD_NODATA | ATA_EXT_BIT)
#define         ATA_CMD_PIOIN           2
#define         ATA_CMD_PIOIN_EXT       (ATA_CMD_PIOIN | ATA_EXT_BIT)
#define         ATA_CMD_PIOOUT          3
#define         ATA_CMD_PIOOUT_EXT      (ATA_CMD_PIOOUT | ATA_EXT_BIT)
#define         ATA_CMD_DMAIN           4
#define         ATA_CMD_DMAIN_EXT       (ATA_CMD_DMAIN | ATA_EXT_BIT)
#define         ATA_CMD_DMAOUT          5
#define         ATA_CMD_DMAOUT_EXT      (ATA_CMD_DMAOUT | ATA_EXT_BIT)
#define         ATA_CMD_NOT_SUPPORTED   6

/* Packet commands cannot be 48-bit (extended) - ATA-6 spec. */
#define         ATA_PKT_NODATA          6
#define         ATA_PKT_PIOIN           7
#define         ATA_PKT_PIOOUT          8
#define         ATA_PKT_DMAIN           9
#define         ATA_PKT_DMAOUT          10

#define         HARD_RESET              11
#define         SOFT_RESET              12
#define         ATA_EXEC_DIAG           13

#define         ERROR_REG_ABORT_MASK     0x02
#define         LBA_SET_BIT              0x40

#define         EXTERROR_CRCERROR       0xa0
#define         EXTERROR_DMAABORT       0xa1

/*Exported Variables ---------------------------------------------------------*/

/*Exported macros    ---------------------------------------------------------*/

#define PIOMODE_TO_INDEX(Mode)      Mode - STATAPI_PIO_MODE_0
#define UDMAMODE_TO_INDEX(Mode)     Mode - STATAPI_DMA_UDMA_MODE_0
#define MWDMAMODE_TO_INDEX(Mode)    Mode - STATAPI_DMA_MWDMA_MODE_0

/*Exported Constants ---------------------------------------------------------*/

typedef enum
{
     UNKNOWN_DEVICE,
     NONE_DEVICE,
     ATA_DEVICE,
     ATAPI_DEVICE
}DeviceInBus_t;
/*Exported Types---------------------------------------------------------*/
typedef struct {

    U8              Error;
    U8              Status;
    U8              CylLow;
    U8              CylHigh;
    U8              DevHead;
    U8              SecCount;
    U32             BytesRW;
} ata_CmdStatus_t;

typedef struct {
    U8              DevCtrl;
    U8              CmdType;
    U8              Feature;
    U8              SecCount;
    U8              SecNum;
    U8              CylLow;
    U8              CylHigh;

    /* Used for LBA-48 addressing; many more FIFO spaces and it might be
     * worth storing as an array.
     */
    U8              SecCount2;
    U8              SecNum2, CylLow2, CylHigh2;
    U8              DevHead;
    U8              CommandCode;
    U8             *DataBuffer;
    U32             BufferSize;
    U8              MultiCnt;
    U32            *BytesRW;
    union {
        STATAPI_CmdStatus_t  *CmdStat;
        STATAPI_PktStatus_t  *PktStat;
    } Stat;
    U8              Pkt[16];
    BOOL            UseDMA;     /* Passed to hal_RegInBlock/OutBlock */
} ata_Cmd_t;

typedef struct
{
    BOOL                Opened;
    BOOL                IsModeSet;
    BOOL                Notify;
    BOOL                Abort;
    STATAPI_Params_t    DeviceParams;
    U8                  CmdType;
    ata_Cmd_t           CmdToSend;
    U8                  MultiCnt;
    semaphore_t         *EndCmdSem_p;
    BOOL                DmaEnabled;
    U8                  PktSize;

    /* Stored (user-specified) timing values; not really expected to be used */
    STATAPI_PioTiming_t         PioTimingOverrides[5];
    STATAPI_UltraDmaTiming_t    UltraDmaTimingOverrides[5];
    STATAPI_MwDmaTiming_t       MwDmaTimingOverrides[3];
    BOOL                PioTimingOverridden[5];
    BOOL                UDMATimingOverridden[5];
    BOOL                MWDMATimingOverridden[3];
} ata_DevCtrlBlock_t;  /* That's the structure behind the STATAPI handle*/

struct ata_ControlBlock_s
{
    ST_DeviceName_t    DeviceName;
    STATAPI_Device_t   DeviceType;
    ST_DeviceName_t    EvtDeviceName;
    STEVT_Handle_t     EvtHndl;
    STEVT_EventID_t    EvtIds[ATAPI_NUM_EVENTS];
    ata_DevCtrlBlock_t Handles[2];
    hal_Handle_t       *HalHndl;
    U8                 UsedHandles;
    U8                 DeviceSelected;
    DeviceInBus_t      DevInBus[2];
    semaphore_t        *BusMutexSemaphore_p;
    semaphore_t        *BATSemaphore_p;
    BOOL               BusBusy;
    BOOL               Terminate;
    U8                 LastExtendedErrorCode;
   /* For bus access task */
	void               *BusTaskStack;
    task_t             *BAT_p;
    tdesc_t             BusTaskDesc; 
    partition_t        *DriverPartition;    
#if defined (ATAPI_FDMA) 
    partition_t        *NCachePartition;
#endif
    struct ata_ControlBlock_s *Next_p;
};

typedef struct ata_ControlBlock_s ata_ControlBlock_t;

/*Exported functions---------------------------------------------------------*/

/* ATA Control Interface */
BOOL ata_ctrl_SelectDevice(ata_ControlBlock_t* Ata_p, U8 Device);
BOOL ata_ctrl_SoftReset(ata_ControlBlock_t* Ata_p);
BOOL ata_ctrl_Probe(ata_ControlBlock_t *Ata_p);
BOOL ata_ctrl_HardReset(ata_ControlBlock_t *Ata_p);

/* Device/Protocol Setup */

BOOL ata_bus_Acquire(ata_ControlBlock_t *Ata_p);
BOOL ata_bus_Release(ata_ControlBlock_t *Ata_p);

/* ATA Command Interface  */
BOOL ata_cmd_NoData(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_cmd_PioIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_cmd_PioOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_cmd_DmaIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_cmd_DmaOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_cmd_ExecuteDiagnostics(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);

/* ATA Packet Interface  */
BOOL ata_packet_NoData(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_packet_PioIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_packet_PioOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_packet_DmaIn(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_packet_DmaOut(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);
BOOL ata_packet_ExecuteDiagnostics(ata_ControlBlock_t *Ata_p, ata_Cmd_t *Cmd_p);

/* Internal functions */
BOOL WaitForBit(hal_Handle_t *HalHndl,ATA_Register_t regNo,U8 bitNo,
                U8 expected_val);
BOOL WaitForBitPkt(hal_Handle_t *HalHndl,ATA_Register_t regNo,U8 bitNo,
                U8 expected_val);
ata_ControlBlock_t *ATAPI_GetControlBlockFromName(const ST_DeviceName_t DeviceName);
ata_ControlBlock_t *ATAPI_GetControlBlockFromHandle(STATAPI_Handle_t Handle);                

#ifdef __cplusplus
}
#endif

#endif /* _STATAPI_ATA_H */
